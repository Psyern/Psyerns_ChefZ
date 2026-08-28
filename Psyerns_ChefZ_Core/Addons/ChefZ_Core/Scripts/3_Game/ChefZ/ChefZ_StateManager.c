//==============================================================================
// ChefZ_StateManager - die Zustandsregistry zur Laufzeit
//
// Entwurf: 06 §4.2 (Schnittstelle woertlich), 06 §3 (Projektionsregel),
// 06 §5 (Datenfluss), 06 §7 (Fehlerverhalten), 06 E5 (Vanilla gewinnt bei
// BURNED und ROTTEN), 03 §2/§4 (drei Darstellungen, drei Zwecke),
// 03 E2 (Persistenz ueber Name.Hash()), 03 E3 (Sync-Ordinal abgeleitet).
//
// ---------------------------------------------------------------------------
// Die drei Zahlen und warum sie hier zusammenlaufen
// ---------------------------------------------------------------------------
//   Laufzeit     ChefZ_Sym      Vergleich im Matcher, schnell, fluechtig
//   Persistenz   id.Hash()      stabil ueber Content-Updates -> OnStoreSave
//   Netz-Sync    syncOrdinal    klein und symmetrisch abgeleitet -> NetSync
//
// Der Sync-Ordinal kommt aus der ChefZ_IdentityMap, die der Config Manager
// baut (03 §4): AUSSCHLIESSLICH aus Rang 1, sortiert, Position ab 1. Er wird
// hier NICHT zweitgerechnet - zwei Ableitungen derselben Groesse waeren zwei
// Wahrheiten, und die zweite faellt erst beim Spieler auf.
//
// Der Persistenz-Hash dagegen wird hier ueber ALLE geladenen Zustaende
// gefuehrt, nicht nur ueber die aus Rang 1. Grund: ein Zustand aus Rang 2 ist
// ein Konfigurationsfehler (der Config Manager meldet ihn als ERROR), aber ein
// Item, das ihn traegt, liegt bereits im Spielstand. Sein Hash muss lesbar
// bleiben, sonst zerstoert ein Konfigurationsfehler stillschweigend Daten.
// Weniger ChefZ ist erlaubt, falsches ChefZ nicht (02 §8).
//
// KEIN CONTENT: dieser Manager definiert keinen einzigen Zustand. Alles kommt
// aus der Registry.
//
// Layer: 3_Game. Er liest Registries und kennt keinen Engine-Typ - kein
// ItemBase, kein FoodStage, kein Enum aus 4_World.
//==============================================================================

class ChefZ_StateManager : Managed
{
    private static ref ChefZ_StateManager s_Instance;

    //--- Bestand, indiziert ueber ChefZ_Sym ----------------------------------
    private ref map<int, ChefZ_StateDef>       m_BySym;        // KEIN ref-Wert:
                                                               // Eigentuemer ist
                                                               // die Registry.
    private ref map<int, ref array<ChefZ_Sym>> m_ImpliedBySym; // geprueft
    private ref map<int, int>                  m_SymByHash;    // Persistenz
    private ref map<int, int>                  m_SymByStage;   // 06 §3, Schritt 3
    private ref array<ChefZ_Sym>               m_Order;        // stabile Folge

    //! Die Ordinaltabelle des Config Managers. Bewusst OHNE ref: sie gehoert
    //! ihm, ihre Lebensdauer ist laenger als die dieses Managers, und ein
    //! zweiter starker Verweis waere ein Zyklus ohne Gewinn.
    private ChefZ_IdentityMap m_Identities;

    private ref ChefZ_StateDef m_Fallback;

    private bool m_Ready;
    private bool m_NotReadyLogged;
    private bool m_QuietForTest;
    private int  m_RejectedCount;

    //! Kategoriebaum des Tests statt des Singletons - dieselbe Loesung wie im
    //! ChefZ_IngredientManager. Ohne ihn muesste der Selbsttest den Singleton
    //! umbauen und damit den echten Bestand des Servers anfassen.
    private ChefZ_CategoryManager m_CategoriesForTest;

    //--------------------------------------------------------------------------

    void ChefZ_StateManager()
    {
        m_BySym        = new map<int, ChefZ_StateDef>();
        m_ImpliedBySym = new map<int, ref array<ChefZ_Sym>>();
        m_SymByHash    = new map<int, int>();
        m_SymByStage   = new map<int, int>();
        m_Order        = new array<ChefZ_Sym>();
        m_QuietForTest = false;

        BuildFallback();
        ResetState();
    }

    static ChefZ_StateManager Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_StateManager();
        return s_Instance;
    }

    /**
     * Der Rueckfall-Datensatz aus 06 §4.2 (GetOrFallback "nie null").
     *
     * Er ist KEIN Content: er hat keine ID, keine Projektion, keine Tags und
     * wird nie in die Registry aufgenommen. Er existiert allein, damit ein
     * Leser im heissen Pfad bedingungslos auf Felder zugreifen kann, statt
     * jede Zeile mit einer Nullpruefung zu umstellen.
     *
     * Seine Werte sind die neutralsten, die es gibt: essbar, nicht terminal,
     * nicht konservierend, Verderb unveraendert, keine Projektion. Ein Item mit
     * unbekanntem Zustand verhaelt sich damit wie eines ohne Zustand - und
     * genau das ist gemeint.
     */
    private void BuildFallback()
    {
        m_Fallback = new ChefZ_StateDef();
        m_Fallback.id        = "";
        m_Fallback.sourceRef = "ChefZ_StateManager (Rueckfall)";
        m_Fallback.MarkExplicit("edible");
        m_Fallback.MarkExplicit("terminal");
        m_Fallback.MarkExplicit("preserved");
        m_Fallback.edible    = true;
        m_Fallback.terminal  = false;
        m_Fallback.preserved = false;
        m_Fallback.ResolveDefaults();
        m_Fallback.projectedStage = ChefZ_VanillaStage.NONE;
    }

    //==========================================================================
    // Aufbau
    //==========================================================================

    /**
     * Baut den Bestand. Einmal beim Boot, danach unveraenderlich.
     *
     * Abweichung von der Signatur in 06 §4.2: die ChefZ_IdentityMap kommt als
     * dritter Parameter dazu. Sie ist keine Zutat des Zustandssystems, sondern
     * das Ergebnis von 03 §4, und den Ordinal hier ein zweites Mal abzuleiten
     * hiesse, dieselbe Groesse an zwei Orten zu berechnen. Der Parameter darf
     * null sein - dann liefern GetSyncOrdinal/FromSyncOrdinal 0 bzw. INVALID
     * und melden einmal. Ein Zustand ohne Ordinal ist clientseitig nicht
     * darstellbar, serverseitig aber voll benutzbar.
     *
     * Der Aufruf ist beim Boot UNBEDINGT: auch ohne einen einzigen Zustand
     * soll der Manager "bereit und leer" sein. Sonst antwortete er auf jede
     * Abfrage mit dem Fehler "vor Build aufgerufen", obwohl schlicht keine
     * Zustaende konfiguriert sind (06 §7, erste Zeile).
     */
    void Build(ChefZ_Registry<ChefZ_StateDef> defs, ChefZ_LoadReport report, ChefZ_IdentityMap identities = null)
    {
        ResetState();
        m_Identities = identities;

        if (!defs || defs.Count() == 0)
        {
            // 06 §7, erste Zeile: kein Fehler. ChefZ_GetState liefert INVALID,
            // Selektoren mit "state" matchen nie, Rezepte ohne
            // Zustandsbedingung funktionieren normal. Vanilla unberuehrt.
            if (report)
                report.AddInfo("Keine Zustaende definiert - das Food State System bleibt leer. " + "Zustands-Selektoren matchen dadurch nie; alles andere ist unberuehrt.");
            m_Ready = true;
            return;
        }

        // Reihenfolge ist Registry.Keys(), also nach ID sortiert (03 §4).
        // Damit ist jede abgeleitete Groesse auf Client und Server gleich.
        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_Sym sym = keys.Get(i);
            ChefZ_StateDef def = defs.Find(sym);
            if (!def)
                continue;

            m_BySym.Set(sym, def);
            m_Order.Insert(sym);

            RegisterHash(def, report);
            RegisterProjection(def, report);
            ResolveImplies(def, report);
        }

        ResolveVanillaBackMapping();

        m_Ready = true;

        if (report)
        {
            string chefzTxt1 = "Zustaende: " + GetStateCount().ToString() + " geladen, " + m_SymByStage.Count().ToString() + " Vanilla-Garstufen rueckabbildbar";
            chefzTxt1 = chefzTxt1 + ", Sync-Ordinale " + GetMaxOrdinal().ToString() + ".";
            report.AddInfo(chefzTxt1);
        }

        LogIfDebug();
    }

    /**
     * Persistenzschluessel (03 E2): id.Hash(), nie der Symbolzaehler.
     *
     * Kollision zweier IDs ist ein Fehler, kein Sonderfall (03 E5). Sie macht
     * die Persistenz mehrdeutig, und "erste gewinnt" verschoebe den Datenfehler
     * in die Zukunft, wo er als "mein Fleisch ist ploetzlich geraeuchert"
     * auftaucht. Beide Eintraege fallen aus der Hashtabelle - die Zustaende
     * selbst bleiben benutzbar, sie sind nur nicht mehr aus einem Spielstand
     * ruecklesbar. Das ist die kleinere Zerstoerung.
     */
    private void RegisterHash(notnull ChefZ_StateDef def, ChefZ_LoadReport report)
    {
        string key = def.id;
        int hash = key.Hash();

        int existing;
        if (m_SymByHash.Find(hash, existing))
        {
            m_SymByHash.Remove(hash);
            m_RejectedCount++;
            Report(report, true, def, "Hash-Kollision mit \"" + ChefZ_SymbolTable.NameOrMark(existing) + "\" (beide Hash " + hash.ToString() + "). BEIDE Zustaende sind ab sofort nicht mehr aus einem " + "Spielstand ruecklesbar; Items mit diesem Zustand fallen beim Laden auf den " + "defaultState ihrer Klasse zurueck. Abhilfe: eine der beiden IDs umbenennen.");
            return;
        }

        m_SymByHash.Set(hash, def.sym);
    }

    /**
     * 06 §7: eine unbekannte Vanilla-Garstufe ist bereits in
     * ChefZ_StateDef.Validate() gemeldet und geloescht worden. Hier bleibt nur
     * die Buchfuehrung fuer die Rueckabbildung.
     */
    private void RegisterProjection(notnull ChefZ_StateDef def, ChefZ_LoadReport report)
    {
        if (!def.HasProjection())
            return;
        if (ChefZ_VanillaStage.IsValid(def.projectedStage))
            return;

        // Kann nach Validate/Compile nicht mehr vorkommen. Wenn doch, hat
        // jemand am Record vorbeigeschrieben - und ein ungueltiger Enumwert in
        // SetFoodStageType waere ein Sync-Fehler (06 §7).
        Report(report, true, def, "projectedStage " + def.projectedStage.ToString() + " liegt ausserhalb von " + ChefZ_VanillaStage.ValidNames() + ". Die Projektion wird abgeschaltet.");
        def.projectedStage = ChefZ_VanillaStage.NONE;
    }

    /**
     * implies[] gegen die Tag-Registry pruefen (04 §6).
     *
     * Ein unbekannter Tag faellt weg, die uebrigen bleiben gueltig. Tags
     * werden nie implizit angelegt - ein implizit angelegter Tag matchte nie
     * und waere stiller toter Code. Dieselbe Regel wie im
     * ChefZ_IngredientManager, und aus demselben Grund.
     */
    private void ResolveImplies(notnull ChefZ_StateDef def, ChefZ_LoadReport report)
    {
        array<ChefZ_Sym> tags = new array<ChefZ_Sym>();
        m_ImpliedBySym.Set(def.sym, tags);

        if (!def.implies)
            return;

        ChefZ_CategoryManager cats = Cats();

        for (int i = 0; i < def.implies.Count(); i++)
        {
            string name = def.implies.Get(i);
            if (name == "")
                continue;

            ChefZ_Sym tag = ChefZ_SymbolTable.Lookup(name);
            if (!cats.TagExists(tag))
            {
                Report(report, false, def, "implies-Tag \"" + name + "\" ist unbekannt und wird fuer diesen Zustand " + "ausgelassen; die uebrigen bleiben gueltig. Tags werden nie implizit " + "angelegt (04 §6).");
                continue;
            }

            if (tags.Find(tag) < 0)
                tags.Insert(tag);
        }
    }

    /**
     * Schritt 3 der Projektionsregel (06 §3): Vanilla-Garstufe -> ChefZ-Zustand.
     *
     * Zwei Wege, in dieser Reihenfolge:
     *
     *   1. Der Zustand mit der kanonischen ID zur Stufe
     *      (ChefZ_VanillaStage.ChefZStateId). Das ist woertlich die Tabelle
     *      aus 06 §3 - RAW->RAW ... BURNED->BURNT.
     *   2. Sonst: der EINZIGE Zustand, der auf diese Stufe projiziert.
     *
     * Warum Weg 2 ueberhaupt existiert: die Projektion ist nicht injektiv.
     * 06 §3 nennt SMOKED und DRIED, die beide auf "Dried" projizieren - eine
     * blosse Umkehrung waere dort mehrdeutig, und Mehrdeutigkeit ist genau
     * das, was die Zustandsermittlung nicht liefern darf. Deshalb entscheidet
     * die kanonische ID zuerst, und die Rueckwaertssuche greift nur dort, wo
     * sie eindeutig ist. Bleibt es mehrdeutig, gibt es fuer diese Stufe keine
     * Rueckabbildung - Schritt 3 faellt aus, Schritt 4 liefert INVALID. Das
     * ist weniger ChefZ, nicht falsches ChefZ.
     */
    private void ResolveVanillaBackMapping()
    {
        for (int stage = ChefZ_VanillaStage.RAW; stage <= ChefZ_VanillaStage.ROTTEN; stage++)
        {
            // Weg 1: kanonische ID.
            string canonical = ChefZ_VanillaStage.ChefZStateId(stage);
            ChefZ_Sym bySym = ChefZ_SymbolTable.Lookup(canonical);
            if (m_BySym.Contains(bySym))
            {
                m_SymByStage.Set(stage, bySym);
                continue;
            }

            // Weg 2: eindeutige Rueckwaertssuche.
            ChefZ_Sym found = ChefZ_SymbolTable.INVALID;
            int hits = 0;
            for (int i = 0; i < m_Order.Count(); i++)
            {
                ChefZ_StateDef def = m_BySym.Get(m_Order.Get(i));
                if (!def || def.projectedStage != stage)
                    continue;
                hits++;
                found = def.sym;
            }

            if (hits == 1)
                m_SymByStage.Set(stage, found);
        }
    }

    //==========================================================================
    // Abfragen - heisser Pfad
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    bool Exists(ChefZ_Sym state)
    {
        if (!GuardReady("Exists"))
            return false;
        return m_BySym.Contains(state);
    }

    /**
     * Der Datensatz oder null.
     *
     * null ist eine normale Antwort: ein Item ohne Zustand, ein Zustand aus
     * einem nicht geladenen Modul. Der Aufrufer im heissen Pfad nimmt
     * GetOrFallback und muss dann gar nichts pruefen.
     */
    ChefZ_StateDef GetDef(ChefZ_Sym state)
    {
        if (!GuardReady("GetDef"))
            return null;

        ChefZ_StateDef def;
        if (!m_BySym.Find(state, def))
            return null;
        return def;
    }

    //! 06 §4.2: "nie null, loggt einmal je Klasse". Hier: einmal je
    //! nachgefragtem Zustand - dieselbe Absicht, nur die genauere Schluessel-
    //! wahl. Eine Logflut nach einem Content-Rueckbau ist damit ausgeschlossen.
    ChefZ_StateDef GetOrFallback(ChefZ_Sym state)
    {
        ChefZ_StateDef def = GetDef(state);
        if (def)
            return def;

        if (ChefZ_SymbolTable.IsValid(state))
        {
            QuietOnce(ChefZ_LogLevel.WARN, "state.unknown." + ChefZ_SymbolTable.Ordinal(state), "Unbekannter Zustand \"" + ChefZ_SymbolTable.NameOrMark(state) + "\" - es gilt der neutrale Rueckfall (essbar, keine Projektion, Verderb " + "unveraendert). Haeufigste Ursache: das Modul mit diesem Zustand ist nicht " + "geladen. Diese Meldung erscheint je Zustand genau einmal.");
        }
        return m_Fallback;
    }

    //! Der neutrale Rueckfall selbst. Nur fuer Leser, die ohne Symbol
    //! auskommen muessen.
    ChefZ_StateDef GetFallbackDef()
    {
        return m_Fallback;
    }

    /**
     * FoodStageType als int, 0 = keine Projektion (06 §4.2).
     *
     * Bewusst int und kein FoodStageType: dieser Manager liegt in 3_Game, und
     * der Enum lebt in 4_World (00 §4). Die Zahlen sind dieselben - sie sind
     * netzsynchronisiert und damit stabil (01 V4).
     */
    int ProjectToVanillaStage(ChefZ_Sym state)
    {
        ChefZ_StateDef def = GetDef(state);
        if (!def)
            return ChefZ_VanillaStage.NONE;
        return def.projectedStage;
    }

    //! Umkehrung, Schritt 3 der Projektionsregel. INVALID, wenn die Stufe
    //! nicht eindeutig rueckabbildbar ist.
    ChefZ_Sym FromVanillaStage(int stage)
    {
        if (!GuardReady("FromVanillaStage"))
            return ChefZ_SymbolTable.INVALID;

        int sym;
        if (!m_SymByStage.Find(stage, sym))
            return ChefZ_SymbolTable.INVALID;
        return sym;
    }

    //! Die geprueften implies-Tags. outTags wird geleert, nie null.
    void GetImpliedTags(ChefZ_Sym state, out array<ChefZ_Sym> outTags)
    {
        if (!outTags)
            outTags = new array<ChefZ_Sym>();
        outTags.Clear();

        if (!GuardReady("GetImpliedTags"))
            return;

        array<ChefZ_Sym> tags;
        if (!m_ImpliedBySym.Find(state, tags))
            return;

        for (int i = 0; i < tags.Count(); i++)
            outTags.Insert(tags.Get(i));
    }

    //! Ein unbekannter Zustand gilt als essbar - siehe GetOrFallback.
    bool IsEdible(ChefZ_Sym state)
    {
        return GetOrFallback(state).edible;
    }

    //! Ein unbekannter Zustand gilt als NICHT terminal: eine Sperre, die
    //! niemand erklaeren kann, ist schlimmer als keine.
    bool IsTerminal(ChefZ_Sym state)
    {
        return GetOrFallback(state).terminal;
    }

    bool IsPreserved(ChefZ_Sym state)
    {
        return GetOrFallback(state).preserved;
    }

    float GetSpoilageMultiplier(ChefZ_Sym state)
    {
        return GetOrFallback(state).spoilageMultiplier;
    }

    //==========================================================================
    // Identitaeten (03)
    //==========================================================================

    /**
     * Persistenzschluessel eines Zustands (03 E2). 0 = unbekannt.
     *
     * Er kommt aus der eigenen Tabelle und nicht aus der ChefZ_IdentityMap:
     * die fuehrt nur Rang 1, und ein Item darf seinen Zustand auch dann
     * speichern, wenn dessen Herkunft ein Konfigurationsfehler war. Siehe
     * Kopfkommentar.
     */
    int GetPersistHash(ChefZ_Sym state)
    {
        if (!GuardReady("GetPersistHash"))
            return 0;
        if (!m_BySym.Contains(state))
            return 0;

        // Zwischenvariable und kein Aufruf auf dem Rueckgabewert: Enforce
        // sichert Methodenaufrufe auf temporaeren Strings nicht zu.
        string name = ChefZ_SymbolTable.Name(state);
        return name.Hash();
    }

    ChefZ_Sym FromPersistHash(int hash)
    {
        if (!GuardReady("FromPersistHash"))
            return ChefZ_SymbolTable.INVALID;
        if (hash == 0)
            return ChefZ_SymbolTable.INVALID;

        int sym;
        if (!m_SymByHash.Find(hash, sym))
            return ChefZ_SymbolTable.INVALID;
        return sym;
    }

    //! 0 = kein Ordinal. Ein Zustand ohne Ordinal ist clientseitig nicht
    //! darstellbar - serverseitig aber voll benutzbar (03 §7).
    int GetSyncOrdinal(ChefZ_Sym state)
    {
        if (!GuardIdentities("GetSyncOrdinal"))
            return 0;
        return m_Identities.ToSyncOrdinal(state);
    }

    ChefZ_Sym FromSyncOrdinal(int ordinal)
    {
        if (ordinal <= 0)
            return ChefZ_SymbolTable.INVALID;
        if (!GuardIdentities("FromSyncOrdinal"))
            return ChefZ_SymbolTable.INVALID;
        return m_Identities.FromSyncOrdinal(ordinal);
    }

    int GetMaxOrdinal()
    {
        if (!m_Identities)
            return 0;
        return m_Identities.GetMaxOrdinal();
    }

    //==========================================================================
    // Diagnose und Zaehler
    //==========================================================================

    int GetStateCount()
    {
        return m_Order.Count();
    }

    int GetRejectedCount()
    {
        return m_RejectedCount;
    }

    //! Symbole in stabiler Reihenfolge (nach ID sortiert).
    void GetAll(out array<ChefZ_Sym> outStates)
    {
        if (!outStates)
            outStates = new array<ChefZ_Sym>();
        outStates.Clear();
        for (int i = 0; i < m_Order.Count(); i++)
            outStates.Insert(m_Order.Get(i));
    }

    void DumpStates(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Zustaende: " + GetStateCount().ToString() + "  ordinale=" + GetMaxOrdinal().ToString() + "  bereit=" + m_Ready.ToString());

        for (int i = 0; i < m_Order.Count(); i++)
        {
            ChefZ_Sym sym = m_Order.Get(i);
            ChefZ_StateDef def = m_BySym.Get(sym);
            if (!def)
                continue;

            string line = "    " + def.ToLine() + "  ord=" + GetSyncOrdinal(sym).ToString() + "  hash=" + GetPersistHash(sym).ToString();

            array<ChefZ_Sym> tags;
            GetImpliedTags(sym, tags);
            if (tags.Count() > 0)
                line = line + "  implies=[" + ChefZ_TextList.JoinSymbols(tags, ",") + "]";

            outLines.Insert(line);
        }
    }

    private void LogIfDebug()
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.STATE, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        DumpStates(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.STATE, lines);
    }

    //==========================================================================
    // Innereien
    //==========================================================================

    private ChefZ_CategoryManager Cats()
    {
        if (m_CategoriesForTest)
            return m_CategoriesForTest;
        return ChefZ_CategoryManager.Get();
    }

    private void ResetState()
    {
        m_BySym.Clear();
        m_ImpliedBySym.Clear();
        m_SymByHash.Clear();
        m_SymByStage.Clear();
        m_Order.Clear();

        m_Identities     = null;
        m_Ready          = false;
        m_NotReadyLogged = false;
        m_RejectedCount  = 0;
    }

    //! Leert den Bestand. Vorgesehener Aufrufer ist der SAFE_MODE (02 §8).
    void Reset()
    {
        ResetState();
    }

    //! 06 §7 sinngemaess: Abfrage vor Build liefert die neutrale Antwort und
    //! meldet EINMAL. Kein Nullzugriff, kein Absturz.
    private bool GuardReady(string what)
    {
        if (m_Ready)
            return true;
        if (m_NotReadyLogged)
            return false;
        m_NotReadyLogged = true;

        if (m_QuietForTest)
            return false;

        ChefZ_Log.Error(ChefZ_LogChannel.STATE, "ChefZ_StateManager." + what + "() wurde vor Build() aufgerufen - die Antwort ist " + "\"kein Zustand\". Zustands-Selektoren matchen solange nicht; Vanilla-Kochen ist " + "davon unberuehrt. Diese Meldung erscheint genau einmal.");
        return false;
    }

    private bool GuardIdentities(string what)
    {
        if (!GuardReady(what))
            return false;
        if (m_Identities)
            return true;

        QuietOnce(ChefZ_LogLevel.WARN, "state.noidentities", "ChefZ_StateManager." + what + "(): es gibt keine Ordinaltabelle. Zustaende sind " + "serverseitig voll benutzbar, koennen aber nicht zum Client synchronisiert und " + "dort nicht angezeigt werden (03 §4). Ursache ist immer ein Reihenfolgefehler im " + "Boot oder eine Zustandsregistry ohne Rang-1-Eintraege.");
        return false;
    }

    private void QuietOnce(int level, string key, string message)
    {
        if (m_QuietForTest)
            return;
        ChefZ_Log.Once(level, ChefZ_LogChannel.STATE, key, message);
    }

    private void Report(ChefZ_LoadReport report, bool isError, notnull ChefZ_StateDef def, string msg)
    {
        if (report)
        {
            if (isError)
                report.AddError(def.sourceRef, def.id, msg);
            else
                report.AddWarn(def.sourceRef, def.id, msg);
            return;
        }

        if (m_QuietForTest)
            return;

        string line = def.sourceRef + " / " + def.id + ": " + msg;
        if (isError)
            ChefZ_Log.Error(ChefZ_LogChannel.STATE, line);
        else
            ChefZ_Log.Warn(ChefZ_LogChannel.STATE, line);
    }

    //! Nur fuer den Selbsttest: unterdrueckt die Meldungen dieser Klasse.
    //! Noetig, weil der Test die Fehlerfaelle absichtlich durchspielt und
    //! ChefZ_Log.GetErrorCount() die Safe-Mode-Schwelle speist (18 §4).
    //! Dieselbe Loesung wie im ChefZ_CategoryManager.
    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }

    //! Kategoriebaum des Tests statt des Singletons.
    void SetCategoryManagerForTest(ChefZ_CategoryManager mgr)
    {
        m_CategoriesForTest = mgr;
    }
}
