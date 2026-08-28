//==============================================================================
// ChefZ_FactCollector - die EINZIGE Stelle im Core mit Entity-Zugriff
//
// Entwurf: 05 §3.4 (Schnittstelle), 05 §4 (Datenfluss), 05 §6 (Einheiten),
// 05 §7 (Fehlerverhalten), 05 E1 (Fakten ohne EntityAI), 05 E3 (nicht
// deklarierte Klassen bleiben adressierbar), 05 E4 (effektive Tags hier, nicht
// im Matcher), 01 V13 (Pot und Cauldron sind selbst Edible_Base).
//
// ---------------------------------------------------------------------------
// Die Regel dieser Datei
// ---------------------------------------------------------------------------
// Ab hier nach rechts gibt es keine Entities mehr. Der Collector liest ein
// ItemBase EINMAL aus und schreibt reine Daten in ein ChefZ_ItemFacts; alles
// danach - Matcher, Priorisierung, Verbrauchsplanung - arbeitet ausschliesslich
// auf diesen Daten (05 §4).
//
// Er liest AUSSCHLIESSLICH. Kein Setter, kein Delete, kein AddQuantity, kein
// SetSynchDirty. Wer etwas veraendert, heisst ChefZ_Applicator und kommt in S7.
//
// ---------------------------------------------------------------------------
// Zwei parallele Arrays, ein Handle (05 §3.4)
// ---------------------------------------------------------------------------
//   snapshot.items[i].handle == Index in outEntities
// Diese Zuordnung existiert genau hier und nirgendwo sonst. Sie ueberlebt
// SortStable(), weil der Handle im Datensatz steht und nicht seine Position
// ist.
//
// ---------------------------------------------------------------------------
// Das Gefaess ist keine Zutat (01 V13)
// ---------------------------------------------------------------------------
// Pot und Cauldron erben von Bottle_Base : Edible_Base und werden von
// Cooking.CookWithEquipment mit ProcessItemToCook(equip, equip, ...) selbst
// mitgekocht. Wuerde der Sammler das Gefaess mitprojizieren, koennte ein
// Rezept den Topf als Zutat binden und der Applicator ihn spaeter verbrauchen.
// Deshalb sammelt CollectFromCargo ausschliesslich das CARGO und ueberspringt
// zusaetzlich ausdruecklich das Gefaess selbst.
//
// ---------------------------------------------------------------------------
// Was hier noch NICHT steht
// ---------------------------------------------------------------------------
// 05 §3.4 nennt ausserdem CollectContext() und CollectToolCategories().
//   - CollectContext ist seit S7 da (siehe unten). Der ChefZ_CookContext kam
//     mit S6 (08 §2), der Geraetedeskriptor mit S7 (10 §4).
//   - CollectToolCategories ist seit S14 da, als CollectToolGroups() (siehe
//     unten). Sie fuellt ChefZ_ProcessContext.availableToolGroups aus der
//     ChefZ_ToolRegistry.
//
//     Der KOCHKONTEXT (ChefZ_CookContext.availableToolGroups) bleibt davon
//     ausdruecklich unberuehrt: ein kochender Topf hat keine Hand (08 §3),
//     und wessen Messer in Reichweite liegt, waere eine Antwort, die sich
//     zwischen zwei Ticks aendert, ohne dass jemand etwas getan haette. Ein
//     Rezept mit requiredToolGroups bindet deshalb weiterhin nicht - die
//     sichere Richtung.
//
//     Der HANDELNDE SPIELER ist davon zu trennen und wird seit S19 sehr wohl
//     gefuehrt - nur nicht hier. Siehe die Anmerkung an CollectContext().
// Beide haengen an derselben Stelle, damit die Regel "genau eine Datei fasst
// Entities an" gilt.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_FactCollector
{
    //! Neutraler Sauberkeitswert fuer Klassen, die varCleanness gar nicht
    //! fuehren. 1.0 und nicht 0.0: "nichts bekannt" darf ein Item nicht
    //! spaeter (V2, Hygiene) als schmutzig aussortieren.
    static const float CLEANNESS_UNKNOWN = 1.0;

    /**
     * varCleannessMax je Klasse, einmal aus der Config gelesen.
     *
     * Ohne den Zwischenspeicher liefe je Item und je Auswertung ein
     * Config-Zugriff - und Konfigurationszugriffe sind das Teuerste, was in
     * diesem Pfad vorkommt. Der Schluessel ist das Klassensymbol, der Bestand
     * ist durch die Zahl der Itemklassen im Spiel begrenzt.
     */
    private static ref map<int, int> s_CleannessMax;

    //! Fluessigkeitstyp -> Symbol des Fluessigkeits-Klassennamens. Gleiche
    //! Begruendung, gleiche Lebensdauer.
    private static ref map<int, int> s_LiquidSyms;

    //==========================================================================
    // Der Kontext einer Auswertung (05 §3.4, 10 §4)
    //==========================================================================

    /**
     * Fuellt ein ChefZ_CookContext aus Geraet und Deskriptor.
     *
     * @param device  das Gefaess. Wird ausschliesslich GELESEN.
     * @param desc    was ChefZ ueber die Geraeteklasse weiss (10 §4). Er
     *                bringt Kategorien, Portionszahl und Qualitaetsmodifikator
     *                mit; sie stammen aus der Registry und nicht vom Item.
     * @param cookingMethod  CookingMethodType dieses Ticks, von Vanilla
     *                erfragt (10 E3).
     * @param ctx     wird GELEERT und gefuellt. notnull und nicht out: der
     *                Aufrufer bringt seinen wiederverwendeten Puffer mit
     *                (10 §7), und ein Feld als out-Parameter ist in Enforce
     *                nicht zugesichert (siehe ChefZ_TextList.SymbolsOf).
     *
     * elapsedSec bleibt 0. Die aufgelaufene Zeit einer TIMED-Kochsitzung
     * gehoert der Sitzung (10 §6) und wird vom Adapter unmittelbar vor der
     * Abschlusspruefung eingetragen. Sie hier zu raten hiesse, sie zweimal zu
     * fuehren.
     *
     * actorIdentityId bleibt 0, und das ist SEIT S19 eine Arbeitsteilung und
     * kein Verzicht mehr.
     *
     * 08 §3 hat recht: ein kochender Topf hat keinen Besitzer. Es gibt am
     * GERAET nichts abzulesen, woraus sich ein handelnder Spieler ergaebe -
     * Vanilla fuehrt dort keine Kennung, und ChefZ darf keine anlegen (00 §5
     * fuehrt Pot, FryingPan, Cauldron und ItemBase in der geschlossenen Liste
     * der NICHT gemoddeten Klassen).
     *
     * Wer gehandelt hat, ist deshalb keine Eigenschaft des Gefaesses, sondern
     * eine BEOBACHTUNG ueber die Zeit: wer stand daneben, als der Bestand
     * wuchs. Diese Beobachtung fuehrt die Kochsitzung, die Sitzung gehoert
     * dem Adapter (10 §7), und der Adapter stempelt den Wert unmittelbar nach
     * diesem Aufruf in denselben Kontext
     * (ChefZ_CookingDeviceAdapter.BuildContextFrom). Die Regel begruendet
     * ChefZ_CookActor.
     *
     * Diese Datei bleibt damit, was sie ist: sie liest EIN Gefaess EINMAL und
     * weiss nichts ueber seine Vorgeschichte. Haette sie den Spieler zu
     * ermitteln, muesste sie die Welt absuchen - und die Zusage "der Sammler
     * liest ausschliesslich das uebergebene Item" waere dahin.
     */
    static bool CollectContext(notnull ItemBase device, notnull ChefZ_DeviceDescriptor desc, int cookingMethod, notnull ChefZ_CookContext ctx)
    {
        ctx.Reset();

        ctx.deviceClass     = desc.deviceClass;
        ctx.deviceRootClass = desc.deviceRootClass;
        for (int i = 0; i < desc.deviceCategories.Count(); i++)
            ctx.AddDeviceCategory(desc.deviceCategories.Get(i));

        ctx.method            = ChefZ_CookingHook.MapVanillaMethod(cookingMethod);
        ctx.deviceTemperature = device.GetTemperature();
        ctx.portionCapacity   = desc.portionCapacity;
        ctx.qualityModifier   = desc.qualityModifier;

        // Fluessigkeit: Typ UND Menge, und beides nur, wenn das Gefaess
        // ueberhaupt eines fuehrt. FryingPan ist kein Fluessigkeitsbehaelter
        // (01 V13), Pot und Cauldron sind es.
        if (device.IsLiquidContainer())
        {
            int liquidType = device.GetLiquidType();
            if (liquidType != LIQUID_NONE)
            {
                ctx.liquidType     = LiquidSym(liquidType);
                ctx.liquidQuantity = device.GetQuantity();
            }
        }

        // availableToolGroups bleibt leer, und das ist seit S14 eine
        // ENTSCHEIDUNG und kein offener Punkt mehr: Werkzeuge gehoeren dem
        // Verarbeitungspfad (ChefZ_ProcessContext), nicht der Kochschleife.
        // Siehe Dateikopf.

        return true;
    }

    //==========================================================================
    // Cargo eines Gefaesses
    //==========================================================================

    /**
     * Sammelt das Cargo eines Gefaesses in zwei parallele Listen.
     *
     * @param container   Gefaess. Wird NIE als Zutat projiziert (01 V13).
     * @param snapshot    entity-freie Fakten fuer den Matcher. Wird angelegt,
     *                    wenn null, und in jedem Fall geleert.
     * @param outEntities echte Instanzen fuer den spaeteren Verbrauch.
     *                    outEntities[facts.handle] ist das Item zu den Fakten.
     *
     * Der Snapshot wird EINMAL je Auswertung gebaut und danach nur gelesen
     * (05 §4). Wer denselben Snapshot wiederverwendet, spart ab dem zweiten
     * Durchlauf jede Allokation.
     */
    static void CollectFromCargo(notnull ItemBase container, out ChefZ_FactSnapshot snapshot, out array<ItemBase> outEntities)
    {
        if (!snapshot)
            snapshot = new ChefZ_FactSnapshot();
        snapshot.Clear();

        if (!outEntities)
            outEntities = new array<ItemBase>();
        outEntities.Clear();

        GameInventory inventory = container.GetInventory();
        if (!inventory)
            return;

        CargoBase cargo = inventory.GetCargo();
        if (!cargo)
            return;

        int count = cargo.GetItemCount();
        for (int i = 0; i < count; i++)
        {
            ItemBase item = ItemBase.Cast(cargo.GetItem(i));

            // 01 V13: das Gefaess ist keine Zutat. Der Vergleich ist billig
            // und deckt auch den Fall ab, in dem ein Kochgeraet sich selbst
            // im eigenen Cargo fuehrt.
            if (item == container)
                continue;

            if (!IsCollectable(item))
                continue;

            int handle = outEntities.Count();
            ChefZ_ItemFacts facts = snapshot.Acquire();

            if (!CollectSingle(item, handle, facts))
            {
                // Kann nach IsCollectable nicht mehr passieren. Wenn doch,
                // darf der halbe Datensatz nicht stehenbleiben: er wuerde die
                // beiden Listen gegeneinander verschieben.
                snapshot.DiscardLast();
                continue;
            }

            outEntities.Insert(item);
        }

        LogIfDebug(container, snapshot);
    }

    //==========================================================================
    // Ein einzelnes Item
    //==========================================================================

    /**
     * Liest ein Item vollstaendig aus.
     *
     * @param handle  Index in die Entity-Liste des Aufrufers. -1 ist erlaubt
     *                (Vorschau, Test) und heisst "zu diesen Fakten gibt es
     *                keine Entity".
     * @return false, wenn das Item nicht gesammelt werden konnte. facts ist
     *         dann zurueckgesetzt und nicht halb gefuellt.
     */
    static bool CollectSingle(notnull ItemBase item, int handle, out ChefZ_ItemFacts facts)
    {
        if (!facts)
            facts = new ChefZ_ItemFacts();
        facts.Reset();

        if (!IsCollectable(item))
            return false;

        string type = item.GetType();
        if (type == "")
        {
            // 05 §7: leerer Typ -> uebersprungen, WARN. Einmalig, weil eine
            // kaputte Klasse sonst je Kochtick eine Zeile schriebe.
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.COOK, "facts.type.empty", "Ein Item im Cargo liefert einen leeren Klassennamen und wird uebersprungen. " + "Das deutet auf ein Item hin, das gerade zerstoert wird, oder auf eine " + "kaputte Klassendefinition.");
            return false;
        }

        facts.handle = handle;

        // 05 E3: die Fakten bekommen IMMER ein gueltiges classSym - auch fuer
        // voellig unbekannte Klassen. Sonst muesste jedes Vanilla-Item, das je
        // in einem Rezept vorkommt, erst registriert werden, und eine
        // vergessene Registrierung waere stumm falsch statt offensichtlich
        // richtig.
        //
        // Intern() und nicht Lookup(): der Wertevorrat ist durch die Zahl der
        // Itemklassen im Spiel begrenzt, Symbole sind reine Laufzeitwerte und
        // werden nie persistiert oder uebertragen (03 E2).
        facts.classSym = ChefZ_SymbolTable.Intern(type);

        ChefZ_IngredientInfo info = ChefZ_IngredientManager.Get().Resolve(facts.classSym);

        ApplyIngredientInfo(facts, info);
        ApplyVanillaFood(item, facts);

        // ZWINGEND nach ApplyVanillaFood: Schritt 3 der Projektionsregel
        // (06 §3) liest facts.vanillaFoodStage. Und zwingend nach
        // ApplyIngredientInfo, weil Schritt 2 den defaultState der Klasse
        // braucht.
        ApplyChefZState(item, facts);

        ApplyPhysical(item, facts);
        ApplyLiquid(item, facts);
        ApplyUnits(facts, info);

        return true;
    }

    //--------------------------------------------------------------------------

    /**
     * 05 §7: eine unbekannte Klasse ist KEIN Fehler und wird NICHT geloggt.
     * Das ist der Normalfall fuer 99 % aller Vanilla-Klassen.
     */
    private static void ApplyIngredientInfo(notnull ChefZ_ItemFacts facts, ChefZ_IngredientInfo info)
    {
        if (!info)
        {
            facts.isChefZManaged = false;

            // Closure und Tags bleiben leer - Kategorie- und Tag-Selektoren
            // matchen also nie, class-Selektoren sehr wohl.
            //
            // Einheit und Umrechnung bekommen trotzdem einen Wert: ein volles
            // Item ist ein Stueck. Ohne ihn koennte ein Rezept ein
            // undeklariertes Vanilla-Item zwar per class ansprechen, aber
            // keine Menge davon verlangen.
            facts.quantityUnit      = ChefZ_IngredientManager.DefaultUnitSym();
            return;
        }

        facts.isChefZManaged = true;
        facts.closure.CopyFrom(info.closure);

        // 05 E4: die EFFEKTIVEN Tags entstehen hier, nicht im Matcher. Das
        // sind die Tags der KLASSE; die Tags aus dem Zustand (06 §4.1,
        // implies) kommen seit S9 in ApplyChefZState() dazu, die der
        // Qualitaetsstufe (12 §3) mit S12 ebendort.
        for (int i = 0; i < info.staticTags.Count(); i++)
            facts.AddTag(info.staticTags.Get(i));

        facts.quantityUnit = info.quantityUnit;

        // Der defaultState der Klasse ist Schritt 2 der Projektionsregel
        // (06 §3) und der V1-NORMALFALL: die Klasse IST der Zustand. Die
        // vollstaendige Regel - inklusive Schritt 1 (Variable auf dem Item)
        // und Schritt 3 (Vanilla-FoodStage) - laeuft in ApplyChefZState().
        // Hier steht sie als Vorbelegung, damit ein Item ohne Traegerklasse
        // und ohne FoodStage trotzdem seinen Klassenzustand traegt.
        facts.chefzState = info.defaultState;
    }

    /**
     * Zustand, implizierte Tags, Qualitaet, Frische und Portionen (06 §5,
     * "LESEN IM MATCHER").
     *
     * Der Zustand kommt vollstaendig aus ChefZ_ItemStateComponent.GetState() -
     * die Projektionsregel steht dort und nur dort. Sie hier ein zweites Mal
     * auszuschreiben waere eine zweite Wahrheit, und die zweite waere die,
     * die im Matcher wirkt.
     *
     * Die drei uebrigen Groessen leben auf den ChefZ-Traegerklassen. Fuer
     * jedes andere Item bleiben sie auf ihren neutralen Werten - 05 §7: eine
     * unbekannte Klasse ist KEIN Fehler und wird NICHT geloggt.
     */
    private static void ApplyChefZState(notnull ItemBase item, notnull ChefZ_ItemFacts facts)
    {
        facts.chefzState = ChefZ_ItemStateComponent.GetState(item);

        // 05 E4: die EFFEKTIVEN Tags entstehen hier, nicht im Matcher - aus
        // Klasse (oben) PLUS Zustand (hier) PLUS Qualitaetsstufe (unten).
        if (ChefZ_SymbolTable.IsValid(facts.chefzState))
        {
            array<ChefZ_Sym> implied;
            ChefZ_StateManager.Get().GetImpliedTags(facts.chefzState, implied);
            for (int i = 0; i < implied.Count(); i++)
                facts.AddTag(implied.Get(i));
        }

        ChefZ_ItemStateComponent comp = ChefZ_ItemStateComponent.Of(item);
        if (!comp)
            return;

        facts.chefzQuality = comp.ResolveQualitySym();
        facts.freshness01  = comp.GetFreshness01();
        facts.portions     = comp.GetPortions();

        // S10 (12 E3): grantsTags der Stufe. Das ist der Griff, mit dem ein
        // fremdes Modul auf "premium" filtert, ohne dass ChefZ etwas ueber
        // Handel oder Anzeige wissen muss - und es ist zugleich der Weg, auf
        // dem ein Rezept eine hochwertige Zutat verlangen kann, ohne jede
        // Stufe einzeln aufzuzaehlen.
        if (ChefZ_SymbolTable.IsValid(facts.chefzQuality))
        {
            array<ChefZ_Sym> granted;
            ChefZ_QualityManager.Get().GetGrantedTags(facts.chefzQuality, granted);
            for (int g = 0; g < granted.Count(); g++)
                facts.AddTag(granted.Get(g));
        }
    }

    /**
     * Vanilla-Nahrungsdaten.
     *
     * ACHTUNG, echte Falle: Edible_Base.GetFoodStageType() ruft intern
     * GetFoodStage().GetFoodStageType() OHNE Nullpruefung
     * (Edible_Base.c:531). Ein Edible_Base ohne FoodStage - etwa ein leerer
     * Topf - wuerde damit einen Nullzugriff ausloesen. Deshalb wird hier
     * zuerst GetFoodStage() geholt und geprueft.
     */
    private static void ApplyVanillaFood(notnull ItemBase item, notnull ChefZ_ItemFacts facts)
    {
        Edible_Base edible = Edible_Base.Cast(item);
        if (!edible)
            return;

        facts.isEdible = true;

        FoodStage stage = edible.GetFoodStage();
        if (!stage)
            return;

        // Als int in die Fakten: 1_Core darf den Enum FoodStageType nicht
        // kennen (05 §3.3). Erweitert wird er ohnehin nie - 01 V4.
        facts.vanillaFoodStage = stage.GetFoodStageType();
    }

    private static void ApplyPhysical(notnull ItemBase item, notnull ChefZ_ItemFacts facts)
    {
        facts.quantity    = item.GetQuantity();
        facts.quantityMax = item.GetQuantityMax();
        facts.health01    = item.GetHealth01();
        facts.temperature = item.GetTemperature();
        facts.wetness     = item.GetWet();
        facts.isFrozen    = item.GetIsFrozen();
        facts.cleanness01 = Cleanness01(item, facts.classSym);
    }

    private static void ApplyLiquid(notnull ItemBase item, notnull ChefZ_ItemFacts facts)
    {
        facts.isLiquidContainer = item.IsLiquidContainer();
        if (!facts.isLiquidContainer)
            return;

        int liquidType = item.GetLiquidType();
        if (liquidType == LIQUID_NONE)
            return;

        facts.liquidTypeSym = LiquidSym(liquidType);
    }

    /**
     * Menge in Rezepteinheiten (05 §6).
     *
     *   units = quantity / quantityMax * unitsPerWholeItem
     *
     * Fuer die Standardeinheit ist unitsPerWholeItem gleich 1, ein volles Item
     * also genau eine Einheit. Der Core rechnet NIE zwischen Einheiten um: ein
     * Slot mit Gramm matcht nur Items in Gramm. Eine Umrechnungstabelle haette
     * Content-Wissen ("was ist ein Gramm?"), und in einem Kochmod braucht es
     * sie nicht.
     *
     * Ein Item ohne Mengenvariable (quantityMax <= 0) gilt als EIN volles
     * Item. Alles andere ergaebe eine Null im Zaehler und liesse ein
     * vollstaendig vorhandenes Steak als "nichts davon da" erscheinen.
     */
    private static void ApplyUnits(notnull ChefZ_ItemFacts facts, ChefZ_IngredientInfo info)
    {
        float perWhole = 1.0;
        if (info)
            perWhole = info.unitsPerWholeItem;

        float fraction = 1.0;
        if (facts.quantityMax > 0.0)
            fraction = facts.quantity / facts.quantityMax;

        facts.units = fraction * perWhole;
    }

    //==========================================================================
    // Hilfen
    //==========================================================================

    /**
     * 05 §7: "Item ist null oder zerstoert beim Sammeln - uebersprungen, kein
     * Eintrag im Snapshot, DEBUG."
     *
     * Ein Item, das bereits zur Loeschung vorgemerkt ist, gehoert genauso
     * dazu: es koennte zwischen Auswertung und Anwendung verschwinden, und ein
     * Rezept, das darauf gebaut hat, verbrauchte dann ein Nichts.
     */
    private static bool IsCollectable(ItemBase item)
    {
        if (!item)
            return false;

        if (item.IsDamageDestroyed() || item.IsSetForDeletion())
        {
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.COOK, "Item uebersprungen (zerstoert oder zur Loeschung vorgemerkt): " + item.GetType());
            return false;
        }

        return true;
    }

    /**
     * varCleanness auf 0..1 normiert.
     *
     * Die Obergrenze steht je Klasse in der Config (varCleannessMax) und wird
     * hier gecacht. Fuehrt eine Klasse sie nicht, gibt es nichts zu normieren
     * und der neutrale Wert gilt.
     */
    private static float Cleanness01(notnull ItemBase item, ChefZ_Sym classSym)
    {
        int max = CleannessMaxOf(item, classSym);
        if (max <= 0)
            return CLEANNESS_UNKNOWN;

        float value = item.GetCleanness();
        return Math.Clamp(value / max, 0.0, 1.0);
    }

    private static int CleannessMaxOf(notnull ItemBase item, ChefZ_Sym classSym)
    {
        if (!s_CleannessMax)
            s_CleannessMax = new map<int, int>();

        int cached;
        if (s_CleannessMax.Find(classSym, cached))
            return cached;

        int max = 0;
        if (g_Game)
        {
            string path = ChefZ_IngredientManager.CFG_VEHICLES + " " + item.GetType()
                        + " varCleannessMax";
            if (g_Game.ConfigIsExisting(path))
                max = g_Game.ConfigGetInt(path);
        }

        s_CleannessMax.Set(classSym, max);
        return max;
    }

    /**
     * Symbol des Fluessigkeits-Klassennamens.
     *
     * Intern() aus demselben Grund wie beim Klassennamen: der Wertevorrat ist
     * die Liste der Fluessigkeiten des Spiels, und ein Rezept soll eine
     * Fluessigkeit benennen koennen, ohne dass sie irgendwo registriert wurde.
     */
    static ChefZ_Sym LiquidSym(int liquidType)
    {
        if (!s_LiquidSyms)
            s_LiquidSyms = new map<int, int>();

        int cached;
        if (s_LiquidSyms.Find(liquidType, cached))
            return cached;

        ChefZ_Sym sym = ChefZ_SymbolTable.INVALID;
        string className = Liquid.GetLiquidClassname(liquidType);
        if (className != "")
            sym = ChefZ_SymbolTable.Intern(className);

        s_LiquidSyms.Set(liquidType, sym);
        return sym;
    }

    //==========================================================================
    // Werkzeuggruppen (05 §3.4, 11 E8) - seit S14
    //==========================================================================

    /**
     * Traegt die Werkzeuggruppen der beteiligten Klassen in einen
     * ChefZ_ProcessContext ein.
     *
     * 05 §3.4 hat diese Stelle ausdruecklich reserviert: "CollectToolCategories
     * loest Werkzeuggruppen auf. Die Aufloesungsregel gehoert dem Processing
     * Manager und entsteht mit S10 (11)." Sie steht hier und nicht dort, damit
     * die Regel "genau eine Datei fasst Entities an" gilt - der Processing
     * Manager liegt in 3_Game und kennt weder ItemBase noch PlayerBase.
     *
     * ZWEI Quellen, und beide sind noetig:
     *
     *   inHands  das Werkzeug im ueblichen Sinn - Messer, Saege, Stampfer.
     *   station  die Station SELBST. Ein Fleischwolf ist kein Item in der
     *            Hand, er IST das Werkzeug. Ohne diese zweite Quelle muesste
     *            ein Content-Autor fuer jede Station einen Prozess ohne
     *            Werkzeugbedingung schreiben - und verloere damit die
     *            Moeglichkeit, zwei Muehlen unterschiedlich fein mahlen zu
     *            lassen.
     *
     * Rein lesend, wie alles in dieser Datei. Der Rueckgabewert ist die Zahl
     * der eingetragenen Gruppen, damit ein Aufrufer ohne Liste auskommt.
     */
    static int CollectToolGroups(ItemBase inHands, ItemBase station, notnull ChefZ_ProcessContext ctx)
    {
        int before = ctx.availableToolGroups.Count();

        AddToolGroupsOf(inHands, ctx);
        AddToolGroupsOf(station, ctx);

        return ctx.availableToolGroups.Count() - before;
    }

    private static void AddToolGroupsOf(ItemBase item, notnull ChefZ_ProcessContext ctx)
    {
        if (!item)
            return;

        string type = item.GetType();
        if (type == "")
            return;

        // Lookup() und nicht Intern(): eine Klasse, die in keinem
        // CfgChefZTools-Eintrag vorkommt, ist hier uninteressant - und der
        // Aufruf laeuft bei JEDEM Zielwechsel des Fadenkreuzes. Intern() wuerde
        // die Symboltabelle mit jedem Vanilla-Item fuellen, das ein Spieler je
        // in der Hand hatte.
        ChefZ_Sym classSym = ChefZ_SymbolTable.Lookup(type);
        if (!ChefZ_SymbolTable.IsValid(classSym))
            return;

        ChefZ_ToolRegistry tools = ChefZ_ToolRegistry.Get();
        if (!tools.IsReady())
            return;

        array<ChefZ_Sym> groups = new array<ChefZ_Sym>();
        tools.GetGroupsForClass(classSym, groups);

        for (int i = 0; i < groups.Count(); i++)
            ctx.AddToolGroup(groups.Get(i));
    }

    /**
     * Der Auszug aus Architekturplan §21, aber hinter der Kanalwache aus 18 §2:
     * bei zwanzig gleichzeitig laufenden Feuerstellen ist ein ungefiltertes
     * DEBUG unbrauchbar. Die Zeichenketten entstehen erst hinter der Pruefung.
     */
    private static void LogIfDebug(notnull ItemBase container, notnull ChefZ_FactSnapshot snapshot)
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        lines.Insert("Faktenerhebung  geraet=" + container.GetType() + "  cargo=" + snapshot.Count().ToString());
        snapshot.DebugDump(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.COOK, lines);
    }

    //! Nur fuer Test und Diagnose: leert die Zwischenspeicher.
    static void ClearCaches()
    {
        s_CleannessMax = null;
        s_LiquidSyms   = null;
    }
}
