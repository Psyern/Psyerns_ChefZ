modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		ChefZ_Config_Plant.Load();

		ChefZ_Config_PlantJson plantConfig = ChefZ_Config_Plant.GetCachedConfig();
		int plantCount = 0;
		if (plantConfig && plantConfig.Plants)
			plantCount = plantConfig.Plants.Count();

		ChefZ_Logger.Log("[START] ChefZ_Plants_Cultivation plant config=" + ChefZ_Config_Plant.GetPath() + " plants=" + plantCount.ToString(), true);
		ChefZ_Logger.LogDebug("[START] ChefZ_Plants_Cultivation server debug recording started");
	}
}
