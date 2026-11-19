#include "scene.h"
void scene_create(Scene *s,uint32_t seed,bool stress) {
    *s=(Scene){0}; Card deck[DECK_SIZE]; deck_standard(deck); deck_shuffle(deck,seed);
    size_t used=0;
    for (size_t col=0;col<7;++col) {
        size_t count=stress && col==0 ? 13 : col+1;
        CardState cards[DECK_SIZE];
        for (size_t i=0;i<count;++i) cards[i]=(CardState){deck[used++],stress || i+3>=count};
        pile_init(&s->tableau[col],cards,count);
    }
    CardState waste={deck[used++],true}; pile_init(&s->waste,&waste,1);
    CardState rest[DECK_SIZE]; size_t n=0;
    while (used<DECK_SIZE) rest[n++]=(CardState){deck[used++],false};
    pile_init(&s->stock,rest,n);
}
