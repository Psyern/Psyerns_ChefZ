//==============================================================================
// ChefZ_NutritionSelfTest - Abnahmepruefung fuer S12, soweit sie ohne Welt geht
//
// Entwurf: 13 §4 (Datenmodell), 13 §5 (Sollrechnung und Audit), 13 §8
// (Fehlerverhalten, Zeile fuer Zeile), 13 E1/E2/E4.
//
// ---------------------------------------------------------------------------
// Was hier geprueft wird - und was nicht, und warum
// ---------------------------------------------------------------------------
// Pruefbar OHNE laufende Welt ist die ganze RECHNUNG: die Vektorarithmetik,
// die Auffindungsreihenfolge Klasse -> Kategorie -> Tag, die Wahl der
// tiefsten Kategorie, der Mengenfaktor aus dem Verbrauchsplan, der
// Modifikator, die NaN-Abwehr und die Klemmung.
//
// Und genau dieser Teil scheitert LEISE, wenn er scheitert - leiser als fast
// alles andere im Core:
//
//   - eine Sollrechnung, die eine Zutat doppelt zaehlt, liefert weiterhin
//     Zahlen und weiterhin Befunde. Der Balance-Reviewer bekommt sie und
//     glaubt ihnen;
//   - eine Auffindung, die den Klassenrecord uebergeht, meldet fuer jedes
//     Gericht dieselbe falsche Abweichung - was aussieht wie ein
//     systematischer Balancingfehler des Content-Moduls;
//   - ein NaN, das durchrutscht, steht als "-1.#IND" im Startlog und macht
//     den ganzen Audit unglaubwuerdig.
//
// NICHT pruefbar ist alles, was CfgVehicles braucht: der Vanilla-Rueckfall
// (13 E4), die V7-Pruefung auf "class Nutrition"/"class Food" und der
// Vergleich Soll gegen Ist. Ohne g_Game gibt es keine Config, und ein
// nachgebauter Configbaum pruefte den Nachbau statt der Engine. Diese drei
// bleiben dem Servertest vorbehalten - der Audit meldet sie beim ersten
// echten Serverstart im Klartext, und genau dafuer ist er gebaut.
//
// Der Test arbeitet auf EIGENEN Manager-Instanzen, nie auf den Singletons, und
// legt ausschliesslich Symbole mit dem Praefix "CHEFZ_NU_" an - Namen, die in
// echtem Content nicht vorkommen. Er beruehrt kein Item, keine Datei und keine
// Vanilla-Logik.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_NutritionSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("Vektor",         ChefZ_NutritionVector.SelfCheck());
        Check("Befundarten",    ChefZ_NutritionFindingKind.SelfCheck());
        Check("NutritionDef",   ChefZ_NutritionDef.SelfCheck());
        Check("Auffindung",     LookupCheck());
        Check("Kategorietiefe", CategoryDepthCheck());
        Check("Sollrechnung",   ExpectedCheck());
        Check("Modifikator",    ModifierCheck());
        Check("Mengenfaktor",   FactorCheck());
        Check("Klemmung",       ClampCheck());
        Check("Abweisung",      RejectCheck());
        Check("LeereRegistry",  EmptyCheck());
        Check("VorBuild",       NotReadyCheck());
        Check("KeinSchreiben",  NoWriteCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.NUTRI, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.NUTRI, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.NUTRI, "Selbsttest " + name + " FEHLGESCHLAGEN. Die Sollrechnung des Naehrwertaudits " + "verhaelt sich nicht wie entworfen - jede Zahl im Startaudit ist ab hier " + "unzuverlaessig. Kochen, Essen und Vanilla sind davon unberuehrt: der " + "Nutrition Manager schreibt zur Laufzeit nichts (13 E1).");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S12: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
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

    private static ChefZ_Sym Sym(string name)
    {
        return ChefZ_SymbolTable.Intern(name);
    }

    private static float Abs(float v)
    {
        if (v < 0.0)
            return -v;
        return v;
    }

    private static bool Near(float a, float b)
    {
        return Abs(a - b) < 0.0001;
    }

    private static ChefZ_NutritionManager NewManager()
    {
        ChefZ_NutritionManager mgr = new ChefZ_NutritionManager();
        mgr.SetQuietForTest(true);
        return mgr;
    }

    /**
     * Eine EIGENE, ungebaute Rezept-Engine.
     *
     * Sie existiert nur, damit der Test nicht auf den Singleton greifen muss:
     * ChefZ_RecipeEngine.Get() wuerde ihn beim Selbsttest anlegen, also noch
     * VOR LoadAll(), und der Test haette damit den echten Bestand des Servers
     * angefasst - wenn auch harmlos. Der Grundsatz "nie auf den Singletons"
     * ist mehr wert als die drei gesparten Zeilen.
     *
     * Ungebaut ist Absicht: der Audit soll an einer nicht bereiten Engine
     * ruhig zurueckkehren, und genau das prueft EmptyCheck.
     */
    private static ChefZ_RecipeEngine NewEngine()
    {
        return new ChefZ_RecipeEngine();
    }

    // Die Fixtures muessen die Pruefung ueberleben.
    //
    // Der Manager speichert seine Defs bewusst OHNE ref: Eigentuemer ist in
    // Produktion die Registry im Config Manager, und die lebt laenger als der
    // Manager. Eine Registry, die nur als lokale Variable dieser
    // Hilfsfunktion entsteht, ist dagegen schon abgeraeumt, bevor der Test
    // den Manager ueberhaupt befragt - der haelt dann ins Leere. Genau daran
    // ist der Testserver am 28.08.2026 gescheitert: "NULL pointer to
    // instance". Diese Liste bildet den Eigentuemer nach, den es in
    // Produktion gibt.
    private static ref array<ref ChefZ_Registry<ChefZ_NutritionDef>> s_AliveRegistries;

    private static ChefZ_Registry<ChefZ_NutritionDef> NewRegistry()
    {
        if (!s_AliveRegistries)
            s_AliveRegistries = new array<ref ChefZ_Registry<ChefZ_NutritionDef>>();

        ChefZ_Registry<ChefZ_NutritionDef> reg = new ChefZ_Registry<ChefZ_NutritionDef>();
        reg.Init(ChefZ_RecordKind.NUTRITION);
        s_AliveRegistries.Insert(reg);
        return reg;
    }

    private static ChefZ_NutritionDef AddDef(notnull ChefZ_Registry<ChefZ_NutritionDef> reg, string id, string scope, float energy, bool perUnit = false)
    {
        // Die Bool-Sonde steht sonst womoeglich noch auf "hoch" und wuerde
        // "perUnit" als gesetzt erscheinen lassen (02 E3).
        ChefZ_RecordProbe.Reset();

        ChefZ_NutritionDef def = new ChefZ_NutritionDef();
        def.id      = id;
        def.scope   = scope;
        def.energy  = energy;
        def.perUnit = perUnit;
        def.MarkExplicit("perUnit");
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.ResolveDefaults();
        def.Compile(null);
        reg.Add(def);
        return def;
    }

    //--------------------------------------------------------------------------

    /**
     * Ein Kategoriebaum fuer den Test. Zwei Ebenen, weil die Regel "die
     * TIEFSTE passende Kategorie gewinnt" sonst nicht pruefbar waere.
     */
    private static ChefZ_CategoryManager NewCategories()
    {
        ChefZ_Registry<ChefZ_CategoryDef> cats = new ChefZ_Registry<ChefZ_CategoryDef>();
        cats.Init(ChefZ_RecordKind.CATEGORY);

        ChefZ_CategoryDef root = new ChefZ_CategoryDef();
        root.id = "CHEFZ_NU_FOOD";
        root.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        root.Compile(null);
        cats.Add(root);

        ChefZ_CategoryDef leaf = new ChefZ_CategoryDef();
        leaf.id     = "CHEFZ_NU_MEAT";
        leaf.parent = "CHEFZ_NU_FOOD";
        leaf.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        leaf.Compile(null);
        cats.Add(leaf);

        ChefZ_Registry<ChefZ_TagDef> tags = new ChefZ_Registry<ChefZ_TagDef>();
        tags.Init(ChefZ_RecordKind.TAG);

        ChefZ_TagDef tag = new ChefZ_TagDef();
        tag.id = "CHEFZ_NU_SALTY";
        tag.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        tag.Compile(null);
        tags.Add(tag);

        ChefZ_CategoryManager mgr = new ChefZ_CategoryManager();
        mgr.SetQuietForTest(true);
        mgr.SetMaxCategoriesForTest(64);
        mgr.Build(cats, tags, null);
        return mgr;
    }

    /**
     * Ein Zutatenmanager mit genau einer Klasse: in beiden Kategorien und mit
     * dem Testtag.
     */
    private static ChefZ_IngredientManager NewIngredients(notnull ChefZ_CategoryManager cats)
    {
        ChefZ_Registry<ChefZ_IngredientDef> reg = new ChefZ_Registry<ChefZ_IngredientDef>();
        reg.Init(ChefZ_RecordKind.INGREDIENT);

        ChefZ_RecordProbe.Reset();

        ChefZ_IngredientDef def = new ChefZ_IngredientDef();
        def.id         = "CHEFZ_NU_STEAK";
        def.categories = new array<string>();
        def.categories.Insert("CHEFZ_NU_MEAT");
        def.tags       = new array<string>();
        def.tags.Insert("CHEFZ_NU_SALTY");
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.Compile(null);
        reg.Add(def);

        ChefZ_IngredientManager mgr = new ChefZ_IngredientManager();
        mgr.SetQuietForTest(true);
        mgr.SetCategoryManagerForTest(cats);
        mgr.SetSkipClassExistsCheckForTest(true);
        mgr.Build(reg, null, null);
        return mgr;
    }

    //==========================================================================
    // 13 E4 - Klasse schlaegt Kategorie schlaegt Tag
    //==========================================================================

    private static bool LookupCheck()
    {
        ChefZ_CategoryManager   cats = NewCategories();
        ChefZ_IngredientManager ing  = NewIngredients(cats);

        ChefZ_Registry<ChefZ_NutritionDef> reg = NewRegistry();
        AddDef(reg, "CHEFZ_NU_SALTY", ChefZ_NutritionScope.NAME_TAG,      10.0);
        AddDef(reg, "CHEFZ_NU_MEAT",  ChefZ_NutritionScope.NAME_CATEGORY, 20.0);

        ChefZ_NutritionManager mgr = NewManager();
        mgr.SetManagersForTest(cats, ing, NewEngine());
        mgr.Build(reg, null, null);
        if (!mgr.IsReady())                                     return false;
        if (mgr.GetRecordCount() != 2)                          return false;

        ChefZ_Sym klasseB = Sym("CHEFZ_NU_STEAK");

        // Ohne Klassenrecord gewinnt die Kategorie.
        ChefZ_NutritionDef found = mgr.FindDefForClass(klasseB);
        if (!found)                                             return false;
        if (found.id != "CHEFZ_NU_MEAT")                        return false;

        // Mit Klassenrecord gewinnt die Klasse.
        ChefZ_Registry<ChefZ_NutritionDef> reg2 = NewRegistry();
        AddDef(reg2, "CHEFZ_NU_SALTY", ChefZ_NutritionScope.NAME_TAG,      10.0);
        AddDef(reg2, "CHEFZ_NU_MEAT",  ChefZ_NutritionScope.NAME_CATEGORY, 20.0);
        AddDef(reg2, "CHEFZ_NU_STEAK", ChefZ_NutritionScope.NAME_CLASS,    30.0);

        ChefZ_NutritionManager mgr2 = NewManager();
        mgr2.SetManagersForTest(cats, ing, NewEngine());
        mgr2.Build(reg2, null, null);

        found = mgr2.FindDefForClass(klasseB);
        if (!found)                                             return false;
        if (found.id != "CHEFZ_NU_STEAK")                       return false;

        // Nur der Tag: er greift, wenn weder Klasse noch Kategorie etwas sagt.
        ChefZ_Registry<ChefZ_NutritionDef> reg3 = NewRegistry();
        AddDef(reg3, "CHEFZ_NU_SALTY", ChefZ_NutritionScope.NAME_TAG, 10.0);

        ChefZ_NutritionManager mgr3 = NewManager();
        mgr3.SetManagersForTest(cats, ing, NewEngine());
        mgr3.Build(reg3, null, null);

        found = mgr3.FindDefForClass(klasseB);
        if (!found)                                             return false;
        if (found.id != "CHEFZ_NU_SALTY")                       return false;

        // Eine voellig unbekannte Klasse findet nichts - und stuerzt nicht ab.
        if (mgr3.FindDefForClass(Sym("CHEFZ_NU_NICHTS")))       return false;
        if (mgr3.FindDefForClass(ChefZ_SymbolTable.INVALID))    return false;

        return true;
    }

    /**
     * Die TIEFSTE passende Kategorie gewinnt - und die Werte werden NICHT
     * addiert.
     *
     * Das ist die eine Stelle, an der der Nutrition Manager bewusst anders
     * rechnet als der Preservation Manager: dort multiplizieren sich Faktoren
     * sinnvoll, hier addierten sich absolute Naehrwerte zu einer Zutat, die
     * doppelt so nahrhaft ist, weil jemand sie doppelt eingeordnet hat.
     */
    private static bool CategoryDepthCheck()
    {
        ChefZ_CategoryManager   cats = NewCategories();
        ChefZ_IngredientManager ing  = NewIngredients(cats);

        ChefZ_Registry<ChefZ_NutritionDef> reg = NewRegistry();
        AddDef(reg, "CHEFZ_NU_FOOD", ChefZ_NutritionScope.NAME_CATEGORY, 100.0);
        AddDef(reg, "CHEFZ_NU_MEAT", ChefZ_NutritionScope.NAME_CATEGORY, 400.0);

        ChefZ_NutritionManager mgr = NewManager();
        mgr.SetManagersForTest(cats, ing, NewEngine());
        mgr.Build(reg, null, null);

        ChefZ_NutritionVector vec;
        if (!mgr.ReadBase(Sym("CHEFZ_NU_STEAK"), ChefZ_VanillaStage.RAW, vec))
            return false;

        // 400 (die tiefere), nicht 500 (die Summe) und nicht 100 (die Wurzel).
        if (!Near(vec.energy, 400.0))                           return false;

        return true;
    }

    //==========================================================================
    // 13 §5 - die Sollrechnung
    //==========================================================================

    /**
     * Der Rechenweg aus 13 §5, nachgerechnet:
     *
     *     450 + 500 + 100 = 1050
     *     1050 x 1.10     = 1155
     *
     * Wenn diese Zahl stimmt, stimmt jeder Summand - drei verschiedene Werte,
     * ein Modifikator, ein Ergebnis.
     */
    private static bool ExpectedCheck()
    {
        ChefZ_CategoryManager   cats = NewCategories();
        ChefZ_IngredientManager ing  = NewIngredients(cats);

        ChefZ_Registry<ChefZ_NutritionDef> reg = NewRegistry();
        AddDef(reg, "CHEFZ_NU_A", ChefZ_NutritionScope.NAME_CLASS, 450.0);
        AddDef(reg, "CHEFZ_NU_B", ChefZ_NutritionScope.NAME_CLASS, 500.0);
        AddDef(reg, "CHEFZ_NU_C", ChefZ_NutritionScope.NAME_CLASS, 100.0);

        ChefZ_NutritionManager mgr = NewManager();
        mgr.SetManagersForTest(cats, ing, NewEngine());
        mgr.Build(reg, null, null);

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        AddWholeItem(snap, 0, "CHEFZ_NU_A");
        AddWholeItem(snap, 1, "CHEFZ_NU_B");
        AddWholeItem(snap, 2, "CHEFZ_NU_C");

        ChefZ_MatchResult match = new ChefZ_MatchResult();
        AddConsumeWhole(match, 0);
        AddConsumeWhole(match, 1);
        AddConsumeWhole(match, 2);

        ChefZ_CompiledRecipe recipe = new ChefZ_CompiledRecipe();
        recipe.id                = "CHEFZ_NU_REZEPT";
        recipe.nutritionModifier = 1.10;

        ChefZ_NutritionVector expected;
        array<string> trace = new array<string>();
        mgr.ComputeExpected(recipe, match, snap, expected, trace);

        if (!expected)                                          return false;
        if (!Near(expected.energy, 1155.0))                     return false;
        if (trace.Count() == 0)                                 return false;

        // Eine Zutat OHNE Naehrwertdaten zaehlt mit 0 und sprengt nichts
        // (13 §8).
        AddWholeItem(snap, 3, "CHEFZ_NU_UNBEKANNT");
        AddConsumeWhole(match, 3);
        mgr.ComputeExpected(recipe, match, snap, expected, trace);
        if (!Near(expected.energy, 1155.0))                     return false;

        // Und ohne Trace darf es genauso laufen.
        array<string> noTrace;
        mgr.ComputeExpected(recipe, match, snap, expected, noTrace);
        if (!Near(expected.energy, 1155.0))                     return false;

        return true;
    }

    /**
     * 13 §8: "nutritionModifier <= 0 -> Auf 1.0 gesetzt, WARN mit Rezept-ID."
     */
    private static bool ModifierCheck()
    {
        ChefZ_CategoryManager cats = NewCategories();

        ChefZ_NutritionManager mgr = NewManager();
        mgr.SetManagersForTest(cats, NewIngredients(cats), NewEngine());

        ChefZ_Registry<ChefZ_NutritionDef> reg = NewRegistry();
        AddDef(reg, "CHEFZ_NU_A", ChefZ_NutritionScope.NAME_CLASS, 200.0);
        mgr.Build(reg, null, null);

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        AddWholeItem(snap, 0, "CHEFZ_NU_A");

        ChefZ_MatchResult match = new ChefZ_MatchResult();
        AddConsumeWhole(match, 0);

        ChefZ_CompiledRecipe recipe = new ChefZ_CompiledRecipe();
        recipe.id = "CHEFZ_NU_NULLMOD";

        ChefZ_NutritionVector expected;
        array<string> trace = new array<string>();

        recipe.nutritionModifier = 0.0;
        mgr.ComputeExpected(recipe, match, snap, expected, trace);
        if (!Near(expected.energy, 200.0))                      return false;

        recipe.nutritionModifier = -3.0;
        mgr.ComputeExpected(recipe, match, snap, expected, trace);
        if (!Near(expected.energy, 200.0))                      return false;

        recipe.nutritionModifier = 2.0;
        mgr.ComputeExpected(recipe, match, snap, expected, trace);
        if (!Near(expected.energy, 400.0))                      return false;

        return true;
    }

    /**
     * Der Mengenfaktor: ganzes Stueck, Teilverbrauch, und perUnit.
     */
    private static bool FactorCheck()
    {
        ChefZ_CategoryManager cats = NewCategories();

        ChefZ_Registry<ChefZ_NutritionDef> reg = NewRegistry();
        AddDef(reg, "CHEFZ_NU_WHOLE", ChefZ_NutritionScope.NAME_CLASS, 100.0);
        AddDef(reg, "CHEFZ_NU_UNITS", ChefZ_NutritionScope.NAME_CLASS,  10.0, true);

        ChefZ_NutritionManager mgr = NewManager();
        mgr.SetManagersForTest(cats, NewIngredients(cats), NewEngine());
        mgr.Build(reg, null, null);

        ChefZ_CompiledRecipe recipe = new ChefZ_CompiledRecipe();
        recipe.id = "CHEFZ_NU_FAKTOR";

        ChefZ_NutritionVector expected;
        array<string> trace = new array<string>();

        // Halb verbrauchtes Item: quantityDelta 50 von quantityMax 100.
        ChefZ_FactSnapshot snapA = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts fa = snapA.Acquire();
        fa.handle      = 0;
        fa.classSym    = Sym("CHEFZ_NU_WHOLE");
        fa.quantity    = 100.0;
        fa.quantityMax = 100.0;

        ChefZ_MatchResult matchA = new ChefZ_MatchResult();
        ChefZ_ConsumePlan planA = new ChefZ_ConsumePlan();
        planA.handle        = 0;
        planA.quantityDelta = 50.0;
        matchA.consumePlan.Insert(planA);

        mgr.ComputeExpected(recipe, matchA, snapA, expected, trace);
        if (!Near(expected.energy, 50.0))                       return false;

        // perUnit: drei Einheiten a 10.
        ChefZ_FactSnapshot snapB = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts fb = snapB.Acquire();
        fb.handle   = 0;
        fb.classSym = Sym("CHEFZ_NU_UNITS");
        fb.units    = 8.0;

        ChefZ_MatchResult matchB = new ChefZ_MatchResult();
        ChefZ_ConsumePlan planB = new ChefZ_ConsumePlan();
        planB.handle     = 0;
        planB.unitsDelta = 3.0;
        matchB.consumePlan.Insert(planB);

        mgr.ComputeExpected(recipe, matchB, snapB, expected, trace);
        if (!Near(expected.energy, 30.0))                       return false;

        // Ganzes Stueck einer perUnit-Klasse: alle acht Einheiten.
        ChefZ_MatchResult matchC = new ChefZ_MatchResult();
        ChefZ_ConsumePlan planC = new ChefZ_ConsumePlan();
        planC.handle       = 0;
        planC.destroyWhole = true;
        matchC.consumePlan.Insert(planC);

        mgr.ComputeExpected(recipe, matchC, snapB, expected, trace);
        if (!Near(expected.energy, 80.0))                       return false;

        // Ein Plan ohne jede Wirkung traegt nichts bei.
        ChefZ_MatchResult matchD = new ChefZ_MatchResult();
        ChefZ_ConsumePlan planD = new ChefZ_ConsumePlan();
        planD.handle = 0;
        matchD.consumePlan.Insert(planD);

        mgr.ComputeExpected(recipe, matchD, snapA, expected, trace);
        if (!Near(expected.energy, 0.0))                        return false;

        return true;
    }

    /**
     * 13 §8: Klemmung an der Sondengrenze, und die NaN-Abwehr.
     *
     * Die zweite Haelfte ist die wichtigere: eine Sollrechnung, die entgleist,
     * darf KEINE Zahl liefern. Eine geklemmte Fantasiezahl saehe aus wie ein
     * Balancingergebnis.
     */
    private static bool ClampCheck()
    {
        ChefZ_NutritionVector caps   = new ChefZ_NutritionVector();
        ChefZ_NutritionVector floors = new ChefZ_NutritionVector();
        caps.energy = 100.0; caps.water = 100.0; caps.fullness = 100.0;
        caps.nutritionalIndex = 100.0; caps.toxicity = 100.0; caps.digestibility = 100.0;
        floors.CopyFrom(caps);
        floors.Scale(-1.0);

        ChefZ_NutritionVector v = new ChefZ_NutritionVector();
        v.energy = 5000.0;
        v.water  = -5000.0;
        if (!v.ClampTo(caps, floors))                           return false;
        if (!Near(v.energy, 100.0))                             return false;
        if (!Near(v.water, -100.0))                             return false;

        // Der Manager klemmt an seiner eigenen Grenze und meldet es NICHT als
        // Fehler - 13 E6: es gibt in V1 kein Deckelsystem, nur eine Sonde.
        ChefZ_Registry<ChefZ_NutritionDef> reg = NewRegistry();
        AddDef(reg, "CHEFZ_NU_HUGE", ChefZ_NutritionScope.NAME_CLASS, 1.0e9);

        ChefZ_CategoryManager cats = NewCategories();

        ChefZ_NutritionManager mgr = NewManager();
        mgr.SetManagersForTest(cats, NewIngredients(cats), NewEngine());
        mgr.Build(reg, null, null);

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        AddWholeItem(snap, 0, "CHEFZ_NU_HUGE");

        ChefZ_MatchResult match = new ChefZ_MatchResult();
        AddConsumeWhole(match, 0);

        ChefZ_CompiledRecipe recipe = new ChefZ_CompiledRecipe();
        recipe.id = "CHEFZ_NU_GROSS";

        ChefZ_NutritionVector expected;
        array<string> trace = new array<string>();
        mgr.ComputeExpected(recipe, match, snap, expected, trace);

        // Vorgabe der Sondengrenze ist 100000 - geklemmt, aber endlich.
        if (!expected.IsFinite())                               return false;
        if (expected.energy > 100000.1)                         return false;

        return true;
    }

    //==========================================================================
    // Fehlerverhalten
    //==========================================================================

    private static bool RejectCheck()
    {
        ChefZ_CategoryManager cats = NewCategories();

        ChefZ_Registry<ChefZ_NutritionDef> reg = NewRegistry();

        // Unbekannte Kategorie -> abgewiesen.
        AddDef(reg, "CHEFZ_NU_GIBTESNICHT", ChefZ_NutritionScope.NAME_CATEGORY, 5.0);
        // Unbekannter Tag -> abgewiesen.
        AddDef(reg, "CHEFZ_NU_KEINTAG", ChefZ_NutritionScope.NAME_TAG, 5.0);
        // Gueltig.
        AddDef(reg, "CHEFZ_NU_MEAT", ChefZ_NutritionScope.NAME_CATEGORY, 5.0);

        ChefZ_NutritionManager mgr = NewManager();
        mgr.SetManagersForTest(cats, NewIngredients(cats), NewEngine());
        mgr.Build(reg, null, null);

        if (!mgr.IsReady())                                     return false;
        if (mgr.GetRejectedCount() != 2)                        return false;
        if (mgr.GetRecordCount() != 1)                          return false;

        return true;
    }

    /**
     * 13 §8, erste Zeile: ohne Records laeuft alles weiter. Der Manager ist
     * "bereit und leer", und ReadBase antwortet mit einem ruhigen false statt
     * mit einem Fehler.
     */
    private static bool EmptyCheck()
    {
        ChefZ_CategoryManager cats = NewCategories();

        ChefZ_NutritionManager mgr = NewManager();
        mgr.SetManagersForTest(cats, NewIngredients(cats), NewEngine());
        mgr.Build(null, null, null);

        if (!mgr.IsReady())                                     return false;
        if (mgr.GetRecordCount() != 0)                          return false;

        // Ohne g_Game gibt es auch keinen Vanilla-Rueckfall - das ist der
        // Selbsttestfall, nicht der Serverfall.
        ChefZ_NutritionVector vec;
        mgr.ReadBase(Sym("CHEFZ_NU_IRGENDWAS"), ChefZ_VanillaStage.RAW, vec);
        if (!vec)                                               return false;
        if (!vec.IsZero())                                      return false;

        // Und der Audit ohne Engine bricht nicht ab, sondern meldet und geht.
        array<ChefZ_NutritionFinding> findings;
        mgr.AuditAllRecipes(findings);
        if (!findings)                                          return false;
        if (findings.Count() != 0)                              return false;

        return true;
    }

    private static bool NotReadyCheck()
    {
        ChefZ_NutritionManager mgr = NewManager();
        ChefZ_CategoryManager cats = NewCategories();
        mgr.SetManagersForTest(cats, NewIngredients(cats), NewEngine());
        if (mgr.IsReady())                                      return false;

        // Vor dem Build darf nichts krachen.
        ChefZ_NutritionVector vec;
        mgr.ReadBase(Sym("CHEFZ_NU_VORBUILD"), ChefZ_VanillaStage.RAW, vec);
        if (!vec)                                               return false;

        string text;
        mgr.DescribeForUI(vec, text);
        if (text == "")                                         return false;

        return true;
    }

    /**
     * Die Kernzusage des Teilsystems auf der Rechenseite (13 E1/E2): der
     * Manager veraendert nichts, was er bekommt.
     *
     * Geprueft wird das an dem einzigen Weg, auf dem er es koennte - dem
     * Verbrauchsplan und der Faktenliste. Beide muessen nach der Sollrechnung
     * bitgenau so aussehen wie davor.
     */
    private static bool NoWriteCheck()
    {
        ChefZ_Registry<ChefZ_NutritionDef> reg = NewRegistry();
        AddDef(reg, "CHEFZ_NU_A", ChefZ_NutritionScope.NAME_CLASS, 250.0);

        ChefZ_CategoryManager cats = NewCategories();

        ChefZ_NutritionManager mgr = NewManager();
        mgr.SetManagersForTest(cats, NewIngredients(cats), NewEngine());
        mgr.Build(reg, null, null);

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts f = snap.Acquire();
        f.handle      = 0;
        f.classSym    = Sym("CHEFZ_NU_A");
        f.quantity    = 42.0;
        f.quantityMax = 84.0;
        f.units       =  7.0;

        ChefZ_MatchResult match = new ChefZ_MatchResult();
        ChefZ_ConsumePlan plan = new ChefZ_ConsumePlan();
        plan.handle       = 0;
        plan.destroyWhole = true;
        match.consumePlan.Insert(plan);

        ChefZ_CompiledRecipe recipe = new ChefZ_CompiledRecipe();
        recipe.id                = "CHEFZ_NU_UNBERUEHRT";
        recipe.nutritionModifier = 1.5;

        ChefZ_NutritionVector expected;
        array<string> trace = new array<string>();
        mgr.ComputeExpected(recipe, match, snap, expected, trace);

        if (!Near(expected.energy, 375.0))                      return false;

        // Nichts an den Eingaben darf sich bewegt haben.
        if (!Near(f.quantity, 42.0))                            return false;
        if (!Near(f.quantityMax, 84.0))                         return false;
        if (!Near(f.units, 7.0))                                return false;
        if (!plan.destroyWhole)                                 return false;
        if (!Near(plan.quantityDelta, 0.0))                     return false;
        if (!Near(recipe.nutritionModifier, 1.5))               return false;

        // Und der Record selbst auch nicht.
        ChefZ_NutritionDef def = mgr.FindDefForClass(Sym("CHEFZ_NU_A"));
        if (!def)                                               return false;
        if (!Near(def.energy, 250.0))                           return false;

        return true;
    }

    //==========================================================================
    // Bausteine
    //==========================================================================

    private static void AddWholeItem(notnull ChefZ_FactSnapshot snap, int handle, string cls)
    {
        ChefZ_ItemFacts f = snap.Acquire();
        f.handle      = handle;
        f.classSym    = Sym(cls);
        f.quantity    = 1.0;
        f.quantityMax = 1.0;
        f.units       = 1.0;
    }

    private static void AddConsumeWhole(notnull ChefZ_MatchResult match, int handle)
    {
        ChefZ_ConsumePlan plan = new ChefZ_ConsumePlan();
        plan.handle       = handle;
        plan.destroyWhole = true;
        match.consumePlan.Insert(plan);
    }
}
