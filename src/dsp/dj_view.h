/* dj_view.h -- the filter-curve display: the numbers the engine, tools/gen_filter_strips.py and tools/gen_layout.py share.
 *
 * MPC draws pictures, it cannot draw live graphs, so each filter's response curve is a pre-drawn filmstrip: one strip per
 * slope (shown by a `when=<filter>_curve:<slope>` panel), whose frames are every (cutoff step, resonance bucket) pair, plus
 * one flat frame for "stage bypassed / fully open". The engine reports `frame / (DJ_VIEW_FRAMES - 1)` and MPC shows that
 * frame. The art is computed by running the real dj_filter.c, so the picture is the DSP's own response.
 *
 * Fluidity = DJ_VIEW_STEPS: 40 log-spaced cutoff positions over 20 Hz..20 kHz is one step per 0.075 decade (a quarter
 * octave), against 20 (a half octave) in 0.1.0. Resonance in 5 buckets (0, 25, 50, 75, 100 %; 3 until 0.2.0, so the peak now
 * grows in quarter steps). 40 steps x 5 resonance buckets + 1 = 201 frames.
 *
 * Memory: 12 strips x DJ_VIEW_W x (DJ_VIEW_FRAMES x DJ_VIEW_H) x 4 bytes = about 119 MB decoded at these sizes (121 frames:
 * 71 MB; 0.1.0: 41 MB).
 * Lower DJ_VIEW_STEPS / DJ_VIEW_W / DJ_VIEW_H if the device shows a skin-load problem; a strip must stay under 12288 px tall
 * (docs/PORTING.md; 201 x 56 = 11256). Then rerun tools/gen_*.py. */
#pragma once

#define DJ_VIEW_W        220     /* picture size in px */
#define DJ_VIEW_H        56
#define DJ_VIEW_STEPS    40      /* cutoff positions: knob 0..1 in this many log-spaced steps (20 Hz .. 20 kHz) */
#define DJ_VIEW_RES      5       /* resonance buckets: 0, 25, 50, 75, 100 % */
#define DJ_VIEW_FRAMES   (DJ_VIEW_STEPS * DJ_VIEW_RES + 1)   /* + 1: the flat "bypassed" frame, always the last */
#define DJ_VIEW_DB_TOP   24.0    /* picture's dB range: +24 (above the +18 dB resonance peak) .. -60 */
#define DJ_VIEW_DB_BOT  (-60.0)
