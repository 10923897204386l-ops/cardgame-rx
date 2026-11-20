#ifndef CARDS_SDL_COMPAT_H
#define CARDS_SDL_COMPAT_H
#include <SDL.h>
/* StockOS ships SDL 2.0.12; SDL_GetTicks64 arrived in 2.0.18. */
#if !SDL_VERSION_ATLEAST(2, 0, 18)
static inline Uint64 cards_sdl_get_ticks64(void) {
    return (Uint64)SDL_GetTicks();
}
#define SDL_GetTicks64() cards_sdl_get_ticks64()
#endif
#endif
