// ChefZ_Baking - Teig, Pasta, Brot (Slice "grain").
//
// Quelle: Production Map §10 (Teig), §11 (Pasta), §12 (Brot),
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
// EINE Teigart, keine Hefe (Entscheidung vom 29.08.2026)
// ---------------------------------------------------------------------------
// Production Map §9/§10 kannten Hefe, einfachen Teig, Hefeteig und Nudelteig.
// Das ist auf EINEN Teig vereinfacht:
//
//   Flour + Water            -> ChefZ_Dough           (PROCESS_KNEAD)
//   ChefZ_Dough              -> Brot, Fladenbrot      (Rezepte, je nach Geraet)
//   ChefZ_Dough + Nudelmaschine -> ChefZ_RawPasta     (PROCESS_ROLL)
//   ChefZ_Dough              -> Kaesefladen, Teigtaschen (Kategorie DOUGH)
//
// Was das Geraet entscheidet: in der Pfanne wird aus dem Teig Fladenbrot, im
// Topf oder Ofen ein Brot. Hefe, Hefeteig und Nudelteig gibt es als Klassen
// nicht mehr.
//
// ---------------------------------------------------------------------------
// Nahrungsdaten
// ---------------------------------------------------------------------------
// Alle essbaren Klassen erben Nutrition und Food von ChefZ_GrainFoodBase aus
// ChefZ_Farming. Der Config-Knoten ist die Anmeldung an Vanillas Magen
// (01 V7), die stueckgenauen Zahlen stehen im Nutrition-Delta des Slices
// (_deltas/grain.json, Entwurf 13 §4).
//
// Damit traegt auch ChefZ_Dough FoodStageTransitions - und das ist hier keine
// Formalie: der Teig ist Zutat eines Rezepts, liegt also im Kochgeraet,
// waehrend Vanilla den Garzustand fortschreibt. Ohne Uebergaenge
// faellt FoodStage.GetNextFoodStageType auf BURNED zurueck (01 V4,
// FoodStage.c:472) - der Spieler legte Teig in die Pfanne und bekaeme Kohle.
//
// ---------------------------------------------------------------------------
// 3D
// ---------------------------------------------------------------------------
// Jede Klasse traegt ein Vanilla-Proxy-Modell; der Bedarf ist im
// Slice-Bericht gemeldet.

class CfgPatches
{
    class ChefZ_Baking
    {
        units[] =
        {
            "ChefZ_Dough", "ChefZ_RawPasta", "ChefZ_DriedPasta", "ChefZ_Bread", "ChefZ_Flatbread"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // ChefZ_Core:       ChefZ_Edible_Base.
        // ChefZ_Farming:    ChefZ_GrainFoodBase (Nahrungsbasis).
        // ChefZ_Processing: ChefZ_Flour als Eingang und die Werkzeuggruppe
        //                   ROLLING_PIN, auf die PROCESS_ROLL zeigt.
        // DZ_Gear_Food:     die Proxy-Modelle.
        requiredAddons[] = {"DZ_Data", "DZ_Gear_Food", "ChefZ_Core", "ChefZ_Farming", "ChefZ_Processing", "ChefZ_Food"};
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
    // DER Teig (§10): Mehl + Wasser. Brot, Fladenbrot, Nudeln, Teigtaschen -
    // alles aus diesem einen Item (Kopf dieser Datei).
    //
    // PROXY: lard.p3d - ein heller Klumpen. Eigenes Mesh gemeldet (S, P2).
    //--------------------------------------------------------------------------
    class ChefZ_Dough : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_DOUGH";
        descriptionShort = "#STR_CHEFZ_DOUGH_DESC";
        model = "\dz\gear\food\lard.p3d";
        weight = 450;
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
        model = "\ChefZ\ChefZ_Food\models\bread.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        weight = 700;
        itemSize[] = {3, 2};
        varQuantityInit = 1;
        varQuantityMin = 0;
        varQuantityMax = 1;
        lifetime = 21600;
    };

    //--------------------------------------------------------------------------
    // Fladenbrot (§12). Derselbe Teig wie beim Brot, nur in der Pfanne.
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
// handcraftRecipeSlots = 2 - genau die zwei HANDCRAFT-Transforms aus
// Config/GrainTransforms.json:
//
//   TR_FlourWaterToDough
//   TR_DoughToRawPasta
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
    // handcraftRecipeSlots = 2 - genau die zwei HANDCRAFT-Transforms aus
    // Config/GrainTransforms.json:
    //
    //   TR_FlourWaterToDough
    //   TR_DoughToRawPasta
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
        handcraftRecipeSlots = 2;
        dataFiles[] =
        {
            "ChefZ_Baking/Config/GrainProcesses.json",
            "ChefZ_Baking/Config/GrainIngredients.json",
            "ChefZ_Baking/Config/GrainTransforms.json",
            "ChefZ_Baking/Config/GrainRecipes.json"
        };
    };
};
