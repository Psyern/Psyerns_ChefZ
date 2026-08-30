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
	class Edible_Base;
	class ChefZ_Food_Base: Edible_Base
	{
        scope=0;
		debug_ItemCategory=6;
		rotationFlags=1;
		weight=250;
		varTemperatureFreezePoint=-60;
		varTemperatureThawPoint=-60;
		varTemperatureFreezeTime=7920;
		varTemperatureThawTime=7920;
		varQuantityInit=200;
		varQuantityMin=0;
		varQuantityMax=200;
		varTemperatureMax=100;
		temperaturePerQuantityWeight=2;
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=200;
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DZ\gear\food\data\tycinky.rvmat",
								"DZ\gear\food\data\tycinky_wrapping.rvmat"
							}
						},
						
						{
							0.69999999,
							
							{
								"DZ\gear\food\data\tycinky.rvmat",
								"DZ\gear\food\data\tycinky_wrapping.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\gear\food\data\tycinky_damage.rvmat",
								"DZ\gear\food\data\tycinky_wrapping_damage.rvmat"
							}
						},
						
						{
							0.30000001,
							
							{
								"DZ\gear\food\data\tycinky_damage.rvmat",
								"DZ\gear\food\data\tycinky_wrapping_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\gear\food\data\tycinky_destruct.rvmat",
								"DZ\gear\food\data\tycinky_wrapping_destruct.rvmat"
							}
						}
					};
				};
			};
		};
		soundImpactType="plastic";
		class AnimEvents
		{
			class SoundWeapon
			{
				class openTunaCan
				{
					soundSet="openTunaCan_SoundSet";
					id=204;
				};
				class pickUpItem
				{
					soundSet="pickUpBloodBag_SoundSet";
					id=797;
				};
				class Eating_TakeFood
				{
					soundSet="Eating_TakeFood_Soundset";
					id=889;
				};
				class Eating_BoxOpen
				{
					soundSet="Eating_BoxOpen_Soundset";
					id=893;
				};
				class Eating_BoxShake
				{
					soundSet="Eating_BoxShake_Soundset";
					id=894;
				};
				class Eating_BoxEnd
				{
					soundSet="Eating_BoxEnd_Soundset";
					id=895;
				};
				class drop
				{
					soundset="bloodbag_drop_SoundSet";
					id=898;
				};
			};
		};
		class InventorySlotsOffsets
		{
			class DirectCookingA
			{
				position[]={0.059999999,0.015,0};
				orientation[]={0,90,0};
			};
			class DirectCookingB
			{
				position[]={0.059999999,0.015,0};
				orientation[]={45,90,0};
			};
			class DirectCookingC
			{
				position[]={0.059999999,0.015,0};
				orientation[]={0,90,0};
			};
		};
	};
};