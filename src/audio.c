#include "audio.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Match go-rx: SDL_QueueAudio, 48k mono S16, no callback (stable on StockOS ALSA). */
enum { SAMPLE_RATE = 48000, MAX_EFFECT_MS = 900 };

static struct {
    bool ready, enabled;
    SDL_AudioDeviceID device;
} audio;

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void synth(SfxId id, Sint16 *out, int count, int rate) {
    Uint32 noise = 1103515245u + (Uint32)id * 97u;
    for (int i = 0; i < count; ++i) {
        float t = (float)i / (float)rate;
        float sample = 0.0f;
        noise = noise * 1664525u + 1013904223u;
        float grit = (float)(noise >> 16) / 32768.0f - 1.0f;
        switch (id) {
        case SFX_PLACE:
        case SFX_DECK_TAP: {
            float contact = grit * expf(-520.0f * t) * 4200.0f;
            float wood = (sinf(3200.0f * t) * 1800.0f + sinf(6100.0f * t) * 900.0f) * expf(-280.0f * t);
            sample = contact + wood;
            break;
        }
        case SFX_DRAW:
            sample = grit * expf(-180.0f * t) * 2800.0f + sinf(900.0f * t) * expf(-40.0f * t) * 900.0f;
            break;
        case SFX_INVALID:
            sample = (sinf(6.2831853f * 180.0f * t) + 0.4f * sinf(6.2831853f * 190.0f * t)) *
                     expf(-8.0f * t) * 5000.0f;
            break;
        case SFX_MENU: {
            float phase = 6.2831853f * 660.0f * t;
            sample = (sinf(phase) + 0.2f * sinf(phase * 2.0f)) * expf(-14.0f * t) * 3800.0f;
            break;
        }
        case SFX_WIN: {
            float age = t < 0.12f ? t : (t < 0.24f ? t - 0.12f : t - 0.24f);
            float pitch = t < 0.12f ? 523.25f : (t < 0.24f ? 659.25f : 784.0f);
            float phase = 6.2831853f * pitch * age;
            sample = (sinf(phase) + 0.22f * sinf(phase * 2.0f)) *
                     clampf(age * 170.0f, 0.0f, 1.0f) * expf(-11.0f * age) * 4800.0f;
            break;
        }
        case SFX_SHUFFLE:
        default: {
            float local = fmodf(t, 0.07f);
            int burst = (int)(t / 0.07f);
            if (burst < 12)
                sample = grit * expf(-90.0f * local) * (2200.0f + 200.0f * (burst % 3));
            else {
                float age = t - 0.84f;
                if (age < 0) age = 0;
                sample = (grit * 900.0f + sinf(900.0f * age) * 1400.0f) * expf(-25.0f * age);
            }
            break;
        }
        }
        float fade = 1.0f - (float)i / (float)count;
        int v = (int)(sample * fade);
        if (v > 12000) v = 12000;
        if (v < -12000) v = -12000;
        out[i] = (Sint16)v;
    }
}

bool audio_open(const char *sfx_dir) {
    (void)sfx_dir;
    audio_close();
    audio.enabled = true;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "audio unavailable: %s\n", SDL_GetError());
        return false;
    }
    SDL_AudioSpec format;
    SDL_zero(format);
    format.freq = SAMPLE_RATE;
    format.format = AUDIO_S16SYS;
    format.channels = 1;
    format.samples = 256;
    audio.device = SDL_OpenAudioDevice(NULL, 0, &format, NULL, 0);
    if (!audio.device) {
        fprintf(stderr, "audio open failed: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    audio.ready = true;
    SDL_PauseAudioDevice(audio.device, 0);
    fprintf(stderr, "audio: queue device ok @ %d Hz\n", SAMPLE_RATE);
    return true;
}

void audio_close(void) {
    if (audio.device) {
        SDL_CloseAudioDevice(audio.device);
        audio.device = 0;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    audio.ready = false;
}

void audio_set_enabled(bool enabled) { audio.enabled = enabled; }
bool audio_enabled(void) { return audio.enabled && audio.ready && audio.device != 0; }

void audio_play(SfxId id) {
    if (!audio_enabled() || id >= SFX_COUNT) return;
    int ms = (id == SFX_SHUFFLE) ? 900 : (id == SFX_WIN ? 500 : 220);
    int count = SAMPLE_RATE * ms / 1000;
    if (count < 64) count = 64;
    if (count > SAMPLE_RATE * MAX_EFFECT_MS / 1000)
        count = SAMPLE_RATE * MAX_EFFECT_MS / 1000;
    Sint16 *samples = (Sint16 *)SDL_malloc((size_t)count * sizeof(Sint16));
    if (!samples) return;
    synth(id, samples, count, SAMPLE_RATE);
    SDL_ClearQueuedAudio(audio.device);
    SDL_QueueAudio(audio.device, samples, (Uint32)count * sizeof(Sint16));
    SDL_PauseAudioDevice(audio.device, 0);
    SDL_free(samples);
}
