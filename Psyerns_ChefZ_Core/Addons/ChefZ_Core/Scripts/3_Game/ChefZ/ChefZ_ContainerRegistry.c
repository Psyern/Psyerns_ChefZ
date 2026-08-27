//==============================================================================
// ChefZ_ContainerRegistry - Kategorie <-> Behaelterklasse, einmal beim Boot
//
// Entwurf: 16 §3.2 (Schnittstelle woertlich), 16 §4 ("AUTO"), 16 §5
// (Datenfluss), 16 §6 (Zustandstabelle), 16 §7 (Fehlerverhalten Zeile fuer
// Zeile), 16 E2 (Kategorien statt Klassenlisten), 16 E5 (searchScope),
// 16 E7 (spoilageModifier), 02 E6 (kein CfgVehicles-Vollscan).
//
// ---------------------------------------------------------------------------
// Was diese Klasse ist - und was sie ausdruecklich NICHT ist
// ---------------------------------------------------------------------------
// Sie ist ein Nachschlagewerk auf SYMBOLEN. Sie kennt kein Inventar, keine
// Haende, keinen Boden und kein einziges Item; sie liegt deshalb in 3_Game
// und nicht in 4_World. Wer tatsaechlich sucht, verbraucht und zurueckgibt,
// heisst ChefZ_ContainerService und lebt eine Schicht weiter aussen.
//
// Die Trennung ist dieselbe wie zwischen ChefZ_PortionManager (rechnet) und
// ChefZ_PortionedFood_Base (handelt), und sie hat denselben Zweck: die
// Auswahlregel ist damit ohne Welt pruefbar. ChooseContainer() bekommt eine
// FERTIGE Liste und entscheidet - ein Selbsttest kann diese Liste hinlegen.
//
// ---------------------------------------------------------------------------
// Beide Richtungen, zwei Indizes, eine Wahrheit
// ---------------------------------------------------------------------------
//   m_ByClass            Klassensymbol  -> ChefZ_ContainerDef
//   m_ClassesByCategory  Kategoriesymbol -> Liste von Klassensymbolen
//
// Anders als in der ChefZ_ToolRegistry werden hier BEIDE Richtungen gefuehrt,
// und das ist kein Widerspruch zu deren Begruendung: dort ist der
// Rueckwaertsindex eine zweite Wahrheit, die gepflegt werden muesste, weil
// Gruppen auch ueber Vererbung entstehen. Hier gibt es keine Vererbung
// (siehe naechster Abschnitt) - beide Indizes entstehen in EINER Schleife aus
// derselben Quelle und koennen nicht auseinanderlaufen.
//
// Gebraucht werden beide Richtungen wirklich:
//   Klasse -> Def          "was habe ich da in der Hand, und was gibt es
//                           zurueck" (ResolveEmptyClass, 16 §4)
//   Kategorie -> Klassen   "welche Klassen kaeme ueberhaupt in Frage"
//                           (ChefZ_ContainerService, bevor er sucht)
//
// ---------------------------------------------------------------------------
// KEINE Vererbung entlang der CfgVehicles-Kette - mit Absicht
// ---------------------------------------------------------------------------
// Ein Behaelter ist eine ausdrueckliche Deklaration, kein Erbe. Wuerde die
// Registry Ableitungen einsammeln, wuerde aus einem als BOWL deklarierten
// Napf irgendwann jede Variante eines fremden Mods ein BOWL - und der Spieler
// verloere Behaelter, von denen der Autor nie gehoert hat. 16 E2 verlangt das
// Gegenteil: ein neuer Behaelter TRAEGT SICH EIN, und genau das ist der eine
// Handgriff, den er kostet.
//
// KEIN CONTENT: kein Behaelter, keine Kategorie, kein Gericht.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_ContainerRegistry
{
    private static ref ChefZ_ContainerRegistry s_Instance;

    //! Klassensymbol -> Deklaration. OHNE ref auf den Record: Eigentuemer ist
    //! die ChefZ_Registry<ChefZ_ContainerDef> im Config Manager, und die lebt
    //! laenger als diese Registry. Ein zweites ref waere ein Zyklus in
    //! Wartestellung (dieselbe Ueberlegung wie bei ChefZ_MatchResult.recipe).
    private ref map<int, ChefZ_ContainerDef> m_ByClass;

    //! Kategoriesymbol -> Klassensymbole, in Ladereihenfolge.
    private ref map<int, ref array<ChefZ_Sym>> m_ClassesByCategory;

    //! Alle bekannten Kategorien, in Ladereihenfolge. Die Reihenfolge ist
    //! nach ID sortiert (03 §4) und damit auf jedem Server dieselbe.
    private ref array<ChefZ_Sym> m_Categories;

    //! Alle bekannten Klassen, in Ladereihenfolge. Nur fuer die Diagnose.
    private ref array<ChefZ_Sym> m_Classes;

    private bool m_Ready;
    private bool m_QuietForTest;
    private int  m_RejectedCount;
    private int  m_MissingEmptyClass;
    private int  m_ForeignClasses;

    //--------------------------------------------------------------------------

    void ChefZ_ContainerRegistry()
    {
        m_ByClass           = new map<int, ChefZ_ContainerDef>();
        m_ClassesByCategory = new map<int, ref array<ChefZ_Sym>>();
        m_Categories        = new array<ChefZ_Sym>();
        m_Classes           = new array<ChefZ_Sym>();
        ResetState();
    }

    static ChefZ_ContainerRegistry Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_ContainerRegistry();
        return s_Instance;
    }

    private void ResetState()
    {
        m_ByClass.Clear();
        m_ClassesByCategory.Clear();
        m_Categories.Clear();
        m_Classes.Clear();
        m_Ready             = false;
        m_RejectedCount     = 0;
        m_MissingEmptyClass = 0;
        m_ForeignClasses    = 0;
    }

    //==========================================================================
    // BUILD (16 §7)
    //==========================================================================

    /**
     * Baut beide Indizes aus den Behaelterrecords.
     *
     * defs darf null sein. Ein Aufruf mit null ist die ausdrueckliche Art zu
     * sagen "Bestand leeren" - genau das braucht der SAFE_MODE (02 §8).
     * Danach ist die Registry "bereit und leer": jede Abfrage antwortet ruhig
     * false, statt einen Fehler ueber einen fehlenden Aufbau zu melden, den es
     * nie geben wird.
     *
     * 16 §7, erste Zeile: "CfgChefZContainers fehlt vollstaendig -> Registry
     * leer. INFO beim Boot - in einem Core ohne Content ist das der
     * Normalzustand." Deshalb INFO und nicht WARN: ein Core ohne Content hat
     * keine Behaelter, und das ist richtig so.
     */
    void Build(ChefZ_Registry<ChefZ_ContainerDef> defs, ChefZ_LoadReport report)
    {
        ResetState();
        m_Ready = true;

        if (!defs || defs.Count() == 0)
        {
            if (report)
                report.AddInfo("Behaelter: keine deklariert (CfgChefZContainers fehlt oder "
                    + "ist leer). Rezepte ohne Behaelteranforderung laufen unveraendert; "
                    + "in einem Core ohne Content ist das der Normalzustand (16 §7).");
            return;
        }

        // Ueber Keys(): die Reihenfolge ist nach ID sortiert (03 §4) und damit
        // auf jedem Server dieselbe. Fuer das Ergebnis ist sie ohne Bedeutung,
        // fuer die Vergleichbarkeit zweier Ladeberichte ist sie alles.
        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_ContainerDef def = defs.Find(keys.Get(i));
            if (def)
                AddDef(def, report);
        }

        if (report)
        {
            report.AddInfo("Behaelter: " + m_Classes.Count().ToString() + " Klassen in "
                + m_Categories.Count().ToString() + " Kategorien"
                + ", " + m_RejectedCount.ToString() + " verworfen"
                + ", " + m_MissingEmptyClass.ToString() + " ohne auffindbare Leerklasse"
                + ", " + m_ForeignClasses.ToString() + " Nicht-ChefZ-Klassen.");
        }

        LogIfDebug();
    }

    /**
     * Ein Record in beide Indizes.
     *
     * Die KLASSE wird interniert und NICHT gegen CfgVehicles geprueft - 05 E3
     * gilt hier genauso wie beim Faktensammler: ein Behaelter darf aus einem
     * OPTIONALEN Modul stammen, das auf diesem Server nicht geladen ist. Dann
     * ist der Eintrag wirkungslos, und wirkungslos ist genau richtig: es wird
     * nie ein Item dieser Klasse geben, also nie eines gefunden.
     *
     * Die LEERKLASSE wird sehr wohl geprueft (16 §7: "emptyClass existiert
     * nicht in CfgVehicles -> Rueckgabe entfaellt, WARN BEIM LADEN, nicht erst
     * beim Essen"). Der Unterschied ist beabsichtigt: eine fehlende
     * Behaelterklasse faellt von selbst auf (die Aktion erscheint nie), eine
     * fehlende Leerklasse dagegen erst Stunden spaeter beim letzten Bissen -
     * und dann sieht es aus, als habe der Server den Teller gefressen.
     */
    private void AddDef(notnull ChefZ_ContainerDef def, ChefZ_LoadReport report)
    {
        ChefZ_Sym classSym = ChefZ_SymbolTable.Intern(def.id);
        if (!ChefZ_SymbolTable.IsValid(classSym))
        {
            m_RejectedCount++;
            return;
        }

        if (m_ByClass.Contains(classSym))
        {
            // Kann nur passieren, wenn zwei Records mit derselben ID durch die
            // Registry gekommen sind - die weist Dubletten bereits ab. Der
            // Zweig bleibt trotzdem stehen: er kostet einen Map-Zugriff und
            // verhindert, dass ein spaeterer Umbau still den ersten Eintrag
            // ueberschreibt.
            m_RejectedCount++;
            Note(report, def, ChefZ_LogLevel.WARN, "container.dup." + def.id,
                "Behaelter \"" + def.id + "\" ist doppelt deklariert. Der zweite Eintrag "
                + "wird verworfen; der erste bleibt gueltig.");
            return;
        }

        // 16 §7: "spoilageModifier <= 0 -> auf 0.01 geklemmt, WARN."
        if (def.spoilageModifier <= 0.0)
        {
            // Ueber lokale Zwischenvariablen: Methodenaufrufe auf statischen
            // Konstanten sind in Enforce nicht zugesichert (dieselbe Vorsicht
            // wie in ChefZ_ManagerSymbolResolver.AdoptQualityTiers).
            float was = def.spoilageModifier;
            float min = ChefZ_ContainerDef.MIN_SPOILAGE;

            Note(report, def, ChefZ_LogLevel.WARN, "container.spoilage." + def.id,
                "Behaelter \"" + def.id + "\" hat spoilageModifier " + was.ToString()
                + ". Ein Faktor <= 0 waere kein Faktor, sondern ein Totalstopp des "
                + "Verfalls - er wird auf " + min.ToString() + " geklemmt. Wer den "
                + "Verfall wirklich anhalten will, benutzt einen Preservation-Record "
                + "mit stopsDecay (14 E7).");
            def.spoilageModifier = min;
        }

        // 16 E5: unbekannte Bits werden ausmaskiert und gemeldet. Ein Bit, das
        // dieser Core nicht kennt, koennte in einer spaeteren Fassung eine
        // Suchstufe bezeichnen - stillschweigend nicht zu suchen ist der
        // Fehlerfall, den niemand findet.
        int unknownBits = ChefZ_ContainerScope.UnknownBits(def.searchScope);
        if (unknownBits != 0)
        {
            Note(report, def, ChefZ_LogLevel.WARN, "container.scope." + def.id,
                "Behaelter \"" + def.id + "\" hat searchScope " + def.searchScope.ToString()
                + " mit unbekannten Bits (" + unknownBits.ToString() + "). Sie werden "
                + "ignoriert. Gueltig: " + ChefZ_ContainerScope.ValidNames() + ".");
            def.searchScope = ChefZ_ContainerScope.Sanitize(def.searchScope);
        }

        if (def.searchScope == ChefZ_ContainerScope.NONE)
        {
            Note(report, def, ChefZ_LogLevel.WARN, "container.scope.none." + def.id,
                "Behaelter \"" + def.id + "\" hat searchScope 0 - er wuerde nirgends "
                + "gesucht und damit nie gefunden. Er bleibt eingetragen (die Rueckgabe "
                + "funktioniert weiterhin), ist als Suchziel aber wirkungslos.");
        }

        // 16 §7: fehlende Leerklasse -> Rueckgabe entfaellt, WARN BEIM LADEN.
        if (def.reusable && def.consumedOnServe && def.emptyClass != ""
            && !ClassExists(def.emptyClass))
        {
            m_MissingEmptyClass++;
            Note(report, def, ChefZ_LogLevel.WARN, "container.empty." + def.emptyClass,
                "Behaelter \"" + def.id + "\" nennt als emptyClass \"" + def.emptyClass
                + "\", und diese Klasse gibt es auf diesem Server nicht. Beim "
                + "vollstaendigen Verzehr kommt deshalb nichts zurueck. Das Gericht "
                + "bleibt essbar - es fehlt nur der Teller danach.");
        }

        // 16 §7: "Vanilla-Item als Behaelterklasse - zulaessig und gelegentlich
        // gewollt. Der Validator meldet es in Gate 1 zur Kenntnis, damit
        // niemand versehentlich Vanilla-Kochgeschirr verheizt." Zur Laufzeit
        // zaehlen wir sie nur; die Zahl steht im Ladebericht.
        if (!IsChefZClass(def.id))
            m_ForeignClasses++;

        m_ByClass.Set(classSym, def);
        m_Classes.Insert(classSym);

        if (!def.DeclaresCategories())
        {
            // Ausdrueckliche leere Liste (siehe ChefZ_ContainerDef.Validate).
            // Kein Fehler, aber eine Sackgasse: der Behaelter ist eingetragen
            // und wird von keinem Rezept je gefordert.
            Note(report, def, ChefZ_LogLevel.INFO, "container.nocat." + def.id,
                "Behaelter \"" + def.id + "\" nennt eine ausdruecklich LEERE "
                + "Kategorieliste. Er ist eingetragen, kann aber von keinem Rezept "
                + "gefordert werden.");
            return;
        }

        for (int c = 0; c < def.containerCategories.Count(); c++)
        {
            string name = def.containerCategories.Get(c);
            if (name == "")
                continue;
            Link(ChefZ_SymbolTable.Intern(name), classSym);
        }
    }

    private void Link(ChefZ_Sym category, ChefZ_Sym classSym)
    {
        if (!ChefZ_SymbolTable.IsValid(category) || !ChefZ_SymbolTable.IsValid(classSym))
            return;

        array<ChefZ_Sym> classes;
        if (!m_ClassesByCategory.Find(category, classes))
        {
            classes = new array<ChefZ_Sym>();
            m_ClassesByCategory.Set(category, classes);
            m_Categories.Insert(category);
        }
        if (classes.Find(classSym) < 0)
            classes.Insert(classSym);
    }

    /**
     * Gibt es diese Klasse ueberhaupt?
     *
     * protected und eine eigene Methode, aus demselben Grund wie
     * ChefZ_ToolRegistry.ResolveConfigParent: sie ist der EINZIGE
     * Config-Zugriff dieser Klasse, und der Selbsttest ersetzt sie durch eine
     * Tabelle. Sonst waere die Zusage aus 16 §7 ("emptyClass existiert nicht
     * -> WARN beim Laden") nur auf einem laufenden Server mit echtem Content
     * pruefbar - und damit praktisch gar nicht.
     */
    protected bool ClassExists(string cls)
    {
        if (!g_Game)
            return true;        // ohne Engine keine Aussage - nicht warnen
        if (g_Game.ConfigIsExisting("CfgVehicles "  + cls)) return true;
        if (g_Game.ConfigIsExisting("CfgWeapons "   + cls)) return true;
        if (g_Game.ConfigIsExisting("CfgMagazines " + cls)) return true;
        return false;
    }

    private bool IsChefZClass(string cls)
    {
        return cls.IndexOf("ChefZ_") == 0;
    }

    //==========================================================================
    // Auskuenfte (16 §3.2)
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    //! Steht ueberhaupt ein Behaeltersystem zur Verfuegung?
    //!
    //! "Bereit und leer" ist NICHT dasselbe wie "vorhanden": eine Registry
    //! ohne eine einzige Kategorie kann keine Behaelterbedingung erfuellen,
    //! und dann gilt 15 §7 - die Bedingung entfaellt, statt jede Entnahme zu
    //! blockieren. Der ChefZ_ConfigManager legt den Schalter des
    //! ChefZ_PortionManager genau anhand dieser Antwort um.
    bool HasAnyContainer()
    {
        return m_Ready && m_Categories.Count() > 0;
    }

    int GetCategoryCount() { return m_Categories.Count(); }
    int GetClassCount()    { return m_Classes.Count(); }

    bool CategoryExists(ChefZ_Sym category)
    {
        if (!ChefZ_SymbolTable.IsValid(category))
            return false;
        return m_ClassesByCategory.Contains(category);
    }

    bool IsContainerOfCategory(ChefZ_Sym classSym, ChefZ_Sym category)
    {
        array<ChefZ_Sym> classes;
        if (!ChefZ_SymbolTable.IsValid(category))
            return false;
        if (!m_ClassesByCategory.Find(category, classes))
            return false;
        return classes.Find(classSym) >= 0;
    }

    //! Ist diese Klasse ueberhaupt ein deklarierter Behaelter? Der billige
    //! Vorfilter fuer die Suche in 4_World.
    bool IsContainerClass(ChefZ_Sym classSym)
    {
        if (!ChefZ_SymbolTable.IsValid(classSym))
            return false;
        return m_ByClass.Contains(classSym);
    }

    bool GetDef(ChefZ_Sym classSym, out ChefZ_ContainerDef def)
    {
        def = null;
        if (!ChefZ_SymbolTable.IsValid(classSym))
            return false;

        ChefZ_ContainerDef found;
        if (!m_ByClass.Find(classSym, found))
            return false;

        def = found;
        return true;
    }

    /**
     * Alle Klassen einer Kategorie.
     *
     * outClasses wird GELEERT und gefuellt, nie null. Die Reihenfolge ist die
     * Eintragungsreihenfolge und damit stabil - der Aufrufer soll sich darauf
     * verlassen koennen.
     */
    void GetClassesForCategory(ChefZ_Sym category, out array<ChefZ_Sym> outClasses)
    {
        if (!outClasses)
            outClasses = new array<ChefZ_Sym>();
        outClasses.Clear();

        array<ChefZ_Sym> classes;
        if (!ChefZ_SymbolTable.IsValid(category))
            return;
        if (!m_ClassesByCategory.Find(category, classes))
            return;

        for (int i = 0; i < classes.Count(); i++)
            outClasses.Insert(classes.Get(i));
    }

    /**
     * Kommt bei diesem Behaelter ueberhaupt etwas zurueck?
     *
     * BEIDE Schalter, und zwar zusammen:
     *
     *   reusable = 0        der Konservenfall (16 §7). Es kommt nichts
     *                       zurueck, und das ist kein Fehler.
     *   consumedOnServe = 0 der Behaelter ist gar nicht verbraucht worden - er
     *                       liegt noch beim Spieler. Etwas zurueckzugeben
     *                       hiesse, ihn zu verdoppeln.
     *
     * Diese Kopplung existiert genau hier. Wer sie an zwei Stellen schriebe,
     * haette irgendwann eine Stelle, an der sie fehlt - und einen
     * Duplikationsexploit.
     */
    bool ReturnsEmpty(ChefZ_Sym containerClass)
    {
        ChefZ_ContainerDef def;
        if (!GetDef(containerClass, def))
            return false;
        return def.reusable && def.consumedOnServe;
    }

    /**
     * "AUTO" (16 §4): der Leerbehaelter GENAU DES Behaelters, der tatsaechlich
     * benutzt wurde.
     *
     * Wer eine Emailleschuessel hineingab, bekommt eine Emailleschuessel
     * zurueck - und ein neuer Behaeltertyp aus einem spaeteren Modul braucht
     * KEIN Rezeptupdate. Genau das ist der Grund, warum die Aufloesung hier
     * steht und nicht im Rezept.
     *
     * INVALID heisst "nichts zurueckgeben" und ist eine normale Antwort:
     * unbekannter Behaelter, reusable = 0, consumedOnServe = 0 oder eine
     * emptyClass, die es auf diesem Server nicht gibt.
     */
    ChefZ_Sym ResolveEmptyClass(ChefZ_Sym usedContainerClass)
    {
        ChefZ_ContainerDef def;
        if (!GetDef(usedContainerClass, def))
            return ChefZ_SymbolTable.INVALID;

        if (!def.reusable || !def.consumedOnServe)
            return ChefZ_SymbolTable.INVALID;

        if (def.emptyClass == "")
            return ChefZ_SymbolTable.INVALID;

        // Die Existenz wurde beim Build geprueft und gemeldet (16 §7). Hier
        // wird sie ERNEUT geprueft, weil der Aufruf auf dem heissen Pfad des
        // Verzehrs liegt und ein nicht erzeugbarer Behaelter dort nur eine
        // weitere Fehlermeldung ergaebe. Der Zweig kostet einen
        // Config-Zugriff je vollstaendig verzehrtem Gericht - das ist
        // ausserhalb jeder Messbarkeit.
        if (!ClassExists(def.emptyClass))
            return ChefZ_SymbolTable.INVALID;

        return ChefZ_SymbolTable.Intern(def.emptyClass);
    }

    /**
     * Die Aufloesung des Feldes returnContainer eines Ergebnisses (16 §4).
     *
     *   ""       nichts zurueckgeben
     *   "AUTO"   der Leerbehaelter des TATSAECHLICH benutzten Behaelters
     *   sonst    genau diese Klasse
     *
     * Der dritte Fall wird NICHT gegen CfgVehicles geprueft: er kann eine
     * Klasse aus einem optionalen Modul nennen. Scheitert die Erzeugung,
     * meldet es ChefZ_ContainerService.ReturnEmpty() mit vollem Zusammenhang.
     */
    ChefZ_Sym ResolveReturnClass(string returnContainerSpec, ChefZ_Sym usedContainerClass)
    {
        if (returnContainerSpec == "")
            return ChefZ_SymbolTable.INVALID;

        if (returnContainerSpec == ChefZ_ContainerDef.AUTO)
            return ResolveEmptyClass(usedContainerClass);

        return ChefZ_SymbolTable.Intern(returnContainerSpec);
    }

    //! Der Haltbarkeitsfaktor des Behaelters (16 E7, wirkt in 14).
    //! 1.0 fuer alles Unbekannte - "nichts bekannt" darf die Haltbarkeit
    //! niemals veraendern.
    float GetSpoilageModifier(ChefZ_Sym containerClass)
    {
        ChefZ_ContainerDef def;
        if (!GetDef(containerClass, def))
            return ChefZ_ContainerDef.DEFAULT_SPOILAGE;
        return def.spoilageModifier;
    }

    /**
     * Wo wird fuer diese KATEGORIE gesucht (16 §3.2)?
     *
     * Das Bitfeld steht laut 16 §3.1 an der KLASSE, die Frage stellt sich aber
     * an der Kategorie: der Sucher weiss vorher nicht, welche Klasse er finden
     * wird. Aufgeloest wird das als ODER ueber alle Mitglieder - eine Stufe
     * wird betreten, sobald irgendein Mitglied dort gefunden werden will.
     *
     * Die Gegenprobe je Fund macht GetSearchScopeForClass(): ein Behaelter,
     * der nur in der Hand zaehlen soll, wird in der Kiste zwar angefasst, aber
     * nicht genommen. Ohne diese zweite Frage waere das Bitfeld an der Klasse
     * wirkungslos, sobald ein einziges Mitglied grosszuegiger ist.
     */
    int GetSearchScope(ChefZ_Sym category)
    {
        array<ChefZ_Sym> classes;
        if (!ChefZ_SymbolTable.IsValid(category))
            return ChefZ_ContainerScope.NONE;
        if (!m_ClassesByCategory.Find(category, classes))
            return ChefZ_ContainerScope.NONE;

        int scope = ChefZ_ContainerScope.NONE;
        for (int i = 0; i < classes.Count(); i++)
        {
            ChefZ_ContainerDef def;
            if (GetDef(classes.Get(i), def))
                scope = scope | def.searchScope;
        }
        return scope;
    }

    int GetSearchScopeForClass(ChefZ_Sym classSym)
    {
        ChefZ_ContainerDef def;
        if (!GetDef(classSym, def))
            return ChefZ_ContainerScope.NONE;
        return def.searchScope;
    }

    /**
     * Reine Auswahl auf Symbolebene (16 §3.2, woertlich: "KEIN
     * Inventarzugriff").
     *
     * availableClasses ist bereits GEORDNET: der Sucher in 4_World hat nach
     * Fundstufe (Haende -> Inventar -> Umgebung), dann nach Gesundheit, dann
     * nach Klassenname sortiert (16 E5). Diese Methode nimmt deshalb den
     * ERSTEN Eintrag, der der Kategorie angehoert, und wendet kein zweites
     * Kriterium an - zwei Reihenfolgen ergaeben zwei Antworten, und der
     * Spieler saehe, dass die Aktion etwas anderes nimmt als angekuendigt.
     *
     * @return false, wenn nichts passt. chosen ist dann INVALID.
     */
    bool ChooseContainer(ChefZ_Sym category, notnull array<ChefZ_Sym> availableClasses,
                         out ChefZ_Sym chosen)
    {
        chosen = ChefZ_SymbolTable.INVALID;

        if (!CategoryExists(category))
            return false;

        for (int i = 0; i < availableClasses.Count(); i++)
        {
            ChefZ_Sym cls = availableClasses.Get(i);
            if (!IsContainerOfCategory(cls, category))
                continue;
            chosen = cls;
            return true;
        }
        return false;
    }

    //==========================================================================
    // Startaudit (16 §7, Zeile 2)
    //==========================================================================

    /**
     * "Rezept nennt unbekannte Behaelterkategorie -> WARN EINMAL BEIM LADEN
     * mit Rezept-ID."
     *
     * Zwingend hier und nicht beim ersten Entnahmeversuch: ein Rezept, dessen
     * Kategorie es nicht gibt, matcht NIE - und "matcht nie" sieht auf einem
     * laufenden Server exakt aus wie fehlender Content. Der Befund muss im
     * Startlog stehen, wo ihn jemand liest.
     *
     * Bewusst KEIN Rezeptabbruch (16 §7): die Kategorie koennte aus einem
     * optionalen Modul kommen, das dieser Server nicht geladen hat. Das Rezept
     * bleibt eingetragen und wird schlicht nicht ausloesbar.
     *
     * Er aendert NICHTS - kein Rezept, keine Registry, keinen Balancingwert.
     */
    void AuditPortionSpecs(ChefZ_PortionManager portions, ChefZ_LoadReport report)
    {
        if (!m_Ready || !portions || !portions.IsReady())
            return;

        int withContainer = 0;
        int unknown       = 0;

        int count = portions.GetSpecCount();
        for (int i = 0; i < count; i++)
        {
            ChefZ_PortionSpec spec = portions.GetSpecAt(i);
            if (!spec || !spec.RequiresContainer())
                continue;

            withContainer++;

            if (CategoryExists(spec.containerCategorySym))
                continue;

            unknown++;
            if (report)
            {
                report.AddWarn(spec.sourceRef, spec.bulkClass,
                    "verlangt fuer die Entnahme einen Behaelter der Kategorie \""
                    + spec.containerCategory + "\", die kein einziger deklarierter "
                    + "Behaelter fuehrt. Das Gericht ist damit nicht portionierbar "
                    + "(16 §7). Kein Abbruch: die Kategorie kann aus einem optionalen "
                    + "Modul stammen, das auf diesem Server fehlt.");
            }
        }

        if (report && withContainer > 0)
        {
            report.AddInfo("Behaelteraudit: " + withContainer.ToString()
                + " Portionsgerichte mit Behaelteranforderung, " + unknown.ToString()
                + " davon mit unbekannter Kategorie.");
        }
    }

    //==========================================================================
    // Diagnose (18)
    //==========================================================================

    void DumpContainers(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("ChefZ Behaelter  bereit=" + m_Ready.ToString()
            + "  klassen=" + m_Classes.Count().ToString()
            + "  kategorien=" + m_Categories.Count().ToString());

        for (int i = 0; i < m_Categories.Count(); i++)
        {
            ChefZ_Sym category = m_Categories.Get(i);
            array<ChefZ_Sym> classes;
            if (!m_ClassesByCategory.Find(category, classes))
                continue;

            outLines.Insert("  " + ChefZ_SymbolTable.NameOrMark(category)
                + " [" + ChefZ_ContainerScope.Name(GetSearchScope(category)) + "]: "
                + ChefZ_TextList.JoinSymbols(classes, ", "));
        }

        for (int k = 0; k < m_Classes.Count(); k++)
        {
            ChefZ_ContainerDef def;
            if (GetDef(m_Classes.Get(k), def))
                outLines.Insert("    " + def.ToDebugString());
        }
    }

    private void LogIfDebug()
    {
        if (m_QuietForTest)
            return;
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.CONTAIN, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        DumpContainers(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.CONTAIN, lines);
    }

    //! Eine Meldung geht in den LADEBERICHT, wenn es einen gibt, sonst ins
    //! Log - und dort genau einmal. Ohne den Ladebericht (Selbsttest,
    //! nachtraeglicher Build) waere sie sonst gar nicht sichtbar.
    private void Note(ChefZ_LoadReport report, notnull ChefZ_ContainerDef def,
                      int level, string onceKey, string message)
    {
        if (m_QuietForTest)
            return;

        if (report)
        {
            if (level == ChefZ_LogLevel.WARN || level == ChefZ_LogLevel.ERR)
                report.AddWarn(def.sourceRef, def.id, message);
            else
                report.AddInfo(message);
            return;
        }

        ChefZ_Log.Once(level, ChefZ_LogChannel.CONTAIN, onceKey, message);
    }

    //! Nur fuer den Selbsttest: er baut echte Registries und soll dabei kein
    //! Startlog erzeugen.
    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }
}
