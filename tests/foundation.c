#include "scene.h"
#include "layout.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x); exit(1); } } while (0)
static void identities(const Card *d) {
    bool seen[4][14]={0}; unsigned ranks[14]={0},suits[4]={0};
    for (size_t i=0;i<52;++i) { CHECK(card_valid(d[i])); CHECK(!seen[d[i].suit][d[i].rank]); seen[d[i].suit][d[i].rank]=true; ++ranks[d[i].rank]; ++suits[d[i].suit]; }
    for (int i=1;i<=13;++i) CHECK(ranks[i]==4);
    for (int i=0;i<4;++i) CHECK(suits[i]==13);
}
int main(void) {
    Card a[52],b[52]; deck_standard(a); identities(a);
    for (uint32_t seed=0;seed<1000;++seed) {
        deck_standard(a); deck_standard(b); deck_shuffle(a,seed); deck_shuffle(b,seed); identities(a);
        for (int i=0;i<52;++i) CHECK(a[i].rank==b[i].rank && a[i].suit==b[i].suit);
    }
    deck_standard(a); deck_standard(b); deck_shuffle(b,42);
    int changed=0; for (int i=0;i<52;++i) changed+=a[i].rank!=b[i].rank || a[i].suit!=b[i].suit; CHECK(changed>40);
    CardState cs[]={{{CLUBS,1},false},{{HEARTS,13},true}}; Pile p;
    CHECK(pile_init(&p,cs,2)); CHECK(pile_at(&p,0)->card.rank==1); CHECK(pile_top(&p)->card.rank==13);
    cs[0].card.rank=2; CHECK(pile_at(&p,0)->card.rank==1);
    CHECK(!pile_at(&p,2)); CHECK(!pile_at(&p,(size_t)-1)); CHECK(!pile_top(NULL));
    CHECK(!pile_init(&p,NULL,1)); CHECK(!pile_init(&p,cs,53)); CHECK(p.count==2);
    cs[1]=cs[0]; CHECK(!pile_init(&p,cs,2)); cs[1].card.rank=14; CHECK(!pile_init(&p,cs,2));
    Layout l=layout_default(); CHECK(layout_valid(&l)); Rect full,strip;
    CHECK(layout_card(&l,&p,6,0,&full,&strip)); CHECK(full.x==548 && full.y==164 && full.h==100 && strip.h==12);
    CHECK(layout_card(&l,&p,6,1,&full,&strip)); CHECK(full.y==176 && strip.h==100); CHECK(full.x+full.w<=640);
    CHECK(!layout_card(&l,&p,7,0,&full,&strip)); CHECK(!layout_card(&l,&p,0,2,&full,&strip));
    l.face_up_overlap=21; CHECK(!layout_valid(&l)); l=layout_default(); l.face_up_overlap=26; CHECK(layout_valid(&l));
    for (int stress=0;stress<2;++stress) {
        Scene s; scene_create(&s,42,stress); Card all[52]; size_t n=0;
        for (int col=0;col<7;++col) for (size_t i=0;i<s.tableau[col].count;++i) {
            all[n++]=s.tableau[col].cards[i].card; CHECK(layout_card(&l,&s.tableau[col],col,i,&full,&strip));
            CHECK(!s.tableau[col].cards[i].face_up || strip.h>=22);
            if (!stress) CHECK(full.y+full.h<=480);
        }
        for (size_t i=0;i<s.stock.count;++i) all[n++]=s.stock.cards[i].card;
        all[n++]=pile_top(&s.waste)->card; CHECK(n==52); identities(all);
        if (stress) { CHECK(layout_card(&l,&s.tableau[0],0,12,&full,&strip)); CHECK(full.y+full.h>480); }
    }
    CHECK(pile_init(&p,NULL,0)); CHECK(!pile_top(&p)); CHECK(!pile_at(&p,0));
    puts("foundation: deck, 1000 seeds, pile bounds, layout and both scenes passed"); return 0;
}
