//==============================================================================
// ChefZ_CookbookOpener - die Naht zwischen Wissen und Oberflaeche
//
// Entwurf: ChefZ_Cookbook_Workflow §3 (zwei Addons), Gate 5.2 ("ohne Dabs
// passiert nichts, aber es steht eine Logzeile im RPT").
//
// ---------------------------------------------------------------------------
// WARUM ES DIESE KLASSE GIBT
// ---------------------------------------------------------------------------
// ChefZ_Cookbook darf Dabs nicht kennen - Regel 2 und 3 dieses Meilensteins.
// Die Aktion am Buch und das Tastenkuerzel muessen aber irgendetwas aufrufen,
// wenn jemand aufschlagen will.
//
// Also ruft beides HIER an, und die Vorgabe ist eine Logzeile. Ist
// ChefZ_Cookbook_UI installiert, ueberschreibt es Open() und zeigt das Menue.
// Ist es das nicht - weil Dabs fehlt -, sagt der Server, warum nichts
// geschieht, statt still zu bleiben.
//
// Dasselbe Muster wie ChefZ_CompNotice im Core: ein leerer Haken, der nicht
// weiss, wer sich daranhaengt. Und derselbe Grund, keinen eigenen
// "modded class MissionServer" dafuer zu nehmen.
//
// Layer: 4_World. Keine Dabs-Referenz.
//==============================================================================

class ChefZ_CookbookOpener
{
    private static bool s_MissingReported;

    /**
     * Von ChefZ_Cookbook_UI ueberschrieben. Hier absichtlich nur eine Meldung.
     *
     * @return true, wenn tatsaechlich etwas geoeffnet wurde. Die Aktion am Buch
     *         benutzt das nicht - sie soll auch ohne Oberflaeche ausloesbar
     *         bleiben, damit der Spieler die Meldung ueberhaupt zu sehen
     *         bekommt.
     */
    bool Open(PlayerBase spieler)
    {
        if (!s_MissingReported)
        {
            s_MissingReported = true;
            ChefZ_Log.Banner("Kochbuch: keine Oberflaeche geladen. ChefZ_Cookbook_UI fehlt oder Dabs Framework ist nicht installiert - das Wissen wird trotzdem gesammelt und gespeichert.");
        }
        return false;
    }

    //! Einmal je Aufruf frisch: die Ueberschreibung des UI-Addons haengt an der
    //! Klasse, nicht an einer Instanz, und ein gehaltener Zeiger waere ein
    //! Zustand, den niemand braucht.
    static bool OpenFor(PlayerBase spieler)
    {
        ChefZ_CookbookOpener opener = new ChefZ_CookbookOpener();
        return opener.Open(spieler);
    }
}
