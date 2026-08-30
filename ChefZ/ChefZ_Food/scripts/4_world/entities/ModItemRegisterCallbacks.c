modded class ModItemRegisterCallbacks
{
    override void RegisterOneHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
    {
		super.RegisterOneHanded(pType, pBehavior);
        
 		pType.AddItemInHandsProfileIK("ChefZ_Food_Cheese",    		   "dz/anims/workspaces/player/player_main/player_main_1h.asi",                   pBehavior, "dz/anims/anm/player/ik/gear/rice.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Food_BeefCubes",          "dz/anims/workspaces/player/player_main/player_main_1h.asi",                   pBehavior, "dz/anims/anm/player/ik/gear/guts_animal.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Food_Leg_Beef",           "dz/anims/workspaces/player/player_main/props/player_main_1h_sodacan.asi",     pBehavior, "dz/anims/anm/player/ik/gear/soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Food_Sausage_Cooked",           "dz/anims/workspaces/player/player_main/props/player_main_1h_sodacan.asi",     pBehavior, "dz/anims/anm/player/ik/gear/soda_can.anm");
	}

    override void RegisterTwoHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
    {
		super.RegisterTwoHanded(pType, pBehavior);

		pType.AddItemInHandsProfileIK("ChefZ_Food_Soup",			   "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi",		  pBehavior, "dz/anims/anm/player/ik/two_handed/Cauldron.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Food_Bread",			   "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi",		  pBehavior, "dz/anims/anm/player/ik/two_handed/Cauldron.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Food_FarmersBreakfast",   "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi",		  pBehavior, "dz/anims/anm/player/ik/two_handed/Cauldron.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Food_MeatDumplings",      "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi",		  pBehavior, "dz/anims/anm/player/ik/two_handed/Cauldron.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Food_Sausage_Breadplate", "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi",		  pBehavior, "dz/anims/anm/player/ik/two_handed/Cauldron.anm");
	}
}