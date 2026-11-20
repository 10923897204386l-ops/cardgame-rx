#ifndef CARDS_RENDER_H
#define CARDS_RENDER_H
#include <SDL.h>
#include "layout.h"
#include "scene.h"
typedef struct { SDL_Color base,ink; bool diamonds; } BackTheme;
BackTheme back_theme(bool alternate);
bool render_assets_open(SDL_Renderer *renderer, const char *asset_dir);
void render_assets_close(void);
void render_table_background(SDL_Renderer *renderer);
bool render_title_background(SDL_Renderer *renderer);
void render_text(SDL_Renderer *r,int x,int y,const char *text,int scale);
void render_round_fill(SDL_Renderer *r,Rect rect,int radius,SDL_Color color);
void render_round_outline(SDL_Renderer *r,Rect rect,int radius,int thickness,SDL_Color color);
/* Caller supplies a valid card and rectangle from layout. */
void render_card(SDL_Renderer *r,Rect rect,const CardState *card,BackTheme theme);
void render_scene(SDL_Renderer *r,const Scene *s,const Layout *l,BackTheme theme,bool stress);
#endif
