#include "play_fixture.h"
#include "desktop.h"
#include "play_render.h"
#include <string.h>
static SDL_Surface *surface;
static SDL_Renderer *renderer;
static Layout layout;
static void draw(const KlondikeState *s, const PlayUI *ui, uint64_t now) {
    play_render(renderer,s,ui,&layout,back_theme(false),now); SDL_RenderPresent(renderer);
}
static Uint32 pixel(int x,int y) { return ((Uint32 *)((unsigned char *)surface->pixels+y*surface->pitch))[x]; }
static bool rgb(int x,int y,int red,int green,int blue) {
    Uint8 r,g,b; SDL_GetRGB(pixel(x,y),surface->format,&r,&g,&b);
    return r == red && g == green && b == blue;
}
static PlayEvent key(PlayUI *ui,KlondikeGame *game,KeyboardMap *map,SDL_Keycode code,uint64_t now) {
    SDL_Event e; SDL_zero(e); e.type = SDL_KEYDOWN; e.key.keysym.sym = code;
    return desktop_event(ui,game,map,&e,now);
}
static void mappings(void) {
    KeyboardMap keys = keyboard_default(); CHECK(keyboard_valid(&keys));
    for (int a = 0; a < INPUT_COUNT; ++a) {
        SDL_Event e; SDL_zero(e); e.type = SDL_KEYDOWN; e.key.keysym.sym = keys.keys[a];
        CHECK(keyboard_action(&keys,&e) == (InputAction)a);
        e.key.repeat = 1; CHECK(keyboard_action(&keys,&e) == INPUT_NONE);
        e.key.repeat = 0; e.type = SDL_KEYUP; CHECK(keyboard_action(&keys,&e) == INPUT_NONE);
    }
    CHECK(keyboard_bind(&keys,"b=Space")); CHECK(keyboard_valid(&keys));
    CHECK(keyboard_bind(&keys,"a=Space")); CHECK(!keyboard_valid(&keys));
    CHECK(keyboard_bind(&keys,"a=z")); CHECK(keyboard_valid(&keys));
    CHECK(!keyboard_bind(&keys,"bogus=z")); CHECK(!keyboard_bind(&keys,"b="));
    CHECK(!keyboard_bind(&keys,"b=not-a-key")); CHECK(!keyboard_bind(&keys,"broken"));
    CHECK(!strcmp(keyboard_action_name(INPUT_START),"start"));
}
static void carry_and_flash(void) {
    KlondikeGame g = {0}; g.state = run_fixture(); PlayUI ui; play_init(&ui,&g.state,42);
    KeyboardMap keys = keyboard_default();
    key(&ui,&g,&keys,SDLK_e,100); key(&ui,&g,&keys,SDLK_z,110);
    CHECK(ui.held && play_held_count(&ui,&g.state) == 4);
    key(&ui,&g,&keys,SDLK_RIGHT,120); /* 9C wants only [8H,7C,6D] */
    size_t chosen; CHECK(play_preview(&ui,&g.state,&chosen) && chosen == 2);
    Rect b = play_held_bounds(&layout,&g.state,&ui); CHECK(b.h == 172); /* entire four-card candidate */
    KlondikeState before = g.state;
    draw(&g.state,&ui,120);
    SDL_Surface *expected = SDL_CreateRGBSurfaceWithFormat(0,640,480,32,SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *er = SDL_CreateSoftwareRenderer(expected); CHECK(expected && er);
    for (size_t i = 0; i < 4; ++i) {
        Rect card = {b.x,b.y+(int)i*24,72,100};
        render_card(er,card,&g.state.tableau[0].cards[1+i],back_theme(false));
    }
    SDL_RenderPresent(er);
    /* Every held index matches an independent full-candidate rendering. */
    for (size_t i = 0; i < 4; ++i)
        for (int y = 4; y < 18; ++y)
            for (int x = 4; x < 38; ++x) {
                int yy = b.y+(int)i*24+y;
                CHECK(pixel(b.x+x,yy) == ((Uint32 *)((unsigned char *)expected->pixels+yy*expected->pitch))[b.x+x]);
            }
    CHECK(!rgb(24,180,255,249,226)); /* source candidate visibly ghosted */
    SDL_DestroyRenderer(er); SDL_FreeSurface(expected);
    key(&ui,&g,&keys,SDLK_RIGHT,130); b = play_held_bounds(&layout,&g.state,&ui);
    CHECK(key(&ui,&g,&keys,SDLK_z,200) == PLAY_ERROR);
    draw(&g.state,&ui,200); CHECK(rgb(b.x-4,b.y-4,255,42,54));
    CHECK(rgb(b.x+b.w+3,b.y+b.h+3,255,42,54)); /* entire stack, not destination */
    draw(&g.state,&ui,499); CHECK(rgb(b.x-4,b.y-4,255,42,54));
    draw(&g.state,&ui,500); CHECK(rgb(b.x-4,b.y-4,119,218,255));
    key(&ui,&g,&keys,SDLK_z,510); draw(&g.state,&ui,510); CHECK(rgb(b.x-4,b.y-4,255,42,54));
    CHECK(same_state(&g.state,&before) && !g.history && play_held_count(&ui,&g.state) == 4);
    CHECK(rgb(639,479,22,83,64)); /* no bottom bar */
    CHECK(SDL_SaveBMP(surface,"play-invalid.bmp") == 0);
    key(&ui,&g,&keys,SDLK_LEFT,820); key(&ui,&g,&keys,SDLK_z,830);
    CHECK(!ui.held && g.undo_depth == 1 && g.state.tableau[0].count == 2);
    key(&ui,&g,&keys,SDLK_BACKSPACE,840); CHECK(same_state(&g.state,&before));
    key(&ui,&g,&keys,SDLK_RETURN,850); draw(&g.state,&ui,850);
    CHECK(SDL_SaveBMP(surface,"play-paused.bmp") == 0);
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
    PlayUI ui; play_init(&ui,&s,42); ui.area = AREA_TABLEAU;
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
}
static void sdl_key_playthrough(void) {
    KlondikeGame g = {0}; klondike_new_game(&g,42); PlayUI ui; play_init(&ui,&g.state,42);
    KeyboardMap keys = keyboard_default(); KlondikeState initial = g.state;
    /* Same SDL key boundary as the app: complete stock pass/recycle/undo. */
    for (int i = 0; i < 25; ++i) CHECK(key(&ui,&g,&keys,SDLK_s,(uint64_t)i*10) == PLAY_COMMITTED);
    CHECK(same_state(&g.state,&initial) && g.undo_depth == 25);
    key(&ui,&g,&keys,SDLK_BACKSPACE,300); CHECK(g.state.stock.count == 0);
    key(&ui,&g,&keys,SDLK_s,310); key(&ui,&g,&keys,SDLK_e,320);
    /* Seed 42: 4D from tableau 6 fits 5S in tableau 0. */
    for (int i = 0; i < 6; ++i) key(&ui,&g,&keys,SDLK_RIGHT,330);
    CHECK(ui.focus[1] == 6); key(&ui,&g,&keys,SDLK_z,340);
    for (int i = 0; i < 6; ++i) key(&ui,&g,&keys,SDLK_LEFT,350);
    CHECK(key(&ui,&g,&keys,SDLK_z,360) == PLAY_COMMITTED);
    CHECK(g.state.tableau[0].count == 2 && g.state.tableau[6].cards[5].face_up);
    key(&ui,&g,&keys,SDLK_BACKSPACE,370); CHECK(g.state.tableau[0].count == 1);
    key(&ui,&g,&keys,SDLK_z,380); key(&ui,&g,&keys,SDLK_RIGHT,390);
    CHECK(key(&ui,&g,&keys,SDLK_z,400) == PLAY_ERROR && ui.held);
    key(&ui,&g,&keys,SDLK_x,410); CHECK(!ui.held && ui.focus[1] == 0);
    key(&ui,&g,&keys,SDLK_RETURN,420); key(&ui,&g,&keys,SDLK_DOWN,430); key(&ui,&g,&keys,SDLK_z,440);
    CHECK(ui.confirming); key(&ui,&g,&keys,SDLK_RIGHT,450);
    CHECK(key(&ui,&g,&keys,SDLK_z,460) == PLAY_NEW_GAME);
    CHECK(ui.seed == 43 && !g.undo_depth); CHECK(klondike_valid(&g.state));
    SDL_Event repeat; SDL_zero(repeat); repeat.type = SDL_KEYDOWN; repeat.key.keysym.sym = SDLK_s; repeat.key.repeat = 1;
    KlondikeState before = g.state; desktop_event(&ui,&g,&keys,&repeat,470); CHECK(same_state(&g.state,&before));
    SDL_Event close; SDL_zero(close); close.type = SDL_QUIT; CHECK(desktop_event(&ui,&g,&keys,&close,480) == PLAY_QUIT);
    klondike_dispose(&g);
}
int main(void) {
    surface = SDL_CreateRGBSurfaceWithFormat(0,640,480,32,SDL_PIXELFORMAT_ARGB8888);
    CHECK(surface); renderer = SDL_CreateSoftwareRenderer(surface); CHECK(renderer); layout = layout_default();
    mappings(); carry_and_flash(); tall_columns(); sdl_key_playthrough();
    SDL_DestroyRenderer(renderer); SDL_FreeSurface(surface);
    puts("desktop: configurable keys, full held overlay/ghost, invalid outline pixels, tall indexes and SDL keyboard playthrough passed");
    return 0;
}
