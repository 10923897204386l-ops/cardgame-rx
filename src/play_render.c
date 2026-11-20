#include "play_render.h"
#include <stdio.h>
#include <string.h>

static void color(SDL_Renderer *r, SDL_Color c) { SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); }
static void fill(SDL_Renderer *r, Rect b) { SDL_Rect rect = {b.x,b.y,b.w,b.h}; SDL_RenderFillRect(r,&rect); }
static void outline(SDL_Renderer *r, Rect b, int expand, SDL_Color c) {
    render_round_outline(r,(Rect){b.x-expand,b.y-expand,b.w+2*expand,b.h+2*expand},8,1,c);
}
static void menu_text(SDL_Renderer *r, int center, int y, const char *label, int width, float zoom) {
    color(r,(SDL_Color){43,35,24,255});
    if (!render_menu_text(r,center,y,label,width,zoom)) render_text(r,center-width/4,y,label,2);
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
    if (!render_menu_text(r,320,y-4,label,260,1.0f)) render_text(r,200,y,label,2);
}
static void hud_text(SDL_Renderer *r, int center, int y, const char *label, int width, float zoom) {
    color(r,(SDL_Color){43,35,24,255});
    if (!render_menu_text(r,center,y,label,width,zoom)) render_text(r,center-width/2,y+1,label,1);
}
static void status_bar(SDL_Renderer *r, Rect b) {
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    render_round_fill(r,b,6,(SDL_Color){246,229,160,238});
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    render_round_outline(r,b,6,2,(SDL_Color){43,35,24,255});
}
static const char *key_name(const KeyboardMap *keys, InputAction action) {
    static KeyboardMap defaults;
    static bool initialized;
    if (!initialized) { defaults = keyboard_default(); initialized = true; }
    if (!keys) keys = &defaults;
    const char *name = SDL_GetScancodeName(keys->keys[action]);
    if (!name || !*name) return keyboard_action_name(action);
    if (!strcmp(name,"Left Ctrl")) return "LCtrl";
    if (!strcmp(name,"Right Ctrl")) return "RCtrl";
    if (!strcmp(name,"Left Shift")) return "LShift";
    if (!strcmp(name,"Right Shift")) return "RShift";
    if (!strcmp(name,"Left Alt")) return "LAlt";
    if (!strcmp(name,"Right Alt")) return "RAlt";
    if (!strcmp(name,"Space")) return "Space";
    if (!strcmp(name,"Return")) return "Enter";
    if (!strcmp(name,"Escape")) return "Esc";
    return name;
}
static bool dpad_default(const KeyboardMap *keys) {
    static KeyboardMap defaults;
    static bool initialized;
    if (!initialized) { defaults = keyboard_default(); initialized = true; }
    if (!keys) keys = &defaults;
    return keys->keys[INPUT_LEFT] == defaults.keys[INPUT_LEFT] &&
           keys->keys[INPUT_RIGHT] == defaults.keys[INPUT_RIGHT] &&
           keys->keys[INPUT_UP] == defaults.keys[INPUT_UP] &&
           keys->keys[INPUT_DOWN] == defaults.keys[INPUT_DOWN];
}
static void controls_bar(SDL_Renderer *r, const KeyboardMap *keys) {
    char label[96];
    status_bar(r,(Rect){6,450,628,26});
    hud_text(r,46,456,dpad_default(keys) ? "ARROWS" : "D-PAD",82,0.46f);
    snprintf(label,sizeof(label),"%s PICK",key_name(keys,INPUT_B));
    hud_text(r,124,456,label,78,0.42f);
    snprintf(label,sizeof(label),"%s CANCEL",key_name(keys,INPUT_A));
    hud_text(r,208,456,label,88,0.40f);
    snprintf(label,sizeof(label),"%s DRAW",key_name(keys,INPUT_Y));
    hud_text(r,302,456,label,82,0.40f);
    snprintf(label,sizeof(label),"%s ACE",key_name(keys,INPUT_X));
    hud_text(r,382,456,label,72,0.40f);
    snprintf(label,sizeof(label),"%s HELP",key_name(keys,INPUT_SELECT));
    hud_text(r,462,456,label,74,0.38f);
    snprintf(label,sizeof(label),"%s+%s UNDO",key_name(keys,INPUT_L2),key_name(keys,INPUT_SELECT));
    hud_text(r,560,456,label,132,0.38f);
}
static void menu_overlay(SDL_Renderer *r, const PlayUI *ui) {
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    color(r,(SDL_Color){0,0,0,ui->screen == MENU_MAIN ? 40 : 165}); fill(r,(Rect){0,0,640,480});
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    if (ui->screen == MENU_CONFIRM_NEW || ui->screen == MENU_CONFIRM_MAIN) {
        menu_panel(r,(Rect){115,145,410,170});
        color(r,(SDL_Color){43,35,24,255});
        menu_text(r,320,166,ui->screen == MENU_CONFIRM_MAIN ? "RETURN TO MAIN MENU" : "DISCARD AND DEAL NEW GAME",360,0.9f);
        menu_text(r,320,226,ui->confirm_yes ? "NO    -YES-" : "-NO-    YES",260,1.0f);
        render_text(r,190,278,"A SELECT   B BACK",1);
    } else if (ui->screen == MENU_OPTIONS) {
        menu_panel(r,(Rect){120,128,400,220});
        color(r,(SDL_Color){43,35,24,255});
        menu_text(r,320,150,"OPTIONS",260,1.0f);
        render_text(r,160,215,"No configurable options yet",1);
        render_text(r,188,296,"A OR B BACK",1);
    } else {
        menu_panel(r,ui->screen == MENU_MAIN ? (Rect){126,116,388,264} : (Rect){126,104,388,292});
        color(r,(SDL_Color){43,35,24,255});
        menu_text(r,320,132,ui->screen == MENU_MAIN ? "CROUPIER-RX" : "PAUSED",320,1.15f);
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
                 const Layout *l, const KeyboardMap *keys, BackTheme theme, uint64_t now) {
    if (ui->screen == MENU_MAIN) {
        if (!render_title_background(r)) render_table_background(r);
        menu_overlay(r,ui);
        return;
    }
    render_table_background(r);
    if (ui->controls_visible) controls_bar(r,keys);
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
    /* Fixed status strip, always on top of table/carry. */
    status_bar(r,(Rect){6,2,628,l->status_h-4});
    char label[96];
    KlondikePile ref = play_focus(ui);
    if (ref.zone == K_TABLEAU) snprintf(label,sizeof(label),"TABLEAU %u",ref.index+1);
    else if (ref.zone == K_FOUNDATION) snprintf(label,sizeof(label),"FOUNDATION %u",ref.index+1);
    else snprintf(label,sizeof(label),"DRAW %s",ref.zone == K_STOCK ? "STOCK" : "WASTE");
    hud_text(r,65,5,label,110,0.50f);
    snprintf(label,sizeof(label),"SEED %u  STOCK %zu",ui->seed,s->stock.count);
    hud_text(r,87,16,label,150,0.43f);
    if (ui->held) {
        size_t n = play_held_count(ui,s);
        snprintf(label,sizeof(label),"HOLD %zu  PLACE %zu",n,legal ? klondike_pile(s,ui->source)->count-chosen : 0);
        hud_text(r,230,5,label,150,0.43f);
    }
    hud_text(r,405,5,play_message(ui,now),160,0.48f);
    hud_text(r,552,16,"START PAUSE",120,0.40f);
    if (klondike_won(s)) hud_text(r,330,16,"YOU WIN - START NEW GAME",220,0.40f);
    if (ui->screen != MENU_PLAYING) menu_overlay(r,ui);
}
