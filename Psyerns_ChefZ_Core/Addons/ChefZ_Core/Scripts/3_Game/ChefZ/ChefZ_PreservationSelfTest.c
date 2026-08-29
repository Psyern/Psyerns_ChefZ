//==============================================================================
// ChefZ_PreservationSelfTest - Abnahmepruefung fuer S11, soweit sie ohne Welt
//                              geht
//
// Entwurf: 14 §3 (die Produktkette), 14 §4 (Restfrische), 14 §8
// (Fehlerverhalten, Zeile fuer Zeile), 14 E1/E2/E3/E7.
//
// ---------------------------------------------------------------------------
// Was hier geprueft wird - und was nicht, und warum
// ---------------------------------------------------------------------------
// Pruefbar OHNE laufende Welt ist alles, was der ChefZ_PreservationManager
// rechnet: die Produktkette, die Klemmung, die Kategorie-Vorfahrenregel, die
// Temperaturbindung, beide Schalter, die Frischefortschreibung und die
// Vererbung. Genau dieser Teil scheitert LEISE, wenn er scheitert:
//
//   - ein Multiplikator, der nicht ankommt, sieht aus wie Balancing, das noch
//     nicht wirkt;
//   - eine Frische, die als Mittelwert statt als Minimum vererbt wird, liefert
//     weiterhin Zahlen und weiterhin Gerichte - auffallen wuerde sie erst dem
//     Spieler, der altes Fleisch in frischem waescht (12 §4.1);
//   - ein Faktor, der versehentlich auch ohne Regeln von 1.0 abweicht, aendert
//     die Haltbarkeit JEDER ChefZ-Nahrung auf dem Server, ohne dass irgendwo
//     eine Zeile im Log steht.
//
// Die letzte Gruppe (VanillaGleich) ist deshalb die wichtigste des ganzen
// Tests: sie prueft die Kernzusage von 14 E2 auf der Rechenseite - ohne Regeln
// ist der Faktor exakt 1.0, und damit ist der Verfall bitgenau Vanilla.
//
// NICHT pruefbar ist alles, was ein Item braucht:
//
//   - dass Vanillas ProcessDecay das skalierte delta wirklich bekommt
//   - dass ein Vanilla-Steak diesen Code nie erreicht
//   - dass der Sync tatsaechlich gedrosselt wird
//   - dass preventsRotten den Uebergang verhindert statt ihn zu flackern
//
// Die ersten beiden sind STRUKTURELL zugesichert (eine Ableitung, ein
// super-Aufruf) und werden am Diff geprueft, nicht in einem Testlauf. Die
// letzten beiden brauchen einen Server mit Welt und bleiben dem Servertest
// vorbehalten. Ein nachgebautes Edible_Base wuerde den Nachbau pruefen, nicht
// die Engine.
//
// Der Test arbeitet auf EIGENEN Manager-Instanzen, nie auf den Singletons, und
// legt ausschliesslich Symbole mit dem Praefix "CHEFZ_PR_" an - Namen, die in
// echtem Content nicht vorkommen. Er beruehrt kein Item, keine Datei und keine
// Vanilla-Logik.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_PreservationSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("PreservationDef", ChefZ_PreservationDef.SelfCheck());
        Check("Produktkette",    ChainCheck());
        Check("Kategorien",      CategoryCheck());
        Check("Temperatur",      TemperatureCheck());
        Check("Schalter",        FlagCheck());
        Check("Klemmung",        ClampCheck());
        Check("AmSpieler",       OnPlayerCheck());
        Check("Frische",         FreshnessCheck());
        Check("Vererbung",       InheritCheck());
        Check("Abweisung",       RejectCheck());
        Check("LeereRegistry",   EmptyCheck());
        Check("VorBuild",        NotReadyCheck());
        Check("VanillaGleich",   VanillaUntouchedCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.PRESERV, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.PRESERV, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.PRESERV, "Selbsttest " + name + " FEHLGESCHLAGEN. Die Haltbarkeitsrechnung verhaelt sich " + "nicht wie entworfen - Verfallsgeschwindigkeit und Restfrische sind ab hier " + "unzuverlaessig. Vanilla-Nahrung ist davon unberuehrt.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S11: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
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

    //--------------------------------------------------------------------------

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
    private static ref array<ref ChefZ_Registry<ChefZ_PreservationDef>> s_AliveRegistries;

    // Eigentuemer fuer alles, was ein Manager nur SCHWACH haelt.
    //
    // Kategoriebaum, Ordinaltabelle und Fremdmanager stehen in den Managern
    // bewusst ohne ref - in Produktion haelt sie der Config Manager, der
    // laenger lebt als jeder von ihnen. Ein Fixture, das als Temporaeres
    // direkt in SetXForTest() oder Build() wandert, ist nach dieser Zeile
    // schon abgeraeumt; der Manager faellt dann still auf den echten
    // Singleton zurueck oder antwortet mit dem Rueckfallwert. Dieselbe Falle
    // wie bei s_AliveRegistries, nur eine Ebene hoeher.
    private static ref array<ref Managed> s_AliveFixtures;

    private static void Keep(Managed o)
    {
        if (!s_AliveFixtures)
            s_AliveFixtures = new array<ref Managed>();
        if (o)
            s_AliveFixtures.Insert(o);
    }

    private static ChefZ_Registry<ChefZ_PreservationDef> NewRegistry()
    {
        if (!s_AliveRegistries)
            s_AliveRegistries = new array<ref ChefZ_Registry<ChefZ_PreservationDef>>();

        ChefZ_Registry<ChefZ_PreservationDef> reg = new ChefZ_Registry<ChefZ_PreservationDef>();
        reg.Init(ChefZ_RecordKind.PRESERVATION);
        s_AliveRegistries.Insert(reg);
        return reg;
    }

    private static ChefZ_PreservationDef AddRule(notnull ChefZ_Registry<ChefZ_PreservationDef> reg, string id, string scope, float mul)
    {
        ChefZ_PreservationDef def = new ChefZ_PreservationDef();
        def.id                 = id;
        def.scope              = scope;
        def.spoilageMultiplier = mul;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.ADDON_JSON);
        def.ResolveDefaults();
        if (!def.Validate(null))
            return null;
        def.Compile(null);
        if (!reg.Add(def))
            return null;
        return def;
    }

    //--------------------------------------------------------------------------

    /**
     * Der Kategoriebaum des Tests: eine Wurzel mit einem Kind, plus zwei Tags.
     *
     * Die Namen tragen das Praefix CHEFZ_PR_ und sind damit ausdruecklich KEIN
     * Content - sie existieren nur innerhalb dieses Tests.
     */
    private static ChefZ_CategoryManager NewCats()
    {
        ChefZ_Registry<ChefZ_CategoryDef> cats = new ChefZ_Registry<ChefZ_CategoryDef>();
        cats.Init(ChefZ_RecordKind.CATEGORY);
        AddCategory(cats, "CHEFZ_PR_ROOT", "");
        AddCategory(cats, "CHEFZ_PR_CHILD", "CHEFZ_PR_ROOT");

        ChefZ_Registry<ChefZ_TagDef> tags = new ChefZ_Registry<ChefZ_TagDef>();
        tags.Init(ChefZ_RecordKind.TAG);
        AddTag(tags, "CHEFZ_PR_TAG_A");
        AddTag(tags, "CHEFZ_PR_TAG_B");

        ChefZ_CategoryManager mgr = new ChefZ_CategoryManager();
        mgr.SetQuietForTest(true);
        mgr.Build(cats, tags, null);
        return mgr;
    }

    private static void AddCategory(notnull ChefZ_Registry<ChefZ_CategoryDef> reg, string id, string parent)
    {
        ChefZ_CategoryDef def = new ChefZ_CategoryDef();
        def.id     = id;
        def.parent = parent;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.ResolveDefaults();
        def.Validate(null);
        def.Compile(null);
        reg.Add(def);
    }

    private static void AddTag(notnull ChefZ_Registry<ChefZ_TagDef> reg, string id)
    {
        ChefZ_TagDef def = new ChefZ_TagDef();
        def.id = id;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.ResolveDefaults();
        def.Validate(null);
        def.Compile(null);
        reg.Add(def);
    }

    //--------------------------------------------------------------------------

    //! Zwei Zustaende: einer neutral, einer mit eigenem Verderbfaktor und
    //! eigener Frischelebensdauer.
    private static ChefZ_StateManager NewStates()
    {
        ChefZ_Registry<ChefZ_StateDef> reg = new ChefZ_Registry<ChefZ_StateDef>();
        reg.Init(ChefZ_RecordKind.STATE);

        AddState(reg, "CHEFZ_PR_STATE_PLAIN", 1.0, ChefZ_Undefined.FLOAT);
        AddState(reg, "CHEFZ_PR_STATE_KEEP",  0.5, 1000.0);

        // Die Registry ist Eigentuemerin der Defs, der Manager haelt sie nur
        // schwach - stirbt sie hier, antwortet GetSpoilageMultiplier mit dem
        // Rueckfall 1.0, und die Produktkette verliert ihren Faktor.
        Keep(reg);
        ChefZ_CategoryManager cats = NewCats();
        Keep(cats);

        ChefZ_StateManager mgr = new ChefZ_StateManager();
        mgr.SetQuietForTest(true);
        mgr.SetCategoryManagerForTest(cats);
        mgr.Build(reg, null, null);
        return mgr;
    }

    private static void AddState(notnull ChefZ_Registry<ChefZ_StateDef> reg, string id, float spoilage, float lifetime)
    {
        ChefZ_StateDef def = new ChefZ_StateDef();
        def.id                   = id;
        def.spoilageMultiplier   = spoilage;
        def.freshnessLifetimeSec = lifetime;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.ResolveDefaults();
        def.Validate(null);
        def.Compile(null);
        reg.Add(def);
    }

    //! Zwei Stufen: eine neutral, eine haltbarer.
    private static ChefZ_QualityManager NewQuality()
    {
        ChefZ_Registry<ChefZ_QualityTierDef> reg = new ChefZ_Registry<ChefZ_QualityTierDef>();
        reg.Init(ChefZ_RecordKind.QUALITY_TIER);

        AddTier(reg, "CHEFZ_PR_TIER_PLAIN", 0, 1.0);
        AddTier(reg, "CHEFZ_PR_TIER_GOOD",  1, 0.5);

        Keep(reg);
        ChefZ_CategoryManager cats = NewCats();
        Keep(cats);

        ChefZ_QualityManager mgr = new ChefZ_QualityManager();
        mgr.SetQuietForTest(true);
        mgr.SetCategoryManagerForTest(cats);
        mgr.Build(reg, null, null, null);
        return mgr;
    }

    private static void AddTier(notnull ChefZ_Registry<ChefZ_QualityTierDef> reg, string id, int rank, float spoilage)
    {
        ChefZ_QualityTierDef def = new ChefZ_QualityTierDef();
        def.id                 = id;
        def.tierSet            = "CHEFZ_PR_SET";
        def.rank               = rank;
        def.spoilageMultiplier = spoilage;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.ResolveDefaults();
        def.Validate(null);
        def.Compile(null);
        reg.Add(def);
    }

    //--------------------------------------------------------------------------

    private static ChefZ_PreservationManager NewManager(ChefZ_CategoryManager cats, ChefZ_StateManager states, ChefZ_QualityManager quality)
    {
        // SetManagersForTest haelt die drei ohne ref. Als Temporaere
        // uebergeben - NewManager(NewCats(), NewStates(), NewQuality()) -
        // waeren sie nach dieser Zeile weg, und der Preservation Manager
        // fiele auf die noch ungebauten Singletons zurueck: der Zielabgleich
        // beim Build waere stumm, RejectCheck saehe 1 statt 4 Abweisungen.
        Keep(cats);
        Keep(states);
        Keep(quality);

        ChefZ_PreservationManager mgr = new ChefZ_PreservationManager();
        mgr.SetQuietForTest(true);
        mgr.SetManagersForTest(cats, states, quality);
        return mgr;
    }

    //! Einstellungen mit genau den Werten, die eine Rechnung nachpruefbar
    //! machen: neutrale Skala, weite Grenzen, bekannte Frischelebensdauer.
    private static ChefZ_CoreSettingsDef NewSettings(float globalScale)
    {
        ChefZ_CoreSettingsDef s = new ChefZ_CoreSettingsDef();
        s.id                          = ChefZ_CoreSettingsDef.PRIMARY_ID;
        s.globalSpoilageScale         = globalScale;
        s.minDecayScale               = 0.01;
        s.maxDecayScale               = 10.0;
        s.defaultFreshnessLifetimeSec = 100.0;
        s.ResolveDefaults();
        return s;
    }

    //! Eine leere Closure - der Normalfall fuer ein Item ohne Kategorien.
    private static ChefZ_CategoryClosure EmptyClosure()
    {
        return new ChefZ_CategoryClosure();
    }

    private static array<ChefZ_Sym> NoTags()
    {
        return new array<ChefZ_Sym>();
    }

    private static float Scale(notnull ChefZ_PreservationManager mgr, ChefZ_Sym state, ChefZ_Sym quality, ChefZ_Sym cls, ChefZ_CategoryClosure closure, array<ChefZ_Sym> tags)
    {
        array<string> trace = null;
        return mgr.ComputeDecayScale(state, quality, cls, closure, tags, 1.0, ChefZ_Undefined.FLOAT, trace);
    }

    //==========================================================================
    // 1. Die Produktkette aus 14 §3
    //==========================================================================

    /**
     * Jeder Faktor traegt einen ANDEREN Wert. Stimmt das Produkt, stimmt jeder
     * einzelne Faktor - dieselbe Beweisfuehrung wie im Qualitaetsselbsttest.
     *
     *   global   2.0
     *   Zustand  0.5   (ChefZ_StateDef.spoilageMultiplier)
     *   Stufe    0.5   (ChefZ_QualityTierDef.spoilageMultiplier)
     *   Regel    0.4   (scope "state")
     *   Klasse   0.25  (scope "class")
     *   ------------
     *            0.05  -> liegt in [minDecayScale, maxDecayScale], wird also
     *                     nicht geklemmt und ist damit ein echter Beweis fuer
     *                     jeden einzelnen Faktor.
     */
    private static bool ChainCheck()
    {
        ChefZ_CategoryManager cats    = NewCats();
        ChefZ_StateManager    states  = NewStates();
        ChefZ_QualityManager  quality = NewQuality();

        ChefZ_Registry<ChefZ_PreservationDef> reg = NewRegistry();
        AddRule(reg, "CHEFZ_PR_STATE_KEEP", ChefZ_PreservationScope.NAME_STATE, 0.4);
        AddRule(reg, "CHEFZ_PR_CLASS_X",    ChefZ_PreservationScope.NAME_CLASS, 0.25);

        ChefZ_PreservationManager mgr = NewManager(cats, states, quality);
        mgr.Build(reg, null, NewSettings(2.0));

        if (!mgr.IsReady())                             return false;
        if (mgr.GetRuleCount() != 2)                    return false;
        if (mgr.GetRejectedCount() != 0)                return false;

        float mul = Scale(mgr, Sym("CHEFZ_PR_STATE_KEEP"), Sym("CHEFZ_PR_TIER_GOOD"), Sym("CHEFZ_PR_CLASS_X"), EmptyClosure(), NoTags());

        if (!Near(mul, 2.0 * 0.5 * 0.5 * 0.4 * 0.25))   return false;

        // Ohne Zustand, ohne Stufe, ohne bekannte Klasse bleibt nur die
        // globale Skala uebrig.
        float bare = Scale(mgr, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, Sym("CHEFZ_PR_CLASS_UNKNOWN"), EmptyClosure(), NoTags());
        if (!Near(bare, 2.0))                           return false;

        // Der Behaelterfaktor multipliziert ebenfalls (16, Default 1.0).
        //
        // Die Liste wird hier ANGELEGT: eine null-Liste heisst "keine
        // Ablaufverfolgung" und ist der Normalfall im Verfallstakt (siehe
        // ComputeDecayScale). Der Test will sie, also legt er sie an.
        array<string> trace = new array<string>();
        float withContainer = mgr.ComputeDecayScale( ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), NoTags(), 0.5, ChefZ_Undefined.FLOAT, trace);
        if (!Near(withContainer, 1.0))                  return false;   // 2.0 * 0.5

        // Und der Trace ist nicht leer, wenn er angefordert wird - er ist die
        // Antwort auf "warum haelt das Ding so lange".
        if (!trace || trace.Count() < 2)                return false;

        return true;
    }

    //==========================================================================
    // 2. Kategorien und Tags
    //==========================================================================

    /**
     * Die Vorfahrenregel (04 E1) ist der Grund, warum Kategorien als
     * Feinschliff taugen: EINE Regel auf der Oberkategorie trifft jede
     * Unterkategorie. Ohne sie muesste ein Content-Autor jede Unterkategorie
     * einzeln nennen - und die naechste vergessen.
     */
    private static bool CategoryCheck()
    {
        ChefZ_CategoryManager cats = NewCats();

        ChefZ_Registry<ChefZ_PreservationDef> reg = NewRegistry();
        AddRule(reg, "CHEFZ_PR_ROOT",    ChefZ_PreservationScope.NAME_CATEGORY, 0.5);
        AddRule(reg, "CHEFZ_PR_TAG_A",   ChefZ_PreservationScope.NAME_TAG, 0.5);
        AddRule(reg, "CHEFZ_PR_TAG_B",   ChefZ_PreservationScope.NAME_TAG, 0.5);

        ChefZ_PreservationManager mgr = NewManager(cats, NewStates(), NewQuality());
        mgr.Build(reg, null, NewSettings(1.0));
        if (mgr.GetRuleCount() != 3)                    return false;

        // Ein Item in der UNTERkategorie: die Regel der Oberkategorie greift.
        array<ChefZ_Sym> direct = new array<ChefZ_Sym>();
        direct.Insert(Sym("CHEFZ_PR_CHILD"));
        ChefZ_CategoryClosure closure;
        cats.BuildClosure(direct, closure);

        if (!Near(Scale(mgr, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, closure, NoTags()), 0.5))
            return false;

        // Ein Item ohne Kategorie bleibt unberuehrt.
        if (!Near(Scale(mgr, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), NoTags()), 1.0))
            return false;

        // 14 §8: "Mehrere Records treffen zu -> Multipliziert." Zwei Tags mit
        // je 0.5 ergeben 0.25, NICHT 1.0 (das waere die Summe) und nicht 0.5
        // (das waere "der erste gewinnt").
        array<ChefZ_Sym> both = new array<ChefZ_Sym>();
        both.Insert(Sym("CHEFZ_PR_TAG_A"));
        both.Insert(Sym("CHEFZ_PR_TAG_B"));
        if (!Near(Scale(mgr, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), both), 0.25))
            return false;

        // Kategorie UND Tags zusammen: alles multipliziert.
        if (!Near(Scale(mgr, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, closure, both), 0.125))
            return false;

        return true;
    }

    //==========================================================================
    // 3. Temperaturbindung
    //==========================================================================

    private static bool TemperatureCheck()
    {
        ChefZ_Registry<ChefZ_PreservationDef> reg = NewRegistry();
        ChefZ_PreservationDef cold = AddRule(reg, "CHEFZ_PR_TAG_A", ChefZ_PreservationScope.NAME_TAG, 0.5);
        if (!cold)                                      return false;
        cold.environmentTemperature = new ChefZ_Range();
        cold.environmentTemperature.Init(-50.0, 5.0);

        ChefZ_PreservationManager mgr = NewManager(NewCats(), NewStates(), NewQuality());
        mgr.Build(reg, null, NewSettings(1.0));

        array<ChefZ_Sym> tags = new array<ChefZ_Sym>();
        tags.Insert(Sym("CHEFZ_PR_TAG_A"));

        array<string> trace = null;

        // In der Kaelte greift die Regel.
        //
        // -10.0 und nicht 0.0: seit ChefZ_Undefined.FLOAT == 0.0 IST die
        // Null der Sentinel fuer "Temperatur unbekannt". Mit 0.0 haette
        // dieser Test denselben Wert einmal als Kaelte (Regel greift) und
        // drei Zeilen weiter als unbekannt (Regel greift nicht) gefordert -
        // beides zugleich ist nicht erfuellbar. -10.0 liegt ebenso im
        // Bereich -50..5 und ist als Wert lesbar.
        //
        // Die Einschraenkung dahinter bleibt und ist echt: eine
        // Umgebungstemperatur von genau 0 Grad gilt als unbekannt, und
        // Kaelteregeln greifen dort nicht. Am Gefrierpunkt ist das die
        // sichere Richtung (02 §8: "Richtung weniger ChefZ"), aber es ist
        // eine Einschraenkung und keine Absicht.
        if (!Near(mgr.ComputeDecayScale(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), tags, 1.0, -10.0, trace), 0.5))
            return false;

        // In der Waerme nicht.
        if (!Near(mgr.ComputeDecayScale(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), tags, 1.0, 25.0, trace), 1.0))
            return false;

        // Und bei unbekannter Temperatur ebenfalls nicht (02 §8: "Richtung
        // weniger ChefZ, nie Richtung falsches ChefZ").
        if (!Near(mgr.ComputeDecayScale(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), tags, 1.0, ChefZ_Undefined.FLOAT, trace), 1.0))
            return false;

        return true;
    }

    //==========================================================================
    // 4. Die beiden Schalter (14 E7)
    //==========================================================================

    private static bool FlagCheck()
    {
        ChefZ_Registry<ChefZ_PreservationDef> reg = NewRegistry();

        ChefZ_PreservationDef canned = AddRule(reg, "CHEFZ_PR_TAG_A", ChefZ_PreservationScope.NAME_TAG, 1.0);
        if (!canned)                                    return false;
        canned.stopsDecay = true;

        ChefZ_PreservationDef noRot = AddRule(reg, "CHEFZ_PR_STATE_KEEP", ChefZ_PreservationScope.NAME_STATE, 1.0);
        if (!noRot)                                     return false;
        noRot.preventsRotten = true;

        ChefZ_PreservationManager mgr = NewManager(NewCats(), NewStates(), NewQuality());
        mgr.Build(reg, null, NewSettings(1.0));

        array<ChefZ_Sym> tags = new array<ChefZ_Sym>();
        tags.Insert(Sym("CHEFZ_PR_TAG_A"));

        // Die billige Vorfrage der Itemseite muss ja sagen, sonst wird die
        // teure nie gestellt - und der Schalter waere still wirkungslos.
        if (!mgr.HasAnyStopsDecay())                    return false;
        if (!mgr.HasAnyPreventsRotten())                return false;

        // Und sie muss nein sagen, wenn niemand einen Schalter setzt. Das ist
        // die Seite, die im Betrieb zaehlt: sie haelt CanProcessDecay auf
        // einem Server ohne Konservenregel billig.
        ChefZ_PreservationManager plain = NewManager(NewCats(), NewStates(), NewQuality());
        ChefZ_Registry<ChefZ_PreservationDef> plainReg = NewRegistry();
        AddRule(plainReg, "CHEFZ_PR_TAG_B", ChefZ_PreservationScope.NAME_TAG, 0.5);
        plain.Build(plainReg, null, NewSettings(1.0));
        if (plain.HasAnyStopsDecay())                   return false;
        if (plain.HasAnyPreventsRotten())               return false;

        // stopsDecay ueber einen Tag.
        if (!mgr.StopsDecay(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), tags))
            return false;

        // 14 E7: die beiden Schalter sind GETRENNT. Ein stopsDecay-Record
        // sagt nichts ueber preventsRotten und umgekehrt.
        if (mgr.PreventsRotten(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), tags))
            return false;

        if (!mgr.PreventsRotten(Sym("CHEFZ_PR_STATE_KEEP"), ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), NoTags()))
            return false;

        if (mgr.StopsDecay(Sym("CHEFZ_PR_STATE_KEEP"), ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), NoTags()))
            return false;

        // Ein Item, das keine der beiden Regeln trifft, ist von beiden
        // unberuehrt - das ist der Vanilla-Fall.
        if (mgr.StopsDecay(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), NoTags()))
            return false;

        return true;
    }

    //==========================================================================
    // 5. Klemmung (14 §8)
    //==========================================================================

    private static bool ClampCheck()
    {
        ChefZ_Registry<ChefZ_PreservationDef> reg = NewRegistry();
        AddRule(reg, "CHEFZ_PR_TAG_A", ChefZ_PreservationScope.NAME_TAG, 0.02);
        AddRule(reg, "CHEFZ_PR_TAG_B", ChefZ_PreservationScope.NAME_TAG, 0.02);

        ChefZ_PreservationManager mgr = NewManager(NewCats(), NewStates(), NewQuality());
        mgr.Build(reg, null, NewSettings(1.0));

        array<ChefZ_Sym> both = new array<ChefZ_Sym>();
        both.Insert(Sym("CHEFZ_PR_TAG_A"));
        both.Insert(Sym("CHEFZ_PR_TAG_B"));

        // 0.02 * 0.02 = 0.0004, geklemmt auf minDecayScale.
        if (!Near(Scale(mgr, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), both), 0.01))
            return false;

        // Nach oben ebenso.
        ChefZ_Registry<ChefZ_PreservationDef> up = NewRegistry();
        AddRule(up, "CHEFZ_PR_TAG_A", ChefZ_PreservationScope.NAME_TAG, 9.0);
        AddRule(up, "CHEFZ_PR_TAG_B", ChefZ_PreservationScope.NAME_TAG, 9.0);

        ChefZ_PreservationManager fast = NewManager(NewCats(), NewStates(), NewQuality());
        fast.Build(up, null, NewSettings(1.0));
        if (!Near(Scale(fast, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), both), 10.0))
            return false;

        // Und die oeffentliche Klemmung, die der Spielerbonus benutzt.
        if (!Near(fast.ClampToBounds(1000.0), 10.0))    return false;
        if (!Near(fast.ClampToBounds(0.0), 0.01))       return false;
        if (!Near(fast.ClampToBounds(1.0), 1.0))        return false;

        return true;
    }

    //==========================================================================
    // 6. Der Spielerbonus (14 §5, onPlayerMultiplier)
    //==========================================================================

    private static bool OnPlayerCheck()
    {
        ChefZ_Registry<ChefZ_PreservationDef> reg = NewRegistry();

        ChefZ_PreservationDef def = AddRule(reg, "CHEFZ_PR_TAG_A", ChefZ_PreservationScope.NAME_TAG, 1.0);
        if (!def)                                       return false;
        def.onPlayerMultiplier = 2.0;

        ChefZ_PreservationManager mgr = NewManager(NewCats(), NewStates(), NewQuality());
        mgr.Build(reg, null, NewSettings(1.0));

        array<ChefZ_Sym> tags = new array<ChefZ_Sym>();
        tags.Insert(Sym("CHEFZ_PR_TAG_A"));

        if (!Near(mgr.ComputeOnPlayerScale(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), tags, ChefZ_Undefined.FLOAT), 2.0))
            return false;

        // Ohne Regel: neutral. Vanillas eigener Spielerbonus bleibt damit die
        // einzige Wirkung - genau das, was der Sentinel bedeutet.
        if (!Near(mgr.ComputeOnPlayerScale(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), NoTags(), ChefZ_Undefined.FLOAT), 1.0))
            return false;

        return true;
    }

    //==========================================================================
    // 7. Restfrische (14 §4)
    //==========================================================================

    private static bool FreshnessCheck()
    {
        ChefZ_PreservationManager mgr = NewManager(NewCats(), NewStates(), NewQuality());
        mgr.Build(NewRegistry(), null, NewSettings(1.0));

        // Servervorgabe 100 s: ein Tick von 10 s mit Faktor 1.0 kostet 0.1.
        float a = mgr.AdvanceFreshness(1.0, 10.0, 1.0, ChefZ_SymbolTable.INVALID);
        if (!Near(a, 0.9))                              return false;

        // Faktor 0.5 halbiert den Verlust - Frische und Verfall im selben Takt.
        float b = mgr.AdvanceFreshness(1.0, 10.0, 0.5, ChefZ_SymbolTable.INVALID);
        if (!Near(b, 0.95))                             return false;

        // Der Zustand ueberschreibt die Servervorgabe (06 §4.1): 1000 s.
        float c = mgr.AdvanceFreshness(1.0, 10.0, 1.0, Sym("CHEFZ_PR_STATE_KEEP"));
        if (!Near(c, 0.99))                             return false;
        if (!Near(mgr.GetFreshnessLifetimeSec(Sym("CHEFZ_PR_STATE_KEEP")), 1000.0))
            return false;
        if (!Near(mgr.GetFreshnessLifetimeSec(Sym("CHEFZ_PR_STATE_PLAIN")), 100.0))
            return false;

        // Monoton fallend und bei 0 angekommen: KEIN Unterlauf, und
        // ausdruecklich keine Folge (14 §4, "Frische bestraft, Vanilla toetet").
        if (!Near(mgr.AdvanceFreshness(0.05, 1000.0, 1.0, ChefZ_SymbolTable.INVALID), 0.0))
            return false;
        if (!Near(mgr.AdvanceFreshness(0.0, 1000.0, 1.0, ChefZ_SymbolTable.INVALID), 0.0))
            return false;

        // Ein Tick ohne Zeit oder ohne Faktor aendert nichts.
        if (!Near(mgr.AdvanceFreshness(0.7, 0.0, 1.0, ChefZ_SymbolTable.INVALID), 0.7))
            return false;
        if (!Near(mgr.AdvanceFreshness(0.7, 10.0, 0.0, ChefZ_SymbolTable.INVALID), 0.7))
            return false;

        // 14 §8: Lebensdauer <= 0 friert die Frische ein.
        ChefZ_PreservationManager frozen = NewManager(NewCats(), NewStates(), NewQuality());
        ChefZ_CoreSettingsDef noLifetime = NewSettings(1.0);
        noLifetime.defaultFreshnessLifetimeSec = 0.0;
        // Die 0 ist hier die Aussage (14 §8), nicht ihr Fehlen - ohne die
        // Markierung ersetzt ResolveDefaults sie durch die Vorgabelaufzeit
        // und nichts friert ein.
        noLifetime.MarkExplicit("defaultFreshnessLifetimeSec");
        frozen.Build(NewRegistry(), null, noLifetime);
        if (!Near(frozen.AdvanceFreshness(0.7, 10000.0, 1.0, ChefZ_SymbolTable.INVALID), 0.7))
            return false;

        // 14 §8: unbrauchbare Frische wird zu 1.0 - und der Test erkennt sie
        // ueber "nicht in 0..1", damit auch NaN hineinfaellt.
        if (!ChefZ_PreservationManager.IsFreshnessBroken(-1.0))     return false;
        if (!ChefZ_PreservationManager.IsFreshnessBroken(2.0))      return false;
        if (ChefZ_PreservationManager.IsFreshnessBroken(0.0))       return false;
        if (ChefZ_PreservationManager.IsFreshnessBroken(1.0))       return false;
        if (!Near(ChefZ_PreservationManager.Sanitize(-1.0), 1.0))   return false;
        if (!Near(ChefZ_PreservationManager.Sanitize(0.5), 0.5))    return false;

        // Restlaufzeit: nur Anzeige, aber sie muss rechnen koennen.
        if (!Near(mgr.EstimateRemainingSec(0.5, ChefZ_SymbolTable.INVALID, 1.0), 50.0))
            return false;
        if (!Near(mgr.EstimateRemainingSec(0.5, ChefZ_SymbolTable.INVALID, 0.5), 100.0))
            return false;
        if (mgr.EstimateRemainingSec(0.5, ChefZ_SymbolTable.INVALID, 0.0) >= 0.0)
            return false;

        return true;
    }

    //==========================================================================
    // 8. Vererbung der Frische
    //==========================================================================

    /**
     * Die wichtigste Zeile: MIN ist die VORGABE.
     *
     * Mit einem Mittelwert liesse sich altes Fleisch in frischem waschen -
     * genau der Exploit, den 12 §4.1 schliesst. Ein Test, der nur MEAN prueft,
     * wuerde einen vertauschten Default nicht bemerken.
     */
    private static bool InheritCheck()
    {
        ChefZ_PreservationManager mgr = NewManager(NewCats(), NewStates(), NewQuality());
        mgr.Build(NewRegistry(), null, NewSettings(1.0));

        array<ref ChefZ_ItemFacts> inputs = new array<ref ChefZ_ItemFacts>();
        inputs.Insert(Facts(1.0, 1.0));
        inputs.Insert(Facts(0.2, 3.0));

        // Vorgabe und leerer Regelname: MIN.
        if (!Near(mgr.InheritFreshness(inputs, 1.0, ""), 0.2))          return false;
        if (!Near(mgr.InheritFreshness(inputs, 1.0, "MIN"), 0.2))       return false;
        if (!Near(mgr.InheritFreshness(inputs, 1.0, "unfug"), 0.2))     return false;

        // MEAN: (1.0 + 0.2) / 2
        if (!Near(mgr.InheritFreshness(inputs, 1.0, "MEAN"), 0.6))      return false;

        // WEIGHTED_MEAN nach units: (1.0*1 + 0.2*3) / 4
        if (!Near(mgr.InheritFreshness(inputs, 1.0, "WEIGHTED_MEAN"), 0.4)) return false;

        // carry multipliziert; negativ heisst "unveraendert" (Sentinel wie in
        // ChefZ_ItemStateComponent.InheritFrom).
        if (!Near(mgr.InheritFreshness(inputs, 0.5, "MIN"), 0.1))       return false;
        if (!Near(mgr.InheritFreshness(inputs, -1.0, "MIN"), 0.2))      return false;

        // Leere Eingabe: ein Gericht aus dem Nichts ist frisch.
        array<ref ChefZ_ItemFacts> none = new array<ref ChefZ_ItemFacts>();
        if (!Near(mgr.InheritFreshness(none, 1.0, "MIN"), 1.0))         return false;
        if (!Near(mgr.InheritFreshness(null, 1.0, "MIN"), 1.0))         return false;

        return true;
    }

    private static ChefZ_ItemFacts Facts(float freshness, float units)
    {
        ChefZ_ItemFacts f = new ChefZ_ItemFacts();
        f.freshness01 = freshness;
        f.units       = units;
        return f;
    }

    //==========================================================================
    // 9. Abweisung (14 §8)
    //==========================================================================

    /**
     * "Record nennt unbekannten Zustand/Kategorie/Tag -> Record abgewiesen,
     *  ERROR beim Build. Restliche Records wirken."
     *
     * Der zweite Halbsatz ist der wichtigere: ein Tippfehler in einer Regel
     * darf nicht die ganze Datei kosten.
     */
    private static bool RejectCheck()
    {
        ChefZ_Registry<ChefZ_PreservationDef> reg = NewRegistry();
        AddRule(reg, "CHEFZ_PR_GIBTSNICHT",  ChefZ_PreservationScope.NAME_STATE, 0.1);
        AddRule(reg, "CHEFZ_PR_KEINEKAT",    ChefZ_PreservationScope.NAME_CATEGORY, 0.1);
        AddRule(reg, "CHEFZ_PR_KEINTAG",     ChefZ_PreservationScope.NAME_TAG, 0.1);
        AddRule(reg, "CHEFZ_PR_KEINESTUFE",  ChefZ_PreservationScope.NAME_QUALITY, 0.1);
        AddRule(reg, "CHEFZ_PR_TAG_A",       ChefZ_PreservationScope.NAME_TAG, 0.5);

        ChefZ_PreservationManager mgr = NewManager(NewCats(), NewStates(), NewQuality());
        mgr.Build(reg, null, NewSettings(1.0));

        if (mgr.GetRejectedCount() != 4)                return false;
        if (mgr.GetRuleCount() != 1)                    return false;

        // Der ueberlebende Record wirkt.
        array<ChefZ_Sym> tags = new array<ChefZ_Sym>();
        tags.Insert(Sym("CHEFZ_PR_TAG_A"));
        if (!Near(Scale(mgr, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), tags), 0.5))
            return false;

        // Ein unbekannter scope wird schon vom Record abgewiesen und kommt nie
        // bis hierher (siehe ChefZ_PreservationDef.SelfCheck, Punkt 2). Eine
        // Klasse dagegen wird NICHT geprueft - 05 E3: nicht deklarierte
        // Klassen bleiben adressierbar.
        ChefZ_Registry<ChefZ_PreservationDef> classReg = NewRegistry();
        AddRule(classReg, "CHEFZ_PR_IRGENDEINEKLASSE", ChefZ_PreservationScope.NAME_CLASS, 0.5);

        ChefZ_PreservationManager classMgr = NewManager(NewCats(), NewStates(), NewQuality());
        classMgr.Build(classReg, null, NewSettings(1.0));
        if (classMgr.GetRuleCount() != 1)               return false;
        if (classMgr.GetRejectedCount() != 0)           return false;

        return true;
    }

    //==========================================================================
    // 10. Leere Registry und Zustand vor dem Build
    //==========================================================================

    private static bool EmptyCheck()
    {
        ChefZ_PreservationManager mgr = NewManager(NewCats(), NewStates(), NewQuality());
        mgr.Build(NewRegistry(), null, NewSettings(1.0));

        if (!mgr.IsReady())                             return false;
        if (mgr.GetRuleCount() != 0)                    return false;

        // 14 §8, erste Zeile: Faktor 1.0, Verfall bitgenau Vanilla.
        if (!Near(Scale(mgr, Sym("CHEFZ_PR_STATE_PLAIN"), Sym("CHEFZ_PR_TIER_PLAIN"), Sym("CHEFZ_PR_CLASS_X"), EmptyClosure(), NoTags()), 1.0))
            return false;

        // Auch ganz ohne Registry und ohne Einstellungen (SAFE_MODE-Rueckbau).
        ChefZ_PreservationManager safe = NewManager(null, null, null);
        safe.Build(null, null, null);
        if (!safe.IsReady())                            return false;
        if (!Near(Scale(safe, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, null, null), 1.0))
            return false;
        if (safe.StopsDecay(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID))
            return false;

        return true;
    }

    private static bool NotReadyCheck()
    {
        ChefZ_PreservationManager mgr = NewManager(NewCats(), NewStates(), NewQuality());
        // KEIN Build.

        if (mgr.IsReady())                              return false;
        if (!Near(Scale(mgr, Sym("CHEFZ_PR_STATE_KEEP"), ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, EmptyClosure(), NoTags()), 1.0))
            return false;
        if (mgr.StopsDecay(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID))
            return false;
        if (mgr.PreventsRotten(ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID, ChefZ_SymbolTable.INVALID))
            return false;

        return true;
    }

    //==========================================================================
    // 11. Die Kernzusage: ohne Regeln passiert NICHTS
    //==========================================================================

    /**
     * 14 E2, auf der Rechenseite geprueft.
     *
     * Ein Server, der ChefZ_Core installiert und KEIN Content-Modul hat, muss
     * fuer jede denkbare Eingabe den Faktor 1.0 bekommen. Waere er 0.99, waere
     * die Haltbarkeit jeder ChefZ-Nahrung auf dem Server verschoben, ohne dass
     * irgendwo eine Zeile im Log stuende - und die Verschiebung waere bei einer
     * Messung im Spiel praktisch nicht von Zufall zu unterscheiden.
     *
     * Der Test tastet dafuer die ganze Flaeche ab: mit und ohne Zustand, mit
     * und ohne Stufe, mit und ohne Kategorie, mit und ohne Tag, bei jeder
     * Temperatur, am Spieler und nicht am Spieler.
     */
    private static bool VanillaUntouchedCheck()
    {
        ChefZ_CategoryManager cats = NewCats();
        ChefZ_PreservationManager mgr = NewManager(cats, NewStates(), NewQuality());
        mgr.Build(NewRegistry(), null, NewSettings(1.0));

        array<ChefZ_Sym> direct = new array<ChefZ_Sym>();
        direct.Insert(Sym("CHEFZ_PR_CHILD"));
        ChefZ_CategoryClosure closure;
        cats.BuildClosure(direct, closure);

        array<ChefZ_Sym> tags = new array<ChefZ_Sym>();
        tags.Insert(Sym("CHEFZ_PR_TAG_A"));
        tags.Insert(Sym("CHEFZ_PR_TAG_B"));

        array<string> trace = null;

        // Ein Zustand MIT eigenem Verderbfaktor darf hier nicht vorkommen -
        // der waere kein Beweis. Deshalb der neutrale Zustand.
        array<float> temps = new array<float>();
        temps.Insert(-40.0);
        temps.Insert(15.0);
        temps.Insert(60.0);

        for (int i = 0; i < temps.Count(); i++)
        {
            float mul = mgr.ComputeDecayScale( Sym("CHEFZ_PR_STATE_PLAIN"), Sym("CHEFZ_PR_TIER_PLAIN"), Sym("CHEFZ_PR_CLASS_X"), closure, tags, 1.0, temps.Get(i), trace);
            if (!Near(mul, 1.0))
                return false;
        }

        if (!Near(mgr.ComputeOnPlayerScale(Sym("CHEFZ_PR_STATE_PLAIN"), Sym("CHEFZ_PR_TIER_PLAIN"), Sym("CHEFZ_PR_CLASS_X"), closure, tags, 15.0), 1.0))
            return false;

        if (mgr.StopsDecay(Sym("CHEFZ_PR_STATE_PLAIN"), Sym("CHEFZ_PR_TIER_PLAIN"), Sym("CHEFZ_PR_CLASS_X"), closure, tags))
            return false;
        if (mgr.PreventsRotten(Sym("CHEFZ_PR_STATE_PLAIN"), Sym("CHEFZ_PR_TIER_PLAIN"), Sym("CHEFZ_PR_CLASS_X"), closure, tags))
            return false;

        return true;
    }
}
