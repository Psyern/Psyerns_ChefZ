r"""Spieltest einer Lieferung: Pruef-Addon fuer ungebundene Modelle + init.c, die jede Klasse spawnt.

Aufruf: py -3 make-dayz-test.py <report.json> <TestServer\src> <Missionsordner>
Umgebung: PSY_SUITE ueberschreibt den Pfad zur Psyerns 3D Suite.
"""
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, os.environ.get("PSY_SUITE", r"E:\DayZ Modding\Psyerns_3D_Suite_For_Blender"))
from psyerns_3d_suite.core import servercheck  # noqa: E402

report = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
src = Path(sys.argv[2])            # TestServer\src
mission = Path(sys.argv[3])        # mission folder for init.c
ok = [r for r in report["results"] if r["status"] in ("OK", "WARNING")]

lines = ["class CfgPatches", "{", "\tclass PSY_SealifeCheck", "\t{", "\t\tunits[] = {};", "\t\tweapons[] = {};",
         "\t\trequiredVersion = 0.1;", '\t\trequiredAddons[] = {"DZ_Data", "DZ_Gear_Food", "ChefZ_Sealife", "ChefZ_Food"};',
         "\t};", "};", "class CfgVehicles", "{", "\tclass Inventory_Base;"]
check_classes = []
for r in ok:
    if r["bind"]:
        continue
    name = f"PSY_Check_{r['name']}"
    check_classes.append(name)
    lines += [f"\tclass {name}: Inventory_Base", "\t{", "\t\tscope = 2;", f'\t\tmodel = "{r["model"]}";',
              f'\t\thiddenSelections[] = {{"{r.get("selection", "camo")}"}};',
              f'\t\thiddenSelectionsTextures[] = {{"{r["texture"]}"}};', "\t};"]
lines += ["};", ""]
addon = src / "PSY_SealifeCheck"
addon.mkdir(parents=True, exist_ok=True)
(addon / "config.cpp").write_text("\n".join(lines), encoding="utf-8")

bound = [c for r in ok for c in r["bind"] if not c.startswith("ChefZ_Sealife_")]
classes = [servercheck.CheckClass(n, "item") for n in bound + check_classes]
(mission / "init.c").write_text(servercheck.init_c_text(classes), encoding="utf-8", newline="\n")
print(f"{len(bound)} gebundene ChefZ-Klassen, {len(check_classes)} Prüfklassen -> {len(classes)} Spawns")

(addon / "$PREFIX$").write_text("PSY_SealifeCheck", encoding="utf-8")
