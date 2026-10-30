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
        foreach (TStringArray l : lines)
        {
            string mode = l.Get(0);
            string cls = l.Get(1);
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
        Print("[VISCHECK] server spawn done");
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
        if (m_ChefZ_VisCheckPhase == 0 && m_ChefZ_VisCheckTime > 30)
        {
            m_ChefZ_VisCheckPhase = 1;
            ShowInventory();
            Print("[VISCHECK] client inventory");
        }
        else if (m_ChefZ_VisCheckPhase == 1 && m_ChefZ_VisCheckTime > 45)
        {
            m_ChefZ_VisCheckPhase = 2;
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
