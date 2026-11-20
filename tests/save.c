#define _POSIX_C_SOURCE 200809L
#include "save.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures;

static void expect(int cond, const char *msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        ++failures;
    }
}

static void setup_tmp_home(char *dir, size_t n) {
    snprintf(dir, n, "/tmp/croupier-rx-save-test-%d", (int)getpid());
    setenv("XDG_DATA_HOME", dir, 1);
}

static void cleanup_tree(const char *dir) {
    char cmd[512];
    snprintf(cmd, sizeof cmd, "rm -rf '%s'", dir);
    (void)system(cmd);
}

static void play_a_few(KlondikeGame *g) {
    klondike_new_game(g, 42);
    (void)klondike_draw(g);
    (void)klondike_draw(g);
    for (unsigned t = 0; t < 7; ++t) {
        const Pile *p = &g->state.tableau[t];
        if (!p->count) continue;
        size_t start = p->count - 1;
        for (unsigned f = 0; f < 4; ++f)
            if (klondike_move(g, (KlondikePile){K_TABLEAU, t}, start, (KlondikePile){K_FOUNDATION, f}))
                break;
    }
}

int main(void) {
    char home[256];
    setup_tmp_home(home, sizeof home);

    KlondikeGame game = {0};
    klondike_set_undo_limit(&game, 32);
    play_a_few(&game);
    expect(klondike_valid(&game.state), "fixture valid");
    expect(game.undo_depth > 0, "fixture has undo");

    SaveMeta meta = {
        .seed = 42,
        .score = 120,
        .move_count = 7,
        .elapsed_ms = 12345,
        .undo_limit = 32,
        .mode = SAVE_MODE_KLONDIKE_DRAW1,
        .saved_at = 1700000000,
        .has_game = true
    };
    SaveBlob blob;
    expect(save_capture(&blob, &game, &meta), "capture");
    expect(blob.undo_count == (uint16_t)(game.undo_depth > SAVE_MAX_UNDO ? SAVE_MAX_UNDO : game.undo_depth),
           "undo count captured");
    expect(blob.state.stock.count == game.state.stock.count, "stock count");
    expect(blob.state.waste.count == game.state.waste.count, "waste count");

    char path[SAVE_PATH_MAX];
    expect(save_autosave_path(path, sizeof path), "autosave path");
    expect(save_write_file(path, &blob) == SAVE_OK, "write autosave file");

    SaveBlob loaded = {0};
    expect(save_read_file(path, &loaded) == SAVE_OK, "read autosave file");
    expect(loaded.meta.seed == 42, "seed roundtrip");
    expect(loaded.meta.score == 120, "score roundtrip");
    expect(loaded.meta.move_count == 7, "moves roundtrip");
    expect(loaded.meta.elapsed_ms == 12345, "time roundtrip");
    expect(loaded.state.stock.count == blob.state.stock.count, "stock roundtrip");
    expect(loaded.state.waste.count == blob.state.waste.count, "waste roundtrip");
    expect(loaded.undo_count == blob.undo_count, "undo count roundtrip");

    KlondikeGame restored = {0};
    klondike_set_undo_limit(&restored, 99);
    SaveMeta out_meta;
    expect(save_restore(&loaded, &restored, &out_meta), "restore");
    expect(restored.state.stock.count == game.state.stock.count, "restored stock");
    expect(restored.state.waste.count == game.state.waste.count, "restored waste");
    expect(restored.undo_depth == game.undo_depth || restored.undo_depth == SAVE_MAX_UNDO, "restored undo depth");
    expect(out_meta.move_count == 7, "restored meta moves");
    expect(memcmp(&restored.state, &game.state, sizeof(KlondikeState)) == 0, "state bytes match");

    /* Corrupt checksum */
    FILE *f = fopen(path, "r+b");
    expect(f != NULL, "open for corrupt");
    if (f) {
        fseek(f, -1, SEEK_END);
        int c = fgetc(f);
        fseek(f, -1, SEEK_END);
        fputc(c ^ 0x5A, f);
        fclose(f);
    }
    SaveBlob bad = {0};
    expect(save_read_file(path, &bad) == SAVE_ERR_CHECKSUM, "checksum detects corruption");

    /* Truncated */
    f = fopen(path, "wb");
    if (f) { fwrite("CRX1", 1, 4, f); fclose(f); }
    expect(save_read_file(path, &bad) == SAVE_ERR_FORMAT ||
           save_read_file(path, &bad) == SAVE_ERR_CHECKSUM ||
           save_read_file(path, &bad) == SAVE_ERR_VERSION, "truncated rejected");

    /* Manual slots independent of autosave */
    expect(save_write_slot(0, &blob) == SAVE_OK, "slot0 write");
    expect(save_write_slot(9, &blob) == SAVE_OK, "slot9 write");
    SaveMeta probe;
    expect(save_probe_slot(0, &probe) == SAVE_OK, "probe slot0");
    expect(probe.move_count == 7, "probe moves");
    expect(save_probe_slot(5, &probe) == SAVE_ERR_EMPTY, "empty slot5");

    SaveBlob slot_blob;
    expect(save_read_slot(9, &slot_blob) == SAVE_OK, "read slot9");
    expect(save_delete_slot(0), "delete slot0");
    expect(save_probe_slot(0, &probe) == SAVE_ERR_EMPTY, "slot0 empty after delete");
    expect(save_probe_slot(9, &probe) == SAVE_OK, "slot9 still present");

    /* Autosave API */
    expect(save_write_autosave(&blob) == SAVE_OK, "write autosave api");
    expect(save_read_autosave(&loaded) == SAVE_OK, "read autosave api");

    klondike_dispose(&game);
    klondike_dispose(&restored);
    cleanup_tree(home);

    if (failures) {
        fprintf(stderr, "%d save test failure(s)\n", failures);
        return 1;
    }
    puts("save tests ok");
    return 0;
}
