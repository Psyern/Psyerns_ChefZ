"""Raises the energy of ChefZ outputs that return less than their ingredients.

Targets (whole items, as recipe_energy_check.py counts them):
    dishes        (output with returnContainer: plate, bowl, can)   out >= DISH x in
    intermediates (everything else that is food)                    left as they are

Only raises, never lowers. Only touches ChefZ classes that already carry energy, and the
value that the item is eaten at (class Nutrition, else the best stage's
nutrition_properties). Shared outputs take the highest requirement over all recipes.
Iterates, because a raised intermediate raises the input of the dishes made from it.

Run:  py -3 tools/chefz-autotest/rebalance_energy.py          (dry run)
      py -3 tools/chefz-autotest/rebalance_energy.py --write
"""
import glob
import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import chefz_index as ci  # noqa: E402
import check_vessel_fit as vf  # noqa: E402
import nutrition_report as nr  # noqa: E402
import recipe_energy_check as rc  # noqa: E402

DISH = 1.3
# Intermediates are not raised: their staged values (Raw after processing, Baked/Boiled
# after cooking) belong together, and their loss is the cost of the next, better dish.
MID = 0.0

# Separation steps lose energy on purpose (whey, skim milk), and the culture is no food.
KEEP = {"ChefZ_Cream", "ChefZ_CheeseCurd", "ChefZ_MushroomCulture"}


def load_db():
    van = glob.glob(os.path.join(vf.VANILLA_DZ, "gear", "food", "config.cpp"))
    chz = glob.glob(os.path.join(ci.ADDONS, "*", "config.cpp"))
    db = nr.parse(van)
    db.update(nr.parse(chz))
    return db, set(chz)


def owner(db, cls, key):
    """Class that defines key for cls (walks the bases)."""
    seen = set()
    while cls in db and cls not in seen:
        seen.add(cls)
        if key in db[cls]:
            return cls
        cls = db[cls]["base"]
    return None


def energy_in(ix, db, r):
    total = 0.0
    for s in r.get("slots", r.get("inputs", [])):
        if s.get("optional") or s.get("minCount", 1) <= 0 or "match" not in s:
            continue
        v, _ = rc.slot_energy(ix, db, s)
        if v:
            total += v
    return total


def requirements(ix, db, chz):
    need = {}
    for kind in ("recipe", "transform"):
        for r in ix.kind(kind):
            e_in = energy_in(ix, db, r)
            outs = r.get("outputs", [])
            if e_in <= 0 or not outs:
                continue
            if any(o.get("quantityMode") == "ratio" for o in outs):
                continue
            units_total = 0.0
            per = []
            for o in outs:
                cls = o.get("cls", "")
                e = rc.item_energy(db, cls)
                if not e or db.get(cls, {}).get("src") not in chz:
                    per = None
                    break
                qty = (o.get("quantity", 0) or e[1]) * max(1, o.get("count", 1))
                units_total += qty
                per.append((cls, qty, "returnContainer" in o))
            if not per or len({c for c, _, _ in per}) != 1:
                continue  # several different output classes: no single knob
            cls, _, dish = per[0]
            if cls in KEEP:
                continue
            factor = DISH if dish else MID
            want = factor * e_in / (units_total / 100.0)
            if want > need.get(cls, (0, ""))[0]:
                need[cls] = (want, r["id"])
    return need


def edit(cls, new_energy, src):
    with open(src, encoding="utf-8", errors="replace", newline="") as fh:
        text = fh.read()
    m = re.search(r"class\s+" + cls + r"\s*(?::\s*\w+)?\s*\{", text)
    if not m:
        return False
    j = m.end()
    d = 1
    while j < len(text) and d:
        if text[j] == "{":
            d += 1
        elif text[j] == "}":
            d -= 1
        j += 1
    body = text[m.end():j - 1]
    nb = None
    # Same order as nutrition_report.best: a staged item is eaten at its stage.
    for st in nr.STAGES:
        sm = re.search(r"class\s+" + st + r"\s*\{", body)
        if not sm:
            continue
        pm = re.compile(r"(nutrition_properties\[\]\s*=\s*\{\s*[-\d.]+\s*,\s*)([-\d.]+)").search(body, sm.end())
        if pm:
            nb = body[:pm.start(2)] + fmt(new_energy) + body[pm.end(2):]
            break
    if nb is None:
        n = re.search(r"(class\s+Nutrition\s*\{[^}]*?energy\s*=\s*)([-\d.]+)", body)
        if n:
            nb = body[:n.start(2)] + fmt(new_energy) + body[n.end(2):]
    if nb is None:
        return False
    with open(src, "w", encoding="utf-8", newline="") as fh:
        fh.write(text[:m.end()] + nb + text[j - 1:])
    return True


def fmt(v):
    return str(int(v)) if float(v).is_integer() else str(v)


def main():
    write = "--write" in sys.argv
    ix = ci.Index()
    changes = {}
    for _ in range(6):
        db, chz = load_db()
        need = requirements(ix, db, chz)
        changed = False
        for cls, (want, rid) in sorted(need.items()):
            _, n = nr.best(db, cls)
            have = n[1]
            if have < 1:
                continue  # no food (spices, cultures): nothing to balance
            target = math.ceil(want / 5.0) * 5
            if have >= target:
                continue
            key = "stages" if nr.get(db, cls, "stages") else "nut"
            own = owner(db, cls, key)
            if own != cls:
                print("SKIP %-32s energy lives in base %s" % (cls, own))
                continue
            changes[cls] = (changes.get(cls, (have,))[0], target, rid)
            if write:
                if not edit(cls, target, db[cls]["src"]):
                    print("FAIL %-32s no energy field found" % cls)
                    continue
                changed = True
        if not write or not changed:
            break
    for cls, (old, new, rid) in sorted(changes.items()):
        print("%-32s energy %6.0f -> %6.0f  (%s)" % (cls, old, new, rid))


if __name__ == "__main__":
    main()
