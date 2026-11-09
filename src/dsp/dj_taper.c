#include "dj_taper.h"
#include <stdio.h>
#include <string.h>

#define FLOOR_N 0.02f
#define FLOOR_DB (-60.0f)

float dj_taper_gain_db(float n, float n0, float max_db)
{
    n = dj_clampf(n, 0.0f, 1.0f);
    if (n <= 0.0f) return -200.0f;
    if (n < FLOOR_N) {                                    /* fade the LINEAR gain to zero below -60 dB */
        float lin = powf(10.0f, FLOOR_DB * 0.05f) * (n / FLOOR_N);
        return lin <= 1.5849e-5f ? -200.0f : 20.0f * log10f(lin);
    }
    if (n <= n0) return FLOOR_DB + (0.0f - FLOOR_DB) * (n - FLOOR_N) / (n0 - FLOOR_N);
    return max_db * (n - n0) / (1.0f - n0);
}

float dj_taper_gain_n_from_db(float db, float n0, float max_db)
{
    if (db <= DJ_NEG_INF_DB) return 0.0f;
    if (db >= 0.0f) return dj_clampf(n0 + (db / max_db) * (1.0f - n0), n0, 1.0f);
    if (db <= FLOOR_DB) return dj_clampf(FLOOR_N * powf(10.0f, (db - FLOOR_DB) * 0.05f), 0.0f, FLOOR_N);
    return FLOOR_N + (db - FLOOR_DB) / (0.0f - FLOOR_DB) * (n0 - FLOOR_N);
}

float dj_taper_bass_hz(float n) { return 40.0f * powf(10.0f, dj_clampf(n, 0.0f, 1.0f)); }
float dj_taper_attack_ms(float n) { return 0.1f * powf(500.0f, dj_clampf(n, 0.0f, 1.0f)); }
float dj_taper_hold_ms(float n) { return 500.0f * dj_clampf(n, 0.0f, 1.0f); }
float dj_taper_release_ms(float n) { return 5.0f * powf(400.0f, dj_clampf(n, 0.0f, 1.0f)); }
float dj_taper_thresh_db(float n) { return -80.0f * (1.0f - dj_clampf(n, 0.0f, 1.0f)); }
float dj_taper_mx_thresh_db(float n) { return DJ_MX_THR_MIN_DB * (1.0f - dj_clampf(n, 0.0f, 1.0f)); }
float dj_taper_mx_attack_ms(float n) { return 0.01f * powf(500.0f, dj_clampf(n, 0.0f, 1.0f)); }
float dj_taper_mx_out_db(float n) { return dj_taper_gain_db(n, DJ_MX_OUT_N0, 0.0f); }
float dj_taper_balance(float n) { return dj_clampf(n, 0.0f, 1.0f) * 2.0f - 1.0f; }

/* locale-proof: snprintf uses the process locale's decimal separator; force '.' */
static int fix(char *buf, int len, int w)
{
    if (len <= 0) return 0;
    if (w < 0) { buf[0] = 0; return 0; }
    if (w >= len) w = len - 1;
    for (int i = 0; i < w; i++) if (buf[i] == ',') buf[i] = '.';
    return w;
}

int dj_fmt_gain(char *buf, int len, float db)
{
    if (db <= -100.0f) return fix(buf, len, snprintf(buf, (size_t)len, "-inf dB"));
    return fix(buf, len, snprintf(buf, (size_t)len, "%+.1f dB", (double)db));
}
int dj_fmt_db(char *buf, int len, float db)
{
    if (db <= -100.0f) return fix(buf, len, snprintf(buf, (size_t)len, "-inf dB"));
    if (db > -0.05f && db < 0.05f) db = 0.0f;
    return fix(buf, len, snprintf(buf, (size_t)len, "%.1f dB", (double)db));
}
int dj_fmt_hz(char *buf, int len, float hz)
{
    if (hz >= 1000.0f) return fix(buf, len, snprintf(buf, (size_t)len, "%.2f kHz", (double)(hz / 1000.0f)));
    return fix(buf, len, snprintf(buf, (size_t)len, "%.0f Hz", (double)hz));
}
int dj_fmt_ms(char *buf, int len, float ms)
{
    if (ms >= 1000.0f) return fix(buf, len, snprintf(buf, (size_t)len, "%.2f s", (double)(ms / 1000.0f)));
    if (ms < 10.0f) return fix(buf, len, snprintf(buf, (size_t)len, "%.2f ms", (double)ms));
    return fix(buf, len, snprintf(buf, (size_t)len, "%.0f ms", (double)ms));
}
int dj_fmt_percent(char *buf, int len, float v01)
{
    return fix(buf, len, snprintf(buf, (size_t)len, "%.0f %%", (double)(v01 * 100.0f)));
}
int dj_fmt_balance(char *buf, int len, float n)
{
    float b = dj_taper_balance(n);
    if (fabsf(b) < 0.005f) return fix(buf, len, snprintf(buf, (size_t)len, "C"));
    return fix(buf, len, snprintf(buf, (size_t)len, "%c %.0f", b < 0.0f ? 'L' : 'R', (double)(fabsf(b) * 100.0f)));
}
int dj_fmt_eq(char *buf, int len, float db)
{
    if (db <= -100.0f) return fix(buf, len, snprintf(buf, (size_t)len, "KILL"));
    return fix(buf, len, snprintf(buf, (size_t)len, "%+.1f dB", (double)db));
}
