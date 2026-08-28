//==============================================================================
// ChefZ_ProcessContext / ChefZ_TransformMatch / ChefZ_ProcessJob
//
// Entwurf: 11 §4 (Feldlisten woertlich), 11 §5 (Datenfluss), 11 §6 (Zustand
// und Persistenzform), 11 E7 (Jobs an der Station), 03 E2 (persistiert wird
// der Hash, nie der Symbolzaehler), 07 §2.3 (ChefZ_ConsumePlan).
//
// ---------------------------------------------------------------------------
// Dieselbe Aufteilung wie beim Kochen
// ---------------------------------------------------------------------------
//   ChefZ_ProcessContext   "wo und womit wird gerade gearbeitet"
//                          - das Gegenstueck zu ChefZ_CookContext (08 §3)
//   ChefZ_FactSnapshot     "was liegt in der Station"    (unveraendert aus 05)
//   ChefZ_TransformMatch   "was soll geschehen"          - eine ANSAGE, keine
//                          Handlung; ausgefuehrt wird sie erst vom
//                          ChefZ_ProcessRunner (4_World)
//
// Alles hier ist REINE DATEN: kein ItemBase, kein EntityAI, kein Enum aus
// 4_World. Genau das macht die Transformauswertung ohne laufendes Spiel
// pruefbar - dieselbe Zusage, die der Matcher seit S5 haelt.
//
// ---------------------------------------------------------------------------
// Warum ChefZ_ProcessJob hier steht und nicht bei der Station
// ---------------------------------------------------------------------------
// 11 §4 fuehrt ihn im 4_World-Block auf, direkt bei
// ChefZ_ProcessingStation_Base. Er enthaelt aber ausschliesslich Zahlen -
// zwei Hashes, zwei Sekundenwerte, eine Identitaet - und keine einzige
// Engine-Berührung. In 1_Core ist damit die FORTSCHRITTSARITHMETIK ohne Welt
// pruefbar: "pausiert statt rueckwaerts" (11 §7) ist eine Rechnung, und eine
// Rechnung, die man nur auf einem laufenden Server pruefen kann, prueft
// niemand.
//
// Die Station in 4_World haelt die Jobs, speichert sie und tickt sie. Was ein
// Job IST, steht hier.
//
// KEIN CONTENT.
//
// Layer: 1_Core.
//==============================================================================

/**
 * Die Umgebung einer Transformauswertung (11 §4).
 *
 * Er beantwortet "an welcher Station, mit welchem Werkzeug, bei welcher
 * Temperatur" - getrennt von "was liegt drin", das der ChefZ_FactSnapshot
 * beantwortet.
 *
 * actorIdentityId ist der EINZIGE spielerbezogene Wert, und er wirkt
 * ausdruecklich NICHT auf die Bindung: welcher Transform passt, muss fuer
 * jeden Spieler dasselbe sein. Er wird ausschliesslich fuer Faehigkeiten
 * (17 §3.3) und fuer die Zuordnung eines laufenden Jobs gebraucht.
 */
class ChefZ_ProcessContext
{
    ChefZ_Sym stationClass;

    //! Kategorien der Station aus ChefZ_StationDef. Nie null.
    ref array<ChefZ_Sym> stationCategories;

    float     stationTemperature;
    bool      hasHeat;
    bool      stationPowered;

    //! Werkzeuggruppen, die der Handelnde fuehrt. Nie null.
    ref array<ChefZ_Sym> availableToolGroups;

    int       actorIdentityId;

    void ChefZ_ProcessContext()
    {
        stationCategories   = new array<ChefZ_Sym>();
        availableToolGroups = new array<ChefZ_Sym>();
        Reset();
    }

    /**
     * Auf "keine Station, nichts bekannt" zuruecksetzen.
     *
     * stationPowered ist TRUE in der Vorgabe, hasHeat FALSE - und das ist
     * kein Widerspruch, sondern zweimal dieselbe Regel: der Wert, der NICHTS
     * blockiert und NICHTS erlaubt, was Daten verlangen.
     *
     *   "nicht mit Strom versorgt" waere eine Behauptung ueber eine Station,
     *   die gar keinen Strom braucht - sie wuerde nie arbeiten.
     *   "Waerme vorhanden" waere eine Behauptung ueber eine kalte Station -
     *   sie wuerde raeuchern, ohne zu brennen.
     */
    void Reset()
    {
        stationClass       = ChefZ_SymbolTable.INVALID;
        stationCategories.Clear();
        stationTemperature = 0.0;
        hasHeat            = false;
        stationPowered     = true;
        availableToolGroups.Clear();
        actorIdentityId    = 0;
    }

    void AddStationCategory(ChefZ_Sym category)
    {
        if (!ChefZ_SymbolTable.IsValid(category))
            return;
        if (stationCategories.Find(category) >= 0)
            return;
        stationCategories.Insert(category);
    }

    void AddToolGroup(ChefZ_Sym group)
    {
        if (!ChefZ_SymbolTable.IsValid(group))
            return;
        if (availableToolGroups.Find(group) >= 0)
            return;
        availableToolGroups.Insert(group);
    }

    bool HasStationCategory(ChefZ_Sym category)
    {
        return stationCategories.Find(category) >= 0;
    }

    bool HasToolGroup(ChefZ_Sym group)
    {
        return availableToolGroups.Find(group) >= 0;
    }

    string ToDebugString()
    {
        string s = ChefZ_SymbolTable.NameOrMark(stationClass) + " [" + ChefZ_TextList.JoinSymbols(stationCategories, ",") + "]" + " temp=" + stationTemperature.ToString();
        if (hasHeat)
            s = s + " +waerme";
        if (!stationPowered)
            s = s + " OHNE-BRENNSTOFF";
        if (availableToolGroups.Count() > 0)
            s = s + " werkzeug=[" + ChefZ_TextList.JoinSymbols(availableToolGroups, ",") + "]";
        return s;
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        if (ChefZ_SymbolTable.IsValid(ctx.stationClass))    return false;
        if (!ctx.stationPowered)                            return false;
        if (ctx.hasHeat)                                    return false;

        ChefZ_Sym kat = ChefZ_SymbolTable.Intern("CHEFZ_PC_KAT");
        ctx.AddStationCategory(kat);
        ctx.AddStationCategory(kat);                        // keine Dublette
        if (ctx.stationCategories.Count() != 1)             return false;
        if (!ctx.HasStationCategory(kat))                   return false;
        ctx.AddStationCategory(ChefZ_SymbolTable.INVALID);
        if (ctx.stationCategories.Count() != 1)             return false;

        ChefZ_Sym tool = ChefZ_SymbolTable.Intern("CHEFZ_PC_TOOL");
        ctx.AddToolGroup(tool);
        if (!ctx.HasToolGroup(tool))                        return false;

        ctx.Reset();
        if (ctx.stationCategories.Count() != 0)             return false;
        if (ctx.availableToolGroups.Count() != 0)           return false;
        if (!ctx.stationPowered)                            return false;

        return true;
    }
}

//==============================================================================

/**
 * Das Ergebnis einer Transformsuche (11 §4).
 *
 * Wie ChefZ_MatchResult beim Kochen ist das eine ANSAGE und keine Handlung:
 * es beschreibt vollstaendig, was geschehen SOLL, und veraendert dabei nichts.
 * Ausgefuehrt wird es vom ChefZ_ProcessRunner, und der revalidiert vorher
 * jeden Handle (08 §6, Schritt 1).
 *
 * transformSym und processSym sind LAUFZEITSYMBOLE und duerfen niemals
 * persistiert werden (03 E2). Wer einen Job speichert, speichert die Hashes -
 * siehe ChefZ_ProcessJob.
 */
class ChefZ_TransformMatch
{
    bool      matched;

    ChefZ_Sym transformSym;
    string    transformId;        // fuer Log und Trace, ohne Symbolnachschlag
    ChefZ_Sym processSym;

    float     durationSec;

    //! slotId -> Handles, in DEKLARATIONSreihenfolge der Eingaenge (07 §4).
    ref map<string, ref array<int>> assignment;

    //! Alle gebundenen Handles, in Slotreihenfolge. Der Runner prueft sie vor
    //! der Ausfuehrung erneut.
    ref array<int> boundHandles;

    ref array<ref ChefZ_ConsumePlan> consumePlan;

    string    failReason;

    //--- Diagnose -------------------------------------------------------------
    int       candidatesTried;
    int       nodesExplored;

    void ChefZ_TransformMatch()
    {
        assignment   = new map<string, ref array<int>>();
        boundHandles = new array<int>();
        consumePlan  = new array<ref ChefZ_ConsumePlan>();
        Reset();
    }

    //! Listen und Map werden GELEERT, nicht neu angelegt - dieselbe
    //! Pooluberlegung wie bei ChefZ_MatchResult (08 §7).
    void Reset()
    {
        matched         = false;
        transformSym    = ChefZ_SymbolTable.INVALID;
        transformId     = "";
        processSym      = ChefZ_SymbolTable.INVALID;
        durationSec     = 0.0;
        assignment.Clear();
        boundHandles.Clear();
        consumePlan.Clear();
        failReason      = "";
        candidatesTried = 0;
        nodesExplored   = 0;
    }

    //! handles wird KOPIERT, nicht uebernommen: die Liste im
    //! ChefZ_SlotBinding gehoert dem Bindungsergebnis und wird beim naechsten
    //! Kandidaten zurueckgesetzt.
    void SetAssignment(string slotId, notnull array<int> handles)
    {
        array<int> copy = new array<int>();
        for (int i = 0; i < handles.Count(); i++)
            copy.Insert(handles.Get(i));
        assignment.Set(slotId, copy);
    }

    array<int> GetAssignment(string slotId)
    {
        array<int> handles;
        if (assignment.Find(slotId, handles))
            return handles;
        return null;
    }

    int BoundItemCount()
    {
        return boundHandles.Count();
    }

    string ToDebugString()
    {
        if (!matched)
        {
            string f = "kein Treffer";
            if (failReason != "")
                f = f + " (" + failReason + ")";
            return f + "  kandidaten=" + candidatesTried.ToString();
        }

        return transformId + "  prozess=" + ChefZ_SymbolTable.NameOrMark(processSym) + " dauer=" + durationSec.ToString() + "s" + " items=" + boundHandles.Count().ToString() + " verbrauch=" + consumePlan.Count().ToString();
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_TransformMatch m = new ChefZ_TransformMatch();
        if (m.matched)                                  return false;

        array<int> handles = new array<int>();
        handles.Insert(3);
        m.SetAssignment("CHEFZ_TM_SLOT", handles);
        handles.Insert(4);                              // darf nichts aendern

        array<int> stored = m.GetAssignment("CHEFZ_TM_SLOT");
        if (!stored)                                    return false;
        if (stored.Count() != 1)                        return false;
        if (m.GetAssignment("CHEFZ_TM_ANDERER"))        return false;

        m.Reset();
        if (m.GetAssignment("CHEFZ_TM_SLOT"))           return false;
        if (m.BoundItemCount() != 0)                    return false;

        return true;
    }
}

//==============================================================================

/**
 * Ein laufender Job in einem Slot einer Station (11 §4, §6).
 *
 * PERSISTENZFORM (11 §6, 03 E2): gespeichert werden HASHES von
 * Transform-ID und Prozess-ID, nie Symbolzaehler. Der Zaehler haengt an der
 * Internierungsreihenfolge und verschiebt sich, sobald ein Content-Modul
 * dazukommt - ein gespeicherter Zaehler bezeichnete nach dem naechsten Update
 * einen anderen Transform. Das waere stille, flaechendeckende Datenkorruption.
 *
 * Existiert der Transform beim Laden nicht mehr, wird der Job abgebrochen,
 * das Eingangsitem bleibt UNVERAENDERT liegen und es gibt ein WARN. Kein
 * Itemverlust durch Content-Aenderungen (11 §6).
 */
class ChefZ_ProcessJob
{
    static const int NO_HASH = 0;

    int   transformPersistHash;   // Name.Hash(), NICHT der Symbolzaehler (03 E2)
    int   processPersistHash;
    float elapsedSec;
    float durationSec;
    int   actorIdentityId;

    //! Laufzeitauflösung der beiden Hashes. NICHT persistiert und nicht
    //! synchronisiert - sie wird nach dem Laden neu abgeleitet, genau wie der
    //! Sync-Ordinal eines Zustands (06 §5).
    ChefZ_Sym transformSym;
    ChefZ_Sym processSym;

    void ChefZ_ProcessJob()
    {
        Clear();
    }

    void Clear()
    {
        transformPersistHash = NO_HASH;
        processPersistHash   = NO_HASH;
        elapsedSec           = 0.0;
        durationSec          = 0.0;
        actorIdentityId      = 0;
        transformSym         = ChefZ_SymbolTable.INVALID;
        processSym           = ChefZ_SymbolTable.INVALID;
    }

    //! Ein Slot ist belegt, sobald ein Transform-Hash darin steht. Der Hash
    //! und nicht das Symbol: nach dem Laden gibt es noch kein Symbol, der Slot
    //! ist aber sehr wohl belegt.
    bool IsActive()
    {
        return transformPersistHash != NO_HASH;
    }

    /**
     * Fortschritt 0..1.
     *
     * Eine Dauer <= 0 liefert 1.0, nicht 0.0: ein Job ohne Dauer ist FERTIG,
     * nicht ewig laufend. Die Gegenrichtung waere ein Slot, der sich nie
     * wieder freigibt.
     */
    float Progress01()
    {
        if (!IsActive())
            return 0.0;
        if (durationSec <= 0.0)
            return 1.0;

        float p = elapsedSec / durationSec;
        return Math.Clamp(p, 0.0, 1.0);
    }

    bool IsComplete()
    {
        if (!IsActive())
            return false;
        return elapsedSec >= durationSec;
    }

    /**
     * Zeit gutschreiben (11 §5, STATION_TIMED).
     *
     * @param deltaSec  vergangene Zeit
     * @param factor    speedMultiplier der Station mal Faehigkeitsfaktor
     * @param running   laeuft die Umgebung? Wenn nicht, PAUSIERT der
     *                  Fortschritt - er laeuft NIE zurueck (11 §7).
     *
     * Der Rueckwaertsfall ist hier strukturell unmoeglich, nicht nur
     * vermieden: es gibt keinen Codepfad, auf dem elapsedSec kleiner wird.
     * "Rueckwaertslaufender Fortschritt waere beim Raeuchern nur frustrierend"
     * (11 E-Note zu §7) - und ein Spieler, der zweimal Feuer nachlegt, soll
     * nicht schlechter dastehen als einer, der einmal wartet.
     */
    void Advance(float deltaSec, float factor, bool running)
    {
        if (!IsActive())
            return;
        if (!running)
            return;
        if (deltaSec <= 0.0)
            return;

        float f = factor;
        if (f <= 0.0)
            f = 1.0;

        elapsedSec = elapsedSec + deltaSec * f;

        // Nach oben klemmen: ein Job, der 300 statt 60 Sekunden zaehlt, waere
        // im Log nicht von einem Rechenfehler zu unterscheiden.
        if (durationSec > 0.0 && elapsedSec > durationSec)
            elapsedSec = durationSec;
    }

    string ToDebugString()
    {
        if (!IsActive())
            return "frei";

        string s = ChefZ_SymbolTable.NameOrMark(transformSym);
        if (!ChefZ_SymbolTable.IsValid(transformSym))
            s = "hash " + transformPersistHash.ToString();

        return s + "  " + elapsedSec.ToString() + "/" + durationSec.ToString() + "s" + " (" + Progress01().ToString() + ")";
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_ProcessJob job = new ChefZ_ProcessJob();
        if (job.IsActive())                             return false;
        if (job.Progress01() != 0.0)                    return false;
        if (job.IsComplete())                           return false;

        job.transformPersistHash = 4711;
        job.durationSec          = 10.0;
        if (!job.IsActive())                            return false;
        if (job.Progress01() != 0.0)                    return false;

        // Laeuft nicht -> pausiert, kein Rueckschritt.
        job.Advance(5.0, 1.0, false);
        if (job.elapsedSec != 0.0)                      return false;

        job.Advance(5.0, 1.0, true);
        if (job.elapsedSec != 5.0)                      return false;
        if (job.Progress01() != 0.5)                    return false;

        // Erneut pausieren: der bereits erreichte Fortschritt bleibt.
        job.Advance(5.0, 1.0, false);
        if (job.elapsedSec != 5.0)                      return false;

        // Geschwindigkeitsfaktor.
        job.Advance(1.0, 2.0, true);
        if (job.elapsedSec != 7.0)                      return false;

        // Ueber die Dauer hinaus wird geklemmt.
        job.Advance(100.0, 1.0, true);
        if (job.elapsedSec != 10.0)                     return false;
        if (!job.IsComplete())                          return false;
        if (job.Progress01() != 1.0)                    return false;

        // Negative oder null Zeitschritte wirken nicht.
        ChefZ_ProcessJob b = new ChefZ_ProcessJob();
        b.transformPersistHash = 1;
        b.durationSec          = 4.0;
        b.Advance(-3.0, 1.0, true);
        if (b.elapsedSec != 0.0)                        return false;

        // Ein Faktor <= 0 gilt als 1.0 - sonst waere die Station stillgelegt,
        // ohne dass es jemand geschrieben haette.
        b.Advance(2.0, 0.0, true);
        if (b.elapsedSec != 2.0)                        return false;

        // Dauer 0 heisst fertig, nicht ewig.
        ChefZ_ProcessJob instant = new ChefZ_ProcessJob();
        instant.transformPersistHash = 2;
        instant.durationSec          = 0.0;
        if (instant.Progress01() != 1.0)                return false;
        if (!instant.IsComplete())                      return false;

        b.Clear();
        if (b.IsActive())                               return false;

        return true;
    }
}
