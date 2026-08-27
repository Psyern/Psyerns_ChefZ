//==============================================================================
// ChefZ_HandcraftBridge - was zwischen den ChefZ-Daten und Vanillas
// Rezeptliste steht.
//
// Entwurf: 11 §4 (RegisterRecipies, woertlich), 11 §5 (BOOT-Fluss), 11 §7
// (Fehlerverhalten, "Vanilla-Crafting vollstaendig unberuehrt"), 11 E2, 11 E3,
// 00 §5 Tabelle Zeile 2 ("super zuerst; danach ausschliesslich additive
// RegisterRecipe(...); nie UnregisterRecipe"), 19 S15.
//
// ---------------------------------------------------------------------------
// Die drei Aufgaben
// ---------------------------------------------------------------------------
//   1. Selektoren in KLASSENNAMEN ausrollen. Vanillas Rezeptliste kennt nur
//      Klassennamen; ein ChefZ-Selektor kennt Kategorien, Zustaende und
//      Wertebereiche. Die Uebersetzung ist bewusst eine UEBER-Naeherung -
//      siehe unten.
//   2. Je HANDCRAFT-Transform ein ChefZ_GenericCraftRecipe bauen und
//      REGISTRIEREN. Additiv, in fester Reihenfolge, nie loeschend.
//   3. Den Zeitpunkt richtig treffen. Das ist der unangenehmste Teil und
//      bekommt einen eigenen Abschnitt.
//
// ---------------------------------------------------------------------------
// ZEITPUNKT: Vanilla registriert, BEVOR ChefZ geladen hat
// ---------------------------------------------------------------------------
// 11 §5 zeichnet den BOOT-Fluss als "erst ProcessingManager.Build(), dann
// PluginRecipesManagerBase.RegisterRecipies()". In der Engine ist es
// umgekehrt, und das ist keine Auslegungsfrage:
//
//     MissionBase()                 <- Konstruktor
//       PluginManagerInit()
//         PluginsInit()
//           new PluginRecipesManager()
//             CreateAllRecipes() -> RegisterRecipies()   <- HIER
//             GenerateRecipeCache()
//     ...
//     MissionServer.OnInit()
//       ChefZ_CoreEntry.BootServer() -> ChefZ_ConfigManager.LoadAll()  <- ERST HIER
//
// Zum Zeitpunkt von RegisterRecipies() gibt es also weder Prozesse noch
// Transforms. Ein Aufbau dort waere ein Aufbau aus Nichts.
//
// Geloest wird das mit ZWEI Einstiegspunkten in dieselbe Methode:
//
//   Install(mgr)         wird aus RegisterRecipies() gerufen. Ist der
//                        ChefZ_ProcessingManager noch nicht bereit, tut sie
//                        NICHTS und merkt sich das auch nicht als erledigt.
//   InstallDeferred()    wird von ChefZ_Boot gerufen, unmittelbar nachdem der
//                        Bestand steht. Sie holt den Aufbau nach und laesst
//                        Vanilla anschliessend seinen Rezeptcache neu bauen -
//                        ueber PluginRecipesManager.CallbackGenerateCache(),
//                        also ueber eine oeffentliche Vanillamethode, die
//                        genau dafuer da ist.
//
// Der Cache MUSS neu gebaut werden: PluginRecipesManager.GenerateRecipeCache()
// bildet "Itemklasse -> Rezept-IDs" und laeuft im Konstruktor genau einmal.
// Ohne Neuaufbau kennt der Cache die ChefZ-Rezepte nicht, und dann findet
// GetValidRecipes() sie nie - registriert waeren sie trotzdem.
//
// Beim zweiten Missionsstart in demselben Prozess (falls es ihn je gibt)
// greift der erste Weg: der ChefZ-Bestand steht dann bereits, Install() baut
// sofort, und Vanillas eigener GenerateRecipeCache()-Aufruf im Konstruktor
// sieht die Rezepte von selbst. Deshalb merkt sich diese Klasse, in WELCHE
// Plugininstanz sie eingetragen hat, und nicht bloss DASS sie es getan hat.
//
// ---------------------------------------------------------------------------
// Warum die Klassenliste eine UEBER-Naeherung ist
// ---------------------------------------------------------------------------
// Ein Selektor prueft auch DYNAMISCHE Fakten - Zustand, Frische, Stufe,
// Temperatur, Menge. Keiner davon steht an einer Klasse; alle stehen am
// einzelnen Item. Vanillas Zutatenliste dagegen wird EINMAL beim Start
// aufgebaut und danach nur noch ueber Klassennamen abgefragt.
//
// Die Liste enthaelt deshalb jede Klasse, die MOEGLICHERWEISE passt: bei
// einem dynamischen Blatt lautet die Antwort "koennte". Entschieden wird
// danach in ChefZ_GenericCraftRecipe.CanDo(), mit dem echten Matcher auf
// echten Fakten.
//
// Die Richtung ist die einzig vertretbare. Eine zu WEITE Liste kostet ein
// paar Eintraege in Vanillas Rezeptcache und eine Craftaktion, die kurz
// erscheint und dann nicht angeboten wird. Eine zu ENGE Liste hiesse: ein
// Rezept, das der Spieler nie zu sehen bekommt, obwohl seine Daten stimmen -
// und niemand koennte sagen, warum.
//
// ---------------------------------------------------------------------------
// ID-DRIFT zwischen Client und Server - offen benannt
// ---------------------------------------------------------------------------
// Vanillas Craftaktion uebertraegt die Rezept-ID, und die ist eine POSITION
// in m_RecipeList (PluginRecipesManager.RegisterRecipe). Client und Server
// muessen deshalb dieselben Rezepte in derselben Reihenfolge registrieren.
//
// Das ist gegeben, solange beide Seiten dieselben Transformdaten sehen. Genau
// das ist NICHT garantiert: der Server liest zusaetzlich Rang 3
// ($profile:ChefZ/-Overlays, 02 §6), der Client nur Rang 1 und 2. Ein
// HANDCRAFT-Transform, den nur ein Overlay einfuehrt, verschiebt alle
// nachfolgenden ChefZ-Rezept-IDs des Servers gegen die des Clients.
//
// Zwei Dinge daempfen das, und keines beseitigt es:
//   - registriert wird in ID-Reihenfolge, nicht in Ladereihenfolge. Ein
//     Overlay, das einen bestehenden Transform nur AENDERT, verschiebt nichts.
//   - der Server prueft jedes Rezept beim Ausfuehren erneut (CheckRecipe ->
//     CanDo -> Matcher). Eine verschobene ID landet fast immer auf einem
//     Rezept, dessen Bindung fehlschlaegt: es geschieht dann NICHTS.
//
// Damit ein Betreiber es trotzdem SEHEN kann, meldet diese Klasse beim Start
// eine Kennsumme ueber die registrierten Transform-IDs. Stimmen die Zeilen im
// Client- und im Server-RPT nicht ueberein, ist die Ursache genau dieser Fall.
//
// ---------------------------------------------------------------------------
// ABHAENGIG VON OF-10 - ebenfalls offen benannt
// ---------------------------------------------------------------------------
// Transforms liegen in JSON. Ob eine JSON-Datei aus einem PBO clientseitig
// lesbar ist, ist eine offene Messfrage (OF-10, siehe auch Kopf von
// ChefZ_ActionProcessAtStation).
//
// Fuer Stationen ist die Antwort nicht kritisch: dort entscheidet der Server,
// und der Client zeigt im Zweifel eine Aktion, die abgelehnt wird. HIER ist
// sie es sehr wohl. Vanillas Craftaktion entsteht CLIENTSEITIG aus
// PluginRecipesManager.GetValidRecipes(); ohne Transformdaten registriert der
// Client kein Rezept, und dann erscheint die Aktion NIE - unabhaengig davon,
// was der Server weiss.
//
// Es gibt dagegen kein Mittel im Code. Sollte die Messung ergeben, dass der
// Client die JSON-Dateien nicht lesen kann, muessen HANDCRAFT-Transforms in
// die Game-Config wandern (Rang 1, 02 §2) - eine Datenentscheidung, keine
// Codeaenderung. Die Kennsumme oben zeigt den Fall unmittelbar an: sie waere
// clientseitig 0.
//
// KEIN CONTENT: kein Prozess, keine Zutat, kein Werkzeug wird hier benannt.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_HandcraftBridge
{
    //! In WELCHE Plugininstanz eingetragen wurde. Schwacher Zeiger: das
    //! Plugin gehoert dem PluginManager, und ein starker Halter hier haette
    //! es ueber das Missionsende hinaus am Leben gehalten.
    private static PluginRecipesManagerBase s_InstalledInto;

    private static int s_Registered;
    private static int s_Rejected;
    private static int s_Fingerprint;

    //! Die registrierten Transform-IDs, in Registrierungsreihenfolge. Nur
    //! Diagnose - sie beantwortet die Frage "welches Rezept ist ChefZ-Rezept
    //! Nummer drei", und die stellt sich genau dann, wenn etwas driftet.
    private static ref array<string> s_Order;

    //==========================================================================
    // Einstiegspunkte
    //==========================================================================

    /**
     * Aufbau und Registrierung - aus RegisterRecipies() oder nachtraeglich.
     *
     * @return Zahl der neu registrierten Rezepte. 0 heisst entweder "es gibt
     *         keine HANDCRAFT-Transforms" oder "die Daten stehen noch nicht".
     *         Beides ist kein Fehler.
     */
    static int Install(PluginRecipesManagerBase mgr)
    {
        if (!mgr)
            return 0;

        if (s_InstalledInto == mgr)
            return 0;                       // diese Instanz hat bereits alles

        ChefZ_ProcessingManager pm = ChefZ_ProcessingManager.Get();
        if (!pm.IsReady())
        {
            // Der Normalfall beim ersten Start - siehe Abschnitt ZEITPUNKT.
            // Ausdruecklich NICHT als erledigt vermerken: InstallDeferred()
            // holt es nach.
            ChefZ_Log.Once(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.PROCESS,
                "handcraft.notready",
                "RegisterRecipies() lief, bevor der ChefZ-Bestand stand. Die "
                + "Handwerksrezepte werden nach dem Laden nachgetragen. Vanillas "
                + "Rezepte sind davon unberuehrt.");
            return 0;
        }

        s_InstalledInto = mgr;
        s_Registered    = 0;
        s_Rejected      = 0;
        s_Fingerprint   = 0;
        s_Order         = new array<string>();

        array<ChefZ_CompiledProcess> processes = new array<ChefZ_CompiledProcess>();
        pm.GetProcessesForExec(ChefZ_ProcessExec.HANDCRAFT, processes);

        if (processes.Count() == 0)
            return 0;                       // kein Handwerksprozess deklariert

        // Prozesse und Transforms zu Paaren ausrollen und in ID-Reihenfolge
        // bringen. Die Reihenfolge ist KEINE Kosmetik - siehe Abschnitt
        // ID-DRIFT.
        array<ChefZ_CompiledProcess>   pairProc = new array<ChefZ_CompiledProcess>();
        array<ChefZ_CompiledTransform> pairTr   = new array<ChefZ_CompiledTransform>();

        array<ChefZ_CompiledTransform> list = new array<ChefZ_CompiledTransform>();
        for (int p = 0; p < processes.Count(); p++)
        {
            ChefZ_CompiledProcess proc = processes.Get(p);
            pm.GetTransformsForProcess(proc.processSym, list);

            for (int t = 0; t < list.Count(); t++)
                InsertByIdOrder(pairProc, pairTr, proc, list.Get(t));
        }

        for (int i = 0; i < pairProc.Count(); i++)
            RegisterOne(mgr, pairProc.Get(i), pairTr.Get(i));

        Report();
        return s_Registered;
    }

    /**
     * Den Aufbau nachholen, nachdem der Bestand steht - und Vanillas
     * Rezeptcache neu bauen lassen.
     *
     * Wird von ChefZ_Boot gerufen, auf BEIDEN Seiten. Der Client braucht die
     * Rezepte fuer die Anzeige der Craftaktion (ActionWorldCraft fragt
     * GetValidRecipes clientseitig); entschieden wird trotzdem nur auf dem
     * Server (00 §5).
     *
     * Kostet einen vollstaendigen Durchlauf durch CfgVehicles, CfgWeapons und
     * CfgMagazines - genau denselben, den Vanilla beim Start ohnehin einmal
     * macht. Er faellt einmal je Missionsstart an und nur dann, wenn
     * tatsaechlich etwas hinzugekommen ist.
     */
    static void InstallDeferred()
    {
        PluginRecipesManager plugin;
        if (!Class.CastTo(plugin, GetPlugin(PluginRecipesManager)))
        {
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PROCESS,
                "handcraft.noplugin",
                "PluginRecipesManager ist nicht erreichbar. Handwerksrezepte werden "
                + "nicht registriert; Vanilla-Crafting ist davon unberuehrt.");
            return;
        }

        if (Install(plugin) <= 0)
            return;

        // Oeffentliche Vanillamethode; sie ruft GenerateRecipeCache(). Ohne
        // diesen Aufruf waeren die Rezepte registriert, aber ueber den Cache
        // nicht auffindbar - siehe Abschnitt ZEITPUNKT.
        plugin.CallbackGenerateCache();
    }

    //==========================================================================
    // Ein einzelnes Rezept
    //==========================================================================

    private static void RegisterOne(notnull PluginRecipesManagerBase mgr,
                                    notnull ChefZ_CompiledProcess proc,
                                    notnull ChefZ_CompiledTransform tr)
    {
        /**
         * stationsAllowed hat bei HANDCRAFT keine Wirkung - es hat eine
         * SCHAEDLICHE Wirkung.
         *
         * Ein Handwerksschritt laeuft ohne Station; der Kontext traegt
         * stationClass = INVALID. Ein Transform mit stationsAllowed steht im
         * Index aber ausschliesslich unter dem kombinierten Schluessel
         * (Prozess, Station) und wird ohne Station nie gefunden. Das Rezept
         * erschiene in der Liste und koennte nie ausloesen.
         */
        if (tr.stationsAllowed.Count() > 0)
        {
            Reject(tr, "der Transform nennt stationsAllowed, sein Prozess ist aber "
                + "HANDCRAFT. Ein Handwerksschritt laeuft ohne Station - der Transform "
                + "koennte nie gebunden werden. Entweder stationsAllowed streichen oder "
                + "den Prozess auf STATION_ACTION umstellen.");
            return;
        }

        //--- Klassenlisten erheben --------------------------------------------
        array<ref array<string>> inputClasses = new array<ref array<string>>();
        for (int s = 0; s < tr.inputs.Count(); s++)
        {
            ChefZ_CompiledSlot slot = tr.inputs.Get(s);
            array<string> classes = new array<string>();
            if (slot)
                CollectSelectorClasses(slot.selector, classes);
            inputClasses.Insert(classes);
        }

        array<string> toolClasses = new array<string>();
        CollectToolClasses(proc, toolClasses);

        //--- Bauen -------------------------------------------------------------
        ChefZ_GenericCraftRecipe recipe = new ChefZ_GenericCraftRecipe();

        string err;
        if (!recipe.InitFromDef(proc, tr, inputClasses, toolClasses, err))
        {
            Reject(tr, err);
            return;
        }

        //--- Registrieren: ADDITIV, nie loeschend -----------------------------
        mgr.ChefZ_RegisterGeneratedRecipe(recipe);

        s_Registered++;
        s_Order.Insert(tr.id);

        // Ordinale Kennsumme ueber die IDs, in Registrierungsreihenfolge.
        // Reihenfolge UND Bestand gehen ein - genau die beiden Dinge, die
        // zwischen Client und Server uebereinstimmen muessen.
        s_Fingerprint = s_Fingerprint * 31 + tr.id.Hash();

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS,
                "Handwerksrezept #" + (s_Registered - 1).ToString() + ": "
                + recipe.ChefZ_ToDebugString());
        }
    }

    /**
     * 11 §7 in dieser Datei: der Transform wird abgewiesen, der Rest laeuft.
     *
     * ERROR und nicht WARN - dieselbe Schwere, die 11 §7 der 2-Eingaenge-
     * Grenze gibt: "Nicht WARN: es liesse sich stumm nicht registrieren."
     * Genau das ist hier der Fall, und ein Content-Autor muss es im RPT
     * finden.
     */
    private static void Reject(notnull ChefZ_CompiledTransform tr, string why)
    {
        s_Rejected++;

        ChefZ_Log.Error(ChefZ_LogChannel.PROCESS,
            "HANDCRAFT " + tr.id + " (" + tr.sourceRef + ") wird nicht als Craftrezept "
            + "registriert: " + why + " Die uebrigen Transforms sind davon unberuehrt, "
            + "Vanilla-Crafting ebenfalls.");
    }

    //==========================================================================
    // Selektor -> Klassennamen (siehe Dateikopf)
    //==========================================================================

    /**
     * Alle Klassen, die auf diesen Selektor MOEGLICHERWEISE passen.
     *
     * Zwei Quellen, und beide werden gebraucht:
     *
     *   1. der Zutatenbestand (05). Jede bekannte Zutatenklasse wird gegen
     *      die STATISCHEN Blaetter des Selektors gehalten.
     *   2. die Klassen, die der Selektor SELBST nennt. Eine {"class":"X"}
     *      muss auch dann in der Liste stehen, wenn X gar keine deklarierte
     *      ChefZ-Zutat ist - etwa ein Vanilla-Item, das nur als Eingang
     *      dient.
     *
     * Das Ergebnis ist aufsteigend sortiert und duplikatfrei. Sortiert, weil
     * Vanillas Rezept-IDs positionell sind und zwei Seiten dieselbe Liste
     * erzeugen muessen (siehe Dateikopf, ID-DRIFT); duplikatfrei, weil
     * derselbe Klassenname zweimal in m_Ingredients Vanillas Cache zweimal
     * denselben Eintrag kostet.
     */
    static void CollectSelectorClasses(ChefZ_CompiledSelector selector,
                                       notnull array<string> outClasses)
    {
        outClasses.Clear();

        if (!selector)
            return;

        ChefZ_IngredientManager ing = ChefZ_IngredientManager.Get();
        int known = ing.GetKnownCount();

        for (int i = 0; i < known; i++)
        {
            ChefZ_IngredientInfo info = ing.GetAt(i);
            if (!info)
                continue;
            if (!ChefZ_SymbolTable.IsValid(info.classSym))
                continue;
            if (!MayMatch(selector, info))
                continue;

            AddUnique(outClasses, ChefZ_SymbolTable.Name(info.classSym));
        }

        HarvestNamedClasses(selector, outClasses);

        ChefZ_StringOrder.SortAscending(outClasses);
    }

    /**
     * Koennte ein Item DIESER Klasse den Selektor bestehen?
     *
     * STATISCH entscheidbar sind CLASS, CATEGORY und TAG - sie stehen in der
     * Zutatendeklaration und aendern sich am einzelnen Item nie. Alles
     * andere - Zustand, Vanillastufe, Fluessigkeit, Wertebereiche,
     * Qualitaetsschwelle - ist dynamisch und wird hier als "koennte" gewertet.
     *
     * Ausdruecklich NICHT ausgewertet werden ranges und acceptedQualities des
     * Knotens. Sie sind der Grund, warum diese Methode TestStructure() nicht
     * einfach aufruft: Test() wuerde sie mitpruefen, und zwar gegen die
     * Vorgabewerte eines leeren ChefZ_ItemFacts. Das waere keine Naeherung,
     * sondern eine falsche Antwort.
     */
    private static bool MayMatch(ChefZ_CompiledSelector sel,
                                 notnull ChefZ_IngredientInfo info)
    {
        if (!sel)
            return true;

        int i;

        switch (sel.op)
        {
            case ChefZ_SelectorOp.CLASS:
                return info.classSym == sel.sym;

            case ChefZ_SelectorOp.CATEGORY:
                if (!info.closure)
                    return false;
                return info.closure.HasBit(sel.categoryBitIndex);

            case ChefZ_SelectorOp.TAG:
                return info.HasTag(sel.sym);

            case ChefZ_SelectorOp.ANY_OF:
                if (!sel.children)
                    return false;
                for (i = 0; i < sel.children.Count(); i++)
                {
                    if (MayMatch(sel.children.Get(i), info))
                        return true;
                }
                return false;

            case ChefZ_SelectorOp.ALL_OF:
                if (!sel.children)
                    return false;
                for (i = 0; i < sel.children.Count(); i++)
                {
                    if (!MayMatch(sel.children.Get(i), info))
                        return false;
                }
                return true;

            case ChefZ_SelectorOp.NOT:
                /**
                 * Eine Verneinung darf nur dann ausschliessen, wenn das
                 * verneinte Blatt STATISCH ist. {"not":{"state":"RAW"}}
                 * schliesst keine Klasse aus - jedes Item dieser Klasse kann
                 * roh sein oder nicht. {"not":{"category":"MEAT"}} schliesst
                 * sehr wohl aus.
                 */
                if (sel.negated && IsStatic(sel.negated))
                    return !MayMatch(sel.negated, info);
                return true;
        }

        // TRUE_OP, STATE, VANILLA_STAGE, LIQUID und alles Unbekannte:
        // dynamisch oder unentscheidbar -> "koennte".
        return true;
    }

    //! Laesst sich dieser Teilbaum allein aus der Klassendeklaration
    //! beantworten? Nur dann darf eine Verneinung darueber ausschliessen.
    private static bool IsStatic(ChefZ_CompiledSelector sel)
    {
        if (!sel)
            return false;

        int i;

        switch (sel.op)
        {
            case ChefZ_SelectorOp.CLASS:
            case ChefZ_SelectorOp.CATEGORY:
            case ChefZ_SelectorOp.TAG:
                // Ein Wertebereich oder eine Qualitaetsschwelle AN DIESEM
                // Knoten macht ihn dynamisch - auch wenn das Blatt es nicht
                // ist.
                if (sel.ranges && sel.ranges.Count() > 0)
                    return false;
                if (sel.acceptedQualities && sel.acceptedQualities.Count() > 0)
                    return false;
                return true;

            case ChefZ_SelectorOp.ANY_OF:
            case ChefZ_SelectorOp.ALL_OF:
                if (!sel.children)
                    return false;
                for (i = 0; i < sel.children.Count(); i++)
                {
                    if (!IsStatic(sel.children.Get(i)))
                        return false;
                }
                return true;

            case ChefZ_SelectorOp.NOT:
                return IsStatic(sel.negated);
        }

        return false;
    }

    /**
     * Die Klassennamen, die der Selektor ausdruecklich nennt.
     *
     * Nur in ZUSTIMMENDER Stellung: was unter einem "not" steht, ist gerade
     * das, was NICHT gemeint ist, und gehoerte in der Zutatenliste an die
     * falsche Stelle.
     */
    private static void HarvestNamedClasses(ChefZ_CompiledSelector sel,
                                            notnull array<string> outClasses)
    {
        if (!sel)
            return;

        if (sel.op == ChefZ_SelectorOp.CLASS)
        {
            if (ChefZ_SymbolTable.IsValid(sel.sym))
                AddUnique(outClasses, ChefZ_SymbolTable.Name(sel.sym));
            return;
        }

        if (sel.op == ChefZ_SelectorOp.NOT)
            return;

        if (!sel.children)
            return;

        for (int i = 0; i < sel.children.Count(); i++)
            HarvestNamedClasses(sel.children.Get(i), outClasses);
    }

    /**
     * Alle Klassen, die eine der geforderten Werkzeuggruppen bedienen.
     *
     * ODER ueber die Gruppen, genau wie ChefZ_CompiledProcess.HasTools(): EINE
     * genuegt. Die Liste ist deshalb die VEREINIGUNG - jedes Werkzeug, das
     * irgendeine der Gruppen bedient, gehoert auf den Werkzeugplatz.
     *
     * Aufsteigend sortiert, aus demselben Grund wie oben.
     */
    static void CollectToolClasses(notnull ChefZ_CompiledProcess proc,
                                   notnull array<string> outClasses)
    {
        outClasses.Clear();

        ChefZ_ToolRegistry tools = ChefZ_ToolRegistry.Get();
        if (!tools.IsReady())
            return;

        array<ChefZ_Sym> classes = new array<ChefZ_Sym>();

        for (int g = 0; g < proc.toolGroups.Count(); g++)
        {
            tools.GetClassesInGroup(proc.toolGroups.Get(g), classes);

            for (int c = 0; c < classes.Count(); c++)
                AddUnique(outClasses, ChefZ_SymbolTable.Name(classes.Get(c)));
        }

        ChefZ_StringOrder.SortAscending(outClasses);
    }

    private static void AddUnique(notnull array<string> list, string value)
    {
        if (value == "")
            return;
        if (list.Find(value) >= 0)
            return;
        list.Insert(value);
    }

    //==========================================================================
    // Paare in ID-Reihenfolge (siehe Dateikopf, ID-DRIFT)
    //==========================================================================

    /**
     * Einfuegesortierung ueber zwei parallele Listen.
     *
     * Ordinal ueber ChefZ_StringOrder und nicht ueber array.Sort(): die
     * Reihenfolge muss auf Client und Server bitgenau dieselbe sein, und
     * array.Sort() ist mit "depends on underlaying type" dokumentiert - das
     * ist keine Zusage (03 §4, Kopf von ChefZ_StringOrder).
     *
     * Zwei Transforms mit derselben ID kann es nicht geben; die Registry des
     * Config Managers ist nach ID eindeutig.
     */
    private static void InsertByIdOrder(notnull array<ChefZ_CompiledProcess> procs,
                                        notnull array<ChefZ_CompiledTransform> trs,
                                        notnull ChefZ_CompiledProcess proc,
                                        ChefZ_CompiledTransform tr)
    {
        if (!tr)
            return;

        int at = trs.Count();
        while (at > 0 && ChefZ_StringOrder.Less(tr.id, trs.Get(at - 1).id))
            at--;

        if (at >= trs.Count())
        {
            procs.Insert(proc);
            trs.Insert(tr);
            return;
        }

        procs.InsertAt(proc, at);
        trs.InsertAt(tr, at);
    }

    //==========================================================================
    // Diagnose (18)
    //==========================================================================

    /**
     * Eine Zeile ins RPT, an der Stufenpruefung vorbei.
     *
     * Sie steht dort aus einem einzigen Grund: die Kennsumme ist auf Client
     * und Server zu vergleichen, und ein Betreiber, der das tun muss, hat
     * keine Debugstufe eingeschaltet (18 §4, dieselbe Begruendung wie bei
     * ChefZ_Boot.ReportState).
     */
    private static void Report()
    {
        if (s_Registered == 0 && s_Rejected == 0)
            return;

        ChefZ_Log.Banner("Handwerk  rezepte=" + s_Registered.ToString()
            + "  abgewiesen=" + s_Rejected.ToString()
            + "  kennsumme=" + s_Fingerprint.ToString()
            + "  (Kennsumme muss auf Client und Server gleich sein)");
    }

    static int  GetRegisteredCount() { return s_Registered; }
    static int  GetRejectedCount()   { return s_Rejected; }
    static int  GetFingerprint()     { return s_Fingerprint; }
    static bool IsInstalled()        { return s_InstalledInto != null; }

    static void DumpHandcraft(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("ChefZ Handwerk  rezepte=" + s_Registered.ToString()
            + "  abgewiesen=" + s_Rejected.ToString()
            + "  kennsumme=" + s_Fingerprint.ToString());

        if (!s_Order)
            return;

        for (int i = 0; i < s_Order.Count(); i++)
            outLines.Insert("  #" + i.ToString() + "  " + s_Order.Get(i));
    }
}
