modded class ModItemRegisterCallbacks
{
    override void RegisterOneHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
    {
		super.RegisterOneHanded(pType, pBehavior);
        
		pType.AddItemInHandsProfileIK("ChefZ_Item_Honeycomb_Frame",    "dz/anims/workspaces/player/player_main/player_main_1h.asi",                   pBehavior, "dz/anims/anm/player/ik/gear/TireRepairKit.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Item_Jar",                "dz/anims/workspaces/player/player_main/player_main_1h.asi",                   pBehavior, "dz/anims/anm/player/ik/gear/marmalade.anm");

		pType.AddItemInHandsProfileIK("ChefZ_Item_BeeSmoker",          "dz/anims/workspaces/player/player_main/weapons/player_main_1h_knife.asi",     pBehavior, "dz/anims/anm/player/ik/gear/screwdriver.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Item_HandRake",           "dz/anims/workspaces/player/player_main/weapons/player_main_1h_pipe.asi",      pBehavior, "dz/anims/anm/player/ik/gear/sickle.anm");
	}

    override void RegisterTwoHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
    {
		super.RegisterTwoHanded(pType, pBehavior);
		pType.AddItemInHandsProfileIK("ChefZ_Item_MilkCan",			   "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi",		  pBehavior, "dz/anims/anm/player/ik/two_handed/CookingPot.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Item_Box",			   	   "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi",		  pBehavior, "dz/anims/anm/player/ik/two_handed/Cauldron.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Item_Box_Open",	   	   "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi",		  pBehavior, "dz/anims/anm/player/ik/two_handed/Cauldron.anm");
	}
}