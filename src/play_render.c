#include "play_render.h"
#include "fx.h"
#include <stdio.h>
#include <string.h>

static void color(SDL_Renderer *r, SDL_Color c) { SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); }
static void fill(SDL_Renderer *r, Rect b) { SDL_Rect rect = {b.x,b.y,b.w,b.h}; SDL_RenderFillRect(r,&rect); }
static void shadow(SDL_Renderer *r, Rect b, int alpha) {
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    render_round_fill(r,(Rect){b.x+3,b.y+4,b.w,b.h},8,(SDL_Color){0,0,0,alpha});
    render_round_fill(r,(Rect){b.x+1,b.y+2,b.w,b.h},8,(SDL_Color){0,0,0,alpha/2});
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
}
/* Soft recessed empty-slot treatment: layered translucent shade (cheap blur stand-in). */
static void empty_slot(SDL_Renderer *r, Rect b, const char *label) {
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    render_round_fill(r,b,10,(SDL_Color){0,18,8,58});
    render_round_fill(r,(Rect){b.x+2,b.y+2,b.w-4,b.h-4},9,(SDL_Color){0,12,6,72});
    render_round_fill(r,(Rect){b.x+5,b.y+5,b.w-10,b.h-10},8,(SDL_Color){4,28,14,48});
    render_round_fill(r,(Rect){b.x+8,b.y+8,b.w-16,b.h-16},7,(SDL_Color){12,42,24,36});
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    render_round_outline(r,b,10,1,(SDL_Color){86,132,108,255});
    render_round_outline(r,(Rect){b.x+1,b.y+1,b.w-2,b.h-2},9,1,(SDL_Color){150,190,160,255});
    color(r,(SDL_Color){214,232,214,255});
    if (label) render_text(r,b.x+8,b.y+44,label,1);
}
static void outline(SDL_Renderer *r, Rect b, int expand, SDL_Color c) {
    render_round_outline(r,(Rect){b.x-expand,b.y-expand,b.w+2*expand,b.h+2*expand},8,1,c);
}
static unsigned fw_hash(unsigned x) {
    x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x ? x : 0xA5A5u;
}
static void firework_burst(SDL_Renderer *r, int cx, int cy, unsigned seed, int age, int life) {
    if (age < 0 || age >= life) return;
    static const SDL_Color palette[] = {
        {255,214,92,255},{255,92,110,255},{110,214,255,255},
        {170,255,120,255},{255,160,70,255},{220,140,255,255},{255,255,240,255}
    };
    int sparks = 18 + (int)(seed % 7u);
    for (int i = 0; i < sparks; ++i) {
        unsigned h = fw_hash(seed + (unsigned)i * 97u);
        int angle = (int)(h % 628u); /* ~0..2pi * 100 */
        int speed = 18 + (int)((h >> 8) % 34u);
        /* Approximate cos/sin with cheap quadrant folds for deterministic layout. */
        int qx = (angle % 314) - 157; /* -157..156 */
        int qy = 157 - ((angle + 78) % 314);
        if (angle >= 314) { qx = -qx; qy = -qy; }
        int x = cx + (qx * speed * age) / (157 * life);
        int y = cy + (qy * speed * age) / (157 * life) + (age * age) / (life * 3);
        SDL_Color c = palette[(seed + (unsigned)i) % (unsigned)(sizeof palette / sizeof palette[0])];
        int fade = 255 - (age * 200) / life;
        if (fade < 40) fade = 40;
        c.a = (Uint8)fade;
        int size = 2 + ((int)(h >> 16) & 1);
        if (age * 3 > life * 2) size = 1;
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        render_round_fill(r,(Rect){x - size, y - size, size * 2 + 1, size * 2 + 1}, size, c);
        if (age < life / 3)
            render_round_fill(r,(Rect){x - 1, y - 1, 3, 3},1,(SDL_Color){255,255,255,(Uint8)(fade)});
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    }
}
static void victory_fireworks(SDL_Renderer *r, uint64_t now) {
    /* Looping multi-burst display; pure function of now for capture stability. */
    const int cycle = 2400;
    int t = (int)(now % (uint64_t)cycle);
    const struct { int x, y; unsigned seed; int start; int life; } bursts[] = {
        {120, 110, 0x11u, 0, 900},
        {320, 90, 0x29u, 180, 950},
        {520, 120, 0x3Bu, 360, 900},
        {200, 200, 0x47u, 540, 1000},
        {440, 180, 0x59u, 720, 1000},
        {300, 150, 0x61u, 980, 1100},
        {160, 250, 0x6Du, 1200, 950},
        {480, 240, 0x73u, 1400, 950},
        {360, 210, 0x85u, 1650, 1000},
        {240, 130, 0x97u, 1900, 900},
    };
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    color(r,(SDL_Color){0,0,0,55}); fill(r,(Rect){0,0,640,480});
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    for (unsigned i = 0; i < sizeof bursts / sizeof bursts[0]; ++i) {
        int age = t - bursts[i].start;
        if (age < 0) age += cycle;
        if (age >= bursts[i].life) continue;
        firework_burst(r, bursts[i].x, bursts[i].y, bursts[i].seed + (unsigned)(now / (uint64_t)cycle), age, bursts[i].life);
    }
    /* Rising launch trails for the next burst window. */
    for (unsigned i = 0; i < sizeof bursts / sizeof bursts[0]; ++i) {
        int lead = bursts[i].start - 120;
        if (lead < 0) lead += cycle;
        int age = t - lead;
        if (age < 0) age += cycle;
        if (age >= 120) continue;
        int y = 430 - (age * (430 - bursts[i].y)) / 120;
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        render_round_fill(r,(Rect){bursts[i].x - 1, y - 4, 3, 8},1,(SDL_Color){255,230,160,200});
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    }
}
static void menu_text(SDL_Renderer *r, int center, int y, const char *label, int width, float zoom) {
    color(r,(SDL_Color){43,35,24,255});
    if (!render_menu_text(r,center,y,label,width,zoom)) render_text(r,center-width/4,y,label,2);
}
static void top_pile_visible(SDL_Renderer *r, const Pile *p, Rect b, const PlayUI *ui,
                             KlondikePile ref, BackTheme theme) {
    const CardState *top = pile_top(p);
    if (ui->held && ui->source.zone == ref.zone && ui->source.index == ref.index) {
        top = ui->source_start ? &p->cards[ui->source_start - 1] : NULL;
    }
    if (top) {
        outline(r,b,0,(SDL_Color){119,163,137,255});
        shadow(r,b,46);
        render_card(r,b,top,theme);
    } else {
        empty_slot(r,b,ref.zone == K_STOCK ? "STOCK" : NULL);
    }
}
static void menu_panel(SDL_Renderer *r, Rect b) {
    render_round_fill(r,b,12,(SDL_Color){246,229,160,255});
    render_round_outline(r,b,12,3,(SDL_Color){43,35,24,255});
}
static void menu_item_at(SDL_Renderer *r, Rect panel, int y, const char *label, bool selected) {
    int pad = 10;
    Rect row = {panel.x + pad, y - 6, panel.w - pad * 2, 28};
    if (selected) {
        render_round_fill(r,row,7,(SDL_Color){255,205,78,255});
        render_round_outline(r,row,7,2,(SDL_Color){43,35,24,255});
    }
    color(r,(SDL_Color){43,35,24,255});
    if (!render_menu_text(r,panel.x + panel.w / 2,y - 3,label,panel.w - 24,0.92f))
        render_text(r,panel.x + 18,y,label,2);
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
    } else if (ui->screen == MENU_OPTIONS) {
        menu_panel(r,(Rect){170,150,300,160});
        color(r,(SDL_Color){43,35,24,255});
        menu_text(r,320,168,"OPTIONS",240,1.0f);
        menu_text(r,320,220,"No options yet",240,0.8f);
    } else if (ui->screen == MENU_MAIN) {
        /* Compact panel over title art: no duplicate title, no bitmap key hints. */
        Rect panel = {210, 188, 220, 168};
        menu_panel(r, panel);
        const char *main_items[] = {"CONTINUE","NEW GAME","OPTIONS","QUIT"};
        int y0 = panel.y + 22;
        for (unsigned i = 0; i < 4; ++i)
            menu_item_at(r, panel, y0 + (int)i * 36, main_items[i], ui->menu == i);
    } else {
        Rect panel = {150, 112, 340, 250};
        menu_panel(r, panel);
        color(r,(SDL_Color){43,35,24,255});
        menu_text(r,320,128,"PAUSED",280,1.05f);
        const char *pause_items[] = {"RESUME","NEW GAME","OPTIONS","MAIN MENU"};
        int y0 = 168;
        for (unsigned i = 0; i < 4; ++i)
            menu_item_at(r, panel, y0 + (int)i * 40, pause_items[i], ui->menu == i);
    }
}
void play_render(SDL_Renderer *r, const KlondikeState *s, const PlayUI *ui,
                 const Layout *l, const KeyboardMap *keys, BackTheme theme, uint64_t now) {
    play_render_fx(r,s,ui,l,keys,theme,now,NULL);
}
void play_render_fx(SDL_Renderer *r, const KlondikeState *s, const PlayUI *ui,
                    const Layout *l, const KeyboardMap *keys, BackTheme theme,
                    uint64_t now, PlayFX *fx) {
    if (ui->screen == MENU_MAIN) {
        if (!render_title_background(r)) render_table_background(r);
        menu_overlay(r,ui);
        if (fx) fx_render_over(r,fx,ui,l,s,theme,now);
        return;
    }
    render_table_background(r);
    if (fx) fx_render_under(r,fx,theme,now);
    if (ui->controls_visible) controls_bar(r,keys);
    top_pile_visible(r,&s->stock,layout_slot(l,0,false),ui,(KlondikePile){K_STOCK,0},theme);
    top_pile_visible(r,&s->waste,layout_slot(l,1,false),ui,(KlondikePile){K_WASTE,0},theme);
    for (unsigned f = 0; f < 4; ++f)
        top_pile_visible(r,&s->foundations[f],layout_slot(l,f+3,false),ui,(KlondikePile){K_FOUNDATION,f},theme);
    for (unsigned col = 0; col < 7; ++col) {
        const Pile *p = &s->tableau[col];
        size_t visible_count = p->count;
        if (ui->held && ui->source.zone == K_TABLEAU && ui->source.index == col)
            visible_count = ui->source_start;
        if (!visible_count) empty_slot(r,layout_slot(l,col,true),NULL);
        else outline(r,layout_slot(l,col,true),0,(SDL_Color){119,163,137,255});
        for (size_t i = 0; i < visible_count; ++i) {
            Rect b = play_card_rect(l,p,col,i);
            if (i + 1 == visible_count) shadow(r,b,40);
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
    Rect focus = play_focus_rect(l,s,ui);
    if (fx && fx_enabled(fx))
        fx_draw_selection_glow(r,(Rect){focus.x-2,focus.y-2,focus.w+4,focus.h+4},legal,now);
    else {
        SDL_Color focus_color = legal ? (SDL_Color){111,255,148,255} : (SDL_Color){255,219,90,255};
        render_round_outline(r,(Rect){focus.x-2,focus.y-2,focus.w+4,focus.h+4},10,4,focus_color);
    }
    if (ui->held) {
        Rect bounds = play_held_bounds(l,s,ui);
        const Pile *p = klondike_pile(s,ui->source);
        size_t count = play_held_count(ui,s);
        for (size_t i = 0; i < count; ++i) {
            Rect b = {bounds.x,bounds.y+(int)i*play_held_overlap(l,count),l->card_w,l->card_h};
            if (i + 1 == count) shadow(r,b,54);
            render_card(r,b,&p->cards[ui->source_start+i],theme);
        }
        SDL_Color held_color = play_invalid_outline(ui,now) ? (SDL_Color){255,42,54,255} : (SDL_Color){119,218,255,255};
        render_round_outline(r,bounds,10,4,held_color);
    }
    if (klondike_won(s) && ui->screen == MENU_PLAYING) {
        if (!(fx && fx_enabled(fx))) victory_fireworks(r,now);
        /* With FX enabled, restrained shimmer/cascade is drawn in fx_render_over. */
    }
    if (fx && ui->held) fx_note_held(fx, play_held_bounds(l,s,ui), now);
    /* Fixed status strip, always on top of table/carry. */
    status_bar(r,(Rect){4,2,632,l->status_h-2});
    char label[96];
    KlondikePile ref = play_focus(ui);
    if (ref.zone == K_TABLEAU) snprintf(label,sizeof(label),"TAB %u",ref.index+1);
    else if (ref.zone == K_FOUNDATION) snprintf(label,sizeof(label),"ACE %u",ref.index+1);
    else snprintf(label,sizeof(label),"%s",ref.zone == K_STOCK ? "STOCK" : "WASTE");
    /* Large single-line HUD: bake is 40px, zoom near 1.0 for handheld. */
    hud_text(r,78,6,label,130,1.05f);
    snprintf(label,sizeof(label),"SEED %u   STK %zu",ui->seed,s->stock.count);
    hud_text(r,250,6,label,220,0.95f);
    if (ui->held) {
        size_t n = play_held_count(ui,s);
        snprintf(label,sizeof(label),"HOLD %zu",n);
        hud_text(r,420,6,label,120,1.0f);
    }
    const char *msg = play_message(ui,now);
    if (msg && msg[0]) hud_text(r,540,6,msg,160,0.95f);
    if (klondike_won(s)) hud_text(r,320,l->status_h/2 - 4,"YOU WIN",200,1.1f);
    if (ui->screen != MENU_PLAYING) menu_overlay(r,ui);
    if (fx) {
        fx_set_victory(fx, klondike_won(s) && ui->screen == MENU_PLAYING, now);
        fx_render_over(r,fx,ui,l,s,theme,now);
    }
}
