#!/usr/bin/env python3
"""The MPC OS 3.10.1 parts of a release (run by tools/make_release.sh after mpc-vst-plugins' release.py).

    tools/vst2_zips.py <mpc-vst-plugins> dist/<Name>-<ver>-mpc-aarch64.zip [dist/<Name>-<ver>-mpc-armv7.zip]

From MPC OS 3.10.1, MPC lists a plugin from <drive>/VST2 Plugins/<Vendor> - VST - <Name>/ when that folder has a plugin.json
(Akai's plugin folder format; release.py writes it into the zip's portable/ folder). For each release zip this:
  1. adds "format": "vst2" to that plugin.json (and its line in SHA256SUMS). Akai's SDK lists the key as required
     (docs/plugin-folder-format.md, schemas/plugin.schema.json); release.py at 20f91bf leaves it out. Every other entry of the
     zip is copied unchanged.
  2. checks the zip with mpc-vst-plugins' tools/catalog_check.py --catalog (stops on an error).
  3. writes the folder-only zip next to it, <Name>-<ver>-Gen2.zip (aarch64) or -Gen1.zip (armv7), with tools/folder_zip.py:
     its only entry is the plugin folder, which 3.10.1 users copy into VST2 Plugins as it is.
Standard library only.
"""
import hashlib
import json
import os
import re
import subprocess
import sys
import zipfile


def add_format(path):
    with zipfile.ZipFile(path) as z:
        infos = z.infolist()
        data = {i.filename: z.read(i) for i in infos}
    hits = [n for n in data if n.endswith("/plugin.json") and n.split("/")[1:2] == ["portable"] and n.count("/") == 3]
    if len(hits) != 1:
        raise SystemExit("%s: expected one portable/<folder>/plugin.json, found %r" % (path, hits))
    pj = json.loads(data[hits[0]])
    if pj.get("format") == "vst2":
        return "already there"
    if "format" in pj:
        raise SystemExit("%s: plugin.json format is %r, not vst2" % (path, pj["format"]))
    out = {}
    for k, v in pj.items():                      # the key after "version", as in the SDK's own plugin.json files
        out[k] = v
        if k == "version":
            out["format"] = "vst2"
    out.setdefault("format", "vst2")
    data[hits[0]] = (json.dumps(out, indent=2) + "\n").encode()
    top = hits[0].split("/")[0]                 # <Name>-<ver>/: SHA256SUMS there lists every file by its path below it
    sums = top + "/SHA256SUMS"
    if sums not in data:
        raise SystemExit("%s: no %s" % (path, sums))
    rel, lines, n = hits[0][len(top) + 1:], [], 0
    for line in data[sums].decode().splitlines():
        h, sep, f = line.partition("  ")
        if sep and f == rel:
            line, n = "%s  %s" % (hashlib.sha256(data[hits[0]]).hexdigest(), f), n + 1
        lines.append(line)
    if n != 1:
        raise SystemExit("%s: SHA256SUMS lists %s %d times" % (path, rel, n))
    data[sums] = ("\n".join(lines) + "\n").encode()
    tmp = path + ".tmp"
    with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as o:
        for i in infos:                          # the same names, dates, modes and order
            o.writestr(i, data[i.filename])
    os.replace(tmp, path)
    return "added"


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    up, zips = sys.argv[1], sys.argv[2:]
    for z in zips:
        m = re.fullmatch(r"(.*)-mpc-(aarch64|armv7)\.zip", z)
        if not m:
            raise SystemExit("%s: not a <Name>-<ver>-mpc-<aarch64|armv7>.zip release zip" % z)
        print("%s: plugin.json format vst2 %s" % (z, add_format(z)))
        subprocess.run([sys.executable, os.path.join(up, "tools", "catalog_check.py"), "--catalog", z], check=True)
        dst = "%s-%s.zip" % (m.group(1), "Gen2" if m.group(2) == "aarch64" else "Gen1")
        subprocess.run([sys.executable, os.path.join(up, "tools", "folder_zip.py"), z, dst], check=True)
        print("%s: the plugin folder only, for VST2 Plugins (MPC OS 3.10.1 and later)" % dst)


if __name__ == "__main__":
    main()
