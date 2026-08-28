//==============================================================================
// ChefZ_DecayPlan / ChefZ_ItemDecay - die Itemseite der Haltbarkeit
//
// Entwurf: 14 §2 (der Eingriffspunkt), 14 §4 (Restfrische), 14 §6
// (Datenfluss), 14 §7 (der Multiplikator wird NICHT persistiert), 14 §8
// (Fehlerverhalten), 14 E1 (delta skalieren statt ProcessDecay ersetzen),
// 14 E2 (kein modded class Edible_Base), 14 E7/E8, 01 V9 (die Vanilla-Fakten).
//
// ---------------------------------------------------------------------------
// Warum diese Datei existiert und nicht alles in ChefZ_Edible_Base steht
// ---------------------------------------------------------------------------
// In ChefZ_Edible_Base.ProcessDecay soll GENAU DAS stehen, was 14 E1 verspricht
// und was jeder Leser in fuenf Sekunden pruefen koennen muss:
//
//     super.ProcessDecay(delta * mul, hasRootAsPlayer);
//
// Eine Zeile, ein super-Aufruf, keine Vanilla-Konstante nachgebaut. Alles
// andere - Faktenerhebung, Umgebungstemperatur, Frischefortschreibung,
// Sync-Drosselung, Verderb-Erkennung - ist Buchhaltung und steht hier. Wer
// wissen will, ob ChefZ am Vanilla-Verfall dreht, liest die andere Datei und
// ist fertig.
//
// ---------------------------------------------------------------------------
// Die wichtigste Zusage des ganzen Teilsystems (14 §8, letzte Zeile)
// ---------------------------------------------------------------------------
//   "Item ist kein ChefZ_Edible_Base -> verdirbt exakt wie ohne ChefZ. Es gibt
//    keinen Pfad, auf dem ChefZ Vanilla-Nahrung beeinflusst."
//
// Diese Zusage ist hier STRUKTURELL erfuellt, nicht durch eine Pruefung: der
// gesamte Code dieser Datei wird ausschliesslich aus ChefZ_Edible_Base
// gerufen, und die ist eine ABLEITUNG. Es gibt kein "modded class
// Edible_Base", keinen Hook auf ItemBase.ProcessVariables und keine
// Registrierung, ueber die ein Vanilla-Steak hier hereinkaeme (14 E2, 06 §2).
//
// Layer: 4_World. Hier leben ItemBase, Edible_Base, FoodStage und die
// Weltdaten; in 3_Game gibt es sie nicht.
//==============================================================================

/**
 * Das Ergebnis der Vorberechnung eines Verfallsticks.
 *
 * Bewusst KEIN persistiertes und KEIN gecachtes Objekt (14 §7 / E4): es lebt
 * genau die Dauer eines ProcessDecay-Aufrufs. Wuerde man den Faktor speichern,
 * wirkte eine Balancing-Aenderung des Betreibers nie auf bestehende Items - so
 * wirkt sie sofort, ohne Migration.
 */
class ChefZ_DecayPlan : Managed
{
    //! Faktor auf das delta, das an Vanilla geht. Immer > 0.
    float scale;

    //! Faktor auf das delta der Frischefortschreibung. Siehe
    //! ChefZ_ItemDecay.Plan(), Abschnitt "Frische und Verfall im selben Takt".
    float freshnessScale;

    //! true => es wird gar nicht verfallen (14 E7).
    bool stopsDecay;

    //! Der ermittelte ChefZ-Zustand. Bestimmt die Frischelebensdauer.
    ChefZ_Sym state;

    //! Vanilla-Garstufe VOR dem super-Aufruf. Speist die Verderb-Erkennung
    //! (14 E8: das Ereignis feuert nach dem tatsaechlichen Uebergang, nicht
    //! wenn der Timer ablaeuft).
    int stageBefore;

    void ChefZ_DecayPlan()
    {
        Reset();
    }

    void Reset()
    {
        scale          = 1.0;
        freshnessScale = 1.0;
        stopsDecay     = false;
        state          = ChefZ_SymbolTable.INVALID;
        stageBefore    = ChefZ_VanillaStage.NONE;
    }
}

//==============================================================================

class ChefZ_ItemDecay
{
    /**
     * Wiederverwendete Puffer.
     *
     * Ein Verfallstick laeuft je Item nur im Minutentakt, aber er laeuft fuer
     * JEDES Lebensmittel auf dem Server. Zwei dauerhafte Objekte sind billiger
     * als zwei Allokationen je Item und Tick - dieselbe Ueberlegung wie beim
     * Vorrat des ChefZ_FactSnapshot (05 §5).
     *
     * DIE BEDINGUNG, UNTER DER DAS TRAEGT, und sie ist geprueft, nicht
     * gehofft: der Pfad ist nicht wiedereintrittsfaehig. Zwischen Plan() und
     * AfterVanilla() laeuft genau ein fremder Aufruf - super.ProcessDecay -,
     * und der arbeitet ausschliesslich auf DIESEM Item. Was er an ChefZ-Code
     * ausloesen kann, ist vollstaendig aufzaehlbar:
     *
     *   ChangeFoodStage    -> ChefZ_Edible_Base.ChangeFoodStage
     *                         -> ChefZ_ItemDecay.PreventsRotten  (nutzt s_Facts,
     *                            aber nie s_Plan - und s_Facts ist zu diesem
     *                            Zeitpunkt bereits ausgewertet)
     *   OnFoodStageChange  -> ChefZ_ItemStateComponent.OnVanillaStageChanged
     *                         (nutzt keinen der beiden Puffer)
     *
     * Keiner dieser Pfade ruft Plan(). Kaeme je einer dazu, gehoert dieser
     * Kommentar mitgeaendert - und der Puffer durch eine Allokation ersetzt.
     */
    private static ref ChefZ_DecayPlan s_Plan;
    private static ref ChefZ_ItemFacts s_Facts;

    //! Bis S17 gibt es keinen Behaelterfaktor (16 §4). 1.0 ist der Wert, den
    //! 14 §3 dafuer ausdruecklich als Default nennt.
    static const float NO_CONTAINER_MODIFIER = 1.0;

    //==========================================================================
    // Schritt 1: die Vorberechnung
    //==========================================================================

    /**
     * Alles, was ChefZ ueber diesen Verfallstick zu sagen hat.
     *
     * @return null heisst "ChefZ hat nichts zu sagen". Der Aufrufer ruft dann
     *         super.ProcessDecay(delta, ...) UNVERAENDERT - der Verfall ist in
     *         diesem Fall bitgenau Vanilla. Das ist der Ausfallpfad fuer
     *         SAFE_MODE, fuer "Config nie geladen", fuer den Client und fuer
     *         jeden Fehler, den wir hier nicht vorhergesehen haben (14 §8).
     */
    static ChefZ_DecayPlan Plan(notnull ItemBase item, bool hasRootAsPlayer)
    {
        // Serverseitig, ausnahmslos. ItemBase.ProcessVariables laeuft ohnehin
        // nur dort, aber die Zeile steht trotzdem: sie ist der Unterschied
        // zwischen "faellt nicht auf" und "kann nicht passieren" (14 §6:
        // "Ausschliesslich Server").
        if (!g_Game || !g_Game.IsServer())
            return null;

        // 14 §8: SAFE_MODE -> Faktor 1.0, kein Log. IsActive() deckt SAFE_MODE,
        // "enabled = false" und "nie geladen" in einem Test ab.
        ChefZ_ConfigManager cfg = ChefZ_ConfigManager.Get();
        if (!cfg || !cfg.IsActive())
            return null;

        ChefZ_PreservationManager mgr = ChefZ_PreservationManager.Get();
        if (!mgr.IsReady())
            return null;

        ChefZ_ItemFacts facts;
        if (!CollectFacts(item, facts))
            return null;

        ChefZ_DecayPlan plan = TakePlan();
        plan.state       = facts.chefzState;
        plan.stageBefore = StageOf(item);

        float envTemp = EnvironmentTemperature(item);

        // 14 E7: der Konservenschalter. Er wird HIER noch einmal gefragt,
        // obwohl CanProcessDecay ihn bereits fragt - und das ist Absicht:
        // CanProcessDecay ist Vanillas Tuersteher und kann von einem anderen
        // Mod ueberschrieben werden. Ein Schutz, der an einer Zusage haengt,
        // die uns nicht gehoert, ist kein Schutz. Die Abfrage kostet einen
        // Map-Zugriff je Dimension.
        plan.stopsDecay = mgr.StopsDecay(facts.chefzState, facts.chefzQuality, facts.classSym, facts.closure, facts.tags);
        if (plan.stopsDecay)
            return plan;

        array<string> noTrace = null;
        float mul = mgr.ComputeDecayScale(facts.chefzState, facts.chefzQuality, facts.classSym, facts.closure, facts.tags, NO_CONTAINER_MODIFIER, envTemp, noTrace);

        if (hasRootAsPlayer)
        {
            // ZUSAETZLICH zu Vanillas DECAY_RATE_ON_PLAYER, nicht statt
            // dessen - die Begruendung steht bei
            // ChefZ_PreservationDef.onPlayerMultiplier. Ohne Regel ist der
            // Faktor 1.0 und diese Zeile ist wirkungslos.
            mul = mgr.ClampToBounds(mul * mgr.ComputeOnPlayerScale( facts.chefzState, facts.chefzQuality, facts.classSym, facts.closure, facts.tags, envTemp));
        }

        plan.scale = mul;

        // --- Frische und Verfall im selben Takt ------------------------------
        // 14 §4 schreibt die Frischefortschreibung als
        //     Freshness01 -= (delta * mul) / freshnessLifetimeSec
        // und meint mit "delta" das delta dieses Ticks.
        //
        // EHRLICH BENANNTE PRAEZISIERUNG: hier kommt zusaetzlich Vanillas
        // globaler Verfallsmodifikator hinein. Vanilla wendet ihn als ERSTE
        // Zeile seines eigenen ProcessDecay auf dasselbe delta an
        // (01 V9, Edible_Base.c:744). Liesse man ihn hier weg, liefe die
        // Frische auf einem Server mit GetFoodDecayModifier = 3 in einem
        // anderen Takt als der Verfall - Fleisch waere verrottet, aber laut
        // ChefZ noch frisch. 14 E2 nennt genau diesen Modifikator als die
        // Stellschraube, an der ein Betreiber drehen SOLL; sie muss dann auch
        // wirken.
        //
        // Der Fall "Modifikator ist 0" braucht keinen Sonderfall: dann ist
        // IsFoodDecayEnabled() false und ProcessDecay wird gar nicht gerufen
        // (14 §8, "Kein Sonderfall im Code").
        plan.freshnessScale = mul * FoodDecayModifier();

        return plan;
    }

    //==========================================================================
    // Schritt 2: die Buchhaltung nach dem Vanilla-Aufruf
    //==========================================================================

    /**
     * Frische fortschreiben, gedrosselt synchronisieren, Verderb erkennen
     * (14 §6, Zeilen 5 bis 7 des Laufzeitablaufs).
     *
     * @param rawDelta  das delta, wie es HEREINKAM - nicht das skalierte. Die
     *        Skalierung steckt in plan.freshnessScale, und sie zweimal
     *        anzuwenden waere ein quadratischer Fehler, den niemand sieht,
     *        weil das Ergebnis immer noch monoton faellt.
     */
    static void AfterVanilla(notnull ItemBase item, notnull ChefZ_DecayPlan plan, float rawDelta)
    {
        AdvanceFreshness(item, plan, rawDelta);
        DetectSpoiled(item, plan);
    }

    /**
     * 14 §4 und 14 §6, Zeile 6: der Sync ist GEDROSSELT.
     *
     * "Die Drosselung des Sync ist Absicht: Freshness01 wird mit Praezision 2
     *  synchronisiert; ohne Schwellwertpruefung loeste JEDER Decay-Tick jedes
     *  Lebensmittels einen Sync-Write aus."
     *
     * Die Schwelle ist deshalb nicht frei gewaehlt, sondern exakt die
     * Aufloesung der Leitung: RegisterNetSyncVariableFloat(..., 0, 1, 2)
     * (03 §4). Was der Client ohnehin nicht unterscheiden kann, wird nicht
     * geschickt.
     */
    private static void AdvanceFreshness(notnull ItemBase item, notnull ChefZ_DecayPlan plan, float rawDelta)
    {
        ChefZ_ItemStateComponent comp = ChefZ_ItemStateComponent.Of(item);
        if (!comp)
            return;

        float current = comp.GetFreshness01();

        // 14 §8: "Freshness01 persistiert als NaN oder ausserhalb 0..1 -> auf
        // 1.0 gesetzt, WARN mit Klassenname." Der Klassenname ist der Grund,
        // warum diese Meldung hier steht und nicht im Manager: dort gibt es
        // nur Zahlen.
        if (ChefZ_PreservationManager.IsFreshnessBroken(current))
        {
            ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PRESERV, "preserv.freshness." + item.GetType(), "\"" + item.GetType() + "\" traegt eine unbrauchbare Restfrische (" + current.ToString() + "). Sie wird auf 1.0 gesetzt. Ursache ist ein " + "beschaedigter Spielstandblock oder eine Rechnung, die NaN erzeugt hat. " + "Diese Meldung erscheint je Klasse genau einmal.");
            current = 1.0;
        }

        float next = ChefZ_PreservationManager.Get().AdvanceFreshness( current, rawDelta, plan.freshnessScale, plan.state);

        ChefZ_ItemStateComponent.SetFreshness01Throttled(item, next);
    }

    /**
     * 14 E8: das Verderb-Ereignis feuert erst NACH dem tatsaechlichen
     * Uebergang.
     *
     * "Damit kann ein Abonnent (Statistik, Quest, Achievement) sich auf das
     *  Ereignis verlassen, und es gibt kein Doppelfeuern bei Items, die
     *  bereits verrottet sind."
     *
     * Deshalb der Vergleich gegen plan.stageBefore und nicht gegen den
     * Decay-Timer: der Timer laeuft auch ab, wenn Vanilla anschliessend auf
     * DRIED statt auf ROTTEN wechselt (Obst, 01 V9), und er laeuft ausserdem
     * bei einem Item ab, das laengst verrottet ist.
     */
    private static void DetectSpoiled(notnull ItemBase item, notnull ChefZ_DecayPlan plan)
    {
        int stageNow = StageOf(item);
        if (stageNow != ChefZ_VanillaStage.ROTTEN)
            return;
        if (plan.stageBefore == ChefZ_VanillaStage.ROTTEN)
            return;

        // EVENT (14 §6, 17): ChefZ_OnFoodSpoiled.
        //
        // Der Event-Bus entsteht mit S18; die Zeile hier vorwegzunehmen hiesse,
        // sie zweimal zu schreiben. Was der Bus brauchen wird, steht
        // vollstaendig in dieser Methode: das Item, seine Klasse, der
        // ChefZ-Zustand vor dem Verderben (plan.state) und die Vanilla-Stufe
        // davor (plan.stageBefore). Dieselbe Loesung wie in
        // ChefZ_ItemStateComponent.SetState.
        if (ChefZ_Log.Enabled(ChefZ_LogChannel.PRESERV, ChefZ_LogLevel.DEBUG))
            ChefZ_Log.Debug(ChefZ_LogChannel.PRESERV, item.GetType() + ": verdorben. " + ChefZ_VanillaStage.Name(plan.stageBefore) + " -> " + ChefZ_VanillaStage.Name(stageNow) + "  zustand=" + ChefZ_SymbolTable.NameOrMark(plan.state) + "  faktor=" + plan.scale.ToString());
    }

    //==========================================================================
    // Die beiden Einzelabfragen der Traegerklasse
    //==========================================================================

    /**
     * Speist CanProcessDecay() (14 §5: "super && !PreservationManager.
     * StopsDecay(...)").
     *
     * Sie fragt den Manager erneut, statt einen Plan zu bauen: CanProcessDecay
     * laeuft in ItemBase.ProcessVariables VOR ProcessDecay und fuer jedes
     * Lebensmittel, auch fuer solche, die danach gar nicht verfallen. Ein
     * vollstaendiger Plan waere dafuer zu teuer; die Faktenerhebung dagegen
     * ist dieselbe.
     */
    static bool StopsDecay(notnull ItemBase item)
    {
        if (!g_Game || !g_Game.IsServer())
            return false;

        ChefZ_ConfigManager cfg = ChefZ_ConfigManager.Get();
        if (!cfg || !cfg.IsActive())
            return false;

        // ZUERST die billige Frage: gibt es ueberhaupt IRGENDEINE Regel mit
        // stopsDecay? Auf einem Server ohne Konservenregel ist die Antwort
        // nein, und dann kostet diese Methode nichts - keine Faktenerhebung,
        // keine Closure, keine Tagliste.
        //
        // Das ist hier kein Feinschliff, sondern noetig: CanProcessDecay laeuft
        // aus ItemBase.ProcessVariables fuer JEDES Lebensmittel bei JEDEM
        // Update-Tick, nicht nur im Verfallstakt.
        ChefZ_PreservationManager mgr = ChefZ_PreservationManager.Get();
        if (!mgr.HasAnyStopsDecay())
            return false;

        ChefZ_ItemFacts facts;
        if (!CollectFacts(item, facts))
            return false;

        return mgr.StopsDecay(facts.chefzState, facts.chefzQuality, facts.classSym, facts.closure, facts.tags);
    }

    /**
     * Speist den ChangeFoodStage-Override der Traegerklasse (14 E7,
     * preventsRotten).
     *
     * Warum der Uebergang VERHINDERT und nicht nachtraeglich rueckgaengig
     * gemacht wird: Vanilla setzt bei jedem Stufenwechsel m_DecayTimer neu,
     * sobald m_LastDecayStage != GetFoodStageType() ist (01 V9). Ein
     * nachtraeglich zurueckgesetzter Stufenwert liesse m_LastDecayStage und
     * die Stufe wieder gleich, der abgelaufene Timer bliebe abgelaufen - und
     * das Item wuerde bei JEDEM Tick erneut verrotten und erneut
     * zurueckgesetzt. Ein Flackern mit Ereignissen im Sekundentakt.
     *
     * Der Aufruf ist bewusst teuer und bewusst selten: er laeuft nur, wenn
     * ueberhaupt jemand ROTTEN setzen will.
     */
    static bool PreventsRotten(notnull ItemBase item)
    {
        if (!g_Game || !g_Game.IsServer())
            return false;

        ChefZ_ConfigManager cfg = ChefZ_ConfigManager.Get();
        if (!cfg || !cfg.IsActive())
            return false;

        // Dieselbe billige Vorfrage wie in StopsDecay: ohne eine einzige
        // preventsRotten-Regel kostet der ChangeFoodStage-Override nichts
        // ausser dem int-Vergleich, der ihn ueberhaupt erst hierher gebracht
        // hat.
        ChefZ_PreservationManager mgr = ChefZ_PreservationManager.Get();
        if (!mgr.HasAnyPreventsRotten())
            return false;

        ChefZ_ItemFacts facts;
        if (!CollectFacts(item, facts))
            return false;

        return mgr.PreventsRotten(facts.chefzState, facts.chefzQuality, facts.classSym, facts.closure, facts.tags);
    }

    /**
     * Der aktuelle Faktor, ohne ihn anzuwenden. Fuer Debug-Text und Cookbook
     * (14 §5, ChefZ_GetDecayScale).
     *
     * @param trace  darf null sein. Wer ihn uebergibt, bekommt die
     *        Produktkette aus 14 §3 zeilenweise aufgeschluesselt - das ist die
     *        Antwort auf "warum haelt das Ding so lange".
     */
    static float ComputeScale(notnull ItemBase item, out array<string> trace)
    {
        ChefZ_PreservationManager mgr = ChefZ_PreservationManager.Get();
        if (!mgr.IsReady())
            return ChefZ_PreservationManager.NEUTRAL;

        ChefZ_ItemFacts facts;
        if (!CollectFacts(item, facts))
            return ChefZ_PreservationManager.NEUTRAL;

        return mgr.ComputeDecayScale(facts.chefzState, facts.chefzQuality, facts.classSym, facts.closure, facts.tags, NO_CONTAINER_MODIFIER, EnvironmentTemperature(item), trace);
    }

    //==========================================================================
    // Hilfen
    //==========================================================================

    /**
     * Die Fakten des Items, ueber den EINEN Faktensammler des Core.
     *
     * Bewusst kein zweiter, schlankerer Weg: Zustand, Qualitaet,
     * Vorfahren-Closure und effektive Tags entstehen im ChefZ_FactCollector
     * nach der Projektionsregel aus 06 §3 und der Tag-Zusammenfuehrung aus
     * 05 E4. Sie hier ein zweites Mal auszurechnen hiesse, eine zweite
     * Wahrheit zu fuehren - und die zweite waere die, die ueber Haltbarkeit
     * entscheidet.
     */
    private static bool CollectFacts(notnull ItemBase item, out ChefZ_ItemFacts facts)
    {
        if (!s_Facts)
            s_Facts = new ChefZ_ItemFacts();
        facts = s_Facts;
        return ChefZ_FactCollector.CollectSingle(item, -1, facts);
    }

    /**
     * Die Umgebungstemperatur am Ort des Items.
     *
     * Vanillas eigene Auskunft (WorldData.GetBaseEnvTemperatureAtObject,
     * benutzt zum Beispiel in Apple.EEOnCECreate) - kein Nachbau, keine
     * eigene Wetterrechnung.
     *
     * @return ChefZ_Undefined.FLOAT, wenn sie nicht zu ermitteln ist. Eine
     *         temperaturgebundene Regel greift dann NICHT (02 §8: "jeder
     *         Fehler bewegt das System Richtung weniger ChefZ").
     */
    private static float EnvironmentTemperature(notnull ItemBase item)
    {
        if (!g_Game)
            return ChefZ_Undefined.FLOAT;

        Mission mission = g_Game.GetMission();
        if (!mission)
            return ChefZ_Undefined.FLOAT;

        WorldData world = mission.GetWorldData();
        if (!world)
            return ChefZ_Undefined.FLOAT;

        return world.GetBaseEnvTemperatureAtObject(item);
    }

    private static float FoodDecayModifier()
    {
        DayZGame game = DayZGame.Cast(g_Game);
        if (!game)
            return 1.0;
        return game.GetFoodDecayModifier();
    }

    /**
     * Die Vanilla-Garstufe, ohne Absturzgefahr.
     *
     * ACHTUNG, derselbe Befund wie in ChefZ_ItemStateComponent.GetState:
     * Edible_Base.GetFoodStageType() greift ohne Nullpruefung auf
     * GetFoodStage() durch (Edible_Base.c:531). Eine ChefZ-Klasse ohne
     * FoodStages-Block - erlaubt, siehe 01 V4 - wuerde damit abstuerzen.
     */
    private static int StageOf(ItemBase item)
    {
        Edible_Base edible = Edible_Base.Cast(item);
        if (!edible)
            return ChefZ_VanillaStage.NONE;

        FoodStage stage = edible.GetFoodStage();
        if (!stage)
            return ChefZ_VanillaStage.NONE;

        return stage.GetFoodStageType();
    }

    private static ChefZ_DecayPlan TakePlan()
    {
        if (!s_Plan)
            s_Plan = new ChefZ_DecayPlan();
        s_Plan.Reset();
        return s_Plan;
    }
}
