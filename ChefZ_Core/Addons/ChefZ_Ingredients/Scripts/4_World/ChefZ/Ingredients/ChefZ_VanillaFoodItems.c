//==============================================================================
// ChefZ_VanillaFoodItems - die Skriptseite des Slice "vanilla-foods".
//
// Quelle: Vanilla-Audit §3 C (Zucchini) und §3 F (Beeren an der vorhandenen
// Trockenkette). Der Slice bindet ueberwiegend FREMDE Klassen - die brauchen
// kein Skript, sie stehen im JSON. Hier steht nur die EINE eigene Klasse,
// die dabei entsteht (die geschnittene Zucchini ist am 29.08.2026 mit dem
// uebrigen Schnittgut entfallen).
//
// Andockregel aus dem Kopf von ChefZ_Edible_Base.c: die CONFIGklasse erbt von
// einer Vanilla-Klasse, die SKRIPTklasse von ChefZ_Edible_Base.
//
// Layer: 4_World.
//==============================================================================

//! Hagebutten und Holunderbeeren vom Trockenrahmen
//! (TR_CaninaBerriesToDried, TR_SambucusBerriesToDried).
//!
//! Eigene Skriptklasse: ihre Configklasse traegt einen Uebergang
//! Dried -> Boiled.
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
