class CfgPatches
{
	class ChefZ_Items
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
	class ChefZ_Items
	{
		dir="ChefZ\ChefZ_Items";
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
					"ChefZ/ChefZ_Items/scripts/4_World"
				};
			};
		};
	};
};
class CfgVehicles
{
	class ChefZ_Item_Base;
	class ChefZ_Item_Honeycomb_Frame : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Honeycomb_Frame";
		model="\ChefZ\ChefZ_Items\models\honeycomb_frame.p3d";
		descriptionShort="#STR_CHEFZ_Honeycomb_Frame_DESC";
		itemSize[]={3,2};
		inventorySlot[]=
		{
			"ChefZ_Honeycomb_Frame01",
			"ChefZ_Honeycomb_Frame02",
			"ChefZ_Honeycomb_Frame03",
			"ChefZ_Honeycomb_Frame04",
			"ChefZ_Honeycomb_Frame05",
			"ChefZ_Honeycomb_Frame06",
			"ChefZ_Honeycomb_Frame07",
			"ChefZ_Honeycomb_Frame08",
			"ChefZ_Honeycomb_Frame09",
			"ChefZ_Honeycomb_Frame10",
			"ChefZ_Honeycomb_Frame11",
			"ChefZ_Honeycomb_Frame12",
			"ChefZ_Honeycomb_Frame13",
			"ChefZ_Honeycomb_Frame14",
			"ChefZ_Honeycomb_Frame15",
			"ChefZ_Honeycomb_Frame16",
			"ChefZ_Honeycomb_Frame17",
			"ChefZ_Honeycomb_Frame18",
			"ChefZ_Honeycomb_Frame19",
			"ChefZ_Honeycomb_Frame20"
		};
	};
	class ChefZ_Item_Wooden_Frame : ChefZ_Item_Honeycomb_Frame
	{
		scope=2;
		displayName="#STR_CHEFZ_Wooden_Frame";
		model="\ChefZ\ChefZ_Items\models\wooden_frame.p3d";
		descriptionShort="#STR_CHEFZ_Wooden_Frame_DESC";
	};
	class ChefZ_Item_BeeSmoker : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_BeeSmoker";
		model="\ChefZ\ChefZ_Items\models\beesmoker.p3d";
		descriptionShort="#STR_CHEFZ_BeeSmoker_DESC";
		itemSize[]={2,2};
	};
///	class ChefZ_Item_Jar : ChefZ_Item_Base
///	{
///		scope=2;
///		displayName="#STR_CHEFZ_Jar";
///		model="\ChefZ\ChefZ_Items\models\Jar.p3d";
///		descriptionShort="#STR_CHEFZ_Jar_DESC";
///		itemSize[]={1,1};
///	};
/// NEED TO BE FIXED!
	class ChefZ_Item_HandRake : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_HandRake";
		model="\ChefZ\ChefZ_Items\models\handrake.p3d";
		descriptionShort="#STR_CHEFZ_HandRake_DESC";
		itemSize[]={3,1};
	};
	class ChefZ_Item_MilkCan : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_MilkCan";
		model="\ChefZ\ChefZ_Items\models\MilkCan.p3d";
		descriptionShort="#STR_CHEFZ_MilkCan_DESC";
		itemSize[]={3,2};
	};
	class ChefZ_Item_Box : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Box";
		model="\ChefZ\ChefZ_Items\models\Box.p3d";
		descriptionShort="#STR_CHEFZ_Box_DESC";
		itemSize[]={2,2};
	};
	class ChefZ_Item_Box_Open : ChefZ_Item_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Box_Open";
		model="\ChefZ\ChefZ_Items\models\Box_Open.p3d";
		descriptionShort="#STR_CHEFZ_Box_Open_DESC";
		itemSize[]={2,2};
	};
};