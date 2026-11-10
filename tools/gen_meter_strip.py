#!/usr/bin/env python3
"""Draw the full-height meter filmstrips: art/meter.png (levels) and art/meter_gr.png (maximizer gain reduction).

    tools/gen_meter_strip.py [--check]

All are DJ_METER_FRAMES (21) frames stacked down, one bar 36 px wide per frame:
  meter.png       540 px tall: the full-height input/output meters at the far left and right of the INPUT and FILTER pages.
  meter_s.png     520 px tall: the same bar, for the maximizer's meter column on the OUTPUT page.
                  Level bars: 20 segments lit from the BOTTOM, 3 dB each over -60..0 dBFS (the range of src/dsp/dj_meter.h): green to
                  -12 dB, amber to -3 dB, red above. Frame n = n segments lit.
  meter_gr_s.png  520 px tall: 20 segments lit from the TOP, 1 dB each, from 2 dB before limiting (DJ_GR_START_DB) to 18 dB of
                  reduction: amber, red from 12 dB of reduction. Frame n = n lit.
The last frame (all segments lit) has a 3 px red border (palette.CLIP), a clip light (the 2026-11-09 skin edit; its four
outer corner pixels stay the well colour).
Why 21 frames: MPC draws filmstrips up to 12288 px tall correctly and a 540 px bar x 21 frames = 11340 px; each frame is one LED step.
Fully deterministic. W, H, FRAMES must match tools/gen_layout.py (METER_W, METER_H) and src/dsp/dj_meter.h.
"""
import os
import re
import sys
from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import palette  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W = 36
_M = open(os.path.join(ROOT, "src", "dsp", "dj_meter.h"), encoding="utf-8").read()
SEGS = int(re.search(r"#define\s+DJ_METER_SEGS\s+(\d+)", _M).group(1))
GR_START = float(re.search(r"#define\s+DJ_GR_START_DB\s+\(?(-?[0-9.]+)", _M).group(1))
FRAMES = SEGS + 1

BG, OFF = palette.rgb(palette.WELL) + (255,), palette.rgb(palette.UNLIT) + (255,)   # tools/palette.py
CLIP, CLIP_W = palette.rgb(palette.CLIP) + (255,), 3
GREEN, AMBER, RED = (70, 200, 110, 255), (232, 170, 50, 255), (226, 70, 56, 255)
OUTS = {"meter.png": ("level", 540), "meter_s.png": ("level", 520), "meter_gr_s.png": ("gr", 520)}   # file: (kind, bar height px)


def seg_color(kind, i):
    """i = 0 is the first segment lit (bottom for levels, top for reduction)."""
    if kind == "level":
        db = -60.0 + 60.0 * (i + 1) / SEGS
        return RED if db > -3.0 else AMBER if db > -12.0 else GREEN
    return RED if GR_START + (i + 1) > 12 else AMBER          # segment i lights at GR_START + i + 1 dB of reduction


def build(kind, H):
    if FRAMES * H > 12288:
        sys.exit("strip would be %d px tall: MPC draws up to 12288 px right" % (FRAMES * H))
    im = Image.new("RGBA", (W, H * FRAMES), BG)
    d = ImageDraw.Draw(im)
    pad, gap = 6, 3
    sh = (H - 2 * pad - (SEGS - 1) * gap) / SEGS
    for f in range(FRAMES):
        y0 = f * H
        d.rounded_rectangle((0, y0, W - 1, y0 + H - 1), 5, fill=BG)
        if f == FRAMES - 1:
            d.rectangle((0, y0, W - 1, y0 + H - 1), outline=CLIP, width=CLIP_W)
            for x, y in ((0, y0), (W - 1, y0), (0, y0 + H - 1), (W - 1, y0 + H - 1)):
                im.putpixel((x, y), BG)
        for i in range(SEGS):
            slot = i if kind == "level" else SEGS - 1 - i          # level: slot 0 at the bottom; reduction: lit from the top
            if kind == "level":
                top = y0 + H - pad - (slot + 1) * sh - slot * gap
            else:
                top = y0 + pad + i * (sh + gap)
            col = seg_color(kind, i) if i < f else OFF
            d.rounded_rectangle((pad, round(top), W - 1 - pad, round(top + sh) - 1), 2, fill=col)
    return im


def main():
    bad = []
    for name, (kind, h) in OUTS.items():
        path = os.path.join(ROOT, "art", name)
        im = build(kind, h)
        if "--check" in sys.argv:
            old = Image.open(path).convert("RGBA").tobytes() if os.path.exists(path) else b""
            if old != im.tobytes():
                bad.append(name)
            continue
        os.makedirs(os.path.dirname(path), exist_ok=True)
        im.save(path, optimize=False)
        print("wrote %s (%dx%d, %d frames)" % (path, im.width, im.height, FRAMES))
    if "--check" in sys.argv:
        if bad:
            sys.exit("%s out of date (run tools/gen_meter_strip.py)" % ", ".join(bad))
        print("art/meter*.png up to date")


if __name__ == "__main__":
    main()
