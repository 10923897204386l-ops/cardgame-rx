#include "model.h"
#include <string.h>
bool card_valid(Card c) { return c.suit >= CLUBS && c.suit <= SPADES && c.rank >= 1 && c.rank <= 13; }
void deck_standard(Card deck[DECK_SIZE]) {
    for (unsigned s=0; s<4; ++s)
        for (unsigned r=1; r<=13; ++r) deck[s*13+r-1]=(Card){(Suit)s,r};
}
static uint32_t next(uint32_t *s) {
    *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5; return *s;
}
void deck_shuffle(Card deck[DECK_SIZE], uint32_t seed) {
    uint32_t state=seed ? seed : UINT32_C(0x6d2b79f5);
    for (uint32_t n=DECK_SIZE; n>1; --n) {
        /* Generator emits 1..UINT32_MAX; map to zero-based uniform draws. */
        uint32_t x, limit=UINT32_MAX-(UINT32_MAX % n);
        do { x=next(&state)-1; } while (x>=limit);
        size_t j=x%n; Card tmp=deck[n-1]; deck[n-1]=deck[j]; deck[j]=tmp;
    }
}
bool pile_init(Pile *p, const CardState *cards, size_t count) {
    if (!p || count>DECK_SIZE || (count && !cards)) return false;
    for (size_t i=0;i<count;++i) {
        if (!card_valid(cards[i].card)) return false;
        for (size_t j=0;j<i;++j)
            if (cards[i].card.suit==cards[j].card.suit && cards[i].card.rank==cards[j].card.rank) return false;
    }
    Pile result={.count=count};
    if (count) memcpy(result.cards,cards,count*sizeof(*cards));
    *p=result; return true;
}
const CardState *pile_at(const Pile *p,size_t i) { return p && p->count<=DECK_SIZE && i<p->count ? &p->cards[i] : NULL; }
const CardState *pile_top(const Pile *p) { return p && p->count ? pile_at(p,p->count-1) : NULL; }
