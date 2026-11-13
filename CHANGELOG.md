# Changelog

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
