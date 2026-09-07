//==============================================================================
// ChefZ_MeatItemBase - Skriptbasis aller essbaren Klassen dieses Moduls.
//
// Andockregel woertlich aus dem Kopf von ChefZ_Edible_Base.c:
//
//     config.cpp   class ChefZ_X : Edible_Base { ... };      (VANILLA-Basis)
//     Skript       class ChefZ_X extends ChefZ_Edible_Base { }
//
// Genau EINE Skriptklasse fuer alle 21 Items, und das ist kein Sparzwang:
// DayZ sucht zu einer Configklasse die gleichnamige Skriptklasse und geht,
// wenn es keine gibt, die CONFIG-Elternkette hinauf. Jede Configklasse dieses
// Moduls erbt von ChefZ_MeatItemBase - also findet die Engine fuer jede von
// ihnen diese Klasse hier. So macht es Vanilla mit seinen Steaks auch.
//
// Sie ist leer, und auch das ist Absicht. Was eine Wurst IST - Kategorien,
// Tags, Zustand, Naehrwert - steht in Daten (Config/Ingredients/Meat.json und
// der config.cpp), nicht in Code. Jede Zeile hier waere Content, der sich
// nicht mehr ueber eine Datei aendern liesse.
//
// Ohne diese Ableitung traegt kein Item des Moduls einen ChefZ-Zustand; es
// waere dann gewoehnliche Vanilla-Nahrung - kein Fehler, nur weniger
// (ChefZ_Edible_Base.c, Kopf).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_MeatItemBase extends ChefZ_Edible_Base
{
    /**
     * Fleisch sagt, dass es Fleisch ist. Object.IsMeat() liefert sonst false.
     *
     * Das ist keine Zierde, sondern die Bedingung, unter der Vanilla die
     * Sonderregeln fuer ROHES Fleisch ueberhaupt anwendet:
     *
     *   ActionEatMeat.ApplyModifiers   "IsMeat() && IsFoodRaw()" -> blutige
     *                                  Haende ueber PluginLifespan
     *   Edible_Base.ProcessDecay       der Fleischzweig mit
     *                                  DECAY_FOOD_RAW_MEAT / BOILED / BAKED /
     *                                  DRIED. Ohne die Zusage faellt jedes
     *                                  Item dieses Moduls in den letzten
     *                                  Zweig ("opened cans") und bekommt statt
     *                                  einer Garstufe irgendwann still
     *                                  eAgents.FOOD_POISON eingesetzt.
     *
     * Das Krankheitsrisiko selbst wird NICHT hier gebaut. Vanilla loest es
     * ueber nutrition_properties[5] der jeweiligen Garstufe, und die
     * config.cpp dieses Moduls setzt dort bereits agents = 4
     * (eAgents.SALMONELLA) auf Raw und 16 (FOOD_POISON) auf Rotten. Code, der
     * dasselbe noch einmal behauptete, waere eine zweite Wahrheit.
     *
     * Alle 22 Vanillaklassen, die ActionEatMeat registrieren - von
     * BearSteakMeat.c bis Lard.c - ueberschreiben IsMeat(). Es gibt keine
     * Ausnahme; die Aktion und die Zusage gehoeren zusammen.
     */
    override bool IsMeat()
    {
        return true;
    }

    /**
     * Die Essaktion. Vanilla setzt sie NICHT auf Edible_Base, sondern auf
     * jeder Nahrungsklasse einzeln (Lard.c:36-42). Ohne diese Zeilen bietet
     * das Spiel das Item nicht zum Essen an - ohne Fehlerbild und ohne
     * Logzeile.
     *
     * Sie steht auf der FAMILIENBASIS und nicht an 21 Einzelklassen: die
     * Engine sucht zu einer Configklasse die gleichnamige Skriptklasse und
     * geht sonst die Config-Elternkette hinauf. Jede Klasse dieses Moduls erbt
     * von ChefZ_MeatItemBase und findet die Aktion damit hier.
     *
     * ActionEatMeat und nicht ActionEatBig: die Fleischvariante bringt
     * ApplyModifiers mit (blutige Haende bei rohem Fleisch) und verbraucht
     * UAQuantityConsumed.EAT_NORMAL statt EAT_BIG - ein Stueck Hack ist kein
     * Teller Eintopf. Vorbild: BearSteakMeat.c, PigSteakMeat.c, Lard.c.
     *
     * ActionForceFeed gehoert dazu, weil in Vanilla ueberall dort, wo selbst
     * gegessen wird, auch gefuettert werden kann.
     *
     * Vanillas Darm (Guts.c) nimmt zwar ActionEatBig, ist dort aber ein
     * Angelkoeder mit Sonderrolle. Innerhalb dieser Familie ist die
     * Fleischvariante die ehrlichere Antwort.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatMeat);
    }
}
