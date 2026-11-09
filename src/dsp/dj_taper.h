/* dj_taper.h -- every knob is 0..1 on the wire; these functions give the engine units and the on-screen text.
 * Keeping all mappings here means presets store the knob position (stable across versions) and the display text
 * always agrees with what the DSP does. */
#pragma once
#include "dj_common.h"

/* Gain with a true -infinity: n = 0 is exactly silent. From n = 0.02 to n0 the gain runs -60..0 dB, above n0 it runs
 * 0..max_db; below 0.02 the linear gain fades to 0. Input: n0 = 0.70, max +24 dB. Output: n0 = 0.80, max +12 dB. */
#define DJ_IN_N0  0.70f
#define DJ_IN_MAX_DB 24.0f
#define DJ_OUT_N0 0.80f
#define DJ_OUT_MAX_DB 12.0f

float dj_taper_gain_db(float n, float n0, float max_db);         /* -200 = -inf */
float dj_taper_gain_n_from_db(float db, float n0, float max_db); /* inverse, for tests and preset tools */

float dj_taper_bass_hz(float n);        /* 40 .. 400 Hz, log */
float dj_taper_attack_ms(float n);      /* 0.1 .. 50 ms, log */
float dj_taper_hold_ms(float n);        /* 0 .. 500 ms, linear */
float dj_taper_release_ms(float n);     /* 5 .. 2000 ms, log */
float dj_taper_thresh_db(float n);      /* -80 .. 0 dB */
float dj_taper_balance(float n);        /* 0..1 -> -1..+1 */

/* Maximizer. Threshold -30..0 dB linear in dB (the signal is raised by -threshold, so a signal at the threshold reaches the
 * 0 dB ceiling). Attack 0.01..5 ms log: it is the time the limiter takes to pull the gain down, and there is no lookahead
 * (zero latency), so the hard 0 dBFS ceiling clip catches whatever a slower attack lets through. Release reuses
 * dj_taper_release_ms. Output is a trim BELOW the ceiling: -inf..0 dB, 1.0 = 0 dB. */
#define DJ_MX_THR_MIN_DB (-30.0f)
#define DJ_MX_OUT_N0 1.0f
float dj_taper_mx_thresh_db(float n);   /* -30 .. 0 dB */
float dj_taper_mx_attack_ms(float n);   /* 0.01 .. 5 ms, log */
float dj_taper_mx_out_db(float n);      /* -200 (-inf) .. 0 dB, the gain taper with its top at 0 dB */

/* display text, written into buf (len bytes); return the length, never writes past len. The decimal point is always '.',
 * whatever the process locale. */
int dj_fmt_gain(char *buf, int len, float db);
int dj_fmt_db(char *buf, int len, float db);        /* "-12.0 dB", "0.0 dB" (no plus sign, no "-0.0"), "-inf dB" */
int dj_fmt_hz(char *buf, int len, float hz);
int dj_fmt_ms(char *buf, int len, float ms);
int dj_fmt_percent(char *buf, int len, float v01);
int dj_fmt_balance(char *buf, int len, float n);
int dj_fmt_eq(char *buf, int len, float db);
