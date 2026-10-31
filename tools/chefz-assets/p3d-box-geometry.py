"""Ersetzt die Geometry-LOD durch einen konvexen Quader gleicher Ausmasse und Masse.

Die Engine verlangt konvexe Kollisionskomponenten. Vorlage fuer Flaechen und Selektion
ist ein Modell, dessen Geometry bereits ein Quader ist.

    py -3 tools/chefz-assets/p3d-box-geometry.py <modell.p3d> <vorlage.p3d>
"""
import copy
import sys

sys.path.insert(0, r"E:\DayZ Modding\Psyerns_3D_Suite_For_Blender")
from psyerns_3d_suite.core.mlod import read_mlod, write_mlod  # noqa: E402
from psyerns_3d_suite.core.model import LodKind  # noqa: E402


def geometry(lods):
    return next(lod for lod in lods if lod.kind == LodKind.GEOMETRY)


path, template_path = sys.argv[1], sys.argv[2]
lods = read_mlod(open(path, "rb").read())
target = geometry(lods)
template = geometry(read_mlod(open(template_path, "rb").read()))
if len(template.points) != 8 or len(template.faces) != 6:
    sys.exit(template_path + ": Geometry ist kein Quader")

low = [min(p[i] for p in target.points) for i in range(3)]
high = [max(p[i] for p in target.points) for i in range(3)]
tlow = [min(p[i] for p in template.points) for i in range(3)]
thigh = [max(p[i] for p in template.points) for i in range(3)]
total = sum(target.mass)
# Die Vorlage ist leicht schief: jede Ecke rastet auf die naechste Seite der Huelle ein.
target.points = [tuple(high[i] if p[i] > (tlow[i] + thigh[i]) * 0.5 else low[i] for i in range(3)) for p in template.points]
target.faces = copy.deepcopy(template.faces)
target.selections = copy.deepcopy(template.selections)
target.mass = [total / 8.0] * 8
target.sharp_edges = copy.deepcopy(template.sharp_edges)
open(path, "wb").write(write_mlod(lods))
print(path, "Quader", low, high, "Masse", total)
