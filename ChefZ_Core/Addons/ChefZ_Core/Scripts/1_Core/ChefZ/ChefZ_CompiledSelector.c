//==============================================================================
// ChefZ_SelectorOp / ChefZ_RangeConstraint / ChefZ_CompiledSelector
//
// Entwurf: 07 §2.2 (Feldliste und Schnittstelle woertlich), 07 §5 (was Test()
// im heissen Pfad tatsaechlich tut), 07 §6 (zustandslos), 07 §7 (Fehlerfaelle),
// 07 E6 (Reihenfolge der Pruefungen bestimmt die NUETZLICHSTE Begruendung).
//
// ---------------------------------------------------------------------------
// Die kompilierte Form ist der Grund, warum der Matcher billig ist
// ---------------------------------------------------------------------------
// In dieser Form gibt es keinen String mehr, keinen Managerzugriff und keinen
// Defaultzweig. Was in der Rohform "category": "MEAT" hiess, ist hier ein
// Bitindex, und der Test dazu ist ein Array-Get und ein AND (04 E1).
//
//     CLASS         classSym == sym                    1 Vergleich
//     CATEGORY      closure.HasBit(categoryBitIndex)   1 Bit-Test
//     TAG           tags.Find(sym) >= 0                kurze Liste
//     STATE         chefzState == sym                  1 Vergleich
//     VANILLA_STAGE vanillaFoodStage == value          1 Vergleich
//     ANY_OF/ALL_OF Kurzschluss ueber die Kinder
//     Ranges        Vergleichskette, nur wenn das Blatt bestanden hat
//
// ---------------------------------------------------------------------------
// minQuality: eine Schwelle wird zur Aufzaehlung
// ---------------------------------------------------------------------------
// ChefZ_ItemFacts traegt das Qualitaetssymbol, aber keinen Rang - und 1_Core
// darf keinen Manager fragen (00 §4). Ein Rangvergleich zur Laufzeit ist damit
// nicht moeglich, ohne den Layer-Schnitt aufzugeben.
//
// Geloest wird das beim KOMPILIEREN: der Compiler loest "minQuality: PREPARED"
// ueber den ChefZ_SymbolResolver in die Liste aller Stufen desselben
// Stufensatzes mit Rang >= PREPARED auf. Zur Laufzeit ist die Pruefung dann
// ein Find() auf einer Liste mit ein bis fuenf Eintraegen. minQualitySym und
// minQualityRank bleiben erhalten, weil 07 §2.2 sie nennt und weil der Trace
// die Schwelle im Klartext ausgeben muss ("quality SIMPLE < PREPARED").
//
// KEIN CONTENT, KEIN ENGINE-TYP, KEIN ZUSTAND. Ein Selektor ist eine Funktion.
// Er wird pro Tick mehrfach fuer verschiedene Rezepte gerufen und darf
// zwischen zwei Aufrufen nichts mitschleppen (07 §6). Wo in den Kommentaren
// Namen wie MEAT oder SALTED auftauchen, sind es woertliche Beispiele aus
// Entwurf 07; im Code steht keiner von ihnen.
//
// Layer: 1_Core.
//==============================================================================

enum ChefZ_SelectorOp
{
    TRUE_OP,            // kein Blattpraedikat - nur Wertebereiche/minQuality
    CLASS,
    CATEGORY,
    TAG,
    STATE,
    VANILLA_STAGE,
    ANY_OF,
    ALL_OF,
    NOT,
    LIQUID
}

//------------------------------------------------------------------------------

/**
 * Ein gebundener Wertebereich, aufgeloest auf das Feld, das er prueft.
 *
 * Warum ein Feldschluessel und keine sieben bool-Flags: der Compiler haengt
 * genau die Bereiche an, die der Autor gesetzt hat. Ein Selektor ohne
 * Wertebereiche hat eine leere Liste, und der heisse Pfad laeuft dann durch
 * eine Schleife mit null Durchlaeufen statt durch sieben Nullpruefungen.
 */
class ChefZ_RangeConstraint : Managed
{
    static const int HEALTH       = 0;
    static const int FRESHNESS    = 1;
    static const int TEMPERATURE  = 2;
    static const int WETNESS      = 3;
    static const int CLEANNESS    = 4;
    static const int QUANTITY     = 5;
    static const int QUANTITY_PCT = 6;

    int field;
    ref ChefZ_Range range;

    void ChefZ_RangeConstraint()
    {
        field = HEALTH;
        range = null;
    }

    void Init(int fieldId, notnull ChefZ_Range r)
    {
        field = fieldId;
        range = r;
    }

    static string FieldName(int fieldId)
    {
        switch (fieldId)
        {
            case HEALTH:       return "health";
            case FRESHNESS:    return "freshness";
            case TEMPERATURE:  return "temperature";
            case WETNESS:      return "wetness";
            case CLEANNESS:    return "cleanness";
            case QUANTITY:     return "quantity";
            case QUANTITY_PCT: return "quantityPct";
        }
        return "?";
    }

    /**
     * Der gepruefte Wert aus den Fakten.
     *
     * quantityPct hat einen Sonderfall: ohne quantityMax gibt es keinen
     * Prozentsatz. Die Antwort ist dann 0.0 und NICHT 1.0 - ein Item ohne
     * Mengenachse soll eine Untergrenze nicht stillschweigend bestehen.
     */
    static float ValueOf(int fieldId, notnull ChefZ_ItemFacts facts)
    {
        switch (fieldId)
        {
            case HEALTH:       return facts.health01;
            case FRESHNESS:    return facts.freshness01;
            case TEMPERATURE:  return facts.temperature;
            case WETNESS:      return facts.wetness;
            case CLEANNESS:    return facts.cleanness01;
            case QUANTITY:     return facts.quantity;
            case QUANTITY_PCT:
                if (facts.quantityMax <= 0.0)
                    return 0.0;
                return facts.quantity / facts.quantityMax;
        }
        return 0.0;
    }

    bool Test(notnull ChefZ_ItemFacts facts)
    {
        if (!range)
            return true;
        return range.Contains(ValueOf(field, facts));
    }

    string ToDebugString()
    {
        string r = "[*..*]";
        if (range)
            r = range.ToDebugString();
        return FieldName(field) + r;
    }
}

//------------------------------------------------------------------------------

class ChefZ_CompiledSelector : Managed
{
    ChefZ_SelectorOp op;
    ChefZ_Sym        sym;                   // CLASS/TAG/STATE/LIQUID: das Symbol
    int              categoryBitIndex;      // Direktindex fuer den Bit-Test
    int              vanillaStageValue;     // FoodStageType als int
    ChefZ_Sym        minQualitySym;
    int              minQualityRank;

    //! Beim Kompilieren aufgeloeste Stufenliste zu minQualitySym (siehe Kopf).
    //! null oder leer = keine Qualitaetsschwelle.
    ref array<ChefZ_Sym> acceptedQualities;

    ref array<ref ChefZ_CompiledSelector> children;
    ref ChefZ_CompiledSelector            negated;
    ref array<ref ChefZ_RangeConstraint>  ranges;

    //! LIQUID: verlangt der Knoten einen Behaelter, einen Typ, oder beides?
    bool requireLiquidContainer;

    float specificity;                      // 09 §4.1, beim Kompilieren
    int   selectivityHint;                  // 07 E4, geschaetzte Trefferzahl

    void ChefZ_CompiledSelector()
    {
        op                     = ChefZ_SelectorOp.TRUE_OP;
        sym                    = ChefZ_SymbolTable.INVALID;
        categoryBitIndex       = -1;
        vanillaStageValue      = -1;
        minQualitySym          = ChefZ_SymbolTable.INVALID;
        minQualityRank         = -1;
        acceptedQualities      = null;
        children               = null;
        negated                = null;
        ranges                 = null;
        requireLiquidContainer = false;
        specificity            = 0.0;
        selectivityHint        = 0;
    }

    //==========================================================================
    // Auswertung - der heisse Pfad
    //==========================================================================

    /**
     * Die eine Frage: passt dieses Item?
     *
     * Reihenfolge: Blatt/Kinder, dann Qualitaetsschwelle, dann Wertebereiche
     * (07 E6). Die teureren Pruefungen laufen nur, wenn die billigere bestanden
     * hat - und die billigste ist fast immer die aussagekraeftigste.
     *
     * Rein lesend. Es gibt keinen Zeiger auf ein ItemBase, also kann diese
     * Funktion per Konstruktion nichts veraendern (05 E1).
     */
    bool Test(notnull ChefZ_ItemFacts facts)
    {
        if (!TestStructure(facts))
            return false;
        if (!TestQuality(facts))
            return false;
        return TestRanges(facts);
    }

    /**
     * Nur das Praedikat, ohne Qualitaetsschwelle und Wertebereiche DIESES
     * Knotens.
     *
     * Getrennt, weil 07 E6 verlangt, dass "passt die Zutat ueberhaupt nicht"
     * vor "Zustand ausgeschlossen" und vor "Frische zu gering" gemeldet wird.
     * Ohne diese Trennung stuende im Trace eine Frischemeldung fuer ein Item,
     * das gar nicht die richtige Zutat ist.
     */
    bool TestStructure(notnull ChefZ_ItemFacts facts)
    {
        int i;

        switch (op)
        {
            case ChefZ_SelectorOp.TRUE_OP:
                return true;

            case ChefZ_SelectorOp.CLASS:
                return facts.classSym == sym;

            case ChefZ_SelectorOp.CATEGORY:
                // Kein Nullzugriff moeglich: der Collector setzt immer eine
                // Closure, notfalls eine leere (07 §7, letzte Zeilen). Eine
                // leere Closure liefert false, und genau das ist gewollt.
                if (!facts.closure)
                    return false;
                return facts.closure.HasBit(categoryBitIndex);

            case ChefZ_SelectorOp.TAG:
                return facts.HasTag(sym);

            case ChefZ_SelectorOp.STATE:
                return facts.chefzState == sym;

            case ChefZ_SelectorOp.VANILLA_STAGE:
                return facts.vanillaFoodStage == vanillaStageValue;

            case ChefZ_SelectorOp.LIQUID:
                if (requireLiquidContainer && !facts.isLiquidContainer)
                    return false;
                if (ChefZ_SymbolTable.IsValid(sym) && facts.liquidTypeSym != sym)
                    return false;
                return true;

            case ChefZ_SelectorOp.ANY_OF:
                if (!children)
                    return false;
                for (i = 0; i < children.Count(); i++)
                {
                    if (children.Get(i).Test(facts))
                        return true;
                }
                return false;

            case ChefZ_SelectorOp.ALL_OF:
                if (!children)
                    return false;
                for (i = 0; i < children.Count(); i++)
                {
                    if (!children.Get(i).Test(facts))
                        return false;
                }
                return true;

            case ChefZ_SelectorOp.NOT:
                if (!negated)
                    return false;
                return !negated.Test(facts);
        }

        // Unerreichbar, solange op aus dem Compiler kommt. Die sichere
        // Antwort ist false: ein unbekannter Operator darf nicht auf alles
        // matchen.
        return false;
    }

    bool TestQuality(notnull ChefZ_ItemFacts facts)
    {
        if (!acceptedQualities || acceptedQualities.Count() == 0)
            return true;
        return acceptedQualities.Find(facts.chefzQuality) >= 0;
    }

    bool TestRanges(notnull ChefZ_ItemFacts facts)
    {
        if (!ranges)
            return true;
        for (int i = 0; i < ranges.Count(); i++)
        {
            if (!ranges.Get(i).Test(facts))
                return false;
        }
        return true;
    }

    //==========================================================================
    // Begruendung - nur wenn Kanal MATCH aktiv ist (07 E6, 18 §2.2)
    //==========================================================================

    /**
     * Erste Ablehnungsbegruendung im Klartext. true, wenn der Selektor passt
     * (reason bleibt dann leer).
     *
     * Die Zeichenketten entstehen NUR hier. Der Produktivpfad ruft Test(), und
     * Test() baut keinen einzigen String (07 E6, letzter Absatz).
     */
    bool Explain(notnull ChefZ_ItemFacts facts, out string reason)
    {
        reason = "";

        if (!TestStructure(facts))
        {
            reason = StructureReason(facts);
            return false;
        }

        if (!TestQuality(facts))
        {
            string have = "keine";
            if (ChefZ_SymbolTable.IsValid(facts.chefzQuality))
                have = ChefZ_SymbolTable.Name(facts.chefzQuality);
            reason = "quality " + have + " < " + ChefZ_SymbolTable.NameOrMark(minQualitySym);
            return false;
        }

        if (ranges)
        {
            for (int i = 0; i < ranges.Count(); i++)
            {
                ChefZ_RangeConstraint rc = ranges.Get(i);
                if (rc.Test(facts))
                    continue;
                float v = ChefZ_RangeConstraint.ValueOf(rc.field, facts);
                reason = ChefZ_RangeConstraint.FieldName(rc.field) + " " + v.ToString() + " ausserhalb " + rc.ToDebugString();
                return false;
            }
        }

        return true;
    }

    private string StructureReason(notnull ChefZ_ItemFacts facts)
    {
        switch (op)
        {
            case ChefZ_SelectorOp.CLASS:
                return "class " + ChefZ_SymbolTable.NameOrMark(facts.classSym) + " != " + ChefZ_SymbolTable.NameOrMark(sym);

            case ChefZ_SelectorOp.CATEGORY:
                return "nicht in Kategorie " + ChefZ_SymbolTable.NameOrMark(sym);

            case ChefZ_SelectorOp.TAG:
                return "ohne Tag " + ChefZ_SymbolTable.NameOrMark(sym);

            case ChefZ_SelectorOp.STATE:
                return "state " + ChefZ_SymbolTable.NameOrMark(facts.chefzState) + " nicht zulaessig, gebraucht wird " + ChefZ_SymbolTable.NameOrMark(sym);

            case ChefZ_SelectorOp.VANILLA_STAGE:
                return "Garstufe " + ChefZ_VanillaStage.Name(facts.vanillaFoodStage) + " != " + ChefZ_VanillaStage.Name(vanillaStageValue);

            case ChefZ_SelectorOp.LIQUID:
                return "kein passender Fluessigkeitsbehaelter";

            case ChefZ_SelectorOp.ANY_OF:
                return "keine der " + ChildCount().ToString() + " Alternativen passt";

            case ChefZ_SelectorOp.ALL_OF:
                return AllOfReason(facts);

            case ChefZ_SelectorOp.NOT:
                return "ausgeschlossen durch not";
        }
        return "passt nicht";
    }

    private string AllOfReason(notnull ChefZ_ItemFacts facts)
    {
        if (children)
        {
            for (int i = 0; i < children.Count(); i++)
            {
                string childReason;
                if (children.Get(i).Explain(facts, childReason))
                    continue;
                return childReason;
            }
        }
        return "passt nicht";
    }

    private int ChildCount()
    {
        if (!children)
            return 0;
        return children.Count();
    }

    //==========================================================================
    // Diagnose
    //==========================================================================

    static string OpName(ChefZ_SelectorOp o)
    {
        switch (o)
        {
            case ChefZ_SelectorOp.TRUE_OP:       return "TRUE";
            case ChefZ_SelectorOp.CLASS:         return "CLASS";
            case ChefZ_SelectorOp.CATEGORY:      return "CATEGORY";
            case ChefZ_SelectorOp.TAG:           return "TAG";
            case ChefZ_SelectorOp.STATE:         return "STATE";
            case ChefZ_SelectorOp.VANILLA_STAGE: return "VANILLA_STAGE";
            case ChefZ_SelectorOp.ANY_OF:        return "ANY_OF";
            case ChefZ_SelectorOp.ALL_OF:        return "ALL_OF";
            case ChefZ_SelectorOp.NOT:           return "NOT";
            case ChefZ_SelectorOp.LIQUID:        return "LIQUID";
        }
        return "?";
    }

    //! Einzeiler in Praefixschreibweise, z.B.
    //! ALL_OF(CATEGORY:MEAT, STATE:SALTED)+freshness[0.5..*]
    string ToDebugString()
    {
        string s = OpName(op);
        int i;

        if (op == ChefZ_SelectorOp.CATEGORY)
        {
            s = s + ":" + ChefZ_SymbolTable.NameOrMark(sym) + "#" + categoryBitIndex.ToString();
        }
        else if (op == ChefZ_SelectorOp.VANILLA_STAGE)
        {
            s = s + ":" + ChefZ_VanillaStage.Name(vanillaStageValue);
        }
        else if (ChefZ_SymbolTable.IsValid(sym))
        {
            s = s + ":" + ChefZ_SymbolTable.Name(sym);
        }

        if (children && children.Count() > 0)
        {
            s = s + "(";
            for (i = 0; i < children.Count(); i++)
            {
                if (i > 0)
                    s = s + ", ";
                s = s + children.Get(i).ToDebugString();
            }
            s = s + ")";
        }

        if (negated)
            s = s + "(" + negated.ToDebugString() + ")";

        if (ChefZ_SymbolTable.IsValid(minQualitySym))
            s = s + "+minQuality:" + ChefZ_SymbolTable.Name(minQualitySym);

        if (ranges)
        {
            for (i = 0; i < ranges.Count(); i++)
                s = s + "+" + ranges.Get(i).ToDebugString();
        }

        return s;
    }
}
