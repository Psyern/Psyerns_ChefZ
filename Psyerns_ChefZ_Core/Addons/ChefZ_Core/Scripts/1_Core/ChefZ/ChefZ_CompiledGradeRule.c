//==============================================================================
// ChefZ_GradeWhen / ChefZ_GradeContextKey / ChefZ_CapabilityProbe /
// ChefZ_CompiledGradeRule - die Qualitaetsregel in Auswertungsform
//
// Entwurf: 12 §3 (ChefZ_GradeRule, Feldliste und die sechs when-Arten),
// 12 §4 (die Regelpunkte sind ein Summand der Punktrechnung), 12 §8
// (Fehlerverhalten beim Build), 12 E1 (additiv und slice-lokal erweiterbar),
// 12 E6 (Capability wirkt als Grade-Regel, nicht als Sonderweg).
//
// ---------------------------------------------------------------------------
// Rohform und Auswertungsform, wie ueberall im Projekt
// ---------------------------------------------------------------------------
//     ChefZ_GradeRule           Strings aus JSON       (ChefZ_RecipeDef.c)
//     ChefZ_CompiledGradeRule   Symbole und Zahlen     (hier)
//
// Der Rezeptcompiler (S6) reicht die Rohform unveraendert an das kompilierte
// Rezept weiter und erklaert im Code ausdruecklich, warum: "Sie gehoeren dem
// Quality Manager, und der kompiliert ihre Selektoren, wenn er gebaut wird."
// Das ist diese Klasse.
//
// ---------------------------------------------------------------------------
// KEIN CONTENT, und zwar strukturell
// ---------------------------------------------------------------------------
// Eine Regel nennt einen Slot, einen Selektor, einen Kontextwert oder eine
// Faehigkeit. Alle vier sind Symbole oder Namen, die aus den Daten kommen -
// hier steht kein "Sauce", kein "CHEFZ_HERB" und kein "CHEFZ_CAP_FIELDCOOK".
// Genau das ist der Punkt von 12 E6: ein Skill gibt PUNKTE wie eine Zutat, und
// damit kann strukturell nie ein Skillname im Core landen.
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ, kein Itemzugriff.
//==============================================================================

/**
 * Die sechs Anlaesse aus 12 §3.
 *
 * Als Konstanten und nicht als enum, aus demselben Grund wie bei
 * ChefZ_RecordKind: der Name steht im JSON und im Trace, und eine unbekannte
 * Art soll sichtbar auffallen statt still auf 0 zu fallen. NONE ist deshalb
 * ein eigener Wert und keine gueltige Regelart.
 */
class ChefZ_GradeWhen
{
    static const int NONE        = 0;
    static const int SLOT_FILLED = 1;   // "slotFilled"
    static const int SLOT_COUNT  = 2;   // "slotCount"
    static const int ANY_ITEM    = 3;   // "anyItem"
    static const int ALL_MATCHED = 4;   // "allMatched"
    static const int CONTEXT     = 5;   // "context"
    static const int CAPABILITY  = 6;   // "capability"

    static int FromName(string name)
    {
        if (name == "slotFilled")   return SLOT_FILLED;
        if (name == "slotCount")    return SLOT_COUNT;
        if (name == "anyItem")      return ANY_ITEM;
        if (name == "allMatched")   return ALL_MATCHED;
        if (name == "context")      return CONTEXT;
        if (name == "capability")   return CAPABILITY;
        return NONE;
    }

    static string Name(int when)
    {
        switch (when)
        {
            case SLOT_FILLED:   return "slotFilled";
            case SLOT_COUNT:    return "slotCount";
            case ANY_ITEM:      return "anyItem";
            case ALL_MATCHED:   return "allMatched";
            case CONTEXT:       return "context";
            case CAPABILITY:    return "capability";
        }
        return "?";
    }

    static string ValidNames()
    {
        return "slotFilled, slotCount, anyItem, allMatched, context, capability";
    }

    //! Braucht diese Art eine slotId?
    static bool NeedsSlot(int when)
    {
        return when == SLOT_FILLED || when == SLOT_COUNT;
    }

    //! Braucht diese Art einen Selektor?
    static bool NeedsSelector(int when)
    {
        return when == ANY_ITEM || when == ALL_MATCHED;
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        if (FromName("slotFilled") != SLOT_FILLED)      return false;
        if (FromName("capability") != CAPABILITY)       return false;
        if (FromName("gibtsNicht") != NONE)             return false;
        if (FromName("") != NONE)                       return false;
        if (Name(SLOT_COUNT) != "slotCount")            return false;
        if (Name(NONE) != "?")                          return false;
        if (!NeedsSlot(SLOT_FILLED))                    return false;
        if (NeedsSlot(ANY_ITEM))                        return false;
        if (!NeedsSelector(ALL_MATCHED))                return false;
        if (NeedsSelector(CONTEXT))                     return false;
        return true;
    }
}

//==============================================================================

/**
 * Die Kontextgroessen, auf die eine Regel mit when: "context" zeigen darf.
 *
 * ---------------------------------------------------------------------------
 * Eine bewusste Ergaenzung des Entwurfs, und warum sie noetig ist
 * ---------------------------------------------------------------------------
 * 12 §3 fuehrt "context" als Regelart auf und gibt ihr einen Wertebereich
 * ("range: gueltiger Wertebereich fuer context/capability"), nennt aber kein
 * Feld, das sagt WELCHE Kontextgroesse gemeint ist. Ohne diese Angabe ist die
 * Art nicht auswertbar - man muesste raten.
 *
 * ChefZ_GradeRule bekommt deshalb ein Feld "contextKey", und diese Klasse ist
 * die Liste der zulaessigen Werte. Sie ist KEIN Content (Invariante I3): jeder
 * Eintrag ist ein Feld von ChefZ_CookContext oder eine Kennzahl des
 * Matchergebnisses. Hier steht die Aussage "es gibt eine Geraetetemperatur" -
 * nicht "es gibt den Gasherd".
 *
 * Die Liste ist absichtlich klein. Jeder Eintrag ist eine Zusage an
 * Content-Autoren, und eine Zusage zurueckzunehmen kostet mehr, als eine
 * spaeter dazuzugeben.
 */
class ChefZ_GradeContextKey
{
    static const int NONE              = 0;
    static const int DEVICE_TEMPERATURE = 1;   // ctx.deviceTemperature
    static const int LIQUID_QUANTITY    = 2;   // ctx.liquidQuantity
    static const int ELAPSED_SEC        = 3;   // ctx.elapsedSec
    static const int PORTION_CAPACITY   = 4;   // ctx.portionCapacity
    static const int QUALITY_MODIFIER   = 5;   // ctx.qualityModifier
    static const int ITEMS_IN_VESSEL    = 6;   // match.itemsInVessel
    static const int BOUND_ITEM_COUNT   = 7;   // match.boundItemCount
    static const int COVERAGE           = 8;   // match.Coverage(), 0..1

    static int FromName(string name)
    {
        if (name == "deviceTemperature") return DEVICE_TEMPERATURE;
        if (name == "liquidQuantity")    return LIQUID_QUANTITY;
        if (name == "elapsedSec")        return ELAPSED_SEC;
        if (name == "portionCapacity")   return PORTION_CAPACITY;
        if (name == "qualityModifier")   return QUALITY_MODIFIER;
        if (name == "itemsInVessel")     return ITEMS_IN_VESSEL;
        if (name == "boundItemCount")    return BOUND_ITEM_COUNT;
        if (name == "coverage")          return COVERAGE;
        return NONE;
    }

    static string Name(int key)
    {
        switch (key)
        {
            case DEVICE_TEMPERATURE: return "deviceTemperature";
            case LIQUID_QUANTITY:    return "liquidQuantity";
            case ELAPSED_SEC:        return "elapsedSec";
            case PORTION_CAPACITY:   return "portionCapacity";
            case QUALITY_MODIFIER:   return "qualityModifier";
            case ITEMS_IN_VESSEL:    return "itemsInVessel";
            case BOUND_ITEM_COUNT:   return "boundItemCount";
            case COVERAGE:           return "coverage";
        }
        return "?";
    }

    static string ValidNames()
    {
        return "deviceTemperature, liquidQuantity, elapsedSec, portionCapacity, " + "qualityModifier, itemsInVessel, boundItemCount, coverage";
    }

    //! Der Wert zur Kennung. 0 fuer NONE - eine Regel mit unbekannter Kennung
    //! wird beim Build verworfen und kommt hier nie an.
    static float Value(int key, notnull ChefZ_CookContext ctx, notnull ChefZ_MatchResult match)
    {
        // Die drei ganzzahligen Groessen ueber eine float-Zwischenvariable:
        // eine implizite Umwandlung im return-Ausdruck ist in Enforce
        // nirgends zugesichert, und ein verschluckter Wert saehe hier aus wie
        // "die Bedingung war eben nicht erfuellt".
        float n;

        switch (key)
        {
            case DEVICE_TEMPERATURE: return ctx.deviceTemperature;
            case LIQUID_QUANTITY:    return ctx.liquidQuantity;
            case ELAPSED_SEC:        return ctx.elapsedSec;
            case QUALITY_MODIFIER:   return ctx.qualityModifier;
            case COVERAGE:           return match.Coverage();

            case PORTION_CAPACITY:
                n = ctx.portionCapacity;
                return n;

            case ITEMS_IN_VESSEL:
                n = match.itemsInVessel;
                return n;

            case BOUND_ITEM_COUNT:
                n = match.boundItemCount;
                return n;
        }
        return 0.0;
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        if (FromName("deviceTemperature") != DEVICE_TEMPERATURE)    return false;
        if (FromName("coverage") != COVERAGE)                       return false;
        if (FromName("gibtsNicht") != NONE)                         return false;
        if (Name(ELAPSED_SEC) != "elapsedSec")                      return false;
        if (Name(NONE) != "?")                                      return false;

        ChefZ_CookContext ctx = new ChefZ_CookContext();
        ctx.deviceTemperature = 150.0;
        ctx.elapsedSec        = 42.0;

        ChefZ_MatchResult m = new ChefZ_MatchResult();
        m.boundItemCount = 3;
        m.itemsInVessel  = 4;

        if (Value(DEVICE_TEMPERATURE, ctx, m) != 150.0)             return false;
        if (Value(ELAPSED_SEC, ctx, m) != 42.0)                     return false;
        if (Value(BOUND_ITEM_COUNT, ctx, m) != 3.0)                 return false;
        if (Value(NONE, ctx, m) != 0.0)                             return false;

        float cov = Value(COVERAGE, ctx, m);
        if (cov < 0.74 || cov > 0.76)                               return false;

        return true;
    }
}

//==============================================================================

/**
 * Die Auskunftsstelle fuer Faehigkeitswerte (12 E6, 17 §3.3).
 *
 * Die Basis kennt NICHTS und antwortet auf jede Frage mit false. Das ist
 * absichtlich und woertlich das Verhalten aus 12 §8: "Capability-Provider
 * fehlt -> Regel gilt als nicht erfuellt (0 Punkte). KEIN FEHLER - ohne
 * Skillmod gibt es eben keinen Skillbonus."
 *
 * Seit S13 haengt ChefZ_ConfigManager eine Ableitung ein, die die
 * registrierten Anbieter befragt (ChefZ_RegistryCapabilityProbe). Sie haelt
 * sich ausdruecklich an DIESEN Vertrag und nicht an den aus 17 §3.3: ohne
 * antwortenden Anbieter liefert sie FALSE, nicht den Config-Default. Sonst
 * bekaeme jeder Spieler auf jedem Server ohne Skillmod denselben Bonus - eine
 * Balancingaussage, die niemand getroffen hat.
 *
 * Ohne Skillmodul ist der Server damit voll spielbar, und der Selbsttest kann
 * die Regelart trotzdem pruefen - er baut sich eine eigene Ableitung.
 *
 * Layer: 1_Core. Die Signatur nennt bewusst KEINEN Spieler, sondern nur die
 * Identitaets-ID aus dem Kochkontext - 1_Core darf kein PlayerBase kennen.
 */
class ChefZ_CapabilityProbe
{
    /**
     * @param capability  Name der Faehigkeit, opaque fuer den Core.
     * @param actorId     ctx.actorIdentityId; 0 heisst "niemand beteiligt".
     * @param value       der Wert, nur bei true belegt.
     * @return false, wenn dieser Anbieter zu der Faehigkeit nichts sagen kann.
     */
    bool TryGetValue(string capability, int actorId, out float value)
    {
        value = 0.0;
        return false;
    }
}

//==============================================================================

/**
 * Eine Qualitaetsregel, fertig kompiliert.
 *
 * Der Selektor ist uebersetzt, die Art ist eine Zahl, und die drei
 * Punktefelder sind aufgeloest: aus dem Sentinel "nicht gesetzt" ist eine
 * Zahl plus ein Merker geworden. Zur Laufzeit kostet die Auswertung damit
 * keinen Stringvergleich und keine Sentinelpruefung.
 */
class ChefZ_CompiledGradeRule
{
    string    ruleId;
    int       when;                 // ChefZ_GradeWhen.*
    string    slotId;
    ref ChefZ_CompiledSelector selector;
    string    capability;           // opaque
    int       contextKey;           // ChefZ_GradeContextKey.*
    ref ChefZ_Range range;          // nie null nach dem Compile

    float     points;
    float     pointsPerItem;
    float     maxPoints;
    bool      hasMaxPoints;

    void ChefZ_CompiledGradeRule()
    {
        ruleId        = "";
        when          = ChefZ_GradeWhen.NONE;
        slotId        = "";
        selector      = null;
        capability    = "";
        contextKey    = ChefZ_GradeContextKey.NONE;
        range         = new ChefZ_Range();
        points        = 0.0;
        pointsPerItem = 0.0;
        maxPoints     = 0.0;
        hasMaxPoints  = false;
    }

    //==========================================================================
    // Auswertung
    //==========================================================================

    /**
     * Die Punkte dieser Regel fuer DIESES Ergebnis.
     *
     * Rein rechnend: kein Itemzugriff, kein Seiteneffekt, keine Allokation
     * ausser der Notiz. Die Fakten kommen aus dem Snapshot, die Bindung aus
     * dem Ergebnis - beide sind zu diesem Zeitpunkt unveraenderlich.
     *
     * @param caps darf null sein. Dann liefert jede capability-Regel 0 Punkte
     *             und sagt das in der Notiz (12 §8).
     * @param note Klartextbegruendung fuer ChefZ_QualityEvaluation.Notes.
     *             Immer belegt, auch bei 0 Punkten - gerade dann.
     */
    float Evaluate(notnull ChefZ_MatchResult match, notnull ChefZ_FactSnapshot snapshot, notnull ChefZ_CookContext ctx, ChefZ_CapabilityProbe caps, out string note)
    {
        note = "";

        switch (when)
        {
            case ChefZ_GradeWhen.SLOT_FILLED:
                return EvalSlotFilled(match, note);

            case ChefZ_GradeWhen.SLOT_COUNT:
                return EvalSlotCount(match, note);

            case ChefZ_GradeWhen.ANY_ITEM:
                return EvalItems(match, snapshot, false, note);

            case ChefZ_GradeWhen.ALL_MATCHED:
                return EvalItems(match, snapshot, true, note);

            case ChefZ_GradeWhen.CONTEXT:
                return EvalContext(match, ctx, note);

            case ChefZ_GradeWhen.CAPABILITY:
                return EvalCapability(ctx, caps, note);
        }

        // Unerreichbar, solange when aus dem Compiler kommt. Die sichere
        // Antwort sind 0 Punkte: eine unbekannte Regelart darf nichts geben.
        note = Label() + ": unbekannte Regelart, 0 Punkte";
        return 0.0;
    }

    //--------------------------------------------------------------------------

    private float EvalSlotFilled(notnull ChefZ_MatchResult match, out string note)
    {
        if (!match.IsSlotFilled(slotId))
        {
            note = Label() + ": Slot \"" + slotId + "\" nicht belegt, 0 Punkte";
            return 0.0;
        }

        float value = Cap(points);
        note = Label() + ": Slot \"" + slotId + "\" belegt, " + value.ToString() + " Punkte";
        return value;
    }

    /**
     * Punkte je gebundenem Item DIESES Slots.
     *
     * pointsPerItem ist die vorgesehene Schreibweise (12 §3). Wer stattdessen
     * "points" schreibt, bekommt sie einmal - nicht null. Das ist keine
     * Grosszuegigkeit, sondern die einzige Lesart, in der ein vergessenes
     * "pointsPerItem" nicht wie ein stiller Ausfall aussieht.
     */
    private float EvalSlotCount(notnull ChefZ_MatchResult match, out string note)
    {
        array<int> handles = match.GetAssignment(slotId);
        int n = 0;
        if (handles)
            n = handles.Count();

        if (n == 0)
        {
            note = Label() + ": Slot \"" + slotId + "\" ohne Zutat, 0 Punkte";
            return 0.0;
        }

        float per = pointsPerItem;
        if (per == 0.0)
            per = points;

        // Zaehler ueber eine float-Zwischenvariable, siehe
        // ChefZ_GradeContextKey.Value().
        float count = n;
        float value = Cap(per * count);
        note = Label() + ": Slot \"" + slotId + "\" mit " + n.ToString() + " Zutat(en) x " + per.ToString() + " = " + value.ToString() + " Punkte";
        return value;
    }

    /**
     * anyItem und allMatched, ueber die GEBUNDENEN Zutaten.
     *
     * Ausdruecklich nicht ueber den ganzen Gefaessinhalt: gewertet wird, was
     * das Rezept tatsaechlich verbraucht. Sonst gaebe ein Kraut, das
     * danebenliegt und gar nicht mitgekocht wird, Punkte - und das waere der
     * naechste Standardexploit gleich nach dem aus 12 §4.1.
     *
     * allMatched auf einem leeren Ergebnis liefert 0 Punkte und nicht "alle
     * erfuellt". Die leere Allaussage ist logisch wahr und hier trotzdem
     * falsch: sie belohnte ein Rezept dafuer, nichts gebunden zu haben.
     */
    private float EvalItems(notnull ChefZ_MatchResult match, notnull ChefZ_FactSnapshot snapshot, bool requireAll, out string note)
    {
        if (!selector)
        {
            note = Label() + ": kein Selektor, 0 Punkte";
            return 0.0;
        }

        int considered = 0;
        int hits = 0;

        for (int i = 0; i < match.boundHandles.Count(); i++)
        {
            ChefZ_ItemFacts facts = snapshot.FindByHandle(match.boundHandles.Get(i));
            if (!facts)
                continue;

            considered++;
            if (selector.Test(facts))
                hits++;
        }

        if (considered == 0)
        {
            note = Label() + ": keine gebundene Zutat, 0 Punkte";
            return 0.0;
        }

        float value;

        if (requireAll)
        {
            if (hits < considered)
            {
                note = Label() + ": nur " + hits.ToString() + " von " + considered.ToString() + " Zutaten erfuellen den Selektor, 0 Punkte";
                return 0.0;
            }

            value = Cap(points);
            note = Label() + ": alle " + considered.ToString() + " Zutaten erfuellen den " + "Selektor, " + value.ToString() + " Punkte";
            return value;
        }

        if (hits == 0)
        {
            note = Label() + ": keine der " + considered.ToString() + " Zutaten erfuellt den Selektor, 0 Punkte";
            return 0.0;
        }

        // Mit pointsPerItem zaehlt jede Treffer-Zutat, sonst zaehlt der
        // Treffer als solcher. Beides ist in 12 §3 belegt: die Beispielregel
        // "wildMeat" arbeitet mit points, "spices" mit pointsPerItem.
        if (pointsPerItem != 0.0)
        {
            float hitCount = hits;
            value = Cap(pointsPerItem * hitCount);
            note = Label() + ": " + hits.ToString() + " Zutat(en) x " + pointsPerItem.ToString() + " = " + value.ToString() + " Punkte";
            return value;
        }

        value = Cap(points);
        note = Label() + ": " + hits.ToString() + " Zutat(en) erfuellen den Selektor, " + value.ToString() + " Punkte";
        return value;
    }

    private float EvalContext(notnull ChefZ_MatchResult match, notnull ChefZ_CookContext ctx, out string note)
    {
        float value = ChefZ_GradeContextKey.Value(contextKey, ctx, match);
        string keyName = ChefZ_GradeContextKey.Name(contextKey);

        if (!range.Contains(value))
        {
            note = Label() + ": " + keyName + " = " + value.ToString() + " liegt nicht in " + range.ToDebugString() + ", 0 Punkte";
            return 0.0;
        }

        float given = Cap(points);
        note = Label() + ": " + keyName + " = " + value.ToString() + " liegt in " + range.ToDebugString() + ", " + given.ToString() + " Punkte";
        return given;
    }

    private float EvalCapability(notnull ChefZ_CookContext ctx, ChefZ_CapabilityProbe caps, out string note)
    {
        if (!caps)
        {
            note = Label() + ": kein Faehigkeitsanbieter registriert, 0 Punkte " + "(kein Fehler - ohne Skillmodul gibt es keinen Skillbonus)";
            return 0.0;
        }

        float value;
        if (!caps.TryGetValue(capability, ctx.actorIdentityId, value))
        {
            note = Label() + ": Faehigkeit \"" + capability + "\" ist unbekannt, 0 Punkte";
            return 0.0;
        }

        if (!range.Contains(value))
        {
            note = Label() + ": Faehigkeit \"" + capability + "\" = " + value.ToString() + " liegt nicht in " + range.ToDebugString() + ", 0 Punkte";
            return 0.0;
        }

        float given = Cap(points);
        note = Label() + ": Faehigkeit \"" + capability + "\" = " + value.ToString() + ", " + given.ToString() + " Punkte";
        return given;
    }

    //--------------------------------------------------------------------------

    /**
     * Deckel aus maxPoints (12 §3).
     *
     * Nur nach OBEN. Eine Regel mit negativen Punkten soll ein maxPoints von 3
     * nicht auf 3 anheben - der Deckel ist eine Obergrenze, keine
     * Normalisierung.
     */
    private float Cap(float value)
    {
        if (hasMaxPoints && value > maxPoints)
            return maxPoints;
        return value;
    }

    private string Label()
    {
        string s = ruleId;
        if (s == "")
            s = "(Regel ohne ruleId)";
        return "Regel " + s;
    }

    string ToDebugString()
    {
        string s = Label() + " " + ChefZ_GradeWhen.Name(when);

        if (slotId != "")
            s = s + " slot=" + slotId;
        if (capability != "")
            s = s + " faehigkeit=" + capability;
        if (contextKey != ChefZ_GradeContextKey.NONE)
            s = s + " kontext=" + ChefZ_GradeContextKey.Name(contextKey);
        if (selector)
            s = s + " " + selector.ToDebugString();
        if (range && !range.IsUnbounded())
            s = s + " bereich=" + range.ToDebugString();

        s = s + " punkte=" + points.ToString();
        if (pointsPerItem != 0.0)
            s = s + " jeItem=" + pointsPerItem.ToString();
        if (hasMaxPoints)
            s = s + " max=" + maxPoints.ToString();

        return s;
    }
}
