//==============================================================================
// ChefZ_ActionRegistration - ohne diese Datei existiert die Aktion nicht
//
// ActionConstructor.RegisterActions() ist eine von Hand gepflegte Liste
// (scripts - 1.29, 4_World/.../ActionConstructor.c:27). ConstructActions()
// instanziiert ausschliesslich, was darin steht; ActionManagerBase.GetAction()
// liefert fuer alles andere null (ActionManagerBase.c:196-202), womit auch
// AddAction() am Item ins Leere greift.
//
// Eine nicht eingetragene Aktion uebersetzt fehlerfrei, meldet nichts und
// erscheint nie. Genau so lag ChefZ_ActionOpenCookbook hier, bis der Validator
// "chefzaction" es am 29.08.2026 sichtbar gemacht hat.
//
// Zwei Erweiterungen dieser Klasse aus zwei ChefZ-PBOs sind unbedenklich: zehn
// Expansion-Module tun auf demselben Server dasselbe.
//
// Layer: 4_World. Keine Dabs-Referenz.
//==============================================================================

// SCOUT-GEPRUEFT 2026-08-30 (chefz-conflict-scout)
// super zuerst, danach nur Insert; kein eigenes Member. 16 Expansion- und
// 4 Terje-Module ketten additiv auf derselben Klasse.
modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ChefZ_ActionOpenCookbook);
    }
}
