//==============================================================================
// ChefZ_Applicator - die Transaktion. Invariante I5.
//
// Entwurf: 08 §3 (Schnittstelle woertlich), 08 §6 (die Reihenfolge, nicht
// verhandelbar), 08 §8 (Fehlerverhalten Zeile fuer Zeile), 10 §5 (Aufruf aus
// Stufe C), 10 §8 (voller Cargo, entfernte Zutat, Rollback, SUPPRESSED),
// 16 §5 und 16 §7 (Behaelter), 15 §4 (Portionen), 19 S8 (Umfang und die drei
// Negativtests), 00 §5 Invariante I5.
//
// ---------------------------------------------------------------------------
// DIE EINE REGEL DIESER DATEI
// ---------------------------------------------------------------------------
// 08 §6: "Der gefaehrlichste Moment im gesamten Core ist der Uebergang
// 'Zutaten weg, Gericht da'."
//
//     1. REVALIDIEREN   Handles noch da, im selben Gefaess, Menge ausreichend?
//     2. PLATZ PRUEFEN  Passt JEDES Ergebnis ins Ziel?
//     3. BEHAELTER      containerCategory gefordert? Behaelter suchen (16)
//     4. ERZEUGEN       Ergebnisse und Nebenprodukte spawnen
//                       Fehlschlag -> RollbackCreated(), NICHTS verbraucht
//     5. EIGENSCHAFTEN  Zustand, Qualitaet, Frische, Portionen, Temperatur,
//                       Agenten, Behaelterbindung, Effekt-IDs
//     6. VERBRAUCHEN    ZULETZT
//     7. EVENTS
//
// Schritt 6 ist der letzte, und zwar ausnahmslos. Jeder Abbruch davor laesst
// die Zutaten unangetastet. Das ist keine Stilfrage: ein verlorenes Gericht
// ist ein Bugreport, verlorene Zutaten sind ein wuetender Spieler.
//
// Wer diese Datei aendert, prueft danach die drei Negativtests aus 19 S8:
//   N1  voller Cargo            -> nichts verbraucht, nichts erzeugt
//   N2  Zutat zwischenzeitlich entfernt -> nichts verbraucht, nichts erzeugt
//   N3  fehlende Ergebnisklasse -> nichts verbraucht, nichts erzeugt
// Sie sind Abnahmebedingung, nicht Kuer.
//
// ---------------------------------------------------------------------------
// Warum ERZEUGEN vor VERBRAUCHEN, obwohl umgekehrt einfacher waere
// ---------------------------------------------------------------------------
// Umgekehrt entstuende der Platz fuer das Ergebnis durch den Verbrauch, und
// die Platzpruefung fiele weg. Der Preis waere, dass jeder Fehler zwischen
// Verbrauch und Erzeugung ersatzlosen Zutatenverlust bedeutet - der
// schlimmstmoegliche Fehler in einem Kochmod.
//
// Das Restrisiko dieser Richtung ist eine Duplikation bei einem Fehler
// zwischen Schritt 4 und Schritt 6, und dagegen steht RollbackCreated().
//
// ---------------------------------------------------------------------------
// Server. Ausschliesslich.
// ---------------------------------------------------------------------------
// Diese Datei ist die einzige Stelle des Core, die Items erzeugt und
// verbraucht. Sie prueft g_Game.IsServer() selbst und verlaesst sich nicht
// darauf, dass der Aufrufer es getan hat.
//
// KEIN CONTENT: kein Klassenname, keine Kategorie, kein Gericht, keine Zutat.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_Applicator
{
    /**
     * Mengenschwelle. Unterhalb davon gilt eine Menge als "keine Menge".
     *
     * Derselbe Wert wie im ChefZ_SlotEvaluator, aus demselben Grund: die
     * Mengen dort und hier sind dieselben Zahlen, und zwei verschiedene
     * Schwellen ergaeben einen Verbrauchsplan, den der Applicator fuer
     * unerfuellbar haelt.
     */
    static const float EPS = 0.001;

    /**
     * Nur fuer den Selbsttest: unterdrueckt die Meldungen dieser Klasse.
     *
     * Noetig, weil der Test die Fehlerfaelle absichtlich durchspielt -
     * fehlende Ergebnisklasse, geforderter Behaelter, entfernte Zutat. Liefen
     * sie ins Log, staende im RPT ein Fehler, den es nicht gibt, und
     * ChefZ_Log.GetErrorCount() speist die Safe-Mode-Schwelle (18 §4): der
     * Test koennte den Server in den SAFE_MODE treiben. Dieselbe Loesung wie
     * im ChefZ_CategoryManager.
     */
    private static bool s_QuietForTest;

    //--- Zaehler fuer "chefz stats" (18 §2) ----------------------------------
    private static int s_CountApplied;
    private static int s_CountFailed;
    private static int s_CountCreated;
    private static int s_CountConsumed;
    private static int s_CountRolledBack;

    //==========================================================================
    // Der Eintritt (08 §3, woertlich)
    //==========================================================================

    /**
     * Wendet ein gebundenes Ergebnis an - ganz oder gar nicht.
     *
     * @param result     das gebundene Rezept. Die Handles darin MUESSEN aus
     *                   derselben Faktenerhebung stammen wie entities; der
     *                   Adapter bindet dafuer unmittelbar vor dem Aufruf neu
     *                   (10 §5, Stufe C).
     * @param entities   die parallele Entity-Liste des ChefZ_FactCollector.
     *                   entities[handle] ist das Item zu einem Handle
     *                   (05 §3.4).
     * @param device     das Gefaess. Ziel der Ergebnisse und zugleich der
     *                   Massstab fuer "ist die Zutat noch da, wo sie war".
     * @param ctx        der Kochkontext derselben Auswertung.
     * @param outCreated wird geleert und mit dem gefuellt, was tatsaechlich
     *                   entstanden ist. Bei false ist die Liste LEER - ein
     *                   Fehlschlag hinterlaesst nichts.
     * @param err        Klartextgrund bei false. Nie leer, wenn false.
     *
     * @return true nur dann, wenn Ergebnisse entstanden UND Zutaten verbraucht
     *         wurden. Bei false ist die Welt unveraendert.
     */
    static bool Apply(notnull ChefZ_MatchResult result, notnull array<ItemBase> entities, notnull ItemBase device, notnull ChefZ_CookContext ctx, out array<ItemBase> outCreated, out string err)
    {
        err = "";

        // Ueber eine lokale Zwischenvariable: der Aufrufer darf seinen
        // wiederverwendeten Puffer mitbringen, und ein Feld als out-Parameter
        // ist in Enforce nicht zugesichert (siehe ChefZ_TextList.SymbolsOf).
        array<ItemBase> created = outCreated;
        if (!created)
            created = new array<ItemBase>();
        created.Clear();
        outCreated = created;

        //--- Torwaechter ------------------------------------------------------
        // Nichts Autoritatives auf dem Client (00 §5). Der Client sieht das
        // Ergebnis ueber die normale Inventar- und Variablensynchronisation.
        if (!g_Game || !g_Game.IsServer())
        {
            err = "Anwendung ausserhalb des Servers angefordert";
            return Failure(err);
        }

        if (!result.matched || !result.recipe)
        {
            err = "kein gebundenes Rezept";
            return Failure(err);
        }

        ChefZ_CompiledRecipe recipe = result.recipe;

        if (device.IsSetForDeletion())
        {
            err = "das Gefaess wird gerade geloescht";
            return Failure(err);
        }

        //--- 1. REVALIDIEREN (08 §6) -----------------------------------------
        //
        // 08 §8: "Handle verweist auf inzwischen geloeschtes Entity ->
        // ValidateHandles schlaegt fehl -> NICHTS verbraucht, NICHTS erzeugt,
        // WARN. Naechster Tick versucht es erneut."
        string why;
        if (!ValidateHandlesEx(result, entities, device, why))
        {
            err = "Revalidierung fehlgeschlagen: " + why;
            return Failure(err);
        }

        // Dieselbe Frage fuer die Fluessigkeit im Gefaess (08 §2,
        // policy.liquidConsumed). Sie ist keine Zutat mit Handle, sondern eine
        // Eigenschaft des Gefaesses - und sie wird genauso VOR jeder
        // Veraenderung geprueft.
        float liquidNeeded = LiquidToConsume(recipe);
        if (liquidNeeded > EPS && !HasEnoughLiquid(device, liquidNeeded, why))
        {
            err = "Fluessigkeit reicht nicht: " + why;
            return Failure(err);
        }

        //--- 2./3. PLANEN: Klassen aufloesen, Zufall werfen, Behaelter -------
        array<ref ChefZ_PlannedOutput> planned = new array<ref ChefZ_PlannedOutput>();
        if (!PlanOutputs(recipe, result, planned, why))
        {
            err = "Planung fehlgeschlagen: " + why;
            return Failure(err);
        }

        if (planned.Count() == 0)
        {
            // Kann nicht vorkommen: 08 §8 weist Rezepte ohne outputs beim
            // Laden ab ("es wuerde Zutaten loeschen und nichts erzeugen").
            // Die zweite Sicherung steht trotzdem hier - und sie verbraucht
            // ausdruecklich NICHTS.
            err = "das Rezept haette nichts erzeugt - Abbruch vor jedem Verbrauch";
            return Failure(err);
        }

        //--- 2. PLATZ PRUEFEN -------------------------------------------------
        if (!HasRoomForAll(device, planned, why))
        {
            err = "kein Platz: " + why;
            return Failure(err);
        }

        //--- 4. ERZEUGEN ------------------------------------------------------
        for (int i = 0; i < planned.Count(); i++)
        {
            ChefZ_PlannedOutput p = planned.Get(i);

            string spawnErr;
            ItemBase item = SpawnOutput(p.def, result.qualityTier, device, spawnErr);
            if (!item)
            {
                // 08 §8: "CreateInInventory liefert null -> Abbruch VOR dem
                // Verbrauch, bereits Erzeugtes wird geloescht, ERROR."
                RollbackCreated(created);
                err = p.Where() + " (" + p.cls + ") konnte nicht erzeugt werden: " + spawnErr;
                return Failure(err);
            }

            p.created = item;
            created.Insert(item);
        }

        //--- 5. EIGENSCHAFTEN -------------------------------------------------
        //
        // Die Zutaten leben hier noch - das ist der Grund, warum dieser
        // Schritt VOR dem Verbrauch steht und nicht danach: Temperatur und
        // Agenten werden von ihnen abgelesen.
        if (!ApplyProperties(planned, result, entities, device, ctx, why))
        {
            RollbackCreated(created);
            err = "Eigenschaften konnten nicht gesetzt werden: " + why;
            return Failure(err);
        }

        //--- 6. VERBRAUCHEN - ZULETZT (Invariante I5) ------------------------
        ConsumeInputs(result, entities);
        ConsumeLiquid(device, liquidNeeded);

        //--- 7. EVENTS --------------------------------------------------------
        //
        // ChefZ_OnRecipeCompleted, recipe.emitEvents und
        // ProgressRegistry.Report("cook") gehoeren an DIESEN Zeitpunkt
        // (08 §6, Schritt 7) - das Ergebnis existiert, die Zutaten sind
        // verbraucht, die Transaktion ist durch.
        //
        // Ausgeloest werden sie seit S13 vom AUFRUFER, unmittelbar nachdem
        // diese Methode true geliefert hat: ChefZ_CookingDeviceAdapter.
        // RaiseRecipeCompleted(). Grund ist die Datenlage, nicht der
        // Geschmack - die Nutzlast nennt die verbrauchten Zutatenklassen
        // (17 §3.1), und die stehen in der Faktenliste, die dem Adapter
        // gehoert. Der Applicator sieht nur Handles und Entities; er koennte
        // die Liste nicht bauen, ohne sie sich zweimal erheben zu lassen.
        //
        // Dieselbe Aufteilung wie bei der Qualitaetsstufe (12 §6): berechnet
        // wird, wo die Eingaben liegen; angewandt wird, wo das Ergebnis
        // entsteht.
        //
        // recipe.emitEvents und recipe.effects werden vom Core NIE
        // ausgewertet (08 §2) - sie werden nur weitergereicht.

        s_CountApplied++;
        s_CountCreated = s_CountCreated + created.Count();

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.INFO))
        {
            ChefZ_Log.Info(ChefZ_LogChannel.COOK, "Angewandt: " + recipe.id + " -> " + DescribeCreated(planned) + ", verbraucht " + result.consumePlan.Count().ToString() + " Eintraege.");
        }

        return true;
    }

    //==========================================================================
    // Schritt 1 - Revalidieren (08 §6)
    //==========================================================================

    /**
     * Die Fassung aus 08 §3, woertlich.
     *
     * Ohne Gefaess: sie prueft dann nur, ob die Handles aufloesbar sind, die
     * Entities leben und die geplanten Mengen noch da sind. Der Test "im
     * selben Gefaess" braucht das Gefaess und steht deshalb in
     * ValidateHandlesEx - die Apply() benutzt.
     */
    static bool ValidateHandles(notnull ChefZ_MatchResult result, notnull array<ItemBase> entities)
    {
        string why;
        return ValidateHandlesEx(result, entities, null, why);
    }

    /**
     * "Sind alle Handles noch da, im selben Gefaess, mit ausreichender Menge?"
     * (08 §6, Schritt 1)
     *
     * Drei Fragen, drei Pruefungen - und jede einzelne endet in false, ohne
     * dass irgendetwas veraendert worden waere.
     *
     * @param device darf null sein; dann entfaellt die Gefaesspruefung.
     * @param why    Klartextgrund bei false, fuer Trace und Log.
     *
     * Die Zeitspanne zwischen Bindung und Anwendung ist normalerweise
     * winzig - der Adapter bindet unmittelbar vor dem Abschluss neu (10 §5).
     * Sie ist trotzdem nicht null, und genau dafuer ist dieser Schritt da:
     * ein Spieler kann im selben Tick etwas herausnehmen, und ein Item kann
     * durch Vanillas eigenen Kochtick verschwinden (Cooking.c zieht bei jedem
     * Stufenwechsel Menge ab).
     */
    static bool ValidateHandlesEx(notnull ChefZ_MatchResult result, notnull array<ItemBase> entities, ItemBase device, out string why)
    {
        why = "";

        int i;

        // Ueber eine lokale Zwischenvariable und nicht direkt in den
        // out-Parameter: einen out-Parameter als out-Parameter weiterzureichen
        // ist in Enforce nirgends zugesichert (siehe Kopf von
        // ChefZ_TextList.SymbolsOf).
        string sub;

        // a) jedes gebundene Item existiert noch und liegt noch im Gefaess
        for (i = 0; i < result.boundHandles.Count(); i++)
        {
            int handle = result.boundHandles.Get(i);
            ItemBase item = EntityOf(entities, handle);
            if (!item)
            {
                why = "gebundenes Item #" + handle.ToString() + " existiert nicht mehr";
                return false;
            }

            if (!IsStillIn(item, device, sub))
            {
                why = "gebundenes Item #" + handle.ToString() + ": " + sub;
                return false;
            }
        }

        // b) jeder Eintrag des Verbrauchsplans ist ausfuehrbar
        //
        // Der Plan enthaelt AUCH Handles, die nicht in boundHandles stehen:
        // bei policy extraItems "consume" wandern die Fremdkoerper hinein
        // (08 §2). Sie werden hier genauso geprueft - was verbraucht werden
        // soll, muss vorher da sein.
        for (i = 0; i < result.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = result.consumePlan.Get(i);
            if (!plan)
                continue;

            ItemBase target = EntityOf(entities, plan.handle);
            if (!target)
            {
                why = "Verbrauchsziel #" + plan.handle.ToString() + " existiert nicht mehr";
                return false;
            }

            if (!IsStillIn(target, device, sub))
            {
                why = "Verbrauchsziel #" + plan.handle.ToString() + ": " + sub;
                return false;
            }

            // "mit ausreichender Menge": nur fuer den Teilabzug. Wer ganz
            // geloescht wird, braucht keine Mindestmenge - er wird ohnehin
            // nicht geteilt.
            if (plan.destroyWhole || plan.quantityDelta <= EPS)
                continue;

            if (!target.HasQuantity())
            {
                why = "Verbrauchsziel #" + plan.handle.ToString()
                    + " fuehrt keine Menge, es sollen aber " + plan.quantityDelta.ToString()
                    + " abgezogen werden";
                return false;
            }

            if (target.GetQuantity() + EPS < plan.quantityDelta)
            {
                why = "Verbrauchsziel #" + plan.handle.ToString() + " hat nur noch "
                    + target.GetQuantity().ToString() + " von " + plan.quantityDelta.ToString();
                return false;
            }
        }

        return true;
    }

    /**
     * Lebt das Item noch, und liegt es noch im selben Gefaess?
     *
     * HasEntityInCargo und nicht GetHierarchyParent: ein Item kann im Cargo
     * eines Gefaesses liegen, ohne dass der Hierarchieelternteil die Antwort
     * eindeutig macht (Proxy-Cargo). Die Frage lautet genau "liegt es in
     * diesem Cargo", und dafuer gibt es genau diese Auskunft.
     */
    private static bool IsStillIn(notnull ItemBase item, ItemBase device, out string why)
    {
        why = "";

        if (item.IsSetForDeletion())
        {
            why = "wird gerade geloescht";
            return false;
        }

        if (!device)
            return true;

        GameInventory inv = device.GetInventory();
        if (!inv)
        {
            why = "das Gefaess hat kein Inventar mehr";
            return false;
        }

        if (!inv.HasEntityInCargo(item))
        {
            why = "liegt nicht mehr im Gefaess";
            return false;
        }

        return true;
    }

    //! entities[handle], oder null bei jedem denkbaren Unsinn.
    static ItemBase EntityOf(notnull array<ItemBase> entities, int handle)
    {
        if (handle < 0 || handle >= entities.Count())
            return null;
        return entities.Get(handle);
    }

    //==========================================================================
    // Schritt 2 und 3 - Planen, Behaelter, Platz (08 §6, 16 §5)
    //==========================================================================

    /**
     * Was soll entstehen?
     *
     * Hier faellt der Zufallswurf fuer Nebenprodukte - genau einmal, siehe
     * Kopf von ChefZ_PlannedOutput. Hier faellt auch die Entscheidung, ob ein
     * Ergebnis ueberhaupt erzeugbar ist:
     *
     *   - Klasse leer                 -> Abbruch (kann der Compiler nicht,
     *                                    aber die zweite Sicherung kostet
     *                                    nichts)
     *   - Klasse nicht in CfgVehicles -> Abbruch (N3). Der Compiler weist das
     *                                    beim Build ab (08 §8), sodass es die
     *                                    Laufzeit nie erreichen SOLLTE. Die
     *                                    Pruefung steht hier trotzdem: der
     *                                    Preis ist ein Config-Zugriff je
     *                                    fertigem Gericht, der Gegenwert ist
     *                                    die Gewissheit, dass ein Loch in der
     *                                    Buildpruefung keine Zutaten kostet.
     *   - containerCategory gefordert -> Abbruch, solange es das
     *                                    Behaeltersystem nicht gibt (16 §7:
     *                                    "Kein Behaelter im Zugriff ->
     *                                    Ausfuehrung abgebrochen, KEIN
     *                                    Zutatenverbrauch")
     *
     * @return false bei Abbruch. Dann ist nichts erzeugt und nichts geplant.
     */
    static bool PlanOutputs(notnull ChefZ_CompiledRecipe recipe, notnull ChefZ_MatchResult result, notnull array<ref ChefZ_PlannedOutput> planned, out string why)
    {
        why = "";
        planned.Clear();

        // Lokale Zwischenvariable, siehe ValidateHandlesEx.
        string sub;

        if (!CollectPlanned(recipe, recipe.outputs, false, result.qualityTier, planned, sub))
        {
            why = sub;
            return false;
        }

        if (!CollectPlanned(recipe, recipe.byproducts, true, result.qualityTier, planned, sub))
        {
            why = sub;
            return false;
        }

        return true;
    }

    private static bool CollectPlanned(notnull ChefZ_CompiledRecipe recipe, array<ref ChefZ_OutputDef> list, bool byproduct, ChefZ_Sym tier, notnull array<ref ChefZ_PlannedOutput> planned, out string why)
    {
        why = "";
        if (!list)
            return true;

        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_OutputDef def = list.Get(i);
            if (!def)
                continue;

            ChefZ_PlannedOutput p = new ChefZ_PlannedOutput();
            p.def       = def;
            p.byproduct = byproduct;
            p.index     = i;
            p.cls       = ResolveOutputClass(def, tier);

            if (p.cls == "")
            {
                why = p.Where() + " hat keine Ergebnisklasse";
                return false;
            }

            if (!ResultClassExists(p.cls))
            {
                // N3. Ausdruecklich VOR jedem Verbrauch und vor jeder
                // Erzeugung: ein Rezept mit einer Klasse, die es nicht gibt,
                // darf keine einzige Zutat kosten.
                Note(ChefZ_LogLevel.ERR, ChefZ_LogChannel.COOK, "apply.class.missing." + recipe.id + "." + p.cls, "Rezept " + recipe.id + ": " + p.Where() + " nennt die Klasse \"" + p.cls + "\", die es in CfgVehicles nicht gibt. Das Rezept wird nicht " + "angewandt, es wird NICHTS verbraucht und NICHTS erzeugt. Der " + "Rezeptbau weist solche Klassen normalerweise bereits beim Laden ab - " + "steht diese Zeile im Log, fehlt dort eine Pruefung oder das Modul mit " + "der Klasse ist nicht geladen.");

                why = p.Where() + ": Klasse \"" + p.cls + "\" existiert nicht";
                return false;
            }

            /**
             * 16 §5: "containerCategory gesetzt? nein -> Gericht direkt
             * erzeugen." Ja -> Behaelter suchen. Der ChefZ_ContainerService
             * entsteht in S17 (M3); bis dahin findet die Suche mit Sicherheit
             * nichts, und die Antwort darauf ist dieselbe wie spaeter bei
             * einem fehlenden Behaelter: Abbruch ohne jeden Verbrauch.
             *
             * NUR fuer NICHT portionierte Ergebnisse - seit S16.
             *
             * Bei einem Portionsgericht bezeichnet containerCategory den
             * Behaelter, den die ENTNAHME braucht (15 §3: "Behaelter, den die
             * Entnahme braucht"), nicht den, den das Kochen braucht. 16 §2
             * sagt es woertlich: "Beim Servieren, nie beim Kochen" - und weil
             * Einzelgerichte Portionsgerichte mit portions = 1 sind (15 E7),
             * laeuft JEDE Behaelterfrage ueber die Entnahmeaktion.
             *
             * Ohne diese Unterscheidung waere ein Kessel Eintopf mit
             * "containerCategory": "BOWL" nicht kochbar - der Applicator
             * verlangte eine Schuessel, die erst beim Portionieren gebraucht
             * wird.
             */
            if (def.containerCategory != "" && !def.IsPortioned())
            {
                Note(ChefZ_LogLevel.WARN, ChefZ_LogChannel.CONTAIN, "apply.container.pending." + recipe.id, "Rezept " + recipe.id + ": " + p.Where() + " verlangt einen Behaelter. " + "Das Behaeltersystem entsteht in einem spaeteren Schritt (16); bis " + "dahin wird dieses Rezept nicht angewandt. Es wird NICHTS verbraucht " + "und NICHTS erzeugt, Vanilla-Kochen laeuft unveraendert weiter.");

                why = p.Where() + " verlangt einen Behaelter, den es noch nicht gibt";
                return false;
            }

            // Nebenprodukte mit chance < 1: der Wurf faellt GENAU HIER, einmal
            // je Transaktion. Warum nicht zweimal - einmal fuer die
            // Platzpruefung, einmal fuers Erzeugen -, steht im Kopf von
            // ChefZ_PlannedOutput.
            //
            // Ein unaufgeloester Sentinel gilt als "immer". Er kann hier nicht
            // ankommen (ResolveDefaults setzt 1.0), und die Alternative waere,
            // ein Ergebnis stillschweigend ausfallen zu lassen - der Autor
            // saehe ein Rezept, das manchmal nichts erzeugt, und faende die
            // Ursache nie.
            if (!ChefZ_Undefined.IsFloatUndefined(def.chance) && def.chance < 1.0)
            {
                if (def.chance <= 0.0)
                    continue;
                if (Math.RandomFloat01() > def.chance)
                    continue;
            }

            planned.Insert(p);
        }

        return true;
    }

    /**
     * Grundklasse oder Qualitaetsvariante (08 §2, 12 §3).
     *
     * Solange der Quality Manager (S10) die Stufe nicht setzt, ist tier
     * INVALID und es gewinnt immer die Grundklasse. Das ist die richtige
     * Richtung: eine Variante ohne belegte Stufe waere geraten.
     */
    static string ResolveOutputClass(notnull ChefZ_OutputDef def, ChefZ_Sym tier)
    {
        if (!ChefZ_SymbolTable.IsValid(tier) || !def.variants)
            return def.cls;

        string tierName = ChefZ_SymbolTable.Name(tier);
        for (int i = 0; i < def.variants.Count(); i++)
        {
            ChefZ_OutputVariant v = def.variants.Get(i);
            if (!v || v.cls == "")
                continue;
            if (v.tier == tierName)
                return v.cls;
        }

        return def.cls;
    }

    //! Gibt es die Klasse ueberhaupt? Ohne laufendes Spiel (Selbsttest) ist
    //! die Frage nicht stellbar - dann gilt sie als beantwortet, sonst
    //! scheiterte jeder Test an einer fehlenden Engine.
    static bool ResultClassExists(string cls)
    {
        if (cls == "")
            return false;
        if (!g_Game)
            return true;
        // Seit S12 ueber ChefZ_VanillaNutrition: derselbe Configzugriff, den
        // auch der Rezeptcompiler und der Startaudit benutzen (13 §3).
        return ChefZ_VanillaNutrition.ClassExists(cls);
    }

    /**
     * Schritt 2: passt JEDES Ergebnis ins Gefaess? (08 §6, 10 §8)
     *
     * Negativtest N1. Ist kein Platz, ist "Rezept loest nicht aus" die
     * richtige Antwort (08 §6) - der Spieler nimmt etwas heraus und es klappt.
     * Ein stiller Notausgang (Boden, Spielerinventar, anderes Gefaess) waere
     * genau das Gegenteil dessen, was 10 §8 verlangt.
     *
     * EHRLICH BENANNTE GRENZE: FindFirstFreeLocationForNewEntity beantwortet
     * "passt EIN Stueck dieser Klasse", nicht "passen diese drei zusammen".
     * Bei mehreren Ergebnissen kann die Vorpruefung deshalb gruen sein,
     * waehrend das zweite Stueck dann doch keinen Platz findet. Genau dafuer
     * steht RollbackCreated in Schritt 4: der Fehlschlag kostet ein bereits
     * erzeugtes Gericht, aber keine einzige Zutat. Die Vorpruefung faengt den
     * haeufigen Fall billig ab, der Rollback deckt den seltenen sicher.
     */
    static bool HasRoomForAll(notnull ItemBase device, notnull array<ref ChefZ_PlannedOutput> planned, out string why)
    {
        why = "";

        GameInventory inv = device.GetInventory();
        if (!inv)
        {
            why = "das Gefaess hat kein Inventar";
            return false;
        }

        for (int i = 0; i < planned.Count(); i++)
        {
            ChefZ_PlannedOutput p = planned.Get(i);
            if (FindCargoSpot(inv, p.cls))
                continue;

            why = p.Where() + " (" + p.cls + ") passt nicht ins Gefaess";
            return false;
        }

        return true;
    }

    //! Freier Cargoplatz fuer eine noch nicht existierende Klasse, oder null.
    //! Ausdruecklich CARGO: ein Ergebnis, das sich als Anbauteil an das
    //! Gefaess haengt oder in der Hand landet, waere kein Kochergebnis mehr.
    private static InventoryLocation FindCargoSpot(notnull GameInventory inv, string cls)
    {
        InventoryLocation loc = new InventoryLocation();
        if (!inv.FindFirstFreeLocationForNewEntity(cls, FindInventoryLocationType.CARGO, loc))
            return null;
        return loc;
    }

    //==========================================================================
    // Schritt 4 - Erzeugen (08 §3, woertlich)
    //==========================================================================

    /**
     * Erzeugt EIN Ergebnis im Cargo des Gefaesses.
     *
     * @return die Instanz oder null. Bei null ist NICHTS entstanden - auch
     *         kein halbes Objekt, das jemand aufraeumen muesste.
     *
     * Der Platz wird hier erneut gesucht und nicht aus Schritt 2 uebernommen:
     * jedes bereits erzeugte Ergebnis hat den Platz veraendert, und eine
     * gemerkte InventoryLocation waere dann veraltet.
     */
    static ItemBase SpawnOutput(notnull ChefZ_OutputDef outDef, ChefZ_Sym tier, notnull ItemBase device, out string err)
    {
        err = "";

        string cls = ResolveOutputClass(outDef, tier);
        if (cls == "")
        {
            err = "keine Ergebnisklasse";
            return null;
        }

        GameInventory inv = device.GetInventory();
        if (!inv)
        {
            err = "das Gefaess hat kein Inventar";
            return null;
        }

        InventoryLocation loc = FindCargoSpot(inv, cls);
        if (!loc)
        {
            err = "kein freier Platz im Gefaess";
            return null;
        }

        EntityAI parent = loc.GetParent();
        if (!parent || !parent.GetInventory())
        {
            err = "der gefundene Platz hat kein Inventar";
            return null;
        }

        EntityAI spawned = parent.GetInventory().CreateEntityInCargoEx( cls, loc.GetIdx(), loc.GetRow(), loc.GetCol(), loc.GetFlip());

        if (!spawned)
        {
            err = "die Engine hat nichts erzeugt";
            return null;
        }

        ItemBase item = ItemBase.Cast(spawned);
        if (!item)
        {
            // Entstanden, aber unbrauchbar. Es wird sofort wieder entfernt -
            // ein herrenloses Objekt im Topf waere schlimmer als gar keines.
            spawned.Delete();
            err = "die erzeugte Klasse ist kein ItemBase";
            return null;
        }

        return item;
    }

    //==========================================================================
    // Schritt 5 - Eigenschaften (08 §6)
    //==========================================================================

    /**
     * Setzt alles, was am fertigen Gericht steht.
     *
     * 08 §6 nennt: Zustand, Qualitaet, Frische, Portionen, Temperatur,
     * Agenten, Behaelterbindung, Effekt-IDs. Was davon in diesem Schritt
     * tatsaechlich gesetzt werden KANN, haengt daran, welche Systeme es gibt:
     *
     *   Menge         hier                           (08 §2, quantityMode)
     *   Temperatur    hier                           (08 §2, inheritTemperature)
     *   Agenten       hier                           (siehe TransferAgents)
     *   Zustand       ChefZ_StateManager, S9   (06)
     *   Frische       ChefZ_Edible_Base, S9    (06 / 14)
     *   Qualitaet     ChefZ_QualityManager, S10 (12)
     *   Portionen     ChefZ_PortionManager, S16 (15)
     *   Behaelter     ChefZ_ContainerService, S17 (16)
     *   Effekt-IDs    werden vom Core nie ausgewertet, nur weitergereicht (08 §2)
     *
     * Die Portionszahl wird NACH der Qualitaetsstufe gesetzt, weil sie diese
     * liest (12 §2: yieldMultiplier und portionBonus). Vor ihr gesetzt waere
     * jede Stufe wirkungslos - und das faellt niemandem auf, weil ein Kessel
     * mit acht statt zehn Portionen immer noch richtig aussieht.
     *
     * Die fehlenden Zeilen sind bewusst LEER und nicht halb geraten. Ein
     * halb gesetzter Zustand waere schlimmer als gar keiner: er saehe richtig
     * aus.
     *
     * @return false nur bei einem echten Fehler. Dann rollt der Aufrufer
     *         zurueck, und es ist weiterhin nichts verbraucht.
     */
    private static bool ApplyProperties(notnull array<ref ChefZ_PlannedOutput> planned, notnull ChefZ_MatchResult result, notnull array<ItemBase> entities, notnull ItemBase device, notnull ChefZ_CookContext ctx, out string why)
    {
        why = "";

        // Beides wird von den Zutaten abgelesen, und die leben nur noch bis
        // Schritt 6. Einmal berechnet statt je Ergebnis erneut.
        float consumedQuantity = ConsumedQuantity(result, entities);
        float inputTemperature = InputTemperature(result, entities, ctx);
        int   inputAgents      = InputAgents(result, entities);
        float inputFreshness   = InputFreshness(result, entities);

        // S16 (15 §5.2): der Mengendeckel misst REZEPTEINHEITEN aus den
        // Pflichtslots, nicht Vanilla-Quantity. Einmal berechnet, weil der
        // Verbrauchsplan fuer alle Ergebnisse derselbe ist.
        float consumedUnits    = ChefZ_PortionManager.ConsumedRequiredUnits(result);

        for (int i = 0; i < planned.Count(); i++)
        {
            ChefZ_PlannedOutput p = planned.Get(i);
            ItemBase item = p.created;

            if (!item || item.IsSetForDeletion())
            {
                why = p.Where() + " (" + p.cls + ") ist unmittelbar nach der Erzeugung "
                    + "wieder verschwunden";
                return false;
            }

            if (!ApplyQuantity(item, p, consumedQuantity))
            {
                // Vanilla hat das eben erzeugte Ergebnis beim Mengensetzen
                // wieder geloescht (varQuantityDestroyOnMin). Weiterzumachen
                // hiesse: Zutaten weg, Gericht weg - genau der Fall, gegen den
                // Invariante I5 steht.
                why = p.Where() + " (" + p.cls + ") wurde beim Setzen der Menge "
                    + "geloescht - die Menge des Rezepts liegt unter dem Minimum der "
                    + "Klasse";
                return false;
            }

            ApplyTemperature(item, p, inputTemperature);

            // Agenten: ChefZ erfindet keine und entfernt keine. Was Vanillas
            // eigener Kochtick an den Zutaten uebrig gelassen hat - er
            // entfernt beim Stufenwechsel alles ausser BRAIN und HEAVYMETAL
            // (Edible_Base.HandleFoodStageChangeAgents) -, geht auf das
            // Gericht ueber. Damit verhaelt sich ein ChefZ-Gericht in dieser
            // Frage wie eine gekochte Vanilla-Zutat, und zwar ohne dass der
            // Core eine eigene Regel dafuer aufstellt.
            if (inputAgents != 0)
                item.TransferAgents(inputAgents);

            // ---- S9 (06): Zustand und Frische ----------------------------
            ApplyChefZState(item, p, inputFreshness);

            // ---- S10 (12): die Qualitaetsstufe ---------------------------
            ApplyChefZQuality(item, result);

            // ---- S16 (15 §4, ERZEUGUNG): die Portionszahl ----------------
            ApplyPortions(item, p, ctx, result.qualityTier, consumedUnits);

            // ---- S17 (16 E3): die Behaelterbindung -----------------------
            ApplyReturnContainer(item, p);

            item.SetSynchDirty();
        }

        return true;
    }

    /**
     * Die Behaelterbindung am fertigen Gericht (16 E3).
     *
     * ---------------------------------------------------------------------
     * Was hier ausdruecklich NICHT passiert: ein Behaelter wird VERBRAUCHT
     * ---------------------------------------------------------------------
     * 16 §2 und 16 E1: "Beim Servieren, nie beim Kochen." Und das ist keine
     * Geschmacksfrage, sondern Vanilla (01 V3): Cooking.ProcessItemToCook
     * behandelt JEDES Cargo-Item, nimmt Temperatur auf und beschaedigt alles,
     * was nicht IsCookware() ist, ueber PARAM_BURN_DAMAGE_COEF - fuenf Prozent
     * je Tick. Ein Teller im Topf ginge im Feuer kaputt. Zusaetzlich
     * veraenderte er die Gefaesssignatur (10 §5) und muesste vom Matcher als
     * Nichtzutat ausgenommen werden - ein Sonderfall im Kern des Systems.
     *
     * In dieser Methode steht deshalb keine Zeile, die etwas verbraucht. Sie
     * schreibt eine Zeichenkette an ein Item, mehr nicht.
     *
     * ---------------------------------------------------------------------
     * Warum nur fuer NICHT portionierte Ergebnisse
     * ---------------------------------------------------------------------
     * Bei einem Portionsgericht entsteht die Bindung an der PORTION, in
     * ChefZ_PortionedFood_Base.ChefZ_TakePortion() - erst dort steht fest,
     * welcher Behaelter tatsaechlich benutzt wurde, und nur dort ist "AUTO"
     * aufloesbar (16 §4). Der Kessel selbst gibt nichts zurueck; er wird
     * gar nicht gegessen.
     *
     * KEIN Rueckgabewert und KEIN Abbruchgrund: eine fehlende Bindung kostet
     * niemanden ein Gericht, und ein Abbruch laege HINTER der Erzeugung - er
     * wuerde das eben entstandene Gericht wieder wegwerfen.
     */
    private static void ApplyReturnContainer(notnull ItemBase item, notnull ChefZ_PlannedOutput p)
    {
        if (p.def.IsPortioned())
            return;

        // Ein Einzelgericht mit containerCategory ist ein Content-Fehler und
        // sieht wie einer aus, den niemand findet: der Autor erwartet, dass
        // eine Schuessel verbraucht wird, und es passiert nichts. 15 E7 nennt
        // den richtigen Weg - ein Einzelgericht IST ein Portionsgericht mit
        // portions = 1, und dann laeuft es ueber die Entnahme.
        if (p.def.containerCategory != "")
        {
            Note(ChefZ_LogLevel.WARN, ChefZ_LogChannel.CONTAIN, "apply.container.atcook." + p.cls, "\"" + p.cls + "\" verlangt einen Behaelter der Kategorie \"" + p.def.containerCategory + "\", ist aber kein Portionsgericht. Ein " + "Behaelter wird NIE beim Kochen verlangt (16 §2) - im Topf ginge er " + "kaputt. Wer ein Tellergericht will, gibt dem Ergebnis portions = 1 " + "und eine portionClass (15 E7); dann wird der Behaelter beim " + "Entnehmen verbraucht. Das Gericht entsteht trotzdem, nur ohne " + "Behaelter.");
        }

        string ret = p.def.returnContainer;
        if (ret == "")
            return;

        if (ret == ChefZ_ContainerDef.AUTO)
        {
            Note(ChefZ_LogLevel.WARN, ChefZ_LogChannel.CONTAIN, "apply.container.auto." + p.cls, "\"" + p.cls + "\" nennt returnContainer \"" + ChefZ_ContainerDef.AUTO + "\", ist aber kein Portionsgericht. \"" + ChefZ_ContainerDef.AUTO + "\" bedeutet \"der Behaelter, der benutzt wurde\" (16 §4) - beim Kochen " + "wird keiner benutzt, also gibt es nichts aufzuloesen. Es kommt " + "nichts zurueck. Wer eine feste Rueckgabe will, nennt die Klasse.");
            return;
        }

        ChefZ_ItemStateComponent.SetReturnContainer(item, ret);
    }

    /**
     * Die Portionszahl am fertigen Gericht (15 §4, ERZEUGUNG).
     *
     * Die RECHNUNG steht im ChefZ_PortionManager - hier steht nur, wo sie
     * ankommt. Das ist die Aufteilung des ganzen Core: gerechnet wird in
     * 3_Game auf Zahlen, geschrieben wird in 4_World auf Items.
     *
     * Der Zaehler landet auf ChefZ_PortionedFood_Base. Ein Ergebnis, das als
     * Portionsgericht deklariert ist, aber nicht von dieser Klasse erbt,
     * bekommt eine Meldung je Klasse und entsteht als gewoehnliches Item -
     * ein Content-Fehler, aber kein Grund, das Gericht zu verweigern (15 §7,
     * Zeile 1: "kein Fehler, nur weniger Komfort").
     *
     * KEIN Rueckgabewert und KEIN Abbruchgrund: eine fehlende Portionszahl
     * kostet niemanden etwas, und ein Abbruch an dieser Stelle laege HINTER
     * der Erzeugung - er wuerde das eben entstandene Gericht wieder wegwerfen.
     */
    private static void ApplyPortions(notnull ItemBase item, notnull ChefZ_PlannedOutput p, notnull ChefZ_CookContext ctx, ChefZ_Sym tier, float consumedUnits)
    {
        if (!p.def.IsPortioned())
            return;

        ChefZ_PortionManager mgr = ChefZ_PortionManager.Get();

        ChefZ_PortionSpec spec;
        if (!mgr.GetSpecForBulk(ChefZ_SymbolTable.Intern(item.GetType()), spec))
        {
            // Die Registry entsteht aus genau diesen Ergebnisdefinitionen -
            // hier nichts zu finden heisst, dass der Manager vor der Engine
            // gebaut wurde oder gar nicht.
            Note(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PORTION, "apply.portions.nospec." + item.GetType(), "\"" + item.GetType() + "\"" + " ist im Rezept als Portionsgericht deklariert, " + "steht aber nicht in der Portionsregistry. Das Gericht entsteht ohne " + "Zaehler und bleibt essbar; entnehmen laesst sich daraus nichts.");
            return;
        }

        ChefZ_PortionedFood_Base bulk = ChefZ_PortionedFood_Base.Cast(item);
        if (!bulk)
        {
            Note(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PORTION, "apply.portions.noclass." + item.GetType(), "\"" + item.GetType() + "\"" + " ist als Portionsgericht deklariert, seine " + "Skriptklasse erbt aber nicht von ChefZ_PortionedFood_Base. Ohne diese " + "Ableitung gibt es keinen Zaehler und keine Entnahmeaktion (15 §3). Das " + "Gericht entsteht als gewoehnliches Item.");
            return;
        }

        array<string> trace = null;
        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PORTION, ChefZ_LogLevel.DEBUG))
            trace = new array<string>();

        int n = mgr.ResolvePortionCount(spec, ctx, consumedUnits, tier, trace);
        bulk.ChefZ_SetPortions(n, n);

        if (trace)
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.PORTION, item.GetType() + ": " + n.ToString() + " Portionen");
            for (int i = 0; i < trace.Count(); i++)
                ChefZ_Log.Debug(ChefZ_LogChannel.PORTION, "    " + trace.Get(i));
        }
    }

    /**
     * Menge am Ergebnis (08 §2, quantityMode).
     *
     *   fixed      quantity, sonst der Klassendefault
     *   fromInput  die Summe der tatsaechlich abgezogenen Vanilla-Menge
     *   ratio      dieselbe Summe mal ratio
     *
     * Ohne "quantity" und ohne verbrauchte Menge bleibt der Klassendefault
     * stehen - das ist der einzige Wert, der mit Sicherheit sinnvoll ist.
     *
     * SetQuantity klemmt selbst auf [min, max] (ItemBase.c:3340) und loescht
     * das Item, wenn der Wert das Minimum erreicht und die Klasse
     * varQuantityDestroyOnMin fuehrt. Deshalb wird ein Wert <= 0 hier gar
     * nicht erst gesetzt: ein Gericht, das im Moment seiner Entstehung wieder
     * verschwindet, waere aus Spielersicht ein Zutatenverlust.
     *
     * @return false, wenn das Item beim Setzen geloescht wurde. Der Aufrufer
     *         bricht dann ab und rollt zurueck - noch ist nichts verbraucht.
     */
    private static bool ApplyQuantity(notnull ItemBase item, notnull ChefZ_PlannedOutput p, float consumedQuantity)
    {
        if (!item.HasQuantity())
            return true;

        ChefZ_OutputDef def = p.def;
        float value = -1.0;

        if (def.quantityMode == "fromInput")
            value = consumedQuantity;
        else if (def.quantityMode == "ratio")
            value = consumedQuantity * def.ratio;
        else if (def.HasQuantity())
            value = def.quantity;

        // Unbekannter Modus: wie "fixed" behandeln, aber einmal melden. Der
        // Compiler laesst den Wert durch (08 §2 kennt drei Namen, mehr nicht),
        // und stillschweigend zu raten waere hier das Falsche.
        if (def.quantityMode != "fixed" && def.quantityMode != "fromInput" && def.quantityMode != "ratio")
        {
            Note(ChefZ_LogLevel.WARN, ChefZ_LogChannel.COOK, "apply.quantityMode." + def.quantityMode, "Unbekannter quantityMode \"" + def.quantityMode + "\" in " + p.Where() + ". Erlaubt sind fixed, fromInput und ratio. Es gilt fixed.");

            value = -1.0;
            if (def.HasQuantity())
                value = def.quantity;
        }

        if (value <= 0.0)
            return true;

        // Der Rueckgabewert von SetQuantity ist "das Item wurde dabei
        // geloescht" (ItemBase.c:3340). Er wird hier ausgewertet und nicht
        // verworfen: er ist die einzige Auskunft darueber, dass das eben
        // erzeugte Gericht schon wieder weg ist.
        return !item.SetQuantity(value);
    }

    /**
     * Temperatur am Ergebnis (08 §2, inheritTemperature).
     *
     * SetTemperatureDirect und nicht SetTemperatureEx: die Ex-Fassung fuehrt
     * Quellenverwaltung, Ueberhitzung und Gefrierfortschritt (EntityAI.c) und
     * ist fuer die Fortschreibung ueber Zeit gedacht. Hier wird ein Startwert
     * gesetzt, einmal, im Moment der Entstehung - und der Wert soll genau der
     * sein, der uebergeben wird.
     */
    private static void ApplyTemperature(notnull ItemBase item, notnull ChefZ_PlannedOutput p, float inputTemperature)
    {
        if (!p.def.inheritTemperature)
            return;
        if (!item.CanHaveTemperature())
            return;
        if (inputTemperature <= 0.0)
            return;

        item.SetTemperatureDirect(inputTemperature);
    }

    //! Die hoechste Temperatur unter den verbrauchten Zutaten, ersatzweise die
    //! des Geraets. Die hoechste und nicht der Durchschnitt: ein Gericht ist
    //! so heiss wie das, was gerade aus dem Topf kommt, und eine kalte Prise
    //! Beigabe soll es nicht abkuehlen.
    private static float InputTemperature(notnull ChefZ_MatchResult result, notnull array<ItemBase> entities, notnull ChefZ_CookContext ctx)
    {
        float best = 0.0;

        for (int i = 0; i < result.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = result.consumePlan.Get(i);
            if (!plan)
                continue;

            ItemBase item = EntityOf(entities, plan.handle);
            if (!item || !item.CanHaveTemperature())
                continue;

            float t = item.GetTemperature();
            if (t > best)
                best = t;
        }

        if (best <= 0.0)
            best = ctx.deviceTemperature;

        return best;
    }

    //! Die Agenten aller verbrauchten Zutaten, ODER-verknuepft.
    private static int InputAgents(notnull ChefZ_MatchResult result, notnull array<ItemBase> entities)
    {
        int agents = 0;

        for (int i = 0; i < result.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = result.consumePlan.Get(i);
            if (!plan)
                continue;

            ItemBase item = EntityOf(entities, plan.handle);
            if (!item)
                continue;

            agents = agents | item.GetAgents();
        }

        return agents;
    }

    /**
     * S9 (06 §4.3, 08 §2): Zustand und Frische am fertigen Gericht.
     *
     * Zustand:  nur wenn das Rezept einen nennt (outputs[].setState). Der
     *           Normalfall in V1 ist ein anderer - dort IST die
     *           Ergebnisklasse der Zustand, ueber ihren defaultState
     *           (06 E2). Beide Wege sind gleichrangig, und der Core hat zu
     *           keinem eine Meinung.
     *
     *           applyVanillaTransition ist hier FALSE: die Vanilla-Kette hat
     *           an einem eben erst erzeugten Item nichts zu tun. Sie wuerde
     *           die Agenten entfernen, die ApplyProperties gerade
     *           uebertragen hat - und damit eine fachliche Entscheidung
     *           treffen, die dem Rezept gehoert und nicht der Buchhaltung
     *           (06 E3).
     *
     * Frische:  inheritFreshness (Default true) uebernimmt die SCHLECHTESTE
     *           Frische der verbrauchten Zutaten, mal freshnessCarry. Das
     *           Minimum und nicht der Mittelwert: eine alte Zutat soll das
     *           Gericht druecken, und ein Durchschnitt liesse sich durch
     *           Beimischen frischer Ware wegrechnen (12).
     *
     * Ein Fehlschlag ist hier KEIN Abbruchgrund. Der Zustand ist eine
     * Zugabe zum Gericht, kein Bestandteil seiner Existenz - fuer ihn die
     * ganze Transaktion zurueckzurollen hiesse, dem Spieler seine Zutaten
     * wegen einer Anzeigeeigenschaft zu erhalten und ihm das Gericht zu
     * nehmen. Die Meldung steht im Log, das Gericht im Topf.
     */
    /**
     * Die Qualitaetsstufe an das Gericht schreiben (12 §6, "BEIM KOCHEN").
     *
     * result.qualityTier hat der ChefZ_CookingDeviceAdapter gesetzt, bevor er
     * angewandt hat - dort liegt die Faktenliste, ohne die sich die Punktzahl
     * nicht rechnen laesst. Bleibt sie INVALID (kein Quality Manager, keine
     * Stufen, SAFE_MODE), passiert hier NICHTS, und das Gericht entsteht ohne
     * Stufe. Genau das verlangt 12 §8: Gerichte entstehen weiterhin, nur ohne
     * Qualitaet.
     *
     * Kein eigener Fehlerpfad: eine unbekannte oder nicht synchronisierbare
     * Stufe meldet ChefZ_ItemStateComponent.SetQuality selbst, einmal je
     * Stufe. Eine Zutat ist zu diesem Zeitpunkt noch nicht verbraucht, aber
     * das Gericht ohne Stufe ist trotzdem das bessere Ergebnis als gar keins.
     */
    private static void ApplyChefZQuality(notnull ItemBase item, notnull ChefZ_MatchResult result)
    {
        if (!ChefZ_SymbolTable.IsValid(result.qualityTier))
            return;
        if (!ChefZ_ItemStateComponent.IsManaged(item))
            return;

        ChefZ_ItemStateComponent.SetQuality(item, result.qualityTier);
    }

    private static void ApplyChefZState(notnull ItemBase item, notnull ChefZ_PlannedOutput p, float inputFreshness)
    {
        if (!ChefZ_ItemStateComponent.IsManaged(item))
            return;

        ChefZ_OutputDef def = p.def;

        if (def.setState != "")
        {
            ChefZ_Sym state = ChefZ_SymbolTable.Lookup(def.setState);
            if (!ChefZ_SymbolTable.IsValid(state))
            {
                Note(ChefZ_LogLevel.WARN, ChefZ_LogChannel.STATE, "apply.setstate." + def.setState, "setState \"" + def.setState + "\" in " + p.Where() + " ist kein bekannter " + "Zustand. Das Gericht entsteht trotzdem; sein Zustand ergibt sich dann aus " + "seiner Klasse (06 §3, Schritt 2).");
            }
            else
            {
                ChefZ_ItemStateComponent.SetState(item, state, false);
            }
        }

        if (!def.inheritFreshness || inputFreshness < 0.0)
            return;

        float carry = def.freshnessCarry;
        if (carry < 0.0)
            carry = 1.0;

        ChefZ_ItemStateComponent.SetFreshness01(item, inputFreshness * carry);
    }

    /**
     * Die schlechteste Frische unter den verbrauchten Zutaten, oder -1, wenn
     * keine einzige davon eine traegt.
     *
     * -1 und nicht 1.0: "niemand hat eine Frische" ist etwas anderes als
     * "alle sind frisch". Im ersten Fall soll das Ergebnis seine eigene
     * Vorgabe behalten, im zweiten den Wert 1.0 bekommen.
     */
    private static float InputFreshness(notnull ChefZ_MatchResult result, notnull array<ItemBase> entities)
    {
        float worst = -1.0;

        for (int i = 0; i < result.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = result.consumePlan.Get(i);
            if (!plan)
                continue;

            ItemBase item = EntityOf(entities, plan.handle);
            if (!item)
                continue;

            ChefZ_ItemStateComponent comp = ChefZ_ItemStateComponent.Of(item);
            if (!comp)
                continue;

            float f = comp.GetFreshness01();
            if (worst < 0.0 || f < worst)
                worst = f;
        }

        return worst;
    }

    //! Wie viel Vanilla-Menge der Plan insgesamt abzieht - die Grundlage fuer
    //! quantityMode "fromInput" und "ratio".
    private static float ConsumedQuantity(notnull ChefZ_MatchResult result, notnull array<ItemBase> entities)
    {
        float sum = 0.0;

        for (int i = 0; i < result.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = result.consumePlan.Get(i);
            if (!plan)
                continue;

            ItemBase item = EntityOf(entities, plan.handle);
            if (!item)
                continue;

            if (plan.destroyWhole)
            {
                sum = sum + item.GetQuantity();
                continue;
            }

            sum = sum + plan.quantityDelta;
        }

        return sum;
    }

    //==========================================================================
    // Schritt 6 - Verbrauchen (08 §3, woertlich). ZULETZT.
    //==========================================================================

    /**
     * Fuehrt den Verbrauchsplan aus.
     *
     * Diese Methode prueft NICHT mehr. Sie darf erst laufen, wenn
     * ValidateHandles gruen war und alle Ergebnisse stehen - Apply() ist die
     * einzige Stelle, die diese Reihenfolge herstellt, und deshalb die
     * einzige, die sie rufen sollte.
     *
     * Sie ist trotzdem oeffentlich, weil 08 §3 sie so fuehrt. Wer sie von
     * anderswo ruft, uebernimmt Invariante I5 in Handarbeit.
     *
     * ObjectDelete gegenueber SetQuantity: 08 §6 nennt beide Wege, und die
     * Wahl steht im Plan, nicht hier. Der Plan setzt destroyWhole bereits
     * dann, wenn nach dem Abzug nichts Brauchbares uebrig bliebe
     * (ChefZ_SlotEvaluator.PlanAmountDraw) - eine Leerhuelle im Topf waere
     * fuer den Spieler nur verwirrend.
     */
    static void ConsumeInputs(notnull ChefZ_MatchResult result, notnull array<ItemBase> entities)
    {
        for (int i = 0; i < result.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = result.consumePlan.Get(i);
            if (!plan)
                continue;

            ItemBase item = EntityOf(entities, plan.handle);
            if (!item)
                continue;               // nach der Revalidierung nicht moeglich

            if (plan.destroyWhole)
            {
                item.Delete();
                s_CountConsumed++;
                continue;
            }

            if (plan.quantityDelta > EPS && item.HasQuantity())
            {
                // Der Rueckgabewert sagt, ob Vanilla das Item dabei geloescht
                // hat (varQuantityDestroyOnMin). Beides ist in Ordnung; das
                // Ergebnis steht bereits.
                item.AddQuantity(-plan.quantityDelta);
                s_CountConsumed++;
            }

            // plan.setStateAfter gilt fuer alles, was ueberlebt (07 §2.3).
            // Der ChefZ-Zustand lebt auf ChefZ_Edible_Base / ChefZ_Item_Base
            // und entsteht mit S9 (06 §4.3) - bis dahin gibt es hier nichts
            // zu setzen. Die Stelle steht bewusst hier, damit S9 sie nicht
            // sucht.
        }
    }

    /**
     * Wie viel Fluessigkeit das Rezept dem Gefaess entnimmt (08 §2,
     * ChefZ_RecipePolicy.liquidConsumed). 0 = keine.
     */
    private static float LiquidToConsume(notnull ChefZ_CompiledRecipe recipe)
    {
        if (!recipe.policy)
            return 0.0;
        if (recipe.policy.liquidConsumed <= 0.0)
            return 0.0;
        return recipe.policy.liquidConsumed;
    }

    //! Hat das Gefaess so viel Fluessigkeit? Wenn nicht, wird NICHTS
    //! verbraucht und NICHTS erzeugt - dieselbe Antwort wie bei einer
    //! fehlenden Zutat.
    private static bool HasEnoughLiquid(notnull ItemBase device, float needed, out string why)
    {
        why = "";

        if (!device.IsLiquidContainer())
        {
            why = "das Gefaess fuehrt gar keine Fluessigkeit";
            return false;
        }

        if (device.GetQuantity() + EPS < needed)
        {
            why = "im Gefaess sind nur " + device.GetQuantity().ToString()
                + " von " + needed.ToString();
            return false;
        }

        return true;
    }

    /**
     * Zieht die Fluessigkeit ab - als Teil von Schritt 6 und nie davor.
     *
     * destroy_config ist ausdruecklich false. Sonst koennte Vanilla das
     * GEFAESS loeschen, sobald seine Menge das Minimum erreicht
     * (ItemBase.SetQuantity, varQuantityDestroyOnMin) - ChefZ wuerde dem
     * Spieler also fuer ein Gericht den Topf einziehen. Ein leeres Gefaess ist
     * die richtige Antwort, kein verschwundenes.
     */
    private static void ConsumeLiquid(notnull ItemBase device, float amount)
    {
        if (amount <= EPS)
            return;
        if (!device.IsLiquidContainer())
            return;

        device.AddQuantity(-amount, false);
        s_CountConsumed++;
    }

    //==========================================================================
    // Rollback (08 §3, woertlich)
    //==========================================================================

    /**
     * Loescht, was in diesem Anlauf entstanden ist.
     *
     * Der Preis eines Fehlers zwischen Schritt 4 und Schritt 6 ist damit ein
     * verlorenes Gericht - nie eine verlorene Zutat und nie eine Duplikation
     * (08 §6).
     *
     * Die Liste wird geleert, damit der Aufrufer nicht versehentlich
     * Ergebnisse herausgibt, die gerade geloescht werden. Genau das sichert
     * Apply() zu: bei false ist outCreated leer.
     *
     * Ein Rollback ist immer eine Meldung wert. Er ist selten und er bedeutet,
     * dass eine der Vorpruefungen die Lage nicht vollstaendig erfasst hat.
     */
    static void RollbackCreated(notnull array<ItemBase> created)
    {
        int deleted = 0;

        for (int i = 0; i < created.Count(); i++)
        {
            ItemBase item = created.Get(i);
            if (!item)
                continue;
            item.Delete();
            deleted++;
        }

        created.Clear();

        if (deleted <= 0)
            return;

        s_CountRolledBack = s_CountRolledBack + deleted;

        if (s_QuietForTest)
            return;

        ChefZ_Log.Warn(ChefZ_LogChannel.COOK, "Rollback: " + deleted.ToString() + " bereits erzeugte(s) Ergebnis(se) wurden " + "wieder geloescht. Es wurde KEINE Zutat verbraucht - der naechste Tick " + "versucht es erneut.");
    }

    //==========================================================================
    // Diagnose (18 §2)
    //==========================================================================

    //! Eine Meldung, die der Selbsttest abschalten kann. Siehe s_QuietForTest.
    private static void Note(int level, int channel, string key, string msg)
    {
        if (s_QuietForTest)
            return;
        ChefZ_Log.Once(level, channel, key, msg);
    }

    //! Nur fuer den Selbsttest (S8).
    static void SetQuietForTest(bool quiet)
    {
        s_QuietForTest = quiet;
    }

    private static bool Failure(string why)
    {
        s_CountFailed++;

        if (s_QuietForTest)
            return false;

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.COOK, "Anwendung abgebrochen, nichts veraendert: " + why);
        }

        return false;
    }

    private static string DescribeCreated(notnull array<ref ChefZ_PlannedOutput> planned)
    {
        string s = "";
        for (int i = 0; i < planned.Count(); i++)
        {
            if (i > 0)
                s = s + ", ";
            s = s + planned.Get(i).cls;
        }
        if (s == "")
            return "(nichts)";
        return s;
    }

    static void ResetCounters()
    {
        s_CountApplied    = 0;
        s_CountFailed     = 0;
        s_CountCreated    = 0;
        s_CountConsumed   = 0;
        s_CountRolledBack = 0;
    }

    static void DumpStats(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Applicator (Transaktion, Invariante I5)");
        outLines.Insert("  angewandt            " + s_CountApplied.ToString());
        outLines.Insert("  abgebrochen          " + s_CountFailed.ToString());
        outLines.Insert("  Ergebnisse erzeugt   " + s_CountCreated.ToString());
        outLines.Insert("  Zutaten verbraucht   " + s_CountConsumed.ToString());
        outLines.Insert("  zurueckgerollt       " + s_CountRolledBack.ToString());
    }

    static int GetAppliedCount()    { return s_CountApplied; }
    static int GetFailedCount()     { return s_CountFailed; }
    static int GetRolledBackCount() { return s_CountRolledBack; }
}
