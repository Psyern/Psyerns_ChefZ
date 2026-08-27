#!/usr/bin/env node
// Selbstpruefung der S19-Validatoren.
//
// 19 S19, "Fertig, wenn": "Ein absichtlich fehlerhaftes Wegwerf-Modul loest
// JEDE Regel mindestens einmal aus, Exit-Code 1; der saubere Projektstand
// liefert Exit-Code 0."
//
// Genau das macht diese Datei - und zwar ohne das Wegwerf-Modul ins Projekt zu
// legen: sie baut den fehlerhaften Baum in einem Temporaerverzeichnis, laesst
// index.mjs mit CHEFZ_VALIDATE_ROOT darauf los und prueft, dass jede einzelne
// Regel anschlaegt. Danach loescht sie ihn wieder.
//
// Warum das noetig ist: ein Pruefer, der nie etwas findet, ist von einem
// kaputten Pruefer nicht zu unterscheiden. Beide melden "bestanden".
//
//   node tools/chefz-validate/selftest.mjs
//
// Exit 0 = jede Regel hat gezuendet. Exit 1 = eine Regel ist blind.

import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const TOOL_DIR = path.dirname(fileURLToPath(import.meta.url));

// --- Das Wegwerf-Modul ------------------------------------------------------

const FILES = {
  // Ein Core, der alles falsch macht, was chefzcore und chefzlog verbieten.
  'Psyerns_ChefZ_Core/Addons/ChefZ_Core/$PREFIX$': 'ChefZ_Core',
  'Psyerns_ChefZ_Core/Addons/ChefZ_Core/config.cpp': `
class CfgPatches
{
    class ChefZ_Core
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data"};
    };
};
class CfgVehicles
{
    class ChefZ_CoreBringtEinItemMit
    {
        scope = 2;
    };
};
`,
  'Psyerns_ChefZ_Core/Addons/ChefZ_Core/Config/Core.json': JSON.stringify({
    kind: 'coreSettings',
    schemaVersion: 1,
    records: [{
      id: 'CORE',
      defaultExtraItems: 'verbieten',           // geschlossene Liste verletzt
      defaultExcludedStates: ['GIBTESNICHT'],   // unbekannter Zustand
    }],
  }, null, 2),
  'Psyerns_ChefZ_Core/Addons/ChefZ_Core/Scripts/1_Core/ChefZ_Schlecht.c': `
// Diese Datei verletzt absichtlich I3 und I4.
// Kommentar-Hook ohne Marker: spaeter vielleicht CF_Trace nutzen.
class ChefZ_Schlecht
{
    static const string ZUSTAND = "SMOKED";              // C3: Content-Wort
    static const string FREMD   = "TerjeSkills";         // C1: Fremdsystem
    static const string GEERBT  = "KAT_A";               // C2: ID eines Content-Moduls

    void MachWas(array<string> liste)
    {
        for (int i = 0; i < liste.Count(); i++)
            ChefZ_Log.Debug(ChefZ_LogChannel.CORE, "Eintrag " + liste.Get(i));
    }
}

enum ChefZ_EFoodState
{
    SMOKED,
    SALTED
}
`,

  // Ein Content-Modul, das alles falsch macht, was chefzsym, chefznut,
  // chefzstage und chefzproc verbieten.
  'Psyerns_ChefZ_Core/Addons/ChefZ_Schlecht/$PREFIX$': 'ChefZ_Schlecht',
  'Psyerns_ChefZ_Core/Addons/ChefZ_Schlecht/config.cpp': `
class CfgPatches
{
    class ChefZ_Schlecht
    {
        units[] = {"ChefZ_TestGericht", "ChefZ_TestZutat"};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data"};
    };
};
class CfgVehicles
{
    class Edible_Base;

    // Ergebnisklasse ohne Nutrition und mit scope 0 -> 01 V7, zweimal
    class ChefZ_TestGericht : Edible_Base
    {
        scope = 0;
        displayName = "Testgericht";
    };

    // Kochbare Zutat ohne FoodStageTransitions -> 01 V4
    class ChefZ_TestZutat : Edible_Base
    {
        scope = 2;
        class Food
        {
            class FoodStages
            {
                class Raw
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                };
            };
        };
    };
};
class CfgChefZStates
{
    class DRY
    {
        displayName = "#STR_CHEFZ_TEST";
    };
};
`,
  'Psyerns_ChefZ_Core/Addons/ChefZ_Schlecht/Config/Categories.json': JSON.stringify({
    kind: 'category', schemaVersion: 1,
    records: [{ id: 'KAT_A' }],
  }, null, 2),
  'Psyerns_ChefZ_Core/Addons/ChefZ_Schlecht/Config/Tools.json': JSON.stringify({
    kind: 'toolGroup', schemaVersion: 1,
    records: [{ id: 'TG_MESSER', classes: ['KitchenKnife'] }],
  }, null, 2),
  'Psyerns_ChefZ_Core/Addons/ChefZ_Schlecht/Config/Processes.json': JSON.stringify({
    kind: 'process', schemaVersion: 1,
    records: [{ id: 'PROZ_HAND', exec: 'HANDCRAFT', toolGroups: ['TG_MESSER'] }],
  }, null, 2),
  'Psyerns_ChefZ_Core/Addons/ChefZ_Schlecht/Config/Transforms.json': JSON.stringify({
    kind: 'transform', schemaVersion: 1,
    records: [{
      id: 'TR_ZUVIEL',
      process: 'PROZ_HAND',
      inputs: [
        { slotId: 'a', match: { cls: 'ChefZ_TestZutat' } },
        { slotId: 'b', match: { cls: 'ChefZ_TestZutat' } },
        { slotId: 'c', match: { cls: 'ChefZ_TestZutat' } },
      ],
      outputs: [{ cls: 'ChefZ_TestGericht' }],
    }],
  }, null, 2),
  'Psyerns_ChefZ_Core/Addons/ChefZ_Schlecht/Config/Recipes.json': JSON.stringify({
    kind: 'recipe', schemaVersion: 1,
    records: [{
      id: 'REZ_TEST',
      completion: 'IRGENDWANN',                    // geschlossene Liste verletzt
      doneStages: ['Gegrillt'],                    // keine Vanilla-Garstufe
      slots: [{
        slotId: 'haupt',
        match: { category: 'KAT_B' },              // Tippfehler: KAT_A existiert
        setStateAfter: 'DRYY',                     // Tippfehler: DRY existiert
      }, {
        // Diese Zutat liegt damit im Kochgeraet - und hat keine
        // FoodStageTransitions. Genau die Falle aus 01 V4.
        slotId: 'zutat',
        match: { cls: 'ChefZ_TestZutat' },
      }],
      outputs: [{ cls: 'ChefZ_TestGericht', portionClass: 'ChefZ_TestGericht' }],
    }],
  }, null, 2),
};

// --- Was zuenden MUSS -------------------------------------------------------

const EXPECT = [
  ['chefzsym', /Kategorie "KAT_B"/, 'unbekannte Kategorie in einem Selektor'],
  ['chefzsym', /Zustand "DRYY"/, 'unbekannter Zustand in setStateAfter'],
  ['chefzsym', /completion: "IRGENDWANN"/, 'Wert ausserhalb der geschlossenen Liste'],
  ['chefzsym', /doneStages\[0\]: "Gegrillt"/, 'keine Vanilla-Garstufe'],
  ['chefzsym', /defaultExtraItems: "verbieten"/, 'CoreSettings gegen die geschlossene Liste'],
  ['chefzcore', /TerjeSkills/, 'I4: Fremdsystemname im Code'],
  ['chefzcore', /steht in einem Kommentar des Core/, 'I4: Fremdsystemname im Kommentar (Warnung)'],
  ['chefzcore', /Content-Wort "smoked"/, 'I3: Content-Wort im Core'],
  ['chefzcore', /ist Content \(category "KAT_A"/, 'I3: ID eines Content-Moduls im Core'],
  ['chefzcore', /enum ChefZ_EFoodState/, 'I3: Aufzaehlung ueber eine Content-Dimension'],
  ['chefzcore', /Der Core deklariert 1 Klasse\(n\) in CfgVehicles/, 'I3: Item im Core'],
  ['chefznut', /weder "class Nutrition" noch "class Food"/, '01 V7: Ergebnis ohne Nutrition'],
  ['chefznut', /scope = 0/, '01 V7: scope 0'],
  ['chefzstage', /ist kochbar \(.*\), deklariert aber keine FoodStageTransitions/, '01 V4: kochbar ohne Uebergaenge'],
  ['chefzproc', /3 Eingaenge/, '01 V12: HANDCRAFT mit mehr als zwei Eingaengen'],
  ['chefzlog', /Ungewachter Log-Aufruf in einer Schleife/, '18 E2: Wache fehlt'],
];

// --- Ablauf -----------------------------------------------------------------

function build(root) {
  for (const [rel, content] of Object.entries(FILES)) {
    const full = path.join(root, rel);
    fs.mkdirSync(path.dirname(full), { recursive: true });
    fs.writeFileSync(full, content, 'utf8');
  }
}

const root = fs.mkdtempSync(path.join(os.tmpdir(), 'chefz-selftest-'));
let report;
let exitCode = 0;
try {
  build(root);
  let stdout = '';
  try {
    stdout = execFileSync(process.execPath, [path.join(TOOL_DIR, 'index.mjs'), '--json'], {
      env: { ...process.env, CHEFZ_VALIDATE_ROOT: root },
      encoding: 'utf8',
    });
    exitCode = 0;
  } catch (err) {
    stdout = err.stdout || '';
    exitCode = err.status;
  }
  report = JSON.parse(stdout);
} finally {
  fs.rmSync(root, { recursive: true, force: true });
}

const byCheck = new Map(report.checks.map(c => [c.name, c]));
const text = (name) => (byCheck.get(name)?.items || [])
  .filter(i => i.severity === 'error' || i.severity === 'warning')
  .map(i => i.summary).join('\n');

let failed = 0;
const B = '─'.repeat(72);
console.log(B);
console.log('S19 - Selbstpruefung der Validatoren am Wegwerf-Modul');
console.log(B);
for (const [check, re, what] of EXPECT) {
  const hit = re.test(text(check));
  if (!hit) failed++;
  console.log(`${hit ? 'zuendet ' : 'BLIND   '} ${check.padEnd(11)} ${what}`);
}

for (const c of report.checks) {
  if (c.toolFailure) { console.log(`\n[WERKZEUGFEHLER] ${c.name}: ${c.message}`); failed++; }
}

// --verbose zeigt jeden Befund des Wegwerf-Moduls. Wer eine Regel aendert,
// sieht damit sofort, was sie am kaputten Baum tatsaechlich sagt - und ob die
// Meldung einem Content-Autor weiterhilft oder nur recht hat.
if (process.argv.includes('--verbose')) {
  for (const c of report.checks) {
    if (!c.items || c.items.length === 0) continue;
    console.log(`\n--- ${c.name}`);
    for (const i of c.items) {
      console.log(`  ${i.severity[0].toUpperCase()} ${i.file || '(projektweit)'}${i.line ? ':' + i.line : ''}`);
      console.log(`     ${i.summary}`);
    }
  }
}

console.log(B);
console.log(`Wegwerf-Modul: Exit-Code ${exitCode} (erwartet 1), `
  + `${report.errors} Fehler, ${report.warnings} Warnungen.`);
if (exitCode !== 1) { console.log('FEHLER: das Wegwerf-Modul haette Exit-Code 1 liefern muessen.'); failed++; }
console.log(failed === 0
  ? 'ERGEBNIS: BESTANDEN - jede Regel hat mindestens einmal gezuendet.'
  : `ERGEBNIS: NICHT BESTANDEN - ${failed} Regel(n) blind.`);
console.log(B);

process.exit(failed === 0 ? 0 : 1);
