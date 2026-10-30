r"""Erzeugt aus einer vorhandenen Farbtextur <name>_co.paa die Garstufen-Kopien
<name>_baked_co.paa, _boiled_co.paa, _dried_co.paa und _burnt_co.paa daneben.

Dieselben Toenungen wie die Lieferstrecke (Psyerns 3D Suite,
tools/import_delivery.py STATE_TINTS), damit alte und neue Stufen gleich
aussehen. Fertige Kopien werden nicht ueberschrieben (--force erzwingt es).

    py -3 tools/chefz-assets/stage-textures.py <a_co.paa> [<b_co.paa> ...] [--force]

Braucht Pillow und numpy sowie DayZ Tools (ImageToPAA) fuer PAA <-> PNG.
"""
import os
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image

IMAGE_TO_PAA = os.path.join(os.environ.get("PSY_DAYZ_TOOLS", r"C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools"), "Bin", "ImageToPAA", "ImageToPAA.exe")

# Farbfaktor (r, g, b) und Saettigung - woertlich aus import_delivery.py
STATE_TINTS = {
    "baked": ((0.82, 0.62, 0.42), 0.85),
    "boiled": ((1.05, 1.02, 0.96), 0.45),
    "dried": ((0.70, 0.60, 0.50), 0.60),
    "burnt": ((0.22, 0.18, 0.16), 0.30),
}


def convert(src, dst):
    subprocess.run([IMAGE_TO_PAA, src, dst], check=True, stdout=subprocess.DEVNULL)
    if not os.path.isfile(dst):
        raise RuntimeError(f"ImageToPAA hat {dst} nicht geschrieben")


def main():
    force = "--force" in sys.argv
    sources = [a for a in sys.argv[1:] if not a.startswith("--")]
    if not sources:
        print(__doc__)
        return 2
    scratch = tempfile.mkdtemp(prefix="chefz_stage_")
    for src in sources:
        if not src.lower().endswith("_co.paa"):
            print(f"uebersprungen (kein _co.paa): {src}")
            continue
        stem = src[: -len("_co.paa")]
        png = os.path.join(scratch, os.path.basename(stem) + "_co.png")
        convert(src, png)
        rgba = np.asarray(Image.open(png).convert("RGBA"), dtype=np.float32) / 255.0
        for state, (factor, saturation) in STATE_TINTS.items():
            out = f"{stem}_{state}_co.paa"
            if os.path.isfile(out) and not force:
                print(f"vorhanden: {out}")
                continue
            data = rgba.copy()
            grey = data[:, :, :3].mean(axis=2, keepdims=True)
            data[:, :, :3] = np.clip((grey + (data[:, :, :3] - grey) * saturation) * np.array(factor), 0.0, 1.0)
            tinted = os.path.join(scratch, os.path.basename(stem) + f"_{state}_co.png")
            Image.fromarray((data * 255.0 + 0.5).astype(np.uint8), "RGBA").save(tinted)
            convert(tinted, out)
            print(f"geschrieben: {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
