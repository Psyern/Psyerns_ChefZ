// ============================================================================
// Psyerns_ChefZ_Terje_Medicine_Comp
//
// Optionale Bruecke zwischen Psyerns_ChefZ und TerjeMedicine.
//
// GRUNDREGEL DES MEILENSTEINS: ChefZ laeuft ohne dieses Modul vollstaendig und
// unveraendert. Dieses PBO ist ein EIGENER Mod. Wird es nicht geladen, existiert
// weder CfgChefZTerjeMedicine noch die modded class - es kann nichts greifen und
// nichts abstuerzen. Terje-Dateien werden nirgends veraendert, nur erweitert.
//
// ----------------------------------------------------------------------------
// WARUM DIE WERTE HIER STEHEN UND NICHT AUF DEN ITEM-KLASSEN
// ----------------------------------------------------------------------------
// TerjeMedicine liest seine Consumable-Parameter woertlich so:
//
//     GetTerjeGameConfig().ConfigGetFloat( classname + " medImmunityGainForce" )
//     TerjeMedicine/Scripts/4_World/Classes/TerjeConsumableEffects.c:320-341
//
// wobei "classname" aus TerjeCore stammt und "CfgVehicles <ItemTyp>" lautet
// (TerjeCore/Scripts/4_World/Entities/ItemBase.c:294-316). Der idiomatische Weg
// waere also, "CfgVehicles/ChefZ_ThymeTea" hier ein zweites Mal zu oeffnen und
// die med*-Parameter anzuhaengen - genau das tut TerjeMedicine/FixVanilla mit
// VitaminBottle.
//
// Dieser Weg ist im Projekt Psyerns_ChefZ VERSPERRT: die Teeklassen gehoeren dem
// Hauptmod, und tools/chefz-validate/configcpp.mjs meldet jeden Klassenpfad, der
// zweimal mit Rumpf definiert ist, als Fehler ("die spaetere ueberschreibt die
// fruehere still"). Ein Compatibility-Modul darf eine ChefZ-Klasse deshalb NICHT
// per Config-Patch nachbearbeiten.
//
// Loesung: ein eigener Config-Wurzelknoten. "CfgChefZTerjeMedicine/ChefZ_ThymeTea"
// ist ein anderer Pfad als "CfgVehicles/ChefZ_ThymeTea" - keine Kollision, kein
// Doppeleintrag, und das Hauptmod bleibt unangetastet. Die Parameter heissen
// bewusst EXAKT wie bei Terje, damit
//   a) jeder, der Terje kennt, sie ohne Uebersetzung liest, und
//   b) $profile:TerjeSettings/Core/GameOverrides.xml sie ueber denselben Pfad
//      ueberschreiben kann - GetTerjeGameConfig() bedient jeden Config-Wurzel-
//      knoten gleich (TerjeCore/Scripts/3_Game/TerjeGameConfig.c:150-163).
//
// Gelesen und angewendet wird das Ganze in
//   Scripts/4_World/ChefZ/TerjeMedicine/ChefZ_TerjeMedConsumableEffects.c
// nach dem Muster von TerjeRadiation/Scripts/4_World/Classes/TerjeConsumableEffects.c.
//
// ----------------------------------------------------------------------------
// STATUS DER ITEMS - BLOCKER
// ----------------------------------------------------------------------------
// ChefZ_ThymeTea, ChefZ_WildGarlicTea und ChefZ_HerbalTea existieren im
// Hauptmod NOCH NICHT (Stand: keine CfgVehicles-Klasse in
// Psyerns_ChefZ_Core/Addons/**). Die Items gehoeren nach ChefZ und werden hier
// bewusst NICHT angelegt.
//
// Die Eintraege unten sind deshalb SCHLAFEND: ohne passende Item-Klasse wird
// nie ein solches Item konsumiert, der Resolver findet nie einen Treffer, und
// es passiert exakt nichts. Sobald das Hauptmod die Klassen liefert, greifen
// sie ohne weitere Aenderung. ChefZ_TerjeMedStartupCheck meldet beim Serverstart
// im RPT, welche der drei Klassen fehlt.
//
// UEBERGABEVERTRAG an den Eigentuemer der Tee-Items (Production Map §68):
//   1. Klassenname exakt wie hier - der Resolver schluesselt darauf.
//   2. Die Tee-Klasse traegt KEINE "<skillId>SkillExpAddToSelf"- oder
//      "...AddToTarget"-Parameter. TerjeCore vergibt daraus Skill-XP
//      (TerjeCore/.../TerjeConsumableEffects.c:18-31); Essen und Trinken geben
//      bereits automatisch Metabolism-XP ueber PlayerStomach
//      (TerjeSkills/Scripts/4_World/Classes/PlayerStomach.c:38-52). Zweite
//      XP-Quelle = doppelte XP.
//   3. Ein Tee ist ein Edible_Base-Item ODER ein Terje-Custom-Liquid. Beides
//      wird unterstuetzt; "chefzServingSize" sagt, wie viele "amount"-Einheiten
//      EINE volle Portion sind (Stueckware: 1, Fluessigkeit: ml je Tasse).
//
// ----------------------------------------------------------------------------
// FOOD-RISIKEN (Analyse §19) - HIER BEWUSST LEER
// ----------------------------------------------------------------------------
// FOOD_POISON (16) bei verdorbenem Essen und SALMONELLA (4) bei rohem Fleisch
// sind im Hauptmod BEREITS gesetzt, in nutrition_properties[] Index 5, siehe
// z.B. Psyerns_ChefZ_Core/Addons/ChefZ_Meat/config.cpp:284-288 (Raw = 4,
// Rotten = 16). Terje uebernimmt diese Vanilla-Agenten von selbst:
// TerjePlayerModifierPoison.c:75-84 rechnet FOOD_POISON, SALMONELLA und CHOLERA
// ueber TransferVanillaAgents in seinen Poison-Wert um, gedaempft durch
// Immunitaet und - fuer FOOD_POISON - durch den Perk "immunity/svdinner".
//
// Hier etwas hinzuzufuegen waere genau die verbotene Doppelung: ein zweiter
// Food-Poison-Pfad neben svdinner. Dieses Modul fasst Agenten deshalb NICHT an.
//
// CHOLERA hat in ChefZ derzeit keinen Traeger - es gibt kein ChefZ-Wasser- oder
// Fluessigkeitsitem. Bleibt offen, siehe Bericht.
// ============================================================================

class CfgPatches
{
    class Psyerns_ChefZ_Terje_Medicine_Comp
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;

        // TerjeCore liefert TerjeConsumableEffects, TerjeGameConfig und die
        // Accessoren; TerjeMedicine liefert TerjePlayerStats mit
        // SetImmunityGainValue/SetHealthExtraRegenTimer und die modded class,
        // auf die wir aufsetzen. ChefZ_Core sichert die Ladereihenfolge
        // gegenueber dem Hauptmod.
        requiredAddons[] = {"TerjeCore", "TerjeMedicine", "ChefZ_Core"};
    };
};

class CfgMods
{
    // Klassenname nach Projekt-Namenskonvention (Workflow §10.7,
    // ChefZ_PascalCase). "dir" ist und bleibt der ORDNERNAME - das PBO-Praefix
    // in $PREFIX$ lautet identisch, sonst ueberspringt DayZ die Skriptmodule
    // still und ohne RPT-Eintrag.
    class ChefZ_TerjeMedicineComp
    {
        dir = "Psyerns_ChefZ_Terje_Medicine_Comp";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ChefZ Terje Medicine Compatibility";
        credits = "Psyern";
        author = "Psyern";
        authorID = "0";
        version = "0.0.1";
        extra = 0;
        type = "mod";
        dependencies[] = {"Game", "World", "Mission"};

        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] = {"Psyerns_ChefZ_Terje_Medicine_Comp/Scripts/4_World"};
            };

            class missionScriptModule
            {
                value = "";
                files[] = {"Psyerns_ChefZ_Terje_Medicine_Comp/Scripts/5_Mission"};
            };
        };
    };
};

// ============================================================================
// CfgChefZTerjeMedicine - die Wirkungstabelle der Kraeutertees
// ============================================================================
//
// MASSSTAB. Referenz ist Terjes eigenes Vitaminpraeparat
// (TerjeMedicine/FixVanilla/config.cpp:238-245):
//
//     VitaminBottle:  medImmunityGainForce = 1
//                     medImmunityGainTimeSec = 120   (je Einheit)
//                     medImmunityGainMaxTimer = 1800
//
// Was daraus wird, rechnet TerjePlayerModifierImmunity.c:20 aus:
//
//     internalImmunity += 0.25 * force * dt * 0.001
//     (0.25 = Medicine.MedicineInternalImmunityGainMod, Standardwert aus
//      TerjeSettingsCollection.c:639)
//
// Volle Wirkung ueber die gesamte Laufzeit, Innere Immunitaet ist auf 1.0
// begrenzt (TerjePlayerStats.c:1139, Math.Clamp(value, 0, 1)):
//
//     VitaminBottle     0.25 * 1.00 * 1800 * 0.001 = 0.450   = 100 %
//     WildGarlicTea     0.25 * 0.40 *  600 * 0.001 = 0.060   =  13 %
//     HerbalTea         0.25 * 0.30 *  480 * 0.001 = 0.036   =   8 %
//     ThymeTea          0.25 * 0.20 *  360 * 0.001 = 0.018   =   4 %
//
// Damit liegt der staerkste Tee bei einem Achtel der Vitamine - "deutlich
// unter den Vanilla-Vitaminen", wie im Auftrag verlangt. Ein Tee ersetzt kein
// Medikament; er wirkt praeventiv, nicht heilend.
//
// WICHTIG, warum der Force-Wert klein bleiben MUSS: Terje setzt den neuen Wert
// nur, wenn "medImmunityGainForce >= aktive Force" (TerjeConsumableEffects.c:333).
// Ein Tee mit Force 1.0 wuerde ein laufendes Vitaminpraeparat ueberschreiben
// duerfen. Mit 0.20-0.40 kann Tee eine echte Medikation nie verdraengen -
// umgekehrt verdraengt das Medikament den Tee sofort. Das ist die Rangordnung,
// die Terjes Medizinsystem braucht.
//
// GESUNDHEITSREGENERATION. TerjePlayerModifierHealthGain.c:16-21 regeneriert
//     0.5 HP/s * (1 - health01)   (Medicine.HealthRegenMedsPerSec = 0.5,
//                                  TerjeSettingsCollection.c:399)
// solange der Timer laeuft. Zum Vergleich, Terjes eigene Injektoren
// (TerjeMedicine/Injectors/config.cpp:245,259):
//     Reanimatal 180 s, Propital 45 s.
// Die Tees bleiben mit einem Deckel von 40-45 s unter dem SCHWAECHSTEN
// Injektor, und eine einzelne Tasse gibt 15-20 s. Bei halber Gesundheit sind
// das rund 3-5 HP je Tasse. Leicht regenerativ, mehr nicht.
//
// PHARMACOLOGIST. Die Zeiten werden von TerjeMedicine selbst mit
// (1.0 + Perkwert med/pharmac) multipliziert (TerjeConsumableEffects.c:8-17).
// ChefZ_TerjeMedConsumableEffects.c wendet exakt denselben Modifikator an -
// die Tees interagieren dadurch ohne Zusatzcode mit dem Perk. Der Deckel
// (medImmunityGainMaxTimer) begrenzt den Gewinn: Pharmacologist spart Tassen,
// er hebt die Obergrenze nicht.
//
// KEINE XP. Kein Eintrag traegt "<skillId>SkillExpAddToSelf"/"AddToTarget" -
// Essen und Trinken geben bereits automatisch Metabolism-XP. Dass laufender
// Immunity-Gain im Minutentakt Immunity-XP erzeugt
// (TerjePlayerModifierImmunity.c:24-34), ist Terjes eigene Mechanik, kein
// Beitrag dieses Moduls; der niedrige Zeitdeckel begrenzt sie zusaetzlich.

class CfgChefZTerjeMedicine
{
    // Thymian - der "Hustentee". Schwaechster Immunitaetsschub, dafuer als
    // einziger neben dem Kraeutertee eine kleine Erholungswirkung.
    class ChefZ_ThymeTea
    {
        chefzServingSize = 1;

        medImmunityGainForce = 0.20;
        medImmunityGainTimeSec = 180;
        medImmunityGainMaxTimer = 360;

        medHealthgainTimeSec = 20;
        medHealthgainMaxTimeSec = 45;
    };

    // Baerlauch - der Immunitaetstee der Analyse §22. Staerkster Force-Wert des
    // Moduls, laengste Laufzeit, bewusst OHNE Regeneration: er soll vorbeugen,
    // nicht heilen.
    class ChefZ_WildGarlicTea
    {
        chefzServingSize = 1;

        medImmunityGainForce = 0.40;
        medImmunityGainTimeSec = 300;
        medImmunityGainMaxTimer = 600;

        medHealthgainTimeSec = 0;
        medHealthgainMaxTimeSec = 0;
    };

    // Kraeutermischung - der Allrounder aus Analyse §21. Mittlerer
    // Immunitaetswert plus eine Spur Regeneration.
    class ChefZ_HerbalTea
    {
        chefzServingSize = 1;

        medImmunityGainForce = 0.30;
        medImmunityGainTimeSec = 240;
        medImmunityGainMaxTimer = 480;

        medHealthgainTimeSec = 15;
        medHealthgainMaxTimeSec = 40;
    };
};
