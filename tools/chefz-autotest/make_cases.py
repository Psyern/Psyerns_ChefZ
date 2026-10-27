"""Generates the concrete test cases for ChefZ_AutoTest from the repo data.

For every cooking recipe and every transform it picks one concrete class per required
slot (category/tag resolved over the ingredient index), the device or station and the
liquid, and writes them as Enforce code:

    ChefZ_AutoTest/Scripts/5_Mission/ChefZ_AutoTest/ChefZ_AT_Cases.c

Run:  py -3 tools/chefz-autotest/make_cases.py
The in-game harness only executes; which inputs it uses is decided here and is readable
in the generated file.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import chefz_index as ci  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ChefZ_AutoTest", "Scripts", "5_Mission", "ChefZ_AutoTest", "ChefZ_AT_Cases.c")

# Classes that must never be chosen as a generic candidate: the test spawns raw items,
# so prefer things that exist as simple spawnable items.
AVOID = {"ChefZ_EmptyPlate", "ChefZ_EmptyBowl", "ChefZ_EmptyCan"}

# Overrides for slots where the cheapest candidate is a poor representative.
PREFER = ["Fillet", "Potato", "Water"]


class Pick:
    def __init__(self, cls, state="", vstage="", qpct=-1.0):
        self.cls = cls
        self.state = state
        self.vstage = vstage
        self.qpct = qpct


def candidates(ix, sel):
    """Returns (set of classes or None=any, state, vstage, excluded set)."""
    state = ""
    vstage = ""
    excl = set()
    if "cls" in sel:
        cls = {sel["cls"]}
    elif "category" in sel:
        cls = {c for c in ix.ingredients if sel["category"] in ix.categories_of(c)}
    elif "tag" in sel:
        cls = {c for c in ix.ingredients if sel["tag"] in ix.tags_of(c)}
    elif "anyOf" in sel:
        cls = set()
        for s in sel["anyOf"]:
            c, st, vs, ex = candidates(ix, s)
            if c is None:
                continue
            cls |= (c - ex)
            state = state or st
            vstage = vstage or vs
    elif "allOf" in sel:
        cls = None
        for s in sel["allOf"]:
            c, st, vs, ex = candidates(ix, s)
            state = state or st
            vstage = vstage or vs
            excl |= ex
            if c is not None:
                cls = c if cls is None else (cls & c)
    elif "not" in sel:
        c, _, _, _ = candidates(ix, sel["not"])
        return None, "", "", (c or set())
    else:
        cls = None
    if "state" in sel:
        state = sel["state"]
    if "vanillaStage" in sel:
        vstage = sel["vanillaStage"]
    return cls, state, vstage, excl


def qty_pct(sel):
    """Fuellstand, den ein Slot mit quantityPct verlangt: Mitte des Bereichs."""
    for s in [sel] + sel.get("allOf", []):
        r = s.get("quantityPct")
        if r:
            lo = r.get("min", 0.0)
            hi = r.get("max", 1.0)
            return (lo + hi) / 2.0
    return -1.0


def choose(ix, sel, avoid):
    p = _choose(ix, sel, avoid)
    if p:
        p.qpct = qty_pct(sel)
    return p


def _choose(ix, sel, avoid):
    cls, state, vstage, excl = candidates(ix, sel)
    if cls is None:
        return None
    pool = sorted(c for c in cls - excl - avoid - AVOID)
    if not pool:
        pool = sorted(cls - excl)
    if not pool:
        return None
    explicit = [c for c in pool if c == sel.get("cls")]
    if explicit:
        return Pick(explicit[0], state, vstage)
    for word in PREFER:
        for c in pool:
            if word in c:
                return Pick(c, state, vstage)
    vanilla = [c for c in pool if not c.startswith("ChefZ_")]
    return Pick((vanilla or pool)[0], state, vstage)


def q(s):
    return '"' + s.replace('"', "") + '"'


_SIZES = None


def fit_inputs(ix, r, case):
    """Falls die gewaehlten Zutaten nicht ins Gefaess passen: je Slot die kleinste Klasse."""
    global _SIZES
    import check_vessel_fit as vf
    if case["device"] not in vf.CARGO:
        return
    if _SIZES is None:
        _SIZES = vf.item_sizes()
    w, h = vf.CARGO[case["device"]]
    area = lambda cls: (lambda z: z[0] * z[1])(vf.size_of(_SIZES, cls) or (1, 1))
    # Grosse Items zuerst: CreateEntityInCargo fuellt der Reihe nach.
    case["inputs"].sort(key=lambda pn: -area(pn[0].cls))
    need = []
    for p, n in case["inputs"]:
        need += [vf.size_of(_SIZES, p.cls) or (1, 1)] * n
    # Ohne Drehen: das Spawnen in den Laderaum dreht nicht von selbst.
    if vf.packs(need, w, h, rotate=False):
        return
    inputs = []
    for s in r["slots"]:
        if s.get("optional") or s.get("minCount", 1) <= 0:
            continue
        cls, state, vstage, excl = candidates(ix, s["match"])
        pool = sorted((cls or set()) - excl - AVOID)
        sized = [(vf.size_of(_SIZES, c), c) for c in pool]
        sized = [(z, c) for z, c in sized if z]
        if not sized:
            continue
        z, c = min(sized, key=lambda t: (t[0][0] * t[0][1], t[1]))
        inputs.append((Pick(c, state, vstage), max(1, s.get("minCount", 1))))
    inputs.sort(key=lambda pn: -area(pn[0].cls))
    case["inputs"] = inputs


def named_classes(sel):
    if "cls" in sel:
        return [sel["cls"]]
    out = []
    for k in ("allOf", "anyOf"):
        for x in sel.get(k, []):
            out += named_classes(x)
    return out


def recipe_cases(ix):
    out = []
    for r in ix.kind("recipe"):
        ctx = r["contexts"][0]
        outs = [o["cls"] for o in r.get("outputs", [])]
        case = {"kind": "COOK", "id": r["id"], "device": ctx["deviceClasses"][0], "method": ctx["methods"][0],
                "liquid": "", "liquidQty": 0, "inputs": [], "outputs": outs, "note": "", "process": "", "seconds": r.get("cookSeconds", 0)}
        if not ctx.get("requiresLiquid") and case["method"] == "BOILING":
            # Kochen in Wasser ohne Mengenvorgabe: ohne Wasser backt Vanilla.
            case["liquid"] = "Water"
            case["liquidQty"] = 1000
        if ctx.get("requiresLiquid"):
            case["liquid"] = ctx.get("liquidTypes", ["Water"])[0]
            lq = ctx.get("liquidQuantity", {}).get("min", 200)
            case["liquidQty"] = max(lq, r.get("policy", {}).get("liquidConsumed", 0)) + 500
        for s in r["slots"]:
            if s.get("optional") or s.get("minCount", 1) <= 0:
                continue
            p = choose(ix, s["match"], set(outs))
            if not p:
                case["note"] += "kein Kandidat fuer Slot " + s["slotId"] + "; "
                continue
            case["inputs"].append((p, max(1, s.get("minCount", 1))))
        # Grosse Rezepte passen nicht in Vanillas Topf: dann der Kessel, falls erlaubt.
        if sum(n for _, n in case["inputs"]) >= 6 and "Cauldron" in ctx["deviceClasses"]:
            case["device"] = "Cauldron"
        fit_inputs(ix, r, case)
        out.append(case)
    return out


# Vanilla-Stufe, auf die ein ChefZ-Zustand projiziert (ChefZ_StateManager).
STAGE_OF_STATE = {"SMOKED": "DRIED", "DRIED": "DRIED"}


def transform_cases(ix):
    stations = {}
    for s in ix.kind("station"):
        for p in s["processes"]:
            stations.setdefault(p, []).append(s["id"])
    # Klassen, die ein Transform ausdruecklich nennt: ein allgemeiner Transform
    # desselben Prozesses wird mit einer anderen Klasse getestet, sonst gewinnt
    # zu Recht der spezifische.
    named = {}
    for t in ix.kind("transform"):
        for s in t["inputs"]:
            for c in named_classes(s["match"]):
                named.setdefault(t["process"], {}).setdefault(c, set()).add(t["id"])
    out = []
    for t in ix.kind("transform"):
        outs = [o["cls"] for o in t.get("outputs", []) if o.get("cls")]
        # Reiner Zustandswechsel (kein Output nennt eine Klasse): der Eingang
        # bleibt und steht danach in der projizierten Vanilla-Stufe.
        pure = not outs
        avoid = {c for c, ids in named.get(t["process"], {}).items() if ids - {t["id"]}}
        st = (t.get("stationsAllowed") or stations.get(t["process"]) or [""])[0]
        case = {"kind": "PROC", "id": t["id"], "device": st, "method": "", "liquid": "", "liquidQty": 0,
                "inputs": [], "outputs": outs, "note": "", "process": t["process"], "seconds": t.get("durationOverrideSec", 0),
                "byproducts": [b["cls"] for b in t.get("byproducts", [])], "expectStage": ""}
        if pure:
            states = [o.get("setState", "") for o in t.get("outputs", [])]
            case["expectStage"] = STAGE_OF_STATE.get(states[0] if states else "", "")
        for s in t["inputs"]:
            if s.get("optional") or s.get("minCount", 1) <= 0:
                continue
            m = s["match"]
            if m.get("isLiquidContainer"):
                case["inputs"].append((Pick("WaterBottle"), 1))
                case["liquid"] = m.get("liquidType", "Water")
                case["liquidQty"] = 1000
                continue
            p = choose(ix, m, set(outs) | avoid)
            if not p:
                case["note"] += "kein Kandidat fuer Slot " + s["slotId"] + "; "
                continue
            case["inputs"].append((p, max(1, s.get("minCount", 1))))
        if pure:
            case["outputs"] = sorted({p.cls for p, n in case["inputs"]})
        if st:
            sort_by_size(case)
        out.append(case)
    return out


def sort_by_size(case):
    """Grosse Items zuerst: CreateEntityInCargo fuellt der Reihe nach."""
    global _SIZES
    import check_vessel_fit as vf
    if _SIZES is None:
        _SIZES = vf.item_sizes()
    area = lambda cls: (lambda z: z[0] * z[1])(vf.size_of(_SIZES, cls) or (1, 1))
    case["inputs"].sort(key=lambda pn: -area(pn[0].cls))


def emit(cases):
    lines = [
        "//==============================================================================",
        "// GENERIERT von tools/chefz-autotest/make_cases.py - nicht von Hand aendern.",
        "// " + str(len(cases)) + " Faelle (Kochrezepte und Transformationen).",
        "//==============================================================================",
        "",
        "class ChefZ_AT_Cases",
        "{",
        "    static void Fill(notnull array<ref ChefZ_AT_Case> list)",
        "    {",
        "        ChefZ_AT_Case c;",
    ]
    for c in cases:
        lines.append("")
        lines.append("        c = ChefZ_AT_Case.Make(" + ", ".join([q(c["kind"]), q(c["id"]), q(c["device"]), q(c["method"]), q(c["process"])]) + ");")
        if c["liquid"]:
            lines.append("        c.SetLiquid(" + q(c["liquid"]) + ", " + str(float(c["liquidQty"])) + ");")
        if c["seconds"]:
            lines.append("        c.seconds = " + str(float(c["seconds"])) + ";")
        for p, n in c["inputs"]:
            lines.append("        c.AddInput(" + ", ".join([q(p.cls), str(n), q(p.state), q(p.vstage)]) + ");")
            if p.qpct >= 0:
                lines.append("        c.SetLastQtyPct(" + str(float(p.qpct)) + ");")
        for o in c["outputs"]:
            lines.append("        c.AddOutput(" + q(o) + ");")
        if c.get("expectStage"):
            lines.append("        c.expectStage = " + q(c["expectStage"]) + ";")
        for o in c.get("byproducts", []):
            lines.append("        c.AddByproduct(" + q(o) + ");")
        if c["note"]:
            lines.append("        c.note = " + q(c["note"]) + ";")
        lines.append("        list.Insert(c);")
    lines += ["    }", "}", ""]
    return "\r\n".join(lines)


FISH_OUT = os.path.join(os.path.dirname(OUT), "ChefZ_AT_FishData.c")


def emit_fish(ix):
    lines = [
        "//==============================================================================",
        "// GENERIERT von tools/chefz-autotest/make_cases.py - nicht von Hand aendern.",
        "// Fangertraege und Koeder aus den Fishing-JSONs.",
        "//==============================================================================",
        "",
        "class ChefZ_AT_FishData",
        "{",
        "    static void FillYields(notnull array<ref ChefZ_AT_Yield> list)",
        "    {",
    ]
    for y in ix.kind("fishingYield"):
        lines.append("        list.Insert(ChefZ_AT_Yield.Make(" + ", ".join([q(y["id"]), q(y.get("enviro", "BOTH")), q(" ".join(y.get("methods", []))), str(bool(y.get("lureOnly"))).lower()]) + "));")
    lines += ["    }", "", "    static void FillBaits(notnull array<ref ChefZ_AT_Bait> list)", "    {", "        ChefZ_AT_Bait b;"]
    for b in ix.kind("bait"):
        lines.append("        b = ChefZ_AT_Bait.Make(" + q(b["id"]) + ", " + q(b.get("baitKind", "soft")) + ");")
        for t in b.get("targets", []):
            lines.append("        b.targets.Insert(" + q(t) + ");")
        lines.append("        list.Insert(b);")
    lines += ["    }", "}", ""]
    with open(FISH_OUT, "w", encoding="utf-8", newline="") as fh:
        fh.write("\r\n".join(lines))
    print(len(ix.kind("fishingYield")), "yields,", len(ix.kind("bait")), "baits ->", FISH_OUT)


def main():
    ix = ci.Index()
    emit_fish(ix)
    cases = recipe_cases(ix) + transform_cases(ix)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="") as fh:
        fh.write(emit(cases))
    bad = [c for c in cases if c["note"]]
    print(len(cases), "cases ->", OUT)
    for c in bad:
        print("  NOTE", c["id"], c["note"])


if __name__ == "__main__":
    main()
