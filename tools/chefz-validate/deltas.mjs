// Prueft die Registry-Deltas der Content-Slices:
// ID-Kollisionen zwischen Slices, Parent-Kategorien, Stationen, und ob der
// Merge in den zentralen Registries tatsaechlich angekommen ist.

import path from 'node:path';
import {
  Findings, deltaFiles, readJson, projectClasses, lineOf, rel,
  ADDONS_DIR, exists, CORE_REGISTRIES, REGISTRY_DIR, REGISTRY_ADDON,
} from './lib.mjs';

const SECTIONS = {
  categories: 'id',
  tags: 'id',
  processes: 'id',
  nutrition: 'class',
  preservation: 'id',
  states: 'id',
};

function stable(v) { return JSON.stringify(v, Object.keys(v ?? {}).sort()); }

export default function run() {
  const f = new Findings('deltas');
  const files = deltaFiles();
  if (files.length === 0) return f;   // vor M2 normal

  const deltas = [];
  for (const file of files) {
    const res = readJson(file);
    if (!res.ok) continue;            // meldet schema.mjs
    deltas.push({ file, slice: res.data.slice ?? path.basename(file, '.json'), data: res.data });
  }
  deltas.sort((a, b) => a.slice.localeCompare(b.slice));

  // --- 1. ID-Kollisionen zwischen Slices ---
  const merged = {};                  // section -> Map(id -> {slice, file, entry})
  for (const section of Object.keys(SECTIONS)) merged[section] = new Map();

  for (const d of deltas) {
    for (const [section, idKey] of Object.entries(SECTIONS)) {
      const list = Array.isArray(d.data[section]) ? d.data[section] : [];
      for (const entry of list) {
        const id = entry?.[idKey];
        if (typeof id !== 'string') continue;
        const prev = merged[section].get(id);
        if (!prev) {
          merged[section].set(id, { slice: d.slice, file: d.file, entry });
          continue;
        }
        if (stable(prev.entry) === stable(entry)) continue;   // identisch, stiller Dedup

        // Sonderfall, den der Integrator ohne Rueckfrage aufloesen darf:
        // parent null gegen konkreten parent
        if (section === 'categories') {
          const a = prev.entry.parent ?? null, b = entry.parent ?? null;
          const rest = (x) => { const c = { ...x }; delete c.parent; return stable(c); };
          if (rest(prev.entry) === rest(entry) && (a === null || b === null)) {
            if (a === null) merged[section].set(id, { slice: d.slice, file: d.file, entry });
            f.warn(d.file, lineOf(d.file, id), `Kategorie "${id}": Slice "${prev.slice}" und "${d.slice}" unterscheiden sich nur im parent (${a} / ${b}) - der konkrete gewinnt`);
            continue;
          }
        }

        f.error(d.file, lineOf(d.file, id), `ID-Kollision in "${section}": "${id}" wird von Slice "${prev.slice}" (${rel(prev.file)}) und "${d.slice}" unterschiedlich definiert`);
      }
    }
  }

  // --- 2. Parent-Kategorien muessen nach dem Merge existieren ---
  for (const [id, rec] of merged.categories) {
    const parent = rec.entry.parent ?? null;
    if (parent === null) continue;
    if (!merged.categories.has(parent)) {
      f.error(rec.file, lineOf(rec.file, id), `Kategorie "${id}" verweist auf Elternkategorie "${parent}", die nach dem Merge nicht existiert`);
    }
    if (parent === id) {
      f.error(rec.file, lineOf(rec.file, id), `Kategorie "${id}" ist ihr eigener parent`);
    }
  }
  // Zyklen
  for (const [id, rec] of merged.categories) {
    const seen = new Set([id]);
    let cur = rec.entry.parent ?? null;
    while (cur && merged.categories.has(cur)) {
      if (seen.has(cur)) {
        f.error(rec.file, lineOf(rec.file, id), `Kategorie-Zyklus ueber "${id}" -> ... -> "${cur}"`);
        break;
      }
      seen.add(cur);
      cur = merged.categories.get(cur).entry.parent ?? null;
    }
  }

  // --- 3. Stationen muessen existieren ---
  const defined = projectClasses();
  const announced = new Set();
  for (const d of deltas) for (const c of (d.data.classes ?? [])) announced.add(c);

  for (const [id, rec] of merged.processes) {
    const st = rec.entry.station;
    if (!st) continue;
    if (!defined.has(st) && !announced.has(st)) {
      f.error(rec.file, lineOf(rec.file, id), `Prozess "${id}" nennt Station "${st}", die weder definiert noch in einem Delta angekuendigt ist`);
    }
  }

  // --- 3b. Preservation-Records muessen auf einen deklarierten Zustand zeigen ---
  //
  // Ein Record mit scope "state", dessen Zustand niemand deklariert, ist statisch
  // sauber und zur Laufzeit wirkungslos: ChefZ_PreservationManager.Build() findet
  // die ID nicht und die Haltbarkeitsregel greift nie. Das ist als Warnung
  // gefaehrlicher denn als Fehler - ein gruener Lauf laesst ein totes System
  // erledigt aussehen. Deshalb Fehler.
  for (const [id, rec] of merged.preservation) {
    const scope = rec.entry.scope ?? 'state';
    if (scope !== 'state') continue;
    if (merged.states.has(id)) continue;
    f.error(rec.file, lineOf(rec.file, id),
      `Haltbarkeitsregel "${id}" gilt fuer den Zustand "${id}", den kein Delta deklariert `
      + `(Abschnitt "states"). Die Regel ist zur Laufzeit wirkungslos - der Preservation `
      + `Manager findet den Zustand nicht.`);
  }

  // --- 4. angekuendigte Klassen muessen irgendwann real werden ---
  for (const d of deltas) {
    for (const c of (d.data.classes ?? [])) {
      if (!defined.has(c)) {
        f.warn(d.file, lineOf(d.file, c), `Slice "${d.slice}" kuendigt Klasse "${c}" an, die noch in keiner config.cpp definiert ist`);
      }
    }
  }

  // --- 5. Ist der Merge angekommen? ---
  const coreCfg = REGISTRY_DIR;
  if (exists(coreCfg)) {
    for (const name of CORE_REGISTRIES) {
      const file = path.join(coreCfg, name);
      if (!exists(file)) {
        f.warn(file, 0, `Zentrale Registry "${name}" fehlt in ${REGISTRY_ADDON}, obwohl ${deltas.length} Delta(s) vorliegen - Integrator noch nicht gelaufen?`);
        continue;
      }
      const res = readJson(file);
      if (!res.ok) { f.error(file, 0, `Registry nicht lesbar: ${res.error}`); continue; }
      const section = name.replace('.json', '').toLowerCase();
      const expected = merged[section];
      if (!expected) continue;
      // Die Registry hat die Dokumentform des Core: { kind, schemaVersion, records }.
      // Dort ist die Kennung IMMER "id" - auch wo das Delta sie "class" oder
      // "state" nennt. Genau diese Umbenennung ist die Arbeit des Integrators.
      const list = Array.isArray(res.data) ? res.data
        : (res.data.records ?? res.data[section] ?? res.data.entries ?? []);
      const have = new Set(Array.isArray(list) ? list.map(e => e?.id ?? e?.class ?? e?.state).filter(Boolean) : []);
      for (const id of expected.keys()) {
        if (!have.has(id)) {
          f.error(file, 0, `"${id}" steht in einem Delta, fehlt aber in ${name} - der Merge ist unvollstaendig`);
        }
      }
      // Und die Gegenrichtung. Ohne sie ueberlebt ein Record in der Registry,
      // dessen Delta laengst geloescht wurde - etwa weil die Klasse durch eine
      // Vanilla-Klasse ersetzt wurde. Er zeigt dann auf etwas, das es nicht mehr
      // gibt, und niemand meldet es: der Merge prueft nur, was ankommen SOLL.
      for (const id of have) {
        if (!expected.has(id)) {
          f.error(file, 0,
            `"${id}" steht in ${name}, aber in keinem Delta - verwaister Record. `
            + `Entweder das Delta des zustaendigen Slice wurde geloescht, oder jemand hat `
            + `die Registry von Hand bearbeitet. Der Integrator muss neu mergen.`);
        }
      }
    }
  }

  return f;
}
