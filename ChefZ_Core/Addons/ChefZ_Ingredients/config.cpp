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
// KEIN SCHNITTGUT MEHR (Entscheidung vom 29.08.2026): die geschnittenen
// Gemuesestufen (die Chopped-Varianten und die geschnittene Kartoffel) und
// der Schnittprozess sind entfernt. Jedes Rezept nimmt das ganze Gemuese - das war ohnehin
// ueberall als Alternative zugelassen. Das ganze Gemuese (Fundpflanze) liegt
// in ChefZ_Farming; hier bleiben Milchprodukte, Salz, getrocknete Kraeuter,
// Gewuerze und die Beeren.
//
// Andockregel des Core (Kopf von ChefZ_Edible_Base.c): die CONFIGklasse erbt
// von einer VANILLA-Klasse, die SKRIPTklasse von ChefZ_Edible_Base. Der Core
// bringt bewusst keinen CfgVehicles-Eintrag mit (Invariante I3).
//
// MODELLE: saemtliche model=-Pfade sind VANILLA-PROXIES. Kein Item dieses
// Slices hat eigene Geometrie; der Bedarf steht im Slice-Bericht.
//==============================================================================

//==============================================================================
// fullnessIndex - DIE HERLEITUNG. Gilt fuer JEDEN Nutrition- und
// nutrition_properties-Block dieser Datei (31.08.2026).
//
// DIE ENGINEZEILE, um die es geht (PlayerStomach.c:86, StomachItem.
// ProcessDigestion, scripts 1.29):
//
//     volume = m_Profile.GetFullnessIndex() * m_Amount;
//
// KEIN Nenner. Kein "/ 100". Das Magenvolumen ist das PRODUKT aus
// fullnessIndex und der Menge, die im Magen liegt - und m_Amount ist die
// gegessene varQuantity, nicht ein Prozentwert. Frueher stand in diesem Modul
// (und in den Nachbarmodulen) die Begruendung, PlayerStomach teile bei :92 durch
// 100. Das ist FALSCH und war die Ursache des In-Game-Befunds "Erbrechen nach
// jedem Essen": PlayerStomach.c:92ff (GetNutritions) teilt energy und water
// durch 100 - ENERGIE und WASSER, nicht das Volumen. Wer diesen Nenner auf das
// Volumen uebertraegt, tippt Werte, die hundertfach zu gross sind.
//
// DIE SCHWELLEN (PlayerConstants.c:208 bzw. :200):
//     VOMIT_THRESHOLD        = 2000   -> darueber wird erbrochen
//     BT_STOMACH_VOLUME_LVL3 = 1000   -> Badge "Stuffed"
//
// EIN BISS (ActionConstants.c:8-10, UAQuantityConsumed):
//     EAT_SMALL 10, EAT_NORMAL 15, EAT_BIG 25 Einheiten varQuantity.
// Ein Item mit varQuantityMax = 1 (Stueckware) geht mit EINEM Biss ganz in den
// Magen; m_Amount ist dann 1.
//
// DIE INVARIANTE, nach der hier jeder Wert gesetzt ist:
//
//     fullnessIndex * varQuantityMax = Volumen des GANZEN Items
//
// und dieses Volumen liegt in folgenden Baendern:
//     rohe/essbare Zutaten            50 - 250   (Ei, getrocknete Beeren)
//     saettigende Zwischenprodukte   200 - 400   (Sahne, Butter, Kaese)
//     Gewuerze, Salz, Kleinstmengen  praktisch 0 (fullnessIndex <= 0.1)
//                                    - niemand isst Salz als Mahlzeit.
//
// Zum Vergleich: Vanilla haelt fullnessIndex im Band 0.75 - 2.5 bei
// varQuantityMax 100, also 75 - 250 Volumen je ganzem Item. Die
// Zwischenprodukte dieses Moduls liegen bewusst darueber (bis 3.5): ein ganzer
// Kaeselaib SOLL mehr fuellen als ein Apfel. Er bleibt mit 350 aber weit unter
// "Stuffed" (1000) - man kann zwei davon essen, drei sind eine schlechte Idee.
//
// Was VORHER hier stand, zum Nachrechnen: ChefZ_Cheese fuehrte fullnessIndex 35
// bei varQuantityMax 100. Ein ganzer Laib waren 3500 Volumen - das 1,75-fache
// der Kotzschwelle, in einem Zug. Genau das hat der In-Game-Test gemeldet.
//
// DIE STUFENWERTE ZAEHLEN, NICHT class Nutrition. Sobald eine Klasse einen
// Food-Block traegt, schlagen die nutrition_properties[0] den fullnessIndex aus
// class Nutrition (Edible_Base.c:394-503). Beide sind deshalb ueberall
// mitskaliert, und die Verhaeltnisse ZWISCHEN den Stufen (gekocht quillt,
// verbrannt schrumpft) sind unveraendert erhalten.
//==============================================================================

class CfgPatches
{
    class ChefZ_Ingredients
    {
        units[] = {
            // ### SLICE dairy ###
            "ChefZ_Cream", "ChefZ_Butter", "ChefZ_Cheese", "ChefZ_Egg",
            "ChefZ_MilkCan",
            // Kaesekette (Todo 10, 31.08.2026): Bruch und Saeuerungsmittel.
            // ChefZ_MushroomCulture steht in der Datei WEITER UNTEN zwischen den
            // Gewuerzen - es erbt von ChefZ_SpiceBase und muss deshalb hinter
            // dessen Rumpf stehen. Es gehoert trotzdem hierher: der Slice ist
            // dairy, nicht herbs.
            "ChefZ_CheeseCurd", "ChefZ_MushroomCulture",
            // ### SLICE salt ###
            "ChefZ_RawSalt",
            "ChefZ_Salt",
            // ### SLICE herbs ###
            "ChefZ_DriedHerbBase",
            "ChefZ_DriedParsley", "ChefZ_DriedThyme",
            "ChefZ_DriedRosemary", "ChefZ_DriedWildGarlic", "ChefZ_DriedPaprika",
            "ChefZ_SpiceBase",
            "ChefZ_PaprikaPowder", "ChefZ_DriedPeppercorns", "ChefZ_BlackPepper",
            "ChefZ_HerbMix", "ChefZ_HunterSeasoning",
            // ### SLICE vanilla-foods ###
            "ChefZ_DriedBerries"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // DZ_Data       - Edible_Base, die Vanilla-Configbasis dieser Items
        // DZ_Gear_Food  - die Proxy-Modelle unter \dz\gear\food\
        // ChefZ_Core    - ChefZ_Edible_Base und die Auswertung der CfgChefZ*-Knoten
        // ChefZ_Farming - die Frischkraeuter als Eingang der Trocknungs-Transforms
        // ChefZ_Processing: die Station ChefZ_DryingRack, an der die beiden
        // Beeren-Transforms des Slice vanilla-foods laufen.
        //   ### SLICE vanilla-foods ### GENAU ZWEI weitere Eintraege:
        //   ChefZ_Meat        die Kategorie MEAT und ihre Geschwister stammen
        //                     aus ChefZ_Meat/Config/Ingredients/Meat.json; die
        //                     Zutatenbindung dieses Slice steht ausdruecklich
        //                     NEBEN ihr (Wurzelkategorie CANNED_MEAT) und muss
        //                     deshalb wissen, dass sie existiert.
        //   ChefZ_Preservation die Kategorie FISH existiert nur, weil jenes
        //                     Modul sie deklariert - Sardines und Bitterlings
        //                     treten dort ein.
        // Keine Zirkularitaet: weder ChefZ_Meat noch ChefZ_Preservation nennt
        // ChefZ_Ingredients in seinem requiredAddons[].
        requiredAddons[] = {"DZ_Data", "DZ_Gear_Food", "ChefZ_Core", "ChefZ_Farming", "ChefZ_Processing", "DZ_Gear_Consumables", "ChefZ_Meat", "ChefZ_Preservation", "ChefZ_Items", "ChefZ_Food"};
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
    class Inventory_Base;
    class GardenLime;   // ### SLICE salt ###

    // ### SLICE dairy ### Proxy-Basen
    //
    // "class Honey;" stand hier fuer den Sahne-Proxy und ist ERSATZLOS
    // gestrichen. Grund: eine Vorwaertsdeklaration ist bereits eine
    // Klassendefinition in CfgVehicles. ChefZ hat damit eine VANILLA-Klasse in
    // den eigenen Klassenbaum gezogen, die es weder erweitert noch erweitern
    // darf - und die zugleich als Zutat in zwei Rezepten steht
    // (RCP_ChefZ_MilkRice, RCP_ChefZ_HoneyBreadPlate).
    //
    // Vanillas Honey braucht hier nichts: es gibt keine Honey.c,
    // Edible_Base.CanBeCooked() liefert false (Edible_Base.c:129),
    // Cooking.ProcessItemToCook laesst das Glas also unangetastet liegen
    // (Cooking.c:47), und RCP_ChefZ_HoneyBreadPlate gart ohnehin nicht
    // (completion INSTANT). Ein Food-Block auf einer Vanilla-Klasse waere kein
    // Zusatz, sondern ein Umbau: FoodStage-Werte schlagen class Nutrition
    // (Edible_Base.c:394-503), und Vanillas Honig-Naehrwerte stehen in
    // Spieldaten, die dieses Projekt nicht liest.
    class Lard;
    class BoxCerealCrunchin;
    class Marmalade;

    class Edible_Base;

    //==========================================================================
    // ### SLICE vanilla-foods ### Eine neue Klasse, mehr nicht
    //
    // Quelle: Vanilla-Audit §3 C (Zucchini) und §3 F (Beeren an der
    // vorhandenen Trockenkette).
    //
    // Seit dem 29.08.2026 nur noch EINE Klasse: die geschnittene Zucchini ist
    // mit dem uebrigen Schnittgut entfallen (Kopf dieser Datei). Der Grund
    // fuer die Bauform von ChefZ_DriedBerries steht in ihrem eigenen Kommentar.
    //==========================================================================

    //--------------------------------------------------------------------------
    // Getrocknete Beeren - Hagebutte und Holunder vom Trockenrahmen.
    //
    // WARUM SIE EINE EIGENE BAUFORM BRAUCHT und nicht bei den Kraeutern liegt:
    // getrocknete Kraeuter sind Wuerze und kommen nie in einen Pflicht-Slot;
    // getrocknete Beeren sind die PFLICHTzutat des Obstkompotts (Slice
    // dishes-vanilla, RCP_ChefZ_FruitCompote). Damit gilt fuer sie
    // ChefZ_RecipeEvaluator.CheckStages: jede gebundene Pflichtzutat muss eine
    // erlaubte Endstufe erreichen. Eine Klasse ohne FoodStage meldet Stufe 0
    // (NONE), und das Gericht wuerde nie fertig.
    //
    // DER UEBERGANG AUS "Dried" IST DER KERN DIESER KLASSE.
    // ChefZ_PreservedFood_Base gibt Doerrfleisch bewusst KEINEN Weg aus Dried
    // heraus - Doerrfleisch ist fertig. Getrocknetes Obst ist das nicht: es
    // wird im Kompott aufgekocht und zieht dabei Wasser. Genau dieser eine
    // Uebergang Dried -> Boiled steht deshalb hier, und ohne ihn faellt
    // FoodStage.GetNextFoodStageType auf BURNED zurueck (FoodStage.c:472) -
    // die Beeren verkohlten im Topf und das Kompott wuerde nie fertig.
    //
    // Der Uebergang aus "Raw" steht daneben, weil eine Klasse ueber Admin- oder
    // Lootspawn auch ohne den Transform entstehen kann und dann RAW ist.
    //
    // PROXY: Marmalade (Schraubglas) - getrocknete Beeren im Glas. Dieselbe
    // Wahl und dieselbe Begruendung wie beim Slice dairy: eine GEERBTE
    // Vanillaklasse statt eines von Hand getippten p3d-Pfades, weil ein
    // geratener Pfad erst beim Packen auffaellt und eine geerbte Klasse nie.
    // Eigene Geometrie steht im Slice-Bericht; kein Item wartet darauf.
    //--------------------------------------------------------------------------
    class ChefZ_DriedBerries : Marmalade
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRIEDBERRIES";
        descriptionShort = "#STR_CHEFZ_ITEM_DRIEDBERRIES_DESC";
        rotationFlags = 17;
        itemSize[] = {1, 1};
        weight = 40;
        absorbency = 0.0;
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        varQuantityDestroyOnMin = 1;
        canBeSplit = 0;
        isMeleeWeapon = 0;
        soundImpactType = "food";
        lifetime = 43200;

        // fullnessIndex 60: STUECKWARE, varQuantityMax = 1. Ein Biss (EAT_BIG
        // 25) nimmt die ganze Menge 1, also ist das Volumen des ganzen Glases
        //     60 * 1 = 60
        // - unteres Ende des Bandes 50-250 fuer essbare Zutaten. Ein Glas
        // getrockneter Beeren ist eine Handvoll Trockenobst, kein Essen.
        // 60 von 2000 (VOMIT_THRESHOLD) sind 3 Prozent.
        //
        // DER WERT STEIGT (vorher 12 -> 60), er faellt nicht: bei
        // varQuantityMax = 1 war das alte Volumen ebenfalls 12 und damit
        // praktisch nicht vorhanden. Nur die Klassen mit varQuantityMax = 100
        // waren zu hoch. Das ist der Unterschied zwischen "die Invariante
        // anwenden" und "alles durch hundert teilen".
        class Nutrition
        {
            fullnessIndex = 60;
            energy = 130;
            water = 4;
            nutritionalIndex = 45;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        // nutrition_properties[0] ist der fullnessIndex und schlaegt den Wert
        // aus class Nutrition (Edible_Base.c:394-503) - er ist hier ueberall
        // mitgezogen. Die Verhaeltnisse der Stufen bleiben:
        //   Raw/Baked/Dried 12 -> 60   (Faktor 5, wie class Nutrition)
        //   Boiled          18 -> 90   (im Kompott aufgekocht, zieht Wasser)
        //   Burned/Rotten    3 -> 15   (ein Viertel, wie vorher)
        class Food
        {
            class FoodStages
            {
                class Raw
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {60, 130, 4, 45, 0, 0, 1};
                };
                class Baked
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {100, 40, 200};
                    nutrition_properties[] = {60, 130, 2, 40, 0, 0, 1};
                };
                class Boiled
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {100, 60, 150};
                    nutrition_properties[] = {90, 125, 55, 42, 0, 0, 1};
                };
                class Dried
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {60, 130, 4, 45, 0, 0, 1};
                };
                class Burned
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {200, 20, 0};
                    nutrition_properties[] = {15, 20, 0, 0, 0, 0, 1};
                };
                class Rotten
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {15, 20, 2, 0, 20, 16, 1};
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
                    class Boiling
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                    class Baking
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                };
                class Dried
                {
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
    // KEIN Food-Block bei Milch und Sahne: sie liegen in keinem Pflicht-Slot
    // eines Kochgeraets. Ohne FoodStages ist HasFoodStage() falsch,
    // CanBeCooked() liefert false und Cooking.ProcessItemToCook laesst das
    // Item unangetastet liegen (Cooking.c:47).
    //
    // KAESE UND BUTTER TRAGEN EINEN FOOD-BLOCK, und zwar aus zwei Gruenden:
    //   1. RCP_ChefZ_CheeseFlatbread verlangt ChefZ_Cheese, RCP_ChefZ_
    //      MushroomPan verlangt ChefZ_Butter - beide in einem PFLICHT-Slot,
    //      beide in einer Pfanne. Ohne Uebergaenge waere das die Falle aus
    //      01 V4 (FoodStage.c:472 faellt auf BURNED zurueck).
    //   2. ChefZ_RecipeEvaluator.CheckStages verlangt von JEDER gebundenen
    //      Pflichtzutat eine erlaubte Endstufe. Eine Klasse ohne FoodStage
    //      meldet Stufe 0 (NONE) - das Gericht wuerde nie fertig.
    // Stufen OHNE Uebergaenge waeren die Falle; Stufen MIT Uebergaengen sind
    // die Loesung. Die Stufen-Naehrwerte stehen an der Klasse, weil sie
    // class Nutrition schlagen (Edible_Base.c:394-503).
    //
    // ZUSTAENDE: dieser Slice vergibt bewusst keinen ChefZ-Zustand. Die
    // Milchkette ist ein reiner Klassentausch (06 §2); der Zustandsraum gehoert
    // der Konservierungskette.
    //==========================================================================

    // §47: Milch. KEINE eigene Klasse mehr (29.08.2026): Vanillas PowderedMilk
    // IST die Milch des Mods. Beide waren reiner Loot, beide hingen an
    // demselben Karton - eine zweite Klasse war ein Nachbau. Die
    // Zutatenbindung steht in Config/Ingredients/Dairy.json (fremde Klasse ->
    // JSON, 05 §2); Butterfass und Kaesepresse nehmen den Karton direkt.

    // §48: Sahne. Entsteht am Butterfass aus Milch.
    //
    // PROXY: Marmalade (Glas). Vorher Honey - der Tausch ist die Folge der
    // gestrichenen Vorwaertsdeklaration oben und kostet nichts: beide sind
    // Vanilla-Glaeser ohne eigenes Skript. Er raeumt zugleich den Befund aus
    // dem Asset-Backlog ab, dass Sahne im Inventar wie Vanilla-Honig aussah.
    // Neue Kollision dafuer: Sahne und Ei teilen sich jetzt das
    // Marmeladenglas - gemeldet im Asset-Bedarf, aufgeloest mit dem eigenen
    // Sahnegefaess.
    class ChefZ_Cream : Marmalade
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

        // fullnessIndex 2.5 (vorher 20): varQuantityMax = 100, also
        //     2.5 * 100 = 250 Volumen fuer das ganze Glas
        // - unteres Ende des Bandes 200-400 fuer saettigende Zwischenprodukte,
        // und zugleich das obere Ende von Vanillas eigenem Band (0.75-2.5).
        // Sahne ist fluessig und saettigt weniger als der Kaeselaib (3.5), aber
        // mehr als Butter (2.0) - dieselbe Reihenfolge wie vorher (20 > 15).
        //
        // Vorher: 20 * 100 = 2000 Volumen. Das war GENAU der VOMIT_THRESHOLD.
        // Ein ausgetrunkenes Glas Sahne loeste zuverlaessig Erbrechen aus.
        class Nutrition
        {
            fullnessIndex = 2.5;
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

        // fullnessIndex 2.0 (vorher 15): varQuantityMax = 100, also
        //     2.0 * 100 = 200 Volumen fuer das ganze Stueck
        // - Untergrenze des Bandes 200-400 fuer saettigende Zwischenprodukte.
        // Butter ist ein Fettblock: viel Energie (600) auf wenig Volumen. Sie
        // bleibt unter Sahne (2.5) und Kaese (3.5), wie vorher (15 < 20 < 35).
        //
        // Vorher: 15 * 100 = 1500 Volumen - drei Viertel der Kotzschwelle und
        // anderthalbmal "Stuffed" (1000), fuer ein Stueck Butter.
        class Nutrition
        {
            fullnessIndex = 2.0;
            energy = 600;
            water = 20;
            nutritionalIndex = 8;
            toxicity = 0;
            digestibility = 1;
        };

        // Butter ist seit der I2-Nachbesserung PFLICHT-Zutat in
        // RCP_ChefZ_MushroomPan - sie ist es, die aus gebratenen Pilzen eine
        // Pilzpfanne macht (Production Map §16). Damit liegt sie in einem
        // Kochgeraet und braucht Stufen und Uebergaenge.
        //
        // Der eigene Block hat einen zweiten Nutzen: die Configklasse erbt von
        // Lard, und Lard ist in Vanilla ein Bratfett MIT Garstufen. Ohne
        // eigenen Block erbte ChefZ_Butter Lards Stufen-NAEHRWERTE, und die
        // Werte in class Nutrition oben waeren wirkungslos
        // (Edible_Base.c:394-503). Hier stehen sie jetzt ausdruecklich.
        //
        // visual_properties fehlen absichtlich: das Modell kommt von Lard, und
        // seine Selection-Indizes kennt nur Lards eigener Block.
        //
        // Die Skriptklasse ist Lard (dieser Slice bringt keine eigene mit),
        // und Lard.c ueberschreibt CanBeCooked() mit true - Butter ist damit
        // die eine ChefZ-Zutat, die ihre Endstufe heute schon erreicht.
        //
        // nutrition_properties[0] ist der fullnessIndex, er schlaegt class
        // Nutrition und ist mitskaliert (Faktor 2.0/15 = 0.1333):
        //   Raw          15 -> 2.0   (200 Volumen fuer das ganze Stueck)
        //   Baked/Boiled 14 -> 1.9   (zerlassene Butter, 190)
        //   Burned        4 -> 0.5   (verbrannt, 50 - ein Viertel, wie vorher)
        //   Rotten        4 -> 0.5
        class Food
        {
            class FoodStages
            {
                class Raw
                {
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {2.0, 600, 20, 8, 0, 0, 1};
                };
                class Baked
                {
                    cooking_properties[] = {80, 30, 200};
                    nutrition_properties[] = {1.9, 620, 8, 8, 0, 0, 1};
                };
                class Boiled
                {
                    cooking_properties[] = {80, 40, 150};
                    nutrition_properties[] = {1.9, 610, 15, 8, 0, 0, 1};
                };
                class Burned
                {
                    cooking_properties[] = {200, 20, 0};
                    nutrition_properties[] = {0.5, 90, 0, 0, 0, 0, 1};
                };
                class Rotten
                {
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {0.5, 90, 4, 0, 15, 0, 1};
                };
            };

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

    // §50: Kaese. V1 kennt GENAU EINE Kaeseklasse; mehrere Sorten sind
    // ausdruecklich V2. Das haltbarste Milchprodukt.
    // PROXY: BoxCerealCrunchin (Schachtel). Ziel: eigener Kaeselaib.
    class ChefZ_Cheese : BoxCerealCrunchin
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHEESE";
        descriptionShort = "#STR_CHEFZ_ITEM_CHEESE_DESC";
        model = "\ChefZ\ChefZ_Food\models\cheese.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        weight = 220;
        itemSize[] = {2, 2};
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;
        lifetime = 86400;

        // fullnessIndex 3.5 (vorher 35): varQuantityMax = 100, also
        //     3.5 * 100 = 350 Volumen fuer den ganzen Laib
        // - oberes Drittel des Bandes 200-400 fuer saettigende
        // Zwischenprodukte. Kaese ist das saettigendste Item dieses Moduls und
        // bleibt es (35 war schon der Hoechstwert). 350 sind gut ein Drittel
        // von "Stuffed" (1000): zwei Laibe gehen, drei sind zu viel.
        //
        // DIESE KLASSE WAR DER BEFUND. Vorher: 35 * 100 = 3500 Volumen - das
        // 1,75-fache von VOMIT_THRESHOLD (2000). Ein ganzer Kaeselaib fuehrte
        // in einem Zug zum Erbrechen, noch bevor die erste Kalorie ankam.
        class Nutrition
        {
            fullnessIndex = 3.5;
            energy = 450;
            water = 60;
            nutritionalIndex = 25;
            toxicity = 0;
            digestibility = 1;
        };

        // Kaese schmilzt in der Pfanne (Baked) und im Topf (Boiled) - beide
        // Uebergaenge stehen hier, weil RCP_ChefZ_CheeseFlatbread beide
        // Geraete zulaesst und ein Topf mit Wasser nach BOILING gart. Aus
        // Baked und Boiled gibt es keinen Ausgang: wer den Kaese im Feuer
        // vergisst, bekommt Kohle (FoodStage.c:472). Das ist gewollt.
        //   FoodStageType:     RAW 1, BAKED 2, BOILED 3, DRIED 4, BURNED 5, ROTTEN 6
        //   CookingMethodType: NONE 0, BAKING 1, BOILING 2, DRYING 3, TIME 4
        //
        // nutrition_properties[0] ist der fullnessIndex, er schlaegt class
        // Nutrition und ist mitskaliert (Faktor 3.5/35 = 0.1):
        //   Raw     35 -> 3.5   (350 Volumen fuer den ganzen Laib)
        //   Baked   32 -> 3.2   (geschmolzen, 320)
        //   Boiled  33 -> 3.3   (330)
        //   Burned   9 -> 0.9   (Kohle, 90)
        //   Rotten   9 -> 0.9
        class Food
        {
            class FoodStages
            {
                class Raw
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {3.5, 450, 60, 25, 0, 0, 1};
                };
                class Baked
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {90, 45, 200};
                    nutrition_properties[] = {3.2, 470, 35, 26, 0, 0, 1};
                };
                class Boiled
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {90, 60, 150};
                    nutrition_properties[] = {3.3, 455, 70, 22, 0, 0, 1};
                };
                class Burned
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {200, 20, 0};
                    nutrition_properties[] = {0.9, 68, 0, 0, 0, 0, 1};
                };
                class Rotten
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {0.9, 68, 12, 0, 15, 0, 1};
                };
            };

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

    //--------------------------------------------------------------------------
    // Kaesebruch - die Zwischenstufe der Kaesekette (Todo 10, 31.08.2026).
    //
    // Milch + ChefZ_MushroomCulture -> Kaesebruch -> (Kaesepresse) -> Kaese.
    // Diesen Slice liefert nur die KLASSEN; die beiden Transforms und die
    // Rezeptzeilen bauen die Processing- und Cooking-Agenten nach uns.
    //
    // ESSBAR, ABER UNATTRAKTIV - und das steht ausdruecklich in den Zahlen und
    // nicht nur in der Beschreibung. Gegen den fertigen Laib (energy 450,
    // nutritionalIndex 25, Volumen 350) hat der Bruch mehr Volumen JE KALORIE:
    // 280 Volumen fuer 200 Energie. Wer ihn isst, statt ihn zu pressen, ist
    // satt und hat nichts davon - genau der Anreiz, den die Kette braucht. Ein
    // Verbot waere die schlechtere Loesung: nasser Quark IST essbar, und ein
    // Hungernder soll ihn essen duerfen.
    //
    // FOOD-BLOCK MIT UEBERGAENGEN, analog ChefZ_Cheese: der Bruch landet in
    // einem Pflicht-Slot, sobald das Pressrezept steht, und
    // ChefZ_RecipeEvaluator.CheckStages verlangt von jeder gebundenen
    // Pflichtzutat eine erlaubte Endstufe. Eine Klasse ohne FoodStage meldet
    // Stufe 0 (NONE). Stufen OHNE Uebergaenge waeren die Falle aus 01 V4
    // (FoodStage.c:472 faellt auf BURNED zurueck) - deshalb beides.
    //
    // lifetime 10800 (3 h): der kuerzeste Wert des ganzen Slice, kuerzer als
    // Sahne (14400). Ungepresster Bruch ist die verderblichste Stufe der Kette;
    // das ist der Grund, ihn zu pressen, und nicht nur Geschmack.
    //
    // PROXY: Marmalade (Schraubglas) - ein Becher weisser Bruch. Dieselbe Wahl
    // und Begruendung wie bei Sahne und Ei: eine GEERBTE Vanillaklasse statt
    // eines von Hand getippten p3d-Pfades. Damit teilen sich jetzt DREI Items
    // des Moduls das Marmeladenglas (Sahne, Ei, Bruch) - gemeldet im
    // Asset-Bedarf, aufgeloest mit einem eigenen Quarkbecher. Kein Item wartet
    // darauf. NICHT cheese.p3d aus der Lieferung c09900f: Bruch und fertiger
    // Laib saehen dann identisch aus, und die Kette waere im Inventar nicht
    // mehr lesbar.
    //--------------------------------------------------------------------------
    class ChefZ_CheeseCurd : Marmalade
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHEESECURD";
        descriptionShort = "#STR_CHEFZ_ITEM_CHEESECURD_DESC";
        weight = 240;
        itemSize[] = {2, 2};
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;
        lifetime = 10800;

        // fullnessIndex 2.8: varQuantityMax = 100 wie bei Sahne, Butter und
        // Kaese, also
        //     2.8 * 100 = 280 Volumen fuer den ganzen Becher
        // - Mitte des Bandes 200-300 fuer saettigende Zwischenprodukte. Der
        // Bruch liegt damit ueber Butter (200) und Sahne (250) und unter dem
        // fertigen Laib (350): nasser, ungepresster Quark ist voluminoeser als
        // das, was nach dem Pressen davon uebrig bleibt, aber ein Becher davon
        // ist kein ganzer Laib. Herleitung im Dateikopf; die Enginezeile ist
        // PlayerStomach.c:86 (volume = fullnessIndex * m_Amount, KEIN "/100"),
        // die Schwellen sind VOMIT_THRESHOLD 2000 und "Stuffed" 1000
        // (PlayerConstants.c:208 / :200). 280 sind gut ein Viertel von
        // "Stuffed" - drei Becher gehen, vier sind eine schlechte Idee.
        class Nutrition
        {
            fullnessIndex = 2.8;
            energy = 200;
            water = 120;
            nutritionalIndex = 14;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        // Stufen wie bei ChefZ_Cheese, auf DERSELBEN Skala: Element 0 ist der
        // fullnessIndex und schlaegt class Nutrition (Edible_Base.c:394-503).
        //   Raw     2.8   (280 - der Becher, wie er aus der Molke kommt)
        //   Baked   2.6   (260 - trocknet in der Pfanne aus)
        //   Boiled  2.7   (270)
        //   Burned  0.7   (70 - ein Viertel, dasselbe Verhaeltnis wie Kaese)
        //   Rotten  0.7   (70)
        // Aus Baked und Boiled gibt es keinen Ausgang: wer den Bruch im Feuer
        // vergisst, bekommt Kohle (FoodStage.c:472). Das ist gewollt.
        //   FoodStageType:     RAW 1, BAKED 2, BOILED 3, DRIED 4, BURNED 5, ROTTEN 6
        //   CookingMethodType: NONE 0, BAKING 1, BOILING 2, DRYING 3, TIME 4
        class Food
        {
            class FoodStages
            {
                class Raw
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {2.8, 200, 120, 14, 0, 0, 1};
                };
                class Baked
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {90, 45, 200};
                    nutrition_properties[] = {2.6, 215, 70, 15, 0, 0, 1};
                };
                class Boiled
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {90, 60, 150};
                    nutrition_properties[] = {2.7, 205, 135, 13, 0, 0, 1};
                };
                class Burned
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {200, 20, 0};
                    nutrition_properties[] = {0.7, 30, 0, 0, 0, 0, 1};
                };
                class Rotten
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {0.7, 30, 20, 0, 15, 0, 1};
                };
            };

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

    //--------------------------------------------------------------------------
    // Die Milchkanne (Lieferung c09900f). Traggut der Milchkette; noch ohne
    // eigenen Prozess - Milch ist in V1 Vanillas PowderedMilk
    // (Config/Ingredients/Dairy.json). Die Kanne existiert, damit das
    // gelieferte Modell im Spiel ist und ein spaeterer Melk-Slice sie fuellt.
    //--------------------------------------------------------------------------
    class ChefZ_MilkCan : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MILKCAN";
        descriptionShort = "#STR_CHEFZ_ITEM_MILKCAN_DESC";
        model = "\ChefZ\ChefZ_Items\models\milkcan.p3d";   // EIGENES MODELL (30.08.2026)
        rotationFlags = 2;
        itemSize[] = {3, 2};
        weight = 1800;
        lifetime = 43200;
        repairableWithKits[] = {};
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

        // fullnessIndex 1.0 (vorher 12): varQuantityMax = 100, also
        //     1.0 * 100 = 100 Volumen fuer das ganze Ei
        // - Mitte des Bandes 50-250 fuer essbare Zutaten und mitten in Vanillas
        // eigenem Band (0.75-2.5). Ein Ei ist eine Zutat, keine Mahlzeit: zwanzig
        // davon fuellen den Magen (2000), zehn machen "Stuffed" (1000). Es bleibt
        // unter Butter (2.0), wie vorher (12 < 15).
        //
        // Vorher: 12 * 100 = 1200 Volumen - ein einziges rohes Ei ueberschritt
        // "Stuffed", zwei loesten Erbrechen aus.
        class Nutrition
        {
            fullnessIndex = 1.0;
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
        //
        // Element 0 ist der fullnessIndex, er schlaegt class Nutrition und ist
        // mitskaliert (Faktor 1.0/12 = 0.0833):
        //   Raw           12 -> 1.0   (100 Volumen fuer das ganze Ei)
        //   Baked/Boiled  14 -> 1.2   (gestockt, 120)
        //   Burned         6 -> 0.5   (verkohlt, 50 - die Haelfte, wie vorher)
        //   Rotten         6 -> 0.5
        class Food
        {
            class FoodStages
            {
                class Raw
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {1.0, 90, 40, 12, 0, 0, 1};
                };
                class Baked
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {1.2, 105, 25, 18, 0, 0, 1};
                };
                class Boiled
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {1.2, 100, 35, 18, 0, 0, 1};
                };
                class Burned
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {0.5, 20, 5, 0, 0, 0, 1};
                };
                class Rotten
                {
                    visual_properties[] = {0.0, 0.0, 0.0};
                    nutrition_properties[] = {0.5, 20, 10, 0, 15, 0, 1};
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
    //
    // fullnessIndex: KEINER, und das ist die staerkste Form der Regel aus dem
    // Dateikopf ("Salz: praktisch 0"). Ohne class Nutrition gibt es keinen
    // NutritionalProfile, PlayerStomach.InitData registriert die Klasse nicht,
    // und PlayerStomach.c:86 wird fuer sie nie ausgefuehrt. Volumen 0, nicht
    // "nahe 0". Beide Salzklassen brauchten deshalb beim Rescale vom
    // 31.08.2026 keine Aenderung.
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
    // ChefZ_PepperBerries) stehen in ChefZ_Farming, die frische Paprika ist
    // Vanillas GreenBellPepper; die
    // Stationen und Transforms in ChefZ_Processing.
    //
    // class Nutrition ist PFLICHT: PlayerStomach.InitData registriert nur
    // Klassen mit Nutrition ODER Food und scope != 0 (01 V7). Getrocknete
    // Kraeuter saettigen fast nichts - aber "fast nichts" und "lautlos nichts"
    // sind zwei verschiedene Dinge.
    //
    // OHNE class Food / FoodStages - MIT EINER AUSNAHME.
    //
    // Die Regel gilt weiter: ein Gewuerz geht als Zutat in ein Gericht ein und
    // liegt nie selbst im Topf; Stufen ohne Uebergaenge waeren die Falle aus
    // 01 V4 (FoodStage.c:472 verbrennt das Item). Genau EIN Gewuerz haelt sich
    // nicht daran, und das ist keine Meinung, sondern eine Rezeptzeile:
    // RCP_ChefZ_ChernarusChili fuehrt ChefZ_PaprikaPowder in einem
    // PFLICHT-Slot. Es liegt damit im Topf, waehrend Vanilla den Garzustand
    // fortschreibt, und ChefZ_RecipeEvaluator.CheckStages verlangt von jeder
    // Pflichtzutat eine erlaubte Endstufe - eine Klasse ohne FoodStage meldet
    // Stufe 0 (NONE), und das Chili wuerde nie fertig. Deshalb traegt
    // ChefZ_PaprikaPowder als einzige Klasse dieses Abschnitts Stufen UND
    // Uebergaenge. Alle anderen Gewuerze stehen ausschliesslich in optionalen
    // Slots; fuer sie bleibt die Regel unveraendert.
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

        // fullnessIndex 0.1 (vorher 2): varQuantityMax = 1 (Stueckware, ein
        // Biss nimmt alles), also
        //     0.1 * 1 = 0.1 Volumen fuer das ganze Buendel
        // - das ist die Kategorie "praktisch 0" aus dem Dateikopf. Ein
        // geloeffeltes Gewuerz IST keine Mahlzeit, und der Magen soll das nicht
        // anders sehen: 20000 Buendel Thymian ergaeben eine Kotzschwelle. Der
        // Wert bleibt ueber 0, damit die Klasse einen NutritionalProfile
        // behaelt (PlayerStomach.InitData, 01 V7) - "fast nichts" und "lautlos
        // nichts" sind zwei verschiedene Dinge.
        //
        // Vorher waren es 2 Volumen. Nicht dramatisch, aber falsch begruendet
        // und in derselben verschobenen Einheit wie der Rest der Datei.
        // Vier Klassen erben diesen Block unveraendert.
        class Nutrition
        {
            fullnessIndex = 0.1;
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

        // fullnessIndex 0.1 (vorher 2): varQuantityMax = 1, also
        //     0.1 * 1 = 0.1 Volumen fuer die ganze Tuete
        // - dieselbe Herleitung wie bei ChefZ_DriedHerbBase daneben. Pfeffer,
        // Paprikapulver, Kraeutermix und Jaegergewuerz saettigen nicht; sie
        // behalten nur so viel Profil, dass PlayerStomach sie ueberhaupt kennt.
        // Vier Klassen erben diesen Block; ChefZ_PaprikaPowder ueberschreibt
        // ihn ueber seine FoodStages (siehe dort).
        class Nutrition
        {
            fullnessIndex = 0.1;
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

        // Die Ausnahme aus dem Abschnittskopf: Pflichtzutat in
        // RCP_ChefZ_ChernarusChili und damit ein Garobjekt.
        //
        // Die Naehrwerte aendern sich ueber die Stufen fast nicht - ein
        // geloeffeltes Gewuerz saettigt weder roh noch gekocht. Sie stehen
        // trotzdem an JEDER Stufe: sobald FoodStages existieren, schlagen sie
        // class Nutrition (Edible_Base.c:394-503), und eine Stufe ohne
        // nutrition_properties saettigte lautlos gar nicht mehr. Die Rohwerte
        // sind die geerbten aus ChefZ_SpiceBase.
        //
        // Verbranntes Paprikapulver ist bitter, kein Nahrungsmittel - deshalb
        // gibt es aus Baked und Boiled keinen Ausgang ausser BURNED.
        //   FoodStageType:     RAW 1, BAKED 2, BOILED 3, DRIED 4, BURNED 5, ROTTEN 6
        //   CookingMethodType: NONE 0, BAKING 1, BOILING 2, DRYING 3, TIME 4
        //
        // Element 0 ist der fullnessIndex, mitskaliert wie die geerbte Basis
        // (Faktor 0.1/2 = 0.05), varQuantityMax = 1:
        //   Raw/Baked/Boiled  2 -> 0.1   (0.1 Volumen fuer die ganze Tuete)
        //   Burned/Rotten     1 -> 0.05  (die Haelfte, wie vorher)
        // Ein Loeffel Paprikapulver im Chili darf im Magen nichts wiegen - der
        // Toepferinhalt ist das Gericht, nicht das Gewuerz.
        class Food
        {
            class FoodStages
            {
                class Raw
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {0.1, 10, 0, 10, 0, 0, 1};
                };
                class Baked
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {90, 30, 200};
                    nutrition_properties[] = {0.1, 10, 0, 9, 0, 0, 1};
                };
                class Boiled
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {90, 40, 150};
                    nutrition_properties[] = {0.1, 10, 0, 9, 0, 0, 1};
                };
                class Burned
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {200, 15, 0};
                    nutrition_properties[] = {0.05, 2, 0, 0, 0, 0, 1};
                };
                class Rotten
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                    nutrition_properties[] = {0.05, 2, 0, 0, 15, 0, 1};
                };
            };

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

    //==========================================================================
    // ### SLICE dairy ###   Pilzkultur - das Saeuerungsmittel der Kaesekette
    //
    // WARUM SIE HIER UNTEN STEHT UND NICHT BEI DER MILCH: sie erbt von
    // ChefZ_SpiceBase, und DayZ loest "class X : Basis" nur auf, wenn der
    // Rumpf der Basis in DERSELBEN config.cpp WEITER OBEN steht. Eine
    // Vorwaertsdeklaration waere hier keine Alternative - die Basis liegt in
    // dieser Datei, nicht in einem anderen Addon. Der Slice ist trotzdem
    // dairy: sie taucht in CfgPatches im dairy-Block auf, ihr Datensatz steht
    // in Dairy.json, und ihr Delta ist _deltas/dairy.json.
    //
    // WARUM ChefZ_SpiceBase UND NICHT Edible_Base DIREKT: die Basis ist genau
    // das, was eine Kultur baulich ist - eine kleine Tuete Pulver, essbar und
    // wertlos, ohne Food-Knoten und ohne Garstufen. Sie bringt ausserdem die
    // Skriptklasse mit, die die Essaktion registriert (ChefZ_SpiceBase in
    // ChefZ_SpiceIngredients.c, ActionEatSmall + ActionForceFeedSmall). Vanilla
    // registriert Essaktionen auf jeder Nahrungsklasse einzeln und NICHT auf
    // Edible_Base (Potato.c:26-31); eine eigene Basis haette eine eigene
    // Skriptklasse fuer denselben Zweizeiler gebraucht.
    //
    // KEIN FOOD-BLOCK, und das ist Absicht: die Kultur wird in die Milch
    // GERUEHRT, nicht gegart. Sie liegt in keinem Kochgeraet. Stufen ohne
    // Uebergaenge waeren die Falle aus 01 V4 (FoodStage.c:472 verbrennt das
    // Item), Stufen mit Uebergaengen waeren Ballast fuer einen Vorgang, den es
    // nicht gibt. Der Transform Milch + Kultur -> Bruch ist ein
    // Klassentausch an der Station, kein Garprozess.
    //
    // KATEGORIE SPICE, KEINE NEUE KATEGORIE "CULTURE": eine Kategorie mit genau
    // einem Mitglied traegt nichts, und Kategorien sind Registry-weit
    // (Workflow §5) - sie anzulegen waere eine Entscheidung fuer alle Slices,
    // nicht fuer diesen. SPICE ist im Mod die Sammelstelle fuer "kleine Menge,
    // saettigt nicht, geht als Zutat in etwas anderes ein", und genau das ist
    // eine Kultur. Getrennt bleibt sie ueber den TAG CHEFZ_CULTURE
    // (_deltas/dairy.json), nicht ueber CHEFZ_SPICE: ein Rezept, das per Tag
    // "irgendein Gewuerz" nimmt, soll keine Bakterienkultur als Wuerze
    // akzeptieren. Der Restfall bleibt offen und steht im Slice-Bericht: ein
    // Rezept, das per KATEGORIE SPICE bindet, wuerde sie mitnehmen.
    //
    // PROXY: \dz\gear\food\Rice.p3d - ein kleiner Beutel. NICHT das geerbte
    // PowderedMilk.p3d der Basis: in einer KAESEkette saehe eine Tuete
    // Milchpulver wie die Milch aus, und der Spieler haette zwei verschiedene
    // Dinge mit demselben Bild. Der Reisbeutel kollidiert dafuer mit
    // ChefZ_DriedPeppercorns - gemeldet im Asset-Bedarf, aufgeloest mit dem
    // gemeinsamen Gewuerzbehaelter-Mesh aus Production Map §71 und der Textur
    // "graues Sporenpulver". Kein Item wartet darauf.
    //==========================================================================
    class ChefZ_MushroomCulture : ChefZ_SpiceBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MUSHROOMCULTURE";
        descriptionShort = "#STR_CHEFZ_ITEM_MUSHROOMCULTURE_DESC";
        model = "\dz\gear\food\Rice.p3d";
        weight = 30;

        // fullnessIndex 0.1: varQuantityMax = 1 (Stueckware aus der Basis, ein
        // Biss nimmt die ganze Menge 1), also
        //     0.1 * 1 = 0.1 Volumen fuer die ganze Tuete
        // - das Gewuerzband "praktisch 0" aus dem Dateikopf. Niemand isst eine
        // Kultur als Mahlzeit, und der Magen soll das nicht anders sehen.
        // Herleitung im Dateikopf: PlayerStomach.c:86 multipliziert
        // (volume = fullnessIndex * m_Amount), es teilt nichts durch 100;
        // VOMIT_THRESHOLD ist 2000 (PlayerConstants.c:208).
        //
        // Der Block steht ausgeschrieben da, obwohl er den fullnessIndex der
        // Basis nur wiederholt: energy und nutritionalIndex sind NIEDRIGER als
        // bei einem Gewuerz (5 statt 10, 4 statt 10), und sobald ein Feld
        // abweicht, muessen alle danebenstehen - ein Teil-Block erbt nicht
        // feldweise. Ueber 0 bleibt der Wert, damit die Klasse einen
        // NutritionalProfile behaelt: PlayerStomach.InitData registriert nur
        // Klassen mit Nutrition oder Food (01 V7).
        class Nutrition
        {
            fullnessIndex = 0.1;
            energy = 5;
            water = 0;
            nutritionalIndex = 4;
            toxicity = 0;
            digestibility = 1;
        };
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
// Die Kategorien VEGETABLE, ROOT_VEGETABLE, LEAF_VEGETABLE und der Tag
// CHEFZ_FRESH stehen im Delta _deltas/produce.json und werden vom
// chefz-registry-integrator gemergt. Dieses Modul fasst keine zentrale
// Registry an (Workflow §5).
//
// Seit dem 29.08.2026 bindet der Slice produce hier NICHTS mehr: das
// Schnittgut ist entfallen, und das ganze Gemuese bindet ChefZ_Farming.
//==============================================================================
class CfgChefZIngredients
{
    // Getrocknete Beeren. BERRY haengt unter FRUIT (Delta _deltas/vanilla-foods.json);
    // ein FRUIT-Slot nimmt sie damit mit, ein BERRY-Slot nur sie und die zwei
    // frischen Vanillabeeren. CHEFZ_PRESERVED trennt sie im Rezept vom frischen
    // Obst - RCP_ChefZ_FruitCompote nutzt genau diesen Unterschied.
    //
    // defaultState "DRIED": die beiden Transforms setzen ihn ohnehin; dieser
    // Wert ist die Rueckfallebene der Zustandsprojektion (06 §3) fuer Exemplare
    // aus Admin- oder Lootspawn.
    class ChefZ_DriedBerries
    {
        categories[]      = {"BERRY"};
        tags[]            = {"CHEFZ_PRESERVED"};
        defaultState      = "DRIED";
        quantityUnit      = "PIECE";
        unitsPerWholeItem = 1;
        decays            = 1;
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
//    0 Plaetze seit dem 29.08.2026: die sieben Schnitt-Transforms sind mit
//    dem Schnittgut entfallen, davor die vier Samen-Transforms mit dem
//    Saatgut. Der Slice bindet nur noch die drei Vanilla-Gemuese.
//==============================================================================
class CfgChefZ
{
    // ### SLICE produce ###
    class ChefZ_Produce
    {
        chefzApiVersion = 1;
        loadOrder = 220;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Ingredients/Config/Ingredients/VanillaProduce.json"
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
    // Station ChefZ_FryingPan. Vanillas Rezeptliste bleibt um kein Bit
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

    // ### SLICE sauces ###
    //
    // Speisepilze. Eigener Knoten, eigene Datei - die Slices teilen sich das
    // Modul, aber keine Datei.
    //
    // Warum die Pilze hier und nicht in ChefZ_Cooking liegen: es sind FREMDE
    // Klassen, und Entwurf 05 §2 bindet fremde Klassen im Slice-JSON eines
    // ZUTATEN-Moduls - genau wie Potato, Tomato und GreenBellPepper darueber.
    // ChefZ_Cooking enthaelt Gerichte und Saucen, keine Rohstoffbindungen.
    //
    // Mushrooms.json bindet SIEBEN der neun Vanilla-Pilzklassen. NICHT
    // gebunden sind AmanitaMushroom (giftig) und PsilocybeMushroom
    // (halluzinogen): wer sie in die Kategorie MUSHROOM aufnaehme, liesse
    // RCP_ChefZ_MushroomCreamSauce eine Sauce aus Fliegenpilzen kochen, deren
    // Ergebnisklasse keine Toxizitaet mehr traegt - das Gift verschwaende beim
    // Kochen. Ein spaeterer Slice, der Giftpilze bewusst will, gibt ihnen eine
    // eigene Kategorie und ein eigenes Ergebnis.
    //
    // loadOrder 230: Pilze sind Zutat FUER andere Ketten (Pilzrahmsauce,
    // Pilzpfanne), nie umgekehrt. Der Core haengt Records nicht voneinander
    // ab; die Reihenfolge ist Vorsorge und kostet nichts.
    //
    // handcraftRecipeSlots = 0: Pilze werden nicht verarbeitet, sie werden
    // gekocht. Vanillas Rezeptliste bleibt um kein Bit veraendert.
    class ChefZ_SauceIngredients
    {
        chefzApiVersion = 1;
        loadOrder = 230;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Ingredients/Config/Ingredients/Mushrooms.json"
        };
    };

    // ### SLICE vanilla-foods ###
    //
    // Die Zutatenbindung der bisher ungenutzten Vanilla-Assets aus
    // Vanilla-Audit §3. Eigener Knoten, weil CfgChefZ genau EINEN Knoten je
    // SLICE traegt (02 §4) - dieses Modul ist ein geteilter Ordner.
    //
    // loadOrder 240: nach den Pilzen (230) und vor den Saucen (300). Der Slice
    // ist reine Rohstoffbindung; er wird von den Gerichteslices gelesen, liest
    // aber selbst aus keiner anderen Kette. Der Core haengt Records nicht
    // voneinander ab - die Reihenfolge ist Vorsorge und kostet nichts.
    //
    // handcraftRecipeSlots = 0: die beiden Beeren-Transforms laufen als
    // PROCESS_DRY (STATION_TIMED am ChefZ_DryingRack) und ruehren Vanillas
    // Rezeptliste nicht an. TR_ChopZucchini ist mit dem Schnittgut entfallen.
    // Wer hier einen HANDCRAFT-Transform ergaenzt, erhoeht diese Zahl in
    // derselben Aenderung (Kopf von ChefZ_HandcraftBridge.c).
    class ChefZ_VanillaFoods
    {
        chefzApiVersion = 1;
        loadOrder = 240;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Ingredients/Config/Ingredients/VanillaFoodstuffs.json",
            "ChefZ_Ingredients/Config/Processing/VanillaFoodProcessing.json"
        };
    };
};
