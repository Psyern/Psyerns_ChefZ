//==============================================================================
// ChefZ_NutritionVector / ChefZ_NutritionFindingKind / ChefZ_NutritionFinding
//
// Entwurf: 13 §4 (Feldliste und Schnittstelle woertlich), 13 §2 (die
// Engine-Grenze, die alles bestimmt), 13 §5 (der Startaudit), 13 §8
// (Fehlerverhalten, Zeile fuer Zeile), 13 E1 (der Manager schreibt nichts),
// 13 E2 (ein Manager ohne Laufzeitwirkung), 01 V6 / V7.
//
// ---------------------------------------------------------------------------
// Der eine Satz, der diese Datei erklaert
// ---------------------------------------------------------------------------
// Ein ChefZ_NutritionVector ist eine RECHENGROESSE, kein Zustand. Er wird
// gebildet, verglichen, geloggt und weggeworfen. Es gibt ihn ausdruecklich
// NICHT an einem Item:
//
//   PlayerStomach.AddToStomach(class_name, amount, food_stage, agents, temp)
//     -> Edible_Base.GetNutritionalProfile(null, class_name, food_stage)
//                                          ^^^^ das Item ist NULL
//
// Der Naehrwert eines Bissens haengt in DayZ ausschliesslich von Klasse x
// Foodstage ab (01 V6). Ein Vektor am Item erreichte den Verzehrpfad nie -
// deshalb traegt ihn hier niemand, und deshalb hat der Nutrition Manager
// keine einzige schreibende Methode (13 E1).
//
// Was diese Rechengroesse leistet, leistet sie beim SERVERSTART: sie ist die
// Sollzahl, gegen die der Audit die Istwerte aus CfgVehicles haelt (13 §5).
//
// ---------------------------------------------------------------------------
// Die Feldreihenfolge ist Vanillas, nicht unsere
// ---------------------------------------------------------------------------
// Die sechs Felder bilden "nutrition_properties" aus
// CfgVehicles <cls> Food FoodStages <Stage> ab. Die Indizes sind aus
// 4_World/DayZ/Classes/FoodStage/FoodStage.c abgelesen, nicht geraten:
//
//   [0] FullnessIndex     GetFullnessIndex
//   [1] Energy            GetEnergy
//   [2] Water             GetWater
//   [3] NutritionalIndex  GetNutritionalIndex
//   [4] Toxicity          GetToxicity
//   [5] Agents            GetAgents            -- KEIN Feld hier, siehe unten
//   [6] Digestibility     GetDigestibility
//   [7] AgentsPerDigest   GetAgentsPerDigest   -- KEIN Feld hier, siehe unten
//
// Index 5 und 7 fehlen bewusst und woertlich nach 13 §4: Agenten sind eine
// BITMASKE und keine Menge. Sie zu addieren oder mit einem Faktor zu
// skalieren ergaebe eine Zahl, die keine Krankheit mehr bezeichnet. Der
// Applicator reicht Agenten deshalb als Maske durch (ODER-Verknuepfung), und
// der Naehrwertvektor fasst sie gar nicht erst an.
//
// KEIN CONTENT: hier steht keine Zutat, kein Gericht, keine Zahl aus einem
// Balancingblatt.
//
// Layer: 1_Core. Reine Rechnung, kein Engine-Typ - kein ItemBase, kein
// FoodStageType, kein NutritionalProfile (die leben in 4_World).
//==============================================================================

class ChefZ_NutritionVector
{
    /**
     * Die Grenze, an der eine Zahl aufhoert, eine Zahl zu sein.
     *
     * Enforce hat kein isnan(). Der Test laeuft deshalb ueber "liegt NICHT im
     * Bereich" statt ueber einen Gleichheitsvergleich: jeder Vergleich mit NaN
     * liefert false, also faellt NaN durch beide Seiten des Bereichstests -
     * ein "== NaN" wuerde ihn dagegen nie finden. Unendlich faellt aus
     * demselben Test heraus.
     *
     * Der Wert ist absichtlich weit jenseits jeder denkbaren Naehrwertzahl
     * (Vanillas groesste Energieangabe liegt im niedrigen vierstelligen
     * Bereich). Er ist eine Sondengrenze, kein Balancingdeckel - dieselbe
     * Unterscheidung, die 13 E6 fuer den Deckel trifft.
     */
    static const float FINITE_LIMIT = 1.0e12;

    float fullness;            // nutrition_properties[0]
    float energy;              // nutrition_properties[1]
    float water;               // nutrition_properties[2]
    float nutritionalIndex;    // nutrition_properties[3]
    float toxicity;            // nutrition_properties[4]
    float digestibility;       // nutrition_properties[6]

    void ChefZ_NutritionVector()
    {
        Reset();
    }

    void Reset()
    {
        fullness         = 0.0;
        energy           = 0.0;
        water            = 0.0;
        nutritionalIndex = 0.0;
        toxicity         = 0.0;
        digestibility    = 0.0;
    }

    //==========================================================================
    // Rechnen
    //==========================================================================

    /**
     * Diesen Vektor um "other * factor" erhoehen (13 §4).
     *
     * Das ist die eine Operation, aus der die Sollrechnung besteht: je
     * verbrauchter Zutat einmal aufgerufen, mit dem Basisvektor der Zutat und
     * dem Anteil, der tatsaechlich in den Topf gewandert ist (13 §5).
     *
     * factor == 0 ist ausdruecklich zulaessig und heisst "zaehlt mit 0" -
     * genau das verlangt 13 §8 fuer eine Zutat ohne Naehrwertdaten. Der Vektor
     * bleibt dann unveraendert; die LUECKE meldet der Aufrufer namentlich, und
     * er tut es unabhaengig von dieser Zeile.
     */
    void AddScaled(notnull ChefZ_NutritionVector other, float factor)
    {
        fullness         = fullness         + other.fullness         * factor;
        energy           = energy           + other.energy           * factor;
        water            = water            + other.water            * factor;
        nutritionalIndex = nutritionalIndex + other.nutritionalIndex * factor;
        toxicity         = toxicity         + other.toxicity         * factor;
        digestibility    = digestibility    + other.digestibility    * factor;
    }

    //! Alle Felder mit demselben Faktor - das ist der nutritionModifier des
    //! Rezepts (13 §5, "x nutritionModifier 1.10").
    void Scale(float f)
    {
        fullness         = fullness         * f;
        energy           = energy           * f;
        water            = water            * f;
        nutritionalIndex = nutritionalIndex * f;
        toxicity         = toxicity         * f;
        digestibility    = digestibility    * f;
    }

    void CopyFrom(notnull ChefZ_NutritionVector other)
    {
        fullness         = other.fullness;
        energy           = other.energy;
        water            = other.water;
        nutritionalIndex = other.nutritionalIndex;
        toxicity         = other.toxicity;
        digestibility    = other.digestibility;
    }

    /**
     * Feldweise auf [floors, caps] klemmen (13 §4).
     *
     * 13 §8: "Sollwert ueberschreitet einen Deckel -> Geklemmt, INFO mit
     * Rezept-ID. Das ist ein Balancinghinweis fuer den Reviewer, kein Fehler."
     *
     * Wichtig zur Einordnung: das ist KEIN Deckelsystem im Sinne von 13 E6.
     * Es gibt in V1 keinen NutritionCap, der Kesselgerichte begrenzt - der
     * waere sinnlos, weil der Sollwert nie angewandt wird. Diese Klemmung ist
     * eine Sonde: sie faengt die Groessenordnung ab, in der eine Sollrechnung
     * offensichtlich entgleist ist, damit im Log eine Zahl steht und keine
     * Zahlenkolonne.
     *
     * @return true, wenn mindestens ein Feld geklemmt wurde.
     */
    bool ClampTo(notnull ChefZ_NutritionVector caps, notnull ChefZ_NutritionVector floors)
    {
        bool touched = false;

        if (OutOfBounds(fullness, floors.fullness, caps.fullness))
        {
            fullness = Pull(fullness, floors.fullness, caps.fullness);
            touched  = true;
        }
        if (OutOfBounds(energy, floors.energy, caps.energy))
        {
            energy  = Pull(energy, floors.energy, caps.energy);
            touched = true;
        }
        if (OutOfBounds(water, floors.water, caps.water))
        {
            water   = Pull(water, floors.water, caps.water);
            touched = true;
        }
        if (OutOfBounds(nutritionalIndex, floors.nutritionalIndex, caps.nutritionalIndex))
        {
            nutritionalIndex = Pull(nutritionalIndex, floors.nutritionalIndex, caps.nutritionalIndex);
            touched          = true;
        }
        if (OutOfBounds(toxicity, floors.toxicity, caps.toxicity))
        {
            toxicity = Pull(toxicity, floors.toxicity, caps.toxicity);
            touched  = true;
        }
        if (OutOfBounds(digestibility, floors.digestibility, caps.digestibility))
        {
            digestibility = Pull(digestibility, floors.digestibility, caps.digestibility);
            touched       = true;
        }

        return touched;
    }

    /**
     * "Liegt NICHT im Bereich" statt "liegt ausserhalb".
     *
     * Der Unterschied ist NaN: jeder Vergleich mit NaN liefert false, also
     * faellt NaN durch beide Seiten eines "groesser/kleiner"-Tests und bliebe
     * unbemerkt stehen. Ueber die Verneinung des Bereichstests faellt es auf.
     */
    private static bool OutOfBounds(float value, float low, float high)
    {
        return !(value >= low && value <= high);
    }

    //! Auf die naechstliegende Grenze ziehen. NaN erfuellt keinen der beiden
    //! Vergleiche und wird zu 0 - eine Zahl, die sichtbar keine Aussage ist.
    private static float Pull(float value, float low, float high)
    {
        if (value > high)
            return high;
        if (value < low)
            return low;
        return 0.0;
    }

    //==========================================================================
    // Pruefen
    //==========================================================================

    /**
     * Ist jedes Feld eine benutzbare Zahl? (13 §4, 13 §8)
     *
     * 13 §8: "Ueberlauf oder NaN in der Sollrechnung -> IsFinite() schlaegt
     * an, Vektor verworfen, ERROR. Der Audit meldet 'nicht berechenbar' statt
     * einer Fantasiezahl."
     *
     * Die Alternative waere, eine kaputte Zahl durchzureichen und im Log
     * auszugeben. Genau das darf nicht passieren: ein Balance-Reviewer, der
     * "-1.#IND" im Startlog liest, weiss nicht, ob sein Rezept schlecht
     * gebalanct oder der Audit kaputt ist.
     */
    bool IsFinite()
    {
        if (!Finite(fullness))         return false;
        if (!Finite(energy))           return false;
        if (!Finite(water))            return false;
        if (!Finite(nutritionalIndex)) return false;
        if (!Finite(toxicity))         return false;
        if (!Finite(digestibility))    return false;
        return true;
    }

    private static bool Finite(float v)
    {
        return v >= -FINITE_LIMIT && v <= FINITE_LIMIT;
    }

    //! true, wenn der Vektor in JEDEM Feld null ist. Der Audit macht daraus
    //! den Befund ZERO_INGREDIENT (13 §8, "Zutat ohne Naehrwertdaten").
    bool IsZero()
    {
        if (fullness         != 0.0) return false;
        if (energy           != 0.0) return false;
        if (water            != 0.0) return false;
        if (nutritionalIndex != 0.0) return false;
        if (toxicity         != 0.0) return false;
        if (digestibility    != 0.0) return false;
        return true;
    }

    //==========================================================================

    string ToDebugString()
    {
        return "energie=" + energy.ToString()
             + " saettigung=" + fullness.ToString()
             + " wasser=" + water.ToString()
             + " naehrwert=" + nutritionalIndex.ToString()
             + " toxisch=" + toxicity.ToString()
             + " verdaulich=" + digestibility.ToString();
    }

    //! Nur fuer den Selbsttest (S12).
    static bool SelfCheck()
    {
        ChefZ_NutritionVector a = new ChefZ_NutritionVector();
        if (!a.IsZero())                                return false;
        if (!a.IsFinite())                              return false;

        ChefZ_NutritionVector b = new ChefZ_NutritionVector();
        b.energy   = 100.0;
        b.water    =  10.0;
        b.fullness =   5.0;

        a.AddScaled(b, 2.0);
        if (a.energy != 200.0)                          return false;
        if (a.water  !=  20.0)                          return false;
        if (a.IsZero())                                 return false;

        a.AddScaled(b, 0.0);                            // "zaehlt mit 0"
        if (a.energy != 200.0)                          return false;

        a.Scale(0.5);
        if (a.energy != 100.0)                          return false;

        ChefZ_NutritionVector copy = new ChefZ_NutritionVector();
        copy.CopyFrom(a);
        if (copy.energy != a.energy)                    return false;

        // Klemmung: oben, unten, und "nichts zu tun".
        ChefZ_NutritionVector caps   = new ChefZ_NutritionVector();
        ChefZ_NutritionVector floors = new ChefZ_NutritionVector();
        caps.energy = 50.0;  caps.water = 1000.0;  caps.fullness = 1000.0;
        caps.nutritionalIndex = 1000.0; caps.toxicity = 1000.0; caps.digestibility = 1000.0;
        floors.energy = -1000.0; floors.water = -1000.0; floors.fullness = -1000.0;
        floors.nutritionalIndex = -1000.0; floors.toxicity = -1000.0; floors.digestibility = -1000.0;

        if (!a.ClampTo(caps, floors))                   return false;
        if (a.energy != 50.0)                           return false;
        if (a.ClampTo(caps, floors))                    return false;   // schon drin

        // Unendlichkeit muss als "nicht berechenbar" auffallen.
        ChefZ_NutritionVector broken = new ChefZ_NutritionVector();
        broken.energy = FINITE_LIMIT * 10.0;
        if (broken.IsFinite())                          return false;

        return true;
    }
}

//==============================================================================

/**
 * Die Arten von Auditbefunden (13 §4, 13 §8).
 *
 * Die ersten vier stehen woertlich in 13 §4. Die drei danach sind EHRLICH
 * BENANNTE ERGAENZUNGEN: 13 §8 verlangt fuer sie ausdruecklich eine Meldung
 * ("WARN mit Rezept-ID", "ERROR ... nicht berechenbar", "INFO mit Rezept-ID"),
 * nennt aber keinen Befundnamen. Sie hier zu fuehren statt sie direkt ins Log
 * zu schreiben, hat einen Grund: der Audit gibt seine Befunde als LISTE
 * heraus (13 §7, "per Adminkommando abrufbar"), und ein Befund, der nur im
 * RPT steht, taucht dort nie auf.
 */
class ChefZ_NutritionFindingKind
{
    //! Essbare Ergebnisklasse ohne "class Nutrition" und ohne "class Food".
    //! Die Engine registriert sie nicht; das Gericht saettigt NICHT, und zwar
    //! ohne jede Meldung (01 V7). ERROR.
    static const string MISSING_BLOCK   = "MISSING_BLOCK";

    //! Ergebnisklasse mit scope = 0. PlayerStomach.InitData ueberspringt sie
    //! aus demselben Grund und mit derselben Folge (01 V7). ERROR.
    static const string SCOPE_ZERO      = "SCOPE_ZERO";

    //! Soll und Ist weichen ueber die Toleranz hinaus ab. WARN, KEINE
    //! Korrektur - der Core aendert nie einen Balancingwert (13 E1).
    static const string DEVIATION       = "DEVIATION";

    //! Eine Zutat der Sollrechnung hat weder ChefZ-Record noch
    //! CfgVehicles-Werte. Sie zaehlt mit 0 und wird NAMENTLICH genannt,
    //! damit die Luecke sichtbar ist (13 §8). INFO.
    static const string ZERO_INGREDIENT = "ZERO_INGREDIENT";

    //! Die Sollrechnung ist entgleist (NaN, Ueberlauf). ERROR, und der Audit
    //! gibt bewusst KEINE Zahl aus (13 §8).
    static const string NOT_COMPUTABLE  = "NOT_COMPUTABLE";

    //! nutritionModifier <= 0 - auf 1.0 gesetzt (13 §8). WARN.
    static const string BAD_MODIFIER    = "BAD_MODIFIER";

    //! Der Sollwert lief in die Sondengrenze (13 §8, Deckelzeile). INFO.
    static const string CLAMPED         = "CLAMPED";

    //! Log-Stufe eines Befundes. ChefZ_LogLevel.ERR / WARN / INFO.
    //!
    //! Sie steht HIER und nicht am Manager, weil sie zur Art gehoert und nicht
    //! zur Auswertung: wer einen Befund entgegennimmt - Log, Adminkommando,
    //! Cookbook -, soll seine Schwere ablesen koennen, ohne den Manager zu
    //! fragen.
    static int SeverityOf(string kind)
    {
        if (kind == MISSING_BLOCK)  return ChefZ_LogLevel.ERR;
        if (kind == SCOPE_ZERO)     return ChefZ_LogLevel.ERR;
        if (kind == NOT_COMPUTABLE) return ChefZ_LogLevel.ERR;
        if (kind == DEVIATION)      return ChefZ_LogLevel.WARN;
        if (kind == BAD_MODIFIER)   return ChefZ_LogLevel.WARN;
        return ChefZ_LogLevel.INFO;
    }

    static bool IsError(string kind)
    {
        return SeverityOf(kind) == ChefZ_LogLevel.ERR;
    }

    //! Nur fuer den Selbsttest (S12).
    static bool SelfCheck()
    {
        if (!IsError(MISSING_BLOCK))                            return false;
        if (!IsError(SCOPE_ZERO))                               return false;
        if (!IsError(NOT_COMPUTABLE))                           return false;
        if (IsError(DEVIATION))                                 return false;
        if (SeverityOf(DEVIATION)       != ChefZ_LogLevel.WARN) return false;
        if (SeverityOf(BAD_MODIFIER)    != ChefZ_LogLevel.WARN) return false;
        if (SeverityOf(ZERO_INGREDIENT) != ChefZ_LogLevel.INFO) return false;
        if (SeverityOf(CLAMPED)         != ChefZ_LogLevel.INFO) return false;
        if (SeverityOf("was auch immer") != ChefZ_LogLevel.INFO) return false;
        return true;
    }
}

//------------------------------------------------------------------------------

/**
 * Ein einzelner Auditbefund (13 §4, Feldliste woertlich).
 *
 * Er ist ein BERICHT, keine Anweisung. Nichts an dieser Klasse fuehrt zu einer
 * Aenderung an Daten, Items oder Configs - 13 E1: "Der Core aendert dabei
 * niemals einen Balancingwert zur Laufzeit. Er rechnet und meldet."
 *
 * Die einzige Ausnahme ist keine Ausnahme, sondern eine andere Stelle: ein
 * MISSING_BLOCK fuehrt zur ABWEISUNG des Rezepts, und die trifft der
 * ChefZ_RecipeCompiler beim Build (08 §8). Der Befund hier ist die zweite von
 * drei Fangstellen (13 §3) und nennt den Klassennamen, den die erste
 * (tools/chefz-validate) statisch nicht immer kennen kann.
 */
class ChefZ_NutritionFinding
{
    string kind;              // ChefZ_NutritionFindingKind.*
    string recipeId;
    string className;
    float  expectedEnergy;
    float  actualEnergy;
    float  deviationPct;      // (ist - soll) / soll * 100, 0 wenn nicht sinnvoll
    string message;

    void ChefZ_NutritionFinding()
    {
        kind           = "";
        recipeId       = "";
        className      = "";
        expectedEnergy = 0.0;
        actualEnergy   = 0.0;
        deviationPct   = 0.0;
        message        = "";
    }

    void Init(string findingKind, string recipe, string cls, string text)
    {
        kind      = findingKind;
        recipeId  = recipe;
        className = cls;
        message   = text;
    }

    int Severity()
    {
        return ChefZ_NutritionFindingKind.SeverityOf(kind);
    }

    /**
     * Die Zeile, wie sie im Startlog steht (13 §5, Beispielblock woertlich).
     *
     * Format: Stufe, Rezept-ID, Klartext. Die Stufe steht mit AUSGESCHRIEBENEM
     * Namen und nicht als Zahl, weil ein Betreiber das Startlog liest und
     * nicht die Stufentabelle.
     */
    string ToLine()
    {
        string s = ChefZ_LogLevel.Name(Severity());
        while (s.Length() < 6)
            s = s + " ";

        s = s + " " + recipeId;
        if (className != "")
            s = s + " (" + className + ")";
        return s + ": " + message;
    }
}
