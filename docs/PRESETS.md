# Presets and saved state

The plugin has **no preset system of its own**. Presets and projects are handled by the MPC's built-in plugin preset
system, and the plugin makes sure everything that system needs works:

- **Saving and loading presets, and projects.** The MPC asks the plugin for its state and hands it back later. The plugin
  answers with one short text string (below). Every sound parameter is in it, nothing else is.
- **The stock program "Init".** The MPC's PRESET menu lists one program, Init, generated from the defaults in
  `src/engine/dj_params.def` into `presets.json`. Choosing it sets every parameter to its default.
- **Automation and Q-Links** use the parameter list (`params.json`); the order follows the controls on the pages.

## The state string

```
mpcdj:2;in_gain=700000;in_on=1000000;ph_inv=0;...;mx_out=1000000;
```

- Tag `mpcdj:2;` (1.0.5; `mpcdj:1;` up to 1.0.4, still read: its switches are 0/1), then `key=value;` pairs in parameter order. Keys are the stable parameter keys, never positions.
- Knobs and switches are integers in millionths (0..1000000; a switch is on from 500000). Option lists are their index (`eq_mode` 0 = EQ,
  1 = ISOLATOR, curves 0..5 = 12, 24, 36, 48, 96, BRICK). No decimal point anywhere, so it reads the same under every locale.
- Meters and the filter-picture codes are display only and are never stored. Neither are `lp_crv`/`hp_crv` (the Q-Link slope
  controls): they are set from `lp_curve`/`hp_curve` on load.
- **Reading is tolerant.** Unknown keys are skipped. Keys that are missing take their default, so a state written before a
  parameter existed loads with that parameter at Init. Out-of-range values are clamped. A string with another tag, or no tag,
  changes nothing. Tested with random and truncated input under ASan/UBSan (`tests/test_engine.c`).
- It is about 700 bytes; the wrapper allows 8192.

## Compatibility rule

A shipped key is never renamed or reused, and a shipped option never changes meaning or order. New parameters are appended to
`dj_params.def`. The parameter list was renumbered once, in 0.2.0, and the stage switches renamed and inverted in 1.0.3
(`*_byp` -> `*_on`), both before any release was published; from the first release `params.lock` makes the build refuse a reorder.
A 0.2.0 state loads in 1.0.3 with every stage switch at its Init value (the old `*_byp` keys are unknown and skipped).

## No files

The plugin writes no files and reads none. In 0.1.0 it had a 16-slot preset page with `User NN.json` files; that was removed in 0.2.0
because the MPC's own presets do the job. Nothing on the device needs deleting, but `presets/` next to the old plugin folder can go.
