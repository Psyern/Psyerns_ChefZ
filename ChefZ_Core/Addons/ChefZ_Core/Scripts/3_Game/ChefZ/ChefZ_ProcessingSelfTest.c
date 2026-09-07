//==============================================================================
// ChefZ_ProcessingSelfTest - Abnahmepruefung fuer S14
//
// Entwurf: 19 §3, S14 - "Fertig, wenn: Eine Teststation bietet genau die
// Prozesse an, die ihre CfgChefZStations-Deklaration nennt; ein
// STATION_TIMED-Job ueberlebt einen Serverneustart; ein Job ohne Waermequelle
// PAUSIERT und laeuft nicht zurueck; ein entferntes Eingangsitem bricht den
// Job OHNE VERLUST ab."
//
// ---------------------------------------------------------------------------
// Was hier geprueft wird - und was ausdruecklich nicht
// ---------------------------------------------------------------------------
// PRUEFBAR OHNE WELT, und deshalb hier:
//
//   - das Datenmodell (Defaults, Sentinel, Bool-Sonde)
//   - die Werkzeugaufloesung, beide Schreibweisen und die Vererbung
//   - der Compiler: alle Abweisungsgruende aus 11 §7
//   - der Index und die Rangreihenfolge
//   - FindTransform samt Umgebungs- und Werkzeugsperre
//   - die FORTSCHRITTSARITHMETIK: pausiert statt rueckwaerts (11 §7)
//
// NICHT PRUEFBAR OHNE WELT, und deshalb dem Servertest vorbehalten:
//
//   - dass ein Job einen Serverneustart ueberlebt. Ein nachgebauter
//     ParamsWriteContext prueft den Nachbau, nicht die Engine - dieselbe
//     Begruendung, mit der der S9-Selbsttest die Persistenz auslaesst.
//     Geprueft wird hier stattdessen das, WORAUF die Wiederherstellung
//     beruht: dass ein Job ueber den HASH aufloesbar ist und nicht ueber den
//     Symbolzaehler (03 E2).
//   - dass ein entferntes Eingangsitem den Job ohne Verlust abbricht. Der
//     Abbruchpfad selbst braucht ein Inventar. Geprueft wird hier die
//     Bedingung, die ihn ausloest: FindTransform liefert auf einem leeren
//     Faktensatz false, und ein false bedeutet an dieser Stelle "nichts
//     veraendern".
//
// KEIN CONTENT: alle Namen tragen das Praefix "CHEFZ_PT_" und sind abstrakt.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_ProcessingSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    //--- Bitindizes des Testkategoriebaums -----------------------------------
    static const int BIT_A = 0;
    static const int BIT_B = 1;

    static const string KAT_A = "CHEFZ_PT_KAT_A";
    static const string KAT_B = "CHEFZ_PT_KAT_B";

    static const string GRUPPE_SCHNEID = "CHEFZ_PT_GRUPPE_SCHNEID";
    static const string GRUPPE_LEER    = "CHEFZ_PT_GRUPPE_LEER";
    static const string WERKZEUG       = "CHEFZ_PT_WERKZEUG";
    static const string WERKZEUG_ERBE  = "CHEFZ_PT_WERKZEUG_ERBE";

    static const string PROZ_ACTION = "CHEFZ_PT_PROZ_ACTION";
    static const string PROZ_TIMED  = "CHEFZ_PT_PROZ_TIMED";
    static const string PROZ_HAND   = "CHEFZ_PT_PROZ_HAND";

    static const string STATION_A = "CHEFZ_PT_STATION_A";
    static const string STATION_B = "CHEFZ_PT_STATION_B";

    static const string ERGEBNIS = "CHEFZ_PT_ERGEBNIS";
    static const string ZUSTAND  = "CHEFZ_PT_ZUSTAND";

    //==========================================================================

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("Prozessmodell",  ChefZ_ProcessDef.SelfCheck());
        Check("Stationsmodell", ChefZ_StationDef.SelfCheck());
        Check("Transformmodell", ChefZ_TransformDef.SelfCheck());
        Check("Werkzeugmodell", ChefZ_ToolGroupDef.SelfCheck());
        Check("Prozesskontext", ChefZ_ProcessContext.SelfCheck());
        Check("Transformtreffer", ChefZ_TransformMatch.SelfCheck());
        Check("Jobarithmetik",  ChefZ_ProcessJob.SelfCheck());

        Check("Werkzeugregistry", TestToolRegistry());
        Check("Compiler",         TestCompilerRejections());
        Check("Stationsangebot",  TestStationOffers());
        Check("Rangfolge",        TestTransformOrder());
        Check("Suche",           TestFindTransform());
        Check("Umgebung",        TestEnvironment());
        Check("Persistenz",      TestPersistHashes());
        Check("Leerbestand",     TestEmptyBuild());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS, "Selbsttest S14 " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.PROCESS, "Selbsttest S14 " + name + " FEHLGESCHLAGEN. Der Processing Manager verhaelt sich " + "nicht wie entworfen - was eine Station aus einer Zutat macht, ist damit " + "unzuverlaessig.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S14: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
        if (s_Failed > 0 && s_FailedNames)
        {
            s = s + "  gescheitert:";
            for (int i = 0; i < s_FailedNames.Count(); i++)
                s = s + " " + s_FailedNames.Get(i);
        }
        return s;
    }

    //==========================================================================
    // Testumgebung
    //==========================================================================

    private static ChefZ_CompileContext MakeContext(ChefZ_LoadReport report)
    {
        ChefZ_CompileContext ctx = new ChefZ_CompileContext();
        ctx.Init(report);
        ctx.SetSubject("selftest", "CHEFZ_PT");

        ChefZ_SymbolResolver res = new ChefZ_SymbolResolver();
        res.DefineCategory(KAT_A, BIT_A, 0, 2);
        res.DefineCategory(KAT_B, BIT_B, 1, 1);
        ctx.SetResolver(res);
        ctx.SetWeights(new ChefZ_PriorityWeights());

        return ctx;
    }

    //--------------------------------------------------------------------------
    // Bausteine
    //--------------------------------------------------------------------------

    private static ChefZ_ToolGroupDef ToolGroup(string id, string memberClass, bool subclasses)
    {
        ChefZ_ToolGroupDef def = new ChefZ_ToolGroupDef();
        def.id      = id;
        def.classes = new array<string>();
        if (memberClass != "")
            def.classes.Insert(memberClass);
        def.allowSubclasses = subclasses;
        def.MarkExplicit("allowSubclasses");
        return def;
    }

    private static ChefZ_ProcessDef Process(string id, string exec, string toolGroup)
    {
        ChefZ_ProcessDef def = new ChefZ_ProcessDef();
        def.id              = id;
        def.exec            = exec;
        def.baseDurationSec = 10.0;

        if (toolGroup != "")
        {
            def.toolGroups = new array<string>();
            def.toolGroups.Insert(toolGroup);
        }

        def.Normalize();
        def.ResolveDefaults();
        return def;
    }

    private static ChefZ_StationDef Station(string id, string processA, string processB)
    {
        ChefZ_StationDef def = new ChefZ_StationDef();
        def.id        = id;
        def.processes = new array<string>();
        if (processA != "")
            def.processes.Insert(processA);
        if (processB != "")
            def.processes.Insert(processB);

        def.Normalize();
        def.ResolveDefaults();
        return def;
    }

    private static ChefZ_SlotDef CategorySlot(string slotId, string category, string consume)
    {
        ChefZ_Selector sel = new ChefZ_Selector();
        sel.category = category;

        ChefZ_SlotDef slot = new ChefZ_SlotDef();
        slot.slotId  = slotId;
        slot.match   = sel;
        slot.consume = consume;
        return slot;
    }

    private static ChefZ_TransformDef Transform(string id, string process, string category)
    {
        ChefZ_TransformDef def = new ChefZ_TransformDef();
        def.id      = id;
        def.process = process;

        def.inputs = new array<ref ChefZ_SlotDef>();
        def.inputs.Insert(CategorySlot("in", category, ChefZ_ConsumeMode.WHOLE_NAME));

        def.outputs = new array<ref ChefZ_OutputDef>();
        ChefZ_OutputDef outDef = new ChefZ_OutputDef();
        outDef.cls = ERGEBNIS;
        def.outputs.Insert(outDef);

        def.Normalize();
        def.ResolveDefaults();
        return def;
    }

    private static ChefZ_ItemFacts AddItem(notnull ChefZ_FactSnapshot snap, int handle, string className, int bit)
    {
        ChefZ_ItemFacts facts = snap.Acquire();
        facts.handle      = handle;
        facts.classSym    = ChefZ_SymbolTable.Intern(className);
        facts.quantity    = 1.0;
        facts.quantityMax = 1.0;
        facts.units       = 1.0;
        facts.health01    = 1.0;
        facts.freshness01 = 1.0;
        if (bit >= 0)
            facts.closure.SetBit(bit);
        return facts;
    }

    /**
     * Eine gefuellte Werkzeugregistry auf dem SINGLETON.
     *
     * Anders als bei den uebrigen Selbsttests laeuft dieser auf der echten
     * Instanz: die ChefZ_ToolRegistry ist keine Instanz, die man dem
     * ChefZ_ProcessCompiler beliebig unterschieben koennte - der Compiler
     * bekommt sie zwar als Parameter, aber der ChefZ_FactCollector und die
     * Action holen sie ueber Get(). Ein Test, der eine ZWEITE Instanz baut,
     * pruefte etwas anderes als das, was im Betrieb laeuft.
     *
     * Der Bestand wird am Ende von Run() nicht zurueckgesetzt - das erledigt
     * der Config Manager beim naechsten Build(), und der laeuft unmittelbar
     * nach dem Selbsttest (ChefZ_Boot.OnMissionStart).
     */
    private static void FillToolRegistry()
    {
        ChefZ_Registry<ChefZ_ToolGroupDef> tools = new ChefZ_Registry<ChefZ_ToolGroupDef>();
        tools.Init(ChefZ_RecordKind.TOOL_GROUP);

        ChefZ_ToolGroupDef schneid = ToolGroup(GRUPPE_SCHNEID, WERKZEUG, false);
        schneid.sym = ChefZ_SymbolTable.Intern(schneid.id);
        tools.Add(schneid);

        ChefZ_ToolGroupDef leer = ToolGroup(GRUPPE_LEER, "", false);
        leer.sym = ChefZ_SymbolTable.Intern(leer.id);
        tools.Add(leer);

        ChefZ_ToolRegistry.Get().Build(tools, null);
    }

    //==========================================================================
    // 1. Werkzeugregistry (11 E8)
    //==========================================================================

    private static bool TestToolRegistry()
    {
        ChefZ_Registry<ChefZ_ToolGroupDef> defs = new ChefZ_Registry<ChefZ_ToolGroupDef>();
        defs.Init(ChefZ_RecordKind.TOOL_GROUP);

        // Gruppenweise: id = Gruppe, classes[] = Mitglieder (02 §5.1).
        ChefZ_ToolGroupDef gruppe = ToolGroup(GRUPPE_SCHNEID, WERKZEUG, false);
        gruppe.sym = ChefZ_SymbolTable.Intern(gruppe.id);
        defs.Add(gruppe);

        // Klassenweise: id = Klasse, toolCategories[] = Gruppen (02 §4).
        ChefZ_ToolGroupDef klasse = new ChefZ_ToolGroupDef();
        klasse.id             = WERKZEUG_ERBE;
        klasse.toolCategories = new array<string>();
        klasse.toolCategories.Insert(GRUPPE_LEER);
        klasse.sym = ChefZ_SymbolTable.Intern(klasse.id);
        defs.Add(klasse);

        ChefZ_ToolRegistry reg = new ChefZ_ToolRegistry();
        reg.Build(defs, null);

        if (!reg.IsReady())                                                     return false;
        if (reg.GetGroupCount() != 2)                                           return false;

        ChefZ_Sym schneid = ChefZ_SymbolTable.Intern(GRUPPE_SCHNEID);
        ChefZ_Sym leer    = ChefZ_SymbolTable.Intern(GRUPPE_LEER);
        ChefZ_Sym tool    = ChefZ_SymbolTable.Intern(WERKZEUG);
        ChefZ_Sym erbe    = ChefZ_SymbolTable.Intern(WERKZEUG_ERBE);

        // Beide Schreibweisen landen im SELBEN Index.
        if (!reg.IsToolOfGroup(tool, schneid))                                  return false;
        if (!reg.IsToolOfGroup(erbe, leer))                                     return false;
        if (reg.IsToolOfGroup(tool, leer))                                      return false;

        if (!reg.HasGroup(schneid))                                             return false;
        if (reg.HasGroup(ChefZ_SymbolTable.Intern("CHEFZ_PT_GIBTSNICHT")))      return false;

        array<ChefZ_Sym> groups = new array<ChefZ_Sym>();
        reg.GetGroupsForClass(tool, groups);
        if (groups.Count() != 1)                                                return false;
        if (groups.Get(0) != schneid)                                           return false;

        // Eine voellig unbekannte Klasse fuehrt kein Werkzeug - der
        // haeufigste Fall im Betrieb, und er muss ruhig false liefern.
        if (reg.IsAnyTool(ChefZ_SymbolTable.Intern("CHEFZ_PT_VANILLAITEM")))    return false;

        // Leerer Bestand: bereit und leer, nicht "nicht gebaut".
        ChefZ_ToolRegistry empty = new ChefZ_ToolRegistry();
        empty.Build(null, null);
        if (!empty.IsReady())                                                   return false;
        if (empty.GetGroupCount() != 0)                                         return false;
        if (empty.IsToolOfGroup(tool, schneid))                                 return false;

        return true;
    }

    //==========================================================================
    // 2. Die Abweisungsgruende aus 11 §7
    //==========================================================================

    private static bool TestCompilerRejections()
    {
        FillToolRegistry();

        ChefZ_LoadReport report = new ChefZ_LoadReport();
        report.SetMirrorToLog(false);

        ChefZ_CompileContext ctx = MakeContext(report);
        ChefZ_ProcessCompiler compiler = new ChefZ_ProcessCompiler();
        compiler.Init(ctx, report);
        compiler.SetVerifyClasses(false);

        ChefZ_ToolRegistry tools = ChefZ_ToolRegistry.Get();

        //--- Prozess mit UNBEKANNTER Werkzeuggruppe -> ABGEWIESEN -------------
        ChefZ_ProcessDef badTool = Process("CHEFZ_PT_P_BADTOOL", ChefZ_ProcessExec.STATION_ACTION_NAME, "CHEFZ_PT_GIBTSNICHT");
        if (compiler.CompileProcess(badTool, tools))                            return false;

        //--- Prozess ohne Werkzeug -> zulaessig -------------------------------
        ChefZ_ProcessDef noTool = Process(PROZ_ACTION, ChefZ_ProcessExec.STATION_ACTION_NAME, "");
        ChefZ_CompiledProcess proc = compiler.CompileProcess(noTool, tools);
        if (!proc)                                                              return false;
        if (proc.exec != ChefZ_ProcessExec.STATION_ACTION)                      return false;
        if (proc.toolGroups.Count() != 0)                                       return false;

        //--- Prozess MIT bekannter Gruppe -------------------------------------
        ChefZ_ProcessDef withTool = Process(PROZ_TIMED, ChefZ_ProcessExec.STATION_TIMED_NAME, GRUPPE_SCHNEID);
        ChefZ_CompiledProcess timed = compiler.CompileProcess(withTool, tools);
        if (!timed)                                                             return false;
        if (timed.toolGroups.Count() != 1)                                      return false;

        //--- Stationsdauer wird geklemmt --------------------------------------
        ChefZ_ProcessDef instant = Process("CHEFZ_PT_P_INSTANT", ChefZ_ProcessExec.STATION_TIMED_NAME, "");
        instant.baseDurationSec = 0.0;
        ChefZ_CompiledProcess clamped = compiler.CompileProcess(instant, tools);
        if (!clamped)                                                           return false;
        if (clamped.baseDurationSec != ChefZ_ProcessingLimits.MIN_DURATION_SEC) return false;

        //--- HANDCRAFT mit DREI Eingaengen -> ABGEWIESEN (01 V12, 11 §3) ------
        ChefZ_ProcessDef hand = Process(PROZ_HAND, ChefZ_ProcessExec.HANDCRAFT_NAME, "");
        ChefZ_CompiledProcess handProc = compiler.CompileProcess(hand, tools);
        if (!handProc)                                                          return false;

        map<int, ref ChefZ_CompiledProcess> procs = new map<int, ref ChefZ_CompiledProcess>();
        procs.Set(proc.processSym, proc);
        procs.Set(timed.processSym, timed);
        procs.Set(handProc.processSym, handProc);

        map<int, int> stations = new map<int, int>();
        stations.Set(ChefZ_SymbolTable.Intern(STATION_A), 1);

        ChefZ_TransformDef three = Transform("CHEFZ_PT_T_DREI", PROZ_HAND, KAT_A);
        three.inputs.Insert(CategorySlot("in2", KAT_B, ChefZ_ConsumeMode.WHOLE_NAME));
        three.inputs.Insert(CategorySlot("in3", KAT_A, ChefZ_ConsumeMode.WHOLE_NAME));
        three.ResolveDefaults();
        if (compiler.CompileTransform(three, procs, stations))                  return false;

        // ZWEI Eingaenge sind bei HANDCRAFT zulaessig - die Grenze ist zwei,
        // nicht eins.
        ChefZ_TransformDef two = Transform("CHEFZ_PT_T_ZWEI", PROZ_HAND, KAT_A);
        two.inputs.Insert(CategorySlot("in2", KAT_B, ChefZ_ConsumeMode.WHOLE_NAME));
        two.ResolveDefaults();
        if (!compiler.CompileTransform(two, procs, stations))                   return false;

        //--- Transform mit UNBEKANNTEM Prozess -> ABGEWIESEN -------------------
        ChefZ_TransformDef orphan = Transform("CHEFZ_PT_T_ORPHAN", "CHEFZ_PT_GIBTSNICHT", KAT_A);
        if (compiler.CompileTransform(orphan, procs, stations))                 return false;

        //--- Output ohne cls UND ohne setState -> ABGEWIESEN -------------------
        ChefZ_TransformDef mute = Transform("CHEFZ_PT_T_STUMM", PROZ_ACTION, KAT_A);
        mute.outputs.Get(0).cls = "";
        if (compiler.CompileTransform(mute, procs, stations))                   return false;

        //--- Reiner Zustandswechsel MIT consume "whole" -> ABGEWIESEN ---------
        ChefZ_TransformDef eater = Transform("CHEFZ_PT_T_FRESSER", PROZ_ACTION, KAT_A);
        eater.outputs.Get(0).cls      = "";
        eater.outputs.Get(0).setState = ZUSTAND;
        if (compiler.CompileTransform(eater, procs, stations))                  return false;

        //--- Reiner Zustandswechsel MIT consume "none" -> zulaessig -----------
        ChefZ_TransformDef keeper = new ChefZ_TransformDef();
        keeper.id      = "CHEFZ_PT_T_BEHALTER";
        keeper.process = PROZ_ACTION;
        keeper.inputs  = new array<ref ChefZ_SlotDef>();
        keeper.inputs.Insert(CategorySlot("in", KAT_A, ChefZ_ConsumeMode.NONE_NAME));
        keeper.outputs = new array<ref ChefZ_OutputDef>();
        ChefZ_OutputDef stateOnly = new ChefZ_OutputDef();
        stateOnly.setState = ZUSTAND;
        keeper.outputs.Insert(stateOnly);
        keeper.Normalize();
        keeper.ResolveDefaults();

        ChefZ_CompiledTransform pure = compiler.CompileTransform(keeper, procs, stations);
        if (!pure)                                                              return false;
        if (!pure.pureStateChange)                                              return false;

        //--- Gemischte Outputs -> ABGEWIESEN -----------------------------------
        ChefZ_TransformDef mixed = Transform("CHEFZ_PT_T_GEMISCHT", PROZ_ACTION, KAT_A);
        ChefZ_OutputDef extra = new ChefZ_OutputDef();
        extra.setState = ZUSTAND;
        extra.ResolveDefaults();
        mixed.outputs.Insert(extra);
        if (compiler.CompileTransform(mixed, procs, stations))                  return false;

        // Und der Bericht hat das alles auch gesagt - stille Abweisungen sind
        // die schlimmste Sorte.
        if (report.ErrorCount() < 6)                                            return false;

        return true;
    }

    //==========================================================================
    // 3. "Eine Teststation bietet genau die Prozesse an, die ihre
    //    Deklaration nennt" (19 S14)
    //==========================================================================

    private static bool TestStationOffers()
    {
        ChefZ_ProcessingManager mgr = BuildManager(null);

        ChefZ_Sym stationA = ChefZ_SymbolTable.Intern(STATION_A);
        ChefZ_Sym stationB = ChefZ_SymbolTable.Intern(STATION_B);
        ChefZ_Sym action   = ChefZ_SymbolTable.Intern(PROZ_ACTION);
        ChefZ_Sym timed    = ChefZ_SymbolTable.Intern(PROZ_TIMED);

        array<ChefZ_Sym> offered = new array<ChefZ_Sym>();

        // STATION_A nennt beide Prozesse.
        if (mgr.GetOfferedProcesses(stationA, offered) != 2)                    return false;
        if (offered.Get(0) != action)                                           return false;
        if (offered.Get(1) != timed)                                            return false;

        // STATION_B nennt nur einen - und einen UNBEKANNTEN, der verworfen
        // wurde (11 §7: Eintrag weg, Station bleibt).
        if (mgr.GetOfferedProcesses(stationB, offered) != 1)                    return false;
        if (offered.Get(0) != action)                                           return false;

        ChefZ_CompiledStation defB = mgr.GetStation(stationB);
        if (!defB)                                                              return false;
        if (!defB.Offers(action))                                               return false;
        if (defB.Offers(timed))                                                 return false;

        // Der Ordinal ist der INDEX in der Deklarationsreihenfolge - darauf
        // beruht die Synchronisierung des aktiven Prozesses.
        ChefZ_CompiledStation defA = mgr.GetStation(stationA);
        if (defA.OrdinalOf(action) != 0)                                        return false;
        if (defA.OrdinalOf(timed) != 1)                                         return false;
        if (defA.ProcessAt(1) != timed)                                         return false;
        if (ChefZ_SymbolTable.IsValid(defA.ProcessAt(9)))                       return false;

        // Eine unbekannte Station bietet nichts an.
        if (mgr.GetOfferedProcesses(ChefZ_SymbolTable.Intern("CHEFZ_PT_NIX"), offered) != 0)                              return false;

        // Ausfuehrungsformen sind trennbar (die Handcraft-Bruecke, S15,
        // braucht genau das).
        array<ChefZ_CompiledProcess> handcraft = new array<ChefZ_CompiledProcess>();
        mgr.GetProcessesForExec(ChefZ_ProcessExec.HANDCRAFT, handcraft);
        if (handcraft.Count() != 0)                                             return false;

        array<ChefZ_CompiledProcess> stationProcs = new array<ChefZ_CompiledProcess>();
        mgr.GetProcessesForExec(ChefZ_ProcessExec.STATION_ACTION, stationProcs);
        if (stationProcs.Count() != 1)                                          return false;

        return true;
    }

    //==========================================================================
    // 4. Rangfolge: spezifischer gewinnt, dann priority, dann id (11 §7)
    //==========================================================================

    private static bool TestTransformOrder()
    {
        ChefZ_ProcessingManager mgr = BuildManager(null);

        // T_ENG bindet KAT_B (Tiefe 1) und ist damit spezifischer als T_BREIT
        // (KAT_A, Tiefe 0). Es muss vorn stehen.
        int idxNarrow = -1;
        int idxBroad  = -1;

        for (int i = 0; i < mgr.GetTransformCount(); i++)
        {
            ChefZ_CompiledTransform tr = mgr.GetTransformAt(i);
            if (tr.id == "CHEFZ_PT_T_ENG")
                idxNarrow = i;
            else if (tr.id == "CHEFZ_PT_T_BREIT")
                idxBroad = i;
        }

        if (idxNarrow < 0 || idxBroad < 0)                                      return false;
        if (idxNarrow >= idxBroad)                                              return false;

        ChefZ_CompiledTransform narrow = mgr.GetTransformAt(idxNarrow);
        ChefZ_CompiledTransform broad  = mgr.GetTransformAt(idxBroad);
        if (narrow.specificity <= broad.specificity)                            return false;

        // minItemCount ist die Summe der minCount der Pflichteingaenge.
        if (narrow.minItemCount != 1)                                           return false;

        return true;
    }

    //==========================================================================
    // 5. FindTransform (11 §4, §5)
    //==========================================================================

    private static bool TestFindTransform()
    {
        ChefZ_ProcessingManager mgr = BuildManager(null);

        ChefZ_Sym action   = ChefZ_SymbolTable.Intern(PROZ_ACTION);
        ChefZ_Sym stationA = ChefZ_SymbolTable.Intern(STATION_A);

        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        ctx.stationClass = stationA;

        //--- Passender Inhalt -------------------------------------------------
        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        AddItem(snap, 0, "CHEFZ_PT_ITEM_ENG", BIT_B);
        snap.Get(0).closure.SetBit(BIT_A);      // KAT_B liegt unter KAT_A
        snap.SortStable();

        ChefZ_TransformMatch match;
        if (!mgr.FindTransform(action, ctx, snap, null, match))                 return false;

        // Der SPEZIFISCHERE gewinnt - erster Treffer in Rangreihenfolge
        // (11 §7, "Zwei Transforms matchen dasselbe Item").
        if (match.transformId != "CHEFZ_PT_T_ENG")                              return false;
        if (match.boundHandles.Count() != 1)                                    return false;
        if (match.consumePlan.Count() != 1)                                     return false;
        if (!match.consumePlan.Get(0).destroyWhole)                             return false;
        if (match.durationSec <= 0.0)                                           return false;

        //--- LEERE Station: kein Treffer, und das ist KEIN Fehler --------------
        //
        // Das ist zugleich die Bedingung, an der ein laufender Job "input_lost"
        // erkennt (11 §7): verschwindet das Eingangsitem, liefert genau diese
        // Suche false - und ein false heisst an der Stelle "nichts
        // veraendern".
        ChefZ_FactSnapshot empty = new ChefZ_FactSnapshot();
        ChefZ_TransformMatch none;
        if (mgr.FindTransform(action, ctx, empty, null, none))                  return false;
        if (none.matched)                                                       return false;
        if (none.failReason == "")                                              return false;

        //--- Falsche Station: der exklusive Transform bindet nicht ------------
        ChefZ_ProcessContext other = new ChefZ_ProcessContext();
        other.stationClass = ChefZ_SymbolTable.Intern(STATION_B);

        ChefZ_FactSnapshot exclusiveInput = new ChefZ_FactSnapshot();
        AddItem(exclusiveInput, 0, "CHEFZ_PT_ITEM_EXKLUSIV", BIT_A);
        exclusiveInput.SortStable();

        ChefZ_TransformMatch atB;
        // An STATION_B gibt es zwar den Prozess, aber der exklusive Transform
        // ist an STATION_A gebunden. Der breite Transform greift trotzdem -
        // er nennt keine Station (11 E5).
        if (!mgr.FindTransform(action, other, exclusiveInput, null, atB))       return false;
        if (atB.transformId == "CHEFZ_PT_T_EXKLUSIV")                           return false;

        //--- Unbekannter Prozess ----------------------------------------------
        ChefZ_TransformMatch nope;
        if (mgr.FindTransform(ChefZ_SymbolTable.Intern("CHEFZ_PT_NIXPROZ"), ctx, snap, null, nope))                           return false;

        return true;
    }

    //==========================================================================
    // 6. Umgebung und Werkzeug sperren die Suche (11 §7)
    //==========================================================================

    private static bool TestEnvironment()
    {
        ChefZ_ProcessingManager mgr = BuildManager(null);

        ChefZ_Sym timed    = ChefZ_SymbolTable.Intern(PROZ_TIMED);
        ChefZ_Sym stationA = ChefZ_SymbolTable.Intern(STATION_A);
        ChefZ_Sym schneid  = ChefZ_SymbolTable.Intern(GRUPPE_SCHNEID);

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        AddItem(snap, 0, "CHEFZ_PT_ITEM_BREIT", BIT_A);
        snap.SortStable();

        //--- Ohne Werkzeug: kein Treffer --------------------------------------
        ChefZ_ProcessContext bare = new ChefZ_ProcessContext();
        bare.stationClass = stationA;

        ChefZ_TransformMatch noTool;
        if (mgr.FindTransform(timed, bare, snap, null, noTool))                 return false;

        //--- Mit Werkzeug: Treffer --------------------------------------------
        ChefZ_ProcessContext armed = new ChefZ_ProcessContext();
        armed.stationClass = stationA;
        armed.AddToolGroup(schneid);

        ChefZ_TransformMatch withTool;
        if (!mgr.FindTransform(timed, armed, snap, null, withTool))             return false;

        //--- Ohne Brennstoff: kein Treffer, und zwar VOR dem Werkzeugtest -----
        ChefZ_ProcessContext dark = new ChefZ_ProcessContext();
        dark.stationClass   = stationA;
        dark.stationPowered = false;
        dark.AddToolGroup(schneid);

        ChefZ_TransformMatch noPower;
        if (mgr.FindTransform(timed, dark, snap, null, noPower))                return false;

        //--- Der Prozess selbst, direkt gefragt --------------------------------
        ChefZ_CompiledProcess proc = mgr.GetProcess(timed);
        if (!proc)                                                              return false;

        string reason;
        if (!proc.MeetsEnvironment(armed, reason))                              return false;
        if (proc.MeetsEnvironment(dark, reason))                                return false;
        if (reason == "")                                                       return false;

        string missing;
        if (proc.HasTools(bare, missing))                                       return false;
        if (missing == "")                                                      return false;
        if (!proc.HasTools(armed, missing))                                     return false;

        //--- CheckTools ueber den Manager (11 §4) ------------------------------
        string viaManager;
        if (mgr.CheckTools(timed, bare, viaManager))                            return false;
        if (!mgr.CheckTools(timed, armed, viaManager))                          return false;

        return true;
    }

    //==========================================================================
    // 7. Persistenz: HASH, nie Symbolzaehler (03 E2, 11 §6)
    //==========================================================================

    private static bool TestPersistHashes()
    {
        ChefZ_ProcessingManager mgr = BuildManager(null);

        ChefZ_Sym action = ChefZ_SymbolTable.Intern(PROZ_ACTION);

        int hash = mgr.GetProcessPersistHash(action);
        if (hash == ChefZ_ProcessJob.NO_HASH)                                   return false;

        // Der Hash haengt am NAMEN, nicht am Symbolzaehler.
        string name = PROZ_ACTION;
        if (hash != name.Hash())                                                return false;

        // Und er ist rueckaufloesbar - genau das braucht ein Job nach einem
        // Serverneustart.
        if (mgr.ProcessFromPersistHash(hash) != action)                         return false;

        ChefZ_CompiledTransform tr = mgr.GetTransformAt(0);
        if (!tr)                                                                return false;

        int trHash = mgr.GetTransformPersistHash(tr.transformSym);
        if (trHash == ChefZ_ProcessJob.NO_HASH)                                 return false;
        if (mgr.TransformFromPersistHash(trHash) != tr.transformSym)            return false;

        // Ein unbekannter Hash loest sich NICHT auf - der Job bricht dann ab,
        // ohne etwas zu verbrauchen (11 §6).
        if (ChefZ_SymbolTable.IsValid(mgr.TransformFromPersistHash(123456789))) return false;
        if (ChefZ_SymbolTable.IsValid( mgr.TransformFromPersistHash(ChefZ_ProcessJob.NO_HASH)))        return false;

        return true;
    }

    //==========================================================================
    // 8. Leerbestand: bereit und LEER, nicht "nicht gebaut" (11 §7)
    //==========================================================================

    private static bool TestEmptyBuild()
    {
        ChefZ_ProcessingManager mgr = new ChefZ_ProcessingManager();
        mgr.Build(null, null, null, null, null, null, null);

        if (!mgr.IsReady())                                                     return false;
        if (mgr.GetProcessCount() != 0)                                         return false;
        if (mgr.GetStationCount() != 0)                                         return false;
        if (mgr.GetTransformCount() != 0)                                       return false;

        ChefZ_Sym any = ChefZ_SymbolTable.Intern(PROZ_ACTION);
        if (mgr.HasAnyTransformFor(any))                                        return false;

        array<ChefZ_Sym> offered = new array<ChefZ_Sym>();
        if (mgr.GetOfferedProcesses(ChefZ_SymbolTable.Intern(STATION_A), offered) != 0)
            return false;

        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        ChefZ_TransformMatch match;
        if (mgr.FindTransform(any, ctx, snap, null, match))                     return false;
        if (match.failReason == "")                                             return false;

        return true;
    }

    //==========================================================================
    // Der Testbestand
    //==========================================================================

    /**
     * Ein vollstaendiger Manager auf EIGENER Instanz.
     *
     * Der Singleton bleibt unberuehrt - er wird erst vom Config Manager
     * gebaut, und das ist die Reihenfolge, die auch jeder andere Selbsttest
     * des Core einhaelt.
     *
     * Der Bestand:
     *   Prozesse   PROZ_ACTION  STATION_ACTION, kein Werkzeug
     *              PROZ_TIMED   STATION_TIMED,  Werkzeug GRUPPE_SCHNEID
     *   Stationen  STATION_A    beide Prozesse
     *              STATION_B    PROZ_ACTION + ein UNBEKANNTER (11 §7)
     *   Transforms T_BREIT      KAT_A, jede Station
     *              T_ENG        KAT_B, jede Station   (spezifischer)
     *              T_EXKLUSIV   KAT_A, nur STATION_A
     *              T_TIMED      KAT_A, PROZ_TIMED
     */
    private static ChefZ_ProcessingManager BuildManager(ChefZ_LoadReport report)
    {
        FillToolRegistry();

        ChefZ_Registry<ChefZ_ProcessDef> procs = new ChefZ_Registry<ChefZ_ProcessDef>();
        procs.Init(ChefZ_RecordKind.PROCESS);
        AddRecord(procs, Process(PROZ_ACTION, ChefZ_ProcessExec.STATION_ACTION_NAME, ""));
        AddRecord(procs, Process(PROZ_TIMED, ChefZ_ProcessExec.STATION_TIMED_NAME, GRUPPE_SCHNEID));

        ChefZ_Registry<ChefZ_StationDef> stations = new ChefZ_Registry<ChefZ_StationDef>();
        stations.Init(ChefZ_RecordKind.STATION);
        AddRecord(stations, Station(STATION_A, PROZ_ACTION, PROZ_TIMED));
        AddRecord(stations, Station(STATION_B, PROZ_ACTION, "CHEFZ_PT_GIBTSNICHT"));

        ChefZ_Registry<ChefZ_TransformDef> transforms = new ChefZ_Registry<ChefZ_TransformDef>();
        transforms.Init(ChefZ_RecordKind.TRANSFORM);

        AddRecord(transforms, Transform("CHEFZ_PT_T_BREIT", PROZ_ACTION, KAT_A));
        AddRecord(transforms, Transform("CHEFZ_PT_T_ENG", PROZ_ACTION, KAT_B));

        ChefZ_TransformDef exclusive = Transform("CHEFZ_PT_T_EXKLUSIV", PROZ_ACTION, KAT_A);
        exclusive.stationsAllowed = new array<string>();
        exclusive.stationsAllowed.Insert(STATION_A);
        exclusive.priority = -5;        // gedaempft, damit T_BREIT vorn bleibt
        exclusive.ResolveDefaults();
        AddRecord(transforms, exclusive);

        AddRecord(transforms, Transform("CHEFZ_PT_T_TIMED", PROZ_TIMED, KAT_A));

        ChefZ_LoadReport rep = report;
        if (!rep)
        {
            rep = new ChefZ_LoadReport();
            rep.SetMirrorToLog(false);
        }

        ChefZ_ProcessingManager mgr = new ChefZ_ProcessingManager();
        mgr.SetVerifyClasses(false);
        mgr.Build(procs, stations, transforms, ChefZ_ToolRegistry.Get(), MakeContext(rep), null, rep);
        return mgr;
    }

    private static void AddRecord(notnull ChefZ_RegistryBase registry, ChefZ_Record rec)
    {
        if (!rec)
            return;
        rec.sym = ChefZ_SymbolTable.Intern(rec.id);
        registry.Add(rec);
    }
}
