//==============================================================================
// ChefZ_QualityScoringDef - die Stellschrauben der Punktrechnung, wie sie in
// Core.json stehen
//
// Entwurf: 12 §4 (die Formel und ihre freien Groessen), 12 §4.2
// (Zustandsstrafen), 02 E3 (Sentinel als Erkennungsmerkmal fuer "nicht
// gesetzt"), 09 §7 sinngemaess ("fehlende Felder aus Code-Defaults").
//
// ---------------------------------------------------------------------------
// Warum es diese zweite Klasse gibt
// ---------------------------------------------------------------------------
// Wortgleich zur Begruendung bei ChefZ_PriorityWeightsDef:
// ChefZ_QualityScoring traegt fertige Werte mit Code-Defaults im Konstruktor.
// Genau das macht sie als JSON-Ziel unbrauchbar - der Enforce-Serializer
// fuellt jedes nicht genannte Feld mit dem TYPdefault (0.0), und ein
// Betreiber, der nur die Frische staerker gewichten will, haette danach
// unbemerkt die Zutatenqualitaet auf null.
//
// ApplyTo() ueberschreibt ausschliesslich, was tatsaechlich in der Datei
// stand. Eine Datei ohne "qualityScoring"-Block laesst die Rechnung damit
// vollstaendig unveraendert.
//
// ---------------------------------------------------------------------------
// Die Ausnahme: statePenalties
// ---------------------------------------------------------------------------
// Die Strafenliste wird als GANZES uebernommen, nicht eintragsweise gemischt.
// Eine Liste ist eine Aussage als Ganzes (dieselbe Regel wie fuer jedes
// ref-Array in 02 E3): wer sie schreibt, meint sie - und wer sie leer
// schreibt, schaltet die Strafen ausdruecklich ab.
//
// KEIN CONTENT: kein Zustandsname steht in dieser Datei.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_QualityScoringDef : Managed
{
    string defaultTierSet;
    float  freshnessWeight;
    float  ingredientQualityWeight;
    float  baseRank;

    //! null = kein Block geschrieben (02 E3, Mittel 1). Ein LEERES Array ist
    //! etwas anderes und heisst "ausdruecklich keine Strafen".
    ref array<ref ChefZ_StatePenaltyDef> statePenalties;

    //! Wie in ChefZ_PriorityWeightsDef und ChefZ_OutputDef: seit
    //! ChefZ_Undefined.FLOAT == 0.0 ist eine geschriebene Null am Wert
    //! allein nicht mehr von einem fehlenden Feld zu unterscheiden, und
    //! genau das verlangt der Selbsttest unten ("ausdrueckliche 0").
    //! Gefuellt heute nur von Hand - qualityScoring ist ein Unterobjekt
    //! von ChefZ_CoreSettingsDef und von ChefZ_JsonExplicit nicht erfasst.
    ref array<string> explicitFields;

    void ChefZ_QualityScoringDef()
    {
        defaultTierSet          = ChefZ_Undefined.TEXT;
        freshnessWeight         = ChefZ_Undefined.FLOAT;
        ingredientQualityWeight = ChefZ_Undefined.FLOAT;
        baseRank                = ChefZ_Undefined.FLOAT;
        statePenalties          = null;
        explicitFields          = null;
    }

    void Normalize()
    {
        defaultTierSet.TrimInPlace();
        if (!statePenalties)
            return;
        for (int i = 0; i < statePenalties.Count(); i++)
        {
            ChefZ_StatePenaltyDef p = statePenalties.Get(i);
            if (p)
                p.Normalize();
        }
    }

    /**
     * Traegt die gesetzten Felder in einen fertigen Satz ein.
     *
     * Rueckgabe: wie viele Angaben tatsaechlich aus der Datei kamen - die
     * Strafenliste zaehlt als EINE. Der Aufrufer schreibt die Zahl in den
     * Ladebericht, damit ein Betreiber sieht, ob sein Block ueberhaupt
     * angekommen ist. Ein Tippfehler im Blocknamen ist sonst nicht von
     * "wirkt wie erwartet" zu unterscheiden.
     *
     * report darf null sein (Selbsttest).
     */
    int ApplyTo(notnull ChefZ_QualityScoring sc, ChefZ_LoadReport report)
    {
        int n = 0;

        if (!ChefZ_Undefined.IsTextUndefined(defaultTierSet))
        {
            sc.defaultTierSet = defaultTierSet;
            n++;
        }
        if (IsSet("freshnessWeight", freshnessWeight))
        {
            sc.freshnessWeight = freshnessWeight;
            n++;
        }
        if (IsSet("ingredientQualityWeight", ingredientQualityWeight))
        {
            sc.ingredientQualityWeight = ingredientQualityWeight;
            n++;
        }
        if (IsSet("baseRank", baseRank))
        {
            sc.baseRank = baseRank;
            n++;
        }

        if (statePenalties)
        {
            sc.ClearStatePenalties();
            ApplyPenalties(sc, report);
            n++;
        }

        return n;
    }

    private void ApplyPenalties(notnull ChefZ_QualityScoring sc, ChefZ_LoadReport report)
    {
        for (int i = 0; i < statePenalties.Count(); i++)
        {
            ChefZ_StatePenaltyDef p = statePenalties.Get(i);
            if (!p)
                continue;

            if (!p.IsUsable())
            {
                // Ein halber Eintrag ist immer ein Tippfehler und nie eine
                // Absicht: ohne "state" trifft er nichts, ohne "points" tut er
                // nichts. Beides still zu schlucken hiesse, dem Betreiber eine
                // wirksame Strafe vorzugaukeln.
                if (report)
                {
                    report.AddError("Core.json / qualityScoring", "statePenalties[" + i.ToString() + "]", "Eintrag ohne \"state\" oder ohne \"points\" - er wird ausgelassen. " + "Form: { \"state\": \"<Zustands-ID>\", \"points\": -3.0 }.");
                }
                continue;
            }

            if (sc.HasStatePenalty(ChefZ_SymbolTable.Lookup(p.state)) && report)
            {
                report.AddWarn("Core.json / qualityScoring", p.state, "Zustandsstrafe steht mehrfach in der Liste. Der letzte Eintrag gewinnt.");
            }

            sc.SetStatePenalty(p.state, p.points);
        }
    }

    //! Steht dieses Feld tatsaechlich in der Datei? Eine ausdrueckliche 0 ist
    //! ein Wert und wird uebernommen - nur der Sentinel heisst "nicht gesetzt".
    private bool IsSet(string field, float value)
    {
        if (HasExplicit(field))
            return true;
        return !ChefZ_Undefined.IsFloatUndefined(value);
    }

    void MarkExplicit(string field)
    {
        if (!explicitFields)
            explicitFields = new array<string>();
        if (explicitFields.Find(field) < 0)
            explicitFields.Insert(field);
    }

    bool HasExplicit(string field)
    {
        if (!explicitFields)
            return false;
        return explicitFields.Find(field) >= 0;
    }

    //! Nur fuer den Selbsttest (S10).
    static bool SelfCheck()
    {
        ChefZ_QualityScoring sc = new ChefZ_QualityScoring();

        // Leerer Block: nichts wird angefasst.
        ChefZ_QualityScoringDef empty = new ChefZ_QualityScoringDef();
        if (empty.ApplyTo(sc, null) != 0)                   return false;
        if (sc.freshnessWeight != 1.0)                      return false;
        if (sc.ingredientQualityWeight != 0.5)              return false;

        // Zwei Felder gesetzt: genau zwei aendern sich.
        ChefZ_QualityScoringDef partial = new ChefZ_QualityScoringDef();
        partial.freshnessWeight = 2.5;
        partial.baseRank        = 0.0;                      // ausdrueckliche 0
        partial.MarkExplicit("baseRank");
        if (partial.ApplyTo(sc, null) != 2)                 return false;
        if (sc.freshnessWeight != 2.5)                      return false;
        if (sc.baseRank != 0.0)                             return false;
        if (sc.ingredientQualityWeight != 0.5)              return false;   // unberuehrt

        // Strafenliste: als Ganzes uebernommen, halbe Eintraege ausgelassen.
        ChefZ_QualityScoringDef pens = new ChefZ_QualityScoringDef();
        pens.statePenalties = new array<ref ChefZ_StatePenaltyDef>();

        ChefZ_StatePenaltyDef ok = new ChefZ_StatePenaltyDef();
        ok.state  = "  CHEFZ_QSD_VERBRANNT  ";
        ok.points = -3.0;
        pens.statePenalties.Insert(ok);

        ChefZ_StatePenaltyDef half = new ChefZ_StatePenaltyDef();
        half.state = "CHEFZ_QSD_OHNEPUNKTE";
        pens.statePenalties.Insert(half);

        pens.Normalize();
        if (ok.state != "CHEFZ_QSD_VERBRANNT")              return false;
        if (pens.ApplyTo(sc, null) != 1)                    return false;
        if (sc.StatePenaltyCount() != 1)                    return false;

        ChefZ_Sym burnt = ChefZ_SymbolTable.Lookup("CHEFZ_QSD_VERBRANNT");
        if (sc.GetStatePenalty(burnt) != -3.0)              return false;

        // Ausdruecklich leere Liste schaltet die Strafen ab.
        ChefZ_QualityScoringDef none = new ChefZ_QualityScoringDef();
        none.statePenalties = new array<ref ChefZ_StatePenaltyDef>();
        if (none.ApplyTo(sc, null) != 1)                    return false;
        if (sc.StatePenaltyCount() != 0)                    return false;

        return true;
    }
}
