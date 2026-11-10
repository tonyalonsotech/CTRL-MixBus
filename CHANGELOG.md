# Changelog

## 2.0.1

- **On/off switches are touchscreen only.** They are no longer on the Q-Links (their Q-Link slots are empty); they look as before.
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
