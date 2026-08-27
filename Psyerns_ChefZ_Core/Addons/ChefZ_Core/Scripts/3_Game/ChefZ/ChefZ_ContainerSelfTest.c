//==============================================================================
// ChefZ_ContainerSelfTest - Abnahmepruefung fuer S17, soweit sie ohne Welt geht
//
// Entwurf: 16 §3.2 (Schnittstelle), 16 §4 ("AUTO"), 16 §5 (Datenfluss),
// 16 §7 (Fehlerverhalten Zeile fuer Zeile), 16 E2/E4/E5/E7.
//
// ---------------------------------------------------------------------------
// Was hier geprueft wird - und was nicht, und warum
// ---------------------------------------------------------------------------
// Pruefbar OHNE laufende Welt ist alles, was die ChefZ_ContainerRegistry
// rechnet: die beiden Indizes, die Kategoriezugehoerigkeit, die
// AUTO-Aufloesung, die Kopplung reusable/consumedOnServe, das Suchbitfeld,
// die Klemmungen und die Auswahlregel. Genau dieser Teil scheitert LEISE:
//
//   - eine AUTO-Aufloesung, die INVALID liefert, sieht aus wie ein Rezept
//     ohne Rueckgabe - der Spieler merkt nur, dass kein Teller kommt, und
//     hielte das fuer die Absicht des Autors;
//   - eine ReturnsEmpty-Kopplung, die consumedOnServe vergisst, VERDOPPELT
//     Behaelter: der Spieler behaelt den Teller UND bekommt einen zweiten.
//     Nichts daran sieht kaputt aus, und niemand meldet es;
//   - eine ChooseContainer, die die Kategorie nicht prueft, verbraucht den
//     falschen Behaelter - und zwar den ersten der Liste, also fast immer den
//     in der Hand.
//
// NICHT pruefbar ist alles, was ein Item braucht:
//
//   - dass ein Behaelter im Inventar wirklich gefunden wird
//   - dass ConsumeForServing tatsaechlich loescht
//   - dass ReturnEmpty die Haende, dann das Inventar, dann den Boden nimmt
//   - dass OnConsume erst bei Quantity <= 0 zurueckgibt
//
// Der erste ist Engineverhalten (EnumerateInventory), die uebrigen drei
// brauchen einen Server mit Welt und bleiben dem Servertest vorbehalten. Ein
// nachgebautes Inventar wuerde den Nachbau pruefen, nicht die Engine.
//
// Der Test arbeitet auf EIGENEN Registryinstanzen, nie auf dem Singleton, und
// legt ausschliesslich Symbole mit dem Praefix "CHEFZ_CS_" an - Namen, die in
// echtem Content nicht vorkommen. Er beruehrt kein Item, keine Datei und keine
// Vanilla-Logik.
//
// Layer: 3_Game.
//==============================================================================

/**
 * Registry mit ersetzter Klassenpruefung.
 *
 * Die einzige Ueberschreibung ist ClassExists(). Alles andere - der Aufbau,
 * die Klemmungen, die Auswahlregel - ist der echte Code. Ein Test, der die
 * Regel nachbaute, statt sie auszufuehren, wuerde seinen eigenen Nachbau
 * pruefen.
 *
 * Bekannt sind genau die Klassen, die der Test ausdruecklich anmeldet. Damit
 * ist die Zeile aus 16 §7 - "emptyClass existiert nicht in CfgVehicles ->
 * Rueckgabe entfaellt" - ohne laufenden Server pruefbar.
 */
class ChefZ_ContainerRegistryProbe extends ChefZ_ContainerRegistry
{
    private ref array<string> m_Known;

    void ChefZ_ContainerRegistryProbe()
    {
        m_Known = new array<string>();
        SetQuietForTest(true);
    }

    void DeclareClass(string cls)
    {
        if (m_Known.Find(cls) < 0)
            m_Known.Insert(cls);
    }

    protected override bool ClassExists(string cls)
    {
        return m_Known.Find(cls) >= 0;
    }
}

//==============================================================================

class ChefZ_ContainerSelfTest
{
    //--- Testvokabular. Praefix CHEFZ_CS_, damit es mit keinem Content
    //--- kollidieren kann; die Namen sind Platzhalter und bedeuten nichts.
    static const string KAT_A   = "CHEFZ_CS_KAT_A";
    static const string KAT_B   = "CHEFZ_CS_KAT_B";
    static const string KAT_X   = "CHEFZ_CS_KAT_UNBEKANNT";

    static const string C_ALPHA = "CHEFZ_CS_ALPHA";
    static const string C_BETA  = "CHEFZ_CS_BETA";
    static const string C_DOSE  = "CHEFZ_CS_DOSE";       // reusable = 0
    static const string C_LEIHE = "CHEFZ_CS_LEIHE";      // consumedOnServe = 0
    static const string C_GLAS  = "CHEFZ_CS_GLAS";       // eigene Leerklasse
    static const string C_LEER  = "CHEFZ_CS_LEERGLAS";
    static const string C_GEIST = "CHEFZ_CS_GEIST";      // emptyClass gibt es nicht
    static const string C_WEG   = "CHEFZ_CS_NICHTDA";

    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("ContainerDef",   ChefZ_ContainerDef.SelfCheck());
        Check("Index",          IndexCheck());
        Check("Rueckgabe",      ReturnCheck());
        Check("Auto",           AutoCheck());
        Check("Suchbereich",    ScopeCheck());
        Check("Auswahl",        ChooseCheck());
        Check("Klemmung",       ClampCheck());
        Check("LeereRegistry",  EmptyCheck());
        Check("VorBuild",       NotReadyCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.CONTAIN, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.CONTAIN, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.CONTAIN,
            "Selbsttest " + name + " FEHLGESCHLAGEN. Das Behaeltersystem verhaelt sich "
            + "nicht wie entworfen - Verbrauch und Rueckgabe von Behaeltern sind ab hier "
            + "unzuverlaessig. Vanilla-Kochen ist davon unberuehrt.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S17: " + s_Passed.ToString() + "/" + total.ToString()
                 + " Gruppen ok";
        if (s_Failed > 0 && s_FailedNames)
        {
            s = s + "  gescheitert:";
            for (int i = 0; i < s_FailedNames.Count(); i++)
                s = s + " " + s_FailedNames.Get(i);
        }
        return s;
    }

    //==========================================================================
    // Hilfen
    //==========================================================================

    private static ChefZ_Sym Sym(string name)
    {
        return ChefZ_SymbolTable.Intern(name);
    }

    private static ChefZ_Registry<ChefZ_ContainerDef> NewRegistry()
    {
        ChefZ_Registry<ChefZ_ContainerDef> reg = new ChefZ_Registry<ChefZ_ContainerDef>();
        reg.Init(ChefZ_RecordKind.CONTAINER);
        return reg;
    }

    /**
     * Ein Behaelterrecord, wie ihn der Record-Sink liefern wuerde:
     * normalisiert, validiert, entsentinelt, kompiliert. Genau diese
     * Reihenfolge laeuft im Config Manager (02 §6).
     */
    private static ChefZ_ContainerDef Add(notnull ChefZ_Registry<ChefZ_ContainerDef> reg,
                                          string id, string category, string emptyClass,
                                          int scope)
    {
        ChefZ_ContainerDef def = new ChefZ_ContainerDef();
        def.id                  = id;
        def.containerCategories = new array<string>();
        def.containerCategories.Insert(category);
        def.emptyClass          = emptyClass;
        def.searchScope         = scope;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);

        def.Normalize();
        if (!def.Validate(null))
            return null;
        def.ResolveDefaults();
        def.Compile(null);

        if (!reg.Add(def))
            return null;
        return def;
    }

    //! Die Standardaufstellung: zwei Behaelter in KAT_A, einer in KAT_B.
    private static ChefZ_ContainerRegistryProbe BuildStandard()
    {
        ChefZ_Registry<ChefZ_ContainerDef> reg = NewRegistry();
        Add(reg, C_ALPHA, KAT_A, "",     ChefZ_ContainerScope.DEFAULT);
        Add(reg, C_BETA,  KAT_A, "",     ChefZ_ContainerScope.HANDS);
        Add(reg, C_GLAS,  KAT_B, C_LEER, ChefZ_ContainerScope.ALL);
        reg.Freeze();

        ChefZ_ContainerRegistryProbe probe = new ChefZ_ContainerRegistryProbe();
        probe.DeclareClass(C_ALPHA);
        probe.DeclareClass(C_BETA);
        probe.DeclareClass(C_GLAS);
        probe.DeclareClass(C_LEER);
        probe.Build(reg, null);
        return probe;
    }

    //==========================================================================
    // Gruppen
    //==========================================================================

    //! Beide Indizes, beide Richtungen (siehe Kopf der Registry).
    private static bool IndexCheck()
    {
        ChefZ_ContainerRegistryProbe probe = BuildStandard();

        if (!probe.IsReady())                                       return false;
        if (!probe.HasAnyContainer())                               return false;
        if (probe.GetClassCount() != 3)                             return false;
        if (probe.GetCategoryCount() != 2)                          return false;

        if (!probe.CategoryExists(Sym(KAT_A)))                      return false;
        if (!probe.CategoryExists(Sym(KAT_B)))                      return false;
        if (probe.CategoryExists(Sym(KAT_X)))                       return false;

        if (!probe.IsContainerClass(Sym(C_ALPHA)))                  return false;
        if (probe.IsContainerClass(Sym(C_WEG)))                     return false;

        if (!probe.IsContainerOfCategory(Sym(C_ALPHA), Sym(KAT_A))) return false;
        if (probe.IsContainerOfCategory(Sym(C_ALPHA), Sym(KAT_B)))  return false;
        if (probe.IsContainerOfCategory(Sym(C_WEG), Sym(KAT_A)))    return false;

        array<ChefZ_Sym> classes = new array<ChefZ_Sym>();
        probe.GetClassesForCategory(Sym(KAT_A), classes);
        if (classes.Count() != 2)                                   return false;

        // Unbekannte Kategorie: LEER, nicht null. Ein Aufrufer, der die Liste
        // ohne Pruefung durchlaeuft, darf daran nicht sterben.
        probe.GetClassesForCategory(Sym(KAT_X), classes);
        if (!classes)                                               return false;
        if (classes.Count() != 0)                                   return false;

        ChefZ_ContainerDef def;
        if (!probe.GetDef(Sym(C_ALPHA), def))                       return false;
        if (!def || def.id != C_ALPHA)                              return false;
        if (probe.GetDef(Sym(C_WEG), def))                          return false;

        return true;
    }

    /**
     * Die Kopplung aus 16 §7: reusable = 0 UND consumedOnServe = 0 geben beide
     * nichts zurueck - und zwar aus verschiedenen Gruenden.
     *
     * Diese Gruppe ist die wichtigste des ganzen Tests: faellt sie um,
     * verdoppeln sich Behaelter, ohne dass irgendwo eine Zeile im Log steht.
     */
    private static bool ReturnCheck()
    {
        ChefZ_Registry<ChefZ_ContainerDef> reg = NewRegistry();

        Add(reg, C_ALPHA, KAT_A, "", ChefZ_ContainerScope.DEFAULT);

        // 16 §7: "reusable = 0 -> es wird nichts zurueckgegeben. Kein Fehler -
        // das ist der Konservenfall."
        ChefZ_ContainerDef dose = Add(reg, C_DOSE, KAT_A, "", ChefZ_ContainerScope.DEFAULT);
        if (!dose)                                                  return false;
        dose.reusable = false;

        // consumedOnServe = 0: der Behaelter bleibt beim Spieler. Etwas
        // zurueckzugeben hiesse, ihn zu verdoppeln.
        ChefZ_ContainerDef leihe = Add(reg, C_LEIHE, KAT_A, "", ChefZ_ContainerScope.DEFAULT);
        if (!leihe)                                                 return false;
        leihe.consumedOnServe = false;

        // 16 §7: emptyClass existiert nicht -> Rueckgabe entfaellt.
        Add(reg, C_GEIST, KAT_A, C_WEG, ChefZ_ContainerScope.DEFAULT);
        reg.Freeze();

        ChefZ_ContainerRegistryProbe probe = new ChefZ_ContainerRegistryProbe();
        probe.DeclareClass(C_ALPHA);
        probe.DeclareClass(C_DOSE);
        probe.DeclareClass(C_LEIHE);
        probe.DeclareClass(C_GEIST);
        probe.Build(reg, null);

        if (!probe.ReturnsEmpty(Sym(C_ALPHA)))                      return false;
        if (probe.ReturnsEmpty(Sym(C_DOSE)))                        return false;
        if (probe.ReturnsEmpty(Sym(C_LEIHE)))                       return false;
        if (probe.ReturnsEmpty(Sym(C_WEG)))                         return false;   // unbekannt

        // Der Normalfall: emptyClass ist per Vorgabe die ID selbst.
        if (probe.ResolveEmptyClass(Sym(C_ALPHA)) != Sym(C_ALPHA))  return false;

        if (ChefZ_SymbolTable.IsValid(probe.ResolveEmptyClass(Sym(C_DOSE))))    return false;
        if (ChefZ_SymbolTable.IsValid(probe.ResolveEmptyClass(Sym(C_LEIHE))))   return false;
        if (ChefZ_SymbolTable.IsValid(probe.ResolveEmptyClass(Sym(C_GEIST))))   return false;
        if (ChefZ_SymbolTable.IsValid(probe.ResolveEmptyClass(Sym(C_WEG))))     return false;

        return true;
    }

    //! 16 §4: "AUTO" gibt den Leerbehaelter GENAU DES benutzten Behaelters
    //! zurueck - eine feste Klasse dagegen genau sich selbst.
    private static bool AutoCheck()
    {
        ChefZ_ContainerRegistryProbe probe = BuildStandard();

        // Leere Angabe: nichts zurueck.
        if (ChefZ_SymbolTable.IsValid(probe.ResolveReturnClass("", Sym(C_ALPHA))))
            return false;

        // AUTO mit benutztem Behaelter: dessen Leerklasse. C_GLAS fuehrt eine
        // eigene - genau der Fall aus 16 §4, "wer eine Emailleschuessel
        // hineingab, bekommt eine Emailleschuessel zurueck".
        if (probe.ResolveReturnClass(ChefZ_ContainerDef.AUTO, Sym(C_GLAS)) != Sym(C_LEER))
            return false;
        if (probe.ResolveReturnClass(ChefZ_ContainerDef.AUTO, Sym(C_ALPHA)) != Sym(C_ALPHA))
            return false;

        // AUTO ohne benutzten Behaelter: nichts. Das ist der Kochfall - beim
        // Kochen wird nie ein Behaelter verbraucht (16 §2).
        if (ChefZ_SymbolTable.IsValid(
                probe.ResolveReturnClass(ChefZ_ContainerDef.AUTO, ChefZ_SymbolTable.INVALID)))
            return false;

        // Feste Klasse: unveraendert, und ausdruecklich OHNE Existenzpruefung -
        // sie darf aus einem optionalen Modul stammen.
        if (probe.ResolveReturnClass(C_WEG, ChefZ_SymbolTable.INVALID) != Sym(C_WEG))
            return false;

        return true;
    }

    //! 16 E5: das Bitfeld an der KLASSE, die Frage an der KATEGORIE.
    private static bool ScopeCheck()
    {
        ChefZ_ContainerRegistryProbe probe = BuildStandard();

        // KAT_A: ALPHA hat HANDS|INVENTORY, BETA nur HANDS. Das ODER ergibt
        // HANDS|INVENTORY - die Inventarstufe wird betreten, weil ALPHA sie
        // will.
        int catA = probe.GetSearchScope(Sym(KAT_A));
        if (!ChefZ_ContainerScope.Has(catA, ChefZ_ContainerScope.HANDS))        return false;
        if (!ChefZ_ContainerScope.Has(catA, ChefZ_ContainerScope.INVENTORY))    return false;
        if (ChefZ_ContainerScope.Has(catA, ChefZ_ContainerScope.NEARBY_CARGO))  return false;

        // Die Gegenprobe je Klasse: BETA zaehlt trotzdem NUR in der Hand.
        // Ohne diese zweite Frage waere das Bitfeld an der Klasse wirkungslos,
        // sobald ein einziges Mitglied grosszuegiger ist.
        int beta = probe.GetSearchScopeForClass(Sym(C_BETA));
        if (!ChefZ_ContainerScope.Has(beta, ChefZ_ContainerScope.HANDS))        return false;
        if (ChefZ_ContainerScope.Has(beta, ChefZ_ContainerScope.INVENTORY))     return false;

        if (probe.GetSearchScope(Sym(KAT_X)) != ChefZ_ContainerScope.NONE)      return false;
        if (probe.GetSearchScopeForClass(Sym(C_WEG)) != ChefZ_ContainerScope.NONE) return false;

        return true;
    }

    /**
     * 16 §5: "deterministisch". Die Liste ist bereits geordnet; gewaehlt wird
     * der erste Eintrag, der der Kategorie angehoert.
     *
     * Der zweite Teil ist der wichtige: ein Behaelter der FALSCHEN Kategorie
     * darf nicht gewaehlt werden, auch wenn er vorne steht.
     */
    private static bool ChooseCheck()
    {
        ChefZ_ContainerRegistryProbe probe = BuildStandard();
        ChefZ_Sym chosen;

        array<ChefZ_Sym> list = new array<ChefZ_Sym>();
        list.Insert(Sym(C_BETA));
        list.Insert(Sym(C_ALPHA));

        if (!probe.ChooseContainer(Sym(KAT_A), list, chosen))       return false;
        if (chosen != Sym(C_BETA))                                  return false;

        // Dieselbe Liste, andere Kategorie: der einzige Kandidat aus KAT_B
        // steht gar nicht drin.
        if (probe.ChooseContainer(Sym(KAT_B), list, chosen))        return false;
        if (ChefZ_SymbolTable.IsValid(chosen))                      return false;

        // Fremdes vorneweg wird uebersprungen, nicht genommen.
        array<ChefZ_Sym> mixed = new array<ChefZ_Sym>();
        mixed.Insert(Sym(C_GLAS));                                  // KAT_B
        mixed.Insert(Sym(C_ALPHA));                                 // KAT_A
        if (!probe.ChooseContainer(Sym(KAT_A), mixed, chosen))      return false;
        if (chosen != Sym(C_ALPHA))                                 return false;

        // Unbekannte Kategorie: nie eine Wahl.
        if (probe.ChooseContainer(Sym(KAT_X), mixed, chosen))       return false;

        // Leere Liste: kein Treffer, kein Absturz.
        array<ChefZ_Sym> nothing = new array<ChefZ_Sym>();
        if (probe.ChooseContainer(Sym(KAT_A), nothing, chosen))     return false;

        return true;
    }

    //! 16 §7: spoilageModifier <= 0 wird geklemmt, unbekannte Scope-Bits
    //! werden ausmaskiert.
    private static bool ClampCheck()
    {
        ChefZ_Registry<ChefZ_ContainerDef> reg = NewRegistry();

        ChefZ_ContainerDef bad = Add(reg, C_ALPHA, KAT_A, "", 8 | ChefZ_ContainerScope.HANDS);
        if (!bad)                                                   return false;
        bad.spoilageModifier = 0.0;

        ChefZ_ContainerDef glas = Add(reg, C_GLAS, KAT_B, C_LEER, ChefZ_ContainerScope.DEFAULT);
        if (!glas)                                                  return false;
        glas.spoilageModifier = 0.10;

        reg.Freeze();

        ChefZ_ContainerRegistryProbe probe = new ChefZ_ContainerRegistryProbe();
        probe.DeclareClass(C_ALPHA);
        probe.DeclareClass(C_GLAS);
        probe.DeclareClass(C_LEER);
        probe.Build(reg, null);

        if (probe.GetSpoilageModifier(Sym(C_ALPHA)) != ChefZ_ContainerDef.MIN_SPOILAGE)
            return false;

        // 16 E7: "Eingemachtes im Glas haelt laenger" ist eine Zahl, kein
        // Zustand.
        if (probe.GetSpoilageModifier(Sym(C_GLAS)) != 0.10)         return false;

        // Unbekannter Behaelter: neutral. "Nichts bekannt" darf die
        // Haltbarkeit niemals veraendern.
        if (probe.GetSpoilageModifier(Sym(C_WEG)) != ChefZ_ContainerDef.DEFAULT_SPOILAGE)
            return false;

        // Das unbekannte Bit ist weg, das bekannte steht.
        int scope = probe.GetSearchScopeForClass(Sym(C_ALPHA));
        if (ChefZ_ContainerScope.UnknownBits(scope) != 0)           return false;
        if (!ChefZ_ContainerScope.Has(scope, ChefZ_ContainerScope.HANDS)) return false;

        return true;
    }

    /**
     * 16 §7, erste Zeile: ohne CfgChefZContainers ist die Registry "bereit und
     * leer" - und antwortet ruhig, statt zu klagen. In einem Core ohne Content
     * ist das der NORMALZUSTAND.
     *
     * HasAnyContainer() muss dabei false sagen: davon haengt ab, ob der
     * ChefZ_PortionManager die Behaelterbedingung fallen laesst (15 §7) oder
     * durchsetzt (16 §7).
     */
    private static bool EmptyCheck()
    {
        ChefZ_ContainerRegistryProbe probe = new ChefZ_ContainerRegistryProbe();
        probe.Build(null, null);

        if (!probe.IsReady())                                       return false;
        if (probe.HasAnyContainer())                                return false;
        if (probe.GetClassCount() != 0)                             return false;
        if (probe.GetCategoryCount() != 0)                          return false;

        if (probe.CategoryExists(Sym(KAT_A)))                       return false;
        if (probe.IsContainerClass(Sym(C_ALPHA)))                   return false;
        if (probe.ReturnsEmpty(Sym(C_ALPHA)))                       return false;
        if (ChefZ_SymbolTable.IsValid(probe.ResolveEmptyClass(Sym(C_ALPHA)))) return false;
        if (probe.GetSpoilageModifier(Sym(C_ALPHA)) != ChefZ_ContainerDef.DEFAULT_SPOILAGE)
            return false;

        ChefZ_Sym chosen;
        array<ChefZ_Sym> list = new array<ChefZ_Sym>();
        list.Insert(Sym(C_ALPHA));
        if (probe.ChooseContainer(Sym(KAT_A), list, chosen))        return false;

        // Und das Audit laeuft auch dann durch, wenn es nichts zu pruefen gibt.
        probe.AuditPortionSpecs(null, null);

        return true;
    }

    //! Vor jedem Build: dieselben ruhigen Antworten. Eine Registry, die vor
    //! dem Boot gefragt wird, darf nichts behaupten - und nicht abstuerzen.
    private static bool NotReadyCheck()
    {
        ChefZ_ContainerRegistryProbe probe = new ChefZ_ContainerRegistryProbe();

        if (probe.IsReady())                                        return false;
        if (probe.HasAnyContainer())                                return false;
        if (probe.CategoryExists(Sym(KAT_A)))                       return false;
        if (probe.IsContainerClass(Sym(C_ALPHA)))                   return false;
        if (probe.IsContainerOfCategory(Sym(C_ALPHA), Sym(KAT_A)))  return false;
        if (ChefZ_SymbolTable.IsValid(probe.ResolveEmptyClass(Sym(C_ALPHA)))) return false;
        if (probe.GetSearchScope(Sym(KAT_A)) != ChefZ_ContainerScope.NONE) return false;

        array<ChefZ_Sym> classes = new array<ChefZ_Sym>();
        probe.GetClassesForCategory(Sym(KAT_A), classes);
        if (!classes || classes.Count() != 0)                       return false;

        return true;
    }
}
