#!/usr/bin/env python3
"""Post-process the built skin's TUI.json (vst.json "skin_post": the platform runs `python3 tools/skin_post.py <skin dir>` after
building the skin; a non-zero exit fails the build). From CTRL Giant (2026-11-12):
  1. identical local component definitions are merged into one (the first name kept), and every use of the others renamed;
  2. TUI.json is written compact;
  3. every menu closes when the screen is tapped outside its list (the user's call): under each list, an invisible button the size of
     the page, shown only while that list is open (the list's own IndexedEnabling handle), whose tap toggles the list's
     "<key>__open" parameter -- the same "Toggle Switch" the menu's field does -- so it closes. It sits after every other part of the
     page and just before its list, so the list stays on top and still takes its taps.
  4. (2026-11-13, house style from CTRL Giant) the Q-Link rings: each Q-Link column's ring takes in its knobs' names and value
     readouts, and stops short of other controls (tools/qlink_rings.py; settings QLINK_RING_FULL, RING_EXTRA in gen_layout.py).
The result is checked: every component type a page uses still has a definition.

    tools/skin_post.py "<skin dir>"     (the folder that holds "Plugin Skins/TUI.json")
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import qlink_rings  # noqa: E402

BACKDROP = "shBackdrop"


def backdrops(tui, defs):
    """Item 4: a page-sized invisible button under each menu's list; returns its definition."""
    clear = {"version": 1, "colour": "0", "image": ""}
    page_w, page_h = 1280, 628
    tap = {"version": 2, "onAction": "Mouse Down", "handler": "Toggle Switch", "handleName": "Data", "additionalData": "",
           "handle remapping": {"version": 1, "map": []}}
    focus = {"version": 2, "componentData": {"version": 1, "name": "Focus", "type": "Focus",
                                             "data": {"version": 1, "backgroundColour": "00000000", "outlineColour": "00000000",
                                                      "backgroundInset": 0.0, "outlineThickness": 0.0}},
             "handle remapping": {"version": 1, "map": []},
             "bounds": {"version": 2, "acceptsHWFocus": "No", "showWhenDataModelInvalid": "Show", "whenVisible": "Always",
                        "boundsType": "Absolute", "bounds": "0 0 %d %d" % (page_w, page_h), "additionalInvalidatingHandles": []}}
    n = 0
    for d in defs:
        comps = d["value"].get("componentsData")
        if not isinstance(comps, list):
            continue
        out = []
        for c in comps:
            cd = c.get("componentData", {})
            if isinstance(cd.get("type"), str) and cd["type"].startswith("shPopPanel_"):
                data = [m["value"] for m in c["handle remapping"]["map"] if m["key"] == "Data"]
                if not data:
                    sys.exit("skin_post: a menu list with no Data handle (%s)" % cd["type"])
                out.append({"version": 2,
                            "componentData": {"version": 1, "name": "%s backdrop" % cd.get("name", ""), "type": BACKDROP,
                                              "data": {"version": 1, "handleName": "Data"}},
                            "handle remapping": {"version": 1, "map": [{"key": "Data", "value": data[0]}]},
                            "bounds": {"version": 2, "acceptsHWFocus": "No", "showWhenDataModelInvalid": "Show",
                                       "whenVisible": "Always", "boundsType": "Absolute", "bounds": "0 0 %d %d" % (page_w, page_h),
                                       "additionalInvalidatingHandles": list(c["bounds"]["additionalInvalidatingHandles"])}})
                n += 1
            out.append(c)
        d["value"]["componentsData"] = out
    backdrops.count = n
    return {"key": BACKDROP, "value": {"version": 4, "actions": [tap], "backgroundData": {"version": 1, "focussed": clear,
                                                                                         "unfocussed": clear},
                                       "ignoreMousePresses": False, "disableCoarseDataWheel": False, "repeats": 1,
                                       "hideQLinkBounds": True, "componentsData": [focus]}}


def main():
    path = os.path.join(sys.argv[1], "Plugin Skins", "TUI.json")
    before = os.path.getsize(path)
    tui = json.load(open(path, encoding="utf-8"))
    defs = tui["pageData"]["componentDefinitions"]["localComponentDefinitions"]

    first, rename, kept = {}, {}, []
    for d in defs:
        body = json.dumps(d["value"], sort_keys=True, separators=(",", ":"))
        if body in first:
            rename[d["key"]] = first[body]
        else:
            first[body] = d["key"]
            kept.append(d)

    def walk(o):
        if isinstance(o, dict):
            for k, v in o.items():
                if k in ("type", "componentName") and isinstance(v, str) and v in rename:
                    o[k] = rename[v]
                else:
                    walk(v)
        elif isinstance(o, list):
            for v in o:
                walk(v)

    tui["pageData"]["componentDefinitions"]["localComponentDefinitions"] = kept
    walk(tui)

    import gen_layout as G   # its ring settings, if any (the default: every page's rings take in names and readouts)
    rings = qlink_rings.sized_rings(tui, kept, getattr(G, "QLINK_RING_FULL", None), getattr(G, "RING_EXTRA", None))
    kept.append(backdrops(tui, kept))
    have = {d["key"] for d in kept}
    used = set()

    def uses(o):
        if isinstance(o, dict):
            cd = o.get("componentData")
            if isinstance(cd, dict) and isinstance(cd.get("type"), str) and (cd["type"].startswith("sh") or "|" in cd["type"]):
                used.add(cd["type"])
            for v in o.values():
                uses(v)
        elif isinstance(o, list):
            for v in o:
                uses(v)

    uses(tui)
    missing = sorted(u for u in used if u not in have)
    if missing:
        sys.exit("skin_post: component types with no definition after merging: %s" % ", ".join(missing[:10]))

    text = json.dumps(tui, separators=(",", ":"))
    open(path, "w", encoding="utf-8").write(text)
    print("skin_post: %d definitions merged into %d; TUI.json %.1f MB -> %.1f MB; %d menu backdrops; %d Q-Link rings"
          % (len(defs), len(kept), before / 1e6, len(text) / 1e6, backdrops.count, rings))


if __name__ == "__main__":
    main()
