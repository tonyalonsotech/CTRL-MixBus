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

# buttons (the user's call, 2026-11-12, from CTRL Giant 1.2.2-1.2.3): the ON/OFF switches are rounded SQUARE buttons reading ON, and
# every other button (option buttons, trigger buttons) has the same look: rounded (BTN_RADIUS, the corners of CTRL Giant's RESET PEAK),
# off a dimmed fill with slightly darker letters, on (selected) a brighter fill with white letters, no gradient, no shadow
BTN_ON = "8a52e8"      # = KNOB: on, selected, and a trigger button at rest
BTN_ON_TEXT = "ffffff"
BTN_OFF = "58217e"     # off / not selected: KNOB dimmed 60 % toward BG
BTN_OFF_TEXT = "c9b6d0"
BTN_PRESS = "a47af0"   # a trigger button while pressed: KNOB a little brighter
BTN_RADIUS = 8
OPTION = "2a002b"      # a menu option's fill (and a closed menu's box): a little darker than BG, the open menu panel's fill


def rgb(hexstr):
    return tuple(int(hexstr[i:i + 2], 16) for i in (0, 2, 4))
