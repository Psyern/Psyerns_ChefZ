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
        // ChefZ_Farming:     ChefZ_Onion, ChefZ_Carrot, ChefZ_Cabbage,
        //                    ChefZ_Garlic und die Frischkraeuter.
        // ChefZ_Ingredients: ChefZ_SlicedPotato, die Chopped*-Klassen,
        //                    ChefZ_Salt und ChefZ_RawSalt.
        // ChefZ_Processing:  ChefZ_Flour, die Teige, die Pasta.
        // ChefZ_Meat:        ChefZ_DicedMeat, die Minced*- und Sausage-Klassen.
        // ChefZ_Baking:      ChefZ_Bread, ChefZ_Flatbread.
        //
        // Alle fuenf sind ECHTE Abhaengigkeiten der DATEN, nicht Kosmetik:
        // jeder Nutrition-Record nennt eine Klasse, und ein Record ohne seine
        // Klasse ist ein Naehrwert fuer nichts. Keines der genannten Module
        // haengt umgekehrt von ChefZ_Registry ab - es gibt also keinen Zyklus.
        //
        // Bis S19 stand hier eine andere Begruendung: Rang-1-Prozesse, die ein
        // Registryrecord feldweise patcht. Die ist mit K1 entfallen - die
        // Registry fuehrt keine Prozesse mehr (siehe dataFiles[] unten). Die
        // fuenf Eintraege bleiben trotzdem noetig, jetzt aus dem
        // Nutrition-Grund.
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
//
// KEINE PROZESSE (K1, entschieden nach S19).
//
// dataFiles[] nennt vier Registries, nicht fuenf. Processing.json ist weg, und
// das ist kein vergessener Eintrag: Prozess-Records gehoeren den Slices, die
// sie ohnehin autoritativ deklarieren. Die Registry haette sie im SELBEN Rang
// ein zweites Mal eingebracht, und ChefZ_RecordSink weist einen doppelten
// Record desselben Rangs ab, statt ihn zu patchen - PROCESS_MILL,
// PROCESS_KNEAD und PROCESS_ROLL waren so gar nicht schreibbar. Nicht die
// Registry und nicht die Slices waren das Problem, sondern die Mischung aus
// beidem.
//
// Die Kollisionspruefung ueber Prozess-IDs bleibt Aufgabe des Integrators
// (tools/chefz-validate/deltas.mjs, Abschnitt 1 und 3). Sie ergibt nur keine
// Datei mehr - ihr Ergebnis ist ein Bericht, kein Datensatz.
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
            "ChefZ_Registry/Config/Preservation.json"
        };
    };
};
