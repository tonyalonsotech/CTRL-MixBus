# DJ EQ / Isolator: verification status

**Status: UNVERIFIED.** The EQ frequencies in `src/dsp/dj_eq_config.h` are provisional. The plugin is a
DJ-style isolator EQ *inspired by* the DJM-900NXS2 workflow. It does not claim to match the hardware's crossover
points or curves.

## Evidence gathered (as of 2026-11-08)

| Claim | Source | Status |
|---|---|---|
| NXS2 EQ frequencies, boost/cut range, ISOLATOR behaviour | Pioneer DJ NXS2 specifications page | **Not published there.** The page only says the EQ was "improved". It names no frequencies, ranges or isolator mode. |
| HI 13 kHz, MID 1 kHz, LOW 70 Hz; -26..+6 dB | Forum post (djtechtools) citing "page 26 of the user manual" of the **original DJM-900** | **Unchecked** second-hand figure. It is for the older model, and it does not say whether the points are shelf or crossover frequencies. |
| Knob reads "infinite" to +6 dB | Same thread, original poster's observation of the front panel | Anecdotal. |
| Crossover slopes / filter order of the ISOLATOR | none found | **Unknown.** |

The NXS2 operating manual (the specifications page links to it) was not readable in this session, so it has not been
checked. It is the first thing to check.

## Provisional values in use

Isolator crossovers 250 Hz / 2.5 kHz; EQ-mode low shelf 250 Hz, mid bell 1 kHz (Q 0.5), high shelf 2.5 kHz; range
+6 / -26 dB (ISOLATOR to -inf). These are plausible DJ-mixer values chosen so the plugin is usable. They are not
sourced from the hardware.

## How to verify (needs a DJM-900NXS2 and an audio interface)

1. Set the mixer to EQ mode (or ISOLATOR for the isolator tests). Trim and faders at unity, FX off, master and
   booth gain moderate. Record from the master out at 44.1 or 48 kHz, 24 bit.
2. Play a log sweep or pink noise (or an impulse train) into one channel. Use identical settings for the reference
   take (all EQ knobs centred).
3. For each band, take three recordings: that knob at full cut, at full boost, and the other two at centre.
4. Divide each spectrum by the centred reference. The result is the band's magnitude response.
5. Read off: the -3 dB points and the frequency at the response mid-point (shelf corner), the slope in dB/octave,
   the maximum boost and cut (check against the published ranges), and in ISOLATOR mode the depth of the kill and
   the crossover points where adjacent bands cross at -6 dB or -3 dB.
6. Put the measured values in `src/dsp/dj_eq_config.h`, replace "PROVISIONAL" with the measurement date and method,
   add the raw numbers to this file, and note the change in `CHANGELOG.md`. Presets will keep loading but sound
   different, so say so.

A documentation-only route is also acceptable if a primary source (the NXS2 manual or Pioneer DJ) states the numbers.
Cite it in the table above and mark it "documented, not measured".
