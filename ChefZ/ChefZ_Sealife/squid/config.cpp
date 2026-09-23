class CfgPatches
{
	class ChefZ_Sealife_Squid
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
	class ChefZ_Sealife_Squid
	{
		dir="ChefZ\ChefZ_Sealife\squid";
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
{
	class ChefZ_Food_Base;
	class ChefZ_Sealife_CrimsonSquid : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_CrimsonSquid";
		model="\ChefZ\ChefZ_Sealife\squid\models\CrimsonSquid.p3d";
		descriptionShort="#STR_CHEFZ_CrimsonSquid_DESC";
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
	class ChefZ_Sealife_NorthernCrimsonSquid : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_NorthernCrimsonSquid";
		model="\ChefZ\ChefZ_Sealife\squid\models\NorthernCrimsonSquid.p3d";
		descriptionShort="#STR_CHEFZ_NorthernCrimsonSquid_DESC";
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
	class ChefZ_Sealife_Octopus : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Octopus";
		model="\ChefZ\ChefZ_Sealife\squid\models\Octopus.p3d";
		descriptionShort="#STR_CHEFZ_Octopus_DESC";
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
	class ChefZ_Sealife_SpeckledSquid : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_SpeckledSquid";
		model="\ChefZ\ChefZ_Sealife\squid\models\SpeckledSquid.p3d";
		descriptionShort="#STR_CHEFZ_SpeckledSquid_DESC";
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
	class ChefZ_Sealife_Squid : ChefZ_Food_Base
	{
		scope=2;
		displayName="#STR_CHEFZ_Squid";
		model="\ChefZ\ChefZ_Sealife\squid\models\Squid.p3d";
		descriptionShort="#STR_CHEFZ_Squid_DESC";
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
};