#ifndef CARDS_PLAY_RENDER_H
#define CARDS_PLAY_RENDER_H
#include "render.h"
#include "keyboard.h"
#include "play_view.h"
#include "fx.h"
void play_render(SDL_Renderer *renderer, const KlondikeState *state, const PlayUI *ui,
                 const Layout *layout, const KeyboardMap *keys, BackTheme theme, uint64_t now);
void play_render_fx(SDL_Renderer *renderer, const KlondikeState *state, const PlayUI *ui,
                    const Layout *layout, const KeyboardMap *keys, BackTheme theme,
                    uint64_t now, PlayFX *fx);
#endif
