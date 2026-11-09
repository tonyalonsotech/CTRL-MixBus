"""The skin's font (Titillium Web, from the mpc-vst-plugins checkout), for the generators that draw text into art/.

Looked up like tools/build_device.sh finds the checkout: $MPC_VST_PLUGINS, else third_party/mpc-vst-plugins (the submodule), else
the sibling ../mpc-vst-plugins. The drawn PNGs are committed, so only a regeneration needs it."""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def font(name="TitilliumWeb-SemiBold.ttf"):
    tried = []
    for base in (os.environ.get("MPC_VST_PLUGINS"), os.path.join(ROOT, "third_party", "mpc-vst-plugins"),
                 os.path.join(os.path.dirname(ROOT), "mpc-vst-plugins")):
        if not base:
            continue
        p = os.path.join(base, "tools", "html_art", "fonts", name)
        if os.path.exists(p):
            return p
        tried.append(p)
    sys.exit("font %s not found (tried %s): set MPC_VST_PLUGINS to the mpc-vst-plugins checkout" % (name, ", ".join(tried)))
