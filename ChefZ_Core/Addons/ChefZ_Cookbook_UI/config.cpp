//==============================================================================
// ChefZ_Cookbook_UI - die Oberflaeche des Kochbuchs
//
// Entwurf: ChefZ_Docs/ChefZ_Cookbook_Workflow.md §3 (zwei Addons) und §6.
// Bilder: ChefZ_Docs/Kochbuch/README.md.
//
// ---------------------------------------------------------------------------
// WAS DIESES ADDON IST
// ---------------------------------------------------------------------------
// Nur das Ansehen. Wissen, Persistenz, RPC, Buch und Taste liegen in
// ChefZ_Cookbook und laufen ohne dieses Addon weiter. Fehlt es, meldet
// ChefZ_CookbookOpener eine Logzeile statt eines Menues.
//
// Abweichend von Workflow §1 ohne Dabs: ein Vanilla-UIScriptedMenu genuegt
// fuer Liste und Seite, und ChefZ bleibt ohne Fremdabhaengigkeit. Begruendung
// im Kopf von ChefZ_CookbookMenu.c.
//
// ---------------------------------------------------------------------------
// WARUM DIE BILDER HIER UND NICHT IN EINEM CONTENT-MODUL LIEGEN
// ---------------------------------------------------------------------------
// Eine Kochbuchseite ist Darstellung, kein Rezept: sie zeigt, was das
// Rezept-JSON ohnehin festlegt, und ohne dieses Addon wird sie nie geladen.
// Ein Content-Modul, das ein Gericht hinzufuegt, braucht dafuer KEINE Seite -
// das Buch zeigt dann Text und den Hinweis "noch keine Zeichnung".
//==============================================================================

class CfgPatches
{
    class ChefZ_Cookbook_UI
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        // DZ_Data        Grundlage, Fonts und Stile der Layouts
        // ChefZ_Core     ChefZ_RecipeEngine, ChefZ_CompiledRecipe,
        //                ChefZ_CategoryManager, ChefZ_SymbolTable - direkt
        //                benutzt, also auch direkt genannt
        // ChefZ_Cookbook ChefZ_CookbookOpener (wird hier ueberschrieben),
        //                ChefZ_KnowledgeManager, ChefZ_CookbookRPC
        requiredAddons[] =
        {
            "DZ_Data",
            "ChefZ_Core",
            "ChefZ_Cookbook"
        };
    };
};

class CfgMods
{
    class ChefZ_Cookbook_UI
    {
        dir = "ChefZ_Cookbook_UI";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Cookbook UI";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        // Nur die Ebene, die auch Dateien hat - eine leere gelistete Ebene
        // beendet den Server ohne Meldung (siehe ChefZ_Cookbook/config.cpp).
        dependencies[] = {"World"};

        class defs
        {
            // 4_World: Menue, Seitenaufbau und das Ueberschreiben von
            // ChefZ_CookbookOpener. Das Menue braucht PlayerBase, und der
            // Opener, den es ersetzt, liegt ebenfalls hier.
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ChefZ_Cookbook_UI/Scripts/4_World"
                };
            };
        };
    };
};

//==============================================================================
// Die Seiten
//
// Ein Eintrag je ERGEBNISKLASSE (outputs[0].cls). Gruppen- und Bruehe-
// Varianten eines Gerichts teilen sich die Seite, weil sie es auf demselben
// Weg zubereiten.
//
// Die Eintraege zwischen den Markern schreibt
//   python tools/chefz-cookbook-art/build.py
// aus den Quellbildern in ChefZ_Docs/Kochbuch/source/. Von Hand ergaenzen
// geht auch - der Code liest nur den Pfad. Bedingung: .paa (oder .edds),
// Kantenlaenge eine Zweierpotenz (sonst rendert die Engine einen weissen
// Kasten). PAA, weil EDDS hier 4,8 MB je Seite waere - siehe build.py.
//==============================================================================
class CfgChefZCookbookPages
{
    // >>> PAGES (generiert, nicht von Hand aendern)
    class ChefZ_BeanSausagePlate { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_BeanSausagePlate_co.paa"; };
    class ChefZ_BloodSausagePlate { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_BloodSausagePlate_co.paa"; };
    class ChefZ_BoarSausage { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_BoarSausage_co.paa"; };
    class ChefZ_BoneBroth { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_BoneBroth_co.paa"; };
    class ChefZ_BoneBrothSoupBowl { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_BoneBrothSoupBowl_co.paa"; };
    class ChefZ_Bread { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_Bread_co.paa"; };
    class ChefZ_CheeseCurd { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_CheeseCurd_co.paa"; };
    class ChefZ_CheeseFlatbread { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_CheeseFlatbread_co.paa"; };
    class ChefZ_ChernarusChiliBowl { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_ChernarusChiliBowl_co.paa"; };
    class ChefZ_CookedSausage { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_CookedSausage_co.paa"; };
    class ChefZ_CreamMushroomPasta { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_CreamMushroomPasta_co.paa"; };
    class ChefZ_CreamSauce { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_CreamSauce_co.paa"; };
    class ChefZ_CrispyFriedCalamari { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_CrispyFriedCalamari_co.paa"; };
    class ChefZ_FarmersBreakfast { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_FarmersBreakfast_co.paa"; };
    class ChefZ_FishPotatoPlate { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_FishPotatoPlate_co.paa"; };
    class ChefZ_FishermanStewBowl { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_FishermanStewBowl_co.paa"; };
    class ChefZ_Flatbread { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_Flatbread_co.paa"; };
    class ChefZ_GrilledStuffedCalamari { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_GrilledStuffedCalamari_co.paa"; };
    class ChefZ_HoneyBreadPlate { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_HoneyBreadPlate_co.paa"; };
    class ChefZ_HunterPasta { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_HunterPasta_co.paa"; };
    class ChefZ_HunterPlate { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_HunterPlate_co.paa"; };
    class ChefZ_HunterSausage { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_HunterSausage_co.paa"; };
    class ChefZ_HunterStewBowl { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_HunterStewBowl_co.paa"; };
    class ChefZ_MacAndCheese { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_MacAndCheese_co.paa"; };
    class ChefZ_MeatDumplings { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_MeatDumplings_co.paa"; };
    class ChefZ_MilkRice { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_MilkRice_co.paa"; };
    class ChefZ_MushroomCreamSauce { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_MushroomCreamSauce_co.paa"; };
    class ChefZ_MushroomPan { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_MushroomPan_co.paa"; };
    class ChefZ_PorkSausage { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_PorkSausage_co.paa"; };
    class ChefZ_PotatoPancakes { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_PotatoPancakes_co.paa"; };
    class ChefZ_PumpkinSoupBowl { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_PumpkinSoupBowl_co.paa"; };
    class ChefZ_SausageBreadPlate { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_SausageBreadPlate_co.paa"; };
    class ChefZ_SausagePasta { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_SausagePasta_co.paa"; };
    class ChefZ_SausagePotatoes { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_SausagePotatoes_co.paa"; };
    class ChefZ_ScrambledEggSausage { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_ScrambledEggSausage_co.paa"; };
    class ChefZ_ShellfishBisqueBowl { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_ShellfishBisqueBowl_co.paa"; };
    class ChefZ_SmallFishPan { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_SmallFishPan_co.paa"; };
    class ChefZ_SpicySausage { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_SpicySausage_co.paa"; };
    class ChefZ_SurvivorSpaghetti { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_SurvivorSpaghetti_co.paa"; };
    class ChefZ_TacticalBreakfast { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_TacticalBreakfast_co.paa"; };
    class ChefZ_TomatoSauce { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_TomatoSauce_co.paa"; };
    class ChefZ_VegetableSoupBowl { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_VegetableSoupBowl_co.paa"; };
    class ChefZ_VenisonSausage { image = "ChefZ_Cookbook_UI/data/pages/ChefZ_VenisonSausage_co.paa"; };
    // <<< PAGES
};
