//==============================================================================
// modded class ChefZ_HerbPlantBase - Ausbeute, Erntemeldung und Hervorhebung
//
// Erweitert wird eine CHEFZ-Klasse, nicht Vanillas PlantBase. Das ist der
// ganze Unterschied zwischen "additiv" und "invasiv": ein Server, der dieses
// PBO laedt, sieht seine Kohlfelder, seine Vanilla-Kuerbisse und die Beete
// jedes anderen Mods voellig unveraendert. Betroffen sind ausschliesslich die
// sieben ChefZ-Kraeuterpflanzen, die von ChefZ_HerbPlantBase erben.
//
// ---------------------------------------------------------------------------
// Fundstellen
// ---------------------------------------------------------------------------
//   scripts/4_World/DayZ/Entities/GardenBase/PlantBase.c:567
//       void Harvest(PlayerBase player)
//       - gerufen aus ActionHarvestCrops.OnFinishProgressServer, also
//         SERVERSEITIG und erst NACH abgeschlossener Aktion. Genau der
//         Zeitpunkt, den Regel "XP nur nach erfolgreichem Abschluss"
//         verlangt: hier ist die Ernte bereits vollzogen.
//       - m_CropsCount und m_CropsType sind private; oeffentlich ist nur
//         GetCropsType() (PlantBase.c:220). Die Stueckzahl wird deshalb aus
//         DERSELBEN Configquelle gelesen, aus der PlantBase sie selbst
//         bezieht (PlantBase.c:64, "Horticulture CropsCount").
//
//   TerjeSkills/Scripts/4_World/Classes/Recipes/PrepareAnimal.c:117-128
//       das Vorbild fuer den Ausbeutebonus (MeatHunter): der Perkwert wird
//       auf die MENGE angewandt, nachdem Vanilla sein Ergebnis erzeugt hat.
//   TerjeSkills/Scripts/4_World/Classes/Recipes/PrepareFish.c:22-35
//       dasselbe fuer MasterFillet - erst super.Do(), dann die Menge
//       nachjustieren. Nie statt Vanilla, immer danach.
//
//   TerjeSkills/Scripts/4_World/Entities/MushroomBase.c
//       das Vorbild fuer die Hervorhebung: IsTerjeClientUpdateRequired() +
//       OnTerjeClientUpdate() + ParticleManager.PlayOnObject().
//   TerjeCore/Scripts/4_World/Entities/ItemBase.c:28,49,54
//       dort sind beide Methoden deklariert; PlantBase erbt von ItemBase.
//
// ---------------------------------------------------------------------------
// Warum der Ausbeutebonus ZUSAETZLICH spawnt statt m_CropsCount zu aendern
// ---------------------------------------------------------------------------
// m_CropsCount ist private und wird von Vanilla beim Duengen erhoeht
// (PlantBase.c:124). Ein Nachbau der Zaehlung wuerde die Duengerwirkung
// verlieren. Deshalb: super.Harvest() erledigt die vollstaendige
// Vanilla-Ernte einschliesslich Duenger, und der Perk legt danach seinen
// Anteil OBENDRAUF - berechnet auf der Grundmenge aus der Config. Das ist im
// gaertnerischen Zweifel zu wenig statt zu viel, und das ist die richtige
// Richtung.
//
// Layer: 4_World.
//==============================================================================

modded class ChefZ_HerbPlantBase
{
    //! Nur clientseitig belegt. Kein ref: ParticleManager haelt das Objekt.
    private Particle m_ChefZTerjeHighlight;

    //==========================================================================
    // Ernte - Server
    //==========================================================================

    override void Harvest(PlayerBase player)
    {
        // Der Zustand VOR super: danach ist m_HasCrops false, und
        // IsHarvestable() liefert immer noch false, wenn die Pflanze gar
        // nicht reif war. Ohne diese Zeile gaebe es Ausbeute und XP fuer den
        // Versuch, eine leere Pflanze zu ernten.
        bool didHarvest = IsHarvestable();

        super.Harvest(player);

        if (!didHarvest)
            return;
        if (!GetGame() || !GetGame().IsServer())
            return;
        if (!ChefZ_TerjeSkillsConfig.IsEnabled())
            return;
        if (!player)
            return;

        string cropsType = GetCropsType();
        if (cropsType == "")
            return;

        int baseCount = BaseCropsCount();
        int extra = GrantYieldBonus(player, cropsType, baseCount);

        GrantHarvestXp(player, cropsType, baseCount + extra);
    }

    /**
     * Die Grundstueckzahl aus der Pflanzenconfig.
     *
     * Dieselbe Zeile, die PlantBase.OnPlantStateChange selbst benutzt
     * (PlantBase.c:64). Fehlt der Eintrag, ist 1 die ehrlichste Annahme -
     * geerntet wurde ja etwas.
     */
    protected int BaseCropsCount()
    {
        int c = GetGame().ConfigGetInt("cfgVehicles " + GetType() + " Horticulture CropsCount");
        if (c < 1)
            c = 1;
        return c;
    }

    /**
     * Der Ausbeutebonus des Kraeuterkundigen.
     *
     * Greift NUR, wenn die Ernteklasse das Tag CHEFZ_HERB traegt - nicht,
     * weil sie zufaellig an einer Kraeuterpflanze haengt (Terje-Analyse §9).
     * Frische Paprika waechst an derselben Klassenfamilie und ist Gemuese;
     * sie bekommt hier deshalb nichts.
     *
     * Der Bruchteil wird ausgewuerfelt statt abgeschnitten: bei 3 Krautern
     * und +10 % waere 0.3 sonst dauerhaft null, und ein Perk, den man nie
     * bemerkt, ist kein Perk.
     *
     * @return wie viele zusaetzliche Items erzeugt wurden.
     */
    protected int GrantYieldBonus(PlayerBase player, string cropsType, int baseCount)
    {
        if (!ChefZ_TerjeSkillsConfig.YieldEnabled())
            return 0;
        if (!ChefZ_TerjeHerbTag.IsHerb(cropsType))
            return 0;

        float bonus = ChefZ_TerjeSkillsBridge.HerbalistYield(player);
        if (bonus <= 0.0)
            return 0;

        float exact = baseCount * bonus;
        int   extra = (int)Math.Floor(exact);
        if (Math.RandomFloat01() < (exact - extra))
            extra = extra + 1;

        if (extra <= 0)
            return 0;

        int created = 0;
        vector pos = player.GetPosition();
        for (int i = 0; i < extra; i++)
        {
            // Wortgleich zu PlantBase.Harvest:573-575, nur mit Nullpruefung.
            ItemBase item = ItemBase.Cast(GetGame().CreateObjectEx(cropsType, pos, ECE_PLACE_ON_SURFACE));
            if (!item)
                continue;
            item.SetQuantity(item.GetQuantityMax());
            created++;
        }

        return created;
    }

    /**
     * Survival-XP fuer die Ernte. Terje-Analyse §10 und §26.
     *
     * Je ERNTEVORGANG, nicht je Kraut - die Stueckzahl geht nur ueber den
     * gedeckelten Mengenbonus ein (§27). Und nur fuer Klassen, die eines der
     * Tags aus ChefZ_Harvest harvestTags[] tragen: eine Kartoffel an einer
     * ChefZ-Pflanze bekaeme sonst Kraeuter-XP.
     */
    protected void GrantHarvestXp(PlayerBase player, string cropsType, int totalCount)
    {
        if (!ChefZ_TerjeSkillsConfig.IsXpEnabled())
            return;
        if (!ChefZ_TerjeHerbTag.IsHarvestRelevant(cropsType))
            return;

        int xp = ChefZ_TerjeSkillsConfig.HarvestClassXp(
            cropsType, ChefZ_TerjeSkillsConfig.HarvestDefaultXp());
        if (xp <= 0)
            return;

        xp = xp + ChefZ_TerjeXpDamper.BatchBonus(xp, totalCount);

        PlayerIdentity ident = player.GetIdentity();
        if (!ident)
            return;

        // Eigener Schluesselraum: eine Kraeuterernte soll die Daempfung fuer
        // eine gleichnamige Verarbeitung nicht mit hochzaehlen.
        int percent = ChefZ_TerjeXpDamper.RepeatPercent(ident.GetPlayerId(),
            "HARVEST:" + cropsType);
        if (percent < 100)
        {
            xp = (xp * percent) / 100;
            if (xp < 1 && percent > 0)
                xp = 1;
        }

        ChefZ_TerjeSkillsBridge.AddSurvivalXp(player, xp,
            ChefZ_TerjeSkillsConfig.ShowNotification());
    }

    //==========================================================================
    // Hervorhebung - Client
    //==========================================================================

    override bool IsTerjeClientUpdateRequired()
    {
        // Konstant true, wie bei MushroomBase. Der Wert MUSS ueber die
        // Lebenszeit gleich bleiben: TerjeCore/Scripts/4_World/Entities/
        // ItemBase.c meldet in EEInit an und in EEDelete wieder ab, jeweils
        // abhaengig von dieser Antwort. Eine Antwort, die sich zwischendurch
        // aendert, wuerde einen Eintrag in der Plugin-Liste zuruecklassen.
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
                // Bewusst der bereits registrierte Terje-Partikel und kein
                // eigener: dieses Modul liefert keine Grafik aus. Fundstelle
                // TerjeSkills/Scripts/3_Game/ParticleList.c:3.
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

    /**
     * Soll diese Pflanze gerade leuchten?
     *
     * Vier Bedingungen, in der Reihenfolge ihrer Billigkeit:
     *   1. der Betreiber hat die Hervorhebung nicht abgeschaltet,
     *   2. die Pflanze ist erntereif (oder highlightUnripe steht auf 1),
     *   3. der lokale Spieler hat den Perk auf Stufe >= 1,
     *   4. die Pflanze liegt innerhalb der Reichweite dieser Stufe.
     *
     * Die Tagpruefung steht bewusst NACH der Perkabfrage: sie ist die
     * teuerste (Symboltabelle + Zutatenregister) und der haeufigste Fall ist
     * "Spieler hat den Perk gar nicht".
     */
    protected bool ShouldHighlight()
    {
        if (!ChefZ_TerjeSkillsConfig.HighlightEnabled())
            return false;

        if (!IsHarvestable() && !ChefZ_TerjeSkillsConfig.HighlightUnripe())
            return false;

        PlayerBase localPlayer = PlayerBase.Cast(GetGame().GetPlayer());
        if (!localPlayer)
            return false;

        int level = ChefZ_TerjeSkillsBridge.HerbalistLevel(localPlayer);
        if (level <= 0)
            return false;

        if (!ChefZ_TerjeHerbTag.IsHerb(GetCropsType()))
            return false;

        float range = ChefZ_TerjeSkillsConfig.HighlightRange(level);
        if (range <= 0.0)
            return false;

        return vector.Distance(GetPosition(), localPlayer.GetPosition()) <= range;
    }
}
