// Das ChefZ-Datenmodell fuer die Validatoren: alle Records aller Quellen in
// EINER Form, plus die daraus gemergten Registries.
//
// Warum eigene Datei und nicht lib.mjs: lib.mjs kennt Dateien, Klassen und
// config.cpp - also Dinge, die jeder DayZ-Mod hat. Hier steht, was ChefZ ist.
// Die Trennung haelt lib.mjs benutzbar, falls jemand die Validatoren spaeter
// fuer ein anderes Modul abwandelt.
//
// Die drei Quellen entsprechen den drei Raengen aus Entwurf 02 §3:
//
//   Rang 1  config.cpp, Knoten CfgChefZ*        (Client UND Server lesen sie)
//   Rang 2  JSON im Addon: { kind, schemaVersion, records[] }
//   Rang 3  $profile:-Overlay                   (statisch nicht sichtbar)
//
// Dazu kommen die Slice-Deltas unter Psyerns_ChefZ_Core/_deltas/ - das
// Uebergabeformat der Content-Slices aus Workflow §2. Sie sind duenner als ein
// vollstaendiger Record, melden aber IDs an, und genau darum geht es hier.
//
// Rang 3 ist statisch nicht pruefbar und ist es auch nicht noetig: 03 §4 verbietet
// dem Overlay, sync-relevante Registries zu erweitern, und der Config Manager
// erzwingt das zur Laufzeit.

import path from 'node:path';
import {
  jsonFiles, deltaFiles, readJson, lineOf, configTrees, ADDONS_DIR,
} from './lib.mjs';

/** Die Record-Arten aus ChefZ_RecordKind.c, in der Ladeordnung aus 02 §6. */
export const KINDS = [
  'coreSettings', 'category', 'tag', 'state', 'qualityTier', 'toolGroup',
  'device', 'container', 'ingredient', 'nutrition', 'preservation',
  'process', 'station', 'transform', 'recipe',
];

/**
 * CfgChefZ*-Wurzelknoten -> Record-Art.
 * Woertlich aus ChefZ_ConfigCppSource.c (Rang 1 liest genau diese zehn).
 */
export const CFG_ROOTS = {
  CfgChefZCategories: 'category',
  CfgChefZTags: 'tag',
  CfgChefZStates: 'state',
  CfgChefZQualityTiers: 'qualityTier',
  CfgChefZTools: 'toolGroup',
  CfgChefZDevices: 'device',
  CfgChefZContainers: 'container',
  CfgChefZIngredients: 'ingredient',
  CfgChefZProcesses: 'process',
  CfgChefZStations: 'station',
};

/** Delta-Abschnitt -> [Record-Art, Schluesselfeld]. */
const DELTA_SECTIONS = {
  categories: ['category', 'id'],
  tags: ['tag', 'id'],
  processes: ['process', 'id'],
  nutrition: ['nutrition', 'class'],
  preservation: ['preservation', 'state'],
};

// --- Records einsammeln -----------------------------------------------------

/**
 * Ein Record, wie ihn die Pruefer sehen:
 *
 *   { kind, id, obj, file, line, rank, origin }
 *
 * `obj` traegt die Felder mit denselben Namen wie im JSON und in den
 * ChefZ_*Def-Klassen - deshalb koennen Rang 1 und Rang 2 gemeinsam geprueft
 * werden, ohne dass irgendwo eine zweite Feldliste entsteht.
 */
let CACHE = null;

export function allRecords() {
  if (CACHE) return CACHE;
  const out = [];

  // --- Rang 2: JSON-Dokumente -------------------------------------------
  for (const file of jsonFiles()) {
    const res = readJson(file);
    if (!res.ok) continue;                       // meldet schema.mjs
    const d = res.data;
    if (!d || typeof d !== 'object' || Array.isArray(d)) continue;
    if (typeof d.kind !== 'string' || !Array.isArray(d.records)) continue;
    for (const rec of d.records) {
      if (!rec || typeof rec !== 'object' || Array.isArray(rec)) continue;
      const id = typeof rec.id === 'string' ? rec.id : '';
      out.push({
        kind: d.kind, id, obj: rec, file,
        line: id ? lineOf(file, `"${id}"`) : 0,
        rank: 2, origin: 'json',
      });
    }
  }

  // --- Rang 1: CfgChefZ* aus den config.cpp ------------------------------
  for (const { file, tree } of configTrees()) {
    for (const node of tree.children) {
      const kind = CFG_ROOTS[node.name];
      if (!kind) continue;
      for (const child of node.children) {
        out.push({
          kind, id: child.name, obj: resolveCfgProps(node, child),
          file, line: child.line, rank: 1, origin: 'config.cpp',
        });
      }
    }
  }

  // --- Slice-Deltas -------------------------------------------------------
  for (const file of deltaFiles()) {
    const res = readJson(file);
    if (!res.ok) continue;
    const d = res.data;
    if (!d || typeof d !== 'object') continue;
    for (const [section, [kind, key]] of Object.entries(DELTA_SECTIONS)) {
      const list = Array.isArray(d[section]) ? d[section] : [];
      for (const entry of list) {
        const id = entry && typeof entry[key] === 'string' ? entry[key] : '';
        if (!id) continue;
        const obj = { ...entry, id };
        if (kind === 'nutrition' && !obj.scope) obj.scope = 'class';
        if (kind === 'preservation' && !obj.scope) obj.scope = 'state';
        out.push({
          kind, id, obj, file, line: lineOf(file, `"${id}"`),
          rank: 2, origin: 'delta',
        });
      }
    }
  }

  CACHE = out;
  return out;
}

/**
 * Felder eines CfgChefZ-Knotens inklusive der Config-Vererbung innerhalb
 * desselben Wurzelknotens.
 *
 * "class ChefZ_HunterSausage : ChefZ_Sausage_Base" INNERHALB von
 * CfgChefZIngredients loest die Engine selbst auf - ConfigIsExisting liefert
 * fuer ein geerbtes Feld bereits true (ChefZ_ConfigCppSource.c, Kopf von
 * ReadIngredients). Wer das hier nicht nachbildet, meldet jedes geerbte Feld
 * als fehlend.
 */
function resolveCfgProps(root, node, seen = new Set()) {
  if (seen.has(node.name)) return { ...node.props };
  seen.add(node.name);
  const parent = node.parent ? root.childMap.get(node.parent) : null;
  const base = parent ? resolveCfgProps(root, parent, seen) : {};
  return { ...base, ...node.props };
}

// --- Registries -------------------------------------------------------------

/**
 * Die Namensraeume, gegen die chefzsym prueft.
 *
 * Jeder ist ein eigener Schluesselraum (03 §3.1: "jede Registry fuehrt ihren
 * eigenen"). Ein Symbol ist genau dann bekannt, wenn IRGENDEIN Record es in
 * diesem Namensraum deklariert - Deklaration heisst je nach Raum "ist die ID
 * eines Records" oder "kommt in dem Feld vor, das den Raum aufspannt".
 */
export const NAMESPACES = {
  category: 'Kategorie',
  tag: 'Tag',
  state: 'Zustand',
  quality: 'Qualitaetsstufe',
  tierSet: 'Qualitaets-Stufensatz',
  toolGroup: 'Werkzeuggruppe',
  toolCategory: 'Werkzeugkategorie',
  deviceCategory: 'Geraetekategorie',
  containerCategory: 'Behaelterkategorie',
  stationCategory: 'Stationskategorie',
  unit: 'Mengeneinheit',
  process: 'Prozess',
  station: 'Station',
  transform: 'Transform',
  recipe: 'Rezept',
};

const asArray = v => {
  if (Array.isArray(v)) return v.filter(x => typeof x === 'string');
  if (typeof v === 'string' && v !== '') return [v];
  return [];
};

/**
 * Die gemergten Registries: Namensraum -> Map(id -> [{file, line}]).
 *
 * Das ist genau die Menge, gegen die 03 E1 den Validator verpflichtet: "alle
 * Symbolreferenzen gegen die GEMERGTEN Registries pruefen".
 */
export function registries() {
  const reg = {};
  for (const ns of Object.keys(NAMESPACES)) reg[ns] = new Map();
  const declare = (ns, id, rec) => {
    if (typeof id !== 'string' || id === '') return;
    if (!reg[ns].has(id)) reg[ns].set(id, []);
    reg[ns].get(id).push({ file: rec.file, line: rec.line, kind: rec.kind });
  };

  for (const rec of allRecords()) {
    const o = rec.obj || {};
    switch (rec.kind) {
      case 'category': declare('category', rec.id, rec); break;
      case 'tag': declare('tag', rec.id, rec); break;
      case 'state': declare('state', rec.id, rec); break;
      case 'qualityTier':
        declare('quality', rec.id, rec);
        declare('tierSet', o.tierSet, rec);
        break;
      case 'toolGroup':
        declare('toolGroup', rec.id, rec);
        for (const c of asArray(o.toolCategories)) declare('toolCategory', c, rec);
        break;
      case 'device':
        for (const c of asArray(o.deviceCategories)) declare('deviceCategory', c, rec);
        break;
      case 'container':
        for (const c of asArray(o.containerCategories)) declare('containerCategory', c, rec);
        break;
      case 'ingredient':
        declare('unit', o.quantityUnit, rec);
        break;
      case 'process': declare('process', rec.id, rec); break;
      case 'station':
        declare('station', rec.id, rec);
        for (const c of asArray(o.stationCategories)) declare('stationCategory', c, rec);
        break;
      case 'transform': declare('transform', rec.id, rec); break;
      case 'recipe': declare('recipe', rec.id, rec); break;
      default: break;
    }
  }
  return reg;
}

// --- Modulzuordnung ---------------------------------------------------------

/** Modulname eines Pfades: "ChefZ_Core", "ChefZ_Meat", "Psyerns_ChefZ_COT_Comp" ... */
export function moduleOf(file) {
  const rel = path.relative(ADDONS_DIR, file);
  if (!rel.startsWith('..')) return rel.split(path.sep)[0];
  return path.basename(path.dirname(file));
}

export function isCoreFile(file) {
  return moduleOf(file) === 'ChefZ_Core';
}

// --- Item-Klassen aus den config.cpp ---------------------------------------
//
// chefznut und chefzstage fragen dasselbe: "hat diese Klasse - eigen oder
// geerbt - den Unterknoten X, und was steht in Feld Y". Die Vererbung laeuft
// dabei entlang der CfgVehicles-Kette, so wie die Engine sie aufloest; endet die
// Kette bei einer Klasse, die das Projekt nicht kennt (Edible_Base, Bottle_Base
// ...), ist die Antwort ausdruecklich "unbekannt" und nicht "nein". Der
// Unterschied entscheidet ueber Fehler oder Warnung.

const ITEM_ROOTS = /^Cfg(Vehicles|Weapons|Magazines)$/;

let ITEM_CACHE = null;

/** Klassenname -> { node, file, parent } fuer alle Item-Klassen des Projekts. */
export function configItemIndex() {
  if (ITEM_CACHE) return ITEM_CACHE;
  const map = new Map();
  for (const { file, tree } of configTrees()) {
    for (const root of tree.children) {
      if (!ITEM_ROOTS.test(root.name)) continue;
      const visit = (node) => {
        // Vorwaertsdeklarationen ("class Lard;") ueberspringen. Sie definieren
        // nichts - sie machen eine FREMDE Basisklasse sichtbar. Als Projektklasse
        // gezaehlt, greift der Vanilla-Skip der nachgelagerten Pruefer nicht mehr,
        // und Lard wird gemeldet, weil es "weder Nutrition noch Food" habe - was
        // in Vanilla selbstverstaendlich beides hat.
        if (node.hasBody === false) return;
        if (!map.has(node.name)) map.set(node.name, { node, file, parent: node.parent });
        for (const c of node.children) visit(c);
      };
      for (const c of root.children) visit(c);
    }
  }
  ITEM_CACHE = map;
  return map;
}

/**
 * Die Vererbungskette einer Item-Klasse, solange sie im Projekt liegt.
 * @return { chain: [{name, entry}], external: string|null }
 *         `external` ist die erste Elternklasse ausserhalb des Projekts -
 *         typischerweise die Vanilla-Basis.
 */
export function configChain(name) {
  const index = configItemIndex();
  const chain = [];
  const seen = new Set();
  let cur = name;
  while (cur && index.has(cur) && !seen.has(cur)) {
    seen.add(cur);
    const entry = index.get(cur);
    chain.push({ name: cur, entry });
    cur = entry.parent;
  }
  return { chain, external: cur && !index.has(cur) ? cur : null };
}

/** Unterknoten "name" der Klasse, eigen oder geerbt. null, wenn keiner. */
export function resolveChild(className, childName) {
  const { chain } = configChain(className);
  for (const { entry } of chain) {
    const child = entry.node.childMap.get(childName);
    if (child) return { node: child, owner: entry, file: entry.file };
  }
  return null;
}

/**
 * Feldwert der Klasse, eigen oder geerbt. undefined, wenn nirgends gesetzt.
 * `owner` ist das Kettenglied { name, entry } - der Name gehoert in die
 * Meldung, sonst steht dort "geerbt von undefined".
 */
export function resolveProp(className, prop) {
  const { chain } = configChain(className);
  for (const link of chain) {
    if (Object.prototype.hasOwnProperty.call(link.entry.node.props, prop)) {
      return { value: link.entry.node.props[prop], owner: link };
    }
  }
  return undefined;
}
