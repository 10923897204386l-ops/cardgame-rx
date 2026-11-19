#include "play_fixture.h"
static PlayEvent press(PlayUI *ui, KlondikeGame *g, InputAction action) {
    return play_dispatch(ui,g,action,1000);
}
static void navigation(void) {
    KlondikeGame g = {0}; g.state = run_fixture(); PlayUI ui; play_init(&ui,&g.state,42);
    KlondikeState initial = g.state;
    CHECK(ui.area == AREA_DRAW && ui.focus[0] == 0 && ui.selected == 1);
    press(&ui,&g,INPUT_LEFT); CHECK(ui.area == AREA_FOUNDATIONS && ui.focus[2] == 3);
    press(&ui,&g,INPUT_RIGHT); CHECK(ui.area == AREA_DRAW && ui.focus[0] == 0);
    press(&ui,&g,INPUT_R2); CHECK(ui.focus[0] == 1);
    press(&ui,&g,INPUT_RIGHT); CHECK(ui.area == AREA_FOUNDATIONS && ui.focus[2] == 0);
    press(&ui,&g,INPUT_LEFT); CHECK(ui.area == AREA_DRAW && ui.focus[0] == 1);
    press(&ui,&g,INPUT_DOWN); CHECK(ui.area == AREA_TABLEAU && ui.focus[1] == 1 && ui.selected == 0);
    press(&ui,&g,INPUT_LEFT); CHECK(ui.area == AREA_TABLEAU && ui.focus[1] == 0);
    CHECK(ui.selected == 1); press(&ui,&g,INPUT_UP); CHECK(ui.selected == 2);
    press(&ui,&g,INPUT_DOWN); press(&ui,&g,INPUT_DOWN); CHECK(ui.selected == 1);
    for (int i = 0; i < 10; ++i) press(&ui,&g,INPUT_UP);
    CHECK(ui.area == AREA_DRAW && ui.focus[0] == 0);
    press(&ui,&g,INPUT_DOWN); CHECK(ui.area == AREA_TABLEAU && ui.focus[1] == 0);
    press(&ui,&g,INPUT_R2); CHECK(ui.focus[1] == 6 && ui.selected == 0);
    press(&ui,&g,INPUT_UP); CHECK(ui.area == AREA_FOUNDATIONS && ui.focus[2] == 3);
    press(&ui,&g,INPUT_R1); CHECK(ui.area == AREA_DRAW);
    press(&ui,&g,INPUT_R2); CHECK(ui.area == AREA_DRAW && ui.focus[0] == 1);
    press(&ui,&g,INPUT_R1); CHECK(ui.area == AREA_TABLEAU && ui.focus[1] == 6);
    press(&ui,&g,INPUT_L1); CHECK(ui.area == AREA_DRAW && ui.focus[0] == 1);
    press(&ui,&g,INPUT_L1); CHECK(ui.area == AREA_FOUNDATIONS && ui.focus[2] == 3);
    press(&ui,&g,INPUT_L1); CHECK(ui.area == AREA_TABLEAU && ui.focus[1] == 6);
    press(&ui,&g,INPUT_L2); CHECK(ui.focus[1] == 0 && ui.selected == 1);
    CHECK(same_state(&g.state,&initial) && !g.history);
    klondike_dispose(&g);
}
static void carrying(void) {
    KlondikeGame g = {0}; g.state = run_fixture(); PlayUI ui; play_init(&ui,&g.state,42);
    KlondikeState before = g.state;
    press(&ui,&g,INPUT_R1); CHECK(press(&ui,&g,INPUT_B) == PLAY_NONE);
    CHECK(ui.held && ui.source_start == 1 && play_held_count(&ui,&g.state) == 4);
    press(&ui,&g,INPUT_R1); press(&ui,&g,INPUT_R2); press(&ui,&g,INPUT_L1);
    press(&ui,&g,INPUT_R2); press(&ui,&g,INPUT_L2);
    press(&ui,&g,INPUT_UP); CHECK(ui.selected == 2 && ui.source_start == 1);
    press(&ui,&g,INPUT_RIGHT); press(&ui,&g,INPUT_RIGHT); CHECK(ui.focus[1] == 2);
    CHECK(press(&ui,&g,INPUT_X) == PLAY_ERROR); CHECK(press(&ui,&g,INPUT_Y) == PLAY_ERROR);
    CHECK(same_state(&g.state,&before) && !g.undo_depth);
    unsigned cursor = ui.focus[1]; size_t selected = ui.selected;
    CHECK(play_dispatch(&ui,&g,INPUT_B,2000) == PLAY_ERROR);
    CHECK(ui.invalid_until == 2300 && play_invalid_outline(&ui,2000));
    CHECK(play_invalid_outline(&ui,2299) && !play_invalid_outline(&ui,2300));
    CHECK(play_dispatch(&ui,&g,INPUT_B,2200) == PLAY_ERROR);
    CHECK(play_invalid_outline(&ui,2499) && !play_invalid_outline(&ui,2500));
    CHECK(ui.focus[1] == cursor && ui.selected == selected && ui.source_start == 1);
    CHECK(play_held_count(&ui,&g.state) == 4 && same_state(&g.state,&before) && !g.history);
    press(&ui,&g,INPUT_LEFT); size_t chosen = 99;
    CHECK(play_preview(&ui,&g.state,&chosen) && chosen == 2);
    CHECK(press(&ui,&g,INPUT_B) == PLAY_COMMITTED);
    CHECK(!ui.held && g.undo_depth == 1 && g.state.tableau[0].count == 2 && g.state.tableau[1].count == 4);
    CHECK(g.state.tableau[0].cards[1].card.rank == 9 && !g.state.tableau[0].cards[0].face_up);
    CHECK(press(&ui,&g,INPUT_SELECT) == PLAY_COMMITTED); CHECK(same_state(&g.state,&before));
    CHECK(ui.selected == 0 && ui.focus[1] == 1);
    press(&ui,&g,INPUT_LEFT); press(&ui,&g,INPUT_B);
    CHECK(press(&ui,&g,INPUT_B) == PLAY_CANCELLED && !ui.held); /* source cancels */
    press(&ui,&g,INPUT_B); press(&ui,&g,INPUT_RIGHT);
    CHECK(press(&ui,&g,INPUT_A) == PLAY_CANCELLED && ui.focus[1] == 0 && ui.selected == 1);
    press(&ui,&g,INPUT_B); press(&ui,&g,INPUT_R1);
    CHECK(press(&ui,&g,INPUT_SELECT) == PLAY_CANCELLED && !g.undo_depth && ui.area == AREA_TABLEAU);
    CHECK(press(&ui,&g,INPUT_SELECT) == PLAY_ERROR); CHECK(same_state(&g.state,&before));
    klondike_dispose(&g);
}
static void waste_and_foundations(void) {
    for (PlayArea area = AREA_DRAW; area < AREA_COUNT; ++area) {
        KlondikeGame g = {0}; g.state = run_fixture(); PlayUI ui; play_init(&ui,&g.state,42);
        ui.area = area; ui.focus[area] = 1;
        KlondikePile cursor = play_focus(&ui); KlondikeState before = g.state;
        press(&ui,&g,INPUT_A);
        CHECK(ui.held && ui.source.zone == K_WASTE && play_held_count(&ui,&g.state) == 1);
        CHECK(play_focus(&ui).zone == cursor.zone && play_focus(&ui).index == cursor.index);
        CHECK(same_state(&g.state,&before) && !g.history);
        press(&ui,&g,INPUT_A); CHECK(!ui.held && ui.area == AREA_DRAW && ui.focus[0] == 1);
        CHECK(press(&ui,&g,INPUT_X) == PLAY_COMMITTED);
        CHECK(g.state.foundations[0].count == 1 && g.state.foundations[0].cards[0].card.suit == HEARTS);
        CHECK(ui.focus[0] == 1 && !g.state.waste.count);
        CHECK(press(&ui,&g,INPUT_A) == PLAY_ERROR && !ui.held);
        press(&ui,&g,INPUT_SELECT); CHECK(same_state(&g.state,&before));
        klondike_dispose(&g);
    }
    KlondikeGame g = {0};
    add(&g.state.foundations[0],CLUBS,1,true);
    add(&g.state.tableau[0],SPADES,2,true); add(&g.state.tableau[0],HEARTS,1,true);
    complete(&g.state); PlayUI ui; play_init(&ui,&g.state,42);
    press(&ui,&g,INPUT_R1); KlondikeState before = g.state;
    CHECK(press(&ui,&g,INPUT_X) == PLAY_ERROR); /* selected run, not hidden auto-top */
    CHECK(same_state(&g.state,&before)); press(&ui,&g,INPUT_UP);
    CHECK(press(&ui,&g,INPUT_X) == PLAY_COMMITTED && g.state.foundations[1].count == 1);
    CHECK(!g.state.foundations[2].count && !g.state.foundations[3].count);
    press(&ui,&g,INPUT_R1); CHECK(press(&ui,&g,INPUT_X) == PLAY_ERROR);
    press(&ui,&g,INPUT_B); CHECK(ui.held && ui.source.zone == K_FOUNDATION);
    press(&ui,&g,INPUT_A); klondike_dispose(&g);
}
static void stock_and_pause(void) {
    KlondikeGame g = {0}; klondike_new_game(&g,42); PlayUI ui; play_init(&ui,&g.state,42);
    CHECK(press(&ui,&g,INPUT_A) == PLAY_ERROR && !ui.held);
    CHECK(press(&ui,&g,INPUT_B) == PLAY_ERROR && !ui.held);
    KlondikeState before = g.state;
    for (int i = 0; i < 24; ++i) CHECK(press(&ui,&g,INPUT_Y) == PLAY_COMMITTED);
    CHECK(!g.state.stock.count && g.undo_depth == 24);
    KlondikeState empty_stock = g.state;
    CHECK(press(&ui,&g,INPUT_Y) == PLAY_COMMITTED && g.undo_depth == 25);
    CHECK(same_state(&g.state,&before));
    press(&ui,&g,INPUT_SELECT); CHECK(same_state(&g.state,&empty_stock));
    press(&ui,&g,INPUT_A); CHECK(ui.held);
    struct KlondikeUndo *history = g.history; size_t depth = g.undo_depth;
    CHECK(press(&ui,&g,INPUT_SELECT) == PLAY_CANCELLED && g.undo_depth == depth);
    press(&ui,&g,INPUT_A); press(&ui,&g,INPUT_START); CHECK(ui.paused && ui.held);
    const InputAction gated[] = {INPUT_X,INPUT_Y,INPUT_SELECT,INPUT_R1,INPUT_L1,INPUT_R2,INPUT_L2};
    for (size_t i = 0; i < sizeof(gated)/sizeof(*gated); ++i) press(&ui,&g,gated[i]);
    CHECK(same_state(&g.state,&empty_stock) && g.history == history && g.undo_depth == depth && ui.held);
    press(&ui,&g,INPUT_A); CHECK(!ui.paused && ui.held); /* Resume */
    press(&ui,&g,INPUT_START); press(&ui,&g,INPUT_DOWN); press(&ui,&g,INPUT_A);
    CHECK(ui.confirming && !ui.confirm_yes);
    press(&ui,&g,INPUT_A); CHECK(ui.paused && !ui.confirming && ui.held); /* No */
    CHECK(same_state(&g.state,&empty_stock) && g.history == history);
    press(&ui,&g,INPUT_A); press(&ui,&g,INPUT_RIGHT); press(&ui,&g,INPUT_B);
    CHECK(!ui.confirming && ui.paused && same_state(&g.state,&empty_stock));
    press(&ui,&g,INPUT_A); press(&ui,&g,INPUT_RIGHT);
    CHECK(press(&ui,&g,INPUT_A) == PLAY_NEW_GAME);
    KlondikeState expected; klondike_deal(&expected,43);
    CHECK(same_state(&g.state,&expected) && !ui.paused && !ui.held && !g.history && !g.undo_depth);
    CHECK(ui.area == AREA_DRAW && ui.focus[0] == 0 && ui.seed == 43);
    press(&ui,&g,INPUT_START); press(&ui,&g,INPUT_START); CHECK(!ui.paused);
    press(&ui,&g,INPUT_START); press(&ui,&g,INPUT_UP);
    CHECK(press(&ui,&g,INPUT_A) == PLAY_QUIT && ui.quit);
    CHECK(same_state(&g.state,&expected));
    CHECK(press(&ui,&g,INPUT_Y) == PLAY_NONE && same_state(&g.state,&expected));
    klondike_dispose(&g);
}
int main(void) {
    navigation(); carrying(); waste_and_foundations(); stock_and_pause();
    puts("play: focus, transient carry, atomic suffix/drop, 300ms invalid feedback, actions, pause and restart passed");
    return 0;
}
