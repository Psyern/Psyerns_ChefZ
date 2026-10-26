//==============================================================================
// ChefZ_AutoTest - Hilfen: spawnen, Zustand setzen, Ergebnisse suchen.
//==============================================================================

class ChefZ_AT_Util
{
    static int LiquidOf(string name)
    {
        if (name == "SaltWater")
            return LIQUID_SALTWATER;
        return LIQUID_WATER;
    }

    static int VanillaStageOf(string name)
    {
        if (name == "Baked")
            return FoodStageType.BAKED;
        if (name == "Boiled")
            return FoodStageType.BOILED;
        if (name == "Dried")
            return FoodStageType.DRIED;
        if (name == "Burned")
            return FoodStageType.BURNED;
        if (name == "Rotten")
            return FoodStageType.ROTTEN;
        return FoodStageType.RAW;
    }

    static ItemBase SpawnGround(string cls, vector pos)
    {
        Object o = g_Game.CreateObjectEx(cls, pos, ECE_PLACE_ON_SURFACE);
        return ItemBase.Cast(o);
    }

    //! Volle Menge, gewuenschter Zustand, gewuenschte Garstufe.
    static void Prepare(ItemBase item, ChefZ_AT_Input input, string liquid, float liquidQty)
    {
        if (!item)
            return;

        if (liquid != "" && item.IsLiquidContainer())
        {
            item.SetLiquidType(LiquidOf(liquid));
            item.SetQuantity(liquidQty);
        }
        else if (item.HasQuantity() && input && input.qtyPct >= 0)
        {
            item.SetQuantity(item.GetQuantityMax() * input.qtyPct);
        }
        else if (item.HasQuantity())
        {
            item.SetQuantityMax();
        }

        if (input && input.vstage != "")
        {
            Edible_Base edible = Edible_Base.Cast(item);
            if (edible && edible.HasFoodStage())
                edible.ChangeFoodStage(VanillaStageOf(input.vstage));
        }

        if (input && input.state != "")
        {
            ChefZ_Sym sym = ChefZ_SymbolTable.Lookup(input.state);
            if (ChefZ_SymbolTable.IsValid(sym))
                ChefZ_ItemStateComponent.SetState(item, sym);
        }
    }

    //! Legt die Eingaben ins Cargo. Liefert "" oder den Grund des Scheiterns.
    static string FillCargo(notnull ItemBase container, notnull ChefZ_AT_Case c)
    {
        foreach (ChefZ_AT_Input input : c.inputs)
        {
            for (int n = 0; n < input.count; n++)
            {
                EntityAI e = CreateAtFreeSpot(container, input.cls);
                if (!e)
                    e = container.GetInventory().CreateEntityInCargo(input.cls);
                if (!e)
                {
                    ItemBase ground = SpawnGround(input.cls, container.GetPosition() + Vector(0.5, 0, 0.5));
                    if (!ground)
                        return "Klasse " + input.cls + " nicht spawnbar";
                    Prepare(ground, input, c.liquid, c.liquidQty);
                    if (!container.ServerTakeEntityToCargo(ground))
                    {
                        g_Game.ObjectDelete(ground);
                        return input.cls + " passt nicht ins Cargo von " + container.GetType();
                    }
                    continue;
                }
                Prepare(ItemBase.Cast(e), input, c.liquid, c.liquidQty);
            }
        }
        return "";
    }

    //! CreateEntityInCargo dreht in 1.30 selbst und laesst dabei Luecken: erst
    //! jede Position ungedreht der Reihe nach (dicht gepackt, groesste Items
    //! kommen zuerst), dann jede gedreht.
    static EntityAI CreateAtFreeSpot(notnull ItemBase container, string cls)
    {
        CargoBase cargo = container.GetInventory().GetCargo();
        if (!cargo)
            return null;
        for (int pass = 0; pass < 2; pass++)
        {
            bool flip = pass == 1;
            for (int row = 0; row < cargo.GetHeight(); row++)
            {
                for (int col = 0; col < cargo.GetWidth(); col++)
                {
                    EntityAI e = container.GetInventory().CreateEntityInCargoEx(cls, 0, row, col, flip);
                    if (e)
                        return e;
                }
            }
        }
        return null;
    }

    static void CollectCargo(notnull EntityAI container, notnull array<EntityAI> outItems)
    {
        array<EntityAI> all = new array<EntityAI>();
        container.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, all);
        foreach (EntityAI e : all)
        {
            if (e != container)
                outItems.Insert(e);
        }
    }

    //! Alle Ausgaben vorhanden? missing nennt die fehlenden.
    static bool HasOutputs(notnull array<EntityAI> items, notnull array<string> outputs, out string missing, out EntityAI first)
    {
        missing = "";
        first = null;
        foreach (string o : outputs)
        {
            bool found = false;
            foreach (EntityAI e : items)
            {
                if (e && e.GetType() == o)
                {
                    found = true;
                    if (!first)
                        first = e;
                    break;
                }
            }
            if (!found)
                missing = missing + o + " ";
        }
        return missing == "";
    }

    static void CollectNear(vector pos, float radius, notnull array<EntityAI> outItems)
    {
        array<Object> objs = new array<Object>();
        g_Game.GetObjectsAtPosition(pos, radius, objs, null);
        foreach (Object o : objs)
        {
            EntityAI e = EntityAI.Cast(o);
            if (!e)
                continue;
            outItems.Insert(e);
            CollectCargo(e, outItems);
        }
    }

    static string TierOf(EntityAI e)
    {
        ItemBase item = ItemBase.Cast(e);
        if (!item)
            return "-";
        ChefZ_ItemStateComponent comp = ChefZ_ItemStateComponent.Of(item);
        if (!comp)
            return "-";
        ChefZ_Sym q = comp.ResolveQualitySym();
        if (!ChefZ_SymbolTable.IsValid(q))
            return "-";
        return ChefZ_SymbolTable.Name(q);
    }

    static void DeleteWithContent(EntityAI e)
    {
        if (e)
            g_Game.ObjectDelete(e);
    }

    static void ClearArea(vector pos, float radius)
    {
        array<Object> objs = new array<Object>();
        g_Game.GetObjectsAtPosition(pos, radius, objs, null);
        foreach (Object o : objs)
        {
            ItemBase ib = ItemBase.Cast(o);
            if (ib && !ib.GetHierarchyRootPlayer())
                g_Game.ObjectDelete(ib);
        }
    }
}
