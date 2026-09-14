//==============================================================================
// ### SLICE fish-sea-a ###  Skriptklassen der Salzwasserfische A.
//
// Andockregel aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/State/
// ChefZ_Edible_Base.c:
//
//     config.cpp   class ChefZ_Cod : Edible_Base { ... };   (Vanilla-Basis)
//     Skript       class ChefZ_Cod extends ChefZ_Edible_Base { }
//
// Ohne diese Ableitung traegt das Item keinen ChefZ-Zustand - es waere ein
// gewoehnliches Vanilla-Nahrungsmittel. Kein Fehler, nur weniger.
//
// Kein modded class, kein Override an den zwoelf Einzelklassen: der gesamte
// Zustands-, Frische- und Verderbpfad liegt in ChefZ_Edible_Base, alles
// Gemeinsame auf der Basis unten. Was an den Klassen steht, ist ausschliesslich
// die Bindung.
//
// KEINE TERJE-REFERENZ. Dieses Modul kennt weder ActionFishingNew noch eine
// Fertigkeit. Der ChefZ-Haken sitzt im Fangkontext des Core (Gewicht je Fisch),
// nicht in der Aktion - beides kann nebeneinander laufen.
//
// Die MAKRELE fehlt hier mit Absicht: sie ist Vanillas "Mackerel" mit Vanillas
// Skriptklasse. Eine ChefZ-Ableitung waere eine zweite Klasse gleichen Namens.
//
// Layer: 4_World.
//==============================================================================

//! Gemeinsame Skriptbasis der ganzen Fische dieses Slice. Entspricht der
//! Configklasse gleichen Namens (scope = 0, sie ist selbst kein Item).
class ChefZ_SaltwaterFishA_Base extends ChefZ_Edible_Base
{
    /**
     * Der Fisch sagt, dass er Fleisch ist. Object.IsMeat() liefert sonst false.
     *
     * Das ist keine Zierde, sondern die Bedingung, unter der Vanilla die
     * Sonderregeln fuer ROHES Fleisch anwendet:
     *
     *   Edible_Base.ProcessDecay     der Fleischzweig mit DECAY_FOOD_RAW_MEAT /
     *                                BOILED / BAKED / DRIED. Ohne die Zusage
     *                                faellt das Item in den letzten Zweig
     *                                ("opened cans") und bekommt statt einer
     *                                Garstufe irgendwann still
     *                                eAgents.FOOD_POISON eingesetzt.
     *   ActionEatMeat.ApplyModifiers "IsMeat() && IsFoodRaw()" -> blutige
     *                                Haende. Wer einen rohen Fisch isst, hat sie.
     *
     * Vanillas MackerelFilletMeat.c macht dasselbe. Das Krankheitsrisiko wird
     * NICHT hier gebaut: es steht als agents = 4 (eAgents.SALMONELLA) in den
     * nutrition_properties der Garstufe Raw. Code, der dasselbe noch einmal
     * behauptete, waere eine zweite Wahrheit.
     */
    override bool IsMeat()
    {
        return true;
    }

    /**
     * Ein GANZER Fisch geht nicht auf den Stock - und seit E-05 auch nicht
     * in Pfanne oder Topf. Die Portion ist das Filet.
     *
     * Der Kopf von ChefZ_Edible_Base verlangt diese Antwort ausdruecklich an
     * der Content-Klasse: eine abgeleitete Zusage waere geraten. Vanillas
     * Mackerel.c sagt an derselben Stelle ebenfalls false.
     */
    /**
     * E-05 (Gate fishing, 14.09.2026): der ganze Fisch kommt NICHT in Topf
     * oder Pfanne - erst das Filet ist kochbar. Die Config traegt seit
     * demselben Tag weder FoodStages noch Uebergaenge; hier steht die
     * Entscheidung trotzdem als Aussage, wie bei ChefZ_FreshwaterFish_Base.
     */
    override bool CanBeCooked()
    {
        return false;
    }

    override bool CanBeCookedOnStick()
    {
        return false;
    }

    /**
     * Die Essaktion. Vanilla setzt sie NICHT auf Edible_Base, sondern auf jeder
     * Nahrungsklasse einzeln (MackerelFilletMeat.c:24-31). Ohne diese Zeilen
     * bietet das Spiel das Item nicht zum Essen an - ohne Fehlerbild und ohne
     * Logzeile.
     *
     * Sie steht auf der Familienbasis und nicht an zwoelf Einzelklassen: die
     * Engine sucht zu einer Configklasse die gleichnamige Skriptklasse und geht
     * sonst die Config-Elternkette hinauf.
     *
     * ActionEatMeat und nicht ActionEatBig: die Fleischvariante bringt
     * ApplyModifiers mit und verbraucht UAQuantityConsumed.EAT_NORMAL. Ein
     * ganzer Fisch ist kein Teller Eintopf, sondern viele Bissen.
     * ActionForceFeed gehoert dazu, weil in Vanilla ueberall dort, wo selbst
     * gegessen wird, auch gefuettert werden kann.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatMeat);
    }
}

// --- Die zwoelf Arten. Reine Bindung, kein Verhalten. ------------------------

class ChefZ_Plaice extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_GiltheadBream extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_Halibut extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_Cod extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_Sole extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_SeaBass extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_RedMullet extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_Conger extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_SeaEel extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_HorseMackerel extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_JapaneseSardine extends ChefZ_SaltwaterFishA_Base {}
class ChefZ_Rockfish extends ChefZ_SaltwaterFishA_Base {}
