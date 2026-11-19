#ifndef CARDS_KEYBOARD_H
#define CARDS_KEYBOARD_H
#include <SDL.h>
#include <stdbool.h>
#include "controls.h"
typedef struct { SDL_Scancode keys[INPUT_COUNT]; } KeyboardMap;
KeyboardMap keyboard_default(void);
const char *keyboard_action_name(InputAction action);
/* Configure with logical action and SDL physical key names, then validate all overrides.
   Duplicate assignments are rejected by keyboard_valid, allowing key swaps. */
bool keyboard_bind(KeyboardMap *map, const char *assignment);
bool keyboard_valid(const KeyboardMap *map);
InputAction keyboard_action(const KeyboardMap *map, const SDL_Event *event);
#endif
