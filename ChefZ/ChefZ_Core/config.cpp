class CfgPatches
{
	class ChefZ_Core
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Scripts",
			"DZ_Data"
		};
	};
};
class CfgMods
{
	class ChefZ_Core
	{
		dir="ChefZ\ChefZ_Core";
		hideName=0;
		hidePicture=0;
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
					"ChefZ/ChefZ_Core/scripts/4_World"
				};
			};
		};
	};
};
class CfgVehicles
{
	class Inventory_Base;
	class ChefZ_Item_Base : Inventory_Base
	{
		scope=0;
		displayName="DO NOT TOUCH";
		weight=1000;
		overrideDrawArea="4.0";
		forceFarBubble="true";
		carveNavmesh=1;
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=1000;
					healthLevels[]=
					{
						{
							1,
							{
								"dz\gear\crafting\data\bp_wooden_stick.rvmat"
							}
						},
						{
							0.69999999,
							{
								"DZ\gear\camping\Data\wooden_log_damage.rvmat"
							}
						},
						{
							0.5,
							{
								"DZ\gear\camping\Data\wooden_log_damage.rvmat"
							}
						},
						{
							0.30000001,
							{
								"DZ\gear\camping\Data\wooden_log_destruct.rvmat"
							}
						},
						{
							0,
							{
								"DZ\gear\camping\Data\wooden_log_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
	class ChefZ_Deployed_Base : ChefZ_Item_Base
	{
		scope=0;
		physLayer="item_large";
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=5000;
				};
			};
		};
	};
    class ChefZ_Base_Kit : Inventory_Base
    {
        scope=0;
		displayName="#STR_CHEFZ_KIT";
		model="\DZ\gear\camping\wooden_case.p3d";
		itemSize[] = {3,2};
		carveNavmesh = 1;
		canBeDigged = 0;
		rotationFlags = 2;
		weight = 2500;
		itemBehaviour = 0;
		physLayer="item_large";
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=500;
				};
			};
		};
    };
};