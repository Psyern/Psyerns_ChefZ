modded class ModItemRegisterCallbacks
{
	override void RegisterOneHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
	{
		super.RegisterOneHanded(pType, pBehavior);

		pType.AddItemInHandsProfileIK("ChefZ_Plant_Corn_Cob", "dz/anims/workspaces/player/player_main/props/player_main_1h_sodacan.asi", pBehavior, "dz/anims/anm/player/ik/gear/soda_can.anm");
		pType.AddItemInHandsProfileIK("ChefZ_Plant_ChiliSeeds", "dz/anims/workspaces/player/player_main/player_main_1h.asi", pBehavior, "dz/anims/anm/player/ik/gear/zucchini_seeds_pack.anm");
	}
}
