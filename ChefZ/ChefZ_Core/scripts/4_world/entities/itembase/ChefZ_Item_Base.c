class ChefZ_Deployed_Base : DeployableContainer_Base
{
	override bool IsContainer()
	{
		return false;
	}
	
	override bool IsDeployable()
	{
		return true;
	}

	override bool CanPutInCargo(EntityAI parent)
	{
		return false;
	}

	override bool CanPutIntoHands(EntityAI parent)
	{
		return false;
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionTogglePlaceObject);
		AddAction(ActionPlaceObject);
	}
};

