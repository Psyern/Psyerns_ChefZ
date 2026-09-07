class ChefZ_Constants
{
	static const string PLANT_CONFIG_FILE = "Plant_Config.json";
	static const int CONFIG_VERSION = 1;
	static const int PLANT_STAGE_COUNT = 5;

	// Corn
	static const string PLANT_TYPE_CORN = "Plant_Corn";
	static const int DEFAULT_CORN_MATURITY_TIME = 120;
	static const int DEFAULT_CORN_HARVEST_MIN = 4;
	static const int DEFAULT_CORN_HARVEST_MAX = 4;

	// Chili
	static const string PLANT_TYPE_CHILI = "Plant_Chili";
	static const int DEFAULT_CHILI_MATURITY_TIME = 120;
	static const int DEFAULT_CHILI_HARVEST_MIN = 4;
	static const int DEFAULT_CHILI_HARVEST_MAX = 4;

	// PlantName
	// static const string PLANT_TYPE_NAME = "Plant_Name";
	// static const int DEFAULT_NAME_MATURITY_TIME = 120;
	// static const int DEFAULT_NAME_HARVEST_MIN = 4;
	// static const int DEFAULT_NAME_HARVEST_MAX = 4;

	static string GetPlantConfigPath()
	{
		return ChefZ_CoreConstants.GetProfilePath(PLANT_CONFIG_FILE);
	}
}
