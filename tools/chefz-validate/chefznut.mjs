// chefznut - "Ein Gericht ohne Nutrition-Block saettigt lautlos nicht" (01 V7).
//
// Der Befund, woertlich aus PlayerStomach.c:208-250 (InitData):
//
//     scope = g_Game.ConfigGetInt(config_path + " " + child_name + " scope");
//     if (config_path == "CfgVehicles" && scope == 0)
//         should_check = 0;
//     if (should_check)
//     {
//         has_nutrition = g_Game.ConfigIsExisting(path + " Nutrition");
//         has_stages    = g_Game.ConfigIsExisting(path + " Food");
//         if (has_nutrition || has_stages)
//             RegisterItem(child_name, consumable_count++);
//     }
//
// und PlayerStomach.c:403:  if (GetIDFromClassname(class_name) == -1) return;
//
// Daraus folgt die Regel, die dieser Pruefer durchsetzt:
//
//   Jede essbare ChefZ-Ergebnis- und Portionsklasse MUSS in CfgVehicles
//   "class Nutrition" ODER "class Food" besitzen UND scope != 0 haben.
//
// Fehlt das, verschwindet der Bissen still: keine Saettigung, keine Energie,
// keine Fehlermeldung. Es ist der leiseste denkbare Content-Fehler und deshalb
// der gefaehrlichste - genau die Sorte Fehler, fuer die ein statischer Pruefer
// da ist.
//
// Was hier NICHT passiert: eine Klasse, die das Projekt nicht selbst
// deklariert (Vanilla-Item als Ergebnis), wird nicht beurteilt. Ihre Config
// liegt in den Game-Daten, nicht im Repo. Und eine Klasse, deren Essbarkeit
// statisch nicht entscheidbar ist, ergibt eine WARNUNG, keinen Fehler -
// falsche Fehler kosten mehr Vertrauen, als richtige Warnungen einbringen.

import { Findings } from './lib.mjs';
import { configItemIndex, moduleOf } from './chefzdata.mjs';
import { foodContext, edibleEvidence, inedibleEvidence, hasNode, resolveProp } from './chefzfood.mjs';

export default function run() {
  const f = new Findings('chefznut');
  const ctx = foodContext();
  const items = configItemIndex();

  if (ctx.resultClasses.size === 0) {
    f.items.push({
      validator: 'chefznut', severity: 'info', file: '', line: 0,
      summary: 'Keine Ergebnisklassen gefunden - es gibt noch kein Rezept und keinen Transform. Nichts zu pruefen.',
    });
    return f;
  }

  let checked = 0, external = 0;

  for (const [cls, uses] of [...ctx.resultClasses.entries()].sort()) {
    const use = uses[0];
    const at = `${use.rec.kind} "${use.rec.id}" -> ${use.where}`;

    if (!items.has(cls)) {
      // Vanilla- oder Fremdklasse als Ergebnis. Ob sie einen Nutrition-Block
      // hat, steht in den Game-Daten - classrefs.mjs prueft die Existenz, mehr
      // ist von hier aus nicht serioes zu sagen.
      external++;
      continue;
    }

    const entry = items.get(cls);
    const evidence = edibleEvidence(cls, ctx);
    const nutrition = hasNode(cls, 'Nutrition');
    const food = hasNode(cls, 'Food');
    const scope = resolveProp(cls, 'scope');
    const scopeValue = scope ? Number(scope.value) : null;

    // --- scope != 0 -------------------------------------------------------
    if (scope !== undefined && Number.isFinite(scopeValue) && scopeValue === 0) {
      const msg = `${at}: Ergebnisklasse "${cls}" hat scope = 0`
        + (scope.owner.name !== cls ? ` (geerbt von "${scope.owner.name}")` : '')
        + '. PlayerStomach.InitData ueberspringt jede CfgVehicles-Klasse mit scope 0 (01 V7) - '
        + 'das Item saettigt nie, ohne dass irgendetwas gemeldet wird. '
        + 'Ergebnisklassen brauchen scope 2 (oder 1, wenn sie nur ueber ein Rezept entstehen sollen).';
      if (evidence) f.error(entry.file, entry.node.line, msg);
      else f.warn(entry.file, entry.node.line, msg);
    }

    // --- Nutrition oder Food ---------------------------------------------
    if (!nutrition && !food) {
      const base = `${at}: Ergebnisklasse "${cls}" hat weder "class Nutrition" noch "class Food". `
        + 'PlayerStomach registriert sie damit nie (01 V7, PlayerStomach.c:208-250); '
        + 'GetIDFromClassname liefert -1 und AddToStomach kehrt still zurueck - der Bissen ist wirkungslos.';
      if (evidence) {
        f.error(entry.file, entry.node.line, `${base} Essbar, weil: ${evidence}.`);
      } else if (inedibleEvidence(cls)) {
        // Die Frage ist entschieden, nur andersherum: ein Bienenstock, ein
        // Teller und ein Sack Salz brauchen keinen Naehrwert. Kein Befund.
      } else {
        f.warn(entry.file, entry.node.line,
          `${base} Ob die Klasse ueberhaupt gegessen werden soll, ist statisch nicht entscheidbar `
          + `(keine Skriptbasis, keine bekannte Nahrungsbasis in der config.cpp). `
          + `Wenn sie es soll: Nutrition-Block ergaenzen. Wenn nicht: dieser Hinweis darf stehen bleiben.`);
      }
    }
    checked++;
  }

  // Portionsklassen sind IMMER essbar - sie entstehen beim Servieren einer
  // Portion (15 §3). Fehlt eine davon in der config.cpp, ist das ein Fehler
  // eigener Art: classrefs meldet die fehlende Klasse, hier faellt sie sonst
  // stillschweigend aus der Nutrition-Pruefung.
  for (const cls of [...ctx.portionClasses].sort()) {
    if (items.has(cls)) continue;
    f.warn(null, 0,
      `Portionsklasse "${cls}" ist im Projekt nicht deklariert - ihr Nutrition-Block ist damit `
      + `nicht pruefbar. Genau hier greift 01 V7 am haertesten: eine Portion ohne Nutrition `
      + `saettigt nicht und meldet nichts.`);
  }

  f.items.push({
    validator: 'chefznut', severity: 'info', file: '', line: 0,
    summary: `Geprueft: ${checked} Ergebnisklassen im Projekt, ${external} ausserhalb `
      + `(Vanilla/Fremdmod, nicht beurteilbar), ${ctx.portionClasses.size} davon Portionsklassen.`
      + (items.size ? ` Module: ${[...new Set([...items.values()].map(v => moduleOf(v.file)))].join(', ')}.` : ''),
  });

  return f;
}
