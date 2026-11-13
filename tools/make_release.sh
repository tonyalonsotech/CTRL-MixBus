#!/usr/bin/env bash
# Package the built plugin into install zips for Plugin Manager G2 / install.sh, plus the folder-only zips for MPC OS 3.10.1's
# VST2 Plugins folder (run tools/build_device.sh first).
#   tools/make_release.sh            both zips: dist/CTRL-MixBus-<version>-mpc-aarch64.zip (Gen 2) and -mpc-armv7.zip (Gen 1)
#   tools/make_release.sh aarch64    just the Gen 2 zip
# The version comes from vst.json ("version": major*1000 + minor*100 + patch*10, e.g. 1030 = 1.0.3), so the zip, the
# plugin's own version and the skin's version.xml always agree. It also points the catalog entry's screenshot
# (catalog/ctrl-mixbus.json) at the release tag v<version>, so the Open MPC card shows the picture of that release: commit
# docs/preview.png before tagging, and send the changed entry with the catalog PR for the release.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
SUB="${MPC_VST_PLUGINS:-}"
if [ -z "$SUB" ]; then
  for c in third_party/mpc-vst-plugins ../mpc-vst-plugins; do [ -f "$c/tools/release.py" ] && SUB="$c" && break; done
fi
[ -n "$SUB" ] || { echo "mpc-vst-plugins checkout not found (set MPC_VST_PLUGINS)" >&2; exit 1; }

ABOUT="$(cat ABOUT.txt)"   # the Plugin Manager description (manifest "about"): edit ABOUT.txt
VERSION="$(python3 -c 'import json; v = json.load(open("vst.json"))["version"]; print("%d.%d.%d" % (v // 1000, v // 100 % 10, v // 10 % 10))')"
SKIN="build/skin/CTRL - VST - CTRL MixBus"

targets="${1:-aarch64 armv7}"
for t in $targets; do
  case "$t" in
    aarch64) so=build/aarch64/ctrl_mixbus.so ;;
    armv7)   so=build/ctrl_mixbus.so ;;
    *) echo "unknown target $t (aarch64 or armv7)" >&2; exit 1 ;;
  esac
  [ -f "$so" ] || { echo "$so missing: run tools/build_device.sh first" >&2; exit 1; }
  python3 "$SUB/tools/release.py" --so "$so" --skin "$SKIN" --entry build/pluginlist-entry.xml \
    --version "$VERSION" --about "$ABOUT" --repo tonyalonsotech/CTRL-MixBus --license AGPL-3.0-only --id ctrl-mixbus -o dist
done

# MPC OS 3.10.1 and later (Akai's plugin folder format): plugin.json gets "format": "vst2", each zip is checked against the
# catalog spec, and a folder-only zip (<Name>-<ver>-Gen2.zip / -Gen1.zip) is written for copying into <drive>/VST2 Plugins/
STEM="$(printf %s "${SKIN##* - VST - }" | sed -E 's/[^A-Za-z0-9]+/-/g; s/^-+|-+$//g')"   # release.py's zip name
zips=(); for t in $targets; do zips+=("dist/$STEM-$VERSION-mpc-$t.zip"); done
python3 tools/vst2_zips.py "$SUB" "${zips[@]}"

# the catalog card's screenshot follows the release tag, never main (Open MPC maintainers' request, 2.0.0)
sed -i -E "s#(raw\.githubusercontent\.com/tonyalonsotech/CTRL-MixBus/)[^/]+(/docs/preview\.png)#\1v$VERSION\2#" catalog/ctrl-mixbus.json
grep -q "/v$VERSION/docs/preview.png" catalog/ctrl-mixbus.json || { echo "catalog/ctrl-mixbus.json: screenshot not set to v$VERSION" >&2; exit 1; }
echo "catalog screenshot: v$VERSION (tag v$VERSION must contain docs/preview.png; include catalog/ctrl-mixbus.json in the catalog PR)"
