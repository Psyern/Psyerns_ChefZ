//==============================================================================
// ChefZ_FishingYieldRegistrar - die Fangtabelle in die Bank der Welt eintragen
//
// Entwurf: 20 §4.5 (Schnittstelle woertlich), 20 §5 (Datenfluss), 20 E3
// (warum spaet und direkt und nicht ueber den Einhaengepunkt der Engine),
// 20 §8 (Fehlerverhalten), 20 §6 D1 (Reihenfolge).
//
// ---------------------------------------------------------------------------
// Warum hier direkt eingetragen wird und NICHT ueber den Einhaengepunkt der
// Engine (20 E3) - der wichtigste Absatz dieser Datei
// ---------------------------------------------------------------------------
// Die Engine bietet einen Aufrufhaken an, mit dem ein Mod seine Ertraege in
// die Bank einer Welt schieben kann. Er wurde geprueft und verworfen, weil er
// ZWEIMAL still scheitert:
//
//   1. Er feuert im Konstruktor der Weltdaten, der seinerseits aus dem
//      Konstruktor der Mission laeuft. Der ChefZ-Config-Manager laedt erst in
//      OnInit - zum Zeitpunkt des Hakens gaebe es keinen einzigen Datensatz.
//
//   2. Zwei der drei offiziellen Karten rufen in ihrer eigenen
//      InitYieldBank() zuerst die Basis - also den Haken - und danach
//      ClearAllRegisteredItems(). Alles ueber den Haken Eingetragene ist dort
//      eine Zeile spaeter geloescht. Der Haken haette auf genau einer Karte
//      funktioniert und auf zwei nicht; das ist die Art Fehler, die erst im
//      Betrieb auffaellt.
//
// Ein "modded class" je Welt waere die dritte Moeglichkeit und verletzt
// Regel 4: eine vierte, gemoddete Karte braeuchte wieder Core-Code.
//
// Der Eintrag hier laeuft NACH InitYieldBank und damit nach jedem
// ClearAllRegisteredItems - auf jeder Welt gleich, auch auf Karten, die es
// heute noch nicht gibt. Der Preis ist ein Flag gegen Doppelausfuehrung, und
// das ist billig gegen zwei stille Totalausfaelle.
//
// ---------------------------------------------------------------------------
// Client UND Server
// ---------------------------------------------------------------------------
// Beide tragen ein, und beide muessen es. Der Fang selbst ist
// serverautoritativ, aber der Client baut dasselbe Gewichtsfeld, um aus
// demselben synchronisierten Zufallsstrom denselben Index zu ziehen - er
// braucht ihn fuer Zykluszeit und Partikel. Eine Seite, die weniger
// eintraegt, sieht den Biss zu einem anderen Zeitpunkt.
//
// Layer: 4_World - CatchYieldBank und die Ertragsobjekte leben hier.
//==============================================================================

/**
 * Was ein Eintragungslauf getan hat.
 *
 * Eigene Klasse statt sechs out-Parametern: der Aufrufer schreibt genau eine
 * Zeile ins Log, und diese Zeile soll vollstaendig sein, ohne dass jemand die
 * Reihenfolge von sechs Zahlen im Kopf hat.
 */
class ChefZ_FishingRegisterReport : Managed
{
    int    requested;
    int    registered;
    int    skippedAlreadyPresent;
    int    replacedExisting;
    int    rejected;
    int    fingerprint;
    string worldName;

    void ChefZ_FishingRegisterReport()
    {
        requested             = 0;
        registered            = 0;
        skippedAlreadyPresent = 0;
        replacedExisting      = 0;
        rejected              = 0;
        fingerprint           = 0;
        worldName             = "";
    }

    string ToLine()
    {
        string s = "Fangtabelle eingetragen  welt=" + worldName + "  angefragt=" + requested.ToString() + "  eingetragen=" + registered.ToString();
        s = s + "  uebersprungen=" + skippedAlreadyPresent.ToString() + "  ersetzt=" + replacedExisting.ToString() + "  abgewiesen=" + rejected.ToString();
        s = s + "  fingerprint=" + fingerprint.ToString();
        return s;
    }
}

//==============================================================================

class ChefZ_FishingYieldRegistrar
{
    private static bool s_Registered;

    /**
     * Genau einmal je Mission, nach dem Einfrieren der Config.
     *
     * @return true, wenn dieser Aufruf etwas getan hat. false heisst
     *         "schon geschehen" oder "nichts einzutragen" - in beiden Faellen
     *         bleibt die Fangtabelle der Welt genau so, wie die Engine sie
     *         aufgebaut hat (Invariante I2).
     */
    static bool RegisterAll(out ChefZ_FishingRegisterReport report)
    {
        if (!report)
            report = new ChefZ_FishingRegisterReport();

        if (s_Registered)
        {
            // Ein zweiter Lauf wuerde die Ordnungsliste der Bank ein zweites
            // Mal verlaengern und den Fehlerpfad der Engine ausloesen, die ein
            // Ertragsobjekt nur EINER Bank zuordnen laesst.
            ChefZ_Log.Warn(ChefZ_LogChannel.FISHING, "RegisterAll() wurde erneut aufgerufen und ignoriert. Die Fangtabelle wird " + "genau einmal je Mission eingetragen; ein zweiter Lauf wuerde doppelte " + "Eintraege erzeugen.");
            return false;
        }

        ChefZ_FishingRegistry registry = ChefZ_FishingRegistry.Get();
        if (!registry.IsReady())
        {
            // Der Normalfall auf einem Server ohne Datenmodul und der
            // Ausfallpfad im SAFE MODE. Beides ist reines Vanilla-Angeln.
            ChefZ_Log.Info(ChefZ_LogChannel.FISHING, "Keine Fangtabelle geladen - die Fangtabelle der Welt bleibt unveraendert.");
            s_Registered = true;
            return false;
        }

        array<ChefZ_FishingYieldDef> defs = new array<ChefZ_FishingYieldDef>();
        registry.GetOrderedYields(defs);

        report.fingerprint = registry.GetContentFingerprint();
        report.requested   = defs.Count();
        report.worldName   = ResolveWorldName();

        if (defs.Count() == 0)
        {
            ChefZ_Log.Info(ChefZ_LogChannel.FISHING, "Fangtabelle ist leer - die Fangtabelle der Welt bleibt unveraendert.");
            s_Registered = true;
            return false;
        }

        CatchYieldBank bank = ResolveBank();
        if (!bank)
        {
            // 20 §8: die Eintragung faellt komplett aus, IsLure bleibt false,
            // und geangelt wird wie in Vanilla. Einmal gemeldet, nicht je
            // Versuch.
            ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.FISHING, "fishing.nobank", "Die Fangtabelle der Welt ist nicht erreichbar - ChefZ traegt nichts ein. " + "Geangelt wird unveraendert nach Vanilla-Gewichten.");
            s_Registered = true;
            return false;
        }

        YieldsMap present = bank.GetYieldsMap();

        for (int i = 0; i < defs.Count(); i++)
        {
            ChefZ_FishingYieldDef def = defs.Get(i);
            if (!def)
            {
                report.rejected++;
                continue;
            }

            int typeHash = def.GetTypeHash();
            bool exists = false;
            if (present)
                exists = present.Contains(typeHash);

            if (exists && !def.overrideExisting)
            {
                // 20 §8: der vorhandene Eintrag bleibt. Das ist die sichere
                // Richtung - ein fremder Eintrag wird nie versehentlich
                // verdraengt, und wer ihn erweitern will, sagt es im
                // Datensatz.
                report.skippedAlreadyPresent++;
                continue;
            }

            if (exists)
            {
                bank.UnregisterYieldItem(def.id);
                report.replacedExisting++;
            }

            bank.RegisterYieldItem(new ChefZ_FishingYieldItem(def.baseWeight, def));
            report.registered++;
        }

        s_Registered = true;

        // Eine Zeile, an der Stufenpruefung vorbei: der Fingerabdruck ist die
        // eine Sonde, die ein Mensch auswerten muss (20 §6, T7). Client und
        // Server schreiben sie beide; sind die Zahlen verschieden, laufen die
        // Gewichtsfelder auseinander.
        ChefZ_Log.Banner(report.ToLine());
        return report.registered > 0;
    }

    static bool HasRegistered()
    {
        return s_Registered;
    }

    /**
     * Fuer den zweiten Missionsstart im selben Prozess.
     *
     * Beim Client ist das der Normalfall, sobald jemand einen zweiten Server
     * betritt: die Mission ist neu, die Weltdaten sind neu, und damit ist auch
     * die Fangtabelle der Welt eine neue und leere. Ohne diesen Ruecksetzer
     * bliebe sie ab dem zweiten Start ohne ChefZ-Eintraege - und das faellt
     * niemandem auf, weil Angeln dann einfach wieder Vanilla ist.
     */
    static void ResetForNewMission()
    {
        s_Registered = false;
    }

    //--------------------------------------------------------------------------

    /**
     * Mission -> Weltdaten -> Bank.
     *
     * Jede Stufe einzeln geprueft: die Kette laeuft im Missionsstart, und dort
     * ist "noch nicht da" ein moeglicher Zustand, kein Fehler. null heisst
     * "nicht eintragen", und nicht eintragen heisst Vanilla.
     */
    protected static CatchYieldBank ResolveBank()
    {
        if (!g_Game)
            return null;

        Mission mission = g_Game.GetMission();
        if (!mission)
            return null;

        WorldData worldData = mission.GetWorldData();
        if (!worldData)
            return null;

        return worldData.GetCatchYieldBank();
    }

    protected static string ResolveWorldName()
    {
        if (!g_Game)
            return "";
        return g_Game.GetWorldName();
    }
}
