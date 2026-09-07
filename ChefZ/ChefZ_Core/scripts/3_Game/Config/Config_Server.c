class ChefZ_Config_Server
{
	protected static ref ChefZ_LogConfigJson s_ChefZ_CachedConfig;

	static string GetPath()
	{
		return ChefZ_CoreConstants.GetServerConfigPath();
	}

	static ChefZ_LogConfigJson GetCachedConfig()
	{
		if (!s_ChefZ_CachedConfig)
			Load();

		return s_ChefZ_CachedConfig;
	}

	static void EnsureDefaultFile()
	{
		if (!g_Game)
			return;

		ChefZ_CoreConstants.EnsureProfileDirectory();
		string configPath = GetPath();
		if (FileExist(configPath))
			return;

		string legacyPath = ChefZ_CoreConstants.GetProfilePath("Mod_Config.json");
		if (FileExist(legacyPath))
		{
			ChefZ_LogConfigJson legacyConfig = new ChefZ_LogConfigJson();
			string legacyError;
			if (JsonFileLoader<ChefZ_LogConfigJson>.LoadFile(legacyPath, legacyConfig, legacyError))
			{
				legacyConfig.Validate();
				string saveError;
				if (JsonFileLoader<ChefZ_LogConfigJson>.SaveFile(configPath, legacyConfig, saveError))
				{
					s_ChefZ_CachedConfig = legacyConfig;
					return;
				}
			}
		}

		ChefZ_LogConfigJson config = new ChefZ_LogConfigJson();
		string errorMessage;
		if (!JsonFileLoader<ChefZ_LogConfigJson>.SaveFile(configPath, config, errorMessage))
		{
			Print("[ChefZ] ERROR: Server_Config.json save failed: " + errorMessage);
			return;
		}

		s_ChefZ_CachedConfig = config;
	}

	static bool Load()
	{
		if (!g_Game)
			return false;

		EnsureDefaultFile();
		ChefZ_CoreConstants.EnsureProfileDirectory();

		string configPath = GetPath();
		if (!FileExist(configPath))
		{
			s_ChefZ_CachedConfig = new ChefZ_LogConfigJson();
			s_ChefZ_CachedConfig.ApplyToLogger();
			return true;
		}

		ChefZ_LogConfigJson config = new ChefZ_LogConfigJson();
		string errorMessage;
		if (!JsonFileLoader<ChefZ_LogConfigJson>.LoadFile(configPath, config, errorMessage))
		{
			Print("[ChefZ] ERROR: Server_Config.json load failed: " + errorMessage);
			s_ChefZ_CachedConfig = new ChefZ_LogConfigJson();
			s_ChefZ_CachedConfig.ApplyToLogger();
			return false;
		}

		config.Validate();
		s_ChefZ_CachedConfig = config;
		s_ChefZ_CachedConfig.ApplyToLogger();
		return true;
	}
}
