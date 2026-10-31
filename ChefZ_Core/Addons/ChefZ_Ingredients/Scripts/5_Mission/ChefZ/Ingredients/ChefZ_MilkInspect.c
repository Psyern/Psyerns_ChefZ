//==============================================================================
// Name der Milch in Inspektion und Tooltip (31.10.2026).
//
// InspectMenuNew.UpdateItemInfoLiquidType ist statisch und kennt nur Vanillas
// Fluessigkeiten; fuer alles andere schreibt sie "ERROR" (InspectMenuNew.c:314).
// Beide Aufrufer werden hier nachgezogen.
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_MilkInspect
{
	static void Apply(Widget root, EntityAI item)
	{
		if (!root || !item)
			return;
		ItemBase asItem = ItemBase.Cast(item);
		if (!asItem || !asItem.IsLiquidContainer() || asItem.GetLiquidType() != LIQUID_CHEFZ_MILK || asItem.GetQuantity() <= 0)
			return;
		InspectMenuNew.WidgetTrySetText(root, "ItemLiquidTypeWidget", "#STR_CHEFZ_LIQUID_MILK", Colors.COLOR_LIQUID);
	}
}

modded class InspectMenuNew
{
	override void SetItem(EntityAI item)
	{
		super.SetItem(item);
		ChefZ_MilkInspect.Apply(layoutRoot, item);
	}
}

modded class ItemManager
{
	override void PrepareTooltip(EntityAI item, int x = 0, int y = 0)
	{
		super.PrepareTooltip(item, x, y);
		ChefZ_MilkInspect.Apply(m_TooltipWidget, item);
	}
}
