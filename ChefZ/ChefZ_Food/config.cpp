class CfgPatches
{
	class ChefZ_Food
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Scripts",
			"DZ_Data",
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
	class ChefZ_Food_Leg_Beef : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Leg_Beef";
		model="\ChefZ\ChefZ_Food\models\Leg_Beef.p3d";
		descriptionShort="#STR_CHEFZ_Leg_Beef_DESC";
		itemSize[]={1,3};
		inventorySlot[]=
		{
			"ChefZ_DryRack_Hook_1",
			"ChefZ_DryRack_Hook_2",
			"ChefZ_DryRack_Hook_3",
			"ChefZ_DryRack_Hook_4",
			"ChefZ_DryRack_Hook_5"
		};
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
		itemSize[]={2,3};
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
	class ChefZ_Food_Panfood_Base : ChefZ_Food_Base
	{
		scope=0;
		displayName="#STR_CHEFZ_Meal";
		model="\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
		descriptionShort="#STR_CHEFZ_Meal_DESC";
		itemSize[]={3,2};
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood_base_co.paa"
		};
		class Nutrition
		{
			fullnessIndex=3;
			energy=100;
			water=0;
			nutritionalIndex=1;
			toxicity=0;
		};
		inventorySlot[]=
		{
			"ChefZ_Meal"
		};
	};
	class ChefZ_Food_PF_BeanSausagePlate : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\BeanSausagePlate_co.paa"
		};
	};
	class ChefZ_Food_PF_BloodSausagePlate : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\BloodSausagePlate_co.paa"
		};
	};
	class ChefZ_Food_PF_CheeseFlatbread : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\CheeseFlatbread_co.paa"
		};
	};
	class ChefZ_Food_PF_CreamMushroomPasta : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\CreamMushroomPasta_co.paa"
		};
	};
	class ChefZ_Food_PF_FishPotatoPlate : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\FishPotatoPlate_co.paa"
		};
	};
	class ChefZ_Food_PF_HoneyBreadPlate : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\HoneyBreadPlate_co.paa"
		};
	};
	class ChefZ_Food_PF_HunterPasta : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\HunterPasta_co.paa"
		};
	};
	class ChefZ_Food_PF_HunterPlate : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\HunterPlate_co.paa"
		};
	};
	class ChefZ_Food_PF_MacAndCheese : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\MacAndCheese_co.paa"
		};
	};
	class ChefZ_Food_PF_MilkRice : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\MilkRice_co.paa"
		};
	};
	class ChefZ_Food_PF_MushroomPan : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\MushroomPan_co.paa"
		};
	};
	class ChefZ_Food_PF_PotatoPancakes : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\PotatoPancakes_co.paa"
		};
	};
	class ChefZ_Food_PF_SausageBreadPlate : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\SausageBreadPlate_co.paa"
		};
	};
	class ChefZ_Food_PF_SausagePasta : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\SausagePasta_co.paa"
		};
	};
	class ChefZ_Food_PF_SausagePotatoes : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\SausagePotatoes_co.paa"
		};
	};
	class ChefZ_Food_PF_ScrambledEggSausage : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\ScrambledEggSausage_co.paa"
		};
	};
	class ChefZ_Food_PF_SmallFishPan : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\SmallFishPan_co.paa"
		};
	};
	class ChefZ_Food_PF_SurvivorSpaghetti : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\SurvivorSpaghetti_co.paa"
		};
	};
	class ChefZ_Food_PF_TacticalBreakfast : ChefZ_Food_Panfood_Base
	{
		scope=2;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"ChefZ\ChefZ_Food\data\panfood\TacticalBreakfast_co.paa"
		};
	};
	class ChefZ_Food_Butter : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Butter";
		model="\ChefZ\ChefZ_Food\models\Butter.p3d";
		descriptionShort="#STR_CHEFZ_Butter_DESC";
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
	class ChefZ_Food_Dough : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Dough";
		model="\ChefZ\ChefZ_Food\models\Dough.p3d";
		descriptionShort="#STR_CHEFZ_Dough_DESC";
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
	class ChefZ_Food_Dried_Pasta : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Dried_Pasta";
		model="\ChefZ\ChefZ_Food\models\Dried_Pasta.p3d";
		descriptionShort="#STR_CHEFZ_Dried_Pasta_DESC";
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
	class ChefZ_Food_Egg : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Egg";
		model="\ChefZ\ChefZ_Food\models\Egg.p3d";
		descriptionShort="#STR_CHEFZ_Egg_DESC";
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
	class ChefZ_Food_Flatbread : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Flatbread";
		model="\ChefZ\ChefZ_Food\models\Flatbread.p3d";
		descriptionShort="#STR_CHEFZ_Flatbread_DESC";
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
	class ChefZ_Food_Rabbit_Raw : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Rabbit_Raw";
		model="\ChefZ\ChefZ_Food\models\Rabbit_Raw.p3d";
		descriptionShort="#STR_CHEFZ_Rabbit_Raw_DESC";
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