//==============================================================================
// Psyerns_ChefZ_Terje_Skills_Comp - optionale Anbindung von ChefZ an
//                                   TerjeSkills
//
// Quellen (alle im Fremdcode nachgeschlagen, nicht aus einer Analyse zitiert):
//   TerjeRadiation/config.cpp                 - das lebende Vorbild dafuer, wie
//                                               ein Fremdmodul Perks in einen
//                                               BESTEHENDEN Terje-Skill haengt:
//                                               "class CfgTerjeSkills { class
//                                               <Skill> { class Perks { ... }}}"
//                                               ohne Basisklassenangabe.
//   TerjeSkills/survival.hpp                  - der Skill "surv" und die Form
//                                               eines Perks (stagesCount,
//                                               requiredSkillLevels[],
//                                               requiredPerkPoints[], values[]).
//   TerjeSkills/Scripts/4_World/Classes/TerjeSkillsRegistry.c
//                                             - Perks werden dynamisch aus den
//                                               KINDERN von "<Skill> Perks"
//                                               gelesen; der Klassenname ist
//                                               beliebig, entscheidend ist "id".
//                                               Dort steht auch die Pruefung,
//                                               dass stagesCount, values[],
//                                               requiredSkillLevels[] und
//                                               requiredPerkPoints[] GLEICH
//                                               VIELE Eintraege haben.
//   TerjeCore/Scripts/3_Game/TerjeGameConfig.c - GetTerjeGameConfig() liest
//                                               config.cpp UND laesst den
//                                               Betreiber jeden Wert per
//                                               $profile:TerjeSettings\Core\
//                                               GameOverrides.xml ueberschreiben.
//                                               Deshalb steht die gesamte
//                                               XP-Matrix hier als Config und
//                                               nicht im Skript.
//
// ---------------------------------------------------------------------------
// Grundregel dieses Meilensteins
// ---------------------------------------------------------------------------
// ChefZ laeuft ohne dieses PBO vollstaendig. Alles hier ist rein additiv:
// keine Terje-Datei wird veraendert, keine ChefZ-Datei wird veraendert. Ohne
// TerjeSkills bleibt dieses Modul vollstaendig untaetig (siehe "WEICHE
// ABHAENGIGKEIT" weiter unten), und ohne dieses Modul merkt weder ChefZ noch
// Terje etwas davon.
//
// KEIN CfgVehicles-Eintrag, kein Item, kein Rezept. Dieses Modul erzeugt
// keinen Content - es verknuepft zwei bestehende Systeme.
//==============================================================================

// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT  -  warum TerjeSkills NICHT in requiredAddons steht
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
// TerjeSkills veroeffentlicht sein Symbol selbst:
//   TerjeMods-master-main/TerjeSkills/config.cpp:29
//       defines[] = { "TERJE_SKILLS_MOD" };
// und nennt TerjeCore seinerseits in requiredAddons
//   (TerjeMods-master-main/TerjeSkills/config.cpp:8-11).
// TERJE_SKILLS_MOD impliziert also TerjeCore; ein zweites Symbol waere
// ueberfluessig.
//
// BELEGE, dass im Fremdcode genau so gebaut wird:
//   TerjeMods-master-main/TerjeRadiation/Scripts/4_Compatibility/
//     TerjeToDogTagsCompatibility.c:1 - "#ifdef WRDG_DOGTAGS" um eine
//     modded class. WRDG_DOGTAGS steht NICHT in requiredAddons von
//     TerjeRadiation; dort steht nur "TerjeCore"
//     (TerjeRadiation/config.cpp:5-10).
//   DayZExpansion/AI/Scripts/5_Mission/DayZExpansion_AI/COT/JMPlayerForm.c:13
//     - "#ifdef JM_COT" um "modded class JMPlayerForm", waehrend
//     DayZExpansion_AI_Scripts in requiredAddons nur DZ_Characters und
//     DayZExpansion_Core_Scripts nennt (DayZExpansion/AI/Scripts/config.cpp:8-12).
//
// GEGENPROBE im selben Fremdcode: TerjeCompatibilityCOT und
// TerjeCompatibilityVPP nennen ihren Zielmod SEHR WOHL in requiredAddons
// (TerjeCompatibilityCOT/config.cpp:7). Das ist die harte Bauart. Sie ist
// zulaessig - und sie ist genau der Startfehler, den dieser Umbau beseitigt.
//
// VERWORFENE ALTERNATIVEN:
//   - Laufzeitpruefung ueber GetGame().ConfigIsExisting(): hilft nicht.
//     "modded class ChefZ_FreshHerbBase { override void
//     OnTerjeClientUpdate(...) }" und "GetTerjeSkills()" werden vom
//     Enforce-Compiler aufgeloest, lange bevor irgendeine if-Abfrage laeuft.
//     Ein fehlender Bezeichner ist ein Kompilierfehler, kein Laufzeitfall.
//     Eine Laufzeitpruefung kann nur ENTSCHEIDEN, nicht KOMPILIEREN.
//   - Weiche Anmeldung ueber die ChefZ-Registries (ChefZ_ProgressRegistry,
//     ChefZ_CapabilityRegistry): entkoppelt bereits die Richtung
//     ChefZ -> Comp-Mod und wird hier auch genutzt (siehe
//     Scripts/5_Mission/ChefZ/ChefZ_TerjeSkillsEntry.c). Sie kann aber die
//     Richtung Comp-Mod -> Terje NICHT entkoppeln: dort stehen die
//     Terje-Bezeichner im eigenen Quelltext. Registrierung ersetzt keine
//     Symbolaufloesung.
//   - requiredAddons ganz leeren: unzulaessig. Ohne ChefZ_Farming waere
//     "modded class ChefZ_HerbPlantBase" unaufloesbar, und
//     tools/chefz-validate/configcpp.mjs meldet leere requiredAddons als
//     Fehler ("die Ladereihenfolge ist damit undefiniert").
//
// WAS HART BLEIBT UND WARUM - alles Addons DESSELBEN Mods, im selben Ordner
// ausgeliefert. Die Richtung bleibt einseitig: dieser Mod kennt ChefZ,
// ChefZ kennt ihn nicht.
//
// VERHALTEN OHNE TerjeSkills: das PBO laedt, alle Skriptdateien sind nach dem
// Praeprozessorlauf leer, CfgTerjeSkills und CfgChefZTerjeSkills bleiben
// Configknoten, die niemand liest, und
// Scripts/5_Mission/ChefZ/ChefZ_TerjeSkillsAbsent.c schreibt beim Serverstart
// genau eine erklaerende Zeile ins RPT - keine Warnung, kein Fehler.
// ---------------------------------------------------------------------------

class CfgPatches
{
    class Psyerns_ChefZ_Terje_Skills_Comp
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;

        // Hier steht NUR, was ohne Ausnahme vorhanden sein muss, damit dieses
        // PBO ueberhaupt kompiliert:
        //
        //   DZ_Data        Basisdaten der Engine.
        //   ChefZ_Core     ChefZ_ProgressRegistry, ChefZ_CapabilityRegistry,
        //                  ChefZ_IProgressSink, ChefZ_ICapabilityProvider,
        //                  ChefZ_Log - Bezeichner im eigenen Quelltext.
        //   ChefZ_Farming  ChefZ_HerbPlantBase und ChefZ_FreshHerbBase - die
        //                  beiden einzigen ChefZ-Klassen, die hier per
        //                  "modded class" erweitert werden.
        //
        // TerjeCore und TerjeSkills stehen bewusst NICHT hier. Sie werden
        // ueber "#ifdef TERJE_SKILLS_MOD" in jeder Skriptdatei geprueft; die
        // ausfuehrliche Begruendung mit Belegstellen steht im Kopf dieser
        // Datei unter "WEICHE ABHAENGIGKEIT".
        //
        // Was aus Terje benutzt wird, damit die Liste nicht verlorengeht:
        //   TerjeCore    GetTerjeSkills(), GetTerjeGameConfig(),
        //                OnTerjeClientUpdate(), GetTerjeSkillsRegistry().
        //   TerjeSkills  der Skill "surv", ParticleList.TERJE_SKILLS_*.
        requiredAddons[] =
        {
            "DZ_Data",
            "ChefZ_Core",
            "ChefZ_Farming"
        };
    };
};

class CfgMods
{
    class ChefZ_TerjeSkillsComp
    {
        dir = "Psyerns_ChefZ_Terje_Skills_Comp";

        // Leer und versteckt wie bei ChefZ_Core: das Modulsymbol
        // Textures/mod_icon.edds ist noch nicht geliefert (siehe
        // Textures/mod_icon.README.txt). Ein Zeiger auf eine nicht vorhandene
        // Datei waere schlechter als gar keiner.
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;

        name = "ChefZ Terje Skills Compatibility";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";

        dependencies[] = {"Core", "Game", "World", "Mission"};

        class defs
        {
            // 4_World: hier leben PlayerBase, ItemBase, PlantBase und der
            // Terje-Skills-Accessor. Kein 1_Core und kein 3_Game - dieses
            // Modul bringt keine Datenschicht mit.
            class worldScriptModule
            {
                value = "";
                files[] = { "Psyerns_ChefZ_Terje_Skills_Comp/Scripts/4_World" };
            };

            // 5_Mission: genau ein Anmeldezeitpunkt je Seite, derselbe, den
            // auch ChefZ_Core benutzt (MissionServer.OnInit /
            // MissionGameplay.OnInit).
            //
            // Zwei Dateien, die sich gegenseitig ausschliessen:
            // ChefZ_TerjeSkillsEntry.c steht unter "#ifdef TERJE_SKILLS_MOD",
            // ChefZ_TerjeSkillsAbsent.c unter "#ifndef". Es ist immer genau
            // eine von beiden kompiliert, nie beide - also gibt es aus diesem
            // PBO auch nie zwei aktive "modded class MissionServer".
            class missionScriptModule
            {
                value = "";
                files[] = { "Psyerns_ChefZ_Terje_Skills_Comp/Scripts/5_Mission" };
            };
        };
    };
};

//==============================================================================
// Der Perk. Genau EINER in V1 (Terje-Analyse §35).
//
// Kein zweiter Food-Poison-Widerstand neben "svdinner" und kein zweiter
// Wildfleisch-Perk neben "wmlover" - beide existieren bereits in
// TerjeMedicine/immunity.hpp bzw. TerjeSkills/metabolism.hpp und werden hier
// bewusst NICHT dupliziert.
//
// Die Perk-ID beginnt mit "chefz", damit sie mit keinem kuenftigen
// Terje-Perk kollidiert.
//==============================================================================

class CfgTerjeSkills
{
    // Kein ": SkillsBase" und keine Vorwaertsdeklaration: TerjeRadiation
    // haengt seine beiden Perks genauso in "class Immunity { class Perks
    // { ... } }" ein. Der Config-Merge der Engine ergaenzt den bestehenden
    // Knoten; er ersetzt ihn nicht.
    class Survival
    {
        class Perks
        {
            // Klassenname frei waehlbar - TerjeSkillCfg.OnInit() liest die
            // KINDER von "Perks" und nimmt "id" als Schluessel.
            class ChefZ_Herbalist
            {
                id = "chefzherb";
                enabled = 1;

                displayName = "#STR_CHEFZ_PERK_HERBALIST";
                description = "#STR_CHEFZ_PERK_HERBALIST_DESC";

                // Fuenf Stufen nach Terje-Analyse §7 und §35.
                stagesCount = 5;

                // Vier Listen, fuenf Eintraege - TerjePerkCfg.OnInit() meldet
                // jede Abweichung als TerjeLog_Error.
                requiredSkillLevels[] = {5, 10, 15, 25, 35};
                requiredPerkPoints[]  = {1, 1, 1, 2, 2};

                // Der Ausbeutebonus. Stufe I ist bewusst 0.0: sie schaltet
                // ausschliesslich das Hervorheben frei (§7).
                values[] = {0.0, 0.1, 0.2, 0.3, 0.5};

                // Symbole aus TerjeSkills/Textures/Icons/TerjePerk.imageset
                // bzw. TerjePerkBlack.imageset. Bewusst ein VORHANDENES
                // Symbol statt eines eigenen Imagesets: dieses Modul liefert
                // keine Grafik aus, und ein Zeiger auf eine fehlende Textur
                // waere ein leeres Feld im Perk-Baum. tp_mushpremonition ist
                // das thematisch naechste (Sammeln/Erspueren von Pflanzen).
                disabledIcon = "set:TerjePerkBlack_icon image:tp_mushpremonition";
                enabledIcon  = "set:TerjePerk_icon image:tp_mushpremonition";
            };
        };
    };
};

//==============================================================================
// Die Stellschrauben dieses Moduls.
//
// Gelesen ueber GetTerjeGameConfig() (TerjeCore/Scripts/3_Game/
// TerjeGameConfig.c). Damit gilt fuer JEDEN Wert hier: der Betreiber kann ihn
// in $profile:TerjeSettings\Core\GameOverrides.xml ueberschreiben, ohne ein
// PBO anzufassen. Deshalb steht die XP-Matrix als Daten hier und nicht als
// Zahlen im Skript.
//==============================================================================

class CfgChefZTerjeSkills
{
    //--------------------------------------------------------------------------
    // Hauptschalter. 0 = dieses Modul tut gar nichts mehr, weder XP noch
    // Perkwirkung. Ein Betreiber, der den Perk behalten, aber die XP-Vergabe
    // abschalten will, setzt stattdessen ChefZ_Xp enabled = 0.
    //--------------------------------------------------------------------------
    enabled = 1;

    //--------------------------------------------------------------------------
    // XP-Matrix. Terje-Analyse §26.
    //
    // ZWEI HARTE REGELN, die man dieser Tabelle ansehen muss:
    //
    //   1. ESSEN steht hier nicht. Terje vergibt Metabolism-XP automatisch in
    //      TerjeSkills/Scripts/4_World/Classes/PlayerStomach.c
    //      (AddToStomach -> "mtblsm"). Eine zweite Vergabe waere doppelt.
    //
    //   2. TIERZERLEGUNG und FISCHFILETIEREN stehen hier nicht. Sie gehoeren
    //      Terje ("hunt" in PrepareAnimal.c, "fish" in PrepareFish.c). Auch
    //      die Wurstherstellung gibt ausdruecklich Survival-XP und NIE
    //      Hunting-XP - sie ist Weiterverarbeitung, keine Zerlegung.
    //--------------------------------------------------------------------------
    class ChefZ_Xp
    {
        enabled = 1;

        // Terje-Skill, in den alles hier einzahlt. "surv" laut
        // TerjeSkills/survival.hpp.
        skill = "surv";

        // Sichtbare Notification bei jedem Gewinn. 0 = still. Terje reicht
        // das als 4. Parameter an AddSkillExperience() durch.
        showNotification = 1;

        //----------------------------------------------------------------------
        // Mengenbonus (§27, "Basis-XP + kleiner Mengenbonus").
        //
        // NICHT volle XP je Stueck. Der Bonus ist doppelt gedeckelt: erst auf
        // batchMaxUnits zusaetzliche Einheiten, dann auf einen Bruchteil der
        // Basis-XP. Bei 10 Wuersten auf einmal gibt es damit 5 + 2 statt 50.
        //----------------------------------------------------------------------
        batchBonusPerUnit = 1;
        batchMaxUnits     = 3;
        batchCapPercent   = 50;

        //----------------------------------------------------------------------
        // Wiederholungsdaempfung (§27).
        //
        // Dieselbe Aktion wieder und wieder in kurzer Folge zahlt weniger. Der
        // erste Durchlauf und die naechsten (repeatFreeCount - 1) zaehlen
        // voll; danach faellt der Faktor je Wiederholung, bis repeatMinPercent
        // erreicht ist. Nach repeatWindowSec ohne diese Aktion ist der Zaehler
        // wieder bei null.
        //
        // 0 fuer repeatFreeCount schaltet die Daempfung ab.
        //----------------------------------------------------------------------
        repeatFreeCount   = 5;
        repeatWindowSec   = 900;
        repeatStepPercent = 25;
        repeatMinPercent  = 25;

        //----------------------------------------------------------------------
        // Kochen. Schluessel ist die Rezept-ID aus dem ChefZ-Rezeptsatz.
        //
        // Die drei Klassen aus §26 - einfache Mahlzeit 3, komplexe Mahlzeit 8,
        // Premium-Gericht 15 - werden ueber die Zahl der TATSAECHLICH
        // verbrauchten Zutaten eingestuft. Das ist die einzige Angabe, die im
        // Abschlussereignis des Core zuverlaessig steht (consumedClasses); ein
        // "premium"-Merkmal am Gericht gibt es in den Rezeptdaten nicht.
        //
        // Wer ein einzelnes Rezept anders bewerten will, traegt es unter
        // ChefZ_Recipes ein - das schlaegt die Einstufung.
        //----------------------------------------------------------------------
        class ChefZ_Cook
        {
            simpleXp  = 3;
            complexXp = 8;
            premiumXp = 15;

            // <= simpleMaxInputs verbrauchte Zutaten  -> simpleXp
            // <= complexMaxInputs verbrauchte Zutaten -> complexXp
            // darueber                                -> premiumXp
            simpleMaxInputs  = 2;
            complexMaxInputs = 5;

            // Ausnahmen je Rezept-ID. Leer gelassen: der Rezeptsatz von V1
            // wird vollstaendig von der Einstufung oben getragen.
            class ChefZ_Recipes
            {
            };
        };

        //----------------------------------------------------------------------
        // Verarbeitung. Schluessel ist die PROZESS-ID; ChefZ_Transforms
        // schlaegt sie fuer einzelne Transform-IDs.
        //
        // Die Werte sind so gewaehlt, dass die SUMME EINER KETTE dem Wert aus
        // §26 entspricht - §26 nennt Ergebnisse ("Salz herstellen 5"), ChefZ
        // meldet Einzelschritte:
        //
        //   Salz          PROCESS_BOIL_BRINE 3 + PROCESS_DRY_SALT 2   = 5
        //   Nudeln        PROCESS_ROLL 2 + TR_PastaDoughToRawPasta 3  = 5
        //   Trockenfleisch PROCESS_SALT_CURE 2 + PROCESS_DRY 3        = 5
        //   Raeucherwurst PROCESS_STUFF_SAUSAGE 5 + PROCESS_SMOKE 3   = 8
        //   Fisch raeuchern  TR_FishToSmoked 5 (einstufig, kein Salzen) = 5
        //
        // Einstufige Eintraege aus §26 stehen unveraendert:
        //   Kraeuter/Pfeffer trocknen 3, Gewuerz mahlen 2, Mehl mahlen 3,
        //   Teig herstellen 3, Wurst herstellen 5.
        //----------------------------------------------------------------------
        class ChefZ_Process
        {
            // Was in dieser Liste nicht steht, gibt so viel:
            defaultXp = 1;

            class ChefZ_Processes
            {
                // Zerkleinern und Zurichten - Handgriffe, kein Erzeugnis.
                PROCESS_CHOP_VEGETABLE = 1;
                PROCESS_CUT_MEAT       = 1;
                PROCESS_CLEAN_CASING   = 1;
                PROCESS_CARVE_BOWL     = 1;
                PROCESS_CARVE_PLATE    = 1;

                // ANTI-EXPLOIT, ausdruecklich 0: Samen aus Gemuese schneiden
                // ist der einzige Schritt im gesamten Datensatz, der eine
                // KREISFOERMIGE Kette schliesst (Zwiebel -> Samen -> pflanzen
                // -> Zwiebel). Jede Zahl > 0 waere eine, wenn auch langsame,
                // XP-Schleife. Siehe Kopf von ChefZ_TerjeProgressSink.c.
                PROCESS_CUT_OUT_SEEDS  = 0;

                PROCESS_GRIND_MEAT     = 2;
                PROCESS_SEPARATE_CREAM = 2;

                // "Gewuerz mahlen" - §26.
                PROCESS_GRIND_HERB     = 2;
                PROCESS_GRIND_SPICE    = 2;

                PROCESS_CHURN_BUTTER   = 3;
                PROCESS_PRESS_CHEESE   = 3;

                // "Salz aus Salzwasser herstellen 5", auf zwei Schritte.
                PROCESS_BOIL_BRINE     = 3;
                PROCESS_DRY_SALT       = 2;

                // "Mehl mahlen 3", "Teig herstellen 3".
                PROCESS_MILL           = 3;
                PROCESS_KNEAD          = 3;

                // Teil von "Nudeln herstellen 5", Rest in ChefZ_Transforms.
                PROCESS_ROLL           = 2;

                // "Wurst herstellen 5".
                PROCESS_STUFF_SAUSAGE  = 5;

                // Vorstufe des Konservierens.
                PROCESS_SALT_CURE      = 2;

                // "Kraeuter trocknen 3" / "Pfeffer trocknen 3" - und zugleich
                // der zweite Schritt von "Trockenfleisch 5".
                PROCESS_DRY            = 3;

                // Zweiter Schritt von "Raeucherwurst 8".
                PROCESS_SMOKE          = 3;
            };

            // Ausnahmen je Transform-ID. Schlaegt ChefZ_Processes.
            class ChefZ_Transforms
            {
                // "Nudeln herstellen 5": PROCESS_ROLL 2 (Teig ausrollen)
                // + 3 hier (Nudeln schneiden).
                TR_PastaDoughToRawPasta = 3;

                // "Fisch raeuchern 5". Einstufig: der Fisch geht ohne
                // Salzschritt in den Raeucherofen, deshalb traegt dieser
                // eine Transform die ganzen 5.
                TR_FishToSmoked = 5;
            };
        };

        //----------------------------------------------------------------------
        // Kraeuterernte. §26 "Kraut ernten 2-5", §10 "seltenes Kraut 4-5",
        // "Pfeffer ernten 5".
        //
        // XP gibt es je ERNTEVORGANG, nicht je Kraut. Der Mengenbonus oben
        // gilt auch hier.
        //
        // Voraussetzung fuer JEDEN Eintrag: die geerntete Klasse traegt eines
        // der Tags aus ChefZ_HarvestTags. Einzelne Kraeuterklassen stehen
        // bewusst nicht im Skript.
        //----------------------------------------------------------------------
        class ChefZ_Harvest
        {
            defaultXp = 2;

            // Tags, die eine Ernte ueberhaupt erst survival-relevant machen.
            harvestTags[] = {"CHEFZ_HERB", "CHEFZ_SPICE"};

            class ChefZ_Classes
            {
                // Selten und langsam (Rosmarin 2100 s Reifezeit).
                ChefZ_Rosemary      = 5;

                // "Pfeffer ernten 5" - §10.
                ChefZ_PepperBerries = 5;
            };
        };
    };

    //--------------------------------------------------------------------------
    // Der Kraeuterkundige. Terje-Analyse §7 und §35.
    //
    // Der Perk fragt AUSSCHLIESSLICH das Food-Tag CHEFZ_HERB ab, nie eine
    // einzelne Kraeuterklasse. Neue Kraeuter wirken damit ohne eine Zeile
    // Code hier.
    //--------------------------------------------------------------------------
    class ChefZ_Herb
    {
        // Das Tag aus ChefZ_Registry/Config/Tags.json.
        tag = "CHEFZ_HERB";

        // Hervorheben. Technisches Vorbild: TerjeSkills/Scripts/4_World/
        // Entities/MushroomBase.c - IsTerjeClientUpdateRequired() +
        // OnTerjeClientUpdate() + ParticleManager.PlayOnObject().
        highlightEnabled = 1;

        // Erkennungsreichweite je Perkstufe, in Metern (§7: Stufe I nah,
        // III und IV groesser, V maximal). Genau stagesCount Eintraege.
        highlightRange[] = {12, 12, 20, 30, 45};

        // Reifende Pflanzen mit hervorheben, nicht nur erntereife. 0 = nur
        // erntereife - das ist der Sinn des Perks.
        highlightUnripe = 0;

        // Ausbeutebonus. Der Faktor kommt aus values[] des Perks, nicht von
        // hier; dieser Schalter erlaubt es nur, ihn ganz abzuschalten.
        yieldEnabled = 1;
    };

    //--------------------------------------------------------------------------
    // Faehigkeitsanbieter fuer ChefZ_CapabilityRegistry.
    //
    // WICHTIG: das sind KEINE Recipe Locks. Dieses Modul beantwortet nur die
    // Frage "welchen Wert hat Faehigkeit X fuer Spieler Y". Ob und wie hart
    // ein Rezept daran gebunden wird, entscheidet der REZEPTAUTOR ueber
    // "requires" im Rezeptdatensatz - und diese Haerte ist in
    // design/FINAL/OFFENE_ENTSCHEIDUNGEN.md unter OF-08 noch OFFEN.
    // Solange sie offen ist, wird hier keine Sperre gebaut. Im gesamten
    // ChefZ-Datenbestand steht derzeit kein einziges "requires", der Anbieter
    // wird also aktuell nie gefragt.
    //--------------------------------------------------------------------------
    class ChefZ_Capabilities
    {
        enabled = 1;

        // Prioritaet im ChefZ_CapabilityRegistry. Hoechste gewinnt.
        priority = 100;

        // Jede Unterklasse beschreibt EINEN Faehigkeitsnamen. Der Name steht
        // im Feld "name", nicht im Klassennamen - die Namenskonvention des
        // Projekts (ChefZ_PascalCase) gilt auch fuer Configklassen.
        //   source = "skill"     -> Skill-Level (0..50)
        //   source = "perkLevel" -> Perkstufe (0..stagesCount)
        //   source = "perkValue" -> values[] der aktiven Perkstufe
        class ChefZ_Names
        {
            class ChefZ_CapSurvival
            {
                name = "CHEFZ_CAP_SURVIVAL";
                source = "skill";
                skill = "surv";
            };

            class ChefZ_CapHerbalist
            {
                name = "CHEFZ_CAP_HERBALIST";
                source = "perkLevel";
                skill = "surv";
                perk = "chefzherb";
            };

            class ChefZ_CapHerbalistYield
            {
                name = "CHEFZ_CAP_HERBALIST_YIELD";
                source = "perkValue";
                skill = "surv";
                perk = "chefzherb";
            };
        };
    };
};
