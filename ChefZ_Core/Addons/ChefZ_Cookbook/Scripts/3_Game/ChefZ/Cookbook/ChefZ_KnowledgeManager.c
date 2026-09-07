//==============================================================================
// ChefZ_KnowledgeManager - leitet den Rezeptzustand aus dem Wissensstand ab
//
// Entwurf: ChefZ_Cookbook_Workflow §4.2, §4.3.
//
// ---------------------------------------------------------------------------
// WAS ER NICHT IST
// ---------------------------------------------------------------------------
// Er haelt KEINEN Spielerzustand und kennt keinen Spieler. PlayerBase gibt es
// erst in 4_World; die Ereignisanbindung und das Aufloesen der Spielerkennung
// stehen deshalb in ChefZ_CookbookServer. Hier liegt nur die Rechenvorschrift.
//
// Das Wissen selbst liegt am Spieler (ChefZ_PlayerKnowledge), weil es dort
// gespeichert und geladen wird und weil eine zentrale Tabelle bei jedem
// Verbindungsabbruch aufgeraeumt werden muesste.
//
// ---------------------------------------------------------------------------
// DER CORE WIRD NICHT ANGEFASST
// ---------------------------------------------------------------------------
// Regel 1 dieses Meilensteins. Das geht auf, weil ChefZ_EventNames den Haken
// bereits deklariert: RECIPE_DISCOVERED ist seit S13 da und wird nirgends
// gefeuert. Der Core hat den Anschluss vorgesehen und das Wissenssystem
// ausgelassen - genau die Luecke, die dieses Addon fuellt.
//
// Layer: 3_Game. Keine Dabs-Referenz: dieses Addon muss ohne Dabs uebersetzen
// und laufen (Regel 3).
//==============================================================================

class ChefZ_KnowledgeManager : Managed
{
    private static ref ChefZ_KnowledgeManager s_Instance;

    //! Ab wie vielen bekannten Pflichtslots ein Rezept als PARTIAL gilt.
    //! Aus CfgChefZCookbook, Vorgabe 1 - siehe Kopf von Ableitung().
    private int m_PartialMinKnownSlots;

    //! Ein wiederverwendeter Fakten-Datensatz. Die Ableitung laeuft ueber alle
    //! Rezepte mal alle bekannten Zutaten; je Paar einen neuen anzulegen waere
    //! bei 44 Rezepten und knapp 200 Zutaten eine fuenfstellige Zahl von
    //! Allokationen je Buchoeffnung.
    private ref ChefZ_ItemFacts m_Probe;

    void ChefZ_KnowledgeManager()
    {
        m_PartialMinKnownSlots = 1;
        m_Probe                = new ChefZ_ItemFacts();
    }

    static ChefZ_KnowledgeManager Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_KnowledgeManager();
        return s_Instance;
    }

    //==========================================================================
    // Einstellung
    //==========================================================================

    /**
     * partialMinKnownSlots aus CfgChefZCookbook.
     *
     * Die Einstellung steht bewusst im EIGENEN Addon und nicht in den
     * CoreSettings: Regel 1 verbietet jede Core-Aenderung fuer das Kochbuch,
     * und eine Balancingzahl des Buches hat im Core auch nichts verloren.
     *
     * Wird von ChefZ_CookbookServer einmal beim Start gerufen. Ein zweiter
     * Aufruf schadet nicht.
     */
    void ReadSettings()
    {
        m_PartialMinKnownSlots = 1;

        if (!g_Game)
            return;
        if (!g_Game.ConfigIsExisting("CfgChefZCookbook partialMinKnownSlots"))
            return;

        int wert = g_Game.ConfigGetInt("CfgChefZCookbook partialMinKnownSlots");
        if (wert >= 1)
            m_PartialMinKnownSlots = wert;
    }

    int GetPartialMinKnownSlots() { return m_PartialMinKnownSlots; }

    //==========================================================================
    // Die Ableitung
    //==========================================================================

    /**
     * Der Zustand EINES Rezepts fuer EINEN Wissensstand.
     *
     * Gezaehlt werden ausschliesslich PFLICHTSLOTS. Ein optionaler Slot sagt
     * nichts darueber aus, ob jemand das Gericht kennt - er ist eine Zutat, die
     * man weglassen darf, und ein Rezept an einer Kuer scheitern zu lassen
     * waere unbegruendbar.
     */
    int DeriveStatus(ChefZ_KnowledgeState stand, ChefZ_CompiledRecipe rezept)
    {
        if (!stand || !rezept)
            return ChefZ_RecipeStatus.UNKNOWN;

        int gesamt = 0;
        int bekannt = 0;
        CountSlots(stand, rezept, gesamt, bekannt);

        if (gesamt == 0)
            return ChefZ_RecipeStatus.UNKNOWN;

        if (bekannt >= gesamt)
        {
            if (stand.HasMastered(rezept.recipeSym))
                return ChefZ_RecipeStatus.MASTERED;
            return ChefZ_RecipeStatus.KNOWN;
        }

        if (bekannt >= m_PartialMinKnownSlots)
            return ChefZ_RecipeStatus.PARTIAL;

        return ChefZ_RecipeStatus.UNKNOWN;
    }

    /**
     * Zaehlt Pflichtslots und davon die, fuer die eine bekannte Zutat passt.
     *
     * Beide Zahlen kommen als out heraus, weil die Fortschrittsanzeige des
     * Buches sie BEIDE braucht ("3 von 5 Zutaten bekannt") und ein zweiter
     * Durchlauf nur fuer den Nenner Verschwendung waere.
     */
    void CountSlots(ChefZ_KnowledgeState stand, ChefZ_CompiledRecipe rezept, out int outGesamt, out int outBekannt)
    {
        outGesamt  = 0;
        outBekannt = 0;

        if (!stand || !rezept || !rezept.slots)
            return;

        for (int i = 0; i < rezept.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = rezept.slots.Get(i);
            if (!slot)
                continue;
            if (slot.optional)
                continue;

            outGesamt++;
            if (SlotIsKnown(stand, slot))
                outBekannt++;
        }
    }

    /**
     * Passt IRGENDEINE bekannte Zutat auf diesen Slot?
     *
     * Gefragt wird der kompilierte Selektor selbst, nicht eine Nachbildung
     * seiner Regeln. Damit gilt fuer das Buch genau das, was beim Kochen auch
     * gilt - inklusive Kategorien, Vorfahren, Tags und Fluessigkeiten. Eine
     * zweite Implementierung derselben Frage waere der sicherste Weg zu einem
     * Buch, das etwas anderes behauptet als der Topf tut.
     */
    private bool SlotIsKnown(ChefZ_KnowledgeState stand, ChefZ_CompiledSlot slot)
    {
        if (!slot.selector)
            return false;

        ChefZ_IngredientManager zutaten = ChefZ_IngredientManager.Get();
        if (!zutaten || !zutaten.IsReady())
            return false;

        int n = stand.IngredientCount();
        for (int i = 0; i < n; i++)
        {
            ChefZ_Sym klasse = stand.IngredientAt(i);
            if (!FillProbe(zutaten, klasse))
                continue;
            if (slot.selector.Test(m_Probe))
                return true;
        }
        return false;
    }

    /**
     * Baut die Faktenzeile einer bekannten Zutat, so wohlwollend wie zulaessig.
     *
     * Wohlwollend heisst: volle Gesundheit, volle Frische, volle Menge. Das
     * Buch beantwortet die Frage "kenne ich eine Zutat, die hier passen KANN" -
     * nicht "habe ich gerade eine passende dabei". Ein Selektor, der frische
     * Zutaten verlangt, darf ein Rezept nicht deshalb verbergen, weil das
     * letzte Exemplar im Inventar verdorben war.
     *
     * @return false, wenn die Klasse dem Core unbekannt ist - etwa weil ihr
     *         Modul entfernt wurde. Dann zaehlt sie einfach nicht mit.
     */
    private bool FillProbe(ChefZ_IngredientManager zutaten, ChefZ_Sym klasse)
    {
        if (!ChefZ_SymbolTable.IsValid(klasse))
            return false;

        ChefZ_IngredientInfo info = zutaten.Resolve(klasse);
        if (!info)
            return false;

        m_Probe.Reset();
        m_Probe.handle      = 0;
        m_Probe.classSym    = klasse;
        m_Probe.freshness01 = 1.0;
        m_Probe.health01    = 1.0;
        m_Probe.quantity    = 1.0;
        m_Probe.quantityMax = 1.0;
        m_Probe.units       = info.unitsPerWholeItem;
        m_Probe.quantityUnit = info.quantityUnit;
        m_Probe.chefzState  = info.defaultState;

        if (info.closure)
            m_Probe.closure.CopyFrom(info.closure);

        if (info.staticTags)
        {
            for (int i = 0; i < info.staticTags.Count(); i++)
                m_Probe.tags.Insert(info.staticTags.Get(i));
        }
        return true;
    }

    //==========================================================================
    // Fortschritt ueber alle Rezepte - fuer die Kopfzeile des Buches
    //==========================================================================

    void CountProgress(ChefZ_KnowledgeState stand, out int outGelistet, out int outGesamt, out int outGemeistert)
    {
        outGelistet   = 0;
        outGesamt     = 0;
        outGemeistert = 0;

        ChefZ_RecipeEngine engine = ChefZ_RecipeEngine.Get();
        if (!engine || !engine.IsReady() || !stand)
            return;

        outGesamt = engine.GetRecipeCount();
        for (int i = 0; i < outGesamt; i++)
        {
            ChefZ_CompiledRecipe rezept = engine.GetRecipeAt(i);
            if (!rezept)
                continue;
            int status = DeriveStatus(stand, rezept);
            if (ChefZ_RecipeStatus.IsListed(status))
                outGelistet++;
            if (status == ChefZ_RecipeStatus.MASTERED)
                outGemeistert++;
        }
    }
}
