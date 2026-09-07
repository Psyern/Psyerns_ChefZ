//==============================================================================
// ChefZ_KnowledgeState - was ein Spieler weiss
//
// Entwurf: ChefZ_Cookbook_Workflow §4.1.
//
// Zwei Mengen, sonst nichts:
//
//   knownIngredients   Zutatenklassen, die der Spieler im Inventar hatte
//   masteredRecipes    Rezepte, die er erfolgreich gekocht hat
//
// ---------------------------------------------------------------------------
// IM SPEICHER SYMBOLE, AUF DER PLATTE ZEICHENKETTEN
// ---------------------------------------------------------------------------
// §4.1 nennt Symbole, und im Speicher sind es welche: ein Symbolvergleich ist
// ein int-Vergleich, und der Matcher rechnet ohnehin mit ChefZ_Sym.
//
// Gespeichert wird trotzdem der KLARTEXT. Ein Symbol ist eine laufende
// Nummer aus ChefZ_SymbolTable, vergeben in der Reihenfolge, in der die
// Registries interniert werden. Diese Reihenfolge haengt an der Ladeordnung
// der Addons - installiert ein Betreiber ein Modul dazu, verschieben sich alle
// Nummern dahinter. Gespeicherte Ordinale zeigten danach auf fremde Zutaten,
// ohne dass irgendetwas auffiele.
//
// Der Preis ist Speicherplatz: rund 30 Byte je Eintrag statt 4. Bei 194
// Zutaten und 44 Rezepten ist das die guenstigere Seite des Tauschs.
//
// Layer: 3_Game. Reine Daten, kein Engine-Typ, keine Dabs-Referenz.
//==============================================================================

class ChefZ_KnowledgeState : Managed
{
    //! Beide Mengen als array<int> und nicht als set<ChefZ_Sym>: Enforce
    //! behandelt den Aliasnamen beim Instanziieren eines Templates als eigenen
    //! Typ, und array<int> ist der Typ, den jede andere Stelle auch sieht.
    private ref array<int> m_KnownIngredients;
    private ref array<int> m_MasteredRecipes;

    void ChefZ_KnowledgeState()
    {
        m_KnownIngredients = new array<int>();
        m_MasteredRecipes  = new array<int>();
    }

    //==========================================================================
    // Lesen
    //==========================================================================

    bool KnowsIngredient(ChefZ_Sym classSym)
    {
        if (!ChefZ_SymbolTable.IsValid(classSym))
            return false;
        int wert = classSym;
        return m_KnownIngredients.Find(wert) >= 0;
    }

    bool HasMastered(ChefZ_Sym recipeSym)
    {
        if (!ChefZ_SymbolTable.IsValid(recipeSym))
            return false;
        int wert = recipeSym;
        return m_MasteredRecipes.Find(wert) >= 0;
    }

    int IngredientCount() { return m_KnownIngredients.Count(); }
    int MasteredCount()   { return m_MasteredRecipes.Count(); }

    ChefZ_Sym IngredientAt(int index)
    {
        if (index < 0 || index >= m_KnownIngredients.Count())
            return ChefZ_SymbolTable.INVALID;
        return m_KnownIngredients.Get(index);
    }

    ChefZ_Sym MasteredAt(int index)
    {
        if (index < 0 || index >= m_MasteredRecipes.Count())
            return ChefZ_SymbolTable.INVALID;
        return m_MasteredRecipes.Get(index);
    }

    //==========================================================================
    // Schreiben
    //==========================================================================

    //! @return true, wenn die Zutat NEU war. Nur dann lohnt ein Speichern und
    //!         nur dann wird ChefZ_OnRecipeDiscovered gefeuert.
    bool AddIngredient(ChefZ_Sym classSym)
    {
        if (!ChefZ_SymbolTable.IsValid(classSym))
            return false;
        int wert = classSym;
        if (m_KnownIngredients.Find(wert) >= 0)
            return false;
        m_KnownIngredients.Insert(wert);
        return true;
    }

    //! @return true, wenn das Rezept vorher nicht gemeistert war.
    bool AddMastered(ChefZ_Sym recipeSym)
    {
        if (!ChefZ_SymbolTable.IsValid(recipeSym))
            return false;
        int wert = recipeSym;
        if (m_MasteredRecipes.Find(wert) >= 0)
            return false;
        m_MasteredRecipes.Insert(wert);
        return true;
    }

    void Clear()
    {
        m_KnownIngredients.Clear();
        m_MasteredRecipes.Clear();
    }

    //==========================================================================
    // Persistenz
    //==========================================================================

    /**
     * Schreibt beide Mengen als Klartext.
     *
     * Aufbau: erst die Anzahl, dann die Namen. Ein unbekannter Name beim Lesen
     * wird stillschweigend uebersprungen - genau das soll passieren, wenn ein
     * Betreiber ein Content-Modul entfernt. Der Spieler verliert dann das
     * Wissen ueber dessen Zutaten und behaelt den Rest.
     */
    void Save(ParamsWriteContext ctx)
    {
        int n = m_KnownIngredients.Count();
        ctx.Write(n);
        for (int i = 0; i < n; i++)
        {
            string name = ChefZ_SymbolTable.Name(m_KnownIngredients.Get(i));
            ctx.Write(name);
        }

        int m = m_MasteredRecipes.Count();
        ctx.Write(m);
        for (int j = 0; j < m; j++)
        {
            string rez = ChefZ_SymbolTable.Name(m_MasteredRecipes.Get(j));
            ctx.Write(rez);
        }
    }

    /**
     * Liest zurueck, was Save() geschrieben hat.
     *
     * @return false, sobald ein Lesevorgang scheitert. Der Aufrufer muss das
     *         weiterreichen: DayZ verwirft den ganzen Spielerstand, wenn
     *         OnStoreLoad false liefert, und das ist richtig so - ein halb
     *         gelesener Kontext bringt jeden nachfolgenden Mod aus dem Tritt.
     */
    bool Load(ParamsReadContext ctx)
    {
        Clear();

        int n;
        if (!ctx.Read(n))
            return false;
        for (int i = 0; i < n; i++)
        {
            string name;
            if (!ctx.Read(name))
                return false;
            AddIngredient(ChefZ_SymbolTable.Lookup(name));
        }

        int m;
        if (!ctx.Read(m))
            return false;
        for (int j = 0; j < m; j++)
        {
            string rez;
            if (!ctx.Read(rez))
                return false;
            AddMastered(ChefZ_SymbolTable.Lookup(rez));
        }
        return true;
    }

    //==========================================================================
    // Diagnose
    //==========================================================================

    string ToLine()
    {
        string s = "Wissen: " + m_KnownIngredients.Count().ToString() + " Zutaten, ";
        s = s + m_MasteredRecipes.Count().ToString() + " gemeisterte Rezepte";
        return s;
    }
}
