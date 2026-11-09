/* dj_bassmono.h -- bass mono below an adjustable crossover.
 *
 * Each channel is split with a Linkwitz-Riley 4th-order crossover (two cascaded Butterworth sections per branch),
 * the two LOW bands are summed to mono, and the HIGH bands stay stereo:  out = mono(low) + high.
 * LR4 low + high sum to an all-pass, so a mono-compatible signal passes with a flat magnitude at every crossover
 * frequency, and the split is phase-aligned (no notch where the bands meet). */
#pragma once
#include "dj_common.h"

typedef struct {
    float fs;
    dj_smooth_t freq;               /* smoothed crossover, Hz (smoothed in log domain by the caller's value) */
    dj_svf_t lp[2][2], hp[2][2];    /* [channel][cascade stage] */
    dj_svf_c_t co;
    double g_cache, k;
    int count;
} dj_bassmono_t;

void dj_bassmono_init(dj_bassmono_t *s, float fs);
void dj_bassmono_set_freq(dj_bassmono_t *s, float hz);
void dj_bassmono_process(dj_bassmono_t *s, float *l, float *r, int n);
void dj_bassmono_reset(dj_bassmono_t *s);
