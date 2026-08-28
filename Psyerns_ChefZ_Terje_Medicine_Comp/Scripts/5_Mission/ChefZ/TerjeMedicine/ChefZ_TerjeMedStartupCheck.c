// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeMedicine
// geladen ist. Fehlt der Mod, ist TERJE_MEDICINE_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_MEDICINE_MOD
// ============================================================================
// Startpruefung des Compatibility-Moduls (nur Server, nur Logausgabe).
//
// Dieses Modul beschreibt Wirkungen fuer Item-Klassen, die im Hauptmod noch
// NICHT existieren (siehe Blocker im Kopf der config.cpp). Ein schlafender
// Eintrag ist harmlos - aber er ist auch unsichtbar. Diese Pruefung macht ihn
// beim Serverstart im RPT sichtbar, damit niemand glaubt, die Tees wirkten
// bereits.
//
// Zusaetzlich prueft sie die eine Regel, deren Verletzung man im Spiel NICHT
// bemerken wuerde: doppelte XP. TerjeCore vergibt Skill-XP fuer jedes
// Consumable, das "<skillId>SkillExpAddToSelf" oder "...AddToTarget" traegt
// (TerjeCore/Scripts/4_World/Classes/Medicine/TerjeConsumableEffects.c:11-32).
// Essen und Trinken geben aber bereits automatisch Metabolism-XP ueber
// PlayerStomach (TerjeSkills/Scripts/4_World/Classes/PlayerStomach.c:38-52).
// Traegt eine Tee-Klasse eines Tages so einen Parameter, waere das eine zweite
// XP-Quelle fuer dieselbe Handlung - lautlos, und nur an einer ungewoehnlich
// schnell steigenden Skillleiste zu erkennen. Also wird es hier laut gemeldet.
//
// Die Pruefung liest ausschliesslich. Sie aendert nichts, weder an ChefZ noch
// an Terje, und laeuft genau einmal.
// ============================================================================

modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        ChefZ_TerjeMed_RunStartupCheck();
    }

    protected void ChefZ_TerjeMed_RunStartupCheck()
    {
        ChefZ_TerjeMedRegistry registry = GetChefZTerjeMedRegistry();
        int count = registry.GetCount();

        if (count == 0)
        {
            TerjeLog_Warning("ChefZ_TerjeMedicineComp: CfgChefZTerjeMedicine ist leer - dieses Modul hat nichts zu tun.");
            return;
        }

        // Die Wurzelknoten, unter denen ein konsumierbares Item ueberhaupt
        // liegen kann. Beide werden von TerjeCore als Consumable-Pfad gebaut
        // (TerjeCore/Scripts/4_World/Entities/ItemBase.c:298-315).
        int missing = 0;
        for (int i = 0; i < count; i++)
        {
            string id = registry.GetIdAt(i);

            bool asItem = GetTerjeGameConfig().ConfigIsExisting("CfgVehicles " + id);
            bool asLiquid = GetTerjeGameConfig().ConfigIsExisting("CfgTerjeCustomLiquids " + id);

            if (!asItem && !asLiquid)
            {
                missing++;
                TerjeLog_Warning("ChefZ_TerjeMedicineComp: '" + id + "' hat hinterlegte Medizinwerte, aber im Hauptmod existiert weder CfgVehicles noch CfgTerjeCustomLiquids dazu. Die Wirkung bleibt schlafend.");
                continue;
            }

            if (asItem)
            {
                ChefZ_TerjeMed_CheckForDoubleExp(id);
            }
        }

        if (missing == 0)
        {
            TerjeLog_Info("ChefZ_TerjeMedicineComp: " + count + " Kraeutertee-Wirkung(en) aktiv.");
        }
        else
        {
            TerjeLog_Warning("ChefZ_TerjeMedicineComp: " + missing + " von " + count + " Eintraegen ohne Item im Hauptmod.");
        }
    }

    // Sucht auf der Item-Klasse nach XP-Parametern, die TerjeCore auswerten
    // wuerde. Gefunden werden sie ueber die tatsaechlich registrierten Skills,
    // nicht ueber eine fest verdrahtete Liste - so erfasst die Pruefung auch
    // Skills, die ein anderes Modul nachtraegt.
    protected void ChefZ_TerjeMed_CheckForDoubleExp(string id)
    {
        array<ref TerjeSkillCfg> skills = new array<ref TerjeSkillCfg>;
        GetTerjeSkillsRegistry().GetSkills(skills);

        foreach (ref TerjeSkillCfg skill : skills)
        {
            if (!skill)
            {
                continue;
            }

            string cfgBase = "CfgVehicles " + id + " " + skill.GetId();

            if (GetTerjeGameConfig().ConfigGetFloat(cfgBase + "SkillExpAddToSelf") >= 1)
            {
                TerjeLog_Error("ChefZ_TerjeMedicineComp: '" + id + "' traegt " + skill.GetId() + "SkillExpAddToSelf. Essen und Trinken geben bereits Metabolism-XP ueber PlayerStomach - das ist eine zweite XP-Quelle fuer dieselbe Handlung. Parameter entfernen.");
            }

            if (GetTerjeGameConfig().ConfigGetFloat(cfgBase + "SkillExpAddToTarget") >= 1)
            {
                TerjeLog_Error("ChefZ_TerjeMedicineComp: '" + id + "' traegt " + skill.GetId() + "SkillExpAddToTarget. Siehe oben - Parameter entfernen.");
            }
        }
    }
}
#endif // TERJE_MEDICINE_MOD
