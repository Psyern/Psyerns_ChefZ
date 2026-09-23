class CfgPatches
{
	class ChefZ_Sealife_Fish
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Scripts",
			"DZ_Data",
			"ChefZ_Sealife"
			"ChefZ_Core",
			"ChefZ_Core_Slots"
		};
	};
};
class CfgMods
{
	class ChefZ_Sealife_Fish
	{
		dir="ChefZ\ChefZ_Sealife\fish";
		hideName=1;
		hidePicture=1;
		name="ChefZ";
		credits="";
		author="Lykos";
		version="1.0";
		extra=0;
		type="mod";
		dependencies[]={};
	};
};
class CfgVehicles
{};