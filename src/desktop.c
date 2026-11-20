#include "desktop.h"
PlayEvent desktop_event(PlayUI *ui, KlondikeGame *game, const KeyboardMap *map,
                        const SDL_Event *event, uint64_t now) {
    if (event->type == SDL_QUIT) { ui->quit = true; return PLAY_QUIT; }
    InputAction action = keyboard_action(map,event);
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    bool l2_held = state[map->keys[INPUT_L2]];
    if (event->type == SDL_KEYDOWN && !event->key.repeat && action == INPUT_SELECT)
        l2_held = l2_held || state[map->keys[INPUT_L2]];
    return play_dispatch_chord(ui,game,action,l2_held,now);
}
