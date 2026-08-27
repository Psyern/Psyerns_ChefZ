//==============================================================================
// ChefZ_TextList - die immer gleichen drei Handgriffe an einer string-Liste
//
// Entwurf: kein eigener Abschnitt. Diese Klasse ist reine Entdopplung.
//
// Ein ref array<string> aus JSON ist null, wenn das Feld fehlte (02 E3,
// Mittel 1). Genau deshalb steht vor jedem Zugriff eine Nullpruefung, und
// genau deshalb stand sie bis S6 an rund zwanzig Stellen ausgeschrieben.
// ChefZ_RecipeDef mit seinen acht Stringlisten haette daraus vierzig gemacht.
//
// Die Funktionen behandeln null wie eine leere Liste. Das ist die einzige
// Lesart, die zum Sentinel passt: "Feld nicht geschrieben" und "Feld leer
// geschrieben" sind fuer TRIM und ZAEHLEN dasselbe. Wo der Unterschied
// fachlich zaehlt - excludeStates in 07 E5 - fragt der Aufrufer weiterhin
// selbst auf null, und diese Klasse nimmt ihm das nicht ab.
//
// KEIN CONTENT, kein Zustand, keine Allokation ausser in SymbolsOf().
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_TextList
{
    //! Laenge, null-sicher.
    static int Count(array<string> list)
    {
        if (!list)
            return 0;
        return list.Count();
    }

    static bool IsEmpty(array<string> list)
    {
        return Count(list) == 0;
    }

    //! TrimInPlace auf jedem Eintrag. Veraendert die uebergebene Liste - das
    //! ist gewollt und der Grund, warum die Funktion nur aus Normalize()
    //! gerufen wird.
    static void TrimAll(array<string> list)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
        {
            string s = list.Get(i);
            s.TrimInPlace();
            list.Set(i, s);
        }
    }

    static bool Contains(array<string> list, string value)
    {
        if (!list)
            return false;
        return list.Find(value) >= 0;
    }

    /**
     * Namen internieren, ohne Duplikate und ohne leere Eintraege.
     *
     * outSyms wird geleert und gefuellt - der Aufrufer bringt die Liste mit.
     * Bewusst "notnull" statt "out": ein out-Parameter, der ein FELD eines
     * anderen Objekts entgegennimmt, ist in Enforce nirgends zugesichert, und
     * genau so wird diese Funktion gerufen (ChefZ_RecipeCompiler fuellt die
     * Listen im kompilierten Kontext). Dieselbe Schreibweise benutzt bereits
     * ChefZ_SlotEvaluator.IndicesToHandles().
     *
     * Ein leerer Name ergibt kein Symbol: er waere sonst als "gueltiges Symbol
     * namens Leerstring" nicht von einem echten zu unterscheiden.
     */
    static void SymbolsOf(array<string> list, notnull array<ChefZ_Sym> outSyms)
    {
        outSyms.Clear();

        if (!list)
            return;

        for (int i = 0; i < list.Count(); i++)
        {
            string name = list.Get(i);
            if (name == "")
                continue;

            ChefZ_Sym sym = ChefZ_SymbolTable.Intern(name);
            if (!ChefZ_SymbolTable.IsValid(sym))
                continue;
            if (outSyms.Find(sym) >= 0)
                continue;
            outSyms.Insert(sym);
        }
    }

    //! Kommaliste fuer Meldungen. "(leer)", wenn nichts drinsteht - ein
    //! unsichtbarer Leerstring in einer Fehlermeldung ist wertlos.
    static string Join(array<string> list, string separator)
    {
        if (!list || list.Count() == 0)
            return "(leer)";

        string s = "";
        for (int i = 0; i < list.Count(); i++)
        {
            if (i > 0)
                s = s + separator;
            s = s + list.Get(i);
        }
        return s;
    }

    //! Symbolnamen als Kommaliste - dasselbe fuer die kompilierte Seite.
    static string JoinSymbols(array<ChefZ_Sym> syms, string separator)
    {
        if (!syms || syms.Count() == 0)
            return "(leer)";

        string s = "";
        for (int i = 0; i < syms.Count(); i++)
        {
            if (i > 0)
                s = s + separator;
            s = s + ChefZ_SymbolTable.NameOrMark(syms.Get(i));
        }
        return s;
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        if (Count(null) != 0)                       return false;
        if (!IsEmpty(null))                         return false;
        if (Contains(null, "x"))                    return false;
        if (Join(null, ", ") != "(leer)")           return false;

        array<string> list = new array<string>();
        list.Insert("  A  ");
        list.Insert("B");
        list.Insert("");
        list.Insert("A");
        TrimAll(list);
        if (list.Get(0) != "A")                     return false;
        if (Count(list) != 4)                       return false;
        if (!Contains(list, "B"))                   return false;
        if (Join(list, "|") != "A|B||A")            return false;

        array<ChefZ_Sym> syms = new array<ChefZ_Sym>();
        SymbolsOf(list, syms);
        if (syms.Count() != 2)                      return false;   // A, B - ohne Leer, ohne Dublette

        TrimAll(null);                              // darf nicht knallen
        SymbolsOf(null, syms);
        if (syms.Count() != 0)                      return false;

        return true;
    }
}
