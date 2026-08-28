//==============================================================================
// ChefZ_QualityScoring - die Stellschrauben der Punktrechnung
//
// Entwurf: 12 §4 (die Formel woertlich), 12 §4.1 (Frische geht als MINIMUM
// ein), 12 §4.2 (Zustandsstrafen sind additiv und duerfen unter die unterste
// Stufe druecken), 12 §8 (Fehlerverhalten), 09 §3 sinngemaess ("aus Core.json,
// nicht aus dem Code").
//
// ---------------------------------------------------------------------------
// Warum das eine eigene Klasse ist und keine Konstanten im Manager
// ---------------------------------------------------------------------------
// Die Formel aus 12 §4 hat drei freie Groessen - freshnessWeight,
// ingredientQualityWeight und den Bezugsrang -, und dazu eine Tabelle von
// Zustandsstrafen. Alle vier sind Balancing und gehoeren damit in JSON, nicht
// in den Code (Auflage des Projekts: "Alles Konfigurierbare gehoert in JSON").
//
// Dieselbe Zweiteilung wie bei den Spezifitaetsgewichten (09 §3):
//
//     ChefZ_QualityScoringDef   Rohform aus Core.json, jedes Feld auf Sentinel
//     ChefZ_QualityScoring      Ergebnis, jedes Feld gesetzt
//
// Die Defaults sind so gewaehlt, dass die Rechnung OHNE jede Konfiguration
// sinnvoll ist: ohne den Block wirkt Frische mit Gewicht 1.0, die
// Zutatenqualitaet mit 0.5, und es gibt keine Zustandsstrafen. Ein Server ohne
// Core.json bewertet damit weiterhin - nur eben ohne Feinabstimmung.
//
// KEIN CONTENT: keine Zahl hier nennt eine Kategorie, ein Gericht oder eine
// Zutat. Die Zustandsstrafen sind eine LEERE Liste - wer "BURNT: -3" will,
// schreibt es in Core.json oder in sein Content-Modul.
//
// Layer: 1_Core.
//==============================================================================

/**
 * Eine Zustandsstrafe: "wer diesen Zustand mitbringt, kostet so viele Punkte".
 *
 * ABWEICHUNG IN DER SCHREIBWEISE, keine in der Bedeutung: 12 §4.2 notiert die
 * Strafen als Objekt ({"BURNT": -3.0, "ROTTEN": -99.0}), also als Abbildung
 * Name -> Zahl. Ein map<string, float> als JSON-Ziel ist mit dem
 * Enforce-Serializer nirgends belegt; die JSON-Klassen in Vanilla sind
 * ausnahmslos Felder und Arrays. Deshalb hier eine Liste aus Paaren:
 *
 *     "statePenalties": [ { "state": "BURNT", "points": -3.0 } ]
 *
 * Die Bedeutung ist dieselbe. Der Preis sind ein paar Zeichen mehr in der
 * Datei; der Gewinn ist, dass der Block sicher gelesen wird - und ein still
 * NICHT gelesener Strafenblock waere die gefaehrlichste Sorte Fehler: alles
 * sieht richtig aus, nur verbranntes Fleisch kostet nichts mehr.
 */
class ChefZ_StatePenaltyDef
{
    string state;
    float  points;

    void ChefZ_StatePenaltyDef()
    {
        state  = ChefZ_Undefined.TEXT;
        points = ChefZ_Undefined.FLOAT;
    }

    void Normalize()
    {
        state.TrimInPlace();
    }

    bool IsUsable()
    {
        if (ChefZ_Undefined.IsTextUndefined(state))
            return false;
        return !ChefZ_Undefined.IsFloatUndefined(points);
    }
}

//==============================================================================

/**
 * Die fertigen Stellschrauben. Jedes Feld gesetzt, keine Sentinel.
 *
 * Die Zustandsstrafen liegen als Symboltabelle vor: der Manager schlaegt sie
 * je gebundener Zutat nach, und das ist der einzige Teil der Rechnung, der
 * ueber alle Items laeuft. Ein Stringvergleich waere dort die teuerste Zeile
 * des ganzen Kochticks.
 */
class ChefZ_QualityScoring
{
    /**
     * Vorgabesatz fuer Rezepte und Stufen ohne eigenen tierSet (12 §8, Zeile
     * "Rezept nennt unbekanntes qualityTierSet").
     *
     * Der Name steht im Entwurf und ist KEIN Content: der Core legt damit
     * keine Stufe an. Steht kein Content dahinter, gibt es diesen Satz
     * schlicht nicht, und jede Aufloesung liefert INVALID - genau wie bei
     * defaultExcludedStates (02 §5.4).
     */
    static const string DEFAULT_TIER_SET = "DISH_DEFAULT";

    string defaultTierSet;

    //! Gewicht des Frischeterms: (minFreshness - 0.5) * 2 * freshnessWeight.
    //! Bei 1.0 reicht der Term von -1 (voellig hinueber) bis +1 (taufrisch).
    float  freshnessWeight;

    //! Gewicht des Zutatenqualitaetsterms:
    //! (mittlerer Zutatenrang - baseRank) * ingredientQualityWeight.
    float  ingredientQualityWeight;

    /**
     * Bezugsrang der Zutatenqualitaet (12 §4, "- baseRank").
     *
     * 1.0, weil eine Leiter POOR/SIMPLE/PREPARED/SEASONED/PREMIUM den
     * Normalfall auf Rang 1 hat: Zutaten im Normalzustand sollen den Term
     * weder heben noch senken. Wer eine andere Leiter baut, verschiebt die
     * Zahl - deshalb steht sie in JSON und nicht im Code.
     */
    float  baseRank;

    private ref map<int, float> m_StatePenalty;    // ChefZ_Sym -> Punkte
    private ref array<ChefZ_Sym> m_PenaltyOrder;   // stabile Ausgabereihenfolge

    /**
     * Code-Defaults. Bewusst im Konstruktor und NICHT ueber Sentinel: diese
     * Klasse wird nicht direkt deserialisiert (siehe ChefZ_QualityScoringDef).
     */
    void ChefZ_QualityScoring()
    {
        defaultTierSet          = DEFAULT_TIER_SET;
        freshnessWeight         = 1.0;
        ingredientQualityWeight = 0.5;
        baseRank                = 1.0;

        m_StatePenalty = new map<int, float>();
        m_PenaltyOrder = new array<ChefZ_Sym>();
    }

    //==========================================================================
    // Zustandsstrafen
    //==========================================================================

    /**
     * Strafe eintragen. Der Name wird interniert, NICHT gegen die
     * Zustandsregistry geprueft.
     *
     * Dieselbe Regel wie bei defaultExcludedStates (07 E5): laeuft kein
     * Content, der diesen Zustand fuehrt, laeuft der Eintrag ins Leere - und
     * das ist harmlos. Ihn abzuweisen hiesse, einen Betreiber dafuer zu
     * bestrafen, dass er ein Modul spaeter dazuladen will.
     */
    void SetStatePenalty(string stateName, float points)
    {
        if (stateName == "")
            return;

        ChefZ_Sym sym = ChefZ_SymbolTable.Intern(stateName);
        if (!ChefZ_SymbolTable.IsValid(sym))
            return;

        if (!m_StatePenalty.Contains(sym))
            m_PenaltyOrder.Insert(sym);
        m_StatePenalty.Set(sym, points);
    }

    //! 0.0 heisst "keine Strafe" - und das ist auch die Antwort fuer einen
    //! Zustand, den niemand erwaehnt hat. Der haeufigste Fall ist der billige.
    float GetStatePenalty(ChefZ_Sym state)
    {
        if (!ChefZ_SymbolTable.IsValid(state))
            return 0.0;

        float points;
        if (m_StatePenalty.Find(state, points))
            return points;
        return 0.0;
    }

    bool HasStatePenalty(ChefZ_Sym state)
    {
        return m_StatePenalty.Contains(state);
    }

    int StatePenaltyCount()
    {
        return m_PenaltyOrder.Count();
    }

    void ClearStatePenalties()
    {
        m_StatePenalty.Clear();
        m_PenaltyOrder.Clear();
    }

    //==========================================================================

    /**
     * Klammert Unsinn, statt ihn abzuweisen (02 §8: jeder Fehler bewegt das
     * System Richtung weniger ChefZ, nie Richtung falsches ChefZ).
     *
     * Ein negatives Gewicht wuerde die Aussage umdrehen - frische Zutaten
     * ergaeben dann ein schlechteres Gericht als verdorbene. Das ist nie
     * gemeint, und ein Server, auf dem es so waere, ist schwerer zu erklaeren
     * als einer, auf dem Frische schlicht nicht zaehlt.
     *
     * Die Zustandsstrafen werden ausdruecklich NICHT geklammert: sie sollen
     * negativ sein (12 §4.2), und -99 ist dort ein legitimer Wert.
     */
    void ClampInPlace()
    {
        if (freshnessWeight < 0.0)
            freshnessWeight = 0.0;
        if (ingredientQualityWeight < 0.0)
            ingredientQualityWeight = 0.0;
        if (defaultTierSet == "")
            defaultTierSet = DEFAULT_TIER_SET;
    }

    string ToDebugString()
    {
        string s = "satz=" + defaultTierSet + " frische=" + freshnessWeight.ToString() + " zutatenqualitaet=" + ingredientQualityWeight.ToString() + " bezugsrang=" + baseRank.ToString() + " strafen=" + StatePenaltyCount().ToString();

        for (int i = 0; i < m_PenaltyOrder.Count(); i++)
        {
            ChefZ_Sym sym = m_PenaltyOrder.Get(i);
            s = s + "  " + ChefZ_SymbolTable.NameOrMark(sym) + ":" + GetStatePenalty(sym).ToString();
        }
        return s;
    }

    //! Nur fuer den Selbsttest (S10).
    static bool SelfCheck()
    {
        ChefZ_QualityScoring sc = new ChefZ_QualityScoring();
        if (sc.defaultTierSet != DEFAULT_TIER_SET)          return false;
        if (sc.freshnessWeight != 1.0)                      return false;
        if (sc.ingredientQualityWeight != 0.5)              return false;
        if (sc.baseRank != 1.0)                             return false;
        if (sc.StatePenaltyCount() != 0)                    return false;

        // Unbekannter Zustand kostet nichts.
        ChefZ_Sym unknown = ChefZ_SymbolTable.Intern("CHEFZ_QS_UNBEKANNT");
        if (sc.GetStatePenalty(unknown) != 0.0)             return false;
        if (sc.HasStatePenalty(unknown))                    return false;
        if (sc.GetStatePenalty(ChefZ_SymbolTable.INVALID) != 0.0) return false;

        sc.SetStatePenalty("CHEFZ_QS_VERBRANNT", -3.0);
        sc.SetStatePenalty("CHEFZ_QS_VERBRANNT", -4.0);      // ueberschreibt
        sc.SetStatePenalty("", -1.0);                        // wird ignoriert
        ChefZ_Sym burnt = ChefZ_SymbolTable.Lookup("CHEFZ_QS_VERBRANNT");
        if (sc.StatePenaltyCount() != 1)                     return false;
        if (sc.GetStatePenalty(burnt) != -4.0)               return false;
        if (!sc.HasStatePenalty(burnt))                      return false;

        // Klammern dreht Gewichte nicht um, laesst Strafen aber negativ.
        sc.freshnessWeight         = -2.0;
        sc.ingredientQualityWeight = -1.0;
        sc.defaultTierSet          = "";
        sc.ClampInPlace();
        if (sc.freshnessWeight != 0.0)                       return false;
        if (sc.ingredientQualityWeight != 0.0)               return false;
        if (sc.defaultTierSet != DEFAULT_TIER_SET)           return false;
        if (sc.GetStatePenalty(burnt) != -4.0)               return false;

        sc.ClearStatePenalties();
        if (sc.StatePenaltyCount() != 0)                     return false;

        return true;
    }
}
