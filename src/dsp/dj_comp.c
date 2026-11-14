#include "dj_comp.h"
#include "dj_transient.h"               /* dj_soft_clip */
#include <string.h>

#define SMOOTH_MS 10.0f

static double coef_ms(float fs, float ms) { return ms <= 0.0f ? 1.0 : 1.0 - exp(-1.0 / (0.001 * (double)ms * (double)fs)); }

void dj_comp_init(dj_comp_t *c, float fs)
{
    memset(c, 0, sizeof *c);
    c->fs = fs;
    c->sm_a = coef_ms(fs, SMOOTH_MS);
    c->rms_a = coef_ms(fs, DJ_COMP_RMS_MS);
    dj_smooth_init(&c->clip_mix, fs, DJ_COMP_SWITCH_MS, 0.0f);
    dj_comp_set(c, 0.0f, 1.0f, 4.0f, 100.0f, 0, 0);
    dj_comp_reset(c);
}

void dj_comp_set(dj_comp_t *c, float thr_db, float ratio, float attack_ms, float release_ms, int rms, int clip)
{
    c->t_thr = dj_clampf(thr_db, -60.0f, 0.0f);
    c->t_slope = 1.0f - 1.0f / dj_clampf(ratio, 1.0f, 20.0f);
    c->att_a = coef_ms(c->fs, dj_clampf(attack_ms, 0.05f, 1000.0f));
    c->rel_a = coef_ms(c->fs, dj_clampf(release_ms, 1.0f, 10000.0f));
    c->rms = rms ? 1 : 0;
    dj_smooth_set(&c->clip_mix, clip ? 1.0f : 0.0f);
}

void dj_comp_reset(dj_comp_t *c)
{
    c->thr = c->thr1 = c->t_thr;
    c->slope = c->slope1 = c->t_slope;
    c->ms = 0.0;
    c->gr = 0.0;
    c->clip_mix.v = c->clip_mix.t;
    c->gr_peak = 0.0f;
    c->drive = -100.0f;
}

/* CTRL Giant's soft knee: the reduction (dB) for a level `over` dB above THRESHOLD */
static inline double knee_gr(double over, double slope)
{
    if (2.0 * over < -DJ_COMP_KNEE_DB) return 0.0;
    if (2.0 * over > DJ_COMP_KNEE_DB) return slope * over;
    double t = over + 0.5 * DJ_COMP_KNEE_DB;
    return slope * t * t / (2.0 * DJ_COMP_KNEE_DB);
}

void dj_comp_process(dj_comp_t *c, float *l, float *r, int n)
{
    const float knee = dj_db_to_lin(DJ_COMP_CLIP_DB), trim = dj_db_to_lin(DJ_COMP_CLIP_TRIM_DB);
    /* at rest: 1:1 with no reduction left (nothing to compute) */
    int rest = c->t_slope <= 0.0f && c->slope <= 1e-6 && c->gr < 1e-5;
    float gr_peak = c->gr_peak, drive = c->drive;
    for (int i = 0; i < n; i++) {
        float a = l[i], b = r[i];
        if (!(fabsf(a) <= 1e6f)) a = 0.0f;               /* NaN or inf in: silence, never a stuck follower */
        if (!(fabsf(b) <= 1e6f)) b = 0.0f;
        /* CLIP (2.1.2, the user's call): at the compressor's input, after the DJ EQ and filters */
        float cm = dj_smooth_next(&c->clip_mix);
        if (cm > 0.0f) { a += cm * (dj_soft_clip(a, knee) * trim - a); b += cm * (dj_soft_clip(b, knee) * trim - b); }
        c->thr1 += c->sm_a * (c->t_thr - c->thr1); c->thr += c->sm_a * (c->thr1 - c->thr);
        c->slope1 += c->sm_a * (c->t_slope - c->slope1); c->slope += c->sm_a * (c->slope1 - c->slope);
        /* the detector: PEAK (the larger channel, as CTRL Giant) or RMS (both channels' mean square); RMS keeps running either way,
         * so switching to it starts from the real average */
        c->ms += c->rms_a * (0.5 * ((double)a * a + (double)b * b) - c->ms);
        if (!rest) {
            double lev;
            if (c->rms) lev = c->ms > 1e-12 ? 10.0 * log10(c->ms) : -120.0;
            else { float pk = fabsf(a) > fabsf(b) ? fabsf(a) : fabsf(b); lev = pk > 1e-6f ? 20.0 * log10(pk) : -120.0; }
            double over = lev - c->thr;
            if (over > drive) drive = (float)over;
            double tgt = knee_gr(over, c->slope);
            c->gr += (tgt > c->gr ? c->att_a : c->rel_a) * (tgt - c->gr);
            if (tgt <= 0.0 && c->gr < 1e-4) c->gr = 0.0;        /* released: none left */
            if (c->gr > gr_peak) gr_peak = (float)c->gr;
            if (c->gr > 0.0) { float g = expf(-0.11512925f * (float)c->gr); a *= g; b *= g; }
        }
        l[i] = a;
        r[i] = b;
    }
    if (rest) { c->slope = c->slope1 = 0.0; c->gr = 0.0; }
    if (c->slope <= 1e-4) drive = -100.0f;     /* 1:1: the meter shows nothing coming */
    c->gr_peak = gr_peak;
    c->drive = drive;
}

void dj_comp_take_stats(dj_comp_t *c, float *gr_db, float *drive_db)
{
    *gr_db = c->gr_peak;
    *drive_db = c->drive;
    c->gr_peak = (float)c->gr;                  /* the next block starts from where the reduction is NOW */
    c->drive = -100.0f;
}
