//==============================================================================
// ChefZ_Undefined - Sentinelwerte fuer "im JSON nicht gesetzt"
//
// Entwurf: 02 E3.
//
// Warum es das ueberhaupt gibt: der JsonSerializer fuellt fehlende Felder mit
// dem Typdefault. Damit ist "nicht gesetzt" nicht von "auf 0 gesetzt"
// unterscheidbar - und genau diese Unterscheidung braucht der feldweise Patch
// aus Rang 3 (02 E3). Fuer ref-Typen loest sich das von selbst (abwesend =
// null); Skalare brauchen einen Sentinel.
//
// ---------------------------------------------------------------------------
// WARUM DIE SENTINEL SEIT DEM 28.08.2026 DIE TYPDEFAULTS SIND
// ---------------------------------------------------------------------------
// Vorher standen hier float.LOWEST und int.MIN, gesetzt im Konstruktor jedes
// Records. Das setzt voraus, dass der Konstruktor beim Deserialisieren laeuft -
// und das tut er nicht. Nachgewiesen auf dem Testserver: eine TRACE-Zeile in
// ChefZ_CoreSettingsDef() erschien KEIN EINZIGES MAL, in einem Lauf mit 162
// anderen TRACE-Zeilen, dessen Protokoll das Lesen des Overlays zwei Zeilen
// vorher zeigt.
//
// Die Folge war die genaue Umkehrung des Overlay-Zwecks: jedes Feld, das der
// Betreiber NICHT geschrieben hatte, kam als 0 zurueck und ueberschrieb den
// Wert aus Rang 2. Die mitgelieferte Overlay-Vorlage enthaelt absichtlich nur
// { "id": "CORE" } - und genau sie klemmte safeModeErrorThreshold auf 1 und
// legte den Core in den SAFE MODE.
//
// Also gilt jetzt, was die Engine ohnehin liefert: 0, 0.0 und "" heissen
// "nicht gesetzt". Fuer Text war das immer schon so, und niemand hat es je als
// Einschraenkung empfunden - IsTextUndefined("") ist unveraendert.
//
// Die Zweideutigkeit, die dadurch entsteht - eine AUSDRUECKLICH geschriebene 0
// sieht aus wie eine fehlende - wird nicht ueber den Wert aufgeloest, sondern
// ueber den Text: ChefZ_JsonRecordReader traegt die tatsaechlich geschriebenen
// Schluessel in explicitFields[] ein. Deshalb fragen ChefZ_Record.PatchInt()
// und ChefZ_Record.DefaultInt() dort nach, bevor sie den Wert verwerfen. Wer
// "minCount": 0 schreibt, bekommt 0 - und wer das Feld weglaesst, bekommt den
// Default.
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

class ChefZ_Undefined
{
    static const float  FLOAT = 0.0;
    static const int    INT   = 0;
    static const string TEXT  = "";

    //! Kein == auf float: ein JSON-Roundtrip kann eine Null als -0.0 oder als
    //! winzigen Rest zurueckgeben. Die Schranke ist so eng, dass kein
    //! Kochdatenwert hineinfaellt, den jemand ernst meint.
    static bool IsFloatUndefined(float v)
    {
        return v > -0.000001 && v < 0.000001;
    }

    static bool IsIntUndefined(int v)
    {
        return v == 0;
    }

    static bool IsTextUndefined(string v)
    {
        return v == "";
    }

    static float FloatOr(float v, float fallback)
    {
        if (IsFloatUndefined(v))
            return fallback;
        return v;
    }

    static int IntOr(int v, int fallback)
    {
        if (IsIntUndefined(v))
            return fallback;
        return v;
    }

    static string TextOr(string v, string fallback)
    {
        if (IsTextUndefined(v))
            return fallback;
        return v;
    }

    //! Nur fuer den Selbsttest (S1). Prueft, dass die Sentinel als solche
    //! erkannt werden und kein Alltagswert faelschlich als "nicht gesetzt" gilt.
    static bool SelfCheck()
    {
        if (!IsFloatUndefined(FLOAT))       return false;
        if (!IsIntUndefined(INT))           return false;
        if (!IsTextUndefined(TEXT))         return false;
        if (IsFloatUndefined(1.0))          return false;
        if (IsFloatUndefined(-1000000.0))   return false;
        if (IsIntUndefined(1))              return false;
        if (IsIntUndefined(-2147483647))    return false;
        if (FloatOr(FLOAT, 2.5) != 2.5)     return false;
        if (FloatOr(1.5, 2.5) != 1.5)       return false;
        if (IntOr(INT, 7) != 7)             return false;
        if (IntOr(3, 7) != 3)               return false;
        if (TextOr(TEXT, "x") != "x")       return false;
        return true;
    }
}
