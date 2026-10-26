//==============================================================================
// ChefZ_AutoTest - Diagnose fuer Handwerksrezepte: welcher Schritt der
// Vanilla-Pruefung (Zutatenzuordnung, Bedingungen, CanDo/Bind) scheitert.
//==============================================================================

modded class ChefZ_GenericCraftRecipe
{
    string ChefZ_AT_TransformId()
    {
        return m_ChefZ_TransformId;
    }

    string ChefZ_AT_Explain(ItemBase a, ItemBase b, PlayerBase player)
    {
        string s = "slot=" + GetID().ToString() + " ready=" + m_ChefZ_Ready.ToString();
        bool match = CheckIngredientMatch(a, b);
        s = s + " zutaten=" + match.ToString();
        if (!match)
            return s;
        ItemBase sorted[2];
        sorted[0] = m_IngredientsSorted[0];
        sorted[1] = m_IngredientsSorted[1];
        s = s + " [" + sorted[0].GetType() + "|" + sorted[1].GetType() + "]";
        s = s + " bedingungen=" + CheckConditions(sorted).ToString();
        bool can = CanDo(sorted, player);
        s = s + " cando=" + can.ToString();
        if (m_ChefZ_In[0] && m_ChefZ_In[1])
            s = s + " in=[" + m_ChefZ_In[0].GetType() + "|" + m_ChefZ_In[1].GetType() + "]";
        if (!can && m_ChefZ_Match)
            s = s + " bind: " + m_ChefZ_Match.failReason;
        if (!can && m_ChefZ_Snapshot && m_ChefZ_Context)
        {
            ChefZ_MatchTrace trace = new ChefZ_MatchTrace();
            ChefZ_TransformMatch probe = new ChefZ_TransformMatch();
            ChefZ_ProcessingManager.Get().FindTransform(m_ChefZ_ProcessSym, m_ChefZ_Context, m_ChefZ_Snapshot, trace, probe);
            array<string> lines = new array<string>();
            trace.ToLines(lines);
            s = s + " fakten=" + m_ChefZ_Snapshot.Count().ToString() + " trace:";
            foreach (string l : lines)
                s = s + " | " + l;
        }
        return s;
    }

    //! Nimmt dieser Rezeptplatz genau diese zwei Items an? Fuer Gegenproben.
    bool ChefZ_AT_Accepts(ItemBase a, ItemBase b, PlayerBase player)
    {
        if (!CheckIngredientMatch(a, b))
            return false;
        ItemBase sorted[2];
        sorted[0] = m_IngredientsSorted[0];
        sorted[1] = m_IngredientsSorted[1];
        if (!CheckConditions(sorted))
            return false;
        return CanDo(sorted, player);
    }
}
