//==============================================================================
// ChefZ_Cookbook - Wissensstand und Kochbuch als Gegenstand
//
// Meilenstein 5.1. Entwurf: ChefZ_Docs/ChefZ_Cookbook_Workflow.md,
// Entscheidungen: ChefZ_Docs/design/M5_Entscheidungen.md.
//
// ---------------------------------------------------------------------------
// WARUM DIESES ADDON KEINE OBERFLAECHE ENTHAELT
// ---------------------------------------------------------------------------
// Es haengt an nichts ausser DayZ und ChefZ_Core. Wer kein Dabs Framework
// installiert hat, bekommt trotzdem alles, was zaehlt: der Server merkt sich,
// welche Zutaten ein Charakter in der Hand hatte und welche Rezepte er gekocht
// hat, und dieser Stand ueberlebt den Relog.
//
// Nur das Blaettern fehlt. Es liegt in ChefZ_Cookbook_UI, und das darf
// wegbleiben, ohne dass hier etwas kaputtgeht (Workflow §3).
//
// ---------------------------------------------------------------------------
// WARUM CfgChefZCookbook HIER STEHT UND NICHT IM CORE
// ---------------------------------------------------------------------------
// Regel 2 des Projekts: der Core enthaelt Systeme, niemals Content. Die
// Schwelle, ab wann ein Rezept als "teilweise bekannt" gilt, ist eine
// Balancing-Entscheidung dieses Meilensteins und kein Systemparameter. Sie
// gehoert deshalb in die Config DIESES Moduls - der Core erfaehrt nie davon.
//==============================================================================

class CfgPatches
{
    class ChefZ_Cookbook
    {
        units[] =
        {
            "ChefZ_CookbookItem"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // Genau vier Eintraege, und jeder wird tatsaechlich gebraucht:
        //   DZ_Data          Grundlage von allem
        //   DZ_Gear_Books    liefert book_kniga.p3d als Proxy-Modell und die
        //                    zugehoerige Textur (Asset-Backlog: eigene
        //                    Geometrie fuer das Kochbuch steht dort offen)
        //   DZ_Characters    definiert SurvivorBase mit attachments[] und
        //                    InventoryEquipment.playerSlots[]. Unser "+=" auf
        //                    beide (Kochbuch-Platz, siehe CfgVehicles) greift
        //                    nur, wenn Vanilla vorher geladen ist.
        //   ChefZ_Core       ChefZ_Sym, ChefZ_SymbolTable, ChefZ_EventBus,
        //                    ChefZ_RecipeRegistry, ChefZ_Log
        //
        // Dabs steht NICHT hier - siehe Kopf. Und kein Content-Modul steht
        // hier: dieses Addon kennt kein einziges Rezept und keine einzige
        // Zutat namentlich, es fragt zur Laufzeit die Registry des Core.
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Gear_Books",
            "DZ_Characters",
            "ChefZ_Core"
        };
    };
};

//==============================================================================
// Der Kochbuch-Platz am Spieler
//
// Ein eigener Anbauplatz, in den ausschliesslich ChefZ_CookbookItem passt
// (inventorySlot[] unten). Sobald ein Spieler das Buch bekommt, legt der Server
// es dort ab (ChefZ_CookbookItem.c, "Kochbuch-Platz") - so liegt es nie im
// Rucksack herum und blockiert keine 2x2 Felder.
//
// ghostIcon: "book" existiert im Vanilla-Imageset
// (gui/imagesets/dayz_inventory.imageset, Name "book") und wird von Vanillas
// eigenem Slot_Book benutzt (scripts/config.cpp).
//==============================================================================
class CfgSlots
{
    class Slot_ChefZ_Cookbook
    {
        name = "ChefZ_Cookbook";
        displayName = "#STR_CHEFZ_SLOT_COOKBOOK";
        ghostIcon = "set:dayz_inventory image:book";
        stackMax = 1;
    };
};

class CfgMods
{
    class ChefZ_Cookbook
    {
        dir = "ChefZ_Cookbook";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Cookbook";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        // ------------------------------------------------------------------
        // Die Tastenbelegung - datengetrieben, weil es anders nicht geht
        // ------------------------------------------------------------------
        // GetUApi().RegisterInput() steht als proto native in
        // 3_Game/DayZ/InputAPI/UAInput.c:194 und hat in den Vanilla-Skripten
        // 1.29 wie in DayZExpansion NULL Aufrufer. Es meldet keinen Fehler, es
        // tut nur nichts. Der einzige Weg, der im Feld traegt, ist diese Zeile
        // plus Scripts/Data/Inputs.xml (Workflow §6.3).
        //
        // Der Pfad ist PBO-relativ und beginnt deshalb mit dem Inhalt von
        // $PREFIX$ ("ChefZ_Cookbook") - nicht mit "Addons/" und nicht mit dem
        // Ordnernamen auf der Platte. Vorbild:
        // DayZExpansion/Book/Scripts/config.cpp:20 gegen dessen Prefix
        // "DayZExpansion/Book".
        //
        // Fehlerbild bei falschem Pfad: die Gruppe "ChefZ" fehlt im
        // Steuerungsmenue, das RPT schweigt.
        inputs = "ChefZ_Cookbook/Scripts/Data/Inputs.xml";

        // Nur die Ebenen, die auch Dateien haben. Eine gelistete Ebene, die zu
        // nichts kompiliert, beendet den Server ohne Meldung - am 27.08.2026
        // zweimal nachgestellt, siehe die Anker-Klassen der Comp-Module.
        //
        // "Mission" kam mit der Tastenabfrage dazu: der Zustand einer Taste
        // laesst sich nur im Bildlauf der Mission lesen, und OnUpdate gibt es
        // erst in MissionGameplay (5_Mission). Das Verzeichnis ist nicht leer -
        // ChefZ_CookbookInput.c liegt darin.
        dependencies[] = {"Game", "World", "Mission"};

        class defs
        {
            // 3_Game: Ableitung und Datenhaltung. Kennt weder PlayerBase noch
            // Widgets und laesst sich deshalb pruefen, ohne dass eine Welt
            // laeuft.
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Cookbook/Scripts/3_Game"
                };
            };

            // 4_World: alles, was einen Spieler, ein Inventar oder eine Aktion
            // anfasst.
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Cookbook/Scripts/4_World"
                };
            };

            // 5_Mission: ausschliesslich die Tastenabfrage. Sie braucht den
            // Bildlauf der Mission (MissionGameplay.OnUpdate) und damit die
            // einzige Ebene, auf der es ihn gibt.
            //
            // Seit 27.10.2026 auch die Oberflaeche selbst
            // (ChefZ_CookbookMenu.c, GUI/layouts/chefz_cookbook.layout): ein
            // Vanilla-UIScriptedMenu, kein Dabs - das geplante Addon
            // ChefZ_Cookbook_UI gab es nie.
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Cookbook/Scripts/5_Mission"
                };
            };
        };
    };
};

class CfgVehicles
{
    class Inventory_Base;
    class Man;

    //==========================================================================
    // Den Kochbuch-Platz am Spieler anmelden
    //
    // attachments[]  macht den Platz fuer die Engine real (FindAttachment,
    //                CanAddAttachmentEx, FindFreeLocationFor).
    // playerSlots[]  macht ihn im Inventar sichtbar: Vanillas PlayerContainer
    //                baut seine Platzreihe ausschliesslich aus
    //                "CfgVehicles SurvivorBase InventoryEquipment playerSlots"
    //                (1.30 scripts/5_Mission/DayZ/GUI/InventoryNew/Inherited/
    //                PlayerContainer.c:58 und :682). Ein Platz, der nur in
    //                attachments[] steht, funktioniert, bleibt aber unsichtbar
    //                - so wie Vanillas "Splint_Right".
    //
    // Beides mit "+=", damit andere Mods, die ebenfalls Spielerplaetze
    // anhaengen, nichts verlieren. Muster: A6_SecureContainers
    // (assets/config.cpp, Slot_SecureContainer) - dieselben zwei Zeilen.
    //==========================================================================
    class SurvivorBase : Man
    {
        attachments[] +=
        {
            "ChefZ_Cookbook"
        };
        class InventoryEquipment
        {
            playerSlots[] +=
            {
                "Slot_ChefZ_Cookbook"
            };
        };
    };

    //==========================================================================
    // Das Kochbuch
    //
    // Ein Gegenstand ohne Mechanik. Er entscheidet nur, OB das Menue aufgeht -
    // was darin steht, haengt am Charakter und nicht am Papier
    // (ChefZ_CookbookItem.c, Kopfkommentar).
    //
    // Modell und Textur sind Vanilla-Platzhalter nach dem Muster jedes anderen
    // ChefZ-Moduls. Der Eintrag im Asset-Backlog steht.
    //==========================================================================
    class ChefZ_CookbookItem : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_BOOK_COOKING";
        descriptionShort = "#STR_CHEFZ_BOOK_COOKING_DESC";
        model = "\dz\gear\books\book_kniga.p3d";
        rotationFlags = 1;
        weight = 400;
        itemSize[] = {2, 2};
        // Der eigene Platz am Spieler (CfgSlots oben). Cargo bleibt erlaubt -
        // inventorySlot schraenkt nur ein, in welche Anbauplaetze es passt.
        inventorySlot[] = {"ChefZ_Cookbook"};
        hiddenSelections[] = {"camoGround"};
        hiddenSelectionsTextures[] = {"dz\gear\books\data\book_TheWarOfTheWorlds_co.paa"};
        varQuantityDestroyOnMin = 0;

        class DamageSystem
        {
            class GlobalHealth
            {
                class Health
                {
                    hitpoints = 50;
                    healthLevels[] =
                    {
                        {1.0,  {"DZ\gear\books\data\book_TheWarOfTheWorlds.rvmat"}},
                        {0.7,  {"DZ\gear\books\data\book_TheWarOfTheWorlds.rvmat"}},
                        {0.5,  {"DZ\gear\books\data\book_TheWarOfTheWorlds_damage.rvmat"}},
                        {0.3,  {"DZ\gear\books\data\book_TheWarOfTheWorlds_damage.rvmat"}},
                        {0.0,  {"DZ\gear\books\data\book_TheWarOfTheWorlds_destruct.rvmat"}}
                    };
                };
            };
        };
    };
};

//==============================================================================
// Balancing dieses Meilensteins
//
// partialMinKnownSlots
//   Ab wie vielen bekannten PFLICHT-Slots ein Rezept von UNKNOWN auf PARTIAL
//   springt - also ab wann der Spieler ueberhaupt erfaehrt, dass es existiert.
//
//   1 bedeutet: die erste passende Zutat in der Hand genuegt. Das ist die
//   Entscheidung aus M5_Entscheidungen.md §3, und der Grund steht dort: ein
//   Kochbuch, das erst nach der halben Zutatenliste etwas zeigt, belohnt
//   niemanden fuer Neugier. Wer es straffer will, setzt hier 2 oder 3 - der
//   Code liest den Wert, er ist nirgends fest verdrahtet.
//
//   0 oder kleiner wird auf 1 gehoben. Ein Rezept ohne jede bekannte Zutat
//   anzuzeigen waere kein Kochbuch mehr, sondern eine Liste.
//==============================================================================
class CfgChefZCookbook
{
    partialMinKnownSlots = 1;
};
