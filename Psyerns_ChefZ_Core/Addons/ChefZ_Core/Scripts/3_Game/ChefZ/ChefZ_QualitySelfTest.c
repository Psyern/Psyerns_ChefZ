//==============================================================================
// ChefZ_QualitySelfTest - Abnahmepruefung fuer S10
//
// Entwurf: 12 §4 (die Punktrechnung), 12 §4.1 (Frische als MINIMUM - die
// wichtigste Einzelregel), 12 §4.2 (additive Zustandsstrafen), 12 §8
// (Fehlerverhalten, Zeile fuer Zeile), 12 E1 (die Rechnung ist der Preis fuer
// die Erweiterbarkeit, die Inspizierbarkeit ist die Gegenleistung),
// 12 E4/E7/E8.
//
// ---------------------------------------------------------------------------
// Warum dieser Test im Auslieferungsstand bleibt
// ---------------------------------------------------------------------------
// Jeder Fehler dieses Teilsystems scheitert LEISE. Eine Frische, die als
// Mittelwert statt als Minimum eingeht, ergibt weiterhin Zahlen, weiterhin
// Stufen und weiterhin Gerichte - nur ist "altes Fleisch in einen
// Premium-Eintopf waschen" dann ein Standardexploit, und auffallen wird das
// erst einem Spieler, der ihn benutzt.
//
// Deshalb rechnet dieser Test die Formel aus 12 §4 an einem Beispiel nach, bei
// dem jeder einzelne Summand einen anderen Wert hat. Stimmt die Summe, stimmt
// jeder Summand.
//
// Der Test arbeitet auf EIGENEN Manager-Instanzen, nie auf dem Singleton, und
// legt ausschliesslich Symbole mit dem Praefix "CHEFZ_QT_" an - Namen, die in
// echtem Content nicht vorkommen. Er beruehrt kein Item, keine Datei und
// keine Vanilla-Logik.
//
// Layer: 3_Game.
//==============================================================================

/**
 * Ein Faehigkeitsanbieter fuer den Test (12 E6).
 *
 * Er beweist zweierlei: dass die Regelart ueberhaupt auswertbar ist, und dass
 * sie OHNE Anbieter still 0 Punkte gibt statt zu scheitern. Beides zusammen
 * ist die Zusage aus 12 §8, dass ein Server ohne Skillmodul voll spielbar
 * bleibt.
 */
class ChefZ_TestCapabilityProbe extends ChefZ_CapabilityProbe
{
    private string m_Known;
    private float  m_Value;

    void Init(string capability, float value)
    {
        m_Known = capability;
        m_Value = value;
    }

    override bool TryGetValue(string capability, int actorId, out float value)
    {
        value = 0.0;
        if (capability != m_Known)
            return false;
        value = m_Value;
        return true;
    }
}

//==============================================================================

class ChefZ_QualitySelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("TierDef",       ChefZ_QualityTierDef.SelfCheck());
        Check("Scoring",       ChefZ_QualityScoring.SelfCheck());
        Check("ScoringDef",    ChefZ_QualityScoringDef.SelfCheck());
        Check("Evaluation",    ChefZ_QualityEvaluation.SelfCheck());
        Check("GradeWhen",     ChefZ_GradeWhen.SelfCheck());
        Check("ContextKey",    ChefZ_GradeContextKey.SelfCheck());

        Check("Leiter",        LadderCheck());
        Check("Umordnung",     ReorderCheck());
        Check("Vergleiche",    CompareCheck());
        Check("Zusammenfassen", CombineCheck());
        Check("Wirkungen",     EffectCheck());
        Check("Punkte",        ScoreCheck());
        Check("Regeln",        RuleCheck());
        Check("LeereRegistry", EmptyCheck());
        Check("VorBuild",      NotReadyCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.QUALITY, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.QUALITY, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.QUALITY, "Selbsttest " + name + " FEHLGESCHLAGEN. Die Qualitaetsbewertung verhaelt sich " + "nicht wie entworfen - Stufen, Ausbeute und Haltbarkeit sind ab hier " + "unzuverlaessig. Vanilla-Kochen ist davon unberuehrt.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S10: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
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
        return ChefZ_SymbolTable.Lookup(name);
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
    private static ref array<ref ChefZ_Registry<ChefZ_QualityTierDef>> s_AliveRegistries;

    private static ChefZ_Registry<ChefZ_QualityTierDef> NewRegistry()
    {
        if (!s_AliveRegistries)
            s_AliveRegistries = new array<ref ChefZ_Registry<ChefZ_QualityTierDef>>();

        ChefZ_Registry<ChefZ_QualityTierDef> reg = new ChefZ_Registry<ChefZ_QualityTierDef>();
        reg.Init(ChefZ_RecordKind.QUALITY_TIER);
        s_AliveRegistries.Insert(reg);
        return reg;
    }

    private static ChefZ_QualityTierDef AddTier(notnull ChefZ_Registry<ChefZ_QualityTierDef> reg, string id, string tierSet, int rank, float minScore)
    {
        ChefZ_QualityTierDef def = new ChefZ_QualityTierDef();
        def.id       = id;
        def.tierSet  = tierSet;
        def.rank     = rank;
        def.minScore = minScore;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.ResolveDefaults();
        def.Validate(null);
        def.Compile(null);
        if (!reg.Add(def))
            return null;
        return def;
    }

    private static ChefZ_QualityManager NewManager()
    {
        ChefZ_QualityManager mgr = new ChefZ_QualityManager();
        mgr.SetQuietForTest(true);
        mgr.SetCategoryManagerForTest(NewTagOwner(new array<string>()));
        return mgr;
    }

    //! Ein Kategoriemanager mit genau den Tags, die der Test braucht. Ohne ihn
    //! filterte ResolveGrantedTags gegen den Bestand des echten Servers.
    private static ChefZ_CategoryManager NewTagOwner(array<string> tagIds)
    {
        ChefZ_Registry<ChefZ_TagDef> tags = new ChefZ_Registry<ChefZ_TagDef>();
        tags.Init(ChefZ_RecordKind.TAG);

        for (int i = 0; i < tagIds.Count(); i++)
        {
            ChefZ_TagDef def = new ChefZ_TagDef();
            def.id = tagIds.Get(i);
            def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
            def.Compile(null);
            tags.Add(def);
        }

        ChefZ_CategoryManager cats = new ChefZ_CategoryManager();
        cats.SetQuietForTest(true);
        cats.Build(null, tags, null);
        return cats;
    }

    /**
     * Die Standardleiter des Tests: vier Stufen in EINEM Satz, mit genau den
     * Schwellen aus dem Beispiel in 12 §3 - nur unter Testnamen, damit hier
     * kein Content entsteht.
     */
    private static ChefZ_Registry<ChefZ_QualityTierDef> StandardLadder()
    {
        ChefZ_Registry<ChefZ_QualityTierDef> reg = NewRegistry();
        AddTier(reg, "CHEFZ_QT_T0", "CHEFZ_QT_SET", 0, -99.0);
        AddTier(reg, "CHEFZ_QT_T1", "CHEFZ_QT_SET", 1, 0.0);
        AddTier(reg, "CHEFZ_QT_T2", "CHEFZ_QT_SET", 2, 2.0);
        AddTier(reg, "CHEFZ_QT_T3", "CHEFZ_QT_SET", 3, 7.0);
        return reg;
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

    //==========================================================================
    // 1. Die Leiter: Reihenfolge, Schwellen, Aufloesung
    //==========================================================================

    private static bool LadderCheck()
    {
        ChefZ_QualityManager mgr = NewManager();
        mgr.Build(StandardLadder(), null, null, null);

        if (!mgr.IsReady())                                         return false;
        if (mgr.GetTierCount() != 4)                                return false;
        if (mgr.GetTierSetCount() != 1)                             return false;

        ChefZ_Sym setSym = Sym("CHEFZ_QT_SET");
        if (!mgr.TierSetExists(setSym))                                return false;

        // Aufloesung an und zwischen den Schwellen.
        if (mgr.ResolveTier(-100.0, setSym) != Sym("CHEFZ_QT_T0"))     return false;
        if (mgr.ResolveTier(-1.0, setSym)   != Sym("CHEFZ_QT_T0"))     return false;
        if (mgr.ResolveTier(0.0, setSym)    != Sym("CHEFZ_QT_T1"))     return false;
        if (mgr.ResolveTier(1.99, setSym)   != Sym("CHEFZ_QT_T1"))     return false;
        if (mgr.ResolveTier(2.0, setSym)    != Sym("CHEFZ_QT_T2"))     return false;
        if (mgr.ResolveTier(6.99, setSym)   != Sym("CHEFZ_QT_T2"))     return false;
        if (mgr.ResolveTier(7.0, setSym)    != Sym("CHEFZ_QT_T3"))     return false;
        if (mgr.ResolveTier(9999.0, setSym) != Sym("CHEFZ_QT_T3"))     return false;

        if (mgr.GetLowestTier(setSym)  != Sym("CHEFZ_QT_T0"))          return false;
        if (mgr.GetHighestTier(setSym) != Sym("CHEFZ_QT_T3"))          return false;

        // 12 §8: ein unbekannter Stufensatz faellt auf den Vorgabesatz
        // zurueck. Hier gibt es den Vorgabesatz nicht - dann ist INVALID die
        // richtige Antwort, und NICHT irgendeine Stufe aus einem fremden Satz.
        ChefZ_Sym fremd = ChefZ_SymbolTable.Intern("CHEFZ_QT_FREMDERSATZ");
        if (ChefZ_SymbolTable.IsValid(mgr.ResolveTier(5.0, fremd))) return false;

        return true;
    }

    //==========================================================================
    // 2. Raenge, die nicht zu den Schwellen passen (12 §8)
    //==========================================================================

    private static bool ReorderCheck()
    {
        // Die Raenge stehen absichtlich verkehrt herum zu den Schwellen. Nach
        // dem Build muss die Leiter der SCHWELLE folgen, nicht dem Rang -
        // sonst landete DegradeTier irgendwo.
        ChefZ_Registry<ChefZ_QualityTierDef> reg = NewRegistry();
        AddTier(reg, "CHEFZ_QT_R_HOCH",   "CHEFZ_QT_RSET", 0, 10.0);
        AddTier(reg, "CHEFZ_QT_R_MITTE",  "CHEFZ_QT_RSET", 1, 5.0);
        AddTier(reg, "CHEFZ_QT_R_TIEF",   "CHEFZ_QT_RSET", 2, 0.0);

        ChefZ_QualityManager mgr = NewManager();
        mgr.Build(reg, null, null, null);

        ChefZ_Sym setSym = Sym("CHEFZ_QT_RSET");
        if (mgr.GetRank(Sym("CHEFZ_QT_R_TIEF"))  != 0)              return false;
        if (mgr.GetRank(Sym("CHEFZ_QT_R_MITTE")) != 1)              return false;
        if (mgr.GetRank(Sym("CHEFZ_QT_R_HOCH"))  != 2)              return false;

        if (mgr.ResolveTier(0.0, setSym)  != Sym("CHEFZ_QT_R_TIEF"))   return false;
        if (mgr.ResolveTier(10.0, setSym) != Sym("CHEFZ_QT_R_HOCH"))   return false;

        // 12 §8: die unterste Stufe faengt auch alles darunter ab - ein
        // Gericht faellt nie durch das Raster.
        ChefZ_Registry<ChefZ_QualityTierDef> gap = NewRegistry();
        AddTier(gap, "CHEFZ_QT_G_EINS", "CHEFZ_QT_GSET", 0, 3.0);
        AddTier(gap, "CHEFZ_QT_G_ZWEI", "CHEFZ_QT_GSET", 1, 6.0);

        ChefZ_QualityManager gapMgr = NewManager();
        gapMgr.Build(gap, null, null, null);
        ChefZ_Sym gapSet = Sym("CHEFZ_QT_GSET");
        if (gapMgr.ResolveTier(0.0, gapSet)   != Sym("CHEFZ_QT_G_EINS"))    return false;
        if (gapMgr.ResolveTier(-50.0, gapSet) != Sym("CHEFZ_QT_G_EINS"))    return false;
        if (gapMgr.ResolveTier(6.0, gapSet)   != Sym("CHEFZ_QT_G_ZWEI"))    return false;

        return true;
    }

    //==========================================================================
    // 3. Vergleichen, absenken, verschieben (12 E4, E8)
    //==========================================================================

    private static bool CompareCheck()
    {
        ChefZ_Registry<ChefZ_QualityTierDef> reg = StandardLadder();
        // Ein zweiter Satz, damit der satzuebergreifende Vergleich pruefbar
        // ist (12 E4: zwischen Saetzen gibt es keine Ordnung).
        AddTier(reg, "CHEFZ_QT_X0", "CHEFZ_QT_ANDERERSET", 0, 0.0);
        AddTier(reg, "CHEFZ_QT_X1", "CHEFZ_QT_ANDERERSET", 1, 5.0);

        ChefZ_QualityManager mgr = NewManager();
        mgr.Build(reg, null, null, null);

        ChefZ_Sym t0 = Sym("CHEFZ_QT_T0");
        ChefZ_Sym t1 = Sym("CHEFZ_QT_T1");
        ChefZ_Sym t3 = Sym("CHEFZ_QT_T3");
        ChefZ_Sym x1 = Sym("CHEFZ_QT_X1");

        if (mgr.CompareTiers(t0, t1) != -1)                         return false;
        if (mgr.CompareTiers(t3, t1) != 1)                          return false;
        if (mgr.CompareTiers(t1, t1) != 0)                          return false;
        if (mgr.CompareTiers(t3, x1) != 0)                          return false;   // andere Leiter

        // 12 E8: degrade verschiebt, blockiert nicht - und klemmt unten.
        if (mgr.DegradeTier(t3, 1) != Sym("CHEFZ_QT_T2"))           return false;
        if (mgr.DegradeTier(t3, 2) != t1)                           return false;
        if (mgr.DegradeTier(t3, 99) != t0)                          return false;
        if (mgr.DegradeTier(t0, 1) != t0)                           return false;
        if (mgr.DegradeTier(t3, 0) != t3)                           return false;

        // ShiftRank: kaufmaennisch gerundet, oben wie unten geklemmt.
        if (mgr.ShiftRank(t1, 1.0)  != Sym("CHEFZ_QT_T2"))          return false;
        if (mgr.ShiftRank(t1, 0.5)  != Sym("CHEFZ_QT_T2"))          return false;
        if (mgr.ShiftRank(t1, 0.4)  != t1)                          return false;
        if (mgr.ShiftRank(t1, -1.0) != t0)                          return false;
        if (mgr.ShiftRank(t1, 99.0) != t3)                          return false;

        // Eine unbekannte Stufe bleibt, wie sie ist.
        ChefZ_Sym fremd = ChefZ_SymbolTable.Intern("CHEFZ_QT_UNBEKANNT");
        if (mgr.ShiftRank(fremd, 2.0) != fremd)                     return false;
        if (mgr.DegradeTier(fremd, 1) != fremd)                     return false;

        return true;
    }

    //==========================================================================
    // 4. Raenge zusammenfassen (12 §6, Transform)
    //==========================================================================

    private static bool CombineCheck()
    {
        ChefZ_QualityManager mgr = NewManager();
        mgr.Build(StandardLadder(), null, null, null);

        ChefZ_Sym setSym = Sym("CHEFZ_QT_SET");
        ChefZ_Sym t0  = Sym("CHEFZ_QT_T0");
        ChefZ_Sym t1  = Sym("CHEFZ_QT_T1");
        ChefZ_Sym t3  = Sym("CHEFZ_QT_T3");

        array<ref ChefZ_ItemFacts> inputs = new array<ref ChefZ_ItemFacts>();

        ChefZ_ItemFacts a = new ChefZ_ItemFacts();
        a.chefzQuality = t0;                    // Rang 0
        a.units        = 1.0;
        inputs.Insert(a);

        ChefZ_ItemFacts b = new ChefZ_ItemFacts();
        b.chefzQuality = t3;                    // Rang 3
        b.units        = 3.0;
        inputs.Insert(b);

        if (mgr.CombineRanks(inputs, "MIN", setSym) != t0)             return false;
        if (mgr.CombineRanks(inputs, "MAX", setSym) != t3)             return false;

        // MEAN: (0 + 3) / 2 = 1.5 -> kaufmaennisch 2
        if (mgr.CombineRanks(inputs, "MEAN", setSym) != Sym("CHEFZ_QT_T2"))    return false;

        // WEIGHTED_MEAN: (0*1 + 3*3) / 4 = 2.25 -> 2
        if (mgr.CombineRanks(inputs, "WEIGHTED_MEAN", setSym) != Sym("CHEFZ_QT_T2"))
            return false;

        // Unbekannte Regel gilt als MIN - die vorsichtigste Antwort.
        if (mgr.CombineRanks(inputs, "UNFUG", setSym) != t0)           return false;

        // Ohne verwertbare Eingaben gibt es keine Stufe.
        array<ref ChefZ_ItemFacts> leer = new array<ref ChefZ_ItemFacts>();
        if (ChefZ_SymbolTable.IsValid(mgr.CombineRanks(leer, "MIN", setSym)))  return false;

        ChefZ_ItemFacts ohne = new ChefZ_ItemFacts();
        leer.Insert(ohne);                      // Zutat ohne Qualitaetsstufe
        if (ChefZ_SymbolTable.IsValid(mgr.CombineRanks(leer, "MIN", setSym)))  return false;

        return true;
    }

    //==========================================================================
    // 5. Was eine Stufe bewirkt (12 E3) - und was der Rueckfall liefert
    //==========================================================================

    private static bool EffectCheck()
    {
        ChefZ_Registry<ChefZ_QualityTierDef> reg = NewRegistry();

        ChefZ_QualityTierDef top = AddTier(reg, "CHEFZ_QT_E_TOP", "CHEFZ_QT_ESET", 1, 5.0);
        if (!top)                                                   return false;
        top.yieldMultiplier    = 1.5;
        top.portionBonus       = 2;
        top.spoilageMultiplier = 0.85;
        // Bewusst OHNE "#STR_"-Praefix: der Core reicht den Schluessel nur
        // durch und zeigt nichts an. Ein echter Stringtable-Schluessel im
        // Testcode waere ein Eintrag, den niemand uebersetzt - und den der
        // Validator zu Recht als fehlend meldet.
        top.displayName        = "CHEFZ_QT_TESTSTUFE";
        top.grantsEffects      = new array<string>();
        top.grantsEffects.Insert("CHEFZ_QT_EFFEKT_A");
        top.grantsEffects.Insert("CHEFZ_QT_EFFEKT_A");   // Dublette
        top.grantsTags         = new array<string>();
        top.grantsTags.Insert("CHEFZ_QT_TAG_BEKANNT");
        top.grantsTags.Insert("CHEFZ_QT_TAG_UNBEKANNT");

        AddTier(reg, "CHEFZ_QT_E_LOW", "CHEFZ_QT_ESET", 0, 0.0);

        array<string> knownTags = new array<string>();
        knownTags.Insert("CHEFZ_QT_TAG_BEKANNT");

        ChefZ_QualityManager mgr = new ChefZ_QualityManager();
        mgr.SetQuietForTest(true);
        mgr.SetCategoryManagerForTest(NewTagOwner(knownTags));
        mgr.Build(reg, null, null, null);

        ChefZ_Sym sym = Sym("CHEFZ_QT_E_TOP");

        if (!Near(mgr.GetYieldMultiplier(sym), 1.5))                return false;
        if (mgr.GetPortionBonus(sym) != 2)                          return false;
        if (!Near(mgr.GetSpoilageMultiplier(sym), 0.85))            return false;
        if (mgr.GetDisplayKey(sym) != "CHEFZ_QT_TESTSTUFE")         return false;

        // 04 §6: ein unbekannter Tag faellt weg, der bekannte bleibt.
        array<ChefZ_Sym> tags;
        mgr.GetGrantedTags(sym, tags);
        if (tags.Count() != 1)                                      return false;
        if (tags.Get(0) != Sym("CHEFZ_QT_TAG_BEKANNT"))             return false;

        // Effekt-IDs sind opaque, aber duplikatfrei.
        array<string> effects;
        mgr.GetGrantedEffects(sym, effects);
        if (effects.Count() != 1)                                   return false;
        if (effects.Get(0) != "CHEFZ_QT_EFFEKT_A")                  return false;

        // 12 §8: eine unbekannte Stufe ist in jeder Hinsicht neutral.
        ChefZ_Sym fremd = ChefZ_SymbolTable.Intern("CHEFZ_QT_E_GIBTSNICHT");
        if (mgr.GetDef(fremd))                                      return false;
        if (!Near(mgr.GetYieldMultiplier(fremd), 1.0))              return false;
        if (mgr.GetPortionBonus(fremd) != 0)                        return false;
        if (!Near(mgr.GetSpoilageMultiplier(fremd), 1.0))           return false;

        array<ChefZ_Sym> noTags;
        mgr.GetGrantedTags(fremd, noTags);
        if (noTags.Count() != 0)                                    return false;

        return true;
    }

    //==========================================================================
    // 6. Die Punktrechnung (12 §4) - der Kern dieses Tests
    //==========================================================================

    /**
     * Ein Beispiel, in dem JEDER Summand einen anderen Wert hat:
     *
     *   Slotpunkte              +2.0    Slot "a" ist belegt und gibt 2
     *   Regelpunkte              0.0    keine Regeln in dieser Gruppe
     *   Frischeterm             -0.5    min(1.0, 0.25) = 0.25 -> (0.25-0.5)*2*1
     *   Zutatenqualitaetsterm    0.0    keine Zutat traegt eine Stufe
     *   Zustandsstrafen         -3.0    eine Zutat ist im gestraften Zustand
     *   Bias                    +0.5    Rezeptvorgabe
     *   externer Bonus          +1.0    innerhalb der Klemme
     *                          -----
     *   Summe                    0.0
     *   mal Geraetefaktor 1.5    0.0
     *
     * Der Frischeterm ist die eigentliche Zusage: 12 §4.1 verlangt das
     * MINIMUM. Waere hier der Mittelwert gerechnet, stuende +0.125 statt -0.5,
     * und die Summe waere 0.625 statt 0.
     */
    private static bool ScoreCheck()
    {
        ChefZ_QualityManager mgr = NewManager();

        ChefZ_CoreSettingsDef settings = new ChefZ_CoreSettingsDef();
        settings.ResolveDefaults();
        settings.qualityScoring = new ChefZ_QualityScoringDef();
        settings.qualityScoring.freshnessWeight = 1.0;
        settings.qualityScoring.statePenalties  = new array<ref ChefZ_StatePenaltyDef>();

        ChefZ_StatePenaltyDef pen = new ChefZ_StatePenaltyDef();
        pen.state  = "CHEFZ_QT_VERBRANNT";
        pen.points = -3.0;
        settings.qualityScoring.statePenalties.Insert(pen);

        mgr.Build(StandardLadder(), null, settings, null);

        ChefZ_CompiledRecipe recipe = new ChefZ_CompiledRecipe();
        recipe.id             = "CHEFZ_QT_REZEPT";
        recipe.recipeSym      = ChefZ_SymbolTable.Intern("CHEFZ_QT_REZEPT");
        recipe.qualityTierSet = Sym("CHEFZ_QT_SET");
        recipe.qualityBias    = 0.5;

        ChefZ_CompiledSlot slotA = new ChefZ_CompiledSlot();
        slotA.slotIndex   = 0;
        slotA.slotId      = "a";
        slotA.gradePoints = 2;
        recipe.slots.Insert(slotA);

        ChefZ_CompiledSlot slotB = new ChefZ_CompiledSlot();
        slotB.slotIndex   = 1;
        slotB.slotId      = "b";
        slotB.gradePoints = 5;                  // NICHT belegt -> zaehlt nicht
        recipe.slots.Insert(slotB);

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();

        ChefZ_ItemFacts f0 = snap.Acquire();
        f0.handle      = 0;
        f0.classSym    = ChefZ_SymbolTable.Intern("CHEFZ_QT_ZUTAT_A");
        f0.freshness01 = 1.0;

        ChefZ_ItemFacts f1 = snap.Acquire();
        f1.handle      = 1;
        f1.classSym    = ChefZ_SymbolTable.Intern("CHEFZ_QT_ZUTAT_B");
        f1.freshness01 = 0.25;
        f1.chefzState  = ChefZ_SymbolTable.Intern("CHEFZ_QT_VERBRANNT");

        ChefZ_MatchResult match = new ChefZ_MatchResult();
        match.matched        = true;
        match.recipe         = recipe;
        match.recipeSym      = recipe.recipeSym;
        match.boundItemCount = 2;
        match.itemsInVessel  = 2;
        match.boundHandles.Insert(0);
        match.boundHandles.Insert(1);

        array<int> handlesA = new array<int>();
        handlesA.Insert(0);
        match.SetAssignment("a", handlesA);

        ChefZ_CookContext ctx = new ChefZ_CookContext();
        ctx.qualityModifier = 1.5;

        ChefZ_QualityEvaluation eval;
        mgr.ComputeScore(recipe, match, snap, ctx, 1.0, eval);

        if (!eval)                                                  return false;
        if (!Near(eval.SlotPoints, 2.0))                            return false;
        if (!Near(eval.RulePoints, 0.0))                            return false;
        if (!Near(eval.FreshnessTerm, -0.5))                        return false;
        if (!Near(eval.IngredientQualityTerm, 0.0))                 return false;
        if (!Near(eval.StatePenalty, -3.0))                         return false;
        if (!Near(eval.Bias, 0.5))                                  return false;
        if (!Near(eval.ExternalBonus, 1.0))                         return false;
        if (!Near(eval.DeviceModifier, 1.5))                        return false;
        if (!Near(eval.MinFreshness, 0.25))                         return false;
        if (eval.ConsideredItems != 2)                              return false;
        if (!Near(eval.AdditiveSum(), 0.0))                         return false;
        if (!Near(eval.TotalScore, 0.0))                            return false;
        if (eval.Notes.Count() == 0)                                return false;

        // 0.0 Punkte -> die Stufe mit minScore 0.
        if (mgr.ResolveTier(eval.TotalScore, recipe.qualityTierSet) != Sym("CHEFZ_QT_T1"))
            return false;

        // 12 §8: der externe Bonus wird geklemmt - Vorgabe ist 2.0.
        ChefZ_QualityEvaluation big;
        mgr.ComputeScore(recipe, match, snap, ctx, 999.0, big);
        if (!Near(big.ExternalBonus, 2.0))                          return false;

        ChefZ_QualityEvaluation small;
        mgr.ComputeScore(recipe, match, snap, ctx, -999.0, small);
        if (!Near(small.ExternalBonus, -2.0))                       return false;

        // EvaluateResult rechnet und loest in einem Schritt auf.
        ChefZ_QualityEvaluation both;
        ChefZ_Sym tier = mgr.EvaluateResult(recipe, match, snap, ctx, 1.0, both);
        if (tier != Sym("CHEFZ_QT_T1"))                             return false;
        if (both.ResultTier != tier)                                return false;

        // 12 §4.1 ausdruecklich: eine einzige schlechte Zutat drueckt das
        // Gericht. Wird sie besser, steigt der Frischeterm - und nur dann.
        f1.freshness01 = 1.0;
        ChefZ_QualityEvaluation fresh;
        mgr.ComputeScore(recipe, match, snap, ctx, 1.0, fresh);
        if (!Near(fresh.FreshnessTerm, 1.0))                        return false;
        if (!Near(fresh.MinFreshness, 1.0))                         return false;

        return true;
    }

    //==========================================================================
    // 7. Die Qualitaetsregeln (12 §3, 12 §8)
    //==========================================================================

    private static bool RuleCheck()
    {
        ChefZ_QualityManager mgr = NewManager();
        mgr.Build(StandardLadder(), null, null, null);

        ChefZ_CompiledRecipe recipe = new ChefZ_CompiledRecipe();
        recipe.id             = "CHEFZ_QT_REGELREZEPT";
        recipe.recipeSym      = ChefZ_SymbolTable.Intern("CHEFZ_QT_REGELREZEPT");
        recipe.qualityTierSet = Sym("CHEFZ_QT_SET");

        ChefZ_CompiledSlot slotA = new ChefZ_CompiledSlot();
        slotA.slotIndex = 0;
        slotA.slotId    = "a";
        recipe.slots.Insert(slotA);

        ChefZ_CompiledSlot slotB = new ChefZ_CompiledSlot();
        slotB.slotIndex = 1;
        slotB.slotId    = "b";
        recipe.slots.Insert(slotB);

        // --- gueltige Regeln ---------------------------------------------
        recipe.gradeRules.Insert(RuleSlotFilled("hatA", "a", 1.0));
        recipe.gradeRules.Insert(RuleSlotCount("zaehltB", "b", 1.0, 2.0));
        recipe.gradeRules.Insert(RuleAnyItem("hatKlasseX", "CHEFZ_QT_ZUTAT_X", 2.0));
        recipe.gradeRules.Insert(RuleAllMatched("alleX", "CHEFZ_QT_ZUTAT_X", 1.0));
        recipe.gradeRules.Insert(RuleContext("heiss", "deviceTemperature", 100.0, 1.0));
        recipe.gradeRules.Insert(RuleCapability("skill", "CHEFZ_QT_CAP", 2.0, 1.0));

        // --- ungueltige Regeln, jede aus einem anderen Grund (12 §8) ------
        recipe.gradeRules.Insert(RuleSlotFilled("falscherSlot", "gibtsNicht", 1.0));
        recipe.gradeRules.Insert(RuleContext("falscherKontext", "gibtsNicht", 1.0, 1.0));

        ChefZ_GradeRule badWhen = new ChefZ_GradeRule();
        badWhen.ruleId = "falscheArt";
        badWhen.when   = "sobaldEsRegnet";
        recipe.gradeRules.Insert(badWhen);

        ChefZ_GradeRule noSelector = new ChefZ_GradeRule();
        noSelector.ruleId = "ohneSelektor";
        noSelector.when   = "anyItem";
        noSelector.points = 1.0;
        recipe.gradeRules.Insert(noSelector);

        ChefZ_CompileContext ctx = new ChefZ_CompileContext();
        ctx.Init(null);
        mgr.BuildRulesForRecipe(recipe, ctx, null);

        if (mgr.GetRuleCount() != 6)                                return false;
        if (mgr.GetRejectedRuleCount() != 4)                        return false;

        array<ref ChefZ_CompiledGradeRule> rules = mgr.GetRules(recipe.recipeSym);
        if (!rules || rules.Count() != 6)                            return false;

        // --- Auswertung ---------------------------------------------------
        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();

        ChefZ_ItemFacts f0 = snap.Acquire();
        f0.handle   = 0;
        f0.classSym = ChefZ_SymbolTable.Intern("CHEFZ_QT_ZUTAT_X");

        ChefZ_ItemFacts f1 = snap.Acquire();
        f1.handle   = 1;
        f1.classSym = ChefZ_SymbolTable.Intern("CHEFZ_QT_ZUTAT_Y");

        ChefZ_ItemFacts f2 = snap.Acquire();
        f2.handle   = 2;
        f2.classSym = ChefZ_SymbolTable.Intern("CHEFZ_QT_ZUTAT_Y");

        ChefZ_ItemFacts f3 = snap.Acquire();
        f3.handle   = 3;
        f3.classSym = ChefZ_SymbolTable.Intern("CHEFZ_QT_ZUTAT_Y");

        ChefZ_MatchResult match = new ChefZ_MatchResult();
        match.matched        = true;
        match.recipe         = recipe;
        match.recipeSym      = recipe.recipeSym;
        match.boundItemCount = 4;
        match.itemsInVessel  = 4;
        for (int h = 0; h < 4; h++)
            match.boundHandles.Insert(h);

        array<int> handlesA = new array<int>();
        handlesA.Insert(0);
        match.SetAssignment("a", handlesA);

        array<int> handlesB = new array<int>();
        handlesB.Insert(1);
        handlesB.Insert(2);
        handlesB.Insert(3);
        match.SetAssignment("b", handlesB);

        ChefZ_CookContext ctxRun = new ChefZ_CookContext();
        ctxRun.deviceTemperature = 150.0;

        // Ohne Faehigkeitsanbieter (12 §8): die skill-Regel gibt 0 Punkte.
        //   hatA        +1
        //   zaehltB     +2   (3 Zutaten x 1, gedeckelt auf 2)
        //   hatKlasseX  +2
        //   alleX        0   (nur eine von vier erfuellt)
        //   heiss       +1
        //   skill        0
        ChefZ_QualityEvaluation eval;
        mgr.ComputeScore(recipe, match, snap, ctxRun, 0.0, eval);
        if (!Near(eval.RulePoints, 6.0))                            return false;

        // Mit Anbieter zaehlt die skill-Regel: Wert 3 liegt in [2, *].
        ChefZ_TestCapabilityProbe probe = new ChefZ_TestCapabilityProbe();
        probe.Init("CHEFZ_QT_CAP", 3.0);
        mgr.SetCapabilityProbe(probe);
        if (!mgr.HasCapabilityProbe())                              return false;

        ChefZ_QualityEvaluation withSkill;
        mgr.ComputeScore(recipe, match, snap, ctxRun, 0.0, withSkill);
        if (!Near(withSkill.RulePoints, 7.0))                       return false;

        // Ein Anbieter, der die Faehigkeit nicht kennt, gibt keine Punkte.
        ChefZ_TestCapabilityProbe other = new ChefZ_TestCapabilityProbe();
        other.Init("CHEFZ_QT_ANDERE_CAP", 99.0);
        mgr.SetCapabilityProbe(other);

        ChefZ_QualityEvaluation noSkill;
        mgr.ComputeScore(recipe, match, snap, ctxRun, 0.0, noSkill);
        if (!Near(noSkill.RulePoints, 6.0))                         return false;

        // Zu kaltes Geraet: die Kontextregel faellt weg.
        ctxRun.deviceTemperature = 20.0;
        ChefZ_QualityEvaluation cold;
        mgr.ComputeScore(recipe, match, snap, ctxRun, 0.0, cold);
        if (!Near(cold.RulePoints, 5.0))                            return false;

        return true;
    }

    //--------------------------------------------------------------------------
    // Regelbaukasten fuer den Test
    //--------------------------------------------------------------------------

    private static ChefZ_GradeRule RuleSlotFilled(string id, string slotId, float points)
    {
        ChefZ_GradeRule r = new ChefZ_GradeRule();
        r.ruleId = id;
        r.when   = "slotFilled";
        r.slotId = slotId;
        r.points = points;
        return r;
    }

    private static ChefZ_GradeRule RuleSlotCount(string id, string slotId, float perItem, float maxPoints)
    {
        ChefZ_GradeRule r = new ChefZ_GradeRule();
        r.ruleId        = id;
        r.when          = "slotCount";
        r.slotId        = slotId;
        r.pointsPerItem = perItem;
        r.maxPoints     = maxPoints;
        return r;
    }

    private static ChefZ_GradeRule RuleAnyItem(string id, string cls, float points)
    {
        ChefZ_GradeRule r = new ChefZ_GradeRule();
        r.ruleId       = id;
        r.when         = "anyItem";
        r.points       = points;
        r.selector     = new ChefZ_Selector();
        r.selector.cls = cls;
        return r;
    }

    private static ChefZ_GradeRule RuleAllMatched(string id, string cls, float points)
    {
        ChefZ_GradeRule r = new ChefZ_GradeRule();
        r.ruleId       = id;
        r.when         = "allMatched";
        r.points       = points;
        r.selector     = new ChefZ_Selector();
        r.selector.cls = cls;
        return r;
    }

    private static ChefZ_GradeRule RuleContext(string id, string key, float min, float points)
    {
        ChefZ_GradeRule r = new ChefZ_GradeRule();
        r.ruleId     = id;
        r.when       = "context";
        r.contextKey = key;
        r.points     = points;
        r.range      = new ChefZ_Range();
        r.range.min  = min;
        return r;
    }

    private static ChefZ_GradeRule RuleCapability(string id, string capability, float min, float points)
    {
        ChefZ_GradeRule r = new ChefZ_GradeRule();
        r.ruleId     = id;
        r.when       = "capability";
        r.capability = capability;
        r.points     = points;
        r.range      = new ChefZ_Range();
        r.range.min  = min;
        return r;
    }

    //==========================================================================
    // 8. Leere Registry (12 §8, erste Zeile)
    //==========================================================================

    private static bool EmptyCheck()
    {
        ChefZ_QualityManager mgr = NewManager();
        mgr.Build(null, null, null, null);

        // "bereit und leer", nicht "nicht gebaut".
        if (!mgr.IsReady())                                         return false;
        if (mgr.GetTierCount() != 0)                                return false;

        ChefZ_Sym setSym = ChefZ_SymbolTable.Intern("CHEFZ_QT_LEERERSATZ");
        if (ChefZ_SymbolTable.IsValid(mgr.ResolveTier(5.0, setSym)))   return false;
        if (ChefZ_SymbolTable.IsValid(mgr.GetLowestTier(setSym)))      return false;
        if (mgr.TierSetExists(setSym))                                 return false;

        // Alle Wirkungen neutral - Gerichte entstehen weiterhin, nur ohne
        // Qualitaet.
        ChefZ_Sym any = ChefZ_SymbolTable.Intern("CHEFZ_QT_IRGENDEINE");
        if (!Near(mgr.GetYieldMultiplier(any), 1.0))                return false;
        if (mgr.GetPortionBonus(any) != 0)                          return false;
        if (!Near(mgr.GetSpoilageMultiplier(any), 1.0))             return false;
        if (mgr.CompareTiers(any, setSym) != 0)                        return false;

        return true;
    }

    //==========================================================================
    // 9. Abfrage vor Build (12 §8 sinngemaess)
    //==========================================================================

    private static bool NotReadyCheck()
    {
        ChefZ_QualityManager mgr = new ChefZ_QualityManager();
        mgr.SetQuietForTest(true);

        if (mgr.IsReady())                                          return false;

        ChefZ_Sym setSym = ChefZ_SymbolTable.Intern("CHEFZ_QT_VORBUILD");
        if (ChefZ_SymbolTable.IsValid(mgr.ResolveTier(1.0, setSym)))   return false;
        if (mgr.GetDef(setSym))                                        return false;
        if (mgr.TierSetExists(setSym))                                 return false;
        if (mgr.GetRank(setSym) != -1)                                 return false;

        // Kein Absturz, neutrale Antworten.
        if (!Near(mgr.GetYieldMultiplier(setSym), 1.0))                return false;
        if (mgr.GetPortionBonus(setSym) != 0)                          return false;

        array<ChefZ_Sym> tags;
        mgr.GetGrantedTags(setSym, tags);
        if (tags.Count() != 0)                                      return false;

        return true;
    }
}
