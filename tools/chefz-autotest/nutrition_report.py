"""Compares the nutrition of ChefZ foods with vanilla foods.

Per class: the nutrition of the best edible stage (Baked/Boiled/Dried, else Raw, else
class Nutrition), the quantity and from that the energy and water of one whole item
(vanilla PlayerStomach: energy / 100 per consumed unit).

Run:  py -3 tools/chefz-autotest/nutrition_report.py [filter]
"""
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import chefz_index as ci  # noqa: E402
import check_vessel_fit as vf  # noqa: E402

STAGES = ("Baked", "Boiled", "Dried", "Raw")


def stage_body(body, st):
    m = re.search(r"class\s+" + st + r"\s*\{", body)
    if not m:
        return None
    j = m.end()
    d = 1
    while j < len(body) and d:
        if body[j] == "{":
            d += 1
        elif body[j] == "}":
            d -= 1
        j += 1
    return body[m.end():j - 1]


def parse(files):
    db = {}
    for f in files:
        with open(f, encoding="utf-8", errors="replace") as fh:
            text = ci._strip_comments(fh.read())
        sec = ci._section(text, "CfgVehicles")
        if not sec:
            continue
        i = 0
        while True:
            m = ci._CLASS_RE.search(sec, i)
            if not m:
                break
            j = m.end()
            d = 1
            while j < len(sec) and d:
                if sec[j] == "{":
                    d += 1
                elif sec[j] == "}":
                    d -= 1
                j += 1
            body = sec[m.end():j - 1]
            info = {"base": m.group(2), "src": f}
            q = re.search(r"\bvarQuantityMax\s*=\s*([\d.]+)", body)
            if q:
                info["qmax"] = float(q.group(1))
            n = re.search(r"class\s+Nutrition\s*\{([^}]*)\}", body)
            if n:
                vals = dict(re.findall(r"(\w+)\s*=\s*([-\d.]+)", n.group(1)))
                info["nut"] = (float(vals.get("fullnessIndex", 0)), float(vals.get("energy", 0)), float(vals.get("water", 0)), float(vals.get("nutritionalIndex", 0)))
            for st in STAGES:
                sb = stage_body(body, st)
                sm = re.search(r"nutrition_properties\[\]\s*=\s*\{([^}]*)\}", sb) if sb else None
                if sm:
                    v = [float(x) for x in sm.group(1).split(",")[:4]]
                    info.setdefault("stages", {})[st] = tuple(v)
            db[m.group(1)] = info
            i = j
    return db


def get(db, cls, key):
    seen = set()
    while cls in db and cls not in seen:
        seen.add(cls)
        if key in db[cls]:
            return db[cls][key]
        cls = db[cls]["base"]
    return None


def best(db, cls):
    st = get(db, cls, "stages")
    if st:
        for s in STAGES:
            if s in st:
                return s, st[s]
    n = get(db, cls, "nut")
    if n:
        return "Nutr", n
    return None, None


def main():
    flt = sys.argv[1] if len(sys.argv) > 1 else ""
    van = glob.glob(os.path.join(vf.VANILLA_DZ, "gear", "food", "config.cpp"))
    chz = glob.glob(os.path.join(ci.ADDONS, "*", "config.cpp"))
    vdb = parse(van)
    db = dict(vdb)
    db.update(parse(chz))
    rows = []
    for cls, info in db.items():
        if flt and flt.lower() not in cls.lower():
            continue
        stage, n = best(db, cls)
        if not n or n[1] <= 0:
            continue
        qmax = get(db, cls, "qmax") or 0
        units = qmax if qmax > 0 else 100
        origin = "CHEFZ" if cls in db and info["src"] in chz else "VAN"
        rows.append((origin, cls, stage, n[0], n[1], n[2], n[3], units, n[1] * units / 100, n[2] * units / 100))
    rows.sort(key=lambda r: -r[8])
    print("%-5s %-34s %-6s %6s %6s %6s %4s %6s %8s %8s" % ("src", "class", "stage", "fullI", "energy", "water", "nutI", "units", "E/item", "W/item"))
    for r in rows:
        print("%-5s %-34s %-6s %6.1f %6.0f %6.0f %4.0f %6.0f %8.0f %8.0f" % r)


if __name__ == "__main__":
    main()
