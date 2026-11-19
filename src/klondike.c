#include "klondike.h"
#include <stdlib.h>

struct KlondikeUndo {
    KlondikeState state;
    struct KlondikeUndo *previous;
};
static bool red(Card c) { return c.suit == DIAMONDS || c.suit == HEARTS; }
static bool descends(Card bottom, Card top) {
    return bottom.rank == top.rank + 1 && red(bottom) != red(top);
}
const Pile *klondike_pile(const KlondikeState *s, KlondikePile ref) {
    if (!s) return NULL;
    switch (ref.zone) {
    case K_STOCK: return ref.index == 0 ? &s->stock : NULL;
    case K_WASTE: return ref.index == 0 ? &s->waste : NULL;
    case K_FOUNDATION: return ref.index < 4 ? &s->foundations[ref.index] : NULL;
    case K_TABLEAU: return ref.index < 7 ? &s->tableau[ref.index] : NULL;
    default: return NULL;
    }
}
static Pile *mutable_pile(KlondikeState *s, KlondikePile ref) {
    /* All callers have validated the reference with klondike_pile. */
    switch (ref.zone) {
    case K_STOCK: return &s->stock;
    case K_WASTE: return &s->waste;
    case K_FOUNDATION: return &s->foundations[ref.index];
    case K_TABLEAU: return &s->tableau[ref.index];
    default: return NULL;
    }
}
void klondike_deal(KlondikeState *s, uint32_t seed) {
    if (!s) return;
    *s = (KlondikeState){0};
    Card deck[DECK_SIZE];
    deck_standard(deck);
    deck_shuffle(deck, seed);
    size_t next = 0;
    for (size_t col = 0; col < 7; ++col) {
        Pile *p = &s->tableau[col];
        p->count = col + 1;
        for (size_t i = 0; i < p->count; ++i)
            p->cards[i] = (CardState){deck[next++], i == col};
    }
    for (size_t i = DECK_SIZE; i > next; --i)
        s->stock.cards[s->stock.count++] = (CardState){deck[i - 1], false};
}
static bool unique_pile(const Pile *p, bool seen[4][14], size_t *total) {
    if (p->count > DECK_SIZE) return false;
    for (size_t i = 0; i < p->count; ++i) {
        Card c = p->cards[i].card;
        if (!card_valid(c) || seen[c.suit][c.rank]) return false;
        seen[c.suit][c.rank] = true;
        ++*total;
    }
    return true;
}
bool klondike_valid(const KlondikeState *s) {
    if (!s) return false;
    bool seen[4][14] = {{false}};
    size_t total = 0;
    const Pile *piles[13] = {&s->stock, &s->waste};
    for (size_t i = 0; i < 4; ++i) piles[2 + i] = &s->foundations[i];
    for (size_t i = 0; i < 7; ++i) piles[6 + i] = &s->tableau[i];
    for (size_t i = 0; i < 13; ++i)
        if (!unique_pile(piles[i], seen, &total)) return false;
    if (total != DECK_SIZE) return false;
    for (size_t i = 0; i < s->stock.count; ++i)
        if (s->stock.cards[i].face_up) return false;
    for (size_t i = 0; i < s->waste.count; ++i)
        if (!s->waste.cards[i].face_up) return false;
    for (size_t f = 0; f < 4; ++f) {
        const Pile *p = &s->foundations[f];
        if (p->count > 13) return false;
        for (size_t i = 0; i < p->count; ++i)
            if (!p->cards[i].face_up || p->cards[i].card.rank != i + 1 ||
                p->cards[i].card.suit != p->cards[0].card.suit) return false;
    }
    for (size_t col = 0; col < 7; ++col) {
        const Pile *p = &s->tableau[col];
        if (p->count && !p->cards[p->count - 1].face_up) return false;
        for (size_t i = 1; i < p->count; ++i) {
            if (!p->cards[i - 1].face_up) continue;
            if (!p->cards[i].face_up || !descends(p->cards[i - 1].card, p->cards[i].card)) return false;
        }
    }
    return true;
}
bool klondike_won(const KlondikeState *s) {
    if (!klondike_valid(s)) return false;
    for (size_t f = 0; f < 4; ++f)
        if (s->foundations[f].count != 13) return false;
    return true;
}
static bool candidate(const KlondikeState *s, KlondikePile source, size_t start) {
    const Pile *p = klondike_pile(s, source);
    if (!p || source.zone == K_STOCK || !pile_at(p, start)) return false;
    if (source.zone != K_TABLEAU && start != p->count - 1) return false;
    for (size_t i = start; i < p->count; ++i) {
        if (!p->cards[i].face_up) return false;
        if (i > start && !descends(p->cards[i - 1].card, p->cards[i].card)) return false;
    }
    return true;
}
static bool legal_destination(const KlondikeState *s, KlondikePile source,
                              size_t start, KlondikePile destination) {
    const Pile *from = klondike_pile(s, source), *to = klondike_pile(s, destination);
    if (!to || from == to) return false;
    Card first = from->cards[start].card;
    const CardState *top = pile_top(to);
    if (destination.zone == K_TABLEAU)
        return top ? top->face_up && descends(top->card, first) : first.rank == 13;
    if (destination.zone != K_FOUNDATION || source.zone == K_FOUNDATION ||
        from->count - start != 1) return false;
    return top ? top->card.suit == first.suit && first.rank == top->card.rank + 1 : first.rank == 1;
}
bool klondike_can_move(const KlondikeState *s, KlondikePile from, size_t start, KlondikePile to) {
    return klondike_valid(s) && candidate(s, from, start) && legal_destination(s, from, start, to);
}
bool klondike_longest_suffix(const KlondikeState *s, KlondikePile from,
                            size_t start, KlondikePile to, size_t *chosen) {
    if (!chosen || !klondike_valid(s) || !candidate(s, from, start)) return false;
    const Pile *p = klondike_pile(s, from);
    for (size_t i = start; i < p->count; ++i) {
        if (legal_destination(s, from, i, to)) { *chosen = i; return true; }
    }
    return false;
}
static CardState pop(Pile *p) {
    CardState c = p->cards[--p->count];
    p->cards[p->count] = (CardState){0};
    return c;
}
static bool commit(KlondikeGame *g, const KlondikeState *next) {
    if (!klondike_valid(next) || g->undo_depth == SIZE_MAX) return false;
    struct KlondikeUndo *undo = malloc(sizeof(*undo));
    if (!undo) return false;
    undo->state = g->state;
    undo->previous = g->history;
    g->history = undo;
    ++g->undo_depth;
    g->state = *next;
    klondike_set_undo_limit(g, g->undo_limit);
    return true;
}
bool klondike_move(KlondikeGame *g, KlondikePile from, size_t start, KlondikePile to) {
    if (!g || !klondike_can_move(&g->state, from, start, to)) return false;
    KlondikeState next = g->state;
    Pile *source = mutable_pile(&next, from), *destination = mutable_pile(&next, to);
    size_t old_count = source->count;
    for (size_t i = start; i < old_count; ++i)
        destination->cards[destination->count++] = source->cards[i];
    while (source->count > start) (void)pop(source);
    if (from.zone == K_TABLEAU && source->count)
        source->cards[source->count - 1].face_up = true;
    return commit(g, &next);
}
bool klondike_drop(KlondikeGame *g, KlondikePile from, size_t start, KlondikePile to) {
    size_t chosen;
    return g && klondike_longest_suffix(&g->state, from, start, to, &chosen) &&
        klondike_move(g, from, chosen, to);
}
bool klondike_draw(KlondikeGame *g) {
    if (!g || !klondike_valid(&g->state) || !g->state.stock.count) return false;
    KlondikeState next = g->state;
    CardState c = pop(&next.stock);
    c.face_up = true;
    next.waste.cards[next.waste.count++] = c;
    return commit(g, &next);
}
bool klondike_recycle(KlondikeGame *g) {
    if (!g || !klondike_valid(&g->state) || g->state.stock.count || !g->state.waste.count) return false;
    KlondikeState next = g->state;
    while (next.waste.count) {
        CardState c = pop(&next.waste);
        c.face_up = false;
        next.stock.cards[next.stock.count++] = c;
    }
    return commit(g, &next);
}
bool klondike_undo(KlondikeGame *g) {
    if (!g || !g->history) return false;
    struct KlondikeUndo *undo = g->history;
    g->state = undo->state;
    g->history = undo->previous;
    --g->undo_depth;
    free(undo);
    return true;
}
void klondike_dispose(KlondikeGame *g) {
    if (!g) return;
    while (g->history) {
        struct KlondikeUndo *undo = g->history;
        g->history = undo->previous;
        free(undo);
    }
    g->undo_depth = 0;
}
void klondike_new_game(KlondikeGame *g, uint32_t seed) {
    if (!g) return;
    klondike_dispose(g);
    klondike_deal(&g->state, seed);
}

void klondike_set_undo_limit(KlondikeGame *g, size_t limit) {
    if (!g) return;
    g->undo_limit = limit;
    if (!limit || g->undo_depth <= limit) return;
    struct KlondikeUndo *last = g->history;
    for (size_t i = 1; i < limit; ++i) last = last->previous;
    struct KlondikeUndo *older = last->previous;
    last->previous = NULL;
    while (older) {
        struct KlondikeUndo *next = older->previous;
        free(older);
        older = next;
    }
    g->undo_depth = limit;
}
