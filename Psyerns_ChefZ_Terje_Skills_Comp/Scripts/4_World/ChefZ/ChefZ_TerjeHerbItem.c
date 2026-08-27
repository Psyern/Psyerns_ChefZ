//==============================================================================
// modded class ChefZ_FreshHerbBase - hervorgehobene Kraeuter am Boden
//
// Die zweite Haelfte von Stufe I des Kraeuterkundigen. Die Pflanze im Beet
// erledigt ChefZ_TerjeHerbPlant.c; hier geht es um ein Buendel Kraut, das
// jemand fallen gelassen oder verloren hat.
//
// Woertlich nach dem Vorbild TerjeSkills/Scripts/4_World/Entities/
// MushroomBase.c, mit denselben drei Bausteinen:
//   - IsTerjeClientUpdateRequired() konstant true
//   - OnTerjeClientUpdate(float) als Sekundentakt
//   - GetHierarchyParent() == null als Bedingung "liegt frei in der Welt"
// Dazu kommen die Reichweite nach Perkstufe und die Tagpruefung, die es bei
// den Pilzen nicht braucht (dort ist die Klasse selbst schon die Bedingung).
//
// Erweitert wird wieder eine ChefZ-Klasse und keine Vanilla-Klasse: nur die
// sieben frischen ChefZ-Kraeuter sind betroffen.
//
// KEIN Eingriff in Ernte, Menge oder Verfall - diese Datei ist rein
// clientseitige Anzeige.
//
// Layer: 4_World.
//==============================================================================

modded class ChefZ_FreshHerbBase
{
    private Particle m_ChefZTerjeHighlight;

    override bool IsTerjeClientUpdateRequired()
    {
        return true;
    }

    override void EEDelete(EntityAI parent)
    {
        super.EEDelete(parent);

        if (GetGame() && GetGame().IsClient() && m_ChefZTerjeHighlight)
        {
            m_ChefZTerjeHighlight.Stop();
            m_ChefZTerjeHighlight = null;
        }
    }

    override void OnTerjeClientUpdate(float deltaTime)
    {
        super.OnTerjeClientUpdate(deltaTime);

        if (!GetGame() || !GetGame().IsClient())
            return;

        bool show = ShouldHighlight();

        if (show)
        {
            if (!m_ChefZTerjeHighlight)
            {
                m_ChefZTerjeHighlight = ParticleManager.GetInstance().PlayOnObject(
                    ParticleList.TERJE_SKILLS_MUSHROOMS_HIGHLIGHT, this);
            }
        }
        else if (m_ChefZTerjeHighlight)
        {
            m_ChefZTerjeHighlight.Stop();
            m_ChefZTerjeHighlight = null;
        }
    }

    protected bool ShouldHighlight()
    {
        if (!ChefZ_TerjeSkillsConfig.HighlightEnabled())
            return false;

        // Nur frei in der Welt liegend. Ein Kraut im Rucksack oder im
        // Kochtopf zu umleuchten waere sinnlos und wuerde bei jedem
        // Inventarblick Partikel erzeugen. Dieselbe Bedingung wie bei
        // MushroomBase.
        if (GetHierarchyParent() != null)
            return false;

        PlayerBase localPlayer = PlayerBase.Cast(GetGame().GetPlayer());
        if (!localPlayer)
            return false;

        int level = ChefZ_TerjeSkillsBridge.HerbalistLevel(localPlayer);
        if (level <= 0)
            return false;

        if (!ChefZ_TerjeHerbTag.IsHerb(GetType()))
            return false;

        float range = ChefZ_TerjeSkillsConfig.HighlightRange(level);
        if (range <= 0.0)
            return false;

        return vector.Distance(GetPosition(), localPlayer.GetPosition()) <= range;
    }
}
