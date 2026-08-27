// Gemeinsame Hilfsfunktionen der ChefZ-Validatoren.
// Node >= 18, keine externen Abhaengigkeiten.

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

export const TOOL_DIR = path.dirname(fileURLToPath(import.meta.url));

// Die Projektwurzel ist normalerweise zwei Ebenen ueber diesem Verzeichnis.
// CHEFZ_VALIDATE_ROOT haengt sie um - das ist der Pruefstand-Haken: die
// Selbstpruefung der Validatoren (siehe README, "Wegwerf-Modul") laesst denselben
// Code ueber einen absichtlich fehlerhaften Baum laufen, ohne diesen Baum ins
// Projekt legen zu muessen.
export const ROOT = process.env.CHEFZ_VALIDATE_ROOT
  ? path.resolve(process.env.CHEFZ_VALIDATE_ROOT)
  : path.resolve(TOOL_DIR, '..', '..');
export const MOD_ROOT = path.join(ROOT, 'Psyerns_ChefZ_Core');
export const ADDONS_DIR = path.join(MOD_ROOT, 'Addons');
export const DELTA_DIR = path.join(MOD_ROOT, '_deltas');
export const REFINDEX_DIR = path.join(TOOL_DIR, 'refindex');

export const COMP_DIRS = [
  path.join(ROOT, 'Psyerns_ChefZ_Terje_Skills_Comp'),
  path.join(ROOT, 'Psyerns_ChefZ_Terje_Medicine_Comp'),
  path.join(ROOT, 'Psyerns_ChefZ_COT_Comp'),
];

// Die zusammengefuehrten Registries. Sie liegen seit Meilenstein 2 NICHT mehr in
// ChefZ_Core: der Core ist eine Regelmaschine ohne eigenes Vokabular und liefert
// keine Datensaetze aus. Ihr Addon deklariert sie in dataFiles[] - nur dadurch
// sind sie ueberhaupt ladbar.
export const REGISTRY_ADDON = 'ChefZ_Registry';
export const REGISTRY_DIR = path.join(ADDONS_DIR, REGISTRY_ADDON, 'Config');

// Prozesse fehlen hier absichtlich: ihre Records gehoeren den Slices, die sie in
// Rang 1 und Rang 2 selbst deklarieren. Die Registry wuerde sie im selben Rang
// ein zweites Mal einbringen, und ChefZ_RecordSink weist einen doppelten Record
// desselben Rangs ab, statt ihn zu patchen.
export const CORE_REGISTRIES = [
  'Categories.json', 'Tags.json',
  'Nutrition.json', 'Preservation.json',
];

// --- Befund-Sammler -------------------------------------------------------

export class Findings {
  constructor(validator) {
    this.validator = validator;
    this.items = [];
  }
  error(file, line, summary, extra = {}) {
    this.items.push({ validator: this.validator, severity: 'error', file: rel(file), line, summary, ...extra });
  }
  warn(file, line, summary, extra = {}) {
    this.items.push({ validator: this.validator, severity: 'warning', file: rel(file), line, summary, ...extra });
  }
  get errors() { return this.items.filter(i => i.severity === 'error').length; }
  get warnings() { return this.items.filter(i => i.severity === 'warning').length; }
}

export function rel(p) {
  if (!p) return '';
  const r = path.relative(ROOT, p);
  return r.startsWith('..') ? p : r.split(path.sep).join('/');
}

// --- Dateisystem ----------------------------------------------------------

export function exists(p) {
  try { fs.accessSync(p); return true; } catch { return false; }
}

export function walk(dir, predicate) {
  const out = [];
  if (!exists(dir)) return out;
  const stack = [dir];
  while (stack.length) {
    const cur = stack.pop();
    let entries;
    try { entries = fs.readdirSync(cur, { withFileTypes: true }); } catch { continue; }
    for (const e of entries) {
      const full = path.join(cur, e.name);
      if (e.isDirectory()) {
        if (e.name === 'node_modules' || e.name === '.git') continue;
        stack.push(full);
      } else if (!predicate || predicate(full, e.name)) {
        out.push(full);
      }
    }
  }
  return out.sort();
}

/** Alle Addon-Verzeichnisse des Hauptmods. */
export function addonDirs() {
  if (!exists(ADDONS_DIR)) return [];
  return fs.readdirSync(ADDONS_DIR, { withFileTypes: true })
    .filter(e => e.isDirectory())
    .map(e => path.join(ADDONS_DIR, e.name))
    .sort();
}

/** Alle Mod-Wurzeln: Hauptmod-Addons plus die Comp-Mods. */
export function allModuleDirs() {
  return [...addonDirs(), ...COMP_DIRS.filter(exists)];
}

export function configCppFiles() {
  return allModuleDirs().flatMap(d => walk(d, (_f, n) => n.toLowerCase() === 'config.cpp'));
}

export function jsonFiles() {
  return allModuleDirs().flatMap(d => walk(d, (_f, n) => n.toLowerCase().endsWith('.json')));
}

export function deltaFiles() {
  return walk(DELTA_DIR, (_f, n) => n.toLowerCase().endsWith('.json'));
}

export function readJson(file) {
  try {
    const raw = fs.readFileSync(file, 'utf8').replace(/^﻿/, '');
    return { ok: true, data: JSON.parse(raw) };
  } catch (err) {
    return { ok: false, error: err.message };
  }
}

export function readText(file) {
  try { return fs.readFileSync(file, 'utf8').replace(/^﻿/, ''); } catch { return null; }
}

// --- config.cpp ------------------------------------------------------------

export function stripComments(src) {
  // Blockkommentare und Zeilenkommentare entfernen, Zeilenzahl erhalten.
  let out = src.replace(/\/\*[\s\S]*?\*\//g, m => m.replace(/[^\n]/g, ' '));
  out = out.replace(/\/\/[^\n]*/g, m => ' '.repeat(m.length));
  return out;
}

/**
 * Sehr toleranter config.cpp-Leser. Liefert alle Klassendeklarationen mit
 * Elternklasse, Zeile und Verschachtelungspfad, plus die CfgPatches-Bloecke.
 */
export function parseConfigCpp(file) {
  const raw = readText(file);
  if (raw === null) return { classes: [], patches: [], arrays: {}, raw: '' };
  const src = stripComments(raw);

  const classes = [];
  const chain = [];
  // Fuer JEDE offene "{" merken, ob sie einen Klassenrumpf geoeffnet hat.
  // Ohne das zaehlt die schliessende Klammer eines Array-Literals
  // (units[] = {...}) als Klassenende, und die gesamte Verschachtelung
  // darunter verrutscht - "Food/FoodStages" statt
  // "CfgVehicles/ChefZ_Wheat/Food/FoodStages".
  const braces = [];
  let line = 1;

  const declRe = /class\s+([A-Za-z_]\w*)\s*(?::\s*([A-Za-z_]\w*)\s*)?/y;

  for (let i = 0; i < src.length; i++) {
    const ch = src[i];
    if (ch === '\n') { line++; continue; }

    // Zeichenketten ueberspringen - sie duerfen Klammern enthalten.
    if (ch === '"') {
      i++;
      while (i < src.length && src[i] !== '"') { if (src[i] === '\n') line++; i++; }
      continue;
    }

    if (ch === '{') { braces.push(false); continue; }   // Klammer ohne Klassenkopf
    if (ch === '}') {
      const wasClass = braces.pop();
      if (wasClass && chain.length) chain.pop();
      continue;
    }

    if (ch === 'c' && /\bclass\b/.test(src.slice(i, i + 6)) && (i === 0 || /[^\w]/.test(src[i - 1]))) {
      declRe.lastIndex = i;
      const m = declRe.exec(src);
      if (m) {
        const after = src.slice(declRe.lastIndex).match(/^\s*([{;])/);
        const name = m[1];
        const parent = m[2] || null;
        if (after && after[1] === '{') {
          classes.push({ name, parent, line, scope: chain.slice(), declaredBody: true });
          chain.push(name);
          braces.push(true);          // diese "{" gehoert einem Klassenrumpf
          i = declRe.lastIndex + after[0].length - 1;
          continue;
        } else if (after && after[1] === ';') {
          // Vorwaertsdeklaration - keine echte Definition.
          classes.push({ name, parent, line, scope: chain.slice(), declaredBody: false });
          i = declRe.lastIndex + after[0].length - 1;
          continue;
        }
      }
    }
  }

  // CfgPatches-Eintraege einsammeln
  const patches = classes
    .filter(c => c.scope.length === 1 && c.scope[0] === 'CfgPatches' && c.declaredBody)
    .map(c => ({ name: c.name, line: c.line }));

  // Array-Werte (units[], requiredAddons[], model=) grob einsammeln
  const arrays = {};
  const arrRe = /([A-Za-z_]\w*)\s*\[\s*\]\s*=\s*\{([^}]*)\}/g;
  let am;
  while ((am = arrRe.exec(src)) !== null) {
    const key = am[1];
    const vals = am[2].split(',').map(s => s.trim().replace(/^"|"$/g, '')).filter(Boolean);
    (arrays[key] ||= []).push(...vals);
  }

  const models = [];
  const modelRe = /\bmodel\s*=\s*"([^"]*)"/g;
  let mm;
  while ((mm = modelRe.exec(src)) !== null) models.push(mm[1]);

  return { classes, patches, arrays, models, raw, src };
}

/** Alle im Projekt tatsaechlich definierten Klassen (mit Body). */
export function projectClasses() {
  const map = new Map(); // name -> [{file, line}]
  for (const f of configCppFiles()) {
    for (const c of parseConfigCpp(f).classes) {
      if (!c.declaredBody) continue;
      if (c.scope.length === 0) continue;      // CfgVehicles, CfgPatches selbst
      if (c.scope[0] === 'CfgPatches') continue;
      if (!map.has(c.name)) map.set(c.name, []);
      map.get(c.name).push({ file: f, line: c.line, parent: c.parent, scope: c.scope });
    }
  }
  return map;
}

// --- Referenzindex ---------------------------------------------------------

/**
 * Bekannte Fremdklassen (Vanilla/Terje/...).
 *
 * `vanillaIndexed` ist separat ausgewiesen und entscheidet ueber die Schwere von
 * Funden: die Vanilla-ITEM-Klassen stecken in den Game-Configs, nicht in den
 * Script-Quellen. Solange vanilla-classes.txt leer ist, waere jede Referenz auf
 * eine Vanilla-Klasse (CookingPot, FryingPan, Edible_Base ...) ein Fehlalarm.
 * Deshalb: unbekannte Fremdklassen sind dann nur eine Warnung.
 */
export function refIndex() {
  const names = new Set();
  const sources = [];
  let vanillaCount = 0;
  if (!exists(REFINDEX_DIR)) return { names, sources, empty: true, vanillaIndexed: false };
  for (const f of walk(REFINDEX_DIR, (_f, n) => n.endsWith('.txt'))) {
    const txt = readText(f);
    if (!txt) continue;
    let count = 0;
    for (const l of txt.split(/\r?\n/)) {
      const s = l.trim();
      if (!s || s.startsWith('#')) continue;
      names.add(s);
      count++;
    }
    if (path.basename(f) === 'vanilla-classes.txt') vanillaCount = count;
    sources.push({ file: rel(f), count });
  }
  return { names, sources, empty: names.size === 0, vanillaIndexed: vanillaCount > 0 };
}

/** Sammelt jeden ChefZ-Klassennamen, der in JSON-Dateien referenziert wird. */
export function collectJsonClassRefs(obj, file, acc = [], keyPath = '') {
  // Felder, deren Wert ein KLASSENNAME ist. Die zweite Zeile ist die Erweiterung
  // aus 19 S19 ("classrefs erw."): cls/portionClass/emptyClass/
  // emptyOnLastPortion/returnContainer sind die Klassenfelder des Rezept-,
  // Transform- und Behaeltermodells (08 §3, 15 §3, 16 §3.1).
  const CLASS_KEYS = new Set([
    'Item', 'Result', 'ReturnContainer', 'class', 'station', 'Station',
    'cls', 'portionClass', 'emptyClass', 'emptyOnLastPortion', 'returnContainer',
  ]);
  const ARRAY_CLASS_KEYS = new Set([
    'CookingDevice', 'OptionalIngredients', 'appliesTo', 'classes', 'Tools', 'AnyOf',
    'deviceClasses',
  ]);
  // Werte, die zwar in einem Klassenfeld stehen, aber keine Klasse benennen.
  // "AUTO" heisst "nimm den Behaelter, aus dem die Zutat kam" (16 §4).
  const NOT_A_CLASS = new Set(['', 'AUTO']);
  if (obj === null || typeof obj !== 'object') return acc;
  if (Array.isArray(obj)) {
    for (const v of obj) collectJsonClassRefs(v, file, acc, keyPath);
    return acc;
  }
  for (const [k, v] of Object.entries(obj)) {
    if (typeof v === 'string' && CLASS_KEYS.has(k)) {
      if (!NOT_A_CLASS.has(v)) acc.push({ name: v, file, key: `${keyPath}${k}` });
    } else if (Array.isArray(v) && ARRAY_CLASS_KEYS.has(k)) {
      for (const s of v) if (typeof s === 'string' && !NOT_A_CLASS.has(s)) acc.push({ name: s, file, key: `${keyPath}${k}[]` });
    } else if (typeof v === 'object') {
      collectJsonClassRefs(v, file, acc, `${keyPath}${k}.`);
    }
  }
  return acc;
}

/** Findet die Zeilennummer eines Suchbegriffs in einer Datei (1-basiert, 0 wenn nicht gefunden). */
export function lineOf(file, needle) {
  const txt = readText(file);
  if (!txt) return 0;
  const lines = txt.split(/\r?\n/);
  for (let i = 0; i < lines.length; i++) if (lines[i].includes(needle)) return i + 1;
  return 0;
}

// --- config.cpp als BAUM ---------------------------------------------------
//
// parseConfigCpp() liefert eine flache Klassenliste - fuer CfgPatches und
// Namenspruefungen genau richtig. Die Pruefer chefznut, chefzstage und chefzsym
// brauchen dagegen den BAUM mitsamt den Feldwerten: "hat ChefZ_X einen
// Unterknoten Nutrition", "was steht in scope", "welche Kinder hat
// CfgChefZCategories". Dafuer ist der folgende Leser da.
//
// Bewusst tolerant und ohne Anspruch auf einen vollstaendigen Enfusion-Parser:
// er kennt Klassen, Zuweisungen (skalar und Array) und sonst nichts. Was er
// nicht versteht, ueberspringt er - ein Validator, der an einer exotischen
// Schreibweise aussteigt, waere schlechter als einer, der sie ignoriert.

function newNode(name, parent, line) {
  return { name, parent, line, props: {}, children: [], childMap: new Map() };
}

/**
 * config.cpp als Baum. Wurzelknoten heisst "" und traegt CfgPatches, CfgMods,
 * CfgVehicles, CfgChefZ* ... als Kinder.
 *
 *   node = { name, parent, line, props, children[], childMap }
 *   props["scope"]        = 0            (Zahl oder String)
 *   props["categories"]   = ["A","B"]    (aus "categories[] = {...}")
 */
export function parseConfigTree(file) {
  const raw = readText(file);
  const root = newNode('', null, 0);
  if (raw === null) return root;
  const src = stripComments(raw);

  const stack = [root];
  let i = 0, line = 1;
  const n = src.length;

  const addChild = (node, child) => {
    node.children.push(child);
    if (!node.childMap.has(child.name)) node.childMap.set(child.name, child);
  };

  while (i < n) {
    const ch = src[i];
    if (ch === '\n') { line++; i++; continue; }
    if (ch === ' ' || ch === '\t' || ch === '\r' || ch === ';') { i++; continue; }
    if (ch === '}') { if (stack.length > 1) stack.pop(); i++; continue; }
    if (ch === '{') { i++; continue; }          // Block ohne Kopf - ignorieren

    // Klassendeklaration
    const cls = /^class\s+([A-Za-z_]\w*)\s*(?::\s*([A-Za-z_]\w*)\s*)?/.exec(src.slice(i));
    if (cls && (i === 0 || /[^\w]/.test(src[i - 1]))) {
      const rest = src.slice(i + cls[0].length);
      const open = /^\s*\{/.exec(rest);
      const node = newNode(cls[1], cls[2] || null, line);
      // Ohne "{" ist das eine Vorwaertsdeklaration ("class Edible_Base;") -
      // DayZ-Standard, um eine FREMDE Basisklasse sichtbar zu machen. Sie
      // definiert nichts. Wer Content zaehlt, muss sie auslassen, sonst gilt
      // jede Vanilla-Basisklasse als Erfindung des Moduls.
      node.hasBody = !!open;
      addChild(stack[stack.length - 1], node);
      const consumed = cls[0].length + (open ? open[0].length : 0);
      for (const c of src.slice(i, i + consumed)) if (c === '\n') line++;
      i += consumed;
      if (open) stack.push(node);
      continue;
    }

    // Zuweisung:  name = wert;   oder   name[] = { ... };
    const assign = /^([A-Za-z_]\w*)\s*(\[\s*\])?\s*=\s*/.exec(src.slice(i));
    if (assign) {
      let j = i + assign[0].length;
      let value;
      if (assign[2] || src[j] === '{') {
        const close = src.indexOf('}', j);
        const body = close < 0 ? '' : src.slice(j + 1, close);
        value = body.split(',').map(s => s.trim().replace(/^"|"$/g, '')).filter(s => s.length > 0);
        j = close < 0 ? n : close + 1;
      } else {
        const end = src.indexOf(';', j);
        const rawVal = (end < 0 ? src.slice(j) : src.slice(j, end)).trim();
        value = rawVal.replace(/^"|"$/g, '');
        j = end < 0 ? n : end + 1;
      }
      for (const c of src.slice(i, j)) if (c === '\n') line++;
      const node = stack[stack.length - 1];
      node.props[assign[1]] = value;
      if (!node.propLines) node.propLines = {};
      node.propLines[assign[1]] = line;
      i = j;
      continue;
    }

    i++;                                        // alles andere: weiter
  }
  return root;
}

/** Alle config.cpp-Baeume des Projekts, mit Dateipfad. */
export function configTrees() {
  return configCppFiles().map(f => ({ file: f, tree: parseConfigTree(f) }));
}

/** Alle Enforce-Skriptdateien der Module. */
export function scriptFiles() {
  return allModuleDirs().flatMap(d => walk(d, (_f, n) => n.toLowerCase().endsWith('.c')));
}

/**
 * Alle Enforce-Skripte des Core - der Geltungsbereich von chefzcore und chefzlog.
 *
 * Bewusst das GANZE Modulverzeichnis und nicht nur Scripts/: 19 S19 schreibt
 * "Addons/ChefZ_Core/Scripts/**", aber Testskripte liegen unter Tests/<name>/
 * Scripts/ und werden ueber missionScriptModule genauso ausgeliefert. Was im PBO
 * landet, faellt unter I3 und I4 - unabhaengig davon, in welchem Unterordner es
 * liegt.
 */
export function coreScriptFiles() {
  const dir = path.join(ADDONS_DIR, 'ChefZ_Core');
  return walk(dir, (_f, n) => n.toLowerCase().endsWith('.c'));
}

/**
 * Alle im Projekt deklarierten SKRIPT-Klassen (nicht Config-Klassen).
 * name -> { parent, file, line, modded }
 */
export function scriptClasses() {
  const map = new Map();
  const re = /(?:^|\n)\s*(modded\s+)?class\s+([A-Za-z_]\w*)\s*(?:(?::|extends)\s*([A-Za-z_]\w*))?/g;
  for (const f of scriptFiles()) {
    const txt = readText(f);
    if (!txt) continue;
    const src = stripComments(txt);
    let m;
    while ((m = re.exec(src)) !== null) {
      const name = m[2];
      const line = src.slice(0, m.index).split('\n').length;
      const entry = { parent: m[3] || null, file: f, line, modded: !!m[1] };
      if (m[1]) continue;                       // modded class erweitert, deklariert nicht
      if (!map.has(name)) map.set(name, entry);
    }
  }
  return map;
}
