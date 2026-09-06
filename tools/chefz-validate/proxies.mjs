// Proxy-Pfade in den .p3d der Asset-Pakete.
//
// WARUM DIESE PRUEFUNG: ein MLOD-Modell traegt seine Proxy-Verweise als
// Klartext im Binaerstrom - "proxy:\ChefZ\ChefZ_Devices\models\proxies\hook_1.001".
// Ein Proxy-Ziel ist eine gewoehnliche .p3d, die mit ins PBO muss. Fehlt sie,
// bricht der Packer ab ("Invalid P3D proxy path(s) found in ChefZ_Devices: 5")
// - und wo er nicht abbricht, haengt am Doerrgestell spaeter nichts und in der
// Pfanne liegt nichts, ohne dass ein Log etwas dazu sagt.
//
// Genau das ist am 03.09.2026 passiert: sync-assets.mjs kopierte nur die
// oberste Ebene von models/ und liess models/proxies/ liegen. Die Modelle
// waren da, ihre Haken nicht. Diese Pruefung ist die Nachhut dazu.
//
// Aufgeloest wird ueber die $PREFIX$-Dateien: der Proxy-Pfad beginnt mit dem
// Laufzeitpraefix eines Addons ("ChefZ\ChefZ_Food"), der Rest ist der Pfad
// innerhalb dieses Addons. Verweise ueber Paketgrenzen hinweg sind erlaubt -
// zur Laufzeit loest DayZ Proxies ueber alle geladenen PBOs auf.

import fs from 'node:fs';
import path from 'node:path';
import { Findings, ADDONS_DIR, ROOT, addonDirs, walk, exists, rel } from './lib.mjs';

const BS = String.fromCharCode(92);
const norm = s => s.toLowerCase().split(BS).join('/');

/** Praefix -> Addon-Verzeichnis, laengste Praefixe zuerst. */
function prefixMap() {
  const out = [];
  for (const dir of addonDirs()) {
    const pf = path.join(dir, '$PREFIX$');
    if (!exists(pf)) continue;
    const raw = fs.readFileSync(pf, 'utf8').replace(/^\uFEFF/, '').trim();
    if (!raw) continue;
    out.push({ prefix: norm(raw), dir, name: path.basename(dir) });
  }
  return out.sort((a, b) => b.prefix.length - a.prefix.length);
}

/** Alle Proxy-Verweise eines MLOD-Modells, entdoppelt. */
function proxyRefs(file) {
  const txt = fs.readFileSync(file).toString('latin1');
  return [...new Set(txt.match(/proxy:[ -~]{2,}/gi) || [])];
}

/** "proxy:\ChefZ\ChefZ_Food\models\leg_pork.001" -> "chefz/chefz_food/models/leg_pork.p3d" */
function targetPath(ref) {
  let p = norm(ref.slice('proxy:'.length));
  while (p.startsWith('/')) p = p.slice(1);
  return p.replace(/\.\d+$/, '') + '.p3d';
}

export default function run() {
  const f = new Findings('proxies');
  const prefixes = prefixMap();

  for (const addon of addonDirs()) {
    const models = path.join(addon, 'models');
    if (!exists(models)) continue;

    for (const p3d of walk(models, (_, name) => name.toLowerCase().endsWith('.p3d'))) {
      for (const ref of proxyRefs(p3d)) {
        const want = targetPath(ref);
        const owner = prefixes.find(p => want.startsWith(p.prefix + '/'));

        if (!owner) {
          f.error(p3d, null,
            `Proxy "${ref}" zeigt auf ein Praefix, das kein ChefZ-Addon traegt - `
            + 'zur Laufzeit findet die Engine dieses Modell nicht',
            { proxy: ref });
          continue;
        }

        const inner = want.slice(owner.prefix.length + 1);
        if (exists(path.join(owner.dir, inner))) continue;

        // Liegt das Ziel in der Lieferung, aber nicht im Addon, ist nur der
        // Abgleich liegengeblieben - dann ist der Hinweis genau ein Befehl.
        const delivered = path.join(ROOT, 'ChefZ', owner.name, inner);
        const hint = exists(delivered)
          ? 'Die Datei liegt in der Lieferung: node tools/chefz-pack/sync-assets.mjs'
          : `Die Datei fehlt auch in der Lieferung ChefZ/${owner.name}/${inner} - beim Modellierer nachfordern`;

        f.error(p3d, null,
          `Proxy-Ziel fehlt: "${ref}" erwartet ${rel(path.join(owner.dir, inner))}. ${hint}`,
          { proxy: ref, expected: rel(path.join(owner.dir, inner)) });
      }
    }
  }

  return f;
}
