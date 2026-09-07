//==============================================================================
// ChefZ_PortionedFood_Base - das Bulk-Gericht mit eigenem Zaehler
//
// Entwurf: 15 §2 (das Modell), 15 §3 (Schnittstelle woertlich), 15 §4
// (ENTNAHME, Schritt fuer Schritt), 15 §6 (Zustandstabelle), 15 §7
// (Fehlerverhalten Zeile fuer Zeile), 15 E1 (Zaehler statt N Einzelitems),
// 15 E2 (NICHT die Vanilla-quantity), 15 E4 (unveraendert erben), 15 E6
// (serverseitige Revalidierung), 15 E8 (das Bulk darf das Gefaess verlassen).
//
// ---------------------------------------------------------------------------
// Warum ein eigener int und nicht Vanillas quantity - 15 E2, belegt in 01 V5
// ---------------------------------------------------------------------------
// Cooking.DecreaseCookedItemQuantity zieht bei JEDEM FoodStage-Wechsel 25 ab,
// ProcessItemToCook zieht bei Ueberhitzung ab, Split und Stack greifen
// ebenfalls zu. Ein Portionszaehler auf quantity verloere Portionen durch
// blosses WARMHALTEN - schwer erklaerbar und schwer zu balancieren.
//
// Zusaetzlich traegt quantity in DayZ bereits die Verzehrmenge pro Biss und
// geht in die Naehrwertrechnung ein. Ein separater int trennt "wie viele
// Schuesseln" sauber von "wie viel ist in dieser Schuessel".
//
// ---------------------------------------------------------------------------
// Wo die beiden Zahlen leben
// ---------------------------------------------------------------------------
//   m_ChefZ_Portions      im gemeinsamen ChefZ_ItemStateComponent.
//                         Persistiert UND gesynct (0..31), also auf dem Client
//                         lesbar - der Tooltip braucht ihn.
//   m_ChefZ_PortionsMax   HIER, in einem eigenen kleinen Block hinter dem
//                         Zustandsblock. Persistiert, NICHT gesynct (15 §6).
//
// Warum die Hoechstzahl NICHT im gemeinsamen Block liegt: der wird von jedem
// ChefZ-Item geschrieben, auch von Salz und Mehl. Ein zusaetzliches Feld dort
// haette die Blockversion erhoeht und den Lesestrom JEDES gespeicherten
// ChefZ-Items um vier Bytes verschoben - fuer eine Zahl, die nur ein
// Portionsgericht braucht. Ein eigener Block auf der einzigen Klasse, die ihn
// braucht, kostet nichts und bricht nichts.
//
// ---------------------------------------------------------------------------
// FUER CONTENT-AUTOREN
// ---------------------------------------------------------------------------
//     config.cpp   class ChefZ_HunterStewBulk : Edible_Base { ... };
//     script       class ChefZ_HunterStewBulk extends ChefZ_PortionedFood_Base { }
//
// Mehr ist nicht noetig: die Entnahmeaktion, der Zaehler, die Persistenz und
// der Tooltip kommen von hier. WAS entsteht, steht im Rezept (portionClass) -
// nicht in dieser Klasse und nirgends sonst im Core.
//
// Der Core bringt fuer diese Klasse KEINEN CfgVehicles-Eintrag mit; er
// enthaelt keine Items, auch keine unsichtbaren (Invariante I3).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_PortionedFood_Base extends ChefZ_Edible_Base
{
    //! "CHZP". Eigener Marker vor dem eigenen Block - dasselbe Muster und
    //! derselbe Grund wie bei ChefZ_ItemStateComponent.MAGIC.
    static const int PORTION_MAGIC   = 0x43485A50;

    //! Bei JEDER Feldaenderung dieses Blocks erhoehen (V-B §2 Folge 4).
    static const int PORTION_VERSION = 1;

    /**
     * Die Hoechstzahl fuer die Anzeige "3 / 8".
     *
     * NICHT gesynct (15 §6). Der Client bekommt sie ueber die Klasse nicht -
     * der Tooltip zeigt dort denselben Wert wie der Zaehler, solange kein
     * Server-Sync sie mitbringt. Das ist die bewusste Entscheidung aus 15 §6:
     * eine zweite Sync-Variable auf jedem Kessel kostet Bandbreite fuer eine
     * Zahl, die sich nie aendert.
     */
    protected int m_ChefZ_PortionsMax;

    void ChefZ_PortionedFood_Base()
    {
        m_ChefZ_PortionsMax = 0;
    }

    //==========================================================================
    // Die API aus 15 §3
    //==========================================================================

    override int ChefZ_GetPortionsMax()
    {
        // Wer nie eine Hoechstzahl bekommen hat - ein Item aus einem
        // Spielstand vor S16, ein Adminspawn - bekommt den aktuellen Stand als
        // Hoechstzahl. Das ist die einzige Antwort, die nie "3 / 0" anzeigt.
        if (m_ChefZ_PortionsMax <= 0)
            return ChefZ_GetPortions();
        return m_ChefZ_PortionsMax;
    }

    /**
     * Zaehler und Hoechstzahl setzen (15 §4, ERZEUGUNG).
     *
     * @param max  < 0 heisst "Hoechstzahl unveraendert lassen". Das ist der
     *        Fall der Entnahme: dort sinkt der Zaehler, die Hoechstzahl bleibt.
     *
     * Serverseitig. ChefZ_ItemStateComponent.SetPortions prueft das selbst und
     * gibt sonst false zurueck; die Hoechstzahl wird dann ebenfalls nicht
     * angefasst, damit Client und Server nicht auseinanderlaufen.
     */
    override bool ChefZ_SetPortions(int count, int max = -1)
    {
        if (!ChefZ_ItemStateComponent.SetPortions(this, count))
            return false;

        if (max >= 0)
            m_ChefZ_PortionsMax = Math.Clamp(max, 0, ChefZ_PortionLimits.MAX);

        // 15 §7: "portionsLeft > portionsMax -> auf portionsMax geklemmt".
        // Hier andersherum gedreht: die Hoechstzahl waechst mit, wenn jemand
        // einen hoeheren Zaehler setzt. Ein Kessel mit "9 / 8" waere fuer den
        // Spieler unerklaerlich, und der Zaehler ist die Wahrheit - er
        // bestimmt, wie oft entnommen werden kann.
        if (m_ChefZ_PortionsMax < ChefZ_GetPortions())
            m_ChefZ_PortionsMax = ChefZ_GetPortions();

        return true;
    }

    /**
     * Ist dieses Item gerade ein entnehmbares Portionsgericht?
     *
     * ZWEI Bedingungen, und beide sind noetig:
     *
     *   1. seine Klasse ist als Portionsgericht deklariert - sonst wuesste
     *      niemand, WAS entstehen soll;
     *   2. es ist noch etwas drin.
     *
     * 15 §7: "portionsLeft <= 0 bei existierendem Item -> Action erscheint
     * nicht; das Item bleibt als normales Item verzehrbar, wenn seine Klasse
     * das erlaubt. KEIN Loeschen von Spielerbesitz."
     */
    override bool ChefZ_IsBulk()
    {
        if (ChefZ_GetPortions() <= 0)
            return false;
        return ChefZ_PortionManager.Get().IsBulkClass(ChefZ_SymbolTable.Lookup(GetType()));
    }

    //! Die Spec dieser Klasse, oder null. null ist eine normale Antwort: die
    //! Klasse kann von hier erben, ohne dass ein Rezept sie je erzeugt.
    ChefZ_PortionSpec ChefZ_GetPortionSpec()
    {
        ChefZ_PortionSpec spec;
        if (!ChefZ_PortionManager.Get().GetSpecForBulk(ChefZ_SymbolTable.Lookup(GetType()), spec))
            return null;
        return spec;
    }

    //==========================================================================
    // Die Entnahme (15 §4, ENTNAHME) - SERVER, AUSSCHLIESSLICH
    //==========================================================================

    /**
     * Entnimmt genau eine Portion.
     *
     * Die Reihenfolge steht so in 15 §4 und ist nicht verhandelbar - sie ist
     * Invariante I5 fuer diesen Pfad:
     *
     *   1. REVALIDIEREN  Portionen, Klasse, Spek, Handelnder
     *   2. VETO          ChefZ_OnPortionTaken, stornierbar (17 E5)
     *   3. ERZEUGEN      Haende -> Inventar -> Boden
     *   4. UEBERTRAGEN   Zustand, Qualitaet, Frische, Temperatur, Agenten
     *   5. BEHAELTER     verbrauchen (S17)
     *   6. DEKREMENTIEREN                       <- ERST JETZT
     *   7. QUELLE        ersetzen oder loeschen, wenn leer
     *   8. SetSynchDirty
     *
     * Schritt 6 ist der erste Schritt, der dem Spieler etwas WEGNIMMT, und er
     * kommt nach allem, was scheitern kann. 15 §7 sagt es zweimal:
     * "Abbruch VOR dem Dekrementieren. Zaehler unveraendert, Behaelter
     * unverbraucht, WARN. NIE Portionen ins Nichts verlieren."
     *
     * @return true nur, wenn eine Portion entstanden UND der Zaehler gesenkt
     *         ist. Bei false ist die Welt unveraendert.
     */
    bool ChefZ_TakePortion(PlayerBase actor, out ItemBase outPortion, out string err)
    {
        outPortion = null;
        err        = "";

        //--- Torwaechter (15 §7, letzte Zeile) --------------------------------
        // "ChefZ_TakePortion clientseitig gerufen -> No-op mit ERROR."
        if (!g_Game || !g_Game.IsServer())
        {
            err = "Entnahme ausserhalb des Servers angefordert";
            ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.PORTION, "portion.client." + GetType(), "ChefZ_TakePortion wurde clientseitig gerufen (" + GetType() + "). Es " + "passiert nichts. Nichts Autoritatives laeuft auf dem Client (00 §5).");
            return false;
        }

        //--- 1. Revalidieren (15 E6) ------------------------------------------
        if (IsSetForDeletion())
        {
            err = "die Quelle wird gerade geloescht";
            return false;
        }

        ChefZ_PortionManager mgr = ChefZ_PortionManager.Get();

        ChefZ_PortionRequest req = new ChefZ_PortionRequest();
        if (!ChefZ_BuildPortionRequest(actor, req))
        {
            err = "diese Klasse ist kein Portionsgericht";
            return false;
        }

        ChefZ_PortionPlan plan;
        if (!mgr.BuildPortionPlan(req, plan))
        {
            // Der haeufigste Fall ist "zwei Spieler entnehmen gleichzeitig"
            // (15 §7): der zweite sieht portions == 0 und bricht wirkungslos
            // ab. Das ist kein Fehler und bekommt deshalb kein WARN.
            string why;
            mgr.CanTakePortion(req, why);
            err = why;

            if (ChefZ_Log.Enabled(ChefZ_LogChannel.PORTION, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.PORTION, GetType() + ": Entnahme abgelehnt - " + why + ". Es wurde nichts veraendert.");
            return false;
        }

        //--- 2. Veto (17 E5: stornierbar heisst VOR der Wirkung) --------------
        //
        // 15 §4 zeichnet ChefZ_OnPortionTaken als Schritt 8, NACH allem. Die
        // Ereignistabelle in 17 §3.1 fuehrt es zugleich als STORNIERBAR, und
        // 17 E5 sagt dazu: "alle vor einer Wirkung. Ein stornierbares
        // 'Completed' waere eine Falle."
        //
        // Beides zugleich geht nicht. Aufgeloest wird es zugunsten von 17 E5,
        // weil dort die BEGRUENDUNG steht: ein Abonnent, der nach dem
        // Dekrementieren storniert, koennte die Portion nicht zurueckgeben -
        // der Core wuerde ihm glauben und der Spieler haette sie trotzdem.
        // Hier, vor jeder Wirkung, ist die Stornierung folgenlos und damit
        // ehrlich.
        string cancelReason;
        if (ChefZ_RaisePortionVeto(req, plan, cancelReason))
        {
            err = "storniert: " + cancelReason;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.PORTION, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.PORTION, GetType() + ": Entnahme von aussen storniert - " + cancelReason);
            return false;
        }

        //--- 3. Erzeugen ------------------------------------------------------
        string spawnErr;
        ItemBase portion = ChefZ_SpawnPortion(plan.portionClass, actor, spawnErr);
        if (!portion)
        {
            // 15 §7: "Erzeugung des Portions-Items scheitert -> Abbruch VOR
            // dem Dekrementieren." Und die Zeile darunter: "Inventar voll ->
            // Portion auf den Boden. Scheitert auch das: Zaehler NICHT
            // dekrementieren, WARN."
            err = "die Portion konnte nicht erzeugt werden: " + spawnErr;
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PORTION, "portion.spawn." + plan.portionClass, "\"" + plan.portionClass + "\" konnte nicht erzeugt werden (" + spawnErr + "). Der Portionszaehler bleibt unveraendert und es wurde nichts " + "verbraucht - es geht keine Portion verloren.");
            return false;
        }

        //--- 4. Uebertragen (15 E4: UNVERAENDERT) -----------------------------
        if (!ChefZ_CarryToPortion(portion, plan, err))
        {
            // Entstanden, aber unbrauchbar. Sofort wieder weg, und der Zaehler
            // bleibt stehen: der Spieler hat noch alles, was er hatte.
            portion.Delete();
            return false;
        }

        //--- 5. Behaelter (16 §5) ---------------------------------------------
        //
        // ERST JETZT, und das ist der Kern von Invariante I5 an dieser Stelle:
        // die Portion existiert bereits und traegt alles, was sie tragen soll.
        // Scheitert der Verbrauch, wird sie wieder geloescht und der Zaehler
        // bleibt stehen - der Spieler hat danach genau das, was er vorher
        // hatte (16 §7: "Behaelter verschwindet zwischen Auswahl und Verbrauch
        // -> nichts verbraucht, bereits Erzeugtes geloescht, WARN").
        //
        // Ohne deklarierte Behaelter steht in plan.containerToConsume nie ein
        // Symbol (ChefZ_PortionManager.BuildPortionPlan setzt es nur bei
        // bereitem Behaeltersystem) - dieser Zweig ist dann tot und kostet
        // einen Symbolvergleich.
        ChefZ_Sym usedContainer = ChefZ_SymbolTable.INVALID;

        if (plan.NeedsContainer())
        {
            string containerErr;
            if (!ChefZ_ContainerService.ConsumeByClass(plan.containerToConsume, actor, this, usedContainer, containerErr))
            {
                portion.Delete();

                err = "der Behaelter konnte nicht verbraucht werden: " + containerErr;
                ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.CONTAIN, "portion.container.consume." + GetType(), "Die Entnahme aus \"" + GetType() + "\" wurde abgebrochen: " + containerErr + ". Der Portionszaehler bleibt unveraendert und es " + "wurde nichts verbraucht - es geht keine Portion verloren.");
                return false;
            }
        }

        // Die Rueckgabebindung steht am ITEM, nicht am Rezept (16 E3): beim
        // Verzehr ist das Rezept nicht mehr bekannt - das Gericht kann
        // gehandelt, gelagert oder ueber einen Serverneustart getragen worden
        // sein.
        //
        // Zwei Wege, und der zweite ist der interessante:
        //   feste Klasse  hat der Manager bereits aufgeloest.
        //   "AUTO"        wird ERST HIER aufloesbar - nur an dieser Stelle ist
        //                 bekannt, welcher Behaelter tatsaechlich benutzt
        //                 wurde (16 §4). Wer eine Emailleschuessel hineingab,
        //                 bekommt eine Emailleschuessel zurueck.
        ChefZ_Sym returnClass = plan.returnContainerClass;
        if (!ChefZ_SymbolTable.IsValid(returnClass))
        {
            ChefZ_PortionSpec containerSpec;
            if (mgr.GetSpecForBulk(req.sourceClass, containerSpec))
            {
                returnClass = ChefZ_ContainerRegistry.Get().ResolveReturnClass( containerSpec.returnContainer, usedContainer);
            }
        }

        if (ChefZ_SymbolTable.IsValid(returnClass))
        {
            ChefZ_ItemStateComponent.SetReturnContainer(portion, ChefZ_SymbolTable.Name(returnClass));
        }

        //--- 6. Dekrementieren - ERST JETZT -----------------------------------
        ChefZ_SetPortions(plan.portionsLeftAfter, -1);

        //--- 7. Quelle, wenn leer (15 §2) -------------------------------------
        if (plan.sourceBecomesEmpty)
            ChefZ_FinishLastPortion(plan);
        else
            SetSynchDirty();

        outPortion = portion;

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PORTION, ChefZ_LogLevel.INFO))
        {
            ChefZ_Log.Info(ChefZ_LogChannel.PORTION, "Portion entnommen: " + plan.ToDebugString());
        }

        return true;
    }

    /**
     * Fuellt die Frage, die der Manager beantwortet.
     *
     * Die Behaelterliste fuellt seit S17 ChefZ_ContainerService.FillRequest()
     * - der Manager sucht nach wie vor NICHT selbst, er hat kein Inventar
     * (16 §3.2: die Registry macht "reine Auswahl auf Symbolebene, KEIN
     * Inventarzugriff"). Die Suche steht damit dort, wo Items leben, und die
     * Auswahlregel dort, wo sie ohne Welt pruefbar ist.
     *
     * Die Reihenfolge der Liste IST die Auswahl (16 E5: Haende -> Inventar ->
     * Umgebung, dann Gesundheit, dann Klassenname). Sie wird hier nicht
     * angefasst.
     *
     * Diese Methode laeuft auch aus ActionCondition, also bei jedem
     * Zielwechsel des Fadenkreuzes und auch auf dem CLIENT. FillRequest steigt
     * fuer jede Spec ohne Behaelteranforderung sofort wieder aus - und das ist
     * der Normalfall.
     */
    bool ChefZ_BuildPortionRequest(PlayerBase actor, notnull ChefZ_PortionRequest req)
    {
        req.Reset();

        // Lookup und NICHT Intern: diese Methode laeuft auch aus
        // ActionCondition, also bei jedem Zielwechsel des Fadenkreuzes. Intern
        // legte dort fuer jede angeschaute Klasse einen Symboleintrag an -
        // eine Tabelle, die mit der Spielzeit waechst, fuer eine Antwort, die
        // ohnehin "kein Portionsgericht" lautet. Jede echte Bulk-Klasse ist
        // beim Build interniert worden.
        req.sourceClass     = ChefZ_SymbolTable.Lookup(GetType());
        req.portionsLeft    = ChefZ_GetPortions();
        req.sourceState     = ChefZ_GetState();
        req.sourceQuality   = ChefZ_GetQuality();
        req.sourceFreshness = ChefZ_GetFreshness01();
        req.actorIdentityId = ChefZ_PortionActorId(actor);

        if (!ChefZ_PortionManager.Get().IsBulkClass(req.sourceClass))
            return false;

        // S17 (16 §5): die verfuegbaren Behaelter. Nach der Bulk-Pruefung und
        // nicht davor - fuer jedes Item, das gar kein Portionsgericht ist,
        // soll kein Inventar durchsucht werden.
        ChefZ_ContainerService.FillRequest(req, actor, this);

        return true;
    }

    //==========================================================================
    // Erzeugen und Uebertragen
    //==========================================================================

    /**
     * Haende -> Inventar -> Boden (15 §4, Schritt 2).
     *
     * Die Reihenfolge ist die des Entwurfs und nicht Vanillas
     * (HumanInventory.CreateInInventory versucht Inventar, DANN Haende). Der
     * Unterschied ist gewollt: wer eine Portion nimmt, will sie in der Regel
     * essen, und ein Teller, der im Rucksack landet, waere ein zweiter
     * Handgriff.
     *
     * Der Bodenwurf ist der letzte Ausweg und ausdruecklich KEIN Fehler
     * (15 §7: "Inventar voll -> Portion auf den Boden"). Erst wenn auch das
     * scheitert, bricht die Entnahme ab - und dann ohne jede Wirkung.
     */
    protected ItemBase ChefZ_SpawnPortion(string cls, PlayerBase actor, out string err)
    {
        err = "";

        if (cls == "")
        {
            err = "keine Portionsklasse";
            return null;
        }

        if (actor && actor.IsAlive())
        {
            HumanInventory hands = actor.GetHumanInventory();
            if (hands && !hands.GetEntityInHands())
            {
                ItemBase inHands = ItemBase.Cast(hands.CreateInHands(cls));
                if (inHands)
                    return inHands;
            }

            GameInventory inv = actor.GetInventory();
            if (inv)
            {
                ItemBase inInv = ItemBase.Cast(inv.CreateInInventory(cls));
                if (inInv)
                    return inInv;
            }

            ItemBase onGround = ItemBase.Cast(actor.SpawnEntityOnGroundRaycastDispersed(cls));
            if (onGround)
                return onGround;
        }

        // Kein Handelnder (Adminwerkzeug, Automatik): dort, wo das Bulk liegt.
        // ECE_PLACE_ON_SURFACE, damit nichts im Boden versinkt - dieselbe
        // Flagge, die ChefZ_ItemTransform.Create benutzt.
        Object obj = g_Game.CreateObjectEx(cls, GetPosition(), ECE_PLACE_ON_SURFACE);
        ItemBase spawned = ItemBase.Cast(obj);
        if (spawned)
            return spawned;

        if (obj)
            g_Game.ObjectDelete(obj);

        err = "weder Haende noch Inventar noch Boden haben \"" + cls + "\" aufgenommen";
        return null;
    }

    /**
     * Zustand, Qualitaet, Frische, Menge, Temperatur, Agenten (15 §4,
     * Schritt 3).
     *
     * 15 E4: UNVERAENDERT. In dieser Methode steht bewusst keine einzige
     * Rechnung auf einem der drei Werte - "Portionieren ist keine
     * Verarbeitung", und jede Ab- oder Aufwertung eroeffnete Rundungsexploits
     * ueber achtmaliges Portionieren.
     *
     * Temperatur und Agenten kommen von Vanilla
     * (MiscGameplayFunctions.TransferItemProperties), nicht aus einer eigenen
     * Regel: eine spaet entnommene Portion ist entsprechend kaelter, und das
     * macht Vanillas Temperatursystem von allein (15 E4, Nachlauf).
     *
     * @return false, wenn die Portion dabei geloescht wurde. Dann bricht der
     *         Aufrufer ab - der Zaehler steht noch.
     */
    protected bool ChefZ_CarryToPortion(notnull ItemBase portion, notnull ChefZ_PortionPlan plan, out string err)
    {
        err = "";

        // Agenten, Nassheit, Sauberkeit, Temperatur, Health. excludeQuantity,
        // weil die Menge gleich aus dem Plan gesetzt wird: das Bulk hat eine
        // andere Hoechstmenge als eine Schuessel, und eine roh kopierte Zahl
        // waere entweder eine Aufwertung oder ein Verlust.
        MiscGameplayFunctions.TransferItemProperties(this, portion, true, true, true, true);

        /**
         * Der ChefZ-Block wird aus dem PLAN gesetzt, nicht mit
         * ChefZ_ItemStateComponent.InheritFrom uebernommen.
         *
         * Zwei Gruende, und beide sind zwingend:
         *
         *   1. InheritFrom kopiert ALLES, auch den Portionszaehler. Eine
         *      Schuessel Eintopf mit dem Zaehler des Kessels waere eine
         *      unbegrenzte Nahrungsquelle - der schwerste denkbare Fehler in
         *      diesem Teilsystem.
         *   2. Die Spec darf sagen, dass etwas NICHT geerbt wird
         *      (inheritQuality/State/Freshness). Der Plan traegt diese
         *      Entscheidung bereits: ein nicht geerbtes Feld steht dort auf
         *      INVALID bzw. -1. Eine frisch erzeugte Portion hat einen leeren
         *      Zustandsblock, also ist "nicht setzen" gleichbedeutend mit
         *      "Vorgabe der eigenen Klasse" (06 §3, Schritt 2) - und genau das
         *      ist gemeint.
         *
         * 15 E4: die Werte werden UNVERAENDERT uebernommen. Es steht hier
         * keine einzige Rechnung darauf.
         */
        if (ChefZ_SymbolTable.IsValid(plan.stateToApply))
            ChefZ_ItemStateComponent.SetState(portion, plan.stateToApply, false);

        if (ChefZ_SymbolTable.IsValid(plan.qualityToApply))
            ChefZ_ItemStateComponent.SetQuality(portion, plan.qualityToApply);

        if (plan.freshnessToApply >= 0.0)
            ChefZ_ItemStateComponent.SetFreshness01(portion, plan.freshnessToApply);

        // Sicherheitsnetz: eine Portion traegt NIE einen Zaehler. Sie kann
        // ihrerseits ein Portionsgericht sein - der Zaehler dafuer kommt dann
        // beim Kochen, nicht von hier.
        ChefZ_ItemStateComponent.SetPortions(portion, 0);

        if (portion.IsSetForDeletion())
        {
            err = "die Portion wurde beim Uebertragen der Eigenschaften geloescht";
            return false;
        }

        // Menge zuletzt: SetQuantity klemmt selbst und LOESCHT das Item, wenn
        // der Wert das Minimum erreicht und die Klasse varQuantityDestroyOnMin
        // fuehrt (ItemBase.c:3340). Der Rueckgabewert ist genau diese Auskunft
        // und wird deshalb ausgewertet.
        if (plan.quantityToApply > 0.0 && portion.HasQuantity())
        {
            if (portion.SetQuantity(plan.quantityToApply))
            {
                err = "die Portionsmenge " + plan.quantityToApply.ToString() + " liegt unter dem Minimum von \"" + plan.portionClass + "\"";
                return false;
            }
        }

        portion.SetSynchDirty();
        return true;
    }

    /**
     * Die letzte Portion ist heraus (15 §2, 15 §4 Schritt 6).
     *
     * "Bei portions == 0 nach der letzten Entnahme wird das Bulk-Item geloescht
     * (oder durch emptyOnLastPortion ersetzt) und das Gefaess ist wieder leer -
     * kein Rueckstand, kein Aufraeumbedarf."
     *
     * Der Klassentausch laeuft ueber ChefZ_ItemTransform.Swap und nicht ueber
     * einen eigenen Weg: der uebertraegt Temperatur, Health, Agenten und die
     * Verfallsdaten und findet den Platz im selben Behaelter. Scheitert er,
     * bleibt das leere Bulk liegen - ein sichtbarer Rueckstand ist besser als
     * ein verschwundener Kessel.
     */
    protected void ChefZ_FinishLastPortion(notnull ChefZ_PortionPlan plan)
    {
        if (plan.SourceIsReplaced())
        {
            string swapErr;
            ItemBase empty = ChefZ_ItemTransform.Swap(this, plan.emptyClass, -1.0, swapErr);
            if (empty)
                return;

            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PORTION, "portion.empty." + plan.emptyClass, "Nach der letzten Portion sollte \"" + GetType() + "\" durch \"" + plan.emptyClass + "\" ersetzt werden, was nicht gelang (" + swapErr + "). Das leere Gericht bleibt liegen und kann normal entsorgt werden.");

            SetSynchDirty();
            return;
        }

        // Ohne emptyOnLastPortion: loeschen. Das Gefaess ist damit wieder leer.
        Delete();
    }

    //==========================================================================
    // Ereignis (17 §4, stornierbar - siehe ChefZ_TakePortion Schritt 2)
    //==========================================================================

    protected bool ChefZ_RaisePortionVeto(notnull ChefZ_PortionRequest req, notnull ChefZ_PortionPlan plan, out string cancelReason)
    {
        cancelReason = "";

        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (!bus.HasSubscribers(ChefZ_EventNames.PORTION_TAKEN))
            return false;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.PORTION_TAKEN);
        args.identityId   = req.actorIdentityId;
        args.subjectClass = req.sourceClass;
        args.stateBefore  = req.sourceState;
        args.qualityTier  = req.sourceQuality;
        args.portionsLeft = plan.portionsLeftAfter;
        args.AddProduced(ChefZ_SymbolTable.Intern(plan.portionClass));

        int low  = 0;
        int high = 0;
        GetNetworkID(low, high);
        args.SetSubjectNetId(low, high);

        // Ueber lokale Zwischenvariablen: einen out-Parameter als
        // out-Parameter weiterzureichen ist in Enforce nirgends zugesichert.
        string reason;
        string by;
        bool cancelled = bus.RaiseCancellable(args, reason, by);

        if (cancelled && by != "")
            reason = reason + " (storniert von " + by + ")";

        cancelReason = reason;
        return cancelled;
    }

    //! 0, wenn kein Spieler beteiligt ist oder er keine Identitaet hat.
    //! ChefZ_CapabilityGate blockiert bei 0 nichts (17 §3.3).
    static int ChefZ_PortionActorId(PlayerBase player)
    {
        if (!player)
            return 0;

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return 0;

        return identity.GetPlayerId();
    }

    //==========================================================================
    // Vanilla-Anbindung (15 §3)
    //==========================================================================

    override void SetActions()
    {
        super.SetActions();

        // 15 E5: EINE Action fuer ALLE portionierten Gerichte. Ein neues
        // Portionsgericht erbt von hier und ist damit fertig - keine neue
        // Action, keine Core-Aenderung, kein Eintrag irgendwo.
        AddAction(ChefZ_ActionTakePortion);
    }

    /*
     * KEIN eigenes CanBeCooked() mehr - und das ist die Erfuellung von 15 §3,
     * nicht ihre Ruecknahme.
     *
     * 15 §3 sagt "true: Bulk darf warmgehalten werden", und genau das liefert
     * ChefZ_Edible_Base.CanBeCooked() fuer jedes Bulk: ChefZ_PortionedDish_Base
     * bringt Food > FoodStages UND Food > FoodStageTransitions mit (siehe den
     * Block dazu in ChefZ_Cooking/config.cpp), also sind beide Bedingungen der
     * Basis erfuellt und die Antwort ist dieselbe wie vorher.
     *
     * Das feste `return true;` ist verschwunden, weil es nach der
     * Basisaenderung eine ZWEITE, widersprechende Regel gewesen waere - und
     * zwar keine harmlose. Es hat auch dann true gesagt, wenn das Bulk gar
     * keinen Food-Knoten hatte. Vanilla haette dieses Item dann als kochbar
     * angenommen und in Cooking.UpdateCookingState ueber ein nicht
     * vorhandenes FoodStage-Objekt gegriffen (Edible_Base.c:605). Ein
     * Content-Autor, der den Food-Knoten seines Bulks vergisst, bekommt jetzt
     * ein Gericht, das nicht warm wird - statt eines Serverfehlers je Kochtakt.
     *
     * Unveraendert gilt: der Portionszaehler ist von alldem unberuehrt, weil er
     * kein quantity ist (15 E2). Cooking.DecreaseCookedItemQuantity greift auf
     * quantity zu, und dort steht die Verzehrmenge, nicht die Portionszahl.
     */

    /**
     * 15 §3: false. Ein Kessel gehoert nicht auf einen Stock.
     *
     * Ausdruecklich hingeschrieben, obwohl ChefZ_Edible_Base den Schalter gar
     * nicht anfasst und der Vanilla-Default (Edible_Base.c:134) ohnehin false
     * ist: 15 §3 fuehrt diese Zeile als Zusage, und eine Zusage, die nur aus
     * einem geerbten Default besteht, liest niemand nach.
     */
    override bool CanBeCookedOnStick()
    {
        return false;
    }

    /**
     * Tooltip "3 / 8 Portionen" (15 §4, ANZEIGE).
     *
     * Der Zaehler ist gesynct, die Hoechstzahl nicht (15 §6) - clientseitig
     * stehen deshalb im schlechtesten Fall zwei gleiche Zahlen. Das ist die
     * bewusste Entscheidung aus 15 §6 und kein Fehler.
     *
     * Der Text kommt aus der Stringtable, nicht aus dem Code. Widget.
     * TranslateString ist derselbe Weg, den Vanilla in Object.c:487 und
     * DamageSystem.c:135 fuer Anzeigetexte benutzt.
     */
    override string GetTooltip()
    {
        string text = super.GetTooltip();

        int left = ChefZ_GetPortions();
        if (left <= 0)
            return text;

        string line = left.ToString() + " / " + ChefZ_GetPortionsMax().ToString() + " " + Widget.TranslateString(ChefZ_ActionTakePortion.PORTIONS_TEXT);

        if (text == "")
            return line;
        return text + " " + line;
    }

    //==========================================================================
    // Persistenz (15 §6) - eigener Block HINTER dem Zustandsblock
    //==========================================================================

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        // super zuerst, immer: Vanilla-Strom, dann der ChefZ-Zustandsblock,
        // dann dieser (V-B §2 Folge 2).
        super.OnStoreSave(ctx);

        // Lokale Variablen und keine Konstanten direkt im Write:
        // ParamsWriteContext.Write nimmt seinen Parameter als "void" entgegen
        // und braucht etwas Adressierbares. Dieselbe Schreibweise benutzt
        // Vanilla (Edible_Base.c:317-318).
        int magic   = PORTION_MAGIC;
        int version = PORTION_VERSION;
        int max     = m_ChefZ_PortionsMax;

        ctx.Write(magic);
        ctx.Write(version);
        ctx.Write(max);
    }

    /**
     * Liest den Portionsblock.
     *
     * Gibt bei einem unlesbaren Block trotzdem true zurueck - dieselbe Regel
     * und derselbe Grund wie in ChefZ_ItemStateComponent.Load: false aus
     * OnStoreLoad laesst das Item aus dem Spielstand verschwinden. Eine
     * fehlende Hoechstzahl kostet die Anzeige, nicht den Kessel.
     */
    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        m_ChefZ_PortionsMax = 0;

        int magic;
        if (!ctx.Read(magic))
            return true;                    // Item aus einer Fassung vor S16

        if (magic != PORTION_MAGIC)
        {
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PORTION, "portion.magic." + GetType(), "\"" + GetType() + "\": an der Stelle des Portionsblocks steht kein " + "Portionsblock. Die Hoechstzahl faellt auf den aktuellen Zaehler zurueck; " + "das Item bleibt vollstaendig spielbar.");
            return true;
        }

        int blockVersion;
        if (!ctx.Read(blockVersion))
            return true;

        if (blockVersion > PORTION_VERSION)
        {
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PORTION, "portion.version." + blockVersion.ToString(), "Der gespeicherte Portionsblock hat Version " + blockVersion.ToString() + ", dieser Core kennt " + PORTION_VERSION.ToString() + ". Der Rest wird " + "uebersprungen. Das passiert nach einem Downgrade des Mods.");
            return true;
        }

        int max;
        if (!ctx.Read(max))
            return true;

        m_ChefZ_PortionsMax = Math.Clamp(max, 0, ChefZ_PortionLimits.MAX);
        return true;
    }

    /**
     * Nach dem Laden: die beiden Zahlen in Einklang bringen (15 §7).
     *
     *   "portionsLeft > portionsMax beim Laden -> auf portionsMax geklemmt, WARN"
     *   "portions persistiert als negativ -> auf 0 gesetzt, WARN"
     *
     * Die zweite Zeile erledigt bereits ChefZ_ItemStateComponent.Load (es
     * klemmt auf 0..31). Hier bleibt die erste - und sie wird in die andere
     * Richtung aufgeloest als der Wortlaut: die Hoechstzahl waechst auf den
     * Zaehler, statt den Zaehler zu senken.
     *
     * Grund: der Zaehler ist die Wahrheit - er bestimmt, wie oft entnommen
     * werden kann. Ihn zu senken naehme dem Spieler Portionen weg, die er
     * hatte; die Hoechstzahl anzuheben kostet nur eine Anzeigezeile. 15 §7
     * verlangt in derselben Tabelle zweimal ausdruecklich, Spielerbesitz nicht
     * zu loeschen, und diese Auslegung folgt dieser Linie.
     */
    override void AfterStoreLoad()
    {
        super.AfterStoreLoad();

        int left = ChefZ_GetPortions();
        if (left > m_ChefZ_PortionsMax)
        {
            if (m_ChefZ_PortionsMax > 0)
            {
                ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PORTION, "portion.maxmismatch." + GetType(), "\"" + GetType() + "\": der gespeicherte Zaehler (" + left.ToString() + ") liegt ueber der gespeicherten Hoechstzahl (" + m_ChefZ_PortionsMax.ToString() + "). Die Hoechstzahl wird angehoben; " + "der Spieler behaelt seine Portionen. Diese Meldung erscheint je " + "Klasse einmal.");
            }
            m_ChefZ_PortionsMax = left;
        }
    }
}
