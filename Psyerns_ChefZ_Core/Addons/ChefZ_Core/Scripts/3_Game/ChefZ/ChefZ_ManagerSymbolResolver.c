//==============================================================================
// ChefZ_ManagerSymbolResolver - der Nachschlager, der die Manager fragt
//
// Entwurf: 07 §5 (was der Compiler beim BOOT aufloest), 00 §4 (1_Core kennt
// keinen Manager), 19 S5/S6.
//
// Das Gegenstueck zum tabellengetriebenen ChefZ_SymbolResolver aus 1_Core:
// dieselben Fragen, aber beantwortet aus dem laufenden Kategorie- und
// Zutatenmanager. Der Selektorcompiler merkt keinen Unterschied - genau das
// ist der Zweck der Trennung.
//
// ---------------------------------------------------------------------------
// Zwei Wege, und warum beide noetig sind
// ---------------------------------------------------------------------------
// Ueberschrieben werden die Fragen, deren Antwort sich aus einem Index ergibt
// (Kategoriebit, Tiefe, Tag, Kandidatenzahl) - dort ist ein Durchgriff
// billiger und immer aktuell.
//
// Nachgetragen (Prepare) werden die Angaben, fuer die es keinen Index gibt:
// welche Mengeneinheiten ueberhaupt vorkommen und welche Klassen deklariert
// sind. Beides wird beim Kompilieren dutzendfach abgefragt und aendert sich
// nach dem Build nicht mehr - eine einmalige Uebernahme in die Basistabellen
// ist billiger als jedes Mal ueber alle Zutaten zu laufen.
//
// ---------------------------------------------------------------------------
// Qualitaetsraenge: nachgetragen, nicht durchgegriffen
// ---------------------------------------------------------------------------
// Seit S10 traegt ChefZ_QualityTierDef Rang, Schwelle und Stufensatz (12 §3),
// und der ChefZ_QualityManager hat sie zu einer Leiter geordnet. Prepare()
// uebernimmt sie ueber DefineQuality() in die Basistabellen.
//
// Uebernommen und nicht durchgegriffen, weil der WIRKSAME Rang eine abgeleitete
// Groesse ist: der Manager sortiert nach minScore und nummeriert neu (12 §8).
// Einen Rang aus der Ladereihenfolge oder aus dem rohen Feld zu nehmen waere
// schlimmer als keiner - die Qualitaetsschwelle eines Rezepts haenge dann an
// etwas anderem als an der Leiter, gegen die sie zur Laufzeit geprueft wird.
//
// Fehlt der Manager oder ist er leer, bleiben die Tabellen leer, und ein
// Rezept mit "minQuality" wird abgewiesen - mit der Meldung "unbekannte
// Qualitaetsstufe", nicht stillschweigend durchgewunken.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_ManagerSymbolResolver extends ChefZ_SymbolResolver
{
    private ChefZ_CategoryManager   m_Categories;   // ohne ref: Singleton
    private ChefZ_IngredientManager m_Ingredients;  // ohne ref: Singleton
    private ChefZ_ConfigManager     m_Config;       // ohne ref: Singleton
    private ChefZ_QualityManager    m_Quality;      // ohne ref: Singleton

    /**
     * Verbindet den Nachschlager mit den Managern und traegt nach, was sich
     * nicht durchgreifen laesst.
     *
     * Jeder Parameter darf null sein. Ein fehlender Manager heisst "kennt
     * nichts" - dann werden Rezepte abgewiesen, statt auf alles zu matchen.
     * Das ist die Richtung, in die jeder Fehler laufen soll (02 §8).
     */
    void Prepare(ChefZ_CategoryManager categories, ChefZ_IngredientManager ingredients, ChefZ_ConfigManager config, ChefZ_QualityManager quality = null)
    {
        m_Categories  = categories;
        m_Ingredients = ingredients;
        m_Config      = config;
        m_Quality     = quality;

        AdoptIngredientFacts();
        AdoptQualityTiers();
    }

    /**
     * Qualitaetsstufen mit ihrem WIRKSAMEN Rang und ihrem Stufensatz in die
     * Basistabellen uebernehmen (12 §3, 12 E4).
     *
     * Zwingend NACH ChefZ_QualityManager.Build(): vorher gibt es weder Leiter
     * noch Rang. Der Config Manager stellt die Reihenfolge her.
     */
    private void AdoptQualityTiers()
    {
        if (!m_Quality || !m_Quality.IsReady())
            return;

        array<ChefZ_Sym> tiers = new array<ChefZ_Sym>();
        m_Quality.GetAll(tiers);

        for (int i = 0; i < tiers.Count(); i++)
        {
            ChefZ_Sym sym = tiers.Get(i);

            int rank = m_Quality.GetRank(sym);
            if (rank < 0)
                continue;

            // Zwischenvariablen und keine Aufrufe auf Rueckgabewerten:
            // Enforce sichert Methodenaufrufe auf temporaeren Strings nicht zu
            // (dieselbe Vorsicht wie in ChefZ_StateManager.GetPersistHash).
            string tierName = ChefZ_SymbolTable.Name(sym);
            string setName  = ChefZ_SymbolTable.Name(m_Quality.GetTierSet(sym));

            DefineQuality(tierName, rank, setName);
        }
    }

    //! Einheiten und Klassennamen in die Basistabellen uebernehmen.
    private void AdoptIngredientFacts()
    {
        if (!m_Ingredients)
            return;

        int count = m_Ingredients.GetKnownCount();
        for (int i = 0; i < count; i++)
        {
            ChefZ_IngredientInfo info = m_Ingredients.GetAt(i);
            if (!info)
                continue;

            if (ChefZ_SymbolTable.IsValid(info.classSym))
                DefineClass(ChefZ_SymbolTable.Name(info.classSym));

            if (ChefZ_SymbolTable.IsValid(info.quantityUnit))
                DefineUnit(ChefZ_SymbolTable.Name(info.quantityUnit));
        }

        SetUniverseSize(count);
    }

    //==========================================================================
    // Durchgriff auf die Indizes
    //==========================================================================

    override int CategoryBit(ChefZ_Sym category)
    {
        if (!m_Categories)
            return ChefZ_SymbolResolver.UNKNOWN;

        int bit = m_Categories.GetBitIndex(category);
        if (bit < 0)
            return ChefZ_SymbolResolver.UNKNOWN;
        return bit;
    }

    override int CategoryDepth(ChefZ_Sym category)
    {
        if (!m_Categories)
            return ChefZ_SymbolResolver.UNKNOWN;
        return m_Categories.GetDepth(category);
    }

    override bool TagExists(ChefZ_Sym tag)
    {
        if (!m_Categories)
            return false;
        return m_Categories.TagExists(tag);
    }

    override bool StateExists(ChefZ_Sym state)
    {
        if (!m_Config || !m_Config.States())
            return false;
        return m_Config.States().Contains(state);
    }

    override int EstimateCandidates(ChefZ_Sym categoryOrTag)
    {
        if (!m_Ingredients)
            return 0;
        return m_Ingredients.EstimateCandidateCount(categoryOrTag);
    }

    override int UniverseSize()
    {
        if (m_Ingredients)
        {
            int n = m_Ingredients.GetKnownCount();
            if (n > 0)
                return n;
        }
        return super.UniverseSize();
    }
}
