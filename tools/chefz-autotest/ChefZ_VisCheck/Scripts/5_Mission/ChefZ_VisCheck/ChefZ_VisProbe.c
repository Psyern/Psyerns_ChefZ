//==============================================================================
// ChefZ_VisProbe - Spielproben auf dem echten Client (31.10.2026). Zeilen der Liste:
//
//   hands <Klasse>   Server legt die Klassen der Reihe nach in die Haende; der Client
//                    stellt eine feste Kamera vor den Spieler und loggt
//                    "[VISCHECK] shot hand_<Klasse>" (vischeck_shots.ps1 fotografiert).
//   probe all        Lauftest gegen Hindernisse, Aktionen am echten Client
//                    (ActionManagerClient mit ForceTarget), Rezept, Waschbaer, Regal.
//
// Layer: 5_Mission.
//==============================================================================

//! Server: alle HAND_SECONDS die naechste Klasse in die Haende.
class ChefZ_VisHands
{
	static const int HAND_MS = 5000;
	protected PlayerBase m_Player;
	protected ref TStringArray m_Classes;
	protected int m_Next;

	void ChefZ_VisHands(PlayerBase player, TStringArray classes)
	{
		m_Player = player;
		m_Classes = classes;
		m_Next = 0;
	}

	//! Der Testspieler ueberlebt mehrere Laeufe; Hunger, Blutverlust und Infizierte
	//! blenden sonst das Bild ab.
	protected void Heal()
	{
		m_Player.SetAllowDamage(false);
		m_Player.SetHealth("", "", m_Player.GetMaxHealth("", ""));
		m_Player.SetHealth("", "Blood", m_Player.GetMaxHealth("", "Blood"));
		m_Player.SetHealth("", "Shock", m_Player.GetMaxHealth("", "Shock"));
		m_Player.GetStatEnergy().Set(m_Player.GetStatEnergy().GetMax());
		m_Player.GetStatWater().Set(m_Player.GetStatWater().GetMax());
	}

	void Tick()
	{
		Heal();
		EntityAI held = m_Player.GetHumanInventory().GetEntityInHands();
		if (held)
			g_Game.ObjectDelete(held);
		if (m_Next >= m_Classes.Count())
		{
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(Tick);
			Print("[VISCHECK] hands done");
			return;
		}
		string cls = m_Classes.Get(m_Next);
		m_Next++;
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(CreateNext, 400, false, cls);
	}

	void CreateNext(string cls)
	{
		EntityAI item = m_Player.GetHumanInventory().CreateInHands(cls);
		Print("[VISCHECK] hand " + cls + " ok=" + (item != null).ToString());
	}
}

//! Server: baut die Proben auf und fuehrt den Zeitplan.
class ChefZ_VisProbeServer
{
	static const float OBSTACLE_AHEAD = 2.5;
	static const int LANE_MS = 11000;
	//! "kit:<Klasse>" baut wie der Spieler ueber OnPlacementComplete auf.
	//! "yaw:<Grad>:<Klasse>" dreht das Hindernis (Anlauf von der Seite), "drop:<Klasse>" kommt aus der Hand.
	static ref TStringArray LANES = {"ChefZ_Beehive", "ChefZ_BeehiveDouble", "kit:ChefZ_BeehiveKit", "kit:ChefZ_BeehiveDoubleKit", "yaw:90:ChefZ_Beehive", "yaw:90:ChefZ_BeehiveDouble", "yaw:180:ChefZ_BeehiveDouble", "yaw:270:ChefZ_BeehiveDouble", "drop:ChefZ_Beehive", "drop:ChefZ_BeehiveDouble"};
	static ref TStringArray WATCH = {"ChefZ_BeehiveDouble", "ChefZ_Beehive"};

	protected PlayerBase m_Player;
	protected vector m_Origin;
	protected vector m_Dir;
	protected vector m_Right;
	protected int m_Lane;
	protected Object m_Obstacle;
	protected EntityAI m_ExtractorSmall;
	protected EntityAI m_ExtractorFull;
	protected EntityAI m_Shelf;
	protected ItemBase m_Can;
	protected ItemBase m_Powder;
	protected ItemBase m_Bottle;
	protected Object m_Cow;
	protected EntityAI m_Raccoon;
	protected vector m_RaccoonPos;

	void ChefZ_VisProbeServer(PlayerBase player)
	{
		m_Player = player;
		m_Origin = player.GetPosition();
		m_Dir = player.GetDirection();
		m_Dir[1] = 0;
		m_Dir.Normalize();
		m_Right = Vector(m_Dir[2], 0, -m_Dir[0]);
		m_Lane = 0;
	}

	protected vector Ground(vector pos)
	{
		pos[1] = g_Game.SurfaceY(pos[0], pos[2]);
		return pos;
	}

	//! "probe keep": Stock mit zehn Rahmen vor den Spieler, bleibt fuer den naechsten Lauf.
	//! "probe reuse": den gespeicherten Stock laden, vor den Spieler stellen, dagegen laufen.
	void StartPersist(string mode)
	{
		array<Object> leftovers = new array<Object>();
		array<CargoBase> leftoverCargos = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(m_Player.GetPosition(), 40.0, leftovers, leftoverCargos);
		foreach (Object leftover : leftovers)
		{
			if (leftover.IsKindOf("ChefZ_HoneyExtractor"))
				g_Game.ObjectDelete(leftover);
		}
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(RunPersist, 80000, false, mode);
	}

	void RunPersist(string mode)
	{
		vector dir = m_Player.GetDirection();
		dir[1] = 0;
		dir.Normalize();
		vector pos = Ground(m_Player.GetPosition() + dir * OBSTACLE_AHEAD);
		array<Object> objects = new array<Object>();
		array<CargoBase> cargos = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(m_Player.GetPosition(), 60.0, objects, cargos);
		EntityAI hive;
		foreach (Object o : objects)
		{
			if (o.IsKindOf("ChefZ_Beehive"))
			{
				if (mode == "keep")
					g_Game.ObjectDelete(o);
				else
					hive = EntityAI.Cast(o);
			}
		}
		if (mode == "keep")
		{
			hive = EntityAI.Cast(g_Game.CreateObjectEx("ChefZ_Beehive", pos, ECE_PLACE_ON_SURFACE));
			int frames = 0;
			for (int i = 0; i < 10; i++)
			{
				if (hive.GetInventory().CreateAttachment("ChefZ_HoneycombFrameFull"))
					frames++;
			}
			Print("[VISCHECK] persist placed frames=" + frames.ToString());
		}
		if (!hive)
		{
			Print("[VISCHECK] persist reuse: kein Stock gefunden");
			return;
		}
		int layer = 0;
		if (dBodyIsSet(hive))
			layer = dBodyGetInteractionLayer(hive);
		Print("[VISCHECK] persist " + mode + " attachments=" + hive.GetInventory().AttachmentCount().ToString() + " body=" + dBodyIsSet(hive).ToString() + " layer=" + layer.ToString());
		if (mode == "reuse")
			hive.SetPosition(pos);
	}

	void Start()
	{
		Cleanup();
		//! Der Client laeuft erst rund eine Minute nach dem Verbinden.
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextLane, 80000, false);
	}

	//! Der Testserver speichert die Welt: Reste frueherer Laeufe stoeren die Probe.
	protected void Cleanup()
	{
		EntityAI held = m_Player.GetHumanInventory().GetEntityInHands();
		if (held)
			g_Game.ObjectDelete(held);
		array<Object> objects = new array<Object>();
		array<CargoBase> cargos = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(m_Player.GetPosition(), 40.0, objects, cargos);
		TStringArray kinds = {"ChefZ_HoneyExtractor", "ChefZ_MushroomShelf", "ChefZ_Beehive", "ChefZ_ButterChurn", "WoodenCrate", "Barrel_Blue", "Animal_BosTaurusF", "ChefZ_Raccoon", "ChefZ_DeadRaccoon"};
		int removed = 0;
		foreach (Object o : objects)
		{
			foreach (string kind : kinds)
			{
				if (o.IsKindOf(kind))
				{
					g_Game.ObjectDelete(o);
					removed++;
					break;
				}
			}
		}
		Print("[VISCHECK] cleanup removed=" + removed.ToString());
	}

	//! Ein Hindernis vor den Spieler stellen. Der Server kann einen verbundenen Spieler
	//! nicht versetzen (SetPosition kommt beim Client nicht an); der Client merkt das
	//! neue Objekt und laeuft darauf zu.
	void NextLane()
	{
		if (m_Obstacle)
			g_Game.ObjectDelete(m_Obstacle);
		if (m_Lane >= LANES.Count())
		{
			Print("[VISCHECK] lanes done");
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(BuildActions, 1000, false);
			return;
		}
		string cls = LANES.Get(m_Lane);
		m_Lane++;
		vector dir = m_Player.GetDirection();
		dir[1] = 0;
		dir.Normalize();
		vector pos = Ground(m_Player.GetPosition() + dir * OBSTACLE_AHEAD);
		if (cls.IndexOf("kit:") == 0)
		{
			string kitClass = cls.Substring(4, cls.Length() - 4);
			ChefZ_BeehiveKit kit = ChefZ_BeehiveKit.Cast(g_Game.CreateObjectEx(kitClass, Ground(m_Player.GetPosition()), ECE_PLACE_ON_SURFACE));
			if (kit)
			{
				kit.OnPlacementComplete(m_Player, pos, "0 0 0");
				g_Game.ObjectDelete(kit);
			}
			array<Object> near = new array<Object>();
			array<CargoBase> nearCargo = new array<CargoBase>();
			g_Game.GetObjectsAtPosition(pos, 1.0, near, nearCargo);
			foreach (Object candidate : near)
			{
				if (candidate.IsKindOf("ChefZ_Beehive"))
					m_Obstacle = candidate;
			}
		}
		else if (cls.IndexOf("drop:") == 0)
		{
			string dropClass = cls.Substring(5, cls.Length() - 5);
			EntityAI carried = m_Player.GetHumanInventory().CreateInHands(dropClass);
			Print("[VISCHECK] drop lane " + dropClass + " inHands=" + (carried != null).ToString());
			if (carried)
			{
				bool droppedOk = m_Player.ServerDropEntity(carried);
				Print("[VISCHECK] drop lane dropped=" + droppedOk.ToString());
				g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(FinishDrop, 2500, false, carried);
				m_Obstacle = carried;
			}
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextLane, LANE_MS + 3000, false);
			return;
		}
		else if (cls.IndexOf("yaw:") == 0)
		{
			TStringArray parts = new TStringArray();
			cls.Split(":", parts);
			m_Obstacle = g_Game.CreateObjectEx(parts.Get(2), pos, ECE_PLACE_ON_SURFACE);
			if (m_Obstacle)
				m_Obstacle.SetOrientation(Vector(parts.Get(1).ToFloat(), 0, 0));
		}
		else
		{
			m_Obstacle = g_Game.CreateObjectEx(cls, pos, ECE_PLACE_ON_SURFACE);
		}
		if (m_Obstacle)
		{
			string info = " body=" + dBodyIsSet(m_Obstacle).ToString();
			if (dBodyIsSet(m_Obstacle))
			{
				int layer = dBodyGetInteractionLayer(m_Obstacle);
				info = info + " layer=" + layer.ToString() + " vsChar=" + dGetInteractionLayer(m_Obstacle, PhxInteractionLayers.CHARACTER, layer).ToString();
			}
			Print("[VISCHECK] obstacle " + cls + " dist=" + vector.Distance(m_Player.GetPosition(), pos).ToString() + info);
		}
		else
		{
			Print("[VISCHECK] obstacle FEHLT " + cls);
		}
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(NextLane, LANE_MS, false);
	}

	//! Fallengelassen: Physik protokollieren, dann vor den Spieler, weil er sonst darin steht.
	void FinishDrop(EntityAI item)
	{
		if (!item)
			return;
		string info = " body=" + dBodyIsSet(item).ToString();
		if (dBodyIsSet(item))
		{
			int layer = dBodyGetInteractionLayer(item);
			info = info + " layer=" + layer.ToString() + " vsChar=" + dGetInteractionLayer(item, PhxInteractionLayers.CHARACTER, layer).ToString() + " dynamic=" + dBodyIsDynamic(item).ToString();
		}
		Print("[VISCHECK] dropped obstacle " + item.GetType() + info);
		vector dir = m_Player.GetDirection();
		dir[1] = 0;
		dir.Normalize();
		item.SetPosition(Ground(m_Player.GetPosition() + dir * OBSTACLE_AHEAD));
	}

	protected EntityAI SpawnExtractor(vector pos, int frames, int jars)
	{
		EntityAI station = EntityAI.Cast(g_Game.CreateObjectEx("ChefZ_HoneyExtractor", pos, ECE_PLACE_ON_SURFACE));
		if (!station)
			return null;
		int made = 0;
		int i;
		for (i = 0; i < frames; i++)
		{
			if (station.GetInventory().CreateAttachment("ChefZ_HoneycombFrameUncapped"))
				made++;
		}
		for (i = 0; i < jars; i++)
		{
			if (station.GetInventory().CreateAttachment("ChefZ_EmptyJar"))
				made++;
		}
		Print("[VISCHECK] extractor attachments=" + made.ToString() + " of " + (frames + jars).ToString());
		return station;
	}

	void BuildActions()
	{
		m_Origin = m_Player.GetPosition();
		m_Dir = m_Player.GetDirection();
		m_Dir[1] = 0;
		m_Dir.Normalize();
		m_Right = Vector(m_Dir[2], 0, -m_Dir[0]);
		m_ExtractorSmall = SpawnExtractor(Ground(m_Origin + m_Right * 1.2), 1, 1);
		m_ExtractorFull = SpawnExtractor(Ground(m_Origin - m_Right * 1.2), 4, 4);

		m_Shelf = EntityAI.Cast(g_Game.CreateObjectEx("ChefZ_MushroomShelf", Ground(m_Origin - m_Dir * 3.0), ECE_PLACE_ON_SURFACE));
		if (m_Shelf)
		{
			bool emptyHands = m_Shelf.CanPutIntoHands(m_Player);
			EntityAI tray = m_Shelf.GetInventory().CreateAttachment("ChefZ_MushroomTray");
			bool loadedHands = m_Shelf.CanPutIntoHands(m_Player);
			Print("[VISCHECK] shelf empty CanPutIntoHands=" + emptyHands.ToString() + " loaded(tray=" + (tray != null).ToString() + ")=" + loadedHands.ToString());
		}
		ChefZ_ReportBehaviour("ChefZ_MushroomTray");
		ChefZ_ReportBehaviour("ChefZ_BeehiveKit");
		ChefZ_ReportBehaviour("ChefZ_BeehiveDoubleKit");

		m_Bottle = ItemBase.Cast(m_Player.GetInventory().CreateInInventory("WaterBottle"));
		if (m_Bottle)
		{
			m_Bottle.SetLiquidType(LIQUID_WATER);
			m_Bottle.SetQuantity(1000);
			Print("[VISCHECK] bottle mask has milk=" + ((m_Bottle.GetLiquidContainerMask() & LIQUID_CHEFZ_MILK) != 0).ToString());
		}

		m_RaccoonPos = Ground(m_Origin + m_Dir * 8.0);
		m_Raccoon = EntityAI.Cast(g_Game.CreateObjectEx("ChefZ_Raccoon", m_RaccoonPos, ECE_PLACE_ON_SURFACE | ECE_INITAI | ECE_CREATEPHYSICS));
		Print("[VISCHECK] raccoon spawned=" + (m_Raccoon != null).ToString());
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(KillRaccoon, 4000, false);

		Print("[VISCHECK] actions ready");
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(MixMilk, 40000, false);
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GiveCan, 50000, false);
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(FinalReport, 110000, false);
	}

	protected void ChefZ_ReportBehaviour(string cls)
	{
		ItemBase item = ItemBase.Cast(g_Game.CreateObjectEx(cls, Ground(m_Origin - m_Dir * 5.0), ECE_PLACE_ON_SURFACE));
		if (!item)
		{
			Print("[VISCHECK] behaviour FEHLT " + cls);
			return;
		}
		Print("[VISCHECK] behaviour " + cls + " twoHanded=" + item.IsTwoHandedBehaviour().ToString() + " heavy=" + item.IsHeavyBehaviour().ToString());
		g_Game.ObjectDelete(item);
	}

	void KillRaccoon()
	{
		if (m_Raccoon)
			m_Raccoon.SetHealth("", "", 0);
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ReportRaccoon, 6000, false);
	}

	void ReportRaccoon()
	{
		array<Object> objects = new array<Object>();
		array<CargoBase> cargos = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(m_RaccoonPos, 25.0, objects, cargos);
		foreach (Object o : objects)
		{
			string type = o.GetType();
			if (o.IsKindOf("DeadFox") || o.IsKindOf("Animal_VulpesVulpes"))
				Print("[VISCHECK] raccoon result " + type + " alive=" + o.IsAlive().ToString());
		}
	}

	void MixMilk()
	{
		PluginRecipesManager manager = PluginRecipesManager.Cast(GetPlugin(PluginRecipesManager));
		// CheckRecipe verlangt eine Zutat in der Hand (RecipeBase.c:487).
		m_Powder = ItemBase.Cast(m_Player.GetHumanInventory().CreateInHands("PowderedMilk"));
		if (!manager || !m_Powder || !m_Bottle)
			return;
		array<int> ids = new array<int>();
		int found = manager.GetValidRecipes(m_Powder, m_Bottle, ids, m_Player);
		Print("[VISCHECK] milk recipes found=" + found.ToString());
		float before = m_Powder.GetQuantity();
		foreach (int id : ids)
		{
			manager.PerformRecipeServer(id, m_Powder, m_Bottle, m_Player);
			break;
		}
		Print("[VISCHECK] milk bottle liquid=" + m_Bottle.GetLiquidType().ToString() + " qty=" + m_Bottle.GetQuantity().ToString() + " powder " + before.ToString() + " -> " + m_Powder.GetQuantity().ToString());
	}

	void GiveCan()
	{
		if (m_Powder)
			g_Game.ObjectDelete(m_Powder);
		g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(GiveCanNow, 800, false);
	}

	void GiveCanNow()
	{
		m_Can = ItemBase.Cast(m_Player.GetHumanInventory().CreateInHands("ChefZ_MilkCan"));
		Print("[VISCHECK] can in hands=" + (m_Can != null).ToString() + " qty=" + m_Can.GetQuantity().ToString());
		// Ohne KI: die Kuh bleibt stehen, damit der Spieler sie mit dem Fadenkreuz treffen kann.
		vector cowDir = m_Player.GetDirection();
		cowDir[1] = 0;
		cowDir.Normalize();
		m_Cow = g_Game.CreateObjectEx("Animal_BosTaurusF_Brown", Ground(m_Player.GetPosition() + cowDir * 1.8), ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS);
		if (m_Cow)
			m_Cow.SetDirection(Vector(cowDir[2], 0, -cowDir[0]));
		Print("[VISCHECK] cow=" + (m_Cow != null).ToString());
	}

	void FinalReport()
	{
		if (m_Can)
			Print("[VISCHECK] can after milking qty=" + m_Can.GetQuantity().ToString());
		ChefZ_VisCheck.Report(m_ExtractorSmall, "extractor small");
		ChefZ_VisCheck.Report(m_ExtractorFull, "extractor full");
		Print("[VISCHECK] report probe done");
	}
}

//! Client: misst den Lauftest und fragt die Aktionen am echten ActionManagerClient ab.
class ChefZ_VisProbeClient
{
	static const float WALK_SECONDS = 6.0;
	protected float m_WalkLeft;
	protected vector m_WalkStart;
	protected Object m_WalkTarget;
	protected string m_WalkType;
	protected float m_NextScan;
	protected int m_Holds;
	protected float m_WalkMinDist;
	protected float m_NextQuery;
	protected float m_Time;
	protected float m_ForcedAt;
	protected Object m_Forced;
	protected string m_ForcedTag;
	protected bool m_ActionsPhase;
	protected ref set<string> m_Started = new set<string>();

	void Update(float dt)
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;
		m_Time += dt;
		vector pos = player.GetPosition();
		if (m_WalkLeft > 0)
		{
			m_WalkLeft -= dt;
			if (m_WalkTarget)
			{
				float dist = vector.Distance(pos, m_WalkTarget.GetPosition());
				if (dist < m_WalkMinDist)
					m_WalkMinDist = dist;
			}
			if (m_WalkLeft <= 0)
				Print("[VISCHECK] walk " + m_WalkType + " traveled=" + vector.Distance(pos, m_WalkStart).ToString() + " minDist=" + m_WalkMinDist.ToString());
			return;
		}
		if (m_ActionsPhase)
		{
			if (m_Time >= m_NextQuery)
				QueryActions(player);
			return;
		}
		if (m_Time < m_NextScan)
			return;
		m_NextScan = m_Time + 0.5;
		if (FindNear(player, "ChefZ_HoneyExtractor", 2, 8))
		{
			m_ActionsPhase = true;
			m_NextQuery = m_Time + 3.0;
			return;
		}
		foreach (string cls : ChefZ_VisProbeServer.WATCH)
		{
			Object o = FindNear(player, cls, 0, 99);
			// Noch an den Fuessen (fallengelassen): erst laufen, wenn es vorn steht.
			if (o && vector.Distance(pos, o.GetPosition()) < 1.2)
				continue;
			if (o && o != m_WalkTarget)
			{
				m_WalkTarget = o;
				m_WalkType = cls;
				m_WalkStart = pos;
				m_WalkLeft = WALK_SECONDS;
				m_WalkMinDist = 1000.0;
				m_Holds++;
				Print("[VISCHECK] hold " + cls + "_" + m_Holds.ToString());
				Print("[VISCHECK] walk " + cls + " startDist=" + vector.Distance(pos, o.GetPosition()).ToString());
				return;
			}
		}
	}

	protected Object FindNear(PlayerBase player, string kind, int minAttachments, int maxAttachments)
	{
		array<Object> objects = new array<Object>();
		array<CargoBase> cargos = new array<CargoBase>();
		g_Game.GetObjectsAtPosition(player.GetPosition(), 4.0, objects, cargos);
		foreach (Object o : objects)
		{
			if (!o.IsKindOf(kind))
				continue;
			EntityAI entity = EntityAI.Cast(o);
			if (!entity)
				continue;
			int n = entity.GetInventory().AttachmentCount();
			if (n >= minAttachments && n <= maxAttachments)
				return o;
		}
		return null;
	}

	protected void Report(PlayerBase player, string tag)
	{
		ActionManagerClient manager = ActionManagerClient.Cast(player.GetActionManager());
		if (!manager)
			return;
		string line = "[VISCHECK] actions " + tag;
		EntityAI held = player.GetHumanInventory().GetEntityInHands();
		if (held)
			line = line + " hands=" + held.GetType();
		if (m_Forced)
			line = line + " dist=" + vector.Distance(player.GetPosition(), m_Forced.GetPosition()).ToString();
		array<typename> inputs = {ContinuousInteractActionInput, InteractActionInput, ContinuousDefaultActionInput, DefaultActionInput};
		foreach (typename inputType : inputs)
		{
			array<ActionBase> possible = manager.GetPossibleActions(inputType);
			line = line + " | " + inputType.ToString() + ":";
			if (!possible)
				continue;
			foreach (ActionBase action : possible)
			{
				line = line + " " + action.Type().ToString();
				if (action.Type() == ChefZ_ActionMilkCow)
					m_MilkOffered = true;
			}
		}
		Print(line);
	}

	protected void Force(PlayerBase player, Object target, string tag)
	{
		ActionManagerClient manager = ActionManagerClient.Cast(player.GetActionManager());
		if (!manager)
			return;
		m_Forced = target;
		m_ForcedTag = tag;
		if (target)
			manager.ForceTarget(target);
		else
			manager.ClearForceTarget();
	}

	//! Wie ein Spieler: Ziel anpeilen und die Taste wirklich halten (vischeck_shots.ps1).
	//! Ein Ziel darf waehrend der Aktion nicht wechseln, sonst bricht sie ab.
	protected void QueryActions(PlayerBase player)
	{
		m_NextQuery = m_Time + 1.5;
		EntityAI held = player.GetHumanInventory().GetEntityInHands();
		if (m_Stage == 0)
		{
			Object extractor = FindNear(player, "ChefZ_HoneyExtractor", 2, 2);
			if (!extractor)
				return;
			Force(player, extractor, "extractorSmall");
			m_Stage = 1;
			return;
		}
		if (m_Stage == 1)
		{
			Report(player, m_ForcedTag);
			Print("[VISCHECK] shot extractor_action");
			Print("[VISCHECK] press F 25 extractor");
			m_NextQuery = m_Time + 30.0;
			m_Stage = 2;
			return;
		}
		if (m_Stage == 2)
		{
			Force(player, null, "");
			m_Stage = 3;
			return;
		}
		Object cow = FindNear(player, "Animal_BosTaurusF", 0, 99);
		if (m_Stage == 3)
		{
			if (!held || !held.IsKindOf("ChefZ_MilkCan") || !cow)
				return;
			Report(player, "cow");
			if (m_MilkOffered)
			{
				Print("[VISCHECK] shot cow_action");
				Print("[VISCHECK] press LMB 11 milk");
				m_Stage = 4;
				m_NextQuery = m_Time + 13.0;
				return;
			}
			m_Looks++;
			Print("[VISCHECK] look 50 " + m_Looks.ToString());
			return;
		}
		if (m_Stage == 4)
		{
			Print("[VISCHECK] milk can client qty=" + ItemBase.Cast(held).GetQuantity().ToString());
			m_Stage = 5;
		}
	}

	protected bool m_MilkOffered;
	protected int m_Looks;
	protected int m_Stage;
}

//! Client: feste Kamera, ein Foto je neuem Gegenstand in der Hand.
class ChefZ_VisHandsClient
{
	protected Camera m_Camera;
	protected string m_Held;
	protected float m_HeldTime;
	protected bool m_Logged;
	protected bool m_LoggedLate;
	protected float m_Time;

	void Update(float dt)
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;
		EntityAI held = player.GetHumanInventory().GetEntityInHands();
		string type = "";
		if (held)
			type = held.GetType();
		//! Erst setzen, wenn der Spieler sicher steht: frueher steht er noch am Weltursprung.
		if (!m_Camera && m_Time > 90.0)
			Place(player);
		m_Time += dt;
		if (!m_Camera)
			return;
		if (type != m_Held)
		{
			m_Held = type;
			m_HeldTime = 0;
			m_Logged = false;
			m_LoggedLate = false;
			return;
		}
		m_HeldTime += dt;
		if (type != "" && m_Logged && !m_LoggedLate && m_HeldTime > 99.0)
		{
			m_LoggedLate = true;
			Print("[VISCHECK] shot late_" + type);
		}
		if (type != "" && !m_Logged && m_HeldTime > 2.0)
		{
			m_Logged = true;
			Print("[VISCHECK] shot hand_" + type);
			Print("[VISCHECK] state " + type + " pos=" + player.GetPosition().ToString() + " alive=" + player.IsAlive().ToString() + " cmd=" + player.GetCurrentCommandID().ToString() + " uncon=" + player.IsUnconscious().ToString());
		}
	}

	protected void Place(PlayerBase player)
	{
		vector dir = player.GetDirection();
		dir[1] = 0;
		dir.Normalize();
		vector right = Vector(dir[2], 0, -dir[0]);
		vector target = player.GetPosition() + right * 0.1;
		target[1] = target[1] + 0.95;
		vector pos = player.GetPosition() + dir * 1.7 + right * 0.5;
		pos[1] = player.GetPosition()[1] + 1.2;
		Class.CastTo(m_Camera, g_Game.CreateObject("staticcamera", pos, true));
		if (!m_Camera)
			return;
		m_Camera.LookAt(target);
		m_Camera.SetActive(true);
	}
}
