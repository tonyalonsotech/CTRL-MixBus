/* dj_gate.h -- noise gate with threshold, attack, hold, release.
 *
 * Linked stereo: ONE detector watches max(|L|, |R|) and ONE gain is applied to both channels, so the stereo image
 * cannot shift when only one side crosses the threshold (rule 9). The gate opens when the detector exceeds the
 * threshold and closes once it has stayed below (threshold - hysteresis) for the hold time. The gain then glides:
 * `attack` is the opening time constant, `release` the closing one. A closed gate is exact silence. */
#pragma once
#include "dj_common.h"

#define DJ_GATE_HYST_DB 3.0f     /* fixed hysteresis below the threshold, so a signal hovering at the threshold cannot chatter */

typedef struct {
    float fs;
    float thr_open, thr_close;       /* linear */
    float att_a, rel_a;              /* per-sample coefficients of the gain glide */
    float det_rel;                   /* detector fall coefficient (fixed ~20 ms) */
    int hold_samples, hold_left;
    float env, gain;
    int open;
} dj_gate_t;

void dj_gate_init(dj_gate_t *g, float fs);
void dj_gate_set(dj_gate_t *g, float thr_db, float attack_ms, float hold_ms, float release_ms);
void dj_gate_process(dj_gate_t *g, float *l, float *r, int n);
void dj_gate_reset(dj_gate_t *g);
