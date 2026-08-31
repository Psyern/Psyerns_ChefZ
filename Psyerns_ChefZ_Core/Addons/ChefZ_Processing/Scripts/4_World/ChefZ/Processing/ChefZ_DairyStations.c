//==============================================================================
// Die beiden Stationen der Milchkette (Slice "dairy").
//
// Andockregel woertlich aus dem Kopf von ChefZ_ProcessingStation_Base.c:
//
//   config.cpp        class ChefZ_ButterChurn : <eine Vanilla-Klasse> { ... };
//   Stationsdatensatz id == Klassenname, processes[] = { ... }
//   Skript            class ChefZ_ButterChurn extends ChefZ_ProcessingStation_Base
//
// Welche Prozesse eine Station anbietet, steht ausschliesslich im
// Stationsdatensatz (Config/Processing/Dairy_Stations.json); was aus welchem
// Eingang wird, steht im Transform (Config/Processing/Dairy_Transforms.json).
// Das bleibt so - was hier steht, ist ausschliesslich das, was Daten nicht
// ausdruecken koennen.
//
//==============================================================================
// ### 31.08.2026 ### DAS FASS ARBEITET JETZT VON SELBST
//==============================================================================
//
// Alex' Zielbild, woertlich: "Milch einfuellen (20 Liter), aktiv
// interagieren, alle 60 Sekunden entsteht 1x Butter."
//
// Drei Dinge folgen daraus, und keines davon laesst sich in einer JSON sagen:
//
//   1. TAKT. PROCESS_CHURN_BUTTER steht auf 60 s, beide Transforms tragen
//      dieselbe Zahl als durationOverrideSec. Die VERHAELTNISSE bleiben
//      unangetastet: 2 Milch -> 1 Sahne, 2 Sahne -> 1 Butter, also vier
//      Milch je Butter. Alex' "alle 60 Sekunden 1x Butter" ist damit der
//      Takt der Station, nicht die Ausbeute je Takt - vier Karton Milch
//      ergeben nach zwei Abrahm- und einem Schlagvorgang ein Stueck Butter,
//      und ab da laeuft es im Wechsel weiter.
//
//   2. SELBSTNACHSTART. Der Spieler kurbelt EINMAL an, danach gehoert der
//      Takt dem Fass. Nach jedem Abschluss versucht es ZUERST das Buttern und
//      erst danach das Abrahmen. Die Reihenfolge ist der ganze Trick: sobald
//      zwei Sahne dastehen, werden sie zu Butter; ist nur Milch da, rahmt es
//      ab. So pendelt eine Ladung ohne weiteres Zutun durch beide Stufen,
//      ohne dass irgendwo eine Ablaufsteuerung stuende.
//
//      Das Muster ist nicht neu erfunden: es ist woertlich das der
//      Honigschleuder (ChefZ_HoneyExtractor.ChefZ_CompleteJob, dieselbe
//      Ordnerebene), einschliesslich der Begruendung fuer CallLater - super
//      hat den Job-Timer gerade angehalten, und ein Timer.Run aus dem eigenen
//      Rueckruf heraus ist kein belegter Pfad (GeyserArea.c:49,
//      FireplaceBase.c:1767).
//
//   3. FASSUNGSVERMOEGEN. "20 Liter" heisst in einem Modul ohne
//      Fluessigkeitsfuehrung: 20 Karton PowderedMilk. Die Umrechnung und die
//      Gittergroesse stehen ausgeschrieben an der Configklasse; gezaehlt wird
//      hier, weil ein Gitter nur Platz zaehlen kann und keine Sorten.
//
// KEINE FLUESSIGKEITSFUEHRUNG, auch jetzt nicht. Milch ist und bleibt
// Vanillas PowderedMilk als Stueckware (ChefZ_Ingredients/config.cpp:389); die
// Ablehnung steht ausfuehrlich in der config.cpp dieses Moduls. Eine echte
// ChefZ-Milchklasse gibt es nicht: ChefZ_MilkCan ist ein leeres Traggut ohne
// Zutatendatensatz (ChefZ_Ingredients/config.cpp:657) und deshalb in keinem
// Transform genannt.
//
// Die Kaesepresse bleibt leer. Sie hat einen Cargo (6x4), einen
// Stationsdatensatz und mit TR_MilkToCheese einen erreichbaren Transform -
// ihr fehlt nichts, was ein Skript geben koennte.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_ButterChurn extends ChefZ_ProcessingStation_Base
{
    //! Die beiden Prozesse, die das Fass sich selbst nachstartet - in dieser
    //! Reihenfolge. Buttern zuerst: liegt genug Sahne da, soll sie Butter
    //! werden, statt dass noch mehr Sahne entsteht.
    static const string CHEFZ_CHURN_PROCESS    = "PROCESS_CHURN_BUTTER";
    static const string CHEFZ_SEPARATE_PROCESS = "PROCESS_SEPARATE_CREAM";

    //! DAIRY trifft ueber die Closure auch CREAM und BUTTER (Kategoriebaum:
    //! ChefZ_Registry/Config/Categories.json). Eine Kategorie reicht damit fuer
    //! Eingang UND Ergebnis - und das ist noetig, denn beide liegen im selben
    //! Cargo. Ein Torwaechter, der nur an die Milch denkt, sperrt die Sahne
    //! aus, die das Fass gerade selbst erzeugt hat.
    static const string CHEFZ_CAT_DAIRY  = "DAIRY";
    static const string CHEFZ_CAT_BUTTER = "BUTTER";

    //! "20 Liter" in Stueck. Gezaehlt wird alles Milchige AUSSER Butter:
    //! Butter ist das Ergebnis und darf liegen bleiben, ohne den Platz fuer
    //! Nachschub wegzunehmen. Die Zahl steht hier und nicht im Gitter - das
    //! Gitter gibt den Platz, die Station gibt die Fassung. Dieselbe
    //! Arbeitsteilung benutzt ChefZ_HoneyExtractor fuer Rahmen und Glaeser.
    static const int CHEFZ_MILK_CAPACITY = 20;

    //! Der Spieler, der angekurbelt hat. Der Folgejob traegt seine Kennung,
    //! damit XP an ihn geht - und erst nach dem Abschluss (Regel 7).
    //! 0 heisst "niemand beteiligt", wie ueberall im Core.
    protected int m_ChefZ_PendingActorId;

    void ChefZ_ButterChurn()
    {
        m_ChefZ_PendingActorId = 0;
    }

    /**
     * Milchiges hinein, sonst nichts - und Nachschub nur bis zur Fassung.
     *
     * Beleg: EntityAI.CanReceiveItemIntoCargo, scripts - 1.29/3_Game/DayZ/
     * Entities/EntityAI.c:1550-1559; Ueberschreibung wie Barrel_ColorBase.c:512.
     */
    override bool CanReceiveItemIntoCargo(EntityAI item)
    {
        if (!super.CanReceiveItemIntoCargo(item))
            return false;
        if (!item)
            return false;

        // Solange die Register nicht stehen, wird nichts abgewiesen - sonst
        // naehme das Fass in genau diesem Fenster gar nichts an.
        if (!ChefZ_StationGate.ChefZ_RegistryReady())
            return true;

        if (!ChefZ_StationGate.ChefZ_InCategory(item, CHEFZ_CAT_DAIRY))
            return false;

        // Butter zaehlt nicht mit: sie ist das Ergebnis. Waere sie
        // fassungsrelevant, koennte ein volles Fass seine eigene Butter nicht
        // mehr ablegen und stuende ohne Verbrauch still.
        if (ChefZ_StationGate.ChefZ_InCategory(item, CHEFZ_CAT_BUTTER))
            return true;

        return ChefZ_CountRawDairy() < CHEFZ_MILK_CAPACITY;
    }

    //! Milch und Sahne im Cargo - alles Milchige ausser Butter.
    protected int ChefZ_CountRawDairy()
    {
        int count = 0;

        GameInventory inventory = GetInventory();
        if (!inventory)
            return count;

        CargoBase cargo = inventory.GetCargo();
        if (!cargo)
            return count;

        int n = cargo.GetItemCount();
        for (int i = 0; i < n; i++)
        {
            EntityAI entry = cargo.GetItem(i);
            if (!ChefZ_StationGate.ChefZ_InCategory(entry, CHEFZ_CAT_DAIRY))
                continue;
            if (ChefZ_StationGate.ChefZ_InCategory(entry, CHEFZ_CAT_BUTTER))
                continue;
            count = count + 1;
        }
        return count;
    }

    /**
     * Der Abschluss eines Vorgangs stoesst den naechsten an.
     *
     * Die Spielerkennung wird VOR super gesichert: die Basis loescht den Job
     * nach dem Lauf (job.Clear()).
     *
     * Anders als beim Fleischwolf wird hier NICHT unterschieden, welcher der
     * beiden Prozesse gerade fertig wurde - beide gehoeren zur selben Kette,
     * und der Nachstart probiert ohnehin beide der Reihe nach.
     */
    override bool ChefZ_CompleteJob(int slotIndex)
    {
        int actorId = 0;
        if (slotIndex >= 0 && slotIndex < m_ChefZ_Jobs.Count())
        {
            ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(slotIndex);
            actorId = job.actorIdentityId;
        }

        bool ok = super.ChefZ_CompleteJob(slotIndex);
        if (!ok)
            return false;

        m_ChefZ_PendingActorId = actorId;
        if (g_Game)
            g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ChefZ_ChurnNext, 0, false);

        return true;
    }

    /**
     * Erst buttern, dann abrahmen. Oeffentlich, weil CallLater ihn ruft.
     *
     * Scheitern beide, steht das Fass - kein Material, oder der Cargo ist voll
     * - und wartet auf das naechste Ankurbeln. Das Nachlegen allein startet
     * nichts; der Anstoss gehoert dem Spieler.
     */
    void ChefZ_ChurnNext()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        if (ChefZ_TryStart(CHEFZ_CHURN_PROCESS))
            return;

        ChefZ_TryStart(CHEFZ_SEPARATE_PROCESS);
    }

    //! Ein Startversuch. false heisst "geht gerade nicht" und ist der
    //! Normalfall am Ende einer Ladung, kein Fehler.
    protected bool ChefZ_TryStart(string processName)
    {
        ChefZ_Sym process = ChefZ_SymbolTable.Lookup(processName);
        if (!ChefZ_SymbolTable.IsValid(process))
            return false;

        string err;
        if (ChefZ_BeginJob(process, null, m_ChefZ_PendingActorId, err))
            return true;

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS, "Butterfass \"" + GetType() + "\" startet " + processName + " nicht: " + err);
        }
        return false;
    }
}

//! Die Kaesepresse. Leer, und das ist kein Versehen: Cargo, Stationsdatensatz
//! und TR_MilkToCheese sind vollstaendig, es gibt nichts, was ein Skript
//! beisteuern koennte. KEIN Selbstnachstart - Kaese ist ein einzelner
//! Vorgang, kein Takt.
class ChefZ_CheesePress extends ChefZ_ProcessingStation_Base
{
}
