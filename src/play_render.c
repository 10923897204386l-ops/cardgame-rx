#include "play_render.h"
#include <stdio.h>

static void color(SDL_Renderer *r, SDL_Color c) { SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); }
static void fill(SDL_Renderer *r, Rect b) { SDL_Rect rect = {b.x,b.y,b.w,b.h}; SDL_RenderFillRect(r,&rect); }
static void outline(SDL_Renderer *r, Rect b, int expand, SDL_Color c) {
    render_round_outline(r,(Rect){b.x-expand,b.y-expand,b.w+2*expand,b.h+2*expand},8,1,c);
}
static void top_pile_visible(SDL_Renderer *r, const Pile *p, Rect b, const PlayUI *ui,
                             KlondikePile ref, BackTheme theme) {
    outline(r,b,0,(SDL_Color){119,163,137,255});
    const CardState *top = pile_top(p);
    if (ui->held && ui->source.zone == ref.zone && ui->source.index == ref.index) {
        top = ui->source_start ? &p->cards[ui->source_start - 1] : NULL;
    }
    if (top) render_card(r,b,top,theme);
    else {
        color(r,(SDL_Color){119,163,137,255});
        render_text(r,b.x+8,b.y+44,ref.zone == K_STOCK ? "STOCK" : ref.zone == K_WASTE ? "WASTE" : "ACE",1);
    }
}
static void menu_panel(SDL_Renderer *r, Rect b) {
    render_round_fill(r,b,12,(SDL_Color){246,229,160,255});
    render_round_outline(r,b,12,3,(SDL_Color){43,35,24,255});
}
static void menu_item(SDL_Renderer *r, int y, const char *label, bool selected) {
    Rect row = {160,y-7,320,31};
    if (selected) {
        render_round_fill(r,row,8,(SDL_Color){255,205,78,255});
        render_round_outline(r,row,8,2,(SDL_Color){43,35,24,255});
    }
    color(r,(SDL_Color){43,35,24,255});
    render_text(r,200,y,label,2);
}
static void menu_overlay(SDL_Renderer *r, const PlayUI *ui) {
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    color(r,(SDL_Color){0,0,0,ui->screen == MENU_MAIN ? 40 : 165}); fill(r,(Rect){0,0,640,480});
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    if (ui->screen == MENU_CONFIRM_NEW || ui->screen == MENU_CONFIRM_MAIN) {
        menu_panel(r,(Rect){115,145,410,170});
        color(r,(SDL_Color){43,35,24,255});
        render_text(r,148,170,ui->screen == MENU_CONFIRM_MAIN ? "RETURN TO MAIN MENU" : "DISCARD AND DEAL NEW GAME",2);
        render_text(r,205,230,ui->confirm_yes ? "NO    -YES-" : "-NO-    YES",2);
        render_text(r,190,278,"A SELECT   B BACK",1);
    } else if (ui->screen == MENU_OPTIONS) {
        menu_panel(r,(Rect){120,128,400,220});
        color(r,(SDL_Color){43,35,24,255});
        render_text(r,212,154,"OPTIONS",2);
        render_text(r,160,215,"No configurable options yet",1);
        render_text(r,188,296,"A OR B BACK",1);
    } else {
        menu_panel(r,ui->screen == MENU_MAIN ? (Rect){126,116,388,264} : (Rect){126,104,388,292});
        color(r,(SDL_Color){43,35,24,255});
        render_text(r,180,136,ui->screen == MENU_MAIN ? "CROUPIER-RX" : "PAUSED",2);
        const char *main_items[] = {"PLAY","NEW GAME","OPTIONS","QUIT"};
        const char *pause_items[] = {"RESUME","NEW GAME","OPTIONS","MAIN MENU"};
        const char **items = ui->screen == MENU_MAIN ? main_items : pause_items;
        int y0 = ui->screen == MENU_MAIN ? 190 : 170;
        for (unsigned i = 0; i < 4; ++i) menu_item(r,y0+(int)i*42,items[i],ui->menu == i);
        color(r,(SDL_Color){43,35,24,255});
        render_text(r,190,ui->screen == MENU_MAIN ? 350 : 394,"A SELECT   B BACK",1);
    }
}
void play_render(SDL_Renderer *r, const KlondikeState *s, const PlayUI *ui,
                 const Layout *l, BackTheme theme, uint64_t now) {
    if (ui->screen == MENU_MAIN) {
        if (!render_title_background(r)) render_table_background(r);
        menu_overlay(r,ui);
        return;
    }
    render_table_background(r);
    top_pile_visible(r,&s->stock,layout_slot(l,0,false),ui,(KlondikePile){K_STOCK,0},theme);
    top_pile_visible(r,&s->waste,layout_slot(l,1,false),ui,(KlondikePile){K_WASTE,0},theme);
    for (unsigned f = 0; f < 4; ++f)
        top_pile_visible(r,&s->foundations[f],layout_slot(l,f+3,false),ui,(KlondikePile){K_FOUNDATION,f},theme);
    for (unsigned col = 0; col < 7; ++col) {
        const Pile *p = &s->tableau[col];
        outline(r,layout_slot(l,col,true),0,(SDL_Color){119,163,137,255});
        size_t visible_count = p->count;
        if (ui->held && ui->source.zone == K_TABLEAU && ui->source.index == col)
            visible_count = ui->source_start;
        for (size_t i = 0; i < visible_count; ++i) {
            Rect b = play_card_rect(l,p,col,i);
            render_card(r,b,&p->cards[i],theme);
            if (i+1 == visible_count && b.y+b.h > CANVAS_HEIGHT) {
                /* Visible ellipsis signals cropped pip art; index stays whole. */
                color(r,(SDL_Color){22,25,31,255});
                for (int dot = 0; dot < 3; ++dot) fill(r,(Rect){b.x+b.w-16+dot*4,475,2,2});
            }
        }
        if (visible_count) {
            Rect top = play_card_rect(l,p,col,visible_count-1);
            if (top.y+top.h > 480) {
                char label[16]; snprintf(label,sizeof(label),"DOWN %zu",play_first_face_up(p));
                color(r,(SDL_Color){199,219,190,255}); render_text(r,top.x+2,l->tableau_y-10,label,1);
            }
        }
    }
    size_t chosen = 0; bool legal = play_preview(ui,s,&chosen);
    SDL_Color focus_color = legal ? (SDL_Color){111,255,148,255} : (SDL_Color){255,219,90,255};
    Rect focus = play_focus_rect(l,s,ui);
    render_round_outline(r,(Rect){focus.x-2,focus.y-2,focus.w+4,focus.h+4},10,4,focus_color);
    if (ui->held) {
        Rect bounds = play_held_bounds(l,s,ui);
        const Pile *p = klondike_pile(s,ui->source);
        size_t count = play_held_count(ui,s);
        for (size_t i = 0; i < count; ++i) {
            Rect b = {bounds.x,bounds.y+(int)i*play_held_overlap(l,count),l->card_w,l->card_h};
            render_card(r,b,&p->cards[ui->source_start+i],theme);
        }
        SDL_Color held_color = play_invalid_outline(ui,now) ? (SDL_Color){255,42,54,255} : (SDL_Color){119,218,255,255};
        render_round_outline(r,bounds,10,4,held_color);
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
    if (ui->screen != MENU_PLAYING) menu_overlay(r,ui);
}
