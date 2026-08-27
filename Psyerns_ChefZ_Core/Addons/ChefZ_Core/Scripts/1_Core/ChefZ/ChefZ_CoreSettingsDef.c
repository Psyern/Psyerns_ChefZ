//==============================================================================
// ChefZ_CoreSettingsDef - die globalen Schalter
//
// Entwurf: 02 §5.4 (Feldliste woertlich), V-B Auflage 2 (defaultExtraItems),
// 18 §6 (Logfelder). Seed: Config/Core.json.
//
// Diese Art ist die einzige, die vollstaendige CODE-Defaults hat. Grund:
// 02 §5.3 verlangt "GetSettings() - nie null, notfalls Code-Defaults". Der
// Config Manager braucht strictMode und safeModeErrorThreshold, BEVOR er
// entscheiden kann, wie er mit einem Fehler umgeht - auch dann, wenn genau die
// Datei fehlt, die diese Werte tragen soll.
//
// Deshalb der Zweischritt:
//   1. Felder starten auf Sentinel (ChefZ_Undefined) - "nicht gesetzt", damit
//      der feldweise Patch aus Rang 3 funktioniert (02 E3).
//   2. ResolveDefaults() ersetzt jeden verbliebenen Sentinel durch den
//      Code-Default aus 02 §5.4.
//
// Ein Wert, den ein Betreiber ausserhalb des Sinnvollen setzt, wird geklammert
// und gemeldet - nicht abgewiesen. Ein Server soll wegen einer 0 in
// matcherNodeBudget nicht ohne Kochsystem dastehen.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_CoreSettingsDef extends ChefZ_Record
{
    //! Die ID, unter der der Core seine Einstellungen sucht. Ein Dokument darf
    //! mehrere Records enthalten; genommen wird dieser, sonst der erste.
    static const string PRIMARY_ID = "CORE";

    //--- Grundschalter (02 §5.4) ---------------------------------------------
    bool  enabled;                  // false => Core inert, reines Vanilla
    bool  strictMode;               // true  => jeder Fehler fuehrt zu SAFE_MODE
    int   safeModeErrorThreshold;

    //--- Log (02 §5.4, 18 §6) ------------------------------------------------
    int   logLevel;
    ref array<string> logChannels;
    bool  logToFile;
    bool  logServerOnly;
    int   logBufferLines;
    int   maxOnceKeys;
    int   maxLogSizeMB;
    bool  logReportToFile;

    //--- Matcher (07, 09) ----------------------------------------------------
    int   matcherNodeBudget;
    float matcherCooldownSec;
    int   matchThrottleTicks;
    int   maxSelectorDepth;
    int   maxCategories;

    //--- Kochadapter (10 §7) -------------------------------------------------
    /**
     * Lebensdauer einer Kochsitzung ohne Kochtick, in Sekunden.
     *
     * 10 §7 nennt die Groesse sessionTtlTicks und begruendet sie: "Ohne das
     * wuechse die Map ueber die Serverlaufzeit unbegrenzt." Die Einheit ist
     * hier die Sekunde und nicht der Tick, weil ein Kochtick kein Serverframe
     * ist - siehe Kopf von ChefZ_CookSession.
     *
     * 0 schaltet die Alterung ab. Das ist ausdruecklich erlaubt (Diagnose,
     * kurze Testlaeufe) und ausdruecklich nichts fuer den Dauerbetrieb.
     */
    float sessionTtlSec;

    //--- Haltbarkeit (14) ----------------------------------------------------
    float globalSpoilageScale;
    float minDecayScale;
    float maxDecayScale;

    /**
     * Servervorgabe fuer die Lebensdauer der Restfrische, in Sekunden. Seit S11.
     *
     * 06 §4.1 nennt fuer ChefZ_StateDef.freshnessLifetimeSec ausdruecklich
     * "Sentinel = aus CoreSettings, siehe 14" - das ist diese Einstellung. Sie
     * steht in 02 §5.4 nicht in der Feldliste, weil dort nur die Schalter
     * stehen, die der Config Manager selbst braucht; die Zeile in 06 §4.1 ist
     * die verbindliche Quelle fuer ihre Existenz.
     *
     * Sie gilt fuer jeden Zustand, der keine eigene Lebensdauer nennt, und
     * damit auch fuer JEDES Item ohne Zustand. Ohne sie waere Freshness01 auf
     * einem Item ohne Zustandsdatensatz eingefroren - und die Frischeregel des
     * Quality Managers (12 §4.1), die den wichtigsten Exploit schliesst,
     * liefe dort ins Leere.
     *
     * Ein Wert <= 0 ist erlaubt und heisst "Frische serverweit einfrieren"
     * (14 §8, Zeile freshnessLifetimeSec <= 0). Er wird deshalb NICHT
     * geklammert - aber der ChefZ_PreservationManager meldet ihn beim Boot,
     * damit ein Tippfehler nicht als Entwurfsentscheidung durchgeht.
     */
    float defaultFreshnessLifetimeSec;

    //--- Rezepte und Qualitaet (08, 09, 12, 17) ------------------------------
    float priorityScale;
    float maxExternalQualityBonus;
    string capabilityMode;                      // asAuthored | neverBlock | ignore
    string defaultExtraItems;                   // forbid | ignore | consume  (V-B Auflage 2)
    ref array<string> defaultExcludedStates;

    /**
     * Die Spezifitaetsgewichte (09 §3: "aus Core.json, nicht aus dem Code").
     * Seit S6.
     *
     * null heisst "kein Block geschrieben" - dann gelten die Code-Defaults aus
     * ChefZ_PriorityWeights vollstaendig (09 §7). Ein TEILWEISE gefuellter
     * Block wirkt ebenso teilweise; dafuer traegt ChefZ_PriorityWeightsDef
     * seine eigenen Sentinel.
     *
     * priorityScale steht doppelt: flach in diesen Einstellungen (02 §5.4) und
     * im Gewichtsblock (09 §3). Beides ist Entwurf, und beides bleibt. Die
     * Aufloesung ist die naheliegende: der flache Wert ist die Vorgabe, ein
     * Wert im Block ueberschreibt ihn - das Speziellere gewinnt. Wer nur eine
     * der beiden Stellen kennt, bekommt trotzdem, was er erwartet.
     */
    ref ChefZ_PriorityWeightsDef priorityWeights;

    /**
     * Die Stellschrauben der Qualitaetsrechnung (12 §4). Seit S10.
     *
     * null heisst "kein Block geschrieben" - dann gelten die Code-Defaults aus
     * ChefZ_QualityScoring vollstaendig. Ein TEILWEISE gefuellter Block wirkt
     * ebenso teilweise; dafuer traegt ChefZ_QualityScoringDef seine eigenen
     * Sentinel.
     *
     * Er steht hier und nicht bei den Qualitaetsstufen, weil er ein
     * SERVERweiter Regler ist: die Stufen sind Content und kommen aus Rang 1,
     * die Gewichte der Rechnung sind Balancing und muessen von einem Betreiber
     * ohne Content-Aenderung verstellbar sein.
     */
    ref ChefZ_QualityScoringDef qualityScoring;

    //--- Naehrwertaudit (13 §5, 13 §8) - seit S12 ----------------------------
    //
    // Vier Regler fuer ein Teilsystem, das zur LAUFZEIT nichts tut (13 E2).
    // Sie steuern ausschliesslich, was beim Serverstart im Log steht - und
    // genau deshalb gehoeren sie in Core.json und nicht in den Code: der
    // Adressat ist der Betreiber, nicht der Content-Autor.

    /**
     * 13 §8, letzte Zeile: "EnableNutritionAudit = 0 -> Audit entfaellt
     * vollstaendig. Fuer Produktivserver, die den Startlog kurz halten
     * wollen."
     *
     * Vorgabe true, und das ist die richtige Vorgabe: der Audit ist die
     * einzige Stelle, an der ein Gericht ohne Nutrition-Block ueberhaupt
     * auffaellt (01 V7). Wer ihn abschaltet, soll das tun, weil er ihn einmal
     * gelesen hat - nicht, weil er ihn nie gesehen hat.
     */
    bool  enableNutritionAudit;

    /**
     * Ab welcher prozentualen Abweichung zwischen Soll und Ist der Audit ein
     * WARN schreibt (13 §8, "Abweichung ueber Toleranz -> WARN mit
     * Prozentangabe, KEINE Korrektur").
     *
     * 25 Prozent als Vorgabe: 13 §5 zeigt als Beispielbefund -31 Prozent und
     * behandelt ihn als meldenswert. Enger gesetzt, meldete der Audit jede
     * bewusste Designentscheidung; weiter gesetzt, verschwiege er sie.
     *
     * Ein Wert <= 0 hiesse "melde jede Abweichung" und ist ausdruecklich
     * erlaubt - fuer den Balance-Reviewer, der einmal ALLE Zahlen sehen will.
     */
    float nutritionTolerancePct;

    /**
     * Wieviele Befunde der Audit ins Log schreibt, bevor er zusammenfasst.
     *
     * Der Deckel ist keine Sparmassnahme, sondern eine Lesbarkeitsgrenze: ein
     * Content-Modul mit einem systematischen Fehler erzeugt sonst hundert
     * gleichlautende Zeilen, und die eine ANDERE Zeile geht darin unter. Die
     * Gesamtzahl steht danach in der Abschlusszeile - es geht nichts
     * verloren, es steht nur nicht alles einzeln da.
     */
    int   nutritionAuditMaxFindings;

    /**
     * Sondengrenze der Sollrechnung (13 §8, Deckelzeile).
     *
     * KEIN Balancingdeckel. 13 E6 streicht das NutritionCap-System aus V1
     * ausdruecklich, und zwar mit gutem Grund: der Sollwert wird nie
     * angewandt, also gibt es nichts zu deckeln. Diese Zahl faengt allein die
     * Groessenordnung ab, in der eine Sollrechnung offensichtlich entgleist
     * ist - damit im Log eine Zahl steht und keine Zahlenkolonne. Ein Treffer
     * ist ein INFO, kein Fehler.
     */
    float nutritionExpectedCap;

    //--- Faehigkeiten und Ereignisse (17 §3.3, §9) - seit S13 -------------
    //
    // Sechs Regler fuer eine Schicht, die auf einem Server ohne Comp-Module
    // nichts tut. Sie stehen trotzdem in Core.json und nicht im Code, weil
    // ihr Adressat der Betreiber ist: er entscheidet, wie hart
    // Faehigkeitsanforderungen wirken und wieviel Nachsicht er einem fremden
    // Abonnenten entgegenbringt.

    /**
     * Was eine Faehigkeit wert ist, solange kein Anbieter antwortet
     * (17 §3.3: "Ohne Provider: Default aus ChefZ_CoreSettingsDef. Nie
     * Fehler.").
     *
     * Vorgabe 0.0, und das ist die ehrliche Vorgabe: ohne Skillmodul hat
     * niemand eine Faehigkeit. Zusammen mit onFail "degrade" (dem Default aus
     * 17 §3.3) heisst das "das Gericht entsteht, nur eine Stufe schlechter" -
     * der Server ist voll spielbar (12 E8).
     *
     * Ein Betreiber, der Faehigkeitsrezepte OHNE Skillmodul in voller
     * Qualitaet will, setzt diesen Wert hoch oder capabilityMode auf
     * "ignore". Beides ist eine bewusste Entscheidung und keine Nebenwirkung.
     *
     * Bewusst NICHT wirksam fuer Qualitaetsregeln mit when: "capability":
     * die gelten ohne Anbieter als nicht erfuellt (12 §8). Sonst bekaeme jeder
     * Spieler auf jedem Server ohne Skillmod denselben Bonus - eine
     * Balancingaussage, die niemand getroffen hat.
     */
    float defaultCapabilityValue;

    /**
     * Der gueltige Wertebereich einer Faehigkeit (17 §9: "Provider liefert
     * NaN, negativ oder unsinnig -> auf den Config-Bereich geklemmt").
     *
     * 0 bis 10 als Vorgabe. Die Zahlen sind bewusst grob: der Core weiss
     * nicht, welche Skala ein fremdes Skillsystem benutzt, und ein zu enger
     * Bereich klemmte gueltige Werte weg. Er dient nur dazu, dass ein
     * Anbieter mit einem Rechenfehler nicht jede Anforderung auf einen Schlag
     * erfuellt oder sperrt.
     */
    float capabilityMin;
    float capabilityMax;

    /**
     * Wie tief sich Ereignisse verschachteln duerfen (17 §9: "Abonnent feuert
     * im Callback ein Event -> erlaubt bis Tiefe 3, danach abgebrochen mit
     * ERROR").
     *
     * Der Deckel verhindert eine Endlosschleife zwischen zwei Modulen, die
     * sich gegenseitig Ereignisse zuwerfen. Er steht in der Config, weil ein
     * Betreiber mit einer ungewoehnlichen Modkombination ihn hochsetzen
     * koennen soll - nicht, weil 3 zu wenig waere.
     */
    int   eventMaxDepth;

    /**
     * Misst der Bus die Dauer je Abonnent? (17 §9: "Der Core misst OPTIONAL
     * die Dauer je Abonnent und meldet Ausreisser als WARN".)
     *
     * Vorgabe aus, weil die Messung zwei Zeitabfragen je Abonnent und
     * Ereignis kostet und die Antwort auf einem gesunden Server immer
     * dieselbe ist. Wer einen haengenden Mod sucht, schaltet sie ein.
     */
    bool  eventTiming;

    //! Ab wievielen Millisekunden ein Abonnent als Ausreisser gilt. Wirkt nur
    //! bei eventTiming = true.
    int   eventSlowSubscriberMs;

    /**
     * Vorgabedauer der Entnahmeaktion in Sekunden (15 §3, takeDurationSec).
     *
     * Sie gilt fuer jedes Portionsgericht, das selbst nichts sagt. In der
     * Config und nicht als Konstante, weil sie eine reine Spielgefuehlsfrage
     * ist: 15 E7 nennt ausdruecklich den Fall, dass der zusaetzliche
     * Interaktionsschritt beim Tellergericht stoert, und die Antwort darauf
     * ist eine Zahl - keine Codeaenderung.
     *
     * 2.0 als Vorgabe: lang genug, dass der Fortschrittsbalken sichtbar ist,
     * kurz genug, dass acht Portionen aus einem Kessel keine Geduldsprobe
     * werden. Ein Rezept, das es anders will, schreibt takeDurationSec.
     */
    float defaultTakePortionSec;

    /**
     * Umkreis der Behaeltersuche in der UMGEBUNG, in Metern (16 E5).
     *
     * Sie wirkt ausschliesslich auf die dritte Suchstufe (NEARBY_CARGO), und
     * die ist per Vorgabe ABGESCHALTET - ein Behaelter wird nur dann in
     * Kisten gesucht, wenn seine Deklaration searchScope 4 mitfuehrt. Auf
     * einem Server ohne solche Behaelter ist diese Zahl folgenlos.
     *
     * In der Config und nicht als Konstante, weil "in Reichweite" eine
     * Spielgefuehlsfrage ist: 16 E5 nennt ausdruecklich den Zielkonflikt
     * zwischen Bequemlichkeit und Nachvollziehbarkeit ("macht die Auswahl fuer
     * den Spieler undurchsichtig"), und die Antwort darauf ist eine Zahl.
     *
     * 3.0 als Vorgabe: etwa Armlaenge plus einen Schritt - der Kessel und die
     * Kiste daneben, nicht das halbe Lager. 0 schaltet die Stufe ab, ohne
     * eine einzige Deklaration anzufassen.
     */
    float containerSearchRadius;

    /**
     * Hoechstzahl gefundener Behaelter je Suche.
     *
     * Ein reiner Schutzdeckel. Gebraucht wird ohnehin nur der erste Eintrag
     * (16 §5: der Sucher ordnet, der Manager nimmt den Ersten) - der Deckel
     * begrenzt, was eine Basis mit hundert Tellern im ActionCondition-Pfad
     * kostet, und der laeuft bei JEDEM Zielwechsel des Fadenkreuzes.
     *
     * Er kann das ERGEBNIS nicht verfaelschen, solange er >= 1 ist: die
     * Fundstufen werden in ihrer festen Reihenfolge abgearbeitet, die beste
     * Wahl steht also vor dem Deckel und nicht dahinter.
     */
    int   maxContainerCandidates;

    //--- Quellen -------------------------------------------------------------
    bool  allowProfileOverlay;
    bool  allowTimedRecipes;

    //--------------------------------------------------------------------------

    void ChefZ_CoreSettingsDef()
    {
        // Alles auf "nicht gesetzt". Die echten Werte setzt ResolveDefaults().
        safeModeErrorThreshold  = ChefZ_Undefined.INT;
        logLevel                = ChefZ_Undefined.INT;
        logBufferLines          = ChefZ_Undefined.INT;
        maxOnceKeys             = ChefZ_Undefined.INT;
        maxLogSizeMB            = ChefZ_Undefined.INT;
        matcherNodeBudget       = ChefZ_Undefined.INT;
        matchThrottleTicks      = ChefZ_Undefined.INT;
        maxSelectorDepth        = ChefZ_Undefined.INT;
        maxCategories           = ChefZ_Undefined.INT;
        sessionTtlSec           = ChefZ_Undefined.FLOAT;

        nutritionAuditMaxFindings = ChefZ_Undefined.INT;
        nutritionTolerancePct     = ChefZ_Undefined.FLOAT;
        nutritionExpectedCap      = ChefZ_Undefined.FLOAT;

        matcherCooldownSec      = ChefZ_Undefined.FLOAT;
        globalSpoilageScale     = ChefZ_Undefined.FLOAT;
        minDecayScale           = ChefZ_Undefined.FLOAT;
        maxDecayScale           = ChefZ_Undefined.FLOAT;
        defaultFreshnessLifetimeSec = ChefZ_Undefined.FLOAT;
        priorityScale           = ChefZ_Undefined.FLOAT;
        maxExternalQualityBonus = ChefZ_Undefined.FLOAT;

        defaultTakePortionSec   = ChefZ_Undefined.FLOAT;
        containerSearchRadius   = ChefZ_Undefined.FLOAT;
        maxContainerCandidates  = ChefZ_Undefined.INT;
        defaultCapabilityValue  = ChefZ_Undefined.FLOAT;
        capabilityMin           = ChefZ_Undefined.FLOAT;
        capabilityMax           = ChefZ_Undefined.FLOAT;
        eventMaxDepth           = ChefZ_Undefined.INT;
        eventSlowSubscriberMs   = ChefZ_Undefined.INT;

        capabilityMode          = ChefZ_Undefined.TEXT;
        defaultExtraItems       = ChefZ_Undefined.TEXT;

        logChannels             = null;
        defaultExcludedStates   = null;
        priorityWeights         = null;
        qualityScoring          = null;

        // bool: kein Sentinel moeglich, deshalb Bool-Sonde (siehe ChefZ_Record).
        enabled             = ChefZ_RecordProbe.Bool();
        strictMode          = ChefZ_RecordProbe.Bool();
        logToFile           = ChefZ_RecordProbe.Bool();
        logServerOnly       = ChefZ_RecordProbe.Bool();
        logReportToFile     = ChefZ_RecordProbe.Bool();
        allowProfileOverlay = ChefZ_RecordProbe.Bool();
        allowTimedRecipes   = ChefZ_RecordProbe.Bool();
        enableNutritionAudit = ChefZ_RecordProbe.Bool();
        eventTiming          = ChefZ_RecordProbe.Bool();
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.CORE_SETTINGS;
    }

    override void Normalize()
    {
        super.Normalize();
        capabilityMode.TrimInPlace();
        defaultExtraItems.TrimInPlace();
        if (qualityScoring)
            qualityScoring.Normalize();
    }

    //--------------------------------------------------------------------------

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_CoreSettingsDef s = ChefZ_CoreSettingsDef.Cast(src);
        if (!s)
            return;

        safeModeErrorThreshold  = PatchInt(safeModeErrorThreshold, s.safeModeErrorThreshold, s, "safeModeErrorThreshold");
        logLevel                = PatchInt(logLevel, s.logLevel, s, "logLevel");
        logBufferLines          = PatchInt(logBufferLines, s.logBufferLines, s, "logBufferLines");
        maxOnceKeys             = PatchInt(maxOnceKeys, s.maxOnceKeys, s, "maxOnceKeys");
        maxLogSizeMB            = PatchInt(maxLogSizeMB, s.maxLogSizeMB, s, "maxLogSizeMB");
        matcherNodeBudget       = PatchInt(matcherNodeBudget, s.matcherNodeBudget, s, "matcherNodeBudget");
        matchThrottleTicks      = PatchInt(matchThrottleTicks, s.matchThrottleTicks, s, "matchThrottleTicks");
        maxSelectorDepth        = PatchInt(maxSelectorDepth, s.maxSelectorDepth, s, "maxSelectorDepth");
        maxCategories           = PatchInt(maxCategories, s.maxCategories, s, "maxCategories");
        sessionTtlSec           = PatchFloat(sessionTtlSec, s.sessionTtlSec, s, "sessionTtlSec");

        nutritionAuditMaxFindings = PatchInt(nutritionAuditMaxFindings, s.nutritionAuditMaxFindings, s, "nutritionAuditMaxFindings");
        nutritionTolerancePct     = PatchFloat(nutritionTolerancePct, s.nutritionTolerancePct, s, "nutritionTolerancePct");
        nutritionExpectedCap      = PatchFloat(nutritionExpectedCap, s.nutritionExpectedCap, s, "nutritionExpectedCap");

        matcherCooldownSec      = PatchFloat(matcherCooldownSec, s.matcherCooldownSec, s, "matcherCooldownSec");
        globalSpoilageScale     = PatchFloat(globalSpoilageScale, s.globalSpoilageScale, s, "globalSpoilageScale");
        minDecayScale           = PatchFloat(minDecayScale, s.minDecayScale, s, "minDecayScale");
        maxDecayScale           = PatchFloat(maxDecayScale, s.maxDecayScale, s, "maxDecayScale");
        defaultFreshnessLifetimeSec = PatchFloat(defaultFreshnessLifetimeSec, s.defaultFreshnessLifetimeSec, s, "defaultFreshnessLifetimeSec");
        priorityScale           = PatchFloat(priorityScale, s.priorityScale, s, "priorityScale");
        maxExternalQualityBonus = PatchFloat(maxExternalQualityBonus, s.maxExternalQualityBonus, s, "maxExternalQualityBonus");

        defaultTakePortionSec   = PatchFloat(defaultTakePortionSec, s.defaultTakePortionSec, s, "defaultTakePortionSec");
        containerSearchRadius   = PatchFloat(containerSearchRadius, s.containerSearchRadius, s, "containerSearchRadius");
        maxContainerCandidates  = PatchInt(maxContainerCandidates, s.maxContainerCandidates, s, "maxContainerCandidates");
        defaultCapabilityValue  = PatchFloat(defaultCapabilityValue, s.defaultCapabilityValue, s, "defaultCapabilityValue");
        capabilityMin           = PatchFloat(capabilityMin, s.capabilityMin, s, "capabilityMin");
        capabilityMax           = PatchFloat(capabilityMax, s.capabilityMax, s, "capabilityMax");
        eventMaxDepth           = PatchInt(eventMaxDepth, s.eventMaxDepth, s, "eventMaxDepth");
        eventSlowSubscriberMs   = PatchInt(eventSlowSubscriberMs, s.eventSlowSubscriberMs, s, "eventSlowSubscriberMs");

        capabilityMode          = PatchText(capabilityMode, s.capabilityMode, s, "capabilityMode");
        defaultExtraItems       = PatchText(defaultExtraItems, s.defaultExtraItems, s, "defaultExtraItems");

        logChannels             = PatchStringArray(logChannels, s.logChannels);
        defaultExcludedStates   = PatchStringArray(defaultExcludedStates, s.defaultExcludedStates);

        // Ganzersatz, nicht feldweise: ein Overlay, das den Gewichtsblock
        // schreibt, meint diesen Block - und ein halb aus zwei Dateien
        // zusammengesetzter Gewichtssatz waere eine Ordnung, die niemand
        // aufgeschrieben hat. Innerhalb des Blocks wirkt der feldweise Patch
        // weiterhin: was der Betreiber nicht nennt, behaelt seinen
        // Code-Default (ChefZ_PriorityWeightsDef).
        if (s.priorityWeights)
            priorityWeights = s.priorityWeights;

        // Dasselbe fuer die Qualitaetsrechnung und aus demselben Grund.
        if (s.qualityScoring)
            qualityScoring = s.qualityScoring;

        enabled             = PatchBool(enabled, s.enabled, s, "enabled");
        strictMode          = PatchBool(strictMode, s.strictMode, s, "strictMode");
        logToFile           = PatchBool(logToFile, s.logToFile, s, "logToFile");
        logServerOnly       = PatchBool(logServerOnly, s.logServerOnly, s, "logServerOnly");
        logReportToFile     = PatchBool(logReportToFile, s.logReportToFile, s, "logReportToFile");
        allowProfileOverlay = PatchBool(allowProfileOverlay, s.allowProfileOverlay, s, "allowProfileOverlay");
        allowTimedRecipes   = PatchBool(allowTimedRecipes, s.allowTimedRecipes, s, "allowTimedRecipes");
        enableNutritionAudit = PatchBool(enableNutritionAudit, s.enableNutritionAudit, s, "enableNutritionAudit");
        eventTiming          = PatchBool(eventTiming, s.eventTiming, s, "eventTiming");
    }

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_CoreSettingsDef o = ChefZ_CoreSettingsDef.Cast(other);
        if (!o)
            return;

        if (enabled             == o.enabled)             MarkExplicit("enabled");
        if (strictMode          == o.strictMode)          MarkExplicit("strictMode");
        if (logToFile           == o.logToFile)           MarkExplicit("logToFile");
        if (logServerOnly       == o.logServerOnly)       MarkExplicit("logServerOnly");
        if (logReportToFile     == o.logReportToFile)     MarkExplicit("logReportToFile");
        if (allowProfileOverlay == o.allowProfileOverlay) MarkExplicit("allowProfileOverlay");
        if (allowTimedRecipes   == o.allowTimedRecipes)   MarkExplicit("allowTimedRecipes");
        if (enableNutritionAudit == o.enableNutritionAudit) MarkExplicit("enableNutritionAudit");
        if (eventTiming         == o.eventTiming)         MarkExplicit("eventTiming");
    }

    //--------------------------------------------------------------------------

    /**
     * Code-Defaults woertlich aus 02 §5.4 plus V-B Auflage 2.
     *
     * Wichtig fuer bool: ein Feld, das NICHT in explicitFields steht, war im
     * JSON nicht vorhanden - es bekommt hier seinen Code-Default und nicht den
     * Wert, den die Sonde zufaellig hinterlassen hat.
     */
    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        if (id == "")
            id = PRIMARY_ID;

        safeModeErrorThreshold  = ChefZ_Undefined.IntOr(safeModeErrorThreshold, 25);
        logLevel                = ChefZ_Undefined.IntOr(logLevel, 2);
        logBufferLines          = ChefZ_Undefined.IntOr(logBufferLines, 64);
        maxOnceKeys             = ChefZ_Undefined.IntOr(maxOnceKeys, 512);
        maxLogSizeMB            = ChefZ_Undefined.IntOr(maxLogSizeMB, 8);
        matcherNodeBudget       = ChefZ_Undefined.IntOr(matcherNodeBudget, 4096);
        matchThrottleTicks      = ChefZ_Undefined.IntOr(matchThrottleTicks, 2);
        maxSelectorDepth        = ChefZ_Undefined.IntOr(maxSelectorDepth, 8);
        maxCategories           = ChefZ_Undefined.IntOr(maxCategories, 256);

        // 300 Sekunden: lang genug, dass ein Spieler zwischen zwei Handgriffen
        // am Topf keine Sitzung verliert, kurz genug, dass eine abgebrannte
        // Feuerstelle nicht bis zum Serverneustart Speicher belegt.
        sessionTtlSec           = ChefZ_Undefined.FloatOr(sessionTtlSec, 300.0);

        matcherCooldownSec      = ChefZ_Undefined.FloatOr(matcherCooldownSec, 1.0);
        globalSpoilageScale     = ChefZ_Undefined.FloatOr(globalSpoilageScale, 1.0);
        minDecayScale           = ChefZ_Undefined.FloatOr(minDecayScale, 0.01);
        maxDecayScale           = ChefZ_Undefined.FloatOr(maxDecayScale, 10.0);

        // 21600 Sekunden = 6 Stunden. Die Zahl ist nicht geraten, sondern
        // GameConstants.DECAY_FOOD_RAW_MEAT (3_Game/DayZ/constants.c:1037) -
        // Vanillas KUERZESTE Haltbarkeit. Ein Item ohne eigene Angabe verliert
        // seine Frische damit ungefaehr im selben Takt, in dem rohes Fleisch
        // verdirbt: die Frische laeuft dem Verfall weder davon, noch bleibt sie
        // sinnlos lange stehen.
        //
        // Die Konstante wird bewusst als Literal geschrieben und nicht aus
        // GameConstants gelesen: GameConstants lebt in 3_Game, dieser Record in
        // 1_Core (00 §4). Der Wert ist eine Vorgabe, kein Vanilla-Vertrag -
        // aendert DayZ ihn, bleibt diese Vorgabe trotzdem sinnvoll.
        defaultFreshnessLifetimeSec = ChefZ_Undefined.FloatOr(defaultFreshnessLifetimeSec, 21600.0);

        priorityScale           = ChefZ_Undefined.FloatOr(priorityScale, 0.01);
        maxExternalQualityBonus = ChefZ_Undefined.FloatOr(maxExternalQualityBonus, 2.0);

        // 13 §8 und 13 §5. Die Begruendung jeder einzelnen Zahl steht am Feld.
        nutritionAuditMaxFindings = ChefZ_Undefined.IntOr(nutritionAuditMaxFindings, 64);
        nutritionTolerancePct     = ChefZ_Undefined.FloatOr(nutritionTolerancePct, 25.0);
        nutritionExpectedCap      = ChefZ_Undefined.FloatOr(nutritionExpectedCap, 100000.0);

        // 15 §3. Die Begruendung der Zahl steht am Feld.
        defaultTakePortionSec   = ChefZ_Undefined.FloatOr(defaultTakePortionSec,
                                        ChefZ_PortionLimits.DEFAULT_TAKE_SEC);

        // 16 E5. Die Begruendung beider Zahlen steht am Feld.
        containerSearchRadius   = ChefZ_Undefined.FloatOr(containerSearchRadius, 3.0);
        maxContainerCandidates  = ChefZ_Undefined.IntOr(maxContainerCandidates, 32);

        // 17 §3.3 und 17 §9. Die Begruendung jeder einzelnen Zahl steht am Feld.
        defaultCapabilityValue  = ChefZ_Undefined.FloatOr(defaultCapabilityValue, 0.0);
        capabilityMin           = ChefZ_Undefined.FloatOr(capabilityMin, 0.0);
        capabilityMax           = ChefZ_Undefined.FloatOr(capabilityMax, 10.0);
        eventMaxDepth           = ChefZ_Undefined.IntOr(eventMaxDepth, 3);
        eventSlowSubscriberMs   = ChefZ_Undefined.IntOr(eventSlowSubscriberMs, 5);

        capabilityMode          = ChefZ_Undefined.TextOr(capabilityMode, "asAuthored");
        defaultExtraItems       = ChefZ_Undefined.TextOr(defaultExtraItems, "forbid");

        if (!logChannels)
        {
            logChannels = new array<string>();
            logChannels.Insert("ALL");
        }
        if (!defaultExcludedStates)
        {
            // 02 §5.4 nennt diesen Seed ausdruecklich. Es ist KEIN Content:
            // die beiden IDs sind Zustaende, die der Core selbst nie anlegt -
            // steht kein Content dahinter, laufen sie ins Leere.
            defaultExcludedStates = new array<string>();
            defaultExcludedStates.Insert("BURNT");
            defaultExcludedStates.Insert("ROTTEN");
        }

        ResolveBool("enabled",             true);
        ResolveBool("strictMode",          false);
        ResolveBool("logToFile",           false);
        ResolveBool("logServerOnly",       true);
        ResolveBool("logReportToFile",     false);
        ResolveBool("allowProfileOverlay", true);
        ResolveBool("allowTimedRecipes",   true);
        ResolveBool("enableNutritionAudit", true);
    }

    /**
     * Ein bool ohne Eintrag in explicitFields[] gilt als nicht gesetzt und
     * bekommt seinen Code-Default; ein gesetztes bleibt, wie es ist.
     *
     * Umweg ueber einen Namensvergleich statt out-Parameter, weil Enforce
     * keine Referenz auf ein Feld kennt. Sieben Felder, einmal beim Boot -
     * die Lesbarkeit ist den Zweig wert.
     */
    private void ResolveBool(string field, bool codeDefault)
    {
        if (HasExplicit(field))
            return;
        SetBool(field, codeDefault);
    }

    private void SetBool(string field, bool value)
    {
        if (field == "enabled")             { enabled = value;             return; }
        if (field == "strictMode")          { strictMode = value;          return; }
        if (field == "logToFile")           { logToFile = value;           return; }
        if (field == "logServerOnly")       { logServerOnly = value;       return; }
        if (field == "logReportToFile")     { logReportToFile = value;     return; }
        if (field == "allowProfileOverlay") { allowProfileOverlay = value; return; }
        if (field == "allowTimedRecipes")   { allowTimedRecipes = value;   return; }
        if (field == "enableNutritionAudit") { enableNutritionAudit = value; return; }
    }

    //--------------------------------------------------------------------------

    /**
     * Klammert unsinnige Werte und meldet jede Klammerung.
     *
     * Bewusst NACH ResolveDefaults und bewusst ohne Abweisung: 02 §8 -
     * "jeder Fehler bewegt das System Richtung weniger ChefZ, nie Richtung
     * falsches ChefZ". Ein geklammerter Schalter ist weniger falsch als ein
     * verworfener Einstellungssatz.
     */
    void ClampAndReport(ChefZ_LoadReport report)
    {
        if (safeModeErrorThreshold < 1)
            safeModeErrorThreshold = ClampReportInt(report, "safeModeErrorThreshold", safeModeErrorThreshold, 1);
        if (!ChefZ_LogLevel.IsValid(logLevel))
        {
            int fixedLevel = ChefZ_LogLevel.Clamp(logLevel);
            Note(report, "logLevel " + logLevel.ToString() + " liegt ausserhalb von 0..5 - benutzt wird "
                + ChefZ_LogLevel.Name(fixedLevel) + ". Gueltig: " + ChefZ_LogLevel.ValidNames());
            logLevel = fixedLevel;
        }
        if (logBufferLines < 1)
            logBufferLines = ClampReportInt(report, "logBufferLines", logBufferLines, 1);
        if (maxOnceKeys < 1)
            maxOnceKeys = ClampReportInt(report, "maxOnceKeys", maxOnceKeys, 1);
        if (maxLogSizeMB < 1)
            maxLogSizeMB = ClampReportInt(report, "maxLogSizeMB", maxLogSizeMB, 1);
        if (matcherNodeBudget < 1)
            matcherNodeBudget = ClampReportInt(report, "matcherNodeBudget", matcherNodeBudget, 1);
        if (matchThrottleTicks < 0)
            matchThrottleTicks = ClampReportInt(report, "matchThrottleTicks", matchThrottleTicks, 0);
        if (maxSelectorDepth < 1)
            maxSelectorDepth = ClampReportInt(report, "maxSelectorDepth", maxSelectorDepth, 1);
        if (maxCategories < 1)
            maxCategories = ClampReportInt(report, "maxCategories", maxCategories, 1);

        if (matcherCooldownSec < 0.0)
            matcherCooldownSec = ClampReportFloat(report, "matcherCooldownSec", matcherCooldownSec, 0.0);
        if (sessionTtlSec < 0.0)
            sessionTtlSec = ClampReportFloat(report, "sessionTtlSec", sessionTtlSec, 0.0);
        if (globalSpoilageScale < 0.0)
            globalSpoilageScale = ClampReportFloat(report, "globalSpoilageScale", globalSpoilageScale, 0.0);
        if (minDecayScale <= 0.0)
            minDecayScale = ClampReportFloat(report, "minDecayScale", minDecayScale, 0.01);
        if (maxDecayScale < minDecayScale)
            maxDecayScale = ClampReportFloat(report, "maxDecayScale", maxDecayScale, minDecayScale);
        if (priorityScale < 0.0)
            priorityScale = ClampReportFloat(report, "priorityScale", priorityScale, 0.0);
        if (maxExternalQualityBonus < 0.0)
            maxExternalQualityBonus = ClampReportFloat(report, "maxExternalQualityBonus", maxExternalQualityBonus, 0.0);

        // 15 E7 laesst 0 ausdruecklich zu ("fast unsichtbar"), negativ ergibt
        // keinen Fortschrittsbalken. Die Untergrenze der TATSAECHLICHEN Dauer
        // zieht ChefZ_PortionSpec.EffectiveTakeSeconds().
        if (defaultTakePortionSec < 0.0)
            defaultTakePortionSec = ClampReportFloat(report, "defaultTakePortionSec", defaultTakePortionSec, 0.0);

        // 16 E5: 0 ist ausdruecklich zulaessig und heisst "keine
        // Umgebungssuche" - ein negativer Radius dagegen ist keine
        // Einstellung, sondern ein Tippfehler.
        if (containerSearchRadius < 0.0)
            containerSearchRadius = ClampReportFloat(report, "containerSearchRadius", containerSearchRadius, 0.0);

        // Unter 1 waere der Deckel kein Schutz mehr, sondern ein Verbot:
        // gefunden wuerde nie etwas, und jede Behaelterbedingung schluege
        // fehl, ohne dass irgendwo eine Zeile im Log stuende.
        if (maxContainerCandidates < 1)
            maxContainerCandidates = ClampReportInt(report, "maxContainerCandidates", maxContainerCandidates, 1);

        // 13 §8: der Audit ist ein Berichtswerkzeug. Ein unbrauchbarer Regler
        // darf ihn deshalb nie abschalten - er wird geklammert und gemeldet,
        // und der Audit laeuft.
        //
        // nutritionTolerancePct wird BEWUSST NICHT nach unten geklammert:
        // 0 heisst "melde jede Abweichung", und das ist eine gueltige
        // Einstellung fuer den Balance-Reviewer. Nur negativ ist es keine.
        if (nutritionTolerancePct < 0.0)
            nutritionTolerancePct = ClampReportFloat(report, "nutritionTolerancePct", nutritionTolerancePct, 0.0);
        if (nutritionAuditMaxFindings < 1)
            nutritionAuditMaxFindings = ClampReportInt(report, "nutritionAuditMaxFindings", nutritionAuditMaxFindings, 1);
        if (nutritionExpectedCap <= 0.0)
            nutritionExpectedCap = ClampReportFloat(report, "nutritionExpectedCap", nutritionExpectedCap, 100000.0);

        // 17 §9. Ein unbrauchbarer Regler darf die Ereignisschicht nie
        // abschalten: eine Tiefe von 0 hiesse "gar keine Zustellung", und ein
        // Comp-Modul bekaeme lautlos nie ein Ereignis.
        if (eventMaxDepth < 1)
            eventMaxDepth = ClampReportInt(report, "eventMaxDepth", eventMaxDepth, 1);
        if (eventSlowSubscriberMs < 1)
            eventSlowSubscriberMs = ClampReportInt(report, "eventSlowSubscriberMs", eventSlowSubscriberMs, 1);

        if (capabilityMax < capabilityMin)
            capabilityMax = ClampReportFloat(report, "capabilityMax", capabilityMax, capabilityMin);
        if (defaultCapabilityValue < capabilityMin)
            defaultCapabilityValue = ClampReportFloat(report, "defaultCapabilityValue", defaultCapabilityValue, capabilityMin);
        if (defaultCapabilityValue > capabilityMax)
            defaultCapabilityValue = ClampReportFloat(report, "defaultCapabilityValue", defaultCapabilityValue, capabilityMax);

        if (!IsKnownCapabilityMode(capabilityMode))
        {
            Note(report, "capabilityMode \"" + capabilityMode + "\" ist unbekannt - benutzt wird \"asAuthored\". "
                + "Gueltig: asAuthored, neverBlock, ignore.");
            capabilityMode = "asAuthored";
        }
        if (!IsKnownExtraItemsMode(defaultExtraItems))
        {
            Note(report, "defaultExtraItems \"" + defaultExtraItems + "\" ist unbekannt - benutzt wird \"forbid\". "
                + "Gueltig: forbid, ignore, consume.");
            defaultExtraItems = "forbid";
        }
    }

    static bool IsKnownCapabilityMode(string mode)
    {
        return mode == "asAuthored" || mode == "neverBlock" || mode == "ignore";
    }

    static bool IsKnownExtraItemsMode(string mode)
    {
        return mode == "forbid" || mode == "ignore" || mode == "consume";
    }

    private int ClampReportInt(ChefZ_LoadReport report, string field, int was, int now)
    {
        Note(report, field + " = " + was.ToString() + " ist unbrauchbar - geklammert auf " + now.ToString() + ".");
        return now;
    }

    private float ClampReportFloat(ChefZ_LoadReport report, string field, float was, float now)
    {
        Note(report, field + " = " + was.ToString() + " ist unbrauchbar - geklammert auf " + now.ToString() + ".");
        return now;
    }

    private void Note(ChefZ_LoadReport report, string msg)
    {
        if (report)
            report.AddWarn(sourceRef, id, msg);
    }

    //--------------------------------------------------------------------------

    /**
     * Der fertige Gewichtssatz fuer die Spezifitaetsrechnung (09 §3, S6).
     *
     * Reihenfolge, und jede Stufe ist begruendet:
     *
     *   1. Code-Defaults aus ChefZ_PriorityWeights - 09 §7 verlangt, dass die
     *      Ordnung OHNE jede Konfiguration sinnvoll ist.
     *   2. priorityScale aus den flachen Einstellungen (02 §5.4) - damit die
     *      beiden Stellen nicht auseinanderlaufen.
     *   3. der Block "priorityWeights" aus Core.json (09 §3), feldweise.
     *   4. Klammerung: negative Gewichte wuerden die Ordnung umdrehen.
     *
     * Nie null. Zwei Zustaende bekommen ein WARN, weil sie die Grundregel aus
     * Architekturplan §16 faktisch abschalten und fast nie Absicht sind
     * (09 §7):
     *
     *   priorityScale > 1.0   die Handzahl ueberstimmt die Spezifitaet
     *   alle Gewichte 0       es entscheidet nur noch der Tiebreak
     */
    ChefZ_PriorityWeights BuildPriorityWeights(ChefZ_LoadReport report)
    {
        ChefZ_PriorityWeights w = new ChefZ_PriorityWeights();

        if (!ChefZ_Undefined.IsFloatUndefined(priorityScale))
            w.priorityScale = priorityScale;

        int taken = 0;
        if (priorityWeights)
            taken = priorityWeights.ApplyTo(w);

        w.ClampInPlace();

        if (w.priorityScale > 1.0)
        {
            Note(report, "priorityScale = " + w.priorityScale.ToString() + " ist groesser als 1.0. "
                + "Damit verschiebt priority 100 den Score um mehr als 100 Punkte und die "
                + "handgepflegte Zahl ueberstimmt die berechnete Spezifitaet - die Grundregel "
                + "\"das spezifischste gueltige Rezept gewinnt\" ist dann ausgehebelt (09 §7).");
        }

        if (w.IsSpecificityDisabled())
        {
            Note(report, "Alle Spezifitaetsgewichte sind 0. Jedes Rezept hat damit denselben "
                + "Score, und es entscheidet allein der Tiebreak ueber Itemzahl, Slotzahl, "
                + "priority und ID. Das ist selten Absicht (09 §7).");
        }

        if (taken > 0 && ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.CONFIG,
                "Spezifitaetsgewichte: " + taken.ToString() + " Feld(er) aus Core.json - "
                + w.ToDebugString());
        }

        return w;
    }

    /**
     * Die fertigen Stellschrauben der Qualitaetsrechnung (12 §4, S10).
     *
     * Reihenfolge wie bei BuildPriorityWeights, und aus denselben Gruenden:
     *
     *   1. Code-Defaults aus ChefZ_QualityScoring - die Rechnung soll OHNE
     *      jede Konfiguration sinnvoll sein.
     *   2. der Block "qualityScoring" aus Core.json, feldweise.
     *   3. Klammerung: ein negatives Gewicht wuerde die Aussage umdrehen und
     *      frische Zutaten schlechter bewerten als verdorbene.
     *
     * Nie null. Genau EIN Aufruf im ganzen Boot (ChefZ_QualityManager.Build) -
     * ein zweiter wuerde jede Meldung doppelt in den Ladebericht schreiben.
     */
    ChefZ_QualityScoring BuildQualityScoring(ChefZ_LoadReport report)
    {
        ChefZ_QualityScoring sc = new ChefZ_QualityScoring();

        int taken = 0;
        if (qualityScoring)
            taken = qualityScoring.ApplyTo(sc, report);

        sc.ClampInPlace();

        if (sc.freshnessWeight == 0.0 && sc.ingredientQualityWeight == 0.0
            && sc.StatePenaltyCount() == 0)
        {
            Note(report, "Frischegewicht und Zutatenqualitaetsgewicht sind beide 0 und es "
                + "gibt keine Zustandsstrafen. Der Zustand der Zutaten hat damit KEINEN "
                + "Einfluss mehr auf die Qualitaet - altes Fleisch ergibt dasselbe Gericht "
                + "wie frisches. Das ist selten Absicht (12 §4.1).");
        }

        if (taken > 0 && ChefZ_Log.Enabled(ChefZ_LogChannel.CONFIG, ChefZ_LogLevel.DEBUG))
        {
            ChefZ_Log.Debug(ChefZ_LogChannel.CONFIG,
                "Qualitaetsrechnung: " + taken.ToString() + " Angabe(n) aus Core.json - "
                + sc.ToDebugString());
        }

        return sc;
    }

    //! Kanalmaske aus logChannels. Unbekannte Namen landen in unknownOut und
    //! werden vom Aufrufer gemeldet (18 §6).
    int ResolveChannelMask(out array<string> unknownOut)
    {
        return ChefZ_LogChannel.MaskFromNames(logChannels, unknownOut);
    }

    string ToDebugString()
    {
        return "enabled=" + enabled.ToString()
             + " strict=" + strictMode.ToString()
             + " safeModeAt=" + safeModeErrorThreshold.ToString()
             + " logLevel=" + ChefZ_LogLevel.Name(logLevel)
             + " overlay=" + allowProfileOverlay.ToString()
             + " extraItems=" + defaultExtraItems
             + " naehrwertaudit=" + enableNutritionAudit.ToString();
    }

    //--------------------------------------------------------------------------

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        // 1. Nackter Datensatz -> vollstaendige Code-Defaults aus 02 §5.4.
        ChefZ_RecordProbe.Reset();
        ChefZ_CoreSettingsDef d = new ChefZ_CoreSettingsDef();
        d.ResolveDefaults();
        if (d.id != PRIMARY_ID)                     return false;
        if (!d.enabled)                             return false;
        if (d.strictMode)                           return false;
        if (d.safeModeErrorThreshold != 25)         return false;
        if (d.logLevel != 2)                        return false;
        if (!d.logServerOnly)                       return false;
        if (d.matcherNodeBudget != 4096)            return false;
        if (d.sessionTtlSec != 300.0)               return false;
        if (d.defaultFreshnessLifetimeSec != 21600.0) return false;
        if (d.capabilityMode != "asAuthored")       return false;
        if (d.defaultExtraItems != "forbid")        return false;

        // S13 (17 §3.3, 17 §9). Die erste Zeile ist die wichtigste: ohne
        // Anbieter gilt jeder Spieler als 0, und genau das macht "degrade"
        // zur richtigen Vorgabe fuer onFail.
        if (d.defaultCapabilityValue != 0.0)        return false;
        if (d.capabilityMin != 0.0)                 return false;
        if (d.capabilityMax != 10.0)                return false;
        if (d.eventMaxDepth != 3)                   return false;
        if (d.eventSlowSubscriberMs != 5)           return false;
        if (d.eventTiming)                          return false;
        if (!d.allowProfileOverlay)                 return false;

        // S16 (15 §3). Ohne Angabe dauert eine Entnahme zwei Sekunden - lang
        // genug fuer einen sichtbaren Balken, kurz genug fuer acht Portionen.
        if (d.defaultTakePortionSec != ChefZ_PortionLimits.DEFAULT_TAKE_SEC) return false;

        // S17 (16 E5). Die Umgebungssuche ist per Vorgabe an KEINEM Behaelter
        // eingeschaltet - der Radius wirkt erst, wenn eine Deklaration ihn
        // ausdruecklich verlangt.
        if (d.containerSearchRadius != 3.0)         return false;
        if (d.maxContainerCandidates != 32)         return false;

        if (!d.defaultExcludedStates)               return false;
        if (d.defaultExcludedStates.Count() != 2)   return false;

        // S12 (13 §8). Der Audit ist per Vorgabe AN, und das ist die
        // wichtigste dieser vier Zeilen: waere er aus, faende niemand ein
        // Gericht, das lautlos nicht saettigt (01 V7).
        if (!d.enableNutritionAudit)                return false;
        if (d.nutritionTolerancePct != 25.0)        return false;
        if (d.nutritionAuditMaxFindings != 64)      return false;
        if (d.nutritionExpectedCap != 100000.0)     return false;

        // 2. Overlay patcht feldweise: eine Zahl gesetzt, alles andere bleibt.
        ChefZ_CoreSettingsDef patch = new ChefZ_CoreSettingsDef();
        patch.id = PRIMARY_ID;
        patch.SetOrigin("$profile:ChefZ\\Core.json", 3);
        patch.matcherNodeBudget = 128;
        d.PatchFrom(patch);
        if (d.matcherNodeBudget != 128)             return false;
        if (d.safeModeErrorThreshold != 25)         return false;   // unberuehrt
        if (d.capabilityMode != "asAuthored")       return false;   // unberuehrt

        // 3. bool ohne explicitFields wirkt nicht, mit wirkt es.
        ChefZ_CoreSettingsDef p2 = new ChefZ_CoreSettingsDef();
        p2.enabled = false;
        d.PatchFrom(p2);
        if (!d.enabled)                             return false;
        p2.MarkExplicit("enabled");
        d.PatchFrom(p2);
        if (d.enabled)                              return false;

        // 4. Unsinn wird geklammert, nicht abgewiesen.
        ChefZ_CoreSettingsDef bad = new ChefZ_CoreSettingsDef();
        bad.ResolveDefaults();
        bad.matcherNodeBudget = 0;
        bad.minDecayScale     = -1.0;
        bad.maxDecayScale     = -5.0;
        bad.nutritionTolerancePct     = -10.0;
        bad.nutritionAuditMaxFindings = 0;
        bad.nutritionExpectedCap      = 0.0;
        bad.capabilityMode    = "unfug";
        bad.defaultExtraItems = "unfug";
        bad.logLevel          = 99;
        bad.eventMaxDepth     = 0;
        bad.capabilityMax     = -3.0;
        ChefZ_LoadReport rep = new ChefZ_LoadReport();
        rep.SetMirrorToLog(false);
        bad.ClampAndReport(rep);
        if (bad.matcherNodeBudget < 1)              return false;
        if (bad.minDecayScale <= 0.0)               return false;
        if (bad.maxDecayScale < bad.minDecayScale)  return false;
        if (bad.nutritionTolerancePct < 0.0)        return false;
        if (bad.nutritionAuditMaxFindings < 1)      return false;
        if (bad.nutritionExpectedCap <= 0.0)        return false;
        if (bad.capabilityMode != "asAuthored")     return false;
        if (bad.defaultExtraItems != "forbid")      return false;
        if (!ChefZ_LogLevel.IsValid(bad.logLevel))  return false;
        if (bad.eventMaxDepth < 1)                  return false;
        if (bad.capabilityMax < bad.capabilityMin)  return false;
        if (rep.WarnCount() < 6)                    return false;
        if (rep.ErrorCount() != 0)                  return false;   // nichts davon ist ein Fehler

        // 5. Ohne Block "qualityScoring" gelten die Code-Defaults der
        //    Qualitaetsrechnung vollstaendig (12 §4).
        ChefZ_CoreSettingsDef q = new ChefZ_CoreSettingsDef();
        q.ResolveDefaults();
        ChefZ_QualityScoring sc = q.BuildQualityScoring(null);
        if (!sc)                                            return false;
        if (sc.freshnessWeight != 1.0)                      return false;
        if (sc.defaultTierSet != ChefZ_QualityScoring.DEFAULT_TIER_SET) return false;

        return true;
    }
}
