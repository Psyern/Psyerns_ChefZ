// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeSkills
// geladen ist. Fehlt der Mod, ist TERJE_SKILLS_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_SKILLS_MOD
//==============================================================================
// ChefZ_TerjeHerbHighlight - die EINE Regel, wann ein Kraut leuchtet
//
// Stufe I des Kraeuterkundigen (Terje-Analyse §7) hebt Kraeuter in der Welt
// hervor. Hervorzuheben sind seit dem 31.08.2026 ZWEI verschiedene Dinge:
//
//   1. das ERNTE-ITEM ChefZ_FreshHerbBase - ein Bund Kraut, das am Boden
//      liegt (ChefZ_TerjeHerbItem.c), und
//   2. die WILDPFLANZE ChefZ_WildThyme / ChefZ_WildRosemary /
//      ChefZ_WildParsley - die stehende Pflanze, die die CE verteilt und die
//      man erntet (ChefZ_TerjeWildHerbPlant.c). Seit dem Wildwuchs-System
//      (Psyerns_ChefZ_Docs/ChefZ_Wildwuchs_Spawn_Plan.md) ist SIE das, was in
//      der Welt steht; das Erntebund entsteht erst durch die Ernte.
//
// Beide sollen sich gleich verhalten. Die Entscheidung steht deshalb genau
// hier und nicht zweimal: ein spaeter geaenderter Schalter (Reichweite,
// Perkstufe, Tag) wirkt sonst auf der einen Seite und auf der anderen nicht.
//
// ---------------------------------------------------------------------------
// Warum die Klasse gefragt wird und nicht die Entitaet
// ---------------------------------------------------------------------------
// Die Tagpruefung laeuft ueber ChefZ_TerjeHerbTag.IsHerb(<Klassenname>) und
// damit ueber die ChefZ-Zutatendaten. Ein ERNTE-ITEM ist selbst eine Zutat -
// dort ist der Klassenname die eigene Klasse. Eine WILDPFLANZE ist KEINE
// Zutat, sondern eine Mini-Station (ChefZ_WildPlants.c); sie traegt kein Tag
// und wird nie eines tragen. Fuer sie ist die richtige Frage: "ist das, WAS
// SIE HERGIBT, ein Kraut?" - also ChefZ_WildPlant_Base.ChefZ_YieldClass().
//
// Deshalb nimmt ShouldShow() den Klassennamen als Parameter und liest ihn
// nicht selbst aus der Entitaet.
//
// ---------------------------------------------------------------------------
// Terje-Fundstellen (im Fremdcode nachgeschlagen)
// ---------------------------------------------------------------------------
//   TerjeCore/Scripts/4_World/Entities/ItemBase.c:49
//       void OnTerjeClientUpdate(float deltaTime)   - Sekundentakt, Client.
//   TerjeCore/Scripts/4_World/Entities/ItemBase.c:54
//       bool IsTerjeClientUpdateRequired()          - Vorgabe false.
//   TerjeCore/Scripts/4_World/Entities/ItemBase.c:22-46
//       EEInit/EEDelete melden das Item am Plugin an und wieder ab - genau
//       dann, wenn IsTerjeClientUpdateRequired() true ist.
//   TerjeCore/Scripts/4_World/Plugins/PluginTerjeClientItemsCore.c:13-27
//       der Takt selbst: einmal je Sekunde ueber alle angemeldeten Items.
//   TerjeSkills/Scripts/4_World/Entities/MushroomBase.c:1-57
//       das VORBILD: Partikel ueber ParticleManager.PlayOnObject(), Bedingung
//       GetHierarchyParent() == null, Abfrage der Perkstufe, Stop() beim
//       Wegfallen.
//   TerjeSkills/Scripts/3_Game/ParticleList.c:3
//       TERJE_SKILLS_MUSHROOMS_HIGHLIGHT - dasselbe Partikel wie bei den
//       Pilzen. Ein eigenes waere ein eigenes Asset; dieses Modul liefert
//       keines aus.
//
// KEIN Eingriff in Ernte, Menge oder Verfall - reine clientseitige Anzeige.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_TerjeHerbHighlight
{
    /**
     * Soll an dieser Stelle ein Kraut leuchten?
     *
     * @param what      das Objekt, an dem das Partikel haengen wuerde.
     * @param herbClass die Klasse, die auf das Tag CHEFZ_HERB geprueft wird -
     *                  beim Erntebund die eigene, bei der Wildpflanze ihre
     *                  Ertragsklasse.
     *
     * false ist die haeufige, normale Antwort. Kein Log: diese Funktion laeuft
     * clientseitig im Sekundentakt fuer jedes angemeldete Objekt.
     */
    static bool ShouldShow(EntityAI what, string herbClass)
    {
        if (!what)
            return false;

        if (!ChefZ_TerjeSkillsConfig.HighlightEnabled())
            return false;

        // Nur der Client zeigt etwas an. Auf einem Listen-Server laeuft beides
        // im selben Prozess; die Abfrage haelt den reinen Server heraus.
        if (!g_Game || !g_Game.IsClient())
            return false;

        // Nur frei in der Welt. Ein Kraut im Rucksack oder im Kochtopf zu
        // umleuchten waere sinnlos und wuerde bei jedem Inventarblick Partikel
        // erzeugen. Dieselbe Bedingung wie bei MushroomBase.c:27. Fuer eine
        // Wildpflanze ist sie immer erfuellt - sie kommt nie in ein Inventar
        // (ChefZ_WildPlants.c, IsTakeable/CanPutInCargo/CanPutIntoHands) -,
        // und genau deshalb kostet sie dort nichts und bleibt stehen.
        if (what.GetHierarchyParent() != null)
            return false;

        PlayerBase localPlayer = PlayerBase.Cast(g_Game.GetPlayer());
        if (!localPlayer)
            return false;

        int level = ChefZ_TerjeSkillsBridge.HerbalistLevel(localPlayer);
        if (level <= 0)
            return false;

        if (!ChefZ_TerjeHerbTag.IsHerb(herbClass))
            return false;

        float range = ChefZ_TerjeSkillsConfig.HighlightRange(level);
        if (range <= 0.0)
            return false;

        return vector.Distance(what.GetPosition(), localPlayer.GetPosition()) <= range;
    }

    /**
     * Partikel an- oder abschalten und den neuen Stand zurueckgeben.
     *
     * Der Aufrufer haelt das Partikel selbst (ein Feld je Objekt) und schreibt
     * den Rueckgabewert dorthin zurueck. Ein out-Parameter waere kuerzer, aber
     * der Rueckgabewert macht an der Aufrufstelle sichtbar, dass sich das Feld
     * aendert.
     *
     * ParticleManager.PlayOnObject liefert ParticleSource; ParticleSource ist
     * ein Particle (scripts - 1.29, ParticleSource.c:123), und Particle ist
     * eine Entity (ParticleBase.c:60) - eine EINFACHE Referenz ist deshalb
     * richtig, kein ref. Genauso haelt Terje sein Partikel
     * (MushroomBase.c:3).
     */
    static Particle Apply(EntityAI what, string herbClass, Particle current)
    {
        if (ShouldShow(what, herbClass))
        {
            if (current)
                return current;

            return ParticleManager.GetInstance().PlayOnObject(
                ParticleList.TERJE_SKILLS_MUSHROOMS_HIGHLIGHT, what);
        }

        return Release(current);
    }

    /**
     * Ein laufendes Partikel beenden. Immer ueber den Rueckgabewert null
     * setzen - ein gestopptes, aber noch gehaltenes Partikel waere beim
     * naechsten Takt ein "laeuft schon".
     *
     * Eine Seitenpruefung braucht es hier nicht: auf dem Server ist current
     * immer null, weil Apply() dort nie eines anlegt.
     */
    static Particle Release(Particle current)
    {
        if (!current)
            return null;

        current.Stop();
        return null;
    }
}
#endif // TERJE_SKILLS_MOD
