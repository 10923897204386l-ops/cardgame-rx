#ifndef CARDS_KLONDIKE_H
#define CARDS_KLONDIKE_H
#include "model.h"

/* All piles are bottom-to-top, as in model.h. No UI or cursor state lives here. */
typedef struct {
    Pile stock, waste, foundations[4], tableau[7];
} KlondikeState;
typedef enum { K_STOCK, K_WASTE, K_FOUNDATION, K_TABLEAU } KlondikeZone;
typedef struct { KlondikeZone zone; unsigned index; } KlondikePile;
struct KlondikeUndo;
/* Owning value: initialize with {0}, never shallow-copy, dispose when finished.
   Treat state as read-only outside fixture construction; new_game clears undo. */
typedef struct {
    KlondikeState state;
    struct KlondikeUndo *history;
    size_t undo_depth;
} KlondikeGame;

/* Column-wise deal from shuffled deck[0..27]; top only face up.
   Stock bottom-to-top is deck[51..28], so deck[28] is the next draw. */
void klondike_deal(KlondikeState *state, uint32_t seed);
void klondike_new_game(KlondikeGame *game, uint32_t seed);
void klondike_dispose(KlondikeGame *game);
const Pile *klondike_pile(const KlondikeState *state, KlondikePile pile);
/* Validate a complete 52-card state, including face orientation and runs. */
bool klondike_valid(const KlondikeState *state);
bool klondike_won(const KlondikeState *state);
/* Exact suffix [start, source.count). Stock is never a transfer source or
   destination. Waste/foundation sources may only transfer their top card.
   Foundation -> tableau is allowed; foundation -> foundation is forbidden. */
bool klondike_can_move(const KlondikeState *state, KlondikePile source,
                       size_t start, KlondikePile destination);
/* Candidate must itself be a valid face-up run. Try candidate then each shorter
   suffix in order. On failure leave *chosen_start untouched. No state mutation. */
bool klondike_longest_suffix(const KlondikeState *state, KlondikePile source,
                             size_t candidate_start, KlondikePile destination,
                             size_t *chosen_start);
/* Each success is one snapshot-backed undo step. Failed operations, including
   allocation failure, preserve state and history. Reveals belong to transfers. */
bool klondike_move(KlondikeGame *game, KlondikePile source, size_t start,
                   KlondikePile destination);
bool klondike_drop(KlondikeGame *game, KlondikePile source, size_t candidate_start,
                   KlondikePile destination);
bool klondike_draw(KlondikeGame *game);
/* Only when stock empty and waste nonempty: reverse waste into face-down stock.
   No shuffle/redeal limit; oldest surviving waste card is next to draw. */
bool klondike_recycle(KlondikeGame *game);
bool klondike_undo(KlondikeGame *game);
#endif
