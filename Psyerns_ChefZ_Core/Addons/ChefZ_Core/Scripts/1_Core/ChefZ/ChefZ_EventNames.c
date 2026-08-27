//==============================================================================
// ChefZ_EventNames / ChefZ_ProgressKind - die Namen, die der Core selbst
//                                         ausloest
//
// Entwurf: 17 §4 (Ereigniskatalog), 17 E1 (String-benannte Ereignisse statt
// eines Enums), 17 E5 (nur wenige Ereignisse sind stornierbar), 17 E6 (genau
// ein Abfrage-Event), 17 E7 (der Core vergibt kein XP).
//
// ---------------------------------------------------------------------------
// Warum das KEINE geschlossene Liste ist
// ---------------------------------------------------------------------------
// 17 §4: "Der Core loest diese Namen aus. Er hat davon KEINE feste Liste im
// Code - die Namen kommen aus den Records (emitEvents an Rezept und Prozess)
// plus einer Handvoll Systemereignisse."
//
// Diese Klasse ist genau diese Handvoll, und sie ist ausdruecklich KEIN Enum
// und keine Zulassungsliste. ChefZ_EventBus.Subscribe() nimmt jeden String an,
// auch einen, der hier nicht steht (17 §9, Zeile "Unbekannte eventId bei
// Subscribe"). Ein Content-Modul, das "ChefZFremd_OnEtwasPassiert" ueber
// emitEvents ausloest, braucht hier keine Zeile - genau deshalb hat 17 E1 den
// String gewaehlt und das Enum verworfen.
//
// Die Konstanten stehen trotzdem hier, und zwar aus zwei Gruenden:
//   1. Ein Tippfehler im Core selbst waere sonst ein stilles Ereignis, das
//      niemand empfaengt. Ein Tippfehler in einem FREMDEN Mod soll dagegen
//      sichtbar bleiben - deshalb wird er gewarnt, nicht abgewiesen.
//   2. IsCancellable() ist Teil der oeffentlichen API (17 §3.2). Die Antwort
//      darauf IST eine feste Entscheidung des Core (17 E5) und muss irgendwo
//      stehen.
//
// ---------------------------------------------------------------------------
// Die XP-Spalte aus 17 §4 steht NICHT hier
// ---------------------------------------------------------------------------
// Sie ist keine Laufzeitangabe, sondern eine Bauform: der Core ruft
// ChefZ_ProgressRegistry.Report() ausschliesslich nach einem erfolgreichen
// Abschluss (17 E7, Regel §10.6). Ein Flag "xpTauglich" waere die Einladung,
// den Aufruf doch woanders hinzuschreiben und sich auf das Flag zu verlassen.
// Es gibt ihn schlicht nicht an den falschen Stellen.
//
// KEIN CONTENT: kein Gericht, keine Zutat, keine Station wird hier benannt.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_EventNames
{
    //--- Systemereignisse (17 §4, in der Reihenfolge der Tabelle) -------------

    //! Nach Freeze(). Nicht stornierbar, nicht XP-tauglich.
    static const string CONFIG_LOADED        = "ChefZ_OnConfigLoaded";

    //! Match gefunden, VOR der Anwendung. STORNIERBAR, nicht XP-tauglich.
    static const string RECIPE_MATCHED       = "ChefZ_OnRecipeMatched";

    //! Ergebnis erzeugt UND Zutaten verbraucht. Nicht stornierbar, XP-tauglich.
    static const string RECIPE_COMPLETED     = "ChefZ_OnRecipeCompleted";

    //! Transaktion abgebrochen; Zutaten garantiert unveraendert (Invariante I5).
    static const string RECIPE_FAILED        = "ChefZ_OnRecipeFailed";

    //! Prozess abgeschlossen, jede Ausfuehrungsform. XP-tauglich.
    static const string INGREDIENT_PROCESSED = "ChefZ_OnIngredientProcessed";

    //! Stationsjob begonnen. STORNIERBAR, ausdruecklich NICHT XP-tauglich.
    static const string PROCESS_JOB_STARTED  = "ChefZ_OnProcessJobStarted";

    static const string PROCESS_JOB_CANCELLED = "ChefZ_OnProcessJobCancelled";

    //! ChefZ-Zustand gewechselt, auch per Klassentausch. Das Rueckgrat fuer
    //! Konservierungsquests: Raeuchern, Trocknen, Salzen und Kochen feuern
    //! DASSELBE Ereignis, weil sie durch dieselbe Anwendungsstelle laufen
    //! (17 §4).
    static const string FOOD_STATE_CHANGED   = "ChefZ_OnFoodStateChanged";

    //! Wechsel in einen Zustand mit preserved = 1. XP-tauglich.
    static const string FOOD_PRESERVED       = "ChefZ_OnFoodPreserved";

    //! Gericht VOLLSTAENDIG verzehrt.
    static const string FOOD_CONSUMED        = "ChefZ_OnFoodConsumed";

    //! Tatsaechlicher Uebergang nach ROTTEN.
    static const string FOOD_SPOILED         = "ChefZ_OnFoodSpoiled";

    //! Portion entnommen, Behaelter bereits verbraucht. STORNIERBAR.
    static const string PORTION_TAKEN        = "ChefZ_OnPortionTaken";

    static const string CONTAINER_RETURNED   = "ChefZ_OnContainerReturned";

    //! Rezept erstmals fuer diesen Spieler gelungen. Sitzungsmenge, KEIN
    //! Entdeckungsregister im Core (17 §8).
    static const string RECIPE_DISCOVERED    = "ChefZ_OnRecipeDiscovered";

    /**
     * Das EINZIGE Abfrage-Event des Core (17 §5, E6).
     *
     * Abonnenten addieren auf args.bonusPoints, der Core klemmt auf
     * maxExternalQualityBonus und rechnet damit weiter. Es ist eine Query mit
     * Punktebeitrag, KEIN "setze Qualitaet auf X" - die Hoheit ueber Stufen
     * und Schwellen bleibt beim Core.
     *
     * Jedes weitere Abfrage-Event waere ein Weg, Kernlogik von aussen zu
     * beeinflussen. Wenn ein Modul mehr Einfluss braucht, gehoert das in eine
     * Config-Zahl, nicht in einen Callback (17 E6).
     */
    static const string QUALITY_BONUS_QUERY  = "ChefZ_QualityBonusQuery";

    //--------------------------------------------------------------------------

    /**
     * Stornierbar sind ausschliesslich Ereignisse VOR einer Wirkung (17 E5).
     *
     * Ein stornierbares "Completed" waere eine Falle: die Zutaten sind zu
     * diesem Zeitpunkt bereits verbraucht. Die Trennung "vorher stornierbar,
     * nachher nur melden" macht diesen Missbrauch strukturell unmoeglich -
     * es gibt keinen Codepfad, auf dem ein Abonnent eine bereits erfolgte
     * Wirkung zuruecknehmen koennte.
     *
     * Fuer alles andere setzt der Bus args.cancelled nach dem Aufruf wieder
     * zurueck und warnt EINMAL je Abonnent (17 §9).
     */
    static bool IsCancellable(string eventId)
    {
        return eventId == RECIPE_MATCHED
            || eventId == PROCESS_JOB_STARTED
            || eventId == PORTION_TAKEN;
    }

    //! Genau eines (17 §5). Der Rueckgabewert dieser Funktion ist die einzige
    //! Stelle, an der der Core einem Abonnenten ueberhaupt zuhoert.
    static bool IsQuery(string eventId)
    {
        return eventId == QUALITY_BONUS_QUERY;
    }

    /**
     * Kennt der Core diesen Namen als eigenes Systemereignis?
     *
     * NICHT als Zulassungspruefung gedacht. Der Bus nutzt sie ausschliesslich,
     * um ein Abonnement auf einen unbekannten Namen zu WARNEN und trotzdem
     * anzunehmen (17 §9): ein Tippfehler soll sichtbar sein, aber ein
     * kuenftiges Event aus einem neueren Core vorab abonnierbar bleiben - und
     * ein Content-Event aus emitEvents ist hier per Definition unbekannt.
     */
    static bool IsCoreEvent(string eventId)
    {
        return eventId == CONFIG_LOADED
            || eventId == RECIPE_MATCHED
            || eventId == RECIPE_COMPLETED
            || eventId == RECIPE_FAILED
            || eventId == INGREDIENT_PROCESSED
            || eventId == PROCESS_JOB_STARTED
            || eventId == PROCESS_JOB_CANCELLED
            || eventId == FOOD_STATE_CHANGED
            || eventId == FOOD_PRESERVED
            || eventId == FOOD_CONSUMED
            || eventId == FOOD_SPOILED
            || eventId == PORTION_TAKEN
            || eventId == CONTAINER_RETURNED
            || eventId == RECIPE_DISCOVERED
            || eventId == QUALITY_BONUS_QUERY;
    }

    //! Fuer Fehlermeldungen und "chefz events". Ausdruecklich unvollstaendig -
    //! Content-Ereignisse aus emitEvents stehen hier nie.
    static string CoreEventNames()
    {
        return CONFIG_LOADED + ", " + RECIPE_MATCHED + ", " + RECIPE_COMPLETED + ", "
             + RECIPE_FAILED + ", " + INGREDIENT_PROCESSED + ", " + PROCESS_JOB_STARTED + ", "
             + PROCESS_JOB_CANCELLED + ", " + FOOD_STATE_CHANGED + ", " + FOOD_PRESERVED + ", "
             + FOOD_CONSUMED + ", " + FOOD_SPOILED + ", " + PORTION_TAKEN + ", "
             + CONTAINER_RETURNED + ", " + RECIPE_DISCOVERED + ", " + QUALITY_BONUS_QUERY;
    }

    //! Nur fuer den Selbsttest (S13).
    static bool SelfCheck()
    {
        // Die drei stornierbaren, und AUSSCHLIESSLICH diese drei (17 E5).
        if (!IsCancellable(RECIPE_MATCHED))          return false;
        if (!IsCancellable(PROCESS_JOB_STARTED))     return false;
        if (!IsCancellable(PORTION_TAKEN))           return false;

        // Die Gegenprobe ist die eigentlich wichtige: waere eines der
        // Abschlussereignisse stornierbar, koennte ein fremder Mod einen
        // bereits vollzogenen Verbrauch "zurueckweisen" und der Core wuerde
        // ihm glauben.
        if (IsCancellable(RECIPE_COMPLETED))         return false;
        if (IsCancellable(RECIPE_FAILED))            return false;
        if (IsCancellable(INGREDIENT_PROCESSED))     return false;
        if (IsCancellable(FOOD_STATE_CHANGED))       return false;
        if (IsCancellable(FOOD_PRESERVED))           return false;
        if (IsCancellable(FOOD_CONSUMED))            return false;
        if (IsCancellable(FOOD_SPOILED))             return false;
        if (IsCancellable(CONFIG_LOADED))            return false;
        if (IsCancellable(CONTAINER_RETURNED))       return false;
        if (IsCancellable(RECIPE_DISCOVERED))        return false;
        if (IsCancellable(PROCESS_JOB_CANCELLED))    return false;
        if (IsCancellable(QUALITY_BONUS_QUERY))      return false;
        if (IsCancellable("ChefZFremd_OnEtwasPassiert")) return false;

        // Genau EIN Abfrage-Event (17 E6).
        if (!IsQuery(QUALITY_BONUS_QUERY))           return false;
        if (IsQuery(RECIPE_MATCHED))                 return false;
        if (IsQuery(RECIPE_COMPLETED))               return false;
        if (IsQuery("ChefZFremd_OnEtwasPassiert"))   return false;

        if (!IsCoreEvent(FOOD_SPOILED))              return false;
        if (IsCoreEvent("ChefZFremd_OnEtwasPassiert")) return false;
        if (IsCoreEvent(""))                         return false;

        return true;
    }
}

//==============================================================================

/**
 * Die Anlaesse, zu denen der Core einen ABSCHLUSS meldet (17 §3.4, E7).
 *
 * Jeder dieser Namen bezeichnet etwas, das FERTIG ist. Es gibt bewusst kein
 * "started", kein "inserted" und kein "attempted": Regel §10.6 ("XP nur nach
 * erfolgreichem Abschluss") ist damit nicht versprochen, sondern gebaut - ein
 * Comp-Modul kann fuer das blosse Einlegen einer Zutat kein XP vergeben, weil
 * es dafuer keinen Aufruf gibt.
 *
 * ChefZ_ProgressRegistry.Report() nimmt jeden String; die Konstanten sind
 * gegen Tippfehler im Core, nicht gegen fremde Namen.
 *
 * Layer: 1_Core.
 */
class ChefZ_ProgressKind
{
    //! Ein Gericht ist entstanden UND die Zutaten sind verbraucht.
    static const string COOK     = "cook";

    //! Ein Verarbeitungsschritt ist abgeschlossen.
    static const string PROCESS  = "process";

    //! Ein Lebensmittel ist in einen Zustand mit preserved = 1 gewechselt.
    static const string PRESERVE = "preserve";

    //! Ein Gericht wurde vollstaendig verzehrt.
    static const string CONSUME  = "consume";

    //! Ein Rezept ist diesem Spieler in dieser Sitzung erstmals gelungen.
    static const string DISCOVER = "discover";

    static string ValidNames()
    {
        return COOK + ", " + PROCESS + ", " + PRESERVE + ", " + CONSUME + ", " + DISCOVER;
    }

    //! Nur fuer den Selbsttest (S13).
    static bool SelfCheck()
    {
        if (COOK != "cook")         return false;
        if (PROCESS != "process")   return false;
        if (PRESERVE != "preserve") return false;
        if (CONSUME != "consume")   return false;
        if (DISCOVER != "discover") return false;
        return true;
    }
}
