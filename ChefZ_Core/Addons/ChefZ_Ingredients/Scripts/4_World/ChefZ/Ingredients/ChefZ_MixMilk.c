//==============================================================================
// ChefZ_MixMilk - Wasser + Milchpulver = Milch im selben Behaelter (31.10.2026).
//
// Ein gewoehnliches Vanilla-Rezept: es verwandelt den Inhalt, also entsteht kein
// neues Item. Die Zutatenliste ist die von PourLiquid.c; die Masken dieser
// Behaelter nehmen LIQUID_CHEFZ_MILK auf (config.cpp).
//
// VERBRAUCH: 100 g Pulver je Liter, aber hoechstens eine volle Packung. Ohne die
// Obergrenze liesse sich ein Fass nie anruehren, denn eine Packung fasst 200 g.
//
// Die Registrierung haengt hinten an, nach dem Core und nach Vanilla. Auf Client
// und Server laeuft sie gleich, die Rezeptnummern bleiben also deckungsgleich
// (siehe ChefZ_HandcraftBridge.c).
//
// Layer: 4_World.
//==============================================================================

//! Vanilla vergibt die Bits bis 4194304 (constants.c:566-591). Derselbe Wert steht
//! in cfgLiquidDefinitions und den Behaeltermasken (config.cpp).
const int LIQUID_CHEFZ_MILK = 8388608;

class ChefZ_MixMilk extends RecipeBase
{
	//! Gramm Pulver je Liter Wasser.
	static const float POWDER_PER_LITER = 100.0;
	//! Eine volle Packung PowderedMilk, zugleich die Obergrenze je Mischung.
	static const float POWDER_MAX = 200.0;

	override void Init()
	{
		m_Name = "#STR_CHEFZ_RECIPE_MIX_MILK";
		m_IsInstaRecipe = false;
		m_AnimationLength = 1.5;
		m_Specialty = -0.01;

		m_MinDamageIngredient[0] = -1;
		m_MaxDamageIngredient[0] = 3;
		m_MinQuantityIngredient[0] = 1;
		m_MaxQuantityIngredient[0] = -1;

		m_MinDamageIngredient[1] = -1;
		m_MaxDamageIngredient[1] = 3;
		m_MinQuantityIngredient[1] = 1;
		m_MaxQuantityIngredient[1] = -1;

		InsertIngredient(0, "PowderedMilk");

		m_IngredientAddHealth[0] = 0;
		m_IngredientSetHealth[0] = -1;
		m_IngredientAddQuantity[0] = 0;
		m_IngredientDestroy[0] = false;
		m_IngredientUseSoftSkills[0] = false;

		InsertIngredient(1, "Pot");
		InsertIngredient(1, "Cauldron");
		InsertIngredient(1, "CanisterGasoline");
		InsertIngredient(1, "Canteen");
		InsertIngredient(1, "WaterBottle");
		InsertIngredient(1, "FilteringBottle");
		InsertIngredient(1, "GlassBottle");
		InsertIngredient(1, "WineBottle");
		InsertIngredient(1, "Waterskin_ColorBase");
		InsertIngredient(1, "Barrel_ColorBase");

		m_IngredientAddHealth[1] = 0;
		m_IngredientSetHealth[1] = -1;
		m_IngredientAddQuantity[1] = 0;
		m_IngredientDestroy[1] = false;
		m_IngredientUseSoftSkills[1] = false;
	}

	//! Gramm Pulver fuer diesen Behaelterinhalt. Die Menge eines Behaelters ist in ml.
	static float PowderNeeded(notnull ItemBase container)
	{
		float needed = container.GetQuantity() * 0.001 * POWDER_PER_LITER;
		return Math.Clamp(Math.Ceil(needed), 1.0, POWDER_MAX);
	}

	override bool CanDo(ItemBase ingredients[], PlayerBase player)
	{
		ItemBase powder = ingredients[0];
		ItemBase container = ingredients[1];
		if (!powder || !container)
			return false;
		if (container.GetQuantity() <= 0 || container.GetIsFrozen())
			return false;
		if ((container.GetLiquidType() & LIQUID_GROUP_DRINKWATER) == 0)
			return false;
		if ((container.GetLiquidContainerMask() & LIQUID_CHEFZ_MILK) == 0)
			return false;
		return powder.GetQuantity() >= PowderNeeded(container);
	}

	override void Do(ItemBase ingredients[], PlayerBase player, array<ItemBase> results, float specialty_weight)
	{
		ItemBase powder = ingredients[0];
		ItemBase container = ingredients[1];
		float needed = PowderNeeded(container);
		container.SetLiquidType(LIQUID_CHEFZ_MILK);
		powder.AddQuantity(-needed);
	}
}

modded class PluginRecipesManagerBase
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();
		RegisterRecipe(new ChefZ_MixMilk);
	}
}
