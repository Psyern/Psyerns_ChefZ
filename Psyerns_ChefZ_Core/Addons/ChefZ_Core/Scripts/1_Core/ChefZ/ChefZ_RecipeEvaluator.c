//==============================================================================
// ChefZ_RecipeEvaluator - EIN Rezept gegen EINEN Gefaessinhalt
//
// Entwurf: 08 §4 (Schritte 2a bis 2f woertlich), 08 §3 (ChefZ_Matcher.MatchOne),
// 08 E2 / V-B §3 (extraItems), 10 §6 (Abschlussbedingung), 07 §4 (die Bindung
// selbst macht ChefZ_Matcher), 08 §8 (kein Fehlerpfad veraendert etwas).
//
// ---------------------------------------------------------------------------
// Was hier NICHT geschieht
// ---------------------------------------------------------------------------
// Keine Auswahl. Diese Klasse kennt genau ein Rezept und beantwortet die
// Frage "bindet es hier". WELCHES Rezept gefragt wird und in welcher
// Reihenfolge, entscheidet die ChefZ_RecipeEngine (3_Game) - 08 §3 trennt
// beides ausdruecklich in "1_Core: reine Datenverarbeitung" und "3_Game:
// Index, Reihenfolge, Auswahl".
//
// Kein Score. Die Bewertung braucht die Gewichte aus Core.json und gehoert
// deshalb zum ChefZ_RecipeRanker. Hier werden nur die ZAHLEN gefuellt, die er
// dafuer braucht (boundItemCount, itemsInVessel, requiredSlots, priority).
//
// Keine Seiteneffekte. Es gibt in dieser Datei keinen Zugriff auf ein
// ItemBase, keinen Zufall und keine Zeitquelle. Zweimal dieselbe Eingabe
// ergibt zweimal dasselbe Ergebnis (08 §7).
//
// ---------------------------------------------------------------------------
// Eine Auslegung, die ausgesprochen gehoert: policy.forbiddenStates
// ---------------------------------------------------------------------------
// 08 §2 fuehrt forbiddenStates unter "WAS darf sonst noch drin sein". Geprueft
// wird deshalb der GANZE Gefaessinhalt, nicht nur die gebundenen Zutaten.
//
// Das ist die einzige Lesart, unter der das Feld etwas tut, was der Slot nicht
// schon tut: excludeStates am Slot (07 E5) haelt verdorbene Zutaten bereits
// aus jedem Slot heraus. Waere forbiddenStates auf gebundene Items beschraenkt,
// waere es eine Verdopplung ohne Wirkung.
//
// Ausserdem ist es die sichere Richtung: ein verrottetes Stueck Fleisch neben
// den Zutaten heisst dann "kein ChefZ-Rezept", und Vanilla kocht weiter
// (Invariante I2).
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_RecipeEvaluator
{
    /**
     * Die Schritte 2a bis 2f aus 08 §4 fuer genau ein Rezept.
     *
     * result wird IMMER gefuellt - auch bei false. Dann tragen failedRecipe,
     * failReason und failSlotId, woran es lag; das ist die Grundlage von
     * "chefz why" (18 §3) und der einzige Weg, aus einem "matcht nicht" eine
     * Auskunft zu machen, die ein Spieler versteht.
     *
     * Rueckgabe false heisst "dieses Rezept bindet hier nicht" und niemals
     * mehr. Es gibt keinen Rueckgabewert fuer "Fehler" - ein Fehler waere ein
     * Grund, es NICHT anzuwenden, und genau das ist false.
     */
    static bool Evaluate(notnull ChefZ_CompiledRecipe recipe,
                         notnull ChefZ_CookContext ctx,
                         notnull ChefZ_FactSnapshot snapshot,
                         int nodeBudget,
                         ChefZ_MatchTrace trace,
                         notnull ChefZ_MatchResult result)
    {
        result.Reset();
        result.failedRecipe  = recipe.recipeSym;
        result.recipeSym     = recipe.recipeSym;
        result.recipeId      = recipe.id;
        result.itemsInVessel = snapshot.Count();

        if (trace)
            trace.RecipeConsidered(recipe.recipeSym, recipe.specificity);

        // --- 2a Kontextfilter ------------------------------------------------
        string contextReason;
        if (!recipe.MatchesContext(ctx, contextReason))
        {
            result.failReason = contextReason;
            if (trace)
                trace.RecipeRejected(recipe.recipeSym, contextReason, "");
            return false;
        }

        // --- 2b Werkzeugfilter -----------------------------------------------
        string missingTool;
        if (!recipe.HasRequiredTools(ctx, missingTool))
        {
            result.failReason = "Werkzeuggruppe \"" + missingTool + "\" ist nicht in Reichweite";
            if (trace)
                trace.RecipeRejected(recipe.recipeSym, result.failReason, "");
            return false;
        }

        // --- 2c Capability-Filter (17) ---------------------------------------
        //
        // Seit S13 belegt. Der Filter selbst liegt in 3_Game
        // (ChefZ_CapabilityRegistry), weil er Anbieter aus fremden PBOs fuehrt;
        // hier steht nur die Frage. ChefZ_CapabilityGate ist die Umkehrung der
        // Abhaengigkeit - 1_Core sagt WO gefragt wird, 3_Game haengt sich ein.
        //
        // Ohne eingehaengte Registry und ohne Anbieter antwortet Denies()
        // immer false: der Ablauf verhaelt sich dann exakt wie bis S12, und
        // ein Server ohne Skillmodul ist voll spielbar (17 §3.3, 12 E8).
        //
        // NUR onFail "block" landet hier. "degrade" und "reduceYield" sind
        // keine Filter, sondern Abwertungen und wirken spaeter - sonst
        // verschwaende ein Rezept, das der Spieler bekommen soll, nur
        // schlechter.
        string capReason;
        if (ChefZ_CapabilityGate.Denies(recipe.requires, ctx.actorIdentityId, capReason))
        {
            result.failReason = capReason;
            if (trace)
                trace.RecipeRejected(recipe.recipeSym, capReason, "");
            return false;
        }

        // --- 2d Slot-Bindung (07 §4) -----------------------------------------
        ChefZ_BindResult bind;
        if (!ChefZ_Matcher.Bind(recipe.slots, snapshot, nodeBudget, recipe.id, trace, bind))
        {
            result.failReason    = bind.failReason;
            result.failSlotId    = bind.failSlotId;
            result.nodesExplored = bind.nodesExplored;
            if (trace)
                trace.RecipeRejected(recipe.recipeSym, bind.failReason, bind.failSlotId);
            return false;
        }
        result.nodesExplored = bind.nodesExplored;

        // --- 2e Policy --------------------------------------------------------
        string policyReason;
        if (!CheckPolicy(recipe, snapshot, bind, policyReason))
        {
            result.failReason = policyReason;
            if (trace)
                trace.RecipeRejected(recipe.recipeSym, policyReason, "");
            return false;
        }

        // --- 2f Ergebnis zusammensetzen ---------------------------------------
        Fill(recipe, snapshot, bind, result);
        result.matched      = true;
        result.failedRecipe = ChefZ_SymbolTable.INVALID;
        result.failReason   = "";
        return true;
    }

    //==========================================================================
    // 2e - Policy
    //==========================================================================

    private static bool CheckPolicy(notnull ChefZ_CompiledRecipe recipe,
                                    notnull ChefZ_FactSnapshot snapshot,
                                    notnull ChefZ_BindResult bind,
                                    out string reason)
    {
        reason = "";
        ChefZ_CompiledPolicy policy = recipe.policy;
        if (!policy)
            return true;

        int i;

        // 1. Verbotene Zustaende, ueber den GANZEN Inhalt (siehe Kopf).
        if (policy.forbiddenStates.Count() > 0)
        {
            for (i = 0; i < snapshot.Count(); i++)
            {
                ChefZ_ItemFacts facts = snapshot.Get(i);
                if (!facts)
                    continue;
                if (policy.IsStateForbidden(facts.chefzState))
                {
                    reason = ChefZ_SymbolTable.NameOrMark(facts.classSym)
                           + " ist im Zustand " + ChefZ_SymbolTable.NameOrMark(facts.chefzState)
                           + ", den dieses Rezept ausschliesst";
                    return false;
                }
            }
        }

        // 2. Mindestgesundheit der GEBUNDENEN Zutaten. Hier bewusst nur die
        //    gebundenen: es ist eine Aussage ueber das, was ins Gericht geht.
        if (policy.minMatchedHealth01 > 0.0)
        {
            for (i = 0; i < bind.boundHandles.Count(); i++)
            {
                ChefZ_ItemFacts facts = snapshot.FindByHandle(bind.boundHandles.Get(i));
                if (!facts)
                    continue;
                if (facts.health01 < policy.minMatchedHealth01)
                {
                    reason = ChefZ_SymbolTable.NameOrMark(facts.classSym)
                           + " ist zu beschaedigt (" + facts.health01.ToString()
                           + " < " + policy.minMatchedHealth01.ToString() + ")";
                    return false;
                }
            }
        }

        // 3. Fremdkoerper (08 E2).
        if (policy.extraItemsMode != ChefZ_ExtraItemsMode.FORBID)
            return true;

        for (i = 0; i < bind.extraHandles.Count(); i++)
        {
            ChefZ_ItemFacts extra = snapshot.FindByHandle(bind.extraHandles.Get(i));
            if (!extra)
                continue;

            // Das Ventil aus 08 E2: geduldete Fremdkoerper. Es wird VOR
            // "forbid" ausgewertet (V-B §3, Folge 2).
            if (policy.extraItemsAllowedIf && policy.extraItemsAllowedIf.Test(extra))
                continue;

            reason = ChefZ_SymbolTable.NameOrMark(extra.classSym)
                   + " liegt zusaetzlich im Gefaess und wird von diesem Rezept nicht "
                   + "geduldet - gekocht wird Vanilla";
            return false;
        }

        return true;
    }

    //==========================================================================
    // 2f - Ergebnis
    //==========================================================================

    private static void Fill(notnull ChefZ_CompiledRecipe recipe,
                             notnull ChefZ_FactSnapshot snapshot,
                             notnull ChefZ_BindResult bind,
                             notnull ChefZ_MatchResult result)
    {
        result.recipe        = recipe;
        result.priority      = recipe.priority;
        result.requiredSlots = recipe.requiredSlots;
        result.gradeScore    = bind.gradePoints + recipe.qualityBias;

        // qualityTier bleibt INVALID: sie zu setzen ist Sache des Quality
        // Managers (12, S10). Eine hier geratene Stufe waere eine zweite
        // Wahrheit neben seiner Stufenleiter.

        int i;

        for (i = 0; i < bind.bindings.Count(); i++)
        {
            ChefZ_SlotBinding binding = bind.bindings.Get(i);
            if (!binding)
                continue;

            result.SetAssignment(binding.slotId, binding.handles);

            if (!binding.filled)
                continue;

            ChefZ_CompiledSlot slot = recipe.FindSlot(binding.slotId);
            if (slot && !ChefZ_CompiledRecipe.IsRequiredSlot(slot))
                result.matchedOptionalSlots.Insert(binding.slotIndex);
        }

        for (i = 0; i < bind.boundHandles.Count(); i++)
            result.boundHandles.Insert(bind.boundHandles.Get(i));
        result.boundItemCount = result.boundHandles.Count();

        for (i = 0; i < bind.extraHandles.Count(); i++)
            result.extraHandles.Insert(bind.extraHandles.Get(i));

        for (i = 0; i < bind.consumePlan.Count(); i++)
            result.consumePlan.Insert(bind.consumePlan.Get(i));

        // extraItems "consume": die Fremdkoerper wandern in den Verbrauchsplan
        // (08 §2). Sie zaehlen ausdruecklich NICHT zur Abdeckung - sonst
        // schluege ein nachlaessiges Rezept mit "consume" ein genaues, nur
        // weil es alles wegraeumt (09 E4).
        if (recipe.policy && recipe.policy.extraItemsMode == ChefZ_ExtraItemsMode.CONSUME)
        {
            for (i = 0; i < bind.extraHandles.Count(); i++)
            {
                ChefZ_ConsumePlan plan = new ChefZ_ConsumePlan();
                plan.handle       = bind.extraHandles.Get(i);
                plan.destroyWhole = true;
                plan.slotIndex    = -1;
                result.consumePlan.Insert(plan);
            }
        }
    }

    //==========================================================================
    // Abschlussbedingung (10 §6)
    //==========================================================================

    /**
     * Ist ein bereits gebundenes Ergebnis fertig?
     *
     * Getrennt von Evaluate(), weil der Adapter genau das pro Tick tut: einmal
     * binden, danach nur noch fragen, ob es so weit ist (10 §5, Stufe C).
     *
     * ON_STAGE ist der Standard und braucht NULL eigene Zeitmessung: Vanillas
     * FoodStage der Zutaten ist zugleich Bereitschaftssignal und persistierte
     * Fortschrittsanzeige (08 E5). Verbranntes Essen wird damit zur
     * natuerlichen Fehlerbedingung, statt nachgebaut zu werden.
     */
    static bool CheckReady(notnull ChefZ_CompiledRecipe recipe,
                           notnull ChefZ_MatchResult result,
                           notnull ChefZ_FactSnapshot snapshot,
                           notnull ChefZ_CookContext ctx,
                           out string reason)
    {
        reason = "";

        // if-Kette und kein switch: ChefZ_Completion.* sind "static const int"
        // einer anderen Klasse, und ein switch-case verlangt in Enforce eine
        // Konstante, deren Auswertbarkeit dort nirgends zugesichert ist.
        if (recipe.completion == ChefZ_Completion.INSTANT)
            return true;

        if (recipe.completion == ChefZ_Completion.TIMED)
        {
            if (ctx.deviceTemperature < recipe.minTemperature)
            {
                reason = "zu kalt (" + ctx.deviceTemperature.ToString()
                       + " < " + recipe.minTemperature.ToString() + ")";
                return false;
            }
            if (ctx.elapsedSec < recipe.cookSeconds)
            {
                reason = "noch " + (recipe.cookSeconds - ctx.elapsedSec).ToString() + "s";
                return false;
            }
            return true;
        }

        return CheckStages(recipe, result, snapshot, reason);
    }

    /**
     * ON_STAGE: JEDE gebundene Pflichtzutat muss in einer erlaubten
     * Vanilla-Endstufe stehen.
     *
     * Optionale Slots zaehlen ausdruecklich nicht mit. Ein Gewuerz hat keine
     * FoodStage, und ein Rezept am Fertigwerden zu hindern, weil eine Prise
     * Salz nicht "gekocht" ist, waere ein Fehler, den niemand als solchen
     * erkennt - das Gericht kaeme einfach nie.
     */
    private static bool CheckStages(notnull ChefZ_CompiledRecipe recipe,
                                    notnull ChefZ_MatchResult result,
                                    notnull ChefZ_FactSnapshot snapshot,
                                    out string reason)
    {
        reason = "";

        if (recipe.doneStages.Count() == 0)
        {
            // Kann nicht vorkommen: der Compiler setzt den Default und meldet
            // ihn (08 §8). Die zweite Sicherung sagt NEIN - ein Rezept ohne
            // Endstufe darf nicht dauernd fertig sein.
            reason = "keine doneStages";
            return false;
        }

        for (int s = 0; s < recipe.slots.Count(); s++)
        {
            ChefZ_CompiledSlot slot = recipe.slots.Get(s);
            if (!ChefZ_CompiledRecipe.IsRequiredSlot(slot))
                continue;

            array<int> handles = result.GetAssignment(slot.slotId);
            if (!handles)
                continue;

            for (int h = 0; h < handles.Count(); h++)
            {
                ChefZ_ItemFacts facts = snapshot.FindByHandle(handles.Get(h));
                if (!facts)
                {
                    // Zutat ist zwischen Bindung und Pruefung verschwunden.
                    // Nicht fertig - und der Adapter matcht im naechsten Tick
                    // ohnehin neu, weil sich die Signatur geaendert hat.
                    reason = "eine gebundene Zutat ist nicht mehr da";
                    return false;
                }

                if (!recipe.AcceptsDoneStage(facts.vanillaFoodStage))
                {
                    reason = ChefZ_SymbolTable.NameOrMark(facts.classSym) + " ist "
                           + ChefZ_VanillaStage.Name(facts.vanillaFoodStage)
                           + ", gebraucht wird " + StagesToString(recipe);
                    return false;
                }
            }
        }

        return true;
    }

    private static string StagesToString(notnull ChefZ_CompiledRecipe recipe)
    {
        string s = "";
        for (int i = 0; i < recipe.doneStages.Count(); i++)
        {
            if (i > 0)
                s = s + "/";
            s = s + ChefZ_VanillaStage.Name(recipe.doneStages.Get(i));
        }
        return s;
    }

    //==========================================================================
    // Teilbericht (08 §3, EvaluatePartial)
    //==========================================================================

    /**
     * Warum bindet dieses Rezept hier nicht?
     *
     * Betrachtet jeden Pflichtslot FUER SICH, ohne Zuordnung. Das ist bewusst
     * schwaecher als der Matcher und deshalb im Bericht ausdruecklich
     * vermerkt (ChefZ_PartialMatchReport.ToLines): zwei Slots, die sich
     * dasselbe Item teilen wollen, sehen hier beide erfuellt aus.
     *
     * Der Grund fuer die schwaechere Analyse ist ihr Zweck. Sie beantwortet
     * "was fehlt mir noch", und darauf ist "dir fehlt ein Pilz" die richtige
     * Antwort - nicht "es gibt keine gueltige Gesamtzuordnung".
     */
    static void BuildPartial(notnull ChefZ_CompiledRecipe recipe,
                             notnull ChefZ_CookContext ctx,
                             notnull ChefZ_FactSnapshot snapshot,
                             notnull ChefZ_PartialMatchReport report)
    {
        report.Reset();
        report.recipeSym = recipe.recipeSym;
        report.recipeId  = recipe.id;

        string contextReason;
        report.contextOk     = recipe.MatchesContext(ctx, contextReason);
        report.contextReason = contextReason;
        if (!report.contextOk)
            return;

        string missingTool;
        report.toolsOk     = recipe.HasRequiredTools(ctx, missingTool);
        report.missingTool = missingTool;

        snapshot.ClearBindings();

        for (int s = 0; s < recipe.slots.Count(); s++)
        {
            ChefZ_CompiledSlot slot = recipe.slots.Get(s);
            if (!ChefZ_CompiledRecipe.IsRequiredSlot(slot))
                continue;

            array<int> candidates = new array<int>();
            ChefZ_SlotEvaluator.CollectCandidateIndices(slot, snapshot, candidates);

            ChefZ_SlotShortfall shortfall = new ChefZ_SlotShortfall();
            shortfall.slotId    = slot.slotId;
            shortfall.have      = candidates.Count();
            shortfall.need      = slot.minCount;
            shortfall.satisfied = candidates.Count() >= slot.minCount;

            if (!shortfall.satisfied)
                shortfall.reason = FirstMissReason(slot, snapshot);

            report.slots.Insert(shortfall);
        }
    }

    //! Warum hat das erste Item des Gefaesses diesen Slot nicht bedient?
    //! Dieselbe Auskunft wie im Matcher-Trace (07 E6), nur auf Anforderung.
    private static string FirstMissReason(notnull ChefZ_CompiledSlot slot,
                                          notnull ChefZ_FactSnapshot snapshot)
    {
        for (int i = 0; i < snapshot.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.Get(i);
            if (!facts)
                continue;

            string reason;
            if (ChefZ_SlotEvaluator.AcceptsExplain(slot, facts, reason))
                continue;

            return ChefZ_SymbolTable.NameOrMark(facts.classSym) + ": " + reason;
        }
        return "Gefaess ist leer";
    }
}
