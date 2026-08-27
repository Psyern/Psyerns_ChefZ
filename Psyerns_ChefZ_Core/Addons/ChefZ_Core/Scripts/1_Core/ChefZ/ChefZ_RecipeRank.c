//==============================================================================
// ChefZ_RecipeRank - die Rangzahl eines Rezepts, einmal beim Build berechnet
//
// Entwurf: 09 §3 (Feldliste und Compare woertlich), 09 §4.3 (die vier
// Schluessel), 09 E1 (berechnete Spezifitaet als Primaerschluessel), 09 E2
// (Sortierung beim Build, nicht zur Laufzeit), 09 §5 (Compare darf NIE auf
// Uhrzeit, Zufall oder map-Iterationsreihenfolge zurueckgreifen).
//
// ---------------------------------------------------------------------------
// Warum die id der letzte Schluessel ist
// ---------------------------------------------------------------------------
// 09 §4.4 nennt Stufe 4 "inhaltlich sinnlos und trotzdem wichtiger als eine
// kluegere Regel": sie macht Compare zu einer TOTALEN Ordnung. Ein Server und
// der Testserver des Entwicklers muessen dasselbe Gericht produzieren, sonst
// sind Fehlerberichte wertlos. Die Ladereihenfolge haengt an der
// Dateiauflistung des Betriebssystems und ist damit nicht portabel; der Name
// ist es.
//
// Gibt Stufe 4 je den Ausschlag, ist das ein CONTENT-Problem, und
// ChefZ_RecipeRanker.ReportAmbiguities() meldet es beim Boot.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_RecipeRank
{
    ChefZ_Sym recipeSym;

    //! Beim Build berechnet, haengt NUR vom Rezept ab (09 §4.1).
    float     specificity;

    //! Aus dem Record, Default 0 (09 §7: "priority fehlt -> 0, KEIN WARN").
    int       priority;

    int       requiredSlots;
    int       totalConstraints;
    string    id;

    //! Position im Rezeptarray der Engine. Gehoert nicht zur Ordnung, sondern
    //! ist ihr Ergebnis: der Index IST nach dem Sortieren die Rangreihenfolge.
    int       recipeIndex;

    void ChefZ_RecipeRank()
    {
        recipeSym        = ChefZ_SymbolTable.INVALID;
        specificity      = 0.0;
        priority         = 0;
        requiredSlots    = 0;
        totalConstraints = 0;
        id               = "";
        recipeIndex      = -1;
    }

    void InitFrom(notnull ChefZ_CompiledRecipe recipe, int index)
    {
        recipeSym        = recipe.recipeSym;
        specificity      = recipe.specificity;
        priority         = recipe.priority;
        requiredSlots    = recipe.requiredSlots;
        totalConstraints = recipe.totalConstraints;
        id               = recipe.id;
        recipeIndex      = index;
    }

    /**
     * Lexikografisch ueber die vier Schluessel aus 09 §4.3:
     *
     *     1. specificity      absteigend
     *     2. priority         absteigend
     *     3. totalConstraints absteigend   mehr Bedingungen = engeres Rezept
     *     4. id               aufsteigend  rein deterministischer Schlussanker
     *
     * Rueckgabe < 0, wenn a VOR b steht; > 0, wenn dahinter; 0 nur bei
     * gleicher id - und das heisst, dass zwei Records dieselbe ID tragen, was
     * die Registry bereits verhindert hat.
     */
    static int Compare(notnull ChefZ_RecipeRank a, notnull ChefZ_RecipeRank b)
    {
        if (a.specificity > b.specificity)              return -1;
        if (a.specificity < b.specificity)              return 1;

        if (a.priority > b.priority)                    return -1;
        if (a.priority < b.priority)                    return 1;

        if (a.totalConstraints > b.totalConstraints)    return -1;
        if (a.totalConstraints < b.totalConstraints)    return 1;

        return ChefZ_StringOrder.Compare(a.id, b.id);
    }

    //! Sind die drei Zahlen identisch? Dann entscheidet nur noch die id, und
    //! 09 §7 verlangt dafuer ein WARN beim Build.
    static bool SameRank(notnull ChefZ_RecipeRank a, notnull ChefZ_RecipeRank b)
    {
        if (a.specificity != b.specificity)             return false;
        if (a.priority != b.priority)                   return false;
        if (a.totalConstraints != b.totalConstraints)   return false;
        return true;
    }

    string ToDebugString()
    {
        return id
             + "  spez=" + specificity.ToString()
             + " prio=" + priority.ToString()
             + " pflichtslots=" + requiredSlots.ToString()
             + " bedingungen=" + totalConstraints.ToString();
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_RecipeRank a = new ChefZ_RecipeRank();
        ChefZ_RecipeRank b = new ChefZ_RecipeRank();

        a.id = "A"; b.id = "B";
        a.specificity = 5.0; b.specificity = 2.0;
        if (Compare(a, b) >= 0)                     return false;   // Spezifitaet schlaegt alles
        if (Compare(b, a) <= 0)                     return false;

        // priority bricht nur bei gleicher Spezifitaet
        b.specificity = 5.0;
        b.priority = 10;
        if (Compare(a, b) <= 0)                     return false;

        // totalConstraints danach
        b.priority = 0;
        a.totalConstraints = 3;
        if (Compare(a, b) >= 0)                     return false;

        // id zuletzt - und immer entscheidend
        a.totalConstraints = 0;
        if (SameRank(a, b) == false)                return false;
        if (Compare(a, b) >= 0)                     return false;   // "A" vor "B"
        if (Compare(a, a) != 0)                     return false;

        return true;
    }
}
