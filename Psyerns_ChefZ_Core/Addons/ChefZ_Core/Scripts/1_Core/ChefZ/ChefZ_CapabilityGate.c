//==============================================================================
// ChefZ_CapabilityGate - der Platz fuer den Capability-Filter im Rezeptablauf
//
// Entwurf: 08 §7 Schritt 2c ("Capability-Filter requires[] -> block? dann
// Kandidat ueberspringen (17)"), 17 §3.3 (Registry und Regeln), 17 §9
// (Fehlerverhalten, Zeilen zu capabilityMode), 12 E8 ("degrade verschiebt
// Stufen, blockiert nicht").
//
// ---------------------------------------------------------------------------
// Warum diese Klasse ueberhaupt existiert
// ---------------------------------------------------------------------------
// Der Rezeptablauf (ChefZ_RecipeEvaluator) liegt in 1_Core und muss dort
// bleiben: 08 §7 und der Matcher-Selbsttest leben davon, dass die Auswertung
// ohne laufendes Spiel pruefbar ist. Die ChefZ_CapabilityRegistry liegt in
// 3_Game, weil sie Provider aus fremden PBOs fuehrt und Einstellungen liest.
//
// Ein direkter Aufruf waere also ein Schichtbruch nach oben. Diese Klasse ist
// die Umkehrung: 1_Core deklariert, WO gefragt wird, und 3_Game haengt sich
// ein. Dieselbe Bauform wie ChefZ_CapabilityProbe fuer die Qualitaetsregeln
// (12 E6) - und aus demselben Grund.
//
// ---------------------------------------------------------------------------
// Die Vorgabe ist "blockiert nichts"
// ---------------------------------------------------------------------------
// Ohne eingehaengte Ableitung antwortet Denies() immer false. Das ist genau
// das Verhalten, das der Core bis S12 hatte, und genau das Verhalten aus
// 17 §3.3: "Ohne Provider: Default aus ChefZ_CoreSettingsDef. Nie Fehler."
// Ein Selbsttest, ein Servertest ohne Config und ein Server ohne
// Capability-Registry verhalten sich damit identisch.
//
// KEIN CONTENT und ausdruecklich KEIN Fremdsystembezug: hier steht kein
// Skillname, kein Modname und keine Zeichenkette, die zu einem fremden Mod
// gehoert. Was eine Faehigkeit ist, entscheidet der Content ueber
// ChefZ_CapabilityReq.capability; woher ihr Wert kommt, entscheidet ein
// registrierter Provider ausserhalb des Core (17 §2).
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_CapabilityGate
{
    /**
     * Die eingehaengte Auskunftsstelle. Genau eine, prozessweit.
     *
     * ref, weil der Core der einzige Halter ist: die 3_Game-Ableitung wird
     * beim Boot angelegt und lebt so lange wie der Prozess. Ein starker Zyklus
     * entsteht nicht - die Ableitung zeigt auf die Registry, die Registry
     * zeigt nicht hierher zurueck.
     */
    private static ref ChefZ_CapabilityGate s_Active;

    /**
     * Blockiert eine dieser Anforderungen das Rezept?
     *
     * Die Basis kennt NICHTS und antwortet immer "nein". Ableitungen liegen in
     * 3_Game.
     *
     * @param reqs    darf null oder leer sein - dann nie blockiert.
     * @param actorId ctx.actorIdentityId; 0 heisst "niemand beteiligt".
     * @param reason  nur bei true belegt, im Klartext fuer Trace und Log.
     * @return true = Kandidat ueberspringen (08 §7 Schritt 2c).
     *
     * NUR "block" fuehrt hierher. "degrade" und "reduceYield" sind keine
     * Filter, sondern Abwertungen und wirken spaeter - beim Kochen ueber die
     * Qualitaetsstufe (12 E8), bei der Ausbeute ueber yieldFactor. Ein
     * Spieler ohne Faehigkeit bekommt das Gericht, nur schlechter; das ist
     * zugleich die Empfehlung zu OF-08.
     */
    bool BlocksRecipe(array<ref ChefZ_CapabilityReq> reqs, int actorId, out string reason)
    {
        reason = "";
        return false;
    }

    //--------------------------------------------------------------------------

    /**
     * Die Aufrufstelle fuer 1_Core.
     *
     * Ohne eingehaengte Ableitung ein Vergleich gegen null und sonst nichts -
     * das ist der Normalfall auf einem Server ohne Skillmod und darf nichts
     * kosten.
     */
    static bool Denies(array<ref ChefZ_CapabilityReq> reqs, int actorId, out string reason)
    {
        reason = "";

        if (!s_Active)
            return false;
        if (!reqs || reqs.Count() == 0)
            return false;

        return s_Active.BlocksRecipe(reqs, actorId, reason);
    }

    //! Einhaengen. null ist erlaubt und heisst "zurueck auf blockiert nichts".
    static void SetActive(ChefZ_CapabilityGate gate)
    {
        s_Active = gate;
    }

    static bool HasActive()
    {
        if (s_Active)
            return true;
        return false;
    }

    //! Nur fuer den Selbsttest und fuer den SAFE_MODE (02 §8): ein Core, der
    //! sich abschaltet, darf keine Rezepte mehr blockieren.
    static void ClearActive()
    {
        s_Active = null;
    }
}
