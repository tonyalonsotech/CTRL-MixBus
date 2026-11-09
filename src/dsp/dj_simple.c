#include "dj_simple.h"

/* ---- gain ---- */
void dj_gain_init(dj_gain_t *s, float fs) { dj_smooth_init(&s->g, fs, 5.0f, 1.0f); }
void dj_gain_set_db(dj_gain_t *s, float db) { dj_smooth_set(&s->g, dj_db_to_lin(db)); }
void dj_gain_process(dj_gain_t *s, float *l, float *r, int n)
{
    for (int i = 0; i < n; i++) {
        float g = dj_smooth_next(&s->g);
        l[i] *= g;
        r[i] *= g;
    }
}
void dj_gain_reset(dj_gain_t *s) { s->g.v = s->g.t; }

/* ---- phase ---- */
void dj_phase_init(dj_phase_t *s, float fs) { dj_smooth_init(&s->sign, fs, 0.7f, 1.0f); }
void dj_phase_set(dj_phase_t *s, int inverted) { dj_smooth_set(&s->sign, inverted ? -1.0f : 1.0f); }
void dj_phase_process(dj_phase_t *s, float *l, float *r, int n)
{
    for (int i = 0; i < n; i++) {
        float g = dj_smooth_next(&s->sign);
        l[i] *= g;
        r[i] *= g;
    }
}
void dj_phase_reset(dj_phase_t *s) { s->sign.v = s->sign.t; }

/* ---- width ---- */
void dj_width_init(dj_width_t *s, float fs) { dj_smooth_init(&s->w, fs, 5.0f, 1.0f); }
void dj_width_set(dj_width_t *s, float w) { dj_smooth_set(&s->w, dj_clampf(w, 0.0f, 1.0f)); }
void dj_width_process(dj_width_t *s, float *l, float *r, int n)
{
    for (int i = 0; i < n; i++) {
        float w = dj_smooth_next(&s->w);
        float m = 0.5f * (l[i] + r[i]);
        float d = 0.5f * (l[i] - r[i]) * w;
        l[i] = m + d;
        r[i] = m - d;
    }
}
void dj_width_reset(dj_width_t *s) { s->w.v = s->w.t; }

/* ---- balance ---- */
void dj_balance_init(dj_balance_t *s, float fs)
{
    dj_smooth_init(&s->gl, fs, 5.0f, 1.0f);
    dj_smooth_init(&s->gr, fs, 5.0f, 1.0f);
}
void dj_balance_set(dj_balance_t *s, float bal)
{
    bal = dj_clampf(bal, -1.0f, 1.0f);
    dj_smooth_set(&s->gl, bal > 0.0f ? 1.0f - bal : 1.0f);
    dj_smooth_set(&s->gr, bal < 0.0f ? 1.0f + bal : 1.0f);
}
void dj_balance_process(dj_balance_t *s, float *l, float *r, int n)
{
    for (int i = 0; i < n; i++) {
        l[i] *= dj_smooth_next(&s->gl);
        r[i] *= dj_smooth_next(&s->gr);
    }
}
void dj_balance_reset(dj_balance_t *s) { s->gl.v = s->gl.t; s->gr.v = s->gr.t; }
