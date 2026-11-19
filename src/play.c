#include "play.h"

static unsigned area_size(PlayArea area) {
    return area == AREA_DRAW ? 2 : area == AREA_TABLEAU ? 7 : 4;
}
KlondikePile play_focus(const PlayUI *ui) {
    if (ui->area == AREA_DRAW) return (KlondikePile){ui->focus[AREA_DRAW] ? K_WASTE : K_STOCK, 0};
    return (KlondikePile){ui->area == AREA_TABLEAU ? K_TABLEAU : K_FOUNDATION, ui->focus[ui->area]};
}
size_t play_first_face_up(const Pile *p) {
    if (!p) return 0;
    size_t i = 0;
    while (i < p->count && !p->cards[i].face_up) ++i;
    return i;
}
static void select_default(PlayUI *ui, const KlondikeState *s) {
    ui->selected = play_first_face_up(&s->tableau[ui->focus[AREA_TABLEAU]]);
}
void play_init(PlayUI *ui, const KlondikeState *s, uint32_t seed) {
    *ui = (PlayUI){.seed = seed};
    select_default(ui, s);
}
size_t play_selected(const PlayUI *ui, const KlondikeState *s) {
    KlondikePile ref = play_focus(ui);
    const Pile *p = klondike_pile(s, ref);
    return ref.zone == K_TABLEAU ? ui->selected : p->count ? p->count - 1 : 0;
}
size_t play_held_count(const PlayUI *ui, const KlondikeState *s) {
    const Pile *p = ui->held ? klondike_pile(s, ui->source) : NULL;
    return p && ui->source_start < p->count ? p->count - ui->source_start : 0;
}
bool play_preview(const PlayUI *ui, const KlondikeState *s, size_t *chosen) {
    return ui->held && klondike_longest_suffix(s, ui->source, ui->source_start, play_focus(ui), chosen);
}
bool play_invalid_outline(const PlayUI *ui, uint64_t now) {
    return ui->held && now < ui->invalid_until;
}
const char *play_message(const PlayUI *ui, uint64_t now) {
    static const char *messages[MSG_COUNT] = {
        "", "CARRYING", "CANCELLED", "INVALID DROP", "NO CARD HERE",
        "CANCEL OR PLACE FIRST", "NO LEGAL MOVE", "PLACED", "DREW ONE", "RECYCLED",
        "UNDONE", "NOTHING TO UNDO", "USE Y FOR STOCK"
    };
    return now < ui->message_until && ui->message < MSG_COUNT ? messages[ui->message] : "";
}
static PlayEvent feedback(PlayUI *ui, PlayMessage message, PlayEvent event, uint64_t now) {
    ui->message = message;
    ui->message_until = now + PLAY_MESSAGE_MS;
    return event;
}
static void cancel(PlayUI *ui, const KlondikeState *s) {
    if (!ui->held) return;
    if (ui->source.zone == K_TABLEAU) { ui->area = AREA_TABLEAU; ui->focus[AREA_TABLEAU] = ui->source.index; }
    else if (ui->source.zone == K_FOUNDATION) { ui->area = AREA_FOUNDATIONS; ui->focus[AREA_FOUNDATIONS] = ui->source.index; }
    else { ui->area = AREA_DRAW; ui->focus[AREA_DRAW] = 1; }
    select_default(ui, s);
    if (ui->source.zone == K_TABLEAU) ui->selected = ui->source_start;
    ui->held = false;
    ui->invalid_until = 0;
}
static PlayEvent pause_action(PlayUI *ui, KlondikeGame *g, InputAction action) {
    if (action == INPUT_B || action == INPUT_START) {
        if (ui->confirming) { ui->confirming = false; ui->confirm_yes = false; }
        else ui->paused = false;
        return PLAY_NONE;
    }
    if (ui->confirming) {
        if (action == INPUT_LEFT || action == INPUT_RIGHT || action == INPUT_UP || action == INPUT_DOWN)
            ui->confirm_yes = !ui->confirm_yes;
        if (action == INPUT_A) {
            if (ui->confirm_yes) {
                uint32_t seed = ui->seed + 1;
                klondike_new_game(g, seed);
                play_init(ui, &g->state, seed);
                return PLAY_NEW_GAME;
            }
            ui->confirming = false;
        }
        return PLAY_NONE;
    }
    if (action == INPUT_UP) ui->menu = (ui->menu + 2) % 3;
    if (action == INPUT_DOWN) ui->menu = (ui->menu + 1) % 3;
    if (action != INPUT_A) return PLAY_NONE;
    if (ui->menu == 0) ui->paused = false;
    else if (ui->menu == 1) { ui->confirming = true; ui->confirm_yes = false; }
    else { ui->quit = true; return PLAY_QUIT; }
    return PLAY_NONE;
}
PlayEvent play_dispatch(PlayUI *ui, KlondikeGame *g, InputAction action, uint64_t now) {
    if (ui->quit || action >= INPUT_COUNT) return PLAY_NONE;
    if (action == INPUT_QUIT) { ui->quit = true; return PLAY_QUIT; }
    if (ui->paused) return pause_action(ui, g, action);
    if (action == INPUT_START) { ui->paused = true; ui->menu = 0; return PLAY_NONE; }
    if (action == INPUT_L1 || action == INPUT_R1) {
        ui->area = (PlayArea)((ui->area + (action == INPUT_R1 ? 1 : 2)) % AREA_COUNT);
        select_default(ui, &g->state);
        return PLAY_NONE;
    }
    if (action == INPUT_LEFT || action == INPUT_RIGHT || action == INPUT_L2 || action == INPUT_R2) {
        unsigned *col = &ui->focus[ui->area], last = area_size(ui->area) - 1;
        if (action == INPUT_LEFT && *col) --*col;
        if (action == INPUT_RIGHT && *col < last) ++*col;
        if (action == INPUT_L2) *col = 0;
        if (action == INPUT_R2) *col = last;
        select_default(ui, &g->state);
        return PLAY_NONE;
    }
    KlondikePile focus = play_focus(ui);
    const Pile *p = klondike_pile(&g->state, focus);
    if (action == INPUT_UP || action == INPUT_DOWN) {
        if (ui->area == AREA_TABLEAU && p->count) {
            size_t first = play_first_face_up(p);
            if (action == INPUT_UP && ui->selected + 1 < p->count) ++ui->selected;
            if (action == INPUT_DOWN && ui->selected > first) --ui->selected;
        }
        return PLAY_NONE;
    }
    if (action == INPUT_A || (action == INPUT_SELECT && ui->held)) {
        if (ui->held) {
            cancel(ui, &g->state);
            return feedback(ui, MSG_CANCELLED, PLAY_CANCELLED, now);
        }
        if (!g->state.waste.count) return feedback(ui, MSG_EMPTY, PLAY_ERROR, now);
        ui->held = true;
        ui->source = (KlondikePile){K_WASTE, 0};
        ui->source_start = g->state.waste.count - 1;
        return feedback(ui, MSG_PICKED, PLAY_NONE, now);
    }
    if (action == INPUT_B) {
        if (ui->held) {
            if (focus.zone == ui->source.zone && focus.index == ui->source.index) {
                cancel(ui, &g->state);
                return feedback(ui, MSG_CANCELLED, PLAY_CANCELLED, now);
            }
            size_t chosen;
            if (play_preview(ui, &g->state, &chosen) && klondike_drop(g, ui->source, ui->source_start, focus)) {
                ui->held = false; ui->invalid_until = 0;
                select_default(ui, &g->state);
                return feedback(ui, MSG_MOVED, PLAY_COMMITTED, now);
            }
            ui->invalid_until = now + PLAY_INVALID_MS;
            return feedback(ui, MSG_INVALID_DROP, PLAY_ERROR, now);
        }
        size_t start = play_selected(ui, &g->state);
        if (focus.zone == K_STOCK) return feedback(ui, MSG_STOCK, PLAY_ERROR, now);
        if (!pile_at(p, start) || !p->cards[start].face_up) return feedback(ui, MSG_EMPTY, PLAY_ERROR, now);
        ui->held = true; ui->source = focus; ui->source_start = start;
        return feedback(ui, MSG_PICKED, PLAY_NONE, now);
    }
    if ((action == INPUT_X || action == INPUT_Y) && ui->held)
        return feedback(ui, MSG_HOLDING, PLAY_ERROR, now);
    if (action == INPUT_SELECT) {
        if (!klondike_undo(g)) return feedback(ui, MSG_NO_UNDO, PLAY_ERROR, now);
        select_default(ui, &g->state);
        return feedback(ui, MSG_UNDONE, PLAY_COMMITTED, now);
    }
    if (action == INPUT_Y) {
        bool draw = g->state.stock.count != 0;
        bool changed = draw ? klondike_draw(g) : klondike_recycle(g);
        if (!changed) return feedback(ui, MSG_NO_MOVE, PLAY_ERROR, now);
        select_default(ui, &g->state);
        return feedback(ui, draw ? MSG_DRAW : MSG_RECYCLE, PLAY_COMMITTED, now);
    }
    if (action == INPUT_X) {
        size_t start = play_selected(ui, &g->state);
        if ((focus.zone == K_WASTE || focus.zone == K_TABLEAU) && p->count && start == p->count - 1) {
            for (unsigned f = 0; f < 4; ++f)
                if (klondike_move(g, focus, start, (KlondikePile){K_FOUNDATION, f})) {
                    select_default(ui, &g->state);
                    return feedback(ui, MSG_MOVED, PLAY_COMMITTED, now);
                }
        }
        return feedback(ui, MSG_NO_MOVE, PLAY_ERROR, now);
    }
    return PLAY_NONE;
}
