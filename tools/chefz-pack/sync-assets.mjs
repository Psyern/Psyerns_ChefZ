// sync-assets.mjs - kopiert Lykos' Modelle und Texturen aus dem Lieferordner
// ChefZ/ in die beiden Asset-Addons, die der Packer zu PBOs macht.
//
// WARUM ZWEI ORTE: ChefZ/ChefZ_Items und ChefZ/ChefZ_Devices sind der Ordner,
// in dem die Lieferung ankommt - so, wie der Modellierer sie ablegt, mit
// seinem eigenen config.cpp und seinen Skripten, die NICHT gepackt werden
// (sie tragen einen eigenen Mini-Core, der mit ChefZ_Core kollidieren
// wuerde). Gepackt werden Psyerns_ChefZ_Core/Addons/ChefZ_Items und
// ChefZ_Devices: nur models/ und data/ plus $PREFIX$ und ein config.cpp ohne
// Klassen. Das Praefix ist "ChefZ\ChefZ_Items" bzw. "ChefZ\ChefZ_Devices",
// weil die Texturpfade so in den .p3d stehen.
//
// Aufruf:  node tools/chefz-pack/sync-assets.mjs
// Kopiert nur, was neuer ist oder fehlt; loescht nichts.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const PAIRS = [
  ['ChefZ/ChefZ_Items',   'Psyerns_ChefZ_Core/Addons/ChefZ_Items'],
  ['ChefZ/ChefZ_Devices', 'Psyerns_ChefZ_Core/Addons/ChefZ_Devices'],
  ['ChefZ/ChefZ_Food',    'Psyerns_ChefZ_Core/Addons/ChefZ_Food'],
  ['ChefZ/ChefZ_Plants',  'Psyerns_ChefZ_Core/Addons/ChefZ_Plants'],
];
const SUBDIRS = ['models', 'data', 'cultivation/data'];

let copied = 0;
let same = 0;
for (const [srcRel, dstRel] of PAIRS) {
  for (const sub of SUBDIRS) {
    const src = path.join(ROOT, srcRel, sub);
    const dst = path.join(ROOT, dstRel, sub);
    if (!fs.existsSync(src)) continue;
    fs.mkdirSync(dst, { recursive: true });
    for (const name of fs.readdirSync(src)) {
      const from = path.join(src, name);
      const to = path.join(dst, name);
      if (!fs.statSync(from).isFile()) continue;
      if (fs.existsSync(to) && fs.readFileSync(from).equals(fs.readFileSync(to))) { same++; continue; }
      fs.copyFileSync(from, to);
      copied++;
      console.log(`  kopiert  ${srcRel}/${sub}/${name}  ->  ${dstRel}/${sub}/`);
    }
  }
}
console.log(`fertig: ${copied} kopiert, ${same} unveraendert`);
