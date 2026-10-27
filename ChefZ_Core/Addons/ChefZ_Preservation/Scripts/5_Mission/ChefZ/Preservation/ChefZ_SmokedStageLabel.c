//==============================================================================
// ChefZ_SmokedStageLabel - "Geraeuchert" statt "Getrocknet" im Inventar
// (27.10.2026, Spieltest "wenn das Item geraeuchert ist, wird es umgewandelt
// in getrocknetes Fleisch").
//
// Vanilla kennt keine Raeucherstufe. Der ChefZ-Zustand SMOKED projiziert
// deshalb auf FoodStageType.DRIED (ChefZ_StateManager), und Vanillas
// InspectMenuNew.UpdateItemInfoFoodStage schreibt dafuer "Getrocknet" in
// Tooltip und Untersuchen-Fenster. Die Stufe bleibt fuer Verderb und Optik
// DRIED; nur das Etikett geraeucherter Items wird danach ersetzt.
//
// Beide Wege, auf denen Vanilla UpdateItemInfo ruft, werden nachgezogen:
// ItemManager.PrepareTooltip (Tooltip) und InspectMenuNew.SetItem.
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_SmokedStageLabel
{
    //! 27.10.2026: Raeuchern laesst Filet und Fleisch bestehen und setzt nur
    //! den Zustand SMOKED (Smoking.json) - die Sammelklassen Raeucherfisch und
    //! Raeucherfleisch gibt es nicht mehr. Erkannt wird deshalb am Zustand.
    static bool IsSmokedProduct(EntityAI item)
    {
        if (!item)
            return false;
        if (item.IsKindOf("ChefZ_SmokedSausage"))
            return true;
        ItemBase ib = ItemBase.Cast(item);
        if (!ib || !ChefZ_ItemStateComponent.IsManaged(ib))
            return false;
        ChefZ_Sym smoked = ChefZ_SymbolTable.Lookup("SMOKED");
        if (!ChefZ_SymbolTable.IsValid(smoked))
            return false;
        return ChefZ_ItemStateComponent.GetState(ib) == smoked;
    }

    static void Apply(Widget root, EntityAI item)
    {
        if (!root || !IsSmokedProduct(item))
            return;
        Edible_Base food = Edible_Base.Cast(item);
        if (!food || !food.GetFoodStage())
            return;
        // Verbrannt und verdorben behalten Vanillas Etikett.
        if (food.GetFoodStage().GetFoodStageType() != FoodStageType.DRIED)
            return;
        InspectMenuNew.WidgetTrySetText(root, "ItemFoodStageWidget", "#STR_CHEFZ_STAGE_SMOKED", Colors.COLOR_DRIED);
    }
}

modded class InspectMenuNew
{
    override void SetItem(EntityAI item)
    {
        super.SetItem(item);
        ChefZ_SmokedStageLabel.Apply(layoutRoot, item);
    }
}

modded class ItemManager
{
    override void PrepareTooltip(EntityAI item, int x = 0, int y = 0)
    {
        super.PrepareTooltip(item, x, y);
        ChefZ_SmokedStageLabel.Apply(m_TooltipWidget, item);
    }
}
