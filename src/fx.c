#include "fx.h"
#include <string.h>

static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
static float ease_out(float t) {
    t = clampf(t, 0.f, 1.f);
    return 1.f - (1.f - t) * (1.f - t);
}
static float motion_t(const FxMotion *m, uint64_t now) {
    if (!m->active || !m->duration_ms) return 1.f;
    if (now <= m->start_ms) return 0.f;
    return clampf((float)(now - m->start_ms) / (float)m->duration_ms, 0.f, 1.f);
}
static Rect lerp_rect(Rect a, Rect b, float t) {
    return (Rect){
        a.x + (int)((b.x - a.x) * t + 0.5f),
        a.y + (int)((b.y - a.y) * t + 0.5f),
        a.w + (int)((b.w - a.w) * t + 0.5f),
        a.h + (int)((b.h - a.h) * t + 0.5f)
    };
}

void fx_init(PlayFX *fx, bool enabled) {
    if (!fx) return;
    memset(fx, 0, sizeof *fx);
    fx->enabled = enabled;
}
void fx_set_enabled(PlayFX *fx, bool enabled) {
    if (!fx) return;
    fx->enabled = enabled;
    if (!enabled) fx_clear(fx);
}
bool fx_enabled(const PlayFX *fx) { return fx && fx->enabled; }
void fx_clear(PlayFX *fx) {
    if (!fx) return;
    memset(fx->motions, 0, sizeof fx->motions);
    fx->count = 0;
    fx->trail_count = 0;
}

void fx_push_motion(PlayFX *fx, FxKind kind, Rect from, Rect to, CardState card, uint64_t now, uint32_t ms) {
    if (!fx || !fx->enabled) return;
    unsigned slot = fx->count < FX_MAX_MOTIONS ? fx->count++ : 0;
    if (fx->count > FX_MAX_MOTIONS) fx->count = FX_MAX_MOTIONS;
    /* Prefer free slot. */
    for (unsigned i = 0; i < FX_MAX_MOTIONS; ++i) if (!fx->motions[i].active) { slot = i; break; }
    fx->motions[slot] = (FxMotion){
        .active = true, .kind = kind, .from = from, .to = to, .card = card,
        .start_ms = now, .duration_ms = ms ? ms : 180
    };
    if (slot + 1 > fx->count) fx->count = slot + 1;
}

void fx_note_held(PlayFX *fx, Rect bounds, uint64_t now) {
    if (!fx || !fx->enabled) return;
    if (fx->trail_count && fx->trail[0].x == bounds.x && fx->trail[0].y == bounds.y) return;
    if (now - fx->trail_stamp < 24) return;
    fx->trail_stamp = now;
    for (unsigned i = 5; i > 0; --i) fx->trail[i] = fx->trail[i - 1];
    fx->trail[0] = bounds;
    if (fx->trail_count < 6) ++fx->trail_count;
}

void fx_menu_change(PlayFX *fx, PlayScreen from, PlayScreen to, uint64_t now) {
    if (!fx || !fx->enabled || from == to) return;
    fx->menu_from = from;
    fx->menu_to = to;
    fx->menu_fade_start = now;
}

void fx_set_victory(PlayFX *fx, bool won, uint64_t now) {
    if (!fx) return;
    if (won && !fx->victory) fx->victory_start = now;
    fx->victory = won;
    if (!won) fx->victory_start = 0;
}

void fx_update(PlayFX *fx, uint64_t now) {
    if (!fx || !fx->enabled) return;
    for (unsigned i = 0; i < FX_MAX_MOTIONS; ++i) {
        FxMotion *m = &fx->motions[i];
        if (!m->active) continue;
        if (motion_t(m, now) >= 1.f) m->active = false;
    }
}

bool fx_busy(const PlayFX *fx, uint64_t now) {
    if (!fx || !fx->enabled) return false;
    for (unsigned i = 0; i < FX_MAX_MOTIONS; ++i)
        if (fx->motions[i].active && motion_t(&fx->motions[i], now) < 1.f) return true;
    return false;
}

float fx_menu_fade(const PlayFX *fx, uint64_t now) {
    if (!fx || !fx->enabled || !fx->menu_fade_start) return 1.f;
    float t = clampf((float)(now - fx->menu_fade_start) / 180.f, 0.f, 1.f);
    return ease_out(t);
}

void fx_draw_selection_glow(SDL_Renderer *r, Rect bounds, bool legal, uint64_t now) {
    /* Warm gold pulse; legality still readable via hue shift. */
    int pulse = (int)((now / 40) % 20);
    int expand = 2 + (pulse < 10 ? pulse : 20 - pulse) / 4;
    SDL_Color gold = legal ? (SDL_Color){255, 214, 110, 255} : (SDL_Color){255, 186, 92, 255};
    SDL_Color outer = legal ? (SDL_Color){255, 236, 170, 180} : (SDL_Color){255, 200, 120, 160};
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    render_round_outline(r, (Rect){bounds.x - expand - 1, bounds.y - expand - 1,
                                   bounds.w + 2 * expand + 2, bounds.h + 2 * expand + 2}, 11, 2, outer);
    render_round_outline(r, (Rect){bounds.x - expand, bounds.y - expand,
                                   bounds.w + 2 * expand, bounds.h + 2 * expand}, 10, 3, gold);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void draw_motion(SDL_Renderer *r, const FxMotion *m, BackTheme theme, uint64_t now) {
    float t = ease_out(motion_t(m, now));
    Rect b = lerp_rect(m->from, m->to, t);
    if (m->kind == FX_FLIP) {
        float squash = t < 0.5f ? (1.f - t * 2.f) : ((t - 0.5f) * 2.f);
        int w = (int)(m->to.w * (0.12f + 0.88f * squash) + 0.5f);
        if (w < 4) w = 4;
        b.x = m->to.x + (m->to.w - w) / 2;
        b.w = w;
        CardState c = m->card;
        if (t < 0.5f) c.face_up = !m->card.face_up;
        render_card(r, b, &c, theme);
        return;
    }
    if (m->kind == FX_SETTLE) {
        float bounce = t < 0.7f ? t / 0.7f : 1.f - (t - 0.7f) / 0.3f * 0.15f;
        b.y = m->to.y - (int)((1.f - bounce) * 6.f);
    }
    /* No cream ghost slabs — they smear badly on mali/software compositing. */
    render_card(r, b, &m->card, theme);
}

void fx_render_under(SDL_Renderer *r, const PlayFX *fx, BackTheme theme, uint64_t now) {
    if (!fx || !fx->enabled) return;
    for (unsigned i = 0; i < FX_MAX_MOTIONS; ++i)
        if (fx->motions[i].active) draw_motion(r, &fx->motions[i], theme, now);
}

void fx_render_over(SDL_Renderer *r, const PlayFX *fx, const PlayUI *ui, const Layout *layout,
                    const KlondikeState *state, BackTheme theme, uint64_t now) {
    (void)theme;
    if (!fx || !fx->enabled) return;
    /* Held-card trail breadcrumbs disabled: cream card-shaped slabs looked like artifacts. */
    (void)ui;
    /* Soft menu fade wash. */
    float fade = fx_menu_fade(fx, now);
    if (fade < 0.999f) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, (Uint8)((1.f - fade) * 90));
        SDL_Rect full = {0, 0, 640, 480};
        SDL_RenderFillRect(r, &full);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
    /* Restrained victory shimmer/cascade over foundations. */
    if (fx->victory && state && layout) {
        uint64_t age = now - fx->victory_start;
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        for (unsigned f = 0; f < 4; ++f) {
            Rect slot = layout_slot(layout, f + 3, false);
            int lift = (int)((age / 30 + f * 7) % 12);
            if (lift > 6) lift = 12 - lift;
            render_round_fill(r, (Rect){slot.x - 2, slot.y - lift - 2, slot.w + 4, slot.h + 4}, 10,
                              (SDL_Color){255, 220, 140, 40});
            for (int s = 0; s < 8; ++s) {
                int x = slot.x + 8 + ((int)(age / 16 + f * 13 + s * 9) % (slot.w - 16));
                int y = slot.y + 6 + ((int)(age / 20 + s * 11) % (slot.h - 12));
                render_round_fill(r, (Rect){x, y, 2, 2}, 1, (SDL_Color){255, 245, 200, 160});
            }
        }
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }
}
