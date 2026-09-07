// ---------------------------------------------------------------------------
// WEICHE ABHAENGIGKEIT: alles unterhalb existiert nur, wenn TerjeSkills
// geladen ist. Fehlt der Mod, ist TERJE_SKILLS_MOD nicht gesetzt, der
// Praeprozessor entfernt den gesamten Rumpf, und es bleibt eine leere Datei
// ohne unaufloesbare Bezeichner. Begruendung, Beleg und Vorbilder stehen im
// Kopf der config.cpp, Abschnitt "WEICHE ABHAENGIGKEIT".
// ---------------------------------------------------------------------------
#ifdef TERJE_SKILLS_MOD
//==============================================================================
// ChefZ_TerjeHerbTag - "ist das ein Kraut?" wird EINMAL beantwortet
//
// Terje-Analyse §9: das Compatibility-Modul prueft NICHT jede Kraeuterklasse
// einzeln, sondern fragt das ChefZ-Tag CHEFZ_HERB ab. Ein neues Kraut wirkt
// damit ohne eine Zeile Code hier - es muss nur in seinem eigenen
// Zutatendatensatz das Tag tragen.
//
// Fundstellen im ChefZ-Core:
//   ChefZ_Core/Scripts/3_Game/ChefZ/ChefZ_IngredientManager.c
//       static ChefZ_IngredientManager Get()
//       bool IsReady()
//       ChefZ_IngredientInfo ResolveByName(string className)
//   ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_IngredientInfo.c:77
//       bool HasTag(ChefZ_Sym tag)
//   ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_SymbolTable.c:79
//       static ChefZ_Sym Lookup(string name)   - Lookup, NICHT Intern:
//       eine Abfrage darf keine Symbole anlegen.
//
// Das Tag selbst steht in ChefZ_Registry/Config/Tags.json ("CHEFZ_HERB").
//
// Der Client kennt dieselben Daten: ChefZ_Boot.OnClientStart() laedt Rang 1
// und 2 aus denselben PBOs. Deshalb funktioniert diese Abfrage auch im
// Hervorhebungspfad, der rein clientseitig laeuft.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_TerjeHerbTag
{
    //! Symbol des Kraeuter-Tags. Erst beim ersten Zugriff aufgeloest: beim
    //! Laden dieser Klasse ist die Symboltabelle noch leer.
    private static ChefZ_Sym s_HerbSym = ChefZ_SymbolTable.INVALID;
    private static bool      s_HerbResolved;

    private static ref array<ChefZ_Sym> s_HarvestSyms;

    /**
     * Traegt die Klasse das Kraeuter-Tag?
     *
     * false ist die haeufige, normale Antwort - fuer jede Vanilla-Klasse, fuer
     * jede ChefZ-Klasse ohne das Tag und fuer jeden Zeitpunkt, an dem die
     * Zutatendaten noch nicht stehen. Nie ein Fehler, nie ein Log: diese
     * Funktion laeuft clientseitig im Sekundentakt.
     */
    static bool IsHerb(string className)
    {
        if (!s_HerbResolved)
        {
            s_HerbSym = ChefZ_SymbolTable.Lookup(ChefZ_TerjeSkillsConfig.HerbTag());

            // Erst als aufgeloest merken, wenn es wirklich etwas aufzuloesen
            // gab. Sonst bliebe ein zu frueher Aufruf (vor dem Laden der
            // Tag-Registry) fuer immer bei INVALID stehen.
            if (ChefZ_SymbolTable.IsValid(s_HerbSym))
                s_HerbResolved = true;
        }

        if (!ChefZ_SymbolTable.IsValid(s_HerbSym))
            return false;

        return HasTagSym(className, s_HerbSym);
    }

    /**
     * Traegt die Klasse eines der Tags, die eine Ernte survival-relevant
     * machen? (config.cpp: ChefZ_Harvest harvestTags[])
     *
     * HEUTE OHNE AUFRUFER (31.08.2026): es gibt keinen Erntepfad, der XP
     * vergibt - der Core kennt keine Fortschrittsart "harvest", und die
     * Wildernte zahlt laut Wildwuchs-Spec §5 0 XP. Die Funktion bleibt als
     * Gegenstueck zur ebenfalls beschrifteten Config stehen; die vollstaendige
     * Begruendung steht an "class ChefZ_Harvest" in der config.cpp.
     */
    static bool IsHarvestRelevant(string className)
    {
        if (!s_HarvestSyms)
            s_HarvestSyms = new array<ChefZ_Sym>();

        array<string> names = ChefZ_TerjeSkillsConfig.HarvestTags();

        // Neu aufloesen, solange noch nicht alle Namen ein Symbol haben. Nach
        // dem Laden der Registry ist das genau einmal der Fall.
        if (s_HarvestSyms.Count() != names.Count())
        {
            s_HarvestSyms.Clear();
            for (int n = 0; n < names.Count(); n++)
            {
                ChefZ_Sym sym = ChefZ_SymbolTable.Lookup(names.Get(n));
                if (ChefZ_SymbolTable.IsValid(sym))
                    s_HarvestSyms.Insert(sym);
                else
                    return false;           // Registry noch nicht bereit
            }
        }

        for (int i = 0; i < s_HarvestSyms.Count(); i++)
        {
            if (HasTagSym(className, s_HarvestSyms.Get(i)))
                return true;
        }

        return false;
    }

    private static bool HasTagSym(string className, ChefZ_Sym tag)
    {
        if (className == "")
            return false;

        ChefZ_IngredientManager mgr = ChefZ_IngredientManager.Get();
        if (!mgr || !mgr.IsReady())
            return false;

        ChefZ_IngredientInfo info = mgr.ResolveByName(className);
        if (!info)
            return false;

        return info.HasTag(tag);
    }
}
#endif // TERJE_SKILLS_MOD
