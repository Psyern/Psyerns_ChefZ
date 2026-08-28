//==============================================================================
// ChefZ_ContainerService - suchen, verbrauchen, zurueckgeben
//
// Entwurf: 16 §3.2 (Schnittstelle woertlich), 16 §5 (Datenfluss, Schritt fuer
// Schritt), 16 §6 (Zustandstabelle), 16 §7 (Fehlerverhalten Zeile fuer Zeile),
// 16 E4 (Rueckgabe an OnConsume, NIE an EEDelete), 16 E5 (Haende -> Inventar
// -> Umgebung, in dieser Reihenfolge), 17 §4 (ChefZ_OnFoodConsumed,
// ChefZ_OnContainerReturned).
//
// ---------------------------------------------------------------------------
// Die zwei Saetze, auf die sich dieses Teilsystem reduzieren laesst
// ---------------------------------------------------------------------------
// 1. Ein Behaelter wird genau dann gebraucht, wenn etwas HINEINKOMMT - also
//    beim Servieren, nie beim Kochen (16 §2, E1). Der Grund ist nicht Geschmack,
//    sondern Vanilla: Cooking.ProcessItemToCook behandelt JEDES Cargo-Item und
//    beschaedigt alles, was nicht IsCookware() ist, ueber PARAM_BURN_DAMAGE_COEF
//    (01 V3). Ein Teller im Topf ginge im Feuer kaputt. In diesem ganzen
//    Teilsystem steht deshalb keine einzige Zeile, die einen Behaelter in ein
//    Kochgeraet legt oder aus einem herausnimmt.
//
// 2. Zurueck kommt er bei Quantity <= 0 in OnConsume - nicht bei jedem Bissen
//    und nicht an EEDelete (16 E4). Beides waere ein Duplikationsexploit
//    erster Ordnung: EEDelete feuert auch bei Serverstopp, Cleanup und
//    Adminloeschung, und "jeder Bissen" ergaebe je Bissen einen Teller.
//
// ---------------------------------------------------------------------------
// Wer hier entscheidet
// ---------------------------------------------------------------------------
//   CLIENT   darf SUCHEN. FindCandidates() liest Inventar und Umgebung, damit
//            ActionCondition weiss, ob die Aktion erscheinen soll (16 §5,
//            letzter Absatz). Er veraendert dabei nichts.
//   SERVER   verbraucht und gibt zurueck. ConsumeForServing() und
//            ReturnEmpty() steigen ausserhalb des Servers wirkungslos aus -
//            nichts Autoritatives auf dem Client (00 §5).
//
// ---------------------------------------------------------------------------
// Die feste Reihenfolge (16 E5)
// ---------------------------------------------------------------------------
//   1. HAENDE      was in der Hand ist, wird zuerst genommen
//   2. INVENTAR    Rucksack, Taschen, Weste
//   3. UMGEBUNG    Kisten und Faesser im Umkreis, nur wenn ausdruecklich
//                  erlaubt (NEARBY_CARGO)
//
// Innerhalb einer Stufe entscheidet Gesundheit, dann Klassenname - also
// deterministisch und nicht nach Slot-Zufall. Zwei Spieler mit demselben
// Inventar bekommen denselben Teller, und derselbe Spieler bekommt zweimal
// dieselbe Antwort.
//
// KEIN CONTENT: kein Teller, keine Schuessel, keine Kategorie, kein Gericht.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_ContainerService
{
    /**
     * Ab wann ein Gericht als AUFGEGESSEN gilt.
     *
     * Nicht 0.0, und der Grund steht in Vanilla: ActionConsume.OnEndServer
     * prueft "GetQuantity() <= 0.01" und setzt die Menge dann auf 0
     * (ActionConsume.c:51). Vanillas eigene Grenze fuer "leer" ist also 0.01,
     * nicht 0 - eine kontinuierliche Essaktion laesst regelmaessig
     * Bruchteile stehen.
     *
     * Diese Zahl HIER gleichzuziehen ist der Unterschied zwischen "der Teller
     * kommt zurueck" und "der Teller kommt fast immer zurueck".
     */
    static const float EMPTY_EPSILON = 0.01;

    /**
     * Merker "hier ist bereits zurueckgegeben worden".
     *
     * Er steht in derselben Itemvariablen wie die Rueckgabeklasse und ist
     * deshalb gratis: kein zusaetzliches Feld, keine neue Blockversion im
     * Spielstand, kein zusaetzliches Byte auf irgendeinem Item.
     *
     * Gebraucht wird er, weil die Bindung nicht immer AM ITEM steht: sie darf
     * auch aus CfgChefZIngredients kommen (16 E3). In diesem Fall gaebe es
     * ohne Merker nichts zu loeschen - und ein zweiter Aufruf von OnConsume
     * auf einem bereits leeren Gericht ergaebe einen zweiten Teller.
     *
     * Ein Klassenname kann nie so heissen; die Verwechslungsgefahr ist null.
     */
    static const string RETURN_DONE = "-";

    //--- Zaehler fuer "chefz stats" (18 §2) ----------------------------------
    private static int s_CountConsumed;
    private static int s_CountReturned;
    private static int s_CountReturnFailed;

    //==========================================================================
    // SUCHEN (16 §3.2, E5) - Client UND Server, rein lesend
    //==========================================================================

    /**
     * Alle benutzbaren Behaelter einer Kategorie, beste Wahl zuerst.
     *
     * outCandidates wird GELEERT und gefuellt, nie null.
     *
     * Diese Methode veraendert NICHTS. Sie laeuft auch clientseitig, und zwar
     * bei jedem Zielwechsel des Fadenkreuzes - jede Stufe steigt deshalb so
     * frueh wie moeglich aus:
     *
     *   - unbekannte Kategorie      -> sofort 0, ohne einen Blick ins Inventar
     *   - searchScope ohne das Bit  -> die Stufe wird gar nicht betreten
     *   - Deckel erreicht           -> Abbruch (siehe MaxCandidates)
     *
     * @param device  das Gefaess bzw. Bulk-Gericht, um das es geht. Es liefert
     *        den Mittelpunkt fuer die Umgebungssuche - NICHT den Spieler:
     *        gesucht wird um das Kochgeraet herum, weil dort die Kisten
     *        stehen, und weil der Spieler beim Servieren ohnehin daneben
     *        steht. null ist zulaessig; dann entfaellt die Umgebungsstufe.
     *
     * @return Zahl der Fundstellen.
     */
    static int FindCandidates(ChefZ_Sym category, PlayerBase actor, ItemBase device, out array<ItemBase> outCandidates)
    {
        if (!outCandidates)
            outCandidates = new array<ItemBase>();
        outCandidates.Clear();

        ChefZ_ContainerRegistry reg = ChefZ_ContainerRegistry.Get();
        if (!reg.IsReady() || !reg.CategoryExists(category))
            return 0;

        int scope = reg.GetSearchScope(category);
        if (scope == ChefZ_ContainerScope.NONE)
            return 0;

        int limit = MaxCandidates();

        // ---- 1. HAENDE ------------------------------------------------------
        if (ChefZ_ContainerScope.Has(scope, ChefZ_ContainerScope.HANDS) && actor)
        {
            array<ItemBase> hands = new array<ItemBase>();
            CollectHands(actor, hands);
            AppendStage(hands, category, ChefZ_ContainerScope.HANDS, outCandidates, limit);
        }

        // ---- 2. INVENTAR ----------------------------------------------------
        if (outCandidates.Count() < limit && ChefZ_ContainerScope.Has(scope, ChefZ_ContainerScope.INVENTORY) && actor)
        {
            array<ItemBase> inv = new array<ItemBase>();
            CollectInventory(actor, inv);
            AppendStage(inv, category, ChefZ_ContainerScope.INVENTORY, outCandidates, limit);
        }

        // ---- 3. UMGEBUNG ----------------------------------------------------
        if (outCandidates.Count() < limit && ChefZ_ContainerScope.Has(scope, ChefZ_ContainerScope.NEARBY_CARGO))
        {
            array<ItemBase> nearby = new array<ItemBase>();
            CollectNearbyCargo(actor, device, nearby);
            AppendStage(nearby, category, ChefZ_ContainerScope.NEARBY_CARGO, outCandidates, limit);
        }

        return outCandidates.Count();
    }

    //! Die beste Wahl, oder null (16 §3.2). Duenne Huelle um FindCandidates -
    //! die Reihenfolge IST die Auswahl.
    static ItemBase FindBest(ChefZ_Sym category, PlayerBase actor, ItemBase device)
    {
        array<ItemBase> found = new array<ItemBase>();
        if (FindCandidates(category, actor, device, found) == 0)
            return null;
        return found.Get(0);
    }

    /**
     * Die beste Wahl einer BESTIMMTEN Klasse.
     *
     * Gebraucht wird sie beim Ausfuehren: der Plan nennt eine Klasse (der
     * ChefZ_PortionManager hat sie aus der geordneten Liste gewaehlt), und
     * Sekunden spaeter muss genau so ein Behaelter noch da sein. 16 §7:
     * "Behaelter verschwindet zwischen Auswahl und Verbrauch -> ConsumeForServing
     * scheitert -> nichts verbraucht."
     *
     * Gesucht wird ueber die KATEGORIEN dieser Klasse, damit die Regeln
     * dieselben bleiben - insbesondere der searchScope.
     */
    static ItemBase FindBestOfClass(ChefZ_Sym containerClass, PlayerBase actor, ItemBase device)
    {
        ChefZ_ContainerDef def;
        if (!ChefZ_ContainerRegistry.Get().GetDef(containerClass, def))
            return null;
        if (!def.containerCategories)
            return null;

        for (int c = 0; c < def.containerCategories.Count(); c++)
        {
            ChefZ_Sym category = ChefZ_SymbolTable.Lookup(def.containerCategories.Get(c));
            if (!ChefZ_SymbolTable.IsValid(category))
                continue;

            array<ItemBase> found = new array<ItemBase>();
            FindCandidates(category, actor, device, found);

            for (int i = 0; i < found.Count(); i++)
            {
                ItemBase item = found.Get(i);
                if (item && ChefZ_SymbolTable.Lookup(item.GetType()) == containerClass)
                    return item;
            }
        }
        return null;
    }

    //==========================================================================
    // VERBRAUCHEN (16 §5) - SERVER, AUSSCHLIESSLICH
    //==========================================================================

    /**
     * Erst pruefen, dann loeschen (16 §3.2, "transaktionssicher").
     *
     * Sie wird gerufen, NACHDEM das Gericht bereits erzeugt ist (16 §5,
     * Schritt 2). Scheitert sie, loescht der Aufrufer das Erzeugte wieder und
     * bricht ohne jede Wirkung ab - der Spieler hat danach genau das, was er
     * vorher hatte.
     *
     * consumedOnServe = 0 ist KEIN Fehlschlag: der Behaelter bleibt beim
     * Spieler, wird also nicht geloescht, und die Auskunft "benutzt wurde
     * diese Klasse" stimmt trotzdem. Dass es dann auch nichts zurueckzugeben
     * gibt, entscheidet ChefZ_ContainerRegistry.ReturnsEmpty() - an einer
     * Stelle, nicht an zweien.
     *
     * @param usedClass  die tatsaechlich benutzte Klasse. SIE ist die Antwort
     *        auf "AUTO" (16 §4): wer eine Emailleschuessel hineingab, bekommt
     *        eine Emailleschuessel zurueck.
     */
    static bool ConsumeForServing(notnull ItemBase container, out ChefZ_Sym usedClass, out string err)
    {
        usedClass = ChefZ_SymbolTable.INVALID;
        err       = "";

        if (!g_Game || !g_Game.IsServer())
        {
            err = "Behaelterverbrauch ausserhalb des Servers angefordert";
            ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.CONTAIN, "container.consume.client", "ChefZ_ContainerService.ConsumeForServing wurde clientseitig gerufen. Es " + "passiert nichts. Nichts Autoritatives laeuft auf dem Client (00 §5).");
            return false;
        }

        if (container.IsSetForDeletion())
        {
            err = "der Behaelter wird gerade geloescht";
            return false;
        }

        ChefZ_Sym classSym = ChefZ_SymbolTable.Lookup(container.GetType());

        ChefZ_ContainerDef def;
        if (!ChefZ_ContainerRegistry.Get().GetDef(classSym, def))
        {
            err = "\"" + container.GetType() + "\" ist kein deklarierter Behaelter";
            return false;
        }

        // Zwischen Auswahl und Verbrauch koennen Sekunden liegen. Was in
        // dieser Zeit gefuellt wurde, ist keine freie Schuessel mehr.
        if (!IsUsable(container))
        {
            err = "\"" + container.GetType() + "\" ist nicht mehr frei (gefuellt oder " + "ruiniert)";
            return false;
        }

        usedClass = classSym;

        if (def.consumedOnServe)
        {
            container.Delete();
            s_CountConsumed++;
        }

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONTAIN, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.CONTAIN, "Behaelter \"" + container.GetType() + "\" fuer das Servieren " + BoolWord(def.consumedOnServe, "verbraucht", "benutzt, aber nicht verbraucht") + ".");
        }

        return true;
    }

    /**
     * Suchen und verbrauchen in einem Schritt - die Nahtstelle fuer S16.
     *
     * Der Plan aus dem ChefZ_PortionManager nennt eine KLASSE, kein Item; der
     * Manager kennt kein Inventar (das ist der Grund, warum er in 3_Game
     * liegt). Diese Methode schlaegt die Bruecke und haelt beide Fehlerwege
     * an einer Stelle:
     *
     *   nichts gefunden        -> der Behaelter ist zwischenzeitlich weg
     *   Verbrauch gescheitert  -> er ist da, aber nicht mehr benutzbar
     *
     * In beiden Faellen false und NICHTS veraendert.
     */
    static bool ConsumeByClass(ChefZ_Sym containerClass, PlayerBase actor, ItemBase device, out ChefZ_Sym usedClass, out string err)
    {
        usedClass = ChefZ_SymbolTable.INVALID;
        err       = "";

        ItemBase container = FindBestOfClass(containerClass, actor, device);
        if (!container)
        {
            err = "kein Behaelter der Klasse \"" + ChefZ_SymbolTable.NameOrMark(containerClass) + "\" mehr im Zugriff";
            return false;
        }

        // Ueber lokale Zwischenvariablen: einen out-Parameter als
        // out-Parameter weiterzureichen ist in Enforce nirgends zugesichert
        // (dieselbe Vorsicht wie in
        // ChefZ_PortionedFood_Base.ChefZ_RaisePortionVeto).
        ChefZ_Sym used;
        string    why;
        bool      ok = ConsumeForServing(container, used, why);

        usedClass = used;
        err       = why;
        return ok;
    }

    //==========================================================================
    // ZURUECKGEBEN (16 §5, §7) - SERVER, AUSSCHLIESSLICH
    //==========================================================================

    /**
     * Haende -> Inventar -> Boden am Spieler (16 §3.2, woertlich).
     *
     * Der Bodenwurf ist ausdruecklich KEIN Fehler (16 §7: "Haende belegt UND
     * Inventar voll -> Behaelter faellt zu Boden. INFO. Nie stillschweigend
     * verwerfen - ein verschwundener Teller ist ein Bugreport").
     *
     * Der tote Spieler ist ebenfalls vorgesehen (16 §7, letzte Zeile): ohne
     * lebenden Verzehrer geht der Behaelter an die uebergebene Position -
     * also dorthin, wo das Gericht war. Scheitert auch das, entfaellt die
     * Rueckgabe mit WARN. NIE ein Absturz.
     *
     * @return das erzeugte Item oder null.
     */
    static ItemBase ReturnEmpty(ChefZ_Sym emptyClass, PlayerBase consumer, vector fallbackPos, out string err)
    {
        err = "";

        if (!g_Game || !g_Game.IsServer())
        {
            err = "Rueckgabe ausserhalb des Servers angefordert";
            return null;
        }

        if (!ChefZ_SymbolTable.IsValid(emptyClass))
        {
            err = "keine Rueckgabeklasse";
            return null;
        }

        string cls = ChefZ_SymbolTable.Name(emptyClass);
        if (cls == "")
        {
            err = "leere Rueckgabeklasse";
            return null;
        }

        // Zwischenvariable, siehe ConsumeByClass.
        string spawnErr;
        ItemBase created = SpawnFor(cls, consumer, fallbackPos, spawnErr);
        err = spawnErr;

        if (!created)
        {
            s_CountReturnFailed++;
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.CONTAIN, "container.return." + cls, "Der leere Behaelter \"" + cls + "\" konnte nicht zurueckgegeben werden (" + err + "). Das Gericht war trotzdem verzehrt - es geht nur der Behaelter " + "verloren. Diese Meldung erscheint je Klasse einmal.");
            return null;
        }

        s_CountReturned++;
        RaiseContainerReturned(created, consumer);

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONTAIN, ChefZ_LogLevel.DEBUG))
            ChefZ_Log.Debug(ChefZ_LogChannel.CONTAIN, "Behaelter \"" + cls + "\" zurueckgegeben.");

        return created;
    }

    /**
     * Der Verzehrpfad (16 §5, VERZEHR; 16 E4).
     *
     * Gerufen aus ChefZ_Edible_Base.OnConsume, und zwar NACH super - dort hat
     * Vanilla die Menge bereits abgezogen (Edible_Base.Consume ruft
     * AddQuantity(-amount) VOR OnConsume, Edible_Base.c:96-98). Die Bedingung
     * lautet deshalb "Quantity jetzt <= 0" und nicht "ein Bissen":
     *
     *   16 E4: "Andernfalls ergaebe jeder Bissen einen leeren Teller: ein
     *           Duplikations-Exploit erster Ordnung."
     *
     * Ein nur teilweise gegessenes Gericht laeuft hier durch und tut nichts.
     * Ein Gericht, das verdirbt, geloescht oder vom Admin entfernt wird,
     * erreicht diese Methode gar nicht - genau das ist der Unterschied zu
     * EEDelete (16 E4).
     */
    static void OnFoodConsumed(notnull ItemBase dish, float amount, PlayerBase consumer)
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        // 16 §7, "Gericht nur teilweise gegessen": keine Rueckgabe. Die
        // Bedingung ist Quantity <= 0, nicht "ein Bissen".
        if (dish.HasQuantity() && dish.GetQuantity() > EMPTY_EPSILON)
            return;

        // Erst die Bindung holen, dann SOFORT entwerten - vor jeder Erzeugung.
        // Scheitert die Rueckgabe danach, ist das laut 16 §7 "Rueckgabe
        // entfaellt mit WARN"; ein zweiter Versuch waere die schlechtere
        // Antwort, weil er bei jedem weiteren OnConsume erneut liefe.
        ChefZ_Sym returnClass = ResolveReturnFor(dish);
        MarkReturned(dish);

        ItemBase returned = null;
        if (ChefZ_SymbolTable.IsValid(returnClass))
        {
            string returnErr;
            returned = ReturnEmpty(returnClass, consumer, dish.GetPosition(), returnErr);
        }

        RaiseFoodConsumed(dish, consumer, returned, amount);
    }

    /**
     * Was gibt dieses Gericht zurueck (16 §6, E3)?
     *
     * Zwei Quellen, in dieser Reihenfolge:
     *
     *   1. die Itemvariable m_ChefZ_ReturnContainer. Sie wurde beim Servieren
     *      gesetzt und ist persistiert - das Gericht kann gehandelt, gelagert
     *      und ueber einen Serverneustart getragen worden sein (16 E3: "das
     *      Rezept ist beim Verzehr nicht mehr bekannt").
     *   2. die Klassenbindung aus CfgChefZIngredients (16 E3, zweiter Absatz).
     *      Sie gilt auch fuer Gerichte, die nie ueber ein ChefZ-Rezept
     *      entstanden sind - etwa per Admin-Spawn.
     *
     * "AUTO" ist an dieser Stelle NICHT aufloesbar und ergibt INVALID: es
     * bedeutet "der Behaelter, der benutzt wurde", und beim Essen weiss das
     * niemand mehr. Aufgeloest wird es beim SERVIEREN, wo der benutzte
     * Behaelter bekannt ist (ChefZ_ContainerRegistry.ResolveReturnClass).
     */
    static ChefZ_Sym ResolveReturnFor(notnull ItemBase dish)
    {
        string cls = "";

        ChefZ_Edible_Base edible = ChefZ_Edible_Base.Cast(dish);
        if (edible)
            cls = edible.ChefZ_GetReturnContainer();

        if (cls == RETURN_DONE)
            return ChefZ_SymbolTable.INVALID;

        if (cls == "")
        {
            ChefZ_IngredientInfo info = ChefZ_IngredientManager.Get().ResolveByName(dish.GetType());
            if (info && ChefZ_SymbolTable.IsValid(info.returnContainer))
                cls = ChefZ_SymbolTable.Name(info.returnContainer);
        }

        if (cls == "" || cls == RETURN_DONE)
            return ChefZ_SymbolTable.INVALID;

        if (cls == ChefZ_ContainerDef.AUTO)
        {
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.CONTAIN, "container.auto." + dish.GetType(), "\"" + dish.GetType() + "\" traegt als Rueckgabe woertlich \"" + ChefZ_ContainerDef.AUTO + "\". Das ist eine Aufloesungsregel fuer den " + "Moment des Servierens (16 §4) und beim Verzehr nicht mehr aufloesbar - " + "es kommt nichts zurueck. Ein Rezept setzt die aufgeloeste Klasse am " + "Item; eine Klassenbindung in CfgChefZIngredients muss eine echte " + "Klasse nennen.");
            return ChefZ_SymbolTable.INVALID;
        }

        return ChefZ_SymbolTable.Intern(cls);
    }

    //==========================================================================
    // Nahtstelle zu S16 (15 §4)
    //==========================================================================

    /**
     * Traegt die verfuegbaren Behaelterklassen in eine Entnahmeanfrage ein.
     *
     * Der ChefZ_PortionManager sucht NICHT selbst - er hat kein Inventar
     * (ChefZ_PortionRequest, Feldkommentar). Diese Methode ist die einzige
     * Stelle, an der die beiden Teilsysteme sich beruehren, und sie ist
     * bewusst einseitig: das Portionssystem weiss von Behaeltern nur, dass
     * seine Liste gefuellt wird.
     *
     * Sie laeuft auch clientseitig, aus ActionCondition. Deshalb steigt sie
     * so frueh wie moeglich aus - der weitaus haeufigste Ausgang ist "diese
     * Spec verlangt gar keinen Behaelter".
     */
    static void FillRequest(notnull ChefZ_PortionRequest req, PlayerBase actor, ItemBase device)
    {
        ChefZ_PortionSpec spec;
        if (!ChefZ_PortionManager.Get().GetSpecForBulk(req.sourceClass, spec))
            return;
        if (!spec.RequiresContainer())
            return;

        ChefZ_ContainerRegistry reg = ChefZ_ContainerRegistry.Get();
        if (!reg.HasAnyContainer())
            return;

        // Unbekannte Kategorie: hier stillschweigend nichts eintragen. Die
        // Meldung dazu gehoert dem Startaudit (16 §7, "WARN EINMAL BEIM
        // LADEN") - eine Warnung an dieser Stelle liefe bei jedem
        // Zielwechsel des Fadenkreuzes.
        if (!reg.CategoryExists(spec.containerCategorySym))
            return;

        array<ItemBase> found = new array<ItemBase>();
        FindCandidates(spec.containerCategorySym, actor, device, found);

        for (int i = 0; i < found.Count(); i++)
        {
            ItemBase item = found.Get(i);
            if (!item)
                continue;

            // Lookup und NICHT Intern: jede echte Behaelterklasse ist beim
            // Build interniert worden. Intern legte hier fuer jede
            // angeschaute Klasse einen Eintrag an - eine Tabelle, die mit der
            // Spielzeit waechst (dieselbe Ueberlegung wie in
            // ChefZ_PortionedFood_Base.ChefZ_BuildPortionRequest).
            req.AddContainer(ChefZ_SymbolTable.Lookup(item.GetType()));
        }
    }

    //==========================================================================
    // Benutzbarkeit
    //==========================================================================

    /**
     * Ist dieser Behaelter frei - die ALLGEMEINE Antwort.
     *
     * "Frei" heisst: kein Cargo drin und keine Menge drin. Ein Topf mit
     * Wasser ist keine leere Schuessel, und ein Kochtopf mit Fleisch erst
     * recht nicht.
     *
     * Diese Methode ruft ausdruecklich NICHT
     * ChefZ_Container_Base.ChefZ_IsEmpty() - das waere eine Endlosschleife,
     * weil dessen Vorgabe hierher zeigt. Die Fallunterscheidung steht in
     * IsUsable().
     */
    static bool IsEmpty(notnull ItemBase item)
    {
        if (item.HasQuantity() && item.GetQuantity() > EMPTY_EPSILON)
            return false;

        GameInventory inv = item.GetInventory();
        if (!inv)
            return true;

        CargoBase cargo = inv.GetCargo();
        if (cargo && cargo.GetItemCount() > 0)
            return false;

        return true;
    }

    /**
     * Kommt dieses Item als Behaelter in Frage?
     *
     * Ruiniert scheidet aus - dieselbe Linie wie CCTNonRuined an der
     * Entnahmeaktion: aus einem zerstoerten Teller wird nicht serviert.
     */
    static bool IsUsable(notnull ItemBase item)
    {
        if (item.IsSetForDeletion())
            return false;
        if (item.IsRuined())
            return false;

        ChefZ_Container_Base container = ChefZ_Container_Base.Cast(item);
        if (container)
            return container.ChefZ_IsEmpty();

        return IsEmpty(item);
    }

    //==========================================================================
    // Innereien - Sammeln
    //==========================================================================

    private static void CollectHands(notnull PlayerBase actor, notnull array<ItemBase> outItems)
    {
        HumanInventory hands = actor.GetHumanInventory();
        if (!hands)
            return;

        ItemBase inHands = ItemBase.Cast(hands.GetEntityInHands());
        if (inHands)
            outItems.Insert(inHands);
    }

    /**
     * Das gesamte Spielerinventar, eine Ebene wie zehn.
     *
     * EnumerateInventory(PREORDER) liefert auch das Handitem mit; Dubletten
     * faengt AppendStage ab. Das ist billiger, als hier eine Sonderregel zu
     * fuehren - und robuster: welche Traversierung was genau einschliesst,
     * ist Engineverhalten und nichts, worauf man eine Regel bauen sollte.
     */
    private static void CollectInventory(notnull PlayerBase actor, notnull array<ItemBase> outItems)
    {
        GameInventory inv = actor.GetInventory();
        if (!inv)
            return;

        array<EntityAI> entities = new array<EntityAI>();
        if (!inv.EnumerateInventory(InventoryTraversalType.PREORDER, entities))
            return;

        for (int i = 0; i < entities.Count(); i++)
        {
            ItemBase item = ItemBase.Cast(entities.Get(i));
            if (item)
                outItems.Insert(item);
        }
    }

    /**
     * Kisten und Faesser im Umkreis (16 E5, NEARBY_CARGO).
     *
     * Ausdruecklich OPTIONAL und per Vorgabe AUS: "es wird bei Basen mit
     * vielen Behaeltern teuer und macht die Auswahl fuer den Spieler
     * undurchsichtig" (16 E5). Wer es einschaltet, weiss das.
     *
     * Gesucht wird EINE Ebene tief - das Cargo der Objekte im Umkreis, nicht
     * das Cargo im Cargo. Eine Schuessel in einer Kiste in einem Zelt zu
     * finden waere eine Suche ohne natuerliches Ende, und der Spieler saehe
     * nicht mehr, woher sein Teller kam.
     *
     * Das Kochgeraet selbst wird uebersprungen: was IM Topf liegt, ist
     * Zutat, nicht Behaelter (01 V13, dieselbe Regel wie im
     * ChefZ_FactCollector).
     */
    private static void CollectNearbyCargo(PlayerBase actor, ItemBase device, notnull array<ItemBase> outItems)
    {
        if (!g_Game)
            return;

        vector center = vector.Zero;
        if (device)
            center = device.GetPosition();
        else if (actor)
            center = actor.GetPosition();
        else
            return;

        float radius = SearchRadius();
        if (radius <= 0.0)
            return;

        array<Object>    objects = new array<Object>();
        array<CargoBase> proxies = new array<CargoBase>();
        g_Game.GetObjectsAtPosition3D(center, radius, objects, proxies);

        for (int i = 0; i < objects.Count(); i++)
        {
            ItemBase holder = ItemBase.Cast(objects.Get(i));
            if (!holder)
                continue;
            if (device && holder == device)
                continue;

            GameInventory inv = holder.GetInventory();
            if (!inv)
                continue;

            CargoBase cargo = inv.GetCargo();
            if (!cargo)
                continue;

            int count = cargo.GetItemCount();
            for (int k = 0; k < count; k++)
            {
                ItemBase item = ItemBase.Cast(cargo.GetItem(k));
                if (item)
                    outItems.Insert(item);
            }
        }
    }

    //==========================================================================
    // Innereien - Ordnen (16 E5)
    //==========================================================================

    /**
     * Eine Fundstufe pruefen, sortieren und anhaengen.
     *
     * Sortiert wird INNERHALB der Stufe, nie ueber Stufen hinweg: die
     * Stufenreihenfolge ist die staerkere Aussage ("was in der Hand ist, wird
     * zuerst genommen", 16 E5). Ein makelloser Teller im Rucksack darf einen
     * angeschlagenen in der Hand nicht ueberholen.
     *
     * Innerhalb der Stufe: hoechste Gesundheit, dann Klassenname - also
     * deterministisch und nicht nach Slot-Zufall.
     */
    private static void AppendStage(notnull array<ItemBase> stage, ChefZ_Sym category, int stageBit, notnull array<ItemBase> outCandidates, int limit)
    {
        ChefZ_ContainerRegistry reg = ChefZ_ContainerRegistry.Get();

        array<ItemBase> accepted = new array<ItemBase>();

        for (int i = 0; i < stage.Count(); i++)
        {
            ItemBase item = stage.Get(i);
            if (!item)
                continue;
            if (outCandidates.Find(item) >= 0 || accepted.Find(item) >= 0)
                continue;

            ChefZ_Sym classSym = ChefZ_SymbolTable.Lookup(item.GetType());
            if (!reg.IsContainerOfCategory(classSym, category))
                continue;

            // Die Gegenprobe zum ODER in GetSearchScope(): ein Behaelter, der
            // nur in der Hand zaehlen soll, wird in der Kiste zwar angefasst,
            // aber nicht genommen. Ohne sie waere das Bitfeld an der Klasse
            // wirkungslos, sobald ein einziges Mitglied grosszuegiger ist.
            if (!ChefZ_ContainerScope.Has(reg.GetSearchScopeForClass(classSym), stageBit))
                continue;

            if (!IsUsable(item))
                continue;

            accepted.Insert(item);
        }

        SortByHealthThenName(accepted);

        for (int k = 0; k < accepted.Count(); k++)
        {
            if (outCandidates.Count() >= limit)
                return;
            outCandidates.Insert(accepted.Get(k));
        }
    }

    /**
     * Einfuegesortierung: hoechste Gesundheit zuerst, bei Gleichstand der
     * Klassenname aufsteigend (16 §5).
     *
     * Einfuegesortierung und kein Quicksort: die Listen sind kurz (eine
     * Handvoll Teller), und eine stabile Sortierung ist hier die Zusage -
     * zwei gleichwertige Behaelter behalten die Reihenfolge, in der sie
     * gefunden wurden, und damit bleibt die Antwort ueber zwei Aufrufe
     * hinweg dieselbe.
     */
    private static void SortByHealthThenName(notnull array<ItemBase> items)
    {
        for (int i = 1; i < items.Count(); i++)
        {
            ItemBase current = items.Get(i);
            int j = i - 1;

            while (j >= 0 && IsBetter(current, items.Get(j)))
            {
                items.Set(j + 1, items.Get(j));
                j--;
            }
            items.Set(j + 1, current);
        }
    }

    private static bool IsBetter(notnull ItemBase a, notnull ItemBase b)
    {
        float ha = a.GetHealth01("", "");
        float hb = b.GetHealth01("", "");

        if (ha > hb)
            return true;
        if (ha < hb)
            return false;

        // Gleicher Zustand: der Klassenname entscheidet. Er ist auf Client und
        // Server derselbe und aendert sich nie - anders als ein Slotindex.
        //
        // Ueber ChefZ_StringOrder und nicht ueber den Operator: der ordinale
        // Vergleich ist dort eine ZUSAGE (zeichenweise ueber ToAscii), waehrend
        // die Vergleichsregel eingebauter Stringoperatoren in Enforce nirgends
        // festgeschrieben ist. Client und Server muessen hier dieselbe Antwort
        // geben, sonst zeigt die Aktion einen anderen Teller an als der Server
        // nimmt.
        return ChefZ_StringOrder.Less(a.GetType(), b.GetType());
    }

    //==========================================================================
    // Innereien - Erzeugen
    //==========================================================================

    /**
     * Haende -> Inventar -> Boden am Spieler -> Boden an der Ersatzposition.
     *
     * Dieselbe Reihenfolge wie bei der Portionsentnahme (15 §4) und aus
     * demselben Grund: wer gerade aufgegessen hat, hat die Hand frei, und ein
     * Teller im Rucksack waere ein zusaetzlicher Handgriff.
     */
    private static ItemBase SpawnFor(string cls, PlayerBase consumer, vector fallbackPos, out string err)
    {
        err = "";

        // 16 §7, letzte Zeile: "Spieler stirbt im selben Tick -> ReturnEmpty
        // prueft consumer != null && consumer.IsAlive(); sonst Boden an der
        // Leichenposition."
        if (consumer && consumer.IsAlive())
        {
            HumanInventory hands = consumer.GetHumanInventory();
            if (hands && !hands.GetEntityInHands())
            {
                ItemBase inHands = ItemBase.Cast(hands.CreateInHands(cls));
                if (inHands)
                    return inHands;
            }

            GameInventory inv = consumer.GetInventory();
            if (inv)
            {
                ItemBase inInv = ItemBase.Cast(inv.CreateInInventory(cls));
                if (inInv)
                    return inInv;
            }

            // 16 §7: "Haende belegt UND Inventar voll -> Behaelter faellt zu
            // Boden. INFO." Kein Fehler - nur nie stillschweigend verwerfen.
            ItemBase onGround = ItemBase.Cast(consumer.SpawnEntityOnGroundRaycastDispersed(cls));
            if (onGround)
            {
                ChefZ_Log.Once(ChefZ_LogLevel.INFO, ChefZ_LogChannel.CONTAIN, "container.ground." + cls, "Der leere Behaelter \"" + cls + "\" ist zu Boden gefallen: Haende " + "belegt und Inventar voll. Das ist kein Fehler; er liegt vor dem " + "Spieler.");
                return onGround;
            }
        }

        // Kein lebender Verzehrer (Leiche, Adminwerkzeug, Automatik): dorthin,
        // wo das Gericht war. ECE_PLACE_ON_SURFACE, damit nichts im Boden
        // versinkt - dieselbe Flagge, die ChefZ_ItemTransform.Create benutzt.
        Object obj = g_Game.CreateObjectEx(cls, fallbackPos, ECE_PLACE_ON_SURFACE);
        ItemBase spawned = ItemBase.Cast(obj);
        if (spawned)
            return spawned;

        if (obj)
            g_Game.ObjectDelete(obj);

        err = "weder Haende noch Inventar noch Boden haben \"" + cls + "\" aufgenommen";
        return null;
    }

    //! Die Bindung entwerten - siehe RETURN_DONE.
    private static void MarkReturned(notnull ItemBase dish)
    {
        ChefZ_ItemStateComponent.SetReturnContainer(dish, RETURN_DONE);
    }

    //==========================================================================
    // Ereignisse (17 §4)
    //==========================================================================

    /**
     * ChefZ_OnFoodConsumed - "Gericht VOLLSTAENDIG verzehrt" (17 §4).
     *
     * Nicht stornierbar: das Essen ist bereits geschehen. XP-tauglich, und
     * deshalb steht hier auch der Fortschrittsbericht - 17 E7 und Regel
     * §10.6: gemeldet wird ausschliesslich ein ABSCHLUSS, nie ein Bissen.
     *
     * Zuerst HasSubscribers(), dann erst die Nutzlast (17 E2). Ohne
     * Comp-Module kostet die Zeile einen Map-Zugriff und sonst nichts.
     */
    private static void RaiseFoodConsumed(notnull ItemBase dish, PlayerBase consumer, ItemBase returned, float amount)
    {
        ChefZ_EventBus bus = ChefZ_EventBus.Get();

        bool anyListener = bus.HasSubscribers(ChefZ_EventNames.FOOD_CONSUMED) || ChefZ_ProgressRegistry.HasSinks();
        if (!anyListener)
            return;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.FOOD_CONSUMED);
        args.identityId   = ActorId(consumer);
        args.subjectClass = ChefZ_SymbolTable.Intern(dish.GetType());
        // Ueber eine int-Zwischenvariable: die Nutzlast fuehrt amount als
        // int (17 §3.1), Math.Round liefert in Enforce einen float.
        int rounded = Math.Round(amount);
        args.amount       = rounded;

        // "containerReturned im Payload" (16 §3.2). Es steht in
        // producedClasses, weil genau das entstanden ist - eine eigene
        // Nutzlastzeile fuer einen Fall waere eine Zeile mehr auf jedem
        // Ereignis des ganzen Core.
        if (returned)
            args.AddProduced(ChefZ_SymbolTable.Intern(returned.GetType()));

        int low  = 0;
        int high = 0;
        dish.GetNetworkID(low, high);
        args.SetSubjectNetId(low, high);

        ChefZ_ProgressRegistry.Report(ChefZ_ProgressKind.CONSUME, args);

        // RaiseKeep und nicht Raise: der Fortschrittsbericht oben benutzt
        // dieselbe Nutzlast, und Raise() gibt sie in den Pool zurueck. Die
        // Freigabe passiert deshalb hier, einmal, am Ende.
        bus.RaiseKeep(args);
        bus.Release(args);
    }

    //! ChefZ_OnContainerReturned - "Leerbehaelter zurueckgegeben" (17 §4).
    //! Nicht stornierbar, nicht XP-tauglich: es meldet einen Vorgang, keine
    //! Leistung.
    private static void RaiseContainerReturned(notnull ItemBase container, PlayerBase consumer)
    {
        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (!bus.HasSubscribers(ChefZ_EventNames.CONTAINER_RETURNED))
            return;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.CONTAINER_RETURNED);
        args.identityId   = ActorId(consumer);
        args.subjectClass = ChefZ_SymbolTable.Intern(container.GetType());

        int low  = 0;
        int high = 0;
        container.GetNetworkID(low, high);
        args.SetSubjectNetId(low, high);

        bus.Raise(args);
    }

    //! 0, wenn kein Spieler beteiligt ist oder er keine Identitaet hat.
    //! ChefZ_CapabilityGate blockiert bei 0 nichts (17 §3.3).
    private static int ActorId(PlayerBase player)
    {
        if (!player)
            return 0;

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return 0;

        return identity.GetPlayerId();
    }

    //==========================================================================
    // Regler und Diagnose
    //==========================================================================

    //! Umkreis der Umgebungssuche in Metern. Aus Core.json, nicht aus einer
    //! Konstanten: wie weit "in Reichweite" reicht, ist eine Spielgefuehls-
    //! frage und die Antwort darauf eine Zahl (16 E5).
    private static float SearchRadius()
    {
        ChefZ_CoreSettingsDef settings = ChefZ_ConfigManager.Get().GetSettings();
        if (!settings)
            return 3.0;
        return settings.containerSearchRadius;
    }

    //! Deckel fuer die Fundliste. Er schuetzt den ActionCondition-Pfad auf
    //! einer Basis mit hundert Tellern - gebraucht wird ohnehin nur der erste
    //! Eintrag.
    private static int MaxCandidates()
    {
        ChefZ_CoreSettingsDef settings = ChefZ_ConfigManager.Get().GetSettings();
        if (!settings)
            return 32;
        return settings.maxContainerCandidates;
    }

    private static string BoolWord(bool value, string yes, string no)
    {
        if (value)
            return yes;
        return no;
    }

    static int GetConsumedCount()     { return s_CountConsumed; }
    static int GetReturnedCount()     { return s_CountReturned; }
    static int GetReturnFailedCount() { return s_CountReturnFailed; }

    static void ResetCounters()
    {
        s_CountConsumed     = 0;
        s_CountReturned     = 0;
        s_CountReturnFailed = 0;
    }
}
