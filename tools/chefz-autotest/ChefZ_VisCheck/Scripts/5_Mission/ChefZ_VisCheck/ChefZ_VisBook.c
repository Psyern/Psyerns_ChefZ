//! "book x": schlaegt das Kochbuch auf und misst, ob Maus und Tasten den Spieler noch bewegen.
class ChefZ_VisBookClient
{
	protected float m_Time;
	protected int m_Stage;
	protected vector m_Pos;
	protected vector m_Cam;
	protected bool m_GroundOnly;

	//! "book ground": nur nach unten blicken und fotografieren, was vor dem Spieler liegt.
	void ChefZ_VisBookClient(string what)
	{
		m_GroundOnly = what == "ground";
	}

	protected void Measure(string tag)
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		float moved = vector.Distance(player.GetPosition(), m_Pos);
		float turned = vector.Distance(g_Game.GetCurrentCameraDirection(), m_Cam);
		Print("[VISCHECK] book " + tag + " moved=" + moved.ToString() + " turned=" + turned.ToString());
		m_Pos = player.GetPosition();
		m_Cam = g_Game.GetCurrentCameraDirection();
	}

	protected void SendKeys(string tag)
	{
		Print("[VISCHECK] look 200 " + tag);
		Print("[VISCHECK] hold " + tag);
	}

	void Update(float dt)
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player)
			return;
		m_Time += dt;
		if (m_GroundOnly)
		{
			if (m_Stage == 0 && m_Time > 15)
			{
				m_Stage = 1;
				Print("[VISCHECK] look 250 ground");
			}
			else if (m_Stage == 1 && m_Time > 18)
			{
				m_Stage = 2;
				Print("[VISCHECK] shot ground");
			}
			else if (m_Stage == 2 && m_Time > 21)
			{
				m_Stage = 3;
				Print("[VISCHECK] client done");
			}
			return;
		}
		if (m_Stage == 0 && m_Time > 15)
		{
			m_Stage = 1;
			Measure("start");
			Print("[VISCHECK] shot book_before");
			SendKeys("bookbefore");
		}
		else if (m_Stage == 1 && m_Time > 25)
		{
			m_Stage = 2;
			Measure("baseline");
			bool opened = ChefZ_CookbookOpener.OpenFor(player);
			Print("[VISCHECK] book opened=" + opened.ToString());
		}
		else if (m_Stage == 2 && m_Time > 30)
		{
			m_Stage = 3;
			UIScriptedMenu menu = g_Game.GetUIManager().GetMenu();
			Print("[VISCHECK] book menu=" + menu.ToString());
			if (menu)
			{
				float w;
				float h;
				Widget image = menu.GetLayoutRoot().FindAnyWidget("PageImage");
				image.GetScreenSize(w, h);
				Print("[VISCHECK] book image visible=" + image.IsVisible().ToString() + " w=" + w.ToString() + " h=" + h.ToString());
				float x;
				float y;
				image.GetScreenPos(x, y);
				float lineX;
				float lineY;
				menu.GetLayoutRoot().FindAnyWidget("DeviceLine").GetScreenPos(lineX, lineY);
				float bookW;
				float bookH;
				float bookX;
				float bookY;
				image.GetParent().GetScreenSize(bookW, bookH);
				image.GetParent().GetScreenPos(bookX, bookY);
				Print("[VISCHECK] book imageRight=" + (x + w).ToString() + " textLeft=" + lineX.ToString() + " imageBottom=" + (y + h).ToString() + " panelBottom=" + (bookY + bookH).ToString());
			}
			Measure("opened");
			Print("[VISCHECK] shot book_open");
			SendKeys("bookopen");
		}
		else if (m_Stage == 3 && m_Time > 40)
		{
			m_Stage = 4;
			Measure("whileopen");
			Print("[VISCHECK] shot book_after_input");
		}
		else if (m_Stage == 4 && m_Time > 43)
		{
			m_Stage = 5;
			g_Game.GetUIManager().GetMenu().Close();
		}
		else if (m_Stage == 5 && m_Time > 47)
		{
			m_Stage = 6;
			Measure("closed");
			Print("[VISCHECK] shot book_closed");
			SendKeys("bookclosed");
		}
		else if (m_Stage == 6 && m_Time > 57)
		{
			m_Stage = 7;
			Measure("afterclose");
			Print("[VISCHECK] client done");
		}
	}
}
