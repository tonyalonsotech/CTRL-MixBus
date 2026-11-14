# Changelog

## 2.2.0 (Release, 2026-11-13): This is the release version including all changes made in beta versions 2.1.1, 2.1.2, 2.1.3, 2.1.4, and 2.1.5.

- **INPUT page:** STEREO (GAIN, WIDTH, BASS FREQ, BALANCE, each with its button), the new TRANSIENT shaper (ATTACK, PUMP, SUSTAIN, SPEED, with CLIP, -3DB, -6DB, ON/OFF) and GATE (moved from OUTPUT, now before the filters, with FLIP and ON/OFF).
- **OUTPUT page:** the new COMPRESSOR (THRESHOLD, ATTACK, RELEASE, RATIO, with CLIP, RMS, PEAK, ON/OFF), IN / COMP / MAX / OUT meters, and the MAXIMIZER with an ON/OFF.
- **Q-Links follow the buttons:** tapping a button switches the Q-Links to its column.
- **CPU:** benched on an MPC One (Gen2), MPC OS 3.10.1: worst p99 3.1 % and worst block 3.4 % of one audio block, PASS (docs/BENCH.md).
- **Tested on:** MPC One (Gen2), MPC Live III (Gen2) and Force (Gen1), MPC OS 3.10.1.
- **Project compatibility:** parameters have only been added. GAIN always applies, and PHASE INVERT works on its own (the old GAIN ON/OFF and PHASE ON/OFF switches are retired).

## 2.1.5 (Test Build, 2026-11-13): Buttons Select Their Q-Link Bank

**This is a test build, not an official release.** No changes to the sound.

- **Q-Links follow the buttons:** Tapping a button now switches the Q-Links to that button's column, as touching a knob does. For example, tapping ON/OFF in TRANSIENT puts ATTACK, PUMP, SUSTAIN and SPEED on the Q-Links. This covers every button on the INPUT, FILTER and OUTPUT pages, including the filter slope buttons and EQ / ISOLATOR. The button still does what it did.
- **Project compatibility:** No parameters added or changed.

## 2.1.4 (Test Build, 2026-11-13): Gate FLIP Clicks Fixed, Compressor CLIP Level

**This is a test build, not an official release.**

- **GATE FLIP:** Fixed the slight clicking in FLIP mode (worst at the shortest ATTACK). In FLIP mode the gate turns the level down while the sound is loud, so each duck and return now starts and ends smoothly (an S-curve of about 3 ms). The normal gate is unchanged.
- **Compressor CLIP:** Now clips at -0.1 dB, so only peaks right at full scale are clipped, and then lowers the level going into the compressor by 1 dB, keeping headroom in the stage before the compressor.
- **Project compatibility:** No parameters added or changed.

## 2.1.3 (Test Build, 2026-11-13): Gate FLIP and Maximizer ON/OFF

**This is a test build, not an official release.**

- **GATE FLIP:** New FLIP button to the right of HOLD (it reads FLIP, with no label below). FLIP turns the gate around: audio above the threshold is muted (ducked) and audio below it passes. ATTACK sets how quickly it ducks when the signal goes over the threshold, RELEASE how quickly the sound returns (after HOLD) once it falls back under. Off at Init.
- **MAXIMIZER ON/OFF:** New ON/OFF button to the right of OUTPUT. Off bypasses the maximizer (its threshold, limiting and 0 dB ceiling); the OUTPUT knob still works either way. On by default, so existing projects keep their maximizer.
- **Project compatibility:** Two parameters added (Gate Flip, Max On). Existing projects are unaffected.

## 2.1.2 (Test Build, 2026-11-13): Gate ON/OFF, Value Buttons, OUTPUT Meter Spacing, Compressor CLIP Position

**This is a test build, not an official release.**

- **GATE:** New ON/OFF button to the right of RELEASE (on by default, so existing projects keep their gate). The gate knobs now line up with the knobs in the other columns.
- **Value buttons:** The -3DB, -6DB, RMS and PEAK buttons now show their value inside the button instead of ON, with no label underneath. Same size as before.
- **OUTPUT page meters:** Each meter pair is now 18 pixels from the next, and the outer pairs 18 pixels from the panel border. The middle panel is wider to fit them; the COMPRESSOR and MAXIMIZER panels keep their size and move outward.
- **Compressor CLIP:** Now before the compressor (after the DJ EQ and filters) instead of after it.
- **Project compatibility:** One parameter added (Gate On). Existing projects are unaffected.

## 2.1.1 (Test Build, 2026-11-13): Transient Shaper, Compressor, and Re-arranged INPUT and OUTPUT Pages

**This is a test build, not an official release.**

- **INPUT page:** Now STEREO | TRANSIENT | GATE. Each control in STEREO and TRANSIENT has its button to the right of it.
  - **STEREO** (replaces GAIN + PHASE, WIDTH + BASS MONO and BALANCE): GAIN with PHASE INVERT; WIDTH with WIDTH ON/OFF; BASS FREQ with BASS MONO ON/OFF; BALANCE with BALANCE ON/OFF.
  - GAIN is always on (GAIN ON/OFF removed). PHASE ON/OFF removed: PHASE INVERT does the job on its own.
  - **TRANSIENT** (new): ATTACK with CLIP; PUMP with -3 DB; SUSTAIN with -6 DB; SPEED with ON/OFF.
    - **ATTACK** (-100 % to +100 %): raises or softens the first moments of each hit.
    - **PUMP** (0 to 100 %): dips the level right after each hit, for punch without a higher peak.
    - **SUSTAIN** (-100 % to +100 %): brings the tail of each sound up or cuts it short.
    - **SPEED:** how quickly the shaper reacts; higher is shorter and snappier, lower is smoother and wider.
    - **CLIP:** a soft clipper at 0 dB at the end of the shaper, then the level is lowered 3 dB.
    - **-3 DB / -6 DB:** lower the shaper's output after the clipper (both together: -9 dB).
    - **ON/OFF:** turns the shaper on and off (off at Init). While CLIP is off, automatic level attenuation lowers any output that goes over 0 dB back to 0 dB.
  - **GATE:** moved here from the OUTPUT page. In the sound it now comes after the transient shaper and before the DJ EQ and filters (it used to come after them).
  - **Q-Links:** 1-4 GAIN, WIDTH, BASS FREQ, BALANCE; 5-8 ATTACK, PUMP, SUSTAIN, SPEED; 9-12 the gate.
- **OUTPUT page:** Now COMPRESSOR | meters | MAXIMIZER.
  - **COMPRESSOR** (new, the same compressor model as CTRL Giant): THRESHOLD with CLIP; ATTACK with RMS; RELEASE with PEAK; RATIO with ON/OFF (off at Init; THRESHOLD 0.0 dB and RATIO 1.0:1 at Init, so turning it on changes nothing until it is set).
    - **PEAK:** reacts to every peak at once: tight and precise.
    - **RMS:** reacts to the average level: smooth, short spikes pass.
    - Only one of RMS and PEAK can be on: tapping one turns the other off.
    - **CLIP:** a soft clipper at 0 dB after the compressor.
  - **Meters:** IN (after the filters, before the compressor), COMP (compressor reduction), MAX (maximizer reduction), OUT.
  - **Q-Links:** 1-4 the compressor, 5-8 the maximizer.
- **FILTER page:** Its input meter now reads after the gate (before the DJ EQ).
- **Project compatibility:** Parameters have only been added. Existing projects load, with two differences: GAIN always applies (a project saved with GAIN switched off now hears its GAIN setting), and PHASE INVERT applies even where PHASE ON/OFF was off.

## 2.1.0 (Release, 2026-11-13): This is the release version including all changes made in beta versions 2.0.2, 2.0.3, and 2.0.4.

**Release:** This version is released both via Open MPC Plugin Manager and available for download from https://tonyalonso.tech/plugins.html

## 2.0.4 (Test Build, 2026-11-13): FILTER Page Layout Updated to Match CTRL Duck

**This is a test build, not an official release.** The sound and audio processing remain unchanged from version 2.0.3.

- **Redesigned FILTER page:** Rearranges the three processing sections to match the FILTER page layout used in CTRL Duck:
  - **LOW-PASS:** Moved to the left side of the page.
  - **HIGH-PASS:** Positioned in the centre.
  - **DJ EQ:** Moved to the right side of the page.
  - Previously, the order was DJ EQ, LOW-PASS, and HIGH-PASS.

- **Updated Q-Link assignments:** Q-Links are now grouped by their corresponding screen sections:
  - **Q-Links 1-4:** LOW-PASS controls (CURVE, CUTOFF, RESONANCE).
  - **Q-Links 5-8:** HIGH-PASS controls.
  - **Q-Links 9-12:** DJ EQ controls (HIGH, MID, LOW).

- **Project compatibility:** No parameters have been moved or changed. Existing saved projects are unaffected.

## 2.0.3 (Test Build, 2026-11-13): Q-Link Rings, FILTER Page Layout, and MPC OS 3.10.1

**This is a test build, not an official release.** No changes have been made to the sound or audio processing.

- **Q-Link rings:** Introduces the Q-Link ring design used in CTRL Giant. MPC now highlights the column of controls assigned to the Q-Links, making it easy to see which knobs each Q-Link controls before touching them. Each ring surrounds the assigned knobs, including their names and value readouts, as well as any switch positioned between them. The rings stop before reaching unrelated controls. MPC draws these rings using its own visual style, including when menus are open.

- **Redesigned FILTER page:** Updates the layout to match the intended screen arrangement:
  - **Filter slope buttons:** Arranged in a 3 x 2 grid (12, 24, 36 / 48, 96, BRICK), positioned directly below each filter's curve display and matching its width.
  - **CUTOFF and RESONANCE:** CUTOFF is aligned with the DJ EQ's MID knob, while RESONANCE is aligned with its LOW knob.
  - **EQ / ISOLATOR buttons:** Expanded to match the width of the filter slope grids.
  - **Q-Link assignments:** Now follow the top-to-bottom order of the controls on screen: CURVE (slope), CUTOFF, and RESONANCE. Previously, the order was CUTOFF, RESONANCE, and CURVE.
  - No parameters have been moved or changed, so existing saved projects are unaffected.

- **MPC OS 3.10.1 compatibility:** Adds support for Akai's MPC OS 3.10.1 release (2026-11-12), which introduces VST2 plugin loading from the `<drive>/VST2 Plugins/` directory.
  - The plugin folder now includes the required `plugin.json` file with `"format": "vst2"`, as specified by Akai.
  - Each release now includes additional folder-only ZIP packages: `CTRL-MixBus-<version>-Gen2.zip` and `CTRL-MixBus-<version>-Gen1.zip`. These can be extracted directly into the MPC's `VST2 Plugins` directory.
  - To use this installation method, enable **Enable Enhancements** and **External Plugin Support** under Preferences > Enhancements.
  - The existing Plugin Manager and `install.sh` installation packages continue to work as before.

- **Audio-thread performance improvement:** Screen updates now run outside the audio thread using `DISPLAY_REV_THREAD`. The plugin wrapper reports meter and readout changes from its own thread instead of the audio thread. Previously, reporting these updates from the audio thread caused MPC's audio processing to stall for approximately 20 ms every few seconds. This issue was observed and measured on an MPC Live III.

- **Build update:** Built against `mpc-vst-plugins` commit `20f91bf` (2026-11-13), updated from `5432c83`.

## 2.0.2 (test build, 2026-11-12)

- **New UTILITY tab:** Adds a dedicated page for monitoring DSP performance, signal levels, and plugin information. The page includes:
  - **IN/OUT Meters:** Input and output level meters positioned on the far left.
  - **DSP:** Displays DSP LOAD, DSP PEAK, OVERLOADS, and BUILD, along with a **RESET PEAK** button to reset the DSP PEAK reading.
  - **SIGNAL:** Displays INPUT PEAK (measured before GAIN), OUTPUT PEAK, CLIPS, and MAX REDUCTION (the maximum gain reduction recorded since the last reset). A **RESET PEAKS** button resets the signal peak measurements and maximum reduction.
  - **ABOUT:** Displays the CTRL logo, CTRL MixBus name, creator credit (TonyAlonso.Tech), and version number, matching CTRL Giant's format.
  - All UTILITY parameters are appended to the end of the parameter list and are not saved with presets.

- **Updated switch and button design:** Stage bypass switches now use rounded-square buttons labeled **ON**, matching the style used in CTRL Giant and CTRL Duck. Buttons appear bright when a stage is active and dim when bypassed. Switch labels now use the `... ON/OFF` naming convention instead of `... BYPASS`. Option buttons (EQ modes and filter slopes) and both RESET buttons also use this updated, flat, rounded-square design.

- **Simplified on-screen control names:** Knobs and switches now display shorter names describing their function (for example, `GAIN`). These labels are drawn directly by each page, using consistent positioning, font, and sizing. Full parameter names remain unchanged in MPC's Q-Link and automation lists, as well as on the OLED displays.

## 2.0.1

- **On/off switches are tap to engage and touchscreen only.** A tap turns a switch on or off (it was a slide). They are no longer on the Q-Links (their Q-Link slots are empty), and they look as before.
- **Bottom switches line up.** The switch at the bottom of each INPUT and FILTER column now sits at the same height on both pages.
- **DJ EQ knobs** use the house centre knob: a dot at 12 o'clock (0 dB), and an arc that grows left (cut) or right (boost).

## 2.0.0 (first release)

1.0.7 as tested on an MPC One Gen 2 (filters, EQ and metering checked in Ableton with pink noise), released for Gen 2
(aarch64) and Gen 1 (armv7). The Plugin Manager description is in ABOUT.txt (manifest "about"). No code changes from 1.0.7.

## 1.0.7 (development)

From device testing of 1.0.6 (filters, EQ and level metering checked in Ableton with pink noise):

- **INPUT page IN meter** changed to read audio level after the Input Gain knob.
- **FILTER page IN meter** changed to read the audio level after the Balance Knob.

## 1.0.6 (development)

Completed skin development.

- **Page backgrounds** and titles changed to a higher contrast theme for visibility.
- **Knobs** changed from a flat purple to a textured purple to match the MPC style knobs more closely.
- **Meters:** now have a red border when clipping.

## 1.0.5 (development)

Changes from device testing of 1.0.4 (MPC One Gen 2 and MPC Live III):

- **On/off switches move with knobs.** Each switch now moves up or down either by touch or with Q-Link knob to alleviate previous issue where Q-Link adjustments did not reflect in the UI.
- **Reduction meter** on Maximizer now starts at 2DB over zero to help avoid clipping when reducing the threshold.

## 1.0.4 (development, ran on an MPC One Gen 2 and MPC Live III)

Changes from device testing of 1.0.3 (MPC One Gen 2 and MPC Live III):

- **Filter curve on a Q-Link follows on screen.** Fixed an issue where filter curve in UI did not match adjustments via Q-Links.
- **INPUT and FILTER Meters** are now both on the far left side of the UI to allow room for the Q-Link overlay.
- **The GATE page is now called OUTPUT.** Changed Gate page to Output to accurately reflect this portion of the chain.
- **Colours:** background #370038, text #feb9ff, borders #ffffff, knobs #8a52e8, knob pointer and value arc #ffffff.

## 1.0.3 (development, ran on an MPC One Gen 2 and MPC Live III)

Changes from device testing of 0.2.0 (MPC One Gen 2 and MPC Live III):

- **Bypass switches behavior** changed to be on/off switches to follow a more logical flow. 
- **Filter curve change audible click** removed.
- **GATE page meters** rearranged. Input, Reduction, and Output meters are not in the center of the UI.
- **dB numbers** added under each level meter. The peak is held for 1 second, giving a visual representation of max levels.

## 0.2.0 (development)

Changes from device testing of 0.1.0:

- **Name:** the plugin is now **CTRL MixBus**
- **Layout:** changed to reflect Q-Link positions. Knobs and Switches are now stacked vertically. Devices are arranged into three pages, following the flow of the audio chain.
- **Filter pictures** created to have a visualization of the filter parameter changes.
- **Gate** bypass switch removed, due to the knob creating redundancy.
- **Maximizer** integrated.

## 0.1.0 (not released)

First device build: DSP chain, simple UI, and metering.
