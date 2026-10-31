"""Baut aus einem stehenden Tiermodell ein liegendes Modell fuer das tote Tier.

Rollt alle LODs um die Laengsachse Z auf die Seite und setzt sie auf den Boden. Das
Ergebnis hat kein model.cfg und damit kein Skelett: ein Item wie Vanillas
vulpes_vulpes_dead.p3d. Land Contact fehlt in Tiermodellen und wird ergaenzt.

    py -3 tools/chefz-assets/make-dead-animal.py <quelle.p3d> <ziel.p3d>
"""
import sys

sys.path.insert(0, r"E:\DayZ Modding\Psyerns_3D_Suite_For_Blender")
from psyerns_3d_suite.core.lods import LodKind  # noqa: E402
from psyerns_3d_suite.core.mlod import read_mlod, write_mlod  # noqa: E402
from psyerns_3d_suite.core.model import Lod  # noqa: E402


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    src, dst = sys.argv[1], sys.argv[2]
    lods = read_mlod(open(src, "rb").read())

    points = lods[0].points
    max_x = max(p[0] for p in points)
    mid_y = (min(p[1] for p in points) + max(p[1] for p in points)) * 0.5
    for lod in lods:
        # Drehung um Z: (x, y) -> (y, -x). Eine Drehung, also bleibt der Umlaufsinn.
        lod.points = [(y - mid_y, max_x - x, z) for (x, y, z) in lod.points]
        for face in lod.faces:
            for vertex in face.vertices:
                if vertex.normal is not None:
                    nx, ny, nz = vertex.normal
                    vertex.normal = (ny, -nx, nz)

    points = lods[0].points
    half_x = max(abs(p[0]) for p in points) * 0.6
    min_z = min(p[2] for p in points)
    max_z = max(p[2] for p in points)
    if not any(lod.kind == LodKind.LAND_CONTACT for lod in lods):
        lods.append(Lod(kind=LodKind.LAND_CONTACT, index=0, points=[(0.0, 0.0, max_z), (-half_x, 0.0, min_z), (half_x, 0.0, min_z)]))

    open(dst, "wb").write(write_mlod(lods))
    print(f"{dst}: {len(lods)} LODs, Hoehe {max(p[1] for p in points):.3f} m")
    return 0


if __name__ == "__main__":
    sys.exit(main())
