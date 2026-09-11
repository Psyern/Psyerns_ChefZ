modded class PluginRecipesManager 
{
	override void RegisterRecipies()
    {
        super.RegisterRecipies();
        RegisterRecipe(new CoutOutChiliSeeds);
        RegisterRecipe(new CoutOutBlackberrySeeds);
        RegisterRecipe(new CoutOutStrawberrySeeds);
        RegisterRecipe(new CoutOutBlueberrySeeds);
        RegisterRecipe(new CoutOutRaspberrySeeds);
    }
}


