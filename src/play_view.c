#include "play_view.h"
static int min(int a, int b) { return a < b ? a : b; }
Rect play_card_rect(const Layout *l, const Pile *p, unsigned col, size_t index) {
    Rect full, visible;
    if (!layout_card(l,p,col,index,&full,&visible)) return (Rect){0};
    size_t hidden = play_first_face_up(p), up = p->count - hidden;
    int height = (int)hidden * l->face_down_overlap + (up ? (int)(up - 1) * l->face_up_overlap : 0) + l->card_h;
    if (l->tableau_y + height <= CANVAS_HEIGHT) return full;
    int y = min(l->tableau_y,CANVAS_HEIGHT - (int)up * 22);
    int band = min((int)hidden * l->face_down_overlap,24);
    band = min(band,CANVAS_HEIGHT - y - (int)up * 22);
    full.y = y + (index < hidden ? (int)index * band / (int)hidden : band + (int)(index - hidden) * 22);
    return full;
}
Rect play_focus_rect(const Layout *l, const KlondikeState *s, const PlayUI *ui) {
    KlondikePile focus = play_focus(ui);
    const Pile *p = klondike_pile(s,focus);
    if (focus.zone == K_TABLEAU) {
        Rect r = p->count ? play_card_rect(l,p,focus.index,ui->selected) : layout_slot(l,focus.index,true);
        /* Focus surrounds the exposed index when another card follows. */
        if (p->count && ui->selected + 1 < p->count) {
            Rect next = play_card_rect(l,p,focus.index,ui->selected + 1);
            r.h = next.y - r.y;
        }
        r.h = min(r.h,CANVAS_HEIGHT - r.y - 2);
        return r;
    }
    return layout_slot(l,focus.zone == K_FOUNDATION ? focus.index + 3 : focus.zone == K_WASTE ? 1 : 0,false);
}
int play_held_overlap(const Layout *l, size_t count) {
    return count > 1 ? min(l->face_up_overlap,(CANVAS_HEIGHT - 40 - l->card_h) / (int)(count - 1)) : l->face_up_overlap;
}
Rect play_held_bounds(const Layout *l, const KlondikeState *s, const PlayUI *ui) {
    size_t n = play_held_count(ui,s);
    if (!n) return (Rect){0};
    Rect at = play_focus_rect(l,s,ui);
    Rect b = {min(at.x + 12,CANVAS_WIDTH - l->card_w - 6),at.y + 12,l->card_w,l->card_h + (int)(n - 1) * play_held_overlap(l,n)};
    b.y = min(b.y,CANVAS_HEIGHT - b.h - 6);
    if (b.y < l->status_h + 6) b.y = l->status_h + 6;
    return b;
}
