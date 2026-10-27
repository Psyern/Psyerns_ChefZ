//==============================================================================
// ChefZ_FishingQteHooks - das Quicktime-Minispiel im Vanilla-Angelablauf
//
// 27.10.2026. Kopf und Ablauf: ChefZ_FishingQte.c (3_Game).
//
// WO VANILLA ENTSCHEIDET
// ActionFishingNew.EvaluateFishingResult laeuft auf Client UND Server, wenn
// der Spieler im Signal reagiert. Vanilla wuerfelt dort mit dem
// SYNCHRONISIERTEN Zufall (Hakenverlust) und faengt sofort. Diese Datei teilt
// das auf:
//   - Hakenverlust (OnBeforeSpawnSignalHit) bleibt an derselben Stelle, auf
//     beiden Seiten im selben Takt - sonst laeuft der synchrone Zufall
//     auseinander und jeder spaetere Wurf wuerfelt verschieden.
//   - Statt zu fangen, startet das Minispiel; EvaluateFishingResult gibt -1
//     zurueck ("noch kein Ergebnis"), die Aktion laeuft weiter.
//   - CAContinuousRepeatFishing.Execute haelt die Zeit an, solange das Spiel
//     laeuft: kein neues Signal, kein Zyklusende.
//   - CAContinuousRepeatFishing.Setup behaelt die Zykluslaenge der Fangtabelle,
//     auch wenn ein anderer Mod (COT-Schnellaktionen) sie verkuerzt - sonst
//     beisst nie ein Fisch und es gibt nichts, worauf das Minispiel antwortet.
//   - Den Fang loest NUR der Server auf (ChefZ_FishingQteServer.Resolve), mit
//     dem Ergebnis des Clients oder nach Zeitablauf mit 0. Dort wird kein
//     synchroner Zufall mehr verbraucht.
//
// Kein Override von GetQualityModifier/GetChanceCoef - andere Angelmods
// (Terje) haengen sich dort an.
//
// Layer: 4_World.
//==============================================================================

// Spieltest 27.10.2026 (zweite Runde) "kein QTE beim Fischen", ohne jede Spur
// im Log: Diagnose fuer den Ablauf Zyklus -> Biss -> Minispiel, auf WARN, damit
// sie mit der Standardstufe (2) im Client- und Serverlog landet. Wieder
// entfernen, sobald das Minispiel im Spieltest bestaetigt ist.
class ChefZ_FishingQteDiag
{
    static void Note(string msg)
    {
        string side = "Client";
        if (g_Game && g_Game.IsDedicatedServer())
            side = "Server";
        ChefZ_Log.Warn(ChefZ_LogChannel.FISHING, "Angel-QTE Diagnose (" + side + "): " + msg);
    }
}

modded class FishingActionData
{
    bool  m_ChefZ_QteActive;
    float m_ChefZ_QteStartedAt;
    float m_ChefZ_QteDeadline;
}

modded class ActionManagerBase
{
    //! Die laufende Aktion samt Daten; Vanilla gibt nur die Aktion heraus.
    ActionData ChefZ_GetCurrentActionData()
    {
        return m_CurrentActionData;
    }
}

// Angeln ohne Festhalten (Spieltest 27.10.2026): ein Klick wirft aus, Loslassen
// beendet nichts mehr (Vanillas SetIgnoreAutomaticInputEnd), ein weiterer Klick
// holt ein - Vanilla wertet das wie ein Loslassen aus (OnEndInput). Bewusst
// KEINE eigene Eingabeklasse: Aktionshinweis und Auswahl haengen fest an
// ContinuousDefaultActionInput (ItemActionsWidget.c:256).
modded class ActionManagerClient
{
    protected bool m_ChefZ_FishingLatch;

    override void InputsUpdate()
    {
        bool fishing = false;
        if (m_CurrentActionData)
            fishing = ActionFishingNew.Cast(m_CurrentActionData.m_Action) != null;
        if (fishing != m_ChefZ_FishingLatch)
        {
            m_ChefZ_FishingLatch = fishing;
            SetIgnoreAutomaticInputEnd(fishing);
            ChefZ_FishingQteDiag.Note("Angeln ohne Festhalten=" + fishing.ToString());
        }

        super.InputsUpdate();

        if (!fishing)
            return;
        if (!m_CurrentActionData)
            return;
        if (m_ActionInputWantEnd)
            return;
        UAInput click = GetUApi().GetInputByName("UADefaultAction");
        if (click && click.LocalPress())
        {
            ChefZ_FishingQteDiag.Note("Klick: einholen");
            EndActionInput();
        }
    }
}

modded class ActionFishingNew
{
    override int EvaluateFishingResult(ActionData action_data)
    {
        FishingActionData fad;
        if (!Class.CastTo(fad, action_data))
            return super.EvaluateFishingResult(action_data);

        // Das Minispiel laeuft schon: ein weiteres Loslassen aendert nichts.
        if (fad.m_ChefZ_QteActive)
            return -1;

        // Kein Biss: Vanilla (Fehlwurf, Koederverlust).
        if (!fad.m_IsSignalActive)
            return super.EvaluateFishingResult(action_data);

        return ChefZ_HookFish(fad);
    }

    // Spieltest 27.10.2026 "beim Fischen habe ich immer noch den Vanilla-Zustand
    // ... ich muss nach wie vor die Maustaste gedrueckt halten": Das Minispiel
    // startet beim Biss von selbst, nicht erst beim Loslassen im Signal. Das
    // Signal laeuft aus CAContinuousRepeatFishing.Execute auf Client und Server
    // im selben Takt - der Hakenverlust darin bleibt damit synchron.
    override void OnSignalStart(FishingActionData action_data)
    {
        super.OnSignalStart(action_data);
        ChefZ_FishingQteDiag.Note("Biss, Minispiel aktiv=" + action_data.m_ChefZ_QteActive.ToString() + " Ergebnis=" + action_data.m_FishingResult.ToString());
        if (action_data.m_ChefZ_QteActive)
            return;
        if (action_data.m_FishingResult != -1)
            return;
        int result = ChefZ_HookFish(action_data);
        if (result != -1)
            action_data.m_FishingResult = result;
    }

    //! Biss getroffen: Hakenverlust wie Vanilla, dann das Minispiel.
    //! -1 = Minispiel laeuft, 0 = Haken weg (kein Fang).
    protected int ChefZ_HookFish(FishingActionData fad)
    {
        fad.m_ContextData.OnBeforeSpawnSignalHit();

        EntityAI hookCheck = fad.m_MainItem.FindAttachmentBySlotName("Hook");
        if (!hookCheck || hookCheck.IsPendingDeletion() || hookCheck.IsSetForDeletion())
        {
            // Haken weg - der Fisch auch. Wie Vanilla ohne Fang.
            fad.m_ContextData.OnAfterSpawnSignalHit();
            ChefZ_FishingQteDiag.Note("Haken beim Biss verloren, kein Minispiel");
            return 0;
        }

        ChefZ_StartQte(fad);
        return -1;
    }

    protected void ChefZ_StartQte(FishingActionData fad)
    {
        fad.m_ChefZ_QteActive = true;
        fad.m_IsSignalActive = false;

        if (!g_Game.IsMultiplayer() || g_Game.IsDedicatedServer())
        {
            fad.m_ChefZ_QteStartedAt = g_Game.GetTickTime();
            fad.m_ChefZ_QteDeadline = fad.m_ChefZ_QteStartedAt + ChefZ_FishingQteConst.SERVER_TIMEOUT_SEC;
        }

        bool own = !g_Game.IsDedicatedServer() && fad.m_Player && fad.m_Player == g_Game.GetPlayer();
        ChefZ_FishingQteDiag.Note("Minispiel gestartet, HUD angefordert=" + own.ToString());
        if (own)
            ChefZ_FishingQteBridge.RequestStart();
    }

    //! Oeffentlicher Zugang zu Vanillas TrySpawnCatch (protected).
    EntityAI ChefZ_SpawnCatch(FishingActionData fad)
    {
        return TrySpawnCatch(fad);
    }
}

modded class CAContinuousRepeatFishing
{
    // Spieltest 27.10.2026 "beim Benutzen der Angel kommt kein QTE":
    // COT-Schnellaktionen (aktives COT + Recht Actions.QuickActions) setzen in
    // CAContinuousRepeat.Setup m_TimeToComplete = 0.1 fuer JEDE wiederholte
    // Aktion (COT cacontinuousrepeat.c:7-8) - die Angel erbt das. Ein Zyklus von
    // 0.1 s ist kuerzer als die beiden Totzonen (1 s), SetNewSignalData
    // verwirft jedes Signal ("Cycle m_TimeToComplete: 0.1 shorter than combined
    // deadzone times" im Client- und Server-Log), es beisst nie ein Fisch und
    // das Minispiel kann nicht starten. Die Zykluslaenge der Angel ist keine
    // Wartezeit, sondern das Zeitfenster fuer den Biss - sie bleibt, was die
    // Fangtabelle vorgibt.
    override void Setup(ActionData action_data)
    {
        super.Setup(action_data);
        m_TimeToComplete = m_DefaultTimeToComplete;
        ChefZ_FishingQteDiag.Note("Auswurf, Zykluslaenge " + m_TimeToComplete.ToString() + " s");
    }

    override void SetNewSignalData(float startTime, float durationTime)
    {
        super.SetNewSignalData(startTime, durationTime);
        ChefZ_FishingQteDiag.Note("Naechster Zyklus: Biss bei " + m_SignalStartTime.ToString() + " s, Dauer " + m_SignalDuration.ToString() + " s (-1 = kein Biss)");
    }

    override int Execute(ActionData action_data)
    {
        FishingActionData fad = FishingActionData.Cast(action_data);
        if (!fad || !fad.m_ChefZ_QteActive)
            return super.Execute(action_data);

        if (!action_data.m_Player)
            return UA_ERROR;

        // Zeit steht. Der Server loest nach Ablauf ohne Ergebnis auf.
        if (!g_Game.IsMultiplayer() || g_Game.IsDedicatedServer())
        {
            if (g_Game.GetTickTime() > fad.m_ChefZ_QteDeadline)
                ChefZ_FishingQteServer.Resolve(fad, 0.0, "Zeit abgelaufen");
        }

        if (fad.m_FishingResult != -1)
            return UA_FINISHED;
        return UA_PROCESSING;
    }
}

//------------------------------------------------------------------------------
// Serverseite
//------------------------------------------------------------------------------

class ChefZ_FishingQteServer
{
    //! Ergebnis eines Clients. Nur gueltig, wenn sein Spieler gerade angelt und
    //! das Minispiel laeuft; zu schnelle Ergebnisse zaehlen als 0.
    static void SubmitScore(PlayerBase player, float score)
    {
        if (!player || !player.GetActionManager())
            return;

        FishingActionData fad = FishingActionData.Cast(player.GetActionManager().ChefZ_GetCurrentActionData());
        if (!fad || !fad.m_ChefZ_QteActive)
            return;

        float dauer = g_Game.GetTickTime() - fad.m_ChefZ_QteStartedAt;
        if (dauer < ChefZ_FishingQteConst.MIN_DURATION_SEC)
        {
            ChefZ_Log.Warn(ChefZ_LogChannel.FISHING, "Angel-QTE: Ergebnis nach " + dauer.ToString() + " s verworfen (kuerzer als " + ChefZ_FishingQteConst.MIN_DURATION_SEC.ToString() + " s).");
            score = 0.0;
        }

        Resolve(fad, score, "Ergebnis");
    }

    static void Resolve(FishingActionData fad, float score, string reason)
    {
        if (!fad || !fad.m_ChefZ_QteActive)
            return;

        fad.m_ChefZ_QteActive = false;
        score = Math.Clamp(score, 0.0, 1.0);

        int result = 0;
        string catchName = "";
        if (score >= ChefZ_FishingQteConst.MIN_SCORE_CATCH)
        {
            ActionFishingNew action = ActionFishingNew.Cast(fad.m_Action);
            if (action)
            {
                float quantity = Math.Lerp(ChefZ_FishingQteConst.QUANTITY_MIN, ChefZ_FishingQteConst.QUANTITY_MAX, score);
                fad.m_ContextData.SetResultQualityOverride(true, quantity);
                EntityAI fang = action.ChefZ_SpawnCatch(fad);
                fad.m_ContextData.SetResultQualityOverride(false);
                if (fang)
                {
                    result = 1;
                    catchName = fang.GetDisplayName();
                    ApplyTier(fang, score);
                }
            }
        }

        // Der Koeder ist so oder so gefressen, der Haken nimmt Schaden (Vanilla
        // nach dem Fang). Kein synchroner Zufall darin.
        fad.m_ContextData.OnAfterSpawnSignalHit();

        fad.m_FishingResult = result;

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.FISHING, ChefZ_LogLevel.INFO))
            ChefZ_Log.Info(ChefZ_LogChannel.FISHING, "Angel-QTE " + reason + ": Ergebnis " + score.ToString() + " -> " + result.ToString() + " " + catchName);

        PlayerBase player = fad.m_Player;
        if (!player)
            return;

        if (!g_Game.IsMultiplayer())
        {
            ChefZ_FishingQteBridge.PostResult(result, catchName);
            return;
        }

        if (player.GetIdentity())
        {
            Param2<int, string> data = new Param2<int, string>(result, catchName);
            g_Game.RPCSingleParam(player, ChefZ_FishingQteRPC.RESULT, data, true, player.GetIdentity());
        }
    }

    protected static void ApplyTier(EntityAI fang, float score)
    {
        ItemBase item = ItemBase.Cast(fang);
        if (!item)
            return;
        ChefZ_Sym tier = ChefZ_SymbolTable.Lookup(ChefZ_FishingQteConst.QualityTierFor(score));
        if (ChefZ_SymbolTable.IsValid(tier))
            ChefZ_ItemStateComponent.SetQuality(item, tier);
    }
}

//------------------------------------------------------------------------------
// Netz
//------------------------------------------------------------------------------

// modded class PlayerBase
// Zwei RPCs am eigenen Spieler, wie das Kochbuch (ChefZ_PlayerKnowledge.c).
// SCORE nimmt nur der Server an und nur vom Besitzer dieses Spielers; RESULT
// nur der Client.
modded class PlayerBase
{
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);

        if (rpc_type == ChefZ_FishingQteRPC.SCORE)
        {
            if (!g_Game.IsDedicatedServer())
                return;
            if (!sender || !GetIdentity() || sender.GetId() != GetIdentity().GetId())
                return;

            Param1<float> score = new Param1<float>(0.0);
            if (!ctx.Read(score))
                return;
            ChefZ_FishingQteServer.SubmitScore(this, score.param1);
            return;
        }

        if (rpc_type == ChefZ_FishingQteRPC.RESULT)
        {
            if (g_Game.IsDedicatedServer())
                return;

            Param2<int, string> data = new Param2<int, string>(0, "");
            if (!ctx.Read(data))
                return;
            ChefZ_ApplyQteResultClient(data.param1);
            ChefZ_FishingQteBridge.PostResult(data.param1, data.param2);
        }
    }

    //! Der Client beendet seine Aktion im selben Sinn wie der Server.
    protected void ChefZ_ApplyQteResultClient(int result)
    {
        if (!GetActionManager())
            return;
        FishingActionData fad = FishingActionData.Cast(GetActionManager().ChefZ_GetCurrentActionData());
        if (!fad)
            return;
        fad.m_ChefZ_QteActive = false;
        fad.m_FishingResult = result;
    }
}
