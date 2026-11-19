#ifndef CARDS_RENDER_H
#define CARDS_RENDER_H
#include <SDL.h>
#include "layout.h"
#include "scene.h"
typedef struct { SDL_Color base,ink; bool diamonds; } BackTheme;
BackTheme back_theme(bool alternate);
void render_text(SDL_Renderer *r,int x,int y,const char *text,int scale);
/* Caller supplies a valid card and rectangle from layout. */
void render_card(SDL_Renderer *r,Rect rect,const CardState *card,BackTheme theme);
void render_scene(SDL_Renderer *r,const Scene *s,const Layout *l,BackTheme theme,bool stress);
#endif
