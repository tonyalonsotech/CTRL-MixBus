/* dj_maximizer.h -- a zero-latency peak maximizer: threshold raises the level into a fixed 0 dBFS ceiling.
 *
 *   in -> threshold gain (+ -thr dB) -> limiter (linked stereo, instant-ish attack, one-pole release) -> hard 0 dBFS clip
 *      -> output trim (-inf .. 0 dB)
 *
 * No ceiling knob (the ceiling is 0.0 dBFS), no knee, no lookahead: the MPC host does not compensate plugin latency, so a
 * lookahead limiter would shift this channel against the rest of the project. Instead the gain computer reacts on the very
 * sample (attack = how fast the gain glides down) and the final clip guarantees nothing leaves above the ceiling even when
 * the attack is slow; a longer attack therefore lets the transient through the limiter and clips it (harder, more edgy).
 * The two stereo channels share ONE gain (linked detection), so the image cannot shift. Output trim is applied after the
 * clip, so the ceiling can only be lowered, never exceeded. With the defaults (threshold 0 dB, output 0 dB) the module is
 * an exact pass-through for any signal at or below full scale. Everything is float, allocation-free, no locks.
 * ON/OFF (2.1.3, the user's call): off bypasses the threshold gain, the limiter and the ceiling clip -- everything but the output
 * trim, which works either way (the switch is after the maximizer, before OUTPUT). The change fades over DJ_MAX_SWITCH_MS. While
 * off the limiter rests (gain 1) and the meter statistics read nothing. */
#pragma once
#include "dj_common.h"

#define DJ_MAX_SWITCH_MS 6.0f

typedef struct {
    float fs;
    int on;
    dj_smooth_t on_mix;              /* 1 = maximizing, 0 = bypassed (the output trim still applies) */
    dj_smooth_t mk, out;             /* threshold gain and output trim, linear, smoothed (no zipper noise) */
    double att_a, rel_a;             /* per-sample glide coefficients */
    double g;                        /* limiter gain, <= 1. Double: a float one-pole stalls just short of 1.0 and never settles */
    /* block statistics for the meters (reset by dj_max_take_stats) */
    float in_pk[2];                  /* peak of the signal the limiter saw (after the threshold gain) */
    double g_min;                    /* lowest limiter gain in the block */
} dj_maximizer_t;

void dj_max_init(dj_maximizer_t *m, float fs);
void dj_max_set(dj_maximizer_t *m, float thr_db, float attack_ms, float release_ms, float out_db);
void dj_max_reset(dj_maximizer_t *m);
void dj_max_set_on(dj_maximizer_t *m, int on);
void dj_max_process(dj_maximizer_t *m, float *l, float *r, int n);
/* peaks (linear) of the signal at the limiter's input since the last call, and the gain reduction in dB (>= 0) */
void dj_max_take_stats(dj_maximizer_t *m, float *in_pk_l, float *in_pk_r, float *gr_db);
