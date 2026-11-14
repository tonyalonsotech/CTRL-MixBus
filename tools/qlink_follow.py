"""A tap on a button selects its column's Q-Link bank, as a touch on a knob does (house style since 2026-11-13, from CTRL MixBus
2.1.5, the user's call; tried and working on the user's MPC).

Every control's tap runs MPC's "Q-Link" action (Akai's MPC Plugin SDK, docs/ui-reference.md 4.2: it banks the Q-Links to the bank
holding the parameter it names). A knob's names its own parameter, so touching a knob brings its column to the Q-Links. A button,
an option segment or a menu field names its own parameter too, which no bank holds, so its tap did nothing. follow() gives each
such control a copy of its definition whose "Q-Link" action names the first Q-Link control of the column it belongs to instead.

Which column: the house layouts put a column's buttons and menus to the RIGHT of its knobs (CTRL MixBus, CTRL Giant), or centred
on them (the FILTER pages' slope buttons). So a control belongs to the nearest Q-Link column whose knobs stand left of it, or no
more than LEFT_REACH px right of it. A control left of every column, or one whose parameter is itself on a Q-Link, is left as it
is; so are menu lists, their options and backdrops, pictures and meters.

Settings in tools/gen_layout.py (optional):
  QLINK_BUTTONS = {"PAGE": {column: ("key", ...)}}   those buttons' columns by hand, where the position rule picks wrong
                                                    (column 0 = Q-Links 1-4); the page's other buttons still go by position
  QLINK_FOLLOW_OFF = ("PAGE", ...)                    pages left as they are

Called by tools/skin_post.py; a control's copy is "<its definition>_ql<parameter>".
"""
import json
import os
import re
import sys


SKIP = ("Image", "shMeter", "shKnob", "shBackdrop", "shPopOpt", "shPopPanel", "shOffHead")
LEFT_REACH = 120                       # a knob's value text is 130 px wide: a control centred under a knob still counts as its


def layout_tabs(path):
    """{page: {"qlinks": [keys], "knobs": {key: (cx, cy)}}} from layout.conf (as tools/qlink_rings.py reads it)."""
    tabs, cur = {}, None
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        m = re.match(r"\[tab (.*)\]$", line)
        if m:
            cur = tabs[m.group(1)] = {"qlinks": [], "knobs": {}}
        elif cur is not None and line.startswith("qlinks "):
            cur["qlinks"] += [k.strip() for k in line.split("=", 1)[1].split(",")]
        elif cur is not None and line.startswith("knob "):
            a = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))
            cur["knobs"].setdefault(a["key"], (int(a["cx"]), int(a["cy"])))
    return tabs


def _cx(comp):
    x, _, w, _ = (float(v) for v in comp["bounds"]["bounds"].split())
    return x + w / 2


def _columns(L, index):
    """[(left edge, first key)] for a page's Q-Link columns that have knobs: their knobs' leftmost x less LEFT_REACH, left to right."""
    cols = []
    q = L["qlinks"]
    for c in range(len(q) // 4):
        keys = [k for k in q[c * 4:c * 4 + 4] if k != "-" and k in index]
        xs = [L["knobs"][k][0] for k in keys if k in L["knobs"]]
        if keys and xs:
            cols.append((min(xs) - LEFT_REACH, keys[0]))
    return sorted(cols)


def follow(tui, defs, root, by_hand=None, off=()):
    """Point each button's "Q-Link" tap at its column's first Q-Link control, in place. Returns {page: {first key: [keys]}}."""
    by_hand = by_hand or {}
    keys = [p["key"] for p in json.load(open(os.path.join(root, "params.json"), encoding="utf-8"))["params"]]
    index = {k: i for i, k in enumerate(keys)}
    lay = layout_tabs(os.path.join(root, "layout.conf"))
    byname = {d["key"]: d for d in defs}
    done = {}
    for tab in tui["pageData"]["tabs"]:
        name = tab["tabName"]
        if name not in lay or name in off:
            continue
        L = lay[name]
        page = tab["componentDefinition"] if "componentDefinition" in tab else byname[tab["componentName"]]["value"]
        banked = {k for k in L["qlinks"] if k != "-"}
        cols = _columns(L, index)
        hand = {}
        for c, ks in by_hand.get(name, {}).items():
            first = [k for k in L["qlinks"][c * 4:c * 4 + 4] if k != "-"]
            if not first:
                sys.exit("qlink_follow: %s column %d has no Q-Link control for its buttons" % (name, c))
            hand.update({k: first[0] for k in ks})
        found = set()
        for comp in page["componentsData"]:
            ty = comp["componentData"]["type"]
            if ty.startswith(SKIP) or ty not in byname:
                continue
            data = [m["value"] for m in comp["handle remapping"]["map"] if m["key"] == "Data"]
            m = re.match(r"Parameter (\d+)$", data[0]) if data else None
            if not m or int(m.group(1)) >= len(keys):
                continue
            key = keys[int(m.group(1))]
            if key in hand:
                to = hand[key]
            elif key in banked:
                to = None
            else:
                cx = _cx(comp)
                to = next((k for x0, k in reversed(cols) if x0 <= cx), None)
            if not to:
                continue
            acts = [a for a in byname[ty]["value"].get("actions", []) if a["handler"] == "Q-Link"]
            if not acts:
                continue
            ckey = "%s_ql%d" % (ty, index[to])
            if ckey not in byname:
                d = json.loads(json.dumps(byname[ty]))
                d["key"] = ckey
                for a in d["value"]["actions"]:
                    if a["handler"] == "Q-Link":
                        a["handleName"] = "Parameter %d" % index[to]
                defs.append(d)
                byname[ckey] = d
            comp["componentData"]["type"] = ckey
            found.add(key)
            done.setdefault(name, {}).setdefault(to, set()).add(key)
        lost = sorted(set(hand) - found)
        if lost:
            sys.exit("qlink_follow: %s: no button on the page for %s" % (name, ", ".join(lost)))
    return {p: {k: sorted(v) for k, v in c.items()} for p, c in done.items()}
