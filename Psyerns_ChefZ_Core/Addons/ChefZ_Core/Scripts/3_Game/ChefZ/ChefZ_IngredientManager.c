//==============================================================================
// ChefZ_IngredientManager - Stammdaten einer Zutatenklasse, einmal aufgeloest
//
// Entwurf: 05 (vollstaendig), insbesondere §3.2 (Schnittstelle woertlich),
// §4 (Aufbau in fuenf Schritten), §6 (Mengen und Einheiten), §7
// (Fehlerverhalten, Zeile fuer Zeile), E2 (Vererbung entlang der
// CfgVehicles-Elternkette), E3 (nicht deklarierte Klassen bleiben
// adressierbar), E5 (keine Musterregeln in V1); ausserdem 02 E6 (KEIN
// CfgVehicles-Vollscan) und 07 E4 (selectivityHint aus den Rueckwaertsindizes).
//
// Der Manager beantwortet fuer eine Itemklasse genau eine Frage schnell:
// welche Kategorien, Tags, welcher Standardzustand, welche Mengeneinheit?
// Er kennt KEINE Zutat - er kennt nur das, was Content-Module ueber ihre
// Zutaten erklaert haben. In dieser Datei steht kein Klassenname, keine
// Kategorie und kein Tag.
//
// ---------------------------------------------------------------------------
// Die drei Entscheidungen, die den Aufbau bestimmen
// ---------------------------------------------------------------------------
//
// 1. VERERBUNG NUR FUER DEKLARIERTE KLASSEN (05 E2 + 02 E6)
//    Fuer jede deklarierte Zutat laeuft der Manager die CfgVehicles-Kette
//    hoch und sammelt die Vorfahren, die SELBST eine Deklaration haben. Es
//    gibt keinen Lauf ueber CfgVehicles - der waere ein ungemessener
//    Startkostenposten ueber ~10^4 Klassen fuer null Zusatznutzen.
//
// 2. EINMAL AUFLOESEN, DANN EINFRIEREN
//    Nach Build gibt es keinen Kettenlauf, keinen Stringvergleich und keinen
//    Defaultzweig mehr. Was zur Laufzeit gebraucht wird, steht als Symbol oder
//    als Bit im ChefZ_IngredientInfo.
//
// 3. JEDER DATENFEHLER MACHT DAS MATCHING ENGER, NIE FALSCHER (05 §7)
//    Eine unbekannte Kategorie faellt weg, die uebrigen bleiben. Eine
//    unbekannte Klasse im Topf ist kein Fehler, sondern der Normalfall.
//    Richtung Vanilla ist immer die sichere Richtung (Invariante I2).
//
// Layer: 3_Game. Der Manager liest Registries und die Game-Config, kennt aber
// keinen Engine-Entity-Typ - kein ItemBase, kein EntityAI. Wer Items anfasst,
// heisst ChefZ_FactCollector und liegt in 4_World.
//==============================================================================

class ChefZ_IngredientManager : Managed
{
    //! Reissleine gegen eine kaputte oder zyklische Config-Elternkette. Ein
    //! echter CfgVehicles-Baum ist keine 20 Stufen tief; 64 ist bequem
    //! darueber und trotzdem endlich.
    static const int MAX_PARENT_CHAIN = 64;

    static const string CFG_VEHICLES  = "CfgVehicles";
    static const string CFG_WEAPONS   = "CfgWeapons";
    static const string CFG_MAGAZINES = "CfgMagazines";

    private static ref ChefZ_IngredientManager s_Instance;

    //! Symbol der Standard-Mengeneinheit. Wird einmal interniert und danach
    //! nur noch gelesen - auch vom Collector fuer nicht deklarierte Klassen.
    private static ChefZ_Sym s_DefaultUnitSym;

    private ref array<ref ChefZ_IngredientInfo> m_Infos;
    private ref map<int, int>                   m_BySym;      // classSym -> Index

    //--- Rueckwaertsindizes (05 §4, Schritt 4; 07 E4) ------------------------
    private ref map<int, ref array<ChefZ_Sym>>  m_ByCategory;
    private ref map<int, ref array<ChefZ_Sym>>  m_ByTag;

    private bool m_Ready;
    private bool m_NotReadyLogged;
    private int  m_RejectedCount;
    private int  m_InheritedCount;

    //--- nur Selbsttest -------------------------------------------------------
    private bool m_QuietForTest;
    private ChefZ_CategoryManager m_CategoriesForTest;   // ohne ref: gehoert dem Test
    private bool m_SkipClassExistsCheck;

    //--------------------------------------------------------------------------

    void ChefZ_IngredientManager()
    {
        m_Infos      = new array<ref ChefZ_IngredientInfo>();
        m_BySym      = new map<int, int>();
        m_ByCategory = new map<int, ref array<ChefZ_Sym>>();
        m_ByTag      = new map<int, ref array<ChefZ_Sym>>();

        m_QuietForTest         = false;
        m_SkipClassExistsCheck = false;
        m_CategoriesForTest    = null;

        ResetState();
    }

    static ChefZ_IngredientManager Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_IngredientManager();
        return s_Instance;
    }

    /**
     * Symbol der Standard-Mengeneinheit (ChefZ_IngredientDef.DEFAULT_QUANTITY_UNIT).
     *
     * Statisch und unabhaengig vom Aufbau, weil der ChefZ_FactCollector sie
     * auch fuer NICHT deklarierte Klassen braucht - und die gibt es auch dann,
     * wenn der Manager nie gebaut wurde (05 E3).
     */
    static ChefZ_Sym DefaultUnitSym()
    {
        if (!ChefZ_SymbolTable.IsValid(s_DefaultUnitSym))
            s_DefaultUnitSym = ChefZ_SymbolTable.Intern(ChefZ_IngredientDef.DEFAULT_QUANTITY_UNIT);
        return s_DefaultUnitSym;
    }

    //==========================================================================
    // Aufbau (05 §4)
    //==========================================================================

    /**
     * Loest alle Zutatendeklarationen auf. Idempotent: ein zweiter Aufruf
     * verwirft den alten Bestand vollstaendig.
     *
     * @param defs    Zutatenregistry. null oder leer ist KEIN Fehler (05 §7).
     * @param report  darf null sein.
     * @param states  Zustandsregistry, nur zur Referenzpruefung von
     *                defaultState. Abweichung von der Signatur in 05 §3.2, und
     *                zwar bewusst: ohne sie bliebe ein Tippfehler in
     *                defaultState bis zur ersten misslungenen Kochprobe
     *                unbemerkt. Der Parameter ist optional, damit der
     *                Selbsttest und ein Aufrufer ohne Zustaende ihn weglassen
     *                koennen.
     *
     * VORBEDINGUNG: der ChefZ_CategoryManager ist gebaut. Ist er es nicht,
     * gilt jede Kategorie als unbekannt - die Zutaten bleiben geladen, haben
     * aber leere Closures. Der Config Manager haelt die Reihenfolge ein.
     */
    void Build(ChefZ_Registry<ChefZ_IngredientDef> defs, ChefZ_LoadReport report, ChefZ_Registry<ChefZ_StateDef> states = null)
    {
        int startTick = TickCount(0);
        ResetState();

        if (!defs || defs.Count() == 0)
        {
            // 05 §7, erste Zeile: keine Zutatendaten ist kein Fehler.
            // IsKnown() liefert dann immer false, jedes Item bekommt eine leere
            // Closure, Kategorierezepte matchen nie - und das Kochen bleibt
            // vollstaendig Vanilla.
            if (report)
                report.AddInfo("Keine Zutatenbindungen deklariert - der Ingredient Manager bleibt " + "leer. Kategorie- und Tag-Selektoren matchen dadurch nie; Rezepte mit " + "reinen Klassen-Selektoren sind unberuehrt.");
            m_Ready = true;
            return;
        }

        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_IngredientDef def = defs.Find(keys.Get(i));
            if (!def)
                continue;
            AdmitOne(def, defs, states, report);
        }

        BuildReverseIndices();

        m_Ready = true;

        if (report)
        {
            string chefzTxt1 = "Zutatenbindungen: " + GetKnownCount().ToString() + " Klassen, " + m_InheritedCount.ToString() + " davon mit geerbten Feldern, ";
            chefzTxt1 = chefzTxt1 + m_RejectedCount.ToString() + " abgewiesen, " + m_ByCategory.Count().ToString() + " Kategorien und " + m_ByTag.Count().ToString();
            chefzTxt1 = chefzTxt1 + " Tags rueckwaerts indiziert" + " in " + TickCount(startTick).ToString() + "ms.";
            report.AddInfo(chefzTxt1);
        }

        LogIfDebug();
    }

    //--------------------------------------------------------------------------

    /**
     * Loest EINE Deklaration auf: Vererbung, Symbole, Closure, Pruefungen.
     *
     * Reihenfolge ist nicht beliebig - erst erben, dann pruefen. Umgekehrt
     * wuerde eine abgeleitete Klasse fuer ein Feld gemeldet, das sie gar nicht
     * selbst nennt.
     */
    private void AdmitOne(notnull ChefZ_IngredientDef def, notnull ChefZ_Registry<ChefZ_IngredientDef> defs, ChefZ_Registry<ChefZ_StateDef> states, ChefZ_LoadReport report)
    {
        if (!ChefZ_SymbolTable.IsValid(def.sym))
        {
            Report(report, true, def, "Zutat hat kein gueltiges Symbol und ist nicht nachschlagbar. Das deutet auf " + "einen Record hin, der die COMPILE-Stufe nicht durchlaufen hat.");
            m_RejectedCount++;
            return;
        }

        if (m_BySym.Contains(def.sym))
        {
            // Kann die Registry eigentlich nicht liefern (sie weist Duplikate
            // ab). Wenn doch, gilt "erste gewinnt" (05 §7) - und es wird
            // gemeldet, statt still zu ueberschreiben.
            Report(report, true, def, "Zutatenbindung fuer diese Klasse ist bereits vorhanden - die zweite wird " + "abgewiesen. Betroffene Quellen sind im Delta-Protokoll und im Ladebericht " + "nachvollziehbar.");
            m_RejectedCount++;
            return;
        }

        // --- 1. Elternkette sammeln (05 E2) ---------------------------------
        array<ChefZ_IngredientDef> chain = new array<ChefZ_IngredientDef>();
        CollectChain(def, defs, chain);
        if (chain.Count() > 1)
            m_InheritedCount++;

        // --- 2. Felder ueber die Kette aufloesen ----------------------------
        array<string> catNames  = InheritedCategories(chain);
        array<string> tagNames  = InheritedTags(chain);
        string stateName        = InheritedText(chain, "defaultState");
        string unitName         = InheritedText(chain, "quantityUnit");
        string containerName    = InheritedText(chain, "containerCategory");
        string returnName       = InheritedText(chain, "returnContainer");
        float  perWhole;
        ResolveUnitsPerWholeItem(chain, perWhole);
        bool   decays           = InheritedDecays(chain);

        if (ChefZ_Undefined.IsTextUndefined(unitName))
            unitName = ChefZ_IngredientDef.DEFAULT_QUANTITY_UNIT;

        bool isDefaultUnit = (unitName == ChefZ_IngredientDef.DEFAULT_QUANTITY_UNIT);

        // --- 3. Menge pruefen (05 §7) ----------------------------------------
        //
        // BEFUND 31.08.2026 (S4, Gruppe "Einheiten", IngredientSelfTest:532):
        // hier stand
        //     if (ChefZ_Undefined.IsFloatUndefined(perWhole)) perWhole = 1.0;
        // Seit der Sentinelumstellung vom 28.08.2026 IST
        // ChefZ_Undefined.FLOAT == 0.0 (siehe Kopf von ChefZ_Undefined). Damit
        // hat diese Zeile jede AUSDRUECKLICH geschriebene Null in eine 1
        // verwandelt - und zwar genau die Null, die
        // ChefZ_IngredientDef.HasUnitsPerWholeItem() ueber explicitFields[]
        // am Sentinel vorbeigerettet hatte, damit der Nenner Null hier
        // ankommt. Die Abweisung darunter war seitdem unerreichbar: wer
        // "quantityUnit": "GRAM" mit "unitsPerWholeItem": 0 schreibt, bekam
        // still eine 1 statt der Fehlermeldung, die 05 §7 zusagt. Kein
        // Absturz, aber eine falsche Menge ohne jeden Hinweis.
        //
        // Die Frage "hat die Kette etwas gesagt?" gehoert deshalb in die
        // Aufloesung (ResolveUnitsPerWholeItem) und nicht in einen Vergleich
        // gegen den Wert - der Wert kann sie nicht beantworten.
        if (perWhole <= 0.0)
        {
            if (!isDefaultUnit)
            {
                // Der Wert steht im Nenner der Einheitenrechnung (05 §6).
                // Eine Null dort waere eine Division durch Null im heissen
                // Pfad - deshalb als einziger Zutatenfehler eine Abweisung.
                Report(report, true, def, "unitsPerWholeItem = " + perWhole.ToString() + " bei quantityUnit \"" + unitName + "\" - der Eintrag wird abgewiesen. Der Wert steht im Nenner " + "der Einheitenrechnung; ein Wert <= 0 waere eine Division durch Null. " + "Abhilfe: einen positiven Wert angeben oder quantityUnit weglassen.");
                m_RejectedCount++;
                return;
            }

            Report(report, false, def, "unitsPerWholeItem = " + perWhole.ToString() + " ist unbrauchbar und wird auf 1 " + "geklemmt. Bei der Einheit \"" + ChefZ_IngredientDef.DEFAULT_QUANTITY_UNIT + "\" ist ein volles Item ohnehin genau eine Einheit (05 §6).");
            perWhole = 1.0;
        }

        // --- 4. Info bauen ---------------------------------------------------
        ChefZ_IngredientInfo info = new ChefZ_IngredientInfo();
        info.classSym          = def.sym;
        info.sourceRef         = def.sourceRef;
        info.isChefZManaged    = true;
        info.unitsPerWholeItem = perWhole;
        info.decays            = decays;
        info.quantityUnit      = ChefZ_SymbolTable.Intern(unitName);

        ResolveCategories(info, catNames, def, report);
        ResolveTags(info, tagNames, def, report);
        ResolveState(info, stateName, states, def, report);

        // Behaelterbindung wird hier NICHT gegen eine Registry geprueft: die
        // Behaelterkategorien und die Bedeutung von "AUTO" gehoeren dem
        // Container System (16) und entstehen mit S17. Zweimal zu pruefen
        // hiesse, zwei Wahrheiten ueber dieselbe Angabe zu fuehren.
        if (!ChefZ_Undefined.IsTextUndefined(containerName))
            info.containerCategory = ChefZ_SymbolTable.Intern(containerName);
        if (!ChefZ_Undefined.IsTextUndefined(returnName))
            info.returnContainer = ChefZ_SymbolTable.Intern(returnName);

        // Closure aus den DIREKTEN Kategorien, einmal und fuer immer (04 §4).
        // Ueber eine lokale Variable, weil BuildClosure einen out-Parameter
        // hat und ein Feldausdruck als out-Argument in Enforce nicht
        // zugesichert ist.
        ChefZ_CategoryClosure closure = info.closure;
        Cats().BuildClosure(info.categories, closure);
        info.closure = closure;

        // --- 5. Klasse existiert ueberhaupt? (05 §7) --------------------------
        WarnIfClassMissing(def, report);

        m_BySym.Set(def.sym, m_Infos.Count());
        m_Infos.Insert(info);
    }

    //--------------------------------------------------------------------------
    // Vererbung (05 E2)
    //--------------------------------------------------------------------------

    /**
     * Sammelt die Kette [eigene Deklaration, naechster deklarierter Vorfahr,
     * ...] entlang der CfgVehicles-Elternkette.
     *
     * Nur Vorfahren, die SELBST eine Zutatenbindung haben, landen darin - alle
     * anderen sind fuer diese Frage bedeutungslos. Damit ist die Kette in der
     * Praxis ein bis drei Eintraege lang.
     *
     * Es wird ausdruecklich NICHT rekursiv aufgeloest ("erst den Vorfahren
     * fertig bauen, dann das Kind"): weil jedes Feld einzeln beim ERSTEN
     * Vorfahren geholt wird, der etwas dazu sagt, ist das Ergebnis dasselbe -
     * ohne Reihenfolgeabhaengigkeit und ohne Zwischenspeicher.
     */
    private void CollectChain(notnull ChefZ_IngredientDef def, notnull ChefZ_Registry<ChefZ_IngredientDef> defs, notnull array<ChefZ_IngredientDef> outChain)
    {
        outChain.Clear();
        outChain.Insert(def);

        // Besuchte Klassennamen statt besuchter Objekte: ein Namensvergleich
        // ist in Enforce zugesichert, ein Objektvergleich ueber array.Find()
        // nicht. Der Besuchsvermerk ist zugleich die Zyklenbremse - eine
        // Config, die sich selbst als Elternteil nennt, laeuft hier nicht
        // endlos.
        array<string> seen = new array<string>();
        seen.Insert(def.id);

        string current = def.id;
        string parent;
        int guard = 0;

        while (guard < MAX_PARENT_CHAIN)
        {
            guard++;

            if (!ResolveConfigParent(current, parent))
                break;
            if (parent == "" || seen.Find(parent) >= 0)
                break;

            seen.Insert(parent);

            ChefZ_IngredientDef ancestor = defs.FindByName(parent);
            if (ancestor && !ancestor.disabled)
                outChain.Insert(ancestor);

            current = parent;
        }
    }

    /**
     * Elternklasse einer Itemklasse laut CfgVehicles.
     *
     * Eigene Methode und protected, aus zwei Gruenden:
     *
     *   1. Sie ist der EINZIGE Config-Zugriff der Vererbung. Wer wissen will,
     *      woher die Elternkette kommt, muss genau hierhin schauen.
     *   2. Der Selbsttest ersetzt sie durch eine Tabelle. Sonst waere 05 E2 -
     *      die zentrale Entscheidung dieses Teilsystems - nur auf einem
     *      laufenden Server mit echtem Content pruefbar, und damit praktisch
     *      gar nicht.
     *
     * false heisst "keine Elternklasse bekannt" und beendet den Aufstieg.
     */
    protected bool ResolveConfigParent(string className, out string parentName)
    {
        parentName = "";
        if (!g_Game)
            return false;
        return g_Game.ConfigGetBaseName(CFG_VEHICLES + " " + className, parentName);
    }

    /**
     * Erstes nicht-null Kategorienarray der Kette.
     *
     * GANZERSATZ, nicht Vereinigung: eine Liste ist eine Aussage als Ganzes
     * (02 E3). Wer in einer abgeleiteten Klasse categories[] nennt, ersetzt
     * die geerbte Liste - genau so steht das Beispiel in 05 §2, wo die
     * abgeleitete Wurst die Basiskategorie mit aufzaehlt.
     */
    private array<string> InheritedCategories(notnull array<ChefZ_IngredientDef> chain)
    {
        for (int i = 0; i < chain.Count(); i++)
        {
            if (chain.Get(i).categories)
                return chain.Get(i).categories;
        }
        return null;
    }

    private array<string> InheritedTags(notnull array<ChefZ_IngredientDef> chain)
    {
        for (int i = 0; i < chain.Count(); i++)
        {
            if (chain.Get(i).tags)
                return chain.Get(i).tags;
        }
        return null;
    }

    //! Erstes gesetztes Textfeld der Kette. Der Feldname wird verglichen,
    //! weil Enforce keine Referenz auf ein Feld kennt; die Kette ist kurz und
    //! der Aufruf faellt einmal beim Boot an.
    private string InheritedText(notnull array<ChefZ_IngredientDef> chain, string field)
    {
        for (int i = 0; i < chain.Count(); i++)
        {
            ChefZ_IngredientDef d = chain.Get(i);
            string value = ChefZ_Undefined.TEXT;

            if (field == "defaultState")           value = d.defaultState;
            else if (field == "quantityUnit")      value = d.quantityUnit;
            else if (field == "containerCategory") value = d.containerCategory;
            else if (field == "returnContainer")   value = d.returnContainer;

            if (!ChefZ_Undefined.IsTextUndefined(value))
                return value;
        }
        return ChefZ_Undefined.TEXT;
    }

    /**
     * Die Stueckzahl der Kette: erster Eintrag, der ueberhaupt etwas sagt -
     * sonst 1.0.
     *
     * Der Unterschied zu den Inherited*-Helfern darueber, und der Grund fuer
     * den out-Parameter: hier gibt es KEINEN Sentinel, den man
     * zurueckgeben koennte. Seit dem 28.08.2026 ist
     * ChefZ_Undefined.FLOAT == 0.0 (Kopf von ChefZ_Undefined), und die
     * ausdrueckliche 0 ist ausgerechnet der Wert, auf den es ankommt: sie ist
     * der Nenner Null, den AdmitOne abweisen soll (05 §7). Ein Sentinel, der
     * genau den einen Wert verschluckt, den er durchlassen muesste, ist
     * keiner - deshalb entscheidet HasUnitsPerWholeItem() (explicitFields[])
     * und nicht der Zahlenwert.
     *
     * @param value  wird IMMER geschrieben, auch wenn niemand etwas sagt: die
     *        Vorgabe 1.0 heisst "ein volles Item ist eine Einheit" (05 §6).
     *        Der Aufrufer muss sich damit nicht darauf verlassen, was Enforce
     *        mit einem out-Parameter beim Eintritt tut.
     */
    private void ResolveUnitsPerWholeItem(notnull array<ChefZ_IngredientDef> chain, out float value)
    {
        value = 1.0;

        for (int i = 0; i < chain.Count(); i++)
        {
            if (chain.Get(i).HasUnitsPerWholeItem())
            {
                value = chain.Get(i).unitsPerWholeItem;
                return;
            }
        }
    }

    //! bool hat keinen Sentinel: "gesetzt" ist der Eintrag in explicitFields[]
    //! (02 E3, Mittel 3). Ohne jede Angabe gilt false - 01 V9: Edible_Base
    //! verdirbt von Haus aus nicht.
    private bool InheritedDecays(notnull array<ChefZ_IngredientDef> chain)
    {
        for (int i = 0; i < chain.Count(); i++)
        {
            if (chain.Get(i).HasDecays())
                return chain.Get(i).decays;
        }
        return false;
    }

    //--------------------------------------------------------------------------
    // Symbolaufloesung mit Referenzpruefung (05 §7)
    //--------------------------------------------------------------------------

    /**
     * Unbekannte Kategorie: DIESE EINE faellt weg, die uebrigen bleiben
     * gueltig, WARN mit Klasse und Kategorie. Das Item wird dadurch enger
     * matchbar, nie falscher.
     */
    private void ResolveCategories(notnull ChefZ_IngredientInfo info, array<string> names, notnull ChefZ_IngredientDef def, ChefZ_LoadReport report)
    {
        if (!names)
            return;

        for (int i = 0; i < names.Count(); i++)
        {
            string name = names.Get(i);
            if (name == "")
                continue;

            ChefZ_Sym sym = ChefZ_SymbolTable.Lookup(name);
            if (!Cats().Exists(sym))
            {
                Report(report, false, def, "Kategorie \"" + name + "\" ist unbekannt und wird fuer diese Zutat " + "ausgelassen; die uebrigen Kategorien bleiben gueltig. Haeufigste " + "Ursachen: Tippfehler, oder das Modul mit dieser Kategorie ist nicht " + "geladen.");
                continue;
            }

            if (info.categories.Find(sym) < 0)
                info.categories.Insert(sym);
        }
    }

    private void ResolveTags(notnull ChefZ_IngredientInfo info, array<string> names, notnull ChefZ_IngredientDef def, ChefZ_LoadReport report)
    {
        if (!names)
            return;

        for (int i = 0; i < names.Count(); i++)
        {
            string name = names.Get(i);
            if (name == "")
                continue;

            ChefZ_Sym sym = ChefZ_SymbolTable.Lookup(name);
            if (!Cats().TagExists(sym))
            {
                Report(report, false, def, "Tag \"" + name + "\" ist unbekannt und wird fuer diese Zutat ausgelassen. " + "Tags werden nie implizit angelegt (04 §6) - ein implizit angelegter Tag " + "matchte nie und waere stiller toter Code.");
                continue;
            }

            if (info.staticTags.Find(sym) < 0)
                info.staticTags.Insert(sym);
        }
    }

    /**
     * defaultState ist der V1-NORMALFALL der Zustandsermittlung (06 §3,
     * Schritt 2): die Klasse IST der Zustand.
     *
     * Ein unbekannter Zustand wird verworfen und gemeldet. Ihn stehenzulassen
     * waere schlimmer als ihn wegzulassen: das Item behauptete dann einen
     * Zustand, den kein Rezept, kein Prozess und keine Anzeige kennt.
     */
    private void ResolveState(notnull ChefZ_IngredientInfo info, string name, ChefZ_Registry<ChefZ_StateDef> states, notnull ChefZ_IngredientDef def, ChefZ_LoadReport report)
    {
        if (ChefZ_Undefined.IsTextUndefined(name))
            return;

        if (states && !states.ContainsName(name))
        {
            Report(report, false, def, "defaultState \"" + name + "\" ist kein bekannter Zustand und wird ausgelassen. " + "Der Zustand des Items ergibt sich dann aus seiner Vanilla-FoodStage " + "(06 §3, Schritt 3).");
            return;
        }

        info.defaultState = ChefZ_SymbolTable.Intern(name);
    }

    /**
     * 05 §7: "JSON-Eintrag nennt eine Klasse, die es im Spiel nicht gibt -
     * Eintrag bleibt geladen, WARN."
     *
     * Bewusst kein Fehler: Bindungen fuer OPTIONALE Fremdmods sollen auf
     * Servern ohne diesen Mod nicht knallen. Der statische Validator
     * (classrefs) prueft dasselbe vor dem Start.
     */
    private void WarnIfClassMissing(notnull ChefZ_IngredientDef def, ChefZ_LoadReport report)
    {
        if (m_SkipClassExistsCheck || !g_Game)
            return;

        if (g_Game.ConfigIsExisting(CFG_VEHICLES  + " " + def.id))  return;
        if (g_Game.ConfigIsExisting(CFG_WEAPONS   + " " + def.id))  return;
        if (g_Game.ConfigIsExisting(CFG_MAGAZINES + " " + def.id))  return;

        Report(report, false, def, "Die Klasse \"" + def.id + "\" existiert in keiner geladenen Config. Die Bindung " + "bleibt geladen und wirkt, sobald das liefernde Addon dazukommt - so laufen " + "Bindungen fuer optionale Fremdmods auf Servern ohne diesen Mod nicht auf " + "einen Fehler.");
    }

    //--------------------------------------------------------------------------
    // Rueckwaertsindizes (05 §4, Schritt 4 und 5)
    //--------------------------------------------------------------------------

    /**
     * Kategorie -> Klassen und Tag -> Klassen.
     *
     * Eine Klasse wird nicht nur unter ihren DIREKTEN Kategorien eingetragen,
     * sondern unter allen Vorfahren. Grund: ein Slot "category: Fleisch"
     * matcht auch Wildfleisch, weil der Kategorietest auf der Closure laeuft
     * (04 E1). Wuerde der Index nur direkte Zugehoerigkeit fuehren, waere die
     * Schaetzung fuer Oberkategorien systematisch zu klein - und der Matcher
     * probierte den falschen Slot zuerst (07 E4).
     */
    private void BuildReverseIndices()
    {
        array<ChefZ_Sym> ancestors = new array<ChefZ_Sym>();

        for (int i = 0; i < m_Infos.Count(); i++)
        {
            ChefZ_IngredientInfo info = m_Infos.Get(i);

            for (int c = 0; c < info.categories.Count(); c++)
            {
                ChefZ_Sym cat = info.categories.Get(c);
                AddToIndex(m_ByCategory, cat, info.classSym);

                Cats().GetAncestors(cat, ancestors);
                for (int a = 0; a < ancestors.Count(); a++)
                    AddToIndex(m_ByCategory, ancestors.Get(a), info.classSym);
            }

            for (int t = 0; t < info.staticTags.Count(); t++)
                AddToIndex(m_ByTag, info.staticTags.Get(t), info.classSym);
        }
    }

    private void AddToIndex(notnull map<int, ref array<ChefZ_Sym>> index, ChefZ_Sym key, ChefZ_Sym classSym)
    {
        if (!ChefZ_SymbolTable.IsValid(key))
            return;

        array<ChefZ_Sym> list;
        if (!index.Find(key, list))
        {
            list = new array<ChefZ_Sym>();
            index.Set(key, list);
        }

        if (list.Find(classSym) < 0)
            list.Insert(classSym);
    }

    //==========================================================================
    // Abfragen - heisser Pfad
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    bool IsKnown(ChefZ_Sym classSym)
    {
        if (!m_Ready)
        {
            ReportNotReady("IsKnown");
            return false;
        }
        return m_BySym.Contains(classSym);
    }

    /**
     * Stammdaten einer Klasse, oder null.
     *
     * null ist die normale, haeufigste Antwort: 99 % aller Vanilla-Klassen
     * sind nicht deklariert (05 §7). Der Aufrufer baut daraus Fakten mit
     * leerer Closure und isChefZManaged = false - kein Fehler, kein Log.
     */
    ChefZ_IngredientInfo Resolve(ChefZ_Sym classSym)
    {
        if (!m_Ready)
        {
            ReportNotReady("Resolve");
            return null;
        }

        int index;
        if (!m_BySym.Find(classSym, index))
            return null;
        return m_Infos.Get(index);
    }

    //! Lookup() und nicht Intern(): eine Abfrage darf keine Symbole anlegen.
    ChefZ_IngredientInfo ResolveByName(string className)
    {
        return Resolve(ChefZ_SymbolTable.Lookup(className));
    }

    //--------------------------------------------------------------------------
    // Rueckwaertssuche: Cookbook, Validator, selectivityHint (05 §3.2)
    //--------------------------------------------------------------------------

    void GetClassesInCategory(ChefZ_Sym category, out array<ChefZ_Sym> outClasses)
    {
        CopyIndex(m_ByCategory, category, outClasses);
    }

    void GetClassesWithTag(ChefZ_Sym tag, out array<ChefZ_Sym> outClasses)
    {
        CopyIndex(m_ByTag, tag, outClasses);
    }

    /**
     * Kopie und nicht die Indexliste selbst.
     *
     * Der Bestand ist nach Build unveraenderlich (05 §5). Wer die Innenliste
     * zurueckbekaeme, koennte sie sortieren oder leeren und damit den
     * selectivityHint aller Rezepte verschieben, die diese Kategorie nennen.
     */
    private void CopyIndex(notnull map<int, ref array<ChefZ_Sym>> index, ChefZ_Sym key, out array<ChefZ_Sym> outClasses)
    {
        if (!outClasses)
            outClasses = new array<ChefZ_Sym>();
        outClasses.Clear();

        if (!m_Ready)
        {
            ReportNotReady("GetClasses");
            return;
        }

        array<ChefZ_Sym> list;
        if (!index.Find(key, list))
            return;

        for (int i = 0; i < list.Count(); i++)
            outClasses.Insert(list.Get(i));
    }

    /**
     * Geschaetzte Trefferzahl eines Selektors (07 E4).
     *
     * Der Matcher probiert den am staerksten eingeschraenkten Slot zuerst;
     * diese Zahl ist sein Massstab. Ein class-Selektor hat Hint 1 und braucht
     * diese Funktion nicht - er kennt seine Antwort selbst.
     *
     * 0 heisst "kein deklariertes Kandidatenfeld". Das ist keine Notluege: ein
     * Kategorie- oder Tag-Selektor auf etwas, das keine einzige Klasse
     * fuehrt, matcht tatsaechlich nie, und er gehoert deshalb an den Anfang
     * der Slotreihenfolge, wo er den Suchbaum sofort abschneidet.
     */
    int EstimateCandidateCount(ChefZ_Sym categoryOrTag)
    {
        if (!m_Ready)
        {
            ReportNotReady("EstimateCandidateCount");
            return 0;
        }

        array<ChefZ_Sym> list;
        if (m_ByCategory.Find(categoryOrTag, list))
            return list.Count();
        if (m_ByTag.Find(categoryOrTag, list))
            return list.Count();
        return 0;
    }

    int GetKnownCount()
    {
        return m_Infos.Count();
    }

    int GetRejectedCount()
    {
        return m_RejectedCount;
    }

    int GetIndexedCategoryCount()
    {
        return m_ByCategory.Count();
    }

    int GetIndexedTagCount()
    {
        return m_ByTag.Count();
    }

    //! Stammdaten der Reihe nach - fuer Diagnose, Auszug und Validator.
    ChefZ_IngredientInfo GetAt(int index)
    {
        if (index < 0 || index >= m_Infos.Count())
            return null;
        return m_Infos.Get(index);
    }

    //==========================================================================
    // Innere Hilfen
    //==========================================================================

    private ChefZ_CategoryManager Cats()
    {
        if (m_CategoriesForTest)
            return m_CategoriesForTest;
        return ChefZ_CategoryManager.Get();
    }

    private void ResetState()
    {
        m_Infos.Clear();
        m_BySym.Clear();
        m_ByCategory.Clear();
        m_ByTag.Clear();

        m_Ready          = false;
        m_NotReadyLogged = false;
        m_RejectedCount  = 0;
        m_InheritedCount = 0;
    }

    /**
     * Setzt den Manager auf "nicht gebaut".
     *
     * Unterschied zu Build(null, null): danach ist IsReady() false und jede
     * Abfrage meldet einmal den Reihenfolgefehler. Der SAFE_MODE benutzt
     * bewusst Build() mit leeren Quellen - dort ist "bereit und leer" die
     * wahre Aussage (02 §8).
     */
    void Reset()
    {
        ResetState();
    }

    //! 05 §7 sinngemaess, 04 §6 woertlich: Abfrage vor Build liefert die
    //! neutrale Antwort und meldet EINMAL. Kein Nullzugriff, kein Absturz.
    private void ReportNotReady(string what)
    {
        if (m_NotReadyLogged)
            return;
        m_NotReadyLogged = true;

        if (m_QuietForTest)
            return;

        ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, "ChefZ_IngredientManager." + what + "() wurde vor Build() aufgerufen - die Antwort " + "ist \"unbekannt\". Jedes Item gilt bis zum Aufbau als nicht deklariert; " + "Kategorie- und Tag-Selektoren matchen solange nicht. Diese Meldung erscheint " + "genau einmal.");
    }

    private void Report(ChefZ_LoadReport report, bool isError, notnull ChefZ_IngredientDef def, string msg)
    {
        if (report)
        {
            if (isError)
                report.AddError(def.sourceRef, def.id, msg);
            else
                report.AddWarn(def.sourceRef, def.id, msg);
            return;
        }

        if (m_QuietForTest)
            return;

        string line = def.sourceRef + " / " + def.id + ": " + msg;
        if (isError)
            ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, line);
        else
            ChefZ_Log.Warn(ChefZ_LogChannel.CONFIG, line);
    }

    //==========================================================================
    // Diagnose
    //==========================================================================

    void DumpIngredients(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        string chefzTxt2 = "Zutatenbindungen: " + GetKnownCount().ToString() + " Klassen" + ", abgewiesen " + m_RejectedCount.ToString();
        chefzTxt2 = chefzTxt2 + ", Kategorien indiziert " + m_ByCategory.Count().ToString() + ", Tags indiziert " + m_ByTag.Count().ToString() + ", ready=";
        chefzTxt2 = chefzTxt2 + m_Ready.ToString();
        outLines.Insert(chefzTxt2);

        for (int i = 0; i < m_Infos.Count(); i++)
            outLines.Insert("  " + m_Infos.Get(i).ToLine());
    }

    //! 18 §2: der Auszug entsteht nur, wenn Kanal CONFIG auf DEBUG steht. Die
    //! Zeichenketten werden erst hinter der Wache gebaut.
    private void LogIfDebug()
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        DumpIngredients(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.CONFIG, lines);
    }

    //==========================================================================
    // Nur fuer den Selbsttest (S4)
    //==========================================================================

    /**
     * Unterdrueckt Meldungen dieses Managers.
     *
     * Noetig, weil der Test absichtlich Fehlerfaelle durchspielt. Wuerden sie
     * ins RPT laufen, staende dort ein Fehler, den es nicht gibt - und
     * ChefZ_Log.GetErrorCount() speist die Safe-Mode-Schwelle (18 §4). Der
     * Test koennte den Server also in den SAFE_MODE treiben.
     */
    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }

    //! Kategoriebaum des Tests statt des Singletons.
    void SetCategoryManagerForTest(ChefZ_CategoryManager mgr)
    {
        m_CategoriesForTest = mgr;
    }

    /**
     * Schaltet die Existenzpruefung der Klasse ab.
     *
     * Der Test arbeitet mit Klassennamen, die es im Spiel nicht gibt - ohne
     * diesen Schalter wuerde er die Warnung aus 05 §7 fuer jede seiner
     * Testklassen ausloesen und damit genau das messen, was er nicht meint.
     */
    void SetSkipClassExistsCheckForTest(bool skip)
    {
        m_SkipClassExistsCheck = skip;
    }
}
