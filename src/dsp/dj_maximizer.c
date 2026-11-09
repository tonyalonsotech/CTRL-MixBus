#include "dj_maximizer.h"
#include <string.h>

#define SMOOTH_MS 10.0f

static double coef_ms(float fs, float ms) { return ms <= 0.0f ? 1.0 : 1.0 - exp(-1.0 / (0.001 * (double)ms * (double)fs)); }

void dj_max_init(dj_maximizer_t *m, float fs)
{
    memset(m, 0, sizeof *m);
    m->fs = fs;
    dj_smooth_init(&m->mk, fs, SMOOTH_MS, 1.0f);
    dj_smooth_init(&m->out, fs, SMOOTH_MS, 1.0f);
    dj_max_set(m, 0.0f, 0.05f, 100.0f, 0.0f);
    dj_max_reset(m);
}

void dj_max_set(dj_maximizer_t *m, float thr_db, float attack_ms, float release_ms, float out_db)
{
    thr_db = dj_clampf(thr_db, -60.0f, 0.0f);
    out_db = dj_clampf(out_db, -200.0f, 0.0f);        /* the output can only lower the ceiling */
    dj_smooth_set(&m->mk, dj_db_to_lin(-thr_db));
    dj_smooth_set(&m->out, dj_db_to_lin(out_db));
    m->att_a = coef_ms(m->fs, dj_clampf(attack_ms, 0.005f, 100.0f));
    m->rel_a = coef_ms(m->fs, dj_clampf(release_ms, 1.0f, 10000.0f));
}

void dj_max_reset(dj_maximizer_t *m)
{
    m->mk.v = m->mk.t;
    m->out.v = m->out.t;
    m->g = 1.0f;
    m->in_pk[0] = m->in_pk[1] = 0.0f;
    m->g_min = 1.0f;
}

void dj_max_process(dj_maximizer_t *m, float *l, float *r, int n)
{
    for (int i = 0; i < n; i++) {
        float mk = dj_smooth_next(&m->mk);
        float a = l[i] * mk, b = r[i] * mk;
        if (a != a) a = 0.0f;                         /* NaN in: silence out, never a stuck gain */
        if (b != b) b = 0.0f;
        float pa = fabsf(a), pb = fabsf(b);
        if (pa > m->in_pk[0]) m->in_pk[0] = pa;
        if (pb > m->in_pk[1]) m->in_pk[1] = pb;

        float pk = pa > pb ? pa : pb;                 /* linked detector */
        double need = pk > 1.0f ? 1.0 / (double)pk : 1.0;   /* the gain that puts this peak exactly on the ceiling */
        if (need < m->g) m->g += m->att_a * (need - m->g);
        else { m->g += m->rel_a * (1.0 - m->g); if (m->g > 1.0 - 1e-7) m->g = 1.0; }   /* settle EXACTLY: back to a pure pass-through */
        if (m->g < m->g_min) m->g_min = m->g;

        float g = (float)m->g;
        float ya = a * g, yb = b * g;
        ya = ya > 1.0f ? 1.0f : ya < -1.0f ? -1.0f : ya;       /* the ceiling: 0 dBFS, always */
        yb = yb > 1.0f ? 1.0f : yb < -1.0f ? -1.0f : yb;
        float o = dj_smooth_next(&m->out);
        l[i] = ya * o;
        r[i] = yb * o;
    }
}

void dj_max_take_stats(dj_maximizer_t *m, float *in_pk_l, float *in_pk_r, float *gr_db)
{
    *in_pk_l = m->in_pk[0];
    *in_pk_r = m->in_pk[1];
    *gr_db = m->g_min >= 1.0 ? 0.0f : -dj_lin_to_db((float)m->g_min);
    m->in_pk[0] = m->in_pk[1] = 0.0f;
    m->g_min = m->g;                                  /* the next block starts from where the gain is NOW */
}
