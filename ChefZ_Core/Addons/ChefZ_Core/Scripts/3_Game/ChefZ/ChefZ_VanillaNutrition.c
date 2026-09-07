//==============================================================================
// ChefZ_VanillaNutrition - CfgVehicles lesen, so wie die Engine es liest
//
// Entwurf: 13 E1 ("CfgVehicles bleibt Wahrheit, ChefZ auditiert von aussen"),
// 13 E4 (der Vanilla-Rueckfall der Auffindungsreihenfolge), 13 §3 / 01 V7
// (die Registrierungsbedingung des Magens), 13 §8.
//
// ---------------------------------------------------------------------------
// Warum diese Klasse existiert und warum sie NICHTS rechnet
// ---------------------------------------------------------------------------
// Sie ist ein Lesegeraet, kein Regelwerk. Jede Zeile hier bildet einen Pfad
// nach, den Vanilla selbst benutzt - abgelesen aus den Quellen, nicht geraten:
//
//   Naehrwert je Garstufe
//     CfgVehicles <cls> Food FoodStages <Stage> nutrition_properties
//     -> FoodStage.c:126, SetupFoodStageMapping()
//
//   Naehrwert ohne Garstufen
//     CfgVehicles <cls> Nutrition {energy, water, nutritionalIndex,
//                                  fullnessIndex, toxicity, digestibility}
//     -> Edible_Base.c:402 ff., GetFoodEnergy() und Geschwister
//
//   Registrierung beim Magen
//     scope != 0 UND ("Nutrition" ODER "Food" vorhanden)
//     -> PlayerStomach.c:230-247, InitData()
//
// Die letzte Zeile ist die wichtigste des ganzen Teilsystems. Sie ist Befund
// 01 V7, und sie lautet ausgeschrieben: eine essbare Klasse, die diese
// Bedingung nicht erfuellt, wird gegessen, verschwindet aus dem Inventar und
// saettigt NICHTS - ohne Fehlermeldung, ohne Logeintrag, ohne Hinweis. Es gibt
// keinen leiseren Content-Fehler in DayZ.
//
// ---------------------------------------------------------------------------
// Warum die Werte NICHT ueber FoodStage.GetEnergy() gelesen werden
// ---------------------------------------------------------------------------
// Weil es beim Serverstart nicht ginge. FoodStage.m_EdibleBasePropertiesMap
// wird von SetupFoodStageMapping() gefuellt, und das laeuft im Konstruktor
// eines Edible_Base - also erst, wenn erstmals ein Item DIESER Klasse
// existiert. Beim Boot existiert keines, und die Karte antwortete auf jede
// Frage mit 0.
//
// Der Audit liest deshalb dieselben Configpfade direkt. Das ist kein Umweg an
// der Engine vorbei, sondern derselbe Weg eine Ebene tiefer - und der einzige,
// der ohne Item funktioniert.
//
// KEIN CONTENT: hier steht kein Klassenname. "Edible_Base" ist ein
// VANILLA-Typ und die Wurzel alles Essbaren in DayZ, kein ChefZ-Inhalt.
//
// Layer: 3_Game. g_Game gibt es erst hier (00 §4); ItemBase und FoodStageType
// leben eine Ebene hoeher und kommen in dieser Datei ausdruecklich nicht vor.
//==============================================================================

class ChefZ_VanillaNutrition
{
    static const string CFG_VEHICLES = "CfgVehicles ";

    //! Vanillawurzel alles Essbaren (01 V7).
    static const string EDIBLE_ROOT  = "Edible_Base";

    //! Notbremse gegen eine ringfoermige Config. Die gibt es nicht - aber eine
    //! Endlosschleife beim Serverstart waere der teuerste denkbare Preis fuer
    //! diese Annahme.
    static const int MAX_INHERIT_DEPTH = 32;

    //--------------------------------------------------------------------------
    // Die Indizes von nutrition_properties, woertlich aus FoodStage.c
    //--------------------------------------------------------------------------
    static const int IDX_FULLNESS      = 0;
    static const int IDX_ENERGY        = 1;
    static const int IDX_WATER         = 2;
    static const int IDX_NUTRITIONAL   = 3;
    static const int IDX_TOXICITY      = 4;
    // 5 = agents (BITMASKE, siehe ChefZ_NutritionVector - kein Feld)
    static const int IDX_DIGESTIBILITY = 6;
    // 7 = agentsPerDigest (gehoert zur Maske, kein Naehrwert)

    //==========================================================================
    // Existenz und Vererbung
    //==========================================================================

    static bool ClassExists(string cls)
    {
        if (cls == "" || !g_Game)
            return false;
        return g_Game.ConfigIsExisting(CFG_VEHICLES + cls);
    }

    /**
     * Haengt diese Klasse unter Edible_Base?
     *
     * Ueber ConfigGetBaseName die Elternkette hoch. Das ist derselbe Test, den
     * der ChefZ_RecipeCompiler fuer seine Abweisung braucht - er steht seit
     * S12 hier und nicht mehr dort, damit es genau EINE Antwort auf die Frage
     * "ist das essbar" gibt.
     */
    static bool IsEdible(string cls)
    {
        if (!g_Game || cls == "")
            return false;

        string current = cls;
        for (int depth = 0; depth < MAX_INHERIT_DEPTH; depth++)
        {
            if (current == EDIBLE_ROOT)
                return true;

            string parent;
            if (!g_Game.ConfigGetBaseName(CFG_VEHICLES + current, parent))
                return false;
            if (parent == "" || parent == current)
                return false;

            current = parent;
        }
        return false;
    }

    //==========================================================================
    // Die Registrierungsbedingung des Magens (01 V7)
    //==========================================================================

    //! scope der Klasse. 0 heisst: PlayerStomach.InitData ueberspringt sie.
    static int ScopeOf(string cls)
    {
        if (!g_Game || cls == "")
            return 0;
        return g_Game.ConfigGetInt(CFG_VEHICLES + cls + " scope");
    }

    //! "class Nutrition" ODER "class Food" - genau die Oder-Verknuepfung aus
    //! PlayerStomach.InitData.
    static bool HasNutritionOrFood(string cls)
    {
        if (!g_Game || cls == "")
            return false;

        string path = CFG_VEHICLES + cls;
        if (g_Game.ConfigIsExisting(path + " Nutrition"))
            return true;
        return g_Game.ConfigIsExisting(path + " Food");
    }

    /**
     * Wuerde der Magen diese Klasse registrieren?
     *
     * Die eine Frage, aus der die drei Fangstellen von 13 §3 bestehen. Sie ist
     * ABSICHTLICH so formuliert und nicht als "ist die Klasse in Ordnung":
     * eine Klasse, die nicht essbar ist, ist nicht falsch - sie wird nur nie
     * registriert, und das ist richtig so.
     *
     * @param outReason Klartextgrund bei false. Leer bei true.
     */
    static bool WouldRegisterAtStomach(string cls, out string outReason)
    {
        outReason = "";

        if (!g_Game || cls == "")
        {
            outReason = "keine Config verfuegbar";
            return false;
        }

        if (ScopeOf(cls) == 0)
        {
            outReason = "scope = 0";
            return false;
        }
        if (!HasNutritionOrFood(cls))
        {
            outReason = "weder \"class Nutrition\" noch \"class Food\"";
            return false;
        }
        return true;
    }

    //==========================================================================
    // Werte lesen
    //==========================================================================

    /**
     * nutrition_properties einer Garstufe.
     *
     * @return false, wenn es fuer diese Klasse und diese Stufe keinen
     *         gefuellten Block gibt. outVec bleibt dann unveraendert.
     *
     * Ein LEERES nutrition_properties[] gilt ausdruecklich als "nicht
     * vorhanden": Vanilla behandelt es genauso und rechnet in dem Fall ueber
     * NutritionModifiers (FoodStage.c:280 ff.). Diese Ersatzrechnung bildet
     * der Audit NICHT nach - sie ist laut Vanilla-Kommentar ein Aufbau, "we do
     * not support internally", und ein nachgebauter Sonderfall waere eine
     * zweite Wahrheit ueber Zahlen, die niemand pflegt. Der Audit meldet
     * stattdessen die Luecke namentlich (13 §8).
     */
    static bool ReadStage(string cls, int stage, notnull ChefZ_NutritionVector outVec)
    {
        if (!g_Game || cls == "")
            return false;

        string stageName = ChefZ_VanillaStage.Name(stage);
        if (stageName == "" || stage <= ChefZ_VanillaStage.NONE)
            return false;

        string path = CFG_VEHICLES + cls + " Food FoodStages " + stageName + " nutrition_properties";
        if (!g_Game.ConfigIsExisting(path))
            return false;

        array<float> props = new array<float>();
        g_Game.ConfigGetFloatArray(path, props);
        if (props.Count() == 0)
            return false;

        outVec.fullness         = At(props, IDX_FULLNESS);
        outVec.energy           = At(props, IDX_ENERGY);
        outVec.water            = At(props, IDX_WATER);
        outVec.nutritionalIndex = At(props, IDX_NUTRITIONAL);
        outVec.toxicity         = At(props, IDX_TOXICITY);
        outVec.digestibility    = At(props, IDX_DIGESTIBILITY);
        return true;
    }

    /**
     * Der "class Nutrition"-Block einer Klasse ohne Garstufen.
     *
     * Die Feldnamen sind die aus Edible_Base.c und nicht die des Vektors:
     * "fullnessIndex" heisst dort so, "water" hat kein Suffix. Wer sie
     * angleicht, liest still Nullen.
     */
    static bool ReadNutritionBlock(string cls, notnull ChefZ_NutritionVector outVec)
    {
        if (!g_Game || cls == "")
            return false;

        string path = CFG_VEHICLES + cls + " Nutrition";
        if (!g_Game.ConfigIsExisting(path))
            return false;

        outVec.fullness         = g_Game.ConfigGetFloat(path + " fullnessIndex");
        outVec.energy           = g_Game.ConfigGetFloat(path + " energy");
        outVec.water            = g_Game.ConfigGetFloat(path + " water");
        outVec.nutritionalIndex = g_Game.ConfigGetFloat(path + " nutritionalIndex");
        outVec.toxicity         = g_Game.ConfigGetFloat(path + " toxicity");
        outVec.digestibility    = g_Game.ConfigGetFloat(path + " digestibility");
        return true;
    }

    /**
     * Die Vanilla-Werte einer Klasse - Garstufe zuerst, Nutrition-Block
     * danach.
     *
     * Das ist genau die Reihenfolge aus Edible_Base.GetFoodEnergy: erst die
     * Stufe, und nur wenn die nichts hergibt, der flache Block. Wer sie
     * umdreht, liest fuer ein Steak den Rohwert, egal wie lange es im Topf lag.
     *
     * @return false, wenn die Klasse ueberhaupt keine Naehrwertdaten traegt.
     */
    static bool Read(string cls, int stage, notnull ChefZ_NutritionVector outVec)
    {
        if (ReadStage(cls, stage, outVec))
            return true;
        return ReadNutritionBlock(cls, outVec);
    }

    //--------------------------------------------------------------------------

    //! 0.0 statt Absturz, wenn der Block kuerzer ist als der Index. Vanilla
    //! macht dasselbe (FoodStage.c:269, "if index > Count()-1 return 0").
    private static float At(notnull array<float> props, int index)
    {
        if (index < 0 || index >= props.Count())
            return 0.0;
        return props.Get(index);
    }
}
