class CfgPatches
{
	class ChefZ_Plants_Cultivation
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"ChefZ_Plants"
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
			"world"
		};
		class defs
		{
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"ChefZ/ChefZ_Plants/scripts/4_World"
				};
			};
		};
	};
};
class CfgHorticulture
{
	class Plants
	{
		class ChefZ_Plant_Corn
		{
			infestedTex="dz\gear\cultivation\data\cannabis_plant_insect_co.paa";
			infestedMat="dz\gear\cultivation\data\cannabis_plant_insect.rvmat";
			healthyTex="ChefZ\ChefZ_Plants\cultivation\data\corn_plant_4_co.paa";
			healthyMat="dz\gear\cultivation\data\cannabis_plant.rvmat";
		};
	};
};
class CfgVehicles
{
	class SeedBase;
	class PlantBase;
	class ChefZ_Plant_Corn_Cob : SeedBase
	{
		scope=2;
		displayName="#STR_CHEFZ_Corn_Cob";
		model="\ChefZ\ChefZ_Plants\models\Corn_Cob.p3d";
		descriptionShort="#STR_CHEFZ_Corn_Cob_DESC";
		itemSize[]={1,3};
		varQuantityInit=1;
		varQuantityMin=0;
		varQuantityMax=1;
		varTemperatureFreezeTime=2640;
		varTemperatureThawTime=2640;
		varTemperatureFreezePoint=-2;
		varTemperatureThawPoint=-2;
		varTemperatureMax=105;
		varTemperatureMin=-100;
		rotationFlags=12;
		weight=200;
		stackedUnit="g";
		absorbency=0.2;
		class Horticulture
		{
			PlantType="ChefZ_Plant_Corn";
		};
	};
	class ChefZ_Plant_Corn: PlantBase
	{
		scope=2;
		displayName="$STR_CHEFZ_Corn";
		descriptionShort="$STR_CHEFZ_Corn_DESC";
		model="\ChefZ\ChefZ_Plants\cultivation\models\corn_plant.p3d";
		class Horticulture
		{
			GrowthStagesCount=6;
			CropsCount=2;
			CropsType="ChefZ_Plant_Corn_Cob";
		};
	};
};