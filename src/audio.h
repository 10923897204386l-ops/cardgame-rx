#ifndef CARDS_AUDIO_H
#define CARDS_AUDIO_H
#include <SDL.h>
#include <stdbool.h>
typedef enum {
    SFX_SHUFFLE = 0,
    SFX_DECK_TAP,
    SFX_PLACE,
    SFX_DRAW,
    SFX_INVALID,
    SFX_WIN,
    SFX_MENU,
    SFX_COUNT
} SfxId;
bool audio_open(const char *sfx_dir);
void audio_close(void);
void audio_set_enabled(bool enabled);
bool audio_enabled(void);
void audio_play(SfxId id);
#endif
