#!/usr/bin/env node
// select - schaltet einzelne ChefZ-PBOs im Deployment scharf.
//
// Fuer die Eingrenzung per Modul: "ChefZ_Core allein startet, mit
// ChefZ_Cooking nicht" ist die schnellste Aussage ueber einen Startfehler, die
// es gibt. Was nicht gewaehlt ist, wandert nach Addons.beiseite und kommt mit
// dem naechsten Aufruf zurueck.
//
// ZWEI FALLEN, beide am 28.08.2026 erlebt:
//
//   1. Ein LEERER @ChefZ-Ordner bei weiterhin eingetragener Modliste bringt den
//      Server in CDPCreateServer zum Absturz - ein anderer Fehler als der
//      gesuchte. Deshalb ist eine leere Auswahl nicht erlaubt.
//   2. requiredAddons ist eine HARTE Abhaengigkeit. Wer ChefZ_Cooking waehlt
//      und ChefZ_Processing weglaesst, misst nicht ChefZ_Cooking, sondern ein
//      fehlendes Addon. Das Skript zieht die Abhaengigkeiten deshalb selbst
//      nach und sagt, was es ergaenzt hat.
//
// Aufruf:
//   node tools/chefz-pack/select.mjs ChefZ_Core ChefZ_Cooking
//   node tools/chefz-pack/select.mjs alle

import fs from 'node:fs';
import path from 'node:path';

const ADDONS = 'D:\\Agent\\deployments\\DME-Test\\@ChefZ\\Addons';
const PARK   = 'D:\\Agent\\deployments\\DME-Test\\@ChefZ\\Addons.beiseite';
const SRC    = path.resolve(path.dirname(new URL(import.meta.url).pathname.replace(/^\/([A-Za-z]:)/, '$1')), '..', '..');

function alleModule() {
  const out = [];
  for (const dir of [path.join(SRC, 'Psyerns_ChefZ_Core', 'Addons')]) {
    if (!fs.existsSync(dir)) continue;
    for (const e of fs.readdirSync(dir, { withFileTypes: true })) if (e.isDirectory()) out.push(e.name);
  }
  for (const e of fs.readdirSync(SRC, { withFileTypes: true })) {
    if (e.isDirectory() && /^Psyerns_ChefZ_.*_Comp$/.test(e.name)) out.push(e.name);
  }
  return out;
}

/** requiredAddons eines Moduls, soweit es ChefZ-Module nennt. */
function abhaengigkeiten(name) {
  const kandidaten = [
    path.join(SRC, 'Psyerns_ChefZ_Core', 'Addons', name, 'config.cpp'),
    path.join(SRC, name, 'config.cpp'),
  ];
  const cfg = kandidaten.find(p => fs.existsSync(p));
  if (!cfg) return [];
  const text = fs.readFileSync(cfg, 'utf8');
  const m = text.match(/requiredAddons\[\]\s*=\s*\{([^}]*)\}/);
  if (!m) return [];
  return [...m[1].matchAll(/"([^"]+)"/g)].map(x => x[1]).filter(x => x.startsWith('ChefZ_'));
}

const alle = alleModule();
let wahl = process.argv.slice(2);

if (wahl.length === 0) {
  console.log('Vorhandene Module:\n  ' + alle.join('\n  '));
  console.log('\nAufruf: node tools/chefz-pack/select.mjs <Modul> [<Modul> ...]   |   alle');
  process.exit(0);
}
if (wahl.length === 1 && wahl[0].toLowerCase() === 'alle') wahl = alle.slice();

// Abhaengigkeiten transitiv nachziehen.
const gewaehlt = new Set(wahl);
const ergaenzt = [];
let gewachsen = true;
while (gewachsen) {
  gewachsen = false;
  for (const m of [...gewaehlt]) {
    for (const dep of abhaengigkeiten(m)) {
      if (!alle.includes(dep) || gewaehlt.has(dep)) continue;
      gewaehlt.add(dep);
      ergaenzt.push(`${dep} (verlangt von ${m})`);
      gewachsen = true;
    }
  }
}

const unbekannt = [...gewaehlt].filter(m => !alle.includes(m));
if (unbekannt.length) {
  console.error('Unbekanntes Modul: ' + unbekannt.join(', '));
  process.exit(2);
}
if (gewaehlt.size === 0) {
  console.error('Leere Auswahl. Ein leerer @ChefZ-Ordner laesst den Server in CDPCreateServer abstuerzen.');
  process.exit(2);
}

fs.mkdirSync(PARK, { recursive: true });

// Erst alles zurueckholen, dann die Nichtgewaehlten beiseite legen.
for (const f of fs.readdirSync(PARK)) fs.renameSync(path.join(PARK, f), path.join(ADDONS, f));

let aktiv = 0;
let beiseite = 0;
for (const f of fs.readdirSync(ADDONS)) {
  if (!f.toLowerCase().endsWith('.pbo')) continue;
  const modul = f.replace(/\.pbo$/i, '');
  if (gewaehlt.has(modul)) { aktiv++; continue; }
  fs.renameSync(path.join(ADDONS, f), path.join(PARK, f));
  beiseite++;
}

for (const e of ergaenzt) console.log('  + ' + e);
console.log(`\nAktiv: ${aktiv} PBO(s) - ${[...gewaehlt].sort().join(', ')}`);
console.log(`Beiseite: ${beiseite}`);
