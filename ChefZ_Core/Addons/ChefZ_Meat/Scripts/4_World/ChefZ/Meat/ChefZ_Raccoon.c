//==============================================================================
// ChefZ_Raccoon - nach dem Tod bleibt ein Waschbaer liegen (31.10.2026).
//
// Der Fuchs tauscht sich beim Sterben gegen ein Item aus (AnimalBase.c:34,
// ReplaceOnDeath); der Name kommt aus GetDeadItemName. Geerbt war "DeadFox", also
// lag ein toter Rotfuchs da (Spieltest).
//
// ChefZ_DeadRaccoon erbt von DeadFox: Zerlegen (PrepareFox) ergibt weiter Fleisch
// und Fell des Fuchses, wie an der Tierklasse in ChefZ_Meat/config.cpp festgehalten.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_Raccoon extends Animal_VulpesVulpes
{
	override string GetDeadItemName()
	{
		return "ChefZ_DeadRaccoon";
	}
}
