r"""Eine Meshy-Vierbeiner-Animation (FBX) als DayZ-.anm auf dem Fuchs-Skelett.

    blender -b --factory-startup --python tools/chefz-assets/animal_anim_from_meshy.py -- <job.json>

job.json: {"fbx": "<Meshy ..._Animation_Walking_frame_rate_30.fbx>", "out_anm": ...,
           "fox_anm": "walk", "cycles": 2, "scale": [215, 250, 215]}

Gegenstueck zu animal_retarget_fox.py (das Mesh). Das Ergebnis ersetzt EINEN Slot des Fuchs-
Animationssatzes (hier "Default.Normal.walk") ueber eine eigene .asi, alle anderen Slots bleiben die des
Fuchses.

Rechnung (dieselbe Semantik wie die Suite fuer Infizierte, core/anm_export.py, und wie die gemessenen
Fuchs-.anm): je Fuchsknochen die globale Rotation = Weltdrehung des zugeordneten Meshy-Knochens gegen
seine Ruhepose (Delta) x globale Bind-Rotation des Fuchses; lokal = Eltern^-1 x global. Translationen
bleiben die der Fuchs-Bindpose, ausser Pelvis (Bind + Huefthub der Meshy-Animation). Wirbelsaeule und
Hals werden zwischen Hips/chest bzw. chest/head interpoliert (Slerp). Knochen ohne Gegenstueck behalten
ihre Bind-Rotation.

Zeit: die Fuchs-Vorlage (fox_anm) gibt Laenge, Wurzelbewegung (EntityPosition) und Schritt-Events vor -
der Animationsgraph bewegt das Tier ueber diese Wurzelbewegung. Die Meshy-Schleife laeuft NACH STRECKE:
je "stride" Metern Wurzelbewegung ein Zyklus (Vorgabe 1.19 m / cycles, d. h. der Fuchs-Walk bekommt genau
"cycles" Zyklen). Steht die Vorlage (walk_eating: schnuppern, fressen), wird ueber die Geschwindigkeit
weich in die Pose der Fuchs-Vorlage geblendet - stehende Beine rutschen nicht, und das Fressen bleibt.

Welche Slots: auf dem Server spielt ein umherstreifendes Tier "walk_eating"/"walk_eating_idle_1", nicht
"walk" (Markerprobe 30.10.2026) - der Waschbaer bekommt deshalb alle drei.
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
FOX_ANIMS = r"D:\DayZ_Game_Data\DZ\animals\animations\red_fox_skeleton"

# Fuchsknochen -> (Meshy-Knochen A, Meshy-Knochen B, Anteil B) ; Delta = slerp(A, B, Anteil)
MAP = {
    "Pelvis": ("Hips", "Hips", 0.0),
    "Spine1": ("Hips", "chest", 0.33),
    "Spine2": ("Hips", "chest", 0.66),
    "Chest": ("chest", "chest", 0.0),
    "Neck1": ("chest", "head", 0.33),
    "Neck2": ("chest", "head", 0.66),
    "Head": ("head", "head", 0.0),
    "Tail1": ("tailstart", "tailstart", 0.0),
    "Tail2": ("tail1", "tail1", 0.0),
    "Tail3": ("tail2", "tail2", 0.0),
    "Tail4": ("tail3", "tail3", 0.0),
}
for side, pre in (("L", ""), ("R", "R_")):
    MAP.update({
        side + "FLeg1": (pre + "frontleg", pre + "frontleg", 0.0),
        side + "FLeg2": (pre + "frontleg0", pre + "frontleg0", 0.0),
        side + "FLeg3": (pre + "frontleg1", pre + "frontleg1", 0.0),
        side + "FLegAnkle": (pre + "frontleg2", pre + "frontleg2", 0.0),
        side + "BLeg1": (pre + "backleg", pre + "backleg", 0.0),
        side + "BLeg2": (pre + "backleg0", pre + "backleg0", 0.0),
        side + "BLeg3": (pre + "backleg1", pre + "backleg1", 0.0),
        side + "BLegAnkle": (pre + "backleg2", pre + "backleg2", 0.0),
    })

P = Matrix(((1, 0, 0), (0, 0, 1), (0, 1, 0)))          # Blender <-> DayZ (x, z, y), selbstinvers
RZ = Matrix.Rotation(math.pi, 3, "Z")


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
    for bo in bones:
        p = bo["parent"]
        bo["gq"] = (bones[p]["gq"] @ bo["q"]) if p >= 0 else bo["q"].copy()
    return bones


def to_dayz(q_blender_world):
    """Weltrotation (Blender, Rohdaten der FBX) -> DayZ/XOB-Raum, mit der 180-Grad-Drehung des Imports."""
    m = P @ (RZ @ q_blender_world.to_matrix() @ RZ.transposed()) @ P
    return m.to_quaternion()


def main(job):
    sys.path.insert(0, SUITE)
    from psyerns_3d_suite.core import anm
    bones = load_xob()
    by_name = {b["name"]: b for b in bones}
    fox = anm.read(open(os.path.join(FOX_ANIMS, job.get("fox_anm", "walk") + ".anm"), "rb").read())
    frames = fox.frame_count
    cycles = int(job.get("cycles", 2))

    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.ops.import_scene.fbx(filepath=job["fbx"])
    arm = next(o for o in bpy.context.scene.objects if o.type == "ARMATURE")
    act = arm.animation_data.action
    f0, f1 = act.frame_range
    period = f1 - f0                     # letzter Frame == erster (Schleife)
    sc = bpy.context.scene
    W = arm.matrix_world

    rest_rot = {b.name: (W @ b.matrix_local).to_quaternion() for b in arm.data.bones}
    hips_rest = W @ arm.data.bones["Hips"].head_local
    sx, sy, sz = job.get("scale", [215, 250, 215])
    pelvis_bind = by_name["Pelvis"]["t"]

    tracks = {b["name"]: {"r": {}, "t": {}} for b in bones}
    gpos = {}
    for bo in bones:
        p = bo["parent"]
        gpos[bo["name"]] = (gpos[bones[p]["name"]] + bones[p]["gq"] @ bo["t"]) if p >= 0 else bo["t"].copy()
    tail_dir = (gpos["Tail4"] - gpos["Tail1"]).normalized()
    tail_up = tail_dir.rotation_difference(Vector((0.0, 1.0, -0.15)).normalized())
    # Phase nach Strecke statt nach Zeit: der Meshy-Schrittzyklus laeuft genau so weit, wie die
    # Wurzelbewegung (EntityPosition) der Fuchs-Vorlage in diesem Frame vorankommt - keine
    # rutschenden Fuesse, und wo der Fuchs steht (walk_eating: schnuppern, fressen), stehen die Beine.
    # "stride" = Strecke je Meshy-Zyklus; Vorgabe so, dass der reine Fuchs-Walk (1.19 m) genau
    # "cycles" Zyklen bekommt.
    ent = fox.bone("EntityPosition")

    def root_z(t):
        if ent is None or not ent.translations:
            return 0.0
        ks = sorted(ent.translations)
        if t <= ks[0]:
            return ent.translations[ks[0]][2]
        for a, b in zip(ks, ks[1:]):
            if a <= t <= b:
                f = (t - a) / max(b - a, 1)
                return ent.translations[a][2] * (1 - f) + ent.translations[b][2] * f
        return ent.translations[ks[-1]][2]

    stride = float(job.get("stride", 1.19 / max(cycles, 1)))
    walk_speed = float(job.get("walk_speed", 1.19 / 54.0))       # m je Frame im Fuchs-Walk
    dist = [abs(root_z(t) - root_z(0)) for t in range(frames)]
    speed = [abs(root_z(min(t + 2, frames - 1)) - root_z(max(t - 2, 0))) / max(min(t + 2, frames - 1) - max(t - 2, 0), 1)
             for t in range(frames)]
    # 0 = Fuchs-Pose, 1 = Meshy. Schmale Rampe und zeitlich geglaettet (+-6 Frames): Mischposen zwischen
    # zwei verschiedenen Beinhaltungen sehen geknickt aus - sie sollen kurz sein.
    raw = [max(0.0, min(1.0, (v / walk_speed - 0.12) / 0.25)) for v in speed]
    blend = []
    for t in range(frames):
        win = raw[max(0, t - 6):t + 7]
        x = sum(win) / len(win)
        blend.append(x * x * (3 - 2 * x))

    def fox_local(name, t):
        fb = fox.bone(name)
        bo = by_name[name]
        q, tr = bo["q"], bo["t"]
        if fb is not None and fb.rotations:
            k = min(fb.rotations, key=lambda f: abs(f - t))
            x, y, z, w = fb.rotations[k]
            q = Quaternion((w, x, y, z))
        if fb is not None and fb.translations:
            k = min(fb.translations, key=lambda f: abs(f - t))
            tr = Vector(fb.translations[k])
        return q, tr

    for t in range(frames):
        mf = f0 + ((dist[t] / stride) * period) % period
        sc.frame_set(int(mf), subframe=mf - int(mf))
        delta = {}
        for pb in arm.pose.bones:
            pose_q = (W @ pb.matrix).to_quaternion()
            delta[pb.name] = to_dayz(pose_q @ rest_rot[pb.name].inverted())
        glob = {}
        for bo in bones:
            name = bo["name"]
            par = bones[bo["parent"]]["name"] if bo["parent"] >= 0 else None
            if name in MAP:
                a, b, f = MAP[name]
                d = delta[a].slerp(delta[b], f) if a != b else delta[a]
                if job.get("marker_tail_up") and name.startswith("Tail"):
                    # NUR FUER DEN SERVERNACHWEIS: Schwanz senkrecht nach oben - eine Lage, die in keiner
                    # Fuchs-Animation vorkommt (Schwanzspitze ueber dem Becken). Nie ausliefern.
                    d = tail_up
                g = d @ bo["gq"]
            else:
                g = (glob[par] @ bo["q"]) if par else bo["q"]
            glob[name] = g
            local = (glob[par].inverted() @ g) if par else g
            local.normalize()
            if name not in ("EntityPosition", "Scene_Root"):
                fq, _ = fox_local(name, t)
                if fq.dot(local) < 0:
                    fq = -fq
                local = fq.slerp(local, blend[t])
            tracks[name]["r"][t] = (local.x, local.y, local.z, local.w)
        # Huefthub (in place): Hips-Verschiebung gegen die Ruhepose, skaliert wie das Mesh, in den DayZ-Raum
        hp = W @ arm.pose.bones["Hips"].head
        d = RZ @ (hp - hips_rest).to_3d()
        d = Vector((d.x * sx, d.y * sy, d.z * sz))
        dz = P @ d
        meshy_pelvis = pelvis_bind + Vector((dz.x, dz.y, dz.z))
        _, fox_pelvis = fox_local("Pelvis", t)
        tracks["Pelvis"]["t"][t] = tuple(fox_pelvis.lerp(meshy_pelvis, blend[t]))

    out_bones = []
    for bo in bones:
        name = bo["name"]
        fb = fox.bone(name)
        ab = anm.AnmBone(name)
        if name in ("EntityPosition", "Scene_Root") and fb is not None:
            ab.translations = dict(fb.translations)
            ab.rotations = dict(fb.rotations)
        else:
            ab.rotations = tracks[name]["r"]
            ab.translations = tracks[name]["t"]
        out_bones.append(ab)
    out = anm.Anm(fps=fox.fps, frame_count=frames, bones=out_bones, events=list(fox.events), version=fox.version)
    os.makedirs(os.path.dirname(job["out_anm"]), exist_ok=True)
    with open(job["out_anm"], "wb") as fh:
        fh.write(anm.write(out))
    # Rueckleseprobe
    back = anm.read(open(job["out_anm"], "rb").read())
    print("ANM", json.dumps({"out": job["out_anm"], "frames": back.frame_count, "fps": back.fps,
                             "bones": len(back.bones), "events": len(back.events), "version": back.version,
                             "meshy_period": period, "stride": stride, "meshy_cycles": round(dist[-1] / stride, 2),
                             "blend_mean": round(sum(blend) / len(blend), 2)}))


if __name__ == "__main__":
    main(json.load(open(sys.argv[sys.argv.index("--") + 1], encoding="utf-8")))
