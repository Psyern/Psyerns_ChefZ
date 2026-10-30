r"""Lokaler Ersatz fuer den Lieferungs-Runner (tools/import_delivery.py der Suite liegt nur auf
der anderen Maschine). Baut eine Meshy-Lieferung aus einem Manifest zu MLOD-P3D + PAA.

    py -3 tools/chefz-assets/build_delivery_local.py tools/chefz-assets/deliveries/<manifest>.json

Ablauf (gleiche Schritte wie der Original-Runner, siehe deliveries/*.report.json):
  1. Texturen  (System-Python, Pillow): Farb-PNG -> <size>^2 RGB -> ImageToPAA (_co, Garstufen,
     optional _rotten_co); Exit-Code von ImageToPAA wird geprueft.
  2. Modelle   (Blender headless + Psyerns 3D Suite aus dem Repo): psy.model_import_file ->
     optional rotate -> Laenge der laengsten Achse -> Ursprung unten mittig -> Selektion als
     Vertex-Gruppe -> psy.auto_build (Template, ohne Build) -> Masse -> offenes Shadow Volume durch
     die Huelle ersetzen -> Proxies (Regalboeden / Erdflaeche, per Raycast gemessen) -> MLOD
     schreiben (build_bridge.collect + core.mlod.write_mlod, mit Named Properties).
  3. Extras    : Myzel-Variante der Tray-Textur (Erdflaeche aus den UVs der Erd-Faces), Proxy-Zieldateien
     (Format wie ChefZ_Devices/models/proxies/hook_1.p3d), model.cfg je Gruppe.
  4. Spiegeln  : ChefZ/<Paket>/<gruppe> -> ChefZ_Core/Addons/<Paket>/<gruppe> (identisch).
  5. Report    : deliveries/<manifest>.report.json
Schreibt keine config.cpp, keine Skripte, kein JSON ausser dem Report.

Manifest wie deliveries/2026-10-30_meshy_kits.json, zusaetzliche optionale Felder je Asset:
  "texture_size", "states" (Liste, auch "rotten"), "mass" (kg, Geometry-LOD), "rotate" [x,y,z] Grad,
  "proxies": {"layout": "shelf_boards" | "tray_soil", "path": "\\ChefZ\\...\\name_", "count": n,
              "item": "<Asset-Name des aufgesetzten Modells>"}  (nur fuer Messung/Passprobe),
  "mycelium": "<Name der Zusatztextur>" (Tray: weisse Myzel-Flecken auf der Erde).
"""
import json
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
SUITE = os.environ.get("PSY_SUITE", r"C:\Users\Administrator\Desktop\Psyerns_3D_Suite_For_Blender")
BLENDER = os.environ.get("PSY_BLENDER", r"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe")
IMAGE_TO_PAA = os.path.join(os.environ.get("PSY_DAYZ_TOOLS", r"C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools"),
                            "Bin", "ImageToPAA", "ImageToPAA.exe")
ROTTEN_TINT = ((0.55, 0.52, 0.40), 0.35)


# --------------------------------------------------------------------------- shared helpers

def load_manifest(path):
    with open(path, encoding="utf-8") as fh:
        manifest = json.load(fh)
    root = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(path)), manifest["project_root"]))
    defaults = manifest.get("defaults", {})
    assets = []
    for entry in manifest["assets"]:
        if entry.get("skip"):
            continue
        group = manifest["groups"][entry["group"]]
        a = dict(defaults)
        a.update({k: v for k, v in group.items() if k != "target"})
        a.update(entry)
        a["target"] = group["target"]
        a["stem"] = entry.get("file", entry["name"].lower())
        a["source_abs"] = os.path.join(manifest["source_root"], entry["source"])
        a["texture_abs"] = os.path.join(manifest["source_root"], entry["texture"])
        a["tex_rel"] = f"{group['target']}\\data\\{entry['name']}_co.paa"
        a["model_rel"] = f"\\{group['target']}\\models\\{a['stem']}.p3d"
        a["out_dir"] = os.path.join(root, *group["target"].split("\\"))
        assets.append(a)
    return manifest, root, assets


# --------------------------------------------------------------------------- system python side

def paa(src, dst):
    proc = subprocess.run([IMAGE_TO_PAA, src, dst], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    ok = proc.returncode == 0 and os.path.isfile(dst)
    return {"file": dst, "exit": proc.returncode, "ok": ok}


def tint(rgb, factor, saturation):
    import numpy as np
    grey = rgb.mean(axis=2, keepdims=True)
    return np.clip((grey + (rgb - grey) * saturation) * np.array(factor), 0.0, 1.0)


def build_textures(asset, scratch):
    import numpy as np
    from PIL import Image
    sys.path.insert(0, HERE)
    import importlib.util
    spec = importlib.util.spec_from_file_location("stage_textures", os.path.join(HERE, "stage-textures.py"))
    stage = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(stage)
    size = int(asset.get("texture_size", 1024))
    data_dir = os.path.join(asset["out_dir"], "data")
    os.makedirs(data_dir, exist_ok=True)
    img = Image.open(asset["texture_abs"]).convert("RGB").resize((size, size), Image.LANCZOS)
    base_png = os.path.join(scratch, f"{asset['name']}_co.png")
    img.save(base_png)
    results = [paa(base_png, os.path.join(data_dir, f"{asset['name']}_co.paa"))]
    rgb = np.asarray(img, dtype=np.float32) / 255.0
    for state in asset.get("states", []):
        factor, saturation = ROTTEN_TINT if state == "rotten" else stage.STATE_TINTS[state]
        png = os.path.join(scratch, f"{asset['name']}_{state}_co.png")
        Image.fromarray((tint(rgb, factor, saturation) * 255.0 + 0.5).astype(np.uint8), "RGB").save(png)
        results.append(paa(png, os.path.join(data_dir, f"{asset['name']}_{state}_co.paa")))
    return base_png, results


def build_mycelium(asset, base_png, soil_tris, scratch, seed=7):
    """White fluffy mycelium on the soil. The soil texels come from the UV triangles of the soil faces
    (each corner = u, v, model x, model y); the noise is evaluated in MODEL space (metres), so it runs
    continuously across the fragmented Meshy UV islands. Only dark brown texels are changed."""
    import numpy as np
    from PIL import Image, ImageFilter
    img = Image.open(base_png).convert("RGB")
    w, h = img.size
    rgb = np.asarray(img, dtype=np.float32) / 255.0
    rng = np.random.default_rng(seed)

    def value_noise(cell, extent=0.6):
        n = int(extent / cell) + 3
        grid = rng.random((n, n)).astype(np.float32)
        def sample(x, y):
            gx = (x + extent / 2) / cell; gy = (y + extent / 2) / cell
            x0 = np.clip(np.floor(gx).astype(int), 0, n - 2); y0 = np.clip(np.floor(gy).astype(int), 0, n - 2)
            fx = gx - x0; fy = gy - y0
            fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy)
            a = grid[y0, x0] * (1 - fx) + grid[y0, x0 + 1] * fx
            b = grid[y0 + 1, x0] * (1 - fx) + grid[y0 + 1, x0 + 1] * fx
            return a * (1 - fy) + b * fy
        return sample

    coarse, mid, fine, speck1, speck2 = (value_noise(c) for c in (0.045, 0.015, 0.006, 0.0025, 0.0016))
    alpha = np.zeros((h, w), np.float32)
    shade = np.zeros((h, w), np.float32)
    covered = np.zeros((h, w), bool)
    for tri in soil_tris:
        P = np.array([[c[0] * w, (1.0 - c[1]) * h] for c in tri], np.float64)
        XY = np.array([[c[2], c[3]] for c in tri], np.float64)
        x0, y0 = np.floor(P.min(axis=0) - 2).astype(int); x1, y1 = np.ceil(P.max(axis=0) + 2).astype(int)
        x0, y0 = max(x0, 0), max(y0, 0); x1, y1 = min(x1, w - 1), min(y1, h - 1)
        if x1 < x0 or y1 < y0:
            continue
        gx, gy = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
        T = np.array([[P[1, 0] - P[0, 0], P[2, 0] - P[0, 0]], [P[1, 1] - P[0, 1], P[2, 1] - P[0, 1]]])
        if abs(np.linalg.det(T)) < 1e-9:
            continue
        Ti = np.linalg.inv(T)
        dx, dy = gx - P[0, 0], gy - P[0, 1]
        l1 = Ti[0, 0] * dx + Ti[0, 1] * dy; l2 = Ti[1, 0] * dx + Ti[1, 1] * dy; l0 = 1 - l1 - l2
        pad = 1.5 / max(1.0, np.abs(T).max())          # ~1.5 px dilation against seams
        inside = (l0 >= -pad) & (l1 >= -pad) & (l2 >= -pad)
        mx = l0 * XY[0, 0] + l1 * XY[1, 0] + l2 * XY[2, 0]
        my = l0 * XY[0, 1] + l1 * XY[1, 1] + l2 * XY[2, 1]
        cloud = 0.55 * coarse(mx, my) + 0.30 * mid(mx, my) + 0.15 * fine(mx, my)
        patches = np.clip((cloud - 0.50) * 3.2, 0.0, 1.0)              # cloudy mats of mycelium
        dots = np.clip((np.maximum(speck1(mx, my), speck2(mx, my)) - 0.80) * 6.0, 0.0, 1.0)  # fluffy speckles
        a = np.clip(patches * (0.55 + 0.4 * fine(mx, my)) + dots * 0.9, 0.0, 0.92)
        sub_a = alpha[y0:y1 + 1, x0:x1 + 1]; sub_s = shade[y0:y1 + 1, x0:x1 + 1]; sub_c = covered[y0:y1 + 1, x0:x1 + 1]
        sub_a[inside] = a[inside]; sub_s[inside] = mid(mx, my)[inside]; sub_c[inside] = True
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    brown = ((r >= b + 0.01) & (rgb.mean(axis=2) < 0.45)).astype(np.float32)   # dark, warm = soil only
    brown = np.asarray(Image.fromarray((brown * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(1.0)),
                       dtype=np.float32) / 255.0
    alpha = alpha * brown
    white = np.stack([0.90 + 0.07 * shade, 0.89 + 0.07 * shade, 0.83 + 0.07 * shade], axis=2)
    out = rgb * (1.0 - alpha[..., None]) + white * alpha[..., None]
    png = os.path.join(scratch, f"{asset['mycelium']}_co.png")
    Image.fromarray((np.clip(out, 0, 1) * 255.0 + 0.5).astype(np.uint8), "RGB").save(png)
    result = paa(png, os.path.join(asset["out_dir"], "data", f"{asset['mycelium']}_co.paa"))
    result["soil_pixels"] = int(covered.sum())
    result["covered_pixels"] = int((alpha > 0.3).sum())
    return result


def write_proxy_file(path, inner_path):
    """Minimal proxy target like ChefZ_Devices/models/proxies/hook_1.p3d: one resolution LOD with one
    proxy triangle at the origin (up +Y length 2, forward +Z length 1) pointing to ``inner_path``."""
    sys.path.insert(0, SUITE)
    from psyerns_3d_suite.core import mlod
    from psyerns_3d_suite.core.lods import LodKind
    from psyerns_3d_suite.core.model import Lod
    from psyerns_3d_suite.core.proxies import add_proxy
    lod = Lod(LodKind.RESOLUTION, 0.0)
    add_proxy(lod, inner_path, 1, (0.0, 0.0, 0.0), (0.0, 2.0, 0.0), (0.0, 0.0, 1.0))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as fh:
        fh.write(mlod.write_mlod([lod]))


def write_model_cfg(path, stems, selection, header):
    lines = [header, "class CfgModels", "{", "    class Default", "    {", "        sections[] = {};",
             '        sectionsInherit="";', '        skeletonName = "";', "    };"]
    for stem in sorted(stems):
        lines += [f"    class {stem}:Default", "    {", "        sections[]=", "        {",
                  f'            "{selection}"', "        };", "    };"]
    lines += ["};", ""]
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines))


def mirror(src_dir, dst_dir):
    copied = []
    for base, _dirs, files in os.walk(src_dir):
        rel = os.path.relpath(base, src_dir)
        os.makedirs(os.path.join(dst_dir, rel), exist_ok=True)
        for name in files:
            shutil.copy2(os.path.join(base, name), os.path.join(dst_dir, rel, name))
            copied.append(os.path.normpath(os.path.join(dst_dir, rel, name)))
    return copied


def orchestrate(manifest_path):
    started = time.time()
    manifest, root, assets = load_manifest(manifest_path)
    scratch = os.path.join(os.environ.get("TEMP", "."), "chefz_delivery_" + os.path.splitext(os.path.basename(manifest_path))[0])
    os.makedirs(scratch, exist_ok=True)
    results = {a["name"]: {"name": a["name"], "group": a["group"], "bind": a.get("bind", []), "note": a.get("note", ""),
                           "selection": a.get("selection", "camo"), "model": a["model_rel"], "texture": a["tex_rel"]}
               for a in assets}
    # 1. textures
    base_pngs = {}
    for a in assets:
        base_pngs[a["name"]], tex = build_textures(a, scratch)
        results[a["name"]]["paa"] = tex
    # 2. models in Blender
    job = os.path.join(scratch, "job.json")
    out = os.path.join(scratch, "blender_result.json")
    with open(job, "w", encoding="utf-8") as fh:
        json.dump({"manifest": os.path.abspath(manifest_path), "result": out}, fh)
    proc = subprocess.run([BLENDER, "-b", "--factory-startup", "--python", os.path.abspath(__file__), "--", "--blender", job],
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace")
    with open(os.path.join(scratch, "blender.log"), "w", encoding="utf-8") as fh:
        fh.write(proc.stdout)
    if not os.path.isfile(out):
        print(proc.stdout[-4000:])
        raise SystemExit(f"Blender lieferte kein Ergebnis (exit {proc.returncode})")
    with open(out, encoding="utf-8") as fh:
        blender_results = json.load(fh)
    for name, res in blender_results.items():
        results[name].update(res)
    # 3. extras
    for a in assets:
        if a.get("mycelium"):
            soil = results[a["name"]].get("soil_uvs") or []
            results[a["name"]]["paa"].append(build_mycelium(a, base_pngs[a["name"]], soil, scratch))
            results[a["name"]].pop("soil_uvs", None)
        spec = a.get("proxies")
        if spec:
            proxy_dir = os.path.join(root, *spec["path"].strip("\\").split("\\")[:-1])
            inner = spec["file_content"]
            files = []
            for i in range(1, spec["count"] + 1):
                target = os.path.join(proxy_dir, spec["path"].strip("\\").split("\\")[-1] + f"{i}.p3d")
                write_proxy_file(target, inner)
                files.append(target)
            results[a["name"]]["proxy_files"] = files
    groups = {}
    for a in assets:
        groups.setdefault(a["group"], []).append(a)
    for gname, members in groups.items():
        sel = members[0].get("selection", "camo")
        header = (f"// {members[0]['target']}\\models - erzeugt von tools/chefz-assets/build_delivery_local.py\n"
                  f"// ({os.path.basename(manifest_path)}). Die MLOD-Modelle tragen die Selektion \"{sel}\"; ohne\n"
                  f"// diese Datei ist sie keine section und hiddenSelectionsTextures greifen nicht.\n"
                  f"// Beide Baeume identisch halten (ChefZ/... und ChefZ_Core/Addons/...).")
        write_model_cfg(os.path.join(members[0]["out_dir"], "models", "model.cfg"), [m["stem"] for m in members], sel, header)
    # 4. mirror ChefZ/<pkg>/<group> -> ChefZ_Core/Addons/<pkg>/<group>
    mirrored = []
    for gname, members in groups.items():
        target = members[0]["target"].split("\\")          # ChefZ, ChefZ_Plants, mushrooms
        dst = os.path.join(root, "ChefZ_Core", "Addons", *target[1:])
        mirrored += mirror(members[0]["out_dir"], dst)
    report = {"manifest": os.path.abspath(manifest_path), "runner": "tools/chefz-assets/build_delivery_local.py",
              "duration": round(time.time() - started, 1), "blender_exit": proc.returncode,
              "mirrored_files": len(mirrored), "results": list(results.values())}
    for r in report["results"]:
        r["status"] = "OK" if all(t["ok"] for t in r.get("paa", [])) and r.get("status_model") == "OK" else "FAILED"
    rp = os.path.splitext(manifest_path)[0] + ".report.json"
    with open(rp, "w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=1, ensure_ascii=False)
    print(f"Report: {rp}")
    for r in report["results"]:
        print(r["name"], r["status"], r.get("size_m"), r.get("lods"))
    return 0 if all(r["status"] == "OK" for r in report["results"]) else 1


# --------------------------------------------------------------------------- Blender side

def blender_main(job_path):
    import bmesh
    import bpy
    from mathutils import Matrix, Vector
    from mathutils.bvhtree import BVHTree

    sys.path.insert(0, SUITE)
    import psyerns_3d_suite
    if not hasattr(bpy.types.Scene, "psy"):
        psyerns_3d_suite.register()
    from psyerns_3d_suite.blender import build_bridge
    from psyerns_3d_suite.core.mlod import write_mlod

    with open(job_path, encoding="utf-8") as fh:
        job = json.load(fh)
    manifest, root, assets = load_manifest(job["manifest"])
    by_name = {a["name"]: a for a in assets}
    out = {}
    built_dims = {}

    def clear_scene():
        for o in list(bpy.data.objects):
            bpy.data.objects.remove(o, do_unlink=True)
        for coll in (bpy.data.meshes, bpy.data.materials, bpy.data.images):
            for item in list(coll):
                if item.users == 0:
                    coll.remove(item)

    def bvh_of(obj):
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bm.transform(obj.matrix_world)
        tree = BVHTree.FromBMesh(bm)
        bm.free()
        return tree

    def closed(obj):
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        ok = all(len(e.link_faces) == 2 for e in bm.edges)
        bm.free()
        return ok

    def horizontal_clusters(obj, min_nz=0.9):
        mesh = obj.data
        bins = {}
        for p in mesh.polygons:
            if p.normal.z < min_nz:
                continue
            z = round(p.center.z, 2)
            bins.setdefault(z, []).append(p)
        merged = []
        for z in sorted(bins):
            if merged and z - merged[-1]["z1"] <= 0.03:
                merged[-1]["polys"] += bins[z]
                merged[-1]["z1"] = z
            else:
                merged.append({"z0": z, "z1": z, "polys": list(bins[z])})
        for c in merged:
            vs = [mesh.vertices[i].co for p in c["polys"] for i in p.vertices]
            c["area"] = sum(p.area for p in c["polys"])
            c["x"] = (min(v.x for v in vs), max(v.x for v in vs))
            c["y"] = (min(v.y for v in vs), max(v.y for v in vs))
            c["z"] = sum(p.center.z * p.area for p in c["polys"]) / c["area"]
        return merged

    def rest_height(tree, cx, cy, half_x, half_y, z_from, z_lo, z_hi, n=7):
        """Highest surface hit (downwards) within the footprint, limited to [z_lo, z_hi]."""
        best, hits = None, 0
        for i in range(n):
            for j in range(n):
                x = cx - half_x + 2 * half_x * i / (n - 1)
                y = cy - half_y + 2 * half_y * j / (n - 1)
                loc, _nrm, _idx, _d = tree.ray_cast(Vector((x, y, z_from)), Vector((0, 0, -1)), z_from - z_lo + 0.01)
                if loc is not None and z_lo <= loc.z <= z_hi:
                    hits += 1
                    best = loc.z if best is None else max(best, loc.z)
        return best, hits

    def add_proxy_empties(path_prefix, frames, lod_indices):
        for i, (x, y, z) in enumerate(frames, start=1):
            for lod in lod_indices:
                e = bpy.data.objects.new(f"proxy_{i}_lod{lod}", None)
                bpy.context.scene.collection.objects.link(e)
                e.location = (x, y, z)
                e.psy.lod_kind = "resolution"
                e.psy.lod_index = float(lod)
                e.psy.proxy_path = f"{path_prefix}{i}"
                e.psy.proxy_index = 1

    order = sorted(assets, key=lambda a: 1 if a.get("proxies", {}).get("layout") == "shelf_boards" else 0)
    for a in order:
        res = {"messages": []}
        out[a["name"]] = res
        try:
            clear_scene()
            scene = bpy.context.scene
            scene.psy.template = a.get("template", "food")
            scene.psy.project_root = root + os.sep
            scene.psy.asset_name = a["stem"]
            scene.psy.addon = a["target"].split(chr(92))[1]   # ChefZ_Plants (validator: only A-Z, 0-9, _)
            bpy.ops.psy.model_import_file(filepath=a["source_abs"])
            meshes = [o for o in scene.objects if o.type == "MESH"]
            for o in [o for o in scene.objects if o.type != "MESH"]:
                bpy.data.objects.remove(o, do_unlink=True)
            for o in bpy.context.view_layer.objects:
                o.select_set(o in meshes)
            bpy.context.view_layer.objects.active = meshes[0]
            if len(meshes) > 1:
                bpy.ops.object.join()
            obj = bpy.context.view_layer.objects.active
            obj.name = a["name"]
            obj.data.name = a["name"]
            res["source_tris"] = sum(len(p.vertices) - 2 for p in obj.data.polygons)
            if a.get("rotate"):
                import math
                rx, ry, rz = (math.radians(v) for v in a["rotate"])
                obj.rotation_euler = (rx, ry, rz)
            bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
            dims = [max(v.co[i] for v in obj.data.vertices) - min(v.co[i] for v in obj.data.vertices) for i in range(3)]
            factor = float(a["length"]) / max(dims)
            obj.data.transform(Matrix.Scale(factor, 4))
            vs = obj.data.vertices
            lo = Vector([min(v.co[i] for v in vs) for i in range(3)])
            hi = Vector([max(v.co[i] for v in vs) for i in range(3)])
            obj.data.transform(Matrix.Translation(Vector((-(lo.x + hi.x) / 2, -(lo.y + hi.y) / 2, -lo.z))))
            obj.data.update()
            dims = [max(v.co[i] for v in vs) - min(v.co[i] for v in vs) for i in range(3)]
            res["size_m"] = [round(d, 3) for d in dims]   # Blender x (width), y (depth), z (height)
            built_dims[a["name"]] = dims
            # material: texture path in the P3D, source PNG for previews
            obj.data.materials.clear()
            mat = bpy.data.materials.new(a["name"])
            mat.psy.texture = a["tex_rel"]
            obj.data.materials.append(mat)
            for p in obj.data.polygons:
                p.material_index = 0
            sel = a.get("selection", "camo")
            group = obj.vertex_groups.new(name=sel)
            group.add(list(range(len(obj.data.vertices))), 1.0, "REPLACE")
            obj.psy.lod_kind = "resolution"
            obj.psy.lod_index = 0.0
            # AUTO BUILD without the final build step
            scene.psy_auto.do_build = False
            bpy.ops.psy.auto_build()
            res["steps"] = [{"step": s.name, "status": s.status, "message": s.message} for s in scene.psy_auto.steps]
            geometry = [o for o in scene.objects if o.type == "MESH" and o.psy.lod_kind == "geometry"]
            if a.get("mass") is not None and geometry:
                geometry[0].psy.mass = float(a["mass"])
                res["messages"].append(f"Geometry-Masse {a['mass']} kg")
            shadows = [o for o in scene.objects if o.type == "MESH" and o.psy.lod_kind == "shadow_volume"]
            if shadows and geometry and not closed(shadows[0]):
                old = shadows[0]
                faces = len(geometry[0].data.polygons)
                new = geometry[0].copy()
                new.data = geometry[0].data.copy()
                scene.collection.objects.link(new)
                new.name = f"{a['name']}_Shadow"
                new.psy.lod_kind = "shadow_volume"
                new.psy.lod_index = old.psy.lod_index
                new.psy.mass = 0.0
                new.vertex_groups.clear()
                bpy.data.objects.remove(old, do_unlink=True)
                res["shadow"] = f"Shadow Volume war offen - ersetzt durch die Kollisionshuelle ({faces} Flaechen)"
            # proxies
            spec = a.get("proxies")
            if spec:
                tree = bvh_of(obj)
                item = built_dims.get(spec.get("item"))
                if item is None:
                    raise RuntimeError(f"Proxy-Messung braucht das Modell {spec.get('item')} (vorher bauen)")
                half_x, half_y = item[0] / 2, item[1] / 2
                frames, measure = [], []
                if spec["layout"] == "shelf_boards":
                    clusters = horizontal_clusters(obj)
                    biggest = max(c["area"] for c in clusters)
                    boards = [c for c in clusters if c["area"] >= 0.3 * biggest]
                    boards = sorted(sorted(boards, key=lambda c: -c["area"])[:3], key=lambda c: c["z"])
                    zs = [b["z"] for b in boards] + [dims[2] + 1.0]
                    for k, b in enumerate(boards):
                        bx0, bx1 = b["x"]
                        by0, by1 = b["y"]
                        width, depth = bx1 - bx0, by1 - by0
                        cx, cy = (bx0 + bx1) / 2, (by0 + by1) / 2
                        board = {"z": round(b["z"], 3), "x": [round(bx0, 3), round(bx1, 3)], "y": [round(by0, 3), round(by1, 3)],
                                 "area": round(b["area"], 3), "clear_above": round(zs[k + 1] - b["z"], 3)}
                        if width / 2 < 2 * half_x + 0.02 or depth < 2 * half_y:
                            res["messages"].append(f"Brett {k + 1} zu klein fuer zwei Trays: {board}")
                        for sx in (-1, 1):
                            px = cx + sx * width / 4
                            top, hits = rest_height(tree, px, cy, half_x, half_y, b["z"] + 0.15, b["z"] - 0.05, b["z"] + 0.06)
                            if top is None:
                                raise RuntimeError(f"Kein Brett unter Proxy bei x={px:.3f} z~{b['z']:.3f}")
                            frames.append((px, cy, top + 0.001))
                            board.setdefault("rays_hit", []).append(hits)
                        measure.append(board)
                    res["boards"] = measure
                elif spec["layout"] == "tray_soil":
                    count = spec["count"]
                    length, width = dims[0], dims[1]
                    xs = [(-0.28 + 0.28 * i) * length for i in range(count)] if count == 3 else \
                         [(-0.5 + (i + 0.5) / count) * length * 0.8 for i in range(count)]
                    ys = [(-0.12 if i % 2 == 0 else 0.12) * width for i in range(count)]
                    for px, py in zip(xs, ys):
                        loc, nrm, _i, _d = tree.ray_cast(Vector((px, py, dims[2] + 0.1)), Vector((0, 0, -1)), dims[2] + 0.2)
                        if loc is None:
                            raise RuntimeError(f"Keine Erdflaeche unter x={px:.3f}, y={py:.3f}")
                        frames.append((px, py, loc.z + 0.001))
                        measure.append({"x": round(px, 3), "y": round(py, 3), "soil_z": round(loc.z, 4),
                                        "normal_z": round(nrm.z, 3)})
                    res["soil"] = measure
                    # soil faces -> UV triangles for the mycelium texture
                    soil_z = sum(m["soil_z"] for m in measure) / len(measure)
                    uv = obj.data.uv_layers.active.data
                    tris = []
                    for p in obj.data.polygons:
                        c = p.center
                        if p.normal.z > 0.6 and abs(c.z - soil_z) < 0.02 and abs(c.x) < length / 2 - 0.015 \
                                and abs(c.y) < width / 2 - 0.015:
                            loops = list(p.loop_indices)
                            corner = lambda li: [uv[li].uv[0], uv[li].uv[1],
                                                 obj.data.vertices[obj.data.loops[li].vertex_index].co.x,
                                                 obj.data.vertices[obj.data.loops[li].vertex_index].co.y]
                            for t in range(1, len(loops) - 1):
                                tris.append([corner(loops[0]), corner(loops[t]), corner(loops[t + 1])])
                    res["soil_uvs"] = tris
                    res["messages"].append(f"Erdflaeche: {len(tris)} UV-Dreiecke fuer die Myzel-Textur")
                # fit check: a box of the item size at every proxy (2 mm lifted) must not cut the model
                clashes = []
                for idx, (x, y, z) in enumerate(frames, start=1):
                    bm = bmesh.new()
                    bmesh.ops.create_cube(bm, size=1.0)
                    bm.transform(Matrix.Translation((x, y, z + 0.002 + item[2] / 2)) @
                                 Matrix.Diagonal((item[0] * 0.98, item[1] * 0.98, item[2], 1.0)))
                    if spec["layout"] == "tray_soil":
                        bm.free()
                        break   # mushrooms are checked visually (sizes differ per species)
                    box = BVHTree.FromBMesh(bm)
                    bm.free()
                    if box.overlap(tree):
                        clashes.append(idx)
                res["proxy_clashes"] = clashes
                lods = sorted({o.psy.lod_index for o in scene.objects if o.type == "MESH" and o.psy.lod_kind == "resolution"})
                add_proxy_empties(spec["path"], frames, lods)
                res["proxies"] = [{"name": f"proxy:{spec['path']}{i}.001", "blender_xyz": [round(v, 4) for v in f]}
                                  for i, f in enumerate(frames, start=1)]
            # write the MLOD
            asset = build_bridge.collect(bpy.context).asset
            path = os.path.join(a["out_dir"], "models", f"{a['stem']}.p3d")
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "wb") as fh:
                fh.write(write_mlod(asset.lods))
            res["p3d"] = path
            res["lods"] = [f"{l.kind.value}{l.index:g}:{len(l.faces)}f" for l in asset.lods]
            failed = [s for s in res["steps"] if s["status"] == "FAILED"]
            res["status_model"] = "FAILED" if failed else "OK"
        except Exception as exc:  # noqa: BLE001 - report per asset
            import traceback
            res["status_model"] = "FAILED"
            res["error"] = f"{type(exc).__name__}: {exc}"
            res["traceback"] = traceback.format_exc()
        print("ASSET", a["name"], res.get("status_model"), res.get("error", ""), flush=True)
    with open(job["result"], "w", encoding="utf-8") as fh:
        json.dump(out, fh, indent=1)


if __name__ == "__main__":
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
    if argv and argv[0] == "--blender":
        blender_main(argv[1])
    elif argv:
        sys.exit(orchestrate(argv[0]))
    else:
        print(__doc__)
        sys.exit(2)
