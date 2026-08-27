//==============================================================================
// ChefZ_CookSession - was der Adapter sich ueber ein Gefaess merkt
//
// Entwurf: 10 §4 (Feldliste woertlich), 10 §5 (Zustandsuebergaenge), 10 §6
// (Abschlussmodi), 10 §7 (Zustandstabelle), 10 E5 (Fingerprint), 10 E6
// (SUPPRESSED nach drei Fehlversuchen).
//
// ---------------------------------------------------------------------------
// Die wichtigste Eigenschaft: ChefZ hat keinen Kochzustand
// ---------------------------------------------------------------------------
// 10 §7 sagt es so deutlich, dass es hier wiederholt gehoert:
//
//     "Der Adapter schreibt keinen eigenen persistenten Zustand und legt kein
//      einziges neues Sync-Feld an einem Kochgeraet an. Ein halb gekochtes
//      ChefZ-Gericht ist einfach ein Topf mit Zutaten in Vanilla-FoodStages -
//      nach dem Neustart matcht es wieder oder nicht."
//
// Diese Klasse ist reine Laufzeit. Sie wird nie gespeichert, nie
// synchronisiert und ueberlebt keinen Serverneustart. Der Fortschritt eines
// ON_STAGE-Rezepts - des Standardfalls - steckt vollstaendig in Vanillas
// m_CookingTime und m_FoodStageType der Zutaten und wird von Vanilla
// persistiert und synchronisiert.
//
// Bei completion TIMED geht der Fortschritt beim Neustart verloren. Das ist
// eine bewusste Entscheidung (10 §6) und keine Luecke: die Alternativen waeren
// ein modded class auf Vanilla-Kochgeraeten oder ein unsichtbares Markeritem,
// und beide kosten mehr, als der Sonderfall wert ist.
//
// ---------------------------------------------------------------------------
// Warum die Sitzung KEINE Entity haelt
// ---------------------------------------------------------------------------
// 10 §7 fuehrt Snapshot- und Entity-Puffer ausdruecklich beim Adapter und
// nicht bei der Sitzung. Eine Sitzung ueberdauert viele Ticks; ein
// festgehaltenes ItemBase waere ein Zeiger auf ein Objekt, das die Engine
// jederzeit loeschen darf. Der Bezug laeuft ueber die Netz-ID, und die
// Alterung raeumt auf, was nicht mehr wiederkommt.
//
// ---------------------------------------------------------------------------
// "Tick" heisst in dieser Datei Millisekunde
// ---------------------------------------------------------------------------
// 10 §4 nennt das Feld lastTouchedTick und 10 §7 die Groesse sessionTtlTicks.
// Die Einheit ist hier ausdruecklich die Millisekunde aus TickCount(0), und
// zwar aus einem Grund: ein Kochtick ist kein Serverframe. Er faellt je
// Geraet unterschiedlich oft an, und ein globaler Zaehler ueber alle
// Feuerstellen liefe umso schneller, je mehr Feuerstellen brennen - die
// Lebensdauer einer Sitzung haenge dann daran, was ANDERE Spieler tun.
// Die Wanduhr hat diese Eigenschaft nicht.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_CookSession
{
    /**
     * Startwert von ticksSinceMatch.
     *
     * "Seit sehr langer Zeit kein Match" - damit laeuft die erste Auswertung
     * eines frisch befuellten Topfes SOFORT und nicht erst nach
     * matchThrottleTicks. Die Drosselung soll den Spieler bremsen, der Items
     * im Sekundentakt ein- und auslagert, nicht den, der einmal etwas
     * hineinlegt.
     */
    static const int LONG_AGO = 1 << 20;

    //! Niedrige 32 Bit der Netz-ID des Gefaesses. Zugleich der Map-Schluessel.
    int vesselId;

    //! Hohe 32 Bit derselben ID. Nicht Teil des Schluessels, sondern seine
    //! Absicherung: rollt die Engine eine niedrige Haelfte erneut aus, faellt
    //! das hier auf, statt dass zwei Gefaesse sich eine Sitzung teilen.
    int vesselIdHigh;

    ChefZ_ESessionState state;

    ref ChefZ_VesselSignature signature;

    /**
     * Das gebundene Ergebnis. ref, weil der Adapter es ueber Ticks hinweg
     * haelt und der Matcher sein eigenes Ergebnis beim naechsten Kandidaten
     * zuruecksetzt - eine geteilte Instanz waere ein Ergebnis, das sich
     * hinter dem Ruecken der Sitzung aendert.
     *
     * Kein Zyklus: ChefZ_MatchResult haelt das kompilierte Rezept bewusst OHNE
     * ref (siehe dort), und der Eigentuemer des Rezepts ist die Engine.
     */
    ref ChefZ_MatchResult outcome;

    //! Nur completion TIMED. Sekunden, nicht persistiert (10 §6).
    float elapsedSec;

    //! Inhaltsfingerprint zum Zeitpunkt des Matches (10 E5). Passt er nicht
    //! mehr, faellt elapsedSec auf 0 - sonst koennte man kurz vor Ende die
    //! Zutaten tauschen und die Zeit erben.
    int contentFingerprint;

    //! Kochticks dieses Gefaesses seit dem letzten Vollmatch (Stufe B).
    int ticksSinceMatch;

    //! Fehlversuche seit dem letzten Signaturwechsel. Ab 3 -> SUPPRESSED.
    int failCount;

    //! Zeitmarke des letzten Kochticks in Millisekunden, siehe Kopf.
    int lastTouchedTick;

    void ChefZ_CookSession()
    {
        signature = new ChefZ_VesselSignature();
        vesselId     = 0;
        vesselIdHigh = 0;
        ResetAll();
    }

    //==========================================================================

    /**
     * Vollstaendig zuruecksetzen, inklusive Signatur.
     *
     * Danach ist die Sitzung von einer frisch angelegten nicht zu
     * unterscheiden - die naechste Stufe A misst neu und die naechste Stufe B
     * laeuft ohne Verzoegerung.
     */
    void ResetAll()
    {
        state              = ChefZ_ESessionState.IDLE;
        outcome            = null;
        elapsedSec         = 0.0;
        contentFingerprint = 0;
        ticksSinceMatch    = LONG_AGO;
        failCount          = 0;
        signature.Reset();
    }

    /**
     * Der Inhalt hat sich geaendert: alles Gebundene verwerfen, die neue
     * Signatur uebernehmen.
     *
     * failCount faellt ebenfalls auf 0 - genau das meint 10 E6 mit
     * "SUPPRESSED bis zum naechsten Signaturwechsel". Ein Gefaess, das einmal
     * in eine unglueckliche Konstellation geraten ist, bleibt nicht auf ewig
     * gesperrt; wer den Inhalt anfasst, bekommt einen neuen Versuch.
     *
     * ticksSinceMatch bleibt UNANGETASTET. Es zaehlt Ticks seit dem letzten
     * Vollmatch, und genau daran haengt die Drosselung gegen den Spieler, der
     * Items im Sekundentakt bewegt (10 E4). Wuerde es hier zurueckgesetzt,
     * fuehrte jede Bewegung sofort zu einem neuen Vollmatch - die Drosselung
     * waere abgeschaltet, ohne dass es jemand merkt.
     */
    void AdoptNewContent(notnull ChefZ_VesselSignature fresh)
    {
        state              = ChefZ_ESessionState.IDLE;
        outcome            = null;
        elapsedSec         = 0.0;
        contentFingerprint = 0;
        failCount          = 0;
        signature.CopyFrom(fresh);
    }

    //==========================================================================

    bool IsInert()
    {
        return state == ChefZ_ESessionState.DONE
            || state == ChefZ_ESessionState.SUPPRESSED;
    }

    bool HasBinding()
    {
        return state == ChefZ_ESessionState.MATCHED && outcome != null;
    }

    void Touch(int nowMillis)
    {
        lastTouchedTick = nowMillis;
        if (ticksSinceMatch < LONG_AGO)
            ticksSinceMatch++;
    }

    //! Millisekunden seit dem letzten Kochtick dieses Gefaesses.
    int AgeMillis(int nowMillis)
    {
        int age = nowMillis - lastTouchedTick;
        if (age < 0)
            return 0;               // Uhr umgeschlagen: als "eben" lesen
        return age;
    }

    /**
     * Ein Fehlversuch. true, wenn damit die Grenze erreicht ist (10 E6).
     *
     * Die Grenze kommt vom Aufrufer, weil sie eine Einstellung ist und keine
     * Eigenschaft der Sitzung.
     */
    bool Fail(int limit)
    {
        failCount++;
        if (failCount < limit)
            return false;
        state = ChefZ_ESessionState.SUPPRESSED;
        return true;
    }

    //==========================================================================

    static string StateName(ChefZ_ESessionState s)
    {
        switch (s)
        {
            case ChefZ_ESessionState.IDLE:       return "IDLE";
            case ChefZ_ESessionState.MATCHED:    return "MATCHED";
            case ChefZ_ESessionState.COMPLETING: return "COMPLETING";
            case ChefZ_ESessionState.DONE:       return "DONE";
            case ChefZ_ESessionState.SUPPRESSED: return "SUPPRESSED";
        }
        return "?";
    }

    string ToDebugString()
    {
        string s = "Gefaess " + vesselId.ToString()
                 + "  " + StateName(state)
                 + "  seitMatch=" + ticksSinceMatch.ToString();

        if (outcome)
            s = s + "  rezept=" + outcome.recipeId;
        if (elapsedSec > 0.0)
            s = s + "  zeit=" + elapsedSec.ToString() + "s";
        if (failCount > 0)
            s = s + "  fehler=" + failCount.ToString();

        return s + "  [" + signature.ToDebugString() + "]";
    }

    //==========================================================================

    //! Nur fuer den Selbsttest - ohne Gefaess, ohne Spiel.
    static bool SelfCheck()
    {
        ChefZ_CookSession s = new ChefZ_CookSession();
        if (s.state != ChefZ_ESessionState.IDLE)        return false;
        if (s.HasBinding())                             return false;
        if (s.IsInert())                                return false;
        if (s.signature.IsMeasured())                   return false;
        if (s.ticksSinceMatch != LONG_AGO)              return false;

        // Touch zaehlt, laeuft aber nicht ueber.
        s.ticksSinceMatch = 0;
        s.Touch(1000);
        s.Touch(2000);
        if (s.ticksSinceMatch != 2)                     return false;
        if (s.lastTouchedTick != 2000)                  return false;
        if (s.AgeMillis(2500) != 500)                   return false;
        if (s.AgeMillis(1000) != 0)                     return false;   // Uhr umgeschlagen

        // Neuer Inhalt: Bindung faellt, ticksSinceMatch bleibt.
        ChefZ_VesselSignature sig = new ChefZ_VesselSignature();
        sig.BeginMeasure(2, 1, 100.0);
        sig.AddItem(7, 1, -1);
        s.state = ChefZ_ESessionState.MATCHED;
        s.failCount = 2;
        s.elapsedSec = 12.0;
        s.AdoptNewContent(sig);
        if (s.state != ChefZ_ESessionState.IDLE)        return false;
        if (s.failCount != 0)                           return false;
        if (s.elapsedSec != 0.0)                        return false;
        if (s.ticksSinceMatch != 2)                     return false;
        if (!s.signature.Equals(sig))                   return false;

        // Die Signatur ist eine KOPIE, keine geteilte Instanz.
        sig.AddItem(9, 2, -1);
        if (s.signature.Equals(sig))                    return false;

        // Drei Fehlschlaege sperren, ein Signaturwechsel entsperrt.
        if (s.Fail(3))                                  return false;
        if (s.Fail(3))                                  return false;
        if (!s.Fail(3))                                 return false;
        if (s.state != ChefZ_ESessionState.SUPPRESSED)  return false;
        if (!s.IsInert())                               return false;
        s.AdoptNewContent(sig);
        if (s.IsInert())                                return false;

        s.ResetAll();
        if (s.signature.IsMeasured())                   return false;
        if (s.ticksSinceMatch != LONG_AGO)              return false;

        return true;
    }
}
