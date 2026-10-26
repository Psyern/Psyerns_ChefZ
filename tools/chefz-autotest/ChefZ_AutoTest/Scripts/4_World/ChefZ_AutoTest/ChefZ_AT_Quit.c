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
}

modded class PlayerBase
{
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (rpc_type != ChefZ_AT_Rpc.QUIT)
            return;
        if (!g_Game || g_Game.IsDedicatedServer())
            return;

        Print("[CHEFZTEST] client quit requested by server");
        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(g_Game.RequestExit, 2000, false, 0);
    }
}
