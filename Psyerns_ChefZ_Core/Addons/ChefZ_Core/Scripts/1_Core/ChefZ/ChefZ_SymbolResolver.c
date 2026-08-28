//==============================================================================
// ChefZ_SymbolResolver - die Nachschlagegrenze zwischen 1_Core und 3_Game
//
// Entwurf: 07 §5 (der Compiler braucht beim BOOT den Bitindex einer Kategorie,
// den Rang einer Qualitaetsstufe und den selectivityHint aus den
// Rueckwaertsindizes des Ingredient Managers), 00 §4 (1_Core kennt keinen
// Manager), 19 S5 ("Der Matcher ist an dieser Stelle ohne laufendes Spiel
// pruefbar - das ist der Zweck des Layer-Schnitts").
//
// ---------------------------------------------------------------------------
// Das Problem, das diese Klasse loest
// ---------------------------------------------------------------------------
// ChefZ_SelectorCompiler liegt in 1_Core. Er muss beim Kompilieren Fragen
// beantworten, deren Antworten in 3_Game liegen:
//
//     "MEAT" -> welcher Bitindex?          ChefZ_CategoryManager
//     "MEAT" -> welche Tiefe?              ChefZ_CategoryManager  (Spezifitaet)
//     "MEAT" -> wie viele Klassen?         ChefZ_IngredientManager (Hint)
//     "PREPARED" -> welcher Rang?          ChefZ_QualityManager    (ab S10)
//
// Wuerde 1_Core die Manager direkt rufen, waere der Layer-Schnitt hin und der
// Matcher nur noch mit laufendem Spiel pruefbar. Stattdessen fragt er DIESES
// Objekt, und wer es fuellt, entscheidet der Aufrufer:
//
//   Selbsttest (1_Core)  -> Tabellen von Hand fuellen, drei Zeilen
//   Betrieb    (3_Game)  -> ChefZ_ManagerSymbolResolver leitet an die Manager
//
// Die Basisklasse ist deshalb NICHT abstrakt, sondern tabellengetrieben und
// vollstaendig benutzbar. Das ist der Unterschied zwischen "man koennte den
// Matcher testen" und "der Test steht im Auslieferungsstand und laeuft bei
// jedem Serverstart".
//
// Alle Methoden sind ueberschreibbar. Keine kennt Content: die Tabellen sind
// leer, bis jemand sie fuellt.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_SymbolResolver : Managed
{
    //! Antwort auf "unbekannt" - fuer Bitindex, Tiefe und Rang gleichermassen.
    static const int UNKNOWN = -1;

    private ref map<int, int> m_CategoryBit;      // Sym -> Bitindex
    private ref map<int, int> m_CategoryDepth;    // Sym -> Tiefe (Wurzel = 0)
    private ref map<int, int> m_CategorySize;     // Sym -> geschaetzte Klassenzahl
    private ref map<int, int> m_TagSize;          // Sym -> geschaetzte Klassenzahl
    private ref map<int, int> m_QualityRank;      // Sym -> Rang
    private ref map<int, int> m_QualityTierSet;   // Sym -> tierSet als Symbol
    private ref array<ChefZ_Sym> m_Qualities;     // Einfuegereihenfolge, stabil
    private ref array<ChefZ_Sym> m_States;
    private ref array<ChefZ_Sym> m_Units;
    private ref array<ChefZ_Sym> m_Classes;
    private int m_UniverseSize;

    void ChefZ_SymbolResolver()
    {
        m_CategoryBit    = new map<int, int>();
        m_CategoryDepth  = new map<int, int>();
        m_CategorySize   = new map<int, int>();
        m_TagSize        = new map<int, int>();
        m_QualityRank    = new map<int, int>();
        m_QualityTierSet = new map<int, int>();
        m_Qualities      = new array<ChefZ_Sym>();
        m_States         = new array<ChefZ_Sym>();
        m_Units          = new array<ChefZ_Sym>();
        m_Classes        = new array<ChefZ_Sym>();
        m_UniverseSize   = 0;
    }

    //==========================================================================
    // Fuellen - nur Bootzeit oder Selbsttest
    //==========================================================================

    void DefineCategory(string name, int bitIndex, int depth, int classCount)
    {
        ChefZ_Sym sym = ChefZ_SymbolTable.Intern(name);
        if (!ChefZ_SymbolTable.IsValid(sym))
            return;
        m_CategoryBit.Set(sym, bitIndex);
        m_CategoryDepth.Set(sym, depth);
        m_CategorySize.Set(sym, classCount);
    }

    void DefineTag(string name, int classCount)
    {
        ChefZ_Sym sym = ChefZ_SymbolTable.Intern(name);
        if (!ChefZ_SymbolTable.IsValid(sym))
            return;
        m_TagSize.Set(sym, classCount);
    }

    void DefineState(string name)
    {
        ChefZ_Sym sym = ChefZ_SymbolTable.Intern(name);
        if (!ChefZ_SymbolTable.IsValid(sym))
            return;
        if (m_States.Find(sym) < 0)
            m_States.Insert(sym);
    }

    /**
     * Qualitaetsstufe mit Rang und Stufensatz (12 §3: Rang aufsteigend,
     * Vergleiche gelten INNERHALB eines tierSet).
     *
     * tierSet darf leer bleiben; dann liegt die Stufe im namenlosen Satz.
     */
    void DefineQuality(string name, int rank, string tierSet)
    {
        ChefZ_Sym sym = ChefZ_SymbolTable.Intern(name);
        if (!ChefZ_SymbolTable.IsValid(sym))
            return;
        m_QualityRank.Set(sym, rank);
        m_QualityTierSet.Set(sym, ChefZ_SymbolTable.Intern(tierSet));
        if (m_Qualities.Find(sym) < 0)
            m_Qualities.Insert(sym);
    }

    void DefineUnit(string name)
    {
        ChefZ_Sym sym = ChefZ_SymbolTable.Intern(name);
        if (!ChefZ_SymbolTable.IsValid(sym))
            return;
        if (m_Units.Find(sym) < 0)
            m_Units.Insert(sym);
    }

    void DefineClass(string name)
    {
        ChefZ_Sym sym = ChefZ_SymbolTable.Intern(name);
        if (!ChefZ_SymbolTable.IsValid(sym))
            return;
        if (m_Classes.Find(sym) < 0)
            m_Classes.Insert(sym);
        m_UniverseSize = m_Classes.Count();
    }

    void SetUniverseSize(int n)
    {
        m_UniverseSize = n;
    }

    //==========================================================================
    // Fragen - der Compiler stellt sie, nie der Matcher
    //==========================================================================

    //! Bitindex fuer den Kategorietest im heissen Pfad. UNKNOWN = Kategorie
    //! existiert nicht -> Kompilierfehler (07 §7).
    int CategoryBit(ChefZ_Sym category)
    {
        int bit;
        if (m_CategoryBit.Find(category, bit))
            return bit;
        return UNKNOWN;
    }

    //! Tiefe fuer die Spezifitaetsrechnung (09 §4.1). Wurzel = 0.
    int CategoryDepth(ChefZ_Sym category)
    {
        int depth;
        if (m_CategoryDepth.Find(category, depth))
            return depth;
        return UNKNOWN;
    }

    bool TagExists(ChefZ_Sym tag)
    {
        int n;
        return m_TagSize.Find(tag, n);
    }

    bool StateExists(ChefZ_Sym state)
    {
        return m_States.Find(state) >= 0;
    }

    bool QualityExists(ChefZ_Sym quality)
    {
        int rank;
        return m_QualityRank.Find(quality, rank);
    }

    int QualityRank(ChefZ_Sym quality)
    {
        int rank;
        if (m_QualityRank.Find(quality, rank))
            return rank;
        return UNKNOWN;
    }

    ChefZ_Sym QualityTierSet(ChefZ_Sym quality)
    {
        int setValue;
        if (m_QualityTierSet.Find(quality, setValue))
            return setValue;
        return ChefZ_SymbolTable.INVALID;
    }

    /**
     * Alle Stufen DESSELBEN Stufensatzes mit Rang >= dem der genannten Stufe,
     * die genannte eingeschlossen.
     *
     * Warum der Compiler das braucht und nicht der Matcher (siehe
     * ChefZ_CompiledSelector): zur Laufzeit haelt ChefZ_ItemFacts nur das
     * Qualitaetssymbol, keinen Rang - und 1_Core darf keinen Manager fragen.
     * Die Schwelle wird deshalb beim Kompilieren in eine Aufzaehlung der
     * zulaessigen Stufen aufgeloest. Aus einem Vergleich wird ein Find() auf
     * einer Liste mit typischerweise ein bis fuenf Eintraegen.
     *
     * Die Ausgabe ist nach Rang aufsteigend sortiert, bei Gleichstand nach
     * Definitionsreihenfolge - der Matcher haengt nicht davon ab, der Trace
     * liest sich damit aber wie die Stufenleiter aus 12.
     */
    void QualitiesAtOrAbove(ChefZ_Sym minQuality, out array<ChefZ_Sym> outTiers)
    {
        if (!outTiers)
            outTiers = new array<ChefZ_Sym>();
        outTiers.Clear();

        int minRank = QualityRank(minQuality);
        if (minRank == UNKNOWN)
            return;

        ChefZ_Sym setSym = QualityTierSet(minQuality);

        for (int i = 0; i < m_Qualities.Count(); i++)
        {
            ChefZ_Sym q = m_Qualities.Get(i);
            if (QualityTierSet(q) != setSym)
                continue;
            if (QualityRank(q) < minRank)
                continue;
            outTiers.Insert(q);
        }

        SortByRank(outTiers);
    }

    protected void SortByRank(notnull array<ChefZ_Sym> tiers)
    {
        // Einfuegesortierung: stabil, ohne Allokation, und die Listen sind
        // einstellig lang.
        for (int i = 1; i < tiers.Count(); i++)
        {
            ChefZ_Sym key = tiers.Get(i);
            int keyRank = QualityRank(key);
            int j = i - 1;
            while (j >= 0 && QualityRank(tiers.Get(j)) > keyRank)
            {
                tiers.Set(j + 1, tiers.Get(j));
                j--;
            }
            tiers.Set(j + 1, key);
        }
    }

    bool UnitExists(ChefZ_Sym unit)
    {
        return m_Units.Find(unit) >= 0;
    }

    //! Ist diese Klasse als Zutat deklariert? Eine unbekannte Klasse ist KEIN
    //! Fehler (07 §7 nennt sie nicht): ein class-Selektor auf eine Klasse aus
    //! einem noch nicht geladenen Content-Modul matcht schlicht nie. Der
    //! Compiler macht daraus einen Hinweis, keine Abweisung.
    bool ClassDeclared(ChefZ_Sym cls)
    {
        return m_Classes.Find(cls) >= 0;
    }

    /**
     * Geschaetzte Trefferzahl fuer den selectivityHint (07 E4).
     *
     * 0 ist eine gueltige Antwort und heisst "kein deklariertes
     * Kandidatenfeld" - so ein Slot gehoert an den ANFANG der Slotreihenfolge,
     * weil er den Suchbaum sofort abschneidet.
     */
    int EstimateCandidates(ChefZ_Sym categoryOrTag)
    {
        int n;
        if (m_CategorySize.Find(categoryOrTag, n))
            return n;
        if (m_TagSize.Find(categoryOrTag, n))
            return n;
        return 0;
    }

    //! Obergrenze fuer Schaetzungen, die nichts einschraenken (STATE, NOT,
    //! TRUE_OP). Mindestens 1, damit ein Hint nie faelschlich "trifft nie"
    //! behauptet.
    int UniverseSize()
    {
        if (m_UniverseSize < 1)
            return 1;
        return m_UniverseSize;
    }

    //! Nur fuer den Selbsttest (S5).
    static bool SelfCheck()
    {
        ChefZ_SymbolResolver r = new ChefZ_SymbolResolver();
        ChefZ_Sym unknown = ChefZ_SymbolTable.Intern("CHEFZ_SR_UNBEKANNT");

        if (r.CategoryBit(unknown) != UNKNOWN)      return false;
        if (r.CategoryDepth(unknown) != UNKNOWN)    return false;
        if (r.TagExists(unknown))                   return false;
        if (r.StateExists(unknown))                 return false;
        if (r.QualityExists(unknown))               return false;
        if (r.EstimateCandidates(unknown) != 0)     return false;
        if (r.UniverseSize() != 1)                  return false;

        r.DefineCategory("CHEFZ_SR_KAT", 4, 2, 7);
        ChefZ_Sym kat = ChefZ_SymbolTable.Lookup("CHEFZ_SR_KAT");
        if (r.CategoryBit(kat) != 4)                return false;
        if (r.CategoryDepth(kat) != 2)              return false;
        if (r.EstimateCandidates(kat) != 7)         return false;

        r.DefineQuality("CHEFZ_SR_Q0", 0, "CHEFZ_SR_SET");
        r.DefineQuality("CHEFZ_SR_Q2", 2, "CHEFZ_SR_SET");
        r.DefineQuality("CHEFZ_SR_Q1", 1, "CHEFZ_SR_SET");
        r.DefineQuality("CHEFZ_SR_X9", 9, "CHEFZ_SR_ANDERER_SET");

        ChefZ_Sym q1 = ChefZ_SymbolTable.Lookup("CHEFZ_SR_Q1");
        array<ChefZ_Sym> tiers = new array<ChefZ_Sym>();
        r.QualitiesAtOrAbove(q1, tiers);
        if (tiers.Count() != 2)                     return false;   // Q1, Q2 - nicht X9
        if (r.QualityRank(tiers.Get(0)) != 1)       return false;
        if (r.QualityRank(tiers.Get(1)) != 2)       return false;

        r.QualitiesAtOrAbove(unknown, tiers);
        if (tiers.Count() != 0)                     return false;

        return true;
    }
}
