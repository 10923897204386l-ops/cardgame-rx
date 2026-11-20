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
static int top_column(const PlayUI *ui) {
    if (ui->area == AREA_DRAW) return ui->focus[AREA_DRAW] ? 1 : 0;
    if (ui->area == AREA_FOUNDATIONS) return (int)ui->focus[AREA_FOUNDATIONS] + 3;
    return -1;
}
static void focus_top_column(PlayUI *ui, int column) {
    if (column <= 0) { ui->area = AREA_DRAW; ui->focus[AREA_DRAW] = 0; return; }
    if (column == 1 || column == 2) { ui->area = AREA_DRAW; ui->focus[AREA_DRAW] = 1; return; }
    if (column >= 6) column = 6;
    ui->area = AREA_FOUNDATIONS; ui->focus[AREA_FOUNDATIONS] = (unsigned)(column - 3);
}
static size_t nearest_selectable_at_y(const Pile *p, size_t current, int direction) {
    size_t first = play_first_face_up(p);
    if (!p || !p->count || first >= p->count) return 0;
    if (current < first) current = first;
    if (current >= p->count) current = p->count - 1;
    size_t offset = current - first;
    size_t count = p->count - first;
    if (offset >= count) offset = count - 1;
    return direction < 0 ? first + offset : p->count - 1 - (count - 1 - offset);
}
static void focus_tableau_column(PlayUI *ui, const KlondikeState *s, unsigned column, size_t y_hint) {
    if (column > 6) column = 6;
    ui->area = AREA_TABLEAU;
    ui->focus[AREA_TABLEAU] = column;
    const Pile *p = &s->tableau[column];
    ui->selected = nearest_selectable_at_y(p, y_hint, 0);
}
static void spatial_horizontal(PlayUI *ui, const KlondikeState *s, InputAction action) {
    int delta = action == INPUT_LEFT ? -1 : 1;
    if (ui->area == AREA_TABLEAU) {
        unsigned old = ui->focus[AREA_TABLEAU];
        unsigned next = (unsigned)((int)(old + 7) + delta) % 7;
        focus_tableau_column(ui, s, next, ui->selected);
        return;
    }
    int col = top_column(ui);
    col = (col + 7 + delta) % 7;
    if (col == 2) col = delta > 0 ? 3 : 1;
    focus_top_column(ui, col);
}
static void spatial_vertical(PlayUI *ui, const KlondikeState *s, InputAction action) {
    if (ui->area != AREA_TABLEAU) {
        if (action == INPUT_DOWN) {
            int col = top_column(ui);
            focus_tableau_column(ui, s, (unsigned)(col < 0 ? 0 : col > 6 ? 6 : col), 0);
        }
        return;
    }
    const Pile *p = &s->tableau[ui->focus[AREA_TABLEAU]];
    if (!p->count) {
        if (action == INPUT_UP) focus_top_column(ui, (int)ui->focus[AREA_TABLEAU]);
        return;
    }
    size_t first = play_first_face_up(p);
    if (action == INPUT_UP) {
        if (ui->selected + 1 < p->count) ++ui->selected;
        else focus_top_column(ui, (int)ui->focus[AREA_TABLEAU]);
    } else if (ui->selected > first) {
        --ui->selected;
    }
}
void play_init(PlayUI *ui, const KlondikeState *s, uint32_t seed) {
    *ui = (PlayUI){.seed = seed, .screen = MENU_MAIN, .paused = true};
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
static bool action_back(InputAction action) { return action == INPUT_B || action == INPUT_START; }
static void enter_play(PlayUI *ui) {
    ui->screen = MENU_PLAYING;
    ui->paused = false;
    ui->confirming = false;
    ui->confirm_yes = false;
    ui->menu = 0;
}
static PlayEvent deal_new(PlayUI *ui, KlondikeGame *g) {
    uint32_t seed = ui->seed + 1;
    klondike_new_game(g, seed);
    play_init(ui, &g->state, seed);
    enter_play(ui);
    return PLAY_NEW_GAME;
}
static PlayEvent menu_action(PlayUI *ui, KlondikeGame *g, InputAction action) {
    if (action == INPUT_QUIT) { ui->quit = true; return PLAY_QUIT; }
    if (ui->screen == MENU_OPTIONS) {
        if (action_back(action) || action == INPUT_A) {
            ui->screen = ui->return_screen;
            ui->menu = 0;
        }
        return PLAY_NONE;
    }
    if (ui->screen == MENU_CONFIRM_NEW || ui->screen == MENU_CONFIRM_MAIN) {
        if (action_back(action)) { ui->screen = MENU_PAUSE; ui->confirm_yes = false; return PLAY_NONE; }
        if (action == INPUT_LEFT || action == INPUT_RIGHT || action == INPUT_UP || action == INPUT_DOWN)
            ui->confirm_yes = !ui->confirm_yes;
        if (action == INPUT_A) {
            if (ui->confirm_yes) {
                if (ui->screen == MENU_CONFIRM_MAIN) { ui->screen = MENU_MAIN; ui->paused = true; ui->held = false; ui->menu = 0; return PLAY_NONE; }
                return deal_new(ui,g);
            }
            ui->screen = MENU_PAUSE;
            ui->confirm_yes = false;
        }
        return PLAY_NONE;
    }
    unsigned count = ui->screen == MENU_MAIN ? 4 : 4;
    if (action == INPUT_UP) ui->menu = (ui->menu + count - 1) % count;
    if (action == INPUT_DOWN) ui->menu = (ui->menu + 1) % count;
    if (ui->screen == MENU_PAUSE && action_back(action)) { enter_play(ui); return PLAY_NONE; }
    if (action != INPUT_A) return PLAY_NONE;
    if (ui->screen == MENU_MAIN) {
        if (ui->menu == 0) enter_play(ui);
        else if (ui->menu == 1) return deal_new(ui,g);
        else if (ui->menu == 2) { ui->return_screen = MENU_MAIN; ui->screen = MENU_OPTIONS; }
        else { ui->quit = true; return PLAY_QUIT; }
    } else if (ui->screen == MENU_PAUSE) {
        if (ui->menu == 0) enter_play(ui);
        else if (ui->menu == 1) { ui->screen = MENU_CONFIRM_NEW; ui->confirm_yes = false; }
        else if (ui->menu == 2) { ui->return_screen = MENU_PAUSE; ui->screen = MENU_OPTIONS; }
        else { ui->screen = MENU_CONFIRM_MAIN; ui->confirm_yes = false; }
    }
    return PLAY_NONE;
}
static PlayEvent pause_action(PlayUI *ui, KlondikeGame *g, InputAction action) {
    return menu_action(ui,g,action);
}
PlayEvent play_dispatch(PlayUI *ui, KlondikeGame *g, InputAction action, uint64_t now) {
    return play_dispatch_chord(ui,g,action,false,now);
}
PlayEvent play_dispatch_chord(PlayUI *ui, KlondikeGame *g, InputAction action, bool l2_held, uint64_t now) {
    if (ui->quit || action >= INPUT_COUNT) return PLAY_NONE;
    if (action == INPUT_QUIT) { ui->quit = true; return PLAY_QUIT; }
    if (ui->screen != MENU_PLAYING) return pause_action(ui, g, action);
    if (action == INPUT_SELECT && l2_held) {
        if (ui->held) return feedback(ui, MSG_HOLDING, PLAY_ERROR, now);
        if (!klondike_undo(g)) return feedback(ui, MSG_NO_UNDO, PLAY_ERROR, now);
        select_default(ui, &g->state);
        return feedback(ui, MSG_UNDONE, PLAY_COMMITTED, now);
    }
    if (action == INPUT_SELECT) {
        ui->controls_visible = !ui->controls_visible;
        return PLAY_NONE;
    }
    if (action == INPUT_START) { ui->paused = true; ui->screen = MENU_PAUSE; ui->menu = 0; return PLAY_NONE; }
    if (action == INPUT_L1 || action == INPUT_R1) {
        ui->area = (PlayArea)((ui->area + (action == INPUT_R1 ? 1 : 2)) % AREA_COUNT);
        select_default(ui, &g->state);
        return PLAY_NONE;
    }
    if (action == INPUT_LEFT || action == INPUT_RIGHT) {
        spatial_horizontal(ui, &g->state, action);
        return PLAY_NONE;
    }
    if (action == INPUT_L2 || action == INPUT_R2) {
        unsigned *col = &ui->focus[ui->area], last = area_size(ui->area) - 1;
        if (action == INPUT_L2) *col = 0;
        if (action == INPUT_R2) *col = last;
        select_default(ui, &g->state);
        return PLAY_NONE;
    }
    KlondikePile focus = play_focus(ui);
    const Pile *p = klondike_pile(&g->state, focus);
    if (action == INPUT_UP || action == INPUT_DOWN) {
        (void)p;
        spatial_vertical(ui, &g->state, action);
        return PLAY_NONE;
    }
    if (action == INPUT_A) {
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
