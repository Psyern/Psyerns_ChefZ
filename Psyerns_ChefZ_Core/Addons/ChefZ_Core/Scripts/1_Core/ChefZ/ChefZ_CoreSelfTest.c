//==============================================================================
// ChefZ_CoreSelfTest - Abnahmepruefung fuer S1
//
// Entwurf: 19 §3, S1 - "Fertig, wenn: Mod laedt, [ChefZ][CORE] v1 geladen steht
// im RPT, ein Testaufruf interniert Symbole und liefert stabile
// persistHash/syncOrdinal-Werte."
//
// Diese Klasse ist genau dieser Testaufruf. Sie ist rein lesend gegenueber dem
// Spiel: sie beruehrt kein Item, keine Entity, keine Vanilla-Kochlogik. Sie
// legt ausschliesslich Symbole mit dem Praefix "CHEFZ_ST_", "CHEFZ_LM_" und
// "CHEFZ_SELFTEST_" an - Namen, die in echtem Content nicht vorkommen.
//
// Warum sie im Auslieferungsstand bleibt und nicht nur im Test-PBO: sie kostet
// beim Boot einige Mikrosekunden und beantwortet dafuer die Frage "laeuft der
// Unterbau ueberhaupt" ohne Werkzeug, ohne Debugbuild und ohne Nachbau. In
// einem System ohne Compilezeit-Sicherheit (03 E1) ist das die Gegenleistung.
//
// Ausgabe: EINE Zeile bei Erfolg, je gescheiterter Gruppe eine ERROR-Zeile.
// Detailzeilen nur, wenn Kanal CORE auf DEBUG steht.
//
// Layer: 1_Core.
//==============================================================================

class ChefZ_CoreSelfTest
{
    private static int  s_Passed;
    private static int  s_Failed;
    private static ref array<string> s_FailedNames;

    /**
     * Fuehrt alle Gruppen aus. true, wenn alle bestanden haben.
     * Mehrfachaufruf ist erlaubt und unschaedlich.
     */
    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("Undefined",   ChefZ_Undefined.SelfCheck());
        Check("Range",       ChefZ_Range.SelfCheck());
        Check("LogDefs",     ChefZ_LogChannel.SelfCheck());
        Check("Log",         ChefZ_Log.SelfCheck());
        Check("StringOrder", ChefZ_StringOrder.SelfCheck());
        Check("SymbolTable", ChefZ_SymbolTable.SelfCheck());
        Check("LoadReport",  ChefZ_LoadReport.SelfCheck());
        Check("IdentityMap", ChefZ_IdentityMap.SelfCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.CORE, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.CORE, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.CORE, "Selbsttest " + name + " FEHLGESCHLAGEN. Der Core-Unterbau verhaelt sich " + "nicht wie entworfen - alles darueber ist unzuverlaessig.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S1: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
        if (s_Failed > 0 && s_FailedNames)
        {
            s = s + "  gescheitert:";
            for (int i = 0; i < s_FailedNames.Count(); i++)
                s = s + " " + s_FailedNames.Get(i);
        }
        return s;
    }
}
