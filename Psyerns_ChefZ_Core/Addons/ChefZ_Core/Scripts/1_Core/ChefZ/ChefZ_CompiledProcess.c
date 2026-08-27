//==============================================================================
// ChefZ_CompiledProcess / ChefZ_CompiledStation / ChefZ_CompiledTransform
//
// Entwurf: 11 §2 (Felder), 11 §5 (Datenfluss BOOT), 11 §6 (nach dem Build
// unveraenderlich), 11 §7 (Fehlerverhalten), 11 E5 (Station -> Prozess),
// 09 §4.1 (Spezifitaet), 07 §2.2 (dieselbe Trennung Rohform/kompiliert).
//
// ---------------------------------------------------------------------------
// Warum es diese Form gibt - dieselbe Antwort wie bei ChefZ_CompiledRecipe
// ---------------------------------------------------------------------------
// In der kompilierten Form gibt es keinen String mehr, keinen Sentinel und
// keinen Defaultzweig. Jede Prozesspruefung ist ein Symbolvergleich, jede
// Temperaturpruefung ein Zahlenvergleich. Der teure Teil - Namen aufloesen,
// Defaults einsetzen, Unsinn abweisen - ist beim Boot einmal passiert.
//
// Das ist hier noch wichtiger als beim Kochen: ActionCondition() laeuft am
// CLIENT bei jedem Zielwechsel des Fadenkreuzes. Eine Aufloesung von Strings
// an dieser Stelle waere in der Bildrate messbar.
//
// Nach dem Build wird an diesen Objekten NICHTS mehr veraendert. Alles
// Veraenderliche liegt im ChefZ_ProcessContext, im ChefZ_FactSnapshot und im
// ChefZ_TransformMatch.
//
// KEIN CONTENT.
//
// Layer: 1_Core.
//==============================================================================

/**
 * Ein Prozess, fertig kompiliert.
 *
 * Die beiden Temperaturgrenzen sind hier als "hat / hat nicht" gefuehrt statt
 * als Sentinel: ein Sentinelvergleich im heissen Pfad ist ein
 * Gleitkommavergleich gegen float.LOWEST, und der ist weder schneller noch
 * lesbarer als ein bool.
 */
class ChefZ_CompiledProcess
{
    ChefZ_Sym processSym;
    string    id;
    string    sourceRef;

    int       exec;                 // ChefZ_ProcessExec.*

    //! ODER-verknuepft: EINE dieser Gruppen genuegt (11 §2). Leer heisst
    //! "kein Werkzeug noetig" - nicht "kein Werkzeug erlaubt".
    ref array<ChefZ_Sym> toolGroups;

    float     baseDurationSec;

    bool      hasMinTemperature;
    float     minTemperature;
    bool      hasMaxTemperature;
    float     maxTemperature;

    bool      requiresHeat;
    int       toolDamage;
    float     animationLength;
    float     specialty;

    //! Stringtable-Schluessel des Aktionstexts. Leer heisst: die Aktion nimmt
    //! ihren Rueckfalltext (#STR_CHEFZ_ACTION_PROCESS).
    string    displayName;

    //! Fuer den Core UNDURCHSICHTIG (17 E1): getragen und weitergereicht, nie
    //! gedeutet - genau deshalb Strings und keine Symbole.
    ref array<string> emitEvents;

    void ChefZ_CompiledProcess()
    {
        processSym        = ChefZ_SymbolTable.INVALID;
        id                = "";
        sourceRef         = "";
        exec              = ChefZ_ProcessExec.STATION_ACTION;
        toolGroups        = new array<ChefZ_Sym>();
        baseDurationSec   = 0.0;
        hasMinTemperature = false;
        minTemperature    = 0.0;
        hasMaxTemperature = false;
        maxTemperature    = 0.0;
        requiresHeat      = false;
        toolDamage        = 0;
        animationLength   = 0.0;
        specialty         = 0.0;
        displayName       = "";
        emitEvents        = new array<string>();
    }

    bool IsStationProcess()
    {
        return ChefZ_ProcessExec.IsStation(exec);
    }

    /**
     * Fuehrt der Handelnde ein passendes Werkzeug (11 §7)?
     *
     * ODER ueber die Gruppen: EINE genuegt. missingGroup traegt bei false die
     * ERSTE geforderte Gruppe - nicht alle. Sie ist die, die ein Spieler am
     * ehesten besorgen wuerde, und eine Aufzaehlung von fuenf Alternativen
     * hilft im Log niemandem.
     */
    bool HasTools(notnull ChefZ_ProcessContext ctx, out string missingGroup)
    {
        missingGroup = "";

        if (toolGroups.Count() == 0)
            return true;

        for (int i = 0; i < toolGroups.Count(); i++)
        {
            if (ctx.HasToolGroup(toolGroups.Get(i)))
                return true;
        }

        missingGroup = ChefZ_SymbolTable.NameOrMark(toolGroups.Get(0));
        return false;
    }

    /**
     * Stimmt die Umgebung (11 §4, ChefZ_MeetsEnvironment)?
     *
     * Bei false PAUSIERT ein STATION_TIMED-Job - er bricht nicht ab und laeuft
     * nicht zurueck (11 §7). Fuer STATION_ACTION heisst false schlicht "die
     * Aktion erscheint nicht".
     *
     * Die Reihenfolge ist nach Aussagekraft gewaehlt: "kein Brennstoff" ist
     * die nuetzlichste Antwort und steht deshalb vor der Temperatur - eine
     * Station ohne Brennstoff ist IMMER auch zu kalt, und "zu kalt" waere
     * dann die irrefuehrende Meldung.
     */
    bool MeetsEnvironment(notnull ChefZ_ProcessContext ctx, out string reason)
    {
        reason = "";

        if (!ctx.stationPowered)
        {
            reason = "die Station hat keinen Brennstoff";
            return false;
        }

        if (requiresHeat && !ctx.hasHeat)
        {
            reason = "der Prozess braucht eine Waermequelle, die Station ist kalt";
            return false;
        }

        if (hasMinTemperature && ctx.stationTemperature < minTemperature)
        {
            reason = "Temperatur " + ctx.stationTemperature.ToString() + " liegt unter "
                   + minTemperature.ToString();
            return false;
        }

        if (hasMaxTemperature && ctx.stationTemperature > maxTemperature)
        {
            reason = "Temperatur " + ctx.stationTemperature.ToString() + " liegt ueber "
                   + maxTemperature.ToString();
            return false;
        }

        return true;
    }

    string ToDebugString()
    {
        string s = id + "  " + ChefZ_ProcessExec.Name(exec)
                 + " dauer=" + baseDurationSec.ToString() + "s";
        if (toolGroups.Count() > 0)
            s = s + " werkzeug=[" + ChefZ_TextList.JoinSymbols(toolGroups, "|") + "]";
        if (requiresHeat)
            s = s + " +waerme";
        if (hasMinTemperature)
            s = s + " ab" + minTemperature.ToString();
        if (hasMaxTemperature)
            s = s + " bis" + maxTemperature.ToString();
        if (toolDamage > 0)
            s = s + " schaden=" + toolDamage.ToString();
        return s;
    }
}

//==============================================================================

/**
 * Eine Station, fertig kompiliert.
 *
 * processes[] enthaelt AUSSCHLIESSLICH Prozesse, die es wirklich gibt: der
 * Compiler wirft unbekannte Eintraege heraus und meldet sie als ERROR, laesst
 * die uebrigen aber stehen (11 §7). Eine Station mit drei Prozessen, von denen
 * einer falsch geschrieben ist, bietet danach zwei an - und das ist die
 * richtige Richtung: der Tippfehler kostet einen Prozess, nicht die Station.
 */
class ChefZ_CompiledStation
{
    ChefZ_Sym stationSym;
    string    id;
    string    sourceRef;

    ref array<ChefZ_Sym> categories;

    //! In DEKLARATIONSreihenfolge der Config. Der Index in dieser Liste ist
    //! der SYNCHRONISIERTE Prozessordinal der Station - siehe
    //! ChefZ_ProcessingStation_Base.
    ref array<ChefZ_Sym> processes;

    int       parallelSlots;
    float     speedMultiplier;
    bool      needsFuel;

    void ChefZ_CompiledStation()
    {
        stationSym      = ChefZ_SymbolTable.INVALID;
        id              = "";
        sourceRef       = "";
        categories      = new array<ChefZ_Sym>();
        processes       = new array<ChefZ_Sym>();
        parallelSlots   = 1;
        speedMultiplier = 1.0;
        needsFuel       = false;
    }

    bool Offers(ChefZ_Sym process)
    {
        if (!ChefZ_SymbolTable.IsValid(process))
            return false;
        return processes.Find(process) >= 0;
    }

    //! -1, wenn die Station den Prozess nicht anbietet.
    int OrdinalOf(ChefZ_Sym process)
    {
        return processes.Find(process);
    }

    //! INVALID bei einem Ordinal ausserhalb der Liste. Ein geratener Prozess
    //! waere hier besonders teuer: er entschiede, welcher Transform gesucht
    //! wird.
    ChefZ_Sym ProcessAt(int ordinal)
    {
        if (ordinal < 0 || ordinal >= processes.Count())
            return ChefZ_SymbolTable.INVALID;
        return processes.Get(ordinal);
    }

    bool HasCategory(ChefZ_Sym category)
    {
        return categories.Find(category) >= 0;
    }

    string ToDebugString()
    {
        return id + "  slots=" + parallelSlots.ToString()
             + " tempo=" + speedMultiplier.ToString()
             + " prozesse=[" + ChefZ_TextList.JoinSymbols(processes, ",") + "]"
             + " kategorien=[" + ChefZ_TextList.JoinSymbols(categories, ",") + "]";
    }
}

//==============================================================================

/**
 * Ein Transform, fertig kompiliert.
 *
 * inputs[] sind ChefZ_CompiledSlot - GENAU dieselbe Klasse, die ein Rezept
 * benutzt, aus genau demselben Compiler (11 E4). Der ChefZ_Matcher bindet sie
 * mit genau derselben Methode. Es gibt keinen zweiten Zuordnungsalgorithmus.
 */
class ChefZ_CompiledTransform
{
    ChefZ_Sym transformSym;
    string    id;
    string    sourceRef;

    ChefZ_Sym processSym;

    ref array<ref ChefZ_CompiledSlot> inputs;

    //! In ROHFORM uebernommen, wie beim Rezept: ein ChefZ_OutputDef besteht
    //! aus Klassennamen, Mengen und Schaltern, und der Applicator arbeitet
    //! ohnehin mit Klassennamen. Eine kompilierte Zwischenform waere eine
    //! Kopie ohne Gewinn - und eine zweite Stelle, an der ein Feld vergessen
    //! werden kann.
    ref array<ref ChefZ_OutputDef> outputs;
    ref array<ref ChefZ_OutputDef> byproducts;

    //! Leer heisst "jede Station, die den Prozess anbietet" (11 E5). Das ist
    //! der Normalfall; die Liste ist der Sonderfall fuer Exklusivitaeten.
    ref array<ChefZ_Sym> stationsAllowed;

    //! < 0 heisst "keine Angabe", dann gilt die Prozessdauer.
    float     durationOverrideSec;

    string    qualityRule;
    float     freshnessCarry;
    float     qualityDelta;

    int       priority;

    ref array<ref ChefZ_CapabilityReq> requires;

    //--- Erst beim Build berechnet -------------------------------------------

    //! Spezifitaet nach 09 §4.1, auf die Eingangsslots angewandt. Haengt NUR
    //! vom Transform ab, nie vom Stationsinhalt - deshalb einmal beim Boot.
    float     specificity;

    //! Wie viele Items muessen mindestens in der Station liegen.
    int       minItemCount;

    //! Reiner Zustandswechsel (11 §2): kein Output nennt eine Klasse. Der
    //! ChefZ_ProcessRunner nimmt dann seinen zweiten, verbrauchsfreien Pfad.
    bool      pureStateChange;

    void ChefZ_CompiledTransform()
    {
        transformSym        = ChefZ_SymbolTable.INVALID;
        id                  = "";
        sourceRef           = "";
        processSym          = ChefZ_SymbolTable.INVALID;
        inputs              = new array<ref ChefZ_CompiledSlot>();
        outputs             = new array<ref ChefZ_OutputDef>();
        byproducts          = new array<ref ChefZ_OutputDef>();
        stationsAllowed     = new array<ChefZ_Sym>();
        requires            = new array<ref ChefZ_CapabilityReq>();
        durationOverrideSec = -1.0;
        qualityRule         = "";
        freshnessCarry      = 1.0;
        qualityDelta        = 0.0;
        priority            = 0;
        specificity         = 0.0;
        minItemCount        = 0;
        pureStateChange     = false;
    }

    bool HasDurationOverride()
    {
        return durationOverrideSec >= 0.0;
    }

    /**
     * Darf dieser Transform an dieser Station laufen (11 E5)?
     *
     * Leere Liste heisst JA. Das ist die einzige Lesart, die zu E5 passt:
     * "eine neue Station, die PROCESS_DRY anbietet, kann sofort alles
     * trocknen, wofuer ein Trocknungs-Transform existiert".
     */
    bool AllowsStation(ChefZ_Sym stationClass)
    {
        if (stationsAllowed.Count() == 0)
            return true;
        return stationsAllowed.Find(stationClass) >= 0;
    }

    int RequiredInputCount()
    {
        int n = 0;
        for (int i = 0; i < inputs.Count(); i++)
        {
            ChefZ_CompiledSlot slot = inputs.Get(i);
            if (slot && !slot.optional && slot.minCount > 0)
                n++;
        }
        return n;
    }

    string ToDebugString()
    {
        string s = id
                 + "  prozess=" + ChefZ_SymbolTable.NameOrMark(processSym)
                 + " spez=" + specificity.ToString()
                 + " prio=" + priority.ToString()
                 + " eingaenge=" + inputs.Count().ToString()
                 + " minItems=" + minItemCount.ToString();
        if (pureStateChange)
            s = s + " ZUSTANDSWECHSEL";
        else
            s = s + " ergebnisse=" + outputs.Count().ToString();
        if (stationsAllowed.Count() > 0)
            s = s + " nur=[" + ChefZ_TextList.JoinSymbols(stationsAllowed, ",") + "]";
        if (HasDurationOverride())
            s = s + " dauer=" + durationOverrideSec.ToString() + "s";
        return s;
    }
}
