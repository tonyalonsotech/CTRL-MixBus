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
  output meters beside them, on INPUT and FILTER (left, so MPC's Q-Link overlay on the right never hides them). OUTPUT is GATE |
  maximizer meters | MAXIMIZER, with no side meters (the middle ones show input and output); the maximizer stays on Q-Link bank B
  although it is the third column. Column titles are centred (a frame without a title plus a centred text line, since the
  renderer draws frame titles left-aligned).
- **Switches are sliders.** Every on/off switch is a short vertical slider (up = on), dragged by touch, not a tap
  toggle (1.0.4; since 2026-11-10 it is touchscreen only, no longer on a Q-Link, and looks as before). The bottom switch of
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
  frames: 20 LED segments of 3 dB (60 dB range). The maximizer's input meter reads after the gate and before the maximizer. Each page's IN meter reads where that page starts (1.0.7, from the user's tests): INPUT after the gain knob and before the phase invert (`m_in_*`), FILTER after balance and its on/off and before the DJ EQ (`m_fin_*`, appended parameters). Its
  reduction meter shows the drive: 1 dB segments from 2 dB before limiting to 18 dB into it (the limiter's input against the
  ceiling, or the reduction still applied), each peak held 0.3 s so the 100 ms screen poll sees it; the number under it is the
  exact reduction (1.0.5).
- **dB numbers.** Under each meter pair, the louder channel's peak, held 1 s, then the loudest peak of the second gone. Plain text
  would only redraw on a full UpdateDisplay, which closes MPC's PRESET popup under live meters, so the number is two filmstrips side
  by side (whole dB with sign, then tenth + "dB"; `src/dsp/dj_readout.h`), 0.1 dB resolution, -60..+24.9 dB, "-inf" below.
- **Gate.** No bypass switch. At the minimum threshold (-80 dB, the default) the gate is switched off through the crossfade and
  costs nothing; the display reads "Off". Any higher threshold gates.
- **Maximizer.** Replaces the output level stage. Threshold raises the signal into a fixed 0 dBFS ceiling (no ceiling knob, no knee);
  Attack/Release shape the gain; Output is a trim below the ceiling (-inf..0 dB), so the ceiling can only be lowered. Zero latency by
  design: the wrapper does not report latency to the MPC, so a lookahead limiter would shift this channel against the rest of the
  project. Instead the gain computer reacts on the same sample and a hard clip guarantees 0 dBFS; a slower attack therefore lets
  transients through the limiter and into the clip (a harder sound). The two channels share one gain.
- **Presets.** The MPC's own system, with a key-based state string and an Init program. See `PRESETS.md`.
