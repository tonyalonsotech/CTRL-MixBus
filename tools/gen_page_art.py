#!/usr/bin/env python3
"""Retitle the INPUT and OUTPUT page pictures (2.1.1, the user's layout), in place:
  art/page_input.png   the three panel titles: STEREO, TRANSIENT, GATE (were GAIN + PHASE, WIDTH + BASS MONO, BALANCE);
  art/page_output.png  built from art/src/page_output.png (the user's 2.1.0 picture, kept as it was): (2.1.2) the middle panel
                       60 px wider (its inside repeated in the middle), the side panels 30 px further out, their shadows and gaps
                       moving with them (tools/gen_layout.py COL_X_PLAIN, OUT_MID_W); the left panel's title COMPRESSOR (was
                       GATE), and the middle panel's meter labels IN, COMP, MAX, OUT (were IN, REDUCTION, OUT) over the four
                       meter pairs tools/gen_layout.py places (OUT_METERS).
Everything else in the pictures is the user's own (2026-11-09 skin edit) and stays as it is. Each title band is painted over
with the panel colour and the new words drawn as the originals are (Titillium Web Bold, the same capital height, letter spacing
and magenta: tools/gen_utility_art.py's title_font for the titles; the meter labels' spacing found by matching the old
"REDUCTION"'s width, 80 px at 10 px capitals).

    tools/gen_page_art.py [--check]

Deterministic: page_input.png is painted over in place (its own words included), page_output.png rebuilt from its source, so a
rerun is byte-identical (and tools/gen_utility_art.py, which takes the panels from
art/page_input.png and paints its own titles, is unchanged by it).
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from font_path import font  # noqa: E402
import gen_layout as G  # noqa: E402
import gen_utility_art as U  # noqa: E402

PAGES = {
    os.path.join(G.ROOT, "art", "page_input.png"): (G.COL_X_METERS, ("STEREO", "TRANSIENT", "GATE")),
    os.path.join(G.ROOT, "art", "page_output.png"): (G.COL_X_PLAIN, ("COMPRESSOR", None, None)),   # None: keep that title
}
OUT_SRC = os.path.join(G.ROOT, "art", "src", "page_output.png")
LABEL_CAP, LABEL_TOP, LABEL_REF = 10, 28, ("REDUCTION", 80)    # the OUTPUT page's meter labels (picture y)


def label_font():
    for size in range(8, 30):
        f = ImageFont.truetype(font("TitilliumWeb-Bold.ttf"), size * U.SS)
        b = f.getbbox("H")
        if (b[3] - b[1]) / U.SS >= LABEL_CAP - 0.25:
            break
    t, w = LABEL_REF
    return f, (w * U.SS - U.text_width(f, t, 0)) / (len(t) - 1)


def draw(im, x0, x1, fill, f, sp, cap_top, words):
    """Paint the band x0..x1 (picture y U.BAND) with the panel colour and draw each (text, centre x) on it."""
    im.paste(fill, (x0, U.BAND[0], x1, U.BAND[1]))
    band = Image.new("RGB", ((x1 - x0) * U.SS, (U.BAND[1] - U.BAND[0]) * U.SS), fill)
    d = ImageDraw.Draw(band)
    top = (cap_top - U.BAND[0]) * U.SS - f.getbbox("H")[1]
    for t, cx in words:
        x = (cx - x0) * U.SS - U.text_width(f, t, sp) / 2
        for ch in t:
            d.text((x, top), ch, font=f, fill=U.INK)
            x += f.getlength(ch) + sp
    im.paste(band.resize((x1 - x0, U.BAND[1] - U.BAND[0]), Image.LANCZOS), (x0, U.BAND[0]))


def widen_output():
    """The 2.1.0 picture (panels at x 120, 472, 824, each 336 wide) with the middle panel OUT_MID_W wide and the others moved out."""
    src = Image.open(OUT_SRC).convert("RGB")
    grow = G.OUT_MID_W - G.COL_W                     # 60
    old_mid, half = 472, G.COL_W // 2                # the middle panel's x and half width in the source
    shift = grow // 2                                # 30: the left panel moves this far left, the right one this far right
    im = Image.new("RGB", src.size)
    im.paste(src.crop((shift, 0, old_mid, src.height)), (0, 0))                       # left margin, panel and gap
    im.paste(src.crop((old_mid, 0, old_mid + half, src.height)), (old_mid - shift, 0))  # the middle panel's left half
    col = src.crop((old_mid + half, 0, old_mid + half + 1, src.height))
    for x in range(grow):                                                              # its inside, repeated
        im.paste(col, (old_mid - shift + half + x, 0))
    im.paste(src.crop((old_mid + half, 0, src.width - shift, src.height)), (old_mid - shift + half + grow, 0))
    assert old_mid - shift == G.COL_X_PLAIN[1] and 824 + shift == G.COL_X_PLAIN[2]
    return im


def page(path, cols, titles):
    out = path.endswith("page_output.png")
    im = widen_output() if out else Image.open(path).convert("RGB")
    fill = im.getpixel((cols[0] + 100, 100))
    f, sp = U.title_font()
    for x0, t in zip(cols, titles):
        if t:
            draw(im, x0 + 2, x0 + G.COL_W - 2, fill, f, sp, U.TITLE_TOP, [(t, x0 + G.COL_W // 2)])
    if out:
        lf, lsp = label_font()
        x0 = cols[1]
        draw(im, x0 + 2, x0 + G.OUT_MID_W - 2, fill, lf, lsp, LABEL_TOP, [(t, x0 + G.OUT_MID_W // 2 + dx) for dx, t, _, _ in G.OUT_METERS])
    return im


def main():
    bad = []
    for path, (cols, titles) in PAGES.items():
        im = page(path, cols, titles)
        if "--check" in sys.argv:
            if Image.open(path).convert("RGB").tobytes() != im.tobytes():
                bad.append(os.path.basename(path))
            continue
        im.save(path, optimize=False)
        print("wrote %s" % os.path.relpath(path, G.ROOT))
    if "--check" in sys.argv:
        if bad:
            sys.exit("%s out of date (run tools/gen_page_art.py)" % ", ".join(bad))
        print("art/page_input.png, art/page_output.png up to date")


if __name__ == "__main__":
    main()
