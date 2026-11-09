# Changelog

## 2.0.0 (first release)

1.0.7 as tested on an MPC One Gen 2 (filters, EQ and metering checked in Ableton with pink noise), released for Gen 2
(aarch64) and Gen 1 (armv7). The Plugin Manager description is in ABOUT.txt (manifest "about"). No code changes from 1.0.7.

## 1.0.7 (development)

From device testing of 1.0.6 (filters, EQ and level metering checked in Ableton with pink noise):

- **INPUT page IN meter** reads after the gain knob and before the phase invert (was the plugin's raw input), and so does its
  dB number.
- **FILTER page IN meter** is its own meter now: after balance (and its on/off) and before the DJ EQ, with its own dB number.
  Four display-only parameters appended (`m_fin_l`, `m_fin_r`, `fin_db_i`, `fin_db_t`; 54 parameters), so saved projects and
  automation keep their mapping.

## 1.0.6 (development)

The user's own skin art (2026-11-09):

- **Page backgrounds** are the user's pictures (`art/page_input.png`, `page_filter.png`, `page_output.png`): lighter magenta page,
  dark panels with a shadow, thin borders, magenta titles. They carry the panels, titles and the OUTPUT page's IN / REDUCTION /
  OUT labels, so the layout no longer draws its own (`tools/gen_layout.py` `PAGE_ART`). The "IN" / "OUT" labels over the
  left meters on INPUT and FILTER (not in those pictures) are drawn by the layout, in the pictures' label magenta (#ff00ff).
- **Knobs** are the user's filmstrips (`art/knob_strip.png`, and `art/knob_plain_strip.png` for Balance, without the value
  ring): darker face, bevelled edge. Knob radius 35 (was 34) so the 78 px frames are drawn at their own size.
  `tools/gen_knob_plain.py` and `art/knob_plain.png` are removed.
- **Meters:** at full scale (all segments lit) a 3 px red border lights around the meter, a clip light
  (`tools/gen_meter_strip.py`, `palette.CLIP`).

## 1.0.5 (development)

Changes from device testing of 1.0.4 (MPC One Gen 2):

- **On/off switches turn like knobs.** Each switch is now a plain 0..1 control kept exactly where the Q-Link or the slider puts
  it: the bottom half (Q-Link 0-63) is off, the top half (64-127) on. The Q-Link sweeps the slider all the way, matching a drag
  on screen; in 1.0.4 the plugin snapped a switch to fully off or on at the first click of the encoder, so the Q-Link and the
  slider disagreed. Saved 1.0.4 projects and presets load with their switches as they were (state format `mpcdj:2`; `mpcdj:1`
  is still read).
- **Reduction meter** now lights 2 dB BEFORE limiting starts: it shows how hard the maximizer is driven (the level going into
  its limiter against the 0 dB ceiling, or the reduction still applied, whichever is more), from -2 dB to 18 dB in. In 1.0.4 it
  started at 5 dB of reduction and fell back faster than the screen redraws, so it looked dark until about 7 dB. Each peak is
  now held 0.3 s. The number under it still reads the exact reduction.

## 1.0.4 (development, ran on an MPC One Gen 2)

Changes from device testing of 1.0.3 (MPC One Gen 2):

- **Switches are short vertical sliders** (up = on), moved by a drag or a Q-Link like a knob, instead of tap toggles: a test of
  whether the tap toggle is what kept the screen from following a Q-Link. The value ("On"/"Off") shows under each.
- **Filter slope on a Q-Link follows on screen.** The Q-Link now turns a new control per filter (`lp_crv`, `hp_crv`, "LP/HP
  Curve") that the MPC keeps exactly as turned; the plugin picks the slope from it (a sixth of the turn each: 12, 24, 36, 48, 96,
  BRICK) and the 12..BRICK buttons follow. Tapping a button still selects a slope, and the Q-Link carries on from there. The
  slope option itself is now named "LP Slope" / "HP Slope". In 1.0.3 a fast Q-Link turn could leave the buttons showing another
  slope than the one in use (the wrapper snapped each Q-Link move to the next slope while MPC kept the in-between value).
- **Reduction meter** starts at 5 dB of reduction (5..25 dB; 1.0.3: 3 dB), so it lights just before the sound turns hard.
- **INPUT and FILTER:** the IN and OUT meters stand side by side at the far left, so both stay visible under MPC's Q-Link overlay.
- **The GATE page is now called OUTPUT.** Column titles are centred over the controls.
- **Colours:** background #370038, text #feb9ff, borders #ffffff, knobs #8a52e8, knob pointer and value arc #ffffff
  (tools/palette.py; meter wells, unlit segments and the filter display use darker shades of the background).

## 1.0.3 (development, ran on an MPC One Gen 2)

Changes from device testing of 0.2.0 (MPC One Gen 2):

- **Bypass switches show the signal path:** lit while the stage processes, grey while it is bypassed. The parameters are now
  "on" switches: `in_on`, `ph_on`, `wd_on`, `bm_on`, `bal_on`, `eq_on`, `lp_on`, `hp_on` (were `*_byp`, the other way round).
  Saved 0.2.0 projects and presets load with every stage switch at Init.
- **Q-Link and the screen:** a switch or option changed by a Q-Link now always shows on screen, with or without audio running (the
  plugin reports the value it settled on). Fixes the bypass switch not following and the filter curve number jumping.
- **Filter curve change click** removed: the new slope warms up unheard and crossfades in over 25 ms instead of a 4 ms mute and
  restart.
- **Filter pictures:** 5 resonance steps (was 3): 201 frames per curve; skin memory about 119 MB (was 71 MB).
- **GATE page:** GATE | maximizer meters | MAXIMIZER; no "MAXIMIZER METERS" title; no far left/right meters on this page.
- **Maximizer input meter** reads after the gate and before the maximizer (it showed the boosted level).
- **Reduction meter** starts at 3 dB of reduction (3..23 dB).
- **dB numbers** under every meter pair: the peak, held 1 second (e.g. "2.0 dB", "-inf dB"); the reduction reads negative.
- Full-height meters are 540 px (were 560) to make room for the numbers.

## 0.2.0 (development)

Changes from device testing of 0.1.0:

- **Name:** the plugin is now **CTRL MixBus** on the MPC and in Plugin Manager G2 (was DJ Utility Strip). Vendor `CTRL`, file
  `ctrl_mixbus.so`, folder and skin `CTRL - VST - CTRL MixBus`, catalog id `ctrl-mixbus`. The UID (`DjUS`) is unchanged. On a device
  that had 0.1.0, remove the old `djutil - VST - DJ Utility Strip` folder and its plugin-list entry.
- **Layout:** three pages (INPUT, FILTER, GATE); controls stacked vertically so they follow the Q-Link banks (A, B, C); full-height
  input meters at the far left and output meters at the far right; the DJ EQ moved onto the FILTER page; the Balance knob has no value ring.
- **Filter pictures:** 121 frames per curve (was 61): the cutoff moves in quarter-octave steps instead of half-octave. Skin memory
  rises from about 41 MB to about 71 MB; see docs/BUILDING.md if the device struggles.
- **Gate:** no bypass switch; threshold default -80 dB, where the gate is off.
- **Maximizer** replaces the output level: threshold, attack, release, output; fixed 0 dBFS ceiling, no knee, zero latency; stereo
  meters for input, gain reduction and output.
- **Presets:** the plugin's own preset page and files are removed; the MPC's built-in plugin presets are used. The plugin now serves a
  key-based state string so MPC presets and projects restore reliably (docs/PRESETS.md), and the Init program sets the defaults.
- **Parameters renumbered** (removed gt_byp, out_byp, out_gain and the preset parameters; added the maximizer's). Projects or
  automation made with 0.1.0 will not map to the right parameters; 0.1.0 was not released, so this is done once, now.
- Fixed: the Init program would have set both filter curves to BRICK (its labels "12"... were read as out-of-range indices).
- Meters are 21-step LED ladders (3 dB per step), not the 32-frame 0.1.0 meter.

## 0.1.0 (not released)

First device build: DSP chain, preset page with 16 file slots, meters, filter pictures.
