/* dj_meter.h -- peak meter with a fixed fall rate, for the input/output/maximizer meters, and a gain-reduction meter.
 * push() takes a block of planar samples; value01() maps the held peak, -60..0 dBFS, to 0..1 for the skin's filmstrip.
 * Allocation-free and lock-free: the engine publishes value01() through get_param().
 *
 * The filmstrips are DJ_METER_FRAMES tall-segment pictures: 20 segments of 3 dB (60 dB) = 21 frames. MPC draws filmstrips
 * up to 12288 px tall correctly, and the full-height meters are 560 px, so 21 frames is the most that fits; the meter
 * therefore moves in 3 dB steps, like an LED ladder. */
#pragma once
#include "dj_common.h"

#define DJ_METER_FLOOR_DB (-60.0f)
#define DJ_METER_FALL_DB_PER_S 24.0f
#define DJ_METER_SEGS 20
#define DJ_METER_FRAMES (DJ_METER_SEGS + 1)      /* frames in the skin's meter filmstrip: 0 = nothing lit .. 20 = all lit */
#define DJ_GR_START_DB (-2.0f)                   /* "reduction" meter: the first segment lights 2 dB BEFORE limiting starts ... */
#define DJ_GR_RANGE_DB 20.0f                     /* ... the last 18 dB into it, one segment per dB (1.0.5). It shows how hard the
                                                    maximizer is driven: the level going into its limiter relative to the 0 dB
                                                    ceiling (after the threshold gain), or the reduction still applied while it
                                                    releases, whichever is more. So the top segments warn just before limiting,
                                                    and from 0 dB on it reads the reduction (1.0.4: from 5 dB, 1.0.3: 3 dB). The dB
                                                    number under it reads the exact reduction. */
#define DJ_GR_HOLD_S 0.3f                        /* each peak stays 0.3 s before falling, so the 100 ms screen poll never misses one */

typedef struct { float fs; float peak[2]; } dj_meter_t;

void dj_meter_init(dj_meter_t *m, float fs);
void dj_meter_push(dj_meter_t *m, const float *l, const float *r, int n);
void dj_meter_push_peaks(dj_meter_t *m, float pl, float pr, int n);   /* the block's peaks already known; n = block length */
float dj_meter_value01(const dj_meter_t *m, int ch);
float dj_block_peak(const float *l, const float *r, int n);           /* the larger channel's peak, linear */
void dj_meter_reset(dj_meter_t *m);

/* the maximizer's drive in dB (negative = below the ceiling, positive = over it / reducing), instant rise, held DJ_GR_HOLD_S, then
 * falls at DJ_METER_FALL_DB_PER_S; value 0..1 = DJ_GR_START_DB .. + DJ_GR_RANGE_DB */
typedef struct { float fs; float db, hold_s; } dj_grmeter_t;
void dj_grmeter_init(dj_grmeter_t *m, float fs);
void dj_grmeter_push(dj_grmeter_t *m, float drive_db, int n);
float dj_grmeter_value01(const dj_grmeter_t *m);
void dj_grmeter_reset(dj_grmeter_t *m);
