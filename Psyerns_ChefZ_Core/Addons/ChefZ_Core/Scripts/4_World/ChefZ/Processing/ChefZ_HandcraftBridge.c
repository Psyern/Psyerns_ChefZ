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
//   2. Je HANDCRAFT-Transform ein ChefZ_GenericCraftRecipe parametrieren.
//      Additiv, in fester Reihenfolge, nie loeschend.
//   3. Den PLATZ richtig treffen. Das ist der unangenehmste Teil und bekommt
//      einen eigenen Abschnitt.
//
//==============================================================================
// POSITIONSANKER - warum ChefZ frueh registriert und spaet parametriert
//==============================================================================
//
// DAS PROBLEM, IN EINEM SATZ
// --------------------------
// Vanillas Craftaktion uebertraegt beim Start die REZEPT-ID
// (ActionWorldCraft.WriteToContext -> ctx.Write(m_RecipeID)), und diese ID ist
// die POSITION des Rezepts in m_RecipeList
// (PluginRecipesManager.RegisterRecipe: m_RegRecipeIndex = m_RecipeList.Insert
// (recipe); recipe.SetID(m_RegRecipeIndex)). Sie ist KEINE Identitaet. Zwei
// Seiten, die dieselben Rezepte in unterschiedlicher REIHENFOLGE registrieren,
// meinen unter derselben Zahl verschiedene Rezepte.
//
// DIE ZEITACHSE, DIE ES BRICHT
// ----------------------------
// Vanilla baut die Liste im MissionBase-KONSTRUKTOR auf:
//
//     MissionBase()                 <- Konstruktor
//       PluginManagerInit()
//         PluginsInit()
//           new PluginRecipesManager()
//             CreateAllRecipes() -> RegisterRecipies()   <- HIER
//             GenerateRecipeCache()
//           plugin.OnInit()                              <- und HIER
//     ...
//     MissionServer.OnInit()  /  MissionGameplay.OnInit()
//       ChefZ_CoreEntry.Boot* -> ChefZ_ConfigManager.LoadAll()  <- ERST HIER
//
// Zwischen RegisterRecipies() und OnInit() liegt fuer JEDEN Mod ein
// Registrierungsfenster, und mehrere ausgelieferte Mods benutzen es. Der
// entscheidende Punkt: manche von ihnen registrieren SEITENABHAENGIG - auf dem
// Server sofort in Plugin.OnInit(), auf dem Client erst per Netzwerkpaket beim
// Verbinden, also LANGE nach MissionGameplay.OnInit().
//
// Registrierte ChefZ - wie bis S15 - erst in OnInit() nach, entstand daraus:
//
//     Server:  [Vanilla][Kette aus RegisterRecipies][Fremdmod][ChefZ]
//     Client:  [Vanilla][Kette aus RegisterRecipies][ChefZ][Fremdmod]
//
// Beide Bloecke sind gegeneinander verschoben - und zwar in BEIDE Richtungen:
// die ChefZ-Rezepte des Clients zeigen serverseitig auf Fremdrezepte, und die
// Fremdrezepte des Clients zeigen serverseitig auf andere Fremdrezepte. Der
// zweite Teil ist der schlimmere: ChefZ haette damit fremdes Crafting kaputt
// gemacht, ohne dass der Spieler ein ChefZ-Rezept auch nur anfasst.
//
// Die frueher an dieser Stelle gemeldete KENNSUMME zeigt diesen Fall NICHT an.
// Sie geht nur ueber den ChefZ-Bestand, und der ist auf beiden Seiten
// identisch - der Versatz kommt von einem fremden Registrierer. Eine Loesung
// aus einer erweiterten Kennsumme gibt es nicht: der Core kann Fremdrezepte
// nicht abzaehlen, ohne sie zu kennen, und er darf sie nicht kennen.
//
// DIE LOESUNG: DEN PLATZ NEHMEN, WENN IHN ALLE NEHMEN
// ---------------------------------------------------
// ChefZ registriert seine Rezeptobjekte jetzt DORT, wo Vanilla registriert -
// in RegisterRecipies(), im Konstruktor, auf beiden Seiten am selben Punkt der
// modded-class-Kette. Zu diesem Zeitpunkt gibt es noch keine Daten; die
// Objekte sind LEER und tun nichts (ChefZ_GenericCraftRecipe.CanDo prueft
// m_ChefZ_Ready als allererstes und liefert false).
//
//     Reserve(mgr)     aus RegisterRecipies(). Traegt N leere Rezepte ein.
//                      N kommt aus der Engine-Config (CfgChefZ
//                      handcraftRecipeSlots, aufsummiert ueber alle Slices) -
//                      also aus der EINZIGEN Quelle, die auf Client und Server
//                      nachweislich dieselbe ist und die schon im Konstruktor
//                      vollstaendig vorliegt.
//     FillReserved()   aus ChefZ_Boot, unmittelbar nachdem der Bestand steht.
//                      Sie PARAMETRIERT die bereits eingetragenen Objekte
//                      (InitFromDef) und laesst Vanilla danach seinen
//                      Rezeptcache neu bauen. Es wird dabei KEIN Rezept mehr
//                      registriert und keine Position mehr veraendert.
//
// Damit gilt wieder, was ohne ChefZ galt: alles, was im Konstruktor
// registriert wird, steht vorn und steht auf beiden Seiten gleich; alles, was
// spaeter kommt, steht dahinter - egal von wem, egal wann. ChefZ verschiebt
// niemanden mehr, und niemand verschiebt ChefZ.
//
// DER PREIS, offen benannt: die Platzzahl ist eine DEKLARATION. Ein Slice mit
// HANDCRAFT-Transforms muss handcraftRecipeSlots nennen. Nennt er zu wenig,
// werden die ueberzaehligen Transforms mit einer Fehlerzeile im Klartext
// ABGEWIESEN - laut, deterministisch und auf beiden Seiten gleich. Das ist die
// Richtung, in die dieser Fehler fallen muss: ein Rezept, das fehlt und im RPT
// steht, kostet einen Konfigurationseintrag; ein Rezept, das das falsche Item
// erzeugt, kostet Vertrauen.
//
// WAS DER ANKER NICHT LEISTET
// ---------------------------
// Er setzt voraus, dass beide Seiten dieselbe Platzzahl lesen und denselben
// Transformbestand haben. Beides kann auseinanderlaufen:
//   - der Server liest zusaetzlich Rang 3 ($profile-Overlays, 02 §6). Ein
//     HANDCRAFT-Transform, den nur ein Overlay einfuehrt, belegt serverseitig
//     einen Platz, den der Client anders belegt.
//   - ein Client kann ein Addon geladen haben, das der Server nicht hat.
// Fuer genau diesen Rest gibt es das zweite Netz: ChefZ_CraftIntent, gefuehrt
// von ChefZ_ModdedWorldCraft. Der Client teilt neben der Position eine
// positionsUNabhaengige Kennung mit; der Server haelt sie gegen das Rezept,
// das bei IHM an dieser Position steht, und VERWEIGERT bei Widerspruch. Die
// Aktion laeuft dann ins Leere - sie erzeugt nie das falsche Ergebnis.
//
// Zusaetzlich meldet diese Klasse beim Start weiterhin eine Kennsumme, jetzt
// samt Ankerposition und Platzzahl. Stimmen die Zeilen im Client- und im
// Server-RPT nicht ueberein, steht die Ursache dort.
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
// ABHAENGIG VON OF-10 - ebenfalls offen benannt
// ---------------------------------------------------------------------------
// Transforms liegen in JSON. Ob eine JSON-Datei aus einem PBO clientseitig
// lesbar ist, ist eine offene Messfrage (OF-10, siehe auch Kopf von
// ChefZ_ActionProcessAtStation).
//
// Fuer Stationen ist die Antwort nicht kritisch: dort entscheidet der Server,
// und der Client zeigt im Zweifel eine Aktion, die abgelehnt wird. HIER ist
// sie es sehr wohl. Vanillas Craftaktion entsteht CLIENTSEITIG aus
// PluginRecipesManager.GetValidRecipes(); ohne Transformdaten parametriert der
// Client kein Rezept, und dann erscheint die Aktion NIE - unabhaengig davon,
// was der Server weiss.
//
// Der Positionsanker macht diesen Ausfall wenigstens HARMLOS: die Plaetze
// stehen auf beiden Seiten trotzdem an derselben Stelle, weil ihre Zahl aus
// der Engine-Config kommt und nicht aus den JSON-Daten. Nichts verschiebt
// sich; es fehlt nur die Aktion. Sollte die Messung negativ ausfallen, muessen
// HANDCRAFT-Transforms in die Game-Config wandern (Rang 1, 02 §2) - eine
// Datenentscheidung, keine Codeaenderung.
//
// KEIN CONTENT: kein Prozess, keine Zutat, kein Werkzeug wird hier benannt.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_HandcraftBridge
{
    /**
     * Obergrenze der Reservierung.
     *
     * Sie ist kein Balancingwert, sondern eine Bremse gegen einen Tippfehler:
     * ein "handcraftRecipeSlots = 100000" in einer fremden config.cpp wuerde
     * sonst Vanillas Rezeptliste (MAX_NUMBER_OF_RECIPES = 2048) sprengen und
     * ueber Error() eine Meldung je Eintrag erzeugen. 256 ist grosszuegig
     * jenseits jeder realistischen Zahl von Handwerksschritten und laesst
     * Vanillas eigener Liste den ganzen Rest.
     */
    static const int MAX_SLOTS = 256;

    //! In WELCHE Plugininstanz reserviert wurde. Schwacher Zeiger: das
    //! Plugin gehoert dem PluginManager, und ein starker Halter hier haette
    //! es ueber das Missionsende hinaus am Leben gehalten.
    private static PluginRecipesManagerBase s_ReservedInto;

    //! In WELCHE Plugininstanz bereits parametriert wurde. Getrennt gefuehrt,
    //! weil zwischen Reservierung und Parametrierung ein Missionsstart liegt -
    //! und weil ein ZWEITER Missionsstart im selben Prozess (Client, der einem
    //! zweiten Server beitritt) ein NEUES Plugin baut, das erneut gefuellt
    //! werden muss.
    private static PluginRecipesManagerBase s_FilledInto;

    //! Die reservierten Rezeptobjekte, in Registrierungsreihenfolge.
    //! ABSICHTLICH ohne ref: Eigentuemer ist Vanillas m_RecipeList
    //! (ref array<ref RecipeBase>). Ein zweiter starker Halter hier haette
    //! Rezepte ueber das Missionsende hinaus am Leben gehalten.
    private static ref array<ChefZ_GenericCraftRecipe> s_Slots;

    private static int s_SlotCount;
    private static int s_BaseIndex;
    private static int s_Filled;
    private static int s_Rejected;
    private static int s_Fingerprint;
    private static int s_IntentRefusals;

    //! Die parametrierten Transform-IDs, in Plaetzereihenfolge. Nur
    //! Diagnose - sie beantwortet die Frage "welches Rezept sitzt auf
    //! ChefZ-Platz drei", und die stellt sich genau dann, wenn etwas driftet.
    private static ref array<string> s_Order;

    //==========================================================================
    // 1) Reservierung - aus RegisterRecipies(), im Missionskonstruktor
    //==========================================================================

    /**
     * Traegt N leere ChefZ-Rezepte in Vanillas Liste ein und merkt sich ihre
     * Positionen.
     *
     * Sie ist ADDITIV und sie ist FOLGENLOS: jedes eingetragene Objekt ist
     * unparametriert, seine Zutatenlisten sind leer, es landet damit in
     * keinem Cache-Eintrag und wird nie angeboten. Faellt der ganze Aufruf
     * aus, ist Vanillas Rezeptliste ununterscheidbar von der eines Servers
     * ohne ChefZ.
     *
     * Kein Rueckgabewert - es gibt an der Aufrufstelle nichts zu entscheiden.
     */
    static void Reserve(PluginRecipesManagerBase mgr)
    {
        if (!mgr)
            return;

        if (s_ReservedInto == mgr)
            return;                         // diese Instanz ist bereits verankert

        ResetState();
        s_ReservedInto = mgr;

        int want = ChefZ_ManifestReader.ReadHandcraftSlotTotal();

        if (want <= 0)
        {
            // Der Normalfall ohne Handwerks-Content - und der Normalfall des
            // Core allein. Es wird KEIN Platz belegt.
            ChefZ_Log.Once(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.PROCESS,
                "handcraft.noslots",
                "Kein Slice meldet handcraftRecipeSlots an. ChefZ traegt kein Rezept "
                + "in Vanillas Liste ein; Vanilla-Crafting ist unveraendert.");
            return;
        }

        if (want > MAX_SLOTS)
        {
            ChefZ_Log.Error(ChefZ_LogChannel.PROCESS,
                "CfgChefZ meldet zusammen " + want.ToString() + " handcraftRecipeSlots. "
                + "Das ist mehr als die Obergrenze " + MAX_SLOTS.ToString()
                + " und mit hoher Wahrscheinlichkeit ein Zahlendreher in einer "
                + "config.cpp. Reserviert werden " + MAX_SLOTS.ToString() + " Plaetze; "
                + "die ueberzaehligen HANDCRAFT-Transforms werden abgewiesen. "
                + "Vanilla-Crafting ist davon unberuehrt.");
            want = MAX_SLOTS;
        }

        for (int i = 0; i < want; i++)
        {
            ChefZ_GenericCraftRecipe slot = new ChefZ_GenericCraftRecipe();

            if (!mgr.ChefZ_ReserveRecipeSlot(slot))
                break;

            if (s_SlotCount == 0)
                s_BaseIndex = slot.GetID();

            s_Slots.Insert(slot);
            s_SlotCount++;
        }

        // An der Stufenpruefung vorbei: diese Zeile muss auf Client und Server
        // vergleichbar sein, und wer sie vergleicht, hat keine Debugstufe an
        // (18 §4).
        ChefZ_Log.Banner("Handwerk Anker  plaetze=" + s_SlotCount.ToString()
            + "  ab Rezept-ID " + s_BaseIndex.ToString()
            + "  (Anker und Platzzahl muessen auf Client und Server gleich sein)");
    }

    private static void ResetState()
    {
        s_ReservedInto   = null;
        s_FilledInto     = null;
        s_Slots          = new array<ChefZ_GenericCraftRecipe>();
        s_Order          = new array<string>();
        s_SlotCount      = 0;
        s_BaseIndex      = -1;
        s_Filled         = 0;
        s_Rejected       = 0;
        s_Fingerprint    = 0;
        s_IntentRefusals = 0;
    }

    //==========================================================================
    // 2) Parametrierung - aus ChefZ_Boot, nachdem der Bestand steht
    //==========================================================================

    /**
     * Fuellt die reservierten Plaetze und laesst Vanillas Rezeptcache neu
     * bauen.
     *
     * Wird von ChefZ_Boot gerufen, auf BEIDEN Seiten. Der Client braucht die
     * Rezepte fuer die Anzeige der Craftaktion (ActionWorldCraft fragt
     * GetValidRecipes clientseitig); entschieden wird trotzdem nur auf dem
     * Server (00 §5).
     *
     * Hier wird NICHTS mehr registriert. Positionen entstehen ausschliesslich
     * in Reserve(); diese Methode beschreibt nur, was dort schon steht.
     *
     * Der Cacheneubau kostet einen vollstaendigen Durchlauf durch
     * CfgVehicles, CfgWeapons und CfgMagazines - genau denselben, den Vanilla
     * beim Start ohnehin einmal macht. Er faellt einmal je Missionsstart an
     * und nur dann, wenn tatsaechlich ein Platz gefuellt wurde.
     */
    static void FillReserved()
    {
        PluginRecipesManager plugin;
        if (!Class.CastTo(plugin, GetPlugin(PluginRecipesManager)))
        {
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PROCESS,
                "handcraft.noplugin",
                "PluginRecipesManager ist nicht erreichbar. Handwerksrezepte werden "
                + "nicht angeboten; Vanilla-Crafting ist davon unberuehrt.");
            return;
        }

        // Ausdruecklich als Basistyp vergleichen: die beiden Merker sind
        // PluginRecipesManagerBase, und ein Vergleich ueber eine
        // Typgrenze hinweg soll hier nicht vom Compiler abhaengen.
        PluginRecipesManagerBase asBase = plugin;

        if (s_FilledInto == asBase)
            return;                         // diese Instanz ist bereits gefuellt

        if (s_ReservedInto != asBase)
        {
            // Reserve() hat diese Plugininstanz nie gesehen. Das heisst, dass
            // RegisterRecipies() unseren Teil der Kette nicht erreicht hat -
            // ein Fremdmod ruft super nicht. NACHTRAEGLICH zu registrieren
            // waere hier genau der Fehler, den der Anker beseitigt: der Platz
            // laege dann hinter allem, was inzwischen dazugekommen ist, und
            // auf den beiden Seiten verschieden.
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PROCESS,
                "handcraft.noanchor",
                "Es sind keine Rezeptplaetze verankert - RegisterRecipies() hat den "
                + "ChefZ-Teil der Kette nicht erreicht. Handwerksrezepte werden NICHT "
                + "nachtraeglich eingetragen: ihre IDs waeren auf Client und Server "
                + "verschieden. Vanilla-Crafting ist vollstaendig, ChefZ-Kochen und "
                + "ChefZ-Stationen sind unberuehrt.");
            return;
        }

        s_FilledInto  = asBase;
        s_Filled      = 0;
        s_Rejected    = 0;
        s_Fingerprint = 0;
        s_Order.Clear();

        ChefZ_ProcessingManager pm = ChefZ_ProcessingManager.Get();
        if (!pm.IsReady())
            return;

        array<ChefZ_CompiledProcess> processes = new array<ChefZ_CompiledProcess>();
        pm.GetProcessesForExec(ChefZ_ProcessExec.HANDCRAFT, processes);

        if (processes.Count() == 0)
            return;                         // kein Handwerksprozess deklariert

        // Prozesse und Transforms zu Paaren ausrollen und in ID-Reihenfolge
        // bringen. Die Reihenfolge ist KEINE Kosmetik: sie bestimmt, welcher
        // Transform auf welchem verankerten Platz sitzt.
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

        if (pairTr.Count() == 0)
            return;

        if (s_SlotCount == 0)
        {
            ReportMissingSlots(pairTr.Count());
            return;
        }

        for (int i = 0; i < pairTr.Count(); i++)
        {
            if (i >= s_SlotCount)
            {
                ReportSurplus(pairTr, i);
                break;
            }

            FillOne(s_Slots.Get(i), pairProc.Get(i), pairTr.Get(i));
        }

        Report();

        if (s_Filled > 0)
        {
            // Oeffentliche Vanillamethode; sie ruft GenerateRecipeCache().
            // Ohne diesen Aufruf waeren die Rezepte parametriert, aber ueber
            // den Cache nicht auffindbar - GenerateRecipeCache() laeuft im
            // Konstruktor genau einmal, und dort waren die Plaetze leer.
            plugin.CallbackGenerateCache();
        }
    }

    //==========================================================================
    // Ein einzelner Platz
    //==========================================================================

    /**
     * Der Platz i gehoert IMMER dem Paar i - auch dann, wenn die
     * Parametrierung fehlschlaegt.
     *
     * Das ist Absicht und nicht Bequemlichkeit: eine Abweisung, die den Platz
     * frei liesse, verschoebe jeden nachfolgenden Transform um eins. Beide
     * Seiten weisen dieselben Daten identisch ab, aber sie duerfen sich nicht
     * darauf verlassen muessen - eine Abweisung kostet ein Rezept, sie darf
     * nie eine Verschiebung kosten.
     */
    private static void FillOne(ChefZ_GenericCraftRecipe slot,
                                notnull ChefZ_CompiledProcess proc,
                                notnull ChefZ_CompiledTransform tr)
    {
        // Die ID geht IMMER in die Kennsumme, auch bei einer Abweisung. Die
        // Kennsumme beantwortet "haben beide Seiten denselben Bestand in
        // derselben Reihenfolge gesehen" - und das ist unabhaengig davon, ob
        // ein Transform abbildbar war.
        s_Order.Insert(tr.id);
        s_Fingerprint = s_Fingerprint * 31 + tr.id.Hash();

        if (!slot)
        {
            Reject(tr, "der verankerte Rezeptplatz fehlt - das ist ein interner Fehler "
                + "und sollte nicht vorkommen");
            return;
        }

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
            ChefZ_CompiledSlot cslot = tr.inputs.Get(s);
            array<string> classes = new array<string>();
            if (cslot)
                CollectSelectorClasses(cslot.selector, classes);
            inputClasses.Insert(classes);
        }

        array<string> toolClasses = new array<string>();
        CollectToolClasses(proc, toolClasses);

        //--- Parametrieren: der Platz bleibt derselbe -------------------------
        string err;
        if (!slot.InitFromDef(proc, tr, inputClasses, toolClasses, err))
        {
            Reject(tr, err);
            return;
        }

        s_Filled++;

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS,
                "Handwerksrezept auf Platz " + (s_Order.Count() - 1).ToString()
                + " (Rezept-ID " + slot.GetID().ToString() + "): "
                + slot.ChefZ_ToDebugString());
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
            + "angeboten: " + why + " Die uebrigen Transforms sind davon unberuehrt, "
            + "Vanilla-Crafting ebenfalls.");
    }

    //! Es gibt Transforms, aber keinen einzigen verankerten Platz.
    private static void ReportMissingSlots(int needed)
    {
        s_Rejected = needed;

        ChefZ_Log.Error(ChefZ_LogChannel.PROCESS,
            "Es sind " + needed.ToString() + " HANDCRAFT-Transforms geladen, aber KEIN "
            + "Rezeptplatz reserviert. Kein Handwerksrezept wird angeboten. Ursache: "
            + "kein Slice nennt in seiner config.cpp \"handcraftRecipeSlots\". Der Platz "
            + "muss VOR dem Laden feststehen - Vanilla vergibt Rezept-IDs als Position "
            + "in seiner Liste, und diese Positionen entstehen im Missionskonstruktor. "
            + "Abhilfe: im CfgChefZ-Knoten des Slice \"handcraftRecipeSlots = "
            + needed.ToString() + ";\" eintragen. Vanilla-Crafting, ChefZ-Kochen und "
            + "ChefZ-Stationen sind unberuehrt.");
    }

    //! Es gibt mehr Transforms als verankerte Plaetze.
    private static void ReportSurplus(notnull array<ChefZ_CompiledTransform> pairs,
                                      int firstSurplus)
    {
        int surplus = pairs.Count() - firstSurplus;
        s_Rejected  = s_Rejected + surplus;

        string names = "";
        for (int i = firstSurplus; i < pairs.Count(); i++)
        {
            if (names != "")
                names = names + ", ";
            names = names + pairs.Get(i).id;
        }

        ChefZ_Log.Error(ChefZ_LogChannel.PROCESS,
            "Es sind " + pairs.Count().ToString() + " HANDCRAFT-Transforms geladen, aber "
            + "nur " + s_SlotCount.ToString() + " Rezeptplaetze reserviert. Die "
            + surplus.ToString() + " ueberzaehligen werden NICHT angeboten: " + names
            + ". Nachtraeglich einzutragen ist keine Loesung - ihre Rezept-IDs waeren "
            + "auf Client und Server verschieden, und der Spieler bekaeme das falsche "
            + "Ergebnis. Abhilfe: \"handcraftRecipeSlots\" im CfgChefZ-Knoten des Slice "
            + "auf die tatsaechliche Zahl erhoehen. Die ersten " + s_SlotCount.ToString()
            + " Transforms funktionieren normal, Vanilla-Crafting ebenfalls.");
    }

    //==========================================================================
    // Identitaetspruefung (zweites Netz, siehe Dateikopf)
    //==========================================================================

    /**
     * Welche ChefZ-Identitaet steht auf DIESER Seite an dieser Rezeptposition?
     *
     * Antwort ChefZ_CraftIntent.NOT_CHEFZ fuer jede Position, die kein
     * fertiges ChefZ-Rezept traegt - Vanilla, Fremdmod, unbelegter Platz,
     * Position ausserhalb der Liste. Genau das ist der Normalfall, und zwei
     * solche Antworten sind gleich: ChefZ mischt sich in fremdes Crafting
     * nicht ein.
     *
     * Rein lesend. Sie veraendert nichts an Vanillas Liste und legt nichts an.
     */
    static int IntentOfRecipeId(int recipeId)
    {
        if (recipeId < 0)
            return ChefZ_CraftIntent.NOT_CHEFZ;

        PluginRecipesManager plugin;
        if (!Class.CastTo(plugin, GetPlugin(PluginRecipesManager)))
            return ChefZ_CraftIntent.NOT_CHEFZ;

        if (!plugin.m_RecipeList)
            return ChefZ_CraftIntent.NOT_CHEFZ;

        if (recipeId >= plugin.m_RecipeList.Count())
            return ChefZ_CraftIntent.NOT_CHEFZ;

        ChefZ_GenericCraftRecipe recipe;
        if (!Class.CastTo(recipe, plugin.m_RecipeList.Get(recipeId)))
            return ChefZ_CraftIntent.NOT_CHEFZ;

        if (!recipe.ChefZ_IsReady())
            return ChefZ_CraftIntent.NOT_CHEFZ;

        return ChefZ_CraftIntent.Of(recipe.ChefZ_GetTransformId());
    }

    /**
     * Darf der Server diese Craftaktion ausfuehren?
     *
     * Serverseitiger Torwaechter. Er wird von ChefZ_ModdedWorldCraft
     * unmittelbar vor PerformRecipeServer gefragt.
     *
     * Er ist BEWUSST nur ein Nein-Geber: ohne Angabe der Gegenseite laesst er
     * durch, und bei Uebereinstimmung laesst er durch. Der Client entscheidet
     * damit nichts - er kann eine Aktion hoechstens verhindern, die ohnehin
     * die falsche gewesen waere.
     */
    static bool AcceptCraftIntent(int recipeId, int clientIntent)
    {
        int serverIntent = IntentOfRecipeId(recipeId);

        if (ChefZ_CraftIntent.Accepts(clientIntent, serverIntent))
            return true;

        s_IntentRefusals++;

        // Once mit festem Schluessel: eine Zeile je Missionsstart genuegt zur
        // Diagnose, und ein Spieler, der es wiederholt versucht, soll das RPT
        // nicht fluten. Die Gesamtzahl steht in "chefz registries".
        ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.PROCESS,
            "handcraft.intentdrift",
            "Eine Craftaktion wurde VERWEIGERT: der Client meinte "
            + ChefZ_CraftIntent.Describe(clientIntent) + ", an Rezept-ID "
            + recipeId.ToString() + " steht auf dem Server aber "
            + ChefZ_CraftIntent.Describe(serverIntent) + ". Die Rezeptlisten von Client "
            + "und Server sind gegeneinander verschoben. Haeufigste Ursachen: die "
            + "Modliste des Clients ist nicht die des Servers, oder ein "
            + "$profile-Overlay bringt HANDCRAFT-Transforms mit, die es im PBO nicht "
            + "gibt (Rang 3 ist serverseitig, 02 §6). Vergleiche die Zeile "
            + "\"Handwerk Anker\" in beiden RPT-Dateien. Es wurde NICHTS erzeugt und "
            + "NICHTS verbraucht.");

        return false;
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
     * Vanillas Rezeptcache auf beiden Seiten gleich aussehen soll;
     * duplikatfrei, weil derselbe Klassenname zweimal in m_Ingredients
     * Vanillas Cache zweimal denselben Eintrag kostet.
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
                 * roh sein oder nicht. {"not":{"category":"X"}} schliesst
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
    // Paare in ID-Reihenfolge (siehe Dateikopf, POSITIONSANKER)
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
     * Sie steht dort aus einem einzigen Grund: Ankerposition, Platzzahl und
     * Kennsumme sind auf Client und Server zu vergleichen, und ein Betreiber,
     * der das tun muss, hat keine Debugstufe eingeschaltet (18 §4, dieselbe
     * Begruendung wie bei ChefZ_Boot.ReportState).
     */
    private static void Report()
    {
        ChefZ_Log.Banner("Handwerk  rezepte=" + s_Filled.ToString()
            + "  abgewiesen=" + s_Rejected.ToString()
            + "  plaetze=" + s_SlotCount.ToString()
            + "  ab Rezept-ID " + s_BaseIndex.ToString()
            + "  kennsumme=" + s_Fingerprint.ToString()
            + "  (alle vier muessen auf Client und Server gleich sein)");
    }

    static int  GetRegisteredCount() { return s_Filled; }
    static int  GetRejectedCount()   { return s_Rejected; }
    static int  GetFingerprint()     { return s_Fingerprint; }
    static int  GetSlotCount()       { return s_SlotCount; }
    static int  GetBaseIndex()       { return s_BaseIndex; }
    static int  GetIntentRefusals()  { return s_IntentRefusals; }
    static bool IsAnchored()         { return s_ReservedInto != null; }
    static bool IsInstalled()        { return s_FilledInto != null; }

    static void DumpHandcraft(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("ChefZ Handwerk  rezepte=" + s_Filled.ToString()
            + "  abgewiesen=" + s_Rejected.ToString()
            + "  plaetze=" + s_SlotCount.ToString()
            + "  ab Rezept-ID " + s_BaseIndex.ToString()
            + "  kennsumme=" + s_Fingerprint.ToString()
            + "  verweigert=" + s_IntentRefusals.ToString());

        if (!s_Order)
            return;

        for (int i = 0; i < s_Order.Count(); i++)
        {
            string id = "?";
            if (s_BaseIndex >= 0)
                id = (s_BaseIndex + i).ToString();

            outLines.Insert("  Platz " + i.ToString() + "  Rezept-ID " + id
                + "  " + s_Order.Get(i));
        }
    }
}
