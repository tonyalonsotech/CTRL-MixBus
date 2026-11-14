/* dj_transient.h -- the transient shaper (2.1.1, the user's spec): ATTACK, PUMP, SUSTAIN, SPEED, then the level stages.
 *
 *   in -> shaper gain (ATTACK + SUSTAIN + PUMP, in dB) -> AUTO LEVEL (CLIP off) or the 0 dB soft clip and -3 dB (CLIP on)
 *      -> TRIM (-3 dB, -6 dB, both -9 dB) -> out
 *
 * The detector is linked stereo (the larger channel), so both channels get one gain and the image never shifts. Four envelope
 * followers read it:
 *   ATTACK   a fast-attack and a slow-attack follower with the same release: at a hit the fast one leads, and the gap between them
 *            (in dB, at most DJ_TS_DIFF_MAX_DB) is the transient. ATTACK scales it: +100 % = up to +12 dB on the hit, -100 % =
 *            down to -12 dB.
 *   SUSTAIN  a fast-release and a slow-release follower with the same attack: after the hit the slow one stays up, and the gap is
 *            the tail. SUSTAIN scales it: +100 % = up to +18 dB on the tail, -100 % = down to -18 dB.
 *   PUMP     the transient's gap through a peak follower (instant rise, slower fall) less the gap itself: zero on the rise, and a
 *            dip that opens as the transient passes and closes again. PUMP 0..100 % = a dip of up to 24 dB right behind the hit.
 *   SPEED    scales every follower's time: 0 = 4 x slower (smooth, wide), 0.5 = the base times, 1 = 4 x faster (short, snappy).
 * At ATTACK 0 %, SUSTAIN 0 %, PUMP 0 % the shaper gain is exactly 0 dB.
 *
 * AUTO LEVEL (CLIP off): the shaper's output peak is measured and, when it goes over 0 dBFS, the output is lowered by that
 * amount (the user's example: +4 dB out is lowered 4 dB, to 0.0 dB). It reacts in DJ_TS_AUTO_ATT_MS and lets go over
 * DJ_TS_AUTO_REL_MS, so it rides the level rather than limiting each wave. Below 0 dBFS it does nothing.
 * CLIP on: AUTO LEVEL lets go and a soft clipper rounds the peaks toward 0 dBFS (the rounding starts DJ_TS_CLIP_KNEE_DB below),
 * then the level is lowered 3 dB. The change between the two fades over DJ_TS_SWITCH_MS, so it never clicks.
 * TRIM: -3 dB, -6 dB, or both (-9 dB), after the clipper; smoothed.
 * Plain C, no allocation, no latency (no lookahead: the MPC does not compensate plugin latency). */
#pragma once
#include "dj_common.h"

#define DJ_TS_DIFF_MAX_DB 24.0f         /* the most a follower gap counts for */
#define DJ_TS_ATTACK_DB_PER_DB 0.5f     /* ATTACK +-100 %: +-12 dB at the full gap */
#define DJ_TS_SUSTAIN_DB_PER_DB 0.75f   /* SUSTAIN +-100 %: +-18 dB */
#define DJ_TS_PUMP_DB_PER_DB 1.0f       /* PUMP 100 %: up to DJ_TS_DIFF_MAX_DB of dip */
#define DJ_TS_AUTO_ATT_MS 1.0f
#define DJ_TS_AUTO_REL_MS 500.0f
#define DJ_TS_CLIP_KNEE_DB 6.0f
#define DJ_TS_CLIP_TRIM_DB (-3.0f)      /* after the clip */
#define DJ_TS_SWITCH_MS 10.0f

typedef struct {
    float fs;
    float att, sus, pump, speed;        /* the knobs: att / sus -1..+1, pump 0..1, speed 0..1 */
    float clip_on, m3_on, m6_on;        /* the buttons, 0 / 1 */
    /* follower coefficients (from SPEED) */
    double a_fa, a_sa, a_rel, s_att, s_fr, s_sr, p_rel;
    double fa, sa, fr, sr, pk;          /* followers (linear) and the pump's peak follower (dB) */
    dj_smooth_t k_att, k_sus, k_pump;   /* the knobs, smoothed */
    dj_smooth_t clip_mix, trim;         /* CLIP: 0 = AUTO LEVEL, 1 = the clipper (and its -3 dB); TRIM linear */
    double auto_g;                      /* AUTO LEVEL's gain, <= 1 */
    double auto_a, auto_r;
} dj_transient_t;

void dj_transient_init(dj_transient_t *t, float fs);
/* att, sus: -1..+1; pump, speed: 0..1; the buttons 0 / 1 */
void dj_transient_set(dj_transient_t *t, float att, float pump, float sus, float speed, int clip, int m3, int m6);
void dj_transient_reset(dj_transient_t *t);
void dj_transient_process(dj_transient_t *t, float *l, float *r, int n);

/* the soft clipper both CLIP buttons use: untouched below knee_lin, rounded toward 1.0 (0 dBFS) above it, never past it */
static inline float dj_soft_clip(float y, float knee_lin)
{
    float a = fabsf(y);
    if (!(a > knee_lin)) return y == y ? y : 0.0f;    /* (NaN: silence) */
    float room = 1.0f - knee_lin, m = knee_lin + room * tanhf((a - knee_lin) / room);
    return y < 0.0f ? -m : m;
}
