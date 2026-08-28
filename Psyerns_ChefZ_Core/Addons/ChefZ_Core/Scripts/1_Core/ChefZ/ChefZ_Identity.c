//==============================================================================
// ChefZ_Identity / ChefZ_IdentityMap - Laufzeit, Persistenz und Netz-Sync
//
// Entwurf: 03 §2, §3.2, §4, §7, E2, E3, E5.
//
// Der zentrale Fehler waere, fuer alle drei Zwecke dieselbe Zahl zu nehmen:
//
//   Laufzeit    ChefZ_Sym      interniert, fortlaufend, schnell - darf sich
//                              zwischen Serverstarts aendern
//   Persistenz  id.Hash()      stabil ueber Content-Updates und
//                              Ladereihenfolgen, kein geteilter Zahlenraum
//   Netz-Sync   syncOrdinal    klein (0..N), auf Client und Server IDENTISCH
//                              abgeleitet und deshalb nie uebertragen
//
// Der Ordinal wird NICHT gepflegt, sondern beim Boot abgeleitet (03 §4):
//   1. alle IDs der Registry sammeln - nur aus Rang 1, den Client und Server
//      identisch lesen
//   2. ordinal aufsteigend sortieren (ChefZ_StringOrder, keine Locale)
//   3. Ordinal = Position, beginnend bei 1; 0 bleibt "unbekannt"
//
// Daraus folgt die Symmetrie von selbst: beide Seiten lesen dieselben PBOs,
// sortieren gleich, kommen auf denselben Ordinal. Und weil nie ein Ordinal
// gespeichert wird, ist es harmlos, wenn ein neuer Zustand alle nachfolgenden
// Ordinale verschiebt (03 E3).
//
// Hash-Kollision ist ein Fehler, kein Sonderfall (03 E5): "erste gewinnt"
// waere Kulanz, die einen Datenfehler in die Zukunft verschiebt, wo er als
// "mein Fleisch ist ploetzlich geraeuchert" auftaucht. BEIDE Records werden
// abgewiesen und gemeldet.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_Identity
{
    ChefZ_Sym sym;              // Laufzeit
    int       persistHash;      // = id.Hash()        -> OnStoreSave
    int       syncOrdinal;      // 1..N, abgeleitet   -> RegisterNetSyncVariableInt
    string    id;               // Klartext, fuer Log und Trace

    void Init(string identifier, int ordinal)
    {
        id          = identifier;
        sym         = ChefZ_SymbolTable.Intern(identifier);
        persistHash = identifier.Hash();
        syncOrdinal = ordinal;
    }

    string ToLine()
    {
        return syncOrdinal.ToString() + "  " + id + "  sym=" + sym.ToString() + "  hash=" + persistHash.ToString();
    }
}

/**
 * Obergrenzen der Netzsync-Variablen (03 §4).
 *
 * Bewusst Konstanten und nicht JSON: sie sind keine Einstellung, sondern die
 * Bitbreite, mit der die Variable auf dem Item registriert wird. Wer sie in
 * einer Datei aendern koennte, koennte die Registrierung und die Daten
 * auseinanderlaufen lassen - und das faellt erst beim Spieler auf.
 */
class ChefZ_SyncLimits
{
    static const int STATE_ORDINAL_MAX   = 63;
    static const int QUALITY_ORDINAL_MAX = 15;
    static const int PORTIONS_MAX        = 31;

    //! Registries ohne Sync-Relevanz (Rezepte, Prozesse, Kategorien, Tags)
    //! bekommen keinen Ordinal-Deckel.
    static const int NO_LIMIT            = 0;
}

class ChefZ_IdentityMap
{
    private string m_RegistryName;
    private bool   m_Built;
    private bool   m_UseBeforeBuildReported;

    private int    m_SyncLimit;

    private ref array<ref ChefZ_Identity> m_ByOrdinal;   // Index 0 bleibt null
    private ref map<int, int>             m_HashToOrdinal;
    private ref map<int, int>             m_SymToOrdinal;

    private bool   m_HadCollision;
    private string m_CollisionA;
    private string m_CollisionB;

    private ref array<string> m_Rejected;

    void ChefZ_IdentityMap()
    {
        m_RegistryName           = "";
        m_Built                  = false;
        m_UseBeforeBuildReported = false;
        m_SyncLimit              = ChefZ_SyncLimits.NO_LIMIT;
        m_HadCollision           = false;
        m_CollisionA             = "";
        m_CollisionB             = "";
        Allocate();
    }

    private void Allocate()
    {
        m_ByOrdinal     = new array<ref ChefZ_Identity>();
        m_ByOrdinal.Insert(null);           // Ordinal 0 = "unbekannt"
        m_HashToOrdinal = new map<int, int>();
        m_SymToOrdinal  = new map<int, int>();
        m_Rejected      = new array<string>();
    }

    //! Name der Registry. Erscheint in jeder Meldung dieser Map.
    void SetRegistryName(string name)
    {
        m_RegistryName = name;
    }

    string GetRegistryName()
    {
        return m_RegistryName;
    }

    //--------------------------------------------------------------------------
    // Aufbau
    //--------------------------------------------------------------------------

    /**
     * Baut die Map. Einmal beim Boot, danach unveraenderlich (03 §6).
     *
     * @param sortedIds   IDs der Registry. Der Aufrufer soll bereits ordinal
     *                    aufsteigend sortiert uebergeben; ist er es nicht,
     *                    sortiert Build defensiv nach und warnt. Ohne diese
     *                    Nachsortierung koennte eine unsortierte Eingabe die
     *                    Client/Server-Symmetrie brechen, und das waere ein
     *                    Fehler, den niemand im Log sieht, sondern nur der
     *                    Spieler an einer falschen Anzeige.
     * @param report      optional. Ist er null, gehen Meldungen nur ins Log.
     * @param syncLimit   Obergrenze fuer den Ordinal (ChefZ_SyncLimits).
     *                    0 = keine Grenze, fuer nicht sync-relevante Registries.
     *
     * Abweichung von der Signatur in 03 §3.2: der Bericht ist ein normaler
     * (nullbarer) Parameter statt "out". "out" auf einem ref-Typ hiesse, dass
     * Build den Bericht des Aufrufers ERSETZEN darf - das ist nicht gemeint,
     * und es machte den haeufigen Fall "kein Bericht" unaufrufbar.
     */
    void Build(notnull array<string> sortedIds, ChefZ_LoadReport report = null, int syncLimit = 0)
    {
        Allocate();
        m_Built                  = false;
        m_HadCollision           = false;
        m_CollisionA             = "";
        m_CollisionB             = "";
        m_UseBeforeBuildReported = false;
        m_SyncLimit              = syncLimit;

        string src = SourceRef();

        // --- 1. Arbeitskopie, leere IDs raus -------------------------------
        array<string> ids = new array<string>();
        for (int i = 0; i < sortedIds.Count(); i++)
        {
            string raw = sortedIds.Get(i);
            if (raw == "")
            {
                Report(report, true, src, "", "Leere ID in der Registry - abgewiesen.");
                continue;
            }
            ids.Insert(raw);
        }

        // --- 2. Reihenfolge sicherstellen ----------------------------------
        if (!ChefZ_StringOrder.IsAscending(ids))
        {
            Report(report, false, src, "", "IDs waren nicht ordinal aufsteigend sortiert. Build sortiert nach. " + "Der Sync-Ordinal bleibt dadurch symmetrisch, aber der Aufrufer " + "sollte bereits sortiert uebergeben (03 §4).");
            ChefZ_StringOrder.SortAscending(ids);
        }

        // --- 3. Duplikate ---------------------------------------------------
        array<string> unique = new array<string>();
        for (int d = 0; d < ids.Count(); d++)
        {
            string cur = ids.Get(d);
            if (d > 0 && cur == ids.Get(d - 1))
            {
                Report(report, true, src, cur, "ID doppelt in derselben Registry - das zweite Vorkommen wird abgewiesen.");
                m_Rejected.Insert(cur);
                continue;
            }
            unique.Insert(cur);
        }

        // --- 4. Hash-Kollisionen (03 E5): BEIDE abweisen ---------------------
        map<int, string> firstByHash = new map<int, string>();
        set<string>      collided    = new set<string>();

        for (int h = 0; h < unique.Count(); h++)
        {
            string id = unique.Get(h);
            int hash  = id.Hash();
            string other;
            if (firstByHash.Find(hash, other))
            {
                m_HadCollision = true;
                if (m_CollisionA == "")
                {
                    m_CollisionA = other;
                    m_CollisionB = id;
                }
                collided.Insert(other);
                collided.Insert(id);
                Report(report, true, src, id, "Hash-Kollision mit \"" + other + "\" (beide Hash " + hash.ToString() + "). BEIDE Records werden abgewiesen - eine Kollision macht die " + "Persistenz mehrdeutig. Abhilfe: eine der beiden IDs umbenennen.");
                continue;
            }
            firstByHash.Set(hash, id);
        }

        array<string> accepted = new array<string>();
        for (int k = 0; k < unique.Count(); k++)
        {
            string cand = unique.Get(k);
            if (collided.Find(cand) >= 0)
            {
                m_Rejected.Insert(cand);
                continue;
            }
            accepted.Insert(cand);
        }

        // --- 5. Sync-Obergrenze (03 §4) --------------------------------------
        if (m_SyncLimit > 0 && accepted.Count() > m_SyncLimit)
        {
            for (int over = m_SyncLimit; over < accepted.Count(); over++)
            {
                string dropped = accepted.Get(over);
                Report(report, true, src, dropped, "Registry ueberschreitet ihre Sync-Obergrenze von " + m_SyncLimit.ToString() + " Eintraegen. Dieser Record wird abgewiesen. " + "Eine harte Grenze ist besser als eine stille Sync-Verstuemmelung.");
                m_Rejected.Insert(dropped);
            }
            accepted.Resize(m_SyncLimit);
        }

        // --- 6. Ordinale vergeben, beginnend bei 1 ---------------------------
        for (int n = 0; n < accepted.Count(); n++)
        {
            ChefZ_Identity ident = new ChefZ_Identity();
            ident.Init(accepted.Get(n), n + 1);

            m_ByOrdinal.Insert(ident);
            m_HashToOrdinal.Set(ident.persistHash, ident.syncOrdinal);
            m_SymToOrdinal.Set(ident.sym, ident.syncOrdinal);
        }

        m_Built = true;

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.INFO))
            ChefZ_Log.Info(ChefZ_LogChannel.CONFIG, src + ": " + GetMaxOrdinal().ToString() + " Identitaeten, " + m_Rejected.Count().ToString() + " abgewiesen");
    }

    private string SourceRef()
    {
        if (m_RegistryName == "")
            return "IdentityMap";
        return "IdentityMap[" + m_RegistryName + "]";
    }

    private void Report(ChefZ_LoadReport report, bool isError, string src, string recordId, string msg)
    {
        if (report)
        {
            if (isError)
                report.AddError(src, recordId, msg);
            else
                report.AddWarn(src, recordId, msg);
            return;
        }

        string line = src;
        if (recordId != "")
            line = line + " / " + recordId;
        line = line + ": " + msg;

        if (isError)
            ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, line);
        else
            ChefZ_Log.Warn(ChefZ_LogChannel.CONFIG, line);
    }

    //--------------------------------------------------------------------------
    // Abfrage
    //--------------------------------------------------------------------------

    bool IsBuilt()
    {
        return m_Built;
    }

    //! 03 §7: Abfrage vor Build() liefert INVALID bzw. 0 und meldet EINMAL.
    //! Kein Nullzugriff, kein Absturz.
    private bool GuardBuilt()
    {
        if (m_Built)
            return true;
        if (!m_UseBeforeBuildReported)
        {
            m_UseBeforeBuildReported = true;
            ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, SourceRef() + ": Abfrage vor Build(). Liefert INVALID. " + "Das ist ein Reihenfolgefehler im Boot, kein Datenfehler.");
        }
        return false;
    }

    ChefZ_Sym FromPersistHash(int hash)
    {
        if (!GuardBuilt())
            return ChefZ_SymbolTable.INVALID;
        int ordinal;
        if (!m_HashToOrdinal.Find(hash, ordinal))
            return ChefZ_SymbolTable.INVALID;
        return m_ByOrdinal.Get(ordinal).sym;
    }

    int ToPersistHash(ChefZ_Sym sym)
    {
        if (!GuardBuilt())
            return 0;
        int ordinal;
        if (!m_SymToOrdinal.Find(sym, ordinal))
            return 0;
        return m_ByOrdinal.Get(ordinal).persistHash;
    }

    ChefZ_Sym FromSyncOrdinal(int ordinal)
    {
        if (!GuardBuilt())
            return ChefZ_SymbolTable.INVALID;
        if (ordinal <= 0 || ordinal >= m_ByOrdinal.Count())
            return ChefZ_SymbolTable.INVALID;
        return m_ByOrdinal.Get(ordinal).sym;
    }

    int ToSyncOrdinal(ChefZ_Sym sym)
    {
        if (!GuardBuilt())
            return 0;
        int ordinal;
        if (!m_SymToOrdinal.Find(sym, ordinal))
            return 0;
        return ordinal;
    }

    //! Obergrenze fuer die Sync-Registrierung: hoechster vergebener Ordinal.
    int GetMaxOrdinal()
    {
        if (!m_ByOrdinal)
            return 0;
        return m_ByOrdinal.Count() - 1;
    }

    //! Vollstaendiger Datensatz zum Ordinal, oder null.
    ChefZ_Identity GetByOrdinal(int ordinal)
    {
        if (!GuardBuilt())
            return null;
        if (ordinal <= 0 || ordinal >= m_ByOrdinal.Count())
            return null;
        return m_ByOrdinal.Get(ordinal);
    }

    ChefZ_Identity GetById(string id)
    {
        if (!GuardBuilt())
            return null;
        return GetByOrdinal(ToSyncOrdinal(ChefZ_SymbolTable.Lookup(id)));
    }

    string NameOfOrdinal(int ordinal)
    {
        ChefZ_Identity ident = GetByOrdinal(ordinal);
        if (!ident)
            return "";
        return ident.id;
    }

    bool HasHashCollision(out string a, out string b)
    {
        a = m_CollisionA;
        b = m_CollisionB;
        return m_HadCollision;
    }

    //! IDs, die Build abgewiesen hat - fuer den Ladebericht und "chefz registries".
    array<string> GetRejected()
    {
        return m_Rejected;
    }

    void DebugDump(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert(SourceRef() + "  gebaut=" + m_Built.ToString() + "  Ordinale=" + GetMaxOrdinal().ToString() + "  Deckel=" + m_SyncLimit.ToString());

        for (int i = 1; i < m_ByOrdinal.Count(); i++)
        {
            ChefZ_Identity ident = m_ByOrdinal.Get(i);
            if (ident)
                outLines.Insert("  " + ident.ToLine());
        }

        for (int r = 0; r < m_Rejected.Count(); r++)
            outLines.Insert("  abgewiesen: " + m_Rejected.Get(r));
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest (S1).
    static bool SelfCheck()
    {
        ChefZ_LoadReport rep = new ChefZ_LoadReport();
        rep.SetMirrorToLog(false);

        ChefZ_IdentityMap m = new ChefZ_IdentityMap();
        m.SetRegistryName("SELFTEST_STATES");

        // Abfrage vor Build darf nicht abstuerzen.
        if (m.IsBuilt())                                        return false;
        if (m.FromSyncOrdinal(1) != ChefZ_SymbolTable.INVALID)  return false;
        if (m.GetByOrdinal(1))                                  return false;

        // Bewusst unsortiert uebergeben: Build muss nachsortieren.
        array<string> ids = new array<string>();
        ids.Insert("CHEFZ_ST_SMOKED");
        ids.Insert("CHEFZ_ST_DRIED");
        ids.Insert("CHEFZ_ST_NONE");
        ids.Insert("CHEFZ_ST_DRIED");     // Duplikat
        ids.Insert("");                   // leere ID

        m.Build(ids, rep, ChefZ_SyncLimits.STATE_ORDINAL_MAX);

        if (!m.IsBuilt())                       return false;
        if (m.GetMaxOrdinal() != 3)             return false;
        if (rep.ErrorCount() != 2)              return false;   // Duplikat + leere ID

        // Ordinale folgen der ordinalen Sortierung, beginnend bei 1.
        if (m.NameOfOrdinal(1) != "CHEFZ_ST_DRIED")  return false;
        if (m.NameOfOrdinal(2) != "CHEFZ_ST_NONE")   return false;
        if (m.NameOfOrdinal(3) != "CHEFZ_ST_SMOKED") return false;
        if (m.NameOfOrdinal(0) != "")                return false;
        if (m.NameOfOrdinal(99) != "")               return false;

        // Persistenz ist der Hash und nichts anderes.
        ChefZ_Identity zustandC = m.GetByOrdinal(3);
        if (!zustandC)                                          return false;
        if (zustandC.persistHash != "CHEFZ_ST_SMOKED".Hash())   return false;
        if (m.ToPersistHash(zustandC.sym) != zustandC.persistHash) return false;
        if (m.FromPersistHash(zustandC.persistHash) != zustandC.sym) return false;
        if (m.FromPersistHash(12345678) != ChefZ_SymbolTable.INVALID) return false;
        if (m.ToSyncOrdinal(zustandC.sym) != 3)                 return false;
        if (m.ToSyncOrdinal(ChefZ_SymbolTable.INVALID) != 0)  return false;

        string ca;
        string cb;
        if (m.HasHashCollision(ca, cb))         return false;

        // Zweiter, unabhaengiger Aufbau derselben IDs in anderer Reihenfolge
        // muss dieselben Ordinale ergeben - das ist die Client/Server-Symmetrie
        // aus 03 E3, und sie ist der einzige Grund, warum der Ordinal nicht
        // synchronisiert werden muss.
        ChefZ_LoadReport rep2 = new ChefZ_LoadReport();
        rep2.SetMirrorToLog(false);
        ChefZ_IdentityMap m2 = new ChefZ_IdentityMap();
        m2.SetRegistryName("SELFTEST_STATES_2");
        array<string> ids2 = new array<string>();
        ids2.Insert("CHEFZ_ST_NONE");
        ids2.Insert("CHEFZ_ST_SMOKED");
        ids2.Insert("CHEFZ_ST_DRIED");
        m2.Build(ids2, rep2, ChefZ_SyncLimits.STATE_ORDINAL_MAX);

        if (m2.GetMaxOrdinal() != m.GetMaxOrdinal())    return false;
        for (int o = 1; o <= 3; o++)
        {
            if (m2.NameOfOrdinal(o) != m.NameOfOrdinal(o))   return false;
        }

        // Sync-Obergrenze weist die ueberzaehligen Records ab.
        ChefZ_LoadReport rep3 = new ChefZ_LoadReport();
        rep3.SetMirrorToLog(false);
        ChefZ_IdentityMap m3 = new ChefZ_IdentityMap();
        m3.SetRegistryName("SELFTEST_LIMIT");
        array<string> ids3 = new array<string>();
        ids3.Insert("CHEFZ_LM_A");
        ids3.Insert("CHEFZ_LM_B");
        ids3.Insert("CHEFZ_LM_C");
        m3.Build(ids3, rep3, 2);
        if (m3.GetMaxOrdinal() != 2)            return false;
        if (rep3.ErrorCount() != 1)             return false;
        if (m3.GetRejected().Count() != 1)      return false;

        return true;
    }
}
