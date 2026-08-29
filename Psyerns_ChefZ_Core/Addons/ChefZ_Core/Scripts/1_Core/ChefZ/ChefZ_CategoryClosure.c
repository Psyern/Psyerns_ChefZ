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
                if ((value & MaskOfBit(b)) != 0)
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
        return MaskOfBit(categoryIndex % BITS_PER_WORD);
    }

    /**
     * Die Maske eines Bits 0..31.
     *
     * Fuer Bit 31 ist die Maske negativ (Vorzeichenbit). Das ist fuer AND
     * und OR unerheblich - aber ob Enforce einen LAUFZEIT-Shift um 31
     * zusichert, steht nirgends: Vanilla schreibt 1 << 31 ausschliesslich
     * als Konstante hin (ActionCheckPulse.c:4), die der Compiler faltet.
     * int.MIN IST dasselbe Bitmuster (0x80000000) und braucht keinen Shift.
     *
     * Das ist Vorsicht, kein Befund: ob der Shift hier je falsch gerechnet
     * hat, ist ungeprueft. Sicher ist nur, dass diese Fassung nicht davon
     * abhaengt - und Bit 31 ist bei 41 geladenen Kategorien in Reichweite.
     */
    private static int MaskOfBit(int bit)
    {
        if (bit == 31)
            return int.MIN;
        return 1 << bit;
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

        if (!a.IsEmpty()) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 235, "!a.IsEmpty()");
        if (a.HasBit(0)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 236, "a.HasBit(0)");
        if (a.HasBit(-1)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 237, "a.HasBit(-1)");
        if (a.CountBits() != 0) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 238, "a.CountBits() != 0");

        a.SetBit(0);
        a.SetBit(31);       // Wortgrenze, Vorzeichenbit
        a.SetBit(32);       // erstes Bit des zweiten Wortes
        a.SetBit(200);

        if (!a.HasBit(0)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 245, "!a.HasBit(0)");
        if (!a.HasBit(31)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 246, "!a.HasBit(31)");
        if (!a.HasBit(32)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 247, "!a.HasBit(32)");
        if (!a.HasBit(200)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 248, "!a.HasBit(200)");
        if (a.HasBit(1)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 249, "a.HasBit(1)");
        if (a.HasBit(33)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 250, "a.HasBit(33)");
        if (a.HasBit(9999)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 251, "a.HasBit(9999)");
        if (a.IsEmpty()) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 252, "a.IsEmpty()");
        if (a.CountBits() != 4) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 253, "a.CountBits() != 4");

        // Idempotenz: dasselbe Bit zweimal setzen aendert nichts.
        a.SetBit(31);
        if (a.CountBits() != 4) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 257, "a.CountBits() != 4");

        // Vereinigung
        ChefZ_CategoryClosure b = new ChefZ_CategoryClosure();
        b.SetBit(1);
        b.OrWith(a);
        if (!b.HasBit(1)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 263, "!b.HasBit(1)");
        if (!b.HasBit(200)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 264, "!b.HasBit(200)");
        if (b.CountBits() != 5) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 265, "b.CountBits() != 5");

        // a darf sich durch das Vereinigen in b NICHT veraendert haben.
        if (a.HasBit(1)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 268, "a.HasBit(1)");
        if (a.CountBits() != 4) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 269, "a.CountBits() != 4");

        // Kopie ist unabhaengig
        ChefZ_CategoryClosure c = new ChefZ_CategoryClosure();
        c.CopyFrom(b);
        if (c.CountBits() != 5) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 274, "c.CountBits() != 5");
        c.SetBit(300);
        if (b.HasBit(300)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 276, "b.HasBit(300)");

        // Leeren
        c.Clear();
        if (!c.IsEmpty()) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 280, "!c.IsEmpty()");
        if (c.CountBits() != 0) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 281, "c.CountBits() != 0");
        if (c.HasBit(200)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 282, "c.HasBit(200)");

        // Ungueltige Indizes bleiben folgenlos.
        ChefZ_CategoryClosure d = new ChefZ_CategoryClosure();
        d.SetBit(-5);
        d.SetBit(MAX_BITS);
        d.SetBit(MAX_BITS + 1000);
        if (!d.IsEmpty()) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 289, "!d.IsEmpty()");

        // OrWith mit einer leeren Closure aendert nichts.
        ChefZ_CategoryClosure e = new ChefZ_CategoryClosure();
        e.SetBit(5);
        e.OrWith(d);
        if (e.CountBits() != 1) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 295, "e.CountBits() != 1");
        if (!e.HasBit(5)) return ChefZ_SelfTestTrace.Fail("CategoryClosure", 296, "!e.HasBit(5)");

        return true;
    }
}
