// ChefZ_Devices - reines Asset-Paket (29.08.2026). Modelle und Texturen der Bienenstoecke.
//
// WARUM EIN EIGENES PBO MIT DIESEM PRAEFIX: die gelieferten MLOD-Modelle
// tragen ihre Texturpfade fest im Modell - "chefz\\chefz_devices\data\*.paa"
// (nachgelesen im Binaerstrom der .p3d). Ein Modell findet seine Textur nur,
// wenn das PBO genau dieses Praefix traegt. Deshalb heisst das Paket so und
// nicht ChefZ_Farming, und deshalb steht hier KEINE Klasse: Klassen und
// Verhalten bleiben bei den Slices, die die Modelle nur ueber den Pfad
// "\ChefZ\\ChefZ_Devices\models\<name>.p3d" ansprechen.
//
// KEIN CfgMods, KEIN Skript: nichts zu laden ausser Dateien. Und KEIN
// ChefZ_Core in requiredAddons - Dateien brauchen keinen Code; der Validator
// fragt nach, und die Antwort ist: ja, beabsichtigt.
class CfgPatches
{
    class ChefZ_Devices
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data"};
    };
};
