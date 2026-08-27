// Kochbarkeit: deklarierter Kochpfad gegen tatsaechlich eingeschalteten Schalter.
//
// Der Blocker, der diese Regel ausgeloest hat, ist an jedem vorhandenen Pruefer
// vorbeigegangen: ChefZ_Edible_Base ueberschrieb CanBeCooked() nicht, Vanillas
// Default ist false, und damit blieb JEDE ChefZ-Zutat mit ChefZ-Skriptklasse
// beim Kochen auf Raw stehen - kein einziges ON_STAGE-Rezept konnte fertig
// werden. Die Validator-Ausgabe war vor und nach dem Fehler byteidentisch.
//
// A: Klasse deklariert Food > FoodStages UND FoodStageTransitions, aber keine
//    Klasse ihrer Skriptkette schaltet die Kochbarkeit ein. Die Uebergaenge sind
//    toter Text; Cooking.ProcessItemToCook geht an dem Item vorbei.
// B: Die Skriptklasse sagt CanBeCooked() == true, aber die Configklasse hat kein
//    Food > FoodStages. Dann entsteht kein FoodStage-Objekt und der Kochtakt
//    greift ins Leere.
// C: Die Klasse ist Nahrung, aber keine Klasse ihrer Skriptkette registriert eine
//    Essaktion. Vanilla setzt sie NICHT auf Edible_Base, sondern auf jeder
//    Nahrungsklasse einzeln (Potato.c: ActionEatFruit). Ohne sie wird das Item
//    im Spiel schlicht nicht zum Essen angeboten - wieder ohne Fehlerbild.

import path from 'node:path';
import { Findings, readText, stripComments, walk, exists, rel } from './lib.mjs';
import { configItemIndex } from './chefzdata.mjs';
import { foodContext, hasNode, scriptChainOf } from './chefzfood.mjs';

const VANILLA_SCRIPTS = 'C:/Users/Administrator/Desktop/Mod Repositories/scripts - 1.29';

/**
 * Skriptklassen mit einer CanBeCooked-Ueberschreibung, samt Urteil.
 *
 * Wichtig: "eingeschaltet" heisst NICHT "return true". Der Core rechnet die
 * Antwort inzwischen aus den Daten der Klasse aus. Als AUSgeschaltet gilt daher
 * nur, was woertlich "return false;" ist und sonst nichts tut - das ist dann
 * eine Entscheidung des Autors und keine Luecke.
 */
function canBeCookedOverrides(files) {
  // 'on'       - bedingungslos "return true;"
  // 'off'      - bedingungslos "return false;"
  // 'computed' - alles andere: die Klasse rechnet die Antwort aus ihren Daten
  //              aus. Das ist eingeschaltet fuer Regel A, aber KEINE Zusage,
  //              dass jede Erbin kocht - Regel B darf daraus nichts folgern.
  const out = new Map();   // Klassenname -> 'on' | 'off' | 'computed'
  for (const file of files) {
    const txt = readText(file);
    if (!txt || !txt.includes('CanBeCooked')) continue;
    const src = stripComments(txt);
    const classPositions = [...src.matchAll(/\bclass\s+([A-Za-z_]\w*)/g)]
      .map(m => ({ name: m[1], at: m.index }));
    for (const m of src.matchAll(/\bbool\s+CanBeCooked\s*\([^)]*\)\s*\{([^}]*)\}/g)) {
      const body = m[1];
      let owner = null;
      for (const c of classPositions) { if (c.at < m.index) owner = c.name; else break; }
      if (!owner) continue;
      const b = body.trim();
      let verdict = 'computed';
      if (/^return\s+false\s*;$/.test(b)) verdict = 'off';
      else if (/^return\s+true\s*;$/.test(b)) verdict = 'on';
      // Die erste Fundstelle je Klasse zaehlt, wie die Engine sie auch nimmt.
      if (!out.has(owner)) out.set(owner, verdict);
    }
  }
  return out;
}

/** Skriptklassen, die eine SELBST-Essaktion registrieren (ActionEat*). */
function eatActionClasses(files) {
  const out = new Set();
  for (const file of files) {
    const txt = readText(file);
    if (!txt || !txt.includes('AddAction')) continue;
    const src = stripComments(txt);
    const classPositions = [...src.matchAll(/class\s+([A-Za-z_]\w*)/g)]
      .map(m => ({ name: m[1], at: m.index }));
    // ActionForceFeed zaehlt bewusst NICHT: das ist Fuettern durch einen anderen
    // Spieler. Wer sein eigenes Essen essen will, braucht ein ActionEat*.
    for (const m of src.matchAll(/AddAction\s*\(\s*(ActionEat\w*)\s*\)/g)) {
      let owner = null;
      for (const c of classPositions) { if (c.at < m.index) owner = c.name; else break; }
      if (owner) out.add(owner);
    }
  }
  return out;
}

function vanillaScriptFiles() {
  if (!exists(VANILLA_SCRIPTS)) return [];
  return walk(VANILLA_SCRIPTS, (_f, n) => n.endsWith('.c'));
}

export default function run() {
  const f = new Findings('chefzcookable');

  const items = configItemIndex();
  if (items.size === 0) return f;

  const ctx = foodContext();

  const chefzFiles = new Set([...ctx.scripts.values()].map(v => v.file));
  const vanillaFiles = vanillaScriptFiles();
  if (vanillaFiles.length === 0) {
    f.warn(null, 0,
      `Vanilla-Skripte nicht gefunden (${VANILLA_SCRIPTS}). Ob eine Vanilla-Basisklasse `
      + 'die Kochbarkeit einschaltet, ist damit nicht pruefbar - Regel A meldet nur, was '
      + 'sicher fehlt.');
  }

  const allScriptFiles = [...chefzFiles, ...vanillaFiles];
  const overrides = canBeCookedOverrides(allScriptFiles);
  const eaters = eatActionClasses(allScriptFiles);


  // Vanilla-Skriptklassen und ihre Elternkette, damit die Kette ueber die
  // ChefZ-Grenze hinaus weiterverfolgt werden kann.
  const vanillaParents = new Map();
  for (const file of vanillaFiles) {
    const txt = readText(file);
    if (!txt) continue;
    for (const m of stripComments(txt).matchAll(/\bclass\s+([A-Za-z_]\w*)\s*(?:extends|:)\s*([A-Za-z_]\w*)/g)) {
      if (!vanillaParents.has(m[1])) vanillaParents.set(m[1], m[2]);
    }
  }

  /**
   * Skriptkette einer Configklasse, ueber ChefZ hinaus in die Vanilla-Skripte.
   *
   * Hat die Klasse selbst keine Skriptklasse, geht es die CONFIG-Elternkette
   * hinauf bis zum ersten Namen, zu dem eine existiert - genau so loest die
   * Engine es auf. Ohne diesen Schritt endet die Kette bei der Klasse selbst,
   * und eine Vanilla-Basisklasse, die die Kochbarkeit einschaltet, bliebe
   * unsichtbar.
   */
  function fullChain(cls) {
    let start = cls;
    const guard = new Set();
    while (start && !ctx.scripts.has(start) && !vanillaParents.has(start)
           && !overrides.has(start) && !guard.has(start)) {
      guard.add(start);
      start = items.get(start)?.parent ?? null;
    }
    if (!start) start = cls;
    const chain = scriptChainOf(start, ctx.scripts);
    if (start !== cls) chain.unshift(cls);
    let cur = chain[chain.length - 1];
    const seen = new Set(chain);
    while (cur && vanillaParents.has(cur)) {
      cur = vanillaParents.get(cur);
      if (!cur || seen.has(cur)) break;
      seen.add(cur);
      chain.push(cur);
    }
    return chain;
  }

  for (const [cls, entry] of [...items.entries()].sort()) {
    const stages = hasNode(cls, 'Food', 'FoodStages');
    const transitions = hasNode(cls, 'Food', 'FoodStageTransitions');
    const chain = fullChain(cls);
    // Ein "return false;" auf einer VANILLA-Basisklasse ist der Engine-Default -
    // Edible_Base und ItemBase tun beide genau das - und damit der Gegenstand
    // dieser Regel, keine Entscheidung eines Autors. Wuerde er zaehlen, endete
    // jede Kette bei ihm auf "bewusst ausgeschaltet" und die Regel schwiege ueber
    // genau den Fall, fuer den sie gebaut wurde. Ein "false" auf einer
    // ChefZ-Klasse zaehlt dagegen: das hat jemand hingeschrieben.
    const verdict = chain.map(n => {
      const v = overrides.get(n);
      if (v === 'off' && !n.startsWith('ChefZ_')) return undefined;
      return v;
    }).find(v => v !== undefined);

    // --- Regel A ---
    if (stages && transitions && verdict === undefined) {
      // Nur an der Klasse melden, die den Uebergangsknoten wirklich besitzt -
      // sonst ergeben acht Brotsorten acht identische Befunde.
      // Nur an der Klasse melden, die den Knoten wirklich besitzt - aber nur,
      // wenn dieser Eigentuemer selbst im Projekt liegt. Erbt die Klasse die
      // Uebergaenge von einer VANILLA-Basis, gibt es dort nichts zu melden,
      // und der Befund gehoert an die ChefZ-Klasse.
      const ownerName = transitions.owner?.name ?? transitions.owner ?? cls;
      if (ownerName !== cls && items.has(ownerName)) continue;
      f.error(entry.file ?? null, entry.line ?? 0,
        `"${cls}" deklariert Food > FoodStageTransitions, aber keine Klasse ihrer Skriptkette `
        + `(${chain.join(' -> ')}) schaltet die Kochbarkeit ein. Edible_Base.CanBeCooked() liefert `
        + `false, Cooking.ProcessItemToCook geht damit an dem Item vorbei, die Garstufe bleibt `
        + `stehen - und ein ON_STAGE-Rezept mit dieser Klasse in einem Pflicht-Slot wird nie fertig. `
        + `Die Uebergaenge sind toter Text. Abhilfe: von ChefZ_Edible_Base erben (dort folgt der `
        + `Schalter den Daten) oder "override bool CanBeCooked() { return true; }" auf der eigenen `
        + `Klasse. Soll sie wirklich nicht kochen, schreib "return false;" hin - dann ist es eine `
        + `Entscheidung und keine Luecke.`);
    }

    // --- Regel C ---
    // Nahrung ist, was Naehrwerte oder Garstufen deklariert. Basisklassen ohne
    // eigene Item-Existenz melden wir mit, weil dort die Loesung hingehoert.
    const nutrition = hasNode(cls, 'Food', 'Nutrition') || hasNode(cls, 'Nutrition');
    // Bulk im Kochgeraet wird nicht gegessen, sondern portioniert - dafuer haengt
    // ChefZ_ActionTakePortion an ChefZ_PortionedFood_Base. Eine Essaktion dort zu
    // verlangen waere falsch.
    const isBulk = chain.includes('ChefZ_PortionedFood_Base') || chain.includes('ChefZ_PortionedDish_Base');
    if (cls.startsWith('ChefZ_') && (stages || nutrition) && !isBulk
        && !chain.some(n => eaters.has(n))) {
      f.error(entry.file ?? null, entry.line ?? 0,
        `"${cls}" ist Nahrung, aber keine Klasse ihrer Skriptkette (${chain.join(' -> ')}) `
        + `registriert eine Essaktion. Vanilla setzt sie nicht auf Edible_Base, sondern auf jeder `
        + `Nahrungsklasse einzeln - Potato.c schreibt "AddAction(ActionEatFruit);". Ohne sie wird `
        + `das Item im Spiel nicht zum Essen angeboten: kein Fehlerbild, keine Logzeile, die `
        + `Aktion fehlt einfach. Abhilfe: SetActions() ueberschreiben und die passende ActionEat*-`
        + `Variante hinzufuegen (Fruit, Big, Small, Cereal, Meat je nach Item und Animation).`);
    }

    // --- Regel B ---
    // Nur bei einer BEDINGUNGSLOSEN Zusage, und nur fuer ChefZ-eigene Klassen:
    // bei einer Vanilla-Klasse, die das Projekt nur erweitert, stehen die
    // FoodStages in den Spieldaten, die hier niemand lesen kann.
    if (!stages && verdict === 'on' && cls.startsWith('ChefZ_')) {
      f.error(entry.file ?? null, entry.line ?? 0,
        `"${cls}" ist ueber ihre Skriptkette (${chain.join(' -> ')}) kochbar, hat aber kein `
        + `Food > FoodStages. Damit entsteht kein FoodStage-Objekt, HasFoodStage() ist false, und `
        + `Cooking.UpdateCookingState greift je Kochtakt ins Leere.`);
    }
  }

  return f;
}
