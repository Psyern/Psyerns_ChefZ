//==============================================================================
// ChefZ_*Doc / ChefZ_JsonText / ChefZ_JsonRecordReader
//
// Entwurf: 02 §3 (Rang 2 und 3 sind JSON), 02 §8 (Fehlerverhalten beim Lesen),
// 02 E3 (feldweiser Patch -> Bool-Sonde), 01 V8 (JsonFileLoader ist der Weg).
//
// Dokumentform, fuer alle Datendateien identisch:
//
//   {
//       "kind": "category",
//       "schemaVersion": 1,
//       "records": [ { "id": "..." }, ... ]
//   }
//
// Ein Dokument traegt genau EINE Art. Das ist die Bedingung dafuer, dass der
// Serializer ueberhaupt typisiert lesen kann: JsonFileLoader<T> braucht einen
// konkreten Zieltyp, und ein gemischtes Dokument haette keinen.
//
// Warum je Art eine eigene Doc-Klasse und keine gemeinsame Basis: Enforce
// deserialisiert ueber die Felder des angegebenen Typs. Ob geerbte Felder dabei
// zuverlaessig mitgezaehlt werden, ist nirgends zugesichert und in Vanilla
// nirgends belegt - die JSON-Klassen dort sind ausnahmslos flach. Fuenfzehn
// flache Vierzeiler sind billiger als ein Ladefehler, den niemand erklaeren
// kann.
//
// Gelesen wird JEDE Datei genau einmal (Rohtext), danach zweimal geparst -
// einmal je Bool-Polaritaet (siehe ChefZ_Record, Bool-Sonde). Das ersetzt
// JsonFileLoader.LoadFile durch LoadData; das Dateilesen macht diese Klasse
// selbst, sonst laege die Datei dreimal auf dem Datentraeger.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_CoreSettingsDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_CoreSettingsDef> records;
}

class ChefZ_CategoryDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_CategoryDef> records;
}

class ChefZ_TagDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_TagDef> records;
}

class ChefZ_StateDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_StateDef> records;
}

class ChefZ_QualityTierDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_QualityTierDef> records;
}

class ChefZ_ToolGroupDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_ToolGroupDef> records;
}

class ChefZ_DeviceDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_DeviceDef> records;
}

class ChefZ_ContainerDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_ContainerDef> records;
}

class ChefZ_IngredientDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_IngredientDef> records;
}

class ChefZ_NutritionDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_NutritionDef> records;
}

class ChefZ_PreservationDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_PreservationDef> records;
}

class ChefZ_ProcessDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_ProcessDef> records;
}

class ChefZ_StationDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_StationDef> records;
}

class ChefZ_TransformDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_TransformDef> records;
}

class ChefZ_RecipeDoc
{
    string kind;
    int    schemaVersion;
    ref array<ref ChefZ_RecipeDef> records;
}

//==============================================================================

/**
 * Rohtextzugriff und Feld-Sonde.
 *
 * Die Sonde beantwortet EINE Frage vor dem eigentlichen Parsen: welche Art
 * steht in diesem Dokument? Sie muss beantwortet sein, bevor ein Zieltyp
 * feststeht - das ist die Henne-Ei-Lage jedes typisierten Deserialisierers.
 *
 * Bewusst KEIN eigener JSON-Parser (02 E3: "die einzige Variante ohne
 * Eigenparser"). Es wird nur eine Zeichenkette gesucht; alles andere macht der
 * Engine-Serializer. Findet die Sonde nichts, ist das ein sauberer Fehler mit
 * Dateinamen - kein Ratespiel.
 */
class ChefZ_JsonText
{
    //! Leseobergrenze. Vanilla nimmt in JsonFileLoader 100 MB; das ist fuer
    //! eine Rezeptdatei absurd viel und im Fehlerfall genau die Menge
    //! Speicher, die man nicht anfassen will. 8 MB sind reichlich.
    static const int MAX_READ = 8388608;

    //! Rohtext oder Leerstring. Der Aufrufer unterscheidet "nicht da" von
    //! "leer" ueber ChefZ_PathTools.Resolve().
    static string ReadWhole(string resolvedPath)
    {
        FileHandle handle = OpenFile(resolvedPath, FileMode.READ);
        if (handle == 0)
            return "";

        string content;
        ReadFile(handle, content, MAX_READ);
        CloseFile(handle);
        return content;
    }

    /**
     * Wert eines Stringfeldes der obersten Ebene, z.B. "kind".
     *
     * Bewusst simpel: erstes Vorkommen von "<feld>", danach der naechste in
     * Anfuehrungszeichen stehende Wert nach dem Doppelpunkt. Fuer den Kopf
     * eines Dokuments reicht das - und wenn nicht, faellt es sofort auf, weil
     * die Art dann unbekannt ist und gemeldet wird.
     */
    static string ExtractString(string text, string field)
    {
        string needle = "\"" + field + "\"";
        int at = text.IndexOf(needle);
        if (at < 0)
            return "";

        int i = at + needle.Length();
        int n = text.Length();

        // Doppelpunkt suchen
        while (i < n && text.Get(i) != ":")
        {
            string c = text.Get(i);
            if (c != " " && c != "\t" && c != "\n" && c != "\r")
                return "";              // etwas anderes als Zwischenraum -> kein Feld
            i++;
        }
        if (i >= n)
            return "";
        i++;

        // oeffnendes Anfuehrungszeichen
        while (i < n && text.Get(i) != "\"")
        {
            string w = text.Get(i);
            if (w != " " && w != "\t" && w != "\n" && w != "\r")
                return "";              // Zahl oder Objekt, kein String
            i++;
        }
        if (i >= n)
            return "";
        i++;

        string value = "";
        while (i < n)
        {
            string ch = text.Get(i);
            if (ch == "\"")
                break;
            value = value + ch;
            i++;
        }
        return value;
    }

    /**
     * Ganzzahl eines Feldes der obersten Ebene. fallback, wenn es fehlt.
     *
     * Nur fuer schemaVersion gedacht; alles andere liest der Serializer.
     */
    static int ExtractInt(string text, string field, int fallback)
    {
        string needle = "\"" + field + "\"";
        int at = text.IndexOf(needle);
        if (at < 0)
            return fallback;

        int i = at + needle.Length();
        int n = text.Length();
        while (i < n && text.Get(i) != ":")
            i++;
        if (i >= n)
            return fallback;
        i++;

        string digits = "";
        while (i < n)
        {
            string ch = text.Get(i);
            if (ch == " " || ch == "\t" || ch == "\n" || ch == "\r")
            {
                if (digits != "")
                    break;
                i++;
                continue;
            }
            // Zeichenvergleich ueber den Code: "<" und ">" sind fuer string in
            // Enforce nicht zugesichert, ToAscii ist es.
            int code = ch.ToAscii();
            if (ch == "-" || (code >= 48 && code <= 57))
            {
                digits = digits + ch;
                i++;
                continue;
            }
            break;
        }
        if (digits == "")
            return fallback;
        return digits.ToInt();
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        string doc = "{ \"kind\": \"category\", \"schemaVersion\": 3, \"records\": [] }";
        if (ExtractString(doc, "kind") != "category")           return false;
        if (ExtractString(doc, "gibtsNicht") != "")             return false;
        if (ExtractInt(doc, "schemaVersion", 1) != 3)           return false;
        if (ExtractInt(doc, "gibtsNicht", 7) != 7)              return false;
        // Zahlenfeld ist kein Stringfeld
        if (ExtractString(doc, "schemaVersion") != "")          return false;
        return true;
    }
}

//==============================================================================

/**
 * Baut aus einem JSON-Text die Recordliste einer Art.
 *
 * Zwei Parsedurchgaenge je Dokument, mit umgekehrter Bool-Polaritaet. Der
 * erste Durchgang liefert die Records, der zweite nur den Vergleichswert fuer
 * die Bool-Sonde (ChefZ_Record, Kopfkommentar). Schlaegt der ZWEITE Durchgang
 * fehl, obwohl der erste lief, wird das nicht zum Fehler erklaert - dann
 * fehlen lediglich die automatisch erkannten bool-Felder, und ein
 * handgeschriebenes explicitFields[] wirkt weiterhin.
 */
class ChefZ_JsonRecordReader
{
    /**
     * @param kind       Art laut Dokumentkopf, bereits als bekannt geprueft
     * @param text       Rohtext des Dokuments
     * @param sourceRef  Herkunft fuer Log und Bericht
     * @param rank       ChefZ_SourceRank.*
     * @param outRecords wird befuellt, nie null
     * @param errorOut   Parsermeldung, wenn false
     */
    static bool Read(string kind, string text, string sourceRef, int rank, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        if (!outRecords)
            outRecords = new array<ref ChefZ_Record>();

        bool ok = false;
        ChefZ_RecordProbe.Set(false);

        if (kind == ChefZ_RecordKind.CORE_SETTINGS)      ok = ReadCoreSettings(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.CATEGORY)      ok = ReadCategory(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.TAG)           ok = ReadTag(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.STATE)         ok = ReadState(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.QUALITY_TIER)  ok = ReadQualityTier(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.TOOL_GROUP)    ok = ReadToolGroup(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.DEVICE)        ok = ReadDevice(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.CONTAINER)     ok = ReadContainer(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.INGREDIENT)    ok = ReadIngredient(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.NUTRITION)     ok = ReadNutrition(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.PRESERVATION)  ok = ReadPreservation(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.PROCESS)       ok = ReadProcess(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.STATION)       ok = ReadStation(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.TRANSFORM)     ok = ReadTransform(text, outRecords, errorOut);
        else if (kind == ChefZ_RecordKind.RECIPE)        ok = ReadRecipe(text, outRecords, errorOut);
        else                                             errorOut = "Unbekannte Art \"" + kind + "\"";

        ChefZ_RecordProbe.Reset();

        if (!ok)
        {
            // 02 §8: kaputte Datei -> GANZE Datei verworfen, nichts halb
            // angewandt. Deshalb wird outRecords geleert und nicht teilweise
            // uebergeben.
            outRecords.Clear();
            return false;
        }

        for (int i = 0; i < outRecords.Count(); i++)
            outRecords.Get(i).SetOrigin(sourceRef, rank);

        return true;
    }

    //--------------------------------------------------------------------------
    // Je Art: Durchgang A (Polaritaet false) liefert die Records, Durchgang B
    // (Polaritaet true) nur den Vergleich fuer die Bool-Sonde.
    //--------------------------------------------------------------------------

    private static bool ReadCoreSettings(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_CoreSettingsDoc a = new ChefZ_CoreSettingsDoc();
        if (!JsonFileLoader<ChefZ_CoreSettingsDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_CoreSettingsDoc b = new ChefZ_CoreSettingsDoc();
        bool probeOk = JsonFileLoader<ChefZ_CoreSettingsDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_CoreSettingsDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadCategory(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_CategoryDoc a = new ChefZ_CategoryDoc();
        if (!JsonFileLoader<ChefZ_CategoryDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_CategoryDoc b = new ChefZ_CategoryDoc();
        bool probeOk = JsonFileLoader<ChefZ_CategoryDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_CategoryDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadTag(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_TagDoc a = new ChefZ_TagDoc();
        if (!JsonFileLoader<ChefZ_TagDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_TagDoc b = new ChefZ_TagDoc();
        bool probeOk = JsonFileLoader<ChefZ_TagDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_TagDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadState(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_StateDoc a = new ChefZ_StateDoc();
        if (!JsonFileLoader<ChefZ_StateDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_StateDoc b = new ChefZ_StateDoc();
        bool probeOk = JsonFileLoader<ChefZ_StateDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_StateDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadQualityTier(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_QualityTierDoc a = new ChefZ_QualityTierDoc();
        if (!JsonFileLoader<ChefZ_QualityTierDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_QualityTierDoc b = new ChefZ_QualityTierDoc();
        bool probeOk = JsonFileLoader<ChefZ_QualityTierDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_QualityTierDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadToolGroup(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_ToolGroupDoc a = new ChefZ_ToolGroupDoc();
        if (!JsonFileLoader<ChefZ_ToolGroupDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_ToolGroupDoc b = new ChefZ_ToolGroupDoc();
        bool probeOk = JsonFileLoader<ChefZ_ToolGroupDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_ToolGroupDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadDevice(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_DeviceDoc a = new ChefZ_DeviceDoc();
        if (!JsonFileLoader<ChefZ_DeviceDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_DeviceDoc b = new ChefZ_DeviceDoc();
        bool probeOk = JsonFileLoader<ChefZ_DeviceDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_DeviceDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadContainer(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_ContainerDoc a = new ChefZ_ContainerDoc();
        if (!JsonFileLoader<ChefZ_ContainerDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_ContainerDoc b = new ChefZ_ContainerDoc();
        bool probeOk = JsonFileLoader<ChefZ_ContainerDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_ContainerDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadIngredient(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_IngredientDoc a = new ChefZ_IngredientDoc();
        if (!JsonFileLoader<ChefZ_IngredientDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_IngredientDoc b = new ChefZ_IngredientDoc();
        bool probeOk = JsonFileLoader<ChefZ_IngredientDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_IngredientDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadNutrition(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_NutritionDoc a = new ChefZ_NutritionDoc();
        if (!JsonFileLoader<ChefZ_NutritionDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_NutritionDoc b = new ChefZ_NutritionDoc();
        bool probeOk = JsonFileLoader<ChefZ_NutritionDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_NutritionDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadPreservation(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_PreservationDoc a = new ChefZ_PreservationDoc();
        if (!JsonFileLoader<ChefZ_PreservationDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_PreservationDoc b = new ChefZ_PreservationDoc();
        bool probeOk = JsonFileLoader<ChefZ_PreservationDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_PreservationDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadProcess(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_ProcessDoc a = new ChefZ_ProcessDoc();
        if (!JsonFileLoader<ChefZ_ProcessDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_ProcessDoc b = new ChefZ_ProcessDoc();
        bool probeOk = JsonFileLoader<ChefZ_ProcessDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_ProcessDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadStation(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_StationDoc a = new ChefZ_StationDoc();
        if (!JsonFileLoader<ChefZ_StationDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_StationDoc b = new ChefZ_StationDoc();
        bool probeOk = JsonFileLoader<ChefZ_StationDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_StationDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadTransform(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        // Die drei Spuren sind kein Beiwerk. Der Deserialisierer der Engine ist
        // nativ: geht er unter, gibt es KEINE Skriptausnahme und keinen
        // Aufrufkeller, nur einen beendeten Serverprozess. Ohne diese Zeilen
        // sieht man am 28.08.2026 nur, dass irgendwo zwischen zwei Dateien
        // Schluss war.
        ChefZ_Log.Trace(ChefZ_LogChannel.CONFIG, "ReadTransform: Durchgang 1 (Werte) beginnt");

        ChefZ_TransformDoc a = new ChefZ_TransformDoc();
        if (!JsonFileLoader<ChefZ_TransformDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_Log.Trace(ChefZ_LogChannel.CONFIG, "ReadTransform: Durchgang 1 ok, " + a.records.Count().ToString() + " Records - Durchgang 2 (Sonde) beginnt");

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_TransformDoc b = new ChefZ_TransformDoc();
        bool probeOk = JsonFileLoader<ChefZ_TransformDoc>.LoadData(text, b, ignored);

        ChefZ_Log.Trace(ChefZ_LogChannel.CONFIG, "ReadTransform: Durchgang 2 beendet, probeOk=" + probeOk.ToString());

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_TransformDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    private static bool ReadRecipe(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_RecipeDoc a = new ChefZ_RecipeDoc();
        if (!JsonFileLoader<ChefZ_RecipeDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_RecordProbe.Set(true);
        string ignored;
        ChefZ_RecipeDoc b = new ChefZ_RecipeDoc();
        bool probeOk = JsonFileLoader<ChefZ_RecipeDoc>.LoadData(text, b, ignored);

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_RecipeDef rec = a.records.Get(i);
            if (probeOk && b.records && i < b.records.Count())
                rec.CaptureExplicitBools(b.records.Get(i));
            outRecords.Insert(rec);
        }
        return true;
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest. Prueft den Fehlerpfad OHNE Datei: kaputter
    //! Text -> false, leere Recordliste, gefuellte Parsermeldung.
    static bool SelfCheck()
    {
        array<ref ChefZ_Record> recs = new array<ref ChefZ_Record>();
        string err;

        // 1. gueltiges Dokument
        string good = "{ \"kind\": \"category\", \"schemaVersion\": 1, \"records\": [" + "{ \"id\": \"CHEFZ_ST_A\", \"parent\": \"CHEFZ_ST_ROOT\" } ] }";
        if (!Read(ChefZ_RecordKind.CATEGORY, good, "Selbsttest", ChefZ_SourceRank.ADDON_JSON, recs, err))
            return false;
        if (recs.Count() != 1)                                  return false;
        ChefZ_CategoryDef c = ChefZ_CategoryDef.Cast(recs.Get(0));
        if (!c)                                                 return false;
        if (c.id != "CHEFZ_ST_A")                               return false;
        if (c.parent != "CHEFZ_ST_ROOT")                        return false;
        if (c.sourceRank != ChefZ_SourceRank.ADDON_JSON)        return false;

        // 2. kaputtes Dokument -> false, nichts halb angewandt
        array<ref ChefZ_Record> broken = new array<ref ChefZ_Record>();
        string err2;
        string bad = "{ \"kind\": \"category\", \"records\": [ { \"id\": ";
        if (Read(ChefZ_RecordKind.CATEGORY, bad, "Selbsttest", ChefZ_SourceRank.ADDON_JSON, broken, err2))
            return false;
        if (broken.Count() != 0)                                return false;

        // 3. Bool-Sonde: gesetztes bool landet in explicitFields
        array<ref ChefZ_Record> withBool = new array<ref ChefZ_Record>();
        string err3;
        string doc = "{ \"kind\": \"container\", \"records\": [" + "{ \"id\": \"CHEFZ_ST_BOWL\", \"reusable\": false }," + "{ \"id\": \"CHEFZ_ST_CUP\" } ] }";
        if (!Read(ChefZ_RecordKind.CONTAINER, doc, "Selbsttest", ChefZ_SourceRank.PROFILE_OVERLAY, withBool, err3))
            return false;
        if (withBool.Count() != 2)                              return false;
        ChefZ_ContainerDef contDef   = ChefZ_ContainerDef.Cast(withBool.Get(0));
        ChefZ_ContainerDef unset = ChefZ_ContainerDef.Cast(withBool.Get(1));
        if (!contDef || !unset)                                     return false;
        if (!contDef.HasExplicit("reusable"))                       return false;
        if (unset.HasExplicit("reusable"))                      return false;

        return true;
    }
}
