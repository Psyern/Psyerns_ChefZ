// ChefZ_Processing - Getreidemuehle, Pastamaschine, Mehl (Slice "grain").
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
// Muehle und Pastamaschine tragen Vanilla-Proxy-Modelle und sind im
// Slice-Bericht als Asset-Bedarf gemeldet (Production Map §70: beide brauchen
// eigene Geometrie). Auf ein Modell wartet hier nichts.

class CfgPatches
{
    class ChefZ_Processing
    {
        units[] = {
            "ChefZ_GrainMill", "ChefZ_PastaMachine", "ChefZ_Flour",
            // Slice "herbs" (DME-Plan §6.3 und §6.7, Production Map §57)
            "ChefZ_HerbStationBase", "ChefZ_Mortar", "ChefZ_DryingRack",
            // ### SLICE dairy ###
            "ChefZ_ButterChurn", "ChefZ_CheesePress",
            // ### SLICE salt ###
            "ChefZ_FryingPan",
            // ### SLICE meat ### (Production Map §57: Fleischwolf. Das
            // Schneidebrett gibt es nicht mehr - Schneiden ist "Zutat +
            // Messer kombinieren", Entscheidung vom 29.08.2026.)
            "ChefZ_MeatGrinder",
            // ### SLICE preservation ### (Production Map §57: Raeucherschrank.
            // Der Trockenrahmen steht schon oben beim Slice "herbs" - er ist
            // dieselbe Station und bekommt hier nur neue Transforms, keine
            // zweite Klasse.)
            "ChefZ_Smoker",
            // ### SLICE apiary ### (Auftrag: "Honey_Extractor"). Die uebrigen
            // Imkereiklassen liegen in ChefZ_Farming - Begruendung an der
            // Klasse.
            "ChefZ_HoneyExtractor"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // ChefZ_Core:     ChefZ_ProcessingStation_Base, ChefZ_Item_Base.
        // ChefZ_Farming:  ChefZ_GrainFoodBase (Nahrungsbasis) und ChefZ_Wheat
        //                 als Eingang des Mahlvorgangs.
        // DZ_Gear_*:      die Proxy-Modelle.
        // ### SLICE apiary ###
        // DZ_Gear_Food:        Honey, das fertige Honigglas - TR_SpinHoney
        //                      erzeugt es, sonst nichts. Steht schon oben.
        // ChefZ_Farming:       steht schon oben - liefert ausserdem
        //                      ChefZ_HoneycombFrameUncapped, den Eingang von
        //                      TR_SpinHoney, und ChefZ_HoneycombFrameEmpty,
        //                      wozu ein leergeschleuderter Rahmen wird.
        // DZ_Gear_Cooking:     steht schon oben - Cauldron.p3d, der Proxy der
        //                      Schleuder.
        //
        // KEIN Rinden-Addon, obwohl ChefZ_Smoker seit dem 31.08.2026 Rinde
        // verbrennt: der Schrank nennt weder Bark_Oak noch Bark_Birch. Er
        // fragt ausschliesslich die SKRIPTBASIS Bark_ColorBase ab, und die
        // steht in Vanillas Basisskripten (scripts - 1.29/4_World/DayZ/
        // Entities/ItemBase/Bark_ColorBase.c) - sie ist immer uebersetzt,
        // unabhaengig davon, welches Gear-Addon die beiden CfgVehicles-Knoten
        // mitbringt. Ein requiredAddons-Eintrag waere hier eine Abhaengigkeit
        // ohne Gegenstueck. Wer Rinde im Spiel hat, hat sie ohnehin.
        //
        // KEIN ChefZ_Cooking, obwohl TR_SpinHoney dessen ChefZ_EmptyJar als
        // leeres Glas nennt: ChefZ_Cooking fuehrt ChefZ_Processing bereits in
        // seinem requiredAddons, die Gegenrichtung waere ein Ladezyklus. Der
        // Transform nennt die Klasse nur als NAMEN und bindet zur Laufzeit;
        // das Skript der Schleuder prueft sie ueber IsKindOf, nie ueber einen
        // Klassenbezug. Fehlt ChefZ_Cooking, matcht der Transform schlicht nie.
        requiredAddons[] = {"DZ_Data", "DZ_Gear_Camping", "DZ_Gear_Tools", "DZ_Gear_Food", "ChefZ_Core", "ChefZ_Farming", "DZ_Gear_Cooking", "ChefZ_Devices"};
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
    // ChefZ_GrainFoodBase kommt aus ChefZ_Farming und MUSS hier
    // vorwaertsdeklariert werden - aus demselben Grund wie Inventory_Base eine
    // Zeile darueber. DayZ loest eine Elternklasse nur innerhalb DERSELBEN
    // config.cpp auf; fehlt sie, bricht der Configlauf mit "Undefined base
    // class" ab. Die frueher hier notierte Sorge, eine leere
    // Vorwaertsdeklaration verdecke den echten Knoten, trifft nicht zu: eine
    // Deklaration ohne Rumpf ersetzt nichts. Siehe den ausfuehrlichen Vermerk
    // in ChefZ_Baking/config.cpp.
    class ChefZ_GrainFoodBase;

    //--------------------------------------------------------------------------
    // Die Getreidemuehle (Production Map §57).
    //
    // Sie ist ein tragbares und ablegbares Objekt, kein Kochgeraet: sie fasst
    // Vanillas Kochkette an keiner Stelle an (11 E6).
    //
    // Eingang: Weizen (TR_WheatToFlour) oder Mais (TR_CornToFlour, seit
    // 29.08.2026), Ausgang immer ChefZ_Flour - es gibt kein eigenes Maismehl.
    //
    // PROXY: wooden_case.p3d - eine Holzkiste in der richtigen Groessenordnung.
    // Eigenes Muehlenmesh ist gemeldet (U, P1).
    //--------------------------------------------------------------------------
    class ChefZ_GrainMill : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_GRAINMILL";
        descriptionShort = "#STR_CHEFZ_GRAINMILL_DESC";
        model = "\ChefZ\ChefZ_Devices\models\grainmill.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        weight = 9000;
        itemSize[] = {6, 4};
        canBeDigged = 0;
        rotationFlags = 2;
        lifetime = 172800;

        // Der Cargo-Bereich IST die Eingangsseite (31.08.2026, Testbefund
        // Alex "Muehle hat keine Funktion"). Bis hierher hatte die Muehle als
        // EINZIGE Station dieses Moduls keinen Cargo-Block, und damit war die
        // ganze Mahlkette tot: ChefZ_ProcessingStation_Base sammelt seine
        // Zutaten ausschliesslich ueber ChefZ_FactCollector.CollectFromCargo
        // aus GetInventory().GetCargo(). Ohne Cargo gibt es kein
        // GetCargo() - der Job findet nie eine Zutat und meldet dabei nichts,
        // was nach einem Fehler aussieht. GrainTransforms.json hat diesen
        // Zustand seit dem 29.08.2026 in einer _note festgehalten; die note
        // ist mit diesem Block hinfaellig und dort entfernt.
        //
        // 5x4 = 20 Zellen, gerechnet aus den beiden Transforms:
        //   TR_CornToFlour  maxCount 5. Ein Kolben (ChefZ_Corn) ist 1x1
        //                   (ChefZ_Farming/config.cpp:275, geerbt von
        //                   ChefZ_VegetableFood_Base) - fuenf Kolben kosten
        //                   fuenf Zellen.
        //   TR_WheatToFlour maxCount 1. Eine Garbe (ChefZ_Wheat) ist 2x2
        //                   (ChefZ_Farming/config.cpp:222) - vier Garben
        //                   liegen in 16 Zellen und lassen vier fuer das Mehl.
        // Das Ergebnis (ChefZ_Flour, 2x2) landet im SELBEN Cargo: getrennte
        // Ein- und Ausgangsbereiche gibt die Engine nicht her, ein Item hat
        // genau einen Cargo. Deshalb ist der Rand von vier Zellen kein
        // Luxus - ist der Cargo voll, endet der Job ohne Verbrauch.
        class Cargo
        {
            itemsCargoSize[] = {5, 4};
            openable = 0;
        };
    };

    //--------------------------------------------------------------------------
    // Die Pastamaschine (Production Map §58, §11; frueher ChefZ_RollingPin).
    //
    // WARUM WERKZEUG UND NICHT STATION
    // --------------------------------
    // Eine Nudelmaschine sieht nach Station aus, und die Muehle daneben ist
    // eine. Der Unterschied liegt nicht im Objekt, sondern in dem, was an ihm
    // passiert:
    //
    //  - Die beiden Vorgaenge, die sie ueberhaupt bedient, haben GENAU EINEN
    //    Eingang: TR_DoughToRawPasta
    //    (ChefZ_Baking/Config/GrainTransforms.json). Der Vorteil einer Station
    //    ist, dass sie mehr als zwei Eingaenge traegt (11 E1) - hier gibt es
    //    nichts, was diesen Vorteil abrufen wuerde.
    //  - Ein Eingang plus Werkzeuggruppe ist genau die Form, die Vanillas
    //    RecipeBase traegt (01 V12): das Werkzeug belegt den zweiten
    //    Zutatenplatz. Ohne Werkzeug waere ein Ein-Eingang-Transform als
    //    Handwerksrezept gar nicht registrierbar - es gaebe nichts zum
    //    Kombinieren. Die Maschine IST dieser zweite Platz.
    //  - Ein Wechsel auf STATION_ACTION waere kein Eintrag hier, sondern vier
    //    Aenderungen in ChefZ_Baking (exec von PROCESS_ROLL, stationsAllowed
    //    an beiden Transforms, handcraftRecipeSlots). Bis die durch waeren,
    //    stuenden beide Transforms ohne Station da und die Backkette waere
    //    genau so tot wie vorher.
    //
    // WIE DER SPIELER SIE BEKOMMT - der eigentliche Punkt dieser Aenderung
    // --------------------------------------------------------------------
    // Der Vorgaenger ChefZ_RollingPin war das einzige Mitglied der Gruppe
    // ROLLING_PIN und hatte KEINE Quelle: kein Loot-Eintrag (ChefZ liefert
    // projektweit keine types.xml, G4-B8), kein erzeugender Transform. Damit
    // war PROCESS_ROLL unerreichbar und die Backkette hinter dem Teig tot -
    // ohne eine einzige Fehlermeldung.
    //
    // Zwei Quellen, beide ohne Zutun des Serverbetreibers:
    //   1. LOOT, sofort: die Vanillaklasse MeatTenderizer steht jetzt
    //      zusaetzlich in ROLLING_PIN.classes[] (Vanilla-Audit §4.2 D). Sie
    //      hat nominal 140 in Town/Village - die Backkette laeuft damit ab
    //      dem Moment, in dem der Mod geladen ist.
    //   2. CRAFT, gezielt: TR_AssemblePastaMachine (Config/GrainTransforms.json)
    //      baut die Maschine aus einem MetalPlate mit METALWORK_TOOL. Wer sie
    //      will, kann sie herstellen, statt auf ein Loot-Roll zu hoffen.
    //
    // PROXY: Meat_Tenderizer.p3d - dasselbe Modell, das die Vanillaklasse
    // MeatTenderizer traegt und damit ein im Projekt belegter Pfad
    // (ChefZ_Asset_Backlog §10.1 nennt ihn als den korrekten). Ein metallenes
    // Kuechengeraet mit Griff. Eigenes Pastamaschinenmesh ist gemeldet (U, P2).
    //--------------------------------------------------------------------------
    class ChefZ_PastaMachine : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_PASTAMACHINE";
        descriptionShort = "#STR_CHEFZ_PASTAMACHINE_DESC";
        model = "\dz\gear\tools\Meat_Tenderizer.p3d";
        weight = 2200;
        itemSize[] = {3, 2};
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
        model = "\ChefZ\ChefZ_Devices\models\mortar.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
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
    // Dieselbe Datei lehnt bei ChefZ_FryingPan die Ableitung von Pot/Cauldron
    // schon laenger ausdruecklich ab. Beide Stationen folgen jetzt genau
    // diesem Vorbild - Inventory_Base, Modell ueber model=, Eingangsseite ueber
    // einen eigenen Cargo-Block.
    //
    // ---------------------------------------------------------------------
    // BRAUCHT DIE MILCHKETTE EINE FLUESSIGKEITSFUEHRUNG? NEIN.
    // ---------------------------------------------------------------------
    // Nachgeprueft an Config/Processing/Dairy_Transforms.json: alle drei
    // Transforms (TR_MilkToCream, TR_CreamToButter, TR_MilkToCheese) matchen
    // ueber "cls" auf ITEM-Klassen - PowderedMilk und ChefZ_Cream. Keiner nennt
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
    //   tragfaehig: der Milchkarton (PowderedMilk) ist 2x3, ChefZ_Cream 2x2.
    //     Butterfass  10x14 - siehe die eigene Rechnung an der Klasse.
    //     Presse      6x4 - TR_MilkToCheese verlangt 3 Milch gleichzeitig.
    //   Ein Cargo, der groesser ist als itemSize, ist in diesem Modul kein
    //   Sonderfall: ChefZ_HerbStationBase macht es genauso, und die
    //   Honigschleuder traegt 10x10.
    //==========================================================================

    // §48/§49: Das Butterfass traegt ZWEI Prozesse - erst abrahmen, dann
    // schlagen. Production Map §48 nennt dafuer zwar eine eigene Station
    // ("ChefZ_DairyProcessor") und erlaubt im selben Satz das vereinfachte
    // Processing fuer V1; zwei Stationen fuer eine dreigliedrige Kette waeren
    // eine Station zu viel. Das Fass kann beides, weil es beide Male dasselbe
    // tut: ruehren.
    //
    // ---------------------------------------------------------------------
    // "20 LITER MILCH EINFUELLEN" - was das in Stueck heisst (31.08.2026)
    // ---------------------------------------------------------------------
    // Alex' Zielbild lautet woertlich: Milch einfuellen (20 Liter), aktiv
    // interagieren, und alle 60 Sekunden entsteht 1x Butter. Liter gibt es in
    // diesem Modul aber nicht - die Fluessigkeitsfuehrung ist oben
    // ausdruecklich verworfen, Milch ist in V1 Vanillas PowderedMilk als
    // STUECKWARE (ChefZ_Ingredients/config.cpp:389). Die Uebersetzung ist
    // deshalb festgelegt als:
    //
    //     1 Milchkarton (PowderedMilk) == 1 "Liter"
    //     Fassungsvermoegen             == 20 Karton
    //
    // Ein Karton ist 2x3 = 6 Zellen. 20 Karton brauchen 120 Zellen; 10x14 =
    // 140 laesst 20 Zellen fuer das, was das Fass SELBST erzeugt - Sahne
    // (ChefZ_Cream, 2x2) und Butter. Getrennte Ein- und Ausgangsbereiche gibt
    // die Engine nicht her: ein Item hat genau EINEN Cargo, und die Ergebnisse
    // landen darin. Ist er voll, endet der Job ohne Verbrauch, und das Fass
    // steht - deshalb der Rand.
    //
    // Die Zahl 20 selbst steht NICHT hier, sondern im Torwaechter der
    // Skriptklasse (ChefZ_DairyStations.c, CHEFZ_MILK_CAPACITY). Das Gitter
    // ist nur der Platz; gezaehlt wird in CanReceiveItemIntoCargo - dasselbe
    // Muster wie bei der Honigschleuder.
    class ChefZ_ButterChurn : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BUTTERCHURN";
        descriptionShort = "#STR_CHEFZ_ITEM_BUTTERCHURN_DESC";
        model = "\ChefZ\ChefZ_Devices\models\butterchurn.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        rotationFlags = 17;
        itemSize[] = {4, 4};
        weight = 4200;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        lifetime = 172800;

        class Cargo
        {
            itemsCargoSize[] = {10, 14};
            openable = 0;
        };
    };

    // §50: Die Kaesepresse.
    class ChefZ_CheesePress : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_CHEESEPRESS";
        descriptionShort = "#STR_CHEFZ_ITEM_CHEESEPRESS_DESC";
        model = "\ChefZ\ChefZ_Devices\models\cheesepress.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
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
    // ChefZ_FryingPan - die Siedepfanne. EINE Station traegt die ganze Kette:
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
    // ueber die Feuerstelle daneben (ChefZ_FryingPan.ChefZ_HasHeat).
    //
    // MODELL: Vanilla-Proxy FryingPan. Ziel: eine breite, flache Siedepfanne
    // mit Salzkruste - eigene Geometrie, siehe Asset-Bedarf des Slice.
    //==========================================================================
    class ChefZ_FryingPan : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_FRYINGPAN";
        descriptionShort = "#STR_CHEFZ_ITEM_FRYINGPAN_DESC";
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
    // Die EINE Station der Fleischkette (Production Map §57).
    //
    // KEIN SCHNEIDEBRETT MEHR (Entscheidung vom 29.08.2026): Schneiden ist
    // "Zutat + Messer kombinieren" - PROCESS_CUT_MEAT ist HANDCRAFT mit
    // CUTTING_TOOL und braucht
    // keinen Ort. Die frueher als Ausstattungsstueck erhaltene Klasse
    // ChefZ_CuttingBoard ist entfernt; ein Server, der eines platziert hatte,
    // verliert dieses eine Objekt beim naechsten Start.
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
    // MODELL: Vanilla-Proxy. Ziel waere ein gusseiserner Wolf mit Kurbel -
    // eigene Geometrie, siehe Asset-Bedarf des Slice.
    //==========================================================================

    class ChefZ_MeatGrinder : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MEATGRINDER0";
        descriptionShort = "#STR_CHEFZ_ITEM_MEATGRINDER1";
        model = "\ChefZ\ChefZ_Devices\models\meatgrinder.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        rotationFlags = 17;
        itemSize[] = {4, 4};
        weight = 3200;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;

        // Der Cargo-Bereich IST die Eingangsseite (31.08.2026, Testbefund
        // Alex "Fleischwolf hat keine Funktion"). Bis hierher hatte der Wolf
        // keinen Cargo-Block - genau der Fehler, an dem schon das entfernte
        // Schneidebrett gescheitert ist und den der Kopf dieses Blocks als
        // Andockregel nennt: ChefZ_ProcessingStation_Base sammelt seine
        // Zutaten ausschliesslich ueber ChefZ_FactCollector.CollectFromCargo
        // aus GetInventory().GetCargo(). Ohne Cargo gibt es kein GetCargo(),
        // und ALLE ZWOELF Transforms in ChefZ_Meat/Config/Processing/Meat.json
        // (sechs Wolf-, sechs Wurst-Transforms) waren unerreichbar - lautlos.
        //
        // 5x3 = 15 Zellen, Vorgabe Alex. Das ist kein Lager: ein Steak ist
        // 2x2, ein Darm (Guts) 2x2 - drei Steaks und ein Darm fuellen den
        // Wolf schon fast. Genau so soll es sein, denn das Ergebnis landet im
        // SELBEN Cargo (die Engine gibt keine getrennten Ein- und
        // Ausgangsbereiche her, ein Item hat genau einen Cargo), und ein
        // voller Cargo laesst den Job ohne Verbrauch enden. Wer wolfen will,
        // raeumt zwischendurch aus - das ist der Preis der Selbstnachstartung
        // von PROCESS_GRIND_MEAT, siehe dort.
        class Cargo
        {
            itemsCargoSize[] = {5, 3};
            openable = 0;
        };
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
    // der ChefZ-Job auch nur laeuft. Diese Ablehnung ist im Projekt dreifach
    // festgehalten und bleibt stehen.
    //
    // -----------------------------------------------------------------------
    // 31.08.2026 - DER SCHRANK HAT SEIN EIGENES FEUER (Testbefund Alex)
    // -----------------------------------------------------------------------
    // Bis hierher stand hier "die Waerme holt sich der Schrank aus einer
    // brennenden Feuerstelle in Reichweite". Das war eine Absicht, kein
    // Zustand: es gab dafuer NIE einen Ueberschreiber. Der Schrank war damit
    // seit seiner Anlage nicht ein einziges Mal betriebsbereit, und zwar aus
    // zwei voneinander unabhaengigen Gruenden:
    //
    //   1. Config/Processing/PreservationStations.json sagt needsFuel = true.
    //      Die Basisantwort ChefZ_ProcessingStation_Base.ChefZ_IsPowered()
    //      lautet dann "nein" (Core, Z. 403-407) - ausdruecklich als sichere
    //      Vorgabe, damit ein vergessener Ueberschreiber sichtbar wird. Er war
    //      vergessen. ChefZ_CompiledProcess.MeetsEnvironment brach jeden Job
    //      sofort ab.
    //   2. PROCESS_SMOKE traegt requiresHeat = 1, und ChefZ_HasHeat() lieferte
    //      die Basisantwort "nein" - der Schrank hatte keinen Ueberschreiber
    //      wie ihn ChefZ_FryingPan hat.
    //
    // Alex' Zielbild: "ein Platz, wo man das Feuer reinpacken kann wie beim
    // Vanilla-Ofen", raeuchern nur bei brennendem Feuer, Rinde als Brennstoff.
    // Umgesetzt ist das als EIGENER BRENNZUSTAND am Schrank, nicht als
    // Umkreisscan:
    //
    //   - Rinde (Bark_Oak / Bark_Birch, Skriptbasis Bark_ColorBase) liegt im
    //     Cargo und ist der Brennstoff.
    //   - Angezuendet wird ueber Vanillas ActionLightItemOnFire, also ohne
    //     eine einzige eigene Action: Feuerzeug oder Streichholz in die Hand,
    //     Schrank anvisieren. Der Schrank beantwortet dafuer die vier Fragen
    //     der Vanilla-Schnittstelle (HasFlammableMaterial, CanBeIgnitedBy,
    //     IsThisIgnitionSuccessful, OnIgnitedThis).
    //   - Brennt er, liefern ChefZ_IsPowered() UND ChefZ_HasHeat() "ja".
    //
    // Vorbild in jeder Zeile ist ChefZ_BeeSmoker (ChefZ_Farming/Scripts/
    // 4_World/ChefZ/Farming/ChefZ_Apiary.c:1136-1282). Die Einzelheiten und
    // die Wahl der Brenndauer stehen an der Skriptklasse.
    //
    // Der Umkreisscan von ChefZ_FryingPan bleibt dort, wo er hingehoert: eine
    // Siedepfanne STEHT auf dem Feuer, ein Raeucherschrank IST eines. Ein
    // Schrank, der sich an einer fremden Feuerstelle bedient, haette ausserdem
    // Alex' Punkt (1.3) - "Rinde muss als Brennstoff drin liegen" - nicht
    // erfuellen koennen.
    //
    // Der Cargo-Bereich IST die Eingangsseite: ChefZ_ProcessingStation_Base
    // liest seine Zutaten ueber ChefZ_FactCollector.CollectFromCargo aus genau
    // diesem Bereich. 5x5 = 25 Zellen (Vorgabe Alex 1.1, vorher 4x3): der
    // Cargo traegt jetzt ZWEIERLEI - das Raeuchergut und die Rinde, die den
    // Schrank heizt. Vier Wuerste (je 2x2), zwei Stueck Rinde und Platz fuer
    // das Ergebnis, das im selben Cargo entsteht.
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
        model = "\ChefZ\ChefZ_Devices\models\smoker.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        rotationFlags = 2;
        itemSize[] = {6, 5};
        weight = 11000;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        lifetime = 172800;

        class Cargo
        {
            itemsCargoSize[] = {5, 5};
            openable = 0;
        };
    };

    //==========================================================================
    // ### SLICE apiary ###   ChefZ_HoneyExtractor - die Honigschleuder
    // (Auftrag: "Honey_Extractor").
    //
    // Die uebrige Imkerei liegt in ChefZ_Farming - Bienenhaltung ist
    // Landwirtschaft. Die Schleuder liegt hier, weil Schleudern Verarbeitung
    // ist: es ist derselbe Vorgang wie Mahlen, Moersern oder Buttern, nur mit
    // anderem Gut. Sie steht damit neben Muehle, Moerser, Butterfass,
    // Kaesepresse, Salzpfanne und Raeucherschrank, und nicht neben dem
    // Gemuesebeet.
    //
    // Dass ChefZ_Processing ChefZ_Farming in requiredAddons fuehrt, macht die
    // Richtung ausserdem zur einzig moeglichen: die Eingangsklasse
    // ChefZ_HoneycombFrameUncapped kommt aus ChefZ_Farming. Umgekehrt waere es
    // ein Ladezyklus.
    //
    // --------------------------------------------------------------------
    // EINE STATION MIT ZWEI EINGAENGEN - und warum nicht zwei Schritte
    // --------------------------------------------------------------------
    // Der Auftrag trennt "[Schleudern]" und "[Abfuellen]". Als zwei Schritte
    // waere der Zwischenstand "Honig steht in der Schleuder" - also eine
    // FLUESSIGKEIT im Gefaess, kein Item. Genau dieser Fall ist in der Recipe
    // Engine nicht ausdrueckbar; die Begruendung steht ausgeschrieben an
    // ChefZ_FryingPan weiter oben ("Salzwasser IM Topf ist Fluessigkeit, kein
    // Item ... ChefZ_RecipeDef.Validate weist ein Rezept ohne slots
    // ausdruecklich ab"). Ein eigener Zwischenstand als ITEM waere eine
    // vierte Wabenklasse ohne eigene Aussage.
    //
    // Als EIN Schritt an der Station ist es dagegen glatt: entdeckelte Rahmen
    // und leere Glaeser (ChefZ_EmptyJar) liegen zusammen im Cargo, der Spieler
    // kurbelt an, und je Glas wird EIN leeres Glas sofort durch Vanillas Honey
    // ersetzt - das Glas verschwindet, das Honigglas entsteht im Cargo
    // (Auftrag 12). Der Rahmen gibt je Glas eine Einheit Vorrat her und traegt
    // seit dem 31.08.2026 FUENF davon: VIER GLAESER plus eine Reserveeinheit
    // (Auftrag 11 von Alex - vier Glaeser, nicht drei, und der entdeckelte
    // Rahmen soll danach ein LEERER Rahmen sein). Beides zugleich geht nur
    // ueber die fuenfte Einheit: sie faengt den Zug ab, der den Rahmen sonst
    // loeschen wuerde, und wird beim Wandeln zum Leerrahmen mit
    // aufgegeben. Die vollstaendige Herleitung steht an der Klasse in
    // ChefZ_Farming und im Kopf von ChefZ_HoneyExtractor.c. Zwei Eingaenge sind an
    // einer Station folgenlos - die Grenze von zwei Zutaten gilt nur fuer
    // HANDCRAFT (01 V12), und dort waere die Kette gar nicht abbildbar: zwei
    // Eingaenge liessen keinen Platz fuer ein Werkzeug, und die Schleuder IST
    // das Werkzeug.
    //
    // STATION_TIMED, nicht STATION_ACTION: der Spieler kurbelt nur AN. Danach
    // arbeitet die Schleuder selbst, 90 s je Glas, und ihr Skript startet nach
    // jedem Abschluss den naechsten Job, solange Rahmen mit Vorrat und Glaeser
    // liegen (Annahme A3). Ein Spieler, der fuenfzehn Glaeser lang kurbelt,
    // waere die Alternative - und nicht der Auftrag.
    //
    // --------------------------------------------------------------------
    // class Cargo IST die Eingangsseite
    // --------------------------------------------------------------------
    // ChefZ_ProcessingStation_Base liest seine Zutaten ueber
    // ChefZ_FactCollector.CollectFromCargo aus genau diesem Bereich. Ohne ihn
    // faende ein Job nie eine Zutat - der Fehler, an dem das fruehere
    // Schneidebrett gescheitert ist. 10x10 = 100 Zellen: fuenf Rahmen a 2x3
    // (30) und fuenfzehn Glaeser a 1x2 (30) sind der Auftrag, die Kapazitaet
    // zaehlt das Skript ueber CanReceiveItemIntoCargo. Der Rest ist Platz fuer
    // das erzeugte Honey, dessen Groesse ohne die Vanilla-Itemconfig nicht
    // belegbar ist. Ist das Cargo voll, endet ein Job ohne Verbrauch
    // (RUN_FAILED in der Basis), und die Schleuder steht, bis ein Spieler
    // Honig entnimmt UND erneut ankurbelt - das Entnehmen allein startet
    // nichts.
    //
    // KEIN Pot und KEIN Cauldron als Basis, obwohl eine Schleuder ein
    // Metallkessel ist: beide stehen in CfgChefZDevices und wuerden die
    // Schleuder fuer ChefZ zu einem Kochgefaess mit Portionszahl machen. Die
    // vollstaendige Begruendung steht an ChefZ_ButterChurn weiter oben.
    //
    // PROXY: Cauldron.p3d - ein grosses zylindrisches Metallgefaess. Genau die
    // Silhouette einer Schleuder, und ein im Projekt belegter Pfad
    // (ChefZ_MeatGrinder traegt ihn bereits). Das MODELL zu benutzen und die
    // KLASSE nicht zu beerben ist derselbe Unterschied, den ChefZ_Vanilla_
    // Assets §22.1 fuer CookingPot festhaelt. Ziel ist ein Blechzylinder mit
    // Kurbel und Auslaufhahn - eigene Geometrie, siehe Asset-Bedarf.
    //==========================================================================
    class ChefZ_HoneyExtractor : Inventory_Base
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HONEYEXTRACTOR";
        descriptionShort = "#STR_CHEFZ_ITEM_HONEYEXTRACTOR_DESC";
        model = "\dz\gear\cooking\Cauldron.p3d";
        rotationFlags = 2;
        itemSize[] = {5, 5};
        weight = 9500;
        absorbency = 0.0;
        canBeDigged = 0;
        varQuantityDestroyOnMin = 0;
        lifetime = 172800;

        class Cargo
        {
            itemsCargoSize[] = {10, 10};
            openable = 0;
        };
    };
};

//------------------------------------------------------------------------------
// Modulanmeldung am Config Manager (Entwurf 02 §4).
//
// handcraftRecipeSlots: dieses Modul reserviert EINEN Platz (02 §4.2):
//
//   ChefZ_GrainProcessing  1   TR_AssemblePastaMachine ueber PROCESS_ASSEMBLE
//
// Die uebrigen Knoten bleiben bei 0 - ihre Ketten laufen an Stationen. Der
// fruehere Platz fuer TR_SausageCasing ist mit der Huelle entfallen (29.08.2026).
// Projektweite Summe damit 23 (vorher 21) bei 22 HANDCRAFT-Transforms; der
// eine ueberzaehlige Platz gehoert ChefZ_Ingredients (12 reserviert, 11
// belegt) und ist nicht Teil dieser Aenderung. Ein unbelegter Platz ist
// folgenlos: das Rezeptobjekt bleibt unparametriert, ChefZ_GenericCraftRecipe.
// CanDo liefert false, es erscheint nie.
//------------------------------------------------------------------------------
class CfgChefZ
{
    // ### SLICE grain ### Ein Knoten je SLICE (02 §4), nicht je Modul - er
    // heisst deshalb nicht wie das Addon.
    // handcraftRecipeSlots = 1 (war 0): dieser Slice bringt seit der
    // Pastamaschine GENAU EINEN Transform mit, dessen Prozess exec =
    // "HANDCRAFT" hat - TR_AssemblePastaMachine ueber PROCESS_ASSEMBLE, beide
    // in diesem Modul. Die Zahl ist eine RESERVIERUNG in Vanillas Rezeptliste
    // und muss vor dem Laden feststehen; wird sie vergessen, erscheint das
    // Rezept nicht, und zwar OHNE Fehlermeldung an der Stelle, an der man
    // sucht (Begruendung im Kopf von ChefZ_HandcraftBridge.c).
    class ChefZ_GrainProcessing
    {
        chefzApiVersion = 1;
        loadOrder = 220;
        handcraftRecipeSlots = 1;
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
    // handcraftRecipeSlots = 0: Wuerfeln und Darm reinigen sind am 29.08.2026
    // entfallen; die drei Keulen-Transforms reserviert ChefZ_Meat selbst.
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

    // ### SLICE apiary ###   Die Honigschleuder.
    //
    // Eigener Knoten je SLICE (02 §4). loadOrder 275 - nach den uebrigen
    // Stationen dieses Moduls, damit die Reihenfolge deterministisch bleibt,
    // und nach ChefZ_Apiary in ChefZ_Farming (217), von dem die
    // Eingangsklassen kommen.
    //
    // handcraftRecipeSlots = 0: der einzige Prozess dieses Knotens
    // (PROCESS_SPIN_HONEY) ist STATION_TIMED und fasst Vanillas Rezeptliste
    // nicht an. Die sieben HANDCRAFT-Plaetze des Slice sind in ChefZ_Farming
    // unter ChefZ_Apiary reserviert, dort wo die Transforms liegen.
    class ChefZ_HoneyProcessing
    {
        chefzApiVersion = 1;
        loadOrder = 275;
        handcraftRecipeSlots = 0;
        dataFiles[] =
        {
            "ChefZ_Processing/Config/Processing/Honey_Stations.json",
            "ChefZ_Processing/Config/Processing/Honey.json"
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
    //
    // 60 statt 180 Sekunden (31.08.2026, Vorgabe Alex: "alle 60 s entsteht 1x
    // Butter"). Die Zahl ist der TAKT des Fasses, nicht die Dauer einer
    // Charge: das Fass startet sich nach jedem Abschluss selbst neu
    // (ChefZ_DairyStations.c), solange Material im Cargo liegt. Der Spieler
    // kurbelt einmal an und bekommt danach im Minutentakt Butter.
    //
    // Die beiden Transforms tragen dieselbe Zahl als durationOverrideSec
    // (Config/Processing/Dairy_Transforms.json). Das ist keine Dopplung,
    // sondern der uebliche Weg: der Prozess ist das Verb und nennt die
    // Vorgabe, der Transform ist der konkrete Vorgang und darf sie ueberschreiben
    // - so haelt auch die Kraeuterkette es (HerbGrinding.json).
    class PROCESS_CHURN_BUTTER
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_CHURN_BUTTER";
        baseDurationSec = 60.0;
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
    // Feuerstelle in Reichweite (ChefZ_FryingPan.ChefZ_HasHeat), nicht an der
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

    // Keule + Messer -> zwei Vanilla-Steaks (TR_CutBeefLeg / PorkLeg /
    // VenisonLeg). HANDCRAFT, weil es OHNE Station gehen muss: es ist der
    // frueheste Schritt der Kette. Ein Eingang plus Werkzeuggruppe ist genau
    // die Form, die Vanillas RecipeBase traegt (01 V12:
    // MAX_NUMBER_OF_INGREDIENTS = 2, das Werkzeug belegt den zweiten Platz).
    // Das Wuerfeln (§29) gibt es seit dem 29.08.2026 nicht mehr - die
    // Eintoepfe nehmen gewolftes Fleisch (MINCED_MEAT).
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

    // §30/§57: Meat -> Minced Meat am Fleischwolf.
    //
    // STATION_TIMED seit dem 31.08.2026 (vorher STATION_ACTION, 20 s).
    // Vorgabe Alex: "pro Einheit 30 Sekunden bis 1x Hackfleisch, solange
    // Fleisch im Cargo liegt". Genau das ist der Unterschied der beiden
    // Ausfuehrungsarten:
    //
    //   STATION_ACTION  der Spieler steht daneben, bis der Balken voll ist,
    //                   und zwar fuer JEDES Stueck Fleisch einzeln. Fuenf
    //                   Steaks waeren fuenf Aktionen.
    //   STATION_TIMED   der Spieler kurbelt EINMAL an, danach gehoert der Takt
    //                   der Station. Sie startet sich nach jedem Abschluss
    //                   selbst neu (ChefZ_MeatStations.c) - dasselbe Muster,
    //                   das die Honigschleuder seit dem Apiary-Slice benutzt.
    //
    // 30 statt 20 Sekunden, weil die Zahl jetzt eine andere Bedeutung hat: sie
    // ist der Takt einer unbeaufsichtigten Station, nicht die Wartezeit eines
    // anwesenden Spielers.
    class PROCESS_GRIND_MEAT
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_GRIND_MEAT";
        baseDurationSec = 30.0;
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

    // §41/§46: Raeuchern. STATION_TIMED - der Schrank arbeitet ohne Spieler
    // weiter (11 §3).
    //
    // 300 statt 1800 Sekunden (31.08.2026, Vorgabe Alex: "vollstaendig
    // raeuchern dauert 5 Minuten"). Die drei Transforms in
    // ChefZ_Preservation/Config/Processing/Smoking.json trugen bis dahin
    // 1800/1500/2400 als durationOverrideSec und stehen jetzt einheitlich
    // ebenfalls auf 300 - die laengste Haltbarkeit der Matrix kostet damit
    // nicht mehr Zeit als eine halbe Stunde Realzeit, sondern Brennstoff.
    // Genau das ist der Handel: fuenf Minuten Vollbrand sind zwei Stueck
    // Rinde (siehe ChefZ_Smoker.c), und Rinde muss man erst haben.
    //
    // requiresHeat = 1: §41 nennt "Raw Sausage + Smoker + FUEL". Ohne diese
    // Bedingung waere Raeuchern strikt besser als Trocknen und Trocknen damit
    // sinnlos.
    //
    // KEIN minTemperature: die Waermebedingung haengt seit dem 31.08.2026 am
    // EIGENEN Brennzustand des Schranks (ChefZ_Smoker.ChefZ_HasHeat), nicht an
    // seiner Eigentemperatur. Ein Temperaturschwellwert waere hier eine
    // geratene Zahl; der Brennzustand ist eine Tatsache.
    class PROCESS_SMOKE
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_SMOKE";
        baseDurationSec = 300.0;
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

    //--------------------------------------------------------------------------
    // ### SLICE grain ### Zusammenbauen.
    //
    // Das Verb, mit dem aus Metall ein Kuechengeraet wird. Heute haengt genau
    // ein Transform daran (TR_AssemblePastaMachine, Config/GrainTransforms.json)
    // - und der ist der Grund, warum es diesen Prozess gibt: die Pastamaschine
    // hatte als ChefZ_RollingPin ueberhaupt keine Quelle, weder Loot noch
    // Transform, und die Backkette stand deshalb still.
    //
    // Der Name traegt bewusst KEIN Objekt. Ein Prozess sagt, WIE gearbeitet
    // wird, nie WORAUS was wird - das steht im Transform. "PROCESS_BUILD_
    // PASTAMACHINE" waere ein Verb mit angewachsenem Objekt und muesste fuer
    // jedes weitere Geraet neu erfunden werden; PROCESS_ASSEMBLE traegt die
    // Muehle, den Fleischwolf und den Raeucherschrank gleich mit, sobald
    // jemand ihnen einen Transform gibt.
    //
    // HANDCRAFT und nicht STATION_*: ein Geraet baut man, BEVOR man eine
    // Station hat. Ein Zusammenbau, der eine Station verlangt, waere ein
    // Henne-Ei-Problem.
    //
    // EIN Eingang plus Werkzeuggruppe - die Form, die Vanillas RecipeBase
    // traegt (01 V12). toolDamage = 5: Metall biegen kostet das Werkzeug mehr
    // als Fleisch schneiden.
    //--------------------------------------------------------------------------
    class PROCESS_ASSEMBLE
    {
        exec = "HANDCRAFT";
        displayName = "#STR_CHEFZ_PROC_ASSEMBLE";
        toolGroups[] = {"METALWORK_TOOL"};
        baseDurationSec = 25.0;
        animationLength = 4.0;
        specialty = 0.03;
        toolDamage = 5;
    };

    //--------------------------------------------------------------------------
    // ### SLICE apiary ###   Schleudern.
    //
    // Auftrag: "[Schleudern] -> Entdeckelten Rahmen in die Honigschleuder
    // einsetzen" und "[Abfuellen] -> Leeres Glas an der Schleuder mit Honig
    // befuellen". EIN Vorgang, weil der Zwischenstand zwischen beiden eine
    // Fluessigkeit im Gefaess waere - die vollstaendige Begruendung steht an
    // ChefZ_HoneyExtractor.
    //
    // STATION_TIMED und nicht STATION_ACTION: der Spieler kurbelt AN
    // (JOB_STARTED), danach gehoert der Takt der Station. baseDurationSec ist
    // die Zeit JE GLAS (Auftrag 13: rund 90 Sekunden); die Fortsetzung Glas
    // fuer Glas steht im Skript der Schleuder, nicht hier. Ein Spieler, der
    // die ganze Charge lang kurbelt, waere der falsche Anker.
    //
    // KEINE toolGroups: an einer Station arbeitet die Station, nicht das
    // Werkzeug. Der Transform hat ausserdem ZWEI Eingaenge - an einer Station
    // ist das folgenlos (11 E1), als HANDCRAFT waere kein Platz mehr fuer ein
    // Werkzeug (01 V12).
    //
    // KEIN requiresHeat: Schleudern ist Mechanik, kein Feuer.
    //--------------------------------------------------------------------------
    class PROCESS_SPIN_HONEY
    {
        exec = "STATION_TIMED";
        displayName = "#STR_CHEFZ_PROC_SPIN_HONEY";
        baseDurationSec = 90.0;
        requiresHeat = 0;
    };
};

//------------------------------------------------------------------------------
// ### SLICE grain ### Werkzeuggruppe der Pastamaschine.
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
    // Die Gruppe HEISST weiter ROLLING_PIN, obwohl kein Nudelholz mehr darin
    // steht. Das ist kein Versehen und keine Bequemlichkeit: PROCESS_ROLL in
    // ChefZ_Baking/Config/GrainProcesses.json nennt genau diesen Namen. Ihn
    // hier umzubenennen hiesse, dort mitzuschreiben - und bis das geschehen
    // waere, zeigte PROCESS_ROLL auf eine Gruppe, die es nicht mehr gibt.
    // Ein Symbol ist ein Name in einem offenen Namensraum, kein Klassenname;
    // was die Gruppe bedeutet, sagen ihre Mitglieder.
    //
    // ZWEI Mitglieder, und beide sind der Punkt dieser Aenderung:
    //
    //   ChefZ_PastaMachine  das gemeinte Geraet. Es ist ueber
    //                       TR_AssemblePastaMachine herstellbar - der
    //                       Vorgaenger ChefZ_RollingPin war es nicht.
    //   MeatTenderizer      Vanilla, nominal 140, Town/Village
    //                       (ChefZ_Vanilla_Assets.md). Vanilla-Audit §4.2 D
    //                       verlangt ihn ausdruecklich: solange die Gruppe nur
    //                       eine Klasse enthaelt, die niemand bekommen kann,
    //                       ist PROCESS_ROLL unerreichbar und die Backkette
    //                       blockiert. Er kostet eine Zeile und macht die
    //                       Kette OHNE types.xml und ohne Craft erreichbar.
    //
    // Die Reihenfolge ist die aus dem Audit (§6 F, "Beide, gestaffelt"):
    // erst die Vanillaklasse, damit die Kette laeuft; ein eigener
    // Klopf-/Zartmachprozess fuer den Fleischklopfer kann spaeter eine eigene
    // Gruppe bekommen, ohne diese hier anzufassen.
    class ROLLING_PIN
    {
        toolCategories[] = {"ROLLING_TOOL"};
        classes[] = {"ChefZ_PastaMachine", "MeatTenderizer"};
        allowSubclasses = 1;
    };

    // ### SLICE grain ### Werkzeug fuer PROCESS_ASSEMBLE.
    //
    // Metall biegen, nieten, schrauben - was man braucht, um aus einem
    // MetalPlate eine Pastamaschine zu machen. Ausschliesslich Vanillaklassen:
    // ChefZ fasst keine fremde config.cpp an, sondern nennt fremde Klassen in
    // einer eigenen Gruppe (11 E8).
    //
    // Alle fuenf sind gewoehnliche Werkzeugloot - die Gruppe ist bewusst weit,
    // damit der Zusammenbau nicht an einem einzelnen seltenen Werkzeug haengt.
    // Genau daran ist die Backkette vorher gescheitert.
    class METALWORK_TOOL
    {
        classes[] =
        {
            "Pliers",
            "Hammer",
            "Wrench",
            "LugWrench",
            "Screwdriver"
        };
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
