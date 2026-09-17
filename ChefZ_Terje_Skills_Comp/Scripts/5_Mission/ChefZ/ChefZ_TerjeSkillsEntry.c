// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeSkills
// geladen ist. Fehlt der Mod, ist TERJE_SKILLS_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_SKILLS_MOD
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
            ChefZ_Log.Banner("TerjeSkills-Anbindung v" + MODULE_VERSION + " geladen, aber per Config abgeschaltet.");
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

        ChefZ_Log.Banner("TerjeSkills-Anbindung v" + MODULE_VERSION + " aktiv  " + ChefZ_TerjeSkillsConfig.Summary());
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
//
// Vanilla 1.30: OnInit "scripts (and more) - 1.30"/scripts/5_Mission/DayZ/
// mission/missionServer.c:85, InvokeOnDisconnect :442 (Aufruf aus
// PlayerDisconnected :690/:703).
//
// Gesucht wurde in "Mod Repositories" nach "modded class MissionServer" und
// danach, wer OnInit oder InvokeOnDisconnect erweitert. Ergebnis: jede
// gefundene Stelle ruft super, die Kette bleibt also vollstaendig.
//   TerjeCore/Scripts/5_Mission/MissionServer.c:1 - OnEvent :3, InvokeOnConnect
//     :49, PlayerDisconnected :56, OnClientNewEvent :62, OnClientReadyEvent
//     :73, jeweils mit super; WEDER OnInit NOCH InvokeOnDisconnect. Keine
//     Ueberschneidung mit diesem Modul.
//   TerjeStartScreen/Scripts/5_Mission/MissionServer.c:9 (experimental :20) -
//     dieselbe Methode InvokeOnDisconnect, super an :11 (experimental :22).
//     Beide Rumpfe laufen, unabhaengig davon, wer zuletzt laedt.
//   COT_New/Scripts/5_mission/communityonlinetools/missionserver.c:21 -
//     OnMissionStart/OnMissionLoaded/OnUpdate/OnEvent, alle mit super, kein
//     OnInit.
//   DayZ-CommunityFramework-production/JM/CF/Scripts/5_Mission/
//     CommunityFramework/Mission/MissionServer.c:27 OnInit mit super :29 und
//     :70 InvokeOnDisconnect mit super :74.
//   ChefZ_Core/.../ChefZ_CoreEntry.c:87 - eigenes Modul, dieselbe Klasse,
//     ebenfalls super zuerst.
// Kein Fund ohne super, also kein Szenario, in dem eine Ladereihenfolge dieses
// OnInit verschluckt. Bleibt das allgemeine Restrisiko: ein fremder Mod, der
// super weglaesst, haengt jede spaeter geladene Erweiterung ab - dagegen
// schuetzt an dieser Stelle nichts.
// SCOUT-GEPRUEFT 2026-09-17 (Conflict-Scout-Lauf im Auftrag chefz-130, Suchraum
// "Mod Repositories": Vanilla 1.30/1.29, Terje stable+experimental, COT_New,
// COT alt, CF, Expansion, uebrige Fremdmods)
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
     * InvokeOnDisconnect ist Vanillas Stelle dafuer und wird aus
     * PlayerDisconnected gerufen: "scripts (and more) - 1.30"/scripts/
     * 5_Mission/DayZ/mission/missionServer.c:442 (Deklaration) und :703
     * (Aufruf); in 1.29 dieselbe Bauart unter :429 und :690.
     * super zuerst, danach nur eine Zeile in einer EIGENEN Tabelle: an
     * Vanillas Abmeldung wird nichts veraendert.
     *
     * DIE IDENTITY KANN HIER FEHLEN. Vanilla schreibt das zwei Zeilen ueber
     * dem Aufruf selbst hin: missionServer.c:692 "Note: At this point,
     * identity can be already deleted" (1.29: :679). Dann faellt Forget()
     * aus. Das ist seit dem 17.09.2026 kein Leck mehr, aber auch nicht hier
     * geheilt: ChefZ_TerjeXpDamper.SweepAll() raeumt abgelaufene Zeilen
     * zeitgesteuert weg, gleich ob der Spieler je abgemeldet wurde. Die
     * Begruendung, warum nicht stattdessen der frueheren Stelle
     * OnClientDisconnectedEvent (missionServer.c:641, 1.29: :628) gefolgt
     * wird - der Spieler kann die Abmeldung dort noch abbrechen -, steht an
     * SweepAll().
     *
     * Dieser Aufruf hier bleibt trotzdem: er ist der Normalfall, er wirkt
     * sofort, und ein Server mit hoher Fluktuation soll keine Zeilen von
     * laengst abgemeldeten Spielern mitschleppen.
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
//
// Vanilla 1.30: OnInit "scripts (and more) - 1.30"/scripts/5_Mission/DayZ/
// mission/missionGameplay.c:102.
//
// Scout-Ergebnis fuer OnInit an dieser Klasse - jede Stelle ruft super:
//   TerjeCore/Scripts/5_Mission/MissionGameplay.c:3 OnInit, super :5.
//   TerjeSkills/Scripts/5_Mission/MissionGameplay.c:3 - nur OnMissionFinish und
//     OnUpdateTerjeCustomGUI :14, kein OnInit; keine Ueberschneidung.
//   COT_New/Scripts/5_mission/communityonlinetools/missiongameplay.c:79 OnInit,
//     super :81.
//   DayZExpansion/Hardline/.../MissionGameplay.c:20 (super :22),
//     NamalskAdventure/.../MissionGameplay.c:19 (super :21),
//     Quests/.../MissionGameplay.c:21 - dort steht super erst NACH dem eigenen
//     Aufruf (:26), die Kette bleibt aber geschlossen.
//   ChefZ_Core/.../ChefZ_CoreEntry.c:116 - eigenes Modul, dieselbe Klasse.
// SCOUT-GEPRUEFT 2026-09-17 (derselbe Lauf; rein lesender Client-Eingriff, der
// Nachweis ersetzt das nicht, er ergaenzt es)
modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();
        ChefZ_TerjeSkillsEntry.BootClient();
    }
}
#endif // TERJE_SKILLS_MOD
