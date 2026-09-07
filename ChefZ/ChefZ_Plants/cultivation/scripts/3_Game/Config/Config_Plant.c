class ChefZ_Config_PlantEntryJson
{
	string ClassName;
	int MaturityTimeSeconds;
	int HarvestAmountMin;
	int HarvestAmountMax;

	void ChefZ_Config_PlantEntryJson()
	{
		ClassName = ChefZ_Constants.PLANT_TYPE_CORN;
		MaturityTimeSeconds = ChefZ_Constants.DEFAULT_CORN_MATURITY_TIME;
		HarvestAmountMin = ChefZ_Constants.DEFAULT_CORN_HARVEST_MIN;
		HarvestAmountMax = ChefZ_Constants.DEFAULT_CORN_HARVEST_MAX;
	}

	void Validate()
	{
		if (ClassName == string.Empty)
			ClassName = ChefZ_Constants.PLANT_TYPE_CORN;

		if (MaturityTimeSeconds < 1)
			MaturityTimeSeconds = ChefZ_Constants.DEFAULT_CORN_MATURITY_TIME;

		if (HarvestAmountMin < 0)
			HarvestAmountMin = 0;

		if (HarvestAmountMax < HarvestAmountMin)
			HarvestAmountMax = HarvestAmountMin;
	}
}

class ChefZ_Config_PlantJson
{
	int ConfigVersion;
	ref array<ref ChefZ_Config_PlantEntryJson> Plants;

	void ChefZ_Config_PlantJson()
	{
		ConfigVersion = ChefZ_Constants.CONFIG_VERSION;
		Plants = new array<ref ChefZ_Config_PlantEntryJson>;
	}

	bool Validate()
	{
		bool addedPlant = false;

		if (ConfigVersion < 1)
			ConfigVersion = ChefZ_Constants.CONFIG_VERSION;

		if (!Plants)
			Plants = new array<ref ChefZ_Config_PlantEntryJson>;

		// Corn
		if (EnsurePlant(ChefZ_Constants.PLANT_TYPE_CORN, ChefZ_Constants.DEFAULT_CORN_MATURITY_TIME, ChefZ_Constants.DEFAULT_CORN_HARVEST_MIN, ChefZ_Constants.DEFAULT_CORN_HARVEST_MAX))
			addedPlant = true;

		// Chili
		if (EnsurePlant(ChefZ_Constants.PLANT_TYPE_CHILI, ChefZ_Constants.DEFAULT_CHILI_MATURITY_TIME, ChefZ_Constants.DEFAULT_CHILI_HARVEST_MIN, ChefZ_Constants.DEFAULT_CHILI_HARVEST_MAX))
			addedPlant = true;

		// PlantName
		// if (EnsurePlant(ChefZ_Constants.PLANT_TYPE_NAME, ChefZ_Constants.DEFAULT_NAME_MATURITY_TIME, ChefZ_Constants.DEFAULT_NAME_HARVEST_MIN, ChefZ_Constants.DEFAULT_NAME_HARVEST_MAX))
		// 	addedPlant = true;

		int i;
		for (i = 0; i < Plants.Count(); i++)
		{
			ChefZ_Config_PlantEntryJson entry = Plants.Get(i);
			if (entry)
				entry.Validate();
		}

		return addedPlant;
	}

	ChefZ_Config_PlantEntryJson GetEntry(string className)
	{
		if (!Plants)
			return null;

		int i;
		for (i = 0; i < Plants.Count(); i++)
		{
			ChefZ_Config_PlantEntryJson entry = Plants.Get(i);
			if (entry && entry.ClassName == className)
				return entry;
		}

		return null;
	}

	protected bool EnsurePlant(string className, int maturityTimeSeconds, int harvestMin, int harvestMax)
	{
		if (GetEntry(className))
			return false;

		ChefZ_Config_PlantEntryJson entry = new ChefZ_Config_PlantEntryJson();
		entry.ClassName = className;
		entry.MaturityTimeSeconds = maturityTimeSeconds;
		entry.HarvestAmountMin = harvestMin;
		entry.HarvestAmountMax = harvestMax;
		Plants.Insert(entry);
		return true;
	}
}

class ChefZ_Config_Plant
{
	protected static ref ChefZ_Config_PlantJson s_ChefZ_CachedConfig;

	static string GetPath()
	{
		return ChefZ_Constants.GetPlantConfigPath();
	}

	static ChefZ_Config_PlantJson GetCachedConfig()
	{
		if (!g_Game || !g_Game.IsServer())
			return null;

		if (!s_ChefZ_CachedConfig)
			Load();

		return s_ChefZ_CachedConfig;
	}

	static ChefZ_Config_PlantEntryJson GetEntry(string className)
	{
		if (!g_Game || !g_Game.IsServer())
			return null;

		ChefZ_Config_PlantJson config = GetCachedConfig();
		if (!config)
			return null;

		return config.GetEntry(className);
	}

	static void EnsureDefaultFile()
	{
		if (!g_Game || !g_Game.IsServer())
			return;

		ChefZ_CoreConstants.EnsureProfileDirectory();
		string configPath = GetPath();
		if (FileExist(configPath))
			return;

		ChefZ_Config_PlantJson config = new ChefZ_Config_PlantJson();
		config.Validate();
		if (!Save(config))
			return;

		s_ChefZ_CachedConfig = config;
	}

	static bool Load()
	{
		if (!g_Game || !g_Game.IsServer())
			return false;

		EnsureDefaultFile();
		ChefZ_CoreConstants.EnsureProfileDirectory();

		string configPath = GetPath();
		if (!FileExist(configPath))
		{
			s_ChefZ_CachedConfig = new ChefZ_Config_PlantJson();
			s_ChefZ_CachedConfig.Validate();
			Save(s_ChefZ_CachedConfig);
			return true;
		}

		ChefZ_Config_PlantJson config = new ChefZ_Config_PlantJson();
		string errorMessage;
		if (!JsonFileLoader<ChefZ_Config_PlantJson>.LoadFile(configPath, config, errorMessage))
		{
			ChefZ_Logger.LogError("Plant_Config.json load failed: " + errorMessage);
			s_ChefZ_CachedConfig = new ChefZ_Config_PlantJson();
			s_ChefZ_CachedConfig.Validate();
			Save(s_ChefZ_CachedConfig);
			return false;
		}

		if (config.Validate())
			Save(config);

		s_ChefZ_CachedConfig = config;
		return true;
	}

	protected static bool Save(ChefZ_Config_PlantJson config)
	{
		if (!config)
			return false;

		string errorMessage;
		if (!JsonFileLoader<ChefZ_Config_PlantJson>.SaveFile(GetPath(), config, errorMessage))
		{
			ChefZ_Logger.LogError("Plant_Config.json save failed: " + errorMessage);
			return false;
		}

		return true;
	}
}
