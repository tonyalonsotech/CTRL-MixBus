/* dj_common.h -- shared helpers for the DJ Utility DSP modules.
 *
 * Every module in src/dsp is plain C11, allocation-free after init, and has no dependency outside libm, so each one
 * can be unit-tested alone (tests/test_dsp.c). Audio buffers are planar float (l[], r[]), processed in place.
 * Nothing here allocates, takes a lock, or touches a file. */
#pragma once
#include <math.h>
#include <stdint.h>

#define DJ_PI 3.14159265358979323846
#define DJ_FS 44100.0f          /* MPC plugin host contract (wrapper/engine.h) */
#define DJ_BLOCK 128            /* MPC's period; modules accept any n <= DJ_BLOCK */
#define DJ_COEF_INTERVAL 16     /* filter coefficients are refreshed every 16 samples while a control moves */
#define DJ_NEG_INF_DB (-96.0f)  /* a gain at or below this is treated as -infinity (exact zero) */

static inline float dj_clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }

static inline float dj_db_to_lin(float db) { return db <= DJ_NEG_INF_DB ? 0.0f : powf(10.0f, db * 0.05f); }

static inline float dj_lin_to_db(float lin) { return lin <= 1.5849e-5f ? -200.0f : 20.0f * log10f(lin); }

/* One-pole smoother. `a` is the per-sample coefficient, so a time constant of `ms` reaches ~63 % in `ms`.
 * State is double: a float one-pole STALLS a few millionths short of its target (the step a*(t-v) falls below half an
 * ulp), so it would never settle, never snap to exact unity or exact silence, and a "settled?" test would never fire. */
typedef struct { double v, t, a; } dj_smooth_t;

static inline void dj_smooth_init(dj_smooth_t *s, float fs, float ms, float v)
{
    s->v = s->t = v;
    s->a = ms <= 0.0f ? 1.0 : 1.0 - exp(-1.0 / (0.001 * (double)ms * (double)fs));
}
static inline void dj_smooth_set(dj_smooth_t *s, float target) { s->t = target; }
static inline float dj_smooth_next(dj_smooth_t *s)
{
    s->v += s->a * (s->t - s->v);
    if (fabs(s->t - s->v) < 1e-6) s->v = s->t;   /* settle exactly (-120 dB, far below 16-bit): a muted stage is exactly silent */
    return (float)s->v;
}
static inline int dj_smooth_settled(const dj_smooth_t *s) { return s->v == s->t; }

/* Linear crossfade between a stage's dry and wet signal: `mix` 1 = stage active, 0 = bypassed. A true bypass: when
 * mix is 0 and the target is 0 the engine does not run the stage at all. */
typedef struct { float mix, target, step; } dj_xfade_t;

static inline void dj_xfade_init(dj_xfade_t *x, float fs, float ms, float active)
{
    x->mix = x->target = active;
    x->step = 1.0f / (0.001f * ms * fs);
}
static inline void dj_xfade_set(dj_xfade_t *x, int active) { x->target = active ? 1.0f : 0.0f; }
static inline float dj_xfade_next(dj_xfade_t *x)
{
    if (x->mix < x->target) { x->mix += x->step; if (x->mix > x->target) x->mix = x->target; }
    else if (x->mix > x->target) { x->mix -= x->step; if (x->mix < x->target) x->mix = x->target; }
    return x->mix;
}
static inline int dj_xfade_off(const dj_xfade_t *x) { return x->mix <= 0.0f && x->target <= 0.0f; }
static inline int dj_xfade_on(const dj_xfade_t *x) { return x->mix >= 1.0f && x->target >= 1.0f; }

/* Trapezoidal (TPT) state-variable filter section, double precision state: stable and click-free under modulation,
 * and accurate down to very low cutoffs where float biquads lose precision. */
typedef struct { double ic1, ic2; } dj_svf_t;
typedef struct { double k, a1, a2, a3; } dj_svf_c_t;

static inline void dj_svf_coef(dj_svf_c_t *c, double g, double k)
{
    c->k = k;
    c->a1 = 1.0 / (1.0 + g * (g + k));
    c->a2 = g * c->a1;
    c->a3 = g * c->a2;
}
static inline void dj_svf_tick(dj_svf_t *s, const dj_svf_c_t *c, double v0, double *lp, double *hp)
{
    double v3 = v0 - s->ic2;
    double v1 = c->a1 * s->ic1 + c->a2 * v3;
    double v2 = s->ic2 + c->a2 * s->ic1 + c->a3 * v3;
    s->ic1 = 2.0 * v1 - s->ic1;
    s->ic2 = 2.0 * v2 - s->ic2;
    *lp = v2;
    *hp = v0 - c->k * v1 - v2;
}
static inline double dj_svf_g(double fc, double fs)
{
    double lim = 0.45 * fs;
    if (fc > lim) fc = lim;
    if (fc < 1.0) fc = 1.0;
    return tan(DJ_PI * fc / fs);
}

/* Direct form II transposed biquad (double), used by the EQ-mode shelves and bell. */
typedef struct { double b0, b1, b2, a1, a2, z1, z2; } dj_biquad_t;

static inline double dj_biquad_tick(dj_biquad_t *q, double x)
{
    double y = q->b0 * x + q->z1;
    q->z1 = q->b1 * x - q->a1 * y + q->z2;
    q->z2 = q->b2 * x - q->a2 * y;
    return y;
}
