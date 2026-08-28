//==============================================================================
// ChefZ_QualityManager - Stufenregistry und Punktrechnung zur Laufzeit
//
// Entwurf: 12 §5 (Schnittstelle woertlich), 12 §4 (die Punktrechnung),
// 12 §4.1 (Frische geht als MINIMUM ein), 12 §4.2 (Zustandsstrafen sind
// additiv und duerfen unter die unterste Stufe druecken), 12 §6 (Datenfluss),
// 12 §7 (was Zustand ist und was nicht), 12 §8 (Fehlerverhalten),
// 12 E1/E3/E4/E5/E7/E8.
//
// ---------------------------------------------------------------------------
// Was dieser Manager NICHT tut
// ---------------------------------------------------------------------------
// Er fasst kein Item an. ComputeScore ist rein rechnend und bekommt seine
// Eingaben als Daten: das kompilierte Rezept, das Matchergebnis, die
// Faktenliste und den Kochkontext. Das ist derselbe Schnitt wie beim Matcher
// (05 E1) und aus demselben Grund - er ist damit ohne laufendes Spiel
// pruefbar, und er KANN am Gericht nichts veraendern.
//
// Wer die Stufe an ein Item schreibt, ist der ChefZ_Applicator (4_World).
//
// ---------------------------------------------------------------------------
// Die eine Grenze, die dieser Manager nicht ueberschreiten kann (12 §2)
// ---------------------------------------------------------------------------
// Qualitaet veraendert den Naehrwert pro Bissen NICHT. PlayerStomach.
// AddToStomach holt das NutritionalProfile ueber Klasse x Foodstage aus
// CfgVehicles und uebergibt item = null (Befund 01 V6). Der Ersatz ist die
// Ausbeute - yieldMultiplier und portionBonus. Deshalb gibt es hier kein
// GetNutritionMultiplier(), und es soll auch keines geben: eine Methode, die
// nichts bewirken kann, ist ein Versprechen, das der Core nicht halten kann.
//
// ---------------------------------------------------------------------------
// Zwei Bauschritte, und warum sie getrennt sind
// ---------------------------------------------------------------------------
//   Build()              die Stufenleitern. Braucht die Stufenregistry und
//                        laeuft VOR dem Selektorkontext, weil der
//                        ChefZ_ManagerSymbolResolver die Raenge von hier holt.
//   BuildRecipeRules()   die Qualitaetsregeln der Rezepte. Braucht die
//                        KOMPILIERTEN Rezepte und laeuft deshalb nach der
//                        Recipe Engine.
//
// Der Rezeptcompiler (S6) sagt das ausdruecklich voraus: "Die Regeln werden
// hier NICHT uebersetzt und nicht geprueft. Sie gehoeren dem Quality Manager,
// und der kompiliert ihre Selektoren, wenn er gebaut wird."
//
// KEIN CONTENT: dieser Manager definiert keine einzige Stufe. Alles kommt aus
// der Registry.
//
// Layer: 3_Game. Er liest Registries und kennt keinen Engine-Typ - kein
// ItemBase, kein FoodStage, kein Enum aus 4_World.
//==============================================================================

class ChefZ_QualityManager
{
    private static ref ChefZ_QualityManager s_Instance;

    //--- Bestand, indiziert ueber ChefZ_Sym ----------------------------------
    private ref map<int, ChefZ_QualityTierDef>  m_BySym;      // KEIN ref-Wert:
                                                              // Eigentuemer ist
                                                              // die Registry.
    private ref map<int, int>                   m_SetOf;      // Stufe  -> Satz
    private ref map<int, int>                   m_RankOf;     // Stufe  -> Rang
    private ref map<int, ref array<ChefZ_Sym>>  m_Sets;       // Satz   -> Stufen
    private ref map<int, ref array<ChefZ_Sym>>  m_TagsOf;     // Stufe  -> Tags
    private ref array<ChefZ_Sym>                m_SetOrder;   // stabile Folge
    private ref array<ChefZ_Sym>                m_Order;      // stabile Folge

    //! Die Qualitaetsregeln je Rezept, uebersetzt. Schluessel ist
    //! ChefZ_CompiledRecipe.recipeSym.
    private ref map<int, ref array<ref ChefZ_CompiledGradeRule>> m_RulesByRecipe;

    //--- Einstellungen -------------------------------------------------------
    private ref ChefZ_QualityScoring m_Scoring;
    private float m_MaxExternalBonus;
    private ChefZ_Sym m_DefaultSetSym;

    //! Die Ordinaltabelle des Config Managers. Bewusst OHNE ref: sie gehoert
    //! ihm, ihre Lebensdauer ist laenger als die dieses Managers, und ein
    //! zweiter starker Verweis waere ein Zyklus ohne Gewinn.
    private ChefZ_IdentityMap m_Identities;

    //! Auskunftsstelle fuer Faehigkeitswerte (12 E6). null ist der Normalfall
    //! auf einem Server ohne Skillmodul und ausdruecklich KEIN Fehler.
    private ref ChefZ_CapabilityProbe m_Capabilities;

    private ref ChefZ_QualityTierDef m_Fallback;

    private bool m_Ready;
    private bool m_NotReadyLogged;
    private bool m_QuietForTest;
    private int  m_RuleCount;
    private int  m_RejectedRules;

    //! Kategoriebaum des Tests statt des Singletons - dieselbe Loesung wie im
    //! ChefZ_StateManager und aus demselben Grund: ohne ihn muesste der
    //! Selbsttest den Singleton umbauen und damit den echten Bestand des
    //! Servers anfassen.
    private ChefZ_CategoryManager m_CategoriesForTest;

    //--------------------------------------------------------------------------

    void ChefZ_QualityManager()
    {
        m_BySym         = new map<int, ChefZ_QualityTierDef>();
        m_SetOf         = new map<int, int>();
        m_RankOf        = new map<int, int>();
        m_Sets          = new map<int, ref array<ChefZ_Sym>>();
        m_TagsOf        = new map<int, ref array<ChefZ_Sym>>();
        m_SetOrder      = new array<ChefZ_Sym>();
        m_Order         = new array<ChefZ_Sym>();
        m_RulesByRecipe = new map<int, ref array<ref ChefZ_CompiledGradeRule>>();
        m_QuietForTest  = false;

        BuildFallback();
        ResetState();
    }

    static ChefZ_QualityManager Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_QualityManager();
        return s_Instance;
    }

    /**
     * Der Rueckfall-Datensatz (12 §8, erste Zeile: "Alle Multiplikatoren
     * gelten als 1.0, portionBonus als 0").
     *
     * Er ist KEIN Content: er hat keine ID, keinen Stufensatz, keine Tags und
     * wird nie in die Registry aufgenommen. Er existiert allein, damit ein
     * Leser im heissen Pfad bedingungslos auf Felder zugreifen kann, statt
     * jede Zeile mit einer Nullpruefung zu umstellen.
     */
    private void BuildFallback()
    {
        m_Fallback = new ChefZ_QualityTierDef();
        m_Fallback.id        = "";
        m_Fallback.sourceRef = "ChefZ_QualityManager (Rueckfall)";
        m_Fallback.ResolveDefaults();
    }

    //==========================================================================
    // Aufbau, Schritt 1: die Stufenleitern (12 §6, BOOT)
    //==========================================================================

    /**
     * Baut den Bestand. Einmal beim Boot, danach unveraenderlich.
     *
     * Zwei Abweichungen von der Signatur in 12 §5, beide aus demselben Grund
     * wie beim ChefZ_StateManager: die Einstellungen und die Ordinaltabelle
     * kommen HEREIN, statt hier zweitberechnet zu werden. Der Gewichtssatz der
     * Punktrechnung wird dabei genau EINMAL im ganzen Boot aufgeloest - ein
     * zweiter Aufruf wuerde jede Warnung doppelt in den Ladebericht schreiben.
     *
     * Beide Parameter duerfen null sein:
     *   settings   fehlt   -> Code-Defaults der Punktrechnung
     *   identities fehlt   -> keine Sync-Ordinale; Stufen sind serverseitig
     *                         voll benutzbar, clientseitig aber nicht
     *                         darstellbar (03 §7).
     *
     * Der Aufruf ist beim Boot UNBEDINGT: auch ohne eine einzige Stufe soll
     * der Manager "bereit und leer" sein (12 §8, erste Zeile). Sonst
     * antwortete er auf jede Abfrage mit dem Fehler "vor Build aufgerufen",
     * obwohl schlicht keine Stufen konfiguriert sind.
     */
    void Build(ChefZ_Registry<ChefZ_QualityTierDef> defs,
               ChefZ_LoadReport report,
               ChefZ_CoreSettingsDef settings = null,
               ChefZ_IdentityMap identities = null)
    {
        ResetState();
        m_Identities = identities;

        if (settings)
        {
            m_Scoring          = settings.BuildQualityScoring(report);
            m_MaxExternalBonus = settings.maxExternalQualityBonus;
        }
        else
        {
            m_Scoring          = new ChefZ_QualityScoring();
            m_MaxExternalBonus = 2.0;
        }

        if (m_MaxExternalBonus < 0.0)
            m_MaxExternalBonus = 0.0;

        m_DefaultSetSym = ChefZ_SymbolTable.Intern(m_Scoring.defaultTierSet);

        if (!defs || defs.Count() == 0)
        {
            // 12 §8, erste Zeile: kein Abbruch. ResolveTier liefert INVALID,
            // alle Multiplikatoren gelten als 1.0, Gerichte entstehen
            // weiterhin - nur ohne Qualitaet. Vanilla unberuehrt.
            if (report)
                report.AddInfo("Keine Qualitaetsstufen definiert - Gerichte entstehen ohne "
                    + "Stufe. Ausbeute, Haltbarkeit und Anzeige bleiben neutral; "
                    + "Vanilla-Kochen ist davon unberuehrt.");
            m_Ready = true;
            return;
        }

        // Reihenfolge ist Registry.Keys(), also nach ID sortiert (03 §4).
        // Damit ist jede abgeleitete Groesse auf Client und Server gleich.
        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_Sym sym = keys.Get(i);
            ChefZ_QualityTierDef def = defs.Find(sym);
            if (!def)
                continue;

            m_BySym.Set(sym, def);
            m_Order.Insert(sym);

            RegisterInSet(def);
            ResolveGrantedTags(def, report);
        }

        OrderAllSets(report);

        m_Ready = true;

        if (report)
        {
            report.AddInfo("Qualitaetsstufen: " + GetTierCount().ToString() + " in "
                + m_SetOrder.Count().ToString() + " Stufensatz/-saetzen, Vorgabesatz \""
                + m_Scoring.defaultTierSet + "\".");
        }

        LogIfDebug();
    }

    //! Stufe ihrem Satz zuordnen. Ohne eigenen Satz gilt der Vorgabesatz
    //! (12 E4) - die Zuordnung faellt hier und nicht im Record, weil der
    //! Vorgabename eine Einstellung ist und kein Code-Wissen.
    private void RegisterInSet(notnull ChefZ_QualityTierDef def)
    {
        ChefZ_Sym setSym = def.tierSetSym;
        if (!ChefZ_SymbolTable.IsValid(setSym))
            setSym = m_DefaultSetSym;

        m_SetOf.Set(def.sym, setSym);

        array<ChefZ_Sym> tiers;
        if (!m_Sets.Find(setSym, tiers))
        {
            tiers = new array<ChefZ_Sym>();
            m_Sets.Set(setSym, tiers);
            m_SetOrder.Insert(setSym);
        }
        tiers.Insert(def.sym);
    }

    /**
     * grantsTags gegen die Tag-Registry pruefen (04 §6).
     *
     * Ein unbekannter Tag faellt weg, die uebrigen bleiben gueltig. Tags
     * werden nie implizit angelegt - ein implizit angelegter Tag matchte nie
     * und waere stiller toter Code. Dieselbe Regel wie fuer
     * ChefZ_StateDef.implies, und aus demselben Grund.
     */
    private void ResolveGrantedTags(notnull ChefZ_QualityTierDef def, ChefZ_LoadReport report)
    {
        array<ChefZ_Sym> tags = new array<ChefZ_Sym>();
        m_TagsOf.Set(def.sym, tags);

        if (!def.grantsTags)
            return;

        ChefZ_CategoryManager cats = Cats();

        for (int i = 0; i < def.grantsTags.Count(); i++)
        {
            string name = def.grantsTags.Get(i);
            if (name == "")
                continue;

            ChefZ_Sym tag = ChefZ_SymbolTable.Lookup(name);
            if (!cats.TagExists(tag))
            {
                Report(report, false, def,
                    "grantsTags nennt \"" + name + "\" - dieser Tag ist unbekannt und wird "
                    + "fuer diese Stufe ausgelassen; die uebrigen bleiben gueltig. Tags "
                    + "werden nie implizit angelegt (04 §6).");
                continue;
            }

            if (tags.Find(tag) < 0)
                tags.Insert(tag);
        }
    }

    private void OrderAllSets(ChefZ_LoadReport report)
    {
        for (int i = 0; i < m_SetOrder.Count(); i++)
            OrderSet(m_SetOrder.Get(i), report);
    }

    /**
     * Eine Stufenleiter in Ordnung bringen (12 §6, BOOT).
     *
     * Sortiert wird nach minScore aufsteigend, bei Gleichstand nach dem
     * geschriebenen rank aufsteigend. Danach ist der WIRKSAME Rang die
     * Position in dieser Liste - genau das meint 12 §8 mit "nach minScore
     * sortiert, rank neu durchnummeriert".
     *
     * Warum nicht nach dem geschriebenen rank sortiert wird: die Schwelle
     * entscheidet, welche Stufe ein Gericht bekommt. Eine Leiter, deren
     * Raenge in einer anderen Reihenfolge stehen als ihre Schwellen, waere ein
     * Loch im Rangraum - DegradeTier(PREMIUM, 1) landete dann irgendwo, nur
     * nicht eine Stufe tiefer. Das ist der stille Rechenfehler, den 12 §8
     * ausdruecklich verhindern will.
     *
     * Der Record selbst wird NICHT umgeschrieben: er gehoert der Registry, und
     * die ist gleich eingefroren. Der wirksame Rang lebt in m_RankOf.
     */
    private void OrderSet(ChefZ_Sym setSym, ChefZ_LoadReport report)
    {
        array<ChefZ_Sym> tiers;
        if (!m_Sets.Find(setSym, tiers) || tiers.Count() == 0)
            return;

        SortTiers(tiers);

        bool reordered = false;
        string before = "";
        string after  = "";

        int i;
        for (i = 0; i < tiers.Count(); i++)
        {
            ChefZ_QualityTierDef def = m_BySym.Get(tiers.Get(i));
            if (!def)
                continue;

            if (def.rank != i)
                reordered = true;

            m_RankOf.Set(def.sym, i);

            if (i > 0)
            {
                before = before + ", ";
                after  = after + ", ";
            }
            before = before + def.id + "(" + def.rank.ToString() + ")";
            after  = after + def.id + "(" + i.ToString() + ")";
        }

        if (reordered)
        {
            ReportSet(report, setSym,
                "Die geschriebenen \"rank\"-Werte stimmen nicht mit der Reihenfolge der "
                + "\"minScore\"-Schwellen ueberein. Es gilt die Reihenfolge der Schwellen; "
                + "die Raenge werden neu durchnummeriert. Vorher: " + before
                + ". Nachher: " + after + ".");
        }

        WarnOnDuplicateScores(tiers, setSym, report);
        WarnOnMissingZero(tiers, setSym, report);
    }

    //! Einfuegesortierung: stabil, ohne Allokation, und die Leitern sind
    //! einstellig lang.
    private void SortTiers(notnull array<ChefZ_Sym> tiers)
    {
        for (int i = 1; i < tiers.Count(); i++)
        {
            ChefZ_Sym key = tiers.Get(i);
            int j = i - 1;
            while (j >= 0 && TierPrecedes(key, tiers.Get(j)))
            {
                tiers.Set(j + 1, tiers.Get(j));
                j--;
            }
            tiers.Set(j + 1, key);
        }
    }

    //! true, wenn a vor b gehoert: kleinere Schwelle zuerst, bei Gleichstand
    //! der kleinere geschriebene Rang. Damit gewinnt bei gleicher Schwelle die
    //! Stufe mit dem HOEHEREN Rang (12 §8) - sie steht hinten, und ResolveTier
    //! nimmt die letzte passende.
    private bool TierPrecedes(ChefZ_Sym a, ChefZ_Sym b)
    {
        ChefZ_QualityTierDef da = m_BySym.Get(a);
        ChefZ_QualityTierDef db = m_BySym.Get(b);
        if (!da || !db)
            return false;

        if (da.minScore != db.minScore)
            return da.minScore < db.minScore;
        if (da.rank != db.rank)
            return da.rank < db.rank;

        // Ueber ChefZ_StringOrder und nicht ueber "<": Enforce sichert keine
        // Ordnung auf string zu, und diese Reihenfolge bestimmt bei zwei
        // gleichwertigen Stufen, welche gewinnt. Sie muss auf Client und
        // Server dieselbe sein (03 §4).
        return ChefZ_StringOrder.Less(da.id, db.id);
    }

    private void WarnOnDuplicateScores(notnull array<ChefZ_Sym> tiers, ChefZ_Sym setSym,
                                       ChefZ_LoadReport report)
    {
        for (int i = 1; i < tiers.Count(); i++)
        {
            ChefZ_QualityTierDef prev = m_BySym.Get(tiers.Get(i - 1));
            ChefZ_QualityTierDef cur  = m_BySym.Get(tiers.Get(i));
            if (!prev || !cur)
                continue;
            if (prev.minScore != cur.minScore)
                continue;

            ReportSet(report, setSym,
                "Die Stufen \"" + prev.id + "\" und \"" + cur.id + "\" haben denselben "
                + "minScore (" + cur.minScore.ToString() + "). Die mit dem hoeheren Rang "
                + "gewinnt (\"" + cur.id + "\"); die andere ist damit unerreichbar.");
        }
    }

    /**
     * 12 §8: "Kein rank 0 mit minScore <= 0 -> implizite Nullstufe wird
     * eingezogen, WARN. Sonst fiele ein Gericht mit 0 Punkten durch das
     * Raster."
     *
     * UMSETZUNG, und sie weicht in der Form ab, nicht in der Wirkung: statt
     * eine Stufe zu ERFINDEN - die haette eine ID, einen Anzeigenamen und
     * einen Sync-Ordinal, den Client und Server unabhaengig ableiten muessten,
     * und waere damit Content aus dem Core - faengt die UNTERSTE vorhandene
     * Stufe eines Satzes ausnahmslos alles ab, was unter ihrer Schwelle liegt
     * (siehe ResolveTier).
     *
     * Das Ergebnis ist dasselbe: kein Gericht faellt durch das Raster. Der
     * Unterschied ist, dass keine Stufe entsteht, die niemand geschrieben hat.
     */
    private void WarnOnMissingZero(notnull array<ChefZ_Sym> tiers, ChefZ_Sym setSym,
                                   ChefZ_LoadReport report)
    {
        ChefZ_QualityTierDef lowest = m_BySym.Get(tiers.Get(0));
        if (!lowest || lowest.minScore <= 0.0)
            return;

        ReportSet(report, setSym,
            "Die unterste Stufe \"" + lowest.id + "\" beginnt erst bei "
            + lowest.minScore.ToString() + " Punkten. Ein Gericht mit weniger Punkten haette "
            + "damit gar keine Stufe. \"" + lowest.id + "\" faengt deshalb auch alles "
            + "darunter ab. Abhilfe: eine unterste Stufe mit minScore <= 0 anlegen.");
    }

    //==========================================================================
    // Aufbau, Schritt 2: die Qualitaetsregeln der Rezepte
    //==========================================================================

    /**
     * Uebersetzt die gradeRules aller kompilierten Rezepte (12 §3, 12 §8).
     *
     * Zwingend NACH ChefZ_RecipeEngine.Build(): die Rohregeln haengen an den
     * kompilierten Rezepten, und die Slot-IDs werden gegen die kompilierten
     * Slots geprueft.
     *
     * Der Aufruf ist unbedingt und darf mit null-Parametern kommen - dann gibt
     * es eben keine Regeln, und jedes Gericht bekommt seine Stufe allein aus
     * Slotpunkten, Frische, Zutatenqualitaet und Zustandsstrafen. Das ist
     * "weniger ChefZ", nicht "falsches ChefZ".
     */
    void BuildRecipeRules(ChefZ_RecipeEngine engine,
                          ChefZ_CompileContext ctx,
                          ChefZ_LoadReport report)
    {
        m_RulesByRecipe.Clear();
        m_RuleCount     = 0;
        m_RejectedRules = 0;

        if (!engine)
            return;

        int count = engine.GetRecipeCount();
        for (int i = 0; i < count; i++)
        {
            ChefZ_CompiledRecipe recipe = engine.GetRecipeAt(i);
            if (!recipe)
                continue;

            BuildRulesForRecipe(recipe, ctx, report);
        }

        if (report && m_RuleCount > 0)
        {
            report.AddInfo("Qualitaetsregeln: " + m_RuleCount.ToString() + " uebersetzt, "
                + m_RejectedRules.ToString() + " verworfen.");
        }
    }

    /**
     * Die Regeln EINES Rezepts uebersetzen.
     *
     * Oeffentlich, weil zwei Aufrufer sie brauchen: die Schleife oben und der
     * Selbsttest, der ein einzelnes handgebautes Rezept prueft, ohne eine
     * ganze Recipe Engine aufzusetzen. Sie setzt die Zaehler NICHT zurueck -
     * das tut BuildRecipeRules bzw. Build.
     */
    void BuildRulesForRecipe(notnull ChefZ_CompiledRecipe recipe,
                             ChefZ_CompileContext ctx,
                             ChefZ_LoadReport report)
    {
        CheckTierSet(recipe, report);
        CompileRulesOf(recipe, ctx, report);
    }

    /**
     * 12 §8: "Rezept nennt unbekanntes qualityTierSet -> Rezept wird NICHT
     * abgewiesen, es faellt auf den Vorgabesatz zurueck, ERROR beim Build."
     *
     * Der Rueckfall geschieht in ResolveTier und nicht hier: das kompilierte
     * Rezept gehoert der Engine, und ein Manager, der fremde Objekte
     * umschreibt, ist die Sorte Kopplung, die spaeter niemand mehr findet.
     */
    private void CheckTierSet(notnull ChefZ_CompiledRecipe recipe, ChefZ_LoadReport report)
    {
        if (!ChefZ_SymbolTable.IsValid(recipe.qualityTierSet))
            return;
        if (m_Sets.Contains(recipe.qualityTierSet))
            return;

        if (report)
        {
            report.AddError(recipe.sourceRef, recipe.id,
                "qualityTierSet \"" + ChefZ_SymbolTable.NameOrMark(recipe.qualityTierSet) + "\" ist kein bekannter Stufensatz. Das Rezept bleibt gueltig und benutzt " + "den Vorgabesatz \"" + Scoring().defaultTierSet + "\" - die Qualitaet ist "
                + "eine Verfeinerung, und ein Rezept deswegen ganz auszuschalten waere "
                + "unverhaeltnismaessig.");
        }
    }

    private void CompileRulesOf(notnull ChefZ_CompiledRecipe recipe,
                                ChefZ_CompileContext ctx,
                                ChefZ_LoadReport report)
    {
        if (!recipe.gradeRules || recipe.gradeRules.Count() == 0)
            return;

        array<ref ChefZ_CompiledGradeRule> compiled = new array<ref ChefZ_CompiledGradeRule>();

        for (int i = 0; i < recipe.gradeRules.Count(); i++)
        {
            ChefZ_GradeRule raw = recipe.gradeRules.Get(i);
            if (!raw)
                continue;

            string why;
            ChefZ_CompiledGradeRule rule = CompileRule(recipe, raw, i, ctx, why);
            if (!rule)
            {
                m_RejectedRules++;
                if (report)
                {
                    report.AddError(recipe.sourceRef, recipe.id,
                        "gradeRules[" + i.ToString() + "]" + RuleIdSuffix(raw) + ": " + why
                        + " Die Regel wird verworfen; die uebrigen Regeln zaehlen weiter.");
                }
                continue;
            }

            compiled.Insert(rule);
            m_RuleCount++;
        }

        if (compiled.Count() > 0)
            m_RulesByRecipe.Set(recipe.recipeSym, compiled);
    }

    private string RuleIdSuffix(notnull ChefZ_GradeRule raw)
    {
        if (raw.ruleId == "")
            return "";
        return " (\"" + raw.ruleId + "\")";
    }

    /**
     * Eine Rohregel uebersetzen. null bei jedem Fehler, why traegt dann die
     * Begruendung im Klartext.
     *
     * Geprueft wird alles, was sich beim Boot pruefen laesst: die Regelart,
     * die Slot-ID gegen die kompilierten Slots, der Selektor gegen den
     * Kompilierkontext, die Kontextkennung gegen die zulaessige Liste. Was
     * hier durchkommt, rechnet zur Laufzeit ohne weitere Pruefung.
     */
    private ChefZ_CompiledGradeRule CompileRule(notnull ChefZ_CompiledRecipe recipe,
                                                notnull ChefZ_GradeRule raw,
                                                int index,
                                                ChefZ_CompileContext ctx,
                                                out string why)
    {
        why = "";

        int when = ChefZ_GradeWhen.FromName(raw.when);
        if (when == ChefZ_GradeWhen.NONE)
        {
            why = "\"when\" ist \"" + raw.when + "\" und damit keine bekannte Regelart. "
                + "Gueltig: " + ChefZ_GradeWhen.ValidNames() + ".";
            return null;
        }

        ChefZ_CompiledGradeRule rule = new ChefZ_CompiledGradeRule();
        rule.when   = when;
        rule.ruleId = raw.ruleId;
        if (rule.ruleId == "")
            rule.ruleId = ChefZ_GradeWhen.Name(when) + "#" + index.ToString();

        rule.points        = ChefZ_Undefined.FloatOr(raw.points, 0.0);
        rule.pointsPerItem = ChefZ_Undefined.FloatOr(raw.pointsPerItem, 0.0);
        rule.hasMaxPoints  = !ChefZ_Undefined.IsFloatUndefined(raw.maxPoints);
        if (rule.hasMaxPoints)
            rule.maxPoints = raw.maxPoints;

        if (raw.range)
        {
            rule.range.min = raw.range.min;
            rule.range.max = raw.range.max;

            if (!rule.range.IsValid())
            {
                why = "\"range\" hat min > max (" + rule.range.ToDebugString() + ") und "
                    + "koennte damit nie erfuellt werden.";
                return null;
            }
        }

        if (ChefZ_GradeWhen.NeedsSlot(when) && !ResolveRuleSlot(recipe, raw, rule, why))
            return null;

        if (ChefZ_GradeWhen.NeedsSelector(when) && !ResolveRuleSelector(raw, rule, ctx, why))
            return null;

        if (when == ChefZ_GradeWhen.CONTEXT && !ResolveRuleContext(raw, rule, why))
            return null;

        if (when == ChefZ_GradeWhen.CAPABILITY)
        {
            if (raw.capability == "")
            {
                why = "eine Regel mit when \"capability\" braucht das Feld \"capability\".";
                return null;
            }
            rule.capability = raw.capability;
        }

        if (rule.points == 0.0 && rule.pointsPerItem == 0.0)
        {
            // Kein Abbruch: eine Regel mit 0 Punkten ist gueltig (sie kann als
            // Platzhalter gemeint sein). Aber sie ist fast immer ein
            // vergessenes Feld, und stillschweigend nichts zu tun ist die
            // schlechteste Eigenschaft einer Regel.
            QuietOnce(ChefZ_LogLevel.WARN, "quality.rule.zero." + recipe.id + "." + rule.ruleId,
                "Rezept " + recipe.id + ", Regel " + rule.ruleId + " vergibt weder \"points\" "
                + "noch \"pointsPerItem\" - sie bleibt wirkungslos.");
        }

        return rule;
    }

    private bool ResolveRuleSlot(notnull ChefZ_CompiledRecipe recipe,
                                 notnull ChefZ_GradeRule raw,
                                 notnull ChefZ_CompiledGradeRule rule,
                                 out string why)
    {
        why = "";

        if (raw.slotId == "")
        {
            why = "eine Regel mit when \"" + ChefZ_GradeWhen.Name(rule.when)
                + "\" braucht das Feld \"slotId\".";
            return false;
        }

        for (int i = 0; i < recipe.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = recipe.slots.Get(i);
            if (slot && slot.slotId == raw.slotId)
            {
                rule.slotId = raw.slotId;
                return true;
            }
        }

        why = "slotId \"" + raw.slotId + "\" gibt es in diesem Rezept nicht.";
        return false;
    }

    private bool ResolveRuleSelector(notnull ChefZ_GradeRule raw,
                                     notnull ChefZ_CompiledGradeRule rule,
                                     ChefZ_CompileContext ctx,
                                     out string why)
    {
        why = "";

        if (!raw.selector)
        {
            why = "eine Regel mit when \"" + ChefZ_GradeWhen.Name(rule.when)
                + "\" braucht das Feld \"selector\".";
            return false;
        }

        if (!ctx)
        {
            why = "es gibt keinen Kompilierkontext - der Selektor liesse sich nicht "
                + "aufloesen.";
            return false;
        }

        string error;
        ChefZ_CompiledSelector node = ChefZ_SelectorCompiler.Compile(raw.selector, ctx, error);
        if (!node)
        {
            why = "der Selektor ist ungueltig: " + error + ".";
            return false;
        }

        rule.selector = node;
        return true;
    }

    private bool ResolveRuleContext(notnull ChefZ_GradeRule raw,
                                    notnull ChefZ_CompiledGradeRule rule,
                                    out string why)
    {
        why = "";

        if (raw.contextKey == "")
        {
            why = "eine Regel mit when \"context\" braucht das Feld \"contextKey\". "
                + "Gueltig: " + ChefZ_GradeContextKey.ValidNames() + ".";
            return false;
        }

        int key = ChefZ_GradeContextKey.FromName(raw.contextKey);
        if (key == ChefZ_GradeContextKey.NONE)
        {
            why = "contextKey \"" + raw.contextKey + "\" ist unbekannt. Gueltig: "
                + ChefZ_GradeContextKey.ValidNames() + ".";
            return false;
        }

        if (rule.range.IsUnbounded())
        {
            why = "eine Regel mit when \"context\" braucht ein \"range\" - ohne Grenzen "
                + "waere sie immer erfuellt.";
            return false;
        }

        rule.contextKey = key;
        return true;
    }

    //==========================================================================
    // Die Punktrechnung (12 §4)
    //==========================================================================

    /**
     * Die Formel aus 12 §4, Term fuer Term und in genau dieser Reihenfolge:
     *
     *   score = SUM slot.gradePoints ueber belegte Slots
     *         + SUM ueber ausgewertete gradeRules
     *         + (minFreshness - 0.5) * 2 * freshnessWeight
     *         + (mittlerer Zutatenrang - baseRank) * ingredientQualityWeight
     *         + SUM statePenalty[zustand] ueber die Zutaten
     *         + recipe.qualityBias
     *         + externalBonus, geklemmt
     *         mal ctx.qualityModifier
     *
     * Rein rechnend: kein Itemzugriff, kein Seiteneffekt. Das Ergebnis steht
     * VOLLSTAENDIG in eval - jeder Summand einzeln, jede Zahl mit einer
     * Klartextnotiz daneben (12 E1).
     *
     * eval darf null sein; dann wird eines angelegt. Der Aufrufer bringt
     * ueblicherweise seinen wiederverwendeten Puffer mit.
     */
    void ComputeScore(notnull ChefZ_CompiledRecipe recipe,
                      notnull ChefZ_MatchResult match,
                      notnull ChefZ_FactSnapshot snapshot,
                      notnull ChefZ_CookContext ctx,
                      float externalBonus,
                      out ChefZ_QualityEvaluation eval)
    {
        // Ueber eine lokale Zwischenvariable: ein Feld als out-Parameter ist
        // in Enforce nicht zugesichert (siehe ChefZ_TextList.SymbolsOf).
        ChefZ_QualityEvaluation e = eval;
        if (!e)
            e = new ChefZ_QualityEvaluation();
        e.Reset();
        eval = e;

        ChefZ_QualityScoring sc = Scoring();

        AddSlotPoints(recipe, match, e);
        AddRulePoints(recipe, match, snapshot, ctx, e);
        AddItemTerms(match, snapshot, sc, e);

        e.Bias = recipe.qualityBias;
        if (e.Bias != 0.0)
            e.AddNote("Rezeptvorgabe qualityBias: " + e.Bias.ToString() + " Punkte");

        e.ExternalBonus = ClampExternal(externalBonus, e);
        e.DeviceModifier = ctx.qualityModifier;

        float total = e.AdditiveSum() * e.DeviceModifier;

        if (!ChefZ_QualityEvaluation.IsFinite(total))
        {
            // 12 §8: "Score ist NaN (kaputte Regeldaten) -> auf 0 gesetzt,
            // ERROR." Ein NaN wuerde jeden Schwellenvergleich false ergeben
            // lassen, und das Gericht bekaeme lautlos gar keine Stufe.
            QuietOnce(ChefZ_LogLevel.ERR, "quality.nan." + recipe.id,
                "Rezept " + recipe.id + ": die Qualitaetspunktzahl ist keine Zahl. Ursache "
                + "sind immer kaputte Regel- oder Gewichtsdaten. Es gilt 0 Punkte.");
            e.AddNote("Punktzahl war keine Zahl - auf 0 gesetzt");
            total = 0.0;
        }

        e.TotalScore = total;

        if (e.DeviceModifier != 1.0)
        {
            e.AddNote("Geraetefaktor " + e.DeviceModifier.ToString() + ": "
                + e.AdditiveSum().ToString() + " -> " + e.TotalScore.ToString() + " Punkte");
        }
    }

    /**
     * Bequemlichkeit fuer den Kochpfad: rechnen UND die Stufe aufloesen.
     *
     * Getrennt von ComputeScore, weil 12 §5 die Rechnung ausdruecklich als
     * "rein rechnend, kein Itemzugriff, kein Seiteneffekt" fuehrt und die
     * Aufloesung eine zweite, eigenstaendige Frage ist. Wer nur wissen will,
     * wie viele Punkte etwas gibt (Cookbook, "chefz why"), ruft die erste.
     */
    ChefZ_Sym EvaluateResult(notnull ChefZ_CompiledRecipe recipe,
                             notnull ChefZ_MatchResult match,
                             notnull ChefZ_FactSnapshot snapshot,
                             notnull ChefZ_CookContext ctx,
                             float externalBonus,
                             out ChefZ_QualityEvaluation eval)
    {
        // Ueber eine lokale Zwischenvariable und nicht direkt durchgereicht:
        // einen out-Parameter als out-Argument weiterzugeben ist in Enforce
        // nirgends zugesichert.
        ChefZ_QualityEvaluation e = eval;
        ComputeScore(recipe, match, snapshot, ctx, externalBonus, e);
        eval = e;

        e.ResultTier = ResolveTier(e.TotalScore, recipe.qualityTierSet);
        return e.ResultTier;
    }

    //! SUM ueber belegte Slots: slot.gradePoints (12 §4, erste Zeile).
    private void AddSlotPoints(notnull ChefZ_CompiledRecipe recipe,
                               notnull ChefZ_MatchResult match,
                               notnull ChefZ_QualityEvaluation eval)
    {
        float sum = 0.0;

        for (int i = 0; i < recipe.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = recipe.slots.Get(i);
            if (!slot || slot.gradePoints == 0)
                continue;
            if (!match.IsSlotFilled(slot.slotId))
                continue;

            float points = slot.gradePoints;
            sum = sum + points;
            eval.AddNote("Slot \"" + slot.slotId + "\" belegt: " + points.ToString()
                + " Punkte");
        }

        eval.SlotPoints = sum;
    }

    //! SUM ueber die ausgewerteten gradeRules (12 §4, zweite Zeile).
    private void AddRulePoints(notnull ChefZ_CompiledRecipe recipe,
                               notnull ChefZ_MatchResult match,
                               notnull ChefZ_FactSnapshot snapshot,
                               notnull ChefZ_CookContext ctx,
                               notnull ChefZ_QualityEvaluation eval)
    {
        array<ref ChefZ_CompiledGradeRule> rules;
        if (!m_RulesByRecipe.Find(recipe.recipeSym, rules))
            return;

        float sum = 0.0;

        for (int i = 0; i < rules.Count(); i++)
        {
            ChefZ_CompiledGradeRule rule = rules.Get(i);
            if (!rule)
                continue;

            string note;
            sum = sum + rule.Evaluate(match, snapshot, ctx, m_Capabilities, note);
            eval.AddNote(note);
        }

        eval.RulePoints = sum;
    }

    /**
     * Die drei Terme, die ueber die gebundenen Zutaten laufen: Frische,
     * Zutatenqualitaet, Zustandsstrafen (12 §4).
     *
     * EIN Durchlauf ueber die Zutaten fuer alle drei. Nicht aus Sparsamkeit,
     * sondern weil "gebundene Zutat" fuer alle drei dieselbe Menge sein MUSS -
     * drei getrennte Schleifen waeren drei Gelegenheiten, die Menge
     * unterschiedlich zu bestimmen.
     *
     * 12 §4.1, die wichtigste Einzelregel dieser Rechnung: die Frische geht
     * als MINIMUM ein, nicht als Mittelwert. Eine einzige fast verdorbene
     * Zutat drueckt das Gericht. Sonst waere "altes Fleisch in einen
     * Premium-Eintopf waschen" ein Standardexploit.
     */
    private void AddItemTerms(notnull ChefZ_MatchResult match,
                              notnull ChefZ_FactSnapshot snapshot,
                              notnull ChefZ_QualityScoring sc,
                              notnull ChefZ_QualityEvaluation eval)
    {
        float minFreshness = 1.0;
        int   considered   = 0;

        float rankSum   = 0.0;
        int   rankCount = 0;

        float penalty = 0.0;

        for (int i = 0; i < match.boundHandles.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.FindByHandle(match.boundHandles.Get(i));
            if (!facts)
                continue;

            considered++;

            if (facts.freshness01 < minFreshness)
                minFreshness = facts.freshness01;

            if (ChefZ_SymbolTable.IsValid(facts.chefzQuality))
            {
                int rank;
                if (m_RankOf.Find(facts.chefzQuality, rank))
                {
                    float r = rank;
                    rankSum = rankSum + r;
                    rankCount++;
                }
                else
                {
                    // 12 §8: "Zutat mit unbekannter Qualitaets-ID -> Fallback,
                    // WARN einmal je Klasse." Die Zutat zaehlt fuer den
                    // Mittelwert dann gar nicht - sie mit Rang 0 zu werten
                    // waere eine Behauptung, die niemand geschrieben hat.
                    QuietOnce(ChefZ_LogLevel.WARN,
                        "quality.unknowntier." + facts.classSym.ToString(),
                        "Zutat \"" + ChefZ_SymbolTable.NameOrMark(facts.classSym)
                        + "\" traegt die unbekannte Qualitaetsstufe \""
                        + ChefZ_SymbolTable.NameOrMark(facts.chefzQuality)
                        + "\". Sie zaehlt fuer den Zutatenqualitaetsterm nicht mit. "
                        + "Haeufigste Ursache: das Modul mit dieser Stufe ist nicht geladen.");
                }
            }

            float p = sc.GetStatePenalty(facts.chefzState);
            if (p != 0.0)
            {
                penalty = penalty + p;
                eval.AddNote("Zustandsstrafe fuer \""
                    + ChefZ_SymbolTable.NameOrMark(facts.chefzState) + "\" an \""
                    + ChefZ_SymbolTable.NameOrMark(facts.classSym) + "\": "
                    + p.ToString() + " Punkte");
            }
        }

        eval.ConsideredItems = considered;
        eval.StatePenalty    = penalty;

        if (considered == 0)
        {
            // Kein gebundenes Item: Frische und Zutatenqualitaet haben keine
            // Grundlage. 0 statt eines geratenen Wertes - ein Rezept ohne
            // Zutaten soll weder belohnt noch bestraft werden.
            eval.MinFreshness = -1.0;
            eval.AddNote("keine gebundene Zutat - Frische- und Zutatenqualitaetsterm sind 0");
            return;
        }

        eval.MinFreshness  = minFreshness;
        eval.FreshnessTerm = (minFreshness - 0.5) * 2.0 * sc.freshnessWeight;
        eval.AddNote("geringste Frische " + minFreshness.ToString() + " (Minimum ueber "
            + considered.ToString() + " Zutaten, nicht Mittelwert): "
            + eval.FreshnessTerm.ToString() + " Punkte");

        if (rankCount == 0)
        {
            eval.AddNote("keine Zutat traegt eine bekannte Qualitaetsstufe - "
                + "Zutatenqualitaetsterm ist 0");
            return;
        }

        float count = rankCount;
        float meanRank = rankSum / count;
        eval.IngredientQualityTerm = (meanRank - sc.baseRank) * sc.ingredientQualityWeight;
        eval.AddNote("mittlerer Zutatenrang " + meanRank.ToString() + " gegen Bezugsrang "
            + sc.baseRank.ToString() + ": " + eval.IngredientQualityTerm.ToString()
            + " Punkte");
    }

    /**
     * 12 §8: "externalBonus unplausibel gross -> auf maxExternalQualityBonus
     * geklemmt, WARN."
     *
     * Symmetrisch geklemmt, also auch nach unten: ein Abonnent, der jedes
     * Gericht auf die unterste Stufe zieht, ist genauso ein Schaden wie einer,
     * der jedes auf PREMIUM hebt. Der Entwurf nennt den Schutz gegen "ein
     * Kompatibilitaetsmodul, das versehentlich jedes Gericht auf PREMIUM
     * hebt"; die andere Richtung ist derselbe Fehler mit anderem Vorzeichen.
     */
    private float ClampExternal(float bonus, notnull ChefZ_QualityEvaluation eval)
    {
        if (!ChefZ_QualityEvaluation.IsFinite(bonus))
        {
            eval.AddNote("externer Bonus war keine Zahl - auf 0 gesetzt");
            return 0.0;
        }

        if (bonus > m_MaxExternalBonus)
        {
            QuietOnce(ChefZ_LogLevel.WARN, "quality.external.high",
                "Ein externer Qualitaetsbonus von " + bonus.ToString() + " wurde auf "
                + m_MaxExternalBonus.ToString() + " geklemmt (CoreSettings."
                + "maxExternalQualityBonus). Diese Meldung erscheint genau einmal.");
            eval.AddNote("externer Bonus " + bonus.ToString() + " auf "
                + m_MaxExternalBonus.ToString() + " geklemmt");
            return m_MaxExternalBonus;
        }

        float lower = -m_MaxExternalBonus;
        if (bonus < lower)
        {
            QuietOnce(ChefZ_LogLevel.WARN, "quality.external.low",
                "Ein externer Qualitaetsabzug von " + bonus.ToString() + " wurde auf "
                + lower.ToString() + " geklemmt (CoreSettings.maxExternalQualityBonus). "
                + "Diese Meldung erscheint genau einmal.");
            eval.AddNote("externer Abzug " + bonus.ToString() + " auf " + lower.ToString()
                + " geklemmt");
            return lower;
        }

        if (bonus != 0.0)
            eval.AddNote("externer Bonus: " + bonus.ToString() + " Punkte");

        return bonus;
    }

    //==========================================================================
    // Stufen aufloesen und vergleichen (12 §5)
    //==========================================================================

    /**
     * Die hoechste Stufe des Satzes mit minScore <= score (12 §4).
     *
     * Zwei Feinheiten, beide aus 12 §8:
     *
     *   - Ein unbekannter Stufensatz faellt auf den Vorgabesatz zurueck, statt
     *     das Gericht ohne Stufe zu lassen.
     *   - Liegt der Score unter JEDER Schwelle, gewinnt die unterste Stufe.
     *     Das ist die "implizite Nullstufe" aus 12 §8 - ohne eine Stufe zu
     *     erfinden, die niemand geschrieben hat (siehe WarnOnMissingZero).
     */
    ChefZ_Sym ResolveTier(float score, ChefZ_Sym tierSet)
    {
        if (!GuardReady("ResolveTier"))
            return ChefZ_SymbolTable.INVALID;

        array<ChefZ_Sym> tiers = TiersOfSet(tierSet);
        if (!tiers || tiers.Count() == 0)
            return ChefZ_SymbolTable.INVALID;

        if (!ChefZ_QualityEvaluation.IsFinite(score))
            return tiers.Get(0);

        ChefZ_Sym result = tiers.Get(0);

        for (int i = 0; i < tiers.Count(); i++)
        {
            ChefZ_QualityTierDef def = m_BySym.Get(tiers.Get(i));
            if (!def)
                continue;
            if (def.minScore > score)
                break;                  // die Leiter ist nach Schwelle sortiert
            result = def.sym;
        }

        return result;
    }

    ChefZ_Sym GetLowestTier(ChefZ_Sym tierSet)
    {
        if (!GuardReady("GetLowestTier"))
            return ChefZ_SymbolTable.INVALID;

        array<ChefZ_Sym> tiers = TiersOfSet(tierSet);
        if (!tiers || tiers.Count() == 0)
            return ChefZ_SymbolTable.INVALID;
        return tiers.Get(0);
    }

    ChefZ_Sym GetHighestTier(ChefZ_Sym tierSet)
    {
        if (!GuardReady("GetHighestTier"))
            return ChefZ_SymbolTable.INVALID;

        array<ChefZ_Sym> tiers = TiersOfSet(tierSet);
        if (!tiers || tiers.Count() == 0)
            return ChefZ_SymbolTable.INVALID;
        return tiers.Get(tiers.Count() - 1);
    }

    /**
     * Stufe um "steps" nach unten (12 E8: degrade verschiebt, blockiert
     * nicht).
     *
     * Geklemmt an der untersten Stufe des Satzes: ein Spieler ohne Skill
     * bekommt das Gericht, nur schlechter. Er bekommt es NIE gar nicht - das
     * waere Blockieren mit anderen Mitteln.
     */
    ChefZ_Sym DegradeTier(ChefZ_Sym tier, int steps)
    {
        return ShiftIndex(tier, -steps);
    }

    /**
     * Stufe um delta verschieben (12 §6, "BEI EINEM TRANSFORM").
     *
     * "Raeuchern hebt die Qualitaet um eine Stufe" ist damit eine Zahl in
     * JSON. float, weil transform.qualityDelta eine ist; gerundet wird
     * kaufmaennisch, damit +0.5 aufsteigt statt zu verschwinden.
     */
    ChefZ_Sym ShiftRank(ChefZ_Sym tier, float delta)
    {
        return ShiftIndex(tier, RoundHalfAwayFromZero(delta));
    }

    private ChefZ_Sym ShiftIndex(ChefZ_Sym tier, int steps)
    {
        if (!GuardReady("ShiftRank"))
            return tier;

        int setSym;
        if (!m_SetOf.Find(tier, setSym))
            return tier;                // unbekannte Stufe bleibt, wie sie ist

        array<ChefZ_Sym> tiers;
        if (!m_Sets.Find(setSym, tiers) || tiers.Count() == 0)
            return tier;

        int rank;
        if (!m_RankOf.Find(tier, rank))
            return tier;

        int target = rank + steps;
        if (target < 0)
            target = 0;
        if (target > tiers.Count() - 1)
            target = tiers.Count() - 1;

        return tiers.Get(target);
    }

    /**
     * Raenge mehrerer Eingaenge zusammenfassen (12 §6, "BEI EINEM TRANSFORM").
     *
     * rule: "MIN" | "MEAN" | "WEIGHTED_MEAN" | "MAX". Unbekannte Regel gilt
     * als MIN - das ist die vorsichtigste Antwort, und Vorsicht ist hier
     * richtig: aus zwei mittelmaessigen Zutaten soll nicht durch einen
     * Tippfehler etwas Gutes werden.
     *
     * Gewichtet wird mit ChefZ_ItemFacts.units, also in Rezepteinheiten
     * (05 §6). Eine Zutat ohne Mengenangabe zaehlt einfach.
     */
    ChefZ_Sym CombineRanks(notnull array<ref ChefZ_ItemFacts> inputs,
                           string rule,
                           ChefZ_Sym tierSet)
    {
        if (!GuardReady("CombineRanks"))
            return ChefZ_SymbolTable.INVALID;

        array<ChefZ_Sym> tiers = TiersOfSet(tierSet);
        if (!tiers || tiers.Count() == 0)
            return ChefZ_SymbolTable.INVALID;

        int   count    = 0;
        int   minRank  = 0;
        int   maxRank  = 0;
        float sum      = 0.0;
        float weighted = 0.0;
        float weight   = 0.0;

        for (int i = 0; i < inputs.Count(); i++)
        {
            ChefZ_ItemFacts facts = inputs.Get(i);
            if (!facts)
                continue;

            int setOf;
            if (!m_SetOf.Find(facts.chefzQuality, setOf))
                continue;
            if (setOf != SetSymOrDefault(tierSet))
                continue;               // 12 E4: Vergleiche gelten je Satz

            int rank;
            if (!m_RankOf.Find(facts.chefzQuality, rank))
                continue;

            float r = rank;
            float w = facts.units;
            if (w <= 0.0)
                w = 1.0;

            if (count == 0)
            {
                minRank = rank;
                maxRank = rank;
            }
            else
            {
                if (rank < minRank)
                    minRank = rank;
                if (rank > maxRank)
                    maxRank = rank;
            }

            sum      = sum + r;
            weighted = weighted + r * w;
            weight   = weight + w;
            count++;
        }

        if (count == 0)
            return ChefZ_SymbolTable.INVALID;

        int target = minRank;

        if (rule == "MAX")
        {
            target = maxRank;
        }
        else if (rule == "MEAN")
        {
            float n = count;
            target = RoundHalfAwayFromZero(sum / n);
        }
        else if (rule == "WEIGHTED_MEAN")
        {
            if (weight > 0.0)
                target = RoundHalfAwayFromZero(weighted / weight);
        }
        else if (rule != "MIN" && rule != "")
        {
            QuietOnce(ChefZ_LogLevel.WARN, "quality.combine." + rule,
                "Unbekannte Zusammenfassungsregel \"" + rule + "\" - es gilt MIN. Gueltig: "
                + "MIN, MEAN, WEIGHTED_MEAN, MAX.");
        }

        if (target < 0)
            target = 0;
        if (target > tiers.Count() - 1)
            target = tiers.Count() - 1;

        return tiers.Get(target);
    }

    /**
     * -1, 0 oder 1 nach dem WIRKSAMEN Rang.
     *
     * Stufen aus verschiedenen Saetzen sind NICHT vergleichbar (12 E4) und
     * liefern 0 - "gleich" im Sinne von "es gibt keine Ordnung zwischen
     * ihnen". Eine erfundene Ordnung waere schlimmer: sie saehe richtig aus.
     */
    int CompareTiers(ChefZ_Sym a, ChefZ_Sym b)
    {
        if (a == b)
            return 0;
        if (!GuardReady("CompareTiers"))
            return 0;

        int setA;
        int setB;
        if (!m_SetOf.Find(a, setA) || !m_SetOf.Find(b, setB))
            return 0;

        if (setA != setB)
        {
            QuietOnce(ChefZ_LogLevel.WARN, "quality.compare.cross",
                "Vergleich zweier Qualitaetsstufen aus verschiedenen Stufensaetzen ("
                + ChefZ_SymbolTable.NameOrMark(a) + " gegen " + ChefZ_SymbolTable.NameOrMark(b)
                + "). Zwischen ihnen gibt es keine Ordnung (12 E4); die Antwort ist "
                + "\"gleich\".");
            return 0;
        }

        int rankA;
        int rankB;
        if (!m_RankOf.Find(a, rankA) || !m_RankOf.Find(b, rankB))
            return 0;

        if (rankA < rankB)
            return -1;
        if (rankA > rankB)
            return 1;
        return 0;
    }

    bool TierSetExists(ChefZ_Sym tierSet)
    {
        if (!m_Ready)
            return false;
        return m_Sets.Contains(tierSet);
    }

    bool Exists(ChefZ_Sym tier)
    {
        if (!m_Ready)
            return false;
        return m_BySym.Contains(tier);
    }

    //==========================================================================
    // Auskuenfte ueber eine Stufe (12 §5)
    //==========================================================================

    /**
     * Der Datensatz oder null.
     *
     * null ist eine normale Antwort: ein Gericht ohne Stufe, eine Stufe aus
     * einem nicht geladenen Modul. Wer im heissen Pfad steht, nimmt
     * GetOrFallback und muss dann gar nichts pruefen.
     */
    ChefZ_QualityTierDef GetDef(ChefZ_Sym tier)
    {
        if (!GuardReady("GetDef"))
            return null;

        ChefZ_QualityTierDef def;
        if (!m_BySym.Find(tier, def))
            return null;
        return def;
    }

    //! Nie null. Der Rueckfall ist in jeder Hinsicht neutral: Ausbeute 1.0,
    //! Verderb 1.0, kein Portionsbonus, keine Tags (12 §8).
    ChefZ_QualityTierDef GetOrFallback(ChefZ_Sym tier)
    {
        ChefZ_QualityTierDef def = GetDef(tier);
        if (def)
            return def;
        return m_Fallback;
    }

    float GetYieldMultiplier(ChefZ_Sym tier)
    {
        return GetOrFallback(tier).yieldMultiplier;
    }

    int GetPortionBonus(ChefZ_Sym tier)
    {
        return GetOrFallback(tier).portionBonus;
    }

    float GetSpoilageMultiplier(ChefZ_Sym tier)
    {
        return GetOrFallback(tier).spoilageMultiplier;
    }

    string GetDisplayKey(ChefZ_Sym tier)
    {
        return GetOrFallback(tier).displayName;
    }

    string GetColorHint(ChefZ_Sym tier)
    {
        return GetOrFallback(tier).colorHint;
    }

    //! Der wirksame Rang innerhalb des Satzes. -1 = unbekannte Stufe.
    int GetRank(ChefZ_Sym tier)
    {
        if (!m_Ready)
            return -1;
        int rank;
        if (m_RankOf.Find(tier, rank))
            return rank;
        return -1;
    }

    //! Der Stufensatz einer Stufe. INVALID = unbekannte Stufe.
    ChefZ_Sym GetTierSet(ChefZ_Sym tier)
    {
        if (!m_Ready)
            return ChefZ_SymbolTable.INVALID;
        int setSym;
        if (m_SetOf.Find(tier, setSym))
            return setSym;
        return ChefZ_SymbolTable.INVALID;
    }

    //! Die geprueften grantsTags. outTags wird geleert, nie null.
    void GetGrantedTags(ChefZ_Sym tier, out array<ChefZ_Sym> outTags)
    {
        if (!outTags)
            outTags = new array<ChefZ_Sym>();
        outTags.Clear();

        if (!m_Ready)
            return;

        array<ChefZ_Sym> tags;
        if (!m_TagsOf.Find(tier, tags))
            return;

        for (int i = 0; i < tags.Count(); i++)
            outTags.Insert(tags.Get(i));
    }

    /**
     * Die Effekt-IDs der Stufe (12 §2, "Effekt-IDs").
     *
     * Vollstaendig opaque: der Core wertet keine einzige aus. Sie werden nur
     * weitergereicht - seit S13 an das ChefZ_OnRecipeCompleted-Ereignis
     * (17 §4), wo ein Comp-Modul sie auswerten kann.
     */
    void GetGrantedEffects(ChefZ_Sym tier, out array<string> outEffects)
    {
        if (!outEffects)
            outEffects = new array<string>();
        outEffects.Clear();

        ChefZ_QualityTierDef def = GetDef(tier);
        if (!def || !def.grantsEffects)
            return;

        for (int i = 0; i < def.grantsEffects.Count(); i++)
        {
            string e = def.grantsEffects.Get(i);
            if (e != "" && outEffects.Find(e) < 0)
                outEffects.Insert(e);
        }
    }

    //==========================================================================
    // Identitaeten (03)
    //==========================================================================

    //! 0 = kein Ordinal. Eine Stufe ohne Ordinal ist clientseitig nicht
    //! darstellbar - serverseitig aber voll benutzbar (03 §7).
    int GetSyncOrdinal(ChefZ_Sym tier)
    {
        if (!m_Identities)
            return 0;
        return m_Identities.ToSyncOrdinal(tier);
    }

    ChefZ_Sym FromSyncOrdinal(int ordinal)
    {
        if (ordinal <= 0 || !m_Identities)
            return ChefZ_SymbolTable.INVALID;
        return m_Identities.FromSyncOrdinal(ordinal);
    }

    //==========================================================================
    // Zugaenge fuer den Boot und fuer Fremdmodule
    //==========================================================================

    /**
     * Die Auskunftsstelle fuer Faehigkeitswerte setzen (12 E6, 17 §3.3).
     *
     * null ist erlaubt und der Normalfall: ohne Skillmodul gibt es keinen
     * Skillbonus, und das ist ausdruecklich kein Fehler.
     */
    void SetCapabilityProbe(ChefZ_CapabilityProbe probe)
    {
        m_Capabilities = probe;
    }

    bool HasCapabilityProbe()
    {
        if (m_Capabilities)
            return true;
        return false;
    }

    //! Nie null - notfalls die Code-Defaults (dieselbe Zusage wie
    //! ChefZ_ConfigManager.GetSettings()).
    ChefZ_QualityScoring Scoring()
    {
        if (!m_Scoring)
            m_Scoring = new ChefZ_QualityScoring();
        return m_Scoring;
    }

    float GetMaxExternalBonus()
    {
        return m_MaxExternalBonus;
    }

    /**
     * Der Vorgabesatz als Symbol. Wird vom ChefZ_ManagerSymbolResolver und von
     * ResolveTier gebraucht.
     */
    ChefZ_Sym GetDefaultTierSet()
    {
        return m_DefaultSetSym;
    }

    //! Alle Stufen in stabiler Reihenfolge (nach ID sortiert).
    void GetAll(out array<ChefZ_Sym> outTiers)
    {
        if (!outTiers)
            outTiers = new array<ChefZ_Sym>();
        outTiers.Clear();
        for (int i = 0; i < m_Order.Count(); i++)
            outTiers.Insert(m_Order.Get(i));
    }

    //! Die Stufen EINES Satzes, aufsteigend nach Schwelle. Nie null.
    void GetTiersOfSet(ChefZ_Sym tierSet, out array<ChefZ_Sym> outTiers)
    {
        if (!outTiers)
            outTiers = new array<ChefZ_Sym>();
        outTiers.Clear();

        array<ChefZ_Sym> tiers = TiersOfSet(tierSet);
        if (!tiers)
            return;
        for (int i = 0; i < tiers.Count(); i++)
            outTiers.Insert(tiers.Get(i));
    }

    //! Alle Stufensaetze, in Entdeckungsreihenfolge.
    void GetTierSets(out array<ChefZ_Sym> outSets)
    {
        if (!outSets)
            outSets = new array<ChefZ_Sym>();
        outSets.Clear();
        for (int i = 0; i < m_SetOrder.Count(); i++)
            outSets.Insert(m_SetOrder.Get(i));
    }

    //==========================================================================
    // Diagnose und Zaehler
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    int GetTierCount()
    {
        return m_Order.Count();
    }

    int GetTierSetCount()
    {
        return m_SetOrder.Count();
    }

    int GetRuleCount()
    {
        return m_RuleCount;
    }

    int GetRejectedRuleCount()
    {
        return m_RejectedRules;
    }

    //! Die uebersetzten Regeln eines Rezepts, oder null. Fuer Diagnose und
    //! Selbsttest - der heisse Pfad geht ueber AddRulePoints.
    array<ref ChefZ_CompiledGradeRule> GetRules(ChefZ_Sym recipeSym)
    {
        array<ref ChefZ_CompiledGradeRule> rules;
        if (m_RulesByRecipe.Find(recipeSym, rules))
            return rules;
        return null;
    }

    void DumpTiers(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Qualitaetsstufen: " + GetTierCount().ToString() + "  saetze=" + GetTierSetCount().ToString()
            + "  regeln=" + m_RuleCount.ToString()
            + "  bereit=" + m_Ready.ToString());
        outLines.Insert("    " + Scoring().ToDebugString());

        for (int s = 0; s < m_SetOrder.Count(); s++)
        {
            ChefZ_Sym setSym = m_SetOrder.Get(s);
            outLines.Insert("    Satz " + ChefZ_SymbolTable.NameOrMark(setSym));

            array<ChefZ_Sym> tiers;
            if (!m_Sets.Find(setSym, tiers))
                continue;

            for (int i = 0; i < tiers.Count(); i++)
            {
                ChefZ_Sym sym = tiers.Get(i);
                ChefZ_QualityTierDef def = m_BySym.Get(sym);
                if (!def)
                    continue;

                string line = "        " + def.ToLine() + "  wirksamerRang=" + GetRank(sym).ToString() + "  ord=" + GetSyncOrdinal(sym).ToString();
                outLines.Insert(line);
            }
        }
    }

    private void LogIfDebug()
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.QUALITY, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        DumpTiers(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.QUALITY, lines);
    }

    //==========================================================================
    // Innereien
    //==========================================================================

    //! Der Satz, in dem gesucht wird: der genannte, sonst der Vorgabesatz
    //! (12 §8).
    private ChefZ_Sym SetSymOrDefault(ChefZ_Sym tierSet)
    {
        if (ChefZ_SymbolTable.IsValid(tierSet) && m_Sets.Contains(tierSet))
            return tierSet;
        return m_DefaultSetSym;
    }

    private array<ChefZ_Sym> TiersOfSet(ChefZ_Sym tierSet)
    {
        array<ChefZ_Sym> tiers;
        if (m_Sets.Find(SetSymOrDefault(tierSet), tiers))
            return tiers;
        return null;
    }

    //! Kaufmaennisch runden. Math.Round rundet in Enforce zur naechsten
    //! GERADEN Zahl; fuer "+0.5 hebt eine Stufe" waere das die falsche Regel.
    private int RoundHalfAwayFromZero(float v)
    {
        if (v >= 0.0)
            return (int)(v + 0.5);
        return -(int)(-v + 0.5);
    }

    private ChefZ_CategoryManager Cats()
    {
        if (m_CategoriesForTest)
            return m_CategoriesForTest;
        return ChefZ_CategoryManager.Get();
    }

    private void ResetState()
    {
        m_BySym.Clear();
        m_SetOf.Clear();
        m_RankOf.Clear();
        m_Sets.Clear();
        m_TagsOf.Clear();
        m_SetOrder.Clear();
        m_Order.Clear();
        m_RulesByRecipe.Clear();

        m_Identities       = null;
        m_Ready            = false;
        m_NotReadyLogged   = false;
        m_RuleCount        = 0;
        m_RejectedRules    = 0;
        m_MaxExternalBonus = 2.0;
        m_DefaultSetSym    = ChefZ_SymbolTable.INVALID;
    }

    //! Leert den Bestand. Vorgesehener Aufrufer ist der SAFE_MODE (02 §8).
    void Reset()
    {
        ResetState();
    }

    //! 12 §8 sinngemaess: Abfrage vor Build liefert die neutrale Antwort und
    //! meldet EINMAL. Kein Nullzugriff, kein Absturz.
    private bool GuardReady(string what)
    {
        if (m_Ready)
            return true;
        if (m_NotReadyLogged)
            return false;
        m_NotReadyLogged = true;

        if (m_QuietForTest)
            return false;

        ChefZ_Log.Error(ChefZ_LogChannel.QUALITY,
            "ChefZ_QualityManager." + what + "() wurde vor Build() aufgerufen - die Antwort "
            + "ist \"keine Stufe\". Gerichte entstehen solange ohne Qualitaet; "
            + "Vanilla-Kochen ist davon unberuehrt. Diese Meldung erscheint genau einmal.");
        return false;
    }

    private void QuietOnce(int level, string key, string message)
    {
        if (m_QuietForTest)
            return;
        ChefZ_Log.Once(level, ChefZ_LogChannel.QUALITY, key, message);
    }

    private void Report(ChefZ_LoadReport report, bool isError,
                        notnull ChefZ_QualityTierDef def, string msg)
    {
        if (report)
        {
            if (isError)
                report.AddError(def.sourceRef, def.id, msg);
            else
                report.AddWarn(def.sourceRef, def.id, msg);
            return;
        }

        if (m_QuietForTest)
            return;

        string line = def.sourceRef + " / " + def.id + ": " + msg;
        if (isError)
            ChefZ_Log.Error(ChefZ_LogChannel.QUALITY, line);
        else
            ChefZ_Log.Warn(ChefZ_LogChannel.QUALITY, line);
    }

    private void ReportSet(ChefZ_LoadReport report, ChefZ_Sym setSym, string msg)
    {
        string setName = ChefZ_SymbolTable.NameOrMark(setSym);

        if (report)
        {
            report.AddWarn("CfgChefZQualityTiers", setName, msg);
            return;
        }

        if (m_QuietForTest)
            return;

        ChefZ_Log.Warn(ChefZ_LogChannel.QUALITY, "Stufensatz " + setName + ": " + msg);
    }

    //! Nur fuer den Selbsttest: unterdrueckt die Meldungen dieser Klasse.
    //! Noetig, weil der Test die Fehlerfaelle absichtlich durchspielt und
    //! ChefZ_Log.GetErrorCount() die Safe-Mode-Schwelle speist (18 §4).
    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }

    //! Kategoriebaum des Tests statt des Singletons.
    void SetCategoryManagerForTest(ChefZ_CategoryManager mgr)
    {
        m_CategoriesForTest = mgr;
    }
}
