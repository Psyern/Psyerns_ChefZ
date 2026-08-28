// chefzmanaged - Klassen, die in einem ref-Container liegen, muessen Managed sein.
//
// Enforce zaehlt Referenzen nur fuer verwaltete Klassen. Steht eine unverwaltete
// Klasse in "array<ref X>", meldet der Compiler
//     autoptr/ref in template is not supported on non-managed class 'X'
// und der Container haelt das Objekt NICHT am Leben. Das ist keine Stilfrage:
// der Zeiger wird irgendwann ungueltig, und der Absturz passiert weit weg von
// der Ursache.
//
// Am 28.08.2026 traf das 47 Klassen des Cores auf einmal - 153 Warnungen im
// Serverprotokoll. Sichtbar wurde es erst, als der Mod ueberhaupt zum Uebersetzen
// kam; deshalb steht die Regel jetzt hier und nicht im Kopf eines Menschen.
//
// Vererbung zaehlt: wer von einer verwalteten Klasse erbt, ist selbst verwaltet.
// Der Pruefer folgt der Kette deshalb bis zur Wurzel.

import { Findings, scriptFiles, readText, stripComments, lineOf, rel } from './lib.mjs';

// Basisklassen, die Enforce selbst als verwaltet fuehrt.
const MANAGED_ROOTS = new Set(['Managed', 'Class', 'Object', 'EntityAI', 'ItemBase', 'Entity']);

/** class X            -> { base: null }
 *  class X : Y        -> { base: 'Y' }
 *  class X extends Y  -> { base: 'Y' }                                        */
// "modded class X" bleibt bewusst draussen: das erweitert eine vorhandene -
// meist Vanilla- - Klasse und fuehrt keine neue ein. Wer es mitzaehlt, haelt X
// fuer basislos und meldet "Cooking ist nicht verwaltet", obwohl die Basis
// schlicht ausserhalb des Projekts liegt.
const DECL = /^\s*class\s+([A-Za-z_]\w*)\s*(?::|extends)?\s*([A-Za-z_]\w*)?\s*$/;

/** ref X innerhalb eines Containers: array<ref X>, set<ref X>, map<K, ref X> */
const REF_IN_TEMPLATE = /<[^<>]*\bref\s+([A-Za-z_]\w*)\s*>/g;

/** ref X name; oder ref X name = ...  - der einfache Besitzzeiger.
 *
 * Der Compiler warnt hier NICHT. Die Folge ist trotzdem dieselbe und schlimmer:
 * beim Container faellt es als Warnung auf, beim einfachen Member gar nicht -
 * bis der Speicher unter dem Zeiger weg ist. Am 28.08.2026 hat der Server nach
 * vollstaendigem Start und vollstaendigem Boot beim Selbsttest mit
 * "SEH exception, Exception code: 0xc0000374" (Heap corruption) abgebrochen. */
const REF_MEMBER = /\bref\s+([A-Za-z_]\w*)\s+[A-Za-z_]\w*\s*[;=]/g;

export default function run() {
  const f = new Findings('chefzmanaged');

  const files = scriptFiles();
  if (files.length === 0) return f;

  // 1. Klassenbaum des Projekts einsammeln.
  const base = new Map();      // Klasse -> Elternklasse oder null
  const declaredIn = new Map(); // Klasse -> Datei
  for (const file of files) {
    const raw = readText(file);
    if (raw === null) continue;
    for (const line of stripComments(raw).split(/\r?\n/)) {
      const m = line.match(DECL);
      if (!m) continue;
      if (base.has(m[1])) continue;      // erste Deklaration gewinnt
      base.set(m[1], m[2] ?? null);
      declaredIn.set(m[1], file);
    }
  }

  /** Haengt die Klasse ueber ihre Kette an etwas Verwaltetem? */
  function isManaged(cls) {
    const seen = new Set();
    let cur = cls;
    while (cur && !seen.has(cur)) {
      seen.add(cur);
      const parent = base.get(cur);
      if (parent === undefined) return true;   // fremde Klasse - nicht unsere Aussage
      if (parent === null) return false;       // basislos und im Projekt -> unverwaltet
      if (MANAGED_ROOTS.has(parent)) return true;
      if (!base.has(parent)) return true;      // Elternklasse ist fremd
      cur = parent;
    }
    return false;
  }

  // 2. Jede Verwendung in einem ref-Container pruefen.
  const reported = new Set();
  let checked = 0;

  for (const file of files) {
    const raw = readText(file);
    if (raw === null) continue;
    checked++;
    const lines = stripComments(raw).split(/\r?\n/);
    lines.forEach((line, i) => {
      const hits = [
        ...[...line.matchAll(REF_IN_TEMPLATE)].map(m => ({ cls: m[1], kind: 'einem ref-Container' })),
        ...[...line.matchAll(REF_MEMBER)].map(m => ({ cls: m[1], kind: 'einem ref-Member' })),
      ];
      for (const h of hits) {
        const cls = h.cls;
        if (!base.has(cls)) continue;          // fremde Klasse
        if (isManaged(cls)) continue;
        if (reported.has(cls)) continue;
        reported.add(cls);
        const where = declaredIn.get(cls);
        f.error(where, lineOf(where, `class ${cls}`),
          `"${cls}" haengt an ${h.kind} (erste Verwendung: ${rel(file)}:${i + 1}), `
          + `ist aber nicht verwaltet. Enforce zaehlt dann keine Referenzen: der `
          + `Besitzer haelt das Objekt nicht am Leben. Im Container meldet der `
          + `Compiler wenigstens "autoptr/ref in template is not supported on `
          + `non-managed class"; beim einfachen Member schweigt er, und der Fehler `
          + `zeigt sich erst als Heap-Korruption zur Laufzeit. `
          + `Abhilfe: "class ${cls} : Managed" - so haelt Vanilla es auch (siehe `
          + `class Param : Managed in param.c).`);
      }
    });
  }

  f.items.push({
    validator: 'chefzmanaged', severity: 'info', file: '', line: 0,
    summary: `Geprueft: ${checked} Skripte, ${base.size} Klassen im Baum.`,
  });

  return f;
}
