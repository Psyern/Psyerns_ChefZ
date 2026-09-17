//==============================================================================
// ChefZ_Milking - die Kanne, die Kuh und die Sperre dazwischen
//
// Drei kleine Klassen, die zusammengehoeren und deshalb in einer Datei stehen:
// das Item, das die Aktion traegt, das Tier, das sie annimmt, und die
// Registrierung, ohne die beides wirkungslos waere.
//
// ---------------------------------------------------------------------------
// WARUM EINE SPERRE SEIN MUSS
// ---------------------------------------------------------------------------
// Ohne sie ist eine Kuh eine Milchquelle ohne Boden: acht Sekunden je Gang,
// beliebig oft, und zwei Milch werden am Butterfass zu einer Sahne, vier zu
// einer Butter. Das ist die Sorte Schleife, auf die der Balancingpruefer
// dieses Projekts ausdruecklich angesetzt ist.
//
// Die Sperre haengt am TIER und nicht an der Kanne. An der Kanne haette jeder
// Spieler seine eigene, und zwei Kannen an derselben Kuh waeren die doppelte
// Ausbeute - die Sperre soll aber die KUH begrenzen, nicht den Melker.
//
// Sie ueberlebt keinen Serverneustart. Das ist hingenommen und nicht
// vergessen: eine persistente Marke waere ein ModStorage-Eintrag an einer
// Vanilla-Tierklasse, und dieser Slice ist die Melkfunktion, nicht ein
// Speicherformat.
//==============================================================================

/**
 * Die Milchkanne.
 *
 * Sie hatte bis zum 07.09.2026 keine Skriptklasse - das ist der Grund, aus dem
 * an ihr nie etwas anzubieten war. AddAction() braucht eine Klasse, an der es
 * stehen kann.
 *
 * SetActions() ruft super zuerst: das Item soll seine geerbten Aktionen
 * behalten (Aufheben, Ablegen, Weitergeben) und die Melkaktion zusaetzlich
 * bekommen.
 */
class ChefZ_MilkCan extends ItemBase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(ChefZ_ActionMilkCow);
	}
}

/**
 * Die Kuh - erweitert um die Melksperre.
 *
 * modded und nicht abgeleitet: Animal_BosTaurusF ist eine Vanilla-Klasse, und
 * sie zu ersetzen wuerde jedes andere Mod treffen, das dasselbe tut. Ein
 * modded class kettet sich ein, statt zu verdraengen (Regel 6: Fremdes wird
 * erweitert, nie veraendert).
 *
 * An Animal_BosTaurusF und nicht an Animal_BosTaurus: die Sperre gehoert dahin,
 * wo die Aktion greift, und die greift ausdruecklich nur an weiblichen Rindern
 * (Begruendung in ChefZ_ActionMilkCow).
 */
// SCOUT-GEPRUEFT 2026-09-07, NACHGEZOGEN 2026-09-17
// Geprueft wurde: ein synchronisiertes bool, ein Timer und drei Methoden, alle
// mit m_ChefZ_/ChefZ_ praefigiert (Namenskonvention, Regel 8). KEIN override.
// Damit greift diese Erweiterung in keinen Vanilla-Ablauf ein und kann sich mit
// einem zweiten modded class an derselben Tierklasse nicht widersprechen: sie
// fuegt hinzu, sie ersetzt nichts.
// Der bekannte Nachbar auf diesem Server ist TerjeSkills; dessen
// Animals/config.cpp fasst Rinder nur ueber CfgVehicles-Werte an, nicht ueber
// eine Skriptklasse (nachgelesen in Mod Repositories).
//
// ---------------------------------------------------------------------------
// WARUM EIN SYNCHRONISIERTES BOOL UND KEIN ZEITSTEMPEL (17.09.2026)
// ---------------------------------------------------------------------------
// Bis hierher stand hier ein reiner int m_ChefZ_NextMilkTime, gesetzt allein im
// Serverpfad (OnFinishProgressServer -> ChefZ_MarkMilked) und nie uebertragen.
// Auf jedem Client blieb er 0, ChefZ_CanBeMilked lieferte dort also immer true.
// Folge: Der Client bot "Kuh melken" weiter an, der Server lehnte den Start ab
// (ActionManagerServer.c:168 ruft pickedAction.Can(...), ActionBase.c:954 ruft
// darin ActionCondition), und der Spieler sah bis zu zehn Minuten lang eine
// Aktion, die beim Tastendruck kommentarlos abbrach.
//
// Ein synchronisierter ZEITSTEMPEL waere die falsche Abhilfe: g_Game.GetTime()
// (Game.c:1534) zaehlt auf Client und Server getrennt, ein uebertragener Wert
// bedeutete auf der Gegenseite etwas anderes. Uebertragen wird deshalb der
// ZUSTAND "leer" als bool; die Uhr laeuft ausschliesslich auf dem Server, als
// Timer.
//
// Belege im 1.30-Stand:
//   EntityAI.c:2809  proto native void RegisterNetSyncVariableBool(string)
//   EntityAI.c:3041  proto native void SetSynchDirty()
//   tools.c:10       const int CALL_CATEGORY_GAMEPLAY = 2
//   tools.c:576/595  class Timer / Timer.Run(duration, obj, fn_name, ...)
//                    - Sekunden, "Call is not executed after the Timer object
//                      is deleted" (tools.c:548): stirbt die Kuh, faellt der
//                      ref-Timer mit ihr, kein Aufruf ins Leere.
//   FireplaceBase.c:1812-1813 - dasselbe Muster serverseitig in Vanilla.
//
// Ein Konstruktor ist dafuer noetig (die Registrierung muss auf beiden Seiten
// gleich laufen). Er ruft nichts weiter auf; die Vanilla-Konstruktorkette
// laeuft in Enforce von selbst - dieselbe Form wie TerjeRadiation/.../
// AnimalBase.c:7-10 in Mod Repositories.
//
// SCOUT-GEPRUEFT 2026-09-17 (Stand mit bool, Timer und Konstruktor)
modded class Animal_BosTaurusF
{
	//! Wie lange eine Kuh nach dem Melken leer bleibt, in Sekunden.
	//! Zehn Minuten sind ein Vorschlag, kein Messergebnis - der Wert steht
	//! als einzige Zahl hier, damit das Balancing ihn an einer Stelle findet.
	//! Sekunden und nicht Millisekunden, weil Timer.Run Sekunden nimmt
	//! (tools.c:595).
	static const float CHEFZ_MILK_COOLDOWN_SEC = 600.0;

	//! true heisst "gerade leer". Synchronisiert, damit der Client dieselbe
	//! Antwort gibt wie der Server. false ist der Ausgangswert, den Enforce
	//! ohnehin setzt - "noch nie gemolken" ist damit sofort frei.
	protected bool m_ChefZ_MilkEmpty;

	//! Der Rueckstellzaehler. Laeuft nur auf dem Server, weil nur dort
	//! ChefZ_MarkMilked aufgerufen wird.
	protected ref Timer m_ChefZ_MilkRefillTimer;

	void Animal_BosTaurusF()
	{
		RegisterNetSyncVariableBool("m_ChefZ_MilkEmpty");
	}

	bool ChefZ_CanBeMilked()
	{
		return !m_ChefZ_MilkEmpty;
	}

	void ChefZ_MarkMilked()
	{
		m_ChefZ_MilkEmpty = true;
		SetSynchDirty();

		if (!m_ChefZ_MilkRefillTimer)
			m_ChefZ_MilkRefillTimer = new Timer(CALL_CATEGORY_GAMEPLAY);

		m_ChefZ_MilkRefillTimer.Run(CHEFZ_MILK_COOLDOWN_SEC, this, "ChefZ_OnMilkRefilled");
	}

	//! Ziel des Timers. protected und trotzdem ueber den Namen aufrufbar -
	//! dasselbe tut Vanilla mit FireplaceBase.Heating (FireplaceBase.c:1825).
	protected void ChefZ_OnMilkRefilled()
	{
		m_ChefZ_MilkEmpty = false;
		SetSynchDirty();
	}
}

//==============================================================================
// Die Registrierung.
//
// Ohne sie existiert die Aktion nicht. ActionConstructor.RegisterActions() ist
// eine von Hand gepflegte Liste (ActionConstructor.c:27); ConstructActions()
// instanziiert ausschliesslich, was darin steht, und ActionManagerBase.
// GetAction() liest nur diese Karte. Eine Aktionsklasse, die hier fehlt, laesst
// sich auch mit AddAction() nicht anhaengen - sie kompiliert, sie ist im Log
// unsichtbar, und sie erscheint nie im Spiel.
//
// Genau so lagen beide Core-Aktionen bis zum 29.08.2026. Die vollstaendige
// Herleitung steht im Kopf von ChefZ_Core/.../ChefZ_ActionRegistration.c; der
// Validator "chefzaction" prueft es seither.
//
// super zuerst, ein Insert, kein eigenes Member - dieselbe Form wie im Core,
// damit sich mehrere ChefZ-PBOs ohne Weiteres ketten.
//==============================================================================
// SCOUT-GEPRUEFT 2026-09-07
// Geprueft wurde: identische Form zur bereits vorhandenen Erweiterung in
// ChefZ_Core/.../ChefZ_ActionRegistration.c (dort SCOUT-GEPRUEFT 2026-08-30) -
// super als erste Anweisung, ein Insert, kein eigenes Member, kein Zugriff auf
// die Liste ausser dem Anhaengen. Dass zwei ChefZ-PBOs dieselbe Klasse
// erweitern, ist damit derselbe Fall, den der Core-Kommentar bereits belegt:
// zehn Expansion-Module tun es auf demselben Server.
modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(ChefZ_ActionMilkCow);
	}
}
