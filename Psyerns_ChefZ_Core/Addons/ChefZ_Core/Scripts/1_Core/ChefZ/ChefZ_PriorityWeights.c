//==============================================================================
// ChefZ_PriorityWeights - die Gewichte der Spezifitaetsrechnung
//
// Entwurf: 09 §3 (Feldliste und Defaults woertlich), 09 §4.1 (wofuer jedes
// Gewicht steht), 09 §7 (Fehlerverhalten), 09 E1.
//
// Diese Datei gehoert eigentlich zu 09/S6. Sie steht schon hier, weil
// 07 §2.2 ChefZ_SelectorCompiler.ComputeSpecificity() zu S5 zaehlt und diese
// Rechnung ohne Gewichte nicht existieren kann. S6 fuellt sie aus Core.json
// und reicht sie an den Compiler weiter; bis dahin gelten die Code-Defaults.
//
// Der Punkt der ganzen Klasse (09 E1): Spezifitaet ist eine BERECHNETE Groesse.
// Kein Content-Autor pflegt eine Prioritaetszahl, also kollidieren auch sechs
// parallel arbeitende Content-Agenten nicht in einem gemeinsam genutzten
// Zahlenraum. Wer ein spezifischeres Rezept schreibt, bekommt automatisch
// Vorrang.
//
// Die Defaults sind so gewaehlt, dass die Ordnung OHNE jede Konfiguration
// sinnvoll ist (09 §7, Zeile "PriorityWeights fehlt").
//
// KEIN CONTENT: keine Zahl hier nennt eine Kategorie, ein Gericht oder eine
// Zutat.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_PriorityWeights
{
    float wClass;               // Slot nennt eine exakte Klasse
    float wState;
    float wTag;
    float wVanillaStage;
    float wCategoryBase;        // plus Tiefenbonus
    float wCategoryPerDepth;    // WILD_MEAT schlaegt MEAT
    float wNot;
    float wRangePerBound;       // je gebundenem Wertebereich
    float wMinQuality;
    float wOptionalSlot;        // optionale Slots zaehlen gedaempft
    float wContextDeviceClass;  // je exakt genannter Geraeteklasse
    float wContextBound;        // je Temperatur-/Fluessigkeitsbedingung
    float wPolicyForbid;        // wenn extraItems == "forbid"
    float wPolicyPerState;      // je Eintrag in forbiddenStates
    float wCapability;          // je Eintrag in requires[]
    float wToolGroup;           // je Eintrag in requiredToolGroups
    int   amountCap;            // Deckel fuer min(minCount, cap)
    float coverageBonus;        // Abdeckungsbonus (09 §4.2)
    float priorityScale;        // Daempfung der Handzahl

    /**
     * Code-Defaults woertlich aus 09 §3.
     *
     * Bewusst im Konstruktor gesetzt und NICHT ueber Sentinel: diese Klasse
     * wird nicht direkt deserialisiert. S6 liest die Werte aus Core.json und
     * setzt nur die tatsaechlich genannten Felder - so bleibt "nicht
     * konfiguriert" gleichbedeutend mit "Default", ohne dass hier eine zweite
     * Sentinel-Mechanik entsteht.
     */
    void ChefZ_PriorityWeights()
    {
        wClass              = 3.00;
        wState              = 2.00;
        wTag                = 1.50;
        wVanillaStage       = 1.50;
        wCategoryBase       = 1.00;
        wCategoryPerDepth   = 0.50;
        wNot                = 0.50;
        wRangePerBound      = 0.25;
        wMinQuality         = 0.75;
        wOptionalSlot       = 0.25;
        wContextDeviceClass = 0.50;
        wContextBound       = 0.25;
        wPolicyForbid       = 0.50;
        wPolicyPerState     = 0.25;
        wCapability         = 0.25;
        wToolGroup          = 0.25;
        amountCap           = 3;
        coverageBonus       = 0.50;
        priorityScale       = 0.01;
    }

    //! Sind faktisch alle Spezifitaetsgewichte aus? 09 §7 verlangt dafuer ein
    //! WARN beim Boot - dann entscheidet nur noch der Tiebreak, und das ist
    //! fast nie die Absicht des Betreibers.
    bool IsSpecificityDisabled()
    {
        if (wClass != 0.0)              return false;
        if (wState != 0.0)              return false;
        if (wTag != 0.0)                return false;
        if (wVanillaStage != 0.0)       return false;
        if (wCategoryBase != 0.0)       return false;
        if (wCategoryPerDepth != 0.0)   return false;
        if (wNot != 0.0)                return false;
        if (wRangePerBound != 0.0)      return false;
        if (wMinQuality != 0.0)         return false;
        return true;
    }

    /**
     * Klammert Unsinn, statt ihn abzuweisen (02 §8: jeder Fehler bewegt das
     * System Richtung weniger ChefZ, nie Richtung falsches ChefZ).
     *
     * Negative Gewichte wuerden die Ordnung umdrehen - eine Unterkategorie
     * waere dann UNspezifischer als ihre Elternkategorie, und die Grundregel
     * aus §16 stuende auf dem Kopf. amountCap unter 1 wuerde jeden
     * Pflichtslot mit 0 gewichten.
     */
    void ClampInPlace()
    {
        wClass              = AtLeastZero(wClass);
        wState              = AtLeastZero(wState);
        wTag                = AtLeastZero(wTag);
        wVanillaStage       = AtLeastZero(wVanillaStage);
        wCategoryBase       = AtLeastZero(wCategoryBase);
        wCategoryPerDepth   = AtLeastZero(wCategoryPerDepth);
        wNot                = AtLeastZero(wNot);
        wRangePerBound      = AtLeastZero(wRangePerBound);
        wMinQuality         = AtLeastZero(wMinQuality);
        wOptionalSlot       = AtLeastZero(wOptionalSlot);
        wContextDeviceClass = AtLeastZero(wContextDeviceClass);
        wContextBound       = AtLeastZero(wContextBound);
        wPolicyForbid       = AtLeastZero(wPolicyForbid);
        wPolicyPerState     = AtLeastZero(wPolicyPerState);
        wCapability         = AtLeastZero(wCapability);
        wToolGroup          = AtLeastZero(wToolGroup);
        coverageBonus       = AtLeastZero(coverageBonus);
        priorityScale       = AtLeastZero(priorityScale);

        if (amountCap < 1)
            amountCap = 1;
    }

    private float AtLeastZero(float v)
    {
        if (v < 0.0)
            return 0.0;
        return v;
    }

    string ToDebugString()
    {
        return "class=" + wClass.ToString() + " state=" + wState.ToString() + " tag=" + wTag.ToString() + " stage=" + wVanillaStage.ToString() + " kat=" + wCategoryBase.ToString() + "+" + wCategoryPerDepth.ToString() + "/Tiefe" + " not=" + wNot.ToString() + " range=" + wRangePerBound.ToString() + " minQual=" + wMinQuality.ToString() + " amountCap=" + amountCap.ToString();
    }

    //! Nur fuer den Selbsttest (S5).
    static bool SelfCheck()
    {
        ChefZ_PriorityWeights w = new ChefZ_PriorityWeights();
        if (w.wClass != 3.00)               return false;
        if (w.wCategoryPerDepth != 0.50)    return false;
        if (w.amountCap != 3)               return false;
        if (w.IsSpecificityDisabled())      return false;

        w.wClass = -5.0;
        w.amountCap = 0;
        w.ClampInPlace();
        if (w.wClass != 0.0)                return false;
        if (w.amountCap != 1)               return false;

        ChefZ_PriorityWeights off = new ChefZ_PriorityWeights();
        off.wClass = 0.0; off.wState = 0.0; off.wTag = 0.0;
        off.wVanillaStage = 0.0; off.wCategoryBase = 0.0;
        off.wCategoryPerDepth = 0.0; off.wNot = 0.0;
        off.wRangePerBound = 0.0; off.wMinQuality = 0.0;
        if (!off.IsSpecificityDisabled())   return false;

        return true;
    }
}
