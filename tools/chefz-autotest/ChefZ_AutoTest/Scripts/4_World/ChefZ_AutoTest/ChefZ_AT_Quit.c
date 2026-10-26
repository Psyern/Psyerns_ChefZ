//==============================================================================
// ChefZ_AutoTest - Clientseite: der Server meldet "fertig", der Testclient
// beendet sich selbst. So endet ein Testlauf ohne taskkill.
//
// Layer: 4_World (PlayerBase).
//==============================================================================

class ChefZ_AT_Rpc
{
    //! Eigener Bereich, weit ueber Vanillas ERPCs und ueber ChefZ (10000+).
    static const int QUIT = 0x43AE2101;
    //! Sichtpruefung: Inventar vor dem brennenden Raeucherschrank oeffnen.
    static const int SHOW_INVENTORY = 0x43AE2102;
}

modded class PlayerBase
{
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (!g_Game || g_Game.IsDedicatedServer())
            return;
        if (rpc_type == ChefZ_AT_Rpc.SHOW_INVENTORY)
        {
            g_Game.GetMission().ShowInventory();
            g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(ChefZ_AT_ReportInventory, 4000, false);
            return;
        }
        if (rpc_type != ChefZ_AT_Rpc.QUIT)
            return;

        Print("[CHEFZTEST] client quit requested by server");
        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(g_Game.RequestExit, 2000, false, 0);
    }

    void ChefZ_AT_ReportInventory()
    {
        UIScriptedMenu menu = g_Game.GetUIManager().FindMenu(MENU_INVENTORY);
        bool open = menu != null;
        Print("[CHEFZTEST] client inventory open=" + open.ToString());
        array<Object> near = new array<Object>();
        g_Game.GetObjectsAtPosition3D(GetPosition(), 4, near, null);
        string seen = "";
        foreach (Object o : near)
        {
            if (o.IsKindOf("ChefZ_Smoker") || o.IsKindOf("FireplaceBase"))
                seen = seen + o.GetType() + "@" + vector.Distance(o.GetPosition(), GetPosition()).ToString() + " ";
        }
        Print("[CHEFZTEST] client umgebung spieler=" + GetPosition().ToString() + " lebt=" + IsAlive().ToString() + " nah: " + seen);
    }
}
