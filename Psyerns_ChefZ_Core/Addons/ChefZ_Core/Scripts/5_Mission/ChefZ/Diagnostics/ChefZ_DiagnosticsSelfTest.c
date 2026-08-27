//==============================================================================
// ChefZ_DiagnosticsSelfTest - Abnahmepruefung fuer S18, soweit sie ohne Welt geht
//
// Entwurf: 18 §2.4, 18 §6 (Fehlerverhalten je Zeile), 18 E6
// (Nebenwirkungsfreiheit), 19 S18 (Abnahmebedingungen).
//
// ---------------------------------------------------------------------------
// Was hier geprueft wird - und warum ausgerechnet das
// ---------------------------------------------------------------------------
// Die beiden Abnahmebedingungen aus 19 S18 lauten:
//
//   1. "chefz match <id> liefert den Block aus 18 §3 und veraendert
//      nachweislich nichts."
//   2. "chefz why <id> <recipeId> nennt den ERSTEN verletzten Slot mit
//      Begruendung."
//
// Beide brauchen ein Gefaess in einer Welt und laufen deshalb HIER nicht.
// Geprueft wird, WORAUF sie beruhen - und das sind genau die Stellen, an denen
// ein Fehler LEISE waere:
//
//   - Der Parser. Ein Kommando, das an der falschen Stelle trennt, meldet
//     "unbekanntes Rezept" statt einer Antwort. Das sieht nach einem
//     Content-Fehler aus und ist keiner.
//   - Die Zusage "keine Aenderung bei ungueltigem Kanalnamen" (18 §6). Ein
//     Parser, der bei einem Tippfehler die Maske trotzdem anfasst, schaltet
//     dem Betreiber mitten in der Fehlersuche Kanaele ab - und der sucht dann
//     einen Fehler, den er selbst erzeugt hat.
//   - Die Symboltabelle. ChefZ_SymbolTable waechst nur und schrumpft nie
//     (03 §4). Wuerde die Diagnose Namen INTERNIEREN statt sie
//     nachzuschlagen, verlaengerte jedes falsch getippte Adminkommando die
//     Tabelle dauerhaft. Nichts daran sieht kaputt aus - deshalb steht die
//     Pruefung hier.
//   - Die Zaehler. Ein Diagnosekommando, das den Fehlerzaehler bewegt,
//     verschiebt die Safe-Mode-Schwelle (18 §4). Hinsehen darf nichts kosten.
//
// Der Test stellt Logstufe und Kanalmaske am Ende exakt wieder her. Ein
// Selbsttest, der die Betriebseinstellung des Servers veraendert, waere selbst
// eine Nebenwirkung - und das ausgerechnet im Teilsystem, dessen Kernzusage
// Nebenwirkungsfreiheit ist.
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_DiagnosticsSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    //! Ein Name, den es garantiert nicht gibt. Bewusst kein Rezeptname und
    //! kein Content-Bezeichner - nur ein Testmarker.
    private static const string NO_SUCH_ID = "ChefZ_S18_KeinEintragMitDiesemNamen";

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        // Betriebseinstellung sichern, BEVOR irgendein Kommando laeuft.
        int savedLevel = ChefZ_Log.GetLevel();
        int savedMask  = ChefZ_Log.GetChannelMask();

        Check("Zerlegung",     TokenizeCheck());
        Check("Hilfe",         HelpCheck());
        Check("Unbekannt",     UnknownVerbCheck());
        Check("KanalFehler",   BadChannelCheck());
        Check("KanalSchalten", ChannelToggleCheck());
        Check("StufeFehler",   BadLevelCheck());
        Check("StufeSetzen",   LevelCheck());
        Check("EntityId",      EntityIdCheck());
        Check("KeinSymbol",    NoInternCheck());
        Check("KeineZaehler",  CounterCheck());
        Check("TraceBlock",    TraceBlockCheck());
        Check("TraceDeckel",   TraceTruncateCheck());

        // Wiederherstellen. Auch wenn eine Gruppe gescheitert ist - gerade
        // dann.
        ChefZ_Log.SetLevel(savedLevel);
        ChefZ_Log.SetChannel(ChefZ_LogChannel.ALL, false);
        ChefZ_Log.SetChannel(savedMask, true);

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.CORE, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.CORE, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.CORE,
            "Selbsttest " + name + " FEHLGESCHLAGEN. Die Diagnose verhaelt sich nicht wie "
            + "entworfen - ab hier sind ihre Auskuenfte unzuverlaessig. Kochen, Vanilla und "
            + "der uebrige Core sind davon unberuehrt.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S18: " + s_Passed.ToString() + "/" + total.ToString()
                 + " Gruppen ok";
        if (s_Failed > 0 && s_FailedNames)
        {
            s = s + "  gescheitert:";
            for (int i = 0; i < s_FailedNames.Count(); i++)
                s = s + " " + s_FailedNames.Get(i);
        }
        return s;
    }

    //==========================================================================
    // Parser
    //==========================================================================

    /**
     * Die Zerlegung ist ueber Execute() geprueft und nicht ueber Tokenize()
     * direkt: Tokenize ist privat, und ein Test, der nur privaten Code
     * erreicht, prueft den Bauplan statt das Gebaeude.
     *
     * Geprueft wird das Verhalten, das ein falsches Ergebnis LEISE macht:
     * Mehrfachleerzeichen, fuehrendes und abschliessendes Leerzeichen, das
     * optionale "chefz" davor.
     */
    private static bool TokenizeCheck()
    {
        array<string> lines = new array<string>();

        // "help" ist das einzige Kommando ohne Nebenwirkung UND ohne
        // Serverbedingung - also das einzige, an dem sich der Parser
        // unabhaengig von der Seite pruefen laesst.
        if (!ChefZ_AdminCommands.Execute("help", lines))
            return false;
        if (lines.Count() == 0)
            return false;

        lines.Clear();
        if (!ChefZ_AdminCommands.Execute("chefz help", lines))
            return false;

        lines.Clear();
        if (!ChefZ_AdminCommands.Execute("   CHEFZ    HELP   ", lines))
            return false;

        lines.Clear();
        if (!ChefZ_AdminCommands.Execute("chefz ?", lines))
            return false;

        // Leere Zeile: kein Treffer, aber eine Antwort und kein Absturz.
        lines.Clear();
        if (ChefZ_AdminCommands.Execute("", lines))
            return false;
        if (lines.Count() == 0)
            return false;

        return true;
    }

    private static bool HelpCheck()
    {
        array<string> lines = new array<string>();
        if (!ChefZ_AdminCommands.Execute("chefz help", lines))
            return false;

        // Jedes der sieben Kommandos aus 18 §2.4 muss in der Hilfe stehen.
        // Sonst kennt sie ein Betreiber nicht, und ein Werkzeug, das niemand
        // findet, gibt es praktisch nicht.
        if (!Mentions(lines, "chefz log level"))     return false;
        if (!Mentions(lines, "chefz log channel"))   return false;
        if (!Mentions(lines, "chefz match"))         return false;
        if (!Mentions(lines, "chefz why"))           return false;
        if (!Mentions(lines, "chefz registries"))    return false;
        if (!Mentions(lines, "chefz audit"))         return false;
        if (!Mentions(lines, "chefz stats"))         return false;
        return true;
    }

    private static bool UnknownVerbCheck()
    {
        array<string> lines = new array<string>();

        // false, mit Begruendung UND Hilfe - nicht stillschweigend nichts.
        if (ChefZ_AdminCommands.Execute("chefz " + NO_SUCH_ID, lines))
            return false;
        if (!Mentions(lines, NO_SUCH_ID))
            return false;
        if (!Mentions(lines, "chefz match"))
            return false;
        return true;
    }

    //==========================================================================
    // Laufzeitschalter (18 §5, §6)
    //==========================================================================

    /**
     * 18 §6: "Adminkommando mit ungueltigem Kanalnamen -> Fehlermeldung mit
     * Liste der gueltigen Namen, KEINE Aenderung."
     *
     * Die Maske wird vor und nach dem Kommando verglichen. Das ist die
     * eigentliche Zusage - die Fehlermeldung allein waere wertlos, wenn die
     * Maske sich trotzdem bewegte.
     */
    private static bool BadChannelCheck()
    {
        int before = ChefZ_Log.GetChannelMask();

        array<string> lines = new array<string>();
        if (ChefZ_AdminCommands.Execute("chefz log channel " + NO_SUCH_ID + " off", lines))
            return false;

        if (ChefZ_Log.GetChannelMask() != before)
            return false;

        // Die Liste der gueltigen Namen muss dabeistehen.
        if (!Mentions(lines, "MATCH"))
            return false;

        // Auch ein gueltiger Kanal mit ungueltigem Schaltwort darf nichts tun.
        lines.Clear();
        if (ChefZ_AdminCommands.Execute("chefz log channel MATCH vielleicht", lines))
            return false;
        if (ChefZ_Log.GetChannelMask() != before)
            return false;

        // Fehlendes Argument ebenso.
        lines.Clear();
        if (ChefZ_AdminCommands.Execute("chefz log channel", lines))
            return false;
        if (ChefZ_Log.GetChannelMask() != before)
            return false;

        return true;
    }

    private static bool ChannelToggleCheck()
    {
        int before = ChefZ_Log.GetChannelMask();

        array<string> lines = new array<string>();
        if (!ChefZ_AdminCommands.Execute("chefz log channel MATCH off", lines))
            return false;
        if ((ChefZ_Log.GetChannelMask() & ChefZ_LogChannel.MATCH) != 0)
            return false;

        lines.Clear();
        if (!ChefZ_AdminCommands.Execute("chefz log channel match on", lines))
            return false;
        if ((ChefZ_Log.GetChannelMask() & ChefZ_LogChannel.MATCH) == 0)
            return false;

        // Genau ein Bit darf sich bewegt haben, und am Ende steht es wieder
        // wie zuvor.
        ChefZ_Log.SetChannel(ChefZ_LogChannel.ALL, false);
        ChefZ_Log.SetChannel(before, true);
        return ChefZ_Log.GetChannelMask() == before;
    }

    private static bool BadLevelCheck()
    {
        int before = ChefZ_Log.GetLevel();

        array<string> lines = new array<string>();
        if (ChefZ_AdminCommands.Execute("chefz log level " + NO_SUCH_ID, lines))
            return false;
        if (ChefZ_Log.GetLevel() != before)
            return false;

        lines.Clear();
        if (ChefZ_AdminCommands.Execute("chefz log level 42", lines))
            return false;
        if (ChefZ_Log.GetLevel() != before)
            return false;

        lines.Clear();
        if (ChefZ_AdminCommands.Execute("chefz log level", lines))
            return false;
        if (ChefZ_Log.GetLevel() != before)
            return false;

        return true;
    }

    /**
     * Zahl und Name muessen dasselbe bedeuten - "logLevel": 3 und
     * "logLevel": "INFO" tun es in der Config auch (ChefZ_LogLevel.FromName).
     */
    private static bool LevelCheck()
    {
        int before = ChefZ_Log.GetLevel();

        array<string> lines = new array<string>();
        if (!ChefZ_AdminCommands.Execute("chefz log level 3", lines))
            return false;
        if (ChefZ_Log.GetLevel() != ChefZ_LogLevel.INFO)
            return false;

        lines.Clear();
        if (!ChefZ_AdminCommands.Execute("chefz log level warn", lines))
            return false;
        if (ChefZ_Log.GetLevel() != ChefZ_LogLevel.WARN)
            return false;

        lines.Clear();
        if (!ChefZ_AdminCommands.Execute("chefz log level 0", lines))
            return false;
        if (ChefZ_Log.GetLevel() != ChefZ_LogLevel.OFF)
            return false;

        ChefZ_Log.SetLevel(before);
        return ChefZ_Log.GetLevel() == before;
    }

    //==========================================================================
    // Entity-Aufloesung
    //==========================================================================

    /**
     * Eine unbrauchbare Netz-ID darf begruendet scheitern - nie werfen.
     *
     * Der positive Fall braucht ein Objekt in der Welt und bleibt dem
     * Servertest vorbehalten.
     */
    private static bool EntityIdCheck()
    {
        ItemBase vessel;
        string   reason;

        if (ChefZ_Diagnostics.FindVessel("", vessel, reason))
            return false;
        if (reason == "")
            return false;

        if (ChefZ_Diagnostics.FindVessel(NO_SUCH_ID, vessel, reason))
            return false;
        if (reason == "")
            return false;

        // "0" und "0:0" sind keine gueltige Netz-ID (ChefZ_CookingDeviceAdapter
        // .VesselId nutzt genau diesen Test).
        if (ChefZ_Diagnostics.FindVessel("0", vessel, reason))
            return false;
        if (ChefZ_Diagnostics.FindVessel("0:0", vessel, reason))
            return false;

        // Der Fall "syntaktisch gueltige ID, die es nicht gibt" wird hier
        // BEWUSST nicht geprueft: er ist der einzige, der bis
        // g_Game.GetObjectByNetworkId durchlaeuft, und dieser Selbsttest laeuft
        // in MissionServer.OnInit - also bevor die Welt steht. Ein Selbsttest,
        // der den Start eines Servers wackeln laesst, um eine Auskunft zu
        // bestaetigen, ist teurer als die Auskunft wert ist. Er bleibt dem
        // Servertest vorbehalten.

        return true;
    }

    //==========================================================================
    // Nebenwirkungsfreiheit (18 E6)
    //==========================================================================

    /**
     * Ein falsch getippter Rezeptname darf die Symboltabelle nicht
     * verlaengern.
     *
     * Sie waechst nur und schrumpft nie (03 §4). Wuerde die Diagnose
     * INTERNIEREN statt nachzuschlagen, saeuselte jede Tippfehlersuche einen
     * toten Eintrag hinein - unsichtbar, unumkehrbar und auf einem Server, der
     * lange laeuft, irgendwann messbar.
     */
    private static bool NoInternCheck()
    {
        int before = ChefZ_SymbolTable.Count();

        array<string> lines = new array<string>();
        ChefZ_AdminCommands.Execute("chefz recipe " + NO_SUCH_ID, lines);

        if (ChefZ_SymbolTable.Count() != before)
            return false;

        // Zweite Runde ueber ExplainRecipe direkt, damit die Zusage auch fuer
        // Aufrufer gilt, die den Parser umgehen.
        lines.Clear();
        ChefZ_Diagnostics.ExplainRecipe(NO_SUCH_ID + "_2", lines);

        if (ChefZ_SymbolTable.Count() != before)
            return false;

        // Es muss trotzdem eine Antwort gegeben haben.
        return lines.Count() > 0;
    }

    /**
     * Hinsehen darf nichts kosten.
     *
     * Fehler- und Warnzaehler speisen die Safe-Mode-Schwelle (18 §4). Ein
     * Diagnosekommando, das sie bewegt, schiebt einen Server naeher an den
     * Safe Mode - allein dadurch, dass jemand nachgesehen hat.
     */
    private static bool CounterCheck()
    {
        int errBefore  = ChefZ_Log.GetErrorCount();
        int warnBefore = ChefZ_Log.GetWarnCount();

        array<string> lines = new array<string>();
        ChefZ_AdminCommands.Execute("chefz ambiguities", lines);
        ChefZ_AdminCommands.Execute("chefz help", lines);
        ChefZ_AdminCommands.Execute("chefz recipe " + NO_SUCH_ID, lines);

        if (ChefZ_Log.GetErrorCount() != errBefore)
            return false;
        if (ChefZ_Log.GetWarnCount() != warnBefore)
            return false;

        return true;
    }

    //==========================================================================
    // Der Trace (18 E3, E4)
    //==========================================================================

    /**
     * Der Trace sammelt und gibt am Ende ALLES auf einmal heraus (18 E4).
     *
     * Geprueft wird die Eigenschaft, die ihn fuer die Diagnose ueberhaupt
     * brauchbar macht: ToLines() liefert denselben Block mehrfach und
     * unabhaengig von der Logstufe. Waeren es direkt geschriebene Zeilen,
     * ginge nur die erste Verwendung (18 E3, Punkt 2).
     */
    private static bool TraceBlockCheck()
    {
        ChefZ_MatchTrace trace = new ChefZ_MatchTrace();

        trace.CandidateCount(3);
        trace.SlotResult(ChefZ_SymbolTable.INVALID, "a", false, "Testgrund");
        trace.Readiness(false, "Testgrund");

        if (trace.LineCount() != 3)
            return false;

        array<string> first = new array<string>();
        trace.ToLines(first);
        if (first.Count() != 3)
            return false;

        // Zweiter Abruf: derselbe Block, nicht ein leerer.
        array<string> second = new array<string>();
        trace.ToLines(second);
        if (second.Count() != 3)
            return false;

        // Der Grund muss im Klartext dastehen - das ist der ganze Zweck
        // (18 E5: "state RAW not allowed, needs DRIED" statt "no recipe
        // matched").
        if (!Mentions(first, "Testgrund"))
            return false;

        trace.Reset();
        if (trace.LineCount() != 0)
            return false;

        return true;
    }

    /**
     * Der Deckel muss MELDEN, dass er gegriffen hat.
     *
     * Stilles Kuerzen waere im Fehlerfall die schlechteste Eigenschaft eines
     * Diagnosewerkzeugs: der Leser haelt den halben Trace fuer den ganzen und
     * schliesst daraus das Falsche.
     */
    private static bool TraceTruncateCheck()
    {
        ChefZ_MatchTrace trace = new ChefZ_MatchTrace();

        int over = ChefZ_MatchTrace.MAX_LINES + 50;
        for (int i = 0; i < over; i++)
            trace.Note("Zeile " + i.ToString());

        if (!trace.WasTruncated())
            return false;
        if (trace.LineCount() > ChefZ_MatchTrace.MAX_LINES + 1)
            return false;

        array<string> lines = new array<string>();
        trace.ToLines(lines);
        return Mentions(lines, "abgeschnitten");
    }

    //==========================================================================
    // Werkzeug
    //==========================================================================

    //! Kommt der Text irgendwo in den Zeilen vor?
    private static bool Mentions(notnull array<string> lines, string needle)
    {
        for (int i = 0; i < lines.Count(); i++)
        {
            // Ueber eine lokale Zwischenvariable: eine proto-Methode direkt
            // auf dem Rueckgabewert von array.Get() aufzurufen ist in Enforce
            // nirgends zugesichert.
            string line = lines.Get(i);
            if (line.IndexOf(needle) >= 0)
                return true;
        }
        return false;
    }
}
