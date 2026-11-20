#include "render.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef CARDGAME_ASSET_DIR
#define CARDGAME_ASSET_DIR "assets/runtime"
#endif
static void usage(void) {
    puts("card-preview [--stress] [--theme blue|wine] [--seed N|random] [--scale 1..4]\n"
         "             [--capture FILE.bmp] [--smoke]\n"
         "Escape quits; T toggles stress; B switches back theme. Courts are temporary art.");
}
int main(int argc,char **argv) {
    bool stress=false,alternate=false,smoke=false; int scale=1; uint32_t seed=42; const char *capture=NULL;
    for(int i=1;i<argc;++i) {
        if(!strcmp(argv[i],"--help")) { usage(); return 0; }
        if(!strcmp(argv[i],"--stress")) stress=true;
        else if(!strcmp(argv[i],"--smoke")) smoke=true;
        else if(!strcmp(argv[i],"--capture") && i+1<argc) capture=argv[++i];
        else if(!strcmp(argv[i],"--theme") && i+1<argc) {
            const char *value=argv[++i]; if(strcmp(value,"blue") && strcmp(value,"wine")) { usage(); return 2; } alternate=!strcmp(value,"wine");
        } else if((!strcmp(argv[i],"--seed") || !strcmp(argv[i],"--scale")) && i+1<argc) {
            bool is_seed=!strcmp(argv[i],"--seed"); const char *v=argv[++i];
            if(is_seed && !strcmp(v,"random")) {
                FILE *f=fopen("/dev/urandom","rb");
                if(!f || fread(&seed,sizeof(seed),1,f)!=1) { if(f) fclose(f); fputs("Cannot obtain random seed\n",stderr); return 1; } fclose(f);
            } else {
                char *end; errno=0; unsigned long n=strtoul(v,&end,10);
                if(errno || !*v || *end || *v=='-' || n>UINT32_MAX || (!is_seed && (n<1 || n>4))) { usage(); return 2; }
                if(is_seed) seed=(uint32_t)n; else scale=(int)n;
            }
        } else { usage(); return 2; }
    }
    printf("Preview seed: %u; temporary court illustrations\n",seed);
    if(SDL_Init(SDL_INIT_VIDEO)!=0) goto fail;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
    SDL_Window *w=SDL_CreateWindow("Cardgame-RX static preview",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,640*scale,480*scale,0);
    if(!w) { SDL_Quit(); goto fail; }
    SDL_Renderer *r=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);
    if(!r) { SDL_DestroyWindow(w); SDL_Quit(); goto fail; }
    SDL_Surface *surface=SDL_CreateRGBSurfaceWithFormat(0,640,480,32,SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *canvas=surface ? SDL_CreateSoftwareRenderer(surface) : NULL;
    int result=0;
    if(!canvas) { result=1; goto cleanup; }
    (void)render_assets_open(canvas,CARDGAME_ASSET_DIR);
    Scene scene; Layout layout=layout_default(); bool running=true;
    while(running) {
        scene_create(&scene,seed,stress); render_scene(canvas,&scene,&layout,back_theme(alternate),stress); SDL_RenderPresent(canvas);
        if(capture) { if(SDL_SaveBMP(surface,capture)!=0) result=1; break; }
        SDL_Texture *texture=SDL_CreateTextureFromSurface(r,surface);
        if(!texture) { result=1; break; }
        SDL_RenderClear(r); SDL_RenderCopy(r,texture,NULL,NULL); SDL_RenderPresent(r); SDL_DestroyTexture(texture);
        if(smoke) break;
        SDL_Event event;
        if(!SDL_WaitEvent(&event)) { result=1; break; }
        if(event.type==SDL_QUIT || (event.type==SDL_KEYDOWN && event.key.keysym.sym==SDLK_ESCAPE)) running=false;
        if(event.type==SDL_KEYDOWN && !event.key.repeat) {
            if(event.key.keysym.sym==SDLK_t) stress=!stress;
            if(event.key.keysym.sym==SDLK_b) alternate=!alternate;
        }
    }
cleanup:
    if(result) fprintf(stderr,"SDL: %s\n",SDL_GetError());
    render_assets_close();
    if(canvas) SDL_DestroyRenderer(canvas);
    if(surface) SDL_FreeSurface(surface);
    SDL_DestroyRenderer(r); SDL_DestroyWindow(w); SDL_Quit(); return result;
fail:
    fprintf(stderr,"SDL: %s\n",SDL_GetError()); return 1;
}
