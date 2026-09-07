class ChefZ_LogConfigJson
{
	int configVersion;
	int EnableLogging;
	int EnableDebugLogging;
	int KeepLogs;

	void ChefZ_LogConfigJson()
	{
		configVersion = ChefZ_CoreConstants.CONFIG_VERSION;
		EnableLogging = ChefZ_CoreConstants.DEFAULT_ENABLE_LOGGING;
		EnableDebugLogging = ChefZ_CoreConstants.DEFAULT_ENABLE_DEBUG_LOGGING;
		KeepLogs = ChefZ_CoreConstants.DEFAULT_KEEP_LOGS;
	}

	void Validate()
	{
		if (configVersion < 1)
			configVersion = ChefZ_CoreConstants.CONFIG_VERSION;

		if (EnableLogging != 0)
			EnableLogging = 1;

		if (EnableDebugLogging != 1)
			EnableDebugLogging = 0;

		if (KeepLogs < 1)
			KeepLogs = ChefZ_CoreConstants.DEFAULT_KEEP_LOGS;
	}

	void ApplyToLogger()
	{
		Validate();
		ChefZ_Logger.Configure(EnableLogging, EnableDebugLogging, KeepLogs);
	}
}

class ChefZ_Config_Client
{
	protected static ref ChefZ_LogConfigJson s_ChefZ_CachedConfig;

	static string GetPath()
	{
		return ChefZ_CoreConstants.GetClientConfigPath();
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

		ChefZ_LogConfigJson config = new ChefZ_LogConfigJson();
		string errorMessage;
		if (!JsonFileLoader<ChefZ_LogConfigJson>.SaveFile(configPath, config, errorMessage))
		{
			Print("[ChefZ] ERROR: Client_Config.json save failed: " + errorMessage);
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
			Print("[ChefZ] ERROR: Client_Config.json load failed: " + errorMessage);
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
