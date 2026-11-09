#include "dj_gate.h"
#include <string.h>

static float coef_ms(float fs, float ms) { return ms <= 0.0f ? 1.0f : 1.0f - expf(-1.0f / (0.001f * ms * fs)); }

void dj_gate_init(dj_gate_t *g, float fs)
{
    memset(g, 0, sizeof *g);
    g->fs = fs;
    g->det_rel = coef_ms(fs, 20.0f);
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
    g->open = 1;
    g->hold_left = 0;
}

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

        float target = g->open ? 1.0f : 0.0f;
        float c = target > g->gain ? g->att_a : g->rel_a;
        g->gain += c * (target - g->gain);
        if (g->gain < 1e-5f && target == 0.0f) g->gain = 0.0f;
        if (g->gain > 0.99999f && target == 1.0f) g->gain = 1.0f;

        l[i] *= g->gain;
        r[i] *= g->gain;
    }
}
