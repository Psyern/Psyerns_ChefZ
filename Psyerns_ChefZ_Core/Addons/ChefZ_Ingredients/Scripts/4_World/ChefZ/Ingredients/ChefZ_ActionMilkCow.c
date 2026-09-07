//==============================================================================
// ChefZ_ActionMilkCow - eine Kuh melken, Milchkanne in den Haenden
//
// Auftrag vom 07.09.2026: "mit ChefZ_MilkCan in der Hand die Funktion bekommen,
// mit einer Kuh zu interagieren und Milch erhalten".
//
// Damit bekommt ChefZ_MilkCan zum ersten Mal eine Aufgabe. Bis hierher war sie
// das, was Dairy_Transforms.json ueber sie schreibt: "ein leeres Traggut ohne
// Zutatendatensatz", geliefert am 30.08. und seither wartend.
//
// ---------------------------------------------------------------------------
// WAS HIER "MILCH" IST
// ---------------------------------------------------------------------------
// Vanillas PowderedMilk, und nichts anderes. Das ist keine Bequemlichkeit,
// sondern die Entscheidung vom 29.08.2026: eine eigene ChefZ-Milchklasse gab
// es, sie war reiner Loot am selben Karton wie PowderedMilk und wurde
// gestrichen. TR_MilkToCream am Butterfass nimmt ausdruecklich
// {"cls": "PowderedMilk"}, zwei Stueck je Sahne. Wer hier etwas anderes
// erzeugte, haenge die Melkkette neben die Molkerei statt davor.
//
// ---------------------------------------------------------------------------
// WARUM Animal_BosTaurusF UND NICHT Animal_BosTaurus
// ---------------------------------------------------------------------------
// AnimalBase.c:38 und :62 - "class Animal_BosTaurus extends AnimalBase" und
// "class Animal_BosTaurusF extends Animal_BosTaurus {}". Die Kuh ist ein KIND
// des Rinds. Ein Cast auf die Basis naehme deshalb auch den Bullen an; der
// Cast auf Animal_BosTaurusF nimmt genau die weiblichen Tiere, samt aller
// Farbvarianten, die davon erben.
//
// Das ist dieselbe Regel, die ChefZ_Meat/config.cpp fuer das Zerlegen
// festhaelt: Basisklassen nennen, Varianten die Kette aufloesen lassen.
//
// ---------------------------------------------------------------------------
// LEBEND, NICHT TOT
// ---------------------------------------------------------------------------
// ActionSkinning.c:52 prueft "!targetObject.IsAlive()". Hier gilt das
// Gegenteil: ein totes Tier wird zerlegt, nicht gemolken. Beide Aktionen
// koennen damit am selben Tier haengen, ohne sich in die Quere zu kommen.
//
// Layer: 4_World. Content, deshalb in ChefZ_Ingredients und nicht im Core
// (Regel 2).
//==============================================================================

class ChefZ_ActionMilkCowCB extends ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(ChefZ_ActionMilkCow.MILK_SECONDS);
    }
}

class ChefZ_ActionMilkCow extends ActionContinuousBase
{
    //! Wie lange das Melken dauert. Balancingwert - er steht hier als einzige
    //! Zahl, damit ihn eine Aenderung an einer Stelle trifft.
    static const float MILK_SECONDS = 8.0;

    //! Was eine Kuh je Melkgang hergibt.
    static const string MILK_CLASS = "PowderedMilk";

    static const string TEXT = "#STR_CHEFZ_ACTION_MILK_COW";

    void ChefZ_ActionMilkCow()
    {
        m_CallbackClass   = ChefZ_ActionMilkCowCB;
        m_CommandUID      = DayZPlayerConstants.CMD_ACTIONFB_CRAFTING;
        m_FullBody        = true;
        m_StanceMask      = DayZPlayerConstants.STANCEMASK_CROUCH;
        m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
        m_Text            = TEXT;
        m_LockTargetOnUse = false;
    }

    override typename GetInputType()
    {
        return ContinuousInteractActionInput;
    }

    /**
     * Dieselben Komponenten, die ActionSkinning fuer ein Tier benutzt
     * (ActionSkinning.c:44-45), und nicht die eines Gegenstandsziels.
     *
     * CCTCursorNoRuinCheck peilt ueber den Cursor und rechnet mit Kopfhoehen
     * je Haltung (CCTCursorNoRuinCheck.c:6-8); ein Rind ist ein grosses,
     * bewegliches Ziel, und genau dafuer ist die Komponente da. CCTNonRuined
     * waere die Wahl fuer ein liegendes Objekt.
     *
     * CCINonRuined an der Kanne: eine zerstoerte Kanne haelt nichts. Die
     * Vorgabe des Konstruktors ist UAMaxDistances.DEFAULT
     * (CCTCursorNoRuinCheck.c:10), deshalb steht hier keine eigene Zahl.
     */
    override void CreateConditionComponents()
    {
        m_ConditionItem   = new CCINonRuined;
        m_ConditionTarget = new CCTCursorNoRuinCheck;
    }

    override bool HasProgress()
    {
        return true;
    }

    override string GetText()
    {
        return TEXT;
    }

    /**
     * Vier Bedingungen, in der Reihenfolge ihrer Kosten.
     *
     * Die Kanne selbst wird NICHT geprueft: die Aktion haengt ueber
     * ChefZ_MilkCan.SetActions() an genau diesem Item, und die Engine bietet
     * sie nur an, wenn es in den Haenden liegt. Eine zweite Pruefung hier
     * waere eine zweite Stelle, an der dieselbe Regel spaeter auseinanderlaufen
     * kann.
     */
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!target)
            return false;

        Object targetObject = target.GetObject();
        if (!targetObject)
            return false;

        if (!targetObject.IsAlive())
            return false;

        Animal_BosTaurusF cow;
        if (!Class.CastTo(cow, targetObject))
            return false;

        return cow.ChefZ_CanBeMilked();
    }

    /**
     * Serverseitig: die Kuh sperrt sich, dann entsteht die Milch.
     *
     * Reihenfolge mit Absicht. Die Sperre zuerst zu setzen kostet im
     * Fehlerfall eine Melkgelegenheit; sie zuletzt zu setzen liesse zwei
     * gleichzeitig fertige Spieler beide Milch ziehen. Der billigere Fehler
     * gewinnt (02 §8).
     */
    override void OnFinishProgressServer(ActionData action_data)
    {
        if (!action_data || !action_data.m_Target || !action_data.m_Player)
            return;

        Animal_BosTaurusF cow;
        if (!Class.CastTo(cow, action_data.m_Target.GetObject()))
            return;

        if (!cow.ChefZ_CanBeMilked())
            return;

        cow.ChefZ_MarkMilked();

        EntityAI created = action_data.m_Player.GetInventory().CreateInInventory(MILK_CLASS);
        if (created)
            return;

        // Kein Platz im Inventar: die Milch faellt vor die Fuesse, statt
        // lautlos zu verschwinden. Vanillas eigenes Muster fuer denselben
        // Fall (EntityAI.c:2141 legt bei fehlendem Platz in der Welt ab).
        vector pos = action_data.m_Player.GetPosition();
        Object dropped = g_Game.CreateObjectEx(MILK_CLASS, pos, ECE_PLACE_ON_SURFACE);

        if (!dropped)
            ChefZ_Log.Warn(ChefZ_LogChannel.CONFIG, "Melken: \"" + MILK_CLASS + "\" liess sich weder im Inventar noch in der Welt erzeugen. " + "Die Klasse fehlt oder ist scope=0 - der Melkgang bleibt ohne Ergebnis.");
    }
}
