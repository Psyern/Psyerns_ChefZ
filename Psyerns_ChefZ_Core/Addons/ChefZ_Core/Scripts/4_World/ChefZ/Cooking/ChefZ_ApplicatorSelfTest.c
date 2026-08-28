//==============================================================================
// ChefZ_ApplicatorSelfTest - was sich ohne Topf und ohne Feuer pruefen laesst
//
// Entwurf: 19 S8 (die drei Negativtests sind Abnahmebedingung), 08 §6 (die
// Reihenfolge), 08 §8 (Fehlerverhalten), 16 §7 (fehlender Behaelter),
// 18 §2 (Selbsttests melden sich beim Boot).
//
// ---------------------------------------------------------------------------
// Was hier geprueft wird - und was der Server pruefen muss
// ---------------------------------------------------------------------------
// Die drei Negativtests aus 19 S8 lauten:
//
//   N1  voller Cargo             -> nichts verbraucht, nichts erzeugt
//   N2  Zutat zwischenzeitlich entfernt -> nichts verbraucht, nichts erzeugt
//   N3  fehlende Ergebnisklasse  -> nichts verbraucht, nichts erzeugt
//
// N2 und N3 sind hier vollstaendig pruefbar, weil beide VOR jeder Beruehrung
// eines Items entschieden werden: N2 in ValidateHandles, N3 in PlanOutputs.
// Beide Pruefungen brauchen kein Item - sie brauchen nur die Auskunft, dass
// eines fehlt.
//
// N1 ist es NICHT: "kein Platz im Gefaess" ist eine Frage an ein echtes
// Inventar. Der Test dafuer bleibt der Servertest aus 19 S8 - Topf randvoll,
// Rezept erfuellt, danach zaehlen: keine Zutat weniger, kein Gericht mehr.
// Was hier stattdessen geprueft wird, ist die Stelle davor: dass die Planung
// ueberhaupt eine vollstaendige Liste dessen liefert, wofuer Platz gebraucht
// wird - denn eine Platzpruefung, die ein Ergebnis uebersieht, ginge im
// Servertest als bestanden durch.
//
// ---------------------------------------------------------------------------
// Warum kein Test den Erfolgsfall prueft
// ---------------------------------------------------------------------------
// Ein Erfolg heisst: Item erzeugt, Item verbraucht. Beides braucht eine Welt.
// Ein Selbsttest, der das nachstellte, pruefte seine eigene Nachstellung.
//
// Damit die Negativtests trotzdem etwas wert sind, prueft T1 ausdruecklich,
// dass ValidateHandles nicht schlicht immer false sagt: ein Ergebnis ohne
// Handles und ohne Verbrauchsplan muss durchgehen.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_ApplicatorSelfTest
{
    //! Testkennungen. Kein Content: weder Klasse noch Kategorie noch Gericht -
    //! nur Zeichenketten, die es nirgends sonst gibt.
    private static const string TEST_RECIPE    = "CHEFZ_APPLY_TESTREZEPT";
    private static const string TEST_MISSING   = "ChefZ_ApplyTestKlasseGibtEsNicht";
    private static const string TEST_CONTAINER = "CHEFZ_APPLY_TESTBEHAELTER";
    private static const string TEST_TIER      = "CHEFZ_APPLY_TESTSTUFE";

    private static int  s_Passed;
    private static int  s_Failed;
    private static bool s_Ran;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_Ran    = true;

        // Der Test spielt Fehlerfaelle durch. Ohne diese Klammer staenden sie
        // als echte Fehler im RPT und speisten die Safe-Mode-Schwelle.
        ChefZ_Applicator.SetQuietForTest(true);

        Check("Revalidierung: nichts zu pruefen",        TestValidateEmpty());
        Check("Revalidierung: Handle fehlt (N2)",        TestValidateMissingHandle());
        Check("Revalidierung: Verbrauchsziel fehlt (N2)", TestValidateMissingConsumeTarget());
        Check("Ergebnisklasse: Varianten",               TestVariants());
        Check("Ergebnisklasse fehlt (N3)",               TestMissingResultClass());
        Check("Behaelter gefordert",                     TestContainerRequired());
        Check("Planung: Liste vollstaendig (N1-Vorstufe)", TestPlanning());
        Check("Rollback leert die Liste",                TestRollback());
        Check("Verbrauch ohne Ziele bleibt folgenlos",   TestConsumeWithoutTargets());

        ChefZ_Applicator.SetQuietForTest(false);

        return s_Failed == 0;
    }

    //==========================================================================
    // T1 - die Gegenprobe: ValidateHandles sagt nicht immer nein
    //==========================================================================

    private static bool TestValidateEmpty()
    {
        ChefZ_MatchResult res = new ChefZ_MatchResult();
        res.matched = true;

        array<ItemBase> entities = new array<ItemBase>();

        return ChefZ_Applicator.ValidateHandles(res, entities);
    }

    //==========================================================================
    // T2 - N2: die Zutat ist zwischen Bindung und Anwendung verschwunden
    //==========================================================================

    private static bool TestValidateMissingHandle()
    {
        ChefZ_MatchResult res = new ChefZ_MatchResult();
        res.matched = true;
        res.boundHandles.Insert(0);

        // a) die Entity-Liste ist kuerzer als der Handle - das ist der Fall,
        //    in dem der Sammler das Item gar nicht mehr gefunden hat.
        array<ItemBase> shorter = new array<ItemBase>();
        if (ChefZ_Applicator.ValidateHandles(res, shorter))
            return false;

        // b) der Platz existiert, ist aber leer - der Fall, in dem die Engine
        //    das Objekt bereits freigegeben hat.
        array<ItemBase> withNull = new array<ItemBase>();
        withNull.Insert(null);
        if (ChefZ_Applicator.ValidateHandles(res, withNull))
            return false;

        // c) negative und absurde Handles fuehren nicht in einen Zugriff
        //    ausserhalb der Liste.
        ChefZ_MatchResult odd = new ChefZ_MatchResult();
        odd.matched = true;
        odd.boundHandles.Insert(-1);
        if (ChefZ_Applicator.ValidateHandles(odd, withNull))
            return false;

        odd.boundHandles.Clear();
        odd.boundHandles.Insert(99999);
        if (ChefZ_Applicator.ValidateHandles(odd, withNull))
            return false;

        return true;
    }

    /**
     * Derselbe Fall fuer den Verbrauchsplan.
     *
     * Er wird getrennt geprueft, weil der Plan Handles enthalten kann, die
     * NICHT in boundHandles stehen: bei policy extraItems "consume" wandern
     * die Fremdkoerper hinein (08 §2). Ein Applicator, der nur die gebundenen
     * Handles prueft, wuerde genau diese Items ungeprueft loeschen.
     */
    private static bool TestValidateMissingConsumeTarget()
    {
        ChefZ_MatchResult res = new ChefZ_MatchResult();
        res.matched = true;

        ChefZ_ConsumePlan plan = new ChefZ_ConsumePlan();
        plan.handle       = 3;
        plan.destroyWhole = true;
        res.consumePlan.Insert(plan);

        array<ItemBase> entities = new array<ItemBase>();
        entities.Insert(null);

        return !ChefZ_Applicator.ValidateHandles(res, entities);
    }

    //==========================================================================
    // T3 - Qualitaetsvarianten (12 §3)
    //==========================================================================

    private static bool TestVariants()
    {
        ChefZ_OutputDef def = new ChefZ_OutputDef();
        def.cls = TEST_MISSING;
        def.ResolveDefaults();

        // Ohne Stufe gewinnt immer die Grundklasse. Das ist der Zustand bis
        // zum Quality Manager (S10) - und der einzige, der nichts raet.
        if (ChefZ_Applicator.ResolveOutputClass(def, ChefZ_SymbolTable.INVALID) != TEST_MISSING)
            return false;

        ChefZ_OutputVariant variant = new ChefZ_OutputVariant();
        variant.tier = TEST_TIER;
        variant.cls  = TEST_MISSING + "Variante";

        def.variants = new array<ref ChefZ_OutputVariant>();
        def.variants.Insert(variant);

        ChefZ_Sym tier = ChefZ_SymbolTable.Intern(TEST_TIER);
        if (ChefZ_Applicator.ResolveOutputClass(def, tier) != TEST_MISSING + "Variante")
            return false;

        // Eine Stufe ohne passende Variante faellt auf die Grundklasse zurueck
        // und erfindet nichts.
        ChefZ_Sym other = ChefZ_SymbolTable.Intern(TEST_TIER + "_ANDERS");
        if (ChefZ_Applicator.ResolveOutputClass(def, other) != TEST_MISSING)
            return false;

        return true;
    }

    //==========================================================================
    // T4 - N3: die Ergebnisklasse gibt es nicht
    //==========================================================================

    private static bool TestMissingResultClass()
    {
        if (ChefZ_Applicator.ResultClassExists(""))
            return false;

        // Ohne laufendes Spiel ist die Frage nicht stellbar, und dann kann
        // dieser Teil nichts beweisen (siehe ChefZ_Applicator.ResultClassExists).
        if (g_Game && ChefZ_Applicator.ResultClassExists(TEST_MISSING))
            return false;

        if (!g_Game)
            return true;

        // Und die Planung bricht deshalb ab - VOR jedem Verbrauch und vor
        // jeder Erzeugung.
        ChefZ_CompiledRecipe recipe = BuildRecipe(TEST_MISSING);
        ChefZ_MatchResult    res    = BuildResult(recipe);

        array<ref ChefZ_PlannedOutput> planned = new array<ref ChefZ_PlannedOutput>();
        string why;

        if (ChefZ_Applicator.PlanOutputs(recipe, res, planned, why))
            return false;
        if (planned.Count() != 0)
            return false;
        if (why == "")
            return false;

        return true;
    }

    //==========================================================================
    // T5 - Behaelter gefordert, Behaeltersystem noch nicht da (16 §7)
    //==========================================================================

    private static bool TestContainerRequired()
    {
        string existing = AnyExistingClass();
        if (existing == "")
            return true;                    // ohne Spiel nicht pruefbar

        ChefZ_CompiledRecipe recipe = BuildRecipe(existing);
        recipe.outputs.Get(0).containerCategory = TEST_CONTAINER;

        ChefZ_MatchResult res = BuildResult(recipe);

        array<ref ChefZ_PlannedOutput> planned = new array<ref ChefZ_PlannedOutput>();
        string why;

        // 16 §7: "Kein Behaelter im Zugriff -> Ausfuehrung wird abgebrochen.
        // KEIN Zutatenverbrauch, kein Gericht."
        if (ChefZ_Applicator.PlanOutputs(recipe, res, planned, why))
            return false;
        if (planned.Count() != 0)
            return false;

        return true;
    }

    //==========================================================================
    // T6 - die Planung liefert genau das, wofuer Platz gebraucht wird
    //==========================================================================

    /**
     * Die Vorstufe zu N1.
     *
     * Die Platzpruefung selbst braucht ein Inventar und bleibt dem Servertest
     * vorbehalten. Was hier geprueft wird, ist ihre Eingabe: die Liste der
     * Ergebnisse muss vollstaendig sein und die Zufallsentscheidung fuer
     * Nebenprodukte muss GENAU EINMAL fallen - sonst prueft der Applicator den
     * Platz fuer eine andere Menge, als er anschliessend erzeugt.
     */
    private static bool TestPlanning()
    {
        string existing = AnyExistingClass();
        if (existing == "")
            return true;                    // ohne Spiel nicht pruefbar

        ChefZ_CompiledRecipe recipe = BuildRecipe(existing);
        ChefZ_MatchResult    res    = BuildResult(recipe);

        array<ref ChefZ_PlannedOutput> planned = new array<ref ChefZ_PlannedOutput>();
        string why;

        if (!ChefZ_Applicator.PlanOutputs(recipe, res, planned, why))
            return false;
        if (planned.Count() != 1)
            return false;
        if (planned.Get(0).cls != existing)
            return false;
        if (planned.Get(0).byproduct)
            return false;
        if (planned.Get(0).created)
            return false;               // geplant heisst NICHT erzeugt

        // Ein Nebenprodukt, das nie faellt, taucht nicht in der Platzpruefung
        // auf.
        ChefZ_OutputDef never = new ChefZ_OutputDef();
        never.cls = existing;
        never.ResolveDefaults();
        never.chance = 0.0;
        recipe.byproducts.Insert(never);

        if (!ChefZ_Applicator.PlanOutputs(recipe, res, planned, why))
            return false;
        if (planned.Count() != 1)
            return false;

        // Eines, das immer faellt, dagegen schon - und es ist als
        // Nebenprodukt gekennzeichnet.
        ChefZ_OutputDef always = new ChefZ_OutputDef();
        always.cls = existing;
        always.ResolveDefaults();
        recipe.byproducts.Insert(always);

        if (!ChefZ_Applicator.PlanOutputs(recipe, res, planned, why))
            return false;
        if (planned.Count() != 2)
            return false;
        if (!planned.Get(1).byproduct)
            return false;

        return true;
    }

    //==========================================================================
    // T7 - Rollback
    //==========================================================================

    private static bool TestRollback()
    {
        array<ItemBase> created = new array<ItemBase>();
        created.Insert(null);
        created.Insert(null);

        ChefZ_Applicator.RollbackCreated(created);

        // Geleert, damit der Aufrufer nichts herausgibt, was gerade geloescht
        // wird. Nullen zaehlen nicht als geloescht.
        if (created.Count() != 0)
            return false;

        ChefZ_Applicator.RollbackCreated(created);      // zweimal ist harmlos
        return created.Count() == 0;
    }

    //==========================================================================
    // T8 - ConsumeInputs greift nicht ins Leere
    //==========================================================================

    private static bool TestConsumeWithoutTargets()
    {
        ChefZ_MatchResult res = new ChefZ_MatchResult();
        res.matched = true;

        ChefZ_ConsumePlan a = new ChefZ_ConsumePlan();
        a.handle       = 0;
        a.destroyWhole = true;
        res.consumePlan.Insert(a);

        ChefZ_ConsumePlan b = new ChefZ_ConsumePlan();
        b.handle        = 7;
        b.quantityDelta = 5.0;
        res.consumePlan.Insert(b);

        array<ItemBase> entities = new array<ItemBase>();
        entities.Insert(null);

        // Darf nicht abstuerzen. Der Fall kann nach der Revalidierung nicht
        // mehr auftreten - aber "kann nicht auftreten" ist genau die Sorte
        // Annahme, die in einem Kochmod Zutaten kostet.
        ChefZ_Applicator.ConsumeInputs(res, entities);
        return true;
    }

    //==========================================================================
    // Hilfsmittel
    //==========================================================================

    private static ChefZ_CompiledRecipe BuildRecipe(string cls)
    {
        ChefZ_CompiledRecipe recipe = new ChefZ_CompiledRecipe();
        recipe.id        = TEST_RECIPE;
        recipe.recipeSym = ChefZ_SymbolTable.Intern(TEST_RECIPE);

        ChefZ_OutputDef out0 = new ChefZ_OutputDef();
        out0.cls = cls;

        // Wie im Ladeweg: der Sondendurchgang setzt Sentinel, ResolveDefaults
        // ersetzt sie. Ohne diesen Aufruf waere chance ein Sentinel, und die
        // Planung liesse das Ergebnis aus - was der Applicator sicher, aber
        // ohne Ergebnis beantwortete.
        out0.ResolveDefaults();

        recipe.outputs.Insert(out0);
        return recipe;
    }

    private static ChefZ_MatchResult BuildResult(notnull ChefZ_CompiledRecipe recipe)
    {
        ChefZ_MatchResult res = new ChefZ_MatchResult();
        res.matched   = true;
        res.recipe    = recipe;
        res.recipeSym = recipe.recipeSym;
        res.recipeId  = recipe.id;
        return res;
    }

    /**
     * Irgendeine Klasse, die es in CfgVehicles wirklich gibt.
     *
     * Sie wird zur Laufzeit gesucht und nicht hingeschrieben: ein fester
     * Klassenname im Core waere Content (Invariante I3), und er waere zudem
     * eine Wette darauf, dass ein kuenftiges DayZ-Update ihn behaelt.
     *
     * Liefert "" ohne laufendes Spiel - die Tests, die sie brauchen, gelten
     * dann als nicht anwendbar statt als fehlgeschlagen.
     */
    private static string AnyExistingClass()
    {
        if (!g_Game)
            return "";

        int count = g_Game.ConfigGetChildrenCount("CfgVehicles");
        if (count > 64)
            count = 64;                     // die ersten paar genuegen

        for (int i = 0; i < count; i++)
        {
            string name;
            if (!g_Game.ConfigGetChildName("CfgVehicles", i, name))
                continue;
            if (name == "")
                continue;
            if (ChefZ_Applicator.ResultClassExists(name))
                return name;
        }

        return "";
    }

    //==========================================================================

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            return;
        }

        s_Failed++;

        // An der Klammer SetQuietForTest vorbei: dieser Fehler ist echt.
        ChefZ_Log.Error(ChefZ_LogChannel.COOK, "Selbsttest S8 fehlgeschlagen: " + name + ". Der Applicator ist damit nicht " + "vertrauenswuerdig - er ist die einzige Stelle des Core, die Zutaten " + "verbraucht. Vanilla-Kochen ist davon unberuehrt.");
    }

    static string Summary()
    {
        if (!s_Ran)
            return "Selbsttest S8 (Applicator): nicht gelaufen";

        return "Selbsttest S8 (Applicator): " + s_Passed.ToString() + " bestanden, "
             + s_Failed.ToString() + " fehlgeschlagen";
    }
}
