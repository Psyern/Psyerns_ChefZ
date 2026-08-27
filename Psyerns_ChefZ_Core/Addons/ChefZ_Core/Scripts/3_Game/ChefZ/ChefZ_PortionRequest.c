//==============================================================================
// ChefZ_PortionRequest / ChefZ_PortionPlan - die Frage und die Antwort einer
//                                            Entnahme
//
// Entwurf: 15 §3 (beide Klassen woertlich), 15 §4 (ENTNAHME), 15 §7
// (Fehlerverhalten), 15 E4 (Portionen erben unveraendert), 15 E6
// (serverseitige Revalidierung).
//
// ---------------------------------------------------------------------------
// Warum beides reine Daten sind
// ---------------------------------------------------------------------------
// Dieselbe Trennung wie zwischen ChefZ_CookContext und ChefZ_MatchResult: die
// FRAGE traegt kein ItemBase, die ANTWORT veraendert nichts. Der
// ChefZ_PortionManager rechnet damit ausschliesslich auf Symbolen und Zahlen -
// er ist in 3_Game, kennt kein Inventar und kann deshalb auch keines anfassen.
//
// Wer den Plan ausfuehrt, ist ChefZ_PortionedFood_Base.ChefZ_TakePortion() in
// 4_World, und der revalidiert vorher erneut (15 E6). Ein Plan ist eine
// Ansage, keine Handlung.
//
// KEIN CONTENT: kein Gericht, keine Schuessel, keine Kategorie.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_PortionRequest
{
    //! Die Klasse des Bulk-Gerichts, aus dem entnommen werden soll.
    ChefZ_Sym sourceClass;

    //! Wie viele Portionen der Zaehler des Items JETZT sagt.
    int       portionsLeft;

    ChefZ_Sym sourceState;
    ChefZ_Sym sourceQuality;
    float     sourceFreshness;

    /**
     * Behaelterklassen, die dem Handelnden zur Verfuegung stehen (16 §3).
     *
     * Der Aufrufer aus 4_World fuellt sie; der Manager sucht NICHT selbst -
     * er hat kein Inventar. Nie null, haeufig leer: leer heisst "keiner
     * gefunden", und das ist bei einer Spec ohne containerCategory die
     * richtige und folgenlose Antwort.
     */
    ref array<ChefZ_Sym> availableContainerClasses;

    //! 0 = niemand. Wird fuer Ereignisse und Faehigkeiten gebraucht (17),
    //! nie fuer die Entscheidung: dieselbe Portion muss fuer jeden Spieler
    //! dieselbe sein.
    int       actorIdentityId;

    void ChefZ_PortionRequest()
    {
        availableContainerClasses = new array<ChefZ_Sym>();
        Reset();
    }

    //! Listen werden GELEERT, nicht neu angelegt - dieselbe Ueberlegung wie
    //! im ChefZ_CookContext: eine Entnahme ist ein haeufiger Vorgang.
    void Reset()
    {
        sourceClass     = ChefZ_SymbolTable.INVALID;
        portionsLeft    = 0;
        sourceState     = ChefZ_SymbolTable.INVALID;
        sourceQuality   = ChefZ_SymbolTable.INVALID;
        sourceFreshness = -1.0;
        availableContainerClasses.Clear();
        actorIdentityId = 0;
    }

    void AddContainer(ChefZ_Sym cls)
    {
        if (!ChefZ_SymbolTable.IsValid(cls))
            return;
        if (availableContainerClasses.Find(cls) >= 0)
            return;
        availableContainerClasses.Insert(cls);
    }

    bool HasContainers()
    {
        return availableContainerClasses.Count() > 0;
    }

    string ToDebugString()
    {
        string s = ChefZ_SymbolTable.NameOrMark(sourceClass)
                 + "  portionen=" + portionsLeft.ToString();

        if (ChefZ_SymbolTable.IsValid(sourceQuality))
            s = s + " stufe=" + ChefZ_SymbolTable.Name(sourceQuality);
        if (ChefZ_SymbolTable.IsValid(sourceState))
            s = s + " zustand=" + ChefZ_SymbolTable.Name(sourceState);
        if (sourceFreshness >= 0.0)
            s = s + " frische=" + sourceFreshness.ToString();
        if (HasContainers())
            s = s + " behaelter=" + availableContainerClasses.Count().ToString();

        return s;
    }
}

//------------------------------------------------------------------------------

class ChefZ_PortionPlan
{
    //! Was entstehen soll. Leer heisst "kein Plan" - BuildPortionPlan gibt
    //! dann false zurueck und hat den Grund gesetzt.
    string    portionClass;

    //! Welcher Behaelter dafuer verbraucht wird. INVALID = keiner, und das ist
    //! der Normalfall (16 §7, "CfgChefZContainers fehlt vollstaendig").
    ChefZ_Sym containerToConsume;

    //! Was beim vollstaendigen Verzehr der Portion zurueckkommt (16 §4).
    //! INVALID = nichts. Wird am Portionsitem vermerkt, nicht hier ausgefuehrt.
    ChefZ_Sym returnContainerClass;

    //--- 15 E4: unveraendert uebernommen, nie neu bewertet --------------------
    ChefZ_Sym stateToApply;
    ChefZ_Sym qualityToApply;
    float     freshnessToApply;      // < 0 = nichts setzen

    //! Menge der Portion. < 0 = Klassendefault der Portionsklasse stehen
    //! lassen. Das ist der einzige Wert, der mit Sicherheit sinnvoll ist.
    float     quantityToApply;

    //--- Was aus der Quelle wird ---------------------------------------------
    bool      sourceBecomesEmpty;    // war das die LETZTE Portion?
    string    emptyClass;            // "" = Quelle loeschen (15 §2)

    //! Nur fuer Trace und Fehlermeldungen.
    int       portionsLeftAfter;

    void ChefZ_PortionPlan()
    {
        Reset();
    }

    void Reset()
    {
        portionClass         = "";
        containerToConsume   = ChefZ_SymbolTable.INVALID;
        returnContainerClass = ChefZ_SymbolTable.INVALID;
        stateToApply         = ChefZ_SymbolTable.INVALID;
        qualityToApply       = ChefZ_SymbolTable.INVALID;
        freshnessToApply     = -1.0;
        quantityToApply      = -1.0;
        sourceBecomesEmpty   = false;
        emptyClass           = "";
        portionsLeftAfter    = 0;
    }

    bool IsValid()
    {
        return portionClass != "";
    }

    bool NeedsContainer()
    {
        return ChefZ_SymbolTable.IsValid(containerToConsume);
    }

    //! true, wenn die Quelle nach dieser Entnahme ersetzt statt geloescht
    //! wird (15 §2, emptyOnLastPortion).
    bool SourceIsReplaced()
    {
        return sourceBecomesEmpty && emptyClass != "";
    }

    string ToDebugString()
    {
        string s = portionClass + "  rest=" + portionsLeftAfter.ToString();

        if (quantityToApply >= 0.0)
            s = s + " menge=" + quantityToApply.ToString();
        if (ChefZ_SymbolTable.IsValid(qualityToApply))
            s = s + " stufe=" + ChefZ_SymbolTable.Name(qualityToApply);
        if (ChefZ_SymbolTable.IsValid(stateToApply))
            s = s + " zustand=" + ChefZ_SymbolTable.Name(stateToApply);
        if (freshnessToApply >= 0.0)
            s = s + " frische=" + freshnessToApply.ToString();
        if (NeedsContainer())
            s = s + " verbraucht=" + ChefZ_SymbolTable.Name(containerToConsume);
        if (sourceBecomesEmpty)
        {
            if (emptyClass != "")
                s = s + "  QUELLE -> " + emptyClass;
            else
                s = s + "  QUELLE LOESCHEN";
        }

        return s;
    }
}
