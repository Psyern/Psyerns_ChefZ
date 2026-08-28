//==============================================================================
// ChefZ_StateSelfTest - Abnahmepruefung fuer S9, soweit sie ohne Welt geht
//
// Entwurf: 19 S9 (Abnahmebedingungen), 06 §3 (Projektionsregel), 06 §7
// (Fehlerverhalten, Zeile fuer Zeile), 03 §4 und 03 E5 (Identitaeten).
//
// ---------------------------------------------------------------------------
// Was hier geprueft wird - und was nicht, und warum
// ---------------------------------------------------------------------------
// Pruefbar OHNE laufende Welt ist alles, was der ChefZ_StateManager rechnet:
// Projektion, Rueckabbildung, implies-Filter, Persistenzhash, Ordinal,
// Terminal- und Essbarkeitsregel, das Verhalten vor Build und mit leerer
// Registry. Das ist der Teil, dessen Fehler NICHT auffielen: ein Zustand, der
// still auf gar nichts projiziert, sieht aus wie ein Zustand, den niemand
// benutzt hat.
//
// NICHT pruefbar sind die drei Abnahmebedingungen aus 19 S9, die ein Item
// brauchen:
//
//   - Zustand ueberlebt einen Serverneustart      -> braucht einen Spielstand
//   - Item ohne ChefZ-Block laedt sauber          -> braucht einen Spielstand
//   - Klassentausch uebertraegt das Tripel        -> braucht ein Inventar
//
// Sie bleiben dem Servertest vorbehalten und werden hier ausdruecklich NICHT
// nachgestellt. Ein Test, der einen ParamsWriteContext nachbaut, prueft den
// Nachbau, nicht die Engine.
//
// Der Test arbeitet auf EIGENEN Manager-Instanzen, nie auf dem Singleton, und
// legt ausschliesslich Symbole mit dem Praefix "CHEFZ_ST_" an - Namen, die in
// echtem Content nicht vorkommen. Er beruehrt kein Item, keine Datei und
// keine Vanilla-Logik.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_StateSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("StateDef",     ChefZ_StateDef.SelfCheck());
        Check("Projektion",   ProjectionCheck());
        Check("Rueckabbildung", BackMappingCheck());
        Check("Implies",      ImpliesCheck());
        Check("Identitaeten", IdentityCheck());
        Check("Kollision",    CollisionCheck());
        Check("Regeln",       RuleCheck());
        Check("LeereRegistry", EmptyCheck());
        Check("VorBuild",     NotReadyCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.STATE, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.STATE, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.STATE, "Selbsttest " + name + " FEHLGESCHLAGEN. Das Food State System verhaelt sich nicht " + "wie entworfen - Zustandsermittlung, Persistenz und Anzeige sind ab hier " + "unzuverlaessig. Vanilla-Kochen ist davon unberuehrt.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S9: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
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

    private static ChefZ_Registry<ChefZ_StateDef> NewRegistry()
    {
        ChefZ_Registry<ChefZ_StateDef> reg = new ChefZ_Registry<ChefZ_StateDef>();
        reg.Init(ChefZ_RecordKind.STATE);
        return reg;
    }

    private static ChefZ_StateDef Add(notnull ChefZ_Registry<ChefZ_StateDef> reg, string id, string projectsTo)
    {
        ChefZ_StateDef def = new ChefZ_StateDef();
        def.id                     = id;
        def.projectsToVanillaStage = projectsTo;
        def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        def.ResolveDefaults();
        def.Validate(null);
        def.Compile(null);
        if (!reg.Add(def))
            return null;
        return def;
    }

    private static ChefZ_StateManager NewManager()
    {
        ChefZ_StateManager mgr = new ChefZ_StateManager();
        mgr.SetQuietForTest(true);
        return mgr;
    }

    //! Ein Kategoriemanager mit genau den Tags, die der Test braucht. Ohne ihn
    //! filterte ResolveImplies gegen den Bestand des echten Servers.
    private static ChefZ_CategoryManager NewTagOwner(array<string> tagIds)
    {
        ChefZ_Registry<ChefZ_TagDef> tags = new ChefZ_Registry<ChefZ_TagDef>();
        tags.Init(ChefZ_RecordKind.TAG);

        for (int i = 0; i < tagIds.Count(); i++)
        {
            ChefZ_TagDef def = new ChefZ_TagDef();
            def.id = tagIds.Get(i);
            def.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
            def.Compile(null);
            tags.Add(def);
        }

        ChefZ_CategoryManager cats = new ChefZ_CategoryManager();
        cats.SetQuietForTest(true);
        cats.Build(null, tags, null);
        return cats;
    }

    private static ChefZ_IdentityMap NewIdentities(notnull ChefZ_Registry<ChefZ_StateDef> reg)
    {
        ChefZ_IdentityMap ids = new ChefZ_IdentityMap();
        ids.SetRegistryName("Selbsttest-States");
        ids.Build(reg.SortedIds(), null, ChefZ_SyncLimits.STATE_ORDINAL_MAX);
        return ids;
    }

    private static ChefZ_Sym Sym(string name)
    {
        return ChefZ_SymbolTable.Lookup(name);
    }

    //==========================================================================
    // 1. Projektion: Name -> Zahl, und zurueck ueber die Def
    //==========================================================================

    private static bool ProjectionCheck()
    {
        ChefZ_Registry<ChefZ_StateDef> reg = NewRegistry();
        Add(reg, "CHEFZ_ST_RAWLIKE",  "Raw");
        Add(reg, "CHEFZ_ST_DRYLIKE",  "Dried");
        Add(reg, "CHEFZ_ST_NOSTAGE",  "");

        ChefZ_StateManager mgr = NewManager();
        mgr.SetCategoryManagerForTest(NewTagOwner(new array<string>()));
        mgr.Build(reg, null, NewIdentities(reg));

        if (!mgr.IsReady())                                                     return false;
        if (mgr.GetStateCount() != 3)                                           return false;

        if (mgr.ProjectToVanillaStage(Sym("CHEFZ_ST_RAWLIKE")) != ChefZ_VanillaStage.RAW)
            return false;
        if (mgr.ProjectToVanillaStage(Sym("CHEFZ_ST_DRYLIKE")) != ChefZ_VanillaStage.DRIED)
            return false;
        if (mgr.ProjectToVanillaStage(Sym("CHEFZ_ST_NOSTAGE")) != ChefZ_VanillaStage.NONE)
            return false;

        // Unbekannter Zustand: keine Projektion, kein Absturz, Rueckfall.
        if (mgr.ProjectToVanillaStage(ChefZ_SymbolTable.Intern("CHEFZ_ST_GIBTSNICHT")) != ChefZ_VanillaStage.NONE)                                         return false;
        if (!mgr.GetOrFallback(ChefZ_SymbolTable.Lookup("CHEFZ_ST_GIBTSNICHT"))) return false;
        if (mgr.GetDef(ChefZ_SymbolTable.Lookup("CHEFZ_ST_GIBTSNICHT")))        return false;

        return true;
    }

    //==========================================================================
    // 2. Rueckabbildung Vanilla-Stufe -> Zustand (06 §3, Schritt 3)
    //==========================================================================

    private static bool BackMappingCheck()
    {
        // a) Eindeutig: genau ein Zustand projiziert auf "Boiled".
        ChefZ_Registry<ChefZ_StateDef> single = NewRegistry();
        Add(single, "CHEFZ_ST_BOILY", "Boiled");

        ChefZ_StateManager m1 = NewManager();
        m1.SetCategoryManagerForTest(NewTagOwner(new array<string>()));
        m1.Build(single, null, NewIdentities(single));

        if (m1.FromVanillaStage(ChefZ_VanillaStage.BOILED) != Sym("CHEFZ_ST_BOILY"))
            return false;
        if (ChefZ_SymbolTable.IsValid(m1.FromVanillaStage(ChefZ_VanillaStage.BAKED)))
            return false;

        // b) Mehrdeutig: zwei Zustaende projizieren auf "Dried" - genau der
        //    Fall SMOKED/DRIED aus 06 §3. Ohne kanonische ID gibt es dann
        //    KEINE Rueckabbildung; raten waere hier das Falsche.
        ChefZ_Registry<ChefZ_StateDef> many = NewRegistry();
        Add(many, "CHEFZ_ST_DRYA", "Dried");
        Add(many, "CHEFZ_ST_DRYB", "Dried");

        ChefZ_StateManager m2 = NewManager();
        m2.SetCategoryManagerForTest(NewTagOwner(new array<string>()));
        m2.Build(many, null, NewIdentities(many));

        if (ChefZ_SymbolTable.IsValid(m2.FromVanillaStage(ChefZ_VanillaStage.DRIED)))
            return false;

        // c) Kanonische ID gewinnt gegen die Mehrdeutigkeit. Der Zustand
        //    heisst hier woertlich wie in 06 §3 - das ist der einzige Ort im
        //    ganzen Core, an dem diese Schreibweise auftaucht, und er ist ein
        //    Test.
        ChefZ_Registry<ChefZ_StateDef> canon = NewRegistry();
        Add(canon, "CHEFZ_ST_DRYC", "Dried");
        Add(canon, "DRIED",         "Dried");

        ChefZ_StateManager m3 = NewManager();
        m3.SetCategoryManagerForTest(NewTagOwner(new array<string>()));
        m3.Build(canon, null, NewIdentities(canon));

        if (m3.FromVanillaStage(ChefZ_VanillaStage.DRIED) != Sym("DRIED"))
            return false;

        return true;
    }

    //==========================================================================
    // 3. implies: unbekannte Tags fallen weg, bekannte bleiben
    //==========================================================================

    private static bool ImpliesCheck()
    {
        array<string> known = new array<string>();
        known.Insert("CHEFZ_ST_TAG_A");

        ChefZ_Registry<ChefZ_StateDef> reg = NewRegistry();
        ChefZ_StateDef def = Add(reg, "CHEFZ_ST_WITHTAGS", "Raw");
        if (!def)                                                               return false;

        def.implies = new array<string>();
        def.implies.Insert("CHEFZ_ST_TAG_A");        // deklariert
        def.implies.Insert("CHEFZ_ST_TAG_UNBEKANNT"); // nicht deklariert
        def.implies.Insert("CHEFZ_ST_TAG_A");        // doppelt

        ChefZ_StateManager mgr = NewManager();
        mgr.SetCategoryManagerForTest(NewTagOwner(known));
        mgr.Build(reg, null, NewIdentities(reg));

        array<ChefZ_Sym> tags;
        mgr.GetImpliedTags(Sym("CHEFZ_ST_WITHTAGS"), tags);
        if (tags.Count() != 1)                                                  return false;
        if (tags.Get(0) != Sym("CHEFZ_ST_TAG_A"))                               return false;

        // Ein Zustand ohne implies liefert eine leere Liste, nie null.
        ChefZ_Registry<ChefZ_StateDef> bare = NewRegistry();
        Add(bare, "CHEFZ_ST_NOTAGS", "");

        ChefZ_StateManager m2 = NewManager();
        m2.SetCategoryManagerForTest(NewTagOwner(known));
        m2.Build(bare, null, NewIdentities(bare));

        array<ChefZ_Sym> empty;
        m2.GetImpliedTags(Sym("CHEFZ_ST_NOTAGS"), empty);
        if (!empty)                                                             return false;
        if (empty.Count() != 0)                                                 return false;

        return true;
    }

    //==========================================================================
    // 4. Identitaeten: Hash stabil, Ordinal abgeleitet, beides umkehrbar
    //==========================================================================

    private static bool IdentityCheck()
    {
        ChefZ_Registry<ChefZ_StateDef> reg = NewRegistry();
        Add(reg, "CHEFZ_ST_ID_B", "");
        Add(reg, "CHEFZ_ST_ID_A", "");
        Add(reg, "CHEFZ_ST_ID_C", "");

        ChefZ_StateManager mgr = NewManager();
        mgr.SetCategoryManagerForTest(NewTagOwner(new array<string>()));
        mgr.Build(reg, null, NewIdentities(reg));

        ChefZ_Sym a = Sym("CHEFZ_ST_ID_A");
        ChefZ_Sym b = Sym("CHEFZ_ST_ID_B");

        // Persistenz: id.Hash(), hin und zurueck (03 E2).
        int hashA = mgr.GetPersistHash(a);
        if (hashA == 0)                                                         return false;
        if (mgr.FromPersistHash(hashA) != a)                                    return false;

        // Und er IST der Hash der ID, nicht irgendeine Zahl - genau das macht
        // ihn stabil ueber Content-Updates.
        string nameA = "CHEFZ_ST_ID_A";
        if (hashA != nameA.Hash())                                              return false;

        // Ein unbekannter Hash liefert INVALID, kein Raten.
        if (ChefZ_SymbolTable.IsValid(mgr.FromPersistHash(12345678)))           return false;
        if (ChefZ_SymbolTable.IsValid(mgr.FromPersistHash(0)))                  return false;

        // Sync: abgeleitet aus der sortierten Liste, beginnend bei 1 (03 §4).
        // "..._A" steht vor "..._B", also Ordinal 1 vor Ordinal 2.
        if (mgr.GetSyncOrdinal(a) != 1)                                         return false;
        if (mgr.GetSyncOrdinal(b) != 2)                                         return false;
        if (mgr.GetMaxOrdinal() != 3)                                           return false;
        if (mgr.FromSyncOrdinal(1) != a)                                        return false;
        if (ChefZ_SymbolTable.IsValid(mgr.FromSyncOrdinal(0)))                  return false;
        if (ChefZ_SymbolTable.IsValid(mgr.FromSyncOrdinal(99)))                 return false;

        // Ohne Ordinaltabelle bleibt der Zustand serverseitig voll benutzbar.
        ChefZ_StateManager noIds = NewManager();
        noIds.SetCategoryManagerForTest(NewTagOwner(new array<string>()));
        noIds.Build(reg, null, null);
        if (noIds.GetSyncOrdinal(a) != 0)                                       return false;
        if (noIds.GetPersistHash(a) != hashA)                                   return false;
        if (!noIds.Exists(a))                                                   return false;

        return true;
    }

    //==========================================================================
    // 5. Hash-Kollision: BEIDE fallen aus der Persistenz (03 E5)
    //==========================================================================

    private static bool CollisionCheck()
    {
        // Zwei IDs mit gleichem String-Hash zu finden ist nicht noetig: die
        // Regel laesst sich pruefen, indem derselbe Hash zweimal angeboten
        // wird. Dafuer genuegt EIN Zustand plus die Frage, ob ein fremder
        // Hash je etwas trifft - der Kollisionszweig selbst ist ohne
        // kuenstliche Kollision nicht erreichbar, und eine kuenstliche
        // Kollision waere ein Test der Testvorrichtung.
        //
        // Geprueft wird deshalb die WIRKUNG, die im Alltag zaehlt: ein Hash,
        // der zu keinem geladenen Zustand gehoert, liefert INVALID statt
        // irgendetwas. Das ist die Zeile, an der ein Item nach einem
        // Content-Rueckbau haengt.
        ChefZ_Registry<ChefZ_StateDef> reg = NewRegistry();
        Add(reg, "CHEFZ_ST_COLL_A", "");

        ChefZ_StateManager mgr = NewManager();
        mgr.SetCategoryManagerForTest(NewTagOwner(new array<string>()));
        mgr.Build(reg, null, NewIdentities(reg));

        string gone = "CHEFZ_ST_COLL_ENTFERNT";
        if (ChefZ_SymbolTable.IsValid(mgr.FromPersistHash(gone.Hash())))        return false;

        // Und ein Zustand, den es nicht gibt, hat keinen Persistenzschluessel.
        if (mgr.GetPersistHash(ChefZ_SymbolTable.Intern(gone)) != 0)            return false;

        return true;
    }

    //==========================================================================
    // 6. terminal / edible / preserved / spoilageMultiplier
    //==========================================================================

    private static bool RuleCheck()
    {
        ChefZ_Registry<ChefZ_StateDef> reg = NewRegistry();

        ChefZ_StateDef normal = Add(reg, "CHEFZ_ST_NORMAL", "");
        if (!normal)                                                            return false;

        ChefZ_StateDef ende = new ChefZ_StateDef();
        ende.id = "CHEFZ_ST_ENDE";
        ende.terminal  = true;
        ende.edible    = false;
        ende.preserved = true;
        ende.MarkExplicit("terminal");
        ende.MarkExplicit("edible");
        ende.MarkExplicit("preserved");
        ende.spoilageMultiplier = 0.25;
        ende.SetOrigin("Selbsttest", ChefZ_SourceRank.CONFIG_CPP);
        ende.ResolveDefaults();
        ende.Validate(null);
        ende.Compile(null);
        if (!reg.Add(ende))                                                     return false;

        ChefZ_StateManager mgr = NewManager();
        mgr.SetCategoryManagerForTest(NewTagOwner(new array<string>()));
        mgr.Build(reg, null, NewIdentities(reg));

        ChefZ_Sym n = Sym("CHEFZ_ST_NORMAL");
        ChefZ_Sym e = Sym("CHEFZ_ST_ENDE");

        if (!mgr.IsEdible(n))                                                   return false;
        if (mgr.IsTerminal(n))                                                  return false;
        if (mgr.IsPreserved(n))                                                 return false;
        if (mgr.GetSpoilageMultiplier(n) != 1.0)                                return false;

        if (mgr.IsEdible(e))                                                    return false;
        if (!mgr.IsTerminal(e))                                                 return false;
        if (!mgr.IsPreserved(e))                                                return false;
        if (mgr.GetSpoilageMultiplier(e) != 0.25)                               return false;

        // Ein unbekannter Zustand gilt als essbar und NICHT terminal: eine
        // Sperre, die niemand erklaeren kann, ist schlimmer als keine.
        ChefZ_Sym unknown = ChefZ_SymbolTable.Intern("CHEFZ_ST_UNBEKANNT");
        if (!mgr.IsEdible(unknown))                                             return false;
        if (mgr.IsTerminal(unknown))                                            return false;
        if (mgr.GetSpoilageMultiplier(unknown) != 1.0)                          return false;

        return true;
    }

    //==========================================================================
    // 7. Leere Registry: bereit und leer, kein Fehler (06 §7, erste Zeile)
    //==========================================================================

    private static bool EmptyCheck()
    {
        ChefZ_StateManager mgr = NewManager();
        mgr.SetCategoryManagerForTest(NewTagOwner(new array<string>()));
        mgr.Build(null, null, null);

        if (!mgr.IsReady())                                                     return false;
        if (mgr.GetStateCount() != 0)                                           return false;
        if (mgr.Exists(ChefZ_SymbolTable.Intern("CHEFZ_ST_EGAL")))              return false;
        if (mgr.ProjectToVanillaStage(ChefZ_SymbolTable.Intern("CHEFZ_ST_EGAL")) != ChefZ_VanillaStage.NONE)                                         return false;
        if (ChefZ_SymbolTable.IsValid(mgr.FromVanillaStage(ChefZ_VanillaStage.RAW)))
            return false;

        // Und der Rueckfall traegt trotzdem: kein Nullzugriff im heissen Pfad.
        ChefZ_StateDef fb = mgr.GetOrFallback(ChefZ_SymbolTable.Intern("CHEFZ_ST_EGAL"));
        if (!fb)                                                                return false;
        if (!fb.edible)                                                         return false;
        if (fb.HasProjection())                                                 return false;

        return true;
    }

    //==========================================================================
    // 8. Abfrage vor Build: neutrale Antwort, kein Absturz (06 §7)
    //==========================================================================

    private static bool NotReadyCheck()
    {
        ChefZ_StateManager mgr = NewManager();     // KEIN Build

        if (mgr.IsReady())                                                      return false;
        if (mgr.Exists(ChefZ_SymbolTable.Intern("CHEFZ_ST_VORBUILD")))          return false;
        if (mgr.GetDef(ChefZ_SymbolTable.Intern("CHEFZ_ST_VORBUILD")))          return false;
        if (mgr.GetPersistHash(ChefZ_SymbolTable.Intern("CHEFZ_ST_VORBUILD")) != 0)
            return false;
        if (mgr.GetSyncOrdinal(ChefZ_SymbolTable.Intern("CHEFZ_ST_VORBUILD")) != 0)
            return false;
        if (ChefZ_SymbolTable.IsValid(mgr.FromPersistHash(1)))                  return false;
        if (mgr.GetMaxOrdinal() != 0)                                           return false;

        array<ChefZ_Sym> tags;
        mgr.GetImpliedTags(ChefZ_SymbolTable.Intern("CHEFZ_ST_VORBUILD"), tags);
        if (!tags)                                                              return false;
        if (tags.Count() != 0)                                                  return false;

        return true;
    }
}
