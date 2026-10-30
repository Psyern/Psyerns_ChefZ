r"""Ein Meshy-Vierbeiner (Meshy "quadruped"-Rig) auf das Fuchs-Skelett von DayZ - als skinned MLOD.

    blender -b --factory-startup --python tools/chefz-assets/animal_retarget_fox.py -- <job.json>

job.json: {"fbx": ..., "texture": "<P3D-Texturpfad>", "out_p3d": ..., "scale": 255,
           "mass": 8.0, "debug_dir": "<optional, Vorschau-PNGs>"}

Warum der Fuchs: ein neues Tier mit eigener Animation braucht in DayZ Skelett (.xob), Animationsgraph
(.agr/.ast/.asi) und binaerisierte .anm aus der Workbench - nichts davon laesst sich hier ohne die
Workbench-GUI erzeugen. Vanillas Animal_VulpesVulpes bringt alles mit (DZ/animals/vulpes_vulpes,
enfanimsys: vulpes_vulpes_skeleton.xob + AmbientLife_Graph.agr + VulpesVulpes_Skeleton.asi, 33 .anm,
KI-Vorlage AmbientLife_Vulpes_Vulpes). Eine Unterklasse erbt das alles; neu ist nur das Mesh. Dafuer muss
es auf den 35 Fuchsknochen in GENAU der Fuchs-Bindpose sitzen.

Ablauf:
  1. Skelett aus der XOB lesen (Namen, Hierarchie ueber naechstes Geschwister/erstes Kind, lokale
     Position + Quaternion -> globale Bindpose; DayZ Y oben, +Z vorn; Blender = (x, z, y)).
  2. FBX laden, um Z 180 Grad drehen (Nase +Y), skalieren.
  3. Umformen per eigenem Linear Blend Skinning: jedes Meshy-Knochensegment wird auf ein Fuchs-
     Segment abgebildet (Drehung + Streckung laengs des Knochens + Verschiebung), gewichtet mit den
     Meshy-Gewichten. Beine, Wirbelsaeule und Schwanz liegen danach in der Fuchs-Bindpose.
  4. Gewichte: Meshy-Gruppen -> Fuchsknochen; Rumpf und Hals werden laengs der Koerperachse weich auf
     Pelvis/Spine1/Spine2/Chest bzw. Neck1/Neck2 verteilt, Pfoten nach Hoehe auf Leg3/Ankle.
     Hoechstens 3 Einfluesse je Punkt, normalisiert.
  5. LODs: Resolution 0/1/2 (Decimate), Fire- und View-Geometry aus Quadern je Koerperteil (Selektionen
     ComponentXX + zone_head/zone_chest/zone_legs + Knochen, starr), Geometry (Rumpf, Masse), Memory
     (aimpoint). Schreiben mit der Psyerns 3D Suite (object_to_lod, write_mlod).
"""
import json
import math
import os
import struct
import sys

import bmesh
import bpy
from mathutils import Matrix, Quaternion, Vector

SUITE = os.environ.get("PSY_SUITE", r"C:\Users\Administrator\Desktop\Psyerns_3D_Suite_For_Blender")
XOB = r"D:\DayZ_Game_Data\DZ\animals\vulpes_vulpes\data\vulpes_vulpes_skeleton.xob"

# ---------------------------------------------------------------------------- fox skeleton


def load_fox():
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
        bones.append({"name": names[k], "pos": Vector(f[:3]), "q": Quaternion((f[6], f[3], f[4], f[5])),
                      "next": nxt, "child": child, "parent": -1})
    for k, bo in enumerate(bones):
        c = bo["child"]
        while c != -1:
            bones[c]["parent"] = k
            c = bones[c]["next"]
    for bo in bones:
        p = bo["parent"]
        if p == -1:
            bo["gpos"], bo["gq"] = bo["pos"].copy(), bo["q"].copy()
        else:
            P = bones[p]
            bo["gpos"] = P["gpos"] + P["gq"] @ bo["pos"]
            bo["gq"] = P["gq"] @ bo["q"]
    out = {}
    for bo in bones:
        g = bo["gpos"]
        out[bo["name"]] = {"pos": Vector((g.x, g.z, g.y)),        # DayZ -> Blender
                           "parent": bones[bo["parent"]]["name"] if bo["parent"] >= 0 else ""}
    return out


# Bones that go into the model (the skeleton of the .p3d / model.cfg); parent "" = root
MODEL_BONES = ["Pelvis", "Spine1", "Spine2", "Chest", "Neck1", "Neck2", "Head", "Jaw", "LEar", "REar",
               "LFLegCollarbone", "LFLeg1", "LFLeg2", "LFLeg3", "LFLegAnkle",
               "RFLegCollarbone", "RFLeg1", "RFLeg2", "RFLeg3", "RFLegAnkle",
               "Tail1", "Tail2", "Tail3", "Tail4",
               "LBLeg1", "LBLeg2", "LBLeg3", "LBLegAnkle", "RBLeg1", "RBLeg2", "RBLeg3", "RBLegAnkle"]


def targets(fox):
    P = lambda n: fox[n]["pos"].copy()
    def ground(n, fwd=0.045):
        p = P(n)
        return Vector((p.x, p.y + fwd, 0.0))
    t = {
        "Hips": (P("Pelvis"), P("Spine2")),
        "chest": (P("Chest"), P("Neck1")),
        "headend": (P("Head"), P("Neck1")),
        "head": (P("Head"), P("pin_lookat")),
        "tail": (P("Pelvis"), P("Tail1")),
        "tailstart": (P("Tail1"), P("Tail2")),
        "tail1": (P("Tail2"), P("Tail3")),
        "tail2": (P("Tail3"), P("Tail4")),
        "tail3": (P("Tail4"), P("Tail4") + (P("Tail4") - P("Tail3")) * 0.9),
    }
    for side, fx in (("", "L"), ("R_", "R")):
        t[side + "frontleg"] = (P(fx + "FLeg1"), P(fx + "FLeg2"))
        t[side + "frontleg0"] = (P(fx + "FLeg2"), P(fx + "FLeg3"))
        t[side + "frontleg1"] = (P(fx + "FLeg3"), P(fx + "FLegAnkle"))
        t[side + "frontleg2"] = (P(fx + "FLegAnkle"), ground(fx + "FLegAnkle"))
        t[side + "backleg"] = (P(fx + "BLeg1"), P(fx + "BLeg2"))
        t[side + "backleg0"] = (P(fx + "BLeg2"), P(fx + "BLeg3"))
        t[side + "backleg1"] = (P(fx + "BLeg3"), P(fx + "BLegAnkle"))
        t[side + "backleg2"] = (P(fx + "BLegAnkle"), ground(fx + "BLegAnkle"))
    return t


def chain_weights(pos, chain, fox, axis):
    """Split one weight along a chain of fox bones by the coordinate ``axis`` of ``pos``: bone i owns
    [joint i, joint i+1]; linear blend across each joint (half a segment each side)."""
    coords = [fox[n]["pos"][axis] for n in chain]
    c = pos[axis]
    if len(chain) == 1:
        return {chain[0]: 1.0}
    centers = [(coords[i] + coords[i + 1]) / 2 if i + 1 < len(coords) else coords[i] + (coords[i] - coords[i - 1]) / 2
               for i in range(len(coords))]
    sign = 1 if centers[-1] > centers[0] else -1
    cs = [x * sign for x in centers]
    v = c * sign
    if v <= cs[0]:
        return {chain[0]: 1.0}
    if v >= cs[-1]:
        return {chain[-1]: 1.0}
    for i in range(len(cs) - 1):
        if cs[i] <= v <= cs[i + 1]:
            f = (v - cs[i]) / max(cs[i + 1] - cs[i], 1e-6)
            return {chain[i]: 1 - f, chain[i + 1]: f}
    return {chain[-1]: 1.0}


def paw_weights(pos, fox, upper, ankle):
    """Hoehe ueber dem Boden: oberhalb des Ankle-Gelenks Leg3, unterhalb 2 cm (Zehen) Ankle, dazwischen linear."""
    top = fox[ankle]["pos"].z
    low = 0.02
    f = max(0.0, min(1.0, (top - pos.z) / max(top - low, 1e-4)))
    return {upper: 1.0 - f, ankle: f}


def fox_weights_for(group, pos, fox):
    X, Y, Z = 0, 1, 2
    if group == "Hips":
        return chain_weights(pos, ["Pelvis", "Spine1", "Spine2"], fox, Y)
    if group == "chest":
        return chain_weights(pos, ["Spine2", "Chest", "Neck1"], fox, Y)
    if group == "headend":
        return chain_weights(pos, ["Neck1", "Neck2", "Head"], fox, Y)
    if group == "head":
        return {"Head": 1.0}
    if group in ("earend", "R_earend"):
        return {("L" if group == "earend" else "R") + "Ear": 1.0}
    m = {"tail": "Pelvis", "tailstart": "Tail1", "tail1": "Tail2", "tail2": "Tail3", "tail3": "Tail4"}
    if group in m:
        return {m[group]: 1.0}
    side = "R" if group.startswith("R_") else "L"
    g = group[2:] if group.startswith("R_") else group
    leg = {"frontleg": side + "FLeg1", "frontleg0": side + "FLeg2", "frontleg1": side + "FLeg3",
           "backleg": side + "BLeg1", "backleg0": side + "BLeg2", "backleg1": side + "BLeg3"}
    if g in leg:
        return {leg[g]: 1.0}
    # Pfote: nach Hoehe weich zwischen Leg3 und Ankle verteilt. Ganz auf Ankle gelegt, knickte die lange
    # Waschbaerpfote in den Fuchs-Clips sichtbar um - der Fuchs beugt das Handgelenk beim Anheben stark
    # (Vorschau walk_eating_idle_1, Frame 120, 30.10.2026). Nur die Zehen folgen dem Ankle-Knochen ganz.
    if g == "frontleg2":
        return paw_weights(pos, fox, side + "FLeg3", side + "FLegAnkle")
    if g == "backleg2":
        return paw_weights(pos, fox, side + "BLeg3", side + "BLegAnkle")
    return {"Pelvis": 1.0}


def bone_affine(rest_a, rest_b, tgt_a, tgt_b):
    d0 = rest_b - rest_a
    d1 = tgt_b - tgt_a
    l0 = max(d0.length, 1e-9)
    l1 = d1.length
    rot = d0.normalized().rotation_difference(d1.normalized()).to_matrix().to_4x4()
    # stretch along the rest direction
    n = d0.normalized()
    s = l1 / l0
    S = Matrix.Identity(3) + (s - 1.0) * Matrix(((n.x * n.x, n.x * n.y, n.x * n.z),
                                                 (n.y * n.x, n.y * n.y, n.y * n.z),
                                                 (n.z * n.x, n.z * n.y, n.z * n.z)))
    return Matrix.Translation(tgt_a) @ rot @ S.to_4x4() @ Matrix.Translation(-rest_a)


# ---------------------------------------------------------------------------- main

def main(job):
    sys.path.insert(0, SUITE)
    import psyerns_3d_suite
    if not hasattr(bpy.types.Scene, "psy"):
        psyerns_3d_suite.register()
    from psyerns_3d_suite.blender.scene_to_asset import object_to_lod, memory_lod_from_objects
    from psyerns_3d_suite.core.mlod import write_mlod
    from psyerns_3d_suite.core.templates import load_builtin_templates

    fox = load_fox()
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.ops.import_scene.fbx(filepath=job["fbx"])
    arm = next(o for o in bpy.context.scene.objects if o.type == "ARMATURE")
    src = next(o for o in bpy.context.scene.objects if o.type == "MESH")
    # "scale": Zahl (gleichmaessig) oder [Breite, Laenge, Hoehe] nach der Drehung (Blender x, y, z).
    # Der Fuchs ist schlanker und laenger als ein Waschbaer: gleichmaessig skaliert ist entweder der
    # Rumpf zu dick oder der Hals muss sich weit strecken.
    sc = job.get("scale", 255.0)
    sx, sy, sz = (sc, sc, sc) if isinstance(sc, (int, float)) else sc
    W = Matrix.Diagonal((sx, sy, sz, 1.0)) @ Matrix.Rotation(math.pi, 4, "Z")

    # rest segments of the Meshy bones in the normalized space
    rest = {b.name: (W @ arm.matrix_world @ b.head_local, W @ arm.matrix_world @ b.tail_local) for b in arm.data.bones}
    tgt = targets(fox)
    aff = {}
    for name, (a, b) in rest.items():
        if name in tgt:
            aff[name] = bone_affine(a, b, *tgt[name])
    # Kopf starr (keine Streckung, der Schaedel behaelt seine Form), gedreht auf die Blickrichtung des
    # Fuchses. Drehpunkt ist die Schaedelbasis (Anfang des Meshy-"headend"), sie liegt danach auf dem
    # Fuchsknochen Head - dort, wo der Kopf beim Fressen und Sichern abknickt. Zwei verworfene Wege:
    # Knochen auf Knochen (das Meshy-"head"-Segment laeuft tief im Kiefer, Hals- und Kopfgelenke lagen
    # ausserhalb des Mesh) und Ohrbasis auf die Ohrknochen (der stilisierte Waschbaerkopf ist gross,
    # der Hals wurde lang und duenn gezogen).
    if "headend" in rest:
        ha, hb = rest["head"]
        R = (hb - ha).normalized().rotation_difference((fox["pin_lookat"]["pos"] - fox["Head"]["pos"]).normalized()).to_matrix().to_4x4()
        na, nb = rest["headend"]
        aff["head"] = Matrix.Translation(fox["Head"]["pos"]) @ R @ Matrix.Translation(-na)
        aff["headend"] = bone_affine(na, nb, fox["Head"]["pos"], fox["Neck1"]["pos"])
    aff["earend"] = aff["head"]
    aff["R_earend"] = aff["head"]

    # evaluated rest mesh (no armature deform), world -> normalized space
    for m in src.modifiers:
        m.show_viewport = False
    deps = bpy.context.evaluated_depsgraph_get()
    me = src.evaluated_get(deps).to_mesh().copy()
    me.transform(W @ src.matrix_world)
    groups = {g.index: g.name for g in src.vertex_groups}

    new_co = []
    new_w = []
    for v in me.vertices:
        ws = [(groups[e.group], e.weight) for e in src.data.vertices[v.index].groups if e.weight > 1e-4 and groups[e.group] in aff]
        tot = sum(w for _, w in ws) or 1.0
        p = Vector((0, 0, 0))
        for g, w in ws:
            p += (aff[g] @ v.co) * (w / tot)
        if not ws:
            p = aff["Hips"] @ v.co
        new_co.append(p)
        fw = {}
        for g, w in ws:
            for fb, f in fox_weights_for(g, p, fox).items():
                fw[fb] = fw.get(fb, 0.0) + f * w / tot
        if not fw:
            fw = {"Pelvis": 1.0}
        top = sorted(fw.items(), key=lambda kv: -kv[1])[:3]
        top = [(b, w) for b, w in top if w >= 0.03] or top[:1]
        s = sum(w for _, w in top)
        new_w.append({b: w / s for b, w in top})
    for v, p in zip(me.vertices, new_co):
        v.co = p
    # die Kopie traegt noch die Meshy-Gewichte (Deform-Layer mit Gruppen-INDIZES) - die wuerden auf die
    # neuen Fuchsgruppen fallen. Layer entfernen, bevor neue Gruppen entstehen.
    bm = bmesh.new()
    bm.from_mesh(me)
    for layer in list(bm.verts.layers.deform.values()):
        bm.verts.layers.deform.remove(layer)
    bm.to_mesh(me)
    bm.free()
    me.update()

    # resolution LODs
    tex = job["texture"]
    template = load_builtin_templates()["item"]
    scene = bpy.context.scene
    scene.psy.template = "item"

    def make_obj(name, mesh, kind, index):
        ob = bpy.data.objects.new(name, mesh)
        scene.collection.objects.link(ob)
        ob.psy.lod_kind = kind
        ob.psy.lod_index = float(index)
        return ob

    res0 = make_obj("res0", me, "resolution", 0)
    for b in MODEL_BONES:
        res0.vertex_groups.new(name=b.lower())
    vg = {g.name: g for g in res0.vertex_groups}
    for i, w in enumerate(new_w):
        for b, f in w.items():
            vg[b.lower()].add([i], f, "REPLACE")
    mat = bpy.data.materials.new("raccoon")
    me.materials.clear()
    me.materials.append(mat)
    mat.psy.texture = tex if hasattr(mat, "psy") and hasattr(mat.psy, "texture") else ""
    for poly in me.polygons:
        poly.use_smooth = True

    lods = []

    def res_lod(ob):
        lod = object_to_lod(ob, bpy.context.evaluated_depsgraph_get(), template)
        for f in lod.faces:
            f.texture = tex
            f.material = job.get("material", "")
        return lod

    lods.append(res_lod(res0))
    for idx, ratio in ((1, 0.5), (2, 0.25)):
        ob = res0.copy()
        ob.data = res0.data.copy()
        scene.collection.objects.link(ob)
        ob.psy.lod_index = float(idx)
        mod = ob.modifiers.new("dec", "DECIMATE")
        mod.ratio = ratio
        bpy.context.view_layer.update()
        # Decimate mischt die Gewichte zusammengelegter Punkte - danach tragen einzelne Punkte mehr als
        # vier Knochen, und DayZ wirft die ueberzaehligen beim Laden weg ("too many bones (more than 4)
        # on vertex", RPT 30.10.2026). Deshalb anwenden und wieder auf drei Einfluesse kuerzen.
        bpy.context.view_layer.objects.active = ob
        with bpy.context.temp_override(object=ob, active_object=ob):
            bpy.ops.object.modifier_apply(modifier="dec")
        names = {g.index: g.name for g in ob.vertex_groups}
        for v in ob.data.vertices:
            ws = sorted(((names[e.group], e.weight) for e in v.groups if e.weight > 0), key=lambda kv: -kv[1])
            keep = [kv for kv in ws[:3] if kv[1] >= 0.03] or ws[:1]
            tot = sum(w for _, w in keep) or 1.0
            for g in [e.group for e in v.groups]:
                ob.vertex_groups[names[g]].remove([v.index])
            for n, w in keep:
                ob.vertex_groups[n].add([v.index], w / tot, "REPLACE")
        lods.append(res_lod(ob))

    # boxes per body part (fire/view geometry, geometry)
    def box(center, size, axis_from=None, axis_to=None):
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0)
        S = Matrix.Diagonal((size[0], size[1], size[2], 1.0))
        R = Matrix.Identity(4)
        if axis_from is not None:
            d = (axis_to - axis_from)
            R = Vector((0, 1, 0)).rotation_difference(d.normalized()).to_matrix().to_4x4()
        bmesh.ops.transform(bm, matrix=Matrix.Translation(center) @ R @ S, verts=bm.verts)
        return bm

    P = lambda n: fox[n]["pos"]
    parts = [  # (zone, bone, from, to, width, height)
        ("zone_head", "Head", P("Neck2"), P("pin_lookat"), 0.10, 0.10),
        ("zone_chest", "Spine2", P("Pelvis"), P("Neck1"), 0.13, 0.17),
    ]
    for s in "LR":
        parts += [("zone_legs", s + "FLeg2", P(s + "FLeg1"), P(s + "FLeg3"), 0.05, 0.05),
                  ("zone_legs", s + "FLegAnkle", P(s + "FLeg3"), Vector((P(s + "FLegAnkle").x, P(s + "FLegAnkle").y, 0.0)), 0.045, 0.045),
                  ("zone_legs", s + "BLeg2", P(s + "BLeg1"), P(s + "BLeg3"), 0.06, 0.06),
                  ("zone_legs", s + "BLegAnkle", P(s + "BLeg3"), Vector((P(s + "BLegAnkle").x, P(s + "BLegAnkle").y, 0.0)), 0.045, 0.045)]

    def parts_mesh(name, kind, part_list, mass=None):
        bm_all = bmesh.new()
        layers = []
        for k, (zone, bone, a, b, wdt, hgt) in enumerate(part_list):
            bm = box((a + b) / 2, (wdt, (b - a).length, hgt), a, b)
            tmp = bpy.data.meshes.new("tmp")
            bm.to_mesh(tmp)
            bm.free()
            offset = len(bm_all.verts)
            bm_all.from_mesh(tmp)
            bpy.data.meshes.remove(tmp)
            layers.append((offset, len(bm_all.verts), zone, bone, k))
        mesh = bpy.data.meshes.new(name)
        bm_all.to_mesh(mesh)
        bm_all.free()
        ob = make_obj(name, mesh, kind, 0)
        for a, b, zone, bone, k in layers:
            for gname in (zone, bone.lower(), "Component%02d" % (k + 1)):
                g = ob.vertex_groups.get(gname) or ob.vertex_groups.new(name=gname)
                g.add(list(range(a, b)), 1.0, "REPLACE")
        if mass is not None:
            ob.psy.mass = mass
        return ob

    fire = parts_mesh("fire", "fire_geometry", parts)
    view = parts_mesh("view", "view_geometry", parts)
    geo = parts_mesh("geo", "geometry", parts[:2], mass=float(job.get("mass", 8.0)))
    for ob in (fire, view, geo):
        lod = object_to_lod(ob, bpy.context.evaluated_depsgraph_get(), template)
        if ob is geo:
            lod.properties = {"autocenter": "0"}
        lods.append(lod)

    mem = bpy.data.objects.new("aimpoint", None)
    scene.collection.objects.link(mem)
    mem.location = (P("Spine2") + P("Chest")) / 2
    mem.psy.lod_kind = "memory"
    mlod_mem = memory_lod_from_objects([mem], bpy.context.evaluated_depsgraph_get())
    if mlod_mem is not None:
        from psyerns_3d_suite.core.model import Selection
        mlod_mem.selections.setdefault("chest", Selection()).points.update(range(len(mlod_mem.points)))
        lods.append(mlod_mem)

    os.makedirs(os.path.dirname(job["out_p3d"]), exist_ok=True)
    with open(job["out_p3d"], "wb") as fh:
        fh.write(write_mlod(lods))

    # report + debug render
    lo = Vector([min(v.co[i] for v in me.vertices) for i in range(3)])
    hi = Vector([max(v.co[i] for v in me.vertices) for i in range(3)])
    report = {"out": job["out_p3d"], "bbox_min": list(lo), "bbox_max": list(hi),
              "lods": [f"{l.kind.value}{l.index:g}:{len(l.faces)}f/{len(l.points)}p" for l in lods],
              "bones_used": sorted({b for w in new_w for b in w}),
              "selections_res0": sorted(lods[0].selections.keys())}
    with open(job.get("report", os.path.splitext(job["out_p3d"])[0] + ".report.json"), "w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=1)
    print("REPORT", json.dumps(report))

    if job.get("debug_dir"):
        render_debug(job, res0, fox)


def render_debug(job, res0, fox):
    sc = bpy.context.scene
    for o in list(sc.objects):
        if o is not res0:
            o.hide_render = True
    img = bpy.data.images.load(job["texture_png"]) if job.get("texture_png") else None
    mat = bpy.data.materials.new("dbg")
    mat.use_nodes = True
    if img:
        n = mat.node_tree.nodes.new("ShaderNodeTexImage")
        n.image = img
        mat.node_tree.links.new(n.outputs[0], mat.node_tree.nodes["Principled BSDF"].inputs[0])
    res0.data.materials.clear()
    res0.data.materials.append(mat)
    # fox joints as small spheres
    for name, d in fox.items():
        if name in ("Scene_Root", "EntityPosition"):
            continue
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.008, location=d["pos"])
        s = bpy.context.active_object
        m2 = bpy.data.materials.new("j")
        m2.diffuse_color = (1, 0, 0, 1)
        m2.use_nodes = True
        m2.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (1, 0.05, 0.05, 1)
        s.data.materials.append(m2)
    sc.render.engine = "CYCLES"
    sc.cycles.device = "CPU"
    sc.cycles.samples = 16
    sc.world = bpy.data.worlds.new("w")
    sc.world.use_nodes = True
    sc.world.node_tree.nodes["Background"].inputs[0].default_value = (0.8, 0.82, 0.88, 1)
    sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
    sc.collection.objects.link(sun)
    sun.rotation_euler = (0.8, 0.2, 0.5)
    cam_d = bpy.data.cameras.new("c")
    cam_d.type = "ORTHO"
    cam_d.ortho_scale = 1.2
    cam = bpy.data.objects.new("cam", cam_d)
    sc.collection.objects.link(cam)
    sc.camera = cam
    sc.render.resolution_x = 900
    sc.render.resolution_y = 600
    c = Vector((0, 0, 0.25))
    for tag, d in (("side", (1, 0, 0.0)), ("persp", (0.8, 0.7, 0.45)), ("front", (0, 1, 0.1)), ("top", (0.0, 0.0, 1))):
        d = Vector(d).normalized()
        cam.location = c + d * 3
        cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
        sc.render.filepath = os.path.join(job["debug_dir"], f"raccoon_{tag}.png")
        bpy.ops.render.render(write_still=True)


if __name__ == "__main__":
    main(json.load(open(sys.argv[sys.argv.index("--") + 1], encoding="utf-8")))
