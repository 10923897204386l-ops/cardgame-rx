#include "klondike.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
#define TABLEAU(n) ((KlondikePile){K_TABLEAU, (n)})
#define FOUNDATION(n) ((KlondikePile){K_FOUNDATION, (n)})
#define WASTE ((KlondikePile){K_WASTE, 0})
#define STOCK ((KlondikePile){K_STOCK, 0})
static bool same_card(Card a, Card b) { return a.suit == b.suit && a.rank == b.rank; }
static bool same_state(const KlondikeState *a, const KlondikeState *b) {
    for (int zone = K_STOCK; zone <= K_TABLEAU; ++zone) {
        unsigned count = zone == K_TABLEAU ? 7 : zone == K_FOUNDATION ? 4 : 1;
        for (unsigned col = 0; col < count; ++col) {
            KlondikePile ref = {(KlondikeZone)zone, col};
            const Pile *p = klondike_pile(a, ref), *q = klondike_pile(b, ref);
            if (p->count != q->count) return false;
            for (size_t i = 0; i < p->count; ++i)
                if (!same_card(p->cards[i].card, q->cards[i].card) || p->cards[i].face_up != q->cards[i].face_up) return false;
        }
    }
    return true;
}
static void add(Pile *p, Suit suit, unsigned rank, bool up) {
    CHECK(p->count < DECK_SIZE);
    p->cards[p->count++] = (CardState){{suit, rank}, up};
}
/* Complete each fixture with every missing identity in face-down stock. */
static void complete(KlondikeState *s) {
    bool seen[4][14] = {{false}};
    for (int zone = K_STOCK; zone <= K_TABLEAU; ++zone) {
        unsigned count = zone == K_TABLEAU ? 7 : zone == K_FOUNDATION ? 4 : 1;
        for (unsigned col = 0; col < count; ++col) {
            const Pile *p = klondike_pile(s, (KlondikePile){(KlondikeZone)zone, col});
            for (size_t i = 0; i < p->count; ++i) {
                Card c = p->cards[i].card;
                CHECK(card_valid(c) && !seen[c.suit][c.rank]);
                seen[c.suit][c.rank] = true;
            }
        }
    }
    for (Suit suit = CLUBS; suit <= SPADES; ++suit)
        for (unsigned rank = 1; rank <= 13; ++rank)
            if (!seen[suit][rank]) add(&s->stock, suit, rank, false);
    CHECK(klondike_valid(s));
}
static void foundation(Pile *p, Suit suit, unsigned count) {
    for (unsigned rank = 1; rank <= count; ++rank) add(p, suit, rank, true);
}
static void unchanged(const KlondikeGame *g, const KlondikeState *before,
                      const struct KlondikeUndo *history, size_t depth) {
    CHECK(same_state(&g->state, before));
    CHECK(g->history == history && g->undo_depth == depth);
}
static void undo_to(KlondikeGame *g, const KlondikeState *before) {
    CHECK(klondike_undo(g)); CHECK(same_state(&g->state, before)); CHECK(klondike_valid(&g->state));
}
static void deals(void) {
    for (uint32_t seed = 0; seed < 1000; ++seed) {
        KlondikeState a, b; klondike_deal(&a, seed); klondike_deal(&b, seed);
        CHECK(klondike_valid(&a)); CHECK(same_state(&a, &b)); CHECK(!klondike_won(&a));
        CHECK(a.stock.count == 24 && a.waste.count == 0);
        Card deck[52]; deck_standard(deck); deck_shuffle(deck, seed); size_t n = 0;
        for (size_t col = 0; col < 7; ++col) {
            CHECK(a.tableau[col].count == col + 1);
            for (size_t i = 0; i <= col; ++i) {
                CHECK(same_card(a.tableau[col].cards[i].card, deck[n++]));
                CHECK(a.tableau[col].cards[i].face_up == (i == col));
            }
        }
        for (size_t i = 0; i < 24; ++i) {
            CHECK(same_card(a.stock.cards[23 - i].card, deck[28 + i]));
            CHECK(!a.stock.cards[i].face_up);
        }
    }
    KlondikeState a, b; klondike_deal(&a, 42); klondike_deal(&b, 43); CHECK(!same_state(&a, &b));
}
static void suffixes(void) {
    const Card destinations[] = {{HEARTS, 10}, {CLUBS, 9}, {DIAMONDS, 8}, {SPADES, 7}};
    for (size_t excluded = 0; excluded < 4; ++excluded) {
        KlondikeGame g = {0};
        add(&g.state.tableau[0], CLUBS, 1, false);
        add(&g.state.tableau[0], SPADES, 9, true);
        add(&g.state.tableau[0], HEARTS, 8, true);
        add(&g.state.tableau[0], CLUBS, 7, true);
        add(&g.state.tableau[0], DIAMONDS, 6, true);
        add(&g.state.tableau[1], destinations[excluded].suit, destinations[excluded].rank, true);
        complete(&g.state);
        KlondikeState before = g.state; size_t chosen = SIZE_MAX;
        CHECK(klondike_longest_suffix(&g.state, TABLEAU(0), 1, TABLEAU(1), &chosen));
        CHECK(chosen == 1 + excluded); unchanged(&g, &before, NULL, 0);
        CHECK(klondike_can_move(&g.state, TABLEAU(0), chosen, TABLEAU(1)));
        if (excluded) { CHECK(!klondike_move(&g, TABLEAU(0), 1, TABLEAU(1))); unchanged(&g, &before, NULL, 0); }
        CHECK(klondike_drop(&g, TABLEAU(0), 1, TABLEAU(1)));
        CHECK(g.undo_depth == 1 && klondike_valid(&g.state));
        CHECK(g.state.tableau[0].count == excluded + 1);
        CHECK(g.state.tableau[1].count == 5 - excluded);
        for (size_t i = 0; i < excluded; ++i)
            CHECK(same_card(g.state.tableau[0].cards[1 + i].card, before.tableau[0].cards[1 + i].card));
        for (size_t i = excluded; i < 4; ++i)
            CHECK(same_card(g.state.tableau[1].cards[1 + i - excluded].card, before.tableau[0].cards[1 + i].card));
        /* Full transfer reveals hidden ace in this same transaction. */
        CHECK(g.state.tableau[0].cards[0].face_up == (excluded == 0));
        undo_to(&g, &before); CHECK(g.undo_depth == 0); CHECK(!klondike_undo(&g));
        klondike_dispose(&g);
    }
    KlondikeGame g = {0};
    add(&g.state.tableau[0], SPADES, 9, true); add(&g.state.tableau[0], HEARTS, 8, true);
    add(&g.state.tableau[0], CLUBS, 7, true); add(&g.state.tableau[0], DIAMONDS, 6, true);
    add(&g.state.tableau[1], HEARTS, 10, true); add(&g.state.tableau[2], SPADES, 5, true);
    complete(&g.state); KlondikeState before = g.state; size_t chosen = 99;
    /* Selecting a higher index cannot expand the candidate back to the 9. */
    CHECK(!klondike_longest_suffix(&g.state, TABLEAU(0), 1, TABLEAU(1), &chosen)); CHECK(chosen == 99);
    CHECK(!klondike_drop(&g, TABLEAU(0), 0, TABLEAU(2)));
    CHECK(!klondike_drop(&g, TABLEAU(0), 0, TABLEAU(3))); /* empty accepts only king */
    CHECK(!klondike_drop(&g, TABLEAU(0), 0, TABLEAU(0)));
    CHECK(!klondike_drop(&g, TABLEAU(0), 4, TABLEAU(1)));
    CHECK(!klondike_drop(&g, TABLEAU(0), SIZE_MAX, TABLEAU(1)));
    CHECK(!klondike_longest_suffix(&g.state, TABLEAU(0), 0, TABLEAU(1), NULL));
    unchanged(&g, &before, NULL, 0);
    klondike_dispose(&g);
}
static void legality(void) {
    KlondikeGame g = {0};
    add(&g.state.tableau[0], HEARTS, 13, true); add(&g.state.tableau[0], CLUBS, 12, true);
    add(&g.state.tableau[1], SPADES, 1, false); add(&g.state.tableau[1], DIAMONDS, 2, true);
    add(&g.state.tableau[2], HEARTS, 3, true); /* same colour rejects 2D */
    add(&g.state.tableau[3], CLUBS, 4, true); /* wrong rank rejects 2D */
    add(&g.state.waste, CLUBS, 1, true); add(&g.state.waste, SPADES, 13, true);
    complete(&g.state); KlondikeState before = g.state;
    CHECK(klondike_can_move(&g.state, TABLEAU(0), 0, TABLEAU(4)));
    CHECK(!klondike_can_move(&g.state, TABLEAU(0), 1, TABLEAU(4)));
    CHECK(!klondike_can_move(&g.state, TABLEAU(1), 0, TABLEAU(4)));
    CHECK(!klondike_can_move(&g.state, TABLEAU(1), 1, TABLEAU(2)));
    CHECK(!klondike_can_move(&g.state, TABLEAU(1), 1, TABLEAU(3)));
    CHECK(!klondike_move(&g, WASTE, 0, FOUNDATION(0))); /* buried waste */
    CHECK(klondike_move(&g, WASTE, 1, TABLEAU(4))); /* waste king to empty */
    CHECK(g.state.waste.count == 1); CHECK(klondike_move(&g, WASTE, 0, FOUNDATION(0)));
    CHECK(g.state.foundations[0].count == 1 && g.undo_depth == 2);
    CHECK(!klondike_move(&g, TABLEAU(0), 0, FOUNDATION(1))); /* run */
    CHECK(!klondike_move(&g, TABLEAU(1), 1, FOUNDATION(0))); /* suit */
    CHECK(!klondike_move(&g, TABLEAU(1), 1, FOUNDATION(1))); /* non-ace */
    CHECK(klondike_undo(&g)); undo_to(&g, &before);
    CHECK(klondike_move(&g, TABLEAU(0), 0, TABLEAU(4))); CHECK(g.state.tableau[0].count == 0);
    undo_to(&g, &before); klondike_dispose(&g);

    g = (KlondikeGame){0};
    foundation(&g.state.foundations[0], HEARTS, 3);
    add(&g.state.tableau[0], CLUBS, 4, true);
    add(&g.state.tableau[1], HEARTS, 4, true);
    add(&g.state.waste, HEARTS, 5, true);
    complete(&g.state); before = g.state;
    CHECK(!klondike_move(&g, WASTE, 0, FOUNDATION(0))); /* skips rank 4 */
    CHECK(klondike_move(&g, TABLEAU(1), 0, FOUNDATION(0)));
    CHECK(klondike_move(&g, WASTE, 0, FOUNDATION(0)));
    CHECK(klondike_undo(&g)); CHECK(klondike_undo(&g)); CHECK(same_state(&g.state, &before));
    CHECK(!klondike_move(&g, FOUNDATION(0), 0, TABLEAU(0))); /* buried foundation */
    CHECK(!klondike_move(&g, FOUNDATION(0), 2, FOUNDATION(1)));
    CHECK(klondike_move(&g, FOUNDATION(0), 2, TABLEAU(0))); /* 3H onto 4C */
    CHECK(g.state.foundations[0].count == 2 && g.state.tableau[0].count == 2);
    undo_to(&g, &before); klondike_dispose(&g);

    /* A run cannot go to foundation, but longest suffix may move its top ace. */
    g = (KlondikeGame){0}; add(&g.state.tableau[0], SPADES, 4, true);
    add(&g.state.tableau[0], HEARTS, 3, true); add(&g.state.tableau[0], CLUBS, 2, true);
    add(&g.state.tableau[0], HEARTS, 1, true); complete(&g.state); before = g.state;
    size_t chosen = 0;
    CHECK(klondike_longest_suffix(&g.state, TABLEAU(0), 0, FOUNDATION(3), &chosen)); CHECK(chosen == 3);
    CHECK(klondike_drop(&g, TABLEAU(0), 0, FOUNDATION(3))); CHECK(g.state.tableau[0].count == 3);
    undo_to(&g, &before); klondike_dispose(&g);
}
static void stock_order(void) {
    KlondikeGame g = {0}; klondike_new_game(&g, 42); KlondikeState initial = g.state;
    Card order[24];
    for (size_t i = 0; i < 24; ++i) order[i] = initial.stock.cards[23 - i].card;
    CHECK(!klondike_recycle(&g)); unchanged(&g, &initial, NULL, 0);
    for (size_t cycle = 0; cycle < 5; ++cycle) {
        for (size_t i = 0; i < 24; ++i) {
            KlondikeState before = g.state; size_t depth = g.undo_depth;
            CHECK(klondike_draw(&g)); CHECK(g.undo_depth == depth + 1);
            CHECK(same_card(pile_top(&g.state.waste)->card, order[i]));
            CHECK(g.state.waste.count == i + 1 && g.state.stock.count == 23 - i);
            if (i == 5) { undo_to(&g, &before); CHECK(klondike_draw(&g)); }
        }
        KlondikeState exhausted = g.state; size_t depth = g.undo_depth; struct KlondikeUndo *history = g.history;
        CHECK(!klondike_draw(&g)); unchanged(&g, &exhausted, history, depth);
        CHECK(klondike_recycle(&g)); CHECK(g.state.stock.count == 24 && !g.state.waste.count);
        CHECK(same_state(&g.state, &initial));
        undo_to(&g, &exhausted); CHECK(klondike_recycle(&g));
    }
    while (g.undo_depth) CHECK(klondike_undo(&g));
    CHECK(same_state(&g.state, &initial)); klondike_dispose(&g);

    /* Transfer aces out during a full pass; recycle only surviving waste,
       preserving its relative draw order. No duplication or resurrection. */
    g = (KlondikeGame){0}; complete(&g.state); Card survivors[52]; size_t n = 0;
    while (g.state.stock.count) {
        CHECK(klondike_draw(&g)); Card c = pile_top(&g.state.waste)->card;
        if (c.rank == 1) CHECK(klondike_move(&g, WASTE, g.state.waste.count - 1, FOUNDATION(c.suit)));
        else survivors[n++] = c;
    }
    CHECK(n == 48); CHECK(klondike_recycle(&g));
    for (size_t i = 0; i < n; ++i) {
        CHECK(klondike_draw(&g)); CHECK(same_card(pile_top(&g.state.waste)->card, survivors[i]));
    }
    CHECK(klondike_valid(&g.state));
    klondike_new_game(&g, 7); CHECK(g.undo_depth == 0 && !g.history && klondike_valid(&g.state));
    CHECK(!klondike_undo(&g)); klondike_dispose(&g); klondike_dispose(&g);
}
static void winning_and_invalid(void) {
    KlondikeGame g = {0};
    for (unsigned i = 0; i < 4; ++i) foundation(&g.state.foundations[i], (Suit)i, i == 3 ? 12 : 13);
    add(&g.state.waste, SPADES, 13, true); complete(&g.state);
    CHECK(!klondike_won(&g.state)); KlondikeState before = g.state;
    CHECK(klondike_move(&g, WASTE, 0, FOUNDATION(3))); CHECK(klondike_won(&g.state));
    KlondikeState won = g.state; size_t depth = g.undo_depth; struct KlondikeUndo *history = g.history;
    CHECK(!klondike_draw(&g)); CHECK(!klondike_recycle(&g));
    CHECK(!klondike_move(&g, FOUNDATION(3), 12, FOUNDATION(0)));
    CHECK(!klondike_move(&g, WASTE, 0, TABLEAU(0))); unchanged(&g, &won, history, depth);
    CHECK(klondike_move(&g, FOUNDATION(3), 12, TABLEAU(6))); CHECK(!klondike_won(&g.state));
    undo_to(&g, &won); undo_to(&g, &before); klondike_dispose(&g);

    klondike_new_game(&g, 42); before = g.state;
    const KlondikePile invalid[] = {{K_TABLEAU,7},{K_FOUNDATION,4},{K_WASTE,1},{K_STOCK,1},{(KlondikeZone)-1,0}};
    for (size_t i = 0; i < sizeof(invalid)/sizeof(*invalid); ++i) {
        CHECK(!klondike_pile(&g.state, invalid[i]));
        CHECK(!klondike_drop(&g, invalid[i], 0, TABLEAU(0)));
        CHECK(!klondike_drop(&g, TABLEAU(0), 0, invalid[i]));
    }
    CHECK(!klondike_move(&g, STOCK, 0, TABLEAU(0)));
    CHECK(!klondike_move(&g, TABLEAU(0), 0, STOCK));
    CHECK(!klondike_move(&g, TABLEAU(0), 0, WASTE));
    unchanged(&g, &before, NULL, 0);
    g.state.stock.count = 53; CHECK(!klondike_valid(&g.state)); CHECK(!klondike_draw(&g));
    g.state = before; g.state.stock.cards[0].card = g.state.stock.cards[1].card; CHECK(!klondike_valid(&g.state));
    g.state = before; g.state.stock.cards[0].face_up = true; CHECK(!klondike_valid(&g.state));
    g.state = before; g.state.tableau[1].cards[1].face_up = false; CHECK(!klondike_valid(&g.state));
    g.state = before; g.state.tableau[6].cards[0].face_up = true; CHECK(!klondike_valid(&g.state));
    g.state = before; --g.state.stock.count; CHECK(!klondike_valid(&g.state));
    g.state = before; g.state.stock.cards[0].card.rank = 14; CHECK(!klondike_valid(&g.state));
    CHECK(!klondike_valid(NULL)); CHECK(!klondike_won(NULL)); CHECK(!klondike_draw(NULL));
    CHECK(!klondike_recycle(NULL)); CHECK(!klondike_undo(NULL)); CHECK(!klondike_move(NULL,WASTE,0,TABLEAU(0)));
    CHECK(!klondike_drop(NULL,WASTE,0,TABLEAU(0))); CHECK(!klondike_pile(NULL,STOCK));
    klondike_deal(NULL,0); klondike_new_game(NULL,0); klondike_dispose(NULL);
    klondike_dispose(&g);
}
int main(void) {
    deals(); suffixes(); legality(); stock_order(); winning_and_invalid();
    puts("klondike: deterministic deals, legality, suffixes, transactions, redeals and undo passed");
    return 0;
}
