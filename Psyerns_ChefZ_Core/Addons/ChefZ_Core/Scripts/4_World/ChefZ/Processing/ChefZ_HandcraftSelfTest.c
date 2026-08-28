//==============================================================================
// ChefZ_HandcraftSelfTest - Abnahmepruefung fuer S15
//
// Entwurf: 19 §3, S15 - "Fertig, wenn: Ein HANDCRAFT-Testtransform erscheint
// als normales Craftrezept im Spiel", 11 §7, 11 E3, 01 V12.
//
// ---------------------------------------------------------------------------
// Warum es diesen Test gibt - und was er wirklich prueft
// ---------------------------------------------------------------------------
// Die Handcraft-Bruecke hat eine Fehlerklasse, die auf einem laufenden Server
// NICHT auffaellt und in keiner Logzeile steht: Vanillas RecipeBase legt alle
// seine Felder mit 0 vor, und 0 bedeutet in fast jedem dieser Felder etwas
// ANDERES als "nichts tun".
//
//     m_ResultSetHealth[i]        = 0  ->  das Ergebnis entsteht RUINIERT
//     m_ResultSetQuantity[i]      = 0  ->  SetQuantity(0) am Ergebnis
//     m_MaxQuantityIngredient[i]  = 0  ->  jede Zutat mit Menge > 0 faellt
//                                          durch CheckConditions
//     m_ResultInheritsColor[i]    = 0  ->  der Klassenname bekommt die
//                                          "color" der Zutat angehaengt
//     m_ResultReplacesIngredient[i] = 0 -> Eigenschaften UND Inventar der
//                                          Zutat wandern ins Ergebnis
//
// Jedes einzelne davon ergibt ein Rezept, das REGISTRIERT ist, in der
// Craftliste ERSCHEINT und dann etwas Falsches tut. Es gibt keinen Absturz und
// keine Meldung. Genau diese Felder prueft die erste Gruppe dieses Tests.
//
// Die zweite Fehlerklasse ist die Abbildung auf Vanillas zwei Zutatenplaetze
// (01 V12). Sie faellt frueher auf - ein Rezept, das gar nicht registriert
// wird, merkt man -, aber sie ist ohne Welt pruefbar und deshalb hier.
//
// PRUEFBAR OHNE WELT, und deshalb hier:
//   - die Vanillafelder nach dem Aufbau
//   - die Abbildung 1 Eingang + Werkzeug / 2 Eingaenge ohne Werkzeug
//   - jede Abweisung aus dem Kopf von ChefZ_GenericCraftRecipe
//   - dass Init() aus dem Konstruktor folgenlos bleibt (11 E3)
//   - die Ueber-Naeherung Selektor -> Klassennamen
//
// NICHT PRUEFBAR OHNE WELT, und deshalb dem Servertest vorbehalten:
//   - dass das Rezept in der Craftliste des Spielers auftaucht. Das braucht
//     Vanillas Rezeptcache, und der braucht einen vollstaendigen
//     CfgVehicles-Durchlauf.
//   - dass super.RegisterRecipies() zuerst laeuft. Das ist keine Rechnung,
//     sondern eine Struktur - dieselbe Lage wie beim Kochhook (10 §3). Der
//     Conflict-Scout prueft es am Diff, nicht ein Testlauf.
//   - Verbrauch, Werkzeugschaden und Animation. Alles drei gehoert Vanilla.
//
// KEIN CONTENT: alle Namen tragen das Praefix "CHEFZ_HC_" und sind abstrakt.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_HandcraftSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static const string PROZ_HAND  = "CHEFZ_HC_PROZ";
    static const string PROZ_LEER  = "CHEFZ_HC_PROZ_OHNE_WERKZEUG";
    static const string GRUPPE     = "CHEFZ_HC_GRUPPE";
    static const string WERKZEUG_A = "CHEFZ_HC_WERKZEUG_A";
    static const string WERKZEUG_B = "CHEFZ_HC_WERKZEUG_B";
    static const string EINGANG_A  = "CHEFZ_HC_EINGANG_A";
    static const string EINGANG_B  = "CHEFZ_HC_EINGANG_B";
    static const string ERGEBNIS   = "CHEFZ_HC_ERGEBNIS";
    static const string ZUSTAND    = "CHEFZ_HC_ZUSTAND";
    static const string TRANSFORM  = "CHEFZ_HC_TRANSFORM";

    //==========================================================================

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("Init ohne Definition", TestInitWithoutDef());
        Check("Vanillafelder",        TestVanillaDefaults());
        Check("Abbildung 1+Werkzeug", TestOneInputWithTool());
        Check("Abbildung 2 Eingaenge", TestTwoInputs());
        Check("Abweisungen",          TestRejections());
        Check("Zustandswechsel",      TestPureStateChange());
        Check("Wiederholbarkeit",     TestRepeatable());

        // Die Identitaetsarithmetik des zweiten Netzes gegen Rezept-ID-
        // Versatz. Sie steht hier und nicht in einem eigenen Selbsttest,
        // weil sie zu genau diesem Teilsystem gehoert - und sie MUSS geprueft
        // werden: faellt sie um, faellt sie in die falsche Richtung.
        //
        // Ein Accepts(), das zu viel durchlaesst, ist bloss wieder der alte
        // Zustand. Ein Accepts(), das zu wenig durchlaesst - etwa weil
        // UNKNOWN nicht mehr der Vorgabewert eines nicht gesetzten int ist -,
        // verweigert JEDE Craftaktion im Einzelspielerbetrieb, und zwar mit
        // einer Fehlermeldung, die auf einen Versatz zeigt, den es gar nicht
        // gibt.
        Check("Kennung Craftaktion",  ChefZ_CraftIntent.SelfCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS, "Selbsttest S15 " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.PROCESS, "Selbsttest S15 " + name + " FEHLGESCHLAGEN. Handwerksrezepte verhalten sich " + "nicht wie entworfen - ein registriertes Rezept kann damit ein ruiniertes " + "oder leeres Ergebnis liefern, ohne dass irgendwo eine Zeile im Log steht.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S15: " + s_Passed.ToString() + "/" + total.ToString()
                 + " Gruppen ok";
        if (s_Failed > 0 && s_FailedNames)
        {
            s = s + "  gescheitert:";
            for (int i = 0; i < s_FailedNames.Count(); i++)
                s = s + " " + s_FailedNames.Get(i);
        }
        return s;
    }

    //==========================================================================
    // Testdaten - handgebaut, ohne Compiler und ohne Registry
    //==========================================================================

    private static ChefZ_CompiledProcess MakeProcess(bool withTool)
    {
        ChefZ_CompiledProcess proc = new ChefZ_CompiledProcess();
        proc.id              = PROZ_HAND;
        if (!withTool)
            proc.id = PROZ_LEER;
        proc.processSym      = ChefZ_SymbolTable.Intern(proc.id);
        proc.exec            = ChefZ_ProcessExec.HANDCRAFT;
        proc.animationLength = 1.5;
        proc.specialty       = 0.02;
        proc.toolDamage      = 6;
        proc.displayName     = "";

        if (withTool)
            proc.toolGroups.Insert(ChefZ_SymbolTable.Intern(GRUPPE));

        return proc;
    }

    private static ChefZ_CompiledSlot MakeSlot(string id, string cls, int consumeMode)
    {
        ChefZ_CompiledSlot slot = new ChefZ_CompiledSlot();
        slot.slotIndex   = 0;
        slot.slotId      = id;
        slot.slotIdSym   = ChefZ_SymbolTable.Intern(id);
        slot.minCount    = 1;
        slot.maxCount    = 1;
        slot.consumeMode = consumeMode;

        ChefZ_CompiledSelector sel = new ChefZ_CompiledSelector();
        sel.op  = ChefZ_SelectorOp.CLASS;
        sel.sym = ChefZ_SymbolTable.Intern(cls);
        slot.selector = sel;

        return slot;
    }

    private static ChefZ_OutputDef MakeOutput(string cls, string setState)
    {
        ChefZ_OutputDef def = new ChefZ_OutputDef();
        def.cls               = cls;
        def.setState          = setState;
        def.quantityMode      = "fixed";
        def.chance            = 1.0;
        def.ratio             = 1.0;
        def.freshnessCarry    = 1.0;
        def.inheritFreshness  = true;
        def.inheritTemperature = true;
        def.containerCategory = "";
        def.returnContainer   = "";
        return def;
    }

    private static ChefZ_CompiledTransform MakeTransform(int inputCount, int consumeMode, bool pureState)
    {
        ChefZ_CompiledTransform tr = new ChefZ_CompiledTransform();
        tr.id           = TRANSFORM;
        tr.transformSym = ChefZ_SymbolTable.Intern(TRANSFORM);
        tr.sourceRef    = "selftest";

        tr.inputs.Insert(MakeSlot("a", EINGANG_A, consumeMode));
        if (inputCount > 1)
            tr.inputs.Insert(MakeSlot("b", EINGANG_B, consumeMode));

        if (pureState)
        {
            tr.pureStateChange = true;
            tr.outputs.Insert(MakeOutput("", ZUSTAND));
        }
        else
        {
            tr.outputs.Insert(MakeOutput(ERGEBNIS, ""));
        }

        return tr;
    }

    private static array<ref array<string>> MakeInputClasses(int count)
    {
        array<ref array<string>> lists = new array<ref array<string>>();

        array<string> a = new array<string>();
        a.Insert(EINGANG_A);
        lists.Insert(a);

        if (count > 1)
        {
            array<string> b = new array<string>();
            b.Insert(EINGANG_B);
            lists.Insert(b);
        }

        return lists;
    }

    private static array<string> MakeToolClasses(bool withTool)
    {
        array<string> tools = new array<string>();
        if (withTool)
        {
            tools.Insert(WERKZEUG_A);
            tools.Insert(WERKZEUG_B);
        }
        return tools;
    }

    //==========================================================================
    // 1 - Init() aus dem Konstruktor bleibt folgenlos (11 E3)
    //==========================================================================

    private static bool TestInitWithoutDef()
    {
        ChefZ_GenericCraftRecipe recipe = new ChefZ_GenericCraftRecipe();

        // RecipeBase() hat Init() bereits gerufen. Waere Init() nicht
        // folgenlos, staende hier bereits irgendetwas.
        if (recipe.ChefZ_IsReady())                       return false;
        if (recipe.ChefZ_GetResultCount() != 0)           return false;
        if (recipe.ChefZ_GetIngredientClassCount(0) != 0) return false;
        if (recipe.ChefZ_GetIngredientClassCount(1) != 0) return false;

        // Und ein zweiter, ausdruecklicher Aufruf ebenfalls.
        recipe.Init();
        if (recipe.ChefZ_IsReady())                       return false;

        // Ohne Definition darf CanDo() niemals true sagen.
        ItemBase none[2];
        if (recipe.CanDo(none, null))                     return false;

        return true;
    }

    //==========================================================================
    // 2 - die Vanillafelder (siehe Dateikopf)
    //==========================================================================

    private static bool TestVanillaDefaults()
    {
        ChefZ_GenericCraftRecipe recipe = new ChefZ_GenericCraftRecipe();

        string err;
        if (!recipe.InitFromDef(MakeProcess(true), MakeTransform(1, ChefZ_ConsumeMode.WHOLE, false), MakeInputClasses(1), MakeToolClasses(true), err))
        {
            return false;
        }

        int i;

        for (i = 0; i < MAX_NUMBER_OF_INGREDIENTS; i++)
        {
            // "-1 = disable check" in Vanillas eigener Schreibweise. Eine 0
            // hier waere die Mengenfalle aus dem Dateikopf.
            if (recipe.m_MinQuantityIngredient[i] != -1) return false;
            if (recipe.m_MaxQuantityIngredient[i] != -1) return false;
            if (recipe.m_MinDamageIngredient[i]   != -1) return false;

            // Ruinierte Zutaten sind ausgeschlossen - Vanillas Konvention.
            if (recipe.m_MaxDamageIngredient[i] != ChefZ_GenericCraftRecipe.MAX_HEALTH_LEVEL) return false;
        }

        for (i = 0; i < MAXIMUM_RESULTS; i++)
        {
            if (recipe.m_ResultSetQuantity[i]        != -1) return false;
            if (recipe.m_ResultSetHealth[i]          != -1) return false;
            if (recipe.m_ResultInheritsColor[i]      != -1) return false;
            if (recipe.m_ResultReplacesIngredient[i] != -1) return false;
            if (recipe.m_ResultSetFullQuantity[i])          return false;
        }

        // Das Ergebnis erbt die Health vom ERSTEN Eingang, nicht vom Werkzeug
        // und nicht vom Durchschnitt beider.
        if (recipe.m_ResultInheritsHealth[0] != 0) return false;

        // In das Inventar des Spielers, ersatzweise auf den Boden.
        if (recipe.m_ResultToInventory[0] != -1)   return false;

        // Der Eingang wird NICHT von Vanilla verbraucht - das tut der
        // ChefZ-Verbrauchsplan (siehe Kopf von ChefZ_GenericCraftRecipe).
        if (recipe.m_IngredientDestroy[0])         return false;
        if (recipe.m_IngredientAddQuantity[0] != 0) return false;

        // Das Werkzeug nutzt sich ab, und zwar um genau toolDamage.
        if (recipe.m_IngredientAddHealth[1] != -6) return false;
        if (recipe.m_IngredientDestroy[1])         return false;

        // Animation und Softskill kommen aus den Daten.
        if (recipe.m_AnimationLength != 1.5)       return false;
        if (recipe.m_Specialty != 0.02)            return false;
        if (recipe.m_IsInstaRecipe)                return false;
        if (recipe.IsRecipeAnywhere())             return false;

        // Ohne displayName der Rueckfalltext aus der Stringtable.
        if (recipe.GetName() != ChefZ_GenericCraftRecipe.FALLBACK_NAME) return false;

        return true;
    }

    //==========================================================================
    // 3 - 1 Eingang + Werkzeug
    //==========================================================================

    private static bool TestOneInputWithTool()
    {
        ChefZ_GenericCraftRecipe recipe = new ChefZ_GenericCraftRecipe();

        string err;
        if (!recipe.InitFromDef(MakeProcess(true), MakeTransform(1, ChefZ_ConsumeMode.WHOLE, false), MakeInputClasses(1), MakeToolClasses(true), err))
        {
            return false;
        }

        if (!recipe.ChefZ_IsReady())                      return false;
        if (recipe.ChefZ_GetInputCount() != 1)            return false;
        if (recipe.ChefZ_GetToolIndex() != 1)             return false;

        // Platz 0 traegt den Eingang, Platz 1 beide Werkzeugklassen.
        if (recipe.ChefZ_GetIngredientClassCount(0) != 1) return false;
        if (recipe.ChefZ_GetIngredientClassCount(1) != 2) return false;

        if (!recipe.IsItemInRecipe(EINGANG_A))            return false;
        if (!recipe.IsItemInRecipe(WERKZEUG_A))           return false;
        if (!recipe.IsItemInRecipe(WERKZEUG_B))           return false;

        if (recipe.ChefZ_GetResultCount() != 1)           return false;

        return true;
    }

    //==========================================================================
    // 4 - 2 Eingaenge, kein Werkzeug
    //==========================================================================

    private static bool TestTwoInputs()
    {
        ChefZ_GenericCraftRecipe recipe = new ChefZ_GenericCraftRecipe();

        string err;
        if (!recipe.InitFromDef(MakeProcess(false), MakeTransform(2, ChefZ_ConsumeMode.WHOLE, false), MakeInputClasses(2), MakeToolClasses(false), err))
        {
            return false;
        }

        if (recipe.ChefZ_GetInputCount() != 2)            return false;
        if (recipe.ChefZ_GetToolIndex() != -1)            return false;
        if (recipe.ChefZ_GetIngredientClassCount(0) != 1) return false;
        if (recipe.ChefZ_GetIngredientClassCount(1) != 1) return false;

        if (!recipe.IsItemInRecipe(EINGANG_A))            return false;
        if (!recipe.IsItemInRecipe(EINGANG_B))            return false;

        return true;
    }

    //==========================================================================
    // 5 - jede Abweisung aus dem Kopf von ChefZ_GenericCraftRecipe
    //==========================================================================

    private static bool TestRejections()
    {
        string err;

        // a) 1 Eingang, KEINE Werkzeuggruppe -> es gaebe keinen zweiten Platz
        ChefZ_GenericCraftRecipe a = new ChefZ_GenericCraftRecipe();
        if (a.InitFromDef(MakeProcess(false), MakeTransform(1, ChefZ_ConsumeMode.WHOLE, false), MakeInputClasses(1), MakeToolClasses(false), err))
        {
            return false;
        }
        if (err == "")        return false;
        if (a.ChefZ_IsReady()) return false;

        // b) 2 Eingaenge MIT Werkzeuggruppe -> das Werkzeug waere der dritte
        //    Platz (01 V12)
        ChefZ_GenericCraftRecipe b = new ChefZ_GenericCraftRecipe();
        if (b.InitFromDef(MakeProcess(true), MakeTransform(2, ChefZ_ConsumeMode.WHOLE, false), MakeInputClasses(2), MakeToolClasses(true), err))
        {
            return false;
        }
        if (b.ChefZ_IsReady()) return false;

        // c) Werkzeuggruppe gefordert, aber keine Klasse dazu bekannt
        ChefZ_GenericCraftRecipe c = new ChefZ_GenericCraftRecipe();
        if (c.InitFromDef(MakeProcess(true), MakeTransform(1, ChefZ_ConsumeMode.WHOLE, false), MakeInputClasses(1), MakeToolClasses(false), err))
        {
            return false;
        }
        if (c.ChefZ_IsReady()) return false;

        // d) Eingangsslot ohne eine einzige passende Klasse
        array<ref array<string>> leer = new array<ref array<string>>();
        leer.Insert(new array<string>());

        ChefZ_GenericCraftRecipe d = new ChefZ_GenericCraftRecipe();
        if (d.InitFromDef(MakeProcess(true), MakeTransform(1, ChefZ_ConsumeMode.WHOLE, false), leer, MakeToolClasses(true), err))
        {
            return false;
        }
        if (d.ChefZ_IsReady()) return false;

        // e) mehr als HANDCRAFT_MAX_INPUTS Eingaenge
        ChefZ_CompiledTransform drei = MakeTransform(2, ChefZ_ConsumeMode.WHOLE, false);
        drei.inputs.Insert(MakeSlot("c", EINGANG_A, ChefZ_ConsumeMode.WHOLE));

        array<ref array<string>> dreiKlassen = MakeInputClasses(2);
        array<string> extra = new array<string>();
        extra.Insert(EINGANG_A);
        dreiKlassen.Insert(extra);

        ChefZ_GenericCraftRecipe e = new ChefZ_GenericCraftRecipe();
        if (e.InitFromDef(MakeProcess(false), drei, dreiKlassen, MakeToolClasses(false), err))
        {
            return false;
        }
        if (e.ChefZ_IsReady()) return false;

        // f) weder Ergebnis noch Zustandswechsel -> Zutatenvernichtung (11 §7)
        ChefZ_CompiledTransform ohne = MakeTransform(1, ChefZ_ConsumeMode.WHOLE, false);
        ohne.outputs.Clear();

        ChefZ_GenericCraftRecipe f = new ChefZ_GenericCraftRecipe();
        if (f.InitFromDef(MakeProcess(true), ohne, MakeInputClasses(1), MakeToolClasses(true), err))
        {
            return false;
        }
        if (f.ChefZ_IsReady()) return false;

        return true;
    }

    //==========================================================================
    // 6 - reiner Zustandswechsel (11 §2)
    //==========================================================================

    private static bool TestPureStateChange()
    {
        ChefZ_GenericCraftRecipe recipe = new ChefZ_GenericCraftRecipe();

        string err;
        if (!recipe.InitFromDef(MakeProcess(true), MakeTransform(1, ChefZ_ConsumeMode.NONE, true), MakeInputClasses(1), MakeToolClasses(true), err))
        {
            return false;
        }

        if (!recipe.ChefZ_IsPureStateChange()) return false;

        // Vanilla darf NICHTS erzeugen: es entsteht kein Item, es wechselt
        // nur ein Zustand.
        if (recipe.ChefZ_GetResultCount() != 0) return false;

        // Und es darf auch nichts verbrauchen.
        if (recipe.m_IngredientDestroy[0])      return false;

        return true;
    }

    //==========================================================================
    // 7 - Wiederholbarkeit ist abgeleitet, nicht erfunden
    //==========================================================================

    private static bool TestRepeatable()
    {
        string err;

        ChefZ_GenericCraftRecipe whole = new ChefZ_GenericCraftRecipe();
        if (!whole.InitFromDef(MakeProcess(true), MakeTransform(1, ChefZ_ConsumeMode.WHOLE, false), MakeInputClasses(1), MakeToolClasses(true), err))
        {
            return false;
        }
        if (whole.IsRepeatable()) return false;      // ganz verbraucht -> einmal

        ChefZ_GenericCraftRecipe part = new ChefZ_GenericCraftRecipe();
        if (!part.InitFromDef(MakeProcess(true), MakeTransform(1, ChefZ_ConsumeMode.AMOUNT, false), MakeInputClasses(1), MakeToolClasses(true), err))
        {
            return false;
        }
        if (!part.IsRepeatable()) return false;      // Rest bleibt liegen -> nochmal

        return true;
    }
}
