r"""Audit: Garstufen-Optik aller essbaren ChefZ-Items (27.10.2026).

Prueft je ChefZ-Klasse (scope 2, erbt von Edible_Base oder hat class Food):
  - model, hiddenSelections, hiddenSelectionsTextures/-Materials (vererbt aufgeloest)
  - FoodStages visual_properties je Stufe und ob die Indizes in die Arrays passen
  - ob jede referenzierte .p3d/.paa/.rvmat in BEIDEN Baeumen liegt
    (ChefZ_Core/Addons/<Addon> und ChefZ/<Addon>, falls das Addon dort existiert)
  - MLOD/ODOL, ob die Selektion im LOD 0 der .p3d existiert
  - ob eine model.cfg die Selektion als section fuehrt (ohne section kein Retexture)
  - ob die Stufen-Texturen sich vom Rohzustand unterscheiden (Dateiinhalt)

Aufruf:  py -3 tools/chefz-assets/stage_texture_audit.py [--md <datei>] [--json <datei>]
Liest nur, schreibt nur die angegebenen Ausgabedateien.
"""
import hashlib
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
CORE = os.path.join(ROOT, "ChefZ_Core", "Addons")
DELIV = os.path.join(ROOT, "ChefZ")
VANILLA = r"E:\DayZ Modding\Mod Repositories\scripts (and more) - 1.30"
VANILLA_CFGS = [os.path.join(VANILLA, "DZ", "gear", "food", "config.cpp"),
                os.path.join(VANILLA, "DZ", "gear_takistan", "food", "config.cpp")]
STAGES = ["Raw", "Baked", "Boiled", "Dried", "Burned", "Rotten"]

# ---------------------------------------------------------------- config parser
TOK = re.compile(r'"(?:[^"]|"")*"|[A-Za-z_][\w]*|-?\d+(?:\.\d+)?(?:e[-+]?\d+)?|\+=|[{}\[\];:=,]|\S', re.S)


def strip_comments(t):
    out = []
    i = 0
    n = len(t)
    while i < n:
        if t.startswith("//", i):
            j = t.find("\n", i)
            i = n if j < 0 else j
        elif t.startswith("/*", i):
            j = t.find("*/", i + 2)
            i = n if j < 0 else j + 2
        elif t[i] == '"':
            j = i + 1
            while j < n:
                if t[j] == '"':
                    if j + 1 < n and t[j + 1] == '"':
                        j += 2
                        continue
                    break
                j += 1
            out.append(t[i:j + 1])
            i = j + 1
        elif t[i] == "#":
            j = t.find("\n", i)
            i = n if j < 0 else j
        else:
            out.append(t[i])
            i += 1
    return "".join(out)


class Node:
    def __init__(self, name, base):
        self.name = name
        self.base = base
        self.props = {}
        self.kids = {}
        self.order = []


def parse(text):
    toks = TOK.findall(strip_comments(text))
    pos = [0]

    def peek(k=0):
        p = pos[0] + k
        return toks[p] if p < len(toks) else None

    def nxt():
        t = toks[pos[0]]
        pos[0] += 1
        return t

    def value():
        t = peek()
        if t == "{":
            nxt()
            arr = []
            while peek() != "}":
                if peek() == ",":
                    nxt()
                    continue
                arr.append(value())
            nxt()
            return arr
        t = nxt()
        if t.startswith('"'):
            return t[1:-1].replace('""', '"')
        # unquoted scalar, may be several tokens until ; , }
        parts = [t]
        while peek() not in (";", ",", "}", None):
            parts.append(nxt())
        return " ".join(parts)

    def body(node):
        while peek() is not None and peek() != "}":
            t = nxt()
            if t == ";":
                continue
            if t == "class":
                name = nxt()
                base = None
                if peek() == ":":
                    nxt()
                    base = nxt()
                if peek() == ";":
                    nxt()
                    continue  # forward decl
                if peek() != "{":
                    continue
                nxt()
                child = Node(name, base)
                body(child)
                if peek() == "}":
                    nxt()
                key = name.lower()
                if key in node.kids:
                    # same class twice in one file: merge (later wins)
                    old = node.kids[key]
                    old.props.update(child.props)
                    old.kids.update(child.kids)
                    if child.base:
                        old.base = child.base
                else:
                    node.kids[key] = child
                    node.order.append(key)
                continue
            if t == "delete":
                nxt()
                continue
            name = t
            arr = False
            if peek() == "[":
                nxt()
                nxt()
                arr = True
            op = nxt()
            if op not in ("=", "+="):
                continue
            v = value()
            node.props[name.lower()] = v
        return node

    root = Node("root", None)
    body(root)
    return root


# ---------------------------------------------------------------- file index
def build_index():
    idx = {}
    for tree, base in (("core", CORE), ("deliv", DELIV)):
        if not os.path.isdir(base):
            continue
        for addon in os.listdir(base):
            ad = os.path.join(base, addon)
            if not os.path.isdir(ad):
                continue
            for dp, dn, fn in os.walk(ad):
                for f in fn:
                    full = os.path.join(dp, f)
                    rel = os.path.relpath(full, ad).replace("/", "\\").lower()
                    idx.setdefault((tree, addon.lower(), rel), full)
    return idx


def addon_prefixes():
    out = {}
    for addon in os.listdir(CORE):
        p = os.path.join(CORE, addon, "$PREFIX$")
        if os.path.isfile(p):
            pre = open(p, encoding="utf-8-sig").read().strip().lower().rstrip("\\")
            out[pre] = addon
    return out


IDX = None
PREFIXES = None


def resolve(path):
    """-> dict(kind, addon, rel, core, deliv) ; core/deliv = full path or None/'n/a'."""
    p = path.strip().lstrip("\\").replace("/", "\\")
    pl = p.lower()
    if pl.startswith("dz\\"):
        full = os.path.join(VANILLA, p)
        return {"kind": "vanilla", "path": p, "core": full if os.path.isfile(full) else None, "deliv": "n/a"}
    best = None
    for pre, addon in PREFIXES.items():
        if pl.startswith(pre + "\\") and (best is None or len(pre) > len(best[0])):
            best = (pre, addon)
    if not best:
        return {"kind": "unknown", "path": p, "core": None, "deliv": None}
    rel = pl[len(best[0]) + 1:]
    addon = best[1]
    core = IDX.get(("core", addon.lower(), rel))
    deliv_exists = os.path.isdir(os.path.join(DELIV, addon))
    deliv = IDX.get(("deliv", addon.lower(), rel)) if deliv_exists else "n/a"
    return {"kind": "chefz", "addon": addon, "rel": rel, "path": p, "core": core, "deliv": deliv}


def fhash(full):
    if not full or full == "n/a" or not os.path.isfile(full):
        return None
    h = hashlib.md5()
    with open(full, "rb") as f:
        h.update(f.read())
    return h.hexdigest()


# ---------------------------------------------------------------- p3d
def mlod_lod0_selections(full):
    b = open(full, "rb").read()
    magic = b[:4].decode("latin1")
    if magic != "MLOD":
        sels = set(s.decode("latin1").lower() for s in re.findall(rb"[\x20-\x7e]{3,40}", b)
                   if re.fullmatch(rb"(cs_\w+|camo\w*|zbytek)", s, re.I))
        return magic, sels, None
    o = 12
    _, _, np_, nn, nf, _ = struct.unpack_from("<IIIIII", b, o + 4)
    o += 28 + np_ * 16 + nn * 12
    texs = set()
    for _ in range(nf):
        o += 72
        e = b.index(b"\0", o)
        texs.add(b[o:e].decode("latin1"))
        o = e + 1
        o = b.index(b"\0", o) + 1
    o += 4
    sels = set()
    while True:
        o += 1
        e = b.index(b"\0", o)
        name = b[o:e].decode("latin1")
        o = e + 1
        sz, = struct.unpack_from("<I", b, o)
        o += 4 + sz
        if name == "#EndOfFile#":
            break
        if not name.startswith("#"):
            sels.add(name.lower())
    return magic, sels, texs


def modelcfg_sections(p3d_full, addon_root):
    """Nearest model.cfg (model dir up to addon root). -> (cfgpath, sections or None)."""
    d = os.path.dirname(p3d_full)
    stem = os.path.splitext(os.path.basename(p3d_full))[0].lower()
    addon_root = os.path.normcase(os.path.abspath(addon_root))
    while True:
        cand = os.path.join(d, "model.cfg")
        if os.path.isfile(cand):
            root = parse(open(cand, encoding="utf-8", errors="replace").read())
            models = root.kids.get("cfgmodels")
            if not models:
                return cand, None
            cls = models.kids.get(stem)
            seen = set()
            while cls is not None and cls.name.lower() not in seen:
                seen.add(cls.name.lower())
                if "sections" in cls.props:
                    return cand, [s.lower() for s in cls.props["sections"]]
                cls = models.kids.get(cls.base.lower()) if cls.base else None
            return cand, None if models.kids.get(stem) is None else []
        if os.path.normcase(os.path.abspath(d)) == addon_root:
            return None, None
        nd = os.path.dirname(d)
        if nd == d:
            return None, None
        d = nd


# ---------------------------------------------------------------- class db
def load_classes():
    db = {}
    origin = {}
    for vc in VANILLA_CFGS:
        if os.path.isfile(vc):
            r = parse(open(vc, encoding="utf-8", errors="replace").read())
            cv = r.kids.get("cfgvehicles")
            if cv:
                for k in cv.order:
                    db[k] = cv.kids[k]
                    origin[k] = "vanilla"
    for addon in sorted(os.listdir(CORE)):
        cfg = os.path.join(CORE, addon, "config.cpp")
        if not os.path.isfile(cfg):
            continue
        r = parse(open(cfg, encoding="utf-8", errors="replace").read())
        cv = r.kids.get("cfgvehicles")
        if not cv:
            continue
        for k in cv.order:
            db[k] = cv.kids[k]
            origin[k] = addon
    return db, origin


def chain(db, name):
    out = []
    seen = set()
    k = name.lower()
    while k and k in db and k not in seen:
        seen.add(k)
        out.append(db[k])
        k = db[k].base.lower() if db[k].base else None
    return out


def inherited(db, name, key):
    for n in chain(db, name):
        if key in n.props:
            return n.props[key]
    return None


def food_node(db, name):
    for n in chain(db, name):
        if "food" in n.kids:
            return n.kids["food"]
    return None


def is_edible(db, name):
    ch = chain(db, name)
    names = [n.name.lower() for n in ch]
    if ch and ch[-1].base:
        names.append(ch[-1].base.lower())  # Basis ausserhalb der gelesenen Configs
    return "edible_base" in names or food_node(db, name) is not None


# ---------------------------------------------------------------- audit
def audit():
    global IDX, PREFIXES
    IDX = build_index()
    PREFIXES = addon_prefixes()
    db, origin = load_classes()
    rows = []
    for k, node in db.items():
        if origin[k] == "vanilla" or not node.name.startswith("ChefZ_"):
            continue
        scope = inherited(db, k, "scope")
        if str(scope) != "2":
            continue
        if not is_edible(db, k):
            continue
        model = inherited(db, k, "model") or ""
        sels = inherited(db, k, "hiddenselections") or []
        texs = inherited(db, k, "hiddenselectionstextures") or []
        mats = inherited(db, k, "hiddenselectionsmaterials") or []
        fn = food_node(db, k)
        stages = {}
        if fn and "foodstages" in fn.kids:
            fs = fn.kids["foodstages"]
            for st in STAGES:
                sn = fs.kids.get(st.lower())
                if sn is None:
                    stages[st] = None
                else:
                    vp = sn.props.get("visual_properties")
                    stages[st] = [int(float(x)) for x in vp] if isinstance(vp, list) else "MISSING"
        r = {"class": node.name, "addon": origin[k], "base": node.base, "model": model,
             "selections": sels, "textures": texs, "materials": mats, "stages": stages,
             "has_foodstages": bool(stages), "problems": []}
        # model
        mres = resolve(model) if model else None
        r["model_res"] = mres
        if mres and mres["kind"] == "chefz":
            if not mres["core"]:
                r["problems"].append("p3d fehlt in ChefZ_Core")
            if mres["deliv"] is None:
                r["problems"].append("p3d fehlt in ChefZ/ (Lieferbaum)")
            full = mres["core"] or (mres["deliv"] if mres["deliv"] not in (None, "n/a") else None)
            if full:
                magic, psels, ptex = mlod_lod0_selections(full)
                r["p3d_magic"] = magic
                r["p3d_sels"] = sorted(psels)
                for s in sels:
                    if s.lower() not in psels:
                        r["problems"].append("Selektion '%s' nicht im p3d (LOD0: %s)" % (s, ",".join(sorted(psels)) or "-"))
                cfgp, sections = modelcfg_sections(full, os.path.join(CORE, mres["addon"]) if full.startswith(CORE) else os.path.join(DELIV, mres["addon"]))
                r["modelcfg"] = os.path.relpath(cfgp, ROOT) if cfgp else None
                r["sections"] = sections
                if sels and magic == "MLOD":
                    if cfgp is None:
                        r["problems"].append("keine model.cfg -> Selektion ist keine section, Retexture wirkt nicht")
                    elif sections is None:
                        r["problems"].append("model.cfg ohne Klasse '%s' -> keine section" % os.path.splitext(os.path.basename(full))[0])
                    else:
                        for s in sels:
                            if s.lower() not in sections:
                                r["problems"].append("Selektion '%s' fehlt in model.cfg sections" % s)
        elif mres and mres["kind"] == "vanilla":
            r["p3d_magic"] = "ODOL(vanilla)"
            if not mres["core"]:
                r["problems"].append("vanilla p3d nicht im lokalen Dump (nur Hinweis)")
        # textures/materials
        tres = [resolve(t) for t in texs]
        mres_ = [resolve(m) for m in mats]
        r["tex_res"] = tres
        r["mat_res"] = mres_
        for lst, what in ((tres, "Textur"), (mres_, "Material")):
            for x in lst:
                if x["kind"] == "chefz":
                    if not x["core"]:
                        r["problems"].append("%s fehlt in ChefZ_Core: %s" % (what, x["path"]))
                    if x["deliv"] is None:
                        r["problems"].append("%s fehlt in ChefZ/: %s" % (what, x["path"]))
                elif x["kind"] == "vanilla" and not x["core"]:
                    r["problems"].append("%s nicht im Vanilla-Dump: %s" % (what, x["path"]))
                elif x["kind"] == "unknown":
                    r["problems"].append("%s mit unbekanntem Praefix: %s" % (what, x["path"]))
        hashes = [fhash(x["core"]) for x in tres]
        # stage checks
        distinct = {}
        for st, vp in stages.items():
            if vp is None:
                distinct[st] = "fehlt"
                continue
            if vp == "MISSING":
                r["problems"].append("%s ohne visual_properties" % st)
                distinct[st] = "fehlt"
                continue
            si, ti, mi = (vp + [0, 0, 0])[:3]
            if sels and si >= len(sels):
                r["problems"].append("%s: Selektionsindex %d >= %d" % (st, si, len(sels)))
            if texs and ti >= len(texs):
                r["problems"].append("%s: Texturindex %d >= %d Texturen" % (st, ti, len(texs)))
            if mats and mi >= len(mats):
                r["problems"].append("%s: Materialindex %d >= %d Materialien" % (st, mi, len(mats)))
            if sels and not mats:
                r["problems"].append("hiddenSelectionsMaterials leer -> FoodStage.UpdateVisualsEx Get(0) auf leerem Array")
            if not texs:
                distinct[st] = "keine Texturen"
            elif st == "Raw":
                distinct[st] = "-"
            else:
                t_ok = 0 <= ti < len(texs)
                h = hashes[ti] if t_ok else None
                if ti == 0 or (h and hashes[0] and h == hashes[0]):
                    distinct[st] = "wie roh"
                elif h is None:
                    distinct[st] = "?"
                else:
                    distinct[st] = "eigen"
        r["distinct"] = distinct
        r["problems"] = sorted(set(r["problems"]))
        rows.append(r)
    rows.sort(key=lambda r: (r["addon"], r["class"]))
    return rows


def files_ok(r):
    """'ok' / 'core!' / 'deliv!' fuer model+Texturen+Materialien."""
    allres = ([r["model_res"]] if r.get("model_res") else []) + r["tex_res"] + r["mat_res"]
    miss_core = sum(1 for x in allres if x["kind"] in ("chefz", "vanilla") and not x["core"])
    miss_del = sum(1 for x in allres if x["kind"] == "chefz" and x["deliv"] is None)
    if not miss_core and not miss_del:
        return "ok"
    out = []
    if miss_core:
        out.append("%d fehlen Core" % miss_core)
    if miss_del:
        out.append("%d fehlen ChefZ/" % miss_del)
    return ", ".join(out)


def section_state(r):
    if not r["selections"]:
        return "keine hiddenSelections"
    if r.get("p3d_magic", "").startswith("ODOL"):
        return "vanilla ODOL"
    if r.get("modelcfg") is None:
        return "KEINE model.cfg"
    sec = r.get("sections")
    if sec is None:
        return "model.cfg ohne Klasse"
    missing = [s for s in r["selections"] if s.lower() not in sec]
    return "ok" if not missing else "fehlt: " + ",".join(missing)


def write_md(rows, path):
    L = []
    L.append("| Klasse | Addon | Modell | Sel. | Stufen R/Ba/Bo/Dr/Bu/Ro (visual_properties) | Tex/Mat | Dateien | section | eigene Stufen-Optik | Probleme |")
    L.append("|---|---|---|---|---|---|---|---|---|---|")
    for r in rows:
        if r["stages"]:
            st = " ".join("-" if v is None else ("?" if v == "MISSING" else "{%s}" % ",".join(str(x) for x in v)) for v in r["stages"].values())
        else:
            st = "keine FoodStages"
        own = [k for k, v in r.get("distinct", {}).items() if v == "eigen"]
        model = r["model"].replace("\\ChefZ\\", "").replace("\\", "/")
        L.append("| %s | %s | %s | %s | %s | %d/%d | %s | %s | %s | %s |" % (
            r["class"], r["addon"].replace("ChefZ_", ""), model, ",".join(r["selections"]) or "-", st,
            len(r["textures"]), len(r["materials"]), files_ok(r), section_state(r),
            ",".join(own) if own else "keine", "; ".join(r["problems"]).replace("|", "/") or "-"))
    open(path, "w", encoding="utf-8").write("\n".join(L) + "\n")


def main():
    rows = audit()
    args = sys.argv[1:]
    if "--md" in args:
        write_md(rows, args[args.index("--md") + 1])
    if "--json" in args:
        p = args[args.index("--json") + 1]
        json.dump(rows, open(p, "w", encoding="utf-8"), indent=1, default=str)
    for r in rows:
        print("%-34s %-18s tex=%d mat=%d stages=%s probs=%d" % (r["class"], r["addon"], len(r["textures"]), len(r["materials"]),
              "".join("x" if v not in (None,) else "-" for v in r["stages"].values()) if r["stages"] else "none", len(r["problems"])))


if __name__ == "__main__":
    main()
