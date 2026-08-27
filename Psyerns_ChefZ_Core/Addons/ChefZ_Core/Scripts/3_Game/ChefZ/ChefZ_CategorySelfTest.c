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
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.CONFIG, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.CONFIG,
            "Selbsttest " + name + " FEHLGESCHLAGEN. Der Kategoriebaum verhaelt sich nicht wie "
            + "entworfen - jede Kategorieabfrage und damit jede Rezeptauswahl ist ab hier "
            + "unzuverlaessig.");
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

    private static bool AddCat(notnull ChefZ_Registry<ChefZ_CategoryDef> reg,
                               string id, string parent, string displayName)
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
        if (!AddCat(cats, "CHEFZ_ST_C_MEAT",   "",                  "Fleisch"))  return false;
        if (!AddCat(cats, "CHEFZ_ST_C_WILD",   "CHEFZ_ST_C_MEAT",   "Wild"))     return false;
        if (!AddCat(cats, "CHEFZ_ST_C_DEER",   "CHEFZ_ST_C_WILD",   "Hirsch"))   return false;
        if (!AddCat(cats, "CHEFZ_ST_C_FISH",   "",                  ""))         return false;
        if (!AddCat(cats, "CHEFZ_ST_C_ORPHAN", "CHEFZ_ST_C_NOPE",   "Waise"))    return false;

        ChefZ_Registry<ChefZ_TagDef> tags = NewTagRegistry();
        if (!AddTag(tags, "CHEFZ_ST_T_SMOKED"))                                  return false;

        ChefZ_LoadReport report = NewReport();
        ChefZ_CategoryManager mgr = NewManager();
        mgr.Build(cats, tags, report);

        if (!mgr.IsReady())                                     return false;
        if (mgr.GetCategoryCount() != 5)                        return false;
        if (mgr.GetMaxDepth() != 2)                             return false;
        if (mgr.GetRejectedCount() != 0)                        return false;

        // Unbekanntes parent ist eine WARNUNG, kein Fehler (04 §6).
        if (report.ErrorCount() != 0)                           return false;
        if (report.WarnCount() != 1)                            return false;

        ChefZ_Sym katA   = Sym("CHEFZ_ST_C_MEAT");
        ChefZ_Sym wild   = Sym("CHEFZ_ST_C_WILD");
        ChefZ_Sym deer   = Sym("CHEFZ_ST_C_DEER");
        ChefZ_Sym katB   = Sym("CHEFZ_ST_C_FISH");
        ChefZ_Sym orphan = Sym("CHEFZ_ST_C_ORPHAN");

        // --- Existenz und Tiefe -------------------------------------------
        if (!mgr.Exists(katA))                                  return false;
        if (mgr.Exists(ChefZ_SymbolTable.Intern("CHEFZ_ST_C_NOPE")))   return false;
        if (mgr.Exists(ChefZ_SymbolTable.INVALID))              return false;

        if (mgr.GetDepth(katA)   != 0)                          return false;
        if (mgr.GetDepth(wild)   != 1)                          return false;
        if (mgr.GetDepth(deer)   != 2)                          return false;
        if (mgr.GetDepth(katB)   != 0)                          return false;
        if (mgr.GetDepth(orphan) != 0)                          return false;   // Wurzel geworden
        if (mgr.GetDepth(Sym("CHEFZ_ST_C_NOPE")) != -1)         return false;

        // --- Eltern --------------------------------------------------------
        if (mgr.GetParent(deer) != wild)                        return false;
        if (mgr.GetParent(wild) != katA)                        return false;
        if (mgr.GetParent(katA) != ChefZ_SymbolTable.INVALID)   return false;
        if (mgr.GetParent(orphan) != ChefZ_SymbolTable.INVALID) return false;

        // --- Spezifitaetsgewicht, 04 E4 ------------------------------------
        if (mgr.GetSpecificityWeight(katA) != 1.0)              return false;
        if (mgr.GetSpecificityWeight(wild) != 1.5)              return false;
        if (mgr.GetSpecificityWeight(deer) != 2.0)              return false;
        if (mgr.GetSpecificityWeight(Sym("CHEFZ_ST_C_NOPE")) != 0.0) return false;

        // --- Bitindizes: dicht, verschieden --------------------------------
        int bKatA = mgr.GetBitIndex(katA);
        int bWild = mgr.GetBitIndex(wild);
        int bDeer = mgr.GetBitIndex(deer);
        if (bKatA < 0 || bWild < 0 || bDeer < 0)                return false;
        if (bKatA == bWild || bWild == bDeer || bKatA == bDeer) return false;
        if (bKatA >= 5 || bWild >= 5 || bDeer >= 5)             return false;
        if (mgr.GetBitIndex(Sym("CHEFZ_ST_C_NOPE")) != -1)      return false;

        // --- Closure: self-or-ancestor (04 E1) ------------------------------
        array<ChefZ_Sym> direct = new array<ChefZ_Sym>();
        direct.Insert(deer);

        ChefZ_CategoryClosure closure;
        mgr.BuildClosure(direct, closure);
        if (!closure)                                           return false;

        if (!mgr.IsInCategory(closure, deer))                   return false;   // sich selbst
        if (!mgr.IsInCategory(closure, wild))                   return false;   // Elternteil
        if (!mgr.IsInCategory(closure, katA))                   return false;   // Grosselternteil
        if (mgr.IsInCategory(closure, katB))                    return false;   // fremder Ast
        if (mgr.IsInCategory(closure, orphan))                  return false;
        if (mgr.IsInCategory(closure, Sym("CHEFZ_ST_C_NOPE")))  return false;   // unbekannt
        if (mgr.IsInCategory(closure, ChefZ_SymbolTable.INVALID)) return false;
        if (closure.CountBits() != 3)                           return false;

        // Vererbung wirkt NUR nach oben: MEAT erfuellt nicht DEER.
        array<ChefZ_Sym> up = new array<ChefZ_Sym>();
        up.Insert(katA);
        ChefZ_CategoryClosure upClosure;
        mgr.BuildClosure(up, upClosure);
        if (!mgr.IsInCategory(upClosure, katA))                 return false;
        if (mgr.IsInCategory(upClosure, deer))                  return false;
        if (upClosure.CountBits() != 1)                         return false;

        // Mehrere direkte Kategorien vereinigen sich.
        array<ChefZ_Sym> both = new array<ChefZ_Sym>();
        both.Insert(deer);
        both.Insert(katB);
        ChefZ_CategoryClosure bothClosure;
        mgr.BuildClosure(both, bothClosure);
        if (!mgr.IsInCategory(bothClosure, katA))               return false;
        if (!mgr.IsInCategory(bothClosure, katB))               return false;
        if (bothClosure.CountBits() != 4)                       return false;

        // Unbekannte Kategorie in der Eingabe wird still uebergangen.
        array<ChefZ_Sym> withUnknown = new array<ChefZ_Sym>();
        withUnknown.Insert(Sym("CHEFZ_ST_C_NOPE"));
        withUnknown.Insert(katB);
        ChefZ_CategoryClosure unknownClosure;
        mgr.BuildClosure(withUnknown, unknownClosure);
        if (unknownClosure.CountBits() != 1)                    return false;

        // outClosure wird wiederverwendet und dabei geleert.
        mgr.BuildClosure(up, unknownClosure);
        if (unknownClosure.CountBits() != 1)                    return false;
        if (!mgr.IsInCategory(unknownClosure, katA))            return false;
        if (mgr.IsInCategory(unknownClosure, katB))             return false;

        // --- Vorfahren und Kinder -------------------------------------------
        array<ChefZ_Sym> chain;
        mgr.GetAncestors(deer, chain);
        if (chain.Count() != 2)                                 return false;
        if (chain.Get(0) != wild)                               return false;
        if (chain.Get(1) != katA)                               return false;

        mgr.GetAncestors(katA, chain);
        if (chain.Count() != 0)                                 return false;

        array<ChefZ_Sym> kids;
        mgr.GetChildren(katA, false, kids);
        if (kids.Count() != 1)                                  return false;
        if (kids.Get(0) != wild)                                return false;

        mgr.GetChildren(katA, true, kids);
        if (kids.Count() != 2)                                  return false;
        if (kids.Get(0) != wild)                                return false;
        if (kids.Get(1) != deer)                                return false;

        mgr.GetChildren(deer, true, kids);
        if (kids.Count() != 0)                                  return false;

        // --- Anzeige --------------------------------------------------------
        if (mgr.GetDisplayKey(katA) != "Fleisch")               return false;
        if (mgr.GetDisplayKey(katB) != "CHEFZ_ST_C_FISH")       return false;   // Rueckfall auf die ID
        if (mgr.GetDisplayKey(Sym("CHEFZ_ST_C_NOPE")) != "")    return false;

        // --- Tags sind flach und erben nichts (04 E3) ------------------------
        if (!mgr.TagExists(Sym("CHEFZ_ST_T_SMOKED")))           return false;
        if (mgr.TagExists(ChefZ_SymbolTable.Intern("CHEFZ_ST_T_NOPE")))  return false;
        if (mgr.TagExists(ChefZ_SymbolTable.INVALID))           return false;
        if (mgr.GetTagCount() != 1)                             return false;
        // Eine Kategorie ist kein Tag, auch wenn beide dasselbe Symbol haetten.
        if (mgr.TagExists(katA))                                return false;

        // --- Dump laeuft und liefert je Kategorie eine Zeile -----------------
        array<string> lines;
        mgr.DumpTree(lines);
        if (lines.Count() < 6)                                  return false;

        // --- Zweiter Build verwirft den alten Bestand vollstaendig -----------
        ChefZ_Registry<ChefZ_CategoryDef> cats2 = NewCatRegistry();
        if (!AddCat(cats2, "CHEFZ_ST_C_SOLO", "", "Solo"))      return false;
        mgr.Build(cats2, null, NewReport());
        if (mgr.GetCategoryCount() != 1)                        return false;
        if (mgr.Exists(katA))                                   return false;
        if (mgr.GetMaxDepth() != 0)                             return false;
        if (mgr.GetTagCount() != 0)                             return false;

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
        if (!AddCat(cats, "CHEFZ_ST_Y_A",     "CHEFZ_ST_Y_C",   "A"))       return false;
        if (!AddCat(cats, "CHEFZ_ST_Y_B",     "CHEFZ_ST_Y_A",   "B"))       return false;
        if (!AddCat(cats, "CHEFZ_ST_Y_C",     "CHEFZ_ST_Y_B",   "C"))       return false;
        if (!AddCat(cats, "CHEFZ_ST_Y_LEAF",  "CHEFZ_ST_Y_A",   "Blatt"))   return false;
        if (!AddCat(cats, "CHEFZ_ST_Y_SOLID", "",               "Fest"))    return false;
        if (!AddCat(cats, "CHEFZ_ST_Y_KID",   "CHEFZ_ST_Y_SOLID", "Kind"))  return false;
        if (!AddCat(cats, "CHEFZ_ST_Y_SELF",  "CHEFZ_ST_Y_SELF", "Selbst")) return false;

        ChefZ_LoadReport report = NewReport();
        ChefZ_CategoryManager mgr = NewManager();
        mgr.Build(cats, null, report);

        if (!mgr.IsReady())                                     return false;

        // Vier verworfene: A, B, C und die Selbstschleife.
        if (mgr.GetRejectedCount() != 4)                        return false;
        if (report.ErrorCount() != 4)                           return false;

        if (mgr.Exists(Sym("CHEFZ_ST_Y_A")))                    return false;
        if (mgr.Exists(Sym("CHEFZ_ST_Y_B")))                    return false;
        if (mgr.Exists(Sym("CHEFZ_ST_Y_C")))                    return false;
        if (mgr.Exists(Sym("CHEFZ_ST_Y_SELF")))                 return false;

        // Ausserhalb des Zyklus bleibt alles gueltig.
        if (!mgr.Exists(Sym("CHEFZ_ST_Y_SOLID")))               return false;
        if (!mgr.Exists(Sym("CHEFZ_ST_Y_KID")))                 return false;
        if (mgr.GetDepth(Sym("CHEFZ_ST_Y_KID")) != 1)           return false;

        // Das Blatt am Zyklus ueberlebt als Wurzel.
        ChefZ_Sym leaf = Sym("CHEFZ_ST_Y_LEAF");
        if (!mgr.Exists(leaf))                                  return false;
        if (mgr.GetDepth(leaf) != 0)                            return false;
        if (mgr.GetParent(leaf) != ChefZ_SymbolTable.INVALID)   return false;
        if (mgr.GetCategoryCount() != 3)                        return false;

        // Und die entscheidende Zusicherung: keine Abfrage laeuft endlos.
        array<ChefZ_Sym> direct = new array<ChefZ_Sym>();
        direct.Insert(leaf);
        ChefZ_CategoryClosure closure;
        mgr.BuildClosure(direct, closure);
        if (closure.CountBits() != 1)                           return false;
        if (mgr.IsInCategory(closure, Sym("CHEFZ_ST_Y_A")))     return false;

        array<ChefZ_Sym> chain;
        mgr.GetAncestors(leaf, chain);
        if (chain.Count() != 0)                                 return false;

        array<ChefZ_Sym> kids;
        mgr.GetChildren(Sym("CHEFZ_ST_Y_SOLID"), true, kids);
        if (kids.Count() != 1)                                  return false;

        return true;
    }

    //==========================================================================
    // 3. Deckel
    //==========================================================================

    //! 04 §6: "Mehr als maxCategories: Ueberzaehlige abgewiesen, ERROR."
    private static bool LimitCheck()
    {
        ChefZ_Registry<ChefZ_CategoryDef> cats = NewCatRegistry();
        if (!AddCat(cats, "CHEFZ_ST_L_A", "", "A"))             return false;
        if (!AddCat(cats, "CHEFZ_ST_L_B", "", "B"))             return false;
        if (!AddCat(cats, "CHEFZ_ST_L_C", "", "C"))             return false;

        ChefZ_LoadReport report = NewReport();
        ChefZ_CategoryManager mgr = NewManager();
        mgr.SetMaxCategoriesForTest(2);
        mgr.Build(cats, null, report);

        if (mgr.GetCategoryCount() != 2)                        return false;
        if (mgr.GetRejectedCount() != 1)                        return false;
        if (report.ErrorCount() != 1)                           return false;

        // Abgewiesen wird deterministisch die letzte in ID-Reihenfolge.
        if (!mgr.Exists(Sym("CHEFZ_ST_L_A")))                   return false;
        if (!mgr.Exists(Sym("CHEFZ_ST_L_B")))                   return false;
        if (mgr.Exists(Sym("CHEFZ_ST_L_C")))                    return false;

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

        if (!mgr.IsReady())                                     return false;
        if (report.ErrorCount() != 0)                           return false;
        if (report.WarnCount() != 0)                            return false;
        if (mgr.GetCategoryCount() != 0)                        return false;
        if (mgr.GetMaxDepth() != 0)                             return false;

        ChefZ_Sym any = ChefZ_SymbolTable.Intern("CHEFZ_ST_E_ANY");
        if (mgr.Exists(any))                                    return false;
        if (mgr.GetBitIndex(any) != -1)                         return false;
        if (mgr.GetDepth(any) != -1)                            return false;
        if (mgr.GetSpecificityWeight(any) != 0.0)               return false;
        if (mgr.TagExists(any))                                 return false;

        array<ChefZ_Sym> direct = new array<ChefZ_Sym>();
        direct.Insert(any);
        ChefZ_CategoryClosure closure;
        mgr.BuildClosure(direct, closure);
        if (!closure)                                           return false;
        if (!closure.IsEmpty())                                 return false;
        if (mgr.IsInCategory(closure, any))                     return false;

        // Auch mit null-Registries: kein Nullzugriff.
        ChefZ_CategoryManager mgr2 = NewManager();
        mgr2.Build(null, null, null);
        if (!mgr2.IsReady())                                    return false;
        if (mgr2.GetCategoryCount() != 0)                       return false;

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
        if (mgr.IsReady())                                      return false;

        ChefZ_Sym any = ChefZ_SymbolTable.Intern("CHEFZ_ST_N_ANY");
        if (mgr.GetBitIndex(any) != -1)                         return false;
        if (mgr.Exists(any))                                    return false;
        if (mgr.GetDepth(any) != -1)                            return false;
        if (mgr.GetParent(any) != ChefZ_SymbolTable.INVALID)    return false;
        if (mgr.GetSpecificityWeight(any) != 0.0)               return false;
        if (mgr.GetDisplayKey(any) != "")                       return false;
        if (mgr.GetCategoryCount() != 0)                        return false;

        ChefZ_CategoryClosure closure = new ChefZ_CategoryClosure();
        closure.SetBit(0);
        if (mgr.IsInCategory(closure, any))                     return false;

        array<ChefZ_Sym> direct = new array<ChefZ_Sym>();
        direct.Insert(any);
        ChefZ_CategoryClosure built;
        mgr.BuildClosure(direct, built);
        if (!built)                                             return false;
        if (!built.IsEmpty())                                   return false;

        array<ChefZ_Sym> kids;
        mgr.GetChildren(any, true, kids);
        if (kids.Count() != 0)                                  return false;

        array<ChefZ_Sym> chain;
        mgr.GetAncestors(any, chain);
        if (chain.Count() != 0)                                 return false;

        // Nach Reset() ist der Manager wieder "nicht gebaut".
        ChefZ_Registry<ChefZ_CategoryDef> cats = NewCatRegistry();
        if (!AddCat(cats, "CHEFZ_ST_N_ONE", "", "Eins"))        return false;
        mgr.Build(cats, null, NewReport());
        if (!mgr.IsReady())                                     return false;
        if (!mgr.Exists(Sym("CHEFZ_ST_N_ONE")))                 return false;

        mgr.Reset();
        if (mgr.IsReady())                                      return false;
        if (mgr.Exists(Sym("CHEFZ_ST_N_ONE")))                  return false;
        if (mgr.GetCategoryCount() != 0)                        return false;

        return true;
    }
}
