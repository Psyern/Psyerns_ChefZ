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
// Stufe I des Kraeuterkundigen, angewendet auf das ERNTE-ITEM: das Buendel
// Kraut, das am Boden liegt.
//
// ---------------------------------------------------------------------------
// WAS SICH AM 31.08.2026 GEAENDERT HAT
// ---------------------------------------------------------------------------
// Bis dahin war dieses Buendel das, was die CE in der Welt verteilte
// (Entscheidung vom 29.08.2026: "Kraeuter sind Fundpflanzen wie Vanillas
// Pilze"). Mit dem Wildwuchs-System verteilt die CE stattdessen die stehende
// PFLANZE (ChefZ_WildThyme/-Rosemary/-Parsley), und das Buendel entsteht erst
// durch die Ernte. Das Hervorheben der Pflanzen steht deshalb seither in
// ChefZ_TerjeWildHerbPlant.c.
//
// Diese Datei bleibt: ein fallengelassenes, aus einem zerstoerten Container
// gefallenes oder von einem Spieler abgelegtes Buendel liegt weiterhin frei in
// der Welt, und es soll fuer den Kraeuterkundigen weiterhin sichtbar sein.
//
// Die ENTSCHEIDUNG, wann etwas leuchtet, steht seit demselben Tag nur noch
// einmal im Modul - in ChefZ_TerjeHerbHighlight.c. Dort stehen auch alle
// Terje-Fundstellen (ItemBase.c:49/54, PluginTerjeClientItemsCore.c:13-27,
// MushroomBase.c:1-57, ParticleList.c:3). Haetten Buendel und Pflanze je eine
// eigene Kopie dieser Regel, waere die naechste Aenderung an Reichweite oder
// Tag zwangslaeufig nur an einer der beiden angekommen.
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
// ChefZ_ShouldHighlight praefixiert; am 31.08.2026 ist der Rumpf dieser
// Methode nach ChefZ_TerjeHerbHighlight.ShouldShow() gewandert - dieselbe
// Bedingungskette, unveraendert, nur an einer Stelle statt an zweien.
modded class ChefZ_FreshHerbBase
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

        // GetType() und nicht eine Ertragsklasse: das Buendel IST die Zutat,
        // die das Tag CHEFZ_HERB traegt. Bei der Wildpflanze ist das anders -
        // siehe ChefZ_TerjeWildHerbPlant.c.
        m_ChefZ_TerjeHighlight = ChefZ_TerjeHerbHighlight.Apply(this, GetType(), m_ChefZ_TerjeHighlight);
    }
}
#endif // TERJE_SKILLS_MOD
