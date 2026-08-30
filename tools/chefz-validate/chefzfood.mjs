// Gemeinsame Grundlage von chefznut (01 V7) und chefzstage (01 V4):
// "ist diese Klasse Nahrung", "wo sind ihre Ergebnisklassen", "was steht in
// ihrem Food-Knoten".
//
// Beide Regeln entspringen demselben Befund: Vanilla entscheidet ueber
// Nahrungsmittel anhand der CONFIG, nicht anhand des Skripts.
//
//   PlayerStomach.c:208-250   registriert nur Klassen mit "Nutrition" ODER
//                             "Food" und scope != 0. Fehlt beides, verschwindet
//                             der Bissen still.            -> chefznut
//   ItemBase.c:2654           HasFoodStage() ist genau
//                             ConfigIsExisting("CfgVehicles %1 Food FoodStages")
//   FoodStage.c:472           GetNextFoodStageType faellt ohne passenden
//                             Uebergang auf BURNED zurueck. -> chefzstage
//
// Diese Datei enthaelt keine Regel, nur die Fakten dazu.

import { scriptClasses } from './lib.mjs';
import { allRecords, configItemIndex, configChain, resolveChild, resolveProp } from './chefzdata.mjs';

/**
 * Skriptbasen, deren Ableitungen essbar sind.
 * ChefZ_Edible_Base und ChefZ_PortionedFood_Base sind Ableitungen von
 * Edible_Base (06 §2, 15 E2) - der Core deklariert sie, ohne ein Item zu sein.
 */
export const EDIBLE_SCRIPT_BASES = new Set([
  'Edible_Base', 'ChefZ_Edible_Base', 'ChefZ_PortionedFood_Base',
]);

/**
 * Config-Basen, an denen man Nahrung ohne Skript erkennt.
 * Bewusst kurz: was hier fehlt, fuehrt zu einer WARNUNG ("nicht entscheidbar"),
 * nie zu einem falschen Fehler. Die Liste zu erweitern ist billig, ein
 * Fehlalarm teuer.
 */
export const EDIBLE_CONFIG_BASES = new Set([
  'Edible_Base', 'Bottle_Base', 'ChefZ_Edible_Base', 'ChefZ_PortionedFood_Base',
]);

/**
 * Config-Basen, an denen man das GEGENTEIL erkennt: sicher keine Nahrung.
 *
 * Beide Vanillaklassen erben im Skript direkt von ItemBase, nicht von
 * Edible_Base (GardenLime.c:1) - Vanillas Gartenkalk ist so wenig ein
 * Nahrungsmittel wie eine Holzkiste. Landet configChain bei einem dieser
 * Namen, ist die letzte projekteigene Klasse DIREKT davon abgeleitet und
 * lief nirgends ueber Edible_Base; sonst stuende hier "Edible_Base" als
 * external, so wie bei jeder essbaren Klasse des Projekts.
 *
 * Die Liste ist so kurz wie die essbare Gegenliste und aus demselben Grund:
 * was hier fehlt, kostet eine Warnung, die stehen bleiben darf. Was hier zu
 * viel steht, verschweigt einen echten Befund.
 */
export const INEDIBLE_CONFIG_BASES = new Set([
  'Inventory_Base', 'GardenLime',
]);

/**
 * Warum eine Klasse sicher KEINE Nahrung ist - oder null.
 *
 * Spiegelbild zu edibleEvidence(). Nur wenn beide null liefern, ist die
 * Frage wirklich offen, und nur dann lohnt eine Warnung.
 */
export function inedibleEvidence(className) {
  const { chain, external } = configChain(className);
  if (external && INEDIBLE_CONFIG_BASES.has(external)) {
    const last = chain.length > 0 ? chain[chain.length - 1].name : className;
    if (last === className) return `erbt in der config.cpp direkt von ${external} (keine Nahrungsbasis)`;
    return `config.cpp-Kette laeuft ueber ${last} nach ${external} (keine Nahrungsbasis)`;
  }
  return null;
}

/** Skript-Vererbungskette, solange sie im Projekt liegt. */
export function scriptChainOf(name, classes = scriptClasses()) {
  const chain = [];
  const seen = new Set();
  let cur = name;
  while (cur && !seen.has(cur)) {
    seen.add(cur);
    chain.push(cur);
    const entry = classes.get(cur);
    if (!entry) break;
    cur = entry.parent;
  }
  return chain;
}

/**
 * Warum eine Klasse als essbar gilt - oder null.
 * Die Begruendung wandert in die Meldung: ein Befund, der nicht sagt, warum er
 * zutrifft, kostet den Content-Autor eine halbe Stunde.
 */
export function edibleEvidence(className, ctx) {
  const { scripts, portionClasses, nutritionClasses } = ctx;

  const sChain = scriptChainOf(className, scripts);
  const sHit = sChain.find(n => EDIBLE_SCRIPT_BASES.has(n) && n !== className);
  if (sHit) return `Skriptklasse erbt von ${sHit}`;

  const { chain, external } = configChain(className);
  for (const link of chain) {
    if (link.name !== className && EDIBLE_CONFIG_BASES.has(link.name)) {
      return `config.cpp-Kette laeuft ueber ${link.name}`;
    }
  }
  if (external && EDIBLE_CONFIG_BASES.has(external)) {
    return `erbt in der config.cpp von ${external}`;
  }
  if (portionClasses.has(className)) return 'wird als portionClass ausgegeben und damit gegessen (15 §3)';
  if (nutritionClasses.has(className)) return 'hat einen Nutrition-Datensatz mit scope "class" (13 §4)';
  return null;
}

/** Sammelt einmal, was beide Pruefer brauchen. */
export function foodContext() {
  const scripts = scriptClasses();
  const portionClasses = new Set();
  const nutritionClasses = new Set();
  const resultClasses = new Map();     // Klasse -> [{rec, where}]
  const inputClasses = new Map();      // Klasse -> [{rec, where}]

  const addResult = (cls, rec, where) => {
    if (typeof cls !== 'string' || cls === '' || cls === 'AUTO') return;
    if (!resultClasses.has(cls)) resultClasses.set(cls, []);
    resultClasses.get(cls).push({ rec, where });
  };
  const addInput = (cls, rec, where) => {
    if (typeof cls !== 'string' || cls === '') return;
    if (!inputClasses.has(cls)) inputClasses.set(cls, []);
    inputClasses.get(cls).push({ rec, where });
  };

  const walkSelector = (sel, rec, where) => {
    if (!sel || typeof sel !== 'object') return;
    addInput(sel.cls, rec, `${where}.cls`);
    if (Array.isArray(sel.anyOf)) sel.anyOf.forEach((s, i) => walkSelector(s, rec, `${where}.anyOf[${i}]`));
    if (Array.isArray(sel.allOf)) sel.allOf.forEach((s, i) => walkSelector(s, rec, `${where}.allOf[${i}]`));
    if (sel.not) walkSelector(sel.not, rec, `${where}.not`);
  };

  const walkOutput = (o, rec, where) => {
    if (!o || typeof o !== 'object') return;
    addResult(o.cls, rec, `${where}.cls`);
    addResult(o.emptyOnLastPortion, rec, `${where}.emptyOnLastPortion`);
    if (typeof o.portionClass === 'string' && o.portionClass !== '') {
      portionClasses.add(o.portionClass);
      addResult(o.portionClass, rec, `${where}.portionClass`);
    }
    if (Array.isArray(o.variants)) {
      o.variants.forEach((v, i) => addResult(v && v.cls, rec, `${where}.variants[${i}].cls`));
    }
  };

  for (const rec of allRecords()) {
    const o = rec.obj || {};
    if (rec.kind === 'nutrition') {
      const scope = typeof o.scope === 'string' ? o.scope.trim().toLowerCase() : 'class';
      if (scope === 'class' && rec.id) nutritionClasses.add(rec.id);
    }
    if (rec.kind !== 'recipe' && rec.kind !== 'transform') continue;
    const slots = rec.kind === 'recipe' ? o.slots : o.inputs;
    if (Array.isArray(slots)) {
      slots.forEach((s, i) => s && walkSelector(s.match, rec, `${rec.kind === 'recipe' ? 'slots' : 'inputs'}[${i}].match`));
    }
    if (Array.isArray(o.outputs)) o.outputs.forEach((x, i) => walkOutput(x, rec, `outputs[${i}]`));
    if (Array.isArray(o.byproducts)) o.byproducts.forEach((x, i) => walkOutput(x, rec, `byproducts[${i}]`));
  }

  return { scripts, portionClasses, nutritionClasses, resultClasses, inputClasses, items: configItemIndex() };
}

/**
 * Hat die Klasse - eigen oder im Projekt geerbt - diesen Unterknotenpfad?
 *
 * Die Kette wird GLIEDWEISE geprueft, nicht nur bis zum ersten Glied mit
 * "Food": ein Kind darf den Food-Block der Elternklasse erben und darin eigene
 * FoodStageTransitions ergaenzen, und umgekehrt. Wer nur beim ersten Treffer
 * fuer "Food" nachsieht, meldet geerbte Uebergaenge als fehlend.
 */
export function hasNode(className, ...pathParts) {
  const { chain } = configChain(className);
  for (const link of chain) {
    let node = link.entry.node;
    let ok = true;
    for (const part of pathParts) {
      const next = node.childMap.get(part);
      if (!next) { ok = false; break; }
      node = next;
    }
    if (ok) return { node, owner: link };
  }
  return null;
}

export { configChain, resolveChild, resolveProp };
