//==============================================================================
// ChefZ_ProfilePaths / ChefZ_ProfileOverlaySource - Rang 3: das Admin-Overlay
//
// Entwurf: 02 §3 (Rang 3, nur Server, feldweiser Patch), 02 §7 ("der Core
// ueberschreibt Betreiberdateien NIE, er legt nur fehlende Vorlagen an"),
// 02 §8 (Verzeichnis fehlt -> anlegen; Dateisystem nicht beschreibbar ->
// Overlay entfaellt, WARN), 03 §4 (keine sync-relevante Erweiterung),
// Architekturplan §22 (der Aufbau unter $profile:ChefZ/).
//
// Warum NUR Server: $profile: ist clientseitig %localappdata%\dayz. Ein
// Overlay dort waere eine Datei auf dem Rechner des Spielers, die ueber
// Serverregeln entscheidet - das waere nicht nur nutzlos, sondern falsch
// (02 E2). Der Client bekommt das Rang-3-Delta gesyncht, er liest es nie
// selbst.
//
// Auffindung der Overlaydateien:
//   - eine feste Datei  $profile:ChefZ\Core.json  (Einstellungen)
//   - alles in          $profile:ChefZ\Overlay\*.json
//
// Der Verzeichnisscan ist hier zulaessig und nicht im Widerspruch zu 02 §4:
// verboten ist der Scan ueber PBO-INHALTE (01 V8 - FindFileFlags kennt nur
// DIRECTORIES und ARCHIVES(.pak)). $profile: ist ein echtes Dateisystem-
// verzeichnis, und genau dort funktioniert FindFileFlags.DIRECTORIES.
// Liefert der Scan wider Erwarten nichts, bleibt Core.json trotzdem wirksam -
// das Overlay faellt nie ganz aus, nur teilweise.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_ProfilePaths
{
    //! Wurzel. Bewusst dieselbe Konstante wie im Log (ChefZ_Log.PROFILE_DIR),
    //! damit es nicht zwei Wahrheiten ueber den Ort gibt.
    static string Root()
    {
        return ChefZ_Log.PROFILE_DIR;               // "$profile:ChefZ"
    }

    static string Logs()
    {
        return ChefZ_Log.LOG_DIR;                   // "$profile:ChefZ\Logs"
    }

    static string OverlayDir()
    {
        return Root() + "\\Overlay";
    }

    static string CoreSettingsFile()
    {
        return Root() + "\\Core.json";
    }

    static string ReadmeFile()
    {
        return Root() + "\\README.txt";
    }

    static string OverlayPattern()
    {
        return OverlayDir() + "\\*.json";
    }

    //! Vorlage im PBO, aus der Core.json beim ersten Start kopiert wird.
    static string CoreSettingsTemplateInPbo()
    {
        return "ChefZ_Core/Config/Templates/Core.overlay.json";
    }
}

//==============================================================================

class ChefZ_ProfileOverlaySource extends ChefZ_IRecordSource
{
    private int  m_FileCount;
    private bool m_Writable;

    void ChefZ_ProfileOverlaySource()
    {
        m_FileCount = 0;
        m_Writable  = true;
    }

    override string GetName()
    {
        return "$profile:ChefZ (Overlay)";
    }

    override int GetRank()
    {
        return ChefZ_SourceRank.PROFILE_OVERLAY;
    }

    override int GetFileCount()
    {
        return m_FileCount;
    }

    bool IsWritable()
    {
        return m_Writable;
    }

    //--------------------------------------------------------------------------

    /**
     * Legt die Verzeichnisse an und kopiert fehlende Vorlagen.
     *
     * Getrennt von Read(), weil es auch dann laufen soll, wenn der Betreiber
     * das Overlay abgeschaltet hat: das Log schreibt ebenfalls nach
     * $profile:ChefZ\Logs, und die Vorlage ist die Anleitung dafuer, wie man
     * das Overlay wieder einschaltet.
     *
     * Ueberschreibt NIE eine vorhandene Datei (02 §7).
     */
    bool EnsureLayout(ChefZ_LoadReport report)
    {
        m_Writable = true;

        // Rueckgabewerte bewusst ignoriert: MakeDirectory meldet auch dann
        // false, wenn das Verzeichnis schon da ist - das waere kein
        // brauchbarer Erfolgstest. Ob geschrieben werden kann, entscheidet der
        // tatsaechliche Schreibversuch weiter unten.
        MakeDirectory(ChefZ_ProfilePaths.Root());
        MakeDirectory(ChefZ_ProfilePaths.Logs());
        MakeDirectory(ChefZ_ProfilePaths.OverlayDir());

        EnsureFile(ChefZ_ProfilePaths.CoreSettingsFile(), ChefZ_ProfilePaths.CoreSettingsTemplateInPbo(), report);
        EnsureReadme(report);

        if (!m_Writable && report)
        {
            // 02 §8: Dateisystem nicht beschreibbar -> Overlay entfaellt,
            // Rang 1+2 gelten, ChefZ laeuft, ist aber nicht tunebar.
            report.AddWarn(ChefZ_ProfilePaths.Root(), "", "Unter " + ChefZ_ProfilePaths.Root() + " kann nicht geschrieben werden - " + "das Overlay entfaellt. ChefZ laeuft mit Rang 1 und 2 weiter, ist aber " + "nicht ohne PBO-Neubau einstellbar. Ursache ist meist ein fehlendes " + "-profiles= am Serverstart oder ein schreibgeschuetztes Verzeichnis.");
        }
        return m_Writable;
    }

    /**
     * Legt eine fehlende Datei an: erst per CopyFile aus dem PBO, sonst aus
     * einer eingebauten Minimalvorlage.
     *
     * Warum beides: CopyFile ist der ehrliche Weg (die Vorlage im PBO ist
     * kommentiert und gepflegt), haengt aber daran, dass Dateien im Mod-PBO
     * ueberhaupt lesbar sind - genau die Frage, die V-A offen laesst. Der
     * eingebaute Ruecktext ist die Versicherung dagegen.
     */
    private void EnsureFile(string profilePath, string pboTemplate, ChefZ_LoadReport report)
    {
        if (FileExist(profilePath))
            return;

        string source = ChefZ_PathTools.Resolve(pboTemplate);
        if (source != "" && CopyFile(source, profilePath) && FileExist(profilePath))
        {
            if (report)
                report.AddInfo("Vorlage angelegt: " + profilePath + " (aus " + source + ")");
            return;
        }

        if (WriteFallbackTemplate(profilePath))
        {
            if (report)
                report.AddInfo("Vorlage angelegt: " + profilePath);
            return;
        }

        // Genau EINE Meldung dazu, und die setzt EnsureLayout - hier nur der
        // Merker. Zwei Zeilen ueber dieselbe Ursache helfen niemandem.
        m_Writable = false;
    }

    /**
     * Eingebaute Minimalvorlage.
     *
     * Zeile fuer Zeile geschrieben statt ueber JsonFileLoader.SaveFile: eine
     * Serialisierung wuerde auch die internen Felder eines Records ausgeben
     * (sourceRef, sourceRank, sym) und den Betreiber einladen, daran zu drehen.
     *
     * BEWUSST OHNE erklaerende Zusatzfelder. JSON kennt keine Kommentare, und
     * 02 §8 nimmt an, dass unbekannte Felder beim Laden ignoriert werden -
     * belegt ist das fuer den Enforce-Serializer aber nirgends. Eine Vorlage,
     * die genau darauf baut, waere im schlechtesten Fall die Datei, die bei
     * JEDEM Start einen Ladefehler erzeugt. Die Erklaerung steht deshalb
     * daneben in README.txt, wo sie nichts kaputtmachen kann.
     */
    private bool WriteFallbackTemplate(string path)
    {
        FileHandle fh = OpenFile(path, FileMode.WRITE);
        if (fh == 0)
            return false;

        FPrintln(fh, "{");
        FPrintln(fh, "    \"kind\": \"coreSettings\",");
        FPrintln(fh, "    \"schemaVersion\": 1,");
        FPrintln(fh, "    \"records\": [");
        FPrintln(fh, "        {");
        FPrintln(fh, "            \"id\": \"CORE\"");
        FPrintln(fh, "        }");
        FPrintln(fh, "    ]");
        FPrintln(fh, "}");
        CloseFile(fh);
        return true;
    }

    private void EnsureReadme(ChefZ_LoadReport report)
    {
        string path = ChefZ_ProfilePaths.ReadmeFile();
        if (FileExist(path))
            return;

        FileHandle fh = OpenFile(path, FileMode.WRITE);
        if (fh == 0)
        {
            m_Writable = false;
            return;
        }

        FPrintln(fh, "ChefZ - Betreiberdaten");
        FPrintln(fh, "======================");
        FPrintln(fh, "");
        FPrintln(fh, "Dieses Verzeichnis gehoert dem Serverbetreiber. ChefZ legt hier fehlende");
        FPrintln(fh, "Vorlagen an und ueberschreibt NIE eine vorhandene Datei.");
        FPrintln(fh, "");
        FPrintln(fh, "  Core.json      Einstellungen. Patcht die Werte aus dem Mod feldweise.");
        FPrintln(fh, "  Overlay\\*.json Weitere Overlays, eine Datei je Art (\"kind\").");
        FPrintln(fh, "  Logs\\          Log und Ladebericht.");
        FPrintln(fh, "");
        FPrintln(fh, "Drei Raenge, hoeherer patcht niedrigeren feldweise:");
        FPrintln(fh, "  1  config.cpp der Addons   2  JSON im PBO   3  dieses Verzeichnis");
        FPrintln(fh, "");
        FPrintln(fh, "Regeln, die weh tun, wenn man sie nicht kennt:");
        FPrintln(fh, "  - Was hier nicht steht, bleibt unveraendert. Eine Datei muss NICHT");
        FPrintln(fh, "    vollstaendig sein - im Gegenteil, sie soll es nicht sein.");
        FPrintln(fh, "  - Ein bool (true/false) wirkt nur, wenn sein Feldname zusaetzlich in");
        FPrintln(fh, "    \"explicitFields\" steht. Grund: der JSON-Leser der Engine kann");
        FPrintln(fh, "    \"nicht gesetzt\" nicht von \"auf false gesetzt\" unterscheiden.");
        FPrintln(fh, "  - Eine Liste ersetzt die bisherige ganz, sie ergaenzt sie nicht.");
        FPrintln(fh, "  - Zustaende und Qualitaetsstufen koennen hier nicht HINZUGEFUEGT");
        FPrintln(fh, "    werden, nur ihre Felder geaendert. Sie tragen eine aus Rang 1");
        FPrintln(fh, "    abgeleitete Netzkennung, die auf Client und Server gleich sein muss.");
        FPrintln(fh, "  - Kaputtes JSON verwirft genau diese eine Datei, sonst nichts. Der");
        FPrintln(fh, "    Ladebericht im RPT nennt Datei und Parsermeldung.");
        FPrintln(fh, "  - Aenderungen wirken beim naechsten Serverstart. Es gibt in V1");
        FPrintln(fh, "    bewusst kein Neuladen zur Laufzeit.");
        CloseFile(fh);

        if (report)
            report.AddInfo("Vorlage angelegt: " + path);
    }

    //--------------------------------------------------------------------------

    override bool Read(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        m_FileCount = 0;
        if (!sink)
            return false;

        int total = 0;

        // 1. die feste Einstellungsdatei
        int fromCore = ChefZ_JsonSourceHelper.ReadInto(ChefZ_ProfilePaths.CoreSettingsFile(), GetRank(), sink, report, "Overlay");
        if (fromCore >= 0)
        {
            m_FileCount++;
            total = total + fromCore;
        }

        // 2. alles unter Overlay\
        array<string> files = ListOverlayFiles();
        for (int i = 0; i < files.Count(); i++)
        {
            int count = ChefZ_JsonSourceHelper.ReadInto(files.Get(i), GetRank(), sink, report, "Overlay");
            if (count < 0)
                continue;
            m_FileCount++;
            total = total + count;
        }

        return total > 0;
    }

    /**
     * Dateien in $profile:ChefZ\Overlay.
     *
     * Aufrufmuster woertlich aus 5_Mission/DayZ/GUI/NewUI/VideoPlayer.c:74-93:
     * kein Handle-Nullcheck (FindFileHandle ist typedef int[]), erst fileName
     * pruefen, dann FindNextFile, am Ende immer CloseFindFile.
     *
     * FindFile liefert den blossen Dateinamen, nicht den Pfad - deshalb wird
     * das Verzeichnis wieder davorgesetzt.
     */
    private array<string> ListOverlayFiles()
    {
        array<string> found = new array<string>();

        string   fileName;
        FileAttr fileAttr;
        FindFileHandle handle = FindFile(ChefZ_ProfilePaths.OverlayPattern(), fileName, fileAttr, FindFileFlags.DIRECTORIES);

        if (fileName != "")
            found.Insert(ChefZ_ProfilePaths.OverlayDir() + "\\" + fileName);

        // Deckel: ein Overlayverzeichnis mit tausenden Dateien ist ein Unfall,
        // kein Betriebsfall.
        while (found.Count() < 256 && FindNextFile(handle, fileName, fileAttr))
        {
            if (fileName != "")
                found.Insert(ChefZ_ProfilePaths.OverlayDir() + "\\" + fileName);
        }

        CloseFindFile(handle);

        // Stabile Reihenfolge: die Aufzaehlungsreihenfolge des Dateisystems ist
        // keine Zusage, und "erste gewinnt" (02 §8) braucht eine.
        ChefZ_StringOrder.SortAscending(found);
        return found;
    }
}
