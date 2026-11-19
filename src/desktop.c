#include "desktop.h"
PlayEvent desktop_event(PlayUI *ui, KlondikeGame *game, const KeyboardMap *map,
                        const SDL_Event *event, uint64_t now) {
    if (event->type == SDL_QUIT) { ui->quit = true; return PLAY_QUIT; }
    return play_dispatch(ui,game,keyboard_action(map,event),now);
}
