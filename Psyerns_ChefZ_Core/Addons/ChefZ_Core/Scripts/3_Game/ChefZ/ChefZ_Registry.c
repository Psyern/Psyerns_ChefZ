//==============================================================================
// ChefZ_Registry<T> - typisierter, einfrierbarer Registryzugriff
//
// Entwurf: 02 §5.3 (Signatur woertlich), 02 §6 (FREEZE), 03 §4 (stabile
// Schluesselreihenfolge).
//
// Aufteilung in Basis + Generikum, und zwar aus einem konkreten Grund:
// Enforce-Generika sind Vorlagen. Jede Instanziierung erzeugt eigenen Code -
// die Speicher- und Indexlogik gehoert deshalb genau einmal in die
// nicht-generische Basis, und das Generikum liefert nur die typisierte
// Sicht. Vorbild aus Vanilla: PlayerStat<Class T> extends PlayerStatBase
// (4_World/DayZ/Classes/PlayerStats/PlayerStatBase.c:32).
//
// FREEZE ist kein Schmuck: nach dem Boot darf keine Registry mehr wachsen.
// Ein spaeter eingefuegter Record haette kein Symbol aus der COMPILE-Stufe,
// keinen Sync-Ordinal und keine Position in Keys() - er waere auf Client und
// Server verschieden. Deshalb meldet Add() nach dem Einfrieren einen Fehler,
// statt still zu wirken.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_RegistryBase
{
    private string m_Kind;
    private bool   m_Frozen;

    private ref array<ref ChefZ_Record> m_Records;   // Eigentuemer der Records
    private ref map<int, int>           m_BySym;     // ChefZ_Sym -> Index
    private ref map<string, int>        m_ByName;    // id        -> Index
    private ref array<int>              m_Keys;      // Symbole, stabil sortiert

    void ChefZ_RegistryBase()
    {
        m_Kind    = "";
        m_Frozen  = false;
        m_Records = new array<ref ChefZ_Record>();
        m_BySym   = new map<int, int>();
        m_ByName  = new map<string, int>();
        m_Keys    = new array<int>();
    }

    void Init(string kind)
    {
        m_Kind = kind;
    }

    string GetKind()
    {
        return m_Kind;
    }

    //--------------------------------------------------------------------------

    /**
     * Nimmt einen fertig validierten und kompilierten Record auf.
     *
     * false bedeutet: nicht aufgenommen. Der Aufrufer (Config Manager) meldet
     * den Grund - hier wird nicht geloggt, damit die Registry ohne Log
     * benutzbar bleibt.
     */
    bool Add(ChefZ_Record rec)
    {
        if (!rec)
            return false;
        if (m_Frozen)
            return false;
        if (rec.id == "")
            return false;
        if (m_ByName.Contains(rec.id))
            return false;

        int index = m_Records.Count();
        m_Records.Insert(rec);
        m_ByName.Set(rec.id, index);

        if (ChefZ_SymbolTable.IsValid(rec.sym))
            m_BySym.Set(rec.sym, index);

        return true;
    }

    //--------------------------------------------------------------------------

    ChefZ_Record FindRecord(ChefZ_Sym symbol)
    {
        int index;
        if (!m_BySym.Find(symbol, index))
            return null;
        return m_Records.Get(index);
    }

    ChefZ_Record FindRecordByName(string id)
    {
        int index;
        if (!m_ByName.Find(id, index))
            return null;
        return m_Records.Get(index);
    }

    ChefZ_Record GetAt(int index)
    {
        if (index < 0 || index >= m_Records.Count())
            return null;
        return m_Records.Get(index);
    }

    bool Contains(ChefZ_Sym symbol)
    {
        return m_BySym.Contains(symbol);
    }

    bool ContainsName(string id)
    {
        return m_ByName.Contains(id);
    }

    int Count()
    {
        return m_Records.Count();
    }

    //--------------------------------------------------------------------------

    /**
     * Symbole in stabiler Reihenfolge: sortiert nach ID, ordinal (03 §4).
     *
     * Nicht nach Symbolwert - der haengt an der Internierungsreihenfolge und
     * damit an der Ladereihenfolge der Addons. Wer ueber Keys() iteriert,
     * bekommt auf Client und Server dieselbe Folge.
     *
     * Die Liste wird beim Freeze einmal gebaut; der Aufrufer bekommt sie
     * gelesen, nicht zum Sortieren.
     */
    array<ChefZ_Sym> Keys()
    {
        if (m_Keys.Count() != m_Records.Count())
            RebuildKeys();
        return m_Keys;
    }

    //! IDs in derselben stabilen Reihenfolge wie Keys().
    array<string> SortedIds()
    {
        array<string> ids = new array<string>();
        for (int i = 0; i < m_Records.Count(); i++)
            ids.Insert(m_Records.Get(i).id);
        ChefZ_StringOrder.SortAscending(ids);
        return ids;
    }

    private void RebuildKeys()
    {
        array<string> ids = SortedIds();
        m_Keys.Clear();
        for (int i = 0; i < ids.Count(); i++)
        {
            ChefZ_Record rec = FindRecordByName(ids.Get(i));
            if (rec)
                m_Keys.Insert(rec.sym);
        }
    }

    //--------------------------------------------------------------------------

    void Freeze()
    {
        RebuildKeys();
        m_Frozen = true;
    }

    bool IsFrozen()
    {
        return m_Frozen;
    }

    /**
     * Leert die Registry, auch eingefroren.
     *
     * Der einzige vorgesehene Aufrufer ist der SAFE_MODE (02 §8): "alle
     * Registries geleert, Core inert. Lieber ganz Vanilla als halb ChefZ."
     * Ein leerer Bestand ist kein Sonderfall - jeder Hook prueft ohnehin
     * zuerst, ob es ueberhaupt etwas zu tun gibt.
     */
    void ClearAll()
    {
        m_Records.Clear();
        m_BySym.Clear();
        m_ByName.Clear();
        m_Keys.Clear();
    }

    void DebugDump(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert(m_Kind + ": " + Count().ToString() + " Records"
            + ", frozen=" + m_Frozen.ToString());

        array<ChefZ_Sym> keys = Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_Record rec = FindRecord(keys.Get(i));
            if (rec)
                outLines.Insert("    " + rec.Describe());
        }
    }
}

//------------------------------------------------------------------------------

/**
 * Typisierte Sicht. Nach Freeze() ausschliesslich lesend (02 §5.3).
 *
 * Find() liefert null statt zu werfen: eine unbekannte ID ist im
 * datengetriebenen Betrieb ein Datenfehler, der beim Laden gemeldet wurde -
 * und ein Nullwert im heissen Pfad ist billiger und harmloser als eine
 * Ausnahme (02 §8, Invariante I2).
 */
class ChefZ_Registry<Class T> extends ChefZ_RegistryBase
{
    T Find(ChefZ_Sym symbol)
    {
        T typed;
        if (Class.CastTo(typed, FindRecord(symbol)))
            return typed;
        return null;
    }

    T FindByName(string id)
    {
        T typed;
        if (Class.CastTo(typed, FindRecordByName(id)))
            return typed;
        return null;
    }

    T At(int index)
    {
        T typed;
        if (Class.CastTo(typed, GetAt(index)))
            return typed;
        return null;
    }
}
