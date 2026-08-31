//==============================================================================
// model.mjs - vom Mod zum Renderbild.
//
// Der Brief verlangt eine "strukturierte Datei" als Quelle und verbietet hart
// codierte Einzelrezepte. Die strukturierte Datei GIBT ES SCHON: die Rezepte
// dieses Mods stehen in Addons/*/Config/Recipes/*.json. Eine zweite, von Hand
// gepflegte recipes.json waere am Tag ihrer Erstellung veraltet.
//
// Gelesen wird deshalb mit dem PARSER DER VALIDATOREN (chefzdata.allRecords).
// Beide sehen damit dasselbe: was der Validator als Rezept kennt, kommt hier
// auf eine Karte, und was er nicht kennt, fehlt auch hier. Dasselbe Prinzip
// wie in tools/chefz-assets/check-todo.mjs.
//
// --- Was eine Zelle ist ---------------------------------------------------
// Ein Slot ist keine Zelle. Ein Slot sagt "1 bis 3 Nudeln"; das Raster zeigt
// die MINDESTMENGE, weil das die Frage des Spielers ist ("was brauche ich?").
// Die Spanne steht als Kuerzel an der Zelle, wenn sie groesser als 1 ist.
//==============================================================================

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const REPO = path.resolve(HERE, '..', '..');

// --- Stringtable: STR-Schluessel -> englischer Text --------------------------
// Spalte 2 ist "english" (Kopfzeile jeder stringtable.csv).
function loadStrings() {
  const out = new Map();
  const addons = path.join(REPO, 'Psyerns_ChefZ_Core', 'Addons');
  if (!fs.existsSync(addons)) return out;
  for (const dir of fs.readdirSync(addons)) {
    const f = path.join(addons, dir, 'stringtable.csv');
    if (!fs.existsSync(f)) continue;
    for (const line of fs.readFileSync(f, 'utf8').split(/\r?\n/)) {
      if (!line.startsWith('"STR_')) continue;
      const cells = splitCsv(line);
      if (cells.length > 2 && cells[0]) out.set(cells[0], cells[2] || cells[1] || '');
    }
  }
  return out;
}

// Kleiner CSV-Leser: nur Anfuehrungszeichen und Kommata, das reicht fuer dieses
// Format und haelt das Werkzeug abhaengigkeitsfrei.
function splitCsv(line) {
  const out = []; let cur = ''; let q = false;
  for (let i = 0; i < line.length; i++) {
    const c = line[i];
    if (c === '"') { if (q && line[i + 1] === '"') { cur += '"'; i++; } else q = !q; }
    else if (c === ',' && !q) { out.push(cur); cur = ''; }
    else cur += c;
  }
  out.push(cur);
  return out;
}

/** "RCP_ChefZ_SurvivorSpaghetti" -> "Survivor Spaghetti" (Notnagel). */
function prettifyId(id) {
  return String(id)
    .replace(/^(RCP|REC)_/, '')
    .replace(/^ChefZ_/, '')
    .replace(/([a-z0-9])([A-Z])/g, '$1 $2')
    .replace(/_/g, ' ')
    .trim();
}

/** Der Prozess, den die Karte als Symbol zeigt. */
function processOf(obj) {
  const ctx = (obj.contexts || [])[0] || {};
  const methods = ctx.methods || [];
  const devices = ctx.deviceClasses || [];
  if (methods.includes('BAKING')) return { key: 'fire',  label: devices[0] || 'BAKE' };
  if (methods.includes('BOILING')) return { key: 'boil', label: devices[0] || 'BOIL' };
  return { key: 'craft', label: devices[0] || 'CRAFT' };
}

/**
 * Ein einzelner match-Term -> lesbarer Text.
 * Ein Term ist { category } | { tag } | { cls } - oder { not: <Term> }.
 */
function termText(t, strings, catNames) {
  if (!t || typeof t !== 'object') return '?';
  if (t.not) return '−' + termText(t.not, strings, catNames);   // Minuszeichen
  if (t.category) return catNames.get(t.category) || pretty(t.category);
  if (t.tag) return pretty(String(t.tag).replace(/^CHEFZ_/, ''));
  if (t.cls) return niceClass(t.cls, strings);
  return '?';
}

/** Stabiler Bildschluessel. Ein Anzeigename taugt dafuer nicht. */
function termKey(t) {
  if (!t || typeof t !== 'object') return null;
  if (t.cls) return t.cls;
  if (t.category) return 'cat:' + t.category;
  if (t.tag) return 'tag:' + t.tag;
  return null;
}

/**
 * Ein Slot -> Beschreibung dessen, was hineingehoert, plus Label und
 * Bildschluessel. match kennt cls | category | tag | anyOf | allOf,
 * und innerhalb von allOf/anyOf zusaetzlich not.
 */
function describeSlot(slot, strings, catNames) {
  const m = slot.match || {};
  let what = '?', label = null, imageKey = null;

  if (m.cls) {
    what = niceClass(m.cls, strings); imageKey = m.cls;
  } else if (m.category) {
    what = catNames.get(m.category) || pretty(m.category); label = 'ANY'; imageKey = 'cat:' + m.category;
  } else if (m.tag) {
    what = pretty(String(m.tag).replace(/^CHEFZ_/, '')); label = 'ANY'; imageKey = 'tag:' + m.tag;
  } else if (Array.isArray(m.anyOf)) {
    what = m.anyOf.map(x => termText(x, strings, catNames)).join(' / ');
    label = 'OR';
    imageKey = termKey(m.anyOf[0]);   // das erste Glied steht stellvertretend
  } else if (Array.isArray(m.allOf)) {
    // Der erste positive Term traegt die Bedeutung, die not-Terme schraenken ein.
    const pos = m.allOf.filter(x => x && !x.not);
    const neg = m.allOf.filter(x => x && x.not);
    const head = pos.map(x => termText(x, strings, catNames)).join(' + ') || '?';
    what = neg.length ? head + ' ' + neg.map(x => termText(x, strings, catNames)).join(' ') : head;
    label = neg.length ? 'EXCEPT' : 'ALL';
    imageKey = termKey(pos[0]);
  }
  if (slot.optional) label = 'OPTIONAL';
  return { what, label, imageKey };
}

function pretty(s) {
  return String(s || '').split('_').map(w => w.charAt(0) + w.slice(1).toLowerCase()).join(' ');
}

function niceClass(cls, strings) {
  const key = `STR_CHEFZ_ITEM_${String(cls).replace(/^ChefZ_/, '').toUpperCase()}`;
  return strings.get(key) || prettifyId(cls);
}

/**
 * Baut das Renderbild aller Rezepte.
 * @param {object} chefzdata - das importierte Validator-Modul
 * @param {object} opts - { sort }
 */
export function buildModel(chefzdata, opts = {}) {
  const strings = loadStrings();
  const all = chefzdata.allRecords();

  const catNames = new Map();
  for (const r of all) {
    if (r.kind !== 'category') continue;
    const dn = r.obj.displayName;
    const txt = dn && dn.startsWith('#') ? strings.get(dn.slice(1)) : dn;
    catNames.set(r.id, txt || pretty(r.id));
  }

  const problems = [];
  const recipes = [];

  for (const r of all) {
    if (r.kind !== 'recipe') continue;
    const o = r.obj;
    const outputs = o.outputs || [];
    if (!outputs.length) { problems.push(`${r.id}: kein outputs[] - Karte ohne Ergebnis, uebersprungen`); continue; }
    const out = outputs[0];
    if (!out.cls) { problems.push(`${r.id}: outputs[0] ohne cls - uebersprungen`); continue; }

    const slots = o.slots || [];
    if (!slots.length) problems.push(`${r.id}: keine slots[] - Karte zeigt ein leeres Raster`);

    const cells = [];
    for (const s of slots) {
      const d = describeSlot(s, strings, catNames);
      const min = Number.isFinite(s.minCount) ? s.minCount : 1;
      const max = Number.isFinite(s.maxCount) ? s.maxCount : min;
      const n = Math.max(1, min);
      for (let i = 0; i < n; i++) {
        cells.push({
          ...d,
          // Die Spanne nur an der ERSTEN Zelle des Slots, sonst wird es Brei.
          span: (i === 0 && max > min) ? `${min}-${max}` : null,
          optional: !!s.optional,
        });
      }
    }

    recipes.push({
      id: r.id,
      name: niceClass(out.cls, strings),
      cells,
      process: processOf(o),
      cookSeconds: o.cookSeconds || null,
      minTemperature: o.minTemperature || null,
      result: { cls: out.cls, quantity: out.quantity || null, state: out.setState || null },
      container: out.returnContainer || null,
      origin: r.file,
    });
  }

  // --- Gleichnamige Rezepte unterscheidbar machen --------------------------
  // Fuenf Gerichte haben mehrere Rezepte mit DEMSELBEN Ergebnis: eine kleine
  // Portion, eine Gruppenportion (1200 g), teils eine Bruehe-Variante. Zwei
  // gleich betitelte Karten waeren fuer den Leser ein Fehler, kein Detail.
  // Der Unterschied steckt in der ID - genau ihr abweichender Teil wird der
  // Zusatz. Bleibt nichts uebrig, entscheidet die Menge.
  const byName = new Map();
  for (const r of recipes) {
    if (!byName.has(r.name)) byName.set(r.name, []);
    byName.get(r.name).push(r);
  }
  for (const [, group] of byName) {
    if (group.length < 2) continue;
    let prefix = group[0].id;
    for (const r of group) {
      let i = 0;
      while (i < prefix.length && i < r.id.length && prefix[i] === r.id[i]) i++;
      prefix = prefix.slice(0, i);
    }
    for (const r of group) {
      const rest = r.id.slice(prefix.length).replace(/^[_-]+/, '');
      const variant = rest ? prettifyId(rest) : (r.result.quantity ? r.result.quantity + ' G' : null);
      if (variant) r.variant = variant.toUpperCase();
      else problems.push(`${r.id}: teilt den Namen "${r.name}" und laesst sich nicht unterscheiden`);
    }
  }

  const sort = opts.sort || 'name';
  const cmp = {
    name: (a, b) => a.name.localeCompare(b.name),
    id: (a, b) => a.id.localeCompare(b.id),
    slots: (a, b) => a.cells.length - b.cells.length || a.name.localeCompare(b.name),
  }[sort];
  if (!cmp) throw new Error(`Unbekannte Sortierung "${sort}" - erlaubt: name, id, slots`);
  recipes.sort(cmp);

  return { recipes, problems, catNames };
}
