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
 * EIN Parsedurchgang je Dokument. Frueher waren es zwei, mit umgekehrter
 * Bool-Polaritaet (die Bool-Sonde, ChefZ_RecordProbe): ein Feld, das in
 * beiden Durchgaengen gleich war, galt als geschrieben. Das setzte voraus,
 * dass der Konstruktor die Polaritaet in den Record traegt - und genau den
 * ruft der Serializer nie (Kopf von ChefZ_JsonExplicit). Beide Durchgaenge
 * lieferten deshalb dasselbe, und die Sonde markierte JEDES abwesende bool
 * als ausdrueckliches false: jeder "Default true"-Schalter in Slots und
 * Ergebnissen (allowPartial, inheritState, inheritFreshness, ...) kippte
 * still auf false, und das Rang-3-Overlay { "id": "CORE" } patchte enabled
 * auf false. Die Explizitheit kommt jetzt fuer alle Felder, bool
 * eingeschlossen, aus dem Text - auf jeder Tiefe.
 */
/**
 * Traegt in jeden Record ein, welche Schluessel im JSON WIRKLICH geschrieben
 * stehen.
 *
 * ---------------------------------------------------------------------------
 * Warum das noetig ist
 * ---------------------------------------------------------------------------
 * Der JsonSerializer ruft den Skriptkonstruktor nicht auf. Nachgewiesen am
 * 28.08.2026: eine TRACE-Zeile in ChefZ_CoreSettingsDef() erschien kein
 * einziges Mal, in einem Lauf mit 162 anderen TRACE-Zeilen. Die
 * Sentinel-Vorbelegung aus 02 E3, Mittel 2, kommt also nie zustande - jedes
 * fehlende Feld erreicht uns als 0, "" oder false.
 *
 * Damit war die Rangordnung auf den Kopf gestellt: das Rang-3-Overlay
 * ueberschrieb jede Einstellung, die der Betreiber NICHT geschrieben hatte. Die
 * mitgelieferte Overlay-Vorlage enthaelt nur { "id": "CORE" } - und genau sie
 * klemmte safeModeErrorThreshold auf 1 und legte den Core lahm.
 *
 * Die Antwort steht in 02 E3 selbst, bei Mittel 3: explicitFields[]. Bisher
 * musste ein Autor das von Hand schreiben. Hier wird es aus dem Text
 * abgeleitet, und damit gilt Mittel 3 fuer JEDES Feld, ohne Zutun.
 *
 * ---------------------------------------------------------------------------
 * Warum ueber den Rohtext und nicht ueber die Bool-Sonde
 * ---------------------------------------------------------------------------
 * Die Sonde (ChefZ_RecordProbe) liest ihren Ausgangswert ebenfalls im
 * Konstruktor. Sie kann aus demselben Grund nicht funktionieren. Der Text ist
 * die einzige Quelle, die uebrig bleibt - und die einzige, die die Frage
 * "steht das Feld da?" ueberhaupt beantwortet, statt sie aus einem Wert zu
 * erraten.
 */
class ChefZ_JsonExplicit
{
    /**
     * Zuordnung ueber die REIHENFOLGE, nicht ueber die ID.
     *
     * Die Leser haengen ihre Records in Dokumentreihenfolge an, und die Zahl
     * muss uebereinstimmen; tut sie es nicht, wird gar nichts eingetragen. Das
     * ist die vorsichtige Wahl: ein falsch zugeordnetes explicitFields[] waere
     * schlimmer als keines, weil es einen fremden Wert durch die Rangordnung
     * traegt.
     *
     * -----------------------------------------------------------------------
     * Pfade statt Namen
     * -----------------------------------------------------------------------
     * Ein Schluessel auf Recordebene wird unter seinem Namen eingetragen
     * ("minCount"). Ein Schluessel in einem Unterobjekt unter seinem PFAD:
     * "slots[2].minCount", "outputs[0].inheritState", "priorityWeights.wTag",
     * "slots[0].match.category". Der Record verteilt die Pfade danach an
     * seine Kinder (ChefZ_Record.DistributeExplicitPaths) - ein Slot fragt
     * dann sein eigenes explicitFields[] nach "minCount", wie bisher.
     *
     * Ohne die Pfade galt Mittel 3 nur fuer die oberste Ebene, und jedes
     * Unterobjekt musste eine geschriebene 0 oder ein geschriebenes false am
     * Wert erraten - was seit ChefZ_Undefined.FLOAT == 0.0 nicht mehr geht.
     *
     * Der Laeufer fuehrt einen Rahmenkeller: je offener Klammer ein Rahmen.
     * Ein Objektrahmen kennt seinen Pfad, ein Arrayrahmen seinen Pfad und den
     * laufenden Index. Drei parallele Listen statt einer Rahmenklasse, weil es
     * genau drei Werte sind und die Klasse nur Zeilen kostete.
     */
    static void Apply(string text, notnull array<ref ChefZ_Record> records)
    {
        if (records.Count() == 0)
            return;

        int start = text.IndexOf("\"records\"");
        if (start < 0)
            return;

        int len = text.Length();
        int i = start;
        while (i < len && text.Get(i) != "[")
            i++;
        if (i >= len)
            return;

        i++;                        // hinter die oeffnende Klammer

        // Rahmen 0 ist records[] selbst: Pfad "", Index = laufender Record.
        array<string> framePath    = new array<string>();
        array<bool>   frameIsArray = new array<bool>();
        array<int>    frameIndex   = new array<int>();
        framePath.Insert("");
        frameIsArray.Insert(true);
        frameIndex.Insert(0);

        string lastKey = "";
        bool inStr     = false;
        bool esc       = false;
        string token   = "";

        while (i < len)
        {
            string c = text.Get(i);
            int top = framePath.Count() - 1;

            if (inStr)
            {
                if (esc)
                {
                    esc = false;
                    token = token + c;
                    i++;
                    continue;
                }
                if (c == "\\")
                {
                    esc = true;
                    i++;
                    continue;
                }
                if (c == "\"")
                {
                    inStr = false;
                    // Ein String in einem Objektrahmen, dem ein ":" folgt, ist
                    // ein Schluessel. Alles andere ist ein Wert.
                    if (!frameIsArray.Get(top))
                    {
                        int j = i + 1;
                        while (j < len && IsSpace(text.Get(j)))
                            j++;
                        if (j < len && text.Get(j) == ":")
                        {
                            lastKey = token;
                            int recIndex = frameIndex.Get(0);
                            if (recIndex < records.Count())
                            {
                                ChefZ_Record rec = records.Get(recIndex);
                                if (rec)
                                    rec.MarkExplicit(JoinPath(framePath.Get(top), token));
                            }
                        }
                    }
                    token = "";
                    i++;
                    continue;
                }
                token = token + c;
                i++;
                continue;
            }

            if (c == "\"")      { inStr = true; token = ""; i++; continue; }

            if (c == "{" || c == "[")
            {
                framePath.Insert(ChildPath(framePath.Get(top), frameIsArray.Get(top), frameIndex.Get(top), lastKey));
                frameIsArray.Insert(c == "[");
                frameIndex.Insert(0);
                i++;
                continue;
            }

            if (c == "}" || c == "]")
            {
                framePath.Remove(top);
                frameIsArray.Remove(top);
                frameIndex.Remove(top);
                if (framePath.Count() == 0)
                    return;         // records[] ist zu Ende
                i++;
                continue;
            }

            if (c == "," && frameIsArray.Get(top))
                frameIndex.Set(top, frameIndex.Get(top) + 1);

            i++;
        }
    }

    //! Pfad des Kindes, das im Rahmen (parentPath, parentIsArray, parentIndex)
    //! gerade beginnt. Unter records[] hat der Record selbst den Pfad "".
    private static string ChildPath(string parentPath, bool parentIsArray, int parentIndex, string lastKey)
    {
        if (parentIsArray)
        {
            if (parentPath == "")
                return "";
            return parentPath + "[" + parentIndex.ToString() + "]";
        }
        return JoinPath(parentPath, lastKey);
    }

    private static string JoinPath(string path, string key)
    {
        if (path == "")
            return key;
        return path + "." + key;
    }

    private static bool IsSpace(string c)
    {
        return c == " " || c == "\t" || c == "\n" || c == "\r";
    }
}

//==============================================================================

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

        // Erst die geschriebenen Schluessel eintragen, dann die Herkunft. Ohne
        // diesen Schritt bleibt explicitFields[] leer, und die Rangordnung
        // koennte "nicht geschrieben" nicht von "auf 0 geschrieben"
        // unterscheiden - siehe den Kopf von ChefZ_JsonExplicit.
        ChefZ_JsonExplicit.Apply(text, outRecords);

        for (int i = 0; i < outRecords.Count(); i++)
        {
            ChefZ_Record r = outRecords.Get(i);
            r.DistributeExplicitPaths();
            r.SetOrigin(sourceRef, rank);
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // Je Art ein Durchgang. Fuenfzehn Vierzeiler, weil JsonFileLoader<T> je
    // Dokumentart einen konkreten Typ braucht (Kopf dieser Datei).
    //--------------------------------------------------------------------------

    private static bool ReadCoreSettings(string text, out array<ref ChefZ_Record> outRecords, out string errorOut)
    {
        ChefZ_CoreSettingsDoc a = new ChefZ_CoreSettingsDoc();
        if (!JsonFileLoader<ChefZ_CoreSettingsDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_CoreSettingsDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_CategoryDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_TagDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_StateDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_QualityTierDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_ToolGroupDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_DeviceDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_ContainerDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_IngredientDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_NutritionDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_PreservationDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_ProcessDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_StationDef rec = a.records.Get(i);
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
        ChefZ_Log.Trace(ChefZ_LogChannel.CONFIG, "ReadTransform: Durchgang 1 beginnt");

        ChefZ_TransformDoc a = new ChefZ_TransformDoc();
        if (!JsonFileLoader<ChefZ_TransformDoc>.LoadData(text, a, errorOut))
            return false;
        if (!a.records)
            return true;

        ChefZ_Log.Trace(ChefZ_LogChannel.CONFIG, "ReadTransform: Durchgang 1 ok, " + a.records.Count().ToString() + " Records");

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_TransformDef rec = a.records.Get(i);
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

        for (int i = 0; i < a.records.Count(); i++)
        {
            ChefZ_RecipeDef rec = a.records.Get(i);
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

        // 4. Unterobjekte: der Laeufer traegt Pfade ein, der Record verteilt
        //    sie an seine Kinder - erst dann kann ein Slot eine geschriebene
        //    0 von einer fehlenden unterscheiden, und ein Ergebnis ein
        //    geschriebenes false von einem weggelassenen.
        array<ref ChefZ_Record> nested = new array<ref ChefZ_Record>();
        string err4;
        string rdoc = "{ \"kind\": \"recipe\", \"records\": [ { \"id\": \"CHEFZ_ST_REZ\", " + "\"slots\": [ { \"slotId\": \"a\", \"minCount\": 0, \"match\": { \"cls\": \"CHEFZ_ST_X\" } } ], " + "\"outputs\": [ { \"cls\": \"CHEFZ_ST_Y\", \"inheritState\": false } ] } ] }";
        if (!Read(ChefZ_RecordKind.RECIPE, rdoc, "Selbsttest", ChefZ_SourceRank.ADDON_JSON, nested, err4))
            return false;
        if (nested.Count() != 1)                                return false;
        ChefZ_RecipeDef rez = ChefZ_RecipeDef.Cast(nested.Get(0));
        if (!rez)                                               return false;
        if (!rez.HasExplicit("slots[0].minCount"))              return false;
        if (!rez.HasExplicit("outputs[0].inheritState"))        return false;
        if (rez.HasExplicit("outputs[0].inheritFreshness"))     return false;
        if (!rez.slots || rez.slots.Count() != 1)               return false;
        if (!rez.outputs || rez.outputs.Count() != 1)           return false;
        ChefZ_SlotDef   slotA = rez.slots.Get(0);
        ChefZ_OutputDef outY  = rez.outputs.Get(0);
        if (!slotA || !outY)                                    return false;
        if (!slotA.HasExplicit("minCount"))                     return false;
        if (!outY.HasExplicit("inheritState"))                  return false;
        if (outY.HasExplicit("inheritFreshness"))               return false;
        rez.ResolveDefaults();
        if (slotA.minCount != 0)                                return false;
        if (outY.inheritState)                                  return false;
        if (!outY.inheritFreshness)                             return false;

        return true;
    }
}
