r"""Basisfarb-Textur aus einer GLB (Higgsfield/Meshy image_to_3d) als PNG ablegen.

    py -3 -I tools/chefz-assets/glb_texture.py <datei.glb> [<ziel.png>]

build_delivery_local.py erwartet je Asset ein Farb-PNG neben dem Modell (Feld "texture" im Manifest);
eine GLB traegt es eingebettet. Gelesen wird das baseColorTexture-Bild des ersten Materials.
"""
import json
import os
import struct
import sys


def main(path, out=None):
    data = open(path, "rb").read()
    magic, _version, _length = struct.unpack_from("<III", data, 0)
    if magic != 0x46546C67:
        raise SystemExit("keine GLB")
    off = 12
    js = binc = None
    while off < len(data):
        clen, ctype = struct.unpack_from("<II", data, off)
        chunk = data[off + 8:off + 8 + clen]
        if ctype == 0x4E4F534A:
            js = json.loads(chunk)
        elif ctype == 0x004E4942:
            binc = chunk
        off += 8 + clen
    mat = js["materials"][0]
    tex_index = mat["pbrMetallicRoughness"]["baseColorTexture"]["index"]
    img = js["images"][js["textures"][tex_index]["source"]]
    view = js["bufferViews"][img["bufferView"]]
    blob = binc[view.get("byteOffset", 0):view.get("byteOffset", 0) + view["byteLength"]]
    ext = ".png" if img.get("mimeType", "image/png") == "image/png" else ".jpg"
    out = out or os.path.splitext(path)[0] + "_texture" + ext
    with open(out, "wb") as fh:
        fh.write(blob)
    print(out, len(blob))


if __name__ == "__main__":
    main(*sys.argv[1:])
