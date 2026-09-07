//==============================================================================
// ChefZ_CookbookServer - die Anbindung an den Ereignisbus
//
// Entwurf: ChefZ_Cookbook_Workflow §4.3, Zeilen 2 und 3 der Tabelle.
//
// ---------------------------------------------------------------------------
// WARUM HIER UND NICHT IM MANAGER
// ---------------------------------------------------------------------------
// ChefZ_EventArgs traegt die Spielerkennung als int, nicht den Spieler. Sie
// aufzuloesen verlangt PlayerBase, und das gibt es erst in 4_World. Der Manager
// bleibt dadurch frei von Engine-Typen und bleibt pruefbar, ohne dass eine
// Welt laeuft.
//
// ---------------------------------------------------------------------------
// WARUM KEIN EIGENER "modded class MissionServer"
// ---------------------------------------------------------------------------
// Der Core hat genau einen Einstiegspunkt je Seite, und zwei Comp-Module mit je
// einem eigenen MissionServer-Override haben den Server am 28.08.2026 mit einer
// Zugriffsverletzung beendet. Diese Anbindung haengt sich deshalb an den ersten
// Spieler, der entsteht - vor dem ersten Spieler kann niemand kochen, und
// damit ist der Zeitpunkt frueh genug.
//
// Layer: 4_World. Keine Dabs-Referenz (Regel 3).
//==============================================================================

class ChefZ_CookbookServer : Managed
{
    //! Abo auf ChefZ_OnRecipeCompleted. -1 = noch nicht abonniert.
    private static int s_SubscriptionId = -1;

    //! Der Abonnent muss als Objekt am Bus haengen; ein statischer Aufruf
    //! allein hat keinen Besitzer, und ChefZ_EventBus.UnsubscribeOwner braucht
    //! einen.
    private static ref ChefZ_CookbookServer s_Instance;

    /**
     * Einmal je Serverlauf. Jeder weitere Aufruf kehrt sofort zurueck.
     *
     * Gerufen aus PlayerBase.EEInit heraus - siehe Kopf.
     */
    static void EnsureAttached()
    {
        if (s_SubscriptionId >= 0)
            return;
        if (!g_Game || !g_Game.IsDedicatedServer())
            return;

        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (!bus)
            return;

        if (!s_Instance)
            s_Instance = new ChefZ_CookbookServer();

        ScriptCaller cb = ScriptCaller.Create(s_Instance.OnRecipeCompleted);
        s_SubscriptionId = bus.Subscribe(ChefZ_EventNames.RECIPE_COMPLETED, s_Instance, cb, "ChefZ_Cookbook", 0);

        ChefZ_KnowledgeManager.Get().ReadSettings();

        int schwelle = ChefZ_KnowledgeManager.Get().GetPartialMinKnownSlots();
        ChefZ_Log.Banner("Kochbuch: Wissensverfolgung aktiv, PARTIAL ab " + schwelle.ToString() + " bekannten Pflichtslots.");
    }

    /**
     * Ein Rezept wurde fertig gekocht.
     *
     * Wer nicht zuzuordnen ist, erzeugt kein Wissen: ein Kochvorgang ohne
     * Spieler - Admin-Spawn, Skript, Testlauf - darf niemandem etwas
     * beibringen.
     */
    void OnRecipeCompleted(ChefZ_EventArgs args)
    {
        if (!args)
            return;

        PlayerBase spieler = FindPlayer(args.identityId);
        if (!spieler)
            return;

        ChefZ_KnowledgeState stand = spieler.ChefZ_GetKnowledge();
        if (!stand)
            return;

        if (!stand.AddMastered(args.recipeOrTransform))
            return;                     // schon gemeistert, nichts Neues

        spieler.ChefZ_MarkKnowledgeDirty();
        spieler.ChefZ_SendFullState();
        RaiseDiscovered(args.identityId, args.recipeOrTransform);
    }

    /**
     * Feuert ChefZ_OnRecipeDiscovered.
     *
     * Der Core deklariert das Ereignis seit S13 und loest es nirgends aus -
     * dies ist die einzige Stelle im Projekt, die es tut. Wer darauf hoert,
     * geht dieses Addon nichts an; das Terje-Modul kann daran seine XP haengen,
     * ohne dass hier ein Terje-Name steht.
     */
    private void RaiseDiscovered(int identityId, ChefZ_Sym recipeSym)
    {
        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (!bus)
            return;
        if (!bus.HasSubscribers(ChefZ_EventNames.RECIPE_DISCOVERED))
            return;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.RECIPE_DISCOVERED);
        args.identityId        = identityId;
        args.recipeOrTransform = recipeSym;
        bus.Raise(args);
    }

    /**
     * Spielerkennung -> Spieler.
     *
     * Dieselbe Aufloesung wie in ChefZ_CookActor: die Kennung stammt aus
     * PlayerIdentity.GetPlayerId(), und der einzige Weg zurueck fuehrt ueber
     * die Spielerliste. Bei einer Handvoll Spieler ist das billig; es faellt
     * ausserdem nur an, wenn wirklich ein Gericht fertig wurde.
     */
    private static PlayerBase FindPlayer(int identityId)
    {
        if (identityId <= 0 || !g_Game)
            return null;

        array<Man> spieler = new array<Man>();
        g_Game.GetPlayers(spieler);

        for (int i = 0; i < spieler.Count(); i++)
        {
            PlayerBase p = PlayerBase.Cast(spieler.Get(i));
            if (!p)
                continue;
            PlayerIdentity id = p.GetIdentity();
            if (!id)
                continue;
            if (id.GetPlayerId() == identityId)
                return p;
        }
        return null;
    }
}
