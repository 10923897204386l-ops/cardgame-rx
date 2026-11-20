#ifndef CARDS_FX_H
#define CARDS_FX_H
#include "play.h"
#include "layout.h"
#include "render.h"
#include <SDL.h>
#include <stdbool.h>
enum { FX_MAX_MOTIONS = 24 };
typedef enum { FX_SLIDE = 0, FX_FLIP, FX_SETTLE, FX_DEAL } FxKind;
typedef struct {
    bool active;
    FxKind kind;
    Rect from, to;
    CardState card;
    uint64_t start_ms;
    uint32_t duration_ms;
} FxMotion;
typedef struct PlayFX {
    bool enabled;
    FxMotion motions[FX_MAX_MOTIONS];
    unsigned count;
    uint64_t menu_fade_start;
    PlayScreen menu_from, menu_to;
    uint64_t victory_start;
    bool victory;
    Rect trail[6];
    unsigned trail_count;
    uint64_t trail_stamp;
} PlayFX;
void fx_init(PlayFX *fx, bool enabled);
void fx_set_enabled(PlayFX *fx, bool enabled);
bool fx_enabled(const PlayFX *fx);
void fx_clear(PlayFX *fx);
void fx_push_motion(PlayFX *fx, FxKind kind, Rect from, Rect to, CardState card, uint64_t now, uint32_t ms);
void fx_note_held(PlayFX *fx, Rect bounds, uint64_t now);
void fx_menu_change(PlayFX *fx, PlayScreen from, PlayScreen to, uint64_t now);
void fx_set_victory(PlayFX *fx, bool won, uint64_t now);
void fx_update(PlayFX *fx, uint64_t now);
bool fx_busy(const PlayFX *fx, uint64_t now);
float fx_menu_fade(const PlayFX *fx, uint64_t now);
void fx_render_under(SDL_Renderer *r, const PlayFX *fx, BackTheme theme, uint64_t now);
void fx_render_over(SDL_Renderer *r, const PlayFX *fx, const PlayUI *ui, const Layout *layout,
                    const KlondikeState *state, BackTheme theme, uint64_t now);
/* Warm-gold selection glow helper used by play_render. */
void fx_draw_selection_glow(SDL_Renderer *r, Rect bounds, bool legal, uint64_t now);
#endif
