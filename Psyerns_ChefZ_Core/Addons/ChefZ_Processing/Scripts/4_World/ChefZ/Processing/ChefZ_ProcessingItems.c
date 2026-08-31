//==============================================================================
// ChefZ_ProcessingItems - Getreidemuehle, Pastamaschine, Mehl.
//
// Andockregel aus dem Kopf von ChefZ_ProcessingStation_Base.c: die
// CONFIGklasse erbt von einer Vanilla-Klasse, die SKRIPTklasse von der
// ChefZ-Basis. Der Core bringt fuer keine von beiden einen CfgVehicles-
// Eintrag mit (Invariante I3).
//
// Was hier NICHT steht: eine eigene Action, ein eigener Tick, eine eigene
// Persistenz. Alles drei liegt in ChefZ_ProcessingStation_Base, und welche
// Prozesse die Muehle anbietet, steht in CfgChefZStations - nicht im Code.
//
// ### 31.08.2026 ### Alex' Testbericht: "Muehle hat keine Funktion".
// Die Ursache lag in der config.cpp - der Muehle fehlte der Cargo-Block, und
// ChefZ_ProcessingStation_Base sammelt seine Zutaten ausschliesslich ueber
// ChefZ_FactCollector.CollectFromCargo aus GetInventory().GetCargo(). Ohne
// Cargo kein GetCargo, ohne GetCargo keine Zutat, und zwar lautlos. Der Cargo
// steht jetzt da (5x4); hier steht der Torwaechter dazu.
//
// Layer: 4_World.
//==============================================================================

//! Die Getreidemuehle. Sie bietet PROCESS_MILL an; welcher Transform daran
//! zuendet, entscheidet der ChefZ_ProcessingManager aus den Daten.
//!
//! PROCESS_MILL bleibt STATION_ACTION (25 s, sofort beim Aktionsende) und
//! bekommt bewusst KEINEN Selbstnachstart wie Fleischwolf und Butterfass:
//! Mahlen ist ein kurzer Handgriff am Ort, kein Vorgang, der ohne den Spieler
//! weiterlaufen soll. Der Wechsel auf STATION_TIMED waere ausserdem der
//! groessere Eingriff - er beruehrte beide Transforms und die Erwartung des
//! Spielers an den Fortschrittsbalken - und Alex' Punkt verlangt an der Muehle
//! nur dreierlei: Funktion, Mais und Cargo. Alle drei sind ohne den Wechsel da.
class ChefZ_GrainMill extends ChefZ_ProcessingStation_Base
{
    //! Was in die Muehle darf. Hier AUSNAHMSWEISE Klassennamen statt
    //! Kategorien, und das hat einen Grund: die beiden Transforms der Muehle
    //! (TR_WheatToFlour, TR_CornToFlour) nennen ihre Eingaenge selbst ueber
    //! "cls" und nicht ueber eine Kategorie. Ein Kategorientor waere hier
    //! weiter als die Transforms - GRAIN traefe auch das Mehl anderer Slices,
    //! VEGETABLE saemtliches Gemuese wegen des Maiskolbens. Wer der Muehle ein
    //! drittes Mahlgut gibt, traegt es in DIESE Liste und in einen Transform
    //! ein; beides gehoert ohnehin zusammen.
    static const string CHEFZ_INPUT_WHEAT = "ChefZ_Wheat";
    static const string CHEFZ_INPUT_CORN  = "ChefZ_Corn";
    //! Das Ergebnis. Es MUSS durch denselben Torwaechter: der Cargo ist Ein-
    //! und Ausgang zugleich (getrennte Bereiche gibt die Engine nicht her,
    //! siehe Kopf von ChefZ_StationGate), und was nicht hinein darf, kann
    //! darin auch nicht entstehen.
    static const string CHEFZ_OUTPUT_FLOUR = "ChefZ_Flour";

    /**
     * Mahlgut und Mehl hinein, sonst nichts.
     *
     * Beleg: EntityAI.CanReceiveItemIntoCargo, scripts - 1.29/3_Game/DayZ/
     * Entities/EntityAI.c:1550-1559; Ueberschreibung wie Barrel_ColorBase.c:512.
     * IsKindOf und nicht Cast, damit eine abgeleitete Weizen- oder Maisklasse
     * eines spaeteren Slice ohne Zutun mitzaehlt: Object.IsKindOf,
     * scripts - 1.29/3_Game/DayZ/Entities/Object.c:517.
     *
     * KEINE Registerabfrage und deshalb auch keine Bereitschaftspruefung -
     * dieser Torwaechter kommt ohne ChefZ_StationGate aus und antwortet vom
     * ersten Frame an gleich.
     */
    override bool CanReceiveItemIntoCargo(EntityAI item)
    {
        if (!super.CanReceiveItemIntoCargo(item))
            return false;
        if (!item)
            return false;

        if (item.IsKindOf(CHEFZ_INPUT_WHEAT))
            return true;
        if (item.IsKindOf(CHEFZ_INPUT_CORN))
            return true;
        if (item.IsKindOf(CHEFZ_OUTPUT_FLOUR))
            return true;

        return false;
    }
}

//! Die Pastamaschine (frueher ChefZ_RollingPin). Reines Werkzeug - sie traegt
//! keinen ChefZ-Zustand und wird nie verbraucht, nur ueber die Werkzeuggruppe
//! ROLLING_PIN gefunden. Die Gruppe heisst weiter so, weil PROCESS_ROLL in
//! ChefZ_Baking auf diesen Namen zeigt; die Begruendung steht an der Gruppe.
//!
//! KEINE Station und deshalb kein ChefZ_ProcessingStation_Base: die beiden
//! Transforms, die sie bedient, haben je EINEN Eingang. Die Begruendung steht
//! vollstaendig an der Configklasse.
class ChefZ_PastaMachine extends ItemBase {}

//! Mehl. Ergebnis des Mahlens und Eingang jedes Teigs.
class ChefZ_Flour extends ChefZ_GrainFoodBase {}
