//==============================================================================
// ChefZ_RecipeRanker - "spezifisch" als BERECHNETE Groesse
//
// Entwurf: 09 vollstaendig, insbesondere §3 (Schnittstelle), §4.1
// (Spezifitaetsformel woertlich), §4.2 (Laufzeitscore), §4.3
// (Rangreihenfolge), §4.4 (Tiebreak), §4.5 (das gerechnete Beispiel),
// §5 (Datenfluss), §7 (Fehlerverhalten), E1 bis E6.
//
// ---------------------------------------------------------------------------
// Wofuer es diese Klasse gibt
// ---------------------------------------------------------------------------
// Architekturplan §16 sagt: "Das spezifischste gueltige Rezept gewinnt."
// 09 E1 macht daraus einen Algorithmus statt einer Meinung - und zwar aus
// einem Grund, der nichts mit Eleganz zu tun hat:
//
// Eine handgepflegte priority-Zahl ist ein GLOBAL GETEILTER NAMENSRAUM. Wer
// priority 100 vergibt, muesste wissen, was alle anderen vergeben haben. Bei
// sechs parallel arbeitenden Content-Agenten in M3 ist das nicht zu halten.
// Eine berechnete Spezifitaet braucht dagegen null Wissen ueber fremde Slices:
// wer ein spezifischeres Rezept schreibt, bekommt automatisch Vorrang.
//
// ---------------------------------------------------------------------------
// Zwei bewusste Abweichungen von den Signaturen in 09 §3
// ---------------------------------------------------------------------------
// 1. SortCandidates() und ExplainOrder() bekommen die Rangliste als Parameter.
//    09 §3 zeigt sie ohne - das setzte eine statische Rangtabelle IM Ranker
//    voraus, also globalen veraenderlichen Zustand in einer sonst statischen
//    Klasse. Dann haenge das Ergebnis eines Aufrufs davon ab, wer vorher was
//    gesetzt hat. Die Rangtabelle gehoert der ChefZ_RecipeEngine, und sie
//    reicht sie herein.
//
// 2. ReportAmbiguities() bekommt die KOMPILIERTEN Rezepte, nicht die
//    Rohregistry. Die Analyse braucht Spezifitaet und Slotmengen, und beide
//    entstehen erst beim Kompilieren - auf der Rohform waere sie schlicht
//    nicht durchfuehrbar.
//
// Beide Abweichungen aendern nichts an dem, WAS gerechnet wird.
//
// ---------------------------------------------------------------------------
// Die Nicht-Verhandelbare aus 09 §5
// ---------------------------------------------------------------------------
// "Compare darf unter keinen Umstaenden auf Uhrzeit, Zufallszahlen oder die
// Iterationsreihenfolge einer map zurueckgreifen." In dieser Datei gibt es
// keine dieser drei Dinge. Sie arbeitet ausschliesslich auf array.
//
// KEIN CONTENT.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_RecipeRanker
{
    /**
     * Obergrenze fuer die Ambiguitaetsanalyse.
     *
     * Sie vergleicht Paare, ist also quadratisch. Bei 60 bis 100 Rezepten ist
     * das beim Boot nicht messbar; bei 2000 waere es eine Startverzoegerung,
     * die niemand erklaeren kann. Ueber der Grenze wird uebersprungen und das
     * ausdruecklich gemeldet - stilles Weglassen einer Diagnose ist die
     * schlechteste Eigenschaft eines Diagnosewerkzeugs (18).
     */
    static const int MAX_PAIRWISE_RECIPES = 400;

    //! Obergrenze fuer gemeldete Paare. Ein Duplikat aus zwei Slices erzeugt
    //! sonst ein WARN je Kombination, und der Ladebericht besteht danach nur
    //! noch daraus.
    static const int MAX_REPORTED_PAIRS = 20;

    //==========================================================================
    // 09 §4.1 - Spezifitaet, beim Build, haengt NUR vom Rezept ab
    //==========================================================================

    /**
     * specificity(recipe) nach 09 §4.1, Term fuer Term:
     *
     *     SUM ueber Pflicht-Slots:    selector.specificity * min(minCount, amountCap)
     *   + SUM ueber optionale Slots:  selector.specificity * wOptionalSlot
     *   + Kontextbonus:               wContextDeviceClass je deviceClasses-Eintrag
     *                                 wContextBound je gebundener Bedingung
     *   + Policybonus:                wPolicyForbid bei extraItems == "forbid"
     *                                 wPolicyPerState je forbiddenStates-Eintrag
     *   + Capabilitybonus:            wCapability je requires[]-Eintrag
     *   + Werkzeugbonus:              wToolGroup je requiredToolGroups-Eintrag
     *
     * amountCap deckelt den einzigen unbegrenzten Faktor: "8x Fleisch" soll
     * nicht automatisch spezifischer sein als "1x Spezialwurst + 1x Spezial-
     * sauce" (09 §4.1).
     *
     * Die Spezifitaet des SELEKTORS kommt aus dem Slotcompiler (07 §2.2) und
     * wird hier nicht neu gerechnet - sie steht bereits in slot.specificity.
     * Zweimal dieselbe Formel waere zwei Stellen, an denen sie auseinander
     * laufen kann.
     */
    static float ComputeSpecificity(notnull ChefZ_CompiledRecipe recipe, notnull ChefZ_PriorityWeights w)
    {
        float total = 0.0;
        int i;

        for (i = 0; i < recipe.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = recipe.slots.Get(i);
            if (!slot)
                continue;

            if (ChefZ_CompiledRecipe.IsRequiredSlot(slot))
            {
                int amount = slot.minCount;
                if (amount > w.amountCap)
                    amount = w.amountCap;
                if (amount < 1)
                    amount = 1;
                total = total + slot.specificity * amount;
            }
            else
            {
                total = total + slot.specificity * w.wOptionalSlot;
            }
        }

        for (i = 0; i < recipe.contexts.Count(); i++)
        {
            ChefZ_CompiledContext ctx = recipe.contexts.Get(i);
            total = total + w.wContextDeviceClass * ctx.deviceClasses.Count();
            total = total + w.wContextBound * ctx.BoundConditionCount();
        }

        if (recipe.policy)
            total = total + recipe.policy.SpecificityBonus(w);

        total = total + w.wCapability * recipe.requires.Count();
        total = total + w.wToolGroup * recipe.requiredToolGroups.Count();

        return total;
    }

    //==========================================================================
    // 09 §4.2 - Laufzeitscore, braucht den Kontext
    //==========================================================================

    /**
     * score(recipe, match) nach 09 §4.2:
     *
     *     specificity(recipe)                                   (gecacht)
     *   + coverageBonus * (gebundene Items / Items im Gefaess)
     *   + priority * priorityScale
     *
     * Der ABDECKUNGSBONUS (09 E4) schliesst das haeufigste
     * Frustrationsergebnis: aus fuenf hochwertigen Zutaten wird ein
     * Zwei-Zutaten-Gericht, und drei Zutaten bleiben ungenutzt im Topf. Wer
     * den Kessel leerkocht, schlaegt den, der die Haelfte liegen laesst - aber
     * nur bei sonst gleicher Spezifitaet, denn 0.50 ist bewusst kleiner als
     * jeder echte Spezifitaetsunterschied.
     *
     * Die DAEMPFUNG (09 E1) macht priority zu dem, was sie sein soll: ein
     * Werkzeug fuer den Gleichstand, kein Hebel. priority = 100 verschiebt den
     * Score um 1.0 - genug, um eine knappe Entscheidung zu drehen, zu wenig,
     * um die Spezifitaetsordnung zu ueberstimmen.
     */
    static float ComputeMatchScore(notnull ChefZ_CompiledRecipe recipe, notnull ChefZ_MatchResult match, int itemsInVessel, notnull ChefZ_PriorityWeights w)
    {
        float score = recipe.specificity;

        if (itemsInVessel > 0)
        {
            float bound = match.boundItemCount;
            float total = itemsInVessel;
            score = score + w.coverageBonus * (bound / total);
        }

        score = score + recipe.priority * w.priorityScale;
        return score;
    }

    //==========================================================================
    // 09 §4.4 - Tiebreak zur Laufzeit
    //==========================================================================

    /**
     * Bei identischem score, in dieser Reihenfolge (09 §4.4):
     *
     *     1. mehr gebundene Items gewinnt
     *     2. mehr Pflicht-Slots gewinnt
     *     3. hoehere explizite priority gewinnt
     *     4. lexikografisch kleinere id gewinnt        <- nie Zufall
     *
     * Stufe 4 ist inhaltlich sinnlos und trotzdem wichtiger als eine
     * "kluegere" Regel: sie macht Compare zu einer TOTALEN Ordnung. Ein Server
     * und der Testserver des Entwicklers muessen dasselbe Gericht produzieren,
     * sonst sind Fehlerberichte wertlos.
     *
     * Rueckgabe < 0, wenn a vor b steht.
     */
    static int CompareMatches(notnull ChefZ_MatchResult a, notnull ChefZ_MatchResult b)
    {
        if (a.score > b.score)                          return -1;
        if (a.score < b.score)                          return 1;

        if (a.boundItemCount > b.boundItemCount)        return -1;
        if (a.boundItemCount < b.boundItemCount)        return 1;

        if (a.requiredSlots > b.requiredSlots)          return -1;
        if (a.requiredSlots < b.requiredSlots)          return 1;

        if (a.priority > b.priority)                    return -1;
        if (a.priority < b.priority)                    return 1;

        return ChefZ_StringOrder.Compare(a.recipeId, b.recipeId);
    }

    //==========================================================================
    // 09 §4.3 - Rangreihenfolge, beim Build
    //==========================================================================

    /**
     * Sortiert eine Kandidatenliste in Rangreihenfolge.
     *
     * candidateIdx enthaelt Indizes in ranks. Einfuegesortierung: stabil, ohne
     * Allokation, und die Listen sind kurz. 09 E2 haelt fest, dass das
     * ausschliesslich beim BUILD geschieht - bei einem Kessel-Tick faellt kein
     * Sortieren an, weil die Indexlisten bereits sortiert sind.
     */
    static void SortCandidates(notnull array<ref ChefZ_RecipeRank> ranks, notnull array<int> candidateIdx)
    {
        for (int i = 1; i < candidateIdx.Count(); i++)
        {
            int key = candidateIdx.Get(i);
            ChefZ_RecipeRank keyRank = RankAt(ranks, key);
            int j = i - 1;

            while (j >= 0)
            {
                ChefZ_RecipeRank other = RankAt(ranks, candidateIdx.Get(j));
                if (!other || !keyRank)
                    break;
                if (ChefZ_RecipeRank.Compare(other, keyRank) <= 0)
                    break;
                candidateIdx.Set(j + 1, candidateIdx.Get(j));
                j--;
            }

            candidateIdx.Set(j + 1, key);
        }
    }

    private static ChefZ_RecipeRank RankAt(notnull array<ref ChefZ_RecipeRank> ranks, int index)
    {
        if (index < 0 || index >= ranks.Count())
            return null;
        return ranks.Get(index);
    }

    /**
     * Die Rangreihenfolge als lesbare Zeilen (09 §3, ExplainOrder).
     *
     * Fuer "chefz recipes" und fuer den Validator: die einzige Stelle, an der
     * ein Content-Autor SIEHT, warum sein Rezept hinter einem anderen steht.
     */
    static void ExplainOrder(notnull array<ref ChefZ_RecipeRank> ranks, notnull array<int> candidateIdx, out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        for (int i = 0; i < candidateIdx.Count(); i++)
        {
            ChefZ_RecipeRank rank = RankAt(ranks, candidateIdx.Get(i));
            if (!rank)
                continue;
            outLines.Insert("  " + (i + 1).ToString() + ". " + rank.ToDebugString());
        }
    }

    //==========================================================================
    // 09 §5 / E5 - Ambiguitaeten beim Boot melden
    //==========================================================================

    /**
     * Findet zwei Sorten Content-Problem und schreibt sie in den Ladebericht:
     *
     *   1. GLEICHSTAND    Zwei Rezepte mit identischem Rang UND identischer
     *                     Slotmenge. Dann entscheidet allein die ID, und das
     *                     ist fast immer ein Duplikat aus zwei Slices.
     *
     *   2. VERDECKUNG     A verlangt alles, was B verlangt, und mehr - rangiert
     *                     aber niedriger. B wird also immer zuerst probiert und
     *                     bindet immer, wenn A binden koennte. A kaeme nie zum
     *                     Zug.
     *
     * 09 E5 benennt die Grenze dieser Analyse ausdruecklich, und sie wird hier
     * ebenso ausdruecklich wiederholt: sie ist HEURISTISCH. Sie vergleicht
     * Slot-Zeichenketten und Geraetemengen, nicht Selektorsemantik. Zwei
     * Selektoren, die dasselbe bedeuten und verschieden geschrieben sind,
     * findet sie nicht. Deshalb meldet sie WARN und nie ERROR: ein Fehlalarm
     * darf kein Rezept blockieren, und eine Luecke darf keinen Server anhalten.
     *
     * Das verdeckte Rezept BLEIBT geladen (09 §7) - die Verdeckung kann bei
     * anderem Geraet oder anderer Menge aufgehoben sein.
     */
    static void ReportAmbiguities(notnull array<ref ChefZ_CompiledRecipe> recipes, notnull array<ref ChefZ_RecipeRank> ranks, ChefZ_LoadReport report)
    {
        if (!report)
            return;

        int n = recipes.Count();
        if (n < 2)
            return;

        if (n > MAX_PAIRWISE_RECIPES)
        {
            report.AddInfo("Ambiguitaetsanalyse uebersprungen: " + n.ToString() + " Rezepte liegen ueber der Grenze von " + MAX_PAIRWISE_RECIPES.ToString() + ". Die Pruefung ist paarweise und damit quadratisch; sie wuerde den " + "Serverstart merklich verzoegern. Die Rangordnung selbst ist davon " + "unberuehrt (09 E5).");
            return;
        }

        // Slotsignaturen einmal vorab. Sie werden n*(n-1)/2 mal verglichen,
        // und sie je Vergleich neu zu bauen waere der teuerste Teil der ganzen
        // Analyse.
        array<ref array<string>> signatures = new array<ref array<string>>();
        for (int s = 0; s < n; s++)
            signatures.Insert(SlotSignature(recipes.Get(s)));

        int reported = 0;

        for (int a = 0; a < n && reported < MAX_REPORTED_PAIRS; a++)
        {
            for (int b = a + 1; b < n && reported < MAX_REPORTED_PAIRS; b++)
            {
                ChefZ_CompiledRecipe ra = recipes.Get(a);
                ChefZ_CompiledRecipe rb = recipes.Get(b);
                if (!ra || !rb)
                    continue;

                // Rezepte, die sich kein Geraet teilen, koennen sich nicht in
                // die Quere kommen (09 §5: "Slot-Obermengen bei GLEICHEM
                // Kontext").
                if (!SharesDevice(ra, rb))
                    continue;

                array<string> sa = signatures.Get(a);
                array<string> sb = signatures.Get(b);

                if (SameSignature(sa, sb) && ChefZ_RecipeRank.SameRank(ranks.Get(a), ranks.Get(b)))
                {
                    report.AddWarn(ra.sourceRef, ra.id, "Rezept \"" + ra.id + "\" und \"" + rb.id + "\" (" + rb.sourceRef + ") " + "haben denselben Rang UND dieselbe Slotmenge. Welches gewinnt, " + "entscheidet allein die alphabetische ID - hier also \"" + FirstById(ra.id, rb.id) + "\". Das ist fast immer ein Duplikat aus " + "zwei Slices (09 §7).");
                    reported++;
                    continue;
                }

                // Verdeckung in beide Richtungen pruefen: die Obermenge ist
                // das potenzielle Opfer, nicht der Taeter.
                if (IsSuperset(sa, sb) && ChefZ_RecipeRank.Compare(ranks.Get(a), ranks.Get(b)) > 0)
                {
                    ReportShadow(report, ra, rb);
                    reported++;
                    continue;
                }
                if (IsSuperset(sb, sa) && ChefZ_RecipeRank.Compare(ranks.Get(b), ranks.Get(a)) > 0)
                {
                    ReportShadow(report, rb, ra);
                    reported++;
                }
            }
        }

        if (reported >= MAX_REPORTED_PAIRS)
        {
            report.AddInfo("Weitere Ambiguitaeten wurden nicht mehr einzeln gemeldet - " + MAX_REPORTED_PAIRS.ToString() + " Paare reichen, um das Muster zu erkennen.");
        }
    }

    private static void ReportShadow(notnull ChefZ_LoadReport report, notnull ChefZ_CompiledRecipe shadowed, notnull ChefZ_CompiledRecipe winner)
    {
        report.AddWarn(shadowed.sourceRef, shadowed.id, "Rezept \"" + shadowed.id + "\" wird voraussichtlich von \"" + winner.id + "\" (" + winner.sourceRef + ") verdeckt: es verlangt alles, was jenes verlangt, und mehr, " + "rangiert aber niedriger (spez " + shadowed.specificity.ToString() + " gegen " + winner.specificity.ToString() + "). Wo beide binden koennten, gewinnt immer das " + "andere. Das Rezept bleibt geladen - bei anderem Geraet oder anderer Menge kann " + "die Verdeckung aufgehoben sein. Die Analyse ist heuristisch (09 E5).");
    }

    private static string FirstById(string a, string b)
    {
        if (ChefZ_StringOrder.Compare(a, b) <= 0)
            return a;
        return b;
    }

    //==========================================================================
    // Signaturen fuer die Ambiguitaetsanalyse
    //==========================================================================

    /**
     * Die PFLICHTslots eines Rezepts als sortierte Zeichenkettenliste.
     *
     * Sortiert, damit die Reihenfolge der Slots in der Datei keine Rolle
     * spielt: zwei Rezepte mit denselben Slots in anderer Reihenfolge sind
     * dasselbe Rezept, und genau das soll die Analyse finden.
     *
     * Optionale Slots bleiben draussen. Sie sind nie ein Bindungsgrund, also
     * auch nie ein Verdeckungsgrund - ein Rezept mit einem zusaetzlichen
     * optionalen Gewuerzslot ist keine Obermenge im Sinne dieser Analyse.
     */
    private static array<string> SlotSignature(ChefZ_CompiledRecipe recipe)
    {
        array<string> sig = new array<string>();
        if (!recipe)
            return sig;

        for (int i = 0; i < recipe.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = recipe.slots.Get(i);
            if (!slot || !ChefZ_CompiledRecipe.IsRequiredSlot(slot))
                continue;

            string s = "?";
            if (slot.selector)
                s = slot.selector.ToDebugString();

            sig.Insert(s + " x" + slot.minCount.ToString());
        }

        ChefZ_StringOrder.SortAscending(sig);
        return sig;
    }

    private static bool SameSignature(notnull array<string> a, notnull array<string> b)
    {
        if (a.Count() != b.Count())
            return false;
        for (int i = 0; i < a.Count(); i++)
        {
            if (a.Get(i) != b.Get(i))
                return false;
        }
        return true;
    }

    //! Enthaelt "sup" jeden Eintrag aus "sub" - und mindestens einen mehr?
    //! Gleich grosse Mengen sind ausdruecklich KEINE Obermenge; die faengt
    //! bereits SameSignature ab.
    private static bool IsSuperset(notnull array<string> sup, notnull array<string> sub)
    {
        if (sup.Count() <= sub.Count())
            return false;

        // Beide Listen sind sortiert, also reicht ein Mischlauf. Duplikate
        // werden dabei richtig gezaehlt: zwei gleiche Slots in "sub"
        // verlangen zwei gleiche in "sup".
        int i = 0;
        for (int j = 0; j < sub.Count(); j++)
        {
            bool found = false;
            while (i < sup.Count())
            {
                int cmp = ChefZ_StringOrder.Compare(sup.Get(i), sub.Get(j));
                i++;
                if (cmp == 0)
                {
                    found = true;
                    break;
                }
                if (cmp > 0)
                    return false;       // sup ist bereits darueber hinaus
            }
            if (!found)
                return false;
        }
        return true;
    }

    //! Teilen sich zwei Rezepte mindestens ein Geraet? Ein Rezept ohne
    //! Geraetebindung teilt sich mit jedem eines.
    private static bool SharesDevice(notnull ChefZ_CompiledRecipe a, notnull ChefZ_CompiledRecipe b)
    {
        if (a.HasUnboundDeviceContext() || b.HasUnboundDeviceContext())
            return true;

        array<ChefZ_Sym> aClasses    = new array<ChefZ_Sym>();
        array<ChefZ_Sym> aCategories = new array<ChefZ_Sym>();
        array<ChefZ_Sym> bClasses    = new array<ChefZ_Sym>();
        array<ChefZ_Sym> bCategories = new array<ChefZ_Sym>();

        a.CollectDeviceKeys(aClasses, aCategories);
        b.CollectDeviceKeys(bClasses, bCategories);

        if (HasCommon(aClasses, bClasses))
            return true;
        return HasCommon(aCategories, bCategories);
    }

    private static bool HasCommon(notnull array<ChefZ_Sym> a, notnull array<ChefZ_Sym> b)
    {
        for (int i = 0; i < a.Count(); i++)
        {
            if (b.Find(a.Get(i)) >= 0)
                return true;
        }
        return false;
    }
}
