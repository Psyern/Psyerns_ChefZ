//==============================================================================
// ChefZ_RecordSink - ein Strom, ein Merge
//
// Entwurf: 02 §5.2 (Signatur), 02 §6 (NORMALIZE -> MERGE), 02 §8 (Fehlertabelle),
// 02 E3 (feldweiser Patch), 03 §4/§7 (Sync-Auflage fuer Rang 3).
//
// Alle Quellen schuetten in denselben Sink, in Rangreihenfolge und innerhalb
// eines Rangs in loadOrder-Reihenfolge. Der Sink entscheidet je (Art, ID):
//
//   erstes Vorkommen                -> aufnehmen
//   gleicher Rang, gleiche ID       -> ERSTE gewinnt, zweite abgewiesen, ERROR
//   hoeherer Rang, gleiche ID       -> feldweiser Patch, DEBUG
//   niedrigerer Rang, gleiche ID    -> ignoriert, WARN (Quellen kamen unsortiert)
//
// "Erste gewinnt" statt "letzte gewinnt" ist bewusst (02 §8): der Merge muss
// reproduzierbar sein, und die Quellreihenfolge innerhalb eines Rangs ist
// deterministisch sortiert. Waere die letzte massgeblich, entschiede die
// Ladereihenfolge zweier Addons ueber das Spielverhalten.
//
// Der Sink haelt die Records, bis der Config Manager sie abholt. Er baut
// KEINE Registry - Validierung, Identitaeten und COMPILE laufen erst danach,
// und zwar in der Ladeordnung aus 02 §6, nicht in Ankunftsreihenfolge.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_RecordSink : Managed
{
    private ChefZ_LoadReport m_Report;      // bewusst ohne ref: Eigentuemer ist
                                            // der Config Manager
    private ref map<string, ref array<ref ChefZ_Record>> m_ByKind;
    private ref map<string, int>                         m_Index;   // "kind|id" -> Position

    private int m_Accepted;
    private int m_Rejected;
    private int m_Patched;
    private int m_Submitted;

    //! Deckel gegen eine fehlerhafte Massendatei, die den Speicher frisst.
    //! 50000 Records sind mehr als das Vielfache dessen, was V1 vorsieht.
    static const int MAX_RECORDS = 50000;

    void ChefZ_RecordSink()
    {
        m_ByKind    = new map<string, ref array<ref ChefZ_Record>>();
        m_Index     = new map<string, int>();
        m_Accepted  = 0;
        m_Rejected  = 0;
        m_Patched   = 0;
        m_Submitted = 0;
    }

    void Init(ChefZ_LoadReport report)
    {
        m_Report = report;
    }

    //--------------------------------------------------------------------------

    /**
     * Einziger Eingang. Der Record MUSS seine Herkunft bereits kennen
     * (SetOrigin) - der Sink setzt sie nicht, weil er sonst raten muesste.
     */
    void Submit(ChefZ_Record rec)
    {
        m_Submitted++;

        if (!rec)
        {
            m_Rejected++;
            Error("", "", "Leerer Record eingereicht - uebersprungen.");
            return;
        }

        string kind = rec.GetKindName();
        if (kind == "" || !ChefZ_RecordKind.IsKnown(kind))
        {
            m_Rejected++;
            Error(rec.sourceRef, rec.id, "Unbekannte Record-Art \"" + kind + "\" - abgewiesen. Gueltig: " + ChefZ_RecordKind.ValidNames());
            return;
        }

        rec.Normalize();

        if (rec.id == "")
        {
            m_Rejected++;
            Error(rec.sourceRef, "", "Record der Art \"" + kind + "\" ohne \"id\" - abgewiesen.");
            return;
        }

        if (m_Accepted >= MAX_RECORDS)
        {
            m_Rejected++;
            ErrorOnce("sink.full", "Mehr als " + MAX_RECORDS.ToString() + " Records - weitere werden verworfen. " + "Das ist fast immer eine fehlerhaft erzeugte Datendatei.");
            return;
        }

        string key = kind + "|" + rec.id;
        int existingIndex;
        if (!m_Index.Find(key, existingIndex))
        {
            Store(kind, key, rec);
            return;
        }

        array<ref ChefZ_Record> bucket = GetBucket(kind);
        ChefZ_Record existing = bucket.Get(existingIndex);

        if (rec.sourceRank == existing.sourceRank)
        {
            m_Rejected++;
            Error(rec.sourceRef, rec.id, "Doppelte ID im selben Rang " + rec.sourceRank.ToString() + " - die erste gewinnt. Bereits geladen aus: " + existing.sourceRef);
            return;
        }

        if (rec.sourceRank < existing.sourceRank)
        {
            m_Rejected++;
            Warn(rec.sourceRef, rec.id, "Record aus Rang " + rec.sourceRank.ToString() + " trifft auf einen bereits " + "geladenen aus Rang " + existing.sourceRank.ToString() + " - ignoriert. Quellen wurden nicht in Rangreihenfolge gelesen.");
            return;
        }

        // Hoeherer Rang: das ist der Override-Mechanismus, kein Fehler (02 §8).
        if (rec.sourceRank >= ChefZ_SourceRank.PROFILE_OVERLAY && ChefZ_RecordKind.IsSyncRelevant(kind))
        {
            // Feld-Patches auf sync-relevanten Arten sind erlaubt, solange sie
            // den Ordinal nicht bewegen - und das tut ein Patch nie, weil er
            // die ID nicht aendert und keinen Record hinzufuegt (03 §4).
            DebugLine("Overlay patcht sync-relevanten Record " + kind + " \"" + rec.id + "\" - erlaubt, solange kein Record hinzukommt.");
        }

        existing.PatchFrom(rec);
        m_Patched++;
        string chefzTxt1 = "Rang " + rec.sourceRank.ToString() + " patcht " + kind + " \"" + rec.id + "\" (";
        chefzTxt1 = chefzTxt1 + rec.sourceRef + ")";
        DebugLine(chefzTxt1);
    }

    //--------------------------------------------------------------------------

    /**
     * Rang 3 darf eine sync-relevante Registry nicht ERWEITERN (03 §4, §7).
     *
     * Getrennt von Submit(), weil hier der Bestand aus Rang 1 bekannt sein
     * muss: "neu" heisst "in Rang 1 nicht vorhanden", nicht "noch nicht
     * gesehen". Der Config Manager ruft das, bevor er Rang 3 einliest.
     */
    bool IsForbiddenOverlayAddition(string kind, string id)
    {
        if (!ChefZ_RecordKind.IsSyncRelevant(kind))
            return false;
        return !m_Index.Contains(kind + "|" + id);
    }

    void RejectOverlayAddition(string kind, string id, string sourceRef)
    {
        m_Rejected++;
        Error(sourceRef, id, "Overlay (Rang 3) darf die sync-relevante Art \"" + kind + "\" nicht erweitern. " + "Der Sync-Ordinal wird auf Client und Server unabhaengig aus Rang 1 abgeleitet - " + "ein zusaetzlicher Record broeche die Symmetrie (03 §4). Feld-Patches bleiben erlaubt.");
    }

    //--------------------------------------------------------------------------

    private void Store(string kind, string key, ChefZ_Record rec)
    {
        array<ref ChefZ_Record> bucket = GetBucket(kind);
        m_Index.Set(key, bucket.Count());
        bucket.Insert(rec);
        m_Accepted++;
    }

    /**
     * Eimer einer Art, bei Bedarf angelegt.
     *
     * Contains + Get statt Find: bei einer Map mit ref-Werten ist Get der in
     * Vanilla belegte Zugriff (3_Game/DayZ/DayZAnimEventMaps.c:54 arbeitet
     * genau so und prueft danach auf NULL). Contains davor, damit ein
     * unbekannter Schluessel keine Engine-Meldung erzeugt.
     */
    private array<ref ChefZ_Record> GetBucket(string kind)
    {
        if (m_ByKind.Contains(kind))
            return m_ByKind.Get(kind);

        array<ref ChefZ_Record> bucket = new array<ref ChefZ_Record>();
        m_ByKind.Set(kind, bucket);
        return bucket;
    }

    //--------------------------------------------------------------------------

    //! Gemergte Records einer Art, in Aufnahmereihenfolge. Leeres Array statt
    //! null - der Aufrufer soll nicht jedes Mal pruefen muessen.
    array<ref ChefZ_Record> GetRecords(string kind)
    {
        return GetBucket(kind);
    }

    int CountOf(string kind)
    {
        return GetBucket(kind).Count();
    }

    int GetAcceptedCount()  { return m_Accepted; }
    int GetRejectedCount()  { return m_Rejected; }
    int GetPatchedCount()   { return m_Patched; }
    int GetSubmittedCount() { return m_Submitted; }

    void Clear()
    {
        m_ByKind.Clear();
        m_Index.Clear();
        m_Accepted  = 0;
        m_Rejected  = 0;
        m_Patched   = 0;
        m_Submitted = 0;
    }

    //--------------------------------------------------------------------------

    private void Error(string src, string id, string msg)
    {
        if (m_Report)
            m_Report.AddError(src, id, msg);
    }

    private void Warn(string src, string id, string msg)
    {
        if (m_Report)
            m_Report.AddWarn(src, id, msg);
    }

    private void ErrorOnce(string key, string msg)
    {
        if (ChefZ_Log.OnceFired(key))
            return;
        if (m_Report)
            m_Report.AddError("", "", msg);
        ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.CONFIG, key, msg);
    }

    private void DebugLine(string msg)
    {
        // Enabled-Wache nach 18 E2: der zusammengesetzte String entsteht beim
        // Aufrufer, deshalb ist die Wache dort ebenfalls noetig, wenn sie teuer
        // wird. Hier ist sie billig genug.
        if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
            ChefZ_Log.Debug(ChefZ_LogChannel.CONFIG, msg);
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest (ChefZ_ConfigSelfTest).
    static bool SelfCheck()
    {
        ChefZ_LoadReport rep = new ChefZ_LoadReport();
        rep.SetMirrorToLog(false);

        ChefZ_RecordSink sink = new ChefZ_RecordSink();
        sink.Init(rep);

        // 1. Rang 2 legt an
        ChefZ_CategoryDef a = new ChefZ_CategoryDef();
        a.id = "  CHEFZ_ST_CAT  ";
        a.displayName = "Erstname";
        a.SetOrigin("Test/Rang2.json", ChefZ_SourceRank.ADDON_JSON);
        sink.Submit(a);
        if (sink.GetAcceptedCount() != 1)                       return false;
        if (a.id != "CHEFZ_ST_CAT")                             return false;   // NORMALIZE lief

        // 2. gleicher Rang, gleiche ID -> erste gewinnt
        ChefZ_CategoryDef dup = new ChefZ_CategoryDef();
        dup.id = "CHEFZ_ST_CAT";
        dup.displayName = "Zweitname";
        dup.SetOrigin("Test/Rang2b.json", ChefZ_SourceRank.ADDON_JSON);
        sink.Submit(dup);
        if (sink.GetRejectedCount() != 1)                       return false;
        if (rep.ErrorCount() != 1)                              return false;
        if (a.displayName != "Erstname")                        return false;

        // 3. Rang 3 patcht feldweise: parent gesetzt, displayName nicht
        ChefZ_CategoryDef ov = new ChefZ_CategoryDef();
        ov.id = "CHEFZ_ST_CAT";
        ov.parent = "CHEFZ_ST_PARENT";
        ov.SetOrigin("$profile:ChefZ\\Test.json", ChefZ_SourceRank.PROFILE_OVERLAY);
        sink.Submit(ov);
        if (sink.GetPatchedCount() != 1)                        return false;
        if (a.parent != "CHEFZ_ST_PARENT")                      return false;
        if (a.displayName != "Erstname")                        return false;   // unberuehrt

        // 4. Record ohne ID
        ChefZ_CategoryDef noId = new ChefZ_CategoryDef();
        noId.SetOrigin("Test/Rang2.json", ChefZ_SourceRank.ADDON_JSON);
        sink.Submit(noId);
        if (sink.GetRejectedCount() != 2)                       return false;

        // 5. Sync-Auflage: Overlay darf Zustaende nicht erweitern
        if (!sink.IsForbiddenOverlayAddition(ChefZ_RecordKind.STATE, "CHEFZ_ST_NEU"))  return false;
        if (sink.IsForbiddenOverlayAddition(ChefZ_RecordKind.RECIPE, "CHEFZ_ST_NEU"))  return false;

        if (sink.CountOf(ChefZ_RecordKind.CATEGORY) != 1)       return false;
        if (sink.CountOf(ChefZ_RecordKind.RECIPE) != 0)         return false;

        return true;
    }
}
