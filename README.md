# CTRL Mixbus

### tereo Mixing and Processing Plugin

**Project:** CTRL Mixbus\
**Developer:** Tony Alonso Tech\
**Plugin type:** MPC Compatible VST audio effect\
**Primary platform:** Akai MPC standalone\

------------------------------------------------------------------------

<img width="1280" height="628" alt="preview" src="https://github.com/user-attachments/assets/000df992-16a9-40ab-8e7a-4c6bf756c8eb" />


------------------------------------------------------------------------

## Description

CTRL Mixbus is a professional stereo mixing and processing effect designed to streamline your workflow across three focused pages. Take control of input gain, stereo width, transient shaping, equalization, precision filtering, dynamic gating, compression, and loudness maximization.

Featuring a versatile three-band DJ EQ with selectable EQ and Isolator modes, adjustable low-pass and high-pass filters with resonance and curve shaping, bass width control, stereo manipulation, phase inversion, and independent processing stages, CTRL Mixbus provides the tools needed for detailed mix shaping and signal control. Fine-tune your stereo image with width, bass width, and balance adjustments, shape the attack and sustain of every hit with the transient shaper, then refine your output with configurable gating, compression, and maximization for precise dynamic control and enhanced loudness.

Dedicated input and output level meters, compressor and maximizer gain reduction monitoring, and visual filter displays provide real-time feedback throughout your signal chain. Integrated clipping indicators flash red to warn of excessive signal levels, helping you maintain control over your mix.

Built natively for MPC, CTRL Mixbus brings essential mixing, stereo processing, tonal shaping, and dynamic control together in a streamlined interface designed for efficient workflow and hands-on operation, without relying on external hardware or software.

------------------------------------------------------------------------

**Versions:**

V1.0.0 - Initial development build with input and output processing stages.

V1.0.1 - Completed the initial implementation of all control pages and verified device functionality and performance on MPC One G2.

V1.0.2 - Reorganized the interface to align with Q-Link mapping and the audio signal chain. Added filter visualizations and integrated the Maximizer.

V1.0.3 - Improved filter response visualization, corrected an audible click when adjusting filter settings, and refined metering and bypass behavior.

V1.0.4 - Updated the interface to match the Tony Alonso Tech color scheme and improved Q-Link filter synchronization.

V1.0.5 - Improved on/off switch synchronization with Q-Link adjustments and refined Maximizer gain reduction metering.

V1.0.6 - Completed skin development with improved contrast, updated knob textures, and clip indicators.

V1.0.7 - Corrected input metering to accurately reflect signal levels at the Input Gain and Balance stages.

V2.0.0 - First production release. Includes the finalized V1.0.7 build, packaged for MPC Gen 1 and Gen 2 devices. No additional plugin code changes.

V2.0.1 - On/off switches are now tap to engage and touchscreen only (removed from the Q-Links). The bottom switches on the INPUT and FILTER pages line up at the same height. The DJ EQ knobs show a center-origin ring.

V2.0.2 (beta) - A new UTILITY page has now been added. This included DSP, Signal, and About information. Switches have been changed to rounded ON buttons labelled ON/OFF. Some control labels have been modified for length.

V2.0.3 (beta) - Q-Link rings show which controls the Q-Links are driving. On the FILTER page, each filter's slope is a 3 x 2 button grid under its curve display. CUTOFF and RESONANCE line up with the DJ EQ's MID and LOW knobs. The filters' Q-Links run CURVE, CUTOFF, RESONANCE top to bottom. Updated for MPC 3.10.1 compatibility.

V2.0.4 (beta) - FILTER page has been re-arranged to follow the same flow as CTRL Duck. This allows the Q-Link overlay to populate on screen without covering the Filter Curve Displays. Q-Link mappings have been adjusted to match the new layout.

V2.1.0 - This is the release version including all changes made in beta versions 2.0.2, 2.0.3, and 2.0.4.

V2.1.1 (beta) - The INPUT page is now STEREO, TRANSIENT and GATE: a new transient shaper (ATTACK, PUMP, SUSTAIN, SPEED, with CLIP, -3 DB and -6 DB) and the gate moved from the OUTPUT page, now before the filters. GAIN is always on; PHASE ON/OFF removed. The OUTPUT page now has a compressor (THRESHOLD, ATTACK, RELEASE, RATIO, with CLIP, RMS and PEAK) and COMP and MAX reduction meters.

V2.1.2 (beta) - GATE now has an ON/OFF button. The -3DB, -6DB, RMS and PEAK buttons show their value inside the button. The OUTPUT page meters are evenly spaced (18 pixels apart). The compressor's CLIP now comes before the compressor.

V2.1.3 (beta) - GATE has a FLIP button that inverts the gate (mutes above the threshold, passes below). The MAXIMIZER has an ON/OFF button; its OUTPUT knob works either way.

V2.1.4 (beta) - Fixed the clicking on GATE in FLIP mode. The compressor's CLIP now clips at -0.1 dB, then lowers the level into the compressor by 1 dB.

V2.1.5 (beta) - Tapping a button now switches the Q-Links to that button's column, as touching a knob does.

V2.2.0 - This is the release version including all changes made in beta versions 2.1.1, 2.1.2, 2.1.3, 2.1.4, and 2.1.5.

------------------------------------------------------------------------


