// Namenskonvention nach DME-Plan §53: ChefZ_PascalCase.
// Prueft ausserdem Kollisionen mit Fremdklassen aus dem Referenzindex.

import { Findings, projectClasses, refIndex, rel } from './lib.mjs';

// Klassen, die bewusst NICHT dem ChefZ-Praefix folgen duerfen.
const ALLOWED_UNPREFIXED = [
  /^Cfg[A-Z]/,                 // CfgVehicles, CfgMods, CfgPatches ...
  /^Terje/,                    // Erweiterungen bestehender Terje-Bloecke in den Comp-Mods
  /^Cot/i,                     // COT-Erweiterungen
];

// "_Base" als Endung ist DayZ-Konvention fuer nicht spawnbare Basisklassen
// (Edible_Base, Inventory_Base, Container_Base). Sie zu verbieten hiesse, gegen
// die Engine-Konvention zu benennen, in die sich ChefZ einreiht.
const PASCAL = /^ChefZ_[A-Z][A-Za-z0-9]*(_Base)?$/;

export default function run() {
  const f = new Findings('naming');
  const defined = projectClasses();
  const ref = refIndex();

  // Nur ITEM-Klassen tragen die Namenskonvention. Alles andere in einer
  // config.cpp ist ein Knoten, kein Gegenstand: "Food", "FoodStages", "Baked",
  // "Horticulture" sind Vanilla-Unterknoten, "PROCESS_DRY", "PREMIUM" und
  // "CUTTING_TOOL" sind Eintraege in ChefZ' eigenen CfgChefZ*-Wurzeln. Von ihnen
  // ein ChefZ_-Praefix zu verlangen hiesse, gegen die Engine zu benennen.
  const ITEM_ROOTS = new Set(['CfgVehicles', 'CfgWeapons', 'CfgMagazines']);

  for (const [name, defs] of defined) {
    const d = defs[0];
    if (!(d.scope.length === 1 && ITEM_ROOTS.has(d.scope[0]))) continue;
    const inCfgVehicles = true;

    // 1. Praefix
    if (!name.startsWith('ChefZ_')) {
      const allowed = ALLOWED_UNPREFIXED.some(re => re.test(name));
      // Erweiterungen bestehender Vanilla-/Fremdklassen sind erlaubt,
      // wenn die Klasse im Referenzindex bekannt ist.
      const isExtension = ref.names.has(name);
      if (!allowed && !isExtension) {
        // Ein Name, der wie ein ChefZ-Name aussieht, ihn aber falsch schreibt,
        // ist auch ohne Vanilla-Index eindeutig falsch.
        const looksLikeChefZ = /^chefz/i.test(name);
        if (looksLikeChefZ || ref.vanillaIndexed) {
          f.error(d.file, d.line, `Klasse "${name}" verletzt die Namenskonvention: kein ChefZ_-Praefix und keine bekannte Fremdklasse`);
        } else {
          f.warn(d.file, d.line, `Klasse "${name}" ohne ChefZ_-Praefix - Erweiterung einer Vanilla-Klasse? Nicht pruefbar, vanilla-classes.txt ist leer`);
        }
      }
      continue;
    }

    // 2. PascalCase nach dem Praefix
    if (!PASCAL.test(name)) {
      f.error(d.file, d.line, `Klasse "${name}" verletzt ChefZ_PascalCase (keine Unterstriche, Zahlen oder Kleinbuchstaben direkt nach dem Praefix)`);
    }

    // 3. Kollision mit einer Fremdklasse
    if (ref.names.has(name)) {
      f.error(d.file, d.line, `Klassenname "${name}" existiert bereits ausserhalb des Projekts (Referenzindex) - stille ueberschreibung droht`);
    }

    // 4. Item-Klassen brauchen scopeArray/Anzeigenamen - nur als Hinweis
    if (inCfgVehicles && defs.length === 1 && !d.parent) {
      f.warn(d.file, d.line, `Item-Klasse "${name}" hat keine Elternklasse - beabsichtigt?`);
    }
  }

  if (!ref.vanillaIndexed) {
    f.warn(null, 0, 'vanilla-classes.txt ist leer - Namenskollisionen mit Vanilla-Item-Klassen sind nicht pruefbar.');
  }
  if (!ref.empty) {
    for (const s of ref.sources) {
      // rein informativ, damit im Report steht, wogegen geprueft wurde
      f.items.push({ validator: 'naming', severity: 'info', file: s.file, line: 0, summary: `Referenzindex: ${s.count} Klassen` });
    }
  }

  return f;
}
