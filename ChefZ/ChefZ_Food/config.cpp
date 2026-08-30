class CfgPatches
{
	class ChefZ_Food
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"ChefZ_Core",
			"ChefZ_Core_Slots"
		};
	};
};
class CfgMods
{
	class ChefZ_Food
	{
		dir="ChefZ\ChefZ_Food";
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
					"ChefZ/ChefZ_Food/scripts/4_World"
				};
			};
		};
	};
};
class CfgVehicles
{
	class ChefZ_Food_Base;
	class ChefZ_Food_Soup : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Soup";
		model="\ChefZ\ChefZ_Food\models\Soup.p3d";
		descriptionShort="#STR_CHEFZ_Soup_DESC";
		itemSize[]={2,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Soup_BoneBroth : ChefZ_Food_Soup
	{
		scope=2;
		displayName="#STR_CHEFZ_Soup_BoneBroth";
		model="\ChefZ\ChefZ_Food\models\Soup_BoneBroth.p3d";
		descriptionShort="#STR_CHEFZ_Soup_BoneBroth_DESC";
		itemSize[]={2,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Soup_Vegetables : ChefZ_Food_Soup
	{
		scope=2;
		displayName="#STR_CHEFZ_Soup_Vegetables";
		model="\ChefZ\ChefZ_Food\models\Soup_Vegetables.p3d";
		descriptionShort="#STR_CHEFZ_Soup_Vegetables_DESC";
		itemSize[]={2,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Soup_ChernarusChili : ChefZ_Food_Soup
	{
		scope=2;
		displayName="#STR_CHEFZ_Soup_ChernarusChili";
		model="\ChefZ\ChefZ_Food\models\Soup_ChernarusChili.p3d";
		descriptionShort="#STR_CHEFZ_Soup_ChernarusChili_DESC";
		itemSize[]={2,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Stew_Fisherman : ChefZ_Food_Soup
	{
		scope=2;
		displayName="#STR_CHEFZ_Stew_Fisherman";
		model="\ChefZ\ChefZ_Food\models\Stew_Fisherman.p3d";
		descriptionShort="#STR_CHEFZ_Stew_Fisherman_DESC";
		itemSize[]={2,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Stew_Hunter : ChefZ_Food_Soup
	{
		scope=2;
		displayName="#STR_CHEFZ_Stew_Hunter";
		model="\ChefZ\ChefZ_Food\models\Stew_Hunter.p3d";
		descriptionShort="#STR_CHEFZ_Stew_Hunter_DESC";
		itemSize[]={2,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Bread : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Bread";
		model="\ChefZ\ChefZ_Food\models\Bread.p3d";
		descriptionShort="#STR_CHEFZ_Bread_DESC";
		itemSize[]={2,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Cheese : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Cheese";
		model="\ChefZ\ChefZ_Food\models\Cheese.p3d";
		descriptionShort="#STR_CHEFZ_Cheese_DESC";
		itemSize[]={2,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_BeefCubes : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_BeefCubes";
		model="\ChefZ\ChefZ_Food\models\beefcubes.p3d";
		descriptionShort="#STR_CHEFZ_BeefCubes_DESC";
		itemSize[]={1,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_DicedMeat : ChefZ_Food_BeefCubes
	{
		scope=2;
		displayName="#STR_CHEFZ_DicedMeat";
		model="\ChefZ\ChefZ_Food\models\DicedMeat.p3d";
		descriptionShort="#STR_CHEFZ_DicedMeat_DESC";
		itemSize[]={1,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_FarmersBreakfast : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_FarmersBreakfast";
		model="\ChefZ\ChefZ_Food\models\FarmersBreakfast.p3d";
		descriptionShort="#STR_CHEFZ_FarmersBreakfast_DESC";
		itemSize[]={3,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Leg_Beef : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Leg_Beef";
		model="\ChefZ\ChefZ_Food\models\Leg_Beef.p3d";
		descriptionShort="#STR_CHEFZ_Leg_Beef_DESC";
		itemSize[]={2,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Leg_Pork : ChefZ_Food_Leg_Beef
	{
		scope=2;
		displayName="#STR_CHEFZ_Leg_Pork";
		model="\ChefZ\ChefZ_Food\models\Leg_Pork.p3d";
		descriptionShort="#STR_CHEFZ_Leg_Pork_DESC";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Leg_Venison : ChefZ_Food_Leg_Beef
	{
		scope=2;
		displayName="#STR_CHEFZ_Leg_Venison";
		model="\ChefZ\ChefZ_Food\models\Leg_Venison.p3d";
		descriptionShort="#STR_CHEFZ_Leg_Venison_DESC";
		itemSize[]={3,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_MeatDumplings : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_MeatDumplings";
		model="\ChefZ\ChefZ_Food\models\MeatDumplings.p3d";
		descriptionShort="#STR_CHEFZ_MeatDumplings_DESC";
		itemSize[]={3,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Breadplate : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Breadplate";
		model="\ChefZ\ChefZ_Food\models\Sausage_Breadplate.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Breadplate_DESC";
		itemSize[]={3,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Cooked : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Cooked";
		model="\ChefZ\ChefZ_Food\models\Sausage_Cooked.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Cooked_DESC";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Dry : ChefZ_Food_Sausage_Cooked
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Dry";
		model="\ChefZ\ChefZ_Food\models\Sausage_Dry.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Dry_DESC";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Raw : ChefZ_Food_Sausage_Cooked
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Raw";
		model="\ChefZ\ChefZ_Food\models\Sausage_Raw.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Raw_DESC";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Raw_2 : ChefZ_Food_Sausage_Raw
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Raw_2";
		model="\ChefZ\ChefZ_Food\models\Sausage_Raw_2.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Raw_2_DESC";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Raw_Boar : ChefZ_Food_Sausage_Raw
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Raw_Boar";
		model="\ChefZ\ChefZ_Food\models\Sausage_Raw_Boar.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Raw_Boar_DESC";
		itemSize[]={3,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Raw_Hunter : ChefZ_Food_Sausage_Raw
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Raw_Hunter";
		model="\ChefZ\ChefZ_Food\models\Sausage_Raw_Hunter.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Raw_Hunter_DESC";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Raw_Pork : ChefZ_Food_Sausage_Raw
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Raw_Pork";
		model="\ChefZ\ChefZ_Food\models\Sausage_Raw_Pork.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Raw_Pork_DESC";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Raw_Spicy : ChefZ_Food_Sausage_Raw
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Raw_Spicy";
		model="\ChefZ\ChefZ_Food\models\Sausage_Raw_Spicy.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Raw_Spicy_DESC";
		itemSize[]={2,1};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Raw_Venison : ChefZ_Food_Sausage_Raw
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Raw_Venison";
		model="\ChefZ\ChefZ_Food\models\Sausage_Raw_Venison.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Raw_Venison_DESC";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ChefZ_Food_Sausage_Smoked : ChefZ_Food_Sausage_Cooked
	{
		scope=2;
		displayName="#STR_CHEFZ_Sausage_Smoked";
		model="\ChefZ\ChefZ_Food\models\Sausage_Smoked.p3d";
		descriptionShort="#STR_CHEFZ_Sausage_Smoked_DESC";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
};