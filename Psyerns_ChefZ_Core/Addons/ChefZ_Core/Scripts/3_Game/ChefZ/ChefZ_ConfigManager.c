//==============================================================================
// ChefZ_ConfigManager - die einzige Stelle, die Dateien und Config-Baeume liest
//
// Entwurf: 02 (vollstaendig), insbesondere §5.3 (Schnittstelle), §6
// (Datenfluss und Ladeordnung), §8 (Fehlerdoktrin), 03 §5 (Identitaeten),
// 18 §4 (Logkonfiguration und Bericht).
//
// Ablauf, woertlich nach 02 §6:
//
//   Quellen (Rang 1 -> 2 -> 3)  ->  Sink  ->  NORMALIZE  ->  MERGE
//     -> VALIDATE  ->  ASSIGN IDENTITIES  ->  COMPILE  ->  INDEX
//     -> AUDIT  ->  FREEZE
//
// INDEX und AUDIT sind in S2 leer - der invertierte Matcherindex gehoert S6,
// das Naehrwertaudit S13. Die Stufen stehen trotzdem sichtbar im Code, damit
// spaeter niemand raten muss, wo sie hingehoeren.
//
// Zwei Punkte, die den Aufbau bestimmen und leicht zu uebersehen sind:
//
// 1. EINSTELLUNGEN ZUERST, ZWEIMAL. strictMode und safeModeErrorThreshold
//    entscheiden, wie mit Fehlern umzugehen ist - sie muessen also gelten,
//    bevor der erste Fehler faellt. Gelesen wird deshalb zuerst die
//    Einstellungsdatei (Rang 2, mit vollstaendigen Code-Defaults als Netz),
//    danach der Rest, und nach Rang 3 werden die Einstellungen erneut
//    aufgeloest - ein Overlay darf sie schliesslich patchen.
//
// 2. DAS LOG WIRD SO FRUEH WIE MOEGLICH KONFIGURIERT. Vor der Konfiguration
//    gilt ERR auf ALL (18 §6). Damit gehen Fehler, die vor dem Lesen der
//    Logeinstellungen anfallen, nicht verloren.
//
// Invariante I2 gilt hier unbedingt: was auch immer schiefgeht, das Ergebnis
// ist ein Core, der weniger tut - nie einer, der etwas Falsches tut.
//
// Layer: 3_Game.
//==============================================================================

enum ChefZ_ConfigHealth
{
    UNINITIALIZED,
    OK,
    DEGRADED,
    SAFE_MODE
}

class ChefZ_ConfigManager
{
    //! Manifestversion, die dieser Core versteht (CfgChefZ chefzApiVersion).
    static const int API_VERSION = 1;

    //! Die eigene Einstellungsdatei. Sie laeuft NICHT ueber CfgChefZ:
    //!   - der Core meldet sich nicht bei sich selbst an,
    //!   - und ihr Inhalt ist der einzige, den der Core vollstaendig als
    //!     Code-Default kennt (02 §5.3). Fehlt sie, ist das eine Warnung und
    //!     kein Fehler - es geht nichts verloren.
    static const string CORE_SETTINGS_FILE = "ChefZ_Core/Config/Core.json";

    private static ref ChefZ_ConfigManager s_Instance;

    private ref ChefZ_LoadReport      m_Report;
    private ref ChefZ_CoreSettingsDef m_Settings;

    //! S5: der Kontext, gegen den Selektoren und Slots kompiliert werden
    //! (07 §5). Er entsteht erst NACH dem Kategoriebaum und den
    //! Zutatenbindungen - vorher waere jede Kategorie unbekannt und jedes
    //! Rezept abgewiesen. Der Rezeptcompiler (S6) holt ihn ueber
    //! SelectorContext().
    private ref ChefZ_CompileContext m_SelectorCtx;
    private ref ChefZ_RecordSink      m_Sink;
    private ref ChefZ_ProfileOverlaySource m_Overlay;

    private ChefZ_ConfigHealth m_Health;
    private bool m_Ready;
    private bool m_IsServer;
    private int  m_SliceCount;
    private int  m_FileCount;
    private int  m_LoadMillis;

    //--- Registries (02 §5.3) -------------------------------------------------
    private ref ChefZ_Registry<ChefZ_CategoryDef>     m_Categories;
    private ref ChefZ_Registry<ChefZ_TagDef>          m_Tags;
    private ref ChefZ_Registry<ChefZ_StateDef>        m_States;
    private ref ChefZ_Registry<ChefZ_QualityTierDef>  m_QualityTiers;
    private ref ChefZ_Registry<ChefZ_ToolGroupDef>    m_ToolGroups;
    private ref ChefZ_Registry<ChefZ_DeviceDef>       m_Devices;
    private ref ChefZ_Registry<ChefZ_ContainerDef>    m_Containers;
    private ref ChefZ_Registry<ChefZ_IngredientDef>   m_Ingredients;
    private ref ChefZ_Registry<ChefZ_NutritionDef>    m_Nutrition;
    private ref ChefZ_Registry<ChefZ_PreservationDef> m_Preservation;
    private ref ChefZ_Registry<ChefZ_ProcessDef>      m_Processes;
    private ref ChefZ_Registry<ChefZ_StationDef>      m_Stations;
    private ref ChefZ_Registry<ChefZ_TransformDef>    m_Transforms;
    private ref ChefZ_Registry<ChefZ_RecipeDef>       m_Recipes;

    //--- Identitaeten der sync-relevanten Arten (03 §3.2) --------------------
    private ref ChefZ_IdentityMap m_StateIdentities;
    private ref ChefZ_IdentityMap m_QualityIdentities;

    //--------------------------------------------------------------------------

    void ChefZ_ConfigManager()
    {
        m_Report   = new ChefZ_LoadReport();
        m_Sink     = new ChefZ_RecordSink();
        m_Settings = new ChefZ_CoreSettingsDef();
        m_Settings.ResolveDefaults();
        m_Overlay  = new ChefZ_ProfileOverlaySource();

        m_Health     = ChefZ_ConfigHealth.UNINITIALIZED;
        m_Ready      = false;
        m_IsServer   = true;
        m_SliceCount = 0;
        m_FileCount  = 0;
        m_LoadMillis = 0;

        m_Categories   = new ChefZ_Registry<ChefZ_CategoryDef>();
        m_Tags         = new ChefZ_Registry<ChefZ_TagDef>();
        m_States       = new ChefZ_Registry<ChefZ_StateDef>();
        m_QualityTiers = new ChefZ_Registry<ChefZ_QualityTierDef>();
        m_ToolGroups   = new ChefZ_Registry<ChefZ_ToolGroupDef>();
        m_Devices      = new ChefZ_Registry<ChefZ_DeviceDef>();
        m_Containers   = new ChefZ_Registry<ChefZ_ContainerDef>();
        m_Ingredients  = new ChefZ_Registry<ChefZ_IngredientDef>();
        m_Nutrition    = new ChefZ_Registry<ChefZ_NutritionDef>();
        m_Preservation = new ChefZ_Registry<ChefZ_PreservationDef>();
        m_Processes    = new ChefZ_Registry<ChefZ_ProcessDef>();
        m_Stations     = new ChefZ_Registry<ChefZ_StationDef>();
        m_Transforms   = new ChefZ_Registry<ChefZ_TransformDef>();
        m_Recipes      = new ChefZ_Registry<ChefZ_RecipeDef>();

        m_Categories.Init(ChefZ_RecordKind.CATEGORY);
        m_Tags.Init(ChefZ_RecordKind.TAG);
        m_States.Init(ChefZ_RecordKind.STATE);
        m_QualityTiers.Init(ChefZ_RecordKind.QUALITY_TIER);
        m_ToolGroups.Init(ChefZ_RecordKind.TOOL_GROUP);
        m_Devices.Init(ChefZ_RecordKind.DEVICE);
        m_Containers.Init(ChefZ_RecordKind.CONTAINER);
        m_Ingredients.Init(ChefZ_RecordKind.INGREDIENT);
        m_Nutrition.Init(ChefZ_RecordKind.NUTRITION);
        m_Preservation.Init(ChefZ_RecordKind.PRESERVATION);
        m_Processes.Init(ChefZ_RecordKind.PROCESS);
        m_Stations.Init(ChefZ_RecordKind.STATION);
        m_Transforms.Init(ChefZ_RecordKind.TRANSFORM);
        m_Recipes.Init(ChefZ_RecordKind.RECIPE);

        m_StateIdentities = new ChefZ_IdentityMap();
        m_StateIdentities.SetRegistryName(ChefZ_RecordKind.STATE);
        m_QualityIdentities = new ChefZ_IdentityMap();
        m_QualityIdentities.SetRegistryName(ChefZ_RecordKind.QUALITY_TIER);
    }

    static ChefZ_ConfigManager Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_ConfigManager();
        return s_Instance;
    }

    //==========================================================================
    // Laden
    //==========================================================================

    /**
     * Server: Rang 1 + 2 + 3. Client: Rang 1 + 2 (02 §6).
     *
     * Rueckgabe: true, wenn der Core danach arbeitsfaehig ist (OK oder
     * DEGRADED). false bei SAFE_MODE oder abgeschaltetem Core - in beiden
     * Faellen laeuft Vanilla unveraendert weiter (Invariante I2).
     *
     * Der Aufruf ist idempotent: ein zweiter Aufruf tut nichts und meldet es.
     * Es gibt in V1 bewusst kein Neuladen zur Laufzeit (02 E5).
     */
    bool LoadAll(bool isServer)
    {
        if (m_Ready)
        {
            ChefZ_Log.Warn(ChefZ_LogChannel.CONFIG, "LoadAll() wurde erneut aufgerufen und ignoriert. Ein Neuladen zur Laufzeit " + "gibt es in V1 bewusst nicht (02 E5) - es wuerde Sync-Ordinale neu vergeben, " + "waehrend Items bereits Zustandswerte tragen.");
            return m_Health == ChefZ_ConfigHealth.OK || m_Health == ChefZ_ConfigHealth.DEGRADED;
        }

        int startTick = TickCount(0);
        m_IsServer = isServer;
        m_Report.SetChannel(ChefZ_LogChannel.CONFIG);

        // ---- Verzeichnis und Vorlagen (nur Server) --------------------------
        // Clientseitig ist $profile: das lokale Benutzerverzeichnis des
        // Spielers (02 E2). Dort etwas anzulegen waere ohne Nutzen.
        if (m_IsServer)
            m_Overlay.EnsureLayout(m_Report);

        // ---- Rang 2, Bootstrap: die eigenen Einstellungen -------------------
        //
        // Diese eine Datei wird VOR Rang 1 gelesen, und das ist die einzige
        // Stelle, an der die Rangreihenfolge nicht eingehalten wird. Zulaessig
        // ist es, weil die Art "coreSettings" aus Rang 1 nicht kommen KANN -
        // ChefZ_ConfigCppSource liest sie nicht, es gibt keinen
        // CfgChefZ-Knoten dafuer. Es kann hier also nichts ueberschrieben
        // werden, was spaeter noch kommt.
        //
        // Noetig ist es, weil strictMode und safeModeErrorThreshold gelten
        // muessen, BEVOR der erste Fehler faellt - und weil die Logstufe
        // bestimmt, ob man den Rest ueberhaupt sieht.
        ChefZ_AddonJsonSource settingsSource = new ChefZ_AddonJsonSource();
        array<string> settingsFiles = new array<string>();
        settingsFiles.Insert(CORE_SETTINGS_FILE);
        settingsSource.Init("ChefZ_Core", settingsFiles, true);
        settingsSource.Read(m_Sink, m_Report);

        ResolveSettings();
        ApplyLogSettings();

        if (!m_Settings.enabled)
        {
            // 02 §5.4: enabled=false => Core inert, reines Vanilla. Nichts
            // weiter laden, nichts einfrieren, nichts melden ausser dieser
            // einen Zeile.
            m_Health = ChefZ_ConfigHealth.OK;
            m_Ready  = true;
            m_LoadMillis = TickCount(startTick);
            ChefZ_Log.Banner("Core ist per Einstellung abgeschaltet (enabled=false) - " + "Vanilla-Kochen laeuft unveraendert.");
            // 18 §4 verlangt die Zusammenfassungszeile IMMER - auch dann, wenn
            // gar nichts geladen wurde. Sonst sieht ein Betreiber im RPT nicht,
            // ob der Core stumm oder abwesend ist.
            ReportSummary();
            return false;
        }

        // ---- Rang 1: config.cpp ---------------------------------------------
        ChefZ_ConfigCppSource cppSource = new ChefZ_ConfigCppSource();
        cppSource.Read(m_Sink, m_Report);
        m_FileCount = m_FileCount + settingsSource.GetFileCount();

        // ---- Rang 2: Addon-JSON laut Manifest -------------------------------
        array<ref ChefZ_SliceManifest> slices = ChefZ_ManifestReader.ReadAll(m_Report);
        m_SliceCount = slices.Count();
        for (int i = 0; i < slices.Count(); i++)
        {
            ChefZ_AddonJsonSource src = new ChefZ_AddonJsonSource();
            src.InitFromSlice(slices.Get(i));
            src.Read(m_Sink, m_Report);
            m_FileCount = m_FileCount + src.GetFileCount();
        }

        if (m_SliceCount == 0 && cppSource.GetFileCount() == 0)
        {
            // 02 §8, erste Zeile: keine Datenquelle ist KEIN Fehler.
            m_Report.AddInfo("Keine ChefZ-Datenquelle gefunden - Core inert. " + "Vanilla-Kochen laeuft unveraendert. Ein Content-Addon meldet sich ueber " + "CfgChefZ in seiner eigenen config.cpp an (02 §4).");
        }

        // ---- Rang 3: Overlay, nur Server -------------------------------------
        if (m_IsServer && m_Settings.allowProfileOverlay)
        {
            m_Overlay.Read(m_Sink, m_Report);
            m_FileCount = m_FileCount + m_Overlay.GetFileCount();

            // Das Overlay darf die Einstellungen patchen - also erneut
            // aufloesen und das Log nachziehen.
            ResolveSettings();
            ApplyLogSettings();
        }
        else if (m_IsServer)
        {
            ChefZ_Log.Info(ChefZ_LogChannel.CONFIG, "Overlay ist per Einstellung abgeschaltet (allowProfileOverlay=false) - " + "$profile:ChefZ wird nicht gelesen.");
        }

        // ---- Aufbereitung ----------------------------------------------------
        BuildRegistries();

        m_LoadMillis = TickCount(startTick);
        DecideHealth();
        ReportSummary();

        m_Ready = true;
        return m_Health == ChefZ_ConfigHealth.OK || m_Health == ChefZ_ConfigHealth.DEGRADED;
    }

    //--------------------------------------------------------------------------

    /**
     * Holt den Einstellungsrecord aus dem Sink und macht ihn benutzbar.
     *
     * Nie null (02 §5.3): fehlt der Record, bleiben die Code-Defaults stehen.
     */
    private void ResolveSettings()
    {
        array<ref ChefZ_Record> records = m_Sink.GetRecords(ChefZ_RecordKind.CORE_SETTINGS);

        ChefZ_CoreSettingsDef found;
        for (int i = 0; i < records.Count(); i++)
        {
            ChefZ_CoreSettingsDef candidate = ChefZ_CoreSettingsDef.Cast(records.Get(i));
            if (!candidate)
                continue;
            if (candidate.id == ChefZ_CoreSettingsDef.PRIMARY_ID)
            {
                found = candidate;
                break;
            }
            if (!found)
                found = candidate;      // Rueckfall: der erste, wenn es kein "CORE" gibt
        }

        if (found)
            m_Settings = found;

        m_Settings.ResolveDefaults();
        m_Settings.ClampAndReport(m_Report);
    }

    /**
     * 18 §4: Logstufe und Kanalmaske aus den Einstellungen uebernehmen.
     * Unbekannte Kanalnamen wirken nicht, blockieren aber auch nichts.
     */
    private void ApplyLogSettings()
    {
        array<string> unknown = new array<string>();
        int mask = m_Settings.ResolveChannelMask(unknown);

        ChefZ_Log.SetLimits(m_Settings.maxOnceKeys, m_Settings.logBufferLines, m_Settings.maxLogSizeMB);
        ChefZ_Log.Configure(m_Settings.logLevel, mask, m_Settings.logToFile, m_Settings.logServerOnly);

        for (int i = 0; i < unknown.Count(); i++)
        {
            m_Report.AddWarn(m_Settings.sourceRef, m_Settings.id, "Unbekannter Logkanal \"" + unknown.Get(i) + "\" - ignoriert. Gueltig: " + ChefZ_LogChannel.ValidNames());
        }
    }

    //==========================================================================
    // VALIDATE -> IDENTITIES -> COMPILE -> INDEX -> AUDIT -> FREEZE
    //==========================================================================

    private void BuildRegistries()
    {
        ChefZ_ValidationContext vctx = new ChefZ_ValidationContext();
        vctx.Init(m_Report);
        ChefZ_CompileContext cctx = new ChefZ_CompileContext();
        cctx.Init(m_Report);

        // Die Ladeordnung ist Vertrag (02 §6): jede Art wird gegen die vor ihr
        // geladenen geprueft. coreSettings ist bereits verarbeitet.
        array<string> order = ChefZ_RecordKind.LoadOrder();
        for (int i = 0; i < order.Count(); i++)
        {
            string kind = order.Get(i);
            if (kind == ChefZ_RecordKind.CORE_SETTINGS)
                continue;

            ChefZ_RegistryBase registry = RegistryOf(kind);
            if (!registry)
                continue;

            FillRegistry(registry, kind, vctx, cctx);
        }

        // ASSIGN IDENTITIES (03 §5): nur sync-relevante Arten, und die
        // ausschliesslich aus Rang 1 - siehe BuildIdentities().
        BuildIdentities(m_States, m_StateIdentities, ChefZ_RecordKind.STATE);
        BuildIdentities(m_QualityTiers, m_QualityIdentities, ChefZ_RecordKind.QUALITY_TIER);

        // KATEGORIEBAUM (04 §4).
        //
        // Zeitpunkt: nach dem Fuellen aller Registries, VOR dem Einfrieren.
        // 04 §4 schreibt "nach MERGE, vor COMPILE" - gemeint ist "bevor die
        // Leser kommen". Hier muss es nach COMPILE liegen, weil COMPILE in
        // dieser Umsetzung je Record in FillRegistry laeuft und der Baum die
        // dort internierten Symbole braucht. Vor FREEZE bleibt es, damit der
        // Baum steht, bevor irgendein Leser (S4 Ingredient Manager, S5
        // Matcher) die Registries als fertig ansieht.
        //
        // Der Aufruf ist unbedingt: auch ohne eine einzige Kategorie soll der
        // Manager gebaut - also "bereit und leer" - sein. Sonst antwortete er
        // auf jede Abfrage mit dem Fehler "vor Build aufgerufen", obwohl
        // schlicht keine Kategorien konfiguriert sind (04 §6).
        ChefZ_CategoryManager.Get().Build(m_Categories, m_Tags, m_Report);

        // ZUSTAENDE (06 §4.2).
        //
        // Zwingend NACH dem Kategoriebaum: der Zustandsmanager prueft jeden
        // implies-Tag gegen die Tag-Menge. Vor dem Baum gebaut, waere jeder
        // Tag unbekannt und jede implies-Liste leer - ein Ausfall, der
        // niemandem auffiele, weil "matcht nicht" wie fehlender Content
        // aussieht.
        //
        // Zwingend NACH BuildIdentities(): die Ordinaltabelle wird
        // hineingereicht und nicht zweitgerechnet (03 §4).
        //
        // Zwingend VOR dem Ingredient Manager ist er NICHT - der prueft
        // defaultState direkt gegen die Registry und braucht den Manager
        // nicht. Er steht hier trotzdem davor, weil die Reihenfolge damit
        // dieselbe ist wie in ChefZ_RecordKind.LoadOrder(): Zustaende vor
        // Zutaten.
        //
        // Der Aufruf ist unbedingt - auch ohne einen einzigen Zustand soll der
        // Manager "bereit und leer" sein (06 §7, erste Zeile).
        ChefZ_StateManager.Get().Build(m_States, m_Report, m_StateIdentities);

        // QUALITAETSSTUFEN (12 §6, BOOT).
        //
        // Zwingend NACH dem Kategoriebaum: der Qualitaetsmanager prueft jeden
        // grantsTags-Eintrag gegen die Tag-Menge. Vor dem Baum gebaut, waere
        // jeder Tag unbekannt und jede Tagliste leer - ein Ausfall, der
        // niemandem auffiele, weil "matcht nicht" wie fehlender Content
        // aussieht.
        //
        // Zwingend NACH BuildIdentities(): die Ordinaltabelle wird
        // hineingereicht und nicht zweitgerechnet (03 §4).
        //
        // Zwingend VOR BuildSelectorContext(): der Nachschlager holt die
        // Qualitaetsraenge von hier, und ohne sie wird jedes Rezept mit
        // "minQuality" abgewiesen.
        //
        // Die Einstellungen kommen mit, weil die Gewichte der Punktrechnung
        // aus Core.json stammen (12 §4). Sie werden dort GENAU EINMAL
        // aufgeloest - ein zweiter Aufruf schriebe jede Warnung doppelt in den
        // Ladebericht.
        //
        // Der Aufruf ist unbedingt - auch ohne eine einzige Stufe soll der
        // Manager "bereit und leer" sein (12 §8, erste Zeile).
        ChefZ_QualityManager.Get().Build(m_QualityTiers, m_Report, GetSettings(), m_QualityIdentities);

        // ZUTATENBINDUNGEN (05 §4).
        //
        // Zwingend NACH dem Kategoriebaum: der Ingredient Manager friert je
        // Klasse eine Vorfahren-Closure ein und prueft jede genannte Kategorie
        // und jeden Tag gegen den Baum. Vor dem Baum gebaut, waere jede
        // Kategorie unbekannt und jede Closure leer - ein Ausfall, der
        // niemandem auffiele, weil "matcht nicht" wie fehlender Content
        // aussieht.
        //
        // Die Zustandsregistry kommt mit, damit ein Tippfehler in defaultState
        // im Ladebericht steht und nicht erst bei der ersten misslungenen
        // Kochprobe (05 §7).
        //
        // Ebenfalls unbedingt: auch ohne eine einzige Bindung soll der Manager
        // "bereit und leer" sein, sonst antwortete er auf jede Abfrage mit dem
        // Fehler "vor Build aufgerufen".
        ChefZ_IngredientManager.Get().Build(m_Ingredients, m_Report, m_States);

        // NAEHRWERTANGABEN (13 §6, BOOT).
        //
        // Zwingend NACH dem Kategoriebaum: eine Angabe mit scope "category"
        // braucht den Bitindex ihrer Kategorie, eine mit scope "tag" die
        // Tag-Menge. Vor dem Baum gebaut, waere jede Kategorie unbekannt und
        // jede Kategorieangabe abgewiesen.
        //
        // Zwingend NACH dem Ingredient Manager: die Auffindungsreihenfolge
        // Klasse -> Kategorie -> Tag (13 E4) fragt fuer eine Klasse deren
        // Vorfahren-Closure und deren Tags ab, und beide gibt es vorher nicht.
        // Ohne ihn faende jede Sollrechnung nur Klassenrecords und liesse
        // jede Kategorieangabe stumm liegen - ein Ausfall, der aussieht wie
        // fehlender Content.
        //
        // Die Einstellungen kommen mit, weil enableNutritionAudit, die
        // Toleranz und der Befunddeckel Bestandteil des Audits sind (13 §8).
        //
        // Der Aufruf ist unbedingt - auch ohne eine einzige Angabe soll der
        // Manager "bereit und leer" sein. Dann liest die Sollrechnung
        // ausschliesslich CfgVehicles, und das ist die genauere Quelle, nicht
        // die schlechtere (13 E4).
        ChefZ_NutritionManager.Get().Build(m_Nutrition, m_Report, GetSettings());

        // HALTBARKEIT (14 §6, BOOT).
        //
        // Zwingend NACH dem Kategoriebaum: eine Regel mit scope "category"
        // braucht den Bitindex ihrer Kategorie, eine mit scope "tag" die
        // Tag-Menge. Vor dem Baum gebaut, waere jede Kategorie unbekannt und
        // jede Kategorieregel abgewiesen.
        //
        // Zwingend NACH dem Zustands- und dem Qualitaetsmanager: die Regeln
        // mit scope "state" bzw. "quality" werden gegen deren Bestand
        // geprueft, und der Multiplikator liest StateDef.spoilageMultiplier
        // und QualityTier.spoilageMultiplier (14 §3).
        //
        // Zwingend NACH dem Ingredient Manager: eine Regel mit scope "class"
        // meldet, wenn ihre Klasse gar keine deklarierte ChefZ-Zutat ist -
        // sie wirkt dann nur, falls die Klasse von ChefZ_Edible_Base ableitet
        // (14 E2).
        //
        // Die Einstellungen kommen mit, weil globalSpoilageScale,
        // minDecayScale, maxDecayScale und defaultFreshnessLifetimeSec
        // Bestandteil der Rechnung sind (14 §3).
        //
        // Der Aufruf ist unbedingt - auch ohne eine einzige Regel soll der
        // Manager "bereit und leer" sein. Dann liefert ComputeDecayScale die
        // globale Skala, und der Verfall ist bitgenau Vanilla (14 §8, erste
        // Zeile).
        ChefZ_PreservationManager.Get().Build(m_Preservation, m_Report, GetSettings());

        // SELEKTORKONTEXT (07 §5).
        //
        // Zwingend NACH beiden Managern: der Nachschlager greift auf
        // Kategoriebitindizes und Rueckwaertsindizes durch, und beide gibt es
        // vorher nicht. Er wird hier gebaut und nicht erst beim ersten
        // Rezeptcompiler-Aufruf, damit die Reihenfolge an EINER Stelle steht
        // und nicht davon abhaengt, wer wann zuerst fragt.
        BuildSelectorContext();

        // INDEX und AUDIT (02 §6) - seit S6 fuer Rezepte gefuellt.
        //
        // Zwingend NACH BuildSelectorContext(): der Rezeptcompiler uebersetzt
        // Selektoren und Slots und braucht dafuer Kategoriebits,
        // Rueckwaertsindizes und die Spezifitaetsgewichte. Zwingend VOR
        // FreezeAll(): der Engine-Build liest die Rezept- und Geraeteregistry,
        // und ein Leser soll sie nicht als fertig ansehen, bevor der Index
        // steht.
        //
        // Der Aufruf ist unbedingt - auch ohne ein einziges Rezept soll die
        // Engine "bereit und leer" sein. Sonst antwortete HasAnyRecipeFor()
        // nicht mit einem ruhigen false, sondern mit einem Fehler ueber einen
        // fehlenden Aufbau, den es nie geben wird.
        //
        // Das Naehrwertaudit (13 §5) laeuft seit S12 im Anschluss an den
        // Engine-Build, siehe RunNutritionAudit() weiter unten. Die
        // Ambiguitaetsanalyse (09 §5) laeuft im Engine-Build mit.
        ChefZ_RecipeEngine.Get().Build(m_Recipes, m_Devices, m_SelectorCtx, GetSettings(), m_Report);

        // QUALITAETSREGELN DER REZEPTE (12 §3).
        //
        // Zwingend NACH dem Engine-Build: die Rohregeln haengen an den
        // KOMPILIERTEN Rezepten, und jede slotId wird gegen die kompilierten
        // Slots geprueft. Der Rezeptcompiler reicht sie ausdruecklich
        // unuebersetzt weiter und begruendet es damit, dass sie dem Quality
        // Manager gehoeren (ChefZ_RecipeCompiler.CompileQuality).
        //
        // Ebenfalls unbedingt: ohne Regeln bekommt ein Gericht seine Stufe
        // allein aus Slotpunkten, Frische, Zutatenqualitaet und
        // Zustandsstrafen. Das ist weniger ChefZ, nicht falsches ChefZ.
        ChefZ_QualityManager.Get().BuildRecipeRules(ChefZ_RecipeEngine.Get(), m_SelectorCtx, m_Report);

        // WERKZEUGE (11 E8) - seit S14.
        //
        // Zwingend VOR dem Processing Manager: 11 §7 weist einen Prozess AB,
        // dessen Werkzeuggruppe unbekannt ist ("ohne Werkzeugpruefung waere er
        // zu leicht ausloesbar"). Ohne gebaute Werkzeugregistry waere JEDE
        // Gruppe unbekannt - und damit jeder Prozess mit Werkzeugbedingung
        // abgewiesen.
        //
        // An keine andere Registry gebunden: eine Werkzeuggruppe nennt
        // Klassennamen und Gruppennamen, sonst nichts. Sie prueft die Klassen
        // ausdruecklich NICHT gegen CfgVehicles - ein Messer aus einem
        // optionalen Modul darf genannt werden, ohne dass es geladen ist
        // (siehe ChefZ_ToolRegistry).
        //
        // Der Aufruf ist unbedingt - auch ohne einen einzigen Eintrag soll die
        // Registry "bereit und leer" sein.
        ChefZ_ToolRegistry.Get().Build(m_ToolGroups, m_Report);

        // VERARBEITUNG (11 §5, BOOT) - seit S14.
        //
        // Zwingend NACH BuildSelectorContext(): ein Transform traegt
        // Eingangsslots im REZEPTFORMAT (11 E4) und wird mit DEMSELBEN
        // Selektorcompiler uebersetzt - der braucht Kategoriebits,
        // Rueckwaertsindizes und die Spezifitaetsgewichte.
        //
        // Zwingend NACH der Werkzeugregistry, siehe oben.
        //
        // Zwingend NACH dem Zustandsmanager: die Selektoren der Eingaenge
        // pruefen Zustaende, und "setState" an einem Ergebnis nennt einen.
        //
        // NICHT abhaengig von der Rezept-Engine, und umgekehrt auch nicht:
        // Verarbeitung und Kochen sind zwei getrennte Pfade (11 §5, "Der
        // Processing Manager beruehrt Cooking nicht"). Die Reihenfolge hier
        // ist deshalb frei gewaehlt - sie steht nach den Rezepten, damit der
        // Ladebericht die teurere Arbeit zuerst zeigt.
        //
        // Der Aufruf ist unbedingt - auch ohne einen einzigen Prozess soll der
        // Manager "bereit und leer" sein. Dann bieten Stationen nichts an und
        // sind inerte Deko (11 §7, erste Zeile), und Vanilla-Crafting ist
        // davon ohnehin unberuehrt.
        ChefZ_ProcessingManager.Get().Build(m_Processes, m_Stations, m_Transforms, ChefZ_ToolRegistry.Get(), m_SelectorCtx, GetSettings(), m_Report);

        // BEHAELTER (16 §7, BOOT) - seit S17.
        //
        // An KEINE andere Registry gebunden, und das ist Absicht: ein
        // Behaelter nennt Kategorien und Klassennamen, sonst nichts. Seine
        // Kategorien sind BEHAELTERkategorien und haben mit dem Kategoriebaum
        // aus 04 nichts zu tun - sie entstehen ausschliesslich aus den
        // containerCategories[] der Behaelter selbst. Deshalb gibt es hier
        // keine Reihenfolgeauflage ausser der einen unten.
        //
        // Zwingend VOR dem Portionsmanager, aber nur wegen des Schalters
        // direkt darunter - nicht wegen der Daten.
        //
        // Der Aufruf ist unbedingt - auch ohne einen einzigen Behaelter soll
        // die Registry "bereit und leer" sein. Dann ist jede Abfrage ein
        // ruhiges false, und ein Core ohne Content ist genau das (16 §7,
        // erste Zeile).
        ChefZ_ContainerRegistry.Get().Build(m_Containers, m_Report);

        // Die EINE Nahtstelle zwischen S16 und S17 (siehe Kopf von
        // ChefZ_PortionManager, Abschnitt "Was hier NICHT steht: der
        // Behaelter").
        //
        // Bewusst HasAnyContainer() und nicht IsReady(): "bereit und leer" ist
        // nicht dasselbe wie "vorhanden". Ohne eine einzige Behaelterkategorie
        // waere jede Kategorie unbekannt, und dann gilt 15 §7 Zeile 4
        // woertlich - die Behaelterbedingung ENTFAELLT, statt jede Entnahme zu
        // blockieren ("Lieber entnehmbar ohne Schuessel als gar nicht
        // entnehmbar").
        //
        // Sobald es Behaelter GIBT, gilt dagegen 16 §7 Zeile 2: eine unbekannte
        // Kategorie macht das Gericht nicht portionierbar. Die beiden Entwuerfe
        // widersprechen sich an dieser Stelle scheinbar; aufgeloest wird es so,
        // weil 15 §7 fuer eine Welt OHNE Behaeltersystem geschrieben ist und
        // 16 §7 fuer eine MIT. Die Fallunterscheidung dazu steht in
        // ChefZ_PortionManager.CanTakePortion().
        ChefZ_PortionManager.SetContainerSystemReady( ChefZ_ContainerRegistry.Get().HasAnyContainer());

        // PORTIONEN (15 §6, BOOT) - seit S16.
        //
        // Zwingend NACH dem Engine-Build UND nach dem Processing Manager: die
        // Portionsregistry wird aus den KOMPILIERTEN Ergebnisdefinitionen
        // abgeleitet (15 §6, "aus den Rezept-Outputs"), und die gibt es vorher
        // beidesmal nicht. Vor beiden gebaut waere die Registry leer, jedes
        // Bulk-Gericht ein Item mit Zaehler ohne Entnahmeaktion - ein Ausfall,
        // der aussieht wie fehlender Content.
        //
        // NICHT abhaengig vom Quality Manager, obwohl die Portionszahl seine
        // Stufenwirkungen liest: das geschieht zur LAUFZEIT
        // (ResolvePortionCount), nicht beim Build. GetOrFallback() antwortet
        // dort auch ohne eine einzige Stufe neutral (12 §8).
        //
        // Die Einstellungen kommen mit, weil defaultTakePortionSec die Vorgabe
        // fuer jede Spec ohne eigene Dauer ist (15 §3).
        //
        // Der Aufruf ist unbedingt - auch ohne ein einziges Portionsgericht
        // soll der Manager "bereit und leer" sein. Dann ist IsBulkClass() ein
        // ruhiges false, die Entnahmeaktion erscheint nirgends, und jedes
        // Kochergebnis entsteht als gewoehnliches Item (15 §7, erste Zeile).
        ChefZ_PortionManager.Get().Build(ChefZ_RecipeEngine.Get(), ChefZ_ProcessingManager.Get(), m_Report, GetSettings());

        // BEHAELTERAUDIT (16 §7, Zeile 2) - seit S17.
        //
        // Zwingend NACH dem Portionsmanager: geprueft werden dessen Specs.
        // "Rezept nennt unbekannte Behaelterkategorie -> WARN EINMAL BEIM
        // LADEN mit Rezept-ID." Beim ersten Entnahmeversuch waere es zu spaet:
        // ein Rezept, dessen Kategorie es nicht gibt, matcht nie - und "matcht
        // nie" sieht auf einem laufenden Server exakt aus wie fehlender
        // Content.
        //
        // Er aendert NICHTS - kein Rezept, keine Registry, keinen
        // Balancingwert. Und er bricht ausdruecklich kein Rezept ab: die
        // Kategorie kann aus einem optionalen Modul stammen, das dieser Server
        // nicht geladen hat.
        ChefZ_ContainerRegistry.Get().AuditPortionSpecs(ChefZ_PortionManager.Get(), m_Report);

        // NAEHRWERTAUDIT (13 §5, 13 §6 BOOT).
        //
        // Zwingend ZULETZT: er laeuft ueber die KOMPILIERTEN Rezepte und
        // vergleicht ihre Ergebnisklassen mit CfgVehicles. Vor dem
        // Engine-Build gaebe es kein Rezept zu pruefen.
        //
        // Er aendert NICHTS - kein Item, keine Registry, keinen
        // Balancingwert (13 E1). Was er tut, steht danach im Startlog: welche
        // Ergebnisklasse lautlos nicht saettigt (01 V7), und welches Gericht
        // von seinen Zutaten weit abweicht.
        //
        // 13 E5 begruendet den Zeitpunkt: beim Kochen waere es billiger, aber
        // bei einem Gericht mit fehlendem Nutrition-Block kaeme der Befund
        // NIE - weil niemand merkt, dass er nicht satt wird.
        RunNutritionAudit();

        // EREIGNISSE UND FAEHIGKEITEN (17 §3.3, §9) - seit S13.
        //
        // Zuletzt und ohne Bedingung: die Schicht haengt an keiner Registry
        // und darf an keiner haengen. Ein Comp-Modul meldet sich in seinem
        // EIGENEN Boot an, und die Reihenfolge der Mods ist nicht
        // vorhersagbar - der Bus muss also auch dann funktionieren, wenn er
        // vor dieser Zeile schon Anmeldungen entgegengenommen hat (17 §9,
        // Zeile "Raise vor LoadAll()").
        WireCapabilityLayer();

        FreezeAll();
    }

    /**
     * Die Ereignis- und Faehigkeitsschicht verdrahten (17 §3.3, 12 E6,
     * 08 §7 Schritt 2c).
     *
     * Drei Einhaengepunkte, und jeder hat einen anderen Vertrag:
     *
     *   Bus       bekommt nur Regler (Tiefe, Zeitmessung).
     *   Probe     beantwortet QUALITAETSREGELN. Ohne Anbieter FALSE, damit
     *             eine capability-Regel nicht zuendet (12 §8).
     *   Gate      beantwortet ANFORDERUNGEN im Rezeptfilter. Ohne Anbieter
     *             gilt der Config-Default, und der blockiert nichts
     *             (17 §3.3).
     *
     * Alle drei sind Objekte ohne Zustand ausser ihrem Verweis auf die
     * Registry - sie werden bei jedem LoadAll neu gesetzt, weil sich die
     * Regler geaendert haben koennen. Die registrierten ANBIETER bleiben
     * dabei unberuehrt: sie gehoeren dem Comp-Modul, nicht dem Config
     * Manager.
     */
    private void WireCapabilityLayer()
    {
        ChefZ_CoreSettingsDef settings = GetSettings();

        ChefZ_EventBus.Get().Configure(settings);
        ChefZ_CapabilityRegistry.Get().Configure(settings);

        ChefZ_QualityManager.Get().SetCapabilityProbe(new ChefZ_RegistryCapabilityProbe());
        ChefZ_CapabilityGate.SetActive(new ChefZ_RegistryCapabilityGate());
    }

    /**
     * Das Naehrwertaudit (13 §5), und was der Config Manager mit seinem
     * Ergebnis macht: NICHTS ausser es zu melden.
     *
     * Ausdruecklich KEINE Rueckwirkung auf den Gesundheitszustand. Zwei
     * Gruende, und beide sind wichtig genug, um hier zu stehen:
     *
     *   1. Die Befunde, die WIRKLICH ein Rezept unbrauchbar machen
     *      (MISSING_BLOCK, SCOPE_ZERO), hat der ChefZ_RecipeCompiler bereits
     *      als Ladefehler gezaehlt und das Rezept abgewiesen (08 §8). Sie hier
     *      ein zweites Mal zu zaehlen, koennte einen Server ueber die
     *      SAFE_MODE-Schwelle schieben, ohne dass ein zweiter Fehler
     *      vorliegt - dieselbe Ursache, doppelt gewichtet.
     *
     *   2. Alles andere ist Balancing. Ein Gericht, das 30 Prozent von seinem
     *      Sollwert abweicht, ist eine Entwurfsentscheidung oder ein
     *      Versehen - aber in keinem Fall ein Grund, das Kochsystem eines
     *      Servers abzuschalten. 13 E1: der Core rechnet und meldet.
     */
    private void RunNutritionAudit()
    {
        array<ChefZ_NutritionFinding> findings;
        ChefZ_NutritionManager.Get().AuditAllRecipes(findings);
    }

    private void FillRegistry(ChefZ_RegistryBase registry, string kind, ChefZ_ValidationContext vctx, ChefZ_CompileContext cctx)
    {
        array<ref ChefZ_Record> records = m_Sink.GetRecords(kind);
        for (int i = 0; i < records.Count(); i++)
        {
            ChefZ_Record rec = records.Get(i);
            if (!rec)
                continue;

            rec.ResolveDefaults();

            if (rec.disabled)
            {
                // 02 §5.1: das Overlay kann abschalten, ohne zu loeschen.
                if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
                    ChefZ_Log.Debug(ChefZ_LogChannel.CONFIG, "Abgeschaltet: " + rec.Describe());
                continue;
            }

            if (!rec.Validate(vctx))
                continue;                       // Validate hat bereits gemeldet

            rec.Compile(cctx);

            if (!registry.Add(rec))
            {
                m_Report.AddError(rec.sourceRef, rec.id, "Record konnte nicht in die Registry \"" + kind + "\" aufgenommen werden " + "(bereits vorhanden oder eingefroren).");
            }
        }
    }

    /**
     * Sync-Ordinale ableiten (03 §4).
     *
     * ZWINGEND nur aus Rang 1: Client und Server lesen dieselbe Game-Config,
     * sortieren gleich und kommen so ohne jede Uebertragung auf denselben
     * Ordinal. Ein Record aus Rang 2 oder 3 waere auf beiden Seiten zwar
     * vorhanden, aber der Overlay-Teil eben nicht - deshalb wird hier hart
     * nach sourceRank gefiltert und ein Verstoss gemeldet.
     */
    private void BuildIdentities(ChefZ_RegistryBase registry, ChefZ_IdentityMap map, string kind)
    {
        array<string> rank1Ids = new array<string>();

        for (int i = 0; i < registry.Count(); i++)
        {
            ChefZ_Record rec = registry.GetAt(i);
            if (!rec)
                continue;

            if (rec.sourceRank == ChefZ_SourceRank.CONFIG_CPP)
            {
                rank1Ids.Insert(rec.id);
                continue;
            }

            // Rang 2 darf sync-relevante Arten nicht stellen: die Datei liegt
            // zwar auf beiden Seiten, aber 03 §4 verlangt EINE Quelle, die
            // beide Seiten garantiert identisch lesen. Das ist die Game-Config.
            m_Report.AddError(rec.sourceRef, rec.id, "Die Art \"" + kind + "\" ist sync-relevant und darf ausschliesslich aus " + "Rang 1 (CfgChefZ*-Klassenbaum) kommen (03 §4). Der Record bleibt geladen, " + "bekommt aber keinen Sync-Ordinal und ist damit clientseitig nicht " + "darstellbar.");
        }

        ChefZ_StringOrder.SortAscending(rank1Ids);
        map.Build(rank1Ids, m_Report, ChefZ_RecordKind.SyncLimit(kind));
    }

    private void FreezeAll()
    {
        m_Categories.Freeze();
        m_Tags.Freeze();
        m_States.Freeze();
        m_QualityTiers.Freeze();
        m_ToolGroups.Freeze();
        m_Devices.Freeze();
        m_Containers.Freeze();
        m_Ingredients.Freeze();
        m_Nutrition.Freeze();
        m_Preservation.Freeze();
        m_Processes.Freeze();
        m_Stations.Freeze();
        m_Transforms.Freeze();
        m_Recipes.Freeze();
    }

    //==========================================================================
    // Gesundheit
    //==========================================================================

    private void DecideHealth()
    {
        int errors = m_Report.ErrorCount();

        if (errors == 0)
        {
            m_Health = ChefZ_ConfigHealth.OK;
            return;
        }

        if (m_Settings.strictMode)
        {
            EnterSafeMode("strictMode ist eingeschaltet und es gab " + errors.ToString() + " Fehler. Das ist der ausdrueckliche Notausgang (02 E4).");
            return;
        }

        if (errors > m_Settings.safeModeErrorThreshold)
        {
            EnterSafeMode(errors.ToString() + " Fehler ueberschreiten die Schwelle " + m_Settings.safeModeErrorThreshold.ToString() + ".");
            return;
        }

        m_Health = ChefZ_ConfigHealth.DEGRADED;
    }

    /**
     * 02 §8: alle Registries geleert, Core inert. "Lieber ganz Vanilla als
     * halb ChefZ."
     *
     * Genau EINE grosse Meldung. Die Einzelfehler stehen bereits im Bericht;
     * sie zu wiederholen wuerde die eine Zeile verstecken, auf die es ankommt.
     */
    private void EnterSafeMode(string reason)
    {
        m_Health = ChefZ_ConfigHealth.SAFE_MODE;

        m_Categories.ClearAll();
        m_Tags.ClearAll();
        m_States.ClearAll();
        m_QualityTiers.ClearAll();
        m_ToolGroups.ClearAll();
        m_Devices.ClearAll();
        m_Containers.ClearAll();
        m_Ingredients.ClearAll();
        m_Nutrition.ClearAll();
        m_Preservation.ClearAll();
        m_Processes.ClearAll();
        m_Stations.ClearAll();
        m_Transforms.ClearAll();
        m_Recipes.ClearAll();

        // Der Kategoriebaum haengt an den Registries und muss mit ihnen
        // fallen. Ein zurueckbleibender Baum wuerde Zugehoerigkeiten
        // behaupten, zu denen es keine Daten mehr gibt - und genau das ist
        // "halb ChefZ".
        //
        // Neu gebaut aus dem Nichts und nicht bloss zurueckgesetzt: der
        // richtige Zustand im SAFE_MODE ist "bereit und leer", nicht "nicht
        // gebaut". Jede Abfrage liefert dann ruhig false, statt einen Fehler
        // ueber einen fehlenden Aufbau zu melden, den es nie geben wird.
        ChefZ_CategoryManager.Get().Build(null, null, null);

        // Dasselbe fuer die Zustaende. Ein stehengebliebener Zustand haette
        // keinen Datensatz mehr hinter sich, wuerde aber weiter projizieren
        // und weiter Tags implizieren - "halb ChefZ" auf einem Item, das im
        // Spielstand liegt. Danach liefert ChefZ_GetState fuer jedes Item
        // INVALID, und das ist die richtige Antwort im SAFE_MODE.
        ChefZ_StateManager.Get().Build(null, null, null);

        // Dasselbe fuer die Qualitaetsstufen. Eine stehengebliebene Stufe
        // haette keinen Datensatz mehr hinter sich, wuerde aber weiter
        // Ausbeute und Haltbarkeit veraendern und weiter Tags vergeben -
        // "halb ChefZ" auf einem Item, das im Spielstand liegt. Danach
        // liefert ResolveTier fuer jedes Gericht INVALID, und das ist die
        // richtige Antwort im SAFE_MODE.
        ChefZ_QualityManager.Get().Build(null, null, null, null);

        // Dasselbe fuer die Zutatenbindungen, aus demselben Grund: eine
        // stehengebliebene Bindung wuerde einem Item Kategorien und Tags
        // zusprechen, zu denen es keine Daten mehr gibt.
        ChefZ_IngredientManager.Get().Build(null, null, null);

        // Dasselbe fuer die Haltbarkeitsregeln. Eine stehengebliebene Regel
        // waere hier besonders unangenehm: sie wuerde den Verfall eines Items
        // weiter skalieren, obwohl es die Kategorie, den Tag oder den Zustand,
        // auf den sie sich beruft, nicht mehr gibt. Danach liefert
        // ComputeDecayScale fuer jedes Item den neutralen Faktor 1.0, und der
        // Verfall ist bitgenau Vanilla - die richtige Antwort im SAFE_MODE
        // (14 §8).
        ChefZ_PreservationManager.Get().Build(null, null, null);

        // Und dasselbe fuer die Rezepte. Ein stehengebliebener Rezeptindex
        // waere der gefaehrlichste Rest von allen: er wuerde weiterkochen und
        // dabei Zutaten verbrauchen, deren Kategorien es nicht mehr gibt.
        // Danach ist HasAnyRecipeFor() ein ruhiges false, und der Kochhook
        // kehrt nach einem Bool-Test zurueck (08 §8).
        ChefZ_RecipeEngine.Get().Build(null, null, null, null, null);

        // Und die Naehrwertangaben. Sie koennen zwar nichts kaputt machen -
        // der Manager schreibt zur Laufzeit nichts (13 E1) -, aber ein
        // stehengebliebener Bestand wuerde ein Adminkommando mit Befunden
        // beantworten, die sich auf Rezepte beziehen, die es nicht mehr gibt.
        ChefZ_NutritionManager.Get().Build(null, null, null);

        // Und die Verarbeitung (11, S14). Ein stehengebliebener Transform
        // waere so gefaehrlich wie ein stehengebliebener Rezeptindex: er
        // wuerde an einer Station weiterlaufen und dabei Zutaten verbrauchen,
        // deren Kategorien es nicht mehr gibt. Danach bieten Stationen nichts
        // mehr an, ChefZ_ActionProcessAtStation.ActionCondition ist ein
        // ruhiges false, und die Stationen in der Welt sind inerte Deko
        // (11 §7, erste Zeile). Laufende Jobs brechen beim naechsten Tick ab -
        // OHNE VERLUST, die Eingaenge bleiben liegen (11 §6).
        //
        // Die Werkzeugregistry faellt mit: eine stehengebliebene Gruppe wuerde
        // einem Messer eine Zugehoerigkeit zusprechen, zu der es keine Daten
        // mehr gibt.
        ChefZ_ToolRegistry.Get().Build(null, null);
        ChefZ_ProcessingManager.Get().Build(null, null, null, null, null, null, null);

        // Und die Portionsregistry (15, S16). Ein stehengebliebener Eintrag
        // wuerde einem Bulk-Gericht im Spielstand weiterhin eine Entnahme
        // anbieten und dabei ein Portionsitem erzeugen, dessen Rezept es nicht
        // mehr gibt - eine Nahrungsquelle ohne Datengrundlage. Danach ist
        // ChefZ_IsBulk() fuer jedes Item false, die Aktion erscheint nirgends,
        // und die Gerichte in der Welt bleiben als gewoehnliche Items
        // verzehrbar (15 §7: kein Loeschen von Spielerbesitz).
        ChefZ_PortionManager.Get().Build(null, null, null, null);

        // Und die Behaelterregistry (16, S17). Ein stehengebliebener Eintrag
        // wuerde beim Verzehr eines Gerichts aus dem Spielstand weiterhin
        // einen Teller erzeugen - Items aus dem Nichts, auf einem Server, der
        // ChefZ gerade abgeschaltet hat. Danach ist ResolveEmptyClass() fuer
        // jede Klasse INVALID, und es kommt schlicht nichts mehr zurueck.
        //
        // Der Schalter des Portionsmanagers wird MITGEZOGEN: er sagt sonst
        // weiterhin "das Behaeltersystem steht", waehrend es keine einzige
        // Kategorie mehr kennt.
        ChefZ_ContainerRegistry.Get().Build(null, null);
        ChefZ_PortionManager.SetContainerSystemReady(false);

        // Und der Faehigkeitsfilter (17 §3.3, S13). Ein stehengebliebenes Gate
        // wuerde im SAFE_MODE Rezepte sperren, die der Core ohnehin nicht mehr
        // kennt - eine Entscheidung ueber einen Bestand, den es nicht mehr
        // gibt. Danach blockiert nichts mehr, was die richtige Antwort ist:
        // im SAFE_MODE kocht ausschliesslich Vanilla.
        //
        // Die registrierten ANBIETER und ABONNENTEN bleiben ausdruecklich
        // stehen. Sie gehoeren fremden Modulen, nicht dem Config Manager, und
        // ein Core, der sie loeschte, machte ein fremdes Modul kaputt, statt
        // sich selbst abzuschalten. Ausgeloest wird ohnehin nichts mehr - es
        // gibt kein Rezept, das abschliessen koennte.
        ChefZ_CapabilityGate.ClearActive();
        ChefZ_QualityManager.Get().SetCapabilityProbe(null);

        ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, "SAFE MODE. " + reason + " Alle ChefZ-Registries sind geleert, der Core ist inert, " + "Vanilla-Kochen laeuft unveraendert weiter. Die Einzelfehler stehen oben im " + "Ladebericht.");
    }

    private void ReportSummary()
    {
        // Format woertlich aus 02 §8.
        string line = "slices=" + m_SliceCount.ToString() + " files=" + m_FileCount.ToString() + " records=" + m_Sink.GetSubmittedCount().ToString() + " ok=" + m_Sink.GetAcceptedCount().ToString() + " rejected=" + m_Sink.GetRejectedCount().ToString() + " patched=" + m_Sink.GetPatchedCount().ToString() + " health=" + HealthName(m_Health)
                    + " in " + m_LoadMillis.ToString() + "ms";

        // Geht an der Stufenpruefung vorbei: 18 §4 verlangt diese Zeile IMMER,
        // auch bei Erfolg und auch bei Stufe WARN.
        PrintToRPT(ChefZ_Log.PREFIX + "[CONFIG] " + line);
        m_Report.PrintSummary();

        if (m_Settings.logReportToFile && m_IsServer)
            m_Report.WriteToDefaultFile();

        ChefZ_Log.Flush();
    }

    static string HealthName(ChefZ_ConfigHealth health)
    {
        switch (health)
        {
            case ChefZ_ConfigHealth.UNINITIALIZED: return "UNINITIALIZED";
            case ChefZ_ConfigHealth.OK:            return "OK";
            case ChefZ_ConfigHealth.DEGRADED:      return "DEGRADED";
            case ChefZ_ConfigHealth.SAFE_MODE:     return "SAFE_MODE";
        }
        return "?";
    }

    //==========================================================================
    // Oeffentliche Abfragen (02 §5.3)
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    ChefZ_ConfigHealth GetHealth()
    {
        return m_Health;
    }

    ChefZ_LoadReport GetReport()
    {
        return m_Report;
    }

    //! Nie null - notfalls Code-Defaults (02 §5.3).
    /**
     * Baut den Selektor-Kompilierkontext aus den fertigen Managern und den
     * CoreSettings (07 §5, 07 §7, 09 §3).
     *
     * Die drei Werte, die hier gesetzt werden, sind genau die, die 07 als
     * konfigurierbar nennt und die sonst still auf Code-Defaults zurueckfielen:
     * Tiefengrenze, Vorgabe fuer excludeStates und die Spezifitaetsgewichte.
     */
    private void BuildSelectorContext()
    {
        // Ueber GetSettings() und nicht ueber m_Settings: der Aufrufpfad ist
        // heute vollstaendig, aber ein spaeterer darf nicht an einer
        // Nullpruefung scheitern, die es hier umsonst gibt.
        ChefZ_CoreSettingsDef settings = GetSettings();

        m_SelectorCtx = new ChefZ_CompileContext();
        m_SelectorCtx.Init(m_Report);

        ChefZ_ManagerSymbolResolver resolver = new ChefZ_ManagerSymbolResolver();
        resolver.Prepare(ChefZ_CategoryManager.Get(), ChefZ_IngredientManager.Get(), this, ChefZ_QualityManager.Get());
        m_SelectorCtx.SetResolver(resolver);

        m_SelectorCtx.SetMaxSelectorDepth(settings.maxSelectorDepth);

        // 07 E5: Der Seed {"BURNT","ROTTEN"} nennt Zustaende, die der Core
        // selbst nie anlegt. Sie werden interniert, aber NICHT gegen die
        // Zustandsregistry geprueft: laeuft kein Content, der sie fuehrt,
        // laufen sie ins Leere - und das ist harmlos. Ein Fehler waere es nur
        // im umgekehrten Fall, und den faengt der Slotcompiler ab.
        array<ChefZ_Sym> excluded = new array<ChefZ_Sym>();
        if (settings.defaultExcludedStates)
        {
            for (int i = 0; i < settings.defaultExcludedStates.Count(); i++)
            {
                ChefZ_Sym sym = ChefZ_SymbolTable.Intern(settings.defaultExcludedStates.Get(i));
                if (ChefZ_SymbolTable.IsValid(sym) && excluded.Find(sym) < 0)
                    excluded.Insert(sym);
            }
        }
        m_SelectorCtx.SetDefaultExcludedStates(excluded);

        // 09 §3: Die Gewichte kommen seit S6 aus Core.json - genauer aus dem
        // Block "priorityWeights", feldweise ueber die Code-Defaults gelegt.
        // Was der Betreiber nicht nennt, behaelt seinen Default (09 §7).
        //
        // Genau EIN Aufruf im ganzen Boot. BuildPriorityWeights() meldet zwei
        // Zustaende, die die Grundregel aus §16 aushebeln (priorityScale > 1.0,
        // alle Gewichte 0); ein zweiter Aufruf wuerde beide Warnungen doppelt
        // in den Ladebericht schreiben. Die Recipe Engine holt sich denselben
        // Satz deshalb aus DIESEM Kontext.
        ChefZ_PriorityWeights weights = settings.BuildPriorityWeights(m_Report);
        m_SelectorCtx.SetWeights(weights);

        // 04 E4 / 09 E3: derselbe Gewichtssatz muss den Kategoriebaum
        // erreichen, sonst rechnete der Baum mit anderen Zahlen als der
        // Selektorcompiler.
        ChefZ_CategoryManager.Get().SetSpecificityWeights(weights.wCategoryBase, weights.wCategoryPerDepth);
    }

    /**
     * Der Kontext fuer ChefZ_SelectorCompiler. null, solange nicht geladen
     * wurde - der Aufrufer prueft das, statt hier einen halben Kontext zu
     * bekommen, der nichts aufloest.
     */
    ChefZ_CompileContext SelectorContext()
    {
        return m_SelectorCtx;
    }

    ChefZ_CoreSettingsDef GetSettings()
    {
        if (!m_Settings)
        {
            m_Settings = new ChefZ_CoreSettingsDef();
            m_Settings.ResolveDefaults();
        }
        return m_Settings;
    }

    /**
     * Die eine Frage, die jeder Hook zuerst stellt.
     *
     * Ein Bool-Test - 19 S7: "Bei leerem Rezeptbestand kostet der Hook einen
     * Bool-Test."
     */
    bool IsActive()
    {
        if (!m_Ready)
            return false;
        if (m_Health == ChefZ_ConfigHealth.SAFE_MODE)
            return false;
        return m_Settings.enabled;
    }

    ChefZ_Registry<ChefZ_CategoryDef>     Categories()    { return m_Categories; }
    ChefZ_Registry<ChefZ_TagDef>          Tags()          { return m_Tags; }
    ChefZ_Registry<ChefZ_StateDef>        States()        { return m_States; }
    ChefZ_Registry<ChefZ_QualityTierDef>  QualityTiers()  { return m_QualityTiers; }
    ChefZ_Registry<ChefZ_ToolGroupDef>    ToolGroups()    { return m_ToolGroups; }
    ChefZ_Registry<ChefZ_DeviceDef>       Devices()       { return m_Devices; }
    ChefZ_Registry<ChefZ_ContainerDef>    Containers()    { return m_Containers; }
    ChefZ_Registry<ChefZ_IngredientDef>   Ingredients()   { return m_Ingredients; }
    ChefZ_Registry<ChefZ_NutritionDef>    Nutrition()     { return m_Nutrition; }
    ChefZ_Registry<ChefZ_PreservationDef> Preservation()  { return m_Preservation; }
    ChefZ_Registry<ChefZ_ProcessDef>      Processes()     { return m_Processes; }
    ChefZ_Registry<ChefZ_StationDef>      Stations()      { return m_Stations; }
    ChefZ_Registry<ChefZ_TransformDef>    Transforms()    { return m_Transforms; }
    ChefZ_Registry<ChefZ_RecipeDef>       Recipes()       { return m_Recipes; }

    ChefZ_IdentityMap StateIdentities()   { return m_StateIdentities; }
    ChefZ_IdentityMap QualityIdentities() { return m_QualityIdentities; }

    //! Registry zu einer Art. null fuer coreSettings - die ist ein einzelner
    //! Datensatz und keine Registry.
    ChefZ_RegistryBase RegistryOf(string kind)
    {
        if (kind == ChefZ_RecordKind.CATEGORY)      return m_Categories;
        if (kind == ChefZ_RecordKind.TAG)           return m_Tags;
        if (kind == ChefZ_RecordKind.STATE)         return m_States;
        if (kind == ChefZ_RecordKind.QUALITY_TIER)  return m_QualityTiers;
        if (kind == ChefZ_RecordKind.TOOL_GROUP)    return m_ToolGroups;
        if (kind == ChefZ_RecordKind.DEVICE)        return m_Devices;
        if (kind == ChefZ_RecordKind.CONTAINER)     return m_Containers;
        if (kind == ChefZ_RecordKind.INGREDIENT)    return m_Ingredients;
        if (kind == ChefZ_RecordKind.NUTRITION)     return m_Nutrition;
        if (kind == ChefZ_RecordKind.PRESERVATION)  return m_Preservation;
        if (kind == ChefZ_RecordKind.PROCESS)       return m_Processes;
        if (kind == ChefZ_RecordKind.STATION)       return m_Stations;
        if (kind == ChefZ_RecordKind.TRANSFORM)     return m_Transforms;
        if (kind == ChefZ_RecordKind.RECIPE)        return m_Recipes;
        return null;
    }

    int TotalRecordCount()
    {
        int total = 0;
        array<string> order = ChefZ_RecordKind.LoadOrder();
        for (int i = 0; i < order.Count(); i++)
        {
            ChefZ_RegistryBase r = RegistryOf(order.Get(i));
            if (r)
                total = total + r.Count();
        }
        return total;
    }

    void DumpRegistries(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("ChefZ Registries  health=" + HealthName(m_Health) + "  records=" + TotalRecordCount().ToString() + "  fehler=" + m_Report.ErrorCount().ToString() + "  warnungen=" + m_Report.WarnCount().ToString());
        outLines.Insert("Einstellungen: " + m_Settings.ToDebugString());

        array<string> order = ChefZ_RecordKind.LoadOrder();
        for (int i = 0; i < order.Count(); i++)
        {
            ChefZ_RegistryBase r = RegistryOf(order.Get(i));
            if (r)
                r.DebugDump(outLines);
        }

        // Baum, Zutatenbindungen und Stufenleitern stehen nicht in den
        // Registries - sie sind das Ergebnis ihrer Auswertung (04 §4, 05 §4,
        // 12 §6) und gehoeren deshalb eigens in den Auszug.
        ChefZ_CategoryManager.Get().DumpTree(outLines);
        ChefZ_IngredientManager.Get().DumpIngredients(outLines);
        ChefZ_QualityManager.Get().DumpTiers(outLines);

        // Die Naehrwertangaben und die Befunde des Startaudits (13 §7). Sie
        // stehen ebenfalls nicht in den Registries: die Angaben schon, aber
        // ihre AUFLOESUNG (Klasse -> Kategorie -> Tag) und die Befunde sind
        // das Ergebnis der Auswertung.
        ChefZ_NutritionManager.Get().DumpRecords(outLines);
        ChefZ_NutritionManager.Get().DumpFindings(outLines);
    }

    //==========================================================================
    // Overlay-Sync (02 §5.3)
    //==========================================================================

    /**
     * NOCH NICHT IMPLEMENTIERT - und das steht hier, statt still nichts zu tun.
     *
     * Vorgesehen ist das Muster von CfgGameplayHandler.SyncDataSend
     * (3_Game/DayZ/CfgGameplayHandler.c:88): ein RPC mit dem Rang-3-Delta beim
     * Spielerbeitritt.
     *
     * Warum in S2 noch nicht: der Client braucht das Delta ausschliesslich fuer
     * Anzeigenamen und ActionCondition. Beides gibt es erst, wenn es Content
     * gibt - und die Uebertragung braucht eine eigene RPC-Kennung sowie ein
     * versioniertes Uebertragungsformat, das mit den Records aus S3 bis S6
     * mitwaechst. Sie jetzt zu bauen hiesse, sie zweimal zu bauen.
     *
     * Bis dahin gilt: der Client kennt Rang 1 und 2 (dieselben PBOs) und damit
     * alles, was er fuer eine Anzeige braucht. Ein Overlay-Wert wirkt
     * serverseitig sofort; clientseitig sieht der Spieler bis dahin den Wert
     * aus dem Mod. Kein Spielentscheid haengt daran - die trifft ausnahmslos
     * der Server.
     */
    void SyncOverlayTo(PlayerIdentity identity)
    {
        ChefZ_Log.Once(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.CONFIG, "config.overlaysync.pending", "Overlay-Sync ist noch nicht aktiv. Der Client arbeitet mit Rang 1 und 2; " + "Spielentscheidungen trifft ausschliesslich der Server.");
    }

    void OnOverlayRPC(ParamsReadContext ctx)
    {
        ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.CONFIG, "config.overlayrpc.unexpected", "Overlay-RPC empfangen, obwohl der Sync nicht aktiv ist - verworfen. " + "Das deutet auf einen Versionsunterschied zwischen Server und Client hin.");
    }
}
