# Building and reproducing

## Dependencies (all identified, nothing hidden)

- The upstream wrapper and tooling are a git submodule: `third_party/mpc-vst-plugins`
  (https://github.com/open-mpc/mpc-vst-plugins.git). **Pinned commit: `5432c834e4b9967900b4f2fbb89b0169b27fe350`.**
  Clone with `git clone --recurse-submodules`, or run `git submodule update --init`.
- No firmware, SDK or proprietary file is needed or redistributed. The plugin is plain C11 plus libm.
- Generated-art tools: Python 3, Pillow, numpy and gcc. These affect only the skin art and test checks, not the
  plugin's code.

## Build the plugin

```
python3 third_party/mpc-vst-plugins/tools/gen_vst.py vst.json     # generates the skin and parameter header into build/
third_party/mpc-vst-plugins/tools/build_port.sh vst.json          # needs Docker; builds armv7 and aarch64
```

`build_port.sh` compiles in containers whose glibc matches the devices:

| Target | Image | Notes |
|---|---|---|
| Gen 1 (armv7) | `arm32v7/gcc:11-bullseye` | glibc 2.31 |
| Gen 2 (aarch64) | `arm64v8/gcc:12-bookworm` | install into `pluginList-arm-64bit` |

Flags (from the upstream script, plus ours from `vst.json`):

```
gcc -O2 -Wall -Wextra -Wno-unused-parameter -fPIC -shared -fvisibility=hidden -std=gnu11 \
    -Isrc/dsp -Isrc/engine -ffp-contract=off -fno-fast-math  <sources> vst2_wrap.c \
    -lm -lpthread -ldl -Wl,--no-undefined        # then strip
```

`-ffp-contract=off -fno-fast-math` keep the float results identical between architectures, which is what lets the
tests compare audio bit for bit. Defines: `HAS_DISPLAY_REV`, `DISPLAY_REV_NO_UPDATE`. The plugin needs only libm (the wrapper adds pthread/dl).
Entry point and plugin metadata (name `CTRL MixBus`, vendor `CTRL`, file `ctrl_mixbus.so`, UID `DjUS`) come from `vst.json`. The UID
stays `DjUS` (not the proof of concept's `CTMX`), so this plugin and an installed PoC are never mixed up.

**Status of device builds:** `tools/build_device.sh` built both targets in Docker for 0.1.0, 0.2.0, 1.0.3 and 1.0.4, and all ran on an MPC One (Gen 2).
1.0.5 is built (2026-11-09) but not yet run on a device; the armv7 (Gen 1) build has never been run on hardware.

## Reproducibility record (fill in for every release)

Record: git commit or tag of this repository, the submodule commit above, the compiler versions printed by the
containers, the flags above, and the sha256 of each `.so`. The host test runs so far used gcc 13.3.0 and Python 3.13.

## Tests (host, no hardware)

```
tests/run_tests.sh          # ASan + UBSan, plus checks that generated files are up to date
tests/run_tests.sh fast     # -O2, representative CPU timing
```

On the x86 dev host the full chain (with the maximizer) uses about 49 us per 128-frame block in the optimised build (`tests/run_tests.sh fast`), against a
budget of roughly 2900 us at 44.1 kHz. This is not an MPC measurement.

## Regenerating generated files

`tools/gen_params.py`, `gen_layout.py` (also writes `skin.css`), `gen_meter_strip.py`, `gen_readout_strip.py`,
`gen_filter_strips.py`. Each has `--check`. Colours come from `tools/palette.py`: after changing one, rerun all of them.
Not generated (hand-made skin art, 2026-11-09): the page backgrounds `art/page_input.png`, `page_filter.png`, `page_output.png`
(1280 x 628, the whole plugin area, panels and titles included) and the knob filmstrips `art/knob_strip.png` and
`art/knob_plain_strip.png` (128 frames of 78 x 78). Replace them with files of the same size; `gen_layout.py` explains how
they are placed (`PAGE_ART`). Edit `src/engine/dj_params.def` (append only), or `src/dsp/dj_view.h`, then rerun.
The generators that draw text (`gen_filter_strips.py`, `gen_readout_strip.py`) take the skin's font from the mpc-vst-plugins checkout
(`tools/font_path.py`: `$MPC_VST_PLUGINS`, `third_party/`, or the sibling folder). `gen_filter_strips.py` also needs numpy; without it
`tests/run_tests.sh` says the filter pictures were not checked. Pixels can differ slightly between Pillow/numpy versions
(anti-aliasing), so regenerate and check on the same machine.

Memory note: the twelve filter-curve strips (201 frames each: 40 cutoff steps x 5 resonance steps + 1) decode to about 119 MB
(0.2.0: 121 frames, 71 MB; 0.1.0: 41 MB). If a device struggles to load the skin, lower `DJ_VIEW_RES` (resonance steps) or
`DJ_VIEW_STEPS` / `DJ_VIEW_W` / `DJ_VIEW_H` in `dj_view.h`, then rerun the `gen_*` scripts.

## License

Not chosen yet. The owner must pick an SPDX license and add a `LICENSE` file before any public release.
