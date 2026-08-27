//==============================================================================
// ChefZ_Range - halboffener bis offener Wertebereich aus JSON
//
// Entwurf: 07 §2.1 ("class ChefZ_Range { float min; float max; }
// Sentinel = unbegrenzt"), verwendet in 07, 08, 12, 14.
//
// Beide Grenzen sind einzeln optional. Nicht gesetzt heisst unbegrenzt, nicht
// null - das ist der Grund fuer ChefZ_Undefined (02 E3). Ein Bereich
// { "min": 0.0 } begrenzt nach unten und laesst oben alles zu.
//
// Der Konstruktor setzt die Sentinel. JsonSerializer erzeugt die Instanz und
// ueberschreibt danach nur die im JSON vorhandenen Felder - deshalb bleibt eine
// weggelassene Grenze zuverlaessig "nicht gesetzt".
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

class ChefZ_Range
{
    float min;
    float max;

    void ChefZ_Range()
    {
        // Bewusst im Konstruktor und nicht als Feldinitialisierer: die
        // Auswertungsreihenfolge statischer Konstanten fremder Klassen bei
        // Feldinitialisierern ist in Enforce nicht zugesichert.
        min = ChefZ_Undefined.FLOAT;
        max = ChefZ_Undefined.FLOAT;
    }

    void Init(float minValue, float maxValue)
    {
        min = minValue;
        max = maxValue;
    }

    bool HasMin()
    {
        return !ChefZ_Undefined.IsFloatUndefined(min);
    }

    bool HasMax()
    {
        return !ChefZ_Undefined.IsFloatUndefined(max);
    }

    bool IsUnbounded()
    {
        return !HasMin() && !HasMax();
    }

    //! Untere Grenze als rechenbarer Wert; unbegrenzt wird zu -FLT_MAX.
    float Lower()
    {
        if (HasMin())
            return min;
        return float.LOWEST;
    }

    //! Obere Grenze als rechenbarer Wert; unbegrenzt wird zu +FLT_MAX.
    float Upper()
    {
        if (HasMax())
            return max;
        return float.MAX;
    }

    //! Beide Grenzen einschliessend. Ein unbegrenzter Bereich nimmt alles an.
    bool Contains(float v)
    {
        if (HasMin() && v < min)
            return false;
        if (HasMax() && v > max)
            return false;
        return true;
    }

    //! Sinnlose Bereiche (min > max) sind ein Datenfehler, kein Laufzeitfehler.
    //! Der Config Manager meldet sie beim Kompilieren; hier nur die Auskunft.
    bool IsValid()
    {
        if (HasMin() && HasMax())
            return min <= max;
        return true;
    }

    //! Klammert einen Wert in den Bereich. Unbegrenzte Seiten klammern nicht.
    float Clamp(float v)
    {
        float r = v;
        if (HasMin() && r < min)
            r = min;
        if (HasMax() && r > max)
            r = max;
        return r;
    }

    string ToDebugString()
    {
        string lo = "*";
        string hi = "*";
        if (HasMin())
            lo = min.ToString();
        if (HasMax())
            hi = max.ToString();
        return "[" + lo + ".." + hi + "]";
    }

    //! Nur fuer den Selbsttest (S1).
    static bool SelfCheck()
    {
        ChefZ_Range openRange = new ChefZ_Range();
        if (!openRange.IsUnbounded())        return false;
        if (!openRange.Contains(0.0))        return false;
        if (!openRange.Contains(1000000.0)) return false;
        if (!openRange.IsValid())            return false;

        ChefZ_Range onlyMin = new ChefZ_Range();
        onlyMin.min = 0.5;
        if (onlyMin.IsUnbounded())        return false;
        if (!onlyMin.HasMin())            return false;
        if (onlyMin.HasMax())             return false;
        if (onlyMin.Contains(0.4))        return false;
        if (!onlyMin.Contains(0.5))       return false;
        if (!onlyMin.Contains(9999.0))    return false;

        ChefZ_Range bounded = new ChefZ_Range();
        bounded.Init(0.2, 0.8);
        if (bounded.Contains(0.19))        return false;
        if (!bounded.Contains(0.2))        return false;
        if (!bounded.Contains(0.8))        return false;
        if (bounded.Contains(0.81))        return false;
        if (bounded.Clamp(0.0) != 0.2)     return false;
        if (bounded.Clamp(1.0) != 0.8)     return false;

        ChefZ_Range bad = new ChefZ_Range();
        bad.Init(1.0, 0.0);
        if (bad.IsValid())              return false;

        return true;
    }
}
