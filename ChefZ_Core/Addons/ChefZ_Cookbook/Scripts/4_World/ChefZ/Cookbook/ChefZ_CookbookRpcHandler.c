//==============================================================================
// ChefZ_CookbookRpcHandler - Empfang der Kochbuch-Nachrichten
//
// Core-Migration: ersetzt den Zweig in modded PlayerBase.OnRPC. PsyCore_RPC
// ruft Handler nur auf Managed-Instanzen auf und haelt sie schwach - deshalb
// eine eigene Klasse mit einer statischen Instanz, die hier am Leben bleibt.
// PlayerBase selbst ist als Handler nicht zulaessig.
//
// Das Ziel-Objekt (target) ist wie frueher der Spieler, an den die Nachricht
// adressiert war; er wird durchgereicht und hier auf PlayerBase gecastet.
//
// Registriert wird aus PlayerBase.EEInit (laeuft auf Server UND Client) -
// derselbe Einhaengepunkt wie ChefZ_CookbookServer.EnsureAttached, ohne einen
// zusaetzlichen modded MissionServer/MissionGameplay (siehe Kopf von
// ChefZ_PlayerKnowledge.c).
//
// Layer: 4_World (braucht PlayerBase).
//==============================================================================

class ChefZ_CookbookRpcHandler : Managed
{
    private static ref ChefZ_CookbookRpcHandler s_Instance;

    //! Idempotent. Einmal je Prozess registriert, danach ein Zeigervergleich.
    static void EnsureRegistered()
    {
        if (s_Instance)
            return;

        s_Instance = new ChefZ_CookbookRpcHandler();
        PsyCore_RPC.Register(ChefZ_CookbookRPC.NS, ChefZ_CookbookRPC.FULL_STATE, s_Instance, "OnFullState", PsyCore_RpcFlags.CLIENT);
        PsyCore_RPC.Register(ChefZ_CookbookRPC.NS, ChefZ_CookbookRPC.REQUEST_STATE, s_Instance, "OnRequestState", PsyCore_RpcFlags.SERVER);
    }

    /**
     * Server -> Client: der vollstaendige Stand.
     *
     * NUR DER CLIENT NIMMT EINEN STAND ENTGEGEN. Das Flag CLIENT laesst den
     * Router die Nachricht am Server bereits verwerfen; die Wache darunter
     * bleibt als zweite Sicherung stehen. Ohne sie koennte ein Client dem
     * Server einen gefaelschten Stand fuer seinen Spieler schicken
     * (FULL_STATE-Exploit vom 30.08.2026).
     */
    void OnFullState(ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (g_Game && g_Game.IsDedicatedServer())
            return;

        PlayerBase spieler = PlayerBase.Cast(target);
        if (!spieler)
            return;

        spieler.ChefZ_ReceiveFullState(ctx);
    }

    /**
     * Client -> Server: "schick mir den Stand".
     *
     * Nur fuer den Absender selbst: das Ziel muss der Spieler des Absenders
     * sein. Frueher hing das allein am Zielobjekt, das der Client waehlt.
     */
    void OnRequestState(ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (!g_Game || !g_Game.IsDedicatedServer())
            return;
        if (!sender)
            return;

        PlayerBase spieler = PlayerBase.Cast(target);
        if (!spieler)
            return;

        PlayerIdentity besitzer = spieler.GetIdentity();
        if (!besitzer || besitzer.GetId() != sender.GetId())
            return;

        spieler.ChefZ_SendFullState();
    }
}
