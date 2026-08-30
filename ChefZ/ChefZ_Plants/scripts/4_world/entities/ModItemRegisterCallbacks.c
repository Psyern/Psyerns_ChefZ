modded class ModItemRegisterCallbacks
{
    override void RegisterOneHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
    {
		super.RegisterOneHanded(pType, pBehavior);
        
		pType.AddItemInHandsProfileIK("ChefZ_Plant_Carrot",             "dz/anims/workspaces/player/player_main/weapons/player_main_1h_knife.asi",     	pBehavior, "dz/anims/anm/player/ik/gear/screwdriver.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Plant_Parsley",            "dz/anims/workspaces/player/player_main/weapons/player_main_1h_knife.asi",     	pBehavior, "dz/anims/anm/player/ik/gear/screwdriver.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Plant_Rosmary",            "dz/anims/workspaces/player/player_main/weapons/player_main_1h_knife.asi",     	pBehavior, "dz/anims/anm/player/ik/gear/screwdriver.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Plant_Corn_Cob",           "dz/anims/workspaces/player/player_main/props/player_main_1h_sodacan.asi",     	pBehavior, "dz/anims/anm/player/ik/gear/soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Plant_Garlic",           	"dz/anims/workspaces/player/player_main/props/player_main_1h_fruit.asi",     	pBehavior, "dz/anims/anm/player/ik/gear/tomato_fresh.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Plant_RedOnion",           "dz/anims/workspaces/player/player_main/props/player_main_1h_fruit.asi",     	pBehavior, "dz/anims/anm/player/ik/gear/tomato_fresh.anm");
	}

    override void RegisterTwoHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
    {
		super.RegisterTwoHanded(pType, pBehavior);
        
		pType.AddItemInHandsProfileIK("ChefZ_Plant_Cabbage",             "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi",     	pBehavior, "dz/anims/anm/player/ik/two_handed/Cauldron.anm");
	}
}