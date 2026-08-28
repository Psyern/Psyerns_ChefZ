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

    /**
     * Startwert von claimItemCount: "noch nie gemessen".
     *
     * -1 und nicht 0, aus demselben Grund, aus dem
     * ChefZ_VesselSignature.Reset() itemCount auf -1 setzt: 0 ist ein
     * gueltiger Bestand (ein leeres Gefaess), und ein leeres Gefaess darf
     * nicht wie ein bereits gemessenes aussehen.
     */
    static const int COUNT_UNMEASURED = -1;

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

    //==========================================================================
    // Zuschreibung (siehe Kopf von ChefZ_CookActor)
    //==========================================================================

    /**
     * Wem gehoert das, was hier gerade entsteht? 0 = niemand.
     *
     * Das ist der EINZIGE spielerbezogene Wert des Kochpfads. Er ist reine
     * Laufzeit wie alles in dieser Klasse: nicht gespeichert, nicht
     * synchronisiert, kein Feld an einem Vanilla-Kochgeraet (10 §7, 00 §5).
     * Ein Serverneustart loescht ihn - und das ist richtig so, denn eine
     * Zuschreibung, die einen Neustart ueberlebt, waere eine Behauptung ueber
     * einen Vorgang, den niemand mehr beobachtet hat.
     *
     * Er ueberlebt ausdruecklich AdoptNewContent(): ein Gericht reift ueber
     * viele Signaturwechsel hinweg, und wer es angesetzt hat, verliert es
     * nicht dadurch, dass seine Zutaten gar werden.
     */
    int actorIdentityId;

    /**
     * Bestand beim letzten ausgewerteten Kochtick. COUNT_UNMEASURED, solange
     * noch nie gemessen wurde.
     *
     * Der Vergleich gegen diesen Wert ist die ganze Erkennung: waechst der
     * Bestand, hat ein Spieler etwas eingelegt (siehe Kopf von
     * ChefZ_CookActor). Er ist bewusst NICHT Teil der Signatur - die Signatur
     * beantwortet "hat sich etwas geaendert", dieses Feld beantwortet "ist
     * etwas dazugekommen", und das sind zwei verschiedene Fragen.
     */
    int claimItemCount;

    /**
     * Der naechste Bestandszuwachs stammt von ChefZ selbst.
     *
     * ChefZ_Applicator.SpawnOutput legt das fertige Gericht in das Gefaess -
     * damit waechst der Bestand ohne Zutun eines Spielers, und ohne diese
     * Sperre entstuende genau in dem Tick nach einem Abschluss ein neuer
     * Anspruch fuer jeden, der dann allein am Feuer steht. Die Sperre gilt
     * fuer genau einen Tick; sie wird bei der naechsten Messung verbraucht,
     * gleich ob der Bestand tatsaechlich gewachsen ist.
     */
    bool claimSelfInflicted;

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

        ForgetClaim();
    }

    /**
     * Die Zuschreibung vergessen - Anspruch, Bestandsmarke und Sperre.
     *
     * Zwei Aufrufer, und beide meinen dasselbe: "was hier war, ist vorbei".
     *
     *   1. ResetAll() - die Sitzung beginnt von vorn (Alterung,
     *      Serverneustart, frisches Gefaess).
     *   2. Der Adapter, sobald das Gefaess unter die Mindestzutatenzahl
     *      faellt und damit aus seinem Blickfeld verschwindet.
     *
     * Der zweite Fall ist der wichtigere, und er ist eine Missbrauchssperre:
     * ohne ihn bliebe die Bestandsmarke auf dem alten Stand stehen, waehrend
     * der Adapter nicht hinsieht. Wer danach ein LEERES Gefaess mit eigenen
     * Zutaten fuellt, ohne den alten Stand zu ueberschreiten, arbeitete auf
     * den Anspruch seines Vorgaengers ein. Mit COUNT_UNMEASURED beginnt die
     * Zaehlung neu, und die erste eingelegte Zutat eroeffnet einen neuen
     * Anspruch - fuer den, der sie eingelegt hat.
     */
    void ForgetClaim()
    {
        actorIdentityId    = 0;
        claimItemCount     = COUNT_UNMEASURED;
        claimSelfInflicted = false;
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
     *
     * actorIdentityId, claimItemCount und claimSelfInflicted bleiben aus
     * demselben Grund unangetastet: sie gehoeren nicht zum INHALT, sondern zur
     * Frage, wer ihn dorthin gelegt hat. Ein Signaturwechsel ist meistens
     * blosses Garen - der Anspruch darf davon nicht abhaengen, und der
     * Bestandsvergleich muss ueber den Wechsel hinweg gueltig bleiben.
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

    /**
     * Den Bestand dieses Ticks eintragen und melden, ob er GEWACHSEN ist.
     *
     * Nur bei true wird ein Anspruch neu aufgeloest. Alles andere - gleich
     * geblieben, geschrumpft, von ChefZ selbst erzeugt - laesst den
     * bestehenden Anspruch unberuehrt.
     *
     * Die Sperre claimSelfInflicted wird IMMER verbraucht, auch wenn der
     * Bestand gar nicht gewachsen ist. Sonst bliebe sie liegen und
     * verschluckte irgendwann einen echten Zuwachs - eine Sperre mit
     * unbestimmter Lebensdauer ist keine Sperre, sondern ein Leck.
     *
     * @param currentItemCount Bestand aus der Signatur dieses Ticks.
     * @return true = ein Spieler hat etwas eingelegt.
     */
    bool ObserveItemCount(int currentItemCount)
    {
        bool grew = currentItemCount > claimItemCount && currentItemCount > 0;
        claimItemCount = currentItemCount;

        if (claimSelfInflicted)
        {
            claimSelfInflicted = false;
            return false;
        }

        return grew;
    }

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
        string s = "Gefaess " + vesselId.ToString() + "  " + StateName(state)
                 + "  seitMatch=" + ticksSinceMatch.ToString();

        if (outcome)
            s = s + "  rezept=" + outcome.recipeId;
        if (elapsedSec > 0.0)
            s = s + "  zeit=" + elapsedSec.ToString() + "s";
        if (failCount > 0)
            s = s + "  fehler=" + failCount.ToString();
        if (actorIdentityId != 0)
            s = s + "  koch=" + actorIdentityId.ToString();

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

        //--- Zuschreibung -----------------------------------------------------
        ChefZ_CookSession t = new ChefZ_CookSession();
        if (t.actorIdentityId != 0)                     return false;
        if (t.claimItemCount != COUNT_UNMEASURED)       return false;
        if (t.claimSelfInflicted)                       return false;

        // Ein leeres Gefaess ist KEIN Zuwachs - sonst bekaeme, wer neben
        // einem leeren Topf steht, einen Anspruch auf nichts.
        if (t.ObserveItemCount(0))                      return false;
        if (t.claimItemCount != 0)                      return false;

        // Der erste echte Inhalt ist ein Zuwachs.
        if (!t.ObserveItemCount(3))                     return false;

        // Derselbe Bestand ist keiner - das ist der Normalfall ueber viele
        // Ticks, waehrend das Essen gart.
        if (t.ObserveItemCount(3))                      return false;

        // Ein SCHRUMPFENDER Bestand ist keiner. Vanilla laesst Zutaten bei
        // Quantity 0 verschwinden; das darf keinen Anspruch eroeffnen.
        if (t.ObserveItemCount(2))                      return false;
        if (t.claimItemCount != 2)                      return false;

        // Danach zaehlt gegen den GESCHRUMPFTEN Bestand, nicht gegen den
        // hoechsten je gesehenen.
        if (!t.ObserveItemCount(3))                     return false;

        // Die Sperre nach einem Abschluss schluckt genau einen Tick.
        t.claimSelfInflicted = true;
        if (t.ObserveItemCount(9))                      return false;   // geschluckt
        if (t.claimSelfInflicted)                       return false;   // und verbraucht
        if (t.claimItemCount != 9)                      return false;   // aber mitgezaehlt
        if (!t.ObserveItemCount(10))                    return false;   // danach wieder scharf

        // Die Sperre wird auch dann verbraucht, wenn nichts gewachsen ist -
        // sonst bliebe sie liegen und traefe einen spaeteren echten Zuwachs.
        t.claimSelfInflicted = true;
        if (t.ObserveItemCount(4))                      return false;
        if (t.claimSelfInflicted)                       return false;
        if (!t.ObserveItemCount(5))                     return false;

        // Der Anspruch ueberlebt einen Inhaltswechsel ...
        t.actorIdentityId = 4711;
        t.AdoptNewContent(sig);
        if (t.actorIdentityId != 4711)                  return false;
        if (t.claimItemCount != 5)                      return false;

        // ... und faellt bei ForgetClaim. Danach ist der naechste Bestand,
        // wie klein auch immer, wieder ein Zuwachs - genau das schliesst die
        // Uebernahme eines fremden Anspruchs an einem geleerten Gefaess aus.
        t.claimItemCount = 12;
        t.ForgetClaim();
        if (t.actorIdentityId != 0)                     return false;
        if (t.claimItemCount != COUNT_UNMEASURED)       return false;
        if (!t.ObserveItemCount(1))                     return false;

        // ResetAll schliesst ForgetClaim ein.
        t.actorIdentityId = 4711;
        t.ResetAll();
        if (t.actorIdentityId != 0)                     return false;
        if (t.claimItemCount != COUNT_UNMEASURED)       return false;

        return true;
    }
}
