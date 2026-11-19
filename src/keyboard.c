#include "keyboard.h"
#include <string.h>
static const char *names[INPUT_COUNT] = {
    "left","right","up","down","a","b","x","y","l1","r1","l2","r2","start","select","quit"
};
KeyboardMap keyboard_default(void) {
    return (KeyboardMap){{SDL_SCANCODE_LEFT,SDL_SCANCODE_RIGHT,SDL_SCANCODE_UP,SDL_SCANCODE_DOWN,
        SDL_SCANCODE_D,SDL_SCANCODE_S,SDL_SCANCODE_W,SDL_SCANCODE_A,
        SDL_SCANCODE_LSHIFT,SDL_SCANCODE_SPACE,SDL_SCANCODE_LCTRL,SDL_SCANCODE_LALT,
        SDL_SCANCODE_E,SDL_SCANCODE_Q,SDL_SCANCODE_ESCAPE}};
}
const char *keyboard_action_name(InputAction action) {
    return action < INPUT_COUNT ? names[action] : "none";
}
bool keyboard_bind(KeyboardMap *map, const char *assignment) {
    const char *equal = strchr(assignment,'=');
    if (!equal || !equal[1]) return false;
    for (int i = 0; i < INPUT_COUNT; ++i) {
        size_t n = strlen(names[i]);
        if ((size_t)(equal - assignment) == n && !strncmp(assignment,names[i],n)) {
            SDL_Scancode code = SDL_GetScancodeFromName(equal + 1);
            if (code == SDL_SCANCODE_UNKNOWN) return false;
            map->keys[i] = code;
            return true;
        }
    }
    return false;
}
bool keyboard_valid(const KeyboardMap *map) {
    for (int i = 0; i < INPUT_COUNT; ++i) {
        if (map->keys[i] == SDL_SCANCODE_UNKNOWN) return false;
        for (int j = 0; j < i; ++j) if (map->keys[j] == map->keys[i]) return false;
    }
    return true;
}
InputAction keyboard_action(const KeyboardMap *map, const SDL_Event *event) {
    if (event->type != SDL_KEYDOWN || event->key.repeat) return INPUT_NONE;
    for (int i = 0; i < INPUT_COUNT; ++i)
        if (map->keys[i] == event->key.keysym.scancode) return (InputAction)i;
    return INPUT_NONE;
}
