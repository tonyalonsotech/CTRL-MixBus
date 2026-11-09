/* dj_filter.h -- resonant low-pass / high-pass with selectable slope.
 *
 * Slopes: 12, 24, 36, 48, 96 dB/oct and "brickwall". Each is a cascade of Butterworth 2nd-order sections
 * (orders 2, 4, 6, 8, 16 and 32). "Brickwall" is the 32nd-order Butterworth cascade, about 192 dB/oct: it is a very
 * steep IIR, NOT a linear-phase FIR wall (that would add latency); see docs/DECISIONS.md.
 *
 * Resonance raises the Q of the highest-Q section by up to DJ_FLT_RES_MAX_DB, giving a peak at the cutoff that is
 * about that tall at every slope. Cutoff and resonance are smoothed.
 *
 * A slope change never mutes or restarts the running filter (until 0.2.0 it ducked the signal for ~4 ms and restarted from
 * silence, which clicked). Instead the new slope's sections start beside the old ones and run, unheard, for DJ_FLT_WARM_MS so
 * they settle on the signal, then the output crossfades from the old slope to the new one over DJ_FLT_FADE_MS (equal gain:
 * both are the same input filtered, so they are correlated). A change that arrives while the new slope is still warming up
 * re-targets it; one that arrives during the fade waits for the fade to end. CPU doubles only during those ~35 ms. */
#pragma once
#include "dj_common.h"

#define DJ_FLT_MAX_SECTIONS 16
#define DJ_FLT_RES_MAX_DB 18.0f
#define DJ_FLT_NUM_CURVES 6      /* 12, 24, 36, 48, 96, brickwall */
#define DJ_FLT_WARM_MS 10.0f     /* slope change: the new sections run unheard this long ... */
#define DJ_FLT_FADE_MS 25.0f     /* ... then the output crossfades old -> new over this long */

typedef enum { DJ_FLT_LP = 0, DJ_FLT_HP = 1 } dj_flt_type_t;

typedef struct {
    float fs;
    dj_flt_type_t type;
    int curve, pending_curve, sections;
    dj_smooth_t cut;               /* normalized 0..1, smoothed */
    dj_smooth_t res;               /* 0..1 */
    double q[DJ_FLT_NUM_CURVES][DJ_FLT_MAX_SECTIONS];
    dj_svf_t st[2][DJ_FLT_MAX_SECTIONS];
    dj_svf_c_t co[DJ_FLT_MAX_SECTIONS];
    /* slope change in progress (next_curve >= 0): the incoming slope's own sections, state and the crossfade position */
    int next_curve, next_sections, xf_n, warm_len, fade_len;
    dj_svf_t st2[2][DJ_FLT_MAX_SECTIONS];
    dj_svf_c_t co2[DJ_FLT_MAX_SECTIONS];
    int count, dirty;
} dj_filter_t;

void dj_filter_init(dj_filter_t *f, float fs, dj_flt_type_t type);
void dj_filter_set(dj_filter_t *f, float cut01, float res01, int curve);
void dj_filter_process(dj_filter_t *f, float *l, float *r, int n);
void dj_filter_reset(dj_filter_t *f);

int dj_filter_sections_for_curve(int curve);          /* 1, 2, 3, 4, 8, 16 */
float dj_filter_cut_hz(float cut01);                    /* 20 Hz .. 20 kHz, log */
