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
// (der Kontext ist ein Strom, nicht ein Verzeichnis).
//
// ---------------------------------------------------------------------------
// WARUM EIN MARKER DAVORSTEHT UND WARUM EIN FEHLSCHLAG NICHT MEHR TOEDLICH IST
// ---------------------------------------------------------------------------
// Frueher stand hier nur die Versionszahl, und ein falscher erster Wert
// quittierte mit false. false in OnStoreLoad heisst: DayZ verwirft den ganzen
// Charakter. Das ist zu scharf, denn der Block steht an einer Stelle, die uns
// nicht gehoert:
//
//   1. DayZ 1.30 haengt in PlayerBase.OnStoreSave einen NEUEN Vanilla-Wert ans
//      Ende des Vanilla-Teils (m_ThermalBiasHandler,
//      1.30 scripts/4_World/DayZ/Entities/ManBase/PlayerBase.c:7395) und liest
//      ihn in OnStoreLoad OHNE Versions-Gate (:7519-7524) - anders als das
//      direkt darueber stehende "if (version >= 134)" (:7513). In 1.29 gibt es
//      weder die Zeile noch die Datei ThermalBiasHandler.c; GAME_STORAGE_VERSION
//      steigt von 142 (1.29 3_Game/DayZ/Global/Game.c:5) auf 144. Bei einem
//      unter 1.29 geschriebenen Charakter frisst dieser Read die ersten 4 Byte
//      des ersten MOD-Blocks - also unsere.
//   2. Die Reihenfolge der modded-PlayerBase-Schichten haengt allein an der
//      -mod-Zeile des Betreibers. Es gibt keine Kante im Abhaengigkeitsgraphen
//      zwischen ChefZ und TerjeCore, und es darf keine geben: TerjeCore in
//      requiredAddons zu schreiben machte Terje zur Pflicht und widerspraeche
//      dem Zweck des Moduls. Tauscht der Betreiber die Reihenfolge nach dem
//      ersten Start, liest jede Schicht den Block der anderen.
//
// In beiden Faellen kann ChefZ den Strom nicht reparieren - der Schaden
// entsteht vor der ersten ChefZ-Zeile. Was ChefZ tun kann: nicht noch den
// Charakter mitreissen. Deshalb gilt jetzt "Wissen verwerfen statt Charakter
// verwerfen". Der Marker macht den Fehlgriff dabei erkennbar, statt ihn still
// als Versionszahl fehlzudeuten - dasselbe Mittel, das TerjeCore benutzt.
//
// Ehrlich bleibt dabei: das rettet nur, wenn ChefZ die AEUSSERSTE Schicht ist.
// Ein bereits verschobener Strom trifft jede weitere Schicht ohnehin; liegt
// TerjeCore hinter uns, faellt dessen eigene Markerpruefung durch. Fuer den
// Sprung auf 1.30 bleibt die Betriebsanweisung bestehen: Charakterdatenbank
// leeren, sonst ist der Verlust systematisch.
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
    //! Erkennungsmarke vor dem Block. Willkuerlich, aber fest: die vier
    //! Buchstaben "CHZK" als int (0x43485A4B). Sie beantwortet beim Lesen die
    //! Frage "gehoert das hier ueberhaupt uns", die eine blosse Versionszahl
    //! nicht beantworten kann - eine 1 sieht wie jede andere 1 aus.
    static const int CHEFZ_KNOWLEDGE_MARKER = 1128815179;

    //! Version DIESES Blocks, nicht die von DayZ.
    static const int CHEFZ_KNOWLEDGE_VERSION = 1;

    //! Aufbau vor dem Marker: ein nackter int 1, direkt gefolgt von den Daten.
    //! Wird nur noch gelesen, nie mehr geschrieben.
    static const int CHEFZ_KNOWLEDGE_LEGACY_VERSION = 1;

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

        ctx.Write(CHEFZ_KNOWLEDGE_MARKER);
        ctx.Write(CHEFZ_KNOWLEDGE_VERSION);
        ChefZ_GetKnowledge().Save(ctx);
    }

    /**
     * Liest den Wissensblock - und gibt niemals false zurueck, sobald super
     * durch ist.
     *
     * Siehe Kopf: was an dieser Stelle im Strom steht, entscheidet nicht
     * ChefZ. Ein falscher Wert kostet deshalb das Wissen, nicht den Charakter.
     * false bleibt allein dem Vanilla-Teil vorbehalten.
     */
    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        int marker;
        if (!ctx.Read(marker))
        {
            ChefZ_DiscardKnowledgeBlock("der Strom endet vor dem Wissensblock");
            return true;
        }

        if (marker == CHEFZ_KNOWLEDGE_LEGACY_VERSION)
        {
            // Aufbau vor dem Marker: der erste int war die Blockversion.
            if (!ChefZ_GetKnowledge().Load(ctx))
                ChefZ_DiscardKnowledgeBlock("der alte Wissensblock liess sich nicht lesen");
            return true;
        }

        if (marker != CHEFZ_KNOWLEDGE_MARKER)
        {
            ChefZ_DiscardKnowledgeBlock("an dieser Stelle steht kein ChefZ-Block (gelesen: " + marker.ToString() + ")");
            return true;
        }

        int blockVersion;
        if (!ctx.Read(blockVersion))
        {
            ChefZ_DiscardKnowledgeBlock("die Blockversion liess sich nicht lesen");
            return true;
        }

        if (blockVersion != CHEFZ_KNOWLEDGE_VERSION)
        {
            string abweichung = "Blockversion " + blockVersion.ToString();
            abweichung = abweichung + ", erwartet wird " + CHEFZ_KNOWLEDGE_VERSION.ToString();
            ChefZ_DiscardKnowledgeBlock(abweichung);
            return true;
        }

        if (!ChefZ_GetKnowledge().Load(ctx))
            ChefZ_DiscardKnowledgeBlock("der Wissensblock brach mitten im Lesen ab");

        return true;
    }

    /**
     * Wissen fallen lassen und sagen, warum.
     *
     * Der Charakter bleibt. Er faengt beim Kochbuch bei null an, was sich durch
     * Spielen wieder fuellt - anders als ein geloeschter Spielstand.
     */
    private void ChefZ_DiscardKnowledgeBlock(string grund)
    {
        ChefZ_GetKnowledge().Clear();

        string warn = "Kochbuch: Wissensblock verworfen, der Charakter bleibt erhalten. Grund: ";
        warn = warn + grund + ".";
        ChefZ_Log.Warn(ChefZ_LogChannel.CORE, warn);
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
