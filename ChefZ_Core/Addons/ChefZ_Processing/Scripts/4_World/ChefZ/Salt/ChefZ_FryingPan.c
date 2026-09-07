//==============================================================================
// ChefZ_FryingPan - die Siedepfanne der Salzkette.
//
// Slice "salt". Production Map §25 (Saltwater -> Raw Salt -> Salt),
// Planungsschritte §16 (Salz als Wirtschaftssystem).
//
// Andockregel woertlich aus dem Kopf von ChefZ_ProcessingStation_Base.c:
//
//   config.cpp          class ChefZ_FryingPan : Inventory_Base { ... };
//   Stationsdatensatz   id == Klassenname, processes[] = { ... }
//   Skript              class ChefZ_FryingPan extends ChefZ_ProcessingStation_Base
//
// WELCHE PROZESSE die Pfanne anbietet, steht ausschliesslich im
// Stationsdatensatz (Config/Processing/SaltStations.json) - nicht hier. WAS aus
// WAS wird, steht im Transform (Config/Processing/Salt.json) - auch nicht hier.
// Diese Datei beantwortet genau EINE Frage, die Daten nicht beantworten
// koennen: hat die Pfanne gerade Feuer unter sich?
//
// ---------------------------------------------------------------------------
// Warum die Waermefrage ueber die Feuerstelle laeuft und nicht ueber die
// Eigentemperatur der Pfanne
// ---------------------------------------------------------------------------
// ChefZ_ProcessingStation_Base.ChefZ_HasHeat() antwortet in der Basis "nein"
// und ist ausdruecklich zum Ueberschreiben da ("Ein Raeucherschrank, der an
// einer Feuerstelle haengt, ueberschreibt das").
//
// Der naheliegende Weg - GetTemperature() >= X - haette einen Schwellwert
// gebraucht, den niemand hier belegen kann: wie warm ein Item neben einem
// Lagerfeuer wird, haengt an UniversalTemperatureSource, an der Weltdatei und
// am Wetter. Eine geratene Zahl haette die ganze Kette entweder unspielbar
// gemacht oder wirkungslos.
//
// Die Frage "brennt in Reichweite eine Feuerstelle" ist dagegen exakt
// beantwortbar: FireplaceBase.IsBurning() ist Vanillas eigene Auskunft. Sie
// kostet einen Umkreisscan alle paar Sekunden - der Job-Timer der Basis tickt
// mit TICK_INTERVAL_SEC, nicht je Frame.
//
// KEINE VANILLA-KLASSE WIRD DABEI VERAENDERT. Diese Datei liest die
// Feuerstelle und faesst sie nicht an: kein modded class, kein Setter, kein
// Brennstoffabzug. Der Brennstoff geht ueber Vanillas eigene Feuerlogik weg,
// waehrend die 15 Minuten des Siedeprozesses laufen - genau das IST der
// Brennstoffpreis des Salzes aus Planungsschritte §16.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_FryingPan extends ChefZ_ProcessingStation_Base
{
    //! Umkreis, in dem eine brennende Feuerstelle als Waermequelle zaehlt.
    //! 2.5 m ist "unmittelbar daneben" - weit genug, dass der Spieler die
    //! Pfanne nicht millimetergenau setzen muss, eng genug, dass eine
    //! Feuerstelle nicht drei Pfannen im Umkreis versorgt, ohne dass man es
    //! sieht.
    static const float CHEFZ_HEAT_RADIUS_M = 2.5;

    /**
     * Brennt in Reichweite eine Feuerstelle?
     *
     * Rein lesend. Wird von ChefZ_ProcessingStation_Base.ChefZ_BuildContext
     * gerufen und landet in ChefZ_ProcessContext.hasHeat; ausgewertet wird sie
     * in ChefZ_CompiledProcess.MeetsEnvironment gegen requiresHeat.
     *
     * Bei "nein" PAUSIERT ein laufender Job, er bricht nicht ab und laeuft nie
     * zurueck (11 §7). Ein ausgehendes Feuer kostet den Spieler damit Zeit,
     * nie Material.
     */
    override bool ChefZ_HasHeat()
    {
        array<Object> nearby = new array<Object>();
        array<CargoBase> proxies = new array<CargoBase>();

        g_Game.GetObjectsAtPosition(GetPosition(), CHEFZ_HEAT_RADIUS_M, nearby, proxies);

        for (int i = 0; i < nearby.Count(); i++)
        {
            FireplaceBase fire = FireplaceBase.Cast(nearby.Get(i));
            if (fire && fire.IsBurning())
                return true;
        }

        return false;
    }
}
