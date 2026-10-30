r"""Vorschau: ein skinned MLOD-Tier mit Vanilla-Animationen des Fuchses posieren und rendern.

    blender -b --factory-startup --python tools/chefz-assets/animal_anim_preview.py -- <job.json>

job.json: {"p3d": ..., "texture_png": ..., "out_dir": ..., "anims": {"walk": [0, 14, 28, 42], "<pfad>.anm": [...]}}

Prueft genau das, was DayZ tut: die .anm traegt je Knochen die LOKALE Transformation relativ zum Eltern-
knochen (DayZ-Raum, y oben, Quaternion x,y,z,w; fehlt eine Spur, gilt die Bindpose aus der .xob). Haut-
matrix je Knochen = animierte Globalmatrix * inverse Bind-Globalmatrix, gemischt mit den Gewichten der
Knochen-Selektionen im p3d (Linear Blend Skinning). Gerendert wird LOD 0.
"""
import json
import math
import os
import struct
import sys

import bpy
from mathutils import Matrix, Quaternion, Vector

SUITE = os.environ.get("PSY_SUITE", r"C:\Users\Administrator\Desktop\Psyerns_3D_Suite_For_Blender")
XOB = r"D:\DayZ_Game_Data\DZ\animals\vulpes_vulpes\data\vulpes_vulpes_skeleton.xob"
ANIMS = r"D:\DayZ_Game_Data\DZ\animals\animations\red_fox_skeleton"


def load_xob():
    b = open(XOB, "rb").read()
    i = 0x50
    names = []
    while len(names) < 35:
        j = b.index(b"\0", i)
        names.append(b[i:j].decode())
        i = j + 1
    rest = b[i:]
    bones = []
    for k in range(35):
        r = rest[k * 36:(k + 1) * 36]
        f = struct.unpack_from("<7f", r, 4)
        nxt, child = struct.unpack_from("<hh", r, 32)
        bones.append({"name": names[k], "t": Vector(f[:3]), "q": Quaternion((f[6], f[3], f[4], f[5])),
                      "next": nxt, "child": child, "parent": -1})
    for k, bo in enumerate(bones):
        c = bo["child"]
        while c != -1:
            bones[c]["parent"] = k
            c = bones[c]["next"]
    return bones


def globals_for(bones, local):
    """local: name -> (t, q) in DayZ space; returns name -> 4x4 global matrix in DayZ space."""
    out = {}
    for bo in bones:   # parents come first in the xob order
        t, q = local.get(bo["name"], (bo["t"], bo["q"]))
        m = Matrix.Translation(t) @ q.to_matrix().to_4x4()
        p = bo["parent"]
        out[bo["name"]] = (out[bones[p]["name"]] @ m) if p >= 0 else m
    return out


def main(job):
    sys.path.insert(0, SUITE)
    from psyerns_3d_suite.core import anm, mlod
    bones = load_xob()
    bind = globals_for(bones, {})
    bind_inv = {k: v.inverted() for k, v in bind.items()}
    lods = mlod.read_mlod(open(job["p3d"], "rb").read())
    lod = [l for l in lods if l.kind.value == "resolution" and l.index == 0][0]
    by_lower = {b["name"].lower(): b["name"] for b in bones}
    weights = [[] for _ in lod.points]
    for sname, sel in lod.selections.items():
        bn = by_lower.get(sname.lower())
        if not bn:
            continue
        for p in sel.points:
            weights[p].append((bn, sel.weight(p)))
    unweighted = sum(1 for w in weights if not w)
    print("POINTS", len(lod.points), "unweighted", unweighted)

    sc = bpy.context.scene
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    sc.render.engine = "CYCLES"
    sc.cycles.device = "CPU"
    sc.cycles.samples = 16
    sc.world = bpy.data.worlds.new("w")
    sc.world.use_nodes = True
    sc.world.node_tree.nodes["Background"].inputs[0].default_value = (0.8, 0.82, 0.88, 1)
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
    sc.collection.objects.link(sun)
    sun.rotation_euler = (0.8, 0.2, 0.5)
    mat = bpy.data.materials.new("m")
    mat.use_nodes = True
    n = mat.node_tree.nodes.new("ShaderNodeTexImage")
    n.image = bpy.data.images.load(job["texture_png"])
    mat.node_tree.links.new(n.outputs[0], mat.node_tree.nodes["Principled BSDF"].inputs[0])
    cam_d = bpy.data.cameras.new("c")
    cam_d.type = "ORTHO"
    cam_d.ortho_scale = 1.3
    cam = bpy.data.objects.new("cam", cam_d)
    sc.collection.objects.link(cam)
    sc.camera = cam
    sc.render.resolution_x = 600
    sc.render.resolution_y = 450
    files = []
    for anim_name, frames in job["anims"].items():
        path = anim_name if anim_name.lower().endswith(".anm") else os.path.join(ANIMS, anim_name + ".anm")
        a = anm.read(open(path, "rb").read())
        anim_name = os.path.splitext(os.path.basename(path))[0]
        for fr in frames:
            local = {}
            for ab in a.bones:
                bo = next((b for b in bones if b["name"] == ab.name), None)
                if bo is None:
                    continue
                t = bo["t"]
                q = bo["q"]
                if ab.translations:
                    k = min(ab.translations, key=lambda f: abs(f - fr))
                    t = Vector(ab.translations[k])
                if ab.rotations:
                    k = min(ab.rotations, key=lambda f: abs(f - fr))
                    x, y, z, w = ab.rotations[k]
                    q = Quaternion((w, x, y, z))
                local[ab.name] = (t, q)
            pose = globals_for(bones, local)
            skin = {k: pose[k] @ bind_inv[k] for k in pose}
            pts = []
            for i, p in enumerate(lod.points):
                v = Vector(p)           # P3D == DayZ model space
                acc = Vector((0, 0, 0))
                ws = weights[i] or [("Pelvis", 1.0)]
                tot = sum(w for _, w in ws)
                for bn, w in ws:
                    acc += (skin[bn] @ v) * (w / tot)
                pts.append((acc.x, acc.z, acc.y))      # DayZ -> Blender
            faces = [[fv.point for fv in f.vertices] for f in lod.faces]
            me = bpy.data.meshes.new("m")
            me.from_pydata(pts, [], faces)
            uv = me.uv_layers.new()
            li = 0
            for f in lod.faces:
                for fv in f.vertices:
                    uv.data[li].uv = (fv.u, 1.0 - fv.v)
                    li += 1
            me.materials.append(mat)
            for poly in me.polygons:
                poly.use_smooth = True
            ob = bpy.data.objects.new("pose", me)
            sc.collection.objects.link(ob)
            for tag, d in (("side", (1, 0, 0.05)), ("persp", (0.8, 0.8, 0.5))):
                d = Vector(d).normalized()
                c = Vector((0, 0, 0.25))
                cam.location = c + d * 3
                cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
                f = os.path.join(job["out_dir"], f"{anim_name}_{fr:03d}_{tag}.png")
                sc.render.filepath = f
                bpy.ops.render.render(write_still=True)
                files.append(f)
            bpy.data.objects.remove(ob, do_unlink=True)
    print("FILES", json.dumps(files))


if __name__ == "__main__":
    main(json.load(open(sys.argv[sys.argv.index("--") + 1], encoding="utf-8")))
