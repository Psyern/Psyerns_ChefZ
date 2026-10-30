r"""Vorratsglas-Varianten: neue Inhalte auf dem ChefZ-Glas (ChefZ_Items/models/tomato_sauce.p3d).

    py -3 -I tools/chefz-assets/jar_recolor.py

Das Vanilla-Glas (Marmalade.p3d, Honey.p3d) hat keine Textur-Selektion und ist binarisiert -
eine neue Textur laesst sich dort nicht setzen (geprueft 30.10.2026). Das ChefZ-Glas hat "camo".
Seine Textur ist ein Splitter-Atlas, also wird nicht gemalt, sondern UMGEFAERBT: jedes Pixel,
dessen Farbton im Rot der Tomatensauce liegt, bekommt die Farbe des neuen Inhalts; Helligkeit
und Struktur bleiben. Deckel (oliv) und Glas (grau/braun, wenig gesaettigt) bleiben unberuehrt.
Danach dieselben Garstufen wie jede Stufen-Textur des Projekts (stage-textures.py STATE_TINTS).

Ziel: ChefZ/ChefZ_Items/data und ChefZ_Core/Addons/ChefZ_Items/data, <name>_co.paa + 4 Stufen.
"""
import colorsys
import importlib.util
import os
import subprocess
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(ROOT, "ChefZ_Core", "Addons", "ChefZ_Items", "data", "tomato_sauce_co.paa")
IMAGE_TO_PAA = r"C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe"
TARGETS = [os.path.join(ROOT, "ChefZ", "ChefZ_Items", "data"),
           os.path.join(ROOT, "ChefZ_Core", "Addons", "ChefZ_Items", "data")]
STATES = ["baked", "boiled", "dried", "burnt"]

# name -> (Grundfarbe, Fleckenfarbe, Fleckenanteil, Fleckengroesse px)
VARIANTS = {
    "cream_sauce":          ((0.93, 0.90, 0.80), None, 0.0, 0),
    "mushroom_cream_sauce": ((0.80, 0.69, 0.52), (0.40, 0.28, 0.17), 0.12, 2),
    "cheese_curd":          ((0.95, 0.94, 0.85), (0.80, 0.78, 0.66), 0.35, 4),
}


def paa(src, dst):
    r = subprocess.run([IMAGE_TO_PAA, src, dst], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return r.returncode == 0 and os.path.isfile(dst)


def box_blur(a, r):
    for _ in range(3):
        p = np.pad(a, ((r + 1, r), (r + 1, r)), mode="edge")
        c = p.cumsum(0).cumsum(1)
        k = 2 * r + 1
        a = (c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]) / (k * k)
    return a


def sauce_mask(rgb):
    """0..1: wie sehr ein Pixel Tomatensauce ist (roter Farbton, gesaettigt)."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    mx = rgb.max(2)
    mn = rgb.min(2)
    sat = (mx - mn) / np.maximum(mx, 1e-4)
    # auch die dunklen, weniger gesaettigten Rottoene am Glasrand gehoeren zur Sauce;
    # kein Weichzeichnen - es zog die Maske ueber Inselraender auf den Deckel.
    redness = np.clip((r - np.maximum(g, b) * 1.10) / 0.18, 0, 1)
    return redness * np.clip((sat - 0.22) / 0.20, 0, 1)


def recolor(rgb, base, fleck, share, size, seed):
    lum = rgb.mean(2)
    m = sauce_mask(rgb)
    ref = np.median(lum[m > 0.8]) if (m > 0.8).any() else 0.4
    shade = np.clip(lum / max(ref, 1e-3), 0.55, 1.35)[..., None] ** 0.7
    col = np.array(base)[None, None, :] * shade
    if fleck and share > 0:
        rng = np.random.default_rng(seed)
        noise = box_blur(rng.random(lum.shape).astype(np.float32), size)
        thr = np.quantile(noise, 1 - share)
        f = np.clip((noise - thr) / 0.01, 0, 1)[..., None]
        col = col * (1 - f) + np.array(fleck)[None, None, :] * shade * f
    return np.clip(rgb * (1 - m[..., None]) + col * m[..., None], 0, 1)


def main():
    spec = importlib.util.spec_from_file_location("stage", os.path.join(HERE, "stage-textures.py"))
    stage = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(stage)
    tmp = os.environ.get("TEMP", ".")
    src_png = os.path.join(tmp, "jar_tomato_src.png")
    if subprocess.run([IMAGE_TO_PAA, SRC, src_png], stdout=subprocess.PIPE).returncode != 0:
        raise SystemExit("PAA -> PNG fehlgeschlagen")
    rgb = np.asarray(Image.open(src_png).convert("RGB"), dtype=np.float32) / 255.0
    ok = True
    for k, (name, (base, fleck, share, size)) in enumerate(VARIANTS.items()):
        out = recolor(rgb, base, fleck, share, size, seed=11 + k)
        images = {"": out}
        for st in STATES:
            factor, saturation = stage.STATE_TINTS[st]
            grey = out.mean(2, keepdims=True)
            images["_" + st] = np.clip((grey + (out - grey) * saturation) * np.array(factor), 0, 1)
        for suffix, img in images.items():
            png = os.path.join(tmp, f"{name}{suffix}_co.png")
            Image.fromarray((img * 255 + 0.5).astype(np.uint8), "RGB").save(png)
            for t in TARGETS:
                os.makedirs(t, exist_ok=True)
                good = paa(png, os.path.join(t, f"{name}{suffix}_co.paa"))
                ok = ok and good
        print(name, "OK" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
