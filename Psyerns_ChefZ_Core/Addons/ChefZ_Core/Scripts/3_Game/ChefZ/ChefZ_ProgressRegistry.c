//==============================================================================
// ChefZ_IProgressSink / ChefZ_ProgressRegistry - der Core meldet Abschluesse,
//                                                er vergibt kein XP
//
// Entwurf: 17 §3.4 (Schnittstelle woertlich), 17 E7 (der Core vergibt kein XP,
// er meldet Abschluesse), 17 §4 (die XP-Spalte des Ereigniskatalogs),
// Workflow §10.6, Planungsschritte §22 (Anti-Exploit-Liste).
//
// ---------------------------------------------------------------------------
// Regel §10.6, strukturell statt versprochen
// ---------------------------------------------------------------------------
// "XP nur nach erfolgreichem Abschluss." Diese Regel wird hier nicht geprueft,
// sondern GEBAUT: Report() steht im Core ausschliesslich hinter einem
// vollzogenen Abschluss - nach dem Verbrauch der Zutaten und der Erzeugung des
// Ergebnisses, nie beim Einlegen, nie beim Start, nie beim Match.
//
// Die Folge ist die eigentliche Zusage: eine XP-Schleife durch Einlegen und
// Entnehmen ist NICHT BAUBAR, weil es dafuer schlicht keinen Aufruf gibt. Ein
// Comp-Modul, das nur diese Schnittstelle benutzt, kann Regel §10.6 nicht
// verletzen, selbst wenn es wollte.
//
// Wer diese Datei erweitert, prueft genau eine Frage: Steht der neue
// Report()-Aufruf hinter einer Wirkung, die bereits eingetreten ist? Wenn
// nein, gehoert er nicht in den Core - dafuer gibt es Ereignisse, und die
// sind ausdruecklich NICHT XP-tauglich markiert (17 §4).
//
// ---------------------------------------------------------------------------
// Warum das nicht einfach ein weiteres Ereignis ist
// ---------------------------------------------------------------------------
// Weil der Unterschied dann verschwaende. Auf dem Bus sind alle Ereignisse
// gleich; ein Comp-Modul muesste die Tabelle aus 17 §4 lesen und sich daran
// halten. Eine eigene Schnittstelle macht die Unterscheidung sichtbar und
// unumgehbar: es gibt genau fuenf Anlaesse, und jeder bezeichnet etwas
// Fertiges (ChefZ_ProgressKind).
//
// Der Core kennt keine Progression, keine Stufe, keine Kurve und keinen
// Punktwert. Er sagt "das hier ist gelungen" und uebergibt dieselbe Nutzlast,
// die auch auf dem Bus liegt - das war es.
//
// KEIN CONTENT und kein Fremdsystembezug.
//
// Layer: 3_Game.
//==============================================================================

/**
 * Der Empfaenger, den ein Comp-Modul liefert.
 *
 * Enforce kennt kein "interface" - 17 §3.4 schreibt es als Pseudocode. Die
 * Umsetzung ist eine Basisklasse mit sicheren Vorgaben.
 *
 * ACHTUNG, dieselbe Regel wie auf dem Bus (17 §8): args gehoert dem Core und
 * ist nach der Rueckkehr aus OnChefZProgress UNGUELTIG. Wer Daten braucht,
 * kopiert sie im Rueckruf. Ein festgehaltener ref auf eine Nutzlast mit der
 * Netz-ID einer laengst geloeschten Entity ist die typische Fehlerquelle.
 */
class ChefZ_IProgressSink
{
    string GetSinkName()
    {
        return "<unbenannter Empfaenger>";
    }

    /**
     * @param progressKind einer aus ChefZ_ProgressKind - "cook", "process",
     *                     "preserve", "consume", "discover".
     * @param args         dieselbe Nutzlast wie auf dem Bus. NUR im Rueckruf
     *                     gueltig.
     */
    void OnChefZProgress(string progressKind, notnull ChefZ_EventArgs args)
    {
    }
}

//==============================================================================

class ChefZ_ProgressRegistry
{
    /**
     * Die Empfaenger. Statisch, weil 17 §3.4 die Schnittstelle statisch
     * vorgibt - und weil es nichts gibt, wovon ein zweiter Bestand abhinge:
     * ein Fortschrittsmelder hat keinen Zustand ausser seiner Liste.
     *
     * MIT ref, aus demselben Grund wie bei den Faehigkeitsanbietern: ein
     * Empfaenger wird als "new MeinSink()" uebergeben und hat sonst keinen
     * Halter.
     */
    private static ref array<ref ChefZ_IProgressSink> s_Sinks;

    /**
     * Die Rueckrufe, parallel zu s_Sinks und mit demselben Index.
     *
     * Einmal bei der Anmeldung erzeugt statt bei jedem Abschluss: ein
     * ScriptCaller je Meldung waere eine Allokation im Kochpfad fuer etwas,
     * das sich nie aendert. Die Parallelfuehrung ist der Preis dafuer -
     * jede Einfuegung und jede Entfernung fasst BEIDE Listen an.
     */
    private static ref array<ref ScriptCaller> s_Callers;

    private static int  s_CountReported;
    private static int  s_CountDelivered;
    private static bool s_QuietForTest;

    private static void EnsureInit()
    {
        if (s_Sinks)
            return;
        s_Sinks   = new array<ref ChefZ_IProgressSink>();
        s_Callers = new array<ref ScriptCaller>();
    }

    static void RegisterSink(notnull ChefZ_IProgressSink sink)
    {
        EnsureInit();

        if (s_Sinks.Find(sink) >= 0)
        {
            if (!s_QuietForTest)
            {
                ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.EVENT, "progress.dup." + sink.GetSinkName(), "Fortschrittsempfaenger \"" + sink.GetSinkName() + "\" ist bereits registriert. Die zweite Anmeldung wird verworfen - " + "sonst bekaeme er jeden Abschluss doppelt.");
            }
            return;
        }

        ScriptCaller call = ScriptCaller.Create(sink.OnChefZProgress);
        if (!call || !call.IsValid())
        {
            if (!s_QuietForTest)
            {
                ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.EVENT, "progress.nocall." + sink.GetSinkName(), "Fortschrittsempfaenger \"" + sink.GetSinkName() + "\" ist nicht aufrufbar - die Anmeldung wird abgelehnt. Ursache ist " + "fast immer eine Ableitung, die OnChefZProgress nicht ueberschreibt.");
            }
            return;
        }

        s_Sinks.Insert(sink);
        s_Callers.Insert(call);

        if (!s_QuietForTest)
        {
            ChefZ_Log.Info(ChefZ_LogChannel.EVENT, "Fortschrittsempfaenger \"" + sink.GetSinkName() + "\" registriert (jetzt " + s_Sinks.Count().ToString() + ").");
        }
    }

    static void UnregisterSink(notnull ChefZ_IProgressSink sink)
    {
        EnsureInit();

        int idx = s_Sinks.Find(sink);
        if (idx < 0)
            return;
        s_Sinks.RemoveOrdered(idx);
        if (idx < s_Callers.Count())
            s_Callers.RemoveOrdered(idx);
    }

    static int GetSinkCount()
    {
        EnsureInit();
        return s_Sinks.Count();
    }

    /**
     * Hoert ueberhaupt jemand zu?
     *
     * Dieselbe Zusage wie ChefZ_EventBus.HasSubscribers (17 E2): ohne
     * Comp-Module wird keine Nutzlast gebaut. Die Ausloesestellen fragen
     * zuerst hier und den Bus, und bauen nur einmal - dieselbe Nutzlast geht
     * an beide.
     */
    static bool HasSinks()
    {
        EnsureInit();
        return s_Sinks.Count() > 0;
    }

    /**
     * Einen ABGESCHLOSSENEN Vorgang melden.
     *
     * VERBINDLICH: Diese Funktion wird im Core ausschliesslich NACH einem
     * erfolgreichen Abschluss gerufen (17 E7). Es gibt keinen Aufruf beim
     * Start einer Aktion und keinen beim Einlegen einer Zutat - und es darf
     * nie einen geben.
     *
     * Ein Empfaenger, der wirft, haelt die uebrigen nicht auf: der Aufruf geht
     * ueber ScriptCaller.Invoke, eine native Grenze, genau wie auf dem Bus.
     */
    static void Report(string progressKind, notnull ChefZ_EventArgs args)
    {
        EnsureInit();

        if (s_Sinks.Count() == 0)
            return;

        s_CountReported++;

        // Kopie, weil ein Empfaenger im Rueckruf einen weiteren registrieren
        // oder sich abmelden darf - beides veraendert die Liste, ueber die
        // gerade gelaufen wird.
        // MIT ref, aus demselben Grund wie im ChefZ_EventBus: meldet sich ein
        // Empfaenger waehrend der Zustellung ab, faellt die starke Halterung in
        // s_Callers weg, und ohne ref stuende hier ein Loch in der Kopie.
        array<ref ScriptCaller> snapshot = new array<ref ScriptCaller>();
        for (int c = 0; c < s_Callers.Count(); c++)
            snapshot.Insert(s_Callers.Get(c));

        for (int i = 0; i < snapshot.Count(); i++)
        {
            ScriptCaller call = snapshot.Get(i);
            if (!call || !call.IsValid())
                continue;

            // ------------------------------------------------------------------
            // DIE GRENZE. Alles jenseits davon ist fremder Code (siehe
            // ChefZ_EventBus, Abschnitt "Ein fremder Mod darf das Kochen nie
            // anhalten").
            // ------------------------------------------------------------------
            call.Invoke(progressKind, args);
            s_CountDelivered++;
        }

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.EVENT, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.EVENT, "Fortschritt \"" + progressKind + "\" an " + snapshot.Count().ToString() + " Empfaenger: " + args.ToDebugString());
        }
    }

    static int GetReportedCount()  { return s_CountReported; }
    static int GetDeliveredCount() { return s_CountDelivered; }

    static void ResetCounters()
    {
        s_CountReported  = 0;
        s_CountDelivered = 0;
    }

    //! Vorgesehener Aufrufer ist der SAFE_MODE (02 §8) und der Selbsttest. Ein
    //! Core, der sich abschaltet, meldet keine Abschluesse mehr - er erzeugt ja
    //! auch keine.
    static void ClearSinks()
    {
        EnsureInit();
        s_Sinks.Clear();
        s_Callers.Clear();
    }

    static void SetQuietForTest(bool quiet)
    {
        s_QuietForTest = quiet;
    }

    static void Dump(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();
        EnsureInit();

        outLines.Insert("Fortschritt: " + s_Sinks.Count().ToString() + " Empfaenger, " + "gemeldet=" + s_CountReported.ToString() + "  zugestellt=" + s_CountDelivered.ToString());

        for (int i = 0; i < s_Sinks.Count(); i++)
        {
            ChefZ_IProgressSink sink = s_Sinks.Get(i);
            if (sink)
                outLines.Insert("  " + sink.GetSinkName());
        }
    }
}
