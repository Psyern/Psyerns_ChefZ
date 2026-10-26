"""Whole-item energy balance of every ChefZ recipe: what goes in vs. what comes out.

The boot audit of ChefZ_NutritionManager (NUTRI channel) adds the per-100 energy of the
RAW ingredients times their piece count and holds that against the per-100 energy of the
dish. It mixes densities with totals and ignores the item size, so its percentages are
no balancing statement (a 200-unit plate against two raw 20/100 squids reads +1465 %).

This report counts energy the way the stomach does (PlayerStomach: energy / 100 per
consumed quantity unit) for whole items:

    in   = sum over required slots: minCount x E/item of the slot's ingredient at its
           best edible stage, or consumeAmount x energy / 100 for "amount" slots.
           A slot with several candidates uses their median.
    out  = sum over outputs: quantity x energy / 100 of the dish.

Run:  py -3 tools/chefz-autotest/recipe_energy_check.py [filter]
"""
import glob
import os
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import chefz_index as ci  # noqa: E402
import check_vessel_fit as vf  # noqa: E402
import make_cases as mc  # noqa: E402
import nutrition_report as nr  # noqa: E402


def item_energy(db, cls):
    """(energy per 100 units, units of a full item) at the best edible stage, or None."""
    _, n = nr.best(db, cls)
    if not n or n[1] <= 0:
        return None
    qmax = nr.get(db, cls, "qmax") or 0
    return n[1], (qmax if qmax > 0 else 100)


def slot_energy(ix, db, s):
    cls, _, _, excl = mc.candidates(ix, s["match"])
    vals = []
    for c in sorted((cls or set()) - excl):
        e = item_energy(db, c)
        if not e:
            continue
        per100, units = e
        if s.get("consume") == "amount":
            vals.append(per100 * s.get("consumeAmount", 0) / 100.0)
        else:
            vals.append(per100 * units / 100.0 * max(1, s.get("minCount", 1)))
    if not vals:
        return None, 0
    return statistics.median(vals), len(vals)


def main():
    flt = sys.argv[1] if len(sys.argv) > 1 else ""
    ix = ci.Index()
    van = glob.glob(os.path.join(vf.VANILLA_DZ, "gear", "food", "config.cpp"))
    chz = glob.glob(os.path.join(ci.ADDONS, "*", "config.cpp"))
    db = nr.parse(van)
    db.update(nr.parse(chz))

    rows = []
    for kind in ("recipe", "transform"):
        for r in ix.kind(kind):
            if flt and flt.lower() not in r["id"].lower():
                continue
            e_in = 0.0
            gaps = []
            for s in r.get("slots", r.get("inputs", [])):
                if s.get("optional") or s.get("minCount", 1) <= 0 or "match" not in s:
                    continue
                v, _ = slot_energy(ix, db, s)
                if v is None:
                    gaps.append(s.get("slotId", "?"))
                    continue
                e_in += v
            if any(o.get("quantityMode") == "ratio" for o in r.get("outputs", [])):
                continue  # output scales with the consumed amount, no fixed item
            e_out = 0.0
            for o in r.get("outputs", []):
                e = item_energy(db, o.get("cls", ""))
                if not e:
                    continue
                qty = o.get("quantity", 0) or e[1]
                e_out += e[0] * qty / 100.0 * max(1, o.get("count", 1))
            if e_out <= 0:
                continue
            ratio = e_out / e_in if e_in > 0 else float("inf")
            rows.append((r["id"], e_in, e_out, ratio, ",".join(gaps)))

    rows.sort(key=lambda t: -t[3])
    print("%-40s %8s %8s %7s  %s" % ("recipe", "E in", "E out", "out/in", "slots without data"))
    for rid, a, b, ratio, gaps in rows:
        print("%-40s %8.0f %8.0f %7.2f  %s" % (rid, a, b, ratio, gaps))


if __name__ == "__main__":
    main()
