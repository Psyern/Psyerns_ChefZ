//==============================================================================
// ChefZ_StationDef - was eine Station ANBIETET
//
// Entwurf: 11 §2 (Feldliste woertlich), 11 E5 (Station kennt Prozesse,
// Transform kennt Station nur optional), 11 E6 (eigene Stationen statt
// Vanillas Smoking-Slots), 02 §2 (was der Client wissen muss, steht in der
// Game-Config), 02 E3.
//
// ---------------------------------------------------------------------------
// Die id IST der Klassenname
// ---------------------------------------------------------------------------
// 11 §2: "id == Klassenname der Station". Ein CfgChefZStations-Knoten heisst
// also genau so wie die CfgVehicles-Klasse, die er beschreibt. Deshalb prueft
// Validate() den Namen NICHT gegen CfgVehicles: das braucht g_Game und gehoert
// in den ChefZ_ProcessCompiler (3_Game) - dieselbe Aufteilung wie bei den
// Ergebnisklassen der Rezepte (08 §8 gegen ChefZ_RecipeCompiler).
//
// ---------------------------------------------------------------------------
// Warum die Richtung Station -> Prozess und nicht umgekehrt (11 E5)
// ---------------------------------------------------------------------------
// Die Station deklariert processes[]. Damit kann eine NEUE Station, die einen
// vorhandenen Prozess anbietet, sofort alles verarbeiten, wofuer ein Transform
// existiert - ohne dass ein einziger Transform angefasst wird. Die
// Gegenrichtung (ChefZ_TransformDef.stationsAllowed) ist der Sonderfall fuer
// Exklusivitaeten.
//
// KEIN CONTENT: kein Stationsname, keine Stationskategorie, kein Prozessname.
// "GRINDER" und "SMOKER" stehen im Content-Modul, nie hier.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_StationDef extends ChefZ_Record
{
    ref array<string> stationCategories;   // freie Gruppierung, vom Content vergeben
    ref array<string> processes;           // welche Prozesse sie anbietet
    int               parallelSlots;       // gleichzeitige Jobs
    float             speedMultiplier;
    bool              needsFuel;

    void ChefZ_StationDef()
    {
        stationCategories = null;
        processes         = null;
        parallelSlots     = ChefZ_Undefined.INT;
        speedMultiplier   = ChefZ_Undefined.FLOAT;

        // bool ohne Sentinel: die Bool-Sonde traegt "needsFuel" nach (02 E3).
        needsFuel         = ChefZ_RecordProbe.Bool();
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.STATION;
    }

    override void Normalize()
    {
        super.Normalize();
        ChefZ_TextList.TrimAll(stationCategories);
        ChefZ_TextList.TrimAll(processes);
    }

    /**
     * Hier wird NICHTS abgewiesen ausser einer fehlenden id (Basis).
     *
     * Eine Station ohne processes[] ist kein Datenfehler, sondern Deko - und
     * Deko ist ein zulaessiges Moebelstueck. Der Compiler meldet sie als WARN,
     * damit ein Content-Autor den vergessenen Eintrag findet, aber ein
     * Serverstart scheitert daran nicht (11 §7, erste Zeile: keine
     * Prozessdaten heisst "inerte Deko").
     */
    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (!ChefZ_Undefined.IsIntUndefined(parallelSlots) && parallelSlots < 0)
        {
            if (ctx)
                ctx.Error(this, "parallelSlots ist negativ (" + parallelSlots.ToString() + ") - abgewiesen.");
            return false;
        }

        return true;
    }

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_StationDef s = ChefZ_StationDef.Cast(src);
        if (!s)
            return;

        stationCategories = PatchStringArray(stationCategories, s.stationCategories);
        processes         = PatchStringArray(processes, s.processes);
        parallelSlots     = PatchInt(parallelSlots, s.parallelSlots, s, "parallelSlots");
        speedMultiplier   = PatchFloat(speedMultiplier, s.speedMultiplier, s, "speedMultiplier");
        needsFuel         = PatchBool(needsFuel, s.needsFuel, s, "needsFuel");
    }

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_StationDef o = ChefZ_StationDef.Cast(other);
        if (!o)
            return;
        if (needsFuel == o.needsFuel)
            MarkExplicit("needsFuel");
    }

    /**
     * Code-Defaults.
     *
     * parallelSlots = 1 und nicht 0: eine Station, die keinen einzigen Job
     * halten kann, waere unbenutzbar, und "der Autor hat es vergessen" ist der
     * weitaus haeufigere Fall als "er wollte sie stillegen". Zum Stillegen gibt
     * es disabled.
     *
     * speedMultiplier = 1.0 = neutral, aus demselben Grund wie bei
     * ChefZ_DeviceDef.qualityModifier: 0.0 waere ein stiller Totalausfall -
     * ein Job, der nie fertig wird.
     */
    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        parallelSlots   = ChefZ_Undefined.IntOr(parallelSlots, 1);
        speedMultiplier = ChefZ_Undefined.FloatOr(speedMultiplier, 1.0);

        if (parallelSlots < 1)
            parallelSlots = 1;
        if (parallelSlots > ChefZ_ProcessingLimits.MAX_PARALLEL_SLOTS)
            parallelSlots = ChefZ_ProcessingLimits.MAX_PARALLEL_SLOTS;

        if (speedMultiplier <= 0.0)
            speedMultiplier = 1.0;

        if (!HasExplicit("needsFuel"))
            needsFuel = false;
    }

    int ProcessCount()
    {
        return ChefZ_TextList.Count(processes);
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_RecordProbe.Reset();

        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        ChefZ_StationDef s = new ChefZ_StationDef();
        s.id = "CHEFZ_SD_A";
        if (!s.Validate(ctx))                                   return false;   // Deko ist erlaubt
        if (s.ProcessCount() != 0)                              return false;

        s.ResolveDefaults();
        if (s.parallelSlots != 1)                               return false;
        if (s.speedMultiplier != 1.0)                           return false;
        if (s.needsFuel)                                        return false;

        // Klemmung nach oben und unten.
        ChefZ_StationDef big = new ChefZ_StationDef();
        big.id            = "CHEFZ_SD_B";
        big.parallelSlots = 999;
        big.ResolveDefaults();
        if (big.parallelSlots != ChefZ_ProcessingLimits.MAX_PARALLEL_SLOTS)  return false;

        ChefZ_StationDef slow = new ChefZ_StationDef();
        slow.id              = "CHEFZ_SD_C";
        slow.speedMultiplier = 0.0;
        slow.ResolveDefaults();
        if (slow.speedMultiplier != 1.0)                        return false;

        ChefZ_StationDef neg = new ChefZ_StationDef();
        neg.id            = "CHEFZ_SD_D";
        neg.parallelSlots = -3;
        if (neg.Validate(ctx))                                  return false;

        return true;
    }
}
