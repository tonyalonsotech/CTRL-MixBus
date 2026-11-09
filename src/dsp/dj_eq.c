#include "dj_eq.h"
#include <string.h>

#define BUTTER_K 1.41421356237309505   /* 1/Q, Q = sqrt(1/2) */

float dj_eq_knob_to_db(float n, dj_eq_mode_t m)
{
    n = dj_clampf(n, 0.0f, 1.0f);
    if (n >= 0.5f) return (float)DJ_EQ_RANGE_BOOST_DB * (2.0f * n - 1.0f);
    if (m == DJ_EQ_MODE_ISOLATOR) {
        float g = (2.0f * n) * (2.0f * n);
        return g <= 1.5849e-5f ? -200.0f : 20.0f * log10f(g);
    }
    return -(float)DJ_EQ_RANGE_CUT_DB * (1.0f - 2.0f * n);
}

float dj_eq_knob_to_lin(float n, dj_eq_mode_t m)
{
    n = dj_clampf(n, 0.0f, 1.0f);
    if (n >= 0.5f) return powf(10.0f, (float)DJ_EQ_RANGE_BOOST_DB * (2.0f * n - 1.0f) * 0.05f);
    if (m == DJ_EQ_MODE_ISOLATOR) return (2.0f * n) * (2.0f * n);
    return powf(10.0f, -(float)DJ_EQ_RANGE_CUT_DB * (1.0f - 2.0f * n) * 0.05f);
}

/* ---- RBJ shelf / bell coefficients (double) ---- */
static void rbj_low_shelf(dj_biquad_t *q, double fs, double f, double db)
{
    double A = pow(10.0, db / 40.0), w = 2.0 * DJ_PI * f / fs, cw = cos(w), sw = sin(w);
    double al = sw / 1.41421356237309505, sA = sqrt(A);
    double b0 = A * ((A + 1) - (A - 1) * cw + 2 * sA * al), b1 = 2 * A * ((A - 1) - (A + 1) * cw),
           b2 = A * ((A + 1) - (A - 1) * cw - 2 * sA * al);
    double a0 = (A + 1) + (A - 1) * cw + 2 * sA * al, a1 = -2 * ((A - 1) + (A + 1) * cw),
           a2 = (A + 1) + (A - 1) * cw - 2 * sA * al;
    q->b0 = b0 / a0; q->b1 = b1 / a0; q->b2 = b2 / a0; q->a1 = a1 / a0; q->a2 = a2 / a0;
}
static void rbj_high_shelf(dj_biquad_t *q, double fs, double f, double db)
{
    double A = pow(10.0, db / 40.0), w = 2.0 * DJ_PI * f / fs, cw = cos(w), sw = sin(w);
    double al = sw / 1.41421356237309505, sA = sqrt(A);
    double b0 = A * ((A + 1) + (A - 1) * cw + 2 * sA * al), b1 = -2 * A * ((A - 1) + (A + 1) * cw),
           b2 = A * ((A + 1) + (A - 1) * cw - 2 * sA * al);
    double a0 = (A + 1) - (A - 1) * cw + 2 * sA * al, a1 = 2 * ((A - 1) - (A + 1) * cw),
           a2 = (A + 1) - (A - 1) * cw - 2 * sA * al;
    q->b0 = b0 / a0; q->b1 = b1 / a0; q->b2 = b2 / a0; q->a1 = a1 / a0; q->a2 = a2 / a0;
}
static void rbj_peak(dj_biquad_t *q, double fs, double f, double Q, double db)
{
    double A = pow(10.0, db / 40.0), w = 2.0 * DJ_PI * f / fs, cw = cos(w), al = sin(w) / (2.0 * Q);
    double a0 = 1 + al / A;
    q->b0 = (1 + al * A) / a0; q->b1 = -2 * cw / a0; q->b2 = (1 - al * A) / a0;
    q->a1 = -2 * cw / a0;       q->a2 = (1 - al / A) / a0;
}

static void eq_coefs(dj_eq_t *s)
{
    for (int c = 0; c < 2; c++) {
        rbj_low_shelf(&s->ls[c], s->fs, DJ_EQ_LOWSHELF_HZ, s->eq_db[0].v);
        rbj_peak(&s->pk[c], s->fs, DJ_EQ_MIDBELL_HZ, DJ_EQ_MIDBELL_Q, s->eq_db[1].v);
        rbj_high_shelf(&s->hs[c], s->fs, DJ_EQ_HIGHSHELF_HZ, s->eq_db[2].v);
    }
}

void dj_eq_init(dj_eq_t *s, float fs)
{
    memset(s, 0, sizeof *s);
    s->fs = fs;
    s->mode = s->target_mode = DJ_EQ_MODE_ISOLATOR;
    s->xf_step = 1.0f / (0.020f * fs);
    for (int b = 0; b < 3; b++) {
        s->n[b] = 0.5f;
        dj_smooth_init(&s->iso_g[b], fs, 5.0f, 1.0f);
        dj_smooth_init(&s->eq_db[b], fs, 8.0f, 0.0f);
    }
    dj_svf_coef(&s->c1, dj_svf_g(DJ_EQ_ISO_XOVER_LOW_HZ, fs), BUTTER_K);
    dj_svf_coef(&s->c2, dj_svf_g(DJ_EQ_ISO_XOVER_HIGH_HZ, fs), BUTTER_K);
    eq_coefs(s);
}

void dj_eq_reset(dj_eq_t *s)
{
    memset(s->s1lp, 0, sizeof s->s1lp); memset(s->s1hp, 0, sizeof s->s1hp);
    memset(s->s2lp, 0, sizeof s->s2lp); memset(s->s2hp, 0, sizeof s->s2hp);
    memset(s->aplp, 0, sizeof s->aplp); memset(s->aphp, 0, sizeof s->aphp);
    for (int c = 0; c < 2; c++) { s->ls[c].z1 = s->ls[c].z2 = s->pk[c].z1 = s->pk[c].z2 = s->hs[c].z1 = s->hs[c].z2 = 0; }
    for (int b = 0; b < 3; b++) { s->iso_g[b].v = s->iso_g[b].t; s->eq_db[b].v = s->eq_db[b].t; }
    s->mode = s->target_mode;
    s->xfading = 0;
    eq_coefs(s);
}

void dj_eq_set_mode(dj_eq_t *s, dj_eq_mode_t m)
{
    if (m == s->target_mode) return;
    s->target_mode = m;
    s->xf = 0.0f;
    s->xfading = 1;     /* both paths run for ~20 ms while the old one fades out */
}

void dj_eq_set(dj_eq_t *s, float low_n, float mid_n, float high_n)
{
    s->n[0] = low_n; s->n[1] = mid_n; s->n[2] = high_n;
    for (int b = 0; b < 3; b++) {
        dj_smooth_set(&s->iso_g[b], dj_eq_knob_to_lin(s->n[b], DJ_EQ_MODE_ISOLATOR));
        dj_smooth_set(&s->eq_db[b], dj_eq_knob_to_db(s->n[b], DJ_EQ_MODE_EQ));
    }
}

static void iso_tick(dj_eq_t *s, double xl, double xr, double *ol, double *or_)
{
    double x[2] = { xl, xr }, out[2];
    double gl = s->iso_g[0].v, gm = s->iso_g[1].v, gh = s->iso_g[2].v;
    for (int c = 0; c < 2; c++) {
        double a, b, j, low, rest, mid, high, lowc;
        /* crossover 1: LR4 */
        dj_svf_tick(&s->s1lp[c][0], &s->c1, x[c], &a, &j);
        dj_svf_tick(&s->s1lp[c][1], &s->c1, a, &low, &j);
        dj_svf_tick(&s->s1hp[c][0], &s->c1, x[c], &j, &b);
        dj_svf_tick(&s->s1hp[c][1], &s->c1, b, &j, &rest);
        /* crossover 2 on the upper split */
        dj_svf_tick(&s->s2lp[c][0], &s->c2, rest, &a, &j);
        dj_svf_tick(&s->s2lp[c][1], &s->c2, a, &mid, &j);
        dj_svf_tick(&s->s2hp[c][0], &s->c2, rest, &j, &b);
        dj_svf_tick(&s->s2hp[c][1], &s->c2, b, &j, &high);
        /* the low band must see crossover 2's phase response too: LR4 low + high = all-pass */
        double lp, hp;
        dj_svf_tick(&s->aplp[c][0], &s->c2, low, &a, &j);
        dj_svf_tick(&s->aplp[c][1], &s->c2, a, &lp, &j);
        dj_svf_tick(&s->aphp[c][0], &s->c2, low, &j, &b);
        dj_svf_tick(&s->aphp[c][1], &s->c2, b, &j, &hp);
        lowc = lp + hp;
        out[c] = gl * lowc + gm * mid + gh * high;
    }
    *ol = out[0]; *or_ = out[1];
}

static void eq_tick(dj_eq_t *s, double xl, double xr, double *ol, double *or_)
{
    double x[2] = { xl, xr }, out[2];
    for (int c = 0; c < 2; c++) {
        double y = dj_biquad_tick(&s->ls[c], x[c]);
        y = dj_biquad_tick(&s->pk[c], y);
        out[c] = dj_biquad_tick(&s->hs[c], y);
    }
    *ol = out[0]; *or_ = out[1];
}

void dj_eq_process(dj_eq_t *s, float *l, float *r, int n)
{
    for (int i = 0; i < n; i++) {
        int moving = 0;
        for (int b = 0; b < 3; b++) {
            dj_smooth_next(&s->iso_g[b]);
            dj_smooth_next(&s->eq_db[b]);
            moving |= !dj_smooth_settled(&s->eq_db[b]);
        }
        if (s->count == 0) {
            if (moving || s->first == 0 || s->xfading) { eq_coefs(s); s->first = 1; }
            s->count = DJ_COEF_INTERVAL;
        }
        s->count--;

        double ol = 0.0, orr = 0.0;
        if (!s->xfading) {
            if (s->mode == DJ_EQ_MODE_ISOLATOR) iso_tick(s, l[i], r[i], &ol, &orr);
            else eq_tick(s, l[i], r[i], &ol, &orr);
        } else {
            double il, ir, el, er;
            iso_tick(s, l[i], r[i], &il, &ir);
            eq_tick(s, l[i], r[i], &el, &er);
            float t = s->xf;                                   /* 0 = old mode, 1 = new mode */
            int old_iso = (s->mode == DJ_EQ_MODE_ISOLATOR);
            double oldl = old_iso ? il : el, oldr = old_iso ? ir : er;
            double newl = old_iso ? el : il, newr = old_iso ? er : ir;
            ol = oldl + (newl - oldl) * t;
            orr = oldr + (newr - oldr) * t;
            s->xf += s->xf_step;
            if (s->xf >= 1.0f) { s->mode = s->target_mode; s->xfading = 0; }
        }
        l[i] = (float)ol;
        r[i] = (float)orr;
    }
}
