r"""Rohsalz-Textur fuer den Salzstreuer: das Salz im Glas grauer, gelblicher und matter.

    py -3 -I tools/chefz-assets/salt_variants.py

Liest die gebaute Streuer-Textur (ChefZ_Core/Addons/ChefZ_Items/saltshaker/data/SaltShaker_co.paa)
und toent nur helle, wenig gesaettigte Pixel - das weisse Salz. Deckel (Blech, dunkler) und
Glasrand bleiben. Schreibt RawSalt_co.paa in beide Baeume (ChefZ/ und ChefZ_Core/Addons/).
"""
import os
import subprocess
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
REL = os.path.join("ChefZ_Items", "saltshaker", "data")
IMAGE_TO_PAA = r"C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe"


def main():
    tmp = os.environ.get("TEMP", ".")
    src = os.path.join(ROOT, "ChefZ_Core", "Addons", REL, "SaltShaker_co.paa")
    png = os.path.join(tmp, "saltshaker_src.png")
    if subprocess.run([IMAGE_TO_PAA, src, png], stdout=subprocess.PIPE).returncode != 0:
        raise SystemExit("PAA -> PNG fehlgeschlagen")
    rgb = np.asarray(Image.open(png).convert("RGB"), dtype=np.float32) / 255.0
    lum = rgb.mean(2)
    sat = (rgb.max(2) - rgb.min(2)) / np.maximum(rgb.max(2), 1e-4)
    salt = np.clip((lum - 0.55) / 0.15, 0, 1) * np.clip((0.25 - sat) / 0.10, 0, 1)
    raw = rgb * np.array([0.80, 0.77, 0.68]) - 0.04
    out = np.clip(rgb * (1 - salt[..., None]) + raw * salt[..., None], 0, 1)
    out_png = os.path.join(tmp, "RawSalt_co.png")
    Image.fromarray((out * 255 + 0.5).astype(np.uint8), "RGB").save(out_png)
    ok = True
    for tree in (os.path.join(ROOT, "ChefZ"), os.path.join(ROOT, "ChefZ_Core", "Addons")):
        dst = os.path.join(tree, REL, "RawSalt_co.paa")
        r = subprocess.run([IMAGE_TO_PAA, out_png, dst], stdout=subprocess.PIPE)
        ok = ok and r.returncode == 0 and os.path.isfile(dst)
    print("RawSalt", "OK" if ok else "FAILED", f"salt pixels {(salt > 0.5).mean():.1%}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
