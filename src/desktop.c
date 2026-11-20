#include "desktop.h"
#include "rx_input.h"
#include "sdl_compat.h"
PlayEvent desktop_event(PlayUI *ui, KlondikeGame *game, const KeyboardMap *map,
                        const SDL_Event *event, uint64_t now) {
    return desktop_event_rx(ui, game, map, NULL, event, now);
}
PlayEvent desktop_event_rx(PlayUI *ui, KlondikeGame *game, const KeyboardMap *map,
                           RxInput *rx, const SDL_Event *event, uint64_t now) {
    if (event->type == SDL_QUIT) {
        ui->return_screen = ui->screen;
        ui->exit_app = true;
        /* Request autosave then quit without an extra dialog on window close. */
        return PLAY_SAVE_NEEDED;
    }
    InputAction action = keyboard_action(map, event);
    if (action == INPUT_NONE && rx) action = rx_input_event(rx, event);
    else if (rx) (void)rx_input_event(rx, event);
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    bool l2_held = state[map->keys[INPUT_L2]] || rx_input_held(rx, INPUT_L2);
    if (action == INPUT_SELECT && rx_input_held(rx, INPUT_L2)) l2_held = true;
    /* Chord uses the other key already held; exclude the key that just pressed. */
    bool select_held = false, start_held = false;
    if (action != INPUT_SELECT)
        select_held = state[map->keys[INPUT_SELECT]] || rx_input_held(rx, INPUT_SELECT);
    if (action != INPUT_START)
        start_held = state[map->keys[INPUT_START]] || rx_input_held(rx, INPUT_START);
    return play_dispatch_chord(ui, game, action, l2_held, select_held, start_held, now);
}
