modded class Hologram
{
	override string ProjectionBasedOnParent()
	{
		ChefZ_Base_Kit kit = ChefZ_Base_Kit.Cast(m_Parent);

		if (kit)
			return kit.GetDeployedClassname();

		return super.ProjectionBasedOnParent();
	}

	override string GetProjectionName(ItemBase item)
	{
		ChefZ_Base_Kit kit = ChefZ_Base_Kit.Cast(m_Parent);

		if (kit)
			return kit.GetDeployedClassname();

		return super.GetProjectionName(item);
	}

	override EntityAI PlaceEntity(EntityAI entity_for_placing)
	{
		ChefZ_Base_Kit kit = ChefZ_Base_Kit.Cast(m_Parent);

		if (kit)
			return entity_for_placing;

		return super.PlaceEntity(entity_for_placing);
	}

	override void EvaluateCollision(ItemBase action_item = null)
	{
		if (m_Parent.IsInherited(ChefZ_Base_Kit))
		{
			SetIsColliding(false);
			return;
		}

		super.EvaluateCollision(action_item);
	}

	override vector GetDefaultOrientation()
	{
		ChefZ_Base_Kit kit = ChefZ_Base_Kit.Cast(m_Parent);

		if (kit)
			return super.GetDefaultOrientation() + kit.GetDeployOrientationOffset();

		return super.GetDefaultOrientation();
	}

	override void SetProjectionPosition(vector position)
	{
		ChefZ_Base_Kit kit = ChefZ_Base_Kit.Cast(m_Parent);

		if (kit)
		{
			m_Projection.SetPosition(position + kit.GetDeployPositionOffset());

			if (IsFloating())
				m_Projection.SetPosition(SetOnGround(position + kit.GetDeployPositionOffset()));

			return;
		}

		super.SetProjectionPosition(position);
	}
};