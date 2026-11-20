#include "desktop.h"
#include "play_render.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef CARDGAME_ASSET_DIR
#define CARDGAME_ASSET_DIR "assets/runtime"
#endif
#ifndef CARDGAME_FONT_PATH
#define CARDGAME_FONT_PATH "assets/fonts/GoRXInk.ttf"
#endif

static void usage(void) {
    puts("cardgame [--seed N|random] [--scale 1..4] [--theme blue|wine]\n"
         "         [--bind action=KeyName] [--undo-limit N] [--capture FILE.bmp] [--smoke]\n"
         "Devkit keys: arrows navigate, S=B, D=A/cancel, W=X/foundation, A=Y/stock,\n"
         "Left Shift/Space=L1/R1, Left Ctrl/Left Alt=L2/R2, E=Start, Q=Select, Escape=Quit.\n"
         "Pause: Up/Down and A choose, B backs out. New game requires confirmation.\n"
         "Static/stress preview remains a separate executable: card-preview.");
}
int main(int argc, char **argv) {
    uint32_t seed = 42; int scale = 1; bool alternate = false, smoke = false;
    const char *capture = NULL; size_t undo_limit = 256;
    KeyboardMap keys = keyboard_default();
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i],"--help")) { usage(); return 0; }
        if (!strcmp(argv[i],"--smoke")) smoke = true;
        else if (!strcmp(argv[i],"--capture") && i+1 < argc) capture = argv[++i];
        else if (!strcmp(argv[i],"--bind") && i+1 < argc) {
            if (!keyboard_bind(&keys,argv[++i])) { fputs("Invalid key binding\n",stderr); return 2; }
        } else if (!strcmp(argv[i],"--theme") && i+1 < argc) {
            const char *theme = argv[++i];
            if (strcmp(theme,"blue") && strcmp(theme,"wine")) { usage(); return 2; }
            alternate = !strcmp(theme,"wine");
        } else if ((!strcmp(argv[i],"--seed") || !strcmp(argv[i],"--scale") || !strcmp(argv[i],"--undo-limit")) && i+1 < argc) {
            bool is_seed = !strcmp(argv[i],"--seed"), is_limit = !strcmp(argv[i],"--undo-limit"); const char *value = argv[++i];
            if (is_seed && !strcmp(value,"random")) {
                FILE *f = fopen("/dev/urandom","rb");
                if (!f || fread(&seed,sizeof(seed),1,f) != 1) {
                    if (f) fclose(f);
                    fputs("Cannot obtain a random seed\n",stderr); return 1;
                }
                fclose(f);
            } else {
                char *end; errno = 0; unsigned long n = strtoul(value,&end,10);
                if (errno || !*value || *end || *value == '-' || (!is_limit && n > UINT32_MAX) || (is_limit && n > SIZE_MAX) || (!is_seed && !is_limit && (n < 1 || n > 4))) { usage(); return 2; }
                if (is_seed) seed = (uint32_t)n; else if (is_limit) undo_limit = (size_t)n; else scale = (int)n;
            }
        } else { usage(); return 2; }
    }
    if (!keyboard_valid(&keys)) { fputs("Key bindings must be unique\n",stderr); return 2; }
    printf("Klondike seed %u; temporary court art; desktop keyboard controls only\n",seed);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr,"SDL: %s\n",SDL_GetError()); return 1; }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
    SDL_Window *window = SDL_CreateWindow("Cardgame-RX Klondike",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,640*scale,480*scale,0);
    SDL_Renderer *screen = window ? SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE) : NULL;
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0,640,480,32,SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *canvas = surface ? SDL_CreateSoftwareRenderer(surface) : NULL;
    SDL_Texture *texture = screen ? SDL_CreateTexture(screen,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,640,480) : NULL;
    int result = 0;
    KlondikeGame game = {0}; klondike_set_undo_limit(&game,undo_limit); klondike_new_game(&game,seed);
    PlayUI ui; play_init(&ui,&game.state,seed); Layout layout = layout_default();
    if (!window || !screen || !surface || !canvas || !texture) { result = 1; goto cleanup; }
    (void)render_assets_open(canvas,CARDGAME_ASSET_DIR);
    (void)render_menu_font_open(canvas,CARDGAME_FONT_PATH);
    while (!ui.quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) (void)desktop_event(&ui,&game,&keys,&event,SDL_GetTicks64());
        if (ui.quit) break;
        play_render(canvas,&game.state,&ui,&layout,back_theme(alternate),SDL_GetTicks64());
        SDL_RenderPresent(canvas);
        if (capture) { if (SDL_SaveBMP(surface,capture) != 0) result = 1; break; }
        if (SDL_UpdateTexture(texture,NULL,surface->pixels,surface->pitch) != 0 ||
            SDL_RenderClear(screen) != 0 || SDL_RenderCopy(screen,texture,NULL,NULL) != 0) { result = 1; break; }
        SDL_RenderPresent(screen);
        if (smoke) break;
        /* A bounded event wait keeps invalid-outline expiry live even with no
           input, without sleeping/blocking the event loop during feedback. */
        if (SDL_WaitEventTimeout(&event,16)) (void)desktop_event(&ui,&game,&keys,&event,SDL_GetTicks64());
    }
cleanup:
    if (result) fprintf(stderr,"SDL: %s\n",SDL_GetError());
    klondike_dispose(&game);
    render_assets_close();
    if (texture) SDL_DestroyTexture(texture);
    if (canvas) SDL_DestroyRenderer(canvas);
    if (surface) SDL_FreeSurface(surface);
    if (screen) SDL_DestroyRenderer(screen);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit(); return result;
}
