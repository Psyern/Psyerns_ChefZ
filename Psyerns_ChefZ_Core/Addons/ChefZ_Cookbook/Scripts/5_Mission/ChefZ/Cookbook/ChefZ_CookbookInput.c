//==============================================================================
// ChefZ_CookbookInput - das Tastenkuerzel zum Aufschlagen
//
// Entwurf: ChefZ_Cookbook_Workflow §6.3 ("Oeffnen - 1:1 wie Expansion").
//
// ---------------------------------------------------------------------------
// WARUM DIE TASTE NICHT IM SKRIPT ANGEMELDET WIRD
// ---------------------------------------------------------------------------
// GetUApi().RegisterInput() gibt es (3_Game/DayZ/InputAPI/UAInput.c:194), es
// wird aber weder in den Vanilla-Skripten 1.29 noch in DayZExpansion ein
// einziges Mal aufgerufen. Wer es benutzt, bekommt keinen Fehler - nur eine
// Gruppe, die im Steuerungsmenue nie erscheint.
//
// Angemeldet wird deshalb datengetrieben, in Scripts/Data/Inputs.xml, und
// verdrahtet in der config.cpp ueber "inputs =". Hier bleibt genau das uebrig,
// was Skript sein MUSS: den Zustand der Taste einmal je Bild lesen.
//
// ---------------------------------------------------------------------------
// WARUM 5_MISSION UND NICHT 4_WORLD
// ---------------------------------------------------------------------------
// Eine Taste hat keinen Besitzer, den man fragen koennte - man muss sie
// abfragen. Der einzige Ort mit einem Bildlauf ist MissionGameplay.OnUpdate,
// und MissionGameplay gibt es erst auf 5_Mission.
//
// Diese Datei ist die EINZIGE auf dieser Ebene in ChefZ_Cookbook, und sie
// kennt kein Dabs (Workflow-Regel 3). Sie ruft ChefZ_CookbookOpener - was
// dahinter haengt, geht sie nichts an.
//
// ---------------------------------------------------------------------------
// WARUM KEIN SERVERANTEIL
// ---------------------------------------------------------------------------
// Ein Fenster geht auf dem Client auf. Der Server weiss laengst, was der
// Spieler kann; er muss nicht erfahren, dass jemand nachschlaegt. Es gibt
// deshalb bewusst kein Gegenstueck in MissionServer und kein RPC.
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_CookbookInput
{
    //! Muss Zeichen fuer Zeichen dem Namen in Scripts/Data/Inputs.xml
    //! entsprechen. Ein Tippfehler ergibt keine Meldung, sondern eine Taste,
    //! die nie ausloest - deshalb steht der Name genau einmal im Projekt.
    static const string TOGGLE_INPUT = "UAChefZCookbookToggle";

    //! Kantenerkennung nach dem Muster von Expansion
    //! (Book/Scripts/5_Mission/.../MissionGameplay.c:32-40). LocalPress() ist
    //! bereits flankengesteuert; der Riegel faengt den Fall ab, dass die Taste
    //! ueber einen Menuewechsel hinweg gehalten wird.
    private static bool s_TogglePressed;

    private static bool s_InputMissingReported;

    /**
     * Einmal je Bild aus MissionGameplay.OnUpdate. Haelt sich kurz: was hier
     * teuer waere, waere es sechzigmal je Sekunde.
     */
    static void Poll()
    {
        if (!g_Game)
            return;

        UAInputAPI api = GetUApi();
        if (!api)
            return;

        UAInput toggle = api.GetInputByName(TOGGLE_INPUT);
        if (!toggle)
        {
            ReportInputMissingOnce();
            return;
        }

        // Menue offen, Textfeld im Fokus, tot oder bewusstlos: nicht abfragen
        // und den Riegel zuruecknehmen, damit eine ueber den Zustandswechsel
        // gehaltene Taste danach nicht sofort ausloest.
        if (!CanReact())
        {
            s_TogglePressed = false;
            return;
        }

        if (toggle.LocalPress() && !s_TogglePressed)
        {
            s_TogglePressed = true;
            OnTogglePressed();
        }
        else if (toggle.LocalRelease() || toggle.LocalValue() == 0)
        {
            s_TogglePressed = false;
        }
    }

    /**
     * Darf die Taste ueberhaupt etwas ausloesen?
     *
     * Die drei Bedingungen sind aus Expansions eigener Vorpruefung uebernommen
     * (Core/Scripts/5_Mission/.../MissionGameplay.c:86-100). Die wichtigste ist
     * die dritte: waehrend das Steuerungsmenue offen ist, belegt der Spieler
     * moeglicherweise gerade DIESE Taste neu.
     */
    private static bool CanReact()
    {
        PlayerBase spieler = PlayerBase.Cast(g_Game.GetPlayer());
        if (!spieler)
            return false;
        if (spieler.GetPlayerState() != EPlayerStates.ALIVE)
            return false;
        if (spieler.IsUnconscious())
            return false;

        UIManager ui = g_Game.GetUIManager();
        if (!ui)
            return false;
        if (ui.GetMenu())
            return false;

        Widget fokus = GetFocus();
        if (!fokus)
            return true;
        if (!fokus.IsVisible())
            return true;
        if (fokus.IsInherited(EditBoxWidget))
            return false;
        if (fokus.IsInherited(MultilineEditBoxWidget))
            return false;

        return true;
    }

    /**
     * Die Besitzpruefung aus §6.3: ohne Buch im Inventar passiert nichts.
     *
     * Sie steht hier und nicht in CanReact(), weil sie teurer ist - sie laeuft
     * das gesamte Inventar ab. Einmal je Tastendruck ist das nichts, einmal je
     * Bild waere es zu viel.
     *
     * Ohne Buch bleibt es still: eine Bildschirmmeldung waere eine Antwort auf
     * eine Frage, die der Spieler nicht gestellt hat. Die Debug-Zeile genuegt
     * fuer die Fehlersuche am Gate.
     */
    private static void OnTogglePressed()
    {
        PlayerBase spieler = PlayerBase.Cast(g_Game.GetPlayer());
        if (!spieler)
            return;

        if (!ChefZ_CookbookItem.CarriedBy(spieler))
        {
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.CORE, ChefZ_LogLevel.DEBUG))
            {
                ChefZ_Log.Debug(ChefZ_LogChannel.CORE, "Kochbuch: Taste gedrueckt, aber kein Kochbuch im Inventar.");
            }
            return;
        }

        ChefZ_CookbookOpener.OpenFor(spieler);
    }

    /**
     * Genau einmal je Sitzung. Wenn der Name unbekannt ist, ist entweder die
     * Inputs.xml nicht im PBO gelandet oder der Pfad in "inputs =" stimmt nicht
     * mit dem $PREFIX$ ueberein. Beides meldet die Engine von sich aus nicht.
     */
    private static void ReportInputMissingOnce()
    {
        if (s_InputMissingReported)
            return;
        s_InputMissingReported = true;

        ChefZ_Log.Warn(ChefZ_LogChannel.CORE, "Kochbuch: Eingabe " + TOGGLE_INPUT + " ist der Engine unbekannt. Scripts/Data/Inputs.xml fehlt im PBO oder der Pfad in inputs= passt nicht zum PBO-Prefix ChefZ_Cookbook.");
    }
}

// modded class MissionGameplay
// Begruendung: eine Taste laesst sich nur im Bildlauf lesen, und der Bildlauf
// des Clients ist OnUpdate. Es gibt keinen anderen Einstieg - auch Expansion
// nimmt fuer sein Buch genau diesen (Book/.../MissionGameplay.c:21).
//
// Umfang: super zuerst, danach ein einziger statischer Aufruf, der nur liest.
// Kein Vanillazustand wird angefasst, kein Eingang gesperrt (kein Lock(),
// kein Supress()), kein Rueckgabewert veraendert. Ein zweiter Mod, der
// MissionGameplay.OnUpdate erweitert, sieht davon nichts.
//
// Das ist die EINZIGE Stelle, an der ChefZ_Cookbook MissionGameplay erweitert.
// ChefZ_Core erweitert dieselbe Klasse in ChefZ_CoreEntry.c, dort aber OnInit -
// die beiden treffen sich nicht.
// SCOUT-GEPRUEFT 2026-08-31: nur OnUpdate-Poll, ruft super, kollisionsarm
modded class MissionGameplay
{
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        ChefZ_CookbookInput.Poll();
    }
}
