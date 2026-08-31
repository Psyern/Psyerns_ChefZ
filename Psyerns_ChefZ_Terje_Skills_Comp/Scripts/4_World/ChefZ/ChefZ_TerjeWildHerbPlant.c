// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeSkills
// geladen ist. Fehlt der Mod, ist TERJE_SKILLS_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_SKILLS_MOD
//==============================================================================
// Die WILDPFLANZEN im Kraeuterkundigen - Stufe I hebt hervor, was in der
// Welt steht
//
// ---------------------------------------------------------------------------
// Warum diese Datei ueberhaupt noetig wurde
// ---------------------------------------------------------------------------
// Bis zum 30.08.2026 lagen die Kraeuter als Erntebund frei in der Welt; das
// Hervorheben sass deshalb an ChefZ_FreshHerbBase (ChefZ_TerjeHerbItem.c).
// Mit dem Wildwuchs-System vom 31.08.2026
// (Psyerns_ChefZ_Docs/ChefZ_Wildwuchs_Spawn_Plan.md) verteilt die CE nicht
// mehr das Bund, sondern die PFLANZE: ChefZ_WildThyme, ChefZ_WildRosemary,
// ChefZ_WildParsley (ChefZ_Farming/Scripts/4_World/ChefZ/Farming/
// ChefZ_WildPlants.c). Das Erntebund entsteht erst BEIM Ernten und liegt dann
// zwei Meter neben dem Spieler, der es gerade geerntet hat - hervorzuheben ist
// dort nichts mehr.
//
// Ohne diese Datei waere Stufe I des Perks faktisch wirkungslos geworden: der
// Spieler haette nur noch das gesehen, was er selbst erzeugt hat.
// ChefZ_TerjeHerbItem.c bleibt trotzdem bestehen - ein fallengelassenes oder
// aus einem Container gefallenes Bund gibt es weiterhin.
//
// ---------------------------------------------------------------------------
// DREI EINZELNE modded class - NICHT eine auf ChefZ_WildPlant_Base
// ---------------------------------------------------------------------------
// Die vierte Wildpflanze ist ChefZ_WildCorn, und Mais ist KEIN Kraut. Eine
// modded class auf der gemeinsamen Basis haette die Maispflanze mit
// hervorgehoben und damit einen Kraeuterperk zu einem Allesfinder gemacht.
//
// Die zweite Sperre liegt in den Daten und wirkt unabhaengig davon: das
// Partikel haengt an ChefZ_TerjeHerbTag.IsHerb(ChefZ_YieldClass()), und
// ChefZ_Corn traegt kein CHEFZ_HERB (ChefZ_Farming/Config/Ingredients/
// Herbs.json fuehrt das Tag nur an Thymian, Rosmarin, Petersilie und den
// uebrigen Kraeutern). Zwei unabhaengige Gruende, dass Mais nicht leuchtet -
// einer davon reicht.
//
// Ein VIERTES Kraut als Wildpflanze braucht hier drei Zeilen (eine modded
// class nach demselben Muster). Das ist der Preis dafuer, die Basis nicht
// anzufassen, und er ist bewusst bezahlt.
//
// ---------------------------------------------------------------------------
// Was hier NICHT passiert
// ---------------------------------------------------------------------------
// KEINE XP. Die Wildernte zahlt in V1 null Erfahrung (Wildwuchs-Spec §5), und
// sie koennte es technisch auch gar nicht: PROCESS_HARVEST_WILD hat keinen
// Transform, der ChefZ_ProcessRunner meldet deshalb kein "process", und der
// Core kennt ueberhaupt keine Fortschrittsart "harvest"
// (ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_EventNames.c:216-231). Siehe auch den
// Inert-Vermerk an "class ChefZ_Harvest" in der config.cpp.
//
// KEIN Eingriff in Ausbeute, Wurf oder Verfall. Diese Datei ist reine
// clientseitige Anzeige; die Ausbeutetabelle bleibt Sache von
// ChefZ_WildPlants.c.
//
// Die gesamte Entscheidungslogik und alle Terje-Fundstellen stehen in
// ChefZ_TerjeHerbHighlight.c. Hier steht nur, WER sie benutzt.
//
// Layer: 4_World.
//==============================================================================

// SCOUT-GEPRUEFT 2026-08-31: ChefZ-EIGENE Klasse aus ChefZ_Farming, kein
// Vanilla-Typ - die Kollisionsflaeche gegenueber fremden Mods ist die
// Klasse selbst, und die kennt ausser ChefZ niemand. super in EEDelete und
// OnTerjeClientUpdate; IsTerjeClientUpdateRequired ist ein konstanter
// Bool-Getter und ruft absichtlich keines (Vorbild
// TerjeSkills/Scripts/4_World/Entities/MushroomBase.c:5-8).
modded class ChefZ_WildThyme
{
    private Particle m_ChefZ_TerjeHighlight;

    override bool IsTerjeClientUpdateRequired()
    {
        return true;
    }

    override void EEDelete(EntityAI parent)
    {
        super.EEDelete(parent);
        m_ChefZ_TerjeHighlight = ChefZ_TerjeHerbHighlight.Release(m_ChefZ_TerjeHighlight);
    }

    override void OnTerjeClientUpdate(float deltaTime)
    {
        super.OnTerjeClientUpdate(deltaTime);

        // ChefZ_YieldClass() und nicht GetType(): geprueft wird, ob das, was
        // die Pflanze HERGIBT, ein Kraut ist. Die Pflanze selbst ist eine
        // Station und traegt kein Zutatentag.
        m_ChefZ_TerjeHighlight = ChefZ_TerjeHerbHighlight.Apply(this, ChefZ_YieldClass(), m_ChefZ_TerjeHighlight);
    }
}

// SCOUT-GEPRUEFT 2026-08-31: ChefZ-eigene Klasse aus ChefZ_Farming, wie
// ChefZ_WildThyme darueber - dieselbe Pruefung, derselbe Rumpf.
modded class ChefZ_WildRosemary
{
    private Particle m_ChefZ_TerjeHighlight;

    override bool IsTerjeClientUpdateRequired()
    {
        return true;
    }

    override void EEDelete(EntityAI parent)
    {
        super.EEDelete(parent);
        m_ChefZ_TerjeHighlight = ChefZ_TerjeHerbHighlight.Release(m_ChefZ_TerjeHighlight);
    }

    override void OnTerjeClientUpdate(float deltaTime)
    {
        super.OnTerjeClientUpdate(deltaTime);
        m_ChefZ_TerjeHighlight = ChefZ_TerjeHerbHighlight.Apply(this, ChefZ_YieldClass(), m_ChefZ_TerjeHighlight);
    }
}

// SCOUT-GEPRUEFT 2026-08-31: ChefZ-eigene Klasse aus ChefZ_Farming, wie
// ChefZ_WildThyme oben - dieselbe Pruefung, derselbe Rumpf.
modded class ChefZ_WildParsley
{
    private Particle m_ChefZ_TerjeHighlight;

    override bool IsTerjeClientUpdateRequired()
    {
        return true;
    }

    override void EEDelete(EntityAI parent)
    {
        super.EEDelete(parent);
        m_ChefZ_TerjeHighlight = ChefZ_TerjeHerbHighlight.Release(m_ChefZ_TerjeHighlight);
    }

    override void OnTerjeClientUpdate(float deltaTime)
    {
        super.OnTerjeClientUpdate(deltaTime);
        m_ChefZ_TerjeHighlight = ChefZ_TerjeHerbHighlight.Apply(this, ChefZ_YieldClass(), m_ChefZ_TerjeHighlight);
    }
}

// ChefZ_WildCorn steht hier ABSICHTLICH NICHT. Mais ist kein Kraut; die
// Begruendung steht im Dateikopf.
#endif // TERJE_SKILLS_MOD
