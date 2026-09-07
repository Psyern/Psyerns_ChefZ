//==============================================================================
// ChefZ_PlayerKnowledge - der Wissensstand haengt am Charakter
//
// Entwurf: ChefZ_Cookbook_Workflow §4.3 ("Alles serverseitig"), §5 (RPC).
//
// ---------------------------------------------------------------------------
// WARUM AM SPIELER UND NICHT IN EINER ZENTRALEN TABELLE
// ---------------------------------------------------------------------------
// Weil DayZ genau dafuer OnStoreSave/OnStoreLoad hat. Eine Tabelle im Manager
// muesste bei jedem Verbindungsabbruch aufgeraeumt und bei jedem Login
// nachgeladen werden, und jeder vergessene Eintrag waere ein Leck, das erst
// nach Wochen auffaellt. Am Charakter erledigt das die Engine.
//
// ---------------------------------------------------------------------------
// DIE VERSIONSZAHL IM SPEICHERBLOCK
// ---------------------------------------------------------------------------
// Vor den Daten steht eine eigene Version. DayZ reicht in OnStoreLoad seine
// eigene herein, aber die sagt nichts ueber DIESEN Block. Wer sie stattdessen
// benutzt, kann seinen Aufbau nie aendern, ohne auf ein Spielupdate zu warten.
//
// Beim Lesen gilt: unbekannte Version -> Block ueberspringen ist NICHT moeglich
// (der Kontext ist ein Strom, nicht ein Verzeichnis). Deshalb wird eine
// unbekannte Version als Lesefehler behandelt und der Spielerstand verworfen -
// das ist haesslich, aber ehrlich. Solange die Version stimmt, passiert das nie.
//
// Layer: 4_World. Keine Dabs-Referenz (Regel 3).
//==============================================================================

// SCOUT-GEPRUEFT 2026-08-30 (chefz-conflict-scout)
// Alle Member und Methoden praefixiert, super in
// EEInit/OnStoreSave/OnStoreLoad/OnRPC zuerst, RPC-Nummern 10000-10002
// kollisionsfrei gegen COT (ab 10100), Terje (negativ), Dabs und CF. Der
// am selben Tag gefundene FULL_STATE-Exploit ist behoben - siehe die Wache
// im Zweig.
modded class PlayerBase
{
    //! Version DIESES Blocks, nicht die von DayZ.
    static const int CHEFZ_KNOWLEDGE_VERSION = 1;

    private ref ChefZ_KnowledgeState m_ChefZ_Knowledge;

    //! Gesammelte, noch nicht verschickte Zutaten. Der Inventarhaken schreibt
    //! hier hinein, der gebuendelte Lauf leert es - siehe ChefZ_KnowledgeHooks.
    private ref array<int> m_ChefZ_PendingIngredients;

    //! Steht ein Bündellauf schon in der Warteschlange? Ohne diese Marke
    //! stellte ein Rucksack mit 40 Gegenstaenden 40 Laeufe ein.
    private bool m_ChefZ_FlushScheduled;

    void PlayerBase()
    {
        m_ChefZ_Knowledge          = new ChefZ_KnowledgeState();
        m_ChefZ_PendingIngredients = new array<int>();
        m_ChefZ_FlushScheduled     = false;
    }

    ChefZ_KnowledgeState ChefZ_GetKnowledge()
    {
        if (!m_ChefZ_Knowledge)
            m_ChefZ_Knowledge = new ChefZ_KnowledgeState();
        return m_ChefZ_Knowledge;
    }

    /**
     * Der Einhaengepunkt des Addons.
     *
     * Bewusst hier und nicht in einem eigenen "modded class MissionServer":
     * zwei Comp-Module mit je einem eigenen MissionServer-Override haben den
     * Server am 28.08.2026 mit einer Zugriffsverletzung beendet. Vor dem ersten
     * Spieler kann niemand kochen - der Zeitpunkt reicht, und er kostet keinen
     * zweiten Einstiegspunkt.
     */
    override void EEInit()
    {
        super.EEInit();
        ChefZ_CookbookServer.EnsureAttached();
    }

    //==========================================================================
    // Bündelung des Inventarhakens
    //==========================================================================

    /**
     * Vormerken, nicht sofort verarbeiten.
     *
     * §4.3 nennt den Fallstrick beim Namen: ein Rucksack mit 40 Gegenstaenden
     * loest 40 Inventarereignisse in EINEM Frame aus. Ungedrosselt waeren das
     * 40 Ableitungslaeufe und 40 Netzwerknachrichten fuer eine einzige
     * Spielerhandlung.
     *
     * Deshalb wird nur gemerkt und ein einziger Lauf eingestellt.
     */
    void ChefZ_NoteIngredient(ChefZ_Sym classSym)
    {
        if (!ChefZ_SymbolTable.IsValid(classSym))
            return;

        int wert = classSym;
        if (m_ChefZ_PendingIngredients.Find(wert) >= 0)
            return;
        m_ChefZ_PendingIngredients.Insert(wert);

        if (m_ChefZ_FlushScheduled)
            return;
        if (!g_Game)
            return;

        m_ChefZ_FlushScheduled = true;
        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "ChefZ_FlushIngredients", 0, false);
    }

    /**
     * Der gebuendelte Lauf. Genau einmal je Frame, in dem etwas anfiel.
     *
     * Oeffentlich, weil CallLaterByName den Namen zur Laufzeit aufloest - eine
     * private Methode faende die Warteschlange nicht.
     */
    void ChefZ_FlushIngredients()
    {
        m_ChefZ_FlushScheduled = false;

        if (m_ChefZ_PendingIngredients.Count() == 0)
            return;

        ChefZ_KnowledgeState stand = ChefZ_GetKnowledge();
        int neu = 0;
        for (int i = 0; i < m_ChefZ_PendingIngredients.Count(); i++)
        {
            if (stand.AddIngredient(m_ChefZ_PendingIngredients.Get(i)))
                neu++;
        }
        m_ChefZ_PendingIngredients.Clear();

        if (neu == 0)
            return;

        ChefZ_MarkKnowledgeDirty();
        ChefZ_SendFullState();

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.CORE, ChefZ_LogLevel.DEBUG))
        {
            string zeile = "Kochbuch: " + neu.ToString() + " neue Zutat(en) gelernt. " + stand.ToLine();
            ChefZ_Log.Debug(ChefZ_LogChannel.CORE, zeile);
        }
    }

    //==========================================================================
    // Persistenz
    //==========================================================================

    //! Der Charakter wird beim naechsten Speicherpunkt geschrieben. DayZ
    //! entscheidet wann; wir setzen nur die Marke.
    void ChefZ_MarkKnowledgeDirty()
    {
        SetSynchDirty();
    }

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);

        ctx.Write(CHEFZ_KNOWLEDGE_VERSION);
        ChefZ_GetKnowledge().Save(ctx);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        int blockVersion;
        if (!ctx.Read(blockVersion))
            return false;

        if (blockVersion != CHEFZ_KNOWLEDGE_VERSION)
        {
            string warn = "Kochbuch: gespeicherter Wissensblock hat Version " + blockVersion.ToString();
            warn = warn + ", erwartet wird " + CHEFZ_KNOWLEDGE_VERSION.ToString() + ".";
            ChefZ_Log.Warn(ChefZ_LogChannel.CORE, warn);
            return false;
        }

        return ChefZ_GetKnowledge().Load(ctx);
    }

    //==========================================================================
    // Synchronisation
    //==========================================================================

    /**
     * Schickt den vollstaendigen Stand an den eigenen Client.
     *
     * Nur der volle Stand, kein Delta: §5 sieht beides vor, aber das Delta
     * lohnt erst, wenn die Mengen gross werden. Bei knapp 200 Zutaten ist der
     * volle Stand ein paar Kilobyte, und eine zweite Nachrichtenart waere eine
     * zweite Stelle, an der Client und Server auseinanderlaufen koennen.
     * ChefZ_CookbookRPC.DELTA bleibt reserviert.
     */
    void ChefZ_SendFullState()
    {
        if (!g_Game || !g_Game.IsDedicatedServer())
            return;

        PlayerIdentity id = GetIdentity();
        if (!id)
            return;

        ChefZ_KnowledgeState stand = ChefZ_GetKnowledge();

        array<string> zutaten = new array<string>();
        for (int i = 0; i < stand.IngredientCount(); i++)
            zutaten.Insert(ChefZ_SymbolTable.Name(stand.IngredientAt(i)));

        array<string> rezepte = new array<string>();
        for (int j = 0; j < stand.MasteredCount(); j++)
            rezepte.Insert(ChefZ_SymbolTable.Name(stand.MasteredAt(j)));

        Param2<ref array<string>, ref array<string>> daten = new Param2<ref array<string>, ref array<string>>(zutaten, rezepte);
        g_Game.RPCSingleParam(this, ChefZ_CookbookRPC.FULL_STATE, daten, true, id);
    }

    /**
     * Empfang.
     *
     * Param6 und Verwandte werden als EIN Objekt gelesen - die Enforce-Skill
     * ist da eindeutig, und feldweises Lesen ist der haeufigste Grund fuer
     * einen stillen Fehlschlag an dieser Stelle.
     */
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (!ChefZ_CookbookRPC.IsOurs(rpc_type))
            return;

        if (rpc_type == ChefZ_CookbookRPC.FULL_STATE)
        {
            // NUR DER CLIENT NIMMT EINEN STAND ENTGEGEN.
            //
            // OnRPC laeuft auf BEIDEN Seiten, und welches Objekt ein RPC
            // erreicht, bestimmt der Absender. Ohne diese Zeile koennte ein
            // Client dem Server ein FULL_STATE an seinen eigenen Spieler
            // schicken: ChefZ_ReceiveFullState ruft stand.Clear() und
            // schreibt danach die mitgelieferten Listen - jedes Rezept als
            // gemeistert, oder der Stand geloescht. OnStoreSave schreibt das
            // anschliessend in den Spielstand.
            //
            // FULL_STATE ist Server->Client (ChefZ_SendFullState), also ist
            // ein FULL_STATE AM SERVER immer gefaelscht und wird verworfen.
            // Dasselbe Muster wie im Zweig darunter, nur andersherum -
            // Vanilla klammert seine Server->Client-Zweige in PlayerBase.OnRPC
            // aus demselben Grund mit #ifndef SERVER.
            if (g_Game && g_Game.IsDedicatedServer())
                return;

            ChefZ_ReceiveFullState(ctx);
            return;
        }

        if (rpc_type == ChefZ_CookbookRPC.REQUEST_STATE)
        {
            // Nur der Server beantwortet das, und nur fuer den Absender.
            if (g_Game && g_Game.IsDedicatedServer())
                ChefZ_SendFullState();
            return;
        }
    }

    private void ChefZ_ReceiveFullState(ParamsReadContext ctx)
    {
        Param2<ref array<string>, ref array<string>> daten = new Param2<ref array<string>, ref array<string>>(null, null);
        if (!ctx.Read(daten))
            return;

        ChefZ_KnowledgeState stand = ChefZ_GetKnowledge();
        stand.Clear();

        array<string> zutaten = daten.param1;
        if (zutaten)
        {
            for (int i = 0; i < zutaten.Count(); i++)
                stand.AddIngredient(ChefZ_SymbolTable.Lookup(zutaten.Get(i)));
        }

        array<string> rezepte = daten.param2;
        if (rezepte)
        {
            for (int j = 0; j < rezepte.Count(); j++)
                stand.AddMastered(ChefZ_SymbolTable.Lookup(rezepte.Get(j)));
        }
    }
}
