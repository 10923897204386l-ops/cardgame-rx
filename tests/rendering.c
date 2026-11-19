#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%s:%d: %s (%s)\n",__FILE__,__LINE__,#x,SDL_GetError()); exit(1); } } while (0)
int main(void) {
    SDL_Surface *s=SDL_CreateRGBSurfaceWithFormat(0,640,480,32,SDL_PIXELFORMAT_ARGB8888); CHECK(s);
    SDL_Renderer *r=SDL_CreateSoftwareRenderer(s); CHECK(r);
    Layout l=layout_default(); Card deck[52]; deck_standard(deck);
    /* For all identities, covering the next card at the default overlap must
       preserve the complete 14-pixel-high grouped index, pixel for pixel. */
    for (int i=0;i<52;++i) {
        CardState c={deck[i],true},cover={{CLUBS,1},false}; Rect rect={20,164,72,100};
        render_card(r,rect,&c,back_theme(false)); SDL_RenderPresent(r);
        unsigned char saved[18][72*4];
        for(int row=0;row<18;++row) memcpy(saved[row],(unsigned char *)s->pixels+(168+row)*s->pitch+20*4,72*4);
        rect.y+=24; render_card(r,rect,&cover,back_theme(false)); SDL_RenderPresent(r);
        for(int row=0;row<18;++row) CHECK(!memcmp(saved[row],(unsigned char *)s->pixels+(168+row)*s->pitch+20*4,72*4));
    }
    for(int stress=0;stress<2;++stress) {
        Scene scene,original; scene_create(&scene,42,stress); original=scene;
        render_scene(r,&scene,&l,back_theme(false),stress); SDL_RenderPresent(r);
        Uint32 before=*((Uint32 *)((unsigned char *)s->pixels+50*s->pitch)+30);
        render_scene(r,&scene,&l,back_theme(true),stress); SDL_RenderPresent(r);
        Uint32 after=*((Uint32 *)((unsigned char *)s->pixels+50*s->pitch)+30); CHECK(before!=after);
        CHECK(!memcmp(&scene,&original,sizeof(scene)));
        /* Fixed top status bar; bottom-right remains green table. */
        Uint8 red,green,blue; SDL_GetRGB(*((Uint32 *)((unsigned char *)s->pixels+479*s->pitch)+639),s->format,&red,&green,&blue);
        CHECK(red==22 && green==83 && blue==64);
    }
    SDL_DestroyRenderer(r); SDL_FreeSurface(s);
    puts("rendering: all 52 exposed indexes, themes, immutable scenes, no bottom bar passed"); return 0;
}
