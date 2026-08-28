// ChefZ_Baking - Hefe, Teig, Pasta, Brot (Slice "grain").
//
// Quelle: Production Map §9 (Hefe), §10 (Teig), §11 (Pasta), §12 (Brot),
// §56 (Preservation-Matrix, Zeile "Raw Pasta -> Dry -> Dried Pasta"),
// §73 (Klassenliste), DME-Plan §53 (Namenskonvention).
//
// PBO-Praefix: $PREFIX$ enthaelt "ChefZ_Baking". Die Wurzel jedes
// Laufzeitpfades - auch jedes dataFiles[]-Eintrags - ist dieses Praefix
// (Entwurf 02 §4.1, B4).
//
// ---------------------------------------------------------------------------
// KEIN NEUES CORE-SYSTEM
// ---------------------------------------------------------------------------
// Die ganze Kette besteht aus Datensaetzen:
//
//   Teig, Pasta        Transforms ueber HANDCRAFT bzw. die Trockenstation
//   Brot, Fladenbrot   Rezepte am Kochgeraet (Entwurf 08, 10)
//
// Kein Skript entscheidet, was woraus wird. Die Skriptdatei dieses Moduls
// enthaelt ausschliesslich Klassenableitungen ohne Rumpf.
//
// ---------------------------------------------------------------------------
// Warum Hefeteig ZWEISTUFIG entsteht
// ---------------------------------------------------------------------------
// Production Map §10 schreibt "Flour + Water + Yeast -> Yeast Dough" - drei
// Eingaenge. Vanillas RecipeBase kennt MAX_NUMBER_OF_INGREDIENTS = 2
// (01 V12); ein HANDCRAFT-Transform mit drei Eingaengen liesse sich STILL
// nicht registrieren. Die Kette ist deshalb aufgeteilt:
//
//   Flour + Water        -> ChefZ_SimpleDough
//   ChefZ_SimpleDough + Yeast -> ChefZ_YeastDough
//
// Das Ergebnis ist dasselbe, die Zwischenstufe ist ohnehin ein eigenes Item
// (§10, "Einfacher Teig"), und keine Zutat geht verloren.
//
// ---------------------------------------------------------------------------
// Nahrungsdaten
// ---------------------------------------------------------------------------
// Alle essbaren Klassen erben Nutrition und Food von ChefZ_GrainFoodBase aus
// ChefZ_Farming. Der Config-Knoten ist die Anmeldung an Vanillas Magen
// (01 V7), die stueckgenauen Zahlen stehen im Nutrition-Delta des Slices
// (_deltas/grain.json, Entwurf 13 §4).
//
// Damit tragen SimpleDough und YeastDough auch FoodStageTransitions - und das
// ist hier keine Formalie: beide sind Zutat eines Rezepts, liegen also im
// Kochgeraet, waehrend Vanilla den Garzustand fortschreibt. Ohne Uebergaenge
// faellt FoodStage.GetNextFoodStageType auf BURNED zurueck (01 V4,
// FoodStage.c:472) - der Spieler legte Teig in die Pfanne und bekaeme Kohle.
//
// ---------------------------------------------------------------------------
// 3D
// ---------------------------------------------------------------------------
// Jede Klasse traegt ein Vanilla-Proxy-Modell. Die drei Teige teilen sich
// bewusst EIN Proxy (Production Map §71, Shared Mesh Strategy); der Bedarf ist
// im Slice-Bericht gemeldet.

class CfgPatches
{
    class ChefZ_Baking
    {
        units[] =
        {
            "ChefZ_Yeast", "ChefZ_SimpleDough", "ChefZ_YeastDough", "ChefZ_PastaDough",
            "ChefZ_RawPasta", "ChefZ_DriedPasta", "ChefZ_Bread", "ChefZ_Flatbread"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // ChefZ_Core:       ChefZ_Edible_Base.
        // ChefZ_Farming:    ChefZ_GrainFoodBase (Nahrungsbasis).
        // ChefZ_Processing: ChefZ_Flour als Eingang und die Werkzeuggruppe
        //                   ROLLING_PIN, auf die PROCESS_ROLL zeigt.
        // DZ_Gear_Food:     die Proxy-Modelle.
        requiredAddons[] = {"DZ_Data", "DZ_Gear_Food", "ChefZ_Core", "ChefZ_Farming", "ChefZ_Processing"};
    };
};

class CfgMods
{
    class ChefZ_Baking
    {
        dir = "ChefZ_Baking";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Baking";
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
                    "ChefZ_Baking/Scripts/4_World"
                };
            };
        };
    };
};

class CfgVehicles
{
    // ChefZ_GrainFoodBase kommt aus ChefZ_Farming und MUSS hier
    // vorwaertsdeklariert werden. DayZ loest eine Elternklasse nur innerhalb
    // DERSELBEN config.cpp auf; steht sie dort weder mit Rumpf noch als
    // Deklaration, bricht der Configlauf mit "Undefined base class" ab und der
    // Server bleibt an einem Fehlerdialog stehen, den auf einem Server niemand
    // wegklickt. Genau das ist am 28.08.2026 passiert.
    //
    // Hier stand vorher die Annahme, eine leere Vorwaertsdeklaration wuerde den
    // echten Knoten mitsamt Nutrition und Food verdecken. Sie ist falsch: eine
    // Deklaration ohne Rumpf ersetzt nichts, sie macht den Namen nur
    // aufloesbar. ChefZ_Farming selbst fuehrt es vor - es deklariert
    // Edible_Base auf dieselbe Weise und erbt dessen Nutrition vollstaendig.
    // Die Ladereihenfolge sichert requiredAddons[] weiter oben zu.
    class ChefZ_GrainFoodBase;

    //--------------------------------------------------------------------------
    // Hefe (Production Map §9). V1 ausschliesslich Loot - keine Hefekultur.
    //
    // PROXY: garden_lime.p3d - ein Pulverbeutel. Eigenes Mesh gemeldet (U, P2).
    //--------------------------------------------------------------------------
    class ChefZ_Yeast : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_YEAST";
        descriptionShort = "#STR_CHEFZ_YEAST_DESC";
        model = "\dz\gear\consumables\garden_lime.p3d";
        weight = 60;
        itemSize[] = {1, 1};
        stackedUnit = "grams";
        quantityBar = 1;
        varQuantityInit = 100;
        varQuantityMin = 0;
        varQuantityMax = 100;
        varQuantityDestroyOnMin = 1;
        canBeSplit = 1;
        lifetime = 28800;
    };

    //--------------------------------------------------------------------------
    // Einfacher Teig (§10). Fladenbrot und einfache Teigtaschen.
    //
    // PROXY: lard.p3d - ein heller Klumpen. Geteiltes Proxy fuer alle drei
    // Teige (§71). Eigenes Mesh gemeldet (S, P2).
    //--------------------------------------------------------------------------
    class ChefZ_SimpleDough : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_SIMPLEDOUGH";
        descriptionShort = "#STR_CHEFZ_SIMPLEDOUGH_DESC";
        model = "\dz\gear\food\lard.p3d";
        weight = 450;
        itemSize[] = {2, 2};
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 7200;
    };

    //--------------------------------------------------------------------------
    // Hefeteig (§10). Grundlage des Brots.
    //--------------------------------------------------------------------------
    class ChefZ_YeastDough : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_YEASTDOUGH";
        descriptionShort = "#STR_CHEFZ_YEASTDOUGH_DESC";
        model = "\dz\gear\food\lard.p3d";
        weight = 500;
        itemSize[] = {2, 2};
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 7200;
    };

    //--------------------------------------------------------------------------
    // Nudelteig (§10, §11).
    //--------------------------------------------------------------------------
    class ChefZ_PastaDough : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PASTADOUGH";
        descriptionShort = "#STR_CHEFZ_PASTADOUGH_DESC";
        model = "\dz\gear\food\lard.p3d";
        weight = 480;
        itemSize[] = {2, 2};
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 7200;
    };

    //--------------------------------------------------------------------------
    // Frische Nudeln (§11). Kurze Haltbarkeit, direkt kochbar.
    //
    // PROXY: Rice.p3d. Eigenes Mesh gemeldet (U, P1).
    //--------------------------------------------------------------------------
    class ChefZ_RawPasta : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_RAWPASTA";
        descriptionShort = "#STR_CHEFZ_RAWPASTA_DESC";
        model = "\dz\gear\food\Rice.p3d";
        weight = 400;
        itemSize[] = {2, 2};
        stackedUnit = "grams";
        quantityBar = 1;
        varQuantityInit = 500;
        varQuantityMin = 0;
        varQuantityMax = 500;
        varQuantityDestroyOnMin = 1;
        canBeSplit = 1;
        lifetime = 10800;
    };

    //--------------------------------------------------------------------------
    // Trockennudeln (§11, §56). Der Vorratsartikel der Kette.
    //
    // PROXY: Rice.p3d, Texturvariante spaeter (§71). Mesh gemeldet (S, P1).
    //--------------------------------------------------------------------------
    class ChefZ_DriedPasta : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_DRIEDPASTA";
        descriptionShort = "#STR_CHEFZ_DRIEDPASTA_DESC";
        model = "\dz\gear\food\Rice.p3d";
        weight = 350;
        itemSize[] = {2, 2};
        stackedUnit = "grams";
        quantityBar = 1;
        varQuantityInit = 500;
        varQuantityMin = 0;
        varQuantityMax = 500;
        varQuantityDestroyOnMin = 1;
        canBeSplit = 1;
        lifetime = 172800;
    };

    //--------------------------------------------------------------------------
    // Brot (§12). Ergebnis des Rezepts REC_ChefZ_Bread am Kochgeraet.
    //
    // PROXY: BoxCereal.p3d - ein Laib in der richtigen Groessenordnung.
    // Eigenes Mesh gemeldet (U, P1).
    //--------------------------------------------------------------------------
    class ChefZ_Bread : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_BREAD";
        descriptionShort = "#STR_CHEFZ_BREAD_DESC";
        model = "\dz\gear\food\BoxCereal.p3d";
        weight = 700;
        itemSize[] = {3, 2};
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 21600;
    };

    //--------------------------------------------------------------------------
    // Fladenbrot (§12). Der Survival-Weg ohne Hefe und ohne Ofen.
    //
    // PROXY: pumpkin_sliced.p3d - eine flache Scheibe. Mesh gemeldet (U, P2).
    //--------------------------------------------------------------------------
    class ChefZ_Flatbread : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_FLATBREAD";
        descriptionShort = "#STR_CHEFZ_FLATBREAD_DESC";
        model = "\dz\gear\food\pumpkin_sliced.p3d";
        weight = 300;
        itemSize[] = {2, 2};
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 14400;
    };
};

//------------------------------------------------------------------------------
// Modulanmeldung am Config Manager (Entwurf 02 §4).
//
// handcraftRecipeSlots = 4 - genau die vier HANDCRAFT-Transforms aus
// Config/GrainTransforms.json:
//
//   TR_FlourWaterToSimpleDough
//   TR_SimpleDoughYeastToYeastDough
//   TR_SimpleDoughToPastaDough
//   TR_PastaDoughToRawPasta
//
// Die Zahl MUSS hier stehen und muss stimmen (02 §4.2): Vanilla vergibt
// Rezept-IDs als POSITION in PluginRecipesManager.m_RecipeList, und die
// Positionen entstehen im MissionBase-Konstruktor - lange bevor ChefZ Daten
// gelesen hat. Zu wenige Plaetze heisst: die ueberzaehligen Transforms werden
// mit einer Fehlerzeile abgewiesen, nicht nachtraeglich eingetragen.
//
// TR_RawPastaToDriedPasta zaehlt NICHT mit: er laeuft ueber PROCESS_DRY und
// damit ueber eine Station, nicht ueber Vanillas Craftsystem.
//------------------------------------------------------------------------------
class CfgChefZ
{
    // ### SLICE grain ### Ein Knoten je SLICE (02 §4), nicht je Modul - er
    // heisst deshalb nicht wie das Addon. Ein gleichnamiger Knoten neben dem
    // CfgMods-Eintrag zaehlte fuer configcpp.mjs als doppelte
    // Klassendefinition.
    //
    // handcraftRecipeSlots = 4 - genau die vier HANDCRAFT-Transforms aus
    // Config/GrainTransforms.json:
    //
    //   TR_FlourWaterToSimpleDough
    //   TR_SimpleDoughYeastToYeastDough
    //   TR_SimpleDoughToPastaDough
    //   TR_PastaDoughToRawPasta
    //
    // Die Zahl MUSS hier stehen und muss stimmen (02 §4.2): Vanilla vergibt
    // Rezept-IDs als POSITION in PluginRecipesManager.m_RecipeList, und die
    // Positionen entstehen im MissionBase-Konstruktor - lange bevor ChefZ
    // Daten gelesen hat. Zu wenige Plaetze heisst: die ueberzaehligen
    // Transforms werden mit einer Fehlerzeile abgewiesen, nicht nachtraeglich
    // eingetragen.
    //
    // TR_RawPastaToDriedPasta zaehlt NICHT mit: er laeuft ueber PROCESS_DRY
    // und damit ueber eine Station, nicht ueber Vanillas Craftsystem.
    //
    // Die ZUTATENBINDUNGEN liegen in Rang 2 (Config/GrainIngredients.json):
    // 02 §4 verlangt "Knotenname == Klassenname", und ein gleichnamiger Knoten
    // neben der CfgVehicles-Klasse zaehlt fuer configcpp.mjs als doppelte
    // Klassendefinition. 02 §2 laesst fuer Item-Bindings ausdruecklich
    // "Game-Config ODER JSON" zu.
    class ChefZ_GrainBaking
    {
        chefzApiVersion = 1;
        loadOrder = 230;
        handcraftRecipeSlots = 4;
        dataFiles[] =
        {
            "ChefZ_Baking/Config/GrainProcesses.json",
            "ChefZ_Baking/Config/GrainIngredients.json",
            "ChefZ_Baking/Config/GrainTransforms.json",
            "ChefZ_Baking/Config/GrainRecipes.json"
        };
    };
};
