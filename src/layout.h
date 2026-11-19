#ifndef CARDS_LAYOUT_H
#define CARDS_LAYOUT_H
#include "model.h"
enum { CANVAS_WIDTH=640, CANVAS_HEIGHT=480 };
typedef struct { int x,y,w,h; } Rect;
typedef struct {
    int card_w,card_h,margin,column_gap,status_h,top_y,tableau_y;
    int face_up_overlap,face_down_overlap;
} Layout;
Layout layout_default(void);
bool layout_valid(const Layout *l);
Rect layout_slot(const Layout *l, unsigned column, bool tableau);
/* Visible height is the exposed strip, or full height for the top card.
   Rectangles may extend below canvas in the deliberate stress scene. */
bool layout_card(const Layout *l,const Pile *p,unsigned column,size_t index,Rect *full,Rect *visible);
#endif
