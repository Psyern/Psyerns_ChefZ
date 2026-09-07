//==============================================================================
// ChefZ_ConsumeMode / ChefZ_CompiledSlot / ChefZ_ConsumePlan
//
// Entwurf: 07 §2.3 (Slot und Verbrauchsplan, Feldlisten woertlich), 07 §4
// (der Slot ist die Einheit der Zuordnung), 07 §6 (Verbrauchsplan gilt bis zur
// Anwendung im selben Tick), 07 E2, E3, E5, 05 §6 (Einheiten).
//
// Der kompilierte Slot ist die Bootzeit-Form: Symbole statt Strings, Zahlen
// statt Sentinel, jeder Default entschieden. Nach dem Kompilieren wird er
// nicht mehr veraendert - alles Veraenderliche liegt im Snapshot und im
// Ergebnisobjekt.
//
// KEIN CONTENT: kein Zustand, keine Kategorie, keine Einheit wird hier
// benannt. "whole", "amount" und "none" sind Vokabular des Core, keine
// Spielinhalte.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_ConsumeMode
{
    static const int WHOLE  = 0;    // Item wird geloescht
    static const int AMOUNT = 1;    // Menge wird abgezogen, Rest bleibt liegen
    static const int NONE   = 2;    // nichts wird verbraucht (Werkzeug, Katalysator)

    static const string WHOLE_NAME  = "whole";
    static const string AMOUNT_NAME = "amount";
    static const string NONE_NAME   = "none";

    //! -1, wenn der Name unbekannt ist. Der Compiler macht daraus ein WARN und
    //! nimmt "whole" - ein Tippfehler soll kein Rezept sprengen, aber auch
    //! nicht stillschweigend zu "nichts verbrauchen" werden.
    static int FromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToLower();

        if (n == WHOLE_NAME)    return WHOLE;
        if (n == AMOUNT_NAME)   return AMOUNT;
        if (n == NONE_NAME)     return NONE;
        return -1;
    }

    static string Name(int mode)
    {
        switch (mode)
        {
            case WHOLE:  return WHOLE_NAME;
            case AMOUNT: return AMOUNT_NAME;
            case NONE:   return NONE_NAME;
        }
        return "?";
    }

    static string ValidNames()
    {
        return "whole, amount, none";
    }
}

//------------------------------------------------------------------------------

/**
 * Ein Slot, fertig kompiliert.
 *
 * Zur Bedeutung von allowPartial (07 §2.3 "Default true: Menge aus mehreren
 * Items sammeln", 07 E3 "fuer Faelle, in denen ein ganzes Stueck verlangt
 * ist"):
 *
 *   true   Die geforderte Menge darf ueber mehrere zugewiesene Items summiert
 *          werden. Zwei halbvolle Mehlbeutel ergeben einen vollen.
 *   false  JEDES zugewiesene Item muss die Menge fuer sich allein tragen, und
 *          der Verbrauchsplan zieht keine Bruchteile ab. Das ist die
 *          Formulierung fuer "ein ganzes Stueck".
 *
 * Diese Lesart ist die einzige, die beide Stellen des Entwurfs erfuellt, und
 * sie steht hier ausdruecklich, weil sie eine Auslegung ist.
 */
class ChefZ_CompiledSlot : Managed
{
    //! Index in der DEKLARATIONSreihenfolge des Rezepts. Die Suchreihenfolge
    //! ist eine andere (07 §4, Schritt 2); der Verbrauch folgt der
    //! Deklaration (07 §4, Schritt 6).
    int       slotIndex;

    string    slotId;
    ChefZ_Sym slotIdSym;

    ref ChefZ_CompiledSelector selector;

    int   minCount;
    int   maxCount;

    //! Mengenforderung in Rezepteinheiten (05 §6). null = keine.
    ref ChefZ_Range amount;

    //! Einheitensymbol. INVALID = beliebig. Der Core rechnet NIE zwischen
    //! Einheiten um (05 §6): ein Slot mit GRAM matcht nur Items, deren
    //! quantityUnit dasselbe Symbol ist.
    ChefZ_Sym unitSym;

    bool  optional;
    bool  allowPartial;

    int   consumeMode;
    float consumeAmount;
    ChefZ_Sym setStateAfter;     // INVALID = kein Zustandswechsel

    int   gradePoints;

    //! Zustaende, die diesen Slot NICHT bedienen duerfen (07 E5). Nie null;
    //! leer heisst "der Autor laesst ausdruecklich alles zu".
    ref array<ChefZ_Sym> excludeStates;

    /**
     * ROHE Spezifitaet des Selektors (09 §4.1).
     *
     * Ausdruecklich OHNE die beiden Slotfaktoren: min(minCount, amountCap) fuer
     * Pflichtslots und wOptionalSlot fuer optionale. Die gehoeren in die
     * Rezeptrechnung (09 §4.1, S6), weil dort amountCap bekannt ist - hier
     * waeren sie zweimal angewandt, sobald jemand die Formel liest und
     * anwendet.
     */
    float specificity;

    int   selectivityHint;       // 07 E4, Slotreihenfolge des Matchers

    void ChefZ_CompiledSlot()
    {
        slotIndex       = -1;
        slotId          = "";
        slotIdSym       = ChefZ_SymbolTable.INVALID;
        selector        = null;
        minCount        = 1;
        maxCount        = 1;
        amount          = null;
        unitSym         = ChefZ_SymbolTable.INVALID;
        optional        = false;
        allowPartial    = true;
        consumeMode     = ChefZ_ConsumeMode.WHOLE;
        consumeAmount   = 0.0;
        setStateAfter   = ChefZ_SymbolTable.INVALID;
        gradePoints     = 0;
        excludeStates   = new array<ChefZ_Sym>();
        specificity     = 0.0;
        selectivityHint = 0;
    }

    bool IsStateExcluded(ChefZ_Sym state)
    {
        if (!ChefZ_SymbolTable.IsValid(state))
            return false;           // "kein Zustand" ist nie ausgeschlossen
        if (!excludeStates)
            return false;
        return excludeStates.Find(state) >= 0;
    }

    bool HasAmountRequirement()
    {
        if (!amount)
            return false;
        return !amount.IsUnbounded();
    }

    //! Untergrenze der Mengenforderung; 0, wenn keine gestellt ist.
    float RequiredUnits()
    {
        if (!amount || !amount.HasMin())
            return 0.0;
        return amount.min;
    }

    string ToDebugString()
    {
        string s = slotId;
        if (s == "")
            s = "#" + slotIndex.ToString();

        s = s + " " + Counts();

        if (selector)
            s = s + " " + selector.ToDebugString();
        if (amount)
            s = s + " menge=" + amount.ToDebugString();
        if (ChefZ_SymbolTable.IsValid(unitSym))
            s = s + " " + ChefZ_SymbolTable.Name(unitSym);
        if (optional)
            s = s + " optional";
        if (!allowPartial)
            s = s + " ganzeStuecke";

        s = s + " verbrauch=" + ChefZ_ConsumeMode.Name(consumeMode);
        if (consumeMode == ChefZ_ConsumeMode.AMOUNT)
            s = s + "(" + consumeAmount.ToString() + ")";
        if (ChefZ_SymbolTable.IsValid(setStateAfter))
            s = s + " danach=" + ChefZ_SymbolTable.Name(setStateAfter);
        if (gradePoints != 0)
            s = s + " punkte=" + gradePoints.ToString();

        s = s + " hint=" + selectivityHint.ToString() + " spez=" + specificity.ToString();

        if (excludeStates && excludeStates.Count() > 0)
        {
            s = s + " ohne=[";
            for (int i = 0; i < excludeStates.Count(); i++)
            {
                if (i > 0)
                    s = s + ",";
                s = s + ChefZ_SymbolTable.Name(excludeStates.Get(i));
            }
            s = s + "]";
        }

        return s;
    }

    private string Counts()
    {
        if (minCount == maxCount)
            return "x" + minCount.ToString();
        return "x" + minCount.ToString() + ".." + maxCount.ToString();
    }
}

//------------------------------------------------------------------------------

/**
 * Was mit EINEM Item geschehen soll, wenn das Rezept angewandt wird
 * (07 §2.3).
 *
 * Der Plan entsteht im Matcher (1_Core, ohne jeden Zugriff auf Entities) und
 * wird vom Applicator (4_World, S8) ausgefuehrt. Zwischen beiden liegt die
 * Handle-Aufloesung, die der Applicator vor der Ausfuehrung noch einmal prueft
 * (08 §6): ein Item, das zwischen Match und Anwendung verschwunden ist, darf
 * nicht dazu fuehren, dass irgendetwas verbraucht wird.
 *
 * quantityDelta ist ein BETRAG in Vanilla-Quantity, immer >= 0, und wird
 * abgezogen. Nicht in Rezepteinheiten: der Applicator ruft AddQuantity() und
 * soll nicht selbst umrechnen muessen. Die Umrechnung Einheit -> Quantity
 * geschieht dort, wo beide Zahlen vorliegen, naemlich im Snapshot (05 §6).
 */
class ChefZ_ConsumePlan : Managed
{
    int       handle;
    bool      destroyWhole;
    float     quantityDelta;      // Vanilla-Quantity, >= 0, wird abgezogen
    float     unitsDelta;         // dasselbe in Rezepteinheiten, fuer den Trace
    ChefZ_Sym setStateAfter;      // INVALID = kein Zustandswechsel
    int       slotIndex;          // fuer Trace und Qualitaetsregeln (12)

    void ChefZ_ConsumePlan()
    {
        handle        = -1;
        destroyWhole  = false;
        quantityDelta = 0.0;
        unitsDelta    = 0.0;
        setStateAfter = ChefZ_SymbolTable.INVALID;
        slotIndex     = -1;
    }

    //! true, wenn dieser Eintrag ueberhaupt etwas bewirkt. Ein Plan ohne
    //! Wirkung wird gar nicht erst erzeugt; die Auskunft steht fuer den
    //! Applicator bereit, der jeden Eintrag noch einmal pruefen soll.
    bool HasEffect()
    {
        if (destroyWhole)
            return true;
        if (quantityDelta > 0.0)
            return true;
        return ChefZ_SymbolTable.IsValid(setStateAfter);
    }

    string ToDebugString()
    {
        string s = "#" + handle.ToString();
        if (destroyWhole)
            s = s + " loeschen";
        if (quantityDelta > 0.0)
            s = s + " -" + quantityDelta.ToString() + " (" + unitsDelta.ToString() + " Einheiten)";
        if (ChefZ_SymbolTable.IsValid(setStateAfter))
            s = s + " -> " + ChefZ_SymbolTable.Name(setStateAfter);
        return s;
    }
}
