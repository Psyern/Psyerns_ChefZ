//==============================================================================
// ChefZ_VisCheck - legt Klassen aus $profile:chefz_vischeck.txt neben den ersten
// Spieler (Server) und oeffnet auf dem Client das Inventar, damit ein Bildschirmfoto
// Vorschaubilder und Groessen zeigt. Marker im Clientlog: "[VISCHECK] ...".
//
// Zeilen der Datei (eine Klasse je Zeile, # = Kommentar):
//   inv   <Klasse> [RAW|BAKED|BOILED|DRIED|BURNED]   auf den Boden direkt neben den Spieler
//   world <Klasse>                                   in eine Reihe 3 m vor den Spieler
//   attach <Klasse>                                  an das zuletzt gespawnte world-Objekt haengen
//   player <Klasse>                                  ins Spielerinventar (Reihenfolge = Ablage)
//   call  <Methode>                                  Methode am letzten world-Objekt rufen (nach 5 s)
//   report <Sekunden>                                Anbauteile des letzten world-Objekts nach N s loggen
//
// Layer: 5_Mission (MissionServer / MissionGameplay).
//==============================================================================

class ChefZ_VisCheck
{
    static const string FILE = "$profile:chefz_vischeck.txt";
    static ref ChefZ_VisCheckReporter s_Reporter;
    static ref ChefZ_VisHands s_Hands;
    static ref ChefZ_VisProbeServer s_Probe;

    static bool Read(out array<ref TStringArray> lines)
    {
        lines = new array<ref TStringArray>();
        if (!FileExist(FILE))
            return false;
        FileHandle fh = OpenFile(FILE, FileMode.READ);
        if (fh == 0)
            return false;
        string line;
        while (FGets(fh, line) >= 0)
        {
            line.Trim();
            if (line == "" || line.IndexOf("#") == 0)
                continue;
            TStringArray parts = new TStringArray();
            line.Split(" ", parts);
            TStringArray clean = new TStringArray();
            foreach (string p : parts)
            {
                p.Trim();
                if (p != "")
                    clean.Insert(p);
            }
            if (clean.Count() >= 2)
                lines.Insert(clean);
        }
        CloseFile(fh);
        return true;
    }

    static int StageFromName(string name)
    {
        if (name == "BAKED")
            return FoodStageType.BAKED;
        if (name == "BOILED")
            return FoodStageType.BOILED;
        if (name == "DRIED")
            return FoodStageType.DRIED;
        if (name == "BURNED")
            return FoodStageType.BURNED;
        return FoodStageType.RAW;
    }

    //! "drop <Klasse>": wirklich fallen lassen, Lage danach im Log (0/0 = wie gespawnt).
    static ref array<EntityAI> s_Dropped = new array<EntityAI>();
    static PlayerBase s_DropPlayer;

    static void DropNow()
    {
        foreach (EntityAI item : s_Dropped)
        {
            bool ok = s_DropPlayer.ServerDropEntity(item);
            Print("[VISCHECK] dropped " + item.GetType() + " ok=" + ok.ToString());
        }
        s_Dropped.Clear();
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(ChefZ_VisCheck.DropNow);
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ChefZ_VisCheck.DropReport, 8000, false);
    }

    static void DropReport()
    {
        array<Object> found = new array<Object>();
        g_Game.GetObjectsAtPosition(s_DropPlayer.GetPosition(), 4.0, found, null);
        foreach (Object o : found)
        {
            if (o.IsKindOf("ChefZ_CookbookItem") || o.IsKindOf("ChefZ_Perch"))
                Print("[VISCHECK] liegt " + o.GetType() + " ypr=" + o.GetOrientation().ToString() + " pos=" + o.GetPosition().ToString());
        }
    }

    static void Spawn(PlayerBase player)
    {
        array<ref TStringArray> lines;
        if (!Read(lines))
            return;
        if (!player)
            return;

        vector dir = player.GetDirection();
        dir[1] = 0;
        dir.Normalize();
        vector right = Vector(dir[2], 0, -dir[0]);
        vector origin = player.GetPosition();
        int invIndex = 0;
        int worldIndex = 0;
        EntityAI lastWorld;
        TStringArray handClasses = new TStringArray();
        g_Game.GetWorld().SetDate(2026, 6, 15, 12, 0);
        foreach (TStringArray l : lines)
        {
            string mode = l.Get(0);
            string cls = l.Get(1);
            if (mode == "preview" || mode == "localworld" || mode == "cot")
                continue;
            if (mode == "hands")
            {
                handClasses.Insert(cls);
                continue;
            }
            if (mode == "probe")
            {
                s_Probe = new ChefZ_VisProbeServer(player);
                if (cls == "all")
                    s_Probe.Start();
                else
                    s_Probe.StartPersist(cls);
                continue;
            }
            if (mode == "attach")
            {
                EntityAI att;
                if (lastWorld)
                    att = lastWorld.GetInventory().CreateAttachment(cls);
                Print("[VISCHECK] attach " + cls + " ok=" + (att != null).ToString());
                continue;
            }
            if (mode == "player")
            {
                EntityAI carried = player.GetInventory().CreateInInventory(cls);
                Print("[VISCHECK] player " + cls + " ok=" + (carried != null).ToString());
                continue;
            }
            if (mode == "drop")
            {
                EntityAI dropped = player.GetInventory().CreateInInventory(cls);
                Print("[VISCHECK] drop " + cls + " ok=" + (dropped != null).ToString());
                s_DropPlayer = player;
                if (dropped)
                    s_Dropped.Insert(dropped);
                if (dropped)
                    g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ChefZ_VisCheck.DropNow, 20000, false);
                continue;
            }
            if (mode == "call")
            {
                if (lastWorld)
                    g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLaterByName(lastWorld, cls, 5000, false);
                Print("[VISCHECK] call " + cls);
                continue;
            }
            if (mode == "report")
            {
                if (lastWorld)
                {
                    s_Reporter = new ChefZ_VisCheckReporter(lastWorld, cls.ToInt());
                    g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(s_Reporter.Tick, 15000, true);
                }
                continue;
            }
            vector pos;
            if (mode == "world")
            {
                int sideIdx = worldIndex % 5;
                int depthIdx = worldIndex / 5;
                float side = sideIdx - 2;
                float depth = 3.0 + depthIdx * 2.5;
                pos = origin + dir * depth + right * (side * 2.0);
                worldIndex++;
            }
            else
            {
                int colIdx = invIndex % 4;
                int rowIdx = invIndex / 4;
                float col = colIdx - 1.5;
                float row = rowIdx * 0.35;
                pos = origin + dir * (0.6 + row) + right * (col * 0.35);
                invIndex++;
            }
            pos[1] = g_Game.SurfaceY(pos[0], pos[2]);
            Object obj = g_Game.CreateObjectEx(cls, pos, ECE_PLACE_ON_SURFACE);
            if (!obj)
            {
                Print("[VISCHECK] FEHLT " + cls);
                continue;
            }
            if (mode == "world")
            {
                obj.SetDirection(-dir);
                lastWorld = EntityAI.Cast(obj);
            }
            Edible_Base food = Edible_Base.Cast(obj);
            if (food && l.Count() >= 3 && food.HasFoodStage())
                food.ChangeFoodStage(StageFromName(l.Get(2)));
            Print("[VISCHECK] gespawnt " + mode + " " + cls + " pos=" + pos.ToString());
        }
        if (handClasses.Count() > 0)
        {
            s_Hands = new ChefZ_VisHands(player, handClasses);
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ChefZ_VisCheck.StartHands, 120000, false);
        }
        Print("[VISCHECK] server spawn done");
    }

    //! Der Client laeuft erst rund eine Minute nach dem Verbinden.
    static void StartHands()
    {
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(s_Hands.Tick, ChefZ_VisHands.HAND_MS, true);
    }

    //! Listet die Anbauteile eines Objekts mit Menge.
    static void Report(EntityAI obj, string tag)
    {
        if (!obj)
            return;
        string line = "[VISCHECK] " + tag + " " + obj.GetType() + ":";
        int n = obj.GetInventory().AttachmentCount();
        for (int i = 0; i < n; i++)
        {
            ItemBase a = ItemBase.Cast(obj.GetInventory().GetAttachmentFromIndex(i));
            if (a)
                line = line + " " + a.GetType() + "(" + a.GetQuantity().ToString() + ")";
        }
        Print(line);
    }
}

//! Client: ein Raster aus ItemPreviewWidgets (wie Inventar und COT), je Zeile
//! "preview <Klasse>" eine Zelle mit Klassennamen. Die Objekte sind lokal
//! (ECE_LOCAL), damit keine Reste aus der Persistenz das Bild verfaelschen.
class ChefZ_VisCheckPreviewGrid
{
    static const string LAYOUT = "ChefZ_VisCheck/gui/preview_cell.layout";
    protected ref array<Widget> m_Cells = new array<Widget>();
    protected ref array<Object> m_Objects = new array<Object>();
    protected ref array<Object> m_WorldObjects = new array<Object>();

    static const int PAGE_CELLS = 24;
    protected int m_Page;
    protected int m_Total;

    //! Naechste Seite aufbauen. false, wenn keine mehr da ist.
    bool NextPage()
    {
        Clear();
        int shown = Build();
        m_Page++;
        return shown > 0;
    }

    int Page()
    {
        return m_Page;
    }

    int Build()
    {
        array<ref TStringArray> lines;
        if (!ChefZ_VisCheck.Read(lines))
            return 0;
        int index = 0;
        int seen = -1;
        int first = m_Page * PAGE_CELLS;
        int last = first + PAGE_CELLS;
        foreach (TStringArray l : lines)
        {
            if (l.Get(0) == "localworld")
            {
                SpawnLocalWorld(l.Get(1));
                continue;
            }
            if (l.Get(0) != "preview")
                continue;
            seen++;
            if (seen < first || seen >= last)
                continue;
            string cls = l.Get(1);
            Widget cell = g_Game.GetWorkspace().CreateWidgets(LAYOUT);
            if (!cell)
            {
                Print("[VISCHECK] preview layout fehlt");
                return index;
            }
            int col = index % 8;
            int row = index / 8;
            cell.SetPos(40 + col * 230, 60 + row * 260);
            TextWidget label = TextWidget.Cast(cell.FindAnyWidget("Label"));
            if (label)
                label.SetText(cls);
            ItemPreviewWidget preview = ItemPreviewWidget.Cast(cell.FindAnyWidget("Preview"));
            Object obj = g_Game.CreateObjectEx(cls, vector.Zero, ECE_LOCAL);
            EntityAI item = EntityAI.Cast(obj);
            // Optionale dritte Spalte: Textur fuer Selektion 0, um Textur und Modell zu tauschen.
            int columns = l.Count();
            bool swapTexture = columns >= 3;
            if (!item)
                swapTexture = false;
            if (swapTexture)
            {
                item.SetObjectTexture(0, l.Get(2));
                if (label)
                    label.SetText(cls + " +tex");
            }
            if (preview && item)
                preview.SetItem(item);
            vector minMax[2];
            float radius = 0;
            if (obj)
                radius = obj.ClippingInfo(minMax);
            Print("[VISCHECK] preview " + cls + " item=" + (item != null).ToString() + " radius=" + radius.ToString() + " min=" + minMax[0].ToString() + " max=" + minMax[1].ToString());
            m_Cells.Insert(cell);
            if (obj)
                m_Objects.Insert(obj);
            index++;
        }
        return index;
    }

    //! "localworld <Klasse>": lokales Objekt in einer Reihe 1.5 m vor der Kamera.
    protected void SpawnLocalWorld(string cls)
    {
        vector dir = g_Game.GetCurrentCameraDirection();
        dir[1] = 0;
        dir.Normalize();
        vector right = Vector(dir[2], 0, -dir[0]);
        int n = m_WorldObjects.Count();
        int slot = n % 6;
        int layer = n / 6;
        float side = slot * 0.3;
        side = side - 0.75;
        float height = layer * 0.4;
        vector pos = g_Game.GetCurrentCameraPosition() + dir * 1.5 + right * side;
        pos[1] = g_Game.SurfaceY(pos[0], pos[2]) + 0.6 + height;
        Object obj = g_Game.CreateObjectEx(cls, pos, ECE_LOCAL | ECE_NOLIFETIME);
        if (obj)
        {
            obj.SetPosition(pos);
            m_WorldObjects.Insert(obj);
        }
        Print("[VISCHECK] localworld " + cls + " ok=" + (obj != null).ToString() + " pos=" + pos.ToString());
    }

    void Clear()
    {
        foreach (Widget w : m_Cells)
        {
            if (w)
                w.Unlink();
        }
        m_Cells.Clear();
        foreach (Object o : m_Objects)
        {
            if (o)
                g_Game.ObjectDelete(o);
        }
        m_Objects.Clear();
    }
}

//! Loggt alle 15 s die Anbauteile; nach N s als "[VISCHECK] report" (Endbericht).
class ChefZ_VisCheckReporter
{
    protected EntityAI m_Target;
    protected int m_Left;

    void ChefZ_VisCheckReporter(EntityAI target, int seconds)
    {
        m_Target = target;
        m_Left = seconds;
    }

    void Tick()
    {
        m_Left = m_Left - 15;
        if (m_Left > 0)
        {
            ChefZ_VisCheck.Report(m_Target, "stand");
            return;
        }
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(Tick);
        ChefZ_VisCheck.Report(m_Target, "report");
    }
}

modded class MissionServer
{
    protected bool m_ChefZ_VisCheckDone;

    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        if (m_ChefZ_VisCheckDone)
            return;
        m_ChefZ_VisCheckDone = true;
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ChefZ_VisCheck.Spawn, 8000, false, player);
    }
}

modded class MissionGameplay
{
    protected float m_ChefZ_VisCheckTime;
    protected int m_ChefZ_VisCheckPhase;
    protected ref ChefZ_VisCheckPreviewGrid m_ChefZ_VisCheckGrid;
    protected ref ChefZ_VisProbeClient m_ChefZ_VisProbe;
    protected ref ChefZ_VisHandsClient m_ChefZ_VisHands;
    protected float m_ChefZ_VisPageTime;
    protected bool m_ChefZ_VisPaged;
    protected float m_ChefZ_VisEndAt;

    //! Neue Listenzeilen: hands, probe, und mehrseitige Vorschau ("page" statt einmaligem Bild).
#ifdef JM_COT
    protected ref ChefZ_VisCotClient m_ChefZ_VisCot;
#endif

    protected void ChefZ_VisUpdateAudit(float timeslice)
    {
        if (!m_ChefZ_VisProbe && !m_ChefZ_VisHands && !m_ChefZ_VisPaged)
        {
            array<ref TStringArray> lines;
            if (!ChefZ_VisCheck.Read(lines))
                return;
            foreach (TStringArray l : lines)
            {
                string mode = l.Get(0);
                if (mode == "probe" && !m_ChefZ_VisProbe)
                    m_ChefZ_VisProbe = new ChefZ_VisProbeClient();
                if (mode == "hands" && !m_ChefZ_VisHands)
                    m_ChefZ_VisHands = new ChefZ_VisHandsClient();
                if (mode == "page")
                    m_ChefZ_VisPaged = true;
#ifdef JM_COT
                if (mode == "cot" && !m_ChefZ_VisCot)
                    m_ChefZ_VisCot = new ChefZ_VisCotClient(l.Get(1));
#endif
            }
        }
#ifdef JM_COT
        if (m_ChefZ_VisCot)
            m_ChefZ_VisCot.Update(timeslice);
#endif
        if (m_ChefZ_VisProbe)
            m_ChefZ_VisProbe.Update(timeslice);
        if (m_ChefZ_VisHands)
            m_ChefZ_VisHands.Update(timeslice);
        if (m_ChefZ_VisPaged)
        {
            if (m_ChefZ_VisCheckTime < 30)
                return;
            m_ChefZ_VisPageTime -= timeslice;
            if (m_ChefZ_VisPageTime > 0)
                return;
            m_ChefZ_VisPageTime = 6.0;
            if (!m_ChefZ_VisCheckGrid)
                m_ChefZ_VisCheckGrid = new ChefZ_VisCheckPreviewGrid();
            if (m_ChefZ_VisCheckGrid.NextPage())
            {
                g_Game.GetCallQueue(CALL_CATEGORY_GUI).CallLater(ChefZ_VisShot, 4000, false, m_ChefZ_VisCheckGrid.Page());
            }
            else
            {
                m_ChefZ_VisPaged = false;
                m_ChefZ_VisCheckPhase = 3;
                Print("[VISCHECK] client done");
            }
        }
    }

    protected void ChefZ_VisShot(int page)
    {
        Print("[VISCHECK] shot preview_" + page.ToString());
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        if (m_ChefZ_VisCheckPhase >= 3)
            return;
        if (!g_Game.GetPlayer())
            return;
        if (m_ChefZ_VisCheckPhase == 0 && !FileExist(ChefZ_VisCheck.FILE))
        {
            m_ChefZ_VisCheckPhase = 3;
            return;
        }
        m_ChefZ_VisCheckTime += timeslice;
        if (m_ChefZ_VisCheckTime > 12)
            ChefZ_VisUpdateAudit(timeslice);
        if (m_ChefZ_VisPaged || m_ChefZ_VisProbe || m_ChefZ_VisHands)
            return;
        if (m_ChefZ_VisCheckPhase == 0 && m_ChefZ_VisCheckTime > 30)
        {
            m_ChefZ_VisCheckPhase = 1;
            m_ChefZ_VisCheckGrid = new ChefZ_VisCheckPreviewGrid();
            int cells = m_ChefZ_VisCheckGrid.Build();
            if (cells == 0)
                ShowInventory();
            Print("[VISCHECK] client inventory previews=" + cells.ToString());
        }
        else if (m_ChefZ_VisCheckPhase == 1 && m_ChefZ_VisCheckTime > 45)
        {
            m_ChefZ_VisCheckPhase = 2;
            m_ChefZ_VisCheckGrid.Clear();
            HideInventory();
            Print("[VISCHECK] client world");
        }
        else if (m_ChefZ_VisCheckPhase == 2 && m_ChefZ_VisCheckTime > 55)
        {
            m_ChefZ_VisCheckPhase = 3;
            Print("[VISCHECK] client done");
        }
    }
}
