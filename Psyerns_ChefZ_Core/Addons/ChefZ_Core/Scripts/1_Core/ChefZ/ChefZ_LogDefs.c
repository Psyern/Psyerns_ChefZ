//==============================================================================
// ChefZ_LogLevel / ChefZ_LogChannel - Stufen und Kanaele des Logs
//
// Entwurf: 18 §2.1, E1.
//
// Zwei Achsen statt einer: Stufe UND Kanalbitmaske. Ein globales DEBUG ist bei
// dutzenden gleichzeitig laufenden Feuerstellen unbrauchbar - die interessante
// Zeile geht in tausenden unter. Mit der Maske sagt ein Betreiber "nur MATCH
// und COOK auf DEBUG" und bekommt genau den Ausschnitt (18 E1). Kosten: ein
// AND-Vergleich.
//
// Die Namensabbildung liegt hier und nicht im Config Manager, weil sie zur
// Fehlermeldung "unbekannter Kanal, gueltig sind: ..." gehoert (18 §6) und
// diese Meldung faellt an, bevor irgendetwas initialisiert ist.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_LogLevel
{
    static const int OFF   = 0;
    static const int ERR   = 1;
    static const int WARN  = 2;
    static const int INFO  = 3;
    static const int DEBUG = 4;
    static const int TRACE = 5;

    static const int MIN_LEVEL = 0;
    static const int MAX_LEVEL = 5;

    static string Name(int level)
    {
        switch (level)
        {
            case OFF:   return "OFF";
            case ERR:   return "ERROR";
            case WARN:  return "WARN";
            case INFO:  return "INFO";
            case DEBUG: return "DEBUG";
            case TRACE: return "TRACE";
        }
        return "?" + level.ToString();
    }

    //! -1, wenn der Name unbekannt ist. Ziffernfolgen sind ebenfalls erlaubt,
    //! damit "logLevel": 3 und "logLevel": "INFO" dasselbe bedeuten.
    static int FromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToUpper();

        if (n == "OFF")     return OFF;
        if (n == "ERR")     return ERR;
        if (n == "ERROR")   return ERR;
        if (n == "WARN")    return WARN;
        if (n == "WARNING") return WARN;
        if (n == "INFO")    return INFO;
        if (n == "DEBUG")   return DEBUG;
        if (n == "TRACE")   return TRACE;
        return -1;
    }

    static bool IsValid(int level)
    {
        return level >= MIN_LEVEL && level <= MAX_LEVEL;
    }

    //! Eine kaputte Stufe darf nie zu einem stillen "gar kein Log" fuehren.
    //! Unter den Bereich geklammert wird auf ERR, nicht auf OFF.
    static int Clamp(int level)
    {
        if (level < MIN_LEVEL)
            return ERR;
        if (level > MAX_LEVEL)
            return MAX_LEVEL;
        return level;
    }

    static string ValidNames()
    {
        return "OFF, ERROR, WARN, INFO, DEBUG, TRACE";
    }
}

class ChefZ_LogChannel
{
    static const int CORE    = 1 << 0;
    static const int CONFIG  = 1 << 1;
    static const int MATCH   = 1 << 2;
    static const int COOK    = 1 << 3;
    static const int PROCESS = 1 << 4;
    static const int STATE   = 1 << 5;
    static const int QUALITY = 1 << 6;
    static const int NUTRI   = 1 << 7;
    static const int PRESERV = 1 << 8;
    static const int PORTION = 1 << 9;
    static const int CONTAIN = 1 << 10;
    static const int EVENT   = 1 << 11;
    static const int PERF    = 1 << 12;

    static const int NONE    = 0;
    static const int ALL     = 0x1FFF;      // Bits 0..12, deckt genau die 13 Kanaele

    //! Name eines EINZELNEN Kanalbits. Fuer Masken mit mehreren Bits liefert
    //! die Funktion den ersten gesetzten Kanal plus "+" - im Ausgabepfad steht
    //! immer genau ein Bit, also ist das kein heisser Sonderfall.
    static string Name(int channel)
    {
        switch (channel)
        {
            case CORE:    return "CORE";
            case CONFIG:  return "CONFIG";
            case MATCH:   return "MATCH";
            case COOK:    return "COOK";
            case PROCESS: return "PROCESS";
            case STATE:   return "STATE";
            case QUALITY: return "QUALITY";
            case NUTRI:   return "NUTRI";
            case PRESERV: return "PRESERV";
            case PORTION: return "PORTION";
            case CONTAIN: return "CONTAIN";
            case EVENT:   return "EVENT";
            case PERF:    return "PERF";
            case NONE:    return "NONE";
            case ALL:     return "ALL";
        }

        for (int bit = 0; bit < 13; bit++)
        {
            int single = 1 << bit;
            if ((channel & single) != 0)
                return Name(single) + "+";
        }
        return "?";
    }

    //! 0, wenn der Name unbekannt ist. "ALL" und "NONE" sind zulaessig.
    static int FromName(string name)
    {
        string n = name;
        n.TrimInPlace();
        n.ToUpper();

        if (n == "CORE")    return CORE;
        if (n == "CONFIG")  return CONFIG;
        if (n == "MATCH")   return MATCH;
        if (n == "COOK")    return COOK;
        if (n == "PROCESS") return PROCESS;
        if (n == "STATE")   return STATE;
        if (n == "QUALITY") return QUALITY;
        if (n == "NUTRI")   return NUTRI;
        if (n == "PRESERV") return PRESERV;
        if (n == "PORTION") return PORTION;
        if (n == "CONTAIN") return CONTAIN;
        if (n == "EVENT")   return EVENT;
        if (n == "PERF")    return PERF;
        if (n == "ALL")     return ALL;
        if (n == "NONE")    return NONE;
        if (n == "OFF")     return NONE;
        return 0;
    }

    static string ValidNames()
    {
        return "CORE, CONFIG, MATCH, COOK, PROCESS, STATE, QUALITY, NUTRI, " + "PRESERV, PORTION, CONTAIN, EVENT, PERF, ALL, NONE";
    }

    /**
     * Baut eine Maske aus der JSON-Liste "logChannels".
     *
     * Unbekannte Namen werden ignoriert und in unknownOut gesammelt - die
     * uebrigen Kanaele wirken trotzdem (18 §6). Eine leere oder komplett
     * unbrauchbare Liste ergibt ALL, nicht NONE: ein Tippfehler in der Config
     * darf nicht dazu fuehren, dass Fehler unsichtbar werden.
     */
    static int MaskFromNames(array<string> names, out array<string> unknownOut)
    {
        if (!unknownOut)
            unknownOut = new array<string>();

        if (!names || names.Count() == 0)
            return ALL;

        int mask = 0;
        int recognised = 0;
        for (int i = 0; i < names.Count(); i++)
        {
            string raw = names.Get(i);
            int bit = FromName(raw);
            if (bit == 0)
            {
                string upper = raw;
                upper.TrimInPlace();
                upper.ToUpper();
                if (upper == "NONE" || upper == "OFF")
                {
                    recognised++;
                    continue;
                }
                unknownOut.Insert(raw);
                continue;
            }
            recognised++;
            mask = mask | bit;
        }

        if (recognised == 0)
            return ALL;
        return mask;
    }

    static bool IsEnabledIn(int mask, int channel)
    {
        return (mask & channel) != 0;
    }

    //! Nur fuer den Selbsttest (S1).
    static bool SelfCheck()
    {
        if (ALL != 0x1FFF)                          return false;
        if (PERF != (1 << 12))                      return false;
        if ((ALL & PERF) == 0)                      return false;
        if (FromName("match") != MATCH)             return false;
        if (FromName(" Cook ") != COOK)             return false;
        if (FromName("nope") != 0)                  return false;
        if (Name(CONFIG) != "CONFIG")               return false;

        array<string> unknown = new array<string>();
        array<string> names = new array<string>();
        names.Insert("MATCH");
        names.Insert("COOK");
        names.Insert("BOGUS");
        int m = MaskFromNames(names, unknown);
        if (m != (MATCH | COOK))                    return false;
        if (unknown.Count() != 1)                   return false;

        array<string> empty = new array<string>();
        array<string> unknown2 = new array<string>();
        if (MaskFromNames(empty, unknown2) != ALL)  return false;

        if (ChefZ_LogLevel.FromName("trace") != ChefZ_LogLevel.TRACE) return false;
        if (ChefZ_LogLevel.Clamp(-5) != ChefZ_LogLevel.ERR)           return false;
        if (ChefZ_LogLevel.Clamp(99) != ChefZ_LogLevel.TRACE)         return false;
        return true;
    }
}
