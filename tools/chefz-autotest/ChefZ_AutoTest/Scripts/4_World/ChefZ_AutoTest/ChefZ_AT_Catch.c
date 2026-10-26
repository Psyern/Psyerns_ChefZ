//==============================================================================
// ChefZ_AutoTest - Zugriff auf die Fangtabelle der Engine.
//
// Die Kontexte bauen ihr Wahrscheinlichkeitsfeld (je Gewichtspunkt ein
// Eintrag) im Konstruktor. Diese Unterklassen lesen es nur aus und koennen
// einen bestimmten Ertrag ueber den normalen Spawnweg der Engine erzeugen.
// Keine Unterklasse hat einen eigenen Konstruktor - wie die Vanilla-Reusen.
//==============================================================================

class ChefZ_AT_CatchRead
{
    static void Weights(YieldsMap all, array<int> field, notnull map<string, int> outWeights)
    {
        outWeights.Clear();
        if (!field)
            return;
        foreach (int hash : field)
        {
            YieldItemBase y = all.Get(hash);
            if (!y)
                continue;
            string t = y.GetType();
            if (outWeights.Contains(t))
                outWeights.Set(t, outWeights.Get(t) + 1);
            else
                outWeights.Set(t, 1);
        }
    }
}

class ChefZ_AT_RodCtx : CatchingContextFishingRodAction
{
    void ChefZ_AT_Weights(notnull map<string, int> outWeights)
    {
        ChefZ_AT_CatchRead.Weights(m_YieldsMapAll, m_ProbabilityArray, outWeights);
    }

    EntityAI ChefZ_AT_SpawnAs(string type, vector pos)
    {
        YieldItemBase y = m_YieldsMapAll.Get(type.Hash());
        if (!y)
            return null;
        m_Result.SetYieldItem(y);
        int idx;
        return SpawnAndSetupCatch(idx, pos);
    }
}

class ChefZ_AT_TrapSmallCtx : CatchingContextTrapFishSmall
{
    void ChefZ_AT_Weights(notnull map<string, int> outWeights)
    {
        ChefZ_AT_CatchRead.Weights(m_YieldsMapAll, m_ProbabilityArray, outWeights);
    }
    //! Die Reuse steht im Test an Land: der Kontext hat sich beim Bau ungueltig
    //! markiert, UpdateDataAndMasks setzt das nicht zurueck.
    void ChefZ_AT_Revalidate()
    {
        m_IsValid = m_CompatibleItems && m_CompatibleItems.Count() > 0;
    }
    EntityAI ChefZ_AT_SpawnAs(string type, vector pos)
    {
        YieldItemBase y = m_YieldsMapAll.Get(type.Hash());
        if (!y)
            return null;
        m_Result.SetYieldItem(y);
        int idx;
        return SpawnAndSetupCatch(idx, pos);
    }
}

class ChefZ_AT_TrapLargeCtx : CatchingContextTrapFishLarge
{
    void ChefZ_AT_Weights(notnull map<string, int> outWeights)
    {
        ChefZ_AT_CatchRead.Weights(m_YieldsMapAll, m_ProbabilityArray, outWeights);
    }
    //! Die Reuse steht im Test an Land: der Kontext hat sich beim Bau ungueltig
    //! markiert, UpdateDataAndMasks setzt das nicht zurueck.
    void ChefZ_AT_Revalidate()
    {
        m_IsValid = m_CompatibleItems && m_CompatibleItems.Count() > 0;
    }
    EntityAI ChefZ_AT_SpawnAs(string type, vector pos)
    {
        YieldItemBase y = m_YieldsMapAll.Get(type.Hash());
        if (!y)
            return null;
        m_Result.SetYieldItem(y);
        int idx;
        return SpawnAndSetupCatch(idx, pos);
    }
}
