//==============================================================================
// ChefZ_MatchTrace - der Match-Trace als Null-Objekt
//
// Entwurf: 18 §2.2 (Schnittstelle woertlich, "Null-Objekt-Muster statt
// globalem Schalter im Matcher"), 18 §3 (ein Trace ist ein zusammenhaengender
// BLOCK, keine verstreuten Zeilen), 07 E6 (die Begruendungen, die hier landen).
//
// ---------------------------------------------------------------------------
// Warum CreateIfEnabled() null liefern DARF und soll
// ---------------------------------------------------------------------------
// Ist Kanal MATCH aus, gibt es kein Objekt - und damit entsteht weder eine
// Allokation noch eine einzige formatierte Zeichenkette. Der Matcher schreibt
//
//     if (trace)
//         trace.SlotResult(...);
//
// und nicht "if (ChefZ_Log.Enabled(...))" in jeder Zeile. Derselbe Matcher
// laeuft damit im Produktivpfad ohne jeden Aufwand und unter "chefz match" mit
// vollem Protokoll - ohne zwei Codepfade.
//
// Der Trace SAMMELT und gibt am Ende alles auf einmal aus (Emit). Bei
// dutzenden gleichzeitig kochenden Feuerstellen waeren zeilenweise Ausgaben
// ineinander verschraenkt und unlesbar.
//
// Layer: 1_Core. Keine Spielabhaengigkeit; die Zeilen gehen an ChefZ_Log.
//==============================================================================

class ChefZ_MatchTrace
{
    /**
     * Obergrenze der gesammelten Zeilen.
     *
     * Ein Trace ueber 30 Kandidatenrezepte x 8 Slots x 12 Items kann sonst
     * vierstellig werden. Die Grenze schneidet ab und sagt, dass sie
     * abgeschnitten hat - stilles Kuerzen waere im Fehlerfall die
     * schlechteste Eigenschaft eines Diagnosewerkzeugs.
     */
    static const int MAX_LINES = 400;

    private ref array<string> m_Lines;
    private bool m_Truncated;
    private int  m_Level;
    private int  m_Channel;

    void ChefZ_MatchTrace()
    {
        m_Lines     = new array<string>();
        m_Truncated = false;
        m_Level     = ChefZ_LogLevel.DEBUG;
        m_Channel   = ChefZ_LogChannel.MATCH;
    }

    /**
     * null, wenn Kanal MATCH auf DEBUG nicht aktiv ist.
     *
     * Der Aufrufer prueft NUR auf null. Wer den Trace erzwingen will
     * (Adminkommando "chefz match"), baut das Objekt mit new und uebergibt es -
     * dann laeuft der Trace unabhaengig von der Logstufe und wird ueber
     * ToLines() abgeholt statt ins RPT geschrieben.
     */
    static ChefZ_MatchTrace CreateIfEnabled()
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.DEBUG))
            return null;
        return new ChefZ_MatchTrace();
    }

    //==========================================================================
    // Aufzeichnen
    //==========================================================================

    void Begin(ChefZ_Sym device, int method, int itemCount)
    {
        Add("=== Match " + ChefZ_SymbolTable.NameOrMark(device)
            + "  Methode=" + method.ToString()
            + "  Items=" + itemCount.ToString());
    }

    void Contents(notnull ChefZ_FactSnapshot snapshot)
    {
        for (int i = 0; i < snapshot.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.Get(i);
            if (facts)
                Add("  Inhalt " + facts.ToLine());
        }
    }

    void CandidateCount(int n)
    {
        Add("  Kandidaten: " + n.ToString());
    }

    void RecipeConsidered(ChefZ_Sym recipe, float specificity)
    {
        Add("  Rezept " + ChefZ_SymbolTable.NameOrMark(recipe)
            + "  spez=" + specificity.ToString());
    }

    void SlotResult(ChefZ_Sym recipe, string slotId, bool ok, string reason)
    {
        string verdict = "ok";
        if (!ok)
            verdict = "nein";

        string line = "    Slot " + slotId + ": " + verdict;
        if (reason != "")
            line = line + "  (" + reason + ")";
        Add(line);
    }

    void SlotAssigned(string slotId, ChefZ_Sym itemClass, int count, float units)
    {
        Add("    Slot " + slotId + " <- " + ChefZ_SymbolTable.NameOrMark(itemClass)
            + " x" + count.ToString() + "  Einheiten=" + units.ToString());
    }

    void RecipeRejected(ChefZ_Sym recipe, string reason, string slotId)
    {
        string line = "  Rezept " + ChefZ_SymbolTable.NameOrMark(recipe) + " abgelehnt: " + reason;
        if (slotId != "")
            line = line + "  (Slot " + slotId + ")";
        Add(line);
    }

    void GradeRuleFired(string ruleId, float points)
    {
        Add("    Regel " + ruleId + " -> " + points.ToString() + " Punkte");
    }

    void Winner(ChefZ_Sym recipe, float score, ChefZ_Sym tier, int nodes)
    {
        Add("  GEWINNER " + ChefZ_SymbolTable.NameOrMark(recipe)
            + "  score=" + score.ToString()
            + "  stufe=" + ChefZ_SymbolTable.NameOrMark(tier)
            + "  knoten=" + nodes.ToString());
    }

    void Readiness(bool ready, string reason)
    {
        string line = "  Abschluss: ";
        if (ready)
            line = line + "erfuellt";
        else
            line = line + "offen";
        if (reason != "")
            line = line + "  (" + reason + ")";
        Add(line);
    }

    //! Freie Zeile - fuer alles, wofuer 18 §2.2 keine eigene Methode nennt
    //! (Knotenbudget, Suchreihenfolge, Zwischenstaende des Backtrackings).
    void Note(string text)
    {
        Add("  " + text);
    }

    //==========================================================================
    // Ausgeben
    //==========================================================================

    void Emit()
    {
        if (m_Lines.Count() == 0)
            return;
        ChefZ_Log.Block(m_Level, m_Channel, m_Lines);
        Reset();
    }

    void ToLines(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();
        for (int i = 0; i < m_Lines.Count(); i++)
            outLines.Insert(m_Lines.Get(i));
    }

    void Reset()
    {
        m_Lines.Clear();
        m_Truncated = false;
    }

    int LineCount()
    {
        return m_Lines.Count();
    }

    bool WasTruncated()
    {
        return m_Truncated;
    }

    //! Stufe und Kanal der Ausgabe umstellen - fuer "chefz match", das den
    //! Trace unabhaengig von der eingestellten Stufe sehen will.
    void SetOutput(int level, int channel)
    {
        m_Level   = level;
        m_Channel = channel;
    }

    private void Add(string line)
    {
        if (m_Lines.Count() >= MAX_LINES)
        {
            if (!m_Truncated)
            {
                m_Truncated = true;
                m_Lines.Insert("  ... Trace bei " + MAX_LINES.ToString()
                    + " Zeilen abgeschnitten.");
            }
            return;
        }
        m_Lines.Insert(line);
    }
}
