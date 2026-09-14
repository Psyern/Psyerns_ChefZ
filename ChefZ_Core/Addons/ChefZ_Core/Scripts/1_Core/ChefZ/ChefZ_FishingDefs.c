//==============================================================================
// ChefZ_FishingMask / ChefZ_BaitKind / ChefZ_FishingLimits
// ChefZ_FishingYieldDef / ChefZ_BaitDef
//
// Entwurf: 20 §4.1 (Feldliste woertlich), 21 §2 und §3 (Datenformat und
// Validatorregeln), 20 §8 (Fehlerverhalten Zeile fuer Zeile), 02 E3 (Sentinel
// und explicitFields), 03 §5 (COMPILE).
//
// ---------------------------------------------------------------------------
// Was diese Datei beschreibt - und was ausdruecklich nicht
// ---------------------------------------------------------------------------
// Ein Fangertrag ist hier ein REINER DATENSATZ: ein Klassenname, ein Gewicht,
// ein Gewaesser, ein oder mehrere Fangverfahren. Ein Koeder ist ein
// Klassenname, eine Art, eine Zielliste und ein Faktor.
//
// KEIN CONTENT: in dieser Datei steht kein einziger Artname, kein Koedername
// und keine Zielbindung. "POND", "SEA", "BOTH", "ROD", "TRAP_LARGE",
// "TRAP_SMALL", "lure" und "soft" sind das geschlossene Vokabular des Formats
// aus 21 §2.2 und §3.1 - Vokabular des Core wie "HANDCRAFT" bei den Prozessen,
// keine Spielinhalte. Welche Art in welchem Gewaesser vorkommt, steht
// ausschliesslich in den Daten eines Content-Moduls.
//
// ---------------------------------------------------------------------------
// Warum die Masken hier als Literale stehen und nicht aus der Engine kommen
// ---------------------------------------------------------------------------
// Die Engine fuehrt dieselben Bits in AnimalCatchingConstants - einer Klasse
// aus 3_Game. Ein Record liegt laut 00 §4 in 1_Core und darf sie nicht sehen;
// ein 1_Core-Typ, der einen 3_Game-Typ nennt, ist eine Layer-Verletzung und
// uebersetzt je nach Modulzuschnitt gar nicht erst.
//
// Aufgeloest wird das NICHT dadurch, dass der Record die Maske spaeter
// berechnet, sondern durch eine Spiegelung mit Beweispflicht: die Zahlen unten
// sind Literale, und ChefZ_FishingSelfTest (5_Mission, wo die Engineklasse
// sichtbar ist) vergleicht JEDE von ihnen mit dem Engine-Gegenstueck. Weicht
// die Engine je ab, faellt der Selbsttest beim Serverstart auf - nicht der
// Spieler, dem der Fang fehlt.
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

/**
 * Gewaesser- und Verfahrensbits (21 §2.2).
 *
 * LITERALE, KEINE SCHIEBEAUSDRUECKE - aus demselben Grund wie in
 * ChefZ_LogDefs: Enforce faltet "1 << 1" nicht zu einer Konstante, und eine
 * ungefaltete Konstante trifft als case-Marke nichts.
 */
class ChefZ_FishingMask
{
    static const int ENVIRO_POND      = 1;      // AnimalCatchingConstants.MASK_ENVIRO_POND
    static const int ENVIRO_SEA       = 2;      // AnimalCatchingConstants.MASK_ENVIRO_SEA
    static const int ENVIRO_WATER_ALL = 3;      // AnimalCatchingConstants.MASK_ENVIRO_WATER_ALL

    static const int METHOD_ROD        = 1;     // AnimalCatchingConstants.MASK_METHOD_ROD
    static const int METHOD_TRAP_LARGE = 2;     // ..MASK_METHOD_FISHTRAP_LARGE
    static const int METHOD_TRAP_SMALL = 4;     // ..MASK_METHOD_FISHTRAP_SMALL

    //! Beide Reusenbits zusammen. Gebraucht fuer genau eine Pruefung: ein
    //! Ertrag mit lureOnly darf keine Reuse nennen (21 §2.4).
    static const int METHOD_TRAP_ANY   = 6;

    static const string ENVIRO_POND_NAME = "POND";
    static const string ENVIRO_SEA_NAME  = "SEA";
    static const string ENVIRO_BOTH_NAME = "BOTH";

    static const string METHOD_ROD_NAME        = "ROD";
    static const string METHOD_TRAP_LARGE_NAME = "TRAP_LARGE";
    static const string METHOD_TRAP_SMALL_NAME = "TRAP_SMALL";

    //! 0 = unbekannt. Ein unbekanntes Gewaesser weist den Record AB (20 §8):
    //! ein stumm falsch einsortierter Ertrag ist schlimmer als ein fehlender.
    static int EnviroFromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToUpper();

        if (n == ENVIRO_POND_NAME) return ENVIRO_POND;
        if (n == ENVIRO_SEA_NAME)  return ENVIRO_SEA;
        if (n == ENVIRO_BOTH_NAME) return ENVIRO_WATER_ALL;
        return 0;
    }

    //! 0 = unbekannt.
    static int MethodFromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToUpper();

        if (n == METHOD_ROD_NAME)        return METHOD_ROD;
        if (n == METHOD_TRAP_LARGE_NAME) return METHOD_TRAP_LARGE;
        if (n == METHOD_TRAP_SMALL_NAME) return METHOD_TRAP_SMALL;
        return 0;
    }

    static string ValidEnviroNames()
    {
        return ENVIRO_POND_NAME + ", " + ENVIRO_SEA_NAME + ", " + ENVIRO_BOTH_NAME;
    }

    static string ValidMethodNames()
    {
        return METHOD_ROD_NAME + ", " + METHOD_TRAP_LARGE_NAME + ", " + METHOD_TRAP_SMALL_NAME;
    }

    static bool SelfCheck()
    {
        if (EnviroFromName(" pond ") != ENVIRO_POND)                return false;
        if (EnviroFromName(ENVIRO_BOTH_NAME) != ENVIRO_WATER_ALL)   return false;
        if (EnviroFromName("Teich") != 0)                           return false;
        if (MethodFromName("rod") != METHOD_ROD)                    return false;
        if (MethodFromName("unfug") != 0)                           return false;
        if ((METHOD_TRAP_LARGE | METHOD_TRAP_SMALL) != METHOD_TRAP_ANY) return false;
        if ((ENVIRO_POND | ENVIRO_SEA) != ENVIRO_WATER_ALL)         return false;
        return true;
    }
}

//------------------------------------------------------------------------------

/**
 * Die beiden Koederarten aus 21 §3.1.
 *
 * Als Konstanten statt als enum, aus demselben Grund wie ChefZ_ProcessExec:
 * der Wert steht als String im JSON und in der Game-Config, und ein enum
 * faellt bei einem Tippfehler still auf 0 - hier waere das LURE und damit
 * lautlos ein Koeder, der nie verbraucht wird.
 */
class ChefZ_BaitKind
{
    static const int LURE = 0;      //! bleibt am Haken und nimmt Schaden
    static const int SOFT = 1;      //! wird verbraucht wie in Vanilla

    static const string LURE_NAME = "lure";
    static const string SOFT_NAME = "soft";

    //! -1, wenn der Name unbekannt ist. Ein unbekannter Wert weist den Record
    //! AB (21 §3.4) - ihn zu raten hiesse, ueber die Lebensdauer eines
    //! Spielerbesitzes zu wuerfeln.
    static int FromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToLower();

        if (n == LURE_NAME) return LURE;
        if (n == SOFT_NAME) return SOFT;
        return -1;
    }

    static string Name(int kind)
    {
        switch (kind)
        {
            case LURE: return LURE_NAME;
            case SOFT: return SOFT_NAME;
        }
        return "?";
    }

    static string ValidNames()
    {
        return LURE_NAME + ", " + SOFT_NAME;
    }
}

//------------------------------------------------------------------------------

/**
 * Code-Defaults und Grenzen des Teilsystems.
 *
 * Sie sind Konstanten und keine Einstellungen, weil jede von ihnen eine
 * FORMAT- oder ENGINE-Groesse beschreibt:
 *
 *   HOURLY_COEF_COUNT   FishYieldItemBase fuehrt genau 24 Koeffizienten, einen
 *                       je Stunde. Das ist die Feldbreite der Engine.
 *   MIN_WEIGHT_MULT     ein Faktor unter 1.0 waere kein Bonus mehr, sondern
 *                       eine Unterdrueckung - eine zweite Mechanik mit eigener
 *                       Balance (20 §8, F-07).
 *   DEFAULT_WEIGHT_MULT / DEFAULT_LURE_WEAR   die beiden Vorgaben aus 21 §3.1.
 *                       Wer sie anders will, schreibt sie in den Datensatz -
 *                       genau dafuer sind es Felder.
 *   MAX_WEIGHT_MULT     Deckel. Das Gewichtsarray der Engine bekommt je
 *                       Gewichtspunkt EINEN Eintrag (CatchingContextBase.c:
 *                       SetupProbabilityArray); ein Faktor ohne Deckel macht
 *                       aus einem Tippfehler eine Speicherfrage.
 */
class ChefZ_FishingLimits
{
    static const int   HOURLY_COEF_COUNT   = 24;
    static const float MIN_WEIGHT_MULT     = 1.0;
    static const float MAX_WEIGHT_MULT     = 100.0;
    static const float DEFAULT_WEIGHT_MULT = 3.0;
    static const float DEFAULT_LURE_WEAR   = 1.5;
    static const int   MAX_BASE_WEIGHT     = 10000;
}

//------------------------------------------------------------------------------

/**
 * Ein Fangertrag in Rohform (20 §4.1, 21 §2.1).
 *
 * Zwei Eigenheiten, die zusammengehoeren:
 *
 * 1. quality bleibt ABSICHTLICH ohne Code-Default. 21 §2.1 nennt zwar 0.35,
 *    aber diese Zahl gehoert der Engine (AnimalCatchingConstants.
 *    QUALITY_FISH_BASE, gesetzt in FishYieldItemBase.Init). Sie hier zu
 *    wiederholen hiesse, sie an zwei Stellen zu pflegen - und die zweite Stelle
 *    liegt in einem anderen Modul. Bleibt das Feld ungesetzt, ruehrt das
 *    Ertragsobjekt m_QualityBase nicht an und die Engine behaelt ihren Wert.
 *
 * 2. hourlyCoefs wird bei falscher Laenge VERWORFEN, nicht abgewiesen (20 §8).
 *    Eine halbe Tageskurve waere schlimmer als gar keine: die Engine liest mit
 *    der aktuellen Stunde in ein Feld fester Breite.
 */
class ChefZ_FishingYieldDef extends ChefZ_Record
{
    int               baseWeight;              // Vorkommensgewicht, > 0
    string            enviro;                  // POND | SEA | BOTH
    ref array<string> methods;                 // ROD | TRAP_LARGE | TRAP_SMALL
    bool              lureOnly;                // ohne passenden Koeder Gewicht 0
    ref array<float>  hourlyCoefs;             // genau 24 Werte oder nichts
    float             quality;                 // Sentinel = Engine-Vorgabe
    ref array<string> baitSensitivityAllow;    // nur fuer Reusen wirksam
    bool              overrideExisting;        // ersetzt einen gleichnamigen Eintrag

    //--- Ergebnis der COMPILE-Stufe, nicht aus JSON zu setzen ----------------
    private int m_EnviroMask;
    private int m_MethodMask;
    private int m_TypeHash;

    void ChefZ_FishingYieldDef()
    {
        baseWeight           = ChefZ_Undefined.INT;
        enviro               = ChefZ_Undefined.TEXT;
        methods              = null;
        hourlyCoefs          = null;
        quality              = ChefZ_Undefined.FLOAT;
        baitSensitivityAllow = null;

        m_EnviroMask = 0;
        m_MethodMask = 0;
        m_TypeHash   = 0;

        // bool ohne Sentinel: die Sonde traegt beide Schalter nach (02 E3).
        lureOnly         = ChefZ_RecordProbe.Bool();
        overrideExisting = ChefZ_RecordProbe.Bool();
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.FISHING_YIELD;
    }

    override void Normalize()
    {
        super.Normalize();
        enviro.TrimInPlace();
        ChefZ_TextList.TrimAll(methods);
        ChefZ_TextList.TrimAll(baitSensitivityAllow);
    }

    /**
     * 21 §2.4, Zeile fuer Zeile.
     *
     * Die Existenz der Klasse in CfgVehicles wird HIER NICHT geprueft: das
     * braucht g_Game und gehoert damit in 3_Game - dieselbe Aufteilung wie bei
     * ChefZ_StationDef gegen ChefZ_ProcessCompiler.
     */
    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (baseWeight <= 0)
        {
            if (ctx)
                ctx.Error(this, "Fangertrag ohne brauchbares \"baseWeight\" (" + baseWeight.ToString() + ") - abgewiesen. Gewicht 0 haelt den Eintrag aus der Gewichtstabelle und " + "taeuscht einen Datensatz vor, der nichts tut.");
            return false;
        }

        if (baseWeight > ChefZ_FishingLimits.MAX_BASE_WEIGHT)
        {
            if (ctx)
                ctx.Warn(this, "\"baseWeight\" " + baseWeight.ToString() + " ist auf " + ChefZ_FishingLimits.MAX_BASE_WEIGHT.ToString() + " geklemmt. Die Engine legt je Gewichtspunkt einen Eintrag im " + "Wahrscheinlichkeitsfeld an.");
            baseWeight = ChefZ_FishingLimits.MAX_BASE_WEIGHT;
        }

        int enviroMask = ChefZ_FishingMask.EnviroFromName(enviro);
        if (enviroMask == 0)
        {
            if (ctx)
                ctx.Error(this, "Unbekanntes \"enviro\": \"" + enviro + "\" - abgewiesen. Gueltig: " + ChefZ_FishingMask.ValidEnviroNames() + ". Ein stumm falsch einsortierter Eintrag ist schlimmer als ein " + "fehlender.");
            return false;
        }

        if (ChefZ_TextList.IsEmpty(methods))
        {
            if (ctx)
                ctx.Error(this, "\"methods\" fehlt oder ist leer - abgewiesen. Eine Verfahrensmaske von 0 " + "trifft nie zu; der Datensatz waere tot. Gueltig: " + ChefZ_FishingMask.ValidMethodNames() + ".");
            return false;
        }

        int methodMask = 0;
        for (int i = 0; i < methods.Count(); i++)
        {
            int bit = ChefZ_FishingMask.MethodFromName(methods.Get(i));
            if (bit == 0)
            {
                if (ctx)
                    ctx.Error(this, "Unbekanntes Verfahren \"" + methods.Get(i) + "\" - abgewiesen. Gueltig: " + ChefZ_FishingMask.ValidMethodNames() + ".");
                return false;
            }
            methodMask = methodMask | bit;
        }

        if (lureOnly && (methodMask & ChefZ_FishingMask.METHOD_TRAP_ANY) != 0)
        {
            if (ctx)
                ctx.Error(this, "\"lureOnly\" zusammen mit einem Reusenverfahren - abgewiesen. In einer " + "Reuse gibt es keinen Rutenkontext, der Multiplikator bleibt 1.0, das " + "Gewicht faellt auf 0 - der Eintrag existierte dort nie.");
            return false;
        }

        if (hourlyCoefs && hourlyCoefs.Count() != ChefZ_FishingLimits.HOURLY_COEF_COUNT)
        {
            if (ctx)
                ctx.Warn(this, "\"hourlyCoefs\" hat " + hourlyCoefs.Count().ToString() + " statt " + ChefZ_FishingLimits.HOURLY_COEF_COUNT.ToString() + " Werten - Feld ignoriert, die Vorgabe der Engine bleibt. Eine halbe " + "Tageskurve waere schlimmer als gar keine.");
            hourlyCoefs = null;
        }

        if (hourlyCoefs)
        {
            for (int k = 0; k < hourlyCoefs.Count(); k++)
            {
                float coef = hourlyCoefs.Get(k);
                if (coef < 0.0)
                    hourlyCoefs.Set(k, 0.0);
                else if (coef > 1.0)
                    hourlyCoefs.Set(k, 1.0);
            }
        }

        if (HasQuality())
        {
            if (quality <= 0.0 || quality > 1.0)
            {
                if (ctx)
                    ctx.Warn(this, "\"quality\" " + quality.ToString() + " liegt ausserhalb von (0,1] und wird geklemmt. Der Wert landet als " + "SetQuantityNormalized am gespawnten Objekt.");
                if (quality <= 0.0)
                    quality = 0.01;
                if (quality > 1.0)
                    quality = 1.0;
            }
        }

        return true;
    }

    override void Compile(ChefZ_CompileContext ctx)
    {
        super.Compile(ctx);

        m_EnviroMask = ChefZ_FishingMask.EnviroFromName(enviro);

        m_MethodMask = 0;
        if (methods)
        {
            for (int i = 0; i < methods.Count(); i++)
                m_MethodMask = m_MethodMask | ChefZ_FishingMask.MethodFromName(methods.Get(i));
        }

        // Vanilla adressiert Ertraege ausschliesslich ueber GetType().Hash()
        // (CatchYieldBank, CatchingContextBase). Der Hash wird hier EINMAL
        // gebildet und nicht bei jedem Gewichtsaufruf - 20 E6.
        m_TypeHash = id.Hash();
    }

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_FishingYieldDef s = ChefZ_FishingYieldDef.Cast(src);
        if (!s)
            return;

        baseWeight           = PatchInt(baseWeight, s.baseWeight, s, "baseWeight");
        enviro               = PatchText(enviro, s.enviro, s, "enviro");
        methods              = PatchStringArray(methods, s.methods, s, "methods");
        hourlyCoefs          = PatchFloatArray(hourlyCoefs, s.hourlyCoefs, s, "hourlyCoefs");
        quality              = PatchFloat(quality, s.quality, s, "quality");
        baitSensitivityAllow = PatchStringArray(baitSensitivityAllow, s.baitSensitivityAllow, s, "baitSensitivityAllow");
        lureOnly             = PatchBool(lureOnly, s.lureOnly, s, "lureOnly");
        overrideExisting     = PatchBool(overrideExisting, s.overrideExisting, s, "overrideExisting");
    }

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_FishingYieldDef o = ChefZ_FishingYieldDef.Cast(other);
        if (!o)
            return;
        if (lureOnly == o.lureOnly)
            MarkExplicit("lureOnly");
        if (overrideExisting == o.overrideExisting)
            MarkExplicit("overrideExisting");
    }

    /**
     * ResolveDefaults fuellt quality BEWUSST NICHT - siehe Klassenkopf.
     * baseWeight bekommt ebenfalls keinen Default: ein Ertrag ohne Gewicht ist
     * kein Datensatz mit Luecke, sondern ein Datensatz ohne Aussage, und den
     * weist Validate ab.
     */
    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        if (!HasExplicit("lureOnly"))
            lureOnly = false;
        if (!HasExplicit("overrideExisting"))
            overrideExisting = false;
    }

    //--------------------------------------------------------------------------

    //! Sagt dieser Datensatz zur Groesse ueberhaupt etwas? Erst der Text, dann
    //! der Wert - dieselbe Reihenfolge wie DefaultFloat, aus demselben Grund.
    bool HasQuality()
    {
        if (HasExplicit("quality"))
            return true;
        return !ChefZ_Undefined.IsFloatUndefined(quality);
    }

    int GetEnviroMask()
    {
        return m_EnviroMask;
    }

    int GetMethodMask()
    {
        return m_MethodMask;
    }

    int GetTypeHash()
    {
        return m_TypeHash;
    }

    bool UsesRod()
    {
        return (m_MethodMask & ChefZ_FishingMask.METHOD_ROD) != 0;
    }

    //! ref-Listen: Ganzersatz, nicht elementweise - dieselbe Lesart wie
    //! ChefZ_Record.PatchStringArray. Es gibt sie hier zusaetzlich, weil die
    //! Basis nur die string-Variante fuehrt.
    private static array<float> PatchFloatArray(array<float> current, array<float> incoming, ChefZ_Record src, string field)
    {
        if (incoming && src && src.MayReplace(field))
            return incoming;
        return current;
    }

    //! Nur fuer den Selbsttest.
    override static bool SelfCheck()
    {
        ChefZ_RecordProbe.Reset();

        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        // Vollstaendiger Datensatz.
        ChefZ_FishingYieldDef ok = new ChefZ_FishingYieldDef();
        ok.id         = "CHEFZ_FY_A";
        ok.baseWeight = 42;
        ok.enviro     = ChefZ_FishingMask.ENVIRO_SEA_NAME;
        ok.methods    = new array<string>();
        ok.methods.Insert(ChefZ_FishingMask.METHOD_ROD_NAME);
        ok.methods.Insert(ChefZ_FishingMask.METHOD_TRAP_LARGE_NAME);
        ok.ResolveDefaults();
        if (!ok.Validate(ctx))                                          return false;
        ok.Compile(null);
        if (ok.GetEnviroMask() != ChefZ_FishingMask.ENVIRO_SEA)          return false;
        if (ok.GetMethodMask() != (ChefZ_FishingMask.METHOD_ROD | ChefZ_FishingMask.METHOD_TRAP_LARGE)) return false;
        if (ok.GetTypeHash() == 0)                                      return false;
        if (!ok.UsesRod())                                              return false;
        if (ok.lureOnly)                                                return false;
        if (ok.overrideExisting)                                        return false;
        if (ok.HasQuality())                                            return false;

        // Gewicht 0 wird abgewiesen.
        ChefZ_FishingYieldDef noWeight = new ChefZ_FishingYieldDef();
        noWeight.id      = "CHEFZ_FY_B";
        noWeight.enviro  = ChefZ_FishingMask.ENVIRO_POND_NAME;
        noWeight.methods = new array<string>();
        noWeight.methods.Insert(ChefZ_FishingMask.METHOD_ROD_NAME);
        if (noWeight.Validate(ctx))                                     return false;

        // Unbekanntes Gewaesser wird abgewiesen.
        ChefZ_FishingYieldDef badEnviro = new ChefZ_FishingYieldDef();
        badEnviro.id         = "CHEFZ_FY_C";
        badEnviro.baseWeight = 10;
        badEnviro.enviro     = "CHEFZ_FY_NIRGENDS";
        badEnviro.methods    = new array<string>();
        badEnviro.methods.Insert(ChefZ_FishingMask.METHOD_ROD_NAME);
        if (badEnviro.Validate(ctx))                                    return false;

        // Leere Verfahrensliste wird abgewiesen.
        ChefZ_FishingYieldDef noMethod = new ChefZ_FishingYieldDef();
        noMethod.id         = "CHEFZ_FY_D";
        noMethod.baseWeight = 10;
        noMethod.enviro     = ChefZ_FishingMask.ENVIRO_POND_NAME;
        if (noMethod.Validate(ctx))                                     return false;

        // lureOnly plus Reuse wird abgewiesen (21 §2.4).
        ChefZ_FishingYieldDef trapLure = new ChefZ_FishingYieldDef();
        trapLure.id         = "CHEFZ_FY_E";
        trapLure.baseWeight = 10;
        trapLure.enviro     = ChefZ_FishingMask.ENVIRO_SEA_NAME;
        trapLure.lureOnly   = true;
        trapLure.MarkExplicit("lureOnly");
        trapLure.methods    = new array<string>();
        trapLure.methods.Insert(ChefZ_FishingMask.METHOD_TRAP_LARGE_NAME);
        trapLure.ResolveDefaults();
        if (trapLure.Validate(ctx))                                     return false;

        // Falsch lange Tageskurve: Feld faellt weg, Datensatz bleibt.
        ChefZ_FishingYieldDef shortCurve = new ChefZ_FishingYieldDef();
        shortCurve.id          = "CHEFZ_FY_F";
        shortCurve.baseWeight  = 10;
        shortCurve.enviro      = ChefZ_FishingMask.ENVIRO_POND_NAME;
        shortCurve.methods     = new array<string>();
        shortCurve.methods.Insert(ChefZ_FishingMask.METHOD_ROD_NAME);
        shortCurve.hourlyCoefs = new array<float>();
        shortCurve.hourlyCoefs.Insert(1.0);
        if (!shortCurve.Validate(ctx))                                  return false;
        if (shortCurve.hourlyCoefs)                                     return false;

        // Groesse ausserhalb (0,1] wird geklemmt, nicht abgewiesen.
        ChefZ_FishingYieldDef bigQuality = new ChefZ_FishingYieldDef();
        bigQuality.id         = "CHEFZ_FY_G";
        bigQuality.baseWeight = 10;
        bigQuality.enviro     = ChefZ_FishingMask.ENVIRO_POND_NAME;
        bigQuality.methods    = new array<string>();
        bigQuality.methods.Insert(ChefZ_FishingMask.METHOD_ROD_NAME);
        bigQuality.quality    = 4.0;
        if (!bigQuality.Validate(ctx))                                  return false;
        if (bigQuality.quality != 1.0)                                  return false;
        if (!bigQuality.HasQuality())                                   return false;

        return true;
    }
}

//------------------------------------------------------------------------------

/**
 * Ein Koeder in Rohform (20 §4.1, 21 §3.1).
 *
 * Ein unbekanntes Ziel ist ausdruecklich nur eine WARNUNG und faellt einzeln
 * weg (21 §3.4): ein Koedermodul muss Ertraege aus einem Modul nennen duerfen,
 * das der Betreiber nicht geladen hat. Diese Pruefung braucht den Bestand der
 * anderen Art und sitzt deshalb in der ChefZ_FishingRegistry, nicht hier.
 */
class ChefZ_BaitDef extends ChefZ_Record
{
    string            baitKind;            // lure | soft
    ref array<string> targets;             // IDs der Ertraege, die er anhebt
    float             weightMultiplier;    // Sentinel = Core-Vorgabe
    float             lureWearPerUse;      // Sentinel = Core-Vorgabe, nur lure

    private int m_Kind;
    private int m_TypeHash;

    void ChefZ_BaitDef()
    {
        baitKind         = ChefZ_Undefined.TEXT;
        targets          = null;
        weightMultiplier = ChefZ_Undefined.FLOAT;
        lureWearPerUse   = ChefZ_Undefined.FLOAT;

        m_Kind     = -1;
        m_TypeHash = 0;
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.BAIT;
    }

    override void Normalize()
    {
        super.Normalize();
        baitKind.TrimInPlace();
        ChefZ_TextList.TrimAll(targets);
    }

    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (ChefZ_BaitKind.FromName(baitKind) < 0)
        {
            if (ctx)
                ctx.Error(this, "Unbekanntes \"baitKind\": \"" + baitKind + "\" - abgewiesen. Gueltig: " + ChefZ_BaitKind.ValidNames() + ". Davon haengt ab, ob der Gegenstand verbraucht wird oder " + "Schaden nimmt - das ist nichts zum Raten.");
            return false;
        }

        if (ChefZ_TextList.IsEmpty(targets))
        {
            if (ctx)
                ctx.Error(this, "\"targets\" fehlt oder ist leer - abgewiesen. Ein Koeder ohne Ziel hebt " + "nichts an und waere ein Datensatz ohne Wirkung.");
            return false;
        }

        if (HasWeightMultiplier() && weightMultiplier < ChefZ_FishingLimits.MIN_WEIGHT_MULT)
        {
            if (ctx)
                ctx.Warn(this, "\"weightMultiplier\" " + weightMultiplier.ToString() + " ist auf " + ChefZ_FishingLimits.MIN_WEIGHT_MULT.ToString() + " geklemmt. Ein Faktor unter 1.0 waere kein Bonus, sondern eine " + "Unterdrueckung - eine zweite Mechanik mit eigener Balance.");
            weightMultiplier = ChefZ_FishingLimits.MIN_WEIGHT_MULT;
        }

        if (HasWeightMultiplier() && weightMultiplier > ChefZ_FishingLimits.MAX_WEIGHT_MULT)
        {
            if (ctx)
                ctx.Warn(this, "\"weightMultiplier\" " + weightMultiplier.ToString() + " ist auf " + ChefZ_FishingLimits.MAX_WEIGHT_MULT.ToString() + " geklemmt. Die Engine legt je Gewichtspunkt einen Eintrag im " + "Wahrscheinlichkeitsfeld an.");
            weightMultiplier = ChefZ_FishingLimits.MAX_WEIGHT_MULT;
        }

        if (ChefZ_BaitKind.FromName(baitKind) == ChefZ_BaitKind.SOFT && HasLureWear())
        {
            if (ctx)
                ctx.Warn(this, "\"lureWearPerUse\" steht an einem Koeder der Art \"" + ChefZ_BaitKind.SOFT_NAME + "\" und wirkt dort nicht - er wird verbraucht, nicht abgenutzt. " + "Feld ignoriert.");
        }

        if (ChefZ_BaitKind.FromName(baitKind) == ChefZ_BaitKind.LURE && HasLureWear() && lureWearPerUse <= 0.0)
        {
            if (ctx)
                ctx.Warn(this, "\"lureWearPerUse\" ist " + lureWearPerUse.ToString() + " - ein Koeder ohne Verschleiss haelt ewig. Die Vorgabe " + ChefZ_FishingLimits.DEFAULT_LURE_WEAR.ToString() + " wird benutzt.");
            lureWearPerUse = ChefZ_FishingLimits.DEFAULT_LURE_WEAR;
        }

        return true;
    }

    override void Compile(ChefZ_CompileContext ctx)
    {
        super.Compile(ctx);
        m_Kind     = ChefZ_BaitKind.FromName(baitKind);
        m_TypeHash = id.Hash();
    }

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_BaitDef s = ChefZ_BaitDef.Cast(src);
        if (!s)
            return;

        baitKind         = PatchText(baitKind, s.baitKind, s, "baitKind");
        targets          = PatchStringArray(targets, s.targets, s, "targets");
        weightMultiplier = PatchFloat(weightMultiplier, s.weightMultiplier, s, "weightMultiplier");
        lureWearPerUse   = PatchFloat(lureWearPerUse, s.lureWearPerUse, s, "lureWearPerUse");
    }

    override void ResolveDefaults()
    {
        super.ResolveDefaults();
        weightMultiplier = DefaultFloat("weightMultiplier", weightMultiplier, ChefZ_FishingLimits.DEFAULT_WEIGHT_MULT);
        lureWearPerUse   = DefaultFloat("lureWearPerUse", lureWearPerUse, ChefZ_FishingLimits.DEFAULT_LURE_WEAR);
    }

    //--------------------------------------------------------------------------

    bool HasWeightMultiplier()
    {
        if (HasExplicit("weightMultiplier"))
            return true;
        return !ChefZ_Undefined.IsFloatUndefined(weightMultiplier);
    }

    bool HasLureWear()
    {
        if (HasExplicit("lureWearPerUse"))
            return true;
        return !ChefZ_Undefined.IsFloatUndefined(lureWearPerUse);
    }

    bool IsLure()
    {
        return m_Kind == ChefZ_BaitKind.LURE;
    }

    int GetKind()
    {
        return m_Kind;
    }

    int GetTypeHash()
    {
        return m_TypeHash;
    }

    //! Nur fuer den Selbsttest.
    override static bool SelfCheck()
    {
        ChefZ_RecordProbe.Reset();

        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        ChefZ_BaitDef ok = new ChefZ_BaitDef();
        ok.id       = "CHEFZ_BA_A";
        ok.baitKind = ChefZ_BaitKind.LURE_NAME;
        ok.targets  = new array<string>();
        ok.targets.Insert("CHEFZ_FY_A");
        ok.ResolveDefaults();
        if (!ok.Validate(ctx))                                          return false;
        ok.Compile(null);
        if (!ok.IsLure())                                               return false;
        if (ok.GetTypeHash() == 0)                                      return false;
        if (ok.weightMultiplier != ChefZ_FishingLimits.DEFAULT_WEIGHT_MULT) return false;
        if (ok.lureWearPerUse != ChefZ_FishingLimits.DEFAULT_LURE_WEAR) return false;

        ChefZ_BaitDef badKind = new ChefZ_BaitDef();
        badKind.id       = "CHEFZ_BA_B";
        badKind.baitKind = "CHEFZ_BA_UNFUG";
        badKind.targets  = new array<string>();
        badKind.targets.Insert("CHEFZ_FY_A");
        if (badKind.Validate(ctx))                                      return false;

        ChefZ_BaitDef noTarget = new ChefZ_BaitDef();
        noTarget.id       = "CHEFZ_BA_C";
        noTarget.baitKind = ChefZ_BaitKind.SOFT_NAME;
        if (noTarget.Validate(ctx))                                     return false;

        // Faktor unter 1.0 wird geklemmt, nicht abgewiesen.
        ChefZ_BaitDef weak = new ChefZ_BaitDef();
        weak.id               = "CHEFZ_BA_D";
        weak.baitKind         = ChefZ_BaitKind.SOFT_NAME;
        weak.weightMultiplier = 0.25;
        weak.targets          = new array<string>();
        weak.targets.Insert("CHEFZ_FY_A");
        if (!weak.Validate(ctx))                                        return false;
        if (weak.weightMultiplier != ChefZ_FishingLimits.MIN_WEIGHT_MULT) return false;
        if (weak.IsLure())                                              return false;

        return true;
    }
}
