//==============================================================================
// ChefZ_Boot - der Startzeitpunkt des Config Managers
//
// Entwurf: 02 §6 ("[5_Mission] ChefZ_Boot.OnMissionStart() / MissionGameplay.
// OnInit() -> [3_Game] ChefZ_ConfigManager.LoadAll(isServer)"), 00 §4
// (5_Mission = Startzeitpunkt und Missionslebensdauer), 19 S2.
//
// Bewusst OHNE eigenen "modded class MissionServer/MissionGameplay":
// ChefZ_CoreEntry hat seit S1 genau einen Einstiegspunkt je Seite, und der ist
// derselbe, den Vanilla fuer CfgGameplayHandler.LoadData() benutzt. Ein
// zweites Paar Overrides waere zulaessig, aber zusaetzliche Kollisionsflaeche
// gegenueber anderen Mods - fuer null Gewinn.
//
// Warum der Zeitpunkt stimmt: MissionServer.OnInit laeuft, nachdem die Engine
// alle Configs gemerged hat (CfgChefZ ist also vollstaendig) und bevor der
// erste Spieler etwas kochen kann.
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_Boot
{
    private static bool s_ServerDone;
    private static bool s_ClientDone;

    /**
     * Serverseitiger Start: Rang 1 + 2 + 3.
     *
     * Fehler werden hier NICHT weitergereicht. Der Manager kehrt in jedem Fall
     * zurueck; was er nicht laden konnte, steht im Ladebericht. Invariante I2:
     * der Ausfallpfad ist der Vanilla-Pfad, und der braucht von hier aus nichts.
     */
    static void OnMissionStart()
    {
        if (s_ServerDone)
        {
            // Zweiter Missionsstart im selben Prozess. Die Daten stehen
            // bereits und werden bewusst NICHT neu geladen (02 E5) - aber
            // Vanilla hat mit der neuen Mission ein NEUES
            // PluginRecipesManager gebaut, und dessen frisch verankerte
            // Rezeptplaetze sind noch leer. Ohne diese Zeile bliebe das
            // Handwerk ab dem zweiten Start stumm.
            ChefZ_HandcraftBridge.FillReserved();
            return;
        }
        s_ServerDone = true;

        RunSelfTest();

        ChefZ_ConfigManager cfg = ChefZ_ConfigManager.Get();
        cfg.LoadAll(true);

        // S15 (11 §5): die bereits VERANKERTEN Rezeptplaetze mit den
        // HANDCRAFT-Transforms parametrieren.
        //
        // Eingetragen wurden sie schon im MissionBase-KONSTRUKTOR, aus
        // RegisterRecipies() heraus (ChefZ_HandcraftBridge.Reserve). Hier
        // wird kein Rezept mehr registriert und keine Position mehr
        // veraendert - nur beschrieben, was schon steht. Warum das so
        // getrennt ist, steht im Kopf von ChefZ_HandcraftBridge, Abschnitt
        // POSITIONSANKER.
        //
        // Ausschliesslich ADDITIV. Faellt der Aufruf aus, bleiben die
        // Plaetze leer und damit folgenlos - Vanilla-Crafting bleibt
        // vollstaendig.
        ChefZ_HandcraftBridge.FillReserved();

        ReportState(cfg, "SERVER");

        // S13 (17 §4, §7): "ChefZ_OnConfigLoaded - nach Freeze()".
        //
        // Hier und nicht im Config Manager, weil 17 §7 den Ablauf ausdruecklich
        // so zeichnet: ChefZ_Boot laedt, baut die Manager und loest DANACH aus.
        // Damit steht das Ereignis hinter allem, was ein Abonnent abfragen
        // koennte - Kategorien, Zustaende, Stufen, Rezepte.
        //
        // Die Reihenfolge der Mods spielt keine Rolle: ein Comp-Modul, das
        // sich erst danach anmeldet, verpasst nur dieses eine vergangene
        // Ereignis und bekommt jedes kuenftige (17 §7).
        RaiseConfigLoaded(cfg);
    }

    /**
     * Clientseitiger Start: Rang 1 + 2 aus denselben PBOs (02 §6).
     *
     * Der Client trifft keine Spielentscheidung. Er laedt, damit Anzeigenamen
     * und ActionCondition etwas zu lesen haben - mehr nicht. $profile: wird
     * clientseitig weder angelegt noch gelesen (02 E2).
     */
    static void OnClientStart()
    {
        if (s_ClientDone)
        {
            // Zweiter Missionsstart im selben Prozess - beim Client der
            // Normalfall, sobald jemand einen zweiten Server betritt.
            // Begruendung siehe OnMissionStart.
            ChefZ_HandcraftBridge.FillReserved();
            return;
        }
        s_ClientDone = true;

        // Auf einem Listen-Server laufen BEIDE Einstiegspunkte im selben
        // Prozess. Dort hat die Serverseite bereits alles geladen, inklusive
        // Rang 3 - ein zweiter Aufruf waere ein Neuladen zur Laufzeit, und das
        // gibt es in V1 bewusst nicht (02 E5).
        if (s_ServerDone)
            return;

        ChefZ_ConfigManager cfg = ChefZ_ConfigManager.Get();
        cfg.LoadAll(false);

        // S15: auch der Client parametriert seine Rezeptplaetze. Er muss es,
        // damit die Craftaktion ueberhaupt erscheint - ActionWorldCraft fragt
        // PluginRecipesManager.GetValidRecipes() clientseitig.
        //
        // Er ENTSCHEIDET dabei nichts (00 §5): was tatsaechlich geschieht,
        // prueft der Server in CheckRecipe -> CanDo erneut und bindet neu -
        // und seit dem Positionsanker prueft er zusaetzlich, dass beide
        // Seiten ueberhaupt dasselbe Rezept meinen (ChefZ_CraftIntent).
        ChefZ_HandcraftBridge.FillReserved();

        ReportState(cfg, "CLIENT");
    }

    //--------------------------------------------------------------------------

    /**
     * Der Selbsttest laeuft VOR dem Laden.
     *
     * Grund: schlaegt er fehl, ist jede spaetere Aussage ueber geladene Daten
     * unzuverlaessig - und dann will man das im RPT ueber dem Ladebericht
     * stehen haben, nicht darunter.
     *
     * Nur serverseitig: er kostet zwar wenig, aber der Client hat nichts
     * davon, und 18 §4 haelt das Log ohnehin serverseitig.
     */
    private static void RunSelfTest()
    {
        bool ok = ChefZ_ConfigSelfTest.Run();
        if (ok)
            ChefZ_Log.Banner(ChefZ_ConfigSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, ChefZ_ConfigSelfTest.Summary());

        // S3: Kategoriebaum, Vorfahrenbitsets, Zyklenerkennung. Laeuft auf
        // eigenen Manager-Instanzen und beruehrt den Singleton nicht - der
        // wird erst in LoadAll() gebaut.
        bool okCategories = ChefZ_CategorySelfTest.Run();
        if (okCategories)
            ChefZ_Log.Banner(ChefZ_CategorySelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, ChefZ_CategorySelfTest.Summary());

        // S4: Zutatenaufloesung, Vererbung, Rueckwaertsindizes, Faktenliste.
        // Ebenfalls auf eigenen Instanzen - der Singleton wird erst in
        // LoadAll() gebaut.
        bool okIngredients = ChefZ_IngredientSelfTest.Run();
        if (okIngredients)
            ChefZ_Log.Banner(ChefZ_IngredientSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.CONFIG, ChefZ_IngredientSelfTest.Summary());

        // S5: Selektorcompiler, Slotauswertung, Matcher mit Backtracking.
        // Er laeuft vollstaendig in 1_Core auf handgebauten Faktenlisten -
        // ohne Item, ohne Feuerstelle, ohne Rezept (19 S5: "Der Matcher ist an
        // dieser Stelle ohne laufendes Spiel pruefbar"). Damit ist die Frage
        // "rechnet der Matcher richtig" beantwortet, bevor der erste Topf
        // ausgewertet wird.
        bool okMatcher = ChefZ_MatcherSelfTest.Run();
        if (okMatcher)
            ChefZ_Log.Banner(ChefZ_MatcherSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.MATCH, ChefZ_MatcherSelfTest.Summary());

        // S6: Rezeptmodell, Spezifitaetsrechnung, invertierter Index,
        // Kandidatenreihenfolge, Ambiguitaetsmeldung. Er baut eigene
        // Engine-Instanzen auf handgebauten Registries - der Singleton wird
        // erst in LoadAll() gebaut und bleibt hier unberuehrt.
        //
        // Der Test rechnet unter anderem die Tabelle aus 09 §4.5 nach. Wenn
        // diese Zahlen stimmen, stimmt die Aussage "das spezifischste gueltige
        // Rezept gewinnt" - und wenn nicht, ist jede Kesselentscheidung des
        // Servers eine andere als die entworfene.
        bool okRecipes = ChefZ_RecipeSelfTest.Run();
        if (okRecipes)
            ChefZ_Log.Banner(ChefZ_RecipeSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.MATCH, ChefZ_RecipeSelfTest.Summary());

        // S7: Signaturarithmetik, Sitzungsautomat, Methodentabelle des
        // Kochadapters. Alles drei ohne Item und ohne Feuerstelle pruefbar -
        // und alles drei so, dass ein Fehler auf dem Server NICHT auffiele:
        // eine Signatur, die eine Aenderung verschluckt, sieht aus wie ein
        // Gefaess, in dem nichts passiert.
        //
        // Der Hook selbst wird hier nicht geprueft und kann es nicht werden.
        // Seine Zusage ist keine Rechnung, sondern eine Struktur: super ist
        // die erste Anweisung und sein Rueckgabewert der Rueckgabewert
        // (10 §3). Das prueft der Conflict-Scout am Diff, nicht ein Testlauf.
        bool okCooking = ChefZ_CookingSelfTest.Run();
        if (okCooking)
            ChefZ_Log.Banner(ChefZ_CookingSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.COOK, ChefZ_CookingSelfTest.Summary());

        // S8: die Transaktion. Zwei der drei Negativtests aus 19 S8 sind ohne
        // Welt entscheidbar, weil beide VOR jeder Beruehrung eines Items
        // fallen - die entfernte Zutat in ValidateHandles, die fehlende
        // Ergebnisklasse in der Planung. Der dritte (voller Cargo) fragt ein
        // echtes Inventar und bleibt dem Servertest vorbehalten.
        //
        // Diese Zeile steht hier und nicht im Kochselbsttest, weil sie eine
        // andere Frage beantwortet: der Adapter ENTSCHEIDET, der Applicator
        // VERBRAUCHT. Nur bei zweiterem kostet ein Fehler den Spieler etwas.
        bool okApply = ChefZ_ApplicatorSelfTest.Run();
        if (okApply)
            ChefZ_Log.Banner(ChefZ_ApplicatorSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.COOK, ChefZ_ApplicatorSelfTest.Summary());

        // S9: Zustandsregistry, Projektion, Rueckabbildung, Identitaeten.
        // Alles davon ist reine Rechnung und ohne Item pruefbar - und alles
        // davon scheitert LEISE, wenn es scheitert: ein Zustand, der still auf
        // gar nichts projiziert, sieht aus wie einer, den niemand benutzt hat.
        //
        // Die drei uebrigen Abnahmebedingungen aus 19 S9 (Neustart, Spielstand
        // ohne ChefZ-Block, Klassentausch) brauchen einen Server mit Welt und
        // bleiben dem Servertest vorbehalten. Sie werden hier ausdruecklich
        // NICHT nachgestellt - ein nachgebauter ParamsWriteContext prueft den
        // Nachbau, nicht die Engine.
        bool okState = ChefZ_StateSelfTest.Run();
        if (okState)
            ChefZ_Log.Banner(ChefZ_StateSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.STATE, ChefZ_StateSelfTest.Summary());

        // S10: Stufenleitern, Punktrechnung, Qualitaetsregeln. Auch das ist
        // reine Rechnung und ohne Item pruefbar - und auch das scheitert
        // LEISE: eine Frische, die als Mittelwert statt als Minimum eingeht
        // (12 §4.1), liefert weiterhin Zahlen, weiterhin Stufen und weiterhin
        // Gerichte. Auffallen wuerde sie erst dem Spieler, der altes Fleisch
        // in einen Premium-Eintopf waescht.
        //
        // Der Test rechnet die Formel aus 12 §4 an einem Beispiel nach, in dem
        // jeder Summand einen anderen Wert hat. Stimmt die Summe, stimmt jeder
        // Summand.
        bool okQuality = ChefZ_QualitySelfTest.Run();
        if (okQuality)
            ChefZ_Log.Banner(ChefZ_QualitySelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.QUALITY, ChefZ_QualitySelfTest.Summary());

        // S11: Haltbarkeitsrechnung und Restfrische. Auch das ist reine
        // Rechnung und ohne Item pruefbar - und auch das scheitert LEISE,
        // sogar leiser als alles davor: ein Verderbfaktor, der um zehn Prozent
        // danebenliegt, ist im Spiel von Zufall nicht zu unterscheiden. Vanilla
        // wuerfelt den Verfallstimer ohnehin mit 25 Prozent Streuung aus
        // (01 V9), und niemand misst nach.
        //
        // Die letzte Gruppe des Tests prueft die Kernzusage des Teilsystems
        // auf der Rechenseite: ohne eine einzige Regel ist der Faktor exakt
        // 1.0, und damit ist der Verfall bitgenau Vanilla. Faellt sie um, ist
        // die Haltbarkeit JEDER ChefZ-Nahrung auf dem Server verschoben, ohne
        // dass irgendwo eine Zeile im Log steht.
        bool okPreserv = ChefZ_PreservationSelfTest.Run();
        if (okPreserv)
            ChefZ_Log.Banner(ChefZ_PreservationSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.PRESERV, ChefZ_PreservationSelfTest.Summary());

        // S12: Naehrwertvektor, Auffindungsreihenfolge, Sollrechnung. Auch das
        // ist reine Rechnung und ohne Item pruefbar - und es ist das leiseste
        // Teilsystem des ganzen Core: der Nutrition Manager veraendert zur
        // Laufzeit NICHTS (13 E1/E2). Ein Fehler hier kostet keinen Spieler
        // ein Gericht; er kostet den Balance-Reviewer seine Datengrundlage,
        // und zwar ohne dass irgendetwas danach aussieht.
        //
        // Der Test rechnet unter anderem das Beispiel aus 13 §5 nach
        // (450 + 500 + 100, mal 1.10, gleich 1155). Stimmt diese Zahl, stimmt
        // jeder Summand und der Modifikator dazu.
        //
        // Die drei Pruefungen, die CfgVehicles brauchen - Vanilla-Rueckfall,
        // die V7-Bedingung und der Vergleich Soll gegen Ist - laufen HIER
        // nicht. Sie laufen wenige Zeilen spaeter am echten Bestand, im
        // Startaudit, und melden ihr Ergebnis im Klartext.
        bool okNutrition = ChefZ_NutritionSelfTest.Run();
        if (okNutrition)
            ChefZ_Log.Banner(ChefZ_NutritionSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.NUTRI, ChefZ_NutritionSelfTest.Summary());

        // S13: Ereignisschicht, Faehigkeiten, Fortschritt. Der einzige
        // Selbsttest des Core, der eine ZUSAGE GEGENUEBER FREMDEM CODE prueft
        // und nicht eine eigene Rechnung - und deshalb der einzige, dessen
        // Fehlschlag einen anderen Mod trifft.
        //
        // Die wichtigste Gruppe ist "Ausnahme": ein Abonnent, der einen Fehler
        // wirft, darf das Kochen nicht anhalten (17 §9). Faellt diese Zusage
        // um, bemerkt man es nicht am Log, sondern daran, dass Spieler mit
        // einem bestimmten Mod keine Gerichte mehr bekommen - und die Ursache
        // steht dann in einem fremden PBO.
        //
        // Der Test laeuft auf einer EIGENEN Businstanz und auf dem Singleton
        // der Faehigkeitsregistry (der zu diesem Zeitpunkt leer ist und dessen
        // Anbieterliste danach wieder leer ist).
        // S14: Verarbeitungsmodell, Werkzeugaufloesung, Transformcompiler,
        // Index und Fortschrittsarithmetik. Auch das ist reine Rechnung und
        // ohne Station pruefbar - und auch das scheitert LEISE: ein
        // Transformindex, der den falschen Kandidaten zuerst liefert, erzeugt
        // weiterhin ein Ergebnis, nur das falsche. Ein Fortschritt, der bei
        // fehlender Waerme zurueckliefe statt zu pausieren, faellt erst nach
        // vierzig Minuten Raeuchern auf, und dann sieht es aus wie ein
        // Serverruckler.
        //
        // Die drei uebrigen Abnahmebedingungen aus 19 S14 (Neustart mit
        // laufendem Job, pausierender Job an einer kalten Station in der Welt,
        // entferntes Eingangsitem) brauchen einen Server mit Welt und bleiben
        // dem Servertest vorbehalten. Geprueft wird hier, WORAUF sie beruhen -
        // siehe Kopf von ChefZ_ProcessingSelfTest.
        bool okProcessing = ChefZ_ProcessingSelfTest.Run();
        if (okProcessing)
            ChefZ_Log.Banner(ChefZ_ProcessingSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.PROCESS, ChefZ_ProcessingSelfTest.Summary());

        // S15: die Handcraft-Bruecke. Sie hat eine Fehlerklasse, die auf
        // einem laufenden Server NICHT auffaellt und in keiner Logzeile
        // steht - Vanillas RecipeBase legt alle seine Felder mit 0 vor, und 0
        // bedeutet in fast jedem dieser Felder etwas ANDERES als "nichts
        // tun". Ein vergessenes m_ResultSetHealth[i] = -1 liefert ein
        // RUINIERTES Gericht, ein vergessenes m_ResultSetQuantity[i] = -1 ein
        // leeres. Beide Rezepte erscheinen in der Craftliste und tun etwas,
        // nur eben das Falsche.
        //
        // Die eigentliche Abnahmebedingung aus 19 S15 - "ein
        // HANDCRAFT-Testtransform erscheint als normales Craftrezept im
        // Spiel" - braucht Vanillas Rezeptcache und damit einen Server mit
        // Welt. Sie bleibt dem Servertest vorbehalten. Geprueft wird hier,
        // WORAUF sie beruht.
        bool okHandcraft = ChefZ_HandcraftSelfTest.Run();
        if (okHandcraft)
            ChefZ_Log.Banner(ChefZ_HandcraftSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.PROCESS, ChefZ_HandcraftSelfTest.Summary());

        // S16: die doppelte Deckelung, die Registry und der Entnahmeplan. Auch
        // das ist reine Rechnung und ohne Item pruefbar - und es ist der
        // Selbsttest mit der unangenehmsten Fehlerklasse des ganzen Core:
        // faellt der Mengendeckel aus (15 §5.2), liefert ein Kessel mit einer
        // einzigen Zutat weiterhin Portionen, nur eben zwoelf. Nichts daran
        // sieht kaputt aus, nichts steht im Log, und die Nahrungsbilanz des
        // Servers ist ab dem ersten Kessel hinueber.
        //
        // Die Abnahmebedingungen aus 19 S16, die eine Welt brauchen - zwei
        // gleichzeitige Entnahmen, ein voller Rucksack - laufen HIER nicht.
        // Geprueft wird, WORAUF sie beruhen: dass der Plan vor jeder Wirkung
        // steht und dass ein Zaehler von 0 keinen Plan mehr ergibt.
        bool okPortion = ChefZ_PortionSelfTest.Run();
        if (okPortion)
            ChefZ_Log.Banner(ChefZ_PortionSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.PORTION, ChefZ_PortionSelfTest.Summary());

        // S17: Behaelterindizes, AUTO-Aufloesung, Suchbitfeld, Auswahlregel.
        // Auch das ist reine Rechnung und ohne Item pruefbar - und es enthaelt
        // die einzige Fehlerklasse dieses Bauabschnitts, die dem Spieler
        // NUTZT statt zu schaden und deshalb nie gemeldet wird: eine
        // ReturnsEmpty-Kopplung, die consumedOnServe vergisst, verdoppelt
        // Behaelter. Der Spieler behaelt seinen Teller UND bekommt einen
        // zweiten; nichts daran sieht kaputt aus, und niemand schreibt ein
        // Ticket.
        //
        // Die Abnahmebedingungen aus 19 S17, die eine Welt brauchen - der
        // Teller im Inventar, das Loeschen beim Servieren, die Rueckgabe in
        // die freie Hand, der Bissen, der noch keinen Teller ergibt - laufen
        // HIER nicht. Geprueft wird, WORAUF sie beruhen.
        bool okContainer = ChefZ_ContainerSelfTest.Run();
        if (okContainer)
            ChefZ_Log.Banner(ChefZ_ContainerSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.CONTAIN, ChefZ_ContainerSelfTest.Summary());

        bool okEvents = ChefZ_EventSelfTest.Run();
        if (okEvents)
            ChefZ_Log.Banner(ChefZ_EventSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.EVENT, ChefZ_EventSelfTest.Summary());

        // S18: Kommandozerlegung, Laufzeitschalter, Entity-Aufloesung und die
        // Kernzusage des Teilsystems - Nebenwirkungsfreiheit.
        //
        // Er laeuft ZULETZT und nicht zuerst, obwohl er der billigste ist:
        // seine interessantesten Gruppen schalten Logstufe und Kanalmaske um
        // und stellen sie danach wieder her. Liefe er vor den anderen, liefen
        // die anderen bei umgestellter Stufe - und ihre Meldungen fehlten
        // genau dann, wenn man sie braucht.
        //
        // Die Fehlerklasse dieses Teilsystems ist einzigartig im Core: sie
        // trifft nie den Spieler, sondern IMMER den, der einen Fehler sucht.
        // Eine Diagnose, die falsch antwortet, schickt einen Content-Autor in
        // die falsche Datei - und das kostet mehr Zeit als gar keine Antwort.
        // Deshalb steht sie nicht nur da, sie wird geprueft.
        //
        // Die beiden Abnahmebedingungen aus 19 S18 selbst - "chefz match
        // liefert den Block und veraendert nachweislich nichts", "chefz why
        // nennt den ersten verletzten Slot" - brauchen ein Gefaess in einer
        // Welt und bleiben dem Servertest vorbehalten.
        bool okDiagnostics = ChefZ_DiagnosticsSelfTest.Run();
        if (okDiagnostics)
            ChefZ_Log.Banner(ChefZ_DiagnosticsSelfTest.Summary());
        else
            ChefZ_Log.Error(ChefZ_LogChannel.CORE, ChefZ_DiagnosticsSelfTest.Summary());
    }

    /**
     * ChefZ_OnConfigLoaded (17 §4).
     *
     * Musterhaft fuer JEDE Ausloesestelle im Core, deshalb hier ausgeschrieben:
     * zuerst HasSubscribers(), dann erst die Nutzlast. Ohne Comp-Module kostet
     * die Zeile einen Map-Zugriff und sonst nichts (17 E2).
     *
     * Nicht stornierbar und nicht XP-tauglich - es meldet einen Zustand, keine
     * Leistung.
     */
    private static void RaiseConfigLoaded(ChefZ_ConfigManager cfg)
    {
        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (!bus.HasSubscribers(ChefZ_EventNames.CONFIG_LOADED))
            return;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.CONFIG_LOADED);
        args.amount = cfg.TotalRecordCount();
        bus.Raise(args);
    }

    private static void ReportState(ChefZ_ConfigManager cfg, string side)
    {
        // Eine Zeile je Seite, an der Stufenpruefung vorbei: ein Betreiber muss
        // ohne Debugstufe sehen koennen, in welchem Zustand der Core ist
        // (18 §4).
        string chefzTxt1 = "Config " + side + "  health=" + ChefZ_ConfigManager.HealthName(cfg.GetHealth()) + "  records=";
        chefzTxt1 = chefzTxt1 + cfg.TotalRecordCount().ToString() + "  kategorien=" + ChefZ_CategoryManager.Get().GetCategoryCount().ToString() + "  zutaten=" + ChefZ_IngredientManager.Get().GetKnownCount().ToString();
        chefzTxt1 = chefzTxt1 + "  zustaende=" + ChefZ_StateManager.Get().GetStateCount().ToString() + "  stufen=" + ChefZ_QualityManager.Get().GetTierCount().ToString() + "  rezepte=";
        chefzTxt1 = chefzTxt1 + ChefZ_RecipeEngine.Get().GetRecipeCount().ToString() + "  haltbarkeit=" + ChefZ_PreservationManager.Get().GetRuleCount().ToString() + "  naehrwerte=" + ChefZ_NutritionManager.Get().GetRecordCount().ToString();
        chefzTxt1 = chefzTxt1 + "  prozesse=" + ChefZ_ProcessingManager.Get().GetProcessCount().ToString() + "  stationen=" + ChefZ_ProcessingManager.Get().GetStationCount().ToString() + "  transforms=";
        chefzTxt1 = chefzTxt1 + ChefZ_ProcessingManager.Get().GetTransformCount().ToString() + "  werkzeuggruppen=" + ChefZ_ToolRegistry.Get().GetGroupCount().ToString() + "  handwerksrezepte=" + ChefZ_HandcraftBridge.GetRegisteredCount().ToString();
        chefzTxt1 = chefzTxt1 + "/" + ChefZ_HandcraftBridge.GetSlotCount().ToString() + "  portionsgerichte=" + ChefZ_PortionManager.Get().GetSpecCount().ToString() + "  behaelter=";
        chefzTxt1 = chefzTxt1 + ChefZ_ContainerRegistry.Get().GetClassCount().ToString() + "  behaelterkategorien=" + ChefZ_ContainerRegistry.Get().GetCategoryCount().ToString() + "  aktiv=" + cfg.IsActive().ToString();
        ChefZ_Log.Banner(chefzTxt1);

        // S13: wer haengt an ChefZ? Eine eigene Zeile und ebenfalls an der
        // Stufenpruefung vorbei, weil sie die haeufigste Betreiberfrage
        // beantwortet - "warum tut mein Comp-Modul nichts". Steht hier eine
        // Null, hat sich niemand angemeldet, und die Ursache liegt im anderen
        // Mod, nicht in ChefZ.
        string chefzTxt2 = "Aussenkante " + side + "  abonnenten=" + ChefZ_EventBus.Get().GetSubscriptionCount().ToString() + "  faehigkeitsanbieter=";
        chefzTxt2 = chefzTxt2 + ChefZ_CapabilityRegistry.Get().GetProviderCount().ToString() + "  fortschrittsempfaenger=" + ChefZ_ProgressRegistry.GetSinkCount().ToString() + "  modus=" + ChefZ_CapabilityRegistry.Get().GetMode();
        ChefZ_Log.Banner(chefzTxt2);

        if (cfg.GetHealth() == ChefZ_ConfigHealth.SAFE_MODE)
        {
            ChefZ_Log.Banner("ChefZ ist im SAFE MODE inert. Vanilla-Kochen ist davon " + "unberuehrt und laeuft vollstaendig.");
        }

        ChefZ_Log.Flush();
    }

    //! Nur fuer Diagnose: hat der Boot je gelaufen?
    static bool HasBootedServer()
    {
        return s_ServerDone;
    }

    static bool HasBootedClient()
    {
        return s_ClientDone;
    }
}
