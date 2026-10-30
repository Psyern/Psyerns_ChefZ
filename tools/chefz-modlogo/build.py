# build.py - das ChefZ-Symbol fuer die Mod-Leiste im DayZ-Hauptmenue.
#
#   python tools/chefz-modlogo/build.py
#
# Zeichnet das Emblem aus dem Banner (Kochmuetze ueber gekreuztem Messer und
# Gabel) als Vektor neu und schreibt zwei PAA nach
# ChefZ_Core/Addons/ChefZ_Core/data/gui/:
#
#   chefz_logo_ca.paa        weiss  - CfgMods logo / logoSmall / picture
#   chefz_logo_over_ca.paa   rot    - CfgMods logoOver (Maus darueber)
#
# --- Warum so ---------------------------------------------------------------
# Vanilla laedt das Symbol in ModsMenuSimpleEntry.LoadData() per
# ImageWidget.LoadImageFile(0, GetLogo()) und (1, GetLogoOver())
# (scripts - 1.30/5_Mission/DayZ/GUI/NewUI/ModsMenu/ModsMenuSimpleEntry.c:32).
# Expansion legt dafuer ein Imageset an (Core/Scripts/config.cpp:22-25,
# "set:expansion_mod_imageset image:mod_c", 256x256 je Kachel); ein direkter
# Dateipfad funktioniert genauso (LBmaster_Groups/config.cpp:26-28, VPP).
# Fuer ein einzelnes Symbol ist der Pfad einfacher.
#
# 256 x 256, Zweierpotenz (sonst weisser Kasten). Suffix _ca = mit Alpha,
# ImageToPAA waehlt DXT5. Aus Ausschnitten des Banners wird nichts: das
# Emblem ist dort nur ~70 px gross. Gezeichnet wird 4-fach und verkleinert,
# das glaettet die Kanten.
#
# Exit 1, wenn ImageToPAA fehlt, eine Datei nicht entsteht oder der Header
# nicht DXT5 256x256 ist.

import os
import shutil
import struct
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
OUT = os.path.join(REPO, 'ChefZ_Core', 'Addons', 'ChefZ_Core', 'data', 'gui')
PREVIEW = os.path.join(REPO, 'tools', 'chefz-modlogo', 'preview')
IMAGETOPAA = r'C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe'

SIZE = 256
SS = 4
PAA_DXT5 = 0xFF05

WHITE = (236, 236, 236, 255)
# Das Rot des Banner-"Z".
RED = (200, 32, 42, 255)

VARIANTS = [
    ('chefz_logo_ca', WHITE),
    ('chefz_logo_over_ca', RED),
]


def paa_size(path):
    """(typ, breite, hoehe) der groessten Mipmap - wie chefz-cookbook-art."""
    data = open(path, 'rb').read(4096)
    typ = struct.unpack_from('<H', data, 0)[0]
    pos = 2
    while data[pos:pos + 4] == b'GGAT':
        length = struct.unpack_from('<I', data, pos + 8)[0]
        pos += 12 + length
    palette = struct.unpack_from('<H', data, pos)[0]
    pos += 2 + 3 * palette
    w, h = struct.unpack_from('<HH', data, pos)
    return typ, w & 0x7FFF, h


def rotated_rect(cx, cy, length, width, angle_deg, s):
    """Ecken eines um (cx, cy) gedrehten Rechtecks, Einheiten 0..1."""
    import math
    a = math.radians(angle_deg)
    dx = math.cos(a)
    dy = math.sin(a)
    px = -dy
    py = dx
    hl = length / 2
    hw = width / 2
    pts = []
    for sl, sw in ((-hl, -hw), (hl, -hw), (hl, hw), (-hl, hw)):
        pts.append(((cx + dx * sl + px * sw) * s, (cy + dy * sl + py * sw) * s))
    return pts


def along(cx, cy, angle_deg, t):
    import math
    a = math.radians(angle_deg)
    return cx + math.cos(a) * t, cy + math.sin(a) * t


def draw_emblem(color):
    s = SIZE * SS
    img = Image.new('RGBA', (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    def circle(cx, cy, r):
        d.ellipse([(cx - r) * s, (cy - r) * s, (cx + r) * s, (cy + r) * s], fill=color)

    # --- Kochmuetze: drei Wolkenbaeusche, Koerper, abgesetztes Band ---------
    circle(0.33, 0.27, 0.115)
    circle(0.50, 0.20, 0.135)
    circle(0.67, 0.27, 0.115)
    d.rectangle([0.30 * s, 0.27 * s, 0.70 * s, 0.43 * s], fill=color)
    d.rounded_rectangle([0.30 * s, 0.455 * s, 0.70 * s, 0.53 * s], radius=0.015 * s, fill=color)

    # --- Gekreuzt: Messer von links unten, Gabel von rechts unten -----------
    cx = 0.50
    cy = 0.745
    # Messer: Griff unten links, Klinge nach oben rechts.
    ka = -45
    d.polygon(rotated_rect(*along(cx, cy, ka, -0.20), 0.15, 0.055, ka, s), fill=color)
    blade = [along(cx, cy, ka, -0.12), along(cx, cy, ka, 0.25)]
    import math
    a = math.radians(ka)
    nx = -math.sin(a)
    ny = math.cos(a)
    b0 = blade[0]
    b1 = blade[1]
    poly = [
        (b0[0] - nx * 0.034, b0[1] - ny * 0.034),
        (b1[0] - nx * 0.010, b1[1] - ny * 0.010),
        (b1[0] + nx * 0.040, b1[1] + ny * 0.040),
        (b0[0] + nx * 0.034, b0[1] + ny * 0.034),
    ]
    d.polygon([(x * s, y * s) for x, y in poly], fill=color)

    # Gabel: Griff unten rechts, Zinken nach oben links.
    ga = -135
    d.polygon(rotated_rect(*along(cx, cy, ga, -0.10), 0.38, 0.045, ga, s), fill=color)
    head = along(cx, cy, ga, 0.10)
    d.polygon(rotated_rect(head[0], head[1], 0.05, 0.11, ga, s), fill=color)
    a = math.radians(ga)
    nx = -math.sin(a)
    ny = math.cos(a)
    for off in (-0.04, 0.0, 0.04):
        bx = head[0] + nx * off
        by = head[1] + ny * off
        tx, ty = along(bx, by, ga, 0.10)
        d.polygon(rotated_rect((bx + tx) / 2, (by + ty) / 2, 0.13, 0.022, ga, s), fill=color)

    return img.resize((SIZE, SIZE), Image.LANCZOS)


def main():
    if not os.path.isfile(IMAGETOPAA):
        print(f'  X ImageToPAA fehlt: {IMAGETOPAA}')
        return 1

    os.makedirs(OUT, exist_ok=True)
    os.makedirs(PREVIEW, exist_ok=True)
    failed = []
    tmp = tempfile.mkdtemp(prefix='chefz_modlogo_')
    try:
        for name, color in VARIANTS:
            img = draw_emblem(color)
            img.save(os.path.join(PREVIEW, name + '.png'))
            png = os.path.join(tmp, name + '.png')
            img.save(png)

            dst = os.path.join(OUT, name + '.paa')
            run = subprocess.run([IMAGETOPAA, png, dst], capture_output=True, text=True)
            if run.returncode != 0 or not os.path.isfile(dst):
                failed.append(f'{name}: ImageToPAA ohne Ergebnis ({run.returncode}) {run.stdout.strip()} {run.stderr.strip()}')
                continue

            typ, w, h = paa_size(dst)
            if typ != PAA_DXT5 or (w, h) != (SIZE, SIZE):
                failed.append(f'{name}: PAA-Header typ=0x{typ:04X} {w}x{h}, erwartet DXT5 {SIZE}x{SIZE}')
                continue
            print(f'  ok {os.path.relpath(dst, REPO)}  ({os.path.getsize(dst)} B)')
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    for f in failed:
        print(f'  X {f}')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
