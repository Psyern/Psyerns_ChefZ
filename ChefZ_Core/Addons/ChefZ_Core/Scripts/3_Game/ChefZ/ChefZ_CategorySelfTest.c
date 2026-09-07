//==============================================================================
// ChefZ_CategorySelfTest - Abnahmepruefung fuer S3
//
// Entwurf: 19 S3, 04 §6 (Fehlerverhalten, Zeile fuer Zeile), 04 E1/E2/E4.
//
// Geprueft wird genau das, was spaeter niemand mehr von aussen sehen kann:
// Vorfahrenbitsets, Tiefen, Spezifitaetsgewichte, und vor allem die drei
// Fehlerfaelle - unbekanntes parent, Zyklus, Deckel. Der Zyklusfall ist der
// wichtigste: 04 §6 sagt "IsInCategory darf unter keinen Umstaenden endlos
// laufen", und eine Endlosschleife im Boot ist der eine Ausfall, der einen
// Server aufhaengt statt ihn abstuerzen zu lassen.
//
// Der Test arbeitet auf EIGENEN Manager-Instanzen, nie auf dem Singleton, und
// mit einem Ladebericht ohne Log-Spiegelung. Er legt ausschliesslich Symbole
// mit dem Praefix "CHEFZ_ST_" an - Namen, die in echtem Content nicht
// vorkommen. Er beruehrt kein Item, keine Datei und keine Vanilla-Logik.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_CategorySelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("Closure",   ChefZ_CategoryClosure.SelfCheck());
        Check("Baum",      TreeCheck());
        Check("Zyklus",    CycleCheck());
        Check("Deckel",    LimitCheck());
        Check("LeererBaum", EmptyCheck());
        Check("VorBuild",  NotReadyCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++; ChefZ_SelfTestTrace.Reset();
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.CONFIG, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, "Selbsttest " + name + " FEHLGESCHLAGEN. Der Kategoriebaum verhaelt sich nicht wie " + "entworfen - jede Kategorieabfrage und damit jede Rezeptauswahl ist ab hier " + "unzuverlaessig." + ChefZ_SelfTestTrace.Take());
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S3: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
        if (s_Failed > 0 && s_FailedNames)
        {
            s = s + "  gescheitert:";
            for (int i = 0; i < s_FailedNames.Count(); i++)
                s = s + " " + s_FailedNames.Get(i);
        }
        return s;
    }

    //==========================================================================
    // Hilfen
    //==========================================================================

    private static ChefZ_Registry<ChefZ_CategoryDef> NewCatRegistry()
    {
        ChefZ_Registry<ChefZ_CategoryDef> reg = new ChefZ_Registry<ChefZ_CategoryDef>();
        reg.Init(ChefZ_RecordKind.CATEGORY);
        return reg;
    }

    private static bool AddCat(notnull ChefZ_Registry<ChefZ_CategoryDef> reg, string id, string parent, string displayName)
    {
        ChefZ_CategoryDef def = new ChefZ_CategoryDef();
        def.id          = id;
        def.parent      = parent;
        def.displayName = displayName;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.Compile(null);                 // COMPILE: eigene ID internen
        return reg.Add(def);
    }

    private static ChefZ_Registry<ChefZ_TagDef> NewTagRegistry()
    {
        ChefZ_Registry<ChefZ_TagDef> reg = new ChefZ_Registry<ChefZ_TagDef>();
        reg.Init(ChefZ_RecordKind.TAG);
        return reg;
    }

    private static bool AddTag(notnull ChefZ_Registry<ChefZ_TagDef> reg, string id)
    {
        ChefZ_TagDef def = new ChefZ_TagDef();
        def.id = id;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.Compile(null);
        return reg.Add(def);
    }

    private static ChefZ_CategoryManager NewManager()
    {
        ChefZ_CategoryManager mgr = new ChefZ_CategoryManager();
        mgr.SetQuietForTest(true);
        mgr.SetMaxCategoriesForTest(64);   // unabhaengig von Core.json
        return mgr;
    }

    private static ChefZ_LoadReport NewReport()
    {
        ChefZ_LoadReport report = new ChefZ_LoadReport();
        report.SetMirrorToLog(false);      // erwartete Fehler gehoeren nicht ins RPT
        return report;
    }

    private static ChefZ_Sym Sym(string id)
    {
        return ChefZ_SymbolTable.Lookup(id);
    }

    //==========================================================================
    // 1. Der normale Baum
    //==========================================================================

    /**
     *   CHEFZ_ST_C_MEAT                  Tiefe 0, Gewicht 1.0
     *     +-- CHEFZ_ST_C_WILD            Tiefe 1, Gewicht 1.5
     *           +-- CHEFZ_ST_C_DEER      Tiefe 2, Gewicht 2.0
     *   CHEFZ_ST_C_FISH                  Tiefe 0, ohne displayName
     *   CHEFZ_ST_C_ORPHAN                parent unbekannt -> Wurzel + WARN
     */
    private static bool TreeCheck()
    {
        ChefZ_Registry<ChefZ_CategoryDef> cats = NewCatRegistry();
        if (!AddCat(cats, "CHEFZ_ST_C_MEAT",   "",                  "Fleisch")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 146, "!AddCat(cats, 'CHEFZ_ST_C_MEAT', '', 'Fleisch')");
        if (!AddCat(cats, "CHEFZ_ST_C_WILD",   "CHEFZ_ST_C_MEAT",   "Wild")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 147, "!AddCat(cats, 'CHEFZ_ST_C_WILD', 'CHEFZ_ST_C_MEAT', 'Wild')");
        if (!AddCat(cats, "CHEFZ_ST_C_DEER",   "CHEFZ_ST_C_WILD",   "Hirsch")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 148, "!AddCat(cats, 'CHEFZ_ST_C_DEER', 'CHEFZ_ST_C_WILD', 'Hirsch')");
        if (!AddCat(cats, "CHEFZ_ST_C_FISH",   "",                  "")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 149, "!AddCat(cats, 'CHEFZ_ST_C_FISH', '', '')");
        if (!AddCat(cats, "CHEFZ_ST_C_ORPHAN", "CHEFZ_ST_C_NOPE",   "Waise")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 150, "!AddCat(cats, 'CHEFZ_ST_C_ORPHAN', 'CHEFZ_ST_C_NOPE', 'Waise')");

        ChefZ_Registry<ChefZ_TagDef> tags = NewTagRegistry();
        if (!AddTag(tags, "CHEFZ_ST_T_SMOKED")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 153, "!AddTag(tags, 'CHEFZ_ST_T_SMOKED')");

        ChefZ_LoadReport report = NewReport();
        ChefZ_CategoryManager mgr = NewManager();
        mgr.Build(cats, tags, report);

        if (!mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 159, "!mgr.IsReady()");
        if (mgr.GetCategoryCount() != 5) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 160, "mgr.GetCategoryCount() != 5");
        if (mgr.GetMaxDepth() != 2) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 161, "mgr.GetMaxDepth() != 2");
        if (mgr.GetRejectedCount() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 162, "mgr.GetRejectedCount() != 0");

        // Unbekanntes parent ist eine WARNUNG, kein Fehler (04 §6).
        if (report.ErrorCount() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 165, "report.ErrorCount() != 0");
        if (report.WarnCount() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 166, "report.WarnCount() != 1");

        ChefZ_Sym katA   = Sym("CHEFZ_ST_C_MEAT");
        ChefZ_Sym wild   = Sym("CHEFZ_ST_C_WILD");
        ChefZ_Sym deer   = Sym("CHEFZ_ST_C_DEER");
        ChefZ_Sym katB   = Sym("CHEFZ_ST_C_FISH");
        ChefZ_Sym orphan = Sym("CHEFZ_ST_C_ORPHAN");

        // --- Existenz und Tiefe -------------------------------------------
        if (!mgr.Exists(katA)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 175, "!mgr.Exists(katA)");
        if (mgr.Exists(ChefZ_SymbolTable.Intern("CHEFZ_ST_C_NOPE"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 176, "mgr.Exists(ChefZ_SymbolTable.Intern('CHEFZ_ST_C_NOPE'))");
        if (mgr.Exists(ChefZ_SymbolTable.INVALID)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 177, "mgr.Exists(ChefZ_SymbolTable.INVALID)");

        if (mgr.GetDepth(katA)   != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 179, "mgr.GetDepth(katA) != 0");
        if (mgr.GetDepth(wild)   != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 180, "mgr.GetDepth(wild) != 1");
        if (mgr.GetDepth(deer)   != 2) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 181, "mgr.GetDepth(deer) != 2");
        if (mgr.GetDepth(katB)   != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 182, "mgr.GetDepth(katB) != 0");
        if (mgr.GetDepth(orphan) != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 183, "mgr.GetDepth(orphan) != 0");   // Wurzel geworden
        if (mgr.GetDepth(Sym("CHEFZ_ST_C_NOPE")) != -1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 184, "mgr.GetDepth(Sym('CHEFZ_ST_C_NOPE')) != -1");

        // --- Eltern --------------------------------------------------------
        if (mgr.GetParent(deer) != wild) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 187, "mgr.GetParent(deer) != wild");
        if (mgr.GetParent(wild) != katA) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 188, "mgr.GetParent(wild) != katA");
        if (mgr.GetParent(katA) != ChefZ_SymbolTable.INVALID) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 189, "mgr.GetParent(katA) != ChefZ_SymbolTable.INVALID");
        if (mgr.GetParent(orphan) != ChefZ_SymbolTable.INVALID) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 190, "mgr.GetParent(orphan) != ChefZ_SymbolTable.INVALID");

        // --- Spezifitaetsgewicht, 04 E4 ------------------------------------
        if (mgr.GetSpecificityWeight(katA) != 1.0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 193, "mgr.GetSpecificityWeight(katA) != 1.0");
        if (mgr.GetSpecificityWeight(wild) != 1.5) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 194, "mgr.GetSpecificityWeight(wild) != 1.5");
        if (mgr.GetSpecificityWeight(deer) != 2.0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 195, "mgr.GetSpecificityWeight(deer) != 2.0");
        if (mgr.GetSpecificityWeight(Sym("CHEFZ_ST_C_NOPE")) != 0.0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 196, "mgr.GetSpecificityWeight(Sym('CHEFZ_ST_C_NOPE')) != 0.0");

        // --- Bitindizes: dicht, verschieden --------------------------------
        int bKatA = mgr.GetBitIndex(katA);
        int bWild = mgr.GetBitIndex(wild);
        int bDeer = mgr.GetBitIndex(deer);
        if (bKatA < 0 || bWild < 0 || bDeer < 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 202, "bKatA < 0 || bWild < 0 || bDeer < 0");
        if (bKatA == bWild || bWild == bDeer || bKatA == bDeer) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 203, "bKatA == bWild || bWild == bDeer || bKatA == bDeer");
        if (bKatA >= 5 || bWild >= 5 || bDeer >= 5) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 204, "bKatA >= 5 || bWild >= 5 || bDeer >= 5");
        if (mgr.GetBitIndex(Sym("CHEFZ_ST_C_NOPE")) != -1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 205, "mgr.GetBitIndex(Sym('CHEFZ_ST_C_NOPE')) != -1");

        // --- Closure: self-or-ancestor (04 E1) ------------------------------
        array<ChefZ_Sym> direct = new array<ChefZ_Sym>();
        direct.Insert(deer);

        ChefZ_CategoryClosure closure;
        mgr.BuildClosure(direct, closure);
        if (!closure) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 213, "!closure");

        if (!mgr.IsInCategory(closure, deer)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 215, "!mgr.IsInCategory(closure, deer)");   // sich selbst
        if (!mgr.IsInCategory(closure, wild)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 216, "!mgr.IsInCategory(closure, wild)");   // Elternteil
        if (!mgr.IsInCategory(closure, katA)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 217, "!mgr.IsInCategory(closure, katA)");   // Grosselternteil
        if (mgr.IsInCategory(closure, katB)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 218, "mgr.IsInCategory(closure, katB)");   // fremder Ast
        if (mgr.IsInCategory(closure, orphan)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 219, "mgr.IsInCategory(closure, orphan)");
        if (mgr.IsInCategory(closure, Sym("CHEFZ_ST_C_NOPE"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 220, "mgr.IsInCategory(closure, Sym('CHEFZ_ST_C_NOPE'))");   // unbekannt
        if (mgr.IsInCategory(closure, ChefZ_SymbolTable.INVALID)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 221, "mgr.IsInCategory(closure, ChefZ_SymbolTable.INVALID)");
        if (closure.CountBits() != 3) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 222, "closure.CountBits() != 3");

        // Vererbung wirkt NUR nach oben: MEAT erfuellt nicht DEER.
        array<ChefZ_Sym> up = new array<ChefZ_Sym>();
        up.Insert(katA);
        ChefZ_CategoryClosure upClosure;
        mgr.BuildClosure(up, upClosure);
        if (!mgr.IsInCategory(upClosure, katA)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 229, "!mgr.IsInCategory(upClosure, katA)");
        if (mgr.IsInCategory(upClosure, deer)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 230, "mgr.IsInCategory(upClosure, deer)");
        if (upClosure.CountBits() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 231, "upClosure.CountBits() != 1");

        // Mehrere direkte Kategorien vereinigen sich.
        array<ChefZ_Sym> both = new array<ChefZ_Sym>();
        both.Insert(deer);
        both.Insert(katB);
        ChefZ_CategoryClosure bothClosure;
        mgr.BuildClosure(both, bothClosure);
        if (!mgr.IsInCategory(bothClosure, katA)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 239, "!mgr.IsInCategory(bothClosure, katA)");
        if (!mgr.IsInCategory(bothClosure, katB)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 240, "!mgr.IsInCategory(bothClosure, katB)");
        if (bothClosure.CountBits() != 4) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 241, "bothClosure.CountBits() != 4");

        // Unbekannte Kategorie in der Eingabe wird still uebergangen.
        array<ChefZ_Sym> withUnknown = new array<ChefZ_Sym>();
        withUnknown.Insert(Sym("CHEFZ_ST_C_NOPE"));
        withUnknown.Insert(katB);
        ChefZ_CategoryClosure unknownClosure;
        mgr.BuildClosure(withUnknown, unknownClosure);
        if (unknownClosure.CountBits() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 249, "unknownClosure.CountBits() != 1");

        // outClosure wird wiederverwendet und dabei geleert.
        mgr.BuildClosure(up, unknownClosure);
        if (unknownClosure.CountBits() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 253, "unknownClosure.CountBits() != 1");
        if (!mgr.IsInCategory(unknownClosure, katA)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 254, "!mgr.IsInCategory(unknownClosure, katA)");
        if (mgr.IsInCategory(unknownClosure, katB)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 255, "mgr.IsInCategory(unknownClosure, katB)");

        // --- Vorfahren und Kinder -------------------------------------------
        array<ChefZ_Sym> chain;
        mgr.GetAncestors(deer, chain);
        if (chain.Count() != 2) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 260, "chain.Count() != 2");
        if (chain.Get(0) != wild) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 261, "chain.Get(0) != wild");
        if (chain.Get(1) != katA) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 262, "chain.Get(1) != katA");

        mgr.GetAncestors(katA, chain);
        if (chain.Count() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 265, "chain.Count() != 0");

        array<ChefZ_Sym> kids;
        mgr.GetChildren(katA, false, kids);
        if (kids.Count() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 269, "kids.Count() != 1");
        if (kids.Get(0) != wild) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 270, "kids.Get(0) != wild");

        mgr.GetChildren(katA, true, kids);
        if (kids.Count() != 2) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 273, "kids.Count() != 2");
        if (kids.Get(0) != wild) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 274, "kids.Get(0) != wild");
        if (kids.Get(1) != deer) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 275, "kids.Get(1) != deer");

        mgr.GetChildren(deer, true, kids);
        if (kids.Count() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 278, "kids.Count() != 0");

        // --- Anzeige --------------------------------------------------------
        if (mgr.GetDisplayKey(katA) != "Fleisch") return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 281, "mgr.GetDisplayKey(katA) != 'Fleisch'");
        if (mgr.GetDisplayKey(katB) != "CHEFZ_ST_C_FISH") return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 282, "mgr.GetDisplayKey(katB) != 'CHEFZ_ST_C_FISH'");   // Rueckfall auf die ID
        if (mgr.GetDisplayKey(Sym("CHEFZ_ST_C_NOPE")) != "") return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 283, "mgr.GetDisplayKey(Sym('CHEFZ_ST_C_NOPE')) != ''");

        // --- Tags sind flach und erben nichts (04 E3) ------------------------
        if (!mgr.TagExists(Sym("CHEFZ_ST_T_SMOKED"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 286, "!mgr.TagExists(Sym('CHEFZ_ST_T_SMOKED'))");
        if (mgr.TagExists(ChefZ_SymbolTable.Intern("CHEFZ_ST_T_NOPE"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 287, "mgr.TagExists(ChefZ_SymbolTable.Intern('CHEFZ_ST_T_NOPE'))");
        if (mgr.TagExists(ChefZ_SymbolTable.INVALID)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 288, "mgr.TagExists(ChefZ_SymbolTable.INVALID)");
        if (mgr.GetTagCount() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 289, "mgr.GetTagCount() != 1");
        // Eine Kategorie ist kein Tag, auch wenn beide dasselbe Symbol haetten.
        if (mgr.TagExists(katA)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 291, "mgr.TagExists(katA)");

        // --- Dump laeuft und liefert je Kategorie eine Zeile -----------------
        array<string> lines;
        mgr.DumpTree(lines);
        if (lines.Count() < 6) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 296, "lines.Count() < 6");

        // --- Zweiter Build verwirft den alten Bestand vollstaendig -----------
        ChefZ_Registry<ChefZ_CategoryDef> cats2 = NewCatRegistry();
        if (!AddCat(cats2, "CHEFZ_ST_C_SOLO", "", "Solo")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 300, "!AddCat(cats2, 'CHEFZ_ST_C_SOLO', '', 'Solo')");
        mgr.Build(cats2, null, NewReport());
        if (mgr.GetCategoryCount() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 302, "mgr.GetCategoryCount() != 1");
        if (mgr.Exists(katA)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 303, "mgr.Exists(katA)");
        if (mgr.GetMaxDepth() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 304, "mgr.GetMaxDepth() != 0");
        if (mgr.GetTagCount() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 305, "mgr.GetTagCount() != 0");

        return true;
    }

    //==========================================================================
    // 2. Zyklen
    //==========================================================================

    /**
     * 04 §6: "Zyklus A -> B -> A: ALLE Kategorien des Zyklus verworfen, ERROR
     * mit vollem Pfad. Kategorien ausserhalb des Zyklus bleiben gueltig."
     *
     * Geprueft wird zusaetzlich der Fall, den die Zyklenerkennung leicht
     * uebersieht: ein Kind, das AN einem Zyklus haengt, ohne selbst darin zu
     * liegen. Es muss ueberleben und zur Wurzel werden.
     */
    private static bool CycleCheck()
    {
        ChefZ_Registry<ChefZ_CategoryDef> cats = NewCatRegistry();
        if (!AddCat(cats, "CHEFZ_ST_Y_A",     "CHEFZ_ST_Y_C",   "A")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 325, "!AddCat(cats, 'CHEFZ_ST_Y_A', 'CHEFZ_ST_Y_C', 'A')");
        if (!AddCat(cats, "CHEFZ_ST_Y_B",     "CHEFZ_ST_Y_A",   "B")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 326, "!AddCat(cats, 'CHEFZ_ST_Y_B', 'CHEFZ_ST_Y_A', 'B')");
        if (!AddCat(cats, "CHEFZ_ST_Y_C",     "CHEFZ_ST_Y_B",   "C")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 327, "!AddCat(cats, 'CHEFZ_ST_Y_C', 'CHEFZ_ST_Y_B', 'C')");
        if (!AddCat(cats, "CHEFZ_ST_Y_LEAF",  "CHEFZ_ST_Y_A",   "Blatt")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 328, "!AddCat(cats, 'CHEFZ_ST_Y_LEAF', 'CHEFZ_ST_Y_A', 'Blatt')");
        if (!AddCat(cats, "CHEFZ_ST_Y_SOLID", "",               "Fest")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 329, "!AddCat(cats, 'CHEFZ_ST_Y_SOLID', '', 'Fest')");
        if (!AddCat(cats, "CHEFZ_ST_Y_KID",   "CHEFZ_ST_Y_SOLID", "Kind")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 330, "!AddCat(cats, 'CHEFZ_ST_Y_KID', 'CHEFZ_ST_Y_SOLID', 'Kind')");
        if (!AddCat(cats, "CHEFZ_ST_Y_SELF",  "CHEFZ_ST_Y_SELF", "Selbst")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 331, "!AddCat(cats, 'CHEFZ_ST_Y_SELF', 'CHEFZ_ST_Y_SELF', 'Selbst')");

        ChefZ_LoadReport report = NewReport();
        ChefZ_CategoryManager mgr = NewManager();
        mgr.Build(cats, null, report);

        if (!mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 337, "!mgr.IsReady()");

        // Vier verworfene: A, B, C und die Selbstschleife.
        if (mgr.GetRejectedCount() != 4) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 340, "mgr.GetRejectedCount() != 4");
        if (report.ErrorCount() != 4) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 341, "report.ErrorCount() != 4");

        if (mgr.Exists(Sym("CHEFZ_ST_Y_A"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 343, "mgr.Exists(Sym('CHEFZ_ST_Y_A'))");
        if (mgr.Exists(Sym("CHEFZ_ST_Y_B"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 344, "mgr.Exists(Sym('CHEFZ_ST_Y_B'))");
        if (mgr.Exists(Sym("CHEFZ_ST_Y_C"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 345, "mgr.Exists(Sym('CHEFZ_ST_Y_C'))");
        if (mgr.Exists(Sym("CHEFZ_ST_Y_SELF"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 346, "mgr.Exists(Sym('CHEFZ_ST_Y_SELF'))");

        // Ausserhalb des Zyklus bleibt alles gueltig.
        if (!mgr.Exists(Sym("CHEFZ_ST_Y_SOLID"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 349, "!mgr.Exists(Sym('CHEFZ_ST_Y_SOLID'))");
        if (!mgr.Exists(Sym("CHEFZ_ST_Y_KID"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 350, "!mgr.Exists(Sym('CHEFZ_ST_Y_KID'))");
        if (mgr.GetDepth(Sym("CHEFZ_ST_Y_KID")) != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 351, "mgr.GetDepth(Sym('CHEFZ_ST_Y_KID')) != 1");

        // Das Blatt am Zyklus ueberlebt als Wurzel.
        ChefZ_Sym leaf = Sym("CHEFZ_ST_Y_LEAF");
        if (!mgr.Exists(leaf)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 355, "!mgr.Exists(leaf)");
        if (mgr.GetDepth(leaf) != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 356, "mgr.GetDepth(leaf) != 0");
        if (mgr.GetParent(leaf) != ChefZ_SymbolTable.INVALID) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 357, "mgr.GetParent(leaf) != ChefZ_SymbolTable.INVALID");
        if (mgr.GetCategoryCount() != 3) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 358, "mgr.GetCategoryCount() != 3");

        // Und die entscheidende Zusicherung: keine Abfrage laeuft endlos.
        array<ChefZ_Sym> direct = new array<ChefZ_Sym>();
        direct.Insert(leaf);
        ChefZ_CategoryClosure closure;
        mgr.BuildClosure(direct, closure);
        if (closure.CountBits() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 365, "closure.CountBits() != 1");
        if (mgr.IsInCategory(closure, Sym("CHEFZ_ST_Y_A"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 366, "mgr.IsInCategory(closure, Sym('CHEFZ_ST_Y_A'))");

        array<ChefZ_Sym> chain;
        mgr.GetAncestors(leaf, chain);
        if (chain.Count() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 370, "chain.Count() != 0");

        array<ChefZ_Sym> kids;
        mgr.GetChildren(Sym("CHEFZ_ST_Y_SOLID"), true, kids);
        if (kids.Count() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 374, "kids.Count() != 1");

        return true;
    }

    //==========================================================================
    // 3. Deckel
    //==========================================================================

    //! 04 §6: "Mehr als maxCategories: Ueberzaehlige abgewiesen, ERROR."
    private static bool LimitCheck()
    {
        ChefZ_Registry<ChefZ_CategoryDef> cats = NewCatRegistry();
        if (!AddCat(cats, "CHEFZ_ST_L_A", "", "A")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 387, "!AddCat(cats, 'CHEFZ_ST_L_A', '', 'A')");
        if (!AddCat(cats, "CHEFZ_ST_L_B", "", "B")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 388, "!AddCat(cats, 'CHEFZ_ST_L_B', '', 'B')");
        if (!AddCat(cats, "CHEFZ_ST_L_C", "", "C")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 389, "!AddCat(cats, 'CHEFZ_ST_L_C', '', 'C')");

        ChefZ_LoadReport report = NewReport();
        ChefZ_CategoryManager mgr = NewManager();
        mgr.SetMaxCategoriesForTest(2);
        mgr.Build(cats, null, report);

        if (mgr.GetCategoryCount() != 2) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 396, "mgr.GetCategoryCount() != 2");
        if (mgr.GetRejectedCount() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 397, "mgr.GetRejectedCount() != 1");
        if (report.ErrorCount() != 1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 398, "report.ErrorCount() != 1");

        // Abgewiesen wird deterministisch die letzte in ID-Reihenfolge.
        if (!mgr.Exists(Sym("CHEFZ_ST_L_A"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 401, "!mgr.Exists(Sym('CHEFZ_ST_L_A'))");
        if (!mgr.Exists(Sym("CHEFZ_ST_L_B"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 402, "!mgr.Exists(Sym('CHEFZ_ST_L_B'))");
        if (mgr.Exists(Sym("CHEFZ_ST_L_C"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 403, "mgr.Exists(Sym('CHEFZ_ST_L_C'))");

        return true;
    }

    //==========================================================================
    // 4. Leerer Baum
    //==========================================================================

    /**
     * 04 §6, erste Zeile: keine Kategorien ist KEIN Fehler. Der Kernpunkt fuer
     * Invariante I2 - ohne Kategorien matcht kein Kategorierezept, und das
     * Kochen bleibt vollstaendig Vanilla.
     */
    private static bool EmptyCheck()
    {
        ChefZ_LoadReport report = NewReport();
        ChefZ_CategoryManager mgr = NewManager();
        mgr.Build(NewCatRegistry(), NewTagRegistry(), report);

        if (!mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 423, "!mgr.IsReady()");
        if (report.ErrorCount() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 424, "report.ErrorCount() != 0");
        if (report.WarnCount() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 425, "report.WarnCount() != 0");
        if (mgr.GetCategoryCount() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 426, "mgr.GetCategoryCount() != 0");
        if (mgr.GetMaxDepth() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 427, "mgr.GetMaxDepth() != 0");

        ChefZ_Sym any = ChefZ_SymbolTable.Intern("CHEFZ_ST_E_ANY");
        if (mgr.Exists(any)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 430, "mgr.Exists(any)");
        if (mgr.GetBitIndex(any) != -1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 431, "mgr.GetBitIndex(any) != -1");
        if (mgr.GetDepth(any) != -1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 432, "mgr.GetDepth(any) != -1");
        if (mgr.GetSpecificityWeight(any) != 0.0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 433, "mgr.GetSpecificityWeight(any) != 0.0");
        if (mgr.TagExists(any)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 434, "mgr.TagExists(any)");

        array<ChefZ_Sym> direct = new array<ChefZ_Sym>();
        direct.Insert(any);
        ChefZ_CategoryClosure closure;
        mgr.BuildClosure(direct, closure);
        if (!closure) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 440, "!closure");
        if (!closure.IsEmpty()) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 441, "!closure.IsEmpty()");
        if (mgr.IsInCategory(closure, any)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 442, "mgr.IsInCategory(closure, any)");

        // Auch mit null-Registries: kein Nullzugriff.
        ChefZ_CategoryManager mgr2 = NewManager();
        mgr2.Build(null, null, null);
        if (!mgr2.IsReady()) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 447, "!mgr2.IsReady()");
        if (mgr2.GetCategoryCount() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 448, "mgr2.GetCategoryCount() != 0");

        return true;
    }

    //==========================================================================
    // 5. Abfrage vor Build
    //==========================================================================

    //! 04 §6, letzte Zeile: "false bzw. -1, einmaliger ERROR. Kein
    //! Nullzugriff." Die Meldung ist hier stummgeschaltet (SetQuietForTest),
    //! sonst staende im RPT ein Fehler, den es nicht gibt.
    private static bool NotReadyCheck()
    {
        ChefZ_CategoryManager mgr = NewManager();
        if (mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 463, "mgr.IsReady()");

        ChefZ_Sym any = ChefZ_SymbolTable.Intern("CHEFZ_ST_N_ANY");
        if (mgr.GetBitIndex(any) != -1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 466, "mgr.GetBitIndex(any) != -1");
        if (mgr.Exists(any)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 467, "mgr.Exists(any)");
        if (mgr.GetDepth(any) != -1) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 468, "mgr.GetDepth(any) != -1");
        if (mgr.GetParent(any) != ChefZ_SymbolTable.INVALID) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 469, "mgr.GetParent(any) != ChefZ_SymbolTable.INVALID");
        if (mgr.GetSpecificityWeight(any) != 0.0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 470, "mgr.GetSpecificityWeight(any) != 0.0");
        if (mgr.GetDisplayKey(any) != "") return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 471, "mgr.GetDisplayKey(any) != ''");
        if (mgr.GetCategoryCount() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 472, "mgr.GetCategoryCount() != 0");

        ChefZ_CategoryClosure closure = new ChefZ_CategoryClosure();
        closure.SetBit(0);
        if (mgr.IsInCategory(closure, any)) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 476, "mgr.IsInCategory(closure, any)");

        array<ChefZ_Sym> direct = new array<ChefZ_Sym>();
        direct.Insert(any);
        ChefZ_CategoryClosure built;
        mgr.BuildClosure(direct, built);
        if (!built) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 482, "!built");
        if (!built.IsEmpty()) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 483, "!built.IsEmpty()");

        array<ChefZ_Sym> kids;
        mgr.GetChildren(any, true, kids);
        if (kids.Count() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 487, "kids.Count() != 0");

        array<ChefZ_Sym> chain;
        mgr.GetAncestors(any, chain);
        if (chain.Count() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 491, "chain.Count() != 0");

        // Nach Reset() ist der Manager wieder "nicht gebaut".
        ChefZ_Registry<ChefZ_CategoryDef> cats = NewCatRegistry();
        if (!AddCat(cats, "CHEFZ_ST_N_ONE", "", "Eins")) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 495, "!AddCat(cats, 'CHEFZ_ST_N_ONE', '', 'Eins')");
        mgr.Build(cats, null, NewReport());
        if (!mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 497, "!mgr.IsReady()");
        if (!mgr.Exists(Sym("CHEFZ_ST_N_ONE"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 498, "!mgr.Exists(Sym('CHEFZ_ST_N_ONE'))");

        mgr.Reset();
        if (mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 501, "mgr.IsReady()");
        if (mgr.Exists(Sym("CHEFZ_ST_N_ONE"))) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 502, "mgr.Exists(Sym('CHEFZ_ST_N_ONE'))");
        if (mgr.GetCategoryCount() != 0) return ChefZ_SelfTestTrace.Fail("CategorySelfTest", 503, "mgr.GetCategoryCount() != 0");

        return true;
    }
}
