// ChefZ_Core - Systemschicht des Mods Psyerns_ChefZ.
//
// Diese Datei enthaelt ausschliesslich die Modulanmeldung. Kein Item, keine
// Zutat, kein Gericht, keine Station - Content lebt in eigenen Addons und
// meldet sich dort ueber CfgChefZ an (Entwurf 02 §4).
//
// PBO-Praefix: $PREFIX$ enthaelt "ChefZ_Core". Die Wurzel jedes Pfades in
// files[] MUSS damit uebereinstimmen, sonst ueberspringt DayZ die
// Skriptmodule still und ohne RPT-Eintrag (README "Packing - PBO Prefix").
//
// PFADWURZEL, verbindlich (B4, Entwurf 02 §4.1): die Wurzel eines
// Laufzeitpfades ist das PBO-Praefix, und das PBO-Praefix ist der ORDNERNAME
// des Addons. Also "ChefZ_Core/..." und - fuer ein Content-Modul -
// "ChefZ_Meat/...". Es gibt keine zweite Form. Das gilt fuer files[] hier
// genauso wie fuer jeden dataFiles[]-Eintrag in CfgChefZ; es ist derselbe
// Adressraum.
//
// HANDWERKS-REZEPTPLAETZE: ein Content-Modul, das Transforms mit
// exec = "HANDCRAFT" mitbringt, deklariert in SEINEM CfgChefZ-Knoten
// zusaetzlich "handcraftRecipeSlots = <Anzahl>". Der Core selbst bringt keine
// mit und deklariert deshalb keinen CfgChefZ-Knoten - er reserviert null
// Plaetze und laesst Vanillas Rezeptliste um kein Bit veraendert. Warum die
// Zahl ueberhaupt vorab feststehen muss, steht im Kopf von
// Scripts/4_World/ChefZ/Processing/ChefZ_HandcraftBridge.c.

class CfgPatches
{
    class ChefZ_Core
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data"};
    };
};

class CfgMods
{
    class ChefZ_Core
    {
        dir = "ChefZ_Core";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Core";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        // "Core" ist seit S1 noetig: ChefZ_Log, ChefZ_SymbolTable,
        // ChefZ_IdentityMap, ChefZ_Range und ChefZ_Undefined liegen laut
        // Entwurf 00 §4 in 1_Core - rein datenverarbeitend, kein Engine-Typ,
        // und damit von Client und Server identisch nutzbar.
        dependencies[] = {"Core", "Game", "World", "Mission"};

        class defs
        {
            // Seit S1: reine Datenverarbeitung, kein Engine-Typ.
            class engineScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Core/Scripts/1_Core"
                };
            };

            // Seit S2: Config Manager, Quellen, Registries. Hier liegen
            // JsonFileLoader und g_Game.ConfigGet* (Entwurf 00 §4) - und
            // ausdruecklich kein ItemBase, kein EntityAI.
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Core/Scripts/3_Game"
                };
            };

            // Seit S4: ChefZ_FactCollector - die Stelle des Core, die Entities
            // in Fakten uebersetzt, und ausschliesslich lesend (Entwurf 05
            // §3.4). Hier leben ItemBase, Edible_Base, CargoBase und Liquid;
            // in 3_Game gibt es sie nicht.
            //
            // Seit S7 zusaetzlich Cooking/: der Kochadapter und die EINZIGE
            // modded class des Core auf einer Vanilla-Spielklasse
            // (modded class Cooking, Entwurf 10 §3 / E2). Sie ruft super als
            // erste Anweisung und gibt dessen Rueckgabewert unveraendert
            // zurueck; ChefZ ist dort rein beobachtend und hat bis S8
            // ueberhaupt keinen Code, der ein Item veraendern koennte.
            //
            // Seit S14 zusaetzlich Processing/: die abstrakte
            // ChefZ_ProcessingStation_Base, die EINE generische
            // ChefZ_ActionProcessAtStation (Entwurf 11 E1) und der
            // ChefZ_ProcessRunner. Auch hier gilt: es ist eine BASIS und
            // KEIN Item - der Core deklariert bewusst keinen
            // CfgVehicles-Eintrag fuer eine Station (Invariante I3). Ein
            // Content-Modul leitet seine Configklasse von einer
            // Vanilla-Klasse ab, seine Skriptklasse von
            // ChefZ_ProcessingStation_Base, und meldet die angebotenen
            // Prozesse ueber CfgChefZStations an.
            //
            // Die Station fasst Vanillas Kochkette an KEINER Stelle an
            // (11 E6): Vanilla-Raeuchern in den Smoking-Slots eines Fasses
            // bleibt exakt wie es ist.
            //
            // Seit S15 zusaetzlich in Processing/: die Handcraft-Bruecke -
            // ChefZ_GenericCraftRecipe (GENAU EINE aus Daten parametrisierte
            // RecipeBase-Ableitung, Entwurf 11 E3), ChefZ_HandcraftBridge und
            // modded class PluginRecipesManagerBase (Entwurf 00 §5, Zeile 2).
            // Sie ruft super.RegisterRecipies() als erste Anweisung und fuegt
            // danach ausschliesslich HINZU; UnregisterRecipe kommt im gesamten
            // Core nicht vor. Vanillas Rezeptliste und die IDs ihrer Rezepte
            // bleiben damit unter allen Umstaenden unveraendert.
            //
            // Dazu ChefZ_ModdedWorldCraft: modded class ActionWorldCraft und
            // ihre beiden Datenhalter. Vanillas Craftaktion uebertraegt die
            // POSITION eines Rezepts, nicht seine Identitaet; diese Datei legt
            // eine positionsunabhaengige Kennung daneben und laesst den Server
            // eine Aktion VERWEIGERN, deren Position auf den beiden Seiten
            // Verschiedenes bedeutet. Alle drei Overrides rufen super als
            // erste Anweisung, ein fehlgeschlagenes Lesen gibt nie false
            // zurueck, und ohne Widerspruch geschieht nichts. Die
            // vollstaendige Begruendung steht im Kopf der Datei.
            //
            // Auch hier: KEIN Content. Der Core registriert kein einziges
            // Rezept aus eigenem Antrieb - jedes entsteht aus einem
            // HANDCRAFT-Transform, den ein Content-Modul beisteuert.
            //
            // Seit S9 zusaetzlich State/: ChefZ_Edible_Base, ChefZ_Item_Base,
            // der Zustandsblock und der Klassentausch (Entwurf 06 §4.3/§4.4).
            // Es sind ABLEITUNGEN, kein "modded class Edible_Base" (06 §2) -
            // der ChefZ-Zustand lebt ausschliesslich auf ChefZ-eigenen
            // Klassen, und Vanilla-Nahrung bleibt dadurch unangetastet.
            //
            // Weiterhin KEIN Item: der Core deklariert auch fuer diese beiden
            // Basisklassen bewusst keinen CfgVehicles-Eintrag. Ein
            // Content-Modul leitet seine Configklasse von einer Vanilla-Klasse
            // ab und seine Skriptklasse von ChefZ_Edible_Base bzw.
            // ChefZ_Item_Base; die Begruendung steht im Kopf von
            // Scripts/4_World/ChefZ/State/ChefZ_Edible_Base.c.
            //
            // Seit S16 zusaetzlich Portion/: ChefZ_PortionedFood_Base - das
            // Bulk-Gericht mit eigenem int-Zaehler statt Vanilla-quantity
            // (Entwurf 15 E2, belegt in 01 V5) - und die EINE generische
            // ChefZ_ActionTakePortion (Entwurf 15 E5). Auch hier: eine BASIS
            // und KEIN Item, kein CfgVehicles-Eintrag, kein Gericht.
            //
            // Die Aktion ist ADDITIV und haengt ausschliesslich an
            // ChefZ_PortionedFood_Base.SetActions() - sie erscheint an keiner
            // Vanilla-Klasse und veraendert keine Vanilla-Aktion. Der
            // Portionszaehler ist bewusst NICHT Vanillas quantity: Vanillas
            // Kochlogik zieht quantity bei jedem Garstufenwechsel ab, ein
            // Zaehler darauf verloere Portionen durch blosses Warmhalten.
            //
            // Seit S17 zusaetzlich Container/: ChefZ_Container_Base - eine
            // OPTIONALE Basis, von der ein Behaelter erben KANN, aber nicht
            // muss - und der ChefZ_ContainerService, der sucht, verbraucht
            // und zurueckgibt. Auch hier: kein Item, kein CfgVehicles-Eintrag,
            // kein Teller. Welche Klasse ein Behaelter ist, steht
            // ausschliesslich in CfgChefZContainers und damit im Content
            // (Entwurf 16 E2).
            //
            // Diese Dateien fassen Vanillas Kochkette an KEINER Stelle an. Ein
            // Behaelter wird beim SERVIEREN verlangt, nie beim Kochen
            // (Entwurf 16 §2 / E1) - und das ist technisch erzwungen, nicht
            // gewaehlt: Cooking.ProcessItemToCook beschaedigt jedes Cargo-Item,
            // das nicht IsCookware() ist, ueber PARAM_BURN_DAMAGE_COEF. Ein
            // Teller im Topf ginge im Feuer kaputt.
            //
            // Der einzige Eingriff ist ein OnConsume auf der ChefZ-EIGENEN
            // ChefZ_Edible_Base (kein modded class): super laeuft als erste
            // Anweisung, danach kommt der leere Behaelter zurueck - und zwar
            // nur bei Quantity <= 0, nie bei jedem Bissen und nie an EEDelete
            // (Entwurf 16 E4). Vanilla-Nahrung erreicht diesen Code nie.
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Core/Scripts/4_World"
                };
            };

            // Seit S18 zusaetzlich Diagnostics/: ChefZ_Diagnostics, die
            // "chefz ..."-Adminkommandos und ihr Selbsttest (Entwurf 18 §2.4).
            //
            // Ausschliesslich Auskunft. Nichts davon erzeugt ein Item,
            // verbraucht eines oder oeffnet eine Kochsitzung; "chefz match"
            // wertet aus und wendet nie an (Entwurf 18 E6). Der Core bringt
            // dafuer bewusst KEINE Oberflaeche mit - eine Adminoberflaeche
            // waere Content -, und er oeffnet auch keinen RPC: 17 E8 legt
            // fest, dass der Core keinen eigenen hat.
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Core/Scripts/5_Mission",

                    // TEMPORAER (V-A Rauchtest). Faellt zusammen mit
                    // Tests/V_A_PboJsonSmoke/ weg, sobald das Ergebnis im
                    // Gate-1-Protokoll steht. Beim Entfernen auch die beiden
                    // markierten Aufrufe in ChefZ_CoreEntry.c loeschen.
                    "ChefZ_Core/Tests/V_A_PboJsonSmoke/Scripts/5_Mission"
                };
            };
        };
    };
};
