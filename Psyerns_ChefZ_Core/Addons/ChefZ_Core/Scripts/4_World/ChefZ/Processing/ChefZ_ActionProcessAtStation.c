//==============================================================================
// ChefZ_ActionProcessAtStation - EINE Action fuer ALLE Stationsprozesse
//
// Entwurf: 11 §4 (Schnittstelle), 11 §5 (Datenfluss STATION_ACTION), 11 §7
// (Fehlerverhalten), 11 E1 (eine Action-Klasse, der Prozess ist ein
// Laufzeitparameter), 00 §5 (nichts Autoritatives auf dem Client), 02 §2
// (was der Client wissen muss, steht in der Game-Config).
//
// ---------------------------------------------------------------------------
// E1, und was er kostet
// ---------------------------------------------------------------------------
// 11 E1 verwirft "eine Action je Prozess" mit einem harten Argument: Actions
// liegen in 4_World und muessen in SetActions() registriert werden - also
// entweder im Core (verboten: Content im Core) oder verstreut in Modulen
// (dann kann der Core sie nicht generisch anbieten). Diese Klasse macht den
// Prozess deshalb zu einem LAUFZEITPARAMETER.
//
// Der Preis steht in E1 ebenfalls offen: Aktionstext und Dauer muessen aus den
// DATEN kommen, und wenn eine Station mehrere Prozesse anbietet, braucht es
// eine Auswahl. Beides ist hier umgesetzt:
//
//   Text    ChefZ_CompiledProcess.displayName, ersatzweise
//           #STR_CHEFZ_ACTION_PROCESS. Kein Text im Code.
//   Dauer   ChefZ_ProcessingManager.GetDuration(), also Transform-Override,
//           sonst Prozessdauer, geteilt durch speedMultiplier der Station.
//   Auswahl Vanillas Variantenmechanik (ActionVariantManager), dieselbe, die
//           ActionWorldCraft fuer mehrere Craftrezepte benutzt.
//
// ---------------------------------------------------------------------------
// Wer entscheidet was
// ---------------------------------------------------------------------------
//   CLIENT   zeigt an. ActionCondition darf hier grosszuegig sein - eine
//            Aktion, die erscheint und dann nichts tut, ist aergerlich; eine,
//            die nicht erscheint, obwohl der Server sie erlauben wuerde, ist
//            ein totes Feature.
//   SERVER   entscheidet. OnFinishProgressServer bindet ERNEUT (11 §5:
//            "FindTransform ERNEUT - der Zustand kann sich geaendert haben,
//            Client nie glauben") und ruft dann den ChefZ_ProcessRunner.
//
// Der Client schickt genau EINE Zahl mit: den Persistenz-Hash des gewaehlten
// Prozesses. Er waehlt damit AUS, was die Station ohnehin anbietet - er kann
// nichts erfinden. Der Server prueft, ob die Station den Prozess wirklich
// fuehrt, und faellt sonst auf den ersten passenden zurueck.
//
// ---------------------------------------------------------------------------
// Zur Transformdatenlage auf dem Client
// ---------------------------------------------------------------------------
// Stationen und Werkzeuggruppen stehen in der Game-Config und liegen dem
// Client damit garantiert vor (02 §2, 11 E8). TRANSFORMS liegen in JSON, und
// ob eine JSON-Datei aus einem PBO clientseitig lesbar ist, ist eine offene
// Messfrage (OF-10).
//
// Deshalb prueft ActionCondition den Transform nur, WENN diese Seite ihn
// kennen kann (HasAnyTransformFor). Kennt sie ihn nicht, entscheidet allein
// der Server - und das ist die richtige Richtung: der Client zeigt eine
// Aktion, die der Server im Zweifel ablehnt, statt eine anzubieten, die es
// nie gibt.
//
// KEIN CONTENT: kein Prozessname, kein Stationsname, kein Anzeigetext.
//
// Layer: 4_World.
//==============================================================================

/**
 * Die Nutzlast der Aktion. Sie traegt genau eine Zahl.
 *
 * Der HASH und nicht der Symbolzaehler oder der Listenindex (03 E2): der
 * Symbolzaehler haengt an der Internierungsreihenfolge und ist auf Client und
 * Server verschieden; ein Listenindex haengt an der Prozessliste der Station
 * und waere nach einem Config-Update etwas anderes. Der Hash der Prozess-ID
 * ist auf beiden Seiten derselbe und ueber Updates hinweg stabil.
 */
class ChefZ_ProcessActionData extends ActionData
{
    int m_ChefZ_ProcessHash;
}

class ChefZ_ProcessActionReciveData extends ActionReciveData
{
    int m_ChefZ_ProcessHash;
}

//------------------------------------------------------------------------------

/**
 * Der Callback. Er bestimmt die DAUER - und zwar aus den Daten.
 *
 * CAContinuousTime und nicht CAContinuousRepeat: ein Verarbeitungsschritt ist
 * eine Zeitspanne, keine Wiederholung. Vanilla waehlt bei ActionSawPlanks
 * dieselbe Bauform (dort CAContinuousRepeat, weil Saegen wiederholt) und
 * berechnet die Zeit ebenfalls im Callback aus dem beteiligten Item.
 */
class ChefZ_ActionProcessAtStationCB extends ActionContinuousBaseCB
{
    //! Rueckfalldauer, wenn die Daten nichts hergeben. Sie ist kurz und
    //! spuerbar: eine Aktion ohne Fortschrittsbalken sieht aus wie ein Fehler,
    //! eine mit zehn Sekunden Leerlauf ebenfalls.
    static const float FALLBACK_SEC = 3.0;

    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(ResolveTime());
    }

    protected float ResolveTime()
    {
        if (!m_ActionData)
            return FALLBACK_SEC;

        ChefZ_ProcessingStation_Base station;
        if (!Class.CastTo(station, m_ActionData.m_Target.GetObject()))
            return FALLBACK_SEC;

        ChefZ_Sym process = ChefZ_ActionProcessAtStation.ResolveProcessFor(
            station, m_ActionData.m_MainItem, ProcessHashOf(m_ActionData));

        float seconds = ChefZ_ActionProcessAtStation.DurationOf(station,
                                                                m_ActionData.m_MainItem,
                                                                process);
        if (seconds <= 0.0)
            return FALLBACK_SEC;
        return seconds;
    }

    protected int ProcessHashOf(ActionData data)
    {
        ChefZ_ProcessActionData typed;
        if (Class.CastTo(typed, data))
            return typed.m_ChefZ_ProcessHash;
        return ChefZ_ProcessJob.NO_HASH;
    }
}

//------------------------------------------------------------------------------

class ChefZ_ActionProcessAtStation extends ActionContinuousBase
{
    //! Rueckfalltext, wenn ein Prozess keinen displayName fuehrt. Der EINZIGE
    //! Text dieser Datei, und er steht in der Stringtable - nicht im Code.
    static const string FALLBACK_TEXT = "#STR_CHEFZ_ACTION_PROCESS";

    /**
     * Die Prozessliste, die der Spieler gerade zur Auswahl hat.
     *
     * STATISCH und AUSSCHLIESSLICH CLIENTSEITIGE ANZEIGE. Sie existiert, weil
     * Vanillas Variantenmechanik fuer jede Variante eine EIGENE Actioninstanz
     * anlegt (ActionVariantManager.SetActionVariantCount), und diese Instanzen
     * kennen einander nicht - sie kennen nur ihre m_VariantID. Irgendwo muss
     * die Zuordnung "ID -> Prozess" liegen; Vanilla legt sie beim Spieler ab
     * (CraftingManager), was hier eine modded class PlayerBase kostete.
     *
     * Sie ist NICHT autoritativ und kann es nicht sein: was tatsaechlich
     * geschieht, entscheidet OnFinishProgressServer, und der bindet neu.
     * Steht hier Unsinn, waehlt der Spieler den falschen Prozess - und der
     * Server lehnt ihn ab oder nimmt den passenden.
     */
    private static ref array<ChefZ_Sym> s_ClientProcesses;

    void ChefZ_ActionProcessAtStation()
    {
        m_CallbackClass  = ChefZ_ActionProcessAtStationCB;
        m_CommandUID     = DayZPlayerConstants.CMD_ACTIONFB_CRAFTING;
        m_FullBody       = true;
        m_StanceMask     = DayZPlayerConstants.STANCEMASK_ERECT;
        m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
        m_Text           = FALLBACK_TEXT;
        m_LockTargetOnUse = false;
    }

    //! Zielbasierte Eingabe: die Action haengt an der STATION, nicht am Item
    //! in der Hand (ContinuousInteractActionInput hat m_DetectFromTarget = 1).
    //! Deshalb registriert ChefZ_ProcessingStation_Base.SetActions() sie, und
    //! nicht irgendein Werkzeug.
    override typename GetInputType()
    {
        return ContinuousInteractActionInput;
    }

    override void CreateConditionComponents()
    {
        // CCINone und nicht CCINonRuined: das Item in der Hand ist optional.
        // Ein Fleischwolf IST das Werkzeug (siehe ChefZ_FactCollector.
        // CollectToolGroups), und ein Spieler mit leeren Haenden soll ihn
        // bedienen koennen.
        m_ConditionItem   = new CCINone;
        m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
    }

    override ActionData CreateActionData()
    {
        ChefZ_ProcessActionData data = new ChefZ_ProcessActionData();
        return data;
    }

    override bool HasProgress()
    {
        return true;
    }

    //==========================================================================
    // Bedingung (11 §5)
    //==========================================================================

    /**
     * Erscheint die Aktion?
     *
     * Die Reihenfolge ist nach KOSTEN gewaehlt, nicht nach Aussagekraft - im
     * Unterschied zu fast allem anderen im Core. Der Grund: diese Methode
     * laeuft bei JEDEM Zielwechsel des Fadenkreuzes, und der weitaus
     * haeufigste Ausgang ist "das Ziel ist gar keine Station". Ein Cast und
     * ein Zaehlvergleich muessen genuegen, um das zu erkennen.
     *
     * ZWEI Betriebsarten, und die Unterscheidung ist wichtig genug fuer einen
     * eigenen Absatz:
     *
     *   MIT Variantenzustand   diese Instanz IST eine Variante und steht fuer
     *                          GENAU EINEN Prozess. Sie antwortet nur fuer
     *                          ihren eigenen.
     *   OHNE Variantenzustand  der erste Bildaufbau an einer Station und
     *                          jeder Aufruf auf dem SERVER. Dann genuegt
     *                          IRGENDEIN nutzbarer Prozess.
     *
     * Der zweite Fall muss grosszuegig sein, und zwar aus einem strukturellen
     * Grund: StandardActionInput ruft OnActionInfoUpdate() - und damit den
     * Aufbau der Variantenliste - AUSSCHLIESSLICH fuer Aktionen, die Can()
     * bereits bejaht haben (ActionInput.c:389-400). Waere ActionCondition hier
     * streng und pruefte nur processes[0], erschiene an einer Station, deren
     * erster Prozess gerade nicht laufen kann, NIE eine Aktion - und damit
     * entstuende auch nie eine Variantenliste, die es besser wuesste.
     */
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!target)
            return false;

        ChefZ_ProcessingStation_Base station;
        if (!Class.CastTo(station, target.GetObject()))
            return false;

        int count = station.ChefZ_GetProcessCount();
        if (count == 0)
            return false;               // inerte Deko (11 §7)

        int actorId = IdentityOf(player);

        ChefZ_Sym variant = VariantProcess();
        if (ChefZ_SymbolTable.IsValid(variant))
            return IsProcessUsable(station, item, variant, actorId);

        for (int i = 0; i < count; i++)
        {
            ChefZ_Sym process = station.ChefZ_GetProcessAt(i);
            if (!ChefZ_SymbolTable.IsValid(process))
                continue;
            if (IsProcessUsable(station, item, process, actorId))
                return true;
        }

        return false;
    }

    /**
     * Kann DIESER Prozess an DIESER Station gerade laufen?
     *
     * Drei Stufen, und die dritte ist bewusst bedingt:
     *
     *   1. Prozess bekannt und Slot frei
     *   2. Umgebung und Werkzeug        - Daten aus Rang 1, auf beiden Seiten
     *                                     vorhanden (02 §2, 11 E8)
     *   3. passender Transform          - NUR, wenn diese Seite Transforms
     *                                     kennt; siehe Dateikopf
     */
    protected bool IsProcessUsable(notnull ChefZ_ProcessingStation_Base station,
                                   ItemBase item, ChefZ_Sym process, int actorId)
    {
        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();

        ChefZ_CompiledProcess proc = mgr.GetProcess(process);
        if (!proc)
            return false;
        if (!proc.IsStationProcess())
            return false;               // HANDCRAFT laeuft ueber Vanillas Craftsystem

        if (station.ChefZ_FreeSlotIndex() < 0)
            return false;               // alle Slots belegt

        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        station.ChefZ_BuildContext(item, actorId, ctx);

        string reason;
        if (!proc.MeetsEnvironment(ctx, reason))
            return false;

        string missing;
        if (!proc.HasTools(ctx, missing))
        {
            // 11 §7: "Werkzeug fehlt -> Action erscheint nicht; DEBUG mit der
            // fehlenden Werkzeuggruppe. Keine irrefuehrende HUD-Meldung."
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
            {
                ChefZ_Log.Once(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.PROCESS,
                    "action.tool." + missing,
                    "Die Aktion an \"" + station.GetType() + "\" erscheint nicht: die "
                    + "Werkzeuggruppe " + missing + " fehlt.");
            }
            return false;
        }

        // Stufe 3, siehe Dateikopf. Kennt diese Seite keine Transforms, wird
        // hier NICHT abgewiesen - der Server entscheidet.
        if (!mgr.HasAnyTransformFor(process))
            return true;

        ChefZ_FactSnapshot snapshot;
        array<ItemBase>    entities;
        ChefZ_FactCollector.CollectFromCargo(station, snapshot, entities);
        snapshot.SortStable();

        ChefZ_TransformMatch match;
        return mgr.FindTransform(process, ctx, snapshot, null, match);
    }

    //==========================================================================
    // Prozessauswahl und Varianten (11 E1)
    //==========================================================================

    /**
     * Der Prozess DIESER Actioninstanz, oder INVALID.
     *
     * INVALID heisst "es gibt noch keinen Variantenzustand" - auf dem Server
     * immer, auf dem Client bis zum ersten OnActionInfoUpdate. Der Aufrufer
     * entscheidet, was das bedeutet.
     *
     * s_ClientProcesses enthaelt bereits nur NUTZBARE Prozesse (siehe
     * RefreshProcesses), und m_VariantID ist der Index darin. Bei genau einem
     * nutzbaren Prozess ist die Liste einelementig und m_VariantID 0 - auch
     * das ist ein gueltiger Variantenzustand, und er ist der haeufigste.
     */
    protected ChefZ_Sym VariantProcess()
    {
        if (!s_ClientProcesses)
            return ChefZ_SymbolTable.INVALID;
        if (m_VariantID < 0 || m_VariantID >= s_ClientProcesses.Count())
            return ChefZ_SymbolTable.INVALID;
        return s_ClientProcesses.Get(m_VariantID);
    }

    /**
     * Welcher Prozess soll ausgefuehrt werden?
     *
     * Erst die Variante des Spielers, dann - falls es keine gibt - der erste
     * Prozess der Station. Der zweite Fall ist ein VORSCHLAG:
     * OnFinishProgressServer nimmt am Ende den, den der Client benannt hat,
     * und prueft ihn gegen das Angebot der Station.
     */
    protected ChefZ_Sym SelectProcess(notnull ChefZ_ProcessingStation_Base station,
                                      ItemBase item)
    {
        ChefZ_Sym variant = VariantProcess();
        if (ChefZ_SymbolTable.IsValid(variant))
            return variant;

        return station.ChefZ_GetProcessAt(0);
    }

    /**
     * Die Auswahlliste neu aufbauen - CLIENTSEITIG.
     *
     * Aufgenommen werden nur Prozesse, die JETZT laufen koennten. Ein
     * Raeuchervorgang an einer kalten Station erscheint damit gar nicht erst
     * als Auswahlpunkt - statt zu erscheinen und beim Ausloesen nichts zu tun.
     *
     * Und hier wird der Variantenzaehler gesetzt. Das MUSS an einer Stelle
     * geschehen, die auch OHNE Varianten laeuft, sonst gaebe es nie welche:
     * StandardActionInput._GetSelectedActions() fragt HasVariants(), BEVOR es
     * UpdateVariants() ruft. Waere der Zaehler nur in UpdateVariants gesetzt,
     * bliebe HasVariants() auf ewig false - eine Henne ohne Ei.
     *
     * OnActionInfoUpdate() laeuft in BEIDEN Zweigen dieser Schleife. Der erste
     * Bildaufbau an einer Station mit drei Prozessen zeigt deshalb noch eine
     * einzelne Aktion, der zweite die Auswahl. Genau so verhaelt sich Vanillas
     * Crafting auch (CraftingManager.SetWorldCraft laeuft im Tick).
     */
    protected void RefreshProcesses(Object item, Object target)
    {
        if (!s_ClientProcesses)
            s_ClientProcesses = new array<ChefZ_Sym>();
        s_ClientProcesses.Clear();

        ChefZ_ProcessingStation_Base station;
        if (!Class.CastTo(station, target))
            return;

        ItemBase inHands = ItemBase.Cast(item);

        int count = station.ChefZ_GetProcessCount();
        for (int i = 0; i < count; i++)
        {
            ChefZ_Sym process = station.ChefZ_GetProcessAt(i);
            if (!ChefZ_SymbolTable.IsValid(process))
                continue;
            if (!IsProcessUsable(station, inHands, process, 0))
                continue;
            s_ClientProcesses.Insert(process);
        }

        ActionVariantManager variants = GetVariantManager();
        if (!variants)
            return;

        if (s_ClientProcesses.Count() > 1)
            variants.SetActionVariantCount(s_ClientProcesses.Count());
        else
            variants.Clear();
    }

    /**
     * Varianten gibt es NUR, wenn wirklich mehrere Prozesse zur Wahl stehen.
     *
     * Die Basis antwortet "ja, sobald ein Variantenmanager existiert" - und
     * der existiert hier ab dem ersten Zielwechsel auf eine Station mit
     * mehreren Prozessen, auch wenn spaeter nur noch einer uebrig ist. Die
     * Folge waere eine Station mit genau einem nutzbaren Prozess, an der GAR
     * KEINE Aktion erscheint: HasVariants() waere true, die Variantenliste
     * aber leer.
     */
    override bool HasVariants()
    {
        if (!s_ClientProcesses)
            return false;
        return s_ClientProcesses.Count() > 1;
    }

    override void UpdateVariants(Object item, Object target, int componentIndex)
    {
        super.UpdateVariants(item, target, componentIndex);
        RefreshProcesses(item, target);
    }

    /**
     * Der Aktionstext (11 E1: "aus ChefZ_ProcessDef.displayName, nicht aus
     * Code").
     *
     * Ohne displayName der Rueckfalltext aus der Stringtable. Der Core
     * enthaelt dadurch KEINEN sichtbaren Text ausser diesem einen Schluessel,
     * und der sagt nichts ueber ein Gericht, eine Zutat oder einen Prozess -
     * er sagt "verarbeiten".
     */
    override string GetText()
    {
        ChefZ_Sym process = VariantProcess();
        if (!ChefZ_SymbolTable.IsValid(process))
            return FALLBACK_TEXT;

        ChefZ_CompiledProcess proc = ChefZ_ProcessingManager.Get().GetProcess(process);
        if (!proc || proc.displayName == "")
            return FALLBACK_TEXT;

        return proc.displayName;
    }

    override void OnActionInfoUpdate(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (target)
            RefreshProcesses(item, target.GetObject());

        m_Text = GetText();
    }

    //==========================================================================
    // Ausfuehrung (11 §5, SERVER)
    //==========================================================================

    /**
     * Den gewaehlten Prozess in die Aktionsdaten schreiben.
     *
     * Nur clientseitig: auf dem Server hat HandleReciveData den Wert bereits
     * gesetzt, und ihn hier zu ueberschreiben hiesse, die Auswahl des Spielers
     * zu verwerfen. Dieselbe Bedingung und derselbe Grund wie in
     * ActionWorldCraft.SetupAction().
     */
    override bool SetupAction(PlayerBase player, ActionTarget target, ItemBase item,
                              out ActionData action_data, Param extra_data = NULL)
    {
        if (!super.SetupAction(player, target, item, action_data, extra_data))
            return false;

        ChefZ_ProcessActionData data;
        if (!Class.CastTo(data, action_data))
            return true;

        if (!g_Game.IsDedicatedServer())
        {
            ChefZ_ProcessingStation_Base station;
            if (Class.CastTo(station, target.GetObject()))
            {
                ChefZ_Sym process = SelectProcess(station, item);
                data.m_ChefZ_ProcessHash =
                    ChefZ_ProcessingManager.Get().GetProcessPersistHash(process);
            }
        }

        return true;
    }

    /**
     * Der Server tut die Arbeit (11 §5).
     *
     * Die Reihenfolge steht so in 11 §5 und ist nicht verhandelbar:
     *
     *   1. FindTransform ERNEUT - "der Zustand kann sich geaendert haben,
     *      Client nie glauben"
     *   2. CapabilityRegistry.requires[] pruefen  (17)  -> im ProcessRunner
     *   3. ChefZ_BeginJob() ODER sofortige Anwendung bei kurzer Dauer
     *
     * Die Aufteilung nach Ausfuehrungsform:
     *
     *   STATION_ACTION  sofort. Der Fortschrittsbalken der AKTION war die
     *                   Wartezeit; ein zweiter Balken an der Station waere
     *                   dieselbe Wartezeit ein zweites Mal.
     *   STATION_TIMED   als JOB. Er laeuft weiter, wenn der Spieler geht -
     *                   das ist der ganze Zweck dieser Ausfuehrungsform
     *                   (11 §7: "Spieler verlaesst den Server waehrend
     *                   STATION_TIMED -> irrelevant, der Timer gehoert der
     *                   Station").
     */
    override void OnFinishProgressServer(ActionData action_data)
    {
        ChefZ_ProcessingStation_Base station;
        if (!Class.CastTo(station, action_data.m_Target.GetObject()))
            return;

        int hash = ChefZ_ProcessJob.NO_HASH;
        ChefZ_ProcessActionData data;
        if (Class.CastTo(data, action_data))
            hash = data.m_ChefZ_ProcessHash;

        ChefZ_Sym process = ResolveProcessFor(station, action_data.m_MainItem, hash);
        if (!ChefZ_SymbolTable.IsValid(process))
            return;

        int actorId = IdentityOf(action_data.m_Player);

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        ChefZ_CompiledProcess proc = mgr.GetProcess(process);
        if (!proc)
            return;

        if (proc.exec == ChefZ_ProcessExec.STATION_TIMED)
        {
            string beginErr;
            if (!station.ChefZ_BeginJob(process, action_data.m_MainItem, actorId, beginErr))
            {
                ChefZ_Log.Once(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.PROCESS,
                    "action.begin." + station.GetType(),
                    "Der Job an \"" + station.GetType() + "\" konnte nicht starten: "
                    + beginErr + ". Es wurde nichts veraendert.");
                return;
            }

            ApplyToolDamage(action_data, proc);
            return;
        }

        RunImmediate(station, action_data, proc, process, actorId);
    }

    /**
     * STATION_ACTION: binden, ausfuehren, fertig.
     *
     * ERNEUT gebunden, obwohl ActionCondition es bereits getan hat. Zwischen
     * Bedingung und Abschluss liegt die volle Aktionsdauer, und in dieser Zeit
     * kann ein zweiter Spieler die Station leergeraeumt haben. 11 §5 verlangt
     * genau das - und der ChefZ_ProcessRunner revalidiert danach ein drittes
     * Mal, unmittelbar vor dem Verbrauch (08 §6, Schritt 1).
     */
    protected void RunImmediate(notnull ChefZ_ProcessingStation_Base station,
                                notnull ActionData action_data,
                                notnull ChefZ_CompiledProcess proc,
                                ChefZ_Sym process, int actorId)
    {
        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        station.ChefZ_BuildContext(action_data.m_MainItem, actorId, ctx);

        ChefZ_FactSnapshot snapshot;
        array<ItemBase>    entities;
        ChefZ_FactCollector.CollectFromCargo(station, snapshot, entities);
        snapshot.SortStable();

        ChefZ_TransformMatch match;
        if (!ChefZ_ProcessingManager.Get().FindTransform(process, ctx, snapshot, null, match))
        {
            ChefZ_Log.Once(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.PROCESS,
                "action.nomatch." + station.GetType(),
                "An \"" + station.GetType() + "\" passt beim Abschluss kein Transform mehr: "
                + match.failReason + ". Es wurde nichts veraendert.");
            return;
        }

        array<ItemBase> created;
        string err;

        if (!ChefZ_ProcessRunner.Run(station, match, entities, snapshot, actorId, created, err))
            return;

        ApplyToolDamage(action_data, proc);
    }

    /**
     * Werkzeugschaden (11 §2, toolDamage).
     *
     * DecreaseHealth mit dem Wert aus den Daten - dieselbe Methode, die
     * Vanilla bei ActionSawPlanks benutzt. Bei toolDamage = 0 passiert nichts,
     * und das ist der Normalfall fuer Stationen, die selbst das Werkzeug sind.
     *
     * NACH dem Erfolg, nie davor: ein Werkzeug, das sich abnutzt, obwohl
     * nichts entstanden ist, waere ein Verlust ohne Gegenleistung.
     */
    protected void ApplyToolDamage(notnull ActionData action_data,
                                   notnull ChefZ_CompiledProcess proc)
    {
        if (proc.toolDamage <= 0)
            return;
        if (!action_data.m_MainItem)
            return;

        // Ueber eine float-Zwischenvariable: DecreaseHealth nimmt einen
        // float, und eine implizite Umwandlung int -> float ist in Enforce an
        // Aufrufgrenzen nirgends zugesichert.
        float damage = proc.toolDamage;
        action_data.m_MainItem.DecreaseHealth("", "", damage);
    }

    //==========================================================================
    // Uebertragung der Auswahl (Muster: ActionWorldCraft)
    //==========================================================================

    override void WriteToContext(ParamsWriteContext ctx, ActionData action_data)
    {
        super.WriteToContext(ctx, action_data);

        int hash = ChefZ_ProcessJob.NO_HASH;

        ChefZ_ProcessActionData data;
        if (Class.CastTo(data, action_data))
            hash = data.m_ChefZ_ProcessHash;

        ctx.Write(hash);
    }

    override bool ReadFromContext(ParamsReadContext ctx, out ActionReciveData action_recive_data)
    {
        if (!action_recive_data)
            action_recive_data = new ChefZ_ProcessActionReciveData();

        super.ReadFromContext(ctx, action_recive_data);

        int hash;
        if (!ctx.Read(hash))
            return false;

        ChefZ_ProcessActionReciveData recive;
        if (Class.CastTo(recive, action_recive_data))
            recive.m_ChefZ_ProcessHash = hash;

        return true;
    }

    override void HandleReciveData(ActionReciveData action_recive_data, ActionData action_data)
    {
        super.HandleReciveData(action_recive_data, action_data);

        ChefZ_ProcessActionReciveData recive;
        ChefZ_ProcessActionData       data;
        if (!Class.CastTo(recive, action_recive_data) || !Class.CastTo(data, action_data))
            return;

        data.m_ChefZ_ProcessHash = recive.m_ChefZ_ProcessHash;
    }

    //==========================================================================
    // Statische Helfer - auch vom Callback benutzt
    //==========================================================================

    /**
     * Der Prozess zu einem uebertragenen Hash, mit Rueckfall.
     *
     * Der Server GLAUBT dem Hash nicht, er benutzt ihn nur als Auswahl: der
     * Prozess muss von DIESER Station angeboten werden. Tut er das nicht -
     * alter Client, geaenderte Config, boeser Wille -, gilt der erste
     * angebotene Prozess. Der Client kann damit auswaehlen, aber nichts
     * erfinden.
     */
    static ChefZ_Sym ResolveProcessFor(notnull ChefZ_ProcessingStation_Base station,
                                       ItemBase inHands, int processHash)
    {
        if (processHash != ChefZ_ProcessJob.NO_HASH)
        {
            ChefZ_Sym asked = ChefZ_ProcessingManager.Get().ProcessFromPersistHash(processHash);
            if (station.ChefZ_SupportsProcess(asked))
                return asked;
        }

        // Kein oder kein brauchbarer Hash: der erste Prozess, der hier und
        // jetzt tatsaechlich starten koennte. Nicht schlicht processes[0] -
        // eine Station, deren erster Prozess gerade kalt ist, waere sonst
        // unbedienbar, obwohl ihr zweiter laufen wuerde.
        int count = station.ChefZ_GetProcessCount();
        for (int i = 0; i < count; i++)
        {
            ChefZ_Sym process = station.ChefZ_GetProcessAt(i);
            if (!ChefZ_SymbolTable.IsValid(process))
                continue;

            string reason;
            if (station.ChefZ_CanStart(process, inHands, 0, reason))
                return process;
        }

        if (count > 0)
            return station.ChefZ_GetProcessAt(0);

        return ChefZ_SymbolTable.INVALID;
    }

    //! Die Dauer, die der Fortschrittsbalken abbilden soll. 0, wenn sie sich
    //! nicht ermitteln laesst - der Callback nimmt dann seinen Rueckfallwert.
    static float DurationOf(notnull ChefZ_ProcessingStation_Base station,
                            ItemBase inHands, ChefZ_Sym process)
    {
        if (!ChefZ_SymbolTable.IsValid(process))
            return 0.0;

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        ChefZ_CompiledProcess proc = mgr.GetProcess(process);
        if (!proc)
            return 0.0;

        /**
         * STATION_TIMED wartet NICHT in der Aktion.
         *
         * Der Job laeuft an der Station weiter, auch ohne den Spieler
         * (11 E7). Die Aktion ist dort nur der Handgriff, der ihn anwirft -
         * und ein Fortschrittsbalken ueber vierzig Minuten waere kein
         * Handgriff, sondern eine Geiselnahme.
         */
        if (proc.exec == ChefZ_ProcessExec.STATION_TIMED)
            return ChefZ_ActionProcessAtStationCB.FALLBACK_SEC;

        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        station.ChefZ_BuildContext(inHands, 0, ctx);

        ChefZ_FactSnapshot snapshot;
        array<ItemBase>    entities;
        ChefZ_FactCollector.CollectFromCargo(station, snapshot, entities);
        snapshot.SortStable();

        ChefZ_TransformMatch match;
        if (!mgr.FindTransform(process, ctx, snapshot, null, match))
            return proc.baseDurationSec;

        return mgr.GetDuration(match, ctx);
    }

    //! 0, wenn kein Spieler beteiligt ist oder er keine Identitaet hat.
    //! Bots und Serveraktionen laufen damit als "niemand" - und
    //! ChefZ_CapabilityGate blockiert bei actorId 0 nichts (17 §3.3).
    static int IdentityOf(PlayerBase player)
    {
        if (!player)
            return 0;

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return 0;

        return identity.GetPlayerId();
    }
}
