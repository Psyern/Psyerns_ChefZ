// Prueft, dass jede in JSON oder als Elternklasse referenzierte Klasse existiert -
// entweder im Projekt selbst oder im Referenzindex (Vanilla / Terje / ...).

import {
  Findings, jsonFiles, deltaFiles, readJson, projectClasses, refIndex,
  collectJsonClassRefs, configCppFiles, parseConfigCpp, lineOf,
} from './lib.mjs';

export default function run() {
  const f = new Findings('classrefs');
  const defined = projectClasses();
  const ref = refIndex();

  // Klassen, die Deltas ankuendigen, gelten als bekannt - auch wenn der
  // zugehoerige config.cpp-Eintrag noch fehlt (wird von deltas.mjs geprueft).
  const announced = new Set();
  for (const file of deltaFiles()) {
    const res = readJson(file);
    if (res.ok && Array.isArray(res.data?.classes)) {
      for (const c of res.data.classes) announced.add(c);
    }
  }

  const known = name => defined.has(name) || announced.has(name) || ref.names.has(name);

  const report = (file, line, name, where) => {
    if (known(name)) return;
    if (name.startsWith('ChefZ_')) {
      // ChefZ-eigene Klasse, die es nicht gibt: eindeutiger Fehler.
      f.error(file, line, `Unbekannte ChefZ-Klasse "${name}" (${where}) - nirgends in einer config.cpp definiert und in keinem Delta angekuendigt`);
    } else if (!ref.vanillaIndexed) {
      // Ohne Vanilla-Index waere jede Vanilla-Referenz ein Fehlalarm.
      f.warn(file, line, `Fremdklasse "${name}" (${where}) nicht pruefbar - vanilla-classes.txt ist leer, siehe build-refindex.mjs`);
    } else {
      f.error(file, line, `Unbekannte Fremdklasse "${name}" (${where}) - weder in Vanilla noch in einem indizierten Mod`);
    }
  };

  // --- Referenzen aus JSON ---
  for (const file of [...jsonFiles(), ...deltaFiles()]) {
    const res = readJson(file);
    if (!res.ok) continue;                     // meldet bereits schema.mjs
    for (const r of collectJsonClassRefs(res.data, file)) {
      report(file, lineOf(file, `"${r.name}"`), r.name, r.key);
    }
  }

  // --- Elternklassen aus config.cpp ---
  for (const file of configCppFiles()) {
    const parsed = parseConfigCpp(file);
    for (const c of parsed.classes) {
      if (!c.parent) continue;
      if (c.scope[0] === 'CfgPatches') continue;
      // Vorwaertsdeklarationen ("class Foo;") definieren die Basis im selben File.
      const fwd = parsed.classes.some(x => x.name === c.parent && !x.declaredBody);
      if (fwd) continue;
      report(file, c.line, c.parent, `Elternklasse von ${c.name}`);
    }
  }

  if (ref.empty) {
    f.warn(null, 0, 'Referenzindex ist leer. Fremdklassen koennen gar nicht geprueft werden. Abhilfe: node tools/chefz-validate/build-refindex.mjs');
  } else if (!ref.vanillaIndexed) {
    f.warn(null, 0, `Referenzindex enthaelt ${ref.names.size} Mod-Klassen, aber keine Vanilla-Item-Klassen. Referenzen auf Vanilla (CookingPot, Edible_Base ...) bleiben ungeprueft. Abhilfe: refindex/vanilla-classes.txt befuellen.`);
  }

  return f;
}
