"""Index of the ChefZ data for the autotest generator.

Reads everything the in-game test needs to build concrete inputs:
ingredient bindings (JSON kind "ingredient" + CfgChefZIngredients in config.cpp),
the category tree (kind "category"), recipes, transforms, stations, fishing yields
and baits. Pure data, no game access.
"""
import glob
import json
import os
import re

ADDONS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "ChefZ_Core", "Addons")


def load_json_records(addons=ADDONS):
    """kind -> list of (file, record)."""
    out = {}
    for f in sorted(glob.glob(os.path.join(addons, "*", "Config", "**", "*.json"), recursive=True)):
        with open(f, encoding="utf-8") as fh:
            d = json.load(fh)
        kind = d.get("kind")
        if not kind:
            continue
        for r in d.get("records", []):
            out.setdefault(kind, []).append((os.path.relpath(f, addons), r))
    return out


_CLASS_RE = re.compile(r"class\s+(\w+)\s*(?::\s*(\w+))?\s*\{")


def _strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def _section(text, name):
    m = re.search(r"\bclass\s+" + name + r"\s*\{", text)
    if not m:
        return ""
    i = m.end()
    depth = 1
    start = i
    while i < len(text) and depth:
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
        i += 1
    return text[start:i - 1]


def _parse_classes(body):
    """Top-level classes of a config section: name -> (base, fields)."""
    classes = {}
    i = 0
    while True:
        m = _CLASS_RE.search(body, i)
        if not m:
            break
        name, base = m.group(1), m.group(2)
        j = m.end()
        depth = 1
        while j < len(body) and depth:
            if body[j] == "{":
                depth += 1
            elif body[j] == "}":
                depth -= 1
            j += 1
        inner = body[m.end():j - 1]
        fields = {}
        for fm in re.finditer(r"(\w+)\s*\[\]\s*=\s*\{([^}]*)\}", inner):
            fields[fm.group(1)] = [x.strip().strip('"') for x in fm.group(2).split(",") if x.strip()]
        for fm in re.finditer(r"(\w+)\s*=\s*(\"[^\"]*\"|[-\w.]+)\s*;", inner):
            if fm.group(1) not in fields:
                fields[fm.group(1)] = fm.group(2).strip('"')
        classes[name] = (base, fields)
        i = j
    return classes


def load_config_ingredients(addons=ADDONS):
    out = {}
    for f in sorted(glob.glob(os.path.join(addons, "*", "config.cpp"))):
        with open(f, encoding="utf-8", errors="replace") as fh:
            text = _strip_comments(fh.read())
        sec = _section(text, "CfgChefZIngredients")
        if not sec:
            continue
        classes = _parse_classes(sec)

        def resolve(n, seen=()):
            base, fields = classes.get(n, (None, {}))
            merged = {}
            if base and base in classes and base not in seen:
                merged.update(resolve(base, seen + (n,)))
            merged.update(fields)
            return merged

        for n in classes:
            merged = resolve(n)
            if merged.get("template") == n:
                continue  # template, not a binding
            out[n] = {"id": n, "categories": merged.get("categories", []), "tags": merged.get("tags", []),
                      "defaultState": merged.get("defaultState", "RAW"), "quantityUnit": merged.get("quantityUnit", "PIECE"),
                      "_src": os.path.basename(os.path.dirname(f)) + "/config.cpp"}
    return out


class Index:
    def __init__(self, addons=ADDONS):
        self.records = load_json_records(addons)
        self.parent = {}
        for _, r in self.records.get("category", []):
            self.parent[r["id"]] = r.get("parent")
        self.ingredients = {}
        for f, r in self.records.get("ingredient", []):
            if r["id"] not in self.ingredients:
                rr = dict(r)
                rr["_src"] = f
                self.ingredients[r["id"]] = rr
        for k, v in load_config_ingredients(addons).items():
            self.ingredients.setdefault(k, v)

    def closure(self, cat):
        out = []
        while cat and cat not in out:
            out.append(cat)
            cat = self.parent.get(cat)
        return out

    def categories_of(self, cls):
        ing = self.ingredients.get(cls)
        if not ing:
            return []
        out = []
        for c in ing.get("categories", []):
            for x in self.closure(c):
                if x not in out:
                    out.append(x)
        return out

    def tags_of(self, cls):
        ing = self.ingredients.get(cls)
        return list(ing.get("tags", [])) if ing else []

    def kind(self, name):
        return [r for _, r in self.records.get(name, [])]


if __name__ == "__main__":
    ix = Index()
    print(len(ix.ingredients), "ingredients;", {k: len(v) for k, v in ix.records.items()})
    print("SEAFOOD:", sorted(c for c in ix.ingredients if "SEAFOOD" in ix.categories_of(c)))
