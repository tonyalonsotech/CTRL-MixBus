#include "dj_transient.h"
#include <string.h>

#define SMOOTH_MS 10.0f
#define FLOOR_LIN 1e-5f                /* -100 dB: below it a follower counts as silence */
/* the followers' base times (SPEED 0.5), ms */
#define FAST_ATT_MS 0.5f
#define SLOW_ATT_MS 15.0f
#define ATT_REL_MS 100.0f               /* both ATTACK followers */
#define FAST_REL_MS 30.0f
#define SLOW_REL_MS 300.0f
#define PUMP_REL_MS 40.0f

static double coef_ms(float fs, float ms) { return ms <= 0.0f ? 1.0 : 1.0 - exp(-1.0 / (0.001 * (double)ms * (double)fs)); }

static void set_times(dj_transient_t *t)
{
    float k = powf(4.0f, 1.0f - 2.0f * dj_clampf(t->speed, 0.0f, 1.0f));   /* SPEED: 4 x slower .. 4 x faster */
    t->a_fa = coef_ms(t->fs, FAST_ATT_MS * k);
    t->a_sa = coef_ms(t->fs, SLOW_ATT_MS * k);
    t->a_rel = coef_ms(t->fs, ATT_REL_MS * k);
    t->s_att = coef_ms(t->fs, FAST_ATT_MS * k);
    t->s_fr = coef_ms(t->fs, FAST_REL_MS * k);
    t->s_sr = coef_ms(t->fs, SLOW_REL_MS * k);
    t->p_rel = coef_ms(t->fs, PUMP_REL_MS * k);
}

void dj_transient_init(dj_transient_t *t, float fs)
{
    memset(t, 0, sizeof *t);
    t->fs = fs;
    dj_smooth_init(&t->k_att, fs, SMOOTH_MS, 0.0f);
    dj_smooth_init(&t->k_sus, fs, SMOOTH_MS, 0.0f);
    dj_smooth_init(&t->k_pump, fs, SMOOTH_MS, 0.0f);
    dj_smooth_init(&t->clip_mix, fs, DJ_TS_SWITCH_MS, 0.0f);
    dj_smooth_init(&t->trim, fs, SMOOTH_MS, 1.0f);
    t->auto_a = coef_ms(fs, DJ_TS_AUTO_ATT_MS);
    t->auto_r = coef_ms(fs, DJ_TS_AUTO_REL_MS);
    dj_transient_set(t, 0.0f, 0.0f, 0.0f, 0.5f, 0, 0, 0);
    dj_transient_reset(t);
}

void dj_transient_set(dj_transient_t *t, float att, float pump, float sus, float speed, int clip, int m3, int m6)
{
    t->att = dj_clampf(att, -1.0f, 1.0f);
    t->sus = dj_clampf(sus, -1.0f, 1.0f);
    t->pump = dj_clampf(pump, 0.0f, 1.0f);
    t->speed = dj_clampf(speed, 0.0f, 1.0f);
    t->clip_on = clip ? 1.0f : 0.0f;
    t->m3_on = m3 ? 1.0f : 0.0f;
    t->m6_on = m6 ? 1.0f : 0.0f;
    dj_smooth_set(&t->k_att, t->att);
    dj_smooth_set(&t->k_sus, t->sus);
    dj_smooth_set(&t->k_pump, t->pump);
    dj_smooth_set(&t->clip_mix, t->clip_on);
    dj_smooth_set(&t->trim, dj_db_to_lin(-3.0f * t->m3_on - 6.0f * t->m6_on));
    set_times(t);
}

void dj_transient_reset(dj_transient_t *t)
{
    t->fa = t->sa = t->fr = t->sr = 0.0;
    t->pk = 0.0;
    t->auto_g = 1.0;
    t->k_att.v = t->k_att.t; t->k_sus.v = t->k_sus.t; t->k_pump.v = t->k_pump.t;
    t->clip_mix.v = t->clip_mix.t; t->trim.v = t->trim.t;
}

static inline double follow(double e, double x, double att, double rel) { return e + (x > e ? att : rel) * (x - e); }

/* 20 log10(a / b), both floored at FLOOR_LIN, kept to 0 .. DJ_TS_DIFF_MAX_DB */
static inline float gap_db(double a, double b)
{
    float fa = a > FLOOR_LIN ? (float)a : FLOOR_LIN, fb = b > FLOOR_LIN ? (float)b : FLOOR_LIN;
    if (fa <= fb) return 0.0f;
    float d = 20.0f * log10f(fa / fb);
    return d > DJ_TS_DIFF_MAX_DB ? DJ_TS_DIFF_MAX_DB : d;
}

void dj_transient_process(dj_transient_t *t, float *l, float *r, int n)
{
    const float clip_knee = dj_db_to_lin(-DJ_TS_CLIP_KNEE_DB), clip_trim = dj_db_to_lin(DJ_TS_CLIP_TRIM_DB);
    for (int i = 0; i < n; i++) {
        float a = l[i], b = r[i];
        if (!(fabsf(a) <= 1e6f)) a = 0.0f;               /* NaN or inf in: silence, never a stuck follower */
        if (!(fabsf(b) <= 1e6f)) b = 0.0f;
        double x = fabsf(a) > fabsf(b) ? fabsf(a) : fabsf(b);   /* linked detector */
        t->fa = follow(t->fa, x, t->a_fa, t->a_rel);
        t->sa = follow(t->sa, x, t->a_sa, t->a_rel);
        t->fr = follow(t->fr, x, t->s_att, t->s_fr);
        t->sr = follow(t->sr, x, t->s_att, t->s_sr);
        float ka = dj_smooth_next(&t->k_att), ks = dj_smooth_next(&t->k_sus), kp = dj_smooth_next(&t->k_pump);
        float g = 1.0f;
        float ga = gap_db(t->fa, t->sa);              /* the transient */
        t->pk = ga > t->pk ? ga : t->pk + t->p_rel * (ga - t->pk);
        if (ka != 0.0f || ks != 0.0f || kp != 0.0f) {
            float db = ka * DJ_TS_ATTACK_DB_PER_DB * ga
                     + ks * DJ_TS_SUSTAIN_DB_PER_DB * gap_db(t->sr, t->fr)          /* the tail */
                     - kp * DJ_TS_PUMP_DB_PER_DB * (float)(t->pk - ga);            /* the dip behind the hit */
            g = expf(0.11512925f * db);
        }
        a *= g; b *= g;
        /* AUTO LEVEL (CLIP off): pull an output over 0 dBFS back to 0 dBFS; CLIP on: let go */
        float cm = dj_smooth_next(&t->clip_mix);
        double pk = fabsf(a) > fabsf(b) ? fabsf(a) : fabsf(b);
        double need = (t->clip_on < 0.5f && pk > 1.0) ? 1.0 / pk : 1.0;
        if (need < t->auto_g) t->auto_g += t->auto_a * (need - t->auto_g);
        else { t->auto_g += t->auto_r * (1.0 - t->auto_g); if (t->auto_g > 1.0 - 1e-7) t->auto_g = 1.0; }
        a *= (float)t->auto_g; b *= (float)t->auto_g;
        if (cm > 0.0f) {                              /* the clipper and its -3 dB, faded in */
            float ca = dj_soft_clip(a, clip_knee) * clip_trim, cb = dj_soft_clip(b, clip_knee) * clip_trim;
            a += cm * (ca - a); b += cm * (cb - b);
        }
        float tr = dj_smooth_next(&t->trim);
        l[i] = a * tr;
        r[i] = b * tr;
    }
}
