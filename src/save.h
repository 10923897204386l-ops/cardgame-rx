#ifndef CARDS_SAVE_H
#define CARDS_SAVE_H
#include "klondike.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

enum {
    SAVE_MAGIC = 0x31585243u, /* 'CRX1' little-endian */
    SAVE_VERSION = 1,
    SAVE_MANUAL_SLOTS = 10,
    SAVE_MAX_UNDO = 64,
    SAVE_PATH_MAX = 512
};

typedef enum {
    SAVE_MODE_KLONDIKE_DRAW1 = 0
} SaveGameMode;

typedef struct {
    uint32_t seed;
    uint32_t score;
    uint32_t move_count;
    uint32_t elapsed_ms;
    uint32_t undo_limit;
    SaveGameMode mode;
    time_t saved_at;
    bool has_game;
    bool won;
} SaveMeta;

typedef struct {
    SaveMeta meta;
    KlondikeState state;
    KlondikeState undo[SAVE_MAX_UNDO];
    uint16_t undo_count; /* oldest at 0, newest at undo_count-1 */
} SaveBlob;

typedef enum {
    SAVE_OK = 0,
    SAVE_ERR_IO,
    SAVE_ERR_FORMAT,
    SAVE_ERR_VERSION,
    SAVE_ERR_CHECKSUM,
    SAVE_ERR_BOUNDS,
    SAVE_ERR_STATE,
    SAVE_ERR_NO_DIR,
    SAVE_ERR_EMPTY
} SaveStatus;

/* Resolve writable application-data directory (XDG on Linux). Creates it. */
bool save_data_dir(char *out, size_t out_size);

/* Path helpers: slot 0..9 manual; autosave is separate. */
bool save_autosave_path(char *out, size_t out_size);
bool save_slot_path(unsigned slot, char *out, size_t out_size);

const char *save_status_text(SaveStatus status);

/* Capture current game into a blob. undo_count is capped at SAVE_MAX_UNDO newest. */
bool save_capture(SaveBlob *blob, const KlondikeGame *game, const SaveMeta *meta);

/* Restore blob into game (replaces state and rebuilds undo history). */
bool save_restore(const SaveBlob *blob, KlondikeGame *game, SaveMeta *meta_out);

SaveStatus save_write_file(const char *path, const SaveBlob *blob);
SaveStatus save_read_file(const char *path, SaveBlob *blob);

SaveStatus save_write_autosave(const SaveBlob *blob);
SaveStatus save_read_autosave(SaveBlob *blob);
SaveStatus save_write_slot(unsigned slot, const SaveBlob *blob);
SaveStatus save_read_slot(unsigned slot, SaveBlob *blob);

/* Probe slot/autosave metadata without full restore. Empty file => SAVE_ERR_EMPTY. */
SaveStatus save_probe_autosave(SaveMeta *meta);
SaveStatus save_probe_slot(unsigned slot, SaveMeta *meta);

bool save_delete_autosave(void);
bool save_delete_slot(unsigned slot);

#endif
