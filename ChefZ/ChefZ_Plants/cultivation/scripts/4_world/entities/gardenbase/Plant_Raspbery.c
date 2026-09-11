class Plant_Raspberry : PlantBase
{
	void Plant_Raspberry()
	{
		m_FullMaturityTime = ChefZ_Constants.DEFAULT_RASPBERRY_MATURITY_TIME;
	}

	override void Init(GardenBase garden_base, float fertility, float harvesting_efficiency, float water)
	{
		super.Init(garden_base, fertility, harvesting_efficiency, water);
		ChefZ_ApplyConfiguredGrowth();
	}

	override void UpdatePlant()
	{
		super.UpdatePlant();
		ChefZ_ApplyStageVisibility();
		ChefZ_LogGrow("GROW");
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (g_Game && g_Game.IsClient())
		{
			ChefZ_ApplyStageVisibility();
			ChefZ_LogGrow("CLIENT-SYNC");
		}
	}

	override void Harvest(PlayerBase player)
	{
		ChefZ_Logger.Log("[HARVEST] " + GetType() + " crops=" + GetCropsType() + " count=" + ChefZ_GetCropsCount().ToString(), true);
		super.Harvest(player);
	}
}
