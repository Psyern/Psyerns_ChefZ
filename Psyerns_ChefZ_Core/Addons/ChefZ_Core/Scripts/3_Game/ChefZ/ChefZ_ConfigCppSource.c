//==============================================================================
// ChefZ_ConfigCppSource - Rang 1: die Game-Config
//
// Entwurf: 02 §2 ("Was der Client wissen muss, steht in der Game-Config"),
// 02 §3 (Rang 1), 02 §4 (die Klassenbaeume), 02 E6 (kein CfgVehicles-Vollscan),
// 03 §4 (sync-relevante Registries kommen AUSSCHLIESSLICH von hier).
//
// Gelesen werden ausschliesslich benannte Wurzelknoten:
//
//   CfgChefZCategories   CfgChefZTags        CfgChefZStates
//   CfgChefZQualityTiers CfgChefZTools       CfgChefZDevices
//   CfgChefZContainers   CfgChefZIngredients
//   CfgChefZProcesses    CfgChefZStations                        (seit S14)
//
// Warum Prozesse und Stationen HIER liegen und nicht nur in JSON (11 E8,
// 02 §2): ChefZ_ActionProcessAtStation.ActionCondition() laeuft auch auf dem
// CLIENT und muss dort wissen, welche Prozesse eine Station anbietet, welche
// Werkzeuggruppe ein Prozess verlangt und wie der Aktionstext heisst. Der
// Client liest Rang 1 garantiert; ob er eine JSON-Datei aus einem PBO lesen
// kann, ist eine offene Messfrage (OF-10).
//
// TRANSFORMS stehen ausdruecklich NICHT hier. Sie tragen Selektoren und
// Ergebnisbeschreibungen - verschachtelte Strukturen, die ein flacher
// Config-Klassenbaum nicht ohne Verrenkungen abbildet - und sie sind eine rein
// SERVERSEITIGE Entscheidung (11 §5).
//
// KEIN CfgVehicles-Vollscan (02 E6): ueber ~10^4 Klassen zu laufen kostet
// unbekannt viel Startzeit fuer null Zusatznutzen, weil die Deklaration
// ohnehin ausdruecklich erfolgt. Die Vererbung entlang der Config-Kette laeuft
// ab S4 nur fuer DEKLARIERTE Klassen (05 E2).
//
// Existiert keiner dieser Knoten, liefert die Quelle null Records und false.
// Das ist der Normalfall auf einem Server, der nur den Core geladen hat - und
// er ist ausdruecklich KEIN Fehler (02 §8, erste Zeile).
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_ConfigCppSource extends ChefZ_IRecordSource
{
    private int m_NodeCount;

    override string GetName()
    {
        return "config.cpp (CfgChefZ*)";
    }

    override int GetRank()
    {
        return ChefZ_SourceRank.CONFIG_CPP;
    }

    override int GetFileCount()
    {
        return m_NodeCount;
    }

    //--------------------------------------------------------------------------

    override bool Read(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        m_NodeCount = 0;
        if (!sink || !g_Game)
            return false;

        int before = sink.GetAcceptedCount();

        ReadCategories(sink, report);
        ReadTags(sink, report);
        ReadStates(sink, report);
        ReadQualityTiers(sink, report);
        ReadToolGroups(sink, report);
        ReadDevices(sink, report);
        ReadContainers(sink, report);
        ReadIngredients(sink, report);
        ReadProcesses(sink, report);
        ReadStations(sink, report);

        return sink.GetAcceptedCount() > before;
    }

    //--------------------------------------------------------------------------
    // Je Knoten eine kleine, ausdrueckliche Leseroutine. Bewusst KEINE
    // generische Feldabbildung: die Config-API ist typisiert
    // (ConfigGetText/Int/Float/TextArray), und ein "lies irgendein Feld"
    // muesste raten, welche davon zustaendig ist.
    //--------------------------------------------------------------------------

    private void ReadCategories(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        string root = "CfgChefZCategories";
        array<string> names = ChildNames(root);
        for (int i = 0; i < names.Count(); i++)
        {
            string id = names.Get(i);
            string node = root + " " + id;

            ChefZ_CategoryDef rec = new ChefZ_CategoryDef();
            rec.id          = id;
            rec.parent      = Text(node + " parent");
            rec.displayName = Text(node + " displayName");
            rec.loadOrder   = IntOr(node + " loadOrder", 0);
            Finish(rec, root, node, sink);
        }
    }

    private void ReadTags(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        string root = "CfgChefZTags";
        array<string> names = ChildNames(root);
        for (int i = 0; i < names.Count(); i++)
        {
            string id = names.Get(i);
            string node = root + " " + id;

            ChefZ_TagDef rec = new ChefZ_TagDef();
            rec.id          = id;
            rec.displayName = Text(node + " displayName");
            rec.loadOrder   = IntOr(node + " loadOrder", 0);
            Finish(rec, root, node, sink);
        }
    }

    private void ReadStates(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        string root = "CfgChefZStates";
        array<string> names = ChildNames(root);
        for (int i = 0; i < names.Count(); i++)
        {
            string id = names.Get(i);
            string node = root + " " + id;

            ChefZ_StateDef rec = new ChefZ_StateDef();
            rec.id                     = id;
            rec.displayName            = Text(node + " displayName");
            rec.projectsToVanillaStage = Text(node + " projectsToVanillaStage");
            rec.implies                = TextArrayOrNull(node + " implies");
            rec.spoilageMultiplier     = FloatOrUndefined(node + " spoilageMultiplier");
            rec.freshnessLifetimeSec   = FloatOrUndefined(node + " freshnessLifetimeSec");
            rec.loadOrder              = IntOr(node + " loadOrder", 0);

            // bool kennt keinen Sentinel: die Anwesenheit des Config-Eintrags
            // IST die Aussage "gesetzt" (02 E3, Mittel 3). Ohne MarkExplicit
            // wuerde ResolveDefaults den Wert gleich wieder ueberschreiben.
            ReadBool(rec, node, "edible");
            ReadBool(rec, node, "terminal");
            ReadBool(rec, node, "preserved");

            Finish(rec, root, node, sink);
        }
    }

    private void ReadQualityTiers(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        string root = "CfgChefZQualityTiers";
        array<string> names = ChildNames(root);
        for (int i = 0; i < names.Count(); i++)
        {
            string id = names.Get(i);
            string node = root + " " + id;

            // Feldliste woertlich aus 12 §3. Sie ist sync-relevant und darf
            // deshalb NUR von hier kommen (03 §4) - ein Overlay, das eine
            // Stufe hinzufuegt, bricht die Ordinalsymmetrie zwischen Client
            // und Server, und der Config Manager weist das als ERROR ab.
            ChefZ_QualityTierDef rec = new ChefZ_QualityTierDef();
            rec.id                 = id;
            rec.tierSet            = Text(node + " tierSet");
            rec.displayName        = Text(node + " displayName");
            rec.colorHint          = Text(node + " colorHint");
            rec.rank               = IntOrUndefined(node + " rank");
            rec.portionBonus       = IntOrUndefined(node + " portionBonus");
            rec.minScore           = FloatOrUndefined(node + " minScore");
            rec.yieldMultiplier    = FloatOrUndefined(node + " yieldMultiplier");
            rec.spoilageMultiplier = FloatOrUndefined(node + " spoilageMultiplier");
            rec.grantsEffects      = TextArrayOrNull(node + " grantsEffects");
            rec.grantsTags         = TextArrayOrNull(node + " grantsTags");
            rec.loadOrder          = IntOr(node + " loadOrder", 0);
            Finish(rec, root, node, sink);
        }
    }

    private void ReadToolGroups(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        string root = "CfgChefZTools";
        array<string> names = ChildNames(root);
        for (int i = 0; i < names.Count(); i++)
        {
            string id = names.Get(i);
            string node = root + " " + id;

            ChefZ_ToolGroupDef rec = new ChefZ_ToolGroupDef();
            rec.id             = id;
            rec.toolCategories = TextArrayOrNull(node + " toolCategories");
            rec.classes        = TextArrayOrNull(node + " classes");
            rec.loadOrder      = IntOr(node + " loadOrder", 0);
            if (g_Game.ConfigIsExisting(node + " allowSubclasses"))
            {
                rec.allowSubclasses = g_Game.ConfigGetInt(node + " allowSubclasses") != 0;
                rec.MarkExplicit("allowSubclasses");
            }
            Finish(rec, root, node, sink);
        }
    }

    /**
     * Verarbeitungsprozesse (11 §2).
     *
     * Feldliste woertlich aus 11 §2. Die Temperaturgrenzen werden ueber
     * FloatOrUndefined gelesen und behalten damit ihren Sentinel, wenn sie
     * fehlen - 11 §2 sagt ausdruecklich "Sentinel = egal", und eine 0 waere
     * dort keine Vorgabe, sondern eine Bedingung.
     */
    private void ReadProcesses(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        string root = "CfgChefZProcesses";
        array<string> names = ChildNames(root);
        for (int i = 0; i < names.Count(); i++)
        {
            string id = names.Get(i);
            string node = root + " " + id;

            ChefZ_ProcessDef rec = new ChefZ_ProcessDef();
            rec.id              = id;
            rec.exec            = Text(node + " exec");
            rec.displayName     = Text(node + " displayName");
            rec.toolGroups      = TextArrayOrNull(node + " toolGroups");
            rec.emitEvents      = TextArrayOrNull(node + " emitEvents");
            rec.baseDurationSec = FloatOrUndefined(node + " baseDurationSec");
            rec.minTemperature  = FloatOrUndefined(node + " minTemperature");
            rec.maxTemperature  = FloatOrUndefined(node + " maxTemperature");
            rec.animationLength = FloatOrUndefined(node + " animationLength");
            rec.specialty       = FloatOrUndefined(node + " specialty");
            rec.toolDamage      = IntOrUndefined(node + " toolDamage");
            rec.loadOrder       = IntOr(node + " loadOrder", 0);

            // bool kennt keinen Sentinel: die Anwesenheit des Config-Eintrags
            // IST die Aussage "gesetzt" (02 E3, Mittel 3).
            if (g_Game.ConfigIsExisting(node + " requiresHeat"))
            {
                rec.requiresHeat = g_Game.ConfigGetInt(node + " requiresHeat") != 0;
                rec.MarkExplicit("requiresHeat");
            }

            Finish(rec, root, node, sink);
        }
    }

    /**
     * Stationen (11 §2).
     *
     * 11 §2: "id == Klassenname der Station". Ein Knoten heisst also genau so
     * wie die CfgVehicles-Klasse, die er beschreibt - der ChefZ_ProcessCompiler
     * prueft das und meldet eine fehlende Klasse als WARN.
     *
     * Die Reihenfolge in processes[] ist BEDEUTUNGSTRAGEND: ihr Index ist der
     * synchronisierte Prozessordinal der Station (siehe
     * ChefZ_ProcessingStation_Base). Sie wird deshalb unveraendert
     * uebernommen und nirgends sortiert.
     */
    private void ReadStations(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        string root = "CfgChefZStations";
        array<string> names = ChildNames(root);
        for (int i = 0; i < names.Count(); i++)
        {
            string id = names.Get(i);
            string node = root + " " + id;

            ChefZ_StationDef rec = new ChefZ_StationDef();
            rec.id                = id;
            rec.stationCategories = TextArrayOrNull(node + " stationCategories");
            rec.processes         = TextArrayOrNull(node + " processes");
            rec.parallelSlots     = IntOrUndefined(node + " parallelSlots");
            rec.speedMultiplier   = FloatOrUndefined(node + " speedMultiplier");
            rec.loadOrder         = IntOr(node + " loadOrder", 0);

            if (g_Game.ConfigIsExisting(node + " needsFuel"))
            {
                rec.needsFuel = g_Game.ConfigGetInt(node + " needsFuel") != 0;
                rec.MarkExplicit("needsFuel");
            }

            Finish(rec, root, node, sink);
        }
    }

    private void ReadDevices(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        string root = "CfgChefZDevices";
        array<string> names = ChildNames(root);
        for (int i = 0; i < names.Count(); i++)
        {
            string id = names.Get(i);
            string node = root + " " + id;

            ChefZ_DeviceDef rec = new ChefZ_DeviceDef();
            rec.id               = id;
            rec.deviceCategories = TextArrayOrNull(node + " deviceCategories");
            rec.portionCapacity  = IntOrUndefined(node + " portionCapacity");
            rec.qualityModifier  = FloatOrUndefined(node + " qualityModifier");
            rec.loadOrder        = IntOr(node + " loadOrder", 0);
            Finish(rec, root, node, sink);
        }
    }

    /**
     * Behaelter (16 §3.1).
     *
     * Feldliste woertlich aus 16 §3.1. "id == Klassenname des LEEREN
     * Behaelters" (16 §3.2) - ein Knoten heisst also genau so wie die
     * CfgVehicles-Klasse, die er beschreibt.
     *
     * Die Klasse wird hier ausdruecklich NICHT gegen CfgVehicles geprueft: ein
     * Behaelter darf aus einem OPTIONALEN Modul stammen, das auf diesem Server
     * fehlt (dieselbe Regel wie bei den Werkzeugen, siehe
     * ChefZ_ToolRegistry). Geprueft wird die emptyClass, und zwar in der
     * ChefZ_ContainerRegistry - dort gehoert die Meldung hin (16 §7).
     *
     * searchScope ist ein BITFELD und wird ueber IntOrUndefined gelesen: eine
     * fehlende Angabe muss "nicht gesetzt" bleiben, damit ResolveDefaults die
     * Vorgabe HANDS|INVENTORY einsetzen kann. Eine 0 waere dort keine Vorgabe,
     * sondern die Ansage "nirgends suchen".
     */
    private void ReadContainers(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        string root = "CfgChefZContainers";
        array<string> names = ChildNames(root);
        for (int i = 0; i < names.Count(); i++)
        {
            string id = names.Get(i);
            string node = root + " " + id;

            ChefZ_ContainerDef rec = new ChefZ_ContainerDef();
            rec.id                  = id;
            rec.containerCategories = TextArrayOrNull(node + " containerCategories");
            rec.emptyClass          = Text(node + " emptyClass");
            rec.displayName         = Text(node + " displayName");
            rec.spoilageModifier    = FloatOrUndefined(node + " spoilageModifier");
            rec.searchScope         = IntOrUndefined(node + " searchScope");
            rec.loadOrder           = IntOr(node + " loadOrder", 0);

            // bool kennt keinen Sentinel: die Anwesenheit des Config-Eintrags
            // IST die Aussage "gesetzt" (02 E3, Mittel 3).
            if (g_Game.ConfigIsExisting(node + " reusable"))
            {
                rec.reusable = g_Game.ConfigGetInt(node + " reusable") != 0;
                rec.MarkExplicit("reusable");
            }
            if (g_Game.ConfigIsExisting(node + " consumedOnServe"))
            {
                rec.consumedOnServe = g_Game.ConfigGetInt(node + " consumedOnServe") != 0;
                rec.MarkExplicit("consumedOnServe");
            }

            Finish(rec, root, node, sink);
        }
    }

    /**
     * Zutatenbindungen (05 §2, erster Deklarationsweg).
     *
     * Zwei Vererbungen wirken hier NEBENEINANDER, und sie sind nicht dasselbe:
     *
     *   Config-Vererbung   "class ChefZ_HunterSausage : ChefZ_Sausage_Base"
     *                      INNERHALB von CfgChefZIngredients loest die Engine
     *                      selbst auf - ConfigIsExisting liefert fuer ein
     *                      geerbtes Feld bereits true. Hier ist nichts zu tun.
     *   ChefZ-Vererbung    entlang der CfgVehicles-Elternkette (05 E2). Sie
     *                      greift, wenn die ITEM-Klasse von einer anderen
     *                      Zutatenklasse erbt, der CfgChefZIngredients-Knoten
     *                      aber flach danebensteht. Sie loest der
     *                      ChefZ_IngredientManager beim Build auf - nicht hier.
     *
     * Deshalb bleiben die Sentinel dieser Quelle unangetastet: ein Feld, das
     * die Config nicht kennt, bleibt "nicht gesetzt" und wird spaeter geerbt
     * oder mit dem Default belegt.
     */
    private void ReadIngredients(ChefZ_RecordSink sink, ChefZ_LoadReport report)
    {
        string root = "CfgChefZIngredients";
        array<string> names = ChildNames(root);
        for (int i = 0; i < names.Count(); i++)
        {
            string id = names.Get(i);
            string node = root + " " + id;

            ChefZ_IngredientDef rec = new ChefZ_IngredientDef();
            rec.id                = id;
            rec.categories        = TextArrayOrNull(node + " categories");
            rec.tags              = TextArrayOrNull(node + " tags");
            rec.defaultState      = Text(node + " defaultState");
            rec.quantityUnit      = Text(node + " quantityUnit");
            rec.unitsPerWholeItem = FloatOrUndefined(node + " unitsPerWholeItem");
            rec.containerCategory = Text(node + " containerCategory");
            rec.returnContainer   = Text(node + " returnContainer");
            rec.loadOrder         = IntOr(node + " loadOrder", 0);

            // bool kennt keinen Sentinel: die Anwesenheit des Config-Eintrags
            // IST die Aussage "gesetzt" (02 E3, Mittel 3).
            if (g_Game.ConfigIsExisting(node + " decays"))
            {
                rec.decays = g_Game.ConfigGetInt(node + " decays") != 0;
                rec.MarkExplicit("decays");
            }

            Finish(rec, root, node, sink);
        }
    }

    //--------------------------------------------------------------------------
    // Helfer
    //--------------------------------------------------------------------------

    private void Finish(ChefZ_Record rec, string root, string node, ChefZ_RecordSink sink)
    {
        rec.SetOrigin(node, ChefZ_SourceRank.CONFIG_CPP);
        m_NodeCount++;
        sink.Submit(rec);
    }

    /**
     * Kindklassen eines Config-Knotens. Leeres Array, wenn es ihn nicht gibt.
     *
     * Muster aus ModLoader.c:18-25. Kein Ueberspringen der ersten Eintraege:
     * die CfgChefZ*-Knoten haben keine Basiseintraege (siehe
     * ChefZ_ManifestReader).
     */
    private array<string> ChildNames(string root)
    {
        array<string> names = new array<string>();
        if (!g_Game.ConfigIsExisting(root))
            return names;

        int count = g_Game.ConfigGetChildrenCount(root);
        for (int i = 0; i < count; i++)
        {
            string name;
            if (!g_Game.ConfigGetChildName(root, i, name))
                continue;
            if (name != "")
                names.Insert(name);
        }
        return names;
    }

    private string Text(string path)
    {
        if (!g_Game.ConfigIsExisting(path))
            return ChefZ_Undefined.TEXT;
        string value;
        if (!g_Game.ConfigGetText(path, value))
            return ChefZ_Undefined.TEXT;
        value.TrimInPlace();
        return value;
    }

    /**
     * bool aus der Game-Config, und zwar NUR wenn der Eintrag existiert.
     *
     * Der Umweg ueber den Feldnamen ist noetig, weil Enforce keine Referenz
     * auf ein Feld kennt. Der Aufruf bleibt dadurch an einer Stelle statt in
     * jeder Leseroutine dreimal - und die Regel "gesetzt heisst explizit"
     * steht dann auch nur einmal da.
     */
    private void ReadBool(notnull ChefZ_Record rec, string node, string field)
    {
        string path = node + " " + field;
        if (!g_Game.ConfigIsExisting(path))
            return;

        bool value = g_Game.ConfigGetInt(path) != 0;

        ChefZ_StateDef state = ChefZ_StateDef.Cast(rec);
        if (state)
        {
            if (field == "edible")          state.edible    = value;
            else if (field == "terminal")   state.terminal  = value;
            else if (field == "preserved")  state.preserved = value;
            else                            return;

            state.MarkExplicit(field);
        }
    }

    private int IntOr(string path, int fallback)
    {
        if (!g_Game.ConfigIsExisting(path))
            return fallback;
        return g_Game.ConfigGetInt(path);
    }

    private int IntOrUndefined(string path)
    {
        if (!g_Game.ConfigIsExisting(path))
            return ChefZ_Undefined.INT;
        return g_Game.ConfigGetInt(path);
    }

    private float FloatOrUndefined(string path)
    {
        if (!g_Game.ConfigIsExisting(path))
            return ChefZ_Undefined.FLOAT;
        return g_Game.ConfigGetFloat(path);
    }

    /**
     * Stringliste oder null.
     *
     * null ist hier bedeutungstragend: "nicht gesetzt" (02 E3). Ein leeres
     * Array hiesse "ausdruecklich leer" und wuerde beim Patchen eine bestehende
     * Liste loeschen.
     */
    private array<string> TextArrayOrNull(string path)
    {
        if (!g_Game.ConfigIsExisting(path))
            return null;

        TStringArray raw = new TStringArray();
        g_Game.ConfigGetTextArray(path, raw);

        array<string> values = new array<string>();
        for (int i = 0; i < raw.Count(); i++)
        {
            string v = raw.Get(i);
            v.TrimInPlace();
            if (v != "")
                values.Insert(v);
        }
        return values;
    }
}
