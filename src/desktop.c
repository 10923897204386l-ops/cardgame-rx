#include "desktop.h"
#include "rx_input.h"
#include "sdl_compat.h"
PlayEvent desktop_event(PlayUI *ui, KlondikeGame *game, const KeyboardMap *map,
                        const SDL_Event *event, uint64_t now) {
    return desktop_event_rx(ui, game, map, NULL, event, now);
}
PlayEvent desktop_event_rx(PlayUI *ui, KlondikeGame *game, const KeyboardMap *map,
                           RxInput *rx, const SDL_Event *event, uint64_t now) {
    if (event->type == SDL_QUIT) { ui->quit = true; return PLAY_QUIT; }
    InputAction action = keyboard_action(map, event);
    if (action == INPUT_NONE && rx) action = rx_input_event(rx, event);
    else if (rx) (void)rx_input_event(rx, event); /* keep held state current */
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    bool l2_held = state[map->keys[INPUT_L2]] || rx_input_held(rx, INPUT_L2);
    if (event->type == SDL_KEYDOWN && !event->key.repeat && action == INPUT_SELECT)
        l2_held = l2_held || state[map->keys[INPUT_L2]];
    if (action == INPUT_SELECT && rx_input_held(rx, INPUT_L2)) l2_held = true;
    return play_dispatch_chord(ui, game, action, l2_held, now);
}
