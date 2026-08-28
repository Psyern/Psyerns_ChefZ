//==============================================================================
// ChefZ_IngredientSelfTest - Abnahmepruefung fuer S4
//
// Entwurf: 19 S4, 05 §6 (Einheiten), 05 §7 (Fehlerverhalten, Zeile fuer
// Zeile), 05 E2 (Vererbung entlang der Elternkette), 05 E3, 07 E4
// (selectivityHint).
//
// Geprueft wird, was spaeter niemand mehr von aussen sieht: ob ein abgeleiteter
// Record wirklich erbt, ob eine unbekannte Kategorie nur SICH SELBST verliert,
// ob der Rueckwaertsindex Oberkategorien mitzaehlt, und ob ein Nenner von Null
// abgewiesen statt geteilt wird.
//
// Der Test laeuft auf EIGENEN Instanzen, nie auf den Singletons, mit einem
// Ladebericht ohne Log-Spiegelung, und legt ausschliesslich Symbole mit dem
// Praefix "CHEFZ_ST_" an. Er beruehrt kein Item, keine Datei, keine
// Vanilla-Logik.
//
// Die Elternkette kommt im Test aus einer Tabelle statt aus CfgVehicles
// (ChefZ_IngredientProbe). Anders waere 05 E2 nur auf einem laufenden Server
// mit echtem Content pruefbar - also praktisch gar nicht.
//
// Layer: 3_Game.
//==============================================================================

/**
 * Manager mit ersetzter Elternaufloesung.
 *
 * Die einzige Ueberschreibung ist ResolveConfigParent(). Alles andere - die
 * Vererbungsregel selbst, die Reihenfolge, die Pruefungen - ist der echte
 * Code. Ein Test, der die Regel nachbaute, statt sie auszufuehren, wuerde
 * seinen eigenen Nachbau pruefen.
 */
class ChefZ_IngredientProbe extends ChefZ_IngredientManager
{
    private ref map<string, string> m_Parents;

    void ChefZ_IngredientProbe()
    {
        m_Parents = new map<string, string>();
        SetQuietForTest(true);
        SetSkipClassExistsCheckForTest(true);
    }

    void SetParent(string child, string parent)
    {
        m_Parents.Set(child, parent);
    }

    protected override bool ResolveConfigParent(string className, out string parentName)
    {
        parentName = "";
        return m_Parents.Find(className, parentName);
    }
}

//==============================================================================

class ChefZ_IngredientSelfTest
{
    //--- Testvokabular. Praefix CHEFZ_ST_, damit es mit keinem Content
    //--- kollidieren kann; die Namen sind Platzhalter und bedeuten nichts.
    static const string C_FOOD = "CHEFZ_ST_IC_FOOD";
    static const string C_KAT_A = "CHEFZ_ST_IC_MEAT";
    static const string C_WILD = "CHEFZ_ST_IC_WILD";
    static const string C_NOPE = "CHEFZ_ST_IC_NOPE";

    static const string T_A    = "CHEFZ_ST_IT_A";
    static const string T_B    = "CHEFZ_ST_IT_B";
    static const string T_NOPE = "CHEFZ_ST_IT_NOPE";

    static const string S_KNOWN = "CHEFZ_ST_IS_KNOWN";
    static const string S_NOPE  = "CHEFZ_ST_IS_NOPE";

    //! Frei erfundene Mengeneinheit. Bewusst NICHT "GRAM": der Core kennt nur
    //! seine Standardeinheit, jede weitere ist Content.
    static const string U_BULK  = "CHEFZ_ST_IU_BULK";

    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("Snapshot",   ChefZ_FactSnapshot.SelfCheck());
        Check("Stammdaten", BasicCheck());
        Check("Vererbung",  InheritanceCheck());
        Check("Index",      IndexCheck());
        Check("Einheiten",  UnitCheck());
        Check("Referenzen", ReferenceCheck());
        Check("Leer",       EmptyCheck());
        Check("VorBuild",   NotReadyCheck());

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
        ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, "Selbsttest " + name + " FEHLGESCHLAGEN. Die Zutatenaufloesung verhaelt sich nicht " + "wie entworfen - jede Faktenerhebung und damit jede Rezeptauswahl ist ab hier " + "unzuverlaessig.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S4: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
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

    private static ChefZ_LoadReport NewReport()
    {
        ChefZ_LoadReport report = new ChefZ_LoadReport();
        report.SetMirrorToLog(false);       // erwartete Fehler gehoeren nicht ins RPT
        return report;
    }

    private static ChefZ_Sym Sym(string id)
    {
        return ChefZ_SymbolTable.Lookup(id);
    }

    /**
     * Kategoriebaum des Tests:
     *
     *   CHEFZ_ST_IC_FOOD
     *     +-- CHEFZ_ST_IC_MEAT
     *           +-- CHEFZ_ST_IC_WILD
     *
     * plus die flachen Tags CHEFZ_ST_IT_A und CHEFZ_ST_IT_B.
     */
    private static ChefZ_CategoryManager NewCategories()
    {
        ChefZ_Registry<ChefZ_CategoryDef> cats = new ChefZ_Registry<ChefZ_CategoryDef>();
        cats.Init(ChefZ_RecordKind.CATEGORY);
        AddCat(cats, C_FOOD, "");
        AddCat(cats, C_KAT_A, C_FOOD);
        AddCat(cats, C_WILD, C_KAT_A);

        ChefZ_Registry<ChefZ_TagDef> tags = new ChefZ_Registry<ChefZ_TagDef>();
        tags.Init(ChefZ_RecordKind.TAG);
        AddTag(tags, T_A);
        AddTag(tags, T_B);

        ChefZ_CategoryManager mgr = new ChefZ_CategoryManager();
        mgr.SetQuietForTest(true);
        mgr.SetMaxCategoriesForTest(64);
        mgr.Build(cats, tags, NewReport());
        return mgr;
    }

    private static void AddCat(notnull ChefZ_Registry<ChefZ_CategoryDef> reg, string id, string parent)
    {
        ChefZ_CategoryDef def = new ChefZ_CategoryDef();
        def.id     = id;
        def.parent = parent;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.Compile(null);
        reg.Add(def);
    }

    private static void AddTag(notnull ChefZ_Registry<ChefZ_TagDef> reg, string id)
    {
        ChefZ_TagDef def = new ChefZ_TagDef();
        def.id = id;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.Compile(null);
        reg.Add(def);
    }

    private static ChefZ_Registry<ChefZ_StateDef> NewStates()
    {
        ChefZ_Registry<ChefZ_StateDef> reg = new ChefZ_Registry<ChefZ_StateDef>();
        reg.Init(ChefZ_RecordKind.STATE);

        ChefZ_StateDef def = new ChefZ_StateDef();
        def.id = S_KNOWN;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.Compile(null);
        reg.Add(def);
        return reg;
    }

    private static ChefZ_Registry<ChefZ_IngredientDef> NewIngredients()
    {
        ChefZ_Registry<ChefZ_IngredientDef> reg = new ChefZ_Registry<ChefZ_IngredientDef>();
        reg.Init(ChefZ_RecordKind.INGREDIENT);
        return reg;
    }

    //! Leerer Record, bereits kompiliert und aufgenommen. Der Aufrufer setzt
    //! die Felder, um die es ihm geht - alles andere bleibt "nicht gesetzt".
    private static ChefZ_IngredientDef NewIng(notnull ChefZ_Registry<ChefZ_IngredientDef> reg, string id)
    {
        // Die Bool-Sonde steht sonst womoeglich noch auf "hoch" und wuerde
        // "decays" als gesetzt erscheinen lassen.
        ChefZ_RecordProbe.Reset();

        ChefZ_IngredientDef def = new ChefZ_IngredientDef();
        def.id = id;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.Compile(null);
        reg.Add(def);
        return def;
    }

    private static array<string> List1(string a)
    {
        array<string> l = new array<string>();
        l.Insert(a);
        return l;
    }

    private static array<string> List2(string a, string b)
    {
        array<string> l = new array<string>();
        l.Insert(a);
        l.Insert(b);
        return l;
    }

    private static ChefZ_IngredientProbe NewManager(ChefZ_CategoryManager cats)
    {
        ChefZ_IngredientProbe mgr = new ChefZ_IngredientProbe();
        mgr.SetCategoryManagerForTest(cats);
        return mgr;
    }

    //==========================================================================
    // 1. Stammdaten
    //==========================================================================

    private static bool BasicCheck()
    {
        ChefZ_CategoryManager cats = NewCategories();

        ChefZ_Registry<ChefZ_IngredientDef> defs = NewIngredients();

        ChefZ_IngredientDef full = NewIng(defs, "CHEFZ_ST_I_FULL");
        full.categories        = List1(C_KAT_A);
        full.tags              = List2(T_A, T_B);
        full.defaultState      = S_KNOWN;
        full.quantityUnit      = U_BULK;
        full.unitsPerWholeItem = 100.0;
        full.decays            = true;
        full.MarkExplicit("decays");
        full.containerCategory = "CHEFZ_ST_ICO_BOWL";
        full.returnContainer   = "AUTO";

        ChefZ_IngredientDef plain = NewIng(defs, "CHEFZ_ST_I_PLAIN");

        ChefZ_LoadReport report = NewReport();
        ChefZ_IngredientProbe mgr = NewManager(cats);
        mgr.Build(defs, report, NewStates());

        if (!mgr.IsReady())                                     return false;
        if (mgr.GetKnownCount() != 2)                           return false;
        if (mgr.GetRejectedCount() != 0)                        return false;
        if (report.ErrorCount() != 0)                           return false;
        if (report.WarnCount() != 0)                            return false;

        // --- vollstaendig deklarierte Klasse --------------------------------
        ChefZ_IngredientInfo a = mgr.ResolveByName("CHEFZ_ST_I_FULL");
        if (!a)                                                 return false;
        if (!a.isChefZManaged)                                  return false;
        if (a.classSym != Sym("CHEFZ_ST_I_FULL"))               return false;

        if (a.categories.Count() != 1)                          return false;
        if (a.categories.Get(0) != Sym(C_KAT_A))                 return false;

        // Closure ist self-or-ancestor: MEAT und FOOD, aber nicht WILD.
        if (!cats.IsInCategory(a.closure, Sym(C_KAT_A)))         return false;
        if (!cats.IsInCategory(a.closure, Sym(C_FOOD)))         return false;
        if (cats.IsInCategory(a.closure, Sym(C_WILD)))          return false;
        if (a.closure.CountBits() != 2)                         return false;

        if (a.staticTags.Count() != 2)                          return false;
        if (!a.HasTag(Sym(T_A)))                                return false;
        if (!a.HasTag(Sym(T_B)))                                return false;

        if (a.defaultState != Sym(S_KNOWN))                     return false;
        if (a.quantityUnit != Sym(U_BULK))                      return false;
        if (a.unitsPerWholeItem != 100.0)                       return false;
        if (!a.decays)                                          return false;
        if (a.containerCategory != Sym("CHEFZ_ST_ICO_BOWL"))    return false;
        if (a.returnContainer != Sym("AUTO"))                   return false;

        // --- Klasse ohne jede Angabe -----------------------------------------
        ChefZ_IngredientInfo b = mgr.ResolveByName("CHEFZ_ST_I_PLAIN");
        if (!b)                                                 return false;
        if (!b.isChefZManaged)                                  return false;
        if (b.categories.Count() != 0)                          return false;
        if (!b.closure.IsEmpty())                               return false;
        if (b.staticTags.Count() != 0)                          return false;
        if (b.defaultState != ChefZ_SymbolTable.INVALID)        return false;
        if (b.quantityUnit != ChefZ_IngredientManager.DefaultUnitSym())  return false;
        if (b.unitsPerWholeItem != 1.0)                         return false;
        if (b.decays)                                           return false;   // 01 V9

        // --- Nachschlagen -----------------------------------------------------
        if (!mgr.IsKnown(Sym("CHEFZ_ST_I_FULL")))               return false;
        if (mgr.IsKnown(ChefZ_SymbolTable.Intern("CHEFZ_ST_I_NEVER")))   return false;
        if (mgr.Resolve(Sym("CHEFZ_ST_I_NEVER")))               return false;
        if (mgr.ResolveByName("gibtsNicht"))                    return false;
        if (mgr.Resolve(ChefZ_SymbolTable.INVALID))             return false;

        // --- Auszug laeuft -----------------------------------------------------
        array<string> lines;
        mgr.DumpIngredients(lines);
        if (lines.Count() < 3)                                  return false;

        return true;
    }

    //==========================================================================
    // 2. Vererbung entlang der Elternkette (05 E2)
    //==========================================================================

    /**
     * Aufbau:
     *
     *   CHEFZ_ST_I_BASE          deklariert: MEAT, Tag A, U_BULK x100, decays
     *     +-- CHEFZ_ST_I_MID     NICHT deklariert - eine Zwischenklasse ohne
     *     |                      Zutatenbindung darf die Kette nicht kappen
     *           +-- CHEFZ_ST_I_KID   deklariert nur: WILD
     *
     *   CHEFZ_ST_I_OWN           erbt von BASE, sagt aber selbst alles
     *   CHEFZ_ST_I_RING          Elternkette zeigt im Kreis - darf nicht haengen
     */
    private static bool InheritanceCheck()
    {
        ChefZ_CategoryManager cats = NewCategories();
        ChefZ_Registry<ChefZ_IngredientDef> defs = NewIngredients();

        ChefZ_IngredientDef base_ = NewIng(defs, "CHEFZ_ST_I_BASE");
        base_.categories        = List1(C_KAT_A);
        base_.tags              = List1(T_A);
        base_.defaultState      = S_KNOWN;
        base_.quantityUnit      = U_BULK;
        base_.unitsPerWholeItem = 100.0;
        base_.decays            = true;
        base_.MarkExplicit("decays");

        ChefZ_IngredientDef kid = NewIng(defs, "CHEFZ_ST_I_KID");
        kid.categories = List1(C_WILD);

        ChefZ_IngredientDef own = NewIng(defs, "CHEFZ_ST_I_OWN");
        own.categories        = List1(C_KAT_A);
        own.tags              = List1(T_B);
        own.unitsPerWholeItem = 2.0;
        own.decays            = false;
        own.MarkExplicit("decays");

        ChefZ_IngredientDef ring = NewIng(defs, "CHEFZ_ST_I_RING");

        ChefZ_IngredientProbe mgr = NewManager(cats);
        mgr.SetParent("CHEFZ_ST_I_KID",  "CHEFZ_ST_I_MID");
        mgr.SetParent("CHEFZ_ST_I_MID",  "CHEFZ_ST_I_BASE");
        mgr.SetParent("CHEFZ_ST_I_OWN",  "CHEFZ_ST_I_BASE");
        mgr.SetParent("CHEFZ_ST_I_RING", "CHEFZ_ST_I_RING");

        ChefZ_LoadReport report = NewReport();
        mgr.Build(defs, report, NewStates());

        if (report.ErrorCount() != 0)                           return false;
        if (mgr.GetKnownCount() != 4)                           return false;

        // --- das Kind erbt ueber die undeklarierte Zwischenklasse hinweg ----
        ChefZ_IngredientInfo k = mgr.ResolveByName("CHEFZ_ST_I_KID");
        if (!k)                                                 return false;

        // Eigene Kategorienliste ERSETZT die geerbte (Ganzersatz).
        if (k.categories.Count() != 1)                          return false;
        if (k.categories.Get(0) != Sym(C_WILD))                 return false;
        if (!cats.IsInCategory(k.closure, Sym(C_WILD)))         return false;
        if (!cats.IsInCategory(k.closure, Sym(C_KAT_A)))         return false;   // ueber den Baum
        if (!cats.IsInCategory(k.closure, Sym(C_FOOD)))         return false;

        // Alles Uebrige stammt aus der Basis.
        if (k.staticTags.Count() != 1)                          return false;
        if (!k.HasTag(Sym(T_A)))                                return false;
        if (k.defaultState != Sym(S_KNOWN))                     return false;
        if (k.quantityUnit != Sym(U_BULK))                      return false;
        if (k.unitsPerWholeItem != 100.0)                       return false;
        if (!k.decays)                                          return false;

        // --- eigene Angaben schlagen die geerbten ----------------------------
        ChefZ_IngredientInfo o = mgr.ResolveByName("CHEFZ_ST_I_OWN");
        if (!o)                                                 return false;
        if (o.staticTags.Count() != 1)                          return false;
        if (!o.HasTag(Sym(T_B)))                                return false;
        if (o.HasTag(Sym(T_A)))                                 return false;
        if (o.unitsPerWholeItem != 2.0)                         return false;
        if (o.decays)                                           return false;
        // quantityUnit hat OWN nicht genannt -> geerbt.
        if (o.quantityUnit != Sym(U_BULK))                      return false;

        // --- die Basis selbst bleibt unveraendert ----------------------------
        ChefZ_IngredientInfo bs = mgr.ResolveByName("CHEFZ_ST_I_BASE");
        if (!bs)                                                return false;
        if (bs.categories.Get(0) != Sym(C_KAT_A))                return false;
        if (bs.unitsPerWholeItem != 100.0)                      return false;

        // --- eine im Kreis zeigende Elternkette haengt nicht ------------------
        ChefZ_IngredientInfo r = mgr.ResolveByName("CHEFZ_ST_I_RING");
        if (!r)                                                 return false;
        if (r.unitsPerWholeItem != 1.0)                         return false;

        return true;
    }

    //==========================================================================
    // 3. Rueckwaertsindizes und Kandidatenschaetzung (07 E4)
    //==========================================================================

    private static bool IndexCheck()
    {
        ChefZ_CategoryManager cats = NewCategories();
        ChefZ_Registry<ChefZ_IngredientDef> defs = NewIngredients();

        ChefZ_IngredientDef zutatA = NewIng(defs, "CHEFZ_ST_I_X_MEAT");
        zutatA.categories = List1(C_KAT_A);
        zutatA.tags       = List1(T_A);

        ChefZ_IngredientDef wild = NewIng(defs, "CHEFZ_ST_I_X_WILD");
        wild.categories = List1(C_WILD);
        wild.tags       = List2(T_A, T_B);

        ChefZ_IngredientDef bare = NewIng(defs, "CHEFZ_ST_I_X_BARE");

        ChefZ_IngredientProbe mgr = NewManager(cats);
        mgr.Build(defs, NewReport(), null);

        array<ChefZ_Sym> found;

        // Oberkategorie zaehlt die Kinder MIT - sonst waere die Schaetzung
        // systematisch zu klein und der Matcher sortierte falsch.
        mgr.GetClassesInCategory(Sym(C_FOOD), found);
        if (found.Count() != 2)                                 return false;

        mgr.GetClassesInCategory(Sym(C_KAT_A), found);
        if (found.Count() != 2)                                 return false;

        mgr.GetClassesInCategory(Sym(C_WILD), found);
        if (found.Count() != 1)                                 return false;
        if (found.Get(0) != Sym("CHEFZ_ST_I_X_WILD"))           return false;

        mgr.GetClassesInCategory(Sym(C_NOPE), found);
        if (found.Count() != 0)                                 return false;

        mgr.GetClassesWithTag(Sym(T_A), found);
        if (found.Count() != 2)                                 return false;
        mgr.GetClassesWithTag(Sym(T_B), found);
        if (found.Count() != 1)                                 return false;
        mgr.GetClassesWithTag(Sym(T_NOPE), found);
        if (found.Count() != 0)                                 return false;

        if (mgr.EstimateCandidateCount(Sym(C_FOOD)) != 2)       return false;
        if (mgr.EstimateCandidateCount(Sym(C_WILD)) != 1)       return false;
        if (mgr.EstimateCandidateCount(Sym(T_B)) != 1)          return false;
        if (mgr.EstimateCandidateCount(Sym(C_NOPE)) != 0)       return false;
        if (mgr.EstimateCandidateCount(ChefZ_SymbolTable.INVALID) != 0)  return false;

        // Die zurueckgegebene Liste ist eine Kopie: sie zu leeren darf den
        // Index nicht anfassen.
        mgr.GetClassesInCategory(Sym(C_FOOD), found);
        found.Clear();
        if (mgr.EstimateCandidateCount(Sym(C_FOOD)) != 2)       return false;

        // Eine Klasse ohne Kategorien und Tags taucht nirgends auf, ist aber
        // ueber ihre Klasse ansprechbar (05 E3).
        if (!mgr.IsKnown(Sym("CHEFZ_ST_I_X_BARE")))             return false;

        return true;
    }

    //==========================================================================
    // 4. Mengeneinheiten (05 §6, §7)
    //==========================================================================

    private static bool UnitCheck()
    {
        ChefZ_CategoryManager cats = NewCategories();
        ChefZ_Registry<ChefZ_IngredientDef> defs = NewIngredients();

        // Nenner Null bei eigener Einheit -> ABGEWIESEN (Division durch Null).
        ChefZ_IngredientDef bad = NewIng(defs, "CHEFZ_ST_I_U_BAD");
        bad.quantityUnit      = U_BULK;
        bad.unitsPerWholeItem = 0.0;

        // Nenner Null bei der Standardeinheit -> auf 1 geklemmt.
        ChefZ_IngredientDef clamped = NewIng(defs, "CHEFZ_ST_I_U_CLAMP");
        clamped.unitsPerWholeItem = 0.0;

        // Negativ, ebenfalls Standardeinheit -> geklemmt.
        ChefZ_IngredientDef negative = NewIng(defs, "CHEFZ_ST_I_U_NEG");
        negative.quantityUnit      = ChefZ_IngredientDef.DEFAULT_QUANTITY_UNIT;
        negative.unitsPerWholeItem = -3.0;

        ChefZ_LoadReport report = NewReport();
        ChefZ_IngredientProbe mgr = NewManager(cats);
        mgr.Build(defs, report, null);

        if (mgr.GetKnownCount() != 2)                           return false;
        if (mgr.GetRejectedCount() != 1)                        return false;
        if (report.ErrorCount() != 1)                           return false;
        if (report.WarnCount() != 2)                            return false;

        if (mgr.ResolveByName("CHEFZ_ST_I_U_BAD"))              return false;

        ChefZ_IngredientInfo c = mgr.ResolveByName("CHEFZ_ST_I_U_CLAMP");
        if (!c)                                                 return false;
        if (c.unitsPerWholeItem != 1.0)                         return false;
        if (c.quantityUnit != ChefZ_IngredientManager.DefaultUnitSym())   return false;

        ChefZ_IngredientInfo n = mgr.ResolveByName("CHEFZ_ST_I_U_NEG");
        if (!n)                                                 return false;
        if (n.unitsPerWholeItem != 1.0)                         return false;

        return true;
    }

    //==========================================================================
    // 5. Referenzen auf Unbekanntes (05 §7)
    //==========================================================================

    /**
     * Die Kernaussage: ein Datenfehler macht die Zutat ENGER matchbar, nie
     * falscher - und er reisst nie die uebrigen Angaben derselben Zutat mit.
     */
    private static bool ReferenceCheck()
    {
        ChefZ_CategoryManager cats = NewCategories();
        ChefZ_Registry<ChefZ_IngredientDef> defs = NewIngredients();

        ChefZ_IngredientDef def = NewIng(defs, "CHEFZ_ST_I_R_MIX");
        def.categories   = List2(C_NOPE, C_KAT_A);      // eine unbekannt
        def.tags         = List2(T_NOPE, T_B);         // einer unbekannt
        def.defaultState = S_NOPE;                     // unbekannt

        ChefZ_LoadReport report = NewReport();
        ChefZ_IngredientProbe mgr = NewManager(cats);
        mgr.Build(defs, report, NewStates());

        // Drei Warnungen, kein Fehler, Record bleibt geladen.
        if (report.ErrorCount() != 0)                           return false;
        if (report.WarnCount() != 3)                            return false;
        if (mgr.GetKnownCount() != 1)                           return false;

        ChefZ_IngredientInfo info = mgr.ResolveByName("CHEFZ_ST_I_R_MIX");
        if (!info)                                              return false;

        if (info.categories.Count() != 1)                       return false;
        if (info.categories.Get(0) != Sym(C_KAT_A))              return false;
        if (!cats.IsInCategory(info.closure, Sym(C_KAT_A)))      return false;
        if (cats.IsInCategory(info.closure, Sym(C_NOPE)))       return false;

        if (info.staticTags.Count() != 1)                       return false;
        if (!info.HasTag(Sym(T_B)))                             return false;

        // Ein unbekannter Zustand wird verworfen, nicht uebernommen.
        if (info.defaultState != ChefZ_SymbolTable.INVALID)     return false;

        // Ohne Zustandsregistry gibt es keine Zustandspruefung - dann bleibt
        // die Angabe stehen, und der Fehler faellt spaeter im Matching auf.
        ChefZ_LoadReport report2 = NewReport();
        ChefZ_IngredientProbe mgr2 = NewManager(cats);
        mgr2.Build(defs, report2, null);
        ChefZ_IngredientInfo info2 = mgr2.ResolveByName("CHEFZ_ST_I_R_MIX");
        if (!info2)                                             return false;
        if (info2.defaultState != Sym(S_NOPE))                  return false;
        if (report2.WarnCount() != 2)                           return false;

        return true;
    }

    //==========================================================================
    // 6. Leerer Bestand
    //==========================================================================

    //! 05 §7, erste Zeile: keine Zutatendaten ist KEIN Fehler. Das ist der
    //! Kernpunkt fuer Invariante I2 - ohne Bindungen matcht kein
    //! Kategorierezept, und das Kochen bleibt vollstaendig Vanilla.
    private static bool EmptyCheck()
    {
        ChefZ_CategoryManager cats = NewCategories();

        ChefZ_LoadReport report = NewReport();
        ChefZ_IngredientProbe mgr = NewManager(cats);
        mgr.Build(NewIngredients(), report, null);

        if (!mgr.IsReady())                                     return false;
        if (report.ErrorCount() != 0)                           return false;
        if (report.WarnCount() != 0)                            return false;
        if (mgr.GetKnownCount() != 0)                           return false;

        ChefZ_Sym any = ChefZ_SymbolTable.Intern("CHEFZ_ST_I_E_ANY");
        if (mgr.IsKnown(any))                                   return false;
        if (mgr.Resolve(any))                                   return false;
        if (mgr.EstimateCandidateCount(any) != 0)               return false;

        array<ChefZ_Sym> found;
        mgr.GetClassesInCategory(any, found);
        if (!found)                                             return false;
        if (found.Count() != 0)                                 return false;

        // Auch mit null-Registry: kein Nullzugriff, "bereit und leer".
        ChefZ_IngredientProbe mgr2 = NewManager(cats);
        mgr2.Build(null, null, null);
        if (!mgr2.IsReady())                                    return false;
        if (mgr2.GetKnownCount() != 0)                          return false;

        return true;
    }

    //==========================================================================
    // 7. Abfrage vor Build
    //==========================================================================

    private static bool NotReadyCheck()
    {
        ChefZ_IngredientProbe mgr = new ChefZ_IngredientProbe();
        if (mgr.IsReady())                                      return false;

        ChefZ_Sym any = ChefZ_SymbolTable.Intern("CHEFZ_ST_I_N_ANY");
        if (mgr.IsKnown(any))                                   return false;
        if (mgr.Resolve(any))                                   return false;
        if (mgr.ResolveByName("CHEFZ_ST_I_N_ANY"))              return false;
        if (mgr.EstimateCandidateCount(any) != 0)               return false;
        if (mgr.GetKnownCount() != 0)                           return false;

        array<ChefZ_Sym> found;
        mgr.GetClassesWithTag(any, found);
        if (!found)                                             return false;
        if (found.Count() != 0)                                 return false;

        // Nach Reset() ist der Manager wieder "nicht gebaut".
        ChefZ_CategoryManager cats = NewCategories();
        mgr.SetCategoryManagerForTest(cats);
        ChefZ_Registry<ChefZ_IngredientDef> defs = NewIngredients();
        NewIng(defs, "CHEFZ_ST_I_N_ONE");
        mgr.Build(defs, NewReport(), null);
        if (!mgr.IsReady())                                     return false;
        if (!mgr.IsKnown(Sym("CHEFZ_ST_I_N_ONE")))              return false;

        mgr.Reset();
        if (mgr.IsReady())                                      return false;
        if (mgr.IsKnown(Sym("CHEFZ_ST_I_N_ONE")))               return false;
        if (mgr.GetKnownCount() != 0)                           return false;

        return true;
    }
}
