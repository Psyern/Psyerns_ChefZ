// Prueft, dass jede #STR_CHEFZ_*-Referenz in der stringtable existiert
// und meldet verwaiste Eintraege.

import path from 'node:path';
import { Findings, allModuleDirs, walk, readText, rel, exists } from './lib.mjs';

const STR_RE = /#(STR_CHEFZ_[A-Z0-9_]+)/g;

function parseCsvKeys(file) {
  const txt = readText(file);
  const keys = new Map(); // key -> zeile
  if (!txt) return keys;
  const lines = txt.split(/\r?\n/);
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i].trim();
    if (!line || line.startsWith('"Language"') || line.toLowerCase().startsWith('language,')) continue;
    const first = line.startsWith('"')
      ? (line.match(/^"([^"]*)"/)?.[1] ?? '')
      : line.split(',')[0];
    const key = first.trim();
    if (key.startsWith('STR_')) keys.set(key, i + 1);
  }
  return keys;
}

export default function run() {
  const f = new Findings('stringtable');
  const modules = allModuleDirs();

  // Alle Schluessel aus allen stringtable.csv einsammeln
  const defined = new Map();     // key -> {file, line}
  const csvFiles = [];
  for (const dir of modules) {
    for (const csv of walk(dir, (_p, n) => n.toLowerCase() === 'stringtable.csv')) {
      csvFiles.push(csv);
      for (const [k, line] of parseCsvKeys(csv)) {
        if (defined.has(k)) {
          f.error(csv, line, `Stringtable-Schluessel "${k}" doppelt (auch in ${rel(defined.get(k).file)}:${defined.get(k).line})`);
        } else {
          defined.set(k, { file: csv, line });
        }
      }
    }
  }

  // Alle Referenzen einsammeln
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

  // Verwaiste Eintraege
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
