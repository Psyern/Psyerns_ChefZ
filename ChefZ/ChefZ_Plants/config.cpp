class CfgPatches
{
	class ChefZ_Plants
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
	class ChefZ_Plants
	{
		dir="ChefZ\ChefZ_Plants";
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
			"World"
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
class CfgVehicles
{
	class ChefZ_Item_Base;
	class ChefZ_Plant_Carrot : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Carrot";
		model="\ChefZ\ChefZ_Plants\models\Carrot.p3d";
		descriptionShort="#STR_CHEFZ_Carrot_DESC";
		itemSize[]={1,2};
	};
	class ChefZ_Plant_Cabbage : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Cabbage";
		model="\ChefZ\ChefZ_Plants\models\Cabbage.p3d";
		descriptionShort="#STR_CHEFZ_Cabbage_DESC";
		itemSize[]={2,2};
	};
	class ChefZ_Plant_Garlic : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Garlic";
		model="\ChefZ\ChefZ_Plants\models\Garlic.p3d";
		descriptionShort="#STR_CHEFZ_Garlic_DESC";
		itemSize[]={1,3};
	};
	class ChefZ_Plant_Parsley : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Parsley";
		model="\ChefZ\ChefZ_Plants\models\Parsley.p3d";
		descriptionShort="#STR_CHEFZ_Parsley_DESC";
		itemSize[]={1,3};
	};
	class ChefZ_Plant_RedOnion : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_RedOnion";
		model="\ChefZ\ChefZ_Plants\models\RedOnion.p3d";
		descriptionShort="#STR_CHEFZ_RedOnion_DESC";
		itemSize[]={1,3};
	};
	class ChefZ_Plant_Rosmary : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Rosmary";
		model="\ChefZ\ChefZ_Plants\models\Rosmary.p3d";
		descriptionShort="#STR_CHEFZ_Rosmary_DESC";
		itemSize[]={1,3};
	};
	class ChefZ_Plant_Chili : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Chili";
		model="\ChefZ\ChefZ_Plants\models\Chili.p3d";
		descriptionShort="#STR_CHEFZ_Chili_DESC";
		itemSize[]={1,2};
	};
	class ChefZ_Plant_Blackberry : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Blackberry";
		model="\ChefZ\ChefZ_Plants\models\Blackberry.p3d";
		descriptionShort="#STR_CHEFZ_Blackberry_DESC";
		itemSize[]={1,1};
		containsSeedsType="ChefZ_Plant_BlackberrySeeds";
		containsSeedsQuantity="3";
	};
	class ChefZ_Plant_Strawberry : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Strawberry";
		model="\ChefZ\ChefZ_Plants\models\Strawberry.p3d";
		descriptionShort="#STR_CHEFZ_Strawberry_DESC";
		itemSize[]={1,1};
		containsSeedsType="ChefZ_Plant_StrawberrySeeds";
		containsSeedsQuantity="3";
	};
	class ChefZ_Plant_Blueberry : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Blueberry";
		model="\ChefZ\ChefZ_Plants\models\Blueberry.p3d";
		descriptionShort="#STR_CHEFZ_Blueberry_DESC";
		itemSize[]={1,1};
		containsSeedsType="ChefZ_Plant_BlueberrySeeds";
		containsSeedsQuantity="3";
	};
	class ChefZ_Plant_Raspberry : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Raspberry";
		model="\ChefZ\ChefZ_Plants\models\Raspberry.p3d";
		descriptionShort="#STR_CHEFZ_Raspberry_DESC";
		itemSize[]={1,1};
		containsSeedsType="ChefZ_Plant_RaspberrySeeds";
		containsSeedsQuantity="3";
	};
};