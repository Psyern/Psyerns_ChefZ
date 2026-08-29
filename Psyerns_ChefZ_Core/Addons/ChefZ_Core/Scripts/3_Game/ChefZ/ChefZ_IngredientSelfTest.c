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
            s_Passed++; ChefZ_SelfTestTrace.Reset();
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.CONFIG, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, "Selbsttest " + name + " FEHLGESCHLAGEN. Die Zutatenaufloesung verhaelt sich nicht " + "wie entworfen - jede Faktenerhebung und damit jede Rezeptauswahl ist ab hier " + "unzuverlaessig." + ChefZ_SelfTestTrace.Take());
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

        if (!mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 280, "!mgr.IsReady()");
        if (mgr.GetKnownCount() != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 281, "mgr.GetKnownCount() != 2");
        if (mgr.GetRejectedCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 282, "mgr.GetRejectedCount() != 0");
        if (report.ErrorCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 283, "report.ErrorCount() != 0");
        if (report.WarnCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 284, "report.WarnCount() != 0");

        // --- vollstaendig deklarierte Klasse --------------------------------
        ChefZ_IngredientInfo a = mgr.ResolveByName("CHEFZ_ST_I_FULL");
        if (!a) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 288, "!a");
        if (!a.isChefZManaged) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 289, "!a.isChefZManaged");
        if (a.classSym != Sym("CHEFZ_ST_I_FULL")) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 290, "a.classSym != Sym('CHEFZ_ST_I_FULL')");

        if (a.categories.Count() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 292, "a.categories.Count() != 1");
        if (a.categories.Get(0) != Sym(C_KAT_A)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 293, "a.categories.Get(0) != Sym(C_KAT_A)");

        // Closure ist self-or-ancestor: MEAT und FOOD, aber nicht WILD.
        if (!cats.IsInCategory(a.closure, Sym(C_KAT_A))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 296, "!cats.IsInCategory(a.closure, Sym(C_KAT_A))");
        if (!cats.IsInCategory(a.closure, Sym(C_FOOD))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 297, "!cats.IsInCategory(a.closure, Sym(C_FOOD))");
        if (cats.IsInCategory(a.closure, Sym(C_WILD))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 298, "cats.IsInCategory(a.closure, Sym(C_WILD))");
        if (a.closure.CountBits() != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 299, "a.closure.CountBits() != 2");

        if (a.staticTags.Count() != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 301, "a.staticTags.Count() != 2");
        if (!a.HasTag(Sym(T_A))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 302, "!a.HasTag(Sym(T_A))");
        if (!a.HasTag(Sym(T_B))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 303, "!a.HasTag(Sym(T_B))");

        if (a.defaultState != Sym(S_KNOWN)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 305, "a.defaultState != Sym(S_KNOWN)");
        if (a.quantityUnit != Sym(U_BULK)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 306, "a.quantityUnit != Sym(U_BULK)");
        if (a.unitsPerWholeItem != 100.0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 307, "a.unitsPerWholeItem != 100.0");
        if (!a.decays) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 308, "!a.decays");
        if (a.containerCategory != Sym("CHEFZ_ST_ICO_BOWL")) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 309, "a.containerCategory != Sym('CHEFZ_ST_ICO_BOWL')");
        if (a.returnContainer != Sym("AUTO")) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 310, "a.returnContainer != Sym('AUTO')");

        // --- Klasse ohne jede Angabe -----------------------------------------
        ChefZ_IngredientInfo b = mgr.ResolveByName("CHEFZ_ST_I_PLAIN");
        if (!b) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 314, "!b");
        if (!b.isChefZManaged) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 315, "!b.isChefZManaged");
        if (b.categories.Count() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 316, "b.categories.Count() != 0");
        if (!b.closure.IsEmpty()) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 317, "!b.closure.IsEmpty()");
        if (b.staticTags.Count() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 318, "b.staticTags.Count() != 0");
        if (b.defaultState != ChefZ_SymbolTable.INVALID) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 319, "b.defaultState != ChefZ_SymbolTable.INVALID");
        if (b.quantityUnit != ChefZ_IngredientManager.DefaultUnitSym()) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 320, "b.quantityUnit != ChefZ_IngredientManager.DefaultUnitSym()");
        if (b.unitsPerWholeItem != 1.0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 321, "b.unitsPerWholeItem != 1.0");
        if (b.decays) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 322, "b.decays");   // 01 V9

        // --- Nachschlagen -----------------------------------------------------
        if (!mgr.IsKnown(Sym("CHEFZ_ST_I_FULL"))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 325, "!mgr.IsKnown(Sym('CHEFZ_ST_I_FULL'))");
        if (mgr.IsKnown(ChefZ_SymbolTable.Intern("CHEFZ_ST_I_NEVER"))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 326, "mgr.IsKnown(ChefZ_SymbolTable.Intern('CHEFZ_ST_I_NEVER'))");
        if (mgr.Resolve(Sym("CHEFZ_ST_I_NEVER"))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 327, "mgr.Resolve(Sym('CHEFZ_ST_I_NEVER'))");
        if (mgr.ResolveByName("gibtsNicht")) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 328, "mgr.ResolveByName('gibtsNicht')");
        if (mgr.Resolve(ChefZ_SymbolTable.INVALID)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 329, "mgr.Resolve(ChefZ_SymbolTable.INVALID)");

        // --- Auszug laeuft -----------------------------------------------------
        array<string> lines;
        mgr.DumpIngredients(lines);
        if (lines.Count() < 3) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 334, "lines.Count() < 3");

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

        if (report.ErrorCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 389, "report.ErrorCount() != 0");
        if (mgr.GetKnownCount() != 4) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 390, "mgr.GetKnownCount() != 4");

        // --- das Kind erbt ueber die undeklarierte Zwischenklasse hinweg ----
        ChefZ_IngredientInfo k = mgr.ResolveByName("CHEFZ_ST_I_KID");
        if (!k) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 394, "!k");

        // Eigene Kategorienliste ERSETZT die geerbte (Ganzersatz).
        if (k.categories.Count() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 397, "k.categories.Count() != 1");
        if (k.categories.Get(0) != Sym(C_WILD)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 398, "k.categories.Get(0) != Sym(C_WILD)");
        if (!cats.IsInCategory(k.closure, Sym(C_WILD))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 399, "!cats.IsInCategory(k.closure, Sym(C_WILD))");
        if (!cats.IsInCategory(k.closure, Sym(C_KAT_A))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 400, "!cats.IsInCategory(k.closure, Sym(C_KAT_A))");   // ueber den Baum
        if (!cats.IsInCategory(k.closure, Sym(C_FOOD))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 401, "!cats.IsInCategory(k.closure, Sym(C_FOOD))");

        // Alles Uebrige stammt aus der Basis.
        if (k.staticTags.Count() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 404, "k.staticTags.Count() != 1");
        if (!k.HasTag(Sym(T_A))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 405, "!k.HasTag(Sym(T_A))");
        if (k.defaultState != Sym(S_KNOWN)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 406, "k.defaultState != Sym(S_KNOWN)");
        if (k.quantityUnit != Sym(U_BULK)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 407, "k.quantityUnit != Sym(U_BULK)");
        if (k.unitsPerWholeItem != 100.0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 408, "k.unitsPerWholeItem != 100.0");
        if (!k.decays) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 409, "!k.decays");

        // --- eigene Angaben schlagen die geerbten ----------------------------
        ChefZ_IngredientInfo o = mgr.ResolveByName("CHEFZ_ST_I_OWN");
        if (!o) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 413, "!o");
        if (o.staticTags.Count() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 414, "o.staticTags.Count() != 1");
        if (!o.HasTag(Sym(T_B))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 415, "!o.HasTag(Sym(T_B))");
        if (o.HasTag(Sym(T_A))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 416, "o.HasTag(Sym(T_A))");
        if (o.unitsPerWholeItem != 2.0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 417, "o.unitsPerWholeItem != 2.0");
        if (o.decays) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 418, "o.decays");
        // quantityUnit hat OWN nicht genannt -> geerbt.
        if (o.quantityUnit != Sym(U_BULK)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 420, "o.quantityUnit != Sym(U_BULK)");

        // --- die Basis selbst bleibt unveraendert ----------------------------
        ChefZ_IngredientInfo bs = mgr.ResolveByName("CHEFZ_ST_I_BASE");
        if (!bs) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 424, "!bs");
        if (bs.categories.Get(0) != Sym(C_KAT_A)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 425, "bs.categories.Get(0) != Sym(C_KAT_A)");
        if (bs.unitsPerWholeItem != 100.0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 426, "bs.unitsPerWholeItem != 100.0");

        // --- eine im Kreis zeigende Elternkette haengt nicht ------------------
        ChefZ_IngredientInfo r = mgr.ResolveByName("CHEFZ_ST_I_RING");
        if (!r) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 430, "!r");
        if (r.unitsPerWholeItem != 1.0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 431, "r.unitsPerWholeItem != 1.0");

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
        if (found.Count() != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 463, "found.Count() != 2");

        mgr.GetClassesInCategory(Sym(C_KAT_A), found);
        if (found.Count() != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 466, "found.Count() != 2");

        mgr.GetClassesInCategory(Sym(C_WILD), found);
        if (found.Count() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 469, "found.Count() != 1");
        if (found.Get(0) != Sym("CHEFZ_ST_I_X_WILD")) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 470, "found.Get(0) != Sym('CHEFZ_ST_I_X_WILD')");

        mgr.GetClassesInCategory(Sym(C_NOPE), found);
        if (found.Count() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 473, "found.Count() != 0");

        mgr.GetClassesWithTag(Sym(T_A), found);
        if (found.Count() != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 476, "found.Count() != 2");
        mgr.GetClassesWithTag(Sym(T_B), found);
        if (found.Count() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 478, "found.Count() != 1");
        mgr.GetClassesWithTag(Sym(T_NOPE), found);
        if (found.Count() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 480, "found.Count() != 0");

        if (mgr.EstimateCandidateCount(Sym(C_FOOD)) != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 482, "mgr.EstimateCandidateCount(Sym(C_FOOD)) != 2");
        if (mgr.EstimateCandidateCount(Sym(C_WILD)) != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 483, "mgr.EstimateCandidateCount(Sym(C_WILD)) != 1");
        if (mgr.EstimateCandidateCount(Sym(T_B)) != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 484, "mgr.EstimateCandidateCount(Sym(T_B)) != 1");
        if (mgr.EstimateCandidateCount(Sym(C_NOPE)) != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 485, "mgr.EstimateCandidateCount(Sym(C_NOPE)) != 0");
        if (mgr.EstimateCandidateCount(ChefZ_SymbolTable.INVALID) != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 486, "mgr.EstimateCandidateCount(ChefZ_SymbolTable.INVALID) != 0");

        // Die zurueckgegebene Liste ist eine Kopie: sie zu leeren darf den
        // Index nicht anfassen.
        mgr.GetClassesInCategory(Sym(C_FOOD), found);
        found.Clear();
        if (mgr.EstimateCandidateCount(Sym(C_FOOD)) != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 492, "mgr.EstimateCandidateCount(Sym(C_FOOD)) != 2");

        // Eine Klasse ohne Kategorien und Tags taucht nirgends auf, ist aber
        // ueber ihre Klasse ansprechbar (05 E3).
        if (!mgr.IsKnown(Sym("CHEFZ_ST_I_X_BARE"))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 496, "!mgr.IsKnown(Sym('CHEFZ_ST_I_X_BARE'))");

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
        // Die Null muss als geschrieben gelten - aus einer Datei traegt sie
        // ChefZ_JsonExplicit ein, von Hand gebaut niemand.
        bad.MarkExplicit("unitsPerWholeItem");

        // Nenner Null bei der Standardeinheit -> auf 1 geklemmt.
        ChefZ_IngredientDef clamped = NewIng(defs, "CHEFZ_ST_I_U_CLAMP");
        clamped.unitsPerWholeItem = 0.0;
        clamped.MarkExplicit("unitsPerWholeItem");

        // Negativ, ebenfalls Standardeinheit -> geklemmt.
        ChefZ_IngredientDef negative = NewIng(defs, "CHEFZ_ST_I_U_NEG");
        negative.quantityUnit      = ChefZ_IngredientDef.DEFAULT_QUANTITY_UNIT;
        negative.unitsPerWholeItem = -3.0;

        ChefZ_LoadReport report = NewReport();
        ChefZ_IngredientProbe mgr = NewManager(cats);
        mgr.Build(defs, report, null);

        if (mgr.GetKnownCount() != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 532, "mgr.GetKnownCount() != 2");
        if (mgr.GetRejectedCount() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 533, "mgr.GetRejectedCount() != 1");
        if (report.ErrorCount() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 534, "report.ErrorCount() != 1");
        if (report.WarnCount() != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 535, "report.WarnCount() != 2");

        if (mgr.ResolveByName("CHEFZ_ST_I_U_BAD")) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 537, "mgr.ResolveByName('CHEFZ_ST_I_U_BAD')");

        ChefZ_IngredientInfo c = mgr.ResolveByName("CHEFZ_ST_I_U_CLAMP");
        if (!c) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 540, "!c");
        if (c.unitsPerWholeItem != 1.0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 541, "c.unitsPerWholeItem != 1.0");
        if (c.quantityUnit != ChefZ_IngredientManager.DefaultUnitSym()) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 542, "c.quantityUnit != ChefZ_IngredientManager.DefaultUnitSym()");

        ChefZ_IngredientInfo n = mgr.ResolveByName("CHEFZ_ST_I_U_NEG");
        if (!n) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 545, "!n");
        if (n.unitsPerWholeItem != 1.0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 546, "n.unitsPerWholeItem != 1.0");

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
        if (report.ErrorCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 574, "report.ErrorCount() != 0");
        if (report.WarnCount() != 3) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 575, "report.WarnCount() != 3");
        if (mgr.GetKnownCount() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 576, "mgr.GetKnownCount() != 1");

        ChefZ_IngredientInfo info = mgr.ResolveByName("CHEFZ_ST_I_R_MIX");
        if (!info) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 579, "!info");

        if (info.categories.Count() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 581, "info.categories.Count() != 1");
        if (info.categories.Get(0) != Sym(C_KAT_A)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 582, "info.categories.Get(0) != Sym(C_KAT_A)");
        if (!cats.IsInCategory(info.closure, Sym(C_KAT_A))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 583, "!cats.IsInCategory(info.closure, Sym(C_KAT_A))");
        if (cats.IsInCategory(info.closure, Sym(C_NOPE))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 584, "cats.IsInCategory(info.closure, Sym(C_NOPE))");

        if (info.staticTags.Count() != 1) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 586, "info.staticTags.Count() != 1");
        if (!info.HasTag(Sym(T_B))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 587, "!info.HasTag(Sym(T_B))");

        // Ein unbekannter Zustand wird verworfen, nicht uebernommen.
        if (info.defaultState != ChefZ_SymbolTable.INVALID) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 590, "info.defaultState != ChefZ_SymbolTable.INVALID");

        // Ohne Zustandsregistry gibt es keine Zustandspruefung - dann bleibt
        // die Angabe stehen, und der Fehler faellt spaeter im Matching auf.
        ChefZ_LoadReport report2 = NewReport();
        ChefZ_IngredientProbe mgr2 = NewManager(cats);
        mgr2.Build(defs, report2, null);
        ChefZ_IngredientInfo info2 = mgr2.ResolveByName("CHEFZ_ST_I_R_MIX");
        if (!info2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 598, "!info2");
        if (info2.defaultState != Sym(S_NOPE)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 599, "info2.defaultState != Sym(S_NOPE)");
        if (report2.WarnCount() != 2) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 600, "report2.WarnCount() != 2");

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

        if (!mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 620, "!mgr.IsReady()");
        if (report.ErrorCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 621, "report.ErrorCount() != 0");
        if (report.WarnCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 622, "report.WarnCount() != 0");
        if (mgr.GetKnownCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 623, "mgr.GetKnownCount() != 0");

        ChefZ_Sym any = ChefZ_SymbolTable.Intern("CHEFZ_ST_I_E_ANY");
        if (mgr.IsKnown(any)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 626, "mgr.IsKnown(any)");
        if (mgr.Resolve(any)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 627, "mgr.Resolve(any)");
        if (mgr.EstimateCandidateCount(any) != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 628, "mgr.EstimateCandidateCount(any) != 0");

        array<ChefZ_Sym> found;
        mgr.GetClassesInCategory(any, found);
        if (!found) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 632, "!found");
        if (found.Count() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 633, "found.Count() != 0");

        // Auch mit null-Registry: kein Nullzugriff, "bereit und leer".
        ChefZ_IngredientProbe mgr2 = NewManager(cats);
        mgr2.Build(null, null, null);
        if (!mgr2.IsReady()) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 638, "!mgr2.IsReady()");
        if (mgr2.GetKnownCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 639, "mgr2.GetKnownCount() != 0");

        return true;
    }

    //==========================================================================
    // 7. Abfrage vor Build
    //==========================================================================

    private static bool NotReadyCheck()
    {
        ChefZ_IngredientProbe mgr = new ChefZ_IngredientProbe();
        if (mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 651, "mgr.IsReady()");

        ChefZ_Sym any = ChefZ_SymbolTable.Intern("CHEFZ_ST_I_N_ANY");
        if (mgr.IsKnown(any)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 654, "mgr.IsKnown(any)");
        if (mgr.Resolve(any)) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 655, "mgr.Resolve(any)");
        if (mgr.ResolveByName("CHEFZ_ST_I_N_ANY")) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 656, "mgr.ResolveByName('CHEFZ_ST_I_N_ANY')");
        if (mgr.EstimateCandidateCount(any) != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 657, "mgr.EstimateCandidateCount(any) != 0");
        if (mgr.GetKnownCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 658, "mgr.GetKnownCount() != 0");

        array<ChefZ_Sym> found;
        mgr.GetClassesWithTag(any, found);
        if (!found) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 662, "!found");
        if (found.Count() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 663, "found.Count() != 0");

        // Nach Reset() ist der Manager wieder "nicht gebaut".
        ChefZ_CategoryManager cats = NewCategories();
        mgr.SetCategoryManagerForTest(cats);
        ChefZ_Registry<ChefZ_IngredientDef> defs = NewIngredients();
        NewIng(defs, "CHEFZ_ST_I_N_ONE");
        mgr.Build(defs, NewReport(), null);
        if (!mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 671, "!mgr.IsReady()");
        if (!mgr.IsKnown(Sym("CHEFZ_ST_I_N_ONE"))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 672, "!mgr.IsKnown(Sym('CHEFZ_ST_I_N_ONE'))");

        mgr.Reset();
        if (mgr.IsReady()) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 675, "mgr.IsReady()");
        if (mgr.IsKnown(Sym("CHEFZ_ST_I_N_ONE"))) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 676, "mgr.IsKnown(Sym('CHEFZ_ST_I_N_ONE'))");
        if (mgr.GetKnownCount() != 0) return ChefZ_SelfTestTrace.Fail("IngredientSelfTest", 677, "mgr.GetKnownCount() != 0");

        return true;
    }
}
