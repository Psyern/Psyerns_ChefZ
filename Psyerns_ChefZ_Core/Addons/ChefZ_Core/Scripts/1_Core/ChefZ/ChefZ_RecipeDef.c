//==============================================================================
// ChefZ_RecipeDef und seine Unterobjekte - die ROHFORM eines Rezepts
//
// Entwurf: 08 §2 (Feldlisten woertlich), 08 §8 (Fehlerverhalten beim Laden),
// 08 E2 / V-B §3 (extraItems-Default "forbid"), 12 §3 (ChefZ_GradeRule),
// 17 §3.3 (ChefZ_CapabilityReq), 09 §7 (priority).
//
// ---------------------------------------------------------------------------
// Was hier steht - und was ausdruecklich nicht
// ---------------------------------------------------------------------------
// Dies ist die unverarbeitete Sicht: Strings, wie ein Content-Autor sie in
// eine JSON-Datei schreibt, und Sentinel fuer "nicht gesetzt". Ausgewertet
// wird NIE auf dieser Form - dafuer gibt es ChefZ_CompiledRecipe. Die
// Trennung ist dieselbe wie ChefZ_Selector gegen ChefZ_CompiledSelector (07)
// und ChefZ_IngredientDef gegen ChefZ_IngredientInfo (05): Rohdaten duerfen
// unvollstaendig sein, das Ergebnis nicht.
//
// KEIN CONTENT. Diese Datei nennt kein Gericht, keine Zutat, keine Kategorie,
// keine Station und kein Geraet. Sie beschreibt ausschliesslich, WELCHE
// FELDER ein Rezept hat. 08 §2 sagt es fuer sein eigenes Beispiel:
// "Kein Zeichen dieses Rezepts existiert im Core."
//
// ---------------------------------------------------------------------------
// Zwei Schreibweisen, die erklaert gehoeren
// ---------------------------------------------------------------------------
// 1. "cls" statt "class". Ein JSON-Schluessel "class" ist mit JsonFileLoader
//    nicht erreichbar - "class" ist ein Enforce-Schluesselwort, ein Feld
//    dieses Namens laesst sich nicht deklarieren. 08 §2 schreibt bei
//    ChefZ_OutputDef bereits selbst "cls". Dieselbe Schreibweise gilt im
//    Selektor (07, ChefZ_Selector, Kopfkommentar).
//
// 2. "requires" steht so in 08 §2 und ist deshalb auch der JSON-Schluessel.
//    Es ist in Enforce kein Schluesselwort und in Vanilla nirgends als
//    Feldname belegt. Sollte ein kuenftiger Compiler ihn dennoch zurueck-
//    weisen, ist die Anpassung an genau drei Stellen noetig: dieses Feld,
//    PatchFrom() und ChefZ_RecipeCompiler.CompileRequirements(). Der
//    JSON-Schluessel hiesse dann "requiresCapabilities".
//
// Layer: 1_Core.
//==============================================================================

/**
 * Abschlussbedingung (08 §2, 10 §6).
 *
 * Als Konstanten statt als enum: der Wert steht als String im JSON, und ein
 * enum faellt bei einem Tippfehler still auf 0 - hier waere das ON_STAGE und
 * damit lautlos etwas anderes als gemeint.
 */
class ChefZ_Completion
{
    static const int ON_STAGE = 0;      //! Vanillas FoodStage ist das Signal
    static const int TIMED    = 1;      //! eigener Zaehler in der Kochsitzung
    static const int INSTANT  = 2;      //! sofort fertig

    static const string ON_STAGE_NAME = "ON_STAGE";
    static const string TIMED_NAME    = "TIMED";
    static const string INSTANT_NAME  = "INSTANT";

    //! -1, wenn der Name unbekannt ist. Der Compiler macht daraus ein WARN
    //! und nimmt ON_STAGE - den Modus, der keine eigene Uhr braucht.
    static int FromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToUpper();

        if (n == ON_STAGE_NAME) return ON_STAGE;
        if (n == TIMED_NAME)    return TIMED;
        if (n == INSTANT_NAME)  return INSTANT;
        return -1;
    }

    static string Name(int mode)
    {
        switch (mode)
        {
            case ON_STAGE: return ON_STAGE_NAME;
            case TIMED:    return TIMED_NAME;
            case INSTANT:  return INSTANT_NAME;
        }
        return "?";
    }

    static string ValidNames()
    {
        return "ON_STAGE, TIMED, INSTANT";
    }
}

//------------------------------------------------------------------------------

/**
 * Umgang mit Items, die kein Slot gebunden hat (08 §2, 08 E2).
 *
 * Der Default ist "forbid" und das ist eine abgenommene Entscheidung
 * (OF-03 / V-B §3): der sichere Default ist der, der zu Vanilla zurueckfaellt.
 * Ein fremdes Item im Topf heisst dann "kein ChefZ-Rezept" - und Vanilla kocht
 * weiter, wie der Spieler es ohne ChefZ erwarten wuerde.
 */
class ChefZ_ExtraItemsMode
{
    static const int FORBID  = 0;
    static const int IGNORE  = 1;
    static const int CONSUME = 2;

    static const string FORBID_NAME  = "forbid";
    static const string IGNORE_NAME  = "ignore";
    static const string CONSUME_NAME = "consume";

    //! -1 = unbekannt.
    static int FromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToLower();

        if (n == FORBID_NAME)  return FORBID;
        if (n == IGNORE_NAME)  return IGNORE;
        if (n == CONSUME_NAME) return CONSUME;
        return -1;
    }

    static string Name(int mode)
    {
        switch (mode)
        {
            case FORBID:  return FORBID_NAME;
            case IGNORE:  return IGNORE_NAME;
            case CONSUME: return CONSUME_NAME;
        }
        return "?";
    }

    static string ValidNames()
    {
        return "forbid, ignore, consume";
    }
}

//==============================================================================

/**
 * WO gilt ein Rezept (08 §2).
 *
 * Mehrere Kontextregeln eines Rezepts sind ODER-verknuepft; die Felder INNER-
 * HALB einer Regel sind UND-verknuepft. Eine Regel ohne Geraetebindung gilt
 * an jedem Geraet - zulaessig, aber selten gewollt, und der Compiler weist
 * darauf hin.
 *
 * 08 E4: Rezepte binden bevorzugt an GERAETEKATEGORIEN, nicht an Klassen. Ein
 * neuer Topf aus einem spaeteren Modul erbt damit alle Topfrezepte, sobald der
 * Betreiber ihn in die Kategorie eintraegt - ohne dass ein Rezept oder gar
 * Core-Code angefasst wird.
 */
class ChefZ_ContextRule
{
    ref array<string> deviceClasses;        // exakte Klassen
    ref array<string> deviceCategories;     // Geraetekategorien
    ref array<string> methods;              // Kochmethoden als Symbolnamen
    ref ChefZ_Range   deviceTemperature;
    ref ChefZ_Range   liquidQuantity;
    ref array<string> liquidTypes;

    //! Default false. Kein Sondendurchgang noetig: "abwesend" und "false"
    //! bedeuten hier dasselbe, und Kontextregeln werden nie feldweise
    //! gepatcht - eine Kontextliste ist eine Aussage als Ganzes.
    bool              requiresLiquid;

    void ChefZ_ContextRule()
    {
        deviceClasses     = null;
        deviceCategories  = null;
        methods           = null;
        deviceTemperature = null;
        liquidQuantity    = null;
        liquidTypes       = null;
        requiresLiquid    = false;
    }

    void Normalize()
    {
        ChefZ_TextList.TrimAll(deviceClasses);
        ChefZ_TextList.TrimAll(deviceCategories);
        ChefZ_TextList.TrimAll(methods);
        ChefZ_TextList.TrimAll(liquidTypes);
    }

    //! Nennt diese Regel ueberhaupt eine Bedingung?
    bool IsEmpty()
    {
        if (ChefZ_TextList.Count(deviceClasses) > 0)    return false;
        if (ChefZ_TextList.Count(deviceCategories) > 0) return false;
        if (ChefZ_TextList.Count(methods) > 0)          return false;
        if (ChefZ_TextList.Count(liquidTypes) > 0)      return false;
        if (deviceTemperature)                          return false;
        if (liquidQuantity)                             return false;
        if (requiresLiquid)                             return false;
        return true;
    }
}

//------------------------------------------------------------------------------

/**
 * WAS darf sonst noch im Gefaess liegen (08 §2).
 *
 * extraItemsAllowedIf ist das Ventil aus 08 E2: ein Rezept darf bestimmte
 * Fremdkoerper dulden, ohne dass der globale Default aufgeweicht wird. V-B
 * Auflage 1 macht davon Gebrauch - sie gehoert allerdings ins CONTENT-Modul,
 * nicht hierher.
 */
class ChefZ_RecipePolicy
{
    //! "" => CoreSettings.defaultExtraItems (V-B Auflage 2: S2 baut, S6 liest)
    string             extraItems;

    ref ChefZ_Selector extraItemsAllowedIf;
    ref array<string>  forbiddenStates;

    //! Untergrenze fuer die Gesundheit gebundener Zutaten. Sentinel = keine.
    float              minMatchedHealth01;

    //! Planungswert fuer den Applicator (S8). Sentinel = keiner.
    float              liquidConsumed;

    void ChefZ_RecipePolicy()
    {
        extraItems          = ChefZ_Undefined.TEXT;
        extraItemsAllowedIf = null;
        forbiddenStates     = null;
        minMatchedHealth01  = ChefZ_Undefined.FLOAT;
        liquidConsumed      = ChefZ_Undefined.FLOAT;
    }

    void Normalize()
    {
        extraItems.TrimInPlace();
        ChefZ_TextList.TrimAll(forbiddenStates);
        if (extraItemsAllowedIf)
            extraItemsAllowedIf.Normalize();
    }
}

//------------------------------------------------------------------------------

//! Eine Ergebnisklasse je Qualitaetsstufe (08 §2, 12 §2).
class ChefZ_OutputVariant
{
    string tier;
    string cls;

    void ChefZ_OutputVariant()
    {
        tier = ChefZ_Undefined.TEXT;
        cls  = ChefZ_Undefined.TEXT;
    }

    void Normalize()
    {
        tier.TrimInPlace();
        cls.TrimInPlace();
    }
}

//------------------------------------------------------------------------------

/**
 * WAS kommt heraus (08 §2).
 *
 * Der Core erzeugt hier nichts - er traegt die Beschreibung. Erzeugt wird in
 * ChefZ_Applicator (4_World, S8), und zwar nach der Reihenfolge aus 08 §6:
 * erzeugen VOR verbrauchen.
 *
 * Zu den beiden bool-Feldern mit Default TRUE: ein abwesendes bool ist im
 * ueberlebenden Parsedurchgang false, und "nicht geschrieben" waere damit
 * nicht von "ausdruecklich false" zu unterscheiden. Deshalb dieselbe
 * Bool-Sonde wie bei den Records (ChefZ_RecordProbe) - ohne sie waere der
 * dokumentierte Default unerreichbar.
 */
class ChefZ_OutputDef
{
    string  cls;                    // Ergebnisklasse (JSON "cls", siehe Kopf)
    float   quantity;               // Sentinel = Klassendefault
    string  quantityMode;           // "fixed" | "fromInput" | "ratio"
    float   ratio;
    //--- Portionierung (15 §3). Alle Felder FLACH im Output, wie im
    //    Content-Beispiel aus 15 §3 - kein Unterobjekt. Ein Unterobjekt
    //    waere eine zweite Ebene im JSON, die der Entwurf nicht schreibt,
    //    und ChefZ_PortionSpec liest sie ohnehin einzeln ab.
    int     portions;               // 0 = kein Portionsgericht
    float   amountPerPortion;
    string  portionClass;           // was bei einer Entnahme entsteht
    float   portionQuantity;        // Quantity je Portion, fest je Rezept
    string  emptyOnLastPortion;     // was aus dem Bulk-Rest wird; "" = loeschen
    bool    scaleWithDevice;        // Default true  (15 §5.1)
    bool    inheritQuality;         // Default true  (15 §3)
    bool    inheritState;           // Default true  (15 §3)
    float   takeDurationSec;        // < 0 = Vorgabe aus den CoreSettings

    //! Aktionstext der Entnahme (15 E5: "GetText aus der PortionSpec, nicht
    //! aus Code"). Stringtable-Schluessel; leer heisst "Rueckfalltext des
    //! Core". Der Core zeigt damit nie einen Gerichtenamen an, den er kennt -
    //! er zeigt den Schluessel an, den die Daten nennen.
    string  takeDisplayName;

    string  containerCategory;
    bool    consumesContainer;      // Default true  (16 §3)
    string  returnContainer;        // "" | "AUTO" | Klassenname
    string  setState;               // ChefZ-Zustand am Ergebnis
    bool    inheritFreshness;       // Default true
    float   freshnessCarry;         // Default 1.0
    bool    inheritTemperature;     // Default true
    float   chance;                 // 0..1, Default 1
    ref array<string> effects;
    ref array<ref ChefZ_OutputVariant> variants;

    //! Wie ChefZ_Record.explicitFields (02 E3, Mittel 3), nur fuer die beiden
    //! bool-Felder dieses Unterobjekts.
    ref array<string> explicitFields;

    void ChefZ_OutputDef()
    {
        cls               = ChefZ_Undefined.TEXT;
        quantityMode      = ChefZ_Undefined.TEXT;
        containerCategory = ChefZ_Undefined.TEXT;
        returnContainer   = ChefZ_Undefined.TEXT;
        setState          = ChefZ_Undefined.TEXT;
        portionClass      = ChefZ_Undefined.TEXT;
        emptyOnLastPortion = ChefZ_Undefined.TEXT;
        takeDisplayName   = ChefZ_Undefined.TEXT;

        quantity          = ChefZ_Undefined.FLOAT;
        ratio             = ChefZ_Undefined.FLOAT;
        amountPerPortion  = ChefZ_Undefined.FLOAT;
        portionQuantity   = ChefZ_Undefined.FLOAT;
        takeDurationSec   = ChefZ_Undefined.FLOAT;
        freshnessCarry    = ChefZ_Undefined.FLOAT;
        chance            = ChefZ_Undefined.FLOAT;
        portions          = ChefZ_Undefined.INT;

        effects           = null;
        variants          = null;
        explicitFields    = null;

        inheritFreshness   = ChefZ_RecordProbe.Bool();
        inheritTemperature = ChefZ_RecordProbe.Bool();
        scaleWithDevice    = ChefZ_RecordProbe.Bool();
        inheritQuality     = ChefZ_RecordProbe.Bool();
        inheritState       = ChefZ_RecordProbe.Bool();
        consumesContainer  = ChefZ_RecordProbe.Bool();
    }

    void Normalize()
    {
        cls.TrimInPlace();
        quantityMode.TrimInPlace();
        containerCategory.TrimInPlace();
        returnContainer.TrimInPlace();
        setState.TrimInPlace();
        portionClass.TrimInPlace();
        emptyOnLastPortion.TrimInPlace();
        takeDisplayName.TrimInPlace();
        ChefZ_TextList.TrimAll(effects);

        if (!variants)
            return;
        for (int i = 0; i < variants.Count(); i++)
        {
            ChefZ_OutputVariant v = variants.Get(i);
            if (v)
                v.Normalize();
        }
    }

    void MarkExplicit(string field)
    {
        if (!explicitFields)
            explicitFields = new array<string>();
        if (explicitFields.Find(field) < 0)
            explicitFields.Insert(field);
    }

    bool HasExplicit(string field)
    {
        if (!explicitFields)
            return false;
        return explicitFields.Find(field) >= 0;
    }

    //! Vergleich mit demselben Output aus dem Sondendurchgang: was in BEIDEN
    //! Durchgaengen gleich ist, stand ausdruecklich in der Datei.
    void CaptureExplicitBools(ChefZ_OutputDef other)
    {
        if (!other)
            return;
        if (inheritFreshness   == other.inheritFreshness)   MarkExplicit("inheritFreshness");
        if (inheritTemperature == other.inheritTemperature) MarkExplicit("inheritTemperature");
        if (scaleWithDevice    == other.scaleWithDevice)    MarkExplicit("scaleWithDevice");
        if (inheritQuality     == other.inheritQuality)     MarkExplicit("inheritQuality");
        if (inheritState       == other.inheritState)       MarkExplicit("inheritState");
        if (consumesContainer  == other.consumesContainer)  MarkExplicit("consumesContainer");
    }

    //! Code-Defaults aus 08 §2.
    void ResolveDefaults()
    {
        quantityMode   = ChefZ_Undefined.TextOr(quantityMode, "fixed");
        portions       = ChefZ_Undefined.IntOr(portions, 0);
        freshnessCarry = ChefZ_Undefined.FloatOr(freshnessCarry, 1.0);
        chance         = ChefZ_Undefined.FloatOr(chance, 1.0);
        ratio          = ChefZ_Undefined.FloatOr(ratio, 1.0);

        if (!HasExplicit("inheritFreshness"))
            inheritFreshness = true;
        if (!HasExplicit("inheritTemperature"))
            inheritTemperature = true;

        // 15 §3: die vier Portions- und Behaelterschalter haben Default TRUE.
        // Dieselbe Sonde wie oben, aus demselben Grund: ein abwesendes bool
        // ist im ueberlebenden Parsedurchgang false, und "nicht geschrieben"
        // waere damit nicht von "ausdruecklich false" zu unterscheiden.
        if (!HasExplicit("scaleWithDevice"))
            scaleWithDevice = true;
        if (!HasExplicit("inheritQuality"))
            inheritQuality = true;
        if (!HasExplicit("inheritState"))
            inheritState = true;
        if (!HasExplicit("consumesContainer"))
            consumesContainer = true;

        // takeDurationSec und portionQuantity bleiben BEWUSST auf ihrem
        // Sentinel: "nichts gesagt" heisst hier "Vorgabe des Core" bzw.
        // "Klassendefault der Portionsklasse", und beide kennt dieser Record
        // nicht. Aufgeloest wird es im ChefZ_PortionManager.
    }

    bool HasQuantity()
    {
        return !ChefZ_Undefined.IsFloatUndefined(quantity);
    }

    bool HasAmountPerPortion()
    {
        return !ChefZ_Undefined.IsFloatUndefined(amountPerPortion);
    }

    bool HasPortionQuantity()
    {
        return !ChefZ_Undefined.IsFloatUndefined(portionQuantity);
    }

    bool HasTakeDuration()
    {
        return !ChefZ_Undefined.IsFloatUndefined(takeDurationSec);
    }

    //! Ist dieses Ergebnis ueberhaupt ein Portionsgericht (15 §7, Zeile 1)?
    //! "Keine Portionsdaten -> portions = 1 -> gewoehnliches Item. Kein
    //! Fehler, nur weniger Komfort." Deshalb ist die Frage hier eng gestellt:
    //! ohne portionClass gibt es nichts zu entnehmen.
    bool IsPortioned()
    {
        return portions > 0 && portionClass != "";
    }

    string ToDebugString()
    {
        string s = cls;
        if (portions > 0)
            s = s + " x" + portions.ToString() + " Portionen";
        if (chance < 1.0)
            s = s + " p=" + chance.ToString();
        if (setState != "")
            s = s + " -> " + setState;
        return s;
    }
}

//------------------------------------------------------------------------------

/**
 * Eine Qualitaetsregel (12 §3).
 *
 * Sie steht hier, weil ein Rezept sie traegt - AUSGEWERTET wird sie vom
 * Quality Manager (S10). Der Core rechnet in S6 mit ihr nicht; er reicht sie
 * unveraendert an das kompilierte Rezept weiter. Seit S10 uebersetzt der
 * ChefZ_QualityManager sie beim Boot in ChefZ_CompiledGradeRule und fuellt
 * beim Kochen ChefZ_MatchResult.qualityTier.
 */
class ChefZ_GradeRule
{
    string             ruleId;
    string             when;        // "slotFilled" | "slotCount" | "anyItem"
                                    // | "allMatched" | "context" | "capability"
    string             slotId;
    ref ChefZ_Selector selector;
    string             capability;

    /**
     * WELCHE Kontextgroesse gemeint ist, wenn when == "context" (S10).
     *
     * ERGAENZUNG GEGENUEBER 12 §3, und eine notwendige: der Entwurf fuehrt
     * "context" als Regelart auf und gibt ihr mit "range" einen Wertebereich,
     * nennt aber kein Feld, das die Groesse benennt. Ohne dieses Feld waere
     * die Regelart nicht auswertbar, sondern nur ratbar.
     *
     * Die zulaessigen Werte stehen in ChefZ_GradeContextKey - allesamt Felder
     * des Kochkontexts oder Kennzahlen des Matchergebnisses, also kein
     * Content. Fuer alle anderen Regelarten bleibt das Feld leer.
     */
    string             contextKey;

    ref ChefZ_Range    range;
    float              points;
    float              pointsPerItem;
    float              maxPoints;

    void ChefZ_GradeRule()
    {
        ruleId        = ChefZ_Undefined.TEXT;
        when          = ChefZ_Undefined.TEXT;
        slotId        = ChefZ_Undefined.TEXT;
        capability    = ChefZ_Undefined.TEXT;
        contextKey    = ChefZ_Undefined.TEXT;
        selector      = null;
        range         = null;
        points        = ChefZ_Undefined.FLOAT;
        pointsPerItem = ChefZ_Undefined.FLOAT;
        maxPoints     = ChefZ_Undefined.FLOAT;
    }

    void Normalize()
    {
        ruleId.TrimInPlace();
        when.TrimInPlace();
        slotId.TrimInPlace();
        capability.TrimInPlace();
        contextKey.TrimInPlace();
        if (selector)
            selector.Normalize();
    }
}

//------------------------------------------------------------------------------

/**
 * Eine Faehigkeitsanforderung (17 §3.3).
 *
 * Wie ChefZ_GradeRule: hier nur getragen. Ausgewertet wird sie seit S13 von
 * der ChefZ_CapabilityRegistry, und zwar an drei Stellen mit drei Wirkungen -
 * "block" filtert den Kandidaten (08 §7 Schritt 2c), "degrade" verschiebt die
 * Qualitaetsstufe (12 E8), "reduceYield" die Ausbeute.
 *
 * Solange es keinen Anbieter gibt, blockiert nichts davon - 17 §3.3: "Ohne
 * Provider: Default aus ChefZ_CoreSettingsDef. Nie Fehler."
 */
class ChefZ_CapabilityReq
{
    string capability;
    float  min;
    string onFail;          // "block" | "degrade" | "reduceYield"
    int    degradeSteps;
    float  yieldFactor;

    void ChefZ_CapabilityReq()
    {
        capability   = ChefZ_Undefined.TEXT;
        onFail       = ChefZ_Undefined.TEXT;
        min          = ChefZ_Undefined.FLOAT;
        degradeSteps = ChefZ_Undefined.INT;
        yieldFactor  = ChefZ_Undefined.FLOAT;
    }

    void Normalize()
    {
        capability.TrimInPlace();
        onFail.TrimInPlace();
    }

    //! Code-Defaults aus 17 §3.3.
    void ResolveDefaults()
    {
        onFail       = ChefZ_Undefined.TextOr(onFail, "degrade");
        min          = ChefZ_Undefined.FloatOr(min, 0.0);
        degradeSteps = ChefZ_Undefined.IntOr(degradeSteps, 1);
        yieldFactor  = ChefZ_Undefined.FloatOr(yieldFactor, 0.75);
    }
}

//==============================================================================

/**
 * Das Rezept in Rohform (08 §2).
 *
 * Validate() setzt genau die drei Abweisungsgruende aus 08 §8 um, die ohne
 * jeden Nachschlager entscheidbar sind:
 *
 *   ohne slots     -> es wuerde auf JEDEN leeren Topf matchen
 *   ohne outputs   -> es wuerde Zutaten loeschen und nichts erzeugen
 *   ohne contexts  -> es zuendete in jedem Topf, jeder Pfanne, jedem Kessel
 *
 * Alles Weitere - unbekannte Kategorie, fehlende Ergebnisklasse, TIMED ohne
 * cookSeconds - braucht Registries und CfgVehicles und sitzt deshalb im
 * ChefZ_RecipeCompiler (3_Game).
 */
class ChefZ_RecipeDef extends ChefZ_Record
{
    //--- WO gilt es ----------------------------------------------------------
    ref array<ref ChefZ_ContextRule>   contexts;

    //--- WAS wird gebraucht --------------------------------------------------
    ref array<ref ChefZ_SlotDef>       slots;

    //--- WAS darf sonst noch drin sein ---------------------------------------
    ref ChefZ_RecipePolicy             policy;

    //--- WANN ist es fertig --------------------------------------------------
    string  completion;                 // "ON_STAGE" | "TIMED" | "INSTANT"
    ref array<string> doneStages;
    float   cookSeconds;
    float   minTemperature;

    //--- WAS kommt heraus ----------------------------------------------------
    ref array<ref ChefZ_OutputDef>     outputs;
    ref array<ref ChefZ_OutputDef>     byproducts;

    //--- WIE gut ist es ------------------------------------------------------
    ref array<ref ChefZ_GradeRule>     gradeRules;
    string  qualityTierSet;             // "" => "DISH_DEFAULT"
    float   qualityBias;

    //--- WER darf es ---------------------------------------------------------
    ref array<ref ChefZ_CapabilityReq> requires;

    //--- WERKZEUG ------------------------------------------------------------
    ref array<string>                  requiredToolGroups;

    //--- Feinjustierung und Weitergabe ---------------------------------------
    int     priority;                   // 09: gedaempft, nie Primaerschluessel
    ref array<string> effects;
    ref array<string> emitEvents;
    float   nutritionModifier;

    void ChefZ_RecipeDef()
    {
        contexts           = null;
        slots              = null;
        policy             = null;
        doneStages         = null;
        outputs            = null;
        byproducts         = null;
        gradeRules         = null;
        requires           = null;
        requiredToolGroups = null;
        effects            = null;
        emitEvents         = null;

        completion         = ChefZ_Undefined.TEXT;
        qualityTierSet     = ChefZ_Undefined.TEXT;

        cookSeconds        = ChefZ_Undefined.FLOAT;
        minTemperature     = ChefZ_Undefined.FLOAT;
        qualityBias        = ChefZ_Undefined.FLOAT;
        nutritionModifier  = ChefZ_Undefined.FLOAT;

        // 09 §7: "priority fehlt -> 0. KEIN WARN - der Normalfall ist, sie
        // nicht zu setzen." Der Sentinel bleibt trotzdem, damit ein Overlay
        // die 0 ausdruecklich setzen kann, ohne dass sie wie "nicht gesetzt"
        // aussieht (02 E3).
        priority           = ChefZ_Undefined.INT;
    }

    override string GetKindName()
    {
        return ChefZ_RecordKind.RECIPE;
    }

    //--------------------------------------------------------------------------
    // NORMALIZE
    //--------------------------------------------------------------------------

    override void Normalize()
    {
        super.Normalize();

        completion.TrimInPlace();
        qualityTierSet.TrimInPlace();
        ChefZ_TextList.TrimAll(doneStages);
        ChefZ_TextList.TrimAll(requiredToolGroups);
        ChefZ_TextList.TrimAll(effects);
        ChefZ_TextList.TrimAll(emitEvents);

        int i;
        if (contexts)
        {
            for (i = 0; i < contexts.Count(); i++)
            {
                ChefZ_ContextRule c = contexts.Get(i);
                if (c)
                    c.Normalize();
            }
        }
        if (slots)
        {
            for (i = 0; i < slots.Count(); i++)
            {
                ChefZ_SlotDef s = slots.Get(i);
                if (s)
                    s.Normalize();
            }
        }
        if (policy)
            policy.Normalize();

        NormalizeOutputs(outputs);
        NormalizeOutputs(byproducts);

        if (gradeRules)
        {
            for (i = 0; i < gradeRules.Count(); i++)
            {
                ChefZ_GradeRule g = gradeRules.Get(i);
                if (g)
                    g.Normalize();
            }
        }
        if (requires)
        {
            for (i = 0; i < requires.Count(); i++)
            {
                ChefZ_CapabilityReq r = requires.Get(i);
                if (r)
                    r.Normalize();
            }
        }
    }

    private void NormalizeOutputs(array<ref ChefZ_OutputDef> list)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_OutputDef o = list.Get(i);
            if (o)
                o.Normalize();
        }
    }

    //--------------------------------------------------------------------------
    // VALIDATE (08 §8)
    //--------------------------------------------------------------------------

    override bool Validate(ChefZ_ValidationContext ctx)
    {
        if (!super.Validate(ctx))
            return false;

        if (!slots || slots.Count() == 0)
        {
            if (ctx)
                ctx.Error(this, "Rezept ohne \"slots\" - abgewiesen. Es haette keine " + "Bedingung und wuerde damit auf jeden Gefaessinhalt passen, auch " + "auf einen leeren.");
            return false;
        }

        if (!outputs || outputs.Count() == 0)
        {
            if (ctx)
                ctx.Error(this, "Rezept ohne \"outputs\" - abgewiesen. Es wuerde die " + "Zutaten verbrauchen und nichts erzeugen.");
            return false;
        }

        if (!contexts || contexts.Count() == 0)
        {
            if (ctx)
                ctx.Error(this, "Rezept ohne \"contexts\" - abgewiesen. Es zuendete in " + "jedem Topf, jeder Pfanne und jedem Kessel (08 E4: Rezepte binden " + "an Geraetekategorien).");
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // MERGE (02 E3)
    //
    // Unterobjektlisten werden als GANZES ersetzt, nie elementweise gemischt.
    // Eine halb gepatchte Slotliste waere ein Rezept, das niemand geschrieben
    // hat - und der Autor des Overlays saehe im Ergebnis etwas anderes als in
    // seiner Datei.
    //--------------------------------------------------------------------------

    override void PatchFrom(notnull ChefZ_Record src)
    {
        super.PatchFrom(src);
        ChefZ_RecipeDef s = ChefZ_RecipeDef.Cast(src);
        if (!s)
            return;

        if (s.contexts)             contexts           = s.contexts;
        if (s.slots)                slots              = s.slots;
        if (s.policy)               policy             = s.policy;
        if (s.outputs)              outputs            = s.outputs;
        if (s.byproducts)           byproducts         = s.byproducts;
        if (s.gradeRules)           gradeRules         = s.gradeRules;
        if (s.requires)             requires           = s.requires;

        doneStages         = PatchStringArray(doneStages, s.doneStages);
        requiredToolGroups = PatchStringArray(requiredToolGroups, s.requiredToolGroups);
        effects            = PatchStringArray(effects, s.effects);
        emitEvents         = PatchStringArray(emitEvents, s.emitEvents);

        completion         = PatchText(completion, s.completion, s, "completion");
        qualityTierSet     = PatchText(qualityTierSet, s.qualityTierSet, s, "qualityTierSet");

        cookSeconds        = PatchFloat(cookSeconds, s.cookSeconds, s, "cookSeconds");
        minTemperature     = PatchFloat(minTemperature, s.minTemperature, s, "minTemperature");
        qualityBias        = PatchFloat(qualityBias, s.qualityBias, s, "qualityBias");
        nutritionModifier  = PatchFloat(nutritionModifier, s.nutritionModifier, s, "nutritionModifier");
        priority           = PatchInt(priority, s.priority, s, "priority");
    }

    //--------------------------------------------------------------------------
    // Bool-Sonde
    //
    // Das Rezept selbst hat ausser "disabled" kein bool. Seine UNTEROBJEKTE
    // haben welche, und die Sonde erreicht sie nur ueber diesen Durchgriff:
    // 07 §2.3 hat es fuer den Slot ausdruecklich so vorgesehen ("Der
    // Rezeptleser (S6) traegt sie aus dem Sondendurchgang nach").
    //
    // Die Paarung geschieht ueber den INDEX, nicht ueber die slotId. Beide
    // Durchgaenge lesen dieselbe Datei mit demselben Serializer; die
    // Reihenfolge ist damit dieselbe. Weicht die Laenge ab, wird der Rest
    // ausgelassen - dann fehlen nur automatisch erkannte bool-Felder, und ein
    // handgeschriebenes explicitFields[] wirkt weiterhin.
    //--------------------------------------------------------------------------

    override void CaptureExplicitBools(ChefZ_Record other)
    {
        super.CaptureExplicitBools(other);
        ChefZ_RecipeDef o = ChefZ_RecipeDef.Cast(other);
        if (!o)
            return;

        int i;
        if (slots && o.slots)
        {
            for (i = 0; i < slots.Count() && i < o.slots.Count(); i++)
            {
                ChefZ_SlotDef mine = slots.Get(i);
                if (mine)
                    mine.CaptureExplicitBools(o.slots.Get(i));
            }
        }

        CaptureOutputBools(outputs, o.outputs);
        CaptureOutputBools(byproducts, o.byproducts);
    }

    private void CaptureOutputBools(array<ref ChefZ_OutputDef> mine, array<ref ChefZ_OutputDef> probe)
    {
        if (!mine || !probe)
            return;
        for (int i = 0; i < mine.Count() && i < probe.Count(); i++)
        {
            ChefZ_OutputDef o = mine.Get(i);
            if (o)
                o.CaptureExplicitBools(probe.Get(i));
        }
    }

    //--------------------------------------------------------------------------
    // Nachbereitung
    //--------------------------------------------------------------------------

    /**
     * Code-Defaults aus 08 §2 und 09 §7.
     *
     * Bewusst NICHT hier: completion, extraItems und doneStages. Die haengen
     * an CoreSettings (allowTimedRecipes, defaultExtraItems) beziehungsweise
     * verlangen ein WARN, und beides gehoert in den Compiler, der einen
     * Ladebericht hat. Ein Record hat keinen.
     */
    override void ResolveDefaults()
    {
        super.ResolveDefaults();

        priority          = ChefZ_Undefined.IntOr(priority, 0);
        qualityBias       = ChefZ_Undefined.FloatOr(qualityBias, 0.0);
        nutritionModifier = ChefZ_Undefined.FloatOr(nutritionModifier, 1.0);
        cookSeconds       = ChefZ_Undefined.FloatOr(cookSeconds, 0.0);
        minTemperature    = ChefZ_Undefined.FloatOr(minTemperature, 0.0);

        int i;
        if (slots)
        {
            for (i = 0; i < slots.Count(); i++)
            {
                ChefZ_SlotDef s = slots.Get(i);
                if (s)
                    s.ResolveDefaults();
            }
        }
        ResolveOutputDefaults(outputs);
        ResolveOutputDefaults(byproducts);

        if (requires)
        {
            for (i = 0; i < requires.Count(); i++)
            {
                ChefZ_CapabilityReq r = requires.Get(i);
                if (r)
                    r.ResolveDefaults();
            }
        }
    }

    private void ResolveOutputDefaults(array<ref ChefZ_OutputDef> list)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_OutputDef o = list.Get(i);
            if (o)
                o.ResolveDefaults();
        }
    }

    //--------------------------------------------------------------------------

    int SlotCount()
    {
        if (!slots)
            return 0;
        return slots.Count();
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_RecordProbe.Reset();

        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);

        // 1. Die drei Abweisungsgruende aus 08 §8.
        ChefZ_RecipeDef bare = new ChefZ_RecipeDef();
        bare.id = "CHEFZ_RD_LEER";
        if (bare.Validate(ctx))                             return false;   // ohne slots

        bare.slots = new array<ref ChefZ_SlotDef>();
        bare.slots.Insert(new ChefZ_SlotDef());
        if (bare.Validate(ctx))                             return false;   // ohne outputs

        bare.outputs = new array<ref ChefZ_OutputDef>();
        bare.outputs.Insert(new ChefZ_OutputDef());
        if (bare.Validate(ctx))                             return false;   // ohne contexts

        bare.contexts = new array<ref ChefZ_ContextRule>();
        bare.contexts.Insert(new ChefZ_ContextRule());
        if (!bare.Validate(ctx))                            return false;

        // 2. Defaults.
        bare.ResolveDefaults();
        if (bare.priority != 0)                             return false;
        if (bare.nutritionModifier != 1.0)                  return false;
        if (!bare.outputs.Get(0).inheritFreshness)          return false;
        if (bare.outputs.Get(0).chance != 1.0)              return false;

        // 3. Namen der Aufzaehlungen.
        if (ChefZ_Completion.FromName("timed") != ChefZ_Completion.TIMED)        return false;
        if (ChefZ_Completion.FromName("unfug") != -1)                            return false;
        if (ChefZ_ExtraItemsMode.FromName("FORBID") != ChefZ_ExtraItemsMode.FORBID) return false;
        if (ChefZ_ExtraItemsMode.FromName("unfug") != -1)                        return false;

        // 4. Bool-Sonde durch die Unterobjekte hindurch: gleicher Wert in
        //    beiden Durchgaengen heisst "stand in der Datei".
        ChefZ_RecipeDef low  = new ChefZ_RecipeDef();
        ChefZ_RecipeDef high = new ChefZ_RecipeDef();
        low.outputs  = new array<ref ChefZ_OutputDef>();
        high.outputs = new array<ref ChefZ_OutputDef>();
        ChefZ_OutputDef lowOut  = new ChefZ_OutputDef();
        ChefZ_OutputDef highOut = new ChefZ_OutputDef();
        lowOut.inheritFreshness  = false;
        highOut.inheritFreshness = false;           // gleich -> ausdruecklich
        low.outputs.Insert(lowOut);
        high.outputs.Insert(highOut);
        low.CaptureExplicitBools(high);
        if (!lowOut.HasExplicit("inheritFreshness"))        return false;
        lowOut.ResolveDefaults();
        if (lowOut.inheritFreshness)                        return false;   // false bleibt false

        return true;
    }
}
