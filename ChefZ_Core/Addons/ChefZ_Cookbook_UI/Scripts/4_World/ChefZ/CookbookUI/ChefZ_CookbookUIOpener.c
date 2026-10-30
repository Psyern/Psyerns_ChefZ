//==============================================================================
// ChefZ_CookbookUIOpener - die Naht von der anderen Seite
//
// ChefZ_Cookbook ruft beim Aufschlagen ChefZ_CookbookOpener.Open() und meldet
// dort ohne Oberflaeche nur eine Logzeile (ChefZ_CookbookOpener.c, Kopf). Ist
// dieses Addon geladen, ueberschreibt es Open() und zeigt das Menue - ohne
// super, denn die Logzeile sagt ja gerade, dass es KEINE Oberflaeche gibt.
//
// Beide Wege dorthin laufen ueber diese eine Stelle: die Taste
// (ChefZ_CookbookInput, prueft den Buchbesitz) und die Aktion am Buch
// (ChefZ_ActionOpenCookbook.OnEndClient).
//
// Layer: 4_World.
//==============================================================================

// SCOUT-GEPRUEFT 2026-10-26 (chefz-conflict-scout): kein anderes ChefZ-Modul
// und keines der Fremd-Repos definiert oder moddet ChefZ_CookbookOpener;
// Signatur gleich der Basis, Lade-Reihenfolge ueber requiredAddons.
modded class ChefZ_CookbookOpener
{
    override bool Open(PlayerBase spieler)
    {
        if (!g_Game || g_Game.IsDedicatedServer())
            return false;
        if (!spieler || spieler != g_Game.GetPlayer())
            return false;
        return ChefZ_CookbookUIHost.Show();
    }
}

//==============================================================================
// ChefZ_CookbookUIHost - wer das Menue haelt
//
// UIManager.ShowScriptedMenu() ist proto native und haelt keine Skriptreferenz.
// Vanilla haelt seine Menues deshalb selbst (DayZGame.c:1892, m_LoginQueue).
// Hier genau eine Instanz; OnHide des Menues gibt sie im Folgebild frei.
//==============================================================================

class ChefZ_CookbookUIHost
{
    private static ref ChefZ_CookbookUIMenu s_Menu;

    static bool Show()
    {
        UIManager ui = g_Game.GetUIManager();
        if (!ui)
            return false;
        // Ueber einem anderen Menue (Inventar, Pause) wird nicht aufgeschlagen.
        if (ui.GetMenu())
            return false;

        s_Menu = new ChefZ_CookbookUIMenu();
        ui.ShowScriptedMenu(s_Menu, null);
        return true;
    }

    static void Release()
    {
        UIManager ui = g_Game.GetUIManager();
        // Inzwischen neu aufgeschlagen? Dann gehoert die Instanz dem neuen Menue.
        if (ui && s_Menu && ui.GetMenu() == s_Menu)
            return;
        s_Menu = null;
    }
}
