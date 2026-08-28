//==============================================================================
// ChefZ_PreservationScope / ChefZ_PreservationDef - der Datensatz einer
//                                                   Haltbarkeitsregel
//
// Entwurf: 14 §5 (Feldliste woertlich), 14 §3 (die Produktkette des
// Multiplikators), 14 §8 (Fehlerverhalten, Zeile fuer Zeile), 14 E3 (Zustand
// ist die primaere Dimension), 14 E7 (stopsDecay und preventsRotten sind zwei
// Schalter, nicht einer), 02 E3 (Sentinel und Bool-Sonde).
//
// Er stand bis S10 als Huelle in ChefZ_RecordTypes.c und traegt seit S11
// Felder, die Verhalten steuern - wie ChefZ_StateDef und ChefZ_QualityTierDef
// bekommt er deshalb eine eigene Datei. Eine Huelle ist eine Zeile, eine
// Haltbarkeitsregel ist ein Teilsystem.
//
// ---------------------------------------------------------------------------
// Der wichtigste Satz zu dieser Klasse (14 E1)
// ---------------------------------------------------------------------------
// Was hier steht, ist ein FAKTOR auf Vanillas Verfallsgeschwindigkeit - keine
// eigene Verfallsrechnung. Es gibt bewusst kein Feld fuer eine Haltbarkeit in
// Sekunden, keinen Ersatz fuer GameConstants.DECAY_FOOD_*, keine eigene
// Zufallsstreuung und keinen eigenen Stufenuebergang. Der Verfall bleibt
// Vanillas Rechnung; ChefZ skaliert nur ihr delta (01 V9).
//
// Ein Multiplikator 0.25 heisst deshalb "vierfache Haltbarkeit" - unabhaengig
// von Garstufe und Nahrungsart, und ohne dass irgendjemand eine
// Vanilla-Konstante nachpflegen muss.
//
// ---------------------------------------------------------------------------
// Die ID IST das Ziel - und was das kostet
// ---------------------------------------------------------------------------
// 14 §5: "id == Zustand, Klasse, Kategorie, Tag oder Qualitaetsstufe, je nach
// scope". Der Datensatz traegt also kein eigenes Zielfeld; sein "id" ist das
// Ziel und "scope" sagt, in welcher Tabelle nachgeschlagen wird.
//
// EHRLICH BENANNTE FOLGE: die Registry haelt IDs eindeutig. Heissen eine
// Kategorie und ein Tag gleich, kann nur EINE Preservation-Regel dafuer
// existieren - die zweite wird beim Laden mit einem Fehler abgewiesen. Das ist
// kein stiller Ausfall (der Ladebericht nennt beide Herkuenfte), aber es ist
// eine echte Grenze. Sie hier zu umgehen hiesse, die ID vom Ziel zu entkoppeln
// und damit den Entwurf umzuschreiben.
//
// ---------------------------------------------------------------------------
// KEIN CONTENT
// ---------------------------------------------------------------------------
// Diese Datei definiert keine einzige Regel. "RAW", "SMOKED", "DRIED" oder
// "SALTED" kommen hier nicht vor und duerfen es nie - sie sind Daten und
// stehen in der Preservation.json eines Content-Moduls. Die Seedtabelle aus
// 14 §3 ist Content und gehoert dorthin, nicht in den Core.
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

/**
 * Die fuenf Dimensionen aus 14 §5.
 *
 * Als Konstanten und nicht als enum, aus demselben Grund wie bei
 * ChefZ_VanillaStage: der Wert wird mit einem int-Feld verglichen, das aus dem
 * COMPILE-Schritt kommt, und ein enum faende bei einem unbekannten Namen still
 * auf 0 zurueck. UNKNOWN ist hier ein ausdruecklicher Wert, kein Unfall.
 *
 * Und: das ist KEIN Content-Vokabular. Die Aussage lautet "es gibt die
 * Dimension Kategorie" - nicht "es gibt die Kategorie X" (Invariante I3, siehe
 * ChefZ_RecordKind).
 */
class ChefZ_PreservationScope
{
    static const int UNKNOWN  = 0;
    static const int STATE    = 1;
    static const int CLASS    = 2;
    static const int CATEGORY = 3;
    static const int TAG      = 4;
    static const int QUALITY  = 5;

    //! Die Namen, wie sie im JSON stehen. Kleingeschrieben, weil 14 §5 sie so
    //! schreibt; FromName() akzeptiert jede Schreibweise.
    static const string NAME_STATE    = "state";
    static const string NAME_CLASS    = "class";
    static const string NAME_CATEGORY = "category";
    static const string NAME_TAG      = "tag";
    static const string NAME_QUALITY  = "quality";

    //! UNKNOWN, wenn der Name keine Dimension bezeichnet.
    static int FromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToLower();

        if (n == NAME_STATE)    return STATE;
        if (n == NAME_CLASS)    return CLASS;
        if (n == NAME_CATEGORY) return CATEGORY;
        if (n == NAME_TAG)      return TAG;
        if (n == NAME_QUALITY)  return QUALITY;
        return UNKNOWN;
    }

    static string Name(int scope)
    {
        switch (scope)
        {
            case STATE:    return NAME_STATE;
            case CLASS:    return NAME_CLASS;
            case CATEGORY: return NAME_CATEGORY;
            case TAG:      return NAME_TAG;
            case QUALITY:  return NAME_QUALITY;
        }
        return "?";
    }

    static bool IsKnown(int scope)
    {
        return scope >= STATE && scope <= QUALITY;
    }

    //! Fuer Fehlermeldungen: "unbekannter scope X, gueltig sind: ...".
    static string ValidNames()
    {
        return NAME_STATE + ", " + NAME_CLASS + ", " + NAME_CATEGORY + ", "
             + NAME_TAG + ", " + NAME_QUALITY;
    }

    //! Nur fuer den Selbsttest (S11).
    static bool SelfCheck()
    {
        if (FromName("state")    != STATE)      return false;
        if (FromName("  State ") != STATE)      return false;   // Trim + Case
        if (FromName("CLASS")    != CLASS)      return false;
        if (FromName("category") != CATEGORY)   return false;
        if (FromName("tag")      != TAG)        return false;
        if (FromName("quality")  != QUALITY)    return false;
        if (FromName("")         != UNKNOWN)    return false;
        if (FromName("zustand")  != UNKNOWN)    return false;
        if (!IsKnown(STATE))                    return false;
        if (IsKnown(UNKNOWN))                   return false;
        if (Name(TAG) != NAME_TAG)              return false;
        return true;
    }
}

//==============================================================================

class ChefZ_PreservationDef extends ChefZ_Record
{
    /**
     * Harte Untergrenze fuer den Multiplikator EINES Datensatzes.
     *
     * 14 §8: "spoilageMultiplier <= 0 -> auf minDecayScale (0.01) geklemmt,
     * WARN. 0 waere Unsterblichkeit durch Tippfehler und der wirkungsvollste
     * Betreiberfehler ueberhaupt."
     *
     * Der Datensatz kennt die Servergrenzen nicht - minDecayScale und
     * maxDecayScale stehen in Core.json und werden vom ChefZ_PreservationManager
     * zusaetzlich angewandt (14 §3). Diese Konstante ist deshalb nur der
     * Notnagel fuer den Fall, den der Record allein entscheiden kann: eine
     * Null oder eine negative Zahl. Ihr Wert ist derselbe wie der
     * Vorgabewert von minDecayScale, damit beide Wege zum selben Ergebnis
     * kommen.
     *
     * Als Konstante und nicht als Einstellung: das ist kein Regler, sondern
     * die Grenze zwischen "sehr haltbar" und "verdirbt nie". Wer letzteres
     * will, sagt es mit stopsDecay - sichtbar, nicht durch eine Null.
     */
    static const float MIN_SPOILAGE_MULTIPLIER = 0.01;

    //--------------------------------------------------------------------------
    // 14 §5, Feldliste
    //--------------------------------------------------------------------------

    //! "state" | "class" | "category" | "tag" | "quality". Leer => "state",
    //! weil 14 E3 den Zustand zur primaeren Dimension erklaert und ein
    //! einzelner Zustandsrecord alles abdeckt, was je diesen Zustand annimmt.
    string scope;

    //! 1.0 = Vanilla, kleiner = haltbarer. Sentinel => 1.0.
    float  spoilageMultiplier;

    //! true => CanProcessDecay() liefert false. Der Verfall laeuft GAR NICHT,
    //! Vanillas m_DecayTimer wird nicht angefasst (Konserven, V2).
    bool   stopsDecay;

    //! true => der Verfall laeuft weiter, die Vanilla-Garstufe wechselt aber
    //! nicht auf ROTTEN. 14 E7: zwei getrennte Schalter, damit eine Konserve
    //! nicht rotten muss, ihr Zustand sich aber weiter veraendern darf - ohne
    //! dass der Core wissen muss, was eine Konserve ist.
    bool   preventsRotten;

    //! Der Multiplikator gilt NUR, solange die Umgebungstemperatur in diesem
    //! Bereich liegt. null = immer. Beide Grenzen sind einzeln optional
    //! (ChefZ_Range).
    ref ChefZ_Range environmentTemperature;

    /**
     * Zusaetzlicher Faktor, solange das Item am Spieler haengt. Sentinel =
     * Vanilla-Verhalten.
     *
     * EHRLICH BENANNTE PRAEZISIERUNG gegenueber 14 §5: das ist ein ZUSAETZLICHER
     * Faktor, kein Ersatz fuer Vanillas GameConstants.DECAY_RATE_ON_PLAYER.
     * Vanilla addiert diesen Bonus INNERHALB von Edible_Base.ProcessDecay auf
     * m_DecayDelta (01 V9); m_DecayDelta ist protected und entsteht erst,
     * nachdem ChefZ sein delta bereits uebergeben hat. Es gibt schlicht keinen
     * Punkt, an dem man ihn ersetzen koennte, ohne die ganze Methode
     * nachzubauen - und genau das verbietet 14 E1.
     *
     * Sentinel heisst deshalb woertlich, was 14 §5 sagt: kein zusaetzlicher
     * Faktor, also exakt Vanillas Spielerbonus.
     */
    float  onPlayerMultiplier;

    //--------------------------------------------------------------------------
    // COMPILE-Ergebnis (02 §6, 03 §5). Nicht aus JSON zu setzen.
    //--------------------------------------------------------------------------

    /**
     * scope als Zahl aus ChefZ_PreservationScope.
     *
     * Zur Laufzeit steht die Dimension damit als int bereit und kostet im
     * Verfallstakt keinen Stringvergleich (03 E1). Das ZIEL braucht kein
     * eigenes Feld - es ist die ID des Records, und die interniert bereits
     * ChefZ_Record.Compile() nach "sym".
     */
    int scopeKind;

    //--------------------------------------------------------------------------

    void ChefZ_PreservationDef()
    {
        scope                  = ChefZ_Undefined.TEXT;
        spoilageMultiplier     = ChefZ_Undefined.FLOAT;
        onPlayerMultiplier     = ChefZ_Undefined.FLOAT;
        environmentTemperature = null;

        scopeKind              = ChefZ_PreservationScope.UNKNOWN;

        // bool kennt keinen Sentinel: die Bool-Sonde traegt das Feld in
        // explicitFields[] nach, wenn es im JSON stand (siehe ChefZ_Record).
        stopsDecay             = ChefZ_RecordProbe.Bool();
        preventsRotten         = ChefZ_RecordProbe.Bool();
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.PRESERVATION;
    }

    //--------------------------------------------------------------------------
    // NORMALIZE
    //--------------------------------------------------------------------------

    override void Normalize()
    {
        super.Normalize();
        scope.TrimInPlace();
    }

    //--------------------------------------------------------------------------
    // VALIDATE
    //--------------------------------------------------------------------------

    /**
     * Genau EIN Feld kann den Datensatz zu Fall bringen: ein unbekannter
     * scope.
     *
     * Der Grund fuer die Ausnahme: bei jedem anderen Fehler ist die Absicht
     * des Autors noch erkennbar und eine abgeschwaechte Wirkung moeglich - ein
     * geklemmter Multiplikator wirkt weniger, ein verworfener Temperaturbereich
     * wirkt immer. Bei einem unbekannten scope ist sie es NICHT: der Record
     * traegt dann eine ID, von der niemand weiss, ob sie einen Zustand, eine
     * Klasse oder einen Tag bezeichnet. Ihn auf "state" zurueckfallen zu
     * lassen waere die schlimmste Variante - er wuerde still auf gar nichts
     * passen und saehe dabei aus wie eine wirksame Regel.
     *
     * Wichtig zur Reihenfolge: der Config Manager ruft ResolveDefaults() VOR
     * Validate() (ChefZ_ConfigManager.FillRegistry). Deshalb wird hier der
     * NEUTRALE Wert gesetzt und nicht der Sentinel - der bliebe sonst stehen
     * und waere ein Verderbfaktor von minus unendlich, den niemand geschrieben
     * hat.
     */
    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (ChefZ_PreservationScope.FromName(scope) == ChefZ_PreservationScope.UNKNOWN)
        {
            if (ctx)
                ctx.Error(this, "scope \"" + scope + "\" ist keine bekannte Dimension - der " + "Record wird abgewiesen. Ohne scope ist unbestimmt, ob \"" + id + "\" einen Zustand, eine Klasse, eine Kategorie, einen Tag oder eine " + "Qualitaetsstufe bezeichnet. Gueltig: " + ChefZ_PreservationScope.ValidNames() + ".");
            return false;
        }

        if (spoilageMultiplier <= 0.0)
        {
            if (ctx)
                ctx.Warn(this, "spoilageMultiplier ist " + spoilageMultiplier.ToString() + " und damit nicht positiv. Ein Faktor <= 0 hiesse \"verdirbt nie\" oder " + "\"verdirbt rueckwaerts\"; gemeint ist das nie, und eine Null waere der " + "wirkungsvollste Tippfehler ueberhaupt. Es gilt " + MIN_SPOILAGE_MULTIPLIER.ToString() + ". Wer echte Unsterblichkeit will, " + "setzt stopsDecay - dann steht es da.");
            spoilageMultiplier = MIN_SPOILAGE_MULTIPLIER;
        }

        if (!ChefZ_Undefined.IsFloatUndefined(onPlayerMultiplier) && onPlayerMultiplier <= 0.0)
        {
            if (ctx)
                ctx.Warn(this, "onPlayerMultiplier ist " + onPlayerMultiplier.ToString() + " und damit nicht positiv. Der Wert wird ausgelassen; am Spieler gilt " + "dann Vanillas eigener Bonus (DECAY_RATE_ON_PLAYER) unveraendert.");

            // Hier ist der Sentinel richtig: er BEDEUTET "Vanilla-Verhalten"
            // (14 §5), und ResolveDefaults laesst ihn bewusst stehen.
            onPlayerMultiplier = ChefZ_Undefined.FLOAT;
        }

        if (environmentTemperature && !environmentTemperature.IsValid())
        {
            if (ctx)
                ctx.Warn(this, "environmentTemperature " + environmentTemperature.ToDebugString() + " hat min > max und beschreibt damit einen leeren Bereich - die Regel " + "koennte nie greifen. Der Bereich wird verworfen; die Regel gilt " + "wieder bei jeder Temperatur.");
            environmentTemperature = null;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // COMPILE
    //--------------------------------------------------------------------------

    /**
     * Der scope-Name wird EINMAL beim Boot zur Zahl.
     *
     * Das ZIEL wird hier NICHT gegen eine Registry geprueft: Zustaende,
     * Kategorien, Tags und Qualitaetsstufen kennen erst die Manager in 3_Game,
     * und ein hier geprueftes Ziel waere entweder falsch geprueft oder gar
     * nicht. Die Pruefung steht deshalb in ChefZ_PreservationManager.Build()
     * (14 §8, Zeile "Record nennt unbekannten Zustand/Kategorie/Tag") -
     * dieselbe Aufteilung wie bei ChefZ_StateDef.implies und
     * ChefZ_QualityTierDef.grantsTags.
     */
    override void Compile(ChefZ_CompileContext ctx)
    {
        super.Compile(ctx);
        scopeKind = ChefZ_PreservationScope.FromName(scope);
    }

    //--------------------------------------------------------------------------
    // MERGE (02 E3)
    //--------------------------------------------------------------------------

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_PreservationDef s = ChefZ_PreservationDef.Cast(src);
        if (!s)
            return;

        scope              = PatchText(scope, s.scope, s, "scope");
        spoilageMultiplier = PatchFloat(spoilageMultiplier, s.spoilageMultiplier, s, "spoilageMultiplier");
        onPlayerMultiplier = PatchFloat(onPlayerMultiplier, s.onPlayerMultiplier, s, "onPlayerMultiplier");

        // Ganzersatz, nicht feldweise - dieselbe Ueberlegung wie beim
        // Gewichtsblock in ChefZ_CoreSettingsDef: ein Overlay, das einen
        // Temperaturbereich schreibt, meint diesen Bereich. Ein halb aus zwei
        // Dateien zusammengesetzter Bereich waere eine Grenze, die niemand
        // aufgeschrieben hat.
        if (s.environmentTemperature)
            environmentTemperature = s.environmentTemperature;

        stopsDecay     = PatchBool(stopsDecay, s.stopsDecay, s, "stopsDecay");
        preventsRotten = PatchBool(preventsRotten, s.preventsRotten, s, "preventsRotten");
    }

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_PreservationDef o = ChefZ_PreservationDef.Cast(other);
        if (!o)
            return;

        if (stopsDecay     == o.stopsDecay)      MarkExplicit("stopsDecay");
        if (preventsRotten == o.preventsRotten)  MarkExplicit("preventsRotten");
    }

    //--------------------------------------------------------------------------
    // Nachbereitung
    //--------------------------------------------------------------------------

    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        // 14 E3: der Zustand ist die primaere Dimension. Ein Record ohne
        // scope meint fast immer einen Zustand - und wenn nicht, faellt es
        // beim Zielabgleich im Manager sofort auf ("unbekannter Zustand").
        scope = ChefZ_Undefined.TextOr(scope, ChefZ_PreservationScope.NAME_STATE);

        // 1.0 = neutral. 0.0 waere ein stiller Totalausfall des Verderbs.
        spoilageMultiplier = ChefZ_Undefined.FloatOr(spoilageMultiplier, 1.0);

        // onPlayerMultiplier bleibt bewusst auf dem Sentinel - siehe Feld.

        if (!HasExplicit("stopsDecay"))
            stopsDecay = false;
        if (!HasExplicit("preventsRotten"))
            preventsRotten = false;
    }

    //--------------------------------------------------------------------------
    // Abfragen
    //--------------------------------------------------------------------------

    bool HasOnPlayerMultiplier()
    {
        return !ChefZ_Undefined.IsFloatUndefined(onPlayerMultiplier);
    }

    bool HasTemperatureGate()
    {
        return environmentTemperature != null && !environmentTemperature.IsUnbounded();
    }

    /**
     * Gilt die Regel bei dieser Umgebungstemperatur?
     *
     * @param temperature  ChefZ_Undefined.FLOAT heisst "unbekannt". Dann greift
     *        eine temperaturgebundene Regel NICHT - 02 §8: "jeder Fehler bewegt
     *        das System Richtung weniger ChefZ, nie Richtung falsches ChefZ".
     *        Eine Regel ohne Bereich ist davon unberuehrt.
     */
    bool AppliesAt(float temperature)
    {
        if (!HasTemperatureGate())
            return true;
        if (ChefZ_Undefined.IsFloatUndefined(temperature))
            return false;
        return environmentTemperature.Contains(temperature);
    }

    string ToLine()
    {
        string s = ChefZ_PreservationScope.Name(scopeKind) + " " + id
                 + "  verderb=" + spoilageMultiplier.ToString();

        if (stopsDecay)
            s = s + "  stopsDecay";
        if (preventsRotten)
            s = s + "  preventsRotten";
        if (HasOnPlayerMultiplier())
            s = s + "  amSpieler=" + onPlayerMultiplier.ToString();
        if (HasTemperatureGate())
            s = s + "  temp=" + environmentTemperature.ToDebugString();

        return s;
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest (S11).
    static bool SelfCheck()
    {
        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        // 1. Nackter Datensatz: Zustandsdimension, neutral in jeder Hinsicht.
        ChefZ_PreservationDef bare = new ChefZ_PreservationDef();
        bare.id = "CHEFZ_SELFTEST_PRES_A";
        bare.ResolveDefaults();
        if (bare.scope != ChefZ_PreservationScope.NAME_STATE)   return false;
        if (bare.spoilageMultiplier != 1.0)                     return false;
        if (bare.stopsDecay)                                    return false;
        if (bare.preventsRotten)                                return false;
        if (bare.HasOnPlayerMultiplier())                       return false;
        if (bare.HasTemperatureGate())                          return false;
        if (!bare.Validate(ctx))                                return false;
        bare.Compile(null);
        if (bare.scopeKind != ChefZ_PreservationScope.STATE)    return false;

        // 2. Unbekannter scope: abgewiesen, nicht stillschweigend "state".
        ChefZ_PreservationDef bad = new ChefZ_PreservationDef();
        bad.id    = "CHEFZ_SELFTEST_PRES_B";
        bad.scope = "zustand";
        bad.ResolveDefaults();
        if (bad.Validate(ctx))                                  return false;

        // 3. Nicht positiver Multiplikator wird geklemmt, nicht abgewiesen.
        //    In der Reihenfolge des Config Managers: ResolveDefaults ZUERST.
        ChefZ_PreservationDef zero = new ChefZ_PreservationDef();
        zero.id    = "CHEFZ_SELFTEST_PRES_C";
        zero.scope = ChefZ_PreservationScope.NAME_TAG;
        zero.spoilageMultiplier = 0.0;
        zero.ResolveDefaults();
        if (!zero.Validate(ctx))                                return false;
        if (zero.spoilageMultiplier != MIN_SPOILAGE_MULTIPLIER) return false;
        zero.Compile(null);
        if (zero.scopeKind != ChefZ_PreservationScope.TAG)      return false;

        // 4. Temperaturbereich: greift innerhalb, nicht ausserhalb, und bei
        //    unbekannter Temperatur ausdruecklich NICHT.
        ChefZ_PreservationDef cold = new ChefZ_PreservationDef();
        cold.id    = "CHEFZ_SELFTEST_PRES_D";
        cold.scope = ChefZ_PreservationScope.NAME_CATEGORY;
        cold.environmentTemperature = new ChefZ_Range();
        cold.environmentTemperature.Init(-50.0, 5.0);
        cold.ResolveDefaults();
        if (!cold.Validate(ctx))                                return false;
        if (!cold.HasTemperatureGate())                         return false;
        if (!cold.AppliesAt(0.0))                               return false;
        if (cold.AppliesAt(20.0))                               return false;
        if (cold.AppliesAt(ChefZ_Undefined.FLOAT))              return false;

        // 5. Leerer Temperaturbereich (min > max) wird verworfen, der Record
        //    bleibt gueltig und gilt danach wieder immer.
        ChefZ_PreservationDef swapped = new ChefZ_PreservationDef();
        swapped.id    = "CHEFZ_SELFTEST_PRES_E";
        swapped.scope = ChefZ_PreservationScope.NAME_QUALITY;
        swapped.environmentTemperature = new ChefZ_Range();
        swapped.environmentTemperature.Init(30.0, 10.0);
        swapped.ResolveDefaults();
        if (!swapped.Validate(ctx))                             return false;
        if (swapped.environmentTemperature)                     return false;
        if (!swapped.AppliesAt(999.0))                          return false;

        // 6. bool ohne explicitFields wirkt nicht, mit wirkt es (02 E3).
        ChefZ_PreservationDef ex = new ChefZ_PreservationDef();
        ex.id         = "CHEFZ_SELFTEST_PRES_F";
        ex.stopsDecay = true;
        ex.ResolveDefaults();
        if (ex.stopsDecay)                                      return false;

        ChefZ_PreservationDef ex2 = new ChefZ_PreservationDef();
        ex2.id         = "CHEFZ_SELFTEST_PRES_G";
        ex2.stopsDecay = true;
        ex2.MarkExplicit("stopsDecay");
        ex2.ResolveDefaults();
        if (!ex2.stopsDecay)                                    return false;

        // 7. Patch: nur gesetzte Felder wandern, der Rest bleibt.
        ChefZ_PreservationDef basis = new ChefZ_PreservationDef();
        basis.id                 = "CHEFZ_SELFTEST_PRES_H";
        basis.scope              = ChefZ_PreservationScope.NAME_STATE;
        basis.spoilageMultiplier = 0.25;

        ChefZ_PreservationDef overlay = new ChefZ_PreservationDef();
        overlay.id                 = "CHEFZ_SELFTEST_PRES_H";
        overlay.spoilageMultiplier = 0.5;
        basis.PatchFrom(overlay);
        if (basis.spoilageMultiplier != 0.5)                    return false;
        if (basis.scope != ChefZ_PreservationScope.NAME_STATE)  return false;

        return ChefZ_PreservationScope.SelfCheck();
    }
}
