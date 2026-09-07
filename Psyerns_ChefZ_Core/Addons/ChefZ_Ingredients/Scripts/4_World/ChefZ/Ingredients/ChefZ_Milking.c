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
// SCOUT-GEPRUEFT 2026-09-07
// Geprueft wurde: ein protected int und zwei Methoden, alle mit m_ChefZ_/
// ChefZ_ praefigiert (Namenskonvention, Regel 8). KEIN override, KEIN
// Konstruktor - ein int ist in Enforce ohnehin 0, und 0 heisst hier "noch nie
// gemolken". Damit greift diese Erweiterung in keinen Vanilla-Ablauf ein und
// kann sich mit einem zweiten modded class an derselben Tierklasse nicht
// widersprechen: sie fuegt hinzu, sie ersetzt nichts.
// Der bekannte Nachbar auf diesem Server ist TerjeSkills; dessen
// Animals/config.cpp fasst Rinder nur ueber CfgVehicles-Werte an, nicht ueber
// eine Skriptklasse (nachgelesen in Mod Repositories).
modded class Animal_BosTaurusF
{
    //! Wie lange eine Kuh nach dem Melken leer bleibt, in Millisekunden.
    //! Zehn Minuten sind ein Vorschlag, kein Messergebnis - der Wert steht
    //! als einzige Zahl hier, damit das Balancing ihn an einer Stelle findet.
    static const int CHEFZ_MILK_COOLDOWN_MS = 600000;

    //! Zeitpunkt, ab dem wieder gemolken werden darf. 0 heisst "noch nie
    //! gemolken" und ist damit sofort frei - Enforce initialisiert einen int
    //! mit 0, deshalb steht hier ausdruecklich KEIN Konstruktor. Einer in
    //! einer modded class waere eine zweite Stelle, an der die Kette der
    //! Erweiterungen brechen kann, und er brauchte nichts zu tun.
    protected int m_ChefZ_NextMilkTime;

    bool ChefZ_CanBeMilked()
    {
        if (m_ChefZ_NextMilkTime <= 0)
            return true;
        return g_Game.GetTime() >= m_ChefZ_NextMilkTime;
    }

    void ChefZ_MarkMilked()
    {
        m_ChefZ_NextMilkTime = g_Game.GetTime() + CHEFZ_MILK_COOLDOWN_MS;
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
