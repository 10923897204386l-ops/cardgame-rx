#include "play_render.h"
#include <stdio.h>

static void color(SDL_Renderer *r, SDL_Color c) { SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); }
static void fill(SDL_Renderer *r, Rect b) { SDL_Rect rect = {b.x,b.y,b.w,b.h}; SDL_RenderFillRect(r,&rect); }
static void outline(SDL_Renderer *r, Rect b, int expand, SDL_Color c) {
    color(r,c); SDL_Rect rect = {b.x-expand,b.y-expand,b.w+2*expand,b.h+2*expand}; SDL_RenderDrawRect(r,&rect);
}
static void ghost(SDL_Renderer *r, Rect b) {
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    color(r,(SDL_Color){60,83,76,130}); fill(r,b);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    outline(r,b,0,(SDL_Color){170,187,177,255});
}
static void top_pile(SDL_Renderer *r, const Pile *p, Rect b, const PlayUI *ui,
                     KlondikePile ref, BackTheme theme) {
    outline(r,b,0,(SDL_Color){119,163,137,255});
    const CardState *top = pile_top(p);
    if (top) {
        render_card(r,b,top,theme);
        if (ui->held && ui->source.zone == ref.zone && ui->source.index == ref.index) ghost(r,b);
    } else {
        color(r,(SDL_Color){119,163,137,255});
        render_text(r,b.x+8,b.y+44,ref.zone == K_STOCK ? "STOCK" : ref.zone == K_WASTE ? "WASTE" : "ACE",1);
    }
}
static void pause_overlay(SDL_Renderer *r, const PlayUI *ui) {
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    color(r,(SDL_Color){0,0,0,165}); fill(r,(Rect){0,28,640,452});
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    color(r,(SDL_Color){18,37,34,255}); fill(r,(Rect){130,130,380,220});
    outline(r,(Rect){130,130,380,220},0,(SDL_Color){226,219,175,255});
    color(r,(SDL_Color){255,249,226,255});
    if (ui->confirming) {
        render_text(r,154,155,"DISCARD AND DEAL NEW GAME",2);
        render_text(r,190,217,ui->confirm_yes ? "NO    -YES-" : "-NO-    YES",2);
    } else {
        render_text(r,166,155,"PAUSED",2);
        const char *items[] = {"RESUME","NEW GAME","QUIT"};
        for (unsigned i = 0; i < 3; ++i) {
            if (ui->menu == i) outline(r,(Rect){158,191+(int)i*40,320,30},0,(SDL_Color){255,219,90,255});
            color(r,(SDL_Color){255,249,226,255}); render_text(r,180,199+(int)i*40,items[i],2);
        }
    }
}
void play_render(SDL_Renderer *r, const KlondikeState *s, const PlayUI *ui,
                 const Layout *l, BackTheme theme, uint64_t now) {
    color(r,(SDL_Color){22,83,64,255}); SDL_RenderClear(r);
    top_pile(r,&s->stock,layout_slot(l,0,false),ui,(KlondikePile){K_STOCK,0},theme);
    top_pile(r,&s->waste,layout_slot(l,1,false),ui,(KlondikePile){K_WASTE,0},theme);
    for (unsigned f = 0; f < 4; ++f)
        top_pile(r,&s->foundations[f],layout_slot(l,f+3,false),ui,(KlondikePile){K_FOUNDATION,f},theme);
    for (unsigned col = 0; col < 7; ++col) {
        const Pile *p = &s->tableau[col];
        outline(r,layout_slot(l,col,true),0,(SDL_Color){119,163,137,255});
        for (size_t i = 0; i < p->count; ++i) {
            Rect b = play_card_rect(l,p,col,i);
            render_card(r,b,&p->cards[i],theme);
            if (ui->held && ui->source.zone == K_TABLEAU && ui->source.index == col && i >= ui->source_start) ghost(r,b);
            if (i+1 == p->count && b.y+b.h > CANVAS_HEIGHT) {
                /* Visible ellipsis signals cropped pip art; index stays whole. */
                color(r,(SDL_Color){22,25,31,255});
                for (int dot = 0; dot < 3; ++dot) fill(r,(Rect){b.x+b.w-16+dot*4,475,2,2});
            }
        }
        if (p->count) {
            Rect top = play_card_rect(l,p,col,p->count-1);
            if (top.y+top.h > 480) {
                char label[16]; snprintf(label,sizeof(label),"DOWN %zu",play_first_face_up(p));
                color(r,(SDL_Color){199,219,190,255}); render_text(r,top.x+2,l->tableau_y-10,label,1);
            }
        }
    }
    size_t chosen = 0; bool legal = play_preview(ui,s,&chosen);
    SDL_Color focus_color = legal ? (SDL_Color){111,255,148,255} : (SDL_Color){255,219,90,255};
    Rect focus = play_focus_rect(l,s,ui);
    outline(r,focus,2,focus_color);
    if (ui->held) {
        Rect bounds = play_held_bounds(l,s,ui);
        const Pile *p = klondike_pile(s,ui->source);
        size_t count = play_held_count(ui,s);
        for (size_t i = 0; i < count; ++i) {
            Rect b = {bounds.x,bounds.y+(int)i*play_held_overlap(l,count),l->card_w,l->card_h};
            render_card(r,b,&p->cards[ui->source_start+i],theme);
        }
        SDL_Color held_color = play_invalid_outline(ui,now) ? (SDL_Color){255,42,54,255} : (SDL_Color){119,218,255,255};
        outline(r,bounds,3,held_color); outline(r,bounds,4,held_color);
    }
    /* Fixed status strip, always on top of table/carry; no bottom bar. */
    color(r,(SDL_Color){16,34,32,255}); fill(r,(Rect){0,0,640,l->status_h});
    color(r,(SDL_Color){255,249,226,255}); char label[96];
    KlondikePile ref = play_focus(ui);
    if (ref.zone == K_TABLEAU) snprintf(label,sizeof(label),"TABLEAU %u",ref.index+1);
    else if (ref.zone == K_FOUNDATION) snprintf(label,sizeof(label),"FOUNDATION %u",ref.index+1);
    else snprintf(label,sizeof(label),"DRAW %s",ref.zone == K_STOCK ? "STOCK" : "WASTE");
    render_text(r,10,5,label,1);
    snprintf(label,sizeof(label),"SEED %u  STOCK %zu",ui->seed,s->stock.count);
    render_text(r,10,17,label,1);
    if (ui->held) {
        size_t n = play_held_count(ui,s);
        snprintf(label,sizeof(label),"HOLD %zu  PLACE %zu",n,legal ? klondike_pile(s,ui->source)->count-chosen : 0);
        render_text(r,160,5,label,1);
    }
    render_text(r,350,5,play_message(ui,now),1);
    render_text(r,500,17,"START PAUSE",1);
    if (klondike_won(s)) render_text(r,230,17,"YOU WIN - START NEW GAME",1);
    if (ui->paused) pause_overlay(r,ui);
}
