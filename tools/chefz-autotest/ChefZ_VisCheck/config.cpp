// ChefZ_VisCheck - Sichtpruefung fuer den Spieltest (30.10.2026).
//
// NICHT Teil des Mods: liegt unter tools/chefz-autotest und wird nur fuer den Test
// nach TestServer\src kopiert. Liest $profile:chefz_vischeck.txt (Serverprofil) und
// legt die genannten Klassen neben den ersten Spieler; der Client oeffnet danach das
// Inventar. Ohne die Datei tut das Addon nichts.
class CfgPatches
{
    class ChefZ_VisCheck
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data", "DZ_Scripts", "ChefZ_Core"};
    };
};

class CfgMods
{
    class ChefZ_VisCheck
    {
        dir = "ChefZ_VisCheck";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ VisCheck";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        dependencies[] = {"Mission"};

        class defs
        {
            class missionScriptModule
            {
                value = "";
                files[] = {"ChefZ_VisCheck/Scripts/5_Mission"};
            };
        };
    };
};
