//==============================================================================
// ChefZ_Registry - das Ergebnis des Delta-Merges, und sonst nichts.
//
// Eigentuemer: chefz-registry-integrator. Kein anderer Agent schreibt hier.
// Erzeugt aus Psyerns_ChefZ_Core/_deltas/*.json (Workflow §5).
//
// ---------------------------------------------------------------------------
// WARUM DIESES ADDON UEBERHAUPT EXISTIERT
// ---------------------------------------------------------------------------
// In Meilenstein 2 lagen die zusammengefuehrten Registries in
// ChefZ_Core/Config/. Das war aus zwei Gruenden falsch:
//
//   1. Der Core ist eine Regelmaschine ohne eigenes Vokabular (Invariante I3).
//      Kategorien, Tags, Prozesse und Naehrwerte SIND Vokabular. Sie im Core
//      abzulegen widerspricht der in Meilenstein 1 gewaehlten Architektur.
//   2. Niemand durfte sie in einem dataFiles[] deklarieren - und ohne diesen
//      Eintrag liest der Core sie nie. Es waren tote Dateien.
//
// Beides ist hier geheilt: ein eigenes Addon, eine eigene CfgChefZ-Anmeldung,
// ein dataFiles[]-Eintrag je Registry.
//
// ---------------------------------------------------------------------------
// KEINE ITEM-KLASSEN
// ---------------------------------------------------------------------------
// Dieses Addon definiert kein CfgVehicles, kein Skript und kein Modell. Es
// liefert ausschliesslich Datensaetze. units[] ist deshalb leer, und das ist
// kein vergessener Eintrag.
//
// ---------------------------------------------------------------------------
// PFADWURZEL
// ---------------------------------------------------------------------------
// $PREFIX$ enthaelt "ChefZ_Registry". Die Wurzel jedes Laufzeitpfades - auch
// jedes dataFiles[]-Eintrags - ist dieser Ordnername (Entwurf 02 §4.1, B4).
//==============================================================================

class CfgPatches
{
    class ChefZ_Registry
    {
        // Keine Item-Klassen: dieses Addon liefert Datensaetze, keine Objekte.
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;

        // ChefZ_Core:        liest die Registries (ChefZ_JsonDocs, Rang 2).
        // ChefZ_Farming:     Rang-1-Prozess PROCESS_CUT_OUT_SEEDS, den ein
        //                    Registryrecord feldweise patcht.
        // ChefZ_Ingredients: Rang-1-Prozess PROCESS_CHOP_VEGETABLE und die
        //                    Werkzeuggruppe CUTTING_TOOL.
        // ChefZ_Processing:  Rang-1-Prozesse, Station-Records und die
        //                    Werkzeuggruppen, auf die toolGroups[] zeigt.
        // ChefZ_Meat, ChefZ_Baking: die Klassen, die Nutrition-Records nennen.
        //
        // Alle fuenf sind ECHTE Abhaengigkeiten der DATEN, nicht Kosmetik: ein
        // Patch auf einen Rang-1-Record braucht den Record, und ein
        // toolGroups[]-Verweis braucht die Gruppe. Keines der genannten Module
        // haengt umgekehrt von ChefZ_Registry ab - es gibt also keinen Zyklus.
        requiredAddons[] =
        {
            "DZ_Data",
            "ChefZ_Core",
            "ChefZ_Farming",
            "ChefZ_Ingredients",
            "ChefZ_Processing",
            "ChefZ_Meat",
            "ChefZ_Baking"
        };
    };
};

//------------------------------------------------------------------------------
// Anmeldung beim Core (02 §4).
//
// Der Knoten heisst ChefZ_MergedRegistry und NICHT wie das Addon: ein
// gleichnamiger Knoten neben dem CfgPatches-Eintrag zaehlt fuer configcpp.mjs
// als doppelte Klassendefinition, und dieselbe Regel gilt in ChefZ_Farming
// (Kommentar an dessen CfgChefZ-Knoten).
//
// loadOrder = 150 - vor jedem Content-Slice (der frueheste ist ChefZ_Processing
// mit 155). Begruendung: dieses Addon traegt das GETEILTE Vokabular. Es zuerst
// zu lesen heisst, dass jeder Slice danach gegen einen bereits vollstaendigen
// Kategorien- und Tagbestand geprueft wird - und dass eine spaetere, ABWEICHENDE
// Zweitdefinition derselben ID im selben Rang als Fehler auffaellt (02 §8,
// "erste gewinnt"), statt die gemergte Fassung still zu verdraengen.
//
// handcraftRecipeSlots = 0: die Registry bringt keinen einzigen Transform mit,
// also auch keinen mit exec = "HANDCRAFT", und reserviert nichts in Vanillas
// Rezeptliste.
//------------------------------------------------------------------------------
class CfgChefZ
{
    class ChefZ_MergedRegistry
    {
        chefzApiVersion = 1;
        loadOrder = 150;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Registry/Config/Categories.json",
            "ChefZ_Registry/Config/Tags.json",
            "ChefZ_Registry/Config/Nutrition.json",
            "ChefZ_Registry/Config/Preservation.json",
            "ChefZ_Registry/Config/Processing.json"
        };
    };
};
