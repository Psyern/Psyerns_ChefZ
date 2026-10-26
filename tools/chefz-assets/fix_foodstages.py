r"""Ergaenzt fehlende cooking_properties/visual_properties in FoodStages.

Verschachtelte Config-Klassen erben NICHT: definiert eine abgeleitete Klasse
"class Food { class FoodStages { class Raw { nutrition_properties[] = ...; }; ... }; };"
neu, fehlen ihr cooking_properties und visual_properties der Basis. Vanillas
Cooking.UpdateCookingState liest dann ueber FoodStage.GetAllCookingPropertiesForStage
null und wirft bei jedem Kochtakt einen NULL-Pointer - das Item garte nie.

Das Skript uebernimmt je Stufe die fehlenden Eintraege aus der naechsten
Vorfahrenklasse, die sie hat, und faellt sonst auf Vanillawerte zurueck. Fehlt
einer Klasse eine Stufe, die ein Vorfahre kennt, wird sie komplett uebernommen.

Aufruf:  py -3 tools/chefz-assets/fix_foodstages.py [--check]
"""
import glob
import os
import re
import sys

ADDONS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "ChefZ_Core", "Addons")
PROPS = ("visual_properties", "cooking_properties")
VANILLA = {
    "Raw": {"visual_properties": "{0, 0, 0}", "cooking_properties": "{0, 0}"},
    "Rotten": {"visual_properties": "{-1, -1, 5}", "cooking_properties": "{0, 0}"},
    "Baked": {"visual_properties": "{0, 1, 1}", "cooking_properties": "{70, 45}"},
    "Boiled": {"visual_properties": "{0, 2, 2}", "cooking_properties": "{105, 55}"},
    "Dried": {"visual_properties": "{0, 3, 3}", "cooking_properties": "{70, 45, 80}"},
    "Burned": {"visual_properties": "{0, 4, 4}", "cooking_properties": "{100, 20}"},
}


def code_mask(text):
    """True per character that is code (not comment, not string)."""
    mask = [True] * len(text)
    i = 0
    n = len(text)
    while i < n:
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                mask[k] = False
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                mask[k] = False
            i = j
        elif text[i] == '"':
            j = i + 1
            while j < n and text[j] != '"':
                j += 1
            for k in range(i, min(j + 1, n)):
                mask[k] = False
            i = j + 1
        else:
            i += 1
    return mask


CLASS_RE = re.compile(r"\bclass\s+(\w+)\s*(?::\s*(\w+))?\s*(\{|;)")


def children(text, mask, start, end):
    """Top-level class blocks between start and end: (name, base, open_idx, close_idx)."""
    out = []
    i = start
    while True:
        m = CLASS_RE.search(text, i, end)
        if not m:
            return out
        if not mask[m.start()]:
            i = m.end()
            continue
        if m.group(3) == ";":
            i = m.end()
            continue
        o = m.end() - 1
        d = 1
        j = o + 1
        while j < end and d:
            if mask[j]:
                if text[j] == "{":
                    d += 1
                elif text[j] == "}":
                    d -= 1
            j += 1
        out.append((m.group(1), m.group(2), o, j - 1))
        i = j


def find_child(text, mask, o, c, name):
    for n, b, oo, cc in children(text, mask, o + 1, c):
        if n == name:
            return oo, cc
    return None


def collect():
    """class name -> dict(file, base, stages{name: (o, c, body)} or None)"""
    db = {}
    files = {}
    for f in sorted(glob.glob(os.path.join(ADDONS, "*", "config.cpp"))):
        text = open(f, encoding="utf-8", newline="").read()
        mask = code_mask(text)
        files[f] = text
        m = re.search(r"\bclass\s+CfgVehicles\s*\{", text)
        if not m:
            continue
        cv = children(text, mask, 0, len(text))
        for n, b, o, c in cv:
            if n != "CfgVehicles":
                continue
            for cn, cb, co, cc in children(text, mask, o + 1, c):
                stages = None
                fst = None
                food = find_child(text, mask, co, cc, "Food")
                if food:
                    for fn, fb, fo, fc in children(text, mask, food[0] + 1, food[1]):
                        if fn == "FoodStageTransitions":
                            hdr = text.rfind("class", 0, fo)
                            end = fc + 1
                            if text[end:end + 1] == ";":
                                end += 1
                            fst = text[hdr:end]
                    if fst is None:
                        m2 = re.search(r"class\s+FoodStageTransitions\s*:\s*\w+\s*;", text[food[0]:food[1]])
                        if m2:
                            fst = m2.group(0)
                    fs = find_child(text, mask, food[0], food[1], "FoodStages")
                    if fs:
                        stages = {}
                        for sn, sb, so, sc in children(text, mask, fs[0] + 1, fs[1]):
                            stages[sn] = (so, sc, text[so + 1:sc])
                        stages["__fs__"] = fs
                db.setdefault(cn, {"file": f, "base": cb, "stages": stages, "food": food, "fst": fst})
    return db, files


def prop(body, key):
    m = re.search(r"\b" + key + r"\s*\[\]\s*=\s*(\{[^}]*\})", body)
    return m.group(1) if m else None


def ancestor_value(db, cls, stage, key, seen=()):
    """value of key in stage, searching cls's ancestors (excluding cls itself)."""
    base = db.get(cls, {}).get("base")
    while base and base in db and base not in seen:
        st = db[base]["stages"]
        if st and stage in st:
            v = prop(st[stage][2], key)
            if v:
                return v
        seen = seen + (base,)
        base = db[base]["base"]
    return None


def ancestor_stages(db, cls):
    base = db.get(cls, {}).get("base")
    seen = ()
    while base and base in db and base not in seen:
        st = db[base]["stages"]
        if st:
            return base, st
        seen = seen + (base,)
        base = db[base]["base"]
    return None, None


def ancestor_fst(db, cls):
    base = db.get(cls, {}).get("base")
    seen = ()
    while base and base in db and base not in seen:
        if db[base].get("fst"):
            return base, db[base]["fst"]
        if db[base].get("food"):
            return None, None      # Vorfahr definiert Food ohne Uebergaenge - dort reparieren
        seen = seen + (base,)
        base = db[base]["base"]
    return None, None


def main():
    check = "--check" in sys.argv
    db, files = collect()
    edits = {}  # file -> list of (pos, text)
    report = []
    for cls, info in sorted(db.items()):
        st = info["stages"]
        if not st:
            continue
        f = info["file"]
        text = files[f]
        for sname, val in st.items():
            if sname == "__fs__":
                continue
            so, sc, body = val
            add = []
            for key in PROPS:
                if prop(body, key):
                    continue
                v = ancestor_value(db, cls, sname, key) or VANILLA.get(sname, {}).get(key)
                if v:
                    add.append(key + "[] = " + v + ";")
            if not add:
                continue
            report.append(cls + "." + sname + ": " + ", ".join(a.split("[")[0] for a in add))
            one_line = "\n" not in text[so:sc]
            if one_line:
                ins = " ".join(add) + " "
                pos = sc
                # "class Raw { a; };" -> put before the closing brace
                while pos > so and text[pos - 1] == " ":
                    pos -= 1
                ins = " " + " ".join(add)
            else:
                line_start = text.rfind("\n", 0, sc) + 1
                indent = text[line_start:sc]
                nl = "\r\n" if "\r\n" in text else "\n"
                ins = "".join(indent + "    " + a + nl for a in add)
                pos = line_start
            edits.setdefault(f, []).append((pos, ins))
        # FoodStageTransitions der Basis gehen mit einem eigenen Food verloren
        if info.get("food") and not info.get("fst"):
            anc_f, fst_text = ancestor_fst(db, cls)
            if fst_text:
                fo, fc = info["food"]
                line_start = text.rfind("\n", 0, fc) + 1
                indent = text[line_start:fc]
                nl = "\r\n" if "\r\n" in text else "\n"
                lines = fst_text.replace("\r\n", "\n").split("\n")
                # Einrueckung der Vorlage auf die des Ziels umsetzen
                strip_n = 0
                if len(lines) > 1:
                    strip_n = len(lines[-1]) - len(lines[-1].lstrip())
                out_lines = []
                for i, ln in enumerate(lines):
                    if i == 0:
                        out_lines.append(indent + "    " + ln.strip())
                    else:
                        out_lines.append(indent + "    " + ln[strip_n:])
                blob = nl.join(out_lines) + nl
                edits.setdefault(f, []).append((line_start, blob))
                report.append(cls + ".FoodStageTransitions: von " + anc_f)
        # stages known to the ancestor but missing here
        anc, ast = ancestor_stages(db, cls)
        if ast:
            missing = [s for s in ast if s != "__fs__" and s not in st]
            if missing:
                fs_o, fs_c = st["__fs__"]
                line_start = text.rfind("\n", 0, fs_c) + 1
                indent = text[line_start:fs_c]
                nl = "\r\n" if "\r\n" in text else "\n"
                blob = ""
                for s in missing:
                    sbody = files[db[anc]["file"]][ast[s][0] + 1:ast[s][1]]
                    parts = [p.strip() for p in sbody.split(";") if p.strip() and not p.strip().startswith("//")]
                    blob += indent + "    class " + s + " { " + " ".join(p + ";" for p in parts) + " };" + nl
                    report.append(cls + "." + s + ": ganze Stufe von " + anc)
                edits.setdefault(f, []).append((line_start, blob))
    for line in report:
        print(line)
    print(len(report), "Ergaenzungen")
    if check:
        sys.exit(1 if report else 0)
    for f, lst in edits.items():
        text = files[f]
        for pos, ins in sorted(lst, key=lambda x: -x[0]):
            text = text[:pos] + ins + text[pos:]
        open(f, "w", encoding="utf-8", newline="").write(text)
        print("geschrieben:", os.path.relpath(f, ADDONS))


if __name__ == "__main__":
    main()
