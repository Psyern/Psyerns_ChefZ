//==============================================================================
// "pan x": spielt die Pfanne durch. Rezept-Rahmen, Laderaum, Gericht im Proxy,
// Essen aus der Pfanne. Fotos und "[VISCHECK] pan ..." im Log.
//
// Der Server arbeitet die Stufen nach Zeit ab, der Client erkennt sie am Inhalt
// der Pfanne in der Hand.
//==============================================================================
class ChefZ_VisPanServer
{
	static const int FIRST_MS = 100000;
	static const int STEP_MS = 30000;
	static const int EAT_MS = 70000;
	static const int SHOW_MS = 18000;

	protected PlayerBase m_Player;
	protected ItemBase m_Pan;
	protected ItemBase m_Meal;
	protected ref TStringArray m_Shows;
	protected float m_MealBefore;

	void ChefZ_VisPanServer(PlayerBase player)
	{
		m_Player = player;
		EntityAI old = player.GetItemInHands();
		if (old)
			g_Game.ObjectDelete(old);
		m_Shows = {"ChefZ_FarmersBreakfast", "ChefZ_SausageBreadPlate", "ChefZ_TacticalBreakfast", "ChefZ_CrispyFriedCalamari", "ChefZ_GrilledStuffedCalamari"};
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(StageRecipe, FIRST_MS, false);
	}

	protected void Clear()
	{
		array<EntityAI> all = new array<EntityAI>();
		m_Pan.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
		foreach (EntityAI item : all)
		{
			if (item != m_Pan)
				g_Game.ObjectDelete(item);
		}
	}

	void StageRecipe()
	{
		m_Pan = ItemBase.Cast(m_Player.GetHumanInventory().CreateInHands("FryingPan"));
		if (!m_Pan)
		{
			Print("[VISCHECK] pan FEHLER keine Pfanne");
			return;
		}
		CargoBase cargo = m_Pan.GetInventory().GetCargo();
		Print("[VISCHECK] pan cargo " + cargo.GetWidth().ToString() + "x" + cargo.GetHeight().ToString());
		EntityAI bread = m_Pan.GetInventory().CreateInInventory("ChefZ_Bread");
		EntityAI honey = m_Pan.GetInventory().CreateInInventory("Honey");
		Print("[VISCHECK] pan recipe bread=" + (bread != null).ToString() + " honey=" + (honey != null).ToString());

		TStringArray soup = new TStringArray();
		g_Game.ConfigGetTextArray("CfgVehicles ChefZ_PumpkinSoupBowl inventorySlot", soup);
		Print("[VISCHECK] pan soup slots=" + soup.ToString());
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(StageExtra, STEP_MS, false);
	}

	void StageExtra()
	{
		EntityAI extra = m_Pan.GetInventory().CreateInInventory("Apple");
		Print("[VISCHECK] pan extra apple=" + (extra != null).ToString());
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(StageMeal, STEP_MS, false);
	}

	void StageMeal()
	{
		Clear();
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CreateMeal, 2000, false);
	}

	void CreateMeal()
	{
		m_Meal = ItemBase.Cast(m_Pan.GetInventory().CreateAttachment("ChefZ_MeatDumplings"));
		if (!m_Meal)
		{
			Print("[VISCHECK] pan FEHLER kein Gericht im Slot");
			return;
		}
		m_MealBefore = m_Meal.GetQuantity();
		Print("[VISCHECK] pan meal qty=" + m_MealBefore.ToString());
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(StageEaten, EAT_MS, false);
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(LogMeal, 5000, true);
	}

	void LogMeal()
	{
		float qty = -1;
		if (m_Meal)
			qty = m_Meal.GetQuantity();
		Print("[VISCHECK] pan qty=" + qty.ToString() + " energy=" + m_Player.GetStatEnergy().Get().ToString() + " inHands=" + (m_Player.GetItemInHands() == m_Pan).ToString());
	}

	void StageEaten()
	{
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(LogMeal);
		float now = -1;
		if (m_Meal)
			now = m_Meal.GetQuantity();
		Print("[VISCHECK] pan eaten before=" + m_MealBefore.ToString() + " after=" + now.ToString() + " panRuined=" + m_Pan.IsRuined().ToString());
		NextShow();
	}

	void NextShow()
	{
		Clear();
		if (m_Shows.Count() == 0)
		{
			Print("[VISCHECK] pan server done");
			return;
		}
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CreateShow, 2000, false);
	}

	void CreateShow()
	{
		string cls = m_Shows.Get(0);
		m_Shows.RemoveOrdered(0);
		EntityAI show = m_Pan.GetInventory().CreateAttachment(cls);
		Print("[VISCHECK] pan show " + cls + " ok=" + (show != null).ToString());
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextShow, SHOW_MS, false);
	}
}

class ChefZ_VisPanClient
{
	protected int m_Stage;
	protected float m_Wait;
	protected string m_LastMeal;
	protected int m_Shows;
	protected bool m_WantInventory;
	protected int m_Tries;

	protected MissionGameplay Gameplay()
	{
		return MissionGameplay.Cast(g_Game.GetMission());
	}

	protected void Log(ItemBase pan, string tag)
	{
		string line = "[VISCHECK] pan " + tag + ":";
		CargoBase cargo = pan.GetInventory().GetCargo();
		for (int i = 0; i < cargo.GetItemCount(); i++)
		{
			ItemBase item = ItemBase.Cast(cargo.GetItem(i));
			line = line + " " + item.GetType() + "=" + ChefZ_CookMatchHint.IsMatched(item).ToString();
		}
		Print(line);
	}

	protected void Next(int stage)
	{
		m_Stage = stage;
		m_Wait = 0;
	}

	//! Das Oeffnen kann scheitern, solange die Hand noch beschaeftigt ist: jedes Bild neu versuchen.
	protected void KeepInventory()
	{
		bool open = g_Game.GetUIManager().FindMenu(MENU_INVENTORY) != null;
		if (m_WantInventory && !open)
		{
			UIScriptedMenu menu = g_Game.GetUIManager().GetMenu();
			string menuName = "none";
			if (menu)
				menuName = menu.ToString();
			PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
			m_Tries++;
			if (m_Tries % 60 == 1)
				Print("[VISCHECK] pan open try=" + m_Tries.ToString() + " menu=" + menuName + " can=" + player.GetHumanInventory().CanOpenInventory().ToString() + " soft=" + player.IsInventorySoftLocked().ToString() + " invMenu=" + Gameplay().GetInventory().ToString());
			Gameplay().ShowInventory();
		}
		if (!m_WantInventory && open)
			Gameplay().HideInventory();
	}

	void Update(float dt)
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;
		ItemBase pan = ItemBase.Cast(player.GetItemInHands());
		if (!pan || pan.GetType() != "FryingPan")
			return;

		KeepInventory();

		m_Wait += dt;
		CargoBase cargo = pan.GetInventory().GetCargo();
		int count = cargo.GetItemCount();
		EntityAI meal = pan.GetInventory().FindAttachment(InventorySlots.GetSlotIdFromString("ChefZ_Meal"));

		if (m_Stage == 0 && count == 2)
		{
			m_WantInventory = true;
			Next(1);
		}
		else if (m_Stage == 1 && m_Wait > 8)
		{
			Log(pan, "match");
			Print("[VISCHECK] pan inventory=" + (g_Game.GetUIManager().FindMenu(MENU_INVENTORY) != null).ToString());
			Print("[VISCHECK] shot pan_match");
			Next(2);
		}
		else if (m_Stage == 2 && count == 3)
		{
			Next(3);
		}
		else if (m_Stage == 3 && m_Wait > 8)
		{
			Log(pan, "extra");
			Print("[VISCHECK] shot pan_extra");
			Next(4);
		}
		else if (m_Stage == 4 && meal)
		{
			Next(5);
		}
		else if (m_Stage == 5 && m_Wait > 3)
		{
			Print("[VISCHECK] shot pan_meal_inv");
			m_LastMeal = meal.GetType();
			Next(6);
		}
		else if (m_Stage == 6 && m_Wait > 2)
		{
			m_WantInventory = false;
			Next(7);
		}
		else if (m_Stage == 7 && m_Wait > 3)
		{
			Print("[VISCHECK] shot pan_meal");
			Next(8);
		}
		else if (m_Stage == 8 && m_Wait > 2)
		{
			Print("[VISCHECK] press LMB 5 pan_eat1");
			Next(9);
		}
		else if (m_Stage == 9 && m_Wait > 2)
		{
			ActionBase running = player.GetActionManager().GetRunningAction();
			Print("[VISCHECK] pan running=" + running.ToString());
			Print("[VISCHECK] shot pan_eating");
			Next(10);
		}
		else if (m_Stage == 10 && m_Wait > 12)
		{
			Print("[VISCHECK] press LMB 20 pan_eat2");
			Next(11);
		}
		else if (m_Stage == 11 && meal && meal.GetType() != m_LastMeal)
		{
			m_LastMeal = meal.GetType();
			Next(12);
		}
		else if (m_Stage == 12 && m_Wait > 3)
		{
			m_WantInventory = true;
			Next(13);
		}
		else if (m_Stage == 13 && m_Wait > 4)
		{
			Print("[VISCHECK] shot paninv_" + m_LastMeal);
			Next(14);
		}
		else if (m_Stage == 14 && m_Wait > 2)
		{
			m_WantInventory = false;
			m_Shows++;
			if (m_Shows >= 5)
				Next(15);
			else
				Next(11);
		}
		else if (m_Stage == 15 && m_Wait > 3)
		{
			Print("[VISCHECK] client done");
			Next(16);
		}
	}
}
