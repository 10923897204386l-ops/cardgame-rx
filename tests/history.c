#include "play_fixture.h"
#ifdef TEST_WRAP_MALLOC
static bool fail_malloc;
static size_t malloc_calls;
void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size) {
    ++malloc_calls;
    return fail_malloc ? NULL : __real_malloc(size);
}
#endif
static void limits(void) {
    KlondikeGame g = {0}; klondike_set_undo_limit(&g,3); klondike_new_game(&g,42);
    KlondikeState states[7]; states[0] = g.state;
    for (size_t i = 1; i <= 6; ++i) {
        CHECK(klondike_draw(&g)); states[i] = g.state;
        CHECK(g.undo_depth == (i < 3 ? i : 3));
    }
    for (size_t i = 6; i > 3; --i) { CHECK(klondike_undo(&g)); CHECK(same_state(&g.state,&states[i-1])); }
    CHECK(!klondike_undo(&g) && same_state(&g.state,&states[3]));
    CHECK(klondike_draw(&g)); CHECK(klondike_draw(&g)); CHECK(klondike_draw(&g));
    klondike_set_undo_limit(&g,1); CHECK(g.undo_depth == 1);
    CHECK(klondike_undo(&g)); CHECK(same_state(&g.state,&states[5])); CHECK(!klondike_undo(&g));
    klondike_set_undo_limit(&g,0);
    for (size_t i = 0; i < 10; ++i) CHECK(klondike_draw(&g));
    CHECK(g.undo_depth == 10);
    klondike_set_undo_limit(&g,2); CHECK(g.undo_depth == 2);
    klondike_new_game(&g,43); CHECK(g.undo_depth == 0 && g.undo_limit == 2);
    CHECK(klondike_draw(&g)); CHECK(klondike_draw(&g)); CHECK(klondike_draw(&g)); CHECK(g.undo_depth == 2);
    klondike_dispose(&g); CHECK(g.undo_depth == 0 && g.undo_limit == 2);
    klondike_set_undo_limit(NULL,3);
}
#ifdef TEST_WRAP_MALLOC
static void allocation_failures(void) {
    KlondikeGame g = {0}; g.state = run_fixture();
    CHECK(klondike_draw(&g)); /* preserve an existing nonempty history too */
    KlondikeState before = g.state;
    struct KlondikeUndo *history = g.history; size_t depth = g.undo_depth;
    fail_malloc = true; size_t calls = malloc_calls;
    CHECK(!klondike_draw(&g)); CHECK(malloc_calls == calls+1);
    CHECK(!klondike_drop(&g,(KlondikePile){K_TABLEAU,0},1,(KlondikePile){K_TABLEAU,1}));
    CHECK(malloc_calls == calls+2);
    CHECK(same_state(&g.state,&before) && g.history == history && g.undo_depth == depth);
    fail_malloc = false; CHECK(klondike_draw(&g)); klondike_dispose(&g);

    g = (KlondikeGame){0}; complete(&g.state);
    while (g.state.stock.count) CHECK(klondike_draw(&g));
    before = g.state; history = g.history; depth = g.undo_depth;
    fail_malloc = true; CHECK(!klondike_recycle(&g));
    CHECK(same_state(&g.state,&before) && g.history == history && g.undo_depth == depth);
    fail_malloc = false; CHECK(klondike_recycle(&g)); klondike_dispose(&g);
    puts("undo allocation failure: draw, suffix drop and recycle preserve state/history");
}
#endif
int main(void) {
    limits();
#ifdef TEST_WRAP_MALLOC
    allocation_failures();
#endif
    puts("history: configurable retention, pruning, unlimited mode, reset and dispose passed");
    return 0;
}
