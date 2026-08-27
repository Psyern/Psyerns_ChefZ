// chefzstage - "kochbar ohne FoodStageTransitions heisst: verbrennt im Topf" (01 V4).
//
// Der Befund, woertlich:
//
//   FoodStage.c:472  (GetNextFoodStageType)
//       return FoodStageType.BURNED;   //If the item cannot transition out of
//                                      //current state, burn it
//   ItemBase.c:2654  (HasFoodStage)
//       "CfgVehicles %1 Food FoodStages"
//   FoodStage.c:167  (SetupFoodStageTransitionMapping)
//       "CfgVehicles %1 Food FoodStageTransitions %2"
//   Cooking.c:47
//       if (item_to_cook && item_to_cook.CanBeCooked())
//
// Die Kette ist damit vollstaendig: Vanilla kocht, was CanBeCooked() sagt;
// den naechsten Garzustand holt es aus FoodStageTransitions; findet es dort
// nichts, VERBRENNT es das Item. Eine ChefZ-Klasse, die kochbar ist und keine
// Uebergaenge deklariert, ist deshalb keine unvollstaendige Config, sondern
// eine Falle: der Spieler legt sie in den Topf und bekommt Kohle.
//
// Die Uebergaenge stehen datengetrieben in der config.cpp und brauchen KEIN
// Skript:
//
//   class Food { class FoodStageTransitions { class Raw { class Baking {
//       transition_to = 2; cooking_method = 0; }; }; }; };
//
// Enum-Erweiterung ist ausgeschlossen (01 V4, Punkt 1): "modded enum
// FoodStageType" aendert die Netzsync-Breite JEDES Nahrungsmittels im Spiel.
// chefzcore.mjs setzt das durch, hier geht es nur um die Uebergaenge.

import { Findings } from './lib.mjs';
import { configItemIndex } from './chefzdata.mjs';
import { foodContext, edibleEvidence, hasNode, scriptChainOf, EDIBLE_SCRIPT_BASES } from './chefzfood.mjs';
import { readText, stripComments } from './lib.mjs';

/**
 * Warum eine Klasse als KOCHBAR gilt - oder null.
 *
 * Zwei Belege, beide statisch nachweisbar:
 *   1. eine Skriptklasse gleichen Namens ueberschreibt CanBeCooked() mit true
 *      (das ist woertlich die Bedingung in Cooking.c:47)
 *   2. die Klasse steht als Eingang in einem ChefZ-REZEPT - und Rezepte zuenden
 *      ausschliesslich an Kochgeraeten (08 §2). Wer dort als Zutat auftaucht,
 *      liegt im Topf, waehrend Vanilla seinen Garzustand fortschreibt.
 */
function cookableEvidence(cls, ctx, cookableScripts) {
  if (cookableScripts.has(cls)) return 'die Skriptklasse liefert CanBeCooked() == true';
  const uses = ctx.inputClasses.get(cls);
  if (uses) {
    const recipeUse = uses.find(u => u.rec.kind === 'recipe');
    if (recipeUse) {
      return `sie ist Zutat in Rezept "${recipeUse.rec.id}" (${recipeUse.where}) und liegt damit im Kochgeraet`;
    }
  }
  return null;
}

/** Skriptklassen, deren CanBeCooked()-Ueberschreibung true liefert. */
function collectCookableScripts(scripts) {
  const out = new Set();
  const files = new Set([...scripts.values()].map(v => v.file));
  for (const file of files) {
    const txt = readText(file);
    if (!txt) continue;
    const src = stripComments(txt);
    // "class X ... override bool CanBeCooked() { return true; }" - die
    // Zuordnung laeuft ueber die zuletzt geoeffnete Klasse vor dem Fund.
    const classPositions = [...src.matchAll(/\bclass\s+([A-Za-z_]\w*)/g)]
      .map(m => ({ name: m[1], at: m.index }));
    for (const m of src.matchAll(/\bbool\s+CanBeCooked\s*\([^)]*\)\s*\{([^}]*)\}/g)) {
      if (!/\breturn\s+true\s*;/.test(m[1])) continue;
      let owner = null;
      for (const c of classPositions) { if (c.at < m.index) owner = c.name; else break; }
      if (owner) out.add(owner);
    }
  }
  return out;
}

export default function run() {
  const f = new Findings('chefzstage');
  const ctx = foodContext();
  const items = configItemIndex();
  const cookableScripts = collectCookableScripts(ctx.scripts);

  if (items.size === 0) {
    f.items.push({
      validator: 'chefzstage', severity: 'info', file: '', line: 0,
      summary: 'Keine Item-Klassen im Projekt - der Core deklariert bewusst keine (Invariante I3). Nichts zu pruefen.',
    });
    return f;
  }

  let checked = 0;

  for (const [cls, entry] of [...items.entries()].sort()) {
    const stages = hasNode(cls, 'Food', 'FoodStages');
    const transitions = hasNode(cls, 'Food', 'FoodStageTransitions');
    const edible = edibleEvidence(cls, ctx);
    const cookable = cookableEvidence(cls, ctx, cookableScripts);

    // Klassen ohne jeden Nahrungsbezug interessieren hier nicht.
    if (!stages && !transitions && !edible && !cookable) continue;
    checked++;

    if (cookable && !transitions) {
      f.error(entry.file, entry.node.line,
        `"${cls}" ist kochbar (${cookable}), deklariert aber keine FoodStageTransitions. `
        + `FoodStage.GetNextFoodStageType faellt dann auf FoodStageType.BURNED zurueck `
        + `(FoodStage.c:472) - das Item VERBRENNT beim ersten Garstufenwechsel. `
        + `Abhilfe: in CfgVehicles einen Block Food > FoodStageTransitions mit den erlaubten `
        + `Uebergaengen anlegen (FoodStage.c:167, kein Skript noetig).`);
      continue;
    }

    if (stages && !transitions) {
      f.warn(entry.file, entry.node.line,
        `"${cls}" hat Food > FoodStages, aber keine FoodStageTransitions. `
        + `Solange die Klasse nie in ein Kochgeraet kommt, ist das folgenlos; sobald sie es tut, `
        + `verbrennt sie beim ersten Wechsel (01 V4). Statisch ist "kommt sie hinein" nicht `
        + `entscheidbar - deshalb Warnung und nicht Fehler.`);
    }

    if (transitions && !stages) {
      f.warn(entry.file, entry.node.line,
        `"${cls}" deklariert FoodStageTransitions, aber keine FoodStages. `
        + `HasFoodStage() prueft genau "CfgVehicles ${cls} Food FoodStages" (ItemBase.c:2654); `
        + `ohne diesen Knoten entsteht gar kein FoodStage-Objekt und die Uebergaenge greifen nie.`);
    }

    // Ein essbares ChefZ-Item, dessen Skriptklasse NICHT von einer Nahrungsbasis
    // erbt, waere ein zweiter, stiller Fehlerweg: der ChefZ-Zustand lebt
    // ausschliesslich auf ChefZ-eigenen Klassen (06 §2, OF-12).
    const script = ctx.scripts.get(cls);
    if (script && stages) {
      const chain = scriptChainOf(cls, ctx.scripts);
      if (!chain.some(n => EDIBLE_SCRIPT_BASES.has(n))) {
        f.warn(script.file, script.line,
          `Skriptklasse "${cls}" traegt Food > FoodStages in der config.cpp, erbt aber von `
          + `"${script.parent || '(nichts)'}" statt von Edible_Base oder ChefZ_Edible_Base. `
          + `Ohne eine Edible-Basis gibt es kein FoodStage-Objekt und keinen ChefZ-Zustand (06 §2).`);
      }
    }
  }

  f.items.push({
    validator: 'chefzstage', severity: 'info', file: '', line: 0,
    summary: `Geprueft: ${checked} nahrungsbezogene Klassen von ${items.size} Item-Klassen; `
      + `${cookableScripts.size} Skriptklassen liefern CanBeCooked() == true.`,
  });

  return f;
}
