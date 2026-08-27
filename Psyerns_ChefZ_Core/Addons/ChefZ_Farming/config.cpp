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
            "ChefZ_PepperPlant", "ChefZ_PaprikaPlant",
            "ChefZ_ParsleySeeds", "ChefZ_DillSeeds", "ChefZ_ThymeSeeds",
            "ChefZ_RosemarySeeds", "ChefZ_WildGarlicSeeds",
            "ChefZ_PepperSeeds", "ChefZ_PaprikaSeeds",
            "ChefZ_Parsley", "ChefZ_Dill", "ChefZ_Thyme", "ChefZ_Rosemary",
            "ChefZ_WildGarlic", "ChefZ_PepperBerries", "ChefZ_Paprika"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // ChefZ_Core:          Skriptbasis ChefZ_Edible_Base.
        // DZ_Gear_Cultivation: SeedBase, PlantBase und ihre Proxy-Modelle.
        // DZ_Gear_Food:        Proxy-Modell des Korns.
        requiredAddons[] = {"DZ_Data", "DZ_Gear_Cultivation", "DZ_Gear_Food", "ChefZ_Core"};
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
    // KEIN class Food / FoodStages: rohes Gemuese ist Zutat. Stufen OHNE
    // Uebergaenge waeren die Falle aus 01 V4 (FoodStage.c:472 faellt auf
    // BURNED zurueck). class Nutrition ist dagegen PFLICHT - PlayerStomach.
    // InitData registriert nur Klassen mit Nutrition ODER Food (01 V7).
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
    };

    //==========================================================================
    // ### SLICE herbs ###   Production Map §21-§24, §15, §16
    //
    // Fuenf Kraeuter, dazu Pfeffer und Paprika: Pflanze -> Ernte -> (spaeter,
    // in ChefZ_Processing) Trockenrahmen und Moerser.
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

    class ChefZ_PaprikaPlant : ChefZ_HerbPlantBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PLANT_PAPRIKA";
        descriptionShort = "#STR_CHEFZ_PLANT_PAPRIKA_DESC";
        class Horticulture
        {
            GrowthStagesCount = 5;
            CropsCount = 3;
            CropsType = "ChefZ_Paprika";
        };
    };

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

    class ChefZ_PepperSeeds : ChefZ_HerbSeedsBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_PEPPER";
        descriptionShort = "#STR_CHEFZ_SEEDS_PEPPER_DESC";
        model = "\dz\gear\cultivation\pepper_seeds.p3d";
        class Horticulture
        {
            PlantType = "ChefZ_PepperPlant";
        };
    };

    class ChefZ_PaprikaSeeds : ChefZ_HerbSeedsBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SEEDS_PAPRIKA";
        descriptionShort = "#STR_CHEFZ_SEEDS_PAPRIKA_DESC";
        model = "\dz\gear\cultivation\pepper_seeds.p3d";
        class Horticulture
        {
            PlantType = "ChefZ_PaprikaPlant";
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

    // Frische Paprika ist Gemuese UND Ausgangsstoff des Paprikapulvers
    // (Production Map §15).
    class ChefZ_Paprika : ChefZ_FreshHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_PAPRIKA";
        descriptionShort = "#STR_CHEFZ_ITEM_PAPRIKA_DESC";
        model = "\dz\gear\food\pepper_green.p3d";
        weight = 120;
        itemSize[] = {2, 1};
        class Nutrition
        {
            fullnessIndex = 20;
            energy = 60;
            water = 45;
            nutritionalIndex = 35;
            toxicity = 0;
            digestibility = 1;
        };
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
};
