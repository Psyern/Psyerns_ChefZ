// ChefZ_Processing - Getreidemuehle, Nudelholz, Mehl (Slice "grain").
//
// Quelle: Production Map §7 (Wheat + GrainMill -> Flour), §8 (Mehl), §57
// (ChefZ_GrainMill), §58 (V1 Tools), §76 (Produktionsabhaengigkeiten nach
// Station), DME-Plan §53 (Namenskonvention).
//
// PBO-Praefix: $PREFIX$ enthaelt "ChefZ_Processing". Die Wurzel jedes
// Laufzeitpfades - auch jedes dataFiles[]-Eintrags - ist dieses Praefix
// (Entwurf 02 §4.1, B4).
//
// ---------------------------------------------------------------------------
// KEIN NEUES CORE-SYSTEM
// ---------------------------------------------------------------------------
// Die Muehle ist genau das, was der Kopf von ChefZ_ProcessingStation_Base als
// Andockregel vorgibt:
//
//   config.cpp   class ChefZ_GrainMill : <Vanilla-Klasse> { ... };
//                Stationsdatensatz in Config/GrainStations.json (Rang 2)
//   Skript       class ChefZ_GrainMill extends ChefZ_ProcessingStation_Base {}
//
// Mehr ist nicht noetig: keine eigene Action, keine eigene Persistenz, keine
// Core-Aenderung. Der Prozess selbst steht als Datensatz in
// Config/GrainProcesses.json, die Umwandlung in Config/GrainTransforms.json.
//
// ---------------------------------------------------------------------------
// Warum die Muehle STATION_ACTION ist und nicht HANDCRAFT
// ---------------------------------------------------------------------------
// Mahlen hat einen Ort. Der Spieler arbeitet an einem Objekt, Fortschritt und
// Abbruch brauchen einen Anker (11 §3). Ausserdem waere HANDCRAFT hier
// unbrauchbar: ein Eingang ohne Werkzeuggruppe ist bei Vanillas Craftsystem
// gar nicht registrierbar, weil es immer ZWEI Dinge kombiniert (01 V12).
//
// ---------------------------------------------------------------------------
// 3D
// ---------------------------------------------------------------------------
// Muehle und Nudelholz tragen Vanilla-Proxy-Modelle und sind im Slice-Bericht
// als Asset-Bedarf gemeldet (Production Map §70: beide brauchen eigene
// Geometrie). Auf ein Modell wartet hier nichts.

class CfgPatches
{
    class ChefZ_Processing
    {
        units[] = {
            "ChefZ_GrainMill", "ChefZ_RollingPin", "ChefZ_Flour",
            // Slice "herbs" (DME-Plan §6.3 und §6.7, Production Map §57)
            "ChefZ_HerbStationBase", "ChefZ_Mortar", "ChefZ_DryingRack",
            // ### SLICE dairy ###
            "ChefZ_ButterChurn", "ChefZ_CheesePress",
            // ### SLICE salt ###
            "ChefZ_SaltPan",
            // ### SLICE meat ### (Production Map §57: Schneidebrett, Fleischwolf)
            "ChefZ_CuttingBoard", "ChefZ_MeatGrinder"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // ChefZ_Core:     ChefZ_ProcessingStation_Base, ChefZ_Item_Base.
        // ChefZ_Farming:  ChefZ_GrainFoodBase (Nahrungsbasis) und ChefZ_Wheat
        //                 als Eingang des Mahlvorgangs.
        // DZ_Gear_*:      die Proxy-Modelle.
        requiredAddons[] = {"DZ_Data", "DZ_Gear_Camping", "DZ_Gear_Tools", "DZ_Gear_Food", "ChefZ_Core", "ChefZ_Farming", "DZ_Gear_Cooking"};
    };
};

class CfgMods
{
    class ChefZ_Processing
    {
        dir = "ChefZ_Processing";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Processing";
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
                    "ChefZ_Processing/Scripts/4_World"
                };
            };
        };
    };
};

class CfgVehicles
{
    // ### SLICE dairy ### Proxy-Basen
    class Pot;
    class Cauldron;

    class Inventory_Base;
    // ChefZ_GrainFoodBase kommt aus ChefZ_Farming und wird NICHT
    // vorwaertsdeklariert: eine leere Vorwaertsdeklaration wuerde den
    // echten Knoten mitsamt Nutrition und Food verdecken.

    //--------------------------------------------------------------------------
    // Die Getreidemuehle (Production Map §57).
    //
    // Sie ist ein tragbares und ablegbares Objekt, kein Kochgeraet: sie fasst
    // Vanillas Kochkette an keiner Stelle an (11 E6).
    //
    // PROXY: wooden_case.p3d - eine Holzkiste in der richtigen Groessenordnung.
    // Eigenes Muehlenmesh ist gemeldet (U, P1).
    //--------------------------------------------------------------------------
    class ChefZ_GrainMill : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_GRAINMILL";
        descriptionShort = "#STR_CHEFZ_GRAINMILL_DESC";
        model = "\DZ\gear\camping\wooden_case.p3d";
        weight = 9000;
        itemSize[] = {6, 4};
        canBeDigged = 0;
        rotationFlags = 2;
        lifetime = 172800;
    };

    //--------------------------------------------------------------------------
    // Das Nudelholz (Production Map §58, §11).
    //
    // Es ist WERKZEUG, kein Eingang: der Prozess PROCESS_ROLL nennt die
    // Werkzeuggruppe ROLLING_PIN, und ChefZ_ToolRegistry loest sie ueber den
    // CfgChefZTools-Knoten unten auf.
    //
    // PROXY: Meat_Tenderizer.p3d - ein Holzgriff-Kuechenwerkzeug. Eigenes
    // Nudelholzmesh ist gemeldet (U, P2).
    //--------------------------------------------------------------------------
    class ChefZ_RollingPin : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ROLLINGPIN";
        descriptionShort = "#STR_CHEFZ_ROLLINGPIN_DESC";
        model = "\dz\gear\tools\Meat_Tenderizer.p3d";
        weight = 600;
        itemSize[] = {3, 1};
        repairableWithKits[] = {};
        lifetime = 43200;
    };

    //--------------------------------------------------------------------------
    // Mehl (Production Map §8).
    //
    // Erbt Nutrition und Food von ChefZ_GrainFoodBase aus ChefZ_Farming -
    // deshalb steht hier kein zweiter Nahrungsblock. Die stueckgenauen Werte
    // liegen im Nutrition-Delta des Slices.
    //
    // PROXY: PowderedMilk.p3d - eine Pulvertuete. Eigenes Mehlmesh ist
    // gemeldet (U, P1).
    //--------------------------------------------------------------------------
    class ChefZ_Flour : ChefZ_GrainFoodBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_FLOUR";
        descriptionShort = "#STR_CHEFZ_FLOUR_DESC";
        model = "\dz\gear\food\PowderedMilk.p3d";
        weight = 300;
        itemSize[] = {2, 2};
        stackedUnit = "grams";
        quantityBar = 1;
        varQuantityInit = 800;
        varQuantityMin = 0;
        varQuantityMax = 1000;
        varQuantityDestroyOnMin = 1;
        canBeSplit = 1;
        lifetime = 43200;
    };

    //==========================================================================
    // ### SLICE herbs ### Moerser und Trockenrahmen
    // (DME-Plan §6.3 und §6.7, Production Map §57)
    //
    // Beide sind tragbare, ablegbare Objekte MIT CARGO - die Station verarbeitet
    // genau das, was in ihrem Cargo liegt (ChefZ_ProcessingStation_Base ruft
    // ChefZ_FactCollector.CollectFromCargo(this, ...)). Ohne Cargo faende ein
    // Job nie eine Zutat.
    //
    // Der Cargo-Block steht EINMAL auf der gemeinsamen Basis. Das ist nicht nur
    // kuerzer: configcpp.mjs prueft Klassennamen projektweit auf Eindeutigkeit
    // und zaehlt verschachtelte Knoten mit.
    //
    // PROXY-MODELLE, beide Vanilla, beide als Asset-Bedarf gemeldet.
    //==========================================================================
    class ChefZ_HerbStationBase : Inventory_Base
    {
        scope = 0;
        rotationFlags = 17;
        canBeDigged = 0;
        absorbency = 0.0;
        lifetime = 172800;

        class Cargo
        {
            itemsCargoSize[] = {4, 3};
            openable = 0;
        };
    };

    // Moerser und Stoessel: Pfefferkoerner -> Pfeffer, getrocknete Paprika ->
    // Paprikapulver, getrocknete Kraeuter -> Kraeutermischung und Hunter
    // Seasoning. PROXY: Kochtopf - eine Schale in der richtigen Groessenordnung.
    class ChefZ_Mortar : ChefZ_HerbStationBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MORTAR";
        descriptionShort = "#STR_CHEFZ_ITEM_MORTAR_DESC";
        model = "\dz\gear\cooking\CookingPot.p3d";
        itemSize[] = {3, 2};
        weight = 1800;
    };

    // Trockenrahmen: Kraeuter, Pfefferbeeren, Paprika - und laut Production Map
    // §57 spaeter auch Fleisch, Fisch und Nudeln anderer Slices. Vier
    // Parallelplaetze, damit sich das Warten lohnt. PROXY: Holzregal.
    class ChefZ_DryingRack : ChefZ_HerbStationBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DRYINGRACK";
        descriptionShort = "#STR_CHEFZ_ITEM_DRYINGRACK_DESC";
        model = "\DZ\structures\furniture\various\rack_dz.p3d";
        itemSize[] = {6, 4};
        weight = 4200;
        rotationFlags = 2;
    };

    //==========================================================================
    // ### SLICE dairy ### Die beiden Stationen der Milchkette
    // (Production Map §48-§50, DME-Plan §6.8 Butterfass, §6.9 Kaesepresse).
    //
    // Andockregel aus dem Kopf von ChefZ_ProcessingStation_Base.c: Configbasis
    // eine Vanilla-Klasse, Skriptbasis ChefZ_ProcessingStation_Base, Prozesse
    // im Stationsdatensatz. Kein Core-Code, keine eigene Action.
    //
    // MODELL-PROXY ueber VERERBUNG statt model=: Pot und Cauldron bringen
    // Modell, Icon, Schadensmodell UND einen Cargo-Bereich mit - und genau aus
    // dem Cargo liest die Station ihre Eingaenge
    // (ChefZ_FactCollector.CollectFromCargo). Ein geratener p3d-Pfad faellt
    // erst beim Packen auf, eine geerbte Klasse nie.
    //
    // Beide fassen Vanillas Kochkette an KEINER Stelle an (11 E6): sie sind
    // fuer die Kochlogik kein Kochgeschirr, weil ihre SKRIPTklasse von
    // ChefZ_ProcessingStation_Base erbt und nicht von der Vanilla-Klasse.
    //==========================================================================

    // §48/§49: Das Butterfass traegt ZWEI Prozesse - erst abrahmen, dann
    // schlagen. Production Map §48 nennt dafuer zwar eine eigene Station
    // ("ChefZ_DairyProcessor") und erlaubt im selben Satz das vereinfachte
    // Processing fuer V1; zwei Stationen fuer eine dreigliedrige Kette waeren
    // eine Station zu viel. Das Fass kann beides, weil es beide Male dasselbe
    // tut: ruehren.
    //
    // PROXY: Pot. Ziel: hoelzernes Stossbutterfass.
    class ChefZ_ButterChurn : Pot
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BUTTERCHURN";
        descriptionShort = "#STR_CHEFZ_ITEM_BUTTERCHURN_DESC";
        rotationFlags = 17;
        itemSize[] = {4, 4};
        weight = 4200;
        canBeDigged = 0;
        lifetime = 172800;
    };

    // §50: Die Kaesepresse.
    // PROXY: Cauldron. Ziel: hoelzerne Presse mit Spindel.
    class ChefZ_CheesePress : Cauldron
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHEESEPRESS";
        descriptionShort = "#STR_CHEFZ_ITEM_CHEESEPRESS_DESC";
        rotationFlags = 17;
        itemSize[] = {5, 4};
        weight = 6800;
        canBeDigged = 0;
        lifetime = 172800;
    };

    //==========================================================================
    // ### SLICE salt ###   Production Map §25, Planungsschritte §16
    //
    // ChefZ_SaltPan - die Siedepfanne. EINE Station traegt die ganze Kette:
    // sieden (PROCESS_BOIL_BRINE) und trocknen (PROCESS_DRY_SALT).
    //
    // WARUM EINE EIGENE STATION UND NICHT DER KOCHTOPF (Production Map §25
    // nennt "Cooking Pot + Heat"):
    // Ein ChefZ-REZEPT braucht mindestens einen Slot, und ein Slot bindet ein
    // ITEM im Gefaess - das Gefaess selbst ist nie Zutat (01 V13,
    // ChefZ_FactCollector.CollectFromCargo). Salzwasser IM Topf ist aber
    // Fluessigkeit, kein Item; ein Rezept "leerer Topf voll Salzwasser" ist
    // in der Recipe Engine nicht ausdrueckbar (ChefZ_RecipeDef.Validate weist
    // ein Rezept ohne slots ausdruecklich ab). Der Verarbeitungspfad kann es:
    // dort ist der Eingang ein GEFUELLTER BEHAELTER im Cargo der Station, und
    // ChefZ_Selector traegt fuer genau diesen Fall isLiquidContainer und
    // liquidType.
    //
    // WARUM NICHT VON Pot ODER Cauldron ABGELEITET: beide sind IsCookware().
    // Vanillas Cooking.ProcessItemToCook beschaedigt jedes Cargo-Item, das
    // nicht selbst Kochgeschirr ist, ueber PARAM_BURN_DAMAGE_COEF - die
    // Feldflasche mit dem Meerwasser ginge im Feuer kaputt. Inventory_Base
    // haelt die Pfanne aus Vanillas Kochkette heraus; die Waerme holt sie sich
    // ueber die Feuerstelle daneben (ChefZ_SaltPan.ChefZ_HasHeat).
    //
    // MODELL: Vanilla-Proxy FryingPan. Ziel: eine breite, flache Siedepfanne
    // mit Salzkruste - eigene Geometrie, siehe Asset-Bedarf des Slice.
    //==========================================================================
    class ChefZ_SaltPan : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SALTPAN";
        descriptionShort = "#STR_CHEFZ_ITEM_SALTPAN_DESC";
        model = "\dz\gear\cooking\FryingPan.p3d";
        rotationFlags = 17;
        itemSize[] = {4, 3};
        weight = 2400;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;

        // Der Cargo-Bereich IST die Eingangsseite der Station:
        // ChefZ_ProcessingStation_Base liest seine Zutaten ueber
        // ChefZ_FactCollector.CollectFromCargo aus genau diesem Bereich.
        // 3x2 fasst zwei Feldflaschen oder einen Kanister nebst Rohsalz - mehr
        // waere ein Lager, weniger ein Flaschenhals ohne Aussage.
        class Cargo
        {
            itemsCargoSize[] = {3, 2};
            openable = 0;
        };
    };

    //==========================================================================
    // ### SLICE meat ###
    //
    // Die beiden Stationen der Fleischkette (Production Map §57).
    //
    // Andockregel woertlich aus dem Kopf von ChefZ_ProcessingStation_Base.c:
    // Configbasis ist eine VANILLA-Klasse, Skriptbasis ist
    // ChefZ_ProcessingStation_Base, und was die Station anbietet, steht im
    // Stationsdatensatz - nicht hier und nicht im Skript.
    //
    // Der Stationsdatensatz liegt in Rang 2
    // (Config/Processing/Stations.json), aus demselben Grund, der oben schon
    // fuer ChefZ_GrainMill steht: 11 §2 verlangt "id == Klassenname der
    // Station", und ein gleichnamiger Knoten unter CfgChefZStations zaehlt
    // fuer configcpp.mjs als doppelte Klassendefinition.
    //
    // MODELLE: Vanilla-Proxys. Ziel waere ein Holzbrett mit Hackspuren bzw.
    // ein gusseiserner Wolf mit Kurbel - eigene Geometrie, siehe Asset-Bedarf
    // des Slice.
    //==========================================================================
    class ChefZ_CuttingBoard : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CUTTINGBOARD0";
        descriptionShort = "#STR_CHEFZ_ITEM_CUTTINGBOARD1";
        model = "\dz\gear\cooking\MeatTenderizer.p3d";
        rotationFlags = 17;
        itemSize[] = {4, 2};
        weight = 900;
        absorbency = 0.1;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
    };

    class ChefZ_MeatGrinder : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MEATGRINDER0";
        descriptionShort = "#STR_CHEFZ_ITEM_MEATGRINDER1";
        model = "\dz\gear\cooking\Cauldron.p3d";
        rotationFlags = 17;
        itemSize[] = {4, 4};
        weight = 3200;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
    };
};

//------------------------------------------------------------------------------
// Modulanmeldung am Config Manager (Entwurf 02 §4).
//
// handcraftRecipeSlots fehlt bewusst: dieses Modul bringt keinen HANDCRAFT-
// Transform mit - Mahlen laeuft ueber die Station - und reserviert deshalb
// null Plaetze in Vanillas Rezeptliste (02 §4.2).
//------------------------------------------------------------------------------
class CfgChefZ
{
    // ### SLICE grain ### Ein Knoten je SLICE (02 §4), nicht je Modul - er
    // heisst deshalb nicht wie das Addon.
    class ChefZ_GrainProcessing
    {
        chefzApiVersion = 1;
        loadOrder = 220;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Processing/Config/GrainProcesses.json",
            "ChefZ_Processing/Config/GrainStations.json",
            "ChefZ_Processing/Config/GrainIngredients.json",
            "ChefZ_Processing/Config/GrainTransforms.json"
        };
    };

    // ### SLICE herbs ### Moerser und Trockenrahmen.
    //
    // Eigener Knoten, weil CfgChefZ genau einen je SLICE traegt und mehrere
    // Slices in dieses Modul liefern. handcraftRecipeSlots = 0: die
    // Kraeuterkette laeuft ausschliesslich ueber Stationen - Trocknen und
    // Moersern haben drei bis fuenf Eingaenge, und Vanillas RecipeBase kennt
    // zwei (01 V12).
    class ChefZ_HerbProcessing
    {
        chefzApiVersion = 1;
        loadOrder = 230;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Processing/Config/Processing/HerbStations.json",
            "ChefZ_Processing/Config/Processing/HerbDrying.json",
            "ChefZ_Processing/Config/Processing/HerbGrinding.json"
        };
    };

    // ### SLICE dairy ###
    //
    // Eigener Knoten je SLICE (02 §4). loadOrder 260 - nach den Stationen der
    // anderen Slices, damit die Reihenfolge deterministisch bleibt.
    //
    // Die STATIONSDATENSAETZE liegen bewusst in Rang 2 und nicht unter
    // CfgChefZStations: 11 §2 verlangt "id == Klassenname der Station", und ein
    // gleichnamiger Knoten neben der CfgVehicles-Klasse zaehlt fuer
    // configcpp.mjs als doppelte Klassendefinition.
    //
    // handcraftRecipeSlots = 0: kein HANDCRAFT-Transform in diesem Slice.
    class ChefZ_DairyProcessing
    {
        chefzApiVersion = 1;
        loadOrder = 260;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Processing/Config/Processing/Dairy_Stations.json",
            "ChefZ_Processing/Config/Processing/Dairy_Transforms.json"
        };
    };

    // ### SLICE salt ###
    //
    // Eigener Knoten, eigene Dateien. handcraftRecipeSlots = 0: die Salzkette
    // laeuft vollstaendig an der Station, Vanillas Rezeptliste bleibt
    // unveraendert.
    class ChefZ_SaltChain
    {
        chefzApiVersion = 1;
        loadOrder = 155;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Processing/Config/Processing/SaltStations.json",
            "ChefZ_Processing/Config/Processing/Salt.json"
        };
    };

    // ### SLICE meat ###
    //
    // Nur die beiden Stationsdatensaetze. Die Transforms der Fleischkette
    // liegen in ChefZ_Meat und melden sich dort an - hier stuende sonst
    // Content eines anderen Moduls.
    //
    // handcraftRecipeSlots = 0: der einzige HANDCRAFT-Transform des Slice
    // (TR_DicedMeat ueber PROCESS_CUT_MEAT) gehoert ChefZ_Meat, und dort ist
    // der Platz auch reserviert. Zweimal reservieren hiesse zwei Plaetze
    // belegen und einen davon leer lassen.
    class ChefZ_MeatProcessing
    {
        chefzApiVersion = 1;
        loadOrder = 190;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Processing/Config/Processing/Stations.json"
        };
    };
};

//------------------------------------------------------------------------------
// ### SLICE herbs ### Die drei Prozesse der Kraeuter- und Gewuerzkette.
//
// Rang 1 und nicht JSON, weil ChefZ_ActionProcessAtStation.ActionCondition()
// auch auf dem CLIENT laeuft und dort Aktionstext und Dauer braucht (11 E8,
// 02 §2). Der Client liest die Game-Config garantiert.
//
// Ein Prozess ist ein VERB ohne Objekt: er sagt, WIE gearbeitet wird - nie,
// woraus was wird. Das steht im Transform (Config/Processing/Herb*.json).
//
// PROCESS_DRY ist bewusst allgemein gehalten und traegt keinen Kraeuterbezug:
// derselbe Trockenrahmen trocknet laut Production Map §57 auch Fleisch, Fisch
// und Nudeln. Die Registry-Anmeldung steht in _deltas/herbs.json; kommt ein
// zweiter Slice mit demselben Prozess, meldet der Integrator den Konflikt.
//
// Keine toolGroups: an einer Station arbeitet die Station, nicht das Werkzeug.
//------------------------------------------------------------------------------
class CfgChefZProcesses
{
    // STATION_TIMED: der Rahmen tickt ohne Spieler weiter (11 §3).
    class PROCESS_DRY
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_DRY";
        baseDurationSec = 600.0;
    };

    // STATION_ACTION: der Spieler arbeitet aktiv am Moerser.
    class PROCESS_GRIND_SPICE
    {
        exec = "STATION_ACTION";
        displayName = "#STR_CHEFZ_PROC_GRIND_SPICE";
        baseDurationSec = 20.0;
    };

    class PROCESS_GRIND_HERB
    {
        exec = "STATION_ACTION";
        displayName = "#STR_CHEFZ_PROC_GRIND_HERB";
        baseDurationSec = 15.0;
    };

    // ### SLICE dairy ### Production Map §48-§50.
    //
    // Alle drei laufen als STATION_TIMED: sie brauchen Zeit und keine
    // Anwesenheit. HANDCRAFT scheidet aus - Vanillas Craftsystem kennt zwei
    // Zutatenplaetze und keine Wartezeit (01 V12).
    //
    // Kein Werkzeug: die Station IST das Werkzeug.
    //
    // Sie stehen in Rang 1 und nicht nur im JSON (11 E8, 02 §2):
    // ChefZ_ActionProcessAtStation.ActionCondition() laeuft auch auf dem
    // CLIENT und muss dort den Aktionstext kennen.

    // §48: Milch abrahmen -> Sahne.
    class PROCESS_SEPARATE_CREAM
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_SEPARATE_CREAM";
        baseDurationSec = 120.0;
    };

    // §49: Sahne zu Butter schlagen.
    class PROCESS_CHURN_BUTTER
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_CHURN_BUTTER";
        baseDurationSec = 180.0;
    };

    // §50: Milch dicklegen und pressen -> Kaese.
    class PROCESS_PRESS_CHEESE
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_PRESS_CHEESE";
        baseDurationSec = 300.0;
    };

    // ### SLICE salt ###   Production Map §25, Planungsschritte §16
    //
    // Zwei Prozesse, zwei Verben. Was WORAUS wird, steht im Transform
    // (Config/Processing/Salt.json) - nie hier.
    //
    // Beide sind STATION_TIMED und nicht STATION_ACTION: Sieden und Trocknen
    // laufen ohne Spieler weiter und dauern Minuten (11 §3). Der Spieler
    // startet den Job und geht.

    // Sieden. Salzwasser -> Rohsalz.
    //
    // baseDurationSec = 900 (15 min) mit requiresHeat: das ist der eigentliche
    // Preis des Salzes. 15 Minuten Dauerfeuer sind in DayZ echter Brennstoff -
    // ohne diese Zahl waere Salz beliebig vermehrbar und als Handelsressource
    // wertlos (Planungsschritte §16: "Salz sollte vollstaendig gebalanced
    // werden ... Energie-/Brennstoffverbrauch").
    //
    // KEIN minTemperature: die Waermebedingung haengt an einer BRENNENDEN
    // Feuerstelle in Reichweite (ChefZ_SaltPan.ChefZ_HasHeat), nicht an der
    // Eigentemperatur der Pfanne. Ein Temperaturschwellwert waere hier eine
    // geratene Zahl - die Feuerabfrage ist eine gepruefte Tatsache.
    class PROCESS_BOIL_BRINE
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_BOIL_BRINE";
        baseDurationSec = 900.0;
        requiresHeat = 1;
    };

    // Trocknen. Rohsalz -> Salz.
    //
    // baseDurationSec = 1200 (20 min) OHNE requiresHeat: Trocknen kostet Zeit,
    // aber keinen Brennstoff. Das Feuer darf ausgehen - der Job pausiert dabei
    // nicht, weil er keine Waerme verlangt (11 §7).
    //
    // Eigener Prozess statt des gemeinsamen PROCESS_DRY: Salz trocknet unter
    // ganz anderen Bedingungen als Kraeuter oder Fleisch, und ein gemeinsamer
    // Prozess haette zwei Slices dieselbe Registry-ID mit verschiedenen Werten
    // schreiben lassen. Ein Trockenrahmen aus einem spaeteren Slice kann
    // PROCESS_DRY_SALT jederzeit zusaetzlich anbieten - der Transform bindet
    // bewusst KEINE Station (stationsAllowed fehlt = jede Station, die den
    // Prozess kann).
    class PROCESS_DRY_SALT
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_DRY_SALT";
        baseDurationSec = 1200.0;
        requiresHeat = 0;
    };

    //--------------------------------------------------------------------------
    // ### SLICE meat ### Die vier Verben der Fleischkette.
    //
    // Ein Prozess ist ein VERB ohne Objekt: er sagt, WIE gearbeitet wird - nie,
    // WORAUS was wird. Das steht in den Transforms in ChefZ_Meat.
    //--------------------------------------------------------------------------

    // §29: Raw Meat + Knife -> ChefZ_DicedMeat. HANDCRAFT, weil es OHNE Station
    // gehen muss: es ist der frueheste Schritt der Kette, und wer noch kein
    // Brett hat, soll trotzdem Fleisch wuerfeln koennen. Ein Eingang plus
    // Werkzeuggruppe ist genau die Form, die Vanillas RecipeBase traegt
    // (01 V12: MAX_NUMBER_OF_INGREDIENTS = 2, das Werkzeug belegt den zweiten
    // Platz).
    class PROCESS_CUT_MEAT
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_CUT_MEAT";
        toolGroups[] = {"CUTTING_TOOL"};
        baseDurationSec = 4.0;
        animationLength = 1.0;
        specialty = 0.02;
        toolDamage = 2;
    };

    // §30/§57: Meat -> Minced Meat am Fleischwolf. STATION_ACTION: der Spieler
    // kurbelt, und der Fortschritt braucht einen Anker (11 §3).
    class PROCESS_GRIND_MEAT
    {
        exec = "STATION_ACTION";
        displayName = "#STR_CHEFZ_PROC_GRIND_MEAT";
        baseDurationSec = 20.0;
    };

    // §34-§39: Wurst fuellen. STATION_ACTION und ausdruecklich NICHT HANDCRAFT:
    // die Wurstrezepte haben drei bis fuenf Eingaenge, Vanillas Craftsystem
    // kennt zwei (01 V12). An der Station gibt es diese Grenze nicht (11 E1).
    class PROCESS_STUFF_SAUSAGE
    {
        exec = "STATION_ACTION";
        displayName = "#STR_CHEFZ_PROC_STUFF_SAUSAGE";
        baseDurationSec = 15.0;
    };

    // §33: Intestines -> ChefZ_SausageCasing am Schneidebrett.
    // Werkzeuggruppe trotz Station: der Darm wird aufgeschnitten, nicht
    // gepresst.
    class PROCESS_CLEAN_CASING
    {
        exec = "STATION_ACTION";
        displayName = "#STR_CHEFZ_PROC_CLEAN_CASING";
        toolGroups[] = {"CUTTING_TOOL"};
        baseDurationSec = 12.0;
        toolDamage = 1;
    };
};

//------------------------------------------------------------------------------
// ### SLICE grain ### Werkzeuggruppe des Nudelholzes.
//
// Sie steht in Rang 1 und nicht in JSON, weil ActionCondition clientseitig
// laeuft (02 §2): der Spieler muss sehen, ob er den Teig ausrollen kann, bevor
// der Server irgendetwas bestaetigt.
//
// Der Name traegt bewusst KEIN ChefZ_-Praefix: er ist ein Symbol in einem
// offenen Namensraum, keine Item-Klasse. Ein fremdes Modul darf derselben
// Gruppe eigene Klassen beisteuern - genau dafuer ist eine Werkzeuggruppe da.
//
// Der STATIONSDATENSATZ zu ChefZ_GrainMill und die ZUTATENBINDUNG zu
// ChefZ_Flour liegen dagegen in Rang 2
// (Config/GrainStations.json, Config/GrainIngredients.json): 11 §2 verlangt
// "id == Klassenname der Station" und 02 §4 dasselbe fuer eine Zutat - ein
// gleichnamiger Knoten neben der CfgVehicles-Klasse zaehlt fuer configcpp.mjs
// als doppelte Klassendefinition. Beide Recordarten sind laut 02 §2
// ausdruecklich auch in JSON zulaessig.
//------------------------------------------------------------------------------
class CfgChefZTools
{
    class ROLLING_PIN
    {
        toolCategories[] = {"ROLLING_TOOL"};
        classes[] = {"ChefZ_RollingPin"};
        allowSubclasses = 1;
    };

    // ### SLICE meat ###
    //
    // Gruppenweise Schreibweise (ChefZ_ToolGroupDef, Kopf): id ist die GRUPPE,
    // classes[] sind ihre Mitglieder. Genau dafuer ist sie da - ChefZ fasst
    // keine Vanilla-config.cpp an, sondern nennt fremde Klassen in einer
    // eigenen Gruppe (11 E8).
    //
    // Die Gruppe ist geteiltes Vokabular: Gemuese schneiden, Kraeuter
    // schneiden und Fleisch schneiden verlangen dasselbe Messer. Sie steht
    // deshalb hier und nicht in ChefZ_Meat, und sie wird bewusst nur EINMAL
    // deklariert - ein zweiter Knoten gleichen Namens in einem anderen Modul
    // waere eine doppelte Klassendefinition.
    class CUTTING_TOOL
    {
        classes[] =
        {
            "KitchenKnife",
            "SteakKnife",
            "HuntingKnife",
            "CombatKnife",
            "KukriKnife",
            "BoneKnife",
            "StoneKnife",
            "FangeKnife"
        };
        allowSubclasses = 1;
    };
};
