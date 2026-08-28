//==============================================================================
// Skriptklassen der Kraeuterpflanzen.
//
// Vanillas PlantBase bringt Wachstum, Bewaesserung, Schaedlinge, Ernte und
// Persistenz vollstaendig mit; die Ertragsmenge und die Ernteklasse stehen
// datengetrieben in der config.cpp (Horticulture CropsCount / CropsType,
// gelesen in PlantBase.c:63-65). Hier steht deshalb genau eine Zahl je Art:
// die Reifezeit.
//
// Massstab aus Vanilla (4_World/DayZ/Entities/ItemBase/Gear/Cultivation/
// cultivation.c): Zucchini 1350 s, Tomate 1650 s, Pfeffer 2250 s, Kartoffel
// und Kuerbis 2850 s. Kraeuter liegen darunter - sie sind Beiwerk, kein
// Grundnahrungsmittel, und die Kette dahinter (trocknen, moersern) kostet
// ohnehin Zeit. Seltenheit steuert die Production Map §21 ueber die
// VERFUEGBARKEIT der Samen, nicht ueber die Wachstumszeit.
//
// Kein Override ausser dem Konstruktor. Kein modded class.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_HerbPlantBase extends PlantBase {}

//! Petersilie - haeufig, schnell (Production Map §21).
class ChefZ_ParsleyPlant extends ChefZ_HerbPlantBase
{
    void ChefZ_ParsleyPlant()
    {
        m_FullMaturityTime = 900;
    }
}

//! Dill - haeufig bis mittel.
class ChefZ_DillPlant extends ChefZ_HerbPlantBase
{
    void ChefZ_DillPlant()
    {
        m_FullMaturityTime = 1050;
    }
}

//! Thymian - mittel.
class ChefZ_ThymePlant extends ChefZ_HerbPlantBase
{
    void ChefZ_ThymePlant()
    {
        m_FullMaturityTime = 1350;
    }
}

//! Rosmarin - selten, langsam. Der Holzstrauch braucht am laengsten.
class ChefZ_RosemaryPlant extends ChefZ_HerbPlantBase
{
    void ChefZ_RosemaryPlant()
    {
        m_FullMaturityTime = 2100;
    }
}

//! Baerlauch - mittel.
class ChefZ_WildGarlicPlant extends ChefZ_HerbPlantBase
{
    void ChefZ_WildGarlicPlant()
    {
        m_FullMaturityTime = 1350;
    }
}

//! Pfeffer - hochwertig und bewusst langsam (Production Map §16:
//! "Pfeffer sollte selten sein"). Gleiche Zeit wie Vanillas Plant_Pepper.
class ChefZ_PepperPlant extends ChefZ_HerbPlantBase
{
    void ChefZ_PepperPlant()
    {
        m_FullMaturityTime = 2250;
    }
}

//! Keine ChefZ-Paprikapflanze: Vanillas Plant_Pepper traegt GreenBellPepper und
//! hat mit PepperSeedsPack/PepperSeeds/CutOutPepperSeeds bereits einen
//! geschlossenen Kreis (Vanilla-Audit §2).
