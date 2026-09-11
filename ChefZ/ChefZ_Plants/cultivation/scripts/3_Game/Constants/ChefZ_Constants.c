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

	// Blackberry
	static const string PLANT_TYPE_BLACKBERRY = "Plant_Blackberry";
	static const int DEFAULT_BLACKBERRY_MATURITY_TIME = 120;
	static const int DEFAULT_BLACKBERRY_HARVEST_MIN = 4;
	static const int DEFAULT_BLACKBERRY_HARVEST_MAX = 4;

	// Strawberry
	static const string PLANT_TYPE_STRAWBERRY = "Plant_Strawberry";
	static const int DEFAULT_STRAWBERRY_MATURITY_TIME = 120;
	static const int DEFAULT_STRAWBERRY_HARVEST_MIN = 4;
	static const int DEFAULT_STRAWBERRY_HARVEST_MAX = 4;

	// Blueberry
	static const string PLANT_TYPE_BLUEBERRY = "Plant_Blueberry";
	static const int DEFAULT_BLUEBERRY_MATURITY_TIME = 120;
	static const int DEFAULT_BLUEBERRY_HARVEST_MIN = 4;
	static const int DEFAULT_BLUEBERRY_HARVEST_MAX = 4;

	// Raspberry
	static const string PLANT_TYPE_RASPBERRY = "Plant_Raspberry";
	static const int DEFAULT_RASPBERRY_MATURITY_TIME = 120;
	static const int DEFAULT_RASPBERRY_HARVEST_MIN = 4;
	static const int DEFAULT_RASPBERRY_HARVEST_MAX = 4;

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
