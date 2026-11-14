/* test_dsp.c -- unit tests for every DSP module, each tested alone (rule 5).
 * Build/run: tests/run_tests.sh  (gcc, libm only; AddressSanitizer + UBSan). Exit status 0 = all passed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/dsp/dj_simple.h"
#include "../src/dsp/dj_bassmono.h"
#include "../src/dsp/dj_eq.h"
#include "../src/dsp/dj_filter.h"
#include "../src/dsp/dj_gate.h"
#include "../src/dsp/dj_maximizer.h"
#include "../src/dsp/dj_transient.h"
#include "../src/dsp/dj_comp.h"
#include "../src/dsp/dj_meter.h"
#include "../src/dsp/dj_taper.h"
#include "../src/dsp/dj_readout.h"

static int g_fail = 0, g_run = 0;
#define CHECK(cond, ...) do { g_run++; if (!(cond)) { g_fail++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

typedef void (*proc_fn)(void *, float *, float *, int);

#define FS DJ_FS
#define BLK 128

/* steady-state RMS gain of a stereo module for a sine at f (Hz). phaseR = 0 in phase, 1 = L/R in anti-phase.
 * Returns the gain of channel `ch` (0 = L, 1 = R) relative to the input amplitude. */
static double tone_gain(proc_fn fn, void *st, double f, int antiphase, int ch, double amp)
{
    const int settle = (int)(FS * 0.6), meas = (int)(FS * 0.25);
    float l[BLK], r[BLK];
    double acc = 0.0, ref = 0.0;
    long t = 0;
    for (int done = 0; done < settle + meas; done += BLK) {
        for (int i = 0; i < BLK; i++, t++) {
            double v = amp * sin(2.0 * DJ_PI * f * (double)t / FS);
            l[i] = (float)v;
            r[i] = (float)(antiphase ? -v : v);
        }
        fn(st, l, r, BLK);
        if (done >= settle) for (int i = 0; i < BLK; i++) {
            double y = ch ? r[i] : l[i];
            acc += y * y;
            ref += amp * amp * 0.5;
        }
    }
    return sqrt(acc / ref);
}
static double to_db(double g) { return g < 1e-12 ? -240.0 : 20.0 * log10(g); }

/* ---- thin wrappers so every module fits proc_fn ---- */
static void w_gain(void *s, float *l, float *r, int n) { dj_gain_process(s, l, r, n); }
static void w_width(void *s, float *l, float *r, int n) { dj_width_process(s, l, r, n); }
static void w_bm(void *s, float *l, float *r, int n) { dj_bassmono_process(s, l, r, n); }
static void w_eq(void *s, float *l, float *r, int n) { dj_eq_process(s, l, r, n); }
static void w_flt(void *s, float *l, float *r, int n) { dj_filter_process(s, l, r, n); }

static int finite_buf(const float *a, int n) { for (int i = 0; i < n; i++) if (!isfinite(a[i])) return 0; return 1; }

/* ------------------------------------------------------------------------------------------------------------- */
static void test_gain(void)
{
    puts("gain");
    dj_gain_t g; dj_gain_init(&g, FS);
    dj_gain_set_db(&g, 6.0f); g.g.v = g.g.t;
    CHECK(fabs(tone_gain(w_gain, &g, 1000, 0, 0, 0.1) - 1.9953) < 0.01, "+6 dB gain");
    dj_gain_set_db(&g, 24.0f); g.g.v = g.g.t;
    CHECK(fabs(to_db(tone_gain(w_gain, &g, 1000, 0, 0, 0.01)) - 24.0) < 0.1, "+24 dB gain");
    dj_gain_set_db(&g, -100.0f);
    float l[BLK], r[BLK];
    for (int k = 0; k < 40; k++) { for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.5f; dj_gain_process(&g, l, r, BLK); }
    int zero = 1; for (int i = 0; i < BLK; i++) if (l[i] != 0.0f || r[i] != 0.0f) zero = 0;
    CHECK(zero, "-inf dB must be exact silence");

    /* zipper: a 0 dB -> +24 dB jump must not step */
    dj_gain_init(&g, FS); dj_gain_set_db(&g, 24.0f);
    float prev = 0.01f, maxd = 0.0f;
    for (int k = 0; k < 8; k++) {
        for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.01f;
        dj_gain_process(&g, l, r, BLK);
        for (int i = 0; i < BLK; i++) { float d = fabsf(l[i] - prev); if (d > maxd) maxd = d; prev = l[i]; }
    }
    CHECK(maxd < 0.001f, "gain change stepped: max delta %g", (double)maxd);
}

static void test_phase(void)
{
    puts("phase");
    dj_phase_t p; dj_phase_init(&p, FS);
    float l[BLK], r[BLK];
    dj_phase_set(&p, 1);
    for (int k = 0; k < 20; k++) { for (int i = 0; i < BLK; i++) { l[i] = 0.3f; r[i] = -0.2f; } dj_phase_process(&p, l, r, BLK); }
    CHECK(fabsf(l[BLK - 1] + 0.3f) < 1e-5f && fabsf(r[BLK - 1] - 0.2f) < 1e-5f, "both channels invert together");
    dj_phase_set(&p, 0);
    float prev = -0.5f /* the inverted constant input the next blocks start from */, maxd = 0.0f;
    for (int k = 0; k < 8; k++) {
        for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.5f;
        dj_phase_process(&p, l, r, BLK);
        for (int i = 0; i < BLK; i++) { float d = fabsf(l[i] - prev); if (d > maxd) maxd = d; prev = l[i]; }
    }
    CHECK(maxd < 0.05f, "phase flip clicked: %g", (double)maxd);
}

static void test_width(void)
{
    puts("width");
    dj_width_t w; dj_width_init(&w, FS);
    float l[BLK], r[BLK];
    dj_width_set(&w, 0.0f);
    for (int k = 0; k < 40; k++) { for (int i = 0; i < BLK; i++) { l[i] = 0.8f; r[i] = 0.2f; } dj_width_process(&w, l, r, BLK); }
    CHECK(fabsf(l[0] - 0.5f) < 1e-5f && fabsf(r[0] - 0.5f) < 1e-5f, "width 0 = mono sum/2 (got %g %g)", (double)l[0], (double)r[0]);
    dj_width_set(&w, 1.0f);
    for (int k = 0; k < 40; k++) { for (int i = 0; i < BLK; i++) { l[i] = 0.8f; r[i] = 0.2f; } dj_width_process(&w, l, r, BLK); }
    CHECK(fabsf(l[0] - 0.8f) < 1e-5f && fabsf(r[0] - 0.2f) < 1e-5f, "width 1 = untouched");
    dj_width_set(&w, 0.0f);
    for (int k = 0; k < 40; k++) { for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.7f; dj_width_process(&w, l, r, BLK); }
    CHECK(fabsf(l[0] - 0.7f) < 1e-6f, "centred signal keeps its level when narrowed");
}

static void test_balance(void)
{
    puts("balance");
    dj_balance_t b; dj_balance_init(&b, FS);
    float l[BLK], r[BLK];
    dj_balance_set(&b, 1.0f);
    for (int k = 0; k < 40; k++) { for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.5f; dj_balance_process(&b, l, r, BLK); }
    CHECK(l[0] == 0.0f && fabsf(r[0] - 0.5f) < 1e-6f, "full right silences left only");
    dj_balance_set(&b, 0.0f);
    for (int k = 0; k < 40; k++) { for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.5f; dj_balance_process(&b, l, r, BLK); }
    CHECK(l[0] == 0.5f && r[0] == 0.5f, "centre is exactly unity");
    dj_balance_set(&b, -0.5f);
    for (int k = 0; k < 40; k++) { for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.5f; dj_balance_process(&b, l, r, BLK); }
    CHECK(fabsf(l[0] - 0.5f) < 1e-6f && fabsf(r[0] - 0.25f) < 1e-6f, "half left halves right");
}

static void test_bassmono(void)
{
    puts("bassmono");
    dj_bassmono_t s; dj_bassmono_init(&s, FS); dj_bassmono_set_freq(&s, 120.0f); s.freq.v = s.freq.t; dj_bassmono_reset(&s);
    double lo = tone_gain(w_bm, &s, 40.0, 1, 0, 0.5);
    dj_bassmono_reset(&s);
    double hi = tone_gain(w_bm, &s, 4000.0, 1, 0, 0.5);
    CHECK(to_db(lo) < -30.0, "out-of-phase bass below the crossover must cancel to mono (got %.1f dB)", to_db(lo));
    CHECK(fabs(to_db(hi)) < 0.3, "out-of-phase highs above the crossover keep their stereo (got %.1f dB)", to_db(hi));
    /* mono input: LR4 low + high is an all-pass => flat magnitude everywhere, incl. around the crossover */
    double fs_[] = { 30, 60, 100, 120, 150, 300, 1000, 8000, 16000 };
    for (unsigned k = 0; k < sizeof fs_ / sizeof *fs_; k++) {
        dj_bassmono_reset(&s);
        double g = to_db(tone_gain(w_bm, &s, fs_[k], 0, 0, 0.3));
        CHECK(fabs(g) < 0.15, "mono input must pass flat: %.0f Hz -> %.2f dB", fs_[k], g);
    }
}

static void test_eq(void)
{
    puts("eq");
    dj_eq_t e; dj_eq_init(&e, FS);
    dj_eq_set_mode(&e, DJ_EQ_MODE_ISOLATOR); e.mode = e.target_mode; e.xfading = 0;
    dj_eq_set(&e, 0.5f, 0.5f, 0.5f); dj_eq_reset(&e);
    double fq[] = { 30, 80, 250, 600, 1000, 2500, 5000, 12000 };
    for (unsigned k = 0; k < sizeof fq / sizeof *fq; k++) {
        dj_eq_reset(&e);
        double g = to_db(tone_gain(w_eq, &e, fq[k], 0, 0, 0.3));
        CHECK(fabs(g) < 0.3, "ISOLATOR at unity must be flat (all-pass): %.0f Hz -> %.2f dB", fq[k], g);
    }
    dj_eq_set(&e, 0.0f, 0.5f, 0.5f); dj_eq_reset(&e);
    double kill = to_db(tone_gain(w_eq, &e, 60, 0, 0, 0.3));
    CHECK(kill < -40.0, "ISOLATOR low kill must be >40 dB down at 60 Hz (got %.1f)", kill);
    dj_eq_reset(&e);
    double mid = to_db(tone_gain(w_eq, &e, 1000, 0, 0, 0.3));
    CHECK(fabs(mid) < 0.5, "ISOLATOR low kill leaves the mids alone (got %.2f)", mid);
    /* an LR4 isolator band is not a brick wall: at the geometric centre of the mid band (~790 Hz) the neighbours' skirts
     * leave about -37 dB, so the honest bar is -30 dB there */
    dj_eq_set(&e, 0.5f, 0.0f, 0.5f); dj_eq_reset(&e);
    double midkill = to_db(tone_gain(w_eq, &e, 790, 0, 0, 0.3));
    CHECK(midkill < -30.0, "ISOLATOR mid kill at the band centre (got %.1f)", midkill);
    dj_eq_set(&e, 0.5f, 0.5f, 0.0f); dj_eq_reset(&e);
    CHECK(to_db(tone_gain(w_eq, &e, 10000, 0, 0, 0.3)) < -40.0, "ISOLATOR high kill");
    dj_eq_set(&e, 1.0f, 1.0f, 1.0f); dj_eq_reset(&e);
    double boost = to_db(tone_gain(w_eq, &e, 1000, 0, 0, 0.2));
    CHECK(fabs(boost - 6.0) < 1.0, "ISOLATOR full boost ~ +6 dB at a band centre (got %.2f)", boost);

    /* EQ mode: cut limited to -26 dB, never a kill */
    dj_eq_set_mode(&e, DJ_EQ_MODE_EQ); dj_eq_set(&e, 0.0f, 0.0f, 0.0f); e.mode = e.target_mode; e.xfading = 0; dj_eq_reset(&e);
    double lo = to_db(tone_gain(w_eq, &e, 40, 0, 0, 0.3));
    CHECK(lo < -22.0 && lo > -29.0, "EQ mode low cut ~ -26 dB shelf at 40 Hz (got %.1f)", lo);
    dj_eq_set(&e, 1.0f, 1.0f, 1.0f); dj_eq_reset(&e);
    double hi = to_db(tone_gain(w_eq, &e, 12000, 0, 0, 0.1));
    CHECK(hi > 4.0 && hi < 7.0, "EQ mode high boost ~ +6 dB shelf at 12 kHz (got %.1f)", hi);
    dj_eq_set(&e, 0.5f, 0.5f, 0.5f); dj_eq_reset(&e);
    CHECK(fabs(to_db(tone_gain(w_eq, &e, 1000, 0, 0, 0.3))) < 0.05, "EQ mode at centre is flat");

    /* knob law */
    CHECK(dj_eq_knob_to_db(0.5f, DJ_EQ_MODE_EQ) == 0.0f, "centre = 0 dB");
    CHECK(fabsf(dj_eq_knob_to_db(1.0f, DJ_EQ_MODE_EQ) - 6.0f) < 1e-4f, "max = +6 dB");
    CHECK(fabsf(dj_eq_knob_to_db(0.0f, DJ_EQ_MODE_EQ) + 26.0f) < 1e-4f, "EQ min = -26 dB");
    CHECK(dj_eq_knob_to_lin(0.0f, DJ_EQ_MODE_ISOLATOR) == 0.0f, "ISOLATOR min = exact kill");

    /* mode switch while audio runs: finite, no big jump */
    dj_eq_init(&e, FS); dj_eq_set(&e, 0.5f, 0.5f, 0.5f);
    float l[BLK], r[BLK], prev = 0.0f, maxd = 0.0f; long t = 0;
    for (int k = 0; k < 60; k++) {
        if (k == 20) dj_eq_set_mode(&e, DJ_EQ_MODE_EQ);
        if (k == 40) dj_eq_set_mode(&e, DJ_EQ_MODE_ISOLATOR);
        for (int i = 0; i < BLK; i++, t++) l[i] = r[i] = (float)(0.3 * sin(2 * DJ_PI * 440.0 * t / FS));
        dj_eq_process(&e, l, r, BLK);
        CHECK(finite_buf(l, BLK), "EQ mode switch produced non-finite output");
        for (int i = 0; i < BLK; i++) { float d = fabsf(l[i] - prev); if (d > maxd) maxd = d; prev = l[i]; }
    }
    CHECK(maxd < 0.1f, "EQ mode switch must not click (max delta %g)", (double)maxd);
}

static void test_filters(void)
{
    puts("filters");
    static const int N[DJ_FLT_NUM_CURVES] = { 2, 4, 6, 8, 16, 32 };
    const double fc = 1000.0, n_fc = log(fc / 20.0) / log(1000.0);
    for (int c = 0; c < DJ_FLT_NUM_CURVES; c++) {
        dj_filter_t f; dj_filter_init(&f, FS, DJ_FLT_LP);
        dj_filter_set(&f, (float)n_fc, 0.0f, c); dj_filter_reset(&f);
        double at_fc = to_db(tone_gain(w_flt, &f, fc, 0, 0, 0.3));
        dj_filter_reset(&f);
        double pass = to_db(tone_gain(w_flt, &f, fc / 8.0, 0, 0, 0.3));
        dj_filter_reset(&f);
        double at_2fc = to_db(tone_gain(w_flt, &f, 2.0 * fc, 0, 0, 0.3));
        double want = -10.0 * log10(1.0 + pow(2.0, 2.0 * N[c]));
        CHECK(fabs(pass) < 0.3, "LP curve %d passband flat (got %.2f)", c, pass);
        CHECK(fabs(at_fc + 3.01) < 0.6, "LP curve %d is -3 dB at the cutoff (got %.2f)", c, at_fc);
        if (N[c] <= 16) CHECK(fabs(at_2fc - want) < 1.5, "LP curve %d at 2*fc: want %.1f got %.1f", c, want, at_2fc);
        else CHECK(at_2fc < -100.0, "LP brickwall at 2*fc must be >100 dB down (got %.1f)", at_2fc);

        dj_filter_t h; dj_filter_init(&h, FS, DJ_FLT_HP);
        dj_filter_set(&h, (float)n_fc, 0.0f, c); dj_filter_reset(&h);
        double h_fc = to_db(tone_gain(w_flt, &h, fc, 0, 0, 0.3));
        dj_filter_reset(&h);
        double h_half = to_db(tone_gain(w_flt, &h, fc / 2.0, 0, 0, 0.3));
        dj_filter_reset(&h);
        double h_pass = to_db(tone_gain(w_flt, &h, fc * 8.0, 0, 0, 0.3));
        CHECK(fabs(h_pass) < 0.3, "HP curve %d passband flat (got %.2f)", c, h_pass);
        CHECK(fabs(h_fc + 3.01) < 0.6, "HP curve %d is -3 dB at the cutoff (got %.2f)", c, h_fc);
        if (N[c] <= 16) CHECK(fabs(h_half - want) < 1.5, "HP curve %d at fc/2: want %.1f got %.1f", c, want, h_half);
        else CHECK(h_half < -100.0, "HP brickwall at fc/2 must be >100 dB down (got %.1f)", h_half);
    }
    /* resonance */
    dj_filter_t f; dj_filter_init(&f, FS, DJ_FLT_LP);
    dj_filter_set(&f, (float)n_fc, 1.0f, 0); dj_filter_reset(&f);
    double peak = -99;
    for (double ff = 700; ff < 1400; ff *= 1.03) { dj_filter_reset(&f); double g = to_db(tone_gain(w_flt, &f, ff, 0, 0, 0.02)); if (g > peak) peak = g; }
    CHECK(peak > 12.0 && peak < 22.0, "max resonance peak should be ~15-19 dB (got %.1f)", peak);
    /* stable and finite under every curve with extreme settings + noise */
    srand(7);
    for (int c = 0; c < DJ_FLT_NUM_CURVES; c++) for (int t = 0; t < 2; t++) {
        dj_filter_t x; dj_filter_init(&x, FS, t ? DJ_FLT_HP : DJ_FLT_LP);
        float l[BLK], r[BLK];
        for (int k = 0; k < 300; k++) {
            if (k % 20 == 0) dj_filter_set(&x, (float)(rand() % 1001) / 1000.0f, (float)(rand() % 2), rand() % DJ_FLT_NUM_CURVES);
            for (int i = 0; i < BLK; i++) { l[i] = (float)(rand() / (double)RAND_MAX * 2 - 1); r[i] = (float)(rand() / (double)RAND_MAX * 2 - 1); }
            dj_filter_process(&x, l, r, BLK);
            CHECK(finite_buf(l, BLK) && finite_buf(r, BLK), "filter %d/%d went non-finite", c, t);
            if (g_fail > 20) return;
        }
    }
    /* slope change must not click: steady tone, switch curves */
    dj_filter_init(&f, FS, DJ_FLT_LP); dj_filter_set(&f, 0.9f, 0.0f, 0); dj_filter_reset(&f);
    float l[BLK], r[BLK], prev = 0, maxd = 0; long t = 0;
    for (int k = 0; k < 120; k++) {
        if (k % 30 == 10) dj_filter_set(&f, 0.9f, 0.0f, (k / 30 + 1) % DJ_FLT_NUM_CURVES);
        for (int i = 0; i < BLK; i++, t++) l[i] = r[i] = (float)(0.3 * sin(2 * DJ_PI * 300.0 * t / FS));
        dj_filter_process(&f, l, r, BLK);
        if (k > 5) for (int i = 0; i < BLK; i++) { float d = fabsf(l[i] - prev); if (d > maxd) maxd = d; prev = l[i]; } else prev = l[BLK - 1];
    }
    CHECK(maxd < 0.1f, "slope change must not click (max delta %g)", (double)maxd);

    /* stricter (0.2.0 device report: an audible click on a curve change). A 300 Hz tone well inside the passband, a change every
     * 300 ms through every slope and back, then a change on EVERY block (a fast Q-Link spin): the output must never dip (the old
     * duck muted it for ~4 ms) and never step faster than the tone itself does (0.3 * 2 pi 300 / 44100 = 0.0128 per sample). */
    for (int hp = 0; hp < 2; hp++) {
        float cut = hp ? 0.05f : 0.9f;                      /* HP at ~28 Hz, LP at ~10 kHz: the tone passes both */
        dj_filter_init(&f, FS, hp ? DJ_FLT_HP : DJ_FLT_LP); dj_filter_set(&f, cut, 0.0f, 0); dj_filter_reset(&f);
        float pd = 0, md = 0, env_min = 1, prev2 = 0; t = 0;
        int curve = 0;
        for (int k = 0; k < 900; k++) {
            if (k >= 100 && k < 400 && k % 100 == 0) curve = (curve + 1) % DJ_FLT_NUM_CURVES;
            if (k >= 400 && k < 700) curve = (k / 2) % DJ_FLT_NUM_CURVES;            /* a new curve every other block */
            if (k == 700) curve = 0;
            dj_filter_set(&f, cut, 0.0f, curve);
            for (int i = 0; i < BLK; i++, t++) l[i] = r[i] = (float)(0.3 * sin(2 * DJ_PI * 300.0 * t / FS));
            dj_filter_process(&f, l, r, BLK);
            if (k > 60) {
                float bp = 0;
                for (int i = 0; i < BLK; i++) { float d = fabsf(l[i] - prev2); if (d > md) md = d; prev2 = l[i]; if (fabsf(l[i]) > bp) bp = fabsf(l[i]); }
                if (k % 2 == 1) { if (bp > pd) pd = bp; }    /* peak over two blocks (a 300 Hz period is 147 frames) */
                else { if (pd < env_min) env_min = pd; pd = bp; }
            } else prev2 = l[BLK - 1];
        }
        CHECK(md < 0.02f, "%s: curve changes are smooth (max step %g per sample, the tone alone 0.0128)", hp ? "HP" : "LP", (double)md);
        CHECK(env_min > 0.2f, "%s: curve changes never dip the level (lowest peak %g of 0.3)", hp ? "HP" : "LP", (double)env_min);
    }
    /* changed and changed back while the new slope is still warming up: nothing heard, the old slope stays */
    dj_filter_init(&f, FS, DJ_FLT_LP); dj_filter_set(&f, 0.5f, 0.0f, 2); dj_filter_reset(&f);
    for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.0f;
    dj_filter_process(&f, l, r, BLK);
    dj_filter_set(&f, 0.5f, 0.0f, 4); dj_filter_process(&f, l, r, 8);
    dj_filter_set(&f, 0.5f, 0.0f, 2); dj_filter_process(&f, l, r, 8);
    CHECK(f.curve == 2 && f.next_curve < 0, "a change undone during the warm-up is dropped (curve %d, next %d)", f.curve, f.next_curve);
    dj_filter_set(&f, 0.5f, 0.0f, 5);
    for (int k = 0; k < 20; k++) dj_filter_process(&f, l, r, BLK);
    CHECK(f.curve == 5 && f.sections == 16 && f.next_curve < 0, "a change completes within 35 ms (curve %d)", f.curve);
}

static void test_gate(void)
{
    puts("gate");
    dj_gate_t g; dj_gate_init(&g, FS);
    dj_gate_set(&g, -30.0f, 0.5f, 100.0f, 50.0f);
    float l[BLK], r[BLK]; long t = 0;
    /* loud passes */
    for (int k = 0; k < 40; k++) { for (int i = 0; i < BLK; i++, t++) l[i] = r[i] = (float)(0.5 * sin(2 * DJ_PI * 200.0 * t / FS)); dj_gate_process(&g, l, r, BLK); }
    CHECK(g.gain == 1.0f, "gate open for loud signal");
    /* quiet: must stay open through the hold, then close */
    int closed_at = -1;
    for (int k = 0; k < 200 && closed_at < 0; k++) {
        for (int i = 0; i < BLK; i++, t++) l[i] = r[i] = (float)(0.001 * sin(2 * DJ_PI * 200.0 * t / FS));
        dj_gate_process(&g, l, r, BLK);
        if (g.gain < 0.5f) closed_at = k * BLK;
    }
    CHECK(closed_at > (int)(0.100 * FS), "gate must hold for >=100 ms before closing (closed at %d samples)", closed_at);
    CHECK(closed_at > 0 && closed_at < (int)(0.6 * FS), "gate must close eventually (closed at %d)", closed_at);
    for (int k = 0; k < 200; k++) { for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.001f; dj_gate_process(&g, l, r, BLK); }
    CHECK(l[0] == 0.0f && r[0] == 0.0f, "closed gate is exact silence");
    /* linked stereo: loud left, quiet right -> the SAME gain on both, image cannot shift */
    dj_gate_reset(&g);
    float maxdiff = 0.0f;
    for (int k = 0; k < 100; k++) {
        for (int i = 0; i < BLK; i++, t++) { l[i] = (float)(0.5 * sin(2 * DJ_PI * 200.0 * t / FS)); r[i] = 0.5f * l[i] * 0.01f / 0.5f; }
        float inl[BLK], inr[BLK]; memcpy(inl, l, sizeof l); memcpy(inr, r, sizeof r);
        dj_gate_process(&g, l, r, BLK);
        for (int i = 0; i < BLK; i++) if (fabsf(inl[i]) > 0.05f) { float gl = l[i] / inl[i], gr = r[i] / inr[i]; if (fabsf(gl - gr) > maxdiff) maxdiff = fabsf(gl - gr); }
    }
    CHECK(maxdiff < 1e-4f, "gate gain must be identical on both channels (diff %g)", (double)maxdiff);
    /* hysteresis: signal sitting just under the open threshold must not chatter once open */
    dj_gate_reset(&g); dj_gate_set(&g, -30.0f, 0.5f, 0.0f, 20.0f);
    int transitions = 0, last = 1;
    for (int k = 0; k < 100; k++) {
        for (int i = 0; i < BLK; i++, t++) { float a = (float)(0.0316 * 0.8 * sin(2 * DJ_PI * 100.0 * t / FS)); l[i] = r[i] = (k < 2 ? 0.5f * (float)sin(2 * DJ_PI * 100.0 * t / FS) : a); }
        dj_gate_process(&g, l, r, BLK);
        if (k > 10) { if (g.open != last) transitions++; last = g.open; } else last = g.open;
    }
    CHECK(transitions <= 1, "gate chattered %d times", transitions);
}

static void test_meter_taper(void)
{
    puts("meter+taper");
    dj_meter_t m; dj_meter_init(&m, FS);
    float l[BLK], r[BLK];
    for (int i = 0; i < BLK; i++) { l[i] = 1.0f; r[i] = 0.1f; }
    dj_meter_push(&m, l, r, BLK);
    CHECK(fabsf(dj_meter_value01(&m, 0) - 1.0f) < 1e-4f, "0 dBFS = full scale");
    CHECK(fabsf(dj_meter_value01(&m, 1) - (1.0f - 20.0f / 60.0f)) < 1e-3f, "-20 dBFS");
    for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.0f;
    for (int k = 0; k < 1100; k++) dj_meter_push(&m, l, r, BLK);   /* ~3.2 s at 24 dB/s = -77 dB, below the -60 dB floor */
    CHECK(dj_meter_value01(&m, 0) == 0.0f, "meter falls to zero");
    CHECK(lroundf(dj_meter_value01(&m, 0) * (DJ_METER_FRAMES - 1)) == 0, "silence = frame 0");

    CHECK(dj_taper_gain_db(0.0f, DJ_IN_N0, DJ_IN_MAX_DB) <= -100.0f, "input gain n=0 is -inf");
    CHECK(fabsf(dj_taper_gain_db(DJ_IN_N0, DJ_IN_N0, DJ_IN_MAX_DB)) < 1e-4f, "input gain unity at n0");
    CHECK(fabsf(dj_taper_gain_db(1.0f, DJ_IN_N0, DJ_IN_MAX_DB) - 24.0f) < 1e-4f, "input gain max +24");
    CHECK(fabsf(dj_taper_gain_db(1.0f, DJ_OUT_N0, DJ_OUT_MAX_DB) - 12.0f) < 1e-4f, "output max +12");
    float prev = -300.0f; int mono = 1;
    for (int i = 0; i <= 1000; i++) { float d = dj_taper_gain_db(i / 1000.0f, DJ_IN_N0, DJ_IN_MAX_DB); if (d < prev) mono = 0; prev = d; }
    CHECK(mono, "gain taper is monotonic");
    for (int i = 0; i <= 20; i++) {
        float n = i / 20.0f, db = dj_taper_gain_db(n, DJ_OUT_N0, DJ_OUT_MAX_DB);
        if (db > -100.0f && n > 0.05f) CHECK(fabsf(dj_taper_gain_n_from_db(db, DJ_OUT_N0, DJ_OUT_MAX_DB) - n) < 1e-3f, "taper inverse at n=%.2f", (double)n);
    }
    char b[32];
    dj_fmt_gain(b, sizeof b, -200.0f); CHECK(!strcmp(b, "-inf dB"), "'%s'", b);
    dj_fmt_hz(b, sizeof b, 1500.0f); CHECK(!strcmp(b, "1.50 kHz"), "'%s'", b);
    dj_fmt_balance(b, sizeof b, 0.5f); CHECK(!strcmp(b, "C"), "'%s'", b);
    dj_fmt_eq(b, sizeof b, -200.0f); CHECK(!strcmp(b, "KILL"), "'%s'", b);
    char tiny[4]; int w = dj_fmt_hz(tiny, sizeof tiny, 12345.0f); CHECK(w < 4 && tiny[w] == 0, "fmt never overruns");
}

/* ---- maximizer: a zero-latency limiter into a fixed 0 dBFS ceiling, threshold = how far the signal is raised ---- */
static float peak_of(const float *a, int n) { float p = 0; for (int i = 0; i < n; i++) if (fabsf(a[i]) > p) p = fabsf(a[i]); return p; }

static void test_maximizer(void)
{
    puts("maximizer");
    dj_maximizer_t m; float l[BLK], r[BLK]; long t = 0;

    /* defaults: an exact pass-through for anything at or below full scale */
    dj_max_init(&m, FS);
    int exact = 1;
    for (int k = 0; k < 100; k++) {
        float il[BLK], ir[BLK];
        for (int i = 0; i < BLK; i++, t++) { l[i] = il[i] = (float)(0.99 * sin(2 * DJ_PI * 333.0 * t / FS)); r[i] = ir[i] = (float)(rand() / (double)RAND_MAX * 2 - 1); }
        dj_max_process(&m, l, r, BLK);
        if (memcmp(l, il, sizeof l) || memcmp(r, ir, sizeof r)) exact = 0;
    }
    CHECK(exact, "defaults (threshold 0 dB, output 0 dB) must pass everything at or under full scale untouched");

    /* the ceiling is hard: nothing leaves above 0 dBFS, whatever the input level, attack, release or threshold */
    int ceiling_ok = 1; float worst = 0;
    const float atts[] = { 0.01f, 0.05f, 0.5f, 5.0f }, rels[] = { 5.0f, 100.0f, 2000.0f }, thrs[] = { 0.0f, -12.0f, -30.0f };
    for (int a = 0; a < 4; a++) for (int rr = 0; rr < 3; rr++) for (int th = 0; th < 3; th++) {
        dj_max_init(&m, FS); dj_max_set(&m, thrs[th], atts[a], rels[rr], 0.0f); dj_max_reset(&m);
        srand(7);
        for (int k = 0; k < 120; k++) {
            for (int i = 0; i < BLK; i++, t++) {
                double amp = (k / 20) % 2 ? 8.0 : 0.3;               /* bursts far above full scale, then quiet */
                l[i] = (float)(amp * sin(2 * DJ_PI * 90.0 * t / FS)); r[i] = (float)(amp * (rand() / (double)RAND_MAX * 2 - 1));
            }
            dj_max_process(&m, l, r, BLK);
            float p = fmaxf(peak_of(l, BLK), peak_of(r, BLK));
            if (p > worst) worst = p;
            if (p > 1.0f) ceiling_ok = 0;
        }
    }
    CHECK(ceiling_ok, "output exceeded the 0 dBFS ceiling (worst %.9f)", (double)worst);

    /* the output trim lowers the ceiling, never raises it */
    dj_max_init(&m, FS); dj_max_set(&m, -12.0f, 0.05f, 100.0f, -6.0f); dj_max_reset(&m);
    for (int k = 0; k < 200; k++) {
        for (int i = 0; i < BLK; i++, t++) l[i] = r[i] = (float)(3.0 * sin(2 * DJ_PI * 150.0 * t / FS));
        dj_max_process(&m, l, r, BLK);
    }
    CHECK(peak_of(l, BLK) <= 0.5012f + 1e-4f && peak_of(l, BLK) > 0.49f, "output -6 dB -> ceiling at -6 dBFS (peak %.4f)", (double)peak_of(l, BLK));

    /* threshold: a signal exactly at the threshold is raised to exactly the ceiling */
    dj_max_init(&m, FS); dj_max_set(&m, -12.0f, 0.05f, 100.0f, 0.0f); dj_max_reset(&m);
    float amp = powf(10.0f, -12.0f / 20.0f);
    for (int k = 0; k < 120; k++) { for (int i = 0; i < BLK; i++, t++) l[i] = r[i] = amp * (float)sin(2 * DJ_PI * 100.0 * t / FS); dj_max_process(&m, l, r, BLK); }
    CHECK(fabsf(peak_of(l, BLK) - 1.0f) < 0.01f, "threshold -12 dB with a -12 dBFS tone -> output at the ceiling (peak %.4f)", (double)peak_of(l, BLK));

    /* linked stereo: loud left, tiny right -> the SAME gain on both channels, so the balance cannot move */
    dj_max_init(&m, FS); dj_max_set(&m, 0.0f, 0.05f, 200.0f, 0.0f); dj_max_reset(&m);
    float maxdiff = 0;
    for (int k = 0; k < 100; k++) {
        float il[BLK], ir[BLK];
        for (int i = 0; i < BLK; i++, t++) { l[i] = il[i] = (float)(0.8 * sin(2 * DJ_PI * 60.0 * t / FS)); r[i] = ir[i] = 0.01f * il[i]; }
        for (int i = 0; i < BLK; i++) { l[i] *= 4.0f; il[i] *= 4.0f; r[i] *= 4.0f; ir[i] *= 4.0f; }   /* left now peaks at 3.2: limited; right at 0.032 */
        dj_max_process(&m, l, r, BLK);
        for (int i = 0; i < BLK; i++) if (fabsf(il[i]) > 0.2f && fabsf(l[i]) < 0.999f) {
            float gl = l[i] / il[i], gr = r[i] / ir[i];
            if (fabsf(gl - gr) > maxdiff) maxdiff = fabsf(gl - gr);
        }
    }
    CHECK(maxdiff < 1e-4f, "limiter gain must be identical on both channels (diff %g)", (double)maxdiff);

    /* release: after a burst the gain returns to unity and quiet material passes untouched again */
    dj_max_init(&m, FS); dj_max_set(&m, 0.0f, 0.05f, 50.0f, 0.0f); dj_max_reset(&m);
    for (int k = 0; k < 30; k++) { for (int i = 0; i < BLK; i++, t++) l[i] = r[i] = 4.0f * (float)sin(2 * DJ_PI * 200.0 * t / FS); dj_max_process(&m, l, r, BLK); }
    float in_pk, in_pk_r, gr_db; dj_max_take_stats(&m, &in_pk, &in_pk_r, &gr_db);
    CHECK(gr_db > 11.0f && gr_db < 12.5f, "a 4x (+12 dB) burst should show ~12 dB of reduction (got %.2f)", (double)gr_db);
    CHECK(fabsf(in_pk - 4.0f) < 0.02f, "input peak statistic (got %.3f)", (double)in_pk);
    int back = 0;
    for (int k = 0; k < 600 && !back; k++) {   /* 50 ms release: ~0.6 s to snap back to exactly 1 */
        float il[BLK];
        for (int i = 0; i < BLK; i++) l[i] = r[i] = il[i] = 0.1f;
        dj_max_process(&m, l, r, BLK);
        if (memcmp(l, il, sizeof l) == 0) back = 1;
    }
    CHECK(back, "gain must recover to exact unity after the burst");
    dj_max_take_stats(&m, &in_pk, &in_pk_r, &gr_db);
    for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.1f;
    dj_max_process(&m, l, r, BLK); dj_max_take_stats(&m, &in_pk, &in_pk_r, &gr_db);
    CHECK(gr_db == 0.0f, "no reduction reported once recovered (got %g)", (double)gr_db);

    /* output at -inf is exact silence; NaN in gives finite out */
    dj_max_init(&m, FS); dj_max_set(&m, 0.0f, 0.05f, 100.0f, -200.0f); dj_max_reset(&m);
    for (int i = 0; i < BLK; i++) l[i] = r[i] = 0.5f;
    dj_max_process(&m, l, r, BLK);
    CHECK(l[BLK - 1] == 0.0f && r[BLK - 1] == 0.0f, "output -inf is silent");
    dj_max_init(&m, FS);
    for (int i = 0; i < BLK; i++) { l[i] = 0.5f; r[i] = NAN; }
    l[3] = INFINITY;
    dj_max_process(&m, l, r, BLK);
    CHECK(finite_buf(l, BLK) && finite_buf(r, BLK) && peak_of(l, BLK) <= 1.0f, "NaN/inf in must not produce NaN or > ceiling out");

    /* moving the threshold while audio runs must not click: the makeup gain glides */
    dj_max_init(&m, FS); dj_max_set(&m, 0.0f, 0.05f, 100.0f, 0.0f); dj_max_reset(&m);
    float prev = 0, maxd = 0; int first = 1;
    for (int k = 0; k < 60; k++) {
        if (k == 20) dj_max_set(&m, -24.0f, 0.05f, 100.0f, 0.0f);
        for (int i = 0; i < BLK; i++, t++) l[i] = r[i] = 0.05f * (float)sin(2 * DJ_PI * 100.0 * t / FS);
        dj_max_process(&m, l, r, BLK);
        for (int i = 0; i < BLK; i++) { if (!first && fabsf(l[i] - prev) > maxd) maxd = fabsf(l[i] - prev); prev = l[i]; first = 0; }
    }
    CHECK(maxd < 0.05f, "threshold move must glide, not step (max sample delta %g)", (double)maxd);

    /* tapers and text */
    CHECK(fabsf(dj_taper_mx_thresh_db(1.0f)) < 1e-6f && fabsf(dj_taper_mx_thresh_db(0.0f) + 30.0f) < 1e-4f, "threshold range 0..-30 dB");
    CHECK(fabsf(dj_taper_mx_attack_ms(0.0f) - 0.01f) < 1e-6f && fabsf(dj_taper_mx_attack_ms(1.0f) - 5.0f) < 1e-3f, "attack range 0.01..5 ms");
    CHECK(fabsf(dj_taper_mx_out_db(1.0f)) < 1e-4f && dj_taper_mx_out_db(0.0f) <= -100.0f, "output 0 dB at the top, -inf at the bottom");
    float pv = -300.0f; int mono = 1;
    for (int i = 0; i <= 1000; i++) { float d = dj_taper_mx_out_db(i / 1000.0f); if (d < pv || d > 1e-4f) mono = 0; pv = d; }
    CHECK(mono, "output trim is monotonic and never above 0 dB");
    CHECK(fabsf(dj_taper_thresh_db(0.0f) + 80.0f) < 1e-4f, "gate threshold minimum is -80 dB");
    char b[32];
    dj_fmt_db(b, sizeof b, 0.0f); CHECK(!strcmp(b, "0.0 dB"), "'%s'", b);
    dj_fmt_db(b, sizeof b, -0.01f); CHECK(!strcmp(b, "0.0 dB"), "no \"-0.0\": '%s'", b);
    dj_fmt_db(b, sizeof b, -12.04f); CHECK(!strcmp(b, "-12.0 dB"), "'%s'", b);
    dj_fmt_db(b, sizeof b, -200.0f); CHECK(!strcmp(b, "-inf dB"), "'%s'", b);

    /* the maximizer's drive meter: from 2 dB before limiting (-2) to 18 dB in, instant rise, held 0.3 s, then 24 dB/s fall */
    CHECK(DJ_GR_START_DB == -2.0f, "the reduction meter starts 2 dB before limiting");
    dj_grmeter_t g; dj_grmeter_init(&g, FS);
    CHECK(dj_grmeter_value01(&g) == 0.0f, "dark at rest");
    dj_grmeter_push(&g, -2.6f, BLK);
    CHECK(dj_grmeter_value01(&g) * DJ_METER_SEGS < 0.5f, "more than 2 dB below the ceiling: dark (%g)", (double)dj_grmeter_value01(&g));
    dj_grmeter_push(&g, -1.4f, BLK);
    CHECK(lroundf(dj_grmeter_value01(&g) * DJ_METER_SEGS) == 1, "1.4 dB below the ceiling: the first segment lights (warning)");
    dj_grmeter_push(&g, 8.0f, BLK);
    CHECK(fabsf(dj_grmeter_value01(&g) - 0.5f) < 1e-4f, "8 dB of reduction = half scale");
    for (int k = 0; k < (int)(0.25f * FS / BLK); k++) dj_grmeter_push(&g, -100.0f, BLK);
    CHECK(fabsf(dj_grmeter_value01(&g) - 0.5f) < 1e-4f, "held for 0.25 s, so a 100 ms screen poll still sees it");
    for (int k = 0; k < 30; k++) dj_grmeter_push(&g, -100.0f, BLK);
    CHECK(dj_grmeter_value01(&g) < 0.5f, "then it falls");
    dj_grmeter_push(&g, 40.0f, BLK);
    CHECK(dj_grmeter_value01(&g) == 1.0f, "beyond 18 dB clamps");
    for (int k = 0; k < 1100; k++) dj_grmeter_push(&g, -100.0f, BLK);
    CHECK(dj_grmeter_value01(&g) == 0.0f, "reduction meter falls back to zero");
    CHECK(DJ_METER_FRAMES == 21 && DJ_METER_SEGS * 3 == 60 && (int)DJ_GR_RANGE_DB == DJ_METER_SEGS, "20 segments: 3 dB each for levels, 1 dB each for reduction");
}

static void test_stress_and_timing(void)
{
    puts("stress/timing/determinism");
    /* the full chain order, random controls, noise in: always finite; and bit-identical on a second run */
    float out1[2][BLK * 400], out2[2][BLK * 400];
    for (int run = 0; run < 2; run++) {
        srand(12345);
        dj_gain_t gi; dj_phase_t ph; dj_width_t wd; dj_bassmono_t bm; dj_balance_t ba; dj_eq_t eq;
        dj_filter_t lp, hp; dj_gate_t gt; dj_maximizer_t mx;
        dj_gain_init(&gi, FS); dj_phase_init(&ph, FS); dj_width_init(&wd, FS); dj_bassmono_init(&bm, FS); dj_balance_init(&ba, FS);
        dj_eq_init(&eq, FS); dj_filter_init(&lp, FS, DJ_FLT_LP); dj_filter_init(&hp, FS, DJ_FLT_HP); dj_gate_init(&gt, FS); dj_max_init(&mx, FS);
        float (*out)[BLK * 400] = run ? out2 : out1;
        float l[BLK], r[BLK];
        for (int k = 0; k < 400; k++) {
            if (k % 7 == 0) {
                dj_gain_set_db(&gi, (float)(rand() % 125) - 100.0f); dj_phase_set(&ph, rand() & 1); dj_width_set(&wd, (rand() % 101) / 100.0f);
                dj_bassmono_set_freq(&bm, 40.0f + (float)(rand() % 360)); dj_balance_set(&ba, (rand() % 201 - 100) / 100.0f);
                dj_eq_set_mode(&eq, (dj_eq_mode_t)(rand() & 1)); dj_eq_set(&eq, (rand() % 101) / 100.0f, (rand() % 101) / 100.0f, (rand() % 101) / 100.0f);
                dj_filter_set(&lp, (rand() % 1001) / 1000.0f, (rand() % 101) / 100.0f, rand() % 6);
                dj_filter_set(&hp, (rand() % 1001) / 1000.0f, (rand() % 101) / 100.0f, rand() % 6);
                dj_gate_set(&gt, -80.0f + (float)(rand() % 80), 0.1f + (float)(rand() % 50), (float)(rand() % 500), 5.0f + (float)(rand() % 2000));
                dj_max_set(&mx, -(float)(rand() % 31), 0.01f + (float)(rand() % 50) / 10.0f, 5.0f + (float)(rand() % 2000), -(float)(rand() % 40));
            }
            for (int i = 0; i < BLK; i++) { l[i] = (float)(rand() / (double)RAND_MAX * 2 - 1); r[i] = (float)(rand() / (double)RAND_MAX * 2 - 1); }
            dj_gain_process(&gi, l, r, BLK); dj_phase_process(&ph, l, r, BLK); dj_width_process(&wd, l, r, BLK);
            dj_bassmono_process(&bm, l, r, BLK); dj_balance_process(&ba, l, r, BLK); dj_eq_process(&eq, l, r, BLK);
            dj_filter_process(&lp, l, r, BLK); dj_filter_process(&hp, l, r, BLK); dj_gate_process(&gt, l, r, BLK); dj_max_process(&mx, l, r, BLK);
            CHECK(finite_buf(l, BLK) && finite_buf(r, BLK), "full chain non-finite at block %d", k);
            CHECK(peak_of(l, BLK) <= 1.0f && peak_of(r, BLK) <= 1.0f, "chain output above the 0 dBFS ceiling at block %d", k);
            memcpy(out[0] + k * BLK, l, sizeof l); memcpy(out[1] + k * BLK, r, sizeof r);
            if (g_fail > 10) return;
        }
    }
    CHECK(memcmp(out1, out2, sizeof out1) == 0, "two identical runs must be bit-identical (deterministic)");

    /* CPU: worst case = everything on, brickwall LP + HP, EQ isolator, per 128-frame block */
    dj_bassmono_t bm; dj_eq_t eq; dj_filter_t lp, hp; dj_gate_t gt; dj_maximizer_t mx;
    dj_bassmono_init(&bm, FS); dj_eq_init(&eq, FS); dj_filter_init(&lp, FS, DJ_FLT_LP); dj_filter_init(&hp, FS, DJ_FLT_HP); dj_gate_init(&gt, FS); dj_max_init(&mx, FS); dj_max_set(&mx, -12.0f, 0.05f, 100.0f, 0.0f);
    dj_filter_set(&lp, 0.7f, 0.5f, 5); dj_filter_set(&hp, 0.3f, 0.5f, 5);
    float l[BLK], r[BLK]; srand(1);
    for (int i = 0; i < BLK; i++) { l[i] = (float)(rand() / (double)RAND_MAX - 0.5); r[i] = l[i]; }
    struct timespec a, b; clock_gettime(CLOCK_MONOTONIC, &a);
    const int blocks = 4000;
    for (int k = 0; k < blocks; k++) { dj_bassmono_process(&bm, l, r, BLK); dj_eq_process(&eq, l, r, BLK); dj_filter_process(&lp, l, r, BLK); dj_filter_process(&hp, l, r, BLK); dj_gate_process(&gt, l, r, BLK); dj_max_process(&mx, l, r, BLK); for (int i = 0; i < BLK; i++) { l[i] = (float)(rand() / (double)RAND_MAX - 0.5); r[i] = l[i]; } }
    clock_gettime(CLOCK_MONOTONIC, &b);
    double us = ((b.tv_sec - a.tv_sec) * 1e9 + (b.tv_nsec - a.tv_nsec)) / 1e3 / blocks;
    printf("  worst-case chain (sanitizer build, this x86 host): %.1f us per 128-frame block (budget ~2900 us)\n", us);
}

static void test_readout(void)
{
    puts("dB readout (frames, 1 s hold)");
    struct { float db; int fi, fd; } T[] = {
        { -1000.0f, 0, 10 }, { -60.5f, 0, 10 }, { -60.0f, 1, 0 }, { -59.96f, 1, 0 }, { -12.34f, 49, 3 }, { -1.0f, 60, 0 },
        { -0.94f, 61, 9 }, { -0.04f, 62, 0 }, { 0.0f, 62, 0 }, { 2.0f, 64, 0 }, { 2.04f, 64, 0 }, { 24.9f, 86, 9 }, { 80.0f, 86, 9 },
    };
    for (unsigned k = 0; k < sizeof T / sizeof T[0]; k++) {
        int fi, fd; dj_readout_frames(T[k].db, &fi, &fd);
        CHECK(fi == T[k].fi && fd == T[k].fd, "%.2f dB -> frames %d,%d (want %d,%d)", (double)T[k].db, fi, fd, T[k].fi, T[k].fd);
    }
    { int fi, fd; dj_readout_frames(NAN, &fi, &fd); CHECK(fi == 0 && fd == 10, "NaN reads -inf"); }
    CHECK(fabs(dj_readout_int01(-1000.0f)) < 1e-12 && fabs(dj_readout_int01(24.9f) - 1.0) < 1e-12, "int strip ends at 0 and 1");
    CHECK(DJ_READOUT_INT_FRAMES == 87 && DJ_READOUT_DEC_FRAMES == 11, "strip frame counts");

    dj_readout_t r; dj_readout_init(&r, FS);
    CHECK(r.held_db < -999.0f, "starts at -inf");
    dj_readout_push(&r, 2.0f, BLK); CHECK(r.held_db == 2.0f, "a peak shows at once");
    int blocks_1s = (int)(FS / BLK);
    for (int k = 0; k < blocks_1s - 2; k++) dj_readout_push(&r, -10.0f, BLK);
    CHECK(r.held_db == 2.0f, "held for a second");
    dj_readout_push(&r, 1.0f, BLK); CHECK(r.held_db == 2.0f, "a lower peak inside the hold does not replace it");
    for (int k = 0; k < 4; k++) dj_readout_push(&r, -10.0f, BLK);
    CHECK(r.held_db == 1.0f, "after the hold: the loudest peak of the second gone (%g)", (double)r.held_db);
    dj_readout_push(&r, 6.0f, BLK); CHECK(r.held_db == 6.0f, "a new high replaces it at once");
    for (int k = 0; k < 3 * blocks_1s; k++) dj_readout_push(&r, -1000.0f, BLK);
    CHECK(r.held_db < -999.0f, "silence falls back to -inf");
}


/* 2.1.1: the transient shaper and the compressor alone: hostile input, extreme settings, odd block lengths */
static void test_transient_comp(void)
{
    puts("transient shaper + compressor (2.1.1)");
    dj_transient_t t; dj_comp_t c;
    float l[DJ_BLOCK], r[DJ_BLOCK];
    for (int m = 0; m < 4; m++) {
        dj_transient_init(&t, DJ_FS); dj_comp_init(&c, DJ_FS);
        dj_transient_set(&t, m & 1 ? 1.0f : -1.0f, 1.0f, m & 2 ? 1.0f : -1.0f, m * 0.33f, m & 1, 1, m & 2);
        dj_comp_set(&c, -60.0f, 20.0f, 0.1f, 5.0f, m & 1, m & 2);
        int ok = 1; srand(11 + m);
        for (int it = 0; it < 2000; it++) {
            int n = 1 + rand() % DJ_BLOCK;
            for (int i = 0; i < n; i++) {
                float v = (rand() % 2001 - 1000) / 100.0f;               /* up to +20 dBFS */
                if (it % 97 == 3 && i == 0) v = NAN;
                if (it % 89 == 5 && i == 1) v = INFINITY;
                l[i] = v; r[i] = -v * 0.5f;
                if (it % 300 > 200) l[i] = r[i] = 0.0f;                  /* and silence */
            }
            dj_transient_process(&t, l, r, n);
            dj_comp_process(&c, l, r, n);
            if (!finite_buf(l, n) || !finite_buf(r, n)) ok = 0;
            float g, d; dj_comp_take_stats(&c, &g, &d); if (!(g >= 0.0f) || g != g) ok = 0;
        }
        CHECK(ok, "mode %d: NaN, inf, +20 dBFS, silence, any block length: always finite", m);
        CHECK(t.auto_g > 0.0 && t.auto_g <= 1.0 && c.gr >= 0.0, "mode %d: gains in range (auto %g, gr %g)", m, t.auto_g, c.gr);
    }
    /* the CLIP shape: untouched under the knee, never past full scale, odd */
    float k = dj_db_to_lin(-6.0f); int ok = 1;
    for (int i = 0; i <= 4000; i++) { float x = i / 100.0f; float y = dj_soft_clip(x, k);
        if (y > 1.0f || y < 0.0f || (x <= k && y != x) || dj_soft_clip(-x, k) != -y) ok = 0; }
    CHECK(ok, "soft clip: identity under the knee, <= 0 dBFS above, odd");
    /* 2.1.2: CLIP is at the compressor's INPUT: a +12 dB tone, THRESHOLD -6 dB, 2:1. Clipped first (and, 2.1.4, lowered 1 dB), the
     * compressor sees about -1 dB (2.5 dB of reduction: about -3.5 dB out); clipped after, +12 dB (9 dB: +3 dB, then clipped). */
    {
        dj_comp_init(&c, DJ_FS); dj_comp_set(&c, -6.0f, 2.0f, 0.1f, 2000.0f, 0, 1);
        float pk = 0.0f;
        for (int b = 0; b < 400; b++) {
            for (int i = 0; i < DJ_BLOCK; i++) { l[i] = r[i] = 4.0f * sinf(2.0f * 3.14159265f * 441.0f * (b * DJ_BLOCK + i) / DJ_FS); }
            dj_comp_process(&c, l, r, DJ_BLOCK);
            if (b == 399) for (int i = 0; i < DJ_BLOCK; i++) pk = fmaxf(pk, fabsf(l[i]));
        }
        CHECK(20.0f * log10f(pk) < -2.8f && 20.0f * log10f(pk) > -4.2f, "CLIP before the compressor: about -3.5 dB out (%.2f dB)", 20.0f * log10f(pk));
    }
    /* compressor at rest (1:1): exact pass-through */
    dj_comp_init(&c, DJ_FS); dj_comp_set(&c, -30.0f, 1.0f, 1.0f, 50.0f, 0, 0);
    for (int i = 0; i < DJ_BLOCK; i++) { l[i] = sinf(i * 0.1f) * 0.9f; r[i] = l[i] * 0.5f; }
    float l0[DJ_BLOCK]; memcpy(l0, l, sizeof l);
    dj_comp_process(&c, l, r, DJ_BLOCK);
    CHECK(!memcmp(l0, l, sizeof l), "compressor at 1:1: bit-exact pass-through");
}


/* 2.1.4: FLIP ducks while the sound is loud, so its gain must turn smoothly: a 100 Hz tone swelling from -60 to -6 dB and back (no
 * click of its own), threshold -10 dB, attack 0.1 ms. A click shows as a spike in the output's 2nd difference. */
static double flip_click(int flip)
{
    dj_gate_t g; dj_gate_init(&g, DJ_FS); dj_gate_set_flip(&g, flip); dj_gate_set(&g, -10.0f, 0.1f, 50.0f, 100.0f); dj_gate_reset(&g);
    float l[1], r[1]; double y0 = 0, y1 = 0, x0 = 0, x1 = 0, wo = 0, wi = 0;
    for (int i = 0; i < 88200; i++) {
        double t = i / 44100.0, e = 0.5 * (1 - cos(2 * M_PI * t)), x = pow(10, (-60 + 54 * e) / 20) * sin(2 * M_PI * 100 * t);
        l[0] = r[0] = (float)x; dj_gate_process(&g, l, r, 1);
        if (i > 2) { double d = fabs(l[0] - 2 * y1 + y0), di = fabs(x - 2 * x1 + x0); if (d > wo) wo = d; if (di > wi) wi = di; }
        y0 = y1; y1 = l[0]; x0 = x1; x1 = x;
    }
    return wo / wi;
}
static void test_gate_flip_click(void)
{
    puts("gate FLIP: no click (2.1.4)");
    double c = flip_click(1);
    CHECK(c < 1.5, "FLIP at 0.1 ms attack: the gain turns no sharper than the tone itself (%.2f x)", c);
}

int main(void)
{
    test_gain(); test_phase(); test_width(); test_balance(); test_bassmono(); test_eq();
    test_filters(); test_gate(); test_maximizer(); test_meter_taper(); test_readout(); test_transient_comp(); test_gate_flip_click(); test_stress_and_timing();
    printf("\n%d checks, %d failed\n", g_run, g_fail);
    puts(g_fail ? "FAILED" : "PASSED");
    return g_fail ? 1 : 0;
}
