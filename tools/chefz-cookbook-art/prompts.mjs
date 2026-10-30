//==============================================================================
// prompts.mjs - aus den Rezepten des Mods die Bildauftraege fuer das Kochbuch.
//
//   node tools/chefz-cookbook-art/prompts.mjs
//
// Ausgabe: ChefZ_Docs/Kochbuch/prompts.json - ein Eintrag je GERICHT
// (outputs[0].cls), nicht je Rezept. Gruppen- und Bruehe-Varianten zeigen
// dasselbe Gericht auf demselben Weg; eine zweite Seite dafuer waere nur ein
// zweites Bild derselben Sache. Die Varianten stehen unter recipeIds.
//
// --- Warum die Bilder KEINEN Text tragen -----------------------------------
// Titel, Mengen, Garzeit und Temperatur setzt die Ingame-UI aus Stringtable und
// Registry darueber. Ein Bild mit Schrift waere einsprachig, und jede
// Balanceaenderung (100 g statt 80 g Mehl) machte es still falsch. Erlaubt sind
// nur die Ziffern 1-4 der Schrittfelder - die sind in jeder Sprache gleich.
//
// --- Warum die vier Schritte immer dieselben sind --------------------------
// ChefZ kennt kein Schneiden und kein Wenden. Ein Rezept ist: die Zutaten liegen
// gemeinsam im Geraet, das Geraet steht auf dem Feuer, nach Zeit oder Garstufe
// ist das Gericht fertig. Der Pilot vom 26.10.2026 zeigte "Schneiden" und "In
// Mehl wenden" - schoen gezeichnet und im Spiel falsch. Darum folgen die Felder
// der Mechanik, nicht dem Kochbuchklischee.
//
// Datenquelle ist buildModel() des Recipecards-Werkzeugs, und damit der Parser
// der Validatoren. Was der Validator nicht als Rezept kennt, bekommt kein Bild.
//==============================================================================

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { buildModel } from '../chefz-recipecards/model.mjs';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const REPO = path.resolve(HERE, '..', '..');
const OUT = path.join(REPO, 'ChefZ_Docs', 'Kochbuch', 'prompts.json');

// --- Stilblock: fuer jede Seite woertlich gleich ---------------------------
// Wer hier etwas aendert, aendert den Look ALLER Seiten - und muss alle neu
// erzeugen, sonst sieht das Buch aus wie aus zwei Haenden.
const STYLE =
  'A single page from a worn survivor\'s field cookbook in the style of the video game DayZ ' +
  '(post-Soviet Eastern European countryside of Chernarus, gritty, muted palette of olive, rust, khaki and charcoal). ' +
  'Aged, water-stained off-white paper with faint military notebook grid lines, coffee rings and soft creases. ' +
  'Hand-inked illustrations with a light watercolor wash, as if a soldier sketched them by firelight. ' +
  'The page is divided into four equal panels in a 2x2 grid with thin inked borders. ' +
  'Each panel has a large stamped number in a dark circle in its top-left corner: 1, 2, 3, 4.';

const RULES =
  'ABSOLUTELY NO words, NO letters, NO labels, NO captions and NO writing in any alphabet anywhere on the page - ' +
  'the only characters allowed are the four panel digits 1, 2, 3 and 4. ' +
  'No hands, no people. Flat front view of the page filling the whole frame, no table around it, no modern objects, no logos.';

// --- Geraete und Behaelter, wie sie in Chernarus herumliegen ---------------
const DEVICE = {
  FryingPan:  'a black cast-iron frying pan',
  Pot:        'a dented aluminium cooking pot',
  Cauldron:   'a large blackened iron cauldron',
  OvenIndoor: 'an old soot-stained brick oven',
};

const CONTAINER = {
  ChefZ_EmptyPlate: 'on a chipped white enamel plate',
  ChefZ_EmptyBowl:  'in a chipped enamel bowl',
};

// --- Bildregie, wo die Daten fuers Auge nicht reichen ----------------------
// Kein Rezept, nur Bildsprache: ein Brot ohne Rueckgabebehaelter liegt nicht in
// einer Schuessel, und "Tactical Bacon Can Opened" zeichnet das Modell als
// Etikett. Wer ein Feld hier ueberschreibt, ersetzt nur dessen Beschreibung.
const ART = {
  ChefZ_Bread:     { ingredients: 'a ball of bread dough', inside: 'the dough inside', served: 'a round crusty loaf resting on the weathered wooden plank' },
  ChefZ_Flatbread: { ingredients: 'a ball of bread dough', inside: 'the dough pressed flat inside', served: 'a golden blistered flatbread resting on the weathered wooden plank' },
};
const WORDS = { 'Tactical Bacon Can Opened': 'an opened can of bacon' };

function deviceOf(obj) {
  const ctx = (obj.contexts || [])[0] || {};
  const cls = (ctx.deviceClasses || [])[0];
  return { cls, text: DEVICE[cls] || 'a battered cooking pot', liquid: !!ctx.requiresLiquid, methods: ctx.methods || [] };
}

/** Die Zutatenzeile fuer Feld 1: Pflichtzutaten voll, optionale als Beiwerk. */
function ingredientText(cells) {
  const need = new Map(), opt = new Set();
  for (const c of cells) {
    // Bei OR steht das erste Glied fuer alle; die Ausschluesse eines EXCEPT
    // ("Dairy −Butter −Cream") malt kein Bild - sie fallen hier weg.
    const raw = c.what.split(' / ')[0].split(' −')[0];
    const what = WORDS[raw] || raw;
    if (c.optional) { opt.add(what); continue; }
    need.set(what, (need.get(what) || 0) + 1);
  }
  const req = [...need].map(([w, n]) => (n > 1 ? `${n} ${w}` : w)).join(', ');
  const o = [...opt].join(', ');
  return { req, opt: o };
}

function promptFor(rec, obj, desc) {
  const dev = deviceOf(obj);
  const ing = ingredientText(rec.cells);
  const art = { ...(ART[rec.result.cls] || {}) };
  if (art.ingredients) { ing.req = art.ingredients; ing.opt = ''; }

  // Eine einzige Zutat (die sechs Wuerste): ohne diesen Satz malt das Modell
  // Zwiebel und Knoblauch dazu und schneidet das Ergebnis klein - der erste
  // Lauf am 26.10.2026 hat es bei allen sechs getan.
  const single = rec.cells.length === 1 && !art.ingredients;
  if (single) {
    ing.req += ' - ONLY this one single item, nothing else on the plank: no onions, no garlic, no herbs, no spices';
    art.inside = art.inside || 'that one item alone, whole, inside';
    art.served = art.served || `the same item whole and browned, not cut, still in ${dev.text}, nothing else in the pan`;
  }
  const served = art.served || CONTAINER[rec.container] || (dev.cls === 'FryingPan' ? 'still in the frying pan' : 'in a small enamel bowl');
  // Nur die Pfanne brutzelt. Ein Topf ohne Wasser (Bruehe, Sauce) koechelt.
  const heat = dev.liquid
    ? 'bubbling and steaming with water'
    : (dev.cls === 'OvenIndoor' ? 'inside the glowing oven' : (dev.cls === 'FryingPan' ? 'sizzling'
      : (dev.methods.includes('BOILING') ? 'simmering and steaming' : 'baking dry with the lid on, no water')));

  return [
    STYLE,
    `Panel 1: the raw ingredients laid out side by side on a weathered wooden plank: ${ing.req}` +
      (ing.opt ? `; next to them, drawn smaller and fainter with a dotted outline to show they are optional extras: ${ing.opt}.` : '.'),
    `Panel 2: ${art.inside || 'all of these ingredients together inside'} ${dev.text}` + (dev.liquid ? ', covered with water.' : '.'),
    `Panel 3: ${dev.text} ${dev.cls === 'OvenIndoor' ? 'with the food inside, the oven fire burning' : 'standing over a small campfire ringed with stones and sticks'}, ${heat}; ` +
      'a small hand-drawn pocket watch floats beside it to show that it needs time.',
    `Panel 4: the finished dish "${rec.name}"${desc && !art.served ? ` (${desc})` : ''}, ${served}, looking hearty and appetising, gentle steam rising.`,
    RULES,
  ].join('\n\n');
}

// --- Lauf --------------------------------------------------------------------
const chefzdata = await import('../chefz-validate/chefzdata.mjs');
const { recipes, problems } = buildModel(chefzdata, { sort: 'id' });
const objById = new Map(chefzdata.allRecords().filter(r => r.kind === 'recipe').map(r => [r.id, r.obj]));
const strings = loadEnglish();

const pages = new Map();
for (const rec of recipes) {
  const key = rec.result.cls;
  if (pages.has(key)) { pages.get(key).recipeIds.push(rec.id); continue; }
  const obj = objById.get(rec.id);
  const desc = strings.get(`STR_CHEFZ_ITEM_${key.replace(/^ChefZ_/, '').toUpperCase()}_DESC`) || '';
  pages.set(key, { key, name: rec.name, recipeIds: [rec.id], prompt: promptFor(rec, obj, desc) });
}

// Varianten ("Group", "Broth") sind nicht die beste Vorlage fuer das Bild:
// die Grundfassung hat die kleinste, klarste Zutatenliste. Liegt eine Variante
// vorne, wird mit dem Grundrezept neu gebaut.
for (const p of pages.values()) {
  const base = p.recipeIds.find(id => !/Group|Broth$/.test(id)) || p.recipeIds[0];
  if (base !== p.recipeIds[0]) {
    const rec = recipes.find(r => r.id === base);
    p.prompt = promptFor(rec, objById.get(base), strings.get(`STR_CHEFZ_ITEM_${p.key.replace(/^ChefZ_/, '').toUpperCase()}_DESC`) || '');
  }
}

fs.mkdirSync(path.dirname(OUT), { recursive: true });
fs.writeFileSync(OUT, JSON.stringify([...pages.values()], null, 2) + '\n');
console.log(`${pages.size} Gerichte aus ${recipes.length} Rezepten -> ${path.relative(REPO, OUT)}`);
for (const p of problems) console.log('  ! ' + p);

/** Nur fuer die Kurzbeschreibung im Prompt: englischer Text je STR-Schluessel. */
function loadEnglish() {
  const out = new Map();
  const addons = path.join(REPO, 'ChefZ_Core', 'Addons');
  for (const dir of fs.readdirSync(addons)) {
    const f = path.join(addons, dir, 'stringtable.csv');
    if (!fs.existsSync(f)) continue;
    for (const line of fs.readFileSync(f, 'utf8').split(/\r?\n/)) {
      const m = /^"(STR_[^"]+)","[^"]*","([^"]*)"/.exec(line);
      if (m) out.set(m[1], m[2].split(/(?<=\.)\s/)[0]);   // nur der erste Satz - der Rest sind Spielwerte
    }
  }
  return out;
}
