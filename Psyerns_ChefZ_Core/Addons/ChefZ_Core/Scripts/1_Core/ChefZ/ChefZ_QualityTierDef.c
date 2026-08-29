//==============================================================================
// ChefZ_QualityTierDef - der Datensatz einer Qualitaetsstufe
//
// Entwurf: 12 §3 (Feldliste woertlich), 12 §2 (was Qualitaet bewirken KANN und
// was nicht), 12 §8 (Fehlerverhalten), 12 E3 (Stufen sind Records MIT
// Wirkungen, keine Beschriftung), 03 §4 (SYNC-RELEVANT: ausschliesslich
// Rang 1), 02 E3 (Sentinel und Bool-Sonde).
//
// Er stand bis S9 als Huelle in ChefZ_RecordTypes.c und traegt seit S10
// Felder, die Verhalten steuern - wie ChefZ_StateDef und ChefZ_RecipeDef
// bekommt er deshalb eine eigene Datei. Eine Huelle ist eine Zeile, eine
// Stufe ist ein Teilsystem.
//
// ---------------------------------------------------------------------------
// Der wichtigste Satz zu dieser Klasse (12 §2)
// ---------------------------------------------------------------------------
// Es gibt hier KEIN nutritionMultiplier-Feld, und das ist kein Versehen:
// PlayerStomach.AddToStomach holt das NutritionalProfile ueber Klasse x
// Foodstage aus CfgVehicles und uebergibt item = null (Befund 01 V6). Es gibt
// keinen Weg, Instanzdaten hineinzureichen. Ein Feld, das nichts bewirken
// kann, waere ein Versprechen an den Content-Autor, das der Core nicht halten
// kann.
//
// Der Ersatz ist die AUSBEUTE: yieldMultiplier und portionBonus. Ueber eine
// ganze Mahlzeit gerechnet ist das derselbe Balancinghebel, nur an der Stelle
// angesetzt, an der die Engine ihn zulaesst.
//
// ---------------------------------------------------------------------------
// Was hier bewusst NICHT steht
// ---------------------------------------------------------------------------
// Die aufgeloesten grantsTags-SYMBOLE. Sie muessen gegen die Tag-Registry
// geprueft werden, und die kennt erst der ChefZ_QualityManager (3_Game). Ein
// hier internierter, aber nirgends deklarierter Tag waere stiller toter Code -
// dieselbe Ueberlegung wie bei ChefZ_StateDef.implies (04 §6).
//
// Und: KEIN CONTENT. Diese Datei definiert keine einzige Stufe. "POOR",
// "SIMPLE" oder "PREMIUM" kommen hier nicht vor und duerfen es nie - sie sind
// Daten und stehen in CfgChefZQualityTiers eines Content-Moduls.
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

class ChefZ_QualityTierDef extends ChefZ_Record
{
    /**
     * Untergrenze fuer yieldMultiplier (12 §8: "yieldMultiplier <= 0 -> auf
     * 0.01 geklemmt, WARN").
     *
     * Als Konstante und nicht als Einstellung: das ist kein Regler, sondern
     * die Grenze zwischen "sehr wenig Ausbeute" und "das Rezept verbraucht
     * Zutaten und erzeugt nichts". Wer sie verschieben koennte, koennte
     * letzteres wieder einschalten.
     */
    static const float MIN_YIELD_MULTIPLIER = 0.01;

    //--------------------------------------------------------------------------
    // 12 §3, Feldliste
    //--------------------------------------------------------------------------

    //! Gruppenname (12 E4). Leer => der Vorgabesatz aus den CoreSettings.
    //! Vergleiche und Raenge gelten AUSSCHLIESSLICH innerhalb eines Satzes.
    string tierSet;

    //! Aufsteigend. Bestimmt Reihenfolge und Vergleiche innerhalb des Satzes.
    int    rank;

    //! Schwelle, ab der diese Stufe gilt. Darf negativ sein - 12 §3 nennt
    //! ausdruecklich minScore -99 fuer die unterste Stufe, damit die
    //! Zustandsstrafen aus 12 §4.2 ueberhaupt Platz nach unten haben.
    float  minScore;

    string displayName;             // Stringtable-Schluessel, der Core zeigt nichts an
    string colorHint;               // opaque, fuer die UI eines Content-Moduls

    //--------------------------------------------------------------------------
    // Wirkungen (12 E3). Preservation (14) und Portion System (15) lesen
    // dieselben Felder, statt eigene Stufenlogik zu fuehren.
    //--------------------------------------------------------------------------

    float  yieldMultiplier;         // wirkt auf Portionen und Quantity   -> 15
    int    portionBonus;            // additiv auf die Portionszahl       -> 15
    float  spoilageMultiplier;      // wirkt in 14

    //! Effekt-IDs, vollstaendig opaque. Der Core wertet sie NIE aus; sie
    //! wandern unveraendert ins ChefZ_OnRecipeCompleted-Event (17).
    ref array<string> grantsEffects;

    //! Tags, die ein Gericht dieser Stufe zusaetzlich traegt (12 E3). Der
    //! Griff fuer alles Weitere: ein Haendler-Mod filtert darauf, ohne dass
    //! ChefZ etwas ueber Handel wissen muss.
    ref array<string> grantsTags;

    //--------------------------------------------------------------------------
    // COMPILE-Ergebnis (02 §6, 03 §5). Nicht aus JSON zu setzen.
    //--------------------------------------------------------------------------

    //! tierSet als Symbol. INVALID heisst "kein Satz genannt" - der Manager
    //! setzt dann den Vorgabesatz ein. Er und nicht dieser Record, weil der
    //! Vorgabename eine Einstellung ist (Core.json) und kein Code-Wissen.
    ChefZ_Sym tierSetSym;

    //--------------------------------------------------------------------------

    void ChefZ_QualityTierDef()
    {
        tierSet            = ChefZ_Undefined.TEXT;
        displayName        = ChefZ_Undefined.TEXT;
        colorHint          = ChefZ_Undefined.TEXT;

        rank               = ChefZ_Undefined.INT;
        portionBonus       = ChefZ_Undefined.INT;

        minScore           = ChefZ_Undefined.FLOAT;
        yieldMultiplier    = ChefZ_Undefined.FLOAT;
        spoilageMultiplier = ChefZ_Undefined.FLOAT;

        grantsEffects      = null;
        grantsTags         = null;

        tierSetSym         = ChefZ_SymbolTable.INVALID;
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.QUALITY_TIER;
    }

    //--------------------------------------------------------------------------
    // NORMALIZE
    //--------------------------------------------------------------------------

    override void Normalize()
    {
        super.Normalize();
        tierSet.TrimInPlace();
        displayName.TrimInPlace();
        colorHint.TrimInPlace();
        ChefZ_TextList.TrimAll(grantsEffects);
        ChefZ_TextList.TrimAll(grantsTags);
    }

    //--------------------------------------------------------------------------
    // VALIDATE
    //--------------------------------------------------------------------------

    /**
     * Kein Feld dieses Records kann den Datensatz zu Fall bringen.
     *
     * 12 §8 fuehrt fuer jeden Fehlerfall dieses Teilsystems eine abgeschwaechte
     * Wirkung auf, keine Abweisung - und das ist richtig herum: eine
     * abgewiesene Stufe reisst ein Loch in die Stufenleiter, und ein Gericht
     * mit dieser Punktzahl faellt dann auf eine ganz andere Stufe. Ein
     * geklemmter Multiplikator kostet dagegen nur Genauigkeit.
     *
     * Wichtig zur Reihenfolge: der Config Manager ruft ResolveDefaults() VOR
     * Validate() (ChefZ_ConfigManager.FillRegistry). Deshalb wird hier der
     * NEUTRALE Wert gesetzt und nicht der Sentinel - der bliebe sonst stehen.
     */
    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (yieldMultiplier <= 0.0)
        {
            if (ctx)
                ctx.Warn(this, "yieldMultiplier ist " + yieldMultiplier.ToString() + " und damit nicht positiv. Ein Faktor <= 0 hiesse \"das Rezept " + "verbraucht die Zutaten und erzeugt nichts\"; gemeint ist das nie. " + "Es gilt " + MIN_YIELD_MULTIPLIER.ToString() + ".");
            yieldMultiplier = MIN_YIELD_MULTIPLIER;
        }

        if (spoilageMultiplier <= 0.0)
        {
            if (ctx)
                ctx.Warn(this, "spoilageMultiplier ist " + spoilageMultiplier.ToString() + " und damit nicht positiv. Ein Faktor <= 0 hiesse \"verdirbt nie\" oder " + "\"verdirbt rueckwaerts\"; gemeint ist das nie. Es gilt 1.0 (neutral).");
            spoilageMultiplier = 1.0;
        }

        if (rank < 0)
        {
            if (ctx)
                ctx.Warn(this, "rank ist " + rank.ToString() + " und damit negativ. Raenge " + "zaehlen ab 0 aufwaerts; ein negativer Rang haette in DegradeTier und " + "ShiftRank keine Entsprechung. Es gilt 0.");
            rank = 0;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // COMPILE
    //--------------------------------------------------------------------------

    /**
     * Der Stufensatz wird EINMAL beim Boot zum Symbol.
     *
     * Ein leerer Satzname bleibt bewusst INVALID: "kein Satz genannt" ist
     * etwas anderes als "der Satz mit dem leeren Namen", und nur der Manager
     * weiss, wie der Vorgabesatz heisst (Core.json, qualityScoring.
     * defaultTierSet).
     */
    override void Compile(ChefZ_CompileContext ctx)
    {
        super.Compile(ctx);

        tierSetSym = ChefZ_SymbolTable.INVALID;
        if (ChefZ_Undefined.IsTextUndefined(tierSet))
            return;

        if (ctx)
            tierSetSym = ctx.Intern(tierSet);
        else
            tierSetSym = ChefZ_SymbolTable.Intern(tierSet);
    }

    //--------------------------------------------------------------------------
    // MERGE (02 E3)
    //--------------------------------------------------------------------------

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_QualityTierDef s = ChefZ_QualityTierDef.Cast(src);
        if (!s)
            return;

        tierSet            = PatchText(tierSet, s.tierSet, s, "tierSet");
        displayName        = PatchText(displayName, s.displayName, s, "displayName");
        colorHint          = PatchText(colorHint, s.colorHint, s, "colorHint");

        rank               = PatchInt(rank, s.rank, s, "rank");
        portionBonus       = PatchInt(portionBonus, s.portionBonus, s, "portionBonus");

        minScore           = PatchFloat(minScore, s.minScore, s, "minScore");
        yieldMultiplier    = PatchFloat(yieldMultiplier, s.yieldMultiplier, s, "yieldMultiplier");
        spoilageMultiplier = PatchFloat(spoilageMultiplier, s.spoilageMultiplier, s, "spoilageMultiplier");

        grantsEffects      = PatchStringArray(grantsEffects, s.grantsEffects);
        grantsTags         = PatchStringArray(grantsTags, s.grantsTags);
    }

    //--------------------------------------------------------------------------
    // Nachbereitung
    //--------------------------------------------------------------------------

    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        rank               = DefaultInt("rank", rank, 0);
        portionBonus       = DefaultInt("portionBonus", portionBonus, 0);

        // 0.0 und nicht "sehr klein": eine Stufe ohne minScore ist die Stufe,
        // ab der es normal wird. Die Leiter beginnt bei null Punkten.
        minScore           = DefaultFloat("minScore", minScore, 0.0);

        // 1.0 = neutral. Eine Stufe ohne Angabe soll weder Ausbeute noch
        // Haltbarkeit veraendern - sie ist dann reine Beschriftung, und das
        // ist ein zulaessiger Entwurf.
        yieldMultiplier    = DefaultFloat("yieldMultiplier", yieldMultiplier, 1.0);
        spoilageMultiplier = DefaultFloat("spoilageMultiplier", spoilageMultiplier, 1.0);
    }

    //--------------------------------------------------------------------------
    // Abfragen
    //--------------------------------------------------------------------------

    //! Nennt der Record ueberhaupt einen Stufensatz?
    bool HasTierSet()
    {
        return !ChefZ_Undefined.IsTextUndefined(tierSet);
    }

    int GrantedTagCount()
    {
        return ChefZ_TextList.Count(grantsTags);
    }

    int GrantedEffectCount()
    {
        return ChefZ_TextList.Count(grantsEffects);
    }

    string ToLine()
    {
        string chefzTxt1 = id + " [" + ChefZ_SymbolTable.NameOrMark(tierSetSym) + "]" + " rang=";
        chefzTxt1 = chefzTxt1 + rank.ToString() + " ab=" + minScore.ToString() + " ausbeute=" + yieldMultiplier.ToString();
        string s = chefzTxt1;

        if (portionBonus != 0)
            s = s + " portionen+" + portionBonus.ToString();
        if (spoilageMultiplier != 1.0)
            s = s + " verderb=" + spoilageMultiplier.ToString();
        if (GrantedEffectCount() > 0)
            s = s + " effekte=[" + ChefZ_TextList.Join(grantsEffects, ",") + "]";
        if (GrantedTagCount() > 0)
            s = s + " tags=[" + ChefZ_TextList.Join(grantsTags, ",") + "]";

        return s;
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest (S10).
    override static bool SelfCheck()
    {
        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        // 1. Defaults: neutral in jeder Hinsicht.
        ChefZ_QualityTierDef bare = new ChefZ_QualityTierDef();
        bare.id = "CHEFZ_SELFTEST_TIER_A";
        bare.ResolveDefaults();
        if (bare.rank != 0)                                 return false;
        if (bare.minScore != 0.0)                           return false;
        if (bare.yieldMultiplier != 1.0)                    return false;
        if (bare.spoilageMultiplier != 1.0)                 return false;
        if (bare.portionBonus != 0)                         return false;
        if (bare.HasTierSet())                              return false;
        if (!bare.Validate(ctx))                            return false;
        bare.Compile(null);
        if (ChefZ_SymbolTable.IsValid(bare.tierSetSym))     return false;

        // 2. Nicht positive Multiplikatoren werden geklemmt, nicht abgewiesen.
        ChefZ_QualityTierDef bad = new ChefZ_QualityTierDef();
        bad.id = "CHEFZ_SELFTEST_TIER_B";
        bad.yieldMultiplier    = 0.0;
        bad.spoilageMultiplier = -2.0;
        bad.rank               = -3;
        // Die 0.0 muss als GESCHRIEBEN gelten, sonst ist sie seit
        // ChefZ_Undefined.FLOAT == 0.0 von einem fehlenden Feld nicht zu
        // unterscheiden und ResolveDefaults ersetzt sie durch die 1.0 -
        // dann klemmt Validate nie und die Zusicherung unten ist unpruefbar.
        // Aus einer Datei besorgt ChefZ_JsonExplicit diese Markierung; ein
        // von Hand gebauter Record hat kein explicitFields[] und muss sie
        // selbst setzen (siehe ChefZ_Record, Abschnitt Defaults).
        bad.MarkExplicit("yieldMultiplier");
        bad.ResolveDefaults();
        if (!bad.Validate(ctx))                             return false;
        if (bad.yieldMultiplier != MIN_YIELD_MULTIPLIER)    return false;
        if (bad.spoilageMultiplier != 1.0)                  return false;
        if (bad.rank != 0)                                  return false;

        // 3. Stufensatz wird beim Compile zum Symbol.
        ChefZ_QualityTierDef tierDef = new ChefZ_QualityTierDef();
        tierDef.id      = "CHEFZ_SELFTEST_TIER_C";
        tierDef.tierSet = "CHEFZ_SELFTEST_TIERSET";
        tierDef.ResolveDefaults();
        if (!tierDef.HasTierSet())                              return false;
        if (!tierDef.Validate(ctx))                             return false;
        tierDef.Compile(null);
        if (!ChefZ_SymbolTable.IsValid(tierDef.tierSetSym))     return false;
        if (tierDef.tierSetSym != ChefZ_SymbolTable.Lookup("CHEFZ_SELFTEST_TIERSET"))
            return false;

        // 4. Patch: nur gesetzte Felder wandern, der Rest bleibt.
        ChefZ_QualityTierDef basis = new ChefZ_QualityTierDef();
        basis.id              = "CHEFZ_SELFTEST_TIER_D";
        basis.yieldMultiplier = 1.5;
        basis.minScore        = 7.0;

        ChefZ_QualityTierDef overlay = new ChefZ_QualityTierDef();
        overlay.id       = "CHEFZ_SELFTEST_TIER_D";
        overlay.minScore = 9.0;
        basis.PatchFrom(overlay);
        if (basis.minScore != 9.0)                          return false;
        if (basis.yieldMultiplier != 1.5)                   return false;

        return true;
    }
}
