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
		varQuantityInit=0;
		varQuantityMin=0;
		varQuantityMax=20;
		class Horticulture
		{
			PlantType="Plant_Chili";
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
};