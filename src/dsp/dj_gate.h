/* dj_gate.h -- noise gate with threshold, attack, hold, release.
 *
 * Linked stereo: ONE detector watches max(|L|, |R|) and ONE gain is applied to both channels, so the stereo image
 * cannot shift when only one side crosses the threshold (rule 9). The gate opens when the detector exceeds the
 * threshold and closes once it has stayed below (threshold - hysteresis) for the hold time. The gain then glides:
 * `attack` is the opening time constant, `release` the closing one. A closed gate is exact silence.
 * FLIP (2.1.3, the user's call) turns it round: above the threshold the sound is muted (ducked), below it the sound passes. Attack
 * is then how fast it ducks when the signal goes over, release how fast the sound comes back (after the hold) once it falls under:
 * attack and release always follow the threshold crossing, whichever way the gain moves. A flipped gate starts open (passing).
 * (2.1.4) A flipped gate changes the gain while the sound is LOUD, where a sudden turn of the gain is a click (a normal gate's
 * opening is masked by the hit that opens it), so its gain also runs through DJ_GATE_FLIP_SMOOTH_STAGES more one-pole stages of
 * DJ_GATE_FLIP_SMOOTH_MS: every duck and return starts and ends on an S-curve. A normal gate is unchanged. */
#pragma once
#include "dj_common.h"

#define DJ_GATE_FLIP_SMOOTH_MS 1.5f
#define DJ_GATE_FLIP_SMOOTH_STAGES 2
#define DJ_GATE_HYST_DB 3.0f     /* fixed hysteresis below the threshold, so a signal hovering at the threshold cannot chatter */

typedef struct {
    float fs;
    float thr_open, thr_close;       /* linear */
    float att_a, rel_a;              /* per-sample coefficients of the gain glide */
    float det_rel;                   /* detector fall coefficient (fixed ~20 ms) */
    int hold_samples, hold_left;
    float env, gain;
    int open;                        /* the signal is over the threshold (or within the hold after it) */
    int flip;
    float sm[DJ_GATE_FLIP_SMOOTH_STAGES], sm_a;   /* FLIP's extra smoothing of the gain */
} dj_gate_t;

void dj_gate_init(dj_gate_t *g, float fs);
void dj_gate_set(dj_gate_t *g, float thr_db, float attack_ms, float hold_ms, float release_ms);
void dj_gate_process(dj_gate_t *g, float *l, float *r, int n);
void dj_gate_reset(dj_gate_t *g);
void dj_gate_set_flip(dj_gate_t *g, int flip);
