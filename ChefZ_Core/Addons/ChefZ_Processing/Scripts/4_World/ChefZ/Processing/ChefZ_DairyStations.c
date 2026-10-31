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
// KEINE FLUESSIGKEITSFUEHRUNG im Vanilla-Sinn. Am Butterfass ist Milch
// Vanillas PowderedMilk als Stueckware (ChefZ_Ingredients/config.cpp); die
// Ablehnung eines eigenen Fluessigkeitstyps steht in der config.cpp dieses
// Moduls. Seit dem 30.10.2026 gibt es daneben Milch als FUELLSTAND der
// ChefZ_MilkCan (Liter als varQuantity, Melken fuellt sie) - und die
// Kaesepresse liest ihn: Kanne in den Milchplatz, je 5 l ein Kaese
// (ChefZ_CheesePress unten). Der Weg ueber den Bruch (TR_CurdToCheese)
// bleibt daneben bestehen.
//
// ### 31.08.2026, Welle 2 ### Die Kaesekette ist jetzt zweistufig (Alex' Todo
// 10): Milch + ChefZ_MushroomCulture werden im Kessel zu ChefZ_CheeseCurd,
// und erst die Presse macht daraus ChefZ_Cheese. Der frueher direkte Weg
// TR_MilkToCheese (3 Milch -> Kaese) ist ersetzt, nicht ergaenzt - die
// Begruendung steht in der _note des neuen Transforms. Der Kesselschritt ist
// ein KOCHREZEPT in ChefZ_Cooking und beruehrt dieses Modul nicht.
//
// Auch nach dieser Aenderung bekommt die Presse KEINEN Torwaechter. Der
// Unterschied zum Butterfass ist nicht Bequemlichkeit, sondern Notwendigkeit:
// das Fass BRAUCHT einen, weil es ein Fassungsvermoegen hat, das es zaehlen
// muss ("20 Liter"), und weil sein Selbstnachstart sonst in fremdem Material
// wuehlt. Die Presse hat weder das eine noch das andere - sie nimmt einen
// Bruch, presst ihn 300 Sekunden lang und ist fertig. Ein Torwaechter waere
// hier reine Ordnungsliebe, und jede Zeile davon ein Ort, an dem der Bruch
// oder der fertige Laib versehentlich ausgesperrt werden kann.
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

//! Die Kaesepresse. Der Weg ueber den Bruch (TR_CurdToCheese) steht
//! vollstaendig in Cargo, Stationsdatensatz und Transform. KEIN
//! Selbstnachstart und KEIN Torwaechter - die Begruendung fuer beides steht im
//! Dateikopf.
//!
//! Dazu seit dem 30.10.2026 der Weg ueber die Milch (Spieltest: "je nach
//! Fuellmenge kann X Anzahl Kaese entstehen"): eine Milchkanne im Platz
//! ChefZ_PressMilkCan, ihr Fuellstand in Millilitern, je volle
//! CHEFZ_LITERS_PER_CHEESE ein Kaese. Gepresst wird mit
//! ChefZ_ActionPressMilkCheese; was nicht fuer einen ganzen Kaese reicht,
//! bleibt in der Kanne.
class ChefZ_CheesePress extends ChefZ_ProcessingStation_Base
{
    static const string CHEFZ_MILK_SLOT = "ChefZ_PressMilkCan";
    static const string CHEFZ_MILK_CAN = "ChefZ_MilkCan";
    static const string CHEFZ_CHEESE = "ChefZ_Cheese";
    static const float CHEFZ_LITERS_PER_CHEESE = 5000.0;
    //! Wie LIQUID_CHEFZ_MILK (ChefZ_MixMilk.c): gepresst wird nur Milch.
    static const int CHEFZ_LIQUID_MILK = 8388608;

    override void SetActions()
    {
        super.SetActions();
        AddAction(ChefZ_ActionPressMilkCheese);
    }

    //! Die Kanne im Milchplatz, sonst null.
    ItemBase ChefZ_GetMilkCan()
    {
        ItemBase can = ItemBase.Cast(FindAttachmentBySlotName(CHEFZ_MILK_SLOT));
        if (can && can.IsKindOf(CHEFZ_MILK_CAN))
            return can;
        return null;
    }

    //! Wie viele Kaese die Milch in der Kanne hergibt.
    int ChefZ_CheesesFromMilk()
    {
        ItemBase can = ChefZ_GetMilkCan();
        if (!can || can.GetLiquidType() != CHEFZ_LIQUID_MILK)
            return 0;
        float liters = can.GetQuantity();
        return Math.Floor(liters / CHEFZ_LITERS_PER_CHEESE);
    }

    //! Presst so viele Kaese, wie Milch und Laderaum hergeben, und zieht die
    //! Milch dafuer ab. Nur auf dem Server. Liefert die Zahl der Kaese.
    int ChefZ_PressMilk()
    {
        if (!g_Game || !g_Game.IsServer())
            return 0;

        ItemBase can = ChefZ_GetMilkCan();
        int wanted = ChefZ_CheesesFromMilk();
        int made = 0;
        for (int i = 0; i < wanted; i++)
        {
            EntityAI cheese = GetInventory().CreateEntityInCargo(CHEFZ_CHEESE);
            if (!cheese)
                break;
            made++;
        }

        if (made > 0)
            can.AddQuantity(-CHEFZ_LITERS_PER_CHEESE * made);
        return made;
    }
}

//==============================================================================
// "Kaese aus Milch pressen" - die Aktion an der Presse (30.10.2026).
//==============================================================================
class ChefZ_ActionPressMilkCheeseCB extends ActionContinuousBaseCB
{
    override void CreateActionComponent()
    {
        m_ActionData.m_ActionComponent = new CAContinuousTime(ChefZ_ActionPressMilkCheese.PRESS_SECONDS);
    }
}

class ChefZ_ActionPressMilkCheese extends ActionContinuousBase
{
    static const float PRESS_SECONDS = 10.0;

    void ChefZ_ActionPressMilkCheese()
    {
        m_CallbackClass = ChefZ_ActionPressMilkCheeseCB;
        m_CommandUID    = DayZPlayerConstants.CMD_ACTIONFB_CRAFTING;
        m_FullBody      = true;
        m_StanceMask    = DayZPlayerConstants.STANCEMASK_CROUCH;
        m_Text          = "#STR_CHEFZ_ACTION_PRESSMILKCHEESE";
    }

    override typename GetInputType()
    {
        return ContinuousInteractActionInput;
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem   = new CCINone();
        m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
    }

    override bool HasProgress()
    {
        return true;
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        ChefZ_CheesePress press = ChefZ_CheesePress.Cast(target.GetObject());
        if (!press)
            return false;
        return press.ChefZ_CheesesFromMilk() > 0;
    }

    override void OnFinishProgressServer(ActionData action_data)
    {
        ChefZ_CheesePress press = ChefZ_CheesePress.Cast(action_data.m_Target.GetObject());
        if (press)
            press.ChefZ_PressMilk();
    }
}

modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ChefZ_ActionPressMilkCheese);
    }
}
