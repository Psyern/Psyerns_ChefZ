//==============================================================================
// Meeresfruechte (26.10.2026). Skriptklassen zu den Configklassen gleichen
// Namens.
//
// Anders als die Fische werden Krebse, Hummer, Garnelen und Kopffuesser GANZ
// gegart - wie Vanillas Sardines (Sardines.c: CanBeCooked und
// CanBeCookedOnStick true). Es gibt kein Filet; die Config traegt deshalb
// FoodStages und Uebergaenge direkt an ChefZ_Seafood_Base.
//
// Layer: 4_World.
//==============================================================================

//! Gemeinsame Skriptbasis der Meeresfruechte (Configklasse scope = 0).
//! Bewusst NICHT von ChefZ_SaltwaterFishA_Base: deren CanBeCooked() sagt false.
class ChefZ_Seafood_Base extends ChefZ_Edible_Base
{
    //! Fleischzweig in ProcessDecay und blutige Haende beim Rohessen, wie Sardines.c.
    override bool IsMeat()
    {
        return true;
    }

    //! Garnelen und Tintenfisch am Spiess - Vanillas Sardines koennen es ebenfalls.
    //! CanBeCooked() kommt aus ChefZ_Edible_Base (rechnet gegen die Garstufen).
    override bool CanBeCookedOnStick()
    {
        return true;
    }

    override void SetActions()
    {
        super.SetActions();

        AddAction(ActionForceFeed);
        AddAction(ActionEatMeat);
    }
}

class ChefZ_BlueSentinelCrab extends ChefZ_Seafood_Base {}
class ChefZ_CoastalCrab extends ChefZ_Seafood_Base {}
class ChefZ_CrimsonKingCrab extends ChefZ_Seafood_Base {}
class ChefZ_Langoustine extends ChefZ_Seafood_Base {}
class ChefZ_MidnightLobster extends ChefZ_Seafood_Base {}
class ChefZ_SnowCrab extends ChefZ_Seafood_Base {}
class ChefZ_SpinyLobster extends ChefZ_Seafood_Base {}
class ChefZ_SpinyOrangeCrab extends ChefZ_Seafood_Base {}
class ChefZ_StripedTigerPrawn extends ChefZ_Seafood_Base {}
class ChefZ_CrimsonSquid extends ChefZ_Seafood_Base {}
class ChefZ_NorthernCrimsonSquid extends ChefZ_Seafood_Base {}
class ChefZ_Octopus extends ChefZ_Seafood_Base {}
class ChefZ_SpeckledSquid extends ChefZ_Seafood_Base {}
class ChefZ_Squid extends ChefZ_Seafood_Base {}
