//==============================================================================
// modded class PluginRecipesManagerBase - der ZWEITE Eingriff des Core in eine
// Vanilla-Spielklasse.
//
// Entwurf: 11 §4 (die Methode, woertlich), 11 §5 (BOOT), 11 §7 ("Vanilla-
// Crafting vollstaendig unberuehrt, weil super.RegisterRecipies() immer zuerst
// laeuft"), 11 E2, 11 E3 (letzter Absatz), 00 §4 (Tabelle der Overrides),
// 00 §5 Zeile 2, 19 S15, G15.
//
//==============================================================================
// BEGRUENDUNG DIESES OVERRIDES
//==============================================================================
// WARUM UEBERHAUPT: Vanillas Craftsystem hat genau EINEN Weg, ein Rezept
// bekannt zu machen - PluginRecipesManagerBase.RegisterRecipies(). Es gibt
// keinen Ereigniskanal, keine Registrierungs-API und keine Datei, in die man
// ein Rezept schreiben koennte. Wer ein Craftrezept hinzufuegen will, muss
// diese Methode erweitern.
//
// WARUM PluginRecipesManagerBase UND NICHT PluginRecipesManager: die Basis ist
// die Klasse, in der RegisterRecipies() DEKLARIERT und definiert ist. Sie
// enthaelt nichts als diese eine Methode und zwei Deklarationen. Die
// Ableitung PluginRecipesManager enthaelt den gesamten Rezeptcache, die
// Zutatenaufloesung und die Ausfuehrung - jede Zeile davon waere zusaetzliche
// Kollisionsflaeche fuer nichts. Der Entwurf nennt in 00 §5 ausdruecklich
// PluginRecipesManagerBase.
//
// WARUM NICHT "einfach eigene Actions": das ist Entscheidung 11 E2. Vanillas
// RecipeBase macht Animation, Werkzeugschaden, Softskills, Mengenuebertragung,
// Health-Vererbung und die Zutatensuche in Hand und Inventar bereits richtig.
// Ein Nachbau hiesse, all das erneut zu erzeugen und mit jedem Patch zu
// pflegen.
//
//==============================================================================
// WARUM HIER RESERVIERT UND NICHT REGISTRIERT WIRD
//==============================================================================
// Vanilla vergibt Rezept-IDs als POSITION in seiner Liste, und die
// Craftaktion uebertraegt genau diese Position ueber das Netz. Wer spaeter
// registriert als die Gegenseite, verschiebt sich gegen sie.
//
// ChefZ nimmt seine Plaetze deshalb HIER - im Missionskonstruktor, an genau
// dem Punkt, an dem Vanilla und jeder super-treue Fremdmod ihre Plaetze
// nehmen. Die eingetragenen Rezepte sind zu diesem Zeitpunkt LEER; sie
// bekommen ihre Daten erst nach dem Laden, ohne dass sich eine einzige
// Position aendert. Die vollstaendige Begruendung samt Zeitachse steht im Kopf
// von ChefZ_HandcraftBridge, Abschnitt POSITIONSANKER.
//
//==============================================================================
// DIE DREI EIGENSCHAFTEN, DIE VANILLA-CRAFTING UNBERUEHRT LASSEN
//==============================================================================
// Sie sind nicht verhandelbar. Wer diese Datei anfasst, prueft sie einzeln.
//
//   1. super.RegisterRecipies() wird BEDINGUNGSLOS und als ERSTE ANWEISUNG
//      aufgerufen. Es gibt keine Bedingung davor und keinen Codepfad, der ihn
//      ueberspringt. Jedes Vanilla-Rezept ist danach registriert, mit
//      derselben ID wie ohne ChefZ - denn IDs werden in
//      Registrierungsreihenfolge vergeben, und ChefZ registriert
//      ausschliesslich HINTER Vanilla.
//
//   2. Es wird NUR HINZUGEFUEGT. UnregisterRecipe() kommt in dieser Datei
//      nicht vor, weder aufgerufen noch ueberschrieben. Kein Vanilla-Rezept
//      wird ersetzt, umbenannt oder abgeschaltet.
//
//   3. Der ChefZ-Teil hat keinen Rueckkanal. ChefZ_HandcraftBridge.Reserve()
//      gibt nichts zurueck und veraendert nichts an dem, was super getan hat.
//
// Daraus folgt strukturell und nicht bloss beabsichtigt: faellt der gesamte
// ChefZ-Teil aus - Config kaputt, keine Transforms, Ausnahme mitten im Aufbau
// -, ist Vanillas Rezeptliste ununterscheidbar von der eines Servers ohne
// ChefZ.
//
//==============================================================================
// VERTRAEGLICHKEIT MIT ANDEREN MODS (G15)
//==============================================================================
// modded class kettet. Unser super-Aufruf ist die erste Anweisung - das
// vertraeglichste moegliche Verhalten. Ein anderer Mod, der ebenfalls
// RegisterRecipies() erweitert, registriert seine Rezepte vor oder nach
// unseren, je nach Ladereihenfolge; beide Listen bleiben vollstaendig, und die
// Reihenfolge innerhalb dieser Kette ist auf Client und Server dieselbe, weil
// sie aus der Ladereihenfolge der Mods folgt und aus nichts sonst.
//
// Ruft ein anderer Mod in seiner Kette super NICHT, bricht er ohnehin Vanilla;
// dann fehlen auch die ChefZ-Plaetze. ChefZ traegt sie in diesem Fall NICHT
// nachtraeglich ein - das waere genau der Versatz, den der Anker beseitigt.
// ChefZ_HandcraftBridge.FillReserved() meldet den Fall im Klartext und laesst
// das Handwerk aus; Kochen und Stationen laufen weiter.
//
// EIN Verhalten eines fremden Mods koennen wir nicht abfangen und benennen es
// deshalb: ruft jemand UnregisterRecipe("ChefZ_GenericCraftRecipe"), trifft es
// wegen 11 E3 alle Handwerksrezepte auf einmal - sie teilen sich EINE Klasse.
// Aus demselben Grund fuehrt Vanillas m_RecipeNamesList unter diesem
// Klassennamen nur EINEN Eintrag. Beides ist der Preis von "genau eine
// generische Ableitung" und beruehrt kein Vanilla-Rezept.
//
// Layer: 4_World.
//==============================================================================

// SCOUT-GEPRUEFT 2026-08-30 (chefz-conflict-scout)
// I4-BELEG: der genannte Fremdmod ist der BEFUND der Kollisionspruefung - der
// eine Nachbar, der vor super registriert und damit die Vanilla-IDs
// verschiebt. Belegt, dass ChefZ das ueberlebt; kein Aufruf, keine
// Abhaengigkeit.
// super zuerst, kein UnregisterRecipe. Expansion Weapons registriert VOR
// super (ExpansionCraftStickySmoke.c:96) und verschiebt damit jede
// Vanilla-ID - ChefZ ueberlebt das, weil ChefZ_HandcraftBridge die
// Ankerposition misst (GetID) statt sie zu rechnen.
modded class PluginRecipesManagerBase
{
    override void RegisterRecipies()
    {
        // ---- 1) VANILLA - immer, zuerst, ohne jede Vorbedingung ------------
        super.RegisterRecipies();

        // ---- 2) ChefZ - ausschliesslich additiv ----------------------------
        //
        // Reserviert N leere Rezeptplaetze. N kommt aus der Engine-Config
        // (CfgChefZ handcraftRecipeSlots) und ist damit auf Client und Server
        // dieselbe Zahl. Ist N gleich 0 - der Normalfall ohne
        // Handwerks-Content -, geschieht hier NICHTS.
        //
        // Es gibt hier nichts zu entscheiden: was ChefZ nicht parametrieren
        // kann, steht im Ladebericht, und Vanillas Liste ist in jedem Fall
        // vollstaendig.
        ChefZ_HandcraftBridge.Reserve(this);
    }

    /**
     * Einen LEEREN ChefZ-Rezeptplatz eintragen.
     *
     * Diese Methode existiert aus genau einem Grund: RegisterRecipe() ist
     * protected und damit nur INNERHALB dieser Klassenhierarchie erreichbar.
     * Die Bruecke liegt ausserhalb - sie muss durch diese Tuer.
     *
     * Sie ist bewusst so eng wie moeglich geschnitten:
     *
     *   - sie nimmt AUSSCHLIESSLICH ein ChefZ_GenericCraftRecipe, nicht ein
     *     RecipeBase. Damit ist sie kein allgemeines Registrierungstor, das
     *     ein anderer Mod versehentlich benutzt.
     *   - sie nimmt ausschliesslich ein UNPARAMETRIERTES Rezept. Ein fertiges
     *     Rezept hier einzutragen hiesse, es NACH dem Konstruktorfenster in
     *     die Liste zu haengen - und genau das ist der Fehler, den der
     *     Positionsanker beseitigt.
     *   - sie veraendert nichts anderes.
     *
     * Ein leeres Rezept ist in Vanillas Liste FOLGENLOS: seine
     * Zutatenlisten sind leer (RecipeBase legt sie im Konstruktor als leere
     * Arrays an), es landet damit in keinem Cache-Eintrag, und
     * ChefZ_GenericCraftRecipe.CanDo() prueft m_ChefZ_Ready als allererstes
     * und liefert false.
     *
     * UnregisterRecipe() wird hier NICHT gespiegelt. Es gibt keinen Weg, ueber
     * ChefZ ein Rezept aus Vanillas Liste zu entfernen, und das ist Absicht.
     *
     * @return false, wenn nichts eingetragen wurde. Der Aufrufer bricht die
     *         Reservierung dann ab - ein halb belegter Anker ist besser als
     *         ein falsch gezaehlter.
     */
    bool ChefZ_ReserveRecipeSlot(ChefZ_GenericCraftRecipe slot)
    {
        if (!slot)
            return false;

        if (slot.ChefZ_IsReady())
        {
            ChefZ_Log.Error(ChefZ_LogChannel.PROCESS, "Ein bereits parametriertes Handwerksrezept sollte als Platzhalter " + "eingetragen werden und wurde abgewiesen. Rezeptplaetze entstehen " + "ausschliesslich leer und im Missionskonstruktor. Vanillas " + "Rezeptliste bleibt unveraendert.");
            return false;
        }

        RegisterRecipe(slot);
        return true;
    }
}
