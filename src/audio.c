#include "audio.h"
#include <stdio.h>
#include <string.h>

enum { AUDIO_VOICES = 8, AUDIO_MAX_SAMPLES = 48000 };

typedef struct {
    Sint16 *data;
    Uint32 frames;
} Sample;

typedef struct {
    const Sint16 *data;
    Uint32 frames, pos;
    bool active;
} Voice;

static struct {
    bool ready, enabled;
    SDL_AudioDeviceID device;
    SDL_AudioSpec spec;
    Sample samples[SFX_COUNT];
    Voice voices[AUDIO_VOICES];
    SDL_mutex *lock;
} audio;

static void SDLCALL audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    memset(stream, 0, (size_t)len);
    if (!audio.enabled || !audio.lock) return;
    int frames = len / (int)sizeof(Sint16);
    Sint16 *out = (Sint16 *)stream;
    SDL_LockMutex(audio.lock);
    for (int v = 0; v < AUDIO_VOICES; ++v) {
        Voice *voice = &audio.voices[v];
        if (!voice->active || !voice->data) continue;
        for (int i = 0; i < frames; ++i) {
            if (voice->pos >= voice->frames) { voice->active = false; break; }
            int mixed = out[i] + (voice->data[voice->pos++] / 2);
            if (mixed > 24000) mixed = 24000;
            if (mixed < -24000) mixed = -24000;
            out[i] = (Sint16)mixed;
        }
    }
    SDL_UnlockMutex(audio.lock);
}

static bool load_wav(Sample *sample, const char *path) {
    SDL_AudioSpec wav;
    Uint8 *buf = NULL;
    Uint32 len = 0;
    if (!SDL_LoadWAV(path, &wav, &buf, &len)) return false;
    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, wav.format, wav.channels, wav.freq,
                          audio.spec.format, audio.spec.channels, audio.spec.freq) < 0) {
        SDL_FreeWAV(buf); return false;
    }
    cvt.len = (int)len;
    cvt.buf = SDL_malloc((size_t)cvt.len * (size_t)cvt.len_mult);
    if (!cvt.buf) { SDL_FreeWAV(buf); return false; }
    memcpy(cvt.buf, buf, len);
    SDL_FreeWAV(buf);
    if (SDL_ConvertAudio(&cvt) != 0) { SDL_free(cvt.buf); return false; }
    sample->frames = (Uint32)(cvt.len_cvt / (int)sizeof(Sint16));
    sample->data = (Sint16 *)cvt.buf;
    /* Attenuate to avoid clipping/distortion on handheld speakers. */
    for (Uint32 i = 0; i < sample->frames; ++i) {
        int s = (sample->data[i] * 45) / 100;
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        sample->data[i] = (Sint16)s;
    }
    return sample->frames > 0 && sample->data;
}

bool audio_open(const char *sfx_dir) {
    audio_close();
    audio.enabled = true;
    audio.lock = SDL_CreateMutex();
    if (!audio.lock) return false;
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = 22050;
    want.format = AUDIO_S16LSB;
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_callback;
    audio.device = SDL_OpenAudioDevice(NULL, 0, &want, &audio.spec, 0);
    if (!audio.device) return false;
    static const char *names[SFX_COUNT] = {
        "shuffle_riffle.wav","deck_tap.wav","place.wav","draw.wav",
        "invalid.wav","win_chime.wav","menu.wav"
    };
    char path[512];
    bool any = false;
    for (int i = 0; i < SFX_COUNT; ++i) {
        snprintf(path, sizeof(path), "%s/%s", sfx_dir ? sfx_dir : "assets/runtime/sfx", names[i]);
        if (load_wav(&audio.samples[i], path)) any = true;
    }
    audio.ready = any;
    if (audio.ready) SDL_PauseAudioDevice(audio.device, 0);
    return audio.ready;
}

void audio_close(void) {
    if (audio.device) {
        SDL_CloseAudioDevice(audio.device);
        audio.device = 0;
    }
    for (int i = 0; i < SFX_COUNT; ++i) {
        SDL_free(audio.samples[i].data);
        audio.samples[i].data = NULL;
        audio.samples[i].frames = 0;
    }
    if (audio.lock) { SDL_DestroyMutex(audio.lock); audio.lock = NULL; }
    memset(audio.voices, 0, sizeof audio.voices);
    audio.ready = false;
}

void audio_set_enabled(bool enabled) { audio.enabled = enabled; }
bool audio_enabled(void) { return audio.enabled && audio.ready; }

void audio_play(SfxId id) {
    if (!audio.ready || !audio.enabled || id >= SFX_COUNT) return;
    Sample *sample = &audio.samples[id];
    if (!sample->data || !sample->frames) return;
    SDL_LockMutex(audio.lock);
    for (int i = 0; i < AUDIO_VOICES; ++i) {
        if (!audio.voices[i].active) {
            audio.voices[i] = (Voice){sample->data, sample->frames, 0, true};
            break;
        }
    }
    SDL_UnlockMutex(audio.lock);
}
