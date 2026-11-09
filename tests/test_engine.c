/* test_engine.c -- the plugin through its real mpc_engine() interface (set_param / get_param / process), exactly as
 * wrapper/vst2_wrap.c drives it: defaults, parameter round trips, the "state" string MPC stores for its own presets and
 * projects, the gate/maximizer behaviour at the plugin level, meters and filter-curve codes.
 * Build/run: tests/run_tests.sh. Exit status 0 = all passed. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine.h"
#include "../src/dsp/dj_view.h"
#include "../src/dsp/dj_meter.h"
#include "../src/dsp/dj_taper.h"
#include "../src/dsp/dj_readout.h"

/* the output-trim knob position that gives -6 dB: found by search so the test follows the taper */
static double dj_taper_n6(void) { double best = 0, bd = 1e9; for (int i = 0; i <= 10000; i++) { double n = i / 10000.0; double d = fabs(dj_taper_mx_out_db((float)n) + 6.0); if (d < bd) { bd = d; best = n; } } return best; }

static int g_fail = 0, g_run = 0;
#define CHECK(cond, ...) do { g_run++; if (!(cond)) { g_fail++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* the real table, from the single .def */
typedef struct { const char *key; const char *kind; double def; const char *opts; } row_t;
static const row_t ROWS[] = {
#define DJP(ID, key, name, kind, def, opts) { key, #kind, def, opts },
#include "../src/engine/dj_params.def"
#undef DJP
};
#define NROWS ((int)(sizeof ROWS / sizeof ROWS[0]))

static const mpc_engine_t *E;

static void setp(void *i, const char *k, const char *v) { E->set_param(i, k, v); }
static void setd(void *i, const char *k, double v) { char b[40]; snprintf(b, sizeof b, "%.6f", v); setp(i, k, b); }
static void gets_(void *i, const char *k, char *b, int n) { b[0] = 0; E->get_param(i, k, b, n); }
static double getd(void *i, const char *k) { char b[64]; gets_(i, k, b, sizeof b); return atof(b); }
static int is_meter(const row_t *r) { return !strcmp(r->kind, "METER"); }

/* run n frames of a stereo tone (int16 peak `amp`, `hz`), return the peak out; `pos` carries the phase across calls */
static int16_t run_tone(void *i, double amp, double hz, int frames, long *pos, int16_t *last_out)
{
    int16_t in[256], out[256], peak = 0;
    while (frames > 0) {
        int n = frames > 128 ? 128 : frames;
        for (int k = 0; k < n; k++, (*pos)++) {
            int16_t v = (int16_t)lrint(amp * sin(2.0 * M_PI * hz * (double)*pos / 44100.0));
            in[2 * k] = v; in[2 * k + 1] = v;
        }
        E->process(i, in, out, n);
        for (int k = 0; k < 2 * n; k++) { int16_t a = out[k] < 0 ? (int16_t)(out[k] == -32768 ? 32767 : -out[k]) : out[k]; if (a > peak) peak = a; }
        if (last_out) memcpy(last_out, out, sizeof out);
        frames -= n;
    }
    return peak;
}

static void snapshot(void *i, double *v)
{
    for (int k = 0; k < NROWS; k++) v[k] = is_meter(&ROWS[k]) ? 0.0 : getd(i, ROWS[k].key);
}

static void test_defaults_and_roundtrip(void)
{
    puts("defaults + parameter round trips");
    void *a = E->create(NULL);
    CHECK(a != NULL, "create(NULL) must work: the plugin keeps no files");
    for (int k = 0; k < NROWS; k++) {
        if (is_meter(&ROWS[k])) continue;
        CHECK(fabs(getd(a, ROWS[k].key) - ROWS[k].def) < 1e-9, "default of %s: got %g, want %g", ROWS[k].key, getd(a, ROWS[k].key), ROWS[k].def);
    }
    /* user-visible defaults the brief asks for */
    char b[40];
    gets_(a, "gt_thr_display", b, sizeof b); CHECK(!strcmp(b, "Off"), "gate threshold default is the -80 dB floor = off, shown as '%s'", b);
    CHECK(getd(a, "gt_thr") == 0.0, "gate threshold default knob = 0 (-80 dB)");
    gets_(a, "mx_thr_display", b, sizeof b); CHECK(!strcmp(b, "0.0 dB"), "maximizer threshold default '%s'", b);
    gets_(a, "mx_out_display", b, sizeof b); CHECK(!strcmp(b, "0.0 dB"), "maximizer output default (ceiling) '%s'", b);
    CHECK(getd(a, "mx_thr") == 1.0 && getd(a, "mx_out") == 1.0, "maximizer threshold and output default to the top = 0 dB");
    for (int k = 0; k < NROWS; k++) {      /* no bypass switches on the gate or the maximizer, no removed parameters left behind */
        CHECK(strcmp(ROWS[k].key, "gt_byp") && strcmp(ROWS[k].key, "out_byp") && strcmp(ROWS[k].key, "out_gain") && strncmp(ROWS[k].key, "pst_", 4),
              "parameter %s must not exist any more", ROWS[k].key);
        if (!strncmp(ROWS[k].key, "mx_", 3)) CHECK(strstr(ROWS[k].key, "byp") == NULL, "no maximizer bypass");
    }
    /* every sound parameter: set a value, read it back; option labels (as presets.json writes them) work too */
    for (int k = 0; k < NROWS; k++) {
        const row_t *r = &ROWS[k];
        if (is_meter(r)) continue;
        if (!strcmp(r->kind, "KNOB")) { setd(a, r->key, 0.37); CHECK(fabs(getd(a, r->key) - 0.37) < 1e-6, "%s knob round trip", r->key); setd(a, r->key, 7.0); CHECK(getd(a, r->key) == 1.0, "%s clamps high", r->key); setd(a, r->key, -3.0); CHECK(getd(a, r->key) == 0.0, "%s clamps low", r->key); }
        else { setp(a, r->key, "1"); CHECK(getd(a, r->key) == 1.0, "%s index 1", r->key); setp(a, r->key, "0"); CHECK(getd(a, r->key) == 0.0, "%s index 0", r->key); setp(a, r->key, "99"); CHECK(getd(a, r->key) <= 5.0, "%s clamps index", r->key); }
    }
    setp(a, "eq_mode", "EQ"); CHECK(getd(a, "eq_mode") == 0, "label EQ");
    setp(a, "eq_mode", "ISOLATOR"); CHECK(getd(a, "eq_mode") == 1, "label ISOLATOR");
    setp(a, "lp_curve", "BRICK"); CHECK(getd(a, "lp_curve") == 5, "label BRICK");
    { const char *lab[] = { "12", "24", "36", "48", "96", "BRICK" };   /* presets.json writes these labels: "12" is option 0, not index 12 */
      for (int i = 0; i < 6; i++) { setp(a, "hp_curve", lab[i]); CHECK(getd(a, "hp_curve") == i, "curve label %s -> option %d (got %g)", lab[i], i, getd(a, "hp_curve")); } }
    { char idx[2] = { 0, 0 }; for (int i = 0; i < 6; i++) { idx[0] = (char)('0' + i); setp(a, "lp_curve", idx); CHECK(getd(a, "lp_curve") == i, "curve index %d", i); } }
    setp(a, "lp_curve", "12");
    setp(a, "in_on", "Off"); CHECK(getd(a, "in_on") == 0, "label Off");
    setp(a, "in_on", "On"); CHECK(getd(a, "in_on") == 1, "label On");
    setp(a, "nonsense_key", "1"); setp(a, "m_in_l", "1"); CHECK(getd(a, "m_in_l") == 0.0, "meters are read-only");
    E->destroy(a);
    void *c = E->create("/nonexistent/folder"); CHECK(c != NULL, "a data_dir that does not exist is ignored"); E->destroy(c);
}

/* MPC's PRESET menu: the Init program sets every listed parameter from presets.json, as label strings. The result must be the defaults. */
static void test_init_program(void)
{
    puts("Init program (presets.json labels)");
    void *a = E->create(NULL);
    for (int k = 0; k < NROWS; k++) {        /* scramble first, then apply Init the way the wrapper's set_program() does */
        if (is_meter(&ROWS[k])) continue;
        if (!strcmp(ROWS[k].kind, "KNOB")) setd(a, ROWS[k].key, 0.123); else setp(a, ROWS[k].key, "1");
    }
    FILE *f = fopen("presets.json", "r");
    if (!f) f = fopen("../presets.json", "r");
    if (!f) { puts("  (presets.json not found from the working directory: skipped)"); E->destroy(a); return; }
    char txt[8192]; size_t n = fread(txt, 1, sizeof txt - 1, f); txt[n] = 0; fclose(f);
    const char *v = strstr(txt, "\"values\"");
    int applied = 0;
    for (int k = 0; k < NROWS && v; k++) {
        if (is_meter(&ROWS[k])) continue;
        char pat[64]; snprintf(pat, sizeof pat, "\"%s\":", ROWS[k].key);
        const char *p = strstr(v, pat); if (!p) continue;
        p += strlen(pat); while (*p == ' ') p++;
        char val[40]; int m = 0;
        if (*p == '"') { p++; while (*p && *p != '"' && m < 39) val[m++] = *p++; } else while (*p && *p != ',' && *p != '\n' && *p != '}' && m < 39) val[m++] = *p++;
        val[m] = 0; setp(a, ROWS[k].key, val); applied++;
    }
    CHECK(applied > 25, "applied %d Init values", applied);
    for (int k = 0; k < NROWS; k++) {
        if (is_meter(&ROWS[k])) continue;
        CHECK(fabs(getd(a, ROWS[k].key) - ROWS[k].def) < 1e-6, "Init program sets %s to %g, want %g", ROWS[k].key, getd(a, ROWS[k].key), ROWS[k].def);
    }
    E->destroy(a);
}

static void test_state(void)
{
    puts("MPC state string (project + preset recall)");
    void *a = E->create(NULL), *b = E->create(NULL);
    srand(99);
    for (int k = 0; k < NROWS; k++) {
        if (is_meter(&ROWS[k])) continue;
        if (!strcmp(ROWS[k].kind, "KNOB") || !strcmp(ROWS[k].kind, "SW")) setd(a, ROWS[k].key, (rand() % 1000001) / 1000000.0);
        else setp(a, ROWS[k].key, (rand() & 1) ? "1" : "0");
    }
    setp(a, "lp_curve", "96"); setp(a, "hp_curve", "BRICK");
    char st[8192]; gets_(a, "state", st, sizeof st);
    CHECK(strncmp(st, "mpcdj:2;", 8) == 0 && strlen(st) > 100 && strlen(st) < 2000, "state text looks right (%zu bytes)", strlen(st));
    CHECK(strchr(st, '.') == NULL && strchr(st, ',') == NULL, "no decimal separator anywhere: identical under any locale");
    CHECK(strstr(st, "m_in_l") == NULL && strstr(st, "lp_view") == NULL && strstr(st, "mx_gr") == NULL, "meters and display codes are not part of a preset");

    setp(b, "state", st);
    double va[64], vb[64]; snapshot(a, va); snapshot(b, vb);
    int same = 1; for (int k = 0; k < NROWS; k++) if (fabs(va[k] - vb[k]) > 1.5e-6) { same = 0; printf("  differs: %s %g vs %g\n", ROWS[k].key, va[k], vb[k]); }
    CHECK(same, "restoring the state on another instance reproduces every parameter");
    char st2[8192]; gets_(b, "state", st2, sizeof st2);
    CHECK(!strcmp(st, st2), "state -> restore -> state is stable");

    /* the same audio from both */
    long pa = 0, pb = 0; int16_t oa[256], ob[256];
    for (int blk = 0; blk < 20; blk++) { run_tone(a, 9000, 220, 128, &pa, oa); run_tone(b, 9000, 220, 128, &pb, ob); }
    CHECK(memcmp(oa, ob, sizeof oa) == 0, "restored instance sounds identical (bit for bit)");

    /* backward compatibility: missing keys -> default, unknown keys ignored, a newer tag is not applied */
    setp(b, "state", "mpcdj:1;in_gain=900000;future_param=5;lp_curve=3;");
    CHECK(fabs(getd(b, "in_gain") - 0.9) < 1e-6 && getd(b, "lp_curve") == 3, "listed keys applied");
    CHECK(getd(b, "width") == 1.0 && getd(b, "mx_thr") == 1.0 && getd(b, "gt_thr") == 0.0, "keys missing from an old state take their defaults");
    double before[64]; snapshot(b, before);
    setp(b, "state", "mpcdj:3;in_gain=0;"); setp(b, "state", ""); setp(b, "state", "garbage"); setp(b, "state", "mpcdj");
    double after[64]; snapshot(b, after);
    CHECK(memcmp(before, after, sizeof before) == 0, "a state with another tag, or none, changes nothing");
    setp(b, "state", "mpcdj:1;in_gain=99999999999999999999;width=-5;eq_mode=7;in_on=abc;;==;=3;noequals;lp_cut=");
    CHECK(getd(b, "in_gain") == 1.0 && getd(b, "width") == 0.0 && getd(b, "eq_mode") == 1.0 && getd(b, "in_on") == 0.0, "hostile values are clamped, never crash");
    /* fuzz: random and truncated states must never crash (ASan/UBSan) and must leave every value in range */
    for (int it = 0; it < 3000; it++) {
        char f[400]; int n = rand() % 380;
        if (it % 2) { memcpy(f, st, 380); f[379] = 0; f[rand() % 379 + 0] = (char)(rand() % 256); n = rand() % 370 + 1; f[n] = 0; }
        else { memcpy(f, "mpcdj:1;", 8); for (int k = 8; k < n + 8 && k < 399; k++) f[k] = "abc=;0123456789_-. xyz"[rand() % 22]; f[n + 8 < 399 ? n + 8 : 399] = 0; }
        setp(b, "state", f);
    }
    double fz[64]; snapshot(b, fz);
    int ok = 1; for (int k = 0; k < NROWS; k++) if (!is_meter(&ROWS[k]) && !(fz[k] >= 0.0 && fz[k] <= (strcmp(ROWS[k].kind, "KNOB") ? 5.0 : 1.0))) ok = 0;
    CHECK(ok, "after fuzzing every value is still in range");
    /* a buffer too small for the state is reported as failure (0), never as a cut-off state */
    char tiny[40]; CHECK(E->get_param(a, "state", tiny, sizeof tiny) == 0, "state that does not fit reports 0");
    /* Init state == defaults */
    char dst[8192]; void *d = E->create(NULL); gets_(d, "state", dst, sizeof dst);
    setp(b, "state", dst); double vd[64], vb2[64]; snapshot(d, vd); snapshot(b, vb2);
    CHECK(memcmp(vd, vb2, sizeof vd) == 0, "the default state restores the defaults (Init)");
    E->destroy(a); E->destroy(b); E->destroy(d);
}

static void test_gate_and_maximizer(void)
{
    puts("gate off at -80 dB; maximizer through the plugin");
    void *a = E->create(NULL); long pos = 0;
    /* default chain, quiet signal (-84 dBFS: 2 LSB) must survive: the gate is off, not closing */
    setp(a, "eq_on", "Off");
    int16_t last[256]; run_tone(a, 2.0, 100, 128 * 100, &pos, last);
    int nz = 0; for (int k = 0; k < 256; k++) if (last[k]) nz = 1;
    CHECK(nz, "default gate (-80 dB = off) lets a -84 dBFS tone through");
    /* raise the gate threshold a little: the same tone is now gated away */
    setd(a, "gt_thr", 0.5); run_tone(a, 2.0, 100, 128 * 400, &pos, last);
    nz = 0; for (int k = 0; k < 256; k++) if (last[k]) nz = 1;
    CHECK(!nz, "gate threshold -40 dB closes on a -84 dBFS tone");
    char b[40]; gets_(a, "gt_thr_display", b, sizeof b); CHECK(!strcmp(b, "-40.0 dB"), "gate threshold text '%s'", b);
    setd(a, "gt_thr", 0.0); run_tone(a, 2.0, 100, 128 * 100, &pos, last);
    nz = 0; for (int k = 0; k < 256; k++) if (last[k]) nz = 1;
    CHECK(nz, "back at the minimum the gate is off again (click-free fade, then it passes)");
    E->destroy(a);

    /* maximizer: huge input gain, then full-scale square-ish loudness: never above the ceiling, meters react */
    a = E->create(NULL); pos = 0;
    setd(a, "in_gain", 1.0);                 /* +24 dB */
    int16_t pk = run_tone(a, 12000, 150, 128 * 200, &pos, last);
    CHECK(getd(a, "mx_gr") > 0.3, "gain reduction meter moves (%g)", getd(a, "mx_gr"));
    CHECK(getd(a, "mx_in_l") > 0.95 && getd(a, "mx_in_r") > 0.95, "maximizer input meter saturates at the top when driven");
    { int fi = (int)lround(getd(a, "gr_db_i") * (DJ_READOUT_INT_FRAMES - 1)); CHECK(fi >= 1 && fi <= 61, "reduction reads as a negative number (frame %d)", fi); }
    CHECK(getd(a, "m_out_l") > 0.95, "output meter near the top: the ceiling is 0 dBFS (%g)", getd(a, "m_out_l"));
    /* lower output trim lowers the ceiling */
    setd(a, "mx_out", dj_taper_n6());
    run_tone(a, 12000, 150, 128 * 200, &pos, last);
    pk = 0; for (int k = 0; k < 256; k++) { int16_t v = last[k] < 0 ? (int16_t)-last[k] : last[k]; if (v > pk) pk = v; }   /* the settled block: the first 10 ms glide down */
    CHECK(pk > 15500 && pk < 16600, "mx_out about -6 dB lowers the ceiling to about 16384 (peak %d)", pk);
    /* threshold raises a quiet signal into the ceiling */
    E->destroy(a); a = E->create(NULL); pos = 0; setp(a, "eq_on", "Off");
    setd(a, "mx_thr", 0.6);                  /* -12 dB */
    pk = run_tone(a, 8192, 150, 128 * 300, &pos, last);   /* 0.25 = -12 dBFS tone */
    CHECK(pk > 31000, "threshold -12 dB with a -12 dBFS tone reaches the ceiling (peak %d)", pk);
    /* the maximizer's input meter reads the signal going IN (after the gate, before the threshold gain): -12 dB here, not the
     * boosted 0 dB (0.2.0 device report: it showed the level after the maximizer) */
    CHECK(fabs(getd(a, "mx_in_l") - (1.0 - 12.0 / 60.0)) < 0.03, "maximizer input meter ~ -12 dB (%g)", getd(a, "mx_in_l"));
    CHECK(getd(a, "m_out_l") > 0.95, "while the output sits at the ceiling (%g)", getd(a, "m_out_l"));
    { int fi = (int)lround(getd(a, "mxi_db_i") * (DJ_READOUT_INT_FRAMES - 1)), fd = (int)lround(getd(a, "mxi_db_t") * (DJ_READOUT_DEC_FRAMES - 1));
      int wi, wd; dj_readout_frames(20.0f * log10f(8192.0f / 32768.0f), &wi, &wd);
      CHECK(fi == wi && fd == wd, "maximizer input number reads -12.0 dB (frames %d,%d want %d,%d)", fi, fd, wi, wd); }
    /* default: a normal-level tone passes the maximizer untouched (compare against the maximizer fully relaxed) */
    E->destroy(a);
    void *x = E->create(NULL), *y = E->create(NULL); long px = 0, py = 0; int16_t ox[256], oy[256];
    setp(x, "eq_on", "Off"); setp(y, "eq_on", "Off"); setd(y, "mx_att", 1.0); setd(y, "mx_rel", 0.0);
    for (int k = 0; k < 50; k++) { run_tone(x, 20000, 440, 128, &px, ox); run_tone(y, 20000, 440, 128, &py, oy); }
    CHECK(memcmp(ox, oy, sizeof ox) == 0, "at its defaults the maximizer ignores attack/release for a signal under the ceiling");
    E->destroy(x); E->destroy(y);
}

static void test_meters_and_views(void)
{
    puts("meters + filter-curve codes + display_rev");
    void *a = E->create(NULL); long pos = 0;
    char b[40]; gets_(a, "display_rev", b, sizeof b); unsigned r0 = (unsigned)atoi(b);
    run_tone(a, 3277, 300, 128 * 20, &pos, NULL);          /* about -20 dBFS peak */
    CHECK(fabs(getd(a, "m_in_l") - (1.0 - 20.0 / 60.0)) < 0.03, "input meter ~ -20 dB (%g)", getd(a, "m_in_l"));
    gets_(a, "display_rev", b, sizeof b); CHECK((unsigned)atoi(b) != r0, "display_rev moves when a meter frame changes");
    int fr = (int)lround(getd(a, "m_in_l") * (DJ_METER_FRAMES - 1)); CHECK(fr >= 12 && fr <= 15, "meter frame %d of %d", fr, DJ_METER_FRAMES);
    /* silence: meters fall to nothing */
    run_tone(a, 0, 300, 128 * 1200, &pos, NULL);
    CHECK(getd(a, "m_in_l") == 0.0 && getd(a, "m_out_l") == 0.0 && getd(a, "mx_in_l") == 0.0 && getd(a, "mx_gr") == 0.0, "all meters fall back to zero (%g %g %g %g)", getd(a, "m_in_l"), getd(a, "m_out_l"), getd(a, "mx_in_l"), getd(a, "mx_gr"));
    /* filter-curve code: flat (last) frame when the stage does nothing, else step*RES + resbucket */
    CHECK(fabs(getd(a, "lp_view") - 1.0) < 1e-9 && fabs(getd(a, "hp_view") - 1.0) < 1e-9, "open LP / closed HP show the flat last frame");
    gets_(a, "display_rev", b, sizeof b); unsigned r1 = (unsigned)atoi(b);
    setd(a, "lp_cut", 0.5); setd(a, "lp_res", 1.0);
    gets_(a, "display_rev", b, sizeof b); CHECK((unsigned)atoi(b) != r1, "moving a filter knob bumps display_rev even with no audio");
    int step = (int)floor(0.5 * (DJ_VIEW_STEPS - 1) + 0.5);
    int want = step * DJ_VIEW_RES + (DJ_VIEW_RES - 1);
    int got = (int)lround(getd(a, "lp_view") * (DJ_VIEW_FRAMES - 1));
    CHECK(got == want, "lp frame %d, want %d", got, want);
    setp(a, "lp_on", "Off"); CHECK(fabs(getd(a, "lp_view") - 1.0) < 1e-9, "bypassed filter shows the flat frame");
    /* a switch or option changed by a Q-Link: display_rev moves with no audio running, so the wrapper reports the settled value
     * back and the on-screen switch / curve number follows (0.2.0 device report); setting the same value again does not */
    /* 1.0.5: the on/off switches turn like knobs. The value is kept exactly as sent (no snapping, so the Q-Link, the slider and
     * the sound agree); a Q-Link's 0-63 is off and 64-127 on. Nothing is reported back: nothing differs from what the host sent. */
    {
        const char *sws[] = { "in_on", "ph_inv", "ph_on", "wd_on", "bm_on", "bal_on", "eq_on", "lp_on", "hp_on" };
        for (unsigned k = 0; k < sizeof sws / sizeof sws[0]; k++) {
            char dk[40]; snprintf(dk, sizeof dk, "%s_display", sws[k]);
            int ok = 1;
            for (int q = 0; q <= 127; q++) {                        /* a full Q-Link sweep, 0..127 */
                setd(a, sws[k], q / 127.0);
                gets_(a, dk, b, sizeof b);
                if (fabs(getd(a, sws[k]) - q / 127.0) > 1e-6 || strcmp(b, q >= 64 ? "On" : "Off")) ok = 0;
            }
            CHECK(ok, "%s: every Q-Link position kept as sent; 0-63 Off, 64-127 On", sws[k]);
            setp(a, sws[k], "Off"); CHECK(getd(a, sws[k]) == 0.0, "%s takes Off", sws[k]);
            setp(a, sws[k], "On"); CHECK(getd(a, sws[k]) == 1.0, "%s takes On", sws[k]);
        }
        gets_(a, "display_rev", b, sizeof b); unsigned ra = (unsigned)atoi(b);
        setd(a, "eq_on", 0.3); setd(a, "eq_on", 0.31);
        gets_(a, "display_rev", b, sizeof b); CHECK((unsigned)atoi(b) == ra, "a switch move reports nothing back");
        /* the stage really follows the 0.5 line: eq at a -12 dB cut, a tone, on at 64/127, off at 63/127 */
        void *e = E->create(NULL); long pe = 0;
        setp(e, "eq_mode", "EQ"); setd(e, "eq_low", 0.0);
        setd(e, "eq_on", 64 / 127.0); int16_t pon = run_tone(e, 8000, 60, 128 * 300, &pe, NULL);
        setd(e, "eq_on", 63 / 127.0); int16_t poff = run_tone(e, 8000, 60, 128 * 300, &pe, NULL);
        CHECK(pon < poff * 0.7 && poff > 7000, "eq at 64/127 cuts the low tone (%d), at 63/127 it is bypassed (%d)", pon, poff);
        E->destroy(e);
        /* a 1.0.4 project (state tag mpcdj:1, switches stored 0/1) still loads */
        void *o = E->create(NULL);
        setp(o, "state", "mpcdj:1;in_on=0;eq_on=1;bm_on=1;lp_curve=2;in_gain=500000;");
        CHECK(getd(o, "in_on") == 0.0 && getd(o, "eq_on") == 1.0 && getd(o, "bm_on") == 1.0 && getd(o, "lp_curve") == 2 && fabs(getd(o, "in_gain") - 0.5) < 1e-9,
              "a 1.0.4 state loads: switches 0/1 read as off/on");
        char st2[8192]; gets_(o, "state", st2, sizeof st2);
        CHECK(!strncmp(st2, "mpcdj:2;", 8) && strstr(st2, "eq_on=1000000;") && strstr(st2, "in_on=0;"), "and is written back as mpcdj:2");
        E->destroy(o);
    }
    {   /* 1.0.5: the reduction meter lights 2 dB BEFORE limiting (a warning), while the number under it reads the exact reduction */
        void *m = E->create(NULL); long pm = 0;
        run_tone(m, 32768.0 * pow(10.0, -3.0 / 20.0), 440, 128 * 100, &pm, NULL);
        CHECK(lround(getd(m, "mx_gr") * (DJ_METER_FRAMES - 1)) == 0, "3 dB under the ceiling: dark");
        run_tone(m, 32768.0 * pow(10.0, -1.0 / 20.0), 440, 128 * 100, &pm, NULL);
        CHECK(lround(getd(m, "mx_gr") * (DJ_METER_FRAMES - 1)) == 1, "1 dB under the ceiling: the first segment (%g)", getd(m, "mx_gr"));
        CHECK(lround(getd(m, "gr_db_i") * (DJ_READOUT_INT_FRAMES - 1)) == 62 && lround(getd(m, "gr_db_t") * (DJ_READOUT_DEC_FRAMES - 1)) == 0,
              "while the number still reads 0.0 dB (no reduction yet)");
        setd(m, "mx_thr", 1.0 - 6.0 / -DJ_MX_THR_MIN_DB);     /* threshold -6 dB: the same tone is driven 5 dB over */
        { char t[32]; gets_(m, "mx_thr_display", t, sizeof t); CHECK(!strcmp(t, "-6.0 dB"), "threshold '%s'", t); }
        run_tone(m, 32768.0 * pow(10.0, -1.0 / 20.0), 440, 128 * 200, &pm, NULL);
        long fr = lround(getd(m, "mx_gr") * (DJ_METER_FRAMES - 1));
        CHECK(fr >= 6 && fr <= 8, "driven 5 dB into the limiter: segment %ld of 20 (2 + 5)", fr);
        E->destroy(m);
    }
    const char *sw[] = { "lp_curve", "hp_curve", "eq_mode" };
    for (unsigned k = 0; k < sizeof sw / sizeof sw[0]; k++) {
        gets_(a, "display_rev", b, sizeof b); unsigned ra = (unsigned)atoi(b);
        setp(a, sw[k], getd(a, sw[k]) >= 1.0 ? "0" : "1");
        gets_(a, "display_rev", b, sizeof b); unsigned rb = (unsigned)atoi(b);
        CHECK(rb != ra, "changing %s moves display_rev", sw[k]);
        char cur[8]; snprintf(cur, sizeof cur, "%d", (int)getd(a, sw[k])); setp(a, sw[k], cur);
        gets_(a, "display_rev", b, sizeof b); CHECK((unsigned)atoi(b) == rb, "re-setting %s to the same value (the host's echo) does not", sw[k]);
    }
    E->destroy(a);

    /* the dB numbers under the meters: the held peak, whole dB and tenth as two filmstrip frames */
    a = E->create(NULL); pos = 0;
    { int fi = (int)lround(getd(a, "in_db_i") * (DJ_READOUT_INT_FRAMES - 1)), fd = (int)lround(getd(a, "in_db_t") * (DJ_READOUT_DEC_FRAMES - 1));
      CHECK(fi == 0 && fd == DJ_READOUT_DEC_FRAMES - 1, "silence reads -inf (frames %d,%d)", fi, fd);
      fi = (int)lround(getd(a, "gr_db_i") * (DJ_READOUT_INT_FRAMES - 1)); fd = (int)lround(getd(a, "gr_db_t") * (DJ_READOUT_DEC_FRAMES - 1));
      CHECK(fi == 62 && fd == 0, "no reduction reads 0.0 dB (frames %d,%d)", fi, fd); }
    run_tone(a, 3277, 300, 128 * 40, &pos, NULL);          /* peak 3277/32768 = -20.0 dBFS */
    { int wi, wd; dj_readout_frames(20.0f * log10f(3277.0f / 32768.0f), &wi, &wd);
      int fi = (int)lround(getd(a, "in_db_i") * (DJ_READOUT_INT_FRAMES - 1)), fd = (int)lround(getd(a, "in_db_t") * (DJ_READOUT_DEC_FRAMES - 1));
      CHECK(fi == wi && fd == wd && wi == 41, "a -20 dBFS tone reads -20.0 dB (frames %d,%d want %d,%d)", fi, fd, wi, wd);
      fi = (int)lround(getd(a, "out_db_i") * (DJ_READOUT_INT_FRAMES - 1));
      CHECK(abs(fi - wi) <= 1, "and about the same at the output with everything at Init (frame %d)", fi); }
    run_tone(a, 0, 300, 128 * 200, &pos, NULL);            /* 0.58 s of silence: still held */
    CHECK((int)lround(getd(a, "in_db_i") * (DJ_READOUT_INT_FRAMES - 1)) == 41, "the number holds through a short silence");
    run_tone(a, 0, 300, 128 * 800, &pos, NULL);            /* 2.9 s more: two holds gone */
    CHECK(getd(a, "in_db_i") == 0.0, "then falls to -inf");
    E->destroy(a);

    /* 1.0.4: the slope on a Q-Link. A Q-Link turns lp_crv (a plain knob the MPC owns); the engine picks lp_curve = round(x 5) and
     * reports it, so the 12..BRICK buttons follow. A tap on a button moves lp_crv to that slope. */
    a = E->create(NULL);
    CHECK(getd(a, "lp_crv") == 0.0 && getd(a, "lp_curve") == 0, "slope control starts at 12 with the slope");
    {   /* a slow Q-Link turn: 1/128 per event, as MPC sends; every event lands where it was sent (no snapping) */
        int changes = 0, last = 0, mono = 1; unsigned rv;
        gets_(a, "display_rev", b, sizeof b); rv = (unsigned)atoi(b);
        for (int e = 1; e <= 128; e++) {
            setd(a, "lp_crv", e / 128.0);
            CHECK(fabs(getd(a, "lp_crv") - e / 128.0) < 1e-6, "the Q-Link value is kept as sent (event %d)", e);
            int c = (int)getd(a, "lp_curve");
            if (c != last) {
                changes++; if (c != last + 1) mono = 0; last = c;
                gets_(a, "display_rev", b, sizeof b); CHECK((unsigned)atoi(b) != rv, "a slope change moves display_rev (to %d)", c); rv = (unsigned)atoi(b);
            }
            int want = (int)floor(e / 128.0 * 5 + 0.5); CHECK(c == want, "event %d: slope %d, want %d", e, c, want);
        }
        CHECK(changes == 5 && mono && last == 5, "a full turn walks 12, 24, 36, 48, 96, BRICK in order (%d changes)", changes);
        gets_(a, "lp_crv_display", b, sizeof b); CHECK(!strcmp(b, "BRICK"), "the Q-Link shows the slope as text: '%s'", b);
        /* the host's echo of the report (lp_curve set to what it already is) changes nothing and reports nothing */
        gets_(a, "display_rev", b, sizeof b); rv = (unsigned)atoi(b);
        setp(a, "lp_curve", "5"); setd(a, "lp_crv", 1.0);
        gets_(a, "display_rev", b, sizeof b); CHECK((unsigned)atoi(b) == rv, "echoes do not start another round");
    }
    setp(a, "lp_curve", "24");                               /* a tap on the 24 button */
    CHECK(fabs(getd(a, "lp_crv") - 0.2) < 1e-9 && getd(a, "lp_curve") == 1, "a tap moves the Q-Link control to 24 (%g)", getd(a, "lp_crv"));
    setd(a, "lp_crv", 0.2 + 3.0 / 128.0);                    /* a Q-Link nudge inside the 24 range: no change */
    CHECK(getd(a, "lp_curve") == 1, "a small turn inside a slope's range keeps it");
    setp(a, "hp_curve", "BRICK"); CHECK(getd(a, "hp_crv") == 1.0, "HP the same (%g)", getd(a, "hp_crv"));
    {   /* not in the state: derived from the slope on load */
        char st[8192]; gets_(a, "state", st, sizeof st);
        CHECK(strstr(st, "lp_crv") == NULL, "the slope controls are not stored");
        void *c2 = E->create(NULL); setp(c2, "state", st);
        CHECK(getd(c2, "lp_curve") == 1 && fabs(getd(c2, "lp_crv") - 0.2) < 1e-9 && getd(c2, "hp_crv") == 1.0, "loaded: control follows slope");
        setp(c2, "state", "mpcdj:1;lp_curve=4;");
        CHECK(getd(c2, "lp_curve") == 4 && fabs(getd(c2, "lp_crv") - 0.8) < 1e-9, "an older state's slope is kept, not reset by the control");
        E->destroy(c2);
    }
    CHECK(DJ_VIEW_FRAMES == DJ_VIEW_STEPS * DJ_VIEW_RES + 1 && DJ_VIEW_FRAMES * DJ_VIEW_H < 12288, "filmstrip fits MPC's 12288 px limit (%d px)", DJ_VIEW_FRAMES * DJ_VIEW_H);
    CHECK(DJ_VIEW_FRAMES > 61, "more frames than 0.1.0 (61): %d", DJ_VIEW_FRAMES);
    E->destroy(a);
}

static void test_meter_taps(void)
{
    puts("meter tap points: INPUT after gain / before phase; FILTER after balance / before EQ");
    long pos = 0;
    void *a = E->create(NULL);
    run_tone(a, 3277, 300, 128 * 40, &pos, NULL);          /* about -20 dBFS, Init */
    double in0 = getd(a, "m_in_l"), fin0 = getd(a, "m_fin_l");
    CHECK(fabs(in0 - (1.0 - 20.0 / 60.0)) < 0.03 && fabs(fin0 - in0) < 0.03, "at Init both input meters read ~ -20 dB (%g %g)", in0, fin0);
    CHECK(getd(a, "fin_db_i") > 0.0, "the FILTER input dB number reads");
    E->destroy(a);

    a = E->create(NULL); setd(a, "in_gain", 1.0);          /* gain up: the INPUT meter follows it */
    run_tone(a, 3277, 300, 128 * 40, &pos, NULL);
    CHECK(getd(a, "m_in_l") > in0 + 0.05, "INPUT meter is after the gain knob (%g -> %g)", in0, getd(a, "m_in_l"));
    E->destroy(a);

    a = E->create(NULL); setd(a, "in_gain", 0.0);          /* the bottom of the gain knob: the meter shows it */
    run_tone(a, 3277, 300, 128 * 40, &pos, NULL);
    CHECK(getd(a, "m_in_l") < in0 - 0.05, "and down with it (%g)", getd(a, "m_in_l"));
    E->destroy(a);

    a = E->create(NULL);                                   /* the EQ is after the FILTER tap: a cut shows on OUT only */
    setd(a, "eq_low", 0.0); setd(a, "eq_mid", 0.0); setd(a, "eq_high", 0.0);
    run_tone(a, 3277, 300, 128 * 200, &pos, NULL);
    CHECK(fabs(getd(a, "m_fin_l") - fin0) < 0.02 && getd(a, "m_out_l") < fin0 - 0.03,
          "an EQ cut shows on OUT (%g) but not on the FILTER input meter (%g)", getd(a, "m_out_l"), getd(a, "m_fin_l"));
    E->destroy(a);

    a = E->create(NULL); setd(a, "balance", 0.0);          /* hard left: the FILTER meter is after balance */
    run_tone(a, 3277, 300, 128 * 1200, &pos, NULL);        /* long enough for the first block's glide to fall off the meter */
    CHECK(getd(a, "m_fin_r") < 0.05 && fabs(getd(a, "m_fin_l") - fin0) < 0.02 && fabs(getd(a, "m_in_r") - in0) < 0.02,
          "FILTER input meter is after balance, INPUT's is not (fin L %g R %g, in R %g)", getd(a, "m_fin_l"), getd(a, "m_fin_r"), getd(a, "m_in_r"));
    E->destroy(a);

    a = E->create(NULL); setd(a, "balance", 0.0); setd(a, "bal_on", 0.0);   /* balance switched off: both sides again */
    run_tone(a, 3277, 300, 128 * 40, &pos, NULL);
    CHECK(fabs(getd(a, "m_fin_r") - fin0) < 0.02, "and after its on/off switch (R %g)", getd(a, "m_fin_r"));
    E->destroy(a);
}

static void test_blocks_and_determinism(void)
{
    puts("odd block sizes, two instances, determinism");
    void *a = E->create(NULL), *b = E->create(NULL);
    srand(5);
    for (int k = 0; k < NROWS; k++) {
        if (is_meter(&ROWS[k])) continue;
        const char *key = ROWS[k].key; double v = (rand() % 1001) / 1000.0;
        if (!strcmp(ROWS[k].kind, "KNOB")) { setd(a, key, v); setd(b, key, v); } else { setp(a, key, v > 0.5 ? "1" : "0"); setp(b, key, v > 0.5 ? "1" : "0"); }
    }
    int sizes[] = { 1, 7, 127, 128, 129, 300, 2, 64 };
    long pa = 0, pb = 0; int ok = 1;
    for (int it = 0; it < 80; it++) {
        int n = sizes[it % 8];
        int16_t ia[2 * 300], oa[2 * 300], ob[2 * 300];
        for (int k = 0; k < n; k++, pa++, pb++) { int16_t v = (int16_t)(rand() % 60001 - 30000); ia[2 * k] = v; ia[2 * k + 1] = (int16_t)-v / 2; }
        E->process(a, ia, oa, n); E->process(b, ia, ob, n);
        if (memcmp(oa, ob, (size_t)n * 4)) ok = 0;
    }
    CHECK(ok, "two instances with identical settings and input are bit-identical for any block length");
    E->destroy(a); E->destroy(b);
    /* with all stages bypassed and a quiet signal the chain is transparent (int16 in = int16 out) */
    void *c = E->create(NULL);
    setp(c, "in_on", "Off"); setp(c, "ph_on", "Off"); setp(c, "wd_on", "Off"); setp(c, "bm_on", "Off"); setp(c, "bal_on", "Off");
    setp(c, "eq_on", "Off"); setp(c, "lp_on", "Off"); setp(c, "hp_on", "Off");
    int16_t io[256], oo[256]; int same = 1;
    for (int it = 0; it < 40; it++) {
        for (int k = 0; k < 256; k++) io[k] = (int16_t)(rand() % 40001 - 20000);
        E->process(c, io, oo, 128);
        if (it > 5 && memcmp(io, oo, sizeof io)) same = 0;
    }
    CHECK(same, "everything bypassed: the plugin is bit-transparent for a signal under the ceiling");
    E->destroy(c);
}

int main(void)
{
    E = mpc_engine();
    test_defaults_and_roundtrip();
    test_init_program();
    test_state();
    test_gate_and_maximizer();
    test_meters_and_views();
    test_meter_taps();
    test_blocks_and_determinism();
    printf("\n%d checks, %d failed\n", g_run, g_fail);
    puts(g_fail ? "FAILED" : "PASSED");
    return g_fail ? 1 : 0;
}
