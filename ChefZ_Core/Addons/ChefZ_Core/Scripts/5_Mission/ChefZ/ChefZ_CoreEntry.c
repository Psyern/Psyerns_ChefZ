//==============================================================================
// ChefZ_CoreEntry - Einstiegspunkt des Core in die Mission
//
// Entwurf: 00 §4 (5_Mission = "Startzeitpunkt und Missionslebensdauer"),
// 19 §3 S1.
//
// Aufgabe:
//   1. dem Log die Seite melden (GetGame() gibt es in 1_Core nicht)
//   2. die Startzeile ins RPT schreiben
//   3. den S1-Selbsttest einmal laufen lassen
//   4. seit S2: ChefZ_Boot anstossen - Selbsttest S2 und Config laden
//
// Was hier NICHT passiert: es wird nichts an Vanilla veraendert. Der Config
// Manager liest ausschliesslich Config-Baeume und Dateien und legt sein
// eigenes Verzeichnis unter $profile:ChefZ an. Vanilla-Kochen bleibt
// unberuehrt - es gibt bis S7 keinen Code, der es anfassen koennte.
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_CoreEntry
{
    //! Muss zu "version" in CfgMods passen. Der Selbsttest prueft das nicht -
    //! er kann die Config nicht lesen -, deshalb steht es hier als Notiz.
    static const string CORE_VERSION = "0.0.1";

    private static bool s_Booted;

    static void BootServer()
    {
        ChefZ_Log.SetSide(true);
        BootOnce("SERVER");

        // S2: Selbsttest des Config Managers und LoadAll(server). Bewusst
        // NACH der Startzeile - wer im RPT sucht, findet zuerst, dass der Mod
        // ueberhaupt geladen hat, und danach, was er geladen hat.
        ChefZ_Boot.OnMissionStart();
    }

    static void BootClient()
    {
        ChefZ_Log.SetSide(false);
        BootOnce("CLIENT");

        // S2: Rang 1 + 2 aus denselben PBOs. Kein $profile:, keine
        // Spielentscheidung (02 §6).
        ChefZ_Boot.OnClientStart();
    }

    private static void BootOnce(string side)
    {
        if (s_Booted)
            return;
        s_Booted = true;

        // Genau die Zeile aus der Abnahmebedingung 19 §3 S1. Sie geht bewusst
        // an der Stufenpruefung vorbei (ChefZ_Log.Banner): bei Standardstufe
        // WARN wuerde eine Info-Zeile nie erscheinen, und ein Betreiber muss
        // sehen koennen, dass der Mod geladen hat.
        ChefZ_Log.Banner("v1 geladen  Core " + CORE_VERSION + "  Seite " + side);

        bool ok = ChefZ_CoreSelfTest.Run();
        if (ok)
        {
            ChefZ_Log.Banner(ChefZ_CoreSelfTest.Summary());
        }
        else
        {
            // Die Einzelfehler hat der Selbsttest bereits als ERROR gemeldet.
            ChefZ_Log.Error(ChefZ_LogChannel.CORE, ChefZ_CoreSelfTest.Summary());
        }

        ChefZ_Log.Flush();
    }
}

// modded class MissionServer
// Begruendung: der Core braucht genau einen Startzeitpunkt je Seite. OnInit
// ist derselbe Einstiegspunkt, den Vanilla fuer CfgGameplayHandler.LoadData()
// nutzt - also exakt der Zeitpunkt, den der Config Manager in S2 belegt.
// super zuerst, danach nur Lesen und Loggen; der Rueckgabewert von Vanilla
// wird nicht angefasst. Das ist die EINZIGE Stelle, an der ChefZ_Core
// MissionServer erweitert.
// SCOUT-GEPRUEFT 2026-08-30 (chefz-conflict-scout)
// super zuerst, danach nur Lesen, Loggen und ChefZ_CompNotice.EmitAll().
// Genau ein Einstiegspunkt je Seite.
modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        ChefZ_CoreEntry.BootServer();

        // Der eine Haken fuer die Comp-Module. Sie brachten frueher jeweils
        // einen eigenen "modded class MissionServer" mit; zwei davon
        // gleichzeitig haben den Server am 28.08.2026 umgebracht. Warum das
        // hier steht und nicht dort, erklaert ChefZ_CompNotice.
        ChefZ_CompNotice.EmitAll();

        // ---- TEMPORAER: Vorarbeit V-A ----------------------------------
        // Faellt zusammen mit Tests/V_A_PboJsonSmoke/ und dessen
        // files[]-Eintrag in config.cpp weg, sobald das Ergebnis im
        // Gate-1-Protokoll steht. Beim Entfernen: diese zwei Zeilen und die
        // Entsprechung in MissionGameplay unten ebenfalls loeschen.
        ChefZ_PboProbe.RunServer();
        // ----------------------------------------------------------------
    }
}

// modded class MissionGameplay
// Begruendung: der Client braucht dieselbe Seitenmeldung ans Log, damit
// Ladefehler im Client-RPT sichtbar bleiben (18 §4). Rein beobachtend.
// Das ist die EINZIGE Stelle, an der ChefZ_Core MissionGameplay erweitert.
// SCOUT-GEPRUEFT 2026-08-30 (chefz-conflict-scout)
// super zuerst, danach nur Lesen und Loggen.
modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();
        ChefZ_CoreEntry.BootClient();

        // ---- TEMPORAER: Vorarbeit V-A ----------------------------------
        ChefZ_PboProbe.RunClient();
        // ----------------------------------------------------------------
    }
}
