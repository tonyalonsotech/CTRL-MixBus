/* dj_engine.c -- the mpc_engine() glue: parameters in, DSP chain, meters out.
 *
 * Contract (wrapper/engine.h): 44.1 kHz, interleaved int16 stereo, 128-frame blocks, an effect implements process().
 * Signal flow, each stage with its own true bypass crossfade (src/dsp/dj_common.h dj_xfade_t):
 *
 *   in -> [meter] -> Input Gain -> Phase Invert -> Stereo Width -> Bass Mono -> Balance -> DJ EQ/Isolator
 *      -> Low-Pass -> High-Pass -> Gate -> [maximizer input meter] -> Maximizer -> [meter] -> out
 *
 * Each "_on" switch is 1 while its stage processes and 0 while it is bypassed. A switch or option change is reported back to
 * the host (display_rev), so the on-screen switch and the curve number follow a Q-Link turn: the wrapper snaps a Q-Link move to
 * the next option, and without the report MPC would keep showing the in-between value it sent.
 *
 * The gate has no bypass switch: at its minimum threshold (-80 dB) it is switched off through the same crossfade, so it costs
 * nothing and passes everything. The maximizer has none either: at its defaults it only acts on peaks above full scale.
 *
 * Presets: the MPC's own preset and project system does the saving. It reads/writes this plugin through the wrapper's
 * parameter calls and through a "state" string that this file serves (get_param("state") / set_param("state", ...)): every
 * sound parameter by KEY with an integer value, so a state saved by one version still loads in a later one (unknown keys are
 * ignored, missing keys take their default). The stock "Init" program comes from presets.json (generated from the defaults).
 *
 * Rules kept here: no allocation, locking, or I/O in process(); all work is on the stack; every stage processes
 * float internally (headroom for simultaneous boosts) and only the final conversion clips to int16. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "engine.h"
#include "../dsp/dj_simple.h"
#include "../dsp/dj_bassmono.h"
#include "../dsp/dj_eq.h"
#include "../dsp/dj_filter.h"
#include "../dsp/dj_gate.h"
#include "../dsp/dj_maximizer.h"
#include "../dsp/dj_meter.h"
#include "../dsp/dj_taper.h"
#include "../dsp/dj_view.h"
#include "../dsp/dj_readout.h"

/* ---- parameter table, generated from the single .def ---- */
enum {
#define DJP(ID, key, name, kind, def, opts) P_##ID,
#include "dj_params.def"
#undef DJP
    P_COUNT
};
typedef enum { K_TOG, K_OPT, K_KNOB, K_SW, K_METER } kind_t;
typedef struct { const char *key; kind_t kind; double def; const char *opts; } pdef_t;
static const pdef_t PDEF[P_COUNT] = {
#define DJP(ID, key, name, kind, def, opts) { key, K_##kind, def, opts },
#include "dj_params.def"
#undef DJP
};

/* stages, in signal order */
enum { S_IN, S_PHASE, S_WIDTH, S_BASS, S_BAL, S_EQ, S_LP, S_HP, S_GATE, S_MAX, S_COUNT };
#define BYPASS_FADE_MS 6.0f
#define GATE_OFF_KNOB 0.0005         /* gate threshold knob at or below this = -80 dB = gate off */
#define N_BAR_FRAMES 9               /* m_in_l/r, m_out_l/r, mx_in_l/r, mx_gr, m_fin_l/r */
#define N_METER_FRAMES (N_BAR_FRAMES + 2 * RD_COUNT)   /* then the dB readouts x (whole, tenth) */
enum { RD_IN, RD_OUT, RD_MXI, RD_GR, RD_FIN, RD_COUNT };

typedef struct {
    double v[P_COUNT];               /* current value of every parameter (knob 0..1, or option index) */
    dj_gain_t gin;
    dj_phase_t phase;
    dj_width_t width;
    dj_bassmono_t bass;
    dj_balance_t bal;
    dj_eq_t eq;
    dj_filter_t lp, hp;
    dj_gate_t gate;
    dj_maximizer_t mx;
    dj_xfade_t xf[S_COUNT];
    int was_off[S_COUNT];
    dj_meter_t min, mout, mxin, mfin;   /* min: after the gain stage; mfin: after balance, before the EQ */
    dj_grmeter_t gr;
    dj_readout_t rd[RD_COUNT];       /* the dB numbers under the meters (RD_GR holds the reduction, >= 0) */
    int last_frame[N_METER_FRAMES];
    unsigned rev;                    /* display_rev: bumped when a meter's filmstrip frame or a curve picture changed */
    int view[2];                     /* filter-curve strip frame of LP, HP (last reported, to know when display_rev must move) */
} inst_t;

static int find_key(const char *key)
{
    for (int i = 0; i < P_COUNT; i++) if (!strcmp(PDEF[i].key, key)) return i;
    return -1;
}

static int opt_count(const pdef_t *p)
{
    if (p->kind == K_TOG) return 2;
    int n = 1;
    for (const char *c = p->opts; *c; c++) if (*c == '|') n++;
    return n;
}

/* an option may arrive as its LABEL ("BRICK", "On", and "12", "96" for the curve slopes: presets.json writes labels) or as its
 * index ("3", which is what the wrapper sends for a knob turn). Labels are matched first, so the slope label "12" is the first
 * option and not an out-of-range index 12. */
static int opt_label_index(const pdef_t *p, const char *val)
{
    if (p->kind == K_TOG) {
        if (!strcasecmp(val, "off") || !strcasecmp(val, "false")) return 0;
        if (!strcasecmp(val, "on") || !strcasecmp(val, "true")) return 1;
        return -1;
    }
    const char *s = p->opts;
    for (int i = 0; ; i++) {
        const char *e = strchr(s, '|');
        size_t len = e ? (size_t)(e - s) : strlen(s);
        if (strlen(val) == len && !strncasecmp(val, s, len)) return i;
        if (!e) break;
        s = e + 1;
    }
    return -1;
}

static double parse_value(const pdef_t *p, const char *val)
{
    if (p->kind == K_TOG || p->kind == K_OPT) {
        int li = opt_label_index(p, val);
        if (li >= 0) return li;
        if (val[0] == '-' || (val[0] >= '0' && val[0] <= '9')) {
            int n = atoi(val), top = opt_count(p) - 1;
            return n < 0 ? 0 : n > top ? top : n;
        }
        return p->def;
    }
    if (p->kind == K_SW) {                                   /* a switch also takes its words (older presets wrote On/Off) */
        if (!strcasecmp(val, "on") || !strcasecmp(val, "true")) return 1.0;
        if (!strcasecmp(val, "off") || !strcasecmp(val, "false")) return 0.0;
    }
    double d = atof(val);
    return d < 0.0 ? 0.0 : d > 1.0 ? 1.0 : d;
}

static int on(const inst_t *s, int id) { return s->v[id] >= 0.5; }

static void mx_apply(inst_t *s)
{
    dj_max_set(&s->mx, dj_taper_mx_thresh_db((float)s->v[P_MX_THR]), dj_taper_mx_attack_ms((float)s->v[P_MX_ATT]),
               dj_taper_release_ms((float)s->v[P_MX_REL]), dj_taper_mx_out_db((float)s->v[P_MX_OUT]));
}

static void gate_apply(inst_t *s)
{
    dj_gate_set(&s->gate, dj_taper_thresh_db((float)s->v[P_GT_THR]), dj_taper_attack_ms((float)s->v[P_GT_ATT]),
                dj_taper_hold_ms((float)s->v[P_GT_HOLD]), dj_taper_release_ms((float)s->v[P_GT_REL]));
}

/* the slope control a Q-Link turns (lp_crv / hp_crv, 0..1) and its slope option: round(value x 5), 0 = 12 ... 5 = BRICK */
static int crv_curve(double k) { int c = (int)floor(k * (DJ_FLT_NUM_CURVES - 1) + 0.5); return c < 0 ? 0 : c > DJ_FLT_NUM_CURVES - 1 ? DJ_FLT_NUM_CURVES - 1 : c; }
static int crv_of(int curve_id) { return curve_id == P_LP_CURVE ? P_LP_CRV : P_HP_CRV; }
static int curve_of(int crv_id) { return crv_id == P_LP_CRV ? P_LP_CURVE : P_HP_CURVE; }
static int no_state(int id) { return PDEF[id].kind == K_METER || id == P_LP_CRV || id == P_HP_CRV; }   /* derived, never stored */

/* push parameter `id`'s value into the DSP modules (any thread that holds the engine lock) */
static void apply(inst_t *s, int id)
{
    const double *v = s->v;
    switch (id) {
    case P_IN_ON: dj_xfade_set(&s->xf[S_IN], on(s, P_IN_ON)); break;
    case P_IN_GAIN: dj_gain_set_db(&s->gin, dj_taper_gain_db((float)v[P_IN_GAIN], DJ_IN_N0, DJ_IN_MAX_DB)); break;
    case P_PH_ON: dj_xfade_set(&s->xf[S_PHASE], on(s, P_PH_ON)); break;
    case P_PH_INV: dj_phase_set(&s->phase, v[P_PH_INV] >= 0.5); break;
    case P_WD_ON: dj_xfade_set(&s->xf[S_WIDTH], on(s, P_WD_ON)); break;
    case P_WD_AMT: dj_width_set(&s->width, (float)v[P_WD_AMT]); break;
    case P_BM_ON: dj_xfade_set(&s->xf[S_BASS], on(s, P_BM_ON)); break;
    case P_BM_FREQ: dj_bassmono_set_freq(&s->bass, dj_taper_bass_hz((float)v[P_BM_FREQ])); break;
    case P_BAL_ON: dj_xfade_set(&s->xf[S_BAL], on(s, P_BAL_ON)); break;
    case P_BAL: dj_balance_set(&s->bal, dj_taper_balance((float)v[P_BAL])); break;
    case P_EQ_ON: dj_xfade_set(&s->xf[S_EQ], on(s, P_EQ_ON)); break;
    case P_EQ_MODE: dj_eq_set_mode(&s->eq, v[P_EQ_MODE] >= 0.5 ? DJ_EQ_MODE_ISOLATOR : DJ_EQ_MODE_EQ); break;
    case P_EQ_LOW: case P_EQ_MID: case P_EQ_HIGH:
        dj_eq_set(&s->eq, (float)v[P_EQ_LOW], (float)v[P_EQ_MID], (float)v[P_EQ_HIGH]); break;
    case P_LP_ON: case P_LP_CUT:
        /* a fully open low-pass (knob at the top) is bypassed too, so the stage costs nothing when it does nothing */
        dj_xfade_set(&s->xf[S_LP], on(s, P_LP_ON) && v[P_LP_CUT] < 0.999);
        dj_filter_set(&s->lp, (float)v[P_LP_CUT], (float)v[P_LP_RES], (int)v[P_LP_CURVE]); break;
    case P_LP_RES: case P_LP_CURVE: dj_filter_set(&s->lp, (float)v[P_LP_CUT], (float)v[P_LP_RES], (int)v[P_LP_CURVE]); break;
    case P_HP_ON: case P_HP_CUT:
        dj_xfade_set(&s->xf[S_HP], on(s, P_HP_ON) && v[P_HP_CUT] > 0.001);
        dj_filter_set(&s->hp, (float)v[P_HP_CUT], (float)v[P_HP_RES], (int)v[P_HP_CURVE]); break;
    case P_HP_RES: case P_HP_CURVE: dj_filter_set(&s->hp, (float)v[P_HP_CUT], (float)v[P_HP_RES], (int)v[P_HP_CURVE]); break;
    case P_GT_THR:
        dj_xfade_set(&s->xf[S_GATE], v[P_GT_THR] > GATE_OFF_KNOB);   /* the minimum threshold switches the gate off, click-free */
        gate_apply(s); break;
    case P_GT_ATT: case P_GT_HOLD: case P_GT_REL: gate_apply(s); break;
    case P_MX_THR: case P_MX_ATT: case P_MX_REL: case P_MX_OUT: mx_apply(s); break;
    default: break;   /* meters */
    }
}

static void reset_stage(inst_t *s, int st)
{
    switch (st) {
    case S_IN: dj_gain_reset(&s->gin); break;
    case S_PHASE: dj_phase_reset(&s->phase); break;
    case S_WIDTH: dj_width_reset(&s->width); break;
    case S_BASS: dj_bassmono_reset(&s->bass); break;
    case S_BAL: dj_balance_reset(&s->bal); break;
    case S_EQ: dj_eq_reset(&s->eq); break;
    case S_LP: dj_filter_reset(&s->lp); break;
    case S_HP: dj_filter_reset(&s->hp); break;
    case S_GATE: dj_gate_reset(&s->gate); break;
    case S_MAX: dj_max_reset(&s->mx); break;
    }
}

static void run_dsp(inst_t *s, int st, float *l, float *r, int n)
{
    switch (st) {
    case S_IN: dj_gain_process(&s->gin, l, r, n); break;
    case S_PHASE: dj_phase_process(&s->phase, l, r, n); break;
    case S_WIDTH: dj_width_process(&s->width, l, r, n); break;
    case S_BASS: dj_bassmono_process(&s->bass, l, r, n); break;
    case S_BAL: dj_balance_process(&s->bal, l, r, n); break;
    case S_EQ: dj_eq_process(&s->eq, l, r, n); break;
    case S_LP: dj_filter_process(&s->lp, l, r, n); break;
    case S_HP: dj_filter_process(&s->hp, l, r, n); break;
    case S_GATE: dj_gate_process(&s->gate, l, r, n); break;
    case S_MAX: dj_max_process(&s->mx, l, r, n); break;
    }
}

/* One stage with a TRUE bypass: fully off = not run at all (and its state cleared, so it re-enters clean); fully on =
 * run in place; in between = run on a copy and crossfade dry/wet per sample, so toggling never clicks. */
static void run_stage(inst_t *s, int st, float *l, float *r, int n)
{
    dj_xfade_t *x = &s->xf[st];
    if (dj_xfade_off(x)) {
        if (!s->was_off[st]) { reset_stage(s, st); s->was_off[st] = 1; }
        return;
    }
    s->was_off[st] = 0;
    if (dj_xfade_on(x)) { run_dsp(s, st, l, r, n); return; }
    float dl[DJ_BLOCK], dr[DJ_BLOCK];
    memcpy(dl, l, (size_t)n * sizeof(float));
    memcpy(dr, r, (size_t)n * sizeof(float));
    run_dsp(s, st, l, r, n);
    for (int i = 0; i < n; i++) {
        float m = dj_xfade_next(x);
        l[i] = dl[i] + m * (l[i] - dl[i]);
        r[i] = dr[i] + m * (r[i] - dr[i]);
    }
}

/* ---- filter-curve displays ---- */
/* The strip frame showing filter `hp` (0 = low-pass, 1 = high-pass) as it is NOW: the cutoff step and resonance bucket nearest
 * the knobs, or the flat last frame when the stage does nothing (bypassed, or the same "fully open" test apply() uses). */
static int view_frame(const inst_t *s, int hp)
{
    const double cut = s->v[hp ? P_HP_CUT : P_LP_CUT], res = s->v[hp ? P_HP_RES : P_LP_RES];
    if (!on(s, hp ? P_HP_ON : P_LP_ON) || (hp ? cut <= 0.001 : cut >= 0.999)) return DJ_VIEW_FRAMES - 1;
    int step = (int)floor(cut * (DJ_VIEW_STEPS - 1) + 0.5), rb = (int)floor(res * (DJ_VIEW_RES - 1) + 0.5);
    step = step < 0 ? 0 : step > DJ_VIEW_STEPS - 1 ? DJ_VIEW_STEPS - 1 : step;
    rb = rb < 0 ? 0 : rb > DJ_VIEW_RES - 1 ? DJ_VIEW_RES - 1 : rb;
    return step * DJ_VIEW_RES + rb;
}

/* after any parameter change: tell the wrapper (display_rev) when a curve picture must change, even with no audio running */
static void refresh_views(inst_t *s)
{
    for (int hp = 0; hp < 2; hp++) {
        int f = view_frame(s, hp);
        if (f != s->view[hp]) { s->view[hp] = f; s->rev++; }
    }
}

/* ---------------------------------------------------------------------------------------------------------- */
static void *create(const char *data_dir)
{
    (void)data_dir;                                     /* the plugin keeps no files: the MPC stores presets and projects */
    inst_t *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    for (int i = 0; i < P_COUNT; i++) s->v[i] = PDEF[i].def;
    dj_gain_init(&s->gin, DJ_FS);
    dj_phase_init(&s->phase, DJ_FS); dj_width_init(&s->width, DJ_FS); dj_bassmono_init(&s->bass, DJ_FS);
    dj_balance_init(&s->bal, DJ_FS); dj_eq_init(&s->eq, DJ_FS);
    dj_filter_init(&s->lp, DJ_FS, DJ_FLT_LP); dj_filter_init(&s->hp, DJ_FS, DJ_FLT_HP);
    dj_gate_init(&s->gate, DJ_FS); dj_max_init(&s->mx, DJ_FS);
    dj_meter_init(&s->min, DJ_FS); dj_meter_init(&s->mout, DJ_FS); dj_meter_init(&s->mxin, DJ_FS); dj_meter_init(&s->mfin, DJ_FS); dj_grmeter_init(&s->gr, DJ_FS);
    for (int i = 0; i < RD_COUNT; i++) dj_readout_init(&s->rd[i], DJ_FS);
    s->rd[RD_GR].held_db = 0.0f;                        /* no reduction reads "0.0 dB", not "-inf" */
    for (int i = 0; i < S_COUNT; i++) dj_xfade_init(&s->xf[i], DJ_FS, BYPASS_FADE_MS, 1.0f);
    for (int i = 0; i < P_COUNT; i++) apply(s, i);
    for (int i = 0; i < S_COUNT; i++) { s->xf[i].mix = s->xf[i].target; s->was_off[i] = dj_xfade_off(&s->xf[i]); }
    /* start settled at the stored values: no ramp from defaults at load */
    for (int i = 0; i < S_COUNT; i++) reset_stage(s, i);
    for (int i = 0; i < N_METER_FRAMES; i++) s->last_frame[i] = -1;
    s->view[0] = view_frame(s, 0); s->view[1] = view_frame(s, 1);
    return s;
}

static void destroy(void *inst) { free(inst); }
static void midi(void *inst, const uint8_t *msg, int len) { (void)inst; (void)msg; (void)len; }

/* ---- the "state" string (the wrapper stores it as the plugin's chunk: MPC projects and presets) ----
 *   mpcdj:2;in_gain=700000;in_on=1000000;eq_mode=1;...
 * Knobs and switches (SW) are integers in millionths (0..1000000), options their index: no decimal point, so the text is the
 * same under any locale. Meters are not stored. Unknown keys are skipped; a key that is missing keeps its DEFAULT (so a
 * state written before a parameter existed loads with that parameter at Init). */
#define STATE_TAG "mpcdj:2;"
#define STATE_TAG_V1 "mpcdj:1;"     /* up to 1.0.4: the switches were two-option TOGs, stored as 0/1; read as off/on */

static int state_write(const inst_t *s, char *buf, int cap)
{
    int n = snprintf(buf, (size_t)cap, "%s", STATE_TAG);
    for (int i = 0; i < P_COUNT && n >= 0 && n < cap; i++) {
        if (no_state(i)) continue;
        long val = (PDEF[i].kind == K_KNOB || PDEF[i].kind == K_SW) ? lround(s->v[i] * 1000000.0) : (long)s->v[i];
        n += snprintf(buf + n, (size_t)(cap - n), "%s=%ld;", PDEF[i].key, val);
    }
    return n >= 0 && n < cap ? n : 0;                  /* 0 = did not fit: the caller reports failure rather than a cut-off state */
}

static void state_read(inst_t *s, const char *text)
{
    int v1 = !strncmp(text, STATE_TAG_V1, strlen(STATE_TAG_V1));
    if (!v1 && strncmp(text, STATE_TAG, strlen(STATE_TAG)) != 0) return;   /* not ours (or a newer format): leave everything alone */
    double nv[P_COUNT];
    for (int i = 0; i < P_COUNT; i++) nv[i] = PDEF[i].def;
    const char *p = text + strlen(STATE_TAG);
    while (*p) {
        const char *eq = strchr(p, '='), *end = strchr(p, ';');
        if (!end) end = p + strlen(p);
        if (eq && eq < end) {
            char key[40];
            size_t kl = (size_t)(eq - p);
            if (kl < sizeof key) {
                memcpy(key, p, kl); key[kl] = 0;
                int id = find_key(key);
                if (id >= 0 && !no_state(id)) {
                    long val = strtol(eq + 1, NULL, 10);
                    if (PDEF[id].kind == K_SW && v1) nv[id] = val >= 1 ? 1.0 : 0.0;           /* 1.0.4 and before: 0/1 */
                    else if (PDEF[id].kind == K_KNOB || PDEF[id].kind == K_SW) nv[id] = val < 0 ? 0.0 : val > 1000000 ? 1.0 : (double)val / 1000000.0;
                    else { int top = opt_count(&PDEF[id]) - 1; nv[id] = val < 0 ? 0 : val > top ? top : (double)val; }
                }
            }
        }
        p = *end ? end + 1 : end;
    }
    nv[P_LP_CRV] = nv[P_LP_CURVE] / (DJ_FLT_NUM_CURVES - 1);                        /* the Q-Link slope controls follow */
    nv[P_HP_CRV] = nv[P_HP_CURVE] / (DJ_FLT_NUM_CURVES - 1);
    for (int i = 0; i < P_COUNT; i++) if (PDEF[i].kind != K_METER) s->v[i] = nv[i];   /* assign all, then apply all: */
    for (int i = 0; i < P_COUNT; i++) if (PDEF[i].kind != K_METER) apply(s, i);        /* a stage sees one consistent set */
    refresh_views(s);
    s->rev++;
}

static void set_param(void *inst, const char *key, const char *val)
{
    inst_t *s = inst;
    if (!strcmp(key, "state")) { state_read(s, val); return; }
    int id = find_key(key);
    if (id < 0 || PDEF[id].kind == K_METER) return;
    double old = s->v[id];
    s->v[id] = parse_value(&PDEF[id], val);
    apply(s, id);
    refresh_views(s);
    /* A switch or an option list: the wrapper settles a Q-Link move (a value between options) to the next option, while MPC keeps
     * the in-between value it sent, so its switch picture or option number can show something else. Moving display_rev makes the
     * wrapper's 100 ms poll report every value that moved (audioMasterAutomate, no UpdateDisplay), and the screen follows, also
     * with no audio running (until 0.2.0 only a moving meter moved display_rev). Only on a real change: the host's echo of that
     * report sets the same value again and must not start another round. */
    if ((PDEF[id].kind == K_TOG || PDEF[id].kind == K_OPT) && s->v[id] != old) s->rev++;
    /* the filter slope: a Q-Link turn of lp_crv picks the slope option; a tap (or preset, automation) on the option moves lp_crv
     * to that slope. Either way the other one is reported (display_rev), only when it really changes, so the host's echo of the
     * report changes nothing and no round trip follows. */
    if (id == P_LP_CRV || id == P_HP_CRV) {
        int cid = curve_of(id), c = crv_curve(s->v[id]);
        if (c != (int)s->v[cid]) { s->v[cid] = c; apply(s, cid); refresh_views(s); s->rev++; }
    } else if (id == P_LP_CURVE || id == P_HP_CURVE) {
        int kid = crv_of(id);
        if (crv_curve(s->v[kid]) != (int)s->v[id]) { s->v[kid] = s->v[id] / (DJ_FLT_NUM_CURVES - 1); s->rev++; }
    }
}

static int fmt_knob(const inst_t *s, int id, char *buf, int len)
{
    float n = (float)s->v[id];
    switch (id) {
    case P_IN_GAIN: return dj_fmt_gain(buf, len, dj_taper_gain_db(n, DJ_IN_N0, DJ_IN_MAX_DB));
    case P_WD_AMT: return dj_fmt_percent(buf, len, n);
    case P_BM_FREQ: return dj_fmt_hz(buf, len, dj_taper_bass_hz(n));
    case P_BAL: return dj_fmt_balance(buf, len, n);
    case P_EQ_LOW: case P_EQ_MID: case P_EQ_HIGH:
        return dj_fmt_eq(buf, len, dj_eq_knob_to_db(n, s->v[P_EQ_MODE] >= 0.5 ? DJ_EQ_MODE_ISOLATOR : DJ_EQ_MODE_EQ));
    case P_LP_CUT: case P_HP_CUT: return dj_fmt_hz(buf, len, dj_filter_cut_hz(n));
    case P_LP_RES: case P_HP_RES: return dj_fmt_percent(buf, len, n);
    case P_LP_CRV: case P_HP_CRV: {
        static const char *const SLOPE[DJ_FLT_NUM_CURVES] = { "12 dB", "24 dB", "36 dB", "48 dB", "96 dB", "BRICK" };
        return snprintf(buf, (size_t)len, "%s", SLOPE[crv_curve(s->v[id])]);
    }
    case P_IN_ON: case P_PH_INV: case P_PH_ON: case P_WD_ON: case P_BM_ON: case P_BAL_ON: case P_EQ_ON: case P_LP_ON: case P_HP_ON:
        return snprintf(buf, (size_t)len, "%s", on(s, id) ? "On" : "Off");
    case P_GT_THR: return s->v[id] <= GATE_OFF_KNOB ? snprintf(buf, (size_t)len, "Off") : dj_fmt_db(buf, len, dj_taper_thresh_db(n));
    case P_GT_ATT: return dj_fmt_ms(buf, len, dj_taper_attack_ms(n));
    case P_GT_HOLD: return dj_fmt_ms(buf, len, dj_taper_hold_ms(n));
    case P_GT_REL: return dj_fmt_ms(buf, len, dj_taper_release_ms(n));
    case P_MX_THR: return dj_fmt_db(buf, len, dj_taper_mx_thresh_db(n));
    case P_MX_ATT: return dj_fmt_ms(buf, len, dj_taper_mx_attack_ms(n));
    case P_MX_REL: return dj_fmt_ms(buf, len, dj_taper_release_ms(n));
    case P_MX_OUT: return dj_fmt_db(buf, len, dj_taper_mx_out_db(n));
    default: return 0;
    }
}

static int get_param(void *inst, const char *key, char *buf, int buf_len)
{
    inst_t *s = inst;
    if (buf_len <= 0) return 0;
    if (!strcmp(key, "display_rev")) return snprintf(buf, (size_t)buf_len, "%u", s->rev);
    if (!strcmp(key, "state")) return state_write(s, buf, buf_len);
    size_t kl = strlen(key);
    if (kl > 8 && !strcmp(key + kl - 8, "_display")) {          /* "<key>_display": the on-screen value text */
        char base[40];
        if (kl - 8 >= sizeof base) return 0;
        memcpy(base, key, kl - 8); base[kl - 8] = 0;
        int id = find_key(base);
        if (id < 0 || (PDEF[id].kind != K_KNOB && PDEF[id].kind != K_SW)) return 0;
        return fmt_knob(s, id, buf, buf_len);
    }
    int id = find_key(key);
    if (id < 0) return 0;
    switch (PDEF[id].kind) {
    case K_TOG: case K_OPT: return snprintf(buf, (size_t)buf_len, "%d", (int)s->v[id]);
    case K_KNOB: case K_SW: return snprintf(buf, (size_t)buf_len, "%g", s->v[id]);
    case K_METER: {
        double v;
        switch (id) {
        case P_M_IN_L: case P_M_IN_R: v = dj_meter_value01(&s->min, id == P_M_IN_R); break;
        case P_M_OUT_L: case P_M_OUT_R: v = dj_meter_value01(&s->mout, id == P_M_OUT_R); break;
        case P_MX_IN_L: case P_MX_IN_R: v = dj_meter_value01(&s->mxin, id == P_MX_IN_R); break;
        case P_M_FIN_L: case P_M_FIN_R: v = dj_meter_value01(&s->mfin, id == P_M_FIN_R); break;
        case P_FIN_DB_I: case P_FIN_DB_T: v = (id == P_FIN_DB_T) ? dj_readout_dec01(s->rd[RD_FIN].held_db) : dj_readout_int01(s->rd[RD_FIN].held_db); break;
        case P_MX_GR: v = dj_grmeter_value01(&s->gr); break;
        case P_IN_DB_I: case P_IN_DB_T: case P_OUT_DB_I: case P_OUT_DB_T: case P_MXI_DB_I: case P_MXI_DB_T: case P_GR_DB_I: case P_GR_DB_T: {
            int rd = (id - P_IN_DB_I) / 2;
            float db = rd == RD_GR ? -s->rd[RD_GR].held_db : s->rd[rd].held_db;   /* reduction reads as "-3.2 dB" */
            v = ((id - P_IN_DB_I) & 1) ? dj_readout_dec01(db) : dj_readout_int01(db);
            break;
        }
        case P_LP_VIEW: case P_HP_VIEW: v = (double)s->view[id == P_HP_VIEW] / (double)(DJ_VIEW_FRAMES - 1); break;
        default: v = 0.0; break;
        }
        return snprintf(buf, (size_t)buf_len, "%.4f", v);
    }
    }
    return 0;
}

static void render(void *inst, int16_t *out_lr, int frames)
{
    (void)inst;                                   /* an effect: no sound of its own */
    memset(out_lr, 0, (size_t)frames * 2 * sizeof(int16_t));
}

static int16_t to_i16(float x)
{
    float v = x * 32768.0f;
    if (!(v > -32768.0f)) return -32768;          /* also catches NaN */
    if (v > 32767.0f) return 32767;
    return (int16_t)lrintf(v);
}

static void process(void *inst, const int16_t *in_lr, int16_t *out_lr, int frames)
{
    inst_t *s = inst;
    float l[DJ_BLOCK], r[DJ_BLOCK];
    while (frames > 0) {
        int n = frames > DJ_BLOCK ? DJ_BLOCK : frames;
        for (int i = 0; i < n; i++) { l[i] = (float)in_lr[2 * i] * (1.0f / 32768.0f); r[i] = (float)in_lr[2 * i + 1] * (1.0f / 32768.0f); }
        for (int st = 0; st < S_MAX; st++) {
            run_stage(s, st, l, r, n);
            if (st == S_IN) {                              /* INPUT page: after the gain knob, before the phase invert */
                dj_meter_push(&s->min, l, r, n);
                dj_readout_push(&s->rd[RD_IN], dj_readout_lin_db(dj_block_peak(l, r, n)), n);
            } else if (st == S_BAL) {                      /* FILTER page: after balance (and its on/off), before the DJ EQ */
                dj_meter_push(&s->mfin, l, r, n);
                dj_readout_push(&s->rd[RD_FIN], dj_readout_lin_db(dj_block_peak(l, r, n)), n);
            }
        }
        dj_meter_push(&s->mxin, l, r, n);                  /* the maximizer's input: after the gate, before its threshold gain */
        dj_readout_push(&s->rd[RD_MXI], dj_readout_lin_db(dj_block_peak(l, r, n)), n);
        run_stage(s, S_MAX, l, r, n);
        dj_meter_push(&s->mout, l, r, n);
        dj_readout_push(&s->rd[RD_OUT], dj_readout_lin_db(dj_block_peak(l, r, n)), n);
        {   /* how hard the maximizer is driven (it always runs: no bypass): the meter shows the level going into its limiter
             * against the 0 dB ceiling, or the reduction still applied, whichever is more (src/dsp/dj_meter.h DJ_GR_START_DB) */
            float pl, pr, gr;
            dj_max_take_stats(&s->mx, &pl, &pr, &gr);
            float drive = dj_readout_lin_db(pl > pr ? pl : pr);
            dj_grmeter_push(&s->gr, gr > 0.0f && gr > drive ? gr : drive, n);   /* no reduction = 0 dB, not a reading */
            dj_readout_push(&s->rd[RD_GR], gr, n);         /* holds the deepest reduction */
        }
        for (int i = 0; i < n; i++) { out_lr[2 * i] = to_i16(l[i]); out_lr[2 * i + 1] = to_i16(r[i]); }

        /* tell the wrapper when a meter's filmstrip frame moved (it polls display_rev every ~100 ms) */
        const int top = DJ_METER_FRAMES - 1;
        int f[N_METER_FRAMES] = {
            (int)lroundf(dj_meter_value01(&s->min, 0) * top), (int)lroundf(dj_meter_value01(&s->min, 1) * top),
            (int)lroundf(dj_meter_value01(&s->mout, 0) * top), (int)lroundf(dj_meter_value01(&s->mout, 1) * top),
            (int)lroundf(dj_meter_value01(&s->mxin, 0) * top), (int)lroundf(dj_meter_value01(&s->mxin, 1) * top),
            (int)lroundf(dj_grmeter_value01(&s->gr) * top),
            (int)lroundf(dj_meter_value01(&s->mfin, 0) * top), (int)lroundf(dj_meter_value01(&s->mfin, 1) * top) };
        for (int k = 0; k < RD_COUNT; k++) {
            float db = k == RD_GR ? -s->rd[RD_GR].held_db : s->rd[k].held_db;
            dj_readout_frames(db, &f[N_BAR_FRAMES + 2 * k], &f[N_BAR_FRAMES + 1 + 2 * k]);
        }
        for (int i = 0; i < N_METER_FRAMES; i++) if (f[i] != s->last_frame[i]) { s->last_frame[i] = f[i]; s->rev++; }

        in_lr += 2 * n; out_lr += 2 * n; frames -= n;
    }
}

static const mpc_engine_t ENGINE = { create, destroy, midi, set_param, get_param, render, process, NULL /* song_pos: not a MIDI generator */ };
const mpc_engine_t *mpc_engine(void) { return &ENGINE; }
