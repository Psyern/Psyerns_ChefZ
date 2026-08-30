#!/usr/bin/env node
// pack - baut aus jedem ChefZ-Addon eine PBO und legt sie in einen Zielordner.
//
// Warum ein Skript und kein Handgriff: der Mod besteht aus fuenfzehn Paketen
// (zwoelf Core-Addons, davon zwei reine Asset-Pakete, drei Comp-Mods). Wer die von Hand packt, vergisst
// irgendwann eines, und ein vergessenes Paket faellt beim Serverstart nicht
// auf - die Klassen fehlen einfach, ohne Fehlermeldung.
//
// WEDER SIGNIEREN NOCH BINARISIEREN. Der Testserver laeuft mit
// verifySignatures=0; Binarisieren wuerde nur Zeit kosten und die Skripte
// unlesbar machen, ohne dem Compilertest etwas hinzuzufuegen. AddonBuilder
// bekommt deshalb "-packonly" - es kopiert, packt, fertig.
//
// PRAEFIX: jedes Addon traegt eine Datei "$PREFIX$" mit seinem Laufzeitnamen.
// Der MUSS der Ordnername sein, sonst findet DayZ die Skriptmodule nicht und
// schweigt dazu (siehe Kopf von ChefZ_Core/config.cpp). Das Skript liest den
// Praefix aus der Datei und uebergibt ihn ausdruecklich, statt sich auf
// AddonBuilders Selbstfindung zu verlassen - und prueft, dass er stimmt.
//
// Aufruf:
//   node tools/chefz-pack/pack.mjs [Zielordner]
// Vorgabe fuer den Zielordner ist der Testserver DME-Test.

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, '..', '..');

const DEFAULT_TARGET = 'D:\\Agent\\deployments\\DME-Test\\@ChefZ\\Addons';
const TARGET = process.argv[2] || DEFAULT_TARGET;

const ADDON_BUILDER = 'C:\\Program Files (x86)\\Steam\\steamapps\\common\\DayZ Tools'
  + '\\Bin\\AddonBuilder\\AddonBuilder.exe';

/** Alle Paketquellen: die Core-Addons plus die drei eigenstaendigen Comp-Mods. */
function sources() {
  const out = [];
  const coreAddons = path.join(ROOT, 'Psyerns_ChefZ_Core', 'Addons');
  if (fs.existsSync(coreAddons)) {
    for (const e of fs.readdirSync(coreAddons, { withFileTypes: true })) {
      if (e.isDirectory()) out.push(path.join(coreAddons, e.name));
    }
  }
  for (const e of fs.readdirSync(ROOT, { withFileTypes: true })) {
    if (!e.isDirectory()) continue;
    if (!/^Psyerns_ChefZ_.*_Comp$/.test(e.name)) continue;
    out.push(path.join(ROOT, e.name));
  }
  return out;
}

function readPrefix(dir) {
  const f = path.join(dir, '$PREFIX$');
  if (!fs.existsSync(f)) return null;
  return fs.readFileSync(f, 'utf8').replace(/^\uFEFF/, '').trim();
}

if (!fs.existsSync(ADDON_BUILDER)) {
  console.error(`AddonBuilder nicht gefunden: ${ADDON_BUILDER}`);
  process.exit(2);
}

fs.mkdirSync(TARGET, { recursive: true });
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'chefz-pack-'));

console.log(`Ziel:  ${TARGET}`);
console.log(`Temp:  ${temp}`);
console.log('');

let ok = 0;
const failed = [];

for (const src of sources()) {
  const name = path.basename(src);
  const prefix = readPrefix(src);

  if (prefix === null) {
    failed.push(`${name}: keine Datei "$PREFIX$" - ohne Praefix laedt DayZ die Skripte nicht`);
    continue;
  }
  // Asset-Pakete tragen den Laufzeitpfad "ChefZ\<Name>" als Praefix, weil
  // die Texturpfade so in den gelieferten .p3d stehen; alles andere muss der
  // Ordnername selbst sein.
  if (prefix !== name && prefix !== 'ChefZ\\' + name) {
    failed.push(`${name}: Praefix "${prefix}" weicht vom Ordnernamen ab - `
      + 'die Laufzeitpfade der Skriptmodule zeigen dann ins Leere');
    continue;
  }
  if (!fs.existsSync(path.join(src, 'config.cpp'))) {
    failed.push(`${name}: keine config.cpp`);
    continue;
  }

  const args = [src, TARGET, '-clear', '-packonly', `-prefix=${prefix}`, `-temp=${temp}`];
  const include = path.join(src, 'include.txt');
  if (fs.existsSync(include)) args.push(`-include=${include}`);

  const r = spawnSync(ADDON_BUILDER, args, { encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
  const pbo = path.join(TARGET, `${name}.pbo`);

  if (r.status !== 0 || !fs.existsSync(pbo)) {
    const log = ((r.stdout || '') + (r.stderr || ''))
      .split(/\r?\n/).filter(l => /ERROR|FAIL|Exception/i.test(l)).slice(0, 5).join('\n      ');
    failed.push(`${name}: AddonBuilder Exit ${r.status}\n      ${log || '(keine Fehlerzeile im Protokoll)'}`);
    continue;
  }

  const kb = Math.round(fs.statSync(pbo).size / 1024);
  console.log(`  ok  ${name}.pbo  (${kb} KB, Praefix ${prefix})`);
  ok++;
}

fs.rmSync(temp, { recursive: true, force: true });

console.log('');
if (failed.length > 0) {
  console.log(`FEHLGESCHLAGEN - ${ok} gepackt, ${failed.length} nicht:`);
  for (const f of failed) console.log(`  ! ${f}`);
  process.exit(1);
}
console.log(`BESTANDEN - ${ok} PBOs gepackt, ohne Signatur und ohne Binarisierung.`);
