modded class PluginRecipesManager 
{
	override void RegisterRecipies()
    {
        super.RegisterRecipies();
        RegisterRecipe(new Pack_BeeHive);
        RegisterRecipe(new Pack_BeeKeeper);
    }
}


