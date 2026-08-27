//==============================================================================
// ChefZ_PreservedFood_Base - Skriptbasis aller essbaren Klassen dieses Moduls.
//
// Andockregel woertlich aus dem Kopf von ChefZ_Edible_Base.c:
//
//     config.cpp   class ChefZ_X : Edible_Base { ... };      (VANILLA-Basis)
//     Skript       class ChefZ_X extends ChefZ_Edible_Base { }
//
// Genau EINE Skriptklasse fuer alle acht Items. DayZ sucht zu einer
// Configklasse die gleichnamige Skriptklasse und geht, wenn es keine gibt, die
// CONFIG-Elternkette hinauf. Jede Configklasse dieses Moduls erbt von
// ChefZ_PreservedFood_Base - also findet die Engine fuer jede von ihnen diese
// Klasse hier.
//
// Sie ist leer, und auch das ist Absicht. Was ein Stueck Doerrfleisch IST -
// Kategorien, Tags, Zustand, Naehrwert, Haltbarkeit - steht in Daten
// (Config/Ingredients/Preservation.json, CfgChefZStates und den
// Preservation-Records der Registry), nicht in Code. Jede Zeile hier waere
// Content, der sich nicht mehr ueber eine Datei aendern liesse.
//
// Insbesondere steht hier KEIN Verfallscode. Die Verlangsamung ist eine
// Vorskalierung von delta in ChefZ_Edible_Base.ProcessDecay (14 §2) und
// braucht von einer Doerrwurst nichts weiter als ihren Zustand. Wer sie hier
// noch einmal schriebe, haette zwei Systeme, die behaupten, dasselbe Item
// verderbe - genau das, was 14 §4 ausschliesst.
//
// Ohne diese Ableitung traegt kein Item des Moduls einen ChefZ-Zustand; es
// waere dann gewoehnliche Vanilla-Nahrung - kein Fehler, nur weniger
// (ChefZ_Edible_Base.c, Kopf).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_PreservedFood_Base extends ChefZ_Edible_Base
{
    /**
     * Alle acht Items dieses Moduls sind Fleisch oder Fisch: gesalzen,
     * getrocknet oder geraeuchert. Konservieren aendert die Haltbarkeit, nicht
     * die Gattung - deshalb dieselbe Zusage wie auf ChefZ_MeatItemBase.
     *
     * Sie ist hier zusaetzlich wertvoll, weil Vanillas Fleischzweig in
     * Edible_Base.ProcessDecay als einziger die Stufe DRIED kennt
     * (DECAY_FOOD_DRIED_MEAT). Ohne IsMeat() liefe Doerrfleisch im Zweig
     * "opened cans", der die Stufe gar nicht betrachtet - und die Preservation
     * des Moduls skalierte eine Uhr, die fuer Trockenware nie gestellt wird.
     *
     * Vanillas Beleg fuer "getrocknetes Fleisch ist Fleisch": die
     * SteakMeat-Klassen tragen die Stufe DRIED in derselben Klasse, die
     * IsMeat() zusagt.
     */
    override bool IsMeat()
    {
        return true;
    }

    /**
     * Die Essaktion fuer alle acht Klassen des Moduls, auf der Familienbasis.
     *
     * Vanilla registriert sie auf jeder Nahrungsklasse einzeln (Lard.c:36-42);
     * ohne sie wird das Item im Spiel schlicht nicht zum Essen angeboten. Die
     * Engine findet diese Klasse fuer jede Erbin ueber die Config-Elternkette.
     *
     * ActionEatMeat, aus demselben Grund wie in ChefZ_Meat: Fleischvariante,
     * EAT_NORMAL, und ApplyModifiers greift, falls je ein Zustand dieses
     * Moduls roh ist. Vorbild: BearSteakMeat.c, Lard.c.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatMeat);
    }
}
