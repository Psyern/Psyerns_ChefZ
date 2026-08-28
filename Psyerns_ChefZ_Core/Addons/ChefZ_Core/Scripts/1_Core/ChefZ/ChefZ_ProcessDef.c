//==============================================================================
// ChefZ_ProcessExec / ChefZ_ProcessingLimits / ChefZ_ProcessDef
//
// Entwurf: 11 §2 (Feldliste woertlich), 11 §3 (die drei Ausfuehrungsformen),
// 11 §7 (Fehlerverhalten), 11 E8 (Werkzeuggruppen sind Daten), 02 E3
// (Sentinel und Bool-Sonde), 19 S14.
//
// ---------------------------------------------------------------------------
// Was ein Prozess IST - und was er ausdruecklich nicht ist
// ---------------------------------------------------------------------------
// Ein Prozess ist ein VERB ohne Objekt: "mahlen", "trocknen", "raeuchern".
// Er sagt, WIE gearbeitet wird (Ausfuehrungsform, Dauer, Werkzeug, Waerme) -
// nie, WORAUS was wird. Das steht im ChefZ_TransformDef, und die Trennung ist
// der Grund, warum eine neue Station sofort alles trocknen kann, wofuer ein
// Trocknungs-Transform existiert (11 E5).
//
// KEIN CONTENT: hier steht kein Prozessname, keine Station, keine Zutat und
// kein Werkzeug. "HANDCRAFT", "STATION_ACTION" und "STATION_TIMED" sind die
// drei Ausfuehrungsformen aus 11 §3 - Vokabular des Core, keine Spielinhalte.
// "PROCESS_GRIND_MEAT" kommt in dieser Datei nicht vor und darf es nie.
//
// Layer: 1_Core.
//==============================================================================

/**
 * Die drei Ausfuehrungsformen aus 11 §3.
 *
 * Als Konstanten statt als enum, aus demselben Grund wie bei
 * ChefZ_Completion (08 §2): der Wert steht als String im JSON und in der
 * Game-Config. Ein enum faellt bei einem Tippfehler still auf 0 - hier waere
 * das HANDCRAFT und damit lautlos etwas anderes als gemeint.
 */
class ChefZ_ProcessExec
{
    static const int HANDCRAFT      = 0;    //! Vanillas Craftsystem (11 E2), max. 2 Eingaenge
    static const int STATION_ACTION = 1;    //! Spieler arbeitet aktiv an der Station
    static const int STATION_TIMED  = 2;    //! Station tickt ohne Spieler weiter

    static const string HANDCRAFT_NAME      = "HANDCRAFT";
    static const string STATION_ACTION_NAME = "STATION_ACTION";
    static const string STATION_TIMED_NAME  = "STATION_TIMED";

    //! -1, wenn der Name unbekannt ist. Ein unbekannter Wert weist den Prozess
    //! AB (11 §7) - er zu raten hiesse, eine Ausfuehrungsform zu waehlen, die
    //! niemand geschrieben hat.
    static int FromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToUpper();

        if (n == HANDCRAFT_NAME)      return HANDCRAFT;
        if (n == STATION_ACTION_NAME) return STATION_ACTION;
        if (n == STATION_TIMED_NAME)  return STATION_TIMED;
        return -1;
    }

    static string Name(int exec)
    {
        switch (exec)
        {
            case HANDCRAFT:      return HANDCRAFT_NAME;
            case STATION_ACTION: return STATION_ACTION_NAME;
            case STATION_TIMED:  return STATION_TIMED_NAME;
        }
        return "?";
    }

    static string ValidNames()
    {
        return HANDCRAFT_NAME + ", " + STATION_ACTION_NAME + ", " + STATION_TIMED_NAME;
    }

    //! Laeuft dieser Prozess an einer ChefZ_ProcessingStation_Base?
    static bool IsStation(int exec)
    {
        return exec == STATION_ACTION || exec == STATION_TIMED;
    }
}

//------------------------------------------------------------------------------

/**
 * Die Grenzen des Verarbeitungssystems.
 *
 * Sie sind Konstanten und keine Einstellungen, weil jede von ihnen eine
 * ENGINE- oder PROTOKOLLgrenze beschreibt und keine Balancingzahl:
 *
 *   HANDCRAFT_MAX_INPUTS  Vanillas RecipeBase kennt genau zwei Zutaten
 *                         (01 V12). Das ist keine Vorliebe, das ist die
 *                         Signatur von RecipeBase.Do().
 *   MAX_PARALLEL_SLOTS    obere Schranke fuer parallelSlots. Jeder Slot ist
 *                         ein persistierter Job und eine Sync-Position.
 *   PROCESS_ORDINAL_MAX   Deckel des synchronisierten Prozessindex einer
 *                         Station (siehe ChefZ_ProcessingStation_Base).
 *   MIN_DURATION_SEC      unterhalb davon ist ein Stationsjob nicht mehr von
 *                         "sofort" zu unterscheiden, und ein Fortschrittsbalken
 *                         waere nur Flackern.
 */
class ChefZ_ProcessingLimits
{
    static const int   HANDCRAFT_MAX_INPUTS = 2;
    static const int   MAX_PARALLEL_SLOTS   = 8;
    static const int   PROCESS_ORDINAL_MAX  = 15;
    static const float MIN_DURATION_SEC     = 0.5;

    //! Knotenbudget der Transform-Bindung, wenn keines aus den Einstellungen
    //! kommt. Ein Transform hat ein bis zwei Eingaenge - das ist um
    //! Groessenordnungen weniger als ein Kesselrezept.
    static const int   DEFAULT_NODE_BUDGET  = 512;
}

//------------------------------------------------------------------------------

/**
 * Ein Verarbeitungsprozess in Rohform (11 §2).
 *
 * Zu den beiden Temperaturfeldern: 11 §2 sagt ausdruecklich "Sentinel = egal".
 * Deshalb ersetzt ResolveDefaults() sie NICHT - anders als bei jedem anderen
 * Zahlenfeld dieser Datei. Ein Default von 0.0 waere hier keine Vorgabe,
 * sondern eine Bedingung ("mindestens 0 Grad"), und die haette einen Prozess
 * im Winter unbemerkt pausieren lassen.
 */
class ChefZ_ProcessDef extends ChefZ_Record
{
    string            exec;               // "HANDCRAFT" | "STATION_ACTION" | "STATION_TIMED"
    ref array<string> toolGroups;         // ODER-verknuepft: eine genuegt
    float             baseDurationSec;
    float             minTemperature;     // Sentinel = egal
    float             maxTemperature;     // Sentinel = egal
    bool              requiresHeat;
    int               toolDamage;         // Health-Abzug am Werkzeug, 0..100
    float             animationLength;    // nur HANDCRAFT
    float             specialty;          // nur HANDCRAFT, Vanilla-Softskill
    string            displayName;        // Stringtable-Schluessel des Aktionstexts
    ref array<string> emitEvents;

    void ChefZ_ProcessDef()
    {
        exec            = ChefZ_Undefined.TEXT;
        displayName     = ChefZ_Undefined.TEXT;
        toolGroups      = null;
        emitEvents      = null;

        baseDurationSec = ChefZ_Undefined.FLOAT;
        minTemperature  = ChefZ_Undefined.FLOAT;
        maxTemperature  = ChefZ_Undefined.FLOAT;
        animationLength = ChefZ_Undefined.FLOAT;
        specialty       = ChefZ_Undefined.FLOAT;
        toolDamage      = ChefZ_Undefined.INT;

        // bool ohne Sentinel: die Bool-Sonde traegt "requiresHeat" in
        // explicitFields[] nach, wenn es in der Datei stand (02 E3).
        requiresHeat    = ChefZ_RecordProbe.Bool();
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.PROCESS;
    }

    //--------------------------------------------------------------------------

    override void Normalize()
    {
        super.Normalize();
        exec.TrimInPlace();
        exec.ToUpper();
        displayName.TrimInPlace();
        ChefZ_TextList.TrimAll(toolGroups);
        ChefZ_TextList.TrimAll(emitEvents);
    }

    /**
     * 11 §7: eine unbekannte Ausfuehrungsform weist den Prozess AB.
     *
     * Nicht "nimm STATION_ACTION": jede der drei Formen hat einen anderen
     * Traeger, eine andere Persistenz und ein anderes Abbruchverhalten. Die
     * falsche zu raten hiesse, einen Raeuchervorgang als Handgriff auszugeben.
     */
    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (ChefZ_Undefined.IsTextUndefined(exec))
        {
            if (ctx)
                ctx.Error(this, "Prozess ohne \"exec\" - abgewiesen. Ohne Ausfuehrungsform " + "ist nicht entscheidbar, ob er als Handgriff, als Stationsaktion oder " + "als Stationstimer laeuft. Gueltig: " + ChefZ_ProcessExec.ValidNames() + ".");
            return false;
        }

        if (ChefZ_ProcessExec.FromName(exec) < 0)
        {
            if (ctx)
                ctx.Error(this, "Prozess nennt die unbekannte Ausfuehrungsform \"" + exec + "\" - abgewiesen. Gueltig: " + ChefZ_ProcessExec.ValidNames() + ".");
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------------------

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_ProcessDef s = ChefZ_ProcessDef.Cast(src);
        if (!s)
            return;

        exec            = PatchText(exec, s.exec, s, "exec");
        displayName     = PatchText(displayName, s.displayName, s, "displayName");
        toolGroups      = PatchStringArray(toolGroups, s.toolGroups);
        emitEvents      = PatchStringArray(emitEvents, s.emitEvents);

        baseDurationSec = PatchFloat(baseDurationSec, s.baseDurationSec, s, "baseDurationSec");
        minTemperature  = PatchFloat(minTemperature, s.minTemperature, s, "minTemperature");
        maxTemperature  = PatchFloat(maxTemperature, s.maxTemperature, s, "maxTemperature");
        animationLength = PatchFloat(animationLength, s.animationLength, s, "animationLength");
        specialty       = PatchFloat(specialty, s.specialty, s, "specialty");
        toolDamage      = PatchInt(toolDamage, s.toolDamage, s, "toolDamage");
        requiresHeat    = PatchBool(requiresHeat, s.requiresHeat, s, "requiresHeat");
    }

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_ProcessDef o = ChefZ_ProcessDef.Cast(other);
        if (!o)
            return;
        if (requiresHeat == o.requiresHeat)
            MarkExplicit("requiresHeat");
    }

    /**
     * Code-Defaults.
     *
     * minTemperature und maxTemperature bleiben ABSICHTLICH Sentinel - siehe
     * Klassenkommentar. Alles andere bekommt den Wert, der "keine Wirkung"
     * bedeutet: kein Werkzeugschaden, keine Animation, kein Softskill.
     */
    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        exec            = ChefZ_Undefined.TextOr(exec, "");
        displayName     = ChefZ_Undefined.TextOr(displayName, "");
        baseDurationSec = ChefZ_Undefined.FloatOr(baseDurationSec, 0.0);
        animationLength = ChefZ_Undefined.FloatOr(animationLength, 0.0);
        specialty       = ChefZ_Undefined.FloatOr(specialty, 0.0);
        toolDamage      = ChefZ_Undefined.IntOr(toolDamage, 0);

        if (!HasExplicit("requiresHeat"))
            requiresHeat = false;
    }

    //--------------------------------------------------------------------------

    bool HasMinTemperature()
    {
        return !ChefZ_Undefined.IsFloatUndefined(minTemperature);
    }

    bool HasMaxTemperature()
    {
        return !ChefZ_Undefined.IsFloatUndefined(maxTemperature);
    }

    int ExecMode()
    {
        return ChefZ_ProcessExec.FromName(exec);
    }

    //! Nur fuer den Selbsttest.
    override static bool SelfCheck()
    {
        ChefZ_RecordProbe.Reset();

        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        ChefZ_ProcessDef p = new ChefZ_ProcessDef();
        p.id = "CHEFZ_PD_A";
        if (p.Validate(ctx))                                        return false;   // ohne exec

        p.exec = "unfug";
        p.Normalize();
        if (p.Validate(ctx))                                        return false;   // unbekannt

        p.exec = "station_timed";
        p.Normalize();
        if (!p.Validate(ctx))                                       return false;
        if (p.ExecMode() != ChefZ_ProcessExec.STATION_TIMED)        return false;

        // Sentinel bleibt Sentinel: "egal" ist eine Aussage, 0.0 waere eine
        // Bedingung.
        if (p.HasMinTemperature())                                  return false;
        p.ResolveDefaults();
        if (p.HasMinTemperature())                                  return false;
        if (p.baseDurationSec != 0.0)                               return false;
        if (p.requiresHeat)                                         return false;

        // Bool-Sonde
        ChefZ_ProcessDef low  = new ChefZ_ProcessDef();
        ChefZ_ProcessDef high = new ChefZ_ProcessDef();
        low.requiresHeat  = true;
        high.requiresHeat = true;                   // gleich -> stand in der Datei
        low.CaptureExplicitBools(high);
        if (!low.HasExplicit("requiresHeat"))                       return false;
        low.ResolveDefaults();
        if (!low.requiresHeat)                                      return false;

        if (ChefZ_ProcessExec.FromName("HANDCRAFT") != ChefZ_ProcessExec.HANDCRAFT)  return false;
        if (ChefZ_ProcessExec.IsStation(ChefZ_ProcessExec.HANDCRAFT))                return false;
        if (!ChefZ_ProcessExec.IsStation(ChefZ_ProcessExec.STATION_ACTION))          return false;

        return true;
    }
}
