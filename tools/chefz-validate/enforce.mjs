// enforce - die harten Enforce-Script-Regeln, mechanisch geprueft.
//
// Quelle: die Regeltabelle der enforce-script-Skill, die ihrerseits auf
// Frameworks/Safe-AI-CodingPrompt.md im Enforce-Wiki verweist. Jede Regel hier
// ist compiler- oder speicherkritisch - keine Stilfrage.
//
// Warum als Pruefer und nicht als Durchsicht: das Projekt hat ueber 180
// Skriptdateien. Ein Ternaer-Operator oder ein GetGame() faellt beim Lesen nicht
// auf, kostet aber einen Compilerfehler oder einen Absturz auf dem Server - und
// der Compiler meldet bei genau diesen beiden Faellen bekanntlich die falsche
// Datei und Zeile.

import { Findings, walk, readText, stripComments, rel, ADDONS_DIR, COMP_DIRS, exists } from './lib.mjs';

/** Zeichenketten neutralisieren, damit Regeln nicht in Texten anschlagen. */
function blankStrings(line) {
  return line.replace(/"(?:[^"\\]|\\.)*"/g, s => '"' + ' '.repeat(Math.max(0, s.length - 2)) + '"');
}

const RULES = [
  {
    id: 'ternaer',
    severity: 'error',
    test: l => /[^?]\?[^?:]{1,80}:/.test(l) && !/\?\?/.test(l),
    why: 'Enforce kennt keinen Ternaeroperator. Der Compiler meldet dabei haeufig '
       + 'die falsche Datei und Zeile - deshalb sucht man ihn besser hier als dort. '
       + 'Abhilfe: if/else.',
  },
  {
    id: 'GetGame',
    severity: 'error',
    test: l => /\bGetGame\s*\(\)/.test(l),
    why: 'Seit DayZ 1.29 ist GetGame() nicht mehr zu benutzen. Abhilfe: '
       + '"if (g_Game && g_Game...)" - mit Null-Pruefung, g_Game kann frueh null sein.',
  },
  {
    id: 'var-auto',
    severity: 'error',
    test: l => /\b(?:var|auto)\s+\w+\s*=/.test(l),
    why: 'Enforce kennt weder var noch auto. Typ ausschreiben.',
  },
  {
    id: 'null-operatoren',
    severity: 'error',
    test: l => /\?\?|\?\./.test(l),
    why: 'Enforce kennt weder ?. noch ??. Abhilfe: ausdrueckliche Null-Pruefung.',
  },
  {
    id: 'delete',
    severity: 'error',
    test: l => /\bdelete\s+[A-Za-z_]/.test(l),
    why: 'delete auf einem verwalteten Objekt ist in Enforce ein Absturzkandidat. '
       + 'Abhilfe: auf null setzen und den GC arbeiten lassen; fuer Weltobjekte '
       + 'g_Game.ObjectDelete().',
  },
  {
    id: 'modded-class-parent',
    severity: 'error',
    test: l => /\bmodded\s+class\s+\w+\s*:/.test(l),
    why: 'Eine modded class nennt nie eine Elternklasse - sie erweitert die '
       + 'vorhandene. Mit ":" bricht die Kette.',
  },
  {
    id: 'mehrfachdeklaration',
    severity: 'error',
    test: l => /^\s*(?:int|float|bool|string|vector)\s+\w+\s*,\s*\w+/.test(l),
    why: 'Eine Variable je Deklaration. "int a, b;" uebersetzt Enforce nicht.',
  },
  {
    id: 'ref-parameter',
    severity: 'error',
    test: l => /\([^)]*\bref\s+\w+\s*[,)]/.test(l) && !/^\s*(?:\/\/|\*)/.test(l),
    why: 'ref gehoert an Membervariablen und Typedefs, nicht an Parameter, '
       + 'Rueckgaben oder lokale Variablen.',
  },
];

function scriptFilesOf(dir) {
  return walk(dir, (_p, n) => n.toLowerCase().endsWith('.c'));
}

export default function run() {
  const f = new Findings('enforce');

  const roots = [ADDONS_DIR, ...COMP_DIRS].filter(exists);
  const files = roots.flatMap(scriptFilesOf);

  if (files.length === 0) {
    f.items.push({
      validator: 'enforce', severity: 'info', file: '', line: 0,
      summary: 'Keine Enforce-Skripte gefunden.',
    });
    return f;
  }

  let checked = 0;
  for (const file of files) {
    const raw = readText(file);
    if (raw === null) continue;
    checked++;
    const lines = stripComments(raw).split(/\r?\n/);
    lines.forEach((line, i) => {
      if (!line.trim()) return;
      const l = blankStrings(line);
      for (const r of RULES) {
        if (!r.test(l)) continue;
        const msg = `${r.id}: ${r.why}`;
        if (r.severity === 'error') f.error(file, i + 1, msg);
        else f.warn(file, i + 1, msg);
      }
    });
  }

  f.items.push({
    validator: 'enforce', severity: 'info', file: '', line: 0,
    summary: `Geprueft: ${checked} Enforce-Skripte gegen ${RULES.length} harte Regeln.`,
  });

  return f;
}
