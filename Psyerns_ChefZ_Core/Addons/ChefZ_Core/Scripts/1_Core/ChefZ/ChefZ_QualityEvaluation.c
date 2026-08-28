//==============================================================================
// ChefZ_QualityEvaluation - das vollstaendig inspizierbare Zwischenergebnis
//
// Entwurf: 12 §5 (Feldliste woertlich), 12 §4 (die Formel, deren Summanden
// diese Felder sind), 12 E1 ("Der Preis: die Stufe ist weniger exakt
// vorhersagbar. Gegenmittel: ChefZ_QualityEvaluation ist VOLLSTAENDIG
// inspizierbar und wird bei aktivem Kanal QUALITY Term fuer Term ins Trace
// geschrieben"), 12 §7 (wird NICHT persistiert), 18 §3.
//
// ---------------------------------------------------------------------------
// Warum dieses Objekt der eigentliche Gegenwert von 12 E1 ist
// ---------------------------------------------------------------------------
// Eine additive Punktrechnung ueber sieben Summanden ist maechtig und
// erweiterbar, aber sie ist nicht mehr im Kopf nachzurechnen. Die
// Gegenleistung, die der Entwurf dafuer verlangt, ist Nachvollziehbarkeit:
// jeder Summand steht einzeln, jede Zahl hat eine Zeile im Klartext daneben,
// und beides wandert 1:1 ins Trace.
//
// Deshalb sind die Felder EINZELN und nicht als Summe gefuehrt - eine
// Gesamtpunktzahl ohne ihre Herkunft beantwortet die einzige Frage nicht, die
// je gestellt wird: "warum ist mein Eintopf nur SIMPLE?"
//
// ---------------------------------------------------------------------------
// Lebensdauer
// ---------------------------------------------------------------------------
// Laufzeit, bis zur Anwendung. NICHT persistiert und NICHT gesynct (12 §7):
// die Punktzahl ist ein Zwischenergebnis, und wer sie speichert, muss sie bei
// jeder Regelaenderung migrieren. Persistiert wird allein die Stufe.
//
// Layer: 1_Core. Reine Daten, kein Engine-Typ, kein Itemzugriff.
//==============================================================================

class ChefZ_QualityEvaluation
{
    //--- die Summanden aus 12 §4, in der Reihenfolge der Formel ---------------
    float SlotPoints;               // SUM ueber belegte Slots: slot.gradePoints
    float RulePoints;               // SUM ueber ausgewertete gradeRules
    float FreshnessTerm;            // (minFreshness - 0.5) * 2 * freshnessWeight
    float IngredientQualityTerm;    // (mittlerer Rang - baseRank) * Gewicht
    float StatePenalty;             // SUM ueber Zutaten, additiv, meist negativ
    float Bias;                     // recipe.qualityBias
    float ExternalBonus;            // geklemmt auf maxExternalQualityBonus (17)

    //! Geraetebonus aus ChefZ_DeviceDef.qualityModifier. MULTIPLIKATIV auf die
    //! Summe der uebrigen Terme (12 §4, letzte Zeile der Formel) - deshalb
    //! steht er getrennt und wird nicht mitaddiert.
    float DeviceModifier;

    float TotalScore;
    ChefZ_Sym ResultTier;

    //--- Diagnose, ueber 12 §5 hinaus ----------------------------------------
    //
    // Zwei Zahlen, die in KEINEN Summanden eingehen, aber jede Rueckfrage
    // beantworten: welche Zutat war die schlechteste, und wie viele wurden
    // ueberhaupt betrachtet. Sie stehen hier und nicht nur in einer Note,
    // weil ein Admin-Kommando sie ausrechnen koennen soll, ohne Text zu
    // zerlegen.
    float MinFreshness;             // -1.0 = keine Zutat betrachtet
    int   ConsideredItems;

    //! Menschenlesbare Begruendung je Term (12 §5). Nie null.
    ref array<string> Notes;

    void ChefZ_QualityEvaluation()
    {
        Notes = new array<string>();
        Reset();
    }

    /**
     * Auf "nichts gerechnet" zuruecksetzen.
     *
     * Die Notizen werden GELEERT, nicht neu angelegt: das Objekt wird je
     * Gefaess wiederverwendet, genau wie ChefZ_MatchResult und
     * ChefZ_CookContext (08 §7).
     */
    void Reset()
    {
        SlotPoints            = 0.0;
        RulePoints            = 0.0;
        FreshnessTerm         = 0.0;
        IngredientQualityTerm = 0.0;
        StatePenalty          = 0.0;
        Bias                  = 0.0;
        ExternalBonus         = 0.0;
        DeviceModifier        = 1.0;
        TotalScore            = 0.0;
        ResultTier            = ChefZ_SymbolTable.INVALID;
        MinFreshness          = -1.0;
        ConsideredItems       = 0;
        Notes.Clear();
    }

    //! Summe der ADDITIVEN Terme, ohne den Geraetefaktor. Getrennt gefuehrt,
    //! damit der Trace beide Zahlen zeigen kann - "8.0 x 1.1 = 8.8" erklaert
    //! sich selbst, "8.8" nicht.
    float AdditiveSum()
    {
        return SlotPoints
             + RulePoints
             + FreshnessTerm
             + IngredientQualityTerm
             + StatePenalty
             + Bias
             + ExternalBonus;
    }

    void AddNote(string text)
    {
        if (text == "")
            return;
        Notes.Insert(text);
    }

    /**
     * Ist das ueberhaupt eine Zahl? (12 §8, Zeile "Score ist NaN")
     *
     * Enforce hat keine Math.IsNaN - Vanilla kennt sie nirgends. Die Pruefung
     * nutzt deshalb die definierende Eigenschaft: NaN ist der einzige Wert,
     * der sich selbst ungleich ist. Der zweite Teil faengt Unendlichkeiten
     * ab, die aus einer Division in einer kuenftigen Regel entstehen koennten.
     *
     * Wichtig ist nicht die Eleganz, sondern die Folge: ein NaN in der
     * Punktzahl wuerde JEDEN Schwellenvergleich false ergeben lassen, und das
     * Gericht bekaeme lautlos gar keine Stufe.
     */
    static bool IsFinite(float v)
    {
        if (v != v)
            return false;
        if (v > float.MAX || v < float.LOWEST)
            return false;
        return true;
    }

    bool HasTier()
    {
        return ChefZ_SymbolTable.IsValid(ResultTier);
    }

    //==========================================================================
    // Ausgabe
    //==========================================================================

    //! Eine Zeile fuer das Log. Alle Summanden, in der Reihenfolge der Formel.
    string ToLine()
    {
        string s = "score=" + TotalScore.ToString()
                 + "  stufe=" + ChefZ_SymbolTable.NameOrMark(ResultTier)
                 + "  slots=" + SlotPoints.ToString()
                 + " regeln=" + RulePoints.ToString()
                 + " frische=" + FreshnessTerm.ToString()
                 + " zutatenqualitaet=" + IngredientQualityTerm.ToString()
                 + " zustandsstrafe=" + StatePenalty.ToString()
                 + " bias=" + Bias.ToString()
                 + " extern=" + ExternalBonus.ToString()
                 + " geraet=x" + DeviceModifier.ToString();
        return s;
    }

    /**
     * Der Block fuer Kanal QUALITY (12 E1): Term fuer Term, dann die Notizen.
     *
     * outLines wird ERGAENZT und nicht geleert - der Aufrufer sammelt oft
     * mehrere Bloecke, und ein Aufraeumen an dieser Stelle waere eine
     * Ueberraschung.
     */
    void ToLines(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Qualitaet: " + ToLine());
        outLines.Insert("    Summe der additiven Terme = " + AdditiveSum().ToString() + ", mal Geraetefaktor " + DeviceModifier.ToString() + " = " + TotalScore.ToString());

        if (ConsideredItems > 0)
        {
            outLines.Insert("    betrachtete Zutaten = " + ConsideredItems.ToString() + ", geringste Frische = " + MinFreshness.ToString());
        }

        for (int i = 0; i < Notes.Count(); i++)
            outLines.Insert("    " + Notes.Get(i));
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest (S10).
    static bool SelfCheck()
    {
        ChefZ_QualityEvaluation e = new ChefZ_QualityEvaluation();
        if (e.TotalScore != 0.0)                        return false;
        if (e.DeviceModifier != 1.0)                    return false;
        if (e.HasTier())                                return false;
        if (e.Notes.Count() != 0)                       return false;
        if (e.AdditiveSum() != 0.0)                     return false;
        if (e.MinFreshness != -1.0)                     return false;

        e.SlotPoints    = 2.0;
        e.RulePoints    = 3.0;
        e.StatePenalty  = -1.0;
        e.Bias          = 0.5;
        if (e.AdditiveSum() != 4.5)                     return false;

        e.AddNote("");                                  // wird ausgelassen
        e.AddNote("Testnotiz");
        if (e.Notes.Count() != 1)                       return false;

        array<string> lines = new array<string>();
        lines.Insert("davor");
        e.ToLines(lines);
        if (lines.Get(0) != "davor")                    return false;   // nicht geleert
        if (lines.Count() < 4)                          return false;

        e.Reset();
        if (e.AdditiveSum() != 0.0)                     return false;
        if (e.Notes.Count() != 0)                       return false;
        if (e.DeviceModifier != 1.0)                    return false;

        if (!IsFinite(0.0))                             return false;
        if (!IsFinite(-99.0))                           return false;
        if (!IsFinite(float.MAX))                       return false;
        if (!IsFinite(float.LOWEST))                    return false;

        return true;
    }
}
