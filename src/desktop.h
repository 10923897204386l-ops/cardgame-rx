#ifndef CARDS_DESKTOP_H
#define CARDS_DESKTOP_H
#include "keyboard.h"
#include "play.h"
/* SDL event boundary; keyboard mapping has no dependency on game policies. */
PlayEvent desktop_event(PlayUI *ui, KlondikeGame *game, const KeyboardMap *map,
                        const SDL_Event *event, uint64_t now);
#endif
