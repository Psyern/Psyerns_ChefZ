"""Checks that every cooking recipe fits into its vessel at the minimum amounts.

Vanilla Pot, Cauldron and FryingPan all have a 4x3 cargo (12 cells). A recipe whose
required slots cannot be packed into that grid even with the smallest allowed class
per slot can never be cooked. Items may be rotated, as in the game inventory.

Run:  py -3 tools/chefz-autotest/check_vessel_fit.py
Exit code 1 when a recipe does not fit.
"""
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import chefz_index as ci  # noqa: E402
import make_cases as mc  # noqa: E402

VANILLA_DZ = r"E:\DayZ Modding\Mod Repositories\scripts (and more) - 1.30\DZ"
CARGO = {"Pot": (5, 4), "Cauldron": (6, 5), "FryingPan": (6, 5)}  # ChefZ_Cooking vergroessert den Laderaum
CLASS_RE = re.compile(r"\bclass\s+(\w+)\s*(?::\s*(\w+))?\s*\{")


def item_sizes():
    """class -> (base, itemSize or None) over vanilla and ChefZ configs (top-level CfgVehicles only)."""
    files = glob.glob(os.path.join(VANILLA_DZ, "**", "config.cpp"), recursive=True)
    files += glob.glob(os.path.join(ci.ADDONS, "*", "config.cpp"))
    db = {}
    for f in files:
        with open(f, encoding="utf-8", errors="replace") as fh:
            text = ci._strip_comments(fh.read())
        sec = ci._section(text, "CfgVehicles")
        if not sec:
            continue
        for name, (base, fields) in ci._parse_classes(sec).items():
            size = fields.get("itemSize")
            if isinstance(size, list) and len(size) == 2:
                size = (int(size[0]), int(size[1]))
            else:
                size = None
            old = db.get(name)
            if old and old[1] and not size:
                continue
            db[name] = (base or (old[0] if old else None), size)
    return db


def size_of(db, cls):
    seen = set()
    while cls and cls in db and cls not in seen:
        seen.add(cls)
        if db[cls][1]:
            return db[cls][1]
        cls = db[cls][0]
    return None


def packs(items, w, h, rotate=True):
    """Backtracking grid packing, optionally with rotation."""
    items = sorted(items, key=lambda s: -s[0] * s[1])
    if sum(a * b for a, b in items) > w * h:
        return False
    grid = [[False] * w for _ in range(h)]

    def free(x, y, a, b):
        if x + a > w or y + b > h:
            return False
        return all(not grid[y + j][x + i] for j in range(b) for i in range(a))

    def mark(x, y, a, b, v):
        for j in range(b):
            for i in range(a):
                grid[y + j][x + i] = v

    def rec(k):
        if k == len(items):
            return True
        a, b = items[k]
        for dims in ({(a, b), (b, a)} if rotate else {(a, b)}):
            for y in range(h):
                for x in range(w):
                    if free(x, y, *dims):
                        mark(x, y, *dims, True)
                        if rec(k + 1):
                            return True
                        mark(x, y, *dims, False)
        return False

    return rec(0)


def main():
    ix = ci.Index()
    db = item_sizes()
    bad = 0
    for r in ix.kind("recipe"):
        devs = [d for c in r["contexts"] for d in c["deviceClasses"] if d in CARGO]
        if not devs:
            continue
        need = []
        names = []
        for s in r["slots"]:
            if s.get("optional") or s.get("minCount", 1) <= 0:
                continue
            cls, _, _, excl = mc.candidates(ix, s["match"])
            pool = sorted((cls or set()) - excl)
            sized = [(size_of(db, c), c) for c in pool]
            sized = [(z, c) for z, c in sized if z]
            if not sized:
                continue
            z, c = min(sized, key=lambda t: t[0][0] * t[0][1])
            n = max(1, s.get("minCount", 1))
            need += [z] * n
            names.append(str(n) + "x" + c + str(z))
        w, h = CARGO[devs[-1]]
        if not packs(need, w, h):
            bad += 1
            cells = sum(a * b for a, b in need)
            print("PASST NICHT", r["id"], devs, str(cells) + "/" + str(w * h), " ".join(names))
    print(bad, "Rezepte passen nicht")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
