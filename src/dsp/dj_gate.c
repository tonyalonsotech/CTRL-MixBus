#include "dj_gate.h"
#include <string.h>

static float coef_ms(float fs, float ms) { return ms <= 0.0f ? 1.0f : 1.0f - expf(-1.0f / (0.001f * ms * fs)); }

void dj_gate_init(dj_gate_t *g, float fs)
{
    memset(g, 0, sizeof *g);
    g->fs = fs;
    g->det_rel = coef_ms(fs, 20.0f);
    g->sm_a = coef_ms(fs, DJ_GATE_FLIP_SMOOTH_MS);
    dj_gate_set(g, -60.0f, 1.0f, 50.0f, 100.0f);
    g->gain = 1.0f;
    g->open = 1;
}

void dj_gate_set(dj_gate_t *g, float thr_db, float attack_ms, float hold_ms, float release_ms)
{
    g->thr_open = dj_db_to_lin(thr_db);
    g->thr_close = dj_db_to_lin(thr_db - DJ_GATE_HYST_DB);
    g->att_a = coef_ms(g->fs, dj_clampf(attack_ms, 0.01f, 1000.0f));
    g->rel_a = coef_ms(g->fs, dj_clampf(release_ms, 0.1f, 10000.0f));
    g->hold_samples = (int)(0.001f * dj_clampf(hold_ms, 0.0f, 5000.0f) * g->fs);
}

void dj_gate_reset(dj_gate_t *g)
{
    g->env = 0.0f;
    g->gain = 1.0f;
    g->open = !g->flip;              /* start passing either way: a normal gate as if just opened, a flipped one as if under */
    g->hold_left = 0;
    for (int k = 0; k < DJ_GATE_FLIP_SMOOTH_STAGES; k++) g->sm[k] = 1.0f;
}

void dj_gate_set_flip(dj_gate_t *g, int flip) { g->flip = flip ? 1 : 0; }

void dj_gate_process(dj_gate_t *g, float *l, float *r, int n)
{
    for (int i = 0; i < n; i++) {
        float a = fmaxf(fabsf(l[i]), fabsf(r[i]));           /* linked detector */
        g->env = a > g->env ? a : g->env + g->det_rel * (a - g->env);

        if (g->env >= g->thr_open) {
            g->open = 1;
            g->hold_left = g->hold_samples;
        } else if (g->open && g->env < g->thr_close) {
            if (g->hold_left > 0) g->hold_left--;
            else g->open = 0;
        }

        float target = (g->open != g->flip) ? 1.0f : 0.0f;
        float c = g->open ? g->att_a : g->rel_a;     /* attack after the signal goes over, release after it falls under */
        g->gain += c * (target - g->gain);
        if (g->gain < 1e-5f && target == 0.0f) g->gain = 0.0f;
        if (g->gain > 0.99999f && target == 1.0f) g->gain = 1.0f;

        float out = g->gain;
        if (g->flip) {                                /* FLIP: the S-curve (each stage follows the one before) */
            for (int k = 0; k < DJ_GATE_FLIP_SMOOTH_STAGES; k++) { g->sm[k] += g->sm_a * (out - g->sm[k]); out = g->sm[k]; }
            if (out > 0.99999f && g->gain == 1.0f) out = 1.0f;
            if (out < 1e-5f && g->gain == 0.0f) out = 0.0f;
        } else {
            for (int k = 0; k < DJ_GATE_FLIP_SMOOTH_STAGES; k++) g->sm[k] = g->gain;   /* ready, so switching FLIP on does not jump */
        }
        l[i] *= out;
        r[i] *= out;
    }
}
