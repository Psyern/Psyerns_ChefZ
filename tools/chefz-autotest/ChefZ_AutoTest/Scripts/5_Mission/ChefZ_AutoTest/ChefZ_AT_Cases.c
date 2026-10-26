//==============================================================================
// GENERIERT von tools/chefz-autotest/make_cases.py - nicht von Hand aendern.
// 143 Faelle (Kochrezepte und Transformationen).
//==============================================================================

class ChefZ_AT_Cases
{
    static void Fill(notnull array<ref ChefZ_AT_Case> list)
    {
        ChefZ_AT_Case c;

        c = ChefZ_AT_Case.Make("COOK", "REC_ChefZ_Bread", "Pot", "BAKING", "");
        c.AddInput("ChefZ_Dough", 1, "", "");
        c.AddOutput("ChefZ_Bread");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "REC_ChefZ_Flatbread", "FryingPan", "BAKING", "");
        c.AddInput("ChefZ_Dough", 1, "", "");
        c.AddOutput("ChefZ_Flatbread");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_HunterStew", "Cauldron", "BOILING", "");
        c.SetLiquid("Water", 700.0);
        c.AddInput("ChefZ_DicedMeat", 2, "", "");
        c.AddInput("Potato", 2, "", "");
        c.AddInput("AgaricusMushroom", 2, "", "");
        c.AddOutput("ChefZ_HunterStewBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_HunterStewBroth", "Cauldron", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.AddInput("ChefZ_BoneBroth", 1, "", "");
        c.AddInput("ChefZ_DicedMeat", 2, "", "");
        c.AddInput("Potato", 2, "", "");
        c.AddInput("AgaricusMushroom", 2, "", "");
        c.AddOutput("ChefZ_HunterStewBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_HunterStewGroup", "Cauldron", "BOILING", "");
        c.SetLiquid("Water", 1100.0);
        c.AddInput("ChefZ_DicedMeat", 4, "", "");
        c.AddInput("Potato", 4, "", "");
        c.AddInput("Tomato", 2, "", "");
        c.AddOutput("ChefZ_HunterStewBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_FishermanStew", "Pot", "BOILING", "");
        c.SetLiquid("Water", 700.0);
        c.AddInput("CarpFilletMeat", 2, "", "");
        c.AddInput("Potato", 2, "", "");
        c.AddInput("ChefZ_Carrot", 1, "", "");
        c.AddOutput("ChefZ_FishermanStewBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_FishermanStewBroth", "Cauldron", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.AddInput("ChefZ_BoneBroth", 1, "", "");
        c.AddInput("CarpFilletMeat", 2, "", "");
        c.AddInput("Potato", 2, "", "");
        c.AddInput("ChefZ_Carrot", 1, "", "");
        c.AddOutput("ChefZ_FishermanStewBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_FishermanStewGroup", "Cauldron", "BOILING", "");
        c.SetLiquid("Water", 1100.0);
        c.AddInput("CarpFilletMeat", 4, "", "");
        c.AddInput("Potato", 4, "", "");
        c.AddInput("ChefZ_Carrot", 2, "", "");
        c.AddOutput("ChefZ_FishermanStewBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_VegetableSoup", "Pot", "BOILING", "");
        c.SetLiquid("Water", 750.0);
        c.AddInput("ChefZ_Cabbage", 1, "", "");
        c.AddInput("Potato", 3, "", "");
        c.AddOutput("ChefZ_VegetableSoupBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_VegetableSoupGroup", "Cauldron", "BOILING", "");
        c.SetLiquid("Water", 1200.0);
        c.AddInput("ChefZ_Cabbage", 2, "", "");
        c.AddInput("Potato", 6, "", "");
        c.AddOutput("ChefZ_VegetableSoupBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_BoneBrothSoup", "Cauldron", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.AddInput("ChefZ_BoneBroth", 2, "", "");
        c.AddInput("Potato", 3, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddOutput("ChefZ_BoneBrothSoupBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_BoneBrothSoupGroup", "Cauldron", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.AddInput("ChefZ_BoneBroth", 4, "", "");
        c.AddInput("Potato", 6, "", "");
        c.AddInput("ChefZ_DriedParsley", 2, "", "");
        c.AddOutput("ChefZ_BoneBrothSoupBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_ChernarusChili", "Cauldron", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.seconds = 300.0;
        c.AddInput("BearSteakMeat", 2, "", "");
        c.AddInput("BakedBeansCan_Opened", 1, "", "");
        c.AddInput("GreenBellPepper", 1, "", "");
        c.AddInput("ChefZ_PaprikaPowder", 1, "", "");
        c.AddInput("Tomato", 2, "", "");
        c.AddOutput("ChefZ_ChernarusChiliBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_ChernarusChiliGroup", "Cauldron", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.seconds = 300.0;
        c.AddInput("BakedBeansCan_Opened", 2, "", "");
        c.AddInput("BoarSteakMeat", 4, "", "");
        c.AddInput("GreenBellPepper", 2, "", "");
        c.AddInput("ChefZ_PaprikaPowder", 2, "", "");
        c.AddInput("Tomato", 4, "", "");
        c.AddOutput("ChefZ_ChernarusChiliBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_CheeseCurd", "Pot", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.seconds = 300.0;
        c.AddInput("PowderedMilk", 3, "", "");
        c.AddInput("ChefZ_MushroomCulture", 1, "", "");
        c.AddOutput("ChefZ_CheeseCurd");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_TacticalBreakfast", "FryingPan", "BAKING", "");
        c.AddInput("ChefZ_Bread", 1, "", "");
        c.AddInput("TacticalBaconCan_Opened", 1, "", "");
        c.AddInput("ChefZ_Egg", 1, "", "");
        c.AddOutput("ChefZ_TacticalBreakfast");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_ScrambledEggSausage", "FryingPan", "BAKING", "");
        c.AddInput("PowderedMilk", 1, "", "");
        c.AddInput("ChefZ_BoarSausage", 1, "", "");
        c.AddInput("ChefZ_Egg", 2, "", "");
        c.AddOutput("ChefZ_ScrambledEggSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_FarmersBreakfast", "FryingPan", "BAKING", "");
        c.AddInput("Potato", 2, "", "");
        c.AddInput("ChefZ_BoarSausage", 1, "", "");
        c.AddInput("ChefZ_Egg", 1, "", "");
        c.AddInput("ChefZ_Onion", 1, "", "");
        c.AddOutput("ChefZ_FarmersBreakfast");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_CheeseFlatbread", "FryingPan", "BAKING", "");
        c.AddInput("ChefZ_Dough", 1, "", "");
        c.AddInput("ChefZ_Cheese", 1, "", "");
        c.AddOutput("ChefZ_CheeseFlatbread");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_SausageBreadPlate", "FryingPan", "NONE", "");
        c.AddInput("ChefZ_Bread", 1, "", "");
        c.AddInput("ChefZ_Cheese", 1, "", "");
        c.AddInput("ChefZ_BoarSausage", 1, "", "");
        c.AddOutput("ChefZ_SausageBreadPlate");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_MushroomPan", "FryingPan", "BAKING", "");
        c.AddInput("AgaricusMushroom", 3, "", "");
        c.AddInput("ChefZ_Butter", 1, "", "");
        c.AddOutput("ChefZ_MushroomPan");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_PotatoPancakes", "FryingPan", "BAKING", "");
        c.AddInput("ChefZ_Flour", 1, "", "");
        c.AddInput("Lard", 1, "", "");
        c.AddInput("Potato", 2, "", "");
        c.AddInput("ChefZ_Egg", 1, "", "");
        c.AddOutput("ChefZ_PotatoPancakes");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_MeatDumplings", "Pot", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.AddInput("ChefZ_Dough", 1, "", "");
        c.AddInput("ChefZ_DicedMeat", 1, "", "");
        c.AddInput("ChefZ_Onion", 1, "", "");
        c.AddOutput("ChefZ_MeatDumplings");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_MilkRice", "Pot", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.seconds = 240.0;
        c.AddInput("Rice", 1, "", "");
        c.AddInput("PowderedMilk", 1, "", "");
        c.AddInput("ChefZ_Butter", 1, "", "");
        c.AddOutput("ChefZ_MilkRice");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_HoneyBreadPlate", "FryingPan", "NONE", "");
        c.AddInput("ChefZ_Bread", 1, "", "");
        c.AddInput("Honey", 1, "", "");
        c.AddOutput("ChefZ_HoneyBreadPlate");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_PumpkinSoup", "Pot", "BOILING", "");
        c.SetLiquid("Water", 750.0);
        c.AddInput("SlicedPumpkin", 3, "", "");
        c.AddInput("ChefZ_Butter", 1, "", "");
        c.AddOutput("ChefZ_PumpkinSoupBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_SmallFishPan", "FryingPan", "BAKING", "");
        c.AddInput("Lard", 1, "", "");
        c.AddInput("Bitterlings", 4, "", "");
        c.AddInput("ChefZ_Garlic", 1, "", "");
        c.AddOutput("ChefZ_SmallFishPan");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_SurvivorSpaghetti", "Pot", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.seconds = 180.0;
        c.AddInput("ChefZ_DriedPasta", 1, "", "");
        c.AddInput("ChefZ_TomatoSauce", 1, "", "");
        c.AddOutput("ChefZ_SurvivorSpaghetti");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_SausagePasta", "FryingPan", "BAKING", "");
        c.seconds = 200.0;
        c.AddInput("ChefZ_DriedPasta", 1, "", "");
        c.AddInput("Lard", 1, "", "");
        c.AddInput("ChefZ_BoarSausage", 1, "", "");
        c.AddOutput("ChefZ_SausagePasta");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_HunterPasta", "FryingPan", "BAKING", "");
        c.seconds = 240.0;
        c.AddInput("ChefZ_DriedPasta", 1, "", "");
        c.AddInput("BoarSteakMeat", 1, "", "");
        c.AddInput("AgaricusMushroom", 1, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddOutput("ChefZ_HunterPasta");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_CreamMushroomPasta", "FryingPan", "BAKING", "");
        c.seconds = 210.0;
        c.AddInput("ChefZ_DriedPasta", 1, "", "");
        c.AddInput("ChefZ_Cream", 1, "", "");
        c.AddInput("AgaricusMushroom", 2, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddOutput("ChefZ_CreamMushroomPasta");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_MacAndCheese", "Pot", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.seconds = 200.0;
        c.AddInput("ChefZ_DriedPasta", 1, "", "");
        c.AddInput("PowderedMilk", 2, "", "");
        c.AddOutput("ChefZ_MacAndCheese");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_SausagePotatoes", "FryingPan", "BAKING", "");
        c.seconds = 220.0;
        c.AddInput("Lard", 1, "", "");
        c.AddInput("Potato", 1, "", "");
        c.AddInput("ChefZ_BoarSausage", 1, "", "");
        c.AddOutput("ChefZ_SausagePotatoes");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_HunterPlate", "FryingPan", "BAKING", "");
        c.seconds = 260.0;
        c.AddInput("BoarSteakMeat", 1, "", "");
        c.AddInput("Potato", 1, "", "");
        c.AddInput("AgaricusMushroom", 1, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddOutput("ChefZ_HunterPlate");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_BloodSausagePlate", "FryingPan", "BAKING", "");
        c.seconds = 220.0;
        c.AddInput("ChefZ_BoarSausage", 1, "", "");
        c.AddInput("Potato", 1, "", "");
        c.AddInput("Potato", 1, "", "");
        c.AddOutput("ChefZ_BloodSausagePlate");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_FishPotatoPlate", "FryingPan", "BAKING", "");
        c.seconds = 200.0;
        c.AddInput("CarpFilletMeat", 1, "", "");
        c.AddInput("Potato", 1, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddOutput("ChefZ_FishPotatoPlate");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_BeanSausagePlate", "FryingPan", "BAKING", "");
        c.seconds = 180.0;
        c.AddInput("BakedBeansCan_Opened", 1, "", "");
        c.AddInput("ChefZ_BoarSausage", 1, "", "");
        c.AddInput("Potato", 1, "", "");
        c.AddOutput("ChefZ_BeanSausagePlate");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_TomatoSauce", "Pot", "BOILING", "");
        c.SetLiquid("Water", 1000.0);
        c.seconds = 180.0;
        c.AddInput("ChefZ_Salt", 1, "", "");
        c.AddInput("Tomato", 3, "", "");
        c.AddOutput("ChefZ_TomatoSauce");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_CreamSauce", "Pot", "BAKING", "");
        c.seconds = 120.0;
        c.AddInput("ChefZ_Cream", 1, "", "");
        c.AddInput("ChefZ_Salt", 1, "", "");
        c.AddInput("ChefZ_Butter", 1, "", "");
        c.AddOutput("ChefZ_CreamSauce");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_MushroomCreamSauce", "Cauldron", "BAKING", "");
        c.seconds = 150.0;
        c.AddInput("ChefZ_Cream", 1, "", "");
        c.AddInput("ChefZ_Salt", 1, "", "");
        c.AddInput("AgaricusMushroom", 3, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddOutput("ChefZ_MushroomCreamSauce");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_BoneBroth", "Pot", "BOILING", "");
        c.SetLiquid("Water", 700.0);
        c.seconds = 420.0;
        c.AddInput("Bone", 2, "", "");
        c.AddInput("Potato", 2, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddOutput("ChefZ_BoneBroth");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_CrispyFriedCalamari", "FryingPan", "BAKING", "");
        c.seconds = 180.0;
        c.AddInput("ChefZ_CrimsonSquid", 2, "", "");
        c.AddInput("ChefZ_Flour", 1, "", "");
        c.AddInput("Lard", 1, "", "");
        c.AddOutput("ChefZ_CrispyFriedCalamari");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_ChefZ_GrilledStuffedCalamari", "FryingPan", "BAKING", "");
        c.seconds = 240.0;
        c.AddInput("ChefZ_CrimsonSquid", 2, "", "");
        c.AddInput("Rice", 1, "", "");
        c.AddInput("Tomato", 1, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddOutput("ChefZ_GrilledStuffedCalamari");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_CookSausage", "FryingPan", "BAKING", "");
        c.AddInput("ChefZ_RawSausage", 1, "", "");
        c.AddOutput("ChefZ_CookedSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_CookPorkSausage", "FryingPan", "BAKING", "");
        c.AddInput("ChefZ_RawPorkSausage", 1, "", "");
        c.AddOutput("ChefZ_PorkSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_CookVenisonSausage", "FryingPan", "BAKING", "");
        c.AddInput("ChefZ_RawVenisonSausage", 1, "", "");
        c.AddOutput("ChefZ_VenisonSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_CookBoarSausage", "FryingPan", "BAKING", "");
        c.AddInput("ChefZ_RawBoarSausage", 1, "", "");
        c.AddOutput("ChefZ_BoarSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_CookHunterSausage", "FryingPan", "BAKING", "");
        c.AddInput("ChefZ_RawHunterSausage", 1, "", "");
        c.AddOutput("ChefZ_HunterSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("COOK", "RCP_CookSpicySausage", "FryingPan", "BAKING", "");
        c.AddInput("ChefZ_RawSpicySausage", 1, "", "");
        c.AddOutput("ChefZ_SpicySausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FlourWaterToDough", "", "", "PROCESS_KNEAD");
        c.SetLiquid("Water", 1000.0);
        c.AddInput("ChefZ_Flour", 1, "", "");
        c.AddInput("WaterBottle", 1, "", "");
        c.AddOutput("ChefZ_Dough");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_DoughToRawPasta", "", "", "PROCESS_ROLL");
        c.AddInput("ChefZ_Dough", 1, "", "");
        c.AddOutput("ChefZ_RawPasta");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawPastaToDriedPasta", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 1800.0;
        c.AddInput("ChefZ_RawPasta", 1, "", "");
        c.AddOutput("ChefZ_DriedPasta");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_BowlFromBark", "", "", "PROCESS_CARVE_BOWL_BARK");
        c.AddInput("Bark_Birch", 1, "", "");
        c.AddOutput("ChefZ_EmptyBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_BoxFromPaper", "", "", "PROCESS_FOLD_BOX");
        c.AddInput("Paper", 1, "", "");
        c.AddInput("Paper", 1, "", "");
        c.AddOutput("ChefZ_EmptyBox");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_CansFromMetalSheet", "", "", "PROCESS_CUT_CANS");
        c.AddInput("MetalPlate", 1, "", "");
        c.AddOutput("ChefZ_EmptyCan");
        c.AddOutput("ChefZ_EmptyCan");
        c.AddOutput("ChefZ_EmptyCan");
        c.AddOutput("ChefZ_EmptyCan");
        c.AddOutput("ChefZ_EmptyCan");
        c.AddOutput("ChefZ_EmptyCan");
        c.AddOutput("ChefZ_EmptyCan");
        c.AddOutput("ChefZ_EmptyCan");
        c.AddOutput("ChefZ_EmptyCan");
        c.AddOutput("ChefZ_EmptyCan");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_CarveWoodenPlate", "", "", "PROCESS_CARVE_PLATE");
        c.AddInput("Firewood", 1, "", "");
        c.AddOutput("ChefZ_EmptyPlate");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_CarveWoodenBowl", "", "", "PROCESS_CARVE_BOWL");
        c.AddInput("Firewood", 1, "", "");
        c.AddOutput("ChefZ_EmptyBowl");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_BuildBeehiveKit", "", "", "PROCESS_BUILD_HIVE_KIT");
        c.AddInput("WoodenPlank", 1, "", "");
        c.AddInput("Nail", 1, "", "");
        c.AddOutput("ChefZ_BeehiveKit");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RaiseBeehive", "", "", "PROCESS_RAISE_HIVE");
        c.AddInput("ChefZ_BeehiveKit", 1, "", "");
        c.AddOutput("ChefZ_Beehive");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_BuildHoneycombFrame", "", "", "PROCESS_BUILD_FRAME");
        c.AddInput("WoodenPlank", 1, "", "");
        c.AddInput("Nail", 1, "", "");
        c.AddOutput("ChefZ_HoneycombFrameEmpty");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_BuildUncappingFork", "", "", "PROCESS_BUILD_UNCAPPING_FORK");
        c.AddInput("WoodenPlank", 1, "", "");
        c.AddInput("Nail", 1, "", "");
        c.AddOutput("ChefZ_UncappingFork");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_BuildBeeSmoker", "", "", "PROCESS_BUILD_BEE_SMOKER");
        c.AddInput("TunaCan_Opened", 1, "", "");
        c.AddOutput("ChefZ_BeeSmoker");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_UncapHoneycombFrame", "", "", "PROCESS_UNCAP_COMB");
        c.AddInput("ChefZ_HoneycombFrameFull", 1, "", "");
        c.AddOutput("ChefZ_HoneycombFrameUncapped");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_ExtendBeehive", "", "", "PROCESS_EXTEND_HIVE");
        c.AddInput("ChefZ_BeehiveKit", 1, "", "");
        c.AddInput("ChefZ_BeehiveKit", 1, "", "");
        c.AddOutput("ChefZ_BeehiveDouble");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FillBeeSmoker", "", "", "PROCESS_FILL_SMOKER");
        c.AddInput("ChefZ_BeeSmoker", 1, "", "");
        c.SetLastQtyPct(0.25);
        c.AddInput("Bark_Birch", 1, "", "");
        c.AddOutput("ChefZ_BeeSmoker");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletPerch", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Perch", 1, "RAW", "");
        c.AddOutput("ChefZ_PerchFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletMuskellunge", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Muskellunge", 1, "RAW", "");
        c.AddOutput("ChefZ_MuskellungeFillet");
        c.AddOutput("ChefZ_MuskellungeFillet");
        c.AddOutput("ChefZ_MuskellungeFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletSnakehead", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Snakehead", 1, "RAW", "");
        c.AddOutput("ChefZ_SnakeheadFillet");
        c.AddOutput("ChefZ_SnakeheadFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletRainbowTrout", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_RainbowTrout", 1, "RAW", "");
        c.AddOutput("ChefZ_RainbowTroutFillet");
        c.AddOutput("ChefZ_RainbowTroutFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletChar", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Char", 1, "RAW", "");
        c.AddOutput("ChefZ_CharFillet");
        c.AddOutput("ChefZ_CharFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletSalmon", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Salmon", 1, "RAW", "");
        c.AddOutput("ChefZ_SalmonFillet");
        c.AddOutput("ChefZ_SalmonFillet");
        c.AddOutput("ChefZ_SalmonFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletSockeyeSalmon", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_SockeyeSalmon", 1, "RAW", "");
        c.AddOutput("ChefZ_SockeyeSalmonFillet");
        c.AddOutput("ChefZ_SockeyeSalmonFillet");
        c.AddOutput("ChefZ_SockeyeSalmonFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletEel", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Eel", 1, "RAW", "");
        c.AddOutput("ChefZ_EelFillet");
        c.AddOutput("ChefZ_EelFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletPlaice", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Plaice", 1, "RAW", "");
        c.AddOutput("ChefZ_PlaiceFillet");
        c.AddOutput("ChefZ_PlaiceFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletGiltheadBream", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_GiltheadBream", 1, "RAW", "");
        c.AddOutput("ChefZ_GiltheadBreamFillet");
        c.AddOutput("ChefZ_GiltheadBreamFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletHalibut", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Halibut", 1, "RAW", "");
        c.AddOutput("ChefZ_HalibutFillet");
        c.AddOutput("ChefZ_HalibutFillet");
        c.AddOutput("ChefZ_HalibutFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletCod", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Cod", 1, "RAW", "");
        c.AddOutput("ChefZ_CodFillet");
        c.AddOutput("ChefZ_CodFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletSole", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Sole", 1, "RAW", "");
        c.AddOutput("ChefZ_SoleFillet");
        c.AddOutput("ChefZ_SoleFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletSeaBass", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_SeaBass", 1, "RAW", "");
        c.AddOutput("ChefZ_SeaBassFillet");
        c.AddOutput("ChefZ_SeaBassFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletRedMullet", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_RedMullet", 1, "RAW", "");
        c.AddOutput("ChefZ_RedMulletFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletConger", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Conger", 1, "RAW", "");
        c.AddOutput("ChefZ_CongerFillet");
        c.AddOutput("ChefZ_CongerFillet");
        c.AddOutput("ChefZ_CongerFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletSeaEel", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_SeaEel", 1, "RAW", "");
        c.AddOutput("ChefZ_EelFillet");
        c.AddOutput("ChefZ_EelFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletHorseMackerel", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_HorseMackerel", 1, "RAW", "");
        c.AddOutput("ChefZ_HorseMackerelFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletJapaneseSardine", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_JapaneseSardine", 1, "RAW", "");
        c.AddOutput("ChefZ_JapaneseSardineFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletRockfish", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Rockfish", 1, "RAW", "");
        c.AddOutput("ChefZ_RockfishFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletBluefinTuna", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_BluefinTuna", 1, "RAW", "");
        c.AddOutput("ChefZ_BluefinTunaFillet");
        c.AddOutput("ChefZ_BluefinTunaFillet");
        c.AddOutput("ChefZ_BluefinTunaFillet");
        c.AddOutput("ChefZ_BluefinTunaFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletYellowfinTuna", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_YellowfinTuna", 1, "RAW", "");
        c.AddOutput("ChefZ_YellowfinTunaFillet");
        c.AddOutput("ChefZ_YellowfinTunaFillet");
        c.AddOutput("ChefZ_YellowfinTunaFillet");
        c.AddOutput("ChefZ_YellowfinTunaFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletRedSnapper", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_RedSnapper", 1, "RAW", "");
        c.AddOutput("ChefZ_RedSnapperFillet");
        c.AddOutput("ChefZ_RedSnapperFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletGiantGrouper", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_GiantGrouper", 1, "RAW", "");
        c.AddOutput("ChefZ_GiantGrouperFillet");
        c.AddOutput("ChefZ_GiantGrouperFillet");
        c.AddOutput("ChefZ_GiantGrouperFillet");
        c.AddOutput("ChefZ_GiantGrouperFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletMahiMahi", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_MahiMahi", 1, "RAW", "");
        c.AddOutput("ChefZ_MahiMahiFillet");
        c.AddOutput("ChefZ_MahiMahiFillet");
        c.AddOutput("ChefZ_MahiMahiFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletGiantTrevally", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_GiantTrevally", 1, "RAW", "");
        c.AddOutput("ChefZ_GiantTrevallyFillet");
        c.AddOutput("ChefZ_GiantTrevallyFillet");
        c.AddOutput("ChefZ_GiantTrevallyFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletBlueMarlin", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_BlueMarlin", 1, "RAW", "");
        c.AddOutput("ChefZ_BlueMarlinFillet");
        c.AddOutput("ChefZ_BlueMarlinFillet");
        c.AddOutput("ChefZ_BlueMarlinFillet");
        c.AddOutput("ChefZ_BlueMarlinFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletSailfish", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Sailfish", 1, "RAW", "");
        c.AddOutput("ChefZ_SailfishFillet");
        c.AddOutput("ChefZ_SailfishFillet");
        c.AddOutput("ChefZ_SailfishFillet");
        c.AddOutput("ChefZ_SailfishFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletBonito", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Bonito", 1, "RAW", "");
        c.AddOutput("ChefZ_BonitoFillet");
        c.AddOutput("ChefZ_BonitoFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletYellowtail", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Yellowtail", 1, "RAW", "");
        c.AddOutput("ChefZ_YellowtailFillet");
        c.AddOutput("ChefZ_YellowtailFillet");
        c.AddOutput("ChefZ_YellowtailFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FilletTarpon", "", "", "PROCESS_FILLET_FISH");
        c.AddInput("ChefZ_Tarpon", 1, "RAW", "");
        c.AddOutput("ChefZ_TarponFillet");
        c.AddOutput("ChefZ_TarponFillet");
        c.AddOutput("ChefZ_TarponFillet");
        c.AddOutput("ChefZ_TarponFillet");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_CaninaBerriesToDried", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 420.0;
        c.AddInput("CaninaBerry", 2, "", "");
        c.AddOutput("ChefZ_DriedBerries");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_SambucusBerriesToDried", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 420.0;
        c.AddInput("SambucusBerry", 2, "", "");
        c.AddOutput("ChefZ_DriedBerries");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_CutBeefLeg", "", "", "PROCESS_CUT_MEAT");
        c.AddInput("ChefZ_BeefLeg", 1, "", "");
        c.AddOutput("CowSteakMeat");
        c.AddOutput("CowSteakMeat");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_CutPorkLeg", "", "", "PROCESS_CUT_MEAT");
        c.AddInput("ChefZ_PorkLeg", 1, "", "");
        c.AddOutput("PigSteakMeat");
        c.AddOutput("PigSteakMeat");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_CutVenisonLeg", "", "", "PROCESS_CUT_MEAT");
        c.AddInput("ChefZ_VenisonLeg", 1, "", "");
        c.AddOutput("DeerSteakMeat");
        c.AddOutput("DeerSteakMeat");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_DicedMeat", "", "", "PROCESS_CUT_MEAT");
        c.AddInput("BearSteakMeat", 1, "RAW", "Raw");
        c.AddOutput("ChefZ_DicedMeat");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_MeatToMinced", "ChefZ_MeatGrinder", "", "PROCESS_GRIND_MEAT");
        c.AddInput("CowSteakMeat", 1, "RAW", "Raw");
        c.AddOutput("ChefZ_MincedMeat");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_PorkToMinced", "ChefZ_MeatGrinder", "", "PROCESS_GRIND_MEAT");
        c.AddInput("PigSteakMeat", 1, "RAW", "Raw");
        c.AddOutput("ChefZ_MincedPork");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_VenisonToMinced", "ChefZ_MeatGrinder", "", "PROCESS_GRIND_MEAT");
        c.AddInput("DeerSteakMeat", 1, "RAW", "Raw");
        c.AddOutput("ChefZ_MincedVenison");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_BoarToMinced", "ChefZ_MeatGrinder", "", "PROCESS_GRIND_MEAT");
        c.AddInput("BoarSteakMeat", 1, "RAW", "Raw");
        c.AddOutput("ChefZ_MincedBoar");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_ChickenToMinced", "ChefZ_MeatGrinder", "", "PROCESS_GRIND_MEAT");
        c.AddInput("ChickenBreastMeat", 1, "RAW", "Raw");
        c.AddOutput("ChefZ_MincedChicken");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_BearToMinced", "ChefZ_MeatGrinder", "", "PROCESS_GRIND_MEAT");
        c.AddInput("BearSteakMeat", 1, "RAW", "Raw");
        c.AddOutput("ChefZ_MincedBear");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawSausage", "ChefZ_MeatGrinder", "", "PROCESS_STUFF_SAUSAGE");
        c.AddInput("Guts", 1, "", "");
        c.AddInput("ChefZ_DicedMeat", 1, "", "");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddOutput("ChefZ_RawSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawPorkSausage", "ChefZ_MeatGrinder", "", "PROCESS_STUFF_SAUSAGE");
        c.AddInput("Guts", 1, "", "");
        c.AddInput("ChefZ_MincedPork", 1, "", "");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddOutput("ChefZ_RawPorkSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawVenisonSausage", "ChefZ_MeatGrinder", "", "PROCESS_STUFF_SAUSAGE");
        c.AddInput("Guts", 1, "", "");
        c.AddInput("ChefZ_MincedVenison", 1, "", "");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddOutput("ChefZ_RawVenisonSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawBoarSausage", "ChefZ_MeatGrinder", "", "PROCESS_STUFF_SAUSAGE");
        c.AddInput("Guts", 1, "", "");
        c.AddInput("ChefZ_MincedBoar", 1, "", "");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddOutput("ChefZ_RawBoarSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawHunterSausage", "ChefZ_MeatGrinder", "", "PROCESS_STUFF_SAUSAGE");
        c.AddInput("Guts", 1, "", "");
        c.AddInput("BoarSteakMeat", 2, "RAW", "Raw");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddOutput("ChefZ_RawHunterSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawSpicySausage", "ChefZ_MeatGrinder", "", "PROCESS_STUFF_SAUSAGE");
        c.AddInput("Guts", 1, "", "");
        c.AddInput("ChefZ_DicedMeat", 1, "", "");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddOutput("ChefZ_RawSpicySausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_SaltedMeatToDried", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 3600.0;
        c.AddInput("ChefZ_SaltedMeat", 1, "", "");
        c.AddOutput("ChefZ_DriedMeat");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_SaltedFishToDried", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 2700.0;
        c.AddInput("ChefZ_SaltedFish", 1, "", "");
        c.AddOutput("ChefZ_DriedFish");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawSausageToDry", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 5400.0;
        c.AddInput("ChefZ_BoarSausage", 1, "RAW", "");
        c.AddOutput("ChefZ_DrySausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_SaltMeat", "", "", "PROCESS_SALT_CURE");
        c.AddInput("BearSteakMeat", 1, "RAW", "");
        c.AddInput("ChefZ_Salt", 1, "", "");
        c.AddOutput("ChefZ_SaltedMeat");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_SaltFish", "", "", "PROCESS_SALT_CURE");
        c.AddInput("CarpFilletMeat", 1, "RAW", "");
        c.AddInput("ChefZ_Salt", 1, "", "");
        c.AddOutput("ChefZ_SaltedFish");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_SaltedMeatToSmoked", "ChefZ_Smoker", "", "PROCESS_SMOKE");
        c.seconds = 300.0;
        c.AddInput("ChefZ_SaltedMeat", 1, "", "");
        c.AddOutput("ChefZ_SmokedMeat");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawMeatToSmoked", "ChefZ_Smoker", "", "PROCESS_SMOKE");
        c.seconds = 300.0;
        c.AddInput("BearSteakMeat", 1, "RAW", "");
        c.AddOutput("ChefZ_SmokedMeat");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_FishToSmoked", "ChefZ_Smoker", "", "PROCESS_SMOKE");
        c.seconds = 300.0;
        c.AddInput("CarpFilletMeat", 1, "RAW", "");
        c.AddOutput("ChefZ_SmokedFish");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawSausageToSmoked", "ChefZ_Smoker", "", "PROCESS_SMOKE");
        c.seconds = 300.0;
        c.AddInput("ChefZ_BoarSausage", 1, "RAW", "");
        c.AddOutput("ChefZ_SmokedSausage");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_WheatToFlour", "ChefZ_GrainMill", "", "PROCESS_MILL");
        c.AddInput("ChefZ_Wheat", 1, "", "");
        c.AddOutput("ChefZ_Flour");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_CornToFlour", "ChefZ_GrainMill", "", "PROCESS_MILL");
        c.AddInput("ChefZ_Corn", 1, "", "");
        c.AddOutput("ChefZ_Flour");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_AssemblePastaMachine", "", "", "PROCESS_ASSEMBLE");
        c.AddInput("MetalPlate", 1, "", "");
        c.AddOutput("ChefZ_PastaMachine");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_MilkToCream", "ChefZ_ButterChurn", "", "PROCESS_SEPARATE_CREAM");
        c.seconds = 60.0;
        c.AddInput("PowderedMilk", 2, "", "");
        c.AddOutput("ChefZ_Cream");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_CreamToButter", "ChefZ_ButterChurn", "", "PROCESS_CHURN_BUTTER");
        c.seconds = 60.0;
        c.AddInput("ChefZ_Cream", 2, "", "");
        c.AddOutput("ChefZ_Butter");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_CurdToCheese", "ChefZ_CheesePress", "", "PROCESS_PRESS_CHEESE");
        c.AddInput("ChefZ_CheeseCurd", 1, "", "");
        c.AddOutput("ChefZ_Cheese");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_ParsleyToDried", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 480.0;
        c.AddInput("ChefZ_Parsley", 1, "", "");
        c.AddOutput("ChefZ_DriedParsley");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_ThymeToDried", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 480.0;
        c.AddInput("ChefZ_Thyme", 1, "", "");
        c.AddOutput("ChefZ_DriedThyme");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RosemaryToDried", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 600.0;
        c.AddInput("ChefZ_Rosemary", 1, "", "");
        c.AddOutput("ChefZ_DriedRosemary");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_WildGarlicToDried", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 480.0;
        c.AddInput("ChefZ_WildGarlic", 1, "", "");
        c.AddOutput("ChefZ_DriedWildGarlic");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_PaprikaToDried", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 900.0;
        c.AddInput("GreenBellPepper", 1, "", "");
        c.AddOutput("ChefZ_DriedPaprika");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_PepperBerriesToDried", "ChefZ_DryingRack", "", "PROCESS_DRY");
        c.seconds = 900.0;
        c.AddInput("ChefZ_PepperBerries", 1, "", "");
        c.AddOutput("ChefZ_DriedPeppercorns");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_PeppercornsToBlackPepper", "ChefZ_Mortar", "", "PROCESS_GRIND_SPICE");
        c.seconds = 20.0;
        c.AddInput("ChefZ_DriedPeppercorns", 1, "", "");
        c.AddOutput("ChefZ_BlackPepper");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_DriedPaprikaToPowder", "ChefZ_Mortar", "", "PROCESS_GRIND_SPICE");
        c.seconds = 20.0;
        c.AddInput("ChefZ_DriedPaprika", 1, "", "");
        c.AddOutput("ChefZ_PaprikaPowder");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RottenMushroomToCulture", "ChefZ_Mortar", "", "PROCESS_GRIND_SPICE");
        c.seconds = 20.0;
        c.AddInput("AgaricusMushroom", 1, "", "Rotten");
        c.AddOutput("ChefZ_MushroomCulture");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_HerbMix", "ChefZ_Mortar", "", "PROCESS_GRIND_HERB");
        c.seconds = 25.0;
        c.AddInput("ChefZ_DriedThyme", 1, "", "");
        c.AddInput("ChefZ_DriedParsley", 1, "", "");
        c.AddInput("ChefZ_DriedRosemary", 1, "", "");
        c.AddOutput("ChefZ_HerbMix");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_HunterSeasoning", "ChefZ_Mortar", "", "PROCESS_GRIND_SPICE");
        c.seconds = 35.0;
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddInput("ChefZ_PaprikaPowder", 1, "", "");
        c.AddInput("ChefZ_BlackPepper", 1, "", "");
        c.AddInput("ChefZ_DriedThyme", 1, "", "");
        c.AddInput("ChefZ_DriedWildGarlic", 1, "", "");
        c.AddOutput("ChefZ_HunterSeasoning");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_SpinHoney", "ChefZ_HoneyExtractor", "", "PROCESS_SPIN_HONEY");
        c.AddInput("ChefZ_HoneycombFrameUncapped", 1, "", "");
        c.AddInput("ChefZ_EmptyJar", 1, "", "");
        c.AddOutput("Honey");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_SaltwaterToRawSalt", "ChefZ_FryingPan", "", "PROCESS_BOIL_BRINE");
        c.SetLiquid("SaltWater", 1000.0);
        c.AddInput("WaterBottle", 1, "", "");
        c.AddOutput("ChefZ_RawSalt");
        list.Insert(c);

        c = ChefZ_AT_Case.Make("PROC", "TR_RawSaltToSalt", "ChefZ_FryingPan", "", "PROCESS_DRY_SALT");
        c.AddInput("ChefZ_RawSalt", 1, "", "");
        c.AddOutput("ChefZ_Salt");
        list.Insert(c);
    }
}
