//==============================================================================
// Skriptklassen der Fischfilets - Slice "fillets".
//
// Andockregel woertlich aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/
// State/ChefZ_Edible_Base.c:
//
//     config.cpp   class ChefZ_CodFillet : Edible_Base { ... };   (VANILLA-Basis)
//     Skript       class ChefZ_CodFillet extends ChefZ_Edible_Base { }
//
// Ohne diese Ableitung traegt das Filet keinen ChefZ-Zustand - es waere ein
// gewoehnliches Vanilla-Nahrungsmittel. Kein Fehler, nur weniger.
//
// Kein modded class, kein Override ausser den beiden Zusagen unten, keine
// eigene Aktion: Zustand, Frische und Verderb liegen vollstaendig in
// ChefZ_Edible_Base. Was hier steht, ist die Bindung.
//
// WAS EIN FILET IST - Kategorie, Tags, Zustand, Naehrwert - steht in Daten
// (Config/Ingredients/Fillets.json und der config.cpp), nicht in Code. Ein
// neuer Fisch braucht deshalb keine Core-Aenderung und keine Zeile hier ausser
// seiner eigenen leeren Klasse.
//
// KEINE TERJE-REFERENZ. Die Koederpraeferenz und die Fangchance beruehrt dieser
// Slice nicht - er liefert das Ziel des Filetierens, sonst nichts.
//
// Layer: 4_World.
//==============================================================================

//! Skriptbasis aller Filets dieses Slice. Entspricht der Configklasse gleichen
//! Namens (scope = 0, sie ist selbst kein Item).
//!
//! Die dreissig Klassen darunter sind leer und muessen es sein: DayZ sucht zu
//! einer Configklasse die gleichnamige Skriptklasse und geht sonst die
//! CONFIG-Elternkette hinauf. Jede von ihnen erbt in der config.cpp von
//! ChefZ_SeaFillet_Base bzw. ChefZ_FreshFillet_Base und damit von
//! ChefZ_FishFillet_Base - sie faenden diese Klasse hier also auch ohne eigene
//! Zeile. Sie stehen trotzdem da, damit jede Configklasse eine benannte
//! Skriptentsprechung hat und ein spaeterer Sonderfall (ein Fisch mit eigenem
//! Verhalten) genau eine Zeile kostet.
class ChefZ_FishFillet_Base extends ChefZ_Edible_Base
{
    /**
     * Ein Filet sagt, dass es Fleisch ist. Object.IsMeat() liefert sonst false.
     *
     * Das ist die Bedingung, unter der Vanilla die Sonderregeln fuer ROHES
     * Fleisch ueberhaupt anwendet:
     *
     *   ActionEatMeat.ApplyModifiers   "IsMeat() && IsFoodRaw()" -> blutige
     *                                  Haende ueber PluginLifespan
     *   Edible_Base.ProcessDecay       der Fleischzweig mit
     *                                  DECAY_FOOD_RAW_MEAT / BOILED / BAKED /
     *                                  DRIED. Ohne die Zusage fiele jedes Filet
     *                                  in den letzten Zweig ("opened cans") und
     *                                  bekaeme statt einer Garstufe irgendwann
     *                                  still eAgents.FOOD_POISON eingesetzt.
     *
     * Vanillas eigene Filets sagen dasselbe: CarpFilletMeat.c:11-14,
     * MackerelFilletMeat.c - beide ueberschreiben IsMeat().
     */
    override bool IsMeat()
    {
        return true;
    }

    /**
     * Am Spiess garen. Vanillas Filets koennen es (CarpFilletMeat.c:6-9), und
     * ein Filet ueber dem Feuer ist die erste Mahlzeit, die ein Angler hat.
     *
     * CanBeCooked() selbst steht NICHT hier: ChefZ_Edible_Base schaltet die
     * Kochbarkeit bereits ein und rechnet sie gegen die deklarierten
     * Garstufen. Eine zweite Behauptung an dieser Stelle waere eine zweite
     * Wahrheit.
     */
    override bool CanBeCookedOnStick()
    {
        return true;
    }

    /**
     * Die Essaktion. Vanilla setzt sie NICHT auf Edible_Base, sondern auf jeder
     * Nahrungsklasse einzeln (CarpFilletMeat.c:26-35). Ohne diese Zeilen bietet
     * das Spiel das Filet nicht zum Essen an - ohne Fehlerbild und ohne
     * Logzeile.
     *
     * ActionEatMeat und nicht ActionEatBig: die Fleischvariante bringt
     * ApplyModifiers mit (blutige Haende bei rohem Fisch) und verbraucht
     * UAQuantityConsumed.EAT_NORMAL statt EAT_BIG. Genau diese Aktion
     * registrieren Vanillas vier Filetklassen.
     *
     * ActionForceFeed gehoert dazu, weil in Vanilla ueberall dort, wo selbst
     * gegessen wird, auch gefuettert werden kann.
     *
     * Vanillas ActionCreateIndoorFireplace/-Oven aus CarpFilletMeat.c fehlen
     * absichtlich: sie haengen dort an der Klasse, weil Vanilla sie an JEDEM
     * Item registriert, das man in der Hand haelt. Sie gehoeren nicht zum
     * Filet, und sie fehlen dem Spieler nicht - jedes Vanilla-Item, das sie
     * traegt, bleibt unveraendert.
     */
    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatMeat);
    }
}

// ---------------------------------------------------------------------------
// Die dreissig Filets. Leer und absichtlich leer - siehe Kopf.
// ---------------------------------------------------------------------------
class ChefZ_PlaiceFillet extends ChefZ_FishFillet_Base {}
class ChefZ_GiltheadBreamFillet extends ChefZ_FishFillet_Base {}
class ChefZ_HalibutFillet extends ChefZ_FishFillet_Base {}
class ChefZ_CodFillet extends ChefZ_FishFillet_Base {}
class ChefZ_SoleFillet extends ChefZ_FishFillet_Base {}
class ChefZ_SeaBassFillet extends ChefZ_FishFillet_Base {}
class ChefZ_RedMulletFillet extends ChefZ_FishFillet_Base {}
class ChefZ_CongerFillet extends ChefZ_FishFillet_Base {}
class ChefZ_HorseMackerelFillet extends ChefZ_FishFillet_Base {}
class ChefZ_JapaneseSardineFillet extends ChefZ_FishFillet_Base {}
class ChefZ_RockfishFillet extends ChefZ_FishFillet_Base {}
class ChefZ_BluefinTunaFillet extends ChefZ_FishFillet_Base {}
class ChefZ_YellowfinTunaFillet extends ChefZ_FishFillet_Base {}
class ChefZ_RedSnapperFillet extends ChefZ_FishFillet_Base {}
class ChefZ_GiantGrouperFillet extends ChefZ_FishFillet_Base {}
class ChefZ_MahiMahiFillet extends ChefZ_FishFillet_Base {}
class ChefZ_GiantTrevallyFillet extends ChefZ_FishFillet_Base {}
class ChefZ_BlueMarlinFillet extends ChefZ_FishFillet_Base {}
class ChefZ_SailfishFillet extends ChefZ_FishFillet_Base {}
class ChefZ_BonitoFillet extends ChefZ_FishFillet_Base {}
class ChefZ_YellowtailFillet extends ChefZ_FishFillet_Base {}
class ChefZ_TarponFillet extends ChefZ_FishFillet_Base {}
class ChefZ_SalmonFillet extends ChefZ_FishFillet_Base {}
class ChefZ_SockeyeSalmonFillet extends ChefZ_FishFillet_Base {}
class ChefZ_PerchFillet extends ChefZ_FishFillet_Base {}
class ChefZ_MuskellungeFillet extends ChefZ_FishFillet_Base {}
class ChefZ_SnakeheadFillet extends ChefZ_FishFillet_Base {}
class ChefZ_RainbowTroutFillet extends ChefZ_FishFillet_Base {}
class ChefZ_CharFillet extends ChefZ_FishFillet_Base {}
class ChefZ_EelFillet extends ChefZ_FishFillet_Base {}
