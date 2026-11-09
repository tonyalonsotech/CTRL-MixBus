#!/usr/bin/env python3
"""Draw the dB-number filmstrips under the meters: art/readout_int.png and art/readout_dec.png.

    tools/gen_readout_strip.py [--check]

MPC redraws plugin text only on a full UpdateDisplay, which this plugin does not send while meters run (vst.json
DISPLAY_REV_NO_UPDATE), so the live number is a picture, as the meters are. A frame per 0.1 dB would be 850 frames, too tall for
MPC's 12288 px strip limit, so the number is two strips side by side (frames as in src/dsp/dj_readout.h):

  readout_int.png  INT_W px wide, right-aligned: "-inf", "-60" .. "-1", "-0", "0" .. "24"   (DJ_READOUT_INT_FRAMES = 87)
  readout_dec.png  DEC_W px wide, left-aligned:  ".0 dB" .. ".9 dB", " dB"                   (DJ_READOUT_DEC_FRAMES = 11)

Laid side by side (tools/gen_layout.py readout()) they read "-12.3 dB", "2.0 dB", "-inf dB". Transparent background, the skin's
text colour and font (Titillium Web SemiBold, tools/font_path.py). Deterministic for a given Pillow and font.
"""
import os
import re
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from font_path import font  # noqa: E402
import palette  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INT_W, DEC_W, H = 44, 42, 22            # must equal tools/gen_layout.py READOUT_*
SIZE, SS = 17, 4                        # font px; supersampling for smooth edges
INK = palette.rgb(palette.TEXT) + (255,)  # tools/palette.py: the text colour

_R = open(os.path.join(ROOT, "src", "dsp", "dj_readout.h"), encoding="utf-8").read()
INT_FRAMES = int(re.search(r"#define\s+DJ_READOUT_INT_FRAMES\s+(\d+)", _R).group(1))
DEC_FRAMES = int(re.search(r"#define\s+DJ_READOUT_DEC_FRAMES\s+(\d+)", _R).group(1))

INT_TEXT = ["-inf"] + ["-%d" % v for v in range(60, 0, -1)] + ["-0"] + ["%d" % v for v in range(0, 25)]
DEC_TEXT = [".%d dB" % d for d in range(10)] + [" dB"]


def strip(texts, w, align):
    f = ImageFont.truetype(font(), SIZE * SS)
    big = Image.new("RGBA", (w * SS, H * SS * len(texts)), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    for i, t in enumerate(texts):
        cy = (i * H + H / 2) * SS
        if align == "right":
            d.text(((w - 1) * SS, cy), t, font=f, fill=INK, anchor="rm")
        else:
            d.text((1 * SS, cy), t, font=f, fill=INK, anchor="lm")
    return big.resize((w, H * len(texts)), Image.LANCZOS)


def main():
    if len(INT_TEXT) != INT_FRAMES or len(DEC_TEXT) != DEC_FRAMES:
        sys.exit("frame lists (%d, %d) do not match dj_readout.h (%d, %d)" % (len(INT_TEXT), len(DEC_TEXT), INT_FRAMES, DEC_FRAMES))
    outs = {"readout_int.png": strip(INT_TEXT, INT_W, "right"), "readout_dec.png": strip(DEC_TEXT, DEC_W, "left")}
    bad = []
    for name, im in outs.items():
        path = os.path.join(ROOT, "art", name)
        if "--check" in sys.argv:
            if not os.path.exists(path) or Image.open(path).convert("RGBA").tobytes() != im.tobytes():
                bad.append(name)
            continue
        im.save(path, optimize=False)
        print("wrote %s (%dx%d, %d frames)" % (path, im.width, im.height, im.height // H))
    if "--check" in sys.argv:
        if bad:
            sys.exit("%s out of date (run tools/gen_readout_strip.py)" % ", ".join(bad))
        print("art/readout_*.png up to date")


if __name__ == "__main__":
    main()
