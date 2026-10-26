//==============================================================================
// ChefZ_AutoTest - Angeln.
//
// Fuer jede Kombination aus Methode (Rute, kleine/grosse Reuse), Gewaesser
// (See, Teich) und Koeder baut der Test den echten Fangkontext der Engine und
// liest das fertige Wahrscheinlichkeitsfeld. Geprueft wird:
//
//   FISH  <ertrag>.catchable   kommt in mindestens einem Kontext vor
//   FISH  <ertrag>.caught      ueber SpawnAndSetupCatch wirklich erzeugt
//   BAIT  <koeder>.<ziel>      Koeder hebt das Gewicht seines Ziels an
//==============================================================================

class ChefZ_AT_Yield
{
    string id;
    string enviro;
    string methods;
    bool lureOnly;

    static ChefZ_AT_Yield Make(string pId, string pEnviro, string pMethods, bool pLureOnly)
    {
        ChefZ_AT_Yield y = new ChefZ_AT_Yield();
        y.id = pId;
        y.enviro = pEnviro;
        y.methods = pMethods;
        y.lureOnly = pLureOnly;
        return y;
    }
}

class ChefZ_AT_Bait
{
    string id;
    string kind;
    ref array<string> targets;

    static ChefZ_AT_Bait Make(string pId, string pKind)
    {
        ChefZ_AT_Bait b = new ChefZ_AT_Bait();
        b.id = pId;
        b.kind = pKind;
        b.targets = new array<string>();
        return b;
    }
}

class ChefZ_AT_Fishing
{
    // Szenario "ROD SEA ChefZ_MinnowLure" -> Typ -> Gewicht
    static ref map<string, ref map<string, int>> s_Weights;
    static ref map<string, bool> s_Caught;
    static ref map<string, string> s_CaughtWhere;

    static void Run(PlayerBase player, vector site)
    {
        s_Weights = new map<string, ref map<string, int>>();
        s_Caught = new map<string, bool>();
        s_CaughtWhere = new map<string, string>();

        array<ref ChefZ_AT_Yield> yields = new array<ref ChefZ_AT_Yield>();
        array<ref ChefZ_AT_Bait> baits = new array<ref ChefZ_AT_Bait>();
        ChefZ_AT_FishData.FillYields(yields);
        ChefZ_AT_FishData.FillBaits(baits);

        array<string> want = new array<string>();
        foreach (ChefZ_AT_Yield y : yields)
            want.Insert(y.id);

        array<string> rodBaits = {"", "Worm"};
        foreach (ChefZ_AT_Bait b : baits)
            rodBaits.Insert(b.id);

        array<bool> seas = {true, false};
        foreach (bool sea : seas)
        {
            foreach (string bait : rodBaits)
                RodScenario(player, site, sea, bait, want);
            TrapScenario(site, "FishNetTrap", sea, "", want);
            TrapScenario(site, "FishNetTrap", sea, "Worm", want);
            TrapScenario(site, "SmallFishTrap", sea, "", want);
            TrapScenario(site, "SmallFishTrap", sea, "Worm", want);
        }

        foreach (ChefZ_AT_Yield yy : yields)
        {
            string where = FindScenario(yy.id);
            ChefZ_AT_Log.Result("FISH", yy.id + ".catchable", where != "", "enviro=" + yy.enviro + " methods=" + yy.methods + " lureOnly=" + yy.lureOnly.ToString() + " erstesSzenario=" + where);
            bool caught = s_Caught.Contains(yy.id);
            ChefZ_AT_Log.Result("FISH", yy.id + ".caught", caught, "via " + s_CaughtWhere.Get(yy.id));
        }

        foreach (ChefZ_AT_Bait bb : baits)
        {
            foreach (string target : bb.targets)
                CheckBait(bb, target);
        }

        int scenarios = s_Weights.Count();
        ChefZ_AT_Log.Info("fishing scenarios=" + scenarios.ToString());
    }

    static string FindScenario(string type)
    {
        for (int i = 0; i < s_Weights.Count(); i++)
        {
            map<string, int> w = s_Weights.GetElement(i);
            if (w.Get(type) > 0)
                return s_Weights.GetKey(i);
        }
        return "";
    }

    static string EnvName(bool sea)
    {
        if (sea)
            return "SEA";
        return "POND";
    }

    static void Record(string scenario, map<string, int> weights)
    {
        s_Weights.Set(scenario, weights);
        int total = 0;
        int chefz = 0;
        string top = "";
        for (int i = 0; i < weights.Count(); i++)
        {
            int n = weights.GetElement(i);
            total = total + n;
            string t = weights.GetKey(i);
            if (t.IndexOf("ChefZ_") == 0)
            {
                chefz = chefz + n;
                top = top + t + ":" + n.ToString() + " ";
            }
        }
        ChefZ_AT_Log.Info("fish " + scenario + " eintraege=" + total.ToString() + " chefz=" + chefz.ToString() + " | " + top);
    }

    static void RodScenario(PlayerBase player, vector site, bool sea, string bait, array<string> want)
    {
        string name = "ROD " + EnvName(sea) + " " + bait;
        EntityAI old = player.GetHumanInventory().GetEntityInHands();
        if (old)
            g_Game.ObjectDelete(old);
        EntityAI rod = player.GetHumanInventory().CreateInHands("FishingRod");
        if (!rod)
        {
            ChefZ_AT_Log.Result("FISH", "setup.rod", false, "FishingRod nicht in die Hand");
            return;
        }

        bool isLure = false;
        if (bait != "")
            isLure = ChefZ_FishingRegistry.Get().IsLure(bait.Hash());
        EntityAI attached;
        if (isLure)
        {
            attached = rod.GetInventory().CreateAttachment(bait);
        }
        else
        {
            EntityAI hook = rod.GetInventory().CreateAttachment("Hook");
            attached = hook;
            if (hook && bait != "")
                attached = hook.GetInventory().CreateAttachment(bait);
        }
        if (!attached)
        {
            ChefZ_AT_Log.Result("FISH", "setup." + bait, false, "Koeder/Haken laesst sich nicht anbringen (lure=" + isLure.ToString() + ")");
            g_Game.ObjectDelete(rod);
            return;
        }

        ChefZ_AT_RodCtx ctx = new ChefZ_AT_RodCtx(new Param2<EntityAI, bool>(rod, sea));
        map<string, int> w = new map<string, int>();
        ctx.ChefZ_AT_Weights(w);
        Record(name, w);

        foreach (string type : want)
        {
            if (s_Caught.Contains(type) || w.Get(type) <= 0)
                continue;
            EntityAI fish = ctx.ChefZ_AT_SpawnAs(type, site + Vector(2, 0, 2));
            if (fish && fish.GetType() == type)
            {
                s_Caught.Set(type, true);
                s_CaughtWhere.Set(type, name);
            }
            if (fish)
                g_Game.ObjectDelete(fish);
        }
        g_Game.ObjectDelete(rod);
    }

    static void TrapScenario(vector site, string trapCls, bool sea, string bait, array<string> want)
    {
        string name = trapCls + " " + EnvName(sea) + " " + bait;
        EntityAI trap = EntityAI.Cast(g_Game.CreateObjectEx(trapCls, site + Vector(-2, 0, 2), ECE_PLACE_ON_SURFACE));
        if (!trap)
        {
            ChefZ_AT_Log.Result("FISH", "setup." + trapCls, false, "nicht spawnbar");
            return;
        }
        if (bait != "")
            trap.GetInventory().CreateAttachment(bait);

        int mask = AnimalCatchingConstants.MASK_ENVIRO_POND;
        if (sea)
            mask = AnimalCatchingConstants.MASK_ENVIRO_SEA;
        Param2<EntityAI, int> par = new Param2<EntityAI, int>(trap, 1);
        map<string, int> w = new map<string, int>();
        ChefZ_AT_TrapSmallCtx small;
        ChefZ_AT_TrapLargeCtx large;
        if (trapCls == "SmallFishTrap")
        {
            small = new ChefZ_AT_TrapSmallCtx(par);
            small.SetTrapEnviroMask(mask);
            small.UpdateDataAndMasks();
            small.ChefZ_AT_Revalidate();
            small.ChefZ_AT_Weights(w);
        }
        else
        {
            large = new ChefZ_AT_TrapLargeCtx(par);
            large.SetTrapEnviroMask(mask);
            large.UpdateDataAndMasks();
            large.ChefZ_AT_Revalidate();
            large.ChefZ_AT_Weights(w);
        }
        Record(name, w);

        foreach (string type : want)
        {
            if (s_Caught.Contains(type) || w.Get(type) <= 0)
                continue;
            EntityAI fish;
            if (small)
                fish = small.ChefZ_AT_SpawnAs(type, site + Vector(-2, 0, 4));
            else
                fish = large.ChefZ_AT_SpawnAs(type, site + Vector(-2, 0, 4));
            if (fish && fish.GetType() == type)
            {
                s_Caught.Set(type, true);
                s_CaughtWhere.Set(type, name);
            }
            if (fish)
                g_Game.ObjectDelete(fish);
        }
        g_Game.ObjectDelete(trap);
    }

    //! Ziel mit Koeder schwerer als mit blankem Haken, in dem Gewaesser, in dem es vorkommt.
    static void CheckBait(ChefZ_AT_Bait b, string target)
    {
        array<bool> seas = {true, false};
        string detail = "";
        bool ok = false;
        bool seen = false;
        foreach (bool sea : seas)
        {
            string env = EnvName(sea);
            map<string, int> with = s_Weights.Get("ROD " + env + " " + b.id);
            map<string, int> plain = s_Weights.Get("ROD " + env + " ");
            if (!with || !plain)
                continue;
            int a = with.Get(target);
            int plainW = plain.Get(target);
            if (a <= 0 && plainW <= 0)
                continue;
            seen = true;
            detail = detail + env + " " + plainW.ToString() + "->" + a.ToString() + " ";
            if (a > plainW)
                ok = true;
        }
        if (!seen)
            detail = "Ziel kommt mit diesem Koeder in keinem Gewaesser vor";
        ChefZ_AT_Log.Result("BAIT", b.id + "." + target, ok, b.kind + " " + detail);
    }
}
