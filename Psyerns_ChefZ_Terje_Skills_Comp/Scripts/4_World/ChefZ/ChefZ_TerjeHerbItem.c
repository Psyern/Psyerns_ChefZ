// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeSkills
// geladen ist. Fehlt der Mod, ist TERJE_SKILLS_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_SKILLS_MOD
//==============================================================================
// modded class ChefZ_FreshHerbBase - hervorgehobene Kraeuter am Boden
//
// Stufe I des Kraeuterkundigen. Kraeuter sind Fundpflanzen wie Vanillas
// Pilze (Entscheidung vom 29.08.2026); das Buendel Kraut, das in der Welt
// liegt, IST die Pflanze - genau wie bei TerjeSkills' MushroomBase.
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

// SCOUT-GEPRUEFT 2026-08-30 (chefz-conflict-scout)
// super in EEDelete und OnTerjeClientUpdate; IsTerjeClientUpdateRequired
// ist ein konstanter Bool-Getter und ruft absichtlich keines (Vorbild
// TerjeSkills/MushroomBase.c). ShouldHighlight wurde am selben Tag zu
// ChefZ_ShouldHighlight praefixiert.
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

        if (g_Game && g_Game.IsClient() && m_ChefZTerjeHighlight)
        {
            m_ChefZTerjeHighlight.Stop();
            m_ChefZTerjeHighlight = null;
        }
    }

    override void OnTerjeClientUpdate(float deltaTime)
    {
        super.OnTerjeClientUpdate(deltaTime);

        if (!g_Game || !g_Game.IsClient())
            return;

        bool show = ChefZ_ShouldHighlight();

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

    protected bool ChefZ_ShouldHighlight()
    {
        if (!ChefZ_TerjeSkillsConfig.HighlightEnabled())
            return false;

        // Nur frei in der Welt liegend. Ein Kraut im Rucksack oder im
        // Kochtopf zu umleuchten waere sinnlos und wuerde bei jedem
        // Inventarblick Partikel erzeugen. Dieselbe Bedingung wie bei
        // MushroomBase.
        if (GetHierarchyParent() != null)
            return false;

        PlayerBase localPlayer = PlayerBase.Cast(g_Game.GetPlayer());
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
#endif // TERJE_SKILLS_MOD
