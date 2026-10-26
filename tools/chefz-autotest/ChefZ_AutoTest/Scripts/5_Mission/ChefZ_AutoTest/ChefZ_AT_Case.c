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
    ref array<ref ChefZ_AT_Input> inputs;
    ref array<string> outputs;

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
        c.inputs = new array<ref ChefZ_AT_Input>();
        c.outputs = new array<string>();
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
