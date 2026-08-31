// ChefZ_Meat - Fleisch, Hackfleisch, Wurst.
//
// Slice "meat", Production Map §27-§39. Die Kette beginnt beim FERTIGEN
// FLEISCHSTUECK: die Tierzerlegung selbst gehoert Vanilla bzw. Terje Hunting
// (§27, Uebergabepunkt). Dieses Modul enthaelt deshalb keinen Zerlegeschritt
// und keine Terje-Referenz, in keiner Form.
//
//   Keule    --Messer-->        2x Vanilla-Steak + Knochen  (Zerteilen)
//   Fleisch  --Fleischwolf-->   ChefZ_Minced*              §30
//   Hack + Gewuerz + Darm   --> ChefZ_Raw*Sausage          §34-§39
//
//   Seit dem 29.08.2026 ohne Zwischenstufen, die Vanilla schon hat: kein
//   Wuerfel mehr (die Eintoepfe nehmen gewolftes Fleisch; das rohe Steak
//   bleibt Vanilla-Kochen, Invariante I2), und Vanillas Guts / SmallGuts
//   sind die Wursthuelle (kein ChefZ_SausageCasing).
//   Raw*Sausage  --Pfanne/Topf--> ChefZ_*Sausage           §40 (Vanilla-Garstufe)
//
// Raeuchern und Trocknen (§41, §42) stehen NICHT hier: sie gehoeren dem Slice
// "preservation", der PROCESS_SMOKE und PROCESS_DRY mitbringt. Dieses Modul
// endet bei der rohen und der gebratenen Wurst.
//
// ---------------------------------------------------------------------------
// DIE KEULEN - und warum sie die Systemgrenze zu Terje NICHT verschieben
// ---------------------------------------------------------------------------
// ChefZ_BeefLeg / ChefZ_PorkLeg / ChefZ_VenisonLeg fallen beim Zerlegen an.
// Angebunden sind sie ueber Vanillas EIGENE Zerlegeausbeute und ueber sonst
// nichts:
//
//   ActionSkinning.OnFinishProgressServer -> SpawnItems(action_data)
//   ActionSkinning.c:236
//       string cfgAnimalClassPath = "cfgVehicles " + body.GetType() + " " + "Skinning ";
//   ActionSkinning.c:245-257
//       ConfigGetChildrenCount(...) ueber ALLE Kinder von "Skinning";
//       je Kind: ConfigGetText(... "item"), ConfigGetInt(... "count")
//
// Vanilla zaehlt also jedes Unterklassenkind von "Skinning" ab und spawnt, was
// dort unter "item" steht. Ein zusaetzliches Kind ist eine reine DATENZEILE in
// einer Vanilla-Tabelle - keine neue Aktion, kein modded class, kein Eingriff
// in den Zerlegevorgang.
//
// Terje-Analyse §14 sagt: das Zerlegen gehoert Terje Hunting, ChefZ beginnt beim
// fertigen Fleischstueck. Das bleibt so. Terje Hunting haengt sich seinerseits
// per "modded class ActionSkinning" AN dieselbe Vanilla-Ausbeute an: es ruft
// super.OnFinishProgressServer() - also Vanillas SpawnItems() mit genau dieser
// Configtabelle - und rechnet danach in TerjeProcessServerSpawnedItems() die
// Perks auf die entstandenen Items. Beide Seiten lesen bzw. beschreiben damit
// unterschiedliche Dinge: ChefZ sagt WAS es gibt, Terje sagt WIE VIEL davon und
// WIE SCHNELL. Es gibt keine gemeinsame Datei, keine Ladereihenfolgefrage und
// keine Terje-Referenz in diesem Modul.
//
// Was das Modul dadurch NICHT tut: es aendert keine vorhandene Ausbeute. Kein
// "count" eines Vanilla-Eintrags wird angefasst, keine Steakzahl gesenkt. Ein
// zusaetzlicher Eintrag ist zusaetzliches Fleisch - das ist eine bewusste
// Balanceentscheidung und steht im Slice-Bericht.
//
// PFADWURZEL: das PBO-Praefix ist der ORDNERNAME des Addons. Jeder Laufzeitpfad
// beginnt deshalb mit "ChefZ_Meat/" (Entwurf 02 §4.1).
//
// ---------------------------------------------------------------------------
// Zwei Pflichtbloecke an JEDER essbaren Klasse - und warum
// ---------------------------------------------------------------------------
// class Nutrition   PlayerStomach.InitData registriert nur Klassen, die
//                   "Nutrition" ODER "Food" haben und scope != 0 sind
//                   (01 V7, PlayerStomach.c:208-250). Fehlt beides, wird der
//                   Bissen gegessen, verschwindet - und saettigt lautlos
//                   nichts. Es gibt dafuer keine Fehlermeldung.
//
// class Food        FoodStage.GetNextFoodStageType faellt ohne passenden
//   FoodStage-      Uebergang auf FoodStageType.BURNED zurueck
//   Transitions     (FoodStage.c:472). Eine kochbare Klasse OHNE Uebergaenge
//                   verbrennt beim ersten Garstufenwechsel (01 V4).
//
// Die Zahlen in nutrition_properties[] stehen in dieser Reihenfolge, woertlich
// aus FoodStage.c:
//
//   { fullnessIndex, energy, water, nutritionalIndex, toxicity, agents, digestibility }
//     GetFullnessIndex(0) GetEnergy(1) GetWater(2) GetNutritionalIndex(3)
//     GetToxicity(4)      GetAgents(5) GetDigestibility(6)
//
// Sie und nicht "class Nutrition" bestimmen den Bissen, sobald eine Klasse
// FoodStages hat: PlayerStomach ruft Edible_Base.GetNutritionalProfile mit
// item = null, und der Zweig "classname + food_stage" liest die
// nutrition_properties der jeweiligen Garstufe (13 §2). "class Nutrition"
// bleibt trotzdem an jeder Klasse - es ist die Eintrittskarte in
// PlayerStomach.InitData und der Wert fuer den Fall ohne Garstufe.
//
// agents: eAgents.SALMONELLA = 4, eAgents.FOOD_POISON = 16 (EAgents.c).
// Rohes Fleisch traegt 4, Verdorbenes 16. Gegartes traegt keine.
//
// ---------------------------------------------------------------------------
// fullnessIndex - DIE HERLEITUNG (Rescale vom 31.08.2026)
// ---------------------------------------------------------------------------
// Die Engine rechnet Magenvolumen OHNE jede Division. Woertlich:
//
//     PlayerStomach.c:86
//         volume = m_Profile.GetFullnessIndex() * m_Amount;
//
// m_Amount ist die noch unverdaute MENGE des gegessenen Stuecks in
// Quantity-Einheiten, nicht "ein Item". Danebengelegt der Grenzwert:
//
//     PlayerConstants.c:208
//         const int VOMIT_THRESHOLD = 2000;
//     PlayerConstants.c:200
//         static const int BT_STOMACH_VOLUME_LVL3 = 1000;   // "Stuffed"
//
// PlayerStomach.c:304-317 setzt m_StomachVolume je Verdauungsdurchlauf auf
// Null und summiert die Volumen ALLER Magenposten neu. Wer ueber 2000 kommt,
// erbricht - und zwar alles.
//
// Der Unterschied zur zweiten Zeile daneben ist der ganze Punkt:
//
//     PlayerStomach.c:92
//         float energy_per_unit = profile.GetEnergy() / 100;
//
// ENERGIE und WASSER werden durch 100 geteilt, das Volumen NICHT. Wer die
// beiden Felder in derselben Einheit denkt, verrechnet sich um Faktor 100.
// Genau daran krankte dieses Modul: fullnessIndex lag bei 95 bis 240, in einer
// Groessenordnung, die zu energy gepasst haette, aber nicht zum Volumen.
//
// DIE INVARIANTE DIESES MODULS
//
//     fullnessIndex * varQuantityMax = Magenvolumen des GANZEN Items
//     varQuantityMax = 250   (ChefZ_MeatItemBase, dort steht die Begruendung)
//
// Also: fullnessIndex = Zielvolumen / 250. Die Rechnung steht an jeder Klasse.
//
// 250 ist die gemeinsame Mengenskala mit dem Slice "preservation"
// (Harmonisierung vom 31.08.2026) und macht zugleich die Energiezahl wieder
// wahr - mit varQuantityMax = 1 lieferte eine Wurst mit energy = 470 ueber
// PlayerStomach.c:92 nur 4,7 Energie.
//
// Die Zielbaender, an einem gemessenen Fremdwert geeicht - DayZ Expansion
// (Objects/Gear/Consumables/config.cpp:73 und :166) gibt seinem Baguette
// varQuantityMax = 125 bei fullnessIndex 2.5, also 312 Volumen fuer ein
// Brot ueber fuenf Inventarfelder:
//
//     kleiner Happen (Hack, Wuerfel)          100 - 300   -> Index 0,40 - 1,20
//     ganzes Fleischprodukt (Wurst)           300 - 600   -> Index 1,20 - 2,40
//     schweres Stueck (Keule)                 bis 700     -> Index bis 2,80
//
// Damit sind rund drei Wuerste oder zwei Keulen ein voller Magen (1000) und
// vier bis fuenf Wuerste die Kotzgrenze (2000) - eine Zahl, die man im Spiel
// erreichen KANN, aber nicht bei jeder Mahlzeit erreicht.
//
// Alle Werte der ESSBAREN Garstufen liegen damit im Vanilla-Band 0,75 - 2,80.
// Burned und Rotten fallen bewusst darunter (0,40 - 0,92): von einem
// verkohlten Rest bleibt weniger im Magen als vom ganzen Stueck.
//
// WER DIE MENGE AENDERT, AENDERT DIESE ZAHLEN MIT. varQuantityMax und
// fullnessIndex sind zwei Faktoren desselben Produkts. Eine Umstellung von 250
// auf einen anderen Wert ohne Anpassung hier verschiebt jedes Volumen im
// selben Verhaeltnis - bei 250 -> 1000 waeren es 2800 fuer eine Keule und
// damit Erbrechen beim ersten Bissen.
//
// Die relative Ordnung der alten Werte ist ueberall erhalten geblieben, auch
// zwischen den Garstufen einer Klasse.
//
// WARUM BEIDE STELLEN GEAENDERT WURDEN - Nutrition UND jede Garstufe
//
// Solange eine Klasse einen FoodStage hat, ist der "class Nutrition"-Block
// fuer das Volumen TOT:
//
//     Edible_Base.c:391-397   (GetFoodTotalVolume)
//         Edible_Base food_item = Edible_Base.Cast(item);
//         if (food_item && food_item.GetFoodStage())
//              return FoodStage.GetFullnessIndex(food_item.GetFoodStage());
//         ...
//         return g_Game.ConfigGetFloat(class_path + " fullnessIndex");
//
//     FoodStage.c:314-317
//         static float GetFullnessIndex(FoodStage stage, ...)
//             return GetNutritionPropertyFromIndex( 0 , ... );
//
// Index 0 ist nutrition_properties[0] der jeweiligen Stufe
// (FoodStage.c:267-276 gibt den Arrayeintrag unveraendert zurueck; der
// Modifikatorenzweig darunter greift nur, wenn das Array LEER ist - hier
// steht ueberall eines). Jede Klasse dieses Moduls hat Garstufen. Wer nur
// den Nutrition-Block umstellt, aendert im Spiel gar nichts.
//
// Der Nutrition-Block bleibt trotzdem gepflegt: er ist die Eintrittskarte in
// PlayerStomach.InitData (01 V7) und traegt denselben Wert wie die Raw-Stufe.
//
// ---------------------------------------------------------------------------
// MODELLE
// ---------------------------------------------------------------------------
// Es gibt noch keine eigene Geometrie. Jede Klasse traegt ein Vanilla-Proxy;
// der Bedarf steht im Bericht des Slice und im Asset-Backlog. Kein Item
// wartet auf ein Modell.

class CfgPatches
{
    class ChefZ_Meat
    {
        units[] =
        {
            // Die Skript- und Configbasis aller Fleischwaren dieses Moduls.
            "ChefZ_MeatItemBase",
            "ChefZ_BeefLeg",
            "ChefZ_PorkLeg",
            "ChefZ_VenisonLeg",
            "ChefZ_DicedMeat",
            "ChefZ_MincedMeat",
            "ChefZ_MincedPork",
            "ChefZ_MincedVenison",
            "ChefZ_MincedBoar",
            "ChefZ_MincedChicken",
            "ChefZ_MincedBear",
            "ChefZ_RawSausage",
            "ChefZ_RawPorkSausage",
            "ChefZ_RawVenisonSausage",
            "ChefZ_RawBoarSausage",
            "ChefZ_RawHunterSausage",
            "ChefZ_RawSpicySausage",
            "ChefZ_CookedSausage",
            "ChefZ_PorkSausage",
            "ChefZ_VenisonSausage",
            "ChefZ_BoarSausage",
            "ChefZ_HunterSausage",
            "ChefZ_SpicySausage"
        };
        weapons[] = {};
        requiredVersion = 0.1;
        // Jeder Eintrag steht fuer etwas, das dieses Modul TATSAECHLICH nutzt:
        //   ChefZ_Core       ChefZ_Edible_Base (Skriptbasis) und der Config Manager
        //   ChefZ_Processing PROCESS_CUT_MEAT, PROCESS_GRIND_MEAT,
        //                    PROCESS_STUFF_SAUSAGE, die
        //                    Werkzeuggruppe CUTTING_TOOL und die beiden Stationen
        //   DZ_Gear_Food     die Proxy-Modelle und die Basisklasse Edible_Base
        //   DZ_Data          Grundlage von allem
        // Nicht mehr und nicht weniger - eine zu breite Liste verschiebt die
        // Ladereihenfolge fremder Mods ohne Grund.
        //
        // Die sieben DZ_Animals_*-Eintraege kamen mit den Keulen dazu und sind
        // genau die PBOs, in denen die unten erweiterten Tierklassen stehen.
        // Ohne sie waere die Ladereihenfolge undefiniert: die Erweiterung
        // "class Animal_BosTaurus: AnimalBase { class Skinning { ... }; };"
        // braucht die Originalklasse VOR sich, sonst legt sie eine neue Klasse
        // gleichen Namens an - und die Kuh im Spiel bleibt die alte, ohne dass
        // irgendwo etwas gemeldet wuerde. Die Namen sind die, die auch
        // TerjeSkills_Animals und DayZExpansion_VanillaFixes_Animals fuehren.
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Gear_Food",
            "ChefZ_Core",
            "ChefZ_Items",
            "ChefZ_Food",
            "ChefZ_Processing",
            "DZ_Animals_bos_taurus",
            "DZ_Animals_bos_taurus_fem",
            "DZ_Animals_sus_domesticus",
            "DZ_Animals_cervus_elaphus",
            "DZ_Animals_cervus_elaphus_feminam",
            "DZ_Animals_capreolus_capreolus",
            "DZ_Animals_capreolus_capreolus_fem"
        };
    };
};

// ---------------------------------------------------------------------------
// Skriptmodul dieses PBO.
//
// Es braucht einen eigenen CfgMods-Knoten, weil der Knoten des Core
// ausschliesslich Pfade unterhalb von "ChefZ_Core/" nennt - und das PBO-Praefix
// die Wurzel JEDES Laufzeitpfades ist (02 §4.1). Ohne diesen Block laedt DayZ
// "ChefZ_Meat/Scripts/..." still nicht: kein Fehler, kein RPT-Eintrag, nur eine
// Klasse, die es zur Laufzeit nicht gibt.
//
// Es ist bewusst NUR das worldScriptModule: dieses Modul bringt genau eine
// Skriptklasse mit, und die ist eine 4_World-Ableitung.
// ---------------------------------------------------------------------------
class CfgMods
{
    // Der Knoten heisst ChefZ_MeatMod und nicht ChefZ_Meat, obwohl er dasselbe
    // Modul meint. Grund: der CfgChefZ-Knoten weiter unten MUSS ChefZ_Meat
    // heissen - er ist die Slice-Identitaet, unter der der Core das Modul
    // meldet -, und zwei gleichnamige Klassen in derselben config.cpp sind
    // eine doppelte Definition. Fuer die Engine ist der Name hier belanglos;
    // was zaehlt, ist "dir".
    class ChefZ_MeatMod
    {
        dir = "ChefZ_Meat";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Meat";
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
                    "ChefZ_Meat/Scripts/4_World"
                };
            };
        };
    };
};

class CfgVehicles
{
    class Edible_Base;

    // Vorwaertsdeklaration der Vanilla-Tierbasis. Sie definiert nichts - sie
    // macht den Namen sichtbar, damit die Erweiterungen am Ende dieser Datei
    // ihre Elternklasse nennen koennen, ohne sie neu zu erfinden.
    class AnimalBase;

    // ------------------------------------------------------------------------
    // Gemeinsame Configbasis dieses Moduls.
    //
    // scope = 0: sie ist kein Item, sie ist die Stelle, an der Garstufen-
    // Uebergaenge und Grundeigenschaften EINMAL stehen. Der Core bringt keine
    // solche Basis mit (Invariante I3, Kopf von ChefZ_Edible_Base.c:
    // "Wer eine gemeinsame Configbasis mit scope = 0 haben will, legt sie in
    // SEINEM Modul an").
    //
    // Configbasis ist eine VANILLA-Klasse, Skriptbasis ist ChefZ_Edible_Base -
    // genau die Andockregel aus demselben Dateikopf.
    // ------------------------------------------------------------------------
    class ChefZ_MeatItemBase : Edible_Base
    {
        scope = 0;
        model = "\dz\gear\food\steak.p3d";
        rotationFlags = 17;
        itemSize[] = {2, 1};
        weight = 250;
        absorbency = 0.7;
        // MENGENSKALA 250 - gramm-artig, wie Vanilla-Fleisch (Harmonisierung mit
        // dem Slice "preservation", 31.08.2026). Zwei Gruende, beide in der
        // Engine nachlesbar:
        //
        //   ENERGIE   PlayerStomach.c:92  energy_per_unit = GetEnergy() / 100
        //             Mit varQuantityMax = 1 kam von energy = 470 genau 4,7 im
        //             Magen an. Erst eine Menge in der Groessenordnung 100+
        //             macht die Energiezahl zu dem, was sie behauptet.
        //   VOLUMEN   PlayerStomach.c:86  volume = GetFullnessIndex() * m_Amount
        //             Erst mit einer Menge > 1 darf der fullnessIndex in das
        //             Vanilla-Band 0,75-2,8 zurueck (siehe Kopf).
        //
        // 250 / UAQuantityConsumed.EAT_NORMAL 15 sind rund 17 Bissen an
        // ActionEatMeat, 250 / EAT_BIG 25 genau zehn. Ein Stueck Fleisch ist
        // damit nichts, was man im Vorbeigehen herunterschlingt.
        //
        // Fuer die Rezeptdaten ist die Zahl TRANSPARENT, und das ist der Grund,
        // warum sie ohne Anfassen fremder Slices geaendert werden darf:
        //
        //   ChefZ_FactCollector.c:439
        //       units = quantity / quantityMax * unitsPerWholeItem
        //   ChefZ_SlotEvaluator.c:364
        //       float perUnit = facts.quantity / facts.units;
        //
        // Einheiten sind ein VERHAELTNIS. Ein volles Item bleibt bei
        // unitsPerWholeItem = 1 genau eine Einheit, egal ob quantityMax 1 oder
        // 250 heisst. Fremde Rezepte, die Fleisch ueber "consumeAmount": 1
        // gegen die Kategorie MINCED_MEAT verbrauchen, rechnen unveraendert
        // richtig.
        //
        // NICHT transparent sind ROHE Mengen: ChefZ_ProcessRunner.c:335-343 und
        // ChefZ_Applicator.c:975-976 schreiben "quantity" per SetQuantity
        // direkt. Die Ausgaenge in Config/Processing/Meat.json und
        // Config/Recipes/Sausage.json tragen deshalb 250, nicht 1.
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
                // Alle Proxys sind einteilige Modelle ohne verstecktes
                // Selection-Set - deshalb ueberall 0. Sobald es eigene
                // Geometrie gibt, wird genau hier umgeschaltet.
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
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {100, 60, 200};
                };
                class Boiled
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {100, 80, 150};
                };
                class Burned
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {200, 20, 0};
                };
                class Rotten
                {
                    visual_properties[] = {0, 0, 0};
                    cooking_properties[] = {0, 0, 0};
                };
            };

            // OHNE DIESEN BLOCK VERBRENNT JEDES ITEM DES MODULS (01 V4).
            //
            // Es steht bewusst nur der Uebergang AUS "Raw" hier: aus "Baked"
            // gibt es keinen, und genau deshalb liefert
            // GetNextFoodStageType dort FoodStageType.BURNED - eine Wurst,
            // die im Feuer liegen bleibt, verbrennt. Das ist gewollt und
            // kostet keine Zeile.
            //
            // transition_to und cooking_method sind ZAHLEN, nicht Namen:
            // SetupFoodStageTransitionMapping liest sie mit ConfigGetInt
            // (FoodStage.c:167ff).
            //   FoodStageType:     RAW 1, BAKED 2, BOILED 3, DRIED 4, BURNED 5, ROTTEN 6
            //   CookingMethodType: NONE 0, BAKING 1, BOILING 2, DRYING 3, TIME 4
            //
            // DRYING fehlt absichtlich: Trocknen und Raeuchern laufen in ChefZ
            // an eigenen Stationen (11 E6) und nicht in Vanillas Smoking-Slots.
            // Der Slice "preservation" bringt sie mit.
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
    // DIE KEULEN
    //
    // Ein Grobteilstueck mit Knochen, wie es beim Zerlegen anfaellt. Es ist
    // AUSDRUECKLICH kein zweites Steak: es traegt in Config/Ingredients/Meat.json
    // KEINE Kategorie, sondern nur Tags. Der Grund steht dort und ist die
    // wichtigste Entscheidung an diesen drei Klassen - kurz: Kategorien sind in
    // ChefZ self-or-ancestor (ChefZ_CategoryClosure), und eine Keule in "MEAT"
    // waere fuer TR_MeatToMinced EIN Fleischstueck. Eine ganze
    // Rinderkeule ergaebe dann ein einziges Hackfleisch.
    //
    // Adressiert wird die Keule deshalb ausschliesslich ueber ihren
    // Klassennamen, und zwar von genau einem Transform je Sorte
    // (TR_CutBeefLeg / TR_CutPorkLeg / TR_CutVenisonLeg). Der loest sie in zwei
    // VANILLA-Steaks plus Knochen bzw. Fett auf, und ab da laeuft die
    // vorhandene Kette weiter - Wuerfeln, Wolfen, Wurst. Die Keule braucht
    // damit keinen einzigen neuen Rezeptslot und ist trotzdem nirgends eine
    // Sackgasse.
    //
    // Kochbar ist sie: sie erbt Food > FoodStages UND FoodStageTransitions von
    // ChefZ_MeatItemBase, also brennt sie nicht an (01 V4), und die Essaktion
    // steht auf der Skriptbasis (ChefZ_MeatItemBase.SetActions). Ein ganzer
    // Braten am Feuer ist die zweite, teurere Verwendung: viel Energie auf
    // einmal, dafuer schleppt man vier Inventarfelder mit sich herum.
    //
    // Die Zahlen sind NICHT frei gewaehlt. Sie sind das Doppelte der
    // entsprechenden Hack-/Wuerfelklasse dieses Moduls - zwei Steaks - abzueglich
    // eines Abschlags fuer den Knochen, der zwar mitgewogen, aber nicht
    // mitgegessen wird:
    //
    //   ChefZ_BeefLeg     <- 2 x Steak (Raw 140)   (Raw 140 -> 265 statt 280)
    //   ChefZ_PorkLeg     <- 2 x ChefZ_MincedPork   (Raw 185 -> 350 statt 370)
    //   ChefZ_VenisonLeg  <- 2 x ChefZ_MincedVenison(Raw 145 -> 275 statt 290)
    //
    // itemSize 2x2: vier Felder fuer zwei Steaks (je 2x1) plus Knochen. Die
    // Keule ist damit im Rucksack minimal guenstiger als ihr zerlegter Inhalt -
    // genau das ist ihr Sinn als Transportform.
    //
    // MODELL: alle drei tragen das Steak-Proxy. Es gibt kein Keulenmodell in
    // Vanilla, das im Projekt belegt waere; geraten wird nicht. Der Bedarf steht
    // im Slice-Bericht.
    // ------------------------------------------------------------------------

    // Rind, aus Animal_BosTaurus* (Vanilla-Assets §20d).
    class ChefZ_BeefLeg : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BEEFLEG0";
        descriptionShort = "#STR_CHEFZ_ITEM_BEEFLEG1";
        model = "\ChefZ\ChefZ_Food\models\leg_beef.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 2};
        weight = 700;

        // VOLUMEN: 670 (schweres Stueck, knapp unter der Keulenobergrenze 700).
        // 670 / varQuantityMax 250 = 2.68. Vorher 230 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 2.68;
            energy = 265;
            water = 85;
            nutritionalIndex = 26;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {2.68, 265, 85, 26, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {2.44, 570, 46, 44, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {2.5, 535, 112, 44, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {1.4, 150, 18, 10, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {2.1, 175, 55, 10, 20, 16, 1}; };
            };
        };
    };

    // Schwein, aus Animal_SusDomesticus. Fetter als Rind - dieselbe Stufung wie
    // ChefZ_MincedPork gegenueber ChefZ_MincedMeat.
    class ChefZ_PorkLeg : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_PORKLEG0";
        descriptionShort = "#STR_CHEFZ_ITEM_PORKLEG1";
        model = "\ChefZ\ChefZ_Food\models\leg_pork.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 2};
        weight = 700;

        // VOLUMEN: 700 (das schwerste Stueck des Moduls, Obergrenze der Keulen).
        // 700 / varQuantityMax 250 = 2.8. Vorher 240 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 2.8;
            energy = 350;
            water = 72;
            nutritionalIndex = 28;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {2.8, 350, 72, 28, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {2.56, 685, 34, 46, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {2.62, 640, 98, 46, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {1.46, 170, 18, 10, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {2.22, 210, 46, 10, 20, 16, 1}; };
            };
        };
    };

    // Wild, aus Animal_CervusElaphus* (Rotwild) und Animal_CapreolusCapreolus*
    // (Rehwild). Beide liefern in Vanilla DeerSteakMeat, deshalb EINE Keule fuer
    // beide - genau so, wie TR_VenisonToMinced es schon haelt. Mager, dafuer
    // hoher Naehrwertindex.
    class ChefZ_VenisonLeg : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_VENISONLEG0";
        descriptionShort = "#STR_CHEFZ_ITEM_VENISONLEG1";
        model = "\ChefZ\ChefZ_Food\models\leg_venison.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 2};
        weight = 620;

        // VOLUMEN: 600 (magerste und leichteste der drei Keulen, 620 g).
        // 600 / varQuantityMax 250 = 2.4. Vorher 205 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 2.4;
            energy = 275;
            water = 84;
            nutritionalIndex = 38;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {2.4, 275, 84, 38, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {2.2, 570, 42, 58, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {2.28, 535, 110, 58, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {1.22, 148, 18, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {1.94, 178, 52, 12, 20, 16, 1}; };
            };
        };
    };

    // §29: Raw Meat + Messer -> Wuerfel. Am 29.08.2026 gestrichen ("ein rohes
    // Steak mit anderem Namen") und am selben Tag wieder aufgenommen, als das
    // eigene Modell kam: beefcubes.p3d aus der Lieferung 043ad52. Kategorie
    // MINCED_MEAT (Zutatendatensatz), damit die Eintoepfe es neben dem Hack
    // nehmen - der I2-Anker der Eintoepfe bleibt verarbeitetes Fleisch.
    class ChefZ_DicedMeat : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_DICEDMEAT0";
        descriptionShort = "#STR_CHEFZ_ITEM_DICEDMEAT1";
        model = "\ChefZ\ChefZ_Food\models\dicedmeat.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 1};
        weight = 260;

        // VOLUMEN: 335 (kleine Portion, 260 g - knapp ueber dem Hack, weil der
        // Wuerfel das groebere Stueck ist).
        // 335 / varQuantityMax 250 = 1.34. Vorher 120 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.34;
            energy = 140;
            water = 45;
            nutritionalIndex = 15;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.34, 140, 45, 15, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.22, 300, 25, 25, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.28, 280, 60, 25, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.9, 80, 10, 5, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {1.12, 90, 30, 5, 20, 16, 1}; };
            };
        };
    };

    // §30: das gattungsneutrale Hack. Entsteht, wenn keine Sorte greift.
    class ChefZ_MincedMeat : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MINCEDMEAT0";
        descriptionShort = "#STR_CHEFZ_ITEM_MINCEDMEAT1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 250;

        // VOLUMEN: 305 (kleine Portion, 250 g - der Bezugswert aller Hacksorten).
        // 305 / varQuantityMax 250 = 1.22. Vorher 110 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.22;
            energy = 150;
            water = 40;
            nutritionalIndex = 15;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.22, 150, 40, 15, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.1, 310, 20, 25, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.16, 290, 55, 25, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.78, 80, 10, 5, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {1.06, 95, 25, 5, 20, 16, 1}; };
            };
        };
    };

    // §30: Schwein - fetter, deshalb mehr Energie.
    class ChefZ_MincedPork : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MINCEDPORK0";
        descriptionShort = "#STR_CHEFZ_ITEM_MINCEDPORK1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 250;

        // VOLUMEN: 320 (kleine Portion, 250 g - fetter als das neutrale Hack).
        // 320 / varQuantityMax 250 = 1.28. Vorher 115 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.28;
            energy = 185;
            water = 38;
            nutritionalIndex = 16;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.28, 185, 38, 16, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.16, 360, 18, 26, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.22, 335, 52, 26, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.8, 90, 10, 5, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {1.1, 110, 24, 5, 20, 16, 1}; };
            };
        };
    };

    // §30/§36: Wild - mager, dafuer hoher Naehrwertindex.
    class ChefZ_MincedVenison : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MINCEDVENISON0";
        descriptionShort = "#STR_CHEFZ_ITEM_MINCEDVENISON1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 245;

        // VOLUMEN: 300 (kleine Portion, 245 g - das magerste der Hacksorten).
        // 300 / varQuantityMax 250 = 1.2. Vorher 108 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.2;
            energy = 145;
            water = 44;
            nutritionalIndex = 22;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.2, 145, 44, 22, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.08, 300, 22, 34, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.14, 280, 58, 34, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.78, 78, 10, 6, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {1.04, 92, 27, 6, 20, 16, 1}; };
            };
        };
    };

    // §30/§37: Wildschwein - zwischen Schwein und Wild.
    class ChefZ_MincedBoar : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MINCEDBOAR0";
        descriptionShort = "#STR_CHEFZ_ITEM_MINCEDBOAR1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 250;

        // VOLUMEN: 310 (kleine Portion, 250 g - zwischen Wild und Schwein).
        // 310 / varQuantityMax 250 = 1.24. Vorher 112 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.24;
            energy = 165;
            water = 40;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.24, 165, 40, 20, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.12, 325, 20, 30, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.18, 305, 55, 30, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.78, 84, 10, 6, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {1.06, 100, 25, 6, 20, 16, 1}; };
            };
        };
    };

    // §30: Gefluegel - leicht, wenig Energie.
    class ChefZ_MincedChicken : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MINCEDCHICKEN0";
        descriptionShort = "#STR_CHEFZ_ITEM_MINCEDCHICKEN1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 220;

        // VOLUMEN: 265 (kleinster Happen des Moduls, 220 g Gefluegelhack).
        // 265 / varQuantityMax 250 = 1.06. Vorher 95 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.06;
            energy = 120;
            water = 46;
            nutritionalIndex = 18;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.06, 120, 46, 18, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {0.94, 250, 24, 28, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.0, 235, 60, 28, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.66, 65, 10, 5, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.92, 76, 28, 5, 25, 16, 1}; };
            };
        };
    };

    // §30: Raubtierfleisch - schwer und energiereich.
    class ChefZ_MincedBear : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_MINCEDBEAR0";
        descriptionShort = "#STR_CHEFZ_ITEM_MINCEDBEAR1";
        model = "\dz\gear\food\steak.p3d";
        itemSize[] = {2, 1};
        weight = 280;

        // VOLUMEN: 360 (schwerstes Hack, 280 g - Obergrenze der kleinen Portionen).
        // 360 / varQuantityMax 250 = 1.44. Vorher 130 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.44;
            energy = 205;
            water = 36;
            nutritionalIndex = 20;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.44, 205, 36, 20, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.32, 400, 16, 30, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.38, 370, 50, 30, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.88, 100, 10, 6, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {1.22, 122, 22, 6, 20, 16, 1}; };
            };
        };
    };

    // §31: Kochfett. KEINE eigene Klasse - Vanillas "Lard" ist die
    // Fettklasse des Mods (Vanilla-Audit §2). Das fruehere ChefZ_AnimalFat
    // trug bereits dz/gear/food/lard.p3d, hatte dieselbe einzige Kategorie FAT
    // und stand in keinem Rezept-Slot: alle sechs Fett-Slots matchen ueber die
    // Kategorie, nie ueber die Klasse. Es faellt jetzt Lard als Beiprodukt des
    // Wolfens an (Config/Processing/Meat.json). Nicht wieder anlegen: soll sich
    // Wolfenfett vom Schlachtfett unterscheiden, gehoert das zuerst als
    // Kategorie oder Tag in die Registry, nicht als zweite Klasse.

    // §34: die Basiswurst - Hack, Salz, Huelle.
    class ChefZ_RawSausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_RAWSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_RAWSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_raw.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 1};
        weight = 320;

        // VOLUMEN: 435 (ganzes Fleischprodukt, 320 g - der Bezugswert aller
        // Rohwuerste). 435 / varQuantityMax 250 = 1.74. Vorher 150 - Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.74;
            energy = 230;
            water = 35;
            nutritionalIndex = 18;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.74, 230, 35, 18, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.62, 470, 18, 30, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.62, 470, 48, 30, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.7, 92, 14, 7, 0, 4, 1}; };
                class Rotten { nutrition_properties[] = {0.88, 115, 18, 9, 20, 16, 1}; };
            };
        };
    };

    // §35: Minced Pork + Salz + Pfeffer + Huelle.
    class ChefZ_RawPorkSausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_RAWPORKSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_RAWPORKSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_raw_pork.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 1};
        weight = 320;

        // VOLUMEN: 450 (ganzes Fleischprodukt, 320 g - fetter als die Basiswurst).
        // 450 / varQuantityMax 250 = 1.8. Vorher 155 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.8;
            energy = 265;
            water = 33;
            nutritionalIndex = 19;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.8, 265, 33, 19, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.68, 530, 16, 31, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.68, 530, 46, 31, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.72, 106, 13, 8, 0, 4, 1}; };
                class Rotten { nutrition_properties[] = {0.9, 133, 17, 10, 20, 16, 1}; };
            };
        };
    };

    // §36: Minced Venison + Salz + Thymian + Pfeffer + Huelle.
    class ChefZ_RawVenisonSausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_RAWVENISONSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_RAWVENISONSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_raw_venison.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 1};
        weight = 320;

        // VOLUMEN: 430 (ganzes Fleischprodukt, 320 g - magerste Rohwurst).
        // 430 / varQuantityMax 250 = 1.72. Vorher 148 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.72;
            energy = 235;
            water = 36;
            nutritionalIndex = 26;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.72, 235, 36, 26, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.6, 480, 18, 40, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.6, 480, 48, 40, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.68, 94, 14, 10, 0, 4, 1}; };
                class Rotten { nutrition_properties[] = {0.86, 118, 18, 13, 20, 16, 1}; };
            };
        };
    };

    // §37: Minced Boar + Salz + Pfeffer + Baerlauch + Huelle.
    class ChefZ_RawBoarSausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_RAWBOARSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_RAWBOARSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_raw_boar.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 1};
        weight = 320;

        // VOLUMEN: 440 (ganzes Fleischprodukt, 320 g - zwischen Wild und Schwein).
        // 440 / varQuantityMax 250 = 1.76. Vorher 152 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.76;
            energy = 250;
            water = 34;
            nutritionalIndex = 23;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.76, 250, 34, 23, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.64, 505, 17, 36, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.64, 505, 47, 36, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.7, 100, 14, 9, 0, 4, 1}; };
                class Rotten { nutrition_properties[] = {0.88, 125, 17, 12, 20, 16, 1}; };
            };
        };
    };

    // §38: Wildfleisch + Salz + Hunter Seasoning + Huelle.
    class ChefZ_RawHunterSausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_RAWHUNTERSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_RAWHUNTERSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_raw_hunter.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 1};
        weight = 320;

        // VOLUMEN: 460 (fuelligste Rohwurst des Moduls, 320 g mit voller Wuerzung).
        // 460 / varQuantityMax 250 = 1.84. Vorher 158 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.84;
            energy = 260;
            water = 33;
            nutritionalIndex = 30;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.84, 260, 33, 30, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.72, 525, 16, 46, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.72, 525, 46, 46, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.74, 104, 13, 12, 0, 4, 1}; };
                class Rotten { nutrition_properties[] = {0.92, 130, 17, 15, 20, 16, 1}; };
            };
        };
    };

    // §39: Hack + Salz + Pfeffer + Paprikapulver + Huelle.
    class ChefZ_RawSpicySausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_RAWSPICYSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_RAWSPICYSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_raw_spicy.p3d";   // EIGENES MODELL (30.08.2026, Lieferung c09900f)
        itemSize[] = {2, 1};
        weight = 320;

        // VOLUMEN: 435 (ganzes Fleischprodukt, 320 g - wie die Basiswurst; die
        // Schaerfe aendert die Menge nicht).
        // 435 / varQuantityMax 250 = 1.74. Vorher 150 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.74;
            energy = 245;
            water = 34;
            nutritionalIndex = 22;
            toxicity = 0;
            agents = 4;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.74, 245, 34, 22, 0, 4, 1}; };
                class Baked { nutrition_properties[] = {1.62, 495, 17, 34, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.62, 495, 47, 34, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.7, 98, 14, 9, 0, 4, 1}; };
                class Rotten { nutrition_properties[] = {0.88, 123, 17, 11, 20, 16, 1}; };
            };
        };
    };

    // §40: gebratene Basiswurst.
    class ChefZ_CookedSausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_COOKEDSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_COOKEDSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_cooked.p3d";   // EIGENES MODELL (30.08.2026) - die eine gebratene Wurst der Lieferung, geteilt von allen sechs Sorten
        itemSize[] = {2, 1};
        weight = 300;

        // VOLUMEN: 405 (gebraten, 300 g - unter der Rohwurst, weil beim Braten
        // Wasser austritt). 405 / varQuantityMax 250 = 1.62. Vorher 140 - Kopf.
        class Nutrition
        {
            fullnessIndex = 1.62;
            energy = 470;
            water = 18;
            nutritionalIndex = 30;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.62, 470, 18, 30, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {1.62, 470, 18, 30, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.62, 470, 48, 30, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.4, 118, 5, 8, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.64, 188, 7, 12, 20, 16, 1}; };
            };
        };
    };

    // §40, DME §53: gebratene Schweinswurst.
    class ChefZ_PorkSausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_PORKSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_PORKSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_cooked.p3d";   // EIGENES MODELL (30.08.2026) - die eine gebratene Wurst der Lieferung, geteilt von allen sechs Sorten
        itemSize[] = {2, 1};
        weight = 300;

        // VOLUMEN: 420 (gebraten, 300 g - fetteste der gebratenen Wuerste).
        // 420 / varQuantityMax 250 = 1.68. Vorher 145 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.68;
            energy = 530;
            water = 16;
            nutritionalIndex = 31;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.68, 530, 16, 31, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {1.68, 530, 16, 31, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.68, 530, 46, 31, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.42, 133, 4, 8, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.68, 212, 6, 12, 20, 16, 1}; };
            };
        };
    };

    // §40, DME §53: gebratene Wildwurst.
    class ChefZ_VenisonSausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_VENISONSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_VENISONSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_cooked.p3d";   // EIGENES MODELL (30.08.2026) - die eine gebratene Wurst der Lieferung, geteilt von allen sechs Sorten
        itemSize[] = {2, 1};
        weight = 300;

        // VOLUMEN: 400 (gebraten, 300 g - magerste der gebratenen Wuerste).
        // 400 / varQuantityMax 250 = 1.6. Vorher 138 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.6;
            energy = 480;
            water = 18;
            nutritionalIndex = 40;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.6, 480, 18, 40, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {1.6, 480, 18, 40, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.6, 480, 48, 40, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.4, 120, 5, 10, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.64, 192, 7, 16, 20, 16, 1}; };
            };
        };
    };

    // §40, DME §53: gebratene Wildschweinwurst.
    class ChefZ_BoarSausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_BOARSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_BOARSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_cooked.p3d";   // EIGENES MODELL (30.08.2026) - die eine gebratene Wurst der Lieferung, geteilt von allen sechs Sorten
        itemSize[] = {2, 1};
        weight = 300;

        // VOLUMEN: 410 (gebraten, 300 g - zwischen Wild und Schwein).
        // 410 / varQuantityMax 250 = 1.64. Vorher 142 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.64;
            energy = 505;
            water = 17;
            nutritionalIndex = 36;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.64, 505, 17, 36, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {1.64, 505, 17, 36, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.64, 505, 47, 36, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.42, 126, 4, 9, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.66, 202, 7, 14, 20, 16, 1}; };
            };
        };
    };

    // §40, DME §53: gebratene Jaegerwurst.
    class ChefZ_HunterSausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_HUNTERSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_HUNTERSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_cooked.p3d";   // EIGENES MODELL (30.08.2026) - die eine gebratene Wurst der Lieferung, geteilt von allen sechs Sorten
        itemSize[] = {2, 1};
        weight = 300;

        // VOLUMEN: 430 (fuelligste gebratene Wurst, 300 g mit voller Wuerzung).
        // 430 / varQuantityMax 250 = 1.72. Vorher 148 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.72;
            energy = 525;
            water = 16;
            nutritionalIndex = 46;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.72, 525, 16, 46, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {1.72, 525, 16, 46, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.72, 525, 46, 46, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.42, 131, 4, 12, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.68, 210, 6, 18, 20, 16, 1}; };
            };
        };
    };

    // §40, DME §53: gebratene Pfefferwurst.
    class ChefZ_SpicySausage : ChefZ_MeatItemBase
    {
        scope = 2;
        displayName = "#STR_CHEFZ_ITEM_SPICYSAUSAGE0";
        descriptionShort = "#STR_CHEFZ_ITEM_SPICYSAUSAGE1";
        model = "\ChefZ\ChefZ_Food\models\sausage_cooked.p3d";   // EIGENES MODELL (30.08.2026) - die eine gebratene Wurst der Lieferung, geteilt von allen sechs Sorten
        itemSize[] = {2, 1};
        weight = 300;

        // VOLUMEN: 405 (gebraten, 300 g - wie die gebratene Basiswurst).
        // 405 / varQuantityMax 250 = 1.62. Vorher 140 - siehe Herleitung im Kopf.
        class Nutrition
        {
            fullnessIndex = 1.62;
            energy = 495;
            water = 17;
            nutritionalIndex = 34;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        class Food
        {
            class FoodStages
            {
                class Raw { nutrition_properties[] = {1.62, 495, 17, 34, 0, 0, 1}; };
                class Baked { nutrition_properties[] = {1.62, 495, 17, 34, 0, 0, 1}; };
                class Boiled { nutrition_properties[] = {1.62, 495, 47, 34, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.4, 124, 4, 9, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.64, 198, 7, 14, 20, 16, 1}; };
            };
        };
    };

    // ========================================================================
    // ZERLEGEAUSBEUTE - je Tierart EIN zusaetzliches Kind in Vanillas
    // "Skinning"-Tabelle.
    //
    // Der Mechanismus steht woertlich im Dateikopf von ActionSkinning.c:
    //
    //     class Skinning
    //     {
    //         // All classes in this scope are parsed, so they can have any name.
    //         class ObtainedSteaks
    //         {
    //             item = "DeerSteakMeat";
    //             count = 10;
    //             transferToolDamageCoef = 1;
    //         };
    //     };
    //
    // "All classes in this scope are parsed" ist die ganze Anbindung: ein
    // zusaetzliches Kind wird mitgezaehlt, die vorhandenen bleiben unberuehrt.
    // Kein "count" eines Vanilla-Eintrags wird hier angefasst.
    //
    // WARUM NUR DIE BASISKLASSEN, NICHT DIE FARBVARIANTEN
    // ---------------------------------------------------
    // Vanilla hat Animal_BosTaurus_Brown/_Spotted/_White und weitere Varianten.
    // Sie stehen hier bewusst NICHT, und das ist keine Luecke: die Configkette
    // loest geerbte Knoten mit auf, und genau darauf baut auch TerjeSkills.
    // Dessen Animals/config.cpp nennt ebenfalls nur "Animal_BosTaurus" und
    // "Animal_BosTaurusF", waehrend sein Skript den Wert ueber
    // ConfigGetInt("CfgVehicles " + animalBody.GetType() + " ...") mit dem
    // KONKRETEN Typ (Animal_BosTaurus_Brown) liest. Dass das seit Jahren
    // funktioniert, ist der Beleg. Eine Variante hier namentlich zu nennen,
    // deren Elternklasse das Projekt nicht belegen kann, waere Raten.
    //
    // WARUM count = 1 UND NICHT itemZones/countByZone
    // -----------------------------------------------
    // ActionSkinning kann die Stueckzahl aus der Gesundheit einzelner
    // Schadenszonen ableiten (itemZones[]/countByZone[], ActionSkinning.c:249ff)
    // - "die Keule gibt es nur, wenn du ihr nicht in die Laeufe geschossen hast"
    // waere die schoenere Regel. Sie braucht aber den exakten Namen der
    // Schadenszone je Tierart, und den fuehrt keine Quelle dieses Projekts.
    // Ein falscher Zonenname liefert GetHealth01 == 0, damit floor(0) == 0, und
    // die Keule erscheint NIE - ohne Fehlermeldung. Deshalb die deterministische
    // Form; die Zonenvariante steht als Nachtrag im Slice-Bericht.
    //
    // transferToolDamageCoef = 1: ein stumpfes Messer liefert eine
    // angeschlagene Keule. Derselbe Schalter, den Vanilla fuer seine eigenen
    // Steaks benutzt - er kostet nichts und macht die Messerpflege spuerbar.
    // ========================================================================

    // --- Rind -> ChefZ_BeefLeg (Vanilla-Assets §20d) ---
    class Animal_BosTaurus : AnimalBase
    {
        class Skinning
        {
            class ChefZ_BeefLegYield
            {
                item = "ChefZ_BeefLeg";
                count = 1;
                transferToolDamageCoef = 1;
            };
        };
    };

    class Animal_BosTaurusF : AnimalBase
    {
        class Skinning
        {
            class ChefZ_BeefLegYield
            {
                item = "ChefZ_BeefLeg";
                count = 1;
                transferToolDamageCoef = 1;
            };
        };
    };

    // --- Schwein -> ChefZ_PorkLeg ---
    class Animal_SusDomesticus : AnimalBase
    {
        class Skinning
        {
            class ChefZ_PorkLegYield
            {
                item = "ChefZ_PorkLeg";
                count = 1;
                transferToolDamageCoef = 1;
            };
        };
    };

    // --- Wild -> ChefZ_VenisonLeg ---
    //
    // Rotwild (CervusElaphus) und Rehwild (CapreolusCapreolus). Beide liefern in
    // Vanilla DeerSteakMeat, also dieselbe Keule. Rentier (RangiferTarandus)
    // steht bewusst NICHT dabei: es liefert ReindeerSteakMeat und kommt in der
    // ganzen Wildkette dieses Moduls nicht vor - TR_VenisonToMinced nimmt
    // ausschliesslich DeerSteakMeat. Eine Rentierkeule, die zu Rotwildsteaks
    // zerfaellt, waere eine stille Umetikettierung.
    class Animal_CervusElaphus : AnimalBase
    {
        class Skinning
        {
            class ChefZ_VenisonLegYield
            {
                item = "ChefZ_VenisonLeg";
                count = 1;
                transferToolDamageCoef = 1;
            };
        };
    };

    class Animal_CervusElaphusF : AnimalBase
    {
        class Skinning
        {
            class ChefZ_VenisonLegYield
            {
                item = "ChefZ_VenisonLeg";
                count = 1;
                transferToolDamageCoef = 1;
            };
        };
    };

    class Animal_CapreolusCapreolus : AnimalBase
    {
        class Skinning
        {
            class ChefZ_VenisonLegYield
            {
                item = "ChefZ_VenisonLeg";
                count = 1;
                transferToolDamageCoef = 1;
            };
        };
    };

    class Animal_CapreolusCapreolusF : AnimalBase
    {
        class Skinning
        {
            class ChefZ_VenisonLegYield
            {
                item = "ChefZ_VenisonLeg";
                count = 1;
                transferToolDamageCoef = 1;
            };
        };
    };
};

// ---------------------------------------------------------------------------
// Anmeldung beim Core (02 §4).
//
// handcraftRecipeSlots = 4: dieses Modul bringt GENAU VIER Transforms mit,
// deren Prozess exec = "HANDCRAFT" hat. Die Liste, damit die Zahl nachpruefbar
// bleibt und nicht wieder driftet:
//
//   TR_CutBeefLeg       PROCESS_CUT_MEAT       Keule   + Messer -> 2x CowSteakMeat  + Bone
//   TR_CutPorkLeg       PROCESS_CUT_MEAT       Keule   + Messer -> 2x PigSteakMeat  + Lard
//   TR_CutVenisonLeg    PROCESS_CUT_MEAT       Keule   + Messer -> 2x DeerSteakMeat + Bone
//   TR_DicedMeat        PROCESS_CUT_MEAT       Fleisch + Messer -> Wuerfel (seit 29.08.2026 wieder)
//
// Die Zahl ist eine Reservierung in Vanillas Rezeptliste und muss vorab
// feststehen; die Begruendung steht im Kopf von ChefZ_HandcraftBridge.c. Nennt
// ein Slice zu wenig Plaetze, weist ChefZ_HandcraftBridge.Reserve die
// ueberzaehligen Transforms ab - sie erscheinen dann nie im Kontextmenue.
//
// Frueher fuenf: Wuerfeln (TR_DicedMeat) und Darm reinigen (TR_SausageCasing)
// sind am 29.08.2026 entfallen - die Eintoepfe nehmen gewolftes Fleisch, und
// Vanillas Darm IST die Huelle. Alles andere laeuft an einer Station und
// braucht keinen Platz.
//
// dataFiles[] beginnt mit dem PBO-Praefix, also dem ORDNERNAMEN des Addons.
// ---------------------------------------------------------------------------
class CfgChefZ
{
    class ChefZ_Meat
    {
        chefzApiVersion = 1;
        loadOrder = 200;
        handcraftRecipeSlots = 4;
        dataFiles[] =
        {
            "ChefZ_Meat/Config/Ingredients/Meat.json",
            "ChefZ_Meat/Config/Processing/Meat.json",
            "ChefZ_Meat/Config/Recipes/Sausage.json"
        };
    };
};
