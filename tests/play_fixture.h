#ifndef PLAY_TEST_FIXTURE_H
#define PLAY_TEST_FIXTURE_H
#include "play.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); exit(1); } } while (0)
static inline bool same_state(const KlondikeState *a, const KlondikeState *b) {
    for (int z = K_STOCK; z <= K_TABLEAU; ++z) {
        unsigned n = z == K_TABLEAU ? 7 : z == K_FOUNDATION ? 4 : 1;
        for (unsigned col = 0; col < n; ++col) {
            KlondikePile ref = {(KlondikeZone)z,col};
            const Pile *p = klondike_pile(a,ref), *q = klondike_pile(b,ref);
            if (p->count != q->count) return false;
            for (size_t i = 0; i < p->count; ++i)
                if (p->cards[i].card.rank != q->cards[i].card.rank || p->cards[i].card.suit != q->cards[i].card.suit || p->cards[i].face_up != q->cards[i].face_up) return false;
        }
    }
    return true;
}
static inline void add(Pile *p, Suit suit, unsigned rank, bool up) {
    CHECK(p->count < 52); p->cards[p->count++] = (CardState){{suit,rank},up};
}
static inline void complete(KlondikeState *s) {
    bool seen[4][14] = {{false}};
    for (int z = K_STOCK; z <= K_TABLEAU; ++z) {
        unsigned n = z == K_TABLEAU ? 7 : z == K_FOUNDATION ? 4 : 1;
        for (unsigned col = 0; col < n; ++col) {
            const Pile *p = klondike_pile(s,(KlondikePile){(KlondikeZone)z,col});
            for (size_t i = 0; i < p->count; ++i) {
                Card c = p->cards[i].card; CHECK(card_valid(c) && !seen[c.suit][c.rank]); seen[c.suit][c.rank] = true;
            }
        }
    }
    for (Suit suit = CLUBS; suit <= SPADES; ++suit)
        for (unsigned rank = 1; rank <= 13; ++rank) if (!seen[suit][rank]) add(&s->stock,suit,rank,false);
    CHECK(klondike_valid(s));
}
static inline KlondikeState run_fixture(void) {
    KlondikeState s = {0};
    add(&s.tableau[0],CLUBS,1,false);
    add(&s.tableau[0],SPADES,9,true); add(&s.tableau[0],HEARTS,8,true);
    add(&s.tableau[0],CLUBS,7,true); add(&s.tableau[0],DIAMONDS,6,true);
    add(&s.tableau[1],CLUBS,9,true); add(&s.tableau[2],HEARTS,5,true);
    add(&s.waste,HEARTS,1,true); complete(&s); return s;
}
#endif
