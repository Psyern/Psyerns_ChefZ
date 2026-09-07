//==============================================================================
// ChefZ_ActionTakePortion - EINE Action fuer ALLE portionierten Gerichte
//
// Entwurf: 15 §3 (Schnittstelle), 15 §4 (ENTNAHME, Datenfluss), 15 §7
// (Fehlerverhalten), 15 E5 (eine Action, das Gericht wird zur Laufzeit
// gefragt), 15 E6 (serverseitige Revalidierung trotz ActionCondition),
// 00 §5 (nichts Autoritatives auf dem Client), 11 E1 (dasselbe Muster wie
// ChefZ_ActionProcessAtStation).
//
// ---------------------------------------------------------------------------
// E5, und warum es genau EINE Klasse ist
// ---------------------------------------------------------------------------
// "ChefZ_ActionTakePortion fragt zur Laufzeit beim Item nach. Ein neues
// portioniertes Gericht erbt von ChefZ_PortionedFood_Base und ist damit
// fertig - keine neue Action, keine Core-Aenderung, kein Eintrag irgendwo."
//
// Der Preis ist derselbe wie bei S14: Aktionstext und Dauer muessen aus DATEN
// kommen. Beides ist hier umgesetzt:
//
//   Text   ChefZ_PortionSpec.displayName, ersatzweise
//          #STR_CHEFZ_ACTION_TAKE_PORTION. Kein Gerichtenname im Code.
//   Dauer  ChefZ_PortionSpec.takeDurationSec, ersatzweise die Vorgabe aus
//          Core.json (defaultTakePortionSec).
//
// Eine Auswahl wie bei den Stationsprozessen braucht es NICHT: ein Bulk-Gericht
// hat genau eine Portionsklasse. Deshalb auch keine Variantenmechanik und
// keine uebertragene Nutzlast - der Client schickt nichts ausser der Aktion
// selbst, und der Server holt sich alles aus dem Item.
//
// ---------------------------------------------------------------------------
// Wer entscheidet was
// ---------------------------------------------------------------------------
//   CLIENT   zeigt an. ActionCondition darf grosszuegig sein.
//   SERVER   entscheidet. OnFinishProgressServer ruft ChefZ_TakePortion(),
//            und DAS revalidiert erneut (15 E6).
//
// 15 E6 nennt den Grund: "zwischen Aktionsstart und OnFinishProgressServer
// liegen Sekunden. Die Doppelentnahme durch zwei Spieler ist genau der Fall,
// den ein Exploit-Sucher zuerst probiert."
//
// ---------------------------------------------------------------------------
// Warum sie am ZIEL haengt und nicht an der Hand
// ---------------------------------------------------------------------------
// 15 §4: "Spieler zielt auf das Bulk-Gericht." Deshalb
// ContinuousInteractActionInput (m_DetectFromTarget = 1), genau wie bei
// ChefZ_ActionProcessAtStation.
//
// Folge, offen benannt: ein Bulk-Gericht, das der Spieler in der HAND haelt,
// bietet diese Aktion nicht an - Vanillas Eingabeart sammelt entweder vom Ziel
// oder vom Handitem (ActionInput.c:374-385), nicht von beidem. Das ist
// verschmerzbar, weil 15 E8 das Bulk ausdruecklich als transportables Objekt
// fuehrt: man stellt es ab oder laesst es im Topf und zielt darauf. Eine
// zweite Actionklasse fuer den Handfall waere gegen E5 ("EINE Action").
//
// KEIN CONTENT: kein Gericht, keine Schuessel, kein Anzeigetext ausser zwei
// Stringtable-Schluesseln, die nichts ueber ein Gericht sagen.
//
// Layer: 4_World.
//==============================================================================

/**
 * Der Callback. Er bestimmt die DAUER - und zwar aus den Daten.
 *
 * CAContinuousTime und nicht CAContinuousRepeat: eine Portion abfuellen ist
 * eine Zeitspanne, keine Wiederholung. Dieselbe Bauform und dieselbe
 * Begruendung wie in ChefZ_ActionProcessAtStationCB.
 */
class ChefZ_ActionTakePortionCB extends ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(ResolveTime());
    }

    protected float ResolveTime()
    {
        if (!m_ActionData || !m_ActionData.m_Target)
            return ChefZ_PortionLimits.DEFAULT_TAKE_SEC;

        ChefZ_PortionedFood_Base bulk;
        if (!Class.CastTo(bulk, m_ActionData.m_Target.GetObject()))
            return ChefZ_PortionLimits.DEFAULT_TAKE_SEC;

        return ChefZ_ActionTakePortion.DurationOf(bulk);
    }
}

//------------------------------------------------------------------------------

class ChefZ_ActionTakePortion extends ActionContinuousBase
{
    //! Rueckfalltext, wenn eine Spec keinen displayName fuehrt. Er sagt
    //! "Portion entnehmen" und nichts ueber ein Gericht.
    static const string FALLBACK_TEXT = "#STR_CHEFZ_ACTION_TAKE_PORTION";

    //! Einheit hinter dem Zaehler im Tooltip ("3 / 8 Portionen"). Wird von
    //! ChefZ_PortionedFood_Base.GetTooltip() gelesen und steht hier, weil
    //! beide Texte zusammengehoeren und zusammen gepflegt werden sollen.
    static const string PORTIONS_TEXT = "#STR_CHEFZ_PORTIONS";

    /**
     * Die Spec des Ziels, das gerade im Fadenkreuz liegt.
     *
     * ANZEIGE, AUSSCHLIESSLICH. GetText() bekommt von Vanilla kein Ziel
     * uebergeben (ActionBase.GetText() ist parameterlos), und der einzige
     * Punkt, an dem das Ziel bekannt ist, ist OnActionInfoUpdate. Dieselbe
     * Bauform und derselbe Grund wie s_ClientProcesses in
     * ChefZ_ActionProcessAtStation - nur ohne Varianten, also ohne statische
     * Liste.
     *
     * OHNE ref: Eigentuemer der Spec ist die Registry des
     * ChefZ_PortionManager, und die lebt laenger als jede Actioninstanz. Ein
     * ref waere hier ein Zyklus in Wartestellung (dieselbe Ueberlegung wie
     * bei ChefZ_MatchResult.recipe).
     *
     * Nicht autoritativ und kann es nicht sein: was geschieht, entscheidet
     * OnFinishProgressServer, und der holt die Spec beim Item neu.
     */
    protected ChefZ_PortionSpec m_ChefZ_InfoSpec;

    void ChefZ_ActionTakePortion()
    {
        m_CallbackClass   = ChefZ_ActionTakePortionCB;
        m_CommandUID      = DayZPlayerConstants.CMD_ACTIONFB_CRAFTING;
        m_FullBody        = true;
        m_StanceMask      = DayZPlayerConstants.STANCEMASK_ERECT;
        m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
        m_Text            = FALLBACK_TEXT;
        m_LockTargetOnUse = false;
    }

    //! Zielbasierte Eingabe - siehe Dateikopf.
    override typename GetInputType()
    {
        return ContinuousInteractActionInput;
    }

    override void CreateConditionComponents()
    {
        // CCINone: das Item in der Hand ist gleichgueltig. Der Behaelter, den
        // die Entnahme spaeter braucht (16), wird NICHT ueber diese Bedingung
        // geprueft - eine Bedingung auf das Handitem verlangte, ihn in der
        // Hand zu halten, und 16 §3 sucht ausdruecklich auch im Inventar.
        m_ConditionItem   = new CCINone;

        // CCTNonRuined: aus einem ruinierten Gericht wird nichts entnommen.
        m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
    }

    override bool HasProgress()
    {
        return true;
    }

    //==========================================================================
    // Bedingung (15 §4) - Client-Vorschau UND erste Serverpruefung
    //==========================================================================

    /**
     * Erscheint die Aktion?
     *
     * Die Reihenfolge ist nach KOSTEN gewaehlt, nicht nach Aussagekraft -
     * dieselbe Ausnahme und derselbe Grund wie bei
     * ChefZ_ActionProcessAtStation: diese Methode laeuft bei JEDEM
     * Zielwechsel des Fadenkreuzes, und der weitaus haeufigste Ausgang ist
     * "das Ziel ist gar kein Portionsgericht". Ein Cast muss genuegen, um das
     * zu erkennen.
     *
     * ChefZ_IsBulk() beantwortet danach beide Fragen auf einmal: ist die
     * Klasse als Portionsgericht deklariert, und ist noch etwas drin. 15 §7:
     * "portionsLeft <= 0 -> Action erscheint nicht; das Item bleibt als
     * normales Item verzehrbar."
     *
     * Die Behaelterbedingung (16) wird seit S17 MITGEPRUEFT, und zwar
     * clientseitig: ChefZ_BuildPortionRequest laesst
     * ChefZ_ContainerService.FillRequest() die verfuegbaren Behaelter
     * eintragen, und CanTakePortion() beantwortet damit auch die Frage "habe
     * ich ueberhaupt eine Schuessel". Genau dafuer stehen die Behaelter in der
     * GAME-CONFIG und nicht in JSON (16 §3.1): der Client liest Rang 1
     * garantiert.
     *
     * Verbindlich ist das NICHT. Es ist eine Vorschau, damit die Aktion nicht
     * erscheint, wenn sie ohnehin scheitern wuerde. Entschieden wird in
     * OnFinishProgressServer -> ChefZ_TakePortion(), und der revalidiert
     * vollstaendig (15 E6, 16 §5 letzter Absatz).
     */
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!target)
            return false;

        ChefZ_PortionedFood_Base bulk;
        if (!Class.CastTo(bulk, target.GetObject()))
            return false;

        if (!bulk.ChefZ_IsBulk())
            return false;

        ChefZ_PortionRequest req = new ChefZ_PortionRequest();
        if (!bulk.ChefZ_BuildPortionRequest(player, req))
            return false;

        string why;
        if (ChefZ_PortionManager.Get().CanTakePortion(req, why))
            return true;

        // 15 §7: "Kein passender Behaelter im Inventar -> Action erscheint
        // nicht; Grund im Aktionstext und auf DEBUG." Kein WARN: eine fehlende
        // Schuessel ist ein Spielzustand, kein Fehler - und diese Methode
        // laeuft bei jedem Zielwechsel.
        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PORTION, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Once(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.PORTION, "action.cond." + bulk.GetType(), "Die Entnahme an \"" + bulk.GetType() + "\" erscheint nicht: " + why + ".");
        }

        return false;
    }

    /**
     * Der Aktionstext (15 E5: aus der PortionSpec, nicht aus Code).
     *
     * Ohne displayName der Rueckfalltext aus der Stringtable. Der Core
     * enthaelt dadurch keinen sichtbaren Text, der irgendetwas ueber ein
     * Gericht aussagt - er sagt "Portion entnehmen".
     *
     * m_Target ist auf dem Server nicht gesetzt, wenn Vanilla den Text fuer
     * die Anzeige holt; dann greift ebenfalls der Rueckfall. Das ist
     * folgenlos, weil der Text ausschliesslich clientseitig sichtbar ist.
     */
    override string GetText()
    {
        ChefZ_PortionSpec spec = SpecOfTarget();
        if (!spec || spec.displayName == "")
            return FALLBACK_TEXT;
        return spec.displayName;
    }

    override void OnActionInfoUpdate(PlayerBase player, ActionTarget target, ItemBase item)
    {
        m_ChefZ_InfoSpec = null;

        if (target)
        {
            ChefZ_PortionedFood_Base bulk;
            if (Class.CastTo(bulk, target.GetObject()))
                m_ChefZ_InfoSpec = bulk.ChefZ_GetPortionSpec();
        }

        m_Text = GetText();
    }

    protected ChefZ_PortionSpec SpecOfTarget()
    {
        return m_ChefZ_InfoSpec;
    }

    //==========================================================================
    // Ausfuehrung (15 §4, SERVER-AUTORITATIV)
    //==========================================================================

    /**
     * Der Server tut die Arbeit.
     *
     * Diese Methode ist bewusst DUENN: sie castet, ruft und meldet. Die
     * gesamte Transaktion - revalidieren, erzeugen, uebertragen,
     * dekrementieren, Quelle aufloesen - steht in
     * ChefZ_PortionedFood_Base.ChefZ_TakePortion(), also AM ITEM.
     *
     * Grund: dieselbe Entnahme muss auch ohne Aktion moeglich sein (Adminwerkzeug,
     * ein Comp-Modul, ein spaeteres Serviersystem). Laege die Reihenfolge hier,
     * gaebe es sie beim zweiten Aufrufer ein zweites Mal - und Invariante I5
     * haette dann zwei Fassungen.
     *
     * 15 §7, "zwei Spieler entnehmen gleichzeitig": DayZ-Script ist
     * einstraengig, OnFinishProgressServer laeuft also sequenziell. Der zweite
     * Aufruf sieht portions == 0 und bricht wirkungslos ab. Keine
     * Doppelentnahme.
     */
    override void OnFinishProgressServer(ActionData action_data)
    {
        if (!action_data || !action_data.m_Target)
            return;

        ChefZ_PortionedFood_Base bulk;
        if (!Class.CastTo(bulk, action_data.m_Target.GetObject()))
            return;

        ItemBase portion;
        string   err;

        if (bulk.ChefZ_TakePortion(action_data.m_Player, portion, err))
            return;

        // Kein WARN: der haeufigste Grund ist "ein anderer war schneller", und
        // das ist ein Spielzustand. Die echten Fehlerfaelle - Erzeugung
        // gescheitert, Klasse fehlt - melden sich in ChefZ_TakePortion selbst
        // und mit mehr Zusammenhang.
        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PORTION, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.PORTION, "Entnahme an \"" + bulk.GetType() + "\" ohne Wirkung beendet: " + err + ". Es wurde nichts veraendert.");
        }
    }

    //==========================================================================
    // Statische Helfer - auch vom Callback benutzt
    //==========================================================================

    /**
     * Die Dauer, die der Fortschrittsbalken abbilden soll.
     *
     * Nie 0: ChefZ_PortionSpec.EffectiveTakeSeconds() zieht die Untergrenze.
     * 15 E7 wuenscht ausdruecklich eine fast unsichtbare Dauer fuer
     * Tellergerichte - "fast" ist hier woertlich zu nehmen, weil
     * CAContinuousTime die Zeit als Nenner fuehrt.
     */
    static float DurationOf(notnull ChefZ_PortionedFood_Base bulk)
    {
        ChefZ_PortionSpec spec = bulk.ChefZ_GetPortionSpec();
        if (!spec)
            return ChefZ_PortionManager.Get().GetDefaultTakeSeconds();
        return spec.EffectiveTakeSeconds();
    }
}
