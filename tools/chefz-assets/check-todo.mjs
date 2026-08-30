// check-todo - haelt ChefZ_3D_Asset_ToDo.md gegen den Code.
//
// Die Liste behauptet von sich, aus dem Code erzeugt zu sein. Am 31.08.2026
// stimmte das nicht mehr: vier Klassen fehlten, achtunddreissig Proxy-Angaben
// nannten ein Modell, das die Klasse laengst nicht mehr traegt, und die
// Kollisionsliste zaehlte achtundzwanzig Bulk-Gerichte, die es seit zwei Tagen
// nicht mehr gab. Niemand hatte etwas falsch gemacht - es gab nur kein
// Werkzeug, das den Unterschied zeigt.
//
// Dieses hier zeigt ihn:
//
//   node tools/chefz-assets/check-todo.mjs
//
// Gemeldet wird, was auseinanderlaeuft - Klassen ohne Zeile, Zeilen ohne
// Klasse, Haken ohne Modell, Proxy-Angaben ohne Deckung. Exit-Code ist immer
// 0: das hier ist ein Spiegel, kein Waechter. Ein Haken, der "Rohmodell beim
// Modellierer vorhanden" heisst, kann dem Code voraus sein, und genau das ist
// bei ChefZ_DryingRack der Fall (Textur geliefert, Mesh fehlt) - der eine
// erwartete Eintrag unter HAKEN FALSCH.
//
// Der Parser ist der der Validatoren, damit beide dasselbe sehen.

// Gleicht ChefZ_3D_Asset_ToDo.md gegen den tatsaechlichen Stand der
// config.cpp ab - mit dem Parser, den auch die Validatoren benutzen.
import { readFileSync } from 'node:fs';
import { configItemIndex, configChain, resolveProp, moduleOf } from '../chefz-validate/chefzdata.mjs';

import path from 'node:path';
import { fileURLToPath } from 'node:url';
const HIER = path.dirname(fileURLToPath(import.meta.url));
const DOC = path.resolve(HIER, '..', '..', 'Psyerns_ChefZ_Docs', 'ChefZ_3D_Asset_ToDo.md');

// --- 1. Der Code: jede ChefZ-Klasse mit scope != 0 und ihr Modell ---------
const index = configItemIndex();
const code = new Map();   // Klasse -> { model, eigen, geerbtVon, modul }
for (const [name, entry] of index) {
  if (!name.startsWith('ChefZ_')) continue;
  const scope = resolveProp(name, 'scope');
  const scopeVal = scope ? Number(scope.value) : null;
  if (scopeVal === 0) continue;            // Basisklassen ohne Item-Existenz
  if (scopeVal === null) continue;         // kein scope = kein Item (Unterknoten)
  const mdl = resolveProp(name, 'model');
  const model = mdl ? String(mdl.value).replace(/^"|"$/g, '') : '';
  const eigenerKnoten = mdl && mdl.owner.name === name;
  code.set(name, {
    model,
    eigen: /\\?ChefZ\\/i.test(model),      // Pfad in einem ChefZ-Asset-PBO
    modelOwner: mdl ? mdl.owner.name : null,
    eigenerKnoten,
    modul: moduleOf(entry.file) || '?',
  });
}

// --- 2. Die Liste: jede Tabellenzeile --------------------------------------
const doc = readFileSync(DOC, 'utf8');
const zeilen = doc.split(/\r?\n/);
const liste = new Map();   // Klasse -> { hak, proxy, zeile, abschnitt }
let abschnitt = '';
zeilen.forEach((l, i) => {
  const h = l.match(/^##+\s+(.*)$/);
  if (h) { abschnitt = h[1].replace(/\*\*/g, ''); return; }
  const m = l.match(/^\|\s*\[([ x])\]\s*\|\s*`(ChefZ_\w+)`\s*\|\s*([^|]*)\|/);
  if (!m) return;
  liste.set(m[2], { hak: m[1] === 'x', proxy: m[3].trim().replace(/`/g, ''), zeile: i + 1, abschnitt });
});

// --- 3. Abgleich ------------------------------------------------------------
const fehlt = [];      // im Code, nicht in der Liste
const veraltet = [];   // in der Liste, nicht im Code
const hakFalsch = [];  // Haken passt nicht zum Modell
const proxyFalsch = [];// Proxy-Angabe passt nicht zum model=

const kurz = m => m.replace(/^\\/, '').split('\\').pop() || m;

for (const [cls, c] of code) {
  if (!liste.has(cls)) { fehlt.push([cls, c.modul, kurz(c.model) || '(kein model)', c.eigen]); continue; }
  const l = liste.get(cls);
  if (l.hak !== c.eigen) hakFalsch.push([cls, l.zeile, l.hak ? '[x]' : '[ ]', c.eigen ? 'eigenes Modell' : 'Vanilla-Proxy', kurz(c.model)]);
  // Proxy-Spalte gegen den echten Dateinamen
  const p = l.proxy.toLowerCase();
  const echt = kurz(c.model).toLowerCase();
  const erbt = /erbt von/.test(p);
  if (!erbt && echt && !p.includes(echt.replace('.p3d', '')) && !c.eigen) {
    proxyFalsch.push([cls, l.zeile, l.proxy, kurz(c.model)]);
  }
  if (!erbt && c.eigen && !/chefz/i.test(l.proxy)) {
    // abgehakt, aber die Proxy-Spalte nennt noch das Vanilla-Modell
    if (!hakFalsch.some(h => h[0] === cls)) proxyFalsch.push([cls, l.zeile, l.proxy, kurz(c.model)]);
  }
}
for (const [cls, l] of liste) if (!code.has(cls)) veraltet.push([cls, l.zeile, l.abschnitt]);

// --- 4. Ausgabe -------------------------------------------------------------
const kopf = doc.match(/\*\*Stand:\*\* ([\d-]+).*?\*\*Quelle:\*\* (.*)/);
console.log('KOPF DER DATEI');
console.log('  Stand:  ' + (kopf ? kopf[1] : '?') + '        ' + '(heute: ' + new Date().toLocaleDateString('sv-SE') + ')');
console.log('  Quelle: ' + (kopf ? kopf[2].replace(/`/g, '') : '?'));
console.log('  Addons im Code: ' + new Set([...code.values()].map(c => c.modul)).size);
console.log('');
console.log('ZAHLEN');
console.log('  Klassen im Code (scope != 0): ' + code.size);
console.log('  Zeilen in der Liste:          ' + liste.size);
console.log('  davon abgehakt [x]:           ' + [...liste.values()].filter(l => l.hak).length);
console.log('  mit eigenem Modell im Code:   ' + [...code.values()].filter(c => c.eigen).length);
console.log('');

const block = (titel, arr, fmt) => {
  console.log(titel + ': ' + arr.length);
  for (const a of arr.sort()) console.log('  ' + fmt(a));
  console.log('');
};
block('FEHLT IN DER LISTE (im Code, nicht dokumentiert)', fehlt,
  a => a[0].padEnd(32) + a[1].padEnd(20) + a[2] + (a[3] ? '  <- hat eigenes Modell' : ''));
block('VERALTET (in der Liste, nicht mehr im Code)', veraltet,
  a => a[0].padEnd(32) + 'Zeile ' + String(a[1]).padEnd(6) + a[2]);
block('HAKEN FALSCH', hakFalsch,
  a => a[0].padEnd(32) + 'Zeile ' + String(a[1]).padEnd(6) + a[2] + ' aber ' + a[3].padEnd(16) + a[4]);
block('PROXY-SPALTE VERALTET', proxyFalsch,
  a => a[0].padEnd(32) + 'Zeile ' + String(a[1]).padEnd(6) + 'sagt "' + a[2] + '", ist ' + a[3]);
