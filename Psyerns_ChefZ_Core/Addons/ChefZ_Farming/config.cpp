// ChefZ_Farming - Weizen, Gemuese und Kraeuter als FUNDPFLANZEN (Slice "grain").
//
// Quelle: Production Map §7 (Weizen-Produktionskette), §69/§73 (Klassenliste),
// DME-Plan §53 (Namenskonvention).
//
// PBO-Praefix: $PREFIX$ enthaelt "ChefZ_Farming". Die Wurzel jedes
// Laufzeitpfades ist dieses Praefix (Entwurf 02 §4.1, B4).
//
// ---------------------------------------------------------------------------
// FUNDPFLANZEN, KEIN ANBAU (Entscheidung vom 29.08.2026)
// ---------------------------------------------------------------------------
// Weizen, die vier Gemuese und die sechs Kraeuter werden GEFUNDEN, nicht
// gezogen - dasselbe Verhalten wie Vanillas Pilze: ein Item liegt in der
// Welt, wird aufgehoben, gegessen oder verarbeitet. Es gibt keine
// Pflanzenklasse, kein Saatgut, keine Wachstumsstufe und keinen Horticulture-
// Knoten mehr. Zwoelf Pflanzen mit je fuenf Wachstumsstufen haetten den
// Modellaufwand vervielfacht, und die Kette dahinter (mahlen, trocknen,
// moersern) ist die eigentliche Spielmechanik - nicht das Beet.
//
// Wo sie liegen, sagt die Servertypentabelle (types.xml / mapgroupproto),
// genau wie bei Pilzen. Das ist Betreibersache und kein Modulinhalt.
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
            "ChefZ_GrainFoodBase", "ChefZ_Wheat",
            // ### SLICE produce ###
            "ChefZ_VegetableFood_Base", 
            
            "ChefZ_Onion", "ChefZ_Garlic", "ChefZ_Carrot", "ChefZ_Cabbage",
            // ### SLICE herbs ###
            "ChefZ_FreshHerbBase",
            
            
            
            "ChefZ_Parsley", "ChefZ_Dill", "ChefZ_Thyme", "ChefZ_Rosemary",
            "ChefZ_WildGarlic", "ChefZ_PepperBerries",
            // ### SLICE apiary ###
            "ChefZ_Beehive", "ChefZ_BeehiveDouble", "ChefZ_BeehiveKit",
            "ChefZ_HoneycombFrame_Base",
            "ChefZ_HoneycombFrameEmpty",
            "ChefZ_HoneycombFrameFull", "ChefZ_HoneycombFrameUncapped",
            "ChefZ_UncappingFork", "ChefZ_BeeSmoker"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // ChefZ_Core:          Skriptbasis ChefZ_Edible_Base und - seit dem
        //                      Slice apiary - ChefZ_ProcessingStation_Base.
        // DZ_Gear_Cultivation: Proxy-Modelle einiger Fundpflanzen (tomato_seeds,
        //                      cannabis_seedman). Keine Vererbung mehr von
        //                      SeedBase oder PlantBase - Fundpflanzen.
        // DZ_Gear_Food:        Proxy-Modell des Korns und, fuer den Slice
        //                      apiary, food_can_open.p3d an der Imkerpfeife.
        // DZ_Gear_Camping:     wooden_case.p3d - Proxy von Bienenstock und
        //                      Bausatz (### SLICE apiary ###).
        // DZ_Gear_Tools:       Meat_Tenderizer.p3d - Proxy der
        //                      Entdeckelungsgabel (### SLICE apiary ###).
        // DZ_Gear_Consumables: birch_bark.p3d - Proxy der drei Raehmchen
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
    // Zwiebel, Knoblauch, Karotte und Kohl sind FUNDPFLANZEN wie Vanillas
    // Pilze (Kopf dieser Datei): kein Saatgut, keine Pflanze, kein Beet.
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

    // --- §17 Zwiebel ---------------------------------------------------------

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
    // Fuenf Kraeuter, dazu Pfeffer: Fund -> (spaeter, in ChefZ_Processing)
    // Trockenrahmen und Moerser. Paprika steht hier nicht mehr - sie ist
    // vollstaendig Vanilla (Vanilla-Audit §2).
    //
    // FUNDPFLANZEN wie Vanillas Pilze (Kopf dieser Datei): kein Saatgut,
    // keine Pflanze, kein Beet. SELTENHEIT (Production Map §21: Petersilie
    // haeufig ... Rosmarin selten) steuert das Weltvorkommen ueber die
    // Servertypen (types.xml / mapgroupproto) - eine Loot-Tabelle ist kein
    // Modulinhalt.
    //
    // class Nutrition ist PFLICHT (01 V7). Bewusst OHNE class Food /
    // FoodStages: frische Kraeuter kommen nicht in den Topf, sie kommen auf
    // den Trockenrahmen. Wer FoodStages ohne FoodStageTransitions deklariert,
    // baut die Falle aus 01 V4.
    //
    // PROXY-MODELLE, alle Vanilla, alle im Asset-Bedarf des Slice gemeldet.
    //==========================================================================

    // KEINE eigene Paprikapflanze (Vanilla-Audit §2). Vanilla schliesst den
    // Kreis bereits vollstaendig: PepperSeedsPack -> PepperSeeds -> Plant_Pepper
    // -> GreenBellPepper -> CutOutPepperSeeds -> PepperSeeds. Die frueheren
    // ChefZ_PaprikaPlant / ChefZ_PaprikaSeeds bauten dieselbe Kette nach, waren
    // aber unerreichbar: kein Datensatz im Projekt erzeugte je ChefZ_PaprikaSeeds.
    // ChefZ setzt jetzt an der Frucht an - GreenBellPepper traegt den
    // Zutaten-Datensatz (ChefZ_Ingredients/Config/Ingredients/VanillaProduce.json)
    // und ist Eingang von TR_PaprikaToDried.

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
    //   Bienenstock, doppelt   ChefZ_BeehiveDouble
    //   Beehive_Kit            ChefZ_BeehiveKit
    //   Honigwabe_Leer         ChefZ_HoneycombFrameEmpty
    //   Honigwabe_Voll         ChefZ_HoneycombFrameFull
    //   Frame_Ready_To_Spin    ChefZ_HoneycombFrameUncapped
    //   Uncapping_Fork         ChefZ_UncappingFork
    //   Smoker                 ChefZ_BeeSmoker
    //   Honey_Extractor        ChefZ_HoneyExtractor  (in ChefZ_Processing)
    //   Honey_Jar_Empty        ChefZ_EmptyJar        (ChefZ_Cooking)
    //   Honigglas befuellt     Honey                 (VANILLA)
    //
    // WAS VANILLA SCHON MITBRINGT UND DESHALB HIER FEHLT
    // --------------------------------------------------
    //   Honey            das gefuellte Honigglas. ChefZ_Vanilla_Assets §16
    //                    fuehrt es mit nominal 60 in Farm/Town/Village/School.
    //                    Es ist das ERGEBNIS der Kette, keine neue Klasse.
    //   WoodenPlank      die Bretter des Auftrags - und der Griff der Gabel.
    //   Nail             die Naegel des Auftrags - und die Zinken der Gabel.
    //                    "Nail" und nicht "Nails": die CONFIG-Klasse heisst
    //                    Nail (types.xml der COT-Mission fuehrt nur
    //                    name="Nail"; Skriptklasse 4_World/DayZ/Entities/
    //                    ItemBase/Nail.c:1). "Nails" ist eine reine
    //                    Skriptklasse ohne CfgVehicles-Eintrag
    //                    (Inventory_Base/Nail.c:1), und Slot_Material_Nails
    //                    ist ein Slotname. Das Serverlog hatte es gesagt:
    //                    'Die Klasse "Nails" existiert in keiner geladenen
    //                    Config'.
    //   TunaCan_Opened   der Blechkoerper der Imkerpfeife.
    //   Hammer/Hatchet/Pliers/Screwdriver   HAND_TOOL beim Aufstellen des
    //                    Stocks und beim Formen der Imkerpfeife.
    //
    // Alle Bau-Rezepte kommen bewusst mit GENAU DIESEN drei Vanillaklassen
    // aus. Der erste Entwurf benutzte zusaetzlich WoodenStick und Rag; beide
    // sind Stapel mit Menge, und ihr varQuantityMax liegt dem Projekt nicht
    // vor (die Item-Configs von DayZ fehlen, refindex fuehrt nur Namen). Eine
    // Mengenangabe darauf waere geraten gewesen. Fuer WoodenPlank und Nail
    // gibt es einen Beleg - siehe Config/Processing/README_Apiary.md.
    //
    // Vanilla hat KEINE Imkerpfeife, KEINE Entdeckelungsgabel, KEINEN
    // Bienenstock und KEINE Wabe - gesucht wurde in refindex/vanilla-classes.txt
    // und refindex/vanilla-scripts-classes.txt nach "bee", "honey", "smok" und
    // "fork". Der einzige Treffer auf "fork" ist Pitchfork, eine Mistgabel.
    // Diese fuenf Klassen bringt ChefZ deshalb selbst mit.
    //
    // DIE DREI RAEHMCHEN, und was zwischen ihnen passiert
    // ----------------------------------------------------
    // Der Auftrag nennt drei Zustaende, und es sind drei Klassen:
    //
    //   Empty     leer, vom Spieler gebaut, liegt im Stock. Traegt den
    //             steigenden Balken (varQuantity 0..100): das Volk fuellt
    //             die Raehmchen EINES NACH DEM ANDEREN, vier Stunden je
    //             Raehmchen. Bei 100 ersetzt der Stock es in seiner
    //             Cargo-Zelle durch Full.
    //   Full      voll und verdeckelt. Nur bei geoeffnetem Stock entnehmbar
    //             - und Oeffnen ist der Moment, in dem gestochen wird.
    //   Uncapped  entdeckelt, schleuderfertig (Frame_Ready_To_Spin). Traegt
    //             drei Glaeser Vorrat plus eine Reserve-Einheit
    //             (varQuantity 4..1, Begruendung an der Klasse).
    //
    // Der Balken ist Vanillas quantityBar - dieselbe Anzeige, mit der ein
    // Apfel beim Essen leerer wird, hier andersherum. Wie der Stock ihn
    // fortschreibt, steht in Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c.
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
    // schon gescheitert (das fruehere Schneidebrett in ChefZ_Processing).
    //
    // class Cargo IST der Innenraum - die Zargen. 10x9 sind neunzig Zellen
    // fuer zehn Raehmchen zu 2x3 - mit Reserve, nicht auf den Punkt: Vanilla
    // darf ein Item gedreht ablegen, und der Spieler darf Raehmchen im Gitter
    // verschieben; ein einziges versetztes Raehmchen liesse in einem Gitter
    // ohne Luft das zehnte nicht mehr hinein. Dass es nicht mehr als zehn
    // werden und nichts anderes hineinkommt, zaehlt das Skript ueber
    // CanReceiveItemIntoCargo, nicht das Gitter.
    //
    // lifetime deutlich ueber der Fuellzeit: zehn Raehmchen brauchen vierzig
    // Stunden Serverlaufzeit, und die CE-Lebensdauer laeuft in derselben Zeit
    // ab. Ein Stock, der frueher verschwaende als sein Honig fertig ist, waere
    // sinnlos. Sieben Tage geben dem Betreiber Luft; die types.xml des
    // Servers darf das ueberschreiben (siehe README_Apiary.md).
    //
    // DER FUELLSTAND IST KEIN STATIONSJOB. Ein Job kennt einen Fortschritt
    // je Job, nicht je Item, und er verbraucht am Ende seinen Eingang. Hier
    // soll jedes Raehmchen seinen eigenen, stetig steigenden Balken tragen -
    // das ist varQuantity am Raehmchen, fortgeschrieben von einem eigenen
    // Timer des Stocks (Skript). parallelSlots im Stationsdatensatz steht
    // deshalb auf 1: der einzige Stationsvorgang, PROCESS_HARVEST_HIVE, legt
    // nie einen Job an.
    //
    // KEIN Pot, KEIN Cauldron, KEIN FireplaceBase als Basis: alle drei sind
    // Vanillas Kochgeschirr bzw. Feuerstellen und wuerden den Stock in Vanillas
    // Kochkette ziehen (Begruendung ausgeschrieben an ChefZ_ButterChurn in
    // ChefZ_Processing). Inventory_Base haelt ihn heraus.
    //
    // KEIN ChefZ_HasHeat: Bienen brauchen kein Feuer. needsFuel bleibt false.
    //
    // DER DECKEL UND DER BIENENSTICH stehen nicht hier, sondern im Skript:
    // ChefZ_Beehive ueberschreibt ChefZ_OnStationActionFinished(), oeffnet
    // den Stock fuer zwei Minuten - erst dann laesst sich ein volles
    // Raehmchen herausnehmen, und nur ein volles - und laesst das Volk sich
    // wehren, wenn jemand ohne Imkerpfeife in der Hand oeffnet
    // (Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c). Das ist gewoehnliche
    // Vererbung - kein modded class, keine eigene Action, keine Zeile im
    // Core.
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
        lifetime = 604800;

        class Cargo
        {
            itemsCargoSize[] = {10, 9};
            openable = 0;
        };
    };

    //--------------------------------------------------------------------------
    // Die Doppelbeute: zwei Zargen uebereinander.
    //
    // Zwanzig Raehmchen, achtzig Stunden, sonst in allem der Stock - sie erbt
    // config UND Skript von ChefZ_Beehive und aendert nur Fassungsvermoegen,
    // Groesse, Gewicht und Lebensdauer. 10x15 sind zwanzig Raehmchen zu 2x3
    // mit derselben Reserve wie beim Stock; die Lebensdauer ist verdoppelt,
    // weil auch die Fuellzeit (achtzig Stunden) die doppelte ist.
    //
    // Sie entsteht aus ZWEI Bausaetzen (TR_ExtendBeehive), nicht aus einem
    // aufgestellten Stock plus Bausatz: ein Handwerksschritt verbraucht seine
    // Zutat samt Cargo, und ein bestueckter Stock verloere dabei seine
    // Raehmchen.
    //
    // PROXY: dieselbe Holzkiste wie der Stock. Eigenes Mesh mit zwei Zargen
    // ist gemeldet.
    //--------------------------------------------------------------------------
    class ChefZ_BeehiveDouble : ChefZ_Beehive
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BEEHIVE_DOUBLE";
        descriptionShort = "#STR_CHEFZ_ITEM_BEEHIVE_DOUBLE_DESC";
        itemSize[] = {6, 8};
        weight = 26000;
        lifetime = 1209600;

        class Cargo
        {
            itemsCargoSize[] = {10, 15};
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
    // Die drei Raehmchen.
    //
    // Eine gemeinsame Basis mit scope = 0: sie unterscheiden sich in Name,
    // Beschreibung, Gewicht und Mengenblock, in nichts sonst. Die Basis steht
    // einmal da, weil configcpp.mjs Klassennamen projektweit auf Eindeutigkeit
    // prueft und drei gleichlautende Bloecke drei Gelegenheiten waeren, sie
    // auseinanderlaufen zu lassen.
    //
    // canBeSplit = 0 an der Basis: zwei der drei Raehmchen tragen eine
    // varQuantity (Fuellgrad, Glaservorrat), und ohne dieses Verbot deutete
    // Vanilla die Menge als Stapel, den man teilen kann - ein halbes
    // Raehmchen gibt es nicht.
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
    // Es ist der beste im Projekt belegte Pfad, mehr nicht - und alle drei
    // Zustaende sehen damit gleich aus. Eigene Geometrie mit drei sichtbar
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
        canBeSplit = 0;
        lifetime = 43200;
        repairableWithKits[] = {};
    };

    //! Auftrag: "Honigwabe_Leer". Ergebnis von TR_BuildHoneycombFrame und
    //! das, was die Schleuder zurueckgibt, wenn ein entdeckeltes Raehmchen
    //! leergeschleudert ist. Damit ist die Kette ein Kreis, kein Strahl.
    //!
    //! DER BALKEN, der sich fuellt (Auftrag 5): varQuantity 0..100 mit
    //! quantityBar = 1 - dieselbe Anzeige wie beim Apfel, der beim Essen
    //! leerer wird, nur steigend. Der Stock zaehlt serverseitig per
    //! AddQuantity hoch (ItemBase.c:3413); die Engine registriert
    //! m_VarQuantity selbst zum Sync (ItemBase.c:254) und speichert und laedt
    //! sie mit dem Variablenblock (ItemBase.c:3045-3056, 3258-3261). Deshalb
    //! ueberlebt der Fuellstand den Serverneustart ohne eine Zeile eigener
    //! Persistenz. quantityShow = 0: der Balken genuegt, eine Prozentzahl am
    //! Raehmchen waere Laerm. Init 0, weil ein frisch gebautes Raehmchen leer
    //! ist - TR_BuildHoneycombFrame setzt deshalb KEINE quantity.
    class ChefZ_HoneycombFrameEmpty : ChefZ_HoneycombFrame_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COMBFRAME_EMPTY";
        descriptionShort = "#STR_CHEFZ_ITEM_COMBFRAME_EMPTY_DESC";
        weight = 400;
        varQuantityInit = 0;
        varQuantityMin = 0;
        varQuantityMax = 100;
        quantityBar = 1;
        quantityShow = 0;
    };

    //! Auftrag: "Honigwabe_Voll" / "Honeycomb_Frame_Full". Entsteht im
    //! Stock, wenn der Balken des Leerraehmchens voll ist, in derselben
    //! Cargo-Zelle. KEINE varQuantity: voll ist voll. Entnehmbar nur bei
    //! geoeffnetem Stock (Skript, CanReleaseCargo).
    class ChefZ_HoneycombFrameFull : ChefZ_HoneycombFrame_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COMBFRAME_FULL";
        descriptionShort = "#STR_CHEFZ_ITEM_COMBFRAME_FULL_DESC";
        weight = 2200;
    };

    //! Auftrag: "Frame_Ready_To_Spin". Eingang der Honigschleuder.
    //!
    //! varQuantity 4..1 = DREI GLAESER VORRAT plus eine Reserve-Einheit
    //! (Auftrag 4: ein Rahmen ergibt drei Glaeser). Die Schleuder zieht je
    //! Glas eine Einheit ab (Zutatendatensatz: unitsPerWholeItem 4, Verbrauch
    //! 1.0 je Durchlauf, Untergrenze 2.0); unterhalb von zwei gibt sie das
    //! Raehmchen leer zurueck.
    //!
    //! WARUM VIER UND NICHT DREI: der Core loescht ein Item, sobald ein
    //! Mengenabzug seine letzte Einheit traefe (ChefZ_SlotEvaluator.
    //! PlanAmountDraw setzt destroyWhole, der Applicator ruft Delete). Mit
    //! drei Einheiten waere der Rahmen nach dem dritten Glas weg - kein
    //! Leerrahmen, ein Brett und zwei Naegel je Durchlauf verloren. Die
    //! vierte Einheit wird nie gezogen; sie ist der Boden, auf dem der Rahmen
    //! die Schleuder ueberlebt. Init 4, weil ein frisch entdeckeltes Raehmchen
    //! voll ist - TR_UncapHoneycombFrame setzt deshalb KEINE quantity.
    //!
    //! quantityShow = 0: die Zahl "4" hiesse fuer den Spieler vier Glaeser,
    //! und das waere gelogen. Der Balken sinkt in Vierteln und reicht als
    //! Anzeige. Das Ergebnis, Honey, ist Vanilla und bekommt keine Menge
    //! gesetzt.
    class ChefZ_HoneycombFrameUncapped : ChefZ_HoneycombFrame_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COMBFRAME_UNCAPPED";
        descriptionShort = "#STR_CHEFZ_ITEM_COMBFRAME_UNCAPPED_DESC";
        weight = 2100;
        varQuantityInit = 4;
        varQuantityMin = 0;
        varQuantityMax = 4;
        quantityBar = 1;
        quantityShow = 0;
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
    // Derselbe Pfad, den ChefZ_PastaMachine traegt und den Asset-Backlog
    // §10.1 als den korrekten ausweist.
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
    //
    // SCHUTZ, NICHT VORAUSSETZUNG. Sie steht in keinem toolGroups mehr. Wer
    // sie beim Ernten in der Hand haelt, kommt ungestochen davon und die
    // Pfeife nimmt den Verschleiss (2.0 je Ernte, wie vorher als toolDamage);
    // wer sie nicht haelt, erntet trotzdem - und blutet. Die Regel steht an
    // ChefZ_Beehive.ChefZ_OnStationActionFinished() im Skript.
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
    // handcraftRecipeSlots = 7. Die Zahl ist eine RESERVIERUNG in Vanillas
    // Rezeptliste und muss VOR dem Laden feststehen; wird sie vergessen,
    // erscheint kein einziges der Rezepte, und zwar OHNE Fehlermeldung an der
    // Stelle, an der man sucht (Kopf von ChefZ_HandcraftBridge.c).
    //
    // Die sieben, einer je HANDCRAFT-Transform dieses Slice:
    //
    //   TR_BuildBeehiveKit      PROCESS_BUILD_HIVE_KIT
    //   TR_RaiseBeehive         PROCESS_RAISE_HIVE
    //   TR_ExtendBeehive        PROCESS_EXTEND_HIVE
    //   TR_BuildHoneycombFrame  PROCESS_BUILD_FRAME
    //   TR_BuildUncappingFork   PROCESS_BUILD_UNCAPPING_FORK
    //   TR_BuildBeeSmoker       PROCESS_BUILD_BEE_SMOKER
    //   TR_UncapHoneycombFrame  PROCESS_UNCAP_COMB
    //
    // Die beiden Stationsvorgaenge (PROCESS_HARVEST_HIVE, PROCESS_SPIN_HONEY)
    // brauchen KEINEN Platz - sie laufen ueber ChefZ_ActionProcessAtStation
    // und fassen Vanillas Rezeptliste nicht an: ChefZ_HandcraftBridge zieht
    // seine Liste ueber GetProcessesForExec(ChefZ_ProcessExec.HANDCRAFT), und
    // ein Stationsprozess ist dort nie dabei.
    //
    // Die beiden aelteren Knoten dieses Moduls bleiben bei 0: massgeblich ist
    // die projektweite SUMME, die ChefZ_HandcraftBridge.Reserve() ueber
    // ChefZ_ManifestReader.ReadHandcraftSlotTotal() liest - welcher Knoten sie
    // beisteuert, ist der Bruecke gleichgueltig. Diese Reservierung hier ist
    // die des Slice apiary und wird nicht mit fremdem Ueberschuss verrechnet;
    // parallel arbeitende Slices koennten ihn jederzeit verbrauchen.
    class ChefZ_Apiary
    {
        chefzApiVersion = 1;
        loadOrder = 217;
        handcraftRecipeSlots = 7;
        dataFiles[] =
        {
            "ChefZ_Farming/Config/Processing/Apiary_Ingredients.json",
            "ChefZ_Farming/Config/Processing/Apiary_Stations.json",
            "ChefZ_Farming/Config/Processing/Apiary_Crafts.json"
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
// Prozesse dieses Moduls, Rang 1. Die Samengewinnung gibt es nicht mehr:
// Gemuese sind Fundpflanzen, es gibt kein Saatgut.
//==============================================================================
class CfgChefZProcesses
{
    //--------------------------------------------------------------------------
    // ### SLICE apiary ###   Die acht Verben der Imkerei
    //
    // Sieben davon sind HANDCRAFT, eines ist eine Stationsaktion (Oeffnen des
    // Stocks). Das Schleudern steht in ChefZ_Processing bei seiner Station.
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
    // Solange sich ihre Eingaenge unterscheiden, ist das folgenlos. Hier ist
    // es NICHT folgenlos: Bausatz, Raehmchen UND
    // Entdeckelungsgabel entstehen alle drei aus WoodenPlank + Nail und
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

    //! Die Doppelbeute aus ZWEI Bausaetzen. Zwei Eingaenge, deshalb KEINE
    //! toolGroups - beide Zutatenplaetze sind belegt (01 V12), dieselbe
    //! Form wie PROCESS_BUILD_HIVE_KIT. Nicht Stock plus Bausatz: ein
    //! Handwerksschritt verbraucht die Zutat samt Cargo, ein bestueckter
    //! Stock verloere seine Raehmchen.
    class PROCESS_EXTEND_HIVE
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_EXTEND_HIVE";
        baseDurationSec = 30.0;
        animationLength = 4.0;
        specialty = 0.03;
        toolDamage = 0;
    };

    // ------------------------------------------------------------------
    // 3. DER STOCK - ein Vorgang an der Station: Oeffnen.
    //
    // Wie Raehmchen voll werden, ist KEIN Prozess. Das Volk fuellt sie
    // eines nach dem anderen, vier Stunden je Raehmchen, als varQuantity am
    // Leerraehmchen - fortgeschrieben von einem eigenen Timer des Stocks
    // (Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c). Ein Stationsjob
    // kennt einen Fortschritt je Job, nicht je Item, und verbraucht seinen
    // Eingang; hier soll jedes Raehmchen seinen eigenen Balken tragen.
    // ------------------------------------------------------------------

    //! Auftrag: "[Bienenstock oeffnen] -> (Smoker in der Hand haelt Schaden
    //! ab)". Die Entnahme des vollen Rahmens ist danach der gewoehnliche
    //! Inventar-Drag - und nur der volle laesst sich ziehen, nur solange
    //! der Deckel offen ist (Skript, CanReleaseCargo).
    //!
    //! DIESER PROZESS HAT ABSICHTLICH KEINEN TRANSFORM. Er verbraucht nichts
    //! und erzeugt nichts; er ist der Moment, in dem der Deckel abgeht.
    //! ChefZ_ActionProcessAtStation.IsProcessUsable() ueberspringt die
    //! Transformpruefung, wenn zu einem Prozess kein Transform bekannt ist
    //! (ChefZ_ActionProcessAtStation.c:321-324), RunImmediate meldet dann
    //! NO_MATCH (Z.627-630), und NotifyStation ruft den Haken trotzdem
    //! (Z.662, 687). Der Haken - ChefZ_Beehive.ChefZ_OnStationActionFinished
    //! - oeffnet den Deckel fuer zwei Minuten und loest den Stich aus.
    //! NO_MATCH ist hier der gewollte Ausgang, kein Fehler.
    //!
    //! KEINE toolGroups - und das ist der Kern des Auftrags, nicht eine
    //! Auslassung. Hier stand einmal toolGroups[] = {"BEE_SMOKER"}, weil die
    //! Imkerpfeife als PFLICHTWERKZEUG gefuehrt wurde: ohne sie erschien die
    //! Aktion nicht. Das war eine Notloesung fuer einen fehlenden
    //! Angriffspunkt - es gab im Verarbeitungspfad keine Stelle, an der der
    //! handelnde Spieler und sein Handinhalt gleichzeitig bekannt waren.
    //!
    //! Die Stelle gibt es jetzt: ChefZ_ProcessingStation_Base.
    //! ChefZ_OnStationActionFinished(PlayerBase, ItemBase, ChefZ_Sym, int).
    //! Die Pfeife ist damit vom Zwang zum SCHUTZ geworden - das Oeffnen
    //! gelingt auch ohne sie, es kostet dann nur Blut und Schock. Die Regel
    //! steht vollstaendig in Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c;
    //! die Werkzeuggruppe BEE_SMOKER bleibt bestehen und ist dort die
    //! Adresse, unter der die Pfeife erkannt wird.
    //!
    //! WAS DAS NICHT BERUEHRT: handcraftRecipeSlots. Vanillas Rezeptplaetze
    //! reserviert ChefZ_HandcraftBridge ausschliesslich fuer exec = HANDCRAFT
    //! (GetProcessesForExec(ChefZ_ProcessExec.HANDCRAFT)); dieser Prozess ist
    //! STATION_ACTION und belegt keinen Platz. Auch die 01-V12-Grenze "zwei
    //! Zutatenplaetze" gilt nur fuer HANDCRAFT - eine Station kennt sie
    //! nicht (11 E1).
    //!
    //! toolDamage = 0, und das ist zwingend, nicht kosmetisch.
    //! ChefZ_ActionProcessAtStation.ApplyToolDamage() beschaedigt
    //! action_data.m_MainItem - was auch immer in der Hand liegt, ohne jede
    //! Pruefung gegen toolGroups. Solange die Aktion ohne Pfeife gar nicht
    //! erschien, konnte dort nur die Pfeife liegen. Ohne Pflichtwerkzeug
    //! fraesse der Stock am Gewehr, an der Feldflasche oder am Kompass des
    //! Spielers. Der Verschleiss der Pfeife (unveraendert 2.0) wandert
    //! deshalb in den Haken, wo geprueft ist, dass es wirklich die Pfeife
    //! ist.
    //!
    //! Acht Sekunden: einen Deckel abnehmen dauert nicht laenger. Frueher
    //! standen hier 25, weil die Aktion auch die Ernte war - die macht jetzt
    //! der Inventar-Drag.
    class PROCESS_HARVEST_HIVE
    {
        exec = "STATION_ACTION";
        displayName = "#STR_CHEFZ_PROC_HARVEST_HIVE";
        baseDurationSec = 8.0;
        toolDamage = 0;
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
    //!
    //! DIESE GRUPPE STEHT IN KEINEM toolGroups MEHR, und sie bleibt trotzdem.
    //! Sie ist seit dem Wegfall des Pflichtwerkzeugs die Adresse, unter der
    //! ChefZ_Beehive.ChefZ_IsBeeSmoker() den Schutz erkennt
    //! (ChefZ_ToolRegistry.IsToolOfGroup). Ueber die Gruppe und nicht ueber
    //! einen festen Klassennamen, damit ein fremdes Modul eine eigene Pfeife
    //! eintragen kann und den Schutz damit geschenkt bekommt (11 E8). Ein
    //! Prozess muss eine Werkzeuggruppe nirgends nennen, damit sie existiert -
    //! ChefZ_ToolRegistry.Build() interniert jeden CfgChefZTools-Eintrag,
    //! unabhaengig davon, ob ihn ein Prozess fuehrt.
    class BEE_SMOKER
    {
        classes[] = {"ChefZ_BeeSmoker"};
        allowSubclasses = 1;
    };
};
