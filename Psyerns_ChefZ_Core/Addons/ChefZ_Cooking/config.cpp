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
            // Je Gericht ZWEI Klassen, und das ist kein Wildwuchs, sondern das
            // Muster aus Config/Recipes/README_Serving.md §1: das Bulk entsteht
            // im Kochgeraet und traegt den Portionszaehler, die servierte
            // Portion ist das, was der Spieler vom Teller isst. Der Name der
            // Portion ist der Name aus Production Map §72 und DME-Plan §53 -
            // sie ist das Gericht, das Bulk ist die Pfanne davor.
            "ChefZ_TacticalBreakfastBulk",
            "ChefZ_TacticalBreakfast",
            "ChefZ_ScrambledEggSausageBulk",
            "ChefZ_ScrambledEggSausage",
            "ChefZ_FarmersBreakfastBulk",
            "ChefZ_FarmersBreakfast",
            "ChefZ_CheeseFlatbreadBulk",
            "ChefZ_CheeseFlatbread",
            "ChefZ_SausageBreadPlateBulk",
            "ChefZ_SausageBreadPlate",
            "ChefZ_MushroomPanBulk",
            "ChefZ_MushroomPan",
            "ChefZ_PotatoPancakesBulk",
            "ChefZ_PotatoPancakes",
            "ChefZ_MeatDumplingsBulk",
            "ChefZ_MeatDumplings",
            "ChefZ_MilkRiceBulk",
            "ChefZ_MilkRice",
            "ChefZ_HoneyBreadPlateBulk",
            "ChefZ_HoneyBreadPlate",

            // ### SLICE dishes-c ###   Suppen und Eintoepfe (Production Map §62)
            //
            // Je Gericht ZWEI Klassen, nach dem Muster aus 15 §2:
            //   ...Bulk   das, was im Topf oder Kessel entsteht und den
            //             Portionszaehler traegt
            //   ...Bowl   die entnommene Schuessel, die gegessen wird
            //
            // Die Endung "Bowl" statt des blossen Gerichtsnamens ist Absicht
            // und folgt der Schreibweise, die 15 §2 woertlich verwendet
            // (ChefZ_HunterStewBulk / ChefZ_HunterStewBowl). Bei einem
            // Bowl-Gericht ist der Behaelter Teil der Identitaet - das
            // Tellergericht heisst zu Recht anders.
            "ChefZ_HunterStewBulk",
            "ChefZ_HunterStewBowl",
            "ChefZ_FishermanStewBulk",
            "ChefZ_FishermanStewBowl",
            "ChefZ_VegetableSoupBulk",
            "ChefZ_VegetableSoupBowl",
            "ChefZ_BoneBrothSoupBulk",
            "ChefZ_BoneBrothSoupBowl",
            "ChefZ_ChernarusChiliBulk",
            "ChefZ_ChernarusChiliBowl",

            // ### SLICE dishes-a ###   Tellergerichte 1-10 (Production Map §61)
            //
            // Dasselbe Paar je Gericht wie in dishes-b: ...Bulk ist das, was im
            // Kochgeraet entsteht und den Portionszaehler traegt, der blosse
            // Gerichtsname ist die servierte Portion vom Teller
            // (Config/Recipes/README_Serving.md §1, DME-Plan §53).
            "ChefZ_SurvivorSpaghettiBulk",
            "ChefZ_SurvivorSpaghetti",
            "ChefZ_SausagePastaBulk",
            "ChefZ_SausagePasta",
            "ChefZ_HunterPastaBulk",
            "ChefZ_HunterPasta",
            "ChefZ_CreamMushroomPastaBulk",
            "ChefZ_CreamMushroomPasta",
            "ChefZ_MacAndCheeseBulk",
            "ChefZ_MacAndCheese",
            "ChefZ_SausagePotatoesBulk",
            "ChefZ_SausagePotatoes",
            "ChefZ_HunterPlateBulk",
            "ChefZ_HunterPlate",
            "ChefZ_BloodSausagePlateBulk",
            "ChefZ_BloodSausagePlate",
            "ChefZ_FishPotatoPlateBulk",
            "ChefZ_FishPotatoPlate",
            "ChefZ_BeanSausagePlateBulk",
            "ChefZ_BeanSausagePlate"
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
        //                    ChefZ_Bread, ChefZ_Flatbread, ChefZ_SimpleDough,
        //                    ChefZ_YeastDough und ChefZ_PastaDough - alle in
        //                    ChefZ_Baking/Config/GrainIngredients.json.
        //   Alles Weitere, was dieser Slice anfasst, steht bereits oben:
        //   FLOUR liegt in ChefZ_Processing, EGG/DAIRY/SALT/SPICE/MUSHROOM und
        //   ChefZ_SlicedPotato in ChefZ_Ingredients, HERB und die Zwiebel in
        //   ChefZ_Farming, SAUSAGE/MINCED_MEAT/FAT in ChefZ_Meat.
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
            fullnessIndex = 60;
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
                class Raw    { nutrition_properties[] = {60, 150, 320, 45, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {60, 158, 280, 45, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {60, 150, 320, 45, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {15, 38, 40, 8, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {24, 60, 128, 9, 20, 16, 1}; };
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
            fullnessIndex = 45;
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
                class Raw    { nutrition_properties[] = {45, 130, 105, 35, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {45, 135, 85, 35, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {45, 130, 125, 35, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {11, 33, 13, 7, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {18, 52, 42, 7, 20, 16, 1}; };
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
            fullnessIndex = 40;
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
                class Raw    { nutrition_properties[] = {40, 620, 90, 20, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {40, 635, 70, 20, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {40, 620, 100, 20, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {10, 155, 12, 4, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {16, 248, 36, 4, 20, 16, 1}; };
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
            fullnessIndex = 45;
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
                class Raw    { nutrition_properties[] = {45, 330, 130, 40, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {45, 340, 110, 40, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {45, 330, 140, 40, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {11, 83, 16, 8, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {18, 132, 52, 8, 20, 16, 1}; };
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
    class ChefZ_EmptyCan : ChefZ_ContainerItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_EMPTYCAN0";
        descriptionShort = "#STR_CHEFZ_ITEM_EMPTYCAN1";
        // PROXY: Dosensilhouette. Bedarf: eigenes Dosenmesh (P3, V2).
        model = "\dz\gear\food\PowderedMilk.p3d";
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
        // PROXY: zylindrischer Behaelter. Bedarf: eigenes Glasmesh (P3, V2).
        model = "\dz\gear\food\PowderedMilk.p3d";
        itemSize[] = {1, 2};
        weight = 210;
    };

    // §18, DME-Plan §32: Lebensmittelbox fuer Trockenware.
    class ChefZ_EmptyBox : ChefZ_ContainerItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_EMPTYBOX0";
        descriptionShort = "#STR_CHEFZ_ITEM_EMPTYBOX1";
        // PROXY: Kartonsilhouette. Bedarf: eigenes Boxmesh (P3, V2).
        model = "\dz\gear\food\BoxCereal.p3d";
        itemSize[] = {2, 2};
        weight = 110;
    };

    // ------------------------------------------------------------------------
    // Das Bulk-Gericht im Kochgefaess (15 §2).
    //
    //     config.cpp   class ChefZ_HunterStewBulk : ChefZ_PortionedDish_Base
    //     script       class ChefZ_HunterStewBulk extends ChefZ_PortionedDish_Base {}
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
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        varQuantityDestroyOnMin = 1;
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
        model = "\dz\gear\cooking\FryingPan.p3d";
        rotationFlags = 17;
        itemSize[] = {2, 2};
        weight = 450;
        absorbency = 0.5;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        varQuantityDestroyOnMin = 1;
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
    // Warum je Gericht ZWEI Klassen und keine einzige mehr
    // --------------------------------------------------------------------------
    // Config/Recipes/README_Serving.md §1, woertlich: ein Bulk-Gericht (das, was
    // im Kochgeraet entsteht und den Portionszaehler traegt) und eine servierte
    // Portion (das, was auf dem Teller liegt und gegessen wird). Einzelgerichte
    // sind Portionsgerichte mit kleiner Portionszahl (15 E7) - es gibt genau
    // EINEN Mechanismus, auch fuer Tellergerichte.
    //
    // Die Portion traegt den Namen aus Production Map §72 / DME-Plan §53
    // (ChefZ_TacticalBreakfast); das Bulk haengt "Bulk" an. Qualitaetsvarianten
    // je Stufe gibt es bewusst NICHT: OF-05 ist als B entschieden (Ausbeute
    // statt eigener Klasse je Stufe), und 25 Gerichte x 4 Stufen waeren 100
    // Klassen mit Modell, Stringtable und Loot-Eintrag.
    //
    // --------------------------------------------------------------------------
    // Warum das Bulk einen Food-Knoten hat und die Portion keinen
    // --------------------------------------------------------------------------
    // Das Bulk steht im Topf, waehrend Vanilla weiterkocht. ChefZ_PortionedDish_Base
    // bringt FoodStages UND FoodStageTransitions mit (01 V4) - ohne die Uebergaenge
    // wuerde jedes Bulk beim ersten Garstufenwechsel zu Kohle. Die Klassen hier
    // ergaenzen in denselben Stufenknoten nur ihre nutrition_properties[]; visual_
    // und cooking_properties bleiben geerbt.
    //
    // Die servierte Portion erbt von ChefZ_ServedDish_Base, das bewusst KEINEN
    // Food-Knoten hat: sie liegt auf dem Teller und nicht mehr im Feuer, also
    // liefert HasFoodStage() false (ItemBase.c:2654) und sie kann nicht verbrennen.
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
    // Es gibt fuer KEINES dieser zwanzig Items ein eigenes Mesh. Alle tragen ein
    // Vanilla-Proxy: das Bulk CookingPot.p3d (es steht im Kochgeraet), die Portion
    // FryingPan.p3d (flach, liest sich als Teller), der Milchreis in der Schuessel
    // CookingPot.p3d. Sobald eigene Geometrie existiert, wird genau diese eine
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
    // Das Bulk traegt dieselben Werte wie EINE Portion. Der Naehrwert eines
    // Bissens haengt in DayZ an Klasse x Foodstage, nie an der Restmenge
    // (01 V6, 13 §2) - wer direkt aus dem Geraet isst, bekommt damit genau eine
    // Portion und keinen Vorteil gegenueber dem, der einen Teller benutzt.
    //--------------------------------------------------------------------------
    class ChefZ_TacticalBreakfastBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_TACTICALBREAKFAST_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_TACTICALBREAKFAST_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 900;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 75;
            energy = 570;
            water = 40;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {75, 570, 40, 55, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {75, 570, 36, 55, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {75, 570, 40, 55, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {19, 142, 6, 11, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {30, 228, 32, 11, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_TacticalBreakfast : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_TACTICALBREAKFAST";
        descriptionShort = "#STR_CHEFZ_ITEM_TACTICALBREAKFAST_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 480;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 75;
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
    // Das Bulk traegt dieselben Werte wie EINE Portion. Der Naehrwert eines
    // Bissens haengt in DayZ an Klasse x Foodstage, nie an der Restmenge
    // (01 V6, 13 §2) - wer direkt aus dem Geraet isst, bekommt damit genau eine
    // Portion und keinen Vorteil gegenueber dem, der einen Teller benutzt.
    //--------------------------------------------------------------------------
    class ChefZ_ScrambledEggSausageBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SCRAMBLEDEGGSAUSAGE_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_SCRAMBLEDEGGSAUSAGE_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 900;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 103;
            energy = 480;
            water = 268;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {103, 480, 268, 60, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {103, 480, 241, 60, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {103, 480, 268, 60, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {26, 120, 40, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {41, 192, 214, 12, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_ScrambledEggSausage : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SCRAMBLEDEGGSAUSAGE";
        descriptionShort = "#STR_CHEFZ_ITEM_SCRAMBLEDEGGSAUSAGE_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 500;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 103;
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
    //
    // Das Bulk traegt dieselben Werte wie EINE Portion. Der Naehrwert eines
    // Bissens haengt in DayZ an Klasse x Foodstage, nie an der Restmenge
    // (01 V6, 13 §2) - wer direkt aus dem Geraet isst, bekommt damit genau eine
    // Portion und keinen Vorteil gegenueber dem, der einen Teller benutzt.
    //--------------------------------------------------------------------------
    class ChefZ_FarmersBreakfastBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FARMERSBREAKFAST_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_FARMERSBREAKFAST_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 1000;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 154;
            energy = 650;
            water = 136;
            nutritionalIndex = 65;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {154, 650, 136, 65, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {154, 650, 122, 65, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {154, 650, 136, 65, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {38, 162, 20, 13, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {62, 260, 109, 13, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_FarmersBreakfast : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FARMERSBREAKFAST";
        descriptionShort = "#STR_CHEFZ_ITEM_FARMERSBREAKFAST_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 620;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 154;
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
    // Das Bulk traegt dieselben Werte wie EINE Portion. Der Naehrwert eines
    // Bissens haengt in DayZ an Klasse x Foodstage, nie an der Restmenge
    // (01 V6, 13 §2) - wer direkt aus dem Geraet isst, bekommt damit genau eine
    // Portion und keinen Vorteil gegenueber dem, der einen Teller benutzt.
    //--------------------------------------------------------------------------
    class ChefZ_CheeseFlatbreadBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHEESEFLATBREAD_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_CHEESEFLATBREAD_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 820;
        lifetime = 21600;

        class Nutrition
        {
            fullnessIndex = 35;
            energy = 375;
            water = 75;
            nutritionalIndex = 50;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {35, 375, 75, 50, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {35, 375, 68, 50, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {35, 375, 75, 50, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {9, 94, 11, 10, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {14, 150, 60, 10, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_CheeseFlatbread : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHEESEFLATBREAD";
        descriptionShort = "#STR_CHEFZ_ITEM_CHEESEFLATBREAD_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 380;
        lifetime = 21600;

        class Nutrition
        {
            fullnessIndex = 35;
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
    // Das Bulk traegt dieselben Werte wie EINE Portion. Der Naehrwert eines
    // Bissens haengt in DayZ an Klasse x Foodstage, nie an der Restmenge
    // (01 V6, 13 §2) - wer direkt aus dem Geraet isst, bekommt damit genau eine
    // Portion und keinen Vorteil gegenueber dem, der einen Teller benutzt.
    //--------------------------------------------------------------------------
    class ChefZ_SausageBreadPlateBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SAUSAGEBREADPLATE_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_SAUSAGEBREADPLATE_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 900;
        lifetime = 18000;

        class Nutrition
        {
            fullnessIndex = 110;
            energy = 690;
            water = 50;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {110, 690, 50, 55, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {110, 690, 45, 55, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {110, 690, 50, 55, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {28, 172, 8, 11, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {44, 276, 40, 11, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_SausageBreadPlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SAUSAGEBREADPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_SAUSAGEBREADPLATE_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 520;
        lifetime = 18000;

        class Nutrition
        {
            fullnessIndex = 110;
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
    // Das Bulk traegt dieselben Werte wie EINE Portion. Der Naehrwert eines
    // Bissens haengt in DayZ an Klasse x Foodstage, nie an der Restmenge
    // (01 V6, 13 §2) - wer direkt aus dem Geraet isst, bekommt damit genau eine
    // Portion und keinen Vorteil gegenueber dem, der einen Teller benutzt.
    //--------------------------------------------------------------------------
    class ChefZ_MushroomPanBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MUSHROOMPAN_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_MUSHROOMPAN_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 860;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 58;
            energy = 480;
            water = 80;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {58, 480, 80, 60, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {58, 480, 72, 60, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {58, 480, 80, 60, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {14, 120, 12, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {23, 192, 64, 12, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_MushroomPan : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MUSHROOMPAN";
        descriptionShort = "#STR_CHEFZ_ITEM_MUSHROOMPAN_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 420;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 58;
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
    // Das Bulk traegt dieselben Werte wie EINE Portion. Der Naehrwert eines
    // Bissens haengt in DayZ an Klasse x Foodstage, nie an der Restmenge
    // (01 V6, 13 §2) - wer direkt aus dem Geraet isst, bekommt damit genau eine
    // Portion und keinen Vorteil gegenueber dem, der einen Teller benutzt.
    //--------------------------------------------------------------------------
    class ChefZ_PotatoPancakesBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_POTATOPANCAKES_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_POTATOPANCAKES_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 900;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 93;
            energy = 565;
            water = 83;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {93, 565, 83, 55, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {93, 565, 75, 55, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {93, 565, 83, 55, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {23, 141, 12, 11, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {37, 226, 66, 11, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_PotatoPancakes : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_POTATOPANCAKES";
        descriptionShort = "#STR_CHEFZ_ITEM_POTATOPANCAKES_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 500;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 93;
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
    // Das Bulk traegt dieselben Werte wie EINE Portion. Der Naehrwert eines
    // Bissens haengt in DayZ an Klasse x Foodstage, nie an der Restmenge
    // (01 V6, 13 §2) - wer direkt aus dem Geraet isst, bekommt damit genau eine
    // Portion und keinen Vorteil gegenueber dem, der einen Teller benutzt.
    //--------------------------------------------------------------------------
    class ChefZ_MeatDumplingsBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MEATDUMPLINGS_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_MEATDUMPLINGS_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 900;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 140;
            energy = 360;
            water = 115;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {140, 360, 115, 60, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {140, 360, 104, 60, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {140, 360, 115, 60, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {35, 90, 17, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {56, 144, 92, 12, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_MeatDumplings : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MEATDUMPLINGS";
        descriptionShort = "#STR_CHEFZ_ITEM_MEATDUMPLINGS_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 460;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 140;
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
    // Das Bulk traegt dieselben Werte wie EINE Portion. Der Naehrwert eines
    // Bissens haengt in DayZ an Klasse x Foodstage, nie an der Restmenge
    // (01 V6, 13 §2) - wer direkt aus dem Geraet isst, bekommt damit genau eine
    // Portion und keinen Vorteil gegenueber dem, der einen Teller benutzt.
    //--------------------------------------------------------------------------
    class ChefZ_MilkRiceBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MILKRICE_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_MILKRICE_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 950;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 55;
            energy = 280;
            water = 205;
            nutritionalIndex = 50;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {55, 280, 205, 50, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {55, 280, 184, 50, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {55, 280, 205, 50, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {14, 70, 31, 10, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {22, 112, 164, 10, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_MilkRice : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MILKRICE";
        descriptionShort = "#STR_CHEFZ_ITEM_MILKRICE_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 540;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 55;
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
    // Das Bulk traegt dieselben Werte wie EINE Portion. Der Naehrwert eines
    // Bissens haengt in DayZ an Klasse x Foodstage, nie an der Restmenge
    // (01 V6, 13 §2) - wer direkt aus dem Geraet isst, bekommt damit genau eine
    // Portion und keinen Vorteil gegenueber dem, der einen Teller benutzt.
    //--------------------------------------------------------------------------
    class ChefZ_HoneyBreadPlateBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HONEYBREADPLATE_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_HONEYBREADPLATE_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 820;
        lifetime = 21600;

        class Nutrition
        {
            fullnessIndex = 30;
            energy = 400;
            water = 20;
            nutritionalIndex = 45;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {30, 400, 20, 45, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {30, 400, 18, 45, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {30, 400, 20, 45, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {8, 100, 3, 9, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {12, 160, 16, 9, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_HoneyBreadPlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HONEYBREADPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_HONEYBREADPLATE_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 360;
        lifetime = 21600;

        class Nutrition
        {
            fullnessIndex = 30;
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
    // Je Gericht zwei Klassen nach dem Muster aus 15 §2:
    //
    //     ChefZ_<Name>Bulk : ChefZ_PortionedDish_Base    im Topf, mit Zaehler
    //     ChefZ_<Name>Bowl : ChefZ_ServedDish_Base       in der Hand, essbar
    //
    // WOHER DIE NAEHRWERTE KOMMEN (13, Architekturplan §10):
    // Sie sind aus den Zutaten ABGELEITET, nicht gesetzt. Ueber jeder Klasse
    // steht die Rechnung. Grundlage sind die Naehrwerte, die die
    // Meilenstein-2-Slices in ihren Deltas festgelegt haben und die inzwischen
    // in ChefZ_Registry/Config/Nutrition.json stehen:
    //
    //     ChefZ_SlicedPotato  180 /  45 / 40    ChefZ_Carrot     100 /  60 / 30
    //     ChefZ_Onion          90 /  55 / 25    ChefZ_Cabbage    110 /  80 / 45
    //     ChefZ_ChoppedTomato  45 /  70 / 20    ChefZ_BoneBroth  150 / 320 / 60
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
    // ChefZ_PortionedDish_Base hat varQuantityMax = 1. Wer direkt aus dem Topf
    // isst, isst genau eine Portion. Ein eigener Wert waere ein zweiter
    // Balancinghebel fuer denselben Bissen.
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
    // 3D-ASSETS: alle zehn Klassen tragen ein VANILLA-PROXY.
    //     Bulk -> \dz\gear\cooking\CookingPot.p3d
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
    class ChefZ_HunterStewBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERSTEW_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERSTEW_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";
        itemSize[] = {3, 2};
        weight = 1200;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 90;
            energy = 270;
            water = 160;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {90, 270, 160, 55, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {90, 284, 141, 55, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {90, 270, 160, 55, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {23, 68, 40, 10, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {36, 108, 64, 11, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_HunterStewBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERSTEW";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERSTEW_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";
        itemSize[] = {2, 2};
        weight = 480;
        lifetime = 7200;

        class Nutrition
        {
            fullnessIndex = 90;
            energy = 270;
            water = 160;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // §62 Fisherman's Stew - Fisch, Kartoffel, Karotte, Dill.
    // DME §42: Fischgericht -> ausgewogene Werte, hoher Naehrwertindex.
    //
    //   2x Fischfilet     2 x 160 /  40 /  95 = 320 /  80 / 190   (Vanilla)
    //   2x Kartoffel      2 x 180 /  45 /  40 = 360 /  90 /  80
    //   1x Karotte            100 /  60 /  30 = 100 /  60 /  30
    //   1x Dill                15 /  12 /   5 =  15 /  12 /   5
    //   Wasser im Topf                        =   0 / 400 /   0
    //   ------------------------------------------------------------------
    //   Summe                                   795 / 642 / 305
    //   x 1.10 / 4 Portionen                 -> 220 / 160 /  75
    //--------------------------------------------------------------------------
    class ChefZ_FishermanStewBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FISHERMANSTEW_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_FISHERMANSTEW_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";
        itemSize[] = {3, 2};
        weight = 1150;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 75;
            energy = 220;
            water = 160;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {75, 220, 160, 60, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {75, 231, 141, 60, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {75, 220, 160, 60, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {19, 55, 40, 11, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {30, 88, 64, 12, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_FishermanStewBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FISHERMANSTEW";
        descriptionShort = "#STR_CHEFZ_ITEM_FISHERMANSTEW_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";
        itemSize[] = {2, 2};
        weight = 470;
        lifetime = 7200;

        class Nutrition
        {
            fullnessIndex = 75;
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
    //   ------------------------------------------------------------------
    //   Summe                                   660 / 785 / 180
    //   x 1.00 / 4 Portionen                 -> 165 / 195 /  45
    //--------------------------------------------------------------------------
    class ChefZ_VegetableSoupBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_VEGETABLESOUP_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_VEGETABLESOUP_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";
        itemSize[] = {3, 2};
        weight = 1100;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 45;
            energy = 165;
            water = 195;
            nutritionalIndex = 65;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {45, 165, 195, 65, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {45, 173, 172, 65, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {45, 165, 195, 65, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {11, 41, 49, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {18, 66, 78, 13, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_VegetableSoupBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_VEGETABLESOUP";
        descriptionShort = "#STR_CHEFZ_ITEM_VEGETABLESOUP_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";
        itemSize[] = {2, 2};
        weight = 450;
        lifetime = 7200;

        class Nutrition
        {
            fullnessIndex = 45;
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
    class ChefZ_BoneBrothSoupBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BONEBROTHSOUP_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_BONEBROTHSOUP_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";
        itemSize[] = {3, 2};
        weight = 1250;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 55;
            energy = 180;
            water = 200;
            nutritionalIndex = 70;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {55, 180, 200, 70, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {55, 189, 176, 70, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {55, 180, 200, 70, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {14, 45, 50, 13, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {22, 72, 80, 14, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_BoneBrothSoupBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BONEBROTHSOUP";
        descriptionShort = "#STR_CHEFZ_ITEM_BONEBROTHSOUP_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";
        itemSize[] = {2, 2};
        weight = 500;
        lifetime = 7200;

        class Nutrition
        {
            fullnessIndex = 55;
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
    //   ------------------------------------------------------------------
    //   Summe                                  1190 / 335 / 423
    //   x 1.10 / 4 Portionen                 ->  325 /  85 / 105
    //--------------------------------------------------------------------------
    class ChefZ_ChernarusChiliBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHERNARUSCHILI_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_CHERNARUSCHILI_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";
        itemSize[] = {3, 2};
        weight = 1300;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 105;
            energy = 325;
            water = 85;
            nutritionalIndex = 50;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {105, 325, 85, 50, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {105, 341, 75, 50, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {105, 325, 85, 50, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {26, 81, 21, 9, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {42, 130, 34, 10, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_ChernarusChiliBowl : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHERNARUSCHILI";
        descriptionShort = "#STR_CHEFZ_ITEM_CHERNARUSCHILI_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";
        itemSize[] = {2, 2};
        weight = 520;
        lifetime = 7200;

        class Nutrition
        {
            fullnessIndex = 105;
            energy = 325;
            water = 85;
            nutritionalIndex = 50;
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
    // Slice dishes-b weiter oben - beide Slices bauen dieselben zwei Klassen je
    // Gericht (Bulk + servierte Portion, Config/Recipes/README_Serving.md §1),
    // und eine zweite Abschrift derselben Begruendung waere eine zweite Stelle,
    // an der sie veralten kann. Hier steht nur, was fuer DIESE zehn Gerichte
    // eigens gilt:
    //
    //   1. NAEHRWERT JE GERICHT: die Zahl unter jeder Klasse ist die Summe
    //      EINER Portion aus den Zutatenwerten der Registry, mal
    //      nutritionModifier des Rezepts. Bulk und Portion tragen DIESELBEN
    //      Werte - der Naehrwert eines Bissens haengt an Klasse x Foodstage und
    //      nie an der Restmenge (01 V6). Wer direkt aus der Pfanne isst,
    //      bekommt damit genau eine Portion und keinen Vorteil gegenueber dem,
    //      der einen Teller benutzt. Dieselbe Regel wie in dishes-b.
    //
    //   2. amountPerPortion = 2.0 in jedem Rezept: eine Portion kostet rund
    //      zwei Zutateneinheiten, und genau so ist die Naehrwertrechnung
    //      aufgestellt (Minimalfuellung -> eine Portion, doppelte Fuellung ->
    //      zwei). Optionale Slots zaehlen dabei nicht mit (15 §5.2) - Gewuerze
    //      koennen die Ausbeute also nicht hochkaufen.
    //
    //   3. MODELLE: kein Gericht hat ein eigenes Mesh. Bulk = CookingPot.p3d
    //      oder FryingPan.p3d (je nachdem, worin es entsteht), Portion =
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
    class ChefZ_SurvivorSpaghettiBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SURVIVORSPAGHETTI_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_SURVIVORSPAGHETTI_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 900;
        lifetime = 14400;

        class Nutrition
        {
            fullnessIndex = 80;
            energy = 560;
            water = 120;
            nutritionalIndex = 50;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {80, 560, 120, 50, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {80, 560, 108, 50, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {80, 560, 120, 50, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {20, 140, 18, 10, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {32, 224, 48, 10, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_SurvivorSpaghetti : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SURVIVORSPAGHETTI";
        descriptionShort = "#STR_CHEFZ_ITEM_SURVIVORSPAGHETTI_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 470;
        lifetime = 14400;

        class Nutrition
        {
            fullnessIndex = 80;
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
    class ChefZ_SausagePastaBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SAUSAGEPASTA_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_SAUSAGEPASTA_BULK_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 920;
        lifetime = 14400;

        class Nutrition
        {
            fullnessIndex = 230;
            energy = 1270;
            water = 70;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {230, 1270, 70, 60, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {230, 1270, 63, 60, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {230, 1270, 70, 60, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {58, 318, 11, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {92, 508, 28, 12, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_SausagePasta : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SAUSAGEPASTA";
        descriptionShort = "#STR_CHEFZ_ITEM_SAUSAGEPASTA_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 490;
        lifetime = 14400;

        class Nutrition
        {
            fullnessIndex = 230;
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
    class ChefZ_HunterPastaBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERPASTA_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERPASTA_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 920;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 185;
            energy = 810;
            water = 145;
            nutritionalIndex = 70;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {185, 810, 145, 70, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {185, 810, 131, 70, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {185, 810, 145, 70, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {46, 203, 22, 14, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {74, 324, 58, 14, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_HunterPasta : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERPASTA";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERPASTA_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 490;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 185;
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
    class ChefZ_CreamMushroomPastaBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CREAMMUSHROOMPASTA_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_CREAMMUSHROOMPASTA_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 920;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 100;
            energy = 950;
            water = 250;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {100, 950, 250, 60, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {100, 950, 225, 60, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {100, 950, 250, 60, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {25, 238, 38, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {40, 380, 100, 12, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_CreamMushroomPasta : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CREAMMUSHROOMPASTA";
        descriptionShort = "#STR_CHEFZ_ITEM_CREAMMUSHROOMPASTA_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 490;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 100;
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
    class ChefZ_MacAndCheeseBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MACANDCHEESE_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_MACANDCHEESE_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 940;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 100;
            energy = 1130;
            water = 510;
            nutritionalIndex = 55;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {100, 1130, 510, 55, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {100, 1130, 459, 55, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {100, 1130, 510, 55, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {25, 283, 77, 11, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {40, 452, 204, 11, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_MacAndCheese : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MACANDCHEESE";
        descriptionShort = "#STR_CHEFZ_ITEM_MACANDCHEESE_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 500;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 100;
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
    class ChefZ_SausagePotatoesBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SAUSAGEPOTATOES_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_SAUSAGEPOTATOES_BULK_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 920;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 230;
            energy = 1000;
            water = 65;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {230, 1000, 65, 60, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {230, 1000, 59, 60, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {230, 1000, 65, 60, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {58, 250, 10, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {92, 400, 26, 12, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_SausagePotatoes : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SAUSAGEPOTATOES";
        descriptionShort = "#STR_CHEFZ_ITEM_SAUSAGEPOTATOES_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 490;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 230;
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
    class ChefZ_HunterPlateBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERPLATE_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERPLATE_BULK_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 920;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 200;
            energy = 580;
            water = 145;
            nutritionalIndex = 70;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {200, 580, 145, 70, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {200, 580, 131, 70, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {200, 580, 145, 70, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {50, 145, 22, 14, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {80, 232, 58, 14, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_HunterPlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERPLATE_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 490;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 200;
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
    class ChefZ_BloodSausagePlateBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BLOODSAUSAGEPLATE_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_BLOODSAUSAGEPLATE_BULK_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 920;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 215;
            energy = 780;
            water = 120;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {215, 780, 120, 60, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {215, 780, 108, 60, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {215, 780, 120, 60, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {54, 195, 18, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {86, 312, 48, 12, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_BloodSausagePlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BLOODSAUSAGEPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_BLOODSAUSAGEPLATE_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 490;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 215;
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
    class ChefZ_FishPotatoPlateBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FISHPOTATOPLATE_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_FISHPOTATOPLATE_BULK_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 900;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 150;
            energy = 435;
            water = 110;
            nutritionalIndex = 65;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {150, 435, 110, 65, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {150, 435, 99, 65, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {150, 435, 110, 65, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {38, 109, 17, 13, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {60, 174, 44, 13, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_FishPotatoPlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FISHPOTATOPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_FISHPOTATOPLATE_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 470;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 150;
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
    class ChefZ_BeanSausagePlateBulk : ChefZ_PortionedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BEANSAUSAGEPLATE_BULK";
        descriptionShort = "#STR_CHEFZ_ITEM_BEANSAUSAGEPLATE_BULK_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";   // PROXY, kein eigenes Mesh
        weight = 940;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 300;
            energy = 1220;
            water = 235;
            nutritionalIndex = 60;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {300, 1220, 235, 60, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {300, 1220, 212, 60, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {300, 1220, 235, 60, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {75, 305, 35, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {120, 488, 94, 12, 20, 16, 1}; };
            };
        };
    };

    class ChefZ_BeanSausagePlate : ChefZ_ServedDish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BEANSAUSAGEPLATE";
        descriptionShort = "#STR_CHEFZ_ITEM_BEANSAUSAGEPLATE_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, kein eigenes Mesh
        weight = 500;
        lifetime = 10800;

        class Nutrition
        {
            fullnessIndex = 300;
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
    // Zwei Sorten Records, mit Absicht unterschiedlich:
    //
    //   ChefZ_DishesBBulk    das, was im Kochgeraet steht. KEIN
    //                        containerCategory: ein Bulk liegt in Topf oder
    //                        Pfanne, nicht auf einem Teller, und es soll beim
    //                        Aufessen nichts zurueckgeben.
    //   ChefZ_DishesBPlate   die servierte Portion. containerCategory sagt,
    //                        WORAUF sie liegt; returnContainer "AUTO" gibt beim
    //                        letzten Bissen genau den Behaelter zurueck, der
    //                        benutzt wurde (16 §4, OF-04: reusable).
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
    class ChefZ_DishesBBulk
    {
        defaultState      = "COOKED";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        decays            = 1;
    };

    class ChefZ_DishesBPlate
    {
        defaultState      = "COOKED";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        containerCategory = "PLATE";
        returnContainer   = "AUTO";
        decays            = 1;
    };

    class ChefZ_TacticalBreakfastBulk : ChefZ_DishesBBulk {};
    class ChefZ_TacticalBreakfast : ChefZ_DishesBPlate {};
    class ChefZ_ScrambledEggSausageBulk : ChefZ_DishesBBulk {};
    class ChefZ_ScrambledEggSausage : ChefZ_DishesBPlate {};
    class ChefZ_FarmersBreakfastBulk : ChefZ_DishesBBulk {};
    class ChefZ_FarmersBreakfast : ChefZ_DishesBPlate {};
    class ChefZ_CheeseFlatbreadBulk : ChefZ_DishesBBulk {};
    class ChefZ_CheeseFlatbread : ChefZ_DishesBPlate {};
    class ChefZ_SausageBreadPlateBulk : ChefZ_DishesBBulk {};
    class ChefZ_SausageBreadPlate : ChefZ_DishesBPlate {};
    class ChefZ_MushroomPanBulk : ChefZ_DishesBBulk {};
    class ChefZ_MushroomPan : ChefZ_DishesBPlate {};
    class ChefZ_PotatoPancakesBulk : ChefZ_DishesBBulk {};
    class ChefZ_PotatoPancakes : ChefZ_DishesBPlate {};
    class ChefZ_MeatDumplingsBulk : ChefZ_DishesBBulk {};
    class ChefZ_MeatDumplings : ChefZ_DishesBPlate {};
    class ChefZ_MilkRiceBulk : ChefZ_DishesBBulk {};
    // §61.19 ist das einzige Gericht dieses Slice in der SCHUESSEL: Milchreis
    // ist ein Brei und kein Teller (Production Map §60 kennt beide Behaelter).
    class ChefZ_MilkRice : ChefZ_DishesBPlate
    {
        containerCategory = "BOWL";
    };
    class ChefZ_HoneyBreadPlateBulk : ChefZ_DishesBBulk {};
    class ChefZ_HoneyBreadPlate : ChefZ_DishesBPlate {};

    //--------------------------------------------------------------------------
    // ### SLICE dishes-c ###   Zutatenbindung der fuenf Bowl-Gerichte
    //
    // Nur die SERVIERTEN Schuesseln stehen hier, nicht die Bulk-Klassen. Der
    // Grund ist 16 §3.2: "Die Rueckgabeklasse steht am Gericht, nicht im
    // Rezept." containerCategory und returnContainer gehoeren also an das
    // Item, das gegessen wird. Das Bulk wird nicht gegessen, es wird
    // portioniert - was dabei entsteht, sagt outputs[].portionClass im Rezept.
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
    class ChefZ_DishesABulk
    {
        defaultState      = "COOKED";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        decays            = 1;
    };

    class ChefZ_DishesAPlate
    {
        defaultState      = "COOKED";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        decays            = 1;
        containerCategory = "PLATE";
        returnContainer   = "AUTO";
    };

    class ChefZ_SurvivorSpaghettiBulk : ChefZ_DishesABulk {};
    class ChefZ_SurvivorSpaghetti : ChefZ_DishesAPlate {};
    class ChefZ_SausagePastaBulk : ChefZ_DishesABulk {};
    class ChefZ_SausagePasta : ChefZ_DishesAPlate {};
    class ChefZ_HunterPastaBulk : ChefZ_DishesABulk {};
    class ChefZ_HunterPasta : ChefZ_DishesAPlate {};
    class ChefZ_CreamMushroomPastaBulk : ChefZ_DishesABulk {};
    class ChefZ_CreamMushroomPasta : ChefZ_DishesAPlate {};
    class ChefZ_MacAndCheeseBulk : ChefZ_DishesABulk {};
    class ChefZ_MacAndCheese : ChefZ_DishesAPlate {};
    class ChefZ_SausagePotatoesBulk : ChefZ_DishesABulk {};
    class ChefZ_SausagePotatoes : ChefZ_DishesAPlate {};
    class ChefZ_HunterPlateBulk : ChefZ_DishesABulk {};
    class ChefZ_HunterPlate : ChefZ_DishesAPlate {};
    class ChefZ_BloodSausagePlateBulk : ChefZ_DishesABulk {};
    class ChefZ_BloodSausagePlate : ChefZ_DishesAPlate {};
    class ChefZ_FishPotatoPlateBulk : ChefZ_DishesABulk {};
    class ChefZ_FishPotatoPlate : ChefZ_DishesAPlate {};
    class ChefZ_BeanSausagePlateBulk : ChefZ_DishesABulk {};
    class ChefZ_BeanSausagePlate : ChefZ_DishesAPlate {};
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
    // handcraftRecipeSlots = 2: dieser Slice bringt GENAU ZWEI Transforms
    // mit, deren Prozess exec = "HANDCRAFT" hat - TR_CarveWoodenPlate und
    // TR_CarveWoodenBowl. Die Zahl ist eine Reservierung in Vanillas
    // Rezeptliste und muss vorab feststehen; die Begruendung steht im Kopf
    // von ChefZ_HandcraftBridge.c.
    //
    // loadOrder 310, also nach den Saucen: Behaelter und Gerichtebasen werden
    // von den Gerichteslices gelesen, nicht umgekehrt. Der Core haengt
    // Records nicht voneinander ab; die Reihenfolge ist Vorsorge.
    class ChefZ_Serving
    {
        chefzApiVersion = 1;
        loadOrder = 310;
        handcraftRecipeSlots = 2;
        dataFiles[] =
        {
            "ChefZ_Cooking/Config/Processing/Tableware.json"
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
    class ChefZ_DishesB
    {
        chefzApiVersion = 1;
        loadOrder = 330;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
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

};
