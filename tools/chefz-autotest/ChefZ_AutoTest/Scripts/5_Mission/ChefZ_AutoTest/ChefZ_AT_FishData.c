//==============================================================================
// GENERIERT von tools/chefz-autotest/make_cases.py - nicht von Hand aendern.
// Fangertraege und Koeder aus den Fishing-JSONs.
//==============================================================================

class ChefZ_AT_FishData
{
    static void FillYields(notnull array<ref ChefZ_AT_Yield> list)
    {
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Perch", "POND", "ROD TRAP_SMALL TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Muskellunge", "POND", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Snakehead", "POND", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_RainbowTrout", "POND", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Char", "POND", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Salmon", "BOTH", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_SockeyeSalmon", "BOTH", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Eel", "POND", "ROD TRAP_SMALL TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("Mackerel", "SEA", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Plaice", "SEA", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_GiltheadBream", "SEA", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Halibut", "SEA", "ROD", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Cod", "SEA", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Sole", "SEA", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_SeaBass", "SEA", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_RedMullet", "SEA", "ROD TRAP_LARGE TRAP_SMALL", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Conger", "SEA", "ROD", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_SeaEel", "SEA", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_HorseMackerel", "SEA", "ROD TRAP_LARGE TRAP_SMALL", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_JapaneseSardine", "SEA", "ROD TRAP_LARGE TRAP_SMALL", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Rockfish", "SEA", "ROD TRAP_LARGE TRAP_SMALL", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_BluefinTuna", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_YellowfinTuna", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_RedSnapper", "SEA", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_GiantGrouper", "SEA", "ROD", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_MahiMahi", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_GiantTrevally", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_BlueMarlin", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Sailfish", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Bonito", "SEA", "ROD TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Yellowtail", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Tarpon", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_CoastalCrab", "SEA", "TRAP_LARGE TRAP_SMALL ROD", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_SpinyOrangeCrab", "SEA", "TRAP_LARGE TRAP_SMALL", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_BlueSentinelCrab", "SEA", "TRAP_LARGE TRAP_SMALL", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_SnowCrab", "SEA", "TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_CrimsonKingCrab", "SEA", "TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_StripedTigerPrawn", "SEA", "TRAP_SMALL TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Langoustine", "SEA", "TRAP_LARGE TRAP_SMALL", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_SpinyLobster", "SEA", "TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_MidnightLobster", "SEA", "TRAP_LARGE", false));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Squid", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_SpeckledSquid", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_CrimsonSquid", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_NorthernCrimsonSquid", "SEA", "ROD", true));
        list.Insert(ChefZ_AT_Yield.Make("ChefZ_Octopus", "SEA", "ROD", true));
    }

    static void FillBaits(notnull array<ref ChefZ_AT_Bait> list)
    {
        ChefZ_AT_Bait b;
        b = ChefZ_AT_Bait.Make("ChefZ_AntiqueSpoonFlasher", "lure");
        b.targets.Insert("Mackerel");
        b.targets.Insert("ChefZ_Salmon");
        b.targets.Insert("ChefZ_SockeyeSalmon");
        b.targets.Insert("ChefZ_RainbowTrout");
        b.targets.Insert("ChefZ_SeaBass");
        b.targets.Insert("ChefZ_Bonito");
        b.targets.Insert("ChefZ_Yellowtail");
        list.Insert(b);
        b = ChefZ_AT_Bait.Make("ChefZ_BlueBait", "soft");
        b.targets.Insert("ChefZ_Cod");
        b.targets.Insert("ChefZ_Halibut");
        b.targets.Insert("ChefZ_GiltheadBream");
        b.targets.Insert("ChefZ_RedSnapper");
        b.targets.Insert("ChefZ_GiantGrouper");
        b.targets.Insert("ChefZ_Rockfish");
        b.targets.Insert("ChefZ_RedMullet");
        list.Insert(b);
        b = ChefZ_AT_Bait.Make("ChefZ_MinnowLure", "lure");
        b.targets.Insert("ChefZ_Perch");
        b.targets.Insert("ChefZ_Muskellunge");
        b.targets.Insert("ChefZ_Snakehead");
        b.targets.Insert("ChefZ_SeaBass");
        b.targets.Insert("ChefZ_MahiMahi");
        b.targets.Insert("ChefZ_GiantTrevally");
        b.targets.Insert("ChefZ_Tarpon");
        list.Insert(b);
        b = ChefZ_AT_Bait.Make("ChefZ_PaddleTailBait", "soft");
        b.targets.Insert("ChefZ_Cod");
        b.targets.Insert("ChefZ_Halibut");
        b.targets.Insert("ChefZ_Sole");
        b.targets.Insert("ChefZ_Plaice");
        b.targets.Insert("ChefZ_Conger");
        b.targets.Insert("ChefZ_GiantGrouper");
        b.targets.Insert("ChefZ_Rockfish");
        b.targets.Insert("ChefZ_SeaEel");
        b.targets.Insert("ChefZ_Eel");
        b.targets.Insert("ChefZ_RedMullet");
        list.Insert(b);
        b = ChefZ_AT_Bait.Make("ChefZ_RedTopperBait", "lure");
        b.targets.Insert("ChefZ_MahiMahi");
        b.targets.Insert("ChefZ_GiantTrevally");
        b.targets.Insert("ChefZ_YellowfinTuna");
        b.targets.Insert("ChefZ_BluefinTuna");
        b.targets.Insert("ChefZ_BlueMarlin");
        b.targets.Insert("ChefZ_Sailfish");
        b.targets.Insert("ChefZ_Bonito");
        list.Insert(b);
        b = ChefZ_AT_Bait.Make("ChefZ_Spinnerbait", "lure");
        b.targets.Insert("ChefZ_Perch");
        b.targets.Insert("ChefZ_Muskellunge");
        b.targets.Insert("ChefZ_Snakehead");
        b.targets.Insert("ChefZ_Char");
        b.targets.Insert("ChefZ_SeaBass");
        b.targets.Insert("ChefZ_Yellowtail");
        list.Insert(b);
        b = ChefZ_AT_Bait.Make("ChefZ_TroutBait", "soft");
        b.targets.Insert("ChefZ_RainbowTrout");
        b.targets.Insert("ChefZ_Char");
        b.targets.Insert("ChefZ_Salmon");
        b.targets.Insert("ChefZ_SockeyeSalmon");
        b.targets.Insert("ChefZ_HorseMackerel");
        b.targets.Insert("ChefZ_JapaneseSardine");
        b.targets.Insert("ChefZ_Eel");
        list.Insert(b);
        b = ChefZ_AT_Bait.Make("ChefZ_CrimsonSquidLure", "lure");
        b.targets.Insert("ChefZ_CrimsonSquid");
        b.targets.Insert("ChefZ_NorthernCrimsonSquid");
        b.targets.Insert("ChefZ_Octopus");
        b.targets.Insert("ChefZ_Squid");
        list.Insert(b);
        b = ChefZ_AT_Bait.Make("ChefZ_NeonPrawnJig", "lure");
        b.targets.Insert("ChefZ_Squid");
        b.targets.Insert("ChefZ_SpeckledSquid");
        b.targets.Insert("ChefZ_Octopus");
        b.targets.Insert("ChefZ_SeaBass");
        list.Insert(b);
        b = ChefZ_AT_Bait.Make("ChefZ_SilverSardineLure", "lure");
        b.targets.Insert("Mackerel");
        b.targets.Insert("ChefZ_Bonito");
        b.targets.Insert("ChefZ_HorseMackerel");
        b.targets.Insert("ChefZ_SpeckledSquid");
        b.targets.Insert("ChefZ_NorthernCrimsonSquid");
        list.Insert(b);
    }
}
