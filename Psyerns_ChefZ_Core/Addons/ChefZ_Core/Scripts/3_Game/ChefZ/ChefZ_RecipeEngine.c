//==============================================================================
// ChefZ_RecipeEngine - Index, Reihenfolge, Auswahl
//
// Entwurf: 08 §3 (Schnittstelle), 08 §4 (Auswertungsablauf), 08 §5 (Index und
// Kosten), 08 §7 (Zustand), 08 §8 (Fehlerverhalten), 08 E3 (der erste Treffer
// gewinnt), 09 §4.3 (Rangreihenfolge im Index), 09 §5 (Datenfluss),
// 10 §5 (wer die Engine wann ruft).
//
// ---------------------------------------------------------------------------
// Die zwei Saetze, die den ganzen Aufbau erklaeren
// ---------------------------------------------------------------------------
// 1. "Kandidaten kommen bereits in Rangreihenfolge aus dem Index" (09 §5).
//    Deshalb wird zur Laufzeit NICHT sortiert. Das Rezeptarray ist nach dem
//    Build in Rangreihenfolge, die Indexlisten werden in dieser Reihenfolge
//    befuellt, und eine Kandidatenliste entsteht als aufsteigende Mischung
//    aufsteigender Listen.
//
// 2. "Der erste Erfolg gewinnt" (08 E3). Weil die Reihenfolge die
//    Spezifitaetsordnung IST, ist der erste Erfolg auch der richtige. Kein
//    Volldurchlauf, keine Bewertung aller Kandidaten - das waere pro Tick und
//    pro brennender Feuerstelle die volle Rechnung.
//
// Der Volldurchlauf existiert trotzdem, aber als eigene Methode fuer Cookbook
// und Validator: EvaluateAll(). Wer sie im Kochpfad benutzt, hat den Entwurf
// missverstanden.
//
// ---------------------------------------------------------------------------
// Der billige Vorfilter
// ---------------------------------------------------------------------------
// 10 §5 Stufe 0 ruft HasAnyRecipeFor() als eine der ersten Zeilen jedes
// Kochticks. Sie muss ein Map-Lookup sein und sonst nichts. Bei leerem
// Rezeptbestand ist der ganze Hook damit ein Bool-Test - 08 §8, erste Zeile:
// "Messbar kostenlos. Vanilla unveraendert."
//
// ---------------------------------------------------------------------------
// Zustand
// ---------------------------------------------------------------------------
// Nach Build() ist alles hier unveraenderlich (08 §7). Die einzigen
// veraenderlichen Felder sind Arbeitspuffer, die je Auswertung geleert werden -
// und die Engine ist gegenueber der WELT zustandslos: zweimal dieselbe
// Eingabe ergibt zweimal dasselbe Ergebnis.
//
// KEIN CONTENT.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_RecipeEngine
{
    private static ref ChefZ_RecipeEngine s_Instance;

    //--- Bestand, nach dem Build in RANGREIHENFOLGE --------------------------
    private ref array<ref ChefZ_CompiledRecipe> m_Recipes;
    private ref array<ref ChefZ_RecipeRank>     m_Ranks;      // parallel zu m_Recipes

    //--- Invertierter Index (08 §5.1) ----------------------------------------
    //
    // Alle Listen sind aufsteigend nach Rezeptindex, und weil der Index die
    // Rangreihenfolge ist, sind sie damit in Rangreihenfolge. Das ist keine
    // Nebenwirkung, sondern der Grund, warum zur Laufzeit nicht sortiert wird.
    private ref map<int, ref array<int>> m_ByDeviceClass;
    private ref map<int, ref array<int>> m_ByDeviceCategory;
    private ref map<int, int>            m_MinItemsByDeviceClass;

    //! Rezepte, deren Kontext an kein Geraet gebunden ist. Sie sind bei jeder
    //! Auswertung Kandidat - und deshalb bekommt jede Kontextregel ohne
    //! Geraetebindung beim Kompilieren ein WARN.
    private ref array<int> m_AnyDevice;
    private int m_AnyDeviceMinItems;

    //--- Einstellungen -------------------------------------------------------
    private ref ChefZ_PriorityWeights m_Weights;
    private int m_NodeBudget;
    private bool m_Ready;

    /**
     * Pruefung der Ergebnisklassen gegen CfgVehicles (08 §8, V-B Auflage 4).
     *
     * Im Betrieb IMMER an - sie ist die letzte der drei Stellen, die eine
     * essbare Ergebnisklasse ohne Nutrition-Block abfangen, und die einzige,
     * die auf dem echten Server laeuft.
     *
     * Abschaltbar ausschliesslich fuer den Selbsttest. Seine Rezepte nennen
     * absichtlich keine echten Klassen: der Core darf keine anlegen
     * (Invariante I3), und eine Vanilla-Klasse zu nennen waere ein
     * Content-Bezeichner im Core - genau das, was chefzcore.mjs spaeter
     * verbietet.
     */
    private bool m_VerifyOutputClasses;

    //--- Arbeitspuffer, je Auswertung geleert --------------------------------
    private ref ChefZ_VesselKeys m_Keys;
    private ref array<int>       m_Candidates;

    void ChefZ_RecipeEngine()
    {
        m_Recipes               = new array<ref ChefZ_CompiledRecipe>();
        m_Ranks                 = new array<ref ChefZ_RecipeRank>();
        m_ByDeviceClass         = new map<int, ref array<int>>();
        m_ByDeviceCategory      = new map<int, ref array<int>>();
        m_MinItemsByDeviceClass = new map<int, int>();
        m_AnyDevice             = new array<int>();
        m_AnyDeviceMinItems     = 0;
        m_Weights               = new ChefZ_PriorityWeights();
        m_NodeBudget            = 4096;
        m_Ready                 = false;
        m_Keys                  = new ChefZ_VesselKeys();
        m_Candidates            = new array<int>();
        m_VerifyOutputClasses   = true;
    }

    //! Nur fuer den Selbsttest (siehe m_VerifyOutputClasses).
    void SetVerifyOutputClasses(bool on)
    {
        m_VerifyOutputClasses = on;
    }

    static ChefZ_RecipeEngine Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_RecipeEngine();
        return s_Instance;
    }

    //==========================================================================
    // BUILD (09 §5)
    //==========================================================================

    /**
     * Baut Bestand, Rangordnung und Index.
     *
     * Abweichung von der Signatur in 08 §3, und sie ist unvermeidlich: dort
     * steht Build(defs, report). Zum Kompilieren braucht es ausserdem den
     * Selektorkontext (07 §5) und die Einstellungen (defaultExtraItems,
     * matcherNodeBudget, allowTimedRecipes), und um Geraetekategorien auf
     * Klassen abzubilden die Geraeteregistry (08 E4). Alles davon liegt beim
     * Config Manager; es hier ueber einen Singleton zu holen waere eine
     * versteckte Abhaengigkeit statt einer sichtbaren.
     *
     * JEDER Parameter darf null sein. Ein Aufruf mit lauter null ist die
     * ausdrueckliche Art zu sagen "Bestand leeren" - genau das braucht der
     * SAFE_MODE (02 §8). Danach ist die Engine "bereit und leer", nicht
     * "nicht gebaut": jede Abfrage antwortet ruhig false, statt einen Fehler
     * ueber einen fehlenden Aufbau zu melden, den es nie geben wird.
     */
    void Build(ChefZ_Registry<ChefZ_RecipeDef> defs, ChefZ_Registry<ChefZ_DeviceDef> devices, ChefZ_CompileContext ctx, ChefZ_CoreSettingsDef settings, ChefZ_LoadReport report)
    {
        ClearAll();

        // Der Selektorcompiler und der Ranker MUESSEN mit demselben
        // Gewichtssatz rechnen, sonst rangiert der Ranker nach anderen Zahlen,
        // als in den Slots stehen. Der Kontext traegt ihn bereits - der Config
        // Manager hat ihn dort aus Core.json abgelegt, und zwar EINMAL, damit
        // die Warnungen aus 09 §7 nicht doppelt im Ladebericht stehen.
        //
        // Nur wenn es keinen Kontext gibt (Selbsttest ohne Config Manager),
        // wird er hier aus den Einstellungen gebaut.
        if (ctx)
            m_Weights = ctx.Weights();
        else if (settings)
            m_Weights = settings.BuildPriorityWeights(report);

        if (settings)
        {
            m_NodeBudget = settings.matcherNodeBudget;
            if (m_NodeBudget < 1)
                m_NodeBudget = 1;
        }

        m_Ready = true;
        if (!defs || defs.Count() == 0)
            return;

        CompileAll(defs, ctx, settings, report);
        BuildRanks();
        BuildIndex(devices);

        ChefZ_RecipeRanker.ReportAmbiguities(m_Recipes, m_Ranks, report);
        ReportSummary(report);
    }

    private void ClearAll()
    {
        m_Recipes.Clear();
        m_Ranks.Clear();
        m_ByDeviceClass.Clear();
        m_ByDeviceCategory.Clear();
        m_MinItemsByDeviceClass.Clear();
        m_AnyDevice.Clear();
        m_AnyDeviceMinItems = 0;
        m_Candidates.Clear();
        m_Keys.Clear();
        m_Ready = false;
    }

    /**
     * Alle Rezepte uebersetzen, in stabiler Reihenfolge.
     *
     * Ueber Keys() und nicht ueber den Laufindex: Keys() ist nach ID sortiert
     * (03 §4), und damit ist die Reihenfolge der Meldungen im Ladebericht auf
     * jedem Server dieselbe. Fuer das Ergebnis ist sie ohne Bedeutung - danach
     * wird ohnehin nach Rang sortiert -, fuer die Vergleichbarkeit zweier
     * Ladeberichte ist sie alles.
     */
    private void CompileAll(notnull ChefZ_Registry<ChefZ_RecipeDef> defs, ChefZ_CompileContext ctx, ChefZ_CoreSettingsDef settings, ChefZ_LoadReport report)
    {
        ChefZ_RecipeCompiler compiler = new ChefZ_RecipeCompiler();
        compiler.Init(ctx, report, settings);
        compiler.SetVerifyClasses(m_VerifyOutputClasses);

        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_RecipeDef def = defs.Find(keys.Get(i));
            if (!def)
                continue;

            ChefZ_CompiledRecipe rec = compiler.Compile(def);
            if (!rec)
                continue;               // abgewiesen, Grund steht im Bericht

            m_Recipes.Insert(rec);
        }
    }

    //! Rangzahlen bilden und den Bestand danach umsortieren (09 §4.3).
    private void BuildRanks()
    {
        int i;

        m_Ranks.Clear();
        for (i = 0; i < m_Recipes.Count(); i++)
        {
            ChefZ_RecipeRank rank = new ChefZ_RecipeRank();
            rank.InitFrom(m_Recipes.Get(i), i);
            m_Ranks.Insert(rank);
        }

        array<int> order = new array<int>();
        for (i = 0; i < m_Recipes.Count(); i++)
            order.Insert(i);

        ChefZ_RecipeRanker.SortCandidates(m_Ranks, order);

        // Umsortieren statt nur eine Reihenfolge zu merken: danach IST der
        // Arrayindex der Rang, und jede Indexliste ist allein dadurch sortiert,
        // dass sie aufsteigend befuellt wird. Eine zweite Ordnung neben der
        // Speicherreihenfolge waere eine zweite Wahrheit.
        array<ref ChefZ_CompiledRecipe> sortedRecipes = new array<ref ChefZ_CompiledRecipe>();
        array<ref ChefZ_RecipeRank>     sortedRanks   = new array<ref ChefZ_RecipeRank>();

        for (i = 0; i < order.Count(); i++)
        {
            int from = order.Get(i);
            sortedRecipes.Insert(m_Recipes.Get(from));
            ChefZ_RecipeRank rank = m_Ranks.Get(from);
            rank.recipeIndex = i;
            sortedRanks.Insert(rank);
        }

        m_Recipes = sortedRecipes;
        m_Ranks   = sortedRanks;
    }

    //==========================================================================
    // Index (08 §5.1)
    //==========================================================================

    /**
     * Baut die drei Nachschlagerichtungen auf.
     *
     * Geraetekategorien werden dabei ueber die Geraeteregistry auf KLASSEN
     * ausgerollt. Das ist der Grund, warum HasAnyRecipeFor() ein einziger
     * Map-Lookup sein kann, obwohl Rezepte nach 08 E4 bevorzugt an Kategorien
     * binden: die Aufloesung passiert einmal beim Boot und nicht bei jedem
     * Tick.
     *
     * Die Kategorieliste bleibt zusaetzlich erhalten. Sie ist das Netz fuer den
     * Fall, dass der Adapter zur Laufzeit eine Kategorie mitbringt, die beim
     * Build nicht in der Registry stand - dann ist die Auswertung teurer, aber
     * nie falsch.
     */
    private void BuildIndex(ChefZ_Registry<ChefZ_DeviceDef> devices)
    {
        map<int, ref array<int>> classesByCategory = BuildDeviceCategoryMap(devices);

        array<ChefZ_Sym> deviceClasses    = new array<ChefZ_Sym>();
        array<ChefZ_Sym> deviceCategories = new array<ChefZ_Sym>();

        for (int i = 0; i < m_Recipes.Count(); i++)
        {
            ChefZ_CompiledRecipe rec = m_Recipes.Get(i);

            if (rec.HasUnboundDeviceContext())
            {
                m_AnyDevice.Insert(i);
                if (m_AnyDevice.Count() == 1 || rec.minItemCount < m_AnyDeviceMinItems)
                    m_AnyDeviceMinItems = rec.minItemCount;
            }

            rec.CollectDeviceKeys(deviceClasses, deviceCategories);

            int k;
            for (k = 0; k < deviceClasses.Count(); k++)
                AddToDeviceClass(deviceClasses.Get(k), i, rec.minItemCount);

            for (k = 0; k < deviceCategories.Count(); k++)
            {
                ChefZ_Sym category = deviceCategories.Get(k);
                AddToIndex(m_ByDeviceCategory, category, i);

                array<int> members;
                if (!classesByCategory.Find(category, members))
                    continue;
                for (int m = 0; m < members.Count(); m++)
                    AddToDeviceClass(members.Get(m), i, rec.minItemCount);
            }
        }
    }

    //! Kategorie -> Geraeteklassen, aus den ChefZ_DeviceDef-Records (10 §4).
    private map<int, ref array<int>> BuildDeviceCategoryMap(ChefZ_Registry<ChefZ_DeviceDef> devices)
    {
        map<int, ref array<int>> byCategory = new map<int, ref array<int>>();
        if (!devices)
            return byCategory;

        array<ChefZ_Sym> keys = devices.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_DeviceDef device = devices.Find(keys.Get(i));
            if (!device || !device.deviceCategories)
                continue;

            ChefZ_Sym deviceSym = ChefZ_SymbolTable.Intern(device.id);
            for (int c = 0; c < device.deviceCategories.Count(); c++)
            {
                string name = device.deviceCategories.Get(c);
                if (name == "")
                    continue;
                AddToIndex(byCategory, ChefZ_SymbolTable.Intern(name), deviceSym);
            }
        }

        return byCategory;
    }

    private void AddToDeviceClass(ChefZ_Sym deviceClass, int recipeIndex, int minItems)
    {
        if (!ChefZ_SymbolTable.IsValid(deviceClass))
            return;

        if (!AddToIndex(m_ByDeviceClass, deviceClass, recipeIndex))
            return;                     // stand schon drin

        int current;
        if (!m_MinItemsByDeviceClass.Find(deviceClass, current) || minItems < current)
            m_MinItemsByDeviceClass.Set(deviceClass, minItems);
    }

    //! false, wenn der Wert schon in der Liste stand. Duplikate waeren nicht
    //! falsch, aber sie kosteten je Auswertung einen zweiten Bindungsversuch
    //! desselben Rezepts.
    private bool AddToIndex(notnull map<int, ref array<int>> index, ChefZ_Sym key, int value)
    {
        if (!ChefZ_SymbolTable.IsValid(key))
            return false;

        array<int> list;
        if (!index.Find(key, list))
        {
            list = new array<int>();
            index.Set(key, list);
        }
        if (list.Find(value) >= 0)
            return false;

        list.Insert(value);
        return true;
    }

    private void ReportSummary(ChefZ_LoadReport report)
    {
        if (!report)
            return;

        report.AddInfo("Recipe Engine: " + m_Recipes.Count().ToString() + " Rezepte, " + m_ByDeviceClass.Count().ToString() + " Geraeteklassen im Index, " + m_ByDeviceCategory.Count().ToString() + " Geraetekategorien, " + m_AnyDevice.Count().ToString() + " ohne Geraetebindung. Knotenbudget " + m_NodeBudget.ToString() + ".");
    }

    //==========================================================================
    // Auskuenfte (08 §3)
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    int GetRecipeCount()
    {
        return m_Recipes.Count();
    }

    ChefZ_CompiledRecipe GetRecipeAt(int index)
    {
        if (index < 0 || index >= m_Recipes.Count())
            return null;
        return m_Recipes.Get(index);
    }

    ChefZ_CompiledRecipe FindRecipe(ChefZ_Sym recipeSym)
    {
        for (int i = 0; i < m_Recipes.Count(); i++)
        {
            if (m_Recipes.Get(i).recipeSym == recipeSym)
                return m_Recipes.Get(i);
        }
        return null;
    }

    /**
     * Die erste Zeile jedes Kochticks (10 §5, Stufe 0).
     *
     * EIN Map-Lookup je Klasse, sonst nichts. Bei leerem Rezeptbestand ist der
     * gesamte ChefZ-Anteil eines Kochticks damit ein Bool-Test - genau das
     * verlangt 19 S7 als Abnahmebedingung.
     */
    bool HasAnyRecipeFor(ChefZ_Sym deviceClass, ChefZ_Sym deviceRootClass)
    {
        if (!m_Ready || m_Recipes.Count() == 0)
            return false;
        if (m_AnyDevice.Count() > 0)
            return true;
        if (m_ByDeviceClass.Contains(deviceClass))
            return true;
        return m_ByDeviceClass.Contains(deviceRootClass);
    }

    /**
     * Wie viele Items muessen mindestens im Gefaess liegen (08 §5.1)?
     *
     * Bei unbekannter Klasse 0 - also "nicht ausschliessen". Ein Vorfilter,
     * der bei fehlendem Wissen zu VIEL wegwirft, laesst Rezepte lautlos
     * ausfallen; einer, der zu wenig wegwirft, kostet nur Rechenzeit. Die
     * Richtung ist nicht verhandelbar.
     */
    int GetMinItemCountFor(ChefZ_Sym deviceClass)
    {
        int minItems;
        if (m_MinItemsByDeviceClass.Find(deviceClass, minItems))
        {
            if (m_AnyDevice.Count() > 0 && m_AnyDeviceMinItems < minItems)
                return m_AnyDeviceMinItems;
            return minItems;
        }

        if (m_AnyDevice.Count() > 0)
            return m_AnyDeviceMinItems;
        return 0;
    }

    ChefZ_PriorityWeights GetWeights()
    {
        return m_Weights;
    }

    int GetNodeBudget()
    {
        return m_NodeBudget;
    }

    //==========================================================================
    // Kandidaten (08 §5.1)
    //==========================================================================

    /**
     * Kandidaten = Geraetemenge, gefiltert nach Itemzahl und Torsymbol.
     *
     * Die Teillisten sind aufsteigend, also ist ihre Mischung aufsteigend, und
     * weil der Index die Rangreihenfolge ist, ist das Ergebnis in
     * Rangreihenfolge (09 §5). Es wird NICHT sortiert.
     */
    private void CollectCandidates(notnull ChefZ_CookContext ctx, int itemCount, notnull array<int> outIdx)
    {
        outIdx.Clear();

        MergeBucket(m_AnyDevice, outIdx);
        MergeFromIndex(m_ByDeviceClass, ctx.deviceClass, outIdx);
        MergeFromIndex(m_ByDeviceClass, ctx.deviceRootClass, outIdx);
        for (int i = 0; i < ctx.deviceCategories.Count(); i++)
            MergeFromIndex(m_ByDeviceCategory, ctx.deviceCategories.Get(i), outIdx);

        // Rueckwaerts filtern, damit das Entfernen die noch zu pruefenden
        // Positionen nicht verschiebt.
        //
        // RemoveOrdered und NICHT Remove: Enforce-Remove() fuellt die Luecke
        // mit dem LETZTEN Element und zerstoert damit genau die Ordnung, um
        // derentwillen dieser ganze Index existiert (EnScript.c:462 -
        // "do not retain order"). Das ist der teurere Aufruf und der einzig
        // richtige.
        for (int k = outIdx.Count() - 1; k >= 0; k--)
        {
            ChefZ_CompiledRecipe rec = m_Recipes.Get(outIdx.Get(k));
            if (rec.minItemCount > itemCount)
            {
                outIdx.RemoveOrdered(k);
                continue;
            }
            if (!m_Keys.Admits(rec.gateKind, rec.gateSym, rec.gateBit))
                outIdx.RemoveOrdered(k);
        }
    }

    private void MergeFromIndex(notnull map<int, ref array<int>> index, ChefZ_Sym key, notnull array<int> dst)
    {
        if (!ChefZ_SymbolTable.IsValid(key))
            return;
        array<int> bucket;
        if (!index.Find(key, bucket))
            return;
        MergeBucket(bucket, dst);
    }

    //! Aufsteigende Liste in eine aufsteigende Liste mischen, ohne Duplikate.
    private void MergeBucket(notnull array<int> src, notnull array<int> dst)
    {
        for (int i = 0; i < src.Count(); i++)
        {
            int value = src.Get(i);
            int at = dst.Count();
            while (at > 0 && dst.Get(at - 1) > value)
                at--;
            if (at > 0 && dst.Get(at - 1) == value)
                continue;

            // Anhaengen statt Einfuegen, wenn die Position ohnehin das Ende
            // ist: das ist der haeufigste Fall (die Teillisten kommen bereits
            // aufsteigend) und spart die Verschiebung.
            if (at >= dst.Count())
                dst.Insert(value);
            else
                dst.InsertAt(value, at);
        }
    }

    //==========================================================================
    // EvaluateBest (08 §4)
    //==========================================================================

    /**
     * Der Kochpfad. Erster Erfolg gewinnt (08 E3).
     *
     * result wird IMMER gefuellt. Bei false traegt es die Begruendung des
     * BESTPLATZIERTEN gescheiterten Kandidaten - nicht die des letzten. Das
     * ist die nuetzlichere Auskunft: der bestplatzierte Kandidat ist der, den
     * der Spieler am ehesten gemeint hat.
     */
    bool EvaluateBest(notnull ChefZ_CookContext ctx, notnull ChefZ_FactSnapshot snapshot, ChefZ_MatchTrace trace, out ChefZ_MatchResult result)
    {
        if (!result)
            result = new ChefZ_MatchResult();
        result.Reset();

        if (!m_Ready || m_Recipes.Count() == 0)
            return false;

        if (trace)
        {
            trace.Begin(ctx.deviceClass, ctx.method, snapshot.Count());
            trace.Contents(snapshot);
        }

        m_Keys.Build(snapshot);
        CollectCandidates(ctx, snapshot.Count(), m_Candidates);

        if (trace)
            trace.CandidateCount(m_Candidates.Count());

        // Die Begruendung des ersten Fehlschlags, bevor sie ueberschrieben wird.
        bool      haveFirstFail = false;
        ChefZ_Sym firstFailSym  = ChefZ_SymbolTable.INVALID;
        string    firstFailWhy  = "";
        string    firstFailSlot = "";
        int       nodes         = 0;
        int       tried         = 0;

        for (int i = 0; i < m_Candidates.Count(); i++)
        {
            ChefZ_CompiledRecipe rec = m_Recipes.Get(m_Candidates.Get(i));
            tried++;

            if (!ChefZ_RecipeEvaluator.Evaluate(rec, ctx, snapshot, m_NodeBudget, trace, result))
            {
                nodes = nodes + result.nodesExplored;
                if (!haveFirstFail)
                {
                    haveFirstFail = true;
                    firstFailSym  = rec.recipeSym;
                    firstFailWhy  = result.failReason;
                    firstFailSlot = result.failSlotId;
                }
                continue;
            }

            nodes = nodes + result.nodesExplored;
            result.nodesExplored   = nodes;
            result.candidatesTried = tried;
            result.score = ChefZ_RecipeRanker.ComputeMatchScore(rec, result, snapshot.Count(), m_Weights);

            string readyReason;
            result.ready          = ChefZ_RecipeEvaluator.CheckReady(rec, result, snapshot, ctx, readyReason);
            result.notReadyReason = readyReason;

            if (trace)
            {
                trace.Winner(rec.recipeSym, result.score, result.qualityTier, nodes);
                trace.Readiness(result.ready, readyReason);
            }
            return true;
        }

        // Kein Treffer. Das ist der HAEUFIGSTE Ausgang und ausdruecklich kein
        // Fehler: Vanilla hat zu diesem Zeitpunkt bereits gekocht (10 §3), die
        // Zutaten garen normal weiter (08 §8).
        result.Reset();
        result.candidatesTried = tried;
        result.nodesExplored   = nodes;
        result.itemsInVessel   = snapshot.Count();
        if (haveFirstFail)
        {
            result.failedRecipe = firstFailSym;
            result.failReason   = firstFailWhy;
            result.failSlotId   = firstFailSlot;
        }
        else
        {
            result.failReason = "kein Kandidatenrezept fuer dieses Geraet und diesen Inhalt";
        }
        return false;
    }

    //==========================================================================
    // EvaluateAll (08 E3, ausdruecklich NICHT fuer den Kochpfad)
    //==========================================================================

    /**
     * Alle passenden Rezepte, bestes zuerst.
     *
     * Fuer Cookbook, Adminkommandos und den Validator. Sie kostet die volle
     * Auswertung ALLER Kandidaten - im Kochtick waere das genau der Aufwand,
     * den 08 E3 vermeidet.
     *
     * Sortiert wird hier sehr wohl, und zwar mit ChefZ_RecipeRanker.
     * CompareMatches: erst hier liegen die Laufzeitscores vor.
     */
    int EvaluateAll(notnull ChefZ_CookContext ctx, notnull ChefZ_FactSnapshot snapshot, out array<ref ChefZ_MatchResult> results, int maxResults)
    {
        if (!results)
            results = new array<ref ChefZ_MatchResult>();
        results.Clear();

        if (!m_Ready || m_Recipes.Count() == 0)
            return 0;

        m_Keys.Build(snapshot);
        array<int> candidates = new array<int>();
        CollectCandidates(ctx, snapshot.Count(), candidates);

        for (int i = 0; i < candidates.Count(); i++)
        {
            ChefZ_CompiledRecipe rec = m_Recipes.Get(candidates.Get(i));

            ChefZ_MatchResult one = new ChefZ_MatchResult();
            if (!ChefZ_RecipeEvaluator.Evaluate(rec, ctx, snapshot, m_NodeBudget, null, one))
                continue;

            one.score = ChefZ_RecipeRanker.ComputeMatchScore(rec, one, snapshot.Count(), m_Weights);

            string readyReason;
            one.ready          = ChefZ_RecipeEvaluator.CheckReady(rec, one, snapshot, ctx, readyReason);
            one.notReadyReason = readyReason;

            InsertSorted(results, one);
        }

        // Das letzte Element zu entfernen ist der eine Fall, in dem Remove()
        // und RemoveOrdered() dasselbe tun.
        while (maxResults > 0 && results.Count() > maxResults)
            results.Remove(results.Count() - 1);

        return results.Count();
    }

    //! Einfuegesortierung nach 09 §4.4. Die Listen sind einstellig lang.
    private void InsertSorted(notnull array<ref ChefZ_MatchResult> list, notnull ChefZ_MatchResult item)
    {
        int at = list.Count();
        while (at > 0 && ChefZ_RecipeRanker.CompareMatches(list.Get(at - 1), item) > 0)
            at--;

        if (at >= list.Count())
            list.Insert(item);
        else
            list.InsertAt(item, at);
    }

    //==========================================================================
    // CheckReady und EvaluatePartial (08 §3)
    //==========================================================================

    /**
     * Nur die Abschlussbedingung eines bereits gebundenen Ergebnisses neu
     * pruefen.
     *
     * Das ist Stufe C aus 10 §5 und laeuft pro Tick, solange eine Sitzung
     * gebunden ist. Sie ist deshalb billig gehalten: kein Kandidatenlauf,
     * keine Bindung, nur ein Blick auf die FoodStages der gebundenen Zutaten.
     */
    bool CheckReady(notnull ChefZ_MatchResult result, notnull ChefZ_FactSnapshot snapshot, notnull ChefZ_CookContext ctx, out string reason)
    {
        reason = "";

        if (!result.matched || !result.recipe)
        {
            reason = "kein gebundenes Rezept";
            return false;
        }

        return ChefZ_RecipeEvaluator.CheckReady(result.recipe, result, snapshot, ctx, reason);
    }

    /**
     * Was fehlt einem BESTIMMTEN Rezept hier noch (08 §3)?
     *
     * Fuer "chefz why" (18 §3) und das Cookbook ab V1.1. Nie im Kochtick.
     */
    bool EvaluatePartial(notnull ChefZ_CookContext ctx, notnull ChefZ_FactSnapshot snapshot, ChefZ_Sym recipeSym, out ChefZ_PartialMatchReport report)
    {
        if (!report)
            report = new ChefZ_PartialMatchReport();
        report.Reset();

        ChefZ_CompiledRecipe rec = FindRecipe(recipeSym);
        if (!rec)
        {
            report.recipeId      = ChefZ_SymbolTable.NameOrMark(recipeSym);
            report.contextReason = "Rezept ist nicht geladen";
            return false;
        }

        ChefZ_RecipeEvaluator.BuildPartial(rec, ctx, snapshot, report);
        return true;
    }

    //==========================================================================
    // Diagnose (18)
    //==========================================================================

    void DumpRecipes(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("ChefZ Recipe Engine  bereit=" + m_Ready.ToString() + "  rezepte=" + m_Recipes.Count().ToString() + "  budget=" + m_NodeBudget.ToString());

        for (int i = 0; i < m_Recipes.Count(); i++)
            outLines.Insert("  " + (i + 1).ToString() + ". " + m_Recipes.Get(i).ToDebugString());
    }

    /**
     * Die Ambiguitaetsanalyse ERNEUT laufen lassen (18 §2.4,
     * "DumpAmbiguities").
     *
     * Beim Boot schreibt Build() dieselbe Analyse in den Ladebericht. Dort
     * geht sie in hunderten Zeilen unter, und genau deshalb gibt es dieses
     * Kommando: ein Content-Autor, der wissen will, warum sein neues Rezept
     * nie zuendet, soll die Antwort auf Zuruf bekommen und nicht im
     * Startprotokoll suchen muessen.
     *
     * Der Lauf ist rein lesend: er bekommt einen EIGENEN, leeren Bericht, der
     * mit dieser Funktion endet. Der Ladebericht des Boots bleibt unberuehrt -
     * seine Fehlerzahl speist die Safe-Mode-Schwelle (18 §4) und darf von
     * einem Diagnosekommando niemals verschoben werden.
     *
     * Die Analyse ist quadratisch und deshalb ueber MAX_PAIRWISE_RECIPES
     * gedeckelt; sie meldet selbst, wenn sie deswegen aussetzt.
     */
    void DumpAmbiguities(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Ambiguitaetsanalyse ueber " + m_Recipes.Count().ToString() + " Rezepte (heuristisch, 09 E5 - sie vergleicht Slotmengen und " + "Geraetemengen, keine Selektorsemantik).");

        if (!m_Ready)
        {
            outLines.Insert("  Die Rezept-Engine ist nicht gebaut. Nichts zu vergleichen.");
            return;
        }

        ChefZ_LoadReport scratch = new ChefZ_LoadReport();
        scratch.SetMirrorToLog(false);
        ChefZ_RecipeRanker.ReportAmbiguities(m_Recipes, m_Ranks, scratch);

        if (scratch.Count() == 0)
        {
            outLines.Insert("  Keine. Kein Rezept verdeckt ein anderes, und kein Paar " + "teilt sich Rang und Slotmenge.");
            return;
        }

        // Ueber eine lokale Zwischenvariable: einen out-Parameter als
        // out-Parameter weiterzureichen ist in Enforce nirgends zugesichert
        // (siehe Kopf von ChefZ_TextList.SymbolsOf).
        array<string> lines = outLines;
        scratch.ToLines(lines);
        outLines = lines;
    }

    //! Die Rangreihenfolge im Klartext (09 §3, ExplainOrder).
    void ExplainOrder(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        array<int> all = new array<int>();
        for (int i = 0; i < m_Ranks.Count(); i++)
            all.Insert(i);

        ChefZ_RecipeRanker.ExplainOrder(m_Ranks, all, outLines);
    }
}
