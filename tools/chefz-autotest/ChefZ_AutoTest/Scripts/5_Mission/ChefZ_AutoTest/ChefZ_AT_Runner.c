//==============================================================================
// ChefZ_AutoTest - der Ablauf. Jeder Fall laeuft in einem eigenen CallLater-Schritt,
// damit Spawns und Loeschungen zwischen den Faellen einen Frame Luft haben.
//
//   1. warten, bis der Testspieler da ist (Diag-Client, hub_launch_diag --mode both)
//   2. COOK   alle Kochrezepte ueber Vanillas Cooking.CookWithEquipment
//   3. PROC   alle Transformationen (Handarbeit, Stationsaktion, Stationsauftrag)
//   4. DEVICE jede Station: Kollision, Prozesse, Raeucherofen heizt
//   5. FISH   alle Ertraege in Rute/Reuse, Koederwirkung
//   6. MEAL   Mahlzeitbonus
//   7. Zusammenfassung, Client beenden, Server beenden
//==============================================================================

class ChefZ_AT_Runner
{
    static ref ChefZ_AT_Runner s_Instance;

    PlayerBase m_Player;
    vector m_Site;
    int m_WaitSec;
    ref array<ref ChefZ_AT_Case> m_Cases;
    int m_Index;

    // Schritte zwischen Start und Pruefung (Handarbeit spawnt verzoegert)
    ref ChefZ_AT_Case m_Pending;
    ItemBase m_PendingStation;
    ItemBase m_PendingFire;
    ItemBase m_PendingTool;
    float m_PendingToolHp;
    string m_PendingToolNote;

    // Erstes zwischengelagertes Kochergebnis: wandert es im naechsten Frame
    // wirklich ins Gefaess? (ChefZ_Applicator.TakeStaged)
    ItemBase m_StagedDev;
    EntityAI m_StagedItem;
    string m_StagedId;

    static void Start()
    {
        s_Instance = new ChefZ_AT_Runner();
        s_Instance.Begin();
    }

    void Begin()
    {
        ChefZ_AT_Log.Reset();
        m_Cases = new array<ref ChefZ_AT_Case>();
        ChefZ_AT_Cases.Fill(m_Cases);
        m_Index = 0;
        m_WaitSec = 0;
        ChefZ_AT_Log.Info("start cases=" + m_Cases.Count().ToString() + " health=" + ChefZ_ConfigManager.HealthName(ChefZ_ConfigManager.Get().GetHealth()));
        ChefZ_AT_Log.Info("fluessigkeiten saltwater=" + Liquid.GetLiquidClassname(LIQUID_SALTWATER) + " water=" + Liquid.GetLiquidClassname(LIQUID_WATER));
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(WaitForPlayer, 1000, false);
    }

    void SetPlayer(PlayerBase p)
    {
        if (!m_Player)
            m_Player = p;
    }

    void WaitForPlayer()
    {
        if (m_Player && m_Player.IsAlive())
        {
            ChefZ_AT_Log.Info("player " + m_Player.GetIdentity().GetName());
            m_Site = "4930 0 2470";
            m_Site[1] = g_Game.SurfaceY(m_Site[0], m_Site[2]);
            m_Player.SetPosition(m_Site + Vector(0, 0, -4));
            m_Player.SetAllowDamage(false);
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextCase, 3000, false);
            return;
        }
        m_WaitSec++;
        if (m_WaitSec > 300)
        {
            ChefZ_AT_Log.Result("SETUP", "player", false, "kein Testspieler nach 300 s");
            Finish();
            return;
        }
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(WaitForPlayer, 1000, false);
    }

    //==========================================================================
    // COOK / PROC
    //==========================================================================

    void NextCase()
    {
        if (m_Index >= m_Cases.Count())
        {
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RunDevices, 500, false);
            return;
        }
        ChefZ_AT_Case c = m_Cases.Get(m_Index);
        m_Index++;

        ChefZ_AT_Util.ClearArea(m_Site, 6);
        if (c.note != "")
        {
            ChefZ_AT_Log.Result(c.kind, c.id, false, "Generator: " + c.note);
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextCase, 100, false);
            return;
        }

        if (c.kind == "COOK")
        {
            RunCook(c);
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextCase, 300, false);
            return;
        }

        int wait = RunProcStart(c);
        if (wait < 0)
        {
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextCase, 300, false);
            return;
        }
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckPending, wait, false);
    }

    void RunCook(ChefZ_AT_Case c)
    {
        ItemBase dev = ChefZ_AT_Util.SpawnGround(c.device, m_Site);
        if (!dev)
        {
            ChefZ_AT_Log.Result("COOK", c.id, false, "Geraet " + c.device + " nicht spawnbar");
            return;
        }
        if (c.liquid != "")
        {
            dev.SetLiquidType(ChefZ_AT_Util.LiquidOf(c.liquid));
            dev.SetQuantity(c.liquidQty);
        }
        string err = ChefZ_AT_Util.FillCargo(dev, c);
        if (err != "")
        {
            ChefZ_AT_Log.Result("COOK", c.id, false, err + " inputs=" + c.DescribeInputs());
            g_Game.ObjectDelete(dev);
            return;
        }

        Cooking cooking = new Cooking();
        cooking.SetCookingUpdateTime(5.0);
        array<EntityAI> items = new array<EntityAI>();
        string missing;
        EntityAI made;
        int tick;
        int oldLevel = ChefZ_Log.GetLevel();
        int oldMask = ChefZ_Log.GetChannelMask();
        int lastState = -2;
        string lastStages = "";
        // 400 Takte: TIMED-Rezepte wie die Knochenbruehe (420 s) brauchen mehr
        // als die frueheren 160.
        for (tick = 0; tick < 400; tick++)
        {
            // Der erste Takt mit Match-Protokoll: warum ein Rezept trifft oder nicht.
            if (tick == 0)
            {
                ChefZ_Log.SetLevel(ChefZ_LogLevel.DEBUG);
                ChefZ_Log.SetChannel(ChefZ_LogChannel.MATCH, true);
                ChefZ_AT_Log.Info("trace " + c.id);
            }
            if (tick == 1 && !IsFocus(c.id))
            {
                ChefZ_Log.SetLevel(oldLevel);
                ChefZ_Log.SetChannel(ChefZ_LogChannel.MATCH, (oldMask & ChefZ_LogChannel.MATCH) != 0);
            }
            dev.SetTemperatureDirect(110);
            if (c.liquid != "" && dev.GetQuantity() < c.liquidQty)
                dev.SetQuantity(c.liquidQty);
            items.Clear();
            ChefZ_AT_Util.CollectCargo(dev, items);
            foreach (EntityAI e : items)
                e.SetTemperatureDirect(110);   // Vanillas Kochstufe braucht 105 °C (cooking_properties {105,45})
            cooking.CookWithEquipment(dev, 1.0);
            ChefZ_CookSession ses = ChefZ_CookingDeviceAdapter.Get().PeekSession(dev);
            int st = -1;
            if (ses)
                st = ses.state;
            if (st != lastState)
            {
                ChefZ_AT_Log.Info("state " + c.id + " tick=" + tick.ToString() + " -> " + typename.EnumToString(ChefZ_ESessionState, st));
                lastState = st;
            }
            items.Clear();
            ChefZ_AT_Util.CollectCargo(dev, items);
            if (IsFocus(c.id))
            {
                string stages = DescribeItems(items);
                if (stages != lastStages)
                {
                    ChefZ_AT_Log.Info("stages " + c.id + " tick=" + tick.ToString() + " state=" + typename.EnumToString(ChefZ_ESessionState, st) + " " + stages);
                    lastStages = stages;
                }
            }
            if (ChefZ_AT_Util.HasOutputs(items, c.outputs, missing, made))
                break;
            // Zwischengelagerte Ergebnisse (ChefZ_Applicator, kein Platz neben
            // den Zutaten) liegen bis zum naechsten Frame neben dem Gefaess -
            // diese Schleife laeuft aber in EINEM Frame.
            array<EntityAI> near = new array<EntityAI>();
            ChefZ_AT_Util.CollectNear(dev.GetPosition(), 1.5, near);
            if (ChefZ_AT_Util.HasOutputs(near, c.outputs, missing, made))
                break;
        }

        ChefZ_Log.SetLevel(oldLevel);
        ChefZ_Log.SetChannel(ChefZ_LogChannel.MATCH, (oldMask & ChefZ_LogChannel.MATCH) != 0);
        if (made)
            ChefZ_AT_Log.Result("COOK", c.id, true, "ticks=" + tick.ToString() + " out=" + made.GetType() + " stufe=" + ChefZ_AT_Util.TierOf(made) + " inputs=" + c.DescribeInputs());
        else
            ChefZ_AT_Log.Result("COOK", c.id, false, "fehlt " + missing + "nach " + tick.ToString() + " Takten; " + CookDiag(dev) + " im Topf: " + DescribeItems(items) + " inputs=" + c.DescribeInputs());
        // Ein zwischengelagertes Ergebnis liegt ausserhalb des Gefaesses und
        // wuerde mit ihm nicht geloescht. Das erste bleibt samt Gefaess eine
        // Sekunde liegen: dann muss es im Gefaess sein.
        if (made && !made.GetHierarchyParent())
        {
            if (!m_StagedItem)
            {
                m_StagedDev = dev;
                m_StagedItem = made;
                m_StagedId = c.id;
                // Aus dem Bereich, den ClearArea vor dem naechsten Fall leert.
                vector away = m_Site + Vector(20, 0, 0);
                dev.SetPosition(away);
                made.SetPosition(away + Vector(0.5, 0, 0));
                g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckStaged, 1000, false);
                return;
            }
            g_Game.ObjectDelete(made);
        }
        g_Game.ObjectDelete(dev);
    }

    void CheckStaged()
    {
        bool inside = m_StagedItem && m_StagedDev && m_StagedItem.GetHierarchyParent() == m_StagedDev;
        string where = "fehlt";
        if (m_StagedItem)
            where = "liegt daneben";
        if (inside)
            where = "im Gefaess";
        ChefZ_AT_Log.Result("DEVICE", "Zwischenlager " + m_StagedId, inside, where);
        if (m_StagedItem)
            g_Game.ObjectDelete(m_StagedItem);
        if (m_StagedDev)
            g_Game.ObjectDelete(m_StagedDev);
    }

    //! Faelle mit Protokoll ueber alle Takte statt nur den ersten.
    bool IsFocus(string id)
    {
        // Liste statt ||-Kette: eine lange Kette bricht den Compiler ("Formula too complex").
        array<string> focus = {"REC_ChefZ_Bread", "RCP_ChefZ_HunterStew", "RCP_ChefZ_TomatoSauce", "RCP_ChefZ_TacticalBreakfast", "RCP_ChefZ_CreamSauce", "RCP_ChefZ_GrilledStuffedCalamari", "RCP_ChefZ_MeatDumplings", "RCP_ChefZ_BoneBrothSoup", "RCP_ChefZ_MushroomPan", "RCP_ChefZ_PotatoPancakes", "RCP_ChefZ_CheeseFlatbread", "RCP_ChefZ_BoneBroth", "RCP_ChefZ_ChernarusChiliGroup", "RCP_ChefZ_ScrambledEggSausage", "RCP_ChefZ_PumpkinSoup", "RCP_ChefZ_SmallFishPan", "RCP_ChefZ_CrispyFriedCalamari", "RCP_ChefZ_BoneBrothSoupGroup"};
        return focus.Find(id) >= 0;
    }

    string CookDiag(ItemBase dev)
    {
        ChefZ_CookingDeviceAdapter ad = ChefZ_CookingDeviceAdapter.Get();
        ChefZ_DeviceDescriptor desc;
        int cargo;
        bool gate = ad.PassesGate(dev, desc, cargo);
        string s = "gate=" + gate.ToString() + " cargo=" + cargo.ToString() + " fluessig=" + dev.GetQuantity().ToString() + "/" + dev.GetLiquidType().ToString() + " temp=" + dev.GetTemperature().ToString();
        ChefZ_DeviceDescriptor found;
        s = s + " device=" + ad.ResolveDevice(dev, found).ToString();
        Cooking probe = new Cooking();
        s = s + " method=" + probe.ChefZ_QueryMethod(dev).ToString();
        ChefZ_CookSession session = ad.PeekSession(dev);
        if (!session)
            return s + " session=none";
        s = s + " state=" + typename.EnumToString(ChefZ_ESessionState, session.state) + " elapsed=" + session.elapsedSec.ToString() + " fails=" + session.failCount.ToString();
        if (session.outcome)
            s = s + " outcome=" + session.outcome.ToDebugString();
        return s;
    }

    string DescribeItems(array<EntityAI> items)
    {
        string s = "";
        foreach (EntityAI e : items)
        {
            string stage = "";
            Edible_Base ed = Edible_Base.Cast(e);
            if (ed && ed.HasFoodStage())
                stage = "/" + typename.EnumToString(FoodStageType, ed.GetFoodStageType());
            s = s + e.GetType() + stage + " ";
        }
        return s;
    }

    ChefZ_CompiledProcess ProcessOf(string id)
    {
        ChefZ_Sym sym = ChefZ_SymbolTable.Lookup(id);
        if (!ChefZ_SymbolTable.IsValid(sym))
            return null;
        return ChefZ_ProcessingManager.Get().GetProcess(sym);
    }

    //! Werkzeug in die Hand. Liefert das Item oder null (kein Werkzeug noetig).
    ItemBase ToolInHands(ChefZ_CompiledProcess proc)
    {
        EmptyHands();
        array<string> tools = new array<string>();
        ChefZ_HandcraftBridge.CollectToolClasses(proc, tools);
        if (tools.Count() == 0)
            return null;
        foreach (string t : tools)
        {
            EntityAI e = m_Player.GetHumanInventory().CreateInHands(t);
            if (e)
                return ItemBase.Cast(e);
        }
        return null;
    }

    void EmptyHands()
    {
        EntityAI h = m_Player.GetHumanInventory().GetEntityInHands();
        if (h)
            g_Game.ObjectDelete(h);
    }

    //! Startet einen PROC-Fall. Rueckgabe: ms bis zur Pruefung, -1 = schon bewertet.
    int RunProcStart(ChefZ_AT_Case c)
    {
        ChefZ_CompiledProcess proc = ProcessOf(c.process);
        if (!proc)
        {
            ChefZ_AT_Log.Result("PROC", c.id, false, "Prozess " + c.process + " unbekannt");
            return -1;
        }
        m_Pending = c;
        m_PendingStation = null;

        if (proc.exec == ChefZ_ProcessExec.HANDCRAFT)
            return RunHandcraft(c, proc);

        ItemBase st = ChefZ_AT_Util.SpawnGround(c.device, m_Site);
        ChefZ_ProcessingStation_Base station = ChefZ_ProcessingStation_Base.Cast(st);
        if (!station)
        {
            ChefZ_AT_Log.Result("PROC", c.id, false, "Station " + c.device + " fehlt oder ist keine Station");
            if (st)
                g_Game.ObjectDelete(st);
            return -1;
        }
        m_PendingStation = station;
        string err = ChefZ_AT_Util.FillCargo(station, c);
        if (err != "")
        {
            ChefZ_AT_Log.Result("PROC", c.id, false, err);
            return -1;
        }
        ItemBase tool = ToolInHands(proc);

        // Der Raeucherofen braucht Feuer.
        ChefZ_Smoker smoker = ChefZ_Smoker.Cast(station);
        if (smoker)
        {
            smoker.GetInventory().CreateAttachment("Firewood");
            smoker.OnIgnitedThis(null);
        }
        else if (proc.requiresHeat)
        {
            // Die Salzpfanne sucht ein brennendes Feuer im Umkreis.
            FireplaceBase fire = FireplaceBase.Cast(ChefZ_AT_Util.SpawnGround("Fireplace", m_Site + Vector(1, 0, 0)));
            if (fire)
            {
                fire.GetInventory().CreateAttachment("Firewood");
                fire.GetInventory().CreateAttachment("Rag");
                fire.StartFire(true);
                m_PendingFire = fire;
            }
        }
        return 500;
    }

    int RunHandcraft(ChefZ_AT_Case c, ChefZ_CompiledProcess proc)
    {
        ItemBase tool = ToolInHands(proc);
        ItemBase a = tool;
        ItemBase b = null;
        foreach (ChefZ_AT_Input input : c.inputs)
        {
            ItemBase it;
            if (!a)
            {
                it = ItemBase.Cast(m_Player.GetHumanInventory().CreateInHands(input.cls));
                a = it;
            }
            else
            {
                it = ItemBase.Cast(m_Player.GetInventory().CreateInInventory(input.cls));
                if (!it)
                    it = ChefZ_AT_Util.SpawnGround(input.cls, m_Player.GetPosition() + Vector(0.3, 0, 0.3));
                if (!b)
                    b = it;
            }
            ChefZ_AT_Util.Prepare(it, input, c.liquid, c.liquidQty);
        }
        if (!a || !b)
        {
            ChefZ_AT_Log.Result("PROC", c.id, false, "Handarbeit braucht zwei Items, hat Hand=" + ItemName(a) + " Ziel=" + ItemName(b));
            return -1;
        }

        PluginRecipesManager pm = PluginRecipesManager.Cast(GetPlugin(PluginRecipesManager));
        array<int> ids = new array<int>();
        int baseIdx = ChefZ_HandcraftBridge.GetBaseIndex();
        int last = baseIdx + ChefZ_HandcraftBridge.GetSlotCount();

        // Gegenprobe: ohne das Werkzeug (ein Lappen an seiner Stelle) darf
        // kein Rezeptplatz dieses Transforms greifen.
        m_PendingTool = tool;
        m_PendingToolNote = "";
        if (tool)
        {
            m_PendingToolHp = tool.GetHealth("", "");
            ItemBase stand = ItemBase.Cast(m_Player.GetInventory().CreateInInventory("Rag"));
            bool without = false;
            for (int n = baseIdx; n < last; n++)
            {
                ChefZ_GenericCraftRecipe probe = ChefZ_GenericCraftRecipe.Cast(pm.m_RecipeList[n]);
                if (probe && probe.ChefZ_AT_TransformId() == c.id && stand && probe.ChefZ_AT_Accepts(stand, b, m_Player))
                    without = true;
            }
            if (stand)
                g_Game.ObjectDelete(stand);
            if (!stand)
            {
                m_PendingToolNote = " ohne-Werkzeug=ungeprueft";
            }
            else if (without)
            {
                ChefZ_AT_Log.Result("PROC", c.id, false, "geht auch ohne Werkzeug " + tool.GetType() + " (Lappen + " + ItemName(b) + ")");
                return -1;
            }
            else
            {
                m_PendingToolNote = " ohne-Werkzeug=abgelehnt";
            }
        }

        pm.GetValidRecipes(a, b, ids, m_Player);
        foreach (int id : ids)
        {
            if (id >= baseIdx && id < last)
            {
                // Mehrere Handrezepte koennen dieselben zwei Items nehmen
                // (Teller und Schuessel aus Holz plus Messer): genau das des Falls.
                ChefZ_GenericCraftRecipe own = ChefZ_GenericCraftRecipe.Cast(pm.m_RecipeList[id]);
                if (!own || own.ChefZ_AT_TransformId() != c.id)
                    continue;
                pm.PerformRecipeServer(id, a, b, m_Player);
                return 1500;
            }
        }
        string why = "";
        for (int r = baseIdx; r < last; r++)
        {
            ChefZ_GenericCraftRecipe slot = ChefZ_GenericCraftRecipe.Cast(pm.m_RecipeList[r]);
            if (slot && slot.ChefZ_AT_TransformId() == c.id)
                why = why + slot.ChefZ_AT_Explain(a, b, m_Player) + "; ";
        }
        if (why == "")
            why = "kein Rezeptplatz traegt diesen Transform";
        ChefZ_AT_Log.Result("PROC", c.id, false, "kein ChefZ-Handrezept fuer " + ItemName(a) + " + " + ItemName(b) + " (gueltig: " + ids.Count().ToString() + ") hand=" + ItemName(m_Player.GetItemInHands()) + " " + why);
        return -1;
    }

    string ItemName(EntityAI e)
    {
        if (!e)
            return "null";
        return e.GetType();
    }

    void CheckPending()
    {
        ChefZ_AT_Case c = m_Pending;
        if (!c)
        {
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextCase, 100, false);
            return;
        }

        array<EntityAI> items = new array<EntityAI>();
        string missing;
        EntityAI made;
        string how = "Handarbeit";

        ChefZ_ProcessingStation_Base station = ChefZ_ProcessingStation_Base.Cast(m_PendingStation);
        if (station)
        {
            ChefZ_CompiledProcess proc = ProcessOf(c.process);
            ChefZ_Sym sym = ChefZ_SymbolTable.Lookup(c.process);
            ItemBase tool = ItemBase.Cast(m_Player.GetHumanInventory().GetEntityInHands());
            string err = "";
            if (proc.exec == ChefZ_ProcessExec.STATION_TIMED)
            {
                how = "Auftrag";
                if (station.ChefZ_ActiveJobCount() == 0 && !station.ChefZ_BeginJob(sym, tool, 0, err))
                    how = how + " (BeginJob: " + err + ")";
                for (int t = 0; t < 400; t++)
                {
                    station.ChefZ_TickJobs(15.0);
                    items.Clear();
                    ChefZ_AT_Util.CollectCargo(station, items);
                    if (ChefZ_AT_Util.HasOutputs(items, c.outputs, missing, made))
                        break;
                }
            }
            else
            {
                how = "Aktion";
                RunImmediate(station, tool, sym, err);
                if (err != "")
                    how = how + " (" + err + ")";
            }
        }

        items.Clear();
        if (station)
            ChefZ_AT_Util.CollectCargo(station, items);
        ChefZ_AT_Util.CollectCargo(m_Player, items);
        ChefZ_AT_Util.CollectNear(m_Site, 8, items);
        ChefZ_AT_Util.HasOutputs(items, c.outputs, missing, made);

        if (m_PendingTool)
            how = how + " werkzeug=" + m_PendingTool.GetType() + " hp " + m_PendingToolHp.ToString() + "->" + m_PendingTool.GetHealth("", "").ToString() + m_PendingToolNote;
        m_PendingTool = null;
        if (made)
            ChefZ_AT_Log.Result("PROC", c.id, true, how + " out=" + made.GetType() + " inputs=" + c.DescribeInputs());
        else
            ChefZ_AT_Log.Result("PROC", c.id, false, how + " fehlt " + missing + "inputs=" + c.DescribeInputs() + " station=" + c.device + " sieht: " + DescribeItems(items));

        EmptyHands();
        ClearPlayerCargo();
        if (m_PendingStation)
            g_Game.ObjectDelete(m_PendingStation);
        m_PendingStation = null;
        if (m_PendingFire)
            g_Game.ObjectDelete(m_PendingFire);
        m_PendingFire = null;
        m_Pending = null;
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextCase, 300, false);
    }

    //! Wie ChefZ_ActionProcessAtStation.RunImmediate, ohne ActionData.
    bool RunImmediate(ChefZ_ProcessingStation_Base station, ItemBase inHands, ChefZ_Sym process, out string err)
    {
        err = "";
        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        station.ChefZ_BuildContext(inHands, 0, ctx);
        ChefZ_FactSnapshot snapshot;
        array<ItemBase> entities;
        ChefZ_FactCollector.CollectFromStation(station, snapshot, entities);
        snapshot.SortStable();
        ChefZ_TransformMatch match;
        if (!ChefZ_ProcessingManager.Get().FindTransform(process, ctx, snapshot, null, match))
        {
            err = "kein Transform: " + match.failReason;
            return false;
        }
        array<ItemBase> created;
        return ChefZ_ProcessRunner.Run(station, match, entities, snapshot, 0, created, err);
    }

    void ClearPlayerCargo()
    {
        array<EntityAI> items = new array<EntityAI>();
        m_Player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        foreach (EntityAI e : items)
        {
            if (e == m_Player)
                continue;
            if (e.IsClothing())
                continue;
            g_Game.ObjectDelete(e);
        }
    }

    //==========================================================================
    // DEVICE
    //==========================================================================

    void RunDevices()
    {
        ChefZ_AT_Util.ClearArea(m_Site, 6);
        // Aufstellbare Geraete brauchen Kollision; Wildpflanzen und die Pfanne nicht.
        array<string> placeable = {"ChefZ_GrainMill", "ChefZ_DryingRack", "ChefZ_ButterChurn", "ChefZ_CheesePress", "ChefZ_MeatGrinder", "ChefZ_Smoker", "ChefZ_HoneyExtractor", "ChefZ_Beehive", "ChefZ_BeehiveDouble"};
        array<string> stations = {"ChefZ_GrainMill", "ChefZ_DryingRack", "ChefZ_ButterChurn", "ChefZ_CheesePress", "ChefZ_MeatGrinder", "ChefZ_Smoker", "ChefZ_HoneyExtractor", "ChefZ_Beehive", "ChefZ_BeehiveDouble", "ChefZ_Mortar", "ChefZ_FryingPan", "ChefZ_WildCorn", "ChefZ_WildThyme", "ChefZ_WildRosemary", "ChefZ_WildParsley", "ChefZ_WildBlackberry", "ChefZ_WildBlueberry", "ChefZ_WildRaspberry", "ChefZ_WildStrawberry"};
        foreach (string s : stations)
        {
            Object o = g_Game.CreateObjectEx(s, m_Site, ECE_PLACE_ON_SURFACE);
            if (!o)
            {
                ChefZ_AT_Log.Result("DEVICE", s, false, "nicht spawnbar");
                continue;
            }
            if (placeable.Find(s) >= 0)
            {
                string layer;
                g_Game.ConfigGetText("CfgVehicles " + s + " physLayer", layer);
                vector box[2];
                bool hasBox = o.GetCollisionBox(box);
                bool ok = layer == "item_large" && hasBox;
                ChefZ_AT_Log.Result("DEVICE", s + ".collision", ok, "physLayer=" + layer + " box=" + hasBox.ToString() + " min=" + box[0].ToString() + " max=" + box[1].ToString());
            }
            ChefZ_ProcessingStation_Base st = ChefZ_ProcessingStation_Base.Cast(o);
            if (st)
            {
                array<ChefZ_Sym> procs = new array<ChefZ_Sym>();
                st.ChefZ_GetSupportedProcesses(procs);
                ChefZ_AT_Log.Result("DEVICE", s + ".processes", procs.Count() > 0, "anzahl=" + procs.Count().ToString() + " slots=" + st.ChefZ_GetSlotCount().ToString());
            }
            g_Game.ObjectDelete(o);
        }
        TestSmokerHeat();
        TestHoneyAutostart();
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RunFishing, 1000, false);
    }

    void TestSmokerHeat()
    {
        ChefZ_Smoker smoker = ChefZ_Smoker.Cast(g_Game.CreateObjectEx("ChefZ_Smoker", m_Site, ECE_PLACE_ON_SURFACE));
        if (!smoker)
        {
            ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.heat", false, "nicht spawnbar");
            return;
        }
        EntityAI wood = smoker.GetInventory().CreateAttachment("Firewood");
        EntityAI meat = smoker.GetInventory().CreateEntityInCargo("PigSteakMeat");
        ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.fuelslot", wood != null, "Firewood als Attachment");
        float t0 = 0;
        if (meat)
            t0 = meat.GetTemperature();
        smoker.OnIgnitedThis(null);
        for (int i = 0; i < 40; i++)
            smoker.ChefZ_TickJobs(15.0);
        float t1 = 0;
        if (meat)
            t1 = meat.GetTemperature();
        ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.lit", smoker.ChefZ_HasHeat(), "jobs=" + smoker.ChefZ_ActiveJobCount().ToString());
        array<EntityAI> items = new array<EntityAI>();
        ChefZ_AT_Util.CollectCargo(smoker, items);
        string missing;
        EntityAI made;
        array<string> want = {"ChefZ_SmokedMeat"};
        ChefZ_AT_Util.HasOutputs(items, want, missing, made);
        ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.smokes", made != null, "Inhalt: " + DescribeItems(items));
        ChefZ_AT_Log.Info("smoker meat temperature " + t0.ToString() + " -> " + t1.ToString());
        g_Game.ObjectDelete(smoker);
    }

    void TestHoneyAutostart()
    {
        ChefZ_HoneyExtractor ex = ChefZ_HoneyExtractor.Cast(g_Game.CreateObjectEx("ChefZ_HoneyExtractor", m_Site, ECE_PLACE_ON_SURFACE));
        if (!ex)
        {
            ChefZ_AT_Log.Result("DEVICE", "ChefZ_HoneyExtractor.autostart", false, "nicht spawnbar");
            return;
        }
        m_PendingStation = ex;
        ex.GetInventory().CreateEntityInCargo("ChefZ_HoneycombFrameUncapped");
        ex.GetInventory().CreateEntityInCargo("ChefZ_EmptyJar");
        ex.GetInventory().CreateEntityInCargo("ChefZ_EmptyJar");
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckHoneyAutostart, 1500, false);
    }

    void CheckHoneyAutostart()
    {
        ChefZ_HoneyExtractor ex = ChefZ_HoneyExtractor.Cast(m_PendingStation);
        if (!ex)
            return;
        int jobs = ex.ChefZ_ActiveJobCount();
        for (int i = 0; i < 200; i++)
            ex.ChefZ_TickJobs(15.0);
        array<EntityAI> items = new array<EntityAI>();
        ChefZ_AT_Util.CollectCargo(ex, items);
        int honey = 0;
        foreach (EntityAI e : items)
        {
            if (e.GetType() == "Honey")
                honey++;
        }
        ChefZ_AT_Log.Result("DEVICE", "ChefZ_HoneyExtractor.autostart", jobs > 0 && honey >= 1, "jobsNachEinlegen=" + jobs.ToString() + " honig=" + honey.ToString() + " Inhalt: " + DescribeItems(items));
        g_Game.ObjectDelete(ex);
        m_PendingStation = null;
    }

    //==========================================================================
    // FISH
    //==========================================================================

    void RunFishing()
    {
        ChefZ_AT_Fishing.Run(m_Player, m_Site);
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RunMeal, 1000, false);
    }

    //==========================================================================
    // MEAL
    //==========================================================================

    void RunMeal()
    {
        EmptyHands();
        Edible_Base dish = Edible_Base.Cast(m_Player.GetInventory().CreateInInventory("ChefZ_FishPotatoPlate"));
        if (!dish)
        {
            ChefZ_AT_Log.Result("MEAL", "bonus", false, "Teller nicht spawnbar");
            Finish();
            return;
        }
        ChefZ_QualityManager qm = ChefZ_QualityManager.Get();
        ChefZ_Sym premium = ChefZ_SymbolTable.Lookup("PREMIUM");
        ChefZ_ItemStateComponent comp = ChefZ_ItemStateComponent.Of(dish);
        bool tierSet = false;
        if (comp && ChefZ_SymbolTable.IsValid(premium))
            tierSet = ChefZ_ItemStateComponent.SetQuality(dish, premium);
        m_Player.SetAllowDamage(true);
        m_Player.SetHealth("", "Blood", 3000);
        m_Player.SetHealth("", "Health", 50);
        float blood0 = m_Player.GetHealth("", "Blood");
        float health0 = m_Player.GetHealth("", "Health");
        dish.Consume(dish.GetQuantityMax() * 0.5, m_Player);
        float blood1 = m_Player.GetHealth("", "Blood");
        float health1 = m_Player.GetHealth("", "Health");
        m_Player.SetHealth("", "Blood", 5000);
        m_Player.SetHealth("", "Health", 100);
        m_Player.SetAllowDamage(false);
        string info = "served=" + dish.IsInherited(ChefZ_ServedDish_Base).ToString() + " qtyMax=" + dish.GetQuantityMax().ToString();
        ChefZ_QualityTierDef tdef = qm.GetDef(premium);
        if (tdef)
            info = info + " rank=" + tdef.rank.ToString();
        array<string> effects;
        qm.GetGrantedEffects(premium, effects);
        info = info + " effekte=" + effects.Count().ToString();
        ChefZ_AT_Log.Result("MEAL", "premium.bonus", tierSet && blood1 > blood0 && health1 > health0, "stufe=" + ChefZ_AT_Util.TierOf(dish) + " blut " + blood0.ToString() + "->" + blood1.ToString() + " gesundheit " + health0.ToString() + "->" + health1.ToString() + " " + info);
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckImmunity, 3000, false);
    }

    //! ActivateModifier wirkt erst im naechsten Modifier-Takt.
    void CheckImmunity()
    {
        bool immune = m_Player.GetModifiersManager().IsModifierActive(eModifiers.MDF_IMMUNITYBOOST);
        ChefZ_AT_Log.Result("MEAL", "premium.immunity", immune, "Immunschub nach PREMIUM-Gericht");
        Finish();
    }

    //==========================================================================

    void Finish()
    {
        // Ereignisbus: das Kochbuch abonniert RECIPE_COMPLETED. Nach den Kochfaellen
        // muss mindestens eine Zustellung stehen und keine Anmeldung unrufbar sein.
        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        string busInfo = "anmeldungen=" + bus.GetSubscriptionCount().ToString() + " ausgeloest=" + bus.GetRaisedCount().ToString() + " zugestellt=" + bus.GetDeliveredCount().ToString() + " unrufbar=" + bus.GetSkipUncallableCount().ToString();
        bool busOk = bus.GetDeliveredCount() > 0 && bus.GetSkipUncallableCount() == 0;
        ChefZ_AT_Log.Result("DEVICE", "EventBus.Kochbuch", busOk, busInfo);
        ChefZ_AT_Log.Summary();
        if (m_Player && m_Player.GetIdentity())
            g_Game.RPCSingleParam(m_Player, ChefZ_AT_Rpc.QUIT, new Param1<int>(0), true, m_Player.GetIdentity());
        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Quit, 8000, false);
    }

    void Quit()
    {
        g_Game.RequestExit(0);
    }
}

modded class MissionServer
{
    override void OnMissionStart()
    {
        super.OnMissionStart();
        if (FileExist("$mission:chefz_autotest.txt"))
            ChefZ_AT_Runner.Start();
    }

    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        if (ChefZ_AT_Runner.s_Instance)
            ChefZ_AT_Runner.s_Instance.SetPlayer(player);
    }
}
