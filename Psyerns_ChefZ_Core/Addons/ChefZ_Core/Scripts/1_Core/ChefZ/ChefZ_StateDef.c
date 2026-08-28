//==============================================================================
// ChefZ_StateDef - der Datensatz eines ChefZ-Zustands
//
// Entwurf: 06 §4.1 (Feldliste woertlich), 06 §3 (Projektionsregel), 06 §7
// (Fehlerverhalten), 06 E1/E3/E5/E6, 03 §4 (SYNC-RELEVANT: ausschliesslich
// Rang 1), 02 E3 (Sentinel und Bool-Sonde).
//
// Er stand bis S8 als Huelle in ChefZ_RecordTypes.c. Seit S9 traegt er
// Felder, die Verhalten steuern, und deshalb steht er wie ChefZ_RecipeDef in
// einer eigenen Datei - eine Huelle ist eine Zeile, ein Zustand ist ein
// Teilsystem.
//
// ---------------------------------------------------------------------------
// Was hier bewusst NICHT steht
// ---------------------------------------------------------------------------
// Die UEBERGAENGE. Ein Zustandswechsel ist immer das Ergebnis eines Prozesses
// oder eines Rezepts (06 E6); er steht als ChefZ_TransformDef beim Processing
// Manager (11). Eine zweite Registry fuer dieselbe Sache waere eine zweite
// Wahrheit und ein zweiter Ort, an dem ein Content-Autor suchen muesste.
//
// Und: KEIN CONTENT. Diese Datei definiert keinen einzigen Zustand. "SMOKED",
// "SALTED" oder "PREPARED" kommen hier nicht vor und duerfen es nie - sie sind
// Daten und stehen in CfgChefZStates eines Content-Moduls.
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

class ChefZ_StateDef extends ChefZ_Record
{
    //! Stringtable-Schluessel. Der Core zeigt nichts an - er reicht ihn durch.
    string displayName;

    /**
     * "Raw"|"Baked"|"Boiled"|"Dried"|"Burned"|"Rotten"|"" (06 §3).
     *
     * Wo eine Projektion deklariert ist, bleiben die VANILLA-Mechaniken
     * zustaendig: visual_properties, Naehrwertbasis, Agentenbereinigung,
     * CanProcessDecay()-Stopp bei ROTTEN, Kochsounds, Frostlogik. ChefZ baut
     * keine davon nach.
     */
    string projectsToVanillaStage;

    //! Tags, die dieser Zustand dem Item zusaetzlich gibt (05 E4). Die
    //! effektive Tagmenge eines Items entsteht im ChefZ_FactCollector aus
    //! Klasse + Zustand + Qualitaet, nicht hier.
    ref array<string> implies;

    //! Faktor auf die Verderbgeschwindigkeit (14). Sentinel -> 1.0.
    float spoilageMultiplier;

    /**
     * Lebensdauer der Frische in Sekunden (14).
     *
     * Bleibt bewusst auf dem Sentinel stehen, wenn niemand etwas sagt:
     * 06 §4.1 nennt als Default "aus CoreSettings", und die zustaendige
     * Einstellung entsteht mit S11/S14. Wuerde ResolveDefaults hier eine Zahl
     * setzen, waere danach nicht mehr unterscheidbar, ob ein Zustand eine
     * eigene Lebensdauer deklariert hat oder ob niemand etwas gesagt hat - und
     * der spaetere Serverwert koennte den Zustandswert nie mehr uebersteuern.
     * Dieselbe Ueberlegung wie bei ChefZ_IngredientDef.quantityUnit (05 E2).
     */
    float freshnessLifetimeSec;

    //! false bei Fehlzustaenden (06 §4.1 nennt BURNT/ROTTEN als Beispiel).
    //! Default true - ein Zustand ohne Angabe soll essbar bleiben.
    bool edible;

    //! true = kein Uebergang mehr erlaubt. 06 §7: ein angefragter Uebergang
    //! wird mit reason "terminal state" abgelehnt.
    bool terminal;

    //! true = setzt den Tag CHEFZ_PRESERVED und feuert OnChefZFoodPreserved
    //! (17). Der Core wertet das Feld selbst nicht aus.
    bool preserved;

    //--------------------------------------------------------------------------
    // COMPILE-Ergebnis (02 §6, 03 §5). Nicht aus JSON zu setzen.
    //--------------------------------------------------------------------------

    /**
     * projectsToVanillaStage als Zahl aus ChefZ_VanillaStage.
     *
     * ChefZ_VanillaStage.NONE (0) heisst "keine Projektion" - genau wie ein
     * leeres Feld. Das ist kein Zufall, sondern der Grund, warum die
     * Fehlerbehandlung aus 06 §7 ("unbekannter Name -> keine Projektion")
     * ohne einen zweiten Merker auskommt.
     */
    int projectedStage;

    //--------------------------------------------------------------------------

    void ChefZ_StateDef()
    {
        displayName            = ChefZ_Undefined.TEXT;
        projectsToVanillaStage = ChefZ_Undefined.TEXT;
        implies                = null;

        spoilageMultiplier     = ChefZ_Undefined.FLOAT;
        freshnessLifetimeSec   = ChefZ_Undefined.FLOAT;

        projectedStage         = ChefZ_VanillaStage.NONE;

        // bool kennt keinen Sentinel: die Bool-Sonde traegt das Feld in
        // explicitFields[] nach, wenn es im JSON stand (siehe ChefZ_Record).
        // ResolveDefaults setzt danach den Code-Default fuer alles, was
        // NICHT dort steht.
        edible                 = ChefZ_RecordProbe.Bool();
        terminal               = ChefZ_RecordProbe.Bool();
        preserved              = ChefZ_RecordProbe.Bool();
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.STATE;
    }

    //--------------------------------------------------------------------------
    // NORMALIZE
    //--------------------------------------------------------------------------

    override void Normalize()
    {
        super.Normalize();
        displayName.TrimInPlace();
        projectsToVanillaStage.TrimInPlace();
        ChefZ_TextList.TrimAll(implies);
    }

    //--------------------------------------------------------------------------
    // VALIDATE
    //--------------------------------------------------------------------------

    /**
     * Kein Feld dieses Records kann den Datensatz zu Fall bringen.
     *
     * 06 §7 fuehrt fuer jeden Fehlerfall dieses Teilsystems eine WARNUNG mit
     * abgeschwaechter Wirkung auf, keine Abweisung - und das ist richtig
     * herum: ein abgewiesener Zustand nimmt jedem Item, das ihn traegt, seine
     * Identitaet, waehrend ein Zustand ohne Projektion nur weniger kann.
     *
     * Ein ungueltiger Enum-Wert in SetFoodStageType waere dagegen ein
     * Sync-Fehler (06 §7), deshalb wird der Name hier hart geprueft und im
     * Zweifel geloescht statt weitergereicht.
     */
    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (!ChefZ_Undefined.IsTextUndefined(projectsToVanillaStage))
        {
            int stage = ChefZ_VanillaStage.FromName(projectsToVanillaStage);
            if (stage < 0)
            {
                if (ctx)
                    ctx.Warn(this, "projectsToVanillaStage \"" + projectsToVanillaStage + "\" ist keine Vanilla-Garstufe. Der Zustand projiziert dann auf gar " + "nichts - Optik, Naehrwertbasis und Agentenbereinigung bleiben, wie " + "sie sind. Gueltig: " + ChefZ_VanillaStage.ValidNames() + ".");
                projectsToVanillaStage = ChefZ_Undefined.TEXT;
            }
        }

        if (!ChefZ_Undefined.IsFloatUndefined(spoilageMultiplier) && spoilageMultiplier <= 0.0)
        {
            if (ctx)
                ctx.Warn(this, "spoilageMultiplier ist " + spoilageMultiplier.ToString() + " und damit nicht positiv. Ein Faktor <= 0 hiesse \"verdirbt nie\" oder " + "\"verdirbt rueckwaerts\"; gemeint ist das nie. Es gilt 1.0 (neutral). " + "Wer \"verdirbt nicht\" will, sagt das an der Zutat ueber decays (05).");

            // Direkt 1.0 und NICHT der Sentinel: der Config Manager ruft
            // ResolveDefaults() VOR Validate() (ChefZ_ConfigManager.FillRegistry).
            // Ein hier gesetzter Sentinel wuerde danach von niemandem mehr
            // aufgeloest und bliebe als float.LOWEST auf dem Record stehen -
            // ein Verderbfaktor von minus unendlich, den niemand geschrieben hat.
            spoilageMultiplier = 1.0;
        }

        if (!ChefZ_Undefined.IsFloatUndefined(freshnessLifetimeSec) && freshnessLifetimeSec <= 0.0)
        {
            if (ctx)
                ctx.Warn(this, "freshnessLifetimeSec ist " + freshnessLifetimeSec.ToString() + " und damit nicht positiv. Der Wert wird ausgelassen; es gilt die " + "Servervorgabe.");

            // Hier ist der Sentinel richtig: er BEDEUTET "es gilt die
            // Servervorgabe", und ResolveDefaults laesst ihn bewusst stehen
            // (siehe Feld).
            freshnessLifetimeSec = ChefZ_Undefined.FLOAT;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // COMPILE
    //--------------------------------------------------------------------------

    /**
     * Der Name der Vanilla-Stufe wird EINMAL beim Boot zur Zahl.
     *
     * Zur Laufzeit steht die Projektion damit als int bereit und kostet im
     * heissen Pfad keinen Stringvergleich (03 E1).
     *
     * Die "implies"-Tags werden hier NICHT aufgeloest: sie muessen gegen die
     * Tag-Registry geprueft werden, und die kennt erst der
     * ChefZ_StateManager (3_Game). Ein hier internierter, aber nirgends
     * deklarierter Tag waere stiller toter Code (04 §6).
     */
    override void Compile(ChefZ_CompileContext ctx)
    {
        super.Compile(ctx);

        projectedStage = ChefZ_VanillaStage.NONE;
        if (ChefZ_Undefined.IsTextUndefined(projectsToVanillaStage))
            return;

        int stage = ChefZ_VanillaStage.FromName(projectsToVanillaStage);
        if (stage > 0)
            projectedStage = stage;
    }

    //--------------------------------------------------------------------------
    // MERGE (02 E3)
    //--------------------------------------------------------------------------

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_StateDef s = ChefZ_StateDef.Cast(src);
        if (!s)
            return;

        displayName            = PatchText(displayName, s.displayName, s, "displayName");
        projectsToVanillaStage = PatchText(projectsToVanillaStage, s.projectsToVanillaStage, s, "projectsToVanillaStage");
        implies                = PatchStringArray(implies, s.implies);
        spoilageMultiplier     = PatchFloat(spoilageMultiplier, s.spoilageMultiplier, s, "spoilageMultiplier");
        freshnessLifetimeSec   = PatchFloat(freshnessLifetimeSec, s.freshnessLifetimeSec, s, "freshnessLifetimeSec");
        edible                 = PatchBool(edible, s.edible, s, "edible");
        terminal               = PatchBool(terminal, s.terminal, s, "terminal");
        preserved              = PatchBool(preserved, s.preserved, s, "preserved");
    }

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_StateDef o = ChefZ_StateDef.Cast(other);
        if (!o)
            return;

        if (edible    == o.edible)      MarkExplicit("edible");
        if (terminal  == o.terminal)    MarkExplicit("terminal");
        if (preserved == o.preserved)   MarkExplicit("preserved");
    }

    //--------------------------------------------------------------------------
    // Nachbereitung
    //--------------------------------------------------------------------------

    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        // 1.0 = neutral. 0.0 waere ein stiller Totalausfall des Verderbs.
        spoilageMultiplier = DefaultFloat("spoilageMultiplier", spoilageMultiplier, 1.0);

        // freshnessLifetimeSec bleibt bewusst auf dem Sentinel - siehe Feld.

        if (!HasExplicit("edible"))
            edible = true;
        if (!HasExplicit("terminal"))
            terminal = false;
        if (!HasExplicit("preserved"))
            preserved = false;
    }

    //--------------------------------------------------------------------------
    // Abfragen
    //--------------------------------------------------------------------------

    //! true, wenn der Zustand ueberhaupt auf eine Vanilla-Garstufe projiziert.
    bool HasProjection()
    {
        return projectedStage > ChefZ_VanillaStage.NONE;
    }

    //! true, wenn der Record eine eigene Frischelebensdauer nennt. Sonst gilt
    //! die Servervorgabe (14).
    bool HasFreshnessLifetime()
    {
        return !ChefZ_Undefined.IsFloatUndefined(freshnessLifetimeSec);
    }

    string ToLine()
    {
        string s = id;
        if (HasProjection())
            s = s + " -> " + ChefZ_VanillaStage.Name(projectedStage);
        s = s + "  spoil=" + spoilageMultiplier.ToString();
        if (HasFreshnessLifetime())
            s = s + "  lifetime=" + freshnessLifetimeSec.ToString() + "s";
        if (!edible)
            s = s + "  ungeniessbar";
        if (terminal)
            s = s + "  terminal";
        if (preserved)
            s = s + "  konserviert";
        return s;
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest (S9).
    override static bool SelfCheck()
    {
        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        // 1. Defaults: essbar, nicht terminal, nicht konserviert, neutral.
        ChefZ_StateDef bare = new ChefZ_StateDef();
        bare.id = "CHEFZ_SELFTEST_STATE_A";
        bare.ResolveDefaults();
        if (!bare.edible)                                       return false;
        if (bare.terminal)                                      return false;
        if (bare.preserved)                                     return false;
        if (bare.spoilageMultiplier != 1.0)                     return false;
        if (bare.HasFreshnessLifetime())                        return false;
        if (!bare.Validate(ctx))                                return false;
        bare.Compile(null);
        if (bare.HasProjection())                               return false;

        // 2. Projektion: Name -> Zahl, einmal beim Compile.
        ChefZ_StateDef proj = new ChefZ_StateDef();
        proj.id = "CHEFZ_SELFTEST_STATE_B";
        proj.projectsToVanillaStage = "Dried";
        proj.ResolveDefaults();
        if (!proj.Validate(ctx))                                return false;
        proj.Compile(null);
        if (!proj.HasProjection())                              return false;
        if (proj.projectedStage != ChefZ_VanillaStage.DRIED)    return false;

        // 3. Unbekannte Stufe: WARN, Feld geleert, Record bleibt gueltig.
        ChefZ_StateDef bad = new ChefZ_StateDef();
        bad.id = "CHEFZ_SELFTEST_STATE_C";
        bad.projectsToVanillaStage = "Geraeuchert";
        bad.ResolveDefaults();
        if (!bad.Validate(ctx))                                 return false;
        if (bad.projectsToVanillaStage != "")                   return false;
        bad.Compile(null);
        if (bad.HasProjection())                                return false;

        // 4. Nicht positiver Multiplikator faellt auf neutral zurueck.
        // In der Reihenfolge des Config Managers: ResolveDefaults ZUERST,
        // Validate danach. Genau deshalb muss Validate den neutralen Wert
        // setzen und nicht den Sentinel - sonst bliebe er stehen.
        ChefZ_StateDef zero = new ChefZ_StateDef();
        zero.id = "CHEFZ_SELFTEST_STATE_D";
        zero.spoilageMultiplier = 0.0;
        zero.ResolveDefaults();
        if (!zero.Validate(ctx))                                return false;
        if (zero.spoilageMultiplier != 1.0)                     return false;

        // 5. bool ohne explicitFields wirkt nicht, mit wirkt es (02 E3).
        ChefZ_StateDef ex = new ChefZ_StateDef();
        ex.id = "CHEFZ_SELFTEST_STATE_E";
        ex.edible = false;
        ex.ResolveDefaults();
        if (!ex.edible)                                         return false;   // nicht explizit -> Default

        ChefZ_StateDef ex2 = new ChefZ_StateDef();
        ex2.id = "CHEFZ_SELFTEST_STATE_F";
        ex2.edible = false;
        ex2.MarkExplicit("edible");
        ex2.ResolveDefaults();
        if (ex2.edible)                                         return false;   // explizit -> bleibt

        // 6. Patch: nur gesetzte Felder wandern, der Rest bleibt.
        ChefZ_StateDef basis = new ChefZ_StateDef();
        basis.id = "CHEFZ_SELFTEST_STATE_G";
        basis.displayName = "alt";
        basis.spoilageMultiplier = 0.5;

        ChefZ_StateDef overlay = new ChefZ_StateDef();
        overlay.id = "CHEFZ_SELFTEST_STATE_G";
        overlay.displayName = "neu";
        basis.PatchFrom(overlay);
        if (basis.displayName != "neu")                         return false;
        if (basis.spoilageMultiplier != 0.5)                    return false;

        return true;
    }
}
