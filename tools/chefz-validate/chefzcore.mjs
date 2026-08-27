// chefzcore - der Core hat kein Content-Vokabular und kennt kein Fremdsystem.
//
// AUFLAGE, KEINE ZUGABE. OF-11, zweiter Absatz:
//
//   "Zweite Auflage: chefzcore.mjs prueft, dass in Addons/ChefZ_Core/Scripts/**
//    kein Content-Bezeichner und kein Fremdsystemname vorkommt. Ohne ihn sind
//    die Invarianten I3 und I4 Absichtserklaerungen."
//
// I3 - Der Core hat kein Content-Vokabular.
//      Kein enum fuer Kategorien, Zustaende, Prozesse, Qualitaetsstufen,
//      Behaelterkategorien, Werkzeuggruppen, Effekt-IDs oder Events. Der Core
//      kennt Record-ARTEN, Content liefert Record-INSTANZEN.
// I4 - Der Core ist fremdsystem-frei.
//      Kein #ifdef, kein optionaler Aufruf, kein Klassenname, keine
//      Zeichenkette. Die Abhaengigkeit ist umgekehrt: Comp-Module kennen ChefZ.
//
// Fuenf Regeln:
//
//   C1  Fremdsystemname irgendwo in einer Core-Datei          (Text UND Kommentar)
//   C2  ID oder Klasse eines Content-Moduls im Core-Code
//   C3  Content-Wort aus der Production Map im Core-Code
//   C4  enum oder modded enum mit Content-Bedeutung
//   C5  der Core selbst deklariert einen Content-Datensatz
//
// Warum C1 auch Kommentare liest und C2/C3 nicht:
//
// I4 sagt ausdruecklich "keine Zeichenkette" - ein Kommentar-Hook auf ein
// Fremdsystem ist der Anfang einer Abhaengigkeit. C1 liest deshalb die ganze
// Datei: im CODE ist der Fund ein Fehler, im KOMMENTAR eine Warnung, denn die
// Vanilla-Befunde dieses Projekts belegen ihre Aussagen an ausgelieferten
// Fremdmods, und ein Beleg ist das Gegenteil einer Abhaengigkeit. Wer den Beleg
// behalten will, schreibt "I4-BELEG" in den Block - bewusst und greppbar.
//
// Bei I3 dagegen ist die Prosa das Gegenteil eines Verstosses: mehrere
// Core-Dateien schreiben ausdruecklich hin, welche Content-Namen dort NICHT
// stehen duerfen. Wer das ahndete, bestrafte die Dokumentation der Regel.
// C2 und C3 pruefen deshalb den CODE - Bezeichner und Zeichenketten.

import path from 'node:path';
import {
  Findings, coreScriptFiles, readText, stripComments, refIndex, rel,
  configTrees, ADDONS_DIR, exists,
} from './lib.mjs';
import { allRecords, moduleOf, isCoreFile, CFG_ROOTS } from './chefzdata.mjs';

// --- C1: Fremdsysteme -------------------------------------------------------
//
// Namensmuster statt Namensliste. Eine Liste veraltet mit dem naechsten
// Fremdmod; ein Praefix nicht - jedes dieser Projekte praefixt konsequent.
const FOREIGN_PATTERNS = [
  { re: /\bTerje\w*/g, what: 'TerjeMods' },
  { re: /\bDayZExpansion\w*|\bExpansion[A-Z]\w*/g, what: 'DayZ Expansion' },
  { re: /\bCommunityOnlineTools\b|\bCOT_\w+|\bJM_\w+/g, what: 'Community Online Tools' },
  { re: /\bCommunityFramework\b|\bCF_\w+/g, what: 'Community Framework' },
  { re: /\bDabsFramework\b|\bDabs\w+/g, what: 'Dabs Framework' },
];

// Zusaetzlich alles, was im Referenzindex steht UND ein Fremdpraefix traegt.
// Damit wachsen die erkannten Namen mit build-refindex.mjs mit, ohne dass die
// Musterliste oben aufgeblaeht wird.
const FOREIGN_PREFIX = /^(Terje|Expansion|CF_|JM_|COT_|Dabs)/;

// --- C3: Content-Woerter ----------------------------------------------------
//
// Quelle: ChefZ_V1_Ingredient_Production_Map (Ketten und §73) und der DME-Plan.
// Bewusst NUR eindeutige Content-Substantive und Zustands-/Stufennamen. Was
// zugleich Vanilla-Vokabular ist (RAW, BAKED, BOILED, DRIED, BURNED, ROTTEN,
// BAKING, BOILING, DRYING), steht NICHT hier - es ist im Core erlaubt und
// noetig (ChefZ_VanillaStage, ChefZ_CookingHook).
const CONTENT_WORDS = [
  // Zustaende und Konservierung (Production Map §56, §64)
  'smoked', 'salted', 'pickled', 'fermented', 'cured', 'minced', 'curing',
  // Qualitaetsstufen (12 §3)
  'prepared', 'seasoned', 'premium',
  // Zutaten und Gerichte (Production Map, Ketten)
  'sausage', 'meat', 'steak', 'fillet', 'broth', 'dough', 'pasta', 'noodle',
  'bread', 'flatbread', 'flour', 'salt', 'sugar', 'herb', 'herbs', 'spice',
  'paprika', 'peppercorn', 'onion', 'potato', 'tomato', 'pumpkin', 'zucchini',
  'berry', 'apple', 'fish', 'carp', 'mackerel', 'chicken', 'pork', 'beef',
  'lard', 'cheese', 'milk', 'honey', 'rice', 'soup', 'stew', 'goulash',
  'casing', 'intestine', 'seasoning', 'dumpling', 'pancake', 'jam',
  // Stationen und Geraete (11, Production Map)
  'grinder', 'smoker', 'mortar', 'oven', 'cauldron', 'kettle',
  // Behaelter (16)
  'plate', 'bowl',
  // Effekt-IDs, wie sie 12 §3 und 17 §5 als Beispiel fuehren. Der Core reicht
  // Effekte nur weiter und darf keinen einzigen benennen.
  'spicy', 'hearty', 'refreshing',
];

// Woerter, die zwar wie Content klingen, aber Vokabular des Core oder der
// Engine sind. Jede Zeile braucht einen Grund - sonst waere die Ausnahmeliste
// die bequeme Tuer, durch die das Content-Vokabular doch hereinkommt.
//
// Die Liste wirkt auf C2 UND C3: sonst koennte ein Content-Modul den Core rot
// faerben, indem es eine Kategorie "CORE" oder eine Einheit "PIECE" deklariert.
// Der Core darf sein eigenes, geschlossenes Vokabular benutzen - das ist keine
// Content-Instanz, sondern die Sprache, in der Record-Arten beschrieben werden.
const CORE_VOCAB = new Map([
  ['DISH_DEFAULT', 'Vorgabe-Stufensatz aus 08 §3 und 12 §3 - der Core braucht einen Namen fuer "kein Stufensatz genannt".'],
  ['FOOD', 'CfgVehicles-Knoten der Engine (Food, FoodStages, FoodStageTransitions).'],
  ['CORE', 'ID des einen CoreSettings-Datensatzes und Name des Logkanals.'],
  ['ALL', 'Kanalauswahl "alle" in ChefZ_LogDefs.'],
  ['NONE', 'Vanilla-Garstufe und Kochmethode NONE (FoodStage.c:1, Cooking.c:1).'],
  ['OFF', 'Logstufe "aus".'],
  ['AUTO', '"nimm den Behaelter, aus dem die Zutat kam" (16 §4).'],
  ['PIECE', 'Vorgabe-Mengeneinheit des Core (05 §6).'],
  ['HANDS', 'Suchbereich eines Behaelters (16 §3.1).'],
  ['INVENTORY', 'Suchbereich eines Behaelters (16 §3.1).'],
  ['NEARBY_CARGO', 'Suchbereich eines Behaelters (16 §3.1).'],
  ['HANDCRAFT', 'Ausfuehrungsform eines Prozesses (11 §2).'],
  ['STATION_ACTION', 'Ausfuehrungsform eines Prozesses (11 §2).'],
  ['STATION_TIMED', 'Ausfuehrungsform eines Prozesses (11 §2).'],
  ['ON_STAGE', 'Abschlussart eines Rezepts (08 §3).'],
  ['TIMED', 'Abschlussart eines Rezepts (08 §3).'],
  ['INSTANT', 'Abschlussart eines Rezepts (08 §3).'],
  ['MIN', 'Qualitaetsregel eines Transforms (11 §4).'],
  ['MEAN', 'Qualitaetsregel eines Transforms (11 §4).'],
  ['MAX', 'Qualitaetsregel eines Transforms (11 §4).'],
  ['WEIGHTED_MEAN', 'Qualitaetsregel eines Transforms (11 §4).'],
  ['RAW', 'Vanilla-Garstufe (FoodStage.c:1).'],
  ['BAKED', 'Vanilla-Garstufe (FoodStage.c:1).'],
  ['BOILED', 'Vanilla-Garstufe (FoodStage.c:1).'],
  ['DRIED', 'Vanilla-Garstufe (FoodStage.c:1).'],
  ['BURNED', 'Vanilla-Garstufe (FoodStage.c:1).'],
  ['BURNT', 'Schreibweise, die ChefZ_VanillaStage auf BURNED abbildet.'],
  ['ROTTEN', 'Vanilla-Garstufe (FoodStage.c:1).'],
  ['BAKING', 'Vanilla-Kochmethode (Cooking.c:1).'],
  ['BOILING', 'Vanilla-Kochmethode (Cooking.c:1).'],
  ['DRYING', 'Vanilla-Kochmethode (Cooking.c:1).'],
  ['TIME', 'Vanilla-Kochmethode (Cooking.c:1).'],
]);

//! Kuerzere Bezeichner werden bei C2 nicht verglichen: eine ein- oder
//! zweibuchstabige Content-ID ("A", "X") wuerde sonst halbe Dateien anzuenden.
const MIN_CONTENT_ID_LENGTH = 3;

// --- C4: verbotene Aufzaehlungen --------------------------------------------
//
// Geprueft wird der INHALT, nicht nur der Name. "enum ChefZ_ESessionState"
// (IDLE, MATCHED, COMPLETING, DONE) ist ein Maschinenzustand des Kochvorgangs
// und voellig in Ordnung; "enum ChefZ_EFoodState" mit SMOKED und SALTED waere
// genau der Verstoss, den I3 meint. Ein Namensmuster allein koennte beide nicht
// unterscheiden.
const ENUM_CONTENT_NAME = /^ChefZ_E?(Category|Categories|FoodState|Quality|Qualities|ToolGroup|Effect|Tag|Tier)/i;

// --- Hilfen -----------------------------------------------------------------

/**
 * Ist die Fundstelle als bewusster Beleg gekennzeichnet?
 *
 * Gesucht wird "I4-BELEG" im Umkreis von zwoelf Zeilen davor - so weit reicht
 * ein Dateikopf. Der Marker ist die einzige Tuer, durch die ein Fremdname im
 * Kommentar ohne Befund bleibt, und sie ist absichtlich eng, laut und mit einem
 * Wort versehen, nach dem man grepen kann.
 */
function markedAsCitation(raw, index) {
  const before = raw.slice(0, index).split('\n');
  const window = before.slice(Math.max(0, before.length - 12)).join('\n');
  return /I4-BELEG/.test(window) || /I4-BELEG/.test(raw.slice(index, index + 400));
}

function lineIndex(src) {
  const offsets = [0];
  for (let i = 0; i < src.length; i++) if (src[i] === '\n') offsets.push(i + 1);
  return pos => {
    let lo = 0, hi = offsets.length - 1;
    while (lo < hi) { const mid = (lo + hi + 1) >> 1; if (offsets[mid] <= pos) lo = mid; else hi = mid - 1; }
    return lo + 1;
  };
}

/**
 * Zerlegt Code in Bezeichner und Zeichenketten - getrennt, weil sie
 * unterschiedlich behandelt werden (siehe Kopf und wordsOf()).
 */
function tokenize(code) {
  const at = lineIndex(code);
  const literals = [];
  const masked = code.replace(/"(?:[^"\\\n]|\\.)*"/g, (m, off) => {
    literals.push({ text: m.slice(1, -1), line: at(off) });
    return ' '.repeat(m.length);
  });
  const idents = [];
  for (const m of masked.matchAll(/[A-Za-z_][A-Za-z0-9_]*/g)) {
    idents.push({ text: m[0], line: at(m.index) });
  }
  return { idents, literals };
}

/** Bezeichner in Woerter zerlegen: Unterstriche und CamelCase-Grenzen. */
function wordsOf(token) {
  return token
    .replace(/([a-z0-9])([A-Z])/g, '$1 $2')
    .replace(/([A-Z]+)([A-Z][a-z])/g, '$1 $2')
    .split(/[^A-Za-z0-9]+/)
    .filter(Boolean)
    .map(w => w.toLowerCase());
}

export default function run() {
  const f = new Findings('chefzcore');
  const files = coreScriptFiles();

  if (files.length === 0) {
    f.warn(null, 0, 'Keine Core-Skripte gefunden - chefzcore hat nichts geprueft. Stimmt der Pfad Addons/ChefZ_Core/Scripts?');
    return f;
  }

  // --- Fremdnamen aus dem Referenzindex, gefiltert auf Fremdpraefixe -------
  const ref = refIndex();
  const foreignNames = new Set();
  for (const n of ref.names) if (FOREIGN_PREFIX.test(n)) foreignNames.add(n);

  // --- Content-Bezeichner der ANDEREN Module ------------------------------
  // Das ist die Regel, die mit dem Projekt mitwaechst: was ein Content-Modul
  // deklariert, darf im Core nicht vorkommen - ganz gleich, wie es heisst.
  const contentIds = new Map();          // token -> Herkunft
  for (const rec of allRecords()) {
    if (isCoreFile(rec.file)) continue;
    if (!rec.id) continue;
    contentIds.set(rec.id, `${rec.kind} "${rec.id}" aus ${moduleOf(rec.file)}`);
  }
  for (const { file, tree } of configTrees()) {
    if (isCoreFile(file)) continue;
    for (const node of tree.children) {
      const isCfgChefZ = !!CFG_ROOTS[node.name];
      const isItems = /^Cfg(Vehicles|Weapons|Magazines)$/.test(node.name);
      if (!isCfgChefZ && !isItems) continue;
      for (const child of node.children) {
        contentIds.set(child.name, `${node.name}-Klasse "${child.name}" aus ${moduleOf(file)}`);
      }
    }
  }

  const wordSet = new Set(CONTENT_WORDS);

  for (const file of files) {
    const raw = readText(file);
    if (raw === null) continue;
    const atRaw = lineIndex(raw);

    const code = stripComments(raw);

    // --- C1: Fremdsystem, ganze Datei inklusive Kommentar ----------------
    //
    // Im CODE ist es ein Fehler - dort waere es eine Abhaengigkeit, und I4
    // schliesst sie ohne Ausnahme aus.
    //
    // Im KOMMENTAR ist es eine Warnung, und das ist kein Aufweichen: die
    // Vanilla-Befunde des Projekts belegen ihre Aussagen an ausgelieferten
    // Fremdmods (siehe Tests/V_A_PboJsonSmoke), und ein Beleg ist das Gegenteil
    // einer Abhaengigkeit. Ein Kommentar-HOOK ("hier spaeter Terje aufrufen")
    // sieht genauso aus - deshalb bleibt der Befund sichtbar und verlangt eine
    // Entscheidung, statt zu verschwinden.
    for (const { re, what } of FOREIGN_PATTERNS) {
      re.lastIndex = 0;
      let m;
      while ((m = re.exec(raw)) !== null) {
        const inCode = code.slice(m.index, m.index + m[0].length) === m[0];
        const where = atRaw(m.index);
        if (inCode) {
          f.error(file, where,
            `Invariante I4: "${m[0]}" benennt ein Fremdsystem (${what}) im CODE. Der Core kennt `
            + `niemanden - die Abhaengigkeit laeuft umgekehrt: Comp-Module kennen ChefZ (00 §3, I4).`);
        } else if (markedAsCitation(raw, m.index)) {
          f.items.push({
            validator: 'chefzcore', severity: 'info', file: rel(file), line: where,
            summary: `"${m[0]}" (${what}) im Kommentar, als I4-BELEG gekennzeichnet - geprueft und in Ordnung.`,
          });
        } else {
          f.warn(file, where,
            `"${m[0]}" (${what}) steht in einem Kommentar des Core. Als Beleg ist das in Ordnung, `
            + `als Hook ("hier spaeter aufrufen") waere es der Anfang einer Abhaengigkeit, die I4 `
            + `ausschliesst. Wer den Beleg behalten will, schreibt "I4-BELEG" in den Block - `
            + `bewusst, sichtbar und greppbar.`);
        }
      }
    }

    const { idents, literals } = tokenize(code);

    // --- C1b: Fremdklassen aus dem Referenzindex --------------------------
    for (const t of idents) {
      if (foreignNames.has(t.text)) {
        f.error(file, t.line,
          `Invariante I4: "${t.text}" ist eine Klasse eines Fremdmods (Referenzindex). Im Core hat sie nichts zu suchen.`);
      }
    }

    // --- C2: Content-Bezeichner anderer Module ---------------------------
    for (const t of [...idents, ...literals]) {
      if (t.text.length < MIN_CONTENT_ID_LENGTH) continue;
      if (CORE_VOCAB.has(t.text)) continue;
      const origin = contentIds.get(t.text);
      if (origin) {
        f.error(file, t.line,
          `Invariante I3: "${t.text}" ist Content (${origin}). Der Core kennt Record-Arten, nicht Record-Instanzen.`);
      }
    }

    // --- C3: Content-Woerter ---------------------------------------------
    for (const t of idents) {
      if (CORE_VOCAB.has(t.text)) continue;
      for (const w of wordsOf(t.text)) {
        if (!wordSet.has(w)) continue;
        f.error(file, t.line,
          `Invariante I3: Bezeichner "${t.text}" enthaelt das Content-Wort "${w}". `
          + `Kategorien, Zustaende, Zutaten und Gerichte sind Daten, kein Code (00 §3, I3).`);
        break;
      }
    }
    for (const t of literals) {
      if (CORE_VOCAB.has(t.text)) continue;
      // Zeichenketten, die mit CHEFZ_ beginnen, sind Testmarken eines
      // Selbsttests. Sie koennen keinen Content bezeichnen: der Praefix ist
      // dafuer reserviert und in keinem Content-Modul zulaessig (DME §53).
      if (/^CHEFZ_/.test(t.text)) continue;
      const words = wordsOf(t.text);
      if (words.length === 0) continue;
      // Nur ID-foermige Zeichenketten pruefen. Ein deutscher Meldungstext
      // ("... die Zutat fehlt ...") ist Prosa und kein Bezeichner.
      if (!/^[A-Za-z][A-Za-z0-9_]*$/.test(t.text)) {
        if (!wordSet.has(t.text.toLowerCase())) continue;
      }
      for (const w of words) {
        if (!wordSet.has(w)) continue;
        f.error(file, t.line,
          `Invariante I3: Zeichenkette "${t.text}" enthaelt das Content-Wort "${w}". `
          + `Ein Zustands-, Kategorie- oder Gerichtename gehoert in die Daten, nicht in den Core.`);
        break;
      }
    }

    // --- C4: Aufzaehlungen ------------------------------------------------
    for (const m of code.matchAll(/\bmodded\s+enum\s+(\w+)/g)) {
      f.error(file, lineIndex(code)(m.index),
        `"modded enum ${m[1]}" ist verboten. Bei FoodStageType aendert es die Netzsync-Breite JEDES `
        + `Nahrungsmittels im Spiel (01 V4); bei jedem anderen Enum ist es dieselbe Art Eingriff.`);
    }
    for (const m of code.matchAll(/\benum\s+(\w+)\s*\{([^}]*)\}/g)) {
      const [name, body] = [m[1], m[2]];
      const members = body.split(',').map(s => s.split('=')[0].trim()).filter(Boolean);
      const badMember = members.find(mem => wordsOf(mem).some(w => wordSet.has(w)));
      const badName = ENUM_CONTENT_NAME.test(name);
      if (!badMember && !badName) continue;
      f.error(file, lineIndex(code)(m.index),
        `Invariante I3: "enum ${name}" zaehlt eine Content-Dimension auf`
        + (badMember ? ` (Wert "${badMember}")` : '')
        + `. Kategorien, Zustaende, Prozesse und Qualitaetsstufen sind Datensaetze mit String-IDs, `
        + `sonst ist jede neue Stufe eine Core-Codeaenderung (03 E1, §10.3).`);
    }
  }

  // --- C5: der Core deklariert selbst keinen Content ----------------------
  for (const rec of allRecords()) {
    if (!isCoreFile(rec.file)) continue;
    if (rec.kind === 'coreSettings') continue;
    f.error(rec.file, rec.line,
      `Invariante I3: Der Core liefert einen Datensatz der Art "${rec.kind}" ("${rec.id}"). `
      + `Der Core bringt ausser seinen eigenen Einstellungen keine Datensaetze mit - Content lebt in eigenen Addons (02 §4).`);
  }
  const coreCfg = path.join(ADDONS_DIR, 'ChefZ_Core', 'config.cpp');
  if (exists(coreCfg)) {
    for (const { file, tree } of configTrees()) {
      if (path.resolve(file) !== path.resolve(coreCfg)) continue;
      for (const node of tree.children) {
        if (CFG_ROOTS[node.name]) {
          f.error(file, node.line,
            `Invariante I3: Der Core deklariert "${node.name}". Rang-1-Datensaetze kommen aus Content-Modulen (02 §4).`);
        }
        if (/^Cfg(Vehicles|Weapons|Magazines)$/.test(node.name) && node.children.length > 0) {
          f.error(file, node.line,
            `Invariante I3: Der Core deklariert ${node.children.length} Klasse(n) in ${node.name} `
            + `("${node.children.map(c => c.name).join('", "')}"). Der Core enthaelt kein Item - auch keine Basisklasse (config.cpp-Kopf).`);
        }
      }
    }
  }

  f.items.push({
    validator: 'chefzcore', severity: 'info', file: rel(path.join(ADDONS_DIR, 'ChefZ_Core')), line: 0,
    summary: `Geprueft: ${files.length} Core-Skripte, ${contentIds.size} bekannte Content-Bezeichner, `
      + `${CONTENT_WORDS.length} Content-Woerter, ${foreignNames.size} indizierte Fremdklassen.`,
  });

  return f;
}
