//==============================================================================
// ChefZ_Cooking - Bruehen und Saucen
//
// ABSCHNITT DES SLICE "sauces". Quellen: Production Map §52 (Tomatensauce),
// §53 (Rahmsauce), §54 (Pilzrahmsauce), §55 (Knochenbruehe), §59 (Kochgeraete);
// DME-Plan §34 (Bruehen und Sossen), §33 (Haltbarkeit), §53 (Namenskonvention).
//
// Dieses Modul ist ein GETEILTER Ordner - der Slice "serving" legt in
// Meilenstein 3 Teller und Schuesseln hier ab. Alles unterhalb eines
// Slice-Banners gehoert dem genannten Slice; wer etwas ergaenzt, HAENGT AN
// und ueberschreibt nichts.
//
// ---------------------------------------------------------------------------
// Warum diese vier Klassen der Flaschenhals der zweiten Welle sind
// ---------------------------------------------------------------------------
// DME-Plan §38 und §40 zaehlen 26 Tellergerichte und Signature Meals; ein
// grosser Teil davon nennt eine Sauce oder eine Bruehe als Zutat. Damit ein
// Gerichtsrezept spaeter NICHT auf Klassennamen zeigen muss, tragen die vier
// Klassen hier Kategorien:
//
//   SAUCE          alles, was als Sauce ueber ein Gericht kommt
//     TOMATO_SAUCE   die Tomatenbasis
//     CREAM_SAUCE    Rahm- und Pilzrahmsauce - genau das, was "Rahm-Pilz-
//                    Nudeln" und "Creamy Chicken" gemeinsam brauchen
//   BROTH          Knochenbruehe; Premium-Basis fuer Suppen und Eintoepfe (§55)
//
// Ein Gerichtsrezept schreibt dann { "category": "CREAM_SAUCE" } und erbt
// jede kuenftige Rahmsauce, ohne angefasst zu werden (08 E4).
//
// ---------------------------------------------------------------------------
// Warum jede Klasse hier FoodStages UND FoodStageTransitions traegt
// ---------------------------------------------------------------------------
// Eine Sauce ist Zutat und geht damit in den Topf. Vanilla schreibt die
// Garstufe jedes Items im Topf fort; findet FoodStage.GetNextFoodStageType
// keinen Uebergang, faellt es auf FoodStageType.BURNED zurueck
// (FoodStage.c:472, 01 V4) - die Sauce VERBRENNT beim ersten Wechsel.
//
// Die Uebergaenge stehen deshalb schon jetzt hier, obwohl dieser Slice noch
// kein Gericht baut. Sie sind die Zusage an die Gerichts-Slices der zweiten
// Welle: ihr duerft diese vier Klassen in einen Topf legen.
//
// ---------------------------------------------------------------------------
// Zwei Pflichtbloecke an jeder essbaren Klasse
// ---------------------------------------------------------------------------
// class Nutrition   PlayerStomach.InitData registriert nur Klassen, die
//                   "Nutrition" ODER "Food" haben und scope != 0
//                   (01 V7, PlayerStomach.c:208-250). Fehlt beides, wird der
//                   Bissen gegessen, verschwindet - und saettigt lautlos
//                   nichts. Es gibt dafuer keine Fehlermeldung.
//
// class Food        siehe oben.
//
// nutrition_properties[] steht in dieser Reihenfolge, woertlich aus
// FoodStage.c:
//     GetFullnessIndex(0) GetEnergy(1)  GetWater(2) GetNutritionalIndex(3)
//     GetToxicity(4)      GetAgents(5)  GetDigestibility(6)
//
// agents: eAgents.FOOD_POISON = 16 (EAgents.c). Nur der Zustand "Rotten"
// traegt ihn - eine fertig gekochte Sauce traegt keine Keime.
//
// ---------------------------------------------------------------------------
// MODELLE
// ---------------------------------------------------------------------------
// Es gibt noch keine eigene Geometrie. Die Configbasis erbt von Vanillas
// Marmalade - einem Schraubglas -, und damit erben alle vier Klassen ein
// Modell, das nicht falsch sein KANN: es kommt aus den Spieldaten und nicht
// aus einem von Hand getippten Pfad. Der Bedarf steht im Slice-Bericht.
// Kein Item wartet auf ein Modell.
//
// PFADWURZEL: das PBO-Praefix ist der ORDNERNAME des Addons. Jeder
// Laufzeitpfad beginnt deshalb mit "ChefZ_Cooking/" (02 §4.1).
//==============================================================================

class CfgPatches
{
    class ChefZ_Cooking
    {
        units[] =
        {
            // ### SLICE sauces ###
            "ChefZ_SauceItemBase",
            "ChefZ_BoneBroth",
            "ChefZ_TomatoSauce",
            "ChefZ_CreamSauce",
            "ChefZ_MushroomCreamSauce",

            // ### SLICE serving ###
            "ChefZ_ContainerItemBase",
            "ChefZ_EmptyPlate",
            "ChefZ_EmptyBowl",
            "ChefZ_EmptyCan",
            "ChefZ_EmptyJar",
            "ChefZ_EmptyBox",
            "ChefZ_PortionedDish_Base",
            "ChefZ_ServedDish_Base",

            // ### SLICE dishes-b ###   Tellergerichte 11-20 (Production Map §61)
            //
            // Je Gericht EINE Klasse (seit 29.08.2026): das Rezept liefert das
            // Gericht direkt, ohne Zwischenstufe im Kochgeraet. Der Name ist der
            // aus Production Map §72 und DME-Plan §53.
            "ChefZ_TacticalBreakfast",
            "ChefZ_ScrambledEggSausage",
            "ChefZ_FarmersBreakfast",
            "ChefZ_CheeseFlatbread",
            "ChefZ_SausageBreadPlate",
            "ChefZ_MushroomPan",
            "ChefZ_PotatoPancakes",
            "ChefZ_MeatDumplings",
            "ChefZ_MilkRice",
            "ChefZ_HoneyBreadPlate",

            // ### SLICE dishes-c ###   Suppen und Eintoepfe (Production Map §62)
            //
            // Je Gericht EINE Klasse (seit 29.08.2026): ...Bowl ist die
            // Schuessel, die im Topf entsteht und gegessen wird.
            //
            // Die Endung "Bowl" statt des blossen Gerichtsnamens ist Absicht:
            // bei einem Bowl-Gericht ist der Behaelter Teil der Identitaet -
            // das Tellergericht heisst zu Recht anders.
            "ChefZ_HunterStewBowl",
            "ChefZ_FishermanStewBowl",
            "ChefZ_VegetableSoupBowl",
            "ChefZ_BoneBrothSoupBowl",
            "ChefZ_ChernarusChiliBowl",

            // ### SLICE dishes-a ###   Tellergerichte 1-10 (Production Map §61)
            //
            // Dieselbe Bauform wie in dishes-b: eine Klasse je Gericht, der
            // blosse Gerichtsname ist der servierte Teller
            // (Config/Recipes/README_Serving.md §1, DME-Plan §53).
            "ChefZ_SurvivorSpaghetti",
            "ChefZ_SausagePasta",
            "ChefZ_HunterPasta",
            "ChefZ_CreamMushroomPasta",
            "ChefZ_MacAndCheese",
            "ChefZ_SausagePotatoes",
            "ChefZ_HunterPlate",
            "ChefZ_BloodSausagePlate",
            "ChefZ_FishPotatoPlate",
            "ChefZ_BeanSausagePlate",

            // ### SLICE dishes-vanilla ###   drei Gerichte aus den bisher
            // ungenutzten Vanilla-Assets (Vanilla-Audit §3). Dasselbe Paar je
            // Gericht wie in dishes-a bis dishes-c.
            "ChefZ_PumpkinSoupBowl",
            "ChefZ_SmallFishPan",
            "ChefZ_FruitCompoteBowl"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // Jeder Eintrag steht fuer etwas, das dieses Modul TATSAECHLICH nutzt:
        //   DZ_Data          Grundlage von allem
        //   DZ_Gear_Food     Marmalade als Configbasis und Proxy-Modell,
        //                    ausserdem die Pilzklassen und Tomato aus den
        //                    Rezeptselektoren
        //   ChefZ_Core       ChefZ_Edible_Base und die Auswertung von CfgChefZ
        //   ChefZ_Ingredients die Kategorien CREAM, BUTTER, TOMATO, SALT,
        //                    DRIED_HERB und MUSHROOM haengen an Zutaten-
        //                    datensaetzen dieses Moduls
        //   ChefZ_Farming    HERB (frische Kraeuter) und ROOT_VEGETABLE
        //                    (Zwiebel, Karotte) fuer die Bruehe
        //   ChefZ_Meat       BONE - der Zutatendatensatz zu Vanillas "Bone"
        //                    liegt dort (Config/Ingredients/Meat.json)
        // Nicht mehr und nicht weniger - eine zu breite Liste verschiebt die
        // Ladereihenfolge fremder Mods ohne Grund. Kochgeraete (Pot, Cauldron,
        // FryingPan) stehen NICHT hier: sie werden in Rezeptkontexten nur
        // BENANNT, nicht abgeleitet.
        //   ### SLICE serving ### zwei weitere Eintraege, und nur zwei:
        //   DZ_Gear_Cooking  die Proxy-Modelle CookingPot.p3d und FryingPan.p3d
        //                    der Behaelter und der beiden Gerichtebasen
        //   ChefZ_Processing die Werkzeuggruppe CUTTING_TOOL der beiden
        //                    Schnitzvorgaenge (Brennholz + Messer -> Teller)
        //   ### SLICE dishes-b ### GENAU EIN weiterer Eintrag:
        //   ChefZ_Baking     die Kategorien BREAD und DOUGH haengen an
        //                    ChefZ_Bread, ChefZ_Flatbread und ChefZ_Dough - alle in
        //                    ChefZ_Baking/Config/GrainIngredients.json.
        //   Alles Weitere, was dieser Slice anfasst, steht bereits oben:
        //   FLOUR liegt in ChefZ_Processing, EGG/DAIRY/SALT/SPICE/MUSHROOM und
        //   Salz in ChefZ_Ingredients, HERB, Kartoffel (Vanilla) und die
        //   Zwiebel in ChefZ_Farming, SAUSAGE/MINCED_MEAT/FAT in ChefZ_Meat.
        //   ChefZ_Preservation steht bewusst NICHT hier: geraeucherte und
        //   getrocknete Wurst erreicht dieser Slice ueber die Kategorie
        //   SAUSAGE, nicht ueber eine Klasse aus jenem Modul (08 E4).
        //   Vanilla-Zutaten (Potato, Rice, Honey, TacticalBaconCan_Opened)
        //   liegen in DZ_Gear_Food und sind bereits abgedeckt.
        //   ### SLICE dishes-c ### GENAU EIN weiterer Eintrag:
        //   ChefZ_Preservation die Kategorie FISH existiert nur, weil jenes
        //                    Modul sie deklariert, und die vier Fischfilets
        //                    (CarpFilletMeat, MackerelFilletMeat,
        //                    SteelheadTroutFilletMeat, WalleyePollockFilletMeat)
        //                    haben ihren Zutatendatensatz dort
        //                    (Config/Ingredients/Preservation.json). Ohne diesen
        //                    Eintrag bindet der Fisch-Slot des Fisherman's Stew
        //                    an nichts. Anders als bei dishes-b ist das KEIN
        //                    Umweg ueber eine Kategorie eines anderen Moduls -
        //                    die Kategorie selbst kommt von dort.
        //   Alles Weitere dieses Slice steht bereits oben: WILD_MEAT/MEAT in
        //   ChefZ_Meat, ROOT_VEGETABLE/LEAF_VEGETABLE/VEGETABLE/HERB in
        //   ChefZ_Farming, MUSHROOM/SALT/SPICE/TOMATO und die Paprikaklassen in
        //   ChefZ_Ingredients, BROTH in diesem Modul selbst (Slice sauces),
        //   BakedBeansCan_Opened in DZ_Gear_Food.
        //   ### SLICE dishes-a ### KEIN weiterer Eintrag. Geprueft, Zeile fuer
        //   Zeile: PASTA haengt an ChefZ_Baking (GrainIngredients.json), FISH an
        //   ChefZ_Preservation, SAUSAGE/MEAT/WILD_MEAT/FAT an ChefZ_Meat,
        //   DAIRY/CREAM/BUTTER/SALT/SPICE/MUSHROOM an ChefZ_Ingredients, HERB
        //   und ROOT_VEGETABLE an ChefZ_Farming, TOMATO_SAUCE/SAUCE/CREAM_SAUCE
        //   an diesem Modul selbst (Slice sauces). Die beiden Vanilla-Zutaten
        //   dieses Slice - "Potato" und "BakedBeansCan_Opened" - liegen in
        //   DZ_Gear_Food, die Proxy-Modelle CookingPot.p3d und FryingPan.p3d in
        //   DZ_Gear_Cooking. Beides steht bereits oben.
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Gear_Food",
            "DZ_Gear_Cooking",
            "ChefZ_Core",
            "ChefZ_Items",
            "ChefZ_Food",
            "ChefZ_Ingredients",
            "ChefZ_Farming",
            "ChefZ_Meat",
            "ChefZ_Processing",
            "ChefZ_Baking",
            "ChefZ_Preservation"
        };
    };
};

// ---------------------------------------------------------------------------
// Skriptmodul dieses PBO.
//
// Es braucht einen eigenen CfgMods-Knoten, weil der Knoten des Core
// ausschliesslich Pfade unterhalb von "ChefZ_Core/" nennt - und das
// PBO-Praefix die Wurzel JEDES Laufzeitpfades ist (02 §4.1). Ohne diesen
// Block laedt DayZ "ChefZ_Cooking/Scripts/..." still nicht: kein Fehler,
// kein RPT-Eintrag, nur eine Klasse, die es zur Laufzeit nicht gibt.
//
// Der Knoten heisst ChefZ_CookingMod und nicht ChefZ_Cooking: der Name
// ChefZ_Cooking ist bereits an CfgPatches vergeben, und zwei gleichnamige
// Klassen in derselben config.cpp sind eine doppelte Definition. Fuer die
// Engine ist der Name hier belanglos; was zaehlt, ist "dir".
// ---------------------------------------------------------------------------
class CfgMods
{
    class ChefZ_CookingMod
    {
        dir = "ChefZ_Cooking";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Cooking";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        dependencies[] = {"World"};

        class defs
        {
            // Nur 4_World: dieses Modul bringt kein System mit, sondern genau
            // eine Skriptbasis fuer seine Items. Systeme gehoeren in den Core
            // (Workflow §10.3).
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Cooking/Scripts/4_World"
                };
            };
        };
    };
};

class CfgVehicles
{
    // ### SLICE sauces ### Proxy-Basis: Vanillas Schraubglas.
    class Marmalade;

    //==========================================================================
    // ### SLICE sauces ### Gemeinsame Configbasis
    //
    // scope = 0: sie ist kein Item, sie ist die Stelle, an der Garstufen,
    // Uebergaenge und Grundeigenschaften EINMAL stehen. Der Core bringt keine
    // solche Basis mit (Invariante I3, Kopf von ChefZ_Edible_Base.c: "Wer eine
    // gemeinsame Configbasis mit scope = 0 haben will, legt sie in SEINEM
    // Modul an").
    //
    // Configbasis ist eine VANILLA-Klasse, Skriptbasis ist ChefZ_Edible_Base -
    // genau die Andockregel aus demselben Dateikopf.
    //
    // varQuantity 0..100: eine Sauce ist eine Menge und kein Stueck. Ein
    // Gericht nimmt sich einen Teil davon, der Rest bleibt im Glas.
    //==========================================================================
    class ChefZ_SauceItemBase : Marmalade
    {
        scope = 0;
        rotationFlags = 17;
        itemSize[] = {2, 2};
        weight = 400;
        absorbency = 0.0;
        soundImpactType = "food";
        isMeleeWeapon = 0;
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;

        class Food
        {
            class FoodStages
            {
                // visual_properties[] = { selectionIndex, textureIndex, materialIndex }
                // Das Proxy-Glas ist einteilig und hat kein verstecktes
                // Selection-Set - deshalb ueberall 0. Sobald es eigene
                // Geometrie gibt, wird genau hier umgeschaltet.
                //
                // cooking_properties[] = { minTemp, cookTime, maxTemp }
                // woertlich aus enum eCookingPropertyIndices (FoodStage.c:15).
                // Eine fertige Sauce ist bereits gekocht; sie wird nur noch
                // warmgehalten. Deshalb lange cookTime bis "Burned": wer sie
                // im Topf vergisst, verliert sie - aber nicht sofort.
                class Raw
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                };
                class Baked
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {70, 120, 200};
                };
                class Boiled
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {70, 120, 150};
                };
                class Burned
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {200, 40, 0};
                };
                class Rotten
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                };
            };

            // OHNE DIESEN BLOCK VERBRENNT JEDE SAUCE DES MODULS (01 V4).
            //
            // transition_to und cooking_method sind ZAHLEN, nicht Namen:
            // SetupFoodStageTransitionMapping liest sie mit ConfigGetInt
            // (FoodStage.c:167ff).
            //   FoodStageType:     RAW 1, BAKED 2, BOILED 3, DRIED 4, BURNED 5, ROTTEN 6
            //   CookingMethodType: NONE 0, BAKING 1, BOILING 2, DRYING 3, TIME 4
            //
            // Nur der Uebergang AUS "Raw": aus "Baked" gibt es keinen, und
            // genau deshalb liefert GetNextFoodStageType dort BURNED - eine
            // Sauce, die im Feuer stehen bleibt, brennt an. Das ist gewollt.
            //
            // DRYING fehlt absichtlich: Trocknen und Raeuchern laufen in ChefZ
            // an eigenen Stationen (11 E6) und nicht in Vanillas
            // Smoking-Slots - und eine Sauce trocknet ohnehin nicht.
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
    // §55 / DME §34: Knochenbruehe.
    //
    // Wenig Energie, viel Wasser, hoher nutritionalIndex - genau das, was eine
    // Bruehe leistet: sie ersaettigt nicht, sie traegt. §55 nennt sie die
    // "Premium-Basis" fuer Suppen, Eintoepfe und Wildgerichte, und das ist
    // hier eine Aussage ueber den Naehrwertindex, nicht ueber die Kalorien.
    //==========================================================================
    class ChefZ_BoneBroth : ChefZ_SauceItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BONEBROTH";
        descriptionShort = "#STR_CHEFZ_ITEM_BONEBROTH_DESC";
        weight = 520;
        itemSize[] = {2, 3};
        // Gekocht = mittlere Haltbarkeit (DME §33). Eine wasserreiche Bruehe
        // steht am unteren Rand dieser Stufe.
        lifetime = 14400;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.0 x 100 Einheiten = 300 - Sauce/Bruehe im Glas.
            // Alt: 60 x 100 = 6000, das 3-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.0;
            energy = 150;
            water = 320;
            nutritionalIndex = 45;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {3.0, 150, 320, 45, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {3.0, 158, 280, 45, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {3.0, 150, 320, 45, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.75, 38, 40, 8, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {1.2, 60, 128, 9, 20, 16, 1}; };
            };
        };
    };

    //==========================================================================
    // §52 / DME §34: Tomatensauce.
    //
    // Drei Tomaten und Salz, im Topf eingekocht. Wasserreich, kalorienarm -
    // sie ist Traeger fuer Pasta und Eintopf, kein Gericht.
    //==========================================================================
    class ChefZ_TomatoSauce : ChefZ_SauceItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_TOMATOSAUCE";
        descriptionShort = "#STR_CHEFZ_ITEM_TOMATOSAUCE_DESC";
        weight = 420;
        // Eingekochtes Fruchtfleisch im Glas haelt laenger als die Bruehe.
        lifetime = 28800;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 2.3 x 100 Einheiten = 230 - Sauce/Bruehe im Glas.
            // Alt: 45 x 100 = 4500, das 2-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 2.3;
            energy = 130;
            water = 105;
            nutritionalIndex = 35;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {2.3, 130, 105, 35, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {2.3, 135, 85, 35, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {2.3, 130, 125, 35, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.58, 33, 13, 7, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.92, 52, 42, 7, 20, 16, 1}; };
            };
        };
    };

    //==========================================================================
    // §53 / DME §34: Rahmsauce.
    //
    // Sahne, Butter, Salz. Die energiereichste der vier - Fett bringt sie
    // dorthin, nicht Menge.
    //==========================================================================
    class ChefZ_CreamSauce : ChefZ_SauceItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CREAMSAUCE";
        descriptionShort = "#STR_CHEFZ_ITEM_CREAMSAUCE_DESC";
        weight = 400;
        // Milchfett verdirbt schneller als eingekochtes Gemuese.
        lifetime = 18000;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 2.0 x 100 Einheiten = 200 - Sauce/Bruehe im Glas.
            // Alt: 40 x 100 = 4000, das 2-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 2.0;
            energy = 620;
            water = 90;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {2.0, 620, 90, 20, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {2.0, 635, 70, 20, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {2.0, 620, 100, 20, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.5, 155, 12, 4, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.8, 248, 36, 4, 20, 16, 1}; };
            };
        };
    };

    //==========================================================================
    // §54 / DME §34: Pilzrahmsauce.
    //
    // Pilze, Sahne, Petersilie, Salz. Weniger Fett als die reine Rahmsauce,
    // dafuer der hoechste Naehrwertindex der drei Saucen - die Pilze bringen
    // ihn.
    //==========================================================================
    class ChefZ_MushroomCreamSauce : ChefZ_SauceItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MUSHROOMCREAMSAUCE";
        descriptionShort = "#STR_CHEFZ_ITEM_MUSHROOMCREAMSAUCE_DESC";
        weight = 440;
        lifetime = 18000;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 2.3 x 100 Einheiten = 230 - Sauce/Bruehe im Glas.
            // Alt: 45 x 100 = 4500, das 2-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 2.3;
            energy = 330;
            water = 130;
            nutritionalIndex = 40;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {2.3, 330, 130, 40, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {2.3, 340, 110, 40, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {2.3, 330, 140, 40, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.58, 83, 16, 8, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.92, 132, 52, 8, 20, 16, 1}; };
            };
        };
    };

    //==========================================================================
    // ### SLICE serving ###   Behaelter und die beiden Gerichtebasen
    //
    // Quellen: Architekturplan §17-§19, Production Map §60, Planungsschritte
    // §11-§12, Entwuerfe 15 (Portion System) und 16 (Container System),
    // Entscheidung OF-04 (Behaelter beim SERVIEREN, wiederverwendbar).
    //
    // Der CORE ist dabei unveraendert geblieben: Portion Manager, Container
    // Registry, ChefZ_ActionTakePortion, ChefZ_PortionedFood_Base,
    // ChefZ_Container_Base und die Rueckgabe in ChefZ_Edible_Base.OnConsume
    // sind vollstaendig vorhanden. Die Ausnahmeerlaubnis dieses Slice,
    // generische Systemteile im Core zu ergaenzen, wurde NICHT gebraucht.
    //
    // KEIN Gericht steht hier. Gerichte gehoeren dishes-a/b/c und sauces.
    //
    // Warum der Behaelter beim SERVIEREN gebraucht wird und nie beim Kochen
    // (belegt, 01 V3 / 16 §2): Cooking.ProcessItemToCook behandelt JEDES
    // Cargo-Item - es nimmt Temperatur auf und nimmt bei !IsCookware()
    // Hitzeschaden ueber PARAM_BURN_DAMAGE_COEF (5 % pro Tick). Ein Teller im
    // Topf ginge im Feuer kaputt und veraenderte zusaetzlich die
    // Gefaesssignatur, die der Matcher liest. Die Regel lautet einheitlich:
    // ein Behaelter wird genau dann gebraucht, wenn etwas hineinkommt.
    //
    // MODELLE: Vanilla-Proxys. Bedarf im Slice-Bericht und im Asset-Backlog
    // (§4 der 3D-Asset-Liste fuehrt EmptyPlate und EmptyBowl als P1). Kein
    // Item wartet auf ein Modell.
    //==========================================================================

    class Inventory_Base;
    class Edible_Base;

    // ------------------------------------------------------------------------
    // Gemeinsame Configbasis der Behaelter.
    //
    // scope = 0: kein Item, sondern die Stelle, an der die gemeinsamen
    // Eigenschaften EINMAL stehen (Invariante I3 - der Core bringt keine
    // solche Basis mit).
    //
    // Configbasis ist Inventory_Base und NICHT Vanillas Container_Base:
    // Container_Base bringt Cargo mit, und die Skriptbasis dieses Slice
    // (ChefZ_Container_Base) leitet laut 16 §3.2 von ItemBase ab. Ein leerer
    // Teller hat kein Cargo - und ChefZ_ContainerService.IsEmpty() prueft
    // genau "kein Cargo und keine Menge", bevor er einen Behaelter
    // verbraucht. Ein Behaelter MIT Cargo waere einer, den man versehentlich
    // voll macht und der dann beim Servieren nicht mehr zaehlt.
    //
    // Kein ChefZ-Zustandskomponent: jedes Zustandsfeld waere Sync- und
    // Spielstandslast auf einem Item, dessen ganzer Zweck es ist, leer zu
    // sein. Fuer ein spaeteres Hygienesystem (OF-06 -> C, nicht in V1)
    // genuegt Vanillas varCleanness, das ChefZ_ItemFacts bereits als
    // cleanness01 fuehrt (16 E6).
    // ------------------------------------------------------------------------
    class ChefZ_ContainerItemBase : Inventory_Base
    {
        scope = 0;
        model = "\dz\gear\cooking\CookingPot.p3d";
        rotationFlags = 17;
        itemSize[] = {2, 2};
        weight = 300;
        absorbency = 0;
        canBeSplit = 0;
        isMeleeWeapon = 0;
        varQuantityDestroyOnMin = 0;
    };

    // §60, §18: Teller. Traeger der Tellergerichte aus §61.
    //
    // reusable = 1 (OF-04): der Teller kommt beim vollstaendigen Verzehr
    // zurueck und ist damit dauerhafte Ausruestung statt Verbrauchsgut. Der
    // Frustfall "mein Eintopf ist fertig, aber ich habe keine Schuessel"
    // trifft einmal, nicht bei jeder Mahlzeit.
    class ChefZ_EmptyPlate : ChefZ_ContainerItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_EMPTYPLATE0";
        descriptionShort = "#STR_CHEFZ_ITEM_EMPTYPLATE1";
        // PROXY: flache runde Silhouette. Bedarf: eigenes Tellermesh (P1).
        model = "\dz\gear\cooking\FryingPan.p3d";
        itemSize[] = {2, 2};
        weight = 240;
    };

    // §60, §18: Schuessel. Traeger der Suppen und Eintoepfe aus §62 und damit
    // der eigentliche Behaelter des Portionssystems (§17).
    //
    // ZWEI HERKUENFTE, EINE KLASSE: TR_CarveWoodenBowl (Brennholz + Messer,
    // Tableware.json) und TR_BowlFromBark (Rinde + Axt oder Messer,
    // Containers.json). Die "Holzschale" des Auftrags ist genau dieses Item -
    // Anzeigename und Beschreibung nennen sie seit jeher eine GESCHNITZTE
    // HOLZschuessel. Eine zweite Schalenklasse haette dieselbe Geometrie,
    // dasselbe Gewicht, dieselbe Behaelterkategorie BOWL und denselben Zweck
    // gehabt; jedes Bowl-Rezept mit returnContainer "AUTO" und jede
    // Behaelterpruefung haette sie zusaetzlich kennen muessen. Ein zweiter
    // WEG zum selben Gegenstand ist ein Transform, keine Klasse.
    class ChefZ_EmptyBowl : ChefZ_ContainerItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_EMPTYBOWL0";
        descriptionShort = "#STR_CHEFZ_ITEM_EMPTYBOWL1";
        // PROXY: Topfsilhouette, zu gross und mit Buegel. Bedarf: eigenes
        // Schuesselmesh (P1).
        model = "\dz\gear\cooking\CookingPot.p3d";
        itemSize[] = {2, 2};
        weight = 300;
    };

    // §60 (optional), §18: Konservendose.
    //
    // Der Konservenfall aus OF-04: reusable = 0. Wer eine Dose oeffnet,
    // bekommt keine leere Dose zurueck - sie geht im Gericht auf. Einkochen
    // selbst ist laut §74 V2; Kategorie und Item stehen bereit, damit ein
    // spaeteres Konservenmodul KEINE Core- und keine Rezeptaenderung braucht
    // (16 E8).
    //
    // HERKUNFT (DME-Plan §32): TR_CansFromMetalSheet in
    // Config/Processing/Containers.json - ein MetalPlate plus Hacksaw ergibt
    // ZEHN Stueck. Bis dahin war diese Klasse ein Gegenstand ohne jede
    // Rezeptreferenz; der Vanilla-Audit empfahl deshalb, sie unsichtbar zu
    // schalten. Sie bleibt sichtbar, weil sie jetzt einen Weg hat.
    class ChefZ_EmptyCan : ChefZ_ContainerItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_EMPTYCAN0";
        descriptionShort = "#STR_CHEFZ_ITEM_EMPTYCAN1";
        // PROXY, dauerhaft: eine GEOEFFNETE Vanilla-Konservendose - genau das,
        // was eine leere Dose ist. Der Pfad ist in den Spieldaten belegt
        // (Asset-Backlog: acht Fundstellen) und loest den vorherigen
        // PowderedMilk-Folienbeutel ab, der als Blechdose nicht trug. Kein
        // eigenes Mesh mehr noetig.
        model = "\dz\gear\food\food_can_open.p3d";
        itemSize[] = {1, 2};
        weight = 70;
    };

    // §60 (optional), §18: Einmachglas. "Eingemachtes haelt laenger" ist eine
    // ZAHL am Behaelter (spoilageModifier, 16 E7) und kein eigener Zustand -
    // beim Oeffnen faellt schlicht der Faktor weg.
    class ChefZ_EmptyJar : ChefZ_ContainerItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_EMPTYJAR0";
        descriptionShort = "#STR_CHEFZ_ITEM_EMPTYJAR1";
        // EIGENES MODELL (29.08.2026): das leere Glas aus ChefZ_Items.
        model = "\ChefZ\ChefZ_Items\models\glass_jar.p3d";   // EIGENES MODELL (30.08.2026): glass_jar ersetzt jar
        itemSize[] = {1, 2};
        weight = 210;
    };

    // §18, DME-Plan §32: Lebensmittelbox fuer Trockenware.
    //
    // HERKUNFT (DME-Plan §32, "Paper + Paper -> Cardboard Food Box"):
    // TR_BoxFromPaper in Config/Processing/Containers.json. Zwei Blatt Papier,
    // kein Werkzeug - der Transform hat damit zwei Zutatenplaetze und ist
    // genau ausgereizt (01 V12).
    class ChefZ_EmptyBox : ChefZ_ContainerItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_EMPTYBOX0";
        descriptionShort = "#STR_CHEFZ_ITEM_EMPTYBOX1";
        // PROXY: Kartonsilhouette. Bedarf: eigenes Boxmesh (P3, V2).
        model = "\ChefZ\ChefZ_Items\models\box_open.p3d";   // EIGENES MODELL (30.08.2026): die offene Box; box.p3d (zu) bleibt Reserve
        itemSize[] = {2, 2};
        weight = 110;
    };

    // ------------------------------------------------------------------------
    // Die Basis eines Bulk-Gerichts im Kochgefaess (15 §2).
    //
    // OHNE CONTENT seit 29.08.2026: die Rezepte liefern das Gericht direkt,
    // es gibt keine Zwischenstufe mehr (Entscheidung des Auftraggebers,
    // "Portionsgebinde wird nicht noetig sein"). Die Basis bleibt als
    // Faehigkeit des Core stehen - ein Modul, das sie braucht, findet sie
    // vor; die 28 Gerichte dieses Moduls benutzen sie nicht.
    //
    //     config.cpp   class ChefZ_<Name>Bulk : ChefZ_PortionedDish_Base
    //     script       class ChefZ_<Name>Bulk extends ChefZ_PortionedDish_Base {}
    //     Rezept       outputs[0] traegt portions, portionClass,
    //                  amountPerPortion, containerCategory, returnContainer
    //
    // WARUM DIE GARSTUFENUEBERGAENGE HIER STEHEN (01 V4, zwingend):
    // ChefZ_PortionedFood_Base liefert CanBeCooked() == true - das Bulk DARF
    // im Topf warmgehalten werden (15 §3). Damit laeuft Vanillas
    // Garstufenfortschreibung darauf; findet FoodStage.GetNextFoodStageType
    // keinen Uebergang, faellt es auf FoodStageType.BURNED zurueck
    // (FoodStage.c:472). Ein Kessel Eintopf wuerde beim ersten Wechsel zu
    // Kohle. Die Uebergaenge sind deshalb keine Kuer, sondern die Bedingung
    // dafuer, dass ein Gruppengericht ueberhaupt am Feuer stehen bleiben kann.
    //
    // Ueberhitzen bleibt moeglich und ist gewollt (15 §2: "warm halten ja,
    // ueberhitzen ja"): aus Baked und aus Boiled fuehrt je ein Uebergang nach
    // Burned. Wer den Kessel vergisst, verliert ihn.
    //
    // nutrition_properties[] stehen NICHT hier. Sie gehoeren an das einzelne
    // Gericht - eine geerbte Naehrwertzeile waere ein stiller Default, der
    // jede vergessene Angabe im Validator gruen aussehen liesse (01 V7).
    // ------------------------------------------------------------------------
    class ChefZ_PortionedDish_Base : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\cooking\CookingPot.p3d";
        rotationFlags = 17;
        itemSize[] = {3, 2};
        weight = 900;
        absorbency = 0.7;
        // MENGE 100 UND EINE LEISTE, WIE DER APFEL (Entscheidung vom
        // 29.08.2026: der Verzehrzustand ist der Restwert auf der Leiste,
        // keine Geometrie- oder Texturstufe). PlayerStomach.c:92 teilt energy
        // und water durch 100 Mengeneinheiten: mit varQuantityMax = 1 kam von
        // energy = 690 genau 6,9 im Magen an - ein Hundertstel. Ein Bissen
        // (UAQuantityConsumed.EAT_BIG = 25) ist jetzt ein Viertel der Portion.
        //
        // ACHTUNG, DIESELBE ZEILE GILT FUER fullnessIndex NICHT. Die
        // Herleitung des Magenvolumens steht vollstaendig an
        // ChefZ_ServedDish_Base, ein Stueck weiter unten in dieser Datei.
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;
        quantityBar = 1;
        canBeSplit = 0;
        isMeleeWeapon = 0;

        class Food
        {
            class FoodStages
            {
                // visual_properties[]  = { selectionIndex, textureIndex, materialIndex }
                // cooking_properties[] = { minTemp, cookTime, maxTemp }
                //   (eCookingPropertyIndices, FoodStage.c:15)
                //
                // Die Garzeiten liegen ueber denen eines Einzelstuecks
                // (ChefZ_MeatItemBase: 60 / 80): ein voller Kessel wird
                // langsamer gar als eine Wurst, und das Fenster bis zum
                // Verbrennen soll gross genug sein, dass eine Gruppe ihre
                // Portionen entnehmen kann.
                class Raw
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                };
                class Baked
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {100, 90, 200};
                };
                class Boiled
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {100, 110, 150};
                };
                class Burned
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {200, 30, 0};
                };
                class Rotten
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                };
            };

            // transition_to und cooking_method sind ZAHLEN, nicht Namen
            // (SetupFoodStageTransitionMapping liest sie mit ConfigGetInt,
            // FoodStage.c:167ff):
            //   FoodStageType:     RAW 1, BAKED 2, BOILED 3, DRIED 4, BURNED 5, ROTTEN 6
            //   CookingMethodType: NONE 0, BAKING 1, BOILING 2, DRYING 3, TIME 4
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_BulkRawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_BulkRawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
                class Baked
                {
                    class ChefZ_BulkBakedToBurned
                    {
                        transition_to = 5;
                        cooking_method = 1;
                    };
                };
                class Boiled
                {
                    class ChefZ_BulkBoiledToBurned
                    {
                        transition_to = 5;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // ------------------------------------------------------------------------
    // Die servierte Portion - das, was der Spieler in der Hand haelt (15 §2).
    //
    //     config.cpp   class ChefZ_HunterStewBowl : ChefZ_ServedDish_Base
    //     script       class ChefZ_HunterStewBowl extends ChefZ_ServedDish_Base {}
    //     CfgChefZIngredients  returnContainer = "AUTO" oder eine feste Klasse
    //
    // BEWUSST OHNE Food-Knoten: eine servierte Portion liegt auf dem Teller
    // und nicht mehr im Feuer. Ohne "Food FoodStages" liefert
    // ItemBase.HasFoodStage() false (ItemBase.c:2654), es entsteht gar kein
    // FoodStage-Objekt - und damit kann die Portion auch nicht verbrennen.
    //
    // Genau deshalb steht hier AUCH KEIN Nutrition-Block: er wuerde geerbt und
    // liesse ein Gericht, das seinen eigenen vergessen hat, im Validator gruen
    // aussehen. PlayerStomach registriert nur Klassen mit eigenem Naehrwert
    // (01 V7) - der Fehler waere sonst still.
    // ------------------------------------------------------------------------
    class ChefZ_ServedDish_Base : Edible_Base
    {
        scope = 0;
        // GEMEINSAMES SPEISEN-MESH (Lieferung a78a247, 01.09.2026).
        // Vanillas FryingPan.p3d traegt KEINE Textur-Selektion - nur
        // Engine-Selektionen und eine fest verdrahtete frying_pan_co.paa;
        // hiddenSelectionsTextures waere dort tote Config gewesen.
        // Panfood_Base bringt die Sektion "camo" mit (model.cfg des
        // Asset-PBOs), also traegt EIN Mesh alle Tellergerichte und jede
        // Klasse nur noch ihre eigene Textur.
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelections[] = {"camo"};
        rotationFlags = 17;
        itemSize[] = {2, 2};
        weight = 450;
        absorbency = 0.5;
        // MENGE 100 UND EINE LEISTE, WIE DER APFEL (Entscheidung vom
        // 29.08.2026: der Verzehrzustand ist der Restwert auf der Leiste,
        // keine Geometrie- oder Texturstufe). PlayerStomach.c:92 teilt energy
        // und water durch 100 Mengeneinheiten: mit varQuantityMax = 1 kam von
        // energy = 690 genau 6,9 im Magen an - ein Hundertstel. Ein Bissen
        // (UAQuantityConsumed.EAT_BIG = 25) ist jetzt ein Viertel der Portion.
        //
        // ------------------------------------------------------------------
        // MAGENVOLUMEN - DIE HERLEITUNG JEDES fullnessIndex IN DIESEM MODUL
        // ------------------------------------------------------------------
        // Sie steht hier EINMAL; jede essbare Klasse des Moduls verweist mit
        // einer Zeile darauf. Anlass ist ein im Spiel bestaetigter Fehler:
        // nach JEDEM ChefZ-Gericht hat sich der Charakter uebergeben.
        //
        // Die Ursache ist ein Bruch in der Vanilla-Rechnung, den die alten
        // Kommentare dieses Moduls uebersehen haben. energy und water werden
        // je Mengeneinheit durch 100 geteilt:
        //
        //     PlayerStomach.c:92  float energy_per_unit = profile.GetEnergy() / 100;
        //     PlayerStomach.c:93  float water_per_unit  = profile.GetWaterContent() / 100;
        //
        // fullnessIndex NICHT. Er geht als Faktor PRO Mengeneinheit direkt
        // ins Magenvolumen, ohne jede Division:
        //
        //     PlayerStomach.c:86  volume = m_Profile.GetFullnessIndex() * m_Amount;
        //
        // (StomachItem.ProcessDigestion; PlayerStomach.c:304-317 summiert das
        // Ergebnis ueber alle Posten in m_StomachVolume.)
        //
        // Daraus folgt die INVARIANTE dieses Moduls:
        //
        //     fullnessIndex x varQuantityMax = Magenvolumen des GANZEN Gerichts
        //
        // Die Grenzen dafuer setzt Vanilla, nicht der Geschmack:
        //
        //     PlayerConstants.c:208  VOMIT_THRESHOLD = 2000
        //         darueber loest VomitStuffed.c aus - der Charakter kotzt.
        //     PlayerConstants.c:200  BT_STOMACH_VOLUME_LVL3 = 1000
        //         ab hier zeigt die HUD-Anzeige "Stuffed".
        //
        // Abgebaut wird nur mit PlayerConstants.DIGESTION_SPEED (rund 1,7
        // Einheiten je Sekunde) - ein zu hoher Wert laesst sich nicht
        // aussitzen.
        //
        // WARUM DER ALTE STAND KOTZEN MUSSTE: die Werte lagen bei 30 bis 300
        // bei varQuantityMax 200 bis 1200. ChefZ_SausageBreadPlate stand auf
        // 110; ein einziger Bissen (UAQuantityConsumed.EAT_BIG = 25,
        // ActionConstants.c:9) ergab 110 x 25 = 2750 Volumen und damit sofort
        // mehr als VOMIT_THRESHOLD. Es war kein Balancingfehler, sondern ein
        // Rechenfehler um den Faktor 100.
        //
        // DIE ZIELBAENDER (Magenvolumen des ganzen Gerichts, nie ueber 950 -
        // ein volles Gericht darf saettigen, aber nie allein die Schwelle
        // reissen):
        //
        //     ueppiges Gericht            850 - 950
        //     Hauptgericht-Teller         600 - 850
        //     Pfannen-/leichtes Gericht   300 - 500
        //     Snack                       200 - 350
        //     Bowl (varQuantityMax 1200)  600 - 850, fullnessIndex also ~0.5-0.7
        //     Sauce/Bruehe im Glas        200 - 300
        //
        // Die relative Reihenfolge der alten Werte bleibt erhalten: gerechnet
        // wurde gegen das ALTE Gesamtvolumen (alter fullnessIndex x
        // varQuantityMax), weil genau das die Groesse ist, die der Magen sieht.
        //
        // Die Zahlen liegen damit ueber dem Vanilla-Band von rund 0.75 bis 2.5,
        // und das ist richtig: ein ChefZ-Teller ist ein Mehrportionen-Item mit
        // varQuantityMax 200 bis 1200, ein Vanilla-Apfel ein Einzelstueck.
        // Verglichen werden muss das Produkt, nicht der Index.
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;
        quantityBar = 1;
        canBeSplit = 0;
        isMeleeWeapon = 0;
    };

    //==========================================================================
    //==========================================================================
    // ### SLICE dishes-b ###   TELLERGERICHTE 11-20
    //
    // Production Map §61.11-§61.20, DME-Plan §38.11-§38.20, §41 (Rezeptqualitaet),
    // §42 (Gerichtsnutzen), §43 (Food-Buffs), §53 (Namenskonvention).
    //
    // --------------------------------------------------------------------------
    // Warum je Gericht EINE Klasse (seit 29.08.2026)
    // --------------------------------------------------------------------------
    // Das Rezept liefert das servierte Gericht direkt im Kochgeraet - ohne
    // Zwischenstufe (Bulk) und ohne Entnahmeaktion. Mehrere Portionen sind
    // MENGE am einen Item: PlayerStomach.c:92 rechnet energy und water je 100
    // Einheiten ab, das Rezept setzt quantity = 100 x Portionen, und die
    // Klasse traegt varQuantityMax fuer ihr groesstes Rezept. Wer aus dem Topf
    // isst, isst Portion fuer Portion vom selben Item. fullnessIndex faellt
    // NICHT unter diese Rechnung - er wirkt mal der Menge
    // (PlayerStomach.c:86); die Herleitung steht an ChefZ_ServedDish_Base.
    //
    // Die Klasse traegt den Namen aus Production Map §72 / DME-Plan §53
    // (ChefZ_TacticalBreakfast). Qualitaetsvarianten
    // je Stufe gibt es bewusst NICHT: OF-05 ist als B entschieden (Ausbeute
    // statt eigener Klasse je Stufe), und 25 Gerichte x 4 Stufen waeren 100
    // Klassen mit Modell, Stringtable und Loot-Eintrag.
    //
    // --------------------------------------------------------------------------
    // Warum das Gericht keinen Food-Knoten hat
    // --------------------------------------------------------------------------
    // Das Gericht erbt von ChefZ_ServedDish_Base, das bewusst KEINEN Food-Knoten
    // hat: HasFoodStage() liefert false (ItemBase.c:2654), Vanillas Cooking
    // fragt CanBeCooked() und laesst es liegen (Cooking.c:47) - ein fertiges
    // Gericht kann im Topf nicht mehr verbrennen, es wird nur warm gehalten.
    // Sie braucht deshalb "class Nutrition" - und zwar eine EIGENE, weil beide
    // Basen absichtlich keine vererben (01 V7: eine geerbte Naehrwertzeile liesse
    // ein Gericht, das seinen eigenen vergessen hat, im Validator gruen aussehen).
    //
    // --------------------------------------------------------------------------
    // nutrition_properties[] - die Reihenfolge, nachgeschlagen
    // --------------------------------------------------------------------------
    //   {fullnessIndex, energy, water, nutritionalIndex, toxicity, agents, digestibility}
    // Burned: ein Viertel Energie, ein Sechstel Wasser. Rotten: zusaetzlich
    // toxicity und agents, wie bei ChefZ_BoneBroth im selben Modul.
    //
    // --------------------------------------------------------------------------
    // 3D-Assets
    // --------------------------------------------------------------------------
    // Es gibt fuer KEINES dieser zehn Items ein eigenes Mesh. Alle tragen ein
    // Vanilla-Proxy: FryingPan.p3d (flach, liest sich als Teller), der Milchreis
    // in der Schuessel CookingPot.p3d. Sobald eigene Geometrie existiert, wird genau diese eine
    // Zeile je Klasse getauscht - sonst nichts.
    //
    // --------------------------------------------------------------------------
    // REZEPTVORRANG - warum jedes der zehn Rezepte priority = 0 traegt
    // --------------------------------------------------------------------------
    // 09 E1 ist eindeutig: die Spezifitaet entscheidet, die Handzahl daempft nur
    // (priorityScale 0.01). Eine Zahl ungleich 0 ist damit ein SIGNAL im Review -
    // und dieser Slice braucht keins. Nachgerechnet gegen alles, was heute
    // ausserdem am Kochgeraet zuenden kann (09 §4.1):
    //
    //   RCP_CookSausage & Co. (ChefZ_Meat)  cls 3.0 + 3 Geraete 1.5      = 4.5
    //       ACHTUNG: die tragen extraItems "ignore" und matchen deshalb AUCH,
    //       wenn Brot und Kaese danebenliegen. Jedes Gericht dieses Slice liegt
    //       mit 6.7 bis 8.5 darueber - der Wurstbrot-Teller mit 8.1 - und
    //       gewinnt damit, ohne dass eine Zahl von Hand vergeben wird.
    //   REC_ChefZ_Flatbread (ChefZ_Baking)  cls 3.0 + 2 Geraete 1.0 + 0.5 = 4.5
    //       ChefZ_CheeseFlatbread liegt mit ~6.8 darueber. Verdeckt wird nichts:
    //       Flatbread verbietet Fremdkoerper, matcht also mit Kaese im Topf
    //       ohnehin nicht.
    //   REC_ChefZ_Bread                     cls 3.0 + 3 Geraete 1.5 + 0.5 = 5.0
    //       ChefZ_MeatDumplings liegt mit ~8.5 darueber, verlangt aber Hackfleisch
    //       und Zwiebel - Brot bleibt fuer den blossen Teig zustaendig.
    //   RCP_ChefZ_MushroomCreamSauce        ~9.2 (priority 20)
    //       Liegt UEBER der Pilzpfanne (~7.0), und das ist richtig: die Sauce
    //       verlangt Sahne, die Pfanne verlangt Fett oder Butter. Beide verbieten
    //       Fremdkoerper, also schliessen sie sich gegenseitig sauber aus.
    //
    // Innerhalb des Slice verdeckt kein Rezept ein anderes: je zwei Rezepte
    // unterscheiden sich in mindestens einem PFLICHT-Slot (Speck, Milchprodukt,
    // Zwiebel, Kaese, Honig, Reis, Mehl), und alle zehn setzen extraItems
    // "forbid". Wo zwei Zutatenmengen sich ueberlagern (Bauernfruehstueck und
    // Kartoffelpuffer teilen Kartoffel und Ei), matcht bewusst KEINES von beiden
    // und Vanilla kocht weiter - die Ausfallrichtung aus OF-03.
    //==========================================================================

    //--------------------------------------------------------------------------
    // §61.11 / DME §38.11 - Tactical Bacon Breakfast        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte aus ChefZ_Registry/Config/Nutrition.json als energy/water/fullness:
    //
    //    Tactical Bacon (~550/20/90) + 1 Ei (90/40/12) + 1 Brot (500/20/45)
    //    = 1140 Energie, 80 Wasser, 147 Saettigung auf ZWEI Portionen.
    //
    // energy und water gelten je 100 Einheiten Menge - eine Portion
    // (PlayerStomach.c:92 rechnet energy / 100 je Einheit). FUER
    // fullnessIndex GILT DAS NICHT: PlayerStomach.c:86 multipliziert ihn OHNE
    // Division mit der Menge. Die Saettigungszahl in der Herleitung oben ist
    // deshalb eine Zutatenbilanz und NICHT der Configwert - der steht an der
    // Klasse und ist an ChefZ_ServedDish_Base hergeleitet. Das Rezept setzt
    // quantity = 100 x Portionen; die Klasse traegt varQuantityMax fuer ihr
    // groesstes Rezept.
    //--------------------------------------------------------------------------
    class ChefZ_TacticalBreakfast : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_TACTICALBREAKFAST";
        descriptionShort = "#STR_CHEFZ_ITEM_TACTICALBREAKFAST_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\tacticalbreakfast_co.paa"};
        weight = 480;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 2.35 x 200 Einheiten = 470 - Pfannen-/leichtes Gericht.
            // Alt: 75 x 200 = 15000, das 8-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 2.35;
            energy = 570;
            water = 40;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.12 / DME §38.12 - Ruehrei mit Wurst        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte aus ChefZ_Registry/Config/Nutrition.json als energy/water/fullness:
    //
    //    3 Eier (270/120/36) + 1 Milch (200/400/30) + 1 Wurst (490/17/140)
    //    = 960 Energie, 537 Wasser, 206 Saettigung auf ZWEI Portionen.
    //    Die Milch macht das Gericht auffaellig wasserreich - das ist gewollt.
    //
    // energy und water gelten je 100 Einheiten Menge - eine Portion
    // (PlayerStomach.c:92 rechnet energy / 100 je Einheit). FUER
    // fullnessIndex GILT DAS NICHT: PlayerStomach.c:86 multipliziert ihn OHNE
    // Division mit der Menge. Die Saettigungszahl in der Herleitung oben ist
    // deshalb eine Zutatenbilanz und NICHT der Configwert - der steht an der
    // Klasse und ist an ChefZ_ServedDish_Base hergeleitet. Das Rezept setzt
    // quantity = 100 x Portionen; die Klasse traegt varQuantityMax fuer ihr
    // groesstes Rezept.
    //--------------------------------------------------------------------------
    class ChefZ_ScrambledEggSausage : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SCRAMBLEDEGGSAUSAGE";
        descriptionShort = "#STR_CHEFZ_ITEM_SCRAMBLEDEGGSAUSAGE_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\scrambledeggsausage_co.paa"};
        weight = 500;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.25 x 200 Einheiten = 650 - Hauptgericht-Teller.
            // Alt: 103 x 200 = 20600, das 10-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.25;
            energy = 480;
            water = 268;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.13 / DME §38.13 - Bauernfruehstueck        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte aus ChefZ_Registry/Config/Nutrition.json als energy/water/fullness:
    //
    //    3 Kartoffeln (540/120/120) + 2 Eier (180/80/24) + 1 Wurst (490/17/140)
    //    + 1 Zwiebel (90/55/25) = 1300 Energie auf ZWEI der drei Portionen.
    //    Mais (180/40/60) ist optional und zaehlt nicht in die Summe.
    //
    // energy und water gelten je 100 Einheiten Menge - eine Portion
    // (PlayerStomach.c:92 rechnet energy / 100 je Einheit). FUER
    // fullnessIndex GILT DAS NICHT: PlayerStomach.c:86 multipliziert ihn OHNE
    // Division mit der Menge. Die Saettigungszahl in der Herleitung oben ist
    // deshalb eine Zutatenbilanz und NICHT der Configwert - der steht an der
    // Klasse und ist an ChefZ_ServedDish_Base hergeleitet. Das Rezept setzt
    // quantity = 100 x Portionen; die Klasse traegt varQuantityMax fuer ihr
    // groesstes Rezept.
    //--------------------------------------------------------------------------
    class ChefZ_FarmersBreakfast : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FARMERSBREAKFAST";
        descriptionShort = "#STR_CHEFZ_ITEM_FARMERSBREAKFAST_DESC";
        model = "\ChefZ\ChefZ_Food\models\farmersbreakfast.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        weight = 620;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 300;
        varQuantityMax = 300;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.0 x 300 Einheiten = 900 - ueppiges Gericht.
            // Alt: 154 x 300 = 46200, das 23-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.0;
            energy = 650;
            water = 136;
            nutritionalIndex = 65;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.14 / DME §38.14 - Kaese-Fladenbrot        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte aus ChefZ_Registry/Config/Nutrition.json als energy/water/fullness:
    //
    //    1 Teig (300/90/35) + 1 Kaese (450/60/35) = 750 Energie, 150 Wasser,
    //    70 Saettigung auf ZWEI Portionen.
    //
    // energy und water gelten je 100 Einheiten Menge - eine Portion
    // (PlayerStomach.c:92 rechnet energy / 100 je Einheit). FUER
    // fullnessIndex GILT DAS NICHT: PlayerStomach.c:86 multipliziert ihn OHNE
    // Division mit der Menge. Die Saettigungszahl in der Herleitung oben ist
    // deshalb eine Zutatenbilanz und NICHT der Configwert - der steht an der
    // Klasse und ist an ChefZ_ServedDish_Base hergeleitet. Das Rezept setzt
    // quantity = 100 x Portionen; die Klasse traegt varQuantityMax fuer ihr
    // groesstes Rezept.
    //--------------------------------------------------------------------------
    class ChefZ_CheeseFlatbread : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHEESEFLATBREAD";
        descriptionShort = "#STR_CHEFZ_ITEM_CHEESEFLATBREAD_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\cheeseflatbread_co.paa"};
        weight = 380;
        lifetime = 21600;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 1.6 x 200 Einheiten = 320 - Snack.
            // Alt: 35 x 200 = 7000, das 4-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 1.6;
            energy = 375;
            water = 75;
            nutritionalIndex = 50;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.15 / DME §38.15 - Wurstbrot-Teller        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte aus ChefZ_Registry/Config/Nutrition.json als energy/water/fullness:
    //
    //    1 Brot (500/20/45) + 1 Wurst (490/17/140) + 1 Kaese (450/60/35) = 1440,
    //    mal nutritionModifier 0.95 (kalt angerichtet, kein Garverlust und kein
    //    Garzugewinn), auf ZWEI Portionen.
    //
    // energy und water gelten je 100 Einheiten Menge - eine Portion
    // (PlayerStomach.c:92 rechnet energy / 100 je Einheit). FUER
    // fullnessIndex GILT DAS NICHT: PlayerStomach.c:86 multipliziert ihn OHNE
    // Division mit der Menge. Die Saettigungszahl in der Herleitung oben ist
    // deshalb eine Zutatenbilanz und NICHT der Configwert - der steht an der
    // Klasse und ist an ChefZ_ServedDish_Base hergeleitet. Das Rezept setzt
    // quantity = 100 x Portionen; die Klasse traegt varQuantityMax fuer ihr
    // groesstes Rezept.
    //--------------------------------------------------------------------------
    class ChefZ_SausageBreadPlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SAUSAGEBREADPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_SAUSAGEBREADPLATE_DESC";
        model = "\ChefZ\ChefZ_Food\models\sausage_breadplate.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        weight = 520;
        lifetime = 18000;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.3 x 200 Einheiten = 660 - Hauptgericht-Teller.
            // Alt: 110 x 200 = 22000, das 11-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.3;
            energy = 690;
            water = 50;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.16 / DME §38.16 - Pilzpfanne        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte aus ChefZ_Registry/Config/Nutrition.json als energy/water/fullness:
    //
    //    4 Pilze (~360/140/100) + 1 Butter (600/20/15) = 960 Energie auf ZWEI
    //    Portionen. Hoher nutritionalIndex, weil DME-Plan §42 Gemuesegerichten
    //    ausdruecklich gute Vitaminwerte zuschreibt.
    //
    // energy und water gelten je 100 Einheiten Menge - eine Portion
    // (PlayerStomach.c:92 rechnet energy / 100 je Einheit). FUER
    // fullnessIndex GILT DAS NICHT: PlayerStomach.c:86 multipliziert ihn OHNE
    // Division mit der Menge. Die Saettigungszahl in der Herleitung oben ist
    // deshalb eine Zutatenbilanz und NICHT der Configwert - der steht an der
    // Klasse und ist an ChefZ_ServedDish_Base hergeleitet. Das Rezept setzt
    // quantity = 100 x Portionen; die Klasse traegt varQuantityMax fuer ihr
    // groesstes Rezept.
    //--------------------------------------------------------------------------
    class ChefZ_MushroomPan : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MUSHROOMPAN";
        descriptionShort = "#STR_CHEFZ_ITEM_MUSHROOMPAN_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\mushroompan_co.paa"};
        weight = 420;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 2.0 x 200 Einheiten = 400 - Pfannen-/leichtes Gericht.
            // Alt: 58 x 200 = 11600, das 6-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 2.0;
            energy = 480;
            water = 80;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.17 / DME §38.17 - Kartoffelpuffer        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte aus ChefZ_Registry/Config/Nutrition.json als energy/water/fullness:
    //
    //    3 Kartoffeln (540/120/120) + 100 g Mehl (~200/1/15) + 1 Ei (90/40/12)
    //    + 1 Fett (300/5/40) = 1130 Energie auf ZWEI Portionen.
    //
    // energy und water gelten je 100 Einheiten Menge - eine Portion
    // (PlayerStomach.c:92 rechnet energy / 100 je Einheit). FUER
    // fullnessIndex GILT DAS NICHT: PlayerStomach.c:86 multipliziert ihn OHNE
    // Division mit der Menge. Die Saettigungszahl in der Herleitung oben ist
    // deshalb eine Zutatenbilanz und NICHT der Configwert - der steht an der
    // Klasse und ist an ChefZ_ServedDish_Base hergeleitet. Das Rezept setzt
    // quantity = 100 x Portionen; die Klasse traegt varQuantityMax fuer ihr
    // groesstes Rezept.
    //--------------------------------------------------------------------------
    class ChefZ_PotatoPancakes : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_POTATOPANCAKES";
        descriptionShort = "#STR_CHEFZ_ITEM_POTATOPANCAKES_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\potatopancakes_co.paa"};
        weight = 500;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.1 x 200 Einheiten = 620 - Hauptgericht-Teller.
            // Alt: 93 x 200 = 18600, das 9-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.1;
            energy = 565;
            water = 83;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.18 / DME §38.18 - Fleisch-Teigtaschen        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte aus ChefZ_Registry/Config/Nutrition.json als energy/water/fullness:
    //
    //    1 Teig (300/90/35) + 2 Hackfleisch (300/80/220) + 1 Zwiebel (90/55/25)
    //    = 690 Energie, 225 Wasser, 280 Saettigung auf ZWEI der drei Portionen.
    //    Wenig Energie, viel Saettigung - genau das Profil eines Fleischgerichts
    //    nach DME-Plan §42.
    //
    // energy und water gelten je 100 Einheiten Menge - eine Portion
    // (PlayerStomach.c:92 rechnet energy / 100 je Einheit). FUER
    // fullnessIndex GILT DAS NICHT: PlayerStomach.c:86 multipliziert ihn OHNE
    // Division mit der Menge. Die Saettigungszahl in der Herleitung oben ist
    // deshalb eine Zutatenbilanz und NICHT der Configwert - der steht an der
    // Klasse und ist an ChefZ_ServedDish_Base hergeleitet. Das Rezept setzt
    // quantity = 100 x Portionen; die Klasse traegt varQuantityMax fuer ihr
    // groesstes Rezept.
    //--------------------------------------------------------------------------
    class ChefZ_MeatDumplings : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MEATDUMPLINGS";
        descriptionShort = "#STR_CHEFZ_ITEM_MEATDUMPLINGS_DESC";
        model = "\ChefZ\ChefZ_Food\models\meatdumplings.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        weight = 460;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 300;
        varQuantityMax = 300;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 2.7 x 300 Einheiten = 810 - Hauptgericht-Teller.
            // Alt: 140 x 300 = 42000, das 21-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 2.7;
            energy = 360;
            water = 115;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.19 / DME §38.19 - Milchreis        Behaelter: BOWL (Schuessel)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte aus ChefZ_Registry/Config/Nutrition.json als energy/water/fullness:
    //
    //    1 Reis (~350/10/30) + 1 Milch (200/400/30) = 550 Energie, 410 Wasser auf
    //    ZWEI Portionen. Die Saettigung liegt ueber der Rohsumme, weil der Reis
    //    in der Milch quillt.
    //
    // energy und water gelten je 100 Einheiten Menge - eine Portion
    // (PlayerStomach.c:92 rechnet energy / 100 je Einheit). FUER
    // fullnessIndex GILT DAS NICHT: PlayerStomach.c:86 multipliziert ihn OHNE
    // Division mit der Menge. Die Saettigungszahl in der Herleitung oben ist
    // deshalb eine Zutatenbilanz und NICHT der Configwert - der steht an der
    // Klasse und ist an ChefZ_ServedDish_Base hergeleitet. Das Rezept setzt
    // quantity = 100 x Portionen; die Klasse traegt varQuantityMax fuer ihr
    // groesstes Rezept.
    //--------------------------------------------------------------------------
    class ChefZ_MilkRice : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MILKRICE";
        descriptionShort = "#STR_CHEFZ_ITEM_MILKRICE_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\milkrice_co.paa"};
        weight = 540;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 1.9 x 200 Einheiten = 380 - Pfannen-/leichtes Gericht.
            // Alt: 55 x 200 = 11000, das 6-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 1.9;
            energy = 280;
            water = 205;
            nutritionalIndex = 50;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.20 / DME §38.20 - Honigbrot-Platte        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte aus ChefZ_Registry/Config/Nutrition.json als energy/water/fullness:
    //
    //    1 Brot (500/20/45) + 1 Honig (~300/10/12) = 800 Energie, 30 Wasser auf
    //    ZWEI Portionen. Kaum Wasser, dafuer schnelle Energie.
    //
    // energy und water gelten je 100 Einheiten Menge - eine Portion
    // (PlayerStomach.c:92 rechnet energy / 100 je Einheit). FUER
    // fullnessIndex GILT DAS NICHT: PlayerStomach.c:86 multipliziert ihn OHNE
    // Division mit der Menge. Die Saettigungszahl in der Herleitung oben ist
    // deshalb eine Zutatenbilanz und NICHT der Configwert - der steht an der
    // Klasse und ist an ChefZ_ServedDish_Base hergeleitet. Das Rezept setzt
    // quantity = 100 x Portionen; die Klasse traegt varQuantityMax fuer ihr
    // groesstes Rezept.
    //--------------------------------------------------------------------------
    class ChefZ_HoneyBreadPlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HONEYBREADPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_HONEYBREADPLATE_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\honeybreadplate_co.paa"};
        weight = 360;
        lifetime = 21600;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 1.4 x 200 Einheiten = 280 - Snack.
            // Alt: 30 x 200 = 6000, das 3-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 1.4;
            energy = 400;
            water = 20;
            nutritionalIndex = 45;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //==========================================================================
    // ### SLICE dishes-c ###   DIE FUENF BOWL-GERICHTE (Production Map §62)
    //
    // Je Gericht EINE Klasse (seit 29.08.2026):
    //
    //     ChefZ_<Name>Bowl : ChefZ_ServedDish_Base    entsteht im Topf, essbar;
    //                                                 Menge = 100 x Portionen
    //
    // WOHER DIE NAEHRWERTE KOMMEN (13, Architekturplan §10):
    // Sie sind aus den Zutaten ABGELEITET, nicht gesetzt. Ueber jeder Klasse
    // steht die Rechnung. Grundlage sind die Naehrwerte, die die
    // Meilenstein-2-Slices in ihren Deltas festgelegt haben und die inzwischen
    // in ChefZ_Registry/Config/Nutrition.json stehen:
    //
    //     Potato (Vanilla)   ~180 / ~45 /~40    ChefZ_Carrot     100 /  60 / 30
    //     ChefZ_Onion          90 /  55 / 25    ChefZ_Cabbage    110 /  80 / 45
    //     Tomato (Vanilla)    ~45 / ~70 /~20    ChefZ_BoneBroth  150 / 320 / 60
    //     Kraeuter             15 /  12 /  5    Paprikapulver     20 /   0 /  3
    //                                            (energy / water / stomach)
    //
    // Fuer Vanilla-Zutaten ohne ChefZ-Datensatz (Wildfleisch, Fischfilet,
    // Pilz, Bohnendose) stehen die Vanilla-Groessenordnungen ausgeschrieben in
    // der jeweiligen Rechnung, damit der chefz-balance-reviewer nicht raten
    // muss, welche Zahl woher kam.
    //
    // Die Summe wird durch die BASIS-Portionszahl geteilt (vier, siehe
    // BowlDishes.json) und mit dem nutritionModifier des Rezepts multipliziert.
    // Ein Gruppenkessel ergibt mehr Portionen aus mehr Zutaten - der Wert JE
    // Portion bleibt derselbe, und genau deshalb ist er hier eine Konstante.
    //
    // WARUM DAS BULK DENSELBEN NAEHRWERT TRAEGT WIE DIE SCHUESSEL:
    // ChefZ_PortionedDish_Base hat varQuantityMax = 100 - eine Portion. Wer
    // direkt aus dem Topf isst, isst genau eine Portion. Ein eigener Wert
    // waere ein zweiter Balancinghebel fuer denselben Bissen.
    //
    // WARUM JEDE KLASSE IHREN EIGENEN NUTRITION-BLOCK HAT (01 V7):
    // PlayerStomach.InitData registriert nur Klassen mit "Nutrition" ODER
    // "Food" und scope != 0. Fehlt beides, verschwindet der Bissen still -
    // keine Saettigung, keine Meldung. Deshalb erbt hier nichts.
    //
    // WARUM ES KEINE _Premium-VARIANTEN GIBT (OF-05, Entscheidung B):
    // Qualitaet kann den Naehrwert je Bissen nicht anheben (01 V6). Sie wirkt
    // ueber die AUSBEUTE - yieldMultiplier und portionBonus des Stufensatzes
    // machen aus demselben Kessel mehr Portionen. Vier Stufen mal fuenf
    // Gerichte waeren sonst zwanzig weitere Klassen gewesen.
    //
    // 3D-ASSETS: alle fuenf Klassen tragen ein VANILLA-PROXY.
    //     Bowl -> \dz\gear\cooking\FryingPan.p3d
    // Vanilla hat keine Schuessel; die Pfanne ist der naechste Traeger mit
    // Inhalt und ist bereits die Wahl von ChefZ_ServedDish_Base. Beides ist
    // als Assetbedarf gemeldet; sobald es eigene Geometrie gibt, faellt auch
    // visual_properties[] von {0,0,0} auf echte Selection-Indizes.
    //==========================================================================

    //--------------------------------------------------------------------------
    // §62 Hunter Stew - Wildfleisch, Wurzelgemuese, Pilze, Thymian.
    // DME §42: Eintopf -> gute Hydration, hohe Energie.
    //
    //   2x Wildfleisch    2 x 260 /  45 / 120 = 520 /  90 / 240   (Vanilla)
    //   2x Wurzelgemuese  2 x 180 /  45 /  40 = 360 /  90 /  80
    //   2x Pilz           2 x  45 /  25 /  15 =  90 /  50 /  30   (Vanilla)
    //   1x Kraut              15 /  12 /   5  =  15 /  12 /   5
    //   Wasser im Topf                        =   0 / 400 /   0
    //   ------------------------------------------------------------------
    //   Summe                                   985 / 642 / 355
    //   x 1.10 nutritionModifier / 4 Portionen -> 270 / 160 /  90
    //--------------------------------------------------------------------------
    class ChefZ_HunterStewBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERSTEW";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERSTEW_DESC";
        model = "\ChefZ\ChefZ_Food\models\stew_hunter.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 2};
        weight = 480;
        lifetime = 7200;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 400;
        varQuantityMax = 1200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 0.67 x 1200 Einheiten = 804 - Bowl.
            // Alt: 90 x 1200 = 108000, das 54-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 0.67;
            energy = 270;
            water = 160;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §62 Fisherman's Stew - Fisch, Kartoffel, Karotte, Petersilie.
    // DME §42: Fischgericht -> ausgewogene Werte, hoher Naehrwertindex.
    //
    //   2x Fischfilet     2 x 160 /  40 /  95 = 320 /  80 / 190   (Vanilla)
    //   2x Kartoffel      2 x 180 /  45 /  40 = 360 /  90 /  80
    //   1x Karotte            100 /  60 /  30 = 100 /  60 /  30
    //   1x Petersilie          15 /  12 /   5 =  15 /  12 /   5
    //   Wasser im Topf                        =   0 / 400 /   0
    //   ------------------------------------------------------------------
    //   Summe                                   795 / 642 / 305
    //   x 1.10 / 4 Portionen                 -> 220 / 160 /  75
    //--------------------------------------------------------------------------
    class ChefZ_FishermanStewBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FISHERMANSTEW";
        descriptionShort = "#STR_CHEFZ_ITEM_FISHERMANSTEW_DESC";
        model = "\ChefZ\ChefZ_Food\models\stew_fisherman.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 2};
        weight = 470;
        lifetime = 7200;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 400;
        varQuantityMax = 1200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 0.62 x 1200 Einheiten = 744 - Bowl.
            // Alt: 75 x 1200 = 90000, das 45-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 0.62;
            energy = 220;
            water = 160;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §62 Vegetable Soup - Kartoffel, Karotte, Zwiebel, Kohl, Wasser.
    // DME §37 fuehrt sie als "Basic": frueh verfuegbar, Survival-Fokus.
    // DME §42: Suppe -> hoher Wasseranteil, mittlere Energie; Gemuesegericht
    // -> gute Gesundheits- und Vitaminwerte, also hoher nutritionalIndex.
    //
    //   2x Kartoffel      2 x 180 /  45 /  40 = 360 /  90 /  80
    //   1x Karotte            100 /  60 /  30 = 100 /  60 /  30
    //   1x Zwiebel             90 /  55 /  25 =  90 /  55 /  25
    //   1x Kohl               110 /  80 /  45 = 110 /  80 /  45
    //   Wasser im Topf                        =   0 / 500 /   0
    //   optional: 1x Mais     180 /  40 /  60 - nicht in der Summe, wie Kraut und Salz
    //   ------------------------------------------------------------------
    //   Summe                                   660 / 785 / 180
    //   x 1.00 / 4 Portionen                 -> 165 / 195 /  45
    //--------------------------------------------------------------------------
    class ChefZ_VegetableSoupBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_VEGETABLESOUP";
        descriptionShort = "#STR_CHEFZ_ITEM_VEGETABLESOUP_DESC";
        model = "\ChefZ\ChefZ_Food\models\soup_vegetables.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 2};
        weight = 450;
        lifetime = 7200;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 400;
        varQuantityMax = 1200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 0.51 x 1200 Einheiten = 612 - Bowl.
            // Alt: 45 x 1200 = 54000, das 27-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 0.51;
            energy = 165;
            water = 195;
            nutritionalIndex = 65;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §62 Bone Broth Soup - Knochenbruehe, Gemuese, Kraeuter.
    // §55 nennt die Bruehe die "Premium-Basis"; das ist eine Aussage ueber den
    // Naehrwertindex, nicht ueber die Kalorien. Deshalb traegt dieses Gericht
    // den hoechsten nutritionalIndex der fuenf bei der zweitniedrigsten
    // Energie - es ersaettigt nicht, es traegt.
    //
    //   2x Knochenbruehe  2 x 150 / 320 /  60 = 300 / 640 / 120
    //   1x Kartoffel          180 /  45 /  40 = 180 /  45 /  40
    //   1x Karotte            100 /  60 /  30 = 100 /  60 /  30
    //   1x Zwiebel             90 /  55 /  25 =  90 /  55 /  25
    //   1x Kraut               15 /  12 /   5 =  15 /  12 /   5
    //   ------------------------------------------------------------------
    //   Summe                                   685 / 812 / 220
    //   x 1.05 / 4 Portionen                 -> 180 / 200 /  55
    //--------------------------------------------------------------------------
    class ChefZ_BoneBrothSoupBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BONEBROTHSOUP";
        descriptionShort = "#STR_CHEFZ_ITEM_BONEBROTHSOUP_DESC";
        model = "\ChefZ\ChefZ_Food\models\soup_bonebroth.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 2};
        weight = 500;
        lifetime = 7200;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 400;
        varQuantityMax = 1200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 0.56 x 1200 Einheiten = 672 - Bowl.
            // Alt: 55 x 1200 = 66000, das 33-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 0.56;
            energy = 180;
            water = 200;
            nutritionalIndex = 70;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §62 Chernarus Chili - Fleisch, Bohnen, Tomate, Paprika, Paprikapulver.
    // DME §42: Fleischgericht -> hohe Saettigung. Das energiereichste der fuenf
    // und zugleich das trockenste. Wer eine Schuessel Chili isst, braucht
    // danach Wasser - das ist der gewollte Gegenpol zur Vegetable Soup und der
    // Grund, warum dieses eine Gericht KEIN CHEFZ_HYDRATED traegt.
    //
    //   2x Fleisch        2 x 260 /  45 / 120 = 520 /  90 / 240   (Vanilla)
    //   1x Bohnendose         500 /  60 / 120 = 500 /  60 / 120   (Vanilla)
    //   2x Tomate         2 x  45 /  70 /  20 =  90 / 140 /  40
    //   1x Paprika             60 /  45 /  20 =  60 /  45 /  20
    //   1x Paprikapulver       20 /   0 /   3 =  20 /   0 /   3
    //   optional: 1x Mais     180 /  40 /  60 - nicht in der Summe (optionaler Slot)
    //   ------------------------------------------------------------------
    //   Summe                                  1190 / 335 / 423
    //   x 1.10 / 4 Portionen                 ->  325 /  85 / 105
    //
    // Die Zeile "1x Paprika" trug die Werte des abgeloesten ChefZ_Paprika. Der
    // Slot nimmt jetzt GreenBellPepper
    // (Vanilla-Audit §2); GreenBellPeppers Naehrwerte setzt Vanilla, sie stehen
    // nicht in diesem Projekt. Die Rechnung bleibt als ENTWURFSZIEL stehen und
    // wird nicht zur Laufzeit nachgerechnet - der Nutrition-Block unten ist der
    // massgebliche Wert. Wer die Summe neu ziehen will, braucht zuerst
    // GreenBellPeppers Vanilla-Werte; geraten wird hier nichts.
    //--------------------------------------------------------------------------
    class ChefZ_ChernarusChiliBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHERNARUSCHILI";
        descriptionShort = "#STR_CHEFZ_ITEM_CHERNARUSCHILI_DESC";
        model = "\ChefZ\ChefZ_Food\models\soup_chernaruschili.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 2};
        weight = 520;
        lifetime = 7200;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 400;
        varQuantityMax = 1200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 0.70 x 1200 Einheiten = 840 - Bowl.
            // Alt: 105 x 1200 = 126000, das 63-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 0.70;
            energy = 325;
            water = 85;
            nutritionalIndex = 50;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //==========================================================================
    // ### SLICE dishes-vanilla ###   DREI GERICHTE AUS UNGENUTZTEN VANILLA-ASSETS
    //
    // Quelle: Vanilla-Audit §3. Rund 106 Vanilla-Klassen waren ungebunden; die
    // drei Gerichte hier sind die, fuer die es keine bestehende Schuessel und
    // keinen bestehenden Teller gibt:
    //
    //   Kuerbissuppe      SlicedPumpkin - Vanillas geschnittener Kuerbis hatte
    //                     bisher gar keinen Weg in ein ChefZ-Gericht.
    //   Kleinfischpfanne  Sardines und Bitterlings - der HAEUFIGSTE Angelfang
    //                     in Vanilla, ohne Filet-Pendant und deshalb bisher
    //                     wertlos (Audit §3 D).
    //   Obstkompott       Apple, Pear, Plum, die zwei Waldbeeren und der Honig -
    //                     das erste suesse Gericht des Mods ueberhaupt.
    //
    // BAUFORM: dieselbe wie in dishes-a, dishes-b und dishes-c - EINE Klasse
    // je Gericht, das Rezept liefert sie direkt (seit 29.08.2026). Suppe und
    // Kompott geben in eine Schuessel und heissen deshalb ...Bowl, die
    // Fischpfanne auf einen Teller und heisst nur nach dem Gericht.
    //
    // NAEHRWERT: energy und water unter jeder Klasse sind die Summe EINER
    // Portion aus den Zutatenwerten der Registry, mal nutritionModifier des
    // Rezepts. Sie gelten je 100 Einheiten Menge (PlayerStomach.c:92); das
    // Rezept setzt 100 x Portionen. fullnessIndex folgt einer anderen
    // Rechnung - er wirkt ungeteilt mal der Menge (PlayerStomach.c:86) und
    // ist an ChefZ_ServedDish_Base hergeleitet.
    //
    // MODELLE: kein Gericht hat ein eigenes Mesh. Alle tragen FryingPan.p3d
    // (flach, liest sich als Teller). Der Bedarf steht
    // im Slice-Bericht; kein Item wartet auf ein Modell.
    //==========================================================================

    //--------------------------------------------------------------------------
    // Kuerbissuppe        Behaelter: BOWL      Geraet: Pot / Cauldron
    //
    // NAEHRWERTHERLEITUNG: 3 SlicedPumpkin (~150/210/95 als fullness/energy/
    // water) + 1 ChefZ_Butter (15/600/20) auf DREI Portionen, mal 1.05.
    // Wenig Energie, viel Wasser - eine Suppe saettigt und traenkt, sie mistet
    // keinen Tagesbedarf ab.
    //--------------------------------------------------------------------------
    class ChefZ_PumpkinSoupBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_PUMPKINSOUP";
        descriptionShort = "#STR_CHEFZ_ITEM_PUMPKINSOUP_DESC";
        model = "\ChefZ\ChefZ_Food\models\soup.p3d";   // EIGENES MODELL (30.08.2026): die namenlose Suppenschuessel der Lieferung
        itemSize[] = {2, 2};
        weight = 480;
        lifetime = 7200;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 300;
        varQuantityMax = 300;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 1.65 x 300 Einheiten = 495 - Pfannen-/leichtes Gericht.
            // Alt: 58 x 300 = 17400, das 9-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 1.65;
            energy = 285;
            water = 115;
            nutritionalIndex = 45;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // Kleinfischpfanne    Behaelter: PLATE     Geraet: FryingPan
    //
    // NAEHRWERTHERLEITUNG: 4 Kleinfische (~4x110 Energie) + 1 Fett
    // (Lard 300 / ChefZ_Butter 600) + 1 Knoblauch (40) auf ZWEI Portionen,
    // mal 1.1. Hoher Proteinanteil, wenig Wasser - eine Pfanne, kein Eintopf.
    //--------------------------------------------------------------------------
    class ChefZ_SmallFishPan : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SMALLFISHPAN";
        descriptionShort = "#STR_CHEFZ_ITEM_SMALLFISHPAN_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\smallfishpan_co.paa"};
        itemSize[] = {2, 2};
        weight = 400;
        lifetime = 7200;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 2.25 x 200 Einheiten = 450 - Pfannen-/leichtes Gericht.
            // Alt: 72 x 200 = 14400, das 7-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 2.25;
            energy = 430;
            water = 35;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // Obstkompott         Behaelter: BOWL      Geraet: Pot / Cauldron
    //
    // NAEHRWERTHERLEITUNG: 4 Fruechte (~4x70 Energie) + 1 getrocknete Beeren
    // (130) auf DREI Portionen, mal 1.05. Der Zucker steckt im optionalen
    // Honigslot und nicht in der Grundrechnung - ohne ihn ist es SIMPLE.
    //--------------------------------------------------------------------------
    class ChefZ_FruitCompoteBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FRUITCOMPOTE";
        descriptionShort = "#STR_CHEFZ_ITEM_FRUITCOMPOTE_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        itemSize[] = {2, 2};
        weight = 420;
        lifetime = 7200;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 300;
        varQuantityMax = 300;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 1.35 x 300 Einheiten = 405 - Pfannen-/leichtes Gericht.
            // Alt: 40 x 300 = 12000, das 6-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 1.35;
            energy = 145;
            water = 95;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //==========================================================================
    // ### SLICE dishes-a ###   TELLERGERICHTE 1-10
    //
    // Production Map §61.1-§61.10, DME-Plan §38, §41 (Rezeptqualitaet),
    // §42 (Gerichtsnutzen), §43 (Food-Buffs), §53 (Namenskonvention).
    //
    // Aufbau, Begruendung und Namensregel stehen vollstaendig im Banner des
    // Slice dishes-b weiter oben - beide Slices bauen dieselbe eine Klasse je
    // Gericht (Config/Recipes/README_Serving.md §1), und eine zweite Abschrift derselben Begruendung waere eine zweite Stelle,
    // an der sie veralten kann. Hier steht nur, was fuer DIESE zehn Gerichte
    // eigens gilt:
    //
    //   1. NAEHRWERT JE GERICHT: energy und water unter jeder Klasse sind die
    //      Summe EINER Portion aus den Zutatenwerten der Registry, mal
    //      nutritionModifier des Rezepts. Sie gelten je 100 Einheiten Menge
    //      (PlayerStomach.c:92); das Rezept setzt 100 x Portionen. Dieselbe
    //      Regel wie in dishes-b. fullnessIndex faellt NICHT darunter: er
    //      wirkt ungeteilt mal der Menge (PlayerStomach.c:86), hergeleitet an
    //      ChefZ_ServedDish_Base.
    //
    //   2. amountPerPortion = 2.0 in jedem Rezept: eine Portion kostet rund
    //      zwei Zutateneinheiten, und genau so ist die Naehrwertrechnung
    //      aufgestellt (Minimalfuellung -> eine Portion, doppelte Fuellung ->
    //      zwei). Optionale Slots zaehlen dabei nicht mit (15 §5.2) - Gewuerze
    //      koennen die Ausbeute also nicht hochkaufen.
    //
    //   3. MODELLE: kein Gericht hat ein eigenes Mesh. Alle tragen
    //      FryingPan.p3d (flach, liest sich als Teller). Der Bedarf steht im
    //      Slice-Bericht; kein Item wartet auf ein Modell.
    //==========================================================================

    //--------------------------------------------------------------------------
    // §61.1 / DME §38.1 - Survivor Spaghetti        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte als energy/water/fullness aus
    // ChefZ_Registry/Config/Nutrition.json:
    //
    //    1 Pasta (380/5/28) + 1 Tomatensauce (130/105/45) = 510 Energie, 110 Wasser,
    //    73 Saettigung. x nutritionModifier 1.10 = 561/121/80. Das Wasser ist auf 120
    //    gerundet: gekochte Pasta zieht Kochwasser, das die Trockenwerte nicht kennen.
    //--------------------------------------------------------------------------
    class ChefZ_SurvivorSpaghetti : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SURVIVORSPAGHETTI";
        descriptionShort = "#STR_CHEFZ_ITEM_SURVIVORSPAGHETTI_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\survivorspaghetti_co.paa"};
        weight = 470;
        lifetime = 14400;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.0 x 200 Einheiten = 600 - Hauptgericht-Teller.
            // Alt: 80 x 200 = 16000, das 8-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.0;
            energy = 560;
            water = 120;
            nutritionalIndex = 50;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.2 / DME §38.2 - Wurst-Nudeln-Pfanne        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte als energy/water/fullness aus
    // ChefZ_Registry/Config/Nutrition.json:
    //
    //    1 Pasta (380/5/28) + 1 Wurst (470/18/140) + 1 Fett (300/5/40) = 1150 Energie,
    //    28 Wasser, 208 Saettigung. x 1.10 = 1265/31/229. Wasser auf 70 angehoben
    //    (Kochwasser der Pasta). Sehr hohe Energie ist DME §42 fuer Pastagerichte.
    //--------------------------------------------------------------------------
    class ChefZ_SausagePasta : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SAUSAGEPASTA";
        descriptionShort = "#STR_CHEFZ_ITEM_SAUSAGEPASTA_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\sausagepasta_co.paa"};
        weight = 490;
        lifetime = 14400;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 4.25 x 200 Einheiten = 850 - Hauptgericht-Teller.
            // Alt: 230 x 200 = 46000, das 23-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 4.25;
            energy = 1270;
            water = 70;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.3 / DME §38.3 - Jaegernudeln        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte als energy/water/fullness aus
    // ChefZ_Registry/Config/Nutrition.json:
    //
    //    1 Pasta (380/5/28) + 1 Wildfleisch (250/45/110) + 1 Pilz (60/30/20)
    //    + 1 Kraut (15/12/5) = 705/92/163. x 1.15 = 811/106/187. Wasser auf 145
    //    angehoben (Kochwasser). Sahne ist optionaler Slot und zaehlt hier nicht mit.
    //--------------------------------------------------------------------------
    class ChefZ_HunterPasta : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERPASTA";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERPASTA_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\hunterpasta_co.paa"};
        weight = 490;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.8 x 200 Einheiten = 760 - Hauptgericht-Teller.
            // Alt: 185 x 200 = 37000, das 18-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.8;
            energy = 810;
            water = 145;
            nutritionalIndex = 70;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.4 / DME §38.4 - Rahm-Pilz-Nudeln        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte als energy/water/fullness aus
    // ChefZ_Registry/Config/Nutrition.json:
    //
    //    1 Pasta (380/5/28) + 2 Pilze (120/60/40) + 1 Sahne (350/150/20)
    //    + 1 Kraut (15/12/5) = 865/227/93. x 1.10 = 952/250/102.
    //--------------------------------------------------------------------------
    class ChefZ_CreamMushroomPasta : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CREAMMUSHROOMPASTA";
        descriptionShort = "#STR_CHEFZ_ITEM_CREAMMUSHROOMPASTA_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\creammushroompasta_co.paa"};
        weight = 490;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.15 x 200 Einheiten = 630 - Hauptgericht-Teller.
            // Alt: 100 x 200 = 20000, das 10-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.15;
            energy = 950;
            water = 250;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.5 / DME §38.5 - Chernarus Mac & Cheese        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte als energy/water/fullness aus
    // ChefZ_Registry/Config/Nutrition.json:
    //
    //    1 Pasta (380/5/28) + 1 Milch (200/400/30) + 1 Kaese (450/60/35) = 1030/465/93.
    //    x 1.10 = 1133/512/102. Die Butter ist OPTIONALER Slot (sie hebt die Stufe)
    //    und zaehlt deshalb nicht in die Grundrechnung - sonst laege das Gericht bei
    //    ueber 1800 Energie und waere das staerkste Nahrungsmittel des Mods.
    //--------------------------------------------------------------------------
    class ChefZ_MacAndCheese : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MACANDCHEESE";
        descriptionShort = "#STR_CHEFZ_ITEM_MACANDCHEESE_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\macandcheese_co.paa"};
        weight = 500;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.15 x 200 Einheiten = 630 - Hauptgericht-Teller.
            // Alt: 100 x 200 = 20000, das 10-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.15;
            energy = 1130;
            water = 510;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.6 / DME §38.6 - Kartoffeln mit Bratwurst        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte als energy/water/fullness aus
    // ChefZ_Registry/Config/Nutrition.json:
    //
    //    1 Kartoffel (180/40/40) + 1 Wurst (470/18/140) + 1 Fett (300/5/40) = 950/63/220.
    //    x 1.05 = 998/66/231. Hohe Saettigung ist DME §42 fuer Fleischgerichte.
    //--------------------------------------------------------------------------
    class ChefZ_SausagePotatoes : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SAUSAGEPOTATOES";
        descriptionShort = "#STR_CHEFZ_ITEM_SAUSAGEPOTATOES_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\sausagepotatoes_co.paa"};
        weight = 490;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 4.25 x 200 Einheiten = 850 - Hauptgericht-Teller.
            // Alt: 230 x 200 = 46000, das 23-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 4.25;
            energy = 1000;
            water = 65;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.7 / DME §38.7 - Jaegerteller        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte als energy/water/fullness aus
    // ChefZ_Registry/Config/Nutrition.json:
    //
    //    1 Wildfleisch (250/45/110) + 1 Kartoffel (180/40/40) + 1 Pilz (60/30/20)
    //    + 1 Kraut (15/12/5) = 505/127/175. x 1.15 = 581/146/201.
    //--------------------------------------------------------------------------
    class ChefZ_HunterPlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERPLATE_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\hunterplate_co.paa"};
        weight = 490;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.95 x 200 Einheiten = 790 - Hauptgericht-Teller.
            // Alt: 200 x 200 = 40000, das 20-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.95;
            energy = 580;
            water = 145;
            nutritionalIndex = 70;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.8 / DME §38.8 - Blutwurstplatte        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte als energy/water/fullness aus
    // ChefZ_Registry/Config/Nutrition.json:
    //
    //    1 Wurst (470/18/140) + 1 Kartoffel (180/40/40) + 1 Zwiebel (90/55/25) = 740/113/205.
    //    x 1.05 = 777/119/215. Gerechnet mit der allgemeinen Wurst: ChefZ_BloodSausage
    //    gibt es in V1 nicht (Production Map §61.8, "kann optional V1.1 werden").
    //--------------------------------------------------------------------------
    class ChefZ_BloodSausagePlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BLOODSAUSAGEPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_BLOODSAUSAGEPLATE_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\bloodsausageplate_co.paa"};
        weight = 490;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 4.1 x 200 Einheiten = 820 - Hauptgericht-Teller.
            // Alt: 215 x 200 = 43000, das 22-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 4.1;
            energy = 780;
            water = 120;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.9 / DME §38.9 - Fisch mit Kartoffeln        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte als energy/water/fullness aus
    // ChefZ_Registry/Config/Nutrition.json:
    //
    //    1 Fischfilet (200/50/90) + 1 Kartoffel (180/40/40) + 1 Kraut (15/12/5) = 395/102/135.
    //    x 1.10 = 435/112/149. Ausgewogene Werte sind DME §42 fuer Fischgerichte.
    //--------------------------------------------------------------------------
    class ChefZ_FishPotatoPlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FISHPOTATOPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_FISHPOTATOPLATE_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\fishpotatoplate_co.paa"};
        weight = 470;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 3.5 x 200 Einheiten = 700 - Hauptgericht-Teller.
            // Alt: 150 x 200 = 30000, das 15-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 3.5;
            energy = 435;
            water = 110;
            nutritionalIndex = 65;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §61.10 / DME §38.10 - Bohnen-Wurst-Teller        Behaelter: PLATE (Teller)
    //
    // NAEHRWERTHERLEITUNG (Auftrag: aus den Zutaten ableiten, nicht erfinden).
    // Zutatenwerte als energy/water/fullness aus
    // ChefZ_Registry/Config/Nutrition.json:
    //
    //    1 Dose Baked Beans (600/150/120) + 1 Wurst (470/18/140) + 1 Zwiebel (90/55/25)
    //    = 1160/223/285. x 1.05 = 1218/234/299. Die Dose ist Vanilla-Loot (§4.5) und
    //    hat keinen ChefZ-Naehrwertdatensatz; die Werte sind aus der Groessenordnung
    //    einer Vanilla-Bohnendose geschaetzt und im Slice-Bericht als solche benannt.
    //--------------------------------------------------------------------------
    class ChefZ_BeanSausagePlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BEANSAUSAGEPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_BEANSAUSAGEPLATE_DESC";
        model = "\ChefZ\ChefZ_Food\models\Panfood_Base.p3d";
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Food\data\beansausageplate_co.paa"};
        weight = 500;
        lifetime = 10800;
        // Menge = 100 je Portion. energy und water rechnet PlayerStomach.c:92
        // je 100 Einheiten einmal ab; fullnessIndex NICHT - der geht in
        // PlayerStomach.c:86 unverkuerzt mal der Menge ins Magenvolumen.
        // Init = kleinstes Rezept (Spawn ohne Rezept), Max = groesstes Rezept;
        // das Rezept setzt 100 x Portionen.
        varQuantityInit = 200;
        varQuantityMax = 200;

        class Nutrition
        {
            // MAGENVOLUMEN (PlayerStomach.c:86, Herleitung an ChefZ_ServedDish_Base):
            // 4.75 x 200 Einheiten = 950 - ueppiges Gericht.
            // Alt: 300 x 200 = 60000, das 30-fache von VOMIT_THRESHOLD 2000.
            fullnessIndex = 4.75;
            energy = 1220;
            water = 235;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };
};

//==============================================================================
// ### SLICE serving ###   DIE BEHAELTERREGISTRY (16 §3.1)
//
// Sie steht in RANG 1 (Game-Config) und nicht in JSON, und das ist keine
// Geschmacksfrage: ChefZ_ActionTakePortion.ActionCondition() laeuft auch auf
// dem CLIENT und muss dort entscheiden, ob eine passende Schuessel im Zugriff
// ist. Sonst erscheint die Aktion, der Server weist sie ab, und der Spieler
// sieht einen Fortschrittsbalken, der nichts bewirkt. Der Client liest Rang 1
// garantiert; ob er JSON aus einem PBO lesen kann, ist eine offene Messfrage
// (OF-10).
//
// id == Klassenname des LEEREN Behaelters (16 §3.2).
//
// HIER - und nur hier - entstehen die fuenf Kategorien aus Architekturplan
// §18. Es gibt keine eigene Kategorieregistry fuer Behaelter: eine Kategorie
// existiert genau dann, wenn ein Behaelter sich zu ihr bekennt. Ein spaeter
// hinzugefuegter Holznapf traegt containerCategories[] = {"BOWL"} ein und
// funktioniert SOFORT mit jedem bestehenden Schuesselgericht - ohne dass ein
// Rezept oder eine Zeile Core-Code angefasst wird (16 E2).
//
// searchScope ist ein BITFELD: 1 HANDS | 2 INVENTORY | 4 NEARBY_CARGO.
// Ueberall 3 = Haende und Inventar. NEARBY_CARGO bleibt aus (16 E5): es wird
// bei Basen mit vielen Kisten teuer und macht fuer den Spieler undurchsichtig,
// aus welchem Fass sein Teller kam.
//==============================================================================
class CfgChefZContainers
{
    // PLATE - Tellergerichte (§61). Wiederverwendbar (OF-04):
    // ChefZ_SurvivorSpaghetti -> consume -> ChefZ_EmptyPlate.
    class ChefZ_EmptyPlate
    {
        containerCategories[] = {"PLATE"};
        emptyClass            = "ChefZ_EmptyPlate";
        displayName           = "#STR_CHEFZ_ITEM_EMPTYPLATE0";
        reusable              = 1;
        consumedOnServe       = 1;
        spoilageModifier      = 1.0;
        searchScope           = 3;
    };

    // BOWL - Suppen, Eintoepfe, Portionsgerichte (§62, §17).
    class ChefZ_EmptyBowl
    {
        containerCategories[] = {"BOWL"};
        emptyClass            = "ChefZ_EmptyBowl";
        displayName           = "#STR_CHEFZ_ITEM_EMPTYBOWL0";
        reusable              = 1;
        consumedOnServe       = 1;
        spoilageModifier      = 1.0;
        searchScope           = 3;
    };

    // CAN - der Konservenfall (OF-04, letzter Absatz): reusable = 0, es kommt
    // nichts zurueck. spoilageModifier 0.15, weil die Dose der Grund ist,
    // warum Konserven ueberhaupt haltbar sind (16 E7).
    class ChefZ_EmptyCan
    {
        containerCategories[] = {"CAN"};
        emptyClass            = "";
        displayName           = "#STR_CHEFZ_ITEM_EMPTYCAN0";
        reusable              = 0;
        consumedOnServe       = 1;
        spoilageModifier      = 0.15;
        searchScope           = 3;
    };

    // JAR - Eingemachtes. Der Wert 0.10 stammt woertlich aus 16 §3.1.
    class ChefZ_EmptyJar
    {
        containerCategories[] = {"JAR"};
        emptyClass            = "ChefZ_EmptyJar";
        displayName           = "#STR_CHEFZ_ITEM_EMPTYJAR0";
        reusable              = 1;
        consumedOnServe       = 1;
        spoilageModifier      = 0.10;
        searchScope           = 3;
    };

    // BOX - Trockenware. Schuetzt vor Feuchtigkeit, nicht vor der Zeit:
    // spuerbarer, aber kleiner Faktor.
    class ChefZ_EmptyBox
    {
        containerCategories[] = {"BOX"};
        emptyClass            = "ChefZ_EmptyBox";
        displayName           = "#STR_CHEFZ_ITEM_EMPTYBOX0";
        reusable              = 1;
        consumedOnServe       = 1;
        spoilageModifier      = 0.80;
        searchScope           = 3;
    };
};

//==============================================================================
// ### SLICE serving ###   PORTIONSKAPAZITAET DER KOCHGERAETE
// (15 §5.1, Planungsschritte §11)
//
// Die Forderung aus Planungsschritte §11 - "Small Pot 2-3, Cooking Pot 4,
// Cauldron 8-12" - wird hier VOLLSTAENDIG AUS DATEN erfuellt. Kein Rezept
// nennt eine Geraetegroesse; der Deckel steht beim GERAET, wo er hingehoert.
// Dasselbe Eintopfrezept ergibt damit in der Pfanne 2 und im Kessel 12
// Portionen, ohne zweimal geschrieben zu werden.
//
// Der ZWEITE Deckel - floor(verbrauchte Zutatenmenge / amountPerPortion) -
// steht am Rezept (15 §5.2) und ist die Sperre gegen den offensichtlichsten
// Nahrungsexploit des Mods: eine Minimalfuellung im Kessel darf keine zwoelf
// Portionen ergeben. BEIDE Deckel wirken. Ein Gerichtsrezept, das
// amountPerPortion weglaesst, hat nur diesen hier - das ist die eine Zahl,
// die dishes-a/b/c/sauces nicht vergessen duerfen.
//
// Vanilla-Klassennamen, bewusst: ChefZ fasst keine fremde config.cpp an, es
// nennt fremde Klassen in einem eigenen Knoten (11 E8). Ein Geraet, das hier
// fehlt, hat keine Kapazitaet - dann greift allein der Zutatendeckel.
//==============================================================================
class CfgChefZDevices
{
    // Die Pfanne ist flach: was hineinpasst, reicht fuer zwei Teller.
    class FryingPan
    {
        portionCapacity = 2;
    };

    // "Cooking Pot -> 4 Portionen", woertlich aus Planungsschritte §11.
    class Pot
    {
        portionCapacity = 4;
    };

    // "Cauldron -> 8-12 Portionen". 12 ist die Obergrenze der Spanne und der
    // Grund, ueberhaupt einen Kessel zu tragen (Architekturplan §17,
    // Gruppengericht).
    class Cauldron
    {
        portionCapacity = 12;
    };
};

//==============================================================================
// ### SLICE serving ###   WOHER DIE ERSTE SCHUESSEL KOMMT
//
// Ohne eine Quelle waere das ganze Servieren an Loot gebunden - und ein
// Spieler, dessen Eintopf fertig ist und der keine Schuessel hat, kann nichts
// tun (genau der Frustfall, den OF-04 benennt). Zwei Schnitzvorgaenge loesen
// das mit Vanilla-Mitteln: Brennholz plus Messer.
//
// exec = "HANDCRAFT": ein Eingang plus Werkzeuggruppe. Vanillas Craftsystem
// kombiniert immer ZWEI Dinge und kennt MAX_NUMBER_OF_INGREDIENTS = 2
// (01 V12) - ein Eingang OHNE Werkzeug waere nicht registrierbar.
//
// Zwei Prozesse statt eines gemeinsamen: der Aktionstext im Kontextmenue
// kommt aus displayName. Mit einem gemeinsamen Prozess hiessen beide Eintraege
// gleich und der Spieler saehe zweimal dasselbe Wort ueber verschiedenen
// Ergebnissen.
//
// Die Werkzeuggruppe CUTTING_TOOL gehoert ChefZ_Processing und wird hier nur
// REFERENZIERT - deshalb steht ChefZ_Processing in requiredAddons.
//==============================================================================
class CfgChefZProcesses
{
    class PROCESS_CARVE_PLATE
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_CARVE_PLATE";
        toolGroups[] = {"CUTTING_TOOL"};
        baseDurationSec = 20.0;
    };

    class PROCESS_CARVE_BOWL
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_CARVE_BOWL";
        toolGroups[] = {"CUTTING_TOOL"};
        baseDurationSec = 25.0;
    };

    //--------------------------------------------------------------------------
    // ### SLICE serving ###   DIE DREI CRAFTBAREN BEHAELTER (DME-Plan §32)
    //
    // Der Vanilla-Audit haelt fest, dass ChefZ_EmptyCan, ChefZ_EmptyBox und
    // ChefZ_EmptyJar NULL Rezeptreferenzen haben - drei Gegenstaende ohne
    // Herkunft. Der Audit schlug vor, sie unsichtbar zu schalten; DME-Plan §32
    // ("Verpackungsmaterialien") dreht das um und gibt zweien davon einen
    // Herstellungsweg. Das Einmachglas bleibt bewusst ohne: §32 nennt es unter
    // "zusaetzlich fuer ChefZ", der Weg dorthin ist Glasarbeit und nicht
    // Blechschnitt - er bleibt offen, statt erfunden zu werden.
    //
    // Alle drei Prozesse sind HANDCRAFT und tragen deshalb dieselbe Form wie
    // die beiden Schnitzvorgaenge darueber: hoechstens ZWEI Zutatenplaetze
    // (01 V12, RecipeBase.MAX_NUMBER_OF_INGREDIENTS = 2), und ein Werkzeug
    // belegt den zweiten. Kein Prozess hier nennt stationsAllowed - ein
    // Handwerksschritt laeuft ohne Station, und ChefZ_HandcraftBridge wiese
    // ihn sonst beim Registrieren ab.
    //
    // Eigene Prozesse und keine Wiederverwendung der beiden oberen: der
    // Aktionstext im Kontextmenue kommt aus displayName. "Schuessel schnitzen"
    // ueber einem Blech waere falsch, und PROCESS_CARVE_BOWL traegt ausserdem
    // nur CUTTING_TOOL - Aexte kaemen dort nicht hinzu, ohne dass jeder
    // Gemueseschnitt sie ebenfalls bekaeme.
    //--------------------------------------------------------------------------

    // "bark + any axe or knife -> Holzschale".
    //
    // ZWEI Werkzeuggruppen, und genau darin liegt die Entscheidung:
    // CUTTING_TOOL (acht Messer, deklariert in ChefZ_Processing) wurde NICHT
    // um Aexte erweitert. Sie ist geteiltes Vokabular - PROCESS_CUT_MEAT und
    // die Schnitzprozesse lesen dieselbe Gruppe.
    // Eine Feuerwehraxt, mit der man Kraeuter hackt, waere der Preis dafuer
    // gewesen. Stattdessen steht die Axtgruppe daneben; ChefZ_HandcraftBridge.
    // CollectToolClasses bildet die VEREINIGUNG ueber alle genannten Gruppen,
    // ein Werkzeug aus EINER von beiden genuegt.
    //
    // 30 Sekunden: laenger als der Schnitzvorgang aus Brennholz (25), weil
    // Rinde die schlechtere Ausgangsform ist. Der Weg ist trotzdem der
    // billigere - Rinde bekommt man mit demselben Messer vom Baum.
    class PROCESS_CARVE_BOWL_BARK
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_CARVE_BOWL_BARK";
        toolGroups[] = {"CUTTING_TOOL", "AXE_TOOL"};
        baseDurationSec = 30.0;
        toolDamage = 1;
    };

    // "paper + paper -> empty box" (DME-Plan §32, "Cardboard Food Box").
    //
    // KEINE Werkzeuggruppe, und das ist Pflicht, nicht Sparsamkeit: der
    // Transform hat ZWEI Eingaenge (zweimal Papier). Ein Werkzeug waere der
    // dritte Zutatenplatz, und Vanilla kennt zwei. Mit Werkzeug wuerde
    // ChefZ_GenericCraftRecipe.InitFromDef den Transform abweisen und das
    // Rezept erschiene nie.
    class PROCESS_FOLD_BOX
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_FOLD_BOX";
        baseDurationSec = 12.0;
    };

    // "metal sheet + hacksaw -> 10 empty cans".
    //
    // Eigene Werkzeuggruppe SAWING_TOOL statt METALWORK_TOOL: die Metallgruppe
    // aus ChefZ_Processing fuehrt Zange, Hammer, Schraubenschluessel und
    // Schraubendreher - Werkzeuge zum BIEGEN und Verbinden. Blech zerteilt man
    // damit nicht. Der Auftrag nennt die Saege ausdruecklich, und
    // ChefZ_Vanilla_Assets.md §20 fuehrt Hacksaw woertlich mit dem Vermerk
    // "Metallsaege - Dosenherstellung nach DME-Plan §32".
    //
    // toolDamage = 5: zehn Dosen aus einem Blech saegen kostet die Saege
    // spuerbar mehr als ein Schnitt Gemuese (dort 1).
    class PROCESS_CUT_CANS
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_CUT_CANS";
        toolGroups[] = {"SAWING_TOOL"};
        baseDurationSec = 45.0;
        toolDamage = 5;
    };
};

//==============================================================================
// ### SLICE serving ###   ZWEI NEUE WERKZEUGGRUPPEN
//
// Gruppenweise Schreibweise (ChefZ_ToolGroupDef, Kopf): id ist die GRUPPE,
// classes[] sind ihre Mitglieder. Genau dafuer ist sie da - ChefZ fasst keine
// fremde config.cpp an, sondern nennt fremde Klassen in einer eigenen Gruppe
// (11 E8).
//
// Beide Gruppen stehen GENAU EINMAL im ganzen Projekt, und zwar hier. Die
// Engine mergt CfgChefZTools ueber alle Addons; zwei Knoten gleichen Namens
// waeren keine Redundanz, sondern eine stille Ueberschreibung, deren Gewinner
// von der Ladereihenfolge abhaengt. CUTTING_TOOL, ROLLING_PIN und
// METALWORK_TOOL gehoeren ChefZ_Processing und werden hier nur BENUTZT -
// deshalb steht ChefZ_Processing in requiredAddons.
//
// Rang 1 und nicht JSON: ChefZ_ActionProcessAtStation.ActionCondition()
// entscheidet auch auf dem CLIENT, ob ein passendes Werkzeug in der Hand
// liegt. Der Client liest die Game-Config garantiert; ob er JSON aus einem PBO
// lesen kann, ist offen (OF-10).
//==============================================================================
class CfgChefZTools
{
    // "any axe" aus dem Auftrag, in Klassennamen.
    //
    // Alle vier sind Vanillaklassen und im Referenzindex belegt
    // (refindex/vanilla-scripts-classes.txt). Aufgenommen ist, womit man Holz
    // bearbeitet:
    //
    //   WoodAxe         Spaltaxt
    //   Hatchet         Beil
    //   FirefighterAxe  Feuerwehraxt
    //   Iceaxe          Eispickel mit Axtblatt
    //
    // NICHT aufgenommen: Pickaxe (Spitzhacke - ein Bergbauwerkzeug, kein
    // Axtblatt) und die drei Macheten. Macheten sind weder Axt noch Messer;
    // wer sie will, deklariert eine eigene Gruppe, statt diese hier
    // aufzuweichen.
    //
    // allowSubclasses = 1 wie bei CUTTING_TOOL: gemoddete Ableitungen einer
    // Vanillaaxt zaehlen mit.
    class AXE_TOOL
    {
        classes[] =
        {
            "WoodAxe",
            "Hatchet",
            "FirefighterAxe",
            "Iceaxe"
        };
        allowSubclasses = 1;
    };

    // Die Metallsaege. EIN Mitglied, und das ist kein Versehen: der Auftrag
    // nennt die Saege namentlich, und Vanilla fuehrt genau eine
    // (ChefZ_Vanilla_Assets.md §20, nominal 140, Industrial/Farm - also
    // erreichbares Loot und keine Rarität, an der die Kette haengen bliebe).
    //
    // Eine eigene Gruppe und kein direkter Klassenname im Prozess: ein Prozess
    // nennt nie ein Werkzeug, sondern immer eine Gruppe (11 E8). Ein
    // Serverbetreiber, der eine Saege aus einem fremden Mod aufnehmen will,
    // ergaenzt dann classes[] und muss kein Rezept anfassen.
    class SAWING_TOOL
    {
        classes[] =
        {
            "Hacksaw"
        };
        allowSubclasses = 1;
    };
};

//==============================================================================
// ### SLICE dishes-a ###   DER STUFENSATZ "DISH_DEFAULT"   (12 §3, §4)
//
// ACHTUNG, GETEILTES GUT: dieser Knoten gehoert keinem Gericht, sondern ALLEN.
// Core.json nennt "DISH_DEFAULT" als qualityScoring.defaultTierSet, und jedes
// Rezept der Slices dishes-a, dishes-b, dishes-c und sauces zeigt darauf.
// Bis hierher hat ihn NIEMAND deklariert - der Validator meldete dazu
// "Qualitaets-Stufensatz: 1 Referenz(en), aber kein einziger Datensatz
// deklariert diesen Namensraum". Er steht jetzt genau EINMAL, hier.
//
//   dishes-b, dishes-c und jeder spaetere Gerichteslice deklarieren ihn NICHT
//   noch einmal. Zwei gleichnamige Knoten in derselben config.cpp sind eine
//   doppelte Definition; zwei in verschiedenen Modulen waeren ein stiller
//   Merge, dessen Gewinner von der Ladereihenfolge abhinge.
//
// RANG 1 und nicht JSON: die Qualitaetsstufe eines Gerichts wird gesynct und
// erscheint am Item (12 §2, "Anzeige ueber die Sync-Variable"). Der Client
// liest Rang 1 garantiert; ob er JSON aus einem PBO lesen kann, ist offen
// (OF-10). Dieselbe Begruendung wie bei CfgChefZContainers und CfgChefZStates.
//
// ---------------------------------------------------------------------------
// Die Zahlen, und warum genau diese
// ---------------------------------------------------------------------------
// minScore-Schwellen und Stufennamen stehen woertlich im Content-Beispiel von
// 12 §3, das seinerseits Architekturplan §9 und DME-Plan §41 abbildet:
//
//   SIMPLE    nur die Grundzutaten                    -> 0 Punkte
//   PREPARED  vollstaendiges normales Rezept (Salz)   -> 2
//   SEASONED  zusaetzliche Gewuerze oder Kraeuter     -> 4
//   PREMIUM   seltene Zutaten, volle Variante         -> 7
//
// Die Rezepte dieses Slice sind auf genau diese Schwellen gerechnet: der
// optionale Salzslot gibt 2, Gewuerz und Kraut je 1, die Premiumzutat
// (Sahne, Sauce, Hunter Sausage, Hunter Seasoning) 2 bis 3.
//
// POOR ist mitdeklariert, obwohl kein Rezept ihn ansteuert: ohne ihn hat die
// dynamische Bewertung keinen Platz nach unten, und ein Eintopf aus altem
// Fleisch waere "auch SIMPLE" statt schlechter (12 §3, letzter Absatz).
// Die Zustandsstrafen dafuer stehen in Core.json (BURNT -3, ROTTEN -99).
//
// WAS QUALITAET BEWIRKT - und was sie nicht kann (12 §2, aus Befund 01 V6):
// Der Naehrwert je Bissen haengt an Klasse x Foodstage und laesst sich nicht
// je Instanz veraendern. Der Hebel ist deshalb die AUSBEUTE: yieldMultiplier
// und portionBonus wirken auf die Portionszahl (15 §4), spoilageMultiplier auf
// die Haltbarkeit (14). grantsEffects sind opake IDs nach Architekturplan §11 -
// der Core wertet sie nie aus, ein Effektmodul deutet sie spaeter.
//==============================================================================
class CfgChefZQualityTiers
{
    class POOR
    {
        tierSet = "DISH_DEFAULT";
        rank = 0;
        minScore = -99.0;
        displayName = "#STR_CHEFZ_TIER_POOR";
        yieldMultiplier = 0.75;
        spoilageMultiplier = 1.25;
        portionBonus = 0;
    };

    class SIMPLE
    {
        tierSet = "DISH_DEFAULT";
        rank = 1;
        minScore = 0.0;
        displayName = "#STR_CHEFZ_TIER_SIMPLE";
        yieldMultiplier = 1.0;
        spoilageMultiplier = 1.0;
        portionBonus = 0;
    };

    class PREPARED
    {
        tierSet = "DISH_DEFAULT";
        rank = 2;
        minScore = 2.0;
        displayName = "#STR_CHEFZ_TIER_PREPARED";
        yieldMultiplier = 1.1;
        spoilageMultiplier = 0.95;
        portionBonus = 0;
    };

    class SEASONED
    {
        tierSet = "DISH_DEFAULT";
        rank = 3;
        minScore = 4.0;
        displayName = "#STR_CHEFZ_TIER_SEASONED";
        yieldMultiplier = 1.25;
        spoilageMultiplier = 0.9;
        portionBonus = 1;
        grantsEffects[] = {"CHEFZ_WARM_MEAL"};
    };

    class PREMIUM
    {
        tierSet = "DISH_DEFAULT";
        rank = 4;
        minScore = 7.0;
        displayName = "#STR_CHEFZ_TIER_PREMIUM";
        yieldMultiplier = 1.5;
        spoilageMultiplier = 0.85;
        portionBonus = 2;
        grantsEffects[] = {"CHEFZ_WARM_MEAL", "CHEFZ_HEARTY_MEAL"};
        grantsTags[] = {"CHEFZ_PREMIUM"};
    };
};

//==============================================================================
// ### SLICE sauces ### Zutatenbindung, Rang 1
//
// Entwurf 05 §2: EIGENE Klassen deklarieren sich in der eigenen config.cpp,
// FREMDE im Slice-JSON. Hier steht deshalb nur ChefZ-Eigenes; die Pilze
// (Vanilla) stehen in ChefZ_Ingredients/Config/Ingredients/Mushrooms.json.
//
// Rang 1 und nicht JSON, weil der CLIENT diese Bindung garantiert liest
// (02 §2, 11 E8): eine Sauce erscheint in Aktionstexten und Rezeptvorschauen,
// und die laufen auf dem Client.
//
// Die Kategorien SAUCE, TOMATO_SAUCE, CREAM_SAUCE, BROTH und die Tags
// CHEFZ_SAUCE_BASE, CHEFZ_CREAMY stehen im Delta _deltas/sauces.json und
// werden vom chefz-registry-integrator gemergt. Dieses Modul fasst keine
// zentrale Registry an (Workflow §5).
//
// Der Basisknoten heisst bewusst NICHT wie die CfgVehicles-Basisklasse: die
// Config-Vererbung INNERHALB von CfgChefZIngredients loest die Engine selbst
// auf (Kopf von ChefZ_ConfigCppSource.ReadIngredients), ein zweiter Knoten
// gleichen Namens waere nur eine Verwechslungsquelle.
//
// defaultState "COOKED": alle vier entstehen im Topf und sind fertig. Ein
// Gerichtsrezept, das ausdruecklich gekochte Zutaten verlangt, findet sie
// damit ohne Sonderfall.
//
// quantityUnit "GRAM": eine Sauce ist eine Menge. unitsPerWholeItem = 100
// entspricht varQuantityMax der Configklasse - ein volles Glas sind 100
// Einheiten, und ein Gericht darf sich 25 davon nehmen.
//==============================================================================
class CfgChefZIngredients
{
    class ChefZ_SauceIngredient
    {
        // VORLAGE, keine Bindung (ChefZ_ConfigCppSource.IsBindingTemplate: template == eigener
        // Klassenname; Erben tragen DIESEN Namen und bleiben Bindungen). Vorfall 31.08.2026:
        // ohne das Feld meldete der Ladebericht je Serverstart 7 Vorlagen als fehlende Klasse.
        template          = "ChefZ_SauceIngredient";
        categories[]      = {"SAUCE"};
        tags[]            = {"CHEFZ_SAUCE_BASE"};
        defaultState      = "COOKED";
        quantityUnit      = "GRAM";
        unitsPerWholeItem = 100;
        decays            = 1;
    };

    class ChefZ_TomatoSauce : ChefZ_SauceIngredient
    {
        categories[] = {"SAUCE", "TOMATO_SAUCE"};
    };

    class ChefZ_CreamSauce : ChefZ_SauceIngredient
    {
        categories[] = {"SAUCE", "CREAM_SAUCE"};
        tags[]       = {"CHEFZ_SAUCE_BASE", "CHEFZ_CREAMY"};
    };

    // Bewusst OHNE die Kategorie MUSHROOM und ohne CHEFZ_MUSHROOM: die
    // Kategorie steht fuer den ROHSTOFF Pilz. Truege die Sauce sie, wuerde ein
    // Rezept mit dem Slot { "category": "MUSHROOM" } die fertige Sauce als
    // Pilznachschub akzeptieren - ein stiller Kreis, den niemand mehr findet.
    class ChefZ_MushroomCreamSauce : ChefZ_SauceIngredient
    {
        categories[] = {"SAUCE", "CREAM_SAUCE"};
        tags[]       = {"CHEFZ_SAUCE_BASE", "CHEFZ_CREAMY"};
    };

    // Die Bruehe ist KEINE Sauce - sie steht unter BROTH und traegt den
    // Sauce-Tag nicht. Ein Rezept, das "irgendeine Sauce" sucht, soll keine
    // Bruehe bekommen und umgekehrt.
    class ChefZ_BoneBroth : ChefZ_SauceIngredient
    {
        categories[] = {"BROTH"};
        tags[]       = {};
    };

    //==========================================================================
    // ### SLICE dishes-b ###   Zutatenbindung der Tellergerichte 11-20
    //
    // Rang 1 und nicht JSON, weil der CLIENT diese Bindung garantiert liest
    // (02 §2, 11 E8): ChefZ_ActionTakePortion.ActionCondition() laeuft auch auf
    // dem Client und muss dort entscheiden, ob ein passender leerer Behaelter
    // im Zugriff ist. Sonst erscheint die Aktion, der Server weist sie ab, und
    // der Spieler sieht einen Fortschrittsbalken, der nichts bewirkt.
    //
    // WOFUER DIESE RECORDS UEBERHAUPT DA SIND, obwohl das Rezept die Portion
    // und den Behaelter schon nennt: fuer den Weg AM REZEPT VORBEI. Ein Gericht
    // aus einem Admin-Spawn oder aus einem Lootspawn ist nie durch den
    // Applicator gelaufen und hat kein m_ChefZ_ReturnContainer gesetzt
    // (README_Serving.md §4). Ohne den Record hier gaebe ein solcher Teller beim
    // Leeressen nichts zurueck - und niemand faende den Grund.
    //
    // Ein Basisrecord je Slice:
    //
    //   ChefZ_DishesBPlate   das Gericht. containerCategory sagt, WORAUF es
    //                        liegt; returnContainer nennt die FESTE Klasse, die
    //                        beim letzten Bissen zurueckkommt. "AUTO" ginge
    //                        nicht mehr: es loest ueber den beim Servieren
    //                        benutzten Behaelter auf, und seit 29.08.2026 wird
    //                        keiner mehr benutzt - das Gericht entsteht direkt.
    //
    // Die Basisknoten heissen slice-eindeutig (ChefZ_DishesB...), weil dieses
    // Modul ein GETEILTER Ordner ist: dishes-a und dishes-c legen ihre Gerichte
    // hier ebenfalls ab, und zwei gleichnamige Knoten waeren eine doppelte
    // Definition. Die Config-Vererbung INNERHALB von CfgChefZIngredients loest
    // die Engine selbst auf (Kopf von ChefZ_ConfigCppSource.ReadIngredients).
    //
    // KEINE categories[] und KEINE tags[]: ein fertiges Tellergericht ist
    // Endprodukt und nie wieder Zutat. Truege es eine Kategorie, koennte ein
    // anderes Rezept es als Nachschub aufsaugen - ein stiller Kreis, den
    // niemand mehr findet (dieselbe Begruendung wie bei der Pilzrahmsauce oben).
    //
    // defaultState "COOKED" fuer alle: auch die kalt angerichteten Teller
    // (Wurstbrot, Honigbrot) bestehen aus fertigen Zutaten. Das Rezept setzt
    // dort ausdruecklich PREPARED; dieser Wert hier ist nur die Rueckfallebene
    // der Zustandsprojektion (06 §3, Stufe 2) fuer Exemplare ohne Variable.
    //==========================================================================

    class ChefZ_DishesBPlate
    {
        // VORLAGE, keine Bindung (ChefZ_ConfigCppSource.IsBindingTemplate: template == eigener
        // Klassenname; Erben tragen DIESEN Namen und bleiben Bindungen). Vorfall 31.08.2026:
        // ohne das Feld meldete der Ladebericht je Serverstart 7 Vorlagen als fehlende Klasse.
        template          = "ChefZ_DishesBPlate";
        defaultState      = "COOKED";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        containerCategory = "PLATE";
        returnContainer   = "ChefZ_EmptyPlate";
        decays            = 1;
    };

    class ChefZ_TacticalBreakfast : ChefZ_DishesBPlate {};
    class ChefZ_ScrambledEggSausage : ChefZ_DishesBPlate {};
    class ChefZ_FarmersBreakfast : ChefZ_DishesBPlate {};
    class ChefZ_CheeseFlatbread : ChefZ_DishesBPlate {};
    class ChefZ_SausageBreadPlate : ChefZ_DishesBPlate {};
    class ChefZ_MushroomPan : ChefZ_DishesBPlate {};
    class ChefZ_PotatoPancakes : ChefZ_DishesBPlate {};
    class ChefZ_MeatDumplings : ChefZ_DishesBPlate {};
    // §61.19 ist das einzige Gericht dieses Slice in der SCHUESSEL: Milchreis
    // ist ein Brei und kein Teller (Production Map §60 kennt beide Behaelter).
    class ChefZ_MilkRice : ChefZ_DishesBPlate
    {
        containerCategory = "BOWL";
        returnContainer   = "ChefZ_EmptyBowl";
    };
    class ChefZ_HoneyBreadPlate : ChefZ_DishesBPlate {};

    //--------------------------------------------------------------------------
    // ### SLICE dishes-c ###   Zutatenbindung der fuenf Bowl-Gerichte
    //
    // 16 §3.2: "Die Rueckgabeklasse steht am Gericht, nicht im Rezept."
    // containerCategory und returnContainer gehoeren also an das Item, das
    // gegessen wird - und das ist seit 29.08.2026 das Ergebnis des Rezepts
    // selbst.
    //
    // categories[] ist LEER, und das ist Absicht. Ein fertiges Gericht ist
    // keine Zutat. Truege es eine Kategorie, koennte ein anderes Rezept es als
    // Eingang binden - ein stiller Kreis, den niemand mehr findet (dasselbe
    // Argument, mit dem der Slice sauces der Pilzrahmsauce die Kategorie
    // MUSHROOM verweigert hat).
    //
    // CHEFZ_HOT_MEAL ist der einzige Tag: er existiert bereits (Slice meat)
    // und macht die fuenf fuer spaetere Systeme - Waermeerhalt, Terje-Comp,
    // Cookbook - filterbar, ohne einen neuen Namen in eine zentrale Registry
    // zu tragen.
    //
    // returnContainer nennt ChefZ_EmptyBowl fest statt "AUTO". "AUTO" loest
    // ueber den TATSAECHLICH benutzten Behaelter auf (16 §4); das ist im
    // Rezept richtig, wo der Behaelter erst zur Laufzeit feststeht. Am Gericht
    // steht dagegen fest, was zurueckkommen soll - und eine Schuessel Eintopf
    // gibt eine Schuessel zurueck, auch wenn sie einmal aus einem fremden Napf
    // der Kategorie BOWL serviert wurde.
    //
    // decays = 1: ein fertiges Gericht verdirbt. Die Rate kommt aus dem
    // Preservation Manager ueber den Zustand COOKED (Faktor 0.80, DME §65).
    //--------------------------------------------------------------------------
    class ChefZ_BowlDishIngredient
    {
        // VORLAGE, keine Bindung (ChefZ_ConfigCppSource.IsBindingTemplate: template == eigener
        // Klassenname; Erben tragen DIESEN Namen und bleiben Bindungen). Vorfall 31.08.2026:
        // ohne das Feld meldete der Ladebericht je Serverstart 7 Vorlagen als fehlende Klasse.
        template          = "ChefZ_BowlDishIngredient";
        categories[]      = {};
        tags[]            = {"CHEFZ_HOT_MEAL"};
        defaultState      = "COOKED";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        decays            = 1;
        containerCategory = "BOWL";
        returnContainer   = "ChefZ_EmptyBowl";
    };

    class ChefZ_HunterStewBowl : ChefZ_BowlDishIngredient {};
    class ChefZ_FishermanStewBowl : ChefZ_BowlDishIngredient {};
    class ChefZ_VegetableSoupBowl : ChefZ_BowlDishIngredient {};
    class ChefZ_BoneBrothSoupBowl : ChefZ_BowlDishIngredient {};
    class ChefZ_ChernarusChiliBowl : ChefZ_BowlDishIngredient {};

    //==========================================================================
    // ### SLICE dishes-a ###   Zutatenbindung der Tellergerichte 1-10
    //
    // Gleiche Bauform und gleiche Begruendung wie bei dishes-b weiter oben -
    // sie steht dort einmal und wird hier nicht abgeschrieben. Der Zweck in
    // einem Satz: diese Records sind der Weg AM REZEPT VORBEI. Ein Teller aus
    // Admin- oder Lootspawn ist nie durch den Applicator gelaufen und traegt
    // kein m_ChefZ_ReturnContainer; ohne den Record gaebe er beim Leeressen
    // nichts zurueck (README_Serving.md §4).
    //
    // Eigene, slice-eindeutige Basisknoten (ChefZ_DishesA...), weil dieses
    // Modul ein geteilter Ordner ist und ChefZ_DishesB... bereits vergeben
    // sind. Zwei gleichnamige Knoten waeren eine doppelte Definition.
    //
    // KEINE categories[] und KEINE tags[]: ein fertiges Tellergericht ist
    // Endprodukt und nie wieder Zutat. Truege es eine Kategorie, koennte ein
    // anderes Rezept es als Nachschub aufsaugen - genau der stille Kreis, den
    // die Pilzrahmsauce weiter oben ausdruecklich vermeidet.
    //
    // returnContainer "AUTO" und nicht "ChefZ_EmptyPlate": zurueck kommt genau
    // der Behaelter, der benutzt wurde (16 §4). Ein spaeter hinzugefuegter
    // Holzteller funktioniert damit sofort, ohne dass hier eine Zeile faellt.
    //==========================================================================

    class ChefZ_DishesAPlate
    {
        // VORLAGE, keine Bindung (ChefZ_ConfigCppSource.IsBindingTemplate: template == eigener
        // Klassenname; Erben tragen DIESEN Namen und bleiben Bindungen). Vorfall 31.08.2026:
        // ohne das Feld meldete der Ladebericht je Serverstart 7 Vorlagen als fehlende Klasse.
        template          = "ChefZ_DishesAPlate";
        defaultState      = "COOKED";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        decays            = 1;
        containerCategory = "PLATE";
        returnContainer   = "ChefZ_EmptyPlate";
    };

    class ChefZ_SurvivorSpaghetti : ChefZ_DishesAPlate {};
    class ChefZ_SausagePasta : ChefZ_DishesAPlate {};
    class ChefZ_HunterPasta : ChefZ_DishesAPlate {};
    class ChefZ_CreamMushroomPasta : ChefZ_DishesAPlate {};
    class ChefZ_MacAndCheese : ChefZ_DishesAPlate {};
    class ChefZ_SausagePotatoes : ChefZ_DishesAPlate {};
    class ChefZ_HunterPlate : ChefZ_DishesAPlate {};
    class ChefZ_BloodSausagePlate : ChefZ_DishesAPlate {};
    class ChefZ_FishPotatoPlate : ChefZ_DishesAPlate {};
    class ChefZ_BeanSausagePlate : ChefZ_DishesAPlate {};

    //==========================================================================
    // ### SLICE dishes-vanilla ###   Zutatenbindung der drei neuen Gerichte
    //
    // Gleiche Bauform und gleiche Begruendung wie bei dishes-a und dishes-b
    // weiter oben - sie steht dort und wird hier nicht abgeschrieben. Der Zweck
    // in einem Satz: diese Records sind der Weg AM REZEPT VORBEI, damit ein
    // Teller aus Admin- oder Lootspawn beim Leeressen trotzdem seinen Behaelter
    // zurueckgibt (README_Serving.md §4).
    //
    // Eigene, slice-eindeutige Basisknoten, weil ChefZ_DishesA... und
    // ChefZ_DishesB... bereits vergeben sind und zwei gleichnamige Knoten eine
    // doppelte Definition waeren.
    //
    // ZWEI Portionsknoten und nicht einer: die Kuerbissuppe und das Kompott
    // gehen in eine Schuessel, die Fischpfanne auf einen Teller.
    //
    // KEINE categories[] und KEINE tags[] ausser CHEFZ_HOT_MEAL: ein fertiges
    // Gericht ist Endprodukt und nie wieder Zutat.
    //==========================================================================

    class ChefZ_DishesVanillaBowl
    {
        // VORLAGE, keine Bindung (ChefZ_ConfigCppSource.IsBindingTemplate: template == eigener
        // Klassenname; Erben tragen DIESEN Namen und bleiben Bindungen). Vorfall 31.08.2026:
        // ohne das Feld meldete der Ladebericht je Serverstart 7 Vorlagen als fehlende Klasse.
        template          = "ChefZ_DishesVanillaBowl";
        tags[]            = {"CHEFZ_HOT_MEAL"};
        defaultState      = "COOKED";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        decays            = 1;
        containerCategory = "BOWL";
        returnContainer   = "ChefZ_EmptyBowl";
    };

    class ChefZ_DishesVanillaPlate
    {
        // VORLAGE, keine Bindung (ChefZ_ConfigCppSource.IsBindingTemplate: template == eigener
        // Klassenname; Erben tragen DIESEN Namen und bleiben Bindungen). Vorfall 31.08.2026:
        // ohne das Feld meldete der Ladebericht je Serverstart 7 Vorlagen als fehlende Klasse.
        template          = "ChefZ_DishesVanillaPlate";
        tags[]            = {"CHEFZ_HOT_MEAL"};
        defaultState      = "COOKED";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        decays            = 1;
        containerCategory = "PLATE";
        returnContainer   = "ChefZ_EmptyPlate";
    };

    class ChefZ_PumpkinSoupBowl : ChefZ_DishesVanillaBowl {};
    class ChefZ_SmallFishPan : ChefZ_DishesVanillaPlate {};
    class ChefZ_FruitCompoteBowl : ChefZ_DishesVanillaBowl {};
};

//==============================================================================
// Anmeldung beim Core (02 §4).
//
// Der Knotenname ist der SLICE-Name, nicht der Modulname - CfgChefZ traegt
// genau einen Knoten je Slice (02 §4), und dieses Modul wird sich in
// Meilenstein 3 mit dem Slice "serving" teilen.
//
// loadOrder 300: Saucen sind Zwischenprodukte. Sie werden AUS den Ketten der
// Meilenstein-2-Slices gebaut (Gemuese 210, Salz 205, Kraeuter 220, Milch 260)
// und sind Eingang der Gerichte, die spaeter kommen. Der Core haengt Records
// nicht voneinander ab; die Reihenfolge ist Vorsorge und kostet nichts.
//
// handcraftRecipeSlots = 0: alle vier Rezepte zuenden am Kochgeraet. Vanillas
// Rezeptliste bleibt um kein Bit veraendert.
//
// dataFiles[] beginnt mit dem PBO-Praefix, also dem ORDNERNAMEN des Addons.
//==============================================================================
class CfgChefZ
{
    class ChefZ_Sauces
    {
        chefzApiVersion = 1;
        loadOrder = 300;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Cooking/Config/Recipes/Sauces.json"
        };
    };

    // ### SLICE serving ###
    //
    // Eigener Knoten, weil CfgChefZ genau einen Knoten je SLICE traegt
    // (02 §4) - nicht je Modul. Dieses Modul ist ein geteilter Ordner.
    //
    // handcraftRecipeSlots = 5: dieser Slice bringt GENAU FUENF Transforms
    // mit, deren Prozess exec = "HANDCRAFT" hat -
    //
    //   Tableware.json   TR_CarveWoodenPlate    PROCESS_CARVE_PLATE
    //                    TR_CarveWoodenBowl     PROCESS_CARVE_BOWL
    //   Containers.json  TR_BowlFromBark        PROCESS_CARVE_BOWL_BARK
    //                    TR_BoxFromPaper        PROCESS_FOLD_BOX
    //                    TR_CansFromMetalSheet  PROCESS_CUT_CANS
    //
    // Die Zahl ist eine Reservierung in Vanillas Rezeptliste und muss VOR dem
    // Laden feststehen; Vanilla vergibt Rezept-IDs als Position in seiner
    // Liste, und diese Positionen entstehen im Missionskonstruktor. Steht hier
    // eine zu kleine Zahl, werden die ueberzaehligen Transforms abgewiesen -
    // steht hier gar keine, erscheint KEIN EINZIGES Handwerksrezept dieses
    // Slice, und zwar ohne dass im Spiel etwas darauf hinweist. Die
    // Begruendung im Wortlaut steht im Kopf von ChefZ_HandcraftBridge.c.
    //
    // Wer hier einen Transform ergaenzt, erhoeht diese Zahl in derselben
    // Aenderung. Nachtraeglich eintragen ist keine Loesung: die Rezept-IDs
    // waeren auf Client und Server verschieden.
    //
    // loadOrder 310, also nach den Saucen: Behaelter und Gerichtebasen werden
    // von den Gerichteslices gelesen, nicht umgekehrt. Der Core haengt
    // Records nicht voneinander ab; die Reihenfolge ist Vorsorge.
    //
    // ZWEI dataFiles: Tableware.json bringt die beiden Schnitzvorgaenge aus
    // Brennholz, Containers.json die drei craftbaren Verpackungen aus
    // DME-Plan §32. Getrennte Dateien, weil ein Teller kein Verpackungs-
    // material ist und die beiden Gruppen verschiedene Fragen beantworten.
    class ChefZ_Serving
    {
        chefzApiVersion = 1;
        loadOrder = 310;
        handcraftRecipeSlots = 5;
        dataFiles[] =
        {
            // DREI dataFiles seit dem 31.08.2026. Craftables.json bindet die
            // drei Vanilla-WERKSTOFFE, aus denen die beiden anderen Dateien
            // schnitzen und falten - Firewood, Paper, MetalPlate. Ohne sie war
            // keine dieser Klassen eine deklarierte Zutat, und vier der fuenf
            // Handcraft-Transforms dieses Slice hatten einen Eingangsslot, der
            // nie gefuellt werden konnte (Livetest, Ladebericht). Sie steht
            // ZUERST: wer die Liste liest, soll den Werkstoff vor dem Vorgang
            // sehen, der ihn verbraucht.
            "ChefZ_Cooking/Config/Ingredients/Craftables.json",
            "ChefZ_Cooking/Config/Processing/Tableware.json",
            "ChefZ_Cooking/Config/Processing/Containers.json"
        };
    };

    // ### SLICE cheese ###   Der Kesselschritt der Kaesekette (Todo 10)
    //
    // Eigener Knoten, weil CfgChefZ genau EINEN Knoten je SLICE traegt (02 §4) -
    // nicht je Modul. Dieses Modul ist ein geteilter Ordner.
    //
    // Der Knoten heisst ChefZ_CheeseChain und NICHT ChefZ_Cheese: ChefZ_Cheese
    // ist bereits der Name einer Itemklasse in ChefZ_Ingredients. Zwei
    // verschiedene Configwurzeln (CfgChefZ und CfgVehicles) kollidieren
    // technisch nicht, aber ein Leser, der "ChefZ_Cheese" sucht, faende dann
    // zwei voellig verschiedene Dinge.
    //
    // loadOrder 320: nach den Saucen (300) und den Behaeltern (310), VOR den
    // Gerichteslices (330/340/350). Der Kaesebruch ist ein Zwischenprodukt -
    // die Gerichte lesen aus ihm, er liest aus keinem von ihnen. Der Core
    // haengt Records nicht voneinander ab; die Reihenfolge ist Vorsorge und
    // kostet nichts.
    //
    // handcraftRecipeSlots = 0: das Rezept zuendet am Kochgeraet. Dieser Slice
    // registriert KEIN Handcraft-Rezept, Vanillas Rezeptliste bleibt um kein
    // Bit veraendert (Regel §10.2).
    //
    // GENAU EINE dataFile und KEINE Zutatenbindung: dieser Slice deklariert
    // keine einzige Klasse. Die drei Klassen der Kette - PowderedMilk,
    // ChefZ_MushroomCulture, ChefZ_CheeseCurd - sind samt Kategorien, Tags und
    // Zustaenden bereits von ChefZ_Ingredients in
    // Config/Ingredients/Dairy.json gebunden, und ChefZ_Ingredients steht
    // ohnehin in requiredAddons. Ein zweiter Datensatz derselben Klassen waere
    // ein Doppeleintrag (05 §2, 08 E4).
    //
    // Das zweite Glied der Kette - Bruch pressen zu ChefZ_Cheese - ist ein
    // Transform an einer Station und gehoert damit NICHT hierher, sondern dem
    // Processing-Slice.
    class ChefZ_CheeseChain
    {
        chefzApiVersion = 1;
        loadOrder = 320;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Cooking/Config/Recipes/Cheese.json"
        };
    };

    // ### SLICE dishes-b ###
    //
    // Eigener Knoten, weil CfgChefZ genau EINEN Knoten je SLICE traegt (02 §4) -
    // nicht je Modul. Dieses Modul ist ein geteilter Ordner.
    //
    // loadOrder 330: nach den Saucen (300) und nach den Behaeltern und
    // Gerichtebasen des Slice serving (310). Tellergerichte sind Endprodukte;
    // sie lesen aus allen vorhergehenden Ketten und niemand liest aus ihnen.
    // Der Core haengt Records nicht voneinander ab - die Reihenfolge ist
    // Vorsorge und kostet nichts.
    //
    // handcraftRecipeSlots = 0: alle zehn Rezepte zuenden am Kochgeraet. Dieser
    // Slice registriert KEIN einziges Handcraft-Rezept, Vanillas Rezeptliste
    // bleibt um kein Bit veraendert (Regel §10.2).
    // ZWEI dataFiles seit dem 31.08.2026, und die erste ist kein Versehen:
    // TacticalBacon.json bindet eine FREMDklasse (Vanillas geoeffnete
    // Speckdose) als Zutat. 05 §2 verlangt dafuer den JSON-Weg - eigene
    // Klassen deklarieren sich in der eigenen config.cpp, fremde im
    // Slice-JSON, weil Vanilla-Dateien nie veraendert werden (Regel §10.5).
    // Dieselbe Bauform wie Beans.json im Slice dishes-c.
    //
    // Sie steht VOR der Rezeptdatei: der Core haengt Records zwar nicht
    // voneinander ab, aber wer die Liste liest, soll die Zutat vor dem Rezept
    // sehen, das sie braucht.
    class ChefZ_DishesB
    {
        chefzApiVersion = 1;
        loadOrder = 330;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Cooking/Config/Ingredients/TacticalBacon.json",
            "ChefZ_Cooking/Config/Recipes/DishesB.json"
        };
    };

    // ### SLICE dishes-c ###
    //
    // Eigener Knoten, weil CfgChefZ genau EINEN Knoten je SLICE traegt (02 §4) -
    // nicht je Modul. Dieses Modul ist ein geteilter Ordner.
    //
    // loadOrder 340: nach den Saucen (300), den Behaeltern (310) und den
    // Tellergerichten (330). Die Bowl-Gerichte lesen aus der Bruehe des Slice
    // sauces und aus den Behaeltern des Slice serving; niemand liest aus
    // ihnen. Der Core haengt Records nicht voneinander ab - die Reihenfolge
    // ist Vorsorge und kostet nichts.
    //
    // handcraftRecipeSlots = 0: alle zwoelf Rezepte zuenden am Kochgeraet.
    // Dieser Slice registriert KEIN Handcraft-Rezept; Vanillas Rezeptliste
    // bleibt um kein Bit veraendert (Regel §10.2).
    //
    // ZWEI dataFiles, und der zweite ist kein Versehen: Beans.json bindet eine
    // FREMDklasse (Vanillas geoeffnete Bohnendose) als Zutat. 05 §2 verlangt
    // dafuer den JSON-Weg - eigene Klassen deklarieren sich in der eigenen
    // config.cpp, fremde im Slice-JSON, weil Vanilla-Dateien nie veraendert
    // werden (Regel §10.5).
    class ChefZ_DishesC
    {
        chefzApiVersion = 1;
        loadOrder = 340;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Cooking/Config/Ingredients/Beans.json",
            "ChefZ_Cooking/Config/Recipes/BowlDishes.json"
        };
    };

    // ### SLICE dishes-a ###
    //
    // Eigener Knoten, weil CfgChefZ genau EINEN Knoten je SLICE traegt (02 §4) -
    // nicht je Modul. Dieses Modul ist ein geteilter Ordner.
    //
    // loadOrder 330: nach den Saucen (300) und den Behaeltern (310), vor den
    // Bowl-Gerichten (340). Die Tellergerichte lesen aus der Tomaten- und der
    // Rahmsauce des Slice sauces und aus den Behaeltern des Slice serving;
    // niemand liest aus ihnen. Der Core haengt Records nicht voneinander ab -
    // die Reihenfolge ist Vorsorge und kostet nichts.
    //
    // handcraftRecipeSlots = 0: alle zehn Rezepte zuenden am Kochgeraet.
    // Dieser Slice registriert KEIN Handcraft-Rezept; Vanillas Rezeptliste
    // bleibt um kein Bit veraendert (Regel §10.2).
    //
    // GENAU EINE dataFile: die Zutatenbindung der zehn Gerichte steht in Rang 1
    // weiter oben (der Client liest sie garantiert), und die einzige
    // Fremdklasse dieses Slice mit eigenem Zutatendatensatz -
    // BakedBeansCan_Opened - ist bereits von dishes-c in
    // Config/Ingredients/Beans.json deklariert. Ein zweiter Datensatz derselben
    // Klasse waere ein Doppeleintrag; der Bohnen-Wurst-Teller bindet deshalb
    // ueber die Kategorie BEANS aus jenem Datensatz (08 E4).
    class ChefZ_DishesA
    {
        chefzApiVersion = 1;
        loadOrder = 330;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Cooking/Config/Recipes/Dishes_A.json"
        };
    };

    // ### SLICE dishes-vanilla ###
    //
    // Die drei Gerichte aus den bisher ungenutzten Vanilla-Assets
    // (Vanilla-Audit §3). Eigener Knoten, weil CfgChefZ genau EINEN Knoten je
    // SLICE traegt (02 §4) - dieses Modul ist ein geteilter Ordner.
    //
    // loadOrder 350: nach allen anderen Gerichteslices. Die drei lesen aus der
    // Zutatenbindung des Slice vanilla-foods (loadOrder 240) und aus der Bruehe
    // und den Behaeltern dieses Moduls; niemand liest aus ihnen. Der Core
    // haengt Records nicht voneinander ab - die Reihenfolge ist Vorsorge.
    //
    // handcraftRecipeSlots = 0: alle drei Rezepte zuenden am Kochgeraet.
    // Dieser Slice registriert KEIN Handcraft-Rezept; Vanillas Rezeptliste
    // bleibt um kein Bit veraendert.
    class ChefZ_DishesVanilla
    {
        chefzApiVersion = 1;
        loadOrder = 350;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Cooking/Config/Recipes/DishesVanilla.json"
        };
    };

};
