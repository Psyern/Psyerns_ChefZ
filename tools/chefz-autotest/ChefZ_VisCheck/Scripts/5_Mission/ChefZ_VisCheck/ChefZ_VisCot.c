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
		Rebuild(cls);
		Print("[VISCHECK] cot " + cls + " dropdown=" + (m_LiquidType != null).ToString() + " values=" + m_LiquidValues.Count().ToString() + " milk=" + (m_LiquidValues.Find(LIQUID_CHEFZ_MILK) >= 0).ToString());
		if (m_LiquidType)
			m_LiquidType.ChefZ_VisOpen();
	}
}

modded class JMObjectSpawnerForm
{
	void ChefZ_VisOpen(string cls)
	{
		if (m_Props)
			m_Props.ChefZ_VisOpen(cls);
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
		if (m_Stage == 3 && m_Time > 27)
		{
			m_Stage = 4;
			Print("[VISCHECK] client done");
		}
	}
}
#endif
