"""Dreht ein flach modelliertes Buch in den Griffrahmen von book.anm.

Die Pose ist gegen Vanillas Buch gestellt: die Hand greift den Ruecken, der Deckel
zeigt zur Seite. Das Modell liegt flach (Dicke auf Y), also Drehung um X: Y wird Z.
Alle LODs samt Memory drehen mit, damit die Vorschau gleich bleibt.

    py -3 tools/chefz-assets/p3d-book-upright.py <modell.p3d>   (nur einmal ausfuehren)
"""
import sys

sys.path.insert(0, r"E:\DayZ Modding\Psyerns_3D_Suite_For_Blender")
from psyerns_3d_suite.core.mlod import read_mlod, write_mlod  # noqa: E402

path = sys.argv[1]
lods = read_mlod(open(path, "rb").read())
points = lods[0].points
mid_y = (min(p[1] for p in points) + max(p[1] for p in points)) * 0.5
for lod in lods:
    lod.points = [(x, -z, y - mid_y) for (x, y, z) in lod.points]
    for face in lod.faces:
        for vertex in face.vertices:
            if vertex.normal is not None:
                nx, ny, nz = vertex.normal
                vertex.normal = (nx, -nz, ny)
open(path, "wb").write(write_mlod(lods))
print(path, "gedreht")
