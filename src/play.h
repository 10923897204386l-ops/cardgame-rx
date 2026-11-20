#ifndef CARDS_PLAY_H
#define CARDS_PLAY_H
#include "controls.h"
#include "klondike.h"

typedef enum { AREA_DRAW, AREA_TABLEAU, AREA_FOUNDATIONS, AREA_COUNT } PlayArea;
typedef enum { PLAY_NONE, PLAY_ERROR, PLAY_COMMITTED, PLAY_CANCELLED, PLAY_NEW_GAME, PLAY_QUIT } PlayEvent;
typedef enum { MENU_MAIN, MENU_PLAYING, MENU_PAUSE, MENU_CONFIRM_NEW, MENU_CONFIRM_MAIN, MENU_OPTIONS } PlayScreen;
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
    uint64_t invalid_until, message_until;
    PlayMessage message;
} PlayUI;
void play_init(PlayUI *ui, const KlondikeState *state, uint32_t seed);
KlondikePile play_focus(const PlayUI *ui);
size_t play_first_face_up(const Pile *pile);
size_t play_selected(const PlayUI *ui, const KlondikeState *state);
size_t play_held_count(const PlayUI *ui, const KlondikeState *state);
bool play_preview(const PlayUI *ui, const KlondikeState *state, size_t *chosen);
bool play_invalid_outline(const PlayUI *ui, uint64_t now);
const char *play_message(const PlayUI *ui, uint64_t now);
/* Time is injected (SDL_GetTicks64 at the boundary), so effects never sleep.
   PLAY_ERROR reserves a semantic event for future sound; no playback here. */
PlayEvent play_dispatch(PlayUI *ui, KlondikeGame *game, InputAction action, uint64_t now);
PlayEvent play_dispatch_chord(PlayUI *ui, KlondikeGame *game, InputAction action, bool l2_held, uint64_t now);
#endif
