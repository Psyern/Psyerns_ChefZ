//==============================================================================
// ChefZ_ToolRegistry - Klasse -> Werkzeuggruppen, einmal beim Boot aufgeloest
//
// Entwurf: 11 §4 (Schnittstelle woertlich), 11 E8 (Werkzeuggruppen als Daten,
// nie als Klassenliste im Code), 11 §7 (Prozess mit unbekannter Toolgruppe
// wird ABGEWIESEN), 05 E2 (Vererbung entlang der CfgVehicles-Elternkette),
// 02 E6 (kein CfgVehicles-Vollscan).
//
// ---------------------------------------------------------------------------
// Die eine Zusage dieser Klasse
// ---------------------------------------------------------------------------
// Im gesamten Core steht kein Werkzeugname. Ein Prozess verlangt eine GRUPPE;
// welche Klassen dieser Gruppe angehoeren, steht in CfgChefZTools und damit
// im Content oder beim Serverbetreiber. Ein Betreiber kann ein Messer aus
// einem fremden Mod aufnehmen, ohne dass irgendjemand ChefZ anfasst - genau
// das verlangt Architekturplan §13.
//
// ---------------------------------------------------------------------------
// Beide Schreibweisen, ein Index
// ---------------------------------------------------------------------------
// Build() liest gruppenweise (id = Gruppe, classes[]) UND klassenweise
// (id = Klasse, toolCategories[]) und traegt beide in DENSELBEN Index ein:
//
//     m_GroupsByClass : Klassensymbol -> Liste von Gruppensymbolen
//
// Die Richtung ist bewusst so herum. Zur Laufzeit lautet die Frage immer
// "was habe ich in der Hand, und wozu gehoert das" - nie "wer gehoert zu
// CUTTING_TOOL". Der umgekehrte Index waere eine zweite Wahrheit, die
// gepflegt werden muesste.
//
// ---------------------------------------------------------------------------
// allowSubclasses und warum es LAZY aufgeloest wird
// ---------------------------------------------------------------------------
// Eine Gruppe mit allowSubclasses nimmt auch Ableitungen ihrer Klassen auf.
// Beim Boot alle Ableitungen zu ERMITTELN hiesse, ueber CfgVehicles zu
// laufen - genau der Vollscan, den 02 E6 verbietet.
//
// Stattdessen laeuft die Frage in die andere Richtung: wird zur Laufzeit nach
// einer Klasse gefragt, die nicht direkt eingetragen ist, steigt der Index
// EINMAL ihre Elternkette hoch (dieselbe Kette wie 05 E2, hoechstens ein paar
// Stufen) und merkt sich das Ergebnis. Danach ist die Antwort ein
// Map-Zugriff. Auch das negative Ergebnis wird gemerkt - sonst liefe ein
// Vanilla-Item, das ein Spieler in der Hand haelt, bei jedem Zielwechsel des
// Fadenkreuzes erneut durch die Config.
//
// KEIN CONTENT.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_ToolRegistry : Managed
{
    static const string CFG_VEHICLES     = "CfgVehicles";
    static const int    MAX_PARENT_CHAIN = 64;

    private static ref ChefZ_ToolRegistry s_Instance;

    //! Klassensymbol -> Gruppensymbole. Der einzige Index; alles andere ist
    //! Buchhaltung fuer die Diagnose.
    private ref map<int, ref array<ChefZ_Sym>> m_GroupsByClass;

    //! Alle bekannten Gruppensymbole. Der ChefZ_ProcessCompiler prueft
    //! toolGroups[] dagegen - 11 §7: ein Prozess mit unbekannter Gruppe wird
    //! ABGEWIESEN, weil er ohne Werkzeugpruefung zu leicht ausloesbar waere.
    private ref array<ChefZ_Sym> m_Groups;

    //! Gruppen mit allowSubclasses, und die Klassen, deren Ableitungen dazu
    //! zaehlen. Nur diese Gruppen loesen ueberhaupt einen Kettenaufstieg aus.
    private ref map<int, ref array<ChefZ_Sym>> m_SubclassRoots;   // Gruppe -> Wurzelklassen

    //! Ergebnisse des Kettenaufstiegs, positive wie negative (siehe Kopf).
    private ref map<int, ref array<ChefZ_Sym>> m_InheritedCache;

    private bool m_Ready;
    private int  m_ClassCount;
    private int  m_RejectedCount;

    void ChefZ_ToolRegistry()
    {
        m_GroupsByClass  = new map<int, ref array<ChefZ_Sym>>();
        m_Groups         = new array<ChefZ_Sym>();
        m_SubclassRoots  = new map<int, ref array<ChefZ_Sym>>();
        m_InheritedCache = new map<int, ref array<ChefZ_Sym>>();
        m_Ready          = false;
        m_ClassCount     = 0;
        m_RejectedCount  = 0;
    }

    static ChefZ_ToolRegistry Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_ToolRegistry();
        return s_Instance;
    }

    //==========================================================================
    // BUILD
    //==========================================================================

    /**
     * Baut den Index aus den Werkzeugrecords.
     *
     * defs darf null sein. Ein Aufruf mit null ist die ausdrueckliche Art zu
     * sagen "Bestand leeren" - genau das braucht der SAFE_MODE (02 §8).
     * Danach ist die Registry "bereit und leer": jede Abfrage antwortet ruhig
     * false, statt einen Fehler ueber einen fehlenden Aufbau zu melden, den es
     * nie geben wird.
     */
    void Build(ChefZ_Registry<ChefZ_ToolGroupDef> defs, ChefZ_LoadReport report)
    {
        Clear();
        m_Ready = true;

        if (!defs || defs.Count() == 0)
            return;

        // Ueber Keys(): die Reihenfolge ist nach ID sortiert (03 §4) und damit
        // auf jedem Server dieselbe. Fuer das Ergebnis ist sie ohne Bedeutung,
        // fuer die Vergleichbarkeit zweier Ladeberichte ist sie alles.
        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_ToolGroupDef def = defs.Find(keys.Get(i));
            if (!def)
                continue;

            if (def.DeclaresMembers())
                AddGroupwise(def, report);

            if (def.DeclaresGroups())
                AddClasswise(def, report);
        }

        if (report)
        {
            string chefzTxt1 = "Werkzeuge: " + m_Groups.Count().ToString() + " Gruppen, " + m_ClassCount.ToString() + " Klassen zugeordnet, ";
            chefzTxt1 = chefzTxt1 + m_SubclassRoots.Count().ToString() + " Gruppen mit Vererbung" + ", " + m_RejectedCount.ToString() + " Eintraege verworfen.";
            report.AddInfo(chefzTxt1);
        }

        LogIfDebug();
    }

    private void Clear()
    {
        m_GroupsByClass.Clear();
        m_Groups.Clear();
        m_SubclassRoots.Clear();
        m_InheritedCache.Clear();
        m_Ready         = false;
        m_ClassCount    = 0;
        m_RejectedCount = 0;
    }

    /**
     * Gruppenweise (02 §5.1): id ist die GRUPPE, classes[] sind die
     * Mitglieder.
     *
     * Die Klassennamen werden interniert und NICHT nachgeschlagen: 05 E3
     * gilt hier genauso wie beim Faktensammler - eine Klasse, die ChefZ nicht
     * kennt, muss trotzdem adressierbar sein. Ein Messer aus einem fremden Mod
     * ist in ChefZ nie deklariert, und genau darum geht es in E8.
     *
     * Die EXISTENZ der Klasse in CfgVehicles wird ebenfalls nicht geprueft.
     * Sie waere eine sinnvolle Warnung, aber kein Fehler: eine Gruppe darf ein
     * Werkzeug aus einem OPTIONALEN Modul nennen, das auf diesem Server nicht
     * geladen ist. Dann ist der Eintrag wirkungslos - und wirkungslos ist
     * genau richtig.
     */
    private void AddGroupwise(notnull ChefZ_ToolGroupDef def, ChefZ_LoadReport report)
    {
        ChefZ_Sym group = ChefZ_SymbolTable.Intern(def.id);
        if (!ChefZ_SymbolTable.IsValid(group))
        {
            m_RejectedCount++;
            return;
        }

        RegisterGroup(group);

        for (int i = 0; i < def.classes.Count(); i++)
        {
            string cls = def.classes.Get(i);
            if (cls == "")
            {
                m_RejectedCount++;
                continue;
            }

            ChefZ_Sym classSym = ChefZ_SymbolTable.Intern(cls);
            LinkGroup(classSym, group);

            if (def.allowSubclasses)
                RegisterSubclassRoot(group, classSym);
        }
    }

    /**
     * Klassenweise (02 §4): id ist die KLASSE, toolCategories[] die Gruppen.
     *
     * allowSubclasses hat in dieser Schreibweise eine andere, aber
     * konsistente Bedeutung: die Ableitungen DIESER Klasse gehoeren allen
     * genannten Gruppen an. Das ist derselbe Satz wie oben, nur von der
     * anderen Seite gelesen.
     */
    private void AddClasswise(notnull ChefZ_ToolGroupDef def, ChefZ_LoadReport report)
    {
        ChefZ_Sym classSym = ChefZ_SymbolTable.Intern(def.id);
        if (!ChefZ_SymbolTable.IsValid(classSym))
        {
            m_RejectedCount++;
            return;
        }

        for (int i = 0; i < def.toolCategories.Count(); i++)
        {
            string name = def.toolCategories.Get(i);
            if (name == "")
            {
                m_RejectedCount++;
                continue;
            }

            ChefZ_Sym group = ChefZ_SymbolTable.Intern(name);
            RegisterGroup(group);
            LinkGroup(classSym, group);

            if (def.allowSubclasses)
                RegisterSubclassRoot(group, classSym);
        }
    }

    private void RegisterGroup(ChefZ_Sym group)
    {
        if (!ChefZ_SymbolTable.IsValid(group))
            return;
        if (m_Groups.Find(group) < 0)
            m_Groups.Insert(group);
    }

    private void RegisterSubclassRoot(ChefZ_Sym group, ChefZ_Sym rootClass)
    {
        if (!ChefZ_SymbolTable.IsValid(group) || !ChefZ_SymbolTable.IsValid(rootClass))
            return;

        array<ChefZ_Sym> roots;
        if (!m_SubclassRoots.Find(group, roots))
        {
            roots = new array<ChefZ_Sym>();
            m_SubclassRoots.Set(group, roots);
        }
        if (roots.Find(rootClass) < 0)
            roots.Insert(rootClass);
    }

    private void LinkGroup(ChefZ_Sym classSym, ChefZ_Sym group)
    {
        if (!ChefZ_SymbolTable.IsValid(classSym) || !ChefZ_SymbolTable.IsValid(group))
            return;

        array<ChefZ_Sym> groups;
        if (!m_GroupsByClass.Find(classSym, groups))
        {
            groups = new array<ChefZ_Sym>();
            m_GroupsByClass.Set(classSym, groups);
            m_ClassCount++;
        }
        if (groups.Find(group) < 0)
            groups.Insert(group);
    }

    //==========================================================================
    // Auskuenfte (11 §4)
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    int GetGroupCount()
    {
        return m_Groups.Count();
    }

    int GetClassCount()
    {
        return m_ClassCount;
    }

    //! Kennt der Bestand diese Gruppe? Der ChefZ_ProcessCompiler entscheidet
    //! damit ueber Annahme oder Abweisung eines Prozesses (11 §7).
    bool HasGroup(ChefZ_Sym group)
    {
        if (!ChefZ_SymbolTable.IsValid(group))
            return false;
        return m_Groups.Find(group) >= 0;
    }

    /**
     * Gehoert diese Klasse dieser Gruppe an (11 §4)?
     *
     * Erst der direkte Eintrag, dann - nur wenn die Gruppe ueberhaupt
     * Vererbung erlaubt - die Elternkette. Der haeufigste Fall (Vanilla-Item
     * ohne jeden Werkzeugbezug) endet nach zwei Map-Zugriffen.
     */
    bool IsToolOfGroup(ChefZ_Sym classSym, ChefZ_Sym group)
    {
        if (!ChefZ_SymbolTable.IsValid(classSym) || !ChefZ_SymbolTable.IsValid(group))
            return false;

        array<ChefZ_Sym> groups;
        if (m_GroupsByClass.Find(classSym, groups) && groups.Find(group) >= 0)
            return true;

        if (m_SubclassRoots.Count() == 0)
            return false;

        array<ChefZ_Sym> inherited = ResolveInherited(classSym);
        return inherited.Find(group) >= 0;
    }

    /**
     * Alle Gruppen dieser Klasse (11 §4).
     *
     * outGroups wird GELEERT und gefuellt, nie null. Die Reihenfolge ist die
     * Eintragungsreihenfolge und damit stabil - der Aufrufer soll sich darauf
     * verlassen koennen, wenn er die erste Gruppe fuer eine Meldung nimmt.
     */
    void GetGroupsForClass(ChefZ_Sym classSym, out array<ChefZ_Sym> outGroups)
    {
        if (!outGroups)
            outGroups = new array<ChefZ_Sym>();
        outGroups.Clear();

        if (!ChefZ_SymbolTable.IsValid(classSym))
            return;

        array<ChefZ_Sym> direct;
        if (m_GroupsByClass.Find(classSym, direct))
        {
            for (int i = 0; i < direct.Count(); i++)
                outGroups.Insert(direct.Get(i));
        }

        if (m_SubclassRoots.Count() == 0)
            return;

        array<ChefZ_Sym> inherited = ResolveInherited(classSym);
        for (int k = 0; k < inherited.Count(); k++)
        {
            ChefZ_Sym g = inherited.Get(k);
            if (outGroups.Find(g) < 0)
                outGroups.Insert(g);
        }
    }

    /**
     * Die Gegenrichtung: alle DEKLARIERTEN Klassen einer Gruppe.
     *
     * Fuer S15. Vanillas Craftsystem verlangt bei der Registrierung eine
     * Liste von KLASSENNAMEN (RecipeBase.InsertIngredient) und kann keine
     * Gruppe auswerten - der Werkzeugplatz eines Handwerksrezepts muss
     * deshalb ausgerollt werden.
     *
     * Ausgerollt werden die DEKLARIERTEN Klassen, nicht die abgeleiteten.
     * Das reicht und ist zugleich das Richtige: Vanillas Zutatenvergleich
     * benutzt g_Game.IsKindOf (RecipeBase.CheckIngredientMatch), eine
     * genannte Basisklasse deckt ihre Ableitungen also von selbst ab. Eine
     * Aufzaehlung aller Ableitungen waere ein voller CfgVehicles-Durchlauf
     * fuer ein Ergebnis, das Vanilla ohnehin selbst herstellt.
     *
     * Die Reihenfolge ist die der Map und damit NICHT zugesichert. Wer sie
     * braucht - und S15 braucht sie, weil Vanillas Rezept-IDs positionell
     * sind - sortiert selbst.
     *
     * Rein lesend.
     *
     * @return Zahl der eingetragenen Klassen.
     */
    int GetClassesInGroup(ChefZ_Sym group, out array<ChefZ_Sym> outClasses)
    {
        if (!outClasses)
            outClasses = new array<ChefZ_Sym>();
        outClasses.Clear();

        if (!ChefZ_SymbolTable.IsValid(group))
            return 0;

        array<int> keys = m_GroupsByClass.GetKeyArray();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_Sym classSym = keys.Get(i);

            array<ChefZ_Sym> groups;
            if (!m_GroupsByClass.Find(classSym, groups))
                continue;
            if (groups.Find(group) < 0)
                continue;

            outClasses.Insert(classSym);
        }

        return outClasses.Count();
    }

    //! Fuehrt diese Klasse ueberhaupt irgendein Werkzeug? Der billige
    //! Vorfilter fuer ActionCondition.
    bool IsAnyTool(ChefZ_Sym classSym)
    {
        if (!ChefZ_SymbolTable.IsValid(classSym))
            return false;
        if (m_GroupsByClass.Contains(classSym))
            return true;
        if (m_SubclassRoots.Count() == 0)
            return false;
        return ResolveInherited(classSym).Count() > 0;
    }

    //==========================================================================
    // Vererbung (lazy, siehe Kopf)
    //==========================================================================

    /**
     * Die geerbten Gruppen einer Klasse. Nie null, oft leer.
     *
     * Das Ergebnis wird gemerkt - auch das LEERE. Ohne den negativen Eintrag
     * liefe jedes Vanilla-Item bei jedem Zielwechsel des Fadenkreuzes erneut
     * durch die Config, und Configzugriffe sind das Teuerste in diesem Pfad.
     */
    private array<ChefZ_Sym> ResolveInherited(ChefZ_Sym classSym)
    {
        array<ChefZ_Sym> cached;
        if (m_InheritedCache.Find(classSym, cached))
            return cached;

        array<ChefZ_Sym> result = new array<ChefZ_Sym>();
        m_InheritedCache.Set(classSym, result);

        string className = ChefZ_SymbolTable.Name(classSym);
        if (className == "")
            return result;

        // Besuchte NAMEN statt Symbole: der Namensvergleich ist zugleich die
        // Zyklenbremse fuer eine Config, die sich selbst als Elternteil nennt.
        array<string> seen = new array<string>();
        seen.Insert(className);

        string current = className;
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

            ChefZ_Sym parentSym = ChefZ_SymbolTable.Lookup(parent);
            if (ChefZ_SymbolTable.IsValid(parentSym))
                CollectSubclassGroups(parentSym, result);

            current = parent;
        }

        return result;
    }

    //! Alle Gruppen, in denen parentSym als VERERBENDE Wurzel eingetragen ist.
    private void CollectSubclassGroups(ChefZ_Sym parentSym, notnull array<ChefZ_Sym> outGroups)
    {
        for (int i = 0; i < m_Groups.Count(); i++)
        {
            ChefZ_Sym group = m_Groups.Get(i);

            array<ChefZ_Sym> roots;
            if (!m_SubclassRoots.Find(group, roots))
                continue;
            if (roots.Find(parentSym) < 0)
                continue;
            if (outGroups.Find(group) < 0)
                outGroups.Insert(group);
        }
    }

    /**
     * Elternklasse laut CfgVehicles.
     *
     * protected und eine eigene Methode, aus denselben zwei Gruenden wie im
     * ChefZ_IngredientManager: sie ist der EINZIGE Config-Zugriff der
     * Vererbung, und der Selbsttest ersetzt sie durch eine Tabelle. Sonst
     * waere allowSubclasses nur auf einem laufenden Server mit echtem Content
     * pruefbar - und damit praktisch gar nicht.
     */
    protected bool ResolveConfigParent(string className, out string parentName)
    {
        parentName = "";
        if (!g_Game)
            return false;
        return g_Game.ConfigGetBaseName(CFG_VEHICLES + " " + className, parentName);
    }

    //==========================================================================
    // Diagnose (18)
    //==========================================================================

    void DumpTools(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("ChefZ Werkzeuge  bereit=" + m_Ready.ToString() + "  gruppen=" + m_Groups.Count().ToString() + "  klassen=" + m_ClassCount.ToString());

        for (int i = 0; i < m_Groups.Count(); i++)
        {
            ChefZ_Sym group = m_Groups.Get(i);
            string line = "  " + ChefZ_SymbolTable.NameOrMark(group) + ": ";

            array<ChefZ_Sym> roots;
            if (m_SubclassRoots.Find(group, roots) && roots.Count() > 0)
                line = line + "[vererbend: " + ChefZ_TextList.JoinSymbols(roots, ",") + "] ";

            outLines.Insert(line + MembersOf(group));
        }
    }

    private string MembersOf(ChefZ_Sym group)
    {
        string s = "";
        int shown = 0;

        array<int> classes = m_GroupsByClass.GetKeyArray();
        for (int i = 0; i < classes.Count(); i++)
        {
            array<ChefZ_Sym> groups;
            if (!m_GroupsByClass.Find(classes.Get(i), groups))
                continue;
            if (groups.Find(group) < 0)
                continue;

            if (shown > 0)
                s = s + ", ";
            s = s + ChefZ_SymbolTable.NameOrMark(classes.Get(i));
            shown++;

            // Die Diagnose ist eine Uebersicht, keine Datenbank. Wer alle
            // Mitglieder braucht, liest CfgChefZTools.
            if (shown >= 12)
            {
                s = s + ", ...";
                break;
            }
        }

        if (shown == 0)
            return "(keine Klasse)";
        return s;
    }

    private void LogIfDebug()
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        DumpTools(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.PROCESS, lines);
    }
}
