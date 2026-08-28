//==============================================================================
// ChefZ_CategoryManager - Kategoriebaum, Vorfahrenbitsets, flache Tag-Menge
//
// Entwurf: 04 (vollstaendig), insbesondere §2 (Schnittstelle woertlich),
// §4 (Datenfluss), §6 (Fehlerverhalten), E1 (Bitset), E2 (genau ein
// Elternteil), E3 (Kategorien vererben, Tags nie), E4 (Spezifitaet aus der
// Tiefe), sowie 09 E3 (die Tiefe ist das Spezifitaetsmass der Rezeptauswahl).
//
// Der Manager beantwortet genau eine Frage schnell: gehoert Symbol X zu
// Kategorie Y - direkt oder ueber Vererbung? Er enthaelt keine Zutat und kein
// Rezept, nur IDs und Elternbeziehungen. KEIN CONTENT: diese Datei DEFINIERT
// keine einzige Kategorie und keinen Tag - sie kommen ausnahmslos aus den
// Registries. Wo in den Kommentaren Namen wie SAUSAGE oder CHEFZ_SMOKED
// auftauchen, sind es woertliche Beispiele aus Entwurf 04; im Code steht
// keiner von ihnen.
//
// ---------------------------------------------------------------------------
// GENAU EIN ELTERNTEIL - die Entscheidung, die alles andere traegt (04 E2)
// ---------------------------------------------------------------------------
// ChefZ_CategoryDef traegt ein einzelnes Feld "parent", keine Liste. Damit ist
// Mehrfachvererbung schon strukturell unmoeglich, und das ist der Zweck:
//
//   - "Tiefe" bleibt ein wohldefinierter Wert. Mit zwei Eltern gaebe es zwei
//     Tiefen, und GetSpecificityWeight() muesste sich fuer eine entscheiden -
//     nach welcher Regel? Jede Antwort haengt an der Einlesereihenfolge.
//   - Die Rezeptauswahl (09) sortiert nach genau diesem Gewicht. Eine
//     mehrdeutige Tiefe macht die Auswahl von der Ladereihenfolge der Addons
//     abhaengig: derselbe Kessel ergaebe auf zwei Servern zwei Gerichte.
//   - IsInCategory bliebe ein Bit-Test, aber der AUFBAU wuerde zur Graphsuche
//     mit Mehrfachbesuch.
//
// Mehrfachzugehoerigkeit wird ueber TAGS ausgedrueckt - dafuer sind sie da
// (04 §3). 04 E2 und 09 E3 haengen zusammen und duerfen nicht getrennt
// revidiert werden.
//
// ---------------------------------------------------------------------------
// Aufbau in fuenf Schritten (04 §4)
// ---------------------------------------------------------------------------
//   1. Kategorie-IDs -> ChefZ_Sym + dichter Bitindex 0..n-1, in stabiler
//      Reihenfolge (Registry.Keys() ist nach ID sortiert, 03 §4)
//   2. Elternkanten aufloesen; unbekanntes parent -> Wurzel + WARN
//   3. Zyklen erkennen; alle Kategorien eines Zyklus verwerfen + ERROR
//   4. Vorfahrenbitsets in topologischer Reihenfolge (Tiefe aufsteigend)
//   5. Spezifitaetsgewicht = 1.0 + 0.5 * Tiefe
//   6. Tag-IDs internen - flach, ohne Vererbung
//
// Nach Build() ist der Bestand unveraenderlich. Der Manager haelt KEINEN
// Item-Zustand (04 §5): er wird nicht persistiert und nicht gesynct, Client
// und Server bauen ihn identisch aus Rang 1/2 auf.
//
// Layer: 3_Game. Er liest Registries und Einstellungen, kennt aber keinen
// Engine-Typ - kein ItemBase, kein EntityAI.
//==============================================================================

class ChefZ_CategoryManager
{
    //! 04 E4 / 09: Gewicht = base + perDepth * Tiefe. Wurzel = 1.0, Tiefe 1 =
    //! 1.5, Tiefe 2 = 2.0. Kein Content-Autor pflegt eine Zahl - genau das ist
    //! der Punkt der Entscheidung.
    static const float DEFAULT_WEIGHT_BASE      = 1.0;
    static const float DEFAULT_WEIGHT_PER_DEPTH = 0.5;

    /**
     * Letzter Rueckfall fuer den Deckel, wenn es (noch) keine Einstellungen
     * gibt. Der massgebliche Wert ist "maxCategories" aus Core.json - 04 §6:
     * "Das Limit ist ein Config-Wert, kein Codewert." Diese Konstante ist
     * ausschliesslich das Netz fuer den Fall, dass Build() vor dem Laden der
     * Einstellungen gerufen wird.
     */
    static const int FALLBACK_MAX_CATEGORIES = 256;

    static const int NO_INDEX = -1;

    //! Farben der Zyklensuche (04 §4, Schritt 3). Auf Klassenebene und nicht
    //! als lokale Konstanten, weil Enforce lokale const-Deklarationen nicht
    //! zugesichert unterstuetzt.
    static const int COLOUR_WHITE = 0;      // unbesucht
    static const int COLOUR_GREY  = 1;      // liegt im aktuell verfolgten Pfad
    static const int COLOUR_BLACK = 2;      // abgeschlossen

    private static ref ChefZ_CategoryManager s_Instance;

    //--- Baum, alles ueber den dichten Bitindex 0..n-1 adressiert ------------
    private ref array<int>    m_Sym;          // Bitindex -> ChefZ_Sym
    private ref array<string> m_Id;           // Bitindex -> Klartext-ID
    private ref array<string> m_DisplayKey;   // Bitindex -> Anzeigeschluessel
    private ref array<int>    m_Parent;       // Bitindex -> Bitindex | NO_INDEX
    private ref array<int>    m_Depth;        // Bitindex -> Tiefe, Wurzel = 0
    private ref array<ref ChefZ_CategoryClosure> m_Closures;
    private ref array<ref array<int>>            m_Children;
    private ref map<int, int>                    m_BitBySym;

    //--- Tags, flach (04 §3) --------------------------------------------------
    private ref map<int, bool> m_TagSet;
    private ref array<int>     m_TagSyms;     // stabile Reihenfolge fuer Dump

    private bool  m_Ready;
    private bool  m_NotReadyLogged;
    private int   m_MaxDepth;
    private int   m_RejectedCount;
    private float m_WeightBase;
    private float m_WeightPerDepth;
    private int   m_MaxCategoriesOverride;    // <= 0: aus den Einstellungen
    private bool  m_QuietForTest;             // nur S3-Selbsttest, siehe unten

    //--------------------------------------------------------------------------

    void ChefZ_CategoryManager()
    {
        m_Sym        = new array<int>();
        m_Id         = new array<string>();
        m_DisplayKey = new array<string>();
        m_Parent     = new array<int>();
        m_Depth      = new array<int>();
        m_Closures   = new array<ref ChefZ_CategoryClosure>();
        m_Children   = new array<ref array<int>>();
        m_BitBySym   = new map<int, int>();
        m_TagSet     = new map<int, bool>();
        m_TagSyms    = new array<int>();

        m_WeightBase           = DEFAULT_WEIGHT_BASE;
        m_WeightPerDepth       = DEFAULT_WEIGHT_PER_DEPTH;
        m_MaxCategoriesOverride = 0;
        m_QuietForTest          = false;

        ResetState();
    }

    static ChefZ_CategoryManager Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_CategoryManager();
        return s_Instance;
    }

    //==========================================================================
    // Aufbau
    //==========================================================================

    /**
     * Baut Baum, Bitsets und Tag-Menge. Idempotent: ein zweiter Aufruf
     * verwirft den alten Bestand vollstaendig und baut neu.
     *
     * "report" darf null sein - dann gibt es keinen Bericht, aber auch keinen
     * Nullzugriff. Der Aufbau selbst laeuft in jedem Fall durch: 04 §6 kennt
     * keinen Fall, in dem der Manager aufgibt.
     */
    void Build(ChefZ_Registry<ChefZ_CategoryDef> cats,
               ChefZ_Registry<ChefZ_TagDef> tags,
               ChefZ_LoadReport report)
    {
        int startTick = TickCount(0);
        ResetState();

        BuildTags(tags, report);

        if (!cats || cats.Count() == 0)
        {
            // 04 §6, erste Zeile: kein Fehler. IsInCategory liefert immer
            // false, Rezepte mit Kategorie-Slots matchen nie und fallen damit
            // auf Vanilla zurueck; Rezepte mit reinen class-Slots laufen
            // weiter.
            if (report)
                report.AddInfo("Keine Kategorien definiert - der Kategoriebaum bleibt leer. "
                    + "Kategorie-Selektoren matchen dadurch nie; Rezepte mit reinen "
                    + "Klassen-Selektoren sind unberuehrt.");
            m_Ready = true;
            return;
        }

        array<ChefZ_CategoryDef> defs = AdmitCategories(cats, report);
        array<int> parentOf = ResolveParents(defs, report);
        DetectCycles(defs, parentOf, report);
        ComputeDepths(parentOf, report);
        BuildClosures();
        BuildChildren();

        m_Ready = true;

        if (report)
            report.AddInfo("Kategoriebaum: " + GetCategoryCount().ToString() + " Kategorien, " + "Tiefe " + m_MaxDepth.ToString() + ", " + GetTagCount().ToString() + " Tags" + " in " + TickCount(startTick).ToString() + "ms.");

        LogTreeIfDebug();
    }

    //--------------------------------------------------------------------------

    /**
     * Schritt 1: IDs internen und dichte Bitindizes vergeben.
     *
     * Reihenfolge ist Registry.Keys(), also nach ID sortiert (03 §4). Damit
     * ist der Bitindex einer Kategorie auf Client und Server gleich - er wird
     * zwar nie uebertragen, aber gleiche Indizes machen jeden Trace
     * vergleichbar. Und der Deckel trifft bei Ueberlauf auf beiden Seiten
     * dieselben Kategorien.
     *
     * Doppelte IDs koennen hier nicht mehr ankommen: die Registry weist sie ab
     * und der Config Manager meldet sie (04 §6, "Erste gewinnt"). Ein zweiter
     * Mechanismus dafuer waere eine zweite Wahrheit.
     */
    private array<ChefZ_CategoryDef> AdmitCategories(ChefZ_Registry<ChefZ_CategoryDef> cats,
                                                     ChefZ_LoadReport report)
    {
        // Kein "ref": die Registry ist Eigentuemerin der Records, diese Liste
        // ist eine kurzlebige Arbeitssicht.
        array<ChefZ_CategoryDef> defs = new array<ChefZ_CategoryDef>();

        int limit = ResolveLimit();
        array<ChefZ_Sym> keys = cats.Keys();

        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_CategoryDef def = cats.Find(keys.Get(i));
            if (!def)
                continue;

            if (defs.Count() >= limit)
            {
                m_RejectedCount++;
                if (report)
                    report.AddError(def.sourceRef, def.id,
                        "Kategoriedeckel maxCategories=" + limit.ToString() + " erreicht - "
                        + "die Kategorie wurde NICHT aufgenommen. Rezepte, die sie fordern, "
                        + "matchen nie. Deckel in Core.json anheben oder Kategorien "
                        + "zusammenlegen.");
                continue;
            }

            int bit = defs.Count();
            defs.Insert(def);

            m_Sym.Insert(def.sym);
            m_Id.Insert(def.id);
            m_DisplayKey.Insert(ResolveDisplayKey(def, report));
            m_Parent.Insert(NO_INDEX);
            m_Depth.Insert(0);
            m_Closures.Insert(new ChefZ_CategoryClosure());
            m_Children.Insert(new array<int>());

            if (ChefZ_SymbolTable.IsValid(def.sym))
            {
                m_BitBySym.Set(def.sym, bit);
            }
            else if (report)
            {
                // Kann nur passieren, wenn ein Record die COMPILE-Stufe nicht
                // gesehen hat. Der Baumplatz bleibt bestehen, nachschlagbar
                // ist die Kategorie aber nicht.
                report.AddError(def.sourceRef, def.id,
                    "Kategorie hat kein gueltiges Symbol - sie ist im Baum enthalten, aber "
                    + "ueber IsInCategory() nicht abfragbar. Das deutet auf einen Record hin, "
                    + "der die COMPILE-Stufe nicht durchlaufen hat.");
            }
        }

        return defs;
    }

    /**
     * Schritt 2: Elternkanten aufloesen.
     *
     * Unbekanntes parent -> Wurzel + WARN, NICHT verwerfen (04 §6): das
     * Verwerfen risse alle Kinder mit, und ein Tippfehler in einer
     * Elternkategorie legte einen halben Baum still. Der Verlust einer
     * Elternbeziehung macht ein Rezept ENGER, nie falscher - also in Richtung
     * Vanilla.
     */
    private array<int> ResolveParents(notnull array<ChefZ_CategoryDef> defs, ChefZ_LoadReport report)
    {
        map<string, int> byId = new map<string, int>();
        for (int i = 0; i < defs.Count(); i++)
            byId.Set(defs.Get(i).id, i);

        array<int> parentOf = new array<int>();

        for (int k = 0; k < defs.Count(); k++)
        {
            ChefZ_CategoryDef def = defs.Get(k);
            string parentId = def.parent;

            if (parentId == "")
            {
                parentOf.Insert(NO_INDEX);          // Wurzel, voellig normal
                continue;
            }

            int parentIndex;
            if (!byId.Find(parentId, parentIndex))
            {
                parentOf.Insert(NO_INDEX);
                if (report)
                    report.AddWarn(def.sourceRef, def.id,
                        "parent \"" + parentId + "\" ist keine bekannte Kategorie - \""
                        + def.id + "\" wird zur Wurzel. Die Kategorie bleibt nutzbar, erbt "
                        + "aber nichts. Haeufigste Ursache: Tippfehler oder ein Modul, das "
                        + "die Elternkategorie liefern sollte, ist nicht geladen.");
                continue;
            }

            parentOf.Insert(parentIndex);
        }

        return parentOf;
    }

    /**
     * Schritt 3: Zyklen erkennen und AUFLOESEN.
     *
     * Verfahren: Tiefensuche mit Farbmarkierung (04 §4). Da jede Kategorie
     * genau ein Elternteil hat, ist die "Suche" ein Aufstieg entlang einer
     * einzelnen Kette - der Farbstand unterscheidet dabei "liegt im aktuellen
     * Pfad" (grau) von "bereits abgeschlossen" (schwarz). Trifft der Aufstieg
     * auf einen grauen Knoten, ist der Ring genau das Stueck des Pfades ab
     * diesem Knoten.
     *
     * ALLE Kategorien des Zyklus werden verworfen (04 §6) - nicht nur eine.
     * Eine willkuerlich gekappte Kante liesse einen Baum zurueck, den niemand
     * so geschrieben hat, und die Auswahl haenge daran, wo die Suche begann.
     * Kategorien ausserhalb des Zyklus bleiben gueltig.
     *
     * Der Grund fuer die Haerte steht in 04 §6: "IsInCategory darf unter
     * keinen Umstaenden endlos laufen."
     */
    private void DetectCycles(notnull array<ChefZ_CategoryDef> defs, notnull array<int> parentOf,
                              ChefZ_LoadReport report)
    {
        int count = parentOf.Count();

        array<int> colour = new array<int>();
        for (int i = 0; i < count; i++)
            colour.Insert(COLOUR_WHITE);

        array<int> path = new array<int>();

        for (int start = 0; start < count; start++)
        {
            if (colour.Get(start) != COLOUR_WHITE)
                continue;

            path.Clear();

            int node = start;
            while (node != NO_INDEX && colour.Get(node) == COLOUR_WHITE)
            {
                colour.Set(node, COLOUR_GREY);
                path.Insert(node);
                node = parentOf.Get(node);
            }

            if (node != NO_INDEX && colour.Get(node) == COLOUR_GREY)
                DropCycle(defs, parentOf, path, node, report);

            for (int p = 0; p < path.Count(); p++)
                colour.Set(path.Get(p), COLOUR_BLACK);
        }
    }

    /**
     * Verwirft die Mitglieder eines gefundenen Zyklus.
     *
     * "Verwerfen" heisst hier: aus dem Baum genommen (Symbol entfernt, damit
     * GetBitIndex -1 liefert und jeder Selektor auf diese Kategorie nie
     * matcht) und als Wurzel abgehaengt, damit der weitere Aufbau eine
     * garantiert azyklische Struktur sieht.
     *
     * Kinder ausserhalb des Zyklus haengen danach an einer verworfenen
     * Kategorie; ComputeDepths() macht sie zu Wurzeln - dieselbe Behandlung
     * wie ein unbekanntes parent, aus demselben Grund.
     */
    private void DropCycle(notnull array<ChefZ_CategoryDef> defs, notnull array<int> parentOf,
                           notnull array<int> path, int entry, ChefZ_LoadReport report)
    {
        int from = path.Find(entry);
        if (from < 0)
            from = 0;

        string chain = "";
        for (int i = from; i < path.Count(); i++)
        {
            if (chain != "")
                chain = chain + " -> ";
            chain = chain + m_Id.Get(path.Get(i));
        }
        chain = chain + " -> " + m_Id.Get(entry);

        for (int k = from; k < path.Count(); k++)
        {
            int index = path.Get(k);
            ChefZ_CategoryDef def = defs.Get(index);

            parentOf.Set(index, NO_INDEX);
            Drop(index);
            m_RejectedCount++;

            if (report)
                report.AddError(def.sourceRef, def.id,
                    "Zyklus im Kategoriebaum: " + chain + ". ALLE Kategorien des Zyklus werden "
                    + "verworfen; Kategorien ausserhalb bleiben gueltig. Eine zyklische "
                    + "Vererbung ist nicht aufloesbar - genau eine Kante zu kappen waere eine "
                    + "Erfindung, und die Vorfahrenmenge haenge daran, wo die Suche begann.");
        }
    }

    //! Nimmt eine Kategorie aus der Nachschlagestruktur. Der Platz im Array
    //! bleibt bestehen (die Bitindizes duerfen sich nicht verschieben), aber
    //! ohne Symbol ist die Kategorie nicht mehr auffindbar.
    private void Drop(int index)
    {
        int sym = m_Sym.Get(index);
        if (ChefZ_SymbolTable.IsValid(sym))
            m_BitBySym.Remove(sym);
        m_Sym.Set(index, ChefZ_SymbolTable.INVALID);
        m_Parent.Set(index, NO_INDEX);
        m_Depth.Set(index, NO_INDEX);
    }

    private bool IsDropped(int index)
    {
        return m_Depth.Get(index) == NO_INDEX;
    }

    /**
     * Schritt 4a: Tiefen berechnen.
     *
     * ZWEI Durchlaeufe, und die Reihenfolge ist nicht beliebig:
     *
     *   1. Alle Kanten kappen, die auf eine verworfene Kategorie zeigen.
     *   2. Erst danach die Tiefen entlang der bereinigten Ketten zaehlen.
     *
     * In einem einzigen Durchlauf waere das Ergebnis von der Indexreihenfolge
     * abhaengig: haengt A an B und B an einer verworfenen Kategorie, dann
     * zaehlte A das noch nicht gekappte B-zu-Zyklus-Stueck mit und bekaeme
     * eine Tiefe groesser als die von B - eine Reihenfolge, die spaeter die
     * Rezeptauswahl (09) verschoebe.
     *
     * Der Aufstieg laeuft trotz garantierter Azyklizitaet mit einem
     * Schrittzaehler. Er kostet einen Vergleich je Schritt und schuetzt gegen
     * die eine Klasse von Fehlern, die einen Server aufhaengt statt ihn
     * abstuerzen zu lassen: eine Endlosschleife im Boot.
     */
    private void ComputeDepths(notnull array<int> parentOf, ChefZ_LoadReport report)
    {
        int count = parentOf.Count();
        m_MaxDepth = 0;

        // --- Durchlauf 1: Kanten auf verworfene Kategorien kappen -----------
        for (int i = 0; i < count; i++)
        {
            if (IsDropped(i))
                continue;

            int parent = parentOf.Get(i);
            if (parent != NO_INDEX && IsDropped(parent))
            {
                // Elternteil lag in einem Zyklus. Gleiche Behandlung wie ein
                // unbekanntes parent (04 §6): Wurzel, WARN, weiterleben.
                if (report)
                    report.AddWarn("", m_Id.Get(i),
                        "parent \"" + m_Id.Get(parent) + "\" wurde als Teil eines Zyklus "
                        + "verworfen - \"" + m_Id.Get(i) + "\" wird zur Wurzel und erbt nichts.");
                parent = NO_INDEX;
                parentOf.Set(i, NO_INDEX);
            }

            m_Parent.Set(i, parent);
        }

        // --- Durchlauf 2: Tiefen zaehlen ------------------------------------
        for (int k = 0; k < count; k++)
        {
            if (IsDropped(k))
                continue;

            int depth = 0;
            int node  = parentOf.Get(k);
            int guard = 0;
            while (node != NO_INDEX && guard <= count)
            {
                depth++;
                node = parentOf.Get(node);
                guard++;
            }

            if (guard > count)
            {
                // Unerreichbar, solange DetectCycles korrekt ist. Wenn doch:
                // laut melden, Kategorie zur Wurzel machen, weiterlaufen.
                if (report)
                    report.AddError("", m_Id.Get(k),
                        "Elternkette laeuft nach " + guard.ToString() + " Schritten nicht aus - "
                        + "die Kategorie wird zur Wurzel gemacht. Das ist ein Fehler in der "
                        + "Zyklenerkennung des Core und gehoert gemeldet.");
                depth = 0;
                m_Parent.Set(k, NO_INDEX);
                parentOf.Set(k, NO_INDEX);
            }

            m_Depth.Set(k, depth);
            if (depth > m_MaxDepth)
                m_MaxDepth = depth;
        }
    }

    /**
     * Schritt 4b: Vorfahrenbitsets in topologischer Reihenfolge.
     *
     * Topologisch heisst hier: Tiefe aufsteigend. Wenn Knoten i an der Reihe
     * ist, ist das Bitset seines Elternteils fertig - ein OR und ein SetBit
     * genuegen, jedes Bitset wird genau einmal geschrieben. Die Alternative,
     * je Knoten die Elternkette hochzulaufen, waere O(n * Tiefe) und
     * rekursionsanfaellig.
     *
     * Das eigene Bit gehoert dazu: die Closure ist "self-or-ancestor", damit
     * IsInCategory(SAUSAGE) fuer eine Wurst ohne Sonderfall true liefert.
     */
    private void BuildClosures()
    {
        for (int depth = 0; depth <= m_MaxDepth; depth++)
        {
            for (int i = 0; i < m_Depth.Count(); i++)
            {
                if (IsDropped(i))
                    continue;
                if (m_Depth.Get(i) != depth)
                    continue;

                ChefZ_CategoryClosure closure = m_Closures.Get(i);
                closure.Clear();

                int parent = m_Parent.Get(i);
                if (parent != NO_INDEX)
                    closure.OrWith(m_Closures.Get(parent));

                closure.SetBit(i);
            }
        }
    }

    //! Kinderlisten in derselben stabilen Reihenfolge wie die Bitindizes.
    private void BuildChildren()
    {
        for (int i = 0; i < m_Parent.Count(); i++)
        {
            if (IsDropped(i))
                continue;
            int parent = m_Parent.Get(i);
            if (parent != NO_INDEX)
                m_Children.Get(parent).Insert(i);
        }
    }

    /**
     * Schritt 6: Tags internen - flach, ohne jede Vererbung (04 E3).
     *
     * Ein Item mit Tag CHEFZ_SMOKED erfuellt NICHT automatisch
     * CHEFZ_PRESERVED. Wer die Implikation will, deklariert sie explizit ueber
     * ChefZ_StateDef.implies[] - an genau einer Stelle, sichtbar im Review.
     * Implizite Tag-Ketten sind die haeufigste Quelle fuer "warum matcht
     * dieses Rezept?"-Verwirrung.
     *
     * Tags werden hier NIE implizit angelegt (04 §6): ein implizit angelegter
     * Tag matcht nie und waere stiller toter Code.
     */
    private void BuildTags(ChefZ_Registry<ChefZ_TagDef> tags, ChefZ_LoadReport report)
    {
        if (!tags || tags.Count() == 0)
            return;

        array<ChefZ_Sym> keys = tags.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_TagDef def = tags.Find(keys.Get(i));
            if (!def)
                continue;

            if (!ChefZ_SymbolTable.IsValid(def.sym))
            {
                if (report)
                    report.AddError(def.sourceRef, def.id,
                        "Tag hat kein gueltiges Symbol und ist nicht abfragbar. Das deutet auf "
                        + "einen Record hin, der die COMPILE-Stufe nicht durchlaufen hat.");
                continue;
            }

            if (m_TagSet.Contains(def.sym))
                continue;

            m_TagSet.Set(def.sym, true);
            m_TagSyms.Insert(def.sym);
        }
    }

    //--------------------------------------------------------------------------

    /**
     * 04 §6: fehlender displayName ist kein Fehler - die ID wird als
     * Anzeigename benutzt, INFO. Der Stringtable-Validator meldet es ohnehin.
     *
     * Bewusst nur EINE Sammelmeldung statt einer je Kategorie: bei einem
     * Modul, das die Anzeigenamen noch nicht gepflegt hat, waeren das sonst
     * dutzende Zeilen mit derselben Aussage.
     */
    private string ResolveDisplayKey(notnull ChefZ_CategoryDef def, ChefZ_LoadReport report)
    {
        if (def.displayName != "")
            return def.displayName;

        QuietOnce(ChefZ_LogLevel.INFO, "category.displayname.missing",
            "Mindestens eine Kategorie hat keinen \"displayName\" (zuerst \"" + def.id
            + "\") - im UI erscheint die ID. Das ist zulaessig; der Stringtable-Validator "
            + "listet die betroffenen Schluessel vollstaendig auf.");
        return def.id;
    }

    private int ResolveLimit()
    {
        if (m_MaxCategoriesOverride > 0)
            return m_MaxCategoriesOverride;

        ChefZ_ConfigManager cfg = ChefZ_ConfigManager.Get();
        if (cfg)
        {
            ChefZ_CoreSettingsDef settings = cfg.GetSettings();
            if (settings && settings.maxCategories > 0)
                return settings.maxCategories;
        }
        return FALLBACK_MAX_CATEGORIES;
    }

    private void ResetState()
    {
        m_Sym.Clear();
        m_Id.Clear();
        m_DisplayKey.Clear();
        m_Parent.Clear();
        m_Depth.Clear();
        m_Closures.Clear();
        m_Children.Clear();
        m_BitBySym.Clear();
        m_TagSet.Clear();
        m_TagSyms.Clear();

        m_Ready          = false;
        m_NotReadyLogged = false;
        m_MaxDepth       = 0;
        m_RejectedCount  = 0;
    }

    /**
     * Setzt den Manager auf "nicht gebaut" zurueck.
     *
     * Unterschied zu Build(null, null, null), und er ist wichtig: danach ist
     * IsReady() FALSE, und jede Abfrage meldet einmal "vor Build aufgerufen".
     * Das ist der richtige Zustand fuer einen Manager, der nie gebaut wurde -
     * nicht fuer einen, dessen Daten weggefallen sind. Der SAFE_MODE (02 §8)
     * benutzt deshalb bewusst Build() mit leeren Quellen: dort ist "bereit und
     * leer" die wahre Aussage.
     */
    void Reset()
    {
        ResetState();
    }

    //==========================================================================
    // Abfragen - heisser Pfad, O(1)
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    /**
     * Der Kategorietest des Matchers: ein Map-Zugriff auf den Bitindex und ein
     * AND. Die Closure gehoert dem ChefZ_ItemFacts, nicht dem Manager (04 E1)
     * - im kompilierten Selektor steht spaeter der Bitindex direkt, dann
     * entfaellt auch der Map-Zugriff.
     *
     * Unbekanntes oder ungueltiges Symbol -> false, OHNE Log (04 §6): das ist
     * der heisseste Pfad des Mods, und der Fehler wurde beim Laden gemeldet.
     */
    bool IsInCategory(notnull ChefZ_CategoryClosure closure, ChefZ_Sym category)
    {
        int bit = GetBitIndex(category);
        if (bit < 0)
            return false;
        return closure.HasBit(bit);
    }

    //! -1, wenn die Kategorie unbekannt, verworfen oder der Baum nicht gebaut
    //! ist.
    int GetBitIndex(ChefZ_Sym category)
    {
        if (!m_Ready)
        {
            ReportNotReady("GetBitIndex");
            return NO_INDEX;
        }

        int bit;
        if (!m_BitBySym.Find(category, bit))
            return NO_INDEX;
        return bit;
    }

    /**
     * 04 §6: "Abfrage vor Build(): false bzw. -1, einmaliger ERROR. Kein
     * Nullzugriff."
     *
     * Als Feld-Bool und nicht ueber ChefZ_Log.Once(), weil diese Pruefung in
     * jeder Abfrage steht: das Bool kostet einen Vergleich, der Once-Schluessel
     * einen Map-Zugriff mit Stringhash.
     */
    private void ReportNotReady(string what)
    {
        if (m_NotReadyLogged)
            return;
        m_NotReadyLogged = true;

        if (m_QuietForTest)
            return;

        ChefZ_Log.Error(ChefZ_LogChannel.CONFIG,
            "ChefZ_CategoryManager." + what + "() wurde vor Build() aufgerufen - die Antwort ist "
            + "\"unbekannt\". Jede Kategorieabfrage liefert bis zum Aufbau false; Rezepte mit "
            + "Kategorie-Slots matchen solange nicht. Diese Meldung erscheint genau einmal.");
    }

    //==========================================================================
    // Baum
    //==========================================================================

    bool Exists(ChefZ_Sym category)
    {
        return GetBitIndex(category) >= 0;
    }

    //! Elternsymbol. INVALID fuer Wurzeln und fuer unbekannte Kategorien -
    //! beides heisst "kein Elternteil", und der Unterschied ist ueber
    //! Exists() zu haben.
    ChefZ_Sym GetParent(ChefZ_Sym category)
    {
        int bit = GetBitIndex(category);
        if (bit < 0)
            return ChefZ_SymbolTable.INVALID;

        int parent = m_Parent.Get(bit);
        if (parent == NO_INDEX)
            return ChefZ_SymbolTable.INVALID;

        return m_Sym.Get(parent);
    }

    //! Wurzel = 0. -1 fuer unbekannte Kategorien - eine unbekannte Kategorie
    //! als Wurzel auszugeben waere eine Aussage, die niemand geprueft hat.
    int GetDepth(ChefZ_Sym category)
    {
        int bit = GetBitIndex(category);
        if (bit < 0)
            return NO_INDEX;
        return m_Depth.Get(bit);
    }

    /**
     * Kinder einer Kategorie, direkt oder rekursiv.
     *
     * outIds wird angelegt, wenn es null ist, und in jedem Fall geleert - ein
     * Aufrufer, der dasselbe Array mehrfach benutzt, bekommt sonst
     * Ergebnisse zweier Abfragen uebereinander.
     */
    void GetChildren(ChefZ_Sym category, bool recursive, out array<ChefZ_Sym> outIds)
    {
        if (!outIds)
            outIds = new array<ChefZ_Sym>();
        outIds.Clear();

        int bit = GetBitIndex(category);
        if (bit < 0)
            return;

        CollectChildren(bit, recursive, outIds);
    }

    private void CollectChildren(int bit, bool recursive, notnull array<ChefZ_Sym> outIds)
    {
        array<int> children = m_Children.Get(bit);
        for (int i = 0; i < children.Count(); i++)
        {
            int child = children.Get(i);
            outIds.Insert(m_Sym.Get(child));
            if (recursive)
                CollectChildren(child, true, outIds);
        }
    }

    /**
     * Vorfahrenkette, vom direkten Elternteil aufwaerts bis zur Wurzel.
     * OHNE die Kategorie selbst - "Vorfahren" ist woertlich gemeint; wer
     * self-or-ancestor braucht, nimmt die Closure.
     */
    void GetAncestors(ChefZ_Sym category, out array<ChefZ_Sym> outChain)
    {
        if (!outChain)
            outChain = new array<ChefZ_Sym>();
        outChain.Clear();

        int bit = GetBitIndex(category);
        if (bit < 0)
            return;

        int guard = 0;
        int node = m_Parent.Get(bit);
        while (node != NO_INDEX && guard <= m_Parent.Count())
        {
            outChain.Insert(m_Sym.Get(node));
            node = m_Parent.Get(node);
            guard++;
        }
    }

    /**
     * Baut aus den DIREKT deklarierten Kategorien einer Zutat die fertige
     * Vorfahren-Closure (05 §4, Schritt 3). Genau einmal je Klasse beim Boot -
     * danach kostet jeder Kategorietest einen Bit-Test.
     *
     * Unbekannte Kategorien werden hier still uebergangen: die Meldung gehoert
     * zur Referenzpruefung des Ingredient Managers, die den Record und seine
     * Quelle kennt. Zweimal zu melden hiesse, denselben Tippfehler zweimal im
     * Bericht zu haben.
     */
    void BuildClosure(notnull array<ChefZ_Sym> directCategories,
                      out ChefZ_CategoryClosure outClosure)
    {
        if (!outClosure)
            outClosure = new ChefZ_CategoryClosure();
        outClosure.Clear();

        if (!m_Ready)
        {
            ReportNotReady("BuildClosure");
            return;
        }

        for (int i = 0; i < directCategories.Count(); i++)
        {
            int bit;
            if (!m_BitBySym.Find(directCategories.Get(i), bit))
                continue;
            outClosure.OrWith(m_Closures.Get(bit));
        }
    }

    //==========================================================================
    // Spezifitaet (04 E4, 09 §4.1)
    //==========================================================================

    /**
     * 1.0 + 0.5 * Tiefe. Eine Unterkategorie ist damit automatisch
     * spezifischer als ihre Elternkategorie, ohne dass irgendein Autor eine
     * Zahl pflegt - die Alternative, ein handgepflegtes "specificity"-Feld,
     * driftet zuverlaessig, sobald mehrere Content-Agenten parallel arbeiten.
     *
     * Unbekannte Kategorie -> 0.0. Ein Selektor auf eine unbekannte Kategorie
     * matcht ohnehin nie; ihm ein Gewicht zu geben wuerde ein totes Rezept in
     * der Rangfolge nach oben schieben.
     */
    float GetSpecificityWeight(ChefZ_Sym category)
    {
        int depth = GetDepth(category);
        if (depth < 0)
            return 0.0;
        return m_WeightBase + m_WeightPerDepth * depth;
    }

    /**
     * Ueberschreibt die beiden Gewichte.
     *
     * Vorgesehener Aufrufer ist S8 (09): dort sind wCategoryBase und
     * wCategoryPerDepth Teil der konfigurierbaren Prioritaetsgewichte. Bis
     * dahin gelten die Defaults 1.0 / 0.5 aus 04 E4. Negative Werte werden
     * abgelehnt - sie wuerden die Ordnung umdrehen und eine Unterkategorie
     * unspezifischer machen als ihre Elternkategorie.
     */
    void SetSpecificityWeights(float weightBase, float weightPerDepth)
    {
        if (weightBase < 0.0 || weightPerDepth < 0.0)
        {
            QuietOnce(ChefZ_LogLevel.WARN, "category.weights.negative",
                "Negative Spezifitaetsgewichte (" + weightBase.ToString() + " / " + weightPerDepth.ToString()
                + ") abgelehnt - die Rangfolge waere umgekehrt und eine Unterkategorie "
                + "unspezifischer als ihre Elternkategorie. Es gelten weiterhin "
                + m_WeightBase.ToString() + " / " + m_WeightPerDepth.ToString() + ".");
            return;
        }
        m_WeightBase     = weightBase;
        m_WeightPerDepth = weightPerDepth;
    }

    //==========================================================================
    // Tags, flach
    //==========================================================================

    bool TagExists(ChefZ_Sym tag)
    {
        if (!ChefZ_SymbolTable.IsValid(tag))
            return false;
        return m_TagSet.Contains(tag);
    }

    int GetTagCount()
    {
        return m_TagSyms.Count();
    }

    //==========================================================================
    // Anzeige und Diagnose
    //==========================================================================

    //! Stringtable-Schluessel der Kategorie, sonst ihre ID (04 §6).
    //! Leerstring nur fuer unbekannte Kategorien.
    string GetDisplayKey(ChefZ_Sym category)
    {
        int bit = GetBitIndex(category);
        if (bit < 0)
            return "";
        return m_DisplayKey.Get(bit);
    }

    //! Anzahl der tatsaechlich nutzbaren Kategorien - verworfene (Zyklus,
    //! Deckel) zaehlen nicht mit.
    int GetCategoryCount()
    {
        return m_BitBySym.Count();
    }

    int GetMaxDepth()
    {
        return m_MaxDepth;
    }

    //! Wie viele Kategorien beim Aufbau verworfen wurden (Zyklus oder Deckel).
    int GetRejectedCount()
    {
        return m_RejectedCount;
    }

    /**
     * Baum als Textzeilen, Wurzeln zuerst, Kinder eingerueckt.
     * Fuer den Ladebericht, das Admin-Werkzeug und den Selbsttest.
     */
    void DumpTree(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Kategoriebaum: " + GetCategoryCount().ToString() + " Kategorien" + ", Tiefe " + m_MaxDepth.ToString() + ", verworfen " + m_RejectedCount.ToString() + ", Tags " + GetTagCount().ToString()
            + ", ready=" + m_Ready.ToString());

        for (int i = 0; i < m_Parent.Count(); i++)
        {
            if (IsDropped(i))
                continue;
            if (m_Parent.Get(i) != NO_INDEX)
                continue;
            DumpSubtree(i, "  ", outLines);
        }

        if (m_TagSyms.Count() > 0)
        {
            string tagLine = "  Tags:";
            for (int t = 0; t < m_TagSyms.Count(); t++)
                tagLine = tagLine + " " + ChefZ_SymbolTable.Name(m_TagSyms.Get(t));
            outLines.Insert(tagLine);
        }
    }

    private void DumpSubtree(int bit, string indent, notnull array<string> outLines)
    {
        // Gewicht direkt aus der Tiefe und nicht ueber GetSpecificityWeight():
        // der Auszug muss auch dann laufen, wenn der Baum noch nicht bereit
        // ist, ohne dabei einen Fehler ueber "Abfrage vor Build" zu melden.
        int depth = m_Depth.Get(bit);
        float weight = m_WeightBase + m_WeightPerDepth * depth;

        outLines.Insert(indent + m_Id.Get(bit)
            + "  bit=" + bit.ToString()
            + " tiefe=" + depth.ToString()
            + " gewicht=" + weight.ToString()
            + " closure=" + m_Closures.Get(bit).ToDebugString());

        array<int> children = m_Children.Get(bit);
        for (int i = 0; i < children.Count(); i++)
            DumpSubtree(children.Get(i), indent + "    ", outLines);
    }

    //! 18 §2: Der Baum landet nur im Log, wenn Kanal CONFIG auf DEBUG steht -
    //! bei 40 Kategorien sind das 40 Zeilen, die im Normalbetrieb niemand
    //! sehen will. Die Zeichenketten entstehen erst hinter der Wache.
    private void LogTreeIfDebug()
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        DumpTree(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.CONFIG, lines);
    }

    //--------------------------------------------------------------------------

    /**
     * Einmalmeldung, die der Selbsttest stummschalten kann.
     *
     * Ohne diesen Umweg wuerde der Test die Once-Schluessel des Betriebs
     * verbrauchen: eine echte Kategorie ohne displayName bliebe danach
     * unbemerkt, weil der Schluessel bereits gefeuert hat.
     */
    private void QuietOnce(int level, string key, string message)
    {
        if (m_QuietForTest)
            return;
        ChefZ_Log.Once(level, ChefZ_LogChannel.CONFIG, key, message);
    }

    /**
     * Nur fuer den Selbsttest (S3): unterdrueckt Meldungen dieses Managers.
     *
     * Noetig, weil der Test absichtlich Fehlerfaelle durchspielt (Zyklus,
     * Deckel, Abfrage vor Build). Wuerden sie ins Log laufen, staende im RPT
     * ein Fehler, den es nicht gibt - und ChefZ_Log.GetErrorCount() speist die
     * Safe-Mode-Schwelle (18 §4). Der Test wuerde den Server also in den
     * SAFE_MODE treiben koennen.
     */
    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }

    /**
     * Nur fuer den Selbsttest (S3): setzt den Deckel, ohne die Einstellungen
     * zu befragen. Damit bleibt der Test unabhaengig davon, ob und wie der
     * Config Manager bereits geladen hat. Wert <= 0 stellt das normale
     * Verhalten wieder her.
     */
    void SetMaxCategoriesForTest(int max)
    {
        m_MaxCategoriesOverride = max;
    }
}
