#include "play.h"
#include <time.h>
#include "save.h"

static void enter_play(PlayUI *ui);
static void pause_timer(PlayUI *ui, uint64_t now);
static void resume_timer(PlayUI *ui, uint64_t now);

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
void play_set_continue_available(PlayUI *ui, bool available) {
    if (ui) ui->continue_available = available;
}
void play_tick(PlayUI *ui, uint64_t now) {
    if (!ui || ui->screen != MENU_PLAYING || ui->paused || !ui->run_started_ms) return;
    if (now >= ui->run_started_ms) {
        uint64_t delta = now - ui->run_started_ms;
        if (delta > 0) {
            if (ui->elapsed_ms > UINT32_MAX - (uint32_t)delta)
                ui->elapsed_ms = UINT32_MAX;
            else
                ui->elapsed_ms += (uint32_t)delta;
            ui->run_started_ms = now;
        }
    }
}
void play_fill_meta(const PlayUI *ui, SaveMeta *meta) {
    if (!ui || !meta) return;
    *meta = (SaveMeta){
        .seed = ui->seed,
        .score = ui->score,
        .move_count = ui->move_count,
        .elapsed_ms = ui->elapsed_ms,
        .mode = SAVE_MODE_KLONDIKE_DRAW1,
        .saved_at = time(NULL),
        .has_game = true
    };
}
void play_apply_loaded(PlayUI *ui, const SaveMeta *meta, uint64_t now) {
    if (!ui || !meta) return;
    ui->seed = meta->seed;
    ui->score = meta->score;
    ui->move_count = meta->move_count;
    ui->elapsed_ms = meta->elapsed_ms;
    ui->held = false;
    ui->invalid_until = 0;
    ui->dirty = false;
    enter_play(ui);
    ui->run_started_ms = now ? now : 1;
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
    ui->exit_app = false;
}
static void pause_timer(PlayUI *ui, uint64_t now) {
    play_tick(ui, now);
    ui->run_started_ms = 0;
    ui->paused = true;
}
static void resume_timer(PlayUI *ui, uint64_t now) {
    ui->paused = false;
    ui->run_started_ms = now ? now : 1;
}
static void note_move(PlayUI *ui) {
    if (ui->move_count < UINT32_MAX) ++ui->move_count;
    ui->dirty = true;
    /* Simple Vegas-ish score placeholder: +1 per successful commit. */
    if (ui->score < UINT32_MAX) ++ui->score;
}
static PlayEvent deal_new(PlayUI *ui, KlondikeGame *g, uint64_t now) {
    uint32_t seed = ui->seed + 1;
    klondike_new_game(g, seed);
    play_init(ui, &g->state, seed);
    enter_play(ui);
    ui->score = 0;
    ui->move_count = 0;
    ui->elapsed_ms = 0;
    ui->dirty = false;
    resume_timer(ui, now);
    return PLAY_NEW_GAME;
}
static unsigned main_item_count(const PlayUI *ui) {
    /* Continue, New Game, Load Game, Options, Quit */
    (void)ui;
    return 5;
}
static unsigned pause_item_count(void) {
    /* Resume, Save, Load, New Game, Options, Main Menu */
    return 6;
}
static PlayEvent request_autosave_main(PlayUI *ui) {
    ui->exit_app = false;
    return PLAY_SAVE_NEEDED; /* game.c performs autosave then transitions */
}
static PlayEvent request_quit_app(PlayUI *ui) {
    ui->screen = MENU_CONFIRM_QUIT;
    ui->confirm_yes = false;
    ui->exit_app = true;
    ui->quit_choice = 0;
    return PLAY_NONE;
}
static PlayEvent menu_action(PlayUI *ui, KlondikeGame *g, InputAction action, uint64_t now) {
    if (action == INPUT_QUIT) {
        /* Escape still exits, but prefer confirmed path when a game is active. */
        if (ui->screen == MENU_PLAYING || ui->screen == MENU_PAUSE ||
            ui->screen == MENU_SAVE_SLOTS || ui->screen == MENU_LOAD_SLOTS) {
            pause_timer(ui, now);
            return request_quit_app(ui);
        }
        ui->quit = true;
        return PLAY_QUIT;
    }
    if (ui->screen == MENU_CONFIRM_QUIT) {
        if (action_back(action)) {
            if (ui->return_screen == MENU_MAIN || ui->screen == MENU_MAIN)
                ui->screen = MENU_MAIN;
            else {
                ui->screen = MENU_PLAYING;
                resume_timer(ui, now);
            }
            /* Prefer return_screen if set while playing. */
            if (ui->return_screen == MENU_PAUSE) { ui->screen = MENU_PAUSE; ui->paused = true; }
            else if (ui->return_screen == MENU_PLAYING) { enter_play(ui); resume_timer(ui, now); }
            else if (ui->return_screen == MENU_MAIN) { ui->screen = MENU_MAIN; ui->paused = true; }
            return PLAY_NONE;
        }
        if (action == INPUT_LEFT || action == INPUT_RIGHT || action == INPUT_UP || action == INPUT_DOWN)
            ui->confirm_yes = !ui->confirm_yes;
        if (action == INPUT_A) {
            if (ui->confirm_yes) {
                ui->exit_app = true;
                return PLAY_SAVE_NEEDED; /* autosave then quit app */
            }
            if (ui->return_screen == MENU_PAUSE) { ui->screen = MENU_PAUSE; ui->paused = true; }
            else if (ui->return_screen == MENU_MAIN) { ui->screen = MENU_MAIN; ui->paused = true; }
            else { enter_play(ui); resume_timer(ui, now); }
        }
        return PLAY_NONE;
    }
    if (ui->screen == MENU_QUIT_SAVE_ERROR || ui->screen == MENU_SAVE_ERROR) {
        if (action == INPUT_LEFT || action == INPUT_RIGHT || action == INPUT_UP || action == INPUT_DOWN) {
            ui->quit_choice = (ui->quit_choice + (action == INPUT_LEFT || action == INPUT_UP ? 2 : 1)) % 3;
            return PLAY_NONE;
        }
        if (action_back(action)) {
            ui->quit_choice = 1; /* cancel */
        }
        if (action == INPUT_A || action_back(action)) {
            if (ui->quit_choice == 0) return PLAY_SAVE_NEEDED; /* retry */
            if (ui->quit_choice == 2) {
                if (ui->exit_app) { ui->quit = true; return PLAY_QUIT; }
                ui->screen = MENU_MAIN; ui->paused = true; ui->held = false; ui->menu = 0;
                return PLAY_NONE;
            }
            /* cancel */
            if (ui->exit_app) {
                enter_play(ui); resume_timer(ui, now);
            } else {
                ui->screen = MENU_PAUSE; ui->paused = true;
            }
            return PLAY_NONE;
        }
        return PLAY_NONE;
    }
    if (ui->screen == MENU_OPTIONS) {
        if (action_back(action) || action == INPUT_A) {
            ui->screen = ui->return_screen;
            ui->menu = 0;
        }
        return PLAY_NONE;
    }
    if (ui->screen == MENU_CONFIRM_NEW || ui->screen == MENU_CONFIRM_MAIN ||
        ui->screen == MENU_CONFIRM_OVERWRITE || ui->screen == MENU_CONFIRM_LOAD) {
        if (action_back(action)) {
            if (ui->screen == MENU_CONFIRM_OVERWRITE) ui->screen = MENU_SAVE_SLOTS;
            else if (ui->screen == MENU_CONFIRM_LOAD) ui->screen = MENU_LOAD_SLOTS;
            else ui->screen = MENU_PAUSE;
            ui->confirm_yes = false;
            return PLAY_NONE;
        }
        if (action == INPUT_LEFT || action == INPUT_RIGHT || action == INPUT_UP || action == INPUT_DOWN)
            ui->confirm_yes = !ui->confirm_yes;
        if (action == INPUT_A) {
            if (ui->confirm_yes) {
                if (ui->screen == MENU_CONFIRM_MAIN) {
                    return request_autosave_main(ui);
                }
                if (ui->screen == MENU_CONFIRM_NEW) return deal_new(ui, g, now);
                if (ui->screen == MENU_CONFIRM_OVERWRITE) {
                    /* Signal game.c to write selected slot */
                    return PLAY_SAVE_NEEDED;
                }
                if (ui->screen == MENU_CONFIRM_LOAD) {
                    return PLAY_LOADED; /* game.c loads slot */
                }
            }
            if (ui->screen == MENU_CONFIRM_OVERWRITE) ui->screen = MENU_SAVE_SLOTS;
            else if (ui->screen == MENU_CONFIRM_LOAD) ui->screen = MENU_LOAD_SLOTS;
            else ui->screen = MENU_PAUSE;
            ui->confirm_yes = false;
        }
        return PLAY_NONE;
    }
    if (ui->screen == MENU_SAVE_SLOTS || ui->screen == MENU_LOAD_SLOTS) {
        if (action_back(action)) {
            ui->screen = ui->return_screen ? ui->return_screen : MENU_PAUSE;
            ui->menu = 0;
            return PLAY_NONE;
        }
        if (action == INPUT_UP) ui->slot = (ui->slot + SAVE_MANUAL_SLOTS - 1) % SAVE_MANUAL_SLOTS;
        if (action == INPUT_DOWN) ui->slot = (ui->slot + 1) % SAVE_MANUAL_SLOTS;
        if (action != INPUT_A) return PLAY_NONE;
        if (ui->screen == MENU_SAVE_SLOTS) {
            SaveMeta probe;
            ui->slot_occupied = (save_probe_slot(ui->slot, &probe) == SAVE_OK);
            if (ui->slot_occupied) {
                ui->screen = MENU_CONFIRM_OVERWRITE;
                ui->confirm_yes = false;
                return PLAY_NONE;
            }
            return PLAY_SAVE_NEEDED; /* empty slot: save immediately */
        }
        /* Load */
        SaveMeta probe;
        if (save_probe_slot(ui->slot, &probe) != SAVE_OK) return PLAY_NONE;
        if (ui->return_screen == MENU_PAUSE || ui->screen == MENU_LOAD_SLOTS) {
            /* Loading over active game from pause: confirm */
            if (ui->return_screen == MENU_PAUSE) {
                ui->screen = MENU_CONFIRM_LOAD;
                ui->confirm_yes = false;
                return PLAY_NONE;
            }
        }
        return PLAY_LOADED;
    }
    unsigned count = ui->screen == MENU_MAIN ? main_item_count(ui) : pause_item_count();
    if (action == INPUT_UP) ui->menu = (ui->menu + count - 1) % count;
    if (action == INPUT_DOWN) ui->menu = (ui->menu + 1) % count;
    if (ui->screen == MENU_PAUSE && action_back(action)) {
        enter_play(ui); resume_timer(ui, now); return PLAY_NONE;
    }
    if (action != INPUT_A) return PLAY_NONE;
    if (ui->screen == MENU_MAIN) {
        /* 0 Continue, 1 New Game, 2 Load Game, 3 Options, 4 Quit */
        if (ui->menu == 0) {
            if (ui->continue_available) return PLAY_LOADED; /* load autosave */
            return PLAY_NONE;
        }
        if (ui->menu == 1) return deal_new(ui, g, now);
        if (ui->menu == 2) {
            ui->return_screen = MENU_MAIN;
            ui->screen = MENU_LOAD_SLOTS;
            ui->slot = 0;
            return PLAY_NONE;
        }
        if (ui->menu == 3) { ui->return_screen = MENU_MAIN; ui->screen = MENU_OPTIONS; }
        else { ui->quit = true; return PLAY_QUIT; }
    } else if (ui->screen == MENU_PAUSE) {
        /* 0 Resume, 1 Save, 2 Load, 3 New Game, 4 Options, 5 Main Menu */
        if (ui->menu == 0) { enter_play(ui); resume_timer(ui, now); }
        else if (ui->menu == 1) {
            ui->return_screen = MENU_PAUSE;
            ui->screen = MENU_SAVE_SLOTS;
            ui->slot = 0;
        } else if (ui->menu == 2) {
            ui->return_screen = MENU_PAUSE;
            ui->screen = MENU_LOAD_SLOTS;
            ui->slot = 0;
        } else if (ui->menu == 3) { ui->screen = MENU_CONFIRM_NEW; ui->confirm_yes = false; }
        else if (ui->menu == 4) { ui->return_screen = MENU_PAUSE; ui->screen = MENU_OPTIONS; }
        else { ui->screen = MENU_CONFIRM_MAIN; ui->confirm_yes = false; }
    }
    return PLAY_NONE;
}
static PlayEvent pause_action(PlayUI *ui, KlondikeGame *g, InputAction action, uint64_t now) {
    return menu_action(ui, g, action, now);
}
PlayEvent play_dispatch(PlayUI *ui, KlondikeGame *g, InputAction action, uint64_t now) {
    return play_dispatch_chord(ui, g, action, false, false, false, now);
}
PlayEvent play_dispatch_chord(PlayUI *ui, KlondikeGame *g, InputAction action,
                              bool l2_held, bool select_held, bool start_held, uint64_t now) {
    if (ui->quit || action >= INPUT_COUNT) return PLAY_NONE;
    play_tick(ui, now);

    /* Global Select+Start quit chord — highest precedence after dialogs that
       already own Start/Select. Never fires while L2+Select undo is intended
       without Start, and never fires inside an open confirmation that uses A/B. */
    bool chord_ok_screen = ui->screen == MENU_PLAYING || ui->screen == MENU_MAIN ||
                           ui->screen == MENU_PAUSE;
    if (chord_ok_screen && !l2_held) {
        if (action == INPUT_SELECT && start_held) {
            ui->return_screen = ui->screen;
            pause_timer(ui, now);
            return request_quit_app(ui);
        }
        if (action == INPUT_START && select_held) {
            ui->return_screen = ui->screen;
            pause_timer(ui, now);
            return request_quit_app(ui);
        }
    }

    if (action == INPUT_QUIT) {
        if (ui->screen == MENU_MAIN) { ui->quit = true; return PLAY_QUIT; }
        ui->return_screen = ui->screen == MENU_PLAYING ? MENU_PLAYING : ui->screen;
        pause_timer(ui, now);
        return request_quit_app(ui);
    }
    if (ui->screen != MENU_PLAYING) return pause_action(ui, g, action, now);
    if (action == INPUT_SELECT && l2_held) {
        if (ui->held) return feedback(ui, MSG_HOLDING, PLAY_ERROR, now);
        if (!klondike_undo(g)) return feedback(ui, MSG_NO_UNDO, PLAY_ERROR, now);
        select_default(ui, &g->state);
        if (ui->move_count) --ui->move_count;
        ui->dirty = true;
        return feedback(ui, MSG_UNDONE, PLAY_COMMITTED, now);
    }
    if (action == INPUT_SELECT) {
        ui->controls_visible = !ui->controls_visible;
        return PLAY_NONE;
    }
    if (action == INPUT_START) {
        pause_timer(ui, now);
        ui->screen = MENU_PAUSE;
        ui->menu = 0;
        return PLAY_NONE;
    }
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
                note_move(ui);
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
        note_move(ui);
        return feedback(ui, draw ? MSG_DRAW : MSG_RECYCLE, PLAY_COMMITTED, now);
    }
    if (action == INPUT_X) {
        size_t start = play_selected(ui, &g->state);
        if ((focus.zone == K_WASTE || focus.zone == K_TABLEAU) && p->count && start == p->count - 1) {
            for (unsigned f = 0; f < 4; ++f)
                if (klondike_move(g, focus, start, (KlondikePile){K_FOUNDATION, f})) {
                    select_default(ui, &g->state);
                    note_move(ui);
                    return feedback(ui, MSG_MOVED, PLAY_COMMITTED, now);
                }
        }
        return feedback(ui, MSG_NO_MOVE, PLAY_ERROR, now);
    }
    return PLAY_NONE;
}
