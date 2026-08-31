// Selbsttest-Fundstellen: die Zeilennummern-Literale muessen stimmen.
//
// ChefZ_SelfTestTrace.Fail("Modul", 276, "...") traegt die Zeilennummer als
// LITERAL - der einzige Ort, an dem eine fehlgeschlagene Selbsttestgruppe im
// RPT ihre Stelle nennt. Verschiebt ein Edit die Zeilen und zieht die Literale
// nicht nach, LUEGT jede Diagnose ab da: der Leser sucht an einer Stelle, an
// der der Test nicht steht. Am 31.08.2026 verschob ein Core-Edit 82 von 836
// Literalen; nachgezogen wurde per Wegwerf-Skript, und genau dieses Werkzeug
// fehlte im Pruefnetz. Deshalb dieser Pruefer.
//
// Konvention (im Bestand ausnahmslos eingehalten): der Aufruf steht in EINER
// Zeile, das zweite Argument ist die 1-basierte Zeilennummer dieser Zeile.

import { Findings, allModuleDirs, walk, readText } from './lib.mjs';

const CALL_RE = /ChefZ_SelfTestTrace\.Fail\(\s*"[^"]*"\s*,\s*(\d+)\s*,/g;

export default function run() {
  const f = new Findings('tracelines');
  let total = 0;

  for (const dir of allModuleDirs()) {
    for (const file of walk(dir, (_p, n) => /\.c$/i.test(n))) {
      const txt = readText(file);
      if (!txt || txt.indexOf('ChefZ_SelfTestTrace.Fail(') < 0) continue;
      const lines = txt.split(/\r?\n/);
      for (let i = 0; i < lines.length; i++) {
        CALL_RE.lastIndex = 0;
        let m;
        while ((m = CALL_RE.exec(lines[i])) !== null) {
          total++;
          const declared = parseInt(m[1], 10);
          const actual = i + 1;
          if (declared !== actual) {
            f.error(file, actual,
              `SelfTestTrace.Fail nennt Zeile ${declared}, steht aber in Zeile ${actual} - `
              + `die RPT-Diagnose zeigte auf die falsche Stelle. Literal nachziehen.`);
          }
        }
      }
    }
  }

  // Null Fundstellen hiesse: das Muster hat sich geaendert und der Pruefer
  // prueft ins Leere. Das ist eine Warnung wert, kein stilles Gruen.
  if (total === 0) {
    f.warn(null, 0,
      'Keine einzige ChefZ_SelfTestTrace.Fail-Fundstelle gefunden - entweder wurden '
      + 'die Selbsttests entfernt oder das Aufrufmuster hat sich geaendert. '
      + 'Dann muss dieser Pruefer nachziehen (tracelines.mjs, CALL_RE).');
  }

  return f;
}
