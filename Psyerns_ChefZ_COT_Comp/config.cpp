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
// Weil COT optional ist und ChefZ nichts von ihm wissen darf. Dieses PBO ist
// die einzige Stelle im ganzen Projekt, die COT-Bezeichner kennt. ChefZ selbst
// laeuft davon vollstaendig unberuehrt weiter - mit oder ohne COT, mit oder
// ohne dieses PBO.
//
// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT  -  warum COT NICHT in requiredAddons steht
// ---------------------------------------------------------------------------
// requiredAddons[] ist eine HARTE Abhaengigkeit. Fehlt ein dort genannter
// Eintrag, laedt das Addon nicht - und der Betreiber faengt sich einen
// Startfehler ein, obwohl er nur einen OPTIONALEN Comp-Mod im Ordner liegen
// hat. Genau das ist unerwuenscht.
//
// Der Weg, den DayZ dafuer vorsieht, ist der Praeprozessor. Jeder Mod darf in
// CfgMods "defines[]" veroeffentlichen; diese Symbole gelten beim Kompilieren
// JEDES Skriptmoduls JEDES geladenen Mods - unabhaengig von der
// Ladereihenfolge und unabhaengig von requiredAddons. Fehlt der Mod, fehlt das
// Symbol, und der Praeprozessor entfernt den abhaengigen Code, bevor der
// Compiler ihn sieht.
//
// COT veroeffentlicht sein Symbol selbst:
//   DayZ-CommunityOnlineTools-production/JM/COT/Scripts/config.cpp:44-46
//       defines[] = { "JM_COT", ... };
// Nicht zu verwechseln mit "#define JM_COT_LOADED" in Zeile 15 derselben
// Datei: das ist ein Praeprozessorsymbol des CONFIG-Parsers und im Skript
// nicht sichtbar. Benutzt wird deshalb "JM_COT".
//
// BELEG, dass im Fremdcode genau so gebaut wird:
//   DayZExpansion/AI/Scripts/5_Mission/DayZExpansion_AI/COT/JMPlayerForm.c:13
//     - "#ifdef JM_COT" um "modded class JMPlayerForm", also exakt derselbe
//     Fall wie hier (eine COT-Formularklasse wird erweitert), waehrend
//     DayZExpansion_AI_Scripts in requiredAddons nur DZ_Characters und
//     DayZExpansion_Core_Scripts nennt (DayZExpansion/AI/Scripts/config.cpp:8-12).
//     Dasselbe Muster liegt in Expansion noch ein Dutzend Mal, u.a. in
//     BaseBuilding/Scripts/5_Mission/.../ESP/BaseBuilding/JMESPMetaBaseBuilding.c:13.
//   TerjeMods-master-main/TerjeRadiation/Scripts/4_Compatibility/
//     TerjeToDogTagsCompatibility.c:1 - "#ifdef WRDG_DOGTAGS", ebenfalls ohne
//     Eintrag in requiredAddons.
//
// GEGENPROBE: TerjeCompatibilityCOT/config.cpp:7 nennt "JM_COT_Scripts" SEHR
// WOHL in requiredAddons. Das ist die harte Bauart - zulaessig, aber genau der
// Startfehler, den dieser Umbau beseitigt.
//
// VERWORFENE ALTERNATIVEN:
//   - Laufzeitpruefung: hilft nicht. "modded class JMObjectSpawnerForm",
//     UIActionManager, UIActionSelectBox und JMObjectSpawnerModule werden vom
//     Enforce-Compiler aufgeloest, lange bevor irgendeine if-Abfrage laeuft.
//     Eine Laufzeitpruefung kann ENTSCHEIDEN, nicht KOMPILIEREN.
//   - Weiche Anmeldung ueber eine ChefZ-Registry: es gibt hier nichts
//     anzumelden. Dieses Modul haengt sich in COTs Formular, nicht in ChefZ.
//   - requiredAddons ganz leeren: unzulaessig, tools/chefz-validate/
//     configcpp.mjs meldet leere requiredAddons als Fehler.
//
// VERHALTEN OHNE COT: das PBO laedt, beide Skriptdateien sind nach dem
// Praeprozessorlauf leer, und Scripts/5_Mission/ChefZ/Cot/ChefZ_CotAbsent.c
// schreibt beim Serverstart genau eine erklaerende Zeile ins RPT - keine
// Warnung, kein Fehler.
//
// ---------------------------------------------------------------------------
// requiredAddons[] - und warum die ChefZ-Addons NICHT mehr darin stehen
// ---------------------------------------------------------------------------
// Die acht Kategorien fuehren nur KLASSENNAMEN als Zeichenketten, und jeder
// einzelne wird zur Laufzeit gegen CfgVehicles geprueft, bevor er in der Liste
// landet (Scripts/4_World/ChefZ/Cot/ChefZ_CotCategories.c, g_Game.ConfigIs-
// Existing). Kein einziger dieser Namen ist ein Bezeichner im Quelltext.
//
// Sie in requiredAddons zu fuehren, haette also genau die dokumentierte
// Nachsicht ausgehebelt: "fehlt ein ChefZ-Addon, verschwinden seine Eintraege
// lautlos aus der Kategorie". Mit hartem requiredAddons waere stattdessen das
// ganze PBO nicht geladen. Deshalb bleibt nur ChefZ_Core stehen - die
// Ladewurzel desselben Mods, aus demselben Ordner, zusammen ausgeliefert.
//
// Aus welchen Addons die Kategorien Klassen fuehren, bleibt hier als
// Herkunftsnachweis dokumentiert (ChefZ_Registry fehlt bewusst: dieses Modul
// liest keine Registry und keinen Datensatz):
//
//   ChefZ_Core          Ladewurzel des Hauptmods
//   ChefZ_Farming       Weizen, Gemuese, Kraeuterpflanzen und -saat, seit dem
//                       Slice "wildplants" auch die vier Wildpflanzen
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
        // Nur die Ladewurzel des eigenen Mods. JM_COT_Scripts steht bewusst
        // NICHT hier - siehe "WEICHE ABHAENGIGKEIT" im Kopf dieser Datei.
        requiredAddons[] = {"ChefZ_Core"};
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
            //
            // Zwei Dateien, die sich gegenseitig ausschliessen:
            // ChefZ_CotObjectSpawner.c steht unter "#ifdef JM_COT",
            // ChefZ_CotAbsent.c unter "#ifndef". Es ist immer genau eine von
            // beiden kompiliert, nie beide.
            class missionScriptModule
            {
                value = "";
                files[] = {"Psyerns_ChefZ_COT_Comp/Scripts/5_Mission"};
            };
        };
    };
};
