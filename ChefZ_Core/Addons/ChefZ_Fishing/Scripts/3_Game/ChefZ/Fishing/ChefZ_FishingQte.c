//==============================================================================
// ChefZ_FishingQte - gemeinsame Konstanten und die Bruecke 4_World <-> 5_Mission
//
// 27.10.2026, Spieltest "Fischen ueberarbeiten wir komplett: Quicktime Events".
//
// Ablauf (Details in ChefZ_FishingQteHooks.c und ChefZ_FishingQteHud.c):
//   1. Vanilla wie gehabt: auswerfen, warten, der Fisch beisst (Signal).
//   2. Wer im Signal reagiert, faengt NICHT sofort - es startet das Minispiel:
//      a) Kreisdiagramm: ein Zeiger laeuft um, dreimal die Zone treffen.
//      b) Pfeiltasten in drei Runden: 4, 8, 12 Tasten, jede mit Zeitring.
//   3. Der Client schickt EIN Ergebnis (0..1). Der Server prueft, ob ueberhaupt
//      ein Minispiel laeuft und wie lange es gedauert hat, und entscheidet:
//      Fang oder entkommen, Menge und ChefZ-Qualitaetsstufe nach Ergebnis.
//
// Diese Datei liegt in 3_Game, weil beide Seiten sie brauchen: 4_World startet
// das Spiel und loest es auf, 5_Mission zeichnet es. 4_World darf 5_Mission
// nicht kennen - deshalb die statische Bruecke statt eines direkten Aufrufs.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_FishingQteRPC
{
    //! Client -> Server: Param1<float> Ergebnis 0..1.
    static const int SCORE  = 10010;
    //! Server -> Client: Param2<int, string> (1 = gefangen, 0 = entkommen; Name des Fangs).
    static const int RESULT = 10011;
}

class ChefZ_FishingQteConst
{
    //! Unter diesem Ergebnis reisst der Fisch ab.
    static const float MIN_SCORE_CATCH = 0.2;

    //! Kein menschliches Minispiel ist schneller - ein kuerzeres Ergebnis gilt als 0.
    static const float MIN_DURATION_SEC = 5.0;

    //! Spaetestens dann loest der Server ohne Ergebnis auf (Ergebnis 0).
    static const float SERVER_TIMEOUT_SEC = 120.0;

    //! Gewichte der beiden Teile im Gesamtergebnis.
    static const float WEIGHT_RING   = 0.3;
    static const float WEIGHT_ARROWS = 0.7;

    //! Fangmenge (vanilla CatchingResult, SetQuantityNormalized) von schlecht bis perfekt.
    static const float QUANTITY_MIN = 0.45;
    static const float QUANTITY_MAX = 1.0;

    //! Qualitaetsstufe aus CfgChefZQualityTiers zum Ergebnis.
    static string QualityTierFor(float score)
    {
        if (score >= 0.95)
            return "PREMIUM";
        if (score >= 0.8)
            return "SEASONED";
        if (score >= 0.6)
            return "PREPARED";
        if (score >= 0.4)
            return "SIMPLE";
        return "POOR";
    }
}

//! Clientseitiger Zustand zwischen 4_World (Start, Ergebnis) und 5_Mission (HUD).
class ChefZ_FishingQteBridge
{
    //! 4_World hat ein Minispiel angefordert; das HUD nimmt es im naechsten Bild auf.
    static bool s_StartPending;

    //! Antwort des Servers ist da: 1 gefangen, 0 entkommen, -1 noch keine.
    static int s_Result = -1;
    static string s_ResultName;

    static void RequestStart()
    {
        s_StartPending = true;
        s_Result = -1;
        s_ResultName = "";
    }

    static void PostResult(int result, string catchName)
    {
        s_Result = result;
        s_ResultName = catchName;
    }
}
