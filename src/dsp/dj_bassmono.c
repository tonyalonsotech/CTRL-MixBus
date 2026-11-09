#include "dj_bassmono.h"
#include <string.h>

#define BUTTER_Q 0.70710678118654752

void dj_bassmono_init(dj_bassmono_t *s, float fs)
{
    memset(s, 0, sizeof *s);
    s->fs = fs;
    s->k = 1.0 / BUTTER_Q;
    dj_smooth_init(&s->freq, fs, 10.0f, 120.0f);
    s->count = 0;
    dj_svf_coef(&s->co, dj_svf_g(120.0, fs), s->k);
}

void dj_bassmono_set_freq(dj_bassmono_t *s, float hz) { dj_smooth_set(&s->freq, dj_clampf(hz, 20.0f, 1000.0f)); }

void dj_bassmono_reset(dj_bassmono_t *s)
{
    memset(s->lp, 0, sizeof s->lp);
    memset(s->hp, 0, sizeof s->hp);
    s->freq.v = s->freq.t;
    dj_svf_coef(&s->co, dj_svf_g(s->freq.v, s->fs), s->k);
}

void dj_bassmono_process(dj_bassmono_t *s, float *l, float *r, int n)
{
    for (int i = 0; i < n; i++) {
        if (s->count == 0) {
            /* refresh coefficients on a sub-block grid, only while the control is still moving */
            if (!dj_smooth_settled(&s->freq) || s->g_cache == 0.0) {
                dj_svf_coef(&s->co, dj_svf_g(s->freq.v, s->fs), s->k);
                s->g_cache = 1.0;
            }
            s->count = DJ_COEF_INTERVAL;
        }
        s->count--;
        dj_smooth_next(&s->freq);

        double x[2] = { l[i], r[i] }, low[2], high[2];
        for (int c = 0; c < 2; c++) {
            double lp1, hp1, lp2, hp2, junk;
            dj_svf_tick(&s->lp[c][0], &s->co, x[c], &lp1, &junk);   /* LP stage 1 */
            dj_svf_tick(&s->lp[c][1], &s->co, lp1, &lp2, &junk);    /* LP stage 2 -> LR4 low */
            dj_svf_tick(&s->hp[c][0], &s->co, x[c], &junk, &hp1);   /* HP stage 1 */
            dj_svf_tick(&s->hp[c][1], &s->co, hp1, &junk, &hp2);    /* HP stage 2 -> LR4 high */
            low[c] = lp2;
            high[c] = hp2;
        }
        double mono = 0.5 * (low[0] + low[1]);
        l[i] = (float)(mono + high[0]);
        r[i] = (float)(mono + high[1]);
    }
}
