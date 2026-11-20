#include "layout.h"
/* Larger status band for readable HUD; cards stay within 640 width. */
Layout layout_default(void) { return (Layout){74,104,11,12,44,48,176,26,12}; }
bool layout_valid(const Layout *l) {
    return l && l->card_w>=60 && l->card_w<=80 && l->card_h>=90 && l->card_h<=120
        && l->margin>=0 && l->margin<=40 && l->column_gap>=0 && l->column_gap<=40
        && l->status_h>=20 && l->status_h<=56 && l->top_y>=l->status_h
        && l->top_y<=60 && l->tableau_y>=l->top_y+l->card_h
        && l->tableau_y<=200 && l->face_up_overlap>=22 && l->face_up_overlap<=l->card_h
        && l->face_down_overlap>=1 && l->face_down_overlap<=l->card_h
        && 2*l->margin+7*l->card_w+6*l->column_gap<=CANVAS_WIDTH;
}
Rect layout_slot(const Layout *l,unsigned column,bool tableau) {
    if (!layout_valid(l) || column>=7) return (Rect){0};
    return (Rect){l->margin+(int)column*(l->card_w+l->column_gap),tableau?l->tableau_y:l->top_y,l->card_w,l->card_h};
}
bool layout_card(const Layout *l,const Pile *p,unsigned col,size_t i,Rect *full,Rect *visible) {
    if (!layout_valid(l) || !pile_at(p,i) || col>=7 || !full || !visible) return false;
    Rect r=layout_slot(l,col,true);
    for (size_t j=0;j<i;++j) r.y+=p->cards[j].face_up?l->face_up_overlap:l->face_down_overlap;
    *full=r; if (i+1<p->count) r.h=p->cards[i].face_up?l->face_up_overlap:l->face_down_overlap;
    *visible=r; return true;
}
