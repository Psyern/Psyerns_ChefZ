//==============================================================================
// ChefZ_StringOrder - ordinaler Stringvergleich und ordinale Sortierung
//
// Entwurf: 03 §4, Schritt 2: "Lexikografisch aufsteigend sortieren (ordinaler
// Vergleich, keine Locale)."
//
// Warum eigener Code statt array<string>.Sort():
//   Der Sync-Ordinal wird auf Client UND Server unabhaengig abgeleitet und
//   NICHT synchronisiert (03 E3). Beide Seiten muessen deshalb bitgenau
//   dieselbe Reihenfolge herstellen. array<string>.Sort() ist mit
//   "depends on underlaying type" dokumentiert - das ist keine Zusage ueber
//   die Vergleichsregel. Ein zeichenweiser Vergleich ueber ToAscii() ist eine.
//   Die Kosten fallen einmal beim Boot an, auf Listen mit dutzenden Eintraegen.
//
// Vergleichsregel: Zeichen fuer Zeichen ueber den Zeichencode; bei gleichem
// Praefix ist der kuerzere String kleiner. Gross- und Kleinschreibung werden
// NICHT eingeebnet - "MEAT" und "Meat" sind verschiedene IDs, und sie still
// zusammenzuziehen waere genau die Art von Mehrdeutigkeit, die 03 E5 verbietet.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_StringOrder
{
    //! -1 wenn a < b, 0 bei Gleichheit, +1 wenn a > b. Rein ordinal.
    static int Compare(string a, string b)
    {
        int la = a.Length();
        int lb = b.Length();
        int n  = la;
        if (lb < n)
            n = lb;

        for (int i = 0; i < n; i++)
        {
            int ca = a.Get(i).ToAscii();
            int cb = b.Get(i).ToAscii();
            if (ca < cb)
                return -1;
            if (ca > cb)
                return 1;
        }

        if (la < lb)
            return -1;
        if (la > lb)
            return 1;
        return 0;
    }

    static bool Less(string a, string b)
    {
        return Compare(a, b) < 0;
    }

    //! Aufsteigend und ohne absteigenden Schritt? Gleiche Nachbarn sind
    //! erlaubt - Duplikate meldet der Aufrufer, nicht die Sortierung.
    static bool IsAscending(notnull array<string> values)
    {
        for (int i = 1; i < values.Count(); i++)
        {
            if (Compare(values.Get(i - 1), values.Get(i)) > 0)
                return false;
        }
        return true;
    }

    /**
     * Stabile Einfuegesortierung, aufsteigend, an Ort und Stelle.
     *
     * O(n^2) und damit fuer die hier auftretenden Groessen (Zustaende <= 63,
     * Qualitaetsstufen <= 15, Registries im dreistelligen Bereich) voellig
     * ausreichend. Sie laeuft einmal beim Boot. Determinismus schlaegt hier
     * Laufzeit, weil das Ergebnis die Netzsynchronitaet traegt.
     */
    static void SortAscending(notnull array<string> values)
    {
        int n = values.Count();
        for (int i = 1; i < n; i++)
        {
            string key = values.Get(i);
            int j = i - 1;
            while (j >= 0 && Compare(values.Get(j), key) > 0)
            {
                values.Set(j + 1, values.Get(j));
                j--;
            }
            values.Set(j + 1, key);
        }
    }

    //! Kopie statt in-place, wenn der Aufrufer seine Eingabe behalten will.
    static array<string> SortedCopy(notnull array<string> values)
    {
        array<string> copy = new array<string>();
        for (int i = 0; i < values.Count(); i++)
            copy.Insert(values.Get(i));
        SortAscending(copy);
        return copy;
    }

    //! Nur fuer den Selbsttest (S1).
    static bool SelfCheck()
    {
        if (Compare("A", "B") != -1)        return false;
        if (Compare("B", "A") != 1)         return false;
        if (Compare("A", "A") != 0)         return false;
        if (Compare("AB", "ABC") != -1)     return false;
        if (Compare("ABC", "AB") != 1)      return false;
        if (Compare("", "A") != -1)         return false;
        // Ordinal heisst: Grossbuchstaben (65..90) vor Unterstrich (95) vor
        // Kleinbuchstaben (97..122).
        if (Compare("Z", "_") != -1)        return false;
        if (Compare("_", "a") != -1)        return false;

        array<string> v = new array<string>();
        v.Insert("SIGMA");
        v.Insert("NOVEMBER");
        v.Insert("PAPA");
        v.Insert("DELTA");
        if (IsAscending(v))                 return false;
        SortAscending(v);
        if (!IsAscending(v))                return false;
        if (v.Get(0) != "DELTA")            return false;
        if (v.Get(1) != "NOVEMBER")         return false;
        if (v.Get(2) != "PAPA")             return false;
        if (v.Get(3) != "SIGMA")            return false;

        array<string> empty = new array<string>();
        SortAscending(empty);
        if (!IsAscending(empty))            return false;

        return true;
    }
}
