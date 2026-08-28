// chefzbase - jede Elternklasse muss in DERSELBEN config.cpp bekannt sein.
//
// DayZ loest "class X : Basis" nur innerhalb einer config.cpp auf. Steht die
// Basis dort weder mit Rumpf noch als Vorwaertsdeklaration "class Basis;", dann
// bricht der Configlauf ab:
//     /CfgVehicles.ChefZ_Yeast: Undefined base class 'ChefZ_GrainFoodBase'
// Der Server zeigt das in einem MODALEN FENSTER. Auf einem Server klickt das
// niemand weg - er sieht aus, als haenge er beim Start, ohne eine einzige
// Fehlerzeile im RPT. Genau so ist am 28.08.2026 ein halber Vormittag vergangen.
//
// Eine Vorwaertsdeklaration ueberschreibt nichts. Sie macht den Namen nur
// aufloesbar; der Rumpf kommt weiterhin aus dem Addon, das ihn definiert - und
// die Ladereihenfolge sichert requiredAddons[] zu.

import path from 'node:path';
import { Findings, configCppFiles, readText, stripComments, rel } from './lib.mjs';

export default function run() {
  const f = new Findings('chefzbase');

  const files = configCppFiles();
  if (files.length === 0) return f;

  let checked = 0;

  for (const file of files) {
    const raw = readText(file);
    if (raw === null) continue;
    checked++;
    const text = stripComments(raw);
    const lines = text.split(/\r?\n/);

    // Bekannt ist, was hier einen Rumpf hat ODER vorwaertsdeklariert ist.
    const known = new Set();
    for (const m of text.matchAll(/\bclass\s+([A-Za-z_]\w*)\s*(?::\s*[A-Za-z_]\w*\s*)?\{/g)) known.add(m[1]);
    for (const m of text.matchAll(/\bclass\s+([A-Za-z_]\w*)\s*;/g)) known.add(m[1]);

    const missing = new Map();   // Basis -> [ "Zeile:Kind" ]
    lines.forEach((l, i) => {
      const m = l.match(/\bclass\s+([A-Za-z_]\w*)\s*:\s*([A-Za-z_]\w*)/);
      if (!m) return;
      if (known.has(m[2])) return;
      if (!missing.has(m[2])) missing.set(m[2], []);
      missing.get(m[2]).push({ line: i + 1, child: m[1] });
    });

    for (const [baseName, users] of missing) {
      const first = users[0];
      const more = users.length > 1 ? ` und ${users.length - 1} weitere` : '';
      f.error(file, first.line,
        `"${first.child}"${more} erbt von "${baseName}", das in dieser config.cpp `
        + `nicht bekannt ist. DayZ loest Elternklassen nur je Datei auf und bricht `
        + `sonst mit "Undefined base class" ab - in einem modalen Fenster, das auf `
        + `einem Server niemand wegklickt. Abhilfe: "class ${baseName};" oben in `
        + `denselben Block schreiben; das ueberschreibt nichts.`);
    }
  }

  f.items.push({
    validator: 'chefzbase', severity: 'info', file: '', line: 0,
    summary: `Geprueft: ${checked} config.cpp auf aufloesbare Elternklassen.`,
  });

  return f;
}
