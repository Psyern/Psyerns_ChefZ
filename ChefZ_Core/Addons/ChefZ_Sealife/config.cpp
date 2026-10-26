// ChefZ_Sealife - reines Asset-Paket (25.10.2026). Fische, Filets, Koeder,
// Krebse und Tintenfische aus der Lieferung "3D Assets.zip", gebaut mit der
// Psyerns 3D Suite (tools/chefz-assets/import-delivery.ps1).
//
// WARUM EIN EIGENES PBO MIT DIESEM PRAEFIX: die Modelle tragen ihre
// Texturpfade fest im Modell - "ChefZ\ChefZ_Sealife\<gruppe>\data\*.paa".
// Ein Modell findet seine Textur nur, wenn das PBO genau dieses Praefix
// traegt. Deshalb steht hier KEINE Klasse: Klassen und Verhalten bleiben bei
// den Slices (ChefZ_Fishing), die die Modelle nur ueber den Pfad
// "\ChefZ\ChefZ_Sealife\<gruppe>\models\<name>.p3d" ansprechen.
//
// QUELLE: die Dateien kommen aus ChefZ/ChefZ_Sealife/ - dem Ordner, in dem
// die Lieferung liegt und bleibt. Nach einem neuen Bau:
//     node tools/chefz-pack/sync-assets.mjs
// (oder bind-delivery.py --apply, das die gebauten Dateien selbst kopiert).
// Hier nichts von Hand aendern.
//
// KEIN CfgMods, KEIN Skript, KEIN ChefZ_Core in requiredAddons - wie
// ChefZ_Food: eine .p3d haengt von keinem Skriptmodul ab.
class CfgPatches
{
    class ChefZ_Sealife
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data"};
    };
};
