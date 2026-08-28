//==============================================================================
// ChefZ_AdminCommands - die "chefz ..."-Kommandozeile
//
// Entwurf: 18 §2.4 (Kommandoliste woertlich), 18 §5 (das Log ist der EINZIGE
// zur Laufzeit aenderbare Teil des Core), 18 §6 (Fehlerverhalten je Kommando),
// 18 E6 ("chefz match" ohne Nebenwirkung), 19 S18.
//
// ---------------------------------------------------------------------------
// Warum ein Textparser und KEIN Chat-Hook, kein RPC und kein Menue
// ---------------------------------------------------------------------------
// Drei Gruende, in dieser Reihenfolge:
//
//   1. 17 E8 legt fest, dass der Core KEINEN eigenen RPC hat. Ein
//      Diagnose-RPC waere ein vom Client erreichbarer Einstiegspunkt in
//      Servercode - fuer ein Werkzeug, das genau eine Person auf dem Server
//      benutzt, ist das ein schlechtes Tauschgeschaeft.
//   2. Vanilla routet Chat serverseitig nicht ins Skript. MissionServer.OnEvent
//      kennt ClientPrepare, ClientNew, ClientReady, ClientRespawn und
//      ClientDisconnect - keinen Chat (missionServer.c:299). Ein Chat-Kommando
//      brauchte also erst einen eigenen Uebertragungsweg, siehe Punkt 1.
//   3. Jede Adminoberflaeche, die es auf einem Server ohnehin gibt, kann eine
//      Zeichenkette weiterreichen. Genau das ist die Schnittstelle hier:
//
//          array<string> answer;
//          ChefZ_AdminCommands.Execute("chefz why 4711 <rezeptId>", answer);
//
//      Execute() ist rein und gibt Zeilen zurueck. Run() schreibt dieselben
//      Zeilen zusaetzlich ins RPT. Wer eine eigene Oberflaeche hat, ruft
//      Execute und zeigt das Ergebnis an, wo er will.
//
// Der Core bringt bewusst KEINE Oberflaeche mit. Eine mitgelieferte
// Adminoberflaeche waere Content, und Content gehoert nicht in den Core.
//
// ---------------------------------------------------------------------------
// Serverseitig, ausnahmslos
// ---------------------------------------------------------------------------
// Jedes Kommando laeuft ueber ChefZ_Diagnostics, und jede Funktion dort prueft
// zuerst g_Game.IsServer(). Auf einem Client bekommt der Aufrufer eine
// erklaerende Zeile und sonst nichts.
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_AdminCommands
{
    static const string PREFIX = "chefz";

    //! Obergrenze der Wortzahl einer Kommandozeile. Ein Kommando hat vier
    //! Woerter; alles darueber ist entweder ein Tippfehler oder ein Versuch,
    //! den Parser zu beschaeftigen.
    static const int MAX_TOKENS = 16;

    /**
     * Kommando auswerten und die Antwortzeilen zurueckgeben.
     *
     * @return true, wenn das Kommando erkannt UND ausgefuehrt wurde. false
     *         heisst "nicht erkannt oder Argument fehlt" - outLines traegt
     *         dann eine Begruendung samt gueltiger Werte (18 §6).
     *
     * Wirft nie. Ein Diagnosewerkzeug, das den Server anhalten kann, ist
     * schlimmer als keines.
     */
    static bool Execute(string commandLine, out array<string> outLines)
    {
        array<string> lines = outLines;
        if (!lines)
            lines = new array<string>();

        array<string> tok = new array<string>();
        Tokenize(commandLine, tok);

        // Das fuehrende "chefz" ist optional: wer den Parser direkt ruft, hat
        // es schon gewusst.
        if (tok.Count() > 0 && Lower(tok.Get(0)) == PREFIX)
            tok.RemoveOrdered(0);

        if (tok.Count() == 0)
        {
            Help(lines);
            outLines = lines;
            return false;
        }

        string verb = Lower(tok.Get(0));
        bool ok = Dispatch(verb, tok, lines);

        outLines = lines;
        return ok;
    }

    /**
     * Wie Execute, schreibt die Antwort aber zusaetzlich blockweise ins RPT
     * (18 §4: "Ausgabe ins RPT und an den Aufrufer").
     */
    static bool Run(string commandLine)
    {
        array<string> lines = new array<string>();
        bool ok = Execute(commandLine, lines);
        ChefZ_Diagnostics.Emit(lines);
        return ok;
    }

    //==========================================================================

    private static bool Dispatch(string verb, notnull array<string> tok, notnull array<string> lines)
    {
        if (verb == "help" || verb == "?")
        {
            Help(lines);
            return true;
        }

        if (verb == "log")
            return CmdLog(tok, lines);

        if (verb == "match")
            return CmdMatch(tok, lines);

        if (verb == "why")
            return CmdWhy(tok, lines);

        if (verb == "registries")
        {
            ChefZ_Diagnostics.DumpRegistries(lines);
            return true;
        }

        if (verb == "audit")
        {
            ChefZ_Diagnostics.DumpNutritionAudit(lines);
            return true;
        }

        if (verb == "stats")
        {
            ChefZ_Diagnostics.DumpPerfCounters(lines);
            return true;
        }

        if (verb == "categories")
        {
            ChefZ_Diagnostics.DumpCategoryTree(lines);
            return true;
        }

        if (verb == "symbols")
        {
            ChefZ_Diagnostics.DumpSymbols(lines);
            return true;
        }

        if (verb == "ambiguities")
        {
            ChefZ_Diagnostics.DumpAmbiguities(lines);
            return true;
        }

        if (verb == "report")
        {
            ChefZ_Diagnostics.DumpLoadReport(lines);
            return true;
        }

        if (verb == "recipe")
            return CmdRecipe(tok, lines);

        // Das ORIGINAL zurueckgeben und nicht die kleingeschriebene Form: wer
        // sich vertippt hat, soll seinen eigenen Tippfehler wiedererkennen.
        lines.Insert("Unbekanntes Kommando \"" + tok.Get(0) + "\".");
        Help(lines);
        return false;
    }

    //==========================================================================
    // chefz log
    //==========================================================================

    /**
     * 18 §5: das Log ist der einzige Teil des Core, dessen Konfiguration zur
     * Laufzeit aenderbar ist - "einen Fehler nachzustellen soll keinen
     * Serverneustart kosten". Die Aenderung ist NICHT persistent; nach einem
     * Neustart gilt wieder, was in Core.json steht.
     */
    private static bool CmdLog(notnull array<string> tok, notnull array<string> lines)
    {
        if (tok.Count() < 2)
        {
            LogStatus(lines);
            return true;
        }

        string sub = Lower(tok.Get(1));

        if (sub == "status")
        {
            LogStatus(lines);
            return true;
        }

        if (sub == "level")
        {
            if (tok.Count() < 3)
            {
                lines.Insert("chefz log level <0..5>   gueltig: " + ChefZ_LogLevel.ValidNames());
                return false;
            }

            string raw = tok.Get(2);
            int level = ChefZ_LogLevel.FromName(raw);
            if (level < 0)
            {
                // Ziffern sind ebenfalls erlaubt - "logLevel": 3 und
                // "logLevel": "INFO" bedeuten dasselbe (ChefZ_LogLevel).
                int asNumber = raw.ToInt();
                if (asNumber == 0 && raw != "0")
                {
                    lines.Insert("\"" + raw + "\" ist keine Logstufe. Gueltig: " + ChefZ_LogLevel.ValidNames() + " oder 0..5.");
                    return false;
                }
                level = asNumber;
            }

            if (!ChefZ_LogLevel.IsValid(level))
            {
                lines.Insert("Stufe " + level.ToString() + " liegt ausserhalb 0..5.");
                return false;
            }

            ChefZ_Log.SetLevel(level);

            if (level >= ChefZ_LogLevel.TRACE)
            {
                // 18 §6: TRACE funktioniert, ist aber teuer. Dieselbe Warnung
                // wie in Configure() - hier ist sie sogar wichtiger, denn wer
                // sie zur Laufzeit setzt, setzt sie meist am laufenden Server.
                lines.Insert("ACHTUNG: TRACE ist auf einem Produktivserver teuer. " + "Nach der Fehlersuche wieder zuruecksetzen.");
            }

            LogStatus(lines);
            return true;
        }

        if (sub == "channel")
        {
            if (tok.Count() < 4)
            {
                lines.Insert("chefz log channel <name> on|off   gueltig: " + ChefZ_LogChannel.ValidNames());
                return false;
            }

            string channelName = tok.Get(2);
            int channel = ChefZ_LogChannel.FromName(channelName);
            if (channel == 0)
            {
                // 18 §6: "Adminkommando mit ungueltigem Kanalnamen ->
                // Fehlermeldung mit Liste der gueltigen Namen, KEINE
                // Aenderung."
                lines.Insert("Unbekannter Kanal \"" + channelName + "\". Gueltig: " + ChefZ_LogChannel.ValidNames() + ". Nichts geaendert.");
                return false;
            }

            string state = Lower(tok.Get(3));
            bool on;
            if (state == "on" || state == "an" || state == "1")
                on = true;
            else if (state == "off" || state == "aus" || state == "0")
                on = false;
            else
            {
                lines.Insert("\"" + state + "\" ist weder on noch off. Nichts geaendert.");
                return false;
            }

            ChefZ_Log.SetChannel(channel, on);
            LogStatus(lines);
            return true;
        }

        lines.Insert("chefz log level <0..5> | chefz log channel <name> on|off | chefz log status");
        return false;
    }

    private static void LogStatus(notnull array<string> lines)
    {
        int mask = ChefZ_Log.GetChannelMask();

        lines.Insert("Log: Stufe " + ChefZ_LogLevel.Name(ChefZ_Log.GetLevel()) + "   Datei " + ChefZ_Log.IsFileOutputActive().ToString() + " (" + ChefZ_Log.GetLogFilePath() + ")" + "   Fehler " + ChefZ_Log.GetErrorCount().ToString() + "   Warnungen " + ChefZ_Log.GetWarnCount().ToString());

        string active = "";
        string inactive = "";
        for (int bit = 0; bit < 13; bit++)
        {
            int single = 1 << bit;
            string name = ChefZ_LogChannel.Name(single);
            if ((mask & single) != 0)
                active = active + " " + name;
            else
                inactive = inactive + " " + name;
        }

        lines.Insert("  an: " + active);
        lines.Insert("  aus:" + inactive);
    }

    //==========================================================================
    // chefz match / why / recipe
    //==========================================================================

    /**
     * 18 §2.4: "chefz match <entityId>  einmaliger Vollmatch mit Trace, OHNE
     * NEBENWIRKUNG."
     *
     * Die Nebenwirkungsfreiheit steht nicht hier, sondern in
     * ChefZ_Diagnostics - dieses Kommando loest nur die Entity auf. Die
     * Begruendung, warum die Trennung nichts kostet, steht in 18 E6:
     * Evaluate() ist ohnehin rein lesend entworfen.
     */
    private static bool CmdMatch(notnull array<string> tok, notnull array<string> lines)
    {
        if (tok.Count() < 2)
        {
            lines.Insert("chefz match <entityId>   Netz-ID des Gefaesses, " + "als <low> oder <low>:<high>.");
            return false;
        }

        ItemBase vessel;
        string   why;
        if (!ChefZ_Diagnostics.FindVessel(JoinId(tok, 1), vessel, why))
        {
            lines.Insert(why);
            return false;
        }

        ChefZ_Diagnostics.ExplainDevice(vessel, lines);
        return true;
    }

    private static bool CmdWhy(notnull array<string> tok, notnull array<string> lines)
    {
        if (tok.Count() < 3)
        {
            lines.Insert("chefz why <entityId> <recipeId>   gezielte " + "Ablehnungsbegruendung fuer EIN Rezept an EINEM Gefaess.");
            return false;
        }

        // Die Entity-ID kann in zwei Woertern stehen ("<low> <high>"); das
        // Rezept ist dann das dritte. Unterschieden wird an der Form: eine
        // Rezept-ID ist nie eine reine Ziffernfolge.
        int recipeAt = 2;
        string idPart = tok.Get(1);
        if (tok.Count() >= 4 && IsNumeric(tok.Get(2)))
        {
            idPart   = tok.Get(1) + ":" + tok.Get(2);
            recipeAt = 3;
        }

        ItemBase vessel;
        string   why;
        if (!ChefZ_Diagnostics.FindVessel(idPart, vessel, why))
        {
            lines.Insert(why);
            return false;
        }

        ChefZ_Diagnostics.WhyNotMatched(vessel, tok.Get(recipeAt), lines);
        return true;
    }

    private static bool CmdRecipe(notnull array<string> tok, notnull array<string> lines)
    {
        if (tok.Count() < 2)
        {
            lines.Insert("chefz recipe <recipeId>   das kompilierte Rezept im Klartext.");
            return false;
        }

        ChefZ_Diagnostics.ExplainRecipe(tok.Get(1), lines);
        return true;
    }

    //==========================================================================
    // Hilfe
    //==========================================================================

    private static void Help(notnull array<string> lines)
    {
        lines.Insert("ChefZ Adminkommandos (18 §2.4). Ausschliesslich serverseitig.");
        lines.Insert("  chefz log level <0..5>            Logstufe zur Laufzeit setzen");
        lines.Insert("  chefz log channel <name> on|off   einzelnen Kanal schalten");
        lines.Insert("  chefz log status                  Stufe, Kanaele, Zaehler");
        lines.Insert("  chefz match <entityId>            Vollmatch mit Trace, OHNE Nebenwirkung");
        lines.Insert("  chefz why <entityId> <recipeId>   warum bindet genau dieses Rezept nicht");
        lines.Insert("  chefz recipe <recipeId>           das kompilierte Rezept im Klartext");
        lines.Insert("  chefz registries                  Bestand, Zustand, Fehlerzahlen");
        lines.Insert("  chefz report                      der Ladebericht des Starts");
        lines.Insert("  chefz audit                       Naehrwert-Startaudit erneut ausgeben");
        lines.Insert("  chefz ambiguities                 verdeckte Rezepte und Rangordnung");
        lines.Insert("  chefz categories                  Kategoriebaum");
        lines.Insert("  chefz symbols                     Symboltabelle");
        lines.Insert("  chefz stats                       Sitzungen, Matchzaehler, PERF-Mittel");
        lines.Insert("Kein Kommando veraendert den Spielzustand. \"match\" und \"why\" " + "werten aus und wenden nie an (18 E6).");
    }

    //==========================================================================
    // Zerlegung
    //==========================================================================

    /**
     * Zeile in Woerter zerlegen - an Leerzeichen und Tabulatoren.
     *
     * Bewusst von Hand und NICHT ueber string.ParseStringEx: jenes zerlegt
     * zusaetzlich an Sonderzeichen und liefert Typen zurueck, die hier
     * niemand braucht. Eine Rezept-ID darf Unterstriche und Ziffern
     * enthalten; ein Parser, der sie in Stuecke schneidet, waere in diesem
     * Werkzeug genau falsch.
     */
    private static void Tokenize(string line, notnull array<string> outTokens)
    {
        outTokens.Clear();

        int len = line.Length();
        int i = 0;
        string current = "";

        while (i < len && outTokens.Count() < MAX_TOKENS)
        {
            string ch = line.Substring(i, 1);
            i++;

            if (ch == " " || ch == "\t")
            {
                if (current != "")
                {
                    outTokens.Insert(current);
                    current = "";
                }
                continue;
            }

            current = current + ch;
        }

        if (current != "" && outTokens.Count() < MAX_TOKENS)
            outTokens.Insert(current);
    }

    //! Entity-ID aus den restlichen Woertern. "<low> <high>" wird zu
    //! "<low>:<high>" zusammengezogen, damit FindVessel nur eine Form kennen
    //! muss.
    private static string JoinId(notnull array<string> tok, int from)
    {
        if (from >= tok.Count())
            return "";

        string id = tok.Get(from);
        if (from + 1 < tok.Count() && IsNumeric(tok.Get(from + 1)))
            id = id + ":" + tok.Get(from + 1);
        return id;
    }

    /**
     * Reine Ziffernfolge?
     *
     * Ueber ToAscii() und nicht ueber einen Zeichenvergleich mit "<" und ">":
     * Enforce hat keine zugesicherte Ordnungsrelation auf string. ToAscii()
     * liefert den Code des ERSTEN Zeichens (EnString.c:86) - deshalb wird
     * jedes Zeichen einzeln herausgeschnitten.
     */
    private static bool IsNumeric(string text)
    {
        int len = text.Length();
        if (len == 0)
            return false;

        for (int i = 0; i < len; i++)
        {
            string ch = text.Substring(i, 1);
            int code = ch.ToAscii();
            if (code < 48 || code > 57)     // '0' .. '9'
                return false;
        }
        return true;
    }

    private static string Lower(string text)
    {
        string s = text;
        s.ToLower();
        return s;
    }
}
