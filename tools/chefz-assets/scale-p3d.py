"""Skaliert ein MLOD-p3d gleichmaessig um den Ursprung (alle LODs, auch Memory
und Land Contact). Normalen bleiben richtig, weil die Skalierung gleichmaessig ist.

    py -3 tools/chefz-assets/scale-p3d.py <model.p3d> <faktor>

Ueberschreibt die Datei; vorher eine Sicherung <model>.p3d.bak anlegen lassen
mit --backup.
"""
import shutil
import sys

sys.path.insert(0, r"E:\DayZ Modding\Psyerns_3D_Suite_For_Blender")
from psyerns_3d_suite.core.mlod import read_mlod, write_mlod  # noqa: E402


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) != 2:
        print(__doc__)
        return 2
    path, factor = args[0], float(args[1])
    if "--backup" in sys.argv:
        shutil.copyfile(path, path + ".bak")
    lods = read_mlod(open(path, "rb").read())
    for lod in lods:
        lod.points = [(x * factor, y * factor, z * factor) for (x, y, z) in lod.points]
    open(path, "wb").write(write_mlod(lods))
    print(f"{path}: {len(lods)} LODs x{factor}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
