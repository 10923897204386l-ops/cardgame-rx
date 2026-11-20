#ifndef CARDS_PLAY_H
#define CARDS_PLAY_H
#include "controls.h"
#include "klondike.h"
#include "save.h"

typedef enum { AREA_DRAW, AREA_TABLEAU, AREA_FOUNDATIONS, AREA_COUNT } PlayArea;
typedef enum {
    PLAY_NONE, PLAY_ERROR, PLAY_COMMITTED, PLAY_CANCELLED, PLAY_NEW_GAME, PLAY_QUIT,
    PLAY_AUTOSAVED, PLAY_LOADED, PLAY_SAVE_NEEDED
} PlayEvent;
typedef enum {
    MENU_MAIN, MENU_PLAYING, MENU_PAUSE, MENU_CONFIRM_NEW, MENU_CONFIRM_MAIN, MENU_OPTIONS,
    MENU_SAVE_SLOTS, MENU_LOAD_SLOTS, MENU_CONFIRM_OVERWRITE, MENU_CONFIRM_LOAD,
    MENU_CONFIRM_QUIT, MENU_SAVE_ERROR, MENU_QUIT_SAVE_ERROR
} PlayScreen;
typedef enum {
    MSG_READY, MSG_PICKED, MSG_CANCELLED, MSG_INVALID_DROP, MSG_EMPTY,
    MSG_HOLDING, MSG_NO_MOVE, MSG_MOVED, MSG_DRAW, MSG_RECYCLE,
    MSG_UNDONE, MSG_NO_UNDO, MSG_STOCK, MSG_COUNT
} PlayMessage;
enum { PLAY_INVALID_MS = 300, PLAY_MESSAGE_MS = 1800 };
/* Transient state only. Never store this in the rules undo history. */
typedef struct {
    PlayArea area;
    unsigned focus[AREA_COUNT];
    size_t selected;
    bool held;
    bool controls_visible;
    KlondikePile source;
    size_t source_start;
    PlayScreen screen, return_screen;
    bool paused, confirming, confirm_yes, quit;
    unsigned menu;
    uint32_t seed;
    uint32_t score;
    uint32_t move_count;
    uint32_t elapsed_ms;
    uint64_t run_started_ms; /* wall tick when unpaused segment began; 0 if paused */
    uint64_t invalid_until, message_until;
    PlayMessage message;
    /* Save/load UI */
    unsigned slot;           /* 0..9 selected manual slot */
    bool slot_occupied;      /* selected slot has data (for overwrite confirm) */
    bool continue_available; /* autosave present at main menu */
    int save_error;          /* SaveStatus code for error dialogs */
    unsigned quit_choice;    /* 0 retry, 1 cancel, 2 quit without saving */
    bool exit_app;           /* true = quit application (vs return to menu) */
    bool dirty;              /* game progressed since last successful save */
} PlayUI;
void play_init(PlayUI *ui, const KlondikeState *state, uint32_t seed);
void play_set_continue_available(PlayUI *ui, bool available);
void play_tick(PlayUI *ui, uint64_t now); /* accumulate elapsed while playing */
void play_fill_meta(const PlayUI *ui, SaveMeta *meta);
KlondikePile play_focus(const PlayUI *ui);
size_t play_first_face_up(const Pile *pile);
size_t play_selected(const PlayUI *ui, const KlondikeState *state);
size_t play_held_count(const PlayUI *ui, const KlondikeState *state);
bool play_preview(const PlayUI *ui, const KlondikeState *state, size_t *chosen);
bool play_invalid_outline(const PlayUI *ui, uint64_t now);
const char *play_message(const PlayUI *ui, uint64_t now);
/* Time is injected (SDL_GetTicks64 at the boundary), so effects never sleep.
   PLAY_ERROR reserves a semantic event for future sound; no playback here.
   Chord flags: l2_held for undo; select_held/start_held for Select+Start quit. */
PlayEvent play_dispatch(PlayUI *ui, KlondikeGame *game, InputAction action, uint64_t now);
PlayEvent play_dispatch_chord(PlayUI *ui, KlondikeGame *game, InputAction action,
                              bool l2_held, bool select_held, bool start_held, uint64_t now);
/* Apply a loaded save into UI session counters and enter play. */
void play_apply_loaded(PlayUI *ui, const SaveMeta *meta, uint64_t now);
#endif
