//==============================================================================
// ChefZ_PortionManager - die beiden Deckel und die Portionsregistry
//
// Entwurf: 15 §3 (Schnittstelle woertlich), 15 §4 (ERZEUGUNG und ENTNAHME),
// 15 §5 (die zwei Deckel), 15 §6 (Zustandstabelle), 15 §7 (Fehlerverhalten
// Zeile fuer Zeile), 15 E3 (doppelte Deckelung), 15 E4 (Portionen erben
// unveraendert), 15 E7 (Einzelgerichte sind Portionsgerichte mit portions=1),
// 12 §2 (yieldMultiplier und portionBonus), 19 S16.
//
// ---------------------------------------------------------------------------
// Die eine Regel dieser Datei
// ---------------------------------------------------------------------------
// 15 §5.2: "Ohne diesen Deckel ergaebe eine Minimalfuellung im Kessel zwoelf
// Portionen - ein glasklarer Nahrungsexploit."
//
//     n = spec.portions
//     n = min(n, ctx.portionCapacity)                    <- Geraetedeckel
//     n = min(n, floor(verbrauchteEinheiten / je))       <- Mengendeckel
//     n = floor(n * yieldMultiplier) + portionBonus      <- Qualitaet
//     n = clamp(n, 1, 31)                                <- Sync-Grenze
//
// Die Reihenfolge steht so in 15 §4 und ist nicht verhandelbar. Wer die
// Qualitaet VOR die Deckel zoege, bekaeme einen Kessel, in dem eine einzige
// hervorragende Zutat zwoelf Portionen ergibt - genau die Schleife, gegen die
// beide Deckel stehen.
//
// ---------------------------------------------------------------------------
// Wo die Registry herkommt
// ---------------------------------------------------------------------------
// 15 §6: "Portions-Registry | Laufzeit, aus den Rezept-Outputs abgeleitet |
// nicht persistiert | nicht gesynct". Es gibt also KEINE eigene Recordart und
// keine eigene JSON-Datei fuer Portionen: eine Spec entsteht aus den
// Portionsfeldern eines ChefZ_OutputDef, und der Schluessel ist die
// Ergebnisklasse.
//
// Transforms (11) tragen dieselben ChefZ_OutputDef und werden deshalb
// mitgelesen. Ohne sie waere ein an einer Station entstandenes Bulk-Gericht
// ein Item mit Zaehler, aus dem niemand etwas entnehmen kann.
//
// ---------------------------------------------------------------------------
// Was hier NICHT steht: der Behaelter
// ---------------------------------------------------------------------------
// Die Entnahme verlangt laut 15 §4 einen Behaelter, und das Behaeltersystem
// ist S17. Solange es fehlt, gilt 15 §7 Zeile 4 woertlich:
//
//     "containerCategory unbekannt -> Behaelterbedingung ENTFAELLT, WARN beim
//      Laden. Lieber entnehmbar ohne Schuessel als gar nicht entnehmbar."
//
// Ohne Behaelterregistry ist JEDE Kategorie unbekannt, also entfaellt die
// Bedingung - und zwar mit genau einer Meldung je Spec, nicht je Entnahme.
// Der Schalter dafuer ist SetContainerSystemReady(): S17 legt ihn um, wenn
// seine Registry steht. Diese eine Zeile ist die ganze Nahtstelle; sie setzt
// nichts ueber die Bauform von S17 voraus.
//
// Wichtig ist, was dabei NICHT passiert: es geht nichts verloren. Der
// schlimmste Fall ist eine Portion, die keine Schuessel kostet.
//
// SEIT S17 gilt das nur noch fuer den Fall, dass es UEBERHAUPT KEINEN
// deklarierten Behaelter gibt (ChefZ_ConfigManager legt den Schalter genau
// danach um). Sobald das System steht, wird eine unbekannte Kategorie
// abgewiesen statt uebergangen - die Begruendung dieser Fallunterscheidung
// steht ausgeschrieben in CanTakePortion().
//
// Gesucht wird weiterhin NICHT hier. Die Liste der verfuegbaren
// Behaelterklassen fuellt ChefZ_ContainerService.FillRequest() in 4_World;
// dieser Manager waehlt nur aus (BuildPortionPlan) und fasst nach wie vor
// kein Item an.
//
// Server und Client. Der Manager RECHNET nur; er fasst kein Item an und kennt
// kein Inventar - deshalb liegt er in 3_Game und nicht in 4_World.
//
// KEIN CONTENT: kein Gericht, keine Schuessel, keine Kategorie, kein Geraet.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_PortionManager
{
    private static ref ChefZ_PortionManager s_Instance;

    //! bulkClassSym -> Spec. Nach dem Build unveraenderlich.
    private ref map<int, ref ChefZ_PortionSpec> m_BySym;

    //! Reihenfolge der Aufnahme, fuer eine stabile Diagnoseausgabe.
    private ref array<ChefZ_Sym> m_Order;

    private bool  m_Ready;
    private bool  m_NotReadyLogged;
    private bool  m_QuietForTest;

    /**
     * Steht das Behaeltersystem (S17)?
     *
     * false ist der Normalzustand dieses Bauzustands und ausdruecklich KEIN
     * Fehler - siehe Dateikopf. Statisch, weil S17 den Schalter umlegt, bevor
     * es diesen Manager unbedingt geben muss.
     */
    private static bool s_ContainerSystemReady;

    private float m_DefaultTakeSec;

    //--- Zaehler fuer "chefz stats" (18 §2) ----------------------------------
    //
    // Es gibt hier BEWUSST keinen "abgewiesen"-Zaehler: eine Spec wird an
    // dieser Stelle nie abgewiesen. Was unbrauchbar ist, faellt schon beim
    // Rezept- bzw. Transformbau heraus (ChefZ_PortionOutputAudit), und ein
    // Zaehler, der immer 0 bleibt, ist eine Zahl, die luegt.
    private int m_Collisions;
    private int m_CountResolved;
    private int m_CountPlanned;

    //--------------------------------------------------------------------------

    void ChefZ_PortionManager()
    {
        m_BySym = new map<int, ref ChefZ_PortionSpec>();
        m_Order = new array<ChefZ_Sym>();
        ResetState();
    }

    static ChefZ_PortionManager Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_PortionManager();
        return s_Instance;
    }

    private void ResetState()
    {
        m_BySym.Clear();
        m_Order.Clear();
        m_Ready          = false;
        m_NotReadyLogged = false;
        m_Collisions     = 0;
        m_CountResolved  = 0;
        m_CountPlanned   = 0;
        m_DefaultTakeSec = ChefZ_PortionLimits.DEFAULT_TAKE_SEC;
    }

    //==========================================================================
    // Aufbau (15 §6, BOOT)
    //==========================================================================

    /**
     * Baut die Portionsregistry. Einmal beim Boot, danach unveraenderlich.
     *
     * ZWEI ABWEICHUNGEN von der Signatur in 15 §3 ("void Build(ChefZ_LoadReport
     * report)"), beide aus demselben Grund wie beim ChefZ_QualityManager: die
     * Quellen kommen HEREIN, statt hier ueber Singletons geholt zu werden.
     *
     *   - Ohne die Engine gaebe es nichts zu lesen: 15 §6 leitet die Registry
     *     ausdruecklich "aus den Rezept-Outputs" ab.
     *   - Der Selbsttest kann damit auf EIGENEN Instanzen laufen und beruehrt
     *     den Bestand des Servers nicht.
     *
     * Alle Parameter duerfen null sein. Der Aufruf ist beim Boot UNBEDINGT:
     * auch ohne ein einziges Portionsgericht soll der Manager "bereit und
     * leer" sein (15 §7, erste Zeile) - sonst antwortete er auf jede Abfrage
     * mit einem Fehler ueber einen fehlenden Aufbau, den es nie geben wird.
     */
    void Build(ChefZ_RecipeEngine engine, ChefZ_ProcessingManager processing, ChefZ_LoadReport report, ChefZ_CoreSettingsDef settings = null)
    {
        ResetState();

        if (settings && settings.defaultTakePortionSec > 0.0)
            m_DefaultTakeSec = settings.defaultTakePortionSec;

        int fromRecipes   = CollectFromRecipes(engine, report);
        int fromTransforms = CollectFromTransforms(processing, report);

        m_Ready = true;

        if (report)
        {
            if (m_Order.Count() == 0)
            {
                // 15 §7, erste Zeile: kein Abbruch, keine Warnung. In einem
                // Core ohne Content ist das der Normalzustand.
                report.AddInfo("Keine Portionsgerichte deklariert - jedes Kochergebnis " + "entsteht als gewoehnliches Item. Vanilla-Kochen ist davon " + "unberuehrt.");
            }
            else
            {
                report.AddInfo("Portionsgerichte: " + m_Order.Count().ToString() + " (aus Rezepten " + fromRecipes.ToString() + ", aus Transforms " + fromTransforms.ToString() + ")" + ", Namenskollisionen " + m_Collisions.ToString() + ".");
            }
        }

        LogIfDebug();
    }

    private int CollectFromRecipes(ChefZ_RecipeEngine engine, ChefZ_LoadReport report)
    {
        if (!engine)
            return 0;

        int taken = 0;
        int count = engine.GetRecipeCount();

        for (int i = 0; i < count; i++)
        {
            ChefZ_CompiledRecipe rec = engine.GetRecipeAt(i);
            if (!rec)
                continue;

            taken = taken + TakeList(rec.outputs, rec.id, report);
            taken = taken + TakeList(rec.byproducts, rec.id, report);
        }

        return taken;
    }

    private int CollectFromTransforms(ChefZ_ProcessingManager processing, ChefZ_LoadReport report)
    {
        if (!processing)
            return 0;

        int taken = 0;
        int count = processing.GetTransformCount();

        for (int i = 0; i < count; i++)
        {
            ChefZ_CompiledTransform tr = processing.GetTransformAt(i);
            if (!tr)
                continue;

            taken = taken + TakeList(tr.outputs, tr.id, report);
            taken = taken + TakeList(tr.byproducts, tr.id, report);
        }

        return taken;
    }

    private int TakeList(array<ref ChefZ_OutputDef> list, string owner, ChefZ_LoadReport report)
    {
        if (!list)
            return 0;

        int taken = 0;
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_OutputDef def = list.Get(i);
            if (!def || !def.IsPortioned())
                continue;
            if (Register(def, owner, report))
                taken++;
        }

        return taken;
    }

    /**
     * Nimmt eine Spec auf.
     *
     * Kollision: dieselbe Ergebnisklasse aus zwei Rezepten. Der ERSTE gewinnt,
     * und der erste ist deterministisch - die Registry gibt ihre Schluessel
     * nach ID sortiert aus (03 §4), also sehen Client und Server dieselbe
     * Reihenfolge.
     *
     * Zwei GLEICHE Specs sind dabei kein Streitfall und bekommen keine
     * Warnung: ein Gericht, das aus zwei Rezepten auf dieselbe Weise entsteht,
     * ist ein voellig normaler Content-Aufbau.
     */
    private bool Register(notnull ChefZ_OutputDef def, string owner, ChefZ_LoadReport report)
    {
        ChefZ_PortionSpec spec = new ChefZ_PortionSpec();
        spec.FillFrom(def, def.cls, owner, m_DefaultTakeSec);

        // 15 §7: "portions > 31 -> geklemmt, WARN". Verhindert zugleich, dass
        // ein Zahlendreher einen 500-Portionen-Topf erzeugt.
        if (spec.portions > ChefZ_PortionLimits.MAX)
        {
            Note(report, owner, "\"" + spec.bulkClass + "\" nennt " + spec.portions.ToString() + " Portionen. Mehr als " + ChefZ_PortionLimits.MAX.ToString() + " lassen sich nicht zum Client synchronisieren - geklemmt. Der Zaehler ist " + "eine 5-Bit-Variable am Item (03 §4).");
            spec.portions = ChefZ_PortionLimits.MAX;
        }

        // 15 §5.2, der Exploitsatz. Kein Fehler und keine Abweisung: es gibt
        // Gerichte, deren Portionszahl bewusst nicht an der Zutatenmenge
        // haengt. Aber es steht im Ladebericht, denn es ist die einzige Sperre
        // gegen den offensichtlichsten Nahrungsexploit des ganzen Mods.
        if (spec.portions > 1 && !spec.HasAmountCap())
        {
            Note(report, owner, "\"" + spec.bulkClass + "\" liefert bis zu " + spec.portions.ToString() + " Portionen, nennt aber kein " + "\"amountPerPortion\". Damit deckelt allein das Geraet: eine " + "Minimalfuellung im groessten Topf ergibt die volle Portionszahl " + "(15 §5.2). Beabsichtigt?");
        }

        ChefZ_PortionSpec existing;
        if (m_BySym.Find(spec.bulkClassSym, existing))
        {
            if (!SameSpec(existing, spec))
            {
                m_Collisions++;
                Note(report, owner, "\"" + spec.bulkClass + "\" ist bereits mit anderen " + "Portionsdaten deklariert (aus " + existing.sourceRef + "). Es gilt die " + "zuerst gelesene Fassung; diese hier wird ignoriert. Ein Gericht kann " + "nur EINE Portionsregel haben - der Zaehler steht am Item, nicht am " + "Rezept (15 §6).");
            }
            return false;
        }

        m_BySym.Set(spec.bulkClassSym, spec);
        m_Order.Insert(spec.bulkClassSym);
        return true;
    }

    //! Vergleicht die spielrelevanten Felder. sourceRef bleibt aussen vor -
    //! zwei Rezepte, die dasselbe sagen, sind kein Streitfall.
    private bool SameSpec(notnull ChefZ_PortionSpec a, notnull ChefZ_PortionSpec b)
    {
        if (a.portions           != b.portions)           return false;
        if (a.portionClass       != b.portionClass)       return false;
        if (a.portionQuantity    != b.portionQuantity)    return false;
        if (a.amountPerPortion   != b.amountPerPortion)   return false;
        if (a.containerCategory  != b.containerCategory)  return false;
        if (a.consumesContainer  != b.consumesContainer)  return false;
        if (a.emptyOnLastPortion != b.emptyOnLastPortion) return false;
        if (a.scaleWithDevice    != b.scaleWithDevice)    return false;
        if (a.inheritQuality     != b.inheritQuality)     return false;
        if (a.inheritState       != b.inheritState)       return false;
        if (a.inheritFreshness   != b.inheritFreshness)   return false;
        if (a.returnContainer    != b.returnContainer)    return false;
        return true;
    }

    //==========================================================================
    // Die Nahtstelle zu S17 (siehe Dateikopf)
    //==========================================================================

    //! S17 legt diesen Schalter um, sobald ChefZ_ContainerRegistry steht.
    //! Bis dahin entfaellt jede Behaelterbedingung mit einer Meldung je Spec.
    static void SetContainerSystemReady(bool ready)
    {
        s_ContainerSystemReady = ready;
    }

    static bool IsContainerSystemReady()
    {
        return s_ContainerSystemReady;
    }

    //==========================================================================
    // 15 §4, ERZEUGUNG - die doppelte Deckelung
    //==========================================================================

    /**
     * Wie viele Portionen entstehen (15 §4, 15 §5).
     *
     * @param consumedRequiredUnits  die Rezepteinheiten, die aus den
     *        PFLICHT-Slots tatsaechlich verbraucht wurden. Nicht die
     *        Vanilla-Quantity und nicht die Itemzahl - siehe
     *        ConsumedRequiredUnits().
     * @param trace  darf null sein. Ist sie gesetzt, steht danach jeder
     *        Rechenschritt einzeln darin (18 §3: der Trace wird von Menschen
     *        gelesen).
     *
     * @return immer >= 1. Ein Portionsgericht mit null Portionen waere ein
     *         Gericht, das man nie essen kann (15 §4, "clamp(n, 1, 31)").
     */
    int ResolvePortionCount(notnull ChefZ_PortionSpec spec, notnull ChefZ_CookContext ctx, float consumedRequiredUnits, ChefZ_Sym qualityTier, out array<string> trace)
    {
        m_CountResolved++;

        // Ueber eine lokale Zwischenvariable: einen out-Parameter als
        // out-Parameter weiterzureichen ist in Enforce nirgends zugesichert
        // (dieselbe Vorsicht wie in ChefZ_TextList.SymbolsOf). null bleibt
        // null - "keine Ablaufverfolgung" ist der Normalfall im Kochtakt.
        array<string> steps = trace;

        int n = spec.portions;
        if (n < ChefZ_PortionLimits.MIN)
            n = ChefZ_PortionLimits.MIN;

        Step(steps, "Rezept nennt " + n.ToString() + " Portionen");

        //--- Deckel 1: das Geraet (15 §5.1) -----------------------------------
        //
        // portionCapacity = 0 heisst "das Geraet sagt nichts dazu"
        // (ChefZ_DeviceDef.portionCapacity, Vorgabe 0) - und dann deckelt es
        // auch nichts. Die Alternative waere, jedes Gericht in jedem
        // undeklarierten Geraet auf eine Portion zu druecken; das saehe aus
        // wie ein kaputtes Rezept, waere aber eine fehlende Geraetezeile.
        if (spec.scaleWithDevice && ctx.portionCapacity > 0 && ctx.portionCapacity < n)
        {
            n = ctx.portionCapacity;
            Step(steps, "Geraetedeckel " + ChefZ_SymbolTable.NameOrMark(ctx.deviceClass) + " -> " + n.ToString());
        }

        //--- Deckel 2: die Zutatenmenge (15 §5.2) -----------------------------
        //
        // DER Exploitsperrpunkt. floor() und nicht round(): eine halbe Portion
        // gibt es nicht, und Aufrunden waere genau der Weg, aus einer
        // Minimalfuellung eine volle Ausbeute zu machen.
        if (spec.HasAmountCap())
        {
            int byAmount = Math.Floor(consumedRequiredUnits / spec.amountPerPortion);
            if (byAmount < n)
            {
                n = byAmount;
                Step(steps, "Mengendeckel " + consumedRequiredUnits.ToString() + " Einheiten / " + spec.amountPerPortion.ToString() + " je Portion -> " + n.ToString());
            }
        }

        //--- Qualitaet (12 §2) ------------------------------------------------
        //
        // NACH beiden Deckeln, so wie 15 §4 es zeichnet. Davor gerechnet
        // koennte eine gute Stufe den Mengendeckel ueberspringen.
        //
        // Der Manager wird ueber den Singleton geholt und nicht hereingereicht:
        // die Stufenwirkungen gehoeren S10, sie stehen zur Laufzeit fest, und
        // GetOrFallback() antwortet auch ohne eine einzige geladene Stufe
        // neutral (12 §8).
        if (ChefZ_SymbolTable.IsValid(qualityTier))
        {
            ChefZ_QualityManager quality = ChefZ_QualityManager.Get();
            float yield = quality.GetYieldMultiplier(qualityTier);
            int   bonus = quality.GetPortionBonus(qualityTier);

            if (yield != 1.0 || bonus != 0)
            {
                // Ueber eine float-Zwischenvariable: eine int-Multiplikation
                // mit einem float ist in Enforce an keiner Stelle zugesichert.
                float scaled = n;
                scaled = scaled * yield;

                // floor() und nicht round(): 15 E4 verbietet Rundungsgewinne
                // ausdruecklich ("achtmal portionieren, achtmal aufrunden").
                // Math.Floor liefert in Enforce einen float; die Zuweisung an
                // ein int ist die Abrundung, um die es geht.
                int floored = Math.Floor(scaled);
                n = floored + bonus;

                Step(steps, "Stufe " + ChefZ_SymbolTable.Name(qualityTier) + ": x" + yield.ToString() + " +" + bonus.ToString() + " -> " + n.ToString());
            }
        }

        //--- Sync-Grenze (15 §4) ----------------------------------------------
        int clamped = ChefZ_PortionLimits.Clamp(n);
        if (clamped != n)
            Step(steps, "geklemmt auf " + clamped.ToString());

        return clamped;
    }

    /**
     * Die Rezepteinheiten aus den PFLICHT-Slots eines gebundenen Ergebnisses.
     *
     * Warum nur Pflichtslots: der Mengendeckel soll messen, wie viel von dem
     * verbraucht wurde, WAS DAS GERICHT AUSMACHT. Eine optionale Prise Salz
     * darf keine zusaetzliche Portion Eintopf ergeben - sonst waere der
     * Mengendeckel ueber die billigste optionale Zutat auszuhebeln, und die
     * Sperre aus 15 §5.2 waere keine.
     *
     * Warum unitsDelta und nicht quantityDelta: Einheiten sind die Zahl, in
     * der ein Rezept seine Mengen schreibt (07 §2.3). Vanilla-Quantity ist je
     * nach Klasse Gramm, Milliliter oder Stueck - eine Division durch
     * "amountPerPortion" waere darin sinnlos.
     *
     * Statisch und ohne Instanzzustand: der ChefZ_Applicator ruft sie, und der
     * soll dafuer keinen gebauten Manager brauchen.
     */
    static float ConsumedRequiredUnits(notnull ChefZ_MatchResult result)
    {
        if (!result.recipe)
            return 0.0;
        return ConsumedRequiredUnitsOf(result.consumePlan, result.recipe.slots);
    }

    /**
     * Dieselbe Rechnung fuer einen Verarbeitungsschritt (11).
     *
     * Ein Transform traegt seine Eingaenge als ChefZ_CompiledSlot und seinen
     * Verbrauch als ChefZ_ConsumePlan - also genau dieselben beiden Listen wie
     * ein Rezept, nur unter anderem Namen. Deshalb EINE Rechnung mit zwei
     * duennen Eingaengen und nicht zwei Rechnungen: der Mengendeckel ist die
     * Exploitsperre aus 15 §5.2, und die darf es nur einmal geben.
     */
    static float ConsumedRequiredUnitsOf(array<ref ChefZ_ConsumePlan> plans, array<ref ChefZ_CompiledSlot> slots)
    {
        if (!plans || !slots)
            return 0.0;

        float sum = 0.0;

        for (int i = 0; i < plans.Count(); i++)
        {
            ChefZ_ConsumePlan plan = plans.Get(i);
            if (!plan)
                continue;

            if (plan.slotIndex < 0 || plan.slotIndex >= slots.Count())
                continue;

            ChefZ_CompiledSlot slot = slots.Get(plan.slotIndex);
            if (!slot || !ChefZ_CompiledRecipe.IsRequiredSlot(slot))
                continue;

            // Ein Slot, der ein GANZES Item verbraucht, traegt keinen
            // Einheitenbetrag - er traegt genau seine Mindestmenge, und die
            // steht im Slot. Ohne diesen Zweig zaehlte ein Rezept aus lauter
            // Ganzitem-Slots null Einheiten, und der Mengendeckel klemmte
            // jedes solche Gericht auf eine Portion.
            if (plan.unitsDelta > 0.0)
            {
                sum = sum + plan.unitsDelta;
                continue;
            }

            if (plan.destroyWhole)
                sum = sum + RequiredUnitsOf(slot);
        }

        return sum;
    }

    //! Was ein Slot als Einheiten verlangt. 1.0, wenn er gar keine Menge
    //! nennt - ein gebundenes Item ist dann "eine Einheit", und das ist die
    //! einzige Lesart, die fuer "ein Stueck Fleisch" richtig ist.
    private static float RequiredUnitsOf(notnull ChefZ_CompiledSlot slot)
    {
        float units = slot.RequiredUnits();
        if (units <= 0.0)
            return 1.0;
        return units;
    }

    private void Step(array<string> trace, string line)
    {
        if (trace)
            trace.Insert(line);
    }

    //==========================================================================
    // 15 §4, ENTNAHME
    //==========================================================================

    bool IsBulkClass(ChefZ_Sym classSym)
    {
        if (!m_Ready)
            return false;
        return m_BySym.Contains(classSym);
    }

    //! true und die Spec, wenn diese Klasse ein Portionsgericht ist.
    bool GetSpecForBulk(ChefZ_Sym bulkClass, out ChefZ_PortionSpec spec)
    {
        spec = null;
        if (!GuardReady("GetSpecForBulk"))
            return false;

        ChefZ_PortionSpec found;
        if (!m_BySym.Find(bulkClass, found))
            return false;

        spec = found;
        return true;
    }

    /**
     * Die i-te Spec in Aufnahmereihenfolge, oder null.
     *
     * Fuer das Behaelteraudit aus 16 §7 (S17): es laeuft beim Boot einmal
     * ueber alle Specs und meldet jede unbekannte Behaelterkategorie mit der
     * Rezept-ID. Ohne diesen Zugang muesste das Audit entweder in diesem
     * Manager stehen - der von Behaeltern nichts wissen soll - oder die
     * Registry muesste die Specs kopiert bekommen.
     *
     * Rein lesend. Die Reihenfolge ist die des Ladeberichts und damit auf
     * jedem Server dieselbe.
     */
    ChefZ_PortionSpec GetSpecAt(int index)
    {
        if (!m_Ready)
            return null;
        if (index < 0 || index >= m_Order.Count())
            return null;
        return m_BySym.Get(m_Order.Get(index));
    }

    /**
     * Darf jetzt eine Portion entnommen werden?
     *
     * Diese Antwort ist eine VORPRUEFUNG und keine Erlaubnis. Verbindlich ist
     * ausschliesslich die zweite Pruefung in
     * ChefZ_PortionedFood_Base.ChefZ_TakePortion(), die serverseitig laeuft
     * und erneut fragt (15 E6).
     *
     * failReason ist bei false NIE leer: der Aktionstext zeigt ihn dem
     * Spieler, und "die Aktion erscheint nicht" ohne Grund ist die
     * frustrierendste Antwort, die ein Kochmod geben kann (15 §7).
     */
    bool CanTakePortion(notnull ChefZ_PortionRequest req, out string failReason)
    {
        failReason = "";

        if (!m_Ready)
        {
            failReason = "das Portionssystem ist nicht gebaut";
            return false;
        }

        ChefZ_PortionSpec spec;
        if (!GetSpecForBulk(req.sourceClass, spec))
        {
            failReason = ChefZ_SymbolTable.NameOrMark(req.sourceClass) + " ist kein Portionsgericht";
            return false;
        }

        if (!spec.IsPortioned())
        {
            failReason = spec.bulkClass + " hat keine Portionsklasse";
            return false;
        }

        // 15 §7: "portionsLeft <= 0 bei existierendem Item -> Action erscheint
        // nicht; das Item bleibt als normales Item verzehrbar. KEIN Loeschen
        // von Spielerbesitz."
        if (req.portionsLeft <= 0)
        {
            failReason = "keine Portion mehr uebrig";
            return false;
        }

        if (!spec.RequiresContainer())
            return true;

        if (!s_ContainerSystemReady)
        {
            // 15 §7, Zeile 4. EINMAL je Spec, nicht je Entnahme - sonst
            // Logflut, sobald jemand einen Kessel leert.
            Once(ChefZ_LogLevel.WARN, "portion.container.missing." + spec.bulkClass, "\"" + spec.bulkClass + "\" verlangt fuer die Entnahme einen Behaelter der " + "Kategorie \"" + spec.containerCategory + "\". Das Behaeltersystem ist " + "nicht geladen, deshalb entfaellt die Bedingung (15 §7): die Portion " + "entsteht ohne Behaelter. Es geht dabei nichts verloren.");
            return true;
        }

        // 16 §7, Zeile 2 - seit S17. Sobald es UEBERHAUPT Behaelter gibt, ist
        // eine unbekannte Kategorie eine Sackgasse: kein Behaelter der Welt
        // gehoert ihr an, also findet die Suche nie etwas.
        //
        // Das ist die Gegenrichtung zu 15 §7 Zeile 4 ("Behaelterbedingung
        // ENTFAELLT"), und der Widerspruch ist bewusst so aufgeloest: 15 §7 ist
        // fuer eine Welt OHNE Behaeltersystem geschrieben - dort ist JEDE
        // Kategorie unbekannt, und die Bedingung fallen zu lassen ist die
        // einzige spielbare Antwort. Sobald das System steht, waere dieselbe
        // Grosszuegigkeit ein Freibrief: ein Tippfehler in einer Kategorie
        // ergaebe Portionen ohne Behaelter, und das faellt niemandem auf.
        //
        // Der Grund steht im Ladebericht, EINMAL, mit Rezept-ID
        // (ChefZ_ContainerRegistry.AuditPortionSpecs). Hier gibt es deshalb
        // keine Meldung: diese Methode laeuft bei jedem Zielwechsel des
        // Fadenkreuzes.
        if (!ChefZ_ContainerRegistry.Get().CategoryExists(spec.containerCategorySym))
        {
            failReason = "die Behaelterkategorie \"" + spec.containerCategory + "\" ist auf diesem Server nicht deklariert";
            return false;
        }

        if (!req.HasContainers())
        {
            failReason = "kein passender Behaelter (" + spec.containerCategory + ") im Zugriff";
            return false;
        }

        return true;
    }

    /**
     * Der vollstaendige Plan einer Entnahme (15 §3).
     *
     * Er VERAENDERT NICHTS - er beschreibt. Ausgefuehrt wird er in 4_World,
     * und dort in genau der Reihenfolge aus 15 §4: erzeugen, uebertragen,
     * Behaelter verbrauchen, DANN dekrementieren.
     *
     * 15 E4: Zustand, Qualitaet und Frische werden UNVERAENDERT uebernommen.
     * Es gibt in dieser Methode bewusst keine einzige Rechnung darauf -
     * "Portionieren ist keine Verarbeitung".
     */
    bool BuildPortionPlan(notnull ChefZ_PortionRequest req, out ChefZ_PortionPlan plan)
    {
        ChefZ_PortionPlan result = plan;
        if (!result)
            result = new ChefZ_PortionPlan();
        result.Reset();
        plan = result;

        string why;
        if (!CanTakePortion(req, why))
            return false;

        ChefZ_PortionSpec spec;
        if (!GetSpecForBulk(req.sourceClass, spec))
            return false;

        result.portionClass    = spec.portionClass;
        result.quantityToApply = spec.portionQuantity;

        if (spec.inheritState)
            result.stateToApply = req.sourceState;
        if (spec.inheritQuality)
            result.qualityToApply = req.sourceQuality;
        if (spec.inheritFreshness)
            result.freshnessToApply = req.sourceFreshness;

        //--- Behaelter (16) ---------------------------------------------------
        //
        // Der Manager WAEHLT nur; gesucht hat der Aufrufer in 4_World, und der
        // hat die Liste bereits geordnet (16 E5: Gesundheit, dann
        // Klassenname). Deshalb der erste Eintrag und kein zweites
        // Auswahlkriterium: zwei Reihenfolgen ergaeben zwei Antworten.
        if (spec.RequiresContainer() && spec.consumesContainer && s_ContainerSystemReady && req.HasContainers())
        {
            // Seit S17 ueber ChefZ_ContainerRegistry.ChooseContainer() statt
            // ueber Get(0): die Auswahlregel gehoert dem Behaeltersystem, und
            // dort ist sie ohne Welt pruefbar. Sie tut dasselbe - den ersten
            // Eintrag nehmen -, prueft aber zusaetzlich die
            // Kategoriezugehoerigkeit. Ohne diese Probe koennte ein Aufrufer
            // eine Liste uebergeben, die zu einer anderen Kategorie gehoert,
            // und der Plan verbrauchte den falschen Behaelter.
            ChefZ_Sym chosen;
            if (ChefZ_ContainerRegistry.Get().ChooseContainer(spec.containerCategorySym, req.availableContainerClasses, chosen))
            {
                result.containerToConsume = chosen;
            }
        }

        // 16 §4: "AUTO" gibt den Leerbehaelter GENAU DES benutzten Behaelters
        // zurueck. Diese Aufloesung findet hier NICHT statt und kann es nicht:
        // welcher Behaelter tatsaechlich benutzt wurde, entscheidet sich erst
        // beim Verbrauch in 4_World. Dort loest
        // ChefZ_PortionedFood_Base.ChefZ_TakePortion() sie ueber
        // ChefZ_ContainerRegistry.ResolveReturnClass() auf.
        //
        // Ein Rezept mit fester Klasse dagegen ist hier vollstaendig
        // beantwortbar - und wird es, damit der haeufige Fall ohne Umweg
        // auskommt.
        if (spec.returnContainer != "" && spec.returnContainer != ChefZ_ContainerDef.AUTO)
            result.returnContainerClass = ChefZ_SymbolTable.Intern(spec.returnContainer);

        //--- Was aus der Quelle wird (15 §2) ----------------------------------
        result.portionsLeftAfter  = req.portionsLeft - 1;
        result.sourceBecomesEmpty = result.portionsLeftAfter <= 0;
        if (result.sourceBecomesEmpty)
            result.emptyClass = spec.emptyOnLastPortion;

        m_CountPlanned++;
        return true;
    }

    //==========================================================================
    // Auskuenfte
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    int GetSpecCount()
    {
        return m_Order.Count();
    }

    float GetDefaultTakeSeconds()
    {
        return m_DefaultTakeSec;
    }

    int GetCollisionCount() { return m_Collisions; }
    int GetResolvedCount()  { return m_CountResolved; }
    int GetPlannedCount()   { return m_CountPlanned; }

    void DumpSpecs(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Portionsgerichte: " + m_Order.Count().ToString() + "  behaeltersystem=" + s_ContainerSystemReady.ToString() + "  vorgabedauer=" + m_DefaultTakeSec.ToString() + "s");

        for (int i = 0; i < m_Order.Count(); i++)
        {
            ChefZ_PortionSpec spec = m_BySym.Get(m_Order.Get(i));
            if (spec)
                outLines.Insert("  " + spec.ToDebugString());
        }
    }

    //==========================================================================
    // Innereien
    //==========================================================================

    private bool GuardReady(string who)
    {
        if (m_Ready)
            return true;

        if (!m_NotReadyLogged && !m_QuietForTest)
        {
            m_NotReadyLogged = true;
            ChefZ_Log.Error(ChefZ_LogChannel.PORTION, "ChefZ_PortionManager." + who + " wurde vor dem Build gerufen. Die Antwort " + "lautet \"kein Portionsgericht\"; Gerichte entstehen dann als gewoehnliche " + "Items. Diese Meldung erscheint genau einmal.");
        }

        return false;
    }

    private void Note(ChefZ_LoadReport report, string owner, string msg)
    {
        if (report)
            report.AddWarn(owner, owner, msg);
        else
            Once(ChefZ_LogLevel.WARN, "portion.build." + owner, msg);
    }

    private void Once(int level, string key, string msg)
    {
        if (m_QuietForTest)
            return;
        ChefZ_Log.Once(level, ChefZ_LogChannel.PORTION, key, msg);
    }

    private void LogIfDebug()
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.PORTION, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        DumpSpecs(lines);
        for (int i = 0; i < lines.Count(); i++)
            ChefZ_Log.Debug(ChefZ_LogChannel.PORTION, lines.Get(i));
    }

    //! Nur fuer den Selbsttest: unterdrueckt die Meldungen dieser Klasse.
    //! Dieselbe Loesung und derselbe Grund wie im ChefZ_QualityManager - ein
    //! Test, der Fehlerfaelle durchspielt, darf ChefZ_Log.GetErrorCount()
    //! nicht fuellen und damit den Server Richtung SAFE_MODE druecken (18 §4).
    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }
}

//==============================================================================

/**
 * ChefZ_PortionOutputAudit - die Bauzeitpruefung der Portionsfelder.
 *
 * Entwurf: 15 §7 (die Zeilen zu portionClass und portions), 08 §8 (der
 * Rezeptbau weist ab, was zur Laufzeit nichts kosten darf).
 *
 * Sie steht hier und nicht im Rezeptcompiler, weil DIESELBEN Regeln fuer
 * Transforms gelten (11 E4: dasselbe ChefZ_OutputDef). Zwei Kopien waeren zwei
 * Gelegenheiten, sie unterschiedlich falsch zu lesen - und die Regel "ein
 * Topf, aus dem man nichts entnehmen kann, darf gar nicht erst entstehen" ist
 * zu wichtig fuer eine Kopie.
 *
 * Die Klassenpruefung selbst bleibt beim Aufrufer: er hat sie schon fuer die
 * Ergebnisklasse (ChefZ_VanillaNutrition), und die Portionsklasse braucht
 * exakt dieselbe - Existenz UND Magenregistrierung (01 V7).
 */
class ChefZ_PortionOutputAudit
{
    /**
     * Prueft und BEGRADIGT die Portionsfelder eines Ergebnisses.
     *
     * @param outWarnings          wird gefuellt, nie geleert. Jeder Eintrag ist
     *                             eine Begradigung, die der Aufrufer melden soll.
     * @param outPortionClass      die zusaetzlich zu pruefende Klasse, oder "".
     * @param rejectReason         bei false gesetzt, sonst leer.
     *
     * @return false = Rezept bzw. Transform ABWEISEN.
     */
    static bool Audit(notnull ChefZ_OutputDef def, string where, notnull array<string> outWarnings, out string outPortionClass, out string rejectReason)
    {
        outPortionClass = "";
        rejectReason    = "";

        bool hasCount = def.portions > 0;
        bool hasClass = def.portionClass != "";

        if (!hasCount && !hasClass)
            return true;                    // 15 §7, Zeile 1: kein Portionsgericht

        /**
         * portionClass ohne portions: als portions = 1 gelesen.
         *
         * 15 E7 sagt, ein Einzelgericht IST ein Portionsgericht mit
         * portions = 1. Wer eine Portionsklasse nennt, meint genau das - die
         * Zahl 1 dazuzuschreiben waere eine Formalie. Sie stillschweigend als
         * "kein Portionsgericht" zu lesen waere dagegen die schlimmste
         * Antwort: das Gericht entstuende, und niemand kaeme an den Inhalt.
         */
        if (hasClass && !hasCount)
        {
            def.portions = 1;
            outWarnings.Insert(where + " nennt \"portionClass\", aber kein \"portions\" - " + "gelesen als 1 Portion (15 E7: ein Einzelgericht ist ein Portionsgericht " + "mit portions = 1).");
            hasCount = true;
        }

        /**
         * portions ohne portionClass: kein Portionsgericht.
         *
         * Ein Zaehler ohne etwas zum Entnehmen ist ein Gericht mit einer Zahl
         * darauf und keiner Aktion. 15 §7 Zeile 1 gibt die Richtung vor
         * ("keine Portionsdaten -> gewoehnliches Item, kein Fehler"), also
         * wird der Zaehler entfernt und nicht das Rezept abgewiesen.
         */
        if (hasCount && !hasClass)
        {
            outWarnings.Insert(where + " nennt " + def.portions.ToString() + " Portionen, aber keine \"portionClass\" - der Zaehler entfaellt und das " + "Ergebnis entsteht als gewoehnliches Item. Ein Zaehler ohne Entnahmeklasse " + "waere ein Topf, aus dem man nichts herausbekommt (15 §7).");
            def.portions = 0;
            return true;
        }

        // 15 §7: geklemmt, WARN. Hier schon und nicht erst in der Registry -
        // ein Autor soll die Zahl in seiner Datei korrigieren koennen.
        if (def.portions > ChefZ_PortionLimits.MAX)
        {
            outWarnings.Insert(where + ": " + def.portions.ToString() + " Portionen sind mehr " + "als die " + ChefZ_PortionLimits.MAX.ToString() + ", die sich zum Client " + "synchronisieren lassen - geklemmt (03 §4).");
            def.portions = ChefZ_PortionLimits.MAX;
        }

        // Ein Bulk, der sich selbst portioniert, waere eine Endlosquelle: jede
        // Entnahme erzeugte wieder ein Bulk mit vollem Zaehler.
        if (def.portionClass == def.cls)
        {
            rejectReason = where + ": \"portionClass\" ist dieselbe Klasse wie \"cls\" (\"" + def.cls + "\") - abgewiesen. Jede Entnahme erzeugte wieder ein volles " + "Portionsgericht; das waere eine unbegrenzte Nahrungsquelle.";
            return false;
        }

        // 16 §7: eine fehlende Restklasse kostet die Rueckgabe, nicht das
        // Rezept. Geprueft wird sie vom Aufrufer - hier faellt nur auf, dass
        // sie gleich der Portionsklasse ist, und das waere widerspruechlich.
        if (def.emptyOnLastPortion != "" && def.emptyOnLastPortion == def.portionClass)
        {
            outWarnings.Insert(where + ": \"emptyOnLastPortion\" ist dieselbe Klasse wie " + "\"portionClass\". Die letzte Entnahme liesse damit eine weitere Portion " + "zurueck - der Eintrag wird ignoriert und der Rest geloescht (15 §2).");
            def.emptyOnLastPortion = "";
        }

        if (def.HasAmountPerPortion() && def.amountPerPortion < 0.0)
        {
            outWarnings.Insert(where + ": \"amountPerPortion\" ist negativ - der Mengendeckel " + "entfaellt. Damit deckelt allein das Geraet (15 §5.2).");
            def.amountPerPortion = 0.0;
        }

        outPortionClass = def.portionClass;
        return true;
    }
}
