// chefzsym - jede Symbolreferenz gegen die gemergten Registries.
//
// AUFLAGE, KEINE ZUGABE. Entwurf 03 E1 und OF-11:
//
//   "Der Preis, ohne Beschoenigung: kein Compilezeit-Schutz. Ein Tippfehler
//    SAUSGE faellt erst zur Laufzeit auf. Deshalb ist der statische Validator
//    keine Zugabe, sondern Auflage. Wird die Auflage nicht erfuellt, ist dieser
//    Entwurf schlechter als ein enum-basierter."
//
// Genau das leistet diese Datei: sie ist der Ersatz fuer den Compiler, den
// Symbole nicht haben. Ein Enum haette den Tippfehler beim Uebersetzen
// gefunden; hier findet ihn der Validator vor dem Serverstart.
//
// Aufbau:
//   1. Registries mergen           chefzdata.registries()  (Rang 1 + Rang 2 + Deltas)
//   2. Referenzen einsammeln       REFERENZTABELLE unten
//   3. jede Referenz aufloesen     unbekannt -> Befund
//
// Zwei Sorten von Namensraeumen, und der Unterschied ist wichtig:
//
//   OFFEN      Kategorie, Tag, Zustand, Qualitaet, Prozess ... - der Inhalt
//              kommt aus Daten. Geprueft wird gegen die gemergten Registries.
//   GESCHLOSSEN completion, exec, when, scope, vanillaStage, Kochmethode ... -
//              der Inhalt steht im Core und kann sich nicht aendern. Geprueft
//              wird gegen die Liste, die der Core selbst fuehrt.
//
// Die Schwere folgt der Laufzeit, nicht dem Geschmack:
//   - unbekannte Kategorie/Tag/Zustand/Qualitaet in einem Selektor weist der
//     ChefZ_SelectorCompiler ab  -> FEHLER
//   - unbekannte Station in stationsAllowed laesst der ChefZ_ProcessCompiler
//     stehen und warnt (11 §7, "koennte aus einem optionalen Modul kommen")
//                                -> WARNUNG
//   - CoreSettings nennen Vorgaben, die erst ein Content-Modul definiert
//                                -> WARNUNG
//
// Ist ein Namensraum projektweit LEER, gibt es keinen Einzelbefund, sondern
// eine Zeile "nicht pruefbar". Sonst haette der Core, der bewusst kein Content
// mitbringt, bei jedem Lauf Fehler gegen sich selbst - und ein Validator, der
// im Normalzustand rot ist, wird nach zwei Wochen ignoriert.

import { Findings } from './lib.mjs';
import { allRecords, registries, NAMESPACES } from './chefzdata.mjs';

// --- Geschlossene Wertelisten des Core --------------------------------------
// Jede Liste hat genau eine Quelle im Code; sie steht dabei.

const CLOSED = {
  // ChefZ_VanillaStage.FromName - Zahlen aus FoodStage.c:1 (01 V4)
  vanillaStage: ['NONE', 'RAW', 'BAKED', 'BOILED', 'DRIED', 'BURNED', 'BURNT', 'ROTTEN'],
  // ChefZ_CookingHook.METHOD_* - CookingMethodType (Cooking.c:1), 08 §2
  method: ['NONE', 'BAKING', 'BOILING', 'DRYING', 'TIME'],
  // ChefZ_Completion
  completion: ['ON_STAGE', 'TIMED', 'INSTANT'],
  // ChefZ_ProcessExec
  exec: ['HANDCRAFT', 'STATION_ACTION', 'STATION_TIMED'],
  // ChefZ_CompiledSlot - "whole" | "amount" | "none"
  consume: ['whole', 'amount', 'none'],
  // ChefZ_ExtraItemsMode / ChefZ_CoreSettingsDef.IsValidExtraItems
  extraItems: ['forbid', 'ignore', 'consume'],
  // ChefZ_CoreSettingsDef.IsValidCapabilityMode
  capabilityMode: ['asAuthored', 'neverBlock', 'ignore'],
  // ChefZ_GradeWhen
  gradeWhen: ['slotFilled', 'slotCount', 'anyItem', 'allMatched', 'context', 'capability'],
  // ChefZ_GradeContextKey
  contextKey: [
    'deviceTemperature', 'liquidQuantity', 'elapsedSec', 'portionCapacity',
    'qualityModifier', 'itemsInVessel', 'boundItemCount', 'coverage',
  ],
  // ChefZ_CapabilityReq.onFail (17 §6)
  onFail: ['block', 'degrade', 'reduceYield'],
  // ChefZ_TransformDef.qualityRule (11 §4)
  qualityRule: ['MIN', 'MEAN', 'WEIGHTED_MEAN', 'MAX'],
  // ChefZ_OutputDef.quantityMode (08 §3)
  quantityMode: ['fixed', 'fromInput', 'ratio'],
  // ChefZ_NutritionScope (13 §4)
  nutritionScope: ['class', 'category', 'tag'],
  // ChefZ_PreservationScope (14 §5)
  preservationScope: ['state', 'class', 'category', 'tag', 'quality'],
};

/**
 * Welche geschlossene Liste die Schreibweise einebnet - und welche nicht.
 *
 * Nachgeschlagen, nicht geraten: ChefZ_ProcessExec.FromName, ChefZ_Completion.
 * FromName, ChefZ_VanillaStage.FromName und ChefZ_TransformDef.Normalize rufen
 * ToUpper(), ChefZ_ConsumeMode.FromName ruft ToLower(). Alles andere vergleicht
 * exakt - allen voran die Kochmethoden, die ueber ChefZ_SymbolTable.Intern
 * laufen und deshalb case-sensitiv sind (ChefZ_CookingHook, Kopf).
 *
 * Waere der Pruefer durchgehend nachsichtig, liesse er ein "Forbid" durch, das
 * die Laufzeit abweist - und waere er durchgehend streng, meldete er ein
 * "boiled", das sie akzeptiert. Beides untergraebt ihn.
 */
const CASE_FOLDED = new Set(['vanillaStage', 'completion', 'exec', 'qualityRule', 'consume']);

// Werte, die in einem Symbolfeld stehen duerfen, ohne ein Symbol zu sein.
const NOT_A_SYMBOL = new Set(['', 'AUTO']);

// Namensraeume ohne statisch pruefbare Registry. Sie werden bewusst NICHT
// geprueft, und der Grund steht jeweils dabei - Schweigen ohne Begruendung
// waere eine Luecke, die niemand mehr findet.
const UNCHECKED = {
  effect: 'Effekt-IDs sind opak (12 §3): der Core kennt sie nicht, ein Comp-Modul deutet sie.',
  capability: 'Faehigkeiten meldet ein Fremdmodul zur Laufzeit an (17 §4) - vor dem Serverstart existieren sie nicht.',
  event: 'Eventnamen sind offen (17 §2): jeder Anbieter darf eigene fuehren.',
  liquidType: 'Fluessigkeiten stehen in cfgLiquidDefinitions der Game-Config, nicht in einer ChefZ-Registry.',
};

// --- Referenzen einsammeln --------------------------------------------------

/**
 * Ein Sammler. Jede Referenz ist { ns, value, where, severity }.
 * ns ist entweder ein Namensraum aus NAMESPACES oder "closed:<liste>".
 */
class RefCollector {
  constructor(rec) { this.rec = rec; this.out = []; }

  ref(ns, value, where, severity = 'error') {
    if (typeof value !== 'string') return;
    const v = value.trim();
    if (NOT_A_SYMBOL.has(v)) return;
    this.out.push({ ns, value: v, where, severity });
  }

  refList(ns, list, where, severity = 'error') {
    if (!Array.isArray(list)) return;
    list.forEach((v, i) => this.ref(ns, v, `${where}[${i}]`, severity));
  }

  closed(list, value, where, severity = 'error') {
    if (typeof value !== 'string' || value.trim() === '') return;
    this.out.push({ ns: `closed:${list}`, value: value.trim(), where, severity });
  }

  closedList(list, values, where, severity = 'error') {
    if (!Array.isArray(values)) return;
    values.forEach((v, i) => this.closed(list, v, `${where}[${i}]`, severity));
  }

  /** Selektor, rekursiv (07 §2.1). */
  selector(sel, where) {
    if (!sel || typeof sel !== 'object' || Array.isArray(sel)) return;
    this.ref('category', sel.category, `${where}.category`);
    this.ref('tag', sel.tag, `${where}.tag`);
    this.ref('state', sel.state, `${where}.state`);
    this.ref('quality', sel.minQuality, `${where}.minQuality`);
    this.closed('vanillaStage', sel.vanillaStage, `${where}.vanillaStage`);
    // sel.cls ist ein Klassenname - dafuer ist classrefs.mjs zustaendig.
    // sel.liquidType siehe UNCHECKED.
    if (Array.isArray(sel.anyOf)) sel.anyOf.forEach((s, i) => this.selector(s, `${where}.anyOf[${i}]`));
    if (Array.isArray(sel.allOf)) sel.allOf.forEach((s, i) => this.selector(s, `${where}.allOf[${i}]`));
    if (sel.not) this.selector(sel.not, `${where}.not`);
  }

  /** Slot eines Rezepts oder Transforms (07 §3, 11 §4). */
  slot(slot, where) {
    if (!slot || typeof slot !== 'object') return;
    this.selector(slot.match, `${where}.match`);
    this.ref('unit', slot.unit, `${where}.unit`, 'warning');
    this.ref('state', slot.setStateAfter, `${where}.setStateAfter`);
    this.refList('state', slot.excludeStates, `${where}.excludeStates`, 'warning');
    this.closed('consume', slot.consume, `${where}.consume`, 'warning');
  }

  /** Ergebnisbeschreibung, in Rezept und Transform dieselbe (11 E4). */
  output(o, where) {
    if (!o || typeof o !== 'object') return;
    this.ref('state', o.setState, `${where}.setState`);
    this.ref('containerCategory', o.containerCategory, `${where}.containerCategory`);
    this.closed('quantityMode', o.quantityMode, `${where}.quantityMode`);
    this.refList('effect', o.effects, `${where}.effects`);
    if (Array.isArray(o.variants)) {
      o.variants.forEach((v, i) => this.ref('quality', v && v.tier, `${where}.variants[${i}].tier`));
    }
    // cls, portionClass, emptyOnLastPortion, returnContainer sind Klassennamen
    // -> classrefs.mjs.
  }
}

function collectRefs(rec) {
  const c = new RefCollector(rec);
  const o = rec.obj || {};

  switch (rec.kind) {
    case 'coreSettings': {
      // 02 §5: die Vorgaben des Core nennen Symbole, die erst ein Content-Modul
      // definiert. Fehlt eines, greift die Vorgabe nur nicht - deshalb Warnung.
      c.refList('state', o.defaultExcludedStates, 'defaultExcludedStates', 'warning');
      c.closed('extraItems', o.defaultExtraItems, 'defaultExtraItems');
      c.closed('capabilityMode', o.capabilityMode, 'capabilityMode');
      const qs = o.qualityScoring;
      if (qs && typeof qs === 'object') {
        c.ref('tierSet', qs.defaultTierSet, 'qualityScoring.defaultTierSet', 'warning');
        if (Array.isArray(qs.statePenalties)) {
          qs.statePenalties.forEach((p, i) =>
            c.ref('state', p && p.state, `qualityScoring.statePenalties[${i}].state`, 'warning'));
        }
      }
      break;
    }
    case 'category':
      c.ref('category', o.parent, 'parent');
      break;
    case 'state':
      c.refList('tag', o.implies, 'implies');
      c.closed('vanillaStage', o.projectsToVanillaStage, 'projectsToVanillaStage');
      break;
    case 'qualityTier':
      c.refList('tag', o.grantsTags, 'grantsTags');
      c.refList('effect', o.grantsEffects, 'grantsEffects');
      break;
    case 'toolGroup':
      c.refList('toolCategory', o.toolCategories, 'toolCategories');
      break;
    case 'device':
      c.refList('deviceCategory', o.deviceCategories, 'deviceCategories');
      break;
    case 'container':
      c.refList('containerCategory', o.containerCategories, 'containerCategories');
      break;
    case 'ingredient':
      c.refList('category', o.categories, 'categories');
      c.refList('tag', o.tags, 'tags');
      c.ref('state', o.defaultState, 'defaultState');
      c.ref('unit', o.quantityUnit, 'quantityUnit', 'warning');
      c.ref('containerCategory', o.containerCategory, 'containerCategory');
      break;
    case 'nutrition':
      c.closed('nutritionScope', o.scope, 'scope');
      scopedId(c, rec, o.scope, CLOSED.nutritionScope);
      break;
    case 'preservation':
      c.closed('preservationScope', o.scope, 'scope');
      scopedId(c, rec, o.scope, CLOSED.preservationScope);
      break;
    case 'process':
      c.closed('exec', o.exec, 'exec');
      c.refList('toolGroup', o.toolGroups, 'toolGroups');
      c.refList('event', o.emitEvents, 'emitEvents');
      break;
    case 'station':
      c.refList('process', o.processes, 'processes');
      c.refList('stationCategory', o.stationCategories, 'stationCategories');
      break;
    case 'transform':
      c.ref('process', o.process, 'process');
      c.refList('station', o.stationsAllowed, 'stationsAllowed', 'warning');
      c.closed('qualityRule', o.qualityRule, 'qualityRule');
      if (Array.isArray(o.inputs)) o.inputs.forEach((s, i) => c.slot(s, `inputs[${i}]`));
      if (Array.isArray(o.outputs)) o.outputs.forEach((x, i) => c.output(x, `outputs[${i}]`));
      if (Array.isArray(o.byproducts)) o.byproducts.forEach((x, i) => c.output(x, `byproducts[${i}]`));
      if (Array.isArray(o.requires)) {
        o.requires.forEach((r, i) => {
          c.ref('capability', r && r.capability, `requires[${i}].capability`);
          c.closed('onFail', r && r.onFail, `requires[${i}].onFail`);
        });
      }
      break;
    case 'recipe':
      c.closed('completion', o.completion, 'completion');
      c.closedList('vanillaStage', o.doneStages, 'doneStages');
      c.ref('tierSet', o.qualityTierSet, 'qualityTierSet');
      c.refList('toolGroup', o.requiredToolGroups, 'requiredToolGroups');
      c.refList('effect', o.effects, 'effects');
      c.refList('event', o.emitEvents, 'emitEvents');
      if (Array.isArray(o.contexts)) {
        o.contexts.forEach((ctx, i) => {
          if (!ctx || typeof ctx !== 'object') return;
          c.refList('deviceCategory', ctx.deviceCategories, `contexts[${i}].deviceCategories`);
          c.closedList('method', ctx.methods, `contexts[${i}].methods`);
          c.refList('liquidType', ctx.liquidTypes, `contexts[${i}].liquidTypes`);
        });
      }
      if (Array.isArray(o.slots)) o.slots.forEach((s, i) => c.slot(s, `slots[${i}]`));
      if (Array.isArray(o.outputs)) o.outputs.forEach((x, i) => c.output(x, `outputs[${i}]`));
      if (Array.isArray(o.byproducts)) o.byproducts.forEach((x, i) => c.output(x, `byproducts[${i}]`));
      if (o.policy && typeof o.policy === 'object') {
        c.closed('extraItems', o.policy.extraItems, 'policy.extraItems');
        c.refList('state', o.policy.forbiddenStates, 'policy.forbiddenStates');
        c.selector(o.policy.extraItemsAllowedIf, 'policy.extraItemsAllowedIf');
      }
      if (Array.isArray(o.gradeRules)) {
        o.gradeRules.forEach((g, i) => {
          if (!g || typeof g !== 'object') return;
          c.closed('gradeWhen', g.when, `gradeRules[${i}].when`);
          c.closed('contextKey', g.contextKey, `gradeRules[${i}].contextKey`);
          c.ref('capability', g.capability, `gradeRules[${i}].capability`);
          c.selector(g.selector, `gradeRules[${i}].selector`);
        });
      }
      if (Array.isArray(o.requires)) {
        o.requires.forEach((r, i) => {
          c.ref('capability', r && r.capability, `requires[${i}].capability`);
          c.closed('onFail', r && r.onFail, `requires[${i}].onFail`);
        });
      }
      break;
    default:
      break;
  }
  return c.out;
}

/**
 * Bei Nutrition und Preservation ist die ID selbst eine Referenz, und WOHIN sie
 * zeigt, sagt das Feld "scope" (13 §4, 14 §5). Ohne diese Aufloesung waere
 * "nutrition auf die Kategorie SAUSGE" ein Datensatz, der nie zieht und nie
 * auffaellt.
 */
function scopedId(c, rec, scope, valid) {
  const s = typeof scope === 'string' ? scope.trim().toLowerCase() : '';
  if (!valid.includes(s)) return;                 // ungueltiger scope: schon gemeldet
  if (s === 'class') return;                      // Klassenname -> classrefs.mjs
  const ns = { category: 'category', tag: 'tag', state: 'state', quality: 'quality' }[s];
  if (ns) c.ref(ns, rec.id, `id (scope "${s}")`);
}

// --- Der Pruefer ------------------------------------------------------------

export default function run() {
  const f = new Findings('chefzsym');
  const reg = registries();
  const records = allRecords();

  const usedButEmpty = new Map();   // ns -> Anzahl Referenzen
  const uncheckedSeen = new Set();

  for (const rec of records) {
    for (const r of collectRefs(rec)) {
      const at = `${rec.kind} "${rec.id}" -> ${r.where}`;

      // 1. Geschlossene Liste des Core
      if (r.ns.startsWith('closed:')) {
        const listName = r.ns.slice('closed:'.length);
        const list = CLOSED[listName];
        if (!list) continue;
        const folded = CASE_FOLDED.has(listName);
        const hit = list.some(v => v === r.value
          || (folded && v.toLowerCase() === r.value.toLowerCase()));
        if (!hit) {
          const nearMiss = !folded && list.some(v => v.toLowerCase() === r.value.toLowerCase());
          f.error(rec.file, rec.line,
            `${at}: "${r.value}" ist kein gueltiger Wert. Zulaessig: ${list.join(', ')}`
            + (nearMiss ? ' - hier zaehlt die Gross- und Kleinschreibung, der Core vergleicht exakt.' : ''));
        }
        continue;
      }

      // 2. Bewusst ungeprueft
      if (UNCHECKED[r.ns]) { uncheckedSeen.add(r.ns); continue; }

      // 3. Offener Namensraum gegen die gemergten Registries
      const table = reg[r.ns];
      if (!table) continue;
      if (table.size === 0) {
        usedButEmpty.set(r.ns, (usedButEmpty.get(r.ns) || 0) + 1);
        continue;
      }
      if (table.has(r.value)) continue;

      const label = NAMESPACES[r.ns] || r.ns;
      const hint = nearest(r.value, table);
      const msg = `${at}: ${label} "${r.value}" steht in keiner Registry`
        + (hint ? ` - meintest du "${hint}"?` : '')
        + ` (${table.size} bekannte Eintraege)`;
      if (r.severity === 'warning') f.warn(rec.file, rec.line, msg);
      else f.error(rec.file, rec.line, msg);
    }
  }

  // Leere Namensraeume: EINE Zeile statt hundert Fehlern.
  for (const [ns, count] of [...usedButEmpty.entries()].sort()) {
    f.warn(null, 0,
      `${NAMESPACES[ns] || ns}: ${count} Referenz(en), aber kein einziger Datensatz deklariert `
      + `diesen Namensraum - nicht pruefbar. Das ist normal, solange kein Content-Modul geladen ist.`);
  }

  for (const ns of [...uncheckedSeen].sort()) {
    f.items.push({
      validator: 'chefzsym', severity: 'info', file: '', line: 0,
      summary: `Nicht geprueft: ${ns} - ${UNCHECKED[ns]}`,
    });
  }

  const filled = Object.entries(reg).filter(([, m]) => m.size > 0)
    .map(([ns, m]) => `${ns}=${m.size}`).join(', ');
  f.items.push({
    validator: 'chefzsym', severity: 'info', file: '', line: 0,
    summary: `Geprueft: ${records.length} Datensaetze gegen `
      + (filled || 'lauter leere Registries - es gibt noch keinen Content'),
  });

  return f;
}

/** Naechster Nachbar nach Levenshtein - nur fuer die Meldung "meintest du". */
function nearest(value, table) {
  let best = null, bestD = Infinity;
  const max = Math.max(2, Math.floor(value.length / 3));
  for (const cand of table.keys()) {
    const d = levenshtein(value.toLowerCase(), cand.toLowerCase());
    if (d < bestD) { bestD = d; best = cand; }
  }
  return bestD <= max ? best : null;
}

function levenshtein(a, b) {
  if (a === b) return 0;
  const prev = new Array(b.length + 1);
  for (let j = 0; j <= b.length; j++) prev[j] = j;
  for (let i = 1; i <= a.length; i++) {
    let last = prev[0];
    prev[0] = i;
    for (let j = 1; j <= b.length; j++) {
      const tmp = prev[j];
      prev[j] = Math.min(prev[j] + 1, prev[j - 1] + 1, last + (a[i - 1] === b[j - 1] ? 0 : 1));
      last = tmp;
    }
  }
  return prev[b.length];
}
