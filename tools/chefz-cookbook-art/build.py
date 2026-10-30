# build.py - Kochbuchseiten vom Quellbild bis in die config.cpp.
#
#   python tools/chefz-cookbook-art/build.py
#
# 1. ChefZ_Docs/Kochbuch/source/<Ergebnisklasse>.png  ->  <Ergebnisklasse>_co.paa
#    unter ChefZ_Core/Addons/ChefZ_Cookbook_UI/data/pages/ (ImageToPAA, DXT1)
# 2. jede .paa wird am Header geprueft: richtiges Format, 1024 x 1024
# 3. der Block zwischen ">>> PAGES" und "<<< PAGES" in der config.cpp von
#    ChefZ_Cookbook_UI wird neu geschrieben - ein Eintrag je Bild
#
# --- Warum PAA und nicht EDDS ----------------------------------------------
# Der erste Lauf (26.10.2026) schrieb Enfusion-EDDS: BGRA8 mit LZ4. Das
# Notizbuchpapier ist Rauschen, LZ4 bekommt es kaum kleiner - 4,8 MB je
# Seite, 206 MB fuer 43. PAA/DXT1 macht daraus 0,7 MB. ImageWidget laedt PAA
# per LoadImageFile; belegt an LBmaster_Core (LBAppearanceConfig.c:22 mit
# logo.paa), das so auf vielen Servern laeuft.
#
# Exit 1, wenn ImageToPAA fehlt, eine Datei nicht entsteht, der Header nicht
# stimmt oder die Marker fehlen. Ein Bild, das die Engine als weissen Kasten
# zeigt, soll den Lauf anhalten, nicht durchrutschen.

import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..'))
SRC = os.path.join(REPO, 'ChefZ_Docs', 'Kochbuch', 'source')
PROMPTS = os.path.join(REPO, 'ChefZ_Docs', 'Kochbuch', 'prompts.json')
ADDON = os.path.join(REPO, 'ChefZ_Core', 'Addons', 'ChefZ_Cookbook_UI')
OUT = os.path.join(ADDON, 'data', 'pages')
CONFIG = os.path.join(ADDON, 'config.cpp')
PREFIX = 'ChefZ_Cookbook_UI'
IMAGETOPAA = r'C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin\ImageToPAA\ImageToPAA.exe'

# Hausstandard fuer Gerichte (Memory chefz-textur-pipeline): 1024.
SIZE = 1024
BEGIN = '// >>> PAGES'
END = '// <<< PAGES'
PAA_DXT1 = 0xFF01


def paa_size(path):
    """(typ, breite, hoehe) der groessten Mipmap, aus dem Header gelesen.

    Aufbau: uint16 Typ, dann GGAT-Bloecke (4 B 'GGAT', 4 B Name, uint32
    Laenge, Daten), dann uint16 Palettenlaenge + 3 B je Eintrag, dann die
    erste Mipmap mit uint16 w, uint16 h. Bit 0x8000 in w markiert LZO und
    gehoert nicht zur Breite.
    """
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


def main():
    if not os.path.isfile(IMAGETOPAA):
        print(f'  X ImageToPAA fehlt: {IMAGETOPAA}')
        return 1

    known = {p['key'] for p in json.load(open(PROMPTS, encoding='utf-8'))}
    os.makedirs(OUT, exist_ok=True)

    failed = []
    pages = []
    tmp = tempfile.mkdtemp(prefix='chefz_cookbook_')
    try:
        for name in sorted(os.listdir(SRC)):
            if not name.lower().endswith('.png'):
                continue
            key = name[:-4]
            if key not in known:
                print(f'  ! {name}: kein Gericht dieses Namens in prompts.json - uebersprungen')
                continue

            # Das Suffix _co waehlt in ImageToPAA DXT1 ohne Alpha.
            img = Image.open(os.path.join(SRC, name)).convert('RGB')
            if img.size != (SIZE, SIZE):
                img = img.resize((SIZE, SIZE), Image.LANCZOS)
            png = os.path.join(tmp, key + '_co.png')
            img.save(png)

            dst = os.path.join(OUT, key + '_co.paa')
            run = subprocess.run([IMAGETOPAA, png, dst], capture_output=True, text=True)
            if run.returncode != 0 or not os.path.isfile(dst):
                failed.append(f'{name}: ImageToPAA ohne Ergebnis ({run.returncode}) {run.stdout.strip()} {run.stderr.strip()}')
                continue

            typ, w, h = paa_size(dst)
            if typ != PAA_DXT1 or (w, h) != (SIZE, SIZE):
                failed.append(f'{name}: PAA-Header typ=0x{typ:04X} {w}x{h}, erwartet DXT1 {SIZE}x{SIZE}')
                continue
            pages.append(key)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)

    missing = sorted(known - set(pages))
    write_config(pages)

    total = sum(os.path.getsize(os.path.join(OUT, k + '_co.paa')) for k in pages)
    print(f'{len(pages)} Seiten, {total / 1048576:.1f} MB -> {os.path.relpath(OUT, REPO)}')
    for m in missing:
        print(f'  - ohne Bild: {m}')
    for f in failed:
        print(f'  X {f}')
    return 1 if failed else 0


def write_config(pages):
    text = open(CONFIG, encoding='utf-8').read()
    a = text.find(BEGIN)
    b = text.find(END)
    if a < 0 or b < a:
        print(f'  X Marker {BEGIN} / {END} fehlen in {CONFIG}')
        sys.exit(1)
    line_start = text.rfind('\n', 0, a) + 1
    indent = text[line_start:a]
    head = text[:a] + BEGIN + ' (generiert, nicht von Hand aendern)\n'
    body = ''
    for key in pages:
        body += f'{indent}class {key} {{ image = "{PREFIX}/data/pages/{key}_co.paa"; }};\n'
    tail = indent + text[b:]
    open(CONFIG, 'w', encoding='utf-8', newline='\n').write(head + body + tail)


if __name__ == '__main__':
    sys.exit(main())
