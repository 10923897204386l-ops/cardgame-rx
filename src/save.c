#define _POSIX_C_SOURCE 200809L
#include "save.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

enum {
    SAVE_HEADER_BYTES = 48,
    SAVE_CARD_BYTES = 2, /* suit:4 rank:4 face:8 packed as u16 */
    /* piles: stock,waste,4 foundations,7 tableau = 13 count bytes + 52 cards */
};

static uint32_t crc32_table[256];
static bool crc_ready;

static void crc_init(void) {
    if (crc_ready) return;
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int k = 0; k < 8; ++k)
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }
    crc_ready = true;
}

static uint32_t crc32(const uint8_t *data, size_t len) {
    crc_init();
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i)
        c = crc32_table[(c ^ data[i]) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static void put_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}
static void put_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}
static void put_u64(uint8_t *p, uint64_t v) {
    for (int i = 0; i < 8; ++i) p[i] = (uint8_t)((v >> (8 * i)) & 0xFFu);
}
static uint16_t get_u16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t get_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t get_u64(const uint8_t *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= (uint64_t)p[i] << (8 * i);
    return v;
}

const char *save_status_text(SaveStatus status) {
    switch (status) {
    case SAVE_OK: return "OK";
    case SAVE_ERR_IO: return "I/O error";
    case SAVE_ERR_FORMAT: return "Invalid save format";
    case SAVE_ERR_VERSION: return "Unsupported save version";
    case SAVE_ERR_CHECKSUM: return "Save checksum failed";
    case SAVE_ERR_BOUNDS: return "Save bounds invalid";
    case SAVE_ERR_STATE: return "Save game state invalid";
    case SAVE_ERR_NO_DIR: return "No writable save directory";
    case SAVE_ERR_EMPTY: return "Empty slot";
    default: return "Unknown save error";
    }
}

static bool mkdir_p(const char *path) {
    char tmp[SAVE_PATH_MAX];
    size_t len = strlen(path);
    if (!path || !len || len >= sizeof tmp) return false;
    memcpy(tmp, path, len + 1);
    for (char *p = tmp + 1; *p; ++p) {
        if (*p != '/') continue;
        *p = 0;
        if (mkdir(tmp, 0700) != 0 && errno != EEXIST) return false;
        *p = '/';
    }
    if (mkdir(tmp, 0700) != 0 && errno != EEXIST) return false;
    return true;
}
bool save_data_dir(char *out, size_t out_size) {
    if (!out || out_size < 8) return false;
    const char *xdg = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");
    char base[SAVE_PATH_MAX];
    int nb;
    if (xdg && xdg[0])
        nb = snprintf(base, sizeof base, "%s/croupier-rx", xdg);
    else if (home && home[0])
        nb = snprintf(base, sizeof base, "%s/.local/share/croupier-rx", home);
    else
        nb = snprintf(base, sizeof base, "/tmp/croupier-rx-saves");
    if (nb < 0 || (size_t)nb >= sizeof base) return false;
    char saves[SAVE_PATH_MAX];
    int n = snprintf(saves, sizeof saves, "%s/saves", base);
    if (n < 0 || (size_t)n >= sizeof saves) return false;
    if (!mkdir_p(saves)) return false;
    if ((size_t)n + 1 > out_size) return false;
    memcpy(out, saves, (size_t)n + 1);
    return true;
}

bool save_autosave_path(char *out, size_t out_size) {
    char dir[SAVE_PATH_MAX];
    if (!save_data_dir(dir, sizeof dir)) return false;
    return (size_t)snprintf(out, out_size, "%s/autosave.crx", dir) < out_size;
}

bool save_slot_path(unsigned slot, char *out, size_t out_size) {
    if (slot >= SAVE_MANUAL_SLOTS) return false;
    char dir[SAVE_PATH_MAX];
    if (!save_data_dir(dir, sizeof dir)) return false;
    return (size_t)snprintf(out, out_size, "%s/slot_%02u.crx", dir, slot + 1) < out_size;
}

static uint16_t pack_card(CardState cs) {
    if (!card_valid(cs.card)) return 0xFFFFu;
    return (uint16_t)(((unsigned)cs.card.suit & 3u) | ((cs.card.rank & 0xFu) << 2) |
                      (cs.face_up ? 0x40u : 0u));
}

static bool unpack_card(uint16_t v, CardState *out) {
    if (v == 0xFFFFu || !out) return false;
    Card c = {(Suit)(v & 3u), (unsigned)((v >> 2) & 0xFu)};
    if (!card_valid(c)) return false;
    out->card = c;
    out->face_up = (v & 0x40u) != 0;
    return true;
}

static size_t encode_pile(uint8_t *dst, size_t cap, const Pile *p) {
    if (!p || p->count > DECK_SIZE || cap < 1 + p->count * SAVE_CARD_BYTES) return 0;
    dst[0] = (uint8_t)p->count;
    size_t o = 1;
    for (size_t i = 0; i < p->count; ++i) {
        uint16_t packed = pack_card(p->cards[i]);
        if (packed == 0xFFFFu) return 0;
        put_u16(dst + o, packed);
        o += 2;
    }
    return o;
}

static size_t decode_pile(const uint8_t *src, size_t len, Pile *p) {
    if (!src || !p || len < 1) return 0;
    size_t count = src[0];
    if (count > DECK_SIZE || len < 1 + count * SAVE_CARD_BYTES) return 0;
    *p = (Pile){0};
    p->count = count;
    size_t o = 1;
    for (size_t i = 0; i < count; ++i) {
        if (!unpack_card(get_u16(src + o), &p->cards[i])) return 0;
        o += 2;
    }
    return o;
}

static size_t encode_state(uint8_t *dst, size_t cap, const KlondikeState *s) {
    if (!s || cap < 1) return 0;
    size_t o = 0;
    const Pile *piles[13] = {
        &s->stock, &s->waste,
        &s->foundations[0], &s->foundations[1], &s->foundations[2], &s->foundations[3],
        &s->tableau[0], &s->tableau[1], &s->tableau[2], &s->tableau[3],
        &s->tableau[4], &s->tableau[5], &s->tableau[6]
    };
    for (int i = 0; i < 13; ++i) {
        size_t n = encode_pile(dst + o, cap - o, piles[i]);
        if (!n) return 0;
        o += n;
    }
    return o;
}

static size_t decode_state(const uint8_t *src, size_t len, KlondikeState *s) {
    if (!src || !s) return 0;
    *s = (KlondikeState){0};
    Pile *piles[13] = {
        &s->stock, &s->waste,
        &s->foundations[0], &s->foundations[1], &s->foundations[2], &s->foundations[3],
        &s->tableau[0], &s->tableau[1], &s->tableau[2], &s->tableau[3],
        &s->tableau[4], &s->tableau[5], &s->tableau[6]
    };
    size_t o = 0;
    for (int i = 0; i < 13; ++i) {
        size_t n = decode_pile(src + o, len - o, piles[i]);
        if (!n) return 0;
        o += n;
    }
    return o;
}

bool save_capture(SaveBlob *blob, const KlondikeGame *game, const SaveMeta *meta) {
    if (!blob || !game || !meta || !klondike_valid(&game->state)) return false;
    *blob = (SaveBlob){0};
    blob->meta = *meta;
    blob->meta.has_game = true;
    blob->meta.won = klondike_won(&game->state);
    blob->meta.undo_limit = (uint32_t)(game->undo_limit > UINT32_MAX ? UINT32_MAX : game->undo_limit);
    blob->state = game->state;
    size_t n = klondike_export_undo(game, blob->undo, SAVE_MAX_UNDO);
    blob->undo_count = (uint16_t)n;
    return true;
}

bool save_restore(const SaveBlob *blob, KlondikeGame *game, SaveMeta *meta_out) {
    if (!blob || !game || !blob->meta.has_game) return false;
    if (!klondike_valid(&blob->state)) return false;
    for (uint16_t i = 0; i < blob->undo_count && i < SAVE_MAX_UNDO; ++i)
        if (!klondike_valid(&blob->undo[i])) return false;

    klondike_dispose(game);
    game->state = blob->state;
    game->history = NULL;
    game->undo_depth = 0;
    if (blob->meta.undo_limit)
        klondike_set_undo_limit(game, blob->meta.undo_limit);
    if (!klondike_import_undo(game, blob->undo, blob->undo_count)) return false;
    if (meta_out) *meta_out = blob->meta;
    return true;
}

static size_t encode_blob(uint8_t *buf, size_t cap, const SaveBlob *blob) {
    if (!buf || !blob || cap < SAVE_HEADER_BYTES + 16) return 0;
    memset(buf, 0, SAVE_HEADER_BYTES);
    put_u32(buf + 0, SAVE_MAGIC);
    put_u16(buf + 4, (uint16_t)SAVE_VERSION);
    put_u16(buf + 6, 0); /* flags */
    put_u32(buf + 8, blob->meta.seed);
    put_u32(buf + 12, blob->meta.score);
    put_u32(buf + 16, blob->meta.move_count);
    put_u32(buf + 20, blob->meta.elapsed_ms);
    put_u32(buf + 24, blob->meta.undo_limit);
    put_u16(buf + 28, (uint16_t)blob->meta.mode);
    put_u16(buf + 30, blob->undo_count);
    put_u64(buf + 32, (uint64_t)blob->meta.saved_at);
    buf[40] = blob->meta.has_game ? 1u : 0u;
    buf[41] = blob->meta.won ? 1u : 0u;

    size_t o = SAVE_HEADER_BYTES;
    size_t n = encode_state(buf + o, cap - o, &blob->state);
    if (!n) return 0;
    o += n;
    for (uint16_t i = 0; i < blob->undo_count; ++i) {
        n = encode_state(buf + o, cap - o, &blob->undo[i]);
        if (!n) return 0;
        o += n;
    }
    if (o + 4 > cap) return 0;
    uint32_t sum = crc32(buf, o);
    put_u32(buf + o, sum);
    return o + 4;
}

static SaveStatus decode_blob(const uint8_t *buf, size_t len, SaveBlob *blob) {
    if (!buf || !blob || len < SAVE_HEADER_BYTES + 4) return SAVE_ERR_FORMAT;
    if (get_u32(buf + 0) != SAVE_MAGIC) return SAVE_ERR_FORMAT;
    uint16_t version = get_u16(buf + 4);
    if (version != SAVE_VERSION) return SAVE_ERR_VERSION;
    uint32_t file_crc = get_u32(buf + len - 4);
    if (crc32(buf, len - 4) != file_crc) return SAVE_ERR_CHECKSUM;

    *blob = (SaveBlob){0};
    blob->meta.seed = get_u32(buf + 8);
    blob->meta.score = get_u32(buf + 12);
    blob->meta.move_count = get_u32(buf + 16);
    blob->meta.elapsed_ms = get_u32(buf + 20);
    blob->meta.undo_limit = get_u32(buf + 24);
    blob->meta.mode = (SaveGameMode)get_u16(buf + 28);
    blob->undo_count = get_u16(buf + 30);
    blob->meta.saved_at = (time_t)get_u64(buf + 32);
    blob->meta.has_game = buf[40] != 0;
    blob->meta.won = buf[41] != 0;
    if (blob->undo_count > SAVE_MAX_UNDO) return SAVE_ERR_BOUNDS;
    if (blob->meta.mode != SAVE_MODE_KLONDIKE_DRAW1) return SAVE_ERR_BOUNDS;

    size_t o = SAVE_HEADER_BYTES;
    size_t n = decode_state(buf + o, len - 4 - o, &blob->state);
    if (!n) return SAVE_ERR_BOUNDS;
    o += n;
    for (uint16_t i = 0; i < blob->undo_count; ++i) {
        n = decode_state(buf + o, len - 4 - o, &blob->undo[i]);
        if (!n) return SAVE_ERR_BOUNDS;
        o += n;
    }
    if (o != len - 4) return SAVE_ERR_FORMAT;
    if (!blob->meta.has_game) return SAVE_ERR_EMPTY;
    if (!klondike_valid(&blob->state)) return SAVE_ERR_STATE;
    for (uint16_t i = 0; i < blob->undo_count; ++i)
        if (!klondike_valid(&blob->undo[i])) return SAVE_ERR_STATE;
    return SAVE_OK;
}

SaveStatus save_write_file(const char *path, const SaveBlob *blob) {
    if (!path || !blob) return SAVE_ERR_IO;
    /* Max: header + (1+52*2)*13 * (1+64 undos) + crc ≈ under 100KB. */
    enum { MAX_BYTES = 1 << 18 };
    uint8_t *buf = malloc(MAX_BYTES);
    if (!buf) return SAVE_ERR_IO;
    size_t n = encode_blob(buf, MAX_BYTES, blob);
    if (!n) { free(buf); return SAVE_ERR_BOUNDS; }

    char tmp[SAVE_PATH_MAX + 16];
    if ((size_t)snprintf(tmp, sizeof tmp, "%s.tmp.%d", path, (int)getpid()) >= sizeof tmp) {
        free(buf);
        return SAVE_ERR_IO;
    }
    FILE *f = fopen(tmp, "wb");
    if (!f) { free(buf); return SAVE_ERR_IO; }
    size_t wrote = fwrite(buf, 1, n, f);
    if (wrote != n || fflush(f) != 0) {
        fclose(f);
        unlink(tmp);
        free(buf);
        return SAVE_ERR_IO;
    }
    int fd = fileno(f);
    if (fd >= 0) (void)fsync(fd);
    if (fclose(f) != 0) { unlink(tmp); free(buf); return SAVE_ERR_IO; }
    free(buf);
    if (rename(tmp, path) != 0) { unlink(tmp); return SAVE_ERR_IO; }
    return SAVE_OK;
}

SaveStatus save_read_file(const char *path, SaveBlob *blob) {
    if (!path || !blob) return SAVE_ERR_IO;
    FILE *f = fopen(path, "rb");
    if (!f) return access(path, F_OK) == 0 ? SAVE_ERR_IO : SAVE_ERR_EMPTY;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return SAVE_ERR_IO; }
    long sz = ftell(f);
    if (sz < 0 || sz > (1 << 20)) { fclose(f); return SAVE_ERR_FORMAT; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return SAVE_ERR_IO; }
    uint8_t *buf = malloc((size_t)sz);
    if (!buf) { fclose(f); return SAVE_ERR_IO; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(buf); return SAVE_ERR_IO; }
    SaveStatus st = decode_blob(buf, got, blob);
    free(buf);
    return st;
}

SaveStatus save_write_autosave(const SaveBlob *blob) {
    char path[SAVE_PATH_MAX];
    if (!save_autosave_path(path, sizeof path)) return SAVE_ERR_NO_DIR;
    return save_write_file(path, blob);
}

SaveStatus save_read_autosave(SaveBlob *blob) {
    char path[SAVE_PATH_MAX];
    if (!save_autosave_path(path, sizeof path)) return SAVE_ERR_NO_DIR;
    return save_read_file(path, blob);
}

SaveStatus save_write_slot(unsigned slot, const SaveBlob *blob) {
    char path[SAVE_PATH_MAX];
    if (!save_slot_path(slot, path, sizeof path)) return SAVE_ERR_BOUNDS;
    return save_write_file(path, blob);
}

SaveStatus save_read_slot(unsigned slot, SaveBlob *blob) {
    char path[SAVE_PATH_MAX];
    if (!save_slot_path(slot, path, sizeof path)) return SAVE_ERR_BOUNDS;
    return save_read_file(path, blob);
}

SaveStatus save_probe_autosave(SaveMeta *meta) {
    SaveBlob blob;
    SaveStatus st = save_read_autosave(&blob);
    if (st == SAVE_OK && meta) *meta = blob.meta;
    return st;
}

SaveStatus save_probe_slot(unsigned slot, SaveMeta *meta) {
    SaveBlob blob;
    SaveStatus st = save_read_slot(slot, &blob);
    if (st == SAVE_OK && meta) *meta = blob.meta;
    return st;
}

bool save_delete_autosave(void) {
    char path[SAVE_PATH_MAX];
    if (!save_autosave_path(path, sizeof path)) return false;
    return unlink(path) == 0 || errno == ENOENT;
}

bool save_delete_slot(unsigned slot) {
    char path[SAVE_PATH_MAX];
    if (!save_slot_path(slot, path, sizeof path)) return false;
    return unlink(path) == 0 || errno == ENOENT;
}
