//==============================================================================
// ChefZ_RecipeSelfTest - Abnahmepruefung fuer S6
//
// Entwurf: 19 §3, S6 - "Fertig, wenn: Testrezepte laden; EvaluateBest auf
// einem Faktensatz liefert das spezifischste Rezept; das §16-Beispiel aus
// 09 §4.5 ergibt die dort gerechnete Reihenfolge; zwei ununterscheidbare
// Rezepte werden beim Boot gemeldet."
//
// Genau diese vier Punkte prueft diese Datei, dazu die Fehlerfaelle aus
// 08 §8, die ohne laufendes Spiel pruefbar sind.
//
// ---------------------------------------------------------------------------
// Wie das §4.5-Beispiel hier abgebildet ist - und was daran abweicht
// ---------------------------------------------------------------------------
// Die STRUKTUR der Tabelle aus 09 §4.5 wird nachgebaut, nicht ihr Vokabular:
// der Core darf keine Zutat und keine Kategorie kennen (Invariante I3). Aus
// "Meat Stew (MEAT + VEGETABLE)" wird "R1 (KAT_A + KAT_C)".
//
// Die SPEZIFITAETSSPALTE wird auf die Nachkommastelle geprueft: 2.0, 3.0,
// 5.0, 8.25. Das ist die Zahl, die 09 §4.1 vorschreibt, und sie haengt an
// nichts als am Rezept.
//
// Bei der ABDECKUNGSSPALTE weicht der Test bewusst ab, und der Grund gehoert
// ausgesprochen: 09 §4.5 setzt dort eine Bindung voraus, die so viele Items
// wie moeglich verbraucht ("Meat Stew ... 3/5"), waehrend 07 §4 fuer den
// Matcher ausdruecklich die KLEINSTE ausreichende Auswahl festlegt. Beides
// zugleich geht nicht. Der Test rechnet die Abdeckung deshalb aus der
// TATSAECHLICHEN Bindung und prueft die Formel aus 09 §4.2 statt der
// Beispielzahlen.
//
// Fuer die beiden Zeilen, auf die es ankommt, fallen beide Lesarten ohnehin
// zusammen: R3 bindet 4 von 4 Items und kommt damit auf 5.00 + 0.50 = 5.50,
// R4 bindet 5 von 5 und kommt auf 8.25 + 0.50 = 8.75 - beides exakt die
// Zahlen aus der Tabelle.
//
// KEIN CONTENT: alle Namen tragen das Praefix "CHEFZ_RT_" und sind abstrakt.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_RecipeSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    //--- Bitindizes des Testkategoriebaums -----------------------------------
    //   KAT_A (0)            "Fleisch"
    //     +- KAT_B (1)       "Wildfleisch"
    //   KAT_C (2)            "Gemuese"
    //     +- KAT_D (3)       "Wurzelgemuese"
    //   KAT_E (4)            "Pilz"
    //   KAT_F (5)            "Kraut"
    static const int BIT_A = 0;
    static const int BIT_B = 1;
    static const int BIT_C = 2;
    static const int BIT_D = 3;
    static const int BIT_E = 4;
    static const int BIT_F = 5;

    static const string KAT_A = "CHEFZ_RT_KAT_A";
    static const string KAT_B = "CHEFZ_RT_KAT_B";
    static const string KAT_C = "CHEFZ_RT_KAT_C";
    static const string KAT_D = "CHEFZ_RT_KAT_D";
    static const string KAT_E = "CHEFZ_RT_KAT_E";
    static const string KAT_F = "CHEFZ_RT_KAT_F";

    static const string CLS_SPEZIAL = "CHEFZ_RT_KLASSE_SPEZIAL";
    static const string GERAET      = "CHEFZ_RT_GERAET";
    static const string GERAETEKAT  = "CHEFZ_RT_GERAETEKAT";
    static const string ERGEBNIS    = "CHEFZ_RT_ERGEBNIS";

    //==========================================================================

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("Rezeptmodell",   ChefZ_RecipeDef.SelfCheck());
        Check("Kochkontext",    ChefZ_CookContext.SelfCheck());
        Check("Ergebnis",       ChefZ_MatchResult.SelfCheck());
        Check("Textlisten",     ChefZ_TextList.SelfCheck());
        Check("Gefaessschluessel", ChefZ_VesselKeys.SelfCheck());
        Check("Rangordnung",    ChefZ_RecipeRank.SelfCheck());
        Check("Gewichte",       ChefZ_PriorityWeightsDef.SelfCheck());
        Check("Compiler",       TestCompilerRejections());
        Check("Spezifitaet",    TestSpecificityTable());
        Check("Index",          TestIndex());
        Check("Auswahl",        TestEvaluateBest());
        Check("Reihenfolge",    TestEvaluateAll());
        Check("Fremdkoerper",   TestExtraItems());
        Check("Abschluss",      TestCompletion());
        Check("Ambiguitaet",    TestAmbiguities());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++; ChefZ_SelfTestTrace.Reset();
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.MATCH, "Selbsttest S6 " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.MATCH, "Selbsttest S6 " + name + " FEHLGESCHLAGEN. Die Recipe Engine verhaelt sich " + "nicht wie entworfen - welches Gericht ein Kessel ergibt, ist damit " + "unzuverlaessig." + ChefZ_SelfTestTrace.Take());
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S6: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
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

    /**
     * Kompilierkontext mit Testkategorien und TESTGEWICHTEN.
     *
     * Die Kontext-, Policy-, Werkzeug- und Bereichsgewichte stehen auf 0. Das
     * ist kein Trick, sondern die Voraussetzung dafuer, die Tabelle aus
     * 09 §4.5 ueberhaupt nachrechnen zu koennen: sie zeigt ausschliesslich die
     * Slot- und Capability-Terme. Mit den Defaults kaeme zu jeder Zeile
     * derselbe Policy- und Kontextbonus hinzu - die Reihenfolge bliebe
     * dieselbe, die Zahlen waeren andere.
     *
     * Dass die Reihenfolge auch mit den ECHTEN Defaults stimmt, prueft
     * TestSpecificityTable() eigens.
     */
    private static ChefZ_CompileContext MakeContextWith(ChefZ_LoadReport report, bool tableWeights)
    {
        ChefZ_CompileContext ctx = new ChefZ_CompileContext();
        ctx.Init(report);
        ctx.SetSubject("selftest", "CHEFZ_RT");

        ChefZ_SymbolResolver res = new ChefZ_SymbolResolver();
        res.DefineCategory(KAT_A, BIT_A, 0, 4);
        res.DefineCategory(KAT_B, BIT_B, 1, 2);
        res.DefineCategory(KAT_C, BIT_C, 0, 6);
        res.DefineCategory(KAT_D, BIT_D, 1, 3);
        res.DefineCategory(KAT_E, BIT_E, 0, 1);
        res.DefineCategory(KAT_F, BIT_F, 0, 2);
        res.DefineClass(CLS_SPEZIAL);
        ctx.SetResolver(res);

        ChefZ_PriorityWeights w = new ChefZ_PriorityWeights();
        if (tableWeights)
        {
            w.wContextDeviceClass = 0.0;
            w.wContextBound       = 0.0;
            w.wPolicyForbid       = 0.0;
            w.wPolicyPerState     = 0.0;
            w.wToolGroup          = 0.0;
        }
        ctx.SetWeights(w);

        return ctx;
    }

    private static ChefZ_RecipeCompiler MakeCompiler(ChefZ_CompileContext ctx, ChefZ_LoadReport report)
    {
        ChefZ_RecipeCompiler compiler = new ChefZ_RecipeCompiler();
        compiler.Init(ctx, report, null);

        // Ohne diese Zeile wuerde JEDES Testrezept an der CfgVehicles-Pruefung
        // scheitern: "CHEFZ_RT_ERGEBNIS" ist absichtlich keine echte Klasse,
        // und der Core darf keine anlegen (Invariante I3).
        compiler.SetVerifyClasses(false);
        return compiler;
    }

    //--------------------------------------------------------------------------
    // Bausteine
    //--------------------------------------------------------------------------

    private static ChefZ_Selector CategorySelector(string category)
    {
        ChefZ_Selector sel = new ChefZ_Selector();
        sel.category = category;
        return sel;
    }

    private static ChefZ_Selector ClassSelector(string cls)
    {
        ChefZ_Selector sel = new ChefZ_Selector();
        sel.cls = cls;
        return sel;
    }

    private static ChefZ_SlotDef Slot(string slotId, ChefZ_Selector match)
    {
        ChefZ_SlotDef slot = new ChefZ_SlotDef();
        slot.slotId = slotId;
        slot.match  = match;
        return slot;
    }

    /**
     * Ein Testrezept mit n Kategorieslots.
     *
     * extraItems wird ausdruecklich uebergeben, weil der Default "forbid" das
     * Ergebnis der halben Testtabelle bestimmt (08 E2) - ihn hier implizit zu
     * lassen hiesse, den wichtigsten Schalter des Tests zu verstecken.
     */
    private static ChefZ_RecipeDef MakeRecipe(string id, string extraItems)
    {
        ChefZ_RecipeDef def = new ChefZ_RecipeDef();
        def.id    = id;
        def.slots = new array<ref ChefZ_SlotDef>();

        def.contexts = new array<ref ChefZ_ContextRule>();
        ChefZ_ContextRule rule = new ChefZ_ContextRule();
        rule.deviceCategories = new array<string>();
        rule.deviceCategories.Insert(GERAETEKAT);
        def.contexts.Insert(rule);

        def.outputs = new array<ref ChefZ_OutputDef>();
        ChefZ_OutputDef output = new ChefZ_OutputDef();
        output.cls = ERGEBNIS;
        def.outputs.Insert(output);

        def.policy = new ChefZ_RecipePolicy();
        def.policy.extraItems = extraItems;

        // completion bleibt leer -> ON_STAGE mit Default-doneStages. Genau der
        // Normalfall aus 08 E5.
        return def;
    }

    private static void AddCategorySlot(notnull ChefZ_RecipeDef def, string slotId, string category)
    {
        def.slots.Insert(Slot(slotId, CategorySelector(category)));
    }

    private static ChefZ_CompiledRecipe CompileOne(notnull ChefZ_RecipeCompiler compiler, notnull ChefZ_RecipeDef def)
    {
        def.Normalize();
        def.ResolveDefaults();
        return compiler.Compile(def);
    }

    //--------------------------------------------------------------------------
    // Die vier Rezepte der Tabelle aus 09 §4.5
    //--------------------------------------------------------------------------

    //! R1: KAT_A + KAT_C            -> 1.0 + 1.0 = 2.00
    private static ChefZ_RecipeDef Recipe1(string extraItems)
    {
        ChefZ_RecipeDef def = MakeRecipe("CHEFZ_RT_R1", extraItems);
        AddCategorySlot(def, "s1", KAT_A);
        AddCategorySlot(def, "s2", KAT_C);
        return def;
    }

    //! R2: KAT_E + KAT_C + KAT_F    -> 3 x 1.0 = 3.00
    private static ChefZ_RecipeDef Recipe2(string extraItems)
    {
        ChefZ_RecipeDef def = MakeRecipe("CHEFZ_RT_R2", extraItems);
        AddCategorySlot(def, "s1", KAT_E);
        AddCategorySlot(def, "s2", KAT_C);
        AddCategorySlot(def, "s3", KAT_F);
        return def;
    }

    //! R3: KAT_B(1) + KAT_D(1) + KAT_E + KAT_F -> 1.5 + 1.5 + 1.0 + 1.0 = 5.00
    private static ChefZ_RecipeDef Recipe3(string extraItems)
    {
        ChefZ_RecipeDef def = MakeRecipe("CHEFZ_RT_R3", extraItems);
        AddCategorySlot(def, "s1", KAT_B);
        AddCategorySlot(def, "s2", KAT_D);
        AddCategorySlot(def, "s3", KAT_E);
        AddCategorySlot(def, "s4", KAT_F);
        return def;
    }

    //! R4: wie R3 + exakte Klasse (3.0) + eine Faehigkeit (0.25) -> 8.25
    private static ChefZ_RecipeDef Recipe4(string extraItems)
    {
        ChefZ_RecipeDef def = Recipe3(extraItems);
        def.id = "CHEFZ_RT_R4";
        def.slots.Insert(Slot("s5", ClassSelector(CLS_SPEZIAL)));

        def.requires = new array<ref ChefZ_CapabilityReq>();
        ChefZ_CapabilityReq req = new ChefZ_CapabilityReq();
        req.capability = "CHEFZ_RT_CAP";
        def.requires.Insert(req);
        return def;
    }

    //--------------------------------------------------------------------------
    // Faktenlisten
    //--------------------------------------------------------------------------

    private static ChefZ_ItemFacts AddItem(notnull ChefZ_FactSnapshot snap, int handle, string className, int bitA, int bitB)
    {
        ChefZ_ItemFacts facts = snap.Acquire();
        facts.handle      = handle;
        facts.classSym    = ChefZ_SymbolTable.Intern(className);
        facts.quantity    = 1.0;
        facts.quantityMax = 1.0;
        facts.units       = 1.0;
        facts.health01    = 1.0;
        facts.freshness01 = 1.0;
        facts.vanillaFoodStage = ChefZ_VanillaStage.FromName("Boiled");
        facts.closure.SetBit(bitA);
        if (bitB >= 0)
            facts.closure.SetBit(bitB);
        return facts;
    }

    /**
     * Vier Items, die R3 vollstaendig bedienen:
     *   #1 KAT_B unter KAT_A   #2 KAT_D unter KAT_C   #3 KAT_E   #4 KAT_F
     *
     * Die Elternbits stehen mit drin, weil der Ingredient Manager die Closure
     * transitiv fuellt (04 E1) - ein Item in einer Unterkategorie liegt auch in
     * ihrer Elternkategorie.
     */
    private static ChefZ_FactSnapshot MakeSnapshot(bool withSpecial)
    {
        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        AddItem(snap, 1, "CHEFZ_RT_ITEM_1", BIT_B, BIT_A);
        AddItem(snap, 2, "CHEFZ_RT_ITEM_2", BIT_D, BIT_C);
        AddItem(snap, 3, "CHEFZ_RT_ITEM_3", BIT_E, -1);
        AddItem(snap, 4, "CHEFZ_RT_ITEM_4", BIT_F, -1);
        if (withSpecial)
        {
            // BIT_A und nicht BIT_F: das Spezialitem darf KEINEN Slot von R3
            // bedienen koennen. Laege es in KAT_F, haetten zwei Items denselben
            // Slot bedienen koennen, und WELCHES der Matcher bindet, haenge an
            // der Internierungsreihenfolge der Symbole - der Test wuerde dann
            // je nach Startzustand der Symboltabelle etwas anderes messen.
            AddItem(snap, 5, CLS_SPEZIAL, BIT_A, -1);
        }
        snap.SortStable();
        return snap;
    }

    private static ChefZ_CookContext MakeCookContext()
    {
        ChefZ_CookContext ctx = new ChefZ_CookContext();
        ctx.deviceClass = ChefZ_SymbolTable.Intern(GERAET);
        ctx.AddDeviceCategory(ChefZ_SymbolTable.Intern(GERAETEKAT));
        return ctx;
    }

    private static ChefZ_Registry<ChefZ_DeviceDef> MakeDeviceRegistry()
    {
        ChefZ_Registry<ChefZ_DeviceDef> devices = new ChefZ_Registry<ChefZ_DeviceDef>();
        devices.Init(ChefZ_RecordKind.DEVICE);

        ChefZ_DeviceDef device = new ChefZ_DeviceDef();
        device.id  = GERAET;
        device.sym = ChefZ_SymbolTable.Intern(GERAET);
        device.deviceCategories = new array<string>();
        device.deviceCategories.Insert(GERAETEKAT);
        devices.Add(device);

        return devices;
    }

    private static void AddToRegistry(notnull ChefZ_Registry<ChefZ_RecipeDef> reg, notnull ChefZ_RecipeDef def)
    {
        def.Normalize();
        def.ResolveDefaults();
        def.sym = ChefZ_SymbolTable.Intern(def.id);
        reg.Add(def);
    }

    //==========================================================================
    // 1. Compiler: was abgewiesen werden MUSS (08 §8)
    //==========================================================================

    private static bool TestCompilerRejections()
    {
        ChefZ_LoadReport report = new ChefZ_LoadReport();
        report.SetMirrorToLog(false);
        ChefZ_CompileContext ctx = MakeContextWith(report, false);
        ChefZ_RecipeCompiler compiler = MakeCompiler(ctx, report);

        // Ein sauberes Rezept muss durchgehen - sonst prueft der Rest nichts.
        ChefZ_RecipeDef good = Recipe1(ChefZ_ExtraItemsMode.FORBID_NAME);
        if (!CompileOne(compiler, good)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 401, "!CompileOne(compiler, good)");

        // Unbekannte Kategorie -> Slot ungueltig -> GANZES Rezept abgewiesen
        // (07 §7, 08 §8).
        ChefZ_RecipeDef badCategory = MakeRecipe("CHEFZ_RT_BAD_KAT", "");
        AddCategorySlot(badCategory, "s1", "CHEFZ_RT_GIBTS_NICHT");
        if (CompileOne(compiler, badCategory)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 407, "CompileOne(compiler, badCategory)");

        // Nur optionale Slots -> wuerde bei jedem Inhalt zuenden.
        ChefZ_RecipeDef allOptional = MakeRecipe("CHEFZ_RT_NUR_OPTIONAL", "");
        ChefZ_SlotDef optionalSlot = Slot("s1", CategorySelector(KAT_A));
        optionalSlot.optional = true;
        optionalSlot.MarkExplicit("optional");
        allOptional.slots.Insert(optionalSlot);
        if (CompileOne(compiler, allOptional)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 415, "CompileOne(compiler, allOptional)");

        // TIMED ohne cookSeconds.
        ChefZ_RecipeDef timed = Recipe1("");
        timed.id         = "CHEFZ_RT_TIMED_OHNE_ZEIT";
        timed.completion = ChefZ_Completion.TIMED_NAME;
        if (CompileOne(compiler, timed)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 421, "CompileOne(compiler, timed)");

        // TIMED mit cookSeconds geht.
        ChefZ_RecipeDef timedOk = Recipe1("");
        timedOk.id          = "CHEFZ_RT_TIMED_OK";
        timedOk.completion  = ChefZ_Completion.TIMED_NAME;
        timedOk.cookSeconds = 30.0;
        ChefZ_CompiledRecipe timedRec = CompileOne(compiler, timedOk);
        if (!timedRec) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 429, "!timedRec");
        if (timedRec.completion != ChefZ_Completion.TIMED) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 430, "timedRec.completion != ChefZ_Completion.TIMED");

        // Unbekannte Vanilla-Endstufe -> abgewiesen.
        ChefZ_RecipeDef badStage = Recipe1("");
        badStage.id = "CHEFZ_RT_BAD_STAGE";
        badStage.doneStages = new array<string>();
        badStage.doneStages.Insert("Verkohlt");
        if (CompileOne(compiler, badStage)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 437, "CompileOne(compiler, badStage)");

        // Ergebnis ohne "cls" -> abgewiesen.
        ChefZ_RecipeDef badOutput = Recipe1("");
        badOutput.id = "CHEFZ_RT_BAD_OUT";
        badOutput.outputs.Get(0).cls = "";
        if (CompileOne(compiler, badOutput)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 443, "CompileOne(compiler, badOutput)");

        // priority ausserhalb [-1000, 1000] wird geklemmt, NICHT abgewiesen.
        ChefZ_RecipeDef loud = Recipe1("");
        loud.id       = "CHEFZ_RT_LAUT";
        loud.priority = 999999;
        ChefZ_CompiledRecipe loudRec = CompileOne(compiler, loud);
        if (!loudRec) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 450, "!loudRec");
        if (loudRec.priority != 1000) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 451, "loudRec.priority != 1000");

        // ON_STAGE ohne doneStages bekommt den Default (08 §8).
        ChefZ_CompiledRecipe defaults = CompileOne(compiler, Recipe2(""));
        if (!defaults) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 455, "!defaults");
        if (defaults.completion != ChefZ_Completion.ON_STAGE) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 456, "defaults.completion != ChefZ_Completion.ON_STAGE");
        if (defaults.doneStages.Count() != 3) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 457, "defaults.doneStages.Count() != 3");

        // Der Default fuer extraItems ist "forbid" (OF-03 / V-B §3).
        if (!defaults.policy) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 460, "!defaults.policy");
        if (defaults.policy.extraItemsMode != ChefZ_ExtraItemsMode.FORBID)
            return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 462, "defaults.policy.extraItemsMode != ChefZ_ExtraItemsMode.FORBID");

        // Jede Abweisung hat gemeldet. Ohne Meldung waere sie unauffindbar.
        if (report.ErrorCount() < 5) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 465, "report.ErrorCount() < 5");

        return true;
    }

    //==========================================================================
    // 2. Die Spezifitaetstabelle aus 09 §4.5
    //==========================================================================

    private static bool TestSpecificityTable()
    {
        ChefZ_LoadReport report = new ChefZ_LoadReport();
        report.SetMirrorToLog(false);
        ChefZ_CompileContext ctx = MakeContextWith(report, true);
        ChefZ_RecipeCompiler compiler = MakeCompiler(ctx, report);

        ChefZ_CompiledRecipe r1 = CompileOne(compiler, Recipe1(""));
        ChefZ_CompiledRecipe r2 = CompileOne(compiler, Recipe2(""));
        ChefZ_CompiledRecipe r3 = CompileOne(compiler, Recipe3(""));
        ChefZ_CompiledRecipe r4 = CompileOne(compiler, Recipe4(""));

        if (!r1 || !r2 || !r3 || !r4) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 486, "!r1 || !r2 || !r3 || !r4");

        if (!Near(r1.specificity, 2.00)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 488, "!Near(r1.specificity, 2.00)");
        if (!Near(r2.specificity, 3.00)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 489, "!Near(r2.specificity, 3.00)");
        if (!Near(r3.specificity, 5.00)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 490, "!Near(r3.specificity, 5.00)");
        if (!Near(r4.specificity, 8.25)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 491, "!Near(r4.specificity, 8.25)");

        // Die Reihenfolge muss auch mit den ECHTEN Defaults stimmen - sonst
        // waere die Tabelle oben nur eine Eigenschaft der Testgewichte.
        ChefZ_CompileContext realCtx = MakeContextWith(report, false);
        ChefZ_RecipeCompiler realCompiler = MakeCompiler(realCtx, report);

        ChefZ_CompiledRecipe d1 = CompileOne(realCompiler, Recipe1(""));
        ChefZ_CompiledRecipe d2 = CompileOne(realCompiler, Recipe2(""));
        ChefZ_CompiledRecipe d3 = CompileOne(realCompiler, Recipe3(""));
        ChefZ_CompiledRecipe d4 = CompileOne(realCompiler, Recipe4(""));
        if (!d1 || !d2 || !d3 || !d4) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 502, "!d1 || !d2 || !d3 || !d4");

        if (!(d1.specificity < d2.specificity)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 504, "!(d1.specificity < d2.specificity)");
        if (!(d2.specificity < d3.specificity)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 505, "!(d2.specificity < d3.specificity)");
        if (!(d3.specificity < d4.specificity)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 506, "!(d3.specificity < d4.specificity)");

        // amountCap deckelt den Mengenfaktor (09 §4.1): ein Slot mit minCount 8
        // zaehlt hoechstens dreifach.
        ChefZ_RecipeDef bulk = MakeRecipe("CHEFZ_RT_BULK", "");
        ChefZ_SlotDef bulkSlot = Slot("s1", CategorySelector(KAT_A));
        bulkSlot.minCount = 8;
        bulkSlot.maxCount = 8;
        bulk.slots.Insert(bulkSlot);
        ChefZ_CompiledRecipe bulkRec = CompileOne(compiler, bulk);
        if (!bulkRec) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 516, "!bulkRec");
        if (!Near(bulkRec.specificity, 3.00)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 517, "!Near(bulkRec.specificity, 3.00)");   // 1.0 * min(8,3)

        return true;
    }

    //==========================================================================
    // 3. Index und Vorfilter (08 §5.1)
    //==========================================================================

    private static bool TestIndex()
    {
        ChefZ_RecipeEngine engine = BuildEngine(true, false);
        if (!engine.IsReady()) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 529, "!engine.IsReady()");
        if (engine.GetRecipeCount() != 4) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 530, "engine.GetRecipeCount() != 4");

        ChefZ_Sym deviceSym  = ChefZ_SymbolTable.Intern(GERAET);
        ChefZ_Sym unknownSym = ChefZ_SymbolTable.Intern("CHEFZ_RT_FREMDGERAET");

        // Die Geraeteklasse steht nur ueber ihre KATEGORIE im Index - genau
        // das ist 08 E4, und genau deshalb ist HasAnyRecipeFor() trotzdem ein
        // einziger Lookup.
        if (!engine.HasAnyRecipeFor(deviceSym, ChefZ_SymbolTable.INVALID)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 538, "!engine.HasAnyRecipeFor(deviceSym, ChefZ_SymbolTable.INVALID)");
        if (engine.HasAnyRecipeFor(unknownSym, ChefZ_SymbolTable.INVALID)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 539, "engine.HasAnyRecipeFor(unknownSym, ChefZ_SymbolTable.INVALID)");

        // minItemCount ist das Minimum ueber alle Rezepte dieses Geraets - R1
        // hat zwei Pflichtslots.
        if (engine.GetMinItemCountFor(deviceSym) != 2) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 543, "engine.GetMinItemCountFor(deviceSym) != 2");

        // Rangreihenfolge: der Bestand liegt absteigend nach Spezifitaet.
        ChefZ_CompiledRecipe first = engine.GetRecipeAt(0);
        ChefZ_CompiledRecipe last  = engine.GetRecipeAt(3);
        if (!first || !last) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 548, "!first || !last");
        if (first.id != "CHEFZ_RT_R4") return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 549, "first.id != 'CHEFZ_RT_R4'");
        if (last.id != "CHEFZ_RT_R1") return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 550, "last.id != 'CHEFZ_RT_R1'");

        // Das Torsymbol: R3 verlangt vier Kategorien, die engste davon ist
        // KAT_E mit einer einzigen Klasse.
        ChefZ_CompiledRecipe r3 = engine.FindRecipe(ChefZ_SymbolTable.Intern("CHEFZ_RT_R3"));
        if (!r3) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 555, "!r3");
        if (r3.gateKind != ChefZ_GateKind.CATEGORY) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 556, "r3.gateKind != ChefZ_GateKind.CATEGORY");
        if (r3.gateBit != BIT_E) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 557, "r3.gateBit != BIT_E");

        // Ohne ein Item aus KAT_E kann R3 nicht binden - der Vorfilter muss
        // das ohne einen einzigen Matcher-Knoten erkennen.
        ChefZ_FactSnapshot thin = new ChefZ_FactSnapshot();
        AddItem(thin, 1, "CHEFZ_RT_ITEM_1", BIT_B, BIT_A);
        AddItem(thin, 2, "CHEFZ_RT_ITEM_2", BIT_D, BIT_C);
        thin.SortStable();

        ChefZ_VesselKeys keys = new ChefZ_VesselKeys();
        keys.Build(thin);
        if (keys.Admits(r3.gateKind, r3.gateSym, r3.gateBit)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 568, "keys.Admits(r3.gateKind, r3.gateSym, r3.gateBit)");

        return true;
    }

    //==========================================================================
    // 4. EvaluateBest - das spezifischste Rezept gewinnt
    //==========================================================================

    private static bool TestEvaluateBest()
    {
        // Ohne das Spezialitem kann R4 nicht binden; R1 und R2 duerfen
        // Fremdkoerper dulden, R3 nicht - also bindet R3 alle vier Items.
        ChefZ_RecipeEngine engine = BuildEngine(true, false);
        ChefZ_FactSnapshot snap   = MakeSnapshot(false);
        ChefZ_CookContext  cook   = MakeCookContext();

        ChefZ_MatchResult result;
        if (!engine.EvaluateBest(cook, snap, null, result))
        {
            // Die Engine weiss, WARUM sie nicht gebunden hat - ohne diese
            // Zeile ginge das in einem nackten false unter, und der naechste
            // Serverlauf sagte wieder nur "Auswahl FEHLGESCHLAGEN".
            ChefZ_Log.Warn(ChefZ_LogChannel.MATCH, "Selbsttest S6 Auswahl: EvaluateBest(R3, 4 Items) " + result.ToDebugString());
            return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 592, "");
        }
        if (result.recipeId != "CHEFZ_RT_R3") return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 594, "result.recipeId != 'CHEFZ_RT_R3'");
        if (result.boundItemCount != 4) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 595, "result.boundItemCount != 4");
        if (result.itemsInVessel != 4) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 596, "result.itemsInVessel != 4");
        if (result.extraHandles.Count() != 0) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 597, "result.extraHandles.Count() != 0");

        // 09 §4.5: 5.00 Spezifitaet + 0.50 voller Abdeckungsbonus = 5.50.
        if (!Near(result.score, 5.50)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 600, "!Near(result.score, 5.50)");

        // ON_STAGE mit Default-doneStages, alle Items sind "Boiled".
        if (!result.ready) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 603, "!result.ready");

        // Jeder Pflichtslot hat genau ein Item, und der Verbrauchsplan
        // umfasst sie alle.
        if (!result.IsSlotFilled("s1")) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 607, "!result.IsSlotFilled('s1')");
        if (!result.IsSlotFilled("s4")) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 608, "!result.IsSlotFilled('s4')");
        if (result.consumePlan.Count() != 4) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 609, "result.consumePlan.Count() != 4");

        // Mit dem Spezialitem gewinnt R4: 8.25 + 0.50 = 8.75.
        ChefZ_FactSnapshot rich = MakeSnapshot(true);
        ChefZ_MatchResult richResult;
        if (!engine.EvaluateBest(cook, rich, null, richResult)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 614, "!engine.EvaluateBest(cook, rich, null, richResult)");
        if (richResult.recipeId != "CHEFZ_RT_R4") return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 615, "richResult.recipeId != 'CHEFZ_RT_R4'");
        if (richResult.boundItemCount != 5) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 616, "richResult.boundItemCount != 5");
        if (!Near(richResult.score, 8.75)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 617, "!Near(richResult.score, 8.75)");

        // Fremdes Geraet: kein Kandidat, kein Treffer, und ausdruecklich kein
        // Fehler - Vanilla kocht weiter (08 §8).
        ChefZ_CookContext alien = new ChefZ_CookContext();
        alien.deviceClass = ChefZ_SymbolTable.Intern("CHEFZ_RT_FREMDGERAET");
        ChefZ_MatchResult none;
        if (engine.EvaluateBest(alien, snap, null, none)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 624, "engine.EvaluateBest(alien, snap, null, none)");
        if (none.matched) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 625, "none.matched");

        // Leeres Gefaess: dasselbe.
        ChefZ_FactSnapshot empty = new ChefZ_FactSnapshot();
        ChefZ_MatchResult nothing;
        if (engine.EvaluateBest(cook, empty, null, nothing)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 630, "engine.EvaluateBest(cook, empty, null, nothing)");

        return true;
    }

    //==========================================================================
    // 5. EvaluateAll - vollstaendige Reihenfolge
    //==========================================================================

    private static bool TestEvaluateAll()
    {
        ChefZ_RecipeEngine engine = BuildEngine(true, false);
        ChefZ_FactSnapshot snap   = MakeSnapshot(false);
        ChefZ_CookContext  cook   = MakeCookContext();

        array<ref ChefZ_MatchResult> results;
        int n = engine.EvaluateAll(cook, snap, results, 0);

        // R4 bindet nicht (kein Spezialitem), die anderen drei schon.
        if (n != 3) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 649, "n != 3");
        if (results.Get(0).recipeId != "CHEFZ_RT_R3") return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 650, "results.Get(0).recipeId != 'CHEFZ_RT_R3'");
        if (results.Get(1).recipeId != "CHEFZ_RT_R2") return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 651, "results.Get(1).recipeId != 'CHEFZ_RT_R2'");
        if (results.Get(2).recipeId != "CHEFZ_RT_R1") return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 652, "results.Get(2).recipeId != 'CHEFZ_RT_R1'");

        // Die Scores muessen absteigen - das ist die Aussage von 09 §4.2.
        if (!(results.Get(0).score > results.Get(1).score)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 655, "!(results.Get(0).score > results.Get(1).score)");
        if (!(results.Get(1).score > results.Get(2).score)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 656, "!(results.Get(1).score > results.Get(2).score)");

        // Deckelung.
        array<ref ChefZ_MatchResult> capped;
        if (engine.EvaluateAll(cook, snap, capped, 1) != 1) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 660, "engine.EvaluateAll(cook, snap, capped, 1) != 1");
        if (capped.Get(0).recipeId != "CHEFZ_RT_R3") return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 661, "capped.Get(0).recipeId != 'CHEFZ_RT_R3'");

        // Teilbericht: was fehlt R4 noch?
        ChefZ_PartialMatchReport partial;
        if (!engine.EvaluatePartial(cook, snap, ChefZ_SymbolTable.Intern("CHEFZ_RT_R4"), partial)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 665, "!engine.EvaluatePartial(cook, snap, ChefZ_SymbolTable.Intern('CHEFZ_RT_R4'), partial)");
        if (!partial.contextOk) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 666, "!partial.contextOk");
        if (partial.AllSlotsSatisfied()) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 667, "partial.AllSlotsSatisfied()");   // s5 fehlt

        return true;
    }

    //==========================================================================
    // 6. Fremdkoerper (08 E2, OF-03)
    //==========================================================================

    private static bool TestExtraItems()
    {
        // NUR R3, und R3 duldet nichts. Das Spezialitem bleibt ungebunden -
        // also kein Treffer, also Vanilla. Das ist der ganze Sinn von "forbid".
        ChefZ_RecipeEngine engine = BuildEngineWith(Recipe3(ChefZ_ExtraItemsMode.FORBID_NAME));
        ChefZ_CookContext  cook   = MakeCookContext();

        ChefZ_MatchResult blocked;
        if (engine.EvaluateBest(cook, MakeSnapshot(true), null, blocked)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 684, "engine.EvaluateBest(cook, MakeSnapshot(true), null, blocked)");
        if (blocked.matched) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 685, "blocked.matched");

        // Ohne den Fremdkoerper bindet dasselbe Rezept.
        ChefZ_MatchResult clean;
        if (!engine.EvaluateBest(cook, MakeSnapshot(false), null, clean))
        {
            ChefZ_Log.Warn(ChefZ_LogChannel.MATCH, "Selbsttest S6 Fremdkoerper: EvaluateBest(ohne Spezialitem) " + clean.ToDebugString());
            return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 692, "");
        }

        // Mit "ignore" bindet es auch mit Fremdkoerper - und laesst ihn liegen.
        ChefZ_RecipeEngine tolerant = BuildEngineWith(Recipe3(ChefZ_ExtraItemsMode.IGNORE_NAME));
        ChefZ_MatchResult tolerated;
        if (!tolerant.EvaluateBest(cook, MakeSnapshot(true), null, tolerated)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 698, "!tolerant.EvaluateBest(cook, MakeSnapshot(true), null, tolerated)");
        if (tolerated.extraHandles.Count() != 1) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 699, "tolerated.extraHandles.Count() != 1");
        if (tolerated.consumePlan.Count() != 4) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 700, "tolerated.consumePlan.Count() != 4");

        // Mit "consume" wandert der Fremdkoerper in den Verbrauchsplan.
        ChefZ_RecipeEngine hungry = BuildEngineWith(Recipe3(ChefZ_ExtraItemsMode.CONSUME_NAME));
        ChefZ_MatchResult consumed;
        if (!hungry.EvaluateBest(cook, MakeSnapshot(true), null, consumed)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 705, "!hungry.EvaluateBest(cook, MakeSnapshot(true), null, consumed)");
        if (consumed.consumePlan.Count() != 5) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 706, "consumed.consumePlan.Count() != 5");

        // Das Ventil aus 08 E2: ein Rezept darf einen bestimmten Fremdkoerper
        // dulden, ohne dass der Default sich aendert.
        ChefZ_RecipeDef valve = Recipe3(ChefZ_ExtraItemsMode.FORBID_NAME);
        valve.policy.extraItemsAllowedIf = ClassSelector(CLS_SPEZIAL);
        ChefZ_RecipeEngine valved = BuildEngineWith(valve);
        ChefZ_MatchResult allowed;
        if (!valved.EvaluateBest(cook, MakeSnapshot(true), null, allowed)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 714, "!valved.EvaluateBest(cook, MakeSnapshot(true), null, allowed)");
        if (allowed.extraHandles.Count() != 1) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 715, "allowed.extraHandles.Count() != 1");

        return true;
    }

    //==========================================================================
    // 7. Abschlussbedingung (10 §6)
    //==========================================================================

    private static bool TestCompletion()
    {
        ChefZ_RecipeEngine engine = BuildEngineWith(Recipe3(ChefZ_ExtraItemsMode.FORBID_NAME));
        ChefZ_CookContext  cook   = MakeCookContext();

        // Alle Items "Boiled" -> ON_STAGE ist erfuellt.
        ChefZ_FactSnapshot done = MakeSnapshot(false);
        ChefZ_MatchResult ready;
        if (!engine.EvaluateBest(cook, done, null, ready))
        {
            ChefZ_Log.Warn(ChefZ_LogChannel.MATCH, "Selbsttest S6 Abschluss: EvaluateBest(fertig) " + ready.ToDebugString());
            return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 735, "");
        }
        if (!ready.ready) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 737, "!ready.ready");

        // Ein rohes Item haelt das Rezept offen - und genau so soll es sein:
        // Vanilla kocht weiter, ChefZ wartet (08 E5).
        ChefZ_FactSnapshot raw = MakeSnapshot(false);
        raw.Get(0).vanillaFoodStage = ChefZ_VanillaStage.FromName("Raw");
        ChefZ_MatchResult pending;
        if (!engine.EvaluateBest(cook, raw, null, pending)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 744, "!engine.EvaluateBest(cook, raw, null, pending)");
        if (pending.ready) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 745, "pending.ready");
        if (pending.notReadyReason == "") return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 746, "pending.notReadyReason == ''");

        // CheckReady() liefert dieselbe Antwort wie EvaluateBest - sonst
        // koennte der Adapter (10 §5, Stufe C) etwas anderes sehen als der
        // Erstmatch.
        string reason;
        if (engine.CheckReady(pending, raw, cook, reason)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 752, "engine.CheckReady(pending, raw, cook, reason)");
        if (!engine.CheckReady(ready, done, cook, reason)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 753, "!engine.CheckReady(ready, done, cook, reason)");

        // INSTANT ist immer fertig.
        ChefZ_RecipeDef instant = Recipe3(ChefZ_ExtraItemsMode.FORBID_NAME);
        instant.completion = ChefZ_Completion.INSTANT_NAME;
        ChefZ_RecipeEngine instantEngine = BuildEngineWith(instant);
        ChefZ_MatchResult instantResult;
        if (!instantEngine.EvaluateBest(cook, raw, null, instantResult)) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 760, "!instantEngine.EvaluateBest(cook, raw, null, instantResult)");
        if (!instantResult.ready) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 761, "!instantResult.ready");

        return true;
    }

    //==========================================================================
    // 8. Ambiguitaeten (09 §5, E5)
    //==========================================================================

    private static bool TestAmbiguities()
    {
        // Zwei ununterscheidbare Rezepte: gleicher Rang, gleiche Slotmenge,
        // gleiches Geraet. 09 §7 verlangt dafuer ein WARN beim Build.
        ChefZ_LoadReport report = new ChefZ_LoadReport();
        report.SetMirrorToLog(false);

        ChefZ_RecipeDef twinA = Recipe1("");
        twinA.id = "CHEFZ_RT_ZWILLING_A";
        ChefZ_RecipeDef twinB = Recipe1("");
        twinB.id = "CHEFZ_RT_ZWILLING_B";

        ChefZ_Registry<ChefZ_RecipeDef> reg = new ChefZ_Registry<ChefZ_RecipeDef>();
        reg.Init(ChefZ_RecordKind.RECIPE);
        AddToRegistry(reg, twinA);
        AddToRegistry(reg, twinB);

        ChefZ_RecipeEngine engine = new ChefZ_RecipeEngine();
        engine.SetVerifyOutputClasses(false);
        engine.Build(reg, MakeDeviceRegistry(), MakeContextWith(report, true), null, report);

        if (engine.GetRecipeCount() != 2) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 791, "engine.GetRecipeCount() != 2");
        if (report.WarnCount() < 1) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 792, "report.WarnCount() < 1");
        if (report.ErrorCount() != 0) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 793, "report.ErrorCount() != 0");

        // ---------------------------------------------------------------
        // Verdeckung (09 §5, zweiter Fall)
        // ---------------------------------------------------------------
        // Ein Rezept ist verdeckt, wenn es alles verlangt, was ein anderes
        // verlangt, UND MEHR - und trotzdem dahinter rangiert. Dann bindet das
        // andere immer zuerst, und das breitere kommt nie zum Zug.
        //
        // Dieser Fall ist im Normalbetrieb schwer herzustellen, und das ist
        // eine EIGENSCHAFT der Ordnung, kein Zufall: mehr Pflichtslots heissen
        // mehr Spezifitaet, und Spezifitaet ist der erste Schluessel (09 E1).
        // Er tritt genau dann ein, wenn die zusaetzlichen Slots nichts zur
        // Spezifitaet beitragen und der Autor des schmaleren Rezepts eine
        // priority gesetzt hat.
        //
        // Der Test stellt genau das her: Kategoriegewichte auf 0, damit beide
        // Rezepte dieselbe Spezifitaet haben, und eine priority am schmaleren.
        // Dass es diesen Aufwand braucht, ist die eigentliche Nachricht dieses
        // Abschnitts - und der Grund, warum 09 E5 die Analyse ausdruecklich
        // heuristisch nennt.
        ChefZ_LoadReport shadowReport = new ChefZ_LoadReport();
        shadowReport.SetMirrorToLog(false);

        ChefZ_RecipeDef broad = Recipe2("");
        broad.id       = "CHEFZ_RT_SCHMAL";
        broad.priority = 500;

        ChefZ_RecipeDef narrow = Recipe2("");
        narrow.id = "CHEFZ_RT_BREIT";
        AddCategorySlot(narrow, "s4", KAT_A);       // echte Obermenge von broad

        ChefZ_Registry<ChefZ_RecipeDef> shadowReg = new ChefZ_Registry<ChefZ_RecipeDef>();
        shadowReg.Init(ChefZ_RecordKind.RECIPE);
        AddToRegistry(shadowReg, broad);
        AddToRegistry(shadowReg, narrow);

        ChefZ_RecipeEngine shadowEngine = new ChefZ_RecipeEngine();
        shadowEngine.SetVerifyOutputClasses(false);
        shadowEngine.Build(shadowReg, MakeDeviceRegistry(), MakeShadowContext(shadowReport), null, shadowReport);

        if (shadowReport.WarnCount() < 1) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 834, "shadowReport.WarnCount() < 1");
        if (shadowReport.ErrorCount() != 0) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 835, "shadowReport.ErrorCount() != 0");

        // Das verdeckte Rezept BLEIBT geladen (09 §7): bei anderem Geraet oder
        // anderer Menge kann die Verdeckung aufgehoben sein.
        if (shadowEngine.GetRecipeCount() != 2) return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 839, "shadowEngine.GetRecipeCount() != 2");

        // Und die Ordnung ist wirklich die behauptete: das schmale zuerst.
        if (shadowEngine.GetRecipeAt(0).id != "CHEFZ_RT_SCHMAL") return ChefZ_SelfTestTrace.Fail("RecipeSelfTest", 842, "shadowEngine.GetRecipeAt(0).id != 'CHEFZ_RT_SCHMAL'");

        return true;
    }

    /**
     * Kontext, in dem Kategorieslots NICHTS zur Spezifitaet beitragen.
     *
     * Nur fuer den Verdeckungstest. Er ist der einzige Weg, zwei Rezepte mit
     * verschieden vielen Pflichtslots auf dieselbe Spezifitaet zu bringen -
     * und ohne das gibt es keine Verdeckung zu melden.
     */
    private static ChefZ_CompileContext MakeShadowContext(ChefZ_LoadReport report)
    {
        ChefZ_CompileContext ctx = MakeContextWith(report, true);

        ChefZ_PriorityWeights w = ctx.Weights();
        w.wCategoryBase     = 0.0;
        w.wCategoryPerDepth = 0.0;
        return ctx;
    }

    //==========================================================================
    // Hilfen
    //==========================================================================

    /**
     * Eine Engine mit den vier Tabellenrezepten.
     *
     * R1 und R2 duerfen Fremdkoerper dulden, R3 und R4 nicht. Ohne diese
     * Aufteilung wuerden R1 und R2 an der forbid-Regel scheitern, sobald das
     * Gefaess mehr enthaelt, als sie binden - und der Test pruefte dann die
     * Policy statt der Rangordnung.
     */
    private static ChefZ_RecipeEngine BuildEngine(bool tolerantSmall, bool mirror)
    {
        ChefZ_LoadReport report = new ChefZ_LoadReport();
        report.SetMirrorToLog(mirror);

        string small = ChefZ_ExtraItemsMode.FORBID_NAME;
        if (tolerantSmall)
            small = ChefZ_ExtraItemsMode.IGNORE_NAME;

        ChefZ_Registry<ChefZ_RecipeDef> reg = new ChefZ_Registry<ChefZ_RecipeDef>();
        reg.Init(ChefZ_RecordKind.RECIPE);
        AddToRegistry(reg, Recipe1(small));
        AddToRegistry(reg, Recipe2(small));
        AddToRegistry(reg, Recipe3(ChefZ_ExtraItemsMode.FORBID_NAME));
        AddToRegistry(reg, Recipe4(ChefZ_ExtraItemsMode.FORBID_NAME));

        ChefZ_RecipeEngine engine = new ChefZ_RecipeEngine();
        engine.SetVerifyOutputClasses(false);
        engine.Build(reg, MakeDeviceRegistry(), MakeContextWith(report, true), null, report);
        return engine;
    }

    private static ChefZ_RecipeEngine BuildEngineWith(notnull ChefZ_RecipeDef def)
    {
        ChefZ_LoadReport report = new ChefZ_LoadReport();
        report.SetMirrorToLog(false);

        ChefZ_Registry<ChefZ_RecipeDef> reg = new ChefZ_Registry<ChefZ_RecipeDef>();
        reg.Init(ChefZ_RecordKind.RECIPE);
        AddToRegistry(reg, def);

        ChefZ_RecipeEngine engine = new ChefZ_RecipeEngine();
        engine.SetVerifyOutputClasses(false);
        engine.Build(reg, MakeDeviceRegistry(), MakeContextWith(report, true), null, report);
        return engine;
    }

    //! Float-Vergleich mit Toleranz. Ein Gleichheitsvergleich auf float waere
    //! eine Wette darauf, dass 1.5 + 1.5 + 1.0 + 1.0 auf jeder Plattform
    //! bitgenau 5.0 ergibt.
    private static bool Near(float actual, float expected)
    {
        float diff = actual - expected;
        if (diff < 0.0)
            diff = -diff;
        return diff < 0.001;
    }
}
