// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeSkills
// geladen ist. Fehlt der Mod, ist TERJE_SKILLS_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_SKILLS_MOD
//==============================================================================
// ChefZ_TerjeCapabilityProvider - Auskunft, KEINE Sperre
//
// ---------------------------------------------------------------------------
// Was diese Datei ausdruecklich NICHT ist
// ---------------------------------------------------------------------------
// Sie baut keine Recipe Locks. Die Haerte von Rezeptsperren ist in
// Psyerns_ChefZ_Docs/design/FINAL/OFFENE_ENTSCHEIDUNGEN.md unter OF-08
// ("Wie hart sind Recipe Locks?") als OFFEN gefuehrt. Solange das so ist,
// wird hier nichts gesperrt, nichts herabgestuft und keine Ausbeute gekuerzt.
//
// Diese Klasse beantwortet ausschliesslich die Frage, die
// ChefZ_ICapabilityProvider stellt: "welchen Wert hat Faehigkeit X fuer
// Spieler Y". WAS mit der Antwort geschieht, entscheidet allein der
// Rezeptautor ueber "requires" im Rezeptdatensatz - ein Feld, das im
// gesamten ChefZ-Datenbestand derzeit an keiner Stelle vorkommt. Solange
// niemand es benutzt, wird dieser Anbieter nie gefragt; er ist die Steckdose,
// nicht das Geraet.
//
// Der Betreiber kann ihn ueber CfgChefZTerjeSkills ChefZ_Capabilities
// enabled = 0 ganz abmelden.
//
// ---------------------------------------------------------------------------
// Fundstellen
// ---------------------------------------------------------------------------
//   ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_CapabilityRegistry.c:69
//       class ChefZ_ICapabilityProvider
//         string GetProviderName()
//         int    GetPriority()
//         bool   TryGetCapability(int identityId, ChefZ_Sym capability,
//                                 out float value)
//       -> "false, wenn dieser Anbieter zu dieser Faehigkeit nichts sagen
//          kann. Dann wird der naechste Anbieter gefragt."
//       -> "identityId 0 heisst 'niemand beteiligt'. Ein Anbieter, der einen
//          Spieler braucht, antwortet dann mit false."
//
// Layer: 4_World.
//==============================================================================

//! Eine Zeile aus CfgChefZTerjeSkills ChefZ_Capabilities ChefZ_Names.
class ChefZ_TerjeCapabilityDef
{
    string source;      // "skill" | "perkLevel" | "perkValue"
    string skillId;
    string perkId;

    void ChefZ_TerjeCapabilityDef()
    {
        source  = "";
        skillId = "";
        perkId  = "";
    }
}

//==============================================================================

class ChefZ_TerjeCapabilityProvider extends ChefZ_ICapabilityProvider
{
    private ref map<string, ref ChefZ_TerjeCapabilityDef> m_Defs;

    void ChefZ_TerjeCapabilityProvider()
    {
        m_Defs = new map<string, ref ChefZ_TerjeCapabilityDef>();
        LoadDefs();
    }

    override string GetProviderName()
    {
        return "ChefZ_TerjeSkills";
    }

    override int GetPriority()
    {
        return ChefZ_TerjeSkillsConfig.CapabilityPriority();
    }

    override bool TryGetCapability(int identityId, ChefZ_Sym capability, out float value)
    {
        value = 0.0;

        // Ein Anbieter, der einen Spieler braucht, antwortet ohne Spieler mit
        // false - so steht es woertlich in der Schnittstelle.
        if (identityId == 0)
            return false;
        if (!g_Game || !g_Game.IsServer())
            return false;
        if (!ChefZ_TerjeSkillsConfig.CapabilitiesEnabled())
            return false;

        string name = ChefZ_SymbolTable.Name(capability);
        if (name == "")
            return false;

        ChefZ_TerjeCapabilityDef def;
        if (!m_Defs.Find(name, def) || !def)
            return false;                   // nicht unsere Faehigkeit

        PlayerBase player = ChefZ_TerjeSkillsBridge.FindPlayerByIdentityId(identityId);
        if (!player)
            return false;

        if (def.source == "skill")
        {
            TerjePlayerSkillsAccessor skills = ChefZ_TerjeSkillsBridge.SkillsOf(player);
            if (!skills)
                return false;
            value = skills.GetSkillLevel(def.skillId);
            return true;
        }

        if (def.source == "perkLevel")
        {
            value = ChefZ_TerjeSkillsBridge.PerkLevel(player, def.skillId, def.perkId);
            return true;
        }

        if (def.source == "perkValue")
        {
            value = ChefZ_TerjeSkillsBridge.PerkValue(player, def.skillId, def.perkId);
            return true;
        }

        // Unbekannte Quelle: lieber schweigen als raten. Der naechste Anbieter
        // bekommt seine Chance, und ohne einen solchen greift der Default aus
        // ChefZ_CoreSettingsDef.
        return false;
    }

    //==========================================================================

    /**
     * Die Tabelle aus der config.cpp lesen.
     *
     * Einmal bei der Anmeldung. Der Faehigkeitsname steht im FELD "name" und
     * nicht im Klassennamen - Configklassen dieses Projekts folgen der
     * Namenskonvention ChefZ_PascalCase, Faehigkeitsnamen dagegen der
     * Schreibweise des Core (CHEFZ_CAP_*).
     */
    private void LoadDefs()
    {
        string basePath = "CfgChefZTerjeSkills ChefZ_Capabilities ChefZ_Names";

        int count = GetTerjeGameConfig().ConfigGetChildrenCount(basePath);
        for (int i = 0; i < count; i++)
        {
            string child;
            if (!GetTerjeGameConfig().ConfigGetChildName(basePath, i, child))
                continue;

            string path = basePath + " " + child;

            string capName;
            if (!GetTerjeGameConfig().ConfigGetTextRaw(path + " name", capName))
                continue;
            capName.TrimInPlace();
            if (capName == "")
                continue;

            // Ueber LOKALE Variablen: out-Parameter auf Klassenfelder sind
            // in Enforce vermeidbar und hier ohne jeden Gewinn.
            string source;
            string skillId;
            string perkId;
            GetTerjeGameConfig().ConfigGetTextRaw(path + " source", source);
            GetTerjeGameConfig().ConfigGetTextRaw(path + " skill",  skillId);
            GetTerjeGameConfig().ConfigGetTextRaw(path + " perk",   perkId);

            source.TrimInPlace();
            skillId.TrimInPlace();
            perkId.TrimInPlace();

            if (skillId == "")
                skillId = ChefZ_TerjeSkillsBridge.SKILL_SURVIVAL;

            ChefZ_TerjeCapabilityDef def = new ChefZ_TerjeCapabilityDef();
            def.source  = source;
            def.skillId = skillId;
            def.perkId  = perkId;

            m_Defs.Set(capName, def);
        }
    }

    int GetDefCount()
    {
        return m_Defs.Count();
    }
}
#endif // TERJE_SKILLS_MOD
