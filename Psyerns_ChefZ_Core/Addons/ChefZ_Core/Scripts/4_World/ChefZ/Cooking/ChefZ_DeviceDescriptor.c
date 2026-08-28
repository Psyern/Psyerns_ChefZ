//==============================================================================
// ChefZ_DeviceDescriptor - was ChefZ ueber ein Kochgeraet weiss
//
// Entwurf: 10 §4 (Feldliste woertlich), 10 E7 (zweite Sicherung ueber
// CfgChefZDevices), 08 §3 (die Felder, die im ChefZ_CookContext landen),
// 01 V13 (nicht jedes Kochgeraet ist ein Edible_Base).
//
// ---------------------------------------------------------------------------
// Die zentrale Eigenschaft: "enabled" ist die zweite Sicherung
// ---------------------------------------------------------------------------
// Neben "super zuerst und bedingungslos" (10 E1) steht eine zweite,
// unabhaengige Bedingung: ChefZ tut an einem Geraet nur dann etwas, wenn es
// ausdruecklich deklariert ist. Ein Kochgeraet, von dem ChefZ nichts weiss,
// verhaelt sich exakt wie ohne ChefZ - und zwar unabhaengig davon, ob die
// uebrige Konfiguration richtig, halb kaputt oder gar nicht vorhanden ist.
//
// enabled == false ist deshalb kein Fehlerzustand, sondern der Normalfall fuer
// die grosse Mehrheit aller Gefaesse auf einem Server. Er wird NICHT geloggt
// (10 §8, Zeile "Gerät nicht in CfgChefZDevices").
//
// ---------------------------------------------------------------------------
// Klassenweite Daten und ein instanzabhaengiges Feld
// ---------------------------------------------------------------------------
// Alle Felder ausser deviceRootClass haengen ausschliesslich an der
// Geraeteklasse. Genau deshalb kann der Adapter Deskriptoren je Klassensymbol
// zwischenspeichern und die Aufloesung kostet ab dem zweiten Tick einen
// Map-Lookup.
//
// deviceRootClass ist die Ausnahme: dasselbe Pot liegt heute auf einer
// Feuerstelle und morgen auf einem Gasherd. Der Adapter setzt dieses eine Feld
// bei JEDEM Aufruf neu, bevor er den Deskriptor herausgibt.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_DeviceDescriptor : Managed
{
    //! Klasse des Gefaesses selbst (Pot, Cauldron, FryingPan ...).
    ChefZ_Sym deviceClass;

    //! Klasse darunter - Feuerstelle, Fass, Gasherd. INVALID, wenn das Gefaess
    //! frei in der Welt liegt oder in einem Inventar steckt. INSTANZABHAENGIG,
    //! siehe Kopf.
    ChefZ_Sym deviceRootClass;

    //! Geraetekategorien aus dem ChefZ_DeviceDef. Nie null, notfalls leer.
    ref array<ChefZ_Sym> deviceCategories;

    int   portionCapacity;
    float qualityModifier;

    //! Ist dieses Geraet fuer ChefZ ueberhaupt ein Geraet? Siehe Kopf.
    bool  enabled;

    //! Klasse, unter der die Deklaration gefunden wurde. Bei direkter
    //! Deklaration ist das deviceClass selbst, sonst die Vorfahrenklasse aus
    //! CfgVehicles, die den Eintrag traegt. Reine Diagnose - der Trace soll
    //! "Pot_Variant erbt von Pot" beantworten koennen, ohne dass jemand die
    //! Config aufschlaegt.
    ChefZ_Sym declaredAs;

    void ChefZ_DeviceDescriptor()
    {
        deviceCategories = new array<ChefZ_Sym>();
        Reset();
    }

    void Reset()
    {
        deviceClass     = ChefZ_SymbolTable.INVALID;
        deviceRootClass = ChefZ_SymbolTable.INVALID;
        deviceCategories.Clear();
        portionCapacity = 0;
        qualityModifier = 1.0;
        enabled         = false;
        declaredAs      = ChefZ_SymbolTable.INVALID;
    }

    void AddCategory(ChefZ_Sym category)
    {
        if (!ChefZ_SymbolTable.IsValid(category))
            return;
        if (deviceCategories.Find(category) >= 0)
            return;
        deviceCategories.Insert(category);
    }

    string ToDebugString()
    {
        if (!enabled)
            return ChefZ_SymbolTable.NameOrMark(deviceClass) + " (kein ChefZ-Geraet)";

        string s = ChefZ_SymbolTable.NameOrMark(deviceClass);
        if (declaredAs != deviceClass && ChefZ_SymbolTable.IsValid(declaredAs))
            s = s + " (deklariert als " + ChefZ_SymbolTable.Name(declaredAs) + ")";
        if (ChefZ_SymbolTable.IsValid(deviceRootClass))
            s = s + " auf " + ChefZ_SymbolTable.Name(deviceRootClass);

        string chefzTxt1 = s + " [" + ChefZ_TextList.JoinSymbols(deviceCategories, ",") + "]" + " portionen=";
        chefzTxt1 = chefzTxt1 + portionCapacity.ToString() + " qmod=" + qualityModifier.ToString();
        s = chefzTxt1;
        return s;
    }
}
