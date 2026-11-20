#include "desktop.h"
#include "play_render.h"
#include "play_view.h"
#include "audio.h"
#include "save.h"
#include "fx.h"
#include "rx_input.h"
#include "sdl_compat.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef CARDGAME_ASSET_DIR
#define CARDGAME_ASSET_DIR "assets/runtime"
#endif
#ifndef CARDGAME_FONT_PATH
#define CARDGAME_FONT_PATH "assets/fonts/PermanentMarker-Regular.ttf"
#endif

static void usage(void) {
    puts("cardgame [--seed N|random] [--scale 1..4] [--theme blue|wine]\n"
         "         [--bind action=KeyName] [--undo-limit N] [--capture FILE.bmp] [--smoke]\n"
         "         [--no-anim] [--no-audio] [--mute-music] [--music-volume 0..100]\n"
         "Devkit keys: arrows navigate, S=B, D=A/cancel, W=X/foundation, A=Y/stock,\n"
         "Left Shift/Space=L1/R1, Left Ctrl/Left Alt=L2/R2, E=Start, Q=Select, Escape=Quit.\n"
         "RG35XXSP: ANBERNIC-keys joystick (D-pad hat, A/B/X/Y/L/R/Start/Select).\n"
         "Pause: Up/Down and A choose, B backs out. New game requires confirmation.");
}

static Rect pile_rect(const Layout *l, KlondikePile ref) {
    if (ref.zone == K_TABLEAU) return layout_slot(l, ref.index, true);
    if (ref.zone == K_FOUNDATION) return layout_slot(l, ref.index + 3, false);
    if (ref.zone == K_WASTE) return layout_slot(l, 1, false);
    return layout_slot(l, 0, false);
}

static void deal_intro_fx(PlayFX *fx, const KlondikeState *s, const Layout *l, uint64_t now) {
    if (!fx_enabled(fx) || !s || !l) return;
    Rect stock = layout_slot(l, 0, false);
    unsigned n = 0;
    for (unsigned row = 0; row < 7 && n < FX_MAX_MOTIONS; ++row) {
        for (unsigned col = row; col < 7 && n < FX_MAX_MOTIONS; ++col) {
            const Pile *p = &s->tableau[col];
            if (row >= p->count) continue;
            Rect to = play_card_rect(l, p, col, row);
            CardState c = p->cards[row];
            fx_push_motion(fx, FX_DEAL, stock, to, c, now + n * 10u, 110u);
            ++n;
        }
    }
}

static void start_new_game_audio(bool sound) {
    if (!sound) return;
    audio_play(SFX_SHUFFLE);
    audio_play(SFX_DECK_TAP);
}

static void handle_play_event(PlayEvent pe, PlayUI *ui, KlondikeGame *game, PlayFX *fx,
                              const Layout *layout, bool sound, uint64_t now,
                              PlayScreen before, KlondikePile focus_before) {
    if (ui->screen != before) {
        fx_menu_change(fx, before, ui->screen, now);
        if (sound && ui->screen != MENU_PLAYING) audio_play(SFX_MENU);
    }
    if (pe == PLAY_NEW_GAME) {
        start_new_game_audio(sound);
        fx_clear(fx);
        deal_intro_fx(fx, &game->state, layout, now);
    } else if (pe == PLAY_COMMITTED) {
        if (sound) audio_play(ui->message == MSG_DRAW || ui->message == MSG_RECYCLE ? SFX_DRAW : SFX_PLACE);
        if (fx_enabled(fx)) {
            Rect from = pile_rect(layout, focus_before);
            Rect to = play_focus_rect(layout, &game->state, ui);
            CardState c = {.face_up = true};
            const Pile *p = klondike_pile(&game->state, play_focus(ui));
            if (p && p->count) c = p->cards[p->count - 1];
            fx_push_motion(fx, ui->message == MSG_DRAW ? FX_FLIP : FX_SETTLE, from, to, c, now,
                           ui->message == MSG_DRAW ? 200u : 160u);
        }
    } else if (pe == PLAY_ERROR) {
        if (sound) audio_play(SFX_INVALID);
    }
}


static SaveStatus write_autosave_now(PlayUI *ui, KlondikeGame *game) {
    play_tick(ui, 0); /* flush not needed without now; elapsed already tracked */
    SaveMeta meta;
    play_fill_meta(ui, &meta);
    meta.undo_limit = (uint32_t)game->undo_limit;
    SaveBlob blob;
    if (!save_capture(&blob, game, &meta)) return SAVE_ERR_STATE;
    return save_write_autosave(&blob);
}

static SaveStatus write_slot_now(PlayUI *ui, KlondikeGame *game, unsigned slot) {
    SaveMeta meta;
    play_fill_meta(ui, &meta);
    meta.undo_limit = (uint32_t)game->undo_limit;
    SaveBlob blob;
    if (!save_capture(&blob, game, &meta)) return SAVE_ERR_STATE;
    return save_write_slot(slot, &blob);
}

static bool load_autosave_now(PlayUI *ui, KlondikeGame *game, uint64_t now) {
    SaveBlob blob;
    if (save_read_autosave(&blob) != SAVE_OK) return false;
    SaveMeta meta;
    if (!save_restore(&blob, game, &meta)) return false;
    play_init(ui, &game->state, meta.seed);
    play_apply_loaded(ui, &meta, now);
    return true;
}

static bool load_slot_now(PlayUI *ui, KlondikeGame *game, unsigned slot, uint64_t now) {
    SaveBlob blob;
    if (save_read_slot(slot, &blob) != SAVE_OK) return false;
    SaveMeta meta;
    if (!save_restore(&blob, game, &meta)) return false;
    play_init(ui, &game->state, meta.seed);
    play_apply_loaded(ui, &meta, now);
    return true;
}

static void refresh_continue(PlayUI *ui) {
    SaveMeta meta;
    play_set_continue_available(ui, save_probe_autosave(&meta) == SAVE_OK);
}


int main(int argc, char **argv) {
    uint32_t seed = 42; int scale = 1; bool alternate = false, smoke = false;
    bool anim = true, sound = true, music = true;
    int music_volume_pct = 42;
    const char *capture = NULL; size_t undo_limit = 256;
    KeyboardMap keys = keyboard_default();
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help")) { usage(); return 0; }
        if (!strcmp(argv[i], "--smoke")) smoke = true;
        else if (!strcmp(argv[i], "--no-anim")) anim = false;
        else if (!strcmp(argv[i], "--no-audio")) sound = false;
        else if (!strcmp(argv[i], "--mute-music")) music = false;
        else if (!strcmp(argv[i], "--capture") && i + 1 < argc) capture = argv[++i];
        else if (!strcmp(argv[i], "--bind") && i + 1 < argc) {
            if (!keyboard_bind(&keys, argv[++i])) { fputs("Invalid key binding\n", stderr); return 2; }
        } else if (!strcmp(argv[i], "--theme") && i + 1 < argc) {
            const char *theme = argv[++i];
            if (strcmp(theme, "blue") && strcmp(theme, "wine")) { usage(); return 2; }
            alternate = !strcmp(theme, "wine");
        } else if ((!strcmp(argv[i], "--seed") || !strcmp(argv[i], "--scale") ||
                    !strcmp(argv[i], "--undo-limit") || !strcmp(argv[i], "--music-volume")) && i + 1 < argc) {
            bool is_seed = !strcmp(argv[i], "--seed"), is_limit = !strcmp(argv[i], "--undo-limit");
            bool is_music_volume = !strcmp(argv[i], "--music-volume");
            const char *value = argv[++i];
            if (is_seed && !strcmp(value, "random")) {
                FILE *f = fopen("/dev/urandom", "rb");
                if (!f || fread(&seed, sizeof(seed), 1, f) != 1) {
                    if (f) fclose(f);
                    fputs("Cannot obtain a random seed\n", stderr); return 1;
                }
                fclose(f);
            } else {
                char *end; errno = 0; unsigned long n = strtoul(value, &end, 10);
                if (errno || !*value || *end || *value == '-' || (!is_limit && n > UINT32_MAX) ||
                    (is_limit && n > SIZE_MAX) ||
                    (!is_seed && !is_limit && !is_music_volume && (n < 1 || n > 4)) ||
                    (is_music_volume && n > 100)) {
                    usage(); return 2;
                }
                if (is_seed) seed = (uint32_t)n;
                else if (is_limit) undo_limit = (size_t)n;
                else if (is_music_volume) music_volume_pct = (int)n;
                else scale = (int)n;
            }
        } else { usage(); return 2; }
    }
    if (!keyboard_valid(&keys)) { fputs("Key bindings must be unique\n", stderr); return 2; }
    printf("Klondike seed %u\n", seed);
    fflush(stdout);

    /* Joystick required on RG35XXSP; audio optional. */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
    SDL_JoystickEventState(SDL_ENABLE);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    Uint32 win_flags = SDL_WINDOW_SHOWN;
#if defined(__linux__)
    /* Handheld ports run fullscreen under mali. */
    if (getenv("SDL_VIDEODRIVER") && !strcmp(getenv("SDL_VIDEODRIVER"), "mali"))
        win_flags |= SDL_WINDOW_FULLSCREEN;
#endif
    SDL_Window *window = SDL_CreateWindow("Croupier-RX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          640 * scale, 480 * scale, win_flags);
    /* Prefer accelerated+vsync on device; fall back to software. */
    SDL_Renderer *screen = window ? SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : NULL;
    if (window && !screen)
        screen = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 640, 480, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *canvas = surface ? SDL_CreateSoftwareRenderer(surface) : NULL;
    SDL_Texture *texture = screen ? SDL_CreateTexture(screen, SDL_PIXELFORMAT_ARGB8888,
                                                     SDL_TEXTUREACCESS_STREAMING, 640, 480) : NULL;
    int result = 0;
    KlondikeGame game = {0}; klondike_set_undo_limit(&game, undo_limit); klondike_new_game(&game, seed);
    PlayUI ui; play_init(&ui, &game.state, seed); refresh_continue(&ui);
    Layout layout = layout_default();
    PlayFX fx; fx_init(&fx, anim);
    RxInput rx; rx_input_init(&rx);
    fprintf(stderr, "input: joysticks=%d pad=%s\n", SDL_NumJoysticks(),
            rx.pad ? SDL_JoystickName(rx.pad) : "(none)");
    fflush(stderr);
    bool announced_win = false;
    if (!window || !screen || !surface || !canvas || !texture) { result = 1; goto cleanup; }
    if (SDL_RenderSetLogicalSize(screen, 640, 480) != 0)
        fprintf(stderr, "logical size: %s\n", SDL_GetError());
    (void)render_assets_open(canvas, CARDGAME_ASSET_DIR);
    (void)render_menu_font_open(canvas, CARDGAME_FONT_PATH);
    if (sound) {
        if (!audio_open(CARDGAME_ASSET_DIR))
            fprintf(stderr, "audio: unavailable (%s)\n", SDL_GetError());
    } else audio_set_enabled(false);
    audio_set_music_enabled(music);
    audio_set_music_volume((float)music_volume_pct / 100.0f);
    audio_set_menu_music_active(true);

    if (sound && audio_enabled()) start_new_game_audio(true);
    deal_intro_fx(&fx, &game.state, &layout, SDL_GetTicks64());
    fprintf(stderr, "loop: enter main menu\n");
    fflush(stderr);

    while (!ui.quit) {
        SDL_Event event;
        uint64_t now = SDL_GetTicks64();
        while (SDL_PollEvent(&event)) {
            PlayScreen before = ui.screen;
            KlondikePile focus_before = play_focus(&ui);
            PlayEvent pe = desktop_event_rx(&ui, &game, &keys, &rx, &event, now);
            if (pe == PLAY_SAVE_NEEDED) {
                play_tick(&ui, now);
                SaveStatus st = SAVE_OK;
                bool to_main = (ui.screen == MENU_CONFIRM_MAIN);
                bool to_quit = ui.exit_app || ui.screen == MENU_CONFIRM_QUIT ||
                               ui.screen == MENU_QUIT_SAVE_ERROR;
                bool slot_save = (ui.screen == MENU_SAVE_SLOTS || ui.screen == MENU_CONFIRM_OVERWRITE);
                if (slot_save) st = write_slot_now(&ui, &game, ui.slot);
                else st = write_autosave_now(&ui, &game);
                if (st != SAVE_OK) {
                    ui.save_error = (int)st;
                    ui.quit_choice = 0;
                    ui.screen = to_quit ? MENU_QUIT_SAVE_ERROR : MENU_SAVE_ERROR;
                    pe = PLAY_ERROR;
                } else {
                    ui.dirty = false;
                    if (slot_save) {
                        ui.screen = MENU_PAUSE;
                        ui.paused = true;
                        pe = PLAY_AUTOSAVED;
                    } else if (to_quit) {
                        ui.quit = true;
                        pe = PLAY_QUIT;
                    } else if (to_main || !ui.exit_app) {
                        ui.screen = MENU_MAIN;
                        ui.paused = true;
                        ui.held = false;
                        ui.menu = 0;
                        refresh_continue(&ui);
                        pe = PLAY_AUTOSAVED;
                    }
                }
            } else if (pe == PLAY_LOADED) {
                bool from_continue = (before == MENU_MAIN && ui.menu == 0 &&
                                      ui.screen != MENU_LOAD_SLOTS &&
                                      ui.screen != MENU_CONFIRM_LOAD);
                bool ok = from_continue ? load_autosave_now(&ui, &game, now)
                                        : load_slot_now(&ui, &game, ui.slot, now);
                if (!ok) {
                    pe = PLAY_ERROR;
                    ui.screen = before;
                } else {
                    announced_win = false;
                    fx_clear(&fx);
                    deal_intro_fx(&fx, &game.state, &layout, now);
                    pe = PLAY_LOADED;
                }
            }
            handle_play_event(pe, &ui, &game, &fx, &layout, sound && audio_enabled(), now, before, focus_before);
            if (pe == PLAY_NEW_GAME) announced_win = false;
            if (pe != PLAY_NONE)
                fprintf(stderr, "event pe=%d screen=%d\n", (int)pe, (int)ui.screen);
        }
        if (ui.quit) break;
        now = SDL_GetTicks64();
        play_tick(&ui, now);
        audio_set_menu_music_active(ui.screen == MENU_MAIN);
        if (klondike_won(&game.state) && ui.screen == MENU_PLAYING && !announced_win) {
            announced_win = true;
            if (sound && audio_enabled()) audio_play(SFX_WIN);
        }
        if (!klondike_won(&game.state)) announced_win = false;
        fx_update(&fx, now);
        play_render_fx(canvas, &game.state, &ui, &layout, &keys, back_theme(alternate), now, &fx);
        SDL_RenderPresent(canvas);
        if (capture) { if (SDL_SaveBMP(surface, capture) != 0) result = 1; break; }
        if (SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch) != 0 ||
            SDL_RenderClear(screen) != 0 || SDL_RenderCopy(screen, texture, NULL, NULL) != 0) {
            result = 1; break;
        }
        SDL_RenderPresent(screen);
        if (smoke) break;
        /* Cap spin if driver ignores vsync (observed high CPU freeze on RX). */
        SDL_Delay(8);
    }
cleanup:
    if (result) fprintf(stderr, "SDL: %s\n", SDL_GetError());
    fprintf(stderr, "exit result=%d\n", result);
    fflush(stderr);
    klondike_dispose(&game);
    rx_input_close(&rx);
    audio_close();
    render_assets_close();
    if (texture) SDL_DestroyTexture(texture);
    if (canvas) SDL_DestroyRenderer(canvas);
    if (surface) SDL_FreeSurface(surface);
    if (screen) SDL_DestroyRenderer(screen);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
