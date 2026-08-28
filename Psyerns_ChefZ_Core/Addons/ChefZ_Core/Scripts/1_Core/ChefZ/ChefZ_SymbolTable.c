//==============================================================================
// ChefZ_Sym / ChefZ_SymbolTable - interniertes Symbol fuer die Laufzeit
//
// Entwurf: 03 §2, §3.1, E1, E4.
//
// Drei Darstellungen, drei Zwecke (03 §2). Das hier ist die LAUFZEIT-Form:
// ein fortlaufendes int, schnell zu vergleichen, kompakt - und ausdruecklich
// NICHT stabil ueber Serverstarts. Wer diesen Zaehler persistiert, erzeugt nach
// dem naechsten Content-Update stille, flaechendeckende Datenkorruption
// (03 E2). Persistiert wird der Hash aus ChefZ_Identity, synchronisiert der
// Ordinal aus ChefZ_IdentityMap.
//
// Symbole sind prozessweit und domaenenuebergreifend (03 E4): "MEAT" als
// Kategorie und "MEAT" als Tag sind dasselbe Symbol. Ungefaehrlich, weil jede
// Registry ihren eigenen Schluesselraum fuehrt - der Kategoriemanager fragt nie
// die Tag-Registry. Spart eine Tabelle und macht Traces lesbar ("Sym 41" ist
// ueberall "MEAT").
//
// Der ehrliche Preis (03 E1): kein Compilezeit-Schutz. Ein Tippfehler "SAUSGE"
// faellt erst zur Laufzeit auf. Deshalb ist tools/chefz-validate keine Zugabe,
// sondern Auflage - es prueft Symbolreferenzen gegen die gemergten Registries.
//
// Layer: 1_Core.
//==============================================================================

typedef int ChefZ_Sym;

class ChefZ_SymbolTable
{
    static const ChefZ_Sym INVALID = 0;

    private static ref map<string, int> s_ByName;
    private static ref array<string>    s_ByOrdinal;

    private static void EnsureInit()
    {
        if (s_ByName)
            return;
        s_ByName    = new map<string, int>();
        s_ByOrdinal = new array<string>();
        s_ByOrdinal.Insert("");     // Index 0 ist INVALID und bleibt namenlos
    }

    /**
     * Liefert das Symbol zu einem Namen und legt es an, wenn es unbekannt ist.
     * Nur beim Laden und Kompilieren aufrufen, nie im heissen Pfad.
     */
    static ChefZ_Sym Intern(string name)
    {
        EnsureInit();

        if (name == "")
        {
            // 03 §7: leerer Name -> INVALID, einmal warnen. Der Schluessel
            // deckelt die Meldung; eine Logflut aus einer kaputten Datei mit
            // tausend leeren IDs hilft niemandem.
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.CONFIG, "sym.intern.empty", "ChefZ_SymbolTable.Intern() mit leerem Namen aufgerufen - liefert INVALID. " + "Ursache ist fast immer ein Record ohne \"id\".");
            return INVALID;
        }

        int existing;
        if (s_ByName.Find(name, existing))
            return existing;

        s_ByOrdinal.Insert(name);
        int sym = s_ByOrdinal.Count() - 1;
        s_ByName.Set(name, sym);
        return sym;
    }

    /**
     * Nachschlagen ohne Anlegen. INVALID, wenn unbekannt - und bewusst OHNE
     * Log: das ist ein heisser Pfad, und der Fehler wurde beim Laden bereits
     * gemeldet (03 §7).
     */
    static ChefZ_Sym Lookup(string name)
    {
        if (!s_ByName)
            return INVALID;
        int sym;
        if (s_ByName.Find(name, sym))
            return sym;
        return INVALID;
    }

    //! Klartext fuer Log und Trace. Leerstring fuer INVALID und Unbekanntes.
    static string Name(ChefZ_Sym sym)
    {
        if (!s_ByOrdinal)
            return "";
        if (sym <= INVALID || sym >= s_ByOrdinal.Count())
            return "";
        return s_ByOrdinal.Get(sym);
    }

    /**
     * Die Ordnungszahl eines Symbols als Text.
     *
     * ChefZ_Sym ist ein "typedef int", aber Enforce behandelt den Aliasnamen
     * beim Methodenaufruf als eigenen Typ: "sym.ToString()" scheitert mit
     * "Undefined function 'ChefZ_Sym.ToString'" und der ganze Skriptmodul
     * kompiliert nicht. Die Zuweisung an ein int ist dagegen zulaessig - der
     * Umweg ueber die lokale Variable ist also kein Schoenheitsfehler, sondern
     * die einzige Form, die uebersetzt.
     *
     * Deshalb steht das hier EINMAL und nicht an jeder Aufrufstelle: eine
     * Ordnungszahl im Text ist fast immer ein Diagnosefall, und die will man
     * nicht in drei Schreibweisen suchen.
     */
    static string Ordinal(ChefZ_Sym sym)
    {
        int value = sym;
        return value.ToString();
    }

    //! Wie Name(), aber mit sichtbarem Platzhalter statt Leerstring. Fuer
    //! Traces, in denen ein leerer Name wie ein fehlendes Feld aussaehe.
    static string NameOrMark(ChefZ_Sym sym)
    {
        string n = Name(sym);
        if (n == "")
            return "<invalid:" + Ordinal(sym) + ">";
        return n;
    }

    static bool IsValid(ChefZ_Sym sym)
    {
        if (!s_ByOrdinal)
            return false;
        return sym > INVALID && sym < s_ByOrdinal.Count();
    }

    static bool Has(string name)
    {
        return Lookup(name) != INVALID;
    }

    //! Anzahl der internierten Symbole, ohne INVALID.
    static int Count()
    {
        if (!s_ByOrdinal)
            return 0;
        return s_ByOrdinal.Count() - 1;
    }

    static void DebugDump(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();
        EnsureInit();

        outLines.Insert("Symbole: " + Count().ToString());
        for (int sym = 1; sym < s_ByOrdinal.Count(); sym++)
            outLines.Insert("  " + sym.ToString() + "  " + s_ByOrdinal.Get(sym));
    }

    /**
     * Leert die Tabelle. AUSSCHLIESSLICH fuer den Selbsttest.
     *
     * Zur Laufzeit aufgerufen wuerde sie jedes bereits aufgeloeste Symbol in
     * jedem kompilierten Selektor ungueltig machen - dieselbe Klasse von
     * Schaden, wegen der es in V1 keinen Config-Reload gibt (02 E5).
     */
    static void ResetForTest()
    {
        s_ByName    = null;
        s_ByOrdinal = null;
        EnsureInit();
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest (S1). Laeuft auf der echten Tabelle, legt aber
    //! ausschliesslich Namen mit Testpraefix an - Reset waere hier gefaehrlich.
    static bool SelfCheck()
    {
        int before = Count();

        ChefZ_Sym a = Intern("CHEFZ_SELFTEST_A");
        ChefZ_Sym b = Intern("CHEFZ_SELFTEST_B");
        ChefZ_Sym a2 = Intern("CHEFZ_SELFTEST_A");

        if (a == INVALID)                       return false;
        if (b == INVALID)                       return false;
        if (a != a2)                            return false;      // Internierung ist idempotent
        if (a == b)                             return false;
        if (Name(a) != "CHEFZ_SELFTEST_A")      return false;
        if (Lookup("CHEFZ_SELFTEST_B") != b)    return false;
        if (Lookup("CHEFZ_SELFTEST_NOPE") != INVALID) return false;
        if (!IsValid(a))                        return false;
        if (IsValid(INVALID))                   return false;
        if (IsValid(999999))                    return false;
        if (Name(INVALID) != "")                return false;
        if (Intern("") != INVALID)              return false;
        if (Count() != before + 2)              return false;

        return true;
    }
}
