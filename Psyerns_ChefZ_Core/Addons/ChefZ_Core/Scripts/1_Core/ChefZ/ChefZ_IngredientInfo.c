//==============================================================================
// ChefZ_IngredientInfo - die aufgeloesten und eingefrorenen Stammdaten
//                        einer Zutatenklasse
//
// Entwurf: 05 §3.1 (Feldliste woertlich), 05 §4 (Aufbau), 05 §5 (Zustand),
// 05 E2 (Vererbung einmal beim Boot), 05 E3 (nicht deklarierte Klassen bleiben
// adressierbar).
//
// Der Unterschied zu ChefZ_IngredientDef ist der Kern dieses Teilsystems:
//
//   ChefZ_IngredientDef    ROHDATEN. Strings, wie sie in config.cpp oder JSON
//                          stehen. Felder duerfen "nicht gesetzt" sein, damit
//                          Vererbung und Overlay-Patch funktionieren.
//   ChefZ_IngredientInfo   ERGEBNIS. Symbole statt Strings, Vorfahren-Bitset
//                          statt Kategorieliste, jedes Feld entschieden.
//                          Nach Build unveraenderlich.
//
// Warum die Trennung: zur Laufzeit darf kein Kettenlauf, kein Stringvergleich
// und kein Defaultzweig mehr passieren (05 E2). Der Preis ist eine zweite
// Struktur; der Gewinn ist, dass der heisseste Pfad des Mods nur noch Bits und
// ints anfasst.
//
// KEIN CONTENT: diese Datei nennt keine Kategorie, keinen Tag, keinen Zustand
// und keine Zutat. Sie beschreibt nur, welche FELDER eine Zutat hat.
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

class ChefZ_IngredientInfo
{
    //! Klassenname als Symbol. Bei einer deklarierten Zutat immer gueltig.
    ChefZ_Sym classSym;

    //! DIREKT deklarierte Kategorien, ohne Vorfahren. Fuer Anzeige, Cookbook
    //! und Diagnose - der Matcher benutzt ausschliesslich die Closure.
    ref array<ChefZ_Sym> categories;

    //! Self-or-ancestor-Bitset (04 E1). Nie null: eine Zutat ohne Kategorie
    //! bekommt eine leere Closure, damit jeder Leser bedingungslos testen kann.
    ref ChefZ_CategoryClosure closure;

    //! Tags AUS DER KLASSE. Die effektiven Tags eines Items entstehen erst im
    //! Collector aus Klasse + Zustand + Qualitaet (05 E4).
    ref array<ChefZ_Sym> staticTags;

    ChefZ_Sym defaultState;         // INVALID = Zustand ergibt sich nicht aus der Klasse
    ChefZ_Sym quantityUnit;         // Symbol der Mengeneinheit (05 §6)
    float     unitsPerWholeItem;    // ein volles Item entspricht wie vielen Einheiten
    bool      decays;               // speist CanDecay() auf der Traegerklasse (01 V9)
    ChefZ_Sym containerCategory;    // Behaelterbindung (16)
    ChefZ_Sym returnContainer;      // Rueckgabeklasse (16)

    //! true, sobald die Klasse ueberhaupt deklariert ist. Ein
    //! ChefZ_IngredientInfo entsteht nur fuer deklarierte Klassen; das Feld
    //! wandert in die ItemFacts, wo es auch false sein kann (05 E3).
    bool      isChefZManaged;

    //! Herkunft der Deklaration. Nur fuer Diagnose und Ladebericht.
    string    sourceRef;

    void ChefZ_IngredientInfo()
    {
        classSym          = ChefZ_SymbolTable.INVALID;
        categories        = new array<ChefZ_Sym>();
        closure           = new ChefZ_CategoryClosure();
        staticTags        = new array<ChefZ_Sym>();
        defaultState      = ChefZ_SymbolTable.INVALID;
        quantityUnit      = ChefZ_SymbolTable.INVALID;
        unitsPerWholeItem = 1.0;
        decays            = false;
        containerCategory = ChefZ_SymbolTable.INVALID;
        returnContainer   = ChefZ_SymbolTable.INVALID;
        isChefZManaged    = false;
        sourceRef         = "";
    }

    bool HasTag(ChefZ_Sym tag)
    {
        return staticTags.Find(tag) >= 0;
    }

    string ToLine()
    {
        string s = ChefZ_SymbolTable.NameOrMark(classSym);

        s = s + "  kat=[";
        for (int i = 0; i < categories.Count(); i++)
        {
            if (i > 0)
                s = s + ",";
            s = s + ChefZ_SymbolTable.Name(categories.Get(i));
        }
        s = s + "]";

        s = s + " tags=[";
        for (int t = 0; t < staticTags.Count(); t++)
        {
            if (t > 0)
                s = s + ",";
            s = s + ChefZ_SymbolTable.Name(staticTags.Get(t));
        }
        s = s + "]";

        s = s + " closure=" + closure.ToDebugString() + " einheit=" + ChefZ_SymbolTable.Name(quantityUnit) + "x" + unitsPerWholeItem.ToString() + " verdirbt=" + decays.ToString();

        if (ChefZ_SymbolTable.IsValid(defaultState))
            s = s + " zustand=" + ChefZ_SymbolTable.Name(defaultState);
        if (ChefZ_SymbolTable.IsValid(containerCategory))
            s = s + " behaelter=" + ChefZ_SymbolTable.Name(containerCategory);
        if (ChefZ_SymbolTable.IsValid(returnContainer))
            s = s + " rueckgabe=" + ChefZ_SymbolTable.Name(returnContainer);

        return s;
    }
}
