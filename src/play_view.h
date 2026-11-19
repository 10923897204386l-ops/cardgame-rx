#ifndef CARDS_PLAY_VIEW_H
#define CARDS_PLAY_VIEW_H
#include "layout.h"
#include "play.h"
/* Normal columns use the M0.1A layout. Overflow compresses the hidden prefix
   into at most 24 pixels and uses 22-pixel face-up strips. Only final pip art
   may clip, never a selectable index. UI can mark that clipped artwork. */
Rect play_card_rect(const Layout *layout, const Pile *pile, unsigned col, size_t index);
Rect play_focus_rect(const Layout *layout, const KlondikeState *state, const PlayUI *ui);
int play_held_overlap(const Layout *layout, size_t count);
Rect play_held_bounds(const Layout *layout, const KlondikeState *state, const PlayUI *ui);
#endif
