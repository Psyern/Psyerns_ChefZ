// chefzproc - "HANDCRAFT hat hoechstens zwei Eingaenge" (01 V12).
//
// Der Befund, woertlich aus
// 4_World/DayZ/Classes/Recipes/RecipeBase.c:
//
//     const int MAX_NUMBER_OF_INGREDIENTS = 2;
//     const int MAXIMUM_RESULTS = 10;
//
// Ein Transform mit drei Eingaengen laesst sich als Vanilla-Craftrezept nicht
// registrieren. Ohne diesen Pruefer entstuende ein Prozess, der sich STUMM
// nicht registriert - der Spieler haelt beide Zutaten in der Hand, das
// Kontextmenue bleibt leer, und im Log steht nichts, wonach man suchen wuerde.
//
// Die Regeln sind nicht erfunden, sondern eins zu eins die Abweisungen aus
// ChefZ_GenericCraftRecipe.BuildFromDef() und ChefZ_ProcessCompiler:
//
//   inputs == 0            nichts zu verarbeiten
//   inputs  > 2            Vanillas Grenze                        (01 V12)
//   inputs == 1 ohne Werkzeug   Vanilla braucht ZWEI Zutaten - es gaebe nichts
//                               zum Kombinieren
//   inputs == 2 mit Werkzeug    das Werkzeug waere der dritte Zutatenplatz
//   Ergebnisse > 10        RecipeBase fuehrt genau zehn Plaetze
//
// Was zur Laufzeit ein ERROR ist, ist hier ein Fehler; was dort eine Warnung
// ist (Qualitaetsvarianten an einem HANDCRAFT-Ergebnis), ist hier eine Warnung.
// Zwei verschiedene Schweregrade fuer denselben Sachverhalt waeren der Anfang
// vom Ende der Glaubwuerdigkeit beider Seiten.

import { Findings } from './lib.mjs';
import { allRecords } from './chefzdata.mjs';

// Beide Zahlen stehen in RecipeBase.c und werden in
// ChefZ_ProcessingLimits.HANDCRAFT_MAX_INPUTS bzw. ueber RecipeBase selbst
// gespiegelt. Sie sind Engine-Vorgabe, nicht Geschmack.
const HANDCRAFT_MAX_INPUTS = 2;
const MAXIMUM_RESULTS = 10;

const HANDCRAFT = 'HANDCRAFT';

export default function run() {
  const f = new Findings('chefzproc');

  const processes = new Map();
  for (const rec of allRecords()) {
    if (rec.kind !== 'process' || !rec.id) continue;
    // Rang 1 (config.cpp) schlaegt Rang 2 nicht - fuer diese Pruefung genuegt
    // der erste Fund je ID; widerspruechliche Doppelungen meldet deltas.mjs.
    if (!processes.has(rec.id)) processes.set(rec.id, rec);
  }

  const transforms = allRecords().filter(r => r.kind === 'transform');
  if (transforms.length === 0) {
    f.items.push({
      validator: 'chefzproc', severity: 'info', file: '', line: 0,
      summary: 'Keine Transforms im Projekt - die HANDCRAFT-Grenze aus 01 V12 kann noch niemand verletzen.',
    });
    return f;
  }

  let handcraft = 0;

  for (const tr of transforms) {
    const o = tr.obj || {};
    const procId = typeof o.process === 'string' ? o.process.trim() : '';
    const proc = processes.get(procId);
    if (!proc) continue;                       // unbekannter Prozess: meldet chefzsym

    const exec = typeof proc.obj.exec === 'string' ? proc.obj.exec.trim().toUpperCase() : '';
    if (exec !== HANDCRAFT) continue;
    handcraft++;

    const inputs = Array.isArray(o.inputs) ? o.inputs.length : 0;
    const toolGroups = Array.isArray(proc.obj.toolGroups)
      ? proc.obj.toolGroups.filter(t => typeof t === 'string' && t.trim() !== '')
      : (typeof proc.obj.toolGroups === 'string' && proc.obj.toolGroups.trim() !== '' ? [proc.obj.toolGroups] : []);
    const hasTools = toolGroups.length > 0;
    const outputs = (Array.isArray(o.outputs) ? o.outputs.length : 0)
      + (Array.isArray(o.byproducts) ? o.byproducts.length : 0);

    const head = `Transform "${tr.id}" laeuft ueber den HANDCRAFT-Prozess "${procId}"`;

    if (inputs === 0) {
      f.error(tr.file, tr.line,
        `${head} und hat keinen Eingang - es gaebe nichts zu verarbeiten. `
        + `ChefZ_GenericCraftRecipe weist ihn ab, das Rezept erscheint nie im Kontextmenue.`);
    } else if (inputs > HANDCRAFT_MAX_INPUTS) {
      f.error(tr.file, tr.line,
        `${head} und hat ${inputs} Eingaenge. Vanillas RecipeBase kennt genau `
        + `MAX_NUMBER_OF_INGREDIENTS = ${HANDCRAFT_MAX_INPUTS} (01 V12) - der Transform laesst sich `
        + `nicht registrieren und faellt STILL aus. Abhilfe: den Prozess auf STATION_ACTION oder `
        + `STATION_TIMED umstellen; Stationen haben diese Grenze nicht (11 E1).`);
    } else if (inputs === 1 && !hasTools) {
      f.error(tr.file, tr.line,
        `${head}, hat einen Eingang und der Prozess nennt keine Werkzeuggruppe. `
        + `Vanillas Craftsystem kombiniert immer ZWEI Dinge - es gaebe nichts, womit der Spieler `
        + `den Eingang kombinieren koennte. Abhilfe: Werkzeuggruppe am Prozess nennen oder auf `
        + `STATION_ACTION umstellen.`);
    } else if (inputs === HANDCRAFT_MAX_INPUTS && hasTools) {
      f.error(tr.file, tr.line,
        `${head}, hat zwei Eingaenge UND der Prozess nennt eine Werkzeuggruppe `
        + `(${toolGroups.join(', ')}). Das Werkzeug belegt den zweiten Zutatenplatz - zusammen `
        + `waeren es drei, und Vanilla kennt zwei (01 V12). Abhilfe: Werkzeuggruppe streichen oder `
        + `auf STATION_ACTION umstellen.`);
    }

    // Ein HANDCRAFT-Transform, der eine Station nennt, wird von der
    // Handwerksbruecke abgewiesen (ChefZ_HandcraftBridge.FillOne: "der Transform
    // nennt stationsAllowed, sein Prozess ist aber HANDCRAFT"). Ohne diese Regel
    // sieht der Lauf gruen aus und das Rezept erscheint im Spiel trotzdem nie -
    // genau die Fehlerart, gegen die dieses Werkzeug gebaut ist.
    const stations = Array.isArray(o.stationsAllowed) ? o.stationsAllowed : [];
    if (stations.length > 0) {
      f.error(tr.file, tr.line,
        `${head}, nennt aber stationsAllowed (${stations.join(', ')}). Ein Handwerksschritt `
        + `laeuft ohne Station; ChefZ_HandcraftBridge weist den Transform beim Registrieren ab, `
        + `und das Rezept erscheint im Spiel nie - ohne Fehlermeldung. Abhilfe: stationsAllowed `
        + `streichen oder den Prozess auf STATION_ACTION umstellen.`);
    }

    if (outputs > MAXIMUM_RESULTS) {
      f.error(tr.file, tr.line,
        `${head} und nennt ${outputs} Ergebnisse. RecipeBase fuehrt genau `
        + `MAXIMUM_RESULTS = ${MAXIMUM_RESULTS} Plaetze (01 V12); alles darueber fehlte stillschweigend.`);
    }

    // Qualitaetsvarianten an einem HANDCRAFT-Ergebnis greifen nie: Vanillas
    // Craftsystem legt die Ergebnisklasse bei der REGISTRIERUNG fest, lange
    // bevor eine Stufe berechnet werden koennte (11 E3).
    for (const [listName, list] of [['outputs', o.outputs], ['byproducts', o.byproducts]]) {
      if (!Array.isArray(list)) continue;
      list.forEach((out, i) => {
        if (out && Array.isArray(out.variants) && out.variants.length > 0) {
          f.warn(tr.file, tr.line,
            `${head}: ${listName}[${i}] nennt Qualitaetsvarianten. Bei HANDCRAFT steht die `
            + `Ergebnisklasse schon bei der Registrierung fest - es entsteht immer die Basisklasse. `
            + `Fuer stufenabhaengige Ergebnisse ist STATION_ACTION die richtige Ausfuehrungsform.`);
        }
        if (out && typeof out.portionClass === 'string' && out.portionClass !== '') {
          f.warn(tr.file, tr.line,
            `${head}: ${listName}[${i}] ist ein Portionsgericht. Portionen entstehen am Kochgeraet `
            + `oder an der Station (15 §3); Vanillas Craftsystem kennt keinen Portionszaehler.`);
        }
      });
    }
  }

  f.items.push({
    validator: 'chefzproc', severity: 'info', file: '', line: 0,
    summary: `Geprueft: ${handcraft} HANDCRAFT-Transforms von ${transforms.length} Transforms `
      + `gegen ${processes.size} Prozesse.`,
  });

  return f;
}
