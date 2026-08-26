// Strukturpruefung der config.cpp je Addon:
// CfgPatches vorhanden, keine doppelten Klassendefinitionen, requiredAddons plausibel.

import path from 'node:path';
import {
  Findings, allModuleDirs, parseConfigCpp,
  projectClasses, exists, rel, walk, readText,
} from './lib.mjs';

export default function run() {
  const f = new Findings('configcpp');
  const modules = allModuleDirs();

  if (modules.length === 0) {
    f.warn(null, 0, 'Keine Addon- oder Comp-Verzeichnisse gefunden - noch nichts gebaut?');
    return f;
  }

  // --- je Modul: config.cpp, $PREFIX$, CfgPatches ---
  const patchNames = new Map();   // CfgPatches-Eintrag -> Datei
  for (const dir of modules) {
    const cfg = path.join(dir, 'config.cpp');
    if (!exists(cfg)) {
      f.error(cfg, 0, `Modul "${path.basename(dir)}" hat keine config.cpp`);
      continue;
    }
    if (!exists(path.join(dir, '$PREFIX$'))) {
      f.error(path.join(dir, '$PREFIX$'), 0, `Modul "${path.basename(dir)}" hat keine $PREFIX$-Datei - PBO-Build schlaegt fehl`);
    }

    const parsed = parseConfigCpp(cfg);
    if (parsed.patches.length === 0) {
      f.error(cfg, 0, `Kein CfgPatches-Eintrag in "${path.basename(dir)}" - der Mod wird nicht geladen`);
    }
    for (const p of parsed.patches) {
      if (patchNames.has(p.name)) {
        f.error(cfg, p.line, `CfgPatches-Name "${p.name}" doppelt vergeben (auch in ${rel(patchNames.get(p.name))})`);
      } else {
        patchNames.set(p.name, cfg);
      }
    }

    // requiredAddons: leer ist bei Content-Modulen fast immer ein Fehler
    const required = parsed.arrays.requiredAddons || [];
    const isCore = path.basename(dir) === 'ChefZ_Core';
    if (!isCore && required.length === 0 && parsed.patches.length > 0) {
      f.error(cfg, parsed.patches[0].line, `"${path.basename(dir)}" deklariert keine requiredAddons - die Ladereihenfolge ist damit undefiniert`);
    }
    if (!isCore && required.length > 0 && !required.some(r => /ChefZ_Core/i.test(r))) {
      f.warn(cfg, parsed.patches[0]?.line ?? 0, `"${path.basename(dir)}" nennt ChefZ_Core nicht in requiredAddons - beabsichtigt?`);
    }

    // units[] sollte die im Modul definierten Item-Klassen nennen
    const units = new Set(parsed.arrays.units || []);
    const own = parsed.classes.filter(c =>
      c.declaredBody && c.scope[0] === 'CfgVehicles' && c.name.startsWith('ChefZ_'));
    for (const c of own) {
      if (units.size > 0 && !units.has(c.name)) {
        f.warn(cfg, c.line, `Klasse "${c.name}" steht nicht in units[] von CfgPatches`);
      }
    }
  }

  // --- doppelte Klassendefinitionen ueber das gesamte Projekt ---
  for (const [name, defs] of projectClasses()) {
    if (defs.length > 1) {
      const where = defs.map(d => `${rel(d.file)}:${d.line}`).join(', ');
      f.error(defs[0].file, defs[0].line, `Klasse "${name}" ist ${defs.length}-mal definiert (${where}) - die spaetere ueberschreibt die fruehere still`);
    }
  }

  // --- modded class: jede Stelle benennen, damit sie bewusst bleibt ---
  for (const dir of modules) {
    for (const file of walk(dir, (_p, n) => n.endsWith('.c'))) {
      const txt = readText(file);
      if (!txt) continue;
      txt.split(/\r?\n/).forEach((l, i) => {
        const m = l.match(/^\s*modded\s+class\s+([A-Za-z_]\w*)/);
        if (m) f.warn(file, i + 1, `modded class ${m[1]} - Kollisionsflaeche gegenueber anderen Mods, im Conflict-Scout pruefen`);
      });
    }
  }

  return f;
}
