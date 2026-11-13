#!/usr/bin/env python3
"""Generate layout.conf (the MPC touchscreen pages) and skin.css (the renderer's colours) so every coordinate and colour lives
in one place.

    tools/gen_layout.py           write layout.conf and skin.css
    tools/gen_layout.py --check   exit 1 if either is out of date

Four pages (INPUT, FILTER, OUTPUT, and UTILITY since 2.0.2). Rules the pages follow (see docs/DECISIONS.md):
  * every page is three columns of controls stacked VERTICALLY, because the Q-Links are a vertical column of four: one column
    on the screen = one Q-Link bank (A, B, C), top control = Q-Link 1 ... fourth = Q-Link 4. Switches (bypass, invert) are
    NOT on the Q-Links (2026-11-10): their place in a page's qlinks line is "-", they are touchscreen only; the picture/selector
    that heads a filter column is not on a Q-Link either. The slope buttons (12..BRICK)
    are for touch; their Q-Link turns lp_crv / hp_crv instead, which the engine turns into the slope (src/engine/dj_params.def),
    so the buttons follow a Q-Link exactly;
  * every switch is a TAP toggle (2.0.1: a tap turns it on or off; until 2.0.0 it was a slider dragged up or down); since 2.0.2 (the
    user's call, as CTRL Giant's) a rounded square button reading ON, art/switch_on.png (bright, white letters) and
    art/switch_off.png (dimmed), drawn by tools/gen_switch_art.py; every "_on" switch is on while its stage processes and off
    while it is bypassed, labelled "... ON/OFF" (it was "... BYPASS"). The switch at the bottom of each INPUT
    and FILTER column sits at one height, SW_BOTTOM_TOP, on both pages;
  * the DJ EQ knobs (HIGH, MID, LOW: 0 dB in the middle) use the house centre knob, art/knob_center_strip.png: a dot at
    12 o'clock, an arc growing left or right of it (tools/gen_knob_center.py);
  * INPUT and FILTER: the input AND output meters stand side by side at the far LEFT (the input meter is the page's own:
    INPUT after the gain knob, before the phase invert; FILTER after balance, before the DJ EQ), so both stay visible while MPC's Q-Link
    overlay covers the right of the screen; under every meter pair its dB number (the peak, held 1 s: two filmstrips side by
    side, tools/gen_readout_strip.py), because MPC only redraws plain text on a full screen update;
  * column titles are centred over the controls (a frame without a title, plus a centred text line);
  * the Balance knob is drawn without the value ring (a knob look with no arc);
  * OUTPUT (the gate and the maximizer): GATE | the maximizer's meters | MAXIMIZER, so the meters sit between the two stages they
    measure: input (after the gate, before the maximizer), reduction (from 2 dB before limiting), output; no title over them and no far meters on
    this page. The maximizer stays on Q-Link bank B;
  * colours: tools/palette.py, written here as the layout's theme_* lines and as skin.css (art_css=);
  * the page backgrounds (panels, titles, the meter labels) and the knobs are the user's own pictures (2026-11-09 skin edit):
    art/page_<tab>.png, each the whole 1280 x 628 plugin area, and art/knob_strip.png / art/knob_plain_strip.png, 128 frames of
    78 x 78. With PAGE_ART on, the layout draws no frames or static text of its own (they are in the pictures); set it False
    to draw them from this file again. Strip knobs use r=35 so the renderer's knob box (2r + 8) is 78 px: the frames are
    drawn at their own size, not resampled.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import palette as P  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "layout.conf")
CSS = os.path.join(ROOT, "skin.css")

# plugin area: x 0..1280, y 86..714 (tools/shadow_skin.py: Y_OFF = 86, H = 628)
TOP, BOT = 86, 714
METER_W, METER_H, METER_S_H = 36, 540, 520      # must equal tools/gen_meter_strip.py
METER_CY = 392                                  # 122..662: the "IN"/"OUT" label above, the dB number below
READOUT_INT_W, READOUT_DEC_W, READOUT_H = 44, 42, 22   # must equal tools/gen_readout_strip.py
READOUT_INT_FRAMES, READOUT_DEC_FRAMES = 87, 11        # src/dsp/dj_readout.h
READOUT_CY = 682                                # under the full-height meters
STRIP_FRAMES = 21                               # DJ_METER_FRAMES
_VIEW = open(os.path.join(ROOT, "src", "dsp", "dj_view.h"), encoding="utf-8").read()
VIEW_W, VIEW_H = (int(re.search(r"#define\s+DJ_VIEW_%s\s+(\d+)" % n, _VIEW).group(1)) for n in ("W", "H"))
# The FILTER page (2026-11-13, the user's layout): each filter's slope a 3 x 2 grid as wide as its curve picture, right under it;
# CUTOFF level with the EQ's MID knob, RESONANCE with its LOW; the EQ / ISOLATOR buttons as wide as the slope grids.
SLOPE_SW, SLOPE_SH = (VIEW_W - 2 * 2) // 3, 40                  # 3 x 72 + 2 gaps = 220 = VIEW_W
SLOPE_CY = 144 + 34 + VIEW_H // 2 + (118 - (2 * SLOPE_SH + 2)) // 2 + SLOPE_SH // 2   # its first row's centre (enum_h cy), centred
                                                                # in the 118 px between the curve and CUTOFF's box
EQ_MID_TOP = 144 + 52 + 128                                     # the EQ MID knob's box top (BODY_Y + 52 + 128: cy 363)
EQ_SW = (VIEW_W - 2) // 2                                       # 2 x 109 + 1 gap = 220
# Q-Link rings (tools/qlink_rings.py): the FILTER page's columns also take in the grids above their knobs
RING_EXTRA = {"FILTER": {0: ("lp_curve",), 1: ("hp_curve",), 2: ("eq_mode",)}}
VIEW_FRAMES = int(re.search(r"#define\s+DJ_VIEW_STEPS\s+(\d+)", _VIEW).group(1)) * int(re.search(r"#define\s+DJ_VIEW_RES\s+(\d+)", _VIEW).group(1)) + 1
SLOPES = ["12", "24", "36", "48", "96", "BRICK"]       # the lp_curve / hp_curve option labels

COL_W, COL_Y, COL_H = 336, 96, 608
COL_X_METERS = (200, 552, 904)                  # INPUT, FILTER: the meters take x 12..182 at the left
COL_X_PLAIN = (120, 472, 824)                   # OUTPUT: no side meters, the three columns centred
BODY_Y = 144                                    # below the frame title rule
TITLE_CY = COL_Y + 20                           # a column title, centred on the column
PAGE_ART = {"INPUT": "art/page_input.png", "FILTER": "art/page_filter.png", "OUTPUT": "art/page_output.png",
            "UTILITY": "art/page_utility.png"}   # (2.0.2: tools/gen_utility_art.py, from page_input.png)
R = 35                                          # knob radius: 2R + 8 = 78, the size of the knob strips' frames
KNOB_STRIP = "art/knob_strip.png"               # the user's knob (value ring), 128 frames
KNOB_FRAMES = 128
SW_W, SW_H = 56, 56                             # a switch picture (art/switch_*.png); its name below it
# control names are the page's own text, placed and sized as MPC's own name label was (mpc-vst-plugins shadow_skin.py: the knob's
# name box starts KNOB_NAME_Y below the knob box's top, NAME_H tall, 17 px; a switch's NAME_H box 4 px below the picture, 15 px)
NAME_H, KNOB_NAME_Y, KNOB_NAME_PX = 20, 2 * R + 5 + 2, 17
SW_NAME_DY, SW_NAME_PX = SW_H // 2 + 4 + NAME_H // 2, 15
ART_LABEL = "ff00ff"                            # the labels' colour in the page pictures (IN / REDUCTION / OUT on OUTPUT)
BAL_STRIP = "art/knob_plain_strip.png"          # the user's knob without the value ring, 128 frames
KNOB_CENTER = "art/knob_center_strip.png"       # the house centre knob (EQ bands): a dot at 12 o'clock, arc left or right
SW_BOTTOM_TOP = 582                             # top of the bottom switch in every INPUT / FILTER column (slider cy = 610):
                                                # the DJ EQ column's height, the lowest that clears its LOW knob's value text

# UTILITY (2.0.2): readouts as CTRL Giant's BETA TEST bank (a readout's centre, its name above it), the ABOUT bank as CTRL Giant's
UT_ROW0, UT_PITCH = 186, 86
ABOUT_LOGO_W, ABOUT_LOGO_H, ABOUT_LOGO_Y = 178, 240, 150


def version_text():
    import json
    v = json.load(open(os.path.join(ROOT, "vst.json"), encoding="utf-8"))["version"]
    return "%d.%d.%d" % (v // 1000, v // 100 % 10, v // 10 % 10)


# INPUT / FILTER side meters: IN pair, then OUT pair, at the far left
IN_CX, OUT_CX = 50, 144                         # pair centres; bars 20 px either side


def theme():
    """The renderer's colour variables (tools/html_art.py THEME; shadow_skin.py's Python-drawn parts read the same lines)."""
    t = {"bg": P.BG, "panel": P.BG, "line": P.BORDER, "ink": P.TEXT, "ink_dim": P.TEXT, "ink_faint": P.TEXT,
         "accent": P.TEXT, "accent_hi": P.TEXT, "knob_face": P.KNOB, "knob_ring": P.GRID, "knob_dot": P.POINTER,
         "lcd": P.OPTION, "box": P.OPTION, "seg_active": P.BTN_ON, "seg_inactive": P.BTN_OFF, "seg_active_tx": P.BTN_ON_TEXT,
         "btn_on": P.BTN_PRESS}   # (the flat, rounded buttons and menus of 2026-11-12: tools/palette.py BTN_*, OPTION)
    # (2026-11-13, house style from CTRL Giant) MPC rings the Q-Link column in use: one rectangle per column; tools/skin_post.py
    # sizes them (tools/qlink_rings.py)
    return ["theme_%s=%s" % kv for kv in t.items()] + ["art_css=skin.css", "qlink_bounds=column"]


def css():
    return ("/* generated by tools/gen_layout.py from tools/palette.py -- edit those, not this file.\n"
            " * Loaded after the renderer's default.css (layout art_css=): colours the parts the theme_* lines do not reach. */\n"
            ".slider-well { fill: #%s; stroke: #%s; stroke-width: 1; }\n"
            ".slider-fill { fill: #%s; }\n"
            ".slider-thumb { fill: #%s; stroke: #%s; stroke-width: 1.5; }\n"
            ".knob-pointer, .knob-arc { stroke: #%s; }\n"
            ".text { fill: #%s; }\n"
            "/* buttons and menus (the user's calls, 2026-11-12, from CTRL Giant): flat, no sheen or shadow; every button rounded; a button\n"
            " * not selected (fill BTN_OFF) has slightly darker letters, a selected one and a trigger button white letters; a menu's\n"
            " * options OPTION on the panel's BG, set apart by the gaps, the panel outlined; a closed menu (and a readout) a rounded\n"
            " * dark box, no border */\n"
            ":root { --sheen: 0; --shadow: none; }\n"
            ".seg-sheen, .button-sheen { display: none; }\n"
            ".seg-bg { rx: %dpx; ry: %dpx; }\n"
            ".seg[style*=\"--fill:#%s\"] .seg-tx { fill: #%s; }\n"
            ".button-tx { fill: #%s; }\n"
            ".tile { fill: #%s; }\n"
            ".tile-border { stroke: #%s; }\n"
            ".box { stroke: none; rx: %dpx; ry: %dpx; }\n") % (P.WELL, P.BORDER, P.KNOB, P.KNOB, P.POINTER, P.POINTER, P.TEXT,
                                                                P.BTN_RADIUS, P.BTN_RADIUS, P.BTN_OFF, P.BTN_OFF_TEXT, P.BTN_ON_TEXT, P.BG,
                                                                P.BORDER, P.BTN_RADIUS, P.BTN_RADIUS)


def tab(name):
    """A page header and, with PAGE_ART, its background picture (drawn first, so everything else is on top of it)."""
    L = ["[tab %s]" % name]
    if PAGE_ART:
        L.append("art file=%s fit=stretch" % PAGE_ART[name])
    return L


def label(cx, cy, text):
    """Static text, unless the page picture already has it."""
    return [] if PAGE_ART else ['text cx=%d cy=%d label="%s"' % (cx, cy, text)]


def frame(x, title):
    """A column box; its title is a separate centred text line (the renderer draws frame titles left-aligned).
    Nothing with PAGE_ART: the page picture has the boxes and titles."""
    if PAGE_ART:
        return []
    L = ['frame x=%d y=%d w=%d h=%d title=""' % (x, COL_Y, COL_W, COL_H)]
    if title:
        L.append('text cx=%d cy=%d label="%s" size=2 weight=700 spacing=1' % (x + COL_W // 2, TITLE_CY, title))
    return L


def knob(cx, cy, key, label, strip=KNOB_STRIP):
    """A knob and its name. The name is the page's own text, not MPC's (ns=0): MPC's name label shows the VST parameter's name,
    the page shows only what the knob does (house rule since 2026-11-11, PluginTemplate). The parameter keeps its full name for
    MPC's Q-Link and automation lists and the OLED screens. vy= keeps the value box where it was under MPC's name."""
    return ['knob cx=%d cy=%d r=%d label="%s" key=%s strip=%s frames=%d ns=0 vy=%d' % (cx, cy, R, label, key, strip, KNOB_FRAMES, NAME_H + 2),
            'text cx=%d cy=%d label="%s" size=%g weight=600' % (cx, cy - (2 * R + 10) // 2 + KNOB_NAME_Y + NAME_H // 2, label,
                                                                KNOB_NAME_PX / 10)]


def switch(cx, top, key, label):
    """An ON/OFF switch: a tap toggle, touchscreen only (its place in the page's qlinks line is "-"). Its name is the page's own text, as knob()'s."""
    cy = top + SW_H // 2
    return ['toggle cx=%d cy=%d w=%d h=%d label="%s" key=%s img=art/switch_off.png img_on=art/switch_on.png ns=0' % (
        cx, cy, SW_W, SW_H, label, key),
            'text cx=%d cy=%d label="%s" size=%g weight=600' % (cx, cy + SW_NAME_DY, label, SW_NAME_PX / 10)]


def readout(cx, cy, key):
    """The dB number centred on cx: the whole-dB strip (right-aligned) and the tenth + "dB" strip (left-aligned) side by side."""
    left = cx - (READOUT_INT_W + READOUT_DEC_W) // 2
    return ["meter cx=%d cy=%d w=%d h=%d key=%s_db_i strip=art/readout_int.png frames=%d"
            % (left + READOUT_INT_W // 2, cy, READOUT_INT_W, READOUT_H, key, READOUT_INT_FRAMES),
            "meter cx=%d cy=%d w=%d h=%d key=%s_db_t strip=art/readout_dec.png frames=%d"
            % (left + READOUT_INT_W + READOUT_DEC_W // 2, cy, READOUT_DEC_W, READOUT_H, key, READOUT_DEC_FRAMES)]


def meters(inp="in"):
    """Input pair and output pair side by side at the far left, full height; their dB numbers below.
    inp: the input meter's key stem: "in" (INPUT page: after the gain stage, before the phase invert) or "fin" (FILTER page:
    after balance, before the DJ EQ)."""
    L = []
    for pcx, keys in ((IN_CX, ("m_%s_l" % inp, "m_%s_r" % inp)), (OUT_CX, ("m_out_l", "m_out_r"))):
        for dx, key in zip((-20, 20), keys):
            L.append("meter cx=%d cy=%d w=%d h=%d key=%s strip=art/meter.png frames=%d" % (pcx + dx, METER_CY, METER_W, METER_H, key, STRIP_FRAMES))
    # always drawn: the INPUT and FILTER pictures have no labels over these meters; in the pictures' label colour
    for cx, t in ((IN_CX, "IN"), (OUT_CX, "OUT")):
        L.append('text cx=%d cy=104 label="%s"%s' % (cx, t, " color=%s" % ART_LABEL if PAGE_ART else ""))
    L += readout(IN_CX, READOUT_CY, inp) + readout(OUT_CX, READOUT_CY, "out")
    return L


def stack(cx, placed):
    """placed: [(top, item)] with item ('knob', key, label[, strip]) | ('sw', key, label) | ('enum', key, sw, sh).
    A knob's touch box is 136 px tall (ring + name + value text, tools/shadow_skin.py), a switch slider's SW_H + 56."""
    L = []
    for top, it in placed:
        if it[0] == "knob":
            L += knob(cx, top + 39, it[1], it[2], *it[3:])           # box = cy-39 .. cy+97
        elif it[0] == "sw":
            L += switch(cx, top, it[1], it[2])
        elif it[0] == "enum":
            L.append("enum_h cx=%d cy=%d sw=%d sh=%d key=%s" % (cx, top + it[3] // 2, it[2], it[3], it[1]))
    return L


def even(first_top, pitch, items):
    return list(zip([first_top + i * pitch for i in range(len(items))], items))


def build():
    L = ["# generated by tools/gen_layout.py -- edit that, not this file"] + theme()

    # ---------------------------------------------------------------- INPUT
    cols = COL_X_METERS
    cxs = [x + COL_W // 2 for x in cols]
    L += tab("INPUT")
    L += frame(cols[0], "GAIN + PHASE")
    L += stack(cxs[0], even(BODY_Y, 138, [("knob", "in_gain", "GAIN"), ("sw", "in_on", "GAIN ON/OFF"), ("sw", "ph_inv", "PHASE INVERT")]) +
               [(SW_BOTTOM_TOP, ("sw", "ph_on", "PHASE ON/OFF"))])
    L += frame(cols[1], "WIDTH + BASS MONO")
    L += stack(cxs[1], even(BODY_Y, 138, [("knob", "width", "WIDTH"), ("sw", "wd_on", "WIDTH ON/OFF"), ("knob", "bm_freq", "BASS FREQ")]) +
               [(SW_BOTTOM_TOP, ("sw", "bm_on", "BASS MONO ON/OFF"))])
    L += frame(cols[2], "BALANCE")
    L += stack(cxs[2], even(BODY_Y, 138, [("knob", "balance", "BALANCE", BAL_STRIP), ("sw", "bal_on", "BALANCE ON/OFF")]))
    L += meters()
    L.append('qlinks "INPUT" = in_gain,-,-,-,width,-,bm_freq,-,balance,-,-,-')

    # ---------------------------------------------------------------- FILTER (low-pass | high-pass | EQ: as CTRL Duck, the user's call 2.0.4)
    L += tab("FILTER")
    for i, (p, name) in enumerate((("lp", "LOW-PASS"), ("hp", "HIGH-PASS"))):
        L += frame(cols[i], name)
        # the response curve: one filmstrip per slope, the one for the current slope shown (when=); the engine picks the frame
        # from cutoff and resonance (src/dsp/dj_view.h). Same picture size and frame count as tools/gen_filter_strips.py.
        for sl in SLOPES:
            L.append("meter cx=%d cy=%d w=%d h=%d key=%s_view strip=art/flt_%s_%s.png frames=%d when=%s_curve:%s"
                     % (cxs[i], BODY_Y + 34, VIEW_W, VIEW_H, p, p, sl.lower(), VIEW_FRAMES, p, sl))
        # (2026-11-13, the user's layout) the slope: a 3 x 2 grid as wide as the curve above it, right under it; CUTOFF level with
        # the EQ's MID, RESONANCE with its LOW; the column's Q-Links follow: CURVE (the slope), CUTOFF, RESONANCE
        L.append("enum_h cx=%d cy=%d sw=%d sh=%d rows=2 key=%s_curve" % (cxs[i], SLOPE_CY, SLOPE_SW, SLOPE_SH, p))
        L += stack(cxs[i], [(EQ_MID_TOP, ("knob", p + "_cut", "CUTOFF")), (EQ_MID_TOP + 128, ("knob", p + "_res", "RESONANCE")),
                            (SW_BOTTOM_TOP, ("sw", p + "_on", "%s ON/OFF" % p.upper()))])
    L += frame(cols[2], "DJ EQ")
    L.append("enum_h cx=%d cy=%d sw=%d sh=40 key=eq_mode" % (cxs[2], BODY_Y + 24, EQ_SW))   # (2026-11-13: as wide as the slope grids)
    L += stack(cxs[2], even(BODY_Y + 52, 128, [("knob", "eq_high", "HIGH", KNOB_CENTER), ("knob", "eq_mid", "MID", KNOB_CENTER),
                                                    ("knob", "eq_low", "LOW", KNOB_CENTER)]) + [(SW_BOTTOM_TOP, ("sw", "eq_on", "EQ ON/OFF"))])
    L += meters("fin")
    L.append('qlinks "FILTER" = lp_crv,lp_cut,lp_res,-,hp_crv,hp_cut,hp_res,-,eq_high,eq_mid,eq_low,-')

    # ---------------------------------------------------------------- OUTPUT: GATE | maximizer meters | MAXIMIZER
    cols = COL_X_PLAIN
    cxs = [x + COL_W // 2 for x in cols]
    L += tab("OUTPUT")
    L += frame(cols[0], "GATE")
    L += stack(cxs[0], even(BODY_Y, 138, [("knob", "gt_thr", "THRESHOLD"), ("knob", "gt_att", "ATTACK"), ("knob", "gt_hold", "HOLD"), ("knob", "gt_rel", "RELEASE")]))
    L += frame(cols[1], "")                         # the meters' box, no title: what they show is plain from their labels
    mcy = 132 + METER_S_H // 2                      # 132..652, label above, dB number below
    for dx, lab, bars, rd in ((-112, "IN", (("mx_in_l", "meter_s"), ("mx_in_r", "meter_s")), "mxi"),
                              (0, "REDUCTION", (("mx_gr", "meter_gr_s"), ("mx_gr", "meter_gr_s")), "gr"),
                              (112, "OUT", (("m_out_l", "meter_s"), ("m_out_r", "meter_s")), "out")):
        cx = cxs[1] + dx
        L += label(cx, 116, lab)
        for ddx, (key, art) in zip((-20, 20), bars):
            L.append("meter cx=%d cy=%d w=%d h=%d key=%s strip=art/%s.png frames=%d" % (cx + ddx, mcy, METER_W, METER_S_H, key, art, STRIP_FRAMES))
        L += readout(cx, mcy + METER_S_H // 2 + 22, rd)
    L += frame(cols[2], "MAXIMIZER")
    L += stack(cxs[2], even(BODY_Y, 138, [("knob", "mx_thr", "THRESHOLD"), ("knob", "mx_att", "ATTACK"), ("knob", "mx_rel", "RELEASE"), ("knob", "mx_out", "OUTPUT")]))
    L.append('qlinks "OUTPUT" = gt_thr,gt_att,gt_hold,gt_rel,mx_thr,mx_att,mx_rel,mx_out')

    # ---------------------------------------------------------------- UTILITY (2.0.2, the user's layout): the IN / OUT meters at the far
    # left (as INPUT's), DSP | SIGNAL | ABOUT; the ABOUT bank as CTRL Giant's (the logo, the name, CREATED BY over TonyAlonso.Tech,
    # the version, at the same heights). The panels and titles are in art/page_utility.png.
    cols = COL_X_METERS
    cxs = [x + COL_W // 2 for x in cols]
    L += tab("UTILITY")
    rw = COL_W - 60
    for c, rows, (bkey, blab) in ((0, (("DSP LOAD", "dsp_load"), ("DSP PEAK", "dsp_peak"), ("OVERLOADS", "overloads"), ("BUILD", "build")),
                                   ("diag_rst", "RESET PEAK")),
                                  (1, (("INPUT PEAK", "in_peak"), ("OUTPUT PEAK", "out_peak"), ("CLIPS", "clips"), ("MAX REDUCTION", "max_gr")),
                                   ("sig_rst", "RESET PEAKS"))):
        for i, (name, key) in enumerate(rows):
            L.append('readout cx=%d cy=%d w=%d h=40 label="%s" key=%s' % (cxs[c], UT_ROW0 + i * UT_PITCH, rw, name, key))
        L.append('button cx=%d cy=%d w=150 h=36 label="%s" key=%s color=%s' % (cxs[c], BOT - 32, blab, bkey, P.BTN_ON))
    ax = cxs[2]
    L.append("art file=art/logo_about.png x=%d y=%d w=%d h=%d fit=contain" % (ax - ABOUT_LOGO_W // 2, ABOUT_LOGO_Y, ABOUT_LOGO_W, ABOUT_LOGO_H))
    L.append('text cx=%d cy=%d label="CTRL MixBus" size=3 weight=700 spacing=2' % (ax, 430))
    L.append('text cx=%d cy=%d label="CREATED BY" size=1.3 weight=600 spacing=1' % (ax, 472))
    L.append('text cx=%d cy=%d label="TonyAlonso.Tech" size=1.7 weight=600' % (ax, 498))
    L.append('text cx=%d cy=%d label="VERSION %s" size=1.5' % (ax, 542, version_text()))
    L += meters()
    L.append('qlinks "UTILITY" = -,-,-,-,-,-,-,-,-,-,-,-')
    return "\n".join(narrow_popup(x) for x in L) + "\n"


POP_PAD = 6                                     # the renderer's padding round a menu's options (shadow_skin.py POP_PAD)


def narrow_popup(line):
    """A plain menu (no groups) opens no wider than its field (the user's call, 2026-11-12, from CTRL Giant): one column of options
    the field's width less the list's padding."""
    if not line.startswith("popup ") or " groups=" in line or " cw=" in line:
        return line
    w = int(re.search(r" w=(\d+)", line).group(1))
    return re.sub(r" cols=\d+", "", line) + " cw=%d cols=1" % (w - 2 * POP_PAD)


def main():
    outs = {OUT: build(), CSS: css()}
    if "--check" in sys.argv:
        bad = [os.path.basename(p) for p, t in outs.items() if not os.path.exists(p) or open(p, encoding="utf-8").read() != t]
        if bad:
            sys.exit("%s out of date (run tools/gen_layout.py)" % ", ".join(bad))
        print("layout.conf, skin.css up to date")
        return
    for p, t in outs.items():
        open(p, "w", encoding="utf-8", newline="\n").write(t)
    print("wrote layout.conf, skin.css")


if __name__ == "__main__":
    main()
