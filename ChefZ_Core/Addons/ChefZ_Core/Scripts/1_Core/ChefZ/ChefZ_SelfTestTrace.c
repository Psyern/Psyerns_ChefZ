/**
 * ChefZ_SelfTestTrace - sagt, WO ein Selbsttest gescheitert ist.
 *
 * Die Selbsttests sind bool-Funktionen mit vielen "return false". Bis zum
 * 29.08.2026 stand im Log nur "FEHLGESCHLAGEN" - ohne Stelle, ohne Grund.
 * Jetzt geht jedes return false ueber Fail(datei, zeile, bedingung); der
 * Check() der Suite haengt die letzten Stationen an seine Fehlermeldung.
 *
 * Die Kette wird von hinten gelesen: der letzte Eintrag ist die aeusserste
 * Stelle im Testverlauf, die davor liegenden fuehren nach innen. Ein Helfer,
 * der absichtlich false liefert (etwa "darf nicht kompilieren"), hinterlaesst
 * ebenfalls eine Station - deshalb setzt Check() bei jeder gruenen Gruppe
 * zurueck, damit nichts in die naechste Meldung hineinlaeuft.
 *
 * Kein Zustand ausserhalb der Selbsttests, kein Einfluss auf das Spiel.
 */
class ChefZ_SelfTestTrace
{
    private static const int MAX_KEPT = 6;
    private static ref array<string> s_Stations;

    //! Vermerkt eine Station und gibt immer false zurueck - Drop-in fuer "return false".
    static bool Fail(string file, int line, string cond)
    {
        if (!s_Stations)
            s_Stations = new array<string>();
        string station = file + ":" + line.ToString();
        if (cond != "")
            station = station + " [" + cond + "]";
        s_Stations.Insert(station);
        if (s_Stations.Count() > MAX_KEPT)
            s_Stations.RemoveOrdered(0);
        return false;
    }

    //! Liefert die Stationen (aeusserste zuerst) als Text und leert die Liste.
    static string Take()
    {
        if (!s_Stations || s_Stations.Count() == 0)
            return " Stelle: unbekannt (kein instrumentiertes return false erreicht).";
        string s = " Stelle: ";
        int i = s_Stations.Count() - 1;
        while (i >= 0)
        {
            s = s + s_Stations.Get(i);
            if (i > 0)
                s = s + " <- ";
            i = i - 1;
        }
        s_Stations.Clear();
        return s;
    }

    static void Reset()
    {
        if (s_Stations)
            s_Stations.Clear();
    }
}
