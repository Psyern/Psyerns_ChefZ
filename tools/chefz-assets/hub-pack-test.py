r"""Packt ChefZ fuer den Spieltest mit Ninjins Environment Hub - MIT *.json.

Der Hub ueberspringt beim Kopieren .json, .md und .txt (dayz_tools.SOURCE_COPY_SKIP_SUFFIXES).
ChefZ liest Stationen, Prozesse, Rezepte und Fangdaten aus den JSON-Dateien im PBO; ohne sie
startet der Core im SAFE MODE (alle Registries leer: keine Stationsaktion, keine Ernte, keine
Koederwirkung). Dieses Skript setzt die Liste fuer den Aufruf auf {".md"}.

Aufruf (vorher alle Addons aus ChefZ_Core/Addons nach <TestServer>\src kopieren):
    py -3 tools\chefz-assets\hub-pack-test.py [ChefZ_Mod ...]
Packt jeden Ordner ChefZ_* in <SRC_PATH> der Hub-Konfiguration. ChefZ_COT_Comp gehoert nur
dazu, wenn der COT-Stand des Testservers die Kategorie-API hat (RebuildCategoryMenu).
Achtung: der Hub-Build beendet laufende DayZ-Prozesse.
"""
import os
import sys
from pathlib import Path

HUB = Path(os.environ.get("NINJINS_HUB", r"E:\DayZ Modding\Ninjins_Environment_Hub"))
sys.path.insert(0, str(HUB))
os.chdir(HUB)
import dayz_tools as hub  # noqa: E402

hub.SOURCE_COPY_SKIP_SUFFIXES = {".md"}
config = hub.read_user_config()
src = Path(hub.config_string_value(config.get("SRC_PATH")))
mods = sorted(p.name for p in src.iterdir() if p.is_dir() and p.name.startswith("ChefZ_"))
if len(sys.argv) > 1:
    mods = [m for m in mods if m in sys.argv[1:]]
print("Mods:", ", ".join(mods))
sys.exit(hub.run_headless_build(force_sign=False, mod_names=mods))
