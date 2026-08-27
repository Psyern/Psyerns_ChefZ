// chefzlog - kein ungewachter Debug/Trace in einer Schleife (18 E2).
//
// Der Grund, woertlich aus 18 E2:
//
//   "Enforce wertet Argumente VOR dem Aufruf aus. ChefZ_Log.Debug(ch, "x" + a + b)
//    baut den String IMMER, auch bei abgeschaltetem Log. Im Matcher-Loop mit vier
//    Items und drei Kandidaten sind das Dutzende Allokationen pro Tick pro
//    Feuerstelle - fuer nichts. Die Wache ist haesslich und muss diszipliniert
//    eingehalten werden. Deshalb wird sie nicht empfohlen, sondern vom statischen
//    Validator geprueft."
//
// Erlaubt sind drei Formen der Wache:
//
//   1. im selben Ausdruck
//        if (ChefZ_Log.Enabled(ch, lvl))
//            ChefZ_Log.Debug(ch, "..." + x);
//   2. als umschliessender Block
//        if (irgendwas && ChefZ_Log.Enabled(ch, lvl))
//        {
//            ChefZ_Log.Debug(...);
//            ChefZ_Log.Debug(...);
//        }
//   3. als fruehe Rueckkehr am Anfang eines Blocks
//        if (!ChefZ_Log.Enabled(ch, lvl))
//            return;
//        ... alles Weitere gilt als gewacht
//
// ChefZ_Log.Once, Error, Warn und Info sind NICHT betroffen: sie sind entweder
// dedupliziert oder gehoeren zu Ereignissen, die selten sind und gemeldet
// gehoeren. Der Trace laeuft ueber ChefZ_MatchTrace und ist ein Nullobjekt
// (18 E3) - auch er braucht diese Wache nicht.
//
// Der Leser ist ein kleiner Zustandsautomat und kein Enforce-Parser: er zaehlt
// Klammern, merkt sich die Art jedes Blocks und den Text der laufenden
// Anweisung. Das reicht fuer diese eine Frage und kann an keiner exotischen
// Schreibweise aussteigen.

import { Findings, coreScriptFiles, readText, stripComments, rel } from './lib.mjs';

const GUARDED_CALLS = /\bChefZ_Log\.(Debug|Trace)\s*\(/;
const ENABLED = /\bChefZ_Log\.Enabled\s*\(/;
const LOOP_HEAD = /\b(for|foreach|while)\s*\($/;

/**
 * @return [{ line, call, reason }] - jeder ungewachte Aufruf in einer Schleife.
 */
export function scan(src) {
  const findings = [];
  const stack = [{ kind: 'file', loop: false, guard: false, guardedFrom: false, line: 0 }];
  let stmt = '';                 // Text der laufenden Anweisung
  let stmtLine = 1;
  let line = 1;
  let paren = 0;

  const inLoop = () => stack.some(b => b.loop);
  const guarded = () => stack.some(b => b.guard || b.guardedFrom);

  // Variablen, die NUR bei eingeschaltetem Kanal einen Wert bekommen. Sie sind
  // das Nullobjekt-Muster aus 18 E3:
  //
  //     array<string> trace = null;
  //     if (ChefZ_Log.Enabled(kanal, stufe))
  //         trace = new array<string>();
  //     ...
  //     if (trace)                        // <- ebenso gut wie Enabled()
  //         ChefZ_Log.Debug(...);
  //
  // Wer das nicht kennt, meldet den saubersten Fall im ganzen Projekt als
  // Verstoss - und der Autor lernt, dem Pruefer nicht zu glauben.
  const guardVars = new Set();
  const IF_VAR = /\bif\s*\(\s*([A-Za-z_]\w*)\s*\)/;

  const noteGuardVars = (text) => {
    for (const m of text.matchAll(/([A-Za-z_]\w*)\s*=[^=]/g)) guardVars.add(m[1]);
  };
  const guardedByVar = (text) => {
    const m = IF_VAR.exec(text);
    return !!(m && guardVars.has(m[1]));
  };

  const flushStatement = () => {
    const text = stmt.trim();
    if (text) {
      const isEarlyReturn = ENABLED.test(text) && /\breturn\b/.test(text) && /^\s*if\s*\(\s*!/.test(text);
      if (isEarlyReturn) stack[stack.length - 1].guardedFrom = true;

      // Zuweisung unter der Wache - die Variable traegt die Wache weiter.
      if (ENABLED.test(text) || guarded()) noteGuardVars(text);

      // Schleife OHNE geschweifte Klammern:  for (...) ChefZ_Log.Debug(...);
      // Dann steht der Schleifenkopf in derselben Anweisung wie der Aufruf, und
      // es wurde nie ein Block geoeffnet, den der Stapel kennen koennte.
      const loopHead = text.search(/\b(for|foreach|while)\s*\(/);
      const callAt = text.search(GUARDED_CALLS);
      const bareLoop = loopHead >= 0 && callAt > loopHead;

      if (GUARDED_CALLS.test(text) && (inLoop() || bareLoop)
          && !guarded() && !ENABLED.test(text) && !guardedByVar(text)) {
        findings.push({ line: stmtLine, call: text.replace(/\s+/g, ' ').slice(0, 120) });
      }
    }
    stmt = '';
  };

  for (let i = 0; i < src.length; i++) {
    const ch = src[i];
    if (ch === '\n') { line++; stmt += ' '; continue; }
    if (ch === '(') paren++;
    if (ch === ')') paren = Math.max(0, paren - 1);

    if (ch === ';' && paren === 0) { flushStatement(); stmtLine = line; continue; }

    if (ch === '{') {
      const head = stmt.trim();
      // Der Kopf endet auf ")" - fuer die Art des Blocks zaehlt, was davor steht.
      const isLoop = LOOP_HEAD.test(head.replace(/\)[^)]*$/, ')').replace(/\([\s\S]*$/, '(')) || /^\s*do\b/.test(head)
        || /\b(for|foreach|while)\s*\(/.test(head);
      const isIf = /\bif\s*\(/.test(head);
      const isGuard = isIf && (ENABLED.test(head) || guardedByVar(head));
      // Eine Schleife IN einem if-Kopf gibt es nicht; ein if gewinnt, weil der
      // Kopf dann mit "if (" beginnt.
      const kind = isIf ? 'if' : (isLoop ? 'loop' : 'block');
      stack.push({
        kind,
        loop: kind === 'loop',
        guard: isGuard,
        guardedFrom: false,
        line,
      });
      stmt = '';
      stmtLine = line;
      continue;
    }

    if (ch === '}') {
      flushStatement();
      if (stack.length > 1) stack.pop();
      stmt = '';
      stmtLine = line;
      continue;
    }

    if (stmt === '') stmtLine = line;
    stmt += ch;
  }
  flushStatement();
  return findings;
}

export default function run() {
  const f = new Findings('chefzlog');
  const files = coreScriptFiles();
  if (files.length === 0) {
    f.warn(null, 0, 'Keine Core-Skripte gefunden - chefzlog hat nichts geprueft.');
    return f;
  }

  let calls = 0;
  for (const file of files) {
    const txt = readText(file);
    if (!txt) continue;
    const src = stripComments(txt);
    calls += (src.match(/\bChefZ_Log\.(Debug|Trace)\s*\(/g) || []).length;
    for (const hit of scan(src)) {
      f.error(file, hit.line,
        `Ungewachter Log-Aufruf in einer Schleife: ${hit.call}. `
        + `Enforce baut die Argumente auch bei abgeschaltetem Kanal (18 E2) - im Kochtick sind das `
        + `Dutzende Allokationen fuer nichts. Abhilfe: `
        + `if (ChefZ_Log.Enabled(kanal, stufe)) davor, oder am Blockanfang `
        + `if (!ChefZ_Log.Enabled(...)) return;`);
    }
  }

  f.items.push({
    validator: 'chefzlog', severity: 'info', file: rel(files[0] || ''), line: 0,
    summary: `Geprueft: ${calls} Debug/Trace-Aufrufe in ${files.length} Core-Skripten.`,
  });

  return f;
}
