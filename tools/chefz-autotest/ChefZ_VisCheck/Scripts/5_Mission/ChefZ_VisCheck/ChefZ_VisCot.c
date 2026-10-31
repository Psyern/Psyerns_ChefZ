//! "cot <Klasse>": oeffnet den COT Object Spawner samt Fluessigkeits-Dropdown und fotografiert ihn.
#ifdef JM_COT
modded class UIActionDropdown
{
	void ChefZ_VisOpen()
	{
		SetOpen(true);
	}
}

modded class JMObjectSpawnerProperties
{
	void ChefZ_VisOpen(string cls)
	{
		Print("[VISCHECK] cot " + cls + " dropdown=" + (m_LiquidType != null).ToString() + " values=" + m_LiquidValues.Count().ToString() + " milk=" + (m_LiquidValues.Find(LIQUID_CHEFZ_MILK) >= 0).ToString());
		if (m_LiquidType)
			m_LiquidType.ChefZ_VisOpen();
	}

	//! Die Milch-Zeile waehlen wie ein Klick: Dropdown zu, Kopfzeile zeigt den Text.
	void ChefZ_VisPickMilk()
	{
		int row = m_LiquidValues.Find(LIQUID_CHEFZ_MILK);
		Print("[VISCHECK] cot pick row=" + row.ToString());
		m_LiquidType.SetSelection(row, true);
		m_LiquidType.Close();
		Print("[VISCHECK] cot picked value=" + m_LiquidValue.ToString());
	}

	void ChefZ_VisRead(out float health, out float temp, out float quantity, out int itemState)
	{
		ReadSpawnValues(health, temp, quantity, itemState);
	}
}

modded class JMObjectSpawnerForm
{
	void ChefZ_VisOpen(string cls)
	{
		m_Module.m_SearchText = cls;
		m_SearchBox.SetText(cls);
		UpdateList();
		int row = m_ListClasses.Find(cls);
		Print("[VISCHECK] cot row=" + row.ToString());
		m_ClassList.SetSelectedIndex(row);
		UpdateItemPreview();
		if (m_Props)
			m_Props.ChefZ_VisOpen(cls);
	}

	void ChefZ_VisPickMilk()
	{
		m_Props.ChefZ_VisPickMilk();
	}

	//! Wie der Spawn-Knopf in der Betriebsart "Spielerposition" (jmobjectspawnerform.c:862).
	void ChefZ_VisSpawn(string cls)
	{
		float health;
		float temp;
		float quantity;
		int itemState;
		m_Props.ChefZ_VisRead(health, temp, quantity, itemState);
		Print("[VISCHECK] cot spawn state=" + itemState.ToString());
		m_Module.SpawnEntity_Position(cls, g_Game.GetPlayer().GetPosition(), quantity, health, temp, itemState);
	}
}

class ChefZ_VisCotClient
{
	protected string m_Class;
	protected float m_Time;
	protected int m_Stage;
	protected JMObjectSpawnerModule m_Module;

	void ChefZ_VisCotClient(string cls)
	{
		m_Class = cls;
	}

	void Update(float dt)
	{
		m_Time += dt;
		if (m_Stage == 0 && m_Time > 15)
		{
			m_Stage = 1;
			m_Module = JMObjectSpawnerModule.Cast(GetModuleManager().GetModule(JMObjectSpawnerModule));
			if (!m_Module)
			{
				Print("[VISCHECK] cot module fehlt");
				return;
			}
			Print("[VISCHECK] cot access=" + m_Module.HasAccess().ToString());
			m_Module.Show();
			return;
		}
		if (m_Stage == 1 && m_Time > 20)
		{
			m_Stage = 2;
			JMObjectSpawnerForm form = JMObjectSpawnerForm.Cast(m_Module.GetForm());
			Print("[VISCHECK] cot form=" + (form != null).ToString());
			if (form)
				form.ChefZ_VisOpen(m_Class);
			return;
		}
		if (m_Stage == 2 && m_Time > 23)
		{
			m_Stage = 3;
			Print("[VISCHECK] shot cot_dropdown");
			return;
		}
		JMObjectSpawnerForm picked = JMObjectSpawnerForm.Cast(m_Module.GetForm());
		if (m_Stage == 3 && m_Time > 27)
		{
			m_Stage = 4;
			picked.ChefZ_VisPickMilk();
			return;
		}
		if (m_Stage == 4 && m_Time > 30)
		{
			m_Stage = 5;
			Print("[VISCHECK] shot cot_milk");
			return;
		}
		if (m_Stage == 5 && m_Time > 34)
		{
			m_Stage = 6;
			picked.ChefZ_VisSpawn(m_Class);
			return;
		}
		if (m_Stage == 6 && m_Time > 40)
		{
			m_Stage = 7;
			array<Object> objects = new array<Object>();
			array<CargoBase> cargos = new array<CargoBase>();
			g_Game.GetObjectsAtPosition(g_Game.GetPlayer().GetPosition(), 3.0, objects, cargos);
			foreach (Object o : objects)
			{
				ItemBase bottle = ItemBase.Cast(o);
				if (bottle && bottle.IsKindOf(m_Class))
					Print("[VISCHECK] cot spawned " + bottle.GetType() + " liquid=" + bottle.GetLiquidType().ToString() + " qty=" + bottle.GetQuantity().ToString());
			}
			Print("[VISCHECK] client done");
		}
	}
}
#endif
