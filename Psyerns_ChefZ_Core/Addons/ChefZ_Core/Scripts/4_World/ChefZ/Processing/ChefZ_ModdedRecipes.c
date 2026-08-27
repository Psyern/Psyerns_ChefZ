//==============================================================================
// modded class PluginRecipesManagerBase - der ZWEITE und LETZTE Eingriff des
// Core in eine Vanilla-Spielklasse.
//
// Entwurf: 11 §4 (die Methode, woertlich), 11 §5 (BOOT), 11 §7 ("Vanilla-
// Crafting vollstaendig unberuehrt, weil super.RegisterRecipies() immer zuerst
// laeuft"), 11 E3 (letzter Absatz), 00 §4 (Tabelle der Overrides), 00 §5
// Zeile 2, 19 S15, G15.
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
//   3. Der ChefZ-Teil hat keinen Rueckkanal. ChefZ_HandcraftBridge.Install()
//      gibt eine Zahl zurueck, die hier NICHT ausgewertet wird, und sie
//      veraendert nichts an dem, was super getan hat.
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
// unseren, je nach Ladereihenfolge; beide Listen bleiben vollstaendig.
//
// Ruft ein anderer Mod in seiner Kette super NICHT, bricht er ohnehin Vanilla;
// dann fehlen auch die ChefZ-Rezepte, und das ist das sichere Ergebnis.
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

modded class PluginRecipesManagerBase
{
    override void RegisterRecipies()
    {
        // ---- 1) VANILLA - immer, zuerst, ohne jede Vorbedingung ------------
        super.RegisterRecipies();

        // ---- 2) ChefZ - ausschliesslich additiv ----------------------------
        //
        // Beim ERSTEN Missionsstart tut dieser Aufruf nichts: der
        // ChefZ-Bestand steht zu diesem Zeitpunkt noch nicht, weil der
        // PluginManager im MissionBase-KONSTRUKTOR laeuft und ChefZ erst in
        // MissionServer.OnInit() laedt. ChefZ_Boot holt den Aufbau
        // unmittelbar nach dem Laden nach; die vollstaendige Begruendung
        // steht im Kopf von ChefZ_HandcraftBridge, Abschnitt ZEITPUNKT.
        //
        // Der Rueckgabewert wird bewusst verworfen. Es gibt hier nichts zu
        // entscheiden - was ChefZ nicht registrieren konnte, steht im
        // Ladebericht, und Vanillas Liste ist in jedem Fall vollstaendig.
        ChefZ_HandcraftBridge.Install(this);
    }

    /**
     * Ein fertig parametrisiertes ChefZ-Rezept eintragen.
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
     *   - sie prueft, dass das Rezept fertig gebaut ist. Ein halb
     *     parametrisiertes Rezept in Vanillas Liste waere ein Rezept ohne
     *     Zutaten - und Vanillas Cache-Aufbau liefe darueber.
     *   - sie gibt nichts zurueck und veraendert nichts anderes.
     *
     * UnregisterRecipe() wird hier NICHT gespiegelt. Es gibt keinen Weg, ueber
     * ChefZ ein Rezept aus Vanillas Liste zu entfernen, und das ist Absicht.
     */
    void ChefZ_RegisterGeneratedRecipe(ChefZ_GenericCraftRecipe recipe)
    {
        if (!recipe)
            return;

        if (!recipe.ChefZ_IsReady())
        {
            ChefZ_Log.Error(ChefZ_LogChannel.PROCESS,
                "Ein unfertiges Handwerksrezept sollte registriert werden und wurde "
                + "abgewiesen: " + recipe.ChefZ_GetInitError()
                + ". Vanillas Rezeptliste bleibt unveraendert.");
            return;
        }

        RegisterRecipe(recipe);
    }
}
