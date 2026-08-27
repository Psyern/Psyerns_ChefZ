//==============================================================================
// ChefZ_Undefined - Sentinelwerte fuer "im JSON nicht gesetzt"
//
// Entwurf: 02 E3.
//
// Warum es das ueberhaupt gibt: Enforce-JsonSerializer fuellt fehlende Felder
// mit dem Typdefault. Damit ist "nicht gesetzt" nicht von "auf 0 gesetzt"
// unterscheidbar - und genau diese Unterscheidung braucht der feldweise Patch
// aus Rang 3 (02 E3). Fuer ref-Typen loest sich das von selbst (abwesend =
// null); Skalare brauchen einen Sentinel.
//
// Die Wahl der Sentinel:
//   FLOAT = float.LOWEST  (= -FLT_MAX)  -> kein realer Wert liegt darunter
//   INT   = int.MIN                     -> Rand des Wertebereichs
// Beides sind Werte, die in Kochdaten (Temperaturen, Mengen, Multiplikatoren)
// nicht vorkommen koennen. Wer sie trotzdem literal setzen will, nennt das Feld
// laut 02 E3 in explicitFields[].
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

class ChefZ_Undefined
{
    static const float  FLOAT = float.LOWEST;
    static const int    INT   = int.MIN;
    static const string TEXT  = "";

    //! Sentinelvergleich fuer float. Kein ==, weil ein JSON-Roundtrip den Wert
    //! theoretisch kippen koennte; unterhalb von float.LOWEST liegt nichts.
    static bool IsFloatUndefined(float v)
    {
        return v <= float.LOWEST;
    }

    static bool IsIntUndefined(int v)
    {
        return v == int.MIN;
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
        if (!IsFloatUndefined(FLOAT))   return false;
        if (!IsIntUndefined(INT))       return false;
        if (!IsTextUndefined(TEXT))     return false;
        if (IsFloatUndefined(0.0))      return false;
        if (IsFloatUndefined(-1000000.0)) return false;
        if (IsIntUndefined(0))          return false;
        if (IsIntUndefined(-2147483647)) return false;
        if (FloatOr(FLOAT, 2.5) != 2.5) return false;
        if (FloatOr(1.5, 2.5) != 1.5)   return false;
        if (IntOr(INT, 7) != 7)         return false;
        if (IntOr(3, 7) != 3)           return false;
        if (TextOr(TEXT, "x") != "x")   return false;
        return true;
    }
}
