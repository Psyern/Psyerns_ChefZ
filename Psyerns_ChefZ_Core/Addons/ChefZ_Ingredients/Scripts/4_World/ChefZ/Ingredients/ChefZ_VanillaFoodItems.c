//==============================================================================
// ChefZ_VanillaFoodItems - die Skriptseite des Slice "vanilla-foods".
//
// Quelle: Vanilla-Audit §3 C (Zucchini) und §3 F (Beeren an der vorhandenen
// Trockenkette). Der Slice bindet ueberwiegend FREMDE Klassen - die brauchen
// kein Skript, sie stehen im JSON. Hier stehen nur die zwei EIGENEN Klassen,
// die dabei entstehen.
//
// Andockregel aus dem Kopf von ChefZ_Edible_Base.c: die CONFIGklasse erbt von
// einer Vanilla-Klasse, die SKRIPTklasse von ChefZ_Edible_Base.
//
// Layer: 4_World.
//==============================================================================

//! Zucchini + Knife -> ChefZ_ChoppedZucchini (TR_ChopZucchini).
//!
//! Leer, und das ist die Aussage: sie erbt Kochbarkeit, Zustand, Essaktion und
//! IsFruit() von ChefZ_ChoppedVegetableBase (Slice produce). Ein achtes
//! Schnittgut mit eigener Basis waere eine zweite Wahrheit ueber dieselbe
//! Frage.
class ChefZ_ChoppedZucchini extends ChefZ_ChoppedVegetableBase {}

//! Hagebutten und Holunderbeeren vom Trockenrahmen
//! (TR_CaninaBerriesToDried, TR_SambucusBerriesToDried).
//!
//! Eigene Skriptklasse und nicht ChefZ_ChoppedVegetableBase: getrocknete
//! Beeren sind kein Schnittgut, und ihre Configklasse traegt einen Uebergang,
//! den kein Schnittgut hat (Dried -> Boiled).
class ChefZ_DriedBerries extends ChefZ_Edible_Base
{
    /**
     * Hagebutte und Holunder sind Obst - fuer Vanilla dieselbe Schublade wie
     * Potato.c und SlicedPumpkin.c, die Obst UND Gemuese meint
     * (Edible_Base.c:755).
     *
     * PFLICHT und nicht Zierde: ActionEatFruit.ActionCondition prueft
     * "food_item.IsFruit()" und liefert sonst false. Die Aktion waere
     * registriert und erschiene trotzdem nie.
     *
     * Vanillas eigene Beeren sagen dasselbe zu (CaninaBerry.c,
     * SambucusBerry.c: override bool IsFruit() { return true; }).
     */
    override bool IsFruit()
    {
        return true;
    }

    /**
     * Vanilla setzt die Essaktion NICHT auf Edible_Base, sondern auf jeder
     * Nahrungsklasse einzeln; ohne sie wird das Item im Spiel schlicht nicht
     * zum Essen angeboten - kein Fehlerbild, keine Logzeile, die Aktion fehlt
     * einfach.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatFruit);
    }
}
