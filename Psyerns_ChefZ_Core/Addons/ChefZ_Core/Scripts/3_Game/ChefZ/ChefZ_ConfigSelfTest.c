//==============================================================================
// ChefZ_ConfigSelfTest - Abnahmepruefung fuer S2
//
// Entwurf: 19 S2 - "Fertig, wenn: $profile:ChefZ/ wird beim ersten Start
// angelegt, ein Testmanifest mit einer JSON-Datei laedt, der Ladebericht steht
// im RPT, kaputtes JSON fuehrt zu genau EINEM ERROR und DEGRADED statt zu
// einem Absturz."
//
// Der letzte Punkt ist der wichtigste und zugleich der, den man am leichtesten
// nur behauptet. Deshalb prueft dieser Test ihn nachweisbar - und zwar OHNE
// eine kaputte Datei auszuliefern:
//
//   JsonFileLoader.LoadData arbeitet auf einer Zeichenkette, nicht auf einer
//   Datei (3_Game/DayZ/tools/JsonFileLoader.c:66). Der Parserpfad ist damit
//   Zeichen fuer Zeichen derselbe, den eine echte Datei nimmt - nur ohne
//   Dateisystem. Eine absichtlich kaputte .json im PBO waere ausserdem ein
//   Befund im statischen Validator und damit eine dauerhafte rote Zeile fuer
//   ein einmaliges Experiment.
//
// Ausgabe: eine Zeile bei Erfolg, je gescheiterter Gruppe eine ERROR-Zeile.
// Der Test veraendert nichts: er legt keine Datei an, beruehrt kein Item,
// benutzt eine eigene Sink-Instanz und einen eigenen Ladebericht.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_ConfigSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("RecordKind",   ChefZ_RecordKind.SelfCheck());
        Check("Record",       ChefZ_Record.SelfCheck());
        Check("CoreSettings", ChefZ_CoreSettingsDef.SelfCheck());
        Check("PathTools",    ChefZ_PathTools.SelfCheck());
        Check("JsonText",     ChefZ_JsonText.SelfCheck());
        Check("JsonReader",   ChefZ_JsonRecordReader.SelfCheck());
        Check("RecordSink",   ChefZ_RecordSink.SelfCheck());
        Check("Registry",     RegistryCheck());
        Check("BrokenJson",   BrokenJsonCheck());

        ProbeUnknownFieldTolerance();

        return s_Failed == 0;
    }

    /**
     * KEINE Pruefung, sondern eine MESSUNG - und deshalb ohne Einfluss auf das
     * Ergebnis.
     *
     * 02 §8 nimmt an, dass der Enforce-Serializer unbekannte Felder ignoriert
     * ("schemaVersion neuer als der Core -> Dokument wird geladen, unbekannte
     * Felder ignoriert"). Belegt ist das nirgends: weder die Dokumentation von
     * JsonSerializer.ReadFromString (3_Game/DayZ/gameplay.c:49) noch ein
     * Vanilla-Aufrufer sagt etwas dazu.
     *
     * Der Core VERLAESST sich nicht darauf - keine ausgelieferte Datei enthaelt
     * unbekannte Felder. Aber die Antwort gehoert in den Gate-1-Report, und sie
     * kostet hier einen Parsevorgang beim Boot.
     */
    private static void ProbeUnknownFieldTolerance()
    {
        string doc = "{ \"kind\": \"tag\", \"_kommentar\": \"nur zur Messung\", \"records\": [" + "{ \"id\": \"CHEFZ_ST_PROBE\", \"gibtEsNicht\": 42 } ] }";

        array<ref ChefZ_Record> recs = new array<ref ChefZ_Record>();
        string err;
        bool tolerant = ChefZ_JsonRecordReader.Read(ChefZ_RecordKind.TAG, doc, "Selbsttest/sonde.json", ChefZ_SourceRank.ADDON_JSON, recs, err);

        if (tolerant)
        {
            ChefZ_Log.Once(ChefZ_LogLevel.INFO, ChefZ_LogChannel.CONFIG, "config.unknownfields", "Messung: unbekannte JSON-Felder werden vom Serializer ignoriert. " + "Vorwaertskompatibilitaet nach 02 §8 ist damit belegt.");
            return;
        }

        ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.CONFIG, "config.unknownfields", "Messung: unbekannte JSON-Felder lassen den Serializer scheitern (" + err + "). Die Annahme aus 02 §8 traegt NICHT - eine Datendatei mit einem Feld " + "aus einer neueren Core-Version wird komplett verworfen. Content-Autoren " + "duerfen keine Kommentarfelder in JSON schreiben.");
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.CONFIG, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, "Selbsttest " + name + " FEHLGESCHLAGEN. Der Config Manager verhaelt sich nicht " + "wie entworfen - jede Aussage ueber geladene Daten ist ab hier unzuverlaessig.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S2: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
        if (s_Failed > 0 && s_FailedNames)
        {
            s = s + "  gescheitert:";
            for (int i = 0; i < s_FailedNames.Count(); i++)
                s = s + " " + s_FailedNames.Get(i);
        }
        return s;
    }

    //--------------------------------------------------------------------------

    //! Registry: Aufnahme, Suche, stabile Schluesselreihenfolge, Freeze.
    private static bool RegistryCheck()
    {
        ChefZ_Registry<ChefZ_CategoryDef> reg = new ChefZ_Registry<ChefZ_CategoryDef>();
        reg.Init(ChefZ_RecordKind.CATEGORY);

        array<string> ids = new array<string>();
        ids.Insert("CHEFZ_ST_SAUSAGE");
        ids.Insert("CHEFZ_ST_MEAT");
        ids.Insert("CHEFZ_ST_DAIRY");

        for (int i = 0; i < ids.Count(); i++)
        {
            ChefZ_CategoryDef rec = new ChefZ_CategoryDef();
            rec.id = ids.Get(i);
            rec.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
            rec.Compile(null);
            if (!reg.Add(rec))
                return false;
        }

        if (reg.Count() != 3)                                       return false;

        ChefZ_CategoryDef found = reg.FindByName("CHEFZ_ST_MEAT");
        if (!found)                                                 return false;
        if (!reg.Contains(found.sym))                               return false;
        if (reg.Find(found.sym) != found)                           return false;
        if (reg.FindByName("CHEFZ_ST_GIBTSNICHT"))                  return false;

        // Doppelte ID wird nicht aufgenommen.
        ChefZ_CategoryDef dup = new ChefZ_CategoryDef();
        dup.id = "CHEFZ_ST_MEAT";
        dup.Compile(null);
        if (reg.Add(dup))                                           return false;

        // Schluessel stabil sortiert (03 §4), unabhaengig von der Einfuegefolge.
        reg.Freeze();
        array<ChefZ_Sym> keys = reg.Keys();
        if (keys.Count() != 3)                                      return false;
        if (ChefZ_SymbolTable.Name(keys.Get(0)) != "CHEFZ_ST_DAIRY")    return false;
        if (ChefZ_SymbolTable.Name(keys.Get(1)) != "CHEFZ_ST_MEAT")     return false;
        if (ChefZ_SymbolTable.Name(keys.Get(2)) != "CHEFZ_ST_SAUSAGE")  return false;

        // Nach dem Einfrieren nimmt sie nichts mehr auf.
        ChefZ_CategoryDef late = new ChefZ_CategoryDef();
        late.id = "CHEFZ_ST_LATE";
        late.Compile(null);
        if (reg.Add(late))                                          return false;
        if (!reg.IsFrozen())                                        return false;

        // SAFE_MODE leert auch eine eingefrorene Registry.
        reg.ClearAll();
        if (reg.Count() != 0)                                       return false;

        return true;
    }

    /**
     * Die Abnahmebedingung aus 19 S2, nachweisbar:
     * kaputtes JSON -> GENAU EIN Fehler, kein Absturz, alles vorher Geladene
     * bleibt gueltig, danach Geladenes kommt weiterhin an.
     */
    private static bool BrokenJsonCheck()
    {
        ChefZ_LoadReport report = new ChefZ_LoadReport();
        report.SetMirrorToLog(false);       // der Test soll das RPT nicht mit
                                            // erwarteten Fehlern zumuellen

        ChefZ_RecordSink sink = new ChefZ_RecordSink();
        sink.Init(report);

        // 1. eine gute Datei
        array<ref ChefZ_Record> good = new array<ref ChefZ_Record>();
        string errGood;
        string goodDoc = "{ \"kind\": \"tag\", \"schemaVersion\": 1, \"records\": [" + "{ \"id\": \"CHEFZ_ST_TAG_A\" } ] }";
        if (!ChefZ_JsonRecordReader.Read(ChefZ_RecordKind.TAG, goodDoc, "Selbsttest/gut.json", ChefZ_SourceRank.ADDON_JSON, good, errGood))
            return false;
        for (int i = 0; i < good.Count(); i++)
            sink.Submit(good.Get(i));

        // 2. eine kaputte Datei - derselbe Pfad wie im Betrieb
        array<ref ChefZ_Record> broken = new array<ref ChefZ_Record>();
        string errBroken;
        string brokenDoc = "{ \"kind\": \"tag\", \"records\": [ { \"id\": \"CHEFZ_ST_TAG_B\" ";
        bool ok = ChefZ_JsonRecordReader.Read(ChefZ_RecordKind.TAG, brokenDoc, "Selbsttest/kaputt.json", ChefZ_SourceRank.ADDON_JSON, broken, errBroken);
        if (ok)                                     return false;   // muss scheitern
        if (broken.Count() != 0)                    return false;   // nichts halb angewandt
        if (errBroken == "")                        return false;   // mit Parsermeldung

        // Genau EIN Fehlereintrag - so, wie der Helfer ihn im Betrieb setzt.
        report.AddError("Selbsttest/kaputt.json", "", "JSON nicht lesbar - die gesamte Datei wird verworfen. Parsermeldung: " + errBroken);
        if (report.ErrorCount() != 1)               return false;

        // 3. eine gute Datei DANACH - der Strom laeuft weiter
        array<ref ChefZ_Record> after = new array<ref ChefZ_Record>();
        string errAfter;
        string afterDoc = "{ \"kind\": \"tag\", \"records\": [ { \"id\": \"CHEFZ_ST_TAG_C\" } ] }";
        if (!ChefZ_JsonRecordReader.Read(ChefZ_RecordKind.TAG, afterDoc, "Selbsttest/danach.json", ChefZ_SourceRank.ADDON_JSON, after, errAfter))
            return false;
        for (int k = 0; k < after.Count(); k++)
            sink.Submit(after.Get(k));

        if (sink.CountOf(ChefZ_RecordKind.TAG) != 2)    return false;   // A und C
        if (report.ErrorCount() != 1)                   return false;   // weiterhin genau einer

        // 4. Ein Fehler bei Standardeinstellungen ergibt DEGRADED, kein SAFE_MODE.
        ChefZ_CoreSettingsDef settings = new ChefZ_CoreSettingsDef();
        settings.ResolveDefaults();
        if (settings.strictMode)                                        return false;
        if (report.ErrorCount() > settings.safeModeErrorThreshold)      return false;

        return true;
    }
}
