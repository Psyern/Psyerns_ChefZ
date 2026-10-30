//==============================================================================
// ChefZ_Fishing - Modul des Angel-Slice.
//
// SLICE fillets: die Filets der 32 Fische aus ChefZ_Fishing_Slice_Plan.md §3/§4.
// Dieses Modul wird von mehreren Slices gleichzeitig gefuellt; jeder Slice
// bringt seinen EIGENEN CfgChefZ-Knoten mit (hier: ChefZ_fillets) und seine
// eigenen Dateien. Fremde Dateien werden nicht angefasst.
//
// PFADWURZEL: das PBO-Praefix ist der ORDNERNAME des Addons. Jeder Laufzeitpfad
// beginnt deshalb mit "ChefZ_Fishing/" (Entwurf 02 §4.1).
//
// ---------------------------------------------------------------------------
// WAS HIER STEHT - UND WAS NICHT
// ---------------------------------------------------------------------------
// HIER:   30 Filetklassen plus drei Configbasen. Sie sind das ZIEL der
//         Filet-Transforms, die die Fisch-Slices (fish-sea-a, fish-sea-b,
//         fish-fresh) mitbringen. Ein Transform ohne Zielklasse waere ein
//         Rezept, das nie ein Ergebnis hat.
// NICHT:  kein Transform, kein Prozess, kein Fisch, kein Koeder. Dieser Slice
//         deklariert deshalb handcraftRecipeSlots = 0 (21 §1). Die 31
//         HANDCRAFT-Slots des Filetierens zaehlen die Fisch-Slices in IHREN
//         Knoten.
//
// ZWEI FISCHE OHNE EIGENE KLASSE, BEIDE ABSICHTLICH:
//   Mackerel   Vanilla hat MackerelFilletMeat und PrepareMackerel. Eine zweite
//              Makrele waere ein Dublett im Kontextmenue (21 §5, Vanilla-
//              Vorrang). Die Zutatenbindung von MackerelFilletMeat liegt
//              bereits in ChefZ_Preservation/Config/Ingredients/Preservation.json
//              und wird hier NICHT angefasst.
//   SeaEel     teilt sich das Filet mit Eel (Slice-Plan §3: "teilt Filet mit
//              Eel"). Der Transform TR_FilletSeaEel gibt ChefZ_EelFillet aus.
// 32 Fische - 1 Vanillafisch - 1 geteiltes Filet = 30 Klassen.
//
// ---------------------------------------------------------------------------
// DIE MODELLE - Stand 14.09.2026: KEIN geliefertes Filetmesh in der Packquelle
// ---------------------------------------------------------------------------
// Der Slice-Plan §4 nennt fuenf gebaute Filetmeshes (BluefinTuna, RedSnapper,
// Eel, MahiMahi, GiantGrouper). Geprueft wurde der Lieferordner ChefZ/ und die
// Packquelle ChefZ_Core/Addons/ChefZ_Food/models/ - dort liegt am 14.09.2026
// KEINE Filetdatei. Gebunden wird nur, was in der Packquelle ankommt
// (sync-assets.mjs, SUBDIRS); ein model=, das auf eine nicht existierende .p3d
// zeigt, meldet kein Werkzeug (siehe Kopf von sync-assets.mjs, corn_plant.p3d).
// Alle 30 Klassen stehen deshalb auf Vanillas Filetmeshes. Sobald die fuenf
// Dateien ankommen, ist es je Klasse EINE Zeile.
//
// Die beiden Vanillapfade sind nicht geraten. Belegt an der ausgelieferten
// Vanilla-Configkopie in
//   Mod Repositories/DayZExpansion/Objects/Structures/BuilderItems/DZ/statics/
//   VanillaItems/config.cpp:4657 (bldr_prop_CarpFilletMeat)
//   ...:4683 (bldr_prop_MackerelFilletMeat)
// samt hiddenSelections, Texturen und Materialien - woertlich uebernommen:
//   Salzwasser  \dz\gear\food\mackerel_fillet.p3d
//   Suesswasser \dz\gear\food\carp_fillet.p3d
// Salmon und SockeyeSalmon stehen auf dem Salzwassermesh, obwohl ihr enviro
// BOTH ist: es ist das groessere der beiden Filets und passt zum Wanderfisch.
//
// ---------------------------------------------------------------------------
// ZWEI PFLICHTBLOECKE AN JEDER ESSBAREN KLASSE (Begruendung aus ChefZ_Meat)
// ---------------------------------------------------------------------------
// class Nutrition   PlayerStomach.InitData registriert nur Klassen mit
//                   "Nutrition" ODER "Food" und scope != 0 (01 V7). Fehlt
//                   beides, saettigt der Bissen lautlos nichts.
// class Food        FoodStage.GetNextFoodStageType faellt ohne passenden
//   + Transitions   Uebergang auf BURNED zurueck (01 V4). Eine kochbare Klasse
//                   OHNE Uebergaenge verbrennt beim ersten Garstufenwechsel.
//
// nutrition_properties[] in der Reihenfolge aus FoodStage.c:
//   { fullnessIndex, energy, water, nutritionalIndex, toxicity, agents, digestibility }
// agents: eAgents.SALMONELLA = 4 auf Raw, eAgents.FOOD_POISON = 16 auf Rotten.
//
// VOLUMEN: PlayerStomach.c:86 rechnet volume = fullnessIndex * m_Amount OHNE
// Division, PlayerStomach.c:92 dagegen energy_per_unit = energy / 100. Die
// Mengenskala ist deshalb 250 wie bei ChefZ_Meat und ChefZ_Preservation, und
// der fullnessIndex bleibt im Vanilla-Band 0,75-2,8. Ein Filet fuellt bei
// 1,40 * 250 = 350 Volumen rund ein Sechstel des Brechgrenzwerts (2000).
//
// DREI NAEHRWERTPROFILE statt dreissig Einzelmeinungen:
//   LEAN   mageres Weissfleisch (Kabeljau, Scholle, Barsch ...)  Raw 110 kcal
//   OILY   fettreiche Schwimmer (Thunfisch, Lachs, Makrelenartige) Raw 165
//   FATTY  Aal und Meeraal                                        Raw 205
// Die Staffelung ist der einzige Unterschied zwischen den Klassen; alles
// andere - Menge, Garstufen, Uebergaenge, Modell - kommt aus der Basis.
//
// KEINE TERJE-REFERENZ. Kein Klassenname, kein Aufruf, kein Schalter.
//==============================================================================

//==============================================================================
// ### SLICE baits ###   Die sieben Koeder
//
// DIESE DATEI GEHOERT MEHREREN SLICES. Jeder Slice ergaenzt hier seine Klassen
// und seinen EIGENEN Knoten in CfgChefZ - er ersetzt nichts. Die Slice-Grenzen
// sind mit "### SLICE <name> ###" markiert, genau wie in ChefZ_Farming.
//
// ---------------------------------------------------------------------------
// ### SLICE baits ###  Warum die sieben Koeder NICHT von BaitBase erben
// ---------------------------------------------------------------------------
// BaitBase.EEItemLocationChanged loescht sich selbst, sobald sie einen Ort
// wechselt, und ersetzt sich durch "hookType" + angehaengten Worm
// (scripts - 1.29, 4_World/DayZ/Entities/ItemBase/Gear/Consumables/
// FishingConsumables.c:18-38). Der Kommentar dort nennt den Zweig
// "Obsolete item prison": Bait und BoneBait sind Altbestand, den die Engine
// beim ersten Anfassen aufloest.
//
// Ein ChefZ-Koeder, der davon erbte, verschwaende sich beim ersten Griff ins
// Inventar - lautlos, und mit einem Wurm als Ersatz. Deshalb ist die Basis
// Inventory_Base, und der Anschluss an die Rute laeuft ueber inventorySlot[]
// mit "Bait", also genau ueber den Weg, den
// CatchingContextFishingRodAction.InitItemValues abfragt
// (GetCurrentAttachmentSlotInfo -> case "Bait"). Der Slot ist Vanilla
// (Slot_Bait); ChefZ legt keinen eigenen an.
//
// Das ist zugleich eine Validatorregel des Datenformats: 21 §3.4,
// "id erbt NICHT von BaitBase" = ERROR.
//
// ---------------------------------------------------------------------------
// ### SLICE baits ###  class Fishing - was sie hier tut und was nicht
// ---------------------------------------------------------------------------
// InitItemValues liest den Knoten "CfgVehicles <Item> Fishing" AUFSUMMIEREND
// (+=) ueber Rute, Haken und Koeder. Von den dort gelesenen Feldern ist fuer
// einen Koeder nur eines sinnvoll:
//
//   baitLossChanceMod   geht in GetBaitLossChanceModifierClamped(), und die
//                       wird ausschliesslich von TryBaitLoss() gerufen - dem
//                       Wurf beim FEHLZUG (OnSignalMiss). Bei 0 kehrt
//                       TryBaitLoss sofort zurueck, ohne zu wuerfeln.
//
// Beide Koederarten stehen hier auf 0, und das ist kein Versehen:
//
//   lure  bleibt am Haken. 0 ist die Zusage nach aussen. Die eigentliche
//         Rettung leistet das Override in
//         ChefZ_Core/Scripts/4_World/ChefZ/Fishing/ChefZ_ModdedCatchingContext.c -
//         denn OnAfterSpawnSignalHit und OnSignalPass rufen RemoveItemSafe
//         BEDINGUNGSLOS (CatchingContextFishingRodAction.c:352-370), da hilft
//         keine Wahrscheinlichkeit der Welt. Der Verschleiss eines
//         Kunstkoeders steht als "lureWearPerUse" im Datensatz, nicht hier.
//
//   soft  wird verbraucht wie Vanillas Wurm - und der Wurm wird von genau
//         denselben zwei bedingungslosen Aufrufen verbraucht. Ein zusaetzlicher
//         Verlustwurf beim Fehlzug wuerde den Weichkoeder SCHLECHTER stellen
//         als den Wurm, den er ersetzt. 0 ist hier also "wie Wurm", nicht
//         "unverwuestlich".
//
// Kein resultQuantityBaseMod, kein signal*: Fanggroesse und Signalzeiten sind
// Sache von Rute und Haken. Ein Koeder, der die Fangchance anhebt, waere eine
// zweite Balance-Mechanik neben dem weightMultiplier des Datensatzes - und der
// ist die eine, die dieser Slice baut.
//
// ---------------------------------------------------------------------------
// ### SLICE baits ###  OFFENER PUNKT: die Namen ChefZ_Bait_<Key>
// ---------------------------------------------------------------------------
// tools/chefz-validate/naming.mjs prueft ChefZ_PascalCase gegen
//     /^ChefZ_[A-Z][A-Za-z0-9]*(_Base)?$/
// und laesst damit GENAU EINEN Unterstrich zu, und nur den vor "_Base". Die
// sieben Koedernamen tragen einen zweiten - nicht aus Nachlaessigkeit:
// "ChefZ_Bait_<Key>" steht woertlich im Auftrag, in
// ChefZ_Docs/ChefZ_Fishing_Slice_Plan.md §3 (Schlusssatz) und als
// ausgeschriebene ID in design/FINAL/21_Fishing_Data_Format.md §3.2. Dieselbe
// Regel trifft "ChefZ_Fish_<Key>" der drei Fisch-Slices.
//
// Der Slice hat die Namen NICHT eigenmaechtig geaendert; das waere eine
// Entscheidung ueber vier Slices hinweg. Gemeldet als Blocker: entweder
// naming.mjs laesst ein Namensraum-Segment zu, oder der Fishing-Plan wird
// projektweit umgeschrieben.
//
// Was der Slice selbst abstellen konnte, ist abgestellt: die beiden
// ZWISCHENBASEN heissen ChefZ_BaitLure_Base und ChefZ_BaitSoft_Base - sie sind
// Erfindung dieses Slice und stehen in keinem Plan.
//
// ---------------------------------------------------------------------------
// ### SLICE baits ###  3D
// ---------------------------------------------------------------------------
// Lykos' Lieferung unter Psyerns_ChefZ/ChefZ/ enthaelt zum 14.09.2026 KEIN
// Koedermesh (geprueft: kein *bait*, *lure*, *spinner*, *flasher*, *minnow*).
// Alle sieben tragen deshalb ein VANILLA-PROXY-MODELL:
//
//   lure -> \dz\gear\crafting\bone_bait.p3d           (Vanillas BoneBait)
//   soft -> \dz\gear\consumables\bait_worm_pinned.p3d (Vanillas Bait)
//
// Beide Pfade sind belegt an der ausgelieferten Vanilla-Configkopie in
// Mod Repositories/DayZExpansion/Objects/Structures/BuilderItems/DZ/statics/
// VanillaItems/config.cpp:18247 (bldr_prop_Bait) und :18252
// (bldr_prop_BoneBait). Der Bedarf an sieben eigenen Meshes ist im
// Slice-Bericht als Asset-Bedarf gemeldet. Auf ein Modell wartet hier nichts.
//==============================================================================

class CfgPatches
{
    class ChefZ_Fishing
    {
        units[] =
        {
            // ### SLICE fish-sea-a ###
            "ChefZ_SaltwaterFishA_Base",
            "ChefZ_Plaice", "ChefZ_GiltheadBream", "ChefZ_Halibut",
            "ChefZ_Cod", "ChefZ_Sole", "ChefZ_SeaBass",
            "ChefZ_RedMullet", "ChefZ_Conger", "ChefZ_SeaEel",
            "ChefZ_HorseMackerel", "ChefZ_JapaneseSardine", "ChefZ_Rockfish",
            // Meeresfruechte (26.10.2026)
            "ChefZ_Seafood_Base",
            "ChefZ_BlueSentinelCrab",
            "ChefZ_CoastalCrab",
            "ChefZ_CrimsonKingCrab",
            "ChefZ_Langoustine",
            "ChefZ_MidnightLobster",
            "ChefZ_SnowCrab",
            "ChefZ_SpinyLobster",
            "ChefZ_SpinyOrangeCrab",
            "ChefZ_StripedTigerPrawn",
            "ChefZ_CrimsonSquid",
            "ChefZ_NorthernCrimsonSquid",
            "ChefZ_Octopus",
            "ChefZ_SpeckledSquid",
            "ChefZ_Squid",
            "ChefZ_FishFillet_Base",
            "ChefZ_SeaFillet_Base",
            "ChefZ_FreshFillet_Base",
            "ChefZ_PlaiceFillet",
            "ChefZ_GiltheadBreamFillet",
            "ChefZ_HalibutFillet",
            "ChefZ_CodFillet",
            "ChefZ_SoleFillet",
            "ChefZ_SeaBassFillet",
            "ChefZ_RedMulletFillet",
            "ChefZ_CongerFillet",
            "ChefZ_HorseMackerelFillet",
            "ChefZ_JapaneseSardineFillet",
            "ChefZ_RockfishFillet",
            "ChefZ_BluefinTunaFillet",
            "ChefZ_YellowfinTunaFillet",
            "ChefZ_RedSnapperFillet",
            "ChefZ_GiantGrouperFillet",
            "ChefZ_MahiMahiFillet",
            "ChefZ_GiantTrevallyFillet",
            "ChefZ_BlueMarlinFillet",
            "ChefZ_SailfishFillet",
            "ChefZ_BonitoFillet",
            "ChefZ_YellowtailFillet",
            "ChefZ_TarponFillet",
            "ChefZ_SalmonFillet",
            "ChefZ_SockeyeSalmonFillet",
            "ChefZ_PerchFillet",
            "ChefZ_MuskellungeFillet",
            "ChefZ_SnakeheadFillet",
            "ChefZ_RainbowTroutFillet",
            "ChefZ_CharFillet",
            "ChefZ_EelFillet",

            // ### SLICE baits ###
            "ChefZ_Bait_Base",
            "ChefZ_BaitLure_Base",
            "ChefZ_BaitSoft_Base",
            "ChefZ_AntiqueSpoonFlasher",
            "ChefZ_BlueBait",
            "ChefZ_MinnowLure",
            "ChefZ_PaddleTailBait",
            "ChefZ_RedTopperBait",
            "ChefZ_Spinnerbait",
            "ChefZ_TroutBait",
            "ChefZ_CrimsonSquidLure",
            "ChefZ_NeonPrawnJig",
            "ChefZ_SilverSardineLure",

            // ### SLICE fish-fresh ###
            "ChefZ_FreshwaterFish_Base",
            "ChefZ_Perch",
            "ChefZ_Muskellunge",
            "ChefZ_Snakehead",
            "ChefZ_RainbowTrout",
            "ChefZ_Char",
            "ChefZ_Salmon",
            "ChefZ_SockeyeSalmon",
            "ChefZ_Eel",

            // ### SLICE fish-sea-b ###
            "ChefZ_SeaFishB_Base",
            "ChefZ_BluefinTuna",
            "ChefZ_YellowfinTuna",
            "ChefZ_RedSnapper",
            "ChefZ_GiantGrouper",
            "ChefZ_MahiMahi",
            "ChefZ_GiantTrevally",
            "ChefZ_BlueMarlin",
            "ChefZ_Sailfish",
            "ChefZ_Bonito",
            "ChefZ_Yellowtail",
            "ChefZ_Tarpon"
        };
        weapons[] = {};
        requiredVersion = 0.1;

        // Jeder Eintrag steht fuer etwas, das dieses Modul TATSAECHLICH nutzt:
        //   DZ_Data        Grundlage von allem
        //   DZ_Gear_Food   Edible_Base und die beiden Filetmodelle
        //                  (\dz\gear\food\mackerel_fillet.p3d, carp_fillet.p3d)
        //   ChefZ_Core     ChefZ_Edible_Base (Skriptbasis) und der Config Manager
        //
        // ChefZ_Registry steht hier BEWUSST NICHT, obwohl der Auftrag es nannte:
        // ChefZ_Registry/config.cpp:93-104 nennt seinerseits jedes Contentmodul,
        // dessen Klassen seine Nutrition-Records ansprechen, und begruendet
        // ausdruecklich "Keines der genannten Module haengt umgekehrt von
        // ChefZ_Registry ab - es gibt also keinen Zyklus". Sobald der Integrator
        // die 30 Nutrition-Records dieses Slice merged, traegt er ChefZ_Fishing
        // dort ein; ein ChefZ_Registry hier waere dann ein echter
        // requiredAddons-Zyklus. Die Ladereihenfolge sichert stattdessen der
        // loadOrder des CfgChefZ-Knotens (Registry 150, dieser Slice 400).
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Gear_Food",
            "ChefZ_Core",
            // ### SLICE baits ###: bone_bait.p3d (DZ_Gear_Crafting),
            // bait_worm_pinned.p3d (DZ_Gear_Consumables), ItemBase und der
            // AnimalCatching-Zweig (DZ_Scripts), der Koederleser (ChefZ_Core).
            "DZ_Scripts",
            "DZ_Gear_Crafting",
            "DZ_Gear_Consumables",
            "ChefZ_Processing"
        };
    };
};

class CfgMods
{
    class ChefZ_Fishing
    {
        dir = "ChefZ_Fishing";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Fishing";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        // Bis 26.10.2026 nur World (Entitaetsklassen).
        // Angel-Minispiel (27.10.2026): 3_Game (Konstanten, Bruecke), 5_Mission
        // (HUD) und die Tasten. Jede gelistete Ebene hat Dateien.
        inputs = "ChefZ_Fishing/Scripts/Data/Inputs.xml";
        dependencies[] = {"Game", "World", "Mission"};

        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Fishing/Scripts/3_Game"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Fishing/Scripts/5_Mission"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Fishing/Scripts/4_World"
                };
            };
        };
    };
};

// Vanilla-Uebergaenge der ganzen Tiere (DZ_Gear_Food), geerbt von ChefZ_Seafood_Base.
class AnimalCorpsesStageTransitions;

class CfgVehicles
{
    // Vorwaertsdeklaration. Sie definiert nichts - sie macht den Namen
    // aufloesbar, damit "class ChefZ_FishFillet_Base : Edible_Base" nicht mit
    // "Undefined base class" in einem MODALEN Fenster endet (chefzbase).
    class Edible_Base;

    // ------------------------------------------------------------------------
    // Gemeinsame Configbasis aller Filets dieses Slice.
    //
    // scope = 0: sie ist kein Item. Sie ist die Stelle, an der Mengenskala,
    // Garstufen und Garstufenuebergaenge EINMAL stehen. Der Core bringt keine
    // solche Basis mit (Invariante I3).
    //
    // Configbasis ist eine VANILLA-Klasse, Skriptbasis ist ChefZ_Edible_Base -
    // die Andockregel aus dem Kopf von ChefZ_Edible_Base.c.
    // ------------------------------------------------------------------------
    class ChefZ_FishFillet_Base : Edible_Base
    {
        scope = 0;
        rotationFlags = 17;
        itemSize[] = {2, 1};
        // Raeucher- und Bratplaetze wie Vanillas Filets (CarpFilletMeat) plus
        // die Haken 5-12 des Raeucherschranks (27.10.2026, Spieltest "man kann
        // keine Fische raeuchern": ohne dieses Feld nahm kein Haken ein Filet).
        // "Ingredient" ist der Platz am angespitzten Stock (SharpWoodenStick,
        // Vanilla DZ/gear/crafting) - Filets ueber dem Feuer, 30.10.2026.
        inventorySlot[] =
        {
            "Ingredient",
            "DirectCookingA", "DirectCookingB", "DirectCookingC",
            "SmokingA", "SmokingB", "SmokingC", "SmokingD",
            "ChefZ_SmokeHook05", "ChefZ_SmokeHook06", "ChefZ_SmokeHook07", "ChefZ_SmokeHook08", "ChefZ_SmokeHook09", "ChefZ_SmokeHook10", "ChefZ_SmokeHook11", "ChefZ_SmokeHook12", "ChefZ_SmokeHook13", "ChefZ_SmokeHook14", "ChefZ_SmokeHook15", "ChefZ_SmokeHook16", "ChefZ_SmokeHook17", "ChefZ_SmokeHook18"
        };
        weight = 200;
        absorbency = 0.5;

        // MENGENSKALA 250 - dieselbe wie ChefZ_Meat und ChefZ_Preservation.
        // Begruendung im Dateikopf (PlayerStomach.c:86 gegen :92).
        // Fuer Rezeptdaten ist die Zahl transparent: Einheiten sind ein
        // Verhaeltnis (ChefZ_FactCollector.c:439), ein volles Item bleibt bei
        // unitsPerWholeItem = 1 genau eine Einheit.
        varQuantityInit = 250;
        varQuantityMin = 0;
        varQuantityMax = 250;
        varQuantityDestroyOnMin = 1;
        quantityBar = 1;
        canBeSplit = 0;
        isMeleeWeapon = 0;

        class Food
        {
            class FoodStages
            {
                // visual_properties[] = { selectionIndex, textureIndex, materialIndex }
                // Die Indizes zeigen in die hiddenSelectionsTextures[] bzw.
                // hiddenSelectionsMaterials[] der beiden Wassertypbasen unten -
                // fuenf Texturen (raw, baked, boiled, dried, burnt) und sechs
                // Materialien (dieselben plus rotten). Rotten hat kein eigenes
                // CO und nimmt deshalb die Burnt-Textur mit dem Rotten-Material.
                //
                // cooking_properties[] = { minTemp, cookTime, maxTemp }
                // woertlich aus enum eCookingPropertyIndices (FoodStage.c:15).
                class Raw
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                };
                class Baked
                {
                    visual_properties[] = {0, 1, 1};
                    cooking_properties[] = {100, 50, 200};
                };
                class Boiled
                {
                    visual_properties[] = {0, 2, 2};
                    cooking_properties[] = {100, 70, 150};
                };
                // Die Garstufe Dried bleibt deklariert, OBWOHL es unten keinen
                // Vanilla-Uebergang dorthin gibt: der Zustand DRIED des Slice
                // "preservation" projiziert auf genau diese Vanilla-Garstufe
                // (Preservation-Delta, projectsToVanillaStage "Dried"). Ohne die
                // Stufe haette der ChefZ-Zustand kein Ziel.
                class Dried
                {
                    visual_properties[] = {0, 3, 3};
                    cooking_properties[] = {0, 0, 0};
                };
                class Burned
                {
                    visual_properties[] = {0, 4, 4};
                    cooking_properties[] = {200, 20, 0};
                };
                class Rotten
                {
                    visual_properties[] = {0, 4, 5};
                    cooking_properties[] = {0, 0, 0};
                };
            };

            // OHNE DIESEN BLOCK VERBRENNT JEDES FILET DES SLICE (01 V4).
            //
            // transition_to und cooking_method sind ZAHLEN, nicht Namen:
            // SetupFoodStageTransitionMapping liest sie mit ConfigGetInt
            // (FoodStage.c:167ff).
            //   FoodStageType:     RAW 1, BAKED 2, BOILED 3, DRIED 4, BURNED 5, ROTTEN 6
            //   CookingMethodType: NONE 0, BAKING 1, BOILING 2, DRYING 3, TIME 4
            //
            // DRYING fehlt absichtlich und aus demselben Grund wie in
            // ChefZ_Meat: Trocknen und Raeuchern laufen in ChefZ an eigenen
            // Stationen (11 E6), nicht in Vanillas Trockenslots. Der Slice
            // "preservation" bringt sie mit. Ein Filet, das im Feuer liegen
            // bleibt, verbrennt - das ist gewollt.
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // ------------------------------------------------------------------------
    // Die beiden Wassertypbasen. Sie unterscheiden sich in NICHTS ausser dem
    // geteilten Mesh und seinen Texturen - siehe Dateikopf. Shared-Mesh-Bedarf,
    // kein eigenes Modell je Fischart.
    // ------------------------------------------------------------------------
    class ChefZ_SeaFillet_Base : ChefZ_FishFillet_Base
    {
        scope = 0;
        model = "\dz\gear\food\mackerel_fillet.p3d";
        hiddenSelections[] = {"cs_raw"};
        hiddenSelectionsTextures[] =
        {
            "dz\gear\food\data\mackerel_fillet_raw_CO.paa",
            "dz\gear\food\data\mackerel_fillet_baked_CO.paa",
            "dz\gear\food\data\mackerel_fillet_boiled_CO.paa",
            "dz\gear\food\data\mackerel_fillet_dried_CO.paa",
            "dz\gear\food\data\mackerel_fillet_burnt_CO.paa"
        };
        hiddenSelectionsMaterials[] =
        {
            "dz\gear\food\data\mackerel_fillet_raw.rvmat",
            "dz\gear\food\data\mackerel_fillet_baked.rvmat",
            "dz\gear\food\data\mackerel_fillet_boiled.rvmat",
            "dz\gear\food\data\mackerel_fillet_dried.rvmat",
            "dz\gear\food\data\mackerel_fillet_burnt.rvmat",
            "dz\gear\food\data\mackerel_fillet_rotten.rvmat"
        };
    };

    class ChefZ_FreshFillet_Base : ChefZ_FishFillet_Base
    {
        scope = 0;
        model = "\dz\gear\food\carp_fillet.p3d";
        hiddenSelections[] = {"cs_raw"};
        hiddenSelectionsTextures[] =
        {
            "dz\gear\food\data\carp_fillet_raw_CO.paa",
            "dz\gear\food\data\carp_fillet_baked_CO.paa",
            "dz\gear\food\data\carp_fillet_boiled_CO.paa",
            "dz\gear\food\data\carp_fillet_dried_CO.paa",
            "dz\gear\food\data\carp_fillet_burnt_CO.paa"
        };
        hiddenSelectionsMaterials[] =
        {
            "dz\gear\food\data\carp_fillet_raw.rvmat",
            "dz\gear\food\data\carp_fillet_baked.rvmat",
            "dz\gear\food\data\carp_fillet_boiled.rvmat",
            "dz\gear\food\data\carp_fillet_dried.rvmat",
            "dz\gear\food\data\carp_fillet_burnt.rvmat",
            "dz\gear\food\data\carp_fillet_rotten.rvmat"
        };
    };

    // ------------------------------------------------------------------------
    // DIE DREISSIG FILETS
    // ------------------------------------------------------------------------

    // Plaice - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_PlaiceFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\plaicefillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\PlaiceFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\PlaiceFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\PlaiceFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\PlaiceFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\PlaiceFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_PLAICEFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_PLAICEFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // GiltheadBream - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_GiltheadBreamFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\giltheadbreamfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\GiltheadBreamFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GiltheadBreamFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GiltheadBreamFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GiltheadBreamFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GiltheadBreamFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_GILTHEADBREAMFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_GILTHEADBREAMFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Halibut - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_HalibutFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\halibutfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\HalibutFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\HalibutFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\HalibutFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\HalibutFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\HalibutFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_HALIBUTFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_HALIBUTFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Cod - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_CodFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\codfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\CodFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CodFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CodFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CodFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CodFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_CODFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_CODFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Sole - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_SoleFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\solefillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\SoleFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SoleFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SoleFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SoleFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SoleFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_SOLEFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_SOLEFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // SeaBass - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_SeaBassFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\seabassfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\SeaBassFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SeaBassFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SeaBassFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SeaBassFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SeaBassFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_SEABASSFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_SEABASSFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // RedMullet - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_RedMulletFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\redmulletfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026; zentriert 30.10.2026, sonst leere Vorschau)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\RedMulletFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RedMulletFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RedMulletFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RedMulletFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RedMulletFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_REDMULLETFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_REDMULLETFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Conger - sehr fettes Fleisch, Proxy mackerel_fillet.
    class ChefZ_CongerFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\congerfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\CongerFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CongerFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CongerFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CongerFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CongerFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_CONGERFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_CONGERFILLET1";
        weight = 225;

        class Nutrition
        {
            fullnessIndex = 1.5;
            energy = 205;
            water = 32;
            nutritionalIndex = 22;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.5, 205, 32, 22, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.34, 390, 12, 34, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.41, 360, 44, 34, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.12, 405, 4, 36, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.94, 80, 5, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.25, 120, 16, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // HorseMackerel - fettreicher Schwimmer, Proxy mackerel_fillet.
    class ChefZ_HorseMackerelFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\horsemackerelfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\HorseMackerelFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\HorseMackerelFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\HorseMackerelFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\HorseMackerelFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\HorseMackerelFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_HORSEMACKERELFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_HORSEMACKERELFILLET1";
        weight = 215;

        class Nutrition
        {
            fullnessIndex = 1.45;
            energy = 165;
            water = 38;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.45, 165, 38, 20, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.3, 320, 15, 32, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.37, 295, 48, 32, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.08, 335, 4, 34, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.92, 70, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.22, 100, 18, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // JapaneseSardine - fettreicher Schwimmer, Proxy mackerel_fillet.
    class ChefZ_JapaneseSardineFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\japanesesardinefillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\JapaneseSardineFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\JapaneseSardineFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\JapaneseSardineFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\JapaneseSardineFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\JapaneseSardineFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_JAPANESESARDINEFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_JAPANESESARDINEFILLET1";
        weight = 215;

        class Nutrition
        {
            fullnessIndex = 1.45;
            energy = 165;
            water = 38;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.45, 165, 38, 20, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.3, 320, 15, 32, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.37, 295, 48, 32, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.08, 335, 4, 34, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.92, 70, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.22, 100, 18, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Rockfish - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_RockfishFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\rockfishfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\RockfishFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RockfishFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RockfishFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RockfishFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RockfishFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_ROCKFISHFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_ROCKFISHFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // BluefinTuna - fettreicher Schwimmer, Proxy mackerel_fillet.
    class ChefZ_BluefinTunaFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\tunafillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\TunaFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TunaFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TunaFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TunaFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TunaFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_BLUEFINTUNAFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_BLUEFINTUNAFILLET1";
        weight = 215;

        class Nutrition
        {
            fullnessIndex = 1.45;
            energy = 165;
            water = 38;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.45, 165, 38, 20, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.3, 320, 15, 32, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.37, 295, 48, 32, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.08, 335, 4, 34, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.92, 70, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.22, 100, 18, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // YellowfinTuna - fettreicher Schwimmer, Proxy mackerel_fillet.
    class ChefZ_YellowfinTunaFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\tunafillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\TunaFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TunaFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TunaFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TunaFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TunaFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_YELLOWFINTUNAFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_YELLOWFINTUNAFILLET1";
        weight = 215;

        class Nutrition
        {
            fullnessIndex = 1.45;
            energy = 165;
            water = 38;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.45, 165, 38, 20, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.3, 320, 15, 32, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.37, 295, 48, 32, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.08, 335, 4, 34, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.92, 70, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.22, 100, 18, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // RedSnapper - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_RedSnapperFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\redsnapperfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\RedSnapperFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RedSnapperFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RedSnapperFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RedSnapperFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RedSnapperFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_REDSNAPPERFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_REDSNAPPERFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // GiantGrouper - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_GiantGrouperFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\grouperfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\GrouperFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GrouperFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GrouperFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GrouperFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GrouperFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_GIANTGROUPERFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_GIANTGROUPERFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // MahiMahi - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_MahiMahiFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\mahimahifillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\MahiMahiFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\MahiMahiFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\MahiMahiFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\MahiMahiFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\MahiMahiFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_MAHIMAHIFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_MAHIMAHIFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // GiantTrevally - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_GiantTrevallyFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\gianttrevallyfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\GiantTrevallyFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GiantTrevallyFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GiantTrevallyFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GiantTrevallyFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\GiantTrevallyFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_GIANTTREVALLYFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_GIANTTREVALLYFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // BlueMarlin - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_BlueMarlinFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\bluemarlinfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\BlueMarlinFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\BlueMarlinFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\BlueMarlinFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\BlueMarlinFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\BlueMarlinFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_BLUEMARLINFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_BLUEMARLINFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Sailfish - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_SailfishFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\sailfishfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\SailfishFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SailfishFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SailfishFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SailfishFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SailfishFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_SAILFISHFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_SAILFISHFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Bonito - fettreicher Schwimmer, Proxy mackerel_fillet.
    class ChefZ_BonitoFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\bonitofillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\BonitoFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\BonitoFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\BonitoFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\BonitoFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\BonitoFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_BONITOFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_BONITOFILLET1";
        weight = 215;

        class Nutrition
        {
            fullnessIndex = 1.45;
            energy = 165;
            water = 38;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.45, 165, 38, 20, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.3, 320, 15, 32, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.37, 295, 48, 32, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.08, 335, 4, 34, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.92, 70, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.22, 100, 18, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Yellowtail - fettreicher Schwimmer, Proxy mackerel_fillet.
    class ChefZ_YellowtailFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\yellowtailfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\YellowtailFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\YellowtailFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\YellowtailFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\YellowtailFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\YellowtailFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_YELLOWTAILFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_YELLOWTAILFILLET1";
        weight = 215;

        class Nutrition
        {
            fullnessIndex = 1.45;
            energy = 165;
            water = 38;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.45, 165, 38, 20, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.3, 320, 15, 32, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.37, 295, 48, 32, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.08, 335, 4, 34, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.92, 70, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.22, 100, 18, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Tarpon - mageres Weissfleisch, Proxy mackerel_fillet.
    class ChefZ_TarponFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\tarponfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026; zentriert 30.10.2026, sonst leere Vorschau)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\TarponFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TarponFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TarponFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TarponFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\TarponFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_TARPONFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_TARPONFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Salmon - fettreicher Schwimmer, Proxy mackerel_fillet.
    class ChefZ_SalmonFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\salmonfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\SalmonFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SalmonFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SalmonFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SalmonFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SalmonFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_SALMONFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_SALMONFILLET1";
        weight = 215;

        class Nutrition
        {
            fullnessIndex = 1.45;
            energy = 165;
            water = 38;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.45, 165, 38, 20, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.3, 320, 15, 32, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.37, 295, 48, 32, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.08, 335, 4, 34, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.92, 70, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.22, 100, 18, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // SockeyeSalmon - fettreicher Schwimmer, Proxy mackerel_fillet.
    class ChefZ_SockeyeSalmonFillet : ChefZ_SeaFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\sockeyesalmonfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\SockeyeSalmonFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SockeyeSalmonFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SockeyeSalmonFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SockeyeSalmonFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SockeyeSalmonFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_SOCKEYESALMONFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_SOCKEYESALMONFILLET1";
        weight = 215;

        class Nutrition
        {
            fullnessIndex = 1.45;
            energy = 165;
            water = 38;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.45, 165, 38, 20, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.3, 320, 15, 32, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.37, 295, 48, 32, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.08, 335, 4, 34, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.92, 70, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.22, 100, 18, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Perch - mageres Weissfleisch, Proxy carp_fillet.
    class ChefZ_PerchFillet : ChefZ_FreshFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\perchfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\PerchFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\PerchFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\PerchFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\PerchFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\PerchFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_PERCHFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_PERCHFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Muskellunge - mageres Weissfleisch, Proxy carp_fillet.
    class ChefZ_MuskellungeFillet : ChefZ_FreshFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\muskellungefillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\MuskellungeFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\MuskellungeFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\MuskellungeFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\MuskellungeFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\MuskellungeFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_MUSKELLUNGEFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_MUSKELLUNGEFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Snakehead - mageres Weissfleisch, Proxy carp_fillet.
    class ChefZ_SnakeheadFillet : ChefZ_FreshFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\snakeheadfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\SnakeheadFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SnakeheadFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SnakeheadFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SnakeheadFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\SnakeheadFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_SNAKEHEADFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_SNAKEHEADFILLET1";
        weight = 200;

        class Nutrition
        {
            fullnessIndex = 1.4;
            energy = 110;
            water = 45;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.4, 110, 45, 16, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.26, 235, 18, 28, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.33, 215, 52, 28, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.05, 245, 4, 30, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.9, 55, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.2, 70, 20, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // RainbowTrout - fettreicher Schwimmer, Proxy carp_fillet.
    class ChefZ_RainbowTroutFillet : ChefZ_FreshFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\rainbowtroutfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\RainbowTroutFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RainbowTroutFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RainbowTroutFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RainbowTroutFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\RainbowTroutFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_RAINBOWTROUTFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_RAINBOWTROUTFILLET1";
        weight = 215;

        class Nutrition
        {
            fullnessIndex = 1.45;
            energy = 165;
            water = 38;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.45, 165, 38, 20, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.3, 320, 15, 32, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.37, 295, 48, 32, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.08, 335, 4, 34, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.92, 70, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.22, 100, 18, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Char - fettreicher Schwimmer, Proxy carp_fillet.
    class ChefZ_CharFillet : ChefZ_FreshFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\charfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\CharFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CharFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CharFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CharFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\CharFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_CHARFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_CHARFILLET1";
        weight = 215;

        class Nutrition
        {
            fullnessIndex = 1.45;
            energy = 165;
            water = 38;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.45, 165, 38, 20, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.3, 320, 15, 32, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.37, 295, 48, 32, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.08, 335, 4, 34, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.92, 70, 6, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.22, 100, 18, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    // Eel - sehr fettes Fleisch, Proxy carp_fillet.
    class ChefZ_EelFillet : ChefZ_FreshFillet_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\fishfilet\models\eelfillet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] =
        {
            "ChefZ\ChefZ_Sealife\fishfilet\data\EelFillet_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\EelFillet_baked_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\EelFillet_boiled_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\EelFillet_dried_co.paa",
            "ChefZ\ChefZ_Sealife\fishfilet\data\EelFillet_burnt_co.paa"
        };
        hiddenSelections[] = {"cs_raw"};
        displayName = "#STR_CHEFZ_ITEM_EELFILLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_EELFILLET1";
        weight = 225;

        class Nutrition
        {
            fullnessIndex = 1.5;
            energy = 205;
            water = 32;
            nutritionalIndex = 22;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.5, 205, 32, 22, 0, 4, 1}; visual_properties[] = {0, 0, 0}; cooking_properties[] = {0, 0, 0}; };
                class Baked { nutrition_properties[] = {1.34, 390, 12, 34, 0, 0, 1}; visual_properties[] = {0, 1, 1}; cooking_properties[] = {100, 50, 200}; };
                class Boiled { nutrition_properties[] = {1.41, 360, 44, 34, 0, 0, 1}; visual_properties[] = {0, 2, 2}; cooking_properties[] = {100, 70, 150}; };
                class Dried { nutrition_properties[] = {1.12, 405, 4, 36, 0, 0, 1}; visual_properties[] = {0, 3, 3}; cooking_properties[] = {0, 0, 0}; };
                class Burned { nutrition_properties[] = {0.94, 80, 5, 5, 0, 0, 1}; visual_properties[] = {0, 4, 4}; cooking_properties[] = {200, 20, 0}; };
                class Rotten { nutrition_properties[] = {1.25, 120, 16, 5, 20, 16, 1}; visual_properties[] = {0, 4, 5}; cooking_properties[] = {0, 0, 0}; };
            };
            class FoodStageTransitions
            {
                class Raw
                {
                    class ChefZ_RawToBaked
                    {
                        transition_to = 2;
                        cooking_method = 1;
                    };
                    class ChefZ_RawToBoiled
                    {
                        transition_to = 3;
                        cooking_method = 2;
                    };
                };
            };
        };
    };

    //==========================================================================
    // ### SLICE baits ###   Die sieben Koeder.
    //
    // Begruendung zu BaitBase, class Fishing und den Proxy-Modellen: Kopf
    // dieser Datei unter "### SLICE baits ###".
    //==========================================================================

    class Inventory_Base;

    //--------------------------------------------------------------------------
    // Gemeinsame Basis aller sieben Koeder. scope = 0 - sie ist selbst kein
    // Gegenstand, sie traegt nur, was alle teilen.
    //
    // inventorySlot[] STEHT HIER und nicht an den Blaettern: die Engine prueft
    // beide Richtungen, und der Platz ist fuer alle sieben derselbe. "Bait" ist
    // Vanillas Slot am Haken (Slot_Bait) - er wird nicht neu angelegt, sonst
    // haetten zwei Addons denselben Slotnamen.
    //--------------------------------------------------------------------------
    class ChefZ_Bait_Base : Inventory_Base
    {
        scope = 0;
        rotationFlags = 17;
        itemSize[] = {1, 1};
        weight = 20;
        absorbency = 0.0;
        canBeDigged = 0;
        canBeSplit = 0;
        varQuantityDestroyOnMin = 0;
        repairableWithKits[] = {};
        lifetime = 43200;

        inventorySlot[] = {"Bait"};
    };

    //--------------------------------------------------------------------------
    // Kunstkoeder (baitKind "lure"). Bleibt am Haken und nimmt Schaden; der
    // Schaden steht als "lureWearPerUse" im Datensatz, nicht hier.
    //
    // Proxy: bone_bait.p3d, Vanillas BoneBait - ein Haken mit einem harten,
    // hellen Koerper daran. Das ist von allem, was DayZ mitbringt, einem
    // Blinker am naechsten. GEERBT WIRD BoneBait NICHT (siehe Kopf) - nur das
    // Modell wird geliehen.
    //--------------------------------------------------------------------------
    class ChefZ_BaitLure_Base : ChefZ_Bait_Base
    {
        scope = 0;
        model = "\dz\gear\crafting\bone_bait.p3d";
        weight = 25;
        // Hook: direkt an die Rute wie Vanillas Jig (consumables Jig, inventorySlot Hook).
        // Bait: weiterhin auch an einen Haken.
        inventorySlot[] = {"Hook", "Bait"};

        class Fishing
        {
            // 0 = TryBaitLoss() kehrt vor dem Wurf zurueck. Begruendung im Kopf.
            baitLossChanceMod = 0.0;
        };
    };

    //--------------------------------------------------------------------------
    // Weichkoeder (baitKind "soft"). Wird verbraucht wie Vanillas Wurm.
    //
    // Proxy: bait_worm_pinned.p3d, Vanillas Bait - ein Wurm am Haken. Ein
    // Gummiwurm sieht genau so aus.
    //--------------------------------------------------------------------------
    class ChefZ_BaitSoft_Base : ChefZ_Bait_Base
    {
        scope = 0;
        model = "\dz\gear\consumables\bait_worm_pinned.p3d";
        weight = 15;

        class Fishing
        {
            // 0 = "wie Wurm". Begruendung im Kopf: der Verbrauch geschieht
            // bedingungslos in OnAfterSpawnSignalHit/OnSignalPass, ein
            // zusaetzlicher Verlustwurf waere eine Verschlechterung.
            baitLossChanceMod = 0.0;
        };
    };

    //--------------------------------------------------------------------------
    // Die vier Kunstkoeder.
    //
    // Zielfische je Koeder stehen AUSSCHLIESSLICH in
    // ChefZ_Fishing/Config/Fishing/Baits.json. Hier steht kein einziger
    // Fischname - sonst gaebe es die Zuordnung zweimal, und zwei Zuordnungen
    // laufen auseinander.
    //--------------------------------------------------------------------------

    //! Der Loeffelblinker. Hebt Makrele, Lachs, Forelle und die schnellen
    //! Oberflaechenraeuber - der Klassiker unter den Schleppkoedern.
    class ChefZ_AntiqueSpoonFlasher : ChefZ_BaitLure_Base
    {
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\bait\models\antiquespoonflasher.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\bait\data\AntiqueSpoonFlasher_co.paa"};
        hiddenSelections[] = {"camo"};
        displayName = "#STR_CHEFZ_BAIT_ANTIQUESPOONFLASHER";
        descriptionShort = "#STR_CHEFZ_BAIT_ANTIQUESPOONFLASHER_DESC";
    };

    //! Der Tintenfischjig. Leuchtet rot im Dunkeln und lockt Kalmare und Kraken.
    class ChefZ_CrimsonSquidLure : ChefZ_BaitLure_Base
    {
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\bait\models\crimsonsquidlure.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\bait\data\CrimsonSquidLure_co.paa"};
        hiddenSelections[] = {"camo"};
        displayName = "#STR_CHEFZ_BAIT_CRIMSONSQUIDLURE";
        descriptionShort = "#STR_CHEFZ_BAIT_CRIMSONSQUIDLURE_DESC";
    };

    //! Der Garnelenjig (Egi). Der klassische Koeder fuer Kalmare, faengt auch Sepien und Brandungsraeuber.
    class ChefZ_NeonPrawnJig : ChefZ_BaitLure_Base
    {
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\bait\models\neonprawnjig.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\bait\data\NeonPrawnJig_co.paa"};
        hiddenSelections[] = {"camo"};
        displayName = "#STR_CHEFZ_BAIT_NEONPRAWNJIG";
        descriptionShort = "#STR_CHEFZ_BAIT_NEONPRAWNJIG_DESC";
    };

    //! Der Sardinenblinker. Silbernes Sardinenimitat fuer Makrele, Bonito und Tintenfisch.
    class ChefZ_SilverSardineLure : ChefZ_BaitLure_Base
    {
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\bait\models\silversardinelure.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\bait\data\SilverSardineLure_co.paa"};
        hiddenSelections[] = {"camo"};
        displayName = "#STR_CHEFZ_BAIT_SILVERSARDINELURE";
        descriptionShort = "#STR_CHEFZ_BAIT_SILVERSARDINELURE_DESC";
    };

    //! Der Fischimitat-Wobbler. Raubfisch im Suesswasser und in der Brandung.
    class ChefZ_MinnowLure : ChefZ_BaitLure_Base
    {
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\bait\models\minnowlure.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\bait\data\MinnowLure_co.paa"};
        hiddenSelections[] = {"camo"};
        displayName = "#STR_CHEFZ_BAIT_MINNOWLURE";
        descriptionShort = "#STR_CHEFZ_BAIT_MINNOWLURE_DESC";
    };

    //! Der Popper. Laeuft an der Oberflaeche und holt die Grossfische hoch.
    class ChefZ_RedTopperBait : ChefZ_BaitLure_Base
    {
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\bait\models\redtopperbait.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\bait\data\RedTopperBait_co.paa"};
        hiddenSelections[] = {"camo"};
        displayName = "#STR_CHEFZ_BAIT_REDTOPPERBAIT";
        descriptionShort = "#STR_CHEFZ_BAIT_REDTOPPERBAIT_DESC";
    };

    //! Der Spinner. Blattfahne am Draht, der Standardkoeder am Fluss.
    class ChefZ_Spinnerbait : ChefZ_BaitLure_Base
    {
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\bait\models\spinnerbait.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\bait\data\Spinnerbait_co.paa"};
        hiddenSelections[] = {"camo"};
        displayName = "#STR_CHEFZ_BAIT_SPINNERBAIT";
        descriptionShort = "#STR_CHEFZ_BAIT_SPINNERBAIT_DESC";
    };

    //--------------------------------------------------------------------------
    // Die drei Weichkoeder.
    //--------------------------------------------------------------------------

    //! Blauer Gummikoeder. Schwer und tief - Grundfisch ueber Riff und Sand.
    class ChefZ_BlueBait : ChefZ_BaitSoft_Base
    {
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\bait\models\bluebait.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\bait\data\BlueBait_co.paa"};
        hiddenSelections[] = {"camo"};
        displayName = "#STR_CHEFZ_BAIT_BLUEBAIT";
        descriptionShort = "#STR_CHEFZ_BAIT_BLUEBAIT_DESC";
    };

    //! Shad mit Schaufelschwanz. Plattfisch und alles, was am Boden liegt.
    class ChefZ_PaddleTailBait : ChefZ_BaitSoft_Base
    {
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\bait\models\paddletailbait.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\bait\data\PaddleTailBait_co.paa"};
        hiddenSelections[] = {"camo"};
        displayName = "#STR_CHEFZ_BAIT_PADDLETAILBAIT";
        descriptionShort = "#STR_CHEFZ_BAIT_PADDLETAILBAIT_DESC";
    };

    //! Forellenteig. Riecht suess, sitzt weich am Haken.
    class ChefZ_TroutBait : ChefZ_BaitSoft_Base
    {
        scope = 2;
        model = "\ChefZ\ChefZ_Sealife\bait\models\troutbait.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\bait\data\TroutBait_co.paa"};
        hiddenSelections[] = {"camo"};
        displayName = "#STR_CHEFZ_BAIT_TROUTBAIT";
        descriptionShort = "#STR_CHEFZ_BAIT_TROUTBAIT_DESC";
    };


    //==========================================================================
    // ### SLICE fish-fresh ###  Suesswasser und Wanderfische, acht Arten
    //==========================================================================
    //
    // WAS EIN GANZER FISCH IST - und was er ausdruecklich nicht ist
    // -------------------------------------------------------------------------
    // Er ist ein KADAVER, kein Gericht. Vanilla sagt das an seinen eigenen
    // Fischen woertlich (scripts - 1.29,
    // 4_World/DayZ/Entities/ItemBase/Edible_Base/Mackerel.c und Carp.c):
    //
    //     override bool CanBeCookedOnStick() { return false; }
    //     override bool CanBeCooked()        { return false; }
    //     override bool IsCorpse()           { return true;  }
    //     override bool CanDecay()           { return true;  }
    //
    // Deshalb traegt KEINE der acht Klassen hier einen Knoten Food > FoodStages
    // oder Food > FoodStageTransitions. Ein ganzer Fisch wandert nicht in den
    // Topf - er wird filetiert (PROCESS_FILLET_FISH weiter unten), und das
    // FILET ist die kochbare Klasse. Sie gehoert dem Slice "fillets" und bringt
    // ihre Garstufen selbst mit. Dass hier keine stehen, ist damit eine
    // ENTSCHEIDUNG und keine Luecke: waeren sie da, ohne dass irgendwo
    // CanBeCooked() zusagt, waeren sie toter Text (01 V4).
    //
    // class Nutrition steht trotzdem an jeder Klasse. PlayerStomach.InitData
    // registriert ausschliesslich Klassen mit "Nutrition" ODER "Food" und
    // scope != 0 (01 V7, PlayerStomach.c:208-250); ohne den Block verschwaende
    // ein roh gegessener Fisch lautlos, ohne zu saettigen und ohne Meldung.
    // agents = 4 ist eAgents.SALMONELLA - roher Fisch ist roh.
    //
    // MENGENSKALA NACH GROESSE, nicht pauschal.
    // ChefZ_FishingYieldDef.quality landet als SetQuantityNormalized am
    // gespawnten Objekt (21 §2.2) - die Zahl in den Daten ist der ANTEIL,
    // varQuantityMax hier die SPANNE. Ein Lachs mit quality 0.7 auf 400 wiegt
    // damit mehr als ein Flussbarsch mit 0.30 auf 150, und beide Zahlen stehen
    // an der Stelle, an die sie gehoeren.
    //
    // 3D: die Lieferung unter Psyerns_ChefZ/ChefZ/ enthaelt zum 14.09.2026 KEIN
    // Fischmesh (geprueft: kein *fish*, kein *fillet*). Alle acht tragen ein
    // VANILLA-PROXY aus gear_food.pbo. Das Praefix "DZ\gear\food" und die
    // Dateinamen sind AUS DEM PBO GELESEN, nicht geraten:
    //
    //   sardines_live.p3d         klein, schlank        -> Flussbarsch
    //   carp_live.p3d             gross, hochrueckig    -> Muskellunge
    //   steelhead_trout_live.p3d  Salmonide             -> Regenbogenforelle,
    //                             Saibling, Lachs, Sockeye
    //                             (die Steelhead IST eine Regenbogenforelle)
    //   walleye_pollock_live.p3d  langgestreckt         -> Snakehead, Aal
    //
    // Der Bedarf an eigenen Meshes steht im Slice-Bericht. Auf ein Modell
    // wartet hier nichts.
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Gemeinsame Configbasis der acht. scope = 0 - sie ist kein Gegenstand.
    //
    // Configbasis ist die VANILLA-Klasse Edible_Base, Skriptbasis ist
    // ChefZ_Edible_Base - genau die Andockregel aus dem Kopf von
    // ChefZ_Edible_Base.c. Der Core bringt keine solche Basis mit (I3).
    //
    // Der Name traegt bewusst "Freshwater" und nicht nur "Fish": fish-sea-a und
    // fish-sea-b schreiben in dieselbe Datei und brauchen ihre eigene Basis.
    // Ein gemeinsames ChefZ_Fish_Base waere eine Klasse, die drei Agenten
    // gleichzeitig definieren - also eine doppelte Definition.
    //--------------------------------------------------------------------------
    class ChefZ_FreshwaterFish_Base : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\food\carp_live.p3d";
        rotationFlags = 17;
        itemSize[] = {3, 2};
        weight = 800;
        absorbency = 0.7;
        varQuantityInit = 250;
        varQuantityMin = 0;
        varQuantityMax = 250;
        varQuantityDestroyOnMin = 1;
        quantityBar = 1;
        canBeSplit = 0;
        isMeleeWeapon = 0;
        lifetime = 3600;
    };

    //! Flussbarsch. Der haeufigste Fisch am Ufer - baseWeight 40, knapp unter
    //! Vanillas Makrele (42). Ein Filet.
    class ChefZ_Perch : ChefZ_FreshwaterFish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_PERCH0";
        descriptionShort = "#STR_CHEFZ_FISH_FRESH_DESC_WHOLE";
        model = "\ChefZ\ChefZ_Sealife\fish\models\perch.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Perch_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {2, 1};
        weight = 300;
        varQuantityInit = 150;
        varQuantityMax = 150;

        class Nutrition
        {
            fullnessIndex = 1.05;
            energy = 120;
            water = 55;
            nutritionalIndex = 12;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Muskellunge. Geht NUR auf den Kunstkoeder (lureOnly) - baseWeight 6, der
    //! seltenste Fisch dieses Slice. Drei Filets.
    class ChefZ_Muskellunge : ChefZ_FreshwaterFish_Base
    {
        scope = 2;
        itemBehaviour = 2;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_ITEM_MUSKELLUNGE0";
        descriptionShort = "#STR_CHEFZ_FISH_FRESH_DESC_WHOLE";
        model = "\ChefZ\ChefZ_Sealife\fish\models\muskellunge.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Muskellunge_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 2};
        weight = 1400;
        varQuantityInit = 400;
        varQuantityMax = 400;

        class Nutrition
        {
            fullnessIndex = 1.15;
            energy = 130;
            water = 52;
            nutritionalIndex = 13;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Northern Snakehead. Zaeh, langgestreckt, beisst auf Wobbler und Spinner.
    //! Zwei Filets.
    class ChefZ_Snakehead : ChefZ_FreshwaterFish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SNAKEHEAD0";
        descriptionShort = "#STR_CHEFZ_FISH_FRESH_DESC_WHOLE";
        model = "\ChefZ\ChefZ_Sealife\fish\models\snakehead.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Snakehead_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 2};
        weight = 700;

        class Nutrition
        {
            fullnessIndex = 1.10;
            energy = 140;
            water = 50;
            nutritionalIndex = 14;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Regenbogenforelle. Zwei Filets. Proxy ist die Steelhead - dieselbe Art.
    class ChefZ_RainbowTrout : ChefZ_FreshwaterFish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_RAINBOWTROUT0";
        descriptionShort = "#STR_CHEFZ_FISH_FRESH_DESC_WHOLE";
        model = "\ChefZ\ChefZ_Sealife\fish\models\rainbowtrout.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\RainbowTrout_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 2};
        weight = 650;

        class Nutrition
        {
            fullnessIndex = 1.10;
            energy = 165;
            water = 48;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Saibling. Kaltes, klares Wasser. Zwei Filets.
    class ChefZ_Char : ChefZ_FreshwaterFish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHAR0";
        descriptionShort = "#STR_CHEFZ_FISH_FRESH_DESC_WHOLE";
        model = "\ChefZ\ChefZ_Sealife\fish\models\char.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Char_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 2};
        weight = 600;

        class Nutrition
        {
            fullnessIndex = 1.10;
            energy = 160;
            water = 48;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Lachs. Wanderfisch - enviro BOTH, im Meer wie im Fluss. Drei Filets.
    class ChefZ_Salmon : ChefZ_FreshwaterFish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SALMON0";
        descriptionShort = "#STR_CHEFZ_FISH_FRESH_DESC_WHOLE";
        model = "\ChefZ\ChefZ_Sealife\fish\models\salmon.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Salmon_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 2};
        weight = 1200;
        varQuantityInit = 400;
        varQuantityMax = 400;

        class Nutrition
        {
            fullnessIndex = 1.20;
            energy = 200;
            water = 45;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Sockeye. Der zweite Wanderfisch - enviro BOTH. Drei Filets.
    class ChefZ_SockeyeSalmon : ChefZ_FreshwaterFish_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SOCKEYESALMON0";
        descriptionShort = "#STR_CHEFZ_FISH_FRESH_DESC_WHOLE";
        model = "\ChefZ\ChefZ_Sealife\fish\models\sockeyesalmon.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026; hingelegt und zentriert 30.10.2026, sonst leere Vorschau)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\SockeyeSalmon_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 2};
        weight = 1100;
        varQuantityInit = 400;
        varQuantityMax = 400;

        class Nutrition
        {
            fullnessIndex = 1.20;
            energy = 190;
            water = 45;
            nutritionalIndex = 19;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Aal. Fett, naehrstoffreich, nachtaktiv (hourlyCoefs im Datensatz).
    //! Zwei Filets - und sie heissen ChefZ_EelFillet, dieselbe Klasse, die der
    //! Salzwasser-Aal des Slice fish-sea-a ausgibt. EINE Filetklasse fuer ZWEI
    //! Fischklassen, abgestimmt mit fish-sea-a; definiert wird sie vom Slice
    //! "fillets", hier wird sie nur referenziert.
    class ChefZ_Eel : ChefZ_FreshwaterFish_Base
    {
        scope = 2;
        itemBehaviour = 2;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_ITEM_EEL0";
        descriptionShort = "#STR_CHEFZ_FISH_FRESH_DESC_WHOLE";
        model = "\ChefZ\ChefZ_Sealife\fish\models\eel.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Eel_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 2};
        weight = 750;

        class Nutrition
        {
            fullnessIndex = 1.25;
            energy = 260;
            water = 40;
            nutritionalIndex = 22;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };


    //==========================================================================
    // ### SLICE fish-sea-a ###  SALZWASSER A - Nordsee und Atlantik
    //
    // 13 Arten, davon zwoelf eigene Klassen. Die MAKRELE ist Vanillas
    // "Mackerel" und steht deshalb NICHT hier: sie wird ueber den
    // fishYield-Record (overrideExisting) und die Zutatenbindung erweitert,
    // nicht dupliziert (Slice-Plan Paragraph 3).
    //
    // KLASSENNAMEN ChefZ_Fish_<Key>: so nennt sie der Slice-Plan, so stehen
    // sie in den bereits ausgelieferten Koederdaten dieses Moduls
    // (Config/Fishing/Baits.json, "targets") und in den Filet-Transforms der
    // Nachbarslices. Der Unterstrich in der Mitte verletzt die Regex in
    // tools/chefz-validate/naming.mjs (dort ist nur die Endung "_Base"
    // vorgesehen); das ist ein bekannter, im Slice-Bericht benannter Befund.
    // Ein Alleingang mit "ChefZ_FishCod" haette die Koederpraeferenz fuer alle
    // zwoelf Arten STILL fallen lassen - ein unbekanntes Ziel ist im
    // bait-Record nur eine Warnung (21 Paragraph 3.4).
    //
    // WARUM DIE BASIS EINEN SLICE-NAMEN TRAEGT: in dasselbe Modul schreiben
    // vier Slices (fillets, baits, fish-sea-b, fish-fresh). Eine gemeinsam
    // benannte "ChefZ_Fish_Base" waere mehrfach definiert worden - fuer
    // configcpp.mjs eine doppelte Klassendefinition, fuer die Engine ein
    // Zufall, welche gewinnt. ChefZ_Fish_SeaB_Base und
    // ChefZ_FreshwaterFish_Base der Nachbarslices folgen derselben Regel.
    //
    // Sie heisst ChefZ_SaltwaterFishA_Base und nicht ChefZ_Fish_SeaA_Base,
    // weil an IHR nichts haengt: kein Koederziel, kein Transformeingang, keine
    // Zutatenbindung. Wo die Konvention aus Paragraph 53 eingehalten werden
    // KANN, ohne Daten zu brechen, wird sie eingehalten - der Unterstrich
    // bleibt genau dort, wo die ausgelieferten Daten ihn bereits verlangen.
    //
    // 3D: es gibt kein eigenes Fischmodell. Jede Klasse traegt ein BELEGTES
    // Vanilla-Proxy - kleine Arten \dz\gear\food\sardines_live.p3d, mittlere
    // und grosse \dz\gear\food\mackerel_live.p3d. Die Pfade sind nicht
    // geraten, sondern aus ausgelieferten Fremdconfigs unter
    // "Mod Repositories" belegt. Der Bedarf an eigener Geometrie steht im
    // Slice-Bericht; gewartet wird auf nichts.
    //==========================================================================

    // ------------------------------------------------------------------------
    // Gemeinsame Configbasis der ganzen Fische dieses Slice.
    //
    // scope = 0: sie ist kein Item, sondern die Stelle, an der Mengenskala,
    // Garstufen und Garstufenuebergaenge EINMAL stehen.
    //
    //   class Nutrition            01 V7 - PlayerStomach.InitData registriert
    //                              nur Klassen mit Nutrition ODER Food und
    //                              scope != 0. Fehlt der Knoten, verschwindet
    //                              der Bissen STILL.
    //   class Food > FoodStages    ohne sie entsteht kein FoodStage-Objekt,
    //                              der Kochtakt greift ins Leere.
    //   class FoodStageTransitions 01 V4 - ohne sie faellt
    //                              FoodStage.GetNextFoodStageType auf BURNED
    //                              zurueck (FoodStage.c:472): der Fisch
    //                              verbrennt beim ersten Garstufenwechsel.
    //
    // MENGENSKALA 300 / 500 / 800 nach Groesse, aus demselben Grund wie die
    // 250 des Fleischmoduls (PlayerStomach.c:92, energy_per_unit =
    // GetEnergy() / 100): erst eine Menge in der Groessenordnung 100+ macht
    // die Energiezahl zu dem, was sie behauptet. Der fishYield-Record setzt
    // ueber "quality" die Startmenge als Anteil davon - das ist die
    // Fischgroesse beim Fang (21 Paragraph 2.1).
    //
    // GANZER FISCH IST KOCHBAR. Vanillas Mackerel.c sagt CanBeCooked() ==
    // false, weil ein ganzer Fisch dort nur Rohstoff ist. In ChefZ ist er
    // Nahrung: ein Fisch am Feuer ist die zweite, langsamere Verwendung neben
    // dem Filetieren. Die Garzeiten unten sind deshalb laenger als die eines
    // Filets.
    // ------------------------------------------------------------------------
    class ChefZ_SaltwaterFishA_Base : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\food\mackerel_live.p3d";
        rotationFlags = 17;
        itemSize[] = {3, 1};
        weight = 1100;
        absorbency = 0.7;
        isMeleeWeapon = 0;
        soundImpactType = "food";
        lifetime = 14400;
        varQuantityInit = 500;
        varQuantityMin = 0;
        varQuantityMax = 500;
        varQuantityDestroyOnMin = 1;
        quantityBar = 1;
        canBeSplit = 0;

        // KEIN class Food (E-05, 14.09.2026): ein ganzer Fisch wandert nicht in
        // den Topf - erst das Filet ist kochbar. Dieselbe Bauform wie
        // ChefZ_FreshwaterFish_Base und ChefZ_SeaFishB_Base.
    };

    // Plaice (Scholle) - mittel, Basisgewicht 30, 2 Filets (ChefZ_PlaiceFillet).
    class ChefZ_Plaice : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_PLAICE0";
        descriptionShort = "#STR_CHEFZ_ITEM_PLAICE1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\plaice.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Plaice_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 1};
        weight = 1100;
        varQuantityInit = 500;
        varQuantityMax = 500;
        class Nutrition
        {
            fullnessIndex = 0.9;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // GiltheadBream (Dorade) - mittel, Basisgewicht 24, 2 Filets (ChefZ_GiltheadBreamFillet).
    class ChefZ_GiltheadBream : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_GILTHEADBREAM0";
        descriptionShort = "#STR_CHEFZ_ITEM_GILTHEADBREAM1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\giltheadbream.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\GiltheadBream_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 1};
        weight = 1100;
        varQuantityInit = 500;
        varQuantityMax = 500;
        class Nutrition
        {
            fullnessIndex = 0.9;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // Halibut (Heilbutt) - gross, Basisgewicht 8, 3 Filets (ChefZ_HalibutFillet).
    class ChefZ_Halibut : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HALIBUT0";
        descriptionShort = "#STR_CHEFZ_ITEM_HALIBUT1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\halibut.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Halibut_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 2};
        weight = 3000;
        varQuantityInit = 800;
        varQuantityMax = 800;
        class Nutrition
        {
            fullnessIndex = 0.8;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // Cod (Kabeljau) - mittel, Basisgewicht 34, 2 Filets (ChefZ_CodFillet).
    class ChefZ_Cod : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        itemBehaviour = 2;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_ITEM_COD0";
        descriptionShort = "#STR_CHEFZ_ITEM_COD1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\cod.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Cod_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 1};
        weight = 1100;
        varQuantityInit = 500;
        varQuantityMax = 500;
        class Nutrition
        {
            fullnessIndex = 0.9;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // Sole (Seezunge) - mittel, Basisgewicht 22, 2 Filets (ChefZ_SoleFillet).
    class ChefZ_Sole : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SOLE0";
        descriptionShort = "#STR_CHEFZ_ITEM_SOLE1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\sole.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026; hingelegt und zentriert 30.10.2026, sonst leere Vorschau)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Sole_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 1};
        weight = 1100;
        varQuantityInit = 500;
        varQuantityMax = 500;
        class Nutrition
        {
            fullnessIndex = 0.9;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // SeaBass (Wolfsbarsch) - mittel, Basisgewicht 26, 2 Filets (ChefZ_SeaBassFillet).
    class ChefZ_SeaBass : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SEABASS0";
        descriptionShort = "#STR_CHEFZ_ITEM_SEABASS1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\seabass.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\SeaBass_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 1};
        weight = 1100;
        varQuantityInit = 500;
        varQuantityMax = 500;
        class Nutrition
        {
            fullnessIndex = 0.9;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // RedMullet (Meerbarbe) - klein, Basisgewicht 28, 1 Filet (ChefZ_RedMulletFillet).
    class ChefZ_RedMullet : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_REDMULLET0";
        descriptionShort = "#STR_CHEFZ_ITEM_REDMULLET1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\redmullet.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\RedMullet_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {2, 1};
        weight = 400;
        varQuantityInit = 300;
        varQuantityMax = 300;
        class Nutrition
        {
            fullnessIndex = 1;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // Conger (Conger) - gross, Basisgewicht 10, 3 Filets (ChefZ_CongerFillet).
    class ChefZ_Conger : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CONGER0";
        descriptionShort = "#STR_CHEFZ_ITEM_CONGER1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\conger.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Conger_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 2};
        weight = 3000;
        varQuantityInit = 800;
        varQuantityMax = 800;
        class Nutrition
        {
            fullnessIndex = 0.8;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // SeaEel (Salzwasser-Aal) - mittel, Basisgewicht 14, 2 Filets (ChefZ_EelFillet).
    // Teilt sich das Filet mit dem Suesswasser-Aal (Slice-Plan Paragraph 3):
    // ein Aalfilet ist ein Aalfilet, und ChefZ_EelFillet gibt es bereits.
    class ChefZ_SeaEel : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        itemBehaviour = 2;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_ITEM_SEAEEL0";
        descriptionShort = "#STR_CHEFZ_ITEM_SEAEEL1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\seaeel.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\SeaEel_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 1};
        weight = 1100;
        varQuantityInit = 500;
        varQuantityMax = 500;
        class Nutrition
        {
            fullnessIndex = 0.9;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // HorseMackerel (Stoeckermakrele) - klein, Basisgewicht 30, 1 Filet (ChefZ_HorseMackerelFillet).
    class ChefZ_HorseMackerel : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HORSEMACKEREL0";
        descriptionShort = "#STR_CHEFZ_ITEM_HORSEMACKEREL1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\horsemackerel.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\HorseMackerel_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {2, 1};
        weight = 400;
        varQuantityInit = 300;
        varQuantityMax = 300;
        class Nutrition
        {
            fullnessIndex = 1;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // JapaneseSardine (Japanische Sardine) - klein, Basisgewicht 30, 1 Filet (ChefZ_JapaneseSardineFillet).
    class ChefZ_JapaneseSardine : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_JAPANESESARDINE0";
        descriptionShort = "#STR_CHEFZ_ITEM_JAPANESESARDINE1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\japanesesardine.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\JapaneseSardine_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {2, 1};
        weight = 400;
        varQuantityInit = 300;
        varQuantityMax = 300;
        class Nutrition
        {
            fullnessIndex = 1;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    // Rockfish (Felsenbarsch) - klein, Basisgewicht 22, 1 Filet (ChefZ_RockfishFillet).
    class ChefZ_Rockfish : ChefZ_SaltwaterFishA_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_ROCKFISH0";
        descriptionShort = "#STR_CHEFZ_ITEM_ROCKFISH1";
        model = "\ChefZ\ChefZ_Sealife\fish\models\rockfish.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Rockfish_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {2, 1};
        weight = 400;
        varQuantityInit = 300;
        varQuantityMax = 300;
        class Nutrition
        {
            fullnessIndex = 1;
            energy = 105;
            water = 60;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };


    //==========================================================================
    // ### SLICE fish-sea-b ###  Salzwasser B - Grossfisch und Tropen, elf Arten
    //==========================================================================
    //
    // DIESELBE ENTSCHEIDUNG WIE BEI fish-fresh, und aus demselben Beleg:
    // ein ganzer Fisch ist ein KADAVER, kein Gericht. Vanilla schreibt das an
    // Mackerel.c und Carp.c woertlich hin -
    //
    //     override bool CanBeCooked() { return false; }
    //     override bool IsCorpse()    { return true;  }
    //
    // - deshalb traegt keine der elf Klassen einen Knoten Food > FoodStages
    // oder Food > FoodStageTransitions. Der Weg fuehrt ueber das Messer:
    // PROCESS_FILLET_FISH macht aus dem Fisch Filets, und das FILET ist die
    // kochbare Klasse. Sie gehoert dem Slice "fillets" und bringt ihre
    // Garstufen selbst mit.
    //
    // class Nutrition steht trotzdem an jeder Klasse: PlayerStomach.InitData
    // registriert ausschliesslich Klassen mit "Nutrition" ODER "Food" und
    // scope != 0 (01 V7). Ohne den Block saettigte ein roh gegessener Fisch
    // lautlos nicht. agents = 4 ist eAgents.SALMONELLA - roher Fisch ist roh.
    //
    // MENGENSKALA NACH GROESSE, geeicht an fish-fresh (Forelle 250, Lachs 400):
    // medium 250, large 400, game 600. ChefZ_FishingYieldDef.quality landet als
    // SetQuantityNormalized am gespawnten Objekt (21 §2.2) - die Zahl in den
    // Daten ist der ANTEIL, varQuantityMax hier die SPANNE. Ein Blauer Marlin
    // mit quality 1.00 auf 600 ist damit das schwerste Stueck des Moduls, und
    // beide Zahlen stehen an der Stelle, an die sie gehoeren.
    //
    // TAGESZEIT: die hourlyCoefs stehen in Fish_Sea_B.json, nicht hier. Drei
    // Kurven, und der Sinn des Feldes ist Vanillas eigener:
    // Math.Lerp(CYCLE_LENGTH_MIN, CYCLE_LENGTH_MAX, coef) - ein HOHER Wert ist
    // ein LANGER Zyklus, also schlechtes Beissen (YieldsFish.c:16).
    //
    //   DAY    Thun, Marlin, Segelfisch, Mahi-Mahi, Bonito - Sichtjaeger im
    //          Freiwasser, tags 0, nachts 1.
    //   CREP   Red Snapper, Giant Trevally, Buri - Vanillas eigene Kurve,
    //          Daemmerung.
    //   NIGHT  Grouper und Tarpon - Lauerjaeger, tags 1, nachts 0.
    //
    // 3D: die Lieferung unter Psyerns_ChefZ/ChefZ/ enthaelt zum 14.09.2026 KEIN
    // Fischmesh (geprueft: kein *fish*, *tuna*, *marlin*, *fillet*). Alle elf
    // tragen ein VANILLA-PROXY aus gear_food.pbo; beide Pfade sind belegt an
    // der ausgelieferten Vanilla-Configkopie in Mod Repositories/
    // DayZExpansion/Objects/Structures/BuilderItems/DZ/statics/Core/
    // config.cpp:1092 (carp_live) und :1136 (mackerel_live):
    //
    //   mackerel_live.p3d   torpedofoermig  -> Thun, Bonito, Marlin,
    //                                          Segelfisch, Buri
    //   carp_live.p3d       hochrueckig     -> Red Snapper, Grouper,
    //                                          Mahi-Mahi, Trevally, Tarpon
    //
    // Der Bedarf an eigenen Meshes steht im Slice-Bericht. Auf ein Modell
    // wartet hier nichts.
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Gemeinsame Configbasis der elf. scope = 0 - sie ist kein Gegenstand.
    //
    // Configbasis ist die VANILLA-Klasse Edible_Base, Skriptbasis ist
    // ChefZ_Edible_Base (Andockregel im Kopf von ChefZ_Edible_Base.c). Der Name
    // traegt "SeaFishB" und nicht nur "Fish": fish-sea-a und fish-fresh
    // schreiben in dieselbe Datei und fuehren ihre eigenen Basen. Ein
    // gemeinsames ChefZ_Fish_Base waere eine Klasse, die drei Agenten
    // gleichzeitig definieren - also eine doppelte Definition.
    //--------------------------------------------------------------------------
    class ChefZ_SeaFishB_Base : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\food\mackerel_live.p3d";
        rotationFlags = 17;
        itemSize[] = {3, 2};
        weight = 700;
        absorbency = 0.7;
        varQuantityInit = 250;
        varQuantityMin = 0;
        varQuantityMax = 250;
        varQuantityDestroyOnMin = 1;
        quantityBar = 1;
        canBeSplit = 0;
        isMeleeWeapon = 0;
        lifetime = 3600;
    };

    //! Fettreich, der Koenig der Freiwasserfische. Vier Filets.
    class ChefZ_BluefinTuna : ChefZ_SeaFishB_Base
    {
        scope = 2;
        itemBehaviour = 0;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_FISH_BLUEFINTUNA";
        descriptionShort = "#STR_CHEFZ_FISH_BLUEFINTUNA_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\bluefintuna.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\BluefinTuna_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 3};
        weight = 2500;
        varQuantityInit = 600;
        varQuantityMax = 600;

        class Nutrition
        {
            fullnessIndex = 1.35;
            energy = 300;
            water = 40;
            nutritionalIndex = 24;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Magerer als der Rote Thun, dafuer etwas haeufiger. Vier Filets.
    class ChefZ_YellowfinTuna : ChefZ_SeaFishB_Base
    {
        scope = 2;
        itemBehaviour = 0;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_FISH_YELLOWFINTUNA";
        descriptionShort = "#STR_CHEFZ_FISH_YELLOWFINTUNA_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\yellowfintuna.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\YellowfinTuna_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 3};
        weight = 2500;
        varQuantityInit = 600;
        varQuantityMax = 600;

        class Nutrition
        {
            fullnessIndex = 1.35;
            energy = 285;
            water = 40;
            nutritionalIndex = 24;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Riffbewohner, geht auch auf Weichkoeder und in die grosse Reuse. Zwei Filets.
    class ChefZ_RedSnapper : ChefZ_SeaFishB_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_FISH_REDSNAPPER";
        descriptionShort = "#STR_CHEFZ_FISH_REDSNAPPER_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\redsnapper.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 27.10.2026; hingelegt und zentriert 30.10.2026, sonst leere Vorschau)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\RedSnapper_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 2};
        weight = 700;
        varQuantityInit = 250;
        varQuantityMax = 250;

        class Nutrition
        {
            fullnessIndex = 1.10;
            energy = 175;
            water = 46;
            nutritionalIndex = 17;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Lauerjaeger am Riff, nachtaktiv. Beisst auf grosse Weichkoeder - deshalb NICHT lureOnly. Vier Filets.
    class ChefZ_GiantGrouper : ChefZ_SeaFishB_Base
    {
        scope = 2;
        itemBehaviour = 0;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_FISH_GIANTGROUPER";
        descriptionShort = "#STR_CHEFZ_FISH_GIANTGROUPER_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\giantgrouper.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\GiantGrouper_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 3};
        weight = 2500;
        varQuantityInit = 600;
        varQuantityMax = 600;

        class Nutrition
        {
            fullnessIndex = 1.35;
            energy = 260;
            water = 40;
            nutritionalIndex = 24;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Goldmakrele, Sichtjaeger an der Oberflaeche. Drei Filets.
    class ChefZ_MahiMahi : ChefZ_SeaFishB_Base
    {
        scope = 2;
        itemBehaviour = 2;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_FISH_MAHIMAHI";
        descriptionShort = "#STR_CHEFZ_FISH_MAHIMAHI_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\mahimahi.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\MahiMahi_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 2};
        weight = 1300;
        varQuantityInit = 400;
        varQuantityMax = 400;

        class Nutrition
        {
            fullnessIndex = 1.20;
            energy = 200;
            water = 44;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Der Raeuber der Brandung, beisst in der Daemmerung. Drei Filets.
    class ChefZ_GiantTrevally : ChefZ_SeaFishB_Base
    {
        scope = 2;
        itemBehaviour = 2;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_FISH_GIANTTREVALLY";
        descriptionShort = "#STR_CHEFZ_FISH_GIANTTREVALLY_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\gianttrevally.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\GiantTrevally_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 2};
        weight = 1300;
        varQuantityInit = 400;
        varQuantityMax = 400;

        class Nutrition
        {
            fullnessIndex = 1.20;
            energy = 205;
            water = 44;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Der seltenste Fisch des Slice (baseWeight 2). Vier Filets.
    class ChefZ_BlueMarlin : ChefZ_SeaFishB_Base
    {
        scope = 2;
        itemBehaviour = 0;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_FISH_BLUEMARLIN";
        descriptionShort = "#STR_CHEFZ_FISH_BLUEMARLIN_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\bluemarlin.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\BlueMarlin_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 3};
        weight = 2500;
        varQuantityInit = 600;
        varQuantityMax = 600;

        class Nutrition
        {
            fullnessIndex = 1.35;
            energy = 270;
            water = 40;
            nutritionalIndex = 24;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! So selten wie der Marlin, jagt im hellen Tageslicht. Vier Filets.
    class ChefZ_Sailfish : ChefZ_SeaFishB_Base
    {
        scope = 2;
        itemBehaviour = 0;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_FISH_SAILFISH";
        descriptionShort = "#STR_CHEFZ_FISH_SAILFISH_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\sailfish.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Sailfish_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 3};
        weight = 2500;
        varQuantityInit = 600;
        varQuantityMax = 600;

        class Nutrition
        {
            fullnessIndex = 1.35;
            energy = 265;
            water = 40;
            nutritionalIndex = 24;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Schwarmfisch, der haeufigste des Slice. Auch in der grossen Reuse. Zwei Filets.
    class ChefZ_Bonito : ChefZ_SeaFishB_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_FISH_BONITO";
        descriptionShort = "#STR_CHEFZ_FISH_BONITO_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\bonito.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Bonito_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {3, 2};
        weight = 700;
        varQuantityInit = 250;
        varQuantityMax = 250;

        class Nutrition
        {
            fullnessIndex = 1.10;
            energy = 185;
            water = 46;
            nutritionalIndex = 17;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Buri - fett, begehrt, beisst in der Daemmerung. Drei Filets.
    class ChefZ_Yellowtail : ChefZ_SeaFishB_Base
    {
        scope = 2;
        itemBehaviour = 2;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_FISH_YELLOWTAIL";
        descriptionShort = "#STR_CHEFZ_FISH_YELLOWTAIL_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\yellowtail.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Yellowtail_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 2};
        weight = 1300;
        varQuantityInit = 400;
        varQuantityMax = 400;

        class Nutrition
        {
            fullnessIndex = 1.20;
            energy = 225;
            water = 44;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //! Nachtjaeger im flachen Wasser. Graetenreich - der schlechteste Naehrwert unter den Grossfischen, und trotzdem vier Filets.
    class ChefZ_Tarpon : ChefZ_SeaFishB_Base
    {
        scope = 2;
        itemBehaviour = 0;   // Haltung: ChefZ_HandProfiles.c
        displayName = "#STR_CHEFZ_FISH_TARPON";
        descriptionShort = "#STR_CHEFZ_FISH_TARPON_DESC";
        model = "\ChefZ\ChefZ_Sealife\fish\models\tarpon.p3d";   // EIGENES MODELL (Lieferung 25.10.2026, gebunden 26.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\fish\data\Tarpon_co.paa"};
        hiddenSelections[] = {"camo"};
        itemSize[] = {4, 3};
        weight = 2500;
        varQuantityInit = 600;
        varQuantityMax = 600;

        class Nutrition
        {
            fullnessIndex = 1.35;
            energy = 240;
            water = 45;
            nutritionalIndex = 18;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };
    };

    //--------------------------------------------------------------------------
    // MEERESFRUECHTE (Lieferung 25.10.2026, Klassen 26.10.2026).
    // Krebse, Hummer, Garnelen und Tintenfische mit eigenen Modellen aus
    // ChefZ_Sealife. Sie werden GANZ gegart wie Vanillas Sardines - in Pfanne,
    // Topf, am Spiess und im Raeucherofen. Garstufen und Naehrwerte sind die
    // von Sardines (P:\DZ\gear\food\config.cpp), je 100 Einheiten.
    // Fang: Config/Fishing/Seafood.json, Zutat: Config/Ingredients/Seafood.json.
    //
    // Die Modelle haben EINE Textur (camo) und kein rvmat. FoodStage.
    // UpdateVisualsEx setzt trotzdem Textur und Material je Stufe; deshalb
    // steht die eine Textur und ein neutrales Material in den Listen, und alle
    // Stufen zeigen auf Index 0.
    //--------------------------------------------------------------------------
    class ChefZ_Seafood_Base : Edible_Base
    {
        scope = 0;
        rotationFlags = 17;
        itemSize[] = {2, 2};
        weight = 400;
        absorbency = 0.7;
        isMeleeWeapon = 0;
        soundImpactType = "organic";
        lifetime = 14400;
        varQuantityInit = 250;
        varQuantityMin = 0;
        varQuantityMax = 250;
        varQuantityDestroyOnMin = 1;
        quantityBar = 1;
        canBeSplit = 0;
        inventorySlot[] = {"DirectCookingA", "DirectCookingB", "DirectCookingC", "SmokingA", "SmokingB", "SmokingC", "SmokingD", "ChefZ_SmokeHook05", "ChefZ_SmokeHook06", "ChefZ_SmokeHook07", "ChefZ_SmokeHook08", "ChefZ_SmokeHook09", "ChefZ_SmokeHook10", "ChefZ_SmokeHook11", "ChefZ_SmokeHook12", "ChefZ_SmokeHook13", "ChefZ_SmokeHook14", "ChefZ_SmokeHook15", "ChefZ_SmokeHook16", "ChefZ_SmokeHook17", "ChefZ_SmokeHook18"};
        hiddenSelections[] = {"camo"};
        // Garstufen (27.10.2026): visual_properties[1] waehlt die Textur der Klasse
        // (roh, gebraten, gekocht, geraeuchert, verbrannt); Material 0-4 neutral,
        // Material 5 = Vanillas Faeulnis-Material (Rotten zeigt das Faeulnis-Overlay).
        hiddenSelectionsMaterials[] = {"dz\data\data\default.rvmat", "dz\data\data\default.rvmat", "dz\data\data\default.rvmat", "dz\data\data\default.rvmat", "dz\data\data\default.rvmat", "dz\gear\food\data\meat_steak_rotten.rvmat"};

        class Nutrition
        {
            fullnessIndex = 5;
            energy = 20;
            water = 20;
            nutritionalIndex = 1;
            toxicity = 0;
            agents = 16;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw
                {
                    visual_properties[] = {0, 0, 0};
                    nutrition_properties[] = {5, 20, 20, 1, 0, 16, 1, 8};
                    cooking_properties[] = {0, 0};
                };
                class Rotten
                {
                    visual_properties[] = {0, 0, 5};
                    nutrition_properties[] = {10, 10, 8, 1, 0, 20, 1, 16};
                    cooking_properties[] = {0, 0};
                };
                class Baked
                {
                    visual_properties[] = {0, 1, 0};
                    nutrition_properties[] = {1, 100, 12, 1, 0};
                    cooking_properties[] = {70, 45};
                };
                class Boiled
                {
                    visual_properties[] = {0, 2, 0};
                    nutrition_properties[] = {1, 80, 32, 1, 0};
                    cooking_properties[] = {105, 55};
                };
                class Dried
                {
                    visual_properties[] = {0, 3, 0};
                    nutrition_properties[] = {2, 80, 4, 1, 0};
                    cooking_properties[] = {70, 45, 80};
                };
                class Burned
                {
                    visual_properties[] = {0, 4, 0};
                    nutrition_properties[] = {5, 20, 0, 1, 0, 16, 1, 3};
                    cooking_properties[] = {100, 30};
                };
            };
            class FoodStageTransitions : AnimalCorpsesStageTransitions
            {
            };
        };
    };

    class ChefZ_BlueSentinelCrab : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BLUESENTINELCRAB";
        descriptionShort = "#STR_CHEFZ_ITEM_SHELLFISH_DESC";
        model = "\ChefZ\ChefZ_Sealife\crab\models\bluesentinelcrab.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\crab\data\BlueSentinelCrab_co.paa", "ChefZ\ChefZ_Sealife\crab\data\BlueSentinelCrab_baked_co.paa", "ChefZ\ChefZ_Sealife\crab\data\BlueSentinelCrab_boiled_co.paa", "ChefZ\ChefZ_Sealife\crab\data\BlueSentinelCrab_dried_co.paa", "ChefZ\ChefZ_Sealife\crab\data\BlueSentinelCrab_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {2, 2};
        weight = 400;
        varQuantityInit = 250;
        varQuantityMax = 250;
    };

    class ChefZ_CoastalCrab : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COASTALCRAB";
        descriptionShort = "#STR_CHEFZ_ITEM_SHELLFISH_DESC";
        model = "\ChefZ\ChefZ_Sealife\crab\models\coastalcrab.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\crab\data\CoastalCrab_co.paa", "ChefZ\ChefZ_Sealife\crab\data\CoastalCrab_baked_co.paa", "ChefZ\ChefZ_Sealife\crab\data\CoastalCrab_boiled_co.paa", "ChefZ\ChefZ_Sealife\crab\data\CoastalCrab_dried_co.paa", "ChefZ\ChefZ_Sealife\crab\data\CoastalCrab_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {2, 2};
        weight = 400;
        varQuantityInit = 250;
        varQuantityMax = 250;
    };

    class ChefZ_CrimsonKingCrab : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CRIMSONKINGCRAB";
        descriptionShort = "#STR_CHEFZ_ITEM_SHELLFISH_DESC";
        model = "\ChefZ\ChefZ_Sealife\crab\models\crimsonkingcrab.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\crab\data\CrimsonKingCrab_co.paa", "ChefZ\ChefZ_Sealife\crab\data\CrimsonKingCrab_baked_co.paa", "ChefZ\ChefZ_Sealife\crab\data\CrimsonKingCrab_boiled_co.paa", "ChefZ\ChefZ_Sealife\crab\data\CrimsonKingCrab_dried_co.paa", "ChefZ\ChefZ_Sealife\crab\data\CrimsonKingCrab_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {3, 3};
        weight = 1500;
        varQuantityInit = 450;
        varQuantityMax = 450;
    };

    class ChefZ_Langoustine : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_LANGOUSTINE";
        descriptionShort = "#STR_CHEFZ_ITEM_SHELLFISH_DESC";
        model = "\ChefZ\ChefZ_Sealife\crab\models\langoustine.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\crab\data\Langoustine_co.paa", "ChefZ\ChefZ_Sealife\crab\data\Langoustine_baked_co.paa", "ChefZ\ChefZ_Sealife\crab\data\Langoustine_boiled_co.paa", "ChefZ\ChefZ_Sealife\crab\data\Langoustine_dried_co.paa", "ChefZ\ChefZ_Sealife\crab\data\Langoustine_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {2, 2};
        weight = 400;
        varQuantityInit = 250;
        varQuantityMax = 250;
    };

    class ChefZ_MidnightLobster : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MIDNIGHTLOBSTER";
        descriptionShort = "#STR_CHEFZ_ITEM_SHELLFISH_DESC";
        model = "\ChefZ\ChefZ_Sealife\crab\models\midnightlobster.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\crab\data\MidnightLobster_co.paa", "ChefZ\ChefZ_Sealife\crab\data\MidnightLobster_baked_co.paa", "ChefZ\ChefZ_Sealife\crab\data\MidnightLobster_boiled_co.paa", "ChefZ\ChefZ_Sealife\crab\data\MidnightLobster_dried_co.paa", "ChefZ\ChefZ_Sealife\crab\data\MidnightLobster_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {3, 3};
        weight = 1500;
        varQuantityInit = 450;
        varQuantityMax = 450;
    };

    class ChefZ_SnowCrab : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SNOWCRAB";
        descriptionShort = "#STR_CHEFZ_ITEM_SHELLFISH_DESC";
        model = "\ChefZ\ChefZ_Sealife\crab\models\snowcrab.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\crab\data\SnowCrab_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SnowCrab_baked_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SnowCrab_boiled_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SnowCrab_dried_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SnowCrab_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {3, 3};
        weight = 1500;
        varQuantityInit = 450;
        varQuantityMax = 450;
    };

    class ChefZ_SpinyLobster : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SPINYLOBSTER";
        descriptionShort = "#STR_CHEFZ_ITEM_SHELLFISH_DESC";
        model = "\ChefZ\ChefZ_Sealife\crab\models\spinylobster.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\crab\data\SpinyLobster_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SpinyLobster_baked_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SpinyLobster_boiled_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SpinyLobster_dried_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SpinyLobster_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {3, 3};
        weight = 1500;
        varQuantityInit = 450;
        varQuantityMax = 450;
    };

    class ChefZ_SpinyOrangeCrab : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SPINYORANGECRAB";
        descriptionShort = "#STR_CHEFZ_ITEM_SHELLFISH_DESC";
        model = "\ChefZ\ChefZ_Sealife\crab\models\spinyorangecrab.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\crab\data\SpinyOrangeCrab_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SpinyOrangeCrab_baked_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SpinyOrangeCrab_boiled_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SpinyOrangeCrab_dried_co.paa", "ChefZ\ChefZ_Sealife\crab\data\SpinyOrangeCrab_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {2, 2};
        weight = 400;
        varQuantityInit = 250;
        varQuantityMax = 250;
    };

    class ChefZ_StripedTigerPrawn : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_STRIPEDTIGERPRAWN";
        descriptionShort = "#STR_CHEFZ_ITEM_SHELLFISH_DESC";
        model = "\ChefZ\ChefZ_Sealife\crab\models\stripedtigerprawn.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\crab\data\StripedTigerPrawn_co.paa", "ChefZ\ChefZ_Sealife\crab\data\StripedTigerPrawn_baked_co.paa", "ChefZ\ChefZ_Sealife\crab\data\StripedTigerPrawn_boiled_co.paa", "ChefZ\ChefZ_Sealife\crab\data\StripedTigerPrawn_dried_co.paa", "ChefZ\ChefZ_Sealife\crab\data\StripedTigerPrawn_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {2, 2};
        weight = 400;
        varQuantityInit = 250;
        varQuantityMax = 250;
    };

    class ChefZ_CrimsonSquid : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CRIMSONSQUID";
        descriptionShort = "#STR_CHEFZ_ITEM_SQUID_DESC";
        model = "\ChefZ\ChefZ_Sealife\squid\models\crimsonsquid.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\squid\data\CrimsonSquid_co.paa", "ChefZ\ChefZ_Sealife\squid\data\CrimsonSquid_baked_co.paa", "ChefZ\ChefZ_Sealife\squid\data\CrimsonSquid_boiled_co.paa", "ChefZ\ChefZ_Sealife\squid\data\CrimsonSquid_dried_co.paa", "ChefZ\ChefZ_Sealife\squid\data\CrimsonSquid_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {3, 2};
        weight = 1500;
        varQuantityInit = 450;
        varQuantityMax = 450;
    };

    class ChefZ_NorthernCrimsonSquid : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_NORTHERNCRIMSONSQUID";
        descriptionShort = "#STR_CHEFZ_ITEM_SQUID_DESC";
        model = "\ChefZ\ChefZ_Sealife\squid\models\northerncrimsonsquid.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\squid\data\NorthernCrimsonSquid_co.paa", "ChefZ\ChefZ_Sealife\squid\data\NorthernCrimsonSquid_baked_co.paa", "ChefZ\ChefZ_Sealife\squid\data\NorthernCrimsonSquid_boiled_co.paa", "ChefZ\ChefZ_Sealife\squid\data\NorthernCrimsonSquid_dried_co.paa", "ChefZ\ChefZ_Sealife\squid\data\NorthernCrimsonSquid_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {3, 2};
        weight = 1500;
        varQuantityInit = 450;
        varQuantityMax = 450;
    };

    class ChefZ_Octopus : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_OCTOPUS";
        descriptionShort = "#STR_CHEFZ_ITEM_SQUID_DESC";
        model = "\ChefZ\ChefZ_Sealife\squid\models\octopus.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\squid\data\Octopus_co.paa", "ChefZ\ChefZ_Sealife\squid\data\Octopus_baked_co.paa", "ChefZ\ChefZ_Sealife\squid\data\Octopus_boiled_co.paa", "ChefZ\ChefZ_Sealife\squid\data\Octopus_dried_co.paa", "ChefZ\ChefZ_Sealife\squid\data\Octopus_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {3, 2};
        weight = 1500;
        varQuantityInit = 450;
        varQuantityMax = 450;
    };

    class ChefZ_SpeckledSquid : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SPECKLEDSQUID";
        descriptionShort = "#STR_CHEFZ_ITEM_SQUID_DESC";
        model = "\ChefZ\ChefZ_Sealife\squid\models\speckledsquid.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\squid\data\SpeckledSquid_co.paa", "ChefZ\ChefZ_Sealife\squid\data\SpeckledSquid_baked_co.paa", "ChefZ\ChefZ_Sealife\squid\data\SpeckledSquid_boiled_co.paa", "ChefZ\ChefZ_Sealife\squid\data\SpeckledSquid_dried_co.paa", "ChefZ\ChefZ_Sealife\squid\data\SpeckledSquid_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {3, 2};
        weight = 400;
        varQuantityInit = 250;
        varQuantityMax = 250;
    };

    class ChefZ_Squid : ChefZ_Seafood_Base
    {
        hiddenSelectionsMaterials[] = {};   // MLOD-Modell: jedes Material macht es unsichtbar, Stufen-Optik nur ueber Texturen (ChefZ_StageVisuals, 27.10.2026)
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SQUID";
        descriptionShort = "#STR_CHEFZ_ITEM_SQUID_DESC";
        model = "\ChefZ\ChefZ_Sealife\squid\models\squid.p3d";   // EIGENES MODELL (Lieferung 25.10.2026)
        hiddenSelectionsTextures[] = {"ChefZ\ChefZ_Sealife\squid\data\Squid_co.paa", "ChefZ\ChefZ_Sealife\squid\data\Squid_baked_co.paa", "ChefZ\ChefZ_Sealife\squid\data\Squid_boiled_co.paa", "ChefZ\ChefZ_Sealife\squid\data\Squid_dried_co.paa", "ChefZ\ChefZ_Sealife\squid\data\Squid_burnt_co.paa"};   // roh, gebraten, gekocht, geraeuchert, verbrannt (27.10.2026)
        itemSize[] = {3, 2};
        weight = 1500;
        varQuantityInit = 450;
        varQuantityMax = 450;
    };
};

//------------------------------------------------------------------------------
// Anmeldung beim Core (02 §4).
//
// EIN KNOTEN JE SLICE. Der Knoten heisst ChefZ_fillets und nicht wie das Addon:
// mehrere Slices schreiben gleichzeitig in dieses Modul, und ein Knotenname, der
// dem CfgPatches- und CfgMods-Eintrag gleicht, zaehlte configcpp.mjs zusaetzlich
// als doppelte Klassendefinition (so bereits in ChefZ_Farming geloest).
//
// handcraftRecipeSlots = 0: dieser Slice bringt KEINEN HANDCRAFT-Transform mit.
// Die 31 Filetier-Transforms und ihre Slots gehoeren den Fisch-Slices (21 §5).
// Vanilla vergibt Rezept-IDs als Position in PluginRecipesManager.m_RecipeList,
// und zu wenige Plaetze weisen die ueberzaehligen Transforms mit Klartextfehler
// ab, statt sie nachzutragen (02 §4.2).
//
// loadOrder 400: nach Registry (150) und den Zutatenmodulen - die Filets sind
// Ergebnisklassen, niemand braucht sie frueher (21 §1).
//------------------------------------------------------------------------------
//==============================================================================
// ### SLICE fish-fresh ###  PROCESS_FILLET_FISH
//
// Der Filetierschritt. Er steht HIER und nicht in einem Delta, weil ein Prozess
// mehr braucht, als das Delta-Schema traegt: exec und toolGroups entscheiden
// ueber die Ausfuehrungsform, und geraten werden darf dabei nichts. Ins Delta
// geht er zusaetzlich in der Form, die das Schema kennt (id + durationSec) -
// identisch, damit der Integrator ihn still deduplizieren kann.
//
// GEMEINSAM FUER ALLE DREI FISCH-SLICES. fish-sea-a und fish-sea-b nennen
// denselben Prozess in ihren Transforms und duerfen ihn NICHT erneut
// definieren - zwei gleichnamige Klassen in derselben Wurzel waeren eine
// doppelte Definition.
//
// exec = HANDCRAFT, genau EIN Eingang plus toolGroups: das ist die Form, die
// die Handwerksbruecke verlangt. Vanillas RecipeBase kombiniert immer ZWEI
// Dinge (MAX_NUMBER_OF_INGREDIENTS = 2, 01 V12); bei einem Eingang MUSS das
// Werkzeug den zweiten Platz belegen, sonst gaebe es nichts zu kombinieren
// (ChefZ_GenericCraftRecipe.c:280-302).
//
// baseDurationSec = 6: Filetieren ist Handarbeit von Sekunden, kein
// Wartevorgang. requiresHeat = 0: ein Messer braucht kein Feuer.
//==============================================================================
class CfgChefZProcesses
{
    class PROCESS_FILLET_FISH
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_FILLET_FISH";
        toolGroups[] = {"CUTTING_TOOL"};
        baseDurationSec = 6.0;
        animationLength = 4.0;
        specialty = 0.02;
        requiresHeat = 0;
        toolDamage = 3;
    };
};

class CfgChefZ
{
    class ChefZ_fillets
    {
        chefzApiVersion = 1;
        loadOrder = 400;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Fishing/Config/Ingredients/Fillets.json"
        };
    };

    //--------------------------------------------------------------------------
    // ### SLICE baits ###
    //
    // EIN KNOTEN JE SLICE (02 §4) - nicht einer je Modul. Mehrere Slices liefern
    // in dieses Addon, und ein gemeinsamer Knoten waere eine Zeile, die vier
    // Agenten gleichzeitig umschreiben.
    //
    // loadOrder 420 und nicht 400: die Fisch-Slices dieses Moduls melden ihre
    // Ertraege frueher an. Ein Koeder nennt in "targets" Fisch-IDs; sind die beim
    // Laden schon bekannt, meldet der Core einen Tippfehler sofort statt erst
    // beim Anhaengen. Eine harte Abhaengigkeit ist es nicht - 21 §3.4 stuft ein
    // unbekanntes Ziel bewusst nur als WARN ein, damit ein Koedermodul Fische aus
    // einem nicht geladenen Slice nennen darf.
    //
    // handcraftRecipeSlots = 0: dieser Slice bringt KEINEN HANDCRAFT-Transform
    // mit. Ein Koeder wird gefunden, nicht gebaut; die Filet-Transforms gehoeren
    // den Fisch-Slices und zaehlen dort (21 §5).
    //--------------------------------------------------------------------------
    class ChefZ_baits
    {
        chefzApiVersion      = 1;
        loadOrder            = 420;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Fishing/Config/Fishing/Baits.json"
        };
    };

    // ### SLICE fish-fresh ###
    //
    // loadOrder 405: nach Registry und den Filets (400), vor den Koedern (420).
    // Die Koeder nennen die Fisch-IDs dieses Slice in "targets"; sind die
    // Ertraege beim Laden schon da, meldet der Core einen Tippfehler sofort
    // statt erst beim Anhaengen.
    //
    // handcraftRecipeSlots = 8 - GEZAEHLT, nicht geschaetzt: acht Fische, je ein
    // HANDCRAFT-Transform in Config/Processing/Fillet_Fresh.json
    // (TR_FilletPerch, TR_FilletMuskellunge, TR_FilletSnakehead,
    // TR_FilletRainbowTrout, TR_FilletChar, TR_FilletSalmon,
    // TR_FilletSockeyeSalmon, TR_FilletEel). Die Zahl ist eine RESERVIERUNG in
    // Vanillas Rezeptliste und muss VOR dem Laden feststehen: Vanilla vergibt
    // Rezept-IDs als Position in PluginRecipesManager.m_RecipeList, und diese
    // Positionen entstehen im MissionBase-Konstruktor (02 §4.2). Zu wenig
    // reserviert heisst, die ueberzaehligen Transforms werden mit
    // Klartextfehler abgewiesen - nicht nachgetragen.
    //
    // PFADWURZEL: jeder Eintrag beginnt mit dem ORDNERNAMEN des Addons
    // (02 §4.1) - "ChefZ_Fishing/", nie "Psyerns/".
    class ChefZ_fish_fresh
    {
        chefzApiVersion = 1;
        loadOrder = 405;
        handcraftRecipeSlots = 8;
        dataFiles[] =
        {
            "ChefZ_Fishing/Config/Fishing/Fish_Fresh.json",
            "ChefZ_Fishing/Config/Ingredients/Fish_Fresh.json",
            "ChefZ_Fishing/Config/Processing/Fillet_Fresh.json"
        };
    };


    //--------------------------------------------------------------------------
    // ### SLICE fish-sea-a ###  Salzwasser A.
    //
    // EIN KNOTEN JE SLICE (02 Paragraph 4), nicht je Modul - in dieses Modul schreiben
    // mehrere Slices gleichzeitig.
    //
    // handcraftRecipeSlots = 12, einer je HANDCRAFT-Transform dieses Slice:
    //
    //   TR_FilletPlaice          TR_FilletGiltheadBream   TR_FilletHalibut
    //   TR_FilletCod             TR_FilletSole            TR_FilletSeaBass
    //   TR_FilletRedMullet       TR_FilletConger          TR_FilletSeaEel
    //   TR_FilletHorseMackerel   TR_FilletJapaneseSardine TR_FilletRockfish
    //
    // KEIN Transform fuer die Makrele: Vanillas PrepareMackerel
    // (4_World/DayZ/Classes/Recipes/Recipes/PrepareMackerel.c) filetiert sie
    // bereits, ein zweites Rezept waere ein Dublett im Kontextmenue
    // (21 Paragraph 5). Zwoelf Fische, zwoelf Transforms, zwoelf Plaetze.
    //
    // Die Zahl ist eine RESERVIERUNG in Vanillas Rezeptliste und muss VOR dem
    // Laden feststehen (02 Paragraph 4.2): Vanilla vergibt Rezept-IDs als Position
    // in PluginRecipesManager.m_RecipeList, und die entstehen im
    // MissionBase-Konstruktor. Zu wenige Plaetze heisst nicht "wird
    // nachgetragen", sondern "die ueberzaehligen Transforms werden mit
    // Klartextfehler abgewiesen".
    //
    // REIHENFOLGE der dataFiles: der Prozess steht VOR den Transforms, die ihn
    // nennen. loadOrder 400 wie die Nachbarslices - nach Registry (150) und
    // nach den Zutatenmodulen.
    //--------------------------------------------------------------------------
    class ChefZ_fish_sea_a
    {
        chefzApiVersion      = 1;
        loadOrder            = 400;
        handcraftRecipeSlots = 12;
        dataFiles[] =
        {
            "ChefZ_Fishing/Config/Fishing/Fish_Sea_A.json",
            "ChefZ_Fishing/Config/Ingredients/Fish_Sea_A.json",
            "ChefZ_Fishing/Config/Processing/Fillet_Sea_A.json"
        };
    };

    //--------------------------------------------------------------------------
    // ### SLICE fish-sea-b ###
    //
    // loadOrder 405: nach Registry (150) und den Filets (400), vor den Koedern
    // (420). Die Koeder nennen die Fisch-IDs dieses Slice in "targets"; sind die
    // Ertraege beim Laden schon da, meldet der Core einen Tippfehler sofort.
    //
    // handcraftRecipeSlots = 11 - GEZAEHLT, nicht geschaetzt: elf Arten, je ein
    // HANDCRAFT-Transform in Config/Processing/Fillet_Sea_B.json (TR_Fillet-
    // BluefinTuna, -YellowfinTuna, -RedSnapper, -GiantGrouper, -MahiMahi,
    // -GiantTrevally, -BlueMarlin, -Sailfish, -Bonito, -Yellowtail, -Tarpon).
    // Die Zahl ist eine RESERVIERUNG in Vanillas Rezeptliste und muss VOR dem
    // Laden feststehen (02 §4.2); zu wenig reserviert heisst, die
    // ueberzaehligen Transforms werden mit Klartextfehler abgewiesen.
    //
    // KEIN eigener Prozess: die Transforms nennen PROCESS_FILLET_FISH aus
    // CfgChefZProcesses weiter oben. Der Knoten gehoert dem Slice fish-fresh
    // und wird geteilt - ihn hier erneut zu definieren waere eine doppelte
    // Klassendefinition (21 §5: EIN Filetierprozess fuer alle Fisch-Slices).
    //--------------------------------------------------------------------------
    class ChefZ_fish_sea_b
    {
        chefzApiVersion      = 1;
        loadOrder            = 405;
        handcraftRecipeSlots = 11;
        dataFiles[] =
        {
            "ChefZ_Fishing/Config/Fishing/Fish_Sea_B.json",
            "ChefZ_Fishing/Config/Ingredients/Fish_Sea_B.json",
            "ChefZ_Fishing/Config/Processing/Fillet_Sea_B.json"
        };
    };

    // ### SLICE seafood ### (26.10.2026)
    // loadOrder 410: nach Registry (Kategorie SEAFOOD) und den Fischslices, vor
    // den Koedern (420), die die Tintenfisch-IDs in "targets" nennen.
    class ChefZ_seafood
    {
        chefzApiVersion      = 1;
        loadOrder            = 410;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Fishing/Config/Fishing/Seafood.json",
            "ChefZ_Fishing/Config/Ingredients/Seafood.json"
        };
    };
};
