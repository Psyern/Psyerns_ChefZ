r"""Sporentuetchen: eine Textur je Pilzart aus der Tuetchen-Textur der Meshy-Lieferung und den
Etikett-Renderings derselben Lieferung (deliveries/2026-10-30_meshy_mycelium_pepper.json).

    py -3 -I tools/chefz-assets/spore_labels.py

Die Tuetchen-UV hat zwei Papierflaechen: rechts die Vorderseite (Zeichnung aufrecht), links die
Rueckseite (Zeichnung um 180 Grad gedreht, gemessen am Rueck-Rendering). Je Art:
  1. die Champignon-Zeichnung beider Seiten mit Papier aus derselben Flaeche uebermalen
     (Ausschnitt ober- bzw. unterhalb, weiche Kante),
  2. die Zeichnung der Art aus ihrem Etikett-Rendering ausschneiden: Tinte = dunkler als das
     umgebende Papier (Papier = stark geglaettete Helligkeit), nur im Mittelfeld des Tuetchens,
  3. sie in die Zeichnungsflaeche einpassen und als Tinte (Farbe der Original-Zeichnung) auftragen,
  4. 512 x 512 -> ImageToPAA (Exit-Code geprueft) nach ChefZ/ und ChefZ_Core/Addons/ (identisch).
Agaricus behaelt die Original-Textur (sie zeigt bereits Champignons).
"""
import os
import subprocess
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
SRC = r"C:\Users\Administrator\Downloads\Neue 3D Assets\Meshy_AI_Mushroom_Packet_20261030_texture_6842cf0a"
BASE = os.path.join(SRC, "Meshy_AI_Mushroom_Packet_1030150011_texture.png")
IMAGE_TO_PAA = r"C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe"
WORK = 1024
OUT = 512
TARGETS = [os.path.join(ROOT, "ChefZ", "ChefZ_Devices", "mycelium", "data"),
           os.path.join(ROOT, "ChefZ_Core", "Addons", "ChefZ_Devices", "mycelium", "data")]

# Art -> Etikett-Rendering der Lieferung (Nummer im Dateinamen)
LABELS = {
    "Agaricus": None,
    "Boletus": "Vintage porcini mushroom inventory sachet-2.png",
    "Pleurotus": "Vintage oyster mushroom sachet-3.png",
    "Lactarius": "Vintage milk cap mushroom sachet-4.png",
    "Macrolepiota": "Vintage parasol mushroom packet-5.png",
    "Auricularia": "Vintage Wood Ear Mushroom Sachet-6.png",
    "Craterellus": "Vintage black trumpet mushroom packet-7.png",
    "Chanterelle": "Vintage Chanterelle Sachet-8.png",
    "Psilocybe": "Vintage engraved mushroom sachet-9.png",
    "Hedgehog": "Vintage Hedgehog Mushroom Sachet-10.png",
    "Amanita": "Vintage fly agaric mushroom sachet-11.png",
    "Morel": "Vintage morel mushroom inventory sachet-12.png",
    "ShaggyInkCap": "Vintage shaggy ink cap sachet-14.png",
}

# Zeichnungsflaechen auf der 1024er Textur (gemessen): x0, y0, x1, y1, Papierquelle (dy), 180 Grad?
FRONT = (600, 520, 935, 752, -250, False)
BACK = (100, 463, 435, 702, +270, True)


def box_blur(a, r):
    """Kastenfilter ueber Summenbilder, dreimal = annaehernd Gauss (PIL blurt keine float-Bilder)."""
    for _ in range(3):
        p = np.pad(a, ((r + 1, r), (r + 1, r)), mode="edge")
        c = p.cumsum(0).cumsum(1)
        k = 2 * r + 1
        a = (c[k:, k:] - c[:-k, k:] - c[k:, :-k] + c[:-k, :-k]) / (k * k)
    return a


def feather(h, w, ramp):
    y = np.minimum(np.arange(h), np.arange(h)[::-1])[:, None]
    x = np.minimum(np.arange(w), np.arange(w)[::-1])[None, :]
    return np.clip(np.minimum(x, y) / float(ramp), 0.0, 1.0)


def erase(img, box):
    x0, y0, x1, y1, dy, _rot = box
    pad = 14
    x0, y0, x1, y1 = x0 - pad, y0 - pad, x1 + pad, y1 + pad
    patch = img[y0 + dy:y1 + dy, x0:x1].copy()
    m = feather(y1 - y0, x1 - x0, pad)[..., None]
    img[y0:y1, x0:x1] = img[y0:y1, x0:x1] * (1 - m) + patch * m


def extract(label_path):
    """Tintenmaske (0..1) und Ausschnitt der Zeichnung aus einem Etikett-Rendering."""
    im = np.asarray(Image.open(label_path).convert("RGB"), dtype=np.float32) / 255.0
    lum = im.mean(2)
    paper = box_blur(lum, 28)
    alpha = np.clip((paper - lum - 0.05) / 0.30, 0.0, 1.0)
    h, w = lum.shape
    win = np.zeros_like(alpha)
    # Mittelfeld: die Seitennaehte des Tuetchens liegen bei rund 27 und 77 Prozent der Bildbreite,
    # die Zeichnung zwischen 35 und 70 Prozent - das Fenster bleibt innerhalb der Naehte.
    win[int(h * 0.33):int(h * 0.86), int(w * 0.29):int(w * 0.75)] = 1.0
    alpha *= win
    # Papierflecken und Knitterschatten sind einzelne Punkte, die Zeichnung ist dicht: nur was in
    # dichter Umgebung liegt, bleibt Tinte und bestimmt den Ausschnitt.
    dense = box_blur(alpha, 10) > 0.12
    near = box_blur(dense.astype(np.float32), 6) > 0.01
    alpha *= near
    ys, xs = np.nonzero(dense)
    y0, y1 = int(np.percentile(ys, 0.5)) - 6, int(np.percentile(ys, 99.5)) + 7
    x0, x1 = int(np.percentile(xs, 0.5)) - 6, int(np.percentile(xs, 99.5)) + 7
    return alpha[max(y0, 0):y1, max(x0, 0):x1]


def stamp(img, alpha, box, ink):
    x0, y0, x1, y1, _dy, rot = box
    bw, bh = x1 - x0, y1 - y0
    h, w = alpha.shape
    s = min(bw / w, bh / h)
    nw, nh = max(1, int(w * s)), max(1, int(h * s))
    a = np.asarray(Image.fromarray((alpha * 255).astype(np.uint8)).resize((nw, nh), Image.LANCZOS), dtype=np.float32) / 255.0
    if rot:
        a = a[::-1, ::-1]
    ox = x0 + (bw - nw) // 2
    oy = y0 + (bh - nh) // 2
    if rot:
        oy = y0 + (bh - nh) - (bh - nh) // 2
    region = img[oy:oy + nh, ox:ox + nw]
    img[oy:oy + nh, ox:ox + nw] = region * (1 - a[..., None]) + ink * a[..., None]


def to_paa(rgb, name):
    png = os.path.join(os.environ.get("TEMP", "."), f"{name}_co.png")
    Image.fromarray((np.clip(rgb, 0, 1) * 255 + 0.5).astype(np.uint8), "RGB").resize((OUT, OUT), Image.LANCZOS).save(png)
    results = []
    for t in TARGETS:
        os.makedirs(t, exist_ok=True)
        dst = os.path.join(t, f"{name}_co.paa")
        proc = subprocess.run([IMAGE_TO_PAA, png, dst], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        results.append(proc.returncode == 0 and os.path.isfile(dst))
    return png, all(results)


def main():
    base = np.asarray(Image.open(BASE).convert("RGB").resize((WORK, WORK), Image.LANCZOS), dtype=np.float32) / 255.0
    x0, y0, x1, y1 = FRONT[:4]
    region = base[y0:y1, x0:x1].reshape(-1, 3)
    ink = np.median(region[region.mean(1) < 0.30], axis=0)
    ok = True
    for species, label in LABELS.items():
        img = base.copy()
        if label:
            erase(img, FRONT)
            erase(img, BACK)
            alpha = extract(os.path.join(SRC, label))
            stamp(img, alpha, FRONT, ink)
            stamp(img, alpha, BACK, ink)
        png, good = to_paa(img, f"Spores{species}")
        print(f"Spores{species:14s} {'OK' if good else 'FAILED'}  {png}")
        ok = ok and good
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
