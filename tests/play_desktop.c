#include "play_fixture.h"
#include "desktop.h"
#include "play_render.h"
#include <string.h>
static SDL_Surface *surface;
static SDL_Renderer *renderer;
static Layout layout;
static void draw(const KlondikeState *s, const PlayUI *ui, uint64_t now) {
    play_render(renderer,s,ui,&layout,NULL,back_theme(false),now); SDL_RenderPresent(renderer);
}
static void draw_keys(const KlondikeState *s, const PlayUI *ui, const KeyboardMap *keys, uint64_t now) {
    play_render(renderer,s,ui,&layout,keys,back_theme(false),now); SDL_RenderPresent(renderer);
}
static Uint32 pixel(int x,int y) { return ((Uint32 *)((unsigned char *)surface->pixels+y*surface->pitch))[x]; }
static bool pixel_equal(int ax,int ay,int bx,int by) { return pixel(ax,ay) == pixel(bx,by); }
static bool rgb(int x,int y,int red,int green,int blue) {
    Uint8 r,g,b; SDL_GetRGB(pixel(x,y),surface->format,&r,&g,&b);
    return r == red && g == green && b == blue;
}
static uint32_t pixel_sum(Rect r) {
    uint32_t sum = 0;
    for (int y = r.y; y < r.y + r.h; ++y)
        for (int x = r.x; x < r.x + r.w; ++x)
            sum = sum * 33u + pixel(x,y);
    return sum;
}
static PlayEvent key(PlayUI *ui,KlondikeGame *game,KeyboardMap *map,SDL_Scancode code,uint64_t now) {
    SDL_Event e; SDL_zero(e); e.type = SDL_KEYDOWN; e.key.keysym.scancode = code;
    return desktop_event(ui,game,map,&e,now);
}
static void start_play(PlayUI *ui, KlondikeGame *game, KeyboardMap *keys) {
    CHECK(ui->screen == MENU_MAIN);
    key(ui,game,keys,SDL_SCANCODE_D,1);
    CHECK(ui->screen == MENU_PLAYING);
}
static void ui_font_smoke(void) {
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderClear(renderer);
    CHECK(render_menu_text(renderer,320,180,"PERMANENT MARKER UI 0123456789",600,1.0f));
    CHECK(render_menu_text(renderer,320,220,"ARROWS S D A W Q LCTRL+Q F1 F2",600,0.9f));
    SDL_RenderPresent(renderer);
    CHECK(pixel_sum((Rect){60,170,520,90}) != 0);
}
static void mappings(void) {
    KeyboardMap keys = keyboard_default(); CHECK(keyboard_valid(&keys));
    const SDL_Scancode devkit[INPUT_COUNT] = {
        SDL_SCANCODE_LEFT,SDL_SCANCODE_RIGHT,SDL_SCANCODE_UP,SDL_SCANCODE_DOWN,
        SDL_SCANCODE_D,SDL_SCANCODE_S,SDL_SCANCODE_W,SDL_SCANCODE_A,
        SDL_SCANCODE_LSHIFT,SDL_SCANCODE_SPACE,SDL_SCANCODE_LCTRL,SDL_SCANCODE_LALT,
        SDL_SCANCODE_E,SDL_SCANCODE_Q,SDL_SCANCODE_ESCAPE
    };
    for (int a = 0; a < INPUT_COUNT; ++a) CHECK(keys.keys[a] == devkit[a]);
    SDL_Event physical; SDL_zero(physical); physical.type = SDL_KEYDOWN;
    physical.key.keysym.scancode = SDL_SCANCODE_W; physical.key.keysym.sym = SDLK_z;
    CHECK(keyboard_action(&keys,&physical) == INPUT_X); /* physical layout, like devkit */
    for (int a = 0; a < INPUT_COUNT; ++a) {
        SDL_Event e; SDL_zero(e); e.type = SDL_KEYDOWN; e.key.keysym.scancode = keys.keys[a];
        CHECK(keyboard_action(&keys,&e) == (InputAction)a);
        e.key.repeat = 1; CHECK(keyboard_action(&keys,&e) == INPUT_NONE);
        e.key.repeat = 0; e.type = SDL_KEYUP; CHECK(keyboard_action(&keys,&e) == INPUT_NONE);
    }
    CHECK(keyboard_bind(&keys,"b=F1")); CHECK(keyboard_valid(&keys));
    CHECK(keyboard_bind(&keys,"a=F1")); CHECK(!keyboard_valid(&keys));
    CHECK(keyboard_bind(&keys,"a=z")); CHECK(keyboard_valid(&keys));
    SDL_Event mapped; SDL_zero(mapped); mapped.type = SDL_KEYDOWN;
    mapped.key.keysym.scancode = SDL_SCANCODE_F1; CHECK(keyboard_action(&keys,&mapped) == INPUT_B);
    mapped.key.keysym.scancode = SDL_SCANCODE_Z; CHECK(keyboard_action(&keys,&mapped) == INPUT_A);
    CHECK(!keyboard_bind(&keys,"bogus=z")); CHECK(!keyboard_bind(&keys,"b="));
    CHECK(!keyboard_bind(&keys,"b=not-a-key")); CHECK(!keyboard_bind(&keys,"broken"));
    CHECK(!strcmp(keyboard_action_name(INPUT_START),"start"));
}
static void carry_and_flash(void) {
    KlondikeGame g = {0}; g.state = run_fixture(); PlayUI ui; play_init(&ui,&g.state,42);
    KeyboardMap keys = keyboard_default();
    draw(&g.state,&ui,0); CHECK(SDL_SaveBMP(surface,"play-main-menu.bmp") == 0);
    start_play(&ui,&g,&keys);
    draw(&g.state,&ui,50);
    /* Empty foundation/waste pads are softly shaded rather than bare outlines. */
    {
        Rect ace = layout_slot(&layout,3,false);
        Uint8 r0,g0,b0,r1,g1,b1;
        SDL_GetRGB(pixel(ace.x+ace.w/2,ace.y+ace.h/2),surface->format,&r0,&g0,&b0);
        SDL_GetRGB(pixel(2,200),surface->format,&r1,&g1,&b1);
        CHECK(!(r0 == r1 && g0 == g1 && b0 == b1));
        CHECK(g0 + 8 < g1 || r0 + 8 < r1); /* recessed shade is darker than open felt edge */
    }
    key(&ui,&g,&keys,SDL_SCANCODE_SPACE,100); key(&ui,&g,&keys,SDL_SCANCODE_S,110);
    CHECK(ui.held && play_held_count(&ui,&g.state) == 4);
    key(&ui,&g,&keys,SDL_SCANCODE_RIGHT,120); /* 9C wants only [8H,7C,6D] */
    size_t chosen; CHECK(play_preview(&ui,&g.state,&chosen) && chosen == 2);
    Rect b = play_held_bounds(&layout,&g.state,&ui); CHECK(b.h == 172); /* entire four-card candidate */
    KlondikeState before = g.state;
    draw(&g.state,&ui,120);
    CHECK(!ui.controls_visible);
    CHECK(SDL_SaveBMP(surface,"play-controls-hidden.bmp") == 0);
    CHECK(play_dispatch(&ui,&g,INPUT_SELECT,121) == PLAY_NONE && ui.controls_visible);
    draw(&g.state,&ui,121);
    CHECK(SDL_SaveBMP(surface,"play-controls-shown.bmp") == 0);
    uint32_t default_help_pixels = pixel_sum((Rect){500,454,130,18});
    KeyboardMap custom = keys;
    CHECK(keyboard_bind(&custom,"select=F2"));
    CHECK(keyboard_bind(&custom,"l2=F1"));
    CHECK(keyboard_valid(&custom));
    draw_keys(&g.state,&ui,&custom,121);
    CHECK(SDL_SaveBMP(surface,"play-controls-custom.bmp") == 0);
    CHECK(pixel_sum((Rect){500,454,130,18}) != default_help_pixels);
    CHECK(play_dispatch(&ui,&g,INPUT_SELECT,122) == PLAY_NONE && !ui.controls_visible);
    draw(&g.state,&ui,122);
    CHECK(pixel(b.x+10,b.y+10) != 0); /* held asset-backed card is rendered */
    Rect source_under = play_card_rect(&layout,&g.state.tableau[0],0,0);
    CHECK(!rgb(24,180,255,249,226)); /* held source card is not still drawn in place */
    CHECK(!pixel_equal(source_under.x+10,source_under.y+10,b.x+10,b.y+10)); /* underlying card remains distinct from held face */
    key(&ui,&g,&keys,SDL_SCANCODE_RIGHT,130); b = play_held_bounds(&layout,&g.state,&ui);
    CHECK(key(&ui,&g,&keys,SDL_SCANCODE_S,200) == PLAY_ERROR);
    draw(&g.state,&ui,200); CHECK(rgb(b.x+b.w/2,b.y-3,255,42,54));
    CHECK(rgb(b.x+b.w/2,b.y+b.h+2,255,42,54)); /* entire stack, not destination */
    draw(&g.state,&ui,499); CHECK(rgb(b.x+b.w/2,b.y-3,255,42,54));
    draw(&g.state,&ui,500); CHECK(rgb(b.x+b.w/2,b.y-3,119,218,255));
    key(&ui,&g,&keys,SDL_SCANCODE_S,510); draw(&g.state,&ui,510); CHECK(rgb(b.x+b.w/2,b.y-3,255,42,54));
    CHECK(same_state(&g.state,&before) && !g.history && play_held_count(&ui,&g.state) == 4);
    CHECK(pixel(639,479) != 0); /* no unrendered bottom strip */
    CHECK(SDL_SaveBMP(surface,"play-invalid.bmp") == 0);
    key(&ui,&g,&keys,SDL_SCANCODE_LEFT,820); key(&ui,&g,&keys,SDL_SCANCODE_S,830);
    CHECK(!ui.held && g.undo_depth == 1 && g.state.tableau[0].count == 2);
    CHECK(play_dispatch_chord(&ui,&g,INPUT_SELECT,true,840) == PLAY_COMMITTED);
    CHECK(same_state(&g.state,&before));
    key(&ui,&g,&keys,SDL_SCANCODE_E,850); draw(&g.state,&ui,850);
    CHECK(SDL_SaveBMP(surface,"play-paused.bmp") == 0);
    key(&ui,&g,&keys,SDL_SCANCODE_DOWN,860); key(&ui,&g,&keys,SDL_SCANCODE_D,870);
    CHECK(ui.screen == MENU_CONFIRM_NEW && !ui.confirm_yes && same_state(&g.state,&before));
    draw(&g.state,&ui,870); CHECK(SDL_SaveBMP(surface,"play-confirm.bmp") == 0);
    klondike_dispose(&g);
}
static void tall_columns(void) {
    KlondikeState s = {0};
    /* A maximal 13-card face-up run; all other identities are hidden below it.
       This is taller than any initial column and stresses hidden compression. */
    bool used[4][14] = {{false}};
    for (unsigned rank = 13; rank; --rank) used[rank%2 ? SPADES : HEARTS][rank] = true;
    for (Suit suit = CLUBS; suit <= SPADES; ++suit)
        for (unsigned rank = 1; rank <= 13; ++rank) if (!used[suit][rank]) add(&s.tableau[0],suit,rank,false);
    for (unsigned rank = 13; rank; --rank) add(&s.tableau[0],rank%2 ? SPADES : HEARTS,rank,true);
    CHECK(klondike_valid(&s));
    PlayUI ui; play_init(&ui,&s,42); ui.screen = MENU_PLAYING; ui.paused = false; ui.area = AREA_TABLEAU;
    size_t first = play_first_face_up(&s.tableau[0]); CHECK(first == 39);
    for (size_t i = first; i < 52; ++i) {
        Rect card = play_card_rect(&layout,&s.tableau[0],0,i);
        CHECK(card.y+22 <= 480 && card.y >= layout.tableau_y);
        if (i+1 < 52) {
            Rect next = play_card_rect(&layout,&s.tableau[0],0,i+1); CHECK(next.y-card.y >= 22);
        }
        ui.selected = i; Rect focus = play_focus_rect(&layout,&s,&ui);
        CHECK(focus.y == card.y && focus.h >= 18 && focus.y+focus.h <= 480);
    }
    draw(&s,&ui,100); CHECK(SDL_SaveBMP(surface,"play-tall.bmp") == 0);
    /* With 13 cards held the full overlay still fits, with readable indexes. */
    ui.held = true; ui.source = (KlondikePile){K_TABLEAU,0}; ui.source_start = first;
    Rect b = play_held_bounds(&layout,&s,&ui);
    CHECK(play_held_count(&ui,&s) == 13 && play_held_overlap(&layout,13) >= 22);
    CHECK(b.y-4 >= layout.status_h && b.y+b.h+4 < 480);
    draw(&s,&ui,100); CHECK(SDL_SaveBMP(surface,"play-tall-held.bmp") == 0);
    Layout large_spacing = layout; large_spacing.status_h = 40; large_spacing.face_up_overlap = 100;
    CHECK(layout_valid(&large_spacing));
    b = play_held_bounds(&large_spacing,&s,&ui);
    CHECK(b.y-4 >= large_spacing.status_h && b.y+b.h+4 < 480);
    CHECK(play_held_overlap(&large_spacing,13) >= 22);
}
static void sdl_key_playthrough(void) {
    KlondikeGame g = {0}; klondike_new_game(&g,42); PlayUI ui; play_init(&ui,&g.state,42);
    KeyboardMap keys = keyboard_default(); KlondikeState initial = g.state;
    start_play(&ui,&g,&keys);
    /* Same SDL key boundary as the app: complete stock pass/recycle/undo. */
    for (int i = 0; i < 25; ++i) CHECK(key(&ui,&g,&keys,SDL_SCANCODE_A,(uint64_t)i*10) == PLAY_COMMITTED);
    CHECK(same_state(&g.state,&initial) && g.undo_depth == 25);
    CHECK(play_dispatch_chord(&ui,&g,INPUT_SELECT,true,300) == PLAY_COMMITTED);
    CHECK(g.state.stock.count == 0);
    key(&ui,&g,&keys,SDL_SCANCODE_A,310); key(&ui,&g,&keys,SDL_SCANCODE_SPACE,320);
    /* Seed 42: red 4 from tableau 6 fits 5S in tableau 0. */
    for (int i = 0; i < 6; ++i) key(&ui,&g,&keys,SDL_SCANCODE_RIGHT,330);
    CHECK(ui.focus[1] == 6); key(&ui,&g,&keys,SDL_SCANCODE_S,340);
    for (int i = 0; i < 6; ++i) key(&ui,&g,&keys,SDL_SCANCODE_LEFT,350);
    CHECK(key(&ui,&g,&keys,SDL_SCANCODE_S,360) == PLAY_COMMITTED);
    CHECK(g.state.tableau[0].count == 2 && g.state.tableau[6].cards[5].face_up);
    CHECK(play_dispatch_chord(&ui,&g,INPUT_SELECT,true,370) == PLAY_COMMITTED);
    CHECK(g.state.tableau[0].count == 1);
    key(&ui,&g,&keys,SDL_SCANCODE_S,380); key(&ui,&g,&keys,SDL_SCANCODE_RIGHT,390);
    CHECK(key(&ui,&g,&keys,SDL_SCANCODE_S,400) == PLAY_ERROR && ui.held);
    key(&ui,&g,&keys,SDL_SCANCODE_D,410); CHECK(!ui.held && ui.focus[1] == 0);
    key(&ui,&g,&keys,SDL_SCANCODE_E,420); key(&ui,&g,&keys,SDL_SCANCODE_DOWN,430); key(&ui,&g,&keys,SDL_SCANCODE_D,440);
    CHECK(ui.screen == MENU_CONFIRM_NEW); key(&ui,&g,&keys,SDL_SCANCODE_RIGHT,450);
    CHECK(key(&ui,&g,&keys,SDL_SCANCODE_D,460) == PLAY_NEW_GAME);
    CHECK(ui.seed == 43 && !g.undo_depth); CHECK(klondike_valid(&g.state));
    SDL_Event repeat; SDL_zero(repeat); repeat.type = SDL_KEYDOWN; repeat.key.keysym.scancode = SDL_SCANCODE_A; repeat.key.repeat = 1;
    KlondikeState before = g.state; desktop_event(&ui,&g,&keys,&repeat,470); CHECK(same_state(&g.state,&before));
    CHECK(key(&ui,&g,&keys,SDL_SCANCODE_ESCAPE,480) == PLAY_QUIT);
    CHECK(same_state(&g.state,&before));
    klondike_dispose(&g);
}
static void winning(void) {
    KlondikeGame g = {0};
    for (Suit suit = CLUBS; suit <= SPADES; ++suit)
        for (unsigned rank = 1; rank <= 13; ++rank) add(&g.state.foundations[suit],suit,rank,true);
    CHECK(klondike_won(&g.state)); PlayUI ui; play_init(&ui,&g.state,42);
    ui.screen = MENU_PLAYING; ui.paused = false;
    draw(&g.state,&ui,0); CHECK(SDL_SaveBMP(surface,"play-win.bmp") == 0);
    /* Empty tableau slots use soft shaded pads; fireworks paint bright sparks. */
    Rect empty = layout_slot(&layout,0,true);
    Uint8 er,eg,eb; SDL_GetRGB(pixel(empty.x+empty.w/2,empty.y+empty.h/2),surface->format,&er,&eg,&eb);
    CHECK(eg > er && eg > eb); /* still green-tinted felt shade */
    bool spark = false;
    for (int y = 70; y < 260 && !spark; ++y)
        for (int x = 80; x < 560; ++x) {
            Uint8 r,g,b; SDL_GetRGB(pixel(x,y),surface->format,&r,&g,&b);
            if (r > 200 && g > 150 && b < 160) { spark = true; break; }
        }
    CHECK(spark);
    KlondikeState before = g.state;
    CHECK(play_dispatch(&ui,&g,INPUT_Y,0) == PLAY_ERROR && same_state(&g.state,&before));
    play_dispatch(&ui,&g,INPUT_START,10); play_dispatch(&ui,&g,INPUT_DOWN,20);
    play_dispatch(&ui,&g,INPUT_A,30); play_dispatch(&ui,&g,INPUT_RIGHT,40);
    CHECK(play_dispatch(&ui,&g,INPUT_A,50) == PLAY_NEW_GAME);
    CHECK(!klondike_won(&g.state) && klondike_valid(&g.state) && ui.seed == 43);
    klondike_dispose(&g);
}
int main(void) {
    surface = SDL_CreateRGBSurfaceWithFormat(0,640,480,32,SDL_PIXELFORMAT_ARGB8888);
    CHECK(surface); renderer = SDL_CreateSoftwareRenderer(surface); CHECK(renderer); layout = layout_default();
    CHECK(render_assets_open(renderer,CARDGAME_ASSET_DIR));
#ifdef CARDGAME_FONT_PATH
    CHECK(!render_menu_font_open(renderer,"/tmp/cardgame-rx-missing-font.ttf"));
    CHECK(render_menu_font_open(renderer,CARDGAME_FONT_PATH));
#endif
    ui_font_smoke(); mappings(); carry_and_flash(); tall_columns(); sdl_key_playthrough(); winning();
    render_assets_close();
    SDL_DestroyRenderer(renderer); SDL_FreeSurface(surface);
    puts("desktop: configurable keys, full held overlay without source ghost, invalid outline pixels, tall indexes and SDL keyboard playthrough passed");
    return 0;
}
