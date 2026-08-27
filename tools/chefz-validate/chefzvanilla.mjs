// Invariante I2: ChefZ nimmt Vanilla das Kochen niemals weg.
//
// Die zentrale Designregel des Projekts lautet: passt kein ChefZ-Rezept, kocht
// DayZ weiter wie immer. Ein ChefZ-Rezept, das sich VOLLSTAENDIG mit
// Vanilla-Zutaten erfuellen laesst, bricht diese Regel von der anderen Seite:
// wer drei Vanilla-Pilze in eine Pfanne legt, bekommt dann ein ChefZ-Gericht
// statt gebratener Pilze - ohne je etwas von ChefZ benutzt zu haben.
//
// Geprueft wird nur, ob ein Rezept vanilla-erfuellbar IST. Ob das im Spiel
// haeufig vorkommt, entscheidet der Mensch; die Regel kennt keinen Grenzwert.

import { Findings, jsonFiles, readJson, rel, lineOf, REGISTRY_DIR, exists } from './lib.mjs';
import path from 'node:path';

const isChefZ = n => typeof n === 'string' && n.startsWith('ChefZ_');

/** Alle Datensaetze einer bestimmten Art aus allen Modul-JSONs. */
function recordsOfKind(kind) {
  const out = [];
  for (const file of jsonFiles()) {
    const res = readJson(file);
    if (!res.ok) continue;
    const d = res.data;
    if (!d || typeof d !== 'object' || Array.isArray(d)) continue;
    if (d.kind !== kind || !Array.isArray(d.records)) continue;
    for (const r of d.records) out.push({ file, rec: r });
  }
  return out;
}

/** parent-Kette der Kategorien aus der zusammengefuehrten Registry. */
function categoryParents() {
  const parents = new Map();
  const file = path.join(REGISTRY_DIR, 'Categories.json');
  if (!exists(file)) return parents;
  const res = readJson(file);
  if (!res.ok || !Array.isArray(res.data?.records)) return parents;
  for (const r of res.data.records) {
    if (typeof r?.id === 'string') parents.set(r.id, r.parent || null);
  }
  return parents;
}

/** Eine Kategorie und alle ihre Vorfahren - ein Slot fuer MEAT nimmt auch DOMESTIC_MEAT. */
function withAncestors(cat, parents) {
  const out = new Set([cat]);
  let cur = parents.get(cat) || null;
  const guard = new Set([cat]);
  while (cur && !guard.has(cur)) {
    out.add(cur);
    guard.add(cur);
    cur = parents.get(cur) || null;
  }
  return out;
}

export default function run() {
  const f = new Findings('chefzvanilla');

  const parents = categoryParents();

  // Klasse -> welche Kategorien und Tags sie (inklusive Vorfahren) erfuellt
  const byCategory = new Map();   // Kategorie -> Set(Klassen)
  const byTag = new Map();        // Tag       -> Set(Klassen)

  for (const { rec } of recordsOfKind('ingredient')) {
    const cls = rec?.id;
    if (typeof cls !== 'string') continue;
    for (const c of (rec.categories ?? [])) {
      for (const anc of withAncestors(c, parents)) {
        if (!byCategory.has(anc)) byCategory.set(anc, new Set());
        byCategory.get(anc).add(cls);
      }
    }
    for (const t of (rec.tags ?? [])) {
      if (!byTag.has(t)) byTag.set(t, new Set());
      byTag.get(t).add(cls);
    }
  }

  if (byCategory.size === 0 && byTag.size === 0) {
    f.warn(null, 0, 'Keine Zutaten-Records gefunden - I2 ist nicht pruefbar. Stimmt die Dokumentform (kind: "ingredient")?');
    return f;
  }

  /** Klassen, die diesen Slot erfuellen koennen. */
  function satisfiers(match) {
    const out = new Set();
    if (!match || typeof match !== 'object') return out;
    const add = m => {
      if (typeof m?.cls === 'string') out.add(m.cls);
      if (typeof m?.category === 'string') for (const c of (byCategory.get(m.category) ?? [])) out.add(c);
      if (typeof m?.tag === 'string') for (const c of (byTag.get(m.tag) ?? [])) out.add(c);
    };
    add(match);
    for (const m of (match.anyOf ?? [])) add(m);
    return out;
  }

  for (const { file, rec } of recordsOfKind('recipe')) {
    const id = rec?.id;
    if (typeof id !== 'string') continue;

    // Ein Rezept kann mehrere Kochkontexte haben - jeder ist ein eigener Weg.
    // Die Slots liegen je nach Rezept im Kontext ODER auf Rezeptebene; im
    // zweiten Fall beschreibt der Kontext nur das Kochgeraet.
    const contexts = Array.isArray(rec.contexts) && rec.contexts.length
      ? rec.contexts
      : [{}];

    for (const ctx of contexts) {
      const slots = Array.isArray(ctx?.slots) && ctx.slots.length ? ctx.slots
        : (Array.isArray(rec.slots) ? rec.slots : []);
      const required = slots.filter(s => s && s.optional !== true);
      if (required.length === 0) continue;

      let allVanilla = true;
      const chefzAnchors = [];
      for (const s of required) {
        const sat = satisfiers(s.match);
        if (sat.size === 0) { allVanilla = false; break; }   // unbekannt, nicht behaupten
        const hasVanilla = [...sat].some(c => !isChefZ(c));
        if (!hasVanilla) {
          allVanilla = false;
          chefzAnchors.push(s.slotId ?? '(ohne slotId)');
        }
      }

      if (allVanilla) {
        const example = required.map(s => {
          const sat = [...satisfiers(s.match)].filter(c => !isChefZ(c));
          return `${s.slotId ?? '?'}=${sat[0] ?? '?'}${(s.minCount ?? 1) > 1 ? ` x${s.minCount}` : ''}`;
        }).join(', ');
        f.error(file, lineOf(file, id),
          `Invariante I2: Rezept "${id}" ist vollstaendig mit Vanilla-Zutaten erfuellbar (${example}). `
          + `Wer diese Zutaten in das Kochgeraet legt, bekommt ein ChefZ-Gericht statt Vanilla-Verhalten, `
          + `ohne je etwas von ChefZ benutzt zu haben. Mindestens eine PFLICHT-Zutat muss ein ChefZ-Item verlangen.`);
      }
    }
  }

  return f;
}
