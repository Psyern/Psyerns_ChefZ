//==============================================================================
// ChefZ_AutoTest - Ergebniszeilen. Jede Zeile beginnt mit [CHEFZTEST], damit
// der Testlauf sie aus script_*.log herausfiltern kann:
//
//   [CHEFZTEST] PASS <gruppe> <name> <detail>
//   [CHEFZTEST] FAIL <gruppe> <name> <detail>
//   [CHEFZTEST] INFO <text>
//   [CHEFZTEST] SUMMARY <gruppe> pass=<n> fail=<n>
//   [CHEFZTEST] done pass=<n> fail=<n>
//==============================================================================

class ChefZ_AT_Log
{
    static int s_Pass;
    static int s_Fail;
    static ref map<string, int> s_GroupPass;
    static ref map<string, int> s_GroupFail;
    static ref array<string> s_Groups;

    static void Reset()
    {
        s_Pass = 0;
        s_Fail = 0;
        s_GroupPass = new map<string, int>();
        s_GroupFail = new map<string, int>();
        s_Groups = new array<string>();
    }

    static void Info(string text)
    {
        Print("[CHEFZTEST] INFO " + text);
    }

    static void Result(string group, string name, bool ok, string detail)
    {
        if (!s_Groups)
            Reset();
        if (s_Groups.Find(group) < 0)
        {
            s_Groups.Insert(group);
            s_GroupPass.Set(group, 0);
            s_GroupFail.Set(group, 0);
        }

        if (ok)
        {
            s_Pass++;
            s_GroupPass.Set(group, s_GroupPass.Get(group) + 1);
            Print("[CHEFZTEST] PASS " + group + " " + name + " " + detail);
        }
        else
        {
            s_Fail++;
            s_GroupFail.Set(group, s_GroupFail.Get(group) + 1);
            Print("[CHEFZTEST] FAIL " + group + " " + name + " " + detail);
        }
    }

    static void Summary()
    {
        if (!s_Groups)
            Reset();
        foreach (string g : s_Groups)
        {
            Print("[CHEFZTEST] SUMMARY " + g + " pass=" + s_GroupPass.Get(g).ToString() + " fail=" + s_GroupFail.Get(g).ToString());
        }
        Print("[CHEFZTEST] done pass=" + s_Pass.ToString() + " fail=" + s_Fail.ToString());
    }
}
