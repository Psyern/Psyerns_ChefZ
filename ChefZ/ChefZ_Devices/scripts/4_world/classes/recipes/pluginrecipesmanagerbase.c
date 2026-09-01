modded class PluginRecipesManager 
{
	override void RegisterRecipies()
    {
        super.RegisterRecipies();
        RegisterRecipe(new Pack_BeeHive);
        RegisterRecipe(new Pack_BeeKeeper);
        RegisterRecipe(new Pack_ButterChurn);
        RegisterRecipe(new Pack_CheesePress);
        RegisterRecipe(new Pack_GrainMill);
        RegisterRecipe(new Pack_MeatGrinder);
        RegisterRecipe(new Pack_Mortar);
        RegisterRecipe(new Pack_Smoker);
        RegisterRecipe(new Pack_DryRack);
    }
}


