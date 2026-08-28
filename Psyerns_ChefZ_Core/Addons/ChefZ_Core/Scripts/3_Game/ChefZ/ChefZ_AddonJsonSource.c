//==============================================================================
// ChefZ_JsonSourceHelper - eine JSON-Datei einlesen und einreichen
// ChefZ_AddonJsonSource - Rang 2: Addon-JSON im PBO, per Manifest benannt
//
// Entwurf: 02 §3 (Rang 2), 02 §4 (nie gescannt, immer deklariert - 01 V8),
// 02 §8 (Fehlertabelle), 03 §4 (Rang 3 darf sync-relevante Arten nicht
// erweitern).
//
// Der Helfer ist mit dem Overlay geteilt, weil beide Raenge dasselbe
// Dateiformat lesen. Unterschiedlich ist nur, WOHER die Pfade kommen und was
// eine fehlende Datei bedeutet.
//
// ---------------------------------------------------------------------------
// OFFENER PUNKT, ehrlich benannt: V-A ist noch nicht gemessen.
//
// Tests/V_A_PboJsonSmoke/README.md §5 ist leer. Belegt ist damit nur das Lesen
// aus dz/-PBOs (01 V8); das Lesen aus einem MOD-PBO ist stark indiziert
// (drei ausgelieferte Fremdmods, siehe README §0), aber nicht auf dem
// Zielbuild gemessen.
//
// Diese Klasse ist so gebaut, dass beide Ausgaenge tragen:
//   Messung positiv  -> nichts zu tun.
//   Messung negativ  -> diese Quelle liefert null Records und meldet je Datei
//                       einen Fehler mit den geprueften Pfadformen. Rang 2
//                       wandert dann laut 02 E7 in ChefZ_ConfigCppSource, und
//                       zwar OHNE Aenderung an Sink, Registry oder Manager.
// Zusaetzlich meldet der erste erfolgreiche Treffer die tragende Pfadform ins
// DEBUG-Log - damit beantwortet der erste Serverstart die Frage im Betrieb.
// ---------------------------------------------------------------------------
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_JsonSourceHelper
{
    //! Hoechste Dokumentversion, die dieser Core kennt.
    static const int SCHEMA_VERSION = 1;

    private static bool s_PathFormReported;

    /**
     * Liest eine Datei und reicht ihre Records ein.
     *
     * @return Anzahl eingereichter Records. -1, wenn die Datei nicht gelesen
     *         werden konnte (der Aufrufer entscheidet, ob das ein Fehler ist).
     */
    static int ReadInto(string declaredPath, int rank, ChefZ_RecordSink sink, ChefZ_LoadReport report, string sliceName)
    {
        string resolved = ChefZ_PathTools.Resolve(declaredPath);
        if (resolved == "")
            return -1;

        ReportPathForm(declaredPath, resolved);

        string text = ChefZ_JsonText.ReadWhole(resolved);
        if (text == "")
        {
            // Existiert, ist aber nicht lesbar oder leer. Beides ist ein
            // Datenfehler, kein Grund abzubrechen (02 §8).
            AddError(report, resolved, "", "Datei existiert, liefert aber keinen Inhalt - uebersprungen.");
            return 0;
        }

        string kind = ChefZ_JsonText.ExtractString(text, "kind");
        if (kind == "")
        {
            AddError(report, resolved, "", "Feld \"kind\" fehlt im Dokumentkopf - die Art der Datensaetze ist damit " + "unbestimmt und die Datei wird verworfen. Gueltig: " + ChefZ_RecordKind.ValidNames());
            return 0;
        }
        if (!ChefZ_RecordKind.IsKnown(kind))
        {
            AddError(report, resolved, "", "Unbekannte Art \"" + kind + "\" - Datei verworfen. Gueltig: " + ChefZ_RecordKind.ValidNames());
            return 0;
        }

        int schema = ChefZ_JsonText.ExtractInt(text, "schemaVersion", SCHEMA_VERSION);
        if (schema > SCHEMA_VERSION)
        {
            // 02 §8: laden, warnen, unbekannte Felder ignorieren. Ein harter
            // Abbruch hiesse, dass ein Core-Update den Content blockiert.
            AddWarn(report, resolved, "", "schemaVersion " + schema.ToString() + " ist neuer als dieser Core (" + SCHEMA_VERSION.ToString() + "). Die Datei wird geladen, unbekannte Felder " + "werden ignoriert.");
        }

        array<ref ChefZ_Record> records = new array<ref ChefZ_Record>();
        string parseError;
        if (!ChefZ_JsonRecordReader.Read(kind, text, resolved, rank, records, parseError))
        {
            // 02 §8: GANZE Datei verworfen, nichts halb angewandt, rangniedrigere
            // Quelle bleibt gueltig, Datei wird nicht angefasst.
            AddError(report, resolved, "", "JSON nicht lesbar - die gesamte Datei wird verworfen, alles bereits " + "Geladene bleibt gueltig. Parsermeldung: " + parseError);
            return 0;
        }

        int submitted = 0;
        for (int i = 0; i < records.Count(); i++)
        {
            ChefZ_Record rec = records.Get(i);
            if (!rec)
                continue;

            if (rank >= ChefZ_SourceRank.PROFILE_OVERLAY)
            {
                rec.Normalize();
                if (sink.IsForbiddenOverlayAddition(kind, rec.id))
                {
                    sink.RejectOverlayAddition(kind, rec.id, resolved);
                    continue;
                }
            }

            sink.Submit(rec);
            submitted++;
        }

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.CONFIG, sliceName + ": " + ChefZ_PathTools.FileName(resolved) + " -> " + submitted.ToString() + " Records der Art \"" + kind + "\"");
        }
        return submitted;
    }

    /**
     * Meldet EINMAL, welche Pfadform tatsaechlich getragen hat.
     *
     * Das ist die Messung, die V-A offen gelassen hat - hier faellt sie im
     * Betrieb an, kostet eine Zeile und beantwortet die Frage fuer den
     * Gate-1-Report.
     */
    private static void ReportPathForm(string declared, string resolved)
    {
        if (s_PathFormReported)
            return;
        s_PathFormReported = true;

        ChefZ_Log.Once(ChefZ_LogLevel.INFO, ChefZ_LogChannel.CONFIG, "config.pathform", "Erste gelesene Datendatei: deklariert \"" + declared + "\", gelesen als \"" + resolved + "\". Damit ist die tragende Pfadform belegt (V-A / 02 E7).");
    }

    static void ResetPathFormReport()
    {
        s_PathFormReported = false;
    }

    private static void AddError(ChefZ_LoadReport report, string src, string id, string msg)
    {
        if (report)
            report.AddError(src, id, msg);
    }

    private static void AddWarn(ChefZ_LoadReport report, string src, string id, string msg)
    {
        if (report)
            report.AddWarn(src, id, msg);
    }
}

//==============================================================================

/**
 * Rang 2: die im Manifest eines Slice benannten Dateien.
 *
 * Eine Instanz je Slice. Kein Verzeichnisscan (02 §4, 01 V8: FindFileFlags
 * kennt nur DIRECTORIES und ARCHIVES(.pak), PBO-Inhalte sind nicht
 * aufzaehlbar) - die Pfade stehen im Manifest, sonst nirgends.
 */
class ChefZ_AddonJsonSource extends ChefZ_IRecordSource
{
    private string m_SliceName;
    private ref array<string> m_Files;
    private int  m_FileCount;
    private bool m_MissingIsWarnOnly;

    void ChefZ_AddonJsonSource()
    {
        m_SliceName         = "?";
        m_Files             = new array<string>();
        m_FileCount         = 0;
        m_MissingIsWarnOnly = false;
    }

    /**
     * @param missingIsWarnOnly  true nur fuer Dateien, deren Inhalt der Core
     *        vollstaendig als Code-Default kennt (Core.json). Fuer alles andere
     *        ist eine im Manifest genannte, aber fehlende Datei ein ERROR
     *        (02 §8) - dort geht echter Inhalt verloren.
     */
    void Init(string sliceName, array<string> files, bool missingIsWarnOnly)
    {
        m_SliceName         = sliceName;
        m_MissingIsWarnOnly = missingIsWarnOnly;
        m_Files.Clear();
        if (files)
        {
            for (int i = 0; i < files.Count(); i++)
                m_Files.Insert(files.Get(i));
        }
    }

    void InitFromSlice(notnull ChefZ_SliceManifest slice)
    {
        Init(slice.name, slice.dataFiles, false);
    }

    override string GetName()
    {
        return m_SliceName;
    }

    override int GetRank()
    {
        return ChefZ_SourceRank.ADDON_JSON;
    }

    override int GetFileCount()
    {
        return m_FileCount;
    }

    int GetDeclaredFileCount()
    {
        return m_Files.Count();
    }

    //--------------------------------------------------------------------------

    override bool Read(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        m_FileCount = 0;
        if (!sink)
            return false;

        int total = 0;
        for (int i = 0; i < m_Files.Count(); i++)
        {
            string declared = m_Files.Get(i);
            int count = ChefZ_JsonSourceHelper.ReadInto(declared, GetRank(), sink, report, m_SliceName);

            if (count < 0)
            {
                ReportMissing(declared, report);
                continue;
            }

            m_FileCount++;
            total = total + count;
        }
        return total > 0;
    }

    private void ReportMissing(string declared, ChefZ_LoadReport report)
    {
        if (!report)
            return;

        string where = "CfgChefZ " + m_SliceName;
        string what  = "Im Manifest genannte Datei ist nicht lesbar. Geprueft wurde "
                     + ChefZ_PathTools.TriedForms(declared) + ".";

        if (m_MissingIsWarnOnly)
        {
            report.AddWarn(where, "", what + " Der Core hat fuer diesen Inhalt vollstaendige Code-Defaults und " + "laeuft ohne Einschraenkung weiter.");
            return;
        }

        report.AddError(where, "", what + " Der Rest des Slice wird geladen; die Datensaetze dieser Datei fehlen. " + "Faellt das fuer JEDE Datei an, ist die Ursache fast immer die Pfadform " + "(die Wurzel eines Laufzeitpfades ist das PBO-Praefix, siehe 02 E7).");
    }
}
