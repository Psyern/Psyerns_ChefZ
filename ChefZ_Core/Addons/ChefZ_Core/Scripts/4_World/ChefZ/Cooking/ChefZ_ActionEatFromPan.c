//==============================================================================
// Direkt aus der Pfanne essen (31.10.2026, Spieltest).
//
// Die Aktion haengt am Gefaess in der Hand (die Slices tragen sie per AddAction ein),
// gegessen wird aber das Essen darin:
// zuerst das Gericht im Slot ChefZ_Meal, sonst das erste Essbare im Laderaum.
// Bulk-Gerichte bleiben aussen vor, die werden portioniert (ChefZ_PortionedFood_Base).
//
// Vanillas Essaktion rechnet alles ueber action_data.m_MainItem. Das bleibt fuer
// den Aktionsablauf die Pfanne und zeigt nur beim Rechnen kurz auf das Essen.
//
// Layer: 4_World.
//==============================================================================
class ChefZ_PanFood
{
	static Edible_Base Find(ItemBase pan)
	{
		if (!pan)
			return null;

		GameInventory inv = pan.GetInventory();
		int slotId = InventorySlots.GetSlotIdFromString(ChefZ_Applicator.MEAL_SLOT);
		if (inv.HasAttachmentSlot(slotId))
		{
			Edible_Base meal = Edible_Base.Cast(inv.FindAttachment(slotId));
			if (IsEatable(meal))
				return meal;
		}

		CargoBase cargo = inv.GetCargo();
		if (!cargo)
			return null;

		for (int i = 0; i < cargo.GetItemCount(); i++)
		{
			Edible_Base food = Edible_Base.Cast(cargo.GetItem(i));
			if (IsEatable(food))
				return food;
		}
		return null;
	}

	protected static bool IsEatable(Edible_Base food)
	{
		if (!food || food.IsRuined() || food.IsInherited(ChefZ_PortionedFood_Base))
			return false;
		return food.GetQuantity() > 0;
	}
}

class ChefZ_CAEatFromPan extends CAContinuousQuantityEdible
{
	protected Edible_Base m_ChefZ_Food;
	protected bool m_ChefZ_FoodChosen;

	Edible_Base ChefZ_GetFood()
	{
		return m_ChefZ_Food;
	}

	override void Setup(ActionData action_data)
	{
		ItemBase pan = action_data.m_MainItem;
		// Festgelegt beim ersten Bissen: ist das Gericht leer, endet die Aktion,
		// statt ungefragt mit dem naechsten Essen in der Pfanne weiterzumachen.
		if (!m_ChefZ_FoodChosen)
		{
			m_ChefZ_Food = ChefZ_PanFood.Find(pan);
			m_ChefZ_FoodChosen = true;
		}

		if (!m_ChefZ_Food)
		{
			m_ItemQuantity = 0;
			return;
		}

		action_data.m_MainItem = m_ChefZ_Food;
		super.Setup(action_data);
		action_data.m_MainItem = pan;
	}

	override void CalcAndSetQuantity(ActionData action_data)
	{
		if (!m_ChefZ_Food)
			return;

		ItemBase pan = action_data.m_MainItem;
		action_data.m_MainItem = m_ChefZ_Food;
		super.CalcAndSetQuantity(action_data);
		action_data.m_MainItem = pan;
	}
}

class ChefZ_ActionEatFromPanCB extends ActionEatBigCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new ChefZ_CAEatFromPan(UAQuantityConsumed.EAT_BIG, UATimeSpent.DEFAULT);
	}
}

class ChefZ_ActionEatFromPan extends ActionEatBig
{
	void ChefZ_ActionEatFromPan()
	{
		m_CallbackClass = ChefZ_ActionEatFromPanCB;
	}

	//! Vanillas Pruefungen (Maske, gefroren) sollen das Essen sehen, nicht die Pfanne.
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		Edible_Base food = ChefZ_PanFood.Find(item);
		if (!food)
			return false;

		return super.ActionCondition(player, target, food);
	}

	//! ActionConsume.OnEndServer setzt die Menge und uebertraegt Erreger auf m_MainItem.
	override void OnEndServer(ActionData action_data)
	{
		ItemBase pan = action_data.m_MainItem;
		ChefZ_CAEatFromPan component = ChefZ_CAEatFromPan.Cast(action_data.m_ActionComponent);
		action_data.m_MainItem = null;
		if (component)
			action_data.m_MainItem = component.ChefZ_GetFood();

		super.OnEndServer(action_data);
		action_data.m_MainItem = pan;
	}
}
