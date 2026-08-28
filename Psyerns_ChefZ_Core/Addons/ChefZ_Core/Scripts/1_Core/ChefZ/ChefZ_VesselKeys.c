//==============================================================================
// ChefZ_GateKind / ChefZ_VesselKeys - der Vorfilter des invertierten Index
//
// Entwurf: 08 §5.1 ("requiredSym -> array<recipeIdx> je Symbol, das ein
// Pflicht-Slot fordert. Kandidaten = Geraetemenge geschnitten mit der
// Vereinigung ueber die im Gefaess vorhandenen Symbole"), 08 §5.2
// (Knotenbudget), 07 E4 (der engste Test zuerst).
//
// ---------------------------------------------------------------------------
// Wozu das gut ist
// ---------------------------------------------------------------------------
// Ohne Vorfilter laeuft der Matcher fuer jedes Kandidatenrezept an, baut
// Kandidatenlisten je Slot auf und stellt dabei fest, dass die erste
// Pflichtzutat gar nicht im Topf liegt. Das ist die haeufigste Absage
// ueberhaupt, und sie ist ohne einen einzigen Suchknoten zu haben:
//
//     "Verlangt dieses Rezept zwingend ein Symbol, das hier niemand traegt?"
//
// ChefZ_VesselKeys beantwortet genau diese Frage - einmal je Auswertung
// aufgebaut, danach ein Find() oder ein Bit-Test je Kandidat.
//
// ---------------------------------------------------------------------------
// Warum die Kategorien als BITSET und nicht als Symbolliste kommen
// ---------------------------------------------------------------------------
// ChefZ_ItemFacts traegt seine Kategorien als ChefZ_CategoryClosure, also als
// Bitset ueber den Kategoriebaum (04 E1). Daraus Symbole zurueckzugewinnen
// hiesse, den Baum zu befragen - und 1_Core kennt keinen Manager (00 §4).
// Stattdessen werden die Closures ver-ODER-t: eine Kategorie liegt im Gefaess,
// wenn irgendein Item ihr Bit traegt. Der Test ist dann derselbe wie im
// Selektor, naemlich HasBit().
//
// KEIN CONTENT, kein Zustand ueber eine Auswertung hinaus.
//
// Layer: 1_Core.
//==============================================================================

/**
 * Die vier Blattarten, die sich in einem Schritt gegen den Gefaessinhalt
 * pruefen lassen - plus NONE fuer "dieses Rezept hat kein Tor".
 *
 * Bewusst eigene Konstanten statt ChefZ_SelectorOp: der Selektor kennt zehn
 * Operatoren, von denen sechs kein Tor bilden koennen. Sie mitzuschleppen
 * hiesse, in jeder Fallunterscheidung sechs unerreichbare Zweige zu erklaeren.
 */
class ChefZ_GateKind
{
    static const int NONE     = 0;
    static const int CLASS    = 1;
    static const int CATEGORY = 2;
    static const int TAG      = 3;
    static const int STATE    = 4;

    static string Name(int kind)
    {
        if (kind == NONE)       return "-";
        if (kind == CLASS)      return "class";
        if (kind == CATEGORY)   return "category";
        if (kind == TAG)        return "tag";
        if (kind == STATE)      return "state";
        return "?";
    }
}

//==============================================================================

/**
 * Was im Gefaess ueberhaupt vorkommt - als Mengen, nicht als Items.
 *
 * Wird EINMAL je Auswertung aus dem ChefZ_FactSnapshot gebaut. Die Listen sind
 * kurz (ein Kessel hat selten mehr als ein Dutzend verschiedener Klassen), und
 * sie werden zwischen zwei Auswertungen wiederverwendet - Build() leert, statt
 * neu anzulegen.
 *
 * Rein lesend gegenueber dem Snapshot: kein Feld wird angefasst, insbesondere
 * nicht slotBoundTo. Der Vorfilter darf die Bindung nicht vorwegnehmen.
 */
class ChefZ_VesselKeys
{
    private ref array<ChefZ_Sym>        m_Classes;
    private ref array<ChefZ_Sym>        m_Tags;
    private ref array<ChefZ_Sym>        m_States;
    private ref ChefZ_CategoryClosure   m_Categories;
    private int m_ItemCount;

    void ChefZ_VesselKeys()
    {
        m_Classes    = new array<ChefZ_Sym>();
        m_Tags       = new array<ChefZ_Sym>();
        m_States     = new array<ChefZ_Sym>();
        m_Categories = new ChefZ_CategoryClosure();
        m_ItemCount  = 0;
    }

    /**
     * Aus einer Faktenliste aufbauen. Ein Durchgang, O(Items * Tags).
     *
     * snapshot darf null sein: dann ist das Gefaess leer, und jedes Rezept mit
     * Torsymbol faellt heraus. Das ist die richtige Antwort und kein Fehler.
     */
    void Build(ChefZ_FactSnapshot snapshot)
    {
        Clear();
        if (!snapshot)
            return;

        m_ItemCount = snapshot.Count();

        for (int i = 0; i < snapshot.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.Get(i);
            if (!facts)
                continue;

            AddUnique(m_Classes, facts.classSym);
            AddUnique(m_States, facts.chefzState);

            if (facts.closure)
                m_Categories.OrWith(facts.closure);

            if (!facts.tags)
                continue;
            for (int t = 0; t < facts.tags.Count(); t++)
                AddUnique(m_Tags, facts.tags.Get(t));
        }
    }

    void Clear()
    {
        m_Classes.Clear();
        m_Tags.Clear();
        m_States.Clear();
        m_Categories.Clear();
        m_ItemCount = 0;
    }

    //==========================================================================
    // Der eine Test, fuer den es diese Klasse gibt
    //==========================================================================

    /**
     * Kann ein Rezept mit diesem Tor hier ueberhaupt binden?
     *
     * ChefZ_GateKind.NONE liefert IMMER true. Ein Rezept ohne Tor wird nicht
     * ausgeschlossen, es wird nur nicht billig ausgeschlossen - die Richtung,
     * in die ein Zweifelsfall laufen muss (Invariante I2: der Ausfallpfad ist
     * "mehr Aufwand", nie "falsches Ergebnis").
     */
    bool Admits(int gateKind, ChefZ_Sym gateSym, int gateBit)
    {
        // if-Kette und kein switch: die Torarten sind "static const int" einer
        // anderen Klasse, und ein switch-case verlangt in Enforce eine
        // Konstante, deren Auswertbarkeit an dieser Stelle nirgends zugesichert
        // ist. Fuenf Vergleiche kosten nichts; ein nicht uebersetzender Core
        // kostet alles.
        if (gateKind == ChefZ_GateKind.NONE)
            return true;

        if (gateKind == ChefZ_GateKind.CLASS)
            return m_Classes.Find(gateSym) >= 0;

        if (gateKind == ChefZ_GateKind.CATEGORY)
        {
            if (gateBit < 0)
                return true;                // Bit unbekannt -> nicht ausschliessen
            return m_Categories.HasBit(gateBit);
        }

        if (gateKind == ChefZ_GateKind.TAG)
            return m_Tags.Find(gateSym) >= 0;

        if (gateKind == ChefZ_GateKind.STATE)
            return m_States.Find(gateSym) >= 0;

        // Unbekannte Torart: durchlassen. Ein Vorfilter, der bei einem
        // Programmierfehler Rezepte verschluckt, waere schlimmer als einer,
        // der zu viele durchlaesst.
        return true;
    }

    //==========================================================================

    int ItemCount()          { return m_ItemCount; }
    int ClassCount()         { return m_Classes.Count(); }
    int TagCount()           { return m_Tags.Count(); }
    int StateCount()         { return m_States.Count(); }

    bool HasClass(ChefZ_Sym cls)   { return m_Classes.Find(cls) >= 0; }
    bool HasTag(ChefZ_Sym tag)     { return m_Tags.Find(tag) >= 0; }
    bool HasState(ChefZ_Sym state) { return m_States.Find(state) >= 0; }
    bool HasCategoryBit(int bit)
    {
        if (bit < 0)
            return false;
        return m_Categories.HasBit(bit);
    }

    private void AddUnique(notnull array<ChefZ_Sym> list, ChefZ_Sym sym)
    {
        if (!ChefZ_SymbolTable.IsValid(sym))
            return;
        if (list.Find(sym) >= 0)
            return;
        list.Insert(sym);
    }

    string ToDebugString()
    {
        return "items=" + m_ItemCount.ToString() + " klassen=" + m_Classes.Count().ToString() + " tags=" + m_Tags.Count().ToString() + " zustaende=" + m_States.Count().ToString() + " kategoriebits=" + m_Categories.CountBits().ToString();
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_VesselKeys keys = new ChefZ_VesselKeys();

        // Leeres Gefaess: kein Tor wird bedient, NONE trotzdem immer.
        keys.Build(null);
        if (!keys.Admits(ChefZ_GateKind.NONE, ChefZ_SymbolTable.INVALID, -1))    return false;
        ChefZ_Sym clsA = ChefZ_SymbolTable.Intern("CHEFZ_VK_KLASSE_A");
        if (keys.Admits(ChefZ_GateKind.CLASS, clsA, -1))                         return false;

        ChefZ_FactSnapshot snap = new ChefZ_FactSnapshot();
        ChefZ_ItemFacts f = snap.Acquire();
        f.handle     = 1;
        f.classSym   = clsA;
        f.chefzState = ChefZ_SymbolTable.Intern("CHEFZ_VK_ZUSTAND");
        f.AddTag(ChefZ_SymbolTable.Intern("CHEFZ_VK_TAG"));
        f.closure.SetBit(5);

        keys.Build(snap);
        if (keys.ItemCount() != 1)                                               return false;
        if (!keys.Admits(ChefZ_GateKind.CLASS, clsA, -1))                        return false;
        if (!keys.Admits(ChefZ_GateKind.CATEGORY, ChefZ_SymbolTable.INVALID, 5)) return false;
        if (keys.Admits(ChefZ_GateKind.CATEGORY, ChefZ_SymbolTable.INVALID, 6))  return false;
        if (!keys.Admits(ChefZ_GateKind.TAG, ChefZ_SymbolTable.Lookup("CHEFZ_VK_TAG"), -1))
            return false;
        if (!keys.Admits(ChefZ_GateKind.STATE, ChefZ_SymbolTable.Lookup("CHEFZ_VK_ZUSTAND"), -1))
            return false;
        if (keys.Admits(ChefZ_GateKind.TAG, clsA, -1))                           return false;

        // Ein Bit ohne Index darf nicht ausschliessen.
        if (!keys.Admits(ChefZ_GateKind.CATEGORY, ChefZ_SymbolTable.INVALID, -1)) return false;

        keys.Clear();
        if (keys.ClassCount() != 0)                                              return false;

        return true;
    }
}
