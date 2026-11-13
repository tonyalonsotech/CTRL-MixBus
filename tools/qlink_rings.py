"""The Q-Link rings (house style since 2026-11-13, from CTRL Giant 1.2.7): MPC draws a ring around the Q-Link column the
Q-Links are driving, as in its own plugins, so you can see which controls the knobs will move before touching anything.

The layout asks the platform for one rectangle per Q-Link column (`qlink_bounds=column`, tools/gen_layout.py theme()); the
platform sizes them around the knobs only. tools/skin_post.py then calls sized_rings(), which makes each ring take in its knobs'
names and value readouts (a knob's readout is 130 px wide and ends 89 px below its centre), and the controls listed for it in
gen_layout.RING_EXTRA (e.g. a filter column's slope buttons), and stops every edge 4 px short of any other visible control.

Optional settings in tools/gen_layout.py:
  QLINK_RING_FULL = ("PAGE", ...)    the pages whose rings take in names and readouts (default: every page); the others keep the
                                     platform's knob-only ring (CTRL Giant: the pages where knobs sit beside target menus)
  RING_EXTRA = {"PAGE": {column: ("key", ...)}}   controls a column's ring also encloses (column 0 = Q-Links 1-4)

MPC draws the ring over everything, open menus included, and no skin flag hides it (CTRL Giant 1.2.8). MPC chooses its colour.
"""
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VALUE_W, PAD, GAP = 130, 6, 4          # a knob's value readout's width; the ring's margin; its gap to other controls
Y_OFF, PAGE_W, PAGE_H = 86, 1280, 628  # layout.conf y is the plugin area's; a page's own starts 86 px lower


def layout_tabs(path):
    """{page: {"qlinks": [16 keys], "knobs": {key: (cx, cy, r)}}} from layout.conf (plugin coordinates)."""
    tabs, cur = {}, None
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        m = re.match(r"\[tab (.*)\]$", line)
        if m:
            cur = tabs[m.group(1)] = {"qlinks": [], "knobs": {}}
        elif cur is not None and line.startswith("qlinks "):
            cur["qlinks"] = [k.strip() for k in line.split("=", 1)[1].split(",")]
        elif cur is not None and line.startswith("knob "):
            a = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))
            cur["knobs"].setdefault(a["key"], (int(a["cx"]), int(a["cy"]), int(a.get("r", 35))))
    return tabs


def full_ring(knobs):
    """A column's ring around its knobs, their names and value readouts (page coordinates), or None."""
    if not knobs:
        return None
    x0 = max(0, min(cx for cx, _, _ in knobs) - VALUE_W // 2 - PAD)
    x1 = min(PAGE_W, max(cx for cx, _, _ in knobs) + VALUE_W // 2 + PAD)
    y0 = max(0, min(cy - r - 8 for _, cy, r in knobs) - Y_OFF - PAD)
    y1 = min(PAGE_H, max(cy + 89 for _, cy, _ in knobs) - Y_OFF + PAD)
    return [x0, y0, x1, y1]


def refs(comp):
    return {int(v) for v in re.findall(r"Parameter (\d+)", json.dumps(comp.get("handle remapping", {})))}


def sized_rings(tui, defs, full_pages=None, extra=None, params_json=None, layout_conf=None):
    """Size every page's Q-Link rings (qlinkBoundsData, one per column) in place. Returns how many."""
    extra = extra or {}
    keys = [p["key"] for p in json.load(open(params_json or os.path.join(ROOT, "params.json"), encoding="utf-8"))["params"]]
    index = {k: i for i, k in enumerate(keys)}
    lay, byname = layout_tabs(layout_conf or os.path.join(ROOT, "layout.conf")), {d["key"]: d["value"] for d in defs}
    n = 0
    for tab in tui["pageData"]["tabs"]:
        name = tab["tabName"]
        if name not in lay or not tab.get("qlinkBoundsData"):
            continue
        page = tab["componentDefinition"] if "componentDefinition" in tab else byname[tab["componentName"]]
        comps = page["componentsData"]
        L, rects = lay[name], []
        for c, r in enumerate(tab["qlinkBoundsData"]):
            x, y, w, h = (int(v) for v in r.split())
            col = [k for k in L["qlinks"][c * 4:c * 4 + 4] if k != "-"]
            add = {index[k] for k in extra.get(name, {}).get(c, ()) if k in index}
            box = None if not w else [x, y, x + w, y + h]
            if full_pages is None or name in full_pages:
                box = full_ring([L["knobs"][k] for k in col if k in L["knobs"]]) or box
            for comp in comps:                         # the controls RING_EXTRA adds (their whole drawn box)
                if add and comp["componentData"]["type"] != "Image" and refs(comp) & add:
                    bx, by, bw, bh = (int(float(v)) for v in comp["bounds"]["bounds"].split())
                    b2 = [bx - PAD, by - PAD, bx + bw + PAD, by + bh + PAD]
                    box = b2 if box is None else [min(box[0], b2[0]), min(box[1], b2[1]), max(box[2], b2[2]), max(box[3], b2[3])]
            if box:
                own = {index[k] for k in col if k in index} | add
                knobs = [L["knobs"][k] for k in col if k in L["knobs"]]
                span = (min(cy - r - 8 for _, cy, r in knobs) - Y_OFF, max(cy for _, cy, _ in knobs) - Y_OFF) if knobs else None
                for comp in comps:
                    ty = comp["componentData"]["type"]
                    # not: pictures (meters and engine-driven pictures included: CTRL Duck's SENDER banner spans the page), the
                    # full-page close layers, and the parts shown only while a menu is open
                    if ty == "Image" or ty.startswith(("shMeter", "shBackdrop", "shPopOpt", "shPopPanel", "shOffHead")):
                        continue
                    ps = refs(comp)
                    if not ps or ps & own:
                        continue
                    bx, by, bw, bh = (int(float(v)) for v in comp["bounds"]["bounds"].split())
                    # a switch stacked between the column's own knobs is inside its ring (CTRL MixBus's INPUT columns)
                    if ty.startswith("shToggle") and span and span[0] < by + bh / 2 < span[1]:
                        continue
                    x0, y0, x1, y1 = box
                    if not (bx < x1 and bx + bw > x0 and by < y1 and by + bh > y0):
                        continue
                    if min(x1 - bx, bx + bw - x0) <= min(y1 - by, by + bh - y0):   # move the nearer edge
                        if bx > (x0 + x1) / 2:
                            box[2] = bx - GAP
                        else:
                            box[0] = bx + bw + GAP
                    elif by > (y0 + y1) / 2:
                        box[3] = by - GAP
                    else:
                        box[1] = by + bh + GAP
                n += 1
            rects.append("%d %d %d %d" % (box[0], box[1], box[2] - box[0], box[3] - box[1]) if box else "0 0 0 0")
        tab["qlinkBoundsData"] = rects
    return n
