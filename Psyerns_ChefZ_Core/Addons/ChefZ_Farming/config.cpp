// ChefZ_Farming - Weizenanbau und Ernte (Slice "grain").
//
// Quelle: Production Map §7 (Weizen-Produktionskette), §69/§73 (Klassenliste),
// DME-Plan §53 (Namenskonvention).
//
// PBO-Praefix: $PREFIX$ enthaelt "ChefZ_Farming". Die Wurzel jedes
// Laufzeitpfades ist dieses Praefix (Entwurf 02 §4.1, B4).
//
// ---------------------------------------------------------------------------
// KEIN NEUES CORE-SYSTEM
// ---------------------------------------------------------------------------
// Der Anbau laeuft vollstaendig ueber Vanillas Gartenkette:
//
//   GardenBase.c:386/425   liest "CfgVehicles <samen> Horticulture PlantType"
//   PlantBase.c:63-65      liest "CfgVehicles <pflanze> Horticulture
//                          GrowthStagesCount / CropsCount / CropsType"
//
// Es ist deshalb keine Zeile ChefZ-Code noetig, um Weizen anzupflanzen und zu
// ernten - und kein Core-System, das es dafuer nicht schon gibt.
//
// ---------------------------------------------------------------------------
// ChefZ_GrainFoodBase - warum die Nahrungsdaten hier stehen
// ---------------------------------------------------------------------------
// Diese Datei fuehrt die EINE gemeinsame Nahrungsbasis der ganzen
// Getreidekette. Weizen, Mehl, Teige, Nudeln und Brot erben von ihr:
//
//   class Nutrition   erfuellt 01 V7 - PlayerStomach.InitData registriert nur
//                     Klassen mit Nutrition ODER Food und scope != 0. Fehlt
//                     der Knoten, verschwindet der Bissen STILL.
//   class Food        erfuellt 01 V4 - ohne FoodStageTransitions faellt
//                     FoodStage.GetNextFoodStageType auf BURNED zurueck
//                     (FoodStage.c:472), das Item verbrennt beim ersten
//                     Garstufenwechsel.
//
// Die stueckgenauen Naehrwerte stehen NICHT hier, sondern im Nutrition-Delta
// des Slices (_deltas/grain.json, Entwurf 13 §4). Der Config-Knoten ist die
// Anmeldung an Vanillas Magen, die Registry ist die Quelle der Zahlen. Ein
// Zahlensatz an zwei Orten waere ein Zahlensatz, der auseinanderlaeuft.
//
// FoodStages fuehrt bewusst kein Kind "Raw": den Rohzustand einer
// ChefZ-Getreideware beschreibt die ChefZ-Nutrition, nicht Vanillas
// Stufentabelle. Der Uebergang Raw -> Baked steht vollstaendig in
// FoodStageTransitions, und genau den verlangt 01 V4.
//
// ---------------------------------------------------------------------------
// 3D
// ---------------------------------------------------------------------------
// Alle Klassen tragen ein VANILLA-PROXY-MODELL. Der Bedarf an eigener
// Geometrie ist im Slice-Bericht als Asset-Bedarf gemeldet; auf ein Modell
// wartet hier nichts.

class CfgPatches
{
    class ChefZ_Farming
    {
        units[] =
        {
            "ChefZ_GrainFoodBase", "ChefZ_WheatPlant", "ChefZ_WheatSeeds", "ChefZ_Wheat",
            // ### SLICE produce ###
            "ChefZ_VegetableFood_Base", "ChefZ_VegetablePlant_Base", "ChefZ_VegetableSeeds_Base",
            "ChefZ_OnionPlant", "ChefZ_GarlicPlant", "ChefZ_CarrotPlant", "ChefZ_CabbagePlant",
            "ChefZ_OnionSeeds", "ChefZ_GarlicSeeds", "ChefZ_CarrotSeeds", "ChefZ_CabbageSeeds",
            "ChefZ_Onion", "ChefZ_Garlic", "ChefZ_Carrot", "ChefZ_Cabbage",
            // ### SLICE herbs ###
            "ChefZ_HerbPlantBase", "ChefZ_HerbSeedsBase", "ChefZ_FreshHerbBase",
            "ChefZ_ParsleyPlant", "ChefZ_DillPlant", "ChefZ_ThymePlant",
            "ChefZ_RosemaryPlant", "ChefZ_WildGarlicPlant",
            "ChefZ_PepperPlant",
            "ChefZ_ParsleySeeds", "ChefZ_DillSeeds", "ChefZ_ThymeSeeds",
            "ChefZ_RosemarySeeds", "ChefZ_WildGarlicSeeds",
            "ChefZ_PeppercornSeeds",
            "ChefZ_Parsley", "ChefZ_Dill", "ChefZ_Thyme", "ChefZ_Rosemary",
            "ChefZ_WildGarlic", "ChefZ_PepperBerries",
            // ### SLICE apiary ###
            "ChefZ_Beehive", "ChefZ_BeehiveKit",
            "ChefZ_HoneycombFrame_Base",
            "ChefZ_HoneycombFrameEmpty", "ChefZ_HoneycombFrameSealed",
            "ChefZ_HoneycombFrameFull", "ChefZ_HoneycombFrameUncapped",
            "ChefZ_UncappingFork", "ChefZ_BeeSmoker"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // ChefZ_Core:          Skriptbasis ChefZ_Edible_Base und - seit dem
        //                      Slice apiary - ChefZ_ProcessingStation_Base.
        // DZ_Gear_Cultivation: SeedBase, PlantBase und ihre Proxy-Modelle.
        // DZ_Gear_Food:        Proxy-Modell des Korns und, fuer den Slice
        //                      apiary, food_can_open.p3d an der Imkerpfeife.
        // DZ_Gear_Camping:     wooden_case.p3d - Proxy von Bienenstock und
        //                      Bausatz (### SLICE apiary ###).
        // DZ_Gear_Tools:       Meat_Tenderizer.p3d - Proxy der
        //                      Entdeckelungsgabel (### SLICE apiary ###).
        // DZ_Gear_Consumables: birch_bark.p3d - Proxy der vier Raehmchen
        //                      (### SLICE apiary ###).
        //
        // KEIN ChefZ_Processing: dieses Modul steht in dessen requiredAddons.
        // Die Gegenrichtung waere ein Zyklus. Der Slice apiary fuehrt deshalb
        // seine drei Werkzeuggruppen selbst (CfgChefZTools weiter unten),
        // statt METALWORK_TOOL aus ChefZ_Processing zu benutzen.
        requiredAddons[] = {"DZ_Data", "DZ_Gear_Cultivation", "DZ_Gear_Food", "DZ_Gear_Camping", "DZ_Gear_Tools", "DZ_Gear_Consumables", "ChefZ_Core"};
    };
};

class CfgMods
{
    class ChefZ_Farming
    {
        dir = "ChefZ_Farming";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Farming";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        // Nur World: dieses Modul bringt ausschliesslich Entitaetsklassen mit.
        dependencies[] = {"World"};

        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Farming/Scripts/4_World"
                };
            };
        };
    };
};

class CfgVehicles
{
    // Vorwaertsdeklarationen der Vanilla-Basen. Sie definieren nichts, sie
    // machen die Elternklasse nur aufloesbar.
    class Edible_Base;
    class SeedBase;
    class PlantBase;
    // ### SLICE apiary ### Configbasis von Bienenstock, Bausatz, Raehmchen,
    // Gabel und Imkerpfeife. Vorwaertsdeklaration, kein Body - sie definiert
    // nichts, sie macht die Vanilla-Basis nur aufloesbar.
    class Inventory_Base;

    //--------------------------------------------------------------------------
    // Die gemeinsame Nahrungsbasis der Getreidekette. scope = 0: eine BASIS,
    // kein Item - sie erscheint in keinem Loot und in keinem Trader.
    //--------------------------------------------------------------------------
    class ChefZ_GrainFoodBase : Edible_Base
    {
        scope = 0;

        class Nutrition
        {
            fullnessIndex = 20;
            energy = 200;
            water = 10;
            nutritionalIndex = 20;
            toxicity = 0;
            digestibility = 0;
        };

        class Food
        {
            class FoodStages
            {
                class Baked
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {25.0, 300.0, 10.0, 25.0, 0.0, 0.0, 0.0};
                    cooking_properties[] = {100.0, 40.0, 200.0};
                };
                class Burned
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {5.0, 20.0, 0.0, 0.0, 5.0, 0.0, 0.0};
                    cooking_properties[] = {200.0, 60.0, 250.0};
                };
                class Rotten
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {5.0, 10.0, 0.0, 0.0, 20.0, 0.0, 0.0};
                };
            };

            // 01 V4: ohne diesen Knoten verbrennt jedes Stueck Teig beim
            // ersten Garstufenwechsel. cooking_method = 1 ist BAKING
            // (Cooking.c:1), transition_to = 2 ist FoodStageType.BAKED
            // (FoodStage.c:1).
            class FoodStageTransitions
            {
                class Raw
                {
                    class Baking
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                };
            };
        };
    };

    //--------------------------------------------------------------------------
    // Die Pflanze im Gartenbeet.
    //
    // PROXY: plant_material.p3d. Eigenes Weizenmesh ist gemeldet (U, P1).
    //--------------------------------------------------------------------------
    class ChefZ_WheatPlant : PlantBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_WHEATPLANT";
        descriptionShort = "#STR_CHEFZ_WHEATPLANT_DESC";
        model = "\dz\gear\cultivation\plant_material.p3d";

        class Horticulture
        {
            GrowthStagesCount = 6;
            CropsCount = 4;
            CropsType = "ChefZ_Wheat";
        };
    };

    //--------------------------------------------------------------------------
    // Saatgut. Vanillas ActionPlantSeed haengt an SeedBase, mehr braucht es
    // nicht.
    //
    // PROXY: pepper_seeds.p3d. Eigenes Mesh ist gemeldet (S, P3).
    //--------------------------------------------------------------------------
    class ChefZ_WheatSeeds : SeedBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_WHEATSEEDS";
        descriptionShort = "#STR_CHEFZ_WHEATSEEDS_DESC";
        model = "\dz\gear\cultivation\pepper_seeds.p3d";
        weight = 5;
        itemSize[] = {1, 1};
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 14400;

        class Horticulture
        {
            PlantType = "ChefZ_WheatPlant";
        };
    };

    //--------------------------------------------------------------------------
    // Das geerntete Korn - Eingang der Getreidemuehle (Production Map §7).
    //
    // PROXY: Rice.p3d. Eigenes Mesh ist gemeldet (U, P1).
    //--------------------------------------------------------------------------
    class ChefZ_Wheat : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_WHEAT";
        descriptionShort = "#STR_CHEFZ_WHEAT_DESC";
        model = "\dz\gear\food\Rice.p3d";
        weight = 400;
        itemSize[] = {2, 2};
        stackedUnit = "grams";
        quantityBar = 1;
        varQuantityInit = 1000;
        varQuantityMin = 0;
        varQuantityMax = 1000;
        varQuantityDestroyOnMin = 1;
        canBeSplit = 1;
        lifetime = 21600;
    };

    //==========================================================================
    // ### SLICE produce ### Gemuese: Pflanze, Samen, Ernte
    //
    // Quellen: Production Map §17 (Zwiebel), §18 (Knoblauch), §19 (Karotte),
    // §20 (Kohl). Kartoffel (§13) und Tomate (§14) EXISTIEREN IN VANILLA -
    // sie werden erweitert und nicht nachgebaut: Plant_Potato, Plant_Tomato,
    // PotatoSeed und TomatoSeeds bringen Anbau und Ernte bereits mit, ihre
    // ChefZ-Zutatenbindung steht in
    // ChefZ_Ingredients/Config/Ingredients/VanillaProduce.json (05 §2,
    // zweiter Deklarationsweg fuer FREMDE Klassen).
    //
    // Der Anbau braucht keine Zeile ChefZ-Code:
    //   GardenBase.c:449-452   liest "CfgVehicles <samen> Horticulture PlantType"
    //   PlantBase.c:63-65      liest "CfgVehicles <pflanze> Horticulture
    //                          GrowthStagesCount / CropsCount / CropsType"
    //
    // class Food MIT FoodStages UND FoodStageTransitions - der urspruengliche
    // Satz "rohes Gemuese ist Zutat" hat zwei Pruefungen nicht ueberstanden:
    //
    //   1. RCP_ChefZ_FarmersBreakfast verlangt ChefZ_Onion in einem
    //      PFLICHT-Slot, die nachgebesserten Fischeintoepfe verlangen
    //      ChefZ_Carrot. Wer in einem Kochgeraet liegt und keine Uebergaenge
    //      hat, verbrennt beim ersten Garstufenwechsel (01 V4,
    //      FoodStage.c:472 faellt auf BURNED zurueck).
    //   2. ChefZ_RecipeEvaluator.CheckStages verlangt von JEDER gebundenen
    //      Pflichtzutat eine erlaubte Vanilla-Endstufe. Eine Klasse ohne
    //      FoodStage meldet Stufe 0 (NONE) - das Gericht wuerde nie fertig.
    //
    // Vanilla macht es bei seinem eigenen Gemuese genauso: Potato.c,
    // GreenBellPepper.c und Zucchini.c sind kochbar, ihre Configklassen tragen
    // Stufen und Uebergaenge. ChefZ-Gemuese ist kein Sonderfall.
    //
    // class Nutrition bleibt PFLICHT - PlayerStomach.InitData registriert nur
    // Klassen mit Nutrition ODER Food (01 V7). Die Stufen-Naehrwerte stehen an
    // jeder einzelnen Klasse, weil sie class Nutrition schlagen
    // (Edible_Base.c:394-503).
    //
    // MODELLE: alles Vanilla-Proxys, Bedarf im Slice-Bericht gemeldet.
    //==========================================================================
    class ChefZ_VegetableFood_Base : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\food\zucchini.p3d";
        weight = 150;
        itemSize[] = {1, 1};
        rotationFlags = 17;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        varQuantityDestroyOnMin = 1;
        absorbency = 0.0;
        isMeleeWeapon = 0;
        soundImpactType = "food";
        lifetime = 21600;

        // visual_properties[] = { selectionIndex, textureIndex, materialIndex }
        // Alle Proxys sind einteilige Modelle - deshalb ueberall 0.
        // cooking_properties[] = { minTemp, cookTime, maxTemp }
        // woertlich aus enum eCookingPropertyIndices (FoodStage.c:15).
        //
        // Die Stufen-NAEHRWERTE stehen NICHT hier, sondern an jeder Klasse:
        // Zwiebel, Knoblauch, Karotte und Kohl liegen zu weit auseinander, als
        // dass ein Basiswert fuer mehr als einen von ihnen richtig waere.
        class Food
        {
            class FoodStages
            {
                class Raw
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                };
                class Baked
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {100, 60, 200};
                };
                class Boiled
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {100, 80, 150};
                };
                class Burned
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {200, 20, 0};
                };
                class Rotten
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                };
            };

            // OHNE DIESEN BLOCK VERBRENNT JEDES STUECK GEMUESE BEIM ERSTEN
            // GARSTUFENWECHSEL (01 V4, FoodStage.c:472).
            //
            // transition_to und cooking_method sind ZAHLEN, nicht Namen:
            // SetupFoodStageTransitionMapping liest sie mit ConfigGetInt
            // (FoodStage.c:167ff).
            //   FoodStageType:     RAW 1, BAKED 2, BOILED 3, DRIED 4, BURNED 5, ROTTEN 6
            //   CookingMethodType: NONE 0, BAKING 1, BOILING 2, DRYING 3, TIME 4
            //
            // Nur Uebergaenge AUS "Raw". DRYING (3) fehlt absichtlich, obwohl
            // Vanilla rohes Gemuese trocknen laesst: Trocknen ist in ChefZ ein
            // Vorgang am eigenen Trockenrahmen (11 E6) mit eigenen
            // Haltbarkeiten, und Vanillas Trocknen kennt genau einen Uebergang
            // RAW -> DRIED (01 V14).
            class FoodStageTransitions
            {
                class Raw
                {
                    class Baking
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class Boiling
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    class ChefZ_VegetablePlant_Base : PlantBase
    {
        scope = 0;
        model = "\dz\gear\cultivation\plant_material.p3d";
    };

    class ChefZ_VegetableSeeds_Base : SeedBase
    {
        scope = 0;
        model = "\dz\gear\cultivation\tomato_seeds.p3d";
        weight = 5;
        itemSize[] = {1, 1};
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 14400;
    };

    // --- §17 Zwiebel ---------------------------------------------------------
    class ChefZ_OnionPlant : ChefZ_VegetablePlant_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_ONION";
        descriptionShort = "#STR_CHEFZ_PLANT_ONION_DESC";
        class Horticulture
        {
            GrowthStagesCount = 5;
            CropsCount = 4;
            CropsType = "ChefZ_Onion";
        };
    };

    class ChefZ_OnionSeeds : ChefZ_VegetableSeeds_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_ONION";
        descriptionShort = "#STR_CHEFZ_SEEDS_ONION_DESC";
        class Horticulture
        {
            PlantType = "ChefZ_OnionPlant";
        };
    };

    class ChefZ_Onion : ChefZ_VegetableFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_ONION";
        descriptionShort = "#STR_CHEFZ_ITEM_ONION_DESC";
        model = "\dz\gear\food\apple.p3d";
        weight = 160;
        class Nutrition
        {
            fullnessIndex = 25;
            energy = 90;
            water = 55;
            nutritionalIndex = 30;
            toxicity = 0;
            digestibility = 1;
        };

        // Stufen-Naehrwerte (01 V7). Rohwerte = class Nutrition; Gebacken
        // trocknet aus und verdichtet, Gekocht zieht Wasser und verliert
        // Vitamine, Verbrannt und Verdorben sind Verlust.
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {25, 90, 55, 30, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {22, 105, 25, 32, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {24, 95, 63, 26, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {6, 14, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {6, 14, 11, 0, 15, 0, 1}; };
            };
        };
    };

    // --- §18 Knoblauch -------------------------------------------------------
    class ChefZ_GarlicPlant : ChefZ_VegetablePlant_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_GARLIC";
        descriptionShort = "#STR_CHEFZ_PLANT_GARLIC_DESC";
        class Horticulture
        {
            GrowthStagesCount = 5;
            CropsCount = 4;
            CropsType = "ChefZ_Garlic";
        };
    };

    class ChefZ_GarlicSeeds : ChefZ_VegetableSeeds_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_GARLIC";
        descriptionShort = "#STR_CHEFZ_SEEDS_GARLIC_DESC";
        model = "\dz\gear\cultivation\pumpkin_seeds.p3d";
        class Horticulture
        {
            PlantType = "ChefZ_GarlicPlant";
        };
    };

    class ChefZ_Garlic : ChefZ_VegetableFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_GARLIC";
        descriptionShort = "#STR_CHEFZ_ITEM_GARLIC_DESC";
        model = "\dz\gear\food\mushroom_agaricus.p3d";
        weight = 60;
        class Nutrition
        {
            fullnessIndex = 8;
            energy = 40;
            water = 15;
            nutritionalIndex = 40;
            toxicity = 0;
            digestibility = 1;
        };

        // Stufen-Naehrwerte (01 V7). Rohwerte = class Nutrition; Gebacken
        // trocknet aus und verdichtet, Gekocht zieht Wasser und verliert
        // Vitamine, Verbrannt und Verdorben sind Verlust.
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {8, 40, 15, 40, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {7, 46, 7, 42, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {8, 42, 17, 34, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {2, 6, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {2, 6, 3, 0, 15, 0, 1}; };
            };
        };
    };

    // --- §19 Karotte ---------------------------------------------------------
    class ChefZ_CarrotPlant : ChefZ_VegetablePlant_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_CARROT";
        descriptionShort = "#STR_CHEFZ_PLANT_CARROT_DESC";
        class Horticulture
        {
            GrowthStagesCount = 5;
            CropsCount = 5;
            CropsType = "ChefZ_Carrot";
        };
    };

    class ChefZ_CarrotSeeds : ChefZ_VegetableSeeds_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_CARROT";
        descriptionShort = "#STR_CHEFZ_SEEDS_CARROT_DESC";
        class Horticulture
        {
            PlantType = "ChefZ_CarrotPlant";
        };
    };

    class ChefZ_Carrot : ChefZ_VegetableFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CARROT";
        descriptionShort = "#STR_CHEFZ_ITEM_CARROT_DESC";
        weight = 120;
        class Nutrition
        {
            fullnessIndex = 30;
            energy = 100;
            water = 60;
            nutritionalIndex = 45;
            toxicity = 0;
            digestibility = 1;
        };

        // Stufen-Naehrwerte (01 V7). Rohwerte = class Nutrition; Gebacken
        // trocknet aus und verdichtet, Gekocht zieht Wasser und verliert
        // Vitamine, Verbrannt und Verdorben sind Verlust.
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {30, 100, 60, 45, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {27, 115, 27, 47, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {29, 105, 69, 38, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {8, 15, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {8, 15, 12, 0, 15, 0, 1}; };
            };
        };
    };

    // --- §20 Kohl ------------------------------------------------------------
    class ChefZ_CabbagePlant : ChefZ_VegetablePlant_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_CABBAGE";
        descriptionShort = "#STR_CHEFZ_PLANT_CABBAGE_DESC";
        class Horticulture
        {
            GrowthStagesCount = 6;
            CropsCount = 2;
            CropsType = "ChefZ_Cabbage";
        };
    };

    class ChefZ_CabbageSeeds : ChefZ_VegetableSeeds_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_CABBAGE";
        descriptionShort = "#STR_CHEFZ_SEEDS_CABBAGE_DESC";
        model = "\dz\gear\cultivation\Zucchini_seeds.p3d";
        class Horticulture
        {
            PlantType = "ChefZ_CabbagePlant";
        };
    };

    class ChefZ_Cabbage : ChefZ_VegetableFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CABBAGE";
        descriptionShort = "#STR_CHEFZ_ITEM_CABBAGE_DESC";
        model = "\dz\gear\food\Pumpkin_fresh.p3d";
        weight = 900;
        itemSize[] = {2, 2};
        class Nutrition
        {
            fullnessIndex = 45;
            energy = 110;
            water = 80;
            nutritionalIndex = 40;
            toxicity = 0;
            digestibility = 1;
        };

        // Stufen-Naehrwerte (01 V7). Rohwerte = class Nutrition; Gebacken
        // trocknet aus und verdichtet, Gekocht zieht Wasser und verliert
        // Vitamine, Verbrannt und Verdorben sind Verlust.
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {45, 110, 80, 40, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {40, 125, 36, 42, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {43, 115, 92, 34, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {11, 17, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {11, 17, 16, 0, 15, 0, 1}; };
            };
        };
    };

    //==========================================================================
    // ### SLICE herbs ###   Production Map §21-§24, §15, §16
    //
    // Fuenf Kraeuter, dazu Pfeffer: Pflanze -> Ernte -> (spaeter,
    // in ChefZ_Processing) Trockenrahmen und Moerser. Paprika steht hier nicht
    // mehr - sie ist vollstaendig Vanilla (Vanilla-Audit §2).
    //
    // KEIN NEUES CORE-SYSTEM: Anbau und Ernte laufen vollstaendig ueber
    // Vanillas Gartenkette. GardenBase.c:386/425 liest "CfgVehicles <samen>
    // Horticulture PlantType", PlantBase.c:63-65 liest GrowthStagesCount,
    // CropsCount und CropsType der Pflanze. Es ist keine Zeile ChefZ-Code
    // noetig, um Kraeuter anzupflanzen und zu ernten.
    //
    // SELTENHEIT (Production Map §21: Petersilie haeufig ... Rosmarin selten)
    // steuert die VERFUEGBARKEIT der Samen und das Weltvorkommen, nicht die
    // Wachstumszeit. Beides gehoert in die Servertypen (types.xml /
    // mapgroupproto) und ist als offener Punkt gemeldet - eine Loot-Tabelle
    // ist kein Modulinhalt.
    //
    // class Nutrition ist PFLICHT (01 V7). Bewusst OHNE class Food /
    // FoodStages: frische Kraeuter kommen nicht in den Topf, sie kommen auf
    // den Trockenrahmen. Wer FoodStages ohne FoodStageTransitions deklariert,
    // baut die Falle aus 01 V4.
    //
    // PROXY-MODELLE, alle Vanilla, alle im Asset-Bedarf des Slice gemeldet.
    //==========================================================================
    class ChefZ_HerbPlantBase : PlantBase
    {
        scope = 0;
        model = "\dz\gear\cultivation\plant_material.p3d";
        weight = 200;
        itemSize[] = {2, 2};
        rotationFlags = 2;
    };

    class ChefZ_ParsleyPlant : ChefZ_HerbPlantBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_PARSLEY";
        descriptionShort = "#STR_CHEFZ_PLANT_PARSLEY_DESC";
        class Horticulture
        {
            GrowthStagesCount = 5;
            CropsCount = 3;
            CropsType = "ChefZ_Parsley";
        };
    };

    class ChefZ_DillPlant : ChefZ_HerbPlantBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_DILL";
        descriptionShort = "#STR_CHEFZ_PLANT_DILL_DESC";
        class Horticulture
        {
            GrowthStagesCount = 5;
            CropsCount = 3;
            CropsType = "ChefZ_Dill";
        };
    };

    class ChefZ_ThymePlant : ChefZ_HerbPlantBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_THYME";
        descriptionShort = "#STR_CHEFZ_PLANT_THYME_DESC";
        class Horticulture
        {
            GrowthStagesCount = 5;
            CropsCount = 2;
            CropsType = "ChefZ_Thyme";
        };
    };

    class ChefZ_RosemaryPlant : ChefZ_HerbPlantBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_ROSEMARY";
        descriptionShort = "#STR_CHEFZ_PLANT_ROSEMARY_DESC";
        class Horticulture
        {
            GrowthStagesCount = 5;
            CropsCount = 2;
            CropsType = "ChefZ_Rosemary";
        };
    };

    class ChefZ_WildGarlicPlant : ChefZ_HerbPlantBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_WILDGARLIC";
        descriptionShort = "#STR_CHEFZ_PLANT_WILDGARLIC_DESC";
        class Horticulture
        {
            GrowthStagesCount = 5;
            CropsCount = 3;
            CropsType = "ChefZ_WildGarlic";
        };
    };

    // Pfeffer traegt Beeren und ist bewusst selten (Production Map §16).
    class ChefZ_PepperPlant : ChefZ_HerbPlantBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_PEPPER";
        descriptionShort = "#STR_CHEFZ_PLANT_PEPPER_DESC";
        class Horticulture
        {
            GrowthStagesCount = 5;
            CropsCount = 2;
            CropsType = "ChefZ_PepperBerries";
        };
    };

    // KEINE eigene Paprikapflanze (Vanilla-Audit §2). Vanilla schliesst den
    // Kreis bereits vollstaendig: PepperSeedsPack -> PepperSeeds -> Plant_Pepper
    // -> GreenBellPepper -> CutOutPepperSeeds -> PepperSeeds. Die frueheren
    // ChefZ_PaprikaPlant / ChefZ_PaprikaSeeds bauten dieselbe Kette nach, waren
    // aber unerreichbar: kein Datensatz im Projekt erzeugte je ChefZ_PaprikaSeeds.
    // ChefZ setzt jetzt an der Frucht an - GreenBellPepper traegt den
    // Zutaten-Datensatz (ChefZ_Ingredients/Config/Ingredients/VanillaProduce.json)
    // und ist Eingang von TR_ChopBellPepper und TR_PaprikaToDried.

    //--------------------------------------------------------------------------
    // Samen. SeedBase bringt ActionPlantSeed und ActionAttachSeeds mit; mehr
    // braucht es nicht.
    //--------------------------------------------------------------------------
    class ChefZ_HerbSeedsBase : SeedBase
    {
        scope = 0;
        model = "\dz\gear\cultivation\tomato_seeds.p3d";
        weight = 10;
        itemSize[] = {1, 1};
        rotationFlags = 17;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 14400;
    };

    class ChefZ_ParsleySeeds : ChefZ_HerbSeedsBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_PARSLEY";
        descriptionShort = "#STR_CHEFZ_SEEDS_PARSLEY_DESC";
        class Horticulture
        {
            PlantType = "ChefZ_ParsleyPlant";
        };
    };

    class ChefZ_DillSeeds : ChefZ_HerbSeedsBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_DILL";
        descriptionShort = "#STR_CHEFZ_SEEDS_DILL_DESC";
        class Horticulture
        {
            PlantType = "ChefZ_DillPlant";
        };
    };

    class ChefZ_ThymeSeeds : ChefZ_HerbSeedsBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_THYME";
        descriptionShort = "#STR_CHEFZ_SEEDS_THYME_DESC";
        class Horticulture
        {
            PlantType = "ChefZ_ThymePlant";
        };
    };

    class ChefZ_RosemarySeeds : ChefZ_HerbSeedsBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_ROSEMARY";
        descriptionShort = "#STR_CHEFZ_SEEDS_ROSEMARY_DESC";
        class Horticulture
        {
            PlantType = "ChefZ_RosemaryPlant";
        };
    };

    class ChefZ_WildGarlicSeeds : ChefZ_HerbSeedsBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_WILDGARLIC";
        descriptionShort = "#STR_CHEFZ_SEEDS_WILDGARLIC_DESC";
        model = "\dz\gear\cultivation\cannabis_seeds.p3d";
        class Horticulture
        {
            PlantType = "ChefZ_WildGarlicPlant";
        };
    };

    // Nicht "ChefZ_PepperSeeds": Vanillas PepperSeeds ist PAPRIKA-Saatgut
    // (PlantType Plant_Pepper -> GreenBellPepper). Hier waechst Piper nigrum.
    // Zwei verschiedene Pflanzen duerfen nicht denselben Namen im Inventar
    // tragen (Vanilla-Audit §2).
    class ChefZ_PeppercornSeeds : ChefZ_HerbSeedsBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_PEPPERCORN";
        descriptionShort = "#STR_CHEFZ_SEEDS_PEPPERCORN_DESC";
        model = "\dz\gear\cultivation\pepper_seeds.p3d";
        class Horticulture
        {
            PlantType = "ChefZ_PepperPlant";
        };
    };

    //--------------------------------------------------------------------------
    // Die Ernte.
    //--------------------------------------------------------------------------
    class ChefZ_FreshHerbBase : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\cultivation\plant_material.p3d";
        weight = 20;
        itemSize[] = {1, 1};
        rotationFlags = 17;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 14400;

        class Nutrition
        {
            fullnessIndex = 5;
            energy = 15;
            water = 12;
            nutritionalIndex = 25;
            toxicity = 0;
            digestibility = 1;
        };
    };

    class ChefZ_Parsley : ChefZ_FreshHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_PARSLEY";
        descriptionShort = "#STR_CHEFZ_ITEM_PARSLEY_DESC";
    };

    class ChefZ_Dill : ChefZ_FreshHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DILL";
        descriptionShort = "#STR_CHEFZ_ITEM_DILL_DESC";
    };

    class ChefZ_Thyme : ChefZ_FreshHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_THYME";
        descriptionShort = "#STR_CHEFZ_ITEM_THYME_DESC";
    };

    class ChefZ_Rosemary : ChefZ_FreshHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_ROSEMARY";
        descriptionShort = "#STR_CHEFZ_ITEM_ROSEMARY_DESC";
    };

    class ChefZ_WildGarlic : ChefZ_FreshHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_WILDGARLIC";
        descriptionShort = "#STR_CHEFZ_ITEM_WILDGARLIC_DESC";
        model = "\dz\gear\food\cannabis_seedman.p3d";
    };

    // Pfefferbeeren: Rohstoff, kein Gewuerz - erst der Trockenrahmen macht
    // Pfefferkoerner daraus (Production Map §16).
    class ChefZ_PepperBerries : ChefZ_FreshHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_PEPPERBERRIES";
        descriptionShort = "#STR_CHEFZ_ITEM_PEPPERBERRIES_DESC";
        model = "\dz\gear\food\Sambucus_nigra.p3d";
        class Nutrition
        {
            fullnessIndex = 4;
            energy = 12;
            water = 8;
            nutritionalIndex = 10;
            toxicity = 0;
            digestibility = 1;
        };
    };

    // KEINE eigene Paprikaklasse (Vanilla-Audit §2). Frische Paprika ist
    // Vanillas GreenBellPepper. Das fruehere ChefZ_Paprika trug bereits
    // dz/gear/food/pepper_green.p3d, dieselbe einzige Kategorie VEGETABLE,
    // denselben defaultState RAW und stand mit GreenBellPepper im selben
    // anyOf-Slot beider Chili-Rezepte; erreichbar war es nie (siehe Kommentar
    // bei den Samen weiter oben). Die Trockenkette §15 haengt jetzt an
    // GreenBellPepper: TR_PaprikaToDried (ChefZ_Processing) -> ChefZ_DriedPaprika
    // -> ChefZ_PaprikaPowder. Der Food-Block mit den Garstufen entfaellt
    // ersatzlos - GreenBellPepper bringt ihn aus Vanilla mit.

    //==========================================================================
    // ### SLICE apiary ###   Imkerei - Honig ernten
    //
    // Auftragsnamen -> Klassennamen (DME-Plan §53, ChefZ_PascalCase):
    //
    //   Bienenstock            ChefZ_Beehive
    //   Beehive_Kit            ChefZ_BeehiveKit
    //   Honigwabe_Leer         ChefZ_HoneycombFrameEmpty
    //   (ohne Auftragsnamen)   ChefZ_HoneycombFrameSealed   - Begruendung unten
    //   Honigwabe_Voll         ChefZ_HoneycombFrameFull
    //   Frame_Ready_To_Spin    ChefZ_HoneycombFrameUncapped
    //   Uncapping_Fork         ChefZ_UncappingFork
    //   Smoker                 ChefZ_BeeSmoker
    //   Honey_Extractor        ChefZ_HoneyExtractor  (in ChefZ_Processing)
    //   Honey_Jar_Empty        GlassBottle           (VANILLA, siehe unten)
    //   Honigglas befuellt     Honey                 (VANILLA)
    //
    // WAS VANILLA SCHON MITBRINGT UND DESHALB HIER FEHLT
    // --------------------------------------------------
    //   Honey            das gefuellte Honigglas. ChefZ_Vanilla_Assets §16
    //                    fuehrt es mit nominal 60 in Farm/Town/Village/School.
    //                    Es ist das ERGEBNIS der Kette, keine neue Klasse.
    //   GlassBottle      das leere Glas. ChefZ_Vanilla_Assets §18 nennt es
    //                    ausdruecklich "Kandidat fuer das ChefZ-Einmachglas".
    //   WoodenPlank      die Bretter des Auftrags - und der Griff der Gabel.
    //   Nails            die Naegel des Auftrags - und die Zinken der Gabel.
    //   TunaCan_Opened   der Blechkoerper der Imkerpfeife.
    //   Hammer/Hatchet/Pliers/Screwdriver   HAND_TOOL beim Aufstellen des
    //                    Stocks und beim Formen der Imkerpfeife.
    //
    // Alle Bau-Rezepte kommen bewusst mit GENAU DIESEN drei Vanillaklassen
    // aus. Der erste Entwurf benutzte zusaetzlich WoodenStick und Rag; beide
    // sind Stapel mit Menge, und ihr varQuantityMax liegt dem Projekt nicht
    // vor (die Item-Configs von DayZ fehlen, refindex fuehrt nur Namen). Eine
    // Mengenangabe darauf waere geraten gewesen. Fuer WoodenPlank und Nails
    // gibt es einen Beleg - siehe Config/Processing/README_Apiary.md.
    //
    // Vanilla hat KEINE Imkerpfeife, KEINE Entdeckelungsgabel, KEINEN
    // Bienenstock und KEINE Wabe - gesucht wurde in refindex/vanilla-classes.txt
    // und refindex/vanilla-scripts-classes.txt nach "bee", "honey", "smok" und
    // "fork". Der einzige Treffer auf "fork" ist Pitchfork, eine Mistgabel.
    // Diese fuenf Klassen bringt ChefZ deshalb selbst mit.
    //
    // WARUM ES VIER RAEHMCHEN GIBT UND NICHT DREI
    // -------------------------------------------
    // Der Auftrag nennt drei Zustaende. Ein vierter steht dazwischen, und er
    // ist der einzige Grund, aus dem die Imkerpfeife im Spiel etwas bedeutet:
    //
    //   Empty     leer, vom Spieler gebaut, wird in den Stock gehaengt
    //   Sealed    von den Bienen ausgebaut und VERDECKELT - im Stock, voller
    //             Bienen. Kein Eingang irgendeines weiteren Schrittes.
    //   Full      geerntet: aus dem Stock genommen, von Bienen befreit. NUR
    //             PROCESS_HARVEST_HIVE erzeugt es, und der verlangt die
    //             Imkerpfeife als Werkzeuggruppe.
    //   Uncapped  entdeckelt, schleuderfertig (Frame_Ready_To_Spin)
    //
    // Ohne "Sealed" gaebe es keinen Schritt, an dem die Pfeife haengen
    // koennte: der Cargo-Bereich einer Station ist fuer den Spieler frei
    // zugaenglich, ein fertiges Raehmchen koennte er einfach herausnehmen.
    // Die Ernte MUSS deshalb ein eigener Vorgang an der Station sein.
    //
    // 3D
    // --
    // Alle Klassen tragen VANILLA-PROXY-MODELLE aus im Projekt bereits
    // belegten Pfaden. Der Bedarf an eigener Geometrie ist im Slice-Bericht
    // als Asset-Bedarf gemeldet; auf ein Modell wartet hier nichts.
    //==========================================================================

    //--------------------------------------------------------------------------
    // Der Bienenstock (Auftrag: "Bienenstock").
    //
    // Er ist eine VERARBEITUNGSSTATION im Sinne von
    // ChefZ_ProcessingStation_Base - die Andockregel steht woertlich im Kopf
    // jener Datei:
    //
    //     config.cpp   class ChefZ_Beehive : Inventory_Base { ... };
    //     JSON/Rang 2  { "kind":"station", "records":[{ "id":"ChefZ_Beehive" }] }
    //     Skript       class ChefZ_Beehive extends ChefZ_ProcessingStation_Base
    //
    // WARUM STATION UND NICHT ETWAS ANDERES: der Auftrag sagt "Bienenstock
    // oeffnen" und "vollen Rahmen entnehmen". Beides setzt einen Innenraum
    // voraus, in dem Raehmchen liegen - und genau das ist der Cargo-Bereich,
    // aus dem ChefZ_ProcessingStation_Base ueber
    // ChefZ_FactCollector.CollectFromCargo seine Zutaten liest. Ein Item ohne
    // Cargo koennte nichts aufnehmen; im Projekt sind Stationen genau daran
    // schon gescheitert (siehe ChefZ_CuttingBoard in ChefZ_Processing).
    //
    // class Cargo IST die Eingangsseite. 4x3 fasst vier Raehmchen nebeneinander
    // - dieselbe Groessenordnung wie der Trockenrahmen, und sie deckt sich mit
    // parallelSlots = 4 im Stationsdatensatz.
    //
    // KEIN Pot, KEIN Cauldron, KEIN FireplaceBase als Basis: alle drei sind
    // Vanillas Kochgeschirr bzw. Feuerstellen und wuerden den Stock in Vanillas
    // Kochkette ziehen (Begruendung ausgeschrieben an ChefZ_ButterChurn in
    // ChefZ_Processing). Inventory_Base haelt ihn heraus.
    //
    // KEIN ChefZ_HasHeat: Bienen brauchen kein Feuer. needsFuel bleibt false.
    //
    // PROXY: wooden_case.p3d - eine Holzkiste. Eine Magazinbeute IST eine
    // Holzkiste; von allen im Projekt belegten Pfaden ist das der einzige, der
    // nicht nur ungefaehr passt. Eigenes Beutenmesh ist gemeldet.
    //--------------------------------------------------------------------------
    class ChefZ_Beehive : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BEEHIVE";
        descriptionShort = "#STR_CHEFZ_ITEM_BEEHIVE_DESC";
        model = "\DZ\gear\camping\wooden_case.p3d";
        rotationFlags = 2;
        itemSize[] = {6, 5};
        weight = 14000;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        lifetime = 172800;

        class Cargo
        {
            itemsCargoSize[] = {4, 3};
            openable = 0;
        };
    };

    //--------------------------------------------------------------------------
    // Der Bausatz (Auftrag: "Beehive_Kit").
    //
    // Warum es ihn ueberhaupt gibt, obwohl TR_AssemblePastaMachine in
    // ChefZ_Processing zeigt, dass ein Geraet auch in EINEM Schritt entstehen
    // darf: der Auftrag nennt ihn als eigenes Mittel, und er traegt eine
    // Aussage, die der fertige Stock nicht traegt. Der Bausatz ist flach
    // gepackt (3x2, 6 kg) und laesst sich tragen; der aufgestellte Stock ist
    // 6x5 und 14 kg. Gebaut wird am Lager, aufgestellt wird an der Wiese.
    //
    // KEIN Hologramm-Deploy: das waere ein neues System (CanBePlaced,
    // ActionPlaceObject, Hologramm-Config) und steht diesem Slice nicht zu.
    // Der zweite Schritt laeuft als gewoehnlicher Handwerksschritt
    // (PROCESS_RAISE_HIVE) - genauso, wie die uebrigen Stationen des Projekts
    // entstehen.
    //
    // PROXY: dieselbe Holzkiste, kleiner gefuehrt.
    //--------------------------------------------------------------------------
    class ChefZ_BeehiveKit : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BEEHIVEKIT";
        descriptionShort = "#STR_CHEFZ_ITEM_BEEHIVEKIT_DESC";
        model = "\DZ\gear\camping\wooden_case.p3d";
        rotationFlags = 17;
        itemSize[] = {3, 2};
        weight = 6000;
        absorbency = 0.0;
        canBeDigged = 0;
        lifetime = 43200;
    };

    //--------------------------------------------------------------------------
    // Die vier Raehmchen.
    //
    // Eine gemeinsame Basis mit scope = 0: sie unterscheiden sich in Name,
    // Beschreibung und Gewicht, in nichts sonst. Die Basis steht einmal da,
    // weil configcpp.mjs Klassennamen projektweit auf Eindeutigkeit prueft und
    // vier gleichlautende Bloecke vier Gelegenheiten waeren, sie
    // auseinanderlaufen zu lassen.
    //
    // KEINE Nahrung: weder Nutrition noch Food noch FoodStages. Eine volle
    // Wabe waere essbar, und genau deshalb steht hier die Entscheidung: das
    // Ergebnis der Kette ist Vanillas Honey, und der ist essbar. Ein zweiter
    // essbarer Honigtraeger haette Nutrition, FoodStageTransitions UND eine
    // Essaktion gebraucht (01 V7, 01 V4, chefzcookable Regel C) - drei
    // Zusagen fuer einen Bissen, den niemand verlangt hat.
    //
    // PROXY: birch_bark.p3d - ein flaches, plattenfoermiges Objekt in der
    // richtigen Groessenordnung. UNPLAUSIBEL im Sinne von Asset-Backlog §10.3:
    // Birkenrinde ist gewellt, ein Raehmchen ist ein rechteckiger Holzrahmen.
    // Es ist der beste im Projekt belegte Pfad, mehr nicht - und alle vier
    // Zustaende sehen damit gleich aus. Eigene Geometrie mit vier sichtbar
    // verschiedenen Fuellgraden ist als Asset-Bedarf gemeldet.
    //--------------------------------------------------------------------------
    class ChefZ_HoneycombFrame_Base : Inventory_Base
    {
        scope = 0;
        model = "\dz\gear\consumables\birch_bark.p3d";
        itemSize[] = {2, 3};
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        lifetime = 43200;
        repairableWithKits[] = {};
    };

    //! Auftrag: "Honigwabe_Leer". Ergebnis von TR_BuildHoneycombFrame,
    //! Eingang von PROCESS_TEND_HIVE - und Nebenprodukt des Schleuderns, das
    //! es leer zurueckgibt. Damit ist die Kette ein Kreis, kein Strahl.
    class ChefZ_HoneycombFrameEmpty : ChefZ_HoneycombFrame_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COMBFRAME_EMPTY";
        descriptionShort = "#STR_CHEFZ_ITEM_COMBFRAME_EMPTY_DESC";
        weight = 400;
    };

    //! Verdeckelt, im Stock, voller Bienen. Es gibt fuer diesen Zustand
    //! ABSICHTLICH keinen weiteren Verwendungszweck: wer ihn aus dem Cargo
    //! nimmt, hat ein schweres Stueck Wachs und sonst nichts. Der Weg nach
    //! vorn fuehrt ausschliesslich ueber PROCESS_HARVEST_HIVE.
    class ChefZ_HoneycombFrameSealed : ChefZ_HoneycombFrame_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COMBFRAME_SEALED";
        descriptionShort = "#STR_CHEFZ_ITEM_COMBFRAME_SEALED_DESC";
        weight = 2200;
    };

    //! Auftrag: "Honigwabe_Voll" / "Honeycomb_Frame_Full".
    class ChefZ_HoneycombFrameFull : ChefZ_HoneycombFrame_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COMBFRAME_FULL";
        descriptionShort = "#STR_CHEFZ_ITEM_COMBFRAME_FULL_DESC";
        weight = 2200;
    };

    //! Auftrag: "Frame_Ready_To_Spin". Eingang der Honigschleuder.
    class ChefZ_HoneycombFrameUncapped : ChefZ_HoneycombFrame_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COMBFRAME_UNCAPPED";
        descriptionShort = "#STR_CHEFZ_ITEM_COMBFRAME_UNCAPPED_DESC";
        weight = 2100;
    };

    //--------------------------------------------------------------------------
    // Die Entdeckelungsgabel (Auftrag: "Uncapping_Fork").
    //
    // Sie ist das einzige Mitglied der Werkzeuggruppe UNCAPPING_TOOL. Dass
    // eine Gruppe nur eine Klasse fuehrt, war im Projekt schon einmal ein
    // stiller Ausfall (ChefZ_RollingPin: kein Loot, kein Craft, damit war
    // PROCESS_ROLL unerreichbar und die Backkette tot). Der Unterschied hier
    // ist, dass diese Klasse eine QUELLE hat: TR_BuildUncappingFork baut sie
    // aus einem Brett und vier Naegeln, beides gewoehnliches Vanilla-Gut.
    // Deshalb steht hier KEINE Vanilla-Klasse zusaetzlich in der Gruppe -
    // ein Kuechenmesser waere ein zweiter Weg zum selben Ziel und machte die
    // Gabel ueberfluessig.
    //
    // PROXY: Meat_Tenderizer.p3d - ein metallenes Kuechengeraet mit Griff.
    // Derselbe Pfad, den ChefZ_PastaMachine und ChefZ_CuttingBoard tragen und
    // den Asset-Backlog §10.1 als den korrekten ausweist.
    //--------------------------------------------------------------------------
    class ChefZ_UncappingFork : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_UNCAPPINGFORK";
        descriptionShort = "#STR_CHEFZ_ITEM_UNCAPPINGFORK_DESC";
        model = "\dz\gear\tools\Meat_Tenderizer.p3d";
        rotationFlags = 17;
        itemSize[] = {1, 3};
        weight = 340;
        repairableWithKits[] = {};
        lifetime = 43200;
    };

    //--------------------------------------------------------------------------
    // Die Imkerpfeife (Auftrag: "Smoker").
    //
    // ACHTUNG, NAMENSGLEICHHEIT: ChefZ_Processing fuehrt bereits eine Klasse
    // ChefZ_Smoker - das ist der RAEUCHERSCHRANK der Konservierungskette, ein
    // ganz anderes Geraet. Diese hier heisst deshalb ChefZ_BeeSmoker. Zwei
    // Klassen gleichen Namens waeren fuer configcpp.mjs ein Fehler und fuer
    // den Spieler eine Verwechslung.
    //
    // Sie ist reines Werkzeug: sie traegt keinen ChefZ-Zustand und wird nicht
    // verbraucht, sondern nur ueber die Werkzeuggruppe BEE_SMOKER gefunden.
    // Beim Ernten nutzt sie sich ab (toolDamage an PROCESS_HARVEST_HIVE).
    //
    // QUELLE: TR_BuildBeeSmoker aus einer TunaCan_Opened plus einem Werkzeug
    // der Gruppe HAND_TOOL. Ohne diesen Weg gaebe es sie im Spiel nicht -
    // ChefZ liefert projektweit keine types.xml (Gate 4, B8) - und mit ihr
    // fiele der ganze Ernteschritt aus.
    //
    // PROXY: food_can_open.p3d - eine offene Blechdose. Eine Imkerpfeife IST
    // ein Blechbehaelter mit Deckel und Balg; die Dose ist davon die Haelfte,
    // und sie ist im Projekt belegt (ChefZ_Cooking fuehrt sie bereits).
    // Eigenes Mesh mit Balg und Rauchtuelle ist gemeldet.
    //--------------------------------------------------------------------------
    class ChefZ_BeeSmoker : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BEESMOKER";
        descriptionShort = "#STR_CHEFZ_ITEM_BEESMOKER_DESC";
        model = "\dz\gear\food\food_can_open.p3d";
        rotationFlags = 17;
        itemSize[] = {2, 3};
        weight = 900;
        repairableWithKits[] = {};
        lifetime = 43200;
    };
};

//------------------------------------------------------------------------------
// Modulanmeldung am Config Manager (Entwurf 02 §4).
//
// ChefZ_Farming bringt keine JSON-Datensaetze mit: seine Zutatenbindung steht
// als Klassenbaum darunter, und das ist laut 02 §4 ausdruecklich erlaubt
// ("Kleine Datenmengen duerfen direkt als Klassenbaum kommen").
//
// handcraftRecipeSlots fehlt bewusst: dieses Modul bringt keinen einzigen
// HANDCRAFT-Transform mit und reserviert deshalb null Plaetze in Vanillas
// Rezeptliste (02 §4.2).
//------------------------------------------------------------------------------
class CfgChefZ
{
    // ### SLICE grain ### Ein Knoten je SLICE (02 §4), nicht je Modul: er
    // heisst deshalb nicht wie das Addon. Ein gleichnamiger Knoten neben dem
    // CfgMods-Eintrag zaehlte fuer configcpp.mjs als doppelte
    // Klassendefinition.
    class ChefZ_GrainFarming
    {
        chefzApiVersion = 1;
        loadOrder = 210;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Farming/Config/GrainIngredients.json"
        };
    };

    // ### SLICE herbs ### Die Zutatenbindung der Ernteprodukte.
    //
    // Rang 2 und nicht CfgChefZIngredients: der Zutatenknoten traegt laut
    // 05 §2 denselben Namen wie die Item-Klasse, und configcpp.mjs zaehlt das
    // projektweit als doppelte Klassendefinition. Der Config Manager liest
    // beide Raenge in denselben Index - serverseitig ist es dasselbe.
    class ChefZ_HerbFarming
    {
        chefzApiVersion = 1;
        loadOrder = 215;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Farming/Config/Ingredients/Herbs.json"
        };
    };

    // ### SLICE apiary ### Imkerei - Honig ernten.
    //
    // handcraftRecipeSlots = 6. Die Zahl ist eine RESERVIERUNG in Vanillas
    // Rezeptliste und muss VOR dem Laden feststehen; wird sie vergessen,
    // erscheint kein einziges der Rezepte, und zwar OHNE Fehlermeldung an der
    // Stelle, an der man sucht (Kopf von ChefZ_HandcraftBridge.c).
    //
    // Die sechs, einer je HANDCRAFT-Transform dieses Slice:
    //
    //   TR_BuildBeehiveKit      PROCESS_BUILD_HIVE_KIT
    //   TR_RaiseBeehive         PROCESS_RAISE_HIVE
    //   TR_BuildHoneycombFrame  PROCESS_BUILD_FRAME
    //   TR_BuildUncappingFork   PROCESS_BUILD_UNCAPPING_FORK
    //   TR_BuildBeeSmoker       PROCESS_BUILD_BEE_SMOKER
    //   TR_UncapHoneycombFrame  PROCESS_UNCAP_COMB
    //
    // Die drei Stationsvorgaenge (PROCESS_TEND_HIVE, PROCESS_HARVEST_HIVE,
    // PROCESS_SPIN_HONEY) brauchen KEINEN Platz - sie laufen ueber
    // ChefZ_ActionProcessAtStation und fassen Vanillas Rezeptliste nicht an.
    //
    // Die beiden aelteren Knoten dieses Moduls bleiben bei 0, obwohl
    // PROCESS_CUT_OUT_SEEDS vier HANDCRAFT-Transforms traegt: massgeblich ist
    // die projektweite SUMME, die ChefZ_HandcraftBridge.Reserve() ueber
    // ChefZ_ManifestReader.ReadHandcraftSlotTotal() liest - welcher Knoten sie
    // beisteuert, ist der Bruecke gleichgueltig. Diese Reservierung hier ist
    // die des Slice apiary und wird nicht mit fremdem Ueberschuss verrechnet;
    // parallel arbeitende Slices koennten ihn jederzeit verbrauchen.
    class ChefZ_Apiary
    {
        chefzApiVersion = 1;
        loadOrder = 217;
        handcraftRecipeSlots = 6;
        dataFiles[] =
        {
            "ChefZ_Farming/Config/Processing/Apiary_Ingredients.json",
            "ChefZ_Farming/Config/Processing/Apiary_Stations.json",
            "ChefZ_Farming/Config/Processing/Apiary_Crafts.json",
            "ChefZ_Farming/Config/Processing/Apiary_Hive.json"
        };
    };
};

//==============================================================================
// ### SLICE produce ### Zutatenbindung, Rang 1
//
// Entwurf 05 §2: EIGENE Klassen deklarieren sich in der eigenen config.cpp.
// Potato, Tomato und GreenBellPepper sind FREMD und stehen deshalb im
// Slice-JSON (ChefZ_Ingredients/Config/Ingredients/VanillaProduce.json), nicht
// hier - Workflow §10.5, fremde Dateien werden nie veraendert.
//
// VEGETABLE, ROOT_VEGETABLE, LEAF_VEGETABLE und CHEFZ_FRESH stehen im Delta
// _deltas/produce.json. Dieses Modul fasst keine zentrale Registry an.
//==============================================================================
class CfgChefZIngredients
{
    class ChefZ_ProduceIngredient
    {
        categories[]      = {"VEGETABLE"};
        tags[]            = {"CHEFZ_FRESH"};
        defaultState      = "RAW";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        decays            = 1;
    };

    class ChefZ_Onion : ChefZ_ProduceIngredient   { categories[] = {"VEGETABLE","ROOT_VEGETABLE"}; };
    class ChefZ_Garlic : ChefZ_ProduceIngredient  { categories[] = {"VEGETABLE","ROOT_VEGETABLE"}; };
    class ChefZ_Carrot : ChefZ_ProduceIngredient  { categories[] = {"VEGETABLE","ROOT_VEGETABLE"}; };
    class ChefZ_Cabbage : ChefZ_ProduceIngredient { categories[] = {"VEGETABLE","LEAF_VEGETABLE"}; };
};

//==============================================================================
// ### SLICE produce ### Samengewinnung, Rang 1
//
// Vanilla macht es genauso: CutOutSeeds (4_World/.../Recipes/CutOutSeeds.c) ist
// ein RecipeBase-Rezept "Gemuese + Messer -> Samen". ChefZ baut das nicht nach,
// sondern beschreibt es als Prozess - die Bruecke
// (ChefZ_GenericCraftRecipe/ChefZ_HandcraftBridge) macht daraus dieselbe Art
// Vanilla-Rezept, ohne Vanillas eigene Liste anzufassen.
//
// Ein Eingang plus Werkzeuggruppe ist die Form, die RecipeBase traegt: das
// Messer belegt den zweiten der zwei Zutatenplaetze (01 V12).
// CUTTING_TOOL kommt aus ChefZ_Processing und wird hier nur BENUTZT.
//==============================================================================
class CfgChefZProcesses
{
    class PROCESS_CUT_OUT_SEEDS
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_CUT_OUT_SEEDS";
        toolGroups[] = {"CUTTING_TOOL"};
        baseDurationSec = 6.0;
        animationLength = 1.0;
        specialty = 0.01;
        toolDamage = 1;
    };

    //--------------------------------------------------------------------------
    // ### SLICE apiary ###   Die acht Verben der Imkerei
    //
    // Rang 1 und nicht JSON, aus demselben Grund, den ChefZ_Processing an
    // seinen Prozessen ausschreibt: ChefZ_ActionProcessAtStation.
    // ActionCondition() laeuft auch auf dem CLIENT und braucht dort
    // Aktionstext und Dauer (11 E8, 02 §2). Der Client liest die Game-Config
    // garantiert.
    //
    // WARUM FUENF SEPARATE BAU-PROZESSE UND NICHT EIN GEMEINSAMES
    // "ZUSAMMENNAGELN"
    // ----------------------------------------------------------
    // Der Anzeigename eines Handwerksrezepts kommt aus dem PROZESS, nicht aus
    // dem Transform: ChefZ_GenericCraftRecipe.InitFromDef() setzt woertlich
    // "m_Name = proc.displayName". Zwei Transforms an EINEM Prozess erscheinen
    // deshalb unter demselben Menuepunkt.
    //
    // Solange sich ihre Eingaenge unterscheiden, ist das folgenlos - so machen
    // es PROCESS_CUT_OUT_SEEDS (vier Gemuese) und PROCESS_CHOP_VEGETABLE
    // (sieben). Hier ist es NICHT folgenlos: Bausatz, Raehmchen UND
    // Entdeckelungsgabel entstehen alle drei aus WoodenPlank + Nails und
    // unterscheiden sich nur in der Menge. An einem gemeinsamen Prozess
    // stuenden drei gleichnamige Eintraege im Kontextmenue, und der Spieler
    // haette keine Moeglichkeit, den richtigen zu treffen.
    //
    // Die Vorlage dafuer steht im Projekt: PROCESS_CARVE_PLATE und
    // PROCESS_CARVE_BOWL in ChefZ_Cooking sind derselbe Vorgang am selben
    // Material und trotzdem zwei Prozesse - aus genau diesem Grund. Ein Objekt
    // im Prozessnamen ist der Preis dafuer, dass der Spieler sieht, was er
    // baut.
    //--------------------------------------------------------------------------
    // ------------------------------------------------------------------
    // 1. BAUEN - vier Handwerksschritte ohne Station.
    //
    // Drei davon haben ZWEI Eingaenge (Bretter und Naegel) und deshalb
    // ausdruecklich KEINE toolGroups-Zeile: Vanillas RecipeBase kennt genau
    // MAX_NUMBER_OF_INGREDIENTS = 2, und beide Plaetze sind mit Zutaten
    // belegt. Ein Werkzeug waere der dritte Platz, und den gibt es nicht
    // (01 V12). Dieselbe Form traegt PROCESS_SALT_CURE in ChefZ_Processing.
    //
    // Der vierte - PROCESS_BUILD_BEE_SMOKER - hat EINEN Eingang und deshalb
    // eine Werkzeuggruppe. Beides ist zwingend miteinander verknuepft: ein
    // Ein-Eingang-Transform OHNE Werkzeug ist bei Vanilla nicht
    // registrierbar, weil es nichts zum Kombinieren gaebe.
    //
    // WIE MENGEN AUSGEDRUECKT WERDEN, und warum nicht ueber minCount:
    // minCount zaehlt ITEM-INSTANZEN (ChefZ_SlotEvaluator.CheckCounts),
    // Bretter und Naegel sind in DayZ aber Stapel MIT MENGE
    // (Construction.HasMaterialWithQuantityAttached fragt GetQuantity()).
    // "4x Planks" steht deshalb als amount/consumeAmount in Rezepteinheiten
    // im Transform, nicht als minCount - die vollstaendige Herleitung samt
    // der einen im Spiel nachzumessenden Zahl steht in
    // Config/Processing/README_Apiary.md.
    // ------------------------------------------------------------------

    //! Auftrag: "Bienenstock bauen: 4x Planks + 10x Nails -> Beehive_Kit".
    class PROCESS_BUILD_HIVE_KIT
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_BUILD_HIVE_KIT";
        baseDurationSec = 25.0;
        animationLength = 4.0;
        specialty = 0.03;
        toolDamage = 0;
    };

    //! Auftrag: "Rahmen bauen: 1x Plank + 2x Nails -> Honeycomb_Frame_Empty".
    class PROCESS_BUILD_FRAME
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_BUILD_FRAME";
        baseDurationSec = 12.0;
        animationLength = 2.0;
        specialty = 0.02;
        toolDamage = 0;
    };

    //! Ohne Auftragsvorgabe: der Auftrag nennt die Gabel als Mittel, sagt
    //! aber nicht, woher sie kommt. ChefZ liefert projektweit keine
    //! types.xml (Gate 4, B8) - ohne diesen Schritt gaebe es die Gabel im
    //! Spiel nicht, und PROCESS_UNCAP_COMB waere unerreichbar. Genau so ist
    //! die Backkette am Nudelholz einmal stillgestanden.
    class PROCESS_BUILD_UNCAPPING_FORK
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_BUILD_UNCAPPING_FORK";
        baseDurationSec = 15.0;
        animationLength = 2.0;
        specialty = 0.02;
        toolDamage = 0;
    };

    //! Ohne Auftragsvorgabe, gleiche Begruendung wie bei der Gabel: ohne
    //! diesen Schritt gaebe es die Imkerpfeife nicht, und die Ernte waere
    //! nicht durchfuehrbar.
    class PROCESS_BUILD_BEE_SMOKER
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_BUILD_BEE_SMOKER";
        toolGroups[] = {"HAND_TOOL"};
        baseDurationSec = 20.0;
        animationLength = 3.0;
        specialty = 0.02;
        toolDamage = 2;
    };

    // ------------------------------------------------------------------
    // 2. AUFSTELLEN - ein Eingang plus Werkzeuggruppe.
    //
    // Die Gegenform zu den vier oben: EIN Eingang, und die Werkzeuggruppe
    // belegt den zweiten der zwei Zutatenplaetze (01 V12). Ohne Werkzeug
    // waere ein Ein-Eingang-Transform bei Vanilla gar nicht registrierbar -
    // es gaebe nichts zum Kombinieren.
    // ------------------------------------------------------------------
    class PROCESS_RAISE_HIVE
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_RAISE_HIVE";
        toolGroups[] = {"HAND_TOOL"};
        baseDurationSec = 30.0;
        animationLength = 4.0;
        specialty = 0.03;
        toolDamage = 3;
    };

    // ------------------------------------------------------------------
    // 3. DER STOCK - die beiden Vorgaenge an der Station.
    // ------------------------------------------------------------------

    //! Die Antwort auf "wie werden Raehmchen voll".
    //!
    //! STATION_TIMED und nicht STATION_ACTION: ein Volk baut aus, waehrend
    //! der Spieler nicht da ist. Genau dafuer ist diese Ausfuehrungsform da
    //! (11 §7: "Spieler verlaesst den Server waehrend STATION_TIMED ->
    //! irrelevant, der Timer gehoert der Station").
    //!
    //! WIE DER SPIELER DEN FORTSCHRITT SIEHT: ueber dieselbe Anzeige, die
    //! Trockenrahmen und Raeucherschrank benutzen.
    //! ChefZ_ProcessingStation_Base synchronisiert Progress01 und den
    //! Ordinal des laufenden Prozesses selbst (11 §6). Dieser Slice baut
    //! dafuer KEIN eigenes HUD und kein eigenes Sync-Feld - es gibt beides
    //! schon, und ein zweites waere ein neues Core-System.
    //!
    //! 3600 Sekunden - eine Stunde je Raehmchen. Der Stock traegt vier
    //! Parallelplaetze (Stationsdatensatz), also vier Raehmchen in einer
    //! Stunde. Das ist die laengste Wartezeit des Projekts und soll es
    //! sein: Honig ist der einzige Suessstoff der Kette.
    //!
    //! KEIN requiresHeat. Bienen brauchen kein Feuer.
    class PROCESS_TEND_HIVE
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_TEND_HIVE";
        baseDurationSec = 3600.0;
        requiresHeat = 0;
    };

    //! Auftrag: "[Bienenstock oeffnen] -> (Smoker in der Hand haelt Schaden
    //! ab)" und "[Vollen Rahmen entnehmen]". Beides ist EIN Vorgang, und
    //! das ist er.
    //!
    //! ABWEICHUNG VOM AUFTRAG, offen benannt: die Imkerpfeife haelt hier
    //! keinen Schaden ab, sie ist VORAUSSETZUNG. Ohne sie erscheint die
    //! Aktion nicht.
    //!
    //! Der Grund ist kein Geschmack, sondern ein fehlender Angriffspunkt.
    //! Fuer "wer ohne Pfeife oeffnet, wird gestochen" braeuchte es einen
    //! Punkt im Ablauf, an dem BEIDES bekannt ist: der handelnde Spieler
    //! und was er in der Hand haelt. Den gibt es an einer ChefZ-Station
    //! nicht:
    //!   - ChefZ_ActionProcessAtStation.OnFinishProgressServer reicht an die
    //!     Station nur (ItemBase inHands, int actorId) weiter. actorId ist
    //!     PlayerIdentity.GetPlayerId(), eine Zahl ohne Rueckweg zum
    //!     PlayerBase - und bei LEEREN Haenden, also genau im Straffall, ist
    //!     inHands null. Es bleibt kein Zeiger auf den Spieler uebrig.
    //!   - Ein STATION_ACTION laeuft ausserdem gar nicht ueber
    //!     ChefZ_BeginJob, sondern ueber RunImmediate - ein Ueberschreiben
    //!     in der Stockklasse zuendete nie.
    //! Vanillas eigene Vorlage fuer so etwas ist
    //! CAContinuousMineWood.DamagePlayersHands() (Handschuhe federn ab,
    //! sonst Blutung). Sie sitzt in einer EIGENEN Actionkomponente. ChefZ
    //! nachzubauen hiesse, eine eigene Action neben
    //! ChefZ_ActionProcessAtStation zu stellen - ein zweiter Weg zur
    //! Station, den kein anderes Modul des Projekts geht, oder eine
    //! Aenderung im Core. Beides steht diesem Slice nicht zu.
    //!
    //! Die Werkzeuggruppe erreicht dasselbe Spielergebnis auf dem Weg, den
    //! das Projekt schon hat: ohne Pfeife kein Honig. Sie ist strenger als
    //! der Auftrag und nie irrefuehrend - 11 §7 verlangt bei fehlendem
    //! Werkzeug ausdruecklich "Action erscheint nicht" statt einer
    //! HUD-Meldung.
    //!
    //! STATION_ACTION und nicht STATION_TIMED: der Spieler steht am Stock
    //! und arbeitet. toolDamage = 2 - die Pfeife brennt aus.
    class PROCESS_HARVEST_HIVE
    {
        exec = "STATION_ACTION";
        displayName = "#STR_CHEFZ_PROC_HARVEST_HIVE";
        toolGroups[] = {"BEE_SMOKER"};
        baseDurationSec = 25.0;
        toolDamage = 2;
    };

    // ------------------------------------------------------------------
    // 4. ENTDECKELN - Auftrag: "Rahmen entdeckeln: Honeycomb_Frame_Full +
    //    Uncapping_Fork -> Frame_Ready_To_Spin".
    //
    // Woertlich die Form, die Vanillas RecipeBase traegt: ein Eingang, ein
    // Werkzeug. Der Auftrag beschreibt sie selbst als "via Crafting", und
    // genau das ist es - HANDCRAFT, keine Station.
    // ------------------------------------------------------------------
    class PROCESS_UNCAP_COMB
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_UNCAP_COMB";
        toolGroups[] = {"UNCAPPING_TOOL"};
        baseDurationSec = 18.0;
        animationLength = 2.0;
        specialty = 0.02;
        toolDamage = 1;
    };
};


//==============================================================================
// ### SLICE apiary ###   Die drei Werkzeuggruppen der Imkerei, Rang 1
//
// Rang 1, weil ActionCondition clientseitig laeuft (02 §2): der Spieler muss
// sehen, ob er den Schritt tun kann, bevor der Server etwas bestaetigt.
//
// Die Namen tragen bewusst KEIN ChefZ_-Praefix: eine Werkzeuggruppe ist ein
// Symbol in einem offenen Namensraum, keine Item-Klasse. Ein fremdes Modul
// darf derselben Gruppe eigene Klassen beisteuern - dafuer ist sie da.
//
// WARUM DIESES MODUL EINE EIGENE HOLZWERKZEUGGRUPPE FUEHRT und nicht
// METALWORK_TOOL aus ChefZ_Processing benutzt: ChefZ_Processing hat
// ChefZ_Farming in seinem requiredAddons. Die Gegenrichtung waere ein Zyklus
// in der Ladeordnung. Die Gruppen sind auch sachlich verschieden - Bretter
// nagelt man, Bleche biegt man.
//==============================================================================
class CfgChefZTools
{
    //! Werkzeug in der Hand - zum Aufstellen des Stocks
    //! (PROCESS_RAISE_HIVE) und zum Formen der Blechdose zur Imkerpfeife
    //! (PROCESS_BUILD_BEE_SMOKER).
    //!
    //! Der Name traegt bewusst KEIN Material: die Gruppe hiess im ersten
    //! Entwurf WOODWORK_TOOL und wurde umbenannt, als die Imkerpfeife
    //! dazukam - ein Hammer, der laut Gruppennamen nur Holz bearbeitet,
    //! aber eine Dose biegt, waere ein Name, der luegt.
    //!
    //! METALWORK_TOOL aus ChefZ_Processing waere fuer die Dose die
    //! naeherliegende Gruppe und ist trotzdem nicht benutzbar:
    //! ChefZ_Processing fuehrt ChefZ_Farming in seinem requiredAddons. Eine
    //! Gruppe aus einem Modul zu benutzen, das dieses hier voraussetzt,
    //! hiesse, sich auf ein PBO zu verlassen, das nicht geladen sein muss -
    //! und die Gruppe waere dann leer, ohne Fehlermeldung.
    //!
    //! Ausschliesslich Vanillaklassen: ChefZ fasst keine fremde config.cpp
    //! an, sondern nennt fremde Klassen in einer eigenen Gruppe (11 E8).
    //! Alle vier sind gewoehnliches Werkzeugloot - der Slice haengt an keinem
    //! einzelnen seltenen Fund.
    class HAND_TOOL
    {
        classes[] =
        {
            "Hammer",
            "Hatchet",
            "Pliers",
            "Screwdriver"
        };
        allowSubclasses = 1;
    };

    //! Werkzeug zum Entdeckeln. GENAU EIN Mitglied, und das ist Absicht -
    //! die Begruendung steht ausgeschrieben an ChefZ_UncappingFork.
    class UNCAPPING_TOOL
    {
        classes[] = {"ChefZ_UncappingFork"};
        allowSubclasses = 1;
    };

    //! Die Imkerpfeife. Auch hier genau ein Mitglied: Vanilla hat keine
    //! zweite Klasse, die Rauch in die Hand gibt (gesucht wurde nach "smok" -
    //! die einzigen Treffer sind Rauchgranaten). Sie ist ueber
    //! TR_BuildBeeSmoker herstellbar und damit erreichbar.
    class BEE_SMOKER
    {
        classes[] = {"ChefZ_BeeSmoker"};
        allowSubclasses = 1;
    };
};
