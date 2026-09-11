class CfgPatches
{
	class ChefZ_Plants_Cultivation
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Scripts",
			"DZ_Data",
			"ChefZ_Core",
			"ChefZ_Plants",
			"DZ_Gear_Cultivation"
		};
	};
};
class CfgMods
{
	class ChefZ_Plants_Cultivation
	{
		dir="ChefZ\ChefZ_Plants\cultivation";
		hideName=1;
		hidePicture=1;
		name="ChefZ";
		credits="";
		author="Lykos";
		version="1.0";
		extra=0;
		type="mod";
		dependencies[]=
		{
			"Game",
			"World",
			"Mission"
		};
		class defs
		{
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"ChefZ/ChefZ_Plants/cultivation/scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"ChefZ/ChefZ_Plants/cultivation/scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"ChefZ/ChefZ_Plants/cultivation/scripts/5_Mission"
				};
			};
		};
	};
};
class CfgHorticulture
{
	class Plants
	{
		class Plant_Corn
		{
			infestedTex="dz\gear\cultivation\data\cannabis_plant_insect_co.paa";
			infestedMat="dz\gear\cultivation\data\cannabis_plant_insect.rvmat";
			healthyTex="ChefZ\ChefZ_Plants\cultivation\Plant_Corn\Data\corn_plant_4_co.paa";
			healthyMat="dz\gear\cultivation\data\cannabis_plant.rvmat";
		};
		class Plant_Chili
		{
			infestedTex="dz\gear\cultivation\data\cannabis_plant_insect_co.paa";
			infestedMat="dz\gear\cultivation\data\cannabis_plant_insect.rvmat";
			healthyTex="ChefZ\ChefZ_Plants\cultivation\Plant_Chili\data\chili_plant_4_co.paa";
			healthyMat="dz\gear\cultivation\data\cannabis_plant.rvmat";
		};
		class Plant_Blackberry
		{
			infestedTex="dz\gear\cultivation\data\cannabis_plant_insect_co.paa";
			infestedMat="dz\gear\cultivation\data\cannabis_plant_insect.rvmat";
			healthyTex="ChefZ\ChefZ_Plants\cultivation\Plant_Blackberry\data\blackberry_plant_4_co.paa";
			healthyMat="dz\gear\cultivation\data\cannabis_plant.rvmat";
		};
		class Plant_Strawberry
		{
			infestedTex="dz\gear\cultivation\data\cannabis_plant_insect_co.paa";
			infestedMat="dz\gear\cultivation\data\cannabis_plant_insect.rvmat";
			healthyTex="ChefZ\ChefZ_Plants\cultivation\Plant_Strawberry\data\strawberry_plant_4_co.paa";
			healthyMat="dz\gear\cultivation\data\cannabis_plant.rvmat";
		};
		class Plant_Blueberry
		{
			infestedTex="dz\gear\cultivation\data\cannabis_plant_insect_co.paa";
			infestedMat="dz\gear\cultivation\data\cannabis_plant_insect.rvmat";
			healthyTex="ChefZ\ChefZ_Plants\cultivation\Plant_Blueberry\data\blueberry_plant_4_co.paa";
			healthyMat="dz\gear\cultivation\data\cannabis_plant.rvmat";
		};
		class Plant_Raspberry
		{
			infestedTex="dz\gear\cultivation\data\cannabis_plant_insect_co.paa";
			infestedMat="dz\gear\cultivation\data\cannabis_plant_insect.rvmat";
			healthyTex="ChefZ\ChefZ_Plants\cultivation\Plant_Raspberry\data\raspberry_plant_4_co.paa";
			healthyMat="dz\gear\cultivation\data\cannabis_plant.rvmat";
		};
	};
};
class CfgVehicles
{
	class ChefZ_Item_Base;
	class SeedBase;
	class PlantBase;
///////////////////////////////////////////
// 				SEEDS					//
/////////////////////////////////////////
	class ChefZ_Plant_Corn_Cob : SeedBase
	{
		scope=2;
		displayName="#STR_CHEFZ_Corn_Cob";
		model="\ChefZ\ChefZ_Plants\models\Corn_Cob.p3d";
		descriptionShort="#STR_CHEFZ_Corn_Cob_DESC";
		itemSize[]={1,2};
		canBeSplit=0;
		varQuantityInit=1;
		varQuantityMin=0;
		varQuantityMax=1;
		class Horticulture
		{
			PlantType="Plant_Corn";
		};
	};
	class ChefZ_Plant_ChiliSeeds : SeedBase
	{
		scope=2;
		displayName="#STR_CHEFZ_ChiliSeeds";
		model="\dz\gear\cultivation\Zucchini_seeds.p3d";
		descriptionShort="#STR_CHEFZ_ChiliSeeds_DESC";
		itemSize[]={1,1};
		canBeSplit=1;
		varQuantityInit=20;
		varQuantityMin=0;
		varQuantityMax=20;
		class Horticulture
		{
			PlantType="Plant_Chili";
		};
	};
	class ChefZ_Plant_BlackberrySeeds : SeedBase
	{
		scope=2;
		displayName="#STR_CHEFZ_BlackberrySeeds";
		model="\dz\gear\cultivation\tomato_seeds.p3d";
		descriptionShort="#STR_CHEFZ_BlackberrySeeds_DESC";
		itemSize[]={1,1};
		canBeSplit=1;
		varQuantityInit=20;
		varQuantityMin=0;
		varQuantityMax=20;
		class Horticulture
		{
			PlantType="Plant_Blackberry";
		};
	};
	class ChefZ_Plant_StrawberrySeeds : SeedBase
	{
		scope=2;
		displayName="#STR_CHEFZ_StrawberrySeeds";
		model="\dz\gear\cultivation\tomato_seeds.p3d";
		descriptionShort="#STR_CHEFZ_StrawberrySeeds_DESC";
		itemSize[]={1,1};
		canBeSplit=1;
		varQuantityInit=20;
		varQuantityMin=0;
		varQuantityMax=20;
		class Horticulture
		{
			PlantType="Plant_Strawberry";
		};
	};
	class ChefZ_Plant_BlueberrySeeds : SeedBase
	{
		scope=2;
		displayName="#STR_CHEFZ_BlueberrySeeds";
		model="\dz\gear\cultivation\Zucchini_seeds.p3d";
		descriptionShort="#STR_CHEFZ_BlueberrySeeds_DESC";
		itemSize[]={1,1};
		canBeSplit=1;
		varQuantityInit=20;
		varQuantityMin=0;
		varQuantityMax=20;
		class Horticulture
		{
			PlantType="Plant_Blueberry";
		};
	};
	class ChefZ_Plant_RaspberrySeeds : SeedBase
	{
		scope=2;
		displayName="#STR_CHEFZ_RaspberrySeeds";
		model="\dz\gear\cultivation\pumpkin_seeds.p3d";
		descriptionShort="#STR_CHEFZ_RaspberrySeeds_DESC";
		itemSize[]={1,1};
		canBeSplit=1;
		varQuantityInit=20;
		varQuantityMin=0;
		varQuantityMax=20;
		class Horticulture
		{
			PlantType="Plant_Raspberry";
		};
	};
///////////////////////////////////////////
// 				PLANTS					//
/////////////////////////////////////////
	class Plant_Corn: PlantBase
	{
		scope=2;
		displayName="$STR_CHEFZ_Corn";
		descriptionShort="$STR_CHEFZ_Corn_DESC";
		model="ChefZ\ChefZ_Plants\cultivation\Plant_Corn\corn_plant.p3d";
		class Horticulture
		{
			GrowthStagesCount=7;
			CropsCount=2;
			CropsType="ChefZ_Plant_Corn_Cob";
		};
	};
	class Plant_Chili: PlantBase
	{
		scope=2;
		displayName="$STR_CHEFZ_Chiliplant";
		descriptionShort="$STR_CHEFZ_Chiliplant_DESC";
		model="ChefZ\ChefZ_Plants\cultivation\Plant_Chili\chili_plant.p3d";
		class Horticulture
		{
			GrowthStagesCount=7;
			CropsCount=2;
			CropsType="ChefZ_Plant_Chili";
		};
	};
	class Plant_Blackberry: PlantBase
	{
		scope=2;
		displayName="$STR_CHEFZ_Blackberryplant";
		descriptionShort="$STR_CHEFZ_Blackberryplant_DESC";
		model="ChefZ\ChefZ_Plants\cultivation\Plant_Blackberry\blackberry_plant.p3d";
		class Horticulture
		{
			GrowthStagesCount=7;
			CropsCount=2;
			CropsType="ChefZ_Plant_Blackberry";
		};
	};
	class Plant_Strawberry: PlantBase
	{
		scope=2;
		displayName="$STR_CHEFZ_Strawberryplant";
		descriptionShort="$STR_CHEFZ_Strawberryplant_DESC";
		model="ChefZ\ChefZ_Plants\cultivation\Plant_Strawberry\strawberry_plant.p3d";
		class Horticulture
		{
			GrowthStagesCount=7;
			CropsCount=2;
			CropsType="ChefZ_Plant_Strawberry";
		};
	};
	class Plant_Blueberry: PlantBase
	{
		scope=2;
		displayName="$STR_CHEFZ_Blueberryplant";
		descriptionShort="$STR_CHEFZ_Blueberryplant_DESC";
		model="ChefZ\ChefZ_Plants\cultivation\Plant_Blueberry\blueberry_plant.p3d";
		class Horticulture
		{
			GrowthStagesCount=7;
			CropsCount=2;
			CropsType="ChefZ_Plant_Blueberry";
		};
	};
	class Plant_Raspberry: PlantBase
	{
		scope=2;
		displayName="$STR_CHEFZ_Raspberryplant";
		descriptionShort="$STR_CHEFZ_Raspberryplant_DESC";
		model="ChefZ\ChefZ_Plants\cultivation\Plant_Raspberry\raspberry_plant.p3d";
		class Horticulture
		{
			GrowthStagesCount=7;
			CropsCount=2;
			CropsType="ChefZ_Plant_Raspberry";
		};
	};
///////////////////////////////////////////
// 				WILD PLANTS				//
/////////////////////////////////////////
	class StaticObject;
	class Plant_Wild_Blackberry : StaticObject
	{
		scope=2;
		displayName="$STR_CHEFZ_Blackberryplant";
		model="ChefZ\ChefZ_Plants\cultivation\Plant_Wild\models\wild_blackberry.p3d";
		isCuttable=1;
		primaryDropsAmount=1;
		secondaryDropsAmount=0;
		toolDamage=4;
		cycleTimeOverride=2;
		primaryOutput="ChefZ_Plant_Blackberry";
		secondaryOutput="";
	};
	class Plant_Wild_Strawberry : StaticObject
	{
		scope=2;
		displayName="$STR_CHEFZ_Strawberryplant";
		model="ChefZ\ChefZ_Plants\cultivation\Plant_Wild\models\wild_Strawberry.p3d";
		isCuttable=1;
		primaryDropsAmount=1;
		secondaryDropsAmount=0;
		toolDamage=4;
		cycleTimeOverride=2;
		primaryOutput="ChefZ_Plant_Strawberry";
		secondaryOutput="";
	};
	class Plant_Wild_Blueberry : StaticObject
	{
		scope=2;
		displayName="$STR_CHEFZ_Blueberryplant";
		model="ChefZ\ChefZ_Plants\cultivation\Plant_Wild\models\wild_Blueberry.p3d";
		isCuttable=1;
		primaryDropsAmount=1;
		secondaryDropsAmount=0;
		toolDamage=4;
		cycleTimeOverride=2;
		primaryOutput="ChefZ_Plant_Blueberry";
		secondaryOutput="";
	};
	class Plant_Wild_Raspberry : StaticObject
	{
		scope=2;
		displayName="$STR_CHEFZ_Raspberryplant";
		model="ChefZ\ChefZ_Plants\cultivation\Plant_Wild\models\wild_Raspberry.p3d";
		isCuttable=1;
		primaryDropsAmount=1;
		secondaryDropsAmount=0;
		toolDamage=4;
		cycleTimeOverride=2;
		primaryOutput="ChefZ_Plant_Raspberry";
		secondaryOutput="";
	};
};