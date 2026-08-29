//==============================================================================
// ChefZ_MatcherSelfTest - Abnahmepruefung fuer S5
//
// Entwurf: 19 §3, S5 - "Fertig, wenn: eine Testtabelle aus Faktenlisten x
// Slotdefinitionen liefert die erwarteten Bindungen; das Backtracking-Beispiel
// aus 07 §4 bindet korrekt; ein Budgetueberlauf endet in 'kein Treffer' plus
// WARN." Und, unmissverstaendlich: "Der Matcher ist an dieser Stelle OHNE
// LAUFENDES SPIEL pruefbar - das ist der Zweck des Layer-Schnitts. Diese
// Pruefbarkeit sollte hier tatsaechlich genutzt werden."
//
// Genau das ist diese Datei. Sie baut Faktenlisten von Hand, kompiliert
// Selektoren gegen einen Nachschlager mit Testtabellen und prueft die Bindung -
// ohne Item, ohne Entity, ohne Feuerstelle, ohne Server.
//
// KEIN CONTENT: alle Namen tragen das Praefix "CHEFZ_MT_" und sind abstrakt
// (KAT_A, KLASSE_B). Das Beispiel aus 07 §4 wird in seiner STRUKTUR
// nachgebaut, nicht mit seinen Zutatennamen - der Core darf keine kennen.
//
// Kosten: einige hundert Mikrosekunden beim Serverstart. Gegenleistung: die
// Frage "rechnet der Matcher richtig" ist beantwortet, bevor der erste Spieler
// einen Topf anfasst - in einem System ohne Compilezeit-Sicherheit (03 E1)
// die einzige, die zaehlt.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_MatcherSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    //! Bitindizes des Testkategoriebaums.
    //!   KAT_TIER (0)
    //!     +- KAT_WILD (1)
    //!   KAT_GRUEN (2)
    //!   KAT_PILZ  (3)
    static const int BIT_TIER  = 0;
    static const int BIT_WILD  = 1;
    static const int BIT_GRUEN = 2;
    static const int BIT_PILZ  = 3;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("VanillaStage",   ChefZ_VanillaStage.SelfCheck());
        Check("Gewichte",       ChefZ_PriorityWeights.SelfCheck());
        Check("Nachschlager",   ChefZ_SymbolResolver.SelfCheck());
        Check("Compiler",       TestCompiler());
        Check("Praedikate",     TestPredicates());
        Check("Kombinatoren",   TestCombinators());
        Check("Bereiche",       TestRanges());
        Check("Qualitaet",      TestMinQuality());
        Check("Spezifitaet",    TestSpecificity());
        Check("Slotdefaults",   TestSlotDefaults());
        Check("Mengen",         TestAmounts());
        Check("Verbrauchsplan", TestConsumePlan());
        Check("Bindung",        TestBinding());
        Check("Backtracking",   TestBacktracking());
        Check("Budget",         TestBudget());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++; ChefZ_SelfTestTrace.Reset();
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.MATCH, "Selbsttest S5 " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.MATCH, "Selbsttest S5 " + name + " FEHLGESCHLAGEN. Der Matcher verhaelt sich nicht " + "wie entworfen - jede Rezeptentscheidung darueber ist unzuverlaessig." + ChefZ_SelfTestTrace.Take());
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S5: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
        if (s_Failed > 0 && s_FailedNames)
        {
            s = s + "  gescheitert:";
            for (int i = 0; i < s_FailedNames.Count(); i++)
                s = s + " " + s_FailedNames.Get(i);
        }
        return s;
    }

    //==========================================================================
    // Gemeinsame Testumgebung
    //==========================================================================

    /**
     * Kontext mit gefuelltem Nachschlager.
     *
     * Der Ladebericht spiegelt NICHT ins Log: der Test erzeugt absichtlich
     * Fehler, und die gehoeren nicht ins RPT eines gesunden Servers.
     */
    private static ChefZ_CompileContext MakeContext()
    {
        ChefZ_LoadReport report = new ChefZ_LoadReport();
        report.SetMirrorToLog(false);

        ChefZ_CompileContext ctx = new ChefZ_CompileContext();
        ctx.Init(report);
        ctx.SetSubject("selftest", "CHEFZ_MT");

        ChefZ_SymbolResolver res = new ChefZ_SymbolResolver();
        res.DefineCategory("CHEFZ_MT_KAT_TIER",  BIT_TIER,  0, 4);
        res.DefineCategory("CHEFZ_MT_KAT_WILD",  BIT_WILD,  1, 2);
        res.DefineCategory("CHEFZ_MT_KAT_GRUEN", BIT_GRUEN, 0, 6);
        res.DefineCategory("CHEFZ_MT_KAT_PILZ",  BIT_PILZ,  0, 1);
        res.DefineTag("CHEFZ_MT_TAG_X", 3);
        res.DefineState("CHEFZ_MT_ZUSTAND_A");
        res.DefineState("CHEFZ_MT_ZUSTAND_B");
        res.DefineState("CHEFZ_MT_ZUSTAND_WEG");
        res.DefineQuality("CHEFZ_MT_Q_NIEDRIG", 0, "CHEFZ_MT_SET");
        res.DefineQuality("CHEFZ_MT_Q_MITTE",   1, "CHEFZ_MT_SET");
        res.DefineQuality("CHEFZ_MT_Q_HOCH",    2, "CHEFZ_MT_SET");
        res.DefineUnit("CHEFZ_MT_EINHEIT");
        res.DefineClass("CHEFZ_MT_KLASSE_A");
        res.DefineClass("CHEFZ_MT_KLASSE_B");
        res.DefineClass("CHEFZ_MT_KLASSE_C");
        res.DefineClass("CHEFZ_MT_KLASSE_D");
        ctx.SetResolver(res);

        array<ChefZ_Sym> excluded = new array<ChefZ_Sym>();
        excluded.Insert(ChefZ_SymbolTable.Intern("CHEFZ_MT_ZUSTAND_WEG"));
        ctx.SetDefaultExcludedStates(excluded);

        return ctx;
    }

    private static ChefZ_Selector Leaf(string field, string value)
    {
        ChefZ_Selector sel = new ChefZ_Selector();
        if (field == "cls")             sel.cls = value;
        else if (field == "category")   sel.category = value;
        else if (field == "tag")        sel.tag = value;
        else if (field == "state")      sel.state = value;
        else if (field == "stage")      sel.vanillaStage = value;
        return sel;
    }

    //! Dasselbe fuer ein KIND der ersten Ebene. Kinder tragen den Typ der
    //! naechsten Ebene - siehe den Kopf von ChefZ_SelectorNode.c. Von Hand
    //! gebaute Baeume muessen das nachbilden, sonst uebersetzt der Test nicht.
    private static ChefZ_SelectorL1 LeafL1(string field, string value)
    {
        ChefZ_SelectorL1 sel = new ChefZ_SelectorL1();
        if (field == "cls")             sel.cls = value;
        else if (field == "category")   sel.category = value;
        else if (field == "tag")        sel.tag = value;
        else if (field == "state")      sel.state = value;
        else if (field == "stage")      sel.vanillaStage = value;
        return sel;
    }

    private static ChefZ_ItemFacts AddItem(notnull ChefZ_FactSnapshot snap, int handle, string className)
    {
        ChefZ_ItemFacts facts = snap.Acquire();
        facts.handle      = handle;
        facts.classSym    = ChefZ_SymbolTable.Intern(className);
        facts.quantity    = 1.0;
        facts.quantityMax = 1.0;
        facts.units       = 1.0;
        facts.health01    = 1.0;
        facts.freshness01 = 1.0;
        return facts;
    }

    private static ChefZ_CompiledSlot MakeSlot(notnull ChefZ_CompileContext ctx, string slotId, ChefZ_Selector match, int minCount, int maxCount, int declIndex)
    {
        ChefZ_SlotDef def = new ChefZ_SlotDef();
        def.slotId   = slotId;
        def.match    = match;
        def.minCount = minCount;
        def.maxCount = maxCount;

        string error;
        return ChefZ_SelectorCompiler.CompileSlot(def, declIndex, ctx, error);
    }

    //==========================================================================
    // 1. Compiler: was abgewiesen werden MUSS (07 §7)
    //==========================================================================

    private static bool TestCompiler()
    {
        ChefZ_CompileContext ctx = MakeContext();
        string error;

        // Leerer Selektor - wuerde auf alles passen.
        if (ChefZ_SelectorCompiler.Compile(new ChefZ_Selector(), ctx, error)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 204, "ChefZ_SelectorCompiler.Compile(new ChefZ_Selector(), ctx, error)");
        if (error == "") return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 205, "error == ''");

        // Zwei Blattfelder gleichzeitig.
        ChefZ_Selector two = new ChefZ_Selector();
        two.cls      = "CHEFZ_MT_KLASSE_A";
        two.category = "CHEFZ_MT_KAT_TIER";
        if (ChefZ_SelectorCompiler.Compile(two, ctx, error)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 211, "ChefZ_SelectorCompiler.Compile(two, ctx, error)");

        // Unbekannte Kategorie -> Abweisung, NICHT "Slot ignorieren".
        if (ChefZ_SelectorCompiler.Compile(Leaf("category", "CHEFZ_MT_GIBTESNICHT"), ctx, error))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 215, "ChefZ_SelectorCompiler.Compile(Leaf('category', 'CHEFZ_MT_GIBTESNICHT'), ctx, error)");

        // Unbekannter Tag und unbekannter Zustand.
        if (ChefZ_SelectorCompiler.Compile(Leaf("tag", "CHEFZ_MT_TAG_WEG"), ctx, error))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 219, "ChefZ_SelectorCompiler.Compile(Leaf('tag', 'CHEFZ_MT_TAG_WEG'), ctx, error)");
        if (ChefZ_SelectorCompiler.Compile(Leaf("state", "CHEFZ_MT_ZUSTAND_WEG_WEG"), ctx, error))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 221, "ChefZ_SelectorCompiler.Compile(Leaf('state', 'CHEFZ_MT_ZUSTAND_WEG_WEG'), ctx, error)");

        // Unbekannte Garstufe.
        if (ChefZ_SelectorCompiler.Compile(Leaf("stage", "Gegrillt"), ctx, error))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 225, "ChefZ_SelectorCompiler.Compile(Leaf('stage', 'Gegrillt'), ctx, error)");

        // Leeres anyOf.
        ChefZ_Selector emptyAny = new ChefZ_Selector();
        emptyAny.anyOf = new array<ref ChefZ_SelectorL1>();
        if (ChefZ_SelectorCompiler.Compile(emptyAny, ctx, error)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 230, "ChefZ_SelectorCompiler.Compile(emptyAny, ctx, error)");

        // not mit kaputtem Kind - der Fehler propagiert nach oben.
        ChefZ_Selector badNot = new ChefZ_Selector();
        badNot.not = LeafL1("category", "CHEFZ_MT_GIBTESNICHT");
        if (ChefZ_SelectorCompiler.Compile(badNot, ctx, error)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 235, "ChefZ_SelectorCompiler.Compile(badNot, ctx, error)");

        // Tiefenbegrenzung.
        // Ausgeschrieben statt in einer Schleife gewachsen: jede Ebene hat seit
        // dem Umbau ihren eigenen Typ (ChefZ_SelectorNode.c), eine Schleife
        // koennte den Baum also gar nicht mehr vertiefen. Sechs Ebenen gegen
        // eine Grenze von zwei - der Compiler muss abweisen.
        ctx.SetMaxSelectorDepth(2);
        ChefZ_SelectorL5 tief5 = new ChefZ_SelectorL5();
        tief5.cls = "CHEFZ_MT_KLASSE_A";
        ChefZ_SelectorL4 tief4 = new ChefZ_SelectorL4();
        tief4.not = tief5;
        ChefZ_SelectorL3 tief3 = new ChefZ_SelectorL3();
        tief3.not = tief4;
        ChefZ_SelectorL2 tief2 = new ChefZ_SelectorL2();
        tief2.not = tief3;
        ChefZ_SelectorL1 tief1 = new ChefZ_SelectorL1();
        tief1.not = tief2;
        ChefZ_Selector deep = new ChefZ_Selector();
        deep.not = tief1;
        if (ChefZ_SelectorCompiler.Compile(deep, ctx, error)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 255, "ChefZ_SelectorCompiler.Compile(deep, ctx, error)");
        ctx.SetMaxSelectorDepth(ChefZ_SelectorLimits.DEFAULT_MAX_DEPTH);

        // Unbekannte KLASSE ist ausdruecklich KEIN Fehler (07 §7 nennt sie nicht).
        ChefZ_CompiledSelector unknownClass = ChefZ_SelectorCompiler.Compile(Leaf("cls", "CHEFZ_MT_FREMDMOD"), ctx, error);
        if (!unknownClass) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 260, "!unknownClass");

        // Gueltiger Selektor kompiliert und traegt den Bitindex.
        ChefZ_CompiledSelector good = ChefZ_SelectorCompiler.Compile(Leaf("category", "CHEFZ_MT_KAT_WILD"), ctx, error);
        if (!good) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 264, "!good");
        if (good.op != ChefZ_SelectorOp.CATEGORY) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 265, "good.op != ChefZ_SelectorOp.CATEGORY");
        if (good.categoryBitIndex != BIT_WILD) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 266, "good.categoryBitIndex != BIT_WILD");
        if (error != "") return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 267, "error != ''");

        return true;
    }

    //==========================================================================
    // 2. Blattpraedikate (07 §3)
    //==========================================================================

    private static bool TestPredicates()
    {
        ChefZ_CompileContext ctx = MakeContext();
        string error;

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts a = AddItem(snap, 0, "CHEFZ_MT_KLASSE_A");
        a.closure.SetBit(BIT_TIER);
        a.closure.SetBit(BIT_WILD);
        a.AddTag(ChefZ_SymbolTable.Intern("CHEFZ_MT_TAG_X"));
        a.chefzState = ChefZ_SymbolTable.Intern("CHEFZ_MT_ZUSTAND_A");
        a.vanillaFoodStage = ChefZ_VanillaStage.BOILED;

        ChefZ_ItemFacts b = AddItem(snap, 1, "CHEFZ_MT_KLASSE_B");
        b.closure.SetBit(BIT_GRUEN);

        // class
        ChefZ_CompiledSelector cls = ChefZ_SelectorCompiler.Compile(Leaf("cls", "CHEFZ_MT_KLASSE_A"), ctx, error);
        if (!cls.Test(a)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 294, "!cls.Test(a)");
        if (cls.Test(b)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 295, "cls.Test(b)");

        // category: trifft die Unterkategorie UND die Oberkategorie
        ChefZ_CompiledSelector kat = ChefZ_SelectorCompiler.Compile(Leaf("category", "CHEFZ_MT_KAT_TIER"), ctx, error);
        if (!kat.Test(a)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 299, "!kat.Test(a)");
        if (kat.Test(b)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 300, "kat.Test(b)");

        // tag
        ChefZ_CompiledSelector tag = ChefZ_SelectorCompiler.Compile(Leaf("tag", "CHEFZ_MT_TAG_X"), ctx, error);
        if (!tag.Test(a)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 304, "!tag.Test(a)");
        if (tag.Test(b)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 305, "tag.Test(b)");

        // state
        ChefZ_CompiledSelector state = ChefZ_SelectorCompiler.Compile(Leaf("state", "CHEFZ_MT_ZUSTAND_A"), ctx, error);
        if (!state.Test(a)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 309, "!state.Test(a)");
        if (state.Test(b)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 310, "state.Test(b)");

        // vanillaStage
        ChefZ_CompiledSelector stage = ChefZ_SelectorCompiler.Compile(Leaf("stage", "Boiled"), ctx, error);
        if (!stage.Test(a)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 314, "!stage.Test(a)");
        if (stage.Test(b)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 315, "stage.Test(b)");

        // Ein Item ohne Closure-Bits matcht keinen Kategorieselektor - und
        // wirft dabei keinen Nullzugriff (07 §7).
        ChefZ_ItemFacts nackt = AddItem(snap, 2, "CHEFZ_MT_KLASSE_C");
        if (kat.Test(nackt)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 320, "kat.Test(nackt)");

        // Begruendung im Klartext (07 E6).
        string reason;
        if (cls.Explain(b, reason)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 324, "cls.Explain(b, reason)");
        if (reason == "") return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 325, "reason == ''");

        return true;
    }

    //==========================================================================
    // 3. Kombinatoren - der Fall, an dem eine flache Liste scheitert (07 §3)
    //==========================================================================

    private static bool TestCombinators()
    {
        ChefZ_CompileContext ctx = MakeContext();
        string error;

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();

        ChefZ_ItemFacts gesalzen = AddItem(snap, 0, "CHEFZ_MT_KLASSE_A");
        gesalzen.closure.SetBit(BIT_TIER);
        gesalzen.chefzState = ChefZ_SymbolTable.Intern("CHEFZ_MT_ZUSTAND_A");

        ChefZ_ItemFacts roh = AddItem(snap, 1, "CHEFZ_MT_KLASSE_B");
        roh.closure.SetBit(BIT_TIER);
        roh.chefzState = ChefZ_SymbolTable.Intern("CHEFZ_MT_ZUSTAND_B");

        ChefZ_ItemFacts gruen = AddItem(snap, 2, "CHEFZ_MT_KLASSE_C");
        gruen.closure.SetBit(BIT_GRUEN);

        // allOf: Kategorie UND Zustand - Production Map §43, der Fall, wegen
        // dem der Baum gewaehlt wurde (07 E1).
        ChefZ_Selector allOf = new ChefZ_Selector();
        allOf.allOf = new array<ref ChefZ_SelectorL1>();
        allOf.allOf.Insert(LeafL1("category", "CHEFZ_MT_KAT_TIER"));
        allOf.allOf.Insert(LeafL1("state", "CHEFZ_MT_ZUSTAND_A"));

        ChefZ_CompiledSelector both = ChefZ_SelectorCompiler.Compile(allOf, ctx, error);
        if (!both) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 360, "!both");
        if (!both.Test(gesalzen)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 361, "!both.Test(gesalzen)");
        if (both.Test(roh)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 362, "both.Test(roh)");
        if (both.Test(gruen)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 363, "both.Test(gruen)");

        // anyOf
        ChefZ_Selector anyOf = new ChefZ_Selector();
        anyOf.anyOf = new array<ref ChefZ_SelectorL1>();
        anyOf.anyOf.Insert(LeafL1("cls", "CHEFZ_MT_KLASSE_C"));
        anyOf.anyOf.Insert(LeafL1("category", "CHEFZ_MT_KAT_TIER"));

        ChefZ_CompiledSelector either = ChefZ_SelectorCompiler.Compile(anyOf, ctx, error);
        if (!either.Test(gesalzen)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 372, "!either.Test(gesalzen)");
        if (!either.Test(roh)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 373, "!either.Test(roh)");
        if (!either.Test(gruen)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 374, "!either.Test(gruen)");

        // not
        ChefZ_Selector notSel = new ChefZ_Selector();
        notSel.not = LeafL1("state", "CHEFZ_MT_ZUSTAND_A");

        ChefZ_CompiledSelector negiert = ChefZ_SelectorCompiler.Compile(notSel, ctx, error);
        if (negiert.Test(gesalzen)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 381, "negiert.Test(gesalzen)");
        if (!negiert.Test(roh)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 382, "!negiert.Test(roh)");
        if (!negiert.Test(gruen)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 383, "!negiert.Test(gruen)");

        return true;
    }

    //==========================================================================
    // 4. Wertebereiche
    //==========================================================================

    private static bool TestRanges()
    {
        ChefZ_CompileContext ctx = MakeContext();
        string error;

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts frisch = AddItem(snap, 0, "CHEFZ_MT_KLASSE_A");
        frisch.closure.SetBit(BIT_GRUEN);
        frisch.freshness01 = 0.9;
        frisch.health01    = 0.8;

        ChefZ_ItemFacts alt = AddItem(snap, 1, "CHEFZ_MT_KLASSE_B");
        alt.closure.SetBit(BIT_GRUEN);
        alt.freshness01 = 0.31;
        alt.health01    = 0.05;

        ChefZ_Selector sel = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        sel.freshness = new ChefZ_Range();
        sel.freshness.min = 0.5;

        ChefZ_CompiledSelector band = ChefZ_SelectorCompiler.Compile(sel, ctx, error);
        if (!band) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 413, "!band");
        if (!band.Test(frisch)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 414, "!band.Test(frisch)");
        if (band.Test(alt)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 415, "band.Test(alt)");

        // Der Trace nennt Feld und Wert, nicht nur "passt nicht" (07 E6).
        string reason;
        if (band.Explain(alt, reason)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 419, "band.Explain(alt, reason)");
        if (reason.IndexOf("freshness") < 0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 420, "reason.IndexOf('freshness') < 0");

        // Vertauschte Grenzen werden getauscht, nicht abgewiesen (07 §7):
        // aus [0.9..0.1] wird [0.1..0.9].
        ChefZ_Selector swapped = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        swapped.health = new ChefZ_Range();
        swapped.health.Init(0.9, 0.1);
        ChefZ_CompiledSelector fixedUp = ChefZ_SelectorCompiler.Compile(swapped, ctx, error);
        if (!fixedUp) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 428, "!fixedUp");
        if (!fixedUp.Test(frisch)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 429, "!fixedUp.Test(frisch)");   // 0.80 liegt drin
        if (fixedUp.Test(alt)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 430, "fixedUp.Test(alt)");   // 0.05 liegt darunter

        // Ein Bereich ohne beide Grenzen schraenkt nichts ein und wird
        // verworfen - der Selektor bleibt trotzdem gueltig.
        ChefZ_Selector offen = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        offen.wetness = new ChefZ_Range();
        ChefZ_CompiledSelector offenNode = ChefZ_SelectorCompiler.Compile(offen, ctx, error);
        if (!offenNode) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 437, "!offenNode");
        if (offenNode.ranges) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 438, "offenNode.ranges");
        if (!offenNode.Test(alt)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 439, "!offenNode.Test(alt)");

        return true;
    }

    //==========================================================================
    // 5. Qualitaetsschwelle
    //==========================================================================

    private static bool TestMinQuality()
    {
        ChefZ_CompileContext ctx = MakeContext();
        string error;

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts hoch = AddItem(snap, 0, "CHEFZ_MT_KLASSE_A");
        hoch.closure.SetBit(BIT_GRUEN);
        hoch.chefzQuality = ChefZ_SymbolTable.Intern("CHEFZ_MT_Q_HOCH");

        ChefZ_ItemFacts mitte = AddItem(snap, 1, "CHEFZ_MT_KLASSE_B");
        mitte.closure.SetBit(BIT_GRUEN);
        mitte.chefzQuality = ChefZ_SymbolTable.Intern("CHEFZ_MT_Q_MITTE");

        ChefZ_ItemFacts niedrig = AddItem(snap, 2, "CHEFZ_MT_KLASSE_C");
        niedrig.closure.SetBit(BIT_GRUEN);
        niedrig.chefzQuality = ChefZ_SymbolTable.Intern("CHEFZ_MT_Q_NIEDRIG");

        ChefZ_ItemFacts ohne = AddItem(snap, 3, "CHEFZ_MT_KLASSE_D");
        ohne.closure.SetBit(BIT_GRUEN);

        ChefZ_Selector sel = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        sel.minQuality = "CHEFZ_MT_Q_MITTE";

        ChefZ_CompiledSelector node = ChefZ_SelectorCompiler.Compile(sel, ctx, error);
        if (!node) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 473, "!node");
        if (!node.acceptedQualities) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 474, "!node.acceptedQualities");
        if (node.acceptedQualities.Count() != 2) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 475, "node.acceptedQualities.Count() != 2");   // MITTE und HOCH
        if (node.minQualityRank != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 476, "node.minQualityRank != 1");

        if (!node.Test(hoch)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 478, "!node.Test(hoch)");
        if (!node.Test(mitte)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 479, "!node.Test(mitte)");
        if (node.Test(niedrig)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 480, "node.Test(niedrig)");
        if (node.Test(ohne)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 481, "node.Test(ohne)");   // keine Qualitaet = nicht genug

        // Unbekannte Stufe weist ab.
        ChefZ_Selector bad = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        bad.minQuality = "CHEFZ_MT_Q_GIBTESNICHT";
        if (ChefZ_SelectorCompiler.Compile(bad, ctx, error)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 486, "ChefZ_SelectorCompiler.Compile(bad, ctx, error)");

        return true;
    }

    //==========================================================================
    // 6. Spezifitaet (09 §4.1)
    //==========================================================================

    private static bool TestSpecificity()
    {
        ChefZ_CompileContext ctx = MakeContext();
        ChefZ_PriorityWeights w = ctx.Weights();
        string error;

        ChefZ_CompiledSelector cls = ChefZ_SelectorCompiler.Compile(Leaf("cls", "CHEFZ_MT_KLASSE_A"), ctx, error);
        if (cls.specificity != w.wClass) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 502, "cls.specificity != w.wClass");

        // Tiefere Kategorie ist spezifischer - ohne dass ein Autor eine Zahl
        // pflegt (09 E3).
        ChefZ_CompiledSelector flach = ChefZ_SelectorCompiler.Compile(Leaf("category", "CHEFZ_MT_KAT_TIER"), ctx, error);
        ChefZ_CompiledSelector tief = ChefZ_SelectorCompiler.Compile(Leaf("category", "CHEFZ_MT_KAT_WILD"), ctx, error);
        if (flach.specificity != w.wCategoryBase) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 508, "flach.specificity != w.wCategoryBase");
        if (tief.specificity != w.wCategoryBase + w.wCategoryPerDepth) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 509, "tief.specificity != w.wCategoryBase + w.wCategoryPerDepth");
        if (tief.specificity <= flach.specificity) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 510, "tief.specificity <= flach.specificity");

        // anyOf nimmt das MINIMUM: "anyOf [exakte Klasse, breite Kategorie]"
        // ist nicht spezifischer als die breite Kategorie allein (09 §4.1).
        ChefZ_Selector anySel = new ChefZ_Selector();
        anySel.anyOf = new array<ref ChefZ_SelectorL1>();
        anySel.anyOf.Insert(LeafL1("cls", "CHEFZ_MT_KLASSE_A"));
        anySel.anyOf.Insert(LeafL1("category", "CHEFZ_MT_KAT_TIER"));
        ChefZ_CompiledSelector any = ChefZ_SelectorCompiler.Compile(anySel, ctx, error);
        if (any.specificity != flach.specificity) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 519, "any.specificity != flach.specificity");

        // allOf summiert.
        ChefZ_Selector allSel = new ChefZ_Selector();
        allSel.allOf = new array<ref ChefZ_SelectorL1>();
        allSel.allOf.Insert(LeafL1("category", "CHEFZ_MT_KAT_TIER"));
        allSel.allOf.Insert(LeafL1("state", "CHEFZ_MT_ZUSTAND_A"));
        ChefZ_CompiledSelector all = ChefZ_SelectorCompiler.Compile(allSel, ctx, error);
        if (all.specificity != w.wCategoryBase + w.wState) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 527, "all.specificity != w.wCategoryBase + w.wState");

        // Jeder gebundene Bereich zaehlt.
        ChefZ_Selector ranged = Leaf("category", "CHEFZ_MT_KAT_TIER");
        ranged.health = new ChefZ_Range();
        ranged.health.min = 0.5;
        ChefZ_CompiledSelector withRange = ChefZ_SelectorCompiler.Compile(ranged, ctx, error);
        if (withRange.specificity != w.wCategoryBase + w.wRangePerBound) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 534, "withRange.specificity != w.wCategoryBase + w.wRangePerBound");

        // selectivityHint: exakte Klasse ist am engsten.
        if (cls.selectivityHint != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 537, "cls.selectivityHint != 1");
        if (tief.selectivityHint != 2) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 538, "tief.selectivityHint != 2");   // KAT_WILD
        if (flach.selectivityHint != 4) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 539, "flach.selectivityHint != 4");   // KAT_TIER

        return true;
    }

    //==========================================================================
    // 7. Slotdefaults und excludeStates (07 §2.3, E5)
    //==========================================================================

    private static bool TestSlotDefaults()
    {
        ChefZ_CompileContext ctx = MakeContext();
        string error;

        // Nackter Slot: minCount 1, maxCount = minCount, whole, allowPartial,
        // excludeStates aus den CoreSettings.
        ChefZ_SlotDef def = new ChefZ_SlotDef();
        def.slotId = "a";
        def.match  = Leaf("category", "CHEFZ_MT_KAT_TIER");

        ChefZ_CompiledSlot slot = ChefZ_SelectorCompiler.CompileSlot(def, 0, ctx, error);
        if (!slot) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 560, "!slot");
        if (slot.minCount != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 561, "slot.minCount != 1");
        if (slot.maxCount != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 562, "slot.maxCount != 1");
        if (slot.optional) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 563, "slot.optional");
        if (!slot.allowPartial) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 564, "!slot.allowPartial");
        if (slot.consumeMode != ChefZ_ConsumeMode.WHOLE) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 565, "slot.consumeMode != ChefZ_ConsumeMode.WHOLE");
        if (slot.excludeStates.Count() != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 566, "slot.excludeStates.Count() != 1");
        if (!slot.IsStateExcluded(ChefZ_SymbolTable.Lookup("CHEFZ_MT_ZUSTAND_WEG")))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 568, "!slot.IsStateExcluded(ChefZ_SymbolTable.Lookup('CHEFZ_MT_ZUSTAND_WEG'))");

        // Ausdrueckliche Freigabe: [] bleibt leer (07 E5).
        ChefZ_SlotDef frei = new ChefZ_SlotDef();
        frei.slotId        = "b";
        frei.match         = Leaf("category", "CHEFZ_MT_KAT_TIER");
        frei.excludeStates = new array<string>();
        ChefZ_CompiledSlot freiSlot = ChefZ_SelectorCompiler.CompileSlot(frei, 1, ctx, error);
        if (!freiSlot) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 576, "!freiSlot");
        if (freiSlot.excludeStates.Count() != 0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 577, "freiSlot.excludeStates.Count() != 0");

        // Nur Tippfehler in excludeStates -> Abweisung, sonst waere der Filter
        // still abgeschaltet (07 §7).
        ChefZ_SlotDef vertippt = new ChefZ_SlotDef();
        vertippt.slotId        = "c";
        vertippt.match         = Leaf("category", "CHEFZ_MT_KAT_TIER");
        vertippt.excludeStates = new array<string>();
        vertippt.excludeStates.Insert("CHEFZ_MT_VERTIPPT");
        if (ChefZ_SelectorCompiler.CompileSlot(vertippt, 2, ctx, error))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 587, "ChefZ_SelectorCompiler.CompileSlot(vertippt, 2, ctx, error)");

        // minCount > maxCount wird geklammert, nicht abgewiesen.
        ChefZ_SlotDef schief = new ChefZ_SlotDef();
        schief.slotId   = "d";
        schief.match    = Leaf("category", "CHEFZ_MT_KAT_TIER");
        schief.minCount = 3;
        schief.maxCount = 1;
        ChefZ_CompiledSlot schiefSlot = ChefZ_SelectorCompiler.CompileSlot(schief, 3, ctx, error);
        if (!schiefSlot) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 596, "!schiefSlot");
        if (schiefSlot.maxCount != 3) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 597, "schiefSlot.maxCount != 3");

        // amount <= 0 wird auf 1 gesetzt (07 §7).
        //
        // Geprueft wird mit -5.0 und nicht mit 0.0: seit
        // ChefZ_Undefined.FLOAT == 0.0 ist eine Untergrenze von null von
        // "keine Untergrenze" nicht mehr zu unterscheiden, und ein Bereich
        // ohne Grenzen ist kein Bereich <= 0, sondern gar keiner. Die Regel
        // selbst ist davon unberuehrt, und ihr Ergebnis auch: ein Slot ohne
        // amount liefert RequiredUnits() == 0.0, was jeder Aufrufer als eine
        // Einheit liest (ChefZ_PortionManager.RequiredUnitsOf). Der Weg ist
        // ein anderer, die Menge dieselbe.
        ChefZ_SlotDef nullAmount = new ChefZ_SlotDef();
        nullAmount.slotId = "e";
        nullAmount.match  = Leaf("category", "CHEFZ_MT_KAT_TIER");
        nullAmount.amount = new ChefZ_Range();
        nullAmount.amount.min = -5.0;
        ChefZ_CompiledSlot nullSlot = ChefZ_SelectorCompiler.CompileSlot(nullAmount, 4, ctx, error);
        if (!nullSlot) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 615, "!nullSlot");
        if (nullSlot.RequiredUnits() != 1.0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 616, "nullSlot.RequiredUnits() != 1.0");

        // Slot ohne match ist ein Fehler.
        ChefZ_SlotDef leer = new ChefZ_SlotDef();
        leer.slotId = "f";
        if (ChefZ_SelectorCompiler.CompileSlot(leer, 5, ctx, error))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 622, "ChefZ_SelectorCompiler.CompileSlot(leer, 5, ctx, error)");

        return true;
    }

    //==========================================================================
    // 8. Mengen: die Summe zaehlt, nicht das einzelne Item (07 E3)
    //==========================================================================

    private static bool TestAmounts()
    {
        ChefZ_CompileContext ctx = MakeContext();
        string error;

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts halb1 = AddItem(snap, 0, "CHEFZ_MT_KLASSE_A");
        halb1.closure.SetBit(BIT_GRUEN);
        halb1.units = 1.0;
        halb1.quantityUnit = ChefZ_SymbolTable.Intern("CHEFZ_MT_EINHEIT");

        ChefZ_ItemFacts halb2 = AddItem(snap, 1, "CHEFZ_MT_KLASSE_B");
        halb2.closure.SetBit(BIT_GRUEN);
        halb2.units = 1.0;
        halb2.quantityUnit = ChefZ_SymbolTable.Intern("CHEFZ_MT_EINHEIT");

        ChefZ_SlotDef def = new ChefZ_SlotDef();
        def.slotId   = "mehl";
        def.match    = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        def.minCount = 1;
        def.maxCount = 2;
        def.unit     = "CHEFZ_MT_EINHEIT";
        def.amount   = new ChefZ_Range();
        def.amount.min = 2.0;

        ChefZ_CompiledSlot slot = ChefZ_SelectorCompiler.CompileSlot(def, 0, ctx, error);
        if (!slot) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 657, "!slot");

        array<ref ChefZ_CompiledSlot> slots = new array<ref ChefZ_CompiledSlot>();
        slots.Insert(slot);

        // Zwei Items mit je einer Einheit erfuellen "min 2" gemeinsam.
        ChefZ_BindResult result;
        if (!ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_MENGE", null, result))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 665, "!ChefZ_Matcher.Bind(slots, snap, 4096, 'CHEFZ_MT_MENGE', null, result)");
        if (result.bindings.Get(0).Count() != 2) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 666, "result.bindings.Get(0).Count() != 2");
        if (result.bindings.Get(0).totalUnits != 2.0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 667, "result.bindings.Get(0).totalUnits != 2.0");

        // Mit allowPartial = false muss ein einzelnes Item die Menge tragen.
        def.allowPartial = false;
        def.MarkExplicit("allowPartial");
        ChefZ_CompiledSlot ganz = ChefZ_SelectorCompiler.CompileSlot(def, 0, ctx, error);
        if (!ganz) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 673, "!ganz");
        if (ganz.allowPartial) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 674, "ganz.allowPartial");

        array<ref ChefZ_CompiledSlot> ganzSlots = new array<ref ChefZ_CompiledSlot>();
        ganzSlots.Insert(ganz);
        if (ChefZ_Matcher.Bind(ganzSlots, snap, 4096, "CHEFZ_MT_GANZ", null, result))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 679, "ChefZ_Matcher.Bind(ganzSlots, snap, 4096, 'CHEFZ_MT_GANZ', null, result)");

        // Falsche Einheit -> kein Kandidat (05 §6, der Core rechnet nie um).
        halb1.quantityUnit = ChefZ_SymbolTable.Intern("CHEFZ_MT_ANDERE_EINHEIT");
        halb2.quantityUnit = ChefZ_SymbolTable.Intern("CHEFZ_MT_ANDERE_EINHEIT");
        if (ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_EINHEIT", null, result))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 685, "ChefZ_Matcher.Bind(slots, snap, 4096, 'CHEFZ_MT_EINHEIT', null, result)");

        return true;
    }

    //==========================================================================
    // 9. Verbrauchsplan (07 §2.3, 05 §6)
    //==========================================================================

    private static bool TestConsumePlan()
    {
        ChefZ_CompileContext ctx = MakeContext();
        string error;

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts stapel = AddItem(snap, 7, "CHEFZ_MT_KLASSE_A");
        stapel.closure.SetBit(BIT_GRUEN);
        stapel.quantity    = 100.0;
        stapel.quantityMax = 100.0;
        stapel.units       = 4.0;       // 25 Quantity je Einheit

        array<int> assigned = new array<int>();
        assigned.Insert(0);

        // whole: Item wird geloescht.
        ChefZ_SlotDef wholeDef = new ChefZ_SlotDef();
        wholeDef.slotId = "ganz";
        wholeDef.match  = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        ChefZ_CompiledSlot wholeSlot = ChefZ_SelectorCompiler.CompileSlot(wholeDef, 0, ctx, error);

        array<ref ChefZ_ConsumePlan> plan = new array<ref ChefZ_ConsumePlan>();
        ChefZ_SlotEvaluator.BuildConsumePlanIdx(wholeSlot, snap, assigned, plan);
        if (plan.Count() != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 717, "plan.Count() != 1");
        if (!plan.Get(0).destroyWhole) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 718, "!plan.Get(0).destroyWhole");
        if (plan.Get(0).handle != 7) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 719, "plan.Get(0).handle != 7");

        // amount: anteilig abziehen, Rest bleibt liegen.
        ChefZ_SlotDef amountDef = new ChefZ_SlotDef();
        amountDef.slotId        = "teil";
        amountDef.match         = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        amountDef.consume       = "amount";
        amountDef.consumeAmount = 1.0;
        ChefZ_CompiledSlot amountSlot = ChefZ_SelectorCompiler.CompileSlot(amountDef, 1, ctx, error);
        if (!amountSlot) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 728, "!amountSlot");
        if (amountSlot.consumeMode != ChefZ_ConsumeMode.AMOUNT) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 729, "amountSlot.consumeMode != ChefZ_ConsumeMode.AMOUNT");

        array<ref ChefZ_ConsumePlan> plan2 = new array<ref ChefZ_ConsumePlan>();
        ChefZ_SlotEvaluator.BuildConsumePlanIdx(amountSlot, snap, assigned, plan2);
        if (plan2.Count() != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 733, "plan2.Count() != 1");
        if (plan2.Get(0).destroyWhole) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 734, "plan2.Get(0).destroyWhole");
        if (plan2.Get(0).unitsDelta != 1.0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 735, "plan2.Get(0).unitsDelta != 1.0");
        if (plan2.Get(0).quantityDelta != 25.0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 736, "plan2.Get(0).quantityDelta != 25.0");

        // amount groesser als vorhanden: Item ist leer und wird geloescht.
        amountDef.consumeAmount = 99.0;
        ChefZ_CompiledSlot leerSlot = ChefZ_SelectorCompiler.CompileSlot(amountDef, 2, ctx, error);
        array<ref ChefZ_ConsumePlan> plan3 = new array<ref ChefZ_ConsumePlan>();
        ChefZ_SlotEvaluator.BuildConsumePlanIdx(leerSlot, snap, assigned, plan3);
        if (plan3.Count() != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 743, "plan3.Count() != 1");
        if (!plan3.Get(0).destroyWhole) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 744, "!plan3.Get(0).destroyWhole");

        // none + setStateAfter: nichts verbraucht, Zustand gewechselt.
        ChefZ_SlotDef noneDef = new ChefZ_SlotDef();
        noneDef.slotId        = "werkzeug";
        noneDef.match         = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        noneDef.consume       = "none";
        noneDef.setStateAfter = "CHEFZ_MT_ZUSTAND_B";
        ChefZ_CompiledSlot noneSlot = ChefZ_SelectorCompiler.CompileSlot(noneDef, 3, ctx, error);
        if (!noneSlot) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 753, "!noneSlot");

        array<ref ChefZ_ConsumePlan> plan4 = new array<ref ChefZ_ConsumePlan>();
        ChefZ_SlotEvaluator.BuildConsumePlanIdx(noneSlot, snap, assigned, plan4);
        if (plan4.Count() != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 757, "plan4.Count() != 1");
        if (plan4.Get(0).destroyWhole) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 758, "plan4.Get(0).destroyWhole");
        if (plan4.Get(0).quantityDelta != 0.0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 759, "plan4.Get(0).quantityDelta != 0.0");
        if (plan4.Get(0).setStateAfter != ChefZ_SymbolTable.Lookup("CHEFZ_MT_ZUSTAND_B"))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 761, "plan4.Get(0).setStateAfter != ChefZ_SymbolTable.Lookup('CHEFZ_MT_ZUSTAND_B')");

        // Unbekannter Zielzustand weist ab.
        noneDef.setStateAfter = "CHEFZ_MT_ZUSTAND_GIBTESNICHT";
        if (ChefZ_SelectorCompiler.CompileSlot(noneDef, 4, ctx, error))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 766, "ChefZ_SelectorCompiler.CompileSlot(noneDef, 4, ctx, error)");

        return true;
    }

    //==========================================================================
    // 10. Bindung: Grundfaelle
    //==========================================================================

    private static bool TestBinding()
    {
        ChefZ_CompileContext ctx = MakeContext();

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts tier = AddItem(snap, 10, "CHEFZ_MT_KLASSE_A");
        tier.closure.SetBit(BIT_TIER);
        ChefZ_ItemFacts gruen = AddItem(snap, 11, "CHEFZ_MT_KLASSE_B");
        gruen.closure.SetBit(BIT_GRUEN);
        ChefZ_ItemFacts fremd = AddItem(snap, 12, "CHEFZ_MT_KLASSE_C");
        snap.SortStable();

        array<ref ChefZ_CompiledSlot> slots = new array<ref ChefZ_CompiledSlot>();
        slots.Insert(MakeSlot(ctx, "fleisch", Leaf("category", "CHEFZ_MT_KAT_TIER"), 1, 1, 0));
        slots.Insert(MakeSlot(ctx, "gemuese", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 1, 1, 1));
        if (!slots.Get(0) || !slots.Get(1)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 790, "!slots.Get(0) || !slots.Get(1)");

        ChefZ_BindResult result;
        if (!ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_BIND", null, result))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 794, "!ChefZ_Matcher.Bind(slots, snap, 4096, 'CHEFZ_MT_BIND', null, result)");

        // Bindung in DEKLARATIONSreihenfolge (07 §4, Schritt 6).
        if (result.bindings.Count() != 2) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 797, "result.bindings.Count() != 2");
        if (result.bindings.Get(0).slotId != "fleisch") return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 798, "result.bindings.Get(0).slotId != 'fleisch'");
        if (result.bindings.Get(0).handles.Get(0) != 10) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 799, "result.bindings.Get(0).handles.Get(0) != 10");
        if (result.bindings.Get(1).handles.Get(0) != 11) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 800, "result.bindings.Get(1).handles.Get(0) != 11");

        // Der Rest bleibt uebrig und geht an die Policy des Rezepts.
        if (result.extraHandles.Count() != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 803, "result.extraHandles.Count() != 1");
        if (result.extraHandles.Get(0) != 12) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 804, "result.extraHandles.Get(0) != 12");
        if (result.BoundItemCount() != 2) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 805, "result.BoundItemCount() != 2");

        // Der Snapshot bleibt unmarkiert zurueck - der Matcher schleppt
        // nichts mit (07 §6).
        for (int i = 0; i < snap.Count(); i++)
        {
            if (snap.Get(i).slotBoundTo != -1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 811, "snap.Get(i).slotBoundTo != -1");
        }

        // Determinismus: zweimal dasselbe Ergebnis.
        ChefZ_BindResult second;
        if (!ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_BIND", null, second))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 817, "!ChefZ_Matcher.Bind(slots, snap, 4096, 'CHEFZ_MT_BIND', null, second)");
        if (second.bindings.Get(0).handles.Get(0) != 10) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 818, "second.bindings.Get(0).handles.Get(0) != 10");
        if (second.bindings.Get(1).handles.Get(0) != 11) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 819, "second.bindings.Get(1).handles.Get(0) != 11");

        // Fehlende Zutat -> kein Treffer, mit Begruendung.
        array<ref ChefZ_CompiledSlot> unmoeglich = new array<ref ChefZ_CompiledSlot>();
        unmoeglich.Insert(MakeSlot(ctx, "pilz", Leaf("category", "CHEFZ_MT_KAT_PILZ"), 1, 1, 0));
        ChefZ_BindResult miss;
        if (ChefZ_Matcher.Bind(unmoeglich, snap, 4096, "CHEFZ_MT_MISS", null, miss))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 826, "ChefZ_Matcher.Bind(unmoeglich, snap, 4096, 'CHEFZ_MT_MISS', null, miss)");
        if (miss.failSlotId != "pilz") return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 827, "miss.failSlotId != 'pilz'");
        if (miss.failReason == "") return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 828, "miss.failReason == ''");

        // EIN Item bedient hoechstens EINEN Slot - sonst waere es ein
        // Duplikations-Exploit (07 §4).
        array<ref ChefZ_CompiledSlot> zweimal = new array<ref ChefZ_CompiledSlot>();
        zweimal.Insert(MakeSlot(ctx, "t1", Leaf("category", "CHEFZ_MT_KAT_TIER"), 1, 1, 0));
        zweimal.Insert(MakeSlot(ctx, "t2", Leaf("category", "CHEFZ_MT_KAT_TIER"), 1, 1, 1));
        ChefZ_BindResult doppelt;
        if (ChefZ_Matcher.Bind(zweimal, snap, 4096, "CHEFZ_MT_DOPPELT", null, doppelt))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 837, "ChefZ_Matcher.Bind(zweimal, snap, 4096, 'CHEFZ_MT_DOPPELT', null, doppelt)");

        // Ausgeschlossener Zustand nimmt das Item aus dem Rennen (07 E5).
        tier.chefzState = ChefZ_SymbolTable.Intern("CHEFZ_MT_ZUSTAND_WEG");
        ChefZ_BindResult verdorben;
        if (ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_VERDORBEN", null, verdorben))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 843, "ChefZ_Matcher.Bind(slots, snap, 4096, 'CHEFZ_MT_VERDORBEN', null, verdorben)");
        tier.chefzState = ChefZ_SymbolTable.INVALID;

        // Optionaler Slot: leer ist kein Abbruchgrund, gefuellt bringt Punkte.
        ChefZ_SlotDef optDef = new ChefZ_SlotDef();
        optDef.slotId      = "gewuerz";
        optDef.match       = Leaf("cls", "CHEFZ_MT_KLASSE_C");
        optDef.optional    = true;
        optDef.gradePoints = 2;
        optDef.MarkExplicit("optional");
        string error;
        ChefZ_CompiledSlot optSlot = ChefZ_SelectorCompiler.CompileSlot(optDef, 2, ctx, error);
        if (!optSlot) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 855, "!optSlot");
        if (!optSlot.optional) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 856, "!optSlot.optional");

        slots.Insert(optSlot);
        ChefZ_BindResult mitOption;
        if (!ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_OPTION", null, mitOption))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 861, "!ChefZ_Matcher.Bind(slots, snap, 4096, 'CHEFZ_MT_OPTION', null, mitOption)");
        if (mitOption.bindings.Count() != 3) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 862, "mitOption.bindings.Count() != 3");
        if (!mitOption.bindings.Get(2).filled) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 863, "!mitOption.bindings.Get(2).filled");
        if (mitOption.gradePoints != 2) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 864, "mitOption.gradePoints != 2");
        if (mitOption.extraHandles.Count() != 0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 865, "mitOption.extraHandles.Count() != 0");

        return true;
    }

    //==========================================================================
    // 11. Das Beispiel aus 07 §4 - der Fall, an dem Greedy scheitert
    //==========================================================================

    private static bool TestBacktracking()
    {
        ChefZ_CompileContext ctx = MakeContext();

        // Struktur des Beispiels aus 07 §4:
        //   A liegt in KAT_WILD UND in KAT_GRUEN  (das Wild, das auch als
        //     Gemuesekandidat durchgeht)
        //   B, C liegen nur in KAT_GRUEN
        //   D ist die exakte Klasse fuer den dritten Slot
        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts a = AddItem(snap, 20, "CHEFZ_MT_KLASSE_A");
        a.closure.SetBit(BIT_WILD);
        a.closure.SetBit(BIT_GRUEN);
        ChefZ_ItemFacts b = AddItem(snap, 21, "CHEFZ_MT_KLASSE_B");
        b.closure.SetBit(BIT_GRUEN);
        ChefZ_ItemFacts c = AddItem(snap, 22, "CHEFZ_MT_KLASSE_C");
        c.closure.SetBit(BIT_GRUEN);
        ChefZ_ItemFacts d = AddItem(snap, 23, "CHEFZ_MT_KLASSE_D");
        snap.SortStable();

        ChefZ_Selector kraut = new ChefZ_Selector();
        kraut.anyOf = new array<ref ChefZ_SelectorL1>();
        kraut.anyOf.Insert(LeafL1("cls", "CHEFZ_MT_KLASSE_D"));
        kraut.anyOf.Insert(LeafL1("cls", "CHEFZ_MT_FEHLT"));

        array<ref ChefZ_CompiledSlot> slots = new array<ref ChefZ_CompiledSlot>();
        slots.Insert(MakeSlot(ctx, "wild",    Leaf("category", "CHEFZ_MT_KAT_WILD"), 1, 1, 0));
        slots.Insert(MakeSlot(ctx, "gemuese", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 2, 2, 1));
        slots.Insert(MakeSlot(ctx, "kraut",   kraut, 1, 1, 2));
        if (!slots.Get(0) || !slots.Get(1) || !slots.Get(2)) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 903, "!slots.Get(0) || !slots.Get(1) || !slots.Get(2)");

        ChefZ_BindResult result;
        if (!ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_BACKTRACK", null, result))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 907, "!ChefZ_Matcher.Bind(slots, snap, 4096, 'CHEFZ_MT_BACKTRACK', null, result)");

        // Die einzige gueltige Gesamtzuordnung: A -> wild, B+C -> gemuese,
        // D -> kraut. Ein gieriger Matcher, der "gemuese" zuerst bedient und
        // dabei A greift, findet sie nicht.
        if (result.bindings.Get(0).handles.Count() != 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 912, "result.bindings.Get(0).handles.Count() != 1");
        if (result.bindings.Get(0).handles.Get(0) != 20) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 913, "result.bindings.Get(0).handles.Get(0) != 20");
        if (result.bindings.Get(1).handles.Count() != 2) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 914, "result.bindings.Get(1).handles.Count() != 2");
        if (result.bindings.Get(1).handles.Find(21) < 0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 915, "result.bindings.Get(1).handles.Find(21) < 0");
        if (result.bindings.Get(1).handles.Find(22) < 0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 916, "result.bindings.Get(1).handles.Find(22) < 0");
        if (result.bindings.Get(1).handles.Find(20) >= 0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 917, "result.bindings.Get(1).handles.Find(20) >= 0");
        if (result.bindings.Get(2).handles.Get(0) != 23) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 918, "result.bindings.Get(2).handles.Get(0) != 23");
        if (result.extraHandles.Count() != 0) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 919, "result.extraHandles.Count() != 0");

        // Ein Fall, der die Rueckwaertssuche wirklich braucht: beide Slots
        // haben gleich viele Kandidaten, und die erste Wahl des ersten Slots
        // ist die falsche.
        ChefZ_FactSnapshot snap2 = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts x = AddItem(snap2, 30, "CHEFZ_MT_KLASSE_A");
        x.closure.SetBit(BIT_GRUEN);
        ChefZ_ItemFacts y = AddItem(snap2, 31, "CHEFZ_MT_KLASSE_B");
        ChefZ_ItemFacts z = AddItem(snap2, 32, "CHEFZ_MT_KLASSE_C");
        z.closure.SetBit(BIT_GRUEN);
        snap2.SortStable();

        ChefZ_Selector entweder = new ChefZ_Selector();
        entweder.anyOf = new array<ref ChefZ_SelectorL1>();
        entweder.anyOf.Insert(LeafL1("cls", "CHEFZ_MT_KLASSE_A"));
        entweder.anyOf.Insert(LeafL1("cls", "CHEFZ_MT_KLASSE_B"));

        array<ref ChefZ_CompiledSlot> slots2 = new array<ref ChefZ_CompiledSlot>();
        slots2.Insert(MakeSlot(ctx, "eins", entweder, 1, 1, 0));
        slots2.Insert(MakeSlot(ctx, "zwei", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 2, 2, 1));

        ChefZ_BindResult result2;
        if (!ChefZ_Matcher.Bind(slots2, snap2, 4096, "CHEFZ_MT_BACKTRACK2", null, result2))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 943, "!ChefZ_Matcher.Bind(slots2, snap2, 4096, 'CHEFZ_MT_BACKTRACK2', null, result2)");
        if (result2.bindings.Get(0).handles.Get(0) != 31) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 944, "result2.bindings.Get(0).handles.Get(0) != 31");   // B, nicht A
        if (result2.bindings.Get(1).handles.Count() != 2) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 945, "result2.bindings.Get(1).handles.Count() != 2");

        return true;
    }

    //==========================================================================
    // 12. Knotenbudget (07 §7)
    //==========================================================================

    private static bool TestBudget()
    {
        ChefZ_CompileContext ctx = MakeContext();

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        for (int i = 0; i < 6; i++)
        {
            ChefZ_ItemFacts f = AddItem(snap, 40 + i, "CHEFZ_MT_KLASSE_A");
            f.closure.SetBit(BIT_GRUEN);
        }
        snap.SortStable();

        array<ref ChefZ_CompiledSlot> slots = new array<ref ChefZ_CompiledSlot>();
        slots.Insert(MakeSlot(ctx, "s1", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 2, 2, 0));
        slots.Insert(MakeSlot(ctx, "s2", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 2, 2, 1));
        slots.Insert(MakeSlot(ctx, "s3", Leaf("category", "CHEFZ_MT_KAT_PILZ"),  1, 1, 2));

        // Mit Budget bindet nichts, weil KAT_PILZ fehlt - aber ohne
        // Budgetueberlauf.
        ChefZ_BindResult ok;
        if (ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_BUDGET_OK", null, ok))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 975, "ChefZ_Matcher.Bind(slots, snap, 4096, 'CHEFZ_MT_BUDGET_OK', null, ok)");
        if (ok.budgetExhausted) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 976, "ok.budgetExhausted");

        // Mit Budget 1 endet schon der erste Bindungsversuch - kein Treffer,
        // Flagge gesetzt, nichts veraendert.
        array<ref ChefZ_CompiledSlot> zwei = new array<ref ChefZ_CompiledSlot>();
        zwei.Insert(MakeSlot(ctx, "a", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 2, 2, 0));
        zwei.Insert(MakeSlot(ctx, "b", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 2, 2, 1));

        ChefZ_Matcher.SetQuietForTest(true);
        ChefZ_BindResult knapp;
        bool matched = ChefZ_Matcher.Bind(zwei, snap, 1, "CHEFZ_MT_BUDGET", null, knapp);
        ChefZ_Matcher.SetQuietForTest(false);

        if (matched) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 989, "matched");
        if (!knapp.budgetExhausted) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 990, "!knapp.budgetExhausted");
        if (knapp.failReason == "") return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 991, "knapp.failReason == ''");
        if (knapp.nodesExplored < 1) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 992, "knapp.nodesExplored < 1");

        // Mit ausreichendem Budget bindet derselbe Fall.
        ChefZ_BindResult reich;
        if (!ChefZ_Matcher.Bind(zwei, snap, 4096, "CHEFZ_MT_BUDGET2", null, reich))
            return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 997, "!ChefZ_Matcher.Bind(zwei, snap, 4096, 'CHEFZ_MT_BUDGET2', null, reich)");
        if (reich.budgetExhausted) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 998, "reich.budgetExhausted");
        if (reich.BoundItemCount() != 4) return ChefZ_SelfTestTrace.Fail("MatcherSelfTest", 999, "reich.BoundItemCount() != 4");

        return true;
    }
}
