//==============================================================================
// ChefZ_ProcessingStation_Base - die abstrakte Verarbeitungsstation
//
// Entwurf: 11 §4 (Schnittstelle woertlich), 11 §5 (Datenfluss STATION_ACTION
// und STATION_TIMED), 11 §6 (Zustand und Persistenzform), 11 §7
// (Fehlerverhalten, Zeile fuer Zeile), 11 E6 (eigene Stationen statt Vanillas
// Smoking-Slots), 11 E7 (Jobs an der Station, nicht in einem globalen
// Scheduler), 03 E2 (Persistenz ueber Name.Hash()), V-B §2 (eigener
// OnStoreSave-Block mit MAGIC und VERSION).
//
// ---------------------------------------------------------------------------
// KEINE KONKRETE STATION. Kein CfgVehicles-Eintrag.
// ---------------------------------------------------------------------------
// Diese Klasse ist eine BASIS, genau wie ChefZ_Edible_Base und
// ChefZ_Item_Base (06 §4.3). Der Core bringt bewusst keinen
// CfgVehicles-Eintrag mit - er darf keinen Content anlegen (Invariante I3).
//
// FUER CONTENT-AUTOREN, dieselbe Andockregel wie bei den beiden anderen:
//
//   config.cpp   class ChefZ_MeatGrinder : <eine Vanilla-Klasse> { ... };
//                class CfgChefZStations {
//                    class ChefZ_MeatGrinder {
//                        stationCategories[] = {"..."};
//                        processes[]         = {"..."};
//                        parallelSlots       = 1;
//                    };
//                };
//   Skript       class ChefZ_MeatGrinder extends ChefZ_ProcessingStation_Base {}
//
// Mehr ist nicht noetig. Kein Core-Code, keine eigene Action, keine eigene
// Persistenz (11 §3, letzter Absatz).
//
// ---------------------------------------------------------------------------
// Warum eigene Stationen und nicht Vanillas Smoking-Slots (11 E6)
// ---------------------------------------------------------------------------
// Vanillas Cooking.SmokeItem kennt GENAU EINEN Uebergang: RAW -> DRIED, sonst
// BURNED (01 V14). Die V1-Preservation-Matrix verlangt vier Uebergaenge mit
// VERSCHIEDENEN Haltbarkeiten (Production Map §56/§65). Das ist in Vanillas
// Kette nicht abbildbar - und der Versuch haette entweder ein
// "modded class FireplaceBase" gekostet (Kollisionsflaeche, Verstoss gegen I6)
// oder einen zweiten Hook auf Cooking.
//
// Vanilla-Raeuchern in den Smoking-Slots eines Fasses bleibt dadurch EXAKT wie
// es ist. Diese Datei fasst Vanillas Kochkette an keiner Stelle an.
//
// ---------------------------------------------------------------------------
// Bekannte Grenze in V1: ein Slot RESERVIERT keine Items
// ---------------------------------------------------------------------------
// Ein laufender Job merkt sich, WELCHER Transform laeuft (als Hash), nicht,
// WELCHE Items er verarbeiten wird. Handles sind Positionen in einer
// fluechtigen Liste und ueber einen Serverneustart hinweg bedeutungslos
// (11 §6).
//
// Die Folge: an einer Station mit mehreren Slots koennen zwei Jobs desselben
// Transforms laufen, obwohl das Material nur fuer einen reicht. Der erste
// Abschluss bindet neu, verbraucht und ist fertig; der zweite bindet neu,
// findet nichts mehr und bricht ab - OHNE VERLUST (11 §7, "Job laeuft,
// Eingangs-Item wird entfernt"). Der Spieler verliert Wartezeit, nie Material.
//
// Das ist offen benannt und nicht versteckt. Die Alternative waere eine
// persistente Reservierung ueber Netz-IDs, also genau der globale Scheduler,
// den 11 E7 verwirft.
//
// ---------------------------------------------------------------------------
// Zwei Abweichungen von den Signaturen in 11 §4
// ---------------------------------------------------------------------------
// 1. ChefZ_CanStart und ChefZ_BeginJob nehmen (ItemBase inHands, int actorId)
//    statt (PlayerBase actor). Die Station braucht vom Spieler genau zwei
//    Dinge: was er in der Hand haelt (eine der beiden Werkzeugquellen) und
//    seine Identitaets-ID (fuer Faehigkeiten, 17 §3.3). Ein PlayerBase-Zeiger
//    waere mehr Zugriff als noetig - und ein Timer-Tick, der einen Job
//    abschliesst, hat gar keinen Spieler.
//
// 2. ChefZ_MeetsEnvironment nimmt ein ChefZ_CompiledProcess statt eines
//    ChefZ_ProcessDef. Die Rohform traegt Sentinel und ungeklemmte Werte; die
//    Frage "reicht die Temperatur" ist auf ihr gar nicht beantwortbar, ohne
//    die Klemmung des Compilers zu wiederholen.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_ProcessingStation_Base extends ItemBase
{
    //--- Persistenz (V-B §2: eigener Block mit MAGIC und VERSION) ------------
    //
    // "CHZP" - ChefZ Processing. Ein anderer Wert als der Zustandsblock
    // (ChefZ_ItemStateComponent.MAGIC), damit ein verschobener Strom nicht
    // ausgerechnet den falschen Block als richtig erkennt.
    //
    // EHRLICH BENANNT (V-B §2 Folge 3): MAGIC richtet einen verschobenen Strom
    // NICHT wieder aus. Es verhindert nur, dass ChefZ fremde Bytes als eigene
    // deutet.
    static const int MAGIC   = 0x43485A50;
    static const int VERSION = 1;

    //! Wie oft die Station ihre Jobs fortschreibt.
    //!
    //! 2 Sekunden und nicht 0.1: ein Raeuchervorgang dauert Minuten, und der
    //! Fortschrittsbalken wird ohnehin nur auf zwei Nachkommastellen
    //! synchronisiert. Ein feinerer Takt kostete Rechenzeit je Station und
    //! Netzverkehr, ohne dass irgendjemand einen Unterschied saehe.
    static const float TICK_INTERVAL_SEC = 2.0;

    //--- Jobs (11 §4) ---------------------------------------------------------
    protected ref array<ref ChefZ_ProcessJob> m_ChefZ_Jobs;

    //--- Was diese Station anbietet, einmal aus CfgChefZStations --------------
    protected ref array<ChefZ_Sym> m_ChefZ_Processes;
    protected bool  m_ChefZ_ProcessesLoaded;
    protected int   m_ChefZ_SlotCount;
    protected float m_ChefZ_SpeedMultiplier;
    protected bool  m_ChefZ_NeedsFuel;

    //--- Sync (11 §6: "Progress01 ist HIER synchronisiert") ------------------
    //
    // 0 = kein Job laeuft. Sonst der INDEX des Prozesses in der
    // processes[]-Liste der Station PLUS EINS.
    //
    // Warum der Stationsindex und kein globaler Prozessordinal: Prozesse sind
    // keine sync-relevante Recordart (03 §4, ChefZ_RecordKind.IsSyncRelevant),
    // es gibt also keine ChefZ_IdentityMap fuer sie. Der Index in der
    // processes[]-Liste ist dagegen auf Client und Server garantiert
    // identisch - er kommt aus der GAME-CONFIG (Rang 1), die beide Seiten
    // gemergt vorliegen haben (02 §2).
    protected int   m_ChefZ_ActiveProcessOrdinal;
    protected float m_ChefZ_Progress01;

    //--- Eigener Tick (11 E7) -------------------------------------------------
    protected ref Timer m_ChefZ_JobTimer;

    //--------------------------------------------------------------------------

    void ChefZ_ProcessingStation_Base()
    {
        m_ChefZ_Jobs                 = new array<ref ChefZ_ProcessJob>();
        m_ChefZ_Processes            = new array<ChefZ_Sym>();
        m_ChefZ_ProcessesLoaded      = false;
        m_ChefZ_SlotCount            = 1;
        m_ChefZ_SpeedMultiplier      = 1.0;
        m_ChefZ_NeedsFuel            = false;
        m_ChefZ_ActiveProcessOrdinal = 0;
        m_ChefZ_Progress01           = 0.0;

        RegisterNetSyncVariableInt("m_ChefZ_ActiveProcessOrdinal",
                                   0, ChefZ_ProcessingLimits.PROCESS_ORDINAL_MAX + 1);
        RegisterNetSyncVariableFloat("m_ChefZ_Progress01", 0.0, 1.0, 2);
    }

    void ~ChefZ_ProcessingStation_Base()
    {
        // Hier - und NUR hier - wird das Timerobjekt auch freigegeben. Die
        // Station ist zu diesem Zeitpunkt fort, ein laufender Rueckruf haette
        // ohnehin nichts mehr zu ticken.
        if (m_ChefZ_JobTimer)
        {
            m_ChefZ_JobTimer.Stop();
            m_ChefZ_JobTimer = null;
        }
    }

    override void EEInit()
    {
        super.EEInit();
        ChefZ_LoadSupportedProcesses();
    }

    //==========================================================================
    // Was die Station anbietet (11 §4)
    //==========================================================================

    /**
     * Liest die eigene Deklaration aus dem ChefZ_ProcessingManager - EINMAL.
     *
     * Nicht direkt aus der Config: der Manager hat den Eintrag bereits
     * geprueft, unbekannte Prozesse herausgeworfen und die Grenzen geklemmt
     * (11 §7). Eine zweite Leseroutine waere eine zweite Gelegenheit, dieselbe
     * Config anders zu deuten.
     *
     * Ohne Eintrag bleibt die Liste leer, und die Station ist inerte Deko -
     * genau das verlangt 11 §7 fuer den Fall "keine Prozessdaten geladen".
     * Sie ist dann trotzdem ein vollwertiges Item: aufhebbar, ablegbar,
     * zerstoerbar.
     */
    void ChefZ_LoadSupportedProcesses()
    {
        if (m_ChefZ_ProcessesLoaded)
            return;

        // NICHT als geladen markieren, solange der Manager nicht gebaut ist.
        //
        // Der Aufruf kommt aus EEInit, und ob der Config Manager zu diesem
        // Zeitpunkt schon gelaufen ist, haengt an der Reihenfolge von
        // Missionsstart und Entity-Erzeugung - also an etwas, worauf diese
        // Klasse keinen Einfluss hat. Wuerde hier vorschnell "geladen"
        // gesetzt, bliebe die Station fuer den Rest ihres Lebens leer, und
        // niemand koennte erklaeren, warum ausgerechnet SIE nichts anbietet.
        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        if (!mgr.IsReady())
            return;

        m_ChefZ_ProcessesLoaded = true;
        m_ChefZ_Processes.Clear();

        ChefZ_Sym stationClass = ChefZ_SymbolTable.Lookup(GetType());
        ChefZ_CompiledStation def = mgr.GetStation(stationClass);
        if (!def)
        {
            // KEINE Meldung. Eine Klasse, die von dieser Basis ableitet, aber
            // keinen CfgChefZStations-Eintrag hat, ist ein zulaessiger
            // Zwischenstand im Content-Bau - und eine Fehlermeldung je
            // gespawnter Instanz waere eine Logflut.
            return;
        }

        for (int i = 0; i < def.processes.Count(); i++)
            m_ChefZ_Processes.Insert(def.processes.Get(i));

        m_ChefZ_SlotCount       = def.parallelSlots;
        m_ChefZ_SpeedMultiplier = def.speedMultiplier;
        m_ChefZ_NeedsFuel       = def.needsFuel;

        EnsureSlots();
    }

    bool ChefZ_SupportsProcess(ChefZ_Sym process)
    {
        ChefZ_LoadSupportedProcesses();
        if (!ChefZ_SymbolTable.IsValid(process))
            return false;
        return m_ChefZ_Processes.Find(process) >= 0;
    }

    void ChefZ_GetSupportedProcesses(out array<ChefZ_Sym> outProcesses)
    {
        if (!outProcesses)
            outProcesses = new array<ChefZ_Sym>();
        outProcesses.Clear();

        ChefZ_LoadSupportedProcesses();
        for (int i = 0; i < m_ChefZ_Processes.Count(); i++)
            outProcesses.Insert(m_ChefZ_Processes.Get(i));
    }

    int ChefZ_GetProcessCount()
    {
        ChefZ_LoadSupportedProcesses();
        return m_ChefZ_Processes.Count();
    }

    ChefZ_Sym ChefZ_GetProcessAt(int index)
    {
        ChefZ_LoadSupportedProcesses();
        if (index < 0 || index >= m_ChefZ_Processes.Count())
            return ChefZ_SymbolTable.INVALID;
        return m_ChefZ_Processes.Get(index);
    }

    int ChefZ_GetSlotCount()
    {
        ChefZ_LoadSupportedProcesses();
        return m_ChefZ_SlotCount;
    }

    //==========================================================================
    // Umgebung - die zwei Haken fuer Content-Module
    //==========================================================================

    /**
     * Hat die Station Waerme?
     *
     * Die Basis antwortet "nein". Ein Raeucherschrank, der an einer
     * Feuerstelle haengt, ueberschreibt das - und ein Trockenrahmen laesst es
     * so, weil Trocknen keine Waerme braucht.
     *
     * "Nein" und nicht "ja" ist die sichere Vorgabe: ein Prozess mit
     * requiresHeat pausiert damit, statt an einer kalten Station zu laufen.
     * Der umgekehrte Fehler waere ein Raeuchervorgang ohne Feuer, und der
     * faellt niemandem auf.
     */
    bool ChefZ_HasHeat()
    {
        return false;
    }

    /**
     * Hat die Station Brennstoff (oder braucht sie keinen)?
     *
     * Die Basis antwortet "ja", solange needsFuel nicht gesetzt ist - und
     * "nein", sobald es gesetzt ist. Damit ist der Default fuer eine Station
     * OHNE Brennstoffbedarf "laeuft", und fuer eine MIT Brennstoffbedarf
     * "laeuft nicht, bis das Content-Modul etwas anderes sagt".
     *
     * Genau diese Richtung ist richtig: needsFuel = 1 zu schreiben und dann zu
     * vergessen, die Abfrage zu ueberschreiben, fuehrt zu einer Station, die
     * nicht arbeitet - sichtbar und meldbar. Die Gegenrichtung fuehrte zu
     * einer Station, die ohne Brennstoff arbeitet, und das faellt nie auf.
     */
    bool ChefZ_IsPowered()
    {
        ChefZ_LoadSupportedProcesses();
        return !m_ChefZ_NeedsFuel;
    }

    /**
     * Stimmt die Umgebung fuer diesen Prozess (11 §4)?
     *
     * Bei false PAUSIERT ein laufender Job - er bricht nicht ab und laeuft nie
     * zurueck (11 §7).
     */
    bool ChefZ_MeetsEnvironment(notnull ChefZ_CompiledProcess def, out string reason)
    {
        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        ChefZ_BuildContext(null, 0, ctx);
        return def.MeetsEnvironment(ctx, reason);
    }

    /**
     * Baut den Auswertungskontext dieser Station.
     *
     * @param inHands  das Item in der Hand des Handelnden, oder null. Es ist
     *                 eine der beiden Werkzeugquellen (die andere ist die
     *                 Station selbst, siehe ChefZ_FactCollector.
     *                 CollectToolGroups).
     * @param actorId  0 = niemand beteiligt (Timer-Tick).
     */
    void ChefZ_BuildContext(ItemBase inHands, int actorId, notnull ChefZ_ProcessContext ctx)
    {
        ctx.Reset();

        ChefZ_LoadSupportedProcesses();

        ctx.stationClass       = ChefZ_SymbolTable.Lookup(GetType());
        ctx.stationTemperature = GetTemperature();
        ctx.hasHeat            = ChefZ_HasHeat();
        ctx.stationPowered     = ChefZ_IsPowered();
        ctx.actorIdentityId    = actorId;

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        ChefZ_CompiledStation def = mgr.GetStation(ctx.stationClass);
        if (def)
        {
            for (int i = 0; i < def.categories.Count(); i++)
                ctx.AddStationCategory(def.categories.Get(i));
        }

        ChefZ_FactCollector.CollectToolGroups(inHands, this, ctx);
    }

    //==========================================================================
    // Kann hier ein Prozess starten (11 §4)?
    //==========================================================================

    /**
     * Rein lesend, kein Seiteneffekt. Aus ActionCondition gefahrlos rufbar.
     *
     * reason traegt bei false die Begruendung im Klartext - fuer den Trace,
     * nicht fuer das HUD. 11 §7 ist an dieser Stelle deutlich: "Werkzeug
     * fehlt -> Action erscheint nicht; DEBUG mit der fehlenden
     * Werkzeuggruppe. KEINE irrefuehrende HUD-Meldung."
     */
    bool ChefZ_CanStart(ChefZ_Sym process, ItemBase inHands, int actorId, out string reason)
    {
        reason = "";

        if (!ChefZ_SupportsProcess(process))
        {
            reason = "diese Station bietet den Prozess nicht an";
            return false;
        }

        if (ChefZ_FreeSlotIndex() < 0)
        {
            reason = "alle " + ChefZ_GetSlotCount().ToString() + " Slots sind belegt";
            return false;
        }

        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        ChefZ_BuildContext(inHands, actorId, ctx);

        ChefZ_FactSnapshot snapshot;
        array<ItemBase>    entities;
        ChefZ_FactCollector.CollectFromCargo(this, snapshot, entities);
        snapshot.SortStable();

        ChefZ_TransformMatch match;
        if (!ChefZ_ProcessingManager.Get().FindTransform(process, ctx, snapshot, null, match))
        {
            reason = match.failReason;
            return false;
        }

        return true;
    }

    //! Erster freier Slot, oder -1.
    int ChefZ_FreeSlotIndex()
    {
        EnsureSlots();
        for (int i = 0; i < m_ChefZ_Jobs.Count(); i++)
        {
            if (!m_ChefZ_Jobs.Get(i).IsActive())
                return i;
        }
        return -1;
    }

    int ChefZ_ActiveJobCount()
    {
        int n = 0;
        for (int i = 0; i < m_ChefZ_Jobs.Count(); i++)
        {
            if (m_ChefZ_Jobs.Get(i).IsActive())
                n++;
        }
        return n;
    }

    //==========================================================================
    // Job starten (11 §5, STATION_TIMED)
    //==========================================================================

    /**
     * Legt einen Job in den ersten freien Slot.
     *
     * SERVER. Ausschliesslich - die Slotbelegung ist eine autoritative
     * Entscheidung. 11 §7: "Zwei Spieler starten gleichzeitig auf demselben
     * Slot -> ChefZ_BeginJob prueft die Slotbelegung SERVERSEITIG; der zweite
     * bekommt false und die Action bricht ab."
     *
     * Der Job merkt sich HASHES, nicht Symbole (03 E2, 11 §6).
     */
    bool ChefZ_BeginJob(ChefZ_Sym process, ItemBase inHands, int actorId, out string err)
    {
        err = "";

        if (!g_Game || !g_Game.IsServer())
        {
            err = "Jobstart ausserhalb des Servers angefordert";
            return false;
        }

        int slot = ChefZ_FreeSlotIndex();
        if (slot < 0)
        {
            err = "alle Slots sind belegt";
            return false;
        }

        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        ChefZ_BuildContext(inHands, actorId, ctx);

        ChefZ_FactSnapshot snapshot;
        array<ItemBase>    entities;
        ChefZ_FactCollector.CollectFromCargo(this, snapshot, entities);
        snapshot.SortStable();

        ChefZ_TransformMatch match;
        if (!ChefZ_ProcessingManager.Get().FindTransform(process, ctx, snapshot, null, match))
        {
            err = match.failReason;
            return false;
        }

        // 17 §4: ChefZ_OnProcessJobStarted ist STORNIERBAR und ausdruecklich
        // NICHT XP-tauglich. Der Abbruch geschieht VOR jeder Wirkung - es ist
        // noch nichts belegt und nichts veraendert.
        string cancelReason;
        if (RaiseJobStarted(match, actorId, cancelReason))
        {
            err = cancelReason;
            return false;
        }

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();

        ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(slot);
        job.Clear();
        job.transformPersistHash = mgr.GetTransformPersistHash(match.transformSym);
        job.processPersistHash   = mgr.GetProcessPersistHash(process);
        job.transformSym         = match.transformSym;
        job.processSym           = process;
        job.durationSec          = mgr.GetDuration(match, ctx);
        job.elapsedSec           = 0.0;
        job.actorIdentityId      = actorId;

        // Ein Job ohne Hash waere nach einem Neustart nicht aufloesbar - und
        // ein Slot, der sich nie wieder freigibt. Lieber gar nicht erst
        // anlegen und sagen, warum.
        if (job.transformPersistHash == ChefZ_ProcessJob.NO_HASH)
        {
            job.Clear();
            err = "der Transform \"" + match.transformId + "\" hat keinen Persistenzschluessel "
                + "(Hash-Kollision beim Laden?) - der Job wird nicht angelegt, damit er nach "
                + "einem Neustart nicht als Geisterjob zurueckbleibt.";
            return false;
        }

        StartJobTimer();
        SyncFromJobs();

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.INFO))
        {
            ChefZ_Log.Info(ChefZ_LogChannel.PROCESS,
                "Job gestartet: " + match.transformId + " an " + GetType()
                + " Slot " + slot.ToString() + ", " + job.durationSec.ToString() + "s.");
        }

        return true;
    }

    //==========================================================================
    // Ticken (11 §5, STATION_TIMED)
    //==========================================================================

    //! Der Timer-Rueckruf. Er heisst so, wie Timer.Run() ihn nennt.
    void ChefZ_OnJobTick()
    {
        ChefZ_TickJobs(TICK_INTERVAL_SEC);
    }

    /**
     * Schreibt alle Jobs fort.
     *
     * Die drei Regeln aus 11 §7, in dieser Reihenfolge:
     *
     *   1. Transform nicht mehr geladen  -> Abbruch OHNE Verlust
     *   2. Umgebung stimmt nicht         -> PAUSE, nie Rueckschritt
     *   3. Fertig                        -> ChefZ_CompleteJob
     */
    void ChefZ_TickJobs(float deltaSec)
    {
        if (!g_Game || !g_Game.IsServer())
            return;
        if (m_ChefZ_Jobs.Count() == 0)
            return;

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        bool anyActive = false;

        for (int i = 0; i < m_ChefZ_Jobs.Count(); i++)
        {
            ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(i);
            if (!job.IsActive())
                continue;

            if (!ResolveJob(job, mgr))
            {
                // 11 §6: "existiert der Transform nicht mehr (Content
                // entfernt), wird der Job abgebrochen, das Eingangs-Item
                // bleibt unveraendert liegen und es gibt ein WARN. Kein
                // Itemverlust durch Content-Aenderungen."
                ChefZ_CancelJob(i, "transform_gone");
                continue;
            }

            ChefZ_CompiledProcess proc = mgr.GetProcess(job.processSym);
            bool running = true;
            if (proc)
            {
                string envReason;
                running = ChefZ_MeetsEnvironment(proc, envReason);
            }

            job.Advance(deltaSec, m_ChefZ_SpeedMultiplier, running);
            anyActive = true;

            if (job.IsComplete())
                ChefZ_CompleteJob(i);
        }

        SyncFromJobs();

        if (!anyActive && ChefZ_ActiveJobCount() == 0)
            StopJobTimer();
    }

    /**
     * Hash -> Symbol, wenn noetig (11 §6, "Beim Laden wird zurueckaufgeloest").
     *
     * false heisst "diesen Transform gibt es nicht mehr". Der Aufrufer bricht
     * den Job dann ab.
     */
    protected bool ResolveJob(notnull ChefZ_ProcessJob job, notnull ChefZ_ProcessingManager mgr)
    {
        if (ChefZ_SymbolTable.IsValid(job.transformSym))
            return true;

        job.transformSym = mgr.TransformFromPersistHash(job.transformPersistHash);
        job.processSym   = mgr.ProcessFromPersistHash(job.processPersistHash);

        return ChefZ_SymbolTable.IsValid(job.transformSym);
    }

    //==========================================================================
    // Job abschliessen (11 §5)
    //==========================================================================

    /**
     * Bindet NEU und fuehrt aus.
     *
     * Das Neubinden ist der Kern und keine Vorsichtsmassnahme: zwischen Start
     * und Abschluss koennen Minuten liegen (11 §6, "ein 40-minuetiger
     * Raeuchervorgang"). Was beim Start passte, muss jetzt nicht mehr da sein -
     * und dem CLIENT oder einem alten Bindungsergebnis zu glauben, waere
     * genau der Fehler, den 11 §5 fuer OnFinishProgressServer ausschliesst.
     *
     * Findet sich kein Transform mehr, wird der Job abgebrochen: kein
     * Ergebnis, KEIN Verbrauch (11 §7, "input_lost").
     */
    bool ChefZ_CompleteJob(int slotIndex)
    {
        if (!g_Game || !g_Game.IsServer())
            return false;
        if (slotIndex < 0 || slotIndex >= m_ChefZ_Jobs.Count())
            return false;

        ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(slotIndex);
        if (!job.IsActive())
            return false;

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        if (!ResolveJob(job, mgr))
        {
            ChefZ_CancelJob(slotIndex, "transform_gone");
            return false;
        }

        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        ChefZ_BuildContext(null, job.actorIdentityId, ctx);

        ChefZ_FactSnapshot snapshot;
        array<ItemBase>    entities;
        ChefZ_FactCollector.CollectFromCargo(this, snapshot, entities);
        snapshot.SortStable();

        ChefZ_TransformMatch match;
        if (!mgr.FindTransform(job.processSym, ctx, snapshot, null, match))
        {
            ChefZ_CancelJob(slotIndex, "input_lost");
            return false;
        }

        // Der neu gebundene Transform kann ein ANDERER sein als der gestartete:
        // jemand hat waehrenddessen etwas Passenderes hineingelegt. Das ist
        // kein Fehler, aber es soll im Log stehen - sonst wundert sich ein
        // Betreiber ueber ein Ergebnis, das er nicht bestellt hat.
        if (match.transformSym != job.transformSym
            && ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS,
                "Job an " + GetType() + " Slot " + slotIndex.ToString() + " startete als \""
                + ChefZ_SymbolTable.NameOrMark(job.transformSym) + "\" und bindet beim "
                + "Abschluss \"" + match.transformId + "\" - der Inhalt hat sich geaendert.");
        }

        array<ItemBase> created;
        string err;

        bool ok = ChefZ_ProcessRunner.Run(this, match, entities, snapshot,
                                          job.actorIdentityId, created, err);

        // Der Slot wird in JEDEM Fall frei. Ein Job, der nach einem
        // gescheiterten Abschluss weiterlaeuft, wuerde es beim naechsten Tick
        // erneut versuchen und dabei nie fertig werden - der Slot waere
        // dauerhaft blockiert.
        job.Clear();
        SyncFromJobs();

        if (ChefZ_ActiveJobCount() == 0)
            StopJobTimer();

        if (!ok)
        {
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PROCESS,
                "station.complete." + GetType(),
                "Der Job an \"" + GetType() + "\" konnte nicht abgeschlossen werden: " + err
                + ". Nichts verbraucht, nichts erzeugt - die Eingaenge liegen unveraendert in "
                + "der Station.");
            return false;
        }

        return true;
    }

    //==========================================================================
    // Job abbrechen (11 §7)
    //==========================================================================

    /**
     * Bricht einen Job ab. Die Eingaenge bleiben UNVERAENDERT.
     *
     * reasonTag ist ein kurzer, maschinenlesbarer Grund ("input_lost",
     * "tool_removed", "transform_gone") - er steht so in 11 §7 und geht
     * unveraendert in das Ereignis. Ein Comp-Modul kann daran unterscheiden,
     * ob ein Spieler abgebrochen hat oder ob etwas schiefging.
     */
    void ChefZ_CancelJob(int slotIndex, string reasonTag)
    {
        if (slotIndex < 0 || slotIndex >= m_ChefZ_Jobs.Count())
            return;

        ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(slotIndex);
        if (!job.IsActive())
            return;

        ChefZ_Sym transformSym = job.transformSym;
        int actorId = job.actorIdentityId;

        job.Clear();
        SyncFromJobs();

        if (ChefZ_ActiveJobCount() == 0)
            StopJobTimer();

        RaiseJobCancelled(transformSym, actorId, reasonTag);

        ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PROCESS,
            "station.cancel." + reasonTag,
            "Ein Job an \"" + GetType() + "\" wurde abgebrochen (" + reasonTag + "). Die "
            + "Eingaenge bleiben unveraendert liegen - es geht nichts verloren.");
    }

    //! Alle Jobs abbrechen. Fuer Adminkommandos und fuer Content-Module, die
    //! eine Station umbauen (Fass zu, Rahmen eingeklappt).
    void ChefZ_CancelAllJobs(string reasonTag)
    {
        for (int i = 0; i < m_ChefZ_Jobs.Count(); i++)
            ChefZ_CancelJob(i, reasonTag);
    }

    //==========================================================================
    // Fortschritt (11 §6)
    //==========================================================================

    float ChefZ_GetProgress01(int slotIndex)
    {
        if (slotIndex < 0 || slotIndex >= m_ChefZ_Jobs.Count())
            return 0.0;
        return m_ChefZ_Jobs.Get(slotIndex).Progress01();
    }

    //! Der SYNCHRONISIERTE Fortschritt - das, was der Client sieht. Auf dem
    //! Server dasselbe wie der Fortschritt des ersten laufenden Jobs.
    float ChefZ_GetSyncedProgress01()
    {
        return m_ChefZ_Progress01;
    }

    //! 0 = kein Job. Sonst der Index des Prozesses in processes[] PLUS EINS.
    int ChefZ_GetActiveProcessOrdinal()
    {
        return m_ChefZ_ActiveProcessOrdinal;
    }

    //! INVALID, wenn kein Job laeuft. Auch auf dem CLIENT gueltig - das ist
    //! der ganze Zweck des Ordinals.
    ChefZ_Sym ChefZ_GetActiveProcess()
    {
        if (m_ChefZ_ActiveProcessOrdinal <= 0)
            return ChefZ_SymbolTable.INVALID;
        return ChefZ_GetProcessAt(m_ChefZ_ActiveProcessOrdinal - 1);
    }

    /**
     * Sync-Variablen aus den Jobs ableiten und - nur bei Aenderung - senden.
     *
     * "Gedrosselt auf die 2. Nachkommastelle" (11 §5). Die Drosselung
     * geschieht hier UND in der Sync-Registrierung: die Registrierung
     * bestimmt, wie viele Bits ueber die Leitung gehen, dieser Vergleich
     * bestimmt, wie oft ueberhaupt gesendet wird. Ohne den Vergleich schickte
     * jede Station alle zwei Sekunden ein Paket, auch wenn sich nichts
     * geaendert hat.
     *
     * Der ERSTE laufende Job bestimmt die Anzeige. Bei mehreren Slots ist das
     * eine Vereinfachung, und sie ist bewusst: ein Trockenrahmen mit vier
     * Fortschrittsbalken braucht ein eigenes UI, und das gehoert nicht in den
     * Core (11 §6 fuehrt genau eine progress01-Zeile).
     */
    protected void SyncFromJobs()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        int   ordinal  = 0;
        float progress = 0.0;

        for (int i = 0; i < m_ChefZ_Jobs.Count(); i++)
        {
            ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(i);
            if (!job.IsActive())
                continue;

            int index = m_ChefZ_Processes.Find(job.processSym);
            if (index >= 0 && index <= ChefZ_ProcessingLimits.PROCESS_ORDINAL_MAX)
                ordinal = index + 1;

            progress = job.Progress01();
            break;
        }

        // Auf zwei Nachkommastellen runden, bevor verglichen wird - sonst
        // meldet jeder Tick eine Aenderung, die ueber die Leitung gar nicht
        // darstellbar waere.
        float rounded = Math.Round(progress * 100.0) / 100.0;

        if (ordinal == m_ChefZ_ActiveProcessOrdinal && rounded == m_ChefZ_Progress01)
            return;

        m_ChefZ_ActiveProcessOrdinal = ordinal;
        m_ChefZ_Progress01           = rounded;
        SetSynchDirty();
    }

    /**
     * 11 §4 nennt OnVariablesSynchronized fuer den Fortschrittsbalken.
     *
     * Die Basis ruft nur super und stellt damit sicher, dass eine Ableitung
     * ihre Anzeige aktualisieren KANN - der Core selbst zeichnet nichts. Ein
     * Fortschrittsbalken ist eine Darstellungsentscheidung des Content-Moduls
     * (Partikel am Raeucherschrank, ein Modellzustand am Trockenrahmen, ein
     * Widget), und der Core hat kein UI (00 §5).
     */
    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
    }

    //==========================================================================
    // Aktionen (11 E1: GENAU EINE Action fuer ALLE Stationsprozesse)
    //==========================================================================

    /**
     * super ZUERST, dann genau eine eigene Action.
     *
     * super zuerst und ohne RemoveAction: eine Station ist ein Item, das ein
     * Spieler aufheben, ablegen und in die Hand nehmen koennen muss. Ihm diese
     * Vanilla-Aktionen zu nehmen, waere ein Eingriff in Vanilla-Verhalten -
     * und der Mod, der das tut, ist der, ueber den sich niemand erklaeren
     * kann.
     *
     * GENAU EINE eigene Action, nicht eine je Prozess (11 E1): der Prozess ist
     * ein LAUFZEITPARAMETER. Ohne diese Entscheidung waere jeder neue
     * Verarbeitungsschritt eine neue Skriptklasse - im Core (verboten: Content
     * im Core) oder verstreut in Modulen (dann kann der Core sie nicht
     * generisch anbieten).
     */
    override void SetActions()
    {
        super.SetActions();
        AddAction(ChefZ_ActionProcessAtStation);
    }

    //==========================================================================
    // Persistenz (11 §6, V-B §2)
    //==========================================================================

    /**
     * Schreibt den Jobblock. IMMER, auch wenn kein Job laeuft.
     *
     * Warum immer: schriebe man ihn bedingt, verschoebe eine
     * Content-Aenderung zwischen zwei Serverstarts den Lesestrom JEDES
     * gespeicherten Items dieser Klasse. Dieselbe Ueberlegung wie beim
     * Zustandsblock (06 §6, V-B §2 Folge 1).
     *
     * Die Breite ist SELBSTBESCHREIBEND: nach MAGIC und VERSION steht die
     * Slotzahl, danach genau so viele Datensaetze. Anders als beim
     * Zustandsblock geht das hier nicht mit fester Breite - parallelSlots
     * kommt aus der Config und darf sich zwischen zwei Starts aendern. Der
     * Leser richtet sich deshalb nach der GESPEICHERTEN Zahl, nie nach der
     * aktuellen Konfiguration.
     *
     * super zuerst, ausnahmslos (V-B §2 Folge 2).
     */
    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);

        // Durchweg lokale Variablen und keine Ausdruecke direkt im Write:
        // ParamsWriteContext.Write nimmt seinen Parameter als "void" entgegen
        // und braucht etwas Adressierbares. Dieselbe Schreibweise benutzt
        // Vanilla (Edible_Base.c:317-318).
        int magic   = MAGIC;
        int version = VERSION;
        ctx.Write(magic);
        ctx.Write(version);

        int count = m_ChefZ_Jobs.Count();
        ctx.Write(count);

        for (int i = 0; i < count; i++)
        {
            ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(i);

            int   transformHash = job.transformPersistHash;
            int   processHash   = job.processPersistHash;
            float elapsed       = job.elapsedSec;
            float duration      = job.durationSec;
            int   actorId       = job.actorIdentityId;

            ctx.Write(transformHash);
            ctx.Write(processHash);
            ctx.Write(elapsed);
            ctx.Write(duration);
            ctx.Write(actorId);
        }
    }

    /**
     * Liest den Jobblock.
     *
     * Gibt IMMER true zurueck, sobald super true geliefert hat - und das ist
     * die wichtigste Zeile des Ladepfads: false aus OnStoreLoad laesst das
     * Item aus dem Spielstand verschwinden. Ein unlesbarer Job darf einen
     * Spieler nicht seine Station kosten; er kostet ihn den Job, mehr nicht
     * (02 §8: "jeder Fehler bewegt das System Richtung weniger ChefZ, nie
     * Richtung falsches ChefZ").
     */
    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        ClearJobs();

        int magic;
        if (!ctx.Read(magic))
        {
            // Kein Block da: die Station wurde von einer aelteren Core-Fassung
            // ohne Block geschrieben. Keine Jobs, weiter, kein Datenverlust.
            return true;
        }

        if (magic != MAGIC)
        {
            // Kontext NICHT weiterlesen. MAGIC richtet den Strom nicht aus -
            // es verhindert nur, dass ChefZ fremde Bytes als eigene deutet
            // (V-B §2 Folge 3).
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PROCESS,
                "station.magic." + GetType(),
                "\"" + GetType() + "\": im Spielstand steht an der Stelle des ChefZ-Jobblocks "
                + "kein ChefZ-Jobblock. Laufende Jobs gehen verloren, die Station bleibt "
                + "vollstaendig spielbar. Ursache ist fast immer ein anderer Mod, der von "
                + "dieser Klasse ableitet und vor ChefZ schreibt.");
            return true;
        }

        int blockVersion;
        if (!ctx.Read(blockVersion))
            return true;

        if (blockVersion > VERSION)
        {
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PROCESS,
                "station.version." + blockVersion.ToString(),
                "Der gespeicherte ChefZ-Jobblock hat Version " + blockVersion.ToString()
                + ", dieser Core kennt " + VERSION.ToString() + ". Der Rest des Blocks wird "
                + "uebersprungen; die Station startet ohne Jobs. Das passiert nach einem "
                + "Downgrade des Mods.");
            return true;
        }

        int count;
        if (!ctx.Read(count))
            return true;

        if (count < 0 || count > ChefZ_ProcessingLimits.MAX_PARALLEL_SLOTS)
        {
            ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.PROCESS,
                "station.slotcount." + GetType(),
                "Der ChefZ-Jobblock von \"" + GetType() + "\" nennt " + count.ToString()
                + " Slots; erlaubt sind 0 bis "
                + ChefZ_ProcessingLimits.MAX_PARALLEL_SLOTS.ToString()
                + ". Der Block gilt als leer, die Station bleibt erhalten.");
            return true;
        }

        for (int i = 0; i < count; i++)
        {
            int   transformHash;
            int   processHash;
            float elapsed;
            float duration;
            int   actorId;

            if (!ctx.Read(transformHash))   return ReadFailed("transformHash");
            if (!ctx.Read(processHash))     return ReadFailed("processHash");
            if (!ctx.Read(elapsed))         return ReadFailed("elapsedSec");
            if (!ctx.Read(duration))        return ReadFailed("durationSec");
            if (!ctx.Read(actorId))         return ReadFailed("actorIdentityId");

            ChefZ_ProcessJob job = new ChefZ_ProcessJob();
            job.transformPersistHash = transformHash;
            job.processPersistHash   = processHash;
            job.elapsedSec           = elapsed;
            job.durationSec          = duration;
            job.actorIdentityId      = actorId;
            m_ChefZ_Jobs.Insert(job);
        }

        return true;
    }

    //! Ein abgebrochener Lesevorgang kostet die Jobs, nicht die Station.
    protected bool ReadFailed(string field)
    {
        ClearJobs();
        ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.PROCESS,
            "station.readfail." + field,
            "Der ChefZ-Jobblock von \"" + GetType() + "\" bricht beim Feld \"" + field
            + "\" ab. Der Block gilt als leer, die Station bleibt erhalten. Das ist ein "
            + "Formatfehler im Spielstand, kein Datenfehler in der Konfiguration.");
        return true;
    }

    /**
     * Nach dem Laden: Hashes aufloesen, Slotzahl angleichen, Timer starten.
     *
     * 11 §7, letzte Zeile: "Serverneustart mit laufendem Job -> Job wird mit
     * elapsedSec wiederhergestellt und laeuft weiter. Existiert der Transform
     * nicht mehr: Abbruch OHNE VERLUST."
     */
    override void AfterStoreLoad()
    {
        super.AfterStoreLoad();

        ChefZ_LoadSupportedProcesses();
        EnsureSlots();

        if (!g_Game || !g_Game.IsServer())
            return;

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();

        for (int i = 0; i < m_ChefZ_Jobs.Count(); i++)
        {
            ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(i);
            if (!job.IsActive())
                continue;

            if (!ResolveJob(job, mgr))
            {
                ChefZ_CancelJob(i, "transform_gone");
                continue;
            }
        }

        if (ChefZ_ActiveJobCount() > 0)
            StartJobTimer();

        SyncFromJobs();
    }

    //==========================================================================
    // Slots und Timer
    //==========================================================================

    /**
     * Sorgt dafuer, dass es genau so viele Slots gibt, wie die Config sagt.
     *
     * WACHSEN ist harmlos. SCHRUMPFEN nicht: ein Slot, der wegfaellt, nimmt
     * einen laufenden Job mit. Deshalb schrumpft diese Methode nur BIS ZUM
     * LETZTEN AKTIVEN Job - eine Station, deren parallelSlots ein Betreiber
     * von 4 auf 1 senkt, verliert ihre laufenden Raeuchervorgaenge nicht.
     * Sobald sie fertig sind, greift die neue Grenze.
     */
    protected void EnsureSlots()
    {
        int want = m_ChefZ_SlotCount;
        if (want < 1)
            want = 1;
        if (want > ChefZ_ProcessingLimits.MAX_PARALLEL_SLOTS)
            want = ChefZ_ProcessingLimits.MAX_PARALLEL_SLOTS;

        while (m_ChefZ_Jobs.Count() < want)
            m_ChefZ_Jobs.Insert(new ChefZ_ProcessJob());

        while (m_ChefZ_Jobs.Count() > want)
        {
            int last = m_ChefZ_Jobs.Count() - 1;
            if (m_ChefZ_Jobs.Get(last).IsActive())
                break;                  // laufender Job haelt seinen Slot
            m_ChefZ_Jobs.Remove(last);
        }
    }

    protected void ClearJobs()
    {
        m_ChefZ_Jobs.Clear();
    }

    /**
     * Der eigene Tick (11 E7).
     *
     * Er laeuft NUR, solange mindestens ein Job aktiv ist. Eine Station ohne
     * Job kostet damit exakt null Rechenzeit - und auf einer Karte mit
     * hunderten aufgestellten Trockenrahmen ist das der Unterschied zwischen
     * "unmerklich" und "messbar".
     *
     * CALL_CATEGORY_SYSTEM und nicht GAMEPLAY: der Tick soll auch dann laufen,
     * wenn das Spiel pausiert ist - auf einem Server ist das ohnehin nie der
     * Fall, aber auf einem Listen-Server sonst schon, und ein Raeuchervorgang,
     * der beim Menueaufruf stehenbleibt, waere eine Ueberraschung.
     */
    protected void StartJobTimer()
    {
        if (!g_Game || !g_Game.IsServer())
            return;
        if (m_ChefZ_JobTimer && m_ChefZ_JobTimer.IsRunning())
            return;

        if (!m_ChefZ_JobTimer)
            m_ChefZ_JobTimer = new Timer(CALL_CATEGORY_SYSTEM);

        m_ChefZ_JobTimer.Run(TICK_INTERVAL_SEC, this, "ChefZ_OnJobTick", null, true);
    }

    /**
     * Anhalten - aber das Timerobjekt BEHALTEN.
     *
     * Der Aufruf kommt regelmaessig aus dem Tick heraus (ein Job wird fertig,
     * und danach ist keiner mehr aktiv). Das Objekt in genau diesem Moment
     * freizugeben hiesse, den gerade laufenden Rueckruf unter sich selbst
     * wegzuziehen. Ein wiederverwendetes, angehaltenes Timerobjekt kostet
     * dagegen nichts - StartJobTimer() nimmt es beim naechsten Job einfach
     * wieder in Betrieb.
     */
    protected void StopJobTimer()
    {
        if (!m_ChefZ_JobTimer)
            return;
        m_ChefZ_JobTimer.Stop();
    }

    //==========================================================================
    // Ereignisse (17 §4)
    //==========================================================================

    /**
     * ChefZ_OnProcessJobStarted - STORNIERBAR (17 §4).
     *
     * @return true = STORNIERT. Der Aufrufer bricht dann ab, und zwar VOR
     *         jeder Wirkung: es ist noch kein Slot belegt und nichts
     *         veraendert. Genau deshalb darf dieses Ereignis stornierbar sein
     *         und "Completed" nicht (17 E5).
     */
    protected bool RaiseJobStarted(notnull ChefZ_TransformMatch match, int actorId,
                                   out string cancelReason)
    {
        cancelReason = "";

        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (!bus.HasSubscribers(ChefZ_EventNames.PROCESS_JOB_STARTED))
            return false;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.PROCESS_JOB_STARTED);
        args.identityId        = actorId;
        args.recipeOrTransform = match.transformSym;
        args.deviceClass       = ChefZ_SymbolTable.Intern(GetType());

        int low = 0;
        int high = 0;
        GetNetworkID(low, high);
        args.SetDeviceNetId(low, high);

        // Ueber lokale Zwischenvariablen: einen out-Parameter als
        // out-Parameter weiterzureichen ist in Enforce nirgends zugesichert.
        string reason;
        string by;
        bool cancelled = bus.RaiseCancellable(args, reason, by);

        if (cancelled && by != "")
            reason = reason + " (storniert von " + by + ")";

        cancelReason = reason;
        return cancelled;
    }

    protected void RaiseJobCancelled(ChefZ_Sym transformSym, int actorId, string reasonTag)
    {
        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (!bus.HasSubscribers(ChefZ_EventNames.PROCESS_JOB_CANCELLED))
            return;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.PROCESS_JOB_CANCELLED);
        args.identityId        = actorId;
        args.recipeOrTransform = transformSym;
        args.deviceClass       = ChefZ_SymbolTable.Intern(GetType());

        // Der Grund gehoert in ein Feld, das es schon gibt, statt in ein neues
        // (17 E3). cancelReason heisst hier "warum es endete".
        args.cancelReason = reasonTag;

        int low = 0;
        int high = 0;
        GetNetworkID(low, high);
        args.SetDeviceNetId(low, high);

        bus.Raise(args);
    }

    //==========================================================================
    // Diagnose (18)
    //==========================================================================

    void ChefZ_DumpStation(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        ChefZ_LoadSupportedProcesses();

        outLines.Insert("Station " + GetType() + "  prozesse=[" + ChefZ_TextList.JoinSymbols(m_ChefZ_Processes, ",") + "]" + "  slots=" + m_ChefZ_Jobs.Count().ToString() + "  tempo=" + m_ChefZ_SpeedMultiplier.ToString() + "  waerme=" + ChefZ_HasHeat().ToString() + "  versorgt=" + ChefZ_IsPowered().ToString());

        for (int i = 0; i < m_ChefZ_Jobs.Count(); i++)
            outLines.Insert("  Slot " + i.ToString() + ": " + m_ChefZ_Jobs.Get(i).ToDebugString());
    }
}
