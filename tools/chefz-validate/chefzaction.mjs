// chefzaction - eine Aktionsklasse, die niemand registriert, gibt es im Spiel nicht.
//
// ActionConstructor.RegisterActions() ist eine von Hand gepflegte Liste
// (scripts - 1.29, 4_World/DayZ/Classes/UserActionsComponent/ActionConstructor.c:27).
// ConstructActions() instanziiert ausschliesslich, was darin steht, und legt es
// in m_ActionNameActionMap ab. ActionManagerBase.GetAction(typename) liest nur
// diese Karte (ActionManagerBase.c:196-202) und liefert sonst null - womit auch
// AddAction() am Item ins Leere greift.
//
// Eine nicht eingetragene Aktion uebersetzt also fehlerfrei, taucht in keinem
// Protokoll auf und erscheint trotzdem nie. Dieselbe Sorte Stille wie bei
// chefzswitch: syntaktisch tadellos, wirkungslos.
//
// Gefunden am 28.08.2026: ChefZ_ActionTakePortion und ChefZ_ActionProcessAtStation
// lagen seit ihrer Entstehung so im Core - die zwei Aktionen, auf die sich
// ChefZ_PortionSpec, ChefZ_ContainerDef, ChefZ_ToolGroupDef und
// ChefZ_ProcessingManager namentlich berufen.

import { Findings, scriptFiles, readText, stripComments } from './lib.mjs';

// "class ChefZ_Foo : ActionSingleUseBase" und "class ChefZ_Foo extends
// ActionContinuousBase" - Enforce erlaubt beide Schreibweisen, und das Projekt
// benutzt beide. Absichtlich am NAMEN der Basis gemessen und nicht an der
// Vererbungskette: die Basen liegen in Vanilla, nicht im Projekt, eine Kette
// laesst sich hier also gar nicht aufloesen.
const ACTION_DECL = /^\s*class\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?::|extends)\s*(Action[A-Za-z0-9_]*Base[A-Za-z0-9_]*)/;

// "actions.Insert(ChefZ_ActionOpenCookbook);" - der Bezeichner, nicht ein String.
const INSERT = /\.Insert\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)/g;

// Callback-Klassen ("ChefZ_FooCB extends ActionContinuousBaseCB") werden NICHT
// registriert. Die Aktion nennt ihre Callback-Klasse selbst ueber
// m_CallbackClass; ActionConstructor sieht sie nie.
function istCallback(name, parent) {
  return name.endsWith('CB') || parent.endsWith('CB');
}

export default function run() {
  const f = new Findings('chefzaction');

  const files = scriptFiles();
  if (files.length === 0) return f;

  const declared = [];        // { name, file, line }
  const registered = new Set();

  for (const file of files) {
    const raw = readText(file);
    if (raw === null) continue;
    const text = stripComments(raw);

    text.split(/\r?\n/).forEach((line, i) => {
      const m = line.match(ACTION_DECL);
      if (!m) return;
      if (istCallback(m[1], m[2])) return;
      declared.push({ name: m[1], file, line: i + 1 });
    });

    // Eintraege zaehlen nur innerhalb einer ActionConstructor-Erweiterung.
    // Ein ".Insert(...)" irgendwo sonst registriert nichts.
    if (!/modded\s+class\s+ActionConstructor\b/.test(text)) continue;
    let m;
    while ((m = INSERT.exec(text)) !== null) registered.add(m[1]);
    INSERT.lastIndex = 0;
  }

  for (const a of declared) {
    if (registered.has(a.name)) continue;
    f.error(a.file, a.line,
      `Aktionsklasse "${a.name}" steht in keinem "modded class ActionConstructor". `
      + `ActionConstructor.RegisterActions() ist eine Handliste; ConstructActions() `
      + `instanziiert nur, was darin steht, und ActionManagerBase.GetAction() liefert `
      + `fuer alles andere null - AddAction() am Item greift damit ins Leere. Die Klasse `
      + `uebersetzt, meldet nichts und erscheint nie im Spiel. Abhilfe: `
      + `"modded class ActionConstructor { override void RegisterActions(TTypenameArray actions) `
      + `{ super.RegisterActions(actions); actions.Insert(${a.name}); } }" im 4_World-Layer `
      + `desselben Moduls.`);
  }

  return f;
}
