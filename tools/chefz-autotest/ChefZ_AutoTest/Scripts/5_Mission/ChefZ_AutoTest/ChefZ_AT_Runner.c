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
    string m_PendingHow;
    ref map<string, float> m_PendingBefore;

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
        m_PendingBefore = InputAmounts(c, CollectAround());

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
                m_PendingBefore = InputAmounts(c, CollectAround());
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

        // Ausgewertet wird einen Frame spaeter: verbrauchte Zutaten loescht die
        // Engine erst am Ende des Frames, vorher stuenden sie noch im Laderaum.
        m_PendingHow = how;
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(FinishPending, 200, false);
    }

    //! Alles um den Fall herum: Station, Spieler, Boden.
    array<EntityAI> CollectAround()
    {
        array<EntityAI> items = new array<EntityAI>();
        if (m_PendingStation)
            ChefZ_AT_Util.CollectCargo(m_PendingStation, items);
        ChefZ_AT_Util.CollectCargo(m_Player, items);
        ChefZ_AT_Util.CollectNear(m_Site, 8, items);
        return items;
    }

    //! Menge je Eingangsklasse: Quantity, bei Items ohne Menge die Stueckzahl.
    //! Klassen, die auch Ausgang sind, zaehlen nicht - dort hiesse "weniger" nichts.
    map<string, float> InputAmounts(ChefZ_AT_Case c, array<EntityAI> items)
    {
        map<string, float> amounts = new map<string, float>();
        foreach (ChefZ_AT_Input input : c.inputs)
        {
            if (c.outputs.Find(input.cls) >= 0)
                continue;
            float sum = 0;
            foreach (EntityAI e : items)
            {
                if (!e || e.GetType() != input.cls || e.IsSetForDeletion())
                    continue;
                ItemBase ib = ItemBase.Cast(e);
                if (ib && ib.HasQuantity())
                    sum = sum + ib.GetQuantity();
                else
                    sum = sum + 1;
            }
            amounts.Set(input.cls, sum);
        }
        return amounts;
    }

    void FinishPending()
    {
        ChefZ_AT_Case c = m_Pending;
        if (!c)
        {
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextCase, 100, false);
            return;
        }
        string how = m_PendingHow;
        string missing;
        EntityAI made;
        array<EntityAI> items = CollectAround();
        ChefZ_AT_Util.HasOutputs(items, c.outputs, missing, made);

        // Die Zutaten muessen weniger geworden sein (Fleischwolf: das Fleisch
        // verschwindet und wird zu Hackfleisch).
        map<string, float> after = InputAmounts(c, items);
        string used = "";
        bool consumed = true;
        foreach (string cls, float was : m_PendingBefore)
        {
            float now = after.Get(cls);
            used = used + " " + cls + " " + was.ToString() + "->" + now.ToString();
            if (now >= was)
                consumed = false;
        }
        how = how + " verbraucht:" + used;
        if (made && !consumed)
            made = null;
        if (!made && missing == "")
            missing = "Zutaten nicht verbraucht ";

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
        // Jedes Geraet auf eigenem Platz: geloescht wird erst am Frameende, und
        // der Kollisionsstrahl traefe sonst das vorige Geraet.
        int spot = 0;
        foreach (string s : stations)
        {
            spot++;
            Object o = g_Game.CreateObjectEx(s, m_Site + Vector(0, 0, 3 * spot), ECE_PLACE_ON_SURFACE);
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
                // Physikstrahl auf der Ebene, auf der auch Zelte, Zaeune und die
                // Feuerstelle den Spieler aufhalten: waagerecht auf halber Hoehe
                // quer durch das Geraet. Er muss das Geraet selbst treffen.
                vector mid = o.GetPosition() + Vector(0, (box[0][1] + box[1][1]) * 0.5, 0);
                vector from = mid + Vector(2, 0, 0);
                vector to = mid - Vector(2, 0, 0);
                Object hitObj;
                vector hitPos;
                vector hitNormal;
                float hitFraction;
                bool rayHit = DayZPhysics.RayCastBullet(from, to, PhxInteractionLayers.ITEM_LARGE, null, hitObj, hitPos, hitNormal, hitFraction);
                bool hitsSelf = rayHit && hitObj == o;
                bool layerOk = (layer == "item_large");
                bool ok = layerOk && hasBox && hitsSelf;
                string col = "physLayer=" + layer + " box=" + hasBox.ToString() + " strahl=" + hitsSelf.ToString();
                col = col + " traf=" + ItemName(EntityAI.Cast(hitObj)) + " bei " + hitPos.ToString();
                col = col + " pos=" + o.GetPosition().ToString() + " min=" + box[0].ToString() + " max=" + box[1].ToString();
                ChefZ_AT_Log.Result("DEVICE", s + ".collision", ok, col);
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
        // Raeucherschrank und Honigschleuder brauchen echte Zeit (Brenntakt
        // 5 s) und werden einen Frame nach dem letzten Takt bewertet, weil die
        // Engine verbrauchte Zutaten erst am Frameende loescht.
        TestSmokerHeat();
    }

    ChefZ_Smoker m_AtSmoker;
    EntityAI m_AtSmokerMeat;
    float m_AtSmokerT0;
    EntityAI m_AtSmokerHook;
    EntityAI m_AtShowSmoker;
    EntityAI m_AtShowFire;
    float m_AtSmokerHookT0;

    void TestSmokerHeat()
    {
        ChefZ_Smoker smoker = ChefZ_Smoker.Cast(g_Game.CreateObjectEx("ChefZ_Smoker", m_Site, ECE_PLACE_ON_SURFACE));
        if (!smoker)
        {
            ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.heat", false, "nicht spawnbar");
            TestHoneyAutostart();
            return;
        }
        EntityAI wood = smoker.GetInventory().CreateAttachment("Firewood");
        EntityAI meat = smoker.GetInventory().CreateEntityInCargo("PigSteakMeat");
        ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.fuelslot", wood != null, "Firewood als Attachment");
        // Raeucherhaken wie Vanillas Ofen: ein Steak haengt, eine Tomate darf nicht.
        int hookA = InventorySlots.GetSlotIdFromString("SmokingA");
        int hookB = InventorySlots.GetSlotIdFromString("SmokingB");
        m_AtSmokerHook = smoker.GetInventory().CreateAttachmentEx("PigSteakMeat", hookA);
        EntityAI tomato = EntityAI.Cast(g_Game.CreateObjectEx("Tomato", m_Site + Vector(1.5, 0, 0), ECE_PLACE_ON_SURFACE));
        bool tomatoRefused = tomato && !smoker.CanReceiveAttachment(tomato, hookB);
        bool steakTaken = m_AtSmokerHook != null;
        ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.hook", steakTaken && tomatoRefused, "Steak am Haken=" + steakTaken.ToString() + " Tomate abgewiesen=" + tomatoRefused.ToString());
        if (tomato)
            g_Game.ObjectDelete(tomato);
        m_AtSmokerHookT0 = 0;
        if (m_AtSmokerHook)
            m_AtSmokerHookT0 = m_AtSmokerHook.GetTemperature();
        m_AtSmokerT0 = 0;
        if (meat)
            m_AtSmokerT0 = meat.GetTemperature();
        smoker.OnIgnitedThis(null);
        ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.lit", smoker.ChefZ_HasHeat(), "jobs=" + smoker.ChefZ_ActiveJobCount().ToString());
        m_AtSmoker = smoker;
        m_AtSmokerMeat = meat;
        // Drei Brenntakte in Echtzeit, ohne die Auftraege vorzuspulen.
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SmokerWarmCheck, 16000, false);
    }

    //! Das Raeuchergut im brennenden Schrank muss warm werden.
    void SmokerWarmCheck()
    {
        ChefZ_Smoker smoker = m_AtSmoker;
        if (!smoker)
        {
            TestHoneyAutostart();
            return;
        }
        float t1 = -1000;
        string which = "PigSteakMeat";
        array<EntityAI> items = new array<EntityAI>();
        ChefZ_AT_Util.CollectCargo(smoker, items);
        foreach (EntityAI e : items)
        {
            if (e.GetType() == "PigSteakMeat" || e.GetType() == "ChefZ_SmokedMeat")
            {
                t1 = e.GetTemperature();
                which = e.GetType();
            }
        }
        ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.warms", t1 >= m_AtSmokerT0 + 5.0, which + " " + m_AtSmokerT0.ToString() + " -> " + t1.ToString() + " C nach 16 s, Schrank " + smoker.GetTemperature().ToString() + " C, brennt=" + smoker.ChefZ_HasHeat().ToString());
        float hookT1 = -1000;
        EntityAI hung = smoker.FindAttachmentBySlotName("SmokingA");
        if (hung)
            hookT1 = hung.GetTemperature();
        ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.hookWarms", hookT1 >= m_AtSmokerHookT0 + 5.0, "Haken " + m_AtSmokerHookT0.ToString() + " -> " + hookT1.ToString() + " C nach 16 s");
        for (int i = 0; i < 40; i++)
            smoker.ChefZ_TickJobs(15.0);
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SmokerDoneCheck, 300, false);
    }

    //! Das rohe Fleisch ist weg, geraeuchertes liegt im Schrank.
    void SmokerDoneCheck()
    {
        ChefZ_Smoker smoker = m_AtSmoker;
        if (smoker)
        {
            array<EntityAI> items = new array<EntityAI>();
            ChefZ_AT_Util.CollectCargo(smoker, items);
            int raw = 0;
            int smoked = 0;
            foreach (EntityAI e : items)
            {
                if (e.IsSetForDeletion())
                    continue;
                if (e.GetType() == "PigSteakMeat")
                    raw++;
                if (e.GetType() == "ChefZ_SmokedMeat")
                    smoked++;
            }
            EntityAI stillHung = smoker.FindAttachmentBySlotName("SmokingA");
            bool hookEmpty = !stillHung || stillHung.IsSetForDeletion();
            // Einzeln gerechnet: in einer Zeile "a == 0 && b == 2 && flag" lieferte
            // Enforce true bei b == 1 (Autotest 26.10.2026).
            bool rawOk = (raw == 0);
            bool countOk = (smoked == 2);
            bool smokedOk = rawOk && countOk && hookEmpty;
            ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.smokes", smokedOk, "roh=" + raw.ToString() + " geraeuchert=" + smoked.ToString() + " (Cargo-Steak + Haken-Steak) Haken leer=" + hookEmpty.ToString() + " Inhalt: " + DescribeItems(items));
            g_Game.ObjectDelete(smoker);
        }
        m_AtSmoker = null;
        m_AtSmokerMeat = null;
        m_AtSmokerHook = null;
        TestHoneyAutostart();
    }

    void TestHoneyAutostart()
    {
        ChefZ_HoneyExtractor ex = ChefZ_HoneyExtractor.Cast(g_Game.CreateObjectEx("ChefZ_HoneyExtractor", m_Site, ECE_PLACE_ON_SURFACE));
        if (!ex)
        {
            ChefZ_AT_Log.Result("DEVICE", "ChefZ_HoneyExtractor.autostart", false, "nicht spawnbar");
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RunFishing, 1000, false);
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
        {
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RunFishing, 1000, false);
            return;
        }
        int jobs = ex.ChefZ_ActiveJobCount();
        ChefZ_AT_Log.Result("DEVICE", "ChefZ_HoneyExtractor.autostart", jobs > 0, "jobsNachEinlegen=" + jobs.ToString());
        m_AtHoneyRounds = 0;
        HoneyRound();
    }

    int m_AtHoneyRounds;

    //! Ein Schleudergang je Runde. Den naechsten startet die Schleuder selbst
    //! im Folgeframe (ChefZ_SpinNextJar) - genau das wird hier mitgeprueft.
    void HoneyRound()
    {
        ChefZ_HoneyExtractor ex = ChefZ_HoneyExtractor.Cast(m_PendingStation);
        if (!ex)
        {
            CheckHoneyJars();
            return;
        }
        for (int i = 0; i < 200; i++)
            ex.ChefZ_TickJobs(15.0);
        m_AtHoneyRounds++;
        if (m_AtHoneyRounds < 4)
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(HoneyRound, 500, false);
        else
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckHoneyJars, 300, false);
    }

    //! Eingesetzte Glaeser werden befuellt: zwei leere Glaeser rein, zwei
    //! Honigglaeser raus, kein leeres Glas bleibt uebrig.
    void CheckHoneyJars()
    {
        ChefZ_HoneyExtractor ex = ChefZ_HoneyExtractor.Cast(m_PendingStation);
        if (ex)
        {
            array<EntityAI> items = new array<EntityAI>();
            ChefZ_AT_Util.CollectCargo(ex, items);
            int honey = 0;
            int empty = 0;
            foreach (EntityAI e : items)
            {
                if (e.IsSetForDeletion())
                    continue;
                if (e.GetType() == "Honey")
                    honey++;
                if (e.GetType() == "ChefZ_EmptyJar")
                    empty++;
            }
            bool honeyOk = (honey == 2);
            bool emptyOk = (empty == 0);
            ChefZ_AT_Log.Result("DEVICE", "ChefZ_HoneyExtractor.fillsJars", honeyOk && emptyOk, "leer=" + empty.ToString() + " voll=" + honey.ToString() + " Inhalt: " + DescribeItems(items));
            g_Game.ObjectDelete(ex);
        }
        m_PendingStation = null;
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RunFishing, 1000, false);
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
        UiShowcase();
    }

    //! Sichtpruefung: ein brennender Raeucherschrank vor dem Spieler, der Client
    //! oeffnet das Inventar (ChefZ_AT_Quit.c) und meldet die Feuerplaetze; das
    //! Bildschirmfoto macht der Testlauf von aussen.
    void UiShowcase()
    {
        // Innerhalb von VICINITY_DISTANCE 0.5 (VicinityItemManager.c:4), sonst haengt
        // die Anzeige an der Blickrichtung der Clientkamera.
        // Erst raeumen, dann zwei Sekunden warten: loescht der Server etwas, das
        // der Client gerade im Umgebungsinventar zeigt, und das Inventar oeffnet
        // im selben Moment, greift Vanillas AttachmentCategoriesRow.RefreshSlot
        // ins Leere (Client-Log 26.10.2026).
        ChefZ_AT_Util.ClearArea(m_Player.GetPosition(), 3);
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(UiShowcasePlace, 2000, false);
    }

    void UiShowcasePlace()
    {
        vector at = m_Player.GetPosition() + m_Player.GetDirection() * 0.45;
        ChefZ_Smoker smoker = ChefZ_Smoker.Cast(g_Game.CreateObjectEx("ChefZ_Smoker", at, ECE_PLACE_ON_SURFACE));
        if (!smoker || !m_Player.GetIdentity())
        {
            ChefZ_AT_Log.Result("DEVICE", "ChefZ_Smoker.ui", false, "kein Schrank oder kein Client");
            Finish();
            return;
        }
        smoker.GetInventory().CreateAttachment("Firewood");
        smoker.GetInventory().CreateAttachment("Rag");
        smoker.GetInventory().CreateEntityInCargo("PigSteakMeat");
        smoker.OnIgnitedThis(null);
        m_AtShowSmoker = smoker;
        // Vergleich: Vanillas Feuerstelle daneben, gleiche Inventar-Umgebung
        vector firePos = m_Player.GetPosition() + m_Player.GetDirectionAside() * 0.45;
        firePos[1] = g_Game.SurfaceY(firePos[0], firePos[2]);
        EntityAI fire = EntityAI.Cast(g_Game.CreateObjectEx("Fireplace", firePos, ECE_CREATEPHYSICS));
        if (fire)
            fire.GetInventory().CreateAttachment("Firewood");
        m_AtShowFire = fire;
        g_Game.RPCSingleParam(m_Player, ChefZ_AT_Rpc.SHOW_INVENTORY, new Param1<int>(0), true, m_Player.GetIdentity());
        Print("[CHEFZTEST] INFO ui-showcase bereit brennt=" + smoker.ChefZ_HasHeat().ToString());
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(UiShowcaseEnd, 25000, false);
    }

    //! Nichts von der Sichtpruefung bleibt in der Welt (die Persistenz haette es
    //! sonst beim naechsten Lauf wieder vor den Spieler gestellt).
    //! Einzeln und eine Sekunde auseinander: verschwinden zwei Behaelter mit
    //! Inventarkategorien im selben Umgebungstakt (0.25 s), frischt Vanillas
    //! VicinityContainer beim Entfernen des ersten den zweiten schon ohne Entity
    //! auf (AttachmentCategoriesRow.RefreshSlot, Client-Log 26.10.2026).
    void UiShowcaseEnd()
    {
        if (m_AtShowFire)
            g_Game.ObjectDelete(m_AtShowFire);
        m_AtShowFire = null;
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(UiShowcaseEndSmoker, 1000, false);
    }

    void UiShowcaseEndSmoker()
    {
        if (m_AtShowSmoker)
            g_Game.ObjectDelete(m_AtShowSmoker);
        m_AtShowSmoker = null;
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
