//==============================================================================
// ChefZ_CategoryClosure - Vorfahrenbitset einer Kategoriemenge
//
// Entwurf: 04 §2 (Schnittstelle woertlich), 04 E1 (warum Bitset), 05 §3.3
// (die Closure liegt im ChefZ_ItemFacts, nicht im Manager), 07 §5
// ("CATEGORY: closure.HasBit(categoryBitIndex) - 1 Bit-Test").
//
// Bit k gesetzt <=> Kategorie mit Bitindex k ist self-or-ancestor.
//
// Warum ein Bitset und nicht der Baumlauf oder eine Vorfahrenmenge (04 E1):
// die Frage "gehoert X zu Y" sitzt im innersten Loop - pro Kandidatenrezept,
// pro Slot, pro Item. Ein Baumlauf ist O(Tiefe) mit einem Map-Zugriff je
// Schritt, eine set<Sym>-Vorfahrenmenge kostet je Kategorie eine Allokation
// und viel Speicherindirektion. Das Bitset ist ein AND auf einem int: bei 256
// Kategorien 32 Byte je Kategorie, 8 KB insgesamt.
//
// Diese Klasse haelt AUSSCHLIESSLICH Bits. Sie kennt keine Kategorie, keinen
// Namen und keinen Baum - die Zuordnung Bitindex <-> ChefZ_Sym gehoert dem
// ChefZ_CategoryManager (3_Game). Genau deshalb kann sie in 1_Core liegen und
// vom Matcher ohne jeden Managerzugriff benutzt werden.
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

class ChefZ_CategoryClosure : Managed
{
    static const int BITS_PER_WORD = 32;

    /**
     * Notbremse gegen einen Index aus kaputten Daten.
     *
     * Ohne sie wuerde ein einziger unsinniger Bitindex (z.B. aus einem
     * Rechenfehler) hier eine Wortliste in Gigabytegroesse anlegen und den
     * Server im Boot toeten. Der echte Deckel ist maxCategories aus Core.json
     * (Default 256) und wird im ChefZ_CategoryManager durchgesetzt - dieser
     * hier ist nur das Netz darunter, deshalb liegt er bewusst weit oberhalb
     * jedes sinnvollen Wertes.
     */
    static const int MAX_BITS = 8192;

    private ref array<int> m_Words;

    void ChefZ_CategoryClosure()
    {
        m_Words = new array<int>();
    }

    //--------------------------------------------------------------------------
    // Schreiben - ausschliesslich zur Bootzeit
    //--------------------------------------------------------------------------

    /**
     * Setzt das Bit einer Kategorie. Negative Indizes und Indizes oberhalb
     * von MAX_BITS werden ignoriert - ein unbekanntes Symbol darf hier nichts
     * anrichten, es wurde beim Laden gemeldet (04 §6).
     */
    void SetBit(int categoryIndex)
    {
        if (categoryIndex < 0)
            return;

        if (categoryIndex >= MAX_BITS)
        {
            ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.CONFIG, "closure.bit.outofrange", "ChefZ_CategoryClosure.SetBit(" + categoryIndex.ToString() + ") liegt oberhalb " + "der Obergrenze " + MAX_BITS.ToString() + " und wurde ignoriert. Das ist ein " + "Programmierfehler im Kategorieaufbau, kein Datenfehler - die betroffene " + "Kategorie matcht ab jetzt nie.");
            return;
        }

        int word = categoryIndex / BITS_PER_WORD;
        EnsureWords(word + 1);
        m_Words.Set(word, m_Words.Get(word) | MaskOf(categoryIndex));
    }

    //! Vereinigung. Der Baumaufbau schiebt damit das Bitset des Elternteils in
    //! das des Kindes (04 §4, Schritt 4).
    void OrWith(notnull ChefZ_CategoryClosure other)
    {
        int count = other.WordCount();
        if (count <= 0)
            return;

        EnsureWords(count);
        for (int i = 0; i < count; i++)
            m_Words.Set(i, m_Words.Get(i) | other.WordAt(i));
    }

    void CopyFrom(notnull ChefZ_CategoryClosure other)
    {
        Clear();
        OrWith(other);
    }

    /**
     * Loescht alle Bits, behaelt aber die Woerter.
     *
     * Absicht: eine Closure, die wiederverwendet wird (ChefZ_ItemFacts je
     * Sammellauf), soll nicht bei jedem Zuruecksetzen neu allokieren.
     */
    void Clear()
    {
        for (int i = 0; i < m_Words.Count(); i++)
            m_Words.Set(i, 0);
    }

    //--------------------------------------------------------------------------
    // Lesen - der heisse Pfad
    //--------------------------------------------------------------------------

    /**
     * Der eine Test, auf den es ankommt: ein Array-Get und ein AND.
     *
     * Bewusst OHNE Log und ohne Bereichsmeldung - ein Bit ausserhalb der
     * bisher belegten Woerter ist schlicht nicht gesetzt.
     */
    bool HasBit(int categoryIndex)
    {
        if (categoryIndex < 0)
            return false;

        int word = categoryIndex / BITS_PER_WORD;
        if (word >= m_Words.Count())
            return false;

        return (m_Words.Get(word) & MaskOf(categoryIndex)) != 0;
    }

    bool IsEmpty()
    {
        for (int i = 0; i < m_Words.Count(); i++)
        {
            if (m_Words.Get(i) != 0)
                return false;
        }
        return true;
    }

    //! Anzahl gesetzter Bits. Nur fuer Diagnose und Selbsttest gedacht -
    //! die Schleife laeuft ueber alle Bits, nicht ueber die gesetzten.
    int CountBits()
    {
        int n = 0;
        for (int w = 0; w < m_Words.Count(); w++)
        {
            int value = m_Words.Get(w);
            if (value == 0)
                continue;
            for (int b = 0; b < BITS_PER_WORD; b++)
            {
                if ((value & (1 << b)) != 0)
                    n++;
            }
        }
        return n;
    }

    //--------------------------------------------------------------------------
    // Wortzugriff
    //
    // Oeffentlich, weil OrWith() auf die Woerter einer ZWEITEN Instanz
    // zugreifen muss und ein Sonderrecht dafuer in Enforce nicht zuverlaessig
    // ausgedrueckt werden kann. Lesend und damit ungefaehrlich; ausserhalb
    // dieser Klasse braucht es niemand.
    //--------------------------------------------------------------------------

    int WordCount()
    {
        return m_Words.Count();
    }

    int WordAt(int index)
    {
        if (index < 0 || index >= m_Words.Count())
            return 0;
        return m_Words.Get(index);
    }

    //--------------------------------------------------------------------------

    private static int MaskOf(int categoryIndex)
    {
        // Fuer Bit 31 ist die Maske negativ (Vorzeichenbit). Das ist
        // unerheblich: es wird ausschliesslich mit AND und OR gearbeitet, nie
        // verglichen oder gerechnet.
        return 1 << (categoryIndex % BITS_PER_WORD);
    }

    private void EnsureWords(int count)
    {
        while (m_Words.Count() < count)
            m_Words.Insert(0);
    }

    //! Gesetzte Bitindizes als Text, z.B. "{0,3,7}". Nur fuer DumpTree und
    //! Trace - im Betrieb wird diese Zeichenkette nie gebaut.
    string ToDebugString()
    {
        string s = "{";
        bool first = true;
        int bits = m_Words.Count() * BITS_PER_WORD;
        for (int i = 0; i < bits; i++)
        {
            if (!HasBit(i))
                continue;
            if (!first)
                s = s + ",";
            s = s + i.ToString();
            first = false;
        }
        return s + "}";
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest (S3).
    static bool SelfCheck()
    {
        ChefZ_CategoryClosure a = new ChefZ_CategoryClosure();

        if (!a.IsEmpty())                       return false;
        if (a.HasBit(0))                        return false;
        if (a.HasBit(-1))                       return false;
        if (a.CountBits() != 0)                 return false;

        a.SetBit(0);
        a.SetBit(31);       // Wortgrenze, Vorzeichenbit
        a.SetBit(32);       // erstes Bit des zweiten Wortes
        a.SetBit(200);

        if (!a.HasBit(0))                       return false;
        if (!a.HasBit(31))                      return false;
        if (!a.HasBit(32))                      return false;
        if (!a.HasBit(200))                     return false;
        if (a.HasBit(1))                        return false;
        if (a.HasBit(33))                       return false;
        if (a.HasBit(9999))                     return false;
        if (a.IsEmpty())                        return false;
        if (a.CountBits() != 4)                 return false;

        // Idempotenz: dasselbe Bit zweimal setzen aendert nichts.
        a.SetBit(31);
        if (a.CountBits() != 4)                 return false;

        // Vereinigung
        ChefZ_CategoryClosure b = new ChefZ_CategoryClosure();
        b.SetBit(1);
        b.OrWith(a);
        if (!b.HasBit(1))                       return false;
        if (!b.HasBit(200))                     return false;
        if (b.CountBits() != 5)                 return false;

        // a darf sich durch das Vereinigen in b NICHT veraendert haben.
        if (a.HasBit(1))                        return false;
        if (a.CountBits() != 4)                 return false;

        // Kopie ist unabhaengig
        ChefZ_CategoryClosure c = new ChefZ_CategoryClosure();
        c.CopyFrom(b);
        if (c.CountBits() != 5)                 return false;
        c.SetBit(300);
        if (b.HasBit(300))                      return false;

        // Leeren
        c.Clear();
        if (!c.IsEmpty())                       return false;
        if (c.CountBits() != 0)                 return false;
        if (c.HasBit(200))                      return false;

        // Ungueltige Indizes bleiben folgenlos.
        ChefZ_CategoryClosure d = new ChefZ_CategoryClosure();
        d.SetBit(-5);
        d.SetBit(MAX_BITS);
        d.SetBit(MAX_BITS + 1000);
        if (!d.IsEmpty())                       return false;

        // OrWith mit einer leeren Closure aendert nichts.
        ChefZ_CategoryClosure e = new ChefZ_CategoryClosure();
        e.SetBit(5);
        e.OrWith(d);
        if (e.CountBits() != 1)                 return false;
        if (!e.HasBit(5))                       return false;

        return true;
    }
}
