//==============================================================================
// ChefZ_Log - kanalbasiertes Logging
//
// Entwurf: 18. Layer 1_Core (18 §2.1: "Bewusst die unterste Schicht: jedes
// andere System braucht es, auch Fehlerpfade im Config Manager, die laufen,
// bevor irgendetwas anderes initialisiert ist.").
//
// DIE AUFRUFREGEL (18 E2), verbindlich:
//
//     if (ChefZ_Log.Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.DEBUG))
//         ChefZ_Log.Debug(ChefZ_LogChannel.MATCH, "Slot " + id + " -> " + n);
//
// Enforce wertet Argumente VOR dem Aufruf aus. Ohne die Wache baut der
// Aufrufer den String auch dann, wenn das Log aus ist - im Matcher-Loop sind
// das dutzende Allokationen pro Tick pro Feuerstelle fuer nichts. Die
// Stufenpruefung in Debug()/Trace() unten ist ein Sicherheitsnetz, KEIN Ersatz
// fuer die Wache: sie verhindert die Ausgabe, nicht die Allokation.
//
// Ausgabe ueber PrintToRPT und nicht Print (18 E7): Print geht zusaetzlich in
// die Skriptkonsole, die auf einem Produktivserver irrelevant ist. Die
// Ausgabefunktion sitzt an genau einer Stelle (Emit), ein Wechsel ist ein
// Einzeiler.
//
// Keine Fremdabhaengigkeit (18 E8): ein Logframework aus einem anderen Mod
// wuerde jeden Server zwingen, diesen Mod mitzuladen. Fuer einen Kochmod ist
// das eine unangemessene Kopplung.
//
// Ein Logproblem darf den Mod nie anhalten (18 §6): schlaegt die Dateiausgabe
// fehl, wird sie abgeschaltet, einmal gewarnt, und die RPT-Ausgabe laeuft
// weiter.
//==============================================================================

class ChefZ_Log
{
    static const string PREFIX          = "[ChefZ]";
    static const string LOG_DIR         = "$profile:ChefZ\\Logs";
    static const string PROFILE_DIR     = "$profile:ChefZ";

    // Defaults, absichtlich als Literale und nicht als Verweis auf
    // ChefZ_LogLevel/ChefZ_LogChannel: statische Initialisierungsreihenfolge
    // ueber Klassengrenzen ist in Enforce nicht zugesichert, und ausgerechnet
    // der Fehlerpfad vor Configure() darf davon nicht abhaengen.
    // Der Selbsttest prueft, dass die Literale zu den Konstanten passen.
    private static int  s_Level        = 1;         // ChefZ_LogLevel.ERR
    private static int  s_ChannelMask  = 0x1FFF;    // ChefZ_LogChannel.ALL
    private static bool s_ToFile       = false;
    private static bool s_ServerOnly   = true;
    private static bool s_Configured   = false;

    // Seite. Ermittelt eine hoehere Schicht (GetGame() gibt es in 1_Core
    // nicht) und meldet sie ueber SetSide(). Solange sie unbekannt ist, gilt
    // die Seite als Server - so geht vor dem Boot nichts verloren.
    private static bool s_SideKnown    = false;
    private static bool s_IsServer     = true;

    private static int  s_ErrorCount   = 0;
    private static int  s_WarnCount    = 0;

    // Grenzen. Werden aus ChefZ_CoreSettingsDef gesetzt (S2), haben aber
    // brauchbare Defaults, weil das Log vor der Config lebt.
    private static int  s_MaxOnceKeys  = 512;       // 18 §5
    private static int  s_BufferLines  = 64;        // 18 §4
    private static int  s_MaxLogSizeMB = 8;
    private static int  s_MaxScopes    = 64;

    static const int FLUSH_INTERVAL_TICKS = 30000;  // 18 §4: "alle 30 Sekunden"

    private static ref map<string, bool>  s_OnceKeys;
    private static ref array<string>      s_OnceOrder;
    private static ref array<string>      s_Buffer;

    private static bool s_FileBroken   = false;
    private static bool s_InFileWrite  = false;     // Rekursionsbremse
    private static int  s_LastFlushTick = 0;
    private static int  s_BytesWritten = 0;

    // PERF-Scopes
    private static ref array<string> s_ScopeNames;
    private static ref array<int>    s_ScopeStarts;
    private static ref array<string> s_PerfNames;
    private static ref array<int>    s_PerfCounts;
    private static ref array<int>    s_PerfTicks;

    private static bool s_BannerDone   = false;

    //--------------------------------------------------------------------------
    // Aufbau
    //--------------------------------------------------------------------------

    private static void EnsureInit()
    {
        if (s_OnceKeys)
            return;
        s_OnceKeys    = new map<string, bool>();
        s_OnceOrder   = new array<string>();
        s_Buffer      = new array<string>();
        s_ScopeNames  = new array<string>();
        s_ScopeStarts = new array<int>();
        s_PerfNames   = new array<string>();
        s_PerfCounts  = new array<int>();
        s_PerfTicks   = new array<int>();
    }

    /**
     * Uebernimmt die Einstellungen aus der Config. Vor dem ersten Aufruf gilt
     * ERR auf ALL ohne Datei (18 §6) - damit gehen Fehler im Config Manager
     * selbst nie verloren.
     */
    static void Configure(int level, int channelMask, bool toFile, bool serverOnly)
    {
        EnsureInit();

        s_Level       = ChefZ_LogLevel.Clamp(level);
        s_ChannelMask = channelMask;
        if (s_ChannelMask == 0)
            s_ChannelMask = ChefZ_LogChannel.ALL;   // stilles Totalabschalten waere die schlechteste Wahl
        s_ServerOnly  = serverOnly;
        s_Configured  = true;

        SetFileOutput(toFile);

        if (s_Level >= ChefZ_LogLevel.TRACE)
        {
            // 18 §6: TRACE funktioniert auf einem Produktivserver, ist aber
            // teuer. Das soll niemand versehentlich stehen lassen.
            Warn(ChefZ_LogChannel.CORE,
                "Logstufe TRACE ist aktiv. Das ist auf einem Produktivserver teuer - "
                + "bitte nur zur Fehlersuche einschalten.");
        }
    }

    //! Grenzen aus der Config. Alles Konfigurierbare gehoert in JSON, nicht in
    //! Konstanten - die Defaults oben gelten nur, bis die Config gelesen ist.
    static void SetLimits(int maxOnceKeys, int bufferLines, int maxLogSizeMB)
    {
        if (maxOnceKeys > 0)
            s_MaxOnceKeys = maxOnceKeys;
        if (bufferLines > 0)
            s_BufferLines = bufferLines;
        if (maxLogSizeMB > 0)
            s_MaxLogSizeMB = maxLogSizeMB;
    }

    /**
     * Meldet die Seite. Aufruf aus 5_Mission, weil GetGame() in 1_Core nicht
     * existiert. Klebrig nach "Server": auf einem Listen-Server laufen beide
     * Einstiegspunkte, und die Serverseite ist die, die etwas zu protokollieren
     * hat.
     */
    static void SetSide(bool isServer)
    {
        if (s_SideKnown && s_IsServer && !isServer)
            return;
        s_SideKnown = true;
        s_IsServer  = isServer;
    }

    static bool IsServerSide()  { return s_IsServer; }
    static bool IsConfigured()  { return s_Configured; }

    //--------------------------------------------------------------------------
    // Laufzeitschalter (18 §5: der einzige zur Laufzeit aenderbare Teil des Core)
    //--------------------------------------------------------------------------

    static void SetLevel(int level)
    {
        s_Level = ChefZ_LogLevel.Clamp(level);
    }

    static int GetLevel()
    {
        return s_Level;
    }

    static void SetChannel(int channel, bool on)
    {
        if (on)
            s_ChannelMask = s_ChannelMask | channel;
        else
            s_ChannelMask = s_ChannelMask & (~channel);
    }

    static int GetChannelMask()
    {
        return s_ChannelMask;
    }

    //--------------------------------------------------------------------------
    // Die Wache
    //--------------------------------------------------------------------------

    /**
     * ERSTE Zeile jedes Aufrufs im heissen Pfad (18 E2).
     * Kostet einen Vergleich und ein AND.
     */
    static bool Enabled(int channel, int level)
    {
        if (level <= ChefZ_LogLevel.OFF)
            return false;
        if (level > s_Level)
            return false;
        if ((s_ChannelMask & channel) == 0)
            return false;
        // 18 §4: bei serverOnly gehen clientseitig nur ERR und WARN durch,
        // damit Ladefehler im Client-RPT sichtbar bleiben.
        if (s_ServerOnly && s_SideKnown && !s_IsServer && level > ChefZ_LogLevel.WARN)
            return false;
        return true;
    }

    //--------------------------------------------------------------------------
    // Ausgabe
    //--------------------------------------------------------------------------

    static void Error(int channel, string msg)
    {
        // Zaehler unabhaengig von Stufe und Kanal: sie speisen die
        // Safe-Mode-Schwelle und GetHealth() (18 §4) und duerfen nicht davon
        // abhaengen, ob gerade jemand zusieht.
        s_ErrorCount++;
        if (Enabled(channel, ChefZ_LogLevel.ERR))
            Emit(ChefZ_LogLevel.ERR, channel, msg);
    }

    static void Warn(int channel, string msg)
    {
        s_WarnCount++;
        if (Enabled(channel, ChefZ_LogLevel.WARN))
            Emit(ChefZ_LogLevel.WARN, channel, msg);
    }

    static void Info(int channel, string msg)
    {
        if (!Enabled(channel, ChefZ_LogLevel.INFO))
            return;
        Emit(ChefZ_LogLevel.INFO, channel, msg);
    }

    static void Debug(int channel, string msg)
    {
        if (!Enabled(channel, ChefZ_LogLevel.DEBUG))
            return;
        Emit(ChefZ_LogLevel.DEBUG, channel, msg);
    }

    static void Trace(int channel, string msg)
    {
        if (!Enabled(channel, ChefZ_LogLevel.TRACE))
            return;
        Emit(ChefZ_LogLevel.TRACE, channel, msg);
    }

    /**
     * Einmalige Meldung, dedupliziert ueber einen Schluessel (18 §2.1).
     * Gegen Logfluten: "gespeicherter persistHash existiert nicht mehr" darf
     * einmal je Klasse erscheinen, nicht einmal je Item (03 §7).
     * Die Menge ist auf maxOnceKeys gedeckelt, danach FIFO (18 §5).
     */
    static void Once(int level, int channel, string key, string msg)
    {
        EnsureInit();

        if (s_OnceKeys.Contains(key))
            return;

        while (s_OnceOrder.Count() >= s_MaxOnceKeys && s_OnceOrder.Count() > 0)
        {
            string oldest = s_OnceOrder.Get(0);
            s_OnceOrder.RemoveOrdered(0);
            s_OnceKeys.Remove(oldest);
        }

        s_OnceKeys.Set(key, true);
        s_OnceOrder.Insert(key);

        Dispatch(level, channel, msg);
    }

    //! Hat dieser Once-Schluessel schon gefeuert? Fuer Aufrufer, die sich die
    //! teure Stringbildung sparen wollen.
    static bool OnceFired(string key)
    {
        EnsureInit();
        return s_OnceKeys.Contains(key);
    }

    /**
     * Startzeile. Bewusst die einzige Ausgabe ohne Stufenpruefung: sie faellt
     * genau einmal pro Prozess an, ist kein heisser Pfad, und ein Betreiber
     * muss im RPT sehen koennen, dass der Mod ueberhaupt geladen hat - auch
     * bei Standardstufe WARN.
     */
    static void Banner(string msg)
    {
        string line = PREFIX + "[" + ChefZ_LogChannel.Name(ChefZ_LogChannel.CORE) + "] " + msg;
        PrintToRPT(line);
        BufferLine(line, ChefZ_LogLevel.INFO);
        s_BannerDone = true;
    }

    static bool BannerDone()
    {
        return s_BannerDone;
    }

    /**
     * Antwort auf eine ausdrueckliche Frage - ohne Stufen- und Kanalpruefung
     * (S18, 18 §2.4 "Ausgabe ins RPT und an den Aufrufer").
     *
     * Warum die Wache hier NICHT gilt: die Wache aus 18 E2 schuetzt den
     * heissen Pfad davor, Zeichenketten fuer niemanden zu bauen. Ein
     * Adminkommando ist das Gegenteil davon - jemand hat getippt, die Zeilen
     * liegen bereits fertig vor, und sie hinter der eingestellten Logstufe zu
     * verstecken hiesse, auf eine Frage nicht zu antworten. Genau dieselbe
     * Begruendung traegt Banner() eine Ebene hoeher.
     *
     * Ausdruecklich KEIN Ersatz fuer Block(): Block ist der Ausgabeweg des
     * Match-Trace im Betrieb und bleibt gewacht. Force ist der Ausgabeweg der
     * Diagnose. Wer die beiden vertauscht, bekommt entweder eine Logflut oder
     * eine stumme Konsole.
     *
     * s_BannerDone bleibt unberuehrt: eine Diagnoseantwort ist keine
     * Startzeile.
     */
    static void Force(int channel, string msg)
    {
        string line = PREFIX + "[" + ChefZ_LogChannel.Name(channel) + "] " + msg;
        PrintToRPT(line);
        BufferLine(line, ChefZ_LogLevel.INFO);
    }

    //! Blockweise Fassung von Force. Ein Diagnoseblock gehoert zusammen
    //! (18 E4) und wird deshalb in einem Rutsch geschrieben.
    static void ForceBlock(int channel, array<string> lines)
    {
        if (!lines)
            return;
        for (int i = 0; i < lines.Count(); i++)
            Force(channel, lines.Get(i));
    }

    private static void Dispatch(int level, int channel, string msg)
    {
        // Bewusst eine if-Kette und kein switch: die Sprungmarken waeren
        // statische Konstanten einer fremden Klasse, und darauf soll sich
        // ausgerechnet der Fehlerpfad nicht verlassen.
        if (level == ChefZ_LogLevel.ERR)   { Error(channel, msg); return; }
        if (level == ChefZ_LogLevel.WARN)  { Warn(channel, msg);  return; }
        if (level == ChefZ_LogLevel.INFO)  { Info(channel, msg);  return; }
        if (level == ChefZ_LogLevel.DEBUG) { Debug(channel, msg); return; }
        if (level == ChefZ_LogLevel.TRACE) { Trace(channel, msg); return; }
    }

    //! Mehrzeilige Ausgabe als zusammenhaengender Block (18 E4). Auf einem
    //! Server mit mehreren gleichzeitigen Kochvorgaengen waeren verstreute
    //! Einzelzeilen nicht zuordenbar.
    static void Block(int level, int channel, array<string> lines)
    {
        if (!lines)
            return;
        if (!Enabled(channel, level))
            return;
        for (int i = 0; i < lines.Count(); i++)
            Emit(level, channel, lines.Get(i));
    }

    private static void Emit(int level, int channel, string msg)
    {
        string line = PREFIX + LevelTag(level) + "[" + ChefZ_LogChannel.Name(channel) + "] " + msg;
        PrintToRPT(line);
        BufferLine(line, level);
    }

    //! ERR und WARN bekommen eine eigene Marke; INFO/DEBUG/TRACE bleiben
    //! schlank, damit die Blockausgaben aus 18 §3 so aussehen wie dort.
    private static string LevelTag(int level)
    {
        if (level == ChefZ_LogLevel.ERR)
            return "[ERROR]";
        if (level == ChefZ_LogLevel.WARN)
            return "[WARN]";
        return "";
    }

    //--------------------------------------------------------------------------
    // Zaehler
    //--------------------------------------------------------------------------

    static int GetErrorCount() { return s_ErrorCount; }
    static int GetWarnCount()  { return s_WarnCount; }

    static void ResetCounters()
    {
        s_ErrorCount = 0;
        s_WarnCount  = 0;
    }

    //--------------------------------------------------------------------------
    // Laufzeitmessung (Kanal PERF)
    //--------------------------------------------------------------------------

    /**
     * -1, wenn PERF aus ist. EndScope(-1) ist ein No-Op, der Aufrufer braucht
     * also keine eigene Fallunterscheidung.
     */
    static int BeginScope(string name)
    {
        if (!Enabled(ChefZ_LogChannel.PERF, ChefZ_LogLevel.DEBUG))
            return -1;

        EnsureInit();
        if (s_ScopeNames.Count() >= s_MaxScopes)
            return -1;                          // Schutz gegen unbalancierte Aufrufe

        s_ScopeNames.Insert(name);
        s_ScopeStarts.Insert(TickCount(0));
        return s_ScopeNames.Count() - 1;
    }

    static void EndScope(int scopeId)
    {
        if (scopeId < 0)
            return;
        EnsureInit();
        if (scopeId >= s_ScopeNames.Count())
            return;

        string name  = s_ScopeNames.Get(scopeId);
        int    start = s_ScopeStarts.Get(scopeId);
        int    spent = TickCount(start);

        // Alles oberhalb von scopeId aufraeumen - unbalancierte Aufrufe sollen
        // den Stapel nicht dauerhaft verstopfen.
        while (s_ScopeNames.Count() > scopeId)
        {
            s_ScopeNames.Remove(s_ScopeNames.Count() - 1);
            s_ScopeStarts.Remove(s_ScopeStarts.Count() - 1);
        }

        AccumulatePerf(name, spent);

        if (Enabled(ChefZ_LogChannel.PERF, ChefZ_LogLevel.TRACE))
            Emit(ChefZ_LogLevel.TRACE, ChefZ_LogChannel.PERF, name + " " + spent.ToString() + " ticks");
    }

    private static void AccumulatePerf(string name, int ticks)
    {
        int idx = s_PerfNames.Find(name);
        if (idx < 0)
        {
            s_PerfNames.Insert(name);
            s_PerfCounts.Insert(1);
            s_PerfTicks.Insert(ticks);
            return;
        }
        s_PerfCounts.Set(idx, s_PerfCounts.Get(idx) + 1);
        s_PerfTicks.Set(idx, s_PerfTicks.Get(idx) + ticks);
    }

    //! Fuer "chefz stats" (5_Mission, spaeterer Schritt).
    static void DumpPerfCounters(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();
        EnsureInit();
        for (int i = 0; i < s_PerfNames.Count(); i++)
        {
            int count = s_PerfCounts.Get(i);
            int total = s_PerfTicks.Get(i);
            int avg   = 0;
            if (count > 0)
                avg = total / count;
            outLines.Insert(s_PerfNames.Get(i) + "  n=" + count.ToString()
                + "  gesamt=" + total.ToString() + "  mittel=" + avg.ToString());
        }
    }

    static void ResetPerfCounters()
    {
        EnsureInit();
        s_PerfNames.Clear();
        s_PerfCounts.Clear();
        s_PerfTicks.Clear();
    }

    //--------------------------------------------------------------------------
    // Dateiausgabe
    //--------------------------------------------------------------------------

    static string GetLogFilePath()
    {
        int year, month, day;
        GetYearMonthDay(year, month, day);
        return LOG_DIR + "\\ChefZ_" + Pad(year, 4) + "-" + Pad(month, 2) + "-" + Pad(day, 2) + ".log";
    }

    static bool IsFileOutputActive()
    {
        return s_ToFile && !s_FileBroken;
    }

    static void SetFileOutput(bool on)
    {
        EnsureInit();
        if (!on)
        {
            Flush();
            s_ToFile = false;
            return;
        }

        s_ToFile      = true;
        s_FileBroken  = false;
        s_BytesWritten = 0;
        s_LastFlushTick = TickCount(0);
        EnsureDirectories();
    }

    private static void EnsureDirectories()
    {
        // Rueckgabewert bewusst ignoriert: MakeDirectory meldet auch dann
        // false, wenn das Verzeichnis bereits existiert. Ob geschrieben werden
        // kann, entscheidet erst OpenFile.
        MakeDirectory(PROFILE_DIR);
        MakeDirectory(LOG_DIR);
    }

    private static void BufferLine(string line, int level)
    {
        if (!s_ToFile || s_FileBroken || s_InFileWrite)
            return;

        EnsureInit();
        s_Buffer.Insert(line);

        // ERR und WARN sofort: was den Server zum Absturz bringt, soll nicht
        // im Puffer verhungern.
        if (level <= ChefZ_LogLevel.WARN)
        {
            Flush();
            return;
        }
        if (s_Buffer.Count() >= s_BufferLines)
        {
            Flush();
            return;
        }
        if (TickCount(s_LastFlushTick) >= FLUSH_INTERVAL_TICKS)
            Flush();
    }

    /**
     * Schreibt den Zeilenpuffer. 18 §4: OpenFile/FPrint/CloseFile je Zeile
     * waere bei aktivem Debug spuerbar, deshalb gepuffert.
     *
     * Scheitert das Oeffnen, wird die Dateiausgabe dauerhaft abgeschaltet und
     * einmal gewarnt. Der Puffer wird in jedem Fall geleert - ein Logproblem
     * darf nicht zu einem Speicherproblem werden.
     */
    static void Flush()
    {
        EnsureInit();

        if (s_Buffer.Count() == 0)
            return;

        if (!s_ToFile || s_FileBroken || s_InFileWrite)
        {
            s_Buffer.Clear();
            return;
        }

        s_InFileWrite = true;

        RotateIfNeeded();

        string path = GetLogFilePath();
        FileHandle fh = OpenFile(path, FileMode.APPEND);
        if (fh == 0)
        {
            EnsureDirectories();
            fh = OpenFile(path, FileMode.APPEND);
        }

        if (fh == 0)
        {
            s_FileBroken = true;
            s_InFileWrite = false;
            s_Buffer.Clear();
            // Nicht ueber Once(), weil Once() wieder hier landen wuerde.
            PrintToRPT(PREFIX + "[WARN][CORE] Logdatei \"" + path
                + "\" ist nicht schreibbar. Dateiausgabe abgeschaltet, RPT-Ausgabe laeuft weiter.");
            s_WarnCount++;
            return;
        }

        for (int i = 0; i < s_Buffer.Count(); i++)
        {
            string line = s_Buffer.Get(i);
            FPrintln(fh, line);
            s_BytesWritten = s_BytesWritten + line.Length() + 2;
        }
        CloseFile(fh);

        s_Buffer.Clear();
        s_LastFlushTick = TickCount(0);
        s_InFileWrite = false;
    }

    /**
     * Groessenrotation (18 §6). Die Tagesrotation steckt bereits im Dateinamen.
     *
     * Gemessen wird, was diese Sitzung geschrieben hat - eine Dateigroesse ist
     * ueber die Enforce-Datei-API nicht abfragbar. Nach einem Neustart beginnt
     * die Zaehlung neu; die Datei kann dadurch groesser werden als die Grenze.
     * Das ist die ehrliche Grenze dieser Umsetzung und harmlos, weil die
     * Rotation nur Plattenplatz schuetzt.
     */
    private static void RotateIfNeeded()
    {
        int limit = s_MaxLogSizeMB * 1024 * 1024;
        if (limit <= 0 || s_BytesWritten < limit)
            return;

        string cur = GetLogFilePath();
        string bak = cur + ".1";

        if (FileExist(bak))
            DeleteFile(bak);
        if (CopyFile(cur, bak))
            DeleteFile(cur);

        s_BytesWritten = 0;
    }

    private static string Pad(int value, int width)
    {
        string s = value.ToString();
        while (s.Length() < width)
            s = "0" + s;
        return s;
    }

    //--------------------------------------------------------------------------
    // Selbsttest (S1)
    //--------------------------------------------------------------------------

    //! Prueft die Wachenlogik ohne etwas auszugeben. Stellt den vorherigen
    //! Zustand vollstaendig wieder her.
    static bool SelfCheck()
    {
        int  keepLevel   = s_Level;
        int  keepMask    = s_ChannelMask;
        bool keepServer  = s_ServerOnly;
        bool keepKnown   = s_SideKnown;
        bool keepIsSrv   = s_IsServer;
        int  keepErrors  = s_ErrorCount;
        int  keepWarns   = s_WarnCount;

        bool ok = true;

        // Die Literaldefaults muessen zu den Konstanten passen.
        if (1      != ChefZ_LogLevel.ERR)       ok = false;
        if (0x1FFF != ChefZ_LogChannel.ALL)     ok = false;

        s_Level       = ChefZ_LogLevel.DEBUG;
        s_ChannelMask = ChefZ_LogChannel.MATCH | ChefZ_LogChannel.COOK;
        s_ServerOnly  = true;
        s_SideKnown   = true;
        s_IsServer    = true;

        if (!Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.DEBUG))  ok = false;
        if (Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.TRACE))   ok = false;
        if (Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.ERR))    ok = false;
        if (Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.OFF))     ok = false;

        // Clientseite: nur ERR und WARN kommen durch.
        s_IsServer = false;
        if (Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.DEBUG))   ok = false;
        if (!Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.WARN))   ok = false;
        if (!Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.ERR))     ok = false;

        // Stufe OFF schaltet alles ab.
        s_IsServer = true;
        s_Level    = ChefZ_LogLevel.OFF;
        if (Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.ERR))     ok = false;

        // Zaehler laufen unabhaengig von der Stufe weiter.
        s_ErrorCount = 0;
        s_WarnCount  = 0;
        Error(ChefZ_LogChannel.CORE, "selftest");
        Warn(ChefZ_LogChannel.CORE, "selftest");
        if (s_ErrorCount != 1) ok = false;
        if (s_WarnCount  != 1) ok = false;

        if (Pad(7, 2)   != "07")   ok = false;
        if (Pad(2026, 4) != "2026") ok = false;

        s_Level       = keepLevel;
        s_ChannelMask = keepMask;
        s_ServerOnly  = keepServer;
        s_SideKnown   = keepKnown;
        s_IsServer    = keepIsSrv;
        s_ErrorCount  = keepErrors;
        s_WarnCount   = keepWarns;

        return ok;
    }
}
