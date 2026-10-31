"""Dreht ein liegendes Modell auf die andere Flanke und um die Hochachse.

Beides aendert nichts daran, wie es am Boden liegt, nur wie es in den Haenden sitzt.
Alle LODs samt Memory drehen mit.

    py -3 tools/chefz-assets/p3d-turn-lying.py <modell.p3d> <grad> [versatz_x]   (nur einmal ausfuehren)
"""
import math
import sys

sys.path.insert(0, r"E:\DayZ Modding\Psyerns_3D_Suite_For_Blender")
from psyerns_3d_suite.core.mlod import read_mlod, write_mlod  # noqa: E402

path = sys.argv[1]
angle = math.radians(float(sys.argv[2]))
cos_a = math.cos(angle)
sin_a = math.sin(angle)
shift_x = float(sys.argv[3]) if len(sys.argv) > 3 else 0.0
lods = read_mlod(open(path, "rb").read())
points = lods[0].points
max_y = max(p[1] for p in points)
min_y = min(p[1] for p in points)


def turn(x, y, z):
    # Flankenwechsel: 180 Grad um Z. Danach um Y. Beides Drehungen, der Umlaufsinn bleibt.
    x, y = -x, -y
    return (x * cos_a + z * sin_a, y, -x * sin_a + z * cos_a)


for lod in lods:
    lod.points = [turn(x, y, z) for (x, y, z) in lod.points]
    lod.points = [(x + shift_x, y + max_y + min_y, z) for (x, y, z) in lod.points]
    for face in lod.faces:
        for vertex in face.vertices:
            if vertex.normal is not None:
                vertex.normal = turn(*vertex.normal)
open(path, "wb").write(write_mlod(lods))
print(path, "gedreht")
