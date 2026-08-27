//==============================================================================
// ChefZ_Ingredients - Grundzutaten und Zwischenprodukte
//
// ABSCHNITT DES SLICE "produce". Quellen: Production Map §13 (Kartoffel),
// §14 (Tomate), §15 (Paprika, nur der Schnitt), §17 (Zwiebel), §18 (Knoblauch),
// §19 (Karotte), §20 (Kohl), §57-58 (Station/Werkzeug), §73 (Klassenliste);
// DME-Plan §53 (Namenskonvention).
//
// Dieses Modul ist ein GETEILTER Ordner: die Slices "salt", "herbs" und "dairy"
// legen hier ebenfalls Zutaten ab. Alles unterhalb eines Slice-Banners gehoert
// dem genannten Slice; wer etwas ergaenzt, HAENGT AN und ueberschreibt nichts.
//
// Was hier liegt: die geschnittenen Gemuesestufen. Das ganze Gemuese (Ernte)
// liegt in ChefZ_Farming, weil es dort waechst - Production Map §2.
//
// Andockregel des Core (Kopf von ChefZ_Edible_Base.c): die CONFIGklasse erbt
// von einer VANILLA-Klasse, die SKRIPTklasse von ChefZ_Edible_Base. Der Core
// bringt bewusst keinen CfgVehicles-Eintrag mit (Invariante I3).
//
// BEWUSST OHNE class Food / FoodStages: geschnittenes Gemuese ist Zutat, nicht
// Garobjekt. Wer FoodStages ohne FoodStageTransitions deklariert, baut die
// Falle aus 01 V4 - FoodStage.GetNextFoodStageType faellt dann auf BURNED
// zurueck und das Item verbrennt beim ersten Garstufenwechsel. Gegart wird in
// M3 ueber ChefZ-Rezepte, die eigene Gerichtsklassen erzeugen.
//
// MODELLE: saemtliche model=-Pfade sind VANILLA-PROXIES. Kein Item dieses
// Slices hat eigene Geometrie; der Bedarf steht im Slice-Bericht und im
// Asset-To-Do §3.4 (geschnittene Varianten teilen sich ein Schnittgut-Mesh).
//==============================================================================

class CfgPatches
{
    class ChefZ_Ingredients
    {
        units[] = {
            "ChefZ_ChoppedVegetableBase",
            "ChefZ_SlicedPotato", "ChefZ_ChoppedTomato", "ChefZ_ChoppedPaprika",
            "ChefZ_ChoppedOnion", "ChefZ_ChoppedGarlic", "ChefZ_ChoppedCarrot",
            "ChefZ_ChoppedCabbage",
            // ### SLICE dairy ###
            "ChefZ_Milk", "ChefZ_Cream", "ChefZ_Butter", "ChefZ_Cheese", "ChefZ_Egg",
            // ### SLICE salt ###
            "ChefZ_RawSalt",
            "ChefZ_Salt",
            // ### SLICE herbs ###
            "ChefZ_DriedHerbBase",
            "ChefZ_DriedParsley", "ChefZ_DriedDill", "ChefZ_DriedThyme",
            "ChefZ_DriedRosemary", "ChefZ_DriedWildGarlic", "ChefZ_DriedPaprika",
            "ChefZ_SpiceBase",
            "ChefZ_PaprikaPowder", "ChefZ_DriedPeppercorns", "ChefZ_BlackPepper",
            "ChefZ_HerbMix", "ChefZ_HunterSeasoning"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // DZ_Data       - Edible_Base, die Vanilla-Configbasis dieser Items
        // DZ_Gear_Food  - die Proxy-Modelle unter \dz\gear\food\
        // ChefZ_Core    - ChefZ_Edible_Base und die Auswertung der CfgChefZ*-Knoten
        // ChefZ_Farming - die Eingangsklassen der Schnitt-Transforms (ChefZ_Onion ...)
        requiredAddons[] = {"DZ_Data", "DZ_Gear_Food", "ChefZ_Core", "ChefZ_Farming", "DZ_Gear_Consumables"};
    };
};

class CfgMods
{
    class ChefZ_Ingredients
    {
        dir = "ChefZ_Ingredients";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Ingredients";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";
        dependencies[] = {"World"};

        class defs
        {
            // Nur 4_World: hier leben die Ableitungen von ChefZ_Edible_Base.
            // Das Modul bringt kein System mit - Content gehoert in Module,
            // Systeme in den Core (Workflow §10.3).
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Ingredients/Scripts/4_World"
                };
            };
        };
    };
};

class CfgVehicles
{
    class GardenLime;   // ### SLICE salt ###

    // ### SLICE dairy ### Proxy-Basen
    class PowderedMilk;
    class Honey;
    class Lard;
    class BoxCerealCrunchin;
    class Marmalade;

    class Edible_Base;

    //==========================================================================
    // ### SLICE produce ### Geschnittenes Gemuese
    //
    // class Nutrition ist PFLICHT und keine Kuer: PlayerStomach.InitData
    // registriert nur Klassen mit "Nutrition" ODER "Food" und scope != 0
    // (01 V7, PlayerStomach.c:208-250). Ohne den Block saettigt der Bissen
    // lautlos nicht - der leiseste denkbare Content-Fehler.
    //==========================================================================
    class ChefZ_ChoppedVegetableBase : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\food\pumpkin_sliced.p3d";
        weight = 90;
        itemSize[] = {1,1};
        rotationFlags = 17;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        varQuantityDestroyOnMin = 1;
        absorbency = 0.0;
        isMeleeWeapon = 0;
        soundImpactType = "food";
    };

    // §13: Potato + Knife -> ChefZ_SlicedPotato
    class ChefZ_SlicedPotato : ChefZ_ChoppedVegetableBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SLICEDPOTATO";
        descriptionShort = "#STR_CHEFZ_ITEM_SLICEDPOTATO_DESC";
        weight = 120;
        class Nutrition
        {
            fullnessIndex = 40;
            energy = 180;
            water = 40;
            nutritionalIndex = 30;
            toxicity = 0;
            digestibility = 1;
        };
    };

    // §14: Tomato + Knife -> ChefZ_ChoppedTomato
    class ChefZ_ChoppedTomato : ChefZ_ChoppedVegetableBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHOPPEDTOMATO";
        descriptionShort = "#STR_CHEFZ_ITEM_CHOPPEDTOMATO_DESC";
        weight = 110;
        class Nutrition
        {
            fullnessIndex = 20;
            energy = 45;
            water = 70;
            nutritionalIndex = 25;
            toxicity = 0;
            digestibility = 1;
        };
    };

    // §15: ChefZ_Paprika (Slice herbs) oder GreenBellPepper + Knife
    class ChefZ_ChoppedPaprika : ChefZ_ChoppedVegetableBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHOPPEDPAPRIKA";
        descriptionShort = "#STR_CHEFZ_ITEM_CHOPPEDPAPRIKA_DESC";
        weight = 100;
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

    // §17: ChefZ_Onion + Knife -> ChefZ_ChoppedOnion
    class ChefZ_ChoppedOnion : ChefZ_ChoppedVegetableBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHOPPEDONION";
        descriptionShort = "#STR_CHEFZ_ITEM_CHOPPEDONION_DESC";
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

    // §18: ChefZ_Garlic + Knife -> ChefZ_ChoppedGarlic
    class ChefZ_ChoppedGarlic : ChefZ_ChoppedVegetableBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHOPPEDGARLIC";
        descriptionShort = "#STR_CHEFZ_ITEM_CHOPPEDGARLIC_DESC";
        weight = 30;
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

    // §19: ChefZ_Carrot + Knife -> ChefZ_ChoppedCarrot
    class ChefZ_ChoppedCarrot : ChefZ_ChoppedVegetableBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHOPPEDCARROT";
        descriptionShort = "#STR_CHEFZ_ITEM_CHOPPEDCARROT_DESC";
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

    // §20: ChefZ_Cabbage + Knife -> ChefZ_ChoppedCabbage
    class ChefZ_ChoppedCabbage : ChefZ_ChoppedVegetableBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHOPPEDCABBAGE";
        descriptionShort = "#STR_CHEFZ_ITEM_CHOPPEDCABBAGE_DESC";
        weight = 140;
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
    // ### SLICE dairy ### Milchkette und Ei
    //
    // Quellen: Production Map §47 (Milch, Loot - Tiermelken ist NICHT V1),
    // §48 (Sahne), §49 (Butter), §50 (Kaese, GENAU EINE Sorte in V1),
    // §51 (Ei); DME-Plan §21, §22, §53.
    //
    // MODELLE sind VANILLA-PROXIES: die Configklasse erbt von einer Vanilla-
    // Klasse, statt einen p3d-Pfad zu nennen. Ein geratener Pfad faellt erst
    // beim Packen auf, eine geerbte Klasse nie. Bewusst nur Vanilla-Klassen
    // OHNE eigenen Food-Block: sonst erbte das ChefZ-Item stillschweigend
    // fremde Stufen-Naehrwerte, und die Werte hier waeren wirkungslos
    // (FoodStage-Werte schlagen class Nutrition, 01 V7). Der Bedarf an eigener
    // Geometrie steht im Slice-Bericht.
    //
    // class Nutrition ist PFLICHT: PlayerStomach.InitData registriert nur
    // Klassen mit Nutrition oder Food und scope != 0 (01 V7).
    //
    // KEIN Food-Block bei Milch, Sahne, Butter und Kaese - und das ist die
    // sichere Richtung, nicht die faule: ohne FoodStages ist HasFoodStage()
    // falsch, CanBeCooked() liefert false und Cooking.ProcessItemToCook laesst
    // das Item unangetastet liegen (Cooking.c:47). Gefaehrlich waeren STUFEN
    // OHNE UEBERGAENGE - genau die Falle aus 01 V4. Gegart wird ueber
    // ChefZ-Rezepte, die eigene Gerichtsklassen erzeugen.
    //
    // ZUSTAENDE: dieser Slice vergibt bewusst keinen ChefZ-Zustand. Die
    // Milchkette ist ein reiner Klassentausch (06 §2); der Zustandsraum gehoert
    // der Konservierungskette.
    //==========================================================================

    // §47: Milch. Quelle ist ausschliesslich Loot.
    // PROXY: PowderedMilk (Karton). Ziel: eigene Milchflasche.
    class ChefZ_Milk : PowderedMilk
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MILK";
        descriptionShort = "#STR_CHEFZ_ITEM_MILK_DESC";
        weight = 520;
        itemSize[] = {2, 3};
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 30;
            energy = 200;
            water = 400;
            nutritionalIndex = 15;
            toxicity = 0;
            digestibility = 1;
        };
    };

    // §48: Sahne. Entsteht am Butterfass aus Milch.
    // PROXY: Honey (Glas). Ziel: eigenes Sahnegefaess.
    class ChefZ_Cream : Honey
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CREAM";
        descriptionShort = "#STR_CHEFZ_ITEM_CREAM_DESC";
        weight = 260;
        itemSize[] = {2, 2};
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;
        lifetime = 14400;

        class Nutrition
        {
            fullnessIndex = 20;
            energy = 350;
            water = 150;
            nutritionalIndex = 10;
            toxicity = 0;
            digestibility = 1;
        };
    };

    // §49: Butter. Haelt laenger als Sahne - deshalb die hoehere lifetime.
    // PROXY: Lard (Fettblock). Ziel: eigenes Butterstueck.
    class ChefZ_Butter : Lard
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BUTTER";
        descriptionShort = "#STR_CHEFZ_ITEM_BUTTER_DESC";
        weight = 250;
        itemSize[] = {2, 1};
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;
        lifetime = 43200;

        class Nutrition
        {
            fullnessIndex = 15;
            energy = 600;
            water = 20;
            nutritionalIndex = 8;
            toxicity = 0;
            digestibility = 1;
        };
    };

    // §50: Kaese. V1 kennt GENAU EINE Kaeseklasse; mehrere Sorten sind
    // ausdruecklich V2. Das haltbarste Milchprodukt.
    // PROXY: BoxCerealCrunchin (Schachtel). Ziel: eigener Kaeselaib.
    class ChefZ_Cheese : BoxCerealCrunchin
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHEESE";
        descriptionShort = "#STR_CHEFZ_ITEM_CHEESE_DESC";
        weight = 220;
        itemSize[] = {2, 2};
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;
        lifetime = 86400;

        class Nutrition
        {
            fullnessIndex = 35;
            energy = 450;
            water = 60;
            nutritionalIndex = 25;
            toxicity = 0;
            digestibility = 1;
        };
    };

    // §51: Ei. Quelle ist Loot, Nest, Farmgebaeude - kein Huhn-System in V1.
    //
    // Das Ei ist das EINZIGE Item dieses Slice MIT Food-Block: es wird als es
    // selbst gebraten und gekocht. Deshalb traegt es beides - Stufen UND
    // Uebergaenge. Stufen ohne Uebergaenge waeren die Falle aus 01 V4.
    //
    // PROXY: Marmalade (kleines Glas). Ziel: Ei / Eierkarton.
    class ChefZ_Egg : Marmalade
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_EGG";
        descriptionShort = "#STR_CHEFZ_ITEM_EGG_DESC";
        weight = 60;
        itemSize[] = {1, 1};
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;
        lifetime = 21600;

        class Nutrition
        {
            fullnessIndex = 12;
            energy = 90;
            water = 40;
            nutritionalIndex = 12;
            toxicity = 0;
            digestibility = 1;
        };

        // nutrition_properties[] = {fullnessIndex, energy, water,
        //                           nutritionalIndex, toxicity, agents,
        //                           digestibility}
        // Reihenfolge woertlich aus FoodStage.GetFullnessIndex(0) /
        // GetEnergy(1) / GetWater(2) / GetNutritionalIndex(3) /
        // GetToxicity(4) / GetAgents(5) / GetDigestibility(6).
        class Food
        {
            class FoodStages
            {
                class Raw
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {12, 90, 40, 12, 0, 0, 1};
                };
                class Baked
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {14, 105, 25, 18, 0, 0, 1};
                };
                class Boiled
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {14, 100, 35, 18, 0, 0, 1};
                };
                class Burned
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {6, 20, 5, 0, 0, 0, 1};
                };
                class Rotten
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {6, 20, 10, 0, 15, 0, 1};
                };
            };

            // cooking_method: 1 = BAKING, 2 = BOILING (Cooking.c:1).
            // transition_to:  2 = BAKED,  3 = BOILED  (FoodStage.c:1).
            //
            // Aus BAKED und BOILED gibt es bewusst KEINEN weiteren Uebergang:
            // ohne passenden Eintrag liefert GetNextFoodStageType
            // FoodStageType.BURNED (FoodStage.c:472) - und genau das soll ein
            // zu lange liegendes Ei werden.
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
    // ### SLICE salt ###   Production Map §25/§26, Planungsschritte §16
    //
    // Das ENDE der Salzkette: Saltwater -> ChefZ_RawSalt -> ChefZ_Salt. Der Weg
    // dorthin - Station, Prozesse, Transforms - liegt in ChefZ_Processing.
    //
    // SALZ IST NICHT ESSBAR, und das ist eine Entscheidung, kein Vergessen.
    // Production Map §26 zaehlt die Verwendung vollstaendig auf: Teig, Brot,
    // Pasta, Wurst, Fleisch, Fisch, Suppen, Eintoepfe, Gewuerzmischungen,
    // Konservierung. "Loeffelweise essen" steht nicht darin. Beide Klassen
    // erben deshalb von GardenLime - einem mengenbasierten Pulverbehaelter OHNE
    // Nutrition- und OHNE Food-Block - und nicht von Edible_Base:
    //
    //   - kein Naehrwert, den ein Spieler durch Salzessen abgreifen koennte,
    //   - keine FoodStages, die im Topf zu Kohle wuerden (01 V4),
    //   - keine geerbten Vanilla-Naehrwerte, die eigene Werte still schlagen.
    //
    // chefznut meldet dafuer je eine WARNUNG ("weder class Nutrition noch class
    // Food"). Sie darf stehen bleiben - der Pruefer sagt das ausdruecklich
    // selbst fuer den Fall "die Klasse soll gar nicht gegessen werden".
    //
    // Kein Skript, keine ChefZ-Skriptklasse: Salz traegt keinen ChefZ-Zustand,
    // verdirbt nicht und wird nicht gegart. "Kein Fehler, nur weniger"
    // (Kopf von ChefZ_Edible_Base.c).
    //==========================================================================

    // --- Rohsalz (Production Map §25, Zwischenprodukt) ----------------------
    //
    // Was aus der Siedepfanne kommt: feucht, grob, mit Restsole. Ausdruecklich
    // KEIN fertiges Gewuerz - es traegt weder die Kategorie SALT noch den Tag
    // CHEFZ_SPICE und passt deshalb in kein Gewuerzrezept. Erst das Trocknen
    // macht daraus ChefZ_Salt.
    //
    // MODELL: Vanilla-Proxy GardenLime (Pulverbeutel). Ziel laut Production
    // Map §71 (Shared Mesh Strategy): EIN gemeinsames Gewuerzbehaelter-Mesh
    // fuer Salz, Pfeffer, Paprikapulver und Kraeutermix, hier mit der Textur
    // "grobes, feuchtes Salz".
    class ChefZ_RawSalt : GardenLime
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_RAWSALT";
        descriptionShort = "#STR_CHEFZ_ITEM_RAWSALT_DESC";
        model = "\dz\gear\consumables\garden_lime.p3d";
        rotationFlags = 17;
        itemSize[] = {2, 2};
        weight = 140;
        absorbency = 0.6;

        // Menge in GRAMM. 60 g ist genau die Ausbeute EINES Siedegangs
        // (TR_SaltwaterToRawSalt: 1500 ml Salzwasser). Der Beutel fasst zwei
        // Siedegaenge - Rohsalz ist ein Zwischenschritt und soll kein Lager
        // werden.
        stackedUnit = "grams";
        quantityBar = 1;
        varQuantityInit = 60;
        varQuantityMin = 0;
        varQuantityMax = 120;
        varQuantityDestroyOnMin = 1;
        canBeSplit = 1;
    };

    // --- Salz (Production Map §25, §26) -------------------------------------
    //
    // Das Endprodukt der Kette und die Handelsressource aus Planungsschritte
    // §16. Verwendung: Teig, Brot, Pasta, Wurst, Fleisch, Fisch, Suppen,
    // Eintoepfe, Gewuerzmischungen, Konservierung.
    //
    // Salz verdirbt nicht (decays = false im Zutatendatensatz). Das ist keine
    // Bequemlichkeit, sondern die Voraussetzung dafuer, dass es ueberhaupt
    // eine Handelswaehrung sein kann - eine verderbende Waehrung ist keine.
    //
    // MODELL: Vanilla-Proxy GardenLime. Ziel: dasselbe gemeinsame
    // Gewuerzbehaelter-Mesh (§71), Textur "feines weisses Salz".
    class ChefZ_Salt : GardenLime
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SALT";
        descriptionShort = "#STR_CHEFZ_ITEM_SALT_DESC";
        model = "\dz\gear\consumables\garden_lime.p3d";
        rotationFlags = 17;
        itemSize[] = {2, 2};
        weight = 90;
        absorbency = 0.0;

        // Menge in GRAMM. 40 g ist die Ausbeute EINES vollstaendigen
        // Durchlaufs (1500 ml Salzwasser -> 60 g Rohsalz -> 40 g Salz).
        // Ein Gericht wuerzt mit rund 5 g; ein Durchlauf reicht also fuer acht
        // Gerichte und kostet dafuer 35 Minuten und ein brennendes Feuer.
        // varQuantityMax = 200 laesst fuenf Durchlaeufe in einem Beutel
        // zusammenlaufen - handelbar, aber nicht beliebig stapelbar.
        stackedUnit = "grams";
        quantityBar = 1;
        varQuantityInit = 40;
        varQuantityMin = 0;
        varQuantityMax = 200;
        varQuantityDestroyOnMin = 1;
        canBeSplit = 1;
    };

    //==========================================================================
    // ### SLICE herbs ###   Production Map §22-§24, §15, §16
    //
    // Das ENDE der Kraeuter-, Pfeffer- und Paprikakette: was den Trockenrahmen
    // und den Moerser verlaesst. Die frischen Vorstufen (ChefZ_Parsley,
    // ChefZ_Paprika, ChefZ_PepperBerries) stehen in ChefZ_Farming, die
    // Stationen und Transforms in ChefZ_Processing.
    //
    // class Nutrition ist PFLICHT: PlayerStomach.InitData registriert nur
    // Klassen mit Nutrition ODER Food und scope != 0 (01 V7). Getrocknete
    // Kraeuter saettigen fast nichts - aber "fast nichts" und "lautlos nichts"
    // sind zwei verschiedene Dinge.
    //
    // Bewusst OHNE class Food / FoodStages: kein Gewuerz dieses Slice ist ein
    // Garobjekt. Es geht als ZUTAT in ein Gericht ein und liegt nie selbst im
    // Topf. Wer FoodStages ohne FoodStageTransitions deklariert, baut die
    // Falle aus 01 V4 (FoodStage.c:472 verbrennt das Item).
    //
    // decays = false steht im Zutatendatensatz (Config/Ingredients/
    // Spices.json), nicht hier: Haltbarkeit ist eine ChefZ-Eigenschaft, kein
    // Vanilla-Configfeld. Getrocknetes ist der Sinn des Trocknens
    // (DME-Plan §9).
    //
    // PROXY-MODELLE, alle Vanilla, alle im Asset-Bedarf des Slice gemeldet.
    //==========================================================================
    class ChefZ_DriedHerbBase : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\consumables\birch_bark.p3d";
        weight = 12;
        itemSize[] = {1, 1};
        rotationFlags = 17;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 172800;

        class Nutrition
        {
            fullnessIndex = 2;
            energy = 8;
            water = 0;
            nutritionalIndex = 15;
            toxicity = 0;
            digestibility = 1;
        };
    };

    class ChefZ_DriedParsley : ChefZ_DriedHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRIEDPARSLEY";
        descriptionShort = "#STR_CHEFZ_ITEM_DRIEDPARSLEY_DESC";
    };

    class ChefZ_DriedDill : ChefZ_DriedHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRIEDDILL";
        descriptionShort = "#STR_CHEFZ_ITEM_DRIEDDILL_DESC";
    };

    class ChefZ_DriedThyme : ChefZ_DriedHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRIEDTHYME";
        descriptionShort = "#STR_CHEFZ_ITEM_DRIEDTHYME_DESC";
    };

    class ChefZ_DriedRosemary : ChefZ_DriedHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRIEDROSEMARY";
        descriptionShort = "#STR_CHEFZ_ITEM_DRIEDROSEMARY_DESC";
    };

    class ChefZ_DriedWildGarlic : ChefZ_DriedHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRIEDWILDGARLIC";
        descriptionShort = "#STR_CHEFZ_ITEM_DRIEDWILDGARLIC_DESC";
    };

    // Getrocknete Paprikaschoten - Zwischenstufe zum Pulver (Production Map §15).
    class ChefZ_DriedPaprika : ChefZ_DriedHerbBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRIEDPAPRIKA";
        descriptionShort = "#STR_CHEFZ_ITEM_DRIEDPAPRIKA_DESC";
        weight = 40;
    };

    //--------------------------------------------------------------------------
    // Gewuerze. Production Map §71 empfiehlt EIN gemeinsames Behaelter-Mesh mit
    // verschiedenen Texturen - der Proxy nimmt das vorweg: eine Pulvertuete.
    //--------------------------------------------------------------------------
    class ChefZ_SpiceBase : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\food\PowderedMilk.p3d";
        weight = 60;
        itemSize[] = {1, 2};
        rotationFlags = 17;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 172800;

        class Nutrition
        {
            fullnessIndex = 2;
            energy = 10;
            water = 0;
            nutritionalIndex = 10;
            toxicity = 0;
            digestibility = 1;
        };
    };

    class ChefZ_PaprikaPowder : ChefZ_SpiceBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_PAPRIKAPOWDER";
        descriptionShort = "#STR_CHEFZ_ITEM_PAPRIKAPOWDER_DESC";
    };

    // Pfefferkoerner sind noch kein Pulver - eigenes Modell, Beutelform.
    class ChefZ_DriedPeppercorns : ChefZ_SpiceBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRIEDPEPPERCORNS";
        descriptionShort = "#STR_CHEFZ_ITEM_DRIEDPEPPERCORNS_DESC";
        model = "\dz\gear\food\Rice.p3d";
        weight = 40;
    };

    class ChefZ_BlackPepper : ChefZ_SpiceBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BLACKPEPPER";
        descriptionShort = "#STR_CHEFZ_ITEM_BLACKPEPPER_DESC";
    };

    class ChefZ_HerbMix : ChefZ_SpiceBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HERBMIX";
        descriptionShort = "#STR_CHEFZ_ITEM_HERBMIX_DESC";
    };

    class ChefZ_HunterSeasoning : ChefZ_SpiceBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERSEASONING";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERSEASONING_DESC";
    };
};

//==============================================================================
// ### SLICE produce ### Zutatenbindung, Rang 1
//
// Entwurf 05 §2: EIGENE Klassen deklarieren sich in der eigenen config.cpp,
// FREMDE (Potato, Tomato, GreenBellPepper) im Slice-JSON. Deshalb steht hier
// nur ChefZ-Eigenes; die drei Vanillaklassen stehen in
// Config/Ingredients/VanillaProduce.json.
//
// Die Kategorien VEGETABLE, ROOT_VEGETABLE, LEAF_VEGETABLE und die Tags
// CHEFZ_FRESH, CHEFZ_CHOPPED stehen im Delta _deltas/produce.json und werden
// vom chefz-registry-integrator gemergt. Dieses Modul fasst keine zentrale
// Registry an (Workflow §5).
//
// Der Basisknoten heisst bewusst NICHT wie die CfgVehicles-Basisklasse: die
// Config-Vererbung INNERHALB von CfgChefZIngredients loest die Engine selbst
// auf (Kopf von ChefZ_ConfigCppSource.ReadIngredients), ein zweiter Knoten
// gleichen Namens waere nur eine Verwechslungsquelle.
//==============================================================================
class CfgChefZIngredients
{
    class ChefZ_ChoppedProduceIngredient
    {
        categories[]      = {"VEGETABLE"};
        tags[]            = {"CHEFZ_CHOPPED"};
        defaultState      = "RAW";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        decays            = 1;
    };

    class ChefZ_SlicedPotato : ChefZ_ChoppedProduceIngredient   { categories[] = {"VEGETABLE","ROOT_VEGETABLE"}; };
    class ChefZ_ChoppedOnion : ChefZ_ChoppedProduceIngredient   { categories[] = {"VEGETABLE","ROOT_VEGETABLE"}; };
    class ChefZ_ChoppedGarlic : ChefZ_ChoppedProduceIngredient  { categories[] = {"VEGETABLE","ROOT_VEGETABLE"}; };
    class ChefZ_ChoppedCarrot : ChefZ_ChoppedProduceIngredient  { categories[] = {"VEGETABLE","ROOT_VEGETABLE"}; };
    class ChefZ_ChoppedCabbage : ChefZ_ChoppedProduceIngredient { categories[] = {"VEGETABLE","LEAF_VEGETABLE"}; };
    class ChefZ_ChoppedTomato : ChefZ_ChoppedProduceIngredient  {};
    class ChefZ_ChoppedPaprika : ChefZ_ChoppedProduceIngredient {};
};

//==============================================================================
// ### SLICE produce ### Der Schnittprozess, Rang 1
//
// Prozesse stehen in der Game-Config und nicht nur im JSON (11 E8, 02 §2):
// ChefZ_ActionProcessAtStation.ActionCondition() laeuft auch auf dem CLIENT und
// muss dort Werkzeuggruppe und Aktionstext kennen. Der Client liest Rang 1
// garantiert.
//
// HANDCRAFT und nicht STATION_ACTION: Production Map §13-§20 sagt durchweg
// "+ Knife", nicht "+ Schneidebrett". Ein Eingang plus Werkzeuggruppe ist genau
// die Form, die Vanillas RecipeBase traegt (01 V12,
// MAX_NUMBER_OF_INGREDIENTS = 2: das Messer belegt den zweiten Platz).
//
// Die Werkzeuggruppe CUTTING_TOOL steht unten. Sie ist bewusst KEIN
// produce-eigener Name: CfgChefZTools wird von der Engine ueber alle Addons
// gemergt, zwei Module duerfen denselben Gruppenknoten mit derselben
// Klassenliste tragen, und der Slice bleibt dadurch fuer sich allein
// lauffaehig - auch wenn ChefZ_Processing gerade nicht geladen ist. Zwei
// VERSCHIEDENE Gruppen mit denselben Messern waeren dagegen eine Doppelung,
// die spaeter auseinanderlaeuft.
//==============================================================================
class CfgChefZTools
{
    // Werkzeuge als DATEN (11 E8): id = die GRUPPE, classes[] = ihre
    // Mitglieder. Das ist die richtige Schreibweise fuer FREMDE Klassen -
    // ChefZ fasst keine Vanilla-config.cpp an, sondern nennt die Klassen in
    // einer eigenen Gruppe (Workflow §10.5).
    class CUTTING_TOOL
    {
        classes[] =
        {
            "KitchenKnife",
            "SteakKnife",
            "HuntingKnife",
            "CombatKnife",
            "KukriKnife",
            "BoneKnife",
            "StoneKnife",
            "FangeKnife"
        };
        allowSubclasses = 1;
    };
};

class CfgChefZProcesses
{
    class PROCESS_CHOP_VEGETABLE
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_CHOP_VEGETABLE";
        toolGroups[] = {"CUTTING_TOOL"};
        baseDurationSec = 5.0;
        animationLength = 1.0;
        specialty = 0.01;
        toolDamage = 1;
    };
};

//==============================================================================
// Anmeldung beim Core (02 §4). CfgChefZ fuehrt einen Knoten je SLICE, nicht je
// Modul: ChefZ_SliceManifest.name ist eine Kennung fuer den Ladebericht (Kopf
// von ChefZ_RecordSource.c). Der Slice "produce" liegt in ZWEI Modulen -
// ChefZ_Ingredients und ChefZ_Farming - und meldet sich deshalb EINMAL an,
// statt zweimal halb. Jeder Pfad traegt sein eigenes PBO-Praefix als Wurzel
// (02 §4.1, B4); requiredAddons oben stellt sicher, dass beide PBOs da sind.
//
// handcraftRecipeSlots ist eine RESERVIERUNG in Vanillas Rezeptliste und muss
// vor dem ersten Laden feststehen (Kopf von ChefZ_HandcraftBridge):
//    8 Schnitt-Transforms (Potato, Tomato, ChefZ_Paprika, GreenBellPepper,
//                          Onion, Garlic, Carrot, Cabbage)
//  + 4 Samen-Transforms   (Onion, Garlic, Carrot, Cabbage)
//  = 12 Plaetze.
//==============================================================================
class CfgChefZ
{
    // ### SLICE produce ###
    class ChefZ_Produce
    {
        chefzApiVersion = 1;
        loadOrder = 220;
        handcraftRecipeSlots = 12;
        dataFiles[] =
        {
            "ChefZ_Ingredients/Config/Ingredients/VanillaProduce.json",
            "ChefZ_Ingredients/Config/Processing/Produce.json",
            "ChefZ_Farming/Config/Processing/ProduceSeeds.json"
        };
    };

    // ### SLICE dairy ###
    //
    // Eigener Knoten und nicht derselbe wie oben: CfgChefZ fuehrt einen Knoten
    // je SLICE (02 §4), und mehrere Slices liefern in dieses Addon.
    //
    // handcraftRecipeSlots = 0: die Milchkette laeuft ausschliesslich an
    // Stationen - Vanillas Rezeptliste bleibt um kein Bit veraendert.
    class ChefZ_DairyIngredients
    {
        chefzApiVersion = 1;
        loadOrder = 260;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Ingredients/Config/Ingredients/Dairy.json"
        };
    };

    // ### SLICE salt ###
    //
    // Eigener Knoten, eigene Datei: die Slices teilen sich das Modul, aber
    // keine Datei. Der Knotenname ist der SLICE-Name, nicht der Modulname -
    // CfgChefZ traegt genau einen Knoten je Slice (02 §4).
    //
    // loadOrder 205: Salz ist Zutat FUER andere Ketten (Production Map §26),
    // nicht umgekehrt. Der Core haengt Records nicht voneinander ab; die
    // Reihenfolge ist Vorsorge und kostet nichts.
    //
    // handcraftRecipeSlots = 0: die Salzkette laeuft vollstaendig an der
    // Station ChefZ_SaltPan. Vanillas Rezeptliste bleibt um kein Bit
    // veraendert.
    class ChefZ_SaltIngredients
    {
        chefzApiVersion = 1;
        loadOrder = 205;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Ingredients/Config/Ingredients/Salt.json"
        };
    };

    // ### SLICE herbs ###
    //
    // Getrocknete Kraeuter und Gewuerze. Eigener Knoten, eigene Datei - die
    // Slices teilen sich das Modul, aber keine Datei.
    //
    // loadOrder 220: Gewuerze sind Zutat FUER andere Ketten (Wurst, Eintopf,
    // Marinade), nie umgekehrt. Der Core haengt Records nicht voneinander ab;
    // die Reihenfolge ist Vorsorge und kostet nichts.
    //
    // handcraftRecipeSlots = 0: die Kraeuterkette laeuft vollstaendig an
    // Moerser und Trockenrahmen. Vanillas Rezeptliste bleibt um kein Bit
    // veraendert.
    class ChefZ_HerbIngredients
    {
        chefzApiVersion = 1;
        loadOrder = 220;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Ingredients/Config/Ingredients/Spices.json"
        };
    };
};
