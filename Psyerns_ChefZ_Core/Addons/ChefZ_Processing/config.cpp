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
            "ChefZ_CuttingBoard", "ChefZ_MeatGrinder",
            // ### SLICE preservation ### (Production Map §57: Raeucherschrank.
            // Der Trockenrahmen steht schon oben beim Slice "herbs" - er ist
            // dieselbe Station und bekommt hier nur neue Transforms, keine
            // zweite Klasse.)
            "ChefZ_Smoker"
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
    // Die einzige Configbasis dieses Moduls. Es gibt hier bewusst KEINE
    // Vorwaertsdeklaration von Pot oder Cauldron mehr: beide sind Vanillas
    // Kochgefaesse und stehen zugleich in CfgChefZDevices
    // (ChefZ_Cooking/config.cpp:2843ff.) als ChefZ-Kochgeraete. Eine Station,
    // die von ihnen erbt, wird von ChefZ_CookingDeviceAdapter.BuildDescriptor
    // ueber den Aufstieg entlang CfgVehicles als Kochgeraet erkannt. Siehe den
    // Block "### SLICE dairy ###" weiter unten.
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
    // ---------------------------------------------------------------------
    // WARUM Inventory_Base UND NICHT LAENGER Pot / Cauldron  (Blocker G4-E2)
    // ---------------------------------------------------------------------
    // Beide Stationen erbten bis Gate 4 von Vanillas Kochgefaessen. Das war
    // falsch, und zwar aus drei voneinander unabhaengigen Gruenden:
    //
    // 1. CONFIGSEITE. Pot und Cauldron stehen in CfgChefZDevices
    //    (ChefZ_Cooking/config.cpp: Pot portionCapacity = 4, Cauldron = 12).
    //    ChefZ_CookingDeviceAdapter.BuildDescriptor steigt die
    //    CfgVehicles-Kette hoch, findet die Basis und setzt desc.enabled auf
    //    true. Fuer ChefZ war das Butterfass damit ein Topf mit 4 Portionen
    //    und die Kaesepresse ein Kessel mit 12 - und jedes Rezept mit
    //    deviceClasses ["Pot"] bzw. ["Cauldron"] (BowlDishes.json, DishesB.json,
    //    Dishes_A.json, Sauces.json, ChefZ_Baking/Config/GrainRecipes.json)
    //    haette im Butterfass gematcht.
    //
    //    Die alte Begruendung an dieser Stelle - "fuer die Kochlogik kein
    //    Kochgeschirr, weil die SKRIPTklasse von ChefZ_ProcessingStation_Base
    //    erbt" - trug nur fuer Vanillas IsCookware() (Skriptseite). Die eigene
    //    Geraeteaufloesung von ChefZ liest die CONFIG und hat die Skriptkette
    //    nie gesehen.
    //
    // 2. VANILLASEITE. FireplaceBase.CookOnDirectSlot ruft
    //    Cooking.CookWithEquipment fuer jedes Item in DirectCookingA/B/C, ohne
    //    IsCookware() zu pruefen. Ueber das von Pot bzw. Cauldron geerbte
    //    inventorySlot[] liessen sich beide Stationen in einen Direktkochplatz
    //    haengen und auf eine Feuerstelle stellen.
    //
    // 3. HIERARCHIE. Config erbte Pot (mit liquidContainerType, varQuantity*,
    //    inventorySlot[]), das Skript ChefZ_ProcessingStation_Base -> ItemBase.
    //    Pot.SetActions() mit ActionDrinkCookingPot / ActionEmptyCookingPot lag
    //    nie in der Kette: ein Behaelter, den man fuellen, aber nicht leeren
    //    kann.
    //
    // Dieselbe Datei lehnt bei ChefZ_SaltPan die Ableitung von Pot/Cauldron
    // schon laenger ausdruecklich ab. Beide Stationen folgen jetzt genau
    // diesem Vorbild - Inventory_Base, Modell ueber model=, Eingangsseite ueber
    // einen eigenen Cargo-Block.
    //
    // ---------------------------------------------------------------------
    // BRAUCHT DIE MILCHKETTE EINE FLUESSIGKEITSFUEHRUNG? NEIN.
    // ---------------------------------------------------------------------
    // Nachgeprueft an Config/Processing/Dairy_Transforms.json: alle drei
    // Transforms (TR_MilkToCream, TR_CreamToButter, TR_MilkToCheese) matchen
    // ueber "cls" auf ITEM-Klassen - ChefZ_Milk und ChefZ_Cream. Keiner nennt
    // isLiquidContainer oder liquidType. Sahne und Bruch sind in V1 Stueckware,
    // keine Fluessigkeit; die Station traegt sie in ihrem Cargo, genau wie der
    // Trockenrahmen seine Kraeuter.
    //
    // Die einzige Stelle im Modul, die tatsaechlich mit Fluessigkeit arbeitet,
    // ist Config/Processing/Salt.json - und auch dort ist der Eingang ein
    // GEFUELLTER BEHAELTER im Cargo, nie die Station selbst. Ein Kochgefaess
    // als Basis war also an keiner Stelle noetig.
    //
    // ---------------------------------------------------------------------
    // MODELL UND CARGO, die beiden Dinge, die Pot/Cauldron mitgebracht haben
    // ---------------------------------------------------------------------
    // Beides steht jetzt ausgeschrieben da:
    //
    // - model=: Vanilla-Proxy wooden_case.p3d aus DZ_Gear_Camping, derselbe
    //   Pfad, den ChefZ_GrainMill und ChefZ_Smoker bereits benutzen. Er ist
    //   damit im Modul nachweislich gueltig, und er zeigt vor allem KEINE
    //   Kochgeschirr-Silhouette mehr - ein Butterfass, das wie ein Kochtopf
    //   aussieht, laedt genau zu der Verwechslung ein, die dieser Blocker
    //   beseitigt. Ziele bleiben hoelzernes Stossbutterfass und hoelzerne
    //   Presse mit Spindel; der Asset-Bedarf ist im Slice-Bericht gemeldet und
    //   waechst durch diesen Wechsel um zwei Stationen, die sich das Kistenmesh
    //   mit Muehle und Raeucherschrank teilen.
    //
    // - class Cargo: die Eingangsseite. ChefZ_ProcessingStation_Base liest
    //   seine Zutaten ueber ChefZ_FactCollector.CollectFromCargo aus genau
    //   diesem Bereich; ohne ihn faende ein Job nie eine Zutat. Die Groessen
    //   sind aus den Transforms gerechnet und in beiden Rasterlesarten
    //   tragfaehig: ChefZ_Milk ist 2x3, ChefZ_Cream 2x2.
    //     Butterfass  4x4 - TR_MilkToCream und TR_CreamToButter verlangen je
    //                 2 Einheiten.
    //     Presse      6x4 - TR_MilkToCheese verlangt 3 Milch gleichzeitig.
    //   Ein Cargo, der groesser ist als itemSize, ist in diesem Modul kein
    //   Sonderfall: ChefZ_HerbStationBase macht es genauso.
    //==========================================================================

    // §48/§49: Das Butterfass traegt ZWEI Prozesse - erst abrahmen, dann
    // schlagen. Production Map §48 nennt dafuer zwar eine eigene Station
    // ("ChefZ_DairyProcessor") und erlaubt im selben Satz das vereinfachte
    // Processing fuer V1; zwei Stationen fuer eine dreigliedrige Kette waeren
    // eine Station zu viel. Das Fass kann beides, weil es beide Male dasselbe
    // tut: ruehren.
    class ChefZ_ButterChurn : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BUTTERCHURN";
        descriptionShort = "#STR_CHEFZ_ITEM_BUTTERCHURN_DESC";
        model = "\DZ\gear\camping\wooden_case.p3d";
        rotationFlags = 17;
        itemSize[] = {4, 4};
        weight = 4200;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        lifetime = 172800;

        class Cargo
        {
            itemsCargoSize[] = {4, 4};
            openable = 0;
        };
    };

    // §50: Die Kaesepresse.
    class ChefZ_CheesePress : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHEESEPRESS";
        descriptionShort = "#STR_CHEFZ_ITEM_CHEESEPRESS_DESC";
        model = "\DZ\gear\camping\wooden_case.p3d";
        rotationFlags = 17;
        itemSize[] = {5, 4};
        weight = 6800;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        lifetime = 172800;

        class Cargo
        {
            itemsCargoSize[] = {6, 4};
            openable = 0;
        };
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

    //==========================================================================
    // ### SLICE preservation ###   Production Map §41/§46/§57, DME-Plan §31.3
    //
    // ChefZ_Smoker - der Raeucherschrank. Die zweite und letzte neue Station
    // der V1-Preservation-Matrix; der Trockenrahmen steht schon weiter oben.
    //
    // WARUM EINE EIGENE STATION UND NICHT VANILLAS RAEUCHERSLOTS (11 E6,
    // woertlich im Kopf von ChefZ_ProcessingStation_Base.c):
    // Vanillas Cooking.SmokeItem kennt GENAU EINEN Uebergang RAW -> DRIED und
    // sonst BURNED (01 V14). Die Matrix §56 verlangt vier Uebergaenge mit
    // VERSCHIEDENEN Haltbarkeiten - geraeuchert ist nicht getrocknet, obwohl
    // beide auf dieselbe Vanilla-Garstufe projizieren. In Vanillas Kette ist
    // das nicht abbildbar, und der Versuch haette ein
    // "modded class FireplaceBase" gekostet (Verstoss gegen I6).
    //
    // Vanilla-Raeuchern in den Smoking-Slots eines Fasses bleibt dadurch EXAKT
    // wie es ist. Diese Station fasst Vanillas Kochkette an keiner Stelle an.
    //
    // WARUM Inventory_Base UND NICHT FireplaceBase ODER Barrel_ColorBase:
    // beide bringen die gesamte Feuerstellenmechanik mit - Brennstoffverwaltung,
    // Kochslots, Rauchslots und Cooking.ProcessItemToCook mit seinem
    // PARAM_BURN_DAMAGE_COEF. Die Wurst im Schrank ginge darin kaputt, bevor
    // der ChefZ-Job auch nur laeuft. Die Waerme holt sich der Schrank statt
    // dessen aus einer brennenden Feuerstelle in Reichweite - dieselbe Loesung
    // wie bei ChefZ_SaltPan und aus demselben Grund.
    //
    // Der Cargo-Bereich IST die Eingangsseite: ChefZ_ProcessingStation_Base
    // liest seine Zutaten ueber ChefZ_FactCollector.CollectFromCargo aus genau
    // diesem Bereich. 4x3 fasst sechs Wuerste oder Filets - ein Raeuchergang
    // soll sich lohnen, ohne ein Lager zu sein.
    //
    // MODELL: Vanilla-Proxy wooden_case.p3d, eine Holzkiste in der richtigen
    // Groessenordnung. Ziel ist ein hoher, schmaler Holzschrank mit Rost und
    // Rauchabzug - eigene Geometrie, siehe Asset-Bedarf des Slice.
    //==========================================================================
    class ChefZ_Smoker : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SMOKER0";
        descriptionShort = "#STR_CHEFZ_ITEM_SMOKER1";
        model = "\DZ\gear\camping\wooden_case.p3d";
        rotationFlags = 2;
        itemSize[] = {6, 5};
        weight = 11000;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        lifetime = 172800;

        class Cargo
        {
            itemsCargoSize[] = {4, 3};
            openable = 0;
        };
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

    // ### SLICE preservation ###
    //
    // Nur der Stationsdatensatz des Raeucherschranks. Die Transforms der
    // Konservierungskette liegen in ChefZ_Preservation und melden sich dort an -
    // hier stuende sonst Content eines anderen Moduls.
    //
    // Der Trockenrahmen fehlt in dieser Liste, weil er nicht fehlt: sein
    // Datensatz steht in HerbStations.json und bietet PROCESS_DRY bereits an.
    // Ein zweiter Datensatz gleicher ID waere ein doppelter Record desselben
    // Rangs, und ChefZ_RecordSink weist einen solchen ab, statt ihn zu patchen.
    //
    // handcraftRecipeSlots = 0: der einzige HANDCRAFT-Prozess des Slice
    // (PROCESS_SALT_CURE) traegt seine beiden Transforms in ChefZ_Preservation,
    // und dort sind die zwei Plaetze auch reserviert. Zweimal reservieren hiesse
    // vier Plaetze belegen und zwei davon leer lassen.
    class ChefZ_PreservationStations
    {
        chefzApiVersion = 1;
        loadOrder = 270;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Processing/Config/Processing/PreservationStations.json"
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

    //--------------------------------------------------------------------------
    // ### SLICE preservation ### Die zwei fehlenden Verben der Matrix §56.
    //
    // Das dritte, PROCESS_DRY, steht schon oben beim Slice "herbs" und wird
    // hier BEWUSST NICHT zweitgenannt: der Kopf jenes Blocks sagt ausdruecklich
    // "PROCESS_DRY ist bewusst allgemein gehalten und traegt keinen
    // Kraeuterbezug: derselbe Trockenrahmen trocknet laut Production Map §57
    // auch Fleisch, Fisch und Nudeln". Genau dieser Fall tritt hier ein. Ein
    // zweiter Knoten gleichen Namens waere eine doppelte Klassendefinition und
    // ein zweiter Ort fuer dieselbe Dauer.
    //--------------------------------------------------------------------------

    // §41/§46: Raeuchern. STATION_TIMED - der Schrank arbeitet stundenlang und
    // ohne Spieler (11 §3).
    //
    // requiresHeat = 1: §41 nennt "Raw Sausage + Smoker + FUEL". Das ist der
    // Preis der laengsten Haltbarkeit der Matrix - 30 Minuten Feuer neben dem
    // Schrank. Ohne diese Bedingung waere Raeuchern strikt besser als Trocknen
    // und Trocknen damit sinnlos.
    //
    // KEIN minTemperature: die Waermebedingung haengt an einer BRENNENDEN
    // Feuerstelle in Reichweite, nicht an der Eigentemperatur des Schranks -
    // dieselbe Festlegung wie bei PROCESS_BOIL_BRINE und aus demselben Grund.
    // Ein Temperaturschwellwert waere hier eine geratene Zahl.
    class PROCESS_SMOKE
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_SMOKE";
        baseDurationSec = 1800.0;
        requiresHeat = 1;
    };

    // §43/§45: Salzen und Poekeln. HANDCRAFT, und das ist die einzige
    // Ausfuehrungsform, die hier passt:
    //
    // - Der Vorgang hat KEINEN Ort. §43 und DME-Plan §31.1 nennen "Raw Meat +
    //   Salt" und keine Station. Wer gerade ein Reh zerlegt hat, soll es
    //   einsalzen koennen, ohne erst eine Station zu bauen - Poekeln ist der
    //   frueheste Schritt der Konservierungskette.
    // - Er hat GENAU ZWEI Eingaenge, Fleisch und Salz. Das ist woertlich die
    //   Form, die Vanillas RecipeBase traegt: MAX_NUMBER_OF_INGREDIENTS = 2
    //   (01 V12). Beide Plaetze sind mit Zutaten belegt - deshalb steht hier
    //   KEINE toolGroups-Zeile. Ein Werkzeug waere der dritte Platz, und den
    //   gibt es nicht.
    //
    // baseDurationSec = 6: kein Wartevorgang. Das Einreiben dauert Sekunden,
    // die Haltbarkeit entsteht danach von selbst.
    class PROCESS_SALT_CURE
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_SALT_CURE";
        baseDurationSec = 6.0;
        animationLength = 1.0;
        specialty = 0.02;
        toolDamage = 0;
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
