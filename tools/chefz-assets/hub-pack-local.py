r"""Packt ChefZ mit Ninjins Environment Hub in einen Testordner - ohne die Hub-Config anzufassen.

Wie hub-pack-test.py, aber fuer diesen Rechner (Hub unter Mod Repositories, das nur gelesen
wird) und fuer einen beliebigen Testordner:

    py -3 tools\chefz-assets\hub-pack-local.py <Testordner>

Erwartet <Testordner>\src\ChefZ_* (Kopien von ChefZ_Core\Addons\*). Schreibt die PBOs nach
<Testordner>\target\@Client\addons und \@Server\addons, Zwischenstaende nach <Testordner>\tmp.

Drei Abweichungen vom Hub-Standard, alle per Monkeypatch nur fuer diesen Aufruf:
  - SOURCE_COPY_SKIP_SUFFIXES = {".md"}: ChefZ braucht seine .json im PBO, sonst SAFE MODE.
  - stop_processes ist abgeschaltet: der Hub beendet sonst per "taskkill /IM DayZServer_x64.exe"
    JEDEN DayZ-Server auf dem Rechner, auch fremde.
  - read_user_config liefert die Pfade direkt; die paths.json im Hub-Ordner bleibt unberuehrt.
"""
import os
import sys
from pathlib import Path

sys.dont_write_bytecode = True
HUB = Path(os.environ.get("NINJINS_HUB", r"C:\Users\Administrator\Desktop\Mod Repositories\Ninjins_Environment_Hub"))
ROOT = Path(sys.argv[1]).resolve()
sys.path.insert(0, str(HUB))
import dayz_tools as hub  # noqa: E402

hub.apply_project_root(ROOT)
hub.SOURCE_COPY_SKIP_SUFFIXES = {".md"}
hub.stop_processes = lambda stop_client, stop_server: None
config = hub.default_hub_paths()
config.update({
    "PROJECT_ROOT_PATH": str(ROOT),
    "SRC_PATH": str(ROOT / "src"),
    "TARGET_CLIENT_PATH": str(ROOT / "target" / "@Client"),
    "TARGET_SERVER_PATH": str(ROOT / "target" / "@Server"),
    "TEMP_BUILD_BASE_PATH": str(ROOT / "tmp"),
    "LOCAL_DAYZ_SERVER_PATH": "",
})
hub.read_user_config = lambda: dict(config)
for folder in (hub.TARGET_CLIENT_ADDONS, hub.TARGET_SERVER_ADDONS):
    folder.mkdir(parents=True, exist_ok=True)
mods = sorted(p.name for p in (ROOT / "src").iterdir() if p.is_dir() and p.name.startswith("ChefZ_"))
if len(sys.argv) > 2:
    mods = [m for m in mods if m in sys.argv[2:]]
print("Mods:", ", ".join(mods))
sys.exit(hub.run_headless_build(force_sign=False, mod_names=mods))
