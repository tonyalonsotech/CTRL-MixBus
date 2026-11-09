/* dj_simple.h -- the four small stages: input/output gain, phase invert, stereo width, balance.
 * Each is an independent struct with init / set / process / reset; each smooths its own control so a moving knob
 * never steps (rule 4). `dj_gain_t` serves both the input gain (-inf..+24 dB) and the output level (-inf..+12 dB):
 * the range lives in the parameter taper (dj_taper.h), not here. */
#pragma once
#include "dj_common.h"

typedef struct { dj_smooth_t g; } dj_gain_t;
void dj_gain_init(dj_gain_t *s, float fs);
void dj_gain_set_db(dj_gain_t *s, float db);            /* db <= -96 means -infinity */
void dj_gain_process(dj_gain_t *s, float *l, float *r, int n);
void dj_gain_reset(dj_gain_t *s);

/* Phase invert: ONE switch, both channels together. The sign is smoothed (about 2 ms) so flipping it mid-note does
 * not click; it passes through zero rather than jumping. */
typedef struct { dj_smooth_t sign; } dj_phase_t;
void dj_phase_init(dj_phase_t *s, float fs);
void dj_phase_set(dj_phase_t *s, int inverted);
void dj_phase_process(dj_phase_t *s, float *l, float *r, int n);
void dj_phase_reset(dj_phase_t *s);

/* Stereo width by mid/side: M = (L+R)/2, S = (L-R)/2, L = M + w*S, R = M - w*S. w = 1 is the untouched signal, w = 0
 * is mono (L = R = M). Mid is never scaled, so a centred (mono-compatible) signal keeps its level at every width;
 * only uncorrelated content loses level as it narrows, which is the audible meaning of "narrower". */
typedef struct { dj_smooth_t w; } dj_width_t;
void dj_width_init(dj_width_t *s, float fs);
void dj_width_set(dj_width_t *s, float width01);        /* 0 = mono .. 1 = normal */
void dj_width_process(dj_width_t *s, float *l, float *r, int n);
void dj_width_reset(dj_width_t *s);

/* Balance: -1 = full left .. 0 = centre .. +1 = full right. The opposite channel is attenuated linearly toward
 * silence; the near channel is never boosted, so centre is exactly unity (no level change in the neutral position). */
typedef struct { dj_smooth_t gl, gr; } dj_balance_t;
void dj_balance_init(dj_balance_t *s, float fs);
void dj_balance_set(dj_balance_t *s, float bal);
void dj_balance_process(dj_balance_t *s, float *l, float *r, int n);
void dj_balance_reset(dj_balance_t *s);
