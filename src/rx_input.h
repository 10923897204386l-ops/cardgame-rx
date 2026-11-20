#ifndef CARDS_RX_INPUT_H
#define CARDS_RX_INPUT_H
#include <SDL.h>
#include <string.h>
#include "controls.h"

/* Opens the StockOS ANBERNIC-keys joystick used by go-rx / RG35XXSP. */
static inline SDL_Joystick *rx_open_controller(void) {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        const char *name = SDL_JoystickNameForIndex(i);
        if (name && !strcmp(name, "ANBERNIC-keys")) return SDL_JoystickOpen(i);
    }
    return SDL_NumJoysticks() > 0 ? SDL_JoystickOpen(0) : NULL;
}

/*
 * Physical ANBERNIC-keys mapping verified in go-rx (2026-11-19):
 * buttons 0..10 = A B Y X L1 R1 SELECT START M L2 R2
 * hat 0 = D-pad.
 */
static inline InputAction rx_button_action(Uint8 button) {
    static const InputAction buttons[] = {
        INPUT_A, INPUT_B, INPUT_Y, INPUT_X, INPUT_L1, INPUT_R1,
        INPUT_SELECT, INPUT_START, INPUT_QUIT, INPUT_L2, INPUT_R2
    };
    return button < (Uint8)(sizeof buttons / sizeof buttons[0]) ? buttons[button] : INPUT_NONE;
}

typedef struct RxInput {
    SDL_Joystick *pad;
    SDL_JoystickID id;
    bool down[INPUT_COUNT];
} RxInput;

static inline void rx_input_init(RxInput *rx) {
    memset(rx, 0, sizeof *rx);
    rx->pad = rx_open_controller();
    rx->id = rx->pad ? SDL_JoystickInstanceID(rx->pad) : -1;
}

static inline void rx_input_close(RxInput *rx) {
    if (rx && rx->pad) SDL_JoystickClose(rx->pad);
    if (rx) memset(rx, 0, sizeof *rx);
}

/* Translate one SDL event. Returns INPUT_NONE unless this is a fresh press. */
static inline InputAction rx_input_event(RxInput *rx, const SDL_Event *event) {
    if (!rx || !event) return INPUT_NONE;
    if (event->type == SDL_JOYDEVICEADDED && !rx->pad) {
        rx_input_init(rx);
        return INPUT_NONE;
    }
    if (event->type == SDL_JOYDEVICEREMOVED && rx->pad &&
        event->jdevice.which == rx->id) {
        rx_input_close(rx);
        return INPUT_NONE;
    }
    if (event->type == SDL_JOYHATMOTION && rx->pad &&
        event->jhat.which == rx->id && event->jhat.hat == 0) {
        Uint8 hat = event->jhat.value;
        struct { InputAction action; int on; } dirs[] = {
            {INPUT_UP, hat & SDL_HAT_UP},
            {INPUT_DOWN, hat & SDL_HAT_DOWN},
            {INPUT_LEFT, hat & SDL_HAT_LEFT},
            {INPUT_RIGHT, hat & SDL_HAT_RIGHT},
        };
        InputAction pressed = INPUT_NONE;
        for (unsigned i = 0; i < 4; ++i) {
            bool was = rx->down[dirs[i].action];
            bool now = dirs[i].on != 0;
            rx->down[dirs[i].action] = now;
            if (now && !was) pressed = dirs[i].action;
        }
        return pressed;
    }
    if ((event->type == SDL_JOYBUTTONDOWN || event->type == SDL_JOYBUTTONUP) &&
        rx->pad && event->jbutton.which == rx->id) {
        InputAction action = rx_button_action(event->jbutton.button);
        if (action == INPUT_NONE) return INPUT_NONE;
        bool now = event->type == SDL_JOYBUTTONDOWN;
        bool was = rx->down[action];
        rx->down[action] = now;
        return now && !was ? action : INPUT_NONE;
    }
    return INPUT_NONE;
}

static inline bool rx_input_held(const RxInput *rx, InputAction action) {
    return rx && action < INPUT_COUNT && rx->down[action];
}
#endif
