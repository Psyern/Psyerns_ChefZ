//==============================================================================
// Skriptklassen der Kraeuter-Ernteprodukte.
//
// Andockregel aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/State/
// ChefZ_Edible_Base.c:
//
//     config.cpp   class ChefZ_Parsley : Edible_Base { ... };   (Vanilla-Basis)
//     script       class ChefZ_Parsley extends ChefZ_Edible_Base { }
//
// Ohne diese Ableitung traegt das Item keinen ChefZ-Zustand - es waere ein
// gewoehnliches Vanilla-Nahrungsmittel. Kein Fehler, nur weniger.
//
// Kein modded class, kein Override, keine eigene Aktion: der gesamte
// Zustands-, Frische- und Verderbpfad liegt in ChefZ_Edible_Base. Was hier
// steht, ist ausschliesslich die Bindung.
//
// Layer: 4_World.
//==============================================================================

//! Gemeinsame Skriptbasis der frischen Kraeuter. Entspricht der Configklasse
//! gleichen Namens (scope = 0, sie ist selbst kein Item).
class ChefZ_FreshHerbBase extends ChefZ_Edible_Base
{
    /**
     * Frisches Pflanzengut sagt in Vanilla "IsFruit". Das ist keine botanische
     * Aussage, sondern die Schublade, in die Edible_Base.ProcessDecay
     * Kraeuter, Beeren, Pilze und Gemuese legt: der Obstzweig kennt als
     * einziger den Ausgang "trocknet statt zu verrotten"
     * (DECAY_FOOD_FRVG_DRIED_CHANCE) - und getrocknete Kraeuter sind der Sinn
     * dieses Moduls. Ohne die Zusage fielen alle sieben in den letzten Zweig
     * ("opened cans") und bekaemen still eAgents.FOOD_POISON eingesetzt.
     *
     * Vanillas Beleg: SambucusBerry.c, CaninaBerry.c und Cannabis.c - frisches
     * Pflanzengut ohne Garstufen - ueberschreiben alle drei IsFruit().
     */
    override bool IsFruit()
    {
        return true;
    }

    /**
     * Die Essaktion der sechs frischen Kraeuter.
     *
     * Vanilla registriert sie auf jeder Nahrungsklasse einzeln; ohne sie wird
     * das Buendel im Spiel nicht zum Essen angeboten, ohne dass irgendwo etwas
     * gemeldet wuerde.
     *
     * ActionEat und NICHT ActionEatFruit, obwohl IsFruit() oben true liefert:
     * genau diese Kombination benutzt Vanilla fuer kleines frisches
     * Pflanzengut - SambucusBerry.c:59-60, Cannabis.c:27-28,
     * GreenBellPepper.c und MushroomBase.c. ActionEatFruit ist die Variante
     * fuer ganze Fruechte und Knollen (Apple, Potato); ein Buendel Thymian ist
     * keins von beidem. Der Unterschied ist die Bissgroesse: EAT_NORMAL (15)
     * statt EAT_BIG (25).
     *
     * Frische Paprika war frueher Teil dieser Familie. Sie ist jetzt Vanillas
     * GreenBellPepper (Vanilla-Audit §2) und bringt ihre Essaktion selbst mit -
     * GreenBellPepper.c registriert ebenfalls ActionEat und nicht
     * ActionEatFruit. Die Familie bleibt dadurch einheitlich; es geht keine
     * Aktion verloren.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEat);
    }
}

class ChefZ_Parsley    extends ChefZ_FreshHerbBase {}
class ChefZ_Dill       extends ChefZ_FreshHerbBase {}
class ChefZ_Thyme      extends ChefZ_FreshHerbBase {}
class ChefZ_Rosemary   extends ChefZ_FreshHerbBase {}
class ChefZ_WildGarlic extends ChefZ_FreshHerbBase {}

//! Pfefferbeeren sind der Rohstoff der Pfefferkette (Production Map §16).
class ChefZ_PepperBerries extends ChefZ_FreshHerbBase {}

//! Frische Paprika hat KEINE eigene Klasse: das ist Vanillas GreenBellPepper
//! (Vanilla-Audit §2). Eingang der Trockenkette §15 ist deshalb ein
//! Vanilla-Item; einen ChefZ-Zustand kann es nicht tragen, es braucht ihn hier
//! auch nicht - TR_PaprikaToDried setzt den Zustand DRIED am AUSGANG
//! ChefZ_DriedPaprika, und der ist eine ChefZ-Klasse.
