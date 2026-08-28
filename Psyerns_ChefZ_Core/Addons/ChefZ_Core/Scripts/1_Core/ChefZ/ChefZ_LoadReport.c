//==============================================================================
// ChefZ_LoadReport - abfragbarer Ladebericht
//
// Entwurf: 18 §2.3.
//
// Ein eigenes Objekt und kein Log-Strom, weil Ladefehler eine andere
// Lebensdauer haben: sie muessen nach dem Start noch ABFRAGBAR sein - fuer
// GetHealth(), fuer die Safe-Mode-Schwelle und fuer den Gate-Report des
// Validators (18 §2.3).
//
// ---------------------------------------------------------------------------
// LAYER-ABWEICHUNG, bewusst und begruendet:
//
// 18 §2.3 verortet diese Klasse in 3_Game, 03 §3.2 verlangt sie aber als
// Parameter von ChefZ_IdentityMap.Build() - und ChefZ_IdentityMap liegt laut
// 03 §3.2 in 1_Core. Eine 1_Core-Klasse kann keine 3_Game-Klasse sehen; eine
// der beiden Angaben muss weichen.
//
// Gewaehlt: die Klasse wandert nach 1_Core. Sie benutzt ausschliesslich
// 1_Core-Mittel (array, string, OpenFile, PrintToRPT) und keinen Engine-Typ,
// erfuellt also die Layer-Regel aus 00 §4 ("rein datenverarbeitend, kein
// Engine-Typ") vollstaendig. Nach unten verschieben ist unschaedlich: jede
// 3_Game-Klasse kann sie unveraendert benutzen, die Schnittstelle aus 18 §2.3
// bleibt Zeichen fuer Zeichen dieselbe.
//
// Fuer S2 heisst das: ChefZ_LoadReport ist bereits da und NICHT erneut in
// 3_Game anzulegen - ein zweiter Klassenname waere ein Compilefehler.
// ---------------------------------------------------------------------------
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_LoadReportEntry
{
    int    severity;        // ChefZ_LogLevel.ERR | WARN | INFO
    string sourceRef;       // Datei, Config-Pfad oder Registryname
    string recordId;        // betroffener Record, "" wenn keiner
    string message;

    void Init(int sev, string src, string rec, string msg)
    {
        severity  = sev;
        sourceRef = src;
        recordId  = rec;
        message   = msg;
    }

    string ToLine()
    {
        string head = ChefZ_LogLevel.Name(severity);
        string where = sourceRef;
        if (recordId != "")
        {
            if (where != "")
                where = where + " / " + recordId;
            else
                where = recordId;
        }
        if (where == "")
            return head + "  " + message;
        return head + "  " + where + "  " + message;
    }
}

class ChefZ_LoadReport
{
    //! Deckel gegen einen Bericht, der bei einer kaputten Massendatei den
    //! Speicher frisst. Darueber hinaus wird nur noch gezaehlt.
    static const int DEFAULT_MAX_ENTRIES = 2000;
    static const int SUMMARY_MAX_LINES   = 20;

    private ref array<ref ChefZ_LoadReportEntry> m_Entries;
    private int  m_Errors;
    private int  m_Warns;
    private int  m_Infos;
    private int  m_Dropped;
    private int  m_MaxEntries;
    private int  m_Channel;
    private bool m_MirrorToLog;

    void ChefZ_LoadReport()
    {
        m_Entries     = new array<ref ChefZ_LoadReportEntry>();
        m_Errors      = 0;
        m_Warns       = 0;
        m_Infos       = 0;
        m_Dropped     = 0;
        m_MaxEntries  = DEFAULT_MAX_ENTRIES;
        m_Channel     = ChefZ_LogChannel.CONFIG;
        m_MirrorToLog = true;
    }

    //! Kanal fuer die Spiegelung ins Log. Default CONFIG.
    void SetChannel(int channel)
    {
        m_Channel = channel;
    }

    /**
     * Spiegelung ins Log an oder aus.
     *
     * Default an: ein Ladefehler soll im RPT stehen, sobald er entsteht, und
     * nicht erst, wenn jemand den Bericht ausgibt. Wenn der Prozess vorher
     * stirbt, ist genau diese Zeile die einzige Spur.
     */
    void SetMirrorToLog(bool on)
    {
        m_MirrorToLog = on;
    }

    void SetMaxEntries(int max)
    {
        if (max > 0)
            m_MaxEntries = max;
    }

    //--------------------------------------------------------------------------

    void AddError(string sourceRef, string recordId, string message)
    {
        m_Errors++;
        Add(ChefZ_LogLevel.ERR, sourceRef, recordId, message);
    }

    void AddWarn(string sourceRef, string recordId, string message)
    {
        m_Warns++;
        Add(ChefZ_LogLevel.WARN, sourceRef, recordId, message);
    }

    void AddInfo(string message)
    {
        m_Infos++;
        Add(ChefZ_LogLevel.INFO, "", "", message);
    }

    private void Add(int severity, string sourceRef, string recordId, string message)
    {
        if (m_Entries.Count() < m_MaxEntries)
        {
            ChefZ_LoadReportEntry e = new ChefZ_LoadReportEntry();
            e.Init(severity, sourceRef, recordId, message);
            m_Entries.Insert(e);
        }
        else
        {
            m_Dropped++;
        }

        if (!m_MirrorToLog)
            return;

        // Enabled-Wache nach 18 E2, auch hier: fuer INFO wuerde der
        // zusammengesetzte String sonst gebaut und weggeworfen. ERR und WARN
        // laufen ohne Wache, weil sie ohnehin durchgehen sollen und ihre
        // Zaehler stufenunabhaengig sind.
        if (severity > ChefZ_LogLevel.WARN
            && !ChefZ_Log.Enabled(m_Channel, severity))
            return;

        string where = sourceRef;
        if (recordId != "")
        {
            if (where != "")
                where = where + " / " + recordId;
            else
                where = recordId;
        }

        string line = message;
        if (where != "")
            line = where + ": " + message;

        if (severity == ChefZ_LogLevel.ERR)
            ChefZ_Log.Error(m_Channel, line);
        else if (severity == ChefZ_LogLevel.WARN)
            ChefZ_Log.Warn(m_Channel, line);
        else
            ChefZ_Log.Info(m_Channel, line);
    }

    //--------------------------------------------------------------------------

    int ErrorCount()   { return m_Errors; }
    int WarnCount()    { return m_Warns; }
    int InfoCount()    { return m_Infos; }
    int DroppedCount() { return m_Dropped; }
    int Count()        { return m_Entries.Count(); }
    bool IsClean()     { return m_Errors == 0; }

    array<ref ChefZ_LoadReportEntry> GetEntries()
    {
        return m_Entries;
    }

    void Clear()
    {
        m_Entries.Clear();
        m_Errors  = 0;
        m_Warns   = 0;
        m_Infos   = 0;
        m_Dropped = 0;
    }

    void ToLines(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert(SummaryLine());
        for (int i = 0; i < m_Entries.Count(); i++)
            outLines.Insert("  " + m_Entries.Get(i).ToLine());

        if (m_Dropped > 0)
            outLines.Insert("  ... " + m_Dropped.ToString()
                + " weitere Eintraege verworfen (Deckel " + m_MaxEntries.ToString() + ")");
    }

    string SummaryLine()
    {
        return "Ladebericht: " + m_Errors.ToString() + " Fehler, "
             + m_Warns.ToString() + " Warnungen, "
             + m_Infos.ToString() + " Hinweise";
    }

    /**
     * Kurzfassung ins RPT. Laut 18 §4 IMMER, auch bei Erfolg - "keine Fehler"
     * ist eine Information, die ein Betreiber sehen will.
     *
     * Geht bewusst nicht durch ChefZ_Log.Info: der Bericht faellt einmal beim
     * Boot an und muss auch bei Stufe WARN sichtbar sein.
     */
    void PrintSummary()
    {
        PrintToRPT(ChefZ_Log.PREFIX + "[CONFIG] " + SummaryLine());

        int shown = 0;
        for (int i = 0; i < m_Entries.Count() && shown < SUMMARY_MAX_LINES; i++)
        {
            ChefZ_LoadReportEntry e = m_Entries.Get(i);
            if (e.severity > ChefZ_LogLevel.WARN)
                continue;
            PrintToRPT(ChefZ_Log.PREFIX + "[CONFIG]   " + e.ToLine());
            shown++;
        }

        int notShown = m_Errors + m_Warns - shown;
        if (notShown > 0)
            PrintToRPT(ChefZ_Log.PREFIX + "[CONFIG]   ... " + notShown.ToString() + " weitere - vollstaendig in " + WriteTargetHint());
    }

    private string WriteTargetHint()
    {
        return "$profile:ChefZ\\Logs\\load_report.txt";
    }

    /**
     * Schreibt den vollstaendigen Bericht. false, wenn die Datei nicht
     * schreibbar ist - das ist kein Grund, irgendetwas abzubrechen.
     */
    bool WriteToFile(string path)
    {
        MakeDirectory(ChefZ_Log.PROFILE_DIR);
        MakeDirectory(ChefZ_Log.LOG_DIR);

        FileHandle fh = OpenFile(path, FileMode.WRITE);
        if (fh == 0)
        {
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.CONFIG,
                "loadreport.write.failed",
                "Ladebericht konnte nicht nach \"" + path + "\" geschrieben werden. "
                + "Der Bericht steht weiterhin im RPT.");
            return false;
        }

        array<string> lines = new array<string>();
        ToLines(lines);
        for (int i = 0; i < lines.Count(); i++)
            FPrintln(fh, lines.Get(i));
        CloseFile(fh);
        return true;
    }

    //! Standardpfad aus 18 §4.
    bool WriteToDefaultFile()
    {
        return WriteToFile(WriteTargetHint());
    }

    /**
     * Uebernimmt die Eintraege eines anderen Berichts. Fuer den Config
     * Manager, der je Quelle einen Teilbericht fuehrt und sie am Ende
     * zusammenzieht.
     */
    void Merge(notnull ChefZ_LoadReport other)
    {
        array<ref ChefZ_LoadReportEntry> src = other.GetEntries();
        for (int i = 0; i < src.Count(); i++)
        {
            if (m_Entries.Count() >= m_MaxEntries)
            {
                m_Dropped++;
                continue;
            }
            m_Entries.Insert(src.Get(i));
        }
        m_Errors  = m_Errors  + other.ErrorCount();
        m_Warns   = m_Warns   + other.WarnCount();
        m_Infos   = m_Infos   + other.InfoCount();
        m_Dropped = m_Dropped + other.DroppedCount();
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest (S1). Ohne Log-Spiegelung, damit der Test
    //! die Fehlerzaehler des Logs nicht verfaelscht.
    static bool SelfCheck()
    {
        ChefZ_LoadReport r = new ChefZ_LoadReport();
        r.SetMirrorToLog(false);

        if (!r.IsClean())                       return false;
        r.AddInfo("nur ein Hinweis");
        if (!r.IsClean())                       return false;

        r.AddWarn("Core.json", "CORE", "Feld unbekannt");
        r.AddError("States.json", "CHEFZ_LR_ZUSTAND", "Hash-Kollision");
        if (r.ErrorCount() != 1)                return false;
        if (r.WarnCount() != 1)                 return false;
        if (r.InfoCount() != 1)                 return false;
        if (r.IsClean())                        return false;
        if (r.Count() != 3)                     return false;

        ChefZ_LoadReport other = new ChefZ_LoadReport();
        other.SetMirrorToLog(false);
        other.AddError("Tags.json", "CHEFZ_LR_TAG", "doppelt");
        r.Merge(other);
        if (r.ErrorCount() != 2)                return false;
        if (r.Count() != 4)                     return false;

        array<string> lines = new array<string>();
        r.ToLines(lines);
        if (lines.Count() != 5)                 return false;

        r.Clear();
        if (r.Count() != 0)                     return false;
        if (!r.IsClean())                       return false;

        return true;
    }
}
