//==============================================================================
// ChefZ_ActionRegistration - ohne diese Datei existieren die Core-Aktionen nicht
//
// ---------------------------------------------------------------------------
// DER BELEG, 29.08.2026
// ---------------------------------------------------------------------------
// ActionConstructor.RegisterActions() ist eine von Hand gepflegte Liste
// (scripts - 1.29, 4_World/.../ActionConstructor.c:27). ConstructActions()
// instanziiert AUSSCHLIESSLICH, was darin steht, und legt es in
// m_ActionNameActionMap ab. ActionManagerBase.GetAction(typename) liest nur
// diese Karte (ActionManagerBase.c:196-202) und liefert sonst null.
//
// Folge: eine Aktionsklasse, die hier nicht eingetragen ist, laesst sich auch
// mit AddAction() nicht an ein Item haengen. Sie kompiliert, sie ist im Log
// unsichtbar, und sie erscheint nie im Spiel.
//
// So lagen BEIDE Core-Aktionen seit ihrer Entstehung: die Entnahmeaktion aller
// portionierten Gerichte und die Verarbeitung an der Station - die zwei
// Aktionen, auf die sich ChefZ_PortionSpec, ChefZ_ContainerDef,
// ChefZ_ToolGroupDef und ChefZ_ProcessingManager namentlich berufen. Sie waren
// im Spiel nie ausloesbar. Der Validator "chefzaction" prueft es seither.
//
// Das Muster ist von zehn Expansion-Modulen uebernommen, die dieselbe Klasse
// auf demselben Server erweitern (DayZExpansion/*/Classes/.../ActionConstructor.c).
//
// Layer: 4_World. Kein Terje-Bezug (Regel 1), kein Content (Regel 2) - eine
// Registrierung ist beides nicht.
//==============================================================================

modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ChefZ_ActionTakePortion);
        actions.Insert(ChefZ_ActionProcessAtStation);
    }
}
