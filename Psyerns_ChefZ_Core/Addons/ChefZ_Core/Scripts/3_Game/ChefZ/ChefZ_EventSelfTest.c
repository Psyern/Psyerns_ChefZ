//==============================================================================
// ChefZ_EventSelfTest - Abnahmepruefung fuer S13, soweit sie ohne Welt geht
//
// Entwurf: 17 §3 (Schnittstellen), 17 §5 (das eine Abfrage-Event), 17 §9
// (Fehlerverhalten, Zeile fuer Zeile), 17 E5/E6/E7, 19 S13
// (Abnahmebedingungen).
//
// ---------------------------------------------------------------------------
// Warum dieser Test anders ist als alle anderen im Core
// ---------------------------------------------------------------------------
// Jeder andere ChefZ-Selbsttest prueft eine EIGENE Rechnung: den Matcher, die
// Spezifitaet, die Punktzahl, den Verderbfaktor. Dieser hier prueft eine
// ZUSAGE GEGENUEBER FREMDEM CODE - und ist damit der einzige, dessen
// Fehlschlag einen anderen Mod trifft statt ChefZ selbst.
//
// Die vier Abnahmebedingungen aus 19 S13:
//
//   1. Ein Test-Abonnent empfaengt ChefZ_OnRecipeCompleted.       -> Zustellung
//   2. Ein Abonnent, der wirft, haelt das Kochen NICHT an.        -> siehe unten
//   3. ChefZ_OnRecipeMatched laesst sich stornieren.              -> Storno
//   4. Ohne Provider gilt der Config-Default.                     -> Faehigkeiten
//
// ---------------------------------------------------------------------------
// Was hier NICHT geprueft wird, und warum - ehrlich
// ---------------------------------------------------------------------------
// BEDINGUNG 2 IST HIER NICHT NACHSTELLBAR. Um sie zu pruefen, muesste der Test
// absichtlich einen Skriptfehler ausloesen (etwa einen Nullzugriff im
// Rueckruf). Das schriebe bei JEDEM Serverstart einen Fehler ins RPT und in
// crash_*.log - ein dauerhafter Fehlalarm fuer jeden Betreiber, als Preis
// dafuer, eine Eigenschaft zu bestaetigen, die nicht gerechnet, sondern
// GEBAUT ist:
//
//     Der Rueckruf laeuft ueber ScriptCaller.Invoke, ein natives proto
//     (2_GameLib/DayZ/tools.c:167). Ein Skriptfehler im Rueckruf laeuft bis zu
//     dieser nativen Grenze und nicht darueber hinaus. Die Schleife in
//     ChefZ_EventBus.Dispatch() steht hinter dieser Grenze.
//
// Das ist eine Struktur, keine Zahl, und wird am Diff geprueft - so wie der
// super-Aufruf im Kochhook (10 §3), der aus demselben Grund nicht in einem
// Testlauf steht.
//
// Ersatzweise prueft die Gruppe "Umbau" die verwandte und testbare Zusage: ein
// Abonnent, der WAEHREND der Zustellung die Abonnentenliste umbaut, bringt die
// Zustellung nicht durcheinander. Das ist der zweithaeufigste Weg, auf dem ein
// fremder Mod einen Bus zerlegt.
//
// Ebenfalls nicht geprueft: die schwache Besitzerreferenz (17 §9, letzte
// Zeile). Sie haengt daran, WANN Enforce ein unerreichbares Objekt einsammelt,
// und dafuer gibt es keine Zusage, auf die man einen Test bauen koennte. Ein
// Test, der "manchmal" gruen ist, ist schlimmer als keiner.
//
// Der Test laeuft auf einer EIGENEN Businstanz - nie auf dem Singleton. Die
// Faehigkeitsregistry und das Fortschrittsregister sind statisch und werden
// benutzt, aber vollstaendig zurueckgesetzt: er laeuft VOR LoadAll(), also vor
// jeder echten Anmeldung, und ChefZ_ConfigManager konfiguriert beide danach
// ohnehin neu.
//
// Layer: 3_Game.
//==============================================================================

/**
 * Ein Abonnent zum Anfassen.
 *
 * Eine Klasse fuer alle Rollen statt fuenf kleine: die Rolle steckt in der
 * gewaehlten Methode, nicht im Typ, und ein Test, der fuenf Typen deklariert,
 * um dreimal etwas zu zaehlen, liest sich schlechter als einer mit fuenf
 * Methoden.
 */
class ChefZ_EventTestSubscriber
{
    string name;
    int    count;
    string lastEventId;
    int    lastAmount;

    //! Gemeinsame Liste aller Aufrufe in Reihenfolge - fuer die Prioritaet.
    ref array<string> order;

    //! Fuer den Verschachtelungstest. OHNE ref: der Test haelt den Bus, nicht
    //! der Abonnent - ein Abonnent, der seinen Bus am Leben haelt, waere genau
    //! der Zyklus, den 17 §8 vermeiden will.
    ChefZ_EventBus bus;

    string cancelReason;
    float  bonus;

    //! Fuer den Umbautest: diese Anmeldung meldet sich im Rueckruf selbst ab.
    int    selfSubscriptionId;

    void ChefZ_EventTestSubscriber(string subscriberName)
    {
        name               = subscriberName;
        count              = 0;
        lastEventId        = "";
        lastAmount         = 0;
        cancelReason       = "";
        bonus              = 0.0;
        selfSubscriptionId = 0;
    }

    private void Note(notnull ChefZ_EventArgs args)
    {
        count++;
        lastEventId = args.eventId;
        lastAmount  = args.amount;
        if (order)
            order.Insert(name);
    }

    //! Zaehlt nur mit.
    void OnEvent(ChefZ_EventArgs args)
    {
        if (!args)
            return;
        Note(args);
    }

    //! Storniert.
    void OnCancel(ChefZ_EventArgs args)
    {
        if (!args)
            return;
        Note(args);
        args.Cancel(cancelReason);
    }

    //! Traegt Punkte bei - additiv, nie setzend (17 §5).
    void OnBonus(ChefZ_EventArgs args)
    {
        if (!args)
            return;
        Note(args);
        args.AddBonus(bonus);
    }

    //! Loest im Rueckruf dasselbe Ereignis erneut aus - der Endlosfall aus
    //! 17 §9, gegen den die Tiefenbegrenzung gebaut ist.
    void OnNested(ChefZ_EventArgs args)
    {
        if (!args)
            return;
        Note(args);

        if (!bus)
            return;
        ChefZ_EventArgs again = bus.Acquire(args.eventId);
        bus.Raise(again);
    }

    //! Meldet sich waehrend der Zustellung selbst ab.
    void OnUnsubscribeSelf(ChefZ_EventArgs args)
    {
        if (!args)
            return;
        Note(args);

        if (bus && selfSubscriptionId > 0)
            bus.Unsubscribe(selfSubscriptionId);
    }
}

//==============================================================================

//! Ein Fortschrittsempfaenger zum Anfassen.
class ChefZ_EventTestSink extends ChefZ_IProgressSink
{
    int    count;
    string lastKind;
    string lastEventId;

    override string GetSinkName()
    {
        return "ChefZ_EventSelfTest";
    }

    override void OnChefZProgress(string progressKind, notnull ChefZ_EventArgs args)
    {
        count++;
        lastKind    = progressKind;
        lastEventId = args.eventId;
    }
}

//==============================================================================

//! Ein Faehigkeitsanbieter zum Anfassen.
class ChefZ_EventTestProvider extends ChefZ_ICapabilityProvider
{
    string    providerName;
    int       priority;
    ChefZ_Sym answersFor;
    float     value;
    bool      silent;         // antwortet auf gar nichts

    void ChefZ_EventTestProvider(string n, int prio, ChefZ_Sym sym, float v)
    {
        providerName = n;
        priority     = prio;
        answersFor   = sym;
        value        = v;
        silent       = false;
    }

    override string GetProviderName()
    {
        return providerName;
    }

    override int GetPriority()
    {
        return priority;
    }

    override bool TryGetCapability(int identityId, ChefZ_Sym capability, out float outValue)
    {
        outValue = 0.0;
        if (silent)
            return false;
        if (capability != answersFor)
            return false;
        outValue = value;
        return true;
    }
}

//==============================================================================

class ChefZ_EventSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("Namen",        ChefZ_EventNames.SelfCheck());
        Check("Fortschrittsarten", ChefZ_ProgressKind.SelfCheck());
        Check("Nutzlast",     ChefZ_EventArgs.SelfCheck());

        Check("Zustellung",   DeliveryCheck());
        Check("Prioritaet",   PriorityCheck());
        Check("Abmeldung",    UnsubscribeCheck());
        Check("Storno",       CancelCheck());
        Check("KeinStorno",   NonCancellableCheck());
        Check("Abfrage",      QueryCheck());
        Check("KeineAbfrage", NonQueryCheck());
        Check("Tiefe",        DepthCheck());
        Check("Umbau",        MutationCheck());
        Check("Pool",         PoolCheck());
        Check("Fortschritt",  ProgressCheck());
        Check("OhneAnbieter", NoProviderCheck());
        Check("Anbieter",     ProviderCheck());
        Check("Klemmung",     ClampCheck());
        Check("Modi",         ModeCheck());
        Check("Filter",       GateCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.EVENT, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.EVENT, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.EVENT, "Selbsttest " + name + " FEHLGESCHLAGEN. Die Ereignisschicht verhaelt sich nicht " + "wie entworfen - Comp-Module bekommen ab hier falsche oder gar keine Meldungen. " + "Kochen und Vanilla sind davon unberuehrt.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S13: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
        if (s_Failed > 0 && s_FailedNames)
        {
            s = s + "  gescheitert:";
            for (int i = 0; i < s_FailedNames.Count(); i++)
                s = s + " " + s_FailedNames.Get(i);
        }
        return s;
    }

    //==========================================================================
    // Werkzeug
    //==========================================================================

    //! Eine eigene Businstanz, serverseitig und still. Nie der Singleton -
    //! der gehoert dem Server und kann zu diesem Zeitpunkt bereits Anmeldungen
    //! aus einem fremden Mod tragen.
    private static ChefZ_EventBus NewBus()
    {
        ChefZ_EventBus bus = new ChefZ_EventBus();
        bus.SetServerForTest(true);
        bus.SetQuietForTest(true);
        return bus;
    }

    private static int Sub(notnull ChefZ_EventBus bus, string eventId, notnull ChefZ_EventTestSubscriber sub, int priority = 0)
    {
        return bus.Subscribe(eventId, sub, ScriptCaller.Create(sub.OnEvent), sub.name, priority);
    }

    //==========================================================================
    // Zustellung
    //==========================================================================

    /**
     * Abnahmebedingung 1 aus 19 S13: "Ein Test-Abonnent empfaengt
     * ChefZ_OnRecipeCompleted."
     *
     * Zusammen mit der Kostenzusage aus 17 E2: VOR der Anmeldung ist
     * HasSubscribers false, und genau daran haengt, dass ein Server ohne
     * Comp-Module nie eine Nutzlast baut.
     */
    private static bool DeliveryCheck()
    {
        ChefZ_EventBus bus = NewBus();
        ChefZ_EventTestSubscriber a = new ChefZ_EventTestSubscriber("A");

        if (bus.HasSubscribers(ChefZ_EventNames.RECIPE_COMPLETED))    return false;
        if (bus.GetSubscriberCount(ChefZ_EventNames.RECIPE_COMPLETED) != 0) return false;

        int id = Sub(bus, ChefZ_EventNames.RECIPE_COMPLETED, a);
        if (id <= 0)                                                  return false;
        if (!bus.HasSubscribers(ChefZ_EventNames.RECIPE_COMPLETED))   return false;

        // Ein anderes Ereignis bleibt unberuehrt - der Bus liefert nach Namen.
        if (bus.HasSubscribers(ChefZ_EventNames.RECIPE_FAILED))       return false;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.RECIPE_COMPLETED);
        args.amount = 7;
        bus.Raise(args);

        if (a.count != 1)                                             return false;
        if (a.lastEventId != ChefZ_EventNames.RECIPE_COMPLETED)       return false;
        if (a.lastAmount != 7)                                        return false;

        // Ein Abonnement ohne ScriptCaller wird abgewiesen, nicht angenommen -
        // sonst stuende eine Anmeldung in der Liste, die nie etwas bekaeme.
        if (bus.Subscribe(ChefZ_EventNames.RECIPE_COMPLETED, a, null, "leer") != 0) return false;
        if (bus.Subscribe("", a, ScriptCaller.Create(a.OnEvent), "leer") != 0)      return false;

        // Ein unbekannter Ereignisname wird ANGENOMMEN (17 §9): Content loest
        // ueber emitEvents eigene Ereignisse aus.
        ChefZ_EventTestSubscriber b = new ChefZ_EventTestSubscriber("B");
        if (Sub(bus, "ChefZTest_EigenesEreignis", b) <= 0)            return false;

        ChefZ_EventArgs custom = bus.Acquire("ChefZTest_EigenesEreignis");
        bus.Raise(custom);
        if (b.count != 1)                                             return false;

        return true;
    }

    //! Hoehere Prioritaet zuerst, bei Gleichstand in Anmeldereihenfolge.
    private static bool PriorityCheck()
    {
        ChefZ_EventBus bus = NewBus();
        array<string> order = new array<string>();

        ChefZ_EventTestSubscriber low   = new ChefZ_EventTestSubscriber("low");
        ChefZ_EventTestSubscriber high  = new ChefZ_EventTestSubscriber("high");
        ChefZ_EventTestSubscriber mid1  = new ChefZ_EventTestSubscriber("mid1");
        ChefZ_EventTestSubscriber mid2  = new ChefZ_EventTestSubscriber("mid2");
        low.order  = order;
        high.order = order;
        mid1.order = order;
        mid2.order = order;

        Sub(bus, ChefZ_EventNames.FOOD_CONSUMED, low,  -10);
        Sub(bus, ChefZ_EventNames.FOOD_CONSUMED, mid1,   0);
        Sub(bus, ChefZ_EventNames.FOOD_CONSUMED, high,  50);
        Sub(bus, ChefZ_EventNames.FOOD_CONSUMED, mid2,   0);

        bus.Raise(bus.Acquire(ChefZ_EventNames.FOOD_CONSUMED));

        if (order.Count() != 4)         return false;
        if (order.Get(0) != "high")     return false;
        if (order.Get(1) != "mid1")     return false;   // Gleichstand: zuerst angemeldet
        if (order.Get(2) != "mid2")     return false;
        if (order.Get(3) != "low")      return false;

        return true;
    }

    private static bool UnsubscribeCheck()
    {
        ChefZ_EventBus bus = NewBus();
        ChefZ_EventTestSubscriber a = new ChefZ_EventTestSubscriber("A");
        ChefZ_EventTestSubscriber b = new ChefZ_EventTestSubscriber("B");

        int idA = Sub(bus, ChefZ_EventNames.FOOD_SPOILED, a);
        Sub(bus, ChefZ_EventNames.FOOD_SPOILED, b);
        if (bus.GetSubscriberCount(ChefZ_EventNames.FOOD_SPOILED) != 2) return false;

        bus.Unsubscribe(idA);
        if (bus.GetSubscriberCount(ChefZ_EventNames.FOOD_SPOILED) != 1) return false;

        bus.Raise(bus.Acquire(ChefZ_EventNames.FOOD_SPOILED));
        if (a.count != 0)   return false;
        if (b.count != 1)   return false;

        // Zweimal abmelden ist kein Fehler, sondern ein No-op.
        bus.Unsubscribe(idA);
        bus.Unsubscribe(0);
        bus.Unsubscribe(-1);

        // Massenabmeldung ueber den Besitzer.
        bus.UnsubscribeOwner(b);
        if (bus.GetSubscriberCount(ChefZ_EventNames.FOOD_SPOILED) != 0) return false;

        bus.Raise(bus.Acquire(ChefZ_EventNames.FOOD_SPOILED));
        if (b.count != 1)   return false;

        return true;
    }

    //==========================================================================
    // Storno (17 E5)
    //==========================================================================

    /**
     * Abnahmebedingung 3 aus 19 S13: "ChefZ_OnRecipeMatched laesst sich
     * stornieren und ChefZ faellt dann auf Vanilla zurueck."
     *
     * Der zweite Teil - der Rueckfall - steht im
     * ChefZ_CookingDeviceAdapter und ist der Ausgang, den auch der Zweig
     * "kein Treffer" nimmt. Hier geprueft wird der erste Teil: die Antwort
     * kommt beim Ausloeser an, mit Grund und mit Namen des Abbrechers.
     */
    private static bool CancelCheck()
    {
        ChefZ_EventBus bus = NewBus();

        ChefZ_EventTestSubscriber blocker = new ChefZ_EventTestSubscriber("Blocker");
        blocker.cancelReason = "kein Skill";
        ChefZ_EventTestSubscriber later   = new ChefZ_EventTestSubscriber("Danach");

        bus.Subscribe(ChefZ_EventNames.RECIPE_MATCHED, blocker, ScriptCaller.Create(blocker.OnCancel), blocker.name, 10);
        Sub(bus, ChefZ_EventNames.RECIPE_MATCHED, later, 0);

        string reason;
        string who;
        bool cancelled = bus.RaiseCancellable(bus.Acquire(ChefZ_EventNames.RECIPE_MATCHED), reason, who);

        if (!cancelled)                         return false;
        if (reason != "kein Skill")             return false;
        if (who == "")                          return false;
        if (blocker.count != 1)                 return false;

        // Nach einer Stornierung wird nicht weiter zugestellt: die Wirkung
        // findet nicht statt, und ein weiterer Abonnent koennte sie weder
        // zurueckholen noch sinnvoll darauf reagieren.
        if (later.count != 0)                   return false;
        if (bus.GetCancelledCount() != 1)       return false;

        // Ohne Abbrecher kommt sauber false zurueck.
        ChefZ_EventBus clean = NewBus();
        ChefZ_EventTestSubscriber quiet = new ChefZ_EventTestSubscriber("Still");
        Sub(clean, ChefZ_EventNames.RECIPE_MATCHED, quiet);

        string r2;
        string w2;
        if (clean.RaiseCancellable(clean.Acquire(ChefZ_EventNames.RECIPE_MATCHED), r2, w2))
            return false;
        if (quiet.count != 1)                   return false;

        return true;
    }

    /**
     * 17 §9: "Abonnent setzt cancelled bei einem nicht stornierbaren Event ->
     * ignoriert, WARN einmal je Abonnent."
     *
     * Die wichtigste Zeile dieses Tests. Waere sie falsch, koennte ein fremder
     * Mod einen bereits vollzogenen Verbrauch "zurueckweisen" - und der Core
     * wuerde ihm glauben.
     */
    private static bool NonCancellableCheck()
    {
        ChefZ_EventBus bus = NewBus();

        ChefZ_EventTestSubscriber blocker = new ChefZ_EventTestSubscriber("Blocker");
        blocker.cancelReason = "zu spaet";
        ChefZ_EventTestSubscriber later   = new ChefZ_EventTestSubscriber("Danach");

        bus.Subscribe(ChefZ_EventNames.RECIPE_COMPLETED, blocker, ScriptCaller.Create(blocker.OnCancel), blocker.name, 10);
        Sub(bus, ChefZ_EventNames.RECIPE_COMPLETED, later, 0);

        string reason;
        string who;
        bool cancelled = bus.RaiseCancellable(bus.Acquire(ChefZ_EventNames.RECIPE_COMPLETED), reason, who);

        if (cancelled)              return false;      // die Stornierung wurde verworfen
        if (reason != "")           return false;
        if (blocker.count != 1)     return false;
        if (later.count != 1)       return false;      // die Zustellung lief weiter

        if (!bus.IsCancellable(ChefZ_EventNames.RECIPE_MATCHED))   return false;
        if (bus.IsCancellable(ChefZ_EventNames.RECIPE_COMPLETED))  return false;

        return true;
    }

    //==========================================================================
    // Das eine Abfrage-Event (17 §5, E6)
    //==========================================================================

    private static bool QueryCheck()
    {
        ChefZ_EventBus bus = NewBus();

        ChefZ_EventTestSubscriber a = new ChefZ_EventTestSubscriber("A");
        ChefZ_EventTestSubscriber b = new ChefZ_EventTestSubscriber("B");
        a.bonus = 0.75;
        b.bonus = 0.50;

        bus.Subscribe(ChefZ_EventNames.QUALITY_BONUS_QUERY, a, ScriptCaller.Create(a.OnBonus), a.name);
        bus.Subscribe(ChefZ_EventNames.QUALITY_BONUS_QUERY, b, ScriptCaller.Create(b.OnBonus), b.name);

        // Additiv: beide tragen bei, keiner loescht den anderen.
        float sum = bus.RaiseQuery(bus.Acquire(ChefZ_EventNames.QUALITY_BONUS_QUERY), 2.0);
        if (sum < 1.24 || sum > 1.26)   return false;
        if (a.count != 1)               return false;
        if (b.count != 1)               return false;

        // Geklemmt: kein Abonnent kann die Qualitaetslogik uebernehmen.
        ChefZ_EventBus greedy = NewBus();
        ChefZ_EventTestSubscriber hog = new ChefZ_EventTestSubscriber("Gierig");
        hog.bonus = 999.0;
        greedy.Subscribe(ChefZ_EventNames.QUALITY_BONUS_QUERY, hog, ScriptCaller.Create(hog.OnBonus), hog.name);

        float clamped = greedy.RaiseQuery(greedy.Acquire(ChefZ_EventNames.QUALITY_BONUS_QUERY), 2.0);
        if (clamped != 2.0)             return false;

        // Symmetrisch nach unten - ein Abonnent darf auch nicht beliebig
        // abwerten.
        hog.bonus = -999.0;
        float low = greedy.RaiseQuery(greedy.Acquire(ChefZ_EventNames.QUALITY_BONUS_QUERY), 2.0);
        if (low != -2.0)                return false;

        // maxBonus 0 heisst "kein externer Bonus" und ist eine gueltige
        // Einstellung.
        hog.bonus = 5.0;
        if (greedy.RaiseQuery(greedy.Acquire(ChefZ_EventNames.QUALITY_BONUS_QUERY), 0.0) != 0.0)
            return false;

        // Ohne Abonnenten: 0.0, und die Nutzlast wird gar nicht erst gebaut.
        ChefZ_EventBus empty = NewBus();
        if (empty.HasSubscribers(ChefZ_EventNames.QUALITY_BONUS_QUERY))  return false;
        if (empty.RaiseQuery(empty.Acquire(ChefZ_EventNames.QUALITY_BONUS_QUERY), 2.0) != 0.0)
            return false;

        return true;
    }

    /**
     * 17 E6: es gibt GENAU EIN Abfrage-Event. Ein Punktebeitrag zu irgendetwas
     * anderem ist wirkungslos.
     */
    private static bool NonQueryCheck()
    {
        ChefZ_EventBus bus = NewBus();

        ChefZ_EventTestSubscriber a = new ChefZ_EventTestSubscriber("A");
        a.bonus = 5.0;
        bus.Subscribe(ChefZ_EventNames.RECIPE_COMPLETED, a, ScriptCaller.Create(a.OnBonus), a.name);

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.RECIPE_COMPLETED);
        bus.RaiseKeep(args);

        if (a.count != 1)               return false;
        if (args.bonusPoints != 0.0)    return false;   // zurueckgesetzt

        bus.Release(args);
        return true;
    }

    //==========================================================================
    // Robustheit (17 §9)
    //==========================================================================

    /**
     * 17 §9: "Abonnent feuert im Callback ein Event -> erlaubt bis Tiefe 3,
     * danach abgebrochen mit ERROR."
     *
     * Mit Tiefe 2 statt 3, damit der Test kurz bleibt: erwartet werden genau
     * zwei Aufrufe. Der dritte faellt gegen die Grenze.
     */
    private static bool DepthCheck()
    {
        ChefZ_EventBus bus = NewBus();
        bus.SetMaxDepthForTest(2);

        ChefZ_EventTestSubscriber loop = new ChefZ_EventTestSubscriber("Schleife");
        loop.bus = bus;
        bus.Subscribe(ChefZ_EventNames.FOOD_STATE_CHANGED, loop, ScriptCaller.Create(loop.OnNested), loop.name);

        bus.Raise(bus.Acquire(ChefZ_EventNames.FOOD_STATE_CHANGED));

        if (loop.count != 2)        return false;
        if (bus.GetDepth() != 0)    return false;   // sauber abgebaut

        return true;
    }

    /**
     * Ein Abonnent baut WAEHREND der Zustellung die Abonnentenliste um.
     *
     * Der zweithaeufigste Weg, auf dem ein fremder Mod einen Bus zerlegt -
     * und der einzige der beiden, der sich ohne absichtlichen Skriptfehler
     * pruefen laesst (siehe Kopf). Erwartet wird: jeder der drei
     * urspruenglichen Abonnenten wird genau einmal gerufen, der neu
     * hinzugekommene in DIESEM Durchlauf gar nicht.
     */
    private static bool MutationCheck()
    {
        ChefZ_EventBus bus = NewBus();
        array<string> order = new array<string>();

        ChefZ_EventTestSubscriber first  = new ChefZ_EventTestSubscriber("Erst");
        ChefZ_EventTestSubscriber leaver = new ChefZ_EventTestSubscriber("Geht");
        ChefZ_EventTestSubscriber last   = new ChefZ_EventTestSubscriber("Zuletzt");
        first.order  = order;
        leaver.order = order;
        last.order   = order;

        Sub(bus, ChefZ_EventNames.FOOD_PRESERVED, first, 30);

        leaver.bus = bus;
        leaver.selfSubscriptionId = bus.Subscribe(ChefZ_EventNames.FOOD_PRESERVED, leaver, ScriptCaller.Create(leaver.OnUnsubscribeSelf), leaver.name, 20);
        Sub(bus, ChefZ_EventNames.FOOD_PRESERVED, last, 10);

        bus.Raise(bus.Acquire(ChefZ_EventNames.FOOD_PRESERVED));

        if (order.Count() != 3)     return false;
        if (first.count != 1)       return false;
        if (leaver.count != 1)      return false;
        if (last.count != 1)        return false;
        if (bus.GetSubscriberCount(ChefZ_EventNames.FOOD_PRESERVED) != 2) return false;

        // Zweiter Durchlauf: der Abgemeldete bekommt nichts mehr.
        bus.Raise(bus.Acquire(ChefZ_EventNames.FOOD_PRESERVED));
        if (leaver.count != 1)      return false;
        if (first.count != 2)       return false;
        if (last.count != 2)        return false;

        return true;
    }

    /**
     * Der Pool (17 §8).
     *
     * Geprueft wird die Buchhaltung, nicht die Identitaet der Objekte: eine
     * Nutzlast, die nach Raise noch als "unterwegs" gilt, waere ein Leck, das
     * erst nach Stunden auffiele.
     */
    private static bool PoolCheck()
    {
        ChefZ_EventBus bus = NewBus();

        if (bus.GetInFlightCount() != 0)        return false;

        ChefZ_EventArgs a = bus.Acquire(ChefZ_EventNames.CONFIG_LOADED);
        if (a.eventId != ChefZ_EventNames.CONFIG_LOADED) return false;
        if (bus.GetInFlightCount() != 1)        return false;

        // Ohne Abonnent kehrt Raise sofort zurueck - und gibt trotzdem zurueck.
        bus.Raise(a);
        if (bus.GetInFlightCount() != 0)        return false;

        // Wiederverwendet und sauber: kein Rest der vorherigen Benutzung.
        ChefZ_EventArgs b = bus.Acquire(ChefZ_EventNames.FOOD_CONSUMED);
        if (b.eventId != ChefZ_EventNames.FOOD_CONSUMED) return false;
        if (b.amount != 0)                      return false;
        if (b.cancelled)                        return false;
        if (b.consumedClasses.Count() != 0)     return false;
        bus.Release(b);

        return true;
    }

    //==========================================================================
    // Fortschritt (17 §3.4, E7)
    //==========================================================================

    private static bool ProgressCheck()
    {
        ChefZ_ProgressRegistry.SetQuietForTest(true);
        ChefZ_ProgressRegistry.ClearSinks();
        ChefZ_ProgressRegistry.ResetCounters();

        bool ok = ProgressInner();

        ChefZ_ProgressRegistry.ClearSinks();
        ChefZ_ProgressRegistry.ResetCounters();
        ChefZ_ProgressRegistry.SetQuietForTest(false);
        return ok;
    }

    private static bool ProgressInner()
    {
        ChefZ_EventBus bus = NewBus();

        if (ChefZ_ProgressRegistry.HasSinks())          return false;
        if (ChefZ_ProgressRegistry.GetSinkCount() != 0) return false;

        // Ohne Empfaenger ist Report ein No-op - und darf keinen Fehler geben.
        ChefZ_EventArgs warmup = bus.Acquire(ChefZ_EventNames.RECIPE_COMPLETED);
        ChefZ_ProgressRegistry.Report(ChefZ_ProgressKind.COOK, warmup);
        if (ChefZ_ProgressRegistry.GetReportedCount() != 0) return false;
        bus.Release(warmup);

        ChefZ_EventTestSink sink = new ChefZ_EventTestSink();
        ChefZ_ProgressRegistry.RegisterSink(sink);
        if (!ChefZ_ProgressRegistry.HasSinks())         return false;

        // Doppelanmeldung wird verworfen - sonst bekaeme er jeden Abschluss
        // zweimal, und ein Comp-Modul vergaebe doppeltes XP.
        ChefZ_ProgressRegistry.RegisterSink(sink);
        if (ChefZ_ProgressRegistry.GetSinkCount() != 1) return false;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.RECIPE_COMPLETED);
        ChefZ_ProgressRegistry.Report(ChefZ_ProgressKind.COOK, args);
        bus.Release(args);

        if (sink.count != 1)                                    return false;
        if (sink.lastKind != ChefZ_ProgressKind.COOK)           return false;
        if (sink.lastEventId != ChefZ_EventNames.RECIPE_COMPLETED) return false;
        if (ChefZ_ProgressRegistry.GetDeliveredCount() != 1)    return false;

        ChefZ_ProgressRegistry.UnregisterSink(sink);
        if (ChefZ_ProgressRegistry.HasSinks())                  return false;

        ChefZ_EventArgs after = bus.Acquire(ChefZ_EventNames.RECIPE_COMPLETED);
        ChefZ_ProgressRegistry.Report(ChefZ_ProgressKind.COOK, after);
        bus.Release(after);
        if (sink.count != 1)                                    return false;

        return true;
    }

    //==========================================================================
    // Faehigkeiten (17 §3.3, §9)
    //==========================================================================

    //! Setzt die Registry auf einen bekannten Stand. Sie ist ein Singleton;
    //! der Test laeuft VOR LoadAll(), also bevor irgendein Comp-Modul etwas
    //! angemeldet haben kann, und ChefZ_ConfigManager konfiguriert sie danach
    //! ohnehin aus Core.json.
    private static ChefZ_CapabilityRegistry FreshRegistry(string mode, float def, float minV, float maxV)
    {
        ChefZ_CapabilityRegistry reg = ChefZ_CapabilityRegistry.Get();
        reg.SetQuietForTest(true);
        reg.ClearProviders();
        reg.ResetCounters();
        reg.ConfigureForTest(mode, def, minV, maxV);
        return reg;
    }

    private static void RestoreRegistry()
    {
        ChefZ_CapabilityRegistry reg = ChefZ_CapabilityRegistry.Get();
        reg.ClearProviders();
        reg.ResetCounters();
        reg.ConfigureForTest(ChefZ_CapabilityRegistry.MODE_AS_AUTHORED, 0.0, 0.0, 10.0);
        reg.SetQuietForTest(false);
    }

    /**
     * Abnahmebedingung 4 aus 19 S13: "ohne Provider gilt der Config-Default."
     *
     * Und die zweite, ebenso wichtige Haelfte: die QUALITAETSSONDE antwortet
     * ohne Anbieter mit FALSE (12 §8). Die beiden Vertraege sind absichtlich
     * verschieden - waeren sie gleich, gaebe der Default entweder jedem
     * Spieler einen Skillbonus oder er sperrte jedes Rezept.
     */
    private static bool NoProviderCheck()
    {
        ChefZ_CapabilityRegistry reg = FreshRegistry( ChefZ_CapabilityRegistry.MODE_AS_AUTHORED, 1.5, 0.0, 10.0);

        bool ok = NoProviderInner(reg);
        RestoreRegistry();
        return ok;
    }

    private static bool NoProviderInner(notnull ChefZ_CapabilityRegistry reg)
    {
        ChefZ_Sym cap = ChefZ_SymbolTable.Intern("CHEFZ_EVTEST_CAP");

        if (reg.GetProviderCount() != 0)                return false;
        if (reg.GetCapability(42, cap) != 1.5)          return false;
        if (reg.GetCapabilityByName(42, "CHEFZ_EVTEST_CAP") != 1.5) return false;

        // Der zweite Vertrag: niemand hat GEANTWORTET.
        float value;
        if (reg.TryQuery(42, cap, value))               return false;

        ChefZ_RegistryCapabilityProbe probe = new ChefZ_RegistryCapabilityProbe();
        float probeValue;
        if (probe.TryGetValue("CHEFZ_EVTEST_CAP", 42, probeValue)) return false;

        // Anforderung unter dem Default: erfuellt. Darueber: nicht erfuellt,
        // aber mit Grund und Stufenzahl - und ohne Fehler.
        ChefZ_CapabilityReq easy = new ChefZ_CapabilityReq();
        easy.capability = "CHEFZ_EVTEST_CAP";
        easy.ResolveDefaults();
        easy.min = 1.0;

        string why;
        int    steps;
        if (!reg.MeetsRequirement(42, easy, why, steps))    return false;

        ChefZ_CapabilityReq hard = new ChefZ_CapabilityReq();
        hard.capability = "CHEFZ_EVTEST_CAP";
        hard.ResolveDefaults();
        hard.min = 5.0;

        if (reg.MeetsRequirement(42, hard, why, steps))     return false;
        if (why == "")                                      return false;
        if (steps != 1)                                     return false;   // Default aus 17 §3.3

        // Der Default aus 17 §3.3 ist "degrade" - ohne Skillmod wird also
        // abgewertet und NICHT gesperrt. Das ist die Zeile, an der haengt, ob
        // ein Server ohne Skillmod spielbar ist.
        if (reg.EffectiveOnFail(hard) != ChefZ_CapabilityRegistry.ON_FAIL_DEGRADE) return false;

        array<ref ChefZ_CapabilityReq> reqs = new array<ref ChefZ_CapabilityReq>();
        reqs.Insert(hard);

        string blockWhy;
        if (reg.BlocksAny(reqs, 42, blockWhy))              return false;

        string degradeWhy;
        if (reg.DegradeStepsFor(reqs, 42, degradeWhy) != 1) return false;
        if (reg.YieldFactorFor(reqs, 42) != 1.0)            return false;

        return true;
    }

    private static bool ProviderCheck()
    {
        ChefZ_CapabilityRegistry reg = FreshRegistry( ChefZ_CapabilityRegistry.MODE_AS_AUTHORED, 0.0, 0.0, 10.0);

        bool ok = ProviderInner(reg);
        RestoreRegistry();
        return ok;
    }

    private static bool ProviderInner(notnull ChefZ_CapabilityRegistry reg)
    {
        ChefZ_Sym cap   = ChefZ_SymbolTable.Intern("CHEFZ_EVTEST_CAP");
        ChefZ_Sym other = ChefZ_SymbolTable.Intern("CHEFZ_EVTEST_ANDERE");

        ChefZ_EventTestProvider weak   = new ChefZ_EventTestProvider("Schwach", 1, cap, 2.0);
        ChefZ_EventTestProvider strong = new ChefZ_EventTestProvider("Stark", 99, cap, 7.0);

        reg.RegisterProvider(weak);
        if (reg.GetProviderCount() != 1)            return false;
        if (reg.GetCapability(1, cap) != 2.0)       return false;

        // Hoechste Prioritaet gewinnt, unabhaengig von der Anmeldereihenfolge.
        reg.RegisterProvider(strong);
        if (reg.GetProviderCount() != 2)            return false;
        if (reg.GetCapability(1, cap) != 7.0)       return false;

        // Doppelanmeldung derselben Instanz wird verworfen.
        reg.RegisterProvider(strong);
        if (reg.GetProviderCount() != 2)            return false;

        // Wozu niemand etwas sagt, gilt der Default.
        if (reg.GetCapability(1, other) != 0.0)     return false;
        float dummy;
        if (reg.TryQuery(1, other, dummy))          return false;

        // Der Anbieter mit der hoeheren Prioritaet faellt weg - der naechste
        // antwortet, und zwar ohne Umbau der Reihenfolge.
        reg.UnregisterProvider(strong);
        if (reg.GetProviderCount() != 1)            return false;
        if (reg.GetCapability(1, cap) != 2.0)       return false;

        // Die Sonde fuer Qualitaetsregeln antwortet jetzt SEHR WOHL - es gibt
        // ja einen Anbieter.
        ChefZ_RegistryCapabilityProbe probe = new ChefZ_RegistryCapabilityProbe();
        float probeValue;
        if (!probe.TryGetValue("CHEFZ_EVTEST_CAP", 1, probeValue)) return false;
        if (probeValue != 2.0)                                     return false;

        return true;
    }

    /**
     * 17 §9: "Provider liefert NaN, negativ oder unsinnig -> auf den
     * Config-Bereich geklemmt, WARN einmal je Provider."
     */
    private static bool ClampCheck()
    {
        ChefZ_CapabilityRegistry reg = FreshRegistry( ChefZ_CapabilityRegistry.MODE_AS_AUTHORED, 0.0, 0.0, 5.0);

        bool ok = ClampInner(reg);
        RestoreRegistry();
        return ok;
    }

    private static bool ClampInner(notnull ChefZ_CapabilityRegistry reg)
    {
        ChefZ_Sym cap = ChefZ_SymbolTable.Intern("CHEFZ_EVTEST_CAP");

        ChefZ_EventTestProvider tooHigh = new ChefZ_EventTestProvider("ZuHoch", 5, cap, 900.0);
        reg.RegisterProvider(tooHigh);
        if (reg.GetCapability(1, cap) != 5.0)       return false;

        tooHigh.value = -900.0;
        if (reg.GetCapability(1, cap) != 0.0)       return false;

        tooHigh.value = 3.0;
        if (reg.GetCapability(1, cap) != 3.0)       return false;
        if (reg.GetClampedCount() != 2)             return false;

        // Ein Anbieter, der zu gar nichts etwas sagt, blockiert niemanden.
        tooHigh.silent = true;
        if (reg.GetCapability(1, cap) != 0.0)       return false;   // Default

        return true;
    }

    /**
     * Die zwei Betreiberschalter aus 17 §9.
     *
     * Sie sind der Notausgang fuer einen Server, dessen Betreiber keine harten
     * Rezeptschloesser will - eine Zeile in Core.json statt einer Aenderung am
     * Content.
     */
    private static bool ModeCheck()
    {
        bool ok = ModeInner();
        RestoreRegistry();
        return ok;
    }

    private static bool ModeInner()
    {
        ChefZ_CapabilityReq blocker = new ChefZ_CapabilityReq();
        blocker.capability = "CHEFZ_EVTEST_CAP";
        blocker.ResolveDefaults();
        blocker.min    = 5.0;
        blocker.onFail = ChefZ_CapabilityRegistry.ON_FAIL_BLOCK;

        array<ref ChefZ_CapabilityReq> reqs = new array<ref ChefZ_CapabilityReq>();
        reqs.Insert(blocker);

        string why;

        // asAuthored: "block" sperrt.
        ChefZ_CapabilityRegistry reg = FreshRegistry( ChefZ_CapabilityRegistry.MODE_AS_AUTHORED, 0.0, 0.0, 10.0);
        if (reg.EffectiveOnFail(blocker) != ChefZ_CapabilityRegistry.ON_FAIL_BLOCK) return false;
        if (!reg.BlocksAny(reqs, 1, why))       return false;
        if (why == "")                          return false;

        // neverBlock: aus "block" wird "degrade" - nichts wird gesperrt.
        reg = FreshRegistry(ChefZ_CapabilityRegistry.MODE_NEVER_BLOCK, 0.0, 0.0, 10.0);
        if (reg.EffectiveOnFail(blocker) != ChefZ_CapabilityRegistry.ON_FAIL_DEGRADE) return false;
        if (reg.BlocksAny(reqs, 1, why))        return false;
        if (reg.DegradeStepsFor(reqs, 1, why) != 1) return false;

        // ignore: alle requires[] gelten als erfuellt.
        reg = FreshRegistry(ChefZ_CapabilityRegistry.MODE_IGNORE, 0.0, 0.0, 10.0);
        string ignoreWhy;
        int    ignoreSteps;
        if (!reg.MeetsRequirement(1, blocker, ignoreWhy, ignoreSteps)) return false;
        if (reg.BlocksAny(reqs, 1, why))            return false;
        if (reg.DegradeStepsFor(reqs, 1, why) != 0) return false;
        if (reg.YieldFactorFor(reqs, 1) != 1.0)     return false;

        // reduceYield: Produkt, nie Summe - und nie negativ.
        ChefZ_CapabilityReq yield = new ChefZ_CapabilityReq();
        yield.capability  = "CHEFZ_EVTEST_CAP";
        yield.ResolveDefaults();
        yield.min         = 5.0;
        yield.onFail      = ChefZ_CapabilityRegistry.ON_FAIL_REDUCE_YIELD;
        yield.yieldFactor = 0.5;

        array<ref ChefZ_CapabilityReq> two = new array<ref ChefZ_CapabilityReq>();
        two.Insert(yield);
        two.Insert(yield);

        reg = FreshRegistry(ChefZ_CapabilityRegistry.MODE_AS_AUTHORED, 0.0, 0.0, 10.0);
        float f = reg.YieldFactorFor(two, 1);
        if (f < 0.24 || f > 0.26)               return false;

        return true;
    }

    /**
     * Der Einhaengepunkt im Rezeptablauf (08 §7 Schritt 2c).
     *
     * Die erste und die letzte Zeile sind die wichtigsten: OHNE eingehaengtes
     * Gate blockiert nichts. Faellt das um, sperrt ein Server ohne Skillmodul
     * lautlos Rezepte, die niemand gesperrt hat.
     */
    private static bool GateCheck()
    {
        // Ausgangslage herstellen. Der Test laeuft VOR LoadAll(), also bevor
        // ChefZ_ConfigManager das Gate einhaengt - aber verlassen soll er sich
        // darauf nicht.
        ChefZ_CapabilityGate.ClearActive();

        bool ok = GateInner();

        ChefZ_CapabilityGate.ClearActive();
        RestoreRegistry();
        return ok;
    }

    private static bool GateInner()
    {
        ChefZ_CapabilityReq blocker = new ChefZ_CapabilityReq();
        blocker.capability = "CHEFZ_EVTEST_CAP";
        blocker.ResolveDefaults();
        blocker.min    = 5.0;
        blocker.onFail = ChefZ_CapabilityRegistry.ON_FAIL_BLOCK;

        array<ref ChefZ_CapabilityReq> reqs = new array<ref ChefZ_CapabilityReq>();
        reqs.Insert(blocker);

        string why;

        // Ohne Gate: nichts blockiert. Das ist der Zustand bis S12 und der
        // Zustand jedes Servers, dessen Config nicht geladen hat.
        if (ChefZ_CapabilityGate.HasActive())               return false;
        if (ChefZ_CapabilityGate.Denies(reqs, 1, why))      return false;

        // Mit Gate und ohne Anbieter: der Default (0) unterschreitet min (5),
        // also sperrt "block".
        FreshRegistry(ChefZ_CapabilityRegistry.MODE_AS_AUTHORED, 0.0, 0.0, 10.0);
        ChefZ_CapabilityGate.SetActive(new ChefZ_RegistryCapabilityGate());

        if (!ChefZ_CapabilityGate.HasActive())              return false;
        if (!ChefZ_CapabilityGate.Denies(reqs, 1, why))     return false;
        if (why == "")                                      return false;

        // Leere und fehlende Anforderungsliste sperren nie - der Normalfall
        // fuer JEDES Rezept ohne requires[].
        array<ref ChefZ_CapabilityReq> none = new array<ref ChefZ_CapabilityReq>();
        if (ChefZ_CapabilityGate.Denies(none, 1, why))      return false;
        if (ChefZ_CapabilityGate.Denies(null, 1, why))      return false;

        // Mit einem Anbieter, der die Faehigkeit liefert: nichts sperrt mehr.
        ChefZ_Sym cap = ChefZ_SymbolTable.Intern("CHEFZ_EVTEST_CAP");
        ChefZ_CapabilityRegistry.Get().RegisterProvider( new ChefZ_EventTestProvider("Koennen", 1, cap, 9.0));
        if (ChefZ_CapabilityGate.Denies(reqs, 1, why))      return false;

        return true;
    }
}
