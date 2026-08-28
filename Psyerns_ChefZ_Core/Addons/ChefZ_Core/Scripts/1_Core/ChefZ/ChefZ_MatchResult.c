//==============================================================================
// ChefZ_MatchResult / ChefZ_SlotShortfall / ChefZ_PartialMatchReport
//
// Entwurf: 08 §3 (Feldliste woertlich), 08 §4 (was wann gefuellt wird),
// 08 §6 (was der Applicator daraus braucht), 09 §4.4 (Tiebreak zur Laufzeit),
// 18 §3 (was im Trace steht).
//
// ---------------------------------------------------------------------------
// Das Ergebnis ist eine ANSAGE, keine Handlung
// ---------------------------------------------------------------------------
// Ein ChefZ_MatchResult beschreibt vollstaendig, was geschehen SOLL - und
// veraendert dabei nichts. Es entsteht in 1_Core ohne jeden Zugriff auf ein
// ItemBase; ausgefuehrt wird es erst vom ChefZ_Applicator (4_World, S8), und
// der revalidiert zuvor jeden Handle (08 §6, Schritt 1).
//
// Diese Trennung ist der Grund, warum zwischen S7 und S8 ein Fenster steht, in
// dem der komplette Entscheidungspfad auf echten Feuerstellen beobachtbar ist,
// waehrend ChefZ garantiert nichts veraendern KANN (19 S7).
//
// ---------------------------------------------------------------------------
// Felder ueber 08 §3 hinaus, und warum
// ---------------------------------------------------------------------------
// 09 §4.4 verlangt einen Tiebreak ueber "mehr gebundene Items", "mehr
// Pflicht-Slots", "hoehere explizite priority" und "lexikografisch kleinere
// id". Keine dieser vier Zahlen ist aus der Feldliste in 08 §3 ablesbar, und
// ChefZ_RecipeRanker.CompareMatches() darf zur Laufzeit nicht in die Registry
// greifen, um sie nachzuschlagen - das waere eine map-Iteration im heissen
// Pfad, und 09 §5 verbietet sie ausdruecklich.
//
// Deshalb traegt das Ergebnis sie mit: boundItemCount, requiredSlots,
// priority, recipeId. Sie werden beim Fuellen einmal kopiert und sind danach
// unveraenderlich. Dazu kommen recipe (der Applicator braucht die Outputs) und
// boundHandles (er braucht sie zum Revalidieren).
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_MatchResult
{
    //--- Ergebnis ------------------------------------------------------------
    bool      matched;

    //! Abschlussbedingung erfuellt? Wird getrennt gefuehrt, weil ein gebundenes
    //! Rezept ueber viele Ticks hinweg "gematcht, aber noch nicht fertig" ist -
    //! genau das ist der Normalfall bei completion ON_STAGE (10 §6).
    bool      ready;
    string    notReadyReason;

    ChefZ_Sym recipeSym;
    string    recipeId;

    //! Das kompilierte Rezept. OHNE ref: Eigentuemer ist die
    //! ChefZ_RecipeEngine, und die lebt laenger als jedes Ergebnis. Ein ref
    //! waere hier ein Zyklus in Wartestellung.
    ChefZ_CompiledRecipe recipe;

    //--- Bewertung (09 §4.2) -------------------------------------------------
    float     score;
    float     gradeScore;

    //! Qualitaetsstufe. Bleibt INVALID, bis der Quality Manager (S10) sie
    //! setzt - bewusst leer statt halb geraten.
    ChefZ_Sym qualityTier;

    //--- Bindung -------------------------------------------------------------
    //! slotId -> Handles, in DEKLARATIONSreihenfolge der Slots (07 §4).
    ref map<string, ref array<int>> assignment;

    //! Deklarationsindizes der optionalen Slots, die gebunden werden konnten.
    ref array<int>  matchedOptionalSlots;

    //! Alle gebundenen Handles, in Slotreihenfolge. Der Applicator prueft sie
    //! vor der Anwendung erneut (08 §6, Schritt 1).
    ref array<int>  boundHandles;

    //! Was kein Slot gebunden hat. Bei extraItems "forbid" ist diese Liste
    //! leer, sonst waere gar nicht gematcht worden.
    ref array<int>  extraHandles;

    ref array<ref ChefZ_ConsumePlan> consumePlan;

    //--- Zahlen fuer Rang und Tiebreak (09 §4.4) -----------------------------
    int       boundItemCount;
    int       requiredSlots;
    int       priority;
    int       itemsInVessel;

    //--- Diagnose (08 §3) ----------------------------------------------------
    ChefZ_Sym failedRecipe;
    string    failReason;
    string    failSlotId;
    int       nodesExplored;
    int       candidatesTried;

    void ChefZ_MatchResult()
    {
        assignment           = new map<string, ref array<int>>();
        matchedOptionalSlots = new array<int>();
        boundHandles         = new array<int>();
        extraHandles         = new array<int>();
        consumePlan          = new array<ref ChefZ_ConsumePlan>();
        Reset();
    }

    /**
     * Auf "nichts gefunden" zuruecksetzen.
     *
     * Listen und Map werden GELEERT, nicht neu angelegt: 08 §7 fuehrt das
     * Ergebnis ausdruecklich als Objekt "aus einem Pool". Bei dutzenden
     * gleichzeitig kochenden Feuerstellen ist die Neuallokation je Tick genau
     * der Aufwand, den dieser Entwurf nicht haben will.
     */
    void Reset()
    {
        matched         = false;
        ready           = false;
        notReadyReason  = "";
        recipeSym       = ChefZ_SymbolTable.INVALID;
        recipeId        = "";
        recipe          = null;

        score           = 0.0;
        gradeScore      = 0.0;
        qualityTier     = ChefZ_SymbolTable.INVALID;

        assignment.Clear();
        matchedOptionalSlots.Clear();
        boundHandles.Clear();
        extraHandles.Clear();
        consumePlan.Clear();

        boundItemCount  = 0;
        requiredSlots   = 0;
        priority        = 0;
        itemsInVessel   = 0;

        failedRecipe    = ChefZ_SymbolTable.INVALID;
        failReason      = "";
        failSlotId      = "";
        nodesExplored   = 0;
        candidatesTried = 0;
    }

    //==========================================================================
    // Fuellen
    //==========================================================================

    /**
     * Uebernimmt eine Slotbindung des Matchers.
     *
     * handles wird KOPIERT, nicht uebernommen: die Liste im ChefZ_SlotBinding
     * gehoert dem Bindungsergebnis, und das wird beim naechsten Kandidaten
     * zurueckgesetzt. Ein geteiltes Array waere ein Ergebnis, das sich
     * nachtraeglich aendert.
     */
    void SetAssignment(string slotId, notnull array<int> handles)
    {
        array<int> copy = new array<int>();
        for (int i = 0; i < handles.Count(); i++)
            copy.Insert(handles.Get(i));
        assignment.Set(slotId, copy);
    }

    array<int> GetAssignment(string slotId)
    {
        array<int> handles;
        if (assignment.Find(slotId, handles))
            return handles;
        return null;
    }

    bool IsSlotFilled(string slotId)
    {
        array<int> handles = GetAssignment(slotId);
        if (!handles)
            return false;
        return handles.Count() > 0;
    }

    //! Abdeckung nach 09 §4.2: gebundene Items geteilt durch Items im Gefaess.
    //! 0, wenn das Gefaess leer ist - eine Division durch null hat hier keine
    //! sinnvolle Antwort, und 1.0 waere die gefaehrliche (sie belohnte ein
    //! Rezept dafuer, aus nichts etwas zu machen).
    float Coverage()
    {
        if (itemsInVessel <= 0)
            return 0.0;

        // Ueber zwei float-Zwischenwerte und nicht direkt: eine Division
        // zweier int in Enforce ist eine GANZZAHLdivision, und die ergaebe
        // fuer 3 von 5 gebundenen Items den Abdeckungsgrad 0.
        float bound = boundItemCount;
        float total = itemsInVessel;
        return bound / total;
    }

    //==========================================================================

    string ToDebugString()
    {
        if (!matched)
        {
            string f = "kein Treffer";
            if (failReason != "")
                f = f + " (" + failReason;
            if (failSlotId != "")
                f = f + ", Slot " + failSlotId;
            if (failReason != "")
                f = f + ")";
            return f + "  kandidaten=" + candidatesTried.ToString()
                     + " knoten=" + nodesExplored.ToString();
        }

        string s = recipeId
                 + "  score=" + score.ToString()
                 + " punkte=" + gradeScore.ToString()
                 + " items=" + boundItemCount.ToString() + "/" + itemsInVessel.ToString()
                 + " knoten=" + nodesExplored.ToString();
        if (ready)
            s = s + "  FERTIG";
        else
            s = s + "  offen (" + notReadyReason + ")";
        return s;
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_MatchResult r = new ChefZ_MatchResult();
        if (r.matched)                              return false;
        if (r.Coverage() != 0.0)                    return false;

        array<int> handles = new array<int>();
        handles.Insert(7);
        r.SetAssignment("CHEFZ_MR_SLOT", handles);
        handles.Insert(8);                          // darf das Ergebnis NICHT aendern

        array<int> stored = r.GetAssignment("CHEFZ_MR_SLOT");
        if (!stored)                                return false;
        if (stored.Count() != 1)                    return false;
        if (!r.IsSlotFilled("CHEFZ_MR_SLOT"))       return false;
        if (r.IsSlotFilled("CHEFZ_MR_ANDERER"))     return false;

        r.boundItemCount = 3;
        r.itemsInVessel  = 5;
        float cov = r.Coverage();
        if (cov < 0.59 || cov > 0.61)               return false;

        r.Reset();
        if (r.GetAssignment("CHEFZ_MR_SLOT"))       return false;
        if (r.Coverage() != 0.0)                    return false;

        return true;
    }
}

//==============================================================================

/**
 * Was einem Slot fehlt (08 §3, ChefZ_PartialMatchReport).
 *
 * Fuer das Cookbook ab V1.1 und fuer "chefz why" (18 §3): nicht "kein Rezept
 * passt", sondern "dir fehlt noch eines davon".
 */
class ChefZ_SlotShortfall
{
    string slotId;
    bool   satisfied;
    int    have;
    int    need;
    string reason;

    void ChefZ_SlotShortfall()
    {
        slotId    = "";
        satisfied = false;
        have      = 0;
        need      = 0;
        reason    = "";
    }

    string ToLine()
    {
        string s = "    " + slotId + ": ";
        if (satisfied)
            return s + "ok (" + have.ToString() + ")";

        s = s + "fehlt - " + have.ToString() + " von " + need.ToString();
        if (reason != "")
            s = s + " (" + reason + ")";
        return s;
    }
}

//------------------------------------------------------------------------------

/**
 * Warum bindet ein BESTIMMTES Rezept hier nicht (08 §3).
 *
 * Bewusst kein Ergebnis, sondern ein Bericht: er wird gelesen, nie angewandt.
 * Er entsteht ausschliesslich auf Anforderung (Adminkommando, Cookbook) und
 * nie im Kochtick.
 */
class ChefZ_PartialMatchReport
{
    ChefZ_Sym recipeSym;
    string    recipeId;
    bool      contextOk;
    string    contextReason;
    bool      toolsOk;
    string    missingTool;
    ref array<ref ChefZ_SlotShortfall> slots;

    void ChefZ_PartialMatchReport()
    {
        slots = new array<ref ChefZ_SlotShortfall>();
        Reset();
    }

    void Reset()
    {
        recipeSym     = ChefZ_SymbolTable.INVALID;
        recipeId      = "";
        contextOk     = false;
        contextReason = "";
        toolsOk       = true;
        missingTool   = "";
        slots.Clear();
    }

    //! Sind alle Pflichtslots bedienbar? Das ist NICHT dasselbe wie "das
    //! Rezept matcht": die Slots werden hier einzeln betrachtet, ohne
    //! Zuordnung. Zwei Slots, die sich dasselbe Item teilen wollen, sehen
    //! beide erfuellt aus. Der Bericht sagt das ausdruecklich (ToLines).
    bool AllSlotsSatisfied()
    {
        for (int i = 0; i < slots.Count(); i++)
        {
            if (!slots.Get(i).satisfied)
                return false;
        }
        return true;
    }

    void ToLines(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Rezept " + recipeId);

        if (!contextOk)
        {
            outLines.Insert("    Kontext: " + contextReason);
            return;
        }
        outLines.Insert("    Kontext: ok");

        if (!toolsOk)
            outLines.Insert("    Werkzeug fehlt: " + missingTool);

        for (int i = 0; i < slots.Count(); i++)
            outLines.Insert(slots.Get(i).ToLine());

        if (AllSlotsSatisfied() && toolsOk)
        {
            outLines.Insert("    Jeder Slot hat fuer sich genug Kandidaten. Bleibt das " + "Rezept trotzdem aus, teilen sich zwei Slots dieselben Items - ein Item " + "bedient hoechstens einen Slot (07 §4).");
        }
    }
}
