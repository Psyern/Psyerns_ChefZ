// ChefZ_AutoTest - Testaddon fuer den autonomen Spieltest (26.10.2026).
//
// NICHT Teil des Mods: liegt unter tools/chefz-autotest und wird nur fuer den Test
// nach TestServer\src kopiert. Es tut nur etwas in einer Mission, die die Datei
// chefz_autotest.txt enthaelt (TestServer\server\mpmissions\chefzauto.chernarusplus).
class CfgPatches
{
    class ChefZ_AutoTest
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data", "DZ_Scripts", "ChefZ_Core", "ChefZ_Cooking", "ChefZ_Processing", "ChefZ_Fishing", "ChefZ_Farming", "ChefZ_Preservation", "ChefZ_Meat"};
    };
};

class CfgMods
{
    class ChefZ_AutoTest
    {
        dir = "ChefZ_AutoTest";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ AutoTest";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        dependencies[] = {"World", "Mission"};

        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] = {"ChefZ_AutoTest/Scripts/4_World"};
            };
            class missionScriptModule
            {
                value = "";
                files[] = {"ChefZ_AutoTest/Scripts/5_Mission"};
            };
        };
    };
};
