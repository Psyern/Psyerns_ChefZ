//==============================================================================
// ChefZ_EventBus - die einzige nach aussen offene Schnittstelle des Core
//
// Entwurf: 17 §3.2 (Schnittstelle woertlich), 17 §2 (warum die Umkehrung der
// Abhaengigkeit Invariante I4 strukturell sichert), 17 §7 (Datenfluss), 17 §8
// (Lebensdauer), 17 §9 (Fehlerverhalten, Zeile fuer Zeile), 17 E1 (Strings
// statt Enum), 17 E2 (HasSubscribers ist oeffentliche API), 17 E5
// (Stornierbarkeit), 17 E8 (kein Core-RPC).
//
// ---------------------------------------------------------------------------
// Wozu das gut ist - und was es strukturell verhindert
// ---------------------------------------------------------------------------
// Der Core enthaelt keinen Klassennamen, keinen Aufruf und keine Zeichenkette,
// die zu einem Fremdsystem gehoert. Er weiss nicht einmal, ob jemand zuhoert.
// Ein Comp-Modul abonniert in seiner EIGENEN MissionServer-Initialisierung:
//
//     ChefZ_EventBus.Get().Subscribe("ChefZ_OnRecipeCompleted", this,
//                                    ScriptCaller.Create(OnCooked), "MeinModul");
//
// Das Provider-Muster kehrt die Abhaengigkeit um. Es ist die einzige Bauform,
// bei der Invariante I4 nicht nur eingehalten, sondern unverletzbar ist -
// ein #ifdef oder ein optionaler Aufruf ueber IsKindOf waere bereits ein
// Fremdsystembezug, auch wenn er nie ausgefuehrt wird (17 §2).
//
// ---------------------------------------------------------------------------
// Die Kostenzusage (17 E2)
// ---------------------------------------------------------------------------
// Auf einem Server ohne Comp-Module kostet die gesamte Ereignisschicht EINEN
// Map-Zugriff je Ereignis. Der Aufrufer fragt zuerst HasSubscribers() und baut
// die Nutzlast gar nicht erst:
//
//     ChefZ_EventBus bus = ChefZ_EventBus.Get();
//     if (bus.HasSubscribers(ChefZ_EventNames.RECIPE_COMPLETED))
//     {
//         ChefZ_EventArgs a = bus.Acquire(ChefZ_EventNames.RECIPE_COMPLETED);
//         ...fuellen...
//         bus.Raise(a);           // gibt a an den Pool zurueck
//     }
//
// Das ist verbindlich und steht so an jeder Ausloesestelle des Core.
//
// ---------------------------------------------------------------------------
// Ein fremder Mod darf das Kochen nie anhalten (17 §9)
// ---------------------------------------------------------------------------
// Enforce hat kein try/catch - es gibt in der ganzen Vanilla-Skriptbasis 1.29
// keine einzige Stelle damit. Die Isolation entsteht deshalb STRUKTURELL, ueber
// die Aufrufform:
//
//     ScriptCaller.Invoke() ist ein natives proto (2_GameLib/DayZ/tools.c:167).
//     Ein Skriptfehler im Callback laeuft bis zu dieser nativen Grenze und
//     nicht darueber hinaus; die Engine protokolliert ihn, und der Aufruf
//     kehrt zurueck. Die Schleife hier laeuft weiter, der naechste Abonnent
//     wird gerufen, und der Kochvorgang kommt zu Ende.
//
// Deshalb wird JEDER Abonnent einzeln ueber einen eigenen ScriptCaller
// gerufen und nicht ueber einen gemeinsamen ScriptInvoker: ein Invoker haette
// alle Abonnenten hinter EINER nativen Grenze, kennte weder Namen noch
// Prioritaet und koennte einen Ausreisser nicht benennen. 17 §8 nennt zwar
// "map<string, ref ScriptInvoker>" als Zustandsform, aber 17 §3.2 gibt die
// Signatur mit ScriptCaller, subscriberName und priority vor - und diese drei
// sind mit einem Invoker nicht darstellbar. Die Signatur gewinnt.
//
// ---------------------------------------------------------------------------
// Kein RPC, keine Clientseite (17 E8, §7)
// ---------------------------------------------------------------------------
// Der Core feuert clientseitig KEINE Ereignisse. HasSubscribers() antwortet
// auf dem Client deshalb immer false - damit baut kein Aufrufer je eine
// Nutzlast, die niemand zugestellt bekaeme. Ein Modul, das etwas anzeigen
// will, schickt es selbst per RPC; es gibt keinen ChefZ-eigenen RPC und damit
// keine Angriffsflaeche ueber gefaelschte Client-Ereignisse.
//
// KEIN CONTENT und kein Fremdsystembezug.
//
// Layer: 3_Game.
//==============================================================================

/**
 * Eine einzelne Anmeldung.
 *
 * Getrennte Klasse und kein Tupel, weil sie ihren eigenen Lebenszyklus hat:
 * sie ueberlebt ihren Besitzer NICHT (siehe m_Owner) und traegt die Zaehler,
 * mit denen der Bus einen Ausreisser benennen kann.
 */
class ChefZ_EventSubscription : Managed
{
    int    id;
    string eventId;
    string subscriberName;
    int    priority;

    //! Der Rueckruf. ref, weil der Bus der einzige Halter ist - ein Comp-Modul
    //! erzeugt den ScriptCaller im Subscribe-Aufruf und vergisst ihn danach.
    ref ScriptCaller caller;

    /**
     * Der Besitzer - AUSDRUECKLICH OHNE ref (17 §9, letzte Zeile: "owner wird
     * als SCHWACHE Referenz gehalten; ist er weg, wird die Anmeldung beim
     * naechsten Raise entfernt").
     *
     * In Enforce ist ein Zeiger ohne ref genau das: er haelt das Objekt nicht
     * am Leben und wird null, wenn es stirbt. Mit ref waere der Bus ein
     * Speicherleck fuer jedes Comp-Modul, das sich nicht abmeldet - und
     * schlimmer: er hielte eine tote Mission am Leben.
     */
    Class  owner;

    //! Unterscheidet "nie einen Besitzer genannt" von "Besitzer ist gestorben".
    //! Ohne dieses Feld waere eine Anmeldung ohne Besitzer beim ersten Raise
    //! sofort wieder weg.
    bool   hasOwner;

    int    callCount;
    int    totalTicks;
    int    slowCount;

    //! 17 §9: "Abonnent setzt cancelled bei einem nicht stornierbaren Event ->
    //! ignoriert, WARN EINMAL JE ABONNENT." Der Merker sitzt deshalb hier und
    //! nicht in ChefZ_Log.Once - er soll je Anmeldung gelten, nicht je Prozess.
    bool   warnedCancel;
    bool   warnedBonus;

    void ChefZ_EventSubscription()
    {
        id             = 0;
        eventId        = "";
        subscriberName = "";
        priority       = 0;
        hasOwner       = false;
        callCount      = 0;
        totalTicks     = 0;
        slowCount      = 0;
        warnedCancel   = false;
        warnedBonus    = false;
    }

    //! Lebt der Besitzer noch? Eine Anmeldung ohne Besitzer lebt immer.
    bool IsAlive()
    {
        if (!hasOwner)
            return true;
        if (owner)
            return true;
        return false;
    }

    bool IsCallable()
    {
        if (!caller)
            return false;
        return caller.IsValid();
    }

    string Label()
    {
        string n = subscriberName;
        if (n == "")
            n = "<ohne Namen>";
        return n + "#" + id.ToString();
    }

    string ToLine()
    {
        string chefzTxt1 = "  " + Label() + "  -> " + eventId + "  prio=";
        chefzTxt1 = chefzTxt1 + priority.ToString() + "  aufrufe=" + callCount.ToString();
        string s = chefzTxt1;
        if (slowCount > 0)
            s = s + "  langsam=" + slowCount.ToString();
        if (!IsAlive())
            s = s + "  BESITZER TOT";
        return s;
    }
}

//==============================================================================

class ChefZ_EventBus : Managed
{
    private static ref ChefZ_EventBus s_Instance;

    //! Anmeldungen je Ereignisname, absteigend nach Prioritaet sortiert.
    private ref map<string, ref array<ref ChefZ_EventSubscription>> m_ByEvent;

    //! Nachschlager fuer Unsubscribe(id). Werte OHNE ref - Eigentuemer ist die
    //! Liste oben, und ein zweiter starker Halter waere eine Stelle, an der
    //! eine abgemeldete Anmeldung ueberlebt.
    private ref map<int, ChefZ_EventSubscription> m_ById;

    /**
     * Der Pool der Nutzlasten (17 §8), in ZWEI Listen.
     *
     * m_Owned haelt JEDE je erzeugte Nutzlast dauerhaft und stark - sie wird
     * dort nie entfernt. m_Free ist die Freiliste und haelt NICHTS: sie nennt
     * nur, welche der Objekte gerade niemand benutzt.
     *
     * Die Aufteilung ist kein Schmuck, sondern die einzige Bauform, die in
     * Enforce sicher ist. Waere die Freiliste die einzige starke Halterung,
     * dann wuerde
     *
     *     args = m_Free.Get(letzter);
     *     m_Free.Remove(letzter);
     *
     * die letzte Referenz loeschen, bevor der Aufrufer das Objekt bekommt.
     * Dieselbe Loesung wie im ChefZ_FactSnapshot ("Vorrat aller je erzeugten
     * Datensaetze. Haelt sie am Leben, waehrend die Sicht geleert wird").
     *
     * Die Groesse ist durch die Verschachtelungstiefe begrenzt: mehr als eine
     * Nutzlast gleichzeitig gibt es nur, wenn ein Abonnent im Rueckruf ein
     * Ereignis ausloest, und das ist auf m_MaxDepth gedeckelt.
     */
    private ref array<ref ChefZ_EventArgs> m_Owned;
    private ref array<ChefZ_EventArgs>     m_Free;
    private int m_InFlight;

    private int m_NextId;
    private int m_Depth;

    //--- Regler aus Core.json (17 §9) ----------------------------------------
    private int  m_MaxDepth;
    private bool m_Timing;
    private int  m_SlowMs;
    private int  m_MaxPool;

    private bool m_QuietForTest;
    private bool m_ServerForTest;

    //--- Zaehler fuer die Diagnose -------------------------------------------
    private int m_CountRaised;
    private int m_CountDelivered;
    private int m_CountCancelled;
    private int m_CountDeadPruned;
    private int m_CountDepthBlocked;

    //! Wer zuletzt storniert hat - fuer den Trace an der Ausloesestelle
    //! (17 §7: "Trace nennt den Abbrecher").
    private string m_LastCancelBy;

    static const int DEFAULT_MAX_DEPTH = 3;
    static const int DEFAULT_SLOW_MS   = 5;
    static const int DEFAULT_MAX_POOL  = 8;

    //--------------------------------------------------------------------------

    void ChefZ_EventBus()
    {
        m_ByEvent = new map<string, ref array<ref ChefZ_EventSubscription>>();
        m_ById    = new map<int, ChefZ_EventSubscription>();
        m_Owned   = new array<ref ChefZ_EventArgs>();
        m_Free    = new array<ChefZ_EventArgs>();

        m_NextId        = 1;                // 0 bleibt "keine Anmeldung"
        m_Depth         = 0;
        m_InFlight      = 0;
        m_MaxDepth      = DEFAULT_MAX_DEPTH;
        m_Timing        = false;
        m_SlowMs        = DEFAULT_SLOW_MS;
        m_MaxPool       = DEFAULT_MAX_POOL;
        m_QuietForTest  = false;
        m_ServerForTest = false;
        m_LastCancelBy  = "";

        ResetCounters();
    }

    static ChefZ_EventBus Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_EventBus();
        return s_Instance;
    }

    /**
     * Die Regler aus Core.json uebernehmen (17 §9).
     *
     * settings darf null sein - dann gelten die Code-Defaults. Der Bus
     * funktioniert ausdruecklich UNABHAENGIG von der Config (17 §9, Zeile
     * "Raise vor LoadAll()"): ein Comp-Modul, das sich vor dem Laden anmeldet,
     * soll das duerfen.
     */
    void Configure(ChefZ_CoreSettingsDef settings)
    {
        if (!settings)
            return;

        if (settings.eventMaxDepth > 0)
            m_MaxDepth = settings.eventMaxDepth;
        m_Timing = settings.eventTiming;
        if (settings.eventSlowSubscriberMs > 0)
            m_SlowMs = settings.eventSlowSubscriberMs;
    }

    //==========================================================================
    // Anmeldung (17 §3.2)
    //==========================================================================

    /**
     * Anmeldung ueber einen freien String - der Core fuehrt KEINE feste
     * Ereignisliste (17 E1).
     *
     * @param eventId        beliebig. Auch ein Name, den der Core nicht kennt:
     *                       Content loest ueber emitEvents eigene Ereignisse
     *                       aus, und ein kuenftiges Core-Ereignis soll vorab
     *                       abonnierbar bleiben. Unbekanntes gibt ein WARN und
     *                       wird angenommen (17 §9).
     * @param owner          darf null sein. Wird als SCHWACHE Referenz
     *                       gehalten; stirbt er, faellt die Anmeldung beim
     *                       naechsten Raise weg.
     * @param cb             ScriptCaller.Create(MeineMethode).
     * @param subscriberName erscheint in jeder Meldung ueber diesen Abonnenten.
     *                       Ohne ihn ist ein Ausreisser nicht zuzuordnen.
     * @param priority       hoeher zuerst; bei Gleichstand in Anmeldereihen-
     *                       folge. Deterministisch, damit zwei Server mit
     *                       denselben Mods dieselbe Reihenfolge haben.
     *
     * @return Anmelde-ID fuer Unsubscribe(), 0 bei Ablehnung.
     *
     * Eine Doppelanmeldung wird NICHT dedupliziert (17 §9): beide bleiben,
     * beide werden gerufen. Stille Magie waere hier schlimmer als eine
     * doppelte Zustellung - bei gleichem subscriberName gibt es ein WARN.
     */
    int Subscribe(string eventId, Class owner, ScriptCaller cb, string subscriberName, int priority = 0)
    {
        string id = eventId;
        id.TrimInPlace();

        if (id == "")
        {
            Warn("bus.sub.empty", "Abonnement ohne Ereignisnamen abgelehnt (Abonnent \"" + subscriberName + "\"). Ein leerer Name kann von nichts ausgeloest werden.");
            return 0;
        }

        if (!cb || !cb.IsValid())
        {
            Warn("bus.sub.nocb." + id + "." + subscriberName, "Abonnement auf \"" + id + "\" von \"" + subscriberName + "\" abgelehnt: der ScriptCaller ist leer oder ungueltig. " + "Erwartet wird ScriptCaller.Create(MeineMethode).");
            return 0;
        }

        if (!ChefZ_EventNames.IsCoreEvent(id))
        {
            // 17 §9: annehmen UND warnen. Ein Tippfehler soll sichtbar sein,
            // ein Content-Ereignis aus emitEvents trotzdem abonnierbar.
            Warn("bus.sub.unknown." + id, "\"" + subscriberName + "\" abonniert \"" + id + "\" - das ist kein " + "Systemereignis des Core. Das ist in Ordnung, wenn es aus emitEvents " + "eines Rezepts oder Prozesses kommt; sonst ist es ein Tippfehler. " + "Systemereignisse: " + ChefZ_EventNames.CoreEventNames());
        }

        array<ref ChefZ_EventSubscription> list = EnsureList(id);

        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_EventSubscription other = list.Get(i);
            if (other && subscriberName != "" && other.subscriberName == subscriberName)
            {
                Warn("bus.sub.dup." + id + "." + subscriberName, "\"" + subscriberName + "\" ist bereits fuer \"" + id + "\" angemeldet. Beide Anmeldungen bleiben und beide werden gerufen - " + "der Core dedupliziert nicht. Das ist selten Absicht.");
                break;
            }
        }

        ChefZ_EventSubscription sub = new ChefZ_EventSubscription();
        sub.id             = m_NextId;
        m_NextId++;
        sub.eventId        = id;
        sub.subscriberName = subscriberName;
        sub.priority       = priority;
        sub.caller         = cb;
        sub.owner          = owner;
        sub.hasOwner       = false;
        if (owner)
            sub.hasOwner = true;

        InsertByPriority(list, sub);
        m_ById.Set(sub.id, sub);

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.EVENT, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.EVENT, "Abonnent " + sub.Label() + " fuer \"" + id + "\" (prio " + priority.ToString() + ", jetzt " + list.Count().ToString() + ")");
        }

        return sub.id;
    }

    void Unsubscribe(int subscriptionId)
    {
        if (subscriptionId <= 0)
            return;

        ChefZ_EventSubscription sub;
        if (!m_ById.Find(subscriptionId, sub) || !sub)
            return;

        m_ById.Remove(subscriptionId);

        array<ref ChefZ_EventSubscription> list;
        if (m_ByEvent.Find(sub.eventId, list) && list)
        {
            int idx = IndexOf(list, subscriptionId);
            if (idx >= 0)
                list.RemoveOrdered(idx);        // Reihenfolge ist die Prioritaet
        }
    }

    /**
     * Massenabmeldung. Der vorgesehene Weg fuer ein Comp-Modul, dessen Mission
     * endet - und die Stelle, an der ein Modul aufraeumt, statt sich auf die
     * schwache Referenz zu verlassen.
     */
    void UnsubscribeOwner(Class owner)
    {
        if (!owner)
            return;

        array<string> names = new array<string>();
        EventNames(names);

        for (int n = 0; n < names.Count(); n++)
        {
            array<ref ChefZ_EventSubscription> list;
            if (!m_ByEvent.Find(names.Get(n), list) || !list)
                continue;

            for (int i = list.Count() - 1; i >= 0; i--)
            {
                ChefZ_EventSubscription sub = list.Get(i);
                if (!sub)
                {
                    list.RemoveOrdered(i);
                    continue;
                }
                if (sub.owner != owner)
                    continue;

                m_ById.Remove(sub.id);
                list.RemoveOrdered(i);
            }
        }
    }

    //==========================================================================
    // Nutzlast (17 §8)
    //==========================================================================

    /**
     * Eine Nutzlast aus dem Pool holen, zurueckgesetzt und mit gesetztem
     * eventId und worldTime.
     *
     * Der Rueckgabewert gehoert dem BUS. Er ist nach Raise() ungueltig, und
     * Raise() gibt ihn selbst zurueck - ein Aufrufer, der ihn danach noch
     * liest, liest beim naechsten Ereignis fremde Daten.
     */
    ChefZ_EventArgs Acquire(string eventId)
    {
        ChefZ_EventArgs args;

        int last = m_Free.Count() - 1;
        if (last >= 0)
        {
            // Sicher, weil m_Owned die starke Halterung ist: das Entfernen aus
            // der Freiliste kann das Objekt nicht einsammeln.
            args = m_Free.Get(last);
            m_Free.Remove(last);
            args.Reset();
        }
        else
        {
            args = new ChefZ_EventArgs();
            m_Owned.Insert(args);

            if (m_Owned.Count() > m_MaxPool)
            {
                // Kein Abbruch - eine fehlende Nutzlast waere schlimmer als
                // eine zu viel. Aber es ist ein Befund: mehr gleichzeitig
                // benutzte Nutzlasten als erlaubte Verschachtelungstiefe heisst
                // fast immer, dass ein Aufrufer Acquire() ohne Release() ruft.
                Warn("bus.pool.grow", "Der Ereignispool ist auf " + m_Owned.Count().ToString() + " Nutzlasten gewachsen (Grenze " + m_MaxPool.ToString() + "). Ursache ist fast immer ein Acquire() ohne passendes " + "Raise() oder Release().");
            }
        }

        args.eventId   = eventId;
        args.worldTime = WorldTime();
        m_InFlight++;
        return args;
    }

    /**
     * Eine Nutzlast zurueckgeben. Wird von Raise(), RaiseCancellable() und
     * RaiseQuery() selbst gerufen; ein Aufrufer braucht sie nur, wenn er eine
     * Nutzlast gebaut und dann DOCH nicht ausgeloest hat - oder wenn er
     * RaiseKeep() benutzt hat.
     *
     * Zurueckgesetzt wird SOFORT und nicht erst beim naechsten Acquire: ein
     * Abonnent, der sich die Nutzlast entgegen 17 §8 gemerkt hat, findet dann
     * eine leere vor statt der Daten des letzten Gerichts. Das macht den
     * Fehler sichtbar, statt ihn in eine falsche Statistik zu verwandeln.
     */
    void Release(ChefZ_EventArgs args)
    {
        if (!args)
            return;

        m_InFlight--;
        if (m_InFlight < 0)
            m_InFlight = 0;

        args.Reset();

        // Doppelte Rueckgabe abfangen: sie wuerde dasselbe Objekt zweimal in
        // die Freiliste legen, und zwei Aufrufer haetten danach dieselbe
        // Nutzlast in der Hand.
        if (m_Free.Find(args) >= 0)
            return;

        m_Free.Insert(args);
    }

    //==========================================================================
    // Ausloesen (17 §7)
    //==========================================================================

    /**
     * Reine Benachrichtigung. Rueckkanal wird NICHT ausgewertet.
     *
     * args ist danach ungueltig - der Bus gibt es an den Pool zurueck.
     */
    void Raise(notnull ChefZ_EventArgs args)
    {
        Dispatch(args);
        Release(args);
    }

    /**
     * Zustellen OHNE die Nutzlast zurueckzugeben.
     *
     * Fuer den einen Fall, in dem DIESELBE Nutzlast mehrfach gebraucht wird:
     * ein abgeschlossenes Rezept loest ChefZ_OnRecipeCompleted aus, meldet
     * denselben Vorgang an das Fortschrittsregister und feuert danach die
     * emitEvents des Rezepts. Dreimal dieselben Zutaten, dieselben Ergebnisse,
     * dieselbe Stufe - dreimal neu zu bauen waere Verschwendung, und drei
     * leicht verschiedene Nutzlasten waeren ein Fehler, der niemandem
     * auffiele.
     *
     * WER RAISEKEEP RUFT, RUFT DANACH RELEASE. Sonst waechst der Pool nicht,
     * aber der Zaehler der unterwegs befindlichen Nutzlasten laeuft davon und
     * die Diagnose luegt.
     */
    void RaiseKeep(notnull ChefZ_EventArgs args)
    {
        Dispatch(args);
    }

    /**
     * Ausloesen und die Stornierung auswerten (17 E5).
     *
     * NUR fuer Ereignisse, die ChefZ_EventNames.IsCancellable() bejaht - bei
     * allen anderen liefert die Funktion immer false, weil Dispatch() eine
     * Stornierung dort verwirft.
     *
     * @param cancelBy wer storniert hat. Gehoert in den Trace, damit ein
     *                 Betreiber nicht raten muss, welcher Mod ihm das Kochen
     *                 abgedreht hat (17 §7).
     * @return true = die Wirkung UNTERBLEIBT. Der Aufrufer faellt dann auf
     *         Vanilla zurueck und veraendert NICHTS.
     */
    bool RaiseCancellable(notnull ChefZ_EventArgs args, out string cancelReason, out string cancelBy)
    {
        Dispatch(args);

        bool cancelled = args.cancelled;
        cancelReason   = args.cancelReason;
        cancelBy       = m_LastCancelBy;

        if (cancelled)
            m_CountCancelled++;

        Release(args);
        return cancelled;
    }

    /**
     * Das EINE Abfrage-Event (17 §5, E6).
     *
     * Der Beitrag der Abonnenten wird auf maxBonus GEKLEMMT - symmetrisch, ein
     * Abonnent darf auch nicht beliebig abwerten. Kein Abonnent kann die
     * Qualitaetslogik uebernehmen; die Hoheit ueber Stufen und Schwellen
     * bleibt beim Core.
     *
     * @param maxBonus aus CoreSettings.maxExternalQualityBonus. <= 0 heisst
     *                 "kein externer Bonus" und ist eine gueltige Einstellung.
     */
    float RaiseQuery(notnull ChefZ_EventArgs args, float maxBonus)
    {
        Dispatch(args);

        float bonus = args.bonusPoints;
        float limit = maxBonus;
        if (limit < 0.0)
            limit = 0.0;

        if (!ChefZ_EventArgs.IsFinite(bonus))
        {
            Warn("bus.query.nan." + args.eventId, "Die Abfrage \"" + args.eventId + "\" hat keine Zahl ergeben. " + "Der externe Bonus gilt als 0.");
            bonus = 0.0;
        }
        else if (bonus > limit)
        {
            Warn("bus.query.high." + args.eventId, "Externer Qualitaetsbonus " + bonus.ToString() + " ueberschreitet " + "maxExternalQualityBonus (" + limit.ToString() + ") und wird geklemmt. " + "Beitraege kamen von: " + SubscriberNames(args.eventId));
            bonus = limit;
        }
        else if (bonus < -limit)
        {
            Warn("bus.query.low." + args.eventId, "Externer Qualitaetsabzug " + bonus.ToString() + " unterschreitet " + (-limit).ToString() + " und wird geklemmt. Beitraege kamen von: " + SubscriberNames(args.eventId));
            bonus = -limit;
        }

        Release(args);
        return bonus;
    }

    //--------------------------------------------------------------------------

    /**
     * Die Zustellung. Die einzige Stelle, an der der Core fremden Code ruft.
     *
     * Reihenfolge der Pruefungen und ihre Begruendung:
     *   1. leerer Name    -> nichts zuzustellen, WARN. Fast immer ein Bug im
     *                        Aufrufer.
     *   2. Clientseite    -> No-op mit WARN (17 §9). Der Core feuert
     *                        clientseitig nichts.
     *   3. keine Liste    -> sofort zurueck. DER Normalfall auf einem Server
     *                        ohne Comp-Module, und er kostet einen Map-Zugriff.
     *   4. Tiefe          -> ab m_MaxDepth Abbruch mit ERROR. Verhindert eine
     *                        Endlosschleife zwischen zwei Modulen, die sich
     *                        gegenseitig Ereignisse zuwerfen (17 §9).
     */
    private void Dispatch(notnull ChefZ_EventArgs args)
    {
        string id = args.eventId;
        m_LastCancelBy = "";

        if (id == "")
        {
            Warn("bus.raise.empty", "Raise() mit leerem Ereignisnamen - es wird nichts zugestellt.");
            return;
        }

        if (!IsServerSide())
        {
            Warn("bus.raise.client", "Raise(\"" + id + "\") auf dem Client - wirkungslos. Der Core feuert " + "clientseitig keine Ereignisse (17 §7). Wer etwas anzeigen will, " + "schickt es selbst.");
            return;
        }

        array<ref ChefZ_EventSubscription> list;
        if (!m_ByEvent.Find(id, list) || !list || list.Count() == 0)
            return;

        m_CountRaised++;

        if (m_Depth >= m_MaxDepth)
        {
            m_CountDepthBlocked++;
            Err("bus.depth." + id, "Ereignistiefe " + m_MaxDepth.ToString() + " erreicht bei \"" + id + "\" - die Zustellung wird abgebrochen. Ursache ist immer ein Abonnent, " + "der im Rueckruf selbst Ereignisse ausloest, die wieder bei ihm landen. " + "Das Kochen laeuft davon unbeeindruckt weiter.");
            return;
        }

        bool cancellable = ChefZ_EventNames.IsCancellable(id);
        bool isQuery     = ChefZ_EventNames.IsQuery(id);

        // Kopie, weil ein Abonnent im Rueckruf abonnieren oder sich abmelden
        // darf - beides veraendert die Liste, ueber die gerade gelaufen wird.
        // Die Kopie entsteht NUR, wenn es ueberhaupt Abonnenten gibt; auf
        // einem Server ohne Comp-Module wird sie nie angelegt.
        //
        // MIT ref, und das ist der Punkt: meldet sich ein Abonnent WAEHREND
        // der Zustellung selbst ab, faellt die starke Halterung in m_ByEvent
        // weg. Ohne ref stuende hier ab der naechsten Zeile ein Loch in der
        // Kopie - mit ref ueberlebt jede Anmeldung genau diesen einen
        // Durchlauf, und danach raeumt Enforce auf.
        array<ref ChefZ_EventSubscription> snapshot = new array<ref ChefZ_EventSubscription>();
        for (int c = 0; c < list.Count(); c++)
            snapshot.Insert(list.Get(c));

        m_Depth++;

        bool sawDead = false;

        for (int i = 0; i < snapshot.Count(); i++)
        {
            ChefZ_EventSubscription sub = snapshot.Get(i);
            if (!sub)
                continue;

            if (!sub.IsAlive())
            {
                sawDead = true;
                continue;
            }
            if (!sub.IsCallable())
            {
                sawDead = true;
                continue;
            }

            bool  hadCancel = args.cancelled;
            float hadBonus  = args.bonusPoints;

            int t0 = 0;
            if (m_Timing)
                t0 = TickNow();

            // ------------------------------------------------------------------
            // DIE GRENZE. Alles jenseits davon ist fremder Code.
            // ScriptCaller.Invoke ist nativ; ein Fehler dort kommt hier nicht
            // an, und der naechste Abonnent wird trotzdem gerufen.
            // ------------------------------------------------------------------
            sub.caller.Invoke(args);

            sub.callCount++;
            m_CountDelivered++;

            if (m_Timing)
            {
                int dt = TickNow() - t0;
                if (dt < 0)
                    dt = 0;
                sub.totalTicks = sub.totalTicks + dt;
                if (dt >= m_SlowMs)
                {
                    sub.slowCount++;
                    SlowWarn("bus.slow." + sub.Label(), "Abonnent " + sub.Label() + " hat fuer \"" + id + "\" " + dt.ToString() + " ms gebraucht (Grenze " + m_SlowMs.ToString() + " ms). Ereignisbehandlung muss kurz sein - eine Endlosschleife " + "im Rueckruf ist in Enforce nicht abfangbar.");
                }
            }

            // 17 §9: Stornierung bei einem nicht stornierbaren Ereignis wird
            // ignoriert und EINMAL JE ABONNENT gewarnt. Stillschweigen waere
            // schlimmer - der fremde Modautor glaubte sonst, seine Sperre wirke.
            if (!cancellable && args.cancelled != hadCancel)
            {
                if (!sub.warnedCancel)
                {
                    sub.warnedCancel = true;
                    Warn("bus.cancel.notallowed." + sub.Label() + "." + id, "Abonnent " + sub.Label() + " storniert \"" + id + "\", aber dieses Ereignis ist nicht stornierbar - es meldet eine " + "bereits eingetretene Wirkung. Die Stornierung wird ignoriert. " + "Stornierbar sind nur Ereignisse VOR einer Wirkung: " + ChefZ_EventNames.RECIPE_MATCHED + ", " + ChefZ_EventNames.PROCESS_JOB_STARTED + ", " + ChefZ_EventNames.PORTION_TAKEN + ".");
                }
                args.cancelled    = hadCancel;
                args.cancelReason = "";
            }

            // Dasselbe fuer den Punktebeitrag: es gibt genau EIN Abfrage-Event
            // (17 E6). Ein Beitrag anderswo ist wirkungslos und wird gemeldet.
            if (!isQuery && args.bonusPoints != hadBonus)
            {
                if (!sub.warnedBonus)
                {
                    sub.warnedBonus = true;
                    Warn("bus.bonus.notallowed." + sub.Label() + "." + id, "Abonnent " + sub.Label() + " traegt Punkte zu \"" + id + "\" bei, aber das einzige Abfrage-Ereignis des Core ist " + ChefZ_EventNames.QUALITY_BONUS_QUERY + ". Der Beitrag " + "bleibt wirkungslos.");
                }
                args.bonusPoints = hadBonus;
            }

            if (cancellable && args.cancelled && !hadCancel)
            {
                // Wer storniert hat, gehoert in den Trace (17 §7).
                m_LastCancelBy = sub.Label();

                if (ChefZ_Log.Enabled(ChefZ_LogChannel.EVENT, ChefZ_LogLevel.DEBUG))
                {
                    ChefZ_Log.Debug(ChefZ_LogChannel.EVENT, "Storniert von " + sub.Label() + ": " + args.ToDebugString());
                }

                // Zustellung beenden. Die Wirkung findet nicht statt, und ein
                // weiterer Abonnent koennte sie weder zurueckholen noch
                // sinnvoll darauf reagieren.
                break;
            }
        }

        m_Depth--;

        if (sawDead)
            PruneDead(list);

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.EVENT, ChefZ_LogLevel.TRACE))
            ChefZ_Log.Trace(ChefZ_LogChannel.EVENT, "Zugestellt: " + args.ToDebugString());
    }

    //==========================================================================
    // Auskunft (17 §3.2, E2)
    //==========================================================================

    /**
     * Hoert ueberhaupt jemand zu? (17 E2)
     *
     * Teil der oeffentlichen API MIT ABSICHT: der Core baut teure Nutzlasten -
     * Klassenlisten, Tag-Listen, Trace-Ausschnitte - nur, wenn jemand zuhoert.
     * Ohne Comp-Module kostet die Ereignisschicht damit einen Map-Zugriff pro
     * Ereignis und sonst nichts.
     *
     * Auf dem Client immer false (17 §7). Tote Anmeldungen werden hier NICHT
     * geprueft: die Pruefung waere teurer als die Ersparnis, und ein zu viel
     * gebauter Kontext ist folgenlos - Dispatch() raeumt sie ohnehin auf.
     */
    bool HasSubscribers(string eventId)
    {
        if (!IsServerSide())
            return false;

        array<ref ChefZ_EventSubscription> list;
        if (!m_ByEvent.Find(eventId, list) || !list)
            return false;
        return list.Count() > 0;
    }

    int GetSubscriberCount(string eventId)
    {
        array<ref ChefZ_EventSubscription> list;
        if (!m_ByEvent.Find(eventId, list) || !list)
            return 0;
        return list.Count();
    }

    //! Weiterreichung von ChefZ_EventNames, damit ein Comp-Modul die Antwort
    //! ueber den Bus bekommt, den es ohnehin in der Hand hat (17 §3.2).
    bool IsCancellable(string eventId)
    {
        return ChefZ_EventNames.IsCancellable(eventId);
    }

    void DumpSubscribers(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        array<string> names = new array<string>();
        EventNames(names);
        ChefZ_StringOrder.SortAscending(names);

        string chefzTxt2 = "Event Bus: " + names.Count().ToString() + " Ereignisse, " + m_ById.Count().ToString() + " Anmeldungen";
        chefzTxt2 = chefzTxt2 + "  ausgeloest=" + m_CountRaised.ToString() + "  zugestellt=" + m_CountDelivered.ToString() + "  storniert=";
        chefzTxt2 = chefzTxt2 + m_CountCancelled.ToString() + "  tot entfernt=" + m_CountDeadPruned.ToString() + "  tiefenabbruch=" + m_CountDepthBlocked.ToString();
        outLines.Insert(chefzTxt2);

        if (names.Count() == 0)
        {
            outLines.Insert("  (kein Abonnent - die Ereignisschicht kostet nichts)");
            return;
        }

        for (int n = 0; n < names.Count(); n++)
        {
            array<ref ChefZ_EventSubscription> list;
            if (!m_ByEvent.Find(names.Get(n), list) || !list)
                continue;
            for (int i = 0; i < list.Count(); i++)
            {
                ChefZ_EventSubscription sub = list.Get(i);
                if (sub)
                    outLines.Insert(sub.ToLine());
            }
        }
    }

    //! Alle Anmeldungen ueber alle Ereignisse. Fuer die Startzeile: ein
    //! Betreiber soll auf einen Blick sehen, ob ueberhaupt ein Comp-Modul an
    //! ChefZ haengt - das ist die haeufigste Frage bei "warum tut Modul X
    //! nichts".
    int GetSubscriptionCount() { return m_ById.Count(); }

    int GetRaisedCount()     { return m_CountRaised; }
    int GetDeliveredCount()  { return m_CountDelivered; }
    int GetCancelledCount()  { return m_CountCancelled; }
    int GetInFlightCount()   { return m_InFlight; }
    int GetDepth()           { return m_Depth; }
    string GetLastCancelBy() { return m_LastCancelBy; }

    void ResetCounters()
    {
        m_CountRaised       = 0;
        m_CountDelivered    = 0;
        m_CountCancelled    = 0;
        m_CountDeadPruned   = 0;
        m_CountDepthBlocked = 0;
    }

    /**
     * Alle Anmeldungen loeschen.
     *
     * Vorgesehene Aufrufer: der Selbsttest und der SAFE_MODE (02 §8). Ein Core,
     * der sich abschaltet, soll keine Ereignisse mehr zustellen - ein
     * Comp-Modul wuerde sonst XP fuer Vorgaenge vergeben, die ChefZ gar nicht
     * mehr ausloest.
     */
    void ClearAll()
    {
        m_ByEvent.Clear();
        m_ById.Clear();
        m_Depth        = 0;
        m_LastCancelBy = "";
    }

    //==========================================================================
    // Innenleben
    //==========================================================================

    private array<ref ChefZ_EventSubscription> EnsureList(string eventId)
    {
        array<ref ChefZ_EventSubscription> list;
        if (m_ByEvent.Find(eventId, list) && list)
            return list;

        list = new array<ref ChefZ_EventSubscription>();
        m_ByEvent.Set(eventId, list);
        return list;
    }

    //! Absteigend nach Prioritaet, bei Gleichstand hinten anfuegen. Damit ist
    //! die Reihenfolge vollstaendig bestimmt: Prioritaet, dann Anmeldezeit.
    private void InsertByPriority(notnull array<ref ChefZ_EventSubscription> list, notnull ChefZ_EventSubscription sub)
    {
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_EventSubscription other = list.Get(i);
            if (other && other.priority < sub.priority)
            {
                list.InsertAt(sub, i);
                return;
            }
        }
        list.Insert(sub);
    }

    private int IndexOf(notnull array<ref ChefZ_EventSubscription> list, int id)
    {
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_EventSubscription sub = list.Get(i);
            if (sub && sub.id == id)
                return i;
        }
        return -1;
    }

    /**
     * Anmeldungen ohne lebenden Besitzer entfernen (17 §9, letzte Zeile).
     *
     * Amortisiert und nur nach einem Raise, in dem eine tote Anmeldung
     * auffiel - kein eigener Timer. Ein zweiter Taktgeber fuer eine Aufgabe,
     * die beim Durchlaufen ohnehin anfaellt, waere Aufwand ohne Gewinn.
     */
    private void PruneDead(notnull array<ref ChefZ_EventSubscription> list)
    {
        for (int i = list.Count() - 1; i >= 0; i--)
        {
            ChefZ_EventSubscription sub = list.Get(i);
            if (!sub)
            {
                list.RemoveOrdered(i);
                continue;
            }
            if (sub.IsAlive() && sub.IsCallable())
                continue;

            if (ChefZ_Log.Enabled(ChefZ_LogChannel.EVENT, ChefZ_LogLevel.DEBUG))
            {
                ChefZ_Log.Debug(ChefZ_LogChannel.EVENT, "Abonnent " + sub.Label() + " entfernt - Besitzer oder Rueckruf " + "existiert nicht mehr.");
            }

            m_ById.Remove(sub.id);
            list.RemoveOrdered(i);
            m_CountDeadPruned++;
        }
    }

    private void EventNames(out array<string> outNames)
    {
        if (!outNames)
            outNames = new array<string>();
        for (int i = 0; i < m_ByEvent.Count(); i++)
            outNames.Insert(m_ByEvent.GetKey(i));
    }

    //! Namen aller Abonnenten eines Ereignisses, fuer Meldungen ueber einen
    //! unplausiblen Beitrag (17 §9: "WARN mit Abonnentenname").
    private string SubscriberNames(string eventId)
    {
        array<ref ChefZ_EventSubscription> list;
        if (!m_ByEvent.Find(eventId, list) || !list || list.Count() == 0)
            return "(niemand)";

        string s = "";
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_EventSubscription sub = list.Get(i);
            if (!sub)
                continue;
            if (s != "")
                s = s + ", ";
            s = s + sub.Label();
        }
        return s;
    }

    /**
     * Seite. g_Game existiert in 3_Game; im Selbsttest laeuft der Bus aber
     * auch ohne Mission, und dort gilt die Seite, die 5_Mission dem Log
     * gemeldet hat.
     */
    private bool IsServerSide()
    {
        if (m_ServerForTest)
            return true;
        if (g_Game)
            return g_Game.IsServer();
        return ChefZ_Log.IsServerSide();
    }

    private int TickNow()
    {
        if (g_Game)
            return g_Game.GetTime();
        return 0;
    }

    //! Missionszeit in Sekunden. 0, solange es keine Mission gibt - das ist
    //! ehrlicher als eine erfundene Zahl.
    private float WorldTime()
    {
        if (!g_Game)
            return 0.0;
        float ms = g_Game.GetTime();
        return ms * 0.001;
    }

    private void Warn(string key, string message)
    {
        if (m_QuietForTest)
            return;
        ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.EVENT, key, message);
    }

    private void Err(string key, string message)
    {
        if (m_QuietForTest)
            return;
        ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.EVENT, key, message);
    }

    //! Ausreisser gehen auf den Kanal PERF und nicht EVENT: sie sind eine
    //! Leistungsaussage ueber einen fremden Mod, keine Aussage ueber das
    //! Kochen (18 §2.1).
    private void SlowWarn(string key, string message)
    {
        if (m_QuietForTest)
            return;
        ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PERF, key, message);
    }

    //==========================================================================
    // Selbsttest-Zugaenge (S13)
    //==========================================================================

    //! Der Selbsttest laeuft auf einer EIGENEN Instanz, nicht auf dem
    //! Singleton - dieselbe Loesung wie in jedem anderen ChefZ-Selbsttest und
    //! aus demselben Grund: er darf den echten Bestand des Servers nicht
    //! anfassen.
    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }

    //! Ohne laufende Mission gibt es kein g_Game.IsServer(). Der Selbsttest
    //! sagt dem Bus deshalb ausdruecklich, dass er auf der Serverseite steht.
    void SetServerForTest(bool isServer)
    {
        m_ServerForTest = isServer;
    }

    void SetMaxDepthForTest(int depth)
    {
        if (depth > 0)
            m_MaxDepth = depth;
    }

    int GetMaxDepth()
    {
        return m_MaxDepth;
    }
}
