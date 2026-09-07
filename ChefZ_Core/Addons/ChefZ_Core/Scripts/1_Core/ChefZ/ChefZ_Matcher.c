//==============================================================================
// ChefZ_SlotBinding / ChefZ_BindResult / ChefZ_Matcher
//
// Entwurf: 07 §4 (der Zuordnungsalgorithmus, Schritt fuer Schritt), 07 §5
// (Laufzeitpfad), 07 §6 (zustandslos), 07 §7 (Knotenbudget), 07 E3, E4,
// 08 §3 (ChefZ_Matcher.MatchOne als Rezeptfassung, S6).
//
// ---------------------------------------------------------------------------
// Warum Backtracking und nicht Greedy
// ---------------------------------------------------------------------------
// Slots und Items sind ein bipartites ZUORDNUNGSPROBLEM, kein Abgleich
// (07 §4). Das Beispiel aus Architekturplan §16:
//
//     Gefaess: Venison, Potato, Tomato, Mushroom, Thyme
//     Rezept:  [WILD_MEAT x1] [VEGETABLE x2] [anyOf Thyme|Parsley x1]
//
// Bindet ein gieriger Matcher Slot 2 zuerst und greift dabei Venison (das
// ebenfalls in einer passenden Kategorie liegen kann), fehlt Slot 1 sein Item -
// obwohl eine gueltige Gesamtzuordnung existiert. Ab drei ueberlappenden Slots
// liefert Greedy FALSCHE NEGATIVE, und "Rezept passt nicht, obwohl es passt"
// ist der Fehler, den Spieler am ehesten bemerken und am schwersten melden
// koennen.
//
// Hopcroft-Karp waere asymptotisch besser, bildet aber Mengenanforderungen
// (amount > 1, Teilverbrauch aus Stapeln) nicht natuerlich ab. Bei den realen
// Groessen - Slots <= 8, Cargo <= 12 - ist Backtracking mit Constraint-
// Ordering nach wenigen Schritten fertig und um Groessenordnungen einfacher zu
// pruefen.
//
// ---------------------------------------------------------------------------
// Die drei Eigenschaften, auf die es ankommt
// ---------------------------------------------------------------------------
// 1. DETERMINISTISCH. Der Snapshot ist stabil sortiert (05 §3.3), die
//    Slotreihenfolge bei Gleichstand nach Deklarationsindex. Derselbe
//    Kesselinhalt liefert IMMER dieselbe Bindung - Voraussetzung dafuer, dass
//    ein Bugreport reproduzierbar ist.
//
// 2. EIN ITEM BEDIENT HOECHSTENS EINEN SLOT. Sonst koennte dasselbe Stueck
//    Fleisch MEAT und WILD_MEAT gleichzeitig erfuellen - ein
//    Duplikations-Exploit (07 §4).
//
// 3. GEDECKELT. Jeder Bindungsversuch zaehlt gegen matcherNodeBudget. Ein
//    pathologisches Content-Rezept kann den Server nicht anhalten, und der
//    Betreiber sieht im Log, WELCHES (07 §7).
//
// Nichts hier veraendert irgendetwas. Der schlimmste moegliche Ausgang ist
// "kein ChefZ-Rezept", und der bedeutet unveraendertes Vanilla.
//
// KEIN CONTENT: wo oben Namen wie Venison oder WILD_MEAT stehen, sind es
// woertliche Beispiele aus Entwurf 07 §4. Im Code steht keiner von ihnen -
// der Matcher sieht ausschliesslich Symbole und Bitindizes.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_SlotBinding : Managed
{
    int    slotIndex;           // Deklarationsindex im Rezept
    string slotId;
    ref array<int> handles;     // gebundene Items, in Snapshot-Reihenfolge
    float  totalUnits;
    bool   filled;
    int    gradePoints;

    void ChefZ_SlotBinding()
    {
        slotIndex   = -1;
        slotId      = "";
        handles     = new array<int>();
        totalUnits  = 0.0;
        filled      = false;
        gradePoints = 0;
    }

    int Count()
    {
        return handles.Count();
    }

    string ToDebugString()
    {
        string s = slotId + " [";
        for (int i = 0; i < handles.Count(); i++)
        {
            if (i > 0)
                s = s + ",";
            s = s + "#" + handles.Get(i).ToString();
        }
        s = s + "] einheiten=" + totalUnits.ToString();
        if (!filled)
            s = s + " (leer)";
        return s;
    }
}

//------------------------------------------------------------------------------

/**
 * Das Ergebnis EINER Bindung.
 *
 * Bewusst nicht ChefZ_MatchResult (08 §3): das ist die Rezeptfassung und
 * gehoert zu S6 - sie traegt Score, Qualitaetsstufe und Abschlussbedingung.
 * Hier steht nur, was die Slotbindung selbst weiss. S6 haengt seine Felder
 * darum herum, statt sie hier vorwegzunehmen.
 */
class ChefZ_BindResult : Managed
{
    bool matched;

    //! In DEKLARATIONSreihenfolge, nicht in Suchreihenfolge (07 §4,
    //! Schritt 6). Der Verbrauch muss stabil sein.
    ref array<ref ChefZ_SlotBinding> bindings;

    ref array<int> boundHandles;
    ref array<int> extraHandles;
    ref array<ref ChefZ_ConsumePlan> consumePlan;

    int gradePoints;

    //! Diagnose
    int    nodesExplored;
    bool   budgetExhausted;
    string failSlotId;
    string failReason;

    void ChefZ_BindResult()
    {
        bindings     = new array<ref ChefZ_SlotBinding>();
        boundHandles = new array<int>();
        extraHandles = new array<int>();
        consumePlan  = new array<ref ChefZ_ConsumePlan>();
        Reset();
    }

    void Reset()
    {
        matched         = false;
        bindings.Clear();
        boundHandles.Clear();
        extraHandles.Clear();
        consumePlan.Clear();
        gradePoints     = 0;
        nodesExplored   = 0;
        budgetExhausted = false;
        failSlotId      = "";
        failReason      = "";
    }

    ChefZ_SlotBinding FindBinding(string slotId)
    {
        for (int i = 0; i < bindings.Count(); i++)
        {
            if (bindings.Get(i).slotId == slotId)
                return bindings.Get(i);
        }
        return null;
    }

    int BoundItemCount()
    {
        return boundHandles.Count();
    }

    string ToDebugString()
    {
        string s = "matched=" + matched.ToString() + " knoten=" + nodesExplored.ToString();
        if (budgetExhausted)
            s = s + " BUDGET";
        if (!matched)
            s = s + " grund=" + failReason + " slot=" + failSlotId;
        for (int i = 0; i < bindings.Count(); i++)
            s = s + "  " + bindings.Get(i).ToDebugString();
        return s;
    }
}

//------------------------------------------------------------------------------

/**
 * Arbeitszustand EINER Bindung.
 *
 * Warum ein eigenes Objekt und keine statischen Felder im Matcher: 07 §6
 * verlangt Zustandsfreiheit zwischen zwei Aufrufen. Der Matcher wird pro Tick
 * potenziell mehrfach fuer verschiedene Rezepte gerufen; statische Puffer
 * waeren genau die Art von Kopplung, bei der ein Fehler nur bei bestimmter
 * Aufrufreihenfolge auftritt. Das Objekt lebt einen Aufruf lang, die Rekursion
 * traegt ihren Zustand darin, und danach ist es fort.
 */
class ChefZ_MatchWork
{
    ref array<ref ChefZ_CompiledSlot> slots;
    ChefZ_FactSnapshot snapshot;        // ohne ref: gehoert dem Aufrufer
    ChefZ_MatchTrace   trace;           // ohne ref: gehoert dem Aufrufer
    ChefZ_BindResult   result;          // ohne ref: gehoert dem Aufrufer

    //! Suchreihenfolge: Indizes in slots, am staerksten eingeschraenkt zuerst.
    ref array<int> order;

    //! Kandidatenliste je Eintrag in order, als Snapshot-POSITIONEN.
    ref array<ref array<int>> candidates;

    //! Zuweisung je Eintrag in order, waehrend der Suche.
    ref array<ref array<int>> assigned;

    int budget;
    int nodes;
    bool exhausted;

    void ChefZ_MatchWork()
    {
        order      = new array<int>();
        candidates = new array<ref array<int>>();
        assigned   = new array<ref array<int>>();
        budget     = 0;
        nodes      = 0;
        exhausted  = false;
    }

    //! true, solange noch Knoten uebrig sind. Jeder Aufruf verbraucht einen.
    bool Spend()
    {
        nodes++;
        if (nodes > budget)
        {
            exhausted = true;
            return false;
        }
        return true;
    }
}

//------------------------------------------------------------------------------

class ChefZ_Matcher
{
    //! Nur fuer den Selbsttest: unterdrueckt die Budget-Warnung. Der
    //! Budgetueberlauf wird im Selbsttest absichtlich herbeigefuehrt, und eine
    //! WARN-Zeile bei jedem Serverstart waere ein Fehlalarm - genau das, was
    //! ein Betreiber nach drei Wochen ignoriert.
    private static bool s_QuietForTest;

    static void SetQuietForTest(bool quiet)
    {
        s_QuietForTest = quiet;
    }

    /**
     * Bindet eine Slotliste an eine Faktenliste (07 §4).
     *
     * Der Snapshot MUSS stabil sortiert sein (ChefZ_FactSnapshot.SortStable);
     * der ChefZ_FactCollector tut das beim Erheben. Hier wird bewusst NICHT
     * nachsortiert: der Matcher ist rein lesend, und die Sicht des Aufrufers
     * umzuordnen waere eine Nebenwirkung - auch eine harmlose bleibt eine.
     *
     * subject ist die Rezept-ID (oder ein anderer sprechender Name). Sie steht
     * ausschliesslich in Meldungen; 07 §7 verlangt beim Budgetueberlauf
     * ausdruecklich, dass der Betreiber sieht, WELCHES Rezept es war.
     *
     * Rueckgabe false heisst "kein Treffer" und niemals mehr. Kein Fehlerfall
     * dieser Funktion veraendert irgendetwas.
     */
    static bool Bind(array<ref ChefZ_CompiledSlot> slots, ChefZ_FactSnapshot snapshot, int nodeBudget, string subject, ChefZ_MatchTrace trace, out ChefZ_BindResult result)
    {
        if (!result)
            result = new ChefZ_BindResult();
        result.Reset();

        if (!snapshot)
        {
            result.failReason = "keine Faktenliste";
            return false;
        }

        if (!slots || slots.Count() == 0)
        {
            result.failReason = "keine Slots";
            return false;
        }

        ChefZ_MatchWork work = new ChefZ_MatchWork();
        work.slots    = slots;
        work.snapshot = snapshot;
        work.trace    = trace;
        work.result   = result;
        work.budget   = nodeBudget;
        if (work.budget < 1)
            work.budget = 1;

        // Jede Bindung faengt mit einer leeren Belegung an. slotBoundTo ist ein
        // Arbeitsfeld INNERHALB eines Bind()-Aufrufs (07 §6) - was ein
        // vorheriger Aufruf hinterlassen hat, gilt nicht mehr.
        snapshot.ClearBindings();

        bool ok = Run(work, subject);

        result.nodesExplored   = work.nodes;
        result.budgetExhausted = work.exhausted;
        result.matched         = ok;

        // Der Snapshot gehoert dem Aufrufer und darf nach der Bindung nicht
        // halb markiert zurueckbleiben. Alles, was das Ergebnis braucht, steht
        // im ChefZ_BindResult.
        snapshot.ClearBindings();

        return ok;
    }

    //--------------------------------------------------------------------------

    /**
     * Die Rezeptfassung aus 08 §3 - seit S6.
     *
     * Bind() kennt nur Slots; MatchOne() kennt ein ganzes Rezept, also auch
     * Kontext, Werkzeug und Policy. Die Arbeit dazu steht bewusst NICHT hier,
     * sondern in ChefZ_RecipeEvaluator: dieser Datei gehoert der
     * Zuordnungsalgorithmus (07 §4), und ihn mit Rezeptregeln zu vermischen
     * haette aus zwei lesbaren Dateien eine unlesbare gemacht.
     *
     * Die Signatur steht trotzdem hier, weil 08 §3 sie hier nennt und weil ein
     * Aufrufer, der "den Matcher" sucht, ihn an dieser Stelle vermutet.
     */
    static bool MatchOne(notnull ChefZ_CompiledRecipe recipe, notnull ChefZ_CookContext ctx, notnull ChefZ_FactSnapshot snapshot, int nodeBudget, ChefZ_MatchTrace trace, out ChefZ_MatchResult result)
    {
        if (!result)
            result = new ChefZ_MatchResult();
        return ChefZ_RecipeEvaluator.Evaluate(recipe, ctx, snapshot, nodeBudget, trace, result);
    }

    //--------------------------------------------------------------------------

    private static bool Run(notnull ChefZ_MatchWork work, string subject)
    {
        if (!BuildCandidates(work))
            return false;

        SortOrder(work);

        // Erst NACH dem Sortieren: die Zuweisungslisten liegen parallel zu
        // order, und sie hier anzulegen erspart es, beim Sortieren eine dritte
        // Liste mitzuschieben. Waeren sie schon gefuellt, waere genau das eine
        // stille Fehlerquelle.
        for (int a = 0; a < work.order.Count(); a++)
            work.assigned.Insert(new array<int>());

        if (work.trace)
            work.trace.Note("Suchreihenfolge: " + OrderToString(work));

        if (!Solve(work, 0))
        {
            if (work.exhausted)
            {
                // 07 §7: Budget erschoepft -> kein Treffer, WARN mit Rezept-ID
                // und Slotzahl. Once(), weil der Matcher pro Tick laeuft und
                // ein pathologisches Rezept sonst das Log flutet.
                work.result.failReason = "Knotenbudget erschoepft";
                if (!s_QuietForTest)
                {
                    ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.MATCH, "match.budget." + subject, "Knotenbudget (" + work.budget.ToString() + ") beim Binden von \"" + subject + "\" erschoepft - " + work.slots.Count().ToString() + " Slots, " + work.snapshot.Count().ToString() + " Items. Der " + "Kandidat gilt als kein Treffer; gekocht wird Vanilla. Entweder " + "ist das Rezept zu unspezifisch oder matcherNodeBudget zu klein.");
                }
            }
            return false;
        }

        FillOptionalSlots(work);
        CollectResult(work);
        return true;
    }

    /**
     * Schritt 1 aus 07 §4: Kandidatenlisten je PFLICHT-Slot.
     *
     * Eine leere Liste beendet die Suche sofort - der billigste Ausschluss
     * zuerst. Bei 30 Kandidatenrezepten je Tick ist das der Unterschied
     * zwischen "ein Durchlauf ueber 8 Items" und "ein Suchbaum".
     */
    private static bool BuildCandidates(notnull ChefZ_MatchWork work)
    {
        for (int i = 0; i < work.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = work.slots.Get(i);
            if (!IsRequired(slot))
                continue;

            array<int> cand = new array<int>();
            ChefZ_SlotEvaluator.CollectCandidateIndices(slot, work.snapshot, cand);

            if (cand.Count() < slot.minCount)
            {
                work.result.failSlotId = slot.slotId;
                work.result.failReason = "nur " + cand.Count().ToString() + " passende Items, gebraucht werden " + slot.minCount.ToString();
                if (work.trace)
                    work.trace.SlotResult(ChefZ_SymbolTable.INVALID, slot.slotId, false, work.result.failReason + " - " + ExplainFirstMiss(slot, work.snapshot));
                return false;
            }

            work.order.Insert(i);
            work.candidates.Insert(cand);
        }

        if (work.order.Count() == 0)
        {
            // Ein Rezept ohne einen einzigen Pflichtslot wuerde bei jedem
            // Topfinhalt zuenden, auch bei einem leeren. Der Rezeptcompiler
            // (S6) faengt das ab; hier ist es die zweite Sicherung.
            work.result.failReason = "keine Pflichtslots";
            return false;
        }

        return true;
    }

    //! Pflicht ist ein Slot, der nicht optional ist UND mindestens ein Item
    //! verlangt. minCount 0 an einem Pflichtslot ist faktisch optional; der
    //! Compiler weist bereits darauf hin.
    private static bool IsRequired(notnull ChefZ_CompiledSlot slot)
    {
        return !slot.optional && slot.minCount > 0;
    }

    /**
     * Schritt 2 aus 07 §4: am staerksten eingeschraenkt zuerst.
     *
     * Primaer nach der TATSAECHLICHEN Kandidatenzahl, denn die kennt den
     * Topfinhalt. selectivityHint (07 E4) bricht Gleichstaende - er weiss aus
     * den Rueckwaertsindizes, welcher Slot grundsaetzlich enger ist. Zuletzt
     * der Deklarationsindex, damit die Ordnung total und die Sortierung
     * stabil ist.
     */
    private static void SortOrder(notnull ChefZ_MatchWork work)
    {
        for (int i = 1; i < work.order.Count(); i++)
        {
            int keySlot = work.order.Get(i);
            array<int> keyCand = work.candidates.Get(i);
            int j = i - 1;

            while (j >= 0 && PrecedesSlot(work, keySlot, keyCand.Count(), work.order.Get(j), work.candidates.Get(j).Count()))
            {
                work.order.Set(j + 1, work.order.Get(j));
                work.candidates.Set(j + 1, work.candidates.Get(j));
                j--;
            }

            work.order.Set(j + 1, keySlot);
            work.candidates.Set(j + 1, keyCand);
        }
    }

    private static bool PrecedesSlot(notnull ChefZ_MatchWork work, int slotA, int countA, int slotB, int countB)
    {
        if (countA != countB)
            return countA < countB;

        int hintA = work.slots.Get(slotA).selectivityHint;
        int hintB = work.slots.Get(slotB).selectivityHint;
        if (hintA != hintB)
            return hintA < hintB;

        return slotA < slotB;
    }

    //--------------------------------------------------------------------------
    // Schritt 3 aus 07 §4: rekursives Backtracking
    //--------------------------------------------------------------------------

    private static bool Solve(notnull ChefZ_MatchWork work, int position)
    {
        if (position >= work.order.Count())
            return true;                    // alle Pflichtslots bedient

        return Choose(work, position, 0);
    }

    /**
     * Waehlt Items fuer den Slot an Position "position", beginnend beim
     * Kandidaten "fromCandidate".
     *
     * Der Aufbau ist bewusst "erst pruefen, dann vertiefen":
     *
     *   1. Reicht die bisherige Auswahl? -> naechsten Slot versuchen
     *   2. Sonst ein weiteres Item aufnehmen und rekursiv weitersuchen
     *
     * Dadurch wird die KLEINSTE ausreichende Auswahl zuerst probiert. Ein Slot
     * "x1..x3" nimmt also ein Item, wenn eines reicht - und greift erst nach
     * einem zweiten, wenn die Gesamtzuordnung sonst scheitert.
     */
    private static bool Choose(notnull ChefZ_MatchWork work, int position, int fromCandidate)
    {
        if (!work.Spend())
            return false;

        int slotIdx = work.order.Get(position);
        ChefZ_CompiledSlot slot = work.slots.Get(slotIdx);
        array<int> chosen = work.assigned.Get(position);
        array<int> cand   = work.candidates.Get(position);

        // Bewusst geschachtelt statt mit && verkettet: CheckAmountIdx hat einen
        // out-Parameter, und ein Aufruf mit out-Parameter innerhalb eines
        // kurzschliessenden Ausdrucks ist in Enforce nirgends zugesichert.
        float units = 0.0;

        if (ChefZ_SlotEvaluator.CheckCounts(slot, chosen.Count()))
        {
            if (ChefZ_SlotEvaluator.CheckAmountIdx(slot, work.snapshot, chosen, units))
            {
                if (Solve(work, position + 1))
                    return true;
                if (work.exhausted)
                    return false;
            }
        }

        if (chosen.Count() >= slot.maxCount)
            return false;

        // Frueherkennung (07 §4, Schritt 3): liegt die Menge bereits ueber der
        // Obergrenze, macht jedes weitere Item es nur schlimmer.
        if (ChefZ_SlotEvaluator.ExceedsMaxAmount(slot, ChefZ_SlotEvaluator.SumUnits(work.snapshot, chosen)))
            return false;

        for (int p = fromCandidate; p < cand.Count(); p++)
        {
            int itemIdx = cand.Get(p);
            ChefZ_ItemFacts facts = work.snapshot.Get(itemIdx);
            if (!facts)
                continue;
            if (facts.slotBoundTo >= 0)
                continue;               // bedient bereits einen anderen Slot

            facts.slotBoundTo = slotIdx;
            chosen.Insert(itemIdx);

            if (Choose(work, position, p + 1))
                return true;

            chosen.Remove(chosen.Count() - 1);
            facts.slotBoundTo = -1;

            if (work.exhausted)
                return false;
        }

        return false;
    }

    /**
     * Schritt 5 aus 07 §4: optionale Slots zuletzt, greedy maximierend.
     *
     * Mehr Optionales heisst mehr Qualitaetspunkte (12) und ist NIE ein
     * Abbruchgrund. Deshalb kein Backtracking: was frei ist und passt, wird
     * genommen; was nicht reicht, bleibt leer.
     */
    private static void FillOptionalSlots(notnull ChefZ_MatchWork work)
    {
        for (int i = 0; i < work.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = work.slots.Get(i);
            if (IsRequired(slot))
                continue;

            array<int> cand = new array<int>();
            ChefZ_SlotEvaluator.CollectCandidateIndices(slot, work.snapshot, cand);

            array<int> take = new array<int>();
            float units = 0.0;

            for (int p = 0; p < cand.Count(); p++)
            {
                if (take.Count() >= slot.maxCount)
                    break;

                int itemIdx = cand.Get(p);
                ChefZ_ItemFacts facts = work.snapshot.Get(itemIdx);
                if (!facts || facts.slotBoundTo >= 0)
                    continue;

                take.Insert(itemIdx);

                // Ein Item, das die Obergrenze reissen wuerde, wird wieder
                // zurueckgelegt - sonst macht ein optionaler Slot die
                // Mengenbedingung kaputt, die er erfuellen sollte.
                if (ChefZ_SlotEvaluator.ExceedsMaxAmount(slot, ChefZ_SlotEvaluator.SumUnits(work.snapshot, take)))
                {
                    take.Remove(take.Count() - 1);
                    continue;
                }

                facts.slotBoundTo = i;
            }

            if (take.Count() == 0)
                continue;

            bool countsOk  = ChefZ_SlotEvaluator.CheckCounts(slot, take.Count());
            bool amountOk   = ChefZ_SlotEvaluator.CheckAmountIdx(slot, work.snapshot, take, units);

            if (!countsOk || !amountOk)
            {
                // Nicht erfuellbar: Belegung zuruecknehmen, Slot bleibt leer.
                // Ein halb gefuellter optionaler Slot waere schlimmer als ein
                // leerer - er haelt Items fest, die ein anderer brauchen kann.
                for (int r = 0; r < take.Count(); r++)
                {
                    ChefZ_ItemFacts releaseFacts = work.snapshot.Get(take.Get(r));
                    if (releaseFacts)
                        releaseFacts.slotBoundTo = -1;
                }
                continue;
            }

            work.order.Insert(i);
            work.candidates.Insert(cand);
            work.assigned.Insert(take);
        }
    }

    /**
     * Schritt 6 aus 07 §4: Bindung in DEKLARATIONSreihenfolge zurueckgeben,
     * nicht in Suchreihenfolge - der Verbrauch muss stabil sein.
     */
    private static void CollectResult(notnull ChefZ_MatchWork work)
    {
        ChefZ_BindResult result = work.result;

        for (int s = 0; s < work.slots.Count(); s++)
        {
            ChefZ_CompiledSlot slot = work.slots.Get(s);

            ChefZ_SlotBinding binding = new ChefZ_SlotBinding();
            binding.slotIndex = slot.slotIndex;
            binding.slotId    = slot.slotId;

            int position = work.order.Find(s);
            if (position >= 0)
            {
                array<int> chosen = work.assigned.Get(position);
                ChefZ_SlotEvaluator.IndicesToHandles(work.snapshot, chosen, binding.handles);
                binding.totalUnits = ChefZ_SlotEvaluator.SumUnits(work.snapshot, chosen);
                binding.filled     = chosen.Count() > 0;

                if (binding.filled)
                {
                    binding.gradePoints = slot.gradePoints;
                    result.gradePoints  = result.gradePoints + slot.gradePoints;

                    ChefZ_SlotEvaluator.BuildConsumePlanIdx(slot, work.snapshot, chosen, result.consumePlan);

                    for (int h = 0; h < binding.handles.Count(); h++)
                        result.boundHandles.Insert(binding.handles.Get(h));

                    if (work.trace)
                    {
                        ChefZ_ItemFacts first = work.snapshot.Get(chosen.Get(0));
                        ChefZ_Sym firstClass = ChefZ_SymbolTable.INVALID;
                        if (first)
                            firstClass = first.classSym;
                        work.trace.SlotAssigned(slot.slotId, firstClass, binding.handles.Count(), binding.totalUnits);
                    }
                }
            }

            result.bindings.Insert(binding);
        }

        // Schritt 4 aus 07 §4: was uebrig bleibt, geht an die Policy des
        // Rezepts (extraItems, S6). Der Matcher entscheidet daraus nichts.
        for (int i = 0; i < work.snapshot.Count(); i++)
        {
            ChefZ_ItemFacts facts = work.snapshot.Get(i);
            if (facts && facts.slotBoundTo < 0)
                result.extraHandles.Insert(facts.handle);
        }
    }

    //--------------------------------------------------------------------------
    // Diagnose
    //--------------------------------------------------------------------------

    /**
     * Warum hat das erste Item des Snapshots diesen Slot nicht bedient?
     *
     * Nur fuer den Trace. Die Zeichenketten entstehen ausschliesslich hier,
     * und diese Funktion wird nur hinter "if (trace)" gerufen (07 E6).
     */
    private static string ExplainFirstMiss(notnull ChefZ_CompiledSlot slot, notnull ChefZ_FactSnapshot snapshot)
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

    private static string OrderToString(notnull ChefZ_MatchWork work)
    {
        string s = "";
        for (int i = 0; i < work.order.Count(); i++)
        {
            if (i > 0)
                s = s + " -> ";
            ChefZ_CompiledSlot slot = work.slots.Get(work.order.Get(i));
            s = s + slot.slotId + "(" + work.candidates.Get(i).Count().ToString() + ")";
        }
        return s;
    }
}
