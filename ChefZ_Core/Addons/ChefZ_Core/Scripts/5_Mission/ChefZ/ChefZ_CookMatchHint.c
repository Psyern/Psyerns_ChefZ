//==============================================================================
// Gruene Markierung im Kochgefaess (31.10.2026, Spieltest).
//
// Ergeben die Zutaten in Pfanne oder Topf ein Rezept, bekommen genau die
// gebundenen Items im Inventar einen gruenen Rahmen. Reine Anzeige auf dem Client:
// dieselbe Auswertung wie beim Kochen (EvaluateBest), aber ohne Sitzung und Wirkung.
//
// Der Rahmen ist das Panel "Selected" (Stil Outline, Prioritaet 999). Das Panel
// "Color" liegt unter dem Itembild, und auf dem PC nutzt Vanilla "Selected" nicht.
//
// Layer: 5_Mission.
//==============================================================================
class ChefZ_CookMatchHint
{
	//! Die Auswertung ist teurer als ein Icon-Takt, und ein Gefaess hat viele Icons.
	static const int REFRESH_MS = 500;
	static const int MAX_VESSELS = 8;

	protected static ref map<ItemBase, ref array<ItemBase>> s_Matched = new map<ItemBase, ref array<ItemBase>>();
	protected static ref map<ItemBase, int> s_NextRefresh = new map<ItemBase, int>();

	static bool IsMatched(ItemBase item)
	{
		ItemBase vessel = ItemBase.Cast(item.GetHierarchyParent());
		if (!vessel || !vessel.IsCookware())
			return false;

		int now = g_Game.GetTime();
		int next;
		if (!s_NextRefresh.Find(vessel, next) || now >= next)
		{
			if (s_NextRefresh.Count() >= MAX_VESSELS)
			{
				s_NextRefresh.Clear();
				s_Matched.Clear();
			}
			s_NextRefresh.Set(vessel, now + REFRESH_MS);
			s_Matched.Set(vessel, Evaluate(vessel));
		}

		array<ItemBase> matched = s_Matched.Get(vessel);
		return matched && matched.Find(item) >= 0;
	}

	//! Mit Fluessigkeit wird gesotten, sonst gebacken - wie Vanillas Cooking.
	protected static int MethodOf(notnull ItemBase vessel)
	{
		if (vessel.IsLiquidContainer() && vessel.GetQuantity() > 0)
			return CookingMethodType.BOILING;
		return CookingMethodType.BAKING;
	}

	protected static array<ItemBase> Evaluate(notnull ItemBase vessel)
	{
		array<ItemBase> matched = new array<ItemBase>();
		if (!ChefZ_RecipeEngine.Get().IsReady())
			return matched;

		ChefZ_CookContext ctx;
		array<ItemBase> entities;
		ChefZ_FactSnapshot snapshot;
		if (!ChefZ_CookingDeviceAdapter.Get().BuildContext(vessel, 0.0, MethodOf(vessel), ctx, entities, snapshot))
			return matched;

		ChefZ_MatchResult result;
		if (!ChefZ_RecipeEngine.Get().EvaluateBest(ctx, snapshot, null, result))
			return matched;

		foreach (string slotId, array<int> handles : result.assignment)
		{
			foreach (int handle : handles)
			{
				if (handle >= 0 && handle < entities.Count())
					matched.Insert(entities.Get(handle));
			}
		}
		return matched;
	}
}

modded class Icon
{
	protected bool m_ChefZ_MatchShown;

	override void UpdateInterval()
	{
		super.UpdateInterval();

		if (!m_Item || m_HandsIcon || !m_SelectedPanel)
			return;

		bool matched = ChefZ_CookMatchHint.IsMatched(m_Item);
		if (matched)
		{
			m_SelectedPanel.SetColor(ARGB(255, 60, 220, 80));
			m_SelectedPanel.Show(true);
		}
		else if (m_ChefZ_MatchShown)
		{
			m_SelectedPanel.Show(false);
		}
		m_ChefZ_MatchShown = matched;
	}
}
