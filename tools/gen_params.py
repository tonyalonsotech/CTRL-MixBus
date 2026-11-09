#!/usr/bin/env python3
"""Generate params.json and presets.json from src/engine/dj_params.def (the single source of truth).

    tools/gen_params.py           write params.json and presets.json next to vst.json
    tools/gen_params.py --check   exit 1 if either file is out of date (CI / pre-release gate)

Output is deterministic (sorted nothing, fixed key order, trailing newline) so a rebuild is byte-identical.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEF = os.path.join(ROOT, "src", "engine", "dj_params.def")
LINE = re.compile(r'^DJP\(\s*(\w+)\s*,\s*"(\w+)"\s*,\s*"([^"]*)"\s*,\s*(TOG|OPT|KNOB|SW|METER)\s*,\s*([-0-9.]+)\s*,\s*"([^"]*)"\s*\)\s*(?:/\*.*\*/)?\s*$')


def parse():
    rows = []
    for ln, line in enumerate(open(DEF, encoding="utf-8"), 1):
        s = line.strip()
        if not s.startswith("DJP("):
            continue
        m = LINE.match(s)
        if not m:
            sys.exit("%s:%d: cannot parse: %s" % (DEF, ln, s))
        ident, key, name, kind, dflt, opts = m.groups()
        rows.append(dict(id=ident, key=key, name=name, kind=kind, default=float(dflt), options=opts.split("|") if opts else []))
    keys = [r["key"] for r in rows]
    if len(set(keys)) != len(keys):
        sys.exit("duplicate key in dj_params.def")
    for r in rows:
        if len(r["name"]) > 12:
            sys.exit("name too long for an MPC label (max 12): %r" % r["name"])
    return rows


def params_json(rows):
    out = []
    for r in rows:
        p = {"key": r["key"], "name": r["name"]}
        if r["kind"] == "TOG":
            p["options"] = ["Off", "On"]
            p["default"] = int(r["default"])
        elif r["kind"] == "OPT":
            p["options"] = r["options"]
            p["default"] = int(r["default"])
        elif r["kind"] == "SW":       # continuous for the host (no snapping); the engine reads >= 0.5 as on, text "On"/"Off"
            p.update({"min": 0.0, "max": 1.0, "default": r["default"], "dynamic_display": True})
        elif r["kind"] == "KNOB":
            p.update({"min": 0.0, "max": 1.0, "default": r["default"], "dynamic_display": True})
        else:  # METER: engine-driven, never user-set
            p.update({"min": 0.0, "max": 1.0, "default": 0.0, "live": True})
        out.append(p)
    return {"name": "CTRL MixBus", "params": out}


def presets_json(rows):
    """MPC's PRESET menu: one stock preset, Init = the defaults. Every parameter is set, in index order."""
    vals = {}
    for r in rows:
        if r["kind"] not in ("TOG", "OPT", "KNOB", "SW"):   # meters are never part of a sound
            continue
        vals[r["key"]] = r["options"][int(r["default"])] if r["kind"] == "OPT" else (("On" if r["default"] else "Off") if r["kind"] == "TOG" else r["default"])
    return {"presets": [{"name": "Init", "values": vals}]}


def dump(obj):
    return json.dumps(obj, indent=1, ensure_ascii=False) + "\n"


def main():
    rows = parse()
    targets = {os.path.join(ROOT, "params.json"): dump(params_json(rows)),
               os.path.join(ROOT, "presets.json"): dump(presets_json(rows))}
    if "--check" in sys.argv:
        bad = [os.path.basename(p) for p, t in targets.items() if not os.path.exists(p) or open(p, encoding="utf-8").read() != t]
        if bad:
            sys.exit("out of date: %s (run tools/gen_params.py)" % ", ".join(bad))
        print("params.json / presets.json up to date (%d params)" % len(rows))
        return
    for p, t in targets.items():
        open(p, "w", encoding="utf-8", newline="\n").write(t)
    print("wrote params.json, presets.json (%d params)" % len(rows))


if __name__ == "__main__":
    main()
