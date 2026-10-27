//==============================================================================
// ChefZ_AutoTest - Clientseite: Kochbuch-Menue und Angel-QTE-HUD (27.10.2026).
//
// Der Server schickt CLIENT_PROBE, sobald der Testspieler steht. Dann:
//   1. Kochbuch oeffnen, nach 3 s Zeilen der Rezeptliste zaehlen, schliessen
//   2. Angel-QTE mit simulierter Eingabe durchspielen: Kreisrunde 2 und die
//      zweite Taste der ersten Pfeilrunde gehen absichtlich daneben.
//      Sollwertung: 2/3 * 0.3 + 23/24 * 0.7 = 0.8708
//   3. Echter Ausloeseweg (Spieltest 27.10.2026 "kein QTE an der Angel"):
//      Angel-Komponente mit COT-Schnellaktionen aufsetzen (Zyklus muss
//      bleiben), Zeit laufen lassen bis der Fisch beisst, dann Vanillas
//      ActionFishingNew.EvaluateFishingResult wie beim Loslassen - das HUD
//      muss ueber die Bruecke starten und mit der Aktion wieder enden.
//
// Ergebniszeilen "[CHEFZTEST] client PASS|FAIL ..." stehen im script_*.log
// des Clients (profiles/client-130), nicht in der Serverzusammenfassung.
//
// Layer: 5_Mission.
//==============================================================================

modded class ChefZ_FishingQteHud
{
    static bool s_ChefZ_AT_Drive;

    //! Echter Ausloeseweg: die Aktionsdaten der Probe stehen fuer die laufende
    //! Aktion des Spielers. Gelesen wird dasselbe Feld wie im Original.
    static FishingActionData s_ChefZ_AT_Fad;

    override protected bool FishingStillRunning()
    {
        if (s_ChefZ_AT_Fad)
            return s_ChefZ_AT_Fad.m_ChefZ_QteActive;
        if (s_ChefZ_AT_Drive)
            return true;
        return super.FishingStillRunning();
    }

    override protected bool Pressed(string inputName)
    {
        if (!s_ChefZ_AT_Drive)
            return super.Pressed(inputName);
        if (inputName != "UAChefZQteHit" || m_Phase != PHASE_RING)
            return false;
        // Zeiger startet gegenueber der Zone: nach 60 Grad sicher daneben.
        if (m_RingRound == 1)
            return m_NeedleTravel > 60 && !InZone(m_NeedleDeg);
        return InZone(m_NeedleDeg);
    }

    override protected int PressedDirection()
    {
        if (!s_ChefZ_AT_Drive)
            return super.PressedDirection();
        if (m_KeyIndex >= m_Sequence.Count())
            return -1;
        if (m_KeyTime - m_KeyTimeLeft < 0.25)
            return -1;
        int want = m_Sequence.Get(m_KeyIndex);
        if (m_ArrowRound == 0 && m_KeyIndex == 1)
            return (want + 1) % 4;
        return want;
    }

    // Sichtpruefung (27.10.2026): was der Canvas wirklich zeichnet.
    static int s_AT_RingGreen;
    static int s_AT_RingRed;
    static int s_AT_KeyGreen;
    static int s_AT_KeyRed;
    static int s_AT_TimerArcs;
    static float s_AT_ArcMin = 999;
    static float s_AT_ArcMax = -1;
    static float s_AT_BarMin = 2;
    static float s_AT_BarMax = -1;

    override protected void Update(float dt)
    {
        super.Update(dt);
        if (!s_ChefZ_AT_Drive)
            return;
        if (m_Phase == PHASE_RING && m_RingFlash > 0)
        {
            if (m_RingFlashOk)
                s_AT_RingGreen++;
            else
                s_AT_RingRed++;
        }
        if (m_Phase != PHASE_ARROWS)
            return;
        int n = m_Sequence.Count();
        if (n > 0 && m_KeyIndex < n)
        {
            float current = Math.Clamp(m_KeyTimeLeft / m_KeyTime, 0, 1);
            float left = (n - m_KeyIndex - 1 + current) / n;
            s_AT_BarMin = Math.Min(s_AT_BarMin, left);
            s_AT_BarMax = Math.Max(s_AT_BarMax, left);
        }
        if (m_KeyFlash > 0 && m_KeyIndex > 0)
        {
            int state = m_KeyState.Get(m_KeyIndex - 1);
            if (state == KEY_OK)
                s_AT_KeyGreen++;
            else if (state == KEY_FAIL)
                s_AT_KeyRed++;
        }
    }

    //! Der Zeitring um die aktuelle Taste ist der einzige Bogen unter 360 Grad
    //! in der Pfeilphase (Zone im Kreis zaehlt nur in PHASE_RING).
    override protected void DrawArc(float cx, float cy, float r, float startDeg, float spanDeg, float width, int color)
    {
        super.DrawArc(cx, cy, r, startDeg, spanDeg, width, color);
        if (!s_ChefZ_AT_Drive || m_Phase != PHASE_ARROWS)
            return;
        if (spanDeg <= 0 || spanDeg >= 360)
            return;
        s_AT_TimerArcs++;
        s_AT_ArcMin = Math.Min(s_AT_ArcMin, spanDeg);
        s_AT_ArcMax = Math.Max(s_AT_ArcMax, spanDeg);
    }

    static bool ChefZ_AT_VisualsOk()
    {
        bool ring = s_AT_RingGreen > 0 && s_AT_RingRed > 0;
        bool keys = s_AT_KeyGreen > 0 && s_AT_KeyRed > 0;
        // Der Treiber drueckt nach 0.25 s: der Ring laeuft dabei um gut 60 Grad ab.
        bool arc = s_AT_TimerArcs > 0 && s_AT_ArcMax - s_AT_ArcMin > 45;
        bool bar = s_AT_BarMax - s_AT_BarMin > 0.5;
        return ring && keys && arc && bar;
    }

    static string ChefZ_AT_VisualsReport()
    {
        string r = "kreis gruen/rot=" + s_AT_RingGreen.ToString() + "/" + s_AT_RingRed.ToString() + " tasten gruen/rot=" + s_AT_KeyGreen.ToString() + "/" + s_AT_KeyRed.ToString();
        r = r + " zeitring frames=" + s_AT_TimerArcs.ToString() + " bogen=" + s_AT_ArcMin.ToString() + ".." + s_AT_ArcMax.ToString();
        r = r + " balken=" + s_AT_BarMin.ToString() + ".." + s_AT_BarMax.ToString();
        return r;
    }

    static int ChefZ_AT_Phase()
    {
        if (!s_Instance)
            return -1;
        return s_Instance.m_Phase;
    }

    static string ChefZ_AT_Report()
    {
        if (!s_Instance)
            return "keine Instanz";
        bool layout = s_Instance.m_Root != null && s_Instance.m_Canvas != null && s_Instance.m_Title != null;
        string r = "layout=" + layout.ToString() + " ring=" + s_Instance.m_RingHits.ToString() + "/" + RING_ROUNDS.ToString();
        r = r + " pfeile=" + s_Instance.m_ArrowHits.ToString() + "/" + s_Instance.m_ArrowTotal.ToString() + " wertung=" + s_Instance.m_Score.ToString();
        return r;
    }

    static bool ChefZ_AT_Ok()
    {
        if (!s_Instance)
            return false;
        bool layout = s_Instance.m_Root != null && s_Instance.m_Canvas != null && s_Instance.m_Title != null;
        bool ring = (s_Instance.m_RingHits == 2);
        bool arrows = (s_Instance.m_ArrowHits == 23);
        bool total = (s_Instance.m_ArrowTotal == 24);
        bool score = Math.AbsFloat(s_Instance.m_Score - 0.8708) < 0.01;
        return layout && ring && arrows && total && score;
    }
}

class ChefZ_AT_ClientProbe
{
    static ref ChefZ_AT_ClientProbe s_Instance;
    float m_QteWait;
    int m_OpenTries;

    // Echter Ausloeseweg: lokale Rute, Aktionsdaten und Angel-Komponente.
    ref FishingActionData m_RealFad;
    ref ChefZ_AT_FishCA m_RealCA;
    EntityAI m_RealRod;

    static void Begin()
    {
        s_Instance = new ChefZ_AT_ClientProbe();
        Print("[CHEFZTEST] client probe start");
        s_Instance.TryOpenCookbook();
    }

    static string MenuName()
    {
        UIScriptedMenu m = g_Game.GetUIManager().GetMenu();
        if (!m)
            return "keins";
        return m.ClassName();
    }

    //! Der Opener tut nichts, solange ein anderes Menue offen ist. Kurz nach
    //! dem Spawn kann das noch der Fall sein: pro Sekunde neu versuchen.
    void TryOpenCookbook()
    {
        m_OpenTries++;
        Print("[CHEFZTEST] client cookbook versuch " + m_OpenTries.ToString() + " offenesMenue=" + MenuName());
        ChefZ_CookbookMenuOpener.Show();
        ChefZ_CookbookMenu menu = ChefZ_CookbookMenu.Cast(g_Game.GetUIManager().GetMenu());
        if (menu || m_OpenTries >= 20)
        {
            g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(CheckCookbook, 3000, false);
            return;
        }
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(TryOpenCookbook, 1000, false);
    }

    static void Report(string name, bool ok, string detail)
    {
        string verdict = "FAIL";
        if (ok)
            verdict = "PASS";
        Print("[CHEFZTEST] client " + verdict + " FIX " + name + " " + detail);
    }

    void CheckCookbook()
    {
        ChefZ_CookbookMenu menu = ChefZ_CookbookMenu.Cast(g_Game.GetUIManager().GetMenu());
        int rows = -1;
        bool detail = false;
        if (menu && menu.GetLayoutRoot())
        {
            Widget root = menu.GetLayoutRoot();
            TextListboxWidget list = TextListboxWidget.Cast(root.FindAnyWidget("RecipeList"));
            if (list)
                rows = list.GetNumItems();
            detail = root.FindAnyWidget("DetailBody") != null && root.FindAnyWidget("Progress") != null;
        }
        bool open = menu != null;
        Report("cookbook.menu", open && rows > 0 && detail, "offen=" + open.ToString() + " zeilen=" + rows.ToString() + " detailfelder=" + detail.ToString() + " versuche=" + m_OpenTries.ToString() + " menue=" + MenuName());
        if (menu)
            menu.Close();

        // Mahi-Mahi hiess im deutschen Client "Goldmakrele" (Fehlerliste 27.10.2026).
        string mahi = Widget.TranslateString("#STR_CHEFZ_FISH_MAHIMAHI");
        // Die Beschreibung zeigt, in welcher Sprache der Client laeuft.
        string desc = Widget.TranslateString("#STR_CHEFZ_FISH_MAHIMAHI_DESC");
        bool notGold = mahi.IndexOf("Goldmakrele") < 0;
        bool isMahi = mahi.IndexOf("Mahi") >= 0;
        Report("fish.mahiName", notGold && isMahi, "name=" + mahi + " beschreibung=" + desc);

        ChefZ_FishingQteHud.s_ChefZ_AT_Drive = true;
        ChefZ_FishingQteBridge.RequestStart();
        m_QteWait = 0;
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(CheckQte, 1000, false);
    }

    void CheckQte()
    {
        m_QteWait = m_QteWait + 1;
        int phase = ChefZ_FishingQteHud.ChefZ_AT_Phase();
        bool done = phase >= ChefZ_FishingQteHud.PHASE_SUBMIT;
        if (!done && m_QteWait < 90)
        {
            g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(CheckQte, 1000, false);
            return;
        }
        Report("qte.hud", done && ChefZ_FishingQteHud.ChefZ_AT_Ok(), "phase=" + phase.ToString() + " nach " + m_QteWait.ToString() + " s " + ChefZ_FishingQteHud.ChefZ_AT_Report() + " (soll ring=2/3 pfeile=23/24 wertung=0.8708)");
        Report("qte.visuals", done && ChefZ_FishingQteHud.ChefZ_AT_VisualsOk(), ChefZ_FishingQteHud.ChefZ_AT_VisualsReport() + " (soll: beide Farben je Kreis und Tasten, Zeitring >45 Grad Spanne, Balken >0.5 Spanne)");
        ChefZ_FishingQteHud.s_ChefZ_AT_Drive = false;

        // Das HUD laeuft noch in SUBMIT/RESULT nach - abwarten, dann der echte Weg.
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(RealTrigger, 10000, false);
    }

    //==========================================================================
    // Echter Ausloeseweg (3.)
    //==========================================================================

    void RealTrigger()
    {
        PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
        ActionFishingNew action = ActionFishingNew.Cast(ActionManagerBase.GetAction(ActionFishingNew));
        if (!player || !action)
        {
            Report("qte.realTrigger", false, "kein Spieler oder keine ActionFishingNew registriert");
            RealDone();
            return;
        }

        // Lokale Rute mit Haken: der Client darf keine Netz-Items bauen.
        vector rodPos = player.GetPosition() + Vector(0, 0, 2);
        m_RealRod = EntityAI.Cast(g_Game.CreateObjectEx("FishingRod", rodPos, ECE_LOCAL));
        EntityAI hook = null;
        if (m_RealRod)
        {
            InventoryLocation hookLoc = new InventoryLocation();
            hookLoc.SetAttachment(m_RealRod, null, InventorySlots.GetSlotIdFromString("Hook"));
            hook = GameInventory.LocationCreateLocalEntity(hookLoc, "Hook", ECE_IN_INVENTORY, RF_DEFAULT);
        }
        if (!hook)
        {
            bool hasRod = m_RealRod != null;
            Report("qte.realTrigger", false, "lokale Rute/Haken nicht erzeugt rute=" + hasRod.ToString());
            RealDone();
            return;
        }

        ChefZ_AT_ClientRodCtx ctx = new ChefZ_AT_ClientRodCtx(new Param2<EntityAI, bool>(m_RealRod, false));

        m_RealFad = new FishingActionData();
        m_RealFad.m_Player = player;
        m_RealFad.m_MainItem = ItemBase.Cast(m_RealRod);
        m_RealFad.m_Action = action;
        m_RealFad.m_Target = new ActionTarget(null, null, -1, player.GetPosition() + Vector(2, 0, 2), 0);
        m_RealFad.m_ContextData = ctx;

        // a) Zyklus ueberlebt COT-Schnellaktionen (Ursache des Spieltest-Fehlers).
        float cycle = 6.0;
        m_RealCA = new ChefZ_AT_FishCA(cycle);
        m_RealFad.m_ActionComponent = m_RealCA;
        CAContinuousRepeat.s_ChefZ_AT_Quick = true;
        m_RealCA.Init(m_RealFad);
        CAContinuousRepeat.s_ChefZ_AT_Quick = false;
        float got = m_RealCA.ChefZ_AT_TimeToComplete();
        bool cycleOk = Math.AbsFloat(got - cycle) < 0.01;
        Report("fish.cycleVsQuickActions", cycleOk, "zyklus=" + got.ToString() + " soll=" + cycle.ToString() + " (COT setzt 0.1 -> kein Biss moeglich)");
        if (!cycleOk)
        {
            // Mit 0.1 s erreicht Execute das Zyklusende und wuerfelt ueber den
            // spielerlosen Kontext - nicht weiterlaufen.
            RealDone();
            return;
        }

        // b) Biss: Vanillas Komponente laeuft mit der Bildzeit des Spielers.
        //    Seit 27.10.2026 ("ich muss nach wie vor die Maustaste halten")
        //    startet der Biss das Minispiel SELBST - ohne Loslassen/OnEndInput.
        ChefZ_FishingQteBridge.s_StartPending = false;
        m_RealCA.SetNewSignalData(1.0, 0.9);
        int steps = 0;
        while (!m_RealFad.m_ChefZ_QteActive && steps < 3000)
        {
            m_RealCA.Execute(m_RealFad);
            steps++;
        }
        bool active = m_RealFad.m_ChefZ_QteActive;
        Report("fish.biteSignal", active, "schritte=" + steps.ToString() + " dt=" + player.GetDeltaT().ToString());
        bool pending = ChefZ_FishingQteBridge.s_StartPending;
        bool signalOff = !m_RealFad.m_IsSignalActive;
        bool noResult = m_RealFad.m_FishingResult == -1;
        bool triggerOk = active && pending;
        triggerOk = triggerOk && signalOff;
        triggerOk = triggerOk && noResult;
        Report("qte.autoStartOnBite", triggerOk, "ohne Loslassen: qteAktiv=" + active.ToString() + " brueckeStart=" + pending.ToString() + " signalAus=" + signalOff.ToString() + " ergebnis=" + m_RealFad.m_FishingResult.ToString());
        if (!triggerOk)
        {
            RealDone();
            return;
        }

        // c) Ein Klick (OnEndInput) waehrend des Spiels aendert nichts.
        int res = action.EvaluateFishingResult(m_RealFad);
        bool clickIgnored = res == -1;
        clickIgnored = clickIgnored && m_RealFad.m_ChefZ_QteActive;
        Report("qte.clickDuringQteIgnored", clickIgnored, "ergebnis=" + res.ToString() + " qteAktiv=" + m_RealFad.m_ChefZ_QteActive.ToString());

        // Zeit steht, solange das Spiel laeuft.
        int keep = m_RealCA.Execute(m_RealFad);
        Report("qte.realHold", keep == UA_PROCESSING, "execute=" + keep.ToString() + " soll=" + UA_PROCESSING.ToString());

        // d) Das HUD nimmt den Start im naechsten Bild auf und liest den Zustand
        //    der Aktion - hier die Daten der Probe.
        ChefZ_FishingQteHud.s_ChefZ_AT_Fad = m_RealFad;
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(CheckRealHud, 1500, false);
    }

    void CheckRealHud()
    {
        int phase = ChefZ_FishingQteHud.ChefZ_AT_Phase();
        bool started = phase >= ChefZ_FishingQteHud.PHASE_INTRO;
        bool running = phase < ChefZ_FishingQteHud.PHASE_SUBMIT;
        Report("qte.realHud", started && running, "phase=" + phase.ToString() + " (soll INTRO/RING nach Start ueber die Bruecke)");

        // Aktion endet (Abbruch): das HUD muss von selbst schliessen.
        m_RealFad.m_ChefZ_QteActive = false;
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(CheckRealAbort, 500, false);
    }

    void CheckRealAbort()
    {
        int phase = ChefZ_FishingQteHud.ChefZ_AT_Phase();
        Report("qte.realAbort", phase == ChefZ_FishingQteHud.PHASE_NONE, "phase=" + phase.ToString() + " (soll NONE, sobald die Aktion nicht mehr laeuft)");
        RealDone();
    }

    void RealDone()
    {
        ChefZ_FishingQteHud.s_ChefZ_AT_Fad = null;
        CAContinuousRepeat.s_ChefZ_AT_Quick = false;
        if (m_RealRod)
            g_Game.ObjectDelete(m_RealRod);
        m_RealRod = null;
        m_RealFad = null;
        m_RealCA = null;
        Print("[CHEFZTEST] client probe done");
        g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(StartGallery, 3000, false);
    }

    void StartGallery()
    {
        ChefZ_AT_Gallery.Begin();
    }
}

modded class MissionGameplay
{
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        if (ChefZ_AT_Rpc.s_ProbeRequested)
        {
            ChefZ_AT_Rpc.s_ProbeRequested = false;
            ChefZ_AT_ClientProbe.Begin();
        }
    }
}
