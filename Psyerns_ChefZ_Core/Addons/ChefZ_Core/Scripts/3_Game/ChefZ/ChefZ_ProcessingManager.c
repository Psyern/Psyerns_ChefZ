//==============================================================================
// ChefZ_ProcessingManager - Prozesse, Stationen, Transforms: Bestand und Suche
//
// Entwurf: 11 §4 (Schnittstelle woertlich), 11 §5 (BOOT- und Laufzeitfluss),
// 11 §6 (Zustand: nach dem Build unveraenderlich), 11 §7 (Fehlerverhalten),
// 11 E4 (derselbe Matcher wie beim Kochen), 11 E5 (Station -> Prozess),
// 09 §4.3 (Rangreihenfolge), 03 E2 (Persistenz ueber Name.Hash()).
//
// ---------------------------------------------------------------------------
// Die drei Saetze, die den Aufbau erklaeren
// ---------------------------------------------------------------------------
// 1. FindTransform() ist REIN LESEND und darf von ueberall gerufen werden -
//    aus ActionCondition (Client wie Server), aus dem Cookbook, aus dem
//    Validator (11 §4, woertlich: "Auch fuer ActionCondition und Cookbook
//    gefahrlos aufrufbar"). Sie veraendert weder Bestand noch Welt.
//
// 2. Der Index ist (Prozess, Station) -> Transformliste, sortiert nach
//    (Spezifitaet, priority, id). Er wird beim Boot EINMAL gebaut, und
//    danach wird zur Laufzeit nicht mehr sortiert - dieselbe Ueberlegung wie
//    in der ChefZ_RecipeEngine (09 §5).
//
// 3. Der erste Treffer gewinnt. Weil die Reihenfolge die Spezifitaetsordnung
//    IST, ist der erste Treffer auch der richtige (08 E3, hier fuer
//    Transforms).
//
// ---------------------------------------------------------------------------
// Warum der Index auf STATIONSKLASSEN ausgerollt wird
// ---------------------------------------------------------------------------
// Ein Transform bindet an einen PROZESS und optional an Stationen (11 E5).
// Ein Job laeuft aber immer an einer konkreten Station. Der Index rollt
// deshalb beim Boot einmal aus:
//
//     (Prozess, Stationsklasse) -> Transformindizes
//
// Damit kostet FindTransform() genau EINEN Map-Zugriff, bevor der erste
// Matcher-Knoten laeuft. Die Alternative - zur Laufzeit ueber alle Transforms
// eines Prozesses laufen und AllowsStation() fragen - waere bei jedem
// Zielwechsel des Fadenkreuzes eine Schleife ueber den halben Bestand.
//
// ---------------------------------------------------------------------------
// Was S14 ausdruecklich NICHT baut
// ---------------------------------------------------------------------------
// Die HANDCRAFT-Bruecke - ChefZ_GenericCraftRecipe und die
// "modded class PluginRecipesManagerBase" aus 11 §4 - gehoert S15 und steht
// hier bewusst nicht. 19 §3 trennt beide Schritte und begruendet es: die
// Bruecke ist der ZWEITE und letzte Vanilla-Override des Core und muss
// eigenstaendig gegen Vanilla-Crafting abgenommen werden.
//
// Alles, was sie braucht, ist trotzdem schon da: HANDCRAFT ist eine gueltige
// Ausfuehrungsform, die 2-Eingaenge-Grenze wird beim Build geprueft (11 §3),
// und GetProcessesForExec() liefert die Registrierungsliste. S15 fuegt eine
// Datei hinzu und aendert an dieser keine Zeile.
//
// KEIN CONTENT.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_ProcessingManager
{
    private static ref ChefZ_ProcessingManager s_Instance;

    //--- Bestand --------------------------------------------------------------
    private ref array<ref ChefZ_CompiledProcess>   m_Processes;
    private ref array<ref ChefZ_CompiledStation>   m_Stations;

    //! Nach dem Build in RANGREIHENFOLGE (09 §4.3). Der Arrayindex IST der
    //! Rang - genau wie in der ChefZ_RecipeEngine, und aus demselben Grund:
    //! eine zweite Ordnung neben der Speicherreihenfolge waere eine zweite
    //! Wahrheit.
    private ref array<ref ChefZ_CompiledTransform> m_Transforms;

    //--- Nachschlagerichtungen ------------------------------------------------
    private ref map<int, ref ChefZ_CompiledProcess> m_ProcessBySym;
    private ref map<int, ref ChefZ_CompiledStation> m_StationBySym;

    //! Persistenzschluessel (03 E2). Getrennt gefuehrt und nicht aus dem
    //! Symbol abgeleitet, weil ein Job den HASH speichert und ihn beim Laden
    //! zurueckaufloesen muss - dann gibt es noch kein Symbol.
    private ref map<int, int> m_TransformByHash;    // Hash -> Symbol
    private ref map<int, int> m_ProcessByHash;      // Hash -> Symbol

    /**
     * Der ausgerollte Index. Schluessel ist ein KOMBINIERTER Wert aus
     * Prozess- und Stationssymbol (siehe IndexKey), Wert die Liste der
     * Transformindizes in Rangreihenfolge.
     */
    private ref map<int, ref array<int>> m_ByProcessAndStation;

    //! Transforms eines Prozesses OHNE stationsAllowed. Sie gelten an jeder
    //! Station, die den Prozess anbietet (11 E5), und werden deshalb bei
    //! jeder Suche mitgemischt.
    private ref map<int, ref array<int>> m_ByProcessAnyStation;

    //--- Einstellungen und Zustand --------------------------------------------
    private int  m_NodeBudget;
    private bool m_Ready;
    private bool m_VerifyClasses;

    //--- Arbeitspuffer, je Auswertung geleert ---------------------------------
    private ref array<int>        m_Candidates;
    private ref ChefZ_BindResult  m_Bind;

    //--- Zaehler fuer "chefz stats" (18 §2) -----------------------------------
    private int m_RejectedProcesses;
    private int m_RejectedTransforms;

    void ChefZ_ProcessingManager()
    {
        m_Processes           = new array<ref ChefZ_CompiledProcess>();
        m_Stations            = new array<ref ChefZ_CompiledStation>();
        m_Transforms          = new array<ref ChefZ_CompiledTransform>();
        m_ProcessBySym        = new map<int, ref ChefZ_CompiledProcess>();
        m_StationBySym        = new map<int, ref ChefZ_CompiledStation>();
        m_TransformByHash     = new map<int, int>();
        m_ProcessByHash       = new map<int, int>();
        m_ByProcessAndStation = new map<int, ref array<int>>();
        m_ByProcessAnyStation = new map<int, ref array<int>>();
        m_Candidates          = new array<int>();
        m_Bind                = new ChefZ_BindResult();
        m_NodeBudget          = ChefZ_ProcessingLimits.DEFAULT_NODE_BUDGET;
        m_Ready               = false;
        m_VerifyClasses       = true;
        m_RejectedProcesses   = 0;
        m_RejectedTransforms  = 0;
    }

    static ChefZ_ProcessingManager Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_ProcessingManager();
        return s_Instance;
    }

    //! Nur fuer den Selbsttest: seine Transforms nennen absichtlich keine
    //! echten Klassen, weil der Core keine anlegen darf (Invariante I3).
    void SetVerifyClasses(bool on)
    {
        m_VerifyClasses = on;
    }

    //==========================================================================
    // BUILD (11 §5)
    //==========================================================================

    /**
     * Baut Bestand, Rangordnung und Index.
     *
     * Abweichung von der Signatur in 11 §4, und sie ist dieselbe wie bei
     * ChefZ_RecipeEngine.Build(): zum Kompilieren braucht es ausserdem den
     * Selektorkontext (07 §5) und die Einstellungen. Sie hier ueber einen
     * Singleton zu holen waere eine versteckte Abhaengigkeit statt einer
     * sichtbaren.
     *
     * JEDER Parameter darf null sein. Ein Aufruf mit lauter null ist die
     * ausdrueckliche Art zu sagen "Bestand leeren" - genau das braucht der
     * SAFE_MODE (02 §8). Danach ist der Manager "bereit und leer", nicht
     * "nicht gebaut": jede Abfrage antwortet ruhig mit 0 oder false, und die
     * Stationen in der Welt sind inerte Deko (11 §7, erste Zeile).
     *
     * Die Werkzeugregistry wird VORHER gebaut, nicht hier: sie ist die
     * Voraussetzung fuer die Prozesspruefung (11 §7), und sie gehoert dem
     * Config Manager, der ihre Records haelt.
     */
    void Build(ChefZ_Registry<ChefZ_ProcessDef> processes, ChefZ_Registry<ChefZ_StationDef> stations, ChefZ_Registry<ChefZ_TransformDef> transforms, ChefZ_ToolRegistry tools, ChefZ_CompileContext ctx, ChefZ_CoreSettingsDef settings, ChefZ_LoadReport report)
    {
        ClearAll();

        if (settings)
        {
            // Dasselbe Knotenbudget wie der Matcher beim Kochen: es ist eine
            // Eigenschaft des ZUORDNUNGSVERFAHRENS, nicht des Aufrufers, und
            // zwei getrennte Regler waeren zwei Gelegenheiten, einen zu
            // vergessen. Ein Transform hat ein bis zwei Eingaenge und wird das
            // Budget nie ausschoepfen.
            m_NodeBudget = settings.matcherNodeBudget;
            if (m_NodeBudget < 1)
                m_NodeBudget = ChefZ_ProcessingLimits.DEFAULT_NODE_BUDGET;
        }

        m_Ready = true;

        ChefZ_ProcessCompiler compiler = new ChefZ_ProcessCompiler();
        compiler.Init(ctx, report);
        compiler.SetVerifyClasses(m_VerifyClasses);

        CompileProcesses(processes, tools, compiler, report);
        CompileStations(stations, compiler, report);
        CompileTransforms(transforms, compiler, report);

        SortTransforms();
        BuildIndex(report);
        ReportSummary(report);
        LogIfDebug();
    }

    private void ClearAll()
    {
        m_Processes.Clear();
        m_Stations.Clear();
        m_Transforms.Clear();
        m_ProcessBySym.Clear();
        m_StationBySym.Clear();
        m_TransformByHash.Clear();
        m_ProcessByHash.Clear();
        m_ByProcessAndStation.Clear();
        m_ByProcessAnyStation.Clear();
        m_Candidates.Clear();
        m_Bind.Reset();
        m_RejectedProcesses  = 0;
        m_RejectedTransforms = 0;
        m_Ready              = false;
    }

    //--------------------------------------------------------------------------

    /**
     * Alle Prozesse uebersetzen, in stabiler Reihenfolge.
     *
     * Ueber Keys() und nicht ueber den Laufindex: Keys() ist nach ID sortiert
     * (03 §4), und damit ist die Reihenfolge der Meldungen im Ladebericht auf
     * jedem Server dieselbe.
     */
    private void CompileProcesses(ChefZ_Registry<ChefZ_ProcessDef> defs, ChefZ_ToolRegistry tools, notnull ChefZ_ProcessCompiler compiler, ChefZ_LoadReport report)
    {
        if (!defs || defs.Count() == 0)
            return;

        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_ProcessDef def = defs.Find(keys.Get(i));
            if (!def)
                continue;

            ChefZ_CompiledProcess proc = compiler.CompileProcess(def, tools);
            if (!proc)
            {
                m_RejectedProcesses++;
                continue;               // abgewiesen, Grund steht im Bericht
            }

            m_Processes.Insert(proc);
            m_ProcessBySym.Set(proc.processSym, proc);
            RegisterHash(m_ProcessByHash, proc.id, proc.processSym, def, report, "Prozess");
        }
    }

    private void CompileStations(ChefZ_Registry<ChefZ_StationDef> defs, notnull ChefZ_ProcessCompiler compiler, ChefZ_LoadReport report)
    {
        if (!defs || defs.Count() == 0)
            return;

        // Die Prozessmenge als reine Symbolmenge - der Compiler braucht nur
        // "kennst du das", nicht den ganzen Prozess.
        map<int, int> known = new map<int, int>();
        for (int p = 0; p < m_Processes.Count(); p++)
            known.Set(m_Processes.Get(p).processSym, 1);

        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_StationDef def = defs.Find(keys.Get(i));
            if (!def)
                continue;

            ChefZ_CompiledStation station = compiler.CompileStation(def, known);
            if (!station)
                continue;

            m_Stations.Insert(station);
            m_StationBySym.Set(station.stationSym, station);
        }
    }

    private void CompileTransforms(ChefZ_Registry<ChefZ_TransformDef> defs, notnull ChefZ_ProcessCompiler compiler, ChefZ_LoadReport report)
    {
        if (!defs || defs.Count() == 0)
            return;

        map<int, int> stationSyms = new map<int, int>();
        for (int s = 0; s < m_Stations.Count(); s++)
            stationSyms.Set(m_Stations.Get(s).stationSym, 1);

        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_TransformDef def = defs.Find(keys.Get(i));
            if (!def)
                continue;

            ChefZ_CompiledTransform tr = compiler.CompileTransform(def, m_ProcessBySym, stationSyms);
            if (!tr)
            {
                m_RejectedTransforms++;
                continue;
            }

            m_Transforms.Insert(tr);

            // Der Persistenzschluessel wird HIER vergeben und nicht erst beim
            // Sortieren: er haengt an der ID, nicht an der Position. Ein Job
            // im Spielstand muss auch dann aufloesbar sein, wenn ein spaeter
            // hinzugekommener Transform die Rangfolge verschoben hat (03 E2).
            RegisterHash(m_TransformByHash, tr.id, tr.transformSym, def, report, "Transform");
        }
    }

    /**
     * Persistenzschluessel eintragen (03 E2).
     *
     * Eine Hash-Kollision ist ein FEHLER, kein Sonderfall (03 E5) - dieselbe
     * Behandlung wie im ChefZ_StateManager: BEIDE Eintraege fallen aus der
     * Tabelle. Sie bleiben zur Laufzeit voll benutzbar; nur ein GESPEICHERTER
     * Job, der einen von beiden nennt, ist nach einem Neustart nicht mehr
     * aufloesbar und wird sauber abgebrochen. Das ist die kleinere
     * Zerstoerung - "erste gewinnt" verschoebe den Datenfehler in die Zukunft,
     * wo er als "mein Raeuchervorgang hat das Falsche erzeugt" auftaucht.
     */
    private void RegisterHash(notnull map<int, int> table, string id, ChefZ_Sym sym, notnull ChefZ_Record rec, ChefZ_LoadReport report, string what)
    {
        string key = id;
        int hash = key.Hash();

        int existing;
        if (table.Find(hash, existing))
        {
            table.Remove(hash);
            if (report)
            {
                report.AddError(rec.sourceRef, rec.id, what + ": Hash-Kollision mit \"" + ChefZ_SymbolTable.NameOrMark(existing) + "\" (beide Hash " + hash.ToString() + "). BEIDE sind ab sofort nicht mehr " + "aus einem Spielstand ruecklesbar; laufende Jobs, die einen von beiden " + "nennen, brechen nach einem Neustart OHNE VERLUST ab. Abhilfe: eine der " + "beiden IDs umbenennen.");
            }
            return;
        }

        table.Set(hash, sym);
    }

    //--------------------------------------------------------------------------
    // Rangordnung (09 §4.3)
    //--------------------------------------------------------------------------

    /**
     * Bestand nach (Spezifitaet absteigend, priority absteigend, id
     * aufsteigend) sortieren.
     *
     * Danach IST der Arrayindex der Rang, und jede Indexliste ist allein
     * dadurch sortiert, dass sie aufsteigend befuellt wird. Genau darauf
     * beruht "der erste Treffer gewinnt".
     *
     * Einfuegesortierung, weil sie stabil ist und die Bestaende hier
     * zweistellig sind. Determinismus schlaegt Laufzeit: sie laeuft einmal
     * beim Boot.
     */
    private void SortTransforms()
    {
        for (int i = 1; i < m_Transforms.Count(); i++)
        {
            ChefZ_CompiledTransform key = m_Transforms.Get(i);
            int j = i - 1;
            while (j >= 0 && Precedes(key, m_Transforms.Get(j)))
            {
                m_Transforms.Set(j + 1, m_Transforms.Get(j));
                j--;
            }
            m_Transforms.Set(j + 1, key);
        }
    }

    //! true, wenn a VOR b gehoert.
    private bool Precedes(notnull ChefZ_CompiledTransform a, notnull ChefZ_CompiledTransform b)
    {
        if (a.specificity != b.specificity)
            return a.specificity > b.specificity;
        if (a.priority != b.priority)
            return a.priority > b.priority;
        return ChefZ_StringOrder.Compare(a.id, b.id) < 0;
    }

    //--------------------------------------------------------------------------
    // Index
    //--------------------------------------------------------------------------

    /**
     * Rollt (Prozess, Station) aus und meldet Mehrdeutigkeiten.
     *
     * 11 §7: "Zwei Transforms matchen dasselbe Item -> spezifischerer gewinnt,
     * dann priority, dann id. Bei vollstaendiger Gleichheit WARN beim Build."
     * Genau diese Warnung faellt hier - beim Boot, im Ladebericht, mit beiden
     * IDs. Zur Laufzeit waere sie wertlos: dann hat bereits einer gewonnen.
     */
    private void BuildIndex(ChefZ_LoadReport report)
    {
        for (int i = 0; i < m_Transforms.Count(); i++)
        {
            ChefZ_CompiledTransform tr = m_Transforms.Get(i);

            if (tr.stationsAllowed.Count() == 0)
            {
                AddToIndex(m_ByProcessAnyStation, tr.processSym, i);
                continue;
            }

            for (int s = 0; s < tr.stationsAllowed.Count(); s++)
            {
                int key = IndexKey(tr.processSym, tr.stationsAllowed.Get(s));
                AddToIndex(m_ByProcessAndStation, key, i);
            }
        }

        ReportAmbiguities(report);
    }

    /**
     * Der kombinierte Indexschluessel aus zwei Symbolen.
     *
     * Symbole sind fortlaufende kleine Ganzzahlen (03 §2). Die Kombination
     * ueber eine Multiplikation mit einer grossen Primzahl plus XOR ist kein
     * kryptografischer Anspruch - sie muss nur streuen. Eine Kollision waere
     * hier auch nicht gefaehrlich, nur teurer: ein zusaetzlicher Kandidat, den
     * AllowsStation() unmittelbar danach wieder verwirft.
     */
    private int IndexKey(ChefZ_Sym process, ChefZ_Sym station)
    {
        return (process * 92821) ^ station;
    }

    private bool AddToIndex(notnull map<int, ref array<int>> index, int key, int value)
    {
        array<int> list;
        if (!index.Find(key, list))
        {
            list = new array<int>();
            index.Set(key, list);
        }
        if (list.Find(value) >= 0)
            return false;

        list.Insert(value);
        return true;
    }

    private void ReportAmbiguities(ChefZ_LoadReport report)
    {
        if (!report)
            return;

        for (int a = 0; a < m_Transforms.Count(); a++)
        {
            ChefZ_CompiledTransform ta = m_Transforms.Get(a);
            for (int b = a + 1; b < m_Transforms.Count(); b++)
            {
                ChefZ_CompiledTransform tb = m_Transforms.Get(b);

                if (ta.processSym != tb.processSym)
                    continue;
                if (ta.specificity != tb.specificity)
                    continue;
                if (ta.priority != tb.priority)
                    continue;

                report.AddWarn(tb.sourceRef, tb.id, "hat dieselbe Spezifitaet (" + ta.specificity.ToString() + ") und dieselbe priority wie \"" + ta.id + "\" am selben Prozess \"" + ChefZ_SymbolTable.NameOrMark(ta.processSym) + "\". Passen beide auf " + "dieselben Eingaben, entscheidet allein die ID - \"" + ta.id + "\" gewinnt. Wenn das nicht gewollt ist, hilft \"priority\".");
            }
        }
    }

    private void ReportSummary(ChefZ_LoadReport report)
    {
        if (!report)
            return;

        report.AddInfo("Processing Manager: " + m_Processes.Count().ToString() + " Prozesse, " + m_Stations.Count().ToString() + " Stationen, " + m_Transforms.Count().ToString() + " Transforms (" + m_ByProcessAnyStation.Count().ToString() + " Prozesse mit stationsfreien " + "Transforms, " + m_ByProcessAndStation.Count().ToString() + " exklusive " + "Zuordnungen). Abgewiesen: " + m_RejectedProcesses.ToString() + " Prozesse, " + m_RejectedTransforms.ToString() + " Transforms. Knotenbudget " + m_NodeBudget.ToString() + ".");
    }

    //==========================================================================
    // Auskuenfte (11 §4)
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    int GetProcessCount()   { return m_Processes.Count(); }
    int GetStationCount()   { return m_Stations.Count(); }
    int GetTransformCount() { return m_Transforms.Count(); }
    int GetNodeBudget()     { return m_NodeBudget; }

    ChefZ_CompiledProcess GetProcess(ChefZ_Sym process)
    {
        ChefZ_CompiledProcess proc;
        if (!m_ProcessBySym.Find(process, proc))
            return null;
        return proc;
    }

    ChefZ_CompiledStation GetStation(ChefZ_Sym stationClass)
    {
        ChefZ_CompiledStation station;
        if (!m_StationBySym.Find(stationClass, station))
            return null;
        return station;
    }

    ChefZ_CompiledTransform GetTransform(ChefZ_Sym transformSym)
    {
        for (int i = 0; i < m_Transforms.Count(); i++)
        {
            if (m_Transforms.Get(i).transformSym == transformSym)
                return m_Transforms.Get(i);
        }
        return null;
    }

    ChefZ_CompiledTransform GetTransformAt(int index)
    {
        if (index < 0 || index >= m_Transforms.Count())
            return null;
        return m_Transforms.Get(index);
    }

    /**
     * Welche Prozesse bietet diese Stationsklasse an (11 §4)?
     *
     * Rueckgabe ist die ANZAHL, damit ein Aufrufer ohne Liste auskommt, wenn
     * ihn nur "ueberhaupt einer?" interessiert.
     */
    int GetOfferedProcesses(ChefZ_Sym stationClass, out array<ChefZ_Sym> outProcesses)
    {
        if (!outProcesses)
            outProcesses = new array<ChefZ_Sym>();
        outProcesses.Clear();

        ChefZ_CompiledStation station = GetStation(stationClass);
        if (!station)
            return 0;

        for (int i = 0; i < station.processes.Count(); i++)
            outProcesses.Insert(station.processes.Get(i));

        return outProcesses.Count();
    }

    /**
     * Alle Prozesse EINER Ausfuehrungsform (11 §4). Die Handcraft-Bruecke
     * (S15) holt sich damit ihre Registrierungsliste, ohne den Bestand zu
     * kennen.
     *
     * ABWEICHUNG von 11 §4: dort steht
     * GetProcessesForExec(string exec, out array<ChefZ_ProcessDef>).
     *
     *   - int statt string, weil der Aufrufer ohnehin gegen
     *     ChefZ_ProcessExec.* vergleicht. Ein String waere hier ein
     *     Namensvergleich je Prozess in einer Schleife, deren einziger Zweck
     *     ein Vergleich ist - und ein Tippfehler faende stumm nichts.
     *   - ChefZ_CompiledProcess statt ChefZ_ProcessDef, weil die Rohform
     *     nach dem Build niemandem mehr gehoert: sie liegt in der Registry
     *     des Config Managers, ihre Werte sind ungeklemmt, und ihre
     *     Werkzeuggruppen sind unaufgeloest. S15 braucht die geprueften Werte.
     */
    void GetProcessesForExec(int exec, out array<ChefZ_CompiledProcess> outProcesses)
    {
        if (!outProcesses)
            outProcesses = new array<ChefZ_CompiledProcess>();
        outProcesses.Clear();

        for (int i = 0; i < m_Processes.Count(); i++)
        {
            ChefZ_CompiledProcess proc = m_Processes.Get(i);
            if (proc.exec == exec)
                outProcesses.Insert(proc);
        }
    }

    //! Alle Transforms EINES Prozesses, in Rangreihenfolge. Fuer S15 und die
    //! Diagnose - nicht fuer den heissen Pfad.
    void GetTransformsForProcess(ChefZ_Sym process, out array<ChefZ_CompiledTransform> outList)
    {
        if (!outList)
            outList = new array<ChefZ_CompiledTransform>();
        outList.Clear();

        for (int i = 0; i < m_Transforms.Count(); i++)
        {
            ChefZ_CompiledTransform tr = m_Transforms.Get(i);
            if (tr.processSym == process)
                outList.Insert(tr);
        }
    }

    /**
     * Gibt es fuer diesen Prozess ueberhaupt einen Transform?
     *
     * Der billige Vorfilter. Er beantwortet zugleich die Frage, ob dieser
     * SEITE (Client oder Server) Transformdaten vorliegen - und genau darauf
     * stuetzt sich ChefZ_ActionProcessAtStation, siehe dort.
     */
    bool HasAnyTransformFor(ChefZ_Sym process)
    {
        if (!m_Ready)
            return false;
        if (m_ByProcessAnyStation.Contains(process))
            return true;

        // Ein Transform mit stationsAllowed steht nur unter dem kombinierten
        // Schluessel. Die Liste ist kurz und diese Frage selten - eine dritte
        // Indexrichtung nur dafuer waere eine dritte Stelle zum Pflegen.
        for (int i = 0; i < m_Transforms.Count(); i++)
        {
            if (m_Transforms.Get(i).processSym == process)
                return true;
        }
        return false;
    }

    /**
     * Fuehrt der Handelnde ein passendes Werkzeug (11 §4)?
     *
     * Rein lesend und ohne Bestandzugriff ausser dem Prozess selbst - damit
     * ist sie aus ActionCondition heraus billig genug.
     *
     * ABWEICHUNG von 11 §4: dort steht
     * CheckTools(process, notnull array<ChefZ_Sym> availableToolGroups, ...).
     * Der ChefZ_ProcessContext TRAEGT diese Liste bereits (11 §4,
     * availableToolGroups) - sie noch einmal einzeln zu uebergeben hiesse,
     * denselben Wert an zwei Stellen zu fuehren und dem Aufrufer die
     * Gelegenheit zu geben, zwei verschiedene zu benutzen.
     */
    bool CheckTools(ChefZ_Sym process, notnull ChefZ_ProcessContext ctx, out string missingGroup)
    {
        missingGroup = "";

        ChefZ_CompiledProcess proc = GetProcess(process);
        if (!proc)
            return false;

        return proc.HasTools(ctx, missingGroup);
    }

    //==========================================================================
    // Persistenz (03 E2, 11 §6)
    //==========================================================================

    //! Persistenzschluessel eines Transforms. 0 = unbekannt.
    int GetTransformPersistHash(ChefZ_Sym transformSym)
    {
        // Zwischenvariable und kein Aufruf auf dem Rueckgabewert: Enforce
        // sichert Methodenaufrufe auf temporaeren Strings nicht zu.
        string name = ChefZ_SymbolTable.Name(transformSym);
        if (name == "")
            return ChefZ_ProcessJob.NO_HASH;
        return name.Hash();
    }

    int GetProcessPersistHash(ChefZ_Sym processSym)
    {
        string name = ChefZ_SymbolTable.Name(processSym);
        if (name == "")
            return ChefZ_ProcessJob.NO_HASH;
        return name.Hash();
    }

    //! INVALID, wenn es den Transform nicht mehr gibt. Der Aufrufer bricht
    //! den Job dann ab - OHNE Verlust (11 §6).
    ChefZ_Sym TransformFromPersistHash(int hash)
    {
        if (hash == ChefZ_ProcessJob.NO_HASH)
            return ChefZ_SymbolTable.INVALID;

        int sym;
        if (!m_TransformByHash.Find(hash, sym))
            return ChefZ_SymbolTable.INVALID;
        return sym;
    }

    ChefZ_Sym ProcessFromPersistHash(int hash)
    {
        if (hash == ChefZ_ProcessJob.NO_HASH)
            return ChefZ_SymbolTable.INVALID;

        int sym;
        if (!m_ProcessByHash.Find(hash, sym))
            return ChefZ_SymbolTable.INVALID;
        return sym;
    }

    //==========================================================================
    // FindTransform (11 §4, §5) - REIN LESEND
    //==========================================================================

    /**
     * Der passende Transform fuer diesen Prozess, diese Station und diesen
     * Inhalt. Erster Treffer gewinnt (siehe Kopf).
     *
     * REIN LESEND. Sie veraendert weder Bestand noch Welt und darf deshalb aus
     * ActionCondition gerufen werden - auf dem Client wie auf dem Server
     * (11 §4, woertlich).
     *
     * @param snapshot  MUSS stabil sortiert sein (ChefZ_FactSnapshot.
     *        SortStable); der ChefZ_FactCollector tut das beim Erheben. Der
     *        Matcher setzt und raeumt slotBoundTo darin - das ist der einzige
     *        Seiteneffekt, und er ist derselbe wie beim Kochen (07 §6).
     * @param match     wird IMMER gefuellt. Bei false traegt failReason den
     *        Grund im Klartext.
     *
     * @return true nur, wenn ein Transform gebunden werden konnte. Kein
     *         Treffer ist der HAEUFIGSTE Ausgang und ausdruecklich kein
     *         Fehler - die Aktion erscheint dann schlicht nicht (11 §7).
     */
    bool FindTransform(ChefZ_Sym process, notnull ChefZ_ProcessContext ctx, notnull ChefZ_FactSnapshot inputs, ChefZ_MatchTrace trace, out ChefZ_TransformMatch match)
    {
        if (!match)
            match = new ChefZ_TransformMatch();
        match.Reset();
        match.processSym = process;

        if (!m_Ready || m_Transforms.Count() == 0)
        {
            match.failReason = "keine Transformdaten geladen";
            return false;
        }

        ChefZ_CompiledProcess proc = GetProcess(process);
        if (!proc)
        {
            match.failReason = "Prozess " + ChefZ_SymbolTable.NameOrMark(process) + " ist nicht geladen";
            return false;
        }

        // Umgebung und Werkzeug ZUERST: beide kosten ein paar Vergleiche und
        // ersparen im Fehlfall den gesamten Kandidatenlauf. Und beide liefern
        // die nuetzlichere Begruendung - "die Station ist kalt" hilft einem
        // Spieler, "kein Transform passt" nicht.
        string envReason;
        if (!proc.MeetsEnvironment(ctx, envReason))
        {
            match.failReason = envReason;
            return false;
        }

        string missingTool;
        if (!proc.HasTools(ctx, missingTool))
        {
            match.failReason = "Werkzeuggruppe " + missingTool + " fehlt";
            return false;
        }

        CollectCandidates(process, ctx.stationClass, m_Candidates);
        if (m_Candidates.Count() == 0)
        {
            match.failReason = "kein Transform fuer diesen Prozess an dieser Station";
            return false;
        }

        int itemCount = inputs.Count();
        int tried     = 0;
        int nodes     = 0;

        string firstFail = "";

        for (int i = 0; i < m_Candidates.Count(); i++)
        {
            ChefZ_CompiledTransform tr = m_Transforms.Get(m_Candidates.Get(i));

            if (tr.minItemCount > itemCount)
                continue;               // Vorfilter, kostet keinen Matcher-Knoten

            tried++;

            // Ueber eine lokale Zwischenvariable und NICHT direkt mit dem
            // Feld: ein Feld als out-Parameter ist in Enforce nicht
            // zugesichert (siehe Kopf von ChefZ_TextList.SymbolsOf). Der
            // Puffer bleibt derselbe - Bind() legt nur dann ein neues Objekt
            // an, wenn keines uebergeben wurde.
            ChefZ_BindResult bind = m_Bind;

            if (!ChefZ_Matcher.Bind(tr.inputs, inputs, m_NodeBudget, tr.id, trace, bind))
            {
                nodes = nodes + m_Bind.nodesExplored;
                if (firstFail == "")
                    firstFail = tr.id + ": " + m_Bind.failReason;
                continue;
            }

            nodes = nodes + m_Bind.nodesExplored;

            // 17 §3.3 / 08 §7 Schritt 2c: "block" filtert den Kandidaten. Erst
            // NACH der Bindung, weil die Faehigkeitsabfrage einen Provider
            // aufrufen kann und die Bindung nicht - der billige Test zuerst.
            string capReason;
            if (ChefZ_CapabilityGate.Denies(tr.requires, ctx.actorIdentityId, capReason))
            {
                if (firstFail == "")
                    firstFail = tr.id + ": " + capReason;
                continue;
            }

            Fill(tr, proc, match);
            match.candidatesTried = tried;
            match.nodesExplored   = nodes;
            return true;
        }

        match.candidatesTried = tried;
        match.nodesExplored   = nodes;
        if (firstFail != "")
            match.failReason = firstFail;
        else
            match.failReason = "kein Transform passt auf den Inhalt dieser Station";
        return false;
    }

    /**
     * Kandidaten = stationsfreie Transforms des Prozesses PLUS die, die diese
     * Station ausdruecklich nennen.
     *
     * Beide Teillisten sind aufsteigend nach Transformindex, und weil der
     * Index die Rangreihenfolge IST, ist ihre Mischung in Rangreihenfolge. Es
     * wird NICHT sortiert.
     */
    private void CollectCandidates(ChefZ_Sym process, ChefZ_Sym stationClass, notnull array<int> outIdx)
    {
        outIdx.Clear();

        array<int> anyStation;
        if (m_ByProcessAnyStation.Find(process, anyStation))
            MergeBucket(anyStation, outIdx);

        array<int> exclusive;
        if (m_ByProcessAndStation.Find(IndexKey(process, stationClass), exclusive))
            MergeBucket(exclusive, outIdx);

        // Der kombinierte Schluessel kann streuungsbedingt kollidieren (siehe
        // IndexKey). Die zweite Pruefung ist billig und macht eine Kollision
        // folgenlos statt falsch.
        for (int k = outIdx.Count() - 1; k >= 0; k--)
        {
            ChefZ_CompiledTransform tr = m_Transforms.Get(outIdx.Get(k));
            if (tr.processSym != process || !tr.AllowsStation(stationClass))
                outIdx.RemoveOrdered(k);
        }
    }

    /**
     * Aufsteigende Liste in eine aufsteigende Liste mischen, ohne Duplikate.
     *
     * RemoveOrdered und InsertAt statt Remove und Insert: Enforce-Remove()
     * fuellt die Luecke mit dem LETZTEN Element und zerstoert damit genau die
     * Ordnung, um derentwillen dieser Index existiert (EnScript.c:462).
     */
    private void MergeBucket(notnull array<int> src, notnull array<int> dst)
    {
        for (int i = 0; i < src.Count(); i++)
        {
            int value = src.Get(i);
            int at = dst.Count();
            while (at > 0 && dst.Get(at - 1) > value)
                at--;
            if (at > 0 && dst.Get(at - 1) == value)
                continue;

            if (at >= dst.Count())
                dst.Insert(value);
            else
                dst.InsertAt(value, at);
        }
    }

    //! Das Bindungsergebnis in die Ansage uebertragen.
    private void Fill(notnull ChefZ_CompiledTransform tr, notnull ChefZ_CompiledProcess proc, notnull ChefZ_TransformMatch match)
    {
        match.matched      = true;
        match.transformSym = tr.transformSym;
        match.transformId  = tr.id;
        match.processSym   = tr.processSym;

        int i;
        for (i = 0; i < m_Bind.bindings.Count(); i++)
        {
            ChefZ_SlotBinding binding = m_Bind.bindings.Get(i);
            if (binding && binding.filled)
                match.SetAssignment(binding.slotId, binding.handles);
        }

        for (i = 0; i < m_Bind.boundHandles.Count(); i++)
            match.boundHandles.Insert(m_Bind.boundHandles.Get(i));

        for (i = 0; i < m_Bind.consumePlan.Count(); i++)
            match.consumePlan.Insert(m_Bind.consumePlan.Get(i));

        match.durationSec = BaseDuration(tr, proc);
    }

    //==========================================================================
    // Dauer (11 §4)
    //==========================================================================

    /**
     * Wie lange dauert dieser Transform an dieser Station (11 §4)?
     *
     *     Grunddauer   Transform-Override, sonst Prozessdauer
     *   / speedMultiplier der Station
     *   / Faehigkeitsfaktor
     *
     * GETEILT und nicht multipliziert: ein speedMultiplier von 2.0 heisst
     * "doppelt so schnell", nicht "doppelt so lang". Die andere Lesart waere
     * fuer einen Content-Autor die Ueberraschung, die er erst nach dem
     * Servertest bemerkt.
     *
     * Der Faehigkeitsfaktor ist bewusst NICHT hier - er kommt aus der
     * ChefZ_CapabilityRegistry und wird von der Station beim Start des Jobs
     * angewandt (11 §5, "capabilityFactor"). Hier steht die Zahl, die ohne
     * jeden Spieler gilt; sonst haenge die angezeigte Dauer davon ab, wer
     * gerade hinschaut.
     */
    float GetDuration(notnull ChefZ_TransformMatch match, notnull ChefZ_ProcessContext ctx)
    {
        ChefZ_CompiledTransform tr = GetTransform(match.transformSym);
        ChefZ_CompiledProcess   proc = GetProcess(match.processSym);

        float seconds;
        if (tr && proc)
            seconds = BaseDuration(tr, proc);
        else
            seconds = match.durationSec;

        ChefZ_CompiledStation station = GetStation(ctx.stationClass);
        if (station && station.speedMultiplier > 0.0)
            seconds = seconds / station.speedMultiplier;

        if (seconds < 0.0)
            return 0.0;
        return seconds;
    }

    private float BaseDuration(notnull ChefZ_CompiledTransform tr, notnull ChefZ_CompiledProcess proc)
    {
        if (tr.HasDurationOverride())
            return tr.durationOverrideSec;
        return proc.baseDurationSec;
    }

    //==========================================================================
    // Diagnose (18)
    //==========================================================================

    void DumpProcessing(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("ChefZ Processing Manager  bereit=" + m_Ready.ToString() + "  prozesse=" + m_Processes.Count().ToString() + "  stationen=" + m_Stations.Count().ToString() + "  transforms=" + m_Transforms.Count().ToString() + "  budget=" + m_NodeBudget.ToString());

        int i;
        for (i = 0; i < m_Processes.Count(); i++)
            outLines.Insert("  P " + m_Processes.Get(i).ToDebugString());
        for (i = 0; i < m_Stations.Count(); i++)
            outLines.Insert("  S " + m_Stations.Get(i).ToDebugString());
        for (i = 0; i < m_Transforms.Count(); i++)
            outLines.Insert("  T " + (i + 1).ToString() + ". " + m_Transforms.Get(i).ToDebugString());
    }

    private void LogIfDebug()
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        DumpProcessing(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.PROCESS, lines);
    }
}
