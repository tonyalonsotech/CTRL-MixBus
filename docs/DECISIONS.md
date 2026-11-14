# Design decisions

- **Engine/UI split.** DSP modules (`src/dsp`) are independent of the wrapper and each tested alone. The engine
  (`src/engine/dj_engine.c`) is the only code that knows the MPC wrapper ABI.
- **Real-time rules.** No allocation, file access or blocking in `create`-after, `set_param`, `get_param` or `process`.
  Preset I/O runs on a worker thread; results are picked up by a non-blocking trylock.
- **Stable parameters.** One X-macro list (`dj_params.def`), append-only `params.lock`. Q-Link order follows the UI
  pages, with at most 16 controls per page; switches are touchscreen only, not on the Q-Links (2026-11-10).
- **No audible stepping.** Every continuous control and every bypass is smoothed. Bypass is a true DSP bypass
  (cross-faded in and out), not a mute. A filter slope change never mutes either: the new slope warms up unheard beside the old one
  for 10 ms, then crossfades in over 25 ms (1.0.3; 0.2.0 ducked for ~4 ms and restarted the filter, which clicked).
- **Switches show the signal path.** Each stage switch is an "on" switch (`*_on`, 1 = processing): lit while the stage works, grey
  while it is bypassed (1.0.3; until then `*_byp`, lit when bypassed). A switch or option changed from a Q-Link is reported back to
  the host (display_rev) so the screen follows the value the wrapper settled on, with or without audio.
- **Filters.** Slopes are cascades of Butterworth TPT state-variable sections (12/24/36/48/96 dB/oct = 1/2/3/4/8
  sections, "brick" = 16). The brickwall is a steep IIR rather than a linear-phase FIR, so it adds no latency.
  Cutoff is logarithmic; resonance up to 18 dB on the first section.
- **Stereo integrity.** Width uses mid/side with gain normalization. The gate detector is linked so both channels
  share one gain. Phase invert flips both channels together.
- **EQ.** EQ and ISOLATOR modes share one parameter set. The frequencies are provisional (see
  `EQ_VERIFICATION.md`) and live in one header.
- **Meters and curves.** The MPC skin can only show prerendered filmstrips, so the level meters and
  the filter response are filmstrips; the filter curves are measured from the real filter code at build time.
  The plugin reports a frame position and the skin picks the picture.
- **Pages and Q-Links.** Three pages (INPUT, FILTER, OUTPUT), each three columns of controls stacked vertically, because the Q-Links
  are a vertical column of four: one column = one Q-Link bank (A, B, C), top control = Q-Link 1. Switches are not on the Q-Links
  (their slots are "-", 2026-11-10); the EQ mode selector and the filter pictures head their column and are not on a Q-Link. Full-height input meters at the far left,
  output meters beside them, on INPUT and FILTER (left, so MPC's Q-Link overlay on the right never hides them). Since 2.1.1 (the
  user's layout) INPUT is STEREO | TRANSIENT | GATE and OUTPUT is COMPRESSOR | meters (IN, COMP, MAX, OUT) | MAXIMIZER, with no
  side meters on OUTPUT; the maximizer stays on Q-Link bank B although it is the third column. In STEREO, TRANSIENT and
  COMPRESSOR each row is a knob with its button to the right of it, level with the knob: the knobs are the Q-Link column (the ring
  encloses them alone), the buttons are touchscreen only. -3DB, -6DB, RMS and PEAK read their value inside the button and have no name under it (2.1.2). OUTPUT's middle panel is 396 px wide, 60 px wider than the others (2.1.2), so its four meter pairs sit 18 px apart and 18 px from its border (`art/page_output.png` is built from `art/src/page_output.png` by `tools/gen_page_art.py`). (Until 2.1.0 INPUT was GAIN + PHASE | WIDTH + BASS MONO | BALANCE, with
  the switches stacked between the knobs, and OUTPUT GATE | meters | MAXIMIZER.) Column titles are centred (a frame without a title plus a centred text line, since the
  renderer draws frame titles left-aligned).
- **Buttons select their Q-Link bank (2.1.5, the user's call).** Every control's tap runs MPC's "Q-Link" action, which banks the
  Q-Links to the bank holding a parameter (Akai's ui-reference 4.2). A knob's names itself; a button's named its own parameter,
  which no bank holds, so it did nothing. `tools/skin_post.py` (qlink_follow) gives each button in `gen_layout.QLINK_BUTTONS` a copy
  of its definition whose action names its column's first Q-Link control instead, so a tap on it brings that column to the Q-Links.
- **Switches are tap toggles (2.0.1).** Every on/off switch is touchscreen only, no longer on a Q-Link, and a tap turns it on or
  off. It keeps the look of the 1.0.4-2.0.0 short vertical slider (up = on): `art/switch_on.png` / `switch_off.png` are that
  slider's top and bottom frames (`tools/gen_switch_art.py`). From 1.0.4 to 2.0.0 it was a slider dragged or turned like a knob,
  not a tap toggle, for the Q-Link reasons below. The bottom switch of
  every INPUT and FILTER column sits at one height (`SW_BOTTOM_TOP` in `tools/gen_layout.py`). The DJ EQ knobs use the house
  centre knob (`art/knob_center_strip.png`: a dot at 12 o'clock, an arc growing left or right). Its parameter is a plain 0..1 control (`SW` in `dj_params.def`), on from 0.5 (Q-Link 64-127), kept exactly as
  the host sends it (1.0.5). A two-option parameter could not work: the wrapper snaps a Q-Link move to the next option, so the
  switch flipped fully on the first click while MPC kept the in-between value, and the Q-Link and the slider disagreed.
- **Filter slope on a Q-Link.** The Q-Link turns `lp_crv`/`hp_crv`, a plain 0..1 control that MPC keeps as sent; the engine
  picks the slope (round(x 5)) and reports it, so the 12..BRICK buttons follow (1.0.4). A Q-Link on the slope option itself
  drifted: the wrapper snaps each move to the next option while MPC keeps its in-between value, and a move that snaps to the
  option already set changes nothing, so nothing is reported. A tap on a button moves the control to that slope.
- **Colours** live in `tools/palette.py`; `tools/gen_layout.py` writes them as the layout's `theme_*` lines and `skin.css`, and every
  art generator imports them.
- **Meters.** MPC can only show prerendered filmstrips and draws strips up to 12288 px tall correctly, so a 540 px bar has 21
  frames: 20 LED segments of 3 dB (60 dB range). Each page's IN meter reads where that page starts (1.0.7, from the user's tests): INPUT after the gain knob and before the phase invert (`m_in_*`), FILTER after the gate and before the DJ EQ (`m_fin_*`, appended parameters; after balance until 2.1.0, when the gate was after the filters), OUTPUT after the filters and before the compressor (`mx_in_*`: the maximizer's input until 2.1.0). The maximizer's
  reduction meter shows the drive: 1 dB segments from 2 dB before limiting to 18 dB into it (the limiter's input against the
  ceiling, or the reduction still applied), each peak held 0.3 s so the 100 ms screen poll sees it; the number under it is the
  exact reduction (1.0.5).
- **dB numbers.** Under each meter pair, the louder channel's peak, held 1 s, then the loudest peak of the second gone. Plain text
  would only redraw on a full UpdateDisplay, which closes MPC's PRESET popup under live meters, so the number is two filmstrips side
  by side (whole dB with sign, then tenth + "dB"; `src/dsp/dj_readout.h`), 0.1 dB resolution, -60..+24.9 dB, "-inf" below.
- **Chain (2.1.1).** GAIN -> PHASE INVERT -> WIDTH -> BASS MONO -> BALANCE -> TRANSIENT -> GATE -> DJ EQ -> LOW-PASS -> HIGH-PASS ->
  COMPRESSOR -> MAXIMIZER. GAIN is always on and PHASE INVERT works alone: `in_on` and `ph_on` stay in the parameter list (saved
  projects and Q-Links bind by index) but do nothing and are not on screen. A project saved with GAIN switched off now has its GAIN
  applied.
- **Transient shaper (2.1.1).** `src/dsp/dj_transient.h`. Linked-stereo envelope followers: ATTACK scales the gap between a fast-
  and a slow-attack follower (the hit), SUSTAIN the gap between a slow- and a fast-release follower (the tail), PUMP is a dip
  that opens as the hit's gap falls (a peak follower of it less the gap), SPEED scales every follower's time 4 x either way.
  AUTO LEVEL (with CLIP off) pulls an output over 0 dBFS back to 0 dBFS (the user's example: +4 dB out is lowered 4 dB); CLIP is a
  soft clip to 0 dBFS and then -3 dB; -3 dB / -6 dB come after both. No lookahead (zero latency, as the maximizer). It sits before
  the DJ EQ, whose crossovers turn the phase: a clipped wave's peaks can come out a few dB higher after the EQ, which the
  compressor and maximizer then see.
- **Compressor (2.1.1).** `src/dsp/dj_comp.h`: CTRL Giant's model (6 dB soft knee, the reduction in dB following ATTACK and
  RELEASE, THRESHOLD and RATIO doubly smoothed), linked stereo, no makeup gain (the maximizer follows). PEAK is CTRL Giant's own
  detector; RMS the mean square over 10 ms. Exactly one of RMS and PEAK is on: a tap on one turns the other off, and tapping the lit
  one off turns the other on (the engine sets the other and reports it). CLIP: at its input, after the DJ EQ and filters (2.1.2; after it in 2.1.1), for when it is driven over 0 dB: peaks over -0.1 dBFS clipped, then -1 dB into the compressor (2.1.4, the user's call: the stage before keeps its headroom) (no -3 dB, the user's call). Init: off, THRESHOLD 0 dB, RATIO 1:1 (the user's call), so switching it on changes nothing until it is set.
- **Gate.** An ON/OFF switch since 2.1.2 (`gt_on`, on by default so older projects keep their gate). Besides it, at the minimum threshold (-80 dB, the default) the gate is switched off through the crossfade and
  costs nothing; the display reads "Off". Any higher threshold gates. FLIP (2.1.3) inverts it: muted above the threshold, passing below; attack and release follow the threshold crossing (attack after going over, release after falling under), and a flipped gate starts passing. A flipped gate turns the gain while the sound is loud, so (2.1.4) its gain also passes two 1.5 ms one-pole stages: an S-curve, no click at any attack (a normal gate's opening is masked by the hit and stays as it was).
- **Maximizer.** An ON/OFF switch since 2.1.3 (`mx_on`, on by default): off bypasses the threshold gain, limiter and ceiling clip with a 6 ms fade; the Output trim applies either way. Replaces the output level stage. Threshold raises the signal into a fixed 0 dBFS ceiling (no ceiling knob, no knee);
  Attack/Release shape the gain; Output is a trim below the ceiling (-inf..0 dB), so the ceiling can only be lowered. Zero latency by
  design: the wrapper does not report latency to the MPC, so a lookahead limiter would shift this channel against the rest of the
  project. Instead the gain computer reacts on the same sample and a hard clip guarantees 0 dBFS; a slower attack therefore lets
  transients through the limiter and into the clip (a harder sound). The two channels share one gain.
- **Presets.** The MPC's own system, with a key-based state string and an Init program. See `PRESETS.md`.
