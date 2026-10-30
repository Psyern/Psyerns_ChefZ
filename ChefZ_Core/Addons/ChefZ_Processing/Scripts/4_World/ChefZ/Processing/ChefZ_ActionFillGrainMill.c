//==============================================================================
// "In die Muehle schuetten" (30.10.2026, Spieltest: "Getreidemuehle -
// Interaktion fehlt (Inventar befuellen mit Corn oder Weizen)").
//
// Die Muehle nimmt Weizen und Mais schon immer ueber ihren Cargo an
// (ChefZ_GrainMill.CanReceiveItemIntoCargo). Nur fuehrte im Spiel kein Weg
// dorthin ausser dem Ziehen im Inventar-Bildschirm. Diese Aktion ist der
// direkte Weg: Garbe oder Kolben in der Hand, auf die Muehle schauen,
// einfuellen. Gemahlen wird danach wie bisher mit der Stationsaktion.
//
// Server-autoritativ: die Bewegung laeuft in OnExecuteServer ueber
// ServerTakeEntityToTargetCargo, die Hand des Spielers synchronisiert die
// Engine. Kein clientseitiges Vorhersagen wie bei Vanillas ActionAttach -
// einfuellen ist nicht zeitkritisch.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_ActionFillGrainMill extends ActionSingleUseBase
{
    void ChefZ_ActionFillGrainMill()
    {
        m_Text = "#STR_CHEFZ_ACTION_FILLMILL";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem   = new CCINonRuined();
        m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
        m_CommandUID      = DayZPlayerConstants.CMD_ACTIONMOD_ATTACHITEM;
        m_StanceMask      = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        ChefZ_GrainMill mill = ChefZ_GrainMill.Cast(target.GetObject());
        return CanFill(mill, item);
    }

    override void OnExecuteServer(ActionData action_data)
    {
        ClearInventoryReservationEx(action_data);

        ChefZ_GrainMill mill = ChefZ_GrainMill.Cast(action_data.m_Target.GetObject());
        if (!CanFill(mill, action_data.m_MainItem))
            return;

        action_data.m_Player.ServerTakeEntityToTargetCargo(mill, action_data.m_MainItem);
    }

    //! Torwaechter der Muehle und ein freier Platz im Cargo. Statisch, damit
    //! der Autotest dieselbe Bedingung prueft wie die Aktion.
    static bool CanFill(ChefZ_GrainMill mill, EntityAI item)
    {
        if (!mill || !item)
            return false;
        if (!mill.GetInventory() || !mill.GetInventory().GetCargo())
            return false;
        if (!mill.CanReceiveItemIntoCargo(item))
            return false;

        InventoryLocation il = new InventoryLocation();
        return mill.GetInventory().FindFreeLocationFor(item, FindInventoryLocationType.CARGO, il);
    }
}

modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ChefZ_ActionFillGrainMill);
    }
}

//! Weizen und Mais bringen die Aktion mit: Aktionen mit Item in der Hand
//! haengen in DayZ am Item, nicht am Ziel.
modded class ChefZ_Wheat
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ChefZ_ActionFillGrainMill);
    }
}

modded class ChefZ_Corn
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ChefZ_ActionFillGrainMill);
    }
}
