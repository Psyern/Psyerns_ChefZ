#!/usr/bin/env node
// ChefZ-Validator-Runner.
//
//   node tools/chefz-validate/index.mjs            lesbarer Bericht
//   node tools/chefz-validate/index.mjs --json     JSON-Bericht fuer Agenten
//   node tools/chefz-validate/index.mjs --only=schema,deltas
//
// Exit-Code 0 = keine Fehler. 1 = Fehler gefunden. 2 = Validator selbst kaputt.

import { ROOT } from './lib.mjs';

// Reihenfolge = Lesereihenfolge im Bericht: erst die Form der Dateien, dann
// die Bedeutung ihres Inhalts, zuletzt die Regeln des Core selbst.
const CHECKS = [
  'schema', 'configcpp', 'classrefs', 'naming', 'stringtable', 'deltas',
  // S19 (19 §3). chefzsym und chefzcore sind AUFLAGEN aus OF-11, keine Zugaben:
  // ohne sie ist der datengetriebene Entwurf schlechter als ein enum-basierter.
  'chefzsym', 'chefzcore', 'chefznut', 'chefzstage', 'chefzproc', 'chefzlog',
];

const args = process.argv.slice(2);
const asJson = args.includes('--json');
const onlyArg = args.find(a => a.startsWith('--only='));
const only = onlyArg ? onlyArg.slice('--only='.length).split(',').map(s => s.trim()) : CHECKS;

const report = { root: ROOT, checks: [], errors: 0, warnings: 0, toolFailures: 0 };

for (const name of CHECKS) {
  if (!only.includes(name)) continue;
  let findings;
  try {
    const mod = await import(`./${name}.mjs`);
    findings = mod.default();
  } catch (err) {
    report.toolFailures++;
    report.checks.push({
      name, ok: false, toolFailure: true,
      message: `Validator "${name}" ist selbst fehlgeschlagen: ${err.message}`,
      stack: err.stack, items: [],
    });
    continue;
  }
  const items = findings.items;
  const errors = items.filter(i => i.severity === 'error').length;
  const warnings = items.filter(i => i.severity === 'warning').length;
  report.errors += errors;
  report.warnings += warnings;
  report.checks.push({ name, ok: errors === 0, errors, warnings, items });
}

report.ok = report.errors === 0 && report.toolFailures === 0;

if (asJson) {
  process.stdout.write(JSON.stringify(report, null, 2) + '\n');
} else {
  const B = '─'.repeat(72);
  console.log(B);
  console.log('ChefZ statische Validierung');
  console.log(B);
  for (const c of report.checks) {
    if (c.toolFailure) {
      console.log(`\n[WERKZEUGFEHLER] ${c.name}`);
      console.log('  ' + c.message);
      continue;
    }
    const badge = c.errors > 0 ? 'FEHLER' : (c.warnings > 0 ? 'hinweise' : 'ok');
    console.log(`\n[${badge}] ${c.name}  (${c.errors} Fehler, ${c.warnings} Warnungen)`);
    for (const i of c.items) {
      if (i.severity === 'info') continue;
      const mark = i.severity === 'error' ? 'E' : 'W';
      const loc = i.file ? `${i.file}${i.line ? ':' + i.line : ''}` : '(projektweit)';
      console.log(`  ${mark}  ${loc}`);
      console.log(`     ${i.summary}`);
    }
  }
  console.log('\n' + B);
  if (report.toolFailures > 0) {
    console.log(`ERGEBNIS: ${report.toolFailures} Validator(en) selbst fehlgeschlagen - Ergebnis unbrauchbar.`);
  } else if (report.errors > 0) {
    console.log(`ERGEBNIS: NICHT BESTANDEN - ${report.errors} Fehler, ${report.warnings} Warnungen.`);
  } else {
    console.log(`ERGEBNIS: BESTANDEN - 0 Fehler, ${report.warnings} Warnungen.`);
  }
  console.log(B);
}

process.exit(report.toolFailures > 0 ? 2 : (report.errors > 0 ? 1 : 0));
