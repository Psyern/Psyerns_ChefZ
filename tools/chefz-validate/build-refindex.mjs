#!/usr/bin/env node
// Baut den Referenzindex bekannter Fremdklassen fuer classrefs.mjs und naming.mjs.
//
//   node tools/chefz-validate/build-refindex.mjs
//       indexiert die Mod-Repositories (Terje, Expansion, COT, CF, Dabs)
//
//   node tools/chefz-validate/build-refindex.mjs "D:/entpackte_dayz_data"
//       indexiert zusaetzlich einen Ordner mit entpackten Vanilla-Configs
//
// Ergebnis: refindex/<quelle>-classes.txt, eine Klasse je Zeile.
//
// WICHTIG - bekannte Grenze:
// Die Vanilla-Item-Klassen von DayZ stehen NICHT in "scripts - 1.29" (das sind
// Script-Klassen, keine Config-Klassen). Fuer eine vollstaendige Pruefung braucht
// es entweder entpackte Game-Data oder einen Klassendump vom Server. Solange der
// fehlt, melden die Validatoren unbekannte Fremdklassen nur als Warnung.

import fs from 'node:fs';
import path from 'node:path';
import { REFINDEX_DIR, walk, parseConfigCpp, exists, readText } from './lib.mjs';

const REPOS = 'C:/Users/Administrator/Desktop/Mod Repositories';

const SOURCES = [
  { name: 'terje', dir: path.join(REPOS, 'TerjeMods-master-main') },
  { name: 'expansion', dir: path.join(REPOS, 'DayZExpansion') },
  { name: 'cot', dir: path.join(REPOS, 'DayZ-CommunityOnlineTools-production') },
  { name: 'cf', dir: path.join(REPOS, 'DayZ-CommunityFramework-production') },
  { name: 'dabs', dir: path.join(REPOS, 'DayZ-Dabs-Framework-production') },
  // Vanilla-SCRIPT-Klassen (nicht die Item-Config-Klassen - siehe Hinweis oben).
  // Deckt Basisklassen und modded-class-Ziele ab.
  { name: 'vanilla-scripts', dir: path.join(REPOS, 'scripts - 1.29') },
];

for (const extra of process.argv.slice(2)) {
  SOURCES.push({ name: path.basename(extra).toLowerCase().replace(/[^a-z0-9]+/g, '-'), dir: extra });
}

fs.mkdirSync(REFINDEX_DIR, { recursive: true });

let total = 0;
for (const src of SOURCES) {
  if (!exists(src.dir)) {
    console.log(`uebersprungen (nicht gefunden): ${src.dir}`);
    continue;
  }
  const names = new Set();

  // 1. Klassen aus config.cpp
  for (const file of walk(src.dir, (_p, n) => n.toLowerCase() === 'config.cpp')) {
    for (const c of parseConfigCpp(file).classes) {
      if (c.scope.length === 0) continue;
      if (c.scope[0] === 'CfgPatches') continue;
      names.add(c.name);
    }
  }

  // 2. Script-Klassen aus .c-Dateien (fuer "modded class"-Ziele und Basisklassen)
  for (const file of walk(src.dir, (_p, n) => n.endsWith('.c'))) {
    const txt = readText(file);
    if (!txt) continue;
    const re = /^\s*(?:modded\s+)?class\s+([A-Za-z_]\w*)/gm;
    let m;
    while ((m = re.exec(txt)) !== null) names.add(m[1]);
  }

  const out = path.join(REFINDEX_DIR, `${src.name}-classes.txt`);
  const sorted = [...names].sort();
  fs.writeFileSync(out,
    `# Referenzindex: ${src.name}\n# Quelle: ${src.dir}\n# Erzeugt von build-refindex.mjs\n` +
    sorted.join('\n') + '\n', 'utf8');
  console.log(`${src.name}: ${sorted.length} Klassen -> ${path.relative(process.cwd(), out)}`);
  total += sorted.length;
}

const vanilla = path.join(REFINDEX_DIR, 'vanilla-classes.txt');
if (!exists(vanilla)) {
  fs.writeFileSync(vanilla,
    '# Vanilla-DayZ-Klassen - NOCH NICHT BEFUELLT\n' +
    '#\n' +
    '# Solange diese Datei leer ist, koennen classrefs.mjs und naming.mjs\n' +
    '# Kollisionen mit Vanilla-Item-Klassen nicht pruefen und melden nur Warnungen.\n' +
    '#\n' +
    '# So befuellen (eine der beiden Wege):\n' +
    '#  a) DayZ-Game-Data entpacken und dann:\n' +
    '#     node tools/chefz-validate/build-refindex.mjs "<pfad zu den entpackten Daten>"\n' +
    '#  b) Auf einem Testserver einen Klassendump erzeugen und die Namen hier\n' +
    '#     einfuegen - eine Klasse je Zeile, Zeilen mit # sind Kommentare.\n', 'utf8');
  console.log(`\nHinweis: ${path.relative(process.cwd(), vanilla)} angelegt, aber leer.`);
  console.log('Vanilla-Kollisionen bleiben bis zur Befuellung ungeprueft.');
}

console.log(`\nGesamt indexiert: ${total} Klassen.`);
