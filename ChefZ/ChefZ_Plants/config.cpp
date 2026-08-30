class CfgPatches
{
	class ChefZ_Plants
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
};