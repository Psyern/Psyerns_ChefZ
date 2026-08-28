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
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.MATCH, "Selbsttest S5 " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.MATCH, "Selbsttest S5 " + name + " FEHLGESCHLAGEN. Der Matcher verhaelt sich nicht " + "wie entworfen - jede Rezeptentscheidung darueber ist unzuverlaessig.");
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
        if (ChefZ_SelectorCompiler.Compile(new ChefZ_Selector(), ctx, error))    return false;
        if (error == "")                                                          return false;

        // Zwei Blattfelder gleichzeitig.
        ChefZ_Selector two = new ChefZ_Selector();
        two.cls      = "CHEFZ_MT_KLASSE_A";
        two.category = "CHEFZ_MT_KAT_TIER";
        if (ChefZ_SelectorCompiler.Compile(two, ctx, error))                      return false;

        // Unbekannte Kategorie -> Abweisung, NICHT "Slot ignorieren".
        if (ChefZ_SelectorCompiler.Compile(Leaf("category", "CHEFZ_MT_GIBTESNICHT"), ctx, error))
            return false;

        // Unbekannter Tag und unbekannter Zustand.
        if (ChefZ_SelectorCompiler.Compile(Leaf("tag", "CHEFZ_MT_TAG_WEG"), ctx, error))
            return false;
        if (ChefZ_SelectorCompiler.Compile(Leaf("state", "CHEFZ_MT_ZUSTAND_WEG_WEG"), ctx, error))
            return false;

        // Unbekannte Garstufe.
        if (ChefZ_SelectorCompiler.Compile(Leaf("stage", "Gegrillt"), ctx, error))
            return false;

        // Leeres anyOf.
        ChefZ_Selector emptyAny = new ChefZ_Selector();
        emptyAny.anyOf = new array<ref ChefZ_Selector>();
        if (ChefZ_SelectorCompiler.Compile(emptyAny, ctx, error))                 return false;

        // not mit kaputtem Kind - der Fehler propagiert nach oben.
        ChefZ_Selector badNot = new ChefZ_Selector();
        badNot.not = Leaf("category", "CHEFZ_MT_GIBTESNICHT");
        if (ChefZ_SelectorCompiler.Compile(badNot, ctx, error))                   return false;

        // Tiefenbegrenzung.
        ctx.SetMaxSelectorDepth(2);
        ChefZ_Selector deep = Leaf("cls", "CHEFZ_MT_KLASSE_A");
        for (int d = 0; d < 5; d++)
        {
            ChefZ_Selector wrap = new ChefZ_Selector();
            wrap.not = deep;
            deep = wrap;
        }
        if (ChefZ_SelectorCompiler.Compile(deep, ctx, error))                     return false;
        ctx.SetMaxSelectorDepth(ChefZ_SelectorLimits.DEFAULT_MAX_DEPTH);

        // Unbekannte KLASSE ist ausdruecklich KEIN Fehler (07 §7 nennt sie nicht).
        ChefZ_CompiledSelector unknownClass =
            ChefZ_SelectorCompiler.Compile(Leaf("cls", "CHEFZ_MT_FREMDMOD"), ctx, error);
        if (!unknownClass)                                                        return false;

        // Gueltiger Selektor kompiliert und traegt den Bitindex.
        ChefZ_CompiledSelector good =
            ChefZ_SelectorCompiler.Compile(Leaf("category", "CHEFZ_MT_KAT_WILD"), ctx, error);
        if (!good)                                                                return false;
        if (good.op != ChefZ_SelectorOp.CATEGORY)                                 return false;
        if (good.categoryBitIndex != BIT_WILD)                                    return false;
        if (error != "")                                                          return false;

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
        ChefZ_CompiledSelector cls =
            ChefZ_SelectorCompiler.Compile(Leaf("cls", "CHEFZ_MT_KLASSE_A"), ctx, error);
        if (!cls.Test(a))                       return false;
        if (cls.Test(b))                        return false;

        // category: trifft die Unterkategorie UND die Oberkategorie
        ChefZ_CompiledSelector kat =
            ChefZ_SelectorCompiler.Compile(Leaf("category", "CHEFZ_MT_KAT_TIER"), ctx, error);
        if (!kat.Test(a))                       return false;
        if (kat.Test(b))                        return false;

        // tag
        ChefZ_CompiledSelector tag =
            ChefZ_SelectorCompiler.Compile(Leaf("tag", "CHEFZ_MT_TAG_X"), ctx, error);
        if (!tag.Test(a))                       return false;
        if (tag.Test(b))                        return false;

        // state
        ChefZ_CompiledSelector state =
            ChefZ_SelectorCompiler.Compile(Leaf("state", "CHEFZ_MT_ZUSTAND_A"), ctx, error);
        if (!state.Test(a))                     return false;
        if (state.Test(b))                      return false;

        // vanillaStage
        ChefZ_CompiledSelector stage =
            ChefZ_SelectorCompiler.Compile(Leaf("stage", "Boiled"), ctx, error);
        if (!stage.Test(a))                     return false;
        if (stage.Test(b))                      return false;

        // Ein Item ohne Closure-Bits matcht keinen Kategorieselektor - und
        // wirft dabei keinen Nullzugriff (07 §7).
        ChefZ_ItemFacts nackt = AddItem(snap, 2, "CHEFZ_MT_KLASSE_C");
        if (kat.Test(nackt))                    return false;

        // Begruendung im Klartext (07 E6).
        string reason;
        if (cls.Explain(b, reason))             return false;
        if (reason == "")                       return false;

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
        allOf.allOf = new array<ref ChefZ_Selector>();
        allOf.allOf.Insert(Leaf("category", "CHEFZ_MT_KAT_TIER"));
        allOf.allOf.Insert(Leaf("state", "CHEFZ_MT_ZUSTAND_A"));

        ChefZ_CompiledSelector both = ChefZ_SelectorCompiler.Compile(allOf, ctx, error);
        if (!both)                              return false;
        if (!both.Test(gesalzen))               return false;
        if (both.Test(roh))                     return false;
        if (both.Test(gruen))                   return false;

        // anyOf
        ChefZ_Selector anyOf = new ChefZ_Selector();
        anyOf.anyOf = new array<ref ChefZ_Selector>();
        anyOf.anyOf.Insert(Leaf("cls", "CHEFZ_MT_KLASSE_C"));
        anyOf.anyOf.Insert(Leaf("category", "CHEFZ_MT_KAT_TIER"));

        ChefZ_CompiledSelector either = ChefZ_SelectorCompiler.Compile(anyOf, ctx, error);
        if (!either.Test(gesalzen))             return false;
        if (!either.Test(roh))                  return false;
        if (!either.Test(gruen))                return false;

        // not
        ChefZ_Selector notSel = new ChefZ_Selector();
        notSel.not = Leaf("state", "CHEFZ_MT_ZUSTAND_A");

        ChefZ_CompiledSelector negiert = ChefZ_SelectorCompiler.Compile(notSel, ctx, error);
        if (negiert.Test(gesalzen))             return false;
        if (!negiert.Test(roh))                 return false;
        if (!negiert.Test(gruen))               return false;

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
        if (!band)                              return false;
        if (!band.Test(frisch))                 return false;
        if (band.Test(alt))                     return false;

        // Der Trace nennt Feld und Wert, nicht nur "passt nicht" (07 E6).
        string reason;
        if (band.Explain(alt, reason))          return false;
        if (reason.IndexOf("freshness") < 0)    return false;

        // Vertauschte Grenzen werden getauscht, nicht abgewiesen (07 §7):
        // aus [0.9..0.1] wird [0.1..0.9].
        ChefZ_Selector swapped = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        swapped.health = new ChefZ_Range();
        swapped.health.Init(0.9, 0.1);
        ChefZ_CompiledSelector fixedUp = ChefZ_SelectorCompiler.Compile(swapped, ctx, error);
        if (!fixedUp)                           return false;
        if (!fixedUp.Test(frisch))              return false;   // 0.80 liegt drin
        if (fixedUp.Test(alt))                  return false;   // 0.05 liegt darunter

        // Ein Bereich ohne beide Grenzen schraenkt nichts ein und wird
        // verworfen - der Selektor bleibt trotzdem gueltig.
        ChefZ_Selector offen = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        offen.wetness = new ChefZ_Range();
        ChefZ_CompiledSelector offenNode = ChefZ_SelectorCompiler.Compile(offen, ctx, error);
        if (!offenNode)                         return false;
        if (offenNode.ranges)                   return false;
        if (!offenNode.Test(alt))               return false;

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
        if (!node)                              return false;
        if (!node.acceptedQualities)            return false;
        if (node.acceptedQualities.Count() != 2) return false;   // MITTE und HOCH
        if (node.minQualityRank != 1)           return false;

        if (!node.Test(hoch))                   return false;
        if (!node.Test(mitte))                  return false;
        if (node.Test(niedrig))                 return false;
        if (node.Test(ohne))                    return false;    // keine Qualitaet = nicht genug

        // Unbekannte Stufe weist ab.
        ChefZ_Selector bad = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        bad.minQuality = "CHEFZ_MT_Q_GIBTESNICHT";
        if (ChefZ_SelectorCompiler.Compile(bad, ctx, error))    return false;

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

        ChefZ_CompiledSelector cls =
            ChefZ_SelectorCompiler.Compile(Leaf("cls", "CHEFZ_MT_KLASSE_A"), ctx, error);
        if (cls.specificity != w.wClass)        return false;

        // Tiefere Kategorie ist spezifischer - ohne dass ein Autor eine Zahl
        // pflegt (09 E3).
        ChefZ_CompiledSelector flach =
            ChefZ_SelectorCompiler.Compile(Leaf("category", "CHEFZ_MT_KAT_TIER"), ctx, error);
        ChefZ_CompiledSelector tief =
            ChefZ_SelectorCompiler.Compile(Leaf("category", "CHEFZ_MT_KAT_WILD"), ctx, error);
        if (flach.specificity != w.wCategoryBase)                           return false;
        if (tief.specificity != w.wCategoryBase + w.wCategoryPerDepth)      return false;
        if (tief.specificity <= flach.specificity)                          return false;

        // anyOf nimmt das MINIMUM: "anyOf [exakte Klasse, breite Kategorie]"
        // ist nicht spezifischer als die breite Kategorie allein (09 §4.1).
        ChefZ_Selector anySel = new ChefZ_Selector();
        anySel.anyOf = new array<ref ChefZ_Selector>();
        anySel.anyOf.Insert(Leaf("cls", "CHEFZ_MT_KLASSE_A"));
        anySel.anyOf.Insert(Leaf("category", "CHEFZ_MT_KAT_TIER"));
        ChefZ_CompiledSelector any = ChefZ_SelectorCompiler.Compile(anySel, ctx, error);
        if (any.specificity != flach.specificity)                           return false;

        // allOf summiert.
        ChefZ_Selector allSel = new ChefZ_Selector();
        allSel.allOf = new array<ref ChefZ_Selector>();
        allSel.allOf.Insert(Leaf("category", "CHEFZ_MT_KAT_TIER"));
        allSel.allOf.Insert(Leaf("state", "CHEFZ_MT_ZUSTAND_A"));
        ChefZ_CompiledSelector all = ChefZ_SelectorCompiler.Compile(allSel, ctx, error);
        if (all.specificity != w.wCategoryBase + w.wState)                  return false;

        // Jeder gebundene Bereich zaehlt.
        ChefZ_Selector ranged = Leaf("category", "CHEFZ_MT_KAT_TIER");
        ranged.health = new ChefZ_Range();
        ranged.health.min = 0.5;
        ChefZ_CompiledSelector withRange = ChefZ_SelectorCompiler.Compile(ranged, ctx, error);
        if (withRange.specificity != w.wCategoryBase + w.wRangePerBound)    return false;

        // selectivityHint: exakte Klasse ist am engsten.
        if (cls.selectivityHint != 1)                                       return false;
        if (tief.selectivityHint != 2)                                      return false;   // KAT_WILD
        if (flach.selectivityHint != 4)                                     return false;   // KAT_TIER

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
        if (!slot)                                                  return false;
        if (slot.minCount != 1)                                     return false;
        if (slot.maxCount != 1)                                     return false;
        if (slot.optional)                                          return false;
        if (!slot.allowPartial)                                     return false;
        if (slot.consumeMode != ChefZ_ConsumeMode.WHOLE)            return false;
        if (slot.excludeStates.Count() != 1)                        return false;
        if (!slot.IsStateExcluded(ChefZ_SymbolTable.Lookup("CHEFZ_MT_ZUSTAND_WEG")))
            return false;

        // Ausdrueckliche Freigabe: [] bleibt leer (07 E5).
        ChefZ_SlotDef frei = new ChefZ_SlotDef();
        frei.slotId        = "b";
        frei.match         = Leaf("category", "CHEFZ_MT_KAT_TIER");
        frei.excludeStates = new array<string>();
        ChefZ_CompiledSlot freiSlot = ChefZ_SelectorCompiler.CompileSlot(frei, 1, ctx, error);
        if (!freiSlot)                                              return false;
        if (freiSlot.excludeStates.Count() != 0)                    return false;

        // Nur Tippfehler in excludeStates -> Abweisung, sonst waere der Filter
        // still abgeschaltet (07 §7).
        ChefZ_SlotDef vertippt = new ChefZ_SlotDef();
        vertippt.slotId        = "c";
        vertippt.match         = Leaf("category", "CHEFZ_MT_KAT_TIER");
        vertippt.excludeStates = new array<string>();
        vertippt.excludeStates.Insert("CHEFZ_MT_VERTIPPT");
        if (ChefZ_SelectorCompiler.CompileSlot(vertippt, 2, ctx, error))
            return false;

        // minCount > maxCount wird geklammert, nicht abgewiesen.
        ChefZ_SlotDef schief = new ChefZ_SlotDef();
        schief.slotId   = "d";
        schief.match    = Leaf("category", "CHEFZ_MT_KAT_TIER");
        schief.minCount = 3;
        schief.maxCount = 1;
        ChefZ_CompiledSlot schiefSlot = ChefZ_SelectorCompiler.CompileSlot(schief, 3, ctx, error);
        if (!schiefSlot)                                            return false;
        if (schiefSlot.maxCount != 3)                               return false;

        // amount <= 0 wird auf 1 gesetzt.
        ChefZ_SlotDef nullAmount = new ChefZ_SlotDef();
        nullAmount.slotId = "e";
        nullAmount.match  = Leaf("category", "CHEFZ_MT_KAT_TIER");
        nullAmount.amount = new ChefZ_Range();
        nullAmount.amount.min = 0.0;
        ChefZ_CompiledSlot nullSlot = ChefZ_SelectorCompiler.CompileSlot(nullAmount, 4, ctx, error);
        if (!nullSlot)                                              return false;
        if (nullSlot.RequiredUnits() != 1.0)                        return false;

        // Slot ohne match ist ein Fehler.
        ChefZ_SlotDef leer = new ChefZ_SlotDef();
        leer.slotId = "f";
        if (ChefZ_SelectorCompiler.CompileSlot(leer, 5, ctx, error))
            return false;

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
        if (!slot)                                                  return false;

        array<ref ChefZ_CompiledSlot> slots = new array<ref ChefZ_CompiledSlot>();
        slots.Insert(slot);

        // Zwei Items mit je einer Einheit erfuellen "min 2" gemeinsam.
        ChefZ_BindResult result;
        if (!ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_MENGE", null, result))
            return false;
        if (result.bindings.Get(0).Count() != 2)                    return false;
        if (result.bindings.Get(0).totalUnits != 2.0)               return false;

        // Mit allowPartial = false muss ein einzelnes Item die Menge tragen.
        def.allowPartial = false;
        def.MarkExplicit("allowPartial");
        ChefZ_CompiledSlot ganz = ChefZ_SelectorCompiler.CompileSlot(def, 0, ctx, error);
        if (!ganz)                                                  return false;
        if (ganz.allowPartial)                                      return false;

        array<ref ChefZ_CompiledSlot> ganzSlots = new array<ref ChefZ_CompiledSlot>();
        ganzSlots.Insert(ganz);
        if (ChefZ_Matcher.Bind(ganzSlots, snap, 4096, "CHEFZ_MT_GANZ", null, result))
            return false;

        // Falsche Einheit -> kein Kandidat (05 §6, der Core rechnet nie um).
        halb1.quantityUnit = ChefZ_SymbolTable.Intern("CHEFZ_MT_ANDERE_EINHEIT");
        halb2.quantityUnit = ChefZ_SymbolTable.Intern("CHEFZ_MT_ANDERE_EINHEIT");
        if (ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_EINHEIT", null, result))
            return false;

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
        if (plan.Count() != 1)                          return false;
        if (!plan.Get(0).destroyWhole)                  return false;
        if (plan.Get(0).handle != 7)                    return false;

        // amount: anteilig abziehen, Rest bleibt liegen.
        ChefZ_SlotDef amountDef = new ChefZ_SlotDef();
        amountDef.slotId        = "teil";
        amountDef.match         = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        amountDef.consume       = "amount";
        amountDef.consumeAmount = 1.0;
        ChefZ_CompiledSlot amountSlot = ChefZ_SelectorCompiler.CompileSlot(amountDef, 1, ctx, error);
        if (!amountSlot)                                return false;
        if (amountSlot.consumeMode != ChefZ_ConsumeMode.AMOUNT)     return false;

        array<ref ChefZ_ConsumePlan> plan2 = new array<ref ChefZ_ConsumePlan>();
        ChefZ_SlotEvaluator.BuildConsumePlanIdx(amountSlot, snap, assigned, plan2);
        if (plan2.Count() != 1)                         return false;
        if (plan2.Get(0).destroyWhole)                  return false;
        if (plan2.Get(0).unitsDelta != 1.0)             return false;
        if (plan2.Get(0).quantityDelta != 25.0)         return false;

        // amount groesser als vorhanden: Item ist leer und wird geloescht.
        amountDef.consumeAmount = 99.0;
        ChefZ_CompiledSlot leerSlot = ChefZ_SelectorCompiler.CompileSlot(amountDef, 2, ctx, error);
        array<ref ChefZ_ConsumePlan> plan3 = new array<ref ChefZ_ConsumePlan>();
        ChefZ_SlotEvaluator.BuildConsumePlanIdx(leerSlot, snap, assigned, plan3);
        if (plan3.Count() != 1)                         return false;
        if (!plan3.Get(0).destroyWhole)                 return false;

        // none + setStateAfter: nichts verbraucht, Zustand gewechselt.
        ChefZ_SlotDef noneDef = new ChefZ_SlotDef();
        noneDef.slotId        = "werkzeug";
        noneDef.match         = Leaf("category", "CHEFZ_MT_KAT_GRUEN");
        noneDef.consume       = "none";
        noneDef.setStateAfter = "CHEFZ_MT_ZUSTAND_B";
        ChefZ_CompiledSlot noneSlot = ChefZ_SelectorCompiler.CompileSlot(noneDef, 3, ctx, error);
        if (!noneSlot)                                  return false;

        array<ref ChefZ_ConsumePlan> plan4 = new array<ref ChefZ_ConsumePlan>();
        ChefZ_SlotEvaluator.BuildConsumePlanIdx(noneSlot, snap, assigned, plan4);
        if (plan4.Count() != 1)                         return false;
        if (plan4.Get(0).destroyWhole)                  return false;
        if (plan4.Get(0).quantityDelta != 0.0)          return false;
        if (plan4.Get(0).setStateAfter != ChefZ_SymbolTable.Lookup("CHEFZ_MT_ZUSTAND_B"))
            return false;

        // Unbekannter Zielzustand weist ab.
        noneDef.setStateAfter = "CHEFZ_MT_ZUSTAND_GIBTESNICHT";
        if (ChefZ_SelectorCompiler.CompileSlot(noneDef, 4, ctx, error))
            return false;

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
        if (!slots.Get(0) || !slots.Get(1))                     return false;

        ChefZ_BindResult result;
        if (!ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_BIND", null, result))
            return false;

        // Bindung in DEKLARATIONSreihenfolge (07 §4, Schritt 6).
        if (result.bindings.Count() != 2)                       return false;
        if (result.bindings.Get(0).slotId != "fleisch")         return false;
        if (result.bindings.Get(0).handles.Get(0) != 10)        return false;
        if (result.bindings.Get(1).handles.Get(0) != 11)        return false;

        // Der Rest bleibt uebrig und geht an die Policy des Rezepts.
        if (result.extraHandles.Count() != 1)                   return false;
        if (result.extraHandles.Get(0) != 12)                   return false;
        if (result.BoundItemCount() != 2)                       return false;

        // Der Snapshot bleibt unmarkiert zurueck - der Matcher schleppt
        // nichts mit (07 §6).
        for (int i = 0; i < snap.Count(); i++)
        {
            if (snap.Get(i).slotBoundTo != -1)                  return false;
        }

        // Determinismus: zweimal dasselbe Ergebnis.
        ChefZ_BindResult second;
        if (!ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_BIND", null, second))
            return false;
        if (second.bindings.Get(0).handles.Get(0) != 10)        return false;
        if (second.bindings.Get(1).handles.Get(0) != 11)        return false;

        // Fehlende Zutat -> kein Treffer, mit Begruendung.
        array<ref ChefZ_CompiledSlot> unmoeglich = new array<ref ChefZ_CompiledSlot>();
        unmoeglich.Insert(MakeSlot(ctx, "pilz", Leaf("category", "CHEFZ_MT_KAT_PILZ"), 1, 1, 0));
        ChefZ_BindResult miss;
        if (ChefZ_Matcher.Bind(unmoeglich, snap, 4096, "CHEFZ_MT_MISS", null, miss))
            return false;
        if (miss.failSlotId != "pilz")                          return false;
        if (miss.failReason == "")                              return false;

        // EIN Item bedient hoechstens EINEN Slot - sonst waere es ein
        // Duplikations-Exploit (07 §4).
        array<ref ChefZ_CompiledSlot> zweimal = new array<ref ChefZ_CompiledSlot>();
        zweimal.Insert(MakeSlot(ctx, "t1", Leaf("category", "CHEFZ_MT_KAT_TIER"), 1, 1, 0));
        zweimal.Insert(MakeSlot(ctx, "t2", Leaf("category", "CHEFZ_MT_KAT_TIER"), 1, 1, 1));
        ChefZ_BindResult doppelt;
        if (ChefZ_Matcher.Bind(zweimal, snap, 4096, "CHEFZ_MT_DOPPELT", null, doppelt))
            return false;

        // Ausgeschlossener Zustand nimmt das Item aus dem Rennen (07 E5).
        tier.chefzState = ChefZ_SymbolTable.Intern("CHEFZ_MT_ZUSTAND_WEG");
        ChefZ_BindResult verdorben;
        if (ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_VERDORBEN", null, verdorben))
            return false;
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
        if (!optSlot)                                           return false;
        if (!optSlot.optional)                                  return false;

        slots.Insert(optSlot);
        ChefZ_BindResult mitOption;
        if (!ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_OPTION", null, mitOption))
            return false;
        if (mitOption.bindings.Count() != 3)                    return false;
        if (!mitOption.bindings.Get(2).filled)                  return false;
        if (mitOption.gradePoints != 2)                         return false;
        if (mitOption.extraHandles.Count() != 0)                return false;

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
        kraut.anyOf = new array<ref ChefZ_Selector>();
        kraut.anyOf.Insert(Leaf("cls", "CHEFZ_MT_KLASSE_D"));
        kraut.anyOf.Insert(Leaf("cls", "CHEFZ_MT_FEHLT"));

        array<ref ChefZ_CompiledSlot> slots = new array<ref ChefZ_CompiledSlot>();
        slots.Insert(MakeSlot(ctx, "wild",    Leaf("category", "CHEFZ_MT_KAT_WILD"), 1, 1, 0));
        slots.Insert(MakeSlot(ctx, "gemuese", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 2, 2, 1));
        slots.Insert(MakeSlot(ctx, "kraut",   kraut, 1, 1, 2));
        if (!slots.Get(0) || !slots.Get(1) || !slots.Get(2))    return false;

        ChefZ_BindResult result;
        if (!ChefZ_Matcher.Bind(slots, snap, 4096, "CHEFZ_MT_BACKTRACK", null, result))
            return false;

        // Die einzige gueltige Gesamtzuordnung: A -> wild, B+C -> gemuese,
        // D -> kraut. Ein gieriger Matcher, der "gemuese" zuerst bedient und
        // dabei A greift, findet sie nicht.
        if (result.bindings.Get(0).handles.Count() != 1)        return false;
        if (result.bindings.Get(0).handles.Get(0) != 20)        return false;
        if (result.bindings.Get(1).handles.Count() != 2)        return false;
        if (result.bindings.Get(1).handles.Find(21) < 0)        return false;
        if (result.bindings.Get(1).handles.Find(22) < 0)        return false;
        if (result.bindings.Get(1).handles.Find(20) >= 0)       return false;
        if (result.bindings.Get(2).handles.Get(0) != 23)        return false;
        if (result.extraHandles.Count() != 0)                   return false;

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
        entweder.anyOf = new array<ref ChefZ_Selector>();
        entweder.anyOf.Insert(Leaf("cls", "CHEFZ_MT_KLASSE_A"));
        entweder.anyOf.Insert(Leaf("cls", "CHEFZ_MT_KLASSE_B"));

        array<ref ChefZ_CompiledSlot> slots2 = new array<ref ChefZ_CompiledSlot>();
        slots2.Insert(MakeSlot(ctx, "eins", entweder, 1, 1, 0));
        slots2.Insert(MakeSlot(ctx, "zwei", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 2, 2, 1));

        ChefZ_BindResult result2;
        if (!ChefZ_Matcher.Bind(slots2, snap2, 4096, "CHEFZ_MT_BACKTRACK2", null, result2))
            return false;
        if (result2.bindings.Get(0).handles.Get(0) != 31)       return false;   // B, nicht A
        if (result2.bindings.Get(1).handles.Count() != 2)       return false;

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
            return false;
        if (ok.budgetExhausted)                                 return false;

        // Mit Budget 1 endet schon der erste Bindungsversuch - kein Treffer,
        // Flagge gesetzt, nichts veraendert.
        array<ref ChefZ_CompiledSlot> zwei = new array<ref ChefZ_CompiledSlot>();
        zwei.Insert(MakeSlot(ctx, "a", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 2, 2, 0));
        zwei.Insert(MakeSlot(ctx, "b", Leaf("category", "CHEFZ_MT_KAT_GRUEN"), 2, 2, 1));

        ChefZ_Matcher.SetQuietForTest(true);
        ChefZ_BindResult knapp;
        bool matched = ChefZ_Matcher.Bind(zwei, snap, 1, "CHEFZ_MT_BUDGET", null, knapp);
        ChefZ_Matcher.SetQuietForTest(false);

        if (matched)                                            return false;
        if (!knapp.budgetExhausted)                             return false;
        if (knapp.failReason == "")                             return false;
        if (knapp.nodesExplored < 1)                            return false;

        // Mit ausreichendem Budget bindet derselbe Fall.
        ChefZ_BindResult reich;
        if (!ChefZ_Matcher.Bind(zwei, snap, 4096, "CHEFZ_MT_BUDGET2", null, reich))
            return false;
        if (reich.budgetExhausted)                              return false;
        if (reich.BoundItemCount() != 4)                        return false;

        return true;
    }
}
