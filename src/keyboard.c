#include "keyboard.h"
#include <string.h>
static const char *names[INPUT_COUNT] = {
    "left","right","up","down","a","b","x","y","l1","r1","l2","r2","start","select"
};
KeyboardMap keyboard_default(void) {
    return (KeyboardMap){{SDLK_LEFT,SDLK_RIGHT,SDLK_UP,SDLK_DOWN,
        SDLK_x,SDLK_z,SDLK_a,SDLK_s,SDLK_q,SDLK_e,SDLK_1,SDLK_3,SDLK_RETURN,SDLK_BACKSPACE}};
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
            SDL_Keycode code = SDL_GetKeyFromName(equal + 1);
            if (code == SDLK_UNKNOWN) return false;
            map->keys[i] = code;
            return true;
        }
    }
    return false;
}
bool keyboard_valid(const KeyboardMap *map) {
    for (int i = 0; i < INPUT_COUNT; ++i) {
        if (map->keys[i] == SDLK_UNKNOWN) return false;
        for (int j = 0; j < i; ++j) if (map->keys[j] == map->keys[i]) return false;
    }
    return true;
}
InputAction keyboard_action(const KeyboardMap *map, const SDL_Event *event) {
    if (event->type != SDL_KEYDOWN || event->key.repeat) return INPUT_NONE;
    for (int i = 0; i < INPUT_COUNT; ++i)
        if (map->keys[i] == event->key.keysym.sym) return (InputAction)i;
    return INPUT_NONE;
}
