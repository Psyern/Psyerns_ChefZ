"""bind-delivery - bindet gebaute Modelle einer Lieferung an die ChefZ-Klassen.

Zweiter Schritt nach tools/chefz-assets/import-delivery.ps1 (die Psyerns 3D Suite baut aus
FBX + PNG die .p3d/.paa im Lieferordner ChefZ/<Paket>/...). Dieses Skript liest den Bericht
des Baus und

  1. kopiert jede gebaute .p3d/.paa in die Packquelle ChefZ_Core/Addons/<Paket>/ (dasselbe,
     was sync-assets.mjs tut - nur ohne Node und nur fuer diese Lieferung),
  2. setzt in jeder Klasse aus "bind" die Zeilen model=, hiddenSelections[] und
     hiddenSelectionsTextures[].

WARUM AUCH DIE TEXTUREN: ein Filet erbt von ChefZ_FreshFillet_Base/SeaFillet_Base die
Selection "cs_raw" mit den Vanilla-Texturen von Karpfen und Makrele. Wer nur model= umbiegt,
legt eine Makrelen-Textur auf das UV-Layout des neuen Modells - im Spiel ein bunter Fleck,
in keinem Log ein Wort. Deshalb bekommt jede gebundene Klasse ihre eigenen Texturen
(Filets: roh/gebacken/gekocht/getrocknet/verbrannt in der Reihenfolge der Vanilla-Filets).

Ohne --apply wird nur der Diff gezeigt. Exit-Code 1, wenn eine Klasse nicht gefunden wird.

    py -3 tools/chefz-assets/bind-delivery.py tools/chefz-assets/deliveries/<lieferung>.report.json [--apply]
"""
import argparse
import datetime
import difflib
import json
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ADDONS = ROOT / "ChefZ_Core" / "Addons"
STATE_ORDER = ["baked", "boiled", "dried", "burnt"]   # nach "roh", wie mackerel_fillet_*_CO.paa


def read_text(path: Path) -> tuple[list[str], str]:
    raw = path.read_bytes().decode("utf-8")
    newline = "\r\n" if "\r\n" in raw else "\n"
    return raw.replace("\r\n", "\n").split("\n"), newline


def find_class(lines: list[str], name: str):
    """(start, open_brace_line, end) of 'class <name> : X' / 'class <name>' with a body, else None."""
    head = re.compile(rf"^\s*class\s+{re.escape(name)}\b(\s*:\s*\w+)?\s*(\{{.*)?$")
    for index, line in enumerate(lines):
        if not head.match(line.split("//")[0]):
            continue
        opening = index if "{" in line else index + 1
        if opening >= len(lines) or "{" not in lines[opening]:
            continue   # forward declaration "class X;"
        depth = 0
        for end in range(opening, len(lines)):
            code = lines[end].split("//")[0]
            depth += code.count("{") - code.count("}")
            if depth == 0:
                return index, opening, end
    return None


def property_span(lines, opening, end, key):
    """Line range of a top-level property inside the class body (arrays may span lines)."""
    pattern = re.compile(rf"^\s*{re.escape(key)}\s*=")
    depth = 0
    for index in range(opening, end + 1):
        code = lines[index].split("//")[0]
        if depth == 1 and pattern.match(code):
            stop = index
            while ";" not in lines[stop].split("//")[0]:
                stop += 1
            return index, stop
        depth += code.count("{") - code.count("}")
    return None


def indent_of(lines, opening, end) -> str:
    for index in range(opening + 1, end):
        match = re.match(r"^(\s+)\w", lines[index])
        if match:
            return match.group(1)
    return re.match(r"^(\s*)", lines[opening]).group(1) + "    "


def array_lines(indent: str, key: str, values: list[str]) -> list[str]:
    quoted = [f'"{v}"' for v in values]
    if len(values) == 1:
        return [f"{indent}{key} = {{{quoted[0]}}};"]
    inner = indent + "    "
    return [f"{indent}{key} ="] + [f"{indent}{{"] + \
        [f"{inner}{q}{',' if i < len(quoted) - 1 else ''}" for i, q in enumerate(quoted)] + [f"{indent}}};"]


def set_property(lines, name, key, new_lines) -> list[str]:
    start, opening, end = find_class(lines, name)
    span = property_span(lines, opening, end, key)
    if span:
        return lines[:span[0]] + new_lines + lines[span[1] + 1:]
    anchor = property_span(lines, opening, end, "model") or property_span(lines, opening, end, "scope")
    at = anchor[1] + 1 if anchor else opening + 1
    return lines[:at] + new_lines + lines[at:]


def bind(lines, name, result, today) -> list[str]:
    span = find_class(lines, name)
    indent = indent_of(lines, span[1], span[2])
    model_line = f'{indent}model = "{result["model"]}";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden {today})'
    lines = set_property(lines, name, "model", [model_line])
    textures = [result["texture"]] + [t for state in STATE_ORDER for t in result.get("state_textures", [])
                                      if t.endswith(f"_{state}_co.paa")]
    selection = result.get("selection") or ("cs_raw" if result.get("state_textures") else "camo")
    lines = set_property(lines, name, "hiddenSelections[]", array_lines(indent, "hiddenSelections[]", [selection]))
    lines = set_property(lines, name, "hiddenSelectionsTextures[]",
                         array_lines(indent, "hiddenSelectionsTextures[]", textures))
    return lines


def package_copy(result, apply: bool) -> list[str]:
    """ChefZ/<Paket>/<rest> -> ChefZ_Core/Addons/<Paket>/<rest> for the P3D and every PAA."""
    copied = []
    for source in [result["p3d"], *result.get("textures", [])]:
        source = Path(source)
        relative = source.relative_to(ROOT / "ChefZ")
        target = ADDONS / relative
        copied.append(str(target.relative_to(ROOT)))
        if apply:
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
    return copied


def main(argv=None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("report")
    parser.add_argument("--apply", action="store_true", help="Dateien kopieren und config.cpp schreiben")
    args = parser.parse_args(argv)
    report = json.loads(Path(args.report).read_text(encoding="utf-8"))
    today = datetime.date.today().strftime("%d.%m.%Y")
    configs = {path: read_text(path) for path in ADDONS.glob("*/config.cpp")}
    changed, missing, unbound, copied = {}, [], [], 0
    for result in report["results"]:
        if result["status"] not in ("OK", "WARNING"):
            continue
        copied += len(package_copy(result, args.apply))
        if not result.get("bind"):
            unbound.append(result["name"] + (f" ({result['note']})" if result.get("note") else ""))
            continue
        for name in result["bind"]:
            path = next((p for p, (lines, _) in configs.items() if find_class(lines, name)), None)
            if path is None:
                missing.append(f"{name} ({result['name']})")
                continue
            lines, newline = configs[path]
            configs[path] = (bind(lines, name, result, today), newline)
            changed[path] = True
    for path in changed:
        old, _ = read_text(path)
        new, newline = configs[path]
        diff = difflib.unified_diff(old, new, str(path.relative_to(ROOT)), str(path.relative_to(ROOT)), lineterm="", n=1)
        print("\n".join(diff))
        if args.apply:
            path.write_bytes(newline.join(new).encode("utf-8"))
    mode = "geschrieben" if args.apply else "Vorschau (ohne --apply)"
    print(f"\n{mode}: {len(changed)} config.cpp geaendert, {copied} Dateien "
          f"{'kopiert' if args.apply else 'zu kopieren'} nach ChefZ_Core/Addons")
    if unbound:
        print(f"ohne Klasse gebaut ({len(unbound)}): " + ", ".join(unbound))
    if missing:
        print(f"KLASSE NICHT GEFUNDEN ({len(missing)}): " + ", ".join(missing))
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
