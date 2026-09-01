class CfgPatches
{
	class ChefZ_Devices
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
	class ChefZ_Devices
	{
		dir="ChefZ\ChefZ_Devices";
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
					"ChefZ/ChefZ_Devices/scripts/4_World"
				};
			};
		};
	};
};
class CfgVehicles
{
	class ChefZ_Base_Kit;
	class ChefZ_Deployed_Base;
	class ChefZ_Device_BeeHive_Kit : ChefZ_Base_Kit
	{
		scope=2;
		displayName="#STR_CHEFZ_BEEHIVE - #STR_CHEFZ_KIT";
	};
	class ChefZ_Device_BeeHive : ChefZ_Deployed_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_BEEHIVE";
		model="\ChefZ\ChefZ_Devices\models\beehive.p3d";
		descriptionShort="#STR_CHEFZ_BEEHIVE_DESC";
		weight=25000;
		itemSize[]={10,10};
		attachments[]=
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
	class ChefZ_Device_BeeKeeper_Kit : ChefZ_Base_Kit
	{
		scope=2;
		displayName="#STR_CHEFZ_BEEKEEPER - #STR_CHEFZ_KIT";
	};
	class ChefZ_Device_BeeKeeper : ChefZ_Deployed_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_BEEKEEPER";
		model="\ChefZ\ChefZ_Devices\models\beekeeper.p3d";
		descriptionShort="#STR_CHEFZ_BEEKEEPER_DESC";
		weight=25000;
		itemSize[]={10,10};
		attachments[]=
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
			"ChefZ_Honeycomb_Frame10"
		};
	};
	class ChefZ_Device_ButterChurn_Kit : ChefZ_Base_Kit
	{
		scope=2;
		displayName="#STR_CHEFZ_ButterChurn - #STR_CHEFZ_KIT";
	};
	class ChefZ_Device_ButterChurn : ChefZ_Deployed_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_ButterChurn";
		model="\ChefZ\ChefZ_Devices\models\ButterChurn.p3d";
		descriptionShort="#STR_CHEFZ_ButterChurn_DESC";
		weight=25000;
		itemSize[]={10,10};
	};
	class ChefZ_Device_CheesePress_Kit : ChefZ_Base_Kit
	{
		scope=2;
		displayName="#STR_CHEFZ_CheesePress - #STR_CHEFZ_KIT";
	};
	class ChefZ_Device_CheesePress : ChefZ_Deployed_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_CheesePress";
		model="\ChefZ\ChefZ_Devices\models\CheesePress.p3d";
		descriptionShort="#STR_CHEFZ_CheesePress_DESC";
		weight=25000;
		itemSize[]={10,10};
	};
	class ChefZ_Device_GrainMill_Kit : ChefZ_Base_Kit
	{
		scope=2;
		displayName="#STR_CHEFZ_GrainMill - #STR_CHEFZ_KIT";
	};
	class ChefZ_Device_GrainMill : ChefZ_Deployed_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_GrainMill";
		model="\ChefZ\ChefZ_Devices\models\GrainMill.p3d";
		descriptionShort="#STR_CHEFZ_GrainMill_DESC";
		weight=25000;
		itemSize[]={10,10};
	};
	class ChefZ_Device_MeatGrinder_Kit : ChefZ_Base_Kit
	{
		scope=2;
		displayName="#STR_CHEFZ_MeatGrinder - #STR_CHEFZ_KIT";
	};
	class ChefZ_Device_MeatGrinder : ChefZ_Deployed_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_MeatGrinder";
		model="\ChefZ\ChefZ_Devices\models\MeatGrinder.p3d";
		descriptionShort="#STR_CHEFZ_MeatGrinder_DESC";
		weight=25000;
		itemSize[]={10,10};
	};
	class ChefZ_Device_Mortar_Kit : ChefZ_Base_Kit
	{
		scope=2;
		displayName="#STR_CHEFZ_Mortar - #STR_CHEFZ_KIT";
	};
	class ChefZ_Device_Mortar : ChefZ_Deployed_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Mortar";
		model="\ChefZ\ChefZ_Devices\models\Mortar.p3d";
		descriptionShort="#STR_CHEFZ_Mortar_DESC";
		weight=25000;
		itemSize[]={10,10};
	};
	class ChefZ_Device_Smoker_Kit : ChefZ_Base_Kit
	{
		scope=2;
		displayName="#STR_CHEFZ_Smoker - #STR_CHEFZ_KIT";
	};
	class ChefZ_Device_Smoker : ChefZ_Deployed_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Smoker";
		model="\ChefZ\ChefZ_Devices\models\Smoker.p3d";
		descriptionShort="#STR_CHEFZ_Smoker_DESC";
		weight=25000;
		itemSize[]={10,10};
	};
	class ChefZ_Device_DryRack_Kit : ChefZ_Base_Kit
	{
		scope=2;
		displayName="#STR_CHEFZ_DryRack - #STR_CHEFZ_KIT";
	};
	class ChefZ_Device_DryRack : ChefZ_Deployed_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_DryRack";
		model="\ChefZ\ChefZ_Devices\models\DryRack.p3d";
		descriptionShort="#STR_CHEFZ_DryRack_DESC";
		weight=25000;
		itemSize[]={10,10};
		attachments[]=
		{
			"ChefZ_DryRack_Hook_1",
			"ChefZ_DryRack_Hook_2",
			"ChefZ_DryRack_Hook_3",
			"ChefZ_DryRack_Hook_4",
			"ChefZ_DryRack_Hook_5"
		};
	};
};
class CfgNonAIVehicles
{
	class ProxyAttachment;
	class Proxyhook_1: ProxyAttachment
	{
		model="ChefZ\ChefZ_Devices\models\proxies\hook_1.p3d";
		inventorySlot[]=
		{
			"ChefZ_DryRack_Hook_1"
		};
	};
	class Proxyhook_2: ProxyAttachment
	{
		model="ChefZ\ChefZ_Devices\models\proxies\hook_2.p3d";
		inventorySlot[]=
		{
			"ChefZ_DryRack_Hook_2"
		};
	};
	class Proxyhook_3: ProxyAttachment
	{
		model="ChefZ\ChefZ_Devices\models\proxies\hook_3.p3d";
		inventorySlot[]=
		{
			"ChefZ_DryRack_Hook_3"
		};
	};
	class Proxyhook_4: ProxyAttachment
	{
		model="ChefZ\ChefZ_Devices\models\proxies\hook_4.p3d";
		inventorySlot[]=
		{
			"ChefZ_DryRack_Hook_4"
		};
	};
	class Proxyhook_5: ProxyAttachment
	{
		model="ChefZ\ChefZ_Devices\models\proxies\hook_5.p3d";
		inventorySlot[]=
		{
			"ChefZ_DryRack_Hook_5"
		};
	};
};