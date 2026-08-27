// Psyerns_ChefZ_COT_Comp - Anbindung an Community Online Tools.
//
// Vorbild: TerjeMods-master-main/TerjeCompatibilityCOT/config.cpp - eigener
// Mod, eigenes CfgPatches, eigene Skriptmodule; das Hauptmod weiss nichts von
// ihm und braucht ihn nicht.
//
// ---------------------------------------------------------------------------
// WAS DIESES MODUL TUT
// ---------------------------------------------------------------------------
// Es haengt an COTs Object Spawner acht ChefZ-Spawnkategorien an. Mehr nicht.
// Kein Rezept, kein Naehrwert, keine Transform, kein Balancing - dieses Modul
// ist ein ADMIN-WERKZEUG und veraendert keine Spielmechanik. Der einzige
// Eingriff ist ein zusaetzlicher Filter in einer Adminmaske.
//
// ---------------------------------------------------------------------------
// WARUM ES EIN EIGENER MOD IST UND KEIN TEIL DES HAUPTMODS
// ---------------------------------------------------------------------------
// requiredAddons[] nennt "JM_COT_Scripts". Ein Addon, dessen requiredAddons
// nicht aufloesbar sind, wird von DayZ nicht geladen. Genau das ist hier die
// Absicht: ohne COT existiert dieses PBO im Spiel schlicht nicht, und die
// modded-Klassen in Scripts/5_Mission werden nie kompiliert. ChefZ selbst
// laeuft davon vollstaendig unberuehrt weiter.
//
// Umgekehrt gilt dasselbe fuer die ChefZ-Seite: die acht Kategorien nennen
// Klassennamen, und jeder einzelne wird zur Laufzeit gegen CfgVehicles
// geprueft, bevor er in der Liste landet (siehe
// Scripts/5_Mission/ChefZ/Cot/ChefZ_CotObjectSpawner.c). Fehlt ein
// ChefZ-Addon, verschwinden seine Eintraege lautlos aus der Kategorie - es
// gibt keine Geisterklasse und keinen Fehler.
//
// ---------------------------------------------------------------------------
// requiredAddons[]
// ---------------------------------------------------------------------------
// Genannt sind COT und GENAU die ChefZ-Addons, aus denen die acht Kategorien
// Klassen fuehren. ChefZ_Registry fehlt bewusst: dieses Modul liest keine
// Registry und keinen Datensatz, es fuehrt nur Klassennamen.
//
//   ChefZ_Core          Ladewurzel des Hauptmods
//   ChefZ_Farming       Weizen, Gemuese, Kraeuterpflanzen und -saat
//   ChefZ_Ingredients   Schnittgut, Milchwaren, Salz, Trockenkraeuter, Gewuerze
//   ChefZ_Baking        Hefe, Teige, Pasta, Brot
//   ChefZ_Meat          Hackfleisch und Wurst
//   ChefZ_Preservation  Gesalzenes, Getrocknetes, Geraeuchertes
//   ChefZ_Processing    Stationen und Werkzeuge, Mehl
//   ChefZ_Cooking       Saucen, Bruehen, Gerichte, Leerbehaelter
//
// ---------------------------------------------------------------------------
// KEINE CfgVehicles
// ---------------------------------------------------------------------------
// Dieses Modul definiert keine einzige Item-Klasse und ueberschreibt keine.
// units[] und weapons[] sind deshalb leer und bleiben es. Wer hier eine Klasse
// ergaenzen will, hat den Zweck des Moduls missverstanden.

class CfgPatches
{
    class Psyerns_ChefZ_COT_Comp
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "JM_COT_Scripts",
            "ChefZ_Core",
            "ChefZ_Farming",
            "ChefZ_Ingredients",
            "ChefZ_Baking",
            "ChefZ_Meat",
            "ChefZ_Preservation",
            "ChefZ_Processing",
            "ChefZ_Cooking"
        };
    };
};

class CfgMods
{
    // Klassenname nach ChefZ-Namenskonvention (DME-Plan §53). Der
    // PBO-Bezeichner in CfgPatches heisst weiterhin wie der Ordner - das sind
    // zwei verschiedene Namensraeume, und nur der CfgPatches-Name ist das, was
    // ein anderes Addon in requiredAddons nennen wuerde.
    class ChefZ_CotComp
    {
        // PFADWURZEL: das PBO-Praefix, und das ist der ORDNERNAME
        // (Entwurf 02 §4.1, B4). $PREFIX$ enthaelt denselben Text.
        dir = "Psyerns_ChefZ_COT_Comp";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Compatibility COT";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        // "Core" und "Game" stehen hier, obwohl dieses Modul in beiden Ebenen
        // keine Datei ausliefert: dependencies[] beschreibt die
        // Ladereihenfolge gegenueber den Basis-Skriptmodulen, nicht den
        // eigenen Lieferumfang. Genau so haelt es TerjeCompatibilityCOT.
        dependencies[] = {"Core", "Game", "World", "Mission"};

        class defs
        {
            // 4_World: die reine Kategorientabelle. Sie ist Daten und sonst
            // nichts - kein Engine-Typ, kein Zugriff auf Mission oder GUI.
            class worldScriptModule
            {
                value = "";
                files[] = {"Psyerns_ChefZ_COT_Comp/Scripts/4_World"};
            };

            // 5_Mission: die modded-Klasse. JMObjectSpawnerForm liegt in COTs
            // missionScriptModule; eine Erweiterung MUSS in derselben Ebene
            // liegen, sonst kennt der Compiler die Basisklasse nicht.
            class missionScriptModule
            {
                value = "";
                files[] = {"Psyerns_ChefZ_COT_Comp/Scripts/5_Mission"};
            };
        };
    };
};
