//==============================================================================
// ChefZ_PriorityWeightsDef - die Spezifitaetsgewichte, wie sie in Core.json
// stehen
//
// Entwurf: 09 §3 ("aus Core.json, nicht aus dem Code"), 09 §7 ("PriorityWeights
// fehlt oder unvollstaendig -> fehlende Felder aus Code-Defaults"), 02 E3
// (Sentinel als Erkennungsmerkmal fuer "nicht gesetzt").
//
// ---------------------------------------------------------------------------
// Warum es diese zweite Klasse ueberhaupt gibt
// ---------------------------------------------------------------------------
// ChefZ_PriorityWeights traegt fertige Werte und hat sinnvolle Code-Defaults
// im Konstruktor. Genau das macht sie als JSON-Ziel unbrauchbar: der
// Enforce-Serializer fuellt jedes nicht genannte Feld mit dem TYPdefault, also
// 0.0 - und ein Betreiber, der nur wClass anheben will, haette danach
// unbemerkt ALLE anderen Gewichte auf null. 09 §7 verlangt das Gegenteil:
// "fehlende Felder aus Code-Defaults".
//
// Deshalb die uebliche Zweiteilung des Projekts (02 E3, 07 §2.1/§2.2):
//
//     ChefZ_PriorityWeightsDef   Rohform, jedes Feld auf Sentinel
//     ChefZ_PriorityWeights      Ergebnis, jedes Feld gesetzt
//
// ApplyTo() ueberschreibt ausschliesslich, was tatsaechlich in der Datei
// stand. Eine Datei ohne "priorityWeights"-Block laesst die Ordnung damit
// vollstaendig unveraendert - und 09 §7 haelt ausdruecklich fest, dass die
// Defaults so gewaehlt sind, dass sie OHNE jede Konfiguration sinnvoll ist.
//
// KEIN CONTENT: keine Zahl hier nennt eine Kategorie, ein Gericht, eine Zutat.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_PriorityWeightsDef : Managed
{
    float wClass;
    float wState;
    float wTag;
    float wVanillaStage;
    float wCategoryBase;
    float wCategoryPerDepth;
    float wNot;
    float wRangePerBound;
    float wMinQuality;
    float wOptionalSlot;
    float wContextDeviceClass;
    float wContextBound;
    float wPolicyForbid;
    float wPolicyPerState;
    float wCapability;
    float wToolGroup;
    int   amountCap;
    float coverageBonus;
    float priorityScale;

    //! Wie ChefZ_Record.explicitFields (02 E3, Mittel 3) und wie das
    //! gleichnamige Feld in ChefZ_OutputDef. Notwendig geworden, als
    //! ChefZ_Undefined.FLOAT zu 0.0 wurde: die Zusicherung im Kopf von
    //! IsSet - "eine ausdrueckliche 0 ist ein Wert" - laesst sich am Wert
    //! allein nicht mehr einhalten.
    //!
    //! Gefuellt wird sie heute nur von Hand (Selbsttest). ChefZ_JsonExplicit
    //! traegt Schluessel auf Recordebene ein; priorityWeights ist ein
    //! Unterobjekt von ChefZ_CoreSettingsDef und damit noch nicht erfasst.
    //! Fuer Dateien bleibt es deshalb beim bisherigen Verhalten.
    ref array<string> explicitFields;

    void ChefZ_PriorityWeightsDef()
    {
        wClass              = ChefZ_Undefined.FLOAT;
        wState              = ChefZ_Undefined.FLOAT;
        wTag                = ChefZ_Undefined.FLOAT;
        wVanillaStage       = ChefZ_Undefined.FLOAT;
        wCategoryBase       = ChefZ_Undefined.FLOAT;
        wCategoryPerDepth   = ChefZ_Undefined.FLOAT;
        wNot                = ChefZ_Undefined.FLOAT;
        wRangePerBound      = ChefZ_Undefined.FLOAT;
        wMinQuality         = ChefZ_Undefined.FLOAT;
        wOptionalSlot       = ChefZ_Undefined.FLOAT;
        wContextDeviceClass = ChefZ_Undefined.FLOAT;
        wContextBound       = ChefZ_Undefined.FLOAT;
        wPolicyForbid       = ChefZ_Undefined.FLOAT;
        wPolicyPerState     = ChefZ_Undefined.FLOAT;
        wCapability         = ChefZ_Undefined.FLOAT;
        wToolGroup          = ChefZ_Undefined.FLOAT;
        coverageBonus       = ChefZ_Undefined.FLOAT;
        priorityScale       = ChefZ_Undefined.FLOAT;
        amountCap           = ChefZ_Undefined.INT;
        explicitFields      = null;
    }

    /**
     * Traegt die gesetzten Felder in einen fertigen Gewichtssatz ein.
     *
     * Rueckgabe: wie viele Felder tatsaechlich aus der Datei kamen. Der
     * Aufrufer schreibt die Zahl in den Ladebericht - ein Betreiber, der einen
     * Block geschrieben hat, der wegen eines Tippfehlers im Namen nirgends
     * ankommt, soll das sehen und nicht raten muessen.
     */
    int ApplyTo(notnull ChefZ_PriorityWeights w)
    {
        // Neunzehnmal dieselbe Zeile, und das ist Absicht: ein Helfer mit
        // "out float" muesste ein FELD eines fremden Objekts als
        // Ausgabeparameter nehmen, und dass Enforce das zusichert, steht
        // nirgends. Neunzehn ausgeschriebene Zeilen, die beim Boot einmal
        // laufen, sind der sichere Weg.
        int n = 0;

        if (IsSet("wClass", wClass))                           { w.wClass = wClass;                           n++; }
        if (IsSet("wState", wState))                           { w.wState = wState;                           n++; }
        if (IsSet("wTag", wTag))                               { w.wTag = wTag;                               n++; }
        if (IsSet("wVanillaStage", wVanillaStage))             { w.wVanillaStage = wVanillaStage;             n++; }
        if (IsSet("wCategoryBase", wCategoryBase))             { w.wCategoryBase = wCategoryBase;             n++; }
        if (IsSet("wCategoryPerDepth", wCategoryPerDepth))     { w.wCategoryPerDepth = wCategoryPerDepth;     n++; }
        if (IsSet("wNot", wNot))                               { w.wNot = wNot;                               n++; }
        if (IsSet("wRangePerBound", wRangePerBound))           { w.wRangePerBound = wRangePerBound;           n++; }
        if (IsSet("wMinQuality", wMinQuality))                 { w.wMinQuality = wMinQuality;                 n++; }
        if (IsSet("wOptionalSlot", wOptionalSlot))             { w.wOptionalSlot = wOptionalSlot;             n++; }
        if (IsSet("wContextDeviceClass", wContextDeviceClass)) { w.wContextDeviceClass = wContextDeviceClass; n++; }
        if (IsSet("wContextBound", wContextBound))             { w.wContextBound = wContextBound;             n++; }
        if (IsSet("wPolicyForbid", wPolicyForbid))             { w.wPolicyForbid = wPolicyForbid;             n++; }
        if (IsSet("wPolicyPerState", wPolicyPerState))         { w.wPolicyPerState = wPolicyPerState;         n++; }
        if (IsSet("wCapability", wCapability))                 { w.wCapability = wCapability;                 n++; }
        if (IsSet("wToolGroup", wToolGroup))                   { w.wToolGroup = wToolGroup;                   n++; }
        if (IsSet("coverageBonus", coverageBonus))             { w.coverageBonus = coverageBonus;             n++; }
        if (IsSet("priorityScale", priorityScale))             { w.priorityScale = priorityScale;             n++; }

        if (HasExplicit("amountCap") || !ChefZ_Undefined.IsIntUndefined(amountCap))
        {
            w.amountCap = amountCap;
            n++;
        }
        return n;
    }

    //! Steht dieses Feld tatsaechlich in der Datei? Eine ausdrueckliche 0 ist
    //! ein Wert und wird uebernommen - nur der Sentinel heisst "nicht gesetzt".
    private bool IsSet(string field, float value)
    {
        if (HasExplicit(field))
            return true;
        return !ChefZ_Undefined.IsFloatUndefined(value);
    }

    void MarkExplicit(string field)
    {
        if (!explicitFields)
            explicitFields = new array<string>();
        if (explicitFields.Find(field) < 0)
            explicitFields.Insert(field);
    }

    bool HasExplicit(string field)
    {
        if (!explicitFields)
            return false;
        return explicitFields.Find(field) >= 0;
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_PriorityWeights w = new ChefZ_PriorityWeights();

        // Leerer Block: nichts wird angefasst.
        ChefZ_PriorityWeightsDef empty = new ChefZ_PriorityWeightsDef();
        if (empty.ApplyTo(w) != 0)          return false;
        if (w.wClass != 3.00)               return false;
        if (w.amountCap != 3)               return false;

        // Zwei Felder gesetzt: genau zwei aendern sich.
        ChefZ_PriorityWeightsDef partial = new ChefZ_PriorityWeightsDef();
        partial.wClass    = 9.5;
        partial.amountCap = 7;
        if (partial.ApplyTo(w) != 2)        return false;
        if (w.wClass != 9.5)                return false;
        if (w.amountCap != 7)               return false;
        if (w.wCategoryBase != 1.00)        return false;   // unberuehrt
        if (w.priorityScale != 0.01)        return false;   // unberuehrt

        // Eine ausdrueckliche 0 ist ein Wert, kein "nicht gesetzt".
        ChefZ_PriorityWeightsDef zero = new ChefZ_PriorityWeightsDef();
        zero.wTag = 0.0;
        zero.MarkExplicit("wTag");
        if (zero.ApplyTo(w) != 1)           return false;
        if (w.wTag != 0.0)                  return false;

        return true;
    }
}
