// Prueft alle ChefZ-JSON-Dateien gegen die erwarteten Formen.
// Bewusst schlank gehalten - kein JSON-Schema-Paket, keine Abhaengigkeiten.

import path from 'node:path';
import { Findings, jsonFiles, deltaFiles, readJson, rel, lineOf } from './lib.mjs';

const T = {
  str: v => typeof v === 'string' && v.length > 0,
  num: v => typeof v === 'number' && Number.isFinite(v),
  int: v => Number.isInteger(v),
  bool: v => typeof v === 'boolean',
  strOrNull: v => v === null || (typeof v === 'string' && v.length > 0),
  arrStr: v => Array.isArray(v) && v.every(x => typeof x === 'string'),
};

// name -> { required: {feld: pruefer}, optional: {feld: pruefer} }
const SHAPES = {
  recipe: {
    required: { RecipeID: T.str, Result: T.str },
    optional: {
      CookingDevice: T.arrStr, Ingredients: Array.isArray, OptionalIngredients: T.arrStr,
      Priority: T.int, Quality: T.str, Portions: T.int, Effects: T.arrStr,
      RequiredTools: T.arrStr, ReturnContainer: T.str, MinTemperature: T.num,
      CookTimeSec: T.num, Enabled: T.bool,
    },
  },
  ingredient: {
    required: { Category: T.str },
    optional: { Amount: T.num, Item: T.str, AnyOf: T.arrStr, Optional: T.bool, State: T.str, Quantity: T.num },
  },
  category: {
    required: { id: T.str },
    optional: { parent: T.strOrNull, displayName: T.str },
  },
  tag: {
    required: { id: T.str },
    optional: { appliesTo: T.arrStr, displayName: T.str },
  },
  process: {
    required: { id: T.str },
    optional: { station: T.str, durationSec: T.num, tool: T.str, xp: T.num },
  },
  nutrition: {
    required: { class: T.str },
    optional: { energy: T.num, water: T.num, stomach: T.num, temperature: T.num, health: T.num, toxicity: T.num },
  },
  // Feldnamen nach ChefZ_PreservationDef: die ID IST das Ziel, "scope" sagt, in
  // welcher Dimension nachgeschlagen wird. Das aeltere Feld "state" bleibt lesbar,
  // damit vorhandene Deltas nicht schlagartig ungueltig werden.
  preservation: {
    required: {},
    optional: {
      id: T.str, state: T.str, scope: T.str,
      spoilageMultiplier: T.num, stopsDecay: T.bool, preventsRotten: T.bool,
      onPlayerMultiplier: T.num, environmentTemperature: v => v === null || typeof v === 'object',
      displayName: T.str,
    },
  },
  // ChefZ_StateDef - die Lebensmittelzustaende, auf die Preservation und Rezepte
  // zeigen. Ohne sie sind alle Zustandsnamen unaufgeloest.
  state: {
    required: { id: T.str },
    optional: {
      displayName: T.str, projectsToVanillaStage: T.str, implies: T.arrStr,
      spoilageMultiplier: T.num, freshnessLifetimeSec: T.num,
      edible: T.bool, terminal: T.bool, preserved: T.bool,
    },
  },
};

const DELTA_SECTIONS = {
  categories: 'category', tags: 'tag', processes: 'process',
  nutrition: 'nutrition', preservation: 'preservation', states: 'state',
};

function checkShape(f, file, obj, shapeName, label) {
  const shape = SHAPES[shapeName];
  if (!shape) return;
  if (obj === null || typeof obj !== 'object' || Array.isArray(obj)) {
    f.error(file, 0, `${label}: erwartet ein Objekt, gefunden ${Array.isArray(obj) ? 'Array' : typeof obj}`);
    return;
  }
  for (const [key, test] of Object.entries(shape.required)) {
    if (!(key in obj)) {
      f.error(file, 0, `${label}: Pflichtfeld "${key}" fehlt`);
    } else if (!test(obj[key])) {
      f.error(file, lineOf(file, `"${key}"`), `${label}: Feld "${key}" hat den falschen Typ`);
    }
  }
  for (const [key, val] of Object.entries(obj)) {
    if (key in shape.required) continue;
    if (key in shape.optional) {
      if (!shape.optional[key](val)) {
        f.error(file, lineOf(file, `"${key}"`), `${label}: Feld "${key}" hat den falschen Typ`);
      }
    } else {
      f.warn(file, lineOf(file, `"${key}"`), `${label}: unbekanntes Feld "${key}" - Tippfehler?`);
    }
  }
}

export default function run() {
  const f = new Findings('schema');

  // --- Rezept-Dateien der Module ---
  for (const file of jsonFiles()) {
    const p = rel(file).toLowerCase();
    const res = readJson(file);
    if (!res.ok) {
      f.error(file, 0, `JSON nicht lesbar: ${res.error}`);
      continue;
    }
    const data = res.data;

    if (p.includes('/config/recipes/')) {
      // Massgeblich ist die Dokumentform, die der Core tatsaechlich laedt
      // (ChefZ_JsonDocs.c): { kind, schemaVersion, records: [...] }.
      // Die beiden aelteren Formen bleiben lesbar, damit vorhandene Dateien
      // nicht schlagartig ungueltig werden.
      const list = Array.isArray(data) ? data
        : (Array.isArray(data.records) ? data.records
          : (Array.isArray(data.Recipes) ? data.Recipes : null));
      if (!list) {
        f.error(file, 0, 'Rezeptdatei muss die Core-Dokumentform { kind, schemaVersion, records: [...] } haben (oder ein Array bzw. ein Objekt mit "Recipes" sein)');
        continue;
      }
      if (data && !Array.isArray(data) && Array.isArray(data.records) && data.kind !== 'recipe') {
        f.warn(file, lineOf(file, '"kind"'), `Rezeptdatei deklariert kind "${data.kind}" statt "recipe" - der Core sortiert sie damit als andere Datensatzart ein`);
      }
      // Bewusst NUR die Huelle. Die Feldstruktur eines Rezepts gehoert
      // ChefZ_RecipeDef im Core und wird von den chefz*-Pruefern gegen den
      // echten Code geprueft. Ein handgepflegter Nachbau davon hier wuerde
      // bei jeder Core-Aenderung driften und Fehler melden, wo keine sind.
      const seen = new Map();
      list.forEach((r, i) => {
        if (r === null || typeof r !== 'object' || Array.isArray(r)) {
          f.error(file, 0, `Rezept #${i + 1}: erwartet ein Objekt`);
          return;
        }
        const id = r.id ?? r.RecipeID;
        if (typeof id !== 'string' || id.length === 0) {
          f.error(file, 0, `Rezept #${i + 1}: hat keine Kennung ("id")`);
          return;
        }
        if (seen.has(id)) {
          f.error(file, lineOf(file, id), `Rezept-Kennung "${id}" doppelt in derselben Datei (auch #${seen.get(id) + 1})`);
        }
        seen.set(id, i);
      });
    }
  }

  // --- Delta-Dateien ---
  for (const file of deltaFiles()) {
    const res = readJson(file);
    if (!res.ok) { f.error(file, 0, `Delta nicht lesbar: ${res.error}`); continue; }
    const d = res.data;
    if (typeof d !== 'object' || d === null || Array.isArray(d)) {
      f.error(file, 0, 'Delta muss ein Objekt sein');
      continue;
    }
    if (!T.str(d.slice)) f.error(file, 0, 'Delta: Pflichtfeld "slice" fehlt oder ist leer');
    const expectedSlice = path.basename(file, '.json');
    if (T.str(d.slice) && d.slice !== expectedSlice) {
      f.error(file, lineOf(file, '"slice"'), `Delta: Feld "slice" ist "${d.slice}", Datei heisst aber "${expectedSlice}.json"`);
    }
    if ('classes' in d && !T.arrStr(d.classes)) {
      f.error(file, lineOf(file, '"classes"'), 'Delta: "classes" muss ein Array aus Strings sein');
    }
    for (const [section, shapeName] of Object.entries(DELTA_SECTIONS)) {
      if (!(section in d)) continue;
      if (!Array.isArray(d[section])) {
        f.error(file, lineOf(file, `"${section}"`), `Delta: "${section}" muss ein Array sein`);
        continue;
      }
      d[section].forEach((entry, i) => {
        checkShape(f, file, entry, shapeName, `${section}[${i}]`);
        // Preservation traegt die Kennung als "id" (Core) oder "state" (altes
        // Delta-Schema) - eines von beiden MUSS da sein.
        if (shapeName === 'preservation' && entry && typeof entry === 'object'
            && !T.str(entry.id) && !T.str(entry.state)) {
          f.error(file, 0, `${section}[${i}]: braucht eine Kennung - "id" (Core-Form) oder "state" (alte Delta-Form)`);
        }
      });
    }
    const known = new Set([...Object.keys(DELTA_SECTIONS), 'slice', 'classes', 'handcraftRecipeSlots']);
    for (const k of Object.keys(d)) {
      if (!known.has(k)) f.warn(file, lineOf(file, `"${k}"`), `Delta: unbekannter Abschnitt "${k}"`);
    }
  }

  return f;
}
