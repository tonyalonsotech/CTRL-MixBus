#include "dj_filter.h"
#include <string.h>

static const int SECTIONS[DJ_FLT_NUM_CURVES] = { 1, 2, 3, 4, 8, 16 };

int dj_filter_sections_for_curve(int curve)
{
    if (curve < 0) curve = 0;
    if (curve >= DJ_FLT_NUM_CURVES) curve = DJ_FLT_NUM_CURVES - 1;
    return SECTIONS[curve];
}

float dj_filter_cut_hz(float n) { return 20.0f * powf(1000.0f, dj_clampf(n, 0.0f, 1.0f)); }

void dj_filter_init(dj_filter_t *f, float fs, dj_flt_type_t type)
{
    memset(f, 0, sizeof *f);
    f->fs = fs;
    f->type = type;
    /* Butterworth section Qs for order N = 2m:  Q_k = 1 / (2 sin((2k-1) pi / (2N))), k = 1..m (k = 1 is the highest Q) */
    for (int c = 0; c < DJ_FLT_NUM_CURVES; c++) {
        int m = SECTIONS[c], N = 2 * m;
        for (int k = 1; k <= m; k++) f->q[c][k - 1] = 1.0 / (2.0 * sin((2.0 * k - 1.0) * DJ_PI / (2.0 * N)));
    }
    f->curve = f->pending_curve = 0;
    f->sections = SECTIONS[0];
    f->next_curve = -1;
    f->warm_len = (int)(DJ_FLT_WARM_MS * 0.001f * fs + 0.5f);
    f->fade_len = (int)(DJ_FLT_FADE_MS * 0.001f * fs + 0.5f);
    dj_smooth_init(&f->cut, fs, 15.0f, type == DJ_FLT_LP ? 1.0f : 0.0f);
    dj_smooth_init(&f->res, fs, 15.0f, 0.0f);
    f->dirty = 1;
}

void dj_filter_set(dj_filter_t *f, float cut01, float res01, int curve)
{
    dj_smooth_set(&f->cut, dj_clampf(cut01, 0.0f, 1.0f));
    dj_smooth_set(&f->res, dj_clampf(res01, 0.0f, 1.0f));
    if (curve < 0) curve = 0;
    if (curve >= DJ_FLT_NUM_CURVES) curve = DJ_FLT_NUM_CURVES - 1;
    f->pending_curve = curve;
}

void dj_filter_reset(dj_filter_t *f)
{
    memset(f->st, 0, sizeof f->st);
    f->cut.v = f->cut.t;
    f->res.v = f->res.t;
    memset(f->st2, 0, sizeof f->st2);
    f->curve = f->pending_curve;
    f->sections = SECTIONS[f->curve];
    f->next_curve = -1;
    f->dirty = 1;
}

static void coefs(const dj_filter_t *f, int curve, int sections, double g, double boost, dj_svf_c_t *co)
{
    for (int k = 0; k < sections; k++) {
        double q = f->q[curve][k] * (k == 0 ? boost : 1.0);
        dj_svf_coef(&co[k], g, 1.0 / q);
    }
}

static void refresh(dj_filter_t *f)
{
    double fc = dj_filter_cut_hz(f->cut.v), g = dj_svf_g(fc, f->fs);
    double boost = pow(10.0, (double)f->res.v * DJ_FLT_RES_MAX_DB / 20.0);
    coefs(f, f->curve, f->sections, g, boost, f->co);
    if (f->next_curve >= 0) coefs(f, f->next_curve, f->next_sections, g, boost, f->co2);
}

static double cascade(dj_svf_t *st, const dj_svf_c_t *co, int sections, int hp, double v)
{
    double lp, h;
    for (int k = 0; k < sections; k++) {
        dj_svf_tick(&st[k], &co[k], v, &lp, &h);
        v = hp ? h : lp;
    }
    return v;
}

static void begin_change(dj_filter_t *f, int curve)
{
    f->next_curve = curve;
    f->next_sections = SECTIONS[curve];
    memset(f->st2, 0, sizeof f->st2);
    f->xf_n = 0;
    refresh(f);                       /* the new sections need their coefficients from the first sample */
}

void dj_filter_process(dj_filter_t *f, float *l, float *r, int n)
{
    const int hp = (f->type == DJ_FLT_HP);
    for (int i = 0; i < n; i++) {
        dj_smooth_next(&f->cut);
        dj_smooth_next(&f->res);

        /* slope change: start the new slope beside the old one (or re-target it while it is still warming up) */
        if (f->next_curve < 0) {
            if (f->pending_curve != f->curve) begin_change(f, f->pending_curve);
        } else if (f->pending_curve != f->next_curve && f->xf_n < f->warm_len) {
            if (f->pending_curve == f->curve) f->next_curve = -1;      /* changed back before anything was heard */
            else begin_change(f, f->pending_curve);
        }

        if (f->count == 0) {
            if (f->dirty || !dj_smooth_settled(&f->cut) || !dj_smooth_settled(&f->res)) { refresh(f); f->dirty = 0; }
            f->count = DJ_COEF_INTERVAL;
        }
        f->count--;

        double x[2] = { l[i], r[i] };
        if (f->next_curve < 0) {
            for (int c = 0; c < 2; c++) x[c] = cascade(f->st[c], f->co, f->sections, hp, x[c]);
        } else {
            /* 0 during the warm-up, then a raised-cosine 0 -> 1 over the fade */
            int k = f->xf_n - f->warm_len;
            double m = k <= 0 ? 0.0 : k >= f->fade_len ? 1.0 : 0.5 - 0.5 * cos(DJ_PI * (double)k / (double)f->fade_len);
            for (int c = 0; c < 2; c++) {
                double a = cascade(f->st[c], f->co, f->sections, hp, x[c]);
                double b = cascade(f->st2[c], f->co2, f->next_sections, hp, x[c]);
                x[c] = a + m * (b - a);
            }
            if (++f->xf_n >= f->warm_len + f->fade_len) {          /* done: the new slope takes over, state and all */
                memcpy(f->st, f->st2, sizeof f->st);
                memcpy(f->co, f->co2, sizeof f->co);
                f->curve = f->next_curve;
                f->sections = f->next_sections;
                f->next_curve = -1;
            }
        }
        l[i] = (float)x[0];
        r[i] = (float)x[1];
    }
}
