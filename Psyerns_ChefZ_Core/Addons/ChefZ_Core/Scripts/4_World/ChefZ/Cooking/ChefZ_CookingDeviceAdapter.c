//==============================================================================
// ChefZ_CookingDeviceAdapter - Stufe 0 / A / B / C
//
// Entwurf: 10 §4 (Schnittstelle woertlich), 10 §5 (Datenfluss, drei Stufen mit
// steigenden Kosten), 10 §6 (Abschlussbedingung), 10 §7 (Zustand und
// Alterung), 10 §8 (Fehlerverhalten, Zeile fuer Zeile), 10 E4 (dreistufige
// Drosselung), 10 E5 (Fingerprint), 10 E6 (SUPPRESSED), 10 E7 (zweite
// Sicherung ueber CfgChefZDevices), 19 S7 und 19 S8 (Umfang der beiden
// Schritte), 08 §6 (die Transaktion, die Stufe C seit S8 ausloest).
//
// ---------------------------------------------------------------------------
// WAS DIESE DATEI VERAENDERT - UND WAS NICHT
// ---------------------------------------------------------------------------
// Sie selbst veraendert nichts. Kein Delete, kein CreateInInventory, kein
// AddQuantity, kein SetQuantity. Sie ENTSCHEIDET, und wenn die Entscheidung
// "fertig" lautet, ruft sie den ChefZ_Applicator - die einzige Stelle des
// Core, die Items erzeugt und verbraucht (08 §6, Invariante I5).
//
// Bis S7 gab es diesen Aufruf nicht, und das war der Zweck jenes Schritts:
// der komplette Entscheidungspfad war auf echten Feuerstellen beobachtbar,
// waehrend ChefZ garantiert nichts veraendern KONNTE. Wer diesen Zustand
// wiederherstellen will - etwa um einen Verdacht einzugrenzen -, setzt
// enabled = 0 in der Core-Konfiguration; dann ist der Hook ein Bool-Test.
//
// Der Uebergang bleibt eng: Complete() ist die EINZIGE Stelle dieser Datei,
// die den Applicator ruft, und sie ruft ihn ausschliesslich mit Puffern, die
// in DEMSELBEN Tick fuer DIESES Gefaess erhoben wurden (siehe
// CheckCompletion).
//
// ---------------------------------------------------------------------------
// Die drei Stufen und warum es drei sind (10 E4)
// ---------------------------------------------------------------------------
//   Stufe 0  Torwaechter    ein paar Vergleiche, jeden Tick
//                           faengt den haeufigsten Fall ab: ein Topf mit
//                           Wasser, an dem kein Rezept je etwas zu tun hat
//   Stufe A  Signatur       ein Cargo-Durchlauf OHNE Registry-Zugriff
//                           faengt den zweithaeufigsten ab: Inhalt unveraendert
//   Stufe B  Vollmatch      nur bei echter Aenderung, zusaetzlich gedrosselt
//   Stufe C  Abschluss      nur bei gebundenem Rezept
//
// ---------------------------------------------------------------------------
// "Tick" heisst hier Millisekunde
// ---------------------------------------------------------------------------
// Siehe Kopf von ChefZ_CookSession. AgeSessions(int currentTick) bekommt
// TickCount(0), also Millisekunden seit Prozessstart - dieselbe Uhr, die
// ChefZ_Log fuer seine Flushgrenze benutzt.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_CookingDeviceAdapter
{
    /**
     * Fehlversuche am selben Gefaess bis SUPPRESSED (10 E6).
     *
     * Bewusst KEINE Einstellung: die Drei steht im Entwurf und ist keine
     * Geschmacksfrage, sondern die Grenze, ab der ein Problem einmal sichtbar
     * statt tausendfach im Log steht. Wer sie erhoeht, bekommt mehr Rauschen;
     * wer sie senkt, sperrt bei einem Zufall.
     */
    static const int FAIL_LIMIT = 3;

    /**
     * Alle wie viele NEUEN Sitzungen wird aufgeraeumt (10 §7: "amortisiert
     * beim Einfuegen, nicht ueber einen eigenen Timer").
     *
     * Ebenfalls keine Einstellung: eine Aufraeumfrequenz beeinflusst nichts,
     * was ein Betreiber beobachten oder wollen koennte.
     */
    static const int SWEEP_EVERY_INSERTS = 32;

    /**
     * Notbremse gegen ein haengendes m_Executing (10 §8, "Reentranz").
     *
     * Enforce kennt kein finally. Bricht die Auswertung mitten im geschuetzten
     * Abschnitt ab, bliebe das Flag sonst fuer immer gesetzt und der Adapter
     * waere bis zum Serverneustart taub. Nach dieser Zeitspanne gilt ein
     * gesetztes Flag als verwaist.
     */
    static const int EXECUTING_STALE_MS = 5000;

    private static ref ChefZ_CookingDeviceAdapter s_Instance;

    //--- Zustand (10 §7: Laufzeit, nicht persistiert, nicht synchronisiert) ---
    private ref map<int, ref ChefZ_CookSession> m_Sessions;

    /**
     * Klassensymbol -> Deskriptor, EINSCHLIESSLICH der Negativfaelle.
     *
     * Der Negativeintrag ist der wichtigere: "diese Klasse ist kein
     * ChefZ-Geraet" ist die Antwort fuer die grosse Mehrheit aller Gefaesse,
     * und ohne Zwischenspeicher liefe dafuer bei JEDEM Tick ein Aufstieg
     * durch die CfgVehicles-Kette.
     */
    private ref map<int, ref ChefZ_DeviceDescriptor> m_DeviceCache;

    //--- Wiederverwendete Puffer (10 §7) --------------------------------------
    private ref ChefZ_VesselSignature m_Scratch;
    private ref ChefZ_CookContext     m_Ctx;
    private ref ChefZ_FactSnapshot    m_Snapshot;
    private ref array<ItemBase>       m_Entities;

    //! Zwischenergebnis der Qualitaetsrechnung (12 §5). Wiederverwendet wie
    //! alle Puffer hier; es lebt nur bis zur naechsten Anwendung und wird
    //! ausdruecklich NICHT persistiert (12 §7).
    private ref ChefZ_QualityEvaluation m_Eval;

    //--- Reentranzschutz (10 §8) ---------------------------------------------
    private bool m_Executing;
    private int  m_ExecutingSince;

    private int m_InsertsSinceSweep;

    //--- Zaehler fuer "chefz stats" (18 §2) ----------------------------------
    private int m_CountObserved;
    private int m_CountSignatureHits;
    private int m_CountFullMatches;
    private int m_CountMatched;
    private int m_CountCompleted;
    private int m_CountSweptSessions;

    //==========================================================================

    void ChefZ_CookingDeviceAdapter()
    {
        m_Sessions    = new map<int, ref ChefZ_CookSession>();
        m_DeviceCache = new map<int, ref ChefZ_DeviceDescriptor>();
        m_Scratch     = new ChefZ_VesselSignature();
        m_Ctx         = new ChefZ_CookContext();
        m_Snapshot    = new ChefZ_FactSnapshot();
        m_Entities    = new array<ItemBase>();
        m_Eval        = new ChefZ_QualityEvaluation();

        m_Executing         = false;
        m_ExecutingSince    = 0;
        m_InsertsSinceSweep = 0;
        ResetCounters();
    }

    static ChefZ_CookingDeviceAdapter Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_CookingDeviceAdapter();
        return s_Instance;
    }

    //==========================================================================
    // Der Eintritt aus dem Hook
    //==========================================================================

    /**
     * Ein Kochtick, NACHDEM Vanilla vollstaendig gelaufen ist.
     *
     * @param device     das Gefaess. Darf null sein.
     * @param timeCoef   cooking_time_coef aus Cooking.CookWithEquipment.
     * @param updateTime m_UpdateTime der Cooking-Instanz (01 V11: die einzige
     *                   verlaessliche Zeitquelle fuer TIMED).
     * @param method     CookingMethodType dieses Ticks, von Vanilla erfragt
     *                   und nicht nachgebaut (10 E3).
     *
     * KEIN Rueckgabewert und kein out-Parameter. Das ist Absicht und laut
     * 10 §3 nicht verhandelbar: der Adapter bekommt keinen Kanal, ueber den er
     * den bereits gelaufenen Vanilla-Tick beeinflussen koennte. Regel §10.2
     * ist damit Struktur und nicht Selbstdisziplin.
     */
    void Observe(ItemBase device, float timeCoef, float updateTime, int method)
    {
        int now = TickCount(0);

        // --- Reentranz (10 §8) -----------------------------------------------
        // DayZ-Script ist einstraengig, aber ein Abonnent eines ChefZ-Events
        // koennte zurueckrufen. Ein zweiter Durchlauf im selben Aufrufstapel
        // arbeitete auf denselben wiederverwendeten Puffern.
        if (m_Executing)
        {
            if ((now - m_ExecutingSince) < EXECUTING_STALE_MS)
                return;

            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.COOK, "cook.reentrancy.stale", "Der Reentranzschutz des Kochadapters war laenger als " + EXECUTING_STALE_MS.ToString() + " ms gesetzt und wird zurueckgesetzt. " + "Ursache ist ein Abbruch mitten in der Auswertung. Vanilla-Kochen ist " + "davon unberuehrt.");
        }

        m_Executing      = true;
        m_ExecutingSince = now;

        ObserveInner(device, timeCoef, updateTime, method, now);

        m_Executing = false;
    }

    //==========================================================================
    // Stufe 0 - Torwaechter (10 §5)
    //==========================================================================

    /**
     * Alle Bedingungen, die ein Gefaess erfuellen muss, damit ChefZ ueberhaupt
     * hinsieht. Jede einzelne endet in einem return - keine davon meldet
     * etwas, weil keine davon ein Fehler ist (10 §8).
     *
     * Sie ist ausserdem oeffentlich, damit der Hook sie stellen kann, BEVOR er
     * Vanilla nach der Kochmethode fragt: GetCookingMethodWithTimeOverride
     * laeuft durchs Cargo, und 19 S7 verlangt "bei leerem Rezeptbestand kostet
     * der Hook einen Bool-Test".
     */
    bool PassesGate(ItemBase device, out ChefZ_DeviceDescriptor desc, out int cargoCount)
    {
        desc       = null;
        cargoCount = 0;

        // Nichts Autoritatives auf dem Client. Der Client sieht das Ergebnis
        // ueber die normale Inventar- und Variablensynchronisation (10 §5).
        if (!g_Game || !g_Game.IsServer())
            return false;

        if (!ChefZ_CookingHook.IsEnabled())
            return false;

        // Vanilla hat bei beidem bereits selbst abgebrochen (Cooking.c:120).
        if (!device || device.IsRuined())
            return false;

        // Ueber eine lokale Zwischenvariable und nicht direkt in den
        // out-Parameter: einen out-Parameter als out-Parameter weiterzureichen
        // ist in Enforce nirgends zugesichert. Dieselbe Vorsicht steht bereits
        // im Kopf von ChefZ_TextList.SymbolsOf.
        ChefZ_DeviceDescriptor found;
        if (!ResolveDevice(device, found))
            return false;
        if (!found.enabled)
            return false;
        desc = found;

        ChefZ_RecipeEngine engine = ChefZ_RecipeEngine.Get();
        if (!engine.HasAnyRecipeFor(found.deviceClass, found.deviceRootClass))
            return false;

        // Direktkochen ohne Gefaess (FireplaceBase.CookOnDirectSlot) landet
        // ebenfalls in CookWithEquipment, hat aber kein Cargo. ChefZ-Rezepte
        // sind gefaessbasiert (10 §3).
        int count = CargoCount(device);
        if (count <= 0)
            return false;

        if (count < engine.GetMinItemCountFor(found.deviceClass))
            return false;

        cargoCount = count;
        return true;
    }

    //==========================================================================

    private void ObserveInner(ItemBase device, float timeCoef, float updateTime, int method, int now)
    {
        ChefZ_DeviceDescriptor desc;
        int cargoCount;
        if (!PassesGate(device, desc, cargoCount))
        {
            // Ein BEKANNTES Kochgeraet, das die Stufe 0 nicht mehr besteht,
            // ist ein Gefaess, in dem nicht einmal mehr ein Rezept anfangen
            // koennte - leer oder unter der Mindestzutatenzahl. Ab hier sieht
            // der Adapter nicht mehr hin, und eine Zuschreibung, die er nicht
            // mehr ueberwacht, ist eine offene Flanke: sie ueberdauerte das
            // Leeren des Topfes und hinge dem naechsten Koch an. Deshalb
            // faellt sie hier - und nicht erst mit der Alterung.
            //
            // desc ist nur gesetzt, wenn die Geraeteaufloesung gelungen und
            // das Geraet in CfgChefZDevices eingetragen ist (10 E7). Fuer
            // alles andere geschieht hier so wenig wie bisher.
            if (desc && desc.enabled)
                ForgetClaim(device);
            return;
        }

        m_CountObserved++;

        ChefZ_CookSession session = GetSession(device);
        if (!session)
            return;
        session.Touch(now);

        //--- Stufe A - Signatur (10 §5) ---------------------------------------
        // MeasureSignature und nicht BuildSignature: m_Scratch ist ein FELD,
        // und ein Feld als out-Parameter zu uebergeben ist in Enforce nicht
        // zugesichert (siehe Kopf von ChefZ_TextList.SymbolsOf).
        if (!MeasureSignature(device, method, m_Scratch))
            return;

        bool unchanged = session.signature.IsMeasured() && session.signature.Equals(m_Scratch);
        if (!unchanged)
        {
            session.AdoptNewContent(m_Scratch);
        }
        else if (session.IsInert())
        {
            // Der mit Abstand haeufigste Ausgang: derselbe Inhalt, hier ist
            // nichts mehr zu tun. Ein Vergleich, dann zurueck.
            m_CountSignatureHits++;
            return;
        }

        ChefZ_CoreSettingsDef settings = ChefZ_ConfigManager.Get().GetSettings();

        //--- Zuschreibung (siehe Kopf von ChefZ_CookActor) --------------------
        // Sie steht HINTER der Ruecksprung-Wache oben und damit nicht im
        // heissesten Pfad: ist die Signatur unveraendert und die Sitzung
        // inert, ist auch der Bestand unveraendert und es gaebe nichts
        // aufzuloesen.
        UpdateActorClaim(device, session, settings);

        //--- Stufe B - Vollmatch (10 §5) --------------------------------------
        bool freshlyBuilt = false;

        if (session.state == ChefZ_ESessionState.IDLE)
        {
            if (session.ticksSinceMatch < settings.matchThrottleTicks)
            {
                // 10 E4: Schutz gegen einen Spieler, der Items im
                // Sekundentakt ein- und auslagert.
                m_CountSignatureHits++;
                return;
            }

            if (!RunFullMatch(device, session, desc, updateTime, method))
                return;

            freshlyBuilt = true;
        }

        //--- Stufe C - Abschluss (10 §5, 10 §6) -------------------------------
        if (session.state == ChefZ_ESessionState.MATCHED)
            CheckCompletion(device, session, desc, timeCoef, updateTime, method, freshlyBuilt);
    }

    //==========================================================================
    // Zuschreibung (siehe Kopf von ChefZ_CookActor)
    //==========================================================================

    /**
     * Die Zuschreibung eines Gefaesses fallen lassen, ohne eine Sitzung
     * anzulegen.
     *
     * PeekSession und nicht GetSession: der Aufruf steht im gedrosselten Pfad
     * und darf fuer ein Gefaess, das ChefZ noch nie gesehen hat, nicht
     * ploetzlich Speicher belegen. Ohne Sitzung gibt es nichts zu vergessen.
     */
    private void ForgetClaim(notnull ItemBase device)
    {
        ChefZ_CookSession session = PeekSession(device);
        if (!session)
            return;
        session.ForgetClaim();
    }

    /**
     * Den Anspruchsinhaber dieses Gefaesses fortschreiben.
     *
     * Der ganze Mechanismus haengt an einem einzigen Befund: Vanillas
     * Kochschleife legt nie etwas in ein Gefaess. Waechst der Bestand, war es
     * ein Spieler - und nur dann wird gefragt, welcher.
     *
     * Drei Waechter davor, in dieser Reihenfolge, weil sie immer billiger
     * werden, je haeufiger sie greifen:
     *
     *   1. kein Zuwachs                -> ein Vergleich, der Normalfall
     *   2. Zuschreibung abgeschaltet   -> cookActorRadius <= 0
     *   3. niemand hoert zu            -> vier Map-Zugriffe (17 E2)
     *
     * Erst danach laeuft die Umkreissuche, und die laeuft damit nur in dem
     * Tick, in dem tatsaechlich jemand etwas eingelegt hat.
     *
     * Sie kann nichts kaputt machen: der Rueckgabewert wird ausschliesslich
     * in die Sitzung geschrieben und von dort in ChefZ_CookContext. Bleibt er
     * 0, verhaelt sich der Core exakt so wie vor dieser Aenderung.
     */
    private void UpdateActorClaim(notnull ItemBase device, notnull ChefZ_CookSession session, ChefZ_CoreSettingsDef settings)
    {
        if (!session.ObserveItemCount(m_Scratch.itemCount))
            return;

        if (!settings)
            return;
        if (settings.cookActorRadius <= 0.0)
            return;

        if (!ChefZ_CookActor.AnyoneCares())
            return;

        int before = session.actorIdentityId;
        int after  = ChefZ_CookActor.Resolve(device, before, settings.cookActorRadius);
        if (after == before)
            return;

        session.actorIdentityId = after;

        // Der Wechsel gehoert ins Log, weil er die einzige Stelle des
        // Kochpfads ist, an der eine SPIELERBEZOGENE Entscheidung faellt.
        // Wer sich fragt, warum ein Gericht bei ihm keine Wirkung hatte, muss
        // die Antwort hier finden koennen.
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.DEBUG))
            return;

        ChefZ_Log.Debug(ChefZ_LogChannel.COOK, "Zuschreibung in Gefaess " + session.vesselId.ToString() + ": " + before.ToString() + " -> " + after.ToString() + "  (Bestand auf " + m_Scratch.itemCount.ToString() + " gewachsen)");
    }

    //==========================================================================
    // Stufe B (10 §5)
    //==========================================================================

    /**
     * Der teure Teil: Fakten erheben, alle Kandidaten pruefen.
     *
     * @return true, wenn die Auswertung gelaufen ist - NICHT, ob sie etwas
     *         gefunden hat. "Kein Treffer" ist der haeufigste Ausgang und
     *         ausdruecklich kein Fehler (08 §4): Vanilla hat in Schritt 1
     *         bereits gearbeitet, die Zutaten garen normal weiter.
     */
    private bool RunFullMatch(notnull ItemBase device, notnull ChefZ_CookSession session, notnull ChefZ_DeviceDescriptor desc, float updateTime, int method)
    {
        if (!BuildContextFrom(device, desc, session.actorIdentityId, updateTime, method))
        {
            FailSession(session, "Kontext konnte nicht erhoben werden");
            return false;
        }

        m_CountFullMatches++;
        session.ticksSinceMatch = 0;

        ChefZ_MatchTrace trace = ChefZ_MatchTrace.CreateIfEnabled();
        if (trace)
            trace.Note("Gefaess " + session.vesselId.ToString() + " - " + desc.ToDebugString());

        ChefZ_MatchResult result;
        bool matched = ChefZ_RecipeEngine.Get().EvaluateBest(m_Ctx, m_Snapshot, trace, result);

        if (trace)
            trace.Emit();

        if (!matched)
        {
            // 10 §5: "state = DONE, Trace ablegen, return - Vanilla hat
            // bereits gearbeitet, FERTIG."
            session.state   = ChefZ_ESessionState.DONE;
            session.outcome = null;

            if (ChefZ_Log.Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.DEBUG))
            {
                ChefZ_Log.Debug(ChefZ_LogChannel.MATCH, "Kein Treffer -> Vanilla-Kochen laeuft unveraendert weiter  (" + desc.ToDebugString() + ", " + result.ToDebugString() + ")");
            }
            return true;
        }

        // S13 (17 §4, §7): ChefZ_OnRecipeMatched - das stornierbare Ereignis
        // VOR jeder Wirkung.
        //
        // Es steht hier und nicht nach der Zustandssetzung, weil eine
        // Stornierung genau das bedeuten soll, was 17 §7 schreibt: "Abbruch,
        // VANILLA laeuft weiter". Zu diesem Zeitpunkt wurde nichts verbraucht,
        // nichts erzeugt und nichts veraendert - eine Stornierung kostet
        // deshalb nichts und hinterlaesst nichts.
        //
        // Das ist zugleich der saubere Hebel fuer HARTE Recipe Locks aus einem
        // externen Mod (17 E5, OF-08): der Comp-Mod storniert, ChefZ faellt auf
        // Vanilla zurueck, und im Core steht dazu kein Wort ueber Skills.
        if (IsMatchCancelled(device, result))
        {
            // Exakt der Ausgang aus dem Zweig "kein Treffer" eine Seite
            // darueber: DONE heisst "hier gibt es fuer ChefZ nichts zu tun".
            // Vanilla hat zu diesem Zeitpunkt bereits gearbeitet und laeuft
            // unveraendert weiter (Invariante I2).
            session.state   = ChefZ_ESessionState.DONE;
            session.outcome = null;
            return true;
        }

        session.state              = ChefZ_ESessionState.MATCHED;
        session.outcome            = result;
        session.elapsedSec         = 0.0;
        session.contentFingerprint = m_Scratch.ToFingerprint();
        m_CountMatched++;

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.MATCH, ChefZ_LogLevel.INFO))
        {
            ChefZ_Log.Info(ChefZ_LogChannel.MATCH, "Gebunden: " + result.recipeId + " in Gefaess " + session.vesselId.ToString() + "  " + result.ToDebugString());
        }
        return true;
    }

    /**
     * ChefZ_OnRecipeMatched ausloesen und die Antwort auswerten (17 §4, E5).
     *
     * Erst HasSubscribers(), dann erst die Nutzlast (17 E2): auf einem Server
     * ohne Comp-Module kostet diese Methode einen Map-Zugriff je Vollmatch -
     * und Vollmatches sind ohnehin gedrosselt (10 §6).
     *
     * @return true = das Rezept wird NICHT angewandt. Der Aufrufer faellt auf
     *         Vanilla zurueck und veraendert nichts.
     */
    private bool IsMatchCancelled(notnull ItemBase device, notnull ChefZ_MatchResult result)
    {
        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (!bus.HasSubscribers(ChefZ_EventNames.RECIPE_MATCHED))
            return false;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.RECIPE_MATCHED);
        FillCookArgs(args, device, result);

        string reason;
        string who;
        if (!bus.RaiseCancellable(args, reason, who))
            return false;

        // An der Stufenpruefung vorbei waere zu viel - aber INFO ist die
        // richtige Hoehe: ein Betreiber, der sich wundert, warum ein Rezept
        // nicht zuendet, muss den Abbrecher im Log finden koennen, ohne den
        // Debugkanal einzuschalten (17 §7).
        ChefZ_Log.Info(ChefZ_LogChannel.MATCH, "Rezept " + result.recipeId + " wurde von " + who + " storniert" + ReasonSuffix(reason) + " - Vanilla-Kochen laeuft unveraendert weiter.");

        return true;
    }

    private static string ReasonSuffix(string reason)
    {
        if (reason == "")
            return " (ohne Angabe eines Grundes)";
        return " (" + reason + ")";
    }

    /**
     * Die gemeinsamen Felder eines Kochereignisses.
     *
     * EINE Stelle fuer Match, Abbruch und Abschluss, damit ein Abonnent bei
     * allen dreien dieselben Felder vorfindet. Die Klassenlisten werden hier
     * gefuellt und nicht im Applicator, weil die Faktenliste dem Adapter
     * gehoert - dieselbe Ueberlegung wie bei ResolveQuality (12 §6).
     *
     * NUR Symbole und Netz-IDs (17 E4). Kein ItemBase verlaesst diese Methode.
     */
    private void FillCookArgs(notnull ChefZ_EventArgs args, ItemBase device, notnull ChefZ_MatchResult result)
    {
        args.identityId        = m_Ctx.actorIdentityId;
        args.recipeOrTransform = result.recipeSym;
        args.deviceClass       = m_Ctx.deviceClass;
        args.qualityTier       = result.qualityTier;
        args.qualityScore      = result.gradeScore;
        args.amount            = result.boundItemCount;

        if (device)
        {
            int low, high;
            if (VesselId(device, low, high))
                args.SetDeviceNetId(low, high);
        }

        ChefZ_CompiledRecipe recipe = result.recipe;
        if (recipe)
        {
            for (int e = 0; e < recipe.effects.Count(); e++)
                args.AddEffect(recipe.effects.Get(e));
        }

        // Die verbrauchten Klassen aus dem Verbrauchsplan. Sie stehen im
        // Ereignis, weil ein Statistik- oder Questmodul sonst raten muesste,
        // was in den Topf ging - und weil sie nach dem Verbrauch nirgends mehr
        // ablesbar sind.
        for (int i = 0; i < result.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = result.consumePlan.Get(i);
            if (!plan)
                continue;
            ChefZ_ItemFacts facts = m_Snapshot.FindByHandle(plan.handle);
            if (facts)
                args.AddConsumed(facts.classSym);
        }
    }

    //==========================================================================
    // Stufe C (10 §5, 10 §6)
    //==========================================================================

    /**
     * Ist das gebundene Rezept fertig?
     *
     * Der ON_STAGE-Zweig braucht die aktuellen Fakten, und zwar aus einem
     * Grund, den 10 E4 selbst nennt: die Signatur ist eine Maske und keine
     * Liste. Drei Items in RAW, RAW, BOILED ergeben dieselbe FoodStage-Maske
     * wie BOILED, RAW, BOILED - der Abschluss hat sich geaendert, die Signatur
     * nicht. Wer sich hier auf die Signatur verliesse, verpasste den
     * Abschluss, bis der Spieler zufaellig etwas anfasst.
     *
     * Deshalb wird das EINE gebundene Rezept neu gegen die frischen Fakten
     * gebunden - nicht der ganze Kandidatensatz. Das ist der Unterschied
     * zwischen Stufe C und Stufe B und der Grund, warum Stufe C "billig" heisst.
     */
    private void CheckCompletion(notnull ItemBase device, notnull ChefZ_CookSession session, notnull ChefZ_DeviceDescriptor desc, float timeCoef, float updateTime, int method, bool freshlyBuilt)
    {
        ChefZ_MatchResult bound = session.outcome;
        if (!bound || !bound.recipe)
        {
            FailSession(session, "gebundenes Rezept ist verschwunden");
            return;
        }

        ChefZ_CompiledRecipe recipe = bound.recipe;

        // ---------------------------------------------------------------
        // Seit S8: OHNE frische Puffer wird hier nichts abgeschlossen.
        // ---------------------------------------------------------------
        // m_Ctx, m_Snapshot und m_Entities gehoeren dem ADAPTER, nicht dem
        // Gefaess (10 §7). Zwischen zwei Kochticks desselben Topfes koennen
        // beliebig viele andere Feuerstellen dieselben Puffer ueberschrieben
        // haben. Solange nur geloggt wurde, war das folgenlos; seit der
        // Applicator daran haengt, waere es der schlimmste denkbare Fehler -
        // die Handles zeigten in die Entity-Liste eines FREMDEN Gefaesses.
        //
        // Deshalb gilt ab hier ausnahmslos: wer abschliessen will, hat in
        // diesem Tick fuer dieses Gefaess Fakten erhoben und das gebundene
        // Rezept dagegen neu gebunden. Genau das tut der !freshlyBuilt-Zweig.
        //
        // Der Applicator revalidiert zusaetzlich jeden Handle gegen das
        // Gefaess (08 §6, Schritt 1) und fiele auch dann sicher aus, wenn
        // diese Zusage einmal brechen sollte. Zwei unabhaengige Sicherungen,
        // weil eine davon Zutaten kostet, wenn sie versagt.
        if (!freshlyBuilt)
        {
            if (!BuildContextFrom(device, desc, session.actorIdentityId, updateTime, method))
            {
                FailSession(session, "Kontext konnte nicht erhoben werden");
                return;
            }

            AdvanceTimedClock(session, recipe, timeCoef, updateTime);

            // Das EINE gebundene Rezept neu gegen die frischen Fakten binden -
            // nicht der ganze Kandidatensatz. Das ist der Unterschied
            // zwischen Stufe C und Stufe B und der Grund, warum Stufe C
            // "billig" heisst.
            //
            // Ziel ist das Ergebnisobjekt der Sitzung SELBST. Evaluate()
            // setzt es als erste Anweisung zurueck und fuellt es neu; eine
            // frische Instanz je Tick waere eine Allokation je Tick je
            // gebundenem Gefaess, und 08 §7 fuehrt das Ergebnis ausdruecklich
            // als Objekt aus einem Pool.
            //
            // Die lokale Variable recipe zeigt weiter auf das kompilierte
            // Rezept, auch nachdem Reset() das Feld im Ergebnis geleert hat:
            // Eigentuemer ist die Engine, nicht das Ergebnis.
            ChefZ_MatchResult fresh = bound;
            if (!ChefZ_RecipeEvaluator.Evaluate(recipe, m_Ctx, m_Snapshot, ChefZ_RecipeEngine.Get().GetNodeBudget(), null, fresh))
            {
                // Der Inhalt passt nicht mehr, ohne dass die Signatur es
                // gezeigt haette. Zurueck auf IDLE, damit Stufe B - gedrosselt
                // - neu entscheidet. NICHT auf DONE: das waere die Aussage
                // "hier gibt es nichts mehr", und die ist hier nicht belegt.
                session.state   = ChefZ_ESessionState.IDLE;
                session.outcome = null;

                if (ChefZ_Log.Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.DEBUG))
                {
                    ChefZ_Log.Debug(ChefZ_LogChannel.COOK, "Bindung von " + recipe.id + " in Gefaess " + session.vesselId.ToString() + " ist entfallen: " + fresh.failReason);
                }
                return;
            }
        }
        else
        {
            // Frisch gebunden: die Fakten dieses Ticks stecken bereits in den
            // Puffern, und EvaluateBest hat die Abschlussfrage schon
            // beantwortet (ChefZ_RecipeEngine.EvaluateBest ruft CheckReady
            // selbst). Sie ein zweites Mal auf denselben Fakten zu stellen
            // ergaebe dieselbe Antwort und kostete einen zweiten Matcherlauf.
            //
            // TIMED ist die Ausnahme: dort hat EvaluateBest mit elapsedSec = 0
            // gerechnet, weil die Uhr der Sitzung gehoert und erst jetzt
            // gestellt wird.
            if (recipe.completion != ChefZ_Completion.TIMED)
            {
                if (!bound.ready)
                {
                    LogStillOpen(session, recipe, bound.notReadyReason);
                    return;
                }
                Complete(device, session, recipe, ChefZ_Completion.Name(recipe.completion));
                return;
            }

            AdvanceTimedClock(session, recipe, timeCoef, updateTime);
        }

        // session.outcome zeigt auf dieselbe Instanz und traegt die frischen
        // Handles - genau die braucht der Applicator zum Revalidieren.
        string reason;
        if (!ChefZ_RecipeEvaluator.CheckReady(recipe, bound, m_Snapshot, m_Ctx, reason))
        {
            // 10 §5: "nicht erfuellt -> return. Vanilla kocht weiter, Zutaten
            // garen normal."
            LogStillOpen(session, recipe, reason);
            return;
        }

        Complete(device, session, recipe, ChefZ_Completion.Name(recipe.completion));
    }

    /**
     * Die eigene Uhr der Sitzung, gegen Zeitdiebstahl gesichert (10 §6, E5).
     *
     * Nur completion TIMED hat eine. Bei ON_STAGE ist Vanillas FoodStage der
     * Fortschritt, und die eigene Zeitmessung waere ein zweiter, langsam
     * auseinanderlaufender Zaehler fuer dieselbe Sache.
     *
     * Sie setzt voraus, dass m_Ctx und m_Scratch fuer dieses Gefaess in
     * diesem Tick gefuellt wurden - der Aufrufer stellt das sicher.
     */
    private void AdvanceTimedClock(notnull ChefZ_CookSession session, notnull ChefZ_CompiledRecipe recipe, float timeCoef, float updateTime)
    {
        if (recipe.completion != ChefZ_Completion.TIMED)
            return;

        int fingerprint = m_Scratch.ToFingerprint();
        if (fingerprint != session.contentFingerprint)
        {
            // 10 E5: sonst koennte man kurz vor Ende die Zutaten tauschen und
            // die aufgelaufene Zeit erben.
            session.elapsedSec         = 0.0;
            session.contentFingerprint = fingerprint;
        }
        else if (m_Ctx.deviceTemperature >= recipe.minTemperature)
        {
            float step = updateTime * timeCoef;
            if (step > 0.0)
                session.elapsedSec = session.elapsedSec + step;
        }

        m_Ctx.elapsedSec = session.elapsedSec;
    }

    /**
     * "Gebunden, aber noch nicht fertig" - der Normalfall ueber viele Ticks.
     *
     * TRACE und nicht DEBUG: die Zeile faellt bei ON_STAGE fuer jedes gebundene
     * Gefaess in JEDEM Tick an, bis das Gericht fertig ist. Auf DEBUG waere
     * damit der Kanal COOK unbenutzbar, und das ist genau der Kanal, auf dem
     * man beim Kochen zusehen will.
     */
    private void LogStillOpen(notnull ChefZ_CookSession session, notnull ChefZ_CompiledRecipe recipe, string reason)
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.TRACE))
            return;

        ChefZ_Log.Trace(ChefZ_LogChannel.COOK, "Abschluss " + ChefZ_Completion.Name(recipe.completion) + " offen: " + recipe.id + " in Gefaess " + session.vesselId.ToString() + " - " + reason);
    }

    /**
     * Der Abschluss: die Transaktion ausloesen (08 §6, 10 §5 Stufe C).
     *
     * COMPLETING steht ueber genau die Dauer des Applicator-Aufrufs. Der
     * Zustand ist kein Schmuck: er ist die Antwort auf einen Wiedereintritt
     * waehrend der Transaktion - eine Sitzung in COMPLETING hat keinen Weg
     * zurueck nach Stufe B, solange sie laeuft.
     *
     * Danach gibt es genau zwei Ausgaenge, und keiner von beiden ist "halb":
     *
     *   Erfolg     -> DONE. Der Inhalt des Gefaesses hat sich veraendert, die
     *                 Signatur passt nicht mehr, und der naechste Tick beginnt
     *                 mit einer frischen Bewertung. Ein zweites Gericht aus
     *                 dem Rest ist dabei ausdruecklich erlaubt.
     *   Fehlschlag -> failCount++. Die Welt ist unveraendert, KEINE Zutat
     *                 wurde verbraucht (Invariante I5). Die Sitzung bleibt in
     *                 MATCHED und versucht es im naechsten Tick erneut - beim
     *                 dritten Mal sperrt FailSession auf SUPPRESSED (10 E6).
     *
     * Der Rueckfall nach MATCHED statt nach DONE ist Absicht: 08 §8 verlangt
     * fuer die haeufigste Fehlerursache ("Handle verweist auf inzwischen
     * geloeschtes Entity") ausdruecklich "Naechster Tick versucht es erneut".
     * DONE hiesse dagegen "hier gibt es nichts mehr zu tun", und das ist nach
     * einem Fehlschlag nicht belegt.
     */
    /**
     * Die Qualitaetsstufe des gebundenen Ergebnisses bestimmen (12 §6).
     *
     * Seit S13 mit dem einen Abfrage-Event: ChefZ_QualityBonusQuery wird VOR
     * der Punktrechnung ausgeloest, Abonnenten addieren auf bonusPoints, und
     * der Bus klemmt die Summe auf maxExternalQualityBonus (17 §5). Es ist
     * eine Query mit Punktebeitrag, KEIN "setze Qualitaet auf X" - die Hoheit
     * ueber Stufen und Schwellen bleibt beim Core. Ohne Abonnenten ist der
     * Bonus 0.0, und die Rechnung ist dieselbe wie bis S12.
     *
     * Danach die zweite Wirkung von requires[]: nicht erfuellte Anforderungen
     * mit onFail "degrade" verschieben die STUFE, sie verweigern das Gericht
     * nicht (12 E8). Der Spieler ohne Faehigkeit bekommt sein Essen, nur
     * schlechter - und das funktioniert auch ohne registrierten Anbieter, was
     * den Server ohne Skillmod voll spielbar haelt.
     *
     * Bei aktivem Kanal QUALITY wandert die Rechnung Term fuer Term ins Log -
     * das ist die Gegenleistung fuer die additive Punktrechnung (12 E1).
     * Ohne den Kanal entsteht keine einzige formatierte Zeichenkette.
     */
    private void ResolveQuality(notnull ItemBase device, notnull ChefZ_MatchResult bound)
    {
        ChefZ_CompiledRecipe recipe = bound.recipe;
        if (!recipe)
            return;

        ChefZ_QualityManager quality = ChefZ_QualityManager.Get();
        if (!quality.IsReady())
            return;

        float externalBonus = QueryExternalBonus(device, bound, quality);

        // Ueber eine lokale Zwischenvariable: m_Eval ist ein FELD, und ein
        // Feld als out-Parameter ist in Enforce nicht zugesichert (dieselbe
        // Vorsicht wie bei Peek()).
        ChefZ_QualityEvaluation eval = m_Eval;
        bound.qualityTier = quality.EvaluateResult(recipe, bound, m_Snapshot, m_Ctx, externalBonus, eval);
        m_Eval = eval;

        ApplyCapabilityDegrade(bound, recipe, quality);

        // Die Vorschaetzung des Evaluators (Slotpunkte + Bias) wird durch die
        // vollstaendige Rechnung ersetzt. Das ist unbedenklich, weil
        // gradeScore ausschliesslich in ToDebugString() gelesen wird und die
        // Rangfolge ueber "score" laeuft - die steht zu diesem Zeitpunkt
        // ohnehin fest. Gewonnen ist, dass im Trace die Zahl steht, die
        // tatsaechlich ueber die Stufe entschieden hat.
        bound.gradeScore = eval.TotalScore;

        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.QUALITY, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        lines.Insert("Qualitaet fuer " + recipe.id);
        eval.ToLines(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.QUALITY, lines);
    }

    /**
     * Das EINE Abfrage-Event (17 §5, E6).
     *
     * Ohne Abonnenten wird keine Nutzlast gebaut und der Bonus ist 0.0 - die
     * Rechnung ist dann bitgenau die aus S10. Die Klemme auf
     * maxExternalQualityBonus macht der Bus; die zweite, unabhaengige Klemme
     * im Quality Manager bleibt bestehen und ist Absicht: sie deckt auch den
     * Fall ab, dass irgendwann jemand anders einen externen Bonus einreicht.
     */
    private float QueryExternalBonus(notnull ItemBase device, notnull ChefZ_MatchResult bound, notnull ChefZ_QualityManager quality)
    {
        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (!bus.HasSubscribers(ChefZ_EventNames.QUALITY_BONUS_QUERY))
            return 0.0;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.QUALITY_BONUS_QUERY);
        FillCookArgs(args, device, bound);

        return bus.RaiseQuery(args, quality.GetMaxExternalBonus());
    }

    /**
     * onFail "degrade" anwenden (12 E8, 17 §3.3).
     *
     * NACH der Punktrechnung und NICHT als Punktabzug: 12 E8 verschiebt
     * ausdruecklich die STUFE. Der Unterschied ist keiner der Zahlen, sondern
     * der Erklaerbarkeit - "eine Stufe schlechter, weil dir die Uebung fehlt"
     * ist eine Aussage, die ein Spieler versteht; "minus 1.7 Punkte" ist es
     * nicht.
     *
     * Ohne registrierten Anbieter gilt der Config-Default. Steht der auf 0
     * (Vorgabe), degradiert ein Rezept mit Faehigkeitsanforderung fuer jeden -
     * und genau das ist gemeint: ohne Skillmodul kann niemand die Uebung
     * haben. Wer das nicht will, setzt capabilityMode auf "ignore".
     */
    private void ApplyCapabilityDegrade(notnull ChefZ_MatchResult bound, notnull ChefZ_CompiledRecipe recipe, notnull ChefZ_QualityManager quality)
    {
        if (recipe.requires.Count() == 0)
            return;
        if (!ChefZ_SymbolTable.IsValid(bound.qualityTier))
            return;

        string why;
        int steps = ChefZ_CapabilityRegistry.Get().DegradeStepsFor(recipe.requires, m_Ctx.actorIdentityId, why);
        if (steps <= 0)
            return;

        ChefZ_Sym before = bound.qualityTier;
        bound.qualityTier = quality.DegradeTier(before, steps);

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.QUALITY, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.QUALITY, "Faehigkeitsabwertung " + steps.ToString() + " Stufe(n): " + ChefZ_SymbolTable.NameOrMark(before) + " -> " + ChefZ_SymbolTable.NameOrMark(bound.qualityTier) + "  (" + why + ")");
        }
    }

    /**
     * Der Naehrwert-Sollwert dieses Kochvorgangs, fuer das Trace (13 §6).
     *
     * DAS EINZIGE, was diese Methode tut, ist rechnen und loggen. Sie
     * veraendert weder das Ergebnis noch die Zutaten noch das Gefaess - der
     * Naehrwert eines Gerichts steht in seiner CfgVehicles-Definition und
     * nirgends sonst (13 E1). Wer hier je eine Zuweisung an ein Item
     * ergaenzt, hebt Befund 01 V6 nicht auf; er baut nur eine Zahl, die den
     * Verzehrpfad nie erreicht.
     *
     * Sie laeuft NUR, wenn der Kanal NUTRI auf TRACE steht. Auf einem
     * Produktivserver kostet sie damit genau einen Bool-Test je fertigem
     * Gericht.
     */
    private void TraceNutrition(notnull ChefZ_MatchResult bound, notnull ChefZ_CompiledRecipe recipe)
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.NUTRI, ChefZ_LogLevel.TRACE))
            return;

        ChefZ_NutritionManager nutrition = ChefZ_NutritionManager.Get();
        if (!nutrition.IsReady())
            return;

        ChefZ_NutritionVector expected;
        array<string> lines = new array<string>();
        nutrition.ComputeExpected(recipe, bound, m_Snapshot, expected, lines);

        string summary;
        nutrition.DescribeForUI(expected, summary);

        lines.InsertAt("Naehrwert-Soll fuer " + recipe.id + ": " + summary + "  (Diagnose - NICHT angewandt, 13 E1)", 0);
        ChefZ_Log.Block(ChefZ_LogLevel.TRACE, ChefZ_LogChannel.NUTRI, lines);
    }

    private void Complete(notnull ItemBase device, notnull ChefZ_CookSession session, notnull ChefZ_CompiledRecipe recipe, string modeName)
    {
        session.state = ChefZ_ESessionState.COMPLETING;

        ChefZ_MatchResult bound = session.outcome;
        if (!bound)
        {
            // Kann nicht vorkommen - CheckCompletion prueft es als erstes.
            // Die zweite Sicherung steht hier, weil Apply() das Ergebnis als
            // notnull entgegennimmt und ein Null-Ergebnis dort ein Absturz
            // waere statt eines Abbruchs.
            session.state = ChefZ_ESessionState.IDLE;
            FailSession(session, "das gebundene Ergebnis ist zwischen Pruefung und " + "Anwendung verschwunden");
            return;
        }

        // S10 (12 §6, "BEIM KOCHEN"): die Qualitaetsstufe wird HIER bestimmt
        // und nicht im Applicator.
        //
        // Grund ist die Datenlage, nicht der Geschmack: die Punktrechnung
        // braucht die Faktenliste der Zutaten (12 §4), und die liegt im
        // Adapter. Der Applicator bekommt sie nie zu sehen - er arbeitet mit
        // Handles und Entities. Berechnet wird also dort, wo die Eingaben
        // sind; geschrieben wird die Stufe im Applicator, wo das Ergebnis
        // entsteht.
        //
        // Zeitpunkt: VOR Apply(), damit die Stufe schon bei der Wahl der
        // Ergebnisklasse zur Verfuegung steht (OutputDef.variants, 12 §2).
        ResolveQuality(device, bound);

        // S12 (13 §6, "KOCHEN"): die Sollrechnung, und zwar AUSSCHLIESSLICH
        // ins Trace. Kein Schreibzugriff auf das Ergebnis, kein Einfluss auf
        // die Ergebnisklasse, keine Bedingung.
        //
        // Sie steht hier und nicht im Applicator, weil sie die Faktenliste
        // braucht und die dem Adapter gehoert - dieselbe Ueberlegung wie bei
        // ResolveQuality eine Zeile darueber.
        TraceNutrition(bound, recipe);

        // Die Puffer des Adapters, wie sie CheckCompletion in DIESEM Tick fuer
        // DIESES Gefaess gefuellt hat. m_Entities ist die parallele Liste zu
        // m_Snapshot; die Handles im Ergebnis zeigen genau dorthin (05 §3.4).
        array<ItemBase> created;
        string err;

        bool ok = ChefZ_Applicator.Apply(bound, m_Entities, device, m_Ctx, created, err);

        if (!ok)
        {
            session.state = ChefZ_ESessionState.MATCHED;
            FailSession(session, "Anwendung von " + recipe.id + " abgebrochen: " + err + " (nichts verbraucht, nichts erzeugt)");

            // S13 (17 §4): ChefZ_OnRecipeFailed. Reine Benachrichtigung, nicht
            // stornierbar und nicht XP-tauglich - die Transaktion ist bereits
            // abgebrochen, und die Zutaten sind garantiert unveraendert
            // (Invariante I5). Ein Abonnent, der hier etwas "retten" wollte,
            // haette nichts zu retten.
            RaiseRecipeFailed(device, bound, err);
            return;
        }

        m_CountCompleted++;
        session.state = ChefZ_ESessionState.DONE;

        // Der Applicator hat das Gericht IN DAS GEFAESS gelegt
        // (ChefZ_Applicator.SpawnOutput). Der Bestand kann dadurch wachsen,
        // ohne dass ein Spieler etwas getan hat - genau der Fall, den die
        // Zuschreibung sonst als Einlegen lesen wuerde. Die Sperre gilt fuer
        // den naechsten Tick dieses Gefaesses und fuer keinen weiteren.
        //
        // Der Anspruch SELBST bleibt bestehen: die Reste im Topf hat
        // derselbe Spieler hineingelegt, und ein zweites Gericht daraus ist
        // ein zweites Gericht aus seinen Zutaten - keine Wiederholung
        // desselben Vorgangs. Doppelte Meldung ist dadurch ausgeschlossen,
        // dass ein Abschluss die gebundenen Zutaten VERBRAUCHT: derselbe
        // Kochvorgang kann kein zweites Mal fertig werden.
        session.claimSelfInflicted = true;

        // S13 (17 §4, §7, E7): ChefZ_OnRecipeCompleted und danach
        // ProgressRegistry.Report("cook").
        //
        // HIER und nicht im Applicator, obwohl 08 §6 Schritt 7 ihn dort
        // verortet: die Nutzlast braucht die Faktenliste der Zutaten, und die
        // gehoert dem Adapter - der Applicator sieht nur Handles und Entities.
        // Dieselbe Ueberlegung wie bei ResolveQuality und TraceNutrition.
        //
        // Der Zeitpunkt ist derselbe, den 08 §6 meint, und darauf kommt es an:
        // Apply() ist zurueck und hat true geliefert. Das Ergebnis existiert,
        // die Zutaten sind verbraucht, die Transaktion ist abgeschlossen. Erst
        // JETZT darf gemeldet werden (Regel §10.6) - und deshalb steht der
        // Aufruf hinter der Fehlerabfrage und nicht davor.
        RaiseRecipeCompleted(device, bound, created);

        // An der Stufenpruefung vorbei waere hier falsch - der Fall tritt je
        // Gericht einmal auf, aber auf einem vollen Server oft genug. INFO ist
        // die richtige Hoehe: ein fertiges Gericht ist das Ereignis, wegen
        // dessen es diesen Mod gibt.
        ChefZ_Log.Info(ChefZ_LogChannel.COOK, "Abschluss erfuellt (" + modeName + "): " + recipe.id + " in Gefaess " + session.vesselId.ToString() + " - " + created.Count().ToString() + " Ergebnis(se) erzeugt.");
    }

    /**
     * ChefZ_OnRecipeCompleted plus Fortschrittsmeldung (17 §4, §7, E7).
     *
     * Die EINZIGE Nutzlast wird einmal gebaut und an beide gegeben - Bus und
     * Fortschrittsregister. Deshalb steht die Wache ueber BEIDE:
     * ohne Abonnenten UND ohne Empfaenger entsteht keine Zeichenkette und
     * keine Liste.
     *
     * Reihenfolge: erst das Ereignis, dann die Fortschrittsmeldung. 17 §7
     * zeichnet sie so, und sie ist die richtige - ein Abonnent, der das
     * Ereignis auswertet, soll das tun koennen, bevor irgendwo XP vergeben
     * wird.
     *
     * emitEvents des Rezepts laufen danach als EIGENE Ereignisse durch
     * denselben Bus (08 §2: der Core wertet sie NIE aus, er reicht sie
     * weiter). Damit kann ein Content-Modul "ChefZFremd_OnEtwasPassiert"
     * ausloesen, ohne dass im Core eine Zeile dafuer steht (17 E1).
     */
    private void RaiseRecipeCompleted(notnull ItemBase device, notnull ChefZ_MatchResult bound, array<ItemBase> created)
    {
        ChefZ_CompiledRecipe recipe = bound.recipe;

        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        bool wantEvent    = bus.HasSubscribers(ChefZ_EventNames.RECIPE_COMPLETED);
        bool wantProgress = ChefZ_ProgressRegistry.HasSinks();
        bool wantCustom   = false;

        if (recipe)
        {
            for (int c = 0; c < recipe.emitEvents.Count(); c++)
            {
                if (bus.HasSubscribers(recipe.emitEvents.Get(c)))
                {
                    wantCustom = true;
                    break;
                }
            }
        }

        if (!wantEvent && !wantProgress && !wantCustom)
            return;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.RECIPE_COMPLETED);
        FillCookArgs(args, device, bound);

        if (created)
        {
            for (int i = 0; i < created.Count(); i++)
            {
                ItemBase item = created.Get(i);
                if (!item)
                    continue;
                args.AddProduced(ChefZ_SymbolTable.Intern(item.GetType()));

                // Das erste Ergebnis ist das Subjekt des Ereignisses. Ein
                // Beiprodukt ist kein Gericht, und ein Abonnent, der "was ist
                // entstanden" fragt, meint das Hauptergebnis.
                if (i == 0)
                {
                    args.subjectClass = ChefZ_SymbolTable.Intern(item.GetType());
                    int low, high;
                    if (VesselId(item, low, high))
                        args.SetSubjectNetId(low, high);
                }
            }
        }

        if (wantEvent)
            bus.RaiseKeep(args);

        // NACH dem Erfolg, nie davor (17 E7). Der Aufruf steht hinter dem
        // Ereignis und hinter dem vollzogenen Verbrauch - das ist Regel §10.6
        // an ihrer Quelle.
        if (wantProgress)
            ChefZ_ProgressRegistry.Report(ChefZ_ProgressKind.COOK, args);

        if (wantCustom && recipe)
        {
            string keep = args.eventId;
            for (int e = 0; e < recipe.emitEvents.Count(); e++)
            {
                args.eventId = recipe.emitEvents.Get(e);
                bus.RaiseKeep(args);
            }
            args.eventId = keep;
        }

        bus.Release(args);
    }

    //! ChefZ_OnRecipeFailed (17 §4). Erst HasSubscribers, dann die Nutzlast.
    private void RaiseRecipeFailed(notnull ItemBase device, notnull ChefZ_MatchResult bound, string err)
    {
        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (!bus.HasSubscribers(ChefZ_EventNames.RECIPE_FAILED))
            return;

        ChefZ_EventArgs args = bus.Acquire(ChefZ_EventNames.RECIPE_FAILED);
        FillCookArgs(args, device, bound);

        // Der Grund gehoert in ein Feld, das es schon gibt, statt in ein neues:
        // 17 E3 lebt davon, dass die Nutzlast wachsen kann, ohne Abonnenten zu
        // brechen - aber jedes Feld, das nur an einer Stelle etwas bedeutet,
        // ist eines zu viel. cancelReason heisst hier "warum es nicht kam".
        args.cancelReason = err;

        bus.Raise(args);
    }

    /**
     * Ein Fehlversuch am selben Gefaess (10 §8, 10 E6).
     *
     * Der dritte sperrt bis zum naechsten Signaturwechsel und meldet EINMAL
     * als ERROR. Ohne das koennte eine unglueckliche Konstellation bei jedem
     * Tick erneut scheitern, jedes Mal loggen und jedes Mal einen Vollmatch
     * kosten.
     */
    private void FailSession(notnull ChefZ_CookSession session, string why)
    {
        if (!session.Fail(FAIL_LIMIT))
        {
            ChefZ_Log.Warn(ChefZ_LogChannel.COOK, "Kochauswertung fehlgeschlagen (" + session.failCount.ToString() + "/" + FAIL_LIMIT.ToString() + ") an Gefaess " + session.vesselId.ToString() + ": " + why);
            return;
        }

        string recipeId = "-";
        if (session.outcome)
            recipeId = session.outcome.recipeId;

        ChefZ_Log.Error(ChefZ_LogChannel.COOK, "Gefaess " + session.vesselId.ToString() + " wird nach " + FAIL_LIMIT.ToString() + " Fehlversuchen bis zur naechsten Inhaltsaenderung uebergangen. " + "Rezept " + recipeId + ", Grund: " + why + ". " + "Vanilla-Kochen ist davon unberuehrt.");
    }

    //==========================================================================
    // Sitzungen (10 §4, 10 §7)
    //==========================================================================

    /**
     * Sitzung zum Gefaess, notfalls neu angelegt. null nur, wenn das Gefaess
     * keine Netz-ID hat - dann gibt es nichts, woran man sich erinnern koennte.
     */
    ChefZ_CookSession GetSession(notnull ItemBase vessel)
    {
        int low, high;
        if (!VesselId(vessel, low, high))
            return null;

        ChefZ_CookSession session;
        if (m_Sessions.Find(low, session))
        {
            if (session.vesselIdHigh == high)
                return session;

            // Dieselbe niedrige Haelfte, anderes Gefaess. Die Sitzung gehoert
            // jemand anderem - sie wird uebernommen und geleert, nicht geteilt.
            session.vesselIdHigh = high;
            session.ResetAll();
            return session;
        }

        session = new ChefZ_CookSession();
        session.vesselId     = low;
        session.vesselIdHigh = high;
        m_Sessions.Set(low, session);

        // 10 §7: aufgeraeumt wird amortisiert beim Einfuegen, nicht ueber
        // einen eigenen Timer. Ein Timer waere ein zweiter Taktgeber fuer ein
        // Problem, das nur beim Wachsen entsteht.
        m_InsertsSinceSweep++;
        if (m_InsertsSinceSweep >= SWEEP_EVERY_INSERTS)
        {
            m_InsertsSinceSweep = 0;
            AgeSessions(TickCount(0));
        }

        return session;
    }

    /**
     * Sitzung zum Gefaess, OHNE eine anzulegen. null, wenn es keine gibt.
     *
     * Fuer Auskunftspfade: die Diagnose soll nachsehen koennen, ohne eine
     * Sitzung zu erzeugen und damit die Alterung anzustossen.
     */
    ChefZ_CookSession PeekSession(notnull ItemBase vessel)
    {
        int low, high;
        if (!VesselId(vessel, low, high))
            return null;

        ChefZ_CookSession session;
        if (!m_Sessions.Find(low, session))
            return null;
        if (session.vesselIdHigh != high)
            return null;                    // dieselbe Haelfte, anderes Gefaess

        return session;
    }

    void DropSession(int vesselId)
    {
        m_Sessions.Remove(vesselId);
    }

    void DropAllSessions()
    {
        m_Sessions.Clear();
        m_InsertsSinceSweep = 0;
    }

    int GetActiveSessionCount()
    {
        return m_Sessions.Count();
    }

    /**
     * Sitzungen ohne Kochtick seit sessionTtlSec verwerfen (10 §7).
     *
     * "Ohne das wuechse die Map ueber die Serverlaufzeit unbegrenzt."
     *
     * Der Entwurf nennt als zweites Kriterium "dessen Entity nicht mehr
     * existiert". Das wird hier bewusst NICHT geprueft, sondern von der Uhr
     * mit erledigt: eine Sitzung haelt kein ItemBase (siehe Kopf von
     * ChefZ_CookSession), und ein geloeschtes Gefaess hat per Definition
     * keinen Kochtick mehr. Die Uhr ist die vollstaendigere der beiden
     * Bedingungen, nicht die schwaechere.
     *
     * GetKey ist laut EnScript.c O(n); der Durchlauf ist damit O(n^2). Bei
     * einer Handvoll bis einigen Dutzend gleichzeitig kochender Gefaesse ist
     * das nichts, und er laeuft nur alle SWEEP_EVERY_INSERTS Neuanlagen.
     */
    void AgeSessions(int currentTick)
    {
        int count = m_Sessions.Count();
        if (count == 0)
            return;

        float ttlSec = ChefZ_ConfigManager.Get().GetSettings().sessionTtlSec;
        if (ttlSec <= 0.0)
            return;

        int ttlMs = (int)(ttlSec * 1000.0);

        array<int> stale = new array<int>();
        for (int i = 0; i < count; i++)
        {
            int key = m_Sessions.GetKey(i);
            ChefZ_CookSession session = m_Sessions.Get(key);
            if (!session)
            {
                stale.Insert(key);
                continue;
            }
            if (session.AgeMillis(currentTick) >= ttlMs)
                stale.Insert(key);
        }

        for (int k = 0; k < stale.Count(); k++)
            m_Sessions.Remove(stale.Get(k));

        m_CountSweptSessions = m_CountSweptSessions + stale.Count();

        if (stale.Count() > 0 && ChefZ_Log.Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.COOK, "Sitzungen gealtert: " + stale.Count().ToString() + " verworfen, " + m_Sessions.Count().ToString() + " aktiv.");
        }
    }

    //==========================================================================
    // Signatur (10 §5 Stufe A)
    //==========================================================================

    /**
     * Der Cargo-Durchlauf OHNE jeden Registry-Zugriff.
     *
     * Es gibt in dieser Methode und allem, was sie ruft, keinen Aufruf an den
     * Category-, Ingredient- oder Recipe-Manager und kein Intern() in die
     * Symboltabelle. 10 §5 begruendet das: die Drosselung muss auch bei
     * degradierter Konfiguration funktionieren - gerade dann.
     *
     * Das Gefaess selbst geht NICHT in die Signatur ein (01 V13: Pot und
     * Cauldron sind selbst Edible_Base und wuerden sonst als eigene Zutat
     * mitzaehlen). Seine Fluessigkeit und seine Menge sehr wohl - die sind
     * Eigenschaften des Gefaesses und nicht des Inhalts.
     */
    bool BuildSignature(notnull ItemBase vessel, int cookingMethod, out ChefZ_VesselSignature sig)
    {
        ChefZ_VesselSignature target = sig;
        if (!target)
            target = new ChefZ_VesselSignature();

        bool ok = MeasureSignature(vessel, cookingMethod, target);
        sig = target;
        return ok;
    }

    //! Dieselbe Messung, aber in eine mitgebrachte Signatur - der Weg, den der
    //! Adapter selbst geht, weil sein Puffer ein Feld ist.
    bool MeasureSignature(notnull ItemBase vessel, int cookingMethod, notnull ChefZ_VesselSignature sig)
    {
        int liquidType = LIQUID_NONE;
        if (vessel.IsLiquidContainer())
            liquidType = vessel.GetLiquidType();

        sig.BeginMeasure(cookingMethod, liquidType, vessel.GetQuantity());

        GameInventory inventory = vessel.GetInventory();
        if (!inventory)
            return true;                    // ein Gefaess ohne Inventar ist leer

        CargoBase cargo = inventory.GetCargo();
        if (!cargo)
            return true;

        int count = cargo.GetItemCount();
        for (int i = 0; i < count; i++)
        {
            ItemBase item = ItemBase.Cast(cargo.GetItem(i));
            if (!item)
                continue;
            if (item == vessel)             // 01 V13
                continue;

            string type = item.GetType();
            if (type == "")
                continue;

            sig.AddItem(type.Hash(), FoodStageOf(item), ChefZStateOrdinalOf(item));
        }

        return true;
    }

    /**
     * Vanilla-Garstufe eines Items, -1 wenn keine.
     *
     * Echte Falle, dieselbe wie im ChefZ_FactCollector:
     * Edible_Base.GetFoodStageType() ruft intern GetFoodStage().
     * GetFoodStageType() OHNE Nullpruefung (Edible_Base.c:531). Ein
     * Edible_Base ohne FoodStage - etwa ein leerer Topf im Topf - loeste damit
     * einen Nullzugriff aus.
     */
    private static int FoodStageOf(notnull ItemBase item)
    {
        Edible_Base edible = Edible_Base.Cast(item);
        if (!edible)
            return -1;

        FoodStage stage = edible.GetFoodStage();
        if (!stage)
            return -1;

        return stage.GetFoodStageType();
    }

    /**
     * ChefZ-Sync-Ordinal des Zustands, -1 wenn keiner.
     *
     * Bis S9 durchgehend -1: der Zustand lebt auf ChefZ_Edible_Base /
     * ChefZ_Item_Base (06 §4.3), und die entstehen dort. Der Platz steht hier,
     * damit S9 ihn nicht sucht - und damit klar ist, dass die Maske in der
     * Signatur kein totes Feld ist, sondern ein noch leeres.
     *
     * Wichtig fuer S9: die Antwort muss OHNE Registry auskommen. Der Ordinal
     * steht auf dem Item, das genuegt.
     */
    private static int ChefZStateOrdinalOf(notnull ItemBase item)
    {
        return -1;
    }

    private static int CargoCount(notnull ItemBase vessel)
    {
        GameInventory inventory = vessel.GetInventory();
        if (!inventory)
            return 0;

        CargoBase cargo = inventory.GetCargo();
        if (!cargo)
            return 0;

        return cargo.GetItemCount();
    }

    //==========================================================================
    // Geraeteaufloesung (10 E7)
    //==========================================================================

    /**
     * Ist dieses Gefaess fuer ChefZ ein Kochgeraet - und mit welchen Daten?
     *
     * @return true, wenn ueberhaupt ein Deskriptor herausgegeben werden konnte.
     *         Der Aufrufer prueft danach desc.enabled; false heisst nur, dass
     *         nicht einmal die Klasse ermittelbar war.
     *
     * Die Auskunft ist zwischengespeichert, EINSCHLIESSLICH der Negativfaelle -
     * siehe m_DeviceCache. deviceRootClass wird bei jedem Aufruf neu gesetzt,
     * weil sie als einziges Feld an der Instanz haengt (siehe
     * ChefZ_DeviceDescriptor).
     */
    bool ResolveDevice(notnull ItemBase vessel, out ChefZ_DeviceDescriptor desc)
    {
        string className = vessel.GetType();
        if (className == "")
            return false;

        ChefZ_Sym classSym = ChefZ_SymbolTable.Intern(className);
        if (!ChefZ_SymbolTable.IsValid(classSym))
            return false;

        ChefZ_DeviceDescriptor cached;
        if (!m_DeviceCache.Find(classSym, cached))
        {
            cached = BuildDescriptor(classSym, className);
            m_DeviceCache.Set(classSym, cached);
        }

        cached.deviceRootClass = RootClassOf(vessel);
        desc = cached;
        return true;
    }

    /**
     * Deskriptor aus der Geraeteregistry.
     *
     * Die Registry wird aus CfgChefZDevices (Rang 1) und den JSON-Quellen
     * (Rang 2/3) gespeist - beides landet als Record der Art "device" im
     * selben Bestand (02 §6). 10 §4 nennt CfgChefZDevices, weil das die
     * Stelle ist, an der ein Content-Modul ein Geraet deklariert; die
     * Sicherung ist dieselbe, egal aus welchem Rang der Eintrag stammt.
     *
     * ---- Vererbung entlang CfgVehicles ------------------------------------
     * Findet sich fuer die Klasse selbst kein Eintrag, wird die
     * CfgVehicles-Kette aufgestiegen. Ein "Pot_Variant : Pot" ist damit ein
     * ChefZ-Geraet, ohne dass jemand ihn eigens eintraegt - dieselbe Regel,
     * die 05 E2 fuer Zutaten festlegt.
     *
     * Das schwaecht 10 E7 NICHT ab. Die Aussage dort lautet "ein Kochgeraet,
     * von dem ChefZ nichts weiss, verhaelt sich exakt wie ohne ChefZ", und
     * eine abgeleitete Klasse ist eine, von der ChefZ weiss - durch ihre
     * Basis. Was keine deklarierte Vorfahrenklasse hat, faellt weiterhin
     * vollstaendig durch.
     */
    private ChefZ_DeviceDescriptor BuildDescriptor(ChefZ_Sym classSym, string className)
    {
        ChefZ_DeviceDescriptor desc = new ChefZ_DeviceDescriptor();
        desc.deviceClass = classSym;

        ChefZ_ConfigManager cfg = ChefZ_ConfigManager.Get();
        ChefZ_Registry<ChefZ_DeviceDef> devices = cfg.Devices();
        if (!devices)
            return desc;

        string current = className;
        ChefZ_DeviceDef def = devices.FindByName(current);

        // Der Aufstieg ist begrenzt: eine kaputte oder zyklische Config darf
        // hier nicht endlos laufen. maxSelectorDepth ist die falsche Groesse -
        // CfgVehicles-Ketten sind flach, aber tiefer als acht.
        int guard = 0;
        while (!def && guard < 32)
        {
            guard++;

            string parent;
            if (!g_Game || !g_Game.ConfigGetBaseName(ChefZ_IngredientManager.CFG_VEHICLES + " " + current, parent))
                break;
            if (parent == "" || parent == current)
                break;

            current = parent;
            def = devices.FindByName(current);
        }

        if (!def)
            return desc;                    // kein ChefZ-Geraet - enabled bleibt false

        desc.enabled         = true;
        desc.declaredAs      = ChefZ_SymbolTable.Intern(def.id);
        desc.portionCapacity = def.portionCapacity;
        desc.qualityModifier = def.qualityModifier;

        if (def.deviceCategories)
        {
            for (int i = 0; i < def.deviceCategories.Count(); i++)
            {
                string category = def.deviceCategories.Get(i);
                if (category == "")
                    continue;
                desc.AddCategory(ChefZ_SymbolTable.Intern(category));
            }
        }

        if (ChefZ_Log.Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.COOK, "Kochgeraet erkannt: " + desc.ToDebugString());
        }

        return desc;
    }

    /**
     * Die Klasse UNTER dem Gefaess - Feuerstelle, Fass, Gasherd (08 §3).
     *
     * Der Hierarchieelternteil und nicht etwa eine Suche in der Umgebung: ein
     * Topf auf einem Kochstaender ist an der Feuerstelle angebaut, ein Topf im
     * Rucksack eines Spielers an dessen Inventar. Im zweiten Fall wird gerade
     * ohnehin nicht gekocht.
     */
    private static ChefZ_Sym RootClassOf(notnull ItemBase vessel)
    {
        EntityAI parent = vessel.GetHierarchyParent();
        if (!parent)
            return ChefZ_SymbolTable.INVALID;

        string type = parent.GetType();
        if (type == "")
            return ChefZ_SymbolTable.INVALID;

        return ChefZ_SymbolTable.Intern(type);
    }

    //==========================================================================
    // Kontext und Fakten (10 §4)
    //==========================================================================

    /**
     * Die Schnittstelle aus 10 §4, woertlich.
     *
     * Sie fuellt die drei Puffer des Adapters und gibt sie heraus. Der
     * Aufrufer bekommt Zeiger auf wiederverwendete Objekte und darf sie
     * ausschliesslich bis zum naechsten Aufruf benutzen (10 §7: "Snapshot- und
     * Entity-Puffer - Laufzeit, wiederverwendet").
     *
     * updateTime steht in der Signatur, weil 10 §4 sie so festlegt, und wird
     * hier NICHT in den Kontext geschrieben: ChefZ_CookContext.elapsedSec ist
     * die aufgelaufene Zeit der Sitzung, nicht die Laenge eines Ticks. Die
     * Sitzung fuehrt sie (10 §6), und Stufe C setzt sie unmittelbar vor der
     * Abschlusspruefung.
     */
    bool BuildContext(notnull ItemBase vessel, float updateTime, int method, out ChefZ_CookContext ctx, out array<ItemBase> outEntities, out ChefZ_FactSnapshot snapshot)
    {
        ChefZ_DeviceDescriptor desc;
        if (!ResolveDevice(vessel, desc))
            return false;

        // PeekSession und nicht GetSession: diese Methode ist die Auskunft
        // aus 10 §4 und wird von der Diagnose gerufen ("chefz match", 18 E6).
        // Eine Auskunft darf keine Sitzung ANLEGEN - sonst veraendert das
        // Nachsehen den Zustand, ueber den es Auskunft gibt.
        int actorId = 0;
        ChefZ_CookSession session = PeekSession(vessel);
        if (session)
            actorId = session.actorIdentityId;

        if (!BuildContextFrom(vessel, desc, actorId, updateTime, method))
            return false;

        ctx         = m_Ctx;
        outEntities = m_Entities;
        snapshot    = m_Snapshot;
        return true;
    }

    private bool BuildContextFrom(notnull ItemBase vessel, notnull ChefZ_DeviceDescriptor desc, int actorIdentityId, float updateTime, int method)
    {
        if (!ChefZ_FactCollector.CollectContext(vessel, desc, method, m_Ctx))
            return false;

        // Der Sammler kann die Identitaet nicht kennen und soll es auch nicht:
        // er liest EIN Gefaess und weiss nichts ueber die Vorgeschichte. Wer
        // an diesem Gefaess gehandelt hat, weiss allein die Sitzung - und die
        // gehoert dem Adapter (10 §7). Deshalb wird hier gestempelt und nicht
        // dort.
        //
        // Der Wert ist damit fuer den GANZEN Tick festgelegt, bevor die
        // Auswertung beginnt: Bindung, Qualitaetsabwertung und Ereignis sehen
        // dieselbe Zahl. Er aendert sich nur bei einem Bestandszuwachs, und
        // ein Bestandszuwachs aendert die Signatur - die gebundene Auswertung
        // laeuft danach ohnehin neu. Damit bleibt die Zusage aus 08 §7
        // erhalten: dieselbe Eingabe ergibt dasselbe Ergebnis.
        m_Ctx.actorIdentityId = actorIdentityId;

        // Ueber lokale Zwischenvariablen: CollectFromCargo nimmt beide Listen
        // als out-Parameter, und m_Snapshot / m_Entities sind FELDER (siehe
        // Kopf von ChefZ_TextList.SymbolsOf). Beide sind bereits angelegt, der
        // Sammler legt also nichts neu an - die Zuweisung danach ist die
        // Absicherung fuer den Fall, dass er es doch tut.
        ChefZ_FactSnapshot snapshot = m_Snapshot;
        array<ItemBase>    entities = m_Entities;
        ChefZ_FactCollector.CollectFromCargo(vessel, snapshot, entities);
        m_Snapshot = snapshot;
        m_Entities = entities;
        return true;
    }

    //! Die zuletzt gebauten Puffer, fuer Diagnose und fuer S8.
    ChefZ_CookContext  CurrentContext()  { return m_Ctx; }
    ChefZ_FactSnapshot CurrentSnapshot() { return m_Snapshot; }
    array<ItemBase>    CurrentEntities() { return m_Entities; }

    //==========================================================================
    // Diagnose (18 §2, "chefz stats")
    //==========================================================================

    void ResetCounters()
    {
        m_CountObserved      = 0;
        m_CountSignatureHits = 0;
        m_CountFullMatches   = 0;
        m_CountMatched       = 0;
        m_CountCompleted     = 0;
        m_CountSweptSessions = 0;

        // Die Zahlen der Transaktion gehoeren zur selben Messung. Wer "chefz
        // stats" zuruecksetzt, will nicht die Haelfte behalten.
        ChefZ_Applicator.ResetCounters();
    }

    void DumpStats(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Kochadapter (Stufe 0/A/B/C)");
        outLines.Insert("  Sitzungen aktiv      " + m_Sessions.Count().ToString());
        outLines.Insert("  Sitzungen gealtert   " + m_CountSweptSessions.ToString());
        outLines.Insert("  Ticks beobachtet     " + m_CountObserved.ToString());
        outLines.Insert("  davon Signaturtreffer" + m_CountSignatureHits.ToString());
        outLines.Insert("  Vollmatches          " + m_CountFullMatches.ToString());
        outLines.Insert("  Bindungen            " + m_CountMatched.ToString());
        outLines.Insert("  Abschluesse          " + m_CountCompleted.ToString());
        outLines.Insert("  Geraeteklassen       " + m_DeviceCache.Count().ToString());

        // Die Transaktion fuehrt ihre eigenen Zahlen (08 §6). Sie stehen im
        // selben Block, weil ein Betreiber sie zusammen liest: "vier
        // Abschluesse, aber nur drei angewandt" ist die interessante Zeile.
        //
        // Ueber eine lokale Zwischenvariable: einen out-Parameter als
        // out-Parameter weiterzureichen ist in Enforce nirgends zugesichert
        // (siehe Kopf von ChefZ_TextList.SymbolsOf).
        array<string> lines = outLines;
        ChefZ_Applicator.DumpStats(lines);
        outLines = lines;

        for (int i = 0; i < m_Sessions.Count(); i++)
        {
            ChefZ_CookSession session = m_Sessions.GetElement(i);
            if (session)
                outLines.Insert("  " + session.ToDebugString());
        }
    }

    //! Nur fuer Test und Diagnose: verwirft Sitzungen und Geraetezwischenspeicher.
    void ClearCaches()
    {
        DropAllSessions();
        m_DeviceCache.Clear();
    }

    //==========================================================================

    /**
     * Netz-ID des Gefaesses in zwei Haelften.
     *
     * "This id is shared between client and server for whole server-client
     * session" (Object.c:814). Sie ist damit genau das, was eine Sitzung
     * braucht: stabil, solange das Gefaess existiert, und nicht persistiert -
     * einen Neustart soll die Sitzung ohnehin nicht ueberleben (10 §7).
     */
    static bool VesselId(notnull ItemBase vessel, out int low, out int high)
    {
        int lo = 0;
        int hi = 0;
        vessel.GetNetworkID(lo, hi);
        low  = lo;
        high = hi;
        return (lo | hi) != 0;
    }
}
