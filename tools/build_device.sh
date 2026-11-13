#!/usr/bin/env bash
# Build the plugin for the MPC (armv7 = Gen 1, aarch64 = Gen 2) in Docker and write a reproducibility record.
#   tools/build_device.sh            builds both targets
# Needs: git, docker (with QEMU/binfmt for arm32/arm64), python3. Output: build/ and build/BUILD_RECORD.txt
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
# mpc-vst-plugins 20f91bf (2026-11-13: MPC OS 3.10.1 plugin.json, the display_rev thread). A git checkout is checked out at PIN;
# a plain download (no .git) is checked by the content the build uses (wrapper/, adapters/, tools/), PIN_SUM
PIN=20f91bf064633029c036c0a345f5ebdee888aca2
PIN_SUM=94599e61954c0ac3
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
upstream_sum() { (cd "$1" && find wrapper adapters tools -type f -not -path '*/__pycache__/*' -not -name '*.pyc' -print0 \
  | LC_ALL=C sort -z | xargs -0 sha256sum | sha256sum | cut -c1-16); }
if [ -e "$SUB/.git" ]; then
  HEAD_SUB="$(git -C "$SUB" rev-parse HEAD 2>/dev/null || echo unknown)"
  [ "$HEAD_SUB" = "$PIN" ] || echo "WARNING: mpc-vst-plugins is at $HEAD_SUB, not the pinned $PIN"
else
  SUM="$(upstream_sum "$SUB")"
  if [ "$SUM" = "$PIN_SUM" ]; then HEAD_SUB="$PIN (download, content $SUM)"
  else HEAD_SUB="unknown (download, content $SUM)"; echo "WARNING: mpc-vst-plugins is a download whose content ($SUM) is not the pinned ${PIN:0:7} ($PIN_SUM)"; fi
fi
if command -v gcc >/dev/null; then tests/run_tests.sh fast; fi
"$SUB/tools/build_port.sh" vst.json all
{
  echo "built: $(date -u +%FT%TZ)"
  echo "project git: $(git rev-parse HEAD 2>/dev/null || echo 'not a git checkout')"
  echo "upstream:    $HEAD_SUB (pinned $PIN)"
  echo "docker:      $(docker --version)"
  echo "host python: $(python3 --version)"
  for img in arm32v7/gcc:11-bullseye arm64v8/gcc:12-bookworm; do
    echo "compiler $img: $(docker run --rm "$img" gcc --version 2>/dev/null | head -1)"
  done
  echo "flags: see docs/BUILDING.md and vst.json"
  find build -name '*.so' -print0 | sort -z | xargs -0 sha256sum
  find build -name '*.so' -print0 | sort -z | xargs -0 file
} | tee build/BUILD_RECORD.txt
