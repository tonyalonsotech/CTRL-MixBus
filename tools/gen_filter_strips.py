#!/usr/bin/env python3
"""Draw the filter-curve filmstrips art/flt_<lp|hp>_<slope>.png, measured from the plugin's own filter code.

    tools/gen_filter_strips.py [--check]

For every filter (LP, HP) and slope (12, 24, 36, 48, 96, BRICK) one strip holds DJ_VIEW_STEPS cutoff positions x DJ_VIEW_RES
resonance buckets, then one flat "bypassed" frame (src/dsp/dj_view.h: sizes shared with the engine and the layout). Each
response is the real dj_filter.c's impulse response, FFT'd (a throwaway shared library is compiled with gcc for that: the
curves cannot drift from the DSP). The axis is log frequency 20 Hz .. 20 kHz, the same law as the cutoff knob, so knob position
n sits at x = n * (W - 1). Needs gcc, numpy, Pillow. The committed PNGs are the shipped artifacts: a build never needs this.
"""
import ctypes
import os
import re
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from font_path import font  # noqa: E402
import palette  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SLOPES = ["12", "24", "36", "48", "96", "BRICK"]          # = the lp_curve / hp_curve option labels, in order
FS, NFFT, SS = 44100.0, 65536, 3                          # SS: supersampling for smooth lines

BG, GRID, ZERO, TICKTXT = (palette.rgb(c) for c in (palette.WELL, palette.GRID, palette.ZERO, palette.TEXT))   # tools/palette.py
BORDER = palette.rgb(palette.BORDER)
COLORS = {"lp": (96, 200, 255), "hp": (255, 176, 72)}      # low-pass cyan, high-pass amber


def constants():
    src = open(os.path.join(ROOT, "src", "dsp", "dj_view.h"), encoding="utf-8").read()
    c = {}
    for name in ("W", "H", "STEPS", "RES", "DB_TOP", "DB_BOT"):
        m = re.search(r"#define\s+DJ_VIEW_%s\s+\(?\s*(-?[0-9.]+)" % name, src)
        if not m:
            sys.exit("dj_view.h: DJ_VIEW_%s not found" % name)
        c[name] = float(m.group(1))
    for k in ("W", "H", "STEPS", "RES"):
        c[k] = int(c[k])
    c["FRAMES"] = c["STEPS"] * c["RES"] + 1
    if c["FRAMES"] * c["H"] > 12288:
        sys.exit("strip would be %d px tall (MPC's limit is 12288): lower DJ_VIEW_STEPS or DJ_VIEW_H" % (c["FRAMES"] * c["H"]))
    return c


def load_shim(tmp):
    so = os.path.join(tmp, "libfiltershim.so")
    cmd = ["gcc", "-O2", "-shared", "-fPIC", "-ffp-contract=off", "-o", so, os.path.join(ROOT, "tools", "filter_shim.c"),
           os.path.join(ROOT, "src", "dsp", "dj_filter.c"), "-lm"]
    subprocess.run(cmd, check=True)
    lib = ctypes.CDLL(so)
    lib.shim_impulse.argtypes = [ctypes.c_int, ctypes.c_float, ctypes.c_float, ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_float)]
    return lib


def response_db(lib, typ, cut, res, curve, xs_hz):
    buf = (ctypes.c_float * NFFT)()
    lib.shim_impulse(typ, cut, res, curve, NFFT, buf)
    h = np.fft.rfft(np.frombuffer(buf, dtype=np.float32).astype(np.float64))
    db = 20.0 * np.log10(np.maximum(np.abs(h), 1e-9))
    return np.interp(xs_hz, np.fft.rfftfreq(NFFT, 1.0 / FS), db)


class Painter:
    def __init__(self, c):
        self.c = c
        self.W, self.H = c["W"], c["H"]
        self.y0, self.y1 = 4, self.H - 15                  # plot area; the labels sit below it
        self.small = ImageFont.truetype(font(), 9 * SS)
        self.mid = ImageFont.truetype(font(), 15 * SS)

    def x_of(self, f_hz):
        return np.log10(f_hz / 20.0) / 3.0 * (self.W - 1)

    def y_of(self, db):
        c = self.c
        db = np.clip(db, c["DB_BOT"], c["DB_TOP"])
        return self.y0 + (c["DB_TOP"] - db) / (c["DB_TOP"] - c["DB_BOT"]) * (self.y1 - self.y0)

    def base(self):
        W, H = self.W * SS, self.H * SS
        im = Image.new("RGB", (W, H), BG)               # RGB, not RGBA: a translucent fill must BLEND with the background (on RGBA
        d = ImageDraw.Draw(im)                          # Pillow overwrites pixels, leaving see-through ones); frames come out opaque
        d.rounded_rectangle((0, 0, W - 1, H - 1), 5 * SS, fill=BG, outline=BORDER, width=SS)
        for f, lab in ((100, "100"), (1000, "1k"), (10000, "10k")):
            x = self.x_of(f) * SS
            d.line((x, self.y0 * SS, x, self.y1 * SS), fill=GRID, width=SS)
            tw = d.textlength(lab, font=self.small)
            d.text((x - tw / 2, (self.y1 + 2) * SS), lab, font=self.small, fill=TICKTXT)
        for db, col in ((0, ZERO), (-20, GRID), (-40, GRID)):
            y = self.y_of(db) * SS
            d.line((0, y, W, y), fill=col, width=SS)
        return im

    def frame(self, kind, xs_px, db, cut_x, bypass):
        im = self.base()
        d = ImageDraw.Draw(im, "RGBA")
        col = COLORS[kind]
        if bypass:
            y = self.y_of(0.0) * SS
            d.line((0, y, im.width, y), fill=ZERO + (255,), width=2 * SS)
            lab = "BYPASSED"
            tw = d.textlength(lab, font=self.mid)
            d.text(((im.width - tw) / 2, (self.y0 + 5) * SS), lab, font=self.mid, fill=TICKTXT + (255,))
        else:
            pts = [(round(float(x) * 8) / 8 * SS, round(float(self.y_of(v)) * 8) / 8 * SS) for x, v in zip(xs_px, db)]
            area = pts + [(pts[-1][0], self.y1 * SS), (pts[0][0], self.y1 * SS)]
            d.polygon(area, fill=col + (64,))
            if cut_x is not None:
                d.line((cut_x * SS, self.y0 * SS, cut_x * SS, self.y1 * SS), fill=(200, 202, 208, 110), width=SS)
            d.line(pts, fill=col + (255,), width=int(2.2 * SS), joint="curve")
        return im.resize((self.W, self.H), Image.LANCZOS).convert("RGBA")


def build():
    c = constants()
    W, H, F = c["W"], c["H"], c["FRAMES"]
    xs_px = np.arange(W)
    xs_hz = 20.0 * 1000.0 ** (xs_px / (W - 1))
    out = {}
    with tempfile.TemporaryDirectory() as tmp:
        lib = load_shim(tmp)
        paint = Painter(c)
        for kind, typ in (("lp", 0), ("hp", 1)):
            for ci, slope in enumerate(SLOPES):
                strip = Image.new("RGBA", (W, H * F), BG + (255,))
                for step in range(c["STEPS"]):
                    cut = step / (c["STEPS"] - 1)
                    for rb in range(c["RES"]):
                        res = rb / (c["RES"] - 1)
                        db = response_db(lib, typ, cut, res, ci, xs_hz)
                        fr = step * c["RES"] + rb
                        strip.paste(paint.frame(kind, xs_px, db, cut * (W - 1), False), (0, fr * H))
                strip.paste(paint.frame(kind, xs_px, None, None, True), (0, (F - 1) * H))
                out["flt_%s_%s.png" % (kind, slope.lower())] = strip
    return out


def main():
    imgs = build()
    d = os.path.join(ROOT, "art")
    if "--check" in sys.argv:
        bad = [n for n, im in imgs.items() if not os.path.exists(os.path.join(d, n)) or Image.open(os.path.join(d, n)).convert("RGBA").tobytes() != im.tobytes()]
        if bad:
            sys.exit("out of date: %s (run tools/gen_filter_strips.py)" % ", ".join(bad))
        print("filter strips up to date (%d files)" % len(imgs))
        return
    os.makedirs(d, exist_ok=True)
    for n, im in imgs.items():
        im.save(os.path.join(d, n), optimize=True)
    print("wrote %d strips, %dx%d px each, to %s" % (len(imgs), next(iter(imgs.values())).width, next(iter(imgs.values())).height, d))


if __name__ == "__main__":
    main()
