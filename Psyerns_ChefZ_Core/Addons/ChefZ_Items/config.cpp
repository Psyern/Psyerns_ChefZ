// ChefZ_Items - reines Asset-Paket (29.08.2026). Modelle und Texturen der Gegenstaende (Waben, Pfeife, Glas, Karotte).
//
// WARUM EIN EIGENES PBO MIT DIESEM PRAEFIX: die gelieferten MLOD-Modelle
// tragen ihre Texturpfade fest im Modell - "chefz\\chefz_items\data\*.paa"
// (nachgelesen im Binaerstrom der .p3d). Ein Modell findet seine Textur nur,
// wenn das PBO genau dieses Praefix traegt. Deshalb heisst das Paket so und
// nicht ChefZ_Farming, und deshalb steht hier KEINE Klasse: Klassen und
// Verhalten bleiben bei den Slices, die die Modelle nur ueber den Pfad
// "\ChefZ\\ChefZ_Items\models\<name>.p3d" ansprechen.
//
// QUELLE: die Dateien kommen aus ChefZ/ChefZ_Items/models und /data - dem Ordner,
// in dem Lykos' Lieferung liegt und bleibt. Nach einer neuen Lieferung:
//     node tools/chefz-pack/sync-assets.mjs
// kopiert sie hierher. Hier nichts von Hand aendern.
//
// KEIN CfgMods, KEIN Skript: nichts zu laden ausser Dateien. Und KEIN
// ChefZ_Core in requiredAddons - Dateien brauchen keinen Code; der Validator
// fragt nach, und die Antwort ist: ja, beabsichtigt.
class CfgPatches
{
    class ChefZ_Items
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data"};
    };
};
