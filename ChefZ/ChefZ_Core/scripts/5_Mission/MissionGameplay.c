modded class MissionGameplay
{
	override void OnInit()
	{
		super.OnInit();
		ChefZ_Config_Client.Load();

		ChefZ_LogConfigJson config = ChefZ_Config_Client.GetCachedConfig();
		int enableLogging = 0;
		int enableDebug = 0;
		int keepLogs = 0;
		if (config)
		{
			enableLogging = config.EnableLogging;
			enableDebug = config.EnableDebugLogging;
			keepLogs = config.KeepLogs;
		}

		ChefZ_Logger.Log("ChefZ_Core client started config=" + ChefZ_Config_Client.GetPath() + " EnableLogging=" + enableLogging.ToString() + " EnableDebugLogging=" + enableDebug.ToString() + " KeepLogs=" + keepLogs.ToString(), true);
		ChefZ_Logger.LogDebug("ChefZ_Core client debug recording started");
	}
}
