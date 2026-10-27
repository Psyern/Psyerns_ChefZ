//==============================================================================
// ChefZ_AutoTest - Pruefungen zur Fehlerliste vom 27.10.2026 (Gruppe FIX):
//   - Raeucherschrank hat 12 Haken, Fisch und ChefZ-Wurst haengen daran
//   - nach dem Raeuchern steht der Zustand auf SMOKED (nicht nur der Name)
//   - ein brennender Raeucherschrank laesst sich nicht aufheben
//   - die Lebensmittelbox nimmt Essen, aber keinen Kochtopf
//   - Angel-QTE: der Server loest mit dem Ergebnis auf (Menge, Stufe, kein Fang
//     unter der Mindestwertung)
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_AT_Fixes
{
    //! Haken 5, 6 und 12: ChefZ-Filet, ChefZ-Rohwurst, Vanilla-Filet.
    static const string HOOK_FILLET  = "ChefZ_CodFillet";
    static const string HOOK_SAUSAGE = "ChefZ_RawSausage";
    static const string HOOK_VANILLA = "CarpFilletMeat";

    //==========================================================================
    // Raeucherschrank
    //==========================================================================

    static void SmokerHooks(ChefZ_Smoker smoker)
    {
        // Engine-Liste der Anbauplaetze (dieselbe, aus der das Inventar-UI
        // baut) plus die Config-Liste. HasAttachmentSlot meldete SmokingA-D
        // faelschlich als fehlend, obwohl das UI alle 12 zeigte.
        array<int> engineSlots = new array<int>();
        int slotCount = smoker.GetInventory().GetAttachmentSlotsCount();
        for (int k = 0; k < slotCount; k++)
        {
            engineSlots.Insert(smoker.GetInventory().GetAttachmentSlotId(k));
        }
        TStringArray cfgSlots = new TStringArray();
        g_Game.ConfigGetTextArray("CfgVehicles " + smoker.GetType() + " attachments", cfgSlots);

        string engineNames = "";
        foreach (int sid : engineSlots)
        {
            engineNames = engineNames + InventorySlots.GetSlotName(sid) + " ";
        }

        // Funktional: echte Steaks an SmokingB-D haengen (A traegt schon eins).
        HangOn(smoker, 1, "PigSteakMeat");
        HangOn(smoker, 2, "DeerSteakMeat");
        HangOn(smoker, 3, "BoarSteakMeat");

        // Danach muss jeder Haken belegt sein oder ein Steak annehmen.
        EntityAI probe = EntityAI.Cast(g_Game.CreateObjectEx("PigSteakMeat", smoker.GetPosition() + Vector(1, 0, 0), ECE_PLACE_ON_SURFACE));
        int known = 0;
        string missing = "";
        for (int i = 0; i < ChefZ_Smoker.CHEFZ_HOOK_COUNT; i++)
        {
            string slot = ChefZ_Smoker.ChefZ_HookSlotName(i);
            int id = InventorySlots.GetSlotIdFromString(slot);
            bool inCfg = cfgSlots.Find(slot) >= 0;
            bool occupied = smoker.FindAttachmentBySlotName(slot) != null;
            bool fits = probe && id >= 0 && smoker.GetInventory().CanAddAttachmentEx(probe, id);
            if (inCfg && (occupied || fits))
                known++;
            else
                missing = missing + slot + "(cfg=" + inCfg.ToString() + ",belegt=" + occupied.ToString() + ",passt=" + fits.ToString() + ") ";
        }
        if (probe)
            g_Game.ObjectDelete(probe);
        // B-D wieder leeren: DEVICE ChefZ_Smoker.smokes zaehlt genau 2 Steaks.
        for (int b = 1; b <= 3; b++)
        {
            EntityAI hung = smoker.FindAttachmentBySlotName(ChefZ_Smoker.ChefZ_HookSlotName(b));
            if (hung)
                g_Game.ObjectDelete(hung);
        }
        bool allHooks = (known == ChefZ_Smoker.CHEFZ_HOOK_COUNT);
        ChefZ_AT_Log.Result("FIX", "smoker.18hooks", allHooks, "haken=" + known.ToString() + " plaetze=" + slotCount.ToString() + " fehlt: " + missing + "| engine: " + engineNames);

        HangOn(smoker, 4, HOOK_FILLET);
        HangOn(smoker, 5, HOOK_SAUSAGE);
        HangOn(smoker, 11, HOOK_VANILLA);
    }

    //! Spieltest 27.10.2026 nachmittags: Cargo nur entnehmbar, Keulen und Obst
    //! an die Haken, ueberall Rauchsymbol.
    static void SmokerInputs(ChefZ_Smoker smoker, EntityAI tomato, vector site)
    {
        bool cargoClosed = tomato && !smoker.CanReceiveItemIntoCargo(tomato);
        ChefZ_AT_Log.Result("FIX", "smoker.cargoTakeOnly", cargoClosed, "Tomate in Cargo abgewiesen=" + cargoClosed.ToString());

        SmokerAccepts(smoker, "ChefZ_PorkLeg", site + Vector(2, 0, 0));
        SmokerAccepts(smoker, "ChefZ_BeefLeg", site + Vector(2, 0, 1));
        SmokerAccepts(smoker, "ChefZ_VenisonLeg", site + Vector(2, 0, 2));
        SmokerAccepts(smoker, "Apple", site + Vector(2, 0, 3));
        SmokerAccepts(smoker, "CaninaBerry", site + Vector(2, 0, 4));

        int smokeIcons = 0;
        for (int h = 0; h < ChefZ_Smoker.CHEFZ_HOOK_COUNT; h++)
        {
            string icon = "";
            g_Game.ConfigGetText("CfgSlots Slot_" + ChefZ_Smoker.ChefZ_HookSlotName(h) + " ghostIcon", icon);
            if (icon.Contains("image:smoking"))
                smokeIcons++;
        }
        ChefZ_AT_Log.Result("FIX", "smoker.smokeIcons", smokeIcons == ChefZ_Smoker.CHEFZ_HOOK_COUNT, "rauchsymbol=" + smokeIcons.ToString() + "/" + ChefZ_Smoker.CHEFZ_HOOK_COUNT.ToString());

        int w = 0;
        int hgt = 0;
        TIntArray size = new TIntArray();
        g_Game.ConfigGetIntArray("CfgVehicles ChefZ_Smoker Cargo itemsCargoSize", size);
        if (size.Count() == 2)
        {
            w = size.Get(0);
            hgt = size.Get(1);
        }
        ChefZ_AT_Log.Result("FIX", "smoker.cargo50", w * hgt == 50, "cargo=" + w.ToString() + "x" + hgt.ToString());
    }

    static void SmokerAccepts(ChefZ_Smoker smoker, string cls, vector pos)
    {
        int hookLast = InventorySlots.GetSlotIdFromString(ChefZ_Smoker.ChefZ_HookSlotName(ChefZ_Smoker.CHEFZ_HOOK_COUNT - 2));
        EntityAI e = EntityAI.Cast(g_Game.CreateObjectEx(cls, pos, ECE_PLACE_ON_SURFACE));
        bool made = e != null;
        bool gate = false;
        bool fits = false;
        if (e)
        {
            gate = smoker.CanReceiveAttachment(e, hookLast);
            fits = smoker.GetInventory().CanAddAttachmentEx(e, hookLast);
            g_Game.ObjectDelete(e);
        }
        bool ok = made && gate && fits;
        ChefZ_AT_Log.Result("FIX", "smoker.accepts." + cls, ok, "erzeugt=" + made.ToString() + " tor=" + gate.ToString() + " slot=" + fits.ToString());
    }

    static void HangOn(ChefZ_Smoker smoker, int index, string cls)
    {
        string slot = ChefZ_Smoker.ChefZ_HookSlotName(index);
        int id = InventorySlots.GetSlotIdFromString(slot);
        EntityAI e = smoker.GetInventory().CreateAttachmentEx(cls, id);
        ChefZ_AT_Log.Result("FIX", "smoker.hook." + cls, e != null, slot);
    }

    static void SmokerPickup(ChefZ_Smoker smoker, PlayerBase player)
    {
        bool hands = smoker.CanPutIntoHands(player);
        bool cargo = smoker.CanPutInCargo(player);
        bool blocked = !hands && !cargo;
        ChefZ_AT_Log.Result("FIX", "smoker.noPickupWhileActive", blocked, "aktiv=" + smoker.ChefZ_IsActive().ToString() + " haende=" + hands.ToString() + " inventar=" + cargo.ToString());
    }

    //! Alles Fleisch und aller Fisch im Schrank muss SMOKED sein.
    static void SmokedStates(ChefZ_Smoker smoker)
    {
        ChefZ_Sym smoked = ChefZ_SymbolTable.Lookup("SMOKED");
        array<EntityAI> items = new array<EntityAI>();
        ChefZ_AT_Util.CollectCargo(smoker, items);

        int goods = 0;
        int ok = 0;
        string bad = "";
        string seen = "";
        foreach (EntityAI e : items)
        {
            ItemBase ib = ItemBase.Cast(e);
            if (!ib || ib.IsSetForDeletion())
                continue;
            bool meat = ChefZ_StationGate.ChefZ_InCategory(ib, ChefZ_Smoker.CHEFZ_CAT_MEAT);
            bool fish = ChefZ_StationGate.ChefZ_InCategory(ib, ChefZ_Smoker.CHEFZ_CAT_FISH);
            if (!meat && !fish)
                continue;
            goods++;
            ChefZ_Sym st = ChefZ_ItemStateComponent.GetState(ib);
            string line = ib.GetType() + "=" + ChefZ_SymbolTable.NameOrMark(st);
            seen = seen + line + " ";
            // Ein Vanilla-Item traegt keinen ChefZ-Zustand, nur die Projektion
            // SMOKED -> DRIED (ChefZ_ProcessRunner.ProjectOntoVanillaStage).
            bool vanillaSmoked = !ChefZ_ItemStateComponent.IsManaged(ib) && ChefZ_AT_Util.InStage(ib, "DRIED");
            if (st == smoked || vanillaSmoked)
                ok++;
            else
                bad = bad + line + " ";
        }
        // 2 Steaks (Cargo + Haken A) + Filet, Wurst, Vanilla-Filet an den neuen Haken
        bool countOk = (goods >= 5);
        bool allOk = (ok == goods);
        ChefZ_AT_Log.Result("FIX", "smoker.stateSmoked", countOk && allOk, "geraeuchert=" + ok.ToString() + "/" + goods.ToString() + " falsch: " + bad + "| " + seen);
    }

    //==========================================================================
    // Lebensmittelbox
    //==========================================================================

    static void FoodBox(vector site)
    {
        EntityAI box = EntityAI.Cast(g_Game.CreateObjectEx("ChefZ_EmptyBox", site + Vector(1, 0, 1), ECE_PLACE_ON_SURFACE));
        if (!box)
        {
            ChefZ_AT_Log.Result("FIX", "foodbox.spawn", false, "ChefZ_EmptyBox nicht spawnbar");
            return;
        }
        EntityAI steak = EntityAI.Cast(g_Game.CreateObjectEx("PigSteakMeat", site + Vector(1.5, 0, 1), ECE_PLACE_ON_SURFACE));
        EntityAI apple = EntityAI.Cast(g_Game.CreateObjectEx("Apple", site + Vector(1.5, 0, 1.5), ECE_PLACE_ON_SURFACE));
        EntityAI pot = EntityAI.Cast(g_Game.CreateObjectEx("Pot", site + Vector(2, 0, 1), ECE_PLACE_ON_SURFACE));

        bool steakAccepted = steak && box.CanReceiveItemIntoCargo(steak) && box.CanLoadItemIntoCargo(steak);
        bool potRefused = pot && !box.CanReceiveItemIntoCargo(pot);
        bool steakIn = false;
        bool appleIn = false;
        if (steak)
        {
            box.ServerTakeEntityToCargo(steak);
            steakIn = steak.GetHierarchyParent() == box;
        }
        if (apple)
        {
            box.ServerTakeEntityToCargo(apple);
            appleIn = apple.GetHierarchyParent() == box;
        }
        bool foodOk = steakAccepted && steakIn && appleIn;
        ChefZ_AT_Log.Result("FIX", "foodbox.takesFood", foodOk, "steakErlaubt=" + steakAccepted.ToString() + " steakDrin=" + steakIn.ToString() + " apfelDrin=" + appleIn.ToString());
        ChefZ_AT_Log.Result("FIX", "foodbox.refusesPot", potRefused, "Kochtopf abgewiesen=" + potRefused.ToString());

        if (pot)
            g_Game.ObjectDelete(pot);
        g_Game.ObjectDelete(box);
    }

    //==========================================================================
    // Fleisch-Garstufen-Texturen
    //==========================================================================

    //! Jede Klasse: 5 Texturen, KEINE Materialien (MLOD: Material = unsichtbar,
    //! 27.10.2026), Stufenwechsel ohne Absturz ueber ChefZ_StageVisuals.
    //! (Vanillas FoodStage.UpdateVisualsEx laese die leere Liste per Index.)
    static void MeatTextures(vector site)
    {
        TStringArray classes = {"ChefZ_BeefLeg", "ChefZ_PorkLeg", "ChefZ_VenisonLeg", "ChefZ_DicedMeat", "ChefZ_MincedMeat", "ChefZ_MincedBear", "ChefZ_RawSausage", "ChefZ_RawPorkSausage", "ChefZ_RawVenisonSausage", "ChefZ_RawBoarSausage", "ChefZ_RawHunterSausage", "ChefZ_RawSpicySausage"};
        int good = 0;
        string bad = "";
        foreach (string cls : classes)
        {
            TStringArray tex = new TStringArray();
            TStringArray mat = new TStringArray();
            g_Game.ConfigGetTextArray("CfgVehicles " + cls + " hiddenSelectionsTextures", tex);
            g_Game.ConfigGetTextArray("CfgVehicles " + cls + " hiddenSelectionsMaterials", mat);
            Edible_Base e = Edible_Base.Cast(g_Game.CreateObjectEx(cls, site, ECE_PLACE_ON_SURFACE));
            bool staged = false;
            if (e)
            {
                e.ChangeFoodStage(FoodStageType.BAKED);
                e.ChangeFoodStage(FoodStageType.BOILED);
                e.ChangeFoodStage(FoodStageType.BURNED);
                staged = e.GetFoodStageType() == FoodStageType.BURNED;
                g_Game.ObjectDelete(e);
            }
            bool ok = tex.Count() == 5 && mat.Count() == 0 && staged;
            if (ok)
                good++;
            else
                bad = bad + cls + "(tex=" + tex.Count().ToString() + ",mat=" + mat.Count().ToString() + ",stufe=" + staged.ToString() + ") ";
        }
        bool all = (good == classes.Count());
        ChefZ_AT_Log.Result("FIX", "meat.stageTextures", all, "ok=" + good.ToString() + "/" + classes.Count().ToString() + " " + bad);
    }

    //! Alle ChefZ-Items der Kategorien MEAT/FISH mit Garstufen: jede Stufe
    //! Gebraten/Geraeuchert/Verbrannt muss eine eigene Textur zeigen (nicht die
    //! Rohtextur). Quelle ist die geladene Config samt Vererbung.
    static void StageTextureAudit(vector site)
    {
        string root = "CfgVehicles";
        int n = g_Game.ConfigGetChildrenCount(root);
        TStringArray stages = {"Baked", "Dried", "Burned"};
        int checked = 0;
        int good = 0;
        string bad = "";
        for (int i = 0; i < n; i++)
        {
            string cls;
            g_Game.ConfigGetChildName(root, i, cls);
            if (cls.IndexOf("ChefZ_") != 0)
                continue;
            string path = root + " " + cls;
            if (g_Game.ConfigGetInt(path + " scope") != 2)
                continue;
            if (!g_Game.ConfigIsExisting(path + " Food FoodStages Raw"))
                continue;

            EntityAI e = EntityAI.Cast(g_Game.CreateObjectEx(cls, site, ECE_PLACE_ON_SURFACE));
            if (!e)
                continue;
            bool meat = ChefZ_StationGate.ChefZ_InCategory(e, ChefZ_Smoker.CHEFZ_CAT_MEAT);
            bool fish = ChefZ_StationGate.ChefZ_InCategory(e, ChefZ_Smoker.CHEFZ_CAT_FISH);
            g_Game.ObjectDelete(e);
            if (!meat && !fish)
                continue;

            checked++;
            TStringArray tex = new TStringArray();
            g_Game.ConfigGetTextArray(path + " hiddenSelectionsTextures", tex);
            TIntArray rawVis = new TIntArray();
            g_Game.ConfigGetIntArray(path + " Food FoodStages Raw visual_properties", rawVis);
            string missing = "";
            foreach (string st : stages)
            {
                string sp = path + " Food FoodStages " + st;
                if (!g_Game.ConfigIsExisting(sp))
                    continue;
                TIntArray vis = new TIntArray();
                g_Game.ConfigGetIntArray(sp + " visual_properties", vis);
                // Einzeln gerechnet (Enforce wertet lange &&-Vergleichsketten falsch aus).
                bool has = false;
                bool shapes = vis.Count() >= 2;
                bool rawShape = rawVis.Count() >= 2;
                if (shapes && rawShape)
                {
                    int ti = vis[1];
                    int ri = rawVis[1];
                    bool inRange = ti >= 0;
                    bool below = ti < tex.Count();
                    bool rawRange = ri >= 0;
                    bool rawBelow = ri < tex.Count();
                    if (inRange && below && rawRange && rawBelow)
                    {
                        string own = tex[ti];
                        string rawTex = tex[ri];
                        has = own != rawTex;
                    }
                }
                // Ein Raeucher-/Trockenprodukt IST schon geraeuchert: seine Stufe
                // "Dried" zeigt zu Recht die Grundtextur.
                bool isDried = st == "Dried";
                bool preservedRaw = cls.Contains("Smoked") || cls.Contains("Dried") || cls.Contains("DrySausage");
                if (!has && isDried && preservedRaw)
                    has = true;
                if (!has)
                    missing = missing + st + ",";
            }
            if (missing == "")
                good++;
            else
                bad = bad + cls + "(" + missing + "tex=" + tex.Count().ToString() + ") ";
        }
        bool any = checked > 0;
        bool allGood = (good == checked);
        bool ok = any && allGood;
        ChefZ_AT_Log.Result("FIX", "food.stageTextureAudit", ok, "fleisch+fisch mit Garstufen=" + checked.ToString() + " mitTexturen=" + good.ToString() + " ohne: " + bad);
    }

    //! Die 33 Meshy-Modelle (9 Fische, 24 Filets): Config-Modell zeigt auf die
    //! neue p3d, das Item spawnt.
    static void NewSealifeModels(vector site)
    {
        TStringArray classes = {"ChefZ_Plaice", "ChefZ_Sole", "ChefZ_Halibut", "ChefZ_GiltheadBream", "ChefZ_Conger", "ChefZ_HorseMackerel", "ChefZ_JapaneseSardine", "ChefZ_RedSnapper", "ChefZ_Salmon", "ChefZ_PlaiceFillet", "ChefZ_GiltheadBreamFillet", "ChefZ_HalibutFillet", "ChefZ_CodFillet", "ChefZ_SoleFillet", "ChefZ_SeaBassFillet", "ChefZ_RedMulletFillet", "ChefZ_CongerFillet", "ChefZ_HorseMackerelFillet", "ChefZ_JapaneseSardineFillet", "ChefZ_RockfishFillet", "ChefZ_GiantTrevallyFillet", "ChefZ_BlueMarlinFillet", "ChefZ_SailfishFillet", "ChefZ_BonitoFillet", "ChefZ_YellowtailFillet", "ChefZ_TarponFillet", "ChefZ_SalmonFillet", "ChefZ_SockeyeSalmonFillet", "ChefZ_PerchFillet", "ChefZ_MuskellungeFillet", "ChefZ_SnakeheadFillet", "ChefZ_RainbowTroutFillet", "ChefZ_CharFillet"};
        int good = 0;
        string bad = "";
        foreach (string cls : classes)
        {
            string model;
            g_Game.ConfigGetText("CfgVehicles " + cls + " model", model);
            string want = cls.Substring(6, cls.Length() - 6);
            want.ToLower();
            bool inSealife = model.IndexOf("ChefZ_Sealife") >= 0;
            bool ownMesh = model.IndexOf(want + ".p3d") >= 0;
            bool modelOk = inSealife && ownMesh;
            EntityAI e = EntityAI.Cast(g_Game.CreateObjectEx(cls, site, ECE_PLACE_ON_SURFACE));
            bool spawned = e != null;
            if (e)
                g_Game.ObjectDelete(e);
            if (modelOk && spawned)
                good++;
            else
                bad = bad + cls + "(modell=" + model + ",spawn=" + spawned.ToString() + ") ";
        }
        bool all = (good == classes.Count());
        ChefZ_AT_Log.Result("FIX", "sealife.newModels", all, "ok=" + good.ToString() + "/" + classes.Count().ToString() + " " + bad);
    }

    //==========================================================================
    // Angel-QTE (Serverseite)
    //==========================================================================

    static void QteTiers()
    {
        float scores[6] = {1.0, 0.95, 0.85, 0.65, 0.45, 0.1};
        string want[6] = {"PREMIUM", "PREMIUM", "SEASONED", "PREPARED", "SIMPLE", "POOR"};
        int good = 0;
        string got = "";
        for (int i = 0; i < 6; i++)
        {
            string t = ChefZ_FishingQteConst.QualityTierFor(scores[i]);
            got = got + scores[i].ToString() + "=" + t + " ";
            if (t == want[i])
                good++;
        }
        ChefZ_AT_Log.Result("FIX", "qte.tiers", good == 6, got);
    }

    //! Baut eine Angelaktion wie die Engine (Rute mit Haken in der Hand, Kontext,
    //! Ziel) und loest sie ueber ChefZ_FishingQteServer.Resolve auf.
    static void QteResolve(PlayerBase player, vector site, float score, bool expectCatch)
    {
        string tag = "qte.resolve." + score.ToString();
        EntityAI old = player.GetHumanInventory().GetEntityInHands();
        if (old)
            g_Game.ObjectDelete(old);
        EntityAI rod = player.GetHumanInventory().CreateInHands("FishingRod");
        if (!rod)
        {
            ChefZ_AT_Log.Result("FIX", tag, false, "FishingRod nicht in die Hand");
            return;
        }
        EntityAI hook = rod.GetInventory().CreateAttachment("Hook");
        if (hook)
            hook.GetInventory().CreateAttachment("Worm");

        ChefZ_AT_RodCtx ctx = new ChefZ_AT_RodCtx(new Param2<EntityAI, bool>(rod, false));
        map<string, int> w = new map<string, int>();
        ctx.ChefZ_AT_Weights(w);
        string yield = "";
        for (int i = 0; i < w.Count(); i++)
        {
            if (w.GetElement(i) > 0)
            {
                yield = w.GetKey(i);
                break;
            }
        }
        if (yield == "" || !ctx.ChefZ_AT_SetYield(yield))
        {
            ChefZ_AT_Log.Result("FIX", tag, false, "kein Ertrag im Teich-Kontext");
            g_Game.ObjectDelete(rod);
            return;
        }

        FishingActionData fad = new FishingActionData();
        fad.m_Player = player;
        fad.m_MainItem = ItemBase.Cast(rod);
        fad.m_Action = new ActionFishingNew();
        fad.m_Target = new ActionTarget(null, null, -1, player.GetPosition() + Vector(2, 0, 2), 0);
        fad.m_ContextData = ctx;
        fad.m_ChefZ_QteActive = true;

        array<EntityAI> before = new array<EntityAI>();
        ChefZ_AT_Util.CollectNear(player.GetPosition(), 4, before);

        ChefZ_FishingQteServer.Resolve(fad, score, "AutoTest");

        array<EntityAI> after = new array<EntityAI>();
        ChefZ_AT_Util.CollectNear(player.GetPosition(), 4, after);
        ItemBase fang = null;
        foreach (EntityAI e : after)
        {
            if (before.Find(e) < 0 && e.GetType() == yield && !e.IsSetForDeletion())
                fang = ItemBase.Cast(e);
        }

        bool resultOk;
        if (expectCatch)
            resultOk = (fad.m_FishingResult == 1) && fang != null;
        else
            resultOk = (fad.m_FishingResult == 0) && fang == null;
        bool closed = !fad.m_ChefZ_QteActive;

        bool hasFang = fang != null;
        string detail = "ertrag=" + yield + " ergebnis=" + fad.m_FishingResult.ToString() + " fang=" + hasFang.ToString() + " beendet=" + closed.ToString();
        if (fang)
        {
            float wantQty = Math.Lerp(ChefZ_FishingQteConst.QUANTITY_MIN, ChefZ_FishingQteConst.QUANTITY_MAX, score);
            float qty = fang.GetQuantityNormalized();
            string tier = ChefZ_AT_Util.TierOf(fang);
            string wantTier = ChefZ_FishingQteConst.QualityTierFor(score);
            bool qtyOk = Math.AbsFloat(qty - wantQty) < 0.05;
            bool tierOk = (tier == wantTier);
            detail = detail + " menge=" + qty.ToString() + " soll=" + wantQty.ToString() + " stufe=" + tier + " soll=" + wantTier;
            resultOk = resultOk && qtyOk && tierOk;
            g_Game.ObjectDelete(fang);
        }
        ChefZ_AT_Log.Result("FIX", tag, resultOk && closed, detail);
        g_Game.ObjectDelete(rod);
    }
}

//==============================================================================
// Kochbuch-Platz (Gruppe FIX, 27.10.2026)
//
// Drei Wege, jeder mit 1,5 s Luft fuer den verzoegerten Versuch des Items
// (ChefZ_CookbookItem.CHEFZ_SLOT_DELAY_MS = 250):
//   create      CreateInInventory -> Buch landet im Platz
//   cargoPickup Buch vom Boden gezielt auf ein Cargo-Feld -> Server zieht es
//               in den Platz
//   noFight     Spieler raeumt es aus dem Platz ins Cargo -> es bleibt dort
//
// Laeuft als eigene Kette neben dem Raeucherschrank (der 16 s wartet).
//==============================================================================

class ChefZ_AT_CookbookSlot
{
    static ref ChefZ_AT_CookbookSlot s_Instance;

    static const int WAIT_MS = 1500;

    PlayerBase m_Player;
    vector m_Site;
    EntityAI m_Book;

    static void Start(PlayerBase player, vector site)
    {
        s_Instance = new ChefZ_AT_CookbookSlot();
        s_Instance.m_Player = player;
        s_Instance.m_Site = site;
        s_Instance.Begin();
    }

    int SlotId()
    {
        return InventorySlots.GetSlotIdFromString(ChefZ_CookbookItem.CHEFZ_SLOT_NAME);
    }

    bool InSlot(EntityAI book)
    {
        if (!book || !m_Player)
            return false;
        int id = SlotId();
        if (id == InventorySlots.INVALID)
            return false;
        return m_Player.GetInventory().FindAttachment(id) == book;
    }

    string Where(EntityAI book)
    {
        if (!book)
            return "null";
        InventoryLocation il = new InventoryLocation();
        if (!book.GetInventory().GetCurrentInventoryLocation(il))
            return "unbekannt";
        return typename.EnumToString(InventoryLocationType, il.GetType()) + " in " + Object.GetDebugName(il.GetParent());
    }

    //! Buch aus dem Boden gezielt auf ein freies Cargo-Feld des Spielers.
    bool MoveToCargo(EntityAI book)
    {
        InventoryLocation src = new InventoryLocation();
        InventoryLocation dst = new InventoryLocation();
        if (!book.GetInventory().GetCurrentInventoryLocation(src))
            return false;
        if (!m_Player.GetInventory().FindFreeLocationFor(book, FindInventoryLocationType.CARGO, dst))
            return false;
        return m_Player.ServerTakeToDst(src, dst);
    }

    void Begin()
    {
        int id = SlotId();
        bool slotKnown = id != InventorySlots.INVALID;
        TStringArray uiSlots = new TStringArray();
        g_Game.ConfigGetTextArray("CfgVehicles SurvivorBase InventoryEquipment playerSlots", uiSlots);
        bool inUi = uiSlots.Find("Slot_ChefZ_Cookbook") >= 0;
        TStringArray atts = new TStringArray();
        g_Game.ConfigGetTextArray("CfgVehicles SurvivorBase attachments", atts);
        bool inAtts = atts.Find(ChefZ_CookbookItem.CHEFZ_SLOT_NAME) >= 0;
        ChefZ_AT_Log.Result("FIX", "cookbook.slot.config", slotKnown && inUi && inAtts, "slotId=" + id.ToString() + " playerSlots=" + inUi.ToString() + " attachments=" + inAtts.ToString() + " ui=" + uiSlots.Count().ToString());
        if (!slotKnown || !m_Player)
            return;

        // Ein Buch aus einem frueheren Schritt (Testmission) raeumt den Platz.
        EntityAI old = m_Player.GetInventory().FindAttachment(id);
        if (old)
        {
            ChefZ_AT_Log.Info("cookbook.slot: Platz war belegt von " + old.GetType() + " - geloescht");
            g_Game.ObjectDelete(old);
        }
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(StepCreate, 300, false);
    }

    void StepCreate()
    {
        m_Book = m_Player.GetInventory().CreateInInventory("ChefZ_CookbookItem");
        if (!m_Book)
        {
            ChefZ_AT_Log.Result("FIX", "cookbook.slot.create", false, "CreateInInventory lieferte null");
            return;
        }
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckCreate, WAIT_MS, false);
    }

    void CheckCreate()
    {
        bool ok = InSlot(m_Book);
        ChefZ_AT_Log.Result("FIX", "cookbook.slot.create", ok, "ort=" + Where(m_Book));
        if (m_Book)
            g_Game.ObjectDelete(m_Book);
        m_Book = null;
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(StepCargoPickup, 300, false);
    }

    void StepCargoPickup()
    {
        m_Book = EntityAI.Cast(g_Game.CreateObjectEx("ChefZ_CookbookItem", m_Site, ECE_PLACE_ON_SURFACE));
        if (!m_Book)
        {
            ChefZ_AT_Log.Result("FIX", "cookbook.slot.cargoPickup", false, "Buch nicht spawnbar");
            return;
        }
        if (!MoveToCargo(m_Book))
        {
            ChefZ_AT_Log.Result("FIX", "cookbook.slot.cargoPickup", false, "kein freies Cargo-Feld am Testspieler, ort=" + Where(m_Book));
            g_Game.ObjectDelete(m_Book);
            m_Book = null;
            return;
        }
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckCargoPickup, WAIT_MS, false);
    }

    void CheckCargoPickup()
    {
        bool ok = InSlot(m_Book);
        ChefZ_AT_Log.Result("FIX", "cookbook.slot.cargoPickup", ok, "ort=" + Where(m_Book));
        if (!ok)
        {
            if (m_Book)
                g_Game.ObjectDelete(m_Book);
            m_Book = null;
            return;
        }

        // Jetzt der Spieler: Buch bewusst aus dem Platz ins Cargo.
        if (!MoveToCargo(m_Book))
        {
            ChefZ_AT_Log.Result("FIX", "cookbook.slot.noFight", false, "Umraeumen ins Cargo fehlgeschlagen, ort=" + Where(m_Book));
            g_Game.ObjectDelete(m_Book);
            m_Book = null;
            return;
        }
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CheckNoFight, WAIT_MS, false);
    }

    void CheckNoFight()
    {
        bool stillMine = m_Book && m_Book.GetHierarchyRootPlayer() == m_Player;
        bool ok = stillMine && !InSlot(m_Book);
        ChefZ_AT_Log.Result("FIX", "cookbook.slot.noFight", ok, "ort=" + Where(m_Book));
        if (m_Book)
            g_Game.ObjectDelete(m_Book);
        m_Book = null;
    }
}
