//==============================================================================
// Aus der Pfanne essen (31.10.2026, Spieltest). Die Aktion liegt im Core
// (ChefZ_ActionEatFromPan), die Pfanne ist Content dieses Slice.
//
// Layer: 4_World.
//==============================================================================
modded class FryingPan
{
	override void SetActions()
	{
		super.SetActions();

		AddAction(ChefZ_ActionEatFromPan);
	}
}
