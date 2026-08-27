//==============================================================================
// ChefZ_CookingSelfTest - S7 ohne Feuerstelle pruefbar
//
// Entwurf: 19 S7 (Abnahmebedingungen), 10 §5 (Drosselung), 10 E4 (warum
// Signatur), 10 E5 (Fingerprint), 10 E6 (SUPPRESSED), 18 §2 (Selbsttests
// melden sich beim Boot).
//
// ---------------------------------------------------------------------------
// Was hier geprueft wird - und was nicht
// ---------------------------------------------------------------------------
// Geprueft wird alles, was OHNE Item, Gefaess und laufende Feuerstelle
// entscheidbar ist: die Signaturarithmetik, der Sitzungsautomat und die
// Zuordnung CookingMethodType -> ChefZ-Symbol.
//
// NICHT geprueft wird, was ohne echte Welt nicht pruefbar ist: der
// Cargo-Durchlauf, die Geraeteaufloesung gegen CfgVehicles und der
// Zusammenbau des Kontextes. Genau dafuer gibt es das Fenster aus 19 S7 -
// der Trace auf einem echten Server mit echten Feuerstellen, waehrend ChefZ
// garantiert nichts veraendern kann.
//
// Die vier Bausteine hier sind die, deren Fehler man auf dem Server NICHT
// saehe: eine Signatur, die eine Aenderung verschluckt, sieht aus wie ein
// Gefaess, in dem nichts passiert - und eine falsche Zuschreibung sieht aus
// wie eine richtige.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_CookingSelfTest
{
    private static int  s_Passed;
    private static int  s_Failed;
    private static bool s_Ran;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_Ran    = true;

        Check("Signatur",       ChefZ_VesselSignature.SelfCheck());
        Check("Kochsitzung",    ChefZ_CookSession.SelfCheck());
        Check("Methodentabelle", ChefZ_CookingHook.SelfCheck());
        Check("Zuschreibung",   ChefZ_CookActor.SelfCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            return;
        }

        s_Failed++;
        ChefZ_Log.Error(ChefZ_LogChannel.COOK,
            "Selbsttest S7 fehlgeschlagen: " + name + ". Der Kochadapter ist damit "
            + "nicht vertrauenswuerdig. Vanilla-Kochen ist davon unberuehrt - der Hook "
            + "ruft super als erste Anweisung und gibt dessen Rueckgabewert zurueck.");
    }

    static string Summary()
    {
        if (!s_Ran)
            return "Selbsttest S7 (Kochadapter): nicht gelaufen";

        return "Selbsttest S7 (Kochadapter): " + s_Passed.ToString() + " bestanden, "
             + s_Failed.ToString() + " fehlgeschlagen";
    }
}
