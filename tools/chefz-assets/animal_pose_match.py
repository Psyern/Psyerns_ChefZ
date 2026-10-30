r"""Welche Animation spielt ein Tier auf dem Server WIRKLICH? Abgleich geloggter Posen gegen .anm-Frames.

    py -3 -I tools/chefz-assets/animal_pose_match.py <poses.txt> <eigene_anims_ordner> [slot ...]

poses.txt: je Zeile "<tag> <geschwindigkeit m/s> <15 Zahlen>" = Modellraum-Lage von LFLeg3, RFLeg3,
LBLeg3, RBLeg3, Tail2 relativ zum Becken (Testmission: DayZCreature.WorldToModel(GetBonePositionWS)).

Je Pose der naechste Frame in drei Mengen:
  RAC    die eigenen Clips (<ordner>/raccoon_<slot>.anm)
  FOXW   die Fuchs-Originale derselben Slots
  OTHER  alle uebrigen Fuchs-Clips (die das Tier unveraendert erbt)
Gezaehlt nur in Bewegung (0.2 .. 1.5 m/s). Ein Tier mit eigener .asi muss RAC treffen, der Vanilla-Fuchs
als Kontrolle FOXW. Beide Achslagen (XOB wie gelesen / um Y gedreht) werden gerechnet; die richtige ist
die, in der der Kontrollfuchs naeher an seinen eigenen Frames liegt.
"""
import glob
import math
import os
import struct
import sys

SUITE = os.environ.get("PSY_SUITE", r"C:\Users\Administrator\Desktop\Psyerns_3D_Suite_For_Blender")
XOB = r"D:\DayZ_Game_Data\DZ\animals\vulpes_vulpes\data\vulpes_vulpes_skeleton.xob"
FOX = r"D:\DayZ_Game_Data\DZ\animals\animations\red_fox_skeleton"
NAMES = ["LFLeg3", "RFLeg3", "LBLeg3", "RBLeg3", "Tail2"]


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


def qrot(q, v):
    x, y, z, w = q
    vx, vy, vz = v
    ix = w * vx + y * vz - z * vy
    iy = w * vy + z * vx - x * vz
    iz = w * vz + x * vy - y * vx
    iw = -x * vx - y * vy - z * vz
    return (ix * w - iw * x - iy * z + iz * y, iy * w - iw * y - iz * x + ix * z, iz * w - iw * z - ix * y + iy * x)


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
        bones.append({"name": names[k], "pos": f[:3], "quat": f[3:7], "next": nxt, "child": child, "parent": -1})
    for k, bo in enumerate(bones):
        c = bo["child"]
        while c != -1:
            bones[c]["parent"] = k
            c = bones[c]["next"]
    return bones


def poses(anm, bones, path):
    a = anm.read(open(path, "rb").read())
    out = []
    for fr in range(a.frame_count):
        loc = {}
        for b in a.bones:
            t = b.translations[min(b.translations, key=lambda f: abs(f - fr))] if b.translations else None
            r = b.rotations[min(b.rotations, key=lambda f: abs(f - fr))] if b.rotations else None
            loc[b.name] = (t, r)
        G = {}
        for bo in bones:
            t, r = loc.get(bo["name"], (None, None))
            t = t or bo["pos"]
            r = r or bo["quat"]
            p = bo["parent"]
            if p < 0:
                G[bo["name"]] = (tuple(t), tuple(r))
            else:
                pt, pq = G[bones[p]["name"]]
                rt = qrot(pq, t)
                G[bo["name"]] = (tuple(pt[i] + rt[i] for i in range(3)), qmul(pq, r))
        pel = G["Pelvis"][0]
        out.append([tuple(G[n][0][i] - pel[i] for i in range(3)) for n in NAMES])
    return out


def main(pose_file, own_dir, *slots):
    sys.path.insert(0, SUITE)
    from psyerns_3d_suite.core import anm
    slots = slots or ("walk", "walk_eating", "walk_eating_idle_1")
    bones = load_xob()
    sets = {"RAC": [], "FOXW": [], "OTHER": []}
    for s in slots:
        sets["RAC"] += poses(anm, bones, os.path.join(own_dir, "raccoon_" + s + ".anm"))
        sets["FOXW"] += poses(anm, bones, os.path.join(FOX, s + ".anm"))
    for f in glob.glob(os.path.join(FOX, "*.anm")):
        if os.path.splitext(os.path.basename(f))[0] not in slots:
            sets["OTHER"] += poses(anm, bones, f)
    samples = []
    for line in open(pose_file, encoding="ascii", errors="ignore"):
        p = line.replace("'", "").split()
        if len(p) != 17:
            continue
        vals = list(map(float, p[2:]))
        samples.append((p[0], float(p[1]), [tuple(vals[i * 3:i * 3 + 3]) for i in range(5)]))

    def dist(a, b):
        return math.sqrt(sum((a[i][j] - b[i][j]) ** 2 for i in range(5) for j in range(3)))

    for orient, fn in (("wie_gelesen", lambda q: q), ("um_Y_gedreht", lambda q: (-q[0], q[1], -q[2]))):
        res = {}
        med = {}
        for tag, v, pose in samples:
            if not 0.2 <= v <= 1.5:
                continue
            pz = [fn(q) for q in pose]
            d = {k: min(dist(pz, r) for r in refs) for k, refs in sets.items()}
            best = min(d, key=d.get)
            if best != "OTHER" and abs(d["RAC"] - d["FOXW"]) < 0.01:
                best = "gleich"
            res.setdefault(tag, {}).setdefault(best, 0)
            res[tag][best] += 1
            med.setdefault(tag, []).append(min(d.values()))
        print(orient, {t: (res[t], "median_abstand=%.3f" % sorted(med[t])[len(med[t]) // 2]) for t in res})


if __name__ == "__main__":
    main(*sys.argv[1:])
