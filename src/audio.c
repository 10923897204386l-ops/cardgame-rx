#include "audio.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { SAMPLE_RATE = 48000, MAX_EFFECT_MS = 900, MAX_VOICES = 8, MUSIC_LOOP_FADE = 2048 };

typedef struct {
    Sint16 *samples;
    Uint32 count;
} PcmBuffer;

typedef struct {
    const Sint16 *samples;
    Uint32 count, position;
    float gain;
    bool active;
} Voice;

static struct {
    bool ready, enabled, music_enabled, menu_music_active;
    float music_volume, music_gain, music_target;
    SDL_AudioDeviceID device;
    PcmBuffer sfx[SFX_COUNT];
    PcmBuffer music;
    Uint32 music_pos;
    Voice voices[MAX_VOICES];
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

static void free_pcm(PcmBuffer *pcm) {
    if (pcm && pcm->samples) SDL_free(pcm->samples);
    if (pcm) *pcm = (PcmBuffer){0};
}

static bool convert_spec(SDL_AudioSpec *spec, Uint8 **data, Uint32 *length) {
    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, spec->format, spec->channels, spec->freq,
                          AUDIO_S16SYS, 1, SAMPLE_RATE) < 0)
        return false;
    if (!cvt.needed) {
        *spec = (SDL_AudioSpec){.freq = SAMPLE_RATE, .format = AUDIO_S16SYS, .channels = 1};
        return true;
    }
    cvt.len = (int)*length;
    cvt.buf = (Uint8 *)SDL_malloc((size_t)cvt.len * (size_t)cvt.len_mult);
    if (!cvt.buf) return false;
    memcpy(cvt.buf, *data, *length);
    if (SDL_ConvertAudio(&cvt) != 0) {
        SDL_free(cvt.buf);
        return false;
    }
    SDL_FreeWAV(*data);
    *data = cvt.buf;
    *length = (Uint32)cvt.len_cvt;
    *spec = (SDL_AudioSpec){.freq = SAMPLE_RATE, .format = AUDIO_S16SYS, .channels = 1};
    return true;
}

static bool load_wav_pcm(const char *path, PcmBuffer *out) {
    SDL_AudioSpec spec;
    Uint8 *data = NULL;
    Uint32 length = 0;
    if (!SDL_LoadWAV(path, &spec, &data, &length)) return false;
    if (!convert_spec(&spec, &data, &length)) {
        SDL_FreeWAV(data);
        return false;
    }
    out->samples = (Sint16 *)data;
    out->count = length / sizeof(Sint16);
    return out->count != 0;
}

static void synth_pcm(SfxId id, PcmBuffer *out) {
    int ms = (id == SFX_SHUFFLE) ? 900 : (id == SFX_WIN ? 500 : 220);
    int count = SAMPLE_RATE * ms / 1000;
    if (count < 64) count = 64;
    if (count > SAMPLE_RATE * MAX_EFFECT_MS / 1000)
        count = SAMPLE_RATE * MAX_EFFECT_MS / 1000;
    out->samples = (Sint16 *)SDL_malloc((size_t)count * sizeof(Sint16));
    if (!out->samples) {
        *out = (PcmBuffer){0};
        return;
    }
    out->count = (Uint32)count;
    synth(id, out->samples, count, SAMPLE_RATE);
}

static float loop_sample(Uint32 *position) {
    Uint32 pos = *position;
    Uint32 count = audio.music.count;
    if (!count) return 0.0f;
    float value = (float)audio.music.samples[pos];
    if (pos >= count - MUSIC_LOOP_FADE) {
        Uint32 fade_pos = pos - (count - MUSIC_LOOP_FADE);
        float t = (float)fade_pos / (float)MUSIC_LOOP_FADE;
        value = value * (1.0f - t) + (float)audio.music.samples[fade_pos % count] * t;
    }
    pos = (pos + 1) % count;
    *position = pos;
    return value;
}

static void audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    Sint16 *out = (Sint16 *)stream;
    int sample_count = len / (int)sizeof(Sint16);
    memset(stream, 0, (size_t)len);
    for (int i = 0; i < sample_count; ++i) {
        float mixed = 0.0f;
        if (audio.enabled && audio.ready && audio.music_enabled && audio.music.samples) {
            float delta = audio.music_target - audio.music_gain;
            if (fabsf(delta) < 0.0008f) audio.music_gain = audio.music_target;
            else audio.music_gain += delta * 0.0035f;
            mixed += loop_sample(&audio.music_pos) * audio.music_gain;
        }
        for (int v = 0; v < MAX_VOICES; ++v) {
            Voice *voice = &audio.voices[v];
            if (!voice->active || !voice->samples || voice->position >= voice->count) {
                voice->active = false;
                continue;
            }
            mixed += (float)voice->samples[voice->position++] * voice->gain;
            if (voice->position >= voice->count) voice->active = false;
        }
        if (mixed > 32767.0f) mixed = 32767.0f;
        if (mixed < -32768.0f) mixed = -32768.0f;
        out[i] = (Sint16)mixed;
    }
}

bool audio_open(const char *asset_dir) {
    audio_close();
    audio.enabled = true;
    audio.music_enabled = true;
    audio.music_volume = 0.42f;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "audio unavailable: %s\n", SDL_GetError());
        return false;
    }
    SDL_AudioSpec format;
    SDL_zero(format);
    format.freq = SAMPLE_RATE;
    format.format = AUDIO_S16SYS;
    format.channels = 1;
    format.samples = 1024;
    format.callback = audio_callback;
    audio.device = SDL_OpenAudioDevice(NULL, 0, &format, NULL, 0);
    if (!audio.device) {
        fprintf(stderr, "audio open failed: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    static const char *sfx_files[SFX_COUNT] = {
        "shuffle_riffle.wav", "deck_tap.wav", "place.wav", "draw.wav",
        "invalid.wav", "win_chime.wav", "menu.wav"
    };
    char path[512];
    for (int i = 0; i < SFX_COUNT; ++i) {
        snprintf(path, sizeof path, "%s/sfx/%s", asset_dir, sfx_files[i]);
        if (!load_wav_pcm(path, &audio.sfx[i])) synth_pcm((SfxId)i, &audio.sfx[i]);
    }
    snprintf(path, sizeof path, "%s/music/menu_theme.wav", asset_dir);
    (void)load_wav_pcm(path, &audio.music);
    audio.ready = true;
    audio.music_target = 0.0f;
    audio.music_gain = 0.0f;
    audio.music_pos = 0;
    SDL_PauseAudioDevice(audio.device, 0);
    fprintf(stderr, "audio: mixed device ok @ %d Hz, menu music %s\n",
            SAMPLE_RATE, audio.music.samples ? "loaded" : "missing");
    return true;
}

void audio_close(void) {
    if (audio.device) {
        SDL_CloseAudioDevice(audio.device);
        audio.device = 0;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    for (int i = 0; i < SFX_COUNT; ++i) free_pcm(&audio.sfx[i]);
    free_pcm(&audio.music);
    memset(audio.voices, 0, sizeof audio.voices);
    audio.ready = false;
}

void audio_set_enabled(bool enabled) {
    audio.enabled = enabled;
    audio.music_target = (enabled && audio.music_enabled && audio.menu_music_active) ? audio.music_volume : 0.0f;
}

bool audio_enabled(void) { return audio.enabled && audio.ready && audio.device != 0; }

void audio_set_music_enabled(bool enabled) {
    audio.music_enabled = enabled;
    audio.music_target = (audio.enabled && audio.music_enabled && audio.menu_music_active) ? audio.music_volume : 0.0f;
}

bool audio_music_enabled(void) { return audio.music_enabled && audio_enabled(); }

void audio_set_music_volume(float volume_0_to_1) {
    audio.music_volume = clampf(volume_0_to_1, 0.0f, 1.0f);
    audio.music_target = (audio.enabled && audio.music_enabled && audio.menu_music_active) ? audio.music_volume : 0.0f;
}

void audio_set_menu_music_active(bool active) {
    audio.menu_music_active = active;
    audio.music_target = (audio.enabled && audio.music_enabled && active) ? audio.music_volume : 0.0f;
}

void audio_play(SfxId id) {
    if (!audio_enabled() || id >= SFX_COUNT || !audio.sfx[id].samples) return;
    SDL_LockAudioDevice(audio.device);
    int slot = -1;
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (!audio.voices[i].active) { slot = i; break; }
    }
    if (slot < 0) slot = 0;
    audio.voices[slot] = (Voice){
        .samples = audio.sfx[id].samples,
        .count = audio.sfx[id].count,
        .position = 0,
        .gain = 1.0f,
        .active = true
    };
    SDL_UnlockAudioDevice(audio.device);
}
