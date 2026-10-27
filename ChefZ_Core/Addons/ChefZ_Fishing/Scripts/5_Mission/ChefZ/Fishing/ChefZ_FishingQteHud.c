//==============================================================================
// ChefZ_FishingQteHud - das Angel-Minispiel auf dem Bildschirm
//
// 27.10.2026. Ablauf und Netz: ChefZ_FishingQte.c / ChefZ_FishingQteHooks.c.
//
//   1. Kreisdiagramm, drei Durchgaenge: ein Zeiger laeuft um den Kreis, die
//      goldene Zone mit [Leertaste] treffen. Zone wird kleiner, Zeiger schneller.
//   2. Pfeiltasten, drei Runden mit 4, 8 und 12 Tasten. Um die aktuelle Taste
//      laeuft ein Zeitring ab, darueber ein Balken fuer die ganze Runde.
//      Richtig = gruen, falsch oder zu spaet = rot.
//
// Alles wird auf EINEN CanvasWidget gezeichnet (Linien), keine Texturen. Kein
// Menue: das Spiel laeuft neben der Angelaktion her, die Eingaben kommen aus
// eigenen Inputs (Scripts/Data/Inputs.xml), Bewegung ist solange gesperrt.
//
// Layer: 5_Mission. Bildlauf aus MissionGameplay.OnUpdate (unten).
//==============================================================================

class ChefZ_FishingQteHud
{
    static const string LAYOUT = "ChefZ_Fishing/GUI/layouts/chefz_fishing_qte.layout";

    static const int PHASE_NONE        = 0;
    static const int PHASE_INTRO       = 1;
    static const int PHASE_RING        = 2;
    static const int PHASE_ROUND_INTRO = 3;
    static const int PHASE_ARROWS      = 4;
    static const int PHASE_SUBMIT      = 5;
    static const int PHASE_RESULT      = 6;

    static const int RING_ROUNDS = 3;
    static const int ARROW_ROUNDS = 3;

    static const int DIR_UP    = 0;
    static const int DIR_RIGHT = 1;
    static const int DIR_DOWN  = 2;
    static const int DIR_LEFT  = 3;

    static const int KEY_PENDING = 0;
    static const int KEY_OK      = 1;
    static const int KEY_FAIL    = 2;

    static const float FLASH_SEC = 0.4;

    protected static ref ChefZ_FishingQteHud s_Instance;

    protected Widget       m_Root;
    protected CanvasWidget m_Canvas;
    protected TextWidget   m_Title;
    protected TextWidget   m_Info;

    protected int   m_Phase;
    protected float m_PhaseTime;

    // Kreisdiagramm
    protected int   m_RingRound;
    protected int   m_RingHits;
    protected float m_NeedleDeg;
    protected float m_NeedleTravel;
    protected float m_ZoneStartDeg;
    protected float m_ZoneWidthDeg;
    protected float m_NeedleSpeed;
    protected float m_RingFlash;
    protected bool  m_RingFlashOk;

    // Pfeiltasten
    protected int   m_ArrowRound;
    protected int   m_ArrowHits;
    protected int   m_ArrowTotal;
    protected int   m_KeyIndex;
    protected float m_KeyTimeLeft;
    protected float m_KeyTime;
    protected float m_KeyFlash;
    protected ref array<int> m_Sequence;
    protected ref array<int> m_KeyState;

    protected float m_Score;

    void ChefZ_FishingQteHud()
    {
        m_Sequence = new array<int>();
        m_KeyState = new array<int>();
    }

    void ~ChefZ_FishingQteHud()
    {
        DestroyWidgets();
    }

    //==========================================================================
    // Einstieg
    //==========================================================================

    static void Tick(float dt)
    {
        if (!g_Game || g_Game.IsDedicatedServer())
            return;

        if (ChefZ_FishingQteBridge.s_StartPending)
        {
            ChefZ_FishingQteBridge.s_StartPending = false;
            if (!s_Instance)
                s_Instance = new ChefZ_FishingQteHud();
            s_Instance.Start();
        }

        if (s_Instance)
            s_Instance.Update(dt);
    }

    protected void Start()
    {
        if (!m_Root)
        {
            m_Root = g_Game.GetWorkspace().CreateWidgets(LAYOUT);
            if (!m_Root)
            {
                ChefZ_Log.Error(ChefZ_LogChannel.FISHING, "Angel-QTE: Layout " + LAYOUT + " nicht ladbar, kein Minispiel.");
                return;
            }
            Class.CastTo(m_Canvas, m_Root.FindAnyWidget("Canvas"));
            Class.CastTo(m_Title, m_Root.FindAnyWidget("Title"));
            Class.CastTo(m_Info, m_Root.FindAnyWidget("Info"));
        }
        m_Root.Show(true);

        g_Game.GetMission().AddActiveInputExcludes({"movement", "stances", "inventory"});

        m_RingRound = 0;
        m_RingHits = 0;
        m_ArrowRound = 0;
        m_ArrowHits = 0;
        m_ArrowTotal = 0;
        m_Score = 0;
        SetPhase(PHASE_INTRO);
    }

    protected void Stop()
    {
        if (m_Phase != PHASE_NONE && g_Game.GetMission())
            g_Game.GetMission().RemoveActiveInputExcludes({"movement", "stances", "inventory"}, false);
        m_Phase = PHASE_NONE;
        if (m_Root)
            m_Root.Show(false);
        if (m_Canvas)
            m_Canvas.Clear();
    }

    protected void DestroyWidgets()
    {
        if (m_Root)
            m_Root.Unlink();
        m_Root = null;
    }

    protected void SetPhase(int phase)
    {
        m_Phase = phase;
        m_PhaseTime = 0;
    }

    //==========================================================================
    // Bildlauf
    //==========================================================================

    protected void Update(float dt)
    {
        if (m_Phase == PHASE_NONE)
            return;

        m_PhaseTime = m_PhaseTime + dt;

        // Aktion abgebrochen (Bewegung, Schaden, Tod): Spiel weg, ohne Ergebnis.
        if (m_Phase < PHASE_SUBMIT && !FishingStillRunning())
        {
            Stop();
            return;
        }

        switch (m_Phase)
        {
            case PHASE_INTRO:
                m_Title.SetText(Tr("STR_CHEFZ_QTE_BITE", "A bite!"));
                m_Info.SetText(Tr("STR_CHEFZ_QTE_RING_HINT", "Hit the golden zone with [Space]"));
                if (m_PhaseTime >= 1.0)
                    BeginRingRound();
                break;
            case PHASE_RING:
                UpdateRing(dt);
                break;
            case PHASE_ROUND_INTRO:
                int roundNo = m_ArrowRound + 1;
                int roundKeys = ArrowCount(m_ArrowRound);
                m_Title.SetText(string.Format(Tr("STR_CHEFZ_QTE_ROUND", "Round %1 - %2 keys"), roundNo.ToString(), roundKeys.ToString()));
                m_Info.SetText(Tr("STR_CHEFZ_QTE_ARROWS_HINT", "Press the arrow keys in the order shown"));
                if (m_PhaseTime >= 1.0)
                    BeginArrowRound();
                break;
            case PHASE_ARROWS:
                UpdateArrows(dt);
                break;
            case PHASE_SUBMIT:
                m_Title.SetText(Tr("STR_CHEFZ_QTE_WAIT", "The fish is fighting..."));
                int percent = Math.Round(m_Score * 100);
                m_Info.SetText(string.Format(Tr("STR_CHEFZ_QTE_SCORE", "Score: %1%"), percent.ToString()));
                if (ChefZ_FishingQteBridge.s_Result != -1 || m_PhaseTime > 6.0)
                    SetPhase(PHASE_RESULT);
                break;
            case PHASE_RESULT:
                if (ChefZ_FishingQteBridge.s_Result == 1)
                    m_Title.SetText(string.Format(Tr("STR_CHEFZ_QTE_CAUGHT", "Caught: %1"), ChefZ_FishingQteBridge.s_ResultName));
                else
                    m_Title.SetText(Tr("STR_CHEFZ_QTE_LOST", "The fish got away"));
                if (m_PhaseTime >= 2.5)
                    Stop();
                break;
        }

        Draw();
    }

    protected bool FishingStillRunning()
    {
        PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
        if (!player || !player.IsAlive() || !player.GetActionManager())
            return false;
        FishingActionData fad = FishingActionData.Cast(player.GetActionManager().ChefZ_GetCurrentActionData());
        return fad && fad.m_ChefZ_QteActive;
    }

    //==========================================================================
    // Kreisdiagramm
    //==========================================================================

    protected void BeginRingRound()
    {
        float widths[3] = {60.0, 44.0, 30.0};
        float speeds[3] = {190.0, 250.0, 320.0};
        m_ZoneWidthDeg = widths[m_RingRound];
        m_NeedleSpeed = speeds[m_RingRound];
        m_ZoneStartDeg = Math.RandomFloatInclusive(0, 360);
        // Zeiger beginnt der Zone gegenueber, damit nichts geschenkt ist.
        m_NeedleDeg = NormDeg(m_ZoneStartDeg + 180);
        m_NeedleTravel = 0;
        m_RingFlash = 0;
        SetPhase(PHASE_RING);
    }

    protected void UpdateRing(float dt)
    {
        m_Title.SetText(string.Format(Tr("STR_CHEFZ_QTE_RING", "Hits %1/%2"), m_RingHits.ToString(), RING_ROUNDS.ToString()));
        m_Info.SetText(Tr("STR_CHEFZ_QTE_RING_HINT", "Hit the golden zone with [Space]"));

        if (m_RingFlash > 0)
        {
            m_RingFlash = m_RingFlash - dt;
            if (m_RingFlash <= 0)
                NextRingRound();
            return;
        }

        float step = m_NeedleSpeed * dt;
        m_NeedleDeg = NormDeg(m_NeedleDeg + step);
        m_NeedleTravel = m_NeedleTravel + step;

        if (Pressed("UAChefZQteHit"))
        {
            ResolveRing(InZone(m_NeedleDeg));
            return;
        }

        // Zwei volle Umlaeufe ohne Druck = verpasst.
        if (m_NeedleTravel >= 720)
            ResolveRing(false);
    }

    protected void ResolveRing(bool ok)
    {
        if (ok)
            m_RingHits++;
        m_RingFlashOk = ok;
        m_RingFlash = FLASH_SEC;
    }

    protected void NextRingRound()
    {
        m_RingRound++;
        if (m_RingRound < RING_ROUNDS)
        {
            BeginRingRound();
            return;
        }
        m_ArrowRound = 0;
        SetPhase(PHASE_ROUND_INTRO);
    }

    protected bool InZone(float deg)
    {
        float diff = NormDeg(deg - m_ZoneStartDeg);
        return diff <= m_ZoneWidthDeg;
    }

    //==========================================================================
    // Pfeiltasten
    //==========================================================================

    static int ArrowCount(int round)
    {
        return 4 + round * 4;
    }

    protected void BeginArrowRound()
    {
        float times[3] = {1.6, 1.3, 1.1};
        m_KeyTime = times[m_ArrowRound];
        int n = ArrowCount(m_ArrowRound);
        m_Sequence.Clear();
        m_KeyState.Clear();
        for (int i = 0; i < n; i++)
        {
            m_Sequence.Insert(Math.RandomInt(0, 4));
            m_KeyState.Insert(KEY_PENDING);
        }
        m_ArrowTotal = m_ArrowTotal + n;
        m_KeyIndex = 0;
        m_KeyTimeLeft = m_KeyTime;
        m_KeyFlash = 0;
        SetPhase(PHASE_ARROWS);
    }

    protected void UpdateArrows(float dt)
    {
        int roundNo = m_ArrowRound + 1;
        int roundKeys = m_Sequence.Count();
        m_Title.SetText(string.Format(Tr("STR_CHEFZ_QTE_ROUND", "Round %1 - %2 keys"), roundNo.ToString(), roundKeys.ToString()));
        m_Info.SetText("");

        if (m_KeyFlash > 0)
            m_KeyFlash = m_KeyFlash - dt;

        if (m_KeyIndex >= m_Sequence.Count())
        {
            // Letzte Taste noch kurz zeigen, dann weiter.
            if (m_KeyFlash <= 0)
                NextArrowRound();
            return;
        }

        m_KeyTimeLeft = m_KeyTimeLeft - dt;

        int pressed = PressedDirection();
        if (pressed != -1)
        {
            ResolveKey(pressed == m_Sequence.Get(m_KeyIndex));
            return;
        }
        if (m_KeyTimeLeft <= 0)
            ResolveKey(false);
    }

    protected void ResolveKey(bool ok)
    {
        if (ok)
        {
            m_KeyState.Set(m_KeyIndex, KEY_OK);
            m_ArrowHits++;
        }
        else
        {
            m_KeyState.Set(m_KeyIndex, KEY_FAIL);
        }
        m_KeyFlash = FLASH_SEC;
        m_KeyIndex++;
        m_KeyTimeLeft = m_KeyTime;
    }

    protected void NextArrowRound()
    {
        m_ArrowRound++;
        if (m_ArrowRound < ARROW_ROUNDS)
        {
            SetPhase(PHASE_ROUND_INTRO);
            return;
        }
        Finish();
    }

    protected void Finish()
    {
        // float zuerst: int / int waere Ganzzahldivision (2 / 3 = 0).
        float ring = m_RingHits;
        ring = ring / RING_ROUNDS;
        float arrows = 0;
        if (m_ArrowTotal > 0)
        {
            arrows = m_ArrowHits;
            arrows = arrows / m_ArrowTotal;
        }
        m_Score = Math.Clamp(ring * ChefZ_FishingQteConst.WEIGHT_RING + arrows * ChefZ_FishingQteConst.WEIGHT_ARROWS, 0, 1);

        ChefZ_FishingQteBridge.s_Result = -1;
        SetPhase(PHASE_SUBMIT);

        PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
        if (!player)
            return;

        if (!g_Game.IsMultiplayer())
        {
            ChefZ_FishingQteServer.SubmitScore(player, m_Score);
            return;
        }

        Param1<float> data = new Param1<float>(m_Score);
        g_Game.RPCSingleParam(player, ChefZ_FishingQteRPC.SCORE, data, true);
    }

    //==========================================================================
    // Eingabe
    //==========================================================================

    protected bool Pressed(string inputName)
    {
        UAInputAPI api = GetUApi();
        if (!api)
            return false;
        UAInput input = api.GetInputByName(inputName);
        return input && input.LocalPress();
    }

    protected int PressedDirection()
    {
        if (Pressed("UAChefZQteUp"))
            return DIR_UP;
        if (Pressed("UAChefZQteRight"))
            return DIR_RIGHT;
        if (Pressed("UAChefZQteDown"))
            return DIR_DOWN;
        if (Pressed("UAChefZQteLeft"))
            return DIR_LEFT;
        return -1;
    }

    //==========================================================================
    // Zeichnen
    //==========================================================================

    protected void Draw()
    {
        if (!m_Canvas)
            return;
        m_Canvas.Clear();

        if (m_Phase == PHASE_RING || m_Phase == PHASE_INTRO)
            DrawRing();
        else if (m_Phase == PHASE_ARROWS)
            DrawArrows();
    }

    protected void DrawRing()
    {
        float w;
        float h;
        m_Canvas.GetScreenSize(w, h);
        float cx = w * 0.5;
        float cy = h * 0.45;
        float r = h * 0.12;

        // Grund, Zone, Zeiger
        DrawArc(cx, cy, r, 0, 360, h * 0.012, ARGB(160, 30, 30, 30));
        DrawArc(cx, cy, r, 0, 360, h * 0.003, ARGB(200, 220, 220, 220));

        int zoneColor = ARGB(255, 232, 190, 70);
        if (m_RingFlash > 0)
        {
            if (m_RingFlashOk)
                zoneColor = ARGB(255, 70, 220, 90);
            else
                zoneColor = ARGB(255, 230, 60, 50);
        }
        if (m_Phase == PHASE_RING)
            DrawArc(cx, cy, r, m_ZoneStartDeg, m_ZoneWidthDeg, h * 0.02, zoneColor);

        float rad = (m_NeedleDeg - 90) * Math.DEG2RAD;
        float nx = cx + Math.Cos(rad) * r * 1.12;
        float ny = cy + Math.Sin(rad) * r * 1.12;
        m_Canvas.DrawLine(cx, cy, nx, ny, h * 0.005, ARGB(255, 255, 255, 255));
        m_Canvas.DrawLine(cx - h * 0.008, cy, cx + h * 0.008, cy, h * 0.016, ARGB(255, 255, 255, 255));

        // Trefferpunkte unter dem Kreis
        for (int i = 0; i < RING_ROUNDS; i++)
        {
            int dotColor = ARGB(160, 80, 80, 80);
            if (i < m_RingHits)
                dotColor = ARGB(255, 70, 220, 90);
            float dx = cx + (i - 1) * h * 0.035;
            float dy = cy + r + h * 0.05;
            m_Canvas.DrawLine(dx - h * 0.008, dy, dx + h * 0.008, dy, h * 0.016, dotColor);
        }
    }

    protected void DrawArrows()
    {
        float w;
        float h;
        m_Canvas.GetScreenSize(w, h);

        int n = m_Sequence.Count();
        float s = h * 0.055;
        float gap = s * 0.45;
        float rowW = n * s + (n - 1) * gap;
        float x0 = (w - rowW) * 0.5;
        float cy = h * 0.42;

        // Rundenbalken: was von der ganzen Runde noch uebrig ist.
        float left = 0;
        if (n > 0)
        {
            float current = 0;
            if (m_KeyIndex < n)
                current = Math.Clamp(m_KeyTimeLeft / m_KeyTime, 0, 1);
            left = (n - m_KeyIndex - 1 + current) / n;
            if (m_KeyIndex >= n)
                left = 0;
        }
        float barY = cy - s * 1.35;
        float barH = s * 0.18;
        m_Canvas.DrawLine(x0, barY, x0 + rowW, barY, barH, ARGB(170, 30, 30, 30));
        if (left > 0)
            m_Canvas.DrawLine(x0, barY, x0 + rowW * left, barY, barH, TimeColor(left));

        for (int i = 0; i < n; i++)
        {
            float kx = x0 + i * (s + gap) + s * 0.5;
            DrawKey(kx, cy, s, i);
        }
    }

    protected void DrawKey(float cx, float cy, float s, int index)
    {
        int state = m_KeyState.Get(index);
        bool current = index == m_KeyIndex;
        bool justResolved = index == m_KeyIndex - 1 && m_KeyFlash > 0;

        int fill = ARGB(200, 40, 40, 40);
        if (current)
            fill = ARGB(230, 75, 75, 75);
        if (state == KEY_OK)
        {
            fill = ARGB(200, 40, 120, 55);
            if (justResolved)
                fill = ARGB(255, 80, 235, 100);
        }
        else if (state == KEY_FAIL)
        {
            fill = ARGB(200, 130, 40, 35);
            if (justResolved)
                fill = ARGB(255, 245, 70, 55);
        }

        float half = s * 0.5;
        m_Canvas.DrawLine(cx - half, cy, cx + half, cy, s, fill);

        int border = ARGB(255, 150, 150, 150);
        if (current)
            border = ARGB(255, 255, 255, 255);
        float bw = s * 0.05;
        m_Canvas.DrawLine(cx - half, cy - half, cx + half, cy - half, bw, border);
        m_Canvas.DrawLine(cx - half, cy + half, cx + half, cy + half, bw, border);
        m_Canvas.DrawLine(cx - half, cy - half, cx - half, cy + half, bw, border);
        m_Canvas.DrawLine(cx + half, cy - half, cx + half, cy + half, bw, border);

        DrawArrowGlyph(cx, cy, s, m_Sequence.Get(index), ARGB(255, 245, 245, 245));

        // Zeitring um die aktuelle Taste: laeuft im Uhrzeigersinn ab.
        if (current)
        {
            float frac = Math.Clamp(m_KeyTimeLeft / m_KeyTime, 0, 1);
            float rr = s * 0.86;
            DrawArc(cx, cy, rr, 0, 360, s * 0.09, ARGB(140, 20, 20, 20));
            if (frac > 0)
                DrawArc(cx, cy, rr, 0, 360 * frac, s * 0.09, TimeColor(frac));
        }
    }

    //! Pfeil nach oben, um dir * 90 Grad gedreht.
    protected void DrawArrowGlyph(float cx, float cy, float s, int dir, int color)
    {
        float ang = dir * 90 * Math.DEG2RAD;
        float c = Math.Cos(ang);
        float sn = Math.Sin(ang);
        float lw = s * 0.09;

        // Lokale Punkte fuer "oben" (y nach unten): Schaft und zwei Spitzenlinien.
        float tipX = 0;
        float tipY = -0.28 * s;
        float tailX = 0;
        float tailY = 0.26 * s;
        float lX = -0.2 * s;
        float lY = -0.06 * s;
        float rX = 0.2 * s;
        float rY = -0.06 * s;

        float ax = cx + tipX * c - tipY * sn;
        float ay = cy + tipX * sn + tipY * c;
        float bx = cx + tailX * c - tailY * sn;
        float by = cy + tailX * sn + tailY * c;
        float lx = cx + lX * c - lY * sn;
        float ly = cy + lX * sn + lY * c;
        float rx = cx + rX * c - rY * sn;
        float ry = cy + rX * sn + rY * c;

        m_Canvas.DrawLine(bx, by, ax, ay, lw, color);
        m_Canvas.DrawLine(ax, ay, lx, ly, lw, color);
        m_Canvas.DrawLine(ax, ay, rx, ry, lw, color);
    }

    //! Kreisbogen ab startDeg (0 = oben, im Uhrzeigersinn) ueber spanDeg.
    protected void DrawArc(float cx, float cy, float r, float startDeg, float spanDeg, float width, int color)
    {
        int segments = Math.Ceil(spanDeg / 6.0);
        if (segments < 2)
            segments = 2;
        float stepDeg = spanDeg / segments;
        float prevRad = (startDeg - 90) * Math.DEG2RAD;
        float px = cx + Math.Cos(prevRad) * r;
        float py = cy + Math.Sin(prevRad) * r;
        for (int i = 1; i <= segments; i++)
        {
            float rad = (startDeg + stepDeg * i - 90) * Math.DEG2RAD;
            float nx = cx + Math.Cos(rad) * r;
            float ny = cy + Math.Sin(rad) * r;
            m_Canvas.DrawLine(px, py, nx, ny, width, color);
            px = nx;
            py = ny;
        }
    }

    //! Gruen -> Gelb -> Rot mit ablaufender Zeit.
    protected int TimeColor(float frac)
    {
        if (frac > 0.5)
            return ARGB(255, 80, 220, 90);
        if (frac > 0.25)
            return ARGB(255, 240, 200, 60);
        return ARGB(255, 240, 70, 55);
    }

    static float NormDeg(float deg)
    {
        float d = deg;
        while (d < 0)
            d = d + 360;
        while (d >= 360)
            d = d - 360;
        return d;
    }

    static string Tr(string key, string fallback)
    {
        string text = Widget.TranslateString("#" + key);
        if (text == "" || text == key || text == "#" + key)
            return fallback;
        return text;
    }
}

// modded class MissionGameplay
// Der Bildlauf des HUDs, wie ChefZ_CookbookInput: super zuerst, danach ein
// statischer Aufruf. Ohne laufendes Minispiel kehrt Tick sofort zurueck.
modded class MissionGameplay
{
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        ChefZ_FishingQteHud.Tick(timeslice);
    }
}
