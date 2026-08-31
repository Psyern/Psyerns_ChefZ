//==============================================================================
// icons.mjs - Vektorsymbole als ZWEITE Wahl, wenn kein Itembild vorliegt.
//
// Warum es sie gibt: das Repo fuehrt keine Inventarsymbole, und eine Karte aus
// leeren Kaesten ist keine Infografik. Ein gezeichnetes Symbol sagt in einer
// Zelle mehr als das Wort "ROOT".
//
// Warum sie den Bericht NICHT ersetzen: der Brief verbietet den stillen
// Platzhalter. Ein Glyph ist ein Platzhalter - nur ein lesbarer. Er wird
// deshalb weiterhin gezaehlt und gemeldet, in einer eigenen Zeile ("als
// Vektorsymbol gezeichnet"), damit niemand ein Symbol fuer ein Itemfoto haelt.
//
// Rangfolge in der Zelle:
//   1. echtes Bild aus item-images.json
//   2. Vektorsymbol dieser Datei
//   3. rot gestrichelter Kasten - nur, wenn auch keine Familie passt
//
// --- Wie 96 Schluessel mit ~30 Zeichnungen auskommen ----------------------
// Nicht jede Klasse bekommt ein eigenes Symbol; das waere am naechsten Gericht
// wieder veraltet. Erkannt wird die FAMILIE (Wurzelgemuese, Wurst, Bruehe,
// Teller ...), und die Familie zeichnet. Ein neues Rezept, das Karotten nennt,
// bekommt sein Symbol ohne eine Zeile Arbeit.
//
// Alle Pfade sind in einem 100x100-Feld gezeichnet und werden in die Zelle
// skaliert. Linien, keine Flaechen - so bleibt es kantig statt Web-bunt.
//==============================================================================

const G = {
  salt:      'M36 34 h28 l6 44 h-40 z M44 22 h12 v12 h-12 z M46 44 h2 M54 50 h2 M48 58 h2',
  spice:     'M32 40 h36 v38 h-36 z M40 30 h20 v10 h-20 z M42 52 h2 M52 58 h2 M46 66 h2 M58 48 h2',
  herb:      'M50 82 V30 M50 44 C36 40 32 28 32 28 C46 28 50 40 50 44 M50 56 C64 52 68 40 68 40 C54 40 50 52 50 56',
  root:      'M50 84 L38 42 C44 34 56 34 62 42 Z M50 42 V26 M50 30 L38 20 M50 30 L62 20',
  vegetable: 'M50 80 A22 22 0 1 1 50 36 A22 22 0 1 1 50 80 Z M50 36 V24 M50 28 L62 20',
  leaf:      'M28 74 C28 40 50 24 74 26 C74 56 52 74 28 74 Z M34 68 C46 56 58 46 70 34',
  mushroom:  'M26 50 C26 32 74 32 74 50 Z M42 50 v22 a8 8 0 0 0 16 0 V50',
  tomato:    'M50 82 A24 22 0 1 1 50 34 A24 22 0 1 1 50 82 Z M50 34 V24 M38 28 L50 32 L62 28',
  fruit:     'M50 82 C30 82 26 60 34 46 C40 36 50 40 50 40 C50 40 60 36 66 46 C74 60 70 82 50 82 Z M50 40 V24 M50 30 L64 22',
  beans:     'M30 44 A10 7 0 1 0 30 58 A10 7 0 1 0 30 44 Z M52 38 A10 7 0 1 0 52 52 A10 7 0 1 0 52 38 Z M42 62 A10 7 0 1 0 42 76 A10 7 0 1 0 42 62 Z M68 56 A10 7 0 1 0 68 70 A10 7 0 1 0 68 56 Z',
  corn:      'M50 84 C34 76 32 44 40 26 C46 16 54 16 60 26 C68 44 66 76 50 84 Z M50 26 V82 M40 40 H60 M40 54 H60 M40 68 H58',
  onion:     'M50 84 C30 84 26 62 34 48 C40 38 50 34 50 34 C50 34 60 38 66 48 C74 62 70 84 50 84 Z M50 34 V20 M42 58 C46 70 54 70 58 58',
  potato:    'M28 56 C28 38 48 28 66 34 C80 40 78 68 62 76 C46 84 28 74 28 56 Z M42 48 h3 M58 58 h3 M50 66 h3',
  pumpkin:   'M50 80 A28 24 0 1 1 50 32 A28 24 0 1 1 50 80 Z M50 32 V80 M36 36 C30 50 30 62 36 76 M64 36 C70 50 70 62 64 76 M50 32 V20',
  pepper:    'M34 44 C34 74 42 82 50 82 C58 82 66 74 66 44 Z M50 44 V30 M50 34 L62 26',
  meat:      'M26 58 C26 36 46 24 62 30 C78 36 80 62 66 72 C52 82 26 78 26 58 Z M44 46 A9 9 0 1 0 44 64 A9 9 0 1 0 44 46 Z',
  minced:    'M24 56 h52 a26 22 0 0 1 -52 0 Z M30 48 h6 M42 44 h6 M54 48 h6 M66 46 h4 M36 38 h6 M50 34 h6',
  sausage:   'M28 68 C24 44 44 26 66 30 C78 32 80 46 70 52 C56 60 48 62 44 74 C40 84 30 82 28 68 Z M34 66 h4 M48 52 h4 M62 40 h4',
  fish:      'M20 56 C34 34 62 34 76 56 C62 78 34 78 20 56 Z M76 56 L90 42 V70 Z M36 50 A4 4 0 1 0 36 58 A4 4 0 1 0 36 50 Z M52 40 V72',
  bone:      'M28 40 A9 9 0 1 1 40 52 L60 60 A9 9 0 1 1 72 72 A9 9 0 1 1 60 60 M40 52 A9 9 0 1 1 28 40',
  fat:       'M32 34 h36 v42 h-36 z M32 48 h36 M32 62 h36 M42 34 V76 M58 34 V76',
  dairy:     'M38 32 h24 v48 h-24 z M38 32 L50 18 L62 32 M40 50 h20',
  butter:    'M24 46 h44 l10 -10 v34 l-10 10 h-44 z M68 46 l10 -10 M24 46 L34 36 h44',
  cream:     'M34 34 h30 v46 h-30 z M64 44 h10 v14 h-10 M38 26 h22 v8 h-22 z',
  cheese:    'M22 70 L74 34 V70 Z M40 60 A5 5 0 1 0 40 62 Z M56 52 A5 5 0 1 0 56 54 Z',
  egg:       'M50 82 C34 82 28 66 34 50 C39 36 45 28 50 28 C55 28 61 36 66 50 C72 66 66 82 50 82 Z',
  broth:     'M22 52 h56 a28 26 0 0 1 -56 0 Z M36 40 c0 -8 8 -8 8 -16 M50 36 c0 -8 8 -8 8 -16 M64 40 c0 -8 8 -8 8 -16',
  sauce:     'M42 30 h16 v12 l8 12 v28 h-32 V54 l8 -12 Z M34 60 h32 M44 22 h12 v8 h-12 z',
  pasta:     'M26 34 C40 46 40 68 26 80 M42 34 C56 46 56 68 42 80 M58 34 C72 46 72 68 58 80',
  bread:     'M22 70 C22 44 34 32 50 32 C66 32 78 44 78 70 Z M34 70 V44 M50 70 V36 M66 70 V44',
  dough:     'M24 74 C24 48 40 36 50 36 C60 36 76 48 76 74 Z M38 60 h4 M56 54 h4 M48 66 h4',
  flour:     'M32 34 h36 v46 h-36 z M32 34 L40 22 h20 l8 12 M42 52 h16 M42 62 h16',
  rice:      'M32 40 a5 8 0 1 0 0.1 0 Z M50 36 a5 8 0 1 0 0.1 0 Z M68 42 a5 8 0 1 0 0.1 0 Z M38 60 a5 8 0 1 0 0.1 0 Z M58 62 a5 8 0 1 0 0.1 0 Z M48 76 a5 8 0 1 0 0.1 0 Z',
  honey:     'M32 40 h36 v40 h-36 z M40 30 h20 v10 h-20 z M50 46 l6 10 h-12 Z M50 62 l6 10 h-12 Z',
  can:       'M32 30 h36 v50 h-36 z M32 30 a18 6 0 0 0 36 0 M32 40 h36 M40 52 h20 M40 62 h20',
  culture:   'M42 22 h16 v18 l14 34 a6 6 0 0 1 -6 8 h-32 a6 6 0 0 1 -6 -8 l14 -34 Z M36 62 h28 M46 70 h3 M56 68 h3',
  plate:     'M16 66 h68 a34 12 0 0 1 -68 0 Z M30 60 C30 44 70 44 70 60 Z M50 44 V36',
  bowl:      'M22 54 h56 a28 26 0 0 1 -56 0 Z M22 54 a28 8 0 0 1 56 0 M40 42 c0 -8 6 -8 6 -14 M56 42 c0 -8 6 -8 6 -14',
  pan:       'M18 46 h48 v10 a24 16 0 0 1 -48 0 Z M66 50 h20 v6 h-20 M30 38 c0 -8 6 -8 6 -14 M46 38 c0 -8 6 -8 6 -14',
  generic:   'M28 28 h44 v44 h-44 z M28 28 L72 72 M72 28 L28 72',
};

// --- Familienerkennung -------------------------------------------------------
// Reihenfolge ist Absicht: das Spezifischere zuerst. "MINCED_MEAT" darf nicht
// bei "MEAT" haengenbleiben, und ein "...SoupBowl" ist eine Schale, kein Teller.
const RULES = [
  [/SALT/i, 'salt'],
  [/MINCED/i, 'minced'],
  [/SAUSAGE/i, 'sausage'],
  [/BACON/i, 'meat'],
  [/(WILD_MEAT|VENISON|BOAR|HUNTER_MEAT)/i, 'meat'],
  [/MEAT/i, 'meat'],
  [/(FISH|SARDINE|BITTERLING)/i, 'fish'],
  [/BONE/i, 'bone'],
  [/(BROTH|SOUP)/i, 'broth'],
  [/(TOMATO_SAUCE|CREAM_SAUCE|SAUCE)/i, 'sauce'],
  [/TOMATO/i, 'tomato'],
  [/(BUTTER)/i, 'butter'],
  [/(CREAM)/i, 'cream'],
  [/(CHEESE|CURD)/i, 'cheese'],
  [/(DAIRY|MILK)/i, 'dairy'],
  [/EGG/i, 'egg'],
  [/FAT|LARD/i, 'fat'],
  [/(HERB)/i, 'herb'],
  [/(SPICE|PAPRIKA|PEPPER_POWDER)/i, 'spice'],
  [/PEPPER/i, 'pepper'],
  [/MUSHROOM/i, 'mushroom'],
  [/(ROOT_VEGETABLE|CARROT)/i, 'root'],
  [/(LEAF_VEGETABLE|CABBAGE)/i, 'leaf'],
  [/POTATO/i, 'potato'],
  [/PUMPKIN/i, 'pumpkin'],
  [/ONION|GARLIC/i, 'onion'],
  [/CORN/i, 'corn'],
  [/BEAN/i, 'beans'],
  [/(FRUIT|BERRY|BERRIES|COMPOTE)/i, 'fruit'],
  [/VEGETABLE/i, 'vegetable'],
  [/PASTA|SPAGHETTI|NOODLE/i, 'pasta'],
  [/(BREAD|FLATBREAD)/i, 'bread'],
  [/DOUGH/i, 'dough'],
  [/FLOUR/i, 'flour'],
  [/RICE/i, 'rice'],
  [/(HONEY|SWEETENER|SUGAR)/i, 'honey'],
  [/(CANNED|CAN_OPENED|TACTICAL)/i, 'can'],
  [/CULTURE/i, 'culture'],
  [/BOWL/i, 'bowl'],
  [/PAN/i, 'pan'],
  [/(PLATE|BREAKFAST|DUMPLING|PANCAKE)/i, 'plate'],
];

// Die FORM steht am Ende eines Klassennamens, nicht am Anfang:
// ChefZ_BoneBrothSoupBowl ist eine Schale, kein Knochen, und
// ChefZ_CheeseFlatbread ist ein Fladenbrot, kein Kaesekeil. Deshalb laeuft
// dieser Durchgang VOR den Zutatenregeln - beide Faelle waren im ersten
// Entwurf falsch gezeichnet.
const SUFFIX = [
  [/Bowl$/i, 'bowl'],
  [/(Plate|Breakfast|Dumplings|Pancakes)$/i, 'plate'],
  [/Pan$/i, 'pan'],
  [/Sauce$/i, 'sauce'],
  [/Sausage$/i, 'sausage'],
  [/(Flatbread|Bread)$/i, 'bread'],
  [/Broth$/i, 'broth'],
  [/(Spaghetti|Pasta)$/i, 'pasta'],
  [/Compote$/i, 'fruit'],
  [/Rice$/i, 'rice'],
];

/**
 * Familie zu einem Bildschluessel.
 *
 * Reihenfolge, und jede Stufe hat ihren Grund:
 *   1. Formsuffix des Klassennamens - was das Gericht IST.
 *   2. Zutatenregeln auf dem SCHLUESSEL.
 *   3. erst zuletzt der Anzeigetext, und nur bis zum ersten Ausschluss:
 *      "Dairy −Butter −Cream" ist Milchprodukt, NICHT Butter. Genau daran
 *      zeichnete der erste Entwurf drei Butterbloecke auf die Kaesekarte.
 *
 * @returns {string|null} Familienname oder null, wenn nichts passt
 */
export function familyOf(key, what) {
  const k = String(key || '').replace(/^(cat:|tag:)/i, '').replace(/^CHEFZ_/i, '');
  for (const [re, fam] of SUFFIX) if (re.test(k)) return fam;
  for (const [re, fam] of RULES) if (re.test(k)) return fam;
  const w = String(what || '').split('−')[0];   // alles ab dem ersten Minus weg
  for (const [re, fam] of RULES) if (re.test(w)) return fam;
  return null;
}

/** Zeichnet die Familie als SVG in ein Quadrat (x, y, size). */
export function renderGlyph(family, x, y, size, color) {
  const d = G[family] || G.generic;
  const k = size / 100;
  return '<g transform="translate(' + x + ',' + y + ') scale(' + k.toFixed(4) + ')" ' +
         'fill="none" stroke="' + color + '" stroke-width="4" ' +
         'stroke-linecap="round" stroke-linejoin="round"><path d="' + d + '"/></g>';
}

export const FAMILIES = Object.keys(G);

// Der Name, unter dem eine Familie in der Legende steht. Ohne diese Liste
// beschriftete sich die Legende mit dem ERSTEN Fundstueck - "Beans And ..."
// fuer das Tellersymbol - was schlechter ist als gar keine Legende.
export const FAMILY_NAMES = {
  salt: 'Salt', spice: 'Spice', herb: 'Herb',
  root: 'Root', vegetable: 'Veg', leaf: 'Leaf',
  mushroom: 'Mushroom', tomato: 'Tomato', fruit: 'Fruit', beans: 'Beans',
  corn: 'Corn', onion: 'Onion', potato: 'Potato', pumpkin: 'Pumpkin',
  pepper: 'Pepper', meat: 'Meat', minced: 'Minced', sausage: 'Sausage',
  fish: 'Fish', bone: 'Bone', fat: 'Fat', dairy: 'Dairy', butter: 'Butter',
  cream: 'Cream', cheese: 'Cheese', egg: 'Egg', broth: 'Broth', sauce: 'Sauce',
  pasta: 'Pasta', bread: 'Bread', dough: 'Dough', flour: 'Flour', rice: 'Rice',
  honey: 'Honey', can: 'Canned', culture: 'Culture', plate: 'Plate',
  bowl: 'Bowl', pan: 'Pan', generic: 'Other',
};
