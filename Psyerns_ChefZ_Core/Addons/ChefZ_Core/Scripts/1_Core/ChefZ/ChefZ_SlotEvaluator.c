//==============================================================================
// ChefZ_SlotEvaluator - alles, was EIN Slot ueber eine Faktenliste sagen kann
//
// Entwurf: 07 §2.3 (Schnittstelle woertlich), 07 §4 (Kandidaten, Zaehl- und
// Mengenbedingung), 07 §5 (Test-Kosten), 07 E3 (Menge wird ueber den Slot
// summiert, nicht pro Item geprueft), 07 E5 (excludeStates), 07 E6
// (Reihenfolge der Begruendungen), 05 §6 (Einheiten und Verbrauch).
//
// ---------------------------------------------------------------------------
// Handles nach aussen, Indizes nach innen
// ---------------------------------------------------------------------------
// Die Schnittstelle in 07 §2.3 spricht von Handles. Der Matcher arbeitet
// intern mit POSITIONEN im Snapshot, weil er sie in der innersten Schleife
// dutzendfach nachschlaegt und ChefZ_FactSnapshot.FindByHandle() linear sucht.
//
// Deshalb gibt es jede Funktion zweimal: die Fassung aus dem Entwurf (Handles,
// fuer Aufrufer ausserhalb des Matchers - Cookbook, Diagnose, Validator) und
// eine Fassung auf Indizes, die der Matcher benutzt. Die Handle-Fassung ist
// eine duenne Huelle um die Index-Fassung; es gibt keine zweite Logik.
//
// Alles hier ist rein lesend. Es gibt keinen Zeiger auf ein ItemBase.
//
// KEIN CONTENT: die Beispielnamen in den Kommentaren stammen woertlich aus
// Entwurf 07; im Code steht keiner von ihnen.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_SlotEvaluator
{
    //! Rechentoleranz fuer Mengenvergleiche. Mengen kommen aus float-Rechnung
    //! (quantity / quantityMax * unitsPerWholeItem); ohne Toleranz scheitert
    //! "2 Einheiten gefordert, 1.9999999 vorhanden" - fuer den Spieler
    //! unerklaerlich.
    static const float EPS = 0.0001;

    //==========================================================================
    // Eignung eines einzelnen Items
    //==========================================================================

    /**
     * Darf dieses Item diesen Slot bedienen?
     *
     * Drei Pruefungen, absteigend nach Aussagekraft (07 E6):
     *   1. Selektor        - ist es ueberhaupt die richtige Zutat?
     *   2. excludeStates   - ist der Zustand ausgeschlossen? (07 E5)
     *   3. Einheit         - fuehrt das Item die verlangte Einheit? (05 §6)
     *
     * Baut keine einzige Zeichenkette. Wer eine Begruendung will, ruft
     * AcceptsExplain() - und das tut nur der Trace.
     */
    static bool Accepts(notnull ChefZ_CompiledSlot slot, notnull ChefZ_ItemFacts facts)
    {
        if (!slot.selector)
            return false;
        if (!slot.selector.Test(facts))
            return false;
        if (slot.IsStateExcluded(facts.chefzState))
            return false;
        if (ChefZ_SymbolTable.IsValid(slot.unitSym) && facts.quantityUnit != slot.unitSym)
            return false;
        return true;
    }

    /**
     * Wie Accepts(), aber mit der NUETZLICHSTEN Begruendung (07 E6).
     *
     * Die Reihenfolge ist der ganze Sinn der Funktion: ein Spieler, der Rohes
     * statt Getrocknetes in den Topf legt, soll "state RAW nicht zulaessig,
     * gebraucht wird DRIED" lesen - nicht "kein Rezept passt".
     */
    static bool AcceptsExplain(notnull ChefZ_CompiledSlot slot, notnull ChefZ_ItemFacts facts, out string reason)
    {
        reason = "";

        if (!slot.selector)
        {
            reason = "Slot ohne Selektor";
            return false;
        }

        // 1. Selektion. Erst das Praedikat, damit nicht die Frische eines
        //    Items gemeldet wird, das gar nicht die richtige Zutat ist.
        if (!slot.selector.TestStructure(facts))
        {
            string structureReason;
            slot.selector.Explain(facts, structureReason);
            reason = structureReason;
            return false;
        }

        // 2. Ausgeschlossene Zustaende.
        if (slot.IsStateExcluded(facts.chefzState))
        {
            reason = "Zustand " + ChefZ_SymbolTable.NameOrMark(facts.chefzState) + " ausgeschlossen";
            return false;
        }

        // 3. Qualitaetsschwelle und Wertebereiche.
        string detailReason;
        if (!slot.selector.Explain(facts, detailReason))
        {
            reason = detailReason;
            return false;
        }

        // 4. Einheit.
        if (ChefZ_SymbolTable.IsValid(slot.unitSym) && facts.quantityUnit != slot.unitSym)
        {
            reason = "Einheit " + ChefZ_SymbolTable.NameOrMark(facts.quantityUnit) + " != " + ChefZ_SymbolTable.Name(slot.unitSym);
            return false;
        }

        return true;
    }

    //==========================================================================
    // Kandidaten
    //==========================================================================

    /**
     * Alle freien Items, die diesen Slot bedienen koennen - als POSITIONEN im
     * Snapshot, in Snapshot-Reihenfolge.
     *
     * Die Reihenfolge ist die stabile Sortierung aus 05 §3.3
     * (classSym, chefzState, Menge absteigend, Handle). Sie traegt den
     * Determinismus des ganzen Matchers: derselbe Topfinhalt liefert dieselbe
     * Bindung, egal in welcher Reihenfolge der Spieler eingelegt hat
     * (07 §4, "Determinismus").
     *
     * "Frei" heisst slotBoundTo < 0. Ein Item bedient hoechstens einen Slot -
     * ohne diese Regel koennte dasselbe Stueck Fleisch MEAT und WILD_MEAT
     * gleichzeitig erfuellen und ein Rezept mit zwei Fleischslots mit EINER
     * Zutat ausloesen (07 §4, letzter Abschnitt).
     */
    static int CollectCandidateIndices(notnull ChefZ_CompiledSlot slot, notnull ChefZ_FactSnapshot snapshot, out array<int> outIndices)
    {
        if (!outIndices)
            outIndices = new array<int>();
        outIndices.Clear();

        for (int i = 0; i < snapshot.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.Get(i);
            if (!facts)
                continue;
            if (facts.slotBoundTo >= 0)
                continue;
            if (!Accepts(slot, facts))
                continue;
            outIndices.Insert(i);
        }

        return outIndices.Count();
    }

    /**
     * Fassung aus 07 §2.3: Kandidaten als HANDLES.
     *
     * alreadyBound schliesst zusaetzlich zu slotBoundTo aus - fuer Aufrufer,
     * die eine eigene Belegung fuehren (Cookbook-Vorschau, "was fehlt mir
     * noch?"). Der Matcher braucht den Parameter nicht; er fuehrt die Belegung
     * im Snapshot.
     */
    static int CollectCandidates(notnull ChefZ_CompiledSlot slot, notnull ChefZ_FactSnapshot snapshot, array<int> alreadyBound, out array<int> outHandles)
    {
        if (!outHandles)
            outHandles = new array<int>();
        outHandles.Clear();

        for (int i = 0; i < snapshot.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.Get(i);
            if (!facts)
                continue;
            if (facts.slotBoundTo >= 0)
                continue;
            if (alreadyBound && alreadyBound.Find(facts.handle) >= 0)
                continue;
            if (!Accepts(slot, facts))
                continue;
            outHandles.Insert(facts.handle);
        }

        return outHandles.Count();
    }

    //==========================================================================
    // Zaehl- und Mengenbedingung
    //==========================================================================

    static bool CheckCounts(notnull ChefZ_CompiledSlot slot, int assignedCount)
    {
        if (assignedCount < slot.minCount)
            return false;
        if (assignedCount > slot.maxCount)
            return false;
        return true;
    }

    /**
     * Summiert die Einheiten der zugewiesenen Items und prueft die
     * Mengenforderung (07 E3).
     *
     * Geprueft wird die SUMME, nicht jedes Item einzeln: ein Slot
     * { "category": "FLOUR", "amount": {"min": 2} } akzeptiert zwei Beutel mit
     * je einer Einheit. Die Alternative waere fuer Spieler unerklaerlich, weil
     * DayZ Mengen frei splittet und stapelt.
     *
     * Ausnahme allowPartial == false: dann muss JEDES zugewiesene Item die
     * Untergrenze fuer sich allein tragen - das ist die Formulierung fuer "ein
     * ganzes Stueck" (07 E3, letzter Satz).
     */
    static bool CheckAmountIdx(notnull ChefZ_CompiledSlot slot, notnull ChefZ_FactSnapshot snapshot, notnull array<int> assignedIndices, out float totalUnits)
    {
        totalUnits = 0.0;

        float required = slot.RequiredUnits();

        for (int i = 0; i < assignedIndices.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.Get(assignedIndices.Get(i));
            if (!facts)
                continue;

            totalUnits = totalUnits + facts.units;

            if (!slot.allowPartial && required > 0.0 && facts.units + EPS < required)
                return false;
        }

        if (!slot.amount)
            return true;

        if (slot.amount.HasMin() && totalUnits + EPS < slot.amount.min)
            return false;

        if (slot.amount.HasMax() && totalUnits > slot.amount.max + EPS)
            return false;

        return true;
    }

    //! Fassung aus 07 §2.3: Zuweisung als HANDLES.
    static bool CheckAmount(notnull ChefZ_CompiledSlot slot, notnull ChefZ_FactSnapshot snapshot, notnull array<int> assignedHandles, out float totalUnits)
    {
        array<int> indices = new array<int>();
        HandlesToIndices(snapshot, assignedHandles, indices);
        return CheckAmountIdx(slot, snapshot, indices, totalUnits);
    }

    /**
     * Kann die Mengenforderung mit den bisher zugewiesenen Items ueberhaupt
     * noch erfuellt werden, oder ist bereits zu viel im Slot?
     *
     * Der Matcher nutzt das zur Frueherkennung (07 §4, Schritt 3): eine
     * Zuweisung, die die Obergrenze bereits reisst, muss nicht weiter
     * vertieft werden - jedes zusaetzliche Item macht es schlimmer.
     */
    static bool ExceedsMaxAmount(notnull ChefZ_CompiledSlot slot, float totalUnits)
    {
        if (!slot.amount || !slot.amount.HasMax())
            return false;
        return totalUnits > slot.amount.max + EPS;
    }

    static float SumUnits(notnull ChefZ_FactSnapshot snapshot, notnull array<int> indices)
    {
        float sum = 0.0;
        for (int i = 0; i < indices.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.Get(indices.Get(i));
            if (facts)
                sum = sum + facts.units;
        }
        return sum;
    }

    //==========================================================================
    // Verbrauchsplan
    //==========================================================================

    /**
     * Was mit den zugewiesenen Items geschehen soll (07 §2.3, 05 §6).
     *
     *   whole   jedes zugewiesene Item wird geloescht
     *   amount  consumeAmount Einheiten werden ABGEZOGEN, und zwar der Reihe
     *           nach in Snapshot-Reihenfolge (07 E3: "deterministisch in der
     *           Sortierreihenfolge des Snapshots"). Was danach leer ist, wird
     *           geloescht; was Rest behaelt, bleibt liegen.
     *   none    nichts wird verbraucht. Genau hier wirkt setStateAfter: der
     *           Slot veraendert den Zustand statt das Item zu verbrauchen.
     *
     * Bei allowPartial == false zieht "amount" keine Bruchteile: es werden
     * ganze Items aufgebraucht, bis die Menge gedeckt ist.
     *
     * Der Plan bleibt ein Vorschlag. Ausgefuehrt wird er vom Applicator
     * (4_World, S8), und der prueft jeden Handle davor noch einmal (08 §6) -
     * ein Item, das zwischen Match und Anwendung verschwindet, darf nicht dazu
     * fuehren, dass irgendetwas anderes verbraucht wird.
     */
    static void BuildConsumePlanIdx(notnull ChefZ_CompiledSlot slot, notnull ChefZ_FactSnapshot snapshot, notnull array<int> assignedIndices, out array<ref ChefZ_ConsumePlan> outPlan)
    {
        if (!outPlan)
            outPlan = new array<ref ChefZ_ConsumePlan>();

        float remaining = slot.consumeAmount;

        for (int i = 0; i < assignedIndices.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.Get(assignedIndices.Get(i));
            if (!facts)
                continue;

            ChefZ_ConsumePlan entry = new ChefZ_ConsumePlan();
            entry.handle    = facts.handle;
            entry.slotIndex = slot.slotIndex;

            if (slot.consumeMode == ChefZ_ConsumeMode.WHOLE)
            {
                entry.destroyWhole  = true;
                entry.quantityDelta = facts.quantity;
                entry.unitsDelta    = facts.units;
            }
            else if (slot.consumeMode == ChefZ_ConsumeMode.AMOUNT)
            {
                if (remaining > EPS)
                    remaining = PlanAmountDraw(slot, facts, entry, remaining);
            }

            // Zustandswechsel gilt fuer alles, was ueberlebt. Bei consume
            // "whole" waere er wirkungslos - der Compiler warnt dort bereits.
            if (ChefZ_SymbolTable.IsValid(slot.setStateAfter) && !entry.destroyWhole)
                entry.setStateAfter = slot.setStateAfter;

            if (entry.HasEffect())
                outPlan.Insert(entry);
        }
    }

    /**
     * Zieht von EINEM Item ab und liefert den Rest der offenen Menge zurueck.
     *
     * Die Umrechnung Einheit -> Quantity geschieht hier, weil hier beide
     * Zahlen vorliegen: units ist quantity in Rezepteinheiten (05 §6), also
     * ist quantity / units der Wert einer Einheit. Ohne Einheiten (units == 0)
     * gibt es nichts abzuziehen - so ein Item haette den Mengentest ohnehin
     * nicht bestanden.
     */
    private static float PlanAmountDraw(notnull ChefZ_CompiledSlot slot, notnull ChefZ_ItemFacts facts, notnull ChefZ_ConsumePlan entry, float remaining)
    {
        if (facts.units <= 0.0)
            return remaining;

        float take = remaining;
        if (take > facts.units)
            take = facts.units;

        // Ganze Stuecke: entweder das Item ist aufgebraucht oder es wird nicht
        // angefasst.
        if (!slot.allowPartial)
            take = facts.units;

        float perUnit = facts.quantity / facts.units;

        entry.unitsDelta    = take;
        entry.quantityDelta = take * perUnit;

        if (take + EPS >= facts.units)
        {
            // Nichts Brauchbares bleibt uebrig. Das Item als Leerhuelle im
            // Topf liegen zu lassen waere fuer den Spieler nur verwirrend.
            entry.destroyWhole  = true;
            entry.quantityDelta = facts.quantity;
            entry.unitsDelta    = facts.units;
        }

        return remaining - take;
    }

    //! Fassung aus 07 §2.3: Zuweisung als HANDLES.
    static void BuildConsumePlan(notnull ChefZ_CompiledSlot slot, notnull ChefZ_FactSnapshot snapshot, notnull array<int> assignedHandles, out array<ref ChefZ_ConsumePlan> outPlan)
    {
        array<int> indices = new array<int>();
        HandlesToIndices(snapshot, assignedHandles, indices);
        BuildConsumePlanIdx(slot, snapshot, indices, outPlan);
    }

    //==========================================================================

    static void HandlesToIndices(notnull ChefZ_FactSnapshot snapshot, notnull array<int> handles, notnull array<int> outIndices)
    {
        outIndices.Clear();
        for (int h = 0; h < handles.Count(); h++)
        {
            for (int i = 0; i < snapshot.Count(); i++)
            {
                ChefZ_ItemFacts facts = snapshot.Get(i);
                if (facts && facts.handle == handles.Get(h))
                {
                    outIndices.Insert(i);
                    break;
                }
            }
        }
    }

    static void IndicesToHandles(notnull ChefZ_FactSnapshot snapshot, notnull array<int> indices, notnull array<int> outHandles)
    {
        outHandles.Clear();
        for (int i = 0; i < indices.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.Get(indices.Get(i));
            if (facts)
                outHandles.Insert(facts.handle);
        }
    }
}
