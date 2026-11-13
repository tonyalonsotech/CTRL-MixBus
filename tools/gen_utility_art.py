#!/usr/bin/env python3
"""Draw the UTILITY page's pictures (2.0.2, the user's call):
  art/page_utility.png  the page background: art/page_input.png (the user's own picture: the three panels, the page colour round
                        them) with its three titles replaced by DSP, SIGNAL and ABOUT, drawn as the originals are (Titillium Web
                        Bold, the same height, letter spacing and magenta; the spacing found by matching "GAIN + PHASE"'s width);
  art/logo_about.png    the user's "CTRL MixBus Logo.png" (this folder) fitted into the ABOUT bank's 178 x 240 box (as CTRL Giant's).

    tools/gen_utility_art.py [--check]

Deterministic: a rerun is byte-identical.
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from font_path import font  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "art", "page_input.png")
LOGO_SRC = os.path.join(ROOT, "CTRL MixBus Logo.png")
OUT_PAGE = os.path.join(ROOT, "art", "page_utility.png")
OUT_LOGO = os.path.join(ROOT, "art", "logo_about.png")
COL_X, COL_W = (200, 552, 904), 336          # = tools/gen_layout.py COL_X_METERS, COL_W
TITLES = ("DSP", "SIGNAL", "ABOUT")
BAND = (14, 48)                              # the title band (picture y), inside the panel's 1 px border
TITLE_TOP = 24                               # the originals' capital letters: y 24..37 (14 px)
INK = (255, 11, 255)
LOGO_W, LOGO_H = 178, 240
SS = 4


def text_width(f, t, sp):
    return sum(f.getlength(ch) for ch in t) + sp * (len(t) - 1)


def title_font():
    """The size whose capitals are 14 px tall, and the letter spacing that makes "GAIN + PHASE" 132 px wide (its width on page_input)."""
    for size in range(14, 30):
        f = ImageFont.truetype(font("TitilliumWeb-Bold.ttf"), size * SS)
        b = f.getbbox("H")
        if (b[3] - b[1]) / SS >= 14 - 0.25:
            break
    sp = (132 * SS - text_width(f, "GAIN + PHASE", 0)) / 11.0
    return f, sp


def page():
    im = Image.open(SRC).convert("RGB")
    fill = im.getpixel((COL_X[0] + 100, 100))
    f, sp = title_font()
    for x0, t in zip(COL_X, TITLES):
        im.paste(fill, (x0 + 2, BAND[0], x0 + COL_W - 2, BAND[1]))
        band = Image.new("RGB", ((COL_W - 4) * SS, (BAND[1] - BAND[0]) * SS), fill)
        d = ImageDraw.Draw(band)
        x = (band.width - text_width(f, t, sp)) / 2
        top = (TITLE_TOP - BAND[0]) * SS - f.getbbox("H")[1]
        for ch in t:
            d.text((x, top), ch, font=f, fill=INK)
            x += f.getlength(ch) + sp
        im.paste(band.resize((COL_W - 4, BAND[1] - BAND[0]), Image.LANCZOS), (x0 + 2, BAND[0]))
    return im


def logo():
    src = Image.open(LOGO_SRC).convert("RGBA")
    k = min(LOGO_W / src.width, LOGO_H / src.height)
    fitted = src.resize((round(src.width * k), round(src.height * k)), Image.LANCZOS)
    out = Image.new("RGBA", (LOGO_W, LOGO_H), (0, 0, 0, 0))
    out.paste(fitted, ((LOGO_W - fitted.width) // 2, (LOGO_H - fitted.height) // 2), fitted)
    return out


def main():
    outs = {OUT_PAGE: page(), OUT_LOGO: logo()}
    bad = []
    for path, im in outs.items():
        if "--check" in sys.argv:
            if not os.path.exists(path) or Image.open(path).convert(im.mode).tobytes() != im.tobytes():
                bad.append(os.path.basename(path))
            continue
        im.save(path, optimize=False)
        print("wrote %s (%dx%d)" % (os.path.relpath(path, ROOT), im.width, im.height))
    if "--check" in sys.argv:
        if bad:
            sys.exit("%s out of date (run tools/gen_utility_art.py)" % ", ".join(bad))
        print("art/page_utility.png, art/logo_about.png up to date")


if __name__ == "__main__":
    main()
