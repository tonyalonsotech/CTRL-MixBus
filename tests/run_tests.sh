#!/usr/bin/env bash
# Host tests: every DSP module alone (incl. the maximizer), and the plugin through its real mpc_engine() interface
# (parameters, the "state" string MPC stores, gate/maximizer behaviour, meters, filter-curve codes).
# Needs gcc and libm (Python 3 + Pillow + numpy only for the generated-file checks, which are skipped without them).
#   tests/run_tests.sh           sanitizer build (ASan + UBSan)
#   tests/run_tests.sh fast      -O2, no sanitizers (representative CPU timing)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
OUT="${TMPDIR:-/tmp}/mpc-ctrl-mixbus-tests"; mkdir -p "$OUT"
# upstream checkout: $MPC_VST_PLUGINS, else third_party/ (submodule), else a sibling folder ../mpc-vst-plugins
UP="${MPC_VST_PLUGINS:-}"
[ -n "$UP" ] || { for c in "$ROOT/third_party/mpc-vst-plugins" "$ROOT/../mpc-vst-plugins"; do [ -f "$c/wrapper/engine.h" ] && UP="$c" && break; done; }
WRAP="$UP/wrapper"
[ -f "$WRAP/engine.h" ] || { echo "mpc-vst-plugins not found: set MPC_VST_PLUGINS=/path/to/mpc-vst-plugins or run  git submodule update --init" >&2; exit 2; }
MODE="${1:-san}"
case "$MODE" in
  fast) F="-O2" ;;
  *)    F="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" ;;
esac
CF="-std=gnu11 -Wall -Wextra -Wno-unused-function -Werror $F"
D="$ROOT/src/dsp"

gcc $CF -o "$OUT/test_dsp" "$HERE/test_dsp.c" "$D"/*.c -lm
"$OUT/test_dsp"
gcc $CF -I"$WRAP" -o "$OUT/test_engine" "$HERE/test_engine.c" "$ROOT/src/engine/dj_engine.c" "$D"/*.c -lm
"$OUT/test_engine"

if [ "$MODE" = san ] && command -v python3 >/dev/null && python3 -c "import PIL" 2>/dev/null; then
  echo "generated files"
  python3 "$ROOT/tools/gen_params.py" --check
  python3 "$ROOT/tools/gen_layout.py" --check
  python3 "$ROOT/tools/gen_meter_strip.py" --check
  python3 "$ROOT/tools/gen_readout_strip.py" --check
  python3 "$ROOT/tools/gen_utility_art.py" --check
  if python3 -c "import numpy" 2>/dev/null; then
    python3 "$ROOT/tools/gen_filter_strips.py" --check   # ~30 s: measures the real filter code (needs gcc + numpy)
  else
    echo "art/flt_*.png: NOT checked (no numpy for python3)"
  fi
fi
echo "ALL HOST TESTS PASSED ($MODE)"
