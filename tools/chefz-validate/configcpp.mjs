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
    // Ein reines Asset-Paket (nur models/ und data/, keine Klasse, kein
    // Skript) braucht ChefZ_Core NICHT: Dateien haengen von keinem Code ab.
    // Wer das so meint, schreibt "ASSET-PBO" in den Kopf seiner config.cpp -
    // dieselbe Tuer, die chefzcore mit "I4-BELEG" oeffnet, und aus demselben
    // Grund: eine bewusste Entscheidung soll in der Datei stehen, nicht im
    // Gedaechtnis dessen, der den Report liest.
    const assetPbo = /ASSET-PBO/.test(readText(cfg) || '');
    if (!isCore && !assetPbo && required.length > 0 && !required.some(r => /ChefZ_Core/i.test(r))) {
      f.warn(cfg, parsed.patches[0]?.line ?? 0, `"${path.basename(dir)}" nennt ChefZ_Core nicht in requiredAddons - beabsichtigt? Wenn es ein reines Asset-Paket ist: "ASSET-PBO" in den Kopf der config.cpp schreiben.`);
    }

    // units[] sollte die im Modul definierten Item-Klassen nennen.
    //
    // scope.length === 1 ist die ganze Praezision dieser Regel: nur eine
    // Klasse DIREKT unter CfgVehicles ist ein Item. Tiefer liegen
    // Unterknoten, die zufaellig einen ChefZ-Namen tragen und in units[]
    // nichts verloren haben - der Skinning-Ertrag
    // (CfgVehicles > Animal_BosTaurus > Skinning > ChefZ_BeefLegYield) und
    // der Garstufenuebergang
    // (CfgVehicles > <Item> > Food > FoodStageTransitions > Raw >
    // ChefZ_RawToBaked). Ohne diese Bedingung meldete der Pruefer neunzehn
    // solcher Knoten, und die echten zwei Luecken gingen darin unter.
    const units = new Set(parsed.arrays.units || []);
    const own = parsed.classes.filter(c =>
      c.declaredBody && c.scope.length === 1 && c.scope[0] === 'CfgVehicles'
      && c.name.startsWith('ChefZ_'));
    for (const c of own) {
      if (units.size > 0 && !units.has(c.name)) {
        f.warn(cfg, c.line, `Klasse "${c.name}" steht nicht in units[] von CfgPatches`);
      }
    }
  }

  // --- doppelte Klassendefinitionen ueber das gesamte Projekt ---
  //
  // Verglichen wird der VOLLE Pfad, nicht der nackte Name. In DayZ-Configs
  // traegt jedes essbare Item seine eigenen Unterknoten - "Nutrition",
  // "FoodStages", "Raw", "Baked", "Horticulture", "defs". Dass "Raw" 26-mal
  // vorkommt, ist der Normalfall und kein Fehler; zwei Item-Klassen mit
  // demselben Namen sind einer.
  const byPath = new Map();
  for (const [name, defs] of projectClasses()) {
    for (const d of defs) {
      const path = [...d.scope, name].join('/');
      if (!byPath.has(path)) byPath.set(path, []);
      byPath.get(path).push({ ...d, name });
    }
  }
  for (const [path, defs] of byPath) {
    if (defs.length <= 1) continue;
    const where = defs.map(d => `${rel(d.file)}:${d.line}`).join(', ');
    f.error(defs[0].file, defs[0].line,
      `"${path}" ist ${defs.length}-mal definiert (${where}) - die spaetere ueberschreibt die fruehere still`);
  }

  // --- modded class: jede Stelle benennen, damit sie bewusst bleibt ---
  //
  // Der Zweck dieser Regel ist nicht, modded class zu verhindern - ohne sie
  // gaebe es diesen Mod nicht -, sondern dass keine Stelle UNGEPRUEFT bleibt.
  // Genau das kann sie erst, seit eine geprueft Stelle das sagen darf:
  // "SCOUT-GEPRUEFT" im Kommentar bis zwoelf Zeilen davor schweigt sie, alles
  // andere meldet sie weiter. Zwanzig Dauerwarnungen, die bei jedem Lauf
  // gleich aussehen, haetten die einundzwanzigste - die neue, ungepruefte -
  // verschluckt.
  //
  // Der Marker traegt das Datum der Pruefung. Er ist keine Absolution: wer
  // die Klasse spaeter umbaut, laesst ihn stehen und luegt damit - deshalb
  // nennt er den Pruefer und den Tag, und deshalb steht er direkt ueber der
  // Zeile, die er betrifft, wo ihn jede Aenderung sieht.
  for (const dir of modules) {
    for (const file of walk(dir, (_p, n) => n.endsWith('.c'))) {
      const txt = readText(file);
      if (!txt) continue;
      const lines = txt.split(/\r?\n/);
      lines.forEach((l, i) => {
        const m = l.match(/^\s*modded\s+class\s+([A-Za-z_]\w*)/);
        if (!m) return;
        const window = lines.slice(Math.max(0, i - 12), i).join('\n');
        if (/SCOUT-GEPRUEFT/.test(window)) return;
        f.warn(file, i + 1, `modded class ${m[1]} - Kollisionsflaeche gegenueber anderen Mods, im Conflict-Scout pruefen. Ist sie geprueft: "SCOUT-GEPRUEFT <Datum>" in den Kommentar darueber.`);
      });
    }
  }

  return f;
}
