#ifndef CARDS_PLAY_RENDER_H
#define CARDS_PLAY_RENDER_H
#include "render.h"
#include "play_view.h"
void play_render(SDL_Renderer *renderer, const KlondikeState *state, const PlayUI *ui,
                 const Layout *layout, BackTheme theme, uint64_t now);
#endif
