// ChefZ_Plants - reines Asset-Paket (30.08.2026). Modelle und Texturen aus Lykos'
// Lieferung c09900f.
//
// WARUM EIN EIGENES PBO MIT DIESEM PRAEFIX: die gelieferten MLOD-Modelle
// tragen ihre Texturpfade fest im Modell - "chefz\chefz_plants\data\*.paa".
// Ein Modell findet seine Textur nur, wenn das PBO genau dieses Praefix
// traegt. Deshalb steht hier KEINE Klasse: Klassen und Verhalten bleiben bei
// den Slices, die die Modelle nur ueber den Pfad
// "\ChefZ\ChefZ_Plants\models\<name>.p3d" ansprechen.
//
// QUELLE: die Dateien kommen aus ChefZ/ChefZ_Plants/ - dem Ordner, in dem die
// Lieferung liegt und bleibt. Nach einer neuen Lieferung:
//     node tools/chefz-pack/sync-assets.mjs
// kopiert sie hierher. Hier nichts von Hand aendern.
//
// KEIN CfgMods, KEIN Skript: nichts zu laden ausser Dateien. Und KEIN
// ChefZ_Core in requiredAddons - Dateien brauchen keinen Code.
class CfgPatches
{
    class ChefZ_Plants
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data"};
    };
};
