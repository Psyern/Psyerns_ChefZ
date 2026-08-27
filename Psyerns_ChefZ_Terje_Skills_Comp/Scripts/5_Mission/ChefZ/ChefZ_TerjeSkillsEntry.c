//==============================================================================
// ChefZ_TerjeSkillsEntry - der Anmeldezeitpunkt dieses Moduls
//
// Genau ein Einstiegspunkt je Seite, derselbe, den auch ChefZ_Core benutzt
// (ChefZ_Core/Scripts/5_Mission/ChefZ/ChefZ_CoreEntry.c: modded class
// MissionServer.OnInit / MissionGameplay.OnInit). Zwei Mods duerfen dieselbe
// Klasse erweitern - Enforce verkettet die Overrides, und jeder ruft super
// als erste Anweisung.
//
// ---------------------------------------------------------------------------
// Reihenfolge
// ---------------------------------------------------------------------------
// ChefZ_Core steht in requiredAddons dieses Moduls, also laufen seine
// Skripte und sein OnInit VOR diesem hier. Der Config Manager ist beim
// Anmelden fertig, die Symboltabelle steht.
//
// Selbst wenn nicht: ChefZ_ProgressRegistry.RegisterSink() und
// ChefZ_CapabilityRegistry.RegisterProvider() sind zu JEDEM Zeitpunkt
// gefahrlos. Ein Empfaenger, der sich spaeter anmeldet, verpasst nur die
// bereits vergangenen Abschluesse - und beim Missionsstart hat noch niemand
// gekocht.
//
// ---------------------------------------------------------------------------
// Clientseitig wird NICHTS angemeldet
// ---------------------------------------------------------------------------
// Der Fortschrittsempfaenger vergibt Erfahrung, der Faehigkeitsanbieter
// beantwortet Serverfragen - beides gehoert auf den Server. Die Hervorhebung
// braucht keine Anmeldung: sie laeuft ueber Terjes eigenen Client-Ticker
// (PluginTerjeClientItemsCore), in den sich die Items per
// IsTerjeClientUpdateRequired() selbst eintragen.
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_TerjeSkillsEntry
{
    static const string MODULE_VERSION = "0.0.1";

    private static bool s_ServerDone;

    private static ref ChefZ_TerjeProgressSink        s_Sink;
    private static ref ChefZ_TerjeCapabilityProvider  s_Provider;

    static void BootServer()
    {
        if (s_ServerDone)
            return;
        s_ServerDone = true;

        ChefZ_TerjeSkillsConfig.Load();

        if (!ChefZ_TerjeSkillsConfig.IsEnabled())
        {
            ChefZ_Log.Banner("TerjeSkills-Anbindung v" + MODULE_VERSION
                + " geladen, aber per Config abgeschaltet.");
            return;
        }

        // Der Fortschrittsempfaenger. Ohne ihn gibt es keine ChefZ-XP - und
        // ohne dieses PBO gibt es ihn nicht, was der Sinn der Sache ist.
        if (ChefZ_TerjeSkillsConfig.IsXpEnabled())
        {
            s_Sink = new ChefZ_TerjeProgressSink();
            ChefZ_ProgressRegistry.RegisterSink(s_Sink);
        }

        // Der Faehigkeitsanbieter. ACHTUNG: das ist kein Recipe Lock, sondern
        // nur die Auskunftsstelle. Solange kein Rezept ein "requires"
        // deklariert - und derzeit tut das keines -, wird er nie gefragt.
        // Die Haerte von Rezeptsperren ist unter OF-08 offen und wird hier
        // nicht vorweggenommen.
        if (ChefZ_TerjeSkillsConfig.CapabilitiesEnabled())
        {
            s_Provider = new ChefZ_TerjeCapabilityProvider();
            ChefZ_CapabilityRegistry.Get().RegisterProvider(s_Provider);
        }

        ChefZ_Log.Banner("TerjeSkills-Anbindung v" + MODULE_VERSION + " aktiv  "
            + ChefZ_TerjeSkillsConfig.Summary());
        ChefZ_Log.Flush();
    }

    static void BootClient()
    {
        // Nur die Config vorladen, damit die Hervorhebung im ersten Tick
        // bereits ihre Reichweiten kennt. Keine Anmeldung, keine
        // Spielentscheidung.
        ChefZ_TerjeSkillsConfig.Load();
    }
}

// modded class MissionServer
// Begruendung: derselbe Einstiegspunkt wie ChefZ_Core. super zuerst, danach
// ausschliesslich eigene Anmeldungen an eigenen Registries - an Vanilla und
// an Terje wird hier nichts veraendert.
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        ChefZ_TerjeSkillsEntry.BootServer();
    }

    /**
     * Aufraeumen der Wiederholungszaehler.
     *
     * scripts/5_Mission/DayZ/mission/missionServer.c:429 - InvokeOnDisconnect
     * ist Vanillas Stelle dafuer und wird aus PlayerDisconnected gerufen.
     * super zuerst, danach nur eine Zeile in einer EIGENEN Tabelle: an
     * Vanillas Abmeldung wird nichts veraendert.
     *
     * Streng genommen unnoetig - die Zaehler verfallen ohnehin nach
     * repeatWindowSec -, aber ein Server mit hoher Fluktuation soll keine
     * Zeilen von laengst abgemeldeten Spielern mitschleppen.
     */
    override void InvokeOnDisconnect(PlayerBase player)
    {
        super.InvokeOnDisconnect(player);

        if (!player)
            return;

        PlayerIdentity ident = player.GetIdentity();
        if (ident)
            ChefZ_TerjeXpDamper.Forget(ident.GetPlayerId());
    }
}

// modded class MissionGameplay
// Begruendung: die Hervorhebung ist rein clientseitig und braucht die
// Config-Werte. Rein lesend.
modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();
        ChefZ_TerjeSkillsEntry.BootClient();
    }
}
