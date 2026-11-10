#!/usr/bin/env python3
"""Write art/switch_off.png and art/switch_on.png: the pictures of the MPC on/off switch (a tap toggle since 2.0.1).

    tools/gen_switch_art.py [--from <slider filmstrip>]

They keep the look of the switch from 1.0.4 to 2.0.0, a short vertical slider (up = on): they are the bottom (value 0, off) and top
(value 1, on) frames of that slider's 28 x 56 filmstrip, which the skin builder rendered from the layout's slider_v and skin.css.
The default source is a 2.0.0/2.0.1 build's build/skin/.../sh_slider_v_28x56.png; once the layout has no slider_v that file is no
longer built, so the two pictures are kept in art/ and this only needs rerunning if the slider look is ever rebuilt.
"""
import os
import sys

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H = 28, 56                                   # tools/gen_layout.py SW_W, SW_H
SRC = os.path.join(ROOT, "build", "skin", "CTRL - VST - CTRL MixBus", "Plugin Skins", "sh_slider_v_%dx%d.png" % (W, H))


def main():
    src = sys.argv[sys.argv.index("--from") + 1] if "--from" in sys.argv else SRC
    s = Image.open(src).convert("RGBA")
    n = s.height // H
    s.crop((0, 0, W, H)).save(os.path.join(ROOT, "art", "switch_off.png"))                 # frame 0: thumb down, bypassed
    s.crop((0, (n - 1) * H, W, n * H)).save(os.path.join(ROOT, "art", "switch_on.png"))    # last frame: thumb up, processing
    print("wrote art/switch_off.png, art/switch_on.png from %s (%d frames)" % (os.path.relpath(src, ROOT), n))


if __name__ == "__main__":
    main()
