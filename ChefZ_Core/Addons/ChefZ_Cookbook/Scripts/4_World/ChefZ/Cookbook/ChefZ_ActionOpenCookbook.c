//==============================================================================
// ChefZ_ActionOpenCookbook - "Kochbuch aufschlagen" am Buch selbst
//
// Entwurf: ChefZ_Cookbook_Workflow §6.3, letzter Absatz: "Sie kostet wenig und
// macht die Funktion auffindbar, ohne dass der Spieler die Tastenbelegung
// kennen muss."
//
// ---------------------------------------------------------------------------
// WARUM DIE AKTION AUF DEM CLIENT ENDET
// ---------------------------------------------------------------------------
// Sie oeffnet ein Fenster, und Fenster gibt es nur auf dem Client. Der Server
// hat hier nichts zu entscheiden: das Wissen liegt bereits bei ihm, und wer
// sein eigenes Buch aufschlaegt, veraendert nichts an der Welt.
//
// Deshalb steht die ganze Wirkung in OnEndClient. OnEndServer reicht nur an
// super weiter und tut sonst nichts - das ist kein Vergessen, aber super darf
// nicht fehlen (Begruendung unten an der Methode).
//
// ---------------------------------------------------------------------------
// SIE MUSS REGISTRIERT WERDEN, SONST GIBT ES SIE NICHT
// ---------------------------------------------------------------------------
// ActionConstructor.RegisterActions() ist eine Handliste; was dort fehlt, legt
// ConstructActions() nie an, und AddAction() findet es nicht. Der Eintrag steht
// in ChefZ_ActionRegistration.c nebenan, der Validator "chefzaction" wacht
// darueber.
//
// Layer: 4_World. Keine Dabs-Referenz: der Aufruf geht an ChefZ_CookbookOpener,
// und ob dahinter eine Oberflaeche haengt, geht diese Aktion nichts an.
//==============================================================================

class ChefZ_ActionOpenCookbook : ActionSingleUseBase
{
    void ChefZ_ActionOpenCookbook()
    {
        m_CommandUID    = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
        m_StanceMask    = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
        m_FullBody      = false;
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem   = new CCINonRuined();
        m_ConditionTarget = new CCTNone();
    }

    override typename GetInputType()
    {
        return ContinuousDefaultActionInput;
    }

    override string GetText()
    {
        return "#STR_CHEFZ_ACTION_OPEN_COOKBOOK";
    }

    /**
     * Nur mit dem Buch in der Hand, und nur fuer einen echten Spieler.
     *
     * Die Besitzpruefung aus §6.3 ("Das Buch muss im Inventar liegen") ist hier
     * geschenkt: wer die Aktion am Buch ausloest, HAT es. Fuer das
     * Tastenkuerzel gilt sie trotzdem - dort steht sie in
     * ChefZ_CookbookItem.CarriedBy.
     */
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!player || !item)
            return false;
        return ChefZ_CookbookItem.Cast(item) != null;
    }

    /**
     * Der Server hat hier inhaltlich nichts zu tun - aber super MUSS laufen.
     *
     * AnimatedActionBase.OnStartServer setzt fuer jede nicht-instante Aktion
     * SetPerformedActionID(GetID()), und NUR AnimatedActionBase.OnEndServer
     * setzt sie mit SetPerformedActionID(-1) wieder zurueck
     * (1.30 scripts/4_World/DayZ/Classes/UserActionsComponent/AnimatedActionBase.c:504-516,
     * in 1.29 identisch bei :488-501). Im gesamten 1.30-Baum gibt es keine
     * zweite Stelle, die zuruecksetzt.
     *
     * Ohne super bliebe GetPerformedActionID() nach dem Aufschlagen ungleich
     * -1, bis irgendeine andere animierte Aktion endet. ActionForceConsume.c:63
     * liest den Wert und liesse solange kein Fuettern oder Traenken durch
     * andere Spieler zu; ItemBase.c:1172 und :1203 werten ihn ebenfalls aus.
     */
    override void OnEndServer(ActionData action_data)
    {
        super.OnEndServer(action_data);
        // Darueber hinaus absichtlich leer. Siehe Kopf: die Wirkung steht in
        // OnEndClient, der Server entscheidet hier nichts.
    }

    override void OnEndClient(ActionData action_data)
    {
        if (!action_data || !action_data.m_Player)
            return;
        ChefZ_CookbookOpener.OpenFor(action_data.m_Player);
    }
}
