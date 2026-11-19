#ifndef CARDS_MODEL_H
#define CARDS_MODEL_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum { DECK_SIZE = 52 };
typedef enum { CLUBS, DIAMONDS, HEARTS, SPADES } Suit;
/* Identity only: ranks 1 (ace) through 13 (king). */
typedef struct { Suit suit; unsigned rank; } Card;
typedef struct { Card card; bool face_up; } CardState;
/* Bottom is index zero; top is count-1. Owns a copy, no screen coordinates. */
typedef struct { CardState cards[DECK_SIZE]; size_t count; } Pile;
bool card_valid(Card card);
void deck_standard(Card deck[DECK_SIZE]);
/* Explicit algorithm: xorshift32 + rejection sampling + Fisher-Yates.
   Zero seed aliases 0x6d2b79f5. No global random state. */
void deck_shuffle(Card deck[DECK_SIZE], uint32_t seed);
bool pile_init(Pile *pile, const CardState *cards, size_t count);
const CardState *pile_at(const Pile *pile, size_t index);
const CardState *pile_top(const Pile *pile);
#endif
