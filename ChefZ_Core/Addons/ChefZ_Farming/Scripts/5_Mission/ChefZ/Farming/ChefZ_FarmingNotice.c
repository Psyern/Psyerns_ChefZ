//==============================================================================
// ChefZ_FarmingNotice - legt Farming.json beim Serverstart an
//
// Ohne diesen Haken entstuende die Datei erst beim ersten Takt eines Bienenstocks
// oder Regals, und ein Betreiber faende sie nach dem ersten Start nicht vor.
// ChefZ_CompNotice ist der eine Starthaken des Core; ein eigener MissionServer-
// Override ist hier verboten (Kopf von ChefZ_CompNotice).
//
// Layer: 5_Mission.
//==============================================================================

modded class ChefZ_CompNotice
{
	override void Emit()
	{
		super.Emit();
		ChefZ_FarmingSettings.EnsureLoaded();
	}
}
