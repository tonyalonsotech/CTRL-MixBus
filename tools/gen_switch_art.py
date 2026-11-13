#!/usr/bin/env python3
"""Draw the house ON/OFF switch: art/switch_off.png and art/switch_on.png.

    tools/gen_switch_art.py [--check]

Since 2026-11-12 (the user's call; CTRL Giant 1.2.2) a switch is a rounded SQUARE button, as tall as the old switch (56 px), always reading
ON: off = a dimmed fill (palette BTN_OFF) with slightly darker letters (BTN_OFF_TEXT), on = the brighter fill (BTN_ON) with white
letters; corners as RESET PEAK's (BTN_RADIUS). Still a TAP toggle, touchscreen only (never on a Q-Link); tools/gen_layout.py
toggle() places it (its name, "... ON/OFF" etc., under it); its parameter is an SW (or an OFF|ON option) in params.def.
Drawn 4x larger and scaled down (smooth edges). Deterministic: a rerun is byte-identical.
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import palette as P  # noqa: E402
from font_path import font  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H = 56, 56                             # = tools/gen_layout.py SW_W, SW_H
TEXT_PX = 17
SS = 4


def button(on, fill=None, ink=None):
    w, h = W * SS, H * SS
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    fill = fill or (P.BTN_ON if on else P.BTN_OFF)
    ink = ink or (P.BTN_ON_TEXT if on else P.BTN_OFF_TEXT)
    d.rounded_rectangle((0, 0, w - 1, h - 1), P.BTN_RADIUS * SS, fill=P.rgb(fill) + (255,))
    f = ImageFont.truetype(font(), TEXT_PX * SS)
    tw = d.textlength("ON", font=f)
    d.text((w / 2 - tw / 2, h / 2), "ON", font=f, fill=P.rgb(ink) + (255,), anchor="lm")
    return im.resize((W, H), Image.LANCZOS)


def main():
    outs = {"switch_off.png": button(False), "switch_on.png": button(True)}
    bad = []
    for name, im in outs.items():
        path = os.path.join(ROOT, "art", name)
        if "--check" in sys.argv:
            if not os.path.exists(path) or Image.open(path).convert("RGBA").tobytes() != im.tobytes():
                bad.append(name)
            continue
        im.save(path, optimize=False)
        print("wrote %s (%dx%d)" % (path, im.width, im.height))
    if "--check" in sys.argv:
        if bad:
            sys.exit("%s out of date (run tools/gen_switch_art.py)" % ", ".join(bad))
        print("art/switch_*.png up to date")


if __name__ == "__main__":
    main()
