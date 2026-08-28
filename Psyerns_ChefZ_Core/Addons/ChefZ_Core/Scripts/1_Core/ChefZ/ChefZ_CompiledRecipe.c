//==============================================================================
// ChefZ_CompiledContext / ChefZ_CompiledPolicy / ChefZ_CompiledRecipe
//
// Entwurf: 08 §2 (Felder), 08 §4 (Auswertungsablauf), 08 §5.1 (was der Index
// braucht), 08 §7 (nach dem Build unveraenderlich), 09 §4.1 (Spezifitaet),
// 09 §4.3 (Rangreihenfolge), 07 §2.2 (dieselbe Trennung Rohform/kompiliert).
//
// ---------------------------------------------------------------------------
// Warum es diese Form gibt
// ---------------------------------------------------------------------------
// In der kompilierten Form gibt es keinen String mehr, keinen Sentinel und
// keinen Defaultzweig. Jede Kontextpruefung ist ein Symbolvergleich, jede
// Zustandspruefung ein Find() auf einer kurzen Liste. Der teure Teil - Namen
// aufloesen, Defaults einsetzen, Unsinn abweisen - ist beim Boot einmal
// passiert und passiert nie wieder (09 E2).
//
// Nach dem Build wird an einem kompilierten Rezept NICHTS mehr veraendert.
// Alles Veraenderliche liegt im ChefZ_FactSnapshot und im ChefZ_MatchResult.
//
// ---------------------------------------------------------------------------
// Das Torsymbol (08 §5.1, "seltenster Pflichtslot")
// ---------------------------------------------------------------------------
// Der invertierte Index fuehrt je Rezept EIN Symbol, das ein Pflichtslot
// zwingend verlangt - und zwar das seltenste. Fehlt dieses Symbol im Gefaess,
// kann das Rezept nicht binden, und der Kandidat faellt ohne einen einzigen
// Matcher-Knoten heraus.
//
// Die Auswahl "das seltenste" folgt derselben Ueberlegung wie die
// Slotreihenfolge des Matchers (07 E4): der engste Test zuerst. Rezepte, deren
// Pflichtslots sich nicht auf ein einzelnes Symbol zurueckfuehren lassen
// (reines anyOf, reine Wertebereiche), bekommen kein Tor - sie sind dann
// Kandidat, sobald das Geraet passt. Das ist korrekt, nur teurer, und es ist
// die richtige Richtung: ein fehlendes Tor darf nie ein Rezept ausschliessen.
//
// KEIN CONTENT: hier steht kein Geraet, keine Methode, kein Zustand.
//
// Layer: 1_Core.
//==============================================================================

/**
 * Eine Kontextregel, fertig kompiliert (08 §2, ChefZ_ContextRule).
 *
 * Matches() ist Schritt 2a aus 08 §4 und die erste Pruefung ueberhaupt: sie
 * kostet ein paar Symbolvergleiche und wirft die grosse Mehrheit der
 * Kandidaten hinaus, bevor der Matcher auch nur anlaeuft.
 *
 * Eine leere Liste heisst "egal", nicht "nichts". Das ist die einzige Lesart,
 * die zu 08 §2 passt: dort ist jedes Feld der Regel optional, und eine Regel,
 * die nur "methods" nennt, soll an jedem Geraet gelten.
 */
class ChefZ_CompiledContext
{
    ref array<ChefZ_Sym> deviceClasses;
    ref array<ChefZ_Sym> deviceCategories;
    ref array<ChefZ_Sym> methods;
    ref ChefZ_Range      deviceTemperature;
    ref ChefZ_Range      liquidQuantity;
    ref array<ChefZ_Sym> liquidTypes;
    bool                 requiresLiquid;

    void ChefZ_CompiledContext()
    {
        deviceClasses     = new array<ChefZ_Sym>();
        deviceCategories  = new array<ChefZ_Sym>();
        methods           = new array<ChefZ_Sym>();
        liquidTypes       = new array<ChefZ_Sym>();
        deviceTemperature = null;
        liquidQuantity    = null;
        requiresLiquid    = false;
    }

    /**
     * Passt diese Regel auf den Kontext? reason traegt bei false die
     * Begruendung im Klartext (07 E6: der Trace wird von Menschen gelesen).
     *
     * Die Reihenfolge ist nach Aussagekraft gewaehlt, nicht nach Kosten -
     * alle Pruefungen sind gleich billig, aber "falsches Geraet" ist die
     * nuetzlichste Antwort und steht deshalb zuerst.
     */
    bool Matches(notnull ChefZ_CookContext ctx, out string reason)
    {
        reason = "";

        if (!MatchesDevice(ctx))
        {
            reason = "Geraet " + ChefZ_SymbolTable.NameOrMark(ctx.deviceClass) + " passt nicht (erwartet: " + DeviceDescription() + ")";
            return false;
        }

        if (methods.Count() > 0 && methods.Find(ctx.method) < 0)
        {
            reason = "Kochmethode " + ChefZ_SymbolTable.NameOrMark(ctx.method)
                   + " passt nicht (erwartet: " + ChefZ_TextList.JoinSymbols(methods, "/") + ")";
            return false;
        }

        if (deviceTemperature && !deviceTemperature.Contains(ctx.deviceTemperature))
        {
            reason = "Temperatur " + ctx.deviceTemperature.ToString()
                   + " ausserhalb " + deviceTemperature.ToDebugString();
            return false;
        }

        if (requiresLiquid && !ctx.HasLiquid())
        {
            reason = "Rezept braucht Fluessigkeit, das Gefaess ist trocken";
            return false;
        }

        if (liquidTypes.Count() > 0 && liquidTypes.Find(ctx.liquidType) < 0)
        {
            reason = "Fluessigkeit " + ChefZ_SymbolTable.NameOrMark(ctx.liquidType)
                   + " passt nicht (erwartet: " + ChefZ_TextList.JoinSymbols(liquidTypes, "/") + ")";
            return false;
        }

        if (liquidQuantity && !liquidQuantity.Contains(ctx.liquidQuantity))
        {
            reason = "Fluessigkeitsmenge " + ctx.liquidQuantity.ToString()
                   + " ausserhalb " + liquidQuantity.ToDebugString();
            return false;
        }

        return true;
    }

    /**
     * Geraeteteil der Pruefung.
     *
     * Klassen und Kategorien sind ODER-verknuepft: ein Rezept, das beides
     * nennt, laeuft an einem Geraet, das eines von beiden erfuellt. Die
     * Alternative (UND) waere nicht ausdrueckbar ohne eine zweite Regel - und
     * genau dafuer gibt es mehrere Kontextregeln.
     */
    bool MatchesDevice(notnull ChefZ_CookContext ctx)
    {
        if (deviceClasses.Count() == 0 && deviceCategories.Count() == 0)
            return true;            // Regel ohne Geraetebindung: jedes Geraet

        if (deviceClasses.Count() > 0)
        {
            if (deviceClasses.Find(ctx.deviceClass) >= 0)
                return true;
            if (ChefZ_SymbolTable.IsValid(ctx.deviceRootClass)
                && deviceClasses.Find(ctx.deviceRootClass) >= 0)
                return true;
        }

        for (int i = 0; i < deviceCategories.Count(); i++)
        {
            if (ctx.HasDeviceCategory(deviceCategories.Get(i)))
                return true;
        }

        return false;
    }

    //! Zahl der GEBUNDENEN Zusatzbedingungen fuer die Spezifitaet (09 §4.1:
    //! "wContextBound je gebundener Temperatur-/Fluessigkeitsbedingung").
    //! Geraeteklassen zaehlen dort eigens mit wContextDeviceClass und sind
    //! hier deshalb ausdruecklich NICHT enthalten.
    int BoundConditionCount()
    {
        int n = 0;
        if (deviceTemperature && !deviceTemperature.IsUnbounded())  n++;
        if (liquidQuantity && !liquidQuantity.IsUnbounded())        n++;
        if (liquidTypes.Count() > 0)                                n++;
        if (requiresLiquid)                                         n++;
        return n;
    }

    //! Zahl aller Bedingungen - fuer totalConstraints (09 §4.3, Schluessel 3).
    int ConstraintCount()
    {
        int n = BoundConditionCount();
        n = n + deviceClasses.Count();
        n = n + deviceCategories.Count();
        if (methods.Count() > 0)
            n++;
        return n;
    }

    private string DeviceDescription()
    {
        if (deviceClasses.Count() == 0 && deviceCategories.Count() == 0)
            return "beliebig";

        string s = "";
        if (deviceClasses.Count() > 0)
            s = ChefZ_TextList.JoinSymbols(deviceClasses, "/");
        if (deviceCategories.Count() > 0)
        {
            if (s != "")
                s = s + " oder ";
            s = s + ChefZ_TextList.JoinSymbols(deviceCategories, "/");
        }
        return s;
    }

    string ToDebugString()
    {
        string s = DeviceDescription();
        if (methods.Count() > 0)
            s = s + " methode=" + ChefZ_TextList.JoinSymbols(methods, "/");
        if (deviceTemperature)
            s = s + " temp" + deviceTemperature.ToDebugString();
        if (requiresLiquid)
            s = s + " +fluessig";
        return s;
    }
}

//==============================================================================

/**
 * Die Policy, fertig kompiliert (08 §2, ChefZ_RecipePolicy).
 *
 * extraItemsMode ist hier IMMER aufgeloest - der Compiler hat den Default aus
 * CoreSettings.defaultExtraItems bereits eingesetzt (V-B Auflage 2). Das ist
 * die Voraussetzung fuer V-B §3 Folge 4: wPolicyForbid bezieht sich auf den
 * EFFEKTIVEN Wert, nicht darauf, ob ein Autor den Default ausgeschrieben hat.
 * Sonst haenge die Rezeptprioritaet an einer Schreibgewohnheit.
 */
class ChefZ_CompiledPolicy
{
    int  extraItemsMode;
    ref ChefZ_CompiledSelector extraItemsAllowedIf;
    ref array<ChefZ_Sym> forbiddenStates;

    //! Untergrenze fuer die Gesundheit gebundener Zutaten. 0 = keine.
    float minMatchedHealth01;

    //! Planungswert fuer den Applicator (S8). 0 = kein Verbrauch.
    float liquidConsumed;

    void ChefZ_CompiledPolicy()
    {
        extraItemsMode      = ChefZ_ExtraItemsMode.FORBID;
        extraItemsAllowedIf = null;
        forbiddenStates     = new array<ChefZ_Sym>();
        minMatchedHealth01  = 0.0;
        liquidConsumed      = 0.0;
    }

    bool IsStateForbidden(ChefZ_Sym state)
    {
        if (!ChefZ_SymbolTable.IsValid(state))
            return false;
        return forbiddenStates.Find(state) >= 0;
    }

    //! Spezifitaetsbeitrag der Policy (09 §4.1).
    float SpecificityBonus(notnull ChefZ_PriorityWeights w)
    {
        float s = 0.0;
        if (extraItemsMode == ChefZ_ExtraItemsMode.FORBID)
            s = s + w.wPolicyForbid;
        s = s + w.wPolicyPerState * forbiddenStates.Count();
        return s;
    }

    int ConstraintCount()
    {
        int n = forbiddenStates.Count();
        if (extraItemsMode == ChefZ_ExtraItemsMode.FORBID)
            n++;
        if (minMatchedHealth01 > 0.0)
            n++;
        return n;
    }

    string ToDebugString()
    {
        string s = "extra=" + ChefZ_ExtraItemsMode.Name(extraItemsMode);
        if (forbiddenStates.Count() > 0)
            s = s + " ohne=[" + ChefZ_TextList.JoinSymbols(forbiddenStates, ",") + "]";
        if (minMatchedHealth01 > 0.0)
            s = s + " minHealth=" + minMatchedHealth01.ToString();
        return s;
    }
}

//==============================================================================

/**
 * Das Rezept, fertig kompiliert.
 *
 * Die Felder sind in derselben Reihenfolge gruppiert wie in 08 §2, damit ein
 * Leser die Rohform und diese Form nebeneinander legen kann. Angehaengt sind
 * die Felder, die es in der Rohform NICHT gibt und die erst beim Build
 * entstehen: Spezifitaet, Rangzahlen und das Torsymbol des Index.
 */
class ChefZ_CompiledRecipe
{
    //--- Identitaet ----------------------------------------------------------
    ChefZ_Sym recipeSym;
    string    id;
    string    sourceRef;

    //--- WO gilt es ----------------------------------------------------------
    ref array<ref ChefZ_CompiledContext> contexts;

    //--- WAS wird gebraucht --------------------------------------------------
    ref array<ref ChefZ_CompiledSlot>    slots;

    //--- WAS darf sonst noch drin sein ---------------------------------------
    ref ChefZ_CompiledPolicy             policy;

    //--- WANN ist es fertig --------------------------------------------------
    int   completion;                       // ChefZ_Completion.*
    ref array<int> doneStages;              // FoodStageType als int
    float cookSeconds;
    float minTemperature;

    //--- WAS kommt heraus ----------------------------------------------------
    //
    // Bewusst in ROHFORM uebernommen. Ein ChefZ_OutputDef besteht aus
    // Klassennamen, Mengen und Schaltern; nichts davon braucht ein Symbol oder
    // einen Index, und der Applicator (4_World, S8) arbeitet ohnehin mit
    // Klassennamen. Eine kompilierte Zwischenform waere hier eine Kopie ohne
    // Gewinn - und eine zweite Stelle, an der ein Feld vergessen werden kann.
    ref array<ref ChefZ_OutputDef>       outputs;
    ref array<ref ChefZ_OutputDef>       byproducts;

    //--- WIE gut ist es ------------------------------------------------------
    //! Ebenfalls Rohform: ausgewertet wird sie vom Quality Manager (S10), und
    //! der kompiliert die enthaltenen Selektoren, wenn er gebaut wird.
    ref array<ref ChefZ_GradeRule>       gradeRules;
    ChefZ_Sym qualityTierSet;
    float     qualityBias;

    //--- WER darf es ---------------------------------------------------------
    /**
     * Rohform. Seit S13 ausgewertet, und zwar an drei getrennten Stellen mit
     * drei verschiedenen Wirkungen (17 §3.3, 12 E8):
     *
     *   onFail "block"        -> ChefZ_CapabilityGate im Rezeptfilter (2c).
     *                            Der Kandidat faellt aus, ChefZ faellt auf
     *                            Vanilla zurueck.
     *   onFail "degrade"      -> ChefZ_QualityManager.DegradeTier nach der
     *                            Punktrechnung. Das Gericht entsteht, nur
     *                            schlechter.
     *   onFail "reduceYield"  -> Ausbeutefaktor (S16, Portionen).
     *
     * Ohne registrierten Anbieter blockiert nichts davon: der Config-Default
     * entscheidet, und der Server ist voll spielbar (17 §9).
     */
    ref array<ref ChefZ_CapabilityReq>   requires;

    //--- WERKZEUG ------------------------------------------------------------
    ref array<ChefZ_Sym> requiredToolGroups;

    //--- Weitergabe ----------------------------------------------------------
    int   priority;
    ref array<string> effects;
    ref array<string> emitEvents;
    float nutritionModifier;

    //==========================================================================
    // Erst beim Build berechnet (09 §4.1, 08 §5.1)
    //==========================================================================

    //! Spezifitaet nach 09 §4.1. Haengt NUR vom Rezept ab, nie vom Gefaess -
    //! deshalb einmal beim Boot und nie wieder (09 E2).
    float specificity;

    int   requiredSlots;
    int   totalConstraints;

    //! Wie viele Items muessen mindestens im Gefaess liegen, damit dieses
    //! Rezept ueberhaupt binden kann (08 §5.1, minItemCount).
    int   minItemCount;

    //--- Torsymbol des invertierten Index (siehe Kopf) ------------------------
    //
    // gateKind ist ChefZ_GateKind.* und ausdruecklich NICHT ChefZ_SelectorOp:
    // ein Tor kennt nur die vier Blattarten, die sich gegen den Gefaessinhalt
    // in einem Schritt pruefen lassen. Die uebrigen Operatoren (anyOf, not,
    // Wertebereiche) haben kein Tor, und das als "Operator ohne Tor" zu
    // fuehren waere eine Aufzaehlung mit sechs unbenutzten Werten.
    int       gateKind;
    ChefZ_Sym gateSym;
    int       gateBit;      // nur fuer ChefZ_GateKind.CATEGORY
    int       gateHint;     // selectivityHint des Torslots, fuer die Diagnose

    void ChefZ_CompiledRecipe()
    {
        recipeSym          = ChefZ_SymbolTable.INVALID;
        id                 = "";
        sourceRef          = "";

        contexts           = new array<ref ChefZ_CompiledContext>();
        slots              = new array<ref ChefZ_CompiledSlot>();
        policy             = new ChefZ_CompiledPolicy();
        doneStages         = new array<int>();
        outputs            = new array<ref ChefZ_OutputDef>();
        byproducts         = new array<ref ChefZ_OutputDef>();
        gradeRules         = new array<ref ChefZ_GradeRule>();
        requires           = new array<ref ChefZ_CapabilityReq>();
        requiredToolGroups = new array<ChefZ_Sym>();
        effects            = new array<string>();
        emitEvents         = new array<string>();

        completion         = ChefZ_Completion.ON_STAGE;
        cookSeconds        = 0.0;
        minTemperature     = 0.0;
        qualityTierSet     = ChefZ_SymbolTable.INVALID;
        qualityBias        = 0.0;
        priority           = 0;
        nutritionModifier  = 1.0;

        specificity        = 0.0;
        requiredSlots      = 0;
        totalConstraints   = 0;
        minItemCount       = 0;

        gateKind           = ChefZ_GateKind.NONE;
        gateSym            = ChefZ_SymbolTable.INVALID;
        gateBit            = -1;
        gateHint           = 0;
    }

    //==========================================================================
    // Auskuenfte
    //==========================================================================

    //! Pflicht ist ein Slot, der nicht optional ist UND mindestens ein Item
    //! verlangt - dieselbe Lesart wie im Matcher (07 §4).
    static bool IsRequiredSlot(notnull ChefZ_CompiledSlot slot)
    {
        return !slot.optional && slot.minCount > 0;
    }

    /**
     * Passt irgendeine Kontextregel? reason traegt bei false die Begruendung
     * der ERSTEN Regel - bei mehreren Regeln ist das die haeufigste Absicht
     * des Autors und die nuetzlichste Meldung.
     */
    bool MatchesContext(notnull ChefZ_CookContext ctx, out string reason)
    {
        reason = "";
        if (contexts.Count() == 0)
        {
            // Kann nicht vorkommen: 08 §8 weist Rezepte ohne contexts beim
            // Laden ab. Die zweite Sicherung steht trotzdem hier, und sie sagt
            // NEIN - ein Rezept ohne Kontext darf nirgends zuenden.
            reason = "Rezept hat keine Kontextregel";
            return false;
        }

        string firstReason = "";
        for (int i = 0; i < contexts.Count(); i++)
        {
            string r = "";
            if (contexts.Get(i).Matches(ctx, r))
                return true;
            if (i == 0)
                firstReason = r;
        }

        reason = firstReason;
        return false;
    }

    //! Schritt 2b aus 08 §4: Werkzeugfilter.
    bool HasRequiredTools(notnull ChefZ_CookContext ctx, out string missing)
    {
        missing = "";
        for (int i = 0; i < requiredToolGroups.Count(); i++)
        {
            ChefZ_Sym group = requiredToolGroups.Get(i);
            if (!ctx.HasToolGroup(group))
            {
                missing = ChefZ_SymbolTable.NameOrMark(group);
                return false;
            }
        }
        return true;
    }

    bool AcceptsDoneStage(int vanillaFoodStage)
    {
        if (doneStages.Count() == 0)
            return false;
        return doneStages.Find(vanillaFoodStage) >= 0;
    }

    ChefZ_CompiledSlot FindSlot(string slotId)
    {
        for (int i = 0; i < slots.Count(); i++)
        {
            if (slots.Get(i).slotId == slotId)
                return slots.Get(i);
        }
        return null;
    }

    //! Alle Geraetesymbole, unter denen dieses Rezept im Index steht. Leer
    //! heisst "an jedem Geraet" (siehe ChefZ_CompiledContext.MatchesDevice).
    void CollectDeviceKeys(notnull array<ChefZ_Sym> outClasses,
                           notnull array<ChefZ_Sym> outCategories)
    {
        outClasses.Clear();
        outCategories.Clear();

        for (int i = 0; i < contexts.Count(); i++)
        {
            ChefZ_CompiledContext c = contexts.Get(i);
            AppendUnique(outClasses, c.deviceClasses);
            AppendUnique(outCategories, c.deviceCategories);
        }
    }

    //! true, wenn MINDESTENS EINE Kontextregel an kein Geraet gebunden ist.
    //! Solche Rezepte muessen bei jeder Auswertung Kandidat sein, egal welches
    //! Geraet gerade kocht.
    bool HasUnboundDeviceContext()
    {
        for (int i = 0; i < contexts.Count(); i++)
        {
            ChefZ_CompiledContext c = contexts.Get(i);
            if (c.deviceClasses.Count() == 0 && c.deviceCategories.Count() == 0)
                return true;
        }
        return false;
    }

    private void AppendUnique(notnull array<ChefZ_Sym> dst, array<ChefZ_Sym> src)
    {
        if (!src)
            return;
        for (int i = 0; i < src.Count(); i++)
        {
            ChefZ_Sym sym = src.Get(i);
            if (dst.Find(sym) < 0)
                dst.Insert(sym);
        }
    }

    string ToDebugString()
    {
        string s = id
                 + "  spez=" + specificity.ToString()
                 + " prio=" + priority.ToString()
                 + " slots=" + requiredSlots.ToString() + "/" + slots.Count().ToString()
                 + " minItems=" + minItemCount.ToString()
                 + " abschluss=" + ChefZ_Completion.Name(completion);
        if (gateKind != ChefZ_GateKind.NONE)
            s = s + " tor=" + ChefZ_SymbolTable.NameOrMark(gateSym);
        return s;
    }
}
