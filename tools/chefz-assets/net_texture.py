r"""Kachelbare Netz-Textur mit Alpha-Kanal (_ca) - fuer Reusen, Fischnetze, Keschern.

    py -3 -I tools/chefz-assets/net_texture.py <zielordner> [--cells 8] [--size 1024] [--name fishnet]

Wie Vanillas Clutter-Gras (DZ/plants/clutter, renderFlags "AlphaTest32"): die Geometrie ist eine
flache Flaeche, das Netz entsteht nur durch den Alpha-Kanal. Erzeugt:
  <name>_ca.png / .paa   RGB = Garnfarbe, ueber die Raender hinaus ausgeblutet (keine Saeume in den
                         Mipmaps), Alpha = Garn 255, Masche 0, Kante weich ueber ~1.5 px
  <name>_nohq.png / .paa Normal Map (Garn rund, Verdrillung), fuer Stage1 im rvmat
  <name>.rvmat           Material wie Vanillas fishnettrap.rvmat (Super-Shader, Normal Map)
  <name>_preview.png     Vorschau auf Wasserfarbe

Rautenmasche (wie eine Reuse): zwei Garnscharen unter +-45 Grad, Knoten an jeder Kreuzung,
verdrilltes Garn, Farbe verwittertes Hanf-/Nylongarn mit Schmutz. Kachelt nahtlos, solange --cells
ganzzahlig ist: die Linien liegen bei (x +- y) = k / cells.
"""
import argparse
import os
import subprocess

import numpy as np
from PIL import Image

IMAGE_TO_PAA = r"C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe"


def box_blur(a, r):
    """Kachelnder Kastenfilter (wrap), dreimal ~ Gauss."""
    for _ in range(3):
        acc = np.zeros_like(a)
        for d in range(-r, r + 1):
            acc += np.roll(a, d, axis=0)
        a = acc / (2 * r + 1)
        acc = np.zeros_like(a)
        for d in range(-r, r + 1):
            acc += np.roll(a, d, axis=1)
        a = acc / (2 * r + 1)
    return a


def tile_noise(size, cell, rng):
    n = max(2, size // cell)
    g = rng.random((n, n)).astype(np.float32)
    img = np.asarray(Image.fromarray((g * 255).astype(np.uint8)).resize((size, size), Image.BICUBIC), dtype=np.float32) / 255
    return box_blur(img, max(1, cell // 4))


def build(size, cells, twine_px, knot, seed):
    rng = np.random.default_rng(seed)
    y, x = np.mgrid[0:size, 0:size].astype(np.float32) / size
    period = 1.0 / cells
    alpha = np.zeros((size, size), np.float32)
    height = np.zeros((size, size), np.float32)
    shade = np.zeros((size, size), np.float32)
    for sign in (+1, -1):
        s = (x + sign * y) / period                       # Querkoordinate in Maschen
        across = (np.abs(((s + 0.5) % 1.0) - 0.5) * period) * size / np.sqrt(2.0)   # Abstand zur Linie in px
        along = (x - sign * y) * size / np.sqrt(2.0)       # Laengskoordinate in px
        # leichtes Durchhaengen/Wellen, damit die Linien nicht wie gezeichnet wirken
        wob = 0.6 * np.sin(along / size * 2 * np.pi * cells * 2 + sign)
        d = np.abs(across + wob)
        r = twine_px / 2.0
        cov = np.clip((r + 0.75 - d) / 1.5, 0, 1)
        h = np.sqrt(np.clip(1 - (d / max(r, 1e-3)) ** 2, 0, 1))
        twist = 0.5 + 0.5 * np.sin((along * 0.9 + d * 2.2 * sign) * (2 * np.pi / (twine_px * 1.6)))
        upd = cov > alpha
        alpha = np.maximum(alpha, cov)
        height = np.where(upd, h, height)
        shade = np.where(upd, twist, shade)
    # Knoten an den Kreuzungen
    u = (x + y) / period
    v = (x - y) / period
    du = (np.abs(((u + 0.5) % 1.0) - 0.5)) * period * size / np.sqrt(2)
    dv = (np.abs(((v + 0.5) % 1.0) - 0.5)) * period * size / np.sqrt(2)
    dk = np.sqrt(du ** 2 + dv ** 2)
    kr = twine_px * knot / 2.0
    kcov = np.clip((kr + 0.75 - dk) / 1.5, 0, 1)
    kh = np.sqrt(np.clip(1 - (dk / kr) ** 2, 0, 1))
    upd = kcov > 0.5
    alpha = np.maximum(alpha, kcov)
    height = np.where(upd, np.maximum(height, kh * 1.15), height)
    shade = np.where(upd, 0.35 + 0.3 * kh, shade)

    # Farbe: verwittertes Garn, olivbraun, mit Schmutz und Ausbleichen
    base = np.array([0.46, 0.43, 0.32], np.float32)
    var = tile_noise(size, size // 6, rng)[..., None]
    dirt = tile_noise(size, size // 20, rng)[..., None]
    col = base * (0.80 + 0.35 * var)
    col = col * (1 - 0.35 * np.clip((dirt - 0.55) * 4, 0, 1)) + np.array([0.20, 0.17, 0.12]) * 0.35 * np.clip((dirt - 0.55) * 4, 0, 1)
    light = (0.55 + 0.45 * height) * (0.78 + 0.22 * shade)
    rgb = np.clip(col * light[..., None], 0, 1)

    # Farbe in die Maschen ausbluten (gegen Saeume in den Mipmaps)
    w = alpha.copy()
    acc = rgb * w[..., None]
    for rr in (2, 4, 8, 16):
        bw = box_blur(w, rr)
        ba = np.stack([box_blur(acc[..., c], rr) for c in range(3)], 2)
        fill = ba / np.maximum(bw[..., None], 1e-4)
        mask = (w < 0.5)[..., None]
        rgb = np.where(mask & (bw[..., None] > 1e-3), fill, rgb)

    # Normal Map aus der Hoehe (kachelnd)
    hgt = height * 3.0
    dx = (np.roll(hgt, -1, 1) - np.roll(hgt, 1, 1)) * 0.5
    dy = (np.roll(hgt, -1, 0) - np.roll(hgt, 1, 0)) * 0.5
    n = np.stack([-dx, dy, np.ones_like(dx)], 2)
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    nrm = (n * 0.5 + 0.5)
    return rgb, alpha, nrm


RVMAT = """// {name}.rvmat - Netz-Material, erzeugt von tools/chefz-assets/net_texture.py.
// Aufbau 1:1 wie Vanillas DZ/gear/traps/data/fishnettrap.rvmat: Super-Shader, KEINE renderFlags.
// Der Alpha-Ausschnitt kommt allein aus der _ca-Grundfarbe ({name}_ca.paa), die das Modell selbst
// traegt (Face-Textur bzw. hiddenSelectionsTextures) - genau so ist Vanillas Fischnetzfalle gebaut.
// Stage1 ist die Normal Map des Garns, die uebrigen Stufen sind Vanillas Neutralwerte.
ambient[]={{1,1,1,1}};
diffuse[]={{1,1,1,1}};
forcedDiffuse[]={{0,0,0,0}};
emmisive[]={{0,0,0,1}};
specular[]={{0.2,0.2,0.2,1}};
specularPower=30;
PixelShaderID="Super";
VertexShaderID="Super";
"""
STAGES = [r"{path}\{name}_nohq.paa",
          "#(argb,8,8,3)color(0.5,0.5,0.5,0.5,DT)",
          "#(argb,8,8,3)color(0,0,0,0,MC)",
          "#(argb,8,8,3)color(1,1,1,1,AS)",
          "#(argb,8,8,3)color(0,0.01,1,0,SMDI)",
          "#(ai,32,128,1)fresnel(1.16,0.22)",
          r"dz\data\data\env_cloth_green_co.paa"]

STAGE = """class Stage{k}
{{
	texture="{tex}";
	uvSource="tex";
	class uvTransform
	{{
		aside[]={{1,0,0}};
		up[]={{0,1,0}};
		dir[]={{0,0,0}};
		pos[]={{0,0,0}};
	}};
}};
"""


def rvmat_text(name, path):
    out = RVMAT.format(name=name)
    for k, tex in enumerate(STAGES, start=1):
        out += STAGE.format(k=k, tex=tex.format(name=name, path=path))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--size", type=int, default=1024)
    ap.add_argument("--cells", type=int, default=8)
    ap.add_argument("--twine", type=float, default=6.0, help="Garndicke in px bei --size")
    ap.add_argument("--knot", type=float, default=1.9, help="Knotendurchmesser als Vielfaches der Garndicke")
    ap.add_argument("--name", default="fishnet")
    ap.add_argument("--prefix", default="ChefZ\\ChefZ_Items\\data", help="Pfad im rvmat")
    ap.add_argument("--seed", type=int, default=3)
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    rgb, alpha, nrm = build(a.size, a.cells, a.twine, a.knot, a.seed)
    ca = np.dstack([rgb, alpha])
    ca_png = os.path.join(a.out, f"{a.name}_ca.png")
    Image.fromarray((ca * 255 + 0.5).astype(np.uint8), "RGBA").save(ca_png)
    nohq_png = os.path.join(a.out, f"{a.name}_nohq.png")
    Image.fromarray((nrm * 255 + 0.5).astype(np.uint8), "RGB").save(nohq_png)
    for png in (ca_png, nohq_png):
        r = subprocess.run([IMAGE_TO_PAA, png, png[:-4] + ".paa"], stdout=subprocess.PIPE)
        print(os.path.basename(png[:-4] + ".paa"), "OK" if r.returncode == 0 else "FAILED")
    with open(os.path.join(a.out, f"{a.name}.rvmat"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write(rvmat_text(a.name, a.prefix))
    # Vorschau: 2x2 gekachelt ueber Wasserfarbe, daneben der Alpha-Kanal
    tiled = np.tile(ca, (2, 2, 1))
    water = np.zeros_like(tiled[..., :3]) + np.array([0.18, 0.27, 0.26])
    comp = water * (1 - tiled[..., 3:]) + tiled[..., :3] * tiled[..., 3:]
    prev = Image.new("RGB", (a.size * 2, a.size), "white")
    prev.paste(Image.fromarray((comp * 255).astype(np.uint8)).resize((a.size, a.size)), (0, 0))
    prev.paste(Image.fromarray((alpha * 255).astype(np.uint8)).convert("RGB"), (a.size, 0))
    prev.save(os.path.join(a.out, f"{a.name}_preview.png"))
    print("coverage", f"{(alpha > 0.5).mean():.1%}")


if __name__ == "__main__":
    main()
