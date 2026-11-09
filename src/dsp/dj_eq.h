/* dj_eq.h -- DJ-mixer-style 3-band channel EQ with two selectable behaviours.
 *
 *   ISOLATOR: three bands split by phase-aligned LR4 crossovers (LOW | MID | HIGH). Each band has its own level;
 *             fully counter-clockwise is a true kill (exact zero). With every band at unity the sum is an all-pass.
 *   EQ:       low shelf + mid bell + high shelf, adjustable boost and cut, no kill.
 *
 * Knob law (both modes, n = 0..1 knob position, 0.5 = centre = 0 dB):
 *   n >= 0.5 : +6 dB at full clockwise, linear in dB.
 *   n <  0.5 : ISOLATOR  gain = (2n)^2  -> -inf at n = 0;   EQ  dB = -26 * (1 - 2n)  -> -26 dB at n = 0.
 * The -26 / +6 dB figures are the DJM-900NXS2 operating-instructions' published range (HI / MID / LOW). The CROSSOVER
 * and SHELF/BELL FREQUENCIES are NOT published: see dj_eq_config.h and docs/EQ_VERIFICATION.md. They are provisional
 * until measured against hardware and must not be treated as matching it. */
#pragma once
#include "dj_common.h"
#include "dj_eq_config.h"

typedef enum { DJ_EQ_MODE_EQ = 0, DJ_EQ_MODE_ISOLATOR = 1 } dj_eq_mode_t;

typedef struct {
    float fs;
    dj_eq_mode_t mode, target_mode;
    float xf, xf_step;                  /* mode crossfade: 0 = all `mode`, 1 = all `target_mode` while changing */
    int xfading;
    /* knob targets */
    float n[3];                          /* low, mid, high */
    dj_smooth_t iso_g[3];                /* ISOLATOR band gains (linear) */
    dj_smooth_t eq_db[3];                /* EQ-mode band gains (dB) */
    /* ISOLATOR: low|rest split at f1, mid|high split at f2, low band all-pass compensated at f2 */
    dj_svf_t s1lp[2][2], s1hp[2][2];     /* [ch][stage] crossover 1 */
    dj_svf_t s2lp[2][2], s2hp[2][2];     /* crossover 2 on the upper split */
    dj_svf_t aplp[2][2], aphp[2][2];     /* crossover-2 all-pass match for the low band */
    dj_svf_c_t c1, c2;
    /* EQ mode */
    dj_biquad_t ls[2], pk[2], hs[2];
    int count;
    int first;
} dj_eq_t;

void dj_eq_init(dj_eq_t *s, float fs);
void dj_eq_set_mode(dj_eq_t *s, dj_eq_mode_t m);
void dj_eq_set(dj_eq_t *s, float low_n, float mid_n, float high_n);
void dj_eq_process(dj_eq_t *s, float *l, float *r, int n);
void dj_eq_reset(dj_eq_t *s);

/* knob position -> gain, shared with the display strings */
float dj_eq_knob_to_db(float n, dj_eq_mode_t m);     /* -200 means -inf */
float dj_eq_knob_to_lin(float n, dj_eq_mode_t m);
