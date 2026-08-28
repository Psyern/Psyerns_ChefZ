//==============================================================================
// ChefZ_ProcessRunner - die Ausfuehrung eines Transforms. Invariante I5.
//
// Entwurf: 11 §4 (Schnittstelle), 11 §5 ("dieselbe Transaktionsreihenfolge wie
// beim Kochen"), 11 §7 (Fehlerverhalten), 11 E4 ("Es gibt genau einen
// Applicator-Pfad"), 11 §2 (Zustandswechsel ohne Klassenwechsel), 08 §6 (die
// Reihenfolge), 12 §6 / 14 §6 (Qualitaet und Frische bei einem Transform),
// 17 §4 (Ereignisse), 17 E7 (Fortschritt NUR nach Erfolg).
//
// ---------------------------------------------------------------------------
// Diese Datei erzeugt NICHTS selbst - mit genau einer Ausnahme
// ---------------------------------------------------------------------------
// 11 E4 ist unmissverstaendlich: "Ebenso benutzen Transforms dasselbe
// ChefZ_OutputDef und dieselbe Transaktionsreihenfolge wie Rezepte (08 §6).
// Es gibt genau einen Applicator-Pfad."
//
// Genau deshalb baut der Runner aus dem Transform ein ChefZ_MatchResult und
// ein ChefZ_CompiledRecipe zusammen und uebergibt beides an
// ChefZ_Applicator.Apply(). Die sieben Schritte aus 08 §6 - revalidieren,
// Platz pruefen, Behaelter, erzeugen, Eigenschaften, VERBRAUCHEN ZULETZT,
// Ereignisse - laufen damit buchstaeblich in demselben Code wie beim Kochen.
// Ein Fehler in der Reihenfolge kann hier nicht anders sein als dort.
//
// Die AUSNAHME ist der reine Zustandswechsel (11 §2): kein Output nennt eine
// Klasse, es wird nichts erzeugt und nichts verbraucht, nur der ChefZ-Zustand
// der Eingaenge wechselt. Dafuer gibt es im Applicator keinen Pfad - er ist
// gebaut, um Items zu ERZEUGEN, und ein Output ohne Klasse waere dort ein
// Abbruchgrund. Er steht deshalb hier, mit derselben Revalidierung davor und
// ohne jeden Verbrauch.
//
// ---------------------------------------------------------------------------
// Warum das synthetische Rezept KEIN Etikettenschwindel ist
// ---------------------------------------------------------------------------
// Ein ChefZ_CompiledRecipe ist an dieser Stelle nichts anderes als ein
// Traeger fuer outputs[], byproducts[] und policy - genau die drei Dinge, die
// der Applicator daraus liest. Slots, Kontexte, Abschlussbedingung und
// Qualitaetsregeln beruehrt er nie. Das synthetische Rezept wird deshalb auch
// NIE in die ChefZ_RecipeEngine eingetragen: es lebt einen Aufruf lang und ist
// danach fort.
//
// KEIN CONTENT: keine Klasse, kein Prozess, keine Station.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_ProcessRunner
{
    //! Nur fuer den Selbsttest: unterdrueckt die Meldungen dieser Klasse.
    //! Dieselbe Loesung und derselbe Grund wie im ChefZ_Applicator - der Test
    //! spielt die Fehlerfaelle absichtlich durch, und ChefZ_Log.GetErrorCount()
    //! speist die Safe-Mode-Schwelle (18 §4).
    private static bool s_QuietForTest;

    //--- Zaehler fuer "chefz stats" (18 §2) ----------------------------------
    private static int s_CountRun;
    private static int s_CountFailed;
    private static int s_CountStateOnly;

    //==========================================================================
    // Der Eintritt (11 §4)
    //==========================================================================

    /**
     * Fuehrt einen gebundenen Transform aus - ganz oder gar nicht.
     *
     * ABWEICHUNG von der Signatur in 11 §4, und sie ist dieselbe Art von
     * Abweichung wie bei ChefZ_ProcessingManager.Build(): dort steht
     * Run(station, match, entities, outCreated, err). Fuer die Qualitaets- und
     * Frischeregeln eines Transforms (12 §6, 14 §6) braucht der Runner
     * ausserdem die FAKTEN der Eingaenge - ChefZ_QualityManager.CombineRanks()
     * und ChefZ_PreservationManager.InheritFreshness() rechnen auf
     * ChefZ_ItemFacts, nicht auf Entities.
     *
     * Sie sich hier selbst zu erheben waere die Alternative gewesen. Sie ist
     * verworfen, weil dann dieselben Items ZWEIMAL ausgelesen wuerden -
     * einmal fuer die Bindung, einmal fuer die Anwendung - und die beiden
     * Sichten zwischen Bindung und Anwendung auseinanderlaufen koennten. Der
     * Aufrufer hat den Snapshot bereits; er reicht ihn herein.
     *
     * @param station     die Station. Ziel der Ergebnisse und zugleich der
     *                    Massstab fuer "liegt die Zutat noch, wo sie war".
     * @param match       das gebundene Ergebnis aus
     *                    ChefZ_ProcessingManager.FindTransform().
     * @param entities    die parallele Entity-Liste des ChefZ_FactCollector.
     * @param inputs      derselbe Snapshot, aus dem die Bindung entstand.
     * @param actorId     0 = niemand beteiligt.
     * @param outCreated  wird geleert und mit dem gefuellt, was entstanden ist.
     *                    Bei false ist die Liste LEER. Bei einem reinen
     *                    Zustandswechsel bleibt sie leer, obwohl true - es ist
     *                    nichts entstanden, es hat sich etwas geaendert.
     * @param err         Klartextgrund bei false. Nie leer, wenn false.
     *
     * @return true nur, wenn die Transaktion vollstaendig durch ist. Bei false
     *         ist die Welt unveraendert.
     */
    static bool Run(notnull ItemBase station, notnull ChefZ_TransformMatch match, notnull array<ItemBase> entities, ChefZ_FactSnapshot inputs, int actorId, out array<ItemBase> outCreated, out string err)
    {
        err = "";

        // Ueber eine lokale Zwischenvariable: der Aufrufer darf seinen
        // wiederverwendeten Puffer mitbringen, und ein Feld als out-Parameter
        // ist in Enforce nicht zugesichert.
        array<ItemBase> created = outCreated;
        if (!created)
            created = new array<ItemBase>();
        created.Clear();
        outCreated = created;

        //--- Torwaechter ------------------------------------------------------
        // Nichts Autoritatives auf dem Client (00 §5). Der Client sieht das
        // Ergebnis ueber die normale Inventar- und Variablensynchronisation.
        if (!g_Game || !g_Game.IsServer())
            return Failure("Ausfuehrung ausserhalb des Servers angefordert", err);

        if (!match.matched)
            return Failure("kein gebundener Transform", err);

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        ChefZ_CompiledTransform tr  = mgr.GetTransform(match.transformSym);
        if (!tr)
        {
            return Failure("der Transform \"" + match.transformId + "\" ist nicht mehr geladen", err);
        }

        if (station.IsSetForDeletion())
            return Failure("die Station wird gerade geloescht", err);

        //--- Faehigkeiten (17 §3.3) -------------------------------------------
        //
        // ERNEUT geprueft, obwohl FindTransform es bereits getan hat. 11 §5
        // verlangt das ausdruecklich fuer OnFinishProgressServer ("FindTransform
        // ERNEUT - der Zustand kann sich geaendert haben, Client nie glauben"),
        // und fuer einen STATION_TIMED-Job liegen zwischen Start und Abschluss
        // womoeglich vierzig Minuten.
        string capReason;
        if (ChefZ_CapabilityGate.Denies(tr.requires, actorId, capReason))
            return Failure("Faehigkeit fehlt: " + capReason, err);

        //--- Der zweite Pfad: reiner Zustandswechsel (11 §2) ------------------
        if (tr.pureStateChange)
            return RunStateChange(station, match, tr, entities, err);

        //--- Der Normalfall: EIN Applicator-Pfad (11 E4) ----------------------
        return RunApplicator(station, match, tr, entities, inputs, actorId, created, err);
    }

    //==========================================================================
    // HANDCRAFT (11 §3, 11 §5, S15)
    //==========================================================================

    /**
     * Der Abschluss eines Handwerksschritts, den VANILLA erzeugt hat.
     *
     * 11 §5 zeichnet den Weg so:
     *
     *     HANDCRAFT [Server]
     *       Vanillas Craftsystem uebernimmt vollstaendig:
     *       Aktionsfindung, Animation, Softskill-Gewichtung, Werkzeugschaden
     *       -> ChefZ_GenericCraftRecipe.Do() -> ChefZ_ProcessRunner.Run(...)
     *
     * Diese Methode ist das "Run(...)" aus dieser Zeile. Sie ist eine EIGENE
     * Methode und nicht Run() selbst, und der Grund ist struktureller Natur:
     *
     *   Run() erzeugt ueber den ChefZ_Applicator, und der erzeugt
     *   ausschliesslich in den CARGO EINES GEFAESSES (08 §6, SpawnOutput).
     *   Bei HANDCRAFT gibt es kein Gefaess. Die Ergebnisse liegen bereits im
     *   Inventar des Spielers oder auf dem Boden - Vanillas SpawnItems() hat
     *   sie dorthin gelegt, samt Health-Vererbung und Mengenuebertragung,
     *   genau wie 11 E2 es verspricht.
     *
     * Was hier noch fehlt, ist die ChefZ-SCHICHT: Zustand, Stufe, Frische,
     * Temperatur, Menge nach quantityMode - und der VERBRAUCH. Der Verbrauch
     * laeuft ueber ChefZ_Applicator.ConsumeInputs(), also ueber dieselbe
     * Zeile wie beim Kochen und an der Station (11 E4). Es gibt weiterhin
     * genau EINEN Verbrauchspfad.
     *
     * Und er laeuft ZULETZT. Das ist Invariante I5, und sie ist hier so
     * wichtig wie ueberall sonst: alles, was fehlschlagen kann, schlaegt
     * VORHER fehl.
     *
     * @param tool        das Werkzeug unter den Zutaten, oder null. Es ist
     *                    KEINE Zutat und wird nie verbraucht; es dient nur
     *                    als Weltbezug der Ereignisnutzlast.
     * @param results     was Vanilla erzeugt hat, in Registrierungsreihenfolge.
     * @param resultDefs  parallel zu results: das ChefZ_OutputDef, das jedes
     *                    Ergebnis beschreibt. Darf kuerzer sein; ueberzaehlige
     *                    Ergebnisse bleiben dann ohne ChefZ-Schicht.
     *
     * @return true, wenn die ChefZ-Schicht sitzt und verbraucht wurde. Bei
     *         false ist NICHTS verbraucht - der Aufrufer entscheidet dann,
     *         was mit den bereits erzeugten Ergebnissen geschieht.
     */
    static bool RunHandcraft(ItemBase tool, notnull ChefZ_TransformMatch match, notnull array<ItemBase> entities, ChefZ_FactSnapshot inputs, int actorId, array<ItemBase> results, array<ref ChefZ_OutputDef> resultDefs, out string err)
    {
        err = "";

        //--- Torwaechter ------------------------------------------------------
        if (!g_Game || !g_Game.IsServer())
            return Failure("Handwerksschritt ausserhalb des Servers angefordert", err);

        if (!match.matched)
            return Failure("kein gebundener Transform", err);

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        ChefZ_CompiledTransform tr  = mgr.GetTransform(match.transformSym);
        if (!tr)
        {
            return Failure("der Transform \"" + match.transformId + "\" ist nicht mehr geladen", err);
        }

        //--- Faehigkeiten (17 §3.3) -------------------------------------------
        // ERNEUT geprueft, aus demselben Grund wie in Run(): der Zustand kann
        // sich zwischen Bedingung und Abschluss geaendert haben.
        string capReason;
        if (ChefZ_CapabilityGate.Denies(tr.requires, actorId, capReason))
            return Failure("Faehigkeit fehlt: " + capReason, err);

        //--- Die ChefZ-Schicht an den Ergebnissen -----------------------------
        //
        // Alle Werte werden von den EINGAENGEN abgelesen, und die leben nur
        // noch bis zum Verbrauch weiter unten. Einmal berechnet statt je
        // Ergebnis erneut - dieselbe Aufteilung wie in
        // ChefZ_Applicator.ApplyProperties().
        ChefZ_Sym tier        = ResolveQuality(tr, match, inputs);
        float     freshness   = BoundFreshness(match, entities);
        float     temperature = BoundTemperature(match, entities);
        float     consumed    = BoundConsumedQuantity(match, entities);

        // S16 (15 §5.2): der Mengendeckel misst REZEPTEINHEITEN aus den
        // Pflichteingaengen, nicht Vanilla-Quantity.
        float     units       = ChefZ_PortionManager.ConsumedRequiredUnitsOf(match.consumePlan, tr.inputs);

        int applied = 0;
        if (results && resultDefs)
        {
            for (int i = 0; i < results.Count(); i++)
            {
                if (i >= resultDefs.Count())
                    break;

                ItemBase item = results.Get(i);
                if (!item || item.IsSetForDeletion())
                    continue;

                ChefZ_OutputDef def = resultDefs.Get(i);
                if (!def)
                    continue;

                ApplyHandcraftLayer(item, def, tier, freshness, temperature, consumed, units);
                applied++;
            }
        }

        //--- Der zweite Pfad: reiner Zustandswechsel (11 §2) ------------------
        if (tr.pureStateChange)
        {
            int changed = ApplyStateOutputs(tr, match, entities);
            if (changed == 0)
            {
                s_CountFailed++;
                err = "kein einziger Eingang konnte den Zustand wechseln - keines der "
                    + "gebundenen Items fuehrt einen ChefZ-Zustandsblock (06 §4.3). "
                    + "Nichts veraendert.";
                return false;
            }
            s_CountStateOnly++;
        }

        //--- Ereignisse VOR dem Verbrauch --------------------------------------
        //
        // Abweichung von Run(), und sie ist bewusst: dort meldet der Aufrufer
        // NACH der Anwendung, hier steht die Meldung eine Zeile davor. Grund
        // ist die Nutzlast - sie nennt die verbrauchten Zutatenklassen
        // (17 §3.1), und die sind nach ConsumeInputs() nicht mehr sicher
        // ablesbar, weil ein ganz verbrauchtes Item bereits geloescht ist.
        //
        // Die Zusage aus 17 E7 ("Fortschritt NUR nach Erfolg") bleibt
        // unberuehrt: ab hier kann nichts mehr fehlschlagen. Alles, was
        // scheitern konnte, ist oben gescheitert.
        RaiseProcessed(tool, tr, match, entities, results, actorId);

        //--- VERBRAUCHEN - ZULETZT (Invariante I5) ---------------------------
        //
        // Ueber ein synthetisches ChefZ_MatchResult, das AUSSCHLIESSLICH den
        // Verbrauchsplan traegt. Dasselbe Mittel und derselbe Grund wie in
        // BuildResult(): ConsumeInputs() liest genau dieses eine Feld, und ein
        // eigener Verbrauchspfad daneben waere eine zweite Gelegenheit,
        // Invariante I5 zu verletzen.
        ChefZ_MatchResult consume = new ChefZ_MatchResult();
        consume.matched = true;

        int c;
        for (c = 0; c < match.boundHandles.Count(); c++)
            consume.boundHandles.Insert(match.boundHandles.Get(c));
        for (c = 0; c < match.consumePlan.Count(); c++)
            consume.consumePlan.Insert(match.consumePlan.Get(c));

        ChefZ_Applicator.ConsumeInputs(consume, entities);

        s_CountRun++;

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.INFO))
        {
            ChefZ_Log.Info(ChefZ_LogChannel.PROCESS, "Handwerk: " + tr.id + " -> " + applied.ToString() + " Ergebnis(se) mit ChefZ-Schicht, verbraucht " + consume.consumePlan.Count().ToString() + " Eintraege.");
        }

        return true;
    }

    /**
     * Die ChefZ-Schicht an EINEM Ergebnis eines Handwerksschritts.
     *
     * Bewusst DIESELBE Reihenfolge und dieselben Aufrufe wie
     * ChefZ_Applicator.ApplyProperties(): Menge, Temperatur, Zustand und
     * Frische, Stufe, SetSynchDirty. Sie sind hier ausgeschrieben und nicht
     * geteilt, weil die Applicator-Fassung auf ChefZ_PlannedOutput und ein
     * Gefaess zugeschnitten ist - beides gibt es bei HANDCRAFT nicht.
     *
     * Was hier NICHT steht, steht dort auch nicht: die Behaelterbindung (S17)
     * braucht eine Bewertung, die es noch nicht gibt.
     */
    private static void ApplyHandcraftLayer(notnull ItemBase item, notnull ChefZ_OutputDef def, ChefZ_Sym tier, float freshness, float temperature, float consumedQuantity, float consumedUnits)
    {
        //--- Menge (08 §2, quantityMode) --------------------------------------
        //
        // Vanilla hat die Menge bereits gesetzt: den Klassendefault, weil
        // m_ResultSetQuantity auf -1 steht. Ueberschrieben wird sie nur, wenn
        // die Daten es verlangen.
        if (item.HasQuantity())
        {
            float value = -1.0;

            if (def.quantityMode == "fromInput")
                value = consumedQuantity;
            else if (def.quantityMode == "ratio")
                value = consumedQuantity * def.ratio;
            else if (def.HasQuantity())
                value = def.quantity;

            // Ein Wert <= 0 wird nicht gesetzt: SetQuantity klemmt selbst und
            // loescht das Item, wenn die Klasse varQuantityDestroyOnMin fuehrt
            // (ItemBase.c:3340). Ein Ergebnis, das im Moment seiner Entstehung
            // wieder verschwindet, waere aus Spielersicht ein Zutatenverlust.
            if (value > 0.0)
                item.SetQuantity(value);
        }

        //--- Temperatur (08 §2, inheritTemperature) ---------------------------
        if (def.inheritTemperature && temperature > 0.0 && item.CanHaveTemperature())
            item.SetTemperatureDirect(temperature);

        //--- Zustand, Frische, Stufe (06, 12, 14) -----------------------------
        if (ChefZ_ItemStateComponent.IsManaged(item))
        {
            if (def.setState != "")
            {
                ChefZ_Sym state = ChefZ_SymbolTable.Lookup(def.setState);
                if (!ChefZ_SymbolTable.IsValid(state))
                {
                    Note(ChefZ_LogLevel.WARN, "process.setstate." + def.setState, "setState \"" + def.setState + "\" ist kein bekannter Zustand. Das " + "Ergebnis entsteht trotzdem; sein Zustand ergibt sich dann aus " + "seiner Klasse (06 §3, Schritt 2).");
                }
                else
                {
                    ChefZ_ItemStateComponent.SetState(item, state, false);
                }
            }

            if (def.inheritFreshness && freshness >= 0.0)
            {
                float carry = def.freshnessCarry;
                if (carry < 0.0)
                    carry = 1.0;
                ChefZ_ItemStateComponent.SetFreshness01(item, freshness * carry);
            }

            if (ChefZ_SymbolTable.IsValid(tier))
                ChefZ_ItemStateComponent.SetQuality(item, tier);
        }

        //--- Portionen (15 §4, ERZEUGUNG) -------------------------------------
        //
        // NACH der Stufe, weil die Portionszahl sie liest (12 §2).
        //
        // Der Kochkontext ist beim Handwerk LEER: es gibt kein Geraet, also
        // keinen portionCapacity-Deckel (15 §5.1) - genau richtig, ein
        // Handgriff hat keine Gefaessgroesse. Der Mengendeckel (15 §5.2) wirkt
        // weiterhin, denn er haengt an den Eingaengen.
        ApplyHandcraftPortions(item, def, tier, consumedUnits);

        item.SetSynchDirty();
    }

    /**
     * Die Portionszahl an einem Handwerksergebnis.
     *
     * Bewusst dieselbe Reihenfolge und dieselben Aufrufe wie
     * ChefZ_Applicator.ApplyPortions - und aus demselben Grund
     * ausgeschrieben statt geteilt: die Applicator-Fassung ist auf
     * ChefZ_PlannedOutput und ein Gefaess zugeschnitten, und beides gibt es bei
     * HANDCRAFT nicht.
     *
     * Die RECHNUNG selbst steht an genau einer Stelle, im
     * ChefZ_PortionManager. Hier steht nur, wo sie ankommt.
     */
    private static void ApplyHandcraftPortions(notnull ItemBase item, notnull ChefZ_OutputDef def, ChefZ_Sym tier, float consumedUnits)
    {
        if (!def.IsPortioned())
            return;

        ChefZ_PortionedFood_Base bulk = ChefZ_PortionedFood_Base.Cast(item);
        if (!bulk)
        {
            Note(ChefZ_LogLevel.WARN, "process.portions.noclass." + item.GetType(), "\"" + item.GetType() + "\" ist als Portionsgericht deklariert, seine " + "Skriptklasse erbt aber nicht von ChefZ_PortionedFood_Base. Ohne diese " + "Ableitung gibt es keinen Zaehler und keine Entnahmeaktion (15 §3). Das " + "Ergebnis entsteht als gewoehnliches Item.");
            return;
        }

        ChefZ_PortionManager mgr = ChefZ_PortionManager.Get();

        ChefZ_PortionSpec spec;
        if (!mgr.GetSpecForBulk(ChefZ_SymbolTable.Intern(item.GetType()), spec))
            return;

        ChefZ_CookContext ctx = new ChefZ_CookContext();

        array<string> trace = null;
        int n = mgr.ResolvePortionCount(spec, ctx, consumedUnits, tier, trace);
        bulk.ChefZ_SetPortions(n, n);
    }

    /**
     * Die SCHLECHTESTE Frische unter den gebundenen Eingaengen, oder -1.
     *
     * -1 und nicht 1.0, mit derselben Begruendung wie in
     * ChefZ_Applicator.InputFreshness(): "niemand hat eine Frische" ist etwas
     * anderes als "alle sind frisch". Im ersten Fall behaelt das Ergebnis
     * seine eigene Vorgabe.
     *
     * Ueber boundHandles und nicht ueber den Verbrauchsplan: bei einem reinen
     * Zustandswechsel ist der Plan leer, die Eingaenge aber gebunden.
     */
    private static float BoundFreshness(notnull ChefZ_TransformMatch match, notnull array<ItemBase> entities)
    {
        float worst = -1.0;

        for (int i = 0; i < match.boundHandles.Count(); i++)
        {
            ItemBase item = ChefZ_Applicator.EntityOf(entities, match.boundHandles.Get(i));
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

    //! Die HOECHSTE Temperatur unter den gebundenen Eingaengen, 0 wenn keine.
    //! Die hoechste und nicht der Durchschnitt - dieselbe Regel und dieselbe
    //! Begruendung wie in ChefZ_Applicator.InputTemperature(). Ein Gefaess,
    //! das ersatzweise einspraenge, gibt es bei HANDCRAFT nicht.
    private static float BoundTemperature(notnull ChefZ_TransformMatch match, notnull array<ItemBase> entities)
    {
        float best = 0.0;

        for (int i = 0; i < match.boundHandles.Count(); i++)
        {
            ItemBase item = ChefZ_Applicator.EntityOf(entities, match.boundHandles.Get(i));
            if (!item || !item.CanHaveTemperature())
                continue;

            float t = item.GetTemperature();
            if (t > best)
                best = t;
        }

        return best;
    }

    //! Wie viel Vanilla-Menge der Plan insgesamt abziehen wird - die Grundlage
    //! fuer quantityMode "fromInput" und "ratio". VOR dem Verbrauch gerechnet,
    //! weil danach nichts mehr abzulesen waere.
    private static float BoundConsumedQuantity(notnull ChefZ_TransformMatch match, notnull array<ItemBase> entities)
    {
        float sum = 0.0;

        for (int i = 0; i < match.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = match.consumePlan.Get(i);
            if (!plan)
                continue;

            ItemBase item = ChefZ_Applicator.EntityOf(entities, plan.handle);
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
    // Normalfall: ueber den Applicator (11 E4, 08 §6)
    //==========================================================================

    private static bool RunApplicator(notnull ItemBase station, notnull ChefZ_TransformMatch match, notnull ChefZ_CompiledTransform tr, notnull array<ItemBase> entities, ChefZ_FactSnapshot inputs, int actorId, notnull array<ItemBase> created, out string err)
    {
        ChefZ_MatchResult result = BuildResult(match, tr);

        // Die Qualitaetsstufe wird VOR der Anwendung bestimmt, damit sie schon
        // bei der Wahl der Ergebnisklasse zur Verfuegung steht
        // (ChefZ_OutputDef.variants, 12 §2). Dieselbe Aufteilung wie beim
        // Kochen: gerechnet wird, wo die Eingaben liegen; geschrieben wird,
        // wo das Ergebnis entsteht.
        result.qualityTier = ResolveQuality(tr, match, inputs);

        // Der Kontext, den der Applicator liest: er braucht daraus die
        // Geraetetemperatur als Rueckfall fuer die Ergebnistemperatur. Alles
        // andere - Methode, Fluessigkeit, Portionsdeckel - gilt an einer
        // Station nicht und bleibt leer.
        ChefZ_CookContext ctx = new ChefZ_CookContext();
        ctx.deviceClass       = ChefZ_SymbolTable.Intern(station.GetType());
        ctx.deviceTemperature = station.GetTemperature();
        ctx.actorIdentityId   = actorId;

        string applyErr;
        if (!ChefZ_Applicator.Apply(result, entities, station, ctx, created, applyErr))
        {
            s_CountFailed++;
            err = "Anwendung von " + tr.id + " abgebrochen: " + applyErr
                + " (nichts verbraucht, nichts erzeugt)";
            return false;
        }

        s_CountRun++;

        // 17 §4 / 17 E7: NACH dem Erfolg, nie davor. Der Aufruf steht hinter
        // dem vollzogenen Verbrauch - das ist Regel §10.6 an ihrer Quelle.
        RaiseProcessed(station, tr, match, entities, created, actorId);

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.INFO))
        {
            ChefZ_Log.Info(ChefZ_LogChannel.PROCESS, "Verarbeitet: " + tr.id + " an " + station.GetType() + " -> " + created.Count().ToString() + " Ergebnis(se), verbraucht " + result.consumePlan.Count().ToString() + " Eintraege.");
        }

        return true;
    }

    /**
     * Baut das ChefZ_MatchResult, das der Applicator erwartet.
     *
     * Das synthetische ChefZ_CompiledRecipe traegt AUSSCHLIESSLICH das, was
     * der Applicator liest: outputs, byproducts und policy. Es bekommt
     * ausdruecklich KEINE Slots und KEINE Kontexte - nicht aus Sparsamkeit,
     * sondern damit es niemals wie ein Rezept aussieht. Wer es versehentlich
     * in die Engine eintraegt, bekommt ein Rezept ohne Slots, und das weist
     * die Engine ab (08 §8).
     */
    private static ChefZ_MatchResult BuildResult(notnull ChefZ_TransformMatch match, notnull ChefZ_CompiledTransform tr)
    {
        ChefZ_CompiledRecipe recipe = new ChefZ_CompiledRecipe();
        recipe.recipeSym = tr.transformSym;
        recipe.id        = tr.id;
        recipe.sourceRef = tr.sourceRef;

        int i;
        for (i = 0; i < tr.outputs.Count(); i++)
            recipe.outputs.Insert(tr.outputs.Get(i));
        for (i = 0; i < tr.byproducts.Count(); i++)
            recipe.byproducts.Insert(tr.byproducts.Get(i));

        // Eine Station hat keine Fluessigkeit zu verbrauchen: policy bleibt
        // die Vorgabe, und LiquidToConsume() liefert damit 0.
        for (i = 0; i < tr.requires.Count(); i++)
            recipe.requires.Insert(tr.requires.Get(i));

        ChefZ_MatchResult result = new ChefZ_MatchResult();
        result.matched   = true;
        result.recipe    = recipe;
        result.recipeSym = tr.transformSym;
        result.recipeId  = tr.id;
        result.priority  = tr.priority;

        for (i = 0; i < match.boundHandles.Count(); i++)
            result.boundHandles.Insert(match.boundHandles.Get(i));
        for (i = 0; i < match.consumePlan.Count(); i++)
            result.consumePlan.Insert(match.consumePlan.Get(i));

        result.boundItemCount = result.boundHandles.Count();
        result.itemsInVessel  = result.boundHandles.Count();

        return result;
    }

    /**
     * Die Qualitaetsstufe des Ergebnisses (12 §6, "BEI EINEM TRANSFORM").
     *
     *   1. Raenge der Eingaenge nach transform.qualityRule zusammenfassen
     *   2. um transform.qualityDelta verschieben
     *
     * INVALID, wenn keine Eingabe eine Stufe traegt. Bewusst leer statt halb
     * geraten: ein Ergebnis ohne Stufe ist ehrlich, ein Ergebnis mit erfundener
     * Stufe sieht richtig aus.
     */
    private static ChefZ_Sym ResolveQuality(notnull ChefZ_CompiledTransform tr, notnull ChefZ_TransformMatch match, ChefZ_FactSnapshot inputs)
    {
        if (!inputs)
            return ChefZ_SymbolTable.INVALID;

        ChefZ_QualityManager quality = ChefZ_QualityManager.Get();
        if (!quality.IsReady())
            return ChefZ_SymbolTable.INVALID;

        array<ref ChefZ_ItemFacts> bound = new array<ref ChefZ_ItemFacts>();
        CollectBoundFacts(match, inputs, bound);
        if (bound.Count() == 0)
            return ChefZ_SymbolTable.INVALID;

        ChefZ_Sym tier = quality.CombineRanks(bound, tr.qualityRule, quality.GetDefaultTierSet());
        if (!ChefZ_SymbolTable.IsValid(tier))
            return ChefZ_SymbolTable.INVALID;

        if (tr.qualityDelta != 0.0)
            tier = quality.ShiftRank(tier, tr.qualityDelta);

        return tier;
    }

    //! Die Fakten der GEBUNDENEN Eingaenge, in Bindungsreihenfolge.
    private static void CollectBoundFacts(notnull ChefZ_TransformMatch match, notnull ChefZ_FactSnapshot inputs, notnull array<ref ChefZ_ItemFacts> outFacts)
    {
        outFacts.Clear();
        for (int i = 0; i < match.boundHandles.Count(); i++)
        {
            ChefZ_ItemFacts facts = inputs.FindByHandle(match.boundHandles.Get(i));
            if (facts)
                outFacts.Insert(facts);
        }
    }

    //==========================================================================
    // Sonderfall: reiner Zustandswechsel (11 §2)
    //==========================================================================

    /**
     * Das Eingangsitem bleibt bestehen und wechselt nur seinen ChefZ-Zustand.
     *
     * Dieselbe Vorsicht wie im Applicator, nur ohne Erzeugen und ohne
     * Verbrauchen:
     *
     *   1. REVALIDIEREN  - liegt jedes gebundene Item noch in der Station?
     *   2. ANWENDEN      - Zustand setzen, Frische uebertragen
     *
     * Schritt 1 benutzt ChefZ_Applicator.ValidateHandlesEx() und keinen
     * Nachbau: die Frage ist woertlich dieselbe, und zwei Nachbildungen
     * derselben Pruefung waeren zwei Gelegenheiten, sie unterschiedlich falsch
     * zu stellen.
     *
     * Es kann hier nichts verlorengehen - es wird nichts verbraucht und nichts
     * geloescht. Der schlimmste denkbare Ausgang ist ein Item, dessen Zustand
     * sich nicht geaendert hat.
     */
    private static bool RunStateChange(notnull ItemBase station, notnull ChefZ_TransformMatch match, notnull ChefZ_CompiledTransform tr, notnull array<ItemBase> entities, out string err)
    {
        ChefZ_MatchResult probe = new ChefZ_MatchResult();
        probe.matched = true;
        for (int h = 0; h < match.boundHandles.Count(); h++)
            probe.boundHandles.Insert(match.boundHandles.Get(h));

        string why;
        if (!ChefZ_Applicator.ValidateHandlesEx(probe, entities, station, why))
        {
            s_CountFailed++;
            err = "Revalidierung fehlgeschlagen: " + why + " (nichts veraendert)";
            return false;
        }

        int changed = ApplyStateOutputs(tr, match, entities);

        if (changed == 0)
        {
            s_CountFailed++;
            err = "kein einziger Eingang konnte den Zustand wechseln - keines der gebundenen "
                + "Items fuehrt einen ChefZ-Zustandsblock (06 §4.3). Nichts veraendert.";
            return false;
        }

        s_CountRun++;
        s_CountStateOnly++;

        RaiseProcessed(station, tr, match, entities, null, 0);

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.INFO))
        {
            ChefZ_Log.Info(ChefZ_LogChannel.PROCESS, "Zustandswechsel: " + tr.id + " an " + station.GetType() + " -> " + changed.ToString() + " Item(s), nichts verbraucht.");
        }

        return true;
    }

    /**
     * Die setState-Ausgaenge eines Transforms auf die gebundenen Eingaenge
     * anwenden.
     *
     * Ausgeloest aus RunStateChange(), weil die Handcraft-Bruecke (S15) genau
     * denselben Schritt braucht: dort hat Vanillas Craftsystem die
     * Transaktion in der Hand, aber der Zustandswechsel ist derselbe. Zwei
     * Nachbildungen desselben Schritts waeren zwei Gelegenheiten, ihn
     * unterschiedlich falsch zu stellen.
     *
     * @return Zahl der Items, deren Zustand tatsaechlich gewechselt hat. 0
     *         heisst: keines der gebundenen Items fuehrt einen
     *         ChefZ-Zustandsblock (06 §4.3).
     */
    private static int ApplyStateOutputs(notnull ChefZ_CompiledTransform tr, notnull ChefZ_TransformMatch match, notnull array<ItemBase> entities)
    {
        int changed = 0;

        for (int i = 0; i < tr.outputs.Count(); i++)
        {
            ChefZ_OutputDef def = tr.outputs.Get(i);
            if (!def || def.setState == "")
                continue;

            ChefZ_Sym state = ChefZ_SymbolTable.Lookup(def.setState);
            if (!ChefZ_SymbolTable.IsValid(state))
            {
                // Kein Abbruch: der Compiler haette das gemeldet, wenn der
                // Zustand beim Build gefehlt haette. Hier ist es ein Hinweis
                // auf einen Zustand, der zur Laufzeit verschwunden ist - und
                // das Item bleibt unangetastet.
                Note(ChefZ_LogLevel.WARN, "process.setstate." + def.setState, "setState \"" + def.setState + "\" in " + tr.id + " ist kein bekannter " + "Zustand. Die Eingaenge bleiben unveraendert.");
                continue;
            }

            for (int b = 0; b < match.boundHandles.Count(); b++)
            {
                ItemBase item = ChefZ_Applicator.EntityOf(entities, match.boundHandles.Get(b));
                if (!item)
                    continue;
                if (!ChefZ_ItemStateComponent.IsManaged(item))
                    continue;           // ein Vanilla-Item hat keinen Zustand

                ChefZ_ItemStateComponent.SetState(item, state, false);
                ApplyCarry(item, def);
                item.SetSynchDirty();
                changed++;
            }
        }

        return changed;
    }

    //! freshnessCarry auf einem Item, das BESTEHEN bleibt: die eigene Frische
    //! wird gedaempft, nicht die eines Ergebnisses uebernommen. Ein Wert von
    //! 1.0 laesst sie unangetastet - der haeufigste Fall, und er kostet dann
    //! nichts.
    private static void ApplyCarry(notnull ItemBase item, notnull ChefZ_OutputDef def)
    {
        if (!def.inheritFreshness)
            return;

        float carry = def.freshnessCarry;
        if (carry < 0.0 || carry >= 1.0)
            return;

        ChefZ_ItemStateComponent comp = ChefZ_ItemStateComponent.Of(item);
        if (!comp)
            return;

        ChefZ_ItemStateComponent.SetFreshness01(item, comp.GetFreshness01() * carry);
    }

    //==========================================================================
    // Ereignisse (17 §4, §7, E7)
    //==========================================================================

    /**
     * ChefZ_OnIngredientProcessed plus Fortschrittsmeldung.
     *
     * Musterhaft nach ChefZ_CookingDeviceAdapter.RaiseRecipeCompleted(): erst
     * HasSubscribers, dann die Nutzlast. Ohne Comp-Module kostet die Zeile
     * einen Map-Zugriff und sonst nichts (17 E2).
     *
     * 17 §4 fuehrt das Ereignis als "Prozess abgeschlossen, jede
     * Ausfuehrungsform" - dieselbe Meldung fuer HANDCRAFT, STATION_ACTION und
     * STATION_TIMED. Das ist der Grund, warum die Handcraft-Bruecke (S15)
     * ebenfalls durch DIESE Datei laeuft und sich kein eigenes Ereignis baut.
     */
    private static void RaiseProcessed(ItemBase station, notnull ChefZ_CompiledTransform tr, notnull ChefZ_TransformMatch match, notnull array<ItemBase> entities, array<ItemBase> created, int actorId)
    {
        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        ChefZ_CompiledProcess proc = ChefZ_ProcessingManager.Get().GetProcess(tr.processSym);

        bool wantEvent    = bus.HasSubscribers(ChefZ_EventNames.INGREDIENT_PROCESSED);
        bool wantProgress = ChefZ_ProgressRegistry.HasSinks();
        bool wantCustom   = false;

        if (proc)
        {
            for (int c = 0; c < proc.emitEvents.Count(); c++)
            {
                if (bus.HasSubscribers(proc.emitEvents.Get(c)))
                {
                    wantCustom = true;
                    break;
                }
            }
        }

        if (!wantEvent && !wantProgress && !wantCustom)
            return;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.INGREDIENT_PROCESSED);
        args.identityId   = actorId;
        args.recipeOrTransform = tr.transformSym;

        // station darf null sein: bei HANDCRAFT gibt es keine Station (11 §3).
        // Dann traegt die Nutzlast kein Geraet - und das ist die ehrliche
        // Auskunft, nicht eine erfundene Klasse. Ein Abonnent, der auf
        // deviceClass filtert, sieht ein Handwerksergebnis damit als
        // geraeteloses Ereignis (17 §3.1).
        int low;
        int high;
        if (station)
        {
            args.deviceClass = ChefZ_SymbolTable.Intern(station.GetType());
            if (NetId(station, low, high))
                args.SetDeviceNetId(low, high);
        }

        int i;
        for (i = 0; i < match.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = match.consumePlan.Get(i);
            if (!plan)
                continue;
            ItemBase eaten = ChefZ_Applicator.EntityOf(entities, plan.handle);
            if (eaten)
                args.AddConsumed(ChefZ_SymbolTable.Intern(eaten.GetType()));
        }

        if (created)
        {
            for (i = 0; i < created.Count(); i++)
            {
                ItemBase item = created.Get(i);
                if (!item)
                    continue;
                args.AddProduced(ChefZ_SymbolTable.Intern(item.GetType()));

                // Das erste Ergebnis ist das Subjekt. Ein Beiprodukt ist nicht
                // das, was ein Abonnent meint, wenn er "was ist entstanden"
                // fragt.
                if (i == 0)
                {
                    args.subjectClass = ChefZ_SymbolTable.Intern(item.GetType());
                    if (NetId(item, low, high))
                        args.SetSubjectNetId(low, high);
                }
            }
        }

        if (wantEvent)
            bus.RaiseKeep(args);

        // NACH dem Erfolg, nie davor (17 E7).
        if (wantProgress)
            ChefZ_ProgressRegistry.Report(ChefZ_ProgressKind.PROCESS, args);

        // Die emitEvents des PROZESSES laufen als eigene Ereignisse durch
        // denselben Bus. Der Core wertet sie nie aus, er reicht sie weiter
        // (17 E1).
        if (wantCustom && proc)
        {
            string keep = args.eventId;
            for (int e = 0; e < proc.emitEvents.Count(); e++)
            {
                args.eventId = proc.emitEvents.Get(e);
                bus.RaiseKeep(args);
            }
            args.eventId = keep;
        }

        bus.Release(args);
    }

    /**
     * Netz-ID eines Entities (17 §3.1, E4: Weltbezug NUR ueber Netz-IDs).
     *
     * Ueber zwei lokale Zwischenvariablen und nicht direkt in die
     * out-Parameter: einen out-Parameter als out-Parameter weiterzureichen ist
     * in Enforce nirgends zugesichert. Dieselbe Schreibweise wie
     * ChefZ_CookingDeviceAdapter.VesselId().
     */
    private static bool NetId(ItemBase item, out int low, out int high)
    {
        low  = 0;
        high = 0;
        if (!item)
            return false;

        int lo = 0;
        int hi = 0;
        item.GetNetworkID(lo, hi);
        low  = lo;
        high = hi;
        return (lo | hi) != 0;
    }

    //==========================================================================
    // Kleinkram
    //==========================================================================

    private static bool Failure(string why, out string err)
    {
        err = why;
        s_CountFailed++;
        Note(ChefZ_LogLevel.ERR, "process.run.fail", "Transform nicht ausgefuehrt: " + why);
        return false;
    }

    private static void Note(int level, string key, string msg)
    {
        if (s_QuietForTest)
            return;
        ChefZ_Log.Once(level, ChefZ_LogChannel.PROCESS, key, msg);
    }

    static void SetQuietForTest(bool quiet)
    {
        s_QuietForTest = quiet;
    }

    static void ResetCounters()
    {
        s_CountRun       = 0;
        s_CountFailed    = 0;
        s_CountStateOnly = 0;
    }

    static int GetRunCount()       { return s_CountRun; }
    static int GetFailedCount()    { return s_CountFailed; }
    static int GetStateOnlyCount() { return s_CountStateOnly; }

    static void DumpStats(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("ChefZ Process Runner  ausgefuehrt=" + s_CountRun.ToString() + "  davon nur Zustand=" + s_CountStateOnly.ToString() + "  abgebrochen=" + s_CountFailed.ToString());
    }
}
