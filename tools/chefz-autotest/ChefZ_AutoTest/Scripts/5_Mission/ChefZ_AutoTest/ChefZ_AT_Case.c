//==============================================================================
// ChefZ_AutoTest - ein Testfall: Kochrezept (COOK) oder Transformation (PROC).
// Die Faelle selbst erzeugt tools/chefz-autotest/make_cases.py.
//==============================================================================

class ChefZ_AT_Input
{
    string cls;
    int    count;
    string state;     // ChefZ-Zustand, "" = Vorgabe des Items
    string vstage;    // Vanilla-Garstufe (Raw, Baked, ...), "" = unveraendert
    float  qtyPct = -1;   // Fuellstand 0..1 fuer Slots mit quantityPct, -1 = voll
}

class ChefZ_AT_Case
{
    string kind;      // COOK | PROC
    string id;
    string device;    // Kochgeraet oder Station, bei Handarbeit ""
    string method;    // BAKING | BOILING | NONE (nur COOK)
    string process;   // nur PROC
    string liquid;
    float  liquidQty;
    float  seconds;
    string note;
    //! Reiner Zustandswechsel (27.10.2026): der Eingang bleibt und muss danach
    //! in dieser Vanilla-Garstufe stehen (Name aus FoodStageType, z. B.
    //! "DRIED"). "" = keine Stufenpruefung.
    string expectStage;
    ref array<ref ChefZ_AT_Input> inputs;
    ref array<string> outputs;
    //! Erlaubte Nebenprodukte (byproducts[] des Transforms). Alles, was nach
    //! einem Stationsfall im Laderaum liegt und weder Eingang, Ausgang noch
    //! Nebenprodukt ist, laesst den Fall scheitern (27.10.2026: der
    //! Fleischwolf darf kein Lard mehr abwerfen).
    ref array<string> byproducts;

    static ChefZ_AT_Case Make(string pKind, string pId, string pDevice, string pMethod, string pProcess)
    {
        ChefZ_AT_Case c = new ChefZ_AT_Case();
        c.kind = pKind;
        c.id = pId;
        c.device = pDevice;
        c.method = pMethod;
        c.process = pProcess;
        c.liquid = "";
        c.liquidQty = 0;
        c.seconds = 0;
        c.note = "";
        c.expectStage = "";
        c.inputs = new array<ref ChefZ_AT_Input>();
        c.outputs = new array<string>();
        c.byproducts = new array<string>();
        return c;
    }

    void SetLiquid(string type, float qty)
    {
        liquid = type;
        liquidQty = qty;
    }

    void AddInput(string pCls, int pCount, string pState, string pVstage)
    {
        ChefZ_AT_Input i = new ChefZ_AT_Input();
        i.cls = pCls;
        i.count = pCount;
        i.state = pState;
        i.vstage = pVstage;
        inputs.Insert(i);
    }

    void SetLastQtyPct(float pct)
    {
        if (inputs.Count() > 0)
            inputs[inputs.Count() - 1].qtyPct = pct;
    }

    void AddOutput(string cls)
    {
        outputs.Insert(cls);
    }

    void AddByproduct(string cls)
    {
        byproducts.Insert(cls);
    }

    //! Darf diese Klasse nach dem Fall im Laderaum der Station liegen?
    bool IsExpectedInCargo(string cls)
    {
        if (outputs.Find(cls) >= 0)
            return true;
        if (byproducts.Find(cls) >= 0)
            return true;
        foreach (ChefZ_AT_Input i : inputs)
        {
            if (i.cls == cls)
                return true;
        }
        return false;
    }

    string DescribeInputs()
    {
        string s = "";
        foreach (ChefZ_AT_Input i : inputs)
        {
            if (s != "")
                s = s + ",";
            s = s + i.count.ToString() + "x" + i.cls;
            if (i.state != "")
                s = s + "[" + i.state + "]";
            if (i.vstage != "")
                s = s + "{" + i.vstage + "}";
        }
        return s;
    }
}
