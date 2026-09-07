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
// Alle essbaren Klassen erben die STRUKTUR (class Food mit FoodStages und
// FoodStageTransitions) von ChefZ_GrainFoodBase aus ChefZ_Farming. Der
// Config-Knoten ist die Anmeldung an Vanillas Magen (01 V7); ohne ihn
// verschwindet der Bissen still (PlayerStomach.c:209-250, :403).
//
// Damit traegt auch ChefZ_Dough FoodStageTransitions - und das ist hier keine
// Formalie: der Teig ist Zutat eines Rezepts, liegt also im Kochgeraet,
// waehrend Vanilla den Garzustand fortschreibt. Ohne Uebergaenge
// faellt FoodStage.GetNextFoodStageType auf BURNED zurueck (01 V4,
// FoodStage.c:472) - der Spieler legte Teig in die Pfanne und bekaeme Kohle.
//
// ===========================================================================
// DIE ZAHLEN STEHEN SEIT DEM 31.08.2026 HIER, NICHT MEHR NUR IN DER BASIS
// ===========================================================================
//
// WIE DIE ENGINE RECHNET, woertlich aus scripts - 1.29:
//
//     volume = m_Profile.GetFullnessIndex() * m_Amount;
//     -- 4_World/DayZ/Classes/PlayerStomach.c:86
//
//     float energy_per_unit = profile.GetEnergy() / 100;
//     float water_per_unit  = profile.GetWaterContent() / 100;
//     -- PlayerStomach.c:92-93
//
// Zwei verschiedene Einheiten in derselben Klasse, und genau daran ist die
// Kette bisher gescheitert:
//
//   MAGENVOLUMEN eines ganzen Items = fullnessIndex * varQuantityMax.
//                                     KEIN Teiler 100.
//   ENERGIE      eines ganzen Items = energy / 100 * varQuantityMax.
//   WASSER       eines ganzen Items = water  / 100 * varQuantityMax.
//
// Die Schwellen, gegen die das Volumen laeuft:
//   2000  Erbrechen  - PlayerConstants.VOMIT_THRESHOLD (PlayerConstants.c:208)
//   1000  "Stuffed"  - PlayerConstants.BT_STOMACH_VOLUME_LVL3 (:200)
//     25  ein grosser Bissen - UAQuantityConsumed.EAT_BIG
// Und die Energie gegen SL_ENERGY_MAX = 5000 (PlayerConstants.c:44).
//
// WELCHE ZAHL UEBERHAUPT GILT. Sobald ein Item eine FoodStage traegt - und
// jede Klasse dieses Moduls traegt eine, weil die Basis einen class
// Food-Block fuehrt -, liest Vanilla NUR NOCH die Stufe:
//
//   Edible_Base.GetFoodTotalVolume (Edible_Base.c:391-397) fragt zuerst
//   FoodStage.GetFullnessIndex; die liest nutrition_properties[0]
//   (FoodStage.c:314-317). GetFoodEnergy (:407-419) und GetFoodWater
//   (:422-434) machen dasselbe mit Index 1 und 2. class Nutrition wird in
//   diesem Fall NIE gelesen - es bleibt Rueckfallweg fuer den Aufruf ohne
//   Item (food_stage == 0) und wird hier trotzdem gepflegt, damit die
//   Rohstufe an beiden Orten dasselbe sagt.
//
// WARUM DIE BASIS FUER DIESES MODUL NICHT REICHT. ChefZ_GrainFoodBase ist auf
// die MENGENKLASSEN der Kette kalibriert (ChefZ_Wheat, ChefZ_Flour:
// varQuantityMax 500..1000, fullnessIndex 0.25 -> Volumen 125..250). Die
// Basis sagt das in ihrem eigenen Kommentar und uebergibt die Stueckklassen
// ausdruecklich an dieses Modul. Fuer varQuantityMax = 1 waere 0.25 nicht
// "etwas wenig", sondern nichts: ein ganzes Brot mit Magenvolumen 0.25 und
// 200/100 = 2 Energie. Deshalb fuehrt JEDE Klasse hier ihren eigenen,
// VOLLSTAENDIGEN Stufensatz - inklusive Raw.
//
// VOLLSTAENDIG ist woertlich gemeint. Eine Stufe, die im Stufenbestand der
// Klasse fehlt, liefert 0 (FoodStage.GetNutritionPropertyFromIndex,
// FoodStage.c:262-263) - nicht den Wert der Basis. Zwar erbt ein
// gleichnamiger Configknoten von der Basis, aber ein halb ueberschriebener
// Satz mischt zwei Skalen; hier steht deshalb jede Stufe ausgeschrieben.
//
// RAW IST NICHT "UNGEBACKEN". Brot und Fladenbrot entstehen als Ergebnis
// eines Rezepts, und ChefZ_Applicator.SpawnOutput erzeugt sie mit
// CreateEntityInCargoEx - ohne Stufenwechsel. FoodStage setzt im Konstruktor
// ChangeFoodStage(FoodStageType.RAW) (FoodStage.c:96). Ein frisch gebackenes
// Brot steht damit in der Stufe RAW. Die Raw-Zeile ist hier also der
// NORMALZUSTAND des fertigen Erzeugnisses, "Baked" ist das Brot, das im Ofen
// stehen geblieben ist: etwas trockener, etwas dichter.
//
// DIE ZIELVOLUMEN dieses Moduls (je ganzes Item):
//   ganzes Brot     400   Fladenbrot 275   roher Teig 120
//   frische Nudeln  375   Trockennudeln 400   (je 500 g)
// Verbrannt = 25 % der Rohstufe, Verdorben = 40 % - dieselben Verhaeltnisse
// wie in ChefZ_Cooking.
//
// NICHT MITSKALIERT wird das "stomach"-Feld in _deltas/grain.json. Es
// fliesst zur Laufzeit nirgends ein (ChefZ_NutritionDef sagt es im
// Dateikopf); es ist die Sollrechnung des Startaudits. Eine Skalierung dort
// entkoppelte die Auditzahlen von den Configzahlen, ohne im Spiel etwas zu
// bewirken.
//
// Reihenfolge in nutrition_properties[] nach FoodStage.c:314-345:
//   {fullnessIndex, energy, water, nutritionalIndex, toxicity, agents,
//    digestibility}
// digestibility 1 ist ausgeschrieben, obwohl 0 von PlayerStomach.c:97-100
// ohnehin als 1 gelesen wird - geschriebene Absicht statt stiller Vorgabe.
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

        // RECHNUNG (Herleitung im Dateikopf), varQuantityMax = 1:
        //   Volumen  = 120 * 1   = 120   (Zielband roher Teig 100..150)
        //   Energie  = 40000/100 = 400
        //   Wasser   = 9000/100  =  90   (150 ml Teigwasser, teils gebunden)
        //
        // Roher Teig SOLL unattraktiv sein, aber nicht wertlos: 400 gegen die
        // 900 des Brotes, das aus demselben Klumpen wird. Wer ihn roh isst,
        // verliert mehr als die Haelfte - das ist die Aussage, nicht eine
        // Strafe. Vorher (geerbte Basis): Volumen 0.25, Energie 2.
        class Nutrition
        {
            fullnessIndex = 120;
            energy = 40000;
            water = 9000;
            nutritionalIndex = 15;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        // Vollstaendiger eigener Stufensatz. "Baked" ist der Augenblick, in
        // dem der Teig im Geraet durchgart - REC_ChefZ_Bread und
        // REC_ChefZ_Flatbread zuenden auf genau dieser Stufe (doneStages
        // ["Baked"] in Config/GrainRecipes.json) und tauschen den Teig gegen
        // das Gebaeck. Die Stufe ist also fluechtig; sie traegt trotzdem
        // Zahlen, weil ein Teig auch in einem Geraet OHNE passendes Rezept
        // liegen bleiben kann.
        //
        //   Raw    120 / 400 kcal / 90 ml
        //   Baked  150 / 450 kcal / 40 ml   (gart durch, trocknet aus)
        //   Burned  30 / 100 kcal /  0 ml   (25 % von Raw)
        //   Rotten  48 / 160 kcal / 36 ml   (40 % von Raw, toxicity 20)
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {120, 40000, 9000, 15, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {150, 45000, 4000, 18, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {30, 10000, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {48, 16000, 3600, 0, 20, 0, 1}; };
            };
        };
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

        // RECHNUNG (Herleitung im Dateikopf), varQuantityMax = 500 GRAMM:
        //   Volumen  = 0.75 * 500 = 375   (500 g frische Nudeln)
        //   Energie  = 220/100 * 500 = 1100
        //   Wasser   = 35/100  * 500 = 175  (frischer Teig ist feucht)
        //
        // 0.75 ist der UNTERE Rand von Vanillas eigenem Band 0.75..2.5 fuer
        // Gramm-Ware - Nudeln sind voluminoes, aber nicht dicht.
        //
        // Warum die Klasse einen eigenen Satz bekommt, obwohl 0.25 aus der
        // Basis fuer Gramm-Ware "nicht kaputt" waere: mit dem Basissatz waeren
        // ChefZ_RawPasta und ChefZ_DriedPasta ZAHLENGLEICH (je 125 Volumen,
        // 1000 Energie, 50 Wasser). Der ganze Trockenschritt
        // TR_RawPastaToDriedPasta haette dann keine Wirkung ausser der
        // Haltbarkeit. Frisch ist feuchter und energieaermer, trocken ist
        // konzentriert - das steht jetzt in den Zahlen.
        class Nutrition
        {
            fullnessIndex = 0.75;
            energy = 220;
            water = 35;
            nutritionalIndex = 25;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        //   Raw    375 / 1100 kcal / 175 ml
        //   Baked  395 / 1165 kcal /  80 ml   (trocknet aus, verdichtet)
        //   Burned  95 /  275 kcal /   0 ml   (25 % von Raw)
        //   Rotten 150 /  440 kcal /  70 ml   (40 % von Raw, toxicity 20)
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {0.75, 220, 35, 25, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {0.79, 233, 16, 25, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.19, 55, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.30, 88, 14, 0, 20, 0, 1}; };
            };
        };
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

        // RECHNUNG (Herleitung im Dateikopf), varQuantityMax = 500 GRAMM:
        //   Volumen  = 0.80 * 500 = 400   (dichter als die frische Nudel)
        //   Energie  = 300/100 * 500 = 1500
        //   Wasser   = 3/100   * 500 =   15  (getrocknet, praktisch nichts)
        //
        // Der Vorratsartikel der Kette und deshalb der energiereichste Posten
        // dieses Moduls: 500 g Trockennudeln sind fuenf Portionen. 1500 gegen
        // SL_ENERGY_MAX 5000 (PlayerConstants.c:44) - viel, aber die Ware ist
        // teilbar (canBeSplit), niemand isst sie in einem Zug. Das Volumen
        // bleibt mit 400 deutlich unter "Stuffed" 1000.
        class Nutrition
        {
            fullnessIndex = 0.80;
            energy = 300;
            water = 3;
            nutritionalIndex = 22;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        //   Raw    400 / 1500 kcal / 15 ml
        //   Baked  420 / 1590 kcal / 10 ml
        //   Burned 100 /  375 kcal /  0 ml   (25 % von Raw)
        //   Rotten 160 /  600 kcal /  5 ml   (40 % von Raw, toxicity 20)
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {0.80, 300, 3, 22, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {0.84, 318, 2, 22, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {0.20, 75, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {0.32, 120, 1, 0, 20, 0, 1}; };
            };
        };
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

        // RECHNUNG (Herleitung im Dateikopf), varQuantityMax = 1:
        //   Volumen  = 400 * 1    = 400   (Zielband ganzes Brot 350..450)
        //   Energie  = 90000/100  = 900   (Zielband 700..1200)
        //   Wasser   = 3000/100   =  30   (Brot loescht keinen Durst)
        //
        // 900 gegen SL_ENERGY_MAX 5000 (PlayerConstants.c:44): ein Laib
        // deckt knapp ein Fuenftel. Das Volumen 400 liegt bei zwei Fuenfteln
        // von "Stuffed" 1000 und bei einem Fuenftel der Kotzschwelle 2000 -
        // zwei Laibe hintereinander gehen, drei nicht.
        //
        // Vorher (geerbte Basis): Volumen 0.25, Energie 2, Wasser 0.1. Ein
        // ganzes Brot war damit ernaehrungsphysiologisch ein Krumen.
        class Nutrition
        {
            fullnessIndex = 400;
            energy = 90000;
            water = 3000;
            nutritionalIndex = 30;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        // "Raw" ist hier der FRISCHE LAIB, nicht roher Teig: das Rezept
        // erzeugt das Brot mit CreateEntityInCargoEx, und FoodStage startet
        // jedes Item in RAW (FoodStage.c:96). Siehe Dateikopf.
        //
        //   Raw    400 / 900 kcal / 30 ml   frisch aus Topf oder Ofen
        //   Baked  420 / 950 kcal / 15 ml   im Ofen stehen geblieben:
        //                                   trockener, dichter, gehaltvoller
        //   Burned 100 / 225 kcal /  0 ml   (25 % von Raw)
        //   Rotten 160 / 360 kcal / 12 ml   (40 % von Raw, toxicity 20)
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {400, 90000, 3000, 30, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {420, 95000, 1500, 30, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {100, 22500, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {160, 36000, 1200, 0, 20, 0, 1}; };
            };
        };
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

        // RECHNUNG (Herleitung im Dateikopf), varQuantityMax = 1:
        //   Volumen  = 275 * 1    = 275   (Zielband Fladenbrot 250..300)
        //   Energie  = 60000/100  = 600
        //   Wasser   = 2500/100   =  25
        //
        // Derselbe Teigklumpen wie beim Brot, nur flach in der Pfanne: die
        // Masse ist dieselbe, die Kruste groesser, der Wasseranteil kleiner.
        // Deshalb rund zwei Drittel des Laibs und nicht die Haelfte - und
        // deshalb steht das Brot als Ziel der aufwendigeren Zubereitung
        // (Topf/Ofen statt Pfanne) darueber.
        //
        // Vorher (geerbte Basis): Volumen 0.25, Energie 2, Wasser 0.1.
        class Nutrition
        {
            fullnessIndex = 275;
            energy = 60000;
            water = 2500;
            nutritionalIndex = 28;
            toxicity = 0;
            agents = 0;
            digestibility = 1;
        };

        // "Raw" ist auch hier der FRISCHE Fladen - Rezeptergebnisse starten in
        // RAW (FoodStage.c:96, Dateikopf).
        //
        //   Raw    275 / 600 kcal / 25 ml
        //   Baked  290 / 630 kcal / 12 ml   (in der Pfanne liegen geblieben)
        //   Burned  69 / 150 kcal /  0 ml   (25 % von Raw)
        //   Rotten 110 / 240 kcal / 10 ml   (40 % von Raw, toxicity 20)
        class Food
        {
            class FoodStages
            {
                class Raw    { nutrition_properties[] = {275, 60000, 2500, 28, 0, 0, 1}; };
                class Baked  { nutrition_properties[] = {290, 63000, 1200, 28, 0, 0, 1}; };
                class Burned { nutrition_properties[] = {69, 15000, 0, 0, 0, 0, 1}; };
                class Rotten { nutrition_properties[] = {110, 24000, 1000, 0, 20, 0, 1}; };
            };
        };
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
