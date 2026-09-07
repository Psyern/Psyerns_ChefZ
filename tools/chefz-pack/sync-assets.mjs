// sync-assets.mjs - kopiert Lykos' Modelle und Texturen aus dem Lieferordner
// ChefZ/ in die beiden Asset-Addons, die der Packer zu PBOs macht.
//
// WARUM ZWEI ORTE: ChefZ/ChefZ_Items und ChefZ/ChefZ_Devices sind der Ordner,
// in dem die Lieferung ankommt - so, wie der Modellierer sie ablegt, mit
// seinem eigenen config.cpp und seinen Skripten, die NICHT gepackt werden
// (sie tragen einen eigenen Mini-Core, der mit ChefZ_Core kollidieren
// wuerde). Gepackt werden ChefZ_Core/Addons/ChefZ_Items und
// ChefZ_Devices: nur models/ und data/ plus $PREFIX$ und ein config.cpp ohne
// Klassen. Das Praefix ist "ChefZ\ChefZ_Items" bzw. "ChefZ\ChefZ_Devices",
// weil die Texturpfade so in den .p3d stehen.
//
// UNTERORDNER WERDEN MITGENOMMEN. Bis zum 03.09.2026 kopierte dieses Skript
// nur die Dateien der obersten Ebene und uebersprang jedes Verzeichnis. Das
// hat models/proxies/ verschluckt - fuenf Haken des Doerrgestells und den
// Teller-Proxy der Pfanne. Die Modelle verweisen mit
// "proxy:\ChefZ\ChefZ_Devices\models\proxies\hook_1" auf Dateien, die es im
// Addon dann nicht gab; der Packer bricht mit "Invalid P3D proxy path(s)" ab.
// Ein Proxy-Ziel ist eine gewoehnliche .p3d - sie muss mit ins PBO, sonst
// haengt am Gestell nichts und in der Pfanne liegt nichts.
//
// Aufruf:  node tools/chefz-pack/sync-assets.mjs
// Kopiert nur, was neuer ist oder fehlt; loescht nichts.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const PAIRS = [
  ['ChefZ/ChefZ_Items',   'ChefZ_Core/Addons/ChefZ_Items'],
  ['ChefZ/ChefZ_Devices', 'ChefZ_Core/Addons/ChefZ_Devices'],
  ['ChefZ/ChefZ_Food',    'ChefZ_Core/Addons/ChefZ_Food'],
  ['ChefZ/ChefZ_Plants',  'ChefZ_Core/Addons/ChefZ_Plants'],
];
// cultivation/models gehoert dazu, seit dem 06.09.2026 nachgetragen. Es fehlte,
// und der Preis war derselbe wie beim vergessenen models/proxies/ am 03.09.:
// ChefZ_CornPlant und ChefZ_WildCorn zeigen seit 46b0c4d auf
// "\ChefZ\ChefZ_Plants\cultivation\models\corn_plant.p3d" - eine Datei, die
// nie in der Packquelle ankam. Die Klassen trugen damit ein totes Modell, ohne
// dass irgendetwas es meldete: der Packer prueft Proxy-Ziele, aber kein
// Werkzeug prueft, ob ein model= ueberhaupt auf eine vorhandene Datei zeigt.
// corn_plant.p3d ist zudem selbst nur ein Proxy-Container - es haengt die
// sieben Wachstumsstufen aus cultivation/models/corn/ ein, die aus demselben
// Grund fehlten.
const SUBDIRS = ['models', 'data', 'cultivation/data', 'cultivation/models'];

let copied = 0;
let same = 0;

/** Kopiert src nach dst, Unterordner eingeschlossen. Loescht nie etwas. */
function copyTree(src, dst, label) {
  fs.mkdirSync(dst, { recursive: true });
  for (const e of fs.readdirSync(src, { withFileTypes: true })) {
    const from = path.join(src, e.name);
    const to = path.join(dst, e.name);
    if (e.isDirectory()) {
      copyTree(from, to, `${label}/${e.name}`);
      continue;
    }
    if (!e.isFile()) continue;
    if (fs.existsSync(to) && fs.readFileSync(from).equals(fs.readFileSync(to))) { same++; continue; }
    fs.copyFileSync(from, to);
    copied++;
    console.log(`  kopiert  ${label}/${e.name}`);
  }
}

for (const [srcRel, dstRel] of PAIRS) {
  for (const sub of SUBDIRS) {
    const src = path.join(ROOT, srcRel, sub);
    if (!fs.existsSync(src)) continue;
    copyTree(src, path.join(ROOT, dstRel, sub), `${srcRel}/${sub}`);
  }
}
console.log(`fertig: ${copied} kopiert, ${same} unveraendert`);
