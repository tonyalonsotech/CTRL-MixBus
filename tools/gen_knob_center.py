#!/usr/bin/env python3
"""Draw art/knob_center_strip.png: the house knob with a CENTRE-origin value ring, for controls whose middle is "nothing" (an EQ
band at 0 dB, a pan). The house style since 2026-11-10.

    tools/gen_knob_center.py [--check]

At 12 o'clock the ring shows only a dot. Turned left of centre, a white arc grows from 12 o'clock to the knob's position
counter-clockwise; turned right, it grows clockwise. Built from art/knob_strip.png (the house knob, 128 frames of 78 x 78, the
pointer sweeping -135..+135 degrees): each frame keeps that knob's body and pointer, its ring band is replaced by the dim track
(frame 0's ring, which has no white arc), and the white arc or the dot is drawn on top, smooth (8x larger, scaled down), in the
house ring's white, radius and width. Deterministic: a rerun is byte-identical.
"""
import math
import os
import sys

from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC, OUT = os.path.join(ROOT, "art", "knob_strip.png"), os.path.join(ROOT, "art", "knob_center_strip.png")
FW = 78                                   # frame size (house knob, r = 35 in the layout)
C = (FW - 1) / 2.0                        # the knob's centre
RING_R, RING_W = 36.6, 3.6                # the house ring's centre radius and width (measured from knob_strip.png)
RING_IN = 33.2                            # the ring band starts here (the knob body and its shadow end before it)
WHITE = (246, 246, 246)                   # the house ring's white
SWEEP = 270.0                             # the pointer's travel, degrees
DOT_DEG = 1.5                             # within this of 12 o'clock: the dot alone
SS = 8


def angle(k, n):
    """Frame k's pointer angle in degrees, clockwise from 12 o'clock."""
    return -SWEEP / 2 + SWEEP * k / (n - 1)


def arc_layer(a):
    """The white arc from 12 o'clock to angle a (or the dot), as an RGBA layer FW x FW."""
    m = Image.new("L", (FW * SS, FW * SS), 0)
    d = ImageDraw.Draw(m)
    cx = cy = (C + 0.5) * SS
    r, w = RING_R * SS, RING_W * SS
    def cap(deg):                          # a round end at angle deg
        t = math.radians(deg)
        x, y = cx + r * math.sin(t), cy - r * math.cos(t)
        d.ellipse((x - w / 2, y - w / 2, x + w / 2, y + w / 2), fill=255)
    if abs(a) < DOT_DEG:
        cap(0.0)
        dot = 1.25                          # the centre dot is a little larger than the line, so it reads as a dot
        t = 0.0
        x, y = cx, cy - r
        d.ellipse((x - w / 2 * dot, y - w / 2 * dot, x + w / 2 * dot, y + w / 2 * dot), fill=255)
    else:
        lo, hi = (0.0, a) if a > 0 else (a, 0.0)
        # PIL angles: 0 = 3 o'clock, clockwise; ours: 0 = 12 o'clock, clockwise
        d.arc((cx - r - w / 2, cy - r - w / 2, cx + r + w / 2, cy + r + w / 2), lo - 90, hi - 90, fill=255, width=round(w))
        cap(lo)
        cap(hi)
    m = m.resize((FW, FW), Image.LANCZOS)
    layer = Image.new("RGBA", (FW, FW), WHITE + (0,))
    layer.putalpha(m)
    return layer


def build():
    src = Image.open(SRC).convert("RGBA")
    n = src.height // FW
    track = src.crop((0, 0, FW, FW))      # frame 0: the dim track, no white arc
    band = Image.new("L", (FW, FW), 0)    # where the ring lives
    for y in range(FW):
        for x in range(FW):
            if math.hypot(x - C, y - C) >= RING_IN:
                band.putpixel((x, y), 255)
    out = Image.new("RGBA", (FW, FW * n), (0, 0, 0, 0))
    for k in range(n):
        f = src.crop((0, k * FW, FW, k * FW + FW))
        f.paste(track, (0, 0), band)      # the house frame with only the dim track in its ring band
        f.alpha_composite(arc_layer(angle(k, n)))
        out.paste(f, (0, k * FW))
    return out


def main():
    im = build()
    if "--check" in sys.argv:
        if not os.path.exists(OUT) or Image.open(OUT).convert("RGBA").tobytes() != im.tobytes():
            sys.exit("art/knob_center_strip.png out of date (run tools/gen_knob_center.py)")
        print("art/knob_center_strip.png up to date")
        return
    im.save(OUT, optimize=False)
    print("wrote %s (%dx%d)" % (OUT, im.width, im.height))


if __name__ == "__main__":
    main()
