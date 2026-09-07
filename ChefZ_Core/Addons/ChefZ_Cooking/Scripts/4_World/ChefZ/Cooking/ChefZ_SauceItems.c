//==============================================================================
// ChefZ_SauceItemBase - Skriptbasis der Bruehen und Saucen.
//
// Slice "sauces". Production Map §52-§55, DME-Plan §34.
//
// Andockregel woertlich aus dem Kopf von ChefZ_Edible_Base.c:
//
//     config.cpp   class ChefZ_X : Marmalade { ... };      (VANILLA-Basis)
//     Skript       class ChefZ_X extends ChefZ_Edible_Base { }
//
// Genau EINE Skriptklasse fuer alle vier Items, und das ist kein Sparzwang:
// DayZ sucht zu einer Configklasse die gleichnamige Skriptklasse und geht,
// wenn es keine gibt, die CONFIG-Elternkette hinauf. Jede Configklasse dieses
// Slice erbt von ChefZ_SauceItemBase - also findet die Engine fuer jede von
// ihnen diese Klasse hier.
//
// Sie ist leer, und auch das ist Absicht. Was eine Sauce IST - Kategorien,
// Tags, Zustand, Naehrwert - steht in Daten (CfgChefZIngredients in der
// config.cpp und Config/Recipes/Sauces.json), nicht in Code. Jede Zeile hier
// waere Content, der sich nicht mehr ueber eine Datei aendern liesse.
//
// Ohne diese Ableitung traegt keine der vier Klassen einen ChefZ-Zustand; sie
// waeren gewoehnliche Vanilla-Nahrung (ChefZ_Edible_Base.c, Kopf). Fuer die
// Saucen waere das besonders teuer: sie sind Eingang der Gerichte der zweiten
// Welle, und deren Ergebnis schreibt Frische und Zustand fort.
//
// KEIN CanBeCooked()-Override: ob eine Sauce im Topf weitergart, entscheidet
// Vanilla anhand des FoodStages-Knotens der config.cpp - dort, wo auch die
// Uebergaenge stehen. Zwei Quellen fuer dieselbe Aussage waeren zwei
// Gelegenheiten, sie unterschiedlich falsch zu beantworten.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_SauceItemBase extends ChefZ_Edible_Base
{
    /**
     * Die Essaktion der vier Bruehen und Saucen.
     *
     * Vanilla registriert sie auf jeder Nahrungsklasse einzeln; ohne sie wird
     * das Glas im Spiel nicht zum Essen angeboten, ohne dass irgendwo etwas
     * gemeldet wuerde.
     *
     * ActionEatBig, und das Vorbild steht schon in der config.cpp: die
     * Configklasse erbt von Marmalade, und Marmalade.c registriert genau
     * ActionForceFeed + ActionEatBig. Ein Glas Eingekochtes ist der Fall, fuer
     * den Vanilla EAT_BIG vorgesehen hat - die Klassen tragen varQuantityMax
     * 100, ein Loeffelmass waere hier eine Ewigkeit.
     *
     * Keine Gattungszusage: eine eingekochte Sauce ist weder Fleisch noch
     * Obst, und Marmalade sagt ebenfalls nichts dazu.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatBig);
    }
}
