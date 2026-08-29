modded class ModItemRegisterCallbacks
{
    override void RegisterOneHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
    {
		super.RegisterOneHanded(pType, pBehavior);
        
		pType.AddItemInHandsProfileIK("ChefZ_Item_Honeycomb_Frame",    "dz/anims/workspaces/player/player_main/player_main_1h.asi",                   pBehavior, "dz/anims/anm/player/ik/gear/TireRepairKit.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Item_BeefCubes",          "dz/anims/workspaces/player/player_main/player_main_1h.asi",                   pBehavior, "dz/anims/anm/player/ik/gear/guts_animal.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Item_Jar",                "dz/anims/workspaces/player/player_main/player_main_1h.asi",                   pBehavior, "dz/anims/anm/player/ik/gear/marmalade.anm");

		pType.AddItemInHandsProfileIK("ChefZ_Item_BeeSmoker",          "dz/anims/workspaces/player/player_main/weapons/player_main_1h_knife.asi",     pBehavior, "dz/anims/anm/player/ik/gear/screwdriver.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Item_Carrot",             "dz/anims/workspaces/player/player_main/weapons/player_main_1h_knife.asi",     pBehavior, "dz/anims/anm/player/ik/gear/screwdriver.anm");
	}
}