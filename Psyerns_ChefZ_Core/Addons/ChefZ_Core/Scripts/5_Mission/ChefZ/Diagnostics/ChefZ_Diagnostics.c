//==============================================================================
// ChefZ_Diagnostics - die Gegenleistung fuer die Datengetriebenheit
//
// Entwurf: 18 §2.4 (Funktionsliste woertlich), 18 §3 (Ausgabeformat),
// 18 E3 (Trace als Null-Objekt, hier ERZWUNGEN statt gewacht),
// 18 E5 (WhyNotMatched als erstklassige Funktion),
// 18 E6 (chefz match ohne Nebenwirkung), 19 S18.
//
// ---------------------------------------------------------------------------
// Warum dieses Teilsystem kein Nebenschauplatz ist
// ---------------------------------------------------------------------------
// Der Kernentwurf tauscht Compilezeit-Sicherheit gegen Datengetriebenheit
// (03 E1). Wer wissen will, warum ein Rezept nicht zuendet, kann keinen Code
// lesen - den Code gibt es nicht, es gibt nur Daten. Er muss einen Trace
// lesen. Dieses Teilsystem IST dieser Trace.
//
// 18 E5 sagt es so: "In einem System ohne Compilezeit-Sicherheit ist das kein
// Komfort, sondern die Gegenleistung."
//
// ---------------------------------------------------------------------------
// Die harte Zusage: NEBENWIRKUNGSFREI (18 E6)
// ---------------------------------------------------------------------------
// Kein Aufruf in dieser Datei erzeugt ein Item, verbraucht eines, veraendert
// einen Zustand, oeffnet eine Kochsitzung oder schreibt in eine Registry.
// Konkret und ueberpruefbar:
//
//   1. EIGENE Puffer. Kontext, Faktenliste und Entityliste werden hier
//      angelegt und leben nur waehrend des Kommandos. Die gleichnamigen
//      FELDER des Kochadapters (m_Ctx, m_Snapshot, m_Entities) werden NICHT
//      angefasst - ein Diagnosekommando mitten in einem Kochtick duerfte den
//      Tick sonst um seine Datengrundlage bringen.
//   2. KEIN Applicator. Die Transaktion aus 08 wird nirgends gerufen. Der
//      Grund steht in 18 E6: "Ein Diagnosekommando, das kochen kann, ist ein
//      Diagnosekommando, das etwas kaputtmachen kann."
//   3. KEINE Sitzung. GetSession() legt an, wenn nichts da ist - deshalb wird
//      es hier nie gerufen. Gelesen wird ausschliesslich ueber
//      GetActiveSessionCount() und DumpStats().
//   4. Lookup statt Intern. Ein Rezeptname mit Tippfehler darf die
//      Symboltabelle nicht dauerhaft um einen toten Eintrag verlaengern
//      (03 §4). ChefZ_SymbolTable.Lookup liefert INVALID und legt nichts an.
//   5. Der Ladebericht des Boots bleibt unberuehrt. Wo eine Analyse einen
//      Bericht braucht, bekommt sie einen eigenen, leeren - die Fehlerzahl des
//      Boots speist die Safe-Mode-Schwelle (18 §4) und darf sich durch Hinsehen
//      nicht aendern.
//
// Das kostet nichts, weil Evaluate() ohnehin rein lesend entworfen ist
// (08 E-Layer). Genau das ist der praktische Nutzen jener Entscheidung.
//
// ---------------------------------------------------------------------------
// Was NICHT nebenwirkungsfrei ist - und warum es trotzdem in Ordnung geht
// ---------------------------------------------------------------------------
// ResolveDevice fuellt den Geraetezwischenspeicher des Adapters, wenn die
// Klasse dort noch nicht steht. Das ist ein CACHE, keine Spielentscheidung:
// derselbe Eintrag entstuende beim naechsten Kochtick ohnehin, er ist aus der
// Config abgeleitet, und er veraendert kein Verhalten. Ihn zu umgehen hiesse,
// die Aufloesung nachzubauen - und ein Nachbau saehe die Vererbung entlang
// CfgVehicles anders als der Adapter. Dann beantwortete die Diagnose eine
// andere Frage als die gestellte, was schlimmer ist.
//
// ---------------------------------------------------------------------------
// Ausschliesslich Server
// ---------------------------------------------------------------------------
// 18 §4: "Es gibt keinen Client-Log-Kanal fuer Spiellogik; ein Client hat
// keine ChefZ-Entscheidung zu protokollieren." Jede Funktion hier prueft das
// zuerst und liefert sonst eine einzige erklaerende Zeile.
//
// Layer: 5_Mission. Hier und nicht in 4_World, weil Diagnose zur
// Missionslebensdauer gehoert und nichts davon im Spielablauf gerufen wird.
//==============================================================================

class ChefZ_Diagnostics
{
    //! Obergrenze einer einzelnen Antwort. Ein Registryauszug auf einem Server
    //! mit vielen Content-Modulen kann fuenfstellig werden; das RPT waere
    //! danach unbrauchbar. Der Deckel MELDET, dass er gegriffen hat - stilles
    //! Kuerzen waere die schlechteste Eigenschaft eines Diagnosewerkzeugs
    //! (dieselbe Regel wie ChefZ_MatchTrace.MAX_LINES).
    static const int MAX_ANSWER_LINES = 4000;

    //! Wie viele passende Rezepte "chefz match" zusaetzlich zum Sieger nennt.
    //! EvaluateAll kostet die volle Auswertung ALLER Kandidaten (08 E3) und
    //! ist deshalb ausdruecklich nichts fuer den Kochtick - fuer ein
    //! Adminkommando ist es genau richtig.
    static const int MAX_ALTERNATIVES = 8;

    private static const string RULE = "----------------------------------------";

    //! Die haeufigste Zeile des ganzen Teilsystems (18 §3, letzter Block).
    //! Sie steht als Konstante da, weil sie die Kernzusage des Mods in einem
    //! Satz wiederholt: kein ChefZ-Match, dann Vanilla-Pfad unveraendert.
    private static const string VANILLA_UNTOUCHED = "Kein Treffer -> Vanilla-Kochen laeuft unveraendert weiter.";

    //==========================================================================
    // Torwaechter
    //==========================================================================

    /**
     * Darf hier ueberhaupt etwas ausgewertet werden?
     *
     * @return false mit einer Begruendung, die dem Fragenden sagt, was zu tun
     *         ist - nicht "nicht verfuegbar".
     */
    static bool IsAvailable(out string reason)
    {
        reason = "";

        if (!g_Game || !g_Game.IsServer())
        {
            reason = "Diagnose laeuft ausschliesslich serverseitig (18 §4). " + "Ein Client trifft keine ChefZ-Entscheidung und hat deshalb " + "auch keine zu erklaeren.";
            return false;
        }

        if (!ChefZ_Boot.HasBootedServer())
        {
            reason = "Der Core hat noch nicht gebootet. MissionServer.OnInit " + "laeuft frueh, aber nicht vor dem ersten Tastendruck.";
            return false;
        }

        return true;
    }

    //==========================================================================
    // Bestand (18 §2.4)
    //==========================================================================

    static void DumpRegistries(out array<string> outLines)
    {
        array<string> lines = Begin(outLines, "Registries");
        if (!Guard(lines))
        {
            outLines = lines;
            return;
        }

        ChefZ_ConfigManager cfg = ChefZ_ConfigManager.Get();

        lines.Insert("Zustand   " + ChefZ_ConfigManager.HealthName(cfg.GetHealth()) + "   aktiv=" + cfg.IsActive().ToString() + "   records=" + cfg.TotalRecordCount().ToString());
        lines.Insert("Logzaehler seit Start: " + ChefZ_Log.GetErrorCount().ToString() + " Fehler, " + ChefZ_Log.GetWarnCount().ToString() + " Warnungen");
        lines.Insert("Symbole   " + ChefZ_SymbolTable.Count().ToString());
        lines.Insert("");

        cfg.DumpRegistries(lines);

        // Die Rezept-Engine, die Zustaende und die uebrigen Manager stehen
        // nicht in den Registries: sie sind das ERGEBNIS ihrer Auswertung.
        // Ein Betreiber, der "registries" tippt, will den Bestand sehen, den
        // der Server tatsaechlich benutzt - nicht den, den er geladen hat.
        lines.Insert("");
        ChefZ_StateManager.Get().DumpStates(lines);
        ChefZ_PreservationManager.Get().DumpRules(lines);
        ChefZ_ToolRegistry.Get().DumpTools(lines);
        ChefZ_ProcessingManager.Get().DumpProcessing(lines);
        ChefZ_PortionManager.Get().DumpSpecs(lines);
        ChefZ_ContainerRegistry.Get().DumpContainers(lines);
        ChefZ_HandcraftBridge.DumpHandcraft(lines);
        ChefZ_RecipeEngine.Get().DumpRecipes(lines);

        // Die Aussenkante zuletzt: sie beantwortet die haeufigste
        // Betreiberfrage ("warum tut mein Comp-Modul nichts"), und wer bis
        // hierher gelesen hat, sucht meist genau danach.
        lines.Insert("");
        ChefZ_EventBus.Get().DumpSubscribers(lines);
        ChefZ_CapabilityRegistry.Get().DumpProviders(lines);
        ChefZ_ProgressRegistry.Dump(lines);

        outLines = End(lines);
    }

    static void DumpCategoryTree(out array<string> outLines)
    {
        array<string> lines = Begin(outLines, "Kategoriebaum");
        if (Guard(lines))
            ChefZ_CategoryManager.Get().DumpTree(lines);
        outLines = End(lines);
    }

    static void DumpSymbols(out array<string> outLines)
    {
        array<string> lines = Begin(outLines, "Symboltabelle");
        if (Guard(lines))
            ChefZ_SymbolTable.DebugDump(lines);
        outLines = End(lines);
    }

    /**
     * Das Naehrwert-Startaudit erneut ausgeben (13 §7, 18 §2.4 "chefz audit").
     *
     * Erneut AUSGEBEN und nicht erneut RECHNEN: das Audit lief beim Boot ueber
     * den damaligen Bestand, und der Bestand aendert sich zur Laufzeit nicht
     * (02 E5). Ein zweiter Lauf ergaebe dieselben Zahlen und kostete einen
     * vollen Durchlauf durch alle Rezepte.
     */
    static void DumpNutritionAudit(out array<string> outLines)
    {
        array<string> lines = Begin(outLines, "Naehrwertaudit");
        if (Guard(lines))
        {
            ChefZ_NutritionManager nut = ChefZ_NutritionManager.Get();
            string chefzTxt1 = "Angaben " + nut.GetRecordCount().ToString() + "   abgewiesen " + nut.GetRejectedCount().ToString() + "   Audit ";
            chefzTxt1 = chefzTxt1 + nut.IsAuditEnabled().ToString() + "   gelaufen " + nut.HasAudited().ToString();
            lines.Insert(chefzTxt1);
            nut.DumpFindings(lines);
        }
        outLines = End(lines);
    }

    static void DumpAmbiguities(out array<string> outLines)
    {
        array<string> lines = Begin(outLines, "Ambiguitaeten");
        if (Guard(lines))
        {
            ChefZ_RecipeEngine.Get().DumpAmbiguities(lines);
            lines.Insert("");
            ChefZ_RecipeEngine.Get().ExplainOrder(lines);
        }
        outLines = End(lines);
    }

    static void DumpPerfCounters(out array<string> outLines)
    {
        array<string> lines = Begin(outLines, "Zaehler");
        if (Guard(lines))
        {
            ChefZ_CookingDeviceAdapter.Get().DumpStats(lines);
            lines.Insert("");
            ChefZ_ProcessRunner.DumpStats(lines);
            lines.Insert("");
            ChefZ_Log.DumpPerfCounters(lines);
        }
        outLines = End(lines);
    }

    /**
     * Der Ladebericht, wie er beim Boot entstanden ist (18 §2.3).
     *
     * Er ist ein eigenes Objekt und kein Logstrom, weil Ladefehler nach dem
     * Start noch ABFRAGBAR sein muessen - genau dafuer ist diese Zeile da.
     */
    static void DumpLoadReport(out array<string> outLines)
    {
        array<string> lines = Begin(outLines, "Ladebericht");
        if (Guard(lines))
        {
            ChefZ_LoadReport report = ChefZ_ConfigManager.Get().GetReport();
            if (report)
                report.ToLines(lines);
            else
                lines.Insert("Kein Ladebericht vorhanden.");
        }
        outLines = End(lines);
    }

    //==========================================================================
    // Ein einzelnes Rezept (18 §2.4)
    //==========================================================================

    static void ExplainRecipe(string recipeId, out array<string> outLines)
    {
        array<string> lines = Begin(outLines, "Rezept " + recipeId);
        if (!Guard(lines))
        {
            outLines = End(lines);
            return;
        }

        ChefZ_CompiledRecipe rec = FindRecipeByName(recipeId);
        if (!rec)
        {
            NoSuchRecipe(recipeId, lines);
            outLines = End(lines);
            return;
        }

        lines.Insert(rec.ToDebugString());
        lines.Insert("Quelle        " + rec.sourceRef);
        string chefzTxt2 = "Spezifitaet   " + rec.specificity.ToString() + "   Prioritaet " + rec.priority.ToString() + "   Pflichtslots ";
        chefzTxt2 = chefzTxt2 + rec.requiredSlots.ToString() + "   Bedingungen " + rec.totalConstraints.ToString();
        lines.Insert(chefzTxt2);
        lines.Insert("Mindestitems  " + rec.minItemCount.ToString());
        lines.Insert("");

        for (int i = 0; i < rec.contexts.Count(); i++)
            lines.Insert("  Kontext  " + rec.contexts.Get(i).ToDebugString());

        for (int s = 0; s < rec.slots.Count(); s++)
            lines.Insert("  Slot     " + rec.slots.Get(s).ToDebugString());

        if (rec.policy)
            lines.Insert("  Politik  " + rec.policy.ToDebugString());

        outLines = End(lines);
    }

    //==========================================================================
    // Ein Gefaess: "chefz match" (18 §3)
    //==========================================================================

    /**
     * Der Vollmatch mit Trace - der Block aus 18 §3.
     *
     * Der Trace wird hier ERZWUNGEN (new statt CreateIfEnabled): ein Admin,
     * der ausdruecklich fragt, soll die Antwort bekommen, ohne vorher den
     * Kanal MATCH auf DEBUG stellen zu muessen. Genau dafuer ist das
     * Null-Objekt aus 18 E3 mehrfach verwendbar: derselbe Matcher laeuft im
     * Produktivpfad mit null und hier mit vollem Protokoll - ohne zwei
     * Codepfade.
     */
    static void ExplainDevice(ItemBase device, out array<string> outLines)
    {
        array<string> lines = Begin(outLines, "Gefaess " + Describe(device));
        if (!Guard(lines))
        {
            outLines = End(lines);
            return;
        }

        if (!device)
        {
            lines.Insert("Kein Gefaess. Die Entity-ID zeigt auf nichts, was noch existiert.");
            outLines = End(lines);
            return;
        }

        ChefZ_DeviceDescriptor  desc;
        ChefZ_CookContext       ctx;
        ChefZ_FactSnapshot      snapshot;
        array<ItemBase>         entities;
        string                  why;

        if (!BuildView(device, desc, ctx, snapshot, entities, why))
        {
            lines.Insert(why);
            lines.Insert(VANILLA_UNTOUCHED);
            outLines = End(lines);
            return;
        }

        ChefZ_RecipeEngine engine = ChefZ_RecipeEngine.Get();

        lines.Insert("Geraet        " + desc.ToDebugString());
        lines.Insert("Methode       " + ChefZ_SymbolTable.NameOrMark(ctx.method) + "   Temperatur " + ctx.deviceTemperature.ToString() + "   Items " + snapshot.Count().ToString());
        if (ctx.HasLiquid())
        {
            lines.Insert("Fluessigkeit  " + ChefZ_SymbolTable.NameOrMark(ctx.liquidType) + " " + ctx.liquidQuantity.ToString());
        }
        lines.Insert("Torstufen     " + GateSummary(device, desc, snapshot.Count()));

        // Nur GEZAEHLT, nie geholt: GetSession() LEGT AN, wenn es nichts
        // findet - und eine Sitzung, die durch blosses Hinsehen entsteht,
        // waere genau die Nebenwirkung, die 18 E6 ausschliesst.
        int sessions = ChefZ_CookingDeviceAdapter.Get().GetActiveSessionCount();
        lines.Insert("Sitzungen     " + sessions.ToString() + " aktiv (serverweit)");

        if (snapshot.Count() == 0)
        {
            // 18 §6: "chefz match auf einem Gefaess ohne Cargo -> Trace mit
            // 'kein Cargo', keine Wirkung."
            lines.Insert("Kein Cargo. ChefZ-Rezepte sind gefaessbasiert (10 §3); " + "ohne Inhalt gibt es nichts zu binden.");
            lines.Insert(VANILLA_UNTOUCHED);
            outLines = End(lines);
            return;
        }

        // ---- der eigentliche Trace ------------------------------------------
        //
        // Erzwungen und auf die Antwortliste umgeleitet: die Zeilen gehen an
        // den Fragenden, nicht in den Logstrom. Emit() wird deshalb NICHT
        // gerufen, ToLines() schon.
        ChefZ_MatchTrace trace = new ChefZ_MatchTrace();

        ChefZ_MatchResult result;
        bool matched = engine.EvaluateBest(ctx, snapshot, trace, result);

        lines.Insert("");
        array<string> traceLines = new array<string>();
        trace.ToLines(traceLines);
        for (int t = 0; t < traceLines.Count(); t++)
            lines.Insert(traceLines.Get(t));

        lines.Insert("");
        if (matched)
        {
            lines.Insert("TREFFER  " + result.ToDebugString());
            if (!result.ready)
                lines.Insert("  Noch nicht fertig: " + result.notReadyReason);
            AppendPlan(result, snapshot, lines);
        }
        else
        {
            // Die haeufigste und wichtigste Zeile des ganzen Teilsystems
            // (18 §3, letzter Block).
            lines.Insert("KEIN TREFFER");
            lines.Insert("  Kandidaten geprueft " + result.candidatesTried.ToString() + "   Knoten " + result.nodesExplored.ToString());
            if (result.failReason != "")
            {
                lines.Insert("  Bestplatzierter Fehlschlag: " + ChefZ_SymbolTable.NameOrMark(result.failedRecipe) + " -> " + result.failReason + SlotSuffix(result.failSlotId));
            }
            lines.Insert(VANILLA_UNTOUCHED);
        }

        // Die uebrigen bindbaren Rezepte. Sie beantworten "was HAETTE hier
        // noch gepasst" und sind der zweite Grund, warum EvaluateAll
        // existiert (08 E3: ausdruecklich nicht fuer den Kochtick).
        AppendAlternatives(engine, ctx, snapshot, lines);

        lines.Insert("");
        lines.Insert("Nichts davon hat gewirkt: ausgewertet, nicht angewandt (18 E6).");

        outLines = End(lines);
    }

    //==========================================================================
    // "chefz why" - die wichtigste Funktion des Teilsystems (18 E5)
    //==========================================================================

    /**
     * Warum bindet GENAU DIESES Rezept an GENAU DIESEM Gefaess nicht?
     *
     * 18 E5: sie beantwortet die haeufigste Frage eines Content-Autors direkt
     * am Objekt, und sie ist der Grund, warum die Pruefreihenfolge im Selektor
     * festgelegt ist (07 E6) - die Antwort soll den verletzten Slot mit
     * Begruendung nennen, nicht "kein Rezept gematcht".
     *
     * Die Auskunft kommt aus ZWEI Quellen, und das ist Absicht:
     *
     *   1. Evaluate() mit erzwungenem Trace. Das ist der echte Bindungslauf
     *      mit Backtracking. Er nennt den ERSTEN Grund, an dem es scheitert -
     *      in der Reihenfolge Kontext, Werkzeug, Faehigkeit, Slots, Politik.
     *   2. BuildPartial() ueber EvaluatePartial(). Das ist die slotweise
     *      Betrachtung OHNE Zuordnung: sie sieht auch die Slots hinter dem
     *      ersten Fehlschlag und sagt fuer jeden, wie viele Kandidaten es
     *      gaebe und warum das erste Item nicht passte.
     *
     * Punkt 1 allein verschwiege alles hinter dem ersten Fehler; ein Autor mit
     * drei falschen Slots kaeme dreimal wieder. Punkt 2 allein verschwiege den
     * Fall, in dem sich zwei Slots dasselbe Item teilen wollen - dort sieht
     * jeder Slot fuer sich erfuellt aus, und nur der Bindungslauf merkt es.
     * Der Bericht sagt genau das ausdruecklich (ChefZ_PartialMatchReport).
     */
    static void WhyNotMatched(ItemBase device, string recipeId, out array<string> outLines)
    {
        array<string> lines = Begin(outLines, "Warum nicht: " + recipeId + " an " + Describe(device));
        if (!Guard(lines))
        {
            outLines = End(lines);
            return;
        }

        if (!device)
        {
            lines.Insert("Kein Gefaess. Die Entity-ID zeigt auf nichts, was noch existiert.");
            outLines = End(lines);
            return;
        }

        ChefZ_CompiledRecipe rec = FindRecipeByName(recipeId);
        if (!rec)
        {
            NoSuchRecipe(recipeId, lines);
            outLines = End(lines);
            return;
        }

        ChefZ_DeviceDescriptor  desc;
        ChefZ_CookContext       ctx;
        ChefZ_FactSnapshot      snapshot;
        array<ItemBase>         entities;
        string                  why;

        if (!BuildView(device, desc, ctx, snapshot, entities, why))
        {
            lines.Insert(why);
            outLines = End(lines);
            return;
        }

        lines.Insert("Geraet        " + desc.ToDebugString());
        lines.Insert("Methode       " + ChefZ_SymbolTable.NameOrMark(ctx.method) + "   Temperatur " + ctx.deviceTemperature.ToString());
        lines.Insert("Inhalt:");
        for (int i = 0; i < snapshot.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.Get(i);
            if (facts)
                lines.Insert("    " + facts.ToLine());
        }
        if (snapshot.Count() == 0)
            lines.Insert("    (leer)");

        // ---- 1) der echte Bindungslauf --------------------------------------
        lines.Insert("");
        lines.Insert("Bindungslauf (mit Backtracking, in der Pruefreihenfolge aus 07 E6):");

        ChefZ_MatchTrace  trace  = new ChefZ_MatchTrace();
        ChefZ_MatchResult result = new ChefZ_MatchResult();

        bool bound = ChefZ_RecipeEvaluator.Evaluate(rec, ctx, snapshot, ChefZ_RecipeEngine.Get().GetNodeBudget(), trace, result);

        array<string> traceLines = new array<string>();
        trace.ToLines(traceLines);
        for (int t = 0; t < traceLines.Count(); t++)
            lines.Insert("  " + traceLines.Get(t));

        lines.Insert("");
        if (bound)
        {
            // Der Fall, den ein Autor am wenigsten erwartet und am haeufigsten
            // hat: das Rezept bindet sehr wohl - es verliert nur gegen ein
            // anderes, oder das Gefaess ist noch nicht fertig.
            lines.Insert("Dieses Rezept BINDET hier. Es ist nicht der Slotgrund.");
            string readyReason;
            bool ready = ChefZ_RecipeEvaluator.CheckReady(rec, result, snapshot, ctx, readyReason);
            if (ready)
            {
                lines.Insert("Es ist ausserdem abschlussbereit. Bleibt es trotzdem aus, " + "gewinnt ein hoeher rangierendes Rezept - siehe \"chefz match\" " + "und \"chefz ambiguities\".");
            }
            else
            {
                lines.Insert("Abschluss steht noch aus: " + readyReason);
            }
        }
        else
        {
            lines.Insert("ERSTER VERLETZTER PUNKT: " + result.failReason + SlotSuffix(result.failSlotId));
        }

        // ---- 2) die slotweise Betrachtung -----------------------------------
        lines.Insert("");
        lines.Insert("Slots einzeln betrachtet (ohne Zuordnung):");

        ChefZ_PartialMatchReport partial;
        ChefZ_RecipeEngine.Get().EvaluatePartial(ctx, snapshot, rec.recipeSym, partial);
        if (partial)
        {
            array<string> partialLines = new array<string>();
            partial.ToLines(partialLines);
            for (int p = 0; p < partialLines.Count(); p++)
                lines.Insert("  " + partialLines.Get(p));
        }

        lines.Insert("");
        lines.Insert("Rein lesend. Es wurde nichts erzeugt und nichts verbraucht (18 §6).");

        outLines = End(lines);
    }

    //==========================================================================
    // Entity-Aufloesung fuer die Kommandos
    //==========================================================================

    /**
     * Netz-ID -> Gefaess.
     *
     * Akzeptiert "low", "low:high" und "low high" (der Aufrufer fuegt das
     * zweite Wort mit einem Doppelpunkt an). Die Netz-ID ist die einzige
     * Kennung, die Client und Server teilen (Object.c:814) - also die einzige,
     * die ein Admin ueberhaupt ablesen kann.
     *
     * @return false, wenn nichts aufloesbar ist. reason sagt, was fehlt.
     */
    static bool FindVessel(string token, out ItemBase vessel, out string reason)
    {
        vessel = null;
        reason = "";

        if (!g_Game)
        {
            reason = "Kein Spiel.";
            return false;
        }

        string text = token;
        text.TrimInPlace();
        if (text == "")
        {
            reason = "Keine Entity-ID angegeben.";
            return false;
        }

        string lowText  = text;
        string highText = "0";

        int sep = text.IndexOf(":");
        if (sep >= 0)
        {
            lowText  = text.Substring(0, sep);
            highText = text.Substring(sep + 1, text.Length() - sep - 1);
        }

        int low  = lowText.ToInt();
        int high = highText.ToInt();

        if (low == 0 && high == 0)
        {
            reason = "\"" + text + "\" ist keine Netz-ID. Erwartet wird " + "<low> oder <low>:<high>.";
            return false;
        }

        Object obj = g_Game.GetObjectByNetworkId(low, high);
        if (!obj)
        {
            reason = "Zur Netz-ID " + low.ToString() + ":" + high.ToString() + " gibt es kein Objekt. Es kann geloescht oder ausgeladen sein.";
            return false;
        }

        ItemBase item = ItemBase.Cast(obj);
        if (!item)
        {
            string chefzTxt3 = "Objekt " + low.ToString() + ":" + high.ToString() + " ist kein ItemBase (";
            chefzTxt3 = chefzTxt3 + obj.GetType() + "). " + "ChefZ wertet nur Gefaesse aus.";
            reason = chefzTxt3;
            return false;
        }

        vessel = item;
        return true;
    }

    //==========================================================================
    // Ausgabe
    //==========================================================================

    /**
     * Eine fertige Antwort ins RPT schreiben - blockweise (18 E4).
     *
     * Ueber ChefZ_Log.Force und nicht ChefZ_Log.Block: die Antwort auf eine
     * ausdrueckliche Frage darf nicht an der eingestellten Logstufe haengen.
     * Die Begruendung steht im Kopf von ChefZ_Log.Force.
     */
    static void Emit(array<string> lines)
    {
        ChefZ_Log.ForceBlock(ChefZ_LogChannel.CORE, lines);
        ChefZ_Log.Flush();
    }

    //==========================================================================
    // Interna
    //==========================================================================

    //! Kopfzeile und Antwortliste anlegen. Der Aufrufer arbeitet danach auf
    //! einer LOKALEN Liste; einen out-Parameter als out-Parameter
    //! weiterzureichen ist in Enforce nirgends zugesichert (siehe Kopf von
    //! ChefZ_TextList.SymbolsOf).
    private static array<string> Begin(array<string> outLines, string title)
    {
        array<string> lines = outLines;
        if (!lines)
            lines = new array<string>();
        lines.Insert(RULE + " " + title + " " + RULE);
        return lines;
    }

    private static array<string> End(array<string> lines)
    {
        if (lines.Count() > MAX_ANSWER_LINES)
        {
            while (lines.Count() > MAX_ANSWER_LINES)
                lines.Remove(lines.Count() - 1);
            lines.Insert("... Antwort bei " + MAX_ANSWER_LINES.ToString() + " Zeilen abgeschnitten. Ein engeres Kommando fragen.");
        }
        lines.Insert(RULE);
        return lines;
    }

    //! true, wenn weitergearbeitet werden darf. Sonst steht die Begruendung
    //! bereits in der Liste.
    private static bool Guard(notnull array<string> lines)
    {
        string reason;
        if (IsAvailable(reason))
            return true;
        lines.Insert(reason);
        return false;
    }

    /**
     * Rezept ueber seinen Namen finden - OHNE zu internieren.
     *
     * Lookup und nicht Intern: ein Tippfehler in einem Adminkommando darf die
     * Symboltabelle nicht dauerhaft um einen toten Eintrag verlaengern. Sie
     * waechst nur, sie schrumpft nie (03 §4).
     */
    private static ChefZ_CompiledRecipe FindRecipeByName(string recipeId)
    {
        string name = recipeId;
        name.TrimInPlace();
        if (name == "")
            return null;

        ChefZ_Sym sym = ChefZ_SymbolTable.Lookup(name);
        if (!ChefZ_SymbolTable.IsValid(sym))
            return null;

        return ChefZ_RecipeEngine.Get().FindRecipe(sym);
    }

    private static void NoSuchRecipe(string recipeId, notnull array<string> lines)
    {
        lines.Insert("Kein geladenes Rezept mit der ID \"" + recipeId + "\".");
        lines.Insert("Der Bestand umfasst " + ChefZ_RecipeEngine.Get().GetRecipeCount().ToString() + " Rezepte. \"chefz registries\" listet sie; \"chefz report\" nennt die, " + "die beim Laden abgewiesen wurden.");
    }

    /**
     * Kontext und Fakten fuer ein Gefaess aufbauen - in EIGENEN Puffern.
     *
     * Siehe Dateikopf, Punkt 1: die gleichnamigen Felder des Kochadapters
     * bleiben unberuehrt. Das kostet drei Allokationen je Kommando und
     * schuetzt einen laufenden Kochtick davor, seine Datengrundlage unter den
     * Fuessen wegzuverlieren.
     */
    private static bool BuildView(notnull ItemBase device, out ChefZ_DeviceDescriptor desc, out ChefZ_CookContext ctx, out ChefZ_FactSnapshot snapshot, out array<ItemBase> entities, out string reason)
    {
        reason = "";

        ChefZ_DeviceDescriptor found;
        if (!ChefZ_CookingDeviceAdapter.Get().ResolveDevice(device, found))
        {
            reason = "Die Klasse dieses Objekts ist nicht ermittelbar.";
            return false;
        }

        if (!found.enabled)
        {
            reason = "\"" + device.GetType() + "\" ist fuer ChefZ kein Kochgeraet. " + "Das ist der Normalfall und kein Fehler (10 E7): was nicht in " + "CfgChefZDevices steht - weder selbst noch ueber eine " + "Vorfahrenklasse -, verhaelt sich exakt wie ohne ChefZ.";
            return false;
        }

        ChefZ_CookContext  localCtx      = new ChefZ_CookContext();
        ChefZ_FactSnapshot localSnapshot = new ChefZ_FactSnapshot();
        array<ItemBase>    localEntities = new array<ItemBase>();

        int method = ChefZ_CookingHook.QueryMethodForDiagnostics(device);

        if (!ChefZ_FactCollector.CollectContext(device, found, method, localCtx))
        {
            reason = "Der Kontext dieses Gefaesses ist nicht lesbar.";
            return false;
        }

        // Dieselbe Zuschreibung, die auch der Kochtick benutzen wuerde -
        // sonst antwortete "chefz match" auf eine andere Frage als die, die
        // der Server sich stellt. PeekSession legt keine Sitzung an: eine
        // Auskunft darf den Zustand nicht veraendern, ueber den sie Auskunft
        // gibt. Ohne laufende Sitzung bleibt es bei 0 - dann steht im Bericht
        // "niemand", und das ist die ehrliche Antwort.
        ChefZ_CookSession session = ChefZ_CookingDeviceAdapter.Get().PeekSession(device);
        if (session)
            localCtx.actorIdentityId = session.actorIdentityId;

        ChefZ_FactCollector.CollectFromCargo(device, localSnapshot, localEntities);

        desc     = found;
        ctx      = localCtx;
        snapshot = localSnapshot;
        entities = localEntities;
        return true;
    }

    /**
     * Welche der vier Torstufen aus 10 §5 haelt hier?
     *
     * Sie einzeln zu benennen ist der Unterschied zwischen "es passiert
     * nichts" und "der Rezeptindex kennt fuer diese Geraeteklasse nichts".
     */
    private static string GateSummary(notnull ItemBase device, notnull ChefZ_DeviceDescriptor desc, int itemCount)
    {
        if (!ChefZ_CookingHook.IsEnabled())
        {
            return "Stufe 0 HAELT: der Core ist inert (enabled=false, SAFE_MODE oder " + "Config nie geladen). Kochen ist bitgenau Vanilla.";
        }

        if (device.IsRuined())
            return "Stufe 0 HAELT: das Gefaess ist ruiniert. Vanilla bricht hier ebenfalls ab.";

        ChefZ_RecipeEngine engine = ChefZ_RecipeEngine.Get();
        if (!engine.HasAnyRecipeFor(desc.deviceClass, desc.deviceRootClass))
        {
            return "Stufe 0 HAELT: der Rezeptindex kennt kein Rezept fuer diese " + "Geraeteklasse. Ein Bool-Test, sonst nichts (19 S7).";
        }

        if (itemCount <= 0)
            return "Stufe 0 HAELT: kein Cargo.";

        int minItems = engine.GetMinItemCountFor(desc.deviceClass);
        if (itemCount < minItems)
        {
            return "Stufe 0 HAELT: " + itemCount.ToString() + " Items, das guenstigste " + "Rezept braucht " + minItems.ToString() + ".";
        }

        return "Stufe 0 passiert (Mindestitems " + minItems.ToString() + ").";
    }

    private static void AppendPlan(notnull ChefZ_MatchResult result, notnull ChefZ_FactSnapshot snapshot, notnull array<string> lines)
    {
        if (result.consumePlan.Count() == 0)
            return;

        lines.Insert("  Verbrauchsplan (HYPOTHETISCH - nichts davon wird ausgefuehrt):");
        for (int i = 0; i < result.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = result.consumePlan.Get(i);
            if (!plan)
                continue;

            ChefZ_ItemFacts facts = snapshot.FindByHandle(plan.handle);
            string what = "handle " + plan.handle.ToString();
            if (facts)
                what = ChefZ_SymbolTable.NameOrMark(facts.classSym);

            lines.Insert("    " + what + "  " + plan.ToDebugString());
        }
    }

    private static void AppendAlternatives(notnull ChefZ_RecipeEngine engine, notnull ChefZ_CookContext ctx, notnull ChefZ_FactSnapshot snapshot, notnull array<string> lines)
    {
        array<ref ChefZ_MatchResult> all = new array<ref ChefZ_MatchResult>();
        int count = engine.EvaluateAll(ctx, snapshot, all, MAX_ALTERNATIVES);

        lines.Insert("");
        if (count <= 0)
        {
            lines.Insert("Bindbare Rezepte an diesem Gefaess: keines.");
            return;
        }

        lines.Insert("Bindbare Rezepte an diesem Gefaess, bestes zuerst (" + count.ToString() + "):");
        for (int i = 0; i < all.Count(); i++)
        {
            ChefZ_MatchResult one = all.Get(i);
            if (!one)
                continue;
            string chefzTxt4 = "  " + (i + 1).ToString() + ". " + one.recipeId + "  score=";
            chefzTxt4 = chefzTxt4 + one.score.ToString() + "  fertig=" + one.ready.ToString();
            lines.Insert(chefzTxt4);
        }
    }

    private static string SlotSuffix(string slotId)
    {
        if (slotId == "")
            return "";
        return "   (Slot \"" + slotId + "\")";
    }

    //! Ein Gefaess so bezeichnen, dass ein Admin es wiedererkennt.
    private static string Describe(ItemBase device)
    {
        if (!device)
            return "(nichts)";

        int low  = 0;
        int high = 0;
        device.GetNetworkID(low, high);

        return device.GetType() + " " + low.ToString() + ":" + high.ToString();
    }
}
