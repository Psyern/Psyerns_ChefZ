// chefzswitch - case-Marken muessen echte Konstanten sein, keine Ausdruecke.
//
// Enforce faltet einen Initialisierer wie "1 << 3" nicht zu einer Konstante.
// Steht so ein Name als case-Marke, uebersetzt der switch anstandslos und
// TRIFFT ZUR LAUFZEIT NICHTS. Kein Compilerfehler, keine Warnung, kein Eintrag
// im Protokoll - der Zweig faellt einfach durch.
//
// Warum das eine eigene Regel wert ist: am 28.08.2026 hat genau das den
// Testserver zum Absturz gebracht, NACHDEM alle fuenf Skriptmodule fehlerfrei
// uebersetzt waren und die Mission lief. ChefZ_LogChannel.Name() faellt bei
// einem nicht getroffenen switch in einen rekursiven Zweig; aus dem stillen
// Durchfallen wurde "Virtual Machine Exception, Reason: Stack overflow". Kein
// anderer Pruefer dieser Suite konnte das sehen, weil syntaktisch alles
// in Ordnung ist.
//
// Vanilla schreibt seine switch-Konstanten deshalb als Literale
// (human.c:349ff.: "static const int STATE_NONE = 0;").

import { Findings, scriptFiles, readText, stripComments, rel } from './lib.mjs';

const CONST_DECL = /^\s*(?:static\s+)?const\s+(?:int|float)\s+([A-Za-z_]\w*)\s*=\s*([^;]+);/;
const CASE_LABEL = /^\s*case\s+([A-Za-z_]\w*)\s*:/;

/** Ist der Initialisierer ein schlichtes Literal? */
function isLiteral(expr) {
  return /^\s*-?(?:0[xX][0-9a-fA-F]+|[0-9]+(?:\.[0-9]+)?)\s*$/.test(expr);
}

export default function run() {
  const f = new Findings('chefzswitch');

  const files = scriptFiles();
  if (files.length === 0) return f;

  // Konstanten projektweit einsammeln - eine case-Marke darf aus einer anderen
  // Datei stammen.
  const decl = new Map();   // Name -> { expr, file, line }
  for (const file of files) {
    const raw = readText(file);
    if (raw === null) continue;
    stripComments(raw).split(/\r?\n/).forEach((line, i) => {
      const m = line.match(CONST_DECL);
      if (!m) return;
      if (decl.has(m[1])) return;
      decl.set(m[1], { expr: m[2].trim(), file, line: i + 1 });
    });
  }

  let checked = 0;
  const reported = new Set();

  for (const file of files) {
    const raw = readText(file);
    if (raw === null) continue;
    checked++;
    stripComments(raw).split(/\r?\n/).forEach((line, i) => {
      const m = line.match(CASE_LABEL);
      if (!m) return;
      const d = decl.get(m[1]);
      if (!d) return;                    // Enum oder fremde Konstante
      if (isLiteral(d.expr)) return;
      const key = `${file}:${i + 1}:${m[1]}`;
      if (reported.has(key)) return;
      reported.add(key);
      f.error(file, i + 1,
        `case-Marke "${m[1]}" ist mit einem Ausdruck definiert `
        + `("${d.expr}" in ${rel(d.file)}:${d.line}). Enforce faltet den nicht zu `
        + `einer Konstante: der switch uebersetzt, trifft aber zur Laufzeit nie - `
        + `still, ohne Fehlermeldung. Abhilfe: den Wert als Literal schreiben, `
        + `wie Vanilla es tut.`);
    });
  }

  f.items.push({
    validator: 'chefzswitch', severity: 'info', file: '', line: 0,
    summary: `Geprueft: ${checked} Skripte, ${decl.size} Konstanten.`,
  });

  return f;
}
