/* dj_comp.h -- the OUTPUT page's compressor (2.1.1, the user's spec): CTRL Giant's compressor model, in stereo, with its
 * detector chosen by the PEAK and RMS buttons, and a 0 dB soft clip at its input (CLIP; 2.1.2: it was after it).
 *
 * The model (CTRL Giant src/engine/ctl.c): THRESHOLD -60 .. 0 dB of full scale, RATIO 1:1 .. 20:1 (log), a 6 dB soft knee,
 * the reduction computed in dB from the level over THRESHOLD and then following ATTACK (0.1 .. 50 ms) and RELEASE (5 ms .. 2 s);
 * THRESHOLD and RATIO through two smoothers in a row (about 10 ms each), so a sudden jump starts gently. No makeup gain (the
 * maximizer follows). Linked stereo: one gain for both channels.
 *   PEAK  the level is the larger channel's instantaneous peak (CTRL Giant's own detector): every transient is caught.
 *   RMS   the level is the mean square of both channels over DJ_COMP_RMS_MS: the average loudness, so short spikes slip past
 *         and sustained energy is compressed.
 *   CLIP  the sound going INTO the compressor (after the DJ EQ and filters): peaks over -0.1 dBFS are clipped (the transient
 *         shaper's clipper, dj_soft_clip, its rounding squeezed into the last 0.1 dB, so it only touches peaks at full scale), then
 *         the level is lowered 1 dB, so the stage before the compressor keeps its headroom (2.1.4, the user's call; the rounding
 *         used to start 6 dB down, with no trim). Faded in and out over DJ_COMP_SWITCH_MS.
 * Plain C, no allocation, no latency. */
#pragma once
#include "dj_common.h"

#define DJ_COMP_KNEE_DB 6.0f
#define DJ_COMP_RMS_MS 10.0f
#define DJ_COMP_SWITCH_MS 10.0f
#define DJ_COMP_CLIP_DB (-0.1f)        /* (2.1.4, the user's call) CLIP: peaks over -0.1 dBFS are clipped ... */
#define DJ_COMP_CLIP_TRIM_DB (-1.0f)   /* ... and then everything going into the compressor is lowered 1 dB */

typedef struct {
    float fs;
    float t_thr, t_slope;               /* targets: THRESHOLD dB, the slope 1 - 1/RATIO */
    double thr, thr1, slope, slope1;    /* their two smoothers */
    double sm_a;
    double att_a, rel_a;
    int rms;                            /* the detector: 1 = RMS, 0 = PEAK */
    double ms;                          /* RMS: the running mean square */
    double rms_a;
    double gr;                          /* the reduction now, dB (>= 0) */
    dj_smooth_t clip_mix;
    /* this block's readings, for the meters (dj_comp_take_stats) */
    float gr_peak, drive;
} dj_comp_t;

void dj_comp_init(dj_comp_t *c, float fs);
void dj_comp_set(dj_comp_t *c, float thr_db, float ratio, float attack_ms, float release_ms, int rms, int clip);
void dj_comp_reset(dj_comp_t *c);
void dj_comp_process(dj_comp_t *c, float *l, float *r, int n);
/* the most reduction in the blocks since the last call (dB, >= 0), and how close the level came to THRESHOLD (dB, < 0 below it;
 * -100 at 1:1, when nothing can come) */
void dj_comp_take_stats(dj_comp_t *c, float *gr_db, float *drive_db);

/* the RATIO knob (0..1) -> the ratio, 1 .. 20 (log); the THRESHOLD knob -> -60 .. 0 dB */
static inline float dj_comp_ratio(float n) { return powf(20.0f, dj_clampf(n, 0.0f, 1.0f)); }
static inline float dj_comp_thresh_db(float n) { return -60.0f + 60.0f * dj_clampf(n, 0.0f, 1.0f); }
