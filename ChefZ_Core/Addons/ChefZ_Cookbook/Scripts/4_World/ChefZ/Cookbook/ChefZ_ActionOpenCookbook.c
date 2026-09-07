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
// Deshalb steht die ganze Wirkung in OnEndClient. OnEndServer bleibt leer -
// nicht vergessen, sondern absichtlich.
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

    override void OnEndServer(ActionData action_data)
    {
        // Absichtlich leer. Siehe Kopf: der Server hat hier nichts zu tun.
    }

    override void OnEndClient(ActionData action_data)
    {
        if (!action_data || !action_data.m_Player)
            return;
        ChefZ_CookbookOpener.OpenFor(action_data.m_Player);
    }
}
