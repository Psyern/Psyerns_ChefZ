// Stringtables: Schluessel vorhanden, Spalten vollstaendig, keine leeren Zellen.
//
// DayZ liest stringtable.csv mit einem festen Spaltensatz. Fehlt eine Sprache,
// faellt sie im Spiel auf den Rohschluessel zurueck - "STR_CHEFZ_ITEM_BREAD"
// statt "Brot". Das ist kein Fehler, den die Engine meldet; es steht einfach da.
// Deshalb wird der Spaltensatz hier geprueft und nicht nur die Existenz der Zeile.

import path from 'node:path';
import { Findings, allModuleDirs, walk, readText, rel, exists } from './lib.mjs';

const STR_RE = /#(STR_CHEFZ_[A-Z0-9_]+)/g;

// Der Spaltensatz, den Vanilla, Terje, Expansion und COT gleichermassen fuehren.
// Kleinschreibung ist nicht kosmetisch - so steht er in jeder Fremdquelle.
export const COLUMNS = [
  'Language', 'original',
  'english', 'czech', 'german', 'russian', 'polish', 'hungarian',
  'italian', 'spanish', 'french', 'chinese', 'japanese', 'portuguese',
  'chinesesimp',
];

/** Eine CSV-Zeile in Felder zerlegen. Beachtet Anfuehrungszeichen und "" als Escape. */
export function parseCsvLine(line) {
  const out = [];
  let cur = '';
  let inQuotes = false;
  for (let i = 0; i < line.length; i++) {
    const ch = line[i];
    if (inQuotes) {
      if (ch === '"') {
        if (line[i + 1] === '"') { cur += '"'; i++; }
        else inQuotes = false;
      } else cur += ch;
    } else if (ch === '"') {
      inQuotes = true;
    } else if (ch === ',') {
      out.push(cur); cur = '';
    } else cur += ch;
  }
  out.push(cur);
  // Ein abschliessendes Komma erzeugt ein leeres Restfeld - das ist die Form,
  // in der die Fremdmods ihre Tabellen schreiben, und kein fehlender Wert.
  if (out.length && out[out.length - 1] === '') out.pop();
  return out;
}

function parseCsv(file) {
  const txt = readText(file);
  if (txt === null) return null;
  const lines = txt.split(/\r?\n/);
  const rows = [];
  for (let i = 0; i < lines.length; i++) {
    if (!lines[i].trim()) continue;
    rows.push({ line: i + 1, fields: parseCsvLine(lines[i]) });
  }
  return rows;
}

export default function run() {
  const f = new Findings('stringtable');
  const modules = allModuleDirs();

  const defined = new Map();     // key -> {file, line}
  const csvFiles = [];

  for (const dir of modules) {
    for (const csv of walk(dir, (_p, n) => n.toLowerCase() === 'stringtable.csv')) {
      csvFiles.push(csv);

      // --- Ablageort ---
      // Die Tabelle muss neben der config.cpp liegen, nicht in einem
      // Unterverzeichnis. Belegt an jedem Referenzmod im Suchraum:
      // TerjeCore/stringtable.csv, TerjeMedicine/stringtable.csv,
      // DayZExpansion/languagecore/<Modul>/stringtable.csv,
      // JM/COT/languagecore/stringtable.csv, DabsFramework/Scripts/stringtable.csv
      // - ausnahmslos <Ordner mit config.cpp>/stringtable.csv.
      //
      // Ausfallbild bei falscher Ablage: der Spieler liest den Rohschluessel,
      // also "STR_CHEFZ_ACTION_PROCESS" statt "Verarbeiten". Die Engine meldet
      // dazu NICHTS - weder RPT noch Ladefehler. Genau deshalb steht die Regel
      // hier und nicht in einer Konvention.
      //
      // Gate 1 hatte den Verdacht schon (GATE_1_REPORT, Conflict-Scout), er
      // blieb unbewiesen und damit unbehoben. Am 29.08.2026 im Spiel bestaetigt.
      if (!exists(path.join(path.dirname(csv), 'config.cpp'))) {
        f.error(csv, 1, 'stringtable.csv liegt nicht neben der config.cpp. DayZ findet sie dort nicht - im Spiel erscheinen die rohen STR_-Schluessel, ohne jede Meldung. Sie gehoert in denselben Ordner wie die config.cpp des Moduls.');
      }

      const rows = parseCsv(csv);
      if (!rows || rows.length === 0) {
        f.error(csv, 0, 'stringtable.csv ist leer oder nicht lesbar');
        continue;
      }

      // --- Kopfzeile ---
      const header = rows[0].fields;
      const missingCols = COLUMNS.filter(c => !header.includes(c));
      const wrongCase = COLUMNS.filter(c =>
        !header.includes(c) && header.some(h => h.toLowerCase() === c.toLowerCase()));

      if (missingCols.length) {
        const shown = missingCols.filter(c => !wrongCase.includes(c));
        if (shown.length) {
          f.error(csv, 1,
            `Stringtable fuehrt die Sprachspalte(n) ${shown.join(', ')} nicht. `
            + `DayZ zeigt Spielern dieser Sprachen dann den rohen Schluesselnamen. `
            + `Vollstaendiger Satz: ${COLUMNS.join(', ')}`);
        }
        for (const c of wrongCase) {
          const actual = header.find(h => h.toLowerCase() === c.toLowerCase());
          f.error(csv, 1,
            `Spalte "${actual}" muss "${c}" heissen - Vanilla, Terje, Expansion und COT `
            + `schreiben die Sprachnamen klein.`);
        }
      }

      // --- Datenzeilen ---
      const colIndex = new Map(header.map((h, i) => [h, i]));
      for (let r = 1; r < rows.length; r++) {
        const { line, fields } = rows[r];
        const key = (fields[0] ?? '').trim();
        if (!key.startsWith('STR_')) continue;

        if (defined.has(key)) {
          f.error(csv, line, `Stringtable-Schluessel "${key}" doppelt (auch in ${rel(defined.get(key).file)}:${defined.get(key).line})`);
        } else {
          defined.set(key, { file: csv, line });
        }

        // Leere Zellen in vorhandenen Sprachspalten
        const empty = [];
        for (const c of COLUMNS) {
          if (c === 'Language') continue;
          const idx = colIndex.get(c);
          if (idx === undefined) continue;              // fehlende Spalte: schon gemeldet
          if (!(fields[idx] ?? '').trim()) empty.push(c);
        }
        if (empty.length) {
          f.error(csv, line,
            `"${key}": leere Uebersetzung in ${empty.join(', ')} - im Spiel erscheint der rohe Schluesselname.`);
        }
      }
    }
  }

  // --- Referenzen aus Code und Configs ---
  const used = new Set();
  for (const dir of modules) {
    const files = walk(dir, (_p, n) => /\.(cpp|c|json|layout|xml)$/i.test(n));
    for (const file of files) {
      if (path.basename(file).toLowerCase() === 'stringtable.csv') continue;
      const txt = readText(file);
      if (!txt) continue;
      const lines = txt.split(/\r?\n/);
      for (let i = 0; i < lines.length; i++) {
        STR_RE.lastIndex = 0;
        let m;
        while ((m = STR_RE.exec(lines[i])) !== null) {
          const key = m[1];
          used.add(key);
          if (!defined.has(key)) {
            f.error(file, i + 1, `Stringtable-Schluessel "${key}" wird verwendet, ist aber in keiner stringtable.csv definiert - im Spiel erscheint der rohe Schluesselname`);
          }
        }
      }
    }
  }

  for (const [key, loc] of defined) {
    if (key.startsWith('STR_CHEFZ_') && !used.has(key)) {
      f.warn(loc.file, loc.line, `Stringtable-Schluessel "${key}" ist definiert, wird aber nirgends verwendet`);
    }
  }

  if (modules.length > 0 && csvFiles.length === 0) {
    f.warn(null, 0, 'Keine stringtable.csv gefunden - sobald es Items gibt, braucht jedes Modul eine.');
  }

  return f;
}
