/* dj_readout.h -- the dB number under each meter pair: its peak, held for DJ_READOUT_HOLD_S, as two pictures.
 *
 * MPC redraws text only on a full UpdateDisplay, which this plugin does not send under live meters (vst.json
 * DISPLAY_REV_NO_UPDATE), so a live number has to be a picture, like the meters. One picture per 0.1 dB would be far too many
 * frames, so the number is split in two filmstrips side by side, both drawn by tools/gen_readout_strip.py:
 *
 *   art/readout_int.png  the sign and whole dB, right-aligned: frame 0 = "-inf" (silence or below DJ_READOUT_FLOOR_DB),
 *                        1..60 = "-60".."-1", 61 = "-0" (for -0.1 .. -0.9), 62..86 = "0".."24"
 *   art/readout_dec.png  the tenth and the unit, left-aligned: frames 0..9 = ".0 dB" .. ".9 dB", 10 = " dB" (after "-inf")
 *
 * Hold: a peak above the shown one replaces it at once and is held a full second; when the hold runs out the readout shows the
 * loudest peak of the second just gone (so a falling level steps down once a second, and a low note's peak is never missed
 * between blocks). The gain-reduction readout shows the
 * reduction as a negative number ("-3.2 dB"), "0.0 dB" when there is none, and holds the deepest reduction instead. */
#pragma once
#include "dj_common.h"

#define DJ_READOUT_HOLD_S    1.0f
#define DJ_READOUT_FLOOR_DB  (-60.0f)
#define DJ_READOUT_TOP_DB    24.9f                   /* clamped above: "24.9 dB" */
#define DJ_READOUT_INT_FRAMES 87                     /* "-inf", -60..-1, "-0", 0..24 */
#define DJ_READOUT_DEC_FRAMES 11                     /* .0 .. .9, blank */

typedef struct { float fs, held_db, win_db, left_s; } dj_readout_t;

static inline void dj_readout_init(dj_readout_t *r, float fs) { r->fs = fs; r->held_db = r->win_db = -1000.0f; r->left_s = DJ_READOUT_HOLD_S; }

/* one block: `db` = this block's peak in dB (-1000 for silence), n = frames in the block */
static inline void dj_readout_push(dj_readout_t *r, float db, int n)
{
    r->left_s -= (float)n / r->fs;
    if (db > r->held_db) {                     /* a new high: shown at once, held a full second */
        r->held_db = db;
        r->win_db = -1000.0f;
        r->left_s = DJ_READOUT_HOLD_S;
        return;
    }
    if (db > r->win_db) r->win_db = db;
    if (r->left_s <= 0.0f) {                   /* hold over: show the loudest peak of the second just gone */
        r->held_db = r->win_db;
        r->win_db = -1000.0f;
        r->left_s = DJ_READOUT_HOLD_S;
    }
}

static inline float dj_readout_lin_db(float peak) { return peak > 1e-9f ? 20.0f * log10f(peak) : -1000.0f; }

/* the two frame indices for a value in dB */
static inline void dj_readout_frames(float db, int *fi, int *fd)
{
    if (!(db >= DJ_READOUT_FLOOR_DB - 0.05f)) { *fi = 0; *fd = DJ_READOUT_DEC_FRAMES - 1; return; }   /* also NaN */
    if (db > DJ_READOUT_TOP_DB) db = DJ_READOUT_TOP_DB;
    long t = lroundf(db * 10.0f);                                                  /* tenths of a dB */
    if (t < -600) t = -600;
    if (t < 0) { long a = -t; *fi = a < 10 ? 61 : 61 - (int)(a / 10); *fd = (int)(a % 10); }
    else { *fi = 62 + (int)(t / 10); *fd = (int)(t % 10); }
}

/* frame -> the 0..1 value the skin's filmstrip shows (frame / (frames - 1)) */
static inline double dj_readout_int01(float db) { int fi, fd; dj_readout_frames(db, &fi, &fd); return (double)fi / (DJ_READOUT_INT_FRAMES - 1); }
static inline double dj_readout_dec01(float db) { int fi, fd; dj_readout_frames(db, &fi, &fd); return (double)fd / (DJ_READOUT_DEC_FRAMES - 1); }
