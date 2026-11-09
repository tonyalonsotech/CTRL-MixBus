"""The skin's colours, in one place: tools/gen_layout.py writes them as the layout's theme_* lines (the renderer's own parts:
frames, knobs, sliders, text), and the generators that draw art/ (meters, dB numbers, filter pictures, the plain Balance knob)
import them, so every picture matches the theme. Change a colour here, then rerun tools/gen_*.py and rebuild.

The five named colours are the brief (2026-11-09); the rest are darker shades of the background for wells, unlit segments and
grid lines, which the brief does not name."""

BG = "370038"          # page background
TEXT = "feb9ff"        # all text: titles, labels, values, numbers, axis ticks
BORDER = "ffffff"      # frame and display borders
KNOB = "8a52e8"        # knob faces (and the selected option of a selector)
POINTER = "ffffff"     # knob pointer line and value arc

WELL = "220023"        # inside of the filter display, meter bars, slider wells, unselected options
UNLIT = "4e1a52"       # unlit meter segments
GRID = "5a2360"        # filter display grid lines, knob track ring
ZERO = "8a4a90"        # the filter display's 0 dB line, the "bypassed" flat line
CLIP = "ff0000"        # a meter's border at full scale (the top frame), a clip light (2026-11-09 skin edit)


def rgb(hexstr):
    return tuple(int(hexstr[i:i + 2], 16) for i in (0, 2, 4))
