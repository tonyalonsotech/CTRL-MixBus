#!/usr/bin/env bash
# Build the plugin for the MPC (armv7 = Gen 1, aarch64 = Gen 2) in Docker and write a reproducibility record.
#   tools/build_device.sh            builds both targets
# Needs: git, docker (with QEMU/binfmt for arm32/arm64), python3. Output: build/ and build/BUILD_RECORD.txt
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
PIN=5432c834e4b9967900b4f2fbb89b0169b27fe350
# upstream checkout: $MPC_VST_PLUGINS, else third_party/ (submodule), else sibling ../mpc-vst-plugins, else clone into third_party/
SUB="${MPC_VST_PLUGINS:-}"
if [ -z "$SUB" ]; then
  for c in third_party/mpc-vst-plugins ../mpc-vst-plugins; do [ -f "$c/tools/build_port.sh" ] && SUB="$c" && break; done
fi
SUB="${SUB:-third_party/mpc-vst-plugins}"
export MPC_VST_PLUGINS="$(cd "$SUB" 2>/dev/null && pwd || echo "$ROOT/$SUB")"
if [ ! -f "$SUB/tools/build_port.sh" ]; then
  rm -rf "$SUB"; mkdir -p third_party
  git clone --recursive https://github.com/open-mpc/mpc-vst-plugins.git "$SUB"
fi
if [ -e "$SUB/.git" ]; then
  git -C "$SUB" fetch -q origin "$PIN" 2>/dev/null || true
  git -C "$SUB" checkout -q "$PIN"
  git -C "$SUB" submodule update -q --init --recursive
fi
HEAD_SUB="$(git -C "$SUB" rev-parse HEAD 2>/dev/null || echo unknown)"
[ "$HEAD_SUB" = "$PIN" ] || echo "WARNING: submodule is at $HEAD_SUB, not the pinned $PIN"
if command -v gcc >/dev/null; then tests/run_tests.sh fast; fi
"$SUB/tools/build_port.sh" vst.json all
{
  echo "built: $(date -u +%FT%TZ)"
  echo "project git: $(git rev-parse HEAD 2>/dev/null || echo 'not a git checkout')"
  echo "submodule:   $HEAD_SUB (pinned $PIN)"
  echo "docker:      $(docker --version)"
  echo "host python: $(python3 --version)"
  for img in arm32v7/gcc:11-bullseye arm64v8/gcc:12-bookworm; do
    echo "compiler $img: $(docker run --rm "$img" gcc --version 2>/dev/null | head -1)"
  done
  echo "flags: see docs/BUILDING.md and vst.json"
  find build -name '*.so' -print0 | sort -z | xargs -0 sha256sum
  find build -name '*.so' -print0 | sort -z | xargs -0 file
} | tee build/BUILD_RECORD.txt
