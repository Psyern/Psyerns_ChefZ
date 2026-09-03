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
// Weizen, die fuenf Gemuese und die fuenf Kraeuter werden GEFUNDEN, nicht
// gezogen - dasselbe Verhalten wie Vanillas Pilze: ein Item liegt in der
// Welt, wird aufgehoben, gegessen oder verarbeitet. Es gibt keine
// Pflanzenklasse, kein Saatgut, keine Wachstumsstufe und keinen Horticulture-
// Knoten mehr. Elf Pflanzen mit je fuenf Wachstumsstufen haetten den
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
            
            "ChefZ_Onion", "ChefZ_Garlic", "ChefZ_Carrot", "ChefZ_Cabbage", "ChefZ_Corn",
            "ChefZ_CornPlant",
            // ### SLICE herbs ###
            "ChefZ_FreshHerbBase",
            
            
            
            "ChefZ_Parsley", "ChefZ_Thyme", "ChefZ_Rosemary",
            "ChefZ_WildGarlic", "ChefZ_PepperBerries", "ChefZ_Chili",
            // ### SLICE apiary ###
            "ChefZ_Beehive", "ChefZ_BeehiveDouble", "ChefZ_BeehiveKit",
            // Die Projektionshuelle des Aufstellvorgangs (31.08.2026).
            "ChefZ_BeehivePlacing",
            "ChefZ_HoneycombFrame_Base",
            "ChefZ_HoneycombFrameEmpty",
            "ChefZ_HoneycombFrameFull", "ChefZ_HoneycombFrameUncapped",
            "ChefZ_UncappingFork", "ChefZ_BeeSmoker",
            "ChefZ_HandRake",
            // ### SLICE wildplants ###
            "ChefZ_WildPlant_Base",
            "ChefZ_WildCorn", "ChefZ_WildThyme", "ChefZ_WildRosemary", "ChefZ_WildParsley"
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
        requiredAddons[] = {"DZ_Data", "DZ_Gear_Cultivation", "DZ_Gear_Food", "DZ_Gear_Camping", "DZ_Gear_Tools", "DZ_Gear_Consumables", "ChefZ_Core", "ChefZ_Items", "ChefZ_Devices", "ChefZ_Plants", "ChefZ_Plants_Cultivation"};
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

//==============================================================================
// ### SLICE apiary ###   Die zwanzig Raehmchenslots des Bienenstocks
//                        (Umbau vom 31.08.2026, Testbefund Alex: "dort sollten
//                        slots sein")
//
// WARUM SLOTS UND NICHT MEHR CARGO
// --------------------------------
// Ein Cargo-Gitter kennt weder Klassen noch Stueckzahlen: es zaehlt Zellen.
// Deshalb musste das Skript bisher beides nachbilden - die Klassenpruefung in
// CanReceiveItemIntoCargo, die Obergrenze ueber eine eigene Zaehlschleife, und
// das Gitter brauchte Reserve (10x9 fuer zehn Raehmchen zu 2x3), damit ein
// gedrehtes Raehmchen das letzte nicht aussperrt. Ein SLOT kann all das von
// sich aus: er nimmt genau ein Item, und nur eines, dessen inventorySlot[] ihn
// nennt. Zehn Slots sind zehn Raehmchen - keine Reserve, keine Zaehlschleife,
// keine Zelle, in die etwas anderes rutschen koennte.
//
// Und der Spieler sieht die Beute so, wie eine Beute aussieht: eine Reihe
// benannter Plaetze, nicht ein Kistenboden.
//
// DIE NAMEN
// ---------
// ChefZ_Frame01..ChefZ_Frame20, mit dem Modulpraefix nach DME-Plan §53.
// Gegen Kollision geprueft (31.08.2026): weder Vanilla (scripts - 1.29/
// config.cpp, class CfgSlots) noch Terje, Expansion, COT oder irgendein
// anderes Repo unter "Mod Repositories" fuehrt einen Slot dieses Namens.
// Der Altbaum ChefZ/ChefZ_Core/slots/config.cpp fuehrt eigene Slots
// (ChefZ_Honeycomb_Frame01..20) - deren Namen werden BEWUSST NICHT
// uebernommen: das Addon ist eine Nur-Referenz-Lieferung, und zwei
// CfgSlots-Eintraege gleichen Namens waeren eine Kollision, sobald beide PBOs
// auf einem Server liegen.
//
// Die Engine findet einen Slot ueber "Slot_" + name (InventorySlots.c,
// GetSlotIdFromString: "searches for class entry Slot_##slot_name") - deshalb
// die doppelte Schreibweise aus Klassenname und name-Feld, genau wie in
// Vanillas eigenem CfgSlots (scripts - 1.29/config.cpp:680-705).
//
// ghostIcon fehlt bewusst: alle Vanilla-Icons sind Kleidungs- und
// Werkzeugsymbole, und ein Rucksacksymbol am Raehmchenplatz waere schlechter
// als gar keines. Ein eigenes Icon ist als Asset-Bedarf gemeldet.
//
// ZWANZIG UND NICHT ZWEIMAL ZEHN. Die Doppelbeute fasst zwanzig Raehmchen und
// bekommt dafuer die Slots 11..20 ZUSAETZLICH zu 01..10 - keinen zweiten
// Zehnersatz mit eigenen Namen. Der Grund ist die Vererbung: die Doppelbeute
// erbt Skript und Config vom Stock, und ChefZ_Beehive.ChefZ_FirstEmptyFrame()
// laeuft die Slots in EINER durchgehenden Reihenfolge 01..Kapazitaet ab.
// Zwei getrennte Namensraeume haetten zwei Suchschleifen gebraucht und die
// Raehmchen haetten zwischen beiden Beuten nicht mehr getauscht werden
// koennen.
//==============================================================================
class CfgSlots
{
    class Slot_ChefZ_Frame01 { name = "ChefZ_Frame01"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame02 { name = "ChefZ_Frame02"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame03 { name = "ChefZ_Frame03"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame04 { name = "ChefZ_Frame04"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame05 { name = "ChefZ_Frame05"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame06 { name = "ChefZ_Frame06"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame07 { name = "ChefZ_Frame07"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame08 { name = "ChefZ_Frame08"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame09 { name = "ChefZ_Frame09"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame10 { name = "ChefZ_Frame10"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    // 11..20 nur an der Doppelbeute (ChefZ_BeehiveDouble.attachments[]).
    class Slot_ChefZ_Frame11 { name = "ChefZ_Frame11"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame12 { name = "ChefZ_Frame12"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame13 { name = "ChefZ_Frame13"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame14 { name = "ChefZ_Frame14"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame15 { name = "ChefZ_Frame15"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame16 { name = "ChefZ_Frame16"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame17 { name = "ChefZ_Frame17"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame18 { name = "ChefZ_Frame18"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame19 { name = "ChefZ_Frame19"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
    class Slot_ChefZ_Frame20 { name = "ChefZ_Frame20"; displayName = "#STR_CHEFZ_SLOT_FRAME"; };
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

        // FULLNESS-RESCALE (31.08.2026) - siehe den Banner "DER
        // EINHEITENFEHLER AM fullnessIndex" weiter unten bei ChefZ_Onion.
        // Kurz: Magenvolumen = fullnessIndex * gegessene Quantity
        // (PlayerStomach.c:86, KEIN Teiler 100). Massgeblich fuer ein ganzes
        // Item ist also fullnessIndex * varQuantityMax.
        //
        //   Rechnung: 250 (Zielvolumen) / 1000 (varQuantityMax von
        //             ChefZ_Wheat) = 0.25
        //
        // Vorher stand hier 20. Das ergab 20 * 1000 = 20000 Volumen fuer ein
        // volles Korn-Item - das Zehnfache der Kotzschwelle
        // (PlayerConstants.VOMIT_THRESHOLD = 2000, PlayerConstants.c:208).
        //
        // ACHTUNG, FREMDE MODULE ERBEN DIESEN WERT. Die Ableitungen dieser
        // Basis liegen in zwei Mengenwelten:
        //   varQuantityMax 500..1000 (ChefZ_Flour, ChefZ_RawPasta,
        //     ChefZ_DriedPasta) - fuer sie ist 0.25 richtig (125..250).
        //   varQuantityMax 1 (ChefZ_Dough, ChefZ_Bread, ChefZ_Flatbread in
        //     ChefZ_Baking) - fuer sie ist 0.25 viel zu wenig; sie brauchen
        //     einen EIGENEN Nutrition-Block mit fullnessIndex ~300
        //     (Zielvolumen 300 / qtyMax 1). Das ist als Uebergabe an die
        //     Eigentuemer jener Module gemeldet; hier wird bewusst der
        //     UNGEFAEHRLICHE Wert gesetzt - zu wenig Saettigung ist laestig,
        //     zu viel laesst den Spieler beim ersten Bissen erbrechen.
        class Nutrition
        {
            fullnessIndex = 0.25;
            energy = 200;
            water = 10;
            nutritionalIndex = 20;
            toxicity = 0;
            digestibility = 0;
        };

        class Food
        {
            // nutrition_properties[0] IST der fullnessIndex der Stufe und
            // SCHLAEGT class Nutrition, sobald das Item eine FoodStage traegt
            // (Edible_Base.GetFoodTotalVolume, Edible_Base.c:391-405, ruft
            // FoodStage.GetFullnessIndex; FoodStage.c:314-317 liest Index 0).
            // Die Stufenwerte sind deshalb mit demselben Faktor skaliert
            // (0.25 / 20 = 0.0125), die Verhaeltnisse der Stufen zueinander
            // bleiben unveraendert.
            class FoodStages
            {
                //------------------------------------------------------------
                // DIE ROHSTUFE, die hier bis zum 31.08.2026 FEHLTE - und
                // deren Fehlen die ganze Getreidekette im Rohzustand
                // NAEHRWERTLOS machte.
                //
                // Die Kette, Schritt fuer Schritt:
                //   1. Diese Basis fuehrt einen class Food-Block, also traegt
                //      jedes abgeleitete Item eine FoodStage.
                //   2. Edible_Base.GetFoodTotalVolume (Edible_Base.c:391-397)
                //      nimmt bei vorhandener FoodStage IMMER den Stufenwert -
                //      class Nutrition wird dann gar nicht mehr gelesen.
                //      GetFoodEnergy (:406-419) und GetFoodWater (:421-434)
                //      machen dasselbe.
                //   3. FoodStage.GetNutritionPropertyFromIndex
                //      (FoodStage.c:262-263) gibt 0 zurueck, wenn die Stufe
                //      im Stufenbestand der Klasse fehlt.
                //   4. FoodStageType.RAW = 1 ist der Vorgabezustand
                //      (FoodStage.c:5, "//default").
                //
                // Ergebnis vor dieser Zeile: rohes ChefZ_Wheat, ChefZ_Flour,
                // ChefZ_Dough, ChefZ_RawPasta und ChefZ_DriedPasta lieferten
                // fullness 0, energy 0 UND water 0 - der class Nutrition-Block
                // darueber war fuer sie tote Konfiguration. Nur Gebackenes kam
                // je an seine Zahlen.
                //
                // Die Werte sind deshalb WOERTLICH aus class Nutrition
                // gespiegelt, nicht neu erfunden: sie machen bekannt, was
                // ohnehin dastand. Reihenfolge nach FoodStage.c:314-332 -
                // fullness, energy, water, nutritionalIndex, toxicity, agents,
                // digestibility.
                //
                // Die Gemuesebasis macht es seit jeher richtig
                // (ChefZ_VegetableFood_Base fuehrt eine Raw-Stufe); hier war
                // es eine Auslassung, kein Entwurf.
                //------------------------------------------------------------
                class Raw
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {0.25, 200.0, 10.0, 20.0, 0.0, 0.0, 0.0};
                    cooking_properties[] = {0.0, 0.0, 0.0};
                };
                class Baked
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    // 25.0 * 0.0125 = 0.31
                    nutrition_properties[] = {0.31, 300.0, 10.0, 25.0, 0.0, 0.0, 0.0};
                    cooking_properties[] = {100.0, 40.0, 200.0};
                };
                class Burned
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    // 5.0 * 0.0125 = 0.06
                    nutrition_properties[] = {0.06, 20.0, 0.0, 0.0, 5.0, 0.0, 0.0};
                    cooking_properties[] = {200.0, 60.0, 250.0};
                };
                class Rotten
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    // 5.0 * 0.0125 = 0.06
                    nutrition_properties[] = {0.06, 10.0, 0.0, 0.0, 20.0, 0.0, 0.0};
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
    // Zwiebel, Knoblauch, Karotte, Kohl und Mais sind FUNDPFLANZEN wie Vanillas
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
        // Zwiebel, Knoblauch, Karotte, Kohl und Mais liegen zu weit auseinander, als
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

    //==========================================================================
    // DER EINHEITENFEHLER AM fullnessIndex (Rescale vom 31.08.2026)
    //==========================================================================
    //
    // WIE DIE ENGINE RECHNET, woertlich:
    //
    //     volume = m_Profile.GetFullnessIndex() * m_Amount;
    //     -- scripts - 1.29, 4_World/DayZ/Classes/PlayerStomach.c:86
    //
    // m_Amount ist die GEGESSENE QUANTITY, nicht ein Prozentsatz. Es gibt
    // KEINEN Teiler 100 - anders als bei energy und water, die die Zeile
    // darueber ausdruecklich durch 100 teilt (PlayerStomach.c:92-93). Ein
    // fullnessIndex ist also kein "Index", sondern ein Faktor je
    // Mengeneinheit.
    //
    // Die Schwellen, gegen die das Ergebnis laeuft:
    //   2000  Erbrechen  - PlayerConstants.VOMIT_THRESHOLD (PlayerConstants.c:208)
    //   1000  "Stuffed"  - PlayerConstants.BT_STOMACH_VOLUME_LVL3 (:200)
    //     25  ein grosser Bissen - UAQuantityConsumed.EAT_BIG (ActionConstants.c:9)
    //
    // MASSGEBLICH IST DESHALB fullnessIndex * varQuantityMax - das Volumen
    // eines ganzen Items. Fuer jede Klasse steht die Rechnung
    // "Zielvolumen / qtyMax" als Kommentar am Wert.
    //
    // WARUM DIE ZAHLEN HIER SO GROSS AUSSEHEN. Vanillas eigenes Band liegt
    // bei 0,75..2,5 (belegt an fremdem Content, der dieselbe Engine benutzt:
    // DayZExpansion/Objects/Gear/Consumables/config.cpp:152 fuehrt Brot mit
    // fullnessIndex 2 bei varQuantityMax 125 - Volumen 250). Dieses Band gilt
    // fuer Items, deren Quantity in GRAMM gefuehrt wird. ChefZ-Gemuese und
    // -Kraeuter sind STUECKWARE mit varQuantityMax = 1 (die Zutatenrechnung
    // haengt daran: unitsPerWholeItem 1, ChefZ_FactCollector rechnet
    // quantity / quantityMax * unitsPerWholeItem). Bei qtyMax = 1 IST der
    // fullnessIndex das Volumen - deshalb dreistellige Werte, die dasselbe
    // aussagen wie Vanillas 2 bei 125 Gramm. Die Quantity wird NICHT
    // angefasst; sie umzustellen hiesse, jede Zutatenmenge des Projekts
    // nachzuziehen.
    //
    // DIE RELATIVE ORDNUNG DER ALTEN WERTE BLEIBT ERHALTEN:
    //   Pfefferbeeren 4 < Kraeuter 5 < Knoblauch 8 < Zwiebel 25 <
    //   Karotte 30 < Kohl 45 < Mais 60
    //   ->  50 < 55 < 70 < 120 < 140 < 180 < 220
    //
    // UND DIE GARSTUFEN WERDEN MITSKALIERT. nutrition_properties[0] ist der
    // fullnessIndex der Stufe und SCHLAEGT class Nutrition, sobald das Item
    // eine FoodStage traegt: Edible_Base.GetFoodTotalVolume
    // (Edible_Base.c:391-405) fragt zuerst FoodStage.GetFullnessIndex, und
    // die liest Index 0 des Stufenfeldes (FoodStage.c:314-317). Ein Rescale
    // nur am Nutrition-Block waere an gekochtem Gemuese wirkungslos. Jede
    // Stufe wird mit demselben Faktor multipliziert; die Verhaeltnisse der
    // Stufen zueinander bleiben unveraendert.
    //
    // NICHT MITSKALIERT wird das "stomach"-Feld der Registry-Deltas. Es
    // fliesst zur Laufzeit nirgends ein: ChefZ_NutritionDef sagt im
    // Dateikopf woertlich "Was hier steht, wird NIE an ein Item geschrieben
    // und NIE beim Verzehr angewandt" - es ist die Sollrechnung des
    // Startaudits (13 E1). Ein Mitskalieren dort wuerde die Auditzahlen von
    // den Configzahlen entkoppeln, ohne im Spiel irgendetwas zu bewirken.
    //==========================================================================

    // --- §17 Zwiebel ---------------------------------------------------------

    class ChefZ_Onion : ChefZ_VegetableFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_ONION";
        descriptionShort = "#STR_CHEFZ_ITEM_ONION_DESC";
        model = "\ChefZ\ChefZ_Plants\models\redonion.p3d";   // EIGENES MODELL (30.08.2026): redonion
        weight = 160;
        class Nutrition
        {
            // 120 (Zielvolumen) / 1 (varQuantityMax) = 120. Faktor 4.8
            // gegenueber der alten 25 (PlayerStomach.c:86).
            fullnessIndex = 120;
            energy = 90;
            water = 55;
            nutritionalIndex = 30;
            toxicity = 0;
            digestibility = 1;
        };

        // Stufen-Naehrwerte (01 V7). Rohwerte = class Nutrition; Gebacken
        // trocknet aus und verdichtet, Gekocht zieht Wasser und verliert
        // Vitamine, Verbrannt und Verdorben sind Verlust.
        //
        // Index 0 mit Faktor 4.8 nachgezogen (FoodStage.c:314-317):
        // 25->120, 22->106, 24->115, 6->29, 6->29.
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {120, 90, 55, 30, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {106, 105, 25, 32, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {115, 95, 63, 26, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {29, 14, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {29, 14, 11, 0, 15, 0, 1}; };
            };
        };
    };

    // --- §18 Knoblauch -------------------------------------------------------

    class ChefZ_Garlic : ChefZ_VegetableFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_GARLIC";
        descriptionShort = "#STR_CHEFZ_ITEM_GARLIC_DESC";
        model = "\ChefZ\ChefZ_Plants\models\garlic.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        weight = 60;
        class Nutrition
        {
            // 70 (Zielvolumen) / 1 (varQuantityMax) = 70. Faktor 8.75
            // gegenueber der alten 8 (PlayerStomach.c:86).
            fullnessIndex = 70;
            energy = 40;
            water = 15;
            nutritionalIndex = 40;
            toxicity = 0;
            digestibility = 1;
        };

        // Stufen-Naehrwerte (01 V7). Rohwerte = class Nutrition; Gebacken
        // trocknet aus und verdichtet, Gekocht zieht Wasser und verliert
        // Vitamine, Verbrannt und Verdorben sind Verlust.
        //
        // Index 0 mit Faktor 8.75 nachgezogen (FoodStage.c:314-317):
        // 8->70, 7->61, 8->70, 2->18, 2->18.
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {70, 40, 15, 40, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {61, 46, 7, 42, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {70, 42, 17, 34, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {18, 6, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {18, 6, 3, 0, 15, 0, 1}; };
            };
        };
    };

    // --- §19 Karotte ---------------------------------------------------------

    class ChefZ_Carrot : ChefZ_VegetableFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CARROT";
        descriptionShort = "#STR_CHEFZ_ITEM_CARROT_DESC";
        // EIGENES MODELL (29.08.2026) statt der geerbten Zucchini.
        model = "\ChefZ\ChefZ_Plants\models\carrot.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        weight = 120;
        class Nutrition
        {
            // 140 (Zielvolumen) / 1 (varQuantityMax) = 140. Faktor 4.667
            // gegenueber der alten 30 (PlayerStomach.c:86).
            fullnessIndex = 140;
            energy = 100;
            water = 60;
            nutritionalIndex = 45;
            toxicity = 0;
            digestibility = 1;
        };

        // Stufen-Naehrwerte (01 V7). Rohwerte = class Nutrition; Gebacken
        // trocknet aus und verdichtet, Gekocht zieht Wasser und verliert
        // Vitamine, Verbrannt und Verdorben sind Verlust.
        //
        // Index 0 mit Faktor 4.667 nachgezogen (FoodStage.c:314-317):
        // 30->140, 27->126, 29->135, 8->37, 8->37.
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {140, 100, 60, 45, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {126, 115, 27, 47, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {135, 105, 69, 38, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {37, 15, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {37, 15, 12, 0, 15, 0, 1}; };
            };
        };
    };

    // --- Mais (29.08.2026, loest Dill ab) --------------------------------------
    //
    // Seit 30.08.2026 ANBAUBAR: die Lieferung c09900f bringt Kolben, Pflanze
    // (7 Wachstumsstufen, 2 Kolben Ertrag) und Textur. Der Kolben selbst ist
    // das Saatgut - GardenBase liest "Horticulture PlantType" aus der Config
    // des gepflanzten Items (GardenBase.c:386), eine SeedBase-Ableitung ist
    // dafuer nicht noetig; der Kolben bleibt damit essbare Zutat. Staerkehaltig,
    // deshalb energiereicher und saettigender als Wurzelgemuese, aber trocken.
    // Referenz ist Vanillas Kartoffel (180 / 45 / 40): ein Kolben traegt mehr
    // Masse, aber nicht das Doppelte an Saettigung.
    //
    // PROXY: zucchini.p3d der Basis - laenglich, einteilig, derzeit von keiner
    // sichtbaren Klasse belegt. Eigenes Mesh ist gemeldet (Asset-ToDo §4).
    //
    // Zweites Leben: an der Getreidemuehle wird der Kolben zu Mehl
    // (TR_CornToFlour in ChefZ_Processing). Die Kategorie GRAIN dafuer steht
    // in CfgChefZIngredients unten.
    class ChefZ_Corn : ChefZ_VegetableFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CORN";
        descriptionShort = "#STR_CHEFZ_ITEM_CORN_DESC";
        model = "\ChefZ\ChefZ_Plants\models\corn_cob.p3d";   // EIGENES MODELL (30.08.2026)
        weight = 250;

        // Der Kolben ist das Saatgut (siehe Bannerkommentar oben).
        class Horticulture
        {
            PlantType = "ChefZ_CornPlant";
        };
        class Nutrition
        {
            // 220 (Zielvolumen) / 1 (varQuantityMax) = 220. Faktor 3.667
            // gegenueber der alten 60 (PlayerStomach.c:86). Der Kolben bleibt
            // das saettigendste Fundgut dieses Moduls - die Ordnung der alten
            // Werte ist erhalten.
            fullnessIndex = 220;
            energy = 180;
            water = 40;
            nutritionalIndex = 40;
            toxicity = 0;
            digestibility = 1;
        };

        // Stufen-Naehrwerte nach demselben Muster wie die Karotte: Gebacken
        // trocknet aus und verdichtet, Gekocht zieht Wasser und verliert
        // Vitamine, Verbrannt und Verdorben sind Verlust.
        //
        // Index 0 mit Faktor 3.667 nachgezogen (FoodStage.c:314-317):
        // 60->220, 54->198, 58->213, 15->55, 15->55.
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {220, 180, 40, 40, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {198, 205, 18, 42, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {213, 190, 46, 34, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {55, 27, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {55, 27, 8, 0, 15, 0, 1}; };
            };
        };
    };

    // --- §20 Kohl ------------------------------------------------------------

    class ChefZ_Cabbage : ChefZ_VegetableFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CABBAGE";
        descriptionShort = "#STR_CHEFZ_ITEM_CABBAGE_DESC";
        model = "\ChefZ\ChefZ_Plants\models\cabbage.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        weight = 900;
        itemSize[] = {2, 2};
        class Nutrition
        {
            // 180 (Zielvolumen) / 1 (varQuantityMax) = 180. Faktor 4.0
            // gegenueber der alten 45 (PlayerStomach.c:86).
            fullnessIndex = 180;
            energy = 110;
            water = 80;
            nutritionalIndex = 40;
            toxicity = 0;
            digestibility = 1;
        };

        // Stufen-Naehrwerte (01 V7). Rohwerte = class Nutrition; Gebacken
        // trocknet aus und verdichtet, Gekocht zieht Wasser und verliert
        // Vitamine, Verbrannt und Verdorben sind Verlust.
        //
        // Index 0 mit Faktor 4.0 nachgezogen (FoodStage.c:314-317):
        // 45->180, 40->160, 43->172, 11->44, 11->44.
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {180, 110, 80, 40, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {160, 125, 36, 42, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {172, 115, 92, 34, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {44, 17, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {44, 17, 16, 0, 15, 0, 1}; };
            };
        };
    };

    //==========================================================================
    // ### SLICE herbs ###   Production Map §21-§24, §15, §16
    //
    // Vier Kraeuter, dazu Pfeffer: Fund -> (spaeter, in ChefZ_Processing)
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

        // KEINE FoodStages an dieser Basis (siehe Bannerkommentar oben) -
        // hier gilt deshalb class Nutrition unmittelbar, und der Rescale
        // erschoepft sich in dieser einen Zahl.
        class Nutrition
        {
            // 55 (Zielvolumen) / 1 (varQuantityMax) = 55. Faktor 11
            // gegenueber der alten 5 (PlayerStomach.c:86). Kraeuter bleiben
            // das leichteste Fundgut nach den Pfefferbeeren.
            fullnessIndex = 55;
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
        model = "\ChefZ\ChefZ_Plants\models\parsley.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
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
        model = "\ChefZ\ChefZ_Plants\models\rosmary.p3d";   // EIGENES MODELL (30.08.2026); Dateiname der Lieferung ohne e
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
            // 50 (Zielvolumen) / 1 (varQuantityMax) = 50. Faktor 12.5
            // gegenueber der alten 4 (PlayerStomach.c:86). Der kleinste Wert
            // des Moduls - so war es vorher, so bleibt es.
            fullnessIndex = 50;
            energy = 12;
            water = 8;
            nutritionalIndex = 10;
            toxicity = 0;
            digestibility = 1;
        };
    };

    // Chili: zweiter Rohstoff der Schaerfe neben den Pfefferbeeren, und wie
    // diese eine FUNDPFLANZE (§3 der Asset-Liste: gefunden, nicht gezogen -
    // kein Saatgut, keine Wachstumsstufe). Kategorie SPICE und nicht
    // VEGETABLE: das Chili wuerzt, es saettigt nicht. Damit greift kein
    // Gemueseslot darauf zu und ein Chili kann in keinem Rezept eine Zwiebel
    // vertreten - dieselbe Trennung, die die Pfefferbeeren tragen.
    //
    // PROXY, kein eigenes Mesh: Sambucus_nigra.p3d ist im Modul bereits an
    // ChefZ_PepperBerries erprobt und damit ein nachgewiesen ladbarer Pfad.
    // dz/gear/food/pepper_green.p3d waere das bessere Bild, steht im Projekt
    // aber nur in einem Kommentar (die geloeschte ChefZ_Paprika) und in keiner
    // aktiven model=-Zeile - unbestaetigt wird er hier nicht gesetzt. Der
    // Tausch ist eine Zeile, sobald das Modell geliefert oder der Pfad
    // verifiziert ist. Zwei Klassen auf einem Proxy = P2 nach der Regel der
    // Asset-Liste (P1 erst ab drei).
    class ChefZ_Chili : ChefZ_FreshHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHILI";
        descriptionShort = "#STR_CHEFZ_ITEM_CHILI_DESC";
        model = "\dz\gear\food\Sambucus_nigra.p3d";
        class Nutrition
        {
            // Zwischen Pfefferbeere (50) und Kraut (55): eine Schote hat mehr
            // Substanz als eine Beere, bleibt aber Wuerzgut. Herleitung wie an
            // ChefZ_FreshHerbBase - Zielvolumen / varQuantityMax (= 1).
            fullnessIndex = 52;
            energy = 14;
            water = 14;
            nutritionalIndex = 18;
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
    //             Raehmchenslot durch Full.
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
    // voraus, in dem Raehmchen liegen.
    //
    // SEIT DEM 31.08.2026 IST DIESER INNENRAUM EIN SATZ ATTACHMENT-SLOTS,
    // KEIN CARGO (Testbefund Alex: "dort sollten slots sein!!"). Die zehn
    // Zargenplaetze heissen ChefZ_Frame01..ChefZ_Frame10 und stehen oben in
    // class CfgSlots; die vollstaendige Begruendung steht dort. Kurz: ein
    // Slot nimmt genau ein Raehmchen und nur ein Raehmchen, waehrend ein
    // Cargo-Gitter Zellen zaehlt und die Klassen- wie die Stueckzahlgrenze
    // dem Skript ueberlassen musste.
    //
    // class Cargo ist damit ERSATZLOS ENTFALLEN. Der Stationsteil leidet
    // nicht darunter: ChefZ_FactCollector.CollectFromCargo kehrt bei einem
    // Behaelter ohne Cargo mit leerem Schnappschuss zurueck (Z.195-197), und
    // die beiden Vorgaenge dieser Station - PROCESS_HARVEST_HIVE und
    // PROCESS_PACK_HIVE - haben ohnehin keinen Transform und damit keine
    // Zutat.
    //
    // SPIELSTAND: Raehmchen, die in einem gespeicherten Stock im CARGO
    // liegen, findet die Engine nach diesem Umbau nicht mehr wieder - dort
    // ist kein Cargo mehr. Der Punkt steht in README_Apiary.md unter
    // "Spielstand".
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
        // EIGENES MODELL (29.08.2026). Die Datei heisst "beekeeper", nicht
        // "beehive" - die Namen der Lieferung sind vertauscht, die Masse
        // entscheiden: beekeeper.p3d ist 0,59 m breit und 1,0 m hoch, die
        // einzargige Beute; beehive.p3d ist 1,65 m breit, die Doppelbeute.
        // Textur steckt im Modell (chefz\chefz_devices\data\beekeeper_co.paa).
        model = "\ChefZ\ChefZ_Devices\models\beekeeper.p3d";
        rotationFlags = 2;
        itemSize[] = {6, 5};
        weight = 14000;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        lifetime = 604800;

        // HOLOGRAMM-MATERIAL des Aufstellvorgangs (### 31.08.2026 ###).
        // Hologram.RefreshVisual() (Hologram.c:1554-1557) liest beide
        // Schluessel an der PROJEKTIONSKLASSE und setzt daraus einen
        // Materialpfad. Sie stehen hier, weil ChefZ_BeehivePlacing sie erbt.
        //
        // BEIDE LEER, und das ist kein Vergessen: ein Geisterschimmer braucht
        // ein .rvmat, das zum Modell gehoert, und das Beutenmodell der
        // Lieferung bringt keines mit. Leer ist der belegte Weg fuer genau
        // diesen Fall - DayZExpansion/Objects/Basebuilding/Safes/config.cpp:
        // 121-122 macht es an seinen Tresoren genauso. Der Spieler sieht dann
        // die Beute in normaler Textur schweben, statt gar nichts. Ein
        // eigenes Hologrammaterial ist als Asset-Bedarf gemeldet.
        hologramMaterial = "";
        hologramMaterialPath = "";

        // DIE ZEHN RAEHMCHENPLAETZE (### 31.08.2026 ###, loest class Cargo ab).
        // Reihenfolge = Fuellreihenfolge: ChefZ_Beehive.ChefZ_FirstEmptyFrame()
        // laeuft 01..ChefZ_FrameCapacity() ab und fuellt das erste leere.
        attachments[] =
        {
            "ChefZ_Frame01", "ChefZ_Frame02", "ChefZ_Frame03", "ChefZ_Frame04",
            "ChefZ_Frame05", "ChefZ_Frame06", "ChefZ_Frame07", "ChefZ_Frame08",
            "ChefZ_Frame09", "ChefZ_Frame10"
        };
    };

    //--------------------------------------------------------------------------
    // Die Projektion des Aufstellvorgangs (### 31.08.2026 ###).
    //
    // WOZU SIE DA IST: Vanillas Hologramm erzeugt beim Platzieren ein ECHTES
    // Objekt der Projektionsklasse (Hologram.c:113-121, CreateObjectEx) und
    // haengt es dem Spieler vor die Nase. Waere das ChefZ_Beehive selbst,
    // entstuende bei jedem Aufstellversuch eine vollwertige Station mit
    // Fuelltimer und zwanzig Slots, nur um sie gleich wieder wegzuwerfen.
    //
    // Deshalb eine eigene, leere Huelle: dasselbe Modell, dieselbe Silhouette
    // fuer die Kollisionspruefung, aber ohne Slots, ohne Station, ohne Skript.
    // Das ist Vanillas eigenes Muster - Hologram.GetProjectionName()
    // (Hologram.c:239-243) haengt an jeden Bausatz ein "Placing" an; der
    // Bausatz nennt diese Klasse ausdruecklich ueber projectionTypename
    // (Hologram.c:104-109), damit der Name nicht aus einer Zeichenkette
    // zusammengesetzt werden muss.
    //
    // scope = 0: sie ist nie Loot, nie handelbar, nie im Inventar.
    //--------------------------------------------------------------------------
    class ChefZ_BeehivePlacing : Inventory_Base
    {
        scope = 0;
        displayName = "#STR_CHEFZ_ITEM_BEEHIVE";
        descriptionShort = "#STR_CHEFZ_ITEM_BEEHIVE_DESC";
        model = "\ChefZ\ChefZ_Devices\models\beekeeper.p3d";
        rotationFlags = 2;
        itemSize[] = {6, 5};
        weight = 14000;
        absorbency = 0.0;
        canBeDigged = 0;
        hologramMaterial = "";
        hologramMaterialPath = "";
    };

    //--------------------------------------------------------------------------
    // Die Doppelbeute: zwei Zargen uebereinander.
    //
    // Zwanzig Raehmchen, achtzig Stunden, sonst in allem der Stock - sie erbt
    // config UND Skript von ChefZ_Beehive und aendert nur Fassungsvermoegen,
    // Groesse, Gewicht und Lebensdauer. Die Lebensdauer ist verdoppelt, weil
    // auch die Fuellzeit (achtzig Stunden) die doppelte ist.
    //
    // Ihre attachments[]-Liste wiederholt die zehn Slots des Stocks und
    // haengt zehn weitere an (### 31.08.2026 ###). Eine Ueberschreibung
    // ERSETZT das geerbte Feld vollstaendig - die ersten zehn muessen deshalb
    // noch einmal dastehen, sonst haette die Doppelbeute nur die Plaetze
    // 11..20. Warum nicht ein zweiter, eigener Zehnersatz: siehe class
    // CfgSlots oben, letzter Absatz.
    //
    // Sie entsteht aus ZWEI Bausaetzen (TR_ExtendBeehive), nicht aus einem
    // aufgestellten Stock plus Bausatz: ein Handwerksschritt verbraucht seine
    // Zutat samt allem, was an ihr haengt, und ein bestueckter Stock verloere seine
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
        // Die breite Zweizargen-Beute (1,65 m) - siehe Anmerkung am Stock.
        model = "\ChefZ\ChefZ_Devices\models\beehive.p3d";
        itemSize[] = {6, 8};
        weight = 26000;
        lifetime = 1209600;

        attachments[] =
        {
            "ChefZ_Frame01", "ChefZ_Frame02", "ChefZ_Frame03", "ChefZ_Frame04",
            "ChefZ_Frame05", "ChefZ_Frame06", "ChefZ_Frame07", "ChefZ_Frame08",
            "ChefZ_Frame09", "ChefZ_Frame10",
            "ChefZ_Frame11", "ChefZ_Frame12", "ChefZ_Frame13", "ChefZ_Frame14",
            "ChefZ_Frame15", "ChefZ_Frame16", "ChefZ_Frame17", "ChefZ_Frame18",
            "ChefZ_Frame19", "ChefZ_Frame20"
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
    // ZWEI WEGE ZUM AUFGESTELLTEN STOCK (### 31.08.2026 ###, Testbefund Alex:
    // "das Kit soll platzierbar sein wie Vanilla-Kits").
    //
    //   1. PLATZIEREN wie ein Vanilla-Bausatz: Kit in die Hand, Hologramm
    //      anwerfen, hinstellen. Neu.
    //   2. PROCESS_RAISE_HIVE, der gewoehnliche Handwerksschritt mit einem
    //      Werkzeug der Gruppe HAND_TOOL. BLEIBT unveraendert bestehen - er
    //      ist der Weg, der auch ohne freie Flaeche vor dem Spieler
    //      funktioniert, und die uebrigen Stationen des Projekts entstehen
    //      genauso.
    //
    // Hier stand vorher "KEIN Hologramm-Deploy: das waere ein neues System".
    // Das war nicht falsch, nur zu vorsichtig: ein neues SYSTEM ist es nicht.
    // Vanilla bringt alles mit, und der Bausatz muss nur vier Aussagen
    // machen - drei davon in dieser Config, eine im Skript:
    //
    //   itemBehaviour       = 2   welche Aufstellanimation. 0 schwer,
    //                             1 einhaendig, 2 zweihaendig
    //                             (ItemBase.c:65, ausgewertet in
    //                             ActionDeployObject.SetupAnimation,
    //                             ActionDeployObject.c:306-326). Ein Bausatz
    //                             von 6 kg und 3x2 wird zweihaendig getragen.
    //   projectionTypename        WAS im Hologramm schwebt
    //                             (Hologram.c:104-109). Ohne diesen Eintrag
    //                             haengt Vanilla an einen Bausatz die
    //                             Zeichenkette "Placing" an
    //                             (Hologram.c:239-243) und suchte nach
    //                             "ChefZ_BeehiveKitPlacing" - ein Name, der
    //                             nur aus einer Rechenregel entstuende.
    //                             Ausgeschrieben ist er nachschlagbar.
    //   hologramMaterial/-Path    stehen an der Projektionsklasse, siehe dort.
    //
    //   Skript (ChefZ_Apiary.c):
    //     IsDeployable()          true - sonst bietet ActionDeployObject
    //                             nichts an (Vorbild HescoBox.c:225-228).
    //     IsBasebuildingKit()     true - DAS ist der Schalter, der den
    //                             Bausatz nach dem Aufstellen verbraucht:
    //                             ActionDeployObject.OnEndServer
    //                             (ActionDeployObject.c:230-233) loescht
    //                             genau dann das Item in der Hand. Er sorgt
    //                             ausserdem dafuer, dass der Bausatz waehrend
    //                             des Aufstellens IN DER HAND bleibt und
    //                             nicht selbst an die Zielstelle wandert
    //                             (ActionDeployBase.c:191, :208).
    //     SetActions()            ActionTogglePlaceObject + ActionDeployObject
    //                             (woertlich KitBase.c:146-152).
    //     OnPlacementComplete()   erzeugt den ChefZ_Beehive an Position und
    //                             Ausrichtung des Hologramms - woertlich
    //                             FenceKit.c:19-33 und TotemKit.c:32-48.
    //
    // KEINE Ableitung von KitBase, obwohl das naheliegt: KitBase haengt sich
    // in EEInit ein Seil an (KitBase.c:116-122, CreateAttachment("Rope")) und
    // schaltet in UpdateVisuals die Modellselektionen "Inventory" und
    // "Placing" (KitBase.c:104-114). Beides setzt ein Modell mit genau diesen
    // Teilen voraus; wooden_case.p3d hat sie nicht, und einen Rope-Slot hat
    // dieser Bausatz auch nicht. Uebernommen wird deshalb nur, was ohne
    // Modellzusagen auskommt.
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

        // Aufstellen (### 31.08.2026 ###) - siehe den Bannerkommentar oben.
        itemBehaviour = 2;
        projectionTypename = "ChefZ_BeehivePlacing";
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
        // EIGENES MODELL (29.08.2026): der gefuellte Rahmen. Voll und
        // entdeckelt erben ihn; der Leerrahmen bekommt wooden_frame.p3d.
        model = "\ChefZ\ChefZ_Items\models\honeycomb_frame.p3d";
        itemSize[] = {2, 3};
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        canBeSplit = 0;
        lifetime = 43200;
        repairableWithKits[] = {};

        // IN WELCHE PLAETZE EIN RAEHMCHEN PASST (### 31.08.2026 ###). Ohne
        // dieses Feld nimmt kein Slot es an - die Engine prueft beide
        // Richtungen: attachments[] am Stock sagt, welche Plaetze es gibt,
        // inventorySlot[] am Raehmchen sagt, in welche es darf.
        //
        // Alle ZWANZIG stehen hier, nicht nur zehn: dasselbe Raehmchen soll
        // sich zwischen Stock (01..10) und Doppelbeute (01..20) hin- und
        // herlegen lassen. An der Basis und nicht an jeder der drei Klassen -
        // leer, voll und entdeckelt passen in dieselben Plaetze. Dass ein
        // ENTDECKELTES Raehmchen nicht in den Stock zurueckdarf, ist eine
        // Spielregel und steht deshalb im Skript
        // (ChefZ_Beehive.CanReceiveAttachment), nicht in der Config: ein
        // Slot kann "nur leer oder voll" nicht ausdruecken.
        inventorySlot[] =
        {
            "ChefZ_Frame01", "ChefZ_Frame02", "ChefZ_Frame03", "ChefZ_Frame04",
            "ChefZ_Frame05", "ChefZ_Frame06", "ChefZ_Frame07", "ChefZ_Frame08",
            "ChefZ_Frame09", "ChefZ_Frame10",
            "ChefZ_Frame11", "ChefZ_Frame12", "ChefZ_Frame13", "ChefZ_Frame14",
            "ChefZ_Frame15", "ChefZ_Frame16", "ChefZ_Frame17", "ChefZ_Frame18",
            "ChefZ_Frame19", "ChefZ_Frame20"
        };
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
        // Der leere Holzrahmen ohne Wabe.
        model = "\ChefZ\ChefZ_Items\models\wooden_frame.p3d";
        weight = 400;
        varQuantityInit = 0;
        varQuantityMin = 0;
        varQuantityMax = 100;
        quantityBar = 1;
        quantityShow = 0;
    };

    //! Auftrag: "Honigwabe_Voll" / "Honeycomb_Frame_Full". Entsteht im
    //! Stock, wenn der Balken des Leerraehmchens voll ist, in derselben
    //! Raehmchenslot. KEINE varQuantity: voll ist voll. Entnehmbar nur bei
    //! geoeffnetem Stock (Skript, CanReleaseAttachment).
    class ChefZ_HoneycombFrameFull : ChefZ_HoneycombFrame_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COMBFRAME_FULL";
        descriptionShort = "#STR_CHEFZ_ITEM_COMBFRAME_FULL_DESC";
        weight = 2200;
    };

    //! Auftrag: "Frame_Ready_To_Spin". Eingang der Honigschleuder.
    //!
    //! VIER GLAESER JE RAHMEN, UND DER RAHMEN KOMMT LEER ZURUECK
    //! (### 31.08.2026 ###, Testbefund Alex: es sollen vier sein, nicht drei).
    //!
    //! varQuantity 5..1, unitsPerWholeItem 5 - VIER GLAESER VORRAT PLUS EINE
    //! RESERVE-EINHEIT. Die Schleuder zieht je Durchlauf 1.0 ab und verlangt
    //! dafuer mindestens 2.0 (Zutatendatensatz und TR_SpinHoney): die Zuege
    //! geschehen bei 5, 4, 3 und 2 - vier Glaeser. Bei Reststand 1
    //! unterschreitet der Rahmen die Schwelle, und das Skript der Schleuder
    //! wandelt ihn in seinem Slot zu ChefZ_HoneycombFrameEmpty zurueck. Die
    //! Kette ist damit ein KREIS und kein Strahl.
    //!
    //! VORHER standen hier 4 Einheiten. Dasselbe Muster eine Stufe tiefer -
    //! Zuege bei 4, 3 und 2, also nur DREI Glaeser. Geaendert hat sich einzig
    //! die Stufenhoehe, nicht die Bauform.
    //!
    //! WARUM DIE RESERVE-EINHEIT BLEIBT und "Verbrauch bis 0" ausdruecklich
    //! VERWORFEN wurde (Abstimmung mit dem Slice processing, 31.08.2026):
    //! ChefZ_SlotEvaluator.PlanAmountDraw (Z.369-376) setzt destroyWhole,
    //! sobald ein Abzug die LETZTE Einheit eines Items traefe, und der
    //! Applicator loescht das Item dann, statt es auf 0 zu setzen. Ein Rahmen,
    //! der bis 0 gezogen wird, ist am Ende ZERSTOERT - es kaeme kein
    //! Leerrahmen zurueck, und genau der ist gefordert. Die fuenfte Einheit
    //! wird nie gezogen; sie ist der Boden, auf dem der Rahmen die Schleuder
    //! ueberlebt.
    //!
    //! Init 5, weil ein frisch entdeckeltes Raehmchen voll ist -
    //! TR_UncapHoneycombFrame setzt deshalb KEINE quantity.
    //!
    //! quantityShow = 0: die Zahl "5" hiesse fuer den Spieler fuenf Glaeser,
    //! und das waere gelogen - es sind vier. Der Balken sinkt in Fuenfteln und
    //! reicht als Anzeige. Das Ergebnis, Honey, ist Vanilla und bekommt keine
    //! Menge gesetzt.
    class ChefZ_HoneycombFrameUncapped : ChefZ_HoneycombFrame_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COMBFRAME_UNCAPPED";
        descriptionShort = "#STR_CHEFZ_ITEM_COMBFRAME_UNCAPPED_DESC";
        weight = 2100;
        varQuantityInit = 5;
        varQuantityMin = 0;
        varQuantityMax = 5;
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
        // EIGENES MODELL (29.08.2026), 0,29 x 0,37 m - eine echte Pfeife.
        model = "\ChefZ\ChefZ_Items\models\beesmoker.p3d";
        rotationFlags = 17;
        itemSize[] = {2, 3};
        weight = 900;
        repairableWithKits[] = {};
        lifetime = 43200;

        // BRENNSTOFF (29.08.2026): varQuantity 0..100 ist die Rindenfuellung.
        // Leer geliefert; TR_FillBeeSmoker (Pfeife + 2 Rinde) macht sie voll,
        // ein Feuerzeug oder Streichholz zuendet sie an (ChefZ_BeeSmoker.
        // CanBeIgnitedBy), und brennend sinkt der Balken in zehn Minuten auf
        // null. Nur eine BRENNENDE Pfeife beruhigt das Volk - eine kalte ist
        // eine Blechdose. quantityShow = 0: der Balken genuegt.
        varQuantityInit = 0;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 0;
        quantityBar = 1;
        quantityShow = 0;
    };

    //--------------------------------------------------------------------------
    // Der Handrechen (Lieferung c09900f). Gartengeraet; noch ohne eigenen
    // Prozess - er existiert, damit das gelieferte Modell im Spiel ist und
    // ein spaeterer Beet-Slice ihn als Werkzeug fassen kann.
    //--------------------------------------------------------------------------
    class ChefZ_HandRake : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HANDRAKE";
        descriptionShort = "#STR_CHEFZ_ITEM_HANDRAKE_DESC";
        model = "\ChefZ\ChefZ_Items\models\handrake.p3d";   // EIGENES MODELL (30.08.2026)
        rotationFlags = 17;
        itemSize[] = {3, 1};
        weight = 700;
        lifetime = 43200;
        repairableWithKits[] = {};
    };

    //--------------------------------------------------------------------------
    // Die Maispflanze im Beet (Lieferung c09900f). Vanillas Anbau uebernimmt
    // alles: GardenBase erzeugt sie als Attachment aus dem PlantType des
    // gepflanzten Kolbens (GardenBase.c:484), PlantBase laesst sie wachsen.
    // 7 Wachstumsstufen sind der Wert der Lieferung und bleiben.
    //
    // ------------------------------------------------------------------------
    // CropsCount 2 -> 4 (B-6, Balance-Review 31.08.2026)
    // ------------------------------------------------------------------------
    // DER BEFUND: Seit es Wildmais gibt, war das Beet vollstaendig entwertet.
    // Eine Wildpflanze gibt im Mittel 1,35 Kolben und kostet nichts als fuenf
    // Sekunden; eine Beetpflanze gab 2 Kolben, verbrauchte davon aber einen
    // als Saatgut - NETTO also 1 Kolben, nach Pflanzen, Giessen und Warten.
    // Ein Wildfund entsprach damit 1,35 Beetpflanzen netto, und wer je ein
    // Beet anlegte, tat es aus Nostalgie.
    //
    // MIT 4: netto +3 Kolben je Pflanze, ein 9er-Beet traegt +27. Das Beet ist
    // damit die SKALIERBARE Quelle (Arbeit rein, Menge raus), der Wildwuchs
    // die SOFORTQUELLE (nichts rein, wenig raus). Beide haben wieder einen
    // Platz.
    //
    // 4 liegt in Vanillas eigenem Band: Plant_Potato, Plant_Tomato und
    // Plant_Pepper fuehren CropsCount 3 bis 5. Die Zahl ist damit keine
    // Ausnahme, sondern die Mitte.
    //
    // ------------------------------------------------------------------------
    // BEWUSSTE ABWEICHUNG VON DER ANTI-RECYCLING-REGEL (Production Map §22)
    // ------------------------------------------------------------------------
    // Der Kolben ist sein EIGENES Saatgut (class Horticulture an ChefZ_Corn -
    // GardenBase liest "Horticulture PlantType" aus der Config des gepflanzten
    // Items, GardenBase.c:386). Es gibt also keinen externen Eingang: ein
    // Kolben hinein, vier heraus. Das Beet ist ein VERVIERFACHER ohne Kosten
    // ausser Zeit und Wasser - vorher war es ein Verdoppler.
    //
    // Das ist eine Schleife, und §22 verbietet Schleifen. Sie steht trotzdem,
    // weil Vanilla bei Kartoffel und Tomate exakt dieselbe Schleife faehrt und
    // ChefZ-Mais sonst die einzige Pflanze im Spiel waere, die sich nicht
    // vermehrt. Die Begrenzung ist die des Beets selbst: Plaetze, Wasser,
    // Wachstumszeit und die Lebensdauer der Kolben.
    //
    // ENTSCHEIDUNG 31.08.2026, GATE-REVIEW VORBEHALTEN. Wer sie zurueckdreht,
    // aendert genau diese eine Zahl - und muss dann erklaeren, wozu ein Beet
    // noch da ist.
    //--------------------------------------------------------------------------
    class ChefZ_CornPlant : PlantBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CORNPLANT";
        descriptionShort = "#STR_CHEFZ_ITEM_CORNPLANT_DESC";
        model = "\ChefZ\ChefZ_Plants\cultivation\models\corn_plant.p3d";
        class Horticulture
        {
            // 6 STUFEN, WEIL DAS MESH SECHS TRAEGT (Lieferung a78a247, 01.09.2026).
            // PlantBase.c:485 waechst bis m_PlantStateIndex == GrowthStagesCount - 2
            // und SetDry/SetSpoiled erhoeht danach genau einmal - die hoechste je
            // gezeigte Selektion ist damit plantStage_(GrowthStagesCount - 1).
            // cultivation/models/model.cfg definiert plantStage_01 bis _05, also
            // ist 6 der Hoechstwert ohne Verweis auf eine fehlende Selektion.
            // Die 7 hier stammte vom alten Mesh, das plantStage_06 noch hatte.
            GrowthStagesCount = 6;
            CropsCount = 4;
            CropsType = "ChefZ_Corn";
        };
    };

    //==========================================================================
    // ### SLICE wildplants ###   Die vier Fundpflanzen der Wildnis
    //
    // Spec: Psyerns_ChefZ_Docs/ChefZ_Wildwuchs_Spawn_Plan.md (freigegeben
    // 31.08.2026). Sie loest den Gate-2-Befund G2-B9 - Mais, Thymian,
    // Rosmarin und Petersilie hatten bis dahin keine einzige Quelle in der
    // Welt.
    //
    // WAS SIE SIND: stehende Weltobjekte, die die CE spielerzentriert
    // verteilt (position=player, wie Vanillas Pilze), mit einer Ernteaktion
    // und ohne jede Inventarbeziehung. Wer sie erntet, bekommt den Ertrag vor
    // die Fuesse gelegt; die Pflanze verschwindet.
    //
    // WARUM STATIONEN UND NICHT PlantBase: PlantBase haengt am Gartenbeet
    // (GardenBase erzeugt sie als Attachment, GardenBase.c:484) und bringt
    // Wachstum, Bewaesserung und Schaedlinge mit - alles Dinge, die eine
    // Wildpflanze nicht hat. Vanillas Busch-Ernte (ActionPickBerry) ist seit
    // 1.29 toter Code. Eine Mini-Station nach dem Muster des Bienenstocks ist
    // der EINZIGE Weg, der ohne eine Zeile Core-Code auskommt (Regel 4) -
    // die Ernte ist der Stationsvorgang PROCESS_HARVEST_WILD, und was dabei
    // geschieht, steht im Haken ChefZ_OnStationActionFinished
    // (Scripts/4_World/ChefZ/Farming/ChefZ_WildPlants.c).
    //
    // KEIN class Cargo und KEIN Transform. Der Vorgang veraendert nicht den
    // Inhalt der Station, sondern die Station selbst - dieselbe Bauform wie
    // PROCESS_PACK_HIVE. Die drei Belege dafuer, dass ein Transform hier
    // ueberhaupt nicht ginge, stehen im Kopf von ChefZ_WildPlants.c.
    //
    // NICHT AUFNEHMBAR. canBeDigged = 0 unten leistet das NICHT (der
    // Schluessel betreibt vergrabene Verstecke); es leistet die
    // Skriptueberschreibung IsTakeable() -> false nach dem Vorbild von
    // GardenPlot.c:113-131. Beides steht da, weil beides gemeint ist.
    //
    // lifetime 900 wie die Pilze (db/types.xml, AgaricusMushroom Z.586). Die
    // types.xml des Servers ueberschreibt das; der Wert hier ist der, mit dem
    // eine per Admin gespawnte Pflanze lebt.
    //==========================================================================

    //--------------------------------------------------------------------------
    // Die gemeinsame Basis. scope = 0 - sie ist eine BASIS, kein Fund.
    //--------------------------------------------------------------------------
    class ChefZ_WildPlant_Base : Inventory_Base
    {
        scope = 0;
        // Bewusst KEIN Modell an der Basis: jede der vier traegt ein anderes,
        // und ein geerbtes Vorgabemodell waere die stille Rueckfallebene,
        // wenn eine Erbin ihres vergisst.
        rotationFlags = 2;
        itemSize[] = {3, 3};
        weight = 400;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        lifetime = 900;
    };

    //--------------------------------------------------------------------------
    // Mais. Die einzige der vier mit einem eigenen, stehenden Modell - und die
    // einzige, die in Gruppen von 1 bis 3 waechst (Auftrag).
    //
    // DAS MODELL HAT SECHS WUCHSSTUFEN UEBEREINANDER (Befund Asset-Tracker,
    // 31.08.2026). corn_plant.p3d haengt an PlantBaseSkeleton;
    // ChefZ_Plants/models/model.cfg fuehrt vierzehn Animationen vom Typ
    // "hide" mit source "user" - Pile_01/02, PlantStage_01..06 und deren
    // _crops. Eine solche Auswahl ist SICHTBAR, solange ihre Phase 0 ist.
    // Sichtbar geschaltet werden sie in Vanilla ausschliesslich von
    // PlantBase.UpdatePlant() (scripts - 1.29, PlantBase.c:448-479, ueber
    // ShowSelection/HideSelection). Eine Wildpflanze ist kein PlantBase und
    // ruft das nie.
    //
    // class AnimationSources unten ist die Antwort darauf, und sie ist die
    // richtige Stelle: initPhase setzt die Phase beim Erzeugen des Modells,
    // ohne jedes Skript und auf jedem Client, der das Objekt hereinstreamt.
    // Die Form ist belegt an DayZExpansion/Objects/Airdrop/config.cpp:23-35
    // und .../SupplyCrates/config.cpp:66-74. Sie ist zugleich VORAUSSETZUNG
    // fuer den Skriptweg: EntityAI.HideAllSelections traegt den Hinweis
    // "These selections must also be defined in the entity's config class in
    // 'AnimationSources'" (EntityAI.c:3354).
    //
    //   initPhase = 1  verborgen (SetAnimationPhase(..., 1), EntityAI.c:3360)
    //   initPhase = 0  sichtbar
    //
    // Sichtbar bleibt genau die reife Stufe samt ihren Kolben. animPeriod ist
    // ueberall 0.01: die Phase soll SOFORT stehen und nicht hineinblenden -
    // dieselbe Groessenordnung, die Expansion an seinem Airdrop-Fahrwerk
    // benutzt.
    //
    // ChefZ_WildCorn.ChefZ_ApplyModelStage() im Skript setzt dieselben
    // vierzehn Phasen beim Erscheinen noch einmal. Die Doppelung ist Absicht;
    // die Begruendung steht dort.
    //--------------------------------------------------------------------------
    class ChefZ_WildCorn : ChefZ_WildPlant_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_WILDCORN";
        descriptionShort = "#STR_CHEFZ_ITEM_WILDCORN_DESC";
        model = "\ChefZ\ChefZ_Plants\cultivation\models\corn_plant.p3d";
        itemSize[] = {4, 4};
        weight = 900;

        class AnimationSources
        {
            class Pile_01           { source = "user"; animPeriod = 0.01; initPhase = 1; };
            class Pile_02           { source = "user"; animPeriod = 0.01; initPhase = 1; };
            class PlantStage_01     { source = "user"; animPeriod = 0.01; initPhase = 1; };
            class PlantStage_02     { source = "user"; animPeriod = 0.01; initPhase = 1; };
            class PlantStage_03     { source = "user"; animPeriod = 0.01; initPhase = 1; };
            class PlantStage_04     { source = "user"; animPeriod = 0.01; initPhase = 1; };
            class PlantStage_05     { source = "user"; animPeriod = 0.01; initPhase = 1; };
            // Die reife Stufe - die einzige, die stehen bleibt.
            class PlantStage_06     { source = "user"; animPeriod = 0.01; initPhase = 0; };
            class PlantStage_01_crops { source = "user"; animPeriod = 0.01; initPhase = 1; };
            class PlantStage_02_crops { source = "user"; animPeriod = 0.01; initPhase = 1; };
            class PlantStage_03_crops { source = "user"; animPeriod = 0.01; initPhase = 1; };
            class PlantStage_04_crops { source = "user"; animPeriod = 0.01; initPhase = 1; };
            class PlantStage_05_crops { source = "user"; animPeriod = 0.01; initPhase = 1; };
            // Die Kolben der reifen Stufe. Ohne sie waere es eine Maispflanze,
            // an der die Ernteaktion luegt.
            class PlantStage_06_crops { source = "user"; animPeriod = 0.01; initPhase = 0; };
        };
    };

    //==========================================================================
    // Die drei Kraeuter - PROXY-MODELLE, und warum ausgerechnet diese
    //
    // Es gibt kein stehendes Kraeuterbueschel im Projekt und keines in
    // Vanilla, das ChefZ ohne neues requiredAddons erreicht (Spec Kap. 2:
    // "Es existiert genau EIN stehendes Pflanzenmodell: corn_plant.p3d").
    // Drei Bueschel plus ein Thymian-Item-Mesh sind als Asset-Bedarf
    // gemeldet.
    //
    // NICHT genommen wurden Vanillas Clutter-Modelle (\dz\plants\clutter\
    // c_*.p3d), obwohl sie am huebschesten aussaehen: Clutter ist
    // Bodenbewuchs und traegt moeglicherweise KEINE Geometry-LOD. Ohne die
    // trifft der Raycast der Aktionszielsuche nichts, und die Ernteaktion
    // erschiene nie - ein Fehler, der wie "die Pflanze ist kaputt" aussieht
    // und in keinem Log steht. Der Versuch steht als Gate-Experiment im
    // Backlog; hier stehen die sicheren Meshes.
    //
    // Sicher heisst: Meshes, die im Projekt bereits als Item in der Hand und
    // am Boden benutzt werden, also nachweislich eine Geometrie haben.
    // Sie sind zu klein fuer eine stehende Pflanze - das ist der bewusst in
    // Kauf genommene Preis, und er kostet keine Spielbarkeit, nur Optik.
    //==========================================================================

    //! Thymian. Das einzige der vier Kraeuter ohne eigenes Mesh im Projekt
    //! (Spec Kap. 2) - deshalb Vanillas Pflanzenmaterial, dasselbe, das
    //! ChefZ_FreshHerbBase traegt. Haesslich, aber getroffen.
    class ChefZ_WildThyme : ChefZ_WildPlant_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_WILDTHYME";
        descriptionShort = "#STR_CHEFZ_ITEM_WILDTHYME_DESC";
        model = "\dz\gear\cultivation\plant_material.p3d";
    };

    //! Rosmarin auf dem Item-Mesh der Ernte (Dateiname der Lieferung ohne e).
    class ChefZ_WildRosemary : ChefZ_WildPlant_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_WILDROSEMARY";
        descriptionShort = "#STR_CHEFZ_ITEM_WILDROSEMARY_DESC";
        model = "\ChefZ\ChefZ_Plants\models\rosmary.p3d";
    };

    //! Petersilie auf dem Item-Mesh der Ernte.
    class ChefZ_WildParsley : ChefZ_WildPlant_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_WILDPARSLEY";
        descriptionShort = "#STR_CHEFZ_ITEM_WILDPARSLEY_DESC";
        model = "\ChefZ\ChefZ_Plants\models\parsley.p3d";
    };
};

// Anbau-Registrierung der Maispflanze: Textur und Material der gesunden
// Pflanze, wie die Lieferung sie mitbringt (cultivation/config.cpp).
class CfgHorticulture
{
    class Plants
    {
        class ChefZ_CornPlant
        {
            // Schluesselnamen: PluginHorticulture.c:52-66 kennt genau diese vier.
            // corn_plant.rvmat und das flache corn_plant_co.paa sind mit a78a247
            // entfallen; die Lieferung setzt auf Vanillas Cannabis-Material und
            // die Stufentextur 4 - identisch zu cultivation/config.cpp.
            infestedTex = "dz\gear\cultivation\data\cannabis_plant_insect_co.paa";
            infestedMat = "dz\gear\cultivation\data\cannabis_plant_insect.rvmat";
            healthyTex = "ChefZ\ChefZ_Plants\cultivation\data\corn_plant_4_co.paa";
            healthyMat = "dz\gear\cultivation\data\cannabis_plant.rvmat";
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

    // ### SLICE apiary ### Imkerei - Honig ernten.
    //
    // handcraftRecipeSlots = 8 (seit dem 29.08.2026 mit dem Stopfen der
    // Pfeife, vorher 7). Die Zahl ist eine RESERVIERUNG in Vanillas
    // Rezeptliste und muss VOR dem Laden feststehen; wird sie vergessen,
    // erscheint kein einziges der Rezepte, und zwar OHNE Fehlermeldung an der
    // Stelle, an der man sucht (Kopf von ChefZ_HandcraftBridge.c).
    //
    // Die ACHT, einer je HANDCRAFT-Transform dieses Slice. Die Liste fuehrte
    // bis zum 31.08.2026 nur sieben - TR_FillBeeSmoker fehlte, obwohl die
    // Zahl darueber laengst 8 war. Ein Nachtrageversaeumnis, kein
    // Rechenfehler; dieselbe Luecke stand im README_Apiary.md:
    //
    //   TR_BuildBeehiveKit      PROCESS_BUILD_HIVE_KIT
    //   TR_RaiseBeehive         PROCESS_RAISE_HIVE
    //   TR_ExtendBeehive        PROCESS_EXTEND_HIVE
    //   TR_BuildHoneycombFrame  PROCESS_BUILD_FRAME
    //   TR_BuildUncappingFork   PROCESS_BUILD_UNCAPPING_FORK
    //   TR_BuildBeeSmoker       PROCESS_BUILD_BEE_SMOKER
    //   TR_UncapHoneycombFrame  PROCESS_UNCAP_COMB
    //   TR_FillBeeSmoker        PROCESS_FILL_SMOKER
    //
    // Die drei Stationsvorgaenge (PROCESS_HARVEST_HIVE, PROCESS_PACK_HIVE,
    // PROCESS_SPIN_HONEY) brauchen KEINEN Platz - sie laufen ueber ChefZ_ActionProcessAtStation
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
        handcraftRecipeSlots = 8;
        dataFiles[] =
        {
            "ChefZ_Farming/Config/Processing/Apiary_Ingredients.json",
            "ChefZ_Farming/Config/Processing/Apiary_Stations.json",
            "ChefZ_Farming/Config/Processing/Apiary_Crafts.json"
        };
    };

    // ### SLICE wildplants ### Die vier Wildpflanzen als Mini-Stationen.
    //
    // GENAU EINE Datei: der Stationsdatensatz. Es gibt keine
    // Zutatenbindung (eine Pflanze ist keine Zutat, sie wird nie verarbeitet)
    // und keinen Transform - PROCESS_HARVEST_WILD traegt keinen, aus den
    // Gruenden, die im Kopf von ChefZ_WildPlants.c stehen.
    //
    // handcraftRecipeSlots = 0: kein HANDCRAFT-Transform, keine Reservierung
    // in Vanillas Rezeptliste. PROCESS_HARVEST_WILD ist STATION_ACTION und
    // laeuft ueber ChefZ_ActionProcessAtStation, das Vanillas Rezeptliste
    // nicht anfasst.
    class ChefZ_WildPlants
    {
        chefzApiVersion = 1;
        loadOrder = 219;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Farming/Config/Processing/WildPlant_Stations.json"
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
// _deltas/produce.json; GRAIN kommt aus dem Slice grain. Dieses Modul fasst
// keine zentrale Registry an.
//==============================================================================
class CfgChefZIngredients
{
    class ChefZ_ProduceIngredient
    {
        // Vorlage, keine Bindung: der eigene Klassenname als template laesst
        // ChefZ_ConfigCppSource.IsBindingTemplate() den Knoten uebergehen -
        // sonst suchte der Manager je Serverstart nach einer CfgVehicles-Klasse
        // "ChefZ_ProduceIngredient", die es nicht gibt (Vorfall 31.08.2026).
        // Die Kinder unten erben diese Zeichenkette, nicht ihren eigenen Namen,
        // und bleiben deshalb echte Bindungen.
        template          = "ChefZ_ProduceIngredient";
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
    // Mais ist Gemuese UND Korn: VEGETABLE fuer Topf und Pfanne, GRAIN fuer die
    // Muehle. Kein Rezept-Slot matcht auf GRAIN - die Kategorie ist deshalb
    // I2-neutral. Tag bleibt CHEFZ_FRESH; CHEFZ_GRAIN gehoert dem Slice grain.
    class ChefZ_Corn : ChefZ_ProduceIngredient    { categories[] = {"VEGETABLE","GRAIN"}; };
};

//==============================================================================
// Prozesse dieses Moduls, Rang 1. Die Samengewinnung gibt es nicht mehr:
// Gemuese sind Fundpflanzen, es gibt kein Saatgut.
//==============================================================================
class CfgChefZProcesses
{
    //--------------------------------------------------------------------------
    // ### SLICE apiary ###   Die zehn Verben der Imkerei
    //
    // Acht davon sind HANDCRAFT, zwei sind Stationsaktionen (Oeffnen und
    // Abbauen des Stocks). Das Schleudern steht in ChefZ_Processing bei seiner
    // Station.
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
    //! Handwerksschritt verbraucht die Zutat samt Anhaengern, ein bestueckter
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
    //! der Deckel offen ist (Skript, CanReleaseAttachment).
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

    // ------------------------------------------------------------------
    // 5. PFEIFE STOPFEN (29.08.2026): Imkerpfeife + Rinde -> volle Pfeife.
    //
    // Zwei Zutaten, kein Werkzeug - beide Plaetze von Vanillas RecipeBase
    // sind belegt (01 V12), deshalb KEINE toolGroups-Zeile. Das Anzuenden
    // ist kein Prozess: es laeuft ueber Vanillas ActionLightItemOnFire, der
    // an jedem Item CanBeIgnitedBy fragt (ChefZ_BeeSmoker beantwortet es).
    // ------------------------------------------------------------------
    class PROCESS_FILL_SMOKER
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_FILL_SMOKER";
        baseDurationSec = 6.0;
        animationLength = 2.0;
        specialty = 0.01;
        toolDamage = 0;
    };

    // ------------------------------------------------------------------
    // 6. STOCK ABBAUEN (29.08.2026): der aufgestellte Stock wird wieder zum
    //    Bausatz - Lykos' Lieferung hatte das als Vanilla-Rezept
    //    (Pack_BeeHive: Stock + Schraubenzieher -> Kit); hier ist es der
    //    zweite Stationsvorgang des Stocks.
    //
    // STATION_ACTION und nicht HANDCRAFT: ein 14-kg-Stock liegt am Boden,
    // und ein Handwerksschritt braeuchte ihn in der Hand. Am Stock in der
    // Welt gibt es genau eine Aktionsform, und das ist diese.
    //
    // KEIN Transform: der Vorgang veraendert nicht den Inhalt der Station,
    // sondern die Station selbst. Das erledigt ChefZ_Beehive im Haken
    // ChefZ_OnStationActionFinished (Scripts/4_World/ChefZ/Farming/
    // ChefZ_Apiary.c): Bausatz an Ort und Stelle, Stock weg. Der Vorgang
    // erscheint nur an einem LEEREN, GESCHLOSSENEN Stock - mit Raehmchen
    // darin blendet das Skript ihn aus (ChefZ_GetProcessAt), damit niemand
    // sein Volk samt vierzig Stunden Arbeit in eine Kiste packt.
    //
    // HAND_TOOL wie beim Aufstellen; toolDamage 2 trifft sicher das
    // Werkzeug, weil die Aktion ohne eines nicht erscheint (vgl. die
    // Anmerkung zu toolDamage 0 an PROCESS_HARVEST_HIVE).
    // ------------------------------------------------------------------
    class PROCESS_PACK_HIVE
    {
        exec = "STATION_ACTION";
        displayName = "#STR_CHEFZ_PROC_PACK_HIVE";
        toolGroups[] = {"HAND_TOOL"};
        baseDurationSec = 20.0;
        toolDamage = 2;
    };

    //--------------------------------------------------------------------------
    // ### SLICE wildplants ###   Der eine Vorgang der Wildnis
    //
    // STATION_ACTION, weil die Pflanze im Boden steht: ein Handwerksschritt
    // braeuchte sie in der Hand, und dorthin kommt sie nie
    // (ChefZ_WildPlant_Base.IsTakeable() -> false).
    //
    // KEIN Transform, und das ist keine Auslassung. ChefZ_TransformDef.
    // Validate() weist einen Transform ohne "inputs" ausdruecklich ab
    // (ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_TransformDef.c:173-178), eine
    // Wildpflanze hat aber kein Cargo, aus dem ein Eingang binden koennte
    // (ChefZ_FactCollector.c:191-196), und der Applicator legt Ergebnisse
    // "ausschliesslich in den CARGO EINES GEFAESSES" ab
    // (ChefZ_ProcessRunner.c:165-166). Der Ertrag entsteht deshalb im Haken
    // ChefZ_OnStationActionFinished und faellt zu Boden - dieselbe Bauform
    // wie PROCESS_HARVEST_HIVE und PROCESS_PACK_HIVE.
    //
    // ChefZ_ActionProcessAtStation.IsProcessUsable() ueberspringt die
    // Transformpruefung, wenn zu einem Prozess kein Transform bekannt ist
    // (ChefZ_ActionProcessAtStation.c:321-324); RunImmediate meldet dann
    // NO_MATCH, und NotifyStation ruft den Haken trotzdem. NO_MATCH ist hier
    // der gewollte Ausgang.
    //
    // KEINE toolGroups - "HAND" laut Spec Kap. 3, kein Werkzeugzwang. Ein
    // Kolben bricht man mit der Hand ab.
    //
    // toolDamage = 0, und das ist zwingend: ChefZ_ActionProcessAtStation.
    // ApplyToolDamage() beschaedigt action_data.m_MainItem - was auch immer
    // in der Hand liegt, ohne jede Pruefung gegen toolGroups. Bei einem
    // Vorgang ohne Pflichtwerkzeug fraesse die Pflanze sonst am Gewehr des
    // Spielers. Dieselbe Anmerkung steht an PROCESS_HARVEST_HIVE.
    //
    // Fuenf Sekunden (Spec Kap. 3): ein Kolben abbrechen, ein Bund abreissen.
    //--------------------------------------------------------------------------
    class PROCESS_HARVEST_WILD
    {
        exec = "STATION_ACTION";
        displayName = "#STR_CHEFZ_PROC_HARVEST_WILD";
        baseDurationSec = 5.0;
        toolDamage = 0;
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

//------------------------------------------------------------------------------
// Die Geraeusche der Imkerei (29.08.2026).
//
// Bees_Attack: das Volk geht auf den Imker los - gespielt vom SERVER ueber
// Vanillas ItemSoundHandler (ItemBase.StartItemSoundServer, scripts - 1.29/
// 4_World/DayZ/Entities/ItemBase.c:4468), damit jeder in Hoerweite es hoert
// und nicht nur der Gestochene. Die Bindung ID -> SoundSet steht im Skript
// (ChefZ_Beehive.InitItemSounds in ChefZ_Apiary.c).
//
// Form und Basisklassen woertlich nach einem Vorbild, das laeuft:
// DayZExpansion/AI/Sounds/config.cpp:14 (Shader) und :270 (Set). Der
// Samplepfad ist der PBO-Prefix plus Dateiname OHNE Endung; die Datei liegt
// als .ogg in ChefZ_Farming/Sounds und wird ueber include.txt (*.ogg)
// gepackt - ohne diesen Eintrag laesst der Packer sie still liegen.
//
// Beehive_Ambient liegt daneben, ist aber noch nicht eingebunden: ein
// Dauerton am Stock braucht eine Schleife mit Start/Stop und einen Grund,
// wann sie schweigt. Das ist eine eigene Entscheidung.
//------------------------------------------------------------------------------
class CfgSoundShaders
{
    class baseCharacter_SoundShader;
    class ChefZ_Bees_Attack_SoundShader : baseCharacter_SoundShader
    {
        samples[] = { { "\ChefZ_Farming\Sounds\Bees_Attack", 1 } };
        volume = 1.0;
        range = 40;
    };
};

class CfgSoundSets
{
    class baseCharacter_SoundSet;
    class ChefZ_Bees_Attack_SoundSet : baseCharacter_SoundSet
    {
        soundShaders[] = { "ChefZ_Bees_Attack_SoundShader" };
        spatial = 1;
        volumeFactor = 1.0;
    };
};
