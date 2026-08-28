//==============================================================================
// ChefZ_ItemStateComponent - Zustandsblock, Persistenz, Sync und Projektion
//
// Entwurf: 06 §4.3 (Schnittstelle woertlich), 06 §3 (Projektionsregel),
// 06 §5 (Datenfluss), 06 §6 (Zustandstabelle), 06 §7 (Fehlerverhalten),
// 06 E3 (SetFoodStageType statt ChangeFoodStage), 06 E5 (Vanilla gewinnt bei
// BURNED und ROTTEN), 03 §4 (Sync-Obergrenzen), 03 E2 (Persistenz ueber
// Name.Hash()), V-B §2 (OF-02: eigener Block mit MAGIC und VERSION).
//
// ---------------------------------------------------------------------------
// Warum die Daten HIER liegen und nicht auf den Traegerklassen
// ---------------------------------------------------------------------------
// 06 §4.3 nennt dieselben sieben Variablen zweimal: einmal auf
// ChefZ_Edible_Base, einmal auf ChefZ_Item_Base ("dieselben Variablen,
// dieselbe API"). Enforce kennt keine Mehrfachvererbung und keine
// Schnittstellen - eine woertliche Umsetzung hiesse also, Felder UND Logik
// doppelt zu fuehren. Zwei Kopien einer Persistenzroutine sind zwei Kopien,
// die auseinanderlaufen, und ein auseinandergelaufener Lesestrom ist ein
// zerschossener Spielstand.
//
// Deshalb traegt jede Traegerklasse EIN Feld:
//
//     protected ref ChefZ_ItemStateComponent m_ChefZ_State;
//
// und registriert ihre Netzsync-Variablen ueber den Pfad "m_ChefZ_State.<feld>".
// Das ist kein Kunstgriff, sondern Vanillas eigenes Muster:
//
//     EntityAI.c:219-224     RegisterNetSyncVariableBool("m_EM.m_IsSwichedOn")
//     Edible_Base.c:31       RegisterNetSyncVariableInt("m_FoodStage.m_FoodStageType", ...)
//     ClaymoreMine.c:18      RegisterNetSyncVariableInt("m_RAIB.m_PairDeviceNetIdLow")
//
// In allen drei Faellen ist das Ziel ein ref-Feld auf eine gewoehnliche
// Skriptklasse mit protected Membern - genau wie hier. Die Feldnamen im Block
// behalten die Schreibweise aus 06 §4.3, damit der Entwurf greifbar bleibt.
//
// ---------------------------------------------------------------------------
// Der Block wird IMMER geschrieben (06 §6, V-B §2 Folge 1)
// ---------------------------------------------------------------------------
// Auch wenn nichts gesetzt ist. Feste Breite: MAGIC, VERSION, vier 4-Byte-
// Felder und die Behaelterrueckgabe. Schriebe man ihn bedingt, verschoebe eine
// Content-Aenderung zwischen zwei Serverstarts den Lesestrom JEDES
// gespeicherten Items dieser Klasse - und zwischen M2 und M3 aendert sich
// Content laufend.
//
// EHRLICH BENANNT (V-B §2 Folge 3): MAGIC richtet einen verschobenen Strom
// NICHT wieder aus. Es verhindert nur, dass ChefZ fremde Bytes als eigene
// deutet. Die Ausrichtung sichert die feste Breite, nicht MAGIC.
//
// ---------------------------------------------------------------------------
// Server. Ausschliesslich, fuer alles Autoritative.
// ---------------------------------------------------------------------------
// SetState, SetQuality, SetFreshness, SetPortions und die Projektion pruefen
// g_Game.IsServer() selbst. Es gibt keinen RPC-Ersatzweg (06 §7).
//
// KEIN CONTENT: kein Klassenname, kein Zustandsname, keine Zutat.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_ItemStateComponent : Managed
{
    //! "CHZ1". Selbstbeschreibender Marker vor dem eigenen Block - Vanillas
    //! eigenes Muster (ItemBase.c:3221, "Keep track of if we should actually
    //! read this in or not"), nur mit mehr Bits.
    static const int MAGIC   = 0x43485A31;

    //! Bei JEDER Feldaenderung erhoehen (V-B §2 Folge 4).
    static const int VERSION = 1;

    //--- Neutrale Werte -------------------------------------------------------
    static const int   NO_ORDINAL       = 0;      // "aus der Klasse ableiten"
    static const int   NO_HASH          = 0;
    static const float FRESHNESS_FULL   = 1.0;
    static const int   NO_PORTIONS      = 0;

    //--- Netzsync (03 §4). Registriert von der Traegerklasse ueber den Pfad
    //    "m_ChefZ_State.<feld>" - siehe RegisterNetSync().
    //    protected und nicht private: Vanilla registriert seine
    //    Sync-Variablen auf FoodStage und ComponentEnergyManager genauso.
    protected int   m_ChefZ_StateOrdinal;      // 0..63, 0 = aus der Klasse
    protected int   m_ChefZ_QualityOrdinal;    // 0..15
    protected float m_ChefZ_Freshness;         // 0..1, Praezision 2
    protected int   m_ChefZ_Portions;          // 0..31

    //--- NUR Persistenz (06 §6) ----------------------------------------------
    protected int    m_ChefZ_StatePersist;     // Name.Hash(), 03 E2
    protected int    m_ChefZ_QualityPersist;   // Name.Hash()
    protected string m_ChefZ_ReturnContainer;  // 16

    /**
     * Laeuft gerade eine reine Buchhaltungsprojektion? (06 E3)
     *
     * Weder persistiert noch gesynct - er lebt genau die Dauer eines
     * SetFoodStageType-Aufrufs. Die Traegerklasse fragt ihn in ihrem
     * OnFoodStageChange-Override ab; steht er, unterbleibt Vanillas
     * Agentenbereinigung. Siehe SetState().
     */
    protected bool m_Projecting;

    //--------------------------------------------------------------------------

    void ChefZ_ItemStateComponent()
    {
        SetDefaults();
    }

    //! Die neutralen Werte. Auch der Ladepfad benutzt sie, damit "kein Block"
    //! und "frisch erzeugt" bitgenau dasselbe bedeuten.
    void SetDefaults()
    {
        m_ChefZ_StateOrdinal    = NO_ORDINAL;
        m_ChefZ_QualityOrdinal  = NO_ORDINAL;
        m_ChefZ_Freshness       = FRESHNESS_FULL;
        m_ChefZ_Portions        = NO_PORTIONS;
        m_ChefZ_StatePersist    = NO_HASH;
        m_ChefZ_QualityPersist  = NO_HASH;
        m_ChefZ_ReturnContainer = "";
        m_Projecting            = false;
    }

    //==========================================================================
    // Netzsync-Registrierung (03 §4, woertlich)
    //==========================================================================

    /**
     * Die vier Registrierungen an EINER Stelle.
     *
     * Sie aus beiden Traegerklassen aufzurufen ist billiger als sie in beiden
     * zu wiederholen: die Obergrenzen sind die Bitbreite auf der Leitung, und
     * zwei Kopien, von denen eine 63 und die andere 31 sagt, ergaeben einen
     * Client, der einen anderen Zustand sieht als der Server meint.
     *
     * Aufzurufen im Konstruktor der Traegerklasse, NACHDEM m_ChefZ_State
     * erzeugt wurde - der Pfad wird beim Registrieren aufgeloest.
     */
    static void RegisterNetSync(notnull EntityAI owner)
    {
        owner.RegisterNetSyncVariableInt("m_ChefZ_State.m_ChefZ_StateOrdinal", 0, ChefZ_SyncLimits.STATE_ORDINAL_MAX);
        owner.RegisterNetSyncVariableInt("m_ChefZ_State.m_ChefZ_QualityOrdinal", 0, ChefZ_SyncLimits.QUALITY_ORDINAL_MAX);
        owner.RegisterNetSyncVariableInt("m_ChefZ_State.m_ChefZ_Portions", 0, ChefZ_SyncLimits.PORTIONS_MAX);
        owner.RegisterNetSyncVariableFloat("m_ChefZ_State.m_ChefZ_Freshness", 0.0, 1.0, 2);
    }

    //==========================================================================
    // Zugriff auf den Block eines Items
    //==========================================================================

    /**
     * Der Zustandsblock eines Items, oder null.
     *
     * null ist die haeufigste Antwort und KEIN Fehler: jedes Vanilla-Item
     * liefert sie. Zwei Casts, weil Enforce keine gemeinsame Schnittstelle
     * ueber zwei Vererbungsaeste kennt (OF-01: es gibt bewusst kein
     * "modded class Edible_Base", das den Ast zusammenfuehren wuerde).
     */
    static ChefZ_ItemStateComponent Of(ItemBase item)
    {
        if (!item)
            return null;

        ChefZ_Edible_Base edible = ChefZ_Edible_Base.Cast(item);
        if (edible)
            return edible.ChefZ_StateBlock();

        ChefZ_Item_Base plain = ChefZ_Item_Base.Cast(item);
        if (plain)
            return plain.ChefZ_StateBlock();

        return null;
    }

    //! true, wenn das Item ueberhaupt einen ChefZ-Zustand tragen KANN
    //! (06 §4.3, ChefZ_IsManaged). Ein Vanilla-Item kann es nie - das ist
    //! Absicht und der Preis fuer null Kollisionsflaeche (V-B §1).
    static bool IsManaged(ItemBase item)
    {
        return Of(item) != null;
    }

    //==========================================================================
    // Rohzugriff auf die Felder
    //==========================================================================

    int    GetStateOrdinal()     { return m_ChefZ_StateOrdinal; }
    int    GetStatePersist()     { return m_ChefZ_StatePersist; }
    int    GetQualityOrdinal()   { return m_ChefZ_QualityOrdinal; }
    int    GetQualityPersist()   { return m_ChefZ_QualityPersist; }
    float  GetFreshness01()      { return m_ChefZ_Freshness; }
    int    GetPortions()         { return m_ChefZ_Portions; }
    string GetReturnContainer()  { return m_ChefZ_ReturnContainer; }

    bool   IsProjecting()        { return m_Projecting; }
    void   BeginProjection()     { m_Projecting = true; }
    void   EndProjection()       { m_Projecting = false; }

    //! true, wenn ueberhaupt ein Zustand auf dem Item liegt (06 §3, Schritt 1).
    bool HasState()
    {
        return m_ChefZ_StatePersist != NO_HASH || m_ChefZ_StateOrdinal != NO_ORDINAL;
    }

    bool HasQuality()
    {
        return m_ChefZ_QualityPersist != NO_HASH || m_ChefZ_QualityOrdinal != NO_ORDINAL;
    }

    /**
     * Der getragene Zustand als Symbol, oder INVALID.
     *
     * Reihenfolge mit Absicht: der Persistenz-Hash zuerst, der Sync-Ordinal
     * danach. Auf dem Server sind beide gesetzt und der Hash ist die
     * Wahrheit; auf dem Client kommt nur der Ordinal an, weil der Hash nicht
     * synchronisiert wird (06 §6). Eine Abfrage, die beide Seiten bedient,
     * muss deshalb genau so herum fragen.
     */
    ChefZ_Sym ResolveStateSym()
    {
        ChefZ_StateManager mgr = ChefZ_StateManager.Get();

        if (m_ChefZ_StatePersist != NO_HASH)
        {
            ChefZ_Sym byHash = mgr.FromPersistHash(m_ChefZ_StatePersist);
            if (ChefZ_SymbolTable.IsValid(byHash))
                return byHash;
        }

        if (m_ChefZ_StateOrdinal != NO_ORDINAL)
            return mgr.FromSyncOrdinal(m_ChefZ_StateOrdinal);

        return ChefZ_SymbolTable.INVALID;
    }

    /**
     * Dieselbe Frage fuer die Qualitaetsstufe.
     *
     * Die Semantik der Stufen gehoert S12 (12); was hier steht, ist reine
     * Identitaetsverwaltung: Hash rein, Symbol raus. Deshalb greift sie direkt
     * auf die ChefZ_IdentityMap des Config Managers zu und nicht auf einen
     * ChefZ_QualityManager, den es noch nicht gibt.
     */
    ChefZ_Sym ResolveQualitySym()
    {
        ChefZ_IdentityMap ids = QualityIdentities();
        if (!ids)
            return ChefZ_SymbolTable.INVALID;

        if (m_ChefZ_QualityPersist != NO_HASH)
        {
            ChefZ_Sym byHash = ids.FromPersistHash(m_ChefZ_QualityPersist);
            if (ChefZ_SymbolTable.IsValid(byHash))
                return byHash;
        }

        if (m_ChefZ_QualityOrdinal != NO_ORDINAL)
            return ids.FromSyncOrdinal(m_ChefZ_QualityOrdinal);

        return ChefZ_SymbolTable.INVALID;
    }

    private static ChefZ_IdentityMap QualityIdentities()
    {
        ChefZ_ConfigManager cfg = ChefZ_ConfigManager.Get();
        if (!cfg)
            return null;
        return cfg.QualityIdentities();
    }

    //==========================================================================
    // Setzen - alles serverseitig
    //==========================================================================

    /**
     * Zustand setzen (06 §5, "ERZEUGUNG / UEBERGANG", Zweig "nein").
     *
     *   1. StateManager.GetDef(sym)
     *   2. m_ChefZ_StatePersist = GetPersistHash(sym)
     *      m_ChefZ_StateOrdinal = GetSyncOrdinal(sym)
     *   3. Projektion auf die Vanilla-FoodStage
     *   4. Visuals auffrischen
     *   5. SetSynchDirty()
     *   6. Event (S18)
     *
     * @param applyVanillaTransition  true = die volle Vanilla-Kette
     *        (ChangeFoodStage) laeuft, inklusive Agentenbereinigung. Das ist
     *        die richtige Wahl, wenn die Zustandsaenderung fachlich AUCH ein
     *        Garvorgang war. false = reine Buchhaltung (06 E3).
     *
     * @return false, wenn nichts gesetzt wurde. Ein "war schon so" gilt als
     *         Erfolg: der gewuenschte Zustand liegt an.
     */
    static bool SetState(ItemBase item, ChefZ_Sym state, bool applyVanillaTransition = false)
    {
        ChefZ_ItemStateComponent comp = Of(item);
        if (!comp)
            return false;

        if (!IsServer())
        {
            // 06 §7: clientseitiger Aufruf ist ein No-op mit ERROR. Es gibt
            // keinen RPC-Ersatzweg - der Zustand ist eine Serverentscheidung.
            Note(ChefZ_LogLevel.ERR, "state.clientset", "ChefZ_SetState wurde clientseitig gerufen und ignoriert. Der Zustand ist " + "autoritativ und wird ausschliesslich serverseitig gesetzt (06 §7).");
            return false;
        }

        if (!ChefZ_SymbolTable.IsValid(state))
        {
            // Kein Zustand ist kein Zustandswechsel. Still, weil der Aufrufer
            // regelmaessig aus Daten kommt, die schlicht nichts sagen
            // ("setState": "") - und dort ist Schweigen die richtige Antwort.
            return false;
        }

        ChefZ_StateManager mgr = ChefZ_StateManager.Get();
        ChefZ_Sym current = GetState(item);

        // 06 §7: Uebergang von X nach X ist ein No-op - kein Event, kein Sync,
        // kein Log. Gemessen wird am ERMITTELTEN Zustand (Projektionsregel),
        // nicht an der Variablen: liegt der Zustand bereits ueber die Klasse
        // an, waere das Setzen der Variablen eine Doppelung, die spaeter
        // niemand mehr von einem echten Uebergang unterscheiden kann.
        if (current == state)
            return true;

        if (ChefZ_SymbolTable.IsValid(current) && mgr.IsTerminal(current))
        {
            Note(ChefZ_LogLevel.WARN, "state.terminal." + ChefZ_SymbolTable.Ordinal(current), "Zustandswechsel abgelehnt, reason = \"terminal state\": \"" + ChefZ_SymbolTable.NameOrMark(current) + "\" ist als terminal deklariert. " + "Ein terminaler Zustand ist das Ende einer Kette, kein Zwischenschritt.");
            return false;
        }

        ChefZ_StateDef def = mgr.GetDef(state);
        if (!def)
        {
            Note(ChefZ_LogLevel.ERR, "state.setunknown." + ChefZ_SymbolTable.Ordinal(state), "ChefZ_SetState auf den unbekannten Zustand \"" + ChefZ_SymbolTable.NameOrMark(state) + "\". Es wird nichts gesetzt - ein Item " + "mit einem Zustand, den kein Rezept und keine Anzeige kennt, waere schlimmer " + "als eines ohne Zustand.");
            return false;
        }

        // Schritt 2: beide Identitaeten gemeinsam (03 §6).
        comp.m_ChefZ_StatePersist = mgr.GetPersistHash(state);
        comp.m_ChefZ_StateOrdinal = mgr.GetSyncOrdinal(state);

        // Schritt 3 und 4.
        comp.ProjectOnto(item, def, applyVanillaTransition);

        // Schritt 5.
        item.SetSynchDirty();

        // Schritt 6 - EVENT (06 §5, 17 §4). Seit S13.
        //
        // Diese eine Stelle ist das Rueckgrat fuer Konservierungsquests:
        // Raeuchern, Trocknen, Salzen und Kochen laufen ALLE hier durch, und
        // deshalb feuern sie alle dasselbe Ereignis. Ein Quest "konserviere 10
        // Lebensmittel" braucht einen Abonnenten, nicht fuenf (17 §4).
        //
        // Nach dem Sync und nicht davor: ein Abonnent, der das Item ueber die
        // Netz-ID nachschlaegt, soll den NEUEN Zustand vorfinden.
        RaiseStateChanged(item, current, state, def);

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.STATE, ChefZ_LogLevel.DEBUG))
            ChefZ_Log.Debug(ChefZ_LogChannel.STATE, item.GetType() + ": Zustand " + ChefZ_SymbolTable.NameOrMark(current) + " -> " + ChefZ_SymbolTable.NameOrMark(state) + "  ord=" + comp.m_ChefZ_StateOrdinal.ToString() + "  vanillaTransition=" + applyVanillaTransition.ToString());

        return true;
    }

    /**
     * Die Zustandsereignisse (17 §4). Seit S13.
     *
     * Drei Ereignisse aus EINER Nutzlast, weil es EIN Vorgang ist:
     *
     *   ChefZ_OnFoodStateChanged  immer
     *   ChefZ_OnFoodPreserved     zusaetzlich bei def.preserved (XP-tauglich)
     *   ChefZ_OnFoodSpoiled       zusaetzlich beim Uebergang nach ROTTEN
     *
     * Erst die Wachen, dann die Nutzlast (17 E2): auf einem Server ohne
     * Comp-Module kostet diese Methode drei Map-Zugriffe je Zustandswechsel -
     * und ein Zustandswechsel ist ein seltenes Ereignis, kein Tick.
     *
     * Der Fortschritt wird NUR fuer die Konservierung gemeldet, und zwar erst
     * NACH dem vollzogenen Wechsel (17 E7): Zustand geschrieben, projiziert,
     * synchronisiert. Fuer den Wechsel an sich gibt es bewusst keine Meldung -
     * ein Item, das von RAW nach RAW_CHOPPED und zurueck geht, waere sonst
     * eine XP-Schleife.
     */
    private static void RaiseStateChanged(notnull ItemBase item, ChefZ_Sym before, ChefZ_Sym after, notnull ChefZ_StateDef def)
    {
        ChefZ_EventBus bus = ChefZ_EventBus.Get();

        bool wantChanged   = bus.HasSubscribers(ChefZ_EventNames.FOOD_STATE_CHANGED);
        bool isPreserved   = def.preserved;
        bool isSpoiled     = def.projectedStage == ChefZ_VanillaStage.ROTTEN;
        bool wantPreserved = isPreserved && bus.HasSubscribers(ChefZ_EventNames.FOOD_PRESERVED);
        bool wantSpoiled   = isSpoiled && bus.HasSubscribers(ChefZ_EventNames.FOOD_SPOILED);
        bool wantProgress  = isPreserved && ChefZ_ProgressRegistry.HasSinks();

        if (!wantChanged && !wantPreserved && !wantSpoiled && !wantProgress)
            return;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.FOOD_STATE_CHANGED);
        args.subjectClass = ChefZ_SymbolTable.Intern(item.GetType());
        args.stateBefore  = before;
        args.stateAfter   = after;

        // Bewusst OHNE qualityTier: die Stufe steht am Item, nicht am Wechsel,
        // und ein Abonnent, der sie braucht, loest die Netz-ID auf und liest
        // sie dort. Sie hier mitzugeben hiesse, dieselbe Zahl an zwei Stellen
        // zu fuehren - und die zweite laege in einer Nutzlast, die nach dem
        // Rueckruf ungueltig ist (17 §8).

        int low;
        int high;
        if (NetIdOf(item, low, high))
            args.SetSubjectNetId(low, high);

        if (wantChanged)
            bus.RaiseKeep(args);

        if (wantPreserved)
        {
            args.eventId = ChefZ_EventNames.FOOD_PRESERVED;
            bus.RaiseKeep(args);
        }

        if (wantSpoiled)
        {
            args.eventId = ChefZ_EventNames.FOOD_SPOILED;
            bus.RaiseKeep(args);
        }

        if (wantProgress)
            ChefZ_ProgressRegistry.Report(ChefZ_ProgressKind.PRESERVE, args);

        bus.Release(args);
    }

    //! Netz-ID eines Items, in zwei Haelften (17 E4). false, wenn das Item
    //! keine hat - dann bleibt das Ereignis ohne Weltbezug, was ehrlicher ist
    //! als eine erfundene Zahl.
    private static bool NetIdOf(notnull ItemBase item, out int low, out int high)
    {
        // Ueber lokale Zwischenvariablen und nicht direkt in die
        // out-Parameter: einen out-Parameter als out-Argument weiterzugeben
        // ist in Enforce nirgends zugesichert (dieselbe Vorsicht wie in
        // ChefZ_CookingDeviceAdapter.VesselId).
        int lo = 0;
        int hi = 0;
        item.GetNetworkID(lo, hi);
        low  = lo;
        high = hi;
        return (lo | hi) != 0;
    }

    /**
     * Schritt 3 der Zustandssetzung: Projektion auf die Vanilla-FoodStage.
     *
     * 06 E3, und das ist die begruendungsbeduerftigste Zeile des ganzen
     * Teilsystems - deshalb ausfuehrlich:
     *
     * Der Entwurf schreibt "SetFoodStageType() statt ChangeFoodStage()", weil
     * ChangeFoodStage die Agentenbereinigung ausloest. In 1.29 ist das so
     * nicht mehr trennbar:
     *
     *     FoodStage.c:506  void ChangeFoodStage(t) { SetFoodStageType(t); }
     *     FoodStage.c:204  void SetFoodStageType(t) { ...; OnFoodStageChange(alt,t);
     *                                                 GetFoodItem().Synchronize(); }
     *     FoodStage.c:511  OnFoodStageChange -> m_FoodItem.OnFoodStageChange(...)
     *     Edible_Base.c:630 OnFoodStageChange -> HandleFoodStageChangeAgents(...)
     *
     * ChangeFoodStage IST SetFoodStageType. Die Absicht des Entwurfs bleibt
     * trotzdem erfuellbar, und zwar praeziser als er selbst annahm: der Punkt,
     * an dem Vanilla die Agenten entfernt, ist ein VIRTUELLER Aufruf auf dem
     * Item - und das Item gehoert uns. Deshalb:
     *
     *     - der Merker m_Projecting wird gesetzt,
     *     - dann SetFoodStageType (die Buchhaltung, wie im Entwurf benannt),
     *     - ChefZ_Edible_Base.OnFoodStageChange sieht den Merker und laesst
     *       Vanillas HandleFoodStageChangeAgents aus, frischt aber die Visuals
     *       auf.
     *
     * Ergebnis: eine reine Verwaltungsprojektion loescht keine Agenten, ein
     * echter Garvorgang (applyVanillaTransition) tut es weiterhin. Genau das
     * war die Aussage von 06 E3.
     *
     * Fuer ChefZ_Item_Base gibt es keine FoodStage und damit keine Projektion.
     * Das ist kein Fehler - Mehl und Salz haben keine Garstufe.
     */
    private void ProjectOnto(notnull ItemBase item, notnull ChefZ_StateDef def, bool applyVanillaTransition)
    {
        if (!def.HasProjection())
            return;

        Edible_Base edible = Edible_Base.Cast(item);
        FoodStage stage;
        if (edible)
            stage = edible.GetFoodStage();

        if (!stage)
        {
            // 06 §7: WARN einmal je Klasse, weil dann Visuals fehlen. Die
            // ChefZ-Variable ist trotzdem gesetzt - kein Nullzugriff.
            Note(ChefZ_LogLevel.WARN, "state.nofoodstage." + item.GetType(), "\"" + item.GetType() + "\" traegt einen Zustand mit Projektion auf \"" + ChefZ_VanillaStage.Name(def.projectedStage) + "\", hat aber keine FoodStage. " + "Der ChefZ-Zustand wirkt, die Optik nicht. Abhilfe: der Klasse in CfgVehicles " + "einen Food-FoodStages-Block geben (01 V4) oder die Projektion entfernen.");
            return;
        }

        int currentStage = stage.GetFoodStageType();
        if (currentStage == def.projectedStage)
            return;

        // Typisierte Zwischenvariable: die Zahl kommt aus ChefZ_VanillaStage
        // (1_Core, bewusst kein Enum), die Vanilla-Schnittstelle will einen
        // FoodStageType. Die Werte sind dieselben - ChefZ_VanillaStage ist
        // woertlich aus FoodStage.c:1 nachgeschrieben.
        FoodStageType target = def.projectedStage;

        if (applyVanillaTransition)
        {
            // Die volle Vanilla-Kette: Agenten werden entfernt, Visuals
            // aufgefrischt, Synchronize gerufen. Der Aufrufer hat ausdruecklich
            // gesagt, dass hier gegart wurde.
            edible.ChangeFoodStage(target);
            return;
        }

        BeginProjection();
        stage.SetFoodStageType(target);
        EndProjection();
    }

    /**
     * 06 E5: Vanilla gewinnt bei BURNED und ROTTEN.
     *
     * Zwei Systeme, die beide sagen duerfen "das ist verdorben", driften
     * garantiert auseinander. Sobald Vanilla einen der beiden Fehlzustaende
     * setzt, wird das ChefZ-Overlay geloescht - das Item faellt damit auf die
     * Projektionsregel zurueck und liest seinen Zustand aus der Vanilla-Stufe.
     * Verdorbenes bleibt verdorben.
     *
     * Aufzurufen aus dem OnFoodStageChange-Override der Traegerklasse, und nur
     * dort, wo KEINE ChefZ-Projektion laeuft.
     */
    static void OnVanillaStageChanged(ItemBase item, int stageNew)
    {
        if (stageNew != ChefZ_VanillaStage.BURNED && stageNew != ChefZ_VanillaStage.ROTTEN)
            return;

        ChefZ_ItemStateComponent comp = Of(item);
        if (!comp || !comp.HasState())
            return;

        comp.m_ChefZ_StateOrdinal = NO_ORDINAL;
        comp.m_ChefZ_StatePersist = NO_HASH;

        if (item)
            item.SetSynchDirty();

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.STATE, ChefZ_LogLevel.DEBUG))
            ChefZ_Log.Debug(ChefZ_LogChannel.STATE, item.GetType() + ": Vanilla hat " + ChefZ_VanillaStage.Name(stageNew) + " gesetzt - das ChefZ-Overlay wurde geloescht (06 E5).");
    }

    //--------------------------------------------------------------------------

    //! Qualitaetsstufe setzen. Identitaetsverwaltung; die Bewertung ist S12.
    static bool SetQuality(ItemBase item, ChefZ_Sym tier)
    {
        ChefZ_ItemStateComponent comp = Of(item);
        if (!comp || !IsServer())
            return false;

        ChefZ_IdentityMap ids = QualityIdentities();
        if (!ids)
        {
            Note(ChefZ_LogLevel.WARN, "quality.noidentities", "Es gibt keine Ordinaltabelle fuer Qualitaetsstufen - die Stufe wird nicht " + "gesetzt. Ursache ist immer eine QualityTier-Registry ohne Rang-1-Eintraege " + "(03 §4).");
            return false;
        }

        int hash = ids.ToPersistHash(tier);
        if (hash == 0 && ChefZ_SymbolTable.IsValid(tier))
        {
            Note(ChefZ_LogLevel.WARN, "quality.unknown." + ChefZ_SymbolTable.Ordinal(tier), "Unbekannte Qualitaetsstufe \"" + ChefZ_SymbolTable.NameOrMark(tier) + "\" - sie wird nicht gesetzt.");
            return false;
        }

        comp.m_ChefZ_QualityPersist = hash;
        comp.m_ChefZ_QualityOrdinal = ids.ToSyncOrdinal(tier);
        item.SetSynchDirty();
        return true;
    }

    //! Frische 0..1. Wird geklemmt statt abgewiesen: ein Wert ausserhalb ist
    //! ein Rechenfehler des Aufrufers, und ein geklemmter Wert ist harmloser
    //! als ein stiller Abbruch (02 §8).
    static bool SetFreshness01(ItemBase item, float value)
    {
        ChefZ_ItemStateComponent comp = Of(item);
        if (!comp || !IsServer())
            return false;

        comp.m_ChefZ_Freshness = Math.Clamp(value, 0.0, 1.0);
        item.SetSynchDirty();
        return true;
    }

    /**
     * Frische setzen, aber NUR synchronisieren, wenn der Client den
     * Unterschied ueberhaupt sehen kann (14 §6, Zeile 6). Seit S11.
     *
     * 14 §6 begruendet es woertlich: "Die Drosselung des Sync ist Absicht:
     * Freshness01 wird mit Praezision 2 synchronisiert; ohne
     * Schwellwertpruefung loeste JEDER Decay-Tick jedes Lebensmittels einen
     * Sync-Write aus."
     *
     * Die Schwelle ist deshalb nicht frei gewaehlt, sondern exakt die
     * Aufloesung der Leitung: RegisterNetSync() meldet das Feld mit Praezision
     * 2 an (03 §4, siehe RegisterNetSync weiter oben). Zwei Werte, die auf
     * zwei Nachkommastellen gleich sind, ergeben auf dem Client BITGENAU
     * dasselbe - ein Sync dafuer ist reine Last.
     *
     * Der WERT wird trotzdem in voller Genauigkeit gefuehrt und gespeichert:
     * die Frische faellt in winzigen Schritten, und wer sie auf zwei Stellen
     * rundete, saehe sie bei kleinen Faktoren nie fallen.
     *
     * @return true, wenn synchronisiert wurde. Nur fuer Diagnose und Tests.
     */
    static bool SetFreshness01Throttled(ItemBase item, float value)
    {
        ChefZ_ItemStateComponent comp = Of(item);
        if (!comp || !IsServer())
            return false;

        float next = Math.Clamp(value, 0.0, 1.0);
        float prev = comp.m_ChefZ_Freshness;

        comp.m_ChefZ_Freshness = next;

        if (SyncBucket(prev) == SyncBucket(next))
            return false;

        item.SetSynchDirty();
        return true;
    }

    //! Der Wert, wie ihn die Leitung traegt: zwei Nachkommastellen.
    //! Math.Round liefert in Enforce einen float (1_Core/DayZ/proto/EnMath.c:413);
    //! die Zuweisung an int ist die Rundung auf die Stufe, um die es geht.
    private static int SyncBucket(float v)
    {
        int bucket = Math.Round(v * 100.0);
        return bucket;
    }

    //! Portionszaehler. 0 = kein Portionsgericht (15). Vanilla-quantity ist
    //! als Zaehler unbrauchbar (01 V5) - deshalb ein eigener int.
    static bool SetPortions(ItemBase item, int count)
    {
        ChefZ_ItemStateComponent comp = Of(item);
        if (!comp || !IsServer())
            return false;

        comp.m_ChefZ_Portions = Math.Clamp(count, 0, ChefZ_SyncLimits.PORTIONS_MAX);
        item.SetSynchDirty();
        return true;
    }

    //! Rueckgabeklasse des Behaelters (16). Nur persistiert, nie gesynct.
    static bool SetReturnContainer(ItemBase item, string cls)
    {
        ChefZ_ItemStateComponent comp = Of(item);
        if (!comp || !IsServer())
            return false;

        comp.m_ChefZ_ReturnContainer = cls;
        return true;
    }

    //==========================================================================
    // Die Projektionsregel (06 §3)
    //==========================================================================

    /**
     *   1. Traegt das Item eine gesetzte Zustandsvariable?
     *        ja  -> diese zurueckgeben              (Uebergang ohne Klassenwechsel)
     *   2. Hat seine Klasse einen defaultState im Ingredient-Binding?
     *        ja  -> diesen zurueckgeben             (V1-NORMALFALL)
     *   3. Hat das Item eine Vanilla-FoodStage?
     *        ja  -> mappen
     *   4. sonst -> INVALID
     *
     * Der Aufruf ist bewusst auch fuer Nicht-ChefZ-Items zulaessig: Schritt 2
     * und 3 beantworten die Frage fuer ein deklariertes Vanilla-Item genauso
     * gut, und der ChefZ_FactCollector fragt fuer jedes Item im Topf.
     */
    static ChefZ_Sym GetState(ItemBase item)
    {
        if (!item)
            return ChefZ_SymbolTable.INVALID;

        // Schritt 1
        ChefZ_ItemStateComponent comp = Of(item);
        if (comp && comp.HasState())
        {
            ChefZ_Sym own = comp.ResolveStateSym();
            if (ChefZ_SymbolTable.IsValid(own))
                return own;
        }

        // Schritt 2 - der V1-Normalfall: die Klasse IST der Zustand (06 E2).
        ChefZ_IngredientInfo info = ChefZ_IngredientManager.Get().ResolveByName(item.GetType());
        if (info && ChefZ_SymbolTable.IsValid(info.defaultState))
            return info.defaultState;

        // Schritt 3
        Edible_Base edible = Edible_Base.Cast(item);
        if (edible)
        {
            // ACHTUNG: Edible_Base.GetFoodStageType() greift ohne Nullpruefung
            // auf GetFoodStage() durch (Edible_Base.c:531). Ein Edible_Base
            // ohne FoodStage - etwa ein leerer Topf - wuerde damit abstuerzen.
            FoodStage stage = edible.GetFoodStage();
            if (stage)
                return ChefZ_StateManager.Get().FromVanillaStage(stage.GetFoodStageType());
        }

        // Schritt 4
        return ChefZ_SymbolTable.INVALID;
    }

    //==========================================================================
    // Vererbung beim Klassentausch und beim Kochen (06 §4.3)
    //==========================================================================

    /**
     * Uebernimmt Zustand, Qualitaet, Frische, Portionen und die
     * Behaelterbindung von einer Quelle.
     *
     * @param freshnessCarry  Faktor auf die uebernommene Frische. Negativ
     *        heisst "unveraendert uebernehmen"; das ist der Sentinelfall, wenn
     *        ein Rezept nichts sagt.
     *
     * Temperatur, Agenten, Health und Nassheit stehen bewusst NICHT hier: sie
     * sind Vanilla-Eigenschaften, und Vanilla hat mit
     * MiscGameplayFunctions.TransferItemProperties bereits eine Routine dafuer.
     * Sie nachzubauen hiesse, Vanilla-Verhalten zu erraten.
     */
    static void InheritFrom(ItemBase target, ItemBase source, float freshnessCarry)
    {
        ChefZ_ItemStateComponent dst = Of(target);
        ChefZ_ItemStateComponent src = Of(source);
        if (!dst || !src || !IsServer())
            return;

        dst.m_ChefZ_StateOrdinal    = src.m_ChefZ_StateOrdinal;
        dst.m_ChefZ_StatePersist    = src.m_ChefZ_StatePersist;
        dst.m_ChefZ_QualityOrdinal  = src.m_ChefZ_QualityOrdinal;
        dst.m_ChefZ_QualityPersist  = src.m_ChefZ_QualityPersist;
        dst.m_ChefZ_Portions        = src.m_ChefZ_Portions;
        dst.m_ChefZ_ReturnContainer = src.m_ChefZ_ReturnContainer;

        float carry = freshnessCarry;
        if (carry < 0.0)
            carry = 1.0;
        dst.m_ChefZ_Freshness = Math.Clamp(src.m_ChefZ_Freshness * carry, 0.0, 1.0);

        target.SetSynchDirty();
    }

    //==========================================================================
    // PERSISTENZ (06 §5, 06 §6, V-B §2)
    //==========================================================================

    /**
     * Schreibt den ChefZ-Block. IMMER, in fester Breite.
     *
     * Aufzurufen NACH super.OnStoreSave(ctx) - der Vanilla-Strom kommt zuerst,
     * ausnahmslos (V-B §2 Folge 2).
     *
     * Ein Item ohne Block waere kein Sonderfall, sondern eine Falle: sobald
     * ein Content-Modul zwischen zwei Serverstarts dazukaeme, laege der
     * Lesestrom jedes gespeicherten Items dieser Klasse um vier Bytes daneben.
     */
    static void Save(notnull ItemBase item, ParamsWriteContext ctx)
    {
        // Durchweg lokale Variablen und keine Konstanten oder Ausdruecke
        // direkt im Write: ParamsWriteContext.Write nimmt seinen Parameter als
        // "void" entgegen und braucht damit etwas Adressierbares. Dieselbe
        // Schreibweise benutzt Vanilla (Edible_Base.c:317-318).
        int magic   = MAGIC;
        int version = VERSION;
        ctx.Write(magic);
        ctx.Write(version);

        int    statePersist    = NO_HASH;
        int    qualityPersist  = NO_HASH;
        float  freshness       = FRESHNESS_FULL;
        int    portions        = NO_PORTIONS;
        string returnContainer = "";

        // Persistiert wird der HASH, nie der Sync-Ordinal (03 E2). Der Ordinal
        // verschiebt sich, sobald ein Zustand dazukommt - ein gespeicherter
        // Ordinal bezeichnete nach einem Content-Update ein anderes Symbol.
        //
        // Fehlt der Block (jemand ruft Save() von aussen auf einem fremden
        // Item), werden die neutralen Werte geschrieben. Die BREITE bleibt in
        // jedem Fall dieselbe - darum geht es hier.
        ChefZ_ItemStateComponent comp = Of(item);
        if (comp)
        {
            statePersist    = comp.m_ChefZ_StatePersist;
            qualityPersist  = comp.m_ChefZ_QualityPersist;
            freshness       = comp.m_ChefZ_Freshness;
            portions        = comp.m_ChefZ_Portions;
            returnContainer = comp.m_ChefZ_ReturnContainer;
        }

        ctx.Write(statePersist);
        ctx.Write(qualityPersist);
        ctx.Write(freshness);
        ctx.Write(portions);
        ctx.Write(returnContainer);
    }

    /**
     * Liest den ChefZ-Block.
     *
     * Gibt IMMER true zurueck. Das ist Absicht und die wichtigste Zeile des
     * Ladepfads: false aus OnStoreLoad laesst das Item aus dem Spielstand
     * verschwinden. Ein ChefZ-Zustand, der nicht lesbar ist, darf einen
     * Spieler nicht sein Fleisch kosten - er kostet ihn den Zustand, mehr
     * nicht (02 §8: "jeder Fehler bewegt das System Richtung weniger ChefZ,
     * nie Richtung falsches ChefZ").
     *
     * @param version  die STORE-Version des Spiels. Sie wird nicht ausgewertet:
     *        der ChefZ-Block fuehrt seine eigene VERSION, weil er sich
     *        unabhaengig von DayZ-Releases aendert.
     */
    static bool Load(notnull ItemBase item, ParamsReadContext ctx, int version)
    {
        ChefZ_ItemStateComponent comp = Of(item);
        if (comp)
            comp.SetDefaults();

        int magic;
        if (!ctx.Read(magic))
        {
            // Kein Block da. Auf einer ChefZ-eigenen Klasse heisst das: das
            // Item wurde von einer aelteren Core-Fassung ohne Block
            // geschrieben. Defaults, weiter, kein Datenverlust (06 §7).
            return true;
        }

        if (magic != MAGIC)
        {
            // 06 §7: Kontext NICHT weiterlesen. MAGIC richtet den Strom nicht
            // aus - es verhindert nur, dass ChefZ fremde Bytes als eigene
            // deutet (V-B §2 Folge 3).
            Note(ChefZ_LogLevel.WARN, "state.magic." + item.GetType(), "\"" + item.GetType() + "\": im Spielstand steht an der Stelle des ChefZ-Blocks " + "kein ChefZ-Block. Der Zustand faellt auf die Vorgabe der Klasse zurueck, das " + "Item bleibt vollstaendig spielbar. Ursache ist fast immer ein anderer Mod, " + "der von dieser Klasse ableitet und vor ChefZ schreibt.");
            return true;
        }

        int blockVersion;
        if (!ctx.Read(blockVersion))
            return true;

        if (blockVersion > VERSION)
        {
            // 06 §7: Rest ueberspringen, Defaults, WARN. Ein neuerer Block
            // kann Felder enthalten, die dieser Core nicht kennt - ihn zu
            // raten waere schlimmer, als ihn liegen zu lassen.
            Note(ChefZ_LogLevel.WARN, "state.version." + blockVersion.ToString(), "Der gespeicherte ChefZ-Block hat Version " + blockVersion.ToString() + ", dieser Core kennt " + VERSION.ToString() + ". Der Rest des Blocks wird " + "uebersprungen und die Vorgaben gelten. Das passiert nach einem Downgrade " + "des Mods.");
            return true;
        }

        int    statePersist;
        int    qualityPersist;
        float  freshness;
        int    portions;
        string returnContainer;

        if (!ctx.Read(statePersist))    return ReadFailed(item, "statePersist");
        if (!ctx.Read(qualityPersist))  return ReadFailed(item, "qualityPersist");
        if (!ctx.Read(freshness))       return ReadFailed(item, "freshness");
        if (!ctx.Read(portions))        return ReadFailed(item, "portions");
        if (!ctx.Read(returnContainer)) return ReadFailed(item, "returnContainer");

        if (!comp)
            return true;

        comp.m_ChefZ_StatePersist    = statePersist;
        comp.m_ChefZ_QualityPersist  = qualityPersist;
        comp.m_ChefZ_Freshness       = Math.Clamp(freshness, 0.0, 1.0);
        comp.m_ChefZ_Portions        = Math.Clamp(portions, 0, ChefZ_SyncLimits.PORTIONS_MAX);
        comp.m_ChefZ_ReturnContainer = returnContainer;

        return true;
    }

    //! Ein abgebrochener Lesevorgang kostet den Zustand, nicht das Item.
    private static bool ReadFailed(notnull ItemBase item, string field)
    {
        Note(ChefZ_LogLevel.ERR, "state.readfail." + field, "Der ChefZ-Block von \"" + item.GetType() + "\" bricht beim Feld \"" + field + "\" ab. Der Block gilt als leer, das Item bleibt erhalten. Das ist ein " + "Formatfehler im Spielstand, kein Datenfehler in der Konfiguration.");
        return true;
    }

    /**
     * Nach dem Laden: aus dem Hash wieder ein Symbol und daraus den Ordinal
     * machen (06 §5, "LADEN").
     *
     * Der Ordinal steht NICHT im Spielstand - er wird hier neu abgeleitet.
     * Genau deshalb ist es harmlos, dass ein neuer Zustand alle nachfolgenden
     * Ordinale verschiebt (03 E3).
     */
    static void ResolveAfterLoad(ItemBase item)
    {
        ChefZ_ItemStateComponent comp = Of(item);
        if (!comp)
            return;

        ChefZ_StateManager mgr = ChefZ_StateManager.Get();

        if (comp.m_ChefZ_StatePersist != NO_HASH)
        {
            ChefZ_Sym sym = mgr.FromPersistHash(comp.m_ChefZ_StatePersist);
            if (ChefZ_SymbolTable.IsValid(sym))
            {
                comp.m_ChefZ_StateOrdinal = mgr.GetSyncOrdinal(sym);
            }
            else
            {
                // 03 §7 / 06 §7: WARN EINMAL JE KLASSE, nicht je Item - sonst
                // Logflut nach einem Content-Rueckbau. Das Item faellt auf den
                // defaultState seiner Klasse zurueck (Schritt 2 der
                // Projektionsregel) und bleibt spielbar.
                Note(ChefZ_LogLevel.WARN, "state.lostpersist." + item.GetType(), "\"" + item.GetType() + "\": der gespeicherte Zustand (Hash " + comp.m_ChefZ_StatePersist.ToString() + ") ist keinem geladenen Zustand " + "mehr zuzuordnen. Das Item faellt auf die Vorgabe seiner Klasse zurueck. " + "Ursache: der Zustand wurde aus dem Content entfernt oder umbenannt. " + "Diese Meldung erscheint je Klasse genau einmal.");

                comp.m_ChefZ_StatePersist = NO_HASH;
                comp.m_ChefZ_StateOrdinal = NO_ORDINAL;
            }
        }

        ChefZ_IdentityMap ids = QualityIdentities();
        if (comp.m_ChefZ_QualityPersist != NO_HASH && ids)
        {
            ChefZ_Sym tier = ids.FromPersistHash(comp.m_ChefZ_QualityPersist);
            if (ChefZ_SymbolTable.IsValid(tier))
            {
                comp.m_ChefZ_QualityOrdinal = ids.ToSyncOrdinal(tier);
            }
            else
            {
                Note(ChefZ_LogLevel.WARN, "quality.lostpersist." + item.GetType(), "\"" + item.GetType() + "\": die gespeicherte Qualitaetsstufe ist keiner " + "geladenen Stufe mehr zuzuordnen. Sie wird verworfen; das Item bleibt " + "spielbar. Diese Meldung erscheint je Klasse genau einmal.");
                comp.m_ChefZ_QualityPersist = NO_HASH;
                comp.m_ChefZ_QualityOrdinal = NO_ORDINAL;
            }
        }

        Sync(item);
    }

    //! 06 §4.3. SetSynchDirty ist die einzige Bewegung Richtung Client - es
    //! gibt keinen ChefZ-RPC.
    static void Sync(notnull ItemBase item)
    {
        if (!IsServer())
            return;
        item.SetSynchDirty();
    }

    //==========================================================================
    // Innereien
    //==========================================================================

    private static bool IsServer()
    {
        return g_Game && g_Game.IsServer();
    }

    private static void Note(int level, string key, string msg)
    {
        ChefZ_Log.Once(level, ChefZ_LogChannel.STATE, key, msg);
    }

    string ToLine()
    {
        string chefzTxt1 = "state ord=" + m_ChefZ_StateOrdinal.ToString() + " hash=" + m_ChefZ_StatePersist.ToString() + "  qual ord=";
        chefzTxt1 = chefzTxt1 + m_ChefZ_QualityOrdinal.ToString() + " hash=" + m_ChefZ_QualityPersist.ToString() + "  frische=" + m_ChefZ_Freshness.ToString();
        chefzTxt1 = chefzTxt1 + "  portionen=" + m_ChefZ_Portions.ToString() + "  rueckgabe=\"" + m_ChefZ_ReturnContainer + "\"";
        return chefzTxt1;
    }
}
