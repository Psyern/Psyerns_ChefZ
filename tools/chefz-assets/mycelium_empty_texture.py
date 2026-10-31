r"""Textur des ungeimpften Brutbeutels: MyceliumBag_co.paa ohne die weissen Myzelfaeden.

    py -3 -I tools/chefz-assets/mycelium_empty_texture.py

Die Textur ist ein Atlas. Helle Pixel mitten im braunen Korn sind die Faeden und werden
zum Mittelwert des Korns in ihrer Umgebung; die grossen hellen Flaechen sind der Beutel
und bleiben, weil dort kaum Braun in der Naehe ist.
"""
import os
import subprocess
import tempfile

import numpy as np
from PIL import Image, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
IMAGE_TO_PAA = r"C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe"
SRC = os.path.join(ROOT, "ChefZ", "ChefZ_Devices", "mycelium", "data", "MyceliumBag_co.paa")
TARGETS = [os.path.join(ROOT, "ChefZ", "ChefZ_Devices", "mycelium", "data"),
           os.path.join(ROOT, "ChefZ_Core", "Addons", "ChefZ_Devices", "mycelium", "data")]
NAME = "MyceliumBag_empty_co"


def blur(values, radius):
    img = Image.fromarray(np.uint8(np.clip(values, 0, 255)))
    return np.asarray(img.filter(ImageFilter.GaussianBlur(radius)), dtype=np.float32)


with tempfile.TemporaryDirectory() as tmp:
    png = os.path.join(tmp, "src.png")
    subprocess.run([IMAGE_TO_PAA, SRC, png], check=True, capture_output=True)
    rgba = np.asarray(Image.open(png).convert("RGBA"), dtype=np.float32)
    rgb = rgba[..., :3]

    bright = rgb.min(axis=2)
    saturation = rgb.max(axis=2) - bright
    grain = (saturation > 35) & (rgb.mean(axis=2) < 150)
    grain_share = blur(grain * 255.0, 6) / 255.0
    threads = (bright > 95) & (saturation < 70) & (grain_share > 0.18)

    # Mittelfarbe nur aus dem Korn, damit die Faeden nicht in ihr Ersatz bluten.
    weight = np.maximum(blur(grain * 255.0, 8), 1.0)
    fill = np.stack([blur(rgb[..., c] * grain, 8) / weight * 255.0 for c in range(3)], axis=2)
    soft = blur(threads * 255.0, 1)[..., None] / 255.0
    out = rgb * (1.0 - soft) + np.clip(fill, 0, 255) * 0.75 * soft
    result = np.concatenate([np.clip(out, 0, 255), rgba[..., 3:]], axis=2).astype(np.uint8)
    print("Faeden: %.1f%% der Pixel" % (100.0 * threads.mean()))

    target_png = os.path.join(tmp, NAME + ".png")
    Image.fromarray(result, "RGBA").save(target_png)
    Image.fromarray(result, "RGBA").save(os.path.join(os.environ.get("CHEFZ_PREVIEW_DIR", tmp), NAME + ".png"))
    for target in TARGETS:
        subprocess.run([IMAGE_TO_PAA, target_png, os.path.join(target, NAME + ".paa")], check=True, capture_output=True)
        print(os.path.join(target, NAME + ".paa"))
