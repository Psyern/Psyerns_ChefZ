//==============================================================================
// ChefZ_FreshwaterFish_Base - Skriptbasis der acht Suesswasser- und
// Wanderfische des Slice "fish-fresh".
//
// Andockregel woertlich aus dem Kopf von ChefZ_Edible_Base.c:
//
//     config.cpp   class ChefZ_X : Edible_Base { ... };      (VANILLA-Basis)
//     Skript       class ChefZ_X extends ChefZ_Edible_Base { }
//
// GENAU EINE Skriptklasse fuer alle acht Fische, und das ist kein Sparzwang:
// DayZ sucht zu einer Configklasse die gleichnamige Skriptklasse und geht,
// wenn es keine gibt, die CONFIG-Elternkette hinauf. Jede der acht
// Configklassen erbt von ChefZ_FreshwaterFish_Base - also findet die Engine
// fuer jede von ihnen diese Klasse hier.
//
// Was ein Fisch IST - Kategorie, Tags, Zustand, Fanggewicht, Gewaesser,
// Filetzahl - steht in Daten (Config/Fishing/Fish_Fresh.json,
// Config/Ingredients/Fish_Fresh.json, Config/Processing/Fillet_Fresh.json),
// nicht in Code. Ein neuer Fisch braucht deshalb keine Zeile hier und keine im
// Core: er ist eine Configklasse plus drei Datensaetze.
//
// KEIN Terje-Bezeichner, kein Aufruf, kein Schalter. Terje ueberschreibt
// ActionFishingNew.TrySpawnCatch (Fangchance, Bonusfisch, XP) - also die
// AKTION. Dieser Slice liefert Daten fuer den KONTEXT (Gewicht je Fisch).
// Beides laeuft nebeneinander, und keine Seite kennt die andere.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_FreshwaterFish_Base extends ChefZ_Edible_Base
{
    /**
     * Ein ganzer Fisch ist ein KADAVER, kein Gericht.
     *
     * Vanilla sagt das an seinen eigenen Fischen woertlich (scripts - 1.29,
     * 4_World/DayZ/Entities/ItemBase/Edible_Base/Mackerel.c und Carp.c: vier
     * Ueberschreibungen, Zeile fuer Zeile dieselben). Die drei Zusagen hier
     * sind die gleichen, und sie haengen zusammen:
     *
     *   CanBeCooked / CanBeCookedOnStick   false - der ganze Fisch kommt nicht
     *       in den Topf und nicht auf den Stock. Deshalb traegt keine der acht
     *       Configklassen einen Knoten Food > FoodStages oder
     *       FoodStageTransitions: Garstufen ohne Kochbarkeit waeren toter Text
     *       (01 V4). Gekocht wird das FILET, und das ist eine andere Klasse.
     *
     *   IsCorpse   true - ein toter Fisch ist ein Kadaver. Vanilla haengt
     *       daran unter anderem das Verhalten beim Ablegen und die Behandlung
     *       durch die Umwelt.
     *
     * Die Basis ChefZ_Edible_Base beantwortet CanBeCooked() sonst RECHNEND
     * ("deklariert die Configklasse FoodStageTransitions?"). Das Ergebnis waere
     * hier ohnehin false - aber es waere ein Rechenergebnis und keine Aussage.
     * Ein "return false;" ist eine ENTSCHEIDUNG und als solche greppbar.
     */
    override bool CanBeCooked()
    {
        return false;
    }

    override bool CanBeCookedOnStick()
    {
        return false;
    }

    override bool IsCorpse()
    {
        return true;
    }

    /**
     * Fisch verdirbt. Vanillas Mackerel und Carp sagen dasselbe; ohne die
     * Zusage bliebe ein Fang beliebig lange frisch, und die gesamte
     * Konservierungskette (Salzen, Raeuchern, Trocknen) haette am ganzen Fisch
     * nichts zu tun.
     */
    override bool CanDecay()
    {
        return true;
    }

    /**
     * Fisch sagt, dass er Fleisch ist. Object.IsMeat() liefert sonst false.
     *
     * Das ist die Bedingung, unter der Vanilla die Sonderregeln fuer ROHES
     * Fleisch ueberhaupt anwendet - vor allem Edible_Base.ProcessDecay, das
     * ohne die Zusage in den letzten Zweig ("opened cans") faellt und dem Item
     * still eAgents.FOOD_POISON einsetzt, statt es normal verderben zu lassen.
     *
     * Das Krankheitsrisiko selbst wird NICHT hier gebaut: die config.cpp setzt
     * an jeder der acht Klassen agents = 4 (eAgents.SALMONELLA). Code, der
     * dasselbe noch einmal behauptete, waere eine zweite Wahrheit.
     */
    override bool IsMeat()
    {
        return true;
    }

    /**
     * Die Essaktion. Vanilla setzt sie NICHT auf Edible_Base, sondern auf jeder
     * Nahrungsklasse einzeln (Lard.c:36-42, Potato.c). Ohne diese Zeilen bietet
     * das Spiel den Fisch nicht zum Essen an - ohne Fehlerbild und ohne
     * Logzeile.
     *
     * ActionEatMeat und nicht ActionEatBig: die Fleischvariante bringt
     * ApplyModifiers mit (blutige Haende bei rohem Fleisch) und verbraucht
     * UAQuantityConsumed.EAT_NORMAL. Ein roher Fisch ist kein Teller Eintopf.
     *
     * ActionForceFeed gehoert dazu, weil in Vanilla ueberall dort, wo selbst
     * gegessen wird, auch gefuettert werden kann.
     *
     * Dass ein roher Fisch essbar ist, ist Absicht und nicht empfehlenswert:
     * agents = 4 an jeder Klasse macht daraus ein Salmonellenrisiko. Der Weg
     * ohne Risiko ist PROCESS_FILLET_FISH und danach die Pfanne.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatMeat);
    }
}

//------------------------------------------------------------------------------
// Die acht Blaetter. Leer, und das ist der Punkt: sie existieren nur, damit
// jede Configklasse eine gleichnamige Skriptklasse HAT - die Engine faende
// sonst die Basis, was hier zum selben Ergebnis fuehrte, aber unsichtbar
// bliebe. Jede Zeile Verhalten mehr waere Content in Code.
//------------------------------------------------------------------------------

class ChefZ_Perch extends ChefZ_FreshwaterFish_Base {}

class ChefZ_Muskellunge extends ChefZ_FreshwaterFish_Base {}

class ChefZ_Snakehead extends ChefZ_FreshwaterFish_Base {}

class ChefZ_RainbowTrout extends ChefZ_FreshwaterFish_Base {}

class ChefZ_Char extends ChefZ_FreshwaterFish_Base {}

class ChefZ_Salmon extends ChefZ_FreshwaterFish_Base {}

class ChefZ_SockeyeSalmon extends ChefZ_FreshwaterFish_Base {}

class ChefZ_Eel extends ChefZ_FreshwaterFish_Base {}
