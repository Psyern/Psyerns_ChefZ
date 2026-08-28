//==============================================================================
// ChefZ_NutritionScope / ChefZ_NutritionDef - der Datensatz einer
//                                             Naehrwertangabe
//
// Entwurf: 13 §4 (Feldliste woertlich), 13 E4 (Auffindungsreihenfolge
// Klasse -> Kategorie -> Tag -> Vanilla), 13 §8 (Fehlerverhalten),
// 02 E3 (Sentinel und Bool-Sonde), 03 §5 (Kompilat statt Stringvergleich).
//
// Er stand bis S11 als Huelle in ChefZ_RecordTypes.c und traegt seit S12
// Felder - wie ChefZ_StateDef, ChefZ_QualityTierDef und ChefZ_PreservationDef
// bekommt er deshalb eine eigene Datei.
//
// ---------------------------------------------------------------------------
// Der wichtigste Satz zu dieser Klasse (13 §2, 13 E1)
// ---------------------------------------------------------------------------
// Was hier steht, wird NIE an ein Item geschrieben und NIE beim Verzehr
// angewandt. Es ist ausschliesslich die Eingangsgroesse der SOLLRECHNUNG des
// Startaudits (13 §5).
//
// Die tatsaechlichen Naehrwerte eines Gerichts stehen in der CfgVehicles-
// Definition seiner Ergebnisklasse - dort, wo die Engine sie erwartet, und
// nirgends sonst (13 E1). Ein Record hier verschiebt keinen einzigen
// Balancingwert; er erlaubt dem Audit lediglich zu sagen, was herauskommen
// SOLLTE.
//
// Wer sich das nicht klarmacht, baut irgendwann einen Record fuer ein
// Gericht und wundert sich, dass der Server ihn ignoriert. Er ignoriert ihn
// zu Recht.
//
// ---------------------------------------------------------------------------
// Warum es "perUnit" gibt
// ---------------------------------------------------------------------------
// Vanillas nutrition_properties sind Werte je 100 Mengeneinheiten - belegt in
// PlayerStomach.c: "float energy_per_unit = profile.GetEnergy() / 100".
// Ein ChefZ-Record kann dieselbe Lesart benutzen (perUnit = false, der
// Normalfall) oder ausdruecklich je REZEPTEINHEIT rechnen (perUnit = true).
//
// Zweiteres ist fuer Zutaten gedacht, die ein Rezept in Einheiten fordert
// statt in ganzen Stuecken (05 §6). Ohne den Schalter muesste ein Autor
// entweder pro Einheit falsch rechnen oder den Record fuer jede
// Portionsgroesse verdoppeln.
//
// ---------------------------------------------------------------------------
// Die ID IST das Ziel
// ---------------------------------------------------------------------------
// 13 §4: "id == Klassenname ODER Kategorie-/Tag-Symbol, je nach scope". Der
// Datensatz traegt also kein eigenes Zielfeld; sein "id" ist das Ziel und
// "scope" sagt, in welcher Tabelle nachgeschlagen wird - dieselbe Bauform wie
// bei ChefZ_PreservationDef.
//
// KEIN CONTENT: hier steht die Aussage "es gibt die Dimension Kategorie",
// nicht "es gibt die Kategorie X" (Invariante I3).
//
// Layer: 1_Core.
//==============================================================================

/**
 * Auf WAS bezieht sich eine Naehrwertangabe.
 *
 * Drei Dimensionen, und die Reihenfolge ist keine Aufzaehlung, sondern die
 * Auffindungsreihenfolge aus 13 E4:
 *
 *   CLASS     gewinnt immer. Wer eine einzelne Klasse abweichend belegt,
 *             meint genau sie.
 *   CATEGORY  ein Autor belegt eine Oberkategorie EINMAL und muss nicht jede
 *             Sorte einzeln pflegen.
 *   TAG       dasselbe quer zum Kategoriebaum.
 *
 * Und danach, ohne dass es hier einen Wert dafuer braucht: VANILLA. Jedes
 * Item ohne jede ChefZ-Deklaration bringt seine echten CfgVehicles-Werte mit
 * und ist damit als Zutat auditierbar (13 E4, letzter Absatz). Das ist der
 * Grund, warum ein leeres Nutrition-Verzeichnis kein Problem ist.
 */
class ChefZ_NutritionScope
{
    static const int UNKNOWN  = 0;
    static const int CLASS    = 1;
    static const int CATEGORY = 2;
    static const int TAG      = 3;

    //! Die Namen, wie sie im JSON stehen. Kleingeschrieben, weil 13 §4 sie so
    //! schreibt; FromName() akzeptiert jede Schreibweise.
    static const string NAME_CLASS    = "class";
    static const string NAME_CATEGORY = "category";
    static const string NAME_TAG      = "tag";

    //! UNKNOWN, wenn der Name keine Dimension bezeichnet.
    static int FromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToLower();

        if (n == NAME_CLASS)    return CLASS;
        if (n == NAME_CATEGORY) return CATEGORY;
        if (n == NAME_TAG)      return TAG;
        return UNKNOWN;
    }

    static string Name(int scope)
    {
        switch (scope)
        {
            case CLASS:    return NAME_CLASS;
            case CATEGORY: return NAME_CATEGORY;
            case TAG:      return NAME_TAG;
        }
        return "?";
    }

    static bool IsKnown(int scope)
    {
        return scope >= CLASS && scope <= TAG;
    }

    static string ValidNames()
    {
        return NAME_CLASS + ", " + NAME_CATEGORY + ", " + NAME_TAG;
    }

    //! Nur fuer den Selbsttest (S12).
    static bool SelfCheck()
    {
        if (FromName("class")     != CLASS)     return false;
        if (FromName("  Class ")  != CLASS)     return false;   // Trim + Case
        if (FromName("CATEGORY")  != CATEGORY)  return false;
        if (FromName("tag")       != TAG)       return false;
        if (FromName("")          != UNKNOWN)   return false;
        if (FromName("klasse")    != UNKNOWN)   return false;
        if (!IsKnown(CLASS))                    return false;
        if (IsKnown(UNKNOWN))                   return false;
        if (Name(TAG) != NAME_TAG)              return false;
        return true;
    }
}

//==============================================================================

class ChefZ_NutritionDef extends ChefZ_Record
{
    //--------------------------------------------------------------------------
    // 13 §4, Feldliste
    //--------------------------------------------------------------------------

    //! "class" | "category" | "tag". Leer => "class", weil der Klassenrecord
    //! der Normalfall ist: eine Kategorieangabe schreibt, wer viele Klassen
    //! auf einmal meint, und das ist die Ausnahme.
    string scope;

    //! nutrition_properties[0] .. [6]. Sentinel => 0.0.
    //!
    //! Ein fehlendes Feld ist ausdruecklich KEIN Fehler: 13 §8 laesst eine
    //! Zutat ohne Naehrwertdaten mit 0 in die Sollrechnung eingehen und nennt
    //! sie namentlich. Dieselbe Lesart gilt fuer ein einzelnes Feld - wer nur
    //! "energy" pflegt, bekommt eine Sollrechnung ueber Energie und keine
    //! ueber Wasser.
    float  fullness;
    float  energy;
    float  water;
    float  nutritionalIndex;
    float  toxicity;
    float  digestibility;

    //! true => die Werte gelten je REZEPTEINHEIT statt je ganzem Item
    //! (13 §4). Siehe Dateikopf.
    bool   perUnit;

    //--------------------------------------------------------------------------
    // COMPILE-Ergebnis (02 §6, 03 §5). Nicht aus JSON zu setzen.
    //--------------------------------------------------------------------------

    /**
     * scope als Zahl aus ChefZ_NutritionScope.
     *
     * Das ZIEL braucht kein eigenes Feld - es ist die ID des Records, und die
     * interniert bereits ChefZ_Record.Compile() nach "sym".
     */
    int scopeKind;

    //--------------------------------------------------------------------------

    void ChefZ_NutritionDef()
    {
        scope            = ChefZ_Undefined.TEXT;

        fullness         = ChefZ_Undefined.FLOAT;
        energy           = ChefZ_Undefined.FLOAT;
        water            = ChefZ_Undefined.FLOAT;
        nutritionalIndex = ChefZ_Undefined.FLOAT;
        toxicity         = ChefZ_Undefined.FLOAT;
        digestibility    = ChefZ_Undefined.FLOAT;

        scopeKind        = ChefZ_NutritionScope.UNKNOWN;

        // bool kennt keinen Sentinel: die Bool-Sonde traegt das Feld in
        // explicitFields[] nach, wenn es im JSON stand (siehe ChefZ_Record).
        perUnit          = ChefZ_RecordProbe.Bool();
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.NUTRITION;
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
     * scope. Dieselbe Begruendung wie bei ChefZ_PreservationDef - bei einem
     * unbekannten scope ist unbestimmt, ob die ID eine Klasse, eine Kategorie
     * oder einen Tag bezeichnet, und ein Rueckfall auf "class" saehe aus wie
     * eine wirksame Angabe, waehrend er auf gar nichts passte.
     *
     * Alles andere ist eine Zahl, und eine Zahl kann hier nichts kaputt
     * machen: sie geht in eine Sollrechnung ein, die NIE angewandt wird
     * (13 E1). Negative Werte sind ausdruecklich zulaessig - "toxicity" und
     * ein bewusst abziehender Kategorierecord sind beides sinnvolle Angaben.
     * Was nicht zulaessig ist, ist eine Zahl, die keine Zahl mehr ist; die
     * faengt der Manager mit ChefZ_NutritionVector.IsFinite ab (13 §8).
     */
    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (ChefZ_NutritionScope.FromName(scope) == ChefZ_NutritionScope.UNKNOWN)
        {
            if (ctx)
                ctx.Error(this, "scope \"" + scope + "\" ist keine bekannte Dimension - der " + "Record wird abgewiesen. Ohne scope ist unbestimmt, ob \"" + id + "\" eine Klasse, eine Kategorie oder einen Tag bezeichnet. Gueltig: " + ChefZ_NutritionScope.ValidNames() + ".");
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // COMPILE
    //--------------------------------------------------------------------------

    /**
     * Der scope-Name wird EINMAL beim Boot zur Zahl.
     *
     * Das ZIEL wird hier NICHT gegen eine Registry geprueft: Kategorien und
     * Tags kennt erst der ChefZ_CategoryManager in 3_Game. Die Pruefung steht
     * deshalb in ChefZ_NutritionManager.Build() - dieselbe Aufteilung wie bei
     * ChefZ_PreservationDef.
     */
    override void Compile(ChefZ_CompileContext ctx)
    {
        super.Compile(ctx);
        scopeKind = ChefZ_NutritionScope.FromName(scope);
    }

    //--------------------------------------------------------------------------
    // MERGE (02 E3)
    //--------------------------------------------------------------------------

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_NutritionDef s = ChefZ_NutritionDef.Cast(src);
        if (!s)
            return;

        scope            = PatchText(scope, s.scope, s, "scope");

        fullness         = PatchFloat(fullness,         s.fullness,         s, "fullness");
        energy           = PatchFloat(energy,           s.energy,           s, "energy");
        water            = PatchFloat(water,            s.water,            s, "water");
        nutritionalIndex = PatchFloat(nutritionalIndex, s.nutritionalIndex, s, "nutritionalIndex");
        toxicity         = PatchFloat(toxicity,         s.toxicity,         s, "toxicity");
        digestibility    = PatchFloat(digestibility,    s.digestibility,    s, "digestibility");

        perUnit          = PatchBool(perUnit, s.perUnit, s, "perUnit");
    }

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_NutritionDef o = ChefZ_NutritionDef.Cast(other);
        if (!o)
            return;

        if (perUnit == o.perUnit)
            MarkExplicit("perUnit");
    }

    //--------------------------------------------------------------------------
    // Nachbereitung
    //--------------------------------------------------------------------------

    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        scope            = ChefZ_Undefined.TextOr(scope, ChefZ_NutritionScope.NAME_CLASS);

        fullness         = ChefZ_Undefined.FloatOr(fullness,         0.0);
        energy           = ChefZ_Undefined.FloatOr(energy,           0.0);
        water            = ChefZ_Undefined.FloatOr(water,            0.0);
        nutritionalIndex = ChefZ_Undefined.FloatOr(nutritionalIndex, 0.0);
        toxicity         = ChefZ_Undefined.FloatOr(toxicity,         0.0);
        digestibility    = ChefZ_Undefined.FloatOr(digestibility,    0.0);

        if (!HasExplicit("perUnit"))
            perUnit = false;
    }

    //--------------------------------------------------------------------------
    // Abfragen
    //--------------------------------------------------------------------------

    /**
     * Die Werte dieses Records als Vektor.
     *
     * Der Vektor wird GEFUELLT und nicht zurueckgegeben: der Manager haelt
     * einen wiederverwendeten Puffer, und ein neuer Vektor je Zutat je Rezept
     * waere beim Audit ueber hundert Rezepte genau die Sorte Allokation, die
     * dieser Entwurf sonst ueberall vermeidet.
     */
    void FillVector(notnull ChefZ_NutritionVector outVec)
    {
        outVec.fullness         = fullness;
        outVec.energy           = energy;
        outVec.water            = water;
        outVec.nutritionalIndex = nutritionalIndex;
        outVec.toxicity         = toxicity;
        outVec.digestibility    = digestibility;
    }

    //! true, wenn der Record ueberhaupt eine Zahl beitraegt. Ein Record, der
    //! nur aus einer ID besteht, ist im Audit dasselbe wie kein Record - und
    //! soll deshalb auch dieselbe Meldung ausloesen (ZERO_INGREDIENT).
    bool HasAnyValue()
    {
        if (fullness         != 0.0) return true;
        if (energy           != 0.0) return true;
        if (water            != 0.0) return true;
        if (nutritionalIndex != 0.0) return true;
        if (toxicity         != 0.0) return true;
        if (digestibility    != 0.0) return true;
        return false;
    }

    //! Eine Zeile fuer den Dump. Bewusst KEIN override von
    //! ChefZ_Record.Describe(): das tut im ganzen Core kein Record, und eine
    //! einzelne Ausnahme machte die Dumpzeilen zwischen den Arten uneinheitlich.
    string DescribeValues()
    {
        return id + " scope=" + scope
             + " energie=" + energy.ToString()
             + " perUnit=" + perUnit.ToString();
    }

    //! Nur fuer den Selbsttest (S12).
    static bool SelfCheck()
    {
        if (!ChefZ_NutritionScope.SelfCheck())
            return false;

        // Sentinel -> 0.0, scope -> "class", perUnit -> false.
        ChefZ_NutritionDef d = new ChefZ_NutritionDef();
        d.id = "CHEFZ_NU_SELFCHECK";
        d.ResolveDefaults();
        if (d.scope != ChefZ_NutritionScope.NAME_CLASS)  return false;
        if (d.energy != 0.0)                             return false;
        if (d.perUnit)                                   return false;
        if (d.HasAnyValue())                             return false;

        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);
        if (!d.Validate(ctx))                            return false;

        d.Compile(null);
        if (d.scopeKind != ChefZ_NutritionScope.CLASS)   return false;

        // Unbekannter scope -> abgewiesen.
        ChefZ_NutritionDef bad = new ChefZ_NutritionDef();
        bad.id    = "CHEFZ_NU_BAD";
        bad.scope = "geschmack";
        bad.ResolveDefaults();
        if (bad.Validate(ctx))                           return false;

        // Werte landen im Vektor, und zwar feldrichtig.
        ChefZ_NutritionDef v = new ChefZ_NutritionDef();
        v.id               = "CHEFZ_NU_VEC";
        v.energy           = 450.0;
        v.water            =  12.0;
        v.fullness         =  30.0;
        v.nutritionalIndex =   7.0;
        v.toxicity         =   1.0;
        v.digestibility    =   2.0;
        v.ResolveDefaults();
        if (!v.HasAnyValue())                            return false;

        ChefZ_NutritionVector vec = new ChefZ_NutritionVector();
        v.FillVector(vec);
        if (vec.energy           != 450.0)               return false;
        if (vec.water            !=  12.0)               return false;
        if (vec.fullness         !=  30.0)               return false;
        if (vec.nutritionalIndex !=   7.0)               return false;
        if (vec.toxicity         !=   1.0)               return false;
        if (vec.digestibility    !=   2.0)               return false;

        return true;
    }
}
