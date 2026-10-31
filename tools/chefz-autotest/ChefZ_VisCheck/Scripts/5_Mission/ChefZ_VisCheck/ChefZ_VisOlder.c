//==============================================================================
// "older x": Kits, Myzel, Sporen, Bienenstock, Pfanne im Dreibein, Milch trinken.
// Der Server zeigt die Stufe ueber das Item in der Hand an (Apfel, dann Milchkanne).
//==============================================================================
class ChefZ_VisOlderServer
{
	protected PlayerBase m_Player;
	protected ItemBase m_Can;
	protected ref TStringArray m_Types;

	void ChefZ_VisOlderServer(PlayerBase player)
	{
		m_Player = player;
		m_Types = {"ChefZ_MushroomShelfKit", "ChefZ_GrainMillKit", "ChefZ_Mycelium_Empty", "ChefZ_MyceliumAgaricus", "ChefZ_SporesAgaricus"};
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextItem, 100000, false);
	}

	protected void ClearHands()
	{
		EntityAI old = m_Player.GetItemInHands();
		if (old)
			g_Game.ObjectDelete(old);
	}

	void NextItem()
	{
		ClearHands();
		if (m_Types.Count() == 0)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CreateCan, 2000, false);
			return;
		}
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CreateItem, 2000, false);
	}

	void CreateItem()
	{
		string type = m_Types.Get(0);
		m_Types.RemoveOrdered(0);
		EntityAI item = m_Player.GetHumanInventory().CreateInHands(type);
		Print("[VISCHECK] older hand " + type + "=" + (item != null).ToString());
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextItem, 12000, false);
	}

	void CreateCan()
	{
		m_Can = ItemBase.Cast(m_Player.GetHumanInventory().CreateInHands("ChefZ_MilkCan"));
		if (!m_Can)
		{
			Print("[VISCHECK] older FEHLER keine Milchkanne");
			return;
		}
		m_Can.SetLiquidType(LIQUID_CHEFZ_MILK);
		m_Can.SetQuantity(5000);
		m_Player.GetStatEnergy().Set(1500);
		m_Player.GetStatWater().Set(1500);
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(LogMilk, 3000, true);
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(StopMilk, 45000, false);
	}

	void LogMilk()
	{
		float qty = -1;
		if (m_Can)
			qty = m_Can.GetQuantity();
		Print("[VISCHECK] older milk qty=" + qty.ToString() + " water=" + m_Player.GetStatWater().Get().ToString() + " energy=" + m_Player.GetStatEnergy().Get().ToString());
	}

	void StopMilk()
	{
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(LogMilk);
	}
}

class ChefZ_VisOlderClient
{
	protected string m_Last;
	protected float m_Wait;
	protected int m_Stage;
	protected bool m_Shot;

	void Update(float dt)
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		MissionGameplay gameplay = MissionGameplay.Cast(g_Game.GetMission());
		if (!player || !gameplay)
			return;
		EntityAI hand = player.GetItemInHands();
		string type = "";
		if (hand)
			type = hand.GetType();

		m_Wait += dt;
		if (m_Stage == 0)
		{
			if (type == "" || type == "FryingPan" || type == "Apple")
				return;
			bool open = g_Game.GetUIManager().FindMenu(MENU_INVENTORY) != null;
			if (type == "ChefZ_MilkCan")
			{
				if (open)
					gameplay.HideInventory();
				m_Wait = 0;
				m_Stage = 1;
				return;
			}
			if (!open)
				gameplay.ShowInventory();
			if (type != m_Last)
			{
				m_Last = type;
				m_Wait = 0;
				m_Shot = false;
			}
			if (!m_Shot && m_Wait > 5)
			{
				m_Shot = true;
				Print("[VISCHECK] shot older_" + type);
			}
		}
		else if (m_Stage == 1 && m_Wait > 4)
		{
			Print("[VISCHECK] press LMB 6 older_drink");
			m_Wait = 0;
			m_Stage = 2;
		}
		else if (m_Stage == 2 && m_Wait > 2)
		{
			ActionBase running = player.GetActionManager().GetRunningAction();
			Print("[VISCHECK] older running=" + running.ToString());
			m_Wait = 0;
			m_Stage = 3;
		}
		else if (m_Stage == 3 && m_Wait > 35)
		{
			Print("[VISCHECK] client done");
			m_Stage = 4;
		}
	}
}
