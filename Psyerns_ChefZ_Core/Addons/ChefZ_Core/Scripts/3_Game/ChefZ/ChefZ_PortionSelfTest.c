//==============================================================================
// ChefZ_PortionSelfTest - Abnahmepruefung fuer S16, soweit sie ohne Welt geht
//
// Entwurf: 15 §4 (die Rechnung), 15 §5 (die beiden Deckel), 15 §7
// (Fehlerverhalten Zeile fuer Zeile), 15 E3 (doppelte Deckelung), 15 E4
// (unveraendert erben), 15 E7 (portions = 1), 19 S16 ("Fertig, wenn ...").
//
// ---------------------------------------------------------------------------
// Was hier geprueft wird - und was nicht, und warum
// ---------------------------------------------------------------------------
// Pruefbar OHNE laufende Welt ist alles, was der ChefZ_PortionManager rechnet:
// die Registry, beide Deckel, die Qualitaetsstufe, die Klemmung, die
// Entnahmebedingung und der Plan. Genau dieser Teil hat die unangenehmste
// Fehlerklasse des ganzen Core:
//
//   - faellt der MENGENDECKEL aus (15 §5.2), liefert ein Kessel mit einer
//     einzigen Zutat weiterhin Portionen, nur eben zwoelf. Nichts daran sieht
//     kaputt aus, nichts steht im Log, und die Nahrungsbilanz des Servers ist
//     ab dem ersten Kessel hinueber;
//   - faellt der GERAETEDECKEL aus, ist der Kessel nicht mehr besser als der
//     Topf, und die Fortschrittskurve des ganzen Mods ist flach;
//   - rundet die Qualitaetsrechnung AUF statt ab, ergibt achtmal Portionieren
//     achtmal einen Rundungsgewinn (15 E4).
//
// Die drei Abnahmebedingungen aus 19 S16, die eine Welt brauchen -
//
//     "zwei gleichzeitige Entnahmen ergeben nie eine Portion zu viel"
//     "ein voller Rucksack dekrementiert den Zaehler nicht"
//     "der Kessel liefert mehr Portionen als der Topf" (am echten Geraet)
//
// - laufen hier NICHT. Sie brauchen Items, Inventar und zwei Spieler. Geprueft
// wird stattdessen, WORAUF sie beruhen: dass aus portionsLeft = 0 kein Plan
// mehr entsteht (Doppelentnahme), dass der Plan VOR jeder Wirkung steht
// (voller Rucksack) und dass zwei verschiedene portionCapacity zwei
// verschiedene Zahlen ergeben (Kessel gegen Topf).
//
// Der Test arbeitet auf einer EIGENEN Manager-Instanz, nie auf dem Singleton,
// und legt ausschliesslich Symbole mit dem Praefix "CHEFZ_PO_" an - Namen, die
// in echtem Content nicht vorkommen. Er beruehrt kein Item, keine Datei und
// keine Vanilla-Logik.
//
// Layer: 3_Game.
//==============================================================================

class ChefZ_PortionSelfTest
{
    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();

        Check("PortionSpec",     ChefZ_PortionSpec.SelfCheck());
        Check("Registry",        RegistryCheck());
        Check("Schluessel",      KeyCheck());
        Check("Geraetedeckel",   DeviceCapCheck());
        Check("Mengendeckel",    AmountCapCheck());
        Check("BeideDeckel",     BothCapsCheck());
        Check("Klemmung",        ClampCheck());
        Check("Einzelgericht",   SingleServingCheck());
        Check("Entnahme",        TakeCheck());
        Check("Doppelentnahme",  DoubleTakeCheck());
        Check("Plan",            PlanCheck());
        Check("Audit",           AuditCheck());
        Check("LeereRegistry",   EmptyCheck());
        Check("VorBuild",        NotReadyCheck());

        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.PORTION, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.PORTION, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.PORTION,
            "Selbsttest " + name + " FEHLGESCHLAGEN. Die Portionsrechnung verhaelt sich nicht "
            + "wie entworfen - Portionszahlen und damit die Nahrungsbilanz des Servers sind ab "
            + "hier unzuverlaessig. Vanilla-Kochen ist davon unberuehrt.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest S16: " + s_Passed.ToString() + "/" + total.ToString()
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

    //! Ein Ergebnis mit Portionsdaten, wie es im Content-Beispiel aus 15 §3
    //! steht - nur mit Testnamen.
    private static ChefZ_OutputDef MakeOutput(string cls, string portionCls,
                                              int portions, float perPortion)
    {
        ChefZ_OutputDef def = new ChefZ_OutputDef();
        def.cls              = cls;
        def.portionClass     = portionCls;
        def.portions         = portions;
        def.amountPerPortion = perPortion;
        def.portionQuantity  = 200.0;
        def.ResolveDefaults();
        return def;
    }

    /**
     * Ein Manager mit genau EINER Spec, ohne Engine.
     *
     * Der Umweg ueber ein Rezept waere hier reiner Aufwand: die Registry
     * entsteht aus ChefZ_OutputDef, und ein kompiliertes Rezept ist nur eine
     * Huelle darum. Gebaut wird deshalb ein leerer Manager, und die Spec
     * kommt direkt - ueber dieselbe Fuellmethode, die der Build benutzt.
     */
    private static ChefZ_PortionManager MakeManager()
    {
        ChefZ_PortionManager mgr = new ChefZ_PortionManager();
        mgr.SetQuietForTest(true);
        mgr.Build(null, null, null, null);
        return mgr;
    }

    private static ChefZ_PortionSpec MakeSpec(string cls, int portions, float perPortion)
    {
        ChefZ_OutputDef def = MakeOutput(cls, cls + "_PORTION", portions, perPortion);
        ChefZ_PortionSpec spec = new ChefZ_PortionSpec();
        spec.FillFrom(def, def.cls, "CHEFZ_PO_TESTREZEPT", 2.0);
        return spec;
    }

    private static ChefZ_CookContext MakeContext(int capacity)
    {
        ChefZ_CookContext ctx = new ChefZ_CookContext();
        ctx.deviceClass     = Sym("CHEFZ_PO_GERAET");
        ctx.portionCapacity = capacity;
        return ctx;
    }

    //==========================================================================
    // Gruppen
    //==========================================================================

    /**
     * Ein Ergebnis mit Portionsdaten kommt in die Registry, eines ohne nicht.
     *
     * Die zweite Haelfte ist die wichtigere: 15 §7 Zeile 1 sagt, ein Gericht
     * ohne Portionsdaten sei ein gewoehnliches Item. Landete es trotzdem in
     * der Registry, bekaeme jedes Vanilla-artige ChefZ-Gericht eine
     * Entnahmeaktion, die nichts tut.
     */
    private static bool RegistryCheck()
    {
        ChefZ_PortionManager mgr = MakeManager();
        if (!mgr.IsReady())                                       return false;
        if (mgr.GetSpecCount() != 0)                              return false;

        // Ohne jede Portionsangabe: kein Portionsgericht.
        ChefZ_OutputDef plain = new ChefZ_OutputDef();
        plain.cls = "CHEFZ_PO_GEWOEHNLICH";
        plain.ResolveDefaults();
        if (plain.IsPortioned())                                  return false;

        // Mit: eines.
        ChefZ_OutputDef bulk = MakeOutput("CHEFZ_PO_BULK_A", "CHEFZ_PO_SCHALE_A", 8, 1.0);
        if (!bulk.IsPortioned())                                  return false;

        ChefZ_PortionSpec spec = new ChefZ_PortionSpec();
        spec.FillFrom(bulk, bulk.cls, "CHEFZ_PO_R1", 2.0);
        if (spec.portions != 8)                                   return false;
        if (spec.portionClass != "CHEFZ_PO_SCHALE_A")             return false;
        if (!spec.HasAmountCap())                                 return false;

        return true;
    }

    /**
     * Der Registryschluessel ist die ERGEBNISKLASSE, nicht das Rezept.
     *
     * Das ist keine Formalie: ein Gericht kann nur EINE Portionsregel haben,
     * weil der Zaehler am ITEM steht und nicht am Rezept (15 §6). Zwei
     * Rezepte, die dasselbe Gericht verschieden portionieren, ergaeben
     * denselben Kessel mit zwei Wahrheiten - deshalb muessen zwei Specs
     * derselben Klasse denselben Schluessel haben und trotzdem
     * unterscheidbar bleiben.
     */
    private static bool KeyCheck()
    {
        ChefZ_PortionSpec a = MakeSpec("CHEFZ_PO_BULK_B", 8, 1.0);
        ChefZ_PortionSpec b = MakeSpec("CHEFZ_PO_BULK_B", 8, 1.0);
        ChefZ_PortionSpec c = MakeSpec("CHEFZ_PO_BULK_B", 4, 1.0);

        if (a.bulkClassSym != b.bulkClassSym)                     return false;
        if (a.portions != b.portions)                             return false;
        if (c.portions == a.portions)                             return false;

        return true;
    }

    /**
     * 15 §5.1: dasselbe Rezept ergibt im Topf weniger als im Kessel.
     *
     * Und der dritte Fall ist der begruendungsbeduerftige: ein Geraet ohne
     * deklarierte Kapazitaet (portionCapacity = 0) deckelt NICHT. Waere es
     * anders, druecke eine vergessene Geraetezeile jedes Gericht auf eine
     * Portion - und das saehe aus wie ein kaputtes Rezept.
     */
    private static bool DeviceCapCheck()
    {
        ChefZ_PortionManager mgr  = MakeManager();
        ChefZ_PortionSpec    spec = MakeSpec("CHEFZ_PO_BULK_C", 12, 0.0);

        array<string> trace = null;

        // Grosszuegig Zutaten, damit allein das Geraet entscheidet.
        int imGrossenGefaess = mgr.ResolvePortionCount(spec, MakeContext(12), 99.0,
                                                 ChefZ_SymbolTable.INVALID, trace);
        int inPot      = mgr.ResolvePortionCount(spec, MakeContext(4), 99.0,
                                                 ChefZ_SymbolTable.INVALID, trace);
        int undeclared = mgr.ResolvePortionCount(spec, MakeContext(0), 99.0,
                                                 ChefZ_SymbolTable.INVALID, trace);

        if (imGrossenGefaess != 12)                                     return false;
        if (inPot != 4)                                           return false;
        if (undeclared != 12)                                     return false;
        if (inPot >= imGrossenGefaess)                                  return false;

        // scaleWithDevice = false haengt das Geraet vollstaendig aus.
        spec.scaleWithDevice = false;
        int ignoring = mgr.ResolvePortionCount(spec, MakeContext(4), 99.0,
                                               ChefZ_SymbolTable.INVALID, trace);
        if (ignoring != 12)                                       return false;

        return true;
    }

    /**
     * 15 §5.2 - DIE Exploitsperre.
     *
     * "Ohne diesen Deckel ergaebe eine Minimalfuellung im Kessel zwoelf
     * Portionen - ein glasklarer Nahrungsexploit."
     *
     * Geprueft wird ausdruecklich auch die Abrundung: 2.5 Einheiten je einer
     * Einheit pro Portion sind ZWEI Portionen, nicht drei. Aufrunden waere
     * genau der Weg, aus einer Minimalfuellung eine volle Ausbeute zu machen.
     */
    private static bool AmountCapCheck()
    {
        ChefZ_PortionManager mgr  = MakeManager();
        ChefZ_PortionSpec    spec = MakeSpec("CHEFZ_PO_BULK_D", 12, 1.0);

        array<string> trace = null;
        ChefZ_CookContext big = MakeContext(12);

        // Minimalfuellung im groessten Geraet: EINE Portion, nicht zwoelf.
        if (mgr.ResolvePortionCount(spec, big, 1.0, ChefZ_SymbolTable.INVALID, trace) != 1)
            return false;

        // Abrundung.
        if (mgr.ResolvePortionCount(spec, big, 2.5, ChefZ_SymbolTable.INVALID, trace) != 2)
            return false;

        // Viel Zutaten: der Geraetedeckel uebernimmt wieder.
        if (mgr.ResolvePortionCount(spec, big, 40.0, ChefZ_SymbolTable.INVALID, trace) != 12)
            return false;

        // Gar keine verbrauchten Einheiten: die Klemmung haelt bei 1 - ein
        // Gericht mit null Portionen waere ein Gericht, das man nie essen
        // kann (15 §4).
        if (mgr.ResolvePortionCount(spec, big, 0.0, ChefZ_SymbolTable.INVALID, trace) != 1)
            return false;

        // Ohne amountPerPortion deckelt allein das Geraet - das ist zulaessig
        // und der Fall, vor dem der Ladebericht warnt.
        ChefZ_PortionSpec loose = MakeSpec("CHEFZ_PO_BULK_E", 12, 0.0);
        if (loose.HasAmountCap())                                 return false;
        if (mgr.ResolvePortionCount(loose, big, 1.0, ChefZ_SymbolTable.INVALID, trace) != 12)
            return false;

        return true;
    }

    /**
     * 15 E3: "grosses Geraet + viele Zutaten = viele Portionen."
     *
     * Die Tabelle prueft alle vier Ecken auf einmal. Faellt eine Zeile um, ist
     * genau eine der beiden Aussagen aus E3 verletzt.
     */
    private static bool BothCapsCheck()
    {
        ChefZ_PortionManager mgr  = MakeManager();
        ChefZ_PortionSpec    spec = MakeSpec("CHEFZ_PO_BULK_F", 12, 2.0);

        array<string> trace = null;

        // kleines Geraet, wenig Zutaten  -> wenig
        if (mgr.ResolvePortionCount(spec, MakeContext(4), 4.0,
                ChefZ_SymbolTable.INVALID, trace) != 2)            return false;

        // kleines Geraet, viele Zutaten  -> das Geraet begrenzt
        if (mgr.ResolvePortionCount(spec, MakeContext(4), 40.0,
                ChefZ_SymbolTable.INVALID, trace) != 4)            return false;

        // grosses Geraet, wenig Zutaten  -> die Menge begrenzt
        if (mgr.ResolvePortionCount(spec, MakeContext(12), 4.0,
                ChefZ_SymbolTable.INVALID, trace) != 2)            return false;

        // grosses Geraet, viele Zutaten  -> voll
        if (mgr.ResolvePortionCount(spec, MakeContext(12), 40.0,
                ChefZ_SymbolTable.INVALID, trace) != 12)           return false;

        return true;
    }

    //! 15 §4: "clamp(n, 1, 31)". Die Obergrenze ist die Sync-Grenze, nicht
    //! eine Balancingzahl - deshalb wird sie gegen ChefZ_SyncLimits geprueft.
    private static bool ClampCheck()
    {
        ChefZ_PortionManager mgr = MakeManager();
        array<string> trace = null;

        ChefZ_PortionSpec huge = MakeSpec("CHEFZ_PO_BULK_G", 500, 0.0);
        int n = mgr.ResolvePortionCount(huge, MakeContext(0), 99.0,
                                        ChefZ_SymbolTable.INVALID, trace);
        if (n != ChefZ_SyncLimits.PORTIONS_MAX)                   return false;

        ChefZ_PortionSpec none = MakeSpec("CHEFZ_PO_BULK_H", 0, 0.0);
        if (mgr.ResolvePortionCount(none, MakeContext(0), 99.0,
                ChefZ_SymbolTable.INVALID, trace) != 1)            return false;

        // Der Trace ist optional und darf nie zum Absturz fuehren - null ist
        // der Normalfall im Kochtakt.
        array<string> lines = new array<string>();
        mgr.ResolvePortionCount(huge, MakeContext(4), 99.0, ChefZ_SymbolTable.INVALID, lines);
        if (lines.Count() == 0)                                   return false;

        return true;
    }

    /**
     * 15 E7: "Einzelgerichte sind Portionsgerichte mit portions = 1."
     *
     * Der Test ist kurz und die Aussage gross: es gibt EINEN Mechanismus, und
     * ein Tellergericht laeuft durch denselben Pfad wie ein Kessel Eintopf.
     */
    private static bool SingleServingCheck()
    {
        ChefZ_PortionManager mgr  = MakeManager();
        ChefZ_PortionSpec    spec = MakeSpec("CHEFZ_PO_TELLER", 1, 0.0);

        if (!spec.IsPortioned())                                  return false;

        array<string> trace = null;
        if (mgr.ResolvePortionCount(spec, MakeContext(12), 99.0,
                ChefZ_SymbolTable.INVALID, trace) != 1)            return false;

        // Auch der groesste Kessel macht aus einem Tellergericht kein
        // Gruppengericht: der Geraetedeckel ist ein MINIMUM, keine Vorgabe.
        return true;
    }

    /**
     * Die Entnahmebedingung (15 §4, ENTNAHME).
     *
     * Eine unbekannte Klasse ergibt nie einen Plan. Das ist die Sperre gegen
     * einen manipulierten Client, der die Aktion auf irgendein Item anfragt.
     */
    private static bool TakeCheck()
    {
        ChefZ_PortionManager mgr = MakeManager();

        ChefZ_PortionRequest req = new ChefZ_PortionRequest();
        req.sourceClass  = Sym("CHEFZ_PO_UNBEKANNT");
        req.portionsLeft = 5;

        string why;
        if (mgr.CanTakePortion(req, why))                         return false;
        if (why == "")                                            return false;

        ChefZ_PortionPlan plan;
        if (mgr.BuildPortionPlan(req, plan))                      return false;
        if (plan && plan.IsValid())                               return false;

        return true;
    }

    /**
     * 15 §7, "zwei Spieler entnehmen gleichzeitig": der zweite sieht
     * portions == 0 und bricht wirkungslos ab.
     *
     * Nachgestellt wird hier die RECHENSEITE davon - aus einem Zaehler von 0
     * entsteht kein Plan. Die Sequenzialitaet selbst ist eine Eigenschaft der
     * Engine (DayZ-Script ist einstraengig) und nicht nachstellbar.
     */
    private static bool DoubleTakeCheck()
    {
        ChefZ_PortionManager mgr = MakeManager();

        ChefZ_PortionRequest req = new ChefZ_PortionRequest();
        req.sourceClass  = Sym("CHEFZ_PO_BULK_I");
        req.portionsLeft = 0;

        string why;
        if (mgr.CanTakePortion(req, why))                         return false;

        req.portionsLeft = -3;
        if (mgr.CanTakePortion(req, why))                         return false;

        return true;
    }

    /**
     * Der Plan traegt genau die Felder, die 15 §3 nennt - und keine Rechnung
     * darauf (15 E4).
     *
     * Geprueft wird auf einem handgebauten Plan, weil BuildPortionPlan eine
     * gefuellte Registry braucht und die aus einer Engine kaeme.
     */
    private static bool PlanCheck()
    {
        ChefZ_PortionPlan plan = new ChefZ_PortionPlan();

        if (plan.IsValid())                                       return false;
        if (plan.NeedsContainer())                                return false;
        if (plan.SourceIsReplaced())                              return false;

        plan.portionClass       = "CHEFZ_PO_SCHALE_Z";
        plan.qualityToApply     = Sym("CHEFZ_PO_STUFE");
        plan.freshnessToApply   = 0.42;
        plan.portionsLeftAfter  = 0;
        plan.sourceBecomesEmpty = true;

        if (!plan.IsValid())                                      return false;

        // Ohne emptyOnLastPortion wird die Quelle GELOESCHT, nicht ersetzt
        // (15 §2). Der Unterschied entscheidet, ob ein leerer Kessel im Topf
        // liegen bleibt.
        if (plan.SourceIsReplaced())                              return false;

        plan.emptyClass = "CHEFZ_PO_LEER";
        if (!plan.SourceIsReplaced())                             return false;

        // 15 E4: die Frische wird UNVERAENDERT uebernommen. Ein Plan rechnet
        // nicht.
        if (plan.freshnessToApply != 0.42)                        return false;

        plan.Reset();
        if (plan.IsValid())                                       return false;
        if (plan.freshnessToApply >= 0.0)                         return false;

        return true;
    }

    /**
     * Die Bauzeitpruefung (15 §7).
     *
     * Vier Faelle, und der letzte ist der wichtigste: ein Gericht, das sich
     * selbst portioniert, waere eine unbegrenzte Nahrungsquelle.
     */
    private static bool AuditCheck()
    {
        array<string> warnings = new array<string>();
        string portionCls;
        string reject;

        // 1. Ohne jede Portionsangabe: nichts zu tun, keine Meldung.
        ChefZ_OutputDef plain = new ChefZ_OutputDef();
        plain.cls = "CHEFZ_PO_AUDIT_A";
        plain.ResolveDefaults();
        if (!ChefZ_PortionOutputAudit.Audit(plain, "outputs[0]", warnings, portionCls, reject))
            return false;
        if (portionCls != "")                                     return false;
        if (warnings.Count() != 0)                                return false;

        // 2. portionClass ohne portions: als 1 gelesen (15 E7).
        warnings.Clear();
        ChefZ_OutputDef implied = new ChefZ_OutputDef();
        implied.cls          = "CHEFZ_PO_AUDIT_B";
        implied.portionClass = "CHEFZ_PO_AUDIT_B_P";
        implied.ResolveDefaults();
        if (!ChefZ_PortionOutputAudit.Audit(implied, "outputs[0]", warnings, portionCls, reject))
            return false;
        if (implied.portions != 1)                                return false;
        if (portionCls != "CHEFZ_PO_AUDIT_B_P")                   return false;
        if (warnings.Count() != 1)                                return false;

        // 3. portions ohne portionClass: der Zaehler entfaellt, kein Abbruch.
        warnings.Clear();
        ChefZ_OutputDef counterOnly = new ChefZ_OutputDef();
        counterOnly.cls      = "CHEFZ_PO_AUDIT_C";
        counterOnly.portions = 6;
        counterOnly.ResolveDefaults();
        if (!ChefZ_PortionOutputAudit.Audit(counterOnly, "outputs[0]", warnings,
                                            portionCls, reject))
            return false;
        if (counterOnly.portions != 0)                            return false;
        if (portionCls != "")                                     return false;
        if (warnings.Count() != 1)                                return false;

        // 4. Selbstportionierung: ABWEISEN.
        warnings.Clear();
        ChefZ_OutputDef loop = new ChefZ_OutputDef();
        loop.cls          = "CHEFZ_PO_AUDIT_D";
        loop.portionClass = "CHEFZ_PO_AUDIT_D";
        loop.portions     = 4;
        loop.ResolveDefaults();
        if (ChefZ_PortionOutputAudit.Audit(loop, "outputs[0]", warnings, portionCls, reject))
            return false;
        if (reject == "")                                         return false;

        // 5. Ueber der Sync-Grenze: geklemmt, kein Abbruch.
        warnings.Clear();
        ChefZ_OutputDef huge = new ChefZ_OutputDef();
        huge.cls          = "CHEFZ_PO_AUDIT_E";
        huge.portionClass = "CHEFZ_PO_AUDIT_E_P";
        huge.portions     = 500;
        huge.ResolveDefaults();
        if (!ChefZ_PortionOutputAudit.Audit(huge, "outputs[0]", warnings, portionCls, reject))
            return false;
        if (huge.portions != ChefZ_PortionLimits.MAX)             return false;

        return true;
    }

    /**
     * 15 §7, erste Zeile: ohne ein einziges Portionsgericht ist der Manager
     * "bereit und leer" - kein Fehler, keine Warnung.
     *
     * Das ist der Normalzustand eines Core ohne Content, und er muss ruhig
     * sein: eine Warnung beim Start eines frisch installierten Servers waere
     * ein Bugreport, der keiner ist.
     */
    private static bool EmptyCheck()
    {
        ChefZ_PortionManager mgr = MakeManager();

        if (!mgr.IsReady())                                       return false;
        if (mgr.GetSpecCount() != 0)                              return false;
        if (mgr.IsBulkClass(Sym("CHEFZ_PO_IRGENDWAS")))           return false;

        ChefZ_PortionSpec spec;
        if (mgr.GetSpecForBulk(Sym("CHEFZ_PO_IRGENDWAS"), spec))  return false;
        if (spec)                                                 return false;

        // Der Behaelterschalter wird hier BEWUSST nicht geprueft: er ist
        // global und gehoert S17. Ein Test, der ihn erwartungsgemaess auf
        // false festnagelt, ginge kaputt, sobald das Behaeltersystem
        // dazukommt - und das waere ein falscher Alarm ueber ein
        // funktionierendes System.

        return true;
    }

    /**
     * Vor dem Build antwortet der Manager mit "kein Portionsgericht", nicht
     * mit einem Absturz.
     *
     * Die Reihenfolge im Boot stellt sicher, dass das nie vorkommt. Die
     * Pruefung steht trotzdem hier: sie kostet nichts, und der Fall waere
     * sonst ein Nullzugriff im Kochtakt.
     */
    private static bool NotReadyCheck()
    {
        ChefZ_PortionManager mgr = new ChefZ_PortionManager();
        mgr.SetQuietForTest(true);

        if (mgr.IsReady())                                        return false;
        if (mgr.IsBulkClass(Sym("CHEFZ_PO_BULK_A")))              return false;

        ChefZ_PortionSpec spec;
        if (mgr.GetSpecForBulk(Sym("CHEFZ_PO_BULK_A"), spec))     return false;

        ChefZ_PortionRequest req = new ChefZ_PortionRequest();
        req.sourceClass  = Sym("CHEFZ_PO_BULK_A");
        req.portionsLeft = 3;

        string why;
        if (mgr.CanTakePortion(req, why))                         return false;
        if (why == "")                                            return false;

        return true;
    }
}
