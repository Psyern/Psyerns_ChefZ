// ChefZ_Preservation - Salzen und Poekeln, Trocknen, Raeuchern.
//
// Slice "preservation". Quellen: Production Map §40-§46 (Wurst-, Fleisch- und
// Fischzustaende), §56 (V1 Preservation Matrix), §57 (Trockenrahmen und
// Raeucherschrank), §73 (je Konservierungszustand eine eigene Klasse),
// DME-Plan §31 (Konservierungssystem), §33 (Haltbarkeitsstufen), §53
// (Namenskonvention), Planungsschritte §17 (Multiplikatoren).
//
// NICHT in V1 (Production Map §74): Fermentieren, Einkochen, Einlegen,
// Konservendosen. Die Zustaende PICKLED und CANNED stehen deshalb NICHT unten
// in CfgChefZStates - ein Zustand ohne Uebergang waere eine Zusage ohne Weg.
//
//   Raw Meat    + Salz   --HANDCRAFT-->      ChefZ_SaltedMeat      §43
//   SaltedMeat  + Rahmen --PROCESS_DRY-->    ChefZ_DriedMeat       §43
//   SaltedMeat  + Schrank--PROCESS_SMOKE-->  ChefZ_SmokedMeat      §43
//   Fischfilet  + Salz   --HANDCRAFT-->      ChefZ_SaltedFish      §45
//   SaltedFish  + Rahmen --PROCESS_DRY-->    ChefZ_DriedFish       §45
//   Fischfilet  + Schrank--PROCESS_SMOKE-->  ChefZ_SmokedFish      §46
//   Raw Sausage + Schrank--PROCESS_SMOKE-->  ChefZ_SmokedSausage   §41
//   Raw Sausage + Rahmen --PROCESS_DRY-->    ChefZ_DrySausage      §42
//
// Kraeuter, Pfefferbeeren und Paprika (§56, Zeilen 1-3) trocknet bereits der
// Slice "herbs" in ChefZ_Processing/Config/Processing/HerbDrying.json, rohe
// Nudeln (Zeile 4) der Slice "grain" in ChefZ_Baking/Config/GrainTransforms.json
// (TR_RawPastaToDriedPasta). Beides steht hier bewusst NICHT noch einmal: ein
// zweiter Transform auf dieselbe Klasse waere ein zweiter Weg zum selben
// Ergebnis und eine zweite Stelle, an der jemand die Dauer aendern muesste.
// Diese Datei schliesst die Matrix, sie schreibt sie nicht neu.
//
// PFADWURZEL: das PBO-Praefix ist der ORDNERNAME des Addons. Jeder Laufzeitpfad
// beginnt deshalb mit "ChefZ_Preservation/" (Entwurf 02 §4.1).
//
// ---------------------------------------------------------------------------
// Warum hier ZEHN Zustaende stehen und nicht drei
// ---------------------------------------------------------------------------
// CfgChefZStates ist die Registry der Lebensmittelzustaende (06 §4.1). Sie ist
// SYNC-RELEVANT und darf deshalb ausschliesslich in Rang 1 stehen, also in
// einer config.cpp - nie in JSON (03 §4, Kopf von ChefZ_StateDef.c).
//
// Bis zu diesem Slice deklarierte sie niemand. Die Folge war nicht "es fehlt
// etwas", sondern: JEDE der 116 Zustandsreferenzen im Projekt zeigte ins Leere,
// und jede Haltbarkeitsregel mit scope "state" war zur Laufzeit wirkungslos -
// ChefZ_PreservationManager.Build() findet einen Zustand, den kein Datensatz
// deklariert, schlicht nicht. Der Slice "preservation" ist der erste, der die
// Zustaende BRAUCHT, und deshalb der Ort, an dem sie entstehen.
//
// Das Vokabular ist geteilt, nicht privat: RAW, COOKED oder BURNT gehoeren
// keinem Slice. Sie stehen hier, weil sie irgendwo stehen muessen und weil
// Zustandsuebergaenge der Auftrag dieses Slice sind - nicht, weil dieses Modul
// sie besitzt. Wer sie spaeter in ein eigenes Vokabelmodul zieht, verschiebt
// diesen einen Block und aendert sonst nichts.
//
// ---------------------------------------------------------------------------
// Wo der Haltbarkeitsmultiplikator steht - und wo NICHT
// ---------------------------------------------------------------------------
// 14 §3 multipliziert StateDef.spoilageMultiplier UND die Records aus
// Preservation.json. Beide Wege fuehren zum selben Zustand. Wer beide fuellt,
// multipliziert zweimal: SMOKED waere dann 0.25 * 0.25 = 0.0625 und damit vier
// mal haltbarer als die Matrix sagt - ohne dass es irgendwo auffiele.
//
// Deshalb die Festlegung dieses Slice: der Multiplikator steht AUSSCHLIESSLICH
// in den Preservation-Records (Registry, gespeist aus _deltas/preservation.json).
// spoilageMultiplier bleibt in CfgChefZStates auf dem Sentinel und damit auf
// 1.0. Eine Zahl, zwei Orte - das waere eine Zahl zu viel.
//
// freshnessLifetimeSec dagegen steht hier, weil es dort NICHT steht: die
// Restfrische (14 §4) hat in Preservation.json kein Feld. Ohne Angabe gilt
// defaultFreshnessLifetimeSec aus Core.json (21600 s); die konservierten
// Zustaende setzen laengere Werte, weil genau das ihr Sinn ist.
//
// ---------------------------------------------------------------------------
// MODELLE
// ---------------------------------------------------------------------------
// Es gibt noch keine eigene Geometrie. Jede Klasse traegt ein Vanilla-Proxy;
// der Bedarf steht im Bericht des Slice. Kein Item wartet auf ein Modell.

class CfgPatches
{
    class ChefZ_Preservation
    {
        units[] =
        {
            // Die Basis der acht haltbar gemachten Waren.
            "ChefZ_PreservedFood_Base",
            "ChefZ_SaltedMeat",
            "ChefZ_DriedMeat",
            "ChefZ_SmokedMeat",
            "ChefZ_SaltedFish",
            "ChefZ_DriedFish",
            "ChefZ_SmokedFish",
            "ChefZ_SmokedSausage",
            "ChefZ_DrySausage"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // Jeder Eintrag steht fuer etwas, das dieses Modul TATSAECHLICH nutzt:
        //   ChefZ_Core       ChefZ_Edible_Base als Skriptbasis und der Config
        //                    Manager, der CfgChefZStates einliest
        //   ChefZ_Processing PROCESS_DRY, PROCESS_SMOKE, PROCESS_SALT_CURE und
        //                    die Stationen ChefZ_DryingRack und ChefZ_Smoker
        //   ChefZ_Meat       ChefZ_RawSausage und die uebrigen Rohwuerste als
        //                    Eingang von Raeuchern und Trocknen
        //   DZ_Gear_Food     Edible_Base und die Proxy-Modelle
        //   DZ_Data          Grundlage von allem
        // ChefZ_Ingredients steht bewusst NICHT hier: das Salz wird ueber die
        // KATEGORIE "SALT" gebunden, nicht ueber eine Klasse. Eine Kategorie ist
        // Daten und kein Addon - der Transform greift, sobald irgendein Modul
        // etwas in diese Kategorie legt, und faellt sonst still aus.
        requiredAddons[] = {"DZ_Data", "DZ_Gear_Food", "ChefZ_Core", "ChefZ_Processing", "ChefZ_Meat", "ChefZ_Food"};
    };
};

// ---------------------------------------------------------------------------
// Skriptmodul dieses PBO.
//
// Eigener CfgMods-Knoten, weil der Knoten des Core ausschliesslich Pfade
// unterhalb von "ChefZ_Core/" nennt und das PBO-Praefix die Wurzel JEDES
// Laufzeitpfades ist (02 §4.1). Ohne diesen Block laedt DayZ
// "ChefZ_Preservation/Scripts/..." still nicht.
//
// Der Knoten heisst ChefZ_PreservationMod und nicht ChefZ_Preservation: der
// CfgChefZ-Knoten weiter unten MUSS ChefZ_Preservation heissen - er ist die
// Slice-Identitaet, unter der der Core das Modul meldet - und zwei gleichnamige
// Klassen in derselben Wurzel waeren eine doppelte Definition.
// ---------------------------------------------------------------------------
class CfgMods
{
    class ChefZ_PreservationMod
    {
        dir = "ChefZ_Preservation";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Preservation";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        dependencies[] = {"World"};

        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Preservation/Scripts/4_World"
                };
            };
        };
    };
};

class CfgVehicles
{
    class Edible_Base;

    // ------------------------------------------------------------------------
    // Gemeinsame Configbasis dieses Moduls.
    //
    // scope = 0: kein Item, sondern die Stelle, an der Garstufen und
    // Grundeigenschaften EINMAL stehen. Der Core bringt keine solche Basis mit
    // (Invariante I3, Kopf von ChefZ_Edible_Base.c).
    //
    // Configbasis ist eine VANILLA-Klasse, Skriptbasis ist ChefZ_Edible_Base.
    //
    // ZWEI PFLICHTBLOECKE an jeder essbaren Klasse, beide belegt:
    //   class Nutrition   PlayerStomach.InitData registriert nur Klassen mit
    //                     "Nutrition" ODER "Food" und scope != 0 (01 V7).
    //                     Fehlt beides, saettigt der Bissen lautlos nicht.
    //   FoodStage-        FoodStage.GetNextFoodStageType faellt ohne passenden
    //   Transitions       Uebergang auf BURNED zurueck (FoodStage.c:472) - eine
    //                     kochbare Klasse OHNE Uebergaenge verbrennt (01 V4).
    //
    // nutrition_properties[] in der Reihenfolge aus FoodStage.c:
    //   { fullnessIndex, energy, water, nutritionalIndex, toxicity, agents,
    //     digestibility }
    // agents: eAgents.SALMONELLA = 4, eAgents.FOOD_POISON = 16 (EAgents.c).
    //
    // Was hier ueber die Keime entschieden ist (06 §3):
    //   SALTED  projiziert auf Raw   -> agents 4. Salzen toetet keine Keime.
    //   DRIED / SMOKED auf Dried     -> agents 0. Vanilla raeumt bei DRIED die
    //                                   Agenten ab; ChefZ baut das nicht nach.
    // ------------------------------------------------------------------------
    class ChefZ_PreservedFood_Base : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\food\steak.p3d";
        rotationFlags = 17;
        itemSize[] = {2, 1};
        weight = 220;
        absorbency = 0.4;
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
                // visual_properties[] = { selectionIndex, textureIndex, materialIndex }
                // Alle Proxys sind einteilige Modelle - deshalb ueberall 0.
                // cooking_properties[] = { minTemp, cookTime, maxTemp }
                // woertlich aus enum eCookingPropertyIndices (FoodStage.c:15).
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
                class Dried
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
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

            // OHNE DIESEN BLOCK VERBRENNT JEDES ITEM DES MODULS (01 V4).
            //
            // Zwei Uebergaenge, beide AUS "Raw" - gepoekeltes Fleisch laesst
            // sich braten und kochen wie rohes.
            //
            // AUS "Dried" GIBT ES KEINEN, und das ist die eigentliche Aussage
            // dieses Blocks: Doerrfleisch, Trockenfisch, Raeucherwurst sind
            // FERTIG. Wer sie in die Pfanne legt, bekommt Kohle - genau wie in
            // der Wirklichkeit und genau wie GetNextFoodStageType es ohne
            // Eintrag ohnehin tut (FoodStage.c:472).
            //
            // transition_to und cooking_method sind ZAHLEN, nicht Namen
            // (SetupFoodStageTransitionMapping liest sie mit ConfigGetInt,
            // FoodStage.c:167ff):
            //   FoodStageType:     RAW 1, BAKED 2, BOILED 3, DRIED 4, BURNED 5, ROTTEN 6
            //   CookingMethodType: NONE 0, BAKING 1, BOILING 2, DRYING 3, TIME 4
            //
            // DRYING (3) fehlt absichtlich, obwohl dieses Modul das Trocknen
            // baut: Vanillas Trocknen kennt GENAU EINEN Uebergang RAW -> DRIED
            // und sonst BURNED (01 V14). Die Matrix §56 verlangt vier
            // Uebergaenge mit verschiedenen Haltbarkeiten. Deshalb laufen
            // Trocknen und Raeuchern an EIGENEN Stationen (11 E6) - Vanillas
            // Smoking-Slots bleiben unangetastet.
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_PreservedRawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_PreservedRawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    //==========================================================================
    // §43 - Fleisch: gesalzen, getrocknet, geraeuchert
    //==========================================================================

    // Raw Meat + Salz. Wasser wird entzogen, die Energie bleibt - deshalb
    // weniger water und ein hoeherer Energiewert als am rohen Stueck.
    // agents 4: Poekeln ist keine Hitze (06 §3).
    // PROXY: steak.p3d. Ziel: ein Stueck Fleisch mit Salzkruste.
    class ChefZ_SaltedMeat : ChefZ_PreservedFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SALTEDMEAT0";
        descriptionShort = "#STR_CHEFZ_ITEM_SALTEDMEAT1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 240;

        class Nutrition
        {
            fullnessIndex = 115;
            energy = 165;
            water = 25;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {115, 165, 25, 16, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {105, 330, 12, 26, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {110, 300, 40, 26, 0, 0, 1}; };
                class Dried { nutrition_properties[] = {90, 320, 5, 30, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {75, 85, 6, 5, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {95, 100, 18, 5, 20, 16, 1}; };
            };
        };
    };

    // SaltedMeat am Trockenrahmen. Der Vorratsartikel schlechthin: sehr wenig
    // Wasser, sehr viel Energie je Platz. Kein Kochbedarf mehr.
    // PROXY: steak.p3d. Ziel: dunkles, geschrumpftes Doerrfleisch.
    class ChefZ_DriedMeat : ChefZ_PreservedFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRIEDMEAT0";
        descriptionShort = "#STR_CHEFZ_ITEM_DRIEDMEAT1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 120;

        class Nutrition
        {
            fullnessIndex = 90;
            energy = 320;
            water = 5;
            nutritionalIndex = 30;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {90, 320, 5, 30, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {90, 320, 5, 30, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {95, 300, 35, 30, 0, 0, 1}; };
                class Dried { nutrition_properties[] = {90, 320, 5, 30, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {22, 80, 1, 7, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {36, 128, 2, 6, 20, 16, 1}; };
            };
        };
    };

    // SaltedMeat im Raeucherschrank. Zwischen frisch und doerr: mehr Wasser als
    // Doerrfleisch, laenger haltbar als gesalzenes - der Reiseproviant aus §41.
    // PROXY: steak.p3d. Ziel: dunkel geraeuchertes Stueck mit Rauchrand.
    class ChefZ_SmokedMeat : ChefZ_PreservedFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SMOKEDMEAT0";
        descriptionShort = "#STR_CHEFZ_ITEM_SMOKEDMEAT1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 180;

        class Nutrition
        {
            fullnessIndex = 100;
            energy = 300;
            water = 12;
            nutritionalIndex = 28;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {100, 300, 12, 28, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {100, 300, 12, 28, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {105, 285, 42, 28, 0, 0, 1}; };
                class Dried { nutrition_properties[] = {100, 300, 12, 28, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {25, 75, 3, 7, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {40, 120, 5, 6, 20, 16, 1}; };
            };
        };
    };

    //==========================================================================
    // §45/§46 - Fisch: gesalzen, getrocknet, geraeuchert
    //
    // Das Filetieren selbst gehoert NICHT hierher (§44, Uebergabepunkt): es ist
    // Vanilla bzw. eine Fischerei-Erweiterung. Dieses Modul beginnt beim
    // fertigen Filet und faellt still aus, wenn keines da ist.
    //==========================================================================

    // Fischfilet + Salz. Fisch traegt weniger Energie und mehr Naehrwertindex
    // als Fleisch - das bleibt ueber die ganze Kette so.
    // PROXY: steak.p3d. Ziel: ein flaches Salzfilet.
    class ChefZ_SaltedFish : ChefZ_PreservedFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SALTEDFISH0";
        descriptionShort = "#STR_CHEFZ_ITEM_SALTEDFISH1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 95;
            energy = 130;
            water = 22;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {95, 130, 22, 20, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {88, 250, 10, 32, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {92, 230, 38, 32, 0, 0, 1}; };
                class Dried { nutrition_properties[] = {80, 260, 4, 34, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {60, 65, 5, 6, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {78, 80, 16, 6, 20, 16, 1}; };
            };
        };
    };

    // SaltedFish am Trockenrahmen. Stockfisch: leicht, trocken, extrem lange
    // haltbar.
    // PROXY: steak.p3d. Ziel: hartes, helles Trockenfilet.
    class ChefZ_DriedFish : ChefZ_PreservedFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRIEDFISH0";
        descriptionShort = "#STR_CHEFZ_ITEM_DRIEDFISH1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 95;

        class Nutrition
        {
            fullnessIndex = 80;
            energy = 260;
            water = 4;
            nutritionalIndex = 34;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {80, 260, 4, 34, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {80, 260, 4, 34, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {85, 245, 32, 34, 0, 0, 1}; };
                class Dried { nutrition_properties[] = {80, 260, 4, 34, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {20, 65, 1, 8, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {32, 104, 2, 7, 20, 16, 1}; };
            };
        };
    };

    // Fischfilet direkt in den Raeucherschrank - §46 kennt hier KEINEN
    // Salzschritt davor, anders als beim Fleisch. Das ist kein Versehen der
    // Vorlage, sondern der kuerzeste Weg zu haltbarem Fisch und der Grund,
    // warum sich Raeuchern lohnt, bevor man Salz hat.
    // PROXY: steak.p3d. Ziel: goldbraun geraeuchertes Filet.
    class ChefZ_SmokedFish : ChefZ_PreservedFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SMOKEDFISH0";
        descriptionShort = "#STR_CHEFZ_ITEM_SMOKEDFISH1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 150;

        class Nutrition
        {
            fullnessIndex = 88;
            energy = 245;
            water = 10;
            nutritionalIndex = 32;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {88, 245, 10, 32, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {88, 245, 10, 32, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {92, 232, 36, 32, 0, 0, 1}; };
                class Dried { nutrition_properties[] = {88, 245, 10, 32, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {22, 61, 2, 8, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {35, 98, 4, 6, 20, 16, 1}; };
            };
        };
    };

    //==========================================================================
    // §41/§42 - Wurst: geraeuchert und getrocknet
    //
    // Beide Wege gehen von der ROHEN Wurst aus (§40) und nicht von der
    // gebratenen: eine gebratene Wurst ist ein fertiges Gericht, keine
    // Vorstufe. Die Wurstsorten aus ChefZ_Meat fuehren alle auf DIESE beiden
    // Klassen zusammen - fuenf Rohwuerste mal zwei Konservierungswege waeren
    // zehn Klassen, die sich in nichts unterscheiden, was der Spieler sieht.
    //==========================================================================

    // §41: Raeucherwurst. Reiseproviant, hoher Handelswert.
    // PROXY: sausage.p3d. Ziel: dunkle, runzelige Rauchwurst.
    class ChefZ_SmokedSausage : ChefZ_PreservedFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SMOKEDSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_SMOKEDSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_smoked.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 1};
        weight = 260;

        class Nutrition
        {
            fullnessIndex = 135;
            energy = 500;
            water = 14;
            nutritionalIndex = 33;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {135, 500, 14, 33, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {135, 500, 14, 33, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {138, 480, 44, 33, 0, 0, 1}; };
                class Dried { nutrition_properties[] = {135, 500, 14, 33, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {34, 125, 4, 8, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {54, 200, 6, 7, 20, 16, 1}; };
            };
        };
    };

    // §42: Trockenwurst. Der haltbarste Posten der ganzen Matrix.
    // PROXY: sausage.p3d. Ziel: harte, bemehlte Dauerwurst.
    class ChefZ_DrySausage : ChefZ_PreservedFood_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRYSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_DRYSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_dry.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 1};
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 125;
            energy = 545;
            water = 6;
            nutritionalIndex = 35;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {125, 545, 6, 35, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {125, 545, 6, 35, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {130, 520, 36, 35, 0, 0, 1}; };
                class Dried { nutrition_properties[] = {125, 545, 6, 35, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {31, 136, 2, 9, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {50, 218, 3, 7, 20, 16, 1}; };
            };
        };
    };
};

//------------------------------------------------------------------------------
// Die Lebensmittelzustaende (06 §4.1).
//
// RANG 1, ZWINGEND: CfgChefZStates ist sync-relevant und darf laut 03 §4
// ausschliesslich aus der config.cpp kommen. In JSON stuende sie im Rang 2 und
// waere ueber das $profile:-Overlay veraenderbar - genau das verbietet der
// Entwurf, weil ein Zustand ueber das Netz geht.
//
// projectsToVanillaStage ist die Projektionsregel aus 06 §3. Wo eine Projektion
// steht, bleiben die VANILLA-Mechaniken zustaendig: visual_properties,
// Naehrwertbasis, Agentenbereinigung, der CanProcessDecay()-Stopp bei ROTTEN,
// Kochsounds, Frostlogik. ChefZ baut keine davon nach.
//
// Die Namen sind ZAHLENLOSE Symbole, keine Enums (03 E1). Ein Tippfehler faellt
// deshalb nicht beim Uebersetzen auf, sondern im Validator - das ist der Preis
// und die Auflage des datengetriebenen Entwurfs.
//------------------------------------------------------------------------------
class CfgChefZStates
{
    // Der Ausgangszustand von allem. Ohne Projektion waere er nicht von
    // "kein Zustand" zu unterscheiden.
    class RAW
    {
        displayName = "#STR_CHEFZ_STATE_RAW";
        projectsToVanillaStage = "Raw";
        edible = 1;
    };

    // Geschnitten, gewolft, gemoersert - roh geblieben. Der Zustand ohne
    // eigene Optik: er sagt "ein Mensch hat daran gearbeitet", mehr nicht.
    class PREPARED
    {
        displayName = "#STR_CHEFZ_STATE_PREPARED";
        projectsToVanillaStage = "Raw";
        edible = 1;
    };

    // Ergebniszustand gekochter Gerichte. Projiziert auf Boiled, damit Vanilla
    // die Agenten abraeumt (06 §3).
    class COOKED
    {
        displayName = "#STR_CHEFZ_STATE_COOKED";
        projectsToVanillaStage = "Boiled";
        edible = 1;
    };

    class BAKED
    {
        displayName = "#STR_CHEFZ_STATE_BAKED";
        projectsToVanillaStage = "Baked";
        edible = 1;
    };

    // Gebraten. Eigener Name, gleiche Projektion wie BAKED: fuer Vanilla ist
    // Pfanne gleich Ofen, fuer ein Rezept nicht.
    class FRIED
    {
        displayName = "#STR_CHEFZ_STATE_FRIED";
        projectsToVanillaStage = "Baked";
        edible = 1;
    };

    //--------------------------------------------------------------------------
    // Die drei Konservierungszustaende dieses Slice.
    //
    // spoilageMultiplier steht hier BEWUSST NICHT - siehe Dateikopf. Er steht
    // einmal, in den Preservation-Records: SALTED 0.50, SMOKED 0.25,
    // DRIED 0.15 (Planungsschritte §17, DME-Plan §33).
    //
    // freshnessLifetimeSec steht dagegen hier, weil es sonst nirgends steht.
    // Bezug ist defaultFreshnessLifetimeSec = 21600 s aus Core.json.
    //--------------------------------------------------------------------------

    // §43/§45. Projiziert auf RAW und nicht auf Dried - Salzen toetet keine
    // Keime (06 §3, ausdruecklich). Gesalzenes Fleisch bleibt roh, es haelt
    // nur laenger. Deshalb traegt ChefZ_SaltedMeat auch agents = 4.
    class SALTED
    {
        displayName = "#STR_CHEFZ_STATE_SALTED";
        projectsToVanillaStage = "Raw";
        implies[] = {"CHEFZ_PRESERVED"};
        freshnessLifetimeSec = 43200;
        edible = 1;
        preserved = 1;
    };

    // §41/§43/§46. Projiziert auf Dried: Vanilla raeumt die Agenten ab und
    // rechnet schon von sich aus mit DECAY_FOOD_DRIED_* - der ChefZ-Faktor
    // wirkt DARAUF, nicht dagegen (14 §3, letzter Absatz).
    // Dass SMOKED und DRIED dieselbe Vanilla-Stufe teilen, ist kein Verlust:
    // ChefZ unterscheidet sie ueber den eigenen Zustand weiter (§56/§65).
    class SMOKED
    {
        displayName = "#STR_CHEFZ_STATE_SMOKED";
        projectsToVanillaStage = "Dried";
        implies[] = {"CHEFZ_PRESERVED"};
        freshnessLifetimeSec = 86400;
        edible = 1;
        preserved = 1;
    };

    // §42/§43/§45 und die Kraeuter-, Pfeffer-, Paprika- und Nudelzeilen der
    // Matrix. Der haltbarste Zustand in V1.
    class DRIED
    {
        displayName = "#STR_CHEFZ_STATE_DRIED";
        projectsToVanillaStage = "Dried";
        implies[] = {"CHEFZ_PRESERVED"};
        freshnessLifetimeSec = 129600;
        edible = 1;
        preserved = 1;
    };

    //--------------------------------------------------------------------------
    // Die beiden Fehlzustaende.
    //
    // Sie stehen hier, weil Core.json sie in defaultExcludedStates und in
    // qualityScoring.statePenalties nennt - ein Ausschluss, der auf einen
    // undeklarierten Zustand zeigt, schliesst nichts aus.
    //--------------------------------------------------------------------------

    // Verbrannt. edible = 0, aber NICHT terminal: aus Kohle wird noch Fauliges.
    class BURNT
    {
        displayName = "#STR_CHEFZ_STATE_BURNT";
        projectsToVanillaStage = "Burned";
        edible = 0;
    };

    // Verdorben. terminal = 1: ein angefragter Uebergang wird mit dem Grund
    // "terminal state" abgelehnt (06 §7). Den Zustand selbst vergibt
    // ausschliesslich der VANILLA-Verfall - "Frische bestraft, Vanilla toetet"
    // (14 §4). ChefZ setzt ihn nie.
    class ROTTEN
    {
        displayName = "#STR_CHEFZ_STATE_ROTTEN";
        projectsToVanillaStage = "Rotten";
        edible = 0;
        terminal = 1;
    };
};

//------------------------------------------------------------------------------
// Anmeldung beim Core (02 §4).
//
// handcraftRecipeSlots = 2: dieses Modul bringt GENAU ZWEI Transforms mit,
// deren Prozess exec = "HANDCRAFT" hat - TR_SaltMeat und TR_SaltFish, beide
// ueber PROCESS_SALT_CURE. Die Zahl ist eine Reservierung in Vanillas
// Rezeptliste und muss vorab feststehen (Kopf von ChefZ_HandcraftBridge.c).
// Trocknen und Raeuchern laufen an Stationen und brauchen keinen Platz.
//
// dataFiles[] beginnt mit dem PBO-Praefix, also dem ORDNERNAMEN des Addons.
// loadOrder 280: nach ChefZ_Meat (200) und ChefZ_Processing (155-260), weil
// die Transforms dieses Moduls deren Klassen und Prozesse voraussetzen.
//------------------------------------------------------------------------------
class CfgChefZ
{
    class ChefZ_Preservation
    {
        chefzApiVersion = 1;
        loadOrder = 280;
        handcraftRecipeSlots = 2;
        dataFiles[] =
        {
            "ChefZ_Preservation/Config/Ingredients/Preservation.json",
            "ChefZ_Preservation/Config/Processing/Salting.json",
            "ChefZ_Preservation/Config/Processing/Drying.json",
            "ChefZ_Preservation/Config/Processing/Smoking.json"
        };
    };
};
