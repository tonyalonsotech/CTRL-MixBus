#include "dj_meter.h"

void dj_meter_init(dj_meter_t *m, float fs) { m->fs = fs; m->peak[0] = m->peak[1] = 0.0f; }
void dj_meter_reset(dj_meter_t *m) { m->peak[0] = m->peak[1] = 0.0f; }

void dj_meter_push_peaks(dj_meter_t *m, float pl, float pr, int n)
{
    float fall = powf(10.0f, -DJ_METER_FALL_DB_PER_S * ((float)n / m->fs) / 20.0f);
    m->peak[0] = fmaxf(pl, m->peak[0] * fall);
    m->peak[1] = fmaxf(pr, m->peak[1] * fall);
    if (m->peak[0] < 1e-6f) m->peak[0] = 0.0f;
    if (m->peak[1] < 1e-6f) m->peak[1] = 0.0f;
}

float dj_block_peak(const float *l, const float *r, int n)
{
    float p = 0.0f;
    for (int i = 0; i < n; i++) {
        float a = fabsf(l[i]), b = fabsf(r[i]);
        if (a > p) p = a;
        if (b > p) p = b;
    }
    return p;
}

void dj_meter_push(dj_meter_t *m, const float *l, const float *r, int n)
{
    float pl = 0.0f, pr = 0.0f;
    for (int i = 0; i < n; i++) {
        float a = fabsf(l[i]), b = fabsf(r[i]);
        if (a > pl) pl = a;
        if (b > pr) pr = b;
    }
    dj_meter_push_peaks(m, pl, pr, n);
}

float dj_meter_value01(const dj_meter_t *m, int ch)
{
    float p = m->peak[ch & 1];
    if (p <= 0.0f) return 0.0f;
    float db = 20.0f * log10f(p);
    return dj_clampf((db - DJ_METER_FLOOR_DB) / -DJ_METER_FLOOR_DB, 0.0f, 1.0f);
}

#define GR_FLOOR_DB (-100.0f)

void dj_grmeter_init(dj_grmeter_t *m, float fs) { m->fs = fs; m->db = GR_FLOOR_DB; m->hold_s = 0.0f; }
void dj_grmeter_reset(dj_grmeter_t *m) { m->db = GR_FLOOR_DB; m->hold_s = 0.0f; }

void dj_grmeter_push(dj_grmeter_t *m, float drive_db, int n)
{
    float dt = (float)n / m->fs;
    if (!(drive_db > GR_FLOOR_DB)) drive_db = GR_FLOOR_DB;          /* also NaN */
    if (drive_db >= m->db) { m->db = drive_db; m->hold_s = DJ_GR_HOLD_S; return; }
    if (m->hold_s > 0.0f) { m->hold_s -= dt; return; }
    float d = m->db - DJ_METER_FALL_DB_PER_S * dt;
    m->db = drive_db > d ? drive_db : d;
}

float dj_grmeter_value01(const dj_grmeter_t *m) { return dj_clampf((m->db - DJ_GR_START_DB) / DJ_GR_RANGE_DB, 0.0f, 1.0f); }
