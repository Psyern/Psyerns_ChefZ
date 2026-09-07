modded class PlantBase
{
	void ChefZ_SetCropsCount(int count)
	{
		if (count < 0)
			count = 0;

		m_CropsCount = count;
	}

	int ChefZ_GetCropsCount()
	{
		return m_CropsCount;
	}

	void ChefZ_SetInfestationChance(float chance)
	{
		if (chance < 0)
			chance = 0;
		if (chance > 1)
			chance = 1;

		m_InfestationChance = chance;
	}

	void ChefZ_ApplyConfiguredGrowth()
	{
		ChefZ_Config_PlantEntryJson entry = ChefZ_Config_Plant.GetEntry(GetType());
		if (!entry)
			return;

		if (entry.MaturityTimeSeconds > 0)
		{
			m_FullMaturityTime = entry.MaturityTimeSeconds;
			DebugSetTimes(entry.MaturityTimeSeconds, 0, 0, 0);
		}

		int harvestMin = entry.HarvestAmountMin;
		int harvestMax = entry.HarvestAmountMax;
		if (harvestMax < harvestMin)
			harvestMax = harvestMin;

		ChefZ_SetCropsCount(harvestMin + Math.RandomInt(0, harvestMax - harvestMin + 1));
		ChefZ_SetInfestationChance(0);

		ChefZ_Logger.Log("[INIT] " + GetType() + " maturityTime=" + m_FullMaturityTime.ToString() + "s crops=" + ChefZ_GetCropsCount().ToString(), true);
		ChefZ_Logger.LogDebug("[INIT] " + GetType() + " state=" + GetPlantState().ToString() + " stage=" + GetPlantStateIndex().ToString());
	}

	void ChefZ_ApplyStageVisibility()
	{
		int stageIndex = GetPlantStateIndex();
		int i;
		for (i = 1; i <= ChefZ_Constants.PLANT_STAGE_COUNT; i++)
		{
			string sourceName = "plantStage_" + i.ToStringLen(2);
			if (stageIndex > 0 && i == stageIndex)
				ShowSelection(sourceName);
			else
				HideSelection(sourceName);
		}
	}

	void ChefZ_LogGrow(string tag)
	{
		int stageIndex = GetPlantStateIndex();
		string phases = "";
		int i;
		for (i = 1; i <= ChefZ_Constants.PLANT_STAGE_COUNT; i++)
		{
			string sourceName = "plantStage_" + i.ToStringLen(2);
			phases = phases + sourceName + "=" + GetAnimationPhase(sourceName).ToString() + " ";
		}

		ChefZ_Logger.LogDebug("[" + tag + "] " + GetType() + " stage=" + stageIndex.ToString() + " state=" + GetPlantState().ToString() + " harvestable=" + IsHarvestable().ToString() + " " + phases);
	}
}
