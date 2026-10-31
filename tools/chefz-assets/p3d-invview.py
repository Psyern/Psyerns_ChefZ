"""Setzt die Blickrichtung der Inventarvorschau: invView liegt auf einer Seite des Modells.

Ohne Abstand zur Mitte hat der Punkt keine Richtung, dann waehlt die Engine selbst.
Blickachse ist die schmalste Koerperachse (10-90-Perzentil, damit Flossen nicht zaehlen),
das Modell zeigt also seine breite Seite. Andere Achse: --axis x|y|z.

    py -3 tools/chefz-assets/p3d-invview.py <modell.p3d> [--axis z]
"""
import sys

sys.path.insert(0, r"E:\DayZ Modding\Psyerns_3D_Suite_For_Blender")
from psyerns_3d_suite.core.mlod import read_mlod, write_mlod  # noqa: E402


def spread(points, axis):
    values = sorted(p[axis] for p in points)
    return values[int(len(values) * 0.9)] - values[int(len(values) * 0.1)]


path = sys.argv[1]
lods = read_mlod(open(path, "rb").read())
points = lods[0].points
if "--axis" in sys.argv:
    axis = "xyz".index(sys.argv[sys.argv.index("--axis") + 1])
else:
    axis = min(range(3), key=lambda i: spread(points, i))
low = [min(p[i] for p in points) for i in range(3)]
high = [max(p[i] for p in points) for i in range(3)]
center = [(low[i] + high[i]) * 0.5 for i in range(3)]
target = list(center)
target[axis] += max(high[i] - low[i] for i in range(3))
done = False
for lod in lods:
    for name, selection in lod.selections.items():
        if name.lower() == "invview":
            for index in selection.points:
                lod.points[index] = tuple(target)
            done = True
if not done:
    sys.exit(path + ": kein invView")
open(path, "wb").write(write_mlod(lods))
print(path, "Achse", "xyz"[axis])
