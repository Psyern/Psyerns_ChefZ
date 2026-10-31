//==============================================================================
// Milch in den Fluessigkeitslisten von COT (31.10.2026).
//
// COT fuehrt seine Fluessigkeiten als feste Tabelle
// (JMObjectSpawnerProperties.LiquidTable, statisch). Deshalb haengt ChefZ die
// Zeile hinter dem Aufbau an: im Object Spawner fuer jeden Behaelter, dessen Maske
// das Milch-Bit traegt, im ESP-Aktionsmenue fuer jeden Behaelter.
//
// Layer: 5_Mission.
//==============================================================================

#ifdef JM_COT
modded class JMObjectSpawnerProperties
{
	override protected string LiquidName(int bit)
	{
		if (bit == LIQUID_CHEFZ_MILK)
			return "#STR_CHEFZ_LIQUID_MILK";
		return super.LiquidName(bit);
	}

	override protected void CollectLiquids(string classname, ItemBase item, out array<string> labels)
	{
		super.CollectLiquids(classname, item, labels);
		if (classname == "" || item.IsBloodContainer())
			return;
		if ((item.GetLiquidContainerMask() & LIQUID_CHEFZ_MILK) == 0)
			return;
		if (item.GetLiquidTypeInit() == LIQUID_CHEFZ_MILK)
			return;
		m_LiquidValues.Insert(LIQUID_CHEFZ_MILK);
		labels.Insert("#STR_CHEFZ_LIQUID_MILK");
	}
}

modded class JMESPActionMenu
{
	override protected array<string> LiquidNames()
	{
		array<string> names = super.LiquidNames();
		names.Insert("#STR_CHEFZ_LIQUID_MILK");
		return names;
	}

	override protected array<int> LiquidValues()
	{
		array<int> values = super.LiquidValues();
		values.Insert(LIQUID_CHEFZ_MILK);
		return values;
	}
}
#endif
