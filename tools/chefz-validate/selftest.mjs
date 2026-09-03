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
import { CHECKS } from './lib.mjs';

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

    // Vier harte Enforce-Verstoesse auf einmal -> enforce
    void MachEsFalsch()
    {
        int a, b;                                   // Mehrfachdeklaration
        string s = GetGame().GetPlayer().GetType(); // GetGame() seit 1.29 verboten
        int c = a > b ? a : b;                      // Ternaeroperator
        delete s;                                   // delete auf Verwaltetem
    }
}

// Basislos, wird aber per ref gehalten -> chefzmanaged
class ChefZ_SchlechtHalter
{
    int wert;
}

class ChefZ_SchlechtBesitzer
{
    ref array<ref ChefZ_SchlechtHalter> liste;
    ref ChefZ_SchlechtHalter einzeln;
}

// case-Marke aus einem Schiebeausdruck -> chefzswitch
class ChefZ_SchlechtSchalter
{
    static const int FLAGGE_A = 1 << 0;
    static const int FLAGGE_B = 1 << 1;

    static string Name(int wert)
    {
        switch (wert)
        {
            case FLAGGE_A: return "A";
            case FLAGGE_B: return "B";
        }
        return "?";
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

    // Elternklasse, die in DIESER config.cpp nirgends bekannt ist -> chefzbase
    class ChefZ_TestErbe : ChefZ_NirgendsDeklariert
    {
        scope = 2;
        displayName = "Erbe ohne Basis";
    };

    // Item-Klasse ohne ChefZ_-Praefix und ohne Vanilla-Entsprechung -> naming
    class chefz_falschBenannt : Edible_Base
    {
        scope = 2;
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

  // Rezept ohne Kennung - loest schema aus. Bewusst NICHT in einem
  // Config/Recipes/-Ordner, damit zugleich belegt ist, dass die Erkennung am
  // Dokumenttyp haengt und nicht am Pfad.
  'Psyerns_ChefZ_Core/Addons/ChefZ_Schlecht/Config/MehrRezepte.json': JSON.stringify({
    kind: 'recipe', schemaVersion: 1,
    records: [
      // ohne Kennung -> schema
      { outputs: [{ cls: 'ChefZ_TestGericht' }] },
      // Ergebnisklasse, die es nirgends gibt -> classrefs
      { id: 'REZ_GEISTERKLASSE', outputs: [{ cls: 'ChefZ_GibtEsNicht' }] },
    ],
  }, null, 2),

  // Zwei Slices definieren dieselbe Kategorie unterschiedlich, und eine
  // Haltbarkeitsregel zeigt auf einen Zustand, den niemand deklariert -
  // loest deltas aus.
  'Psyerns_ChefZ_Core/_deltas/schlecht_a.json': JSON.stringify({
    slice: 'schlecht_a',
    categories: [{ id: 'KAT_STREIT', parent: null, displayName: '#STR_A' }],
    preservation: [{ id: 'NIE_DEKLARIERT', scope: 'state', spoilageMultiplier: 0.5 }],
  }, null, 2),
  'Psyerns_ChefZ_Core/_deltas/schlecht_b.json': JSON.stringify({
    slice: 'schlecht_b',
    categories: [{ id: 'KAT_STREIT', parent: null, displayName: '#STR_B' }],
  }, null, 2),

  // Ein Asset-Paket, dessen Modell einen Proxy nennt, den niemand liefert -
  // loest proxies aus. Der Pruefer liest den Binaerstrom nach Klartext ab,
  // deshalb genuegt hier eine Textdatei mit der Endung .p3d: das ist die
  // Zeile, die in einer echten MLOD-Datei genauso dasteht.
  'Psyerns_ChefZ_Core/Addons/ChefZ_SchlechtAssets/$PREFIX$': 'ChefZ\\ChefZ_SchlechtAssets',
  'Psyerns_ChefZ_Core/Addons/ChefZ_SchlechtAssets/config.cpp': `
class CfgPatches
{
    class ChefZ_SchlechtAssets
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data"};
    };
};
`,
  'Psyerns_ChefZ_Core/Addons/ChefZ_SchlechtAssets/models/gestell.p3d':
    'MLOD proxy:\\ChefZ\\ChefZ_SchlechtAssets\\models\\proxies\\haken_1.001',
};

// --- Was zuenden MUSS -------------------------------------------------------

const EXPECT = [
  ['chefzbase', /ChefZ_NirgendsDeklariert/, 'Elternklasse in der eigenen config.cpp unbekannt'],
  ['chefzmanaged', /ChefZ_SchlechtHalter/, 'per ref gehaltene Klasse ohne Managed'],
  ['chefzswitch', /FLAGGE_A|FLAGGE_B/, 'case-Marke aus einem Schiebeausdruck'],
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
  ['enforce', /mehrfachdeklaration/, 'zwei Variablen in einer Deklaration'],
  ['enforce', /GetGame/, 'GetGame() statt g_Game'],
  ['enforce', /ternaer/, 'Ternaeroperator, den Enforce nicht kennt'],
  ['enforce', /delete/, 'delete auf einem verwalteten Objekt'],
  ['classrefs', /ChefZ_GibtEsNicht/, 'Rezept zeigt auf eine nicht existierende ChefZ-Klasse'],
  ['naming', /chefz_falschBenannt/, 'Item-Klasse verletzt die Namenskonvention'],
  ['schema', /hat keine Kennung/, 'Rezept ohne id - und Erkennung am Dokumenttyp, nicht am Pfad'],
  ['deltas', /ID-Kollision in "categories"/, 'zwei Slices definieren dieselbe Kategorie unterschiedlich'],
  ['deltas', /Haltbarkeitsregel "NIE_DEKLARIERT"/, 'Preservation zeigt auf einen undeklarierten Zustand'],
  ['proxies', /haken_1/, 'Modell nennt einen Proxy, den niemand liefert'],
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
  + `${report.errors} Fehler, ${report.warnungen ?? report.warnings} Warnungen.`);
if (exitCode !== 1) { console.log('FEHLER: das Wegwerf-Modul haette Exit-Code 1 liefern muessen.'); failed++; }

// --- Wie weit reicht dieser Selbsttest ueberhaupt? -------------------------
//
// Die Erfolgsmeldung hiess frueher "jede Regel hat gezuendet". Das war zu gross
// gesprochen: das Wegwerf-Modul loest nur Regeln aus, die auf seine Bauart
// passen. Ein Selbsttest, der seine eigene Reichweite verschweigt, erzeugt genau
// das falsche Zutrauen, gegen das die Pruefer gebaut wurden.
const ALL_CHECKS = CHECKS;
// Nur ECHTE Befunde zaehlen. Mehrere Pruefer geben immer eine info-Zeile aus
// ("Geprueft: N Dateien ..."); wuerde die mitzaehlen, galte ein Pruefer als
// ausgeloest, der nichts gefunden hat - und die Abdeckungszahl waere wertlos.
const exercised = new Set(
  report.checks
    .filter(c => (c.items || []).some(i => i.severity === 'error' || i.severity === 'warning'))
    .map(c => c.name));
const untested = ALL_CHECKS.filter(n => !exercised.has(n));

console.log(`\nAbdeckung: ${exercised.size} von ${ALL_CHECKS.length} Pruefern werden `
  + `vom Wegwerf-Modul ausgeloest.`);
if (untested.length) {
  console.log(`Nicht ausgeloest: ${untested.join(', ')}`);
  console.log('Diese Pruefer sind damit NICHT selbstgetestet - sie koennten still');
  console.log('nichts melden, ohne dass es hier auffiele. Das Wegwerf-Modul braucht');
  console.log('je einen passenden Fehlerfall, um sie abzudecken.');
}

console.log(failed === 0
  ? `\nERGEBNIS: BESTANDEN - jede der ${ALL_CHECKS.length - untested.length} abgedeckten `
    + `Pruefergruppen hat gezuendet.`
  : `\nERGEBNIS: NICHT BESTANDEN - ${failed} Regel(n) blind.`);
console.log(B);

process.exit(failed === 0 ? 0 : 1);
