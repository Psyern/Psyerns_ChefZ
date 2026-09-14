//==============================================================================
// ChefZ_FishingSelfTest - Abnahmepruefung des Fishing-Slice, soweit sie ohne
// Welt geht
//
// Entwurf: 20 §6 (die Sondentabelle T1-T11), 20 §4 (Schnittstellen),
// 20 §8 (Fehlerverhalten), 21 §2.4 und §3.4 (Validatorregeln).
//
// ---------------------------------------------------------------------------
// Was hier geprueft wird - und warum genau das
// ---------------------------------------------------------------------------
// Pruefbar OHNE laufende Welt ist alles, was gerechnet wird: die
// Gewichtsregel, die Koedertabelle, die Reihenfolge, der Fingerabdruck und
// die Klemmungen. Genau dieser Teil scheitert LEISE, und zwar leiser als in
// fast jedem anderen Teilsystem des Core:
//
//   - ein Faktor, der nicht greift, liefert weiterhin Faenge. Nur eben die
//     falschen, und das merkt niemand, weil Fangen ohnehin Zufall ist;
//   - ein lureOnly, das nicht sperrt, macht den seltensten Fang zum
//     haeufigsten - ohne eine einzige Zeile im Protokoll;
//   - eine Reihenfolge, die auf Client und Server auseinanderlaeuft, aendert
//     NICHT den gefangenen Gegenstand (den entscheidet der Server), sondern
//     die Zykluszeit der Aktion. Der Spieler sieht den Biss zu einem anderen
//     Zeitpunkt, als der Server ihn wertet - und meldet das als "Lag".
//
// NICHT pruefbar ist alles, was die Engine braucht:
//
//   - dass der Koeder am Haken wirklich gefunden wird (das macht die Engine
//     ueber die ganze Inventarhierarchie)
//   - dass ein Eintrag in der Fangtabelle der Welt landet
//   - dass der Kunstkoeder Schaden nimmt statt geloescht zu werden
//   - dass Client und Server denselben Index ziehen
//
// Alle vier brauchen einen Server mit Welt und bleiben dem Servertest
// vorbehalten. Geprueft wird hier, WORAUF sie beruhen. Die Sonde T7
// (Fingerabdruck) ist ausdruecklich die einzige, die ein Mensch auswerten
// muss: beide Seiten schreiben ihre Zahl ins Protokoll, und der Betreiber
// vergleicht. Eine automatische Abgleichsroutine waere Netzcode gegen ein
// Problem, das ungleiche PBOs voraussetzt.
//
// Der Test arbeitet auf EIGENEN Registryinstanzen, nie auf dem Singleton, und
// legt ausschliesslich Namen mit dem Praefix "CHEFZ_FS_" an - Namen, die in
// echtem Inhalt nicht vorkommen. Er beruehrt kein Item, keine Datei und keine
// Vanilla-Logik.
//
// Die EINE Ausnahme ist die Sonde T10: sie laeuft ueber den Registrar, und der
// fuehrt sein Flag notwendigerweise statisch. Sie stellt den Ausgangszustand
// danach wieder her - siehe dort.
//
// Layer: 5_Mission. Hier sind sowohl die Ertragsklassen aus 4_World als auch
// die Konstanten der Engine aus 3_Game sichtbar; das ist der einzige Ort, an
// dem der Spiegelvergleich der Masken ueberhaupt geschrieben werden kann.
//==============================================================================

/**
 * Registry mit ersetzter Klassenpruefung.
 *
 * Die einzige Ueberschreibung ist ClassIsSpawnable(). Alles andere - Reihenfolge,
 * Koedertabelle, Fingerabdruck - ist der echte Code. Ein Test, der die Regel
 * nachbaute, statt sie auszufuehren, wuerde seinen eigenen Nachbau pruefen.
 */
class ChefZ_FishingRegistryProbe extends ChefZ_FishingRegistry
{
    private ref array<string> m_Known;

    void ChefZ_FishingRegistryProbe()
    {
        m_Known = new array<string>();
        SetQuietForTest(true);
    }

    void DeclareClass(string cls)
    {
        if (m_Known.Find(cls) < 0)
            m_Known.Insert(cls);
    }

    protected override bool ClassIsSpawnable(string cls)
    {
        return m_Known.Find(cls) >= 0;
    }
}

/**
 * Ertragsobjekt mit fest vorgegebenem Koeder.
 *
 * Einen echten Rutenkontext gibt es ohne Welt, ohne Spieler und ohne Rute
 * nicht - sein Konstruktor liest die Fangtabelle der Welt und laeuft ueber
 * ein Inventar. Ersetzt wird deshalb GENAU die eine Methode, die den Kontext
 * anfasst; die Gewichtsregel darunter ist der echte Code und wird hier auch
 * ueber den echten Einstiegspunkt gerufen.
 */
class ChefZ_FishingYieldItemProbe extends ChefZ_FishingYieldItem
{
    private int m_ProbeBaitHash;

    void ChefZ_FishingYieldItemProbe(int baseWeight, ChefZ_FishingYieldDef def)
    {
        m_ProbeBaitHash = 0;
    }

    void SetProbeBait(int baitTypeHash)
    {
        m_ProbeBaitHash = baitTypeHash;
    }

    protected override int ChefZ_ResolveBaitHash(CatchingContextBase ctx)
    {
        return m_ProbeBaitHash;
    }
}

//==============================================================================

class ChefZ_FishingSelfTest
{
    //--- Testvokabular. Praefix CHEFZ_FS_, damit es mit keinem Inhalt
    //--- kollidieren kann; die Namen sind Platzhalter und bedeuten nichts.
    static const string Y_A     = "CHEFZ_FS_YIELD_A";       // 40, POND, ROD
    static const string Y_B     = "CHEFZ_FS_YIELD_B";       // 10, SEA, ROD, lureOnly
    static const string Y_C     = "CHEFZ_FS_YIELD_C";       // 20, BOTH, ROD+TRAP
    static const string Y_WEG   = "CHEFZ_FS_YIELD_OHNEKLASSE";

    static const string B_LURE  = "CHEFZ_FS_BAIT_L";        // lure,  Faktor 3, Ziele A und B
    static const string B_SOFT  = "CHEFZ_FS_BAIT_S";        // soft,  Faktor 2, Ziel C
    static const string B_FREMD = "CHEFZ_FS_BAIT_FREMD";    // nirgends deklariert

    private static int s_Passed;
    private static int s_Failed;
    private static ref array<string> s_FailedNames;

    //! Die Fixtures muessen die Pruefung ueberleben.
    //!
    //! Registry und Ertragsobjekt halten ihre Datensaetze bewusst OHNE ref:
    //! Eigentuemer ist in Produktion die Registry im Config Manager, und die
    //! lebt laenger. Eine Registry, die nur als lokale Variable einer
    //! Hilfsfunktion entsteht, ist dagegen schon abgeraeumt, bevor der Test
    //! sie befragt - der haelt dann ins Leere. Diese Liste bildet den
    //! Eigentuemer nach, den es in Produktion gibt.
    //!
    //! Zwei getrennte Listen statt einer gemeinsamen Oberklasse: der
    //! ChefZ_ContainerSelfTest haelt seine Fixtures genauso, und eine
    //! Aufwaertswandlung in einen ref-Container ist in Enforce nirgends
    //! zugesichert. Zwei Zeilen sind billiger als ein Zeiger, der ins Leere
    //! zeigt.
    private static ref array<ref ChefZ_Registry<ChefZ_FishingYieldDef>> s_AliveYields;
    private static ref array<ref ChefZ_Registry<ChefZ_BaitDef>>         s_AliveBaits;

    static bool Run()
    {
        s_Passed = 0;
        s_Failed = 0;
        s_FailedNames = new array<string>();
        s_AliveYields = new array<ref ChefZ_Registry<ChefZ_FishingYieldDef>>();
        s_AliveBaits  = new array<ref ChefZ_Registry<ChefZ_BaitDef>>();

        Check("Masken",         MaskMirrorCheck());
        Check("Maskennamen",    ChefZ_FishingMask.SelfCheck());
        Check("Ertragsmodell",  ChefZ_FishingYieldDef.SelfCheck());
        Check("Koedermodell",   ChefZ_BaitDef.SelfCheck());
        Check("Tabelle",        TableCheck());
        Check("Gewicht",        WeightCheck());
        Check("Kunstkoeder",    LureCheck());
        Check("Reihenfolge",    OrderCheck());
        Check("Fingerabdruck",  FingerprintCheck());
        Check("LeereTabelle",   EmptyCheck());
        Check("VorBuild",       NotReadyCheck());
        Check("Registrar",      RegistrarCheck());

        s_AliveYields = null;
        s_AliveBaits  = null;
        return s_Failed == 0;
    }

    private static void Check(string name, bool ok)
    {
        if (ok)
        {
            s_Passed++;
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.FISHING, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.FISHING, "Selbsttest " + name + ": ok");
            return;
        }

        s_Failed++;
        s_FailedNames.Insert(name);
        ChefZ_Log.Error(ChefZ_LogChannel.FISHING, "Selbsttest " + name + " FEHLGESCHLAGEN. Die Fangtabelle verhaelt sich nicht " + "wie entworfen - Koederpraeferenz und Fangverteilung sind ab hier " + "unzuverlaessig. Vanilla-Kochen ist davon unberuehrt.");
    }

    static int PassedCount() { return s_Passed; }
    static int FailedCount() { return s_Failed; }

    static string Summary()
    {
        int total = s_Passed + s_Failed;
        string s = "Selbsttest Fishing: " + s_Passed.ToString() + "/" + total.ToString() + " Gruppen ok";
        if (s_Failed > 0 && s_FailedNames)
        {
            s = s + "  gescheitert:";
            for (int i = 0; i < s_FailedNames.Count(); i++)
                s = s + " " + s_FailedNames.Get(i);
        }
        return s;
    }

    //==========================================================================
    // Die Spiegelung der Engine-Masken
    //==========================================================================

    /**
     * Die Beweispflicht aus dem Kopf von ChefZ_FishingDefs.
     *
     * Der Record liegt in 1_Core und darf die Konstanten der Engine nicht
     * sehen; er fuehrt deshalb dieselben Bits als Literale. Diese Gruppe ist
     * der Beleg, dass "dieselben" auch dieselben sind. Weicht die Engine je
     * ab, faellt es hier auf - beim Serverstart, mit Klartext, und nicht
     * einem Spieler, dem der Fang fehlt.
     */
    private static bool MaskMirrorCheck()
    {
        if (ChefZ_FishingMask.ENVIRO_POND != AnimalCatchingConstants.MASK_ENVIRO_POND)               return false;
        if (ChefZ_FishingMask.ENVIRO_SEA != AnimalCatchingConstants.MASK_ENVIRO_SEA)                 return false;
        if (ChefZ_FishingMask.ENVIRO_WATER_ALL != AnimalCatchingConstants.MASK_ENVIRO_WATER_ALL)     return false;
        if (ChefZ_FishingMask.METHOD_ROD != AnimalCatchingConstants.MASK_METHOD_ROD)                 return false;
        if (ChefZ_FishingMask.METHOD_TRAP_LARGE != AnimalCatchingConstants.MASK_METHOD_FISHTRAP_LARGE) return false;
        if (ChefZ_FishingMask.METHOD_TRAP_SMALL != AnimalCatchingConstants.MASK_METHOD_FISHTRAP_SMALL) return false;
        return true;
    }

    //==========================================================================
    // Aufbau der Tabelle
    //==========================================================================

    private static bool TableCheck()
    {
        ChefZ_FishingRegistryProbe reg = BuildProbe();
        if (!reg)                                                       return false;

        if (!reg.IsReady())                                             return false;
        if (reg.GetYieldCount() != 3)                                   return false;   // Y_WEG fehlt die Klasse
        if (reg.GetBaitCount() != 2)                                    return false;

        if (!reg.IsKnownYield(HashOf(Y_A)))                             return false;
        if (reg.IsKnownYield(HashOf(Y_WEG)))                            return false;
        if (!reg.IsKnownBait(HashOf(B_LURE)))                           return false;
        if (reg.IsKnownBait(HashOf(B_FREMD)))                           return false;

        return true;
    }

    //==========================================================================
    // T1, T2, T3, T4, T5, T8, T9 - die Gewichtsregel
    //==========================================================================

    private static bool WeightCheck()
    {
        ChefZ_FishingRegistryProbe reg = BuildProbe();
        if (!reg)                                                       return false;

        int noBait    = 0;
        int lureHash  = HashOf(B_LURE);
        int softHash  = HashOf(B_SOFT);
        int alienHash = HashOf(B_FREMD);

        // T1 - ohne Koeder ist der Faktor neutral.
        if (reg.GetWeightMultiplier(noBait, HashOf(Y_A)) != 1.0)        return false;
        if (reg.GetWeightMultiplier(noBait, HashOf(Y_B)) != 1.0)        return false;

        // T2 - der Zielkoeder hebt an.
        if (reg.GetWeightMultiplier(lureHash, HashOf(Y_A)) != 3.0)      return false;
        if (reg.GetWeightMultiplier(softHash, HashOf(Y_C)) != 2.0)      return false;

        // T3 - ein Koeder, der diesen Ertrag nicht nennt, hebt nichts an.
        if (reg.GetWeightMultiplier(lureHash, HashOf(Y_C)) != 1.0)      return false;

        // T9 - ein unbekannter Koeder bleibt neutral. Das ist die Zusage an
        //      Vanilla: der Wurm der Engine faengt weiterhin alles gleich.
        if (reg.GetWeightMultiplier(alienHash, HashOf(Y_A)) != 1.0)     return false;

        // Jetzt dieselbe Regel ueber das Ertragsobjekt.
        ChefZ_FishingYieldItemProbe itemA = MakeItem(reg, Y_A);
        ChefZ_FishingYieldItemProbe itemB = MakeItem(reg, Y_B);
        if (!itemA || !itemB)                                           return false;

        // Ohne Koeder: das Grundgewicht.
        if (itemA.ChefZ_ComputeWeight(noBait, reg) != 40)               return false;
        // Mit Zielkoeder: Grundgewicht mal Faktor.
        if (itemA.ChefZ_ComputeWeight(lureHash, reg) != 120)            return false;
        // Mit fremdem Koeder: wieder das Grundgewicht.
        if (itemA.ChefZ_ComputeWeight(softHash, reg) != 40)             return false;

        // T4 - lureOnly ohne passenden Koeder: kein Eintrag im Gewichtsfeld.
        if (itemB.ChefZ_ComputeWeight(noBait, reg) != 0)                return false;
        if (itemB.ChefZ_ComputeWeight(softHash, reg) != 0)              return false;
        // T5 - derselbe Ertrag mit passendem Koeder.
        if (itemB.ChefZ_ComputeWeight(lureHash, reg) != 30)             return false;

        // T8 - zweimal dieselbe Eingabe, zweimal dasselbe Ergebnis. In die
        //      Rechnung geht nichts ein, was sich zwischen zwei Aufrufen
        //      aendern koennte; die Sonde haelt das fest, damit es so bleibt.
        int first  = itemA.ChefZ_ComputeWeight(lureHash, reg);
        int second = itemA.ChefZ_ComputeWeight(lureHash, reg);
        if (first != second)                                            return false;

        // Und ueber den echten Einstiegspunkt, mit vorgegebenem Koeder. Ohne
        // ausdrueckliche Tabelle greift der Einstiegspunkt auf die Tabelle des
        // Prozesses zu - verglichen wird deshalb gegen genau diesen Aufruf und
        // nicht gegen eine feste Zahl. Die Aussage lautet "GetYieldWeight
        // fuehrt durch dieselbe Regel", nicht "die Tabelle des Prozesses hat
        // gerade diesen Inhalt".
        itemA.SetProbeBait(lureHash);
        if (itemA.GetYieldWeight(null) != itemA.ChefZ_ComputeWeight(lureHash)) return false;

        return true;
    }

    //==========================================================================
    // Kunstkoeder: Art und Verschleiss
    //==========================================================================

    private static bool LureCheck()
    {
        ChefZ_FishingRegistryProbe reg = BuildProbe();
        if (!reg)                                                       return false;

        if (!reg.IsLure(HashOf(B_LURE)))                                return false;
        if (reg.IsLure(HashOf(B_SOFT)))                                 return false;
        if (reg.IsLure(HashOf(B_FREMD)))                                return false;
        if (reg.IsLure(0))                                              return false;

        if (reg.GetLureWear(HashOf(B_LURE)) != ChefZ_FishingLimits.DEFAULT_LURE_WEAR) return false;
        if (reg.GetLureWear(HashOf(B_FREMD)) != 0.0)                    return false;

        return true;
    }

    //==========================================================================
    // T6 - die Reihenfolge, und das Gewichtsfeld, das daraus entsteht
    //==========================================================================

    private static bool OrderCheck()
    {
        ChefZ_FishingRegistryProbe reg = BuildProbe();
        if (!reg)                                                       return false;

        array<ChefZ_FishingYieldDef> first = new array<ChefZ_FishingYieldDef>();
        array<ChefZ_FishingYieldDef> second = new array<ChefZ_FishingYieldDef>();
        reg.GetOrderedYields(first);
        reg.GetOrderedYields(second);

        if (first.Count() != second.Count())                            return false;
        if (first.Count() != 3)                                         return false;
        for (int i = 0; i < first.Count(); i++)
        {
            if (first.Get(i).id != second.Get(i).id)                    return false;
        }

        // Die Ordnung ist (loadOrder, id). Y_C traegt loadOrder 1 und steht
        // deshalb HINTER Y_A und Y_B, obwohl sein Name dazwischen laege.
        if (first.Get(0).id != Y_A)                                     return false;
        if (first.Get(1).id != Y_B)                                     return false;
        if (first.Get(2).id != Y_C)                                     return false;

        // Und das Gewichtsfeld, das die Engine daraus baut: je Gewichtspunkt
        // ein Eintrag, in genau dieser Reihenfolge. Zweimal gebaut, zweimal
        // gleich - das ist die Aussage, an der die Angelaktion haengt.
        array<int> fieldA = new array<int>();
        array<int> fieldB = new array<int>();
        BuildProbabilityField(reg, HashOf(B_LURE), fieldA);
        BuildProbabilityField(reg, HashOf(B_LURE), fieldB);

        if (fieldA.Count() != fieldB.Count())                           return false;
        if (fieldA.Count() == 0)                                        return false;
        for (int k = 0; k < fieldA.Count(); k++)
        {
            if (fieldA.Get(k) != fieldB.Get(k))                         return false;
        }

        // 40*3 + 10*3 + 20*1 = 170
        if (fieldA.Count() != 170)                                      return false;

        // Ohne Koeder faellt Y_B als lureOnly ganz heraus: 40 + 20 = 60.
        array<int> fieldNoBait = new array<int>();
        BuildProbabilityField(reg, 0, fieldNoBait);
        if (fieldNoBait.Count() != 60)                                  return false;

        return true;
    }

    /**
     * Baut das Gewichtsfeld so, wie die Engine es baut: ueber die geordnete
     * Ertragsliste, je Gewichtspunkt ein Eintrag mit dem Typhash.
     *
     * Bewusst hier und nicht im Produktionscode: die Engine baut es selbst,
     * und ein zweiter Erbauer waere eine zweite Wahrheit. Der Test braucht ihn
     * trotzdem, weil die Aussage "beide Seiten bekommen dasselbe Feld" ohne
     * ein Feld nicht pruefbar ist.
     */
    private static void BuildProbabilityField(ChefZ_FishingRegistry registry, int baitTypeHash, notnull array<int> outField)
    {
        outField.Clear();

        array<ChefZ_FishingYieldDef> defs = new array<ChefZ_FishingYieldDef>();
        registry.GetOrderedYields(defs);

        for (int i = 0; i < defs.Count(); i++)
        {
            ChefZ_FishingYieldDef def = defs.Get(i);
            ChefZ_FishingYieldItemProbe item = new ChefZ_FishingYieldItemProbe(def.baseWeight, def);
            int weight = item.ChefZ_ComputeWeight(baitTypeHash, registry);
            for (int k = 0; k < weight; k++)
                outField.Insert(def.GetTypeHash());
        }
    }

    //==========================================================================
    // T7 - der Fingerabdruck
    //==========================================================================

    private static bool FingerprintCheck()
    {
        ChefZ_FishingRegistryProbe a = BuildProbe();
        ChefZ_FishingRegistryProbe b = BuildProbe();
        if (!a || !b)                                                   return false;

        // Gleiche Daten, gleiche Zahl. Das ist die Aussage, die ein Betreiber
        // zwischen Client und Server vergleicht.
        if (a.GetContentFingerprint() != b.GetContentFingerprint())     return false;
        if (a.GetContentFingerprint() == 0)                             return false;

        // Ein geaendertes Gewicht muss die Zahl bewegen - sonst koennte sie
        // Gleichheit behaupten, wo keine ist.
        ChefZ_FishingRegistryProbe c = BuildProbe(50);
        if (!c)                                                         return false;
        if (c.GetContentFingerprint() == a.GetContentFingerprint())     return false;

        return true;
    }

    //==========================================================================
    // Der Ausfallpfad: leer und vor dem Aufbau
    //==========================================================================

    /**
     * 20 §8, erste Zeile: ohne Datensaetze ist die Registry "bereit und leer"
     * und antwortet ruhig. In einem Core ohne Inhalt ist das der
     * NORMALZUSTAND - und das Angeln ist dann bitgenau Vanilla.
     */
    private static bool EmptyCheck()
    {
        ChefZ_FishingRegistryProbe reg = new ChefZ_FishingRegistryProbe();
        reg.Build(null, null, null);

        if (!reg.IsReady())                                             return false;
        if (reg.GetYieldCount() != 0)                                   return false;
        if (reg.GetBaitCount() != 0)                                    return false;
        if (reg.GetWeightMultiplier(HashOf(B_LURE), HashOf(Y_A)) != 1.0) return false;
        if (reg.IsLure(HashOf(B_LURE)))                                 return false;
        if (reg.GetLureWear(HashOf(B_LURE)) != 0.0)                     return false;
        if (reg.IsKnownBait(HashOf(B_LURE)))                            return false;

        array<ChefZ_FishingYieldDef> defs = new array<ChefZ_FishingYieldDef>();
        if (reg.GetOrderedYields(defs) != 0)                            return false;

        array<string> lines = new array<string>();
        reg.Dump(lines);
        if (lines.Count() == 0)                                         return false;

        return true;
    }

    //! Vor jedem Build: dieselben ruhigen Antworten. Eine Registry, die vor
    //! dem Boot gefragt wird, darf nichts behaupten - und nicht abstuerzen.
    private static bool NotReadyCheck()
    {
        ChefZ_FishingRegistryProbe reg = new ChefZ_FishingRegistryProbe();

        if (reg.IsReady())                                              return false;
        if (reg.GetWeightMultiplier(HashOf(B_LURE), HashOf(Y_A)) != 1.0) return false;
        if (reg.IsLure(HashOf(B_LURE)))                                 return false;
        if (reg.IsKnownYield(HashOf(Y_A)))                              return false;
        if (reg.GetContentFingerprint() != 0)                           return false;

        return true;
    }

    //==========================================================================
    // T10 - der Registrar laeuft genau einmal
    //==========================================================================

    /**
     * Die einzige Gruppe, die einen statischen Zustand ausserhalb ihrer
     * eigenen Fixtures anfasst.
     *
     * Sie muss es: das Flag gegen Doppelausfuehrung ist notwendigerweise
     * statisch, und genau dieses Flag ist die Zusage. Der Ausgangszustand wird
     * am Ende wiederhergestellt - der Selbsttest laeuft VOR dem Laden der
     * Config, der eigentliche Eintrag danach, und er darf nicht ausfallen,
     * weil dieser Test schon einmal "ja" gesagt hat.
     *
     * Zu diesem Zeitpunkt ist die Fangtabelle des Prozesses noch nicht
     * gebaut. Der Registrar nimmt deshalb seinen Ausfallpfad, traegt nichts
     * ein und meldet es - genau das Verhalten, das ein Server ohne Datenmodul
     * dauerhaft zeigt.
     */
    private static bool RegistrarCheck()
    {
        bool wasRegistered = ChefZ_FishingYieldRegistrar.HasRegistered();

        ChefZ_FishingYieldRegistrar.ResetForNewMission();
        if (ChefZ_FishingYieldRegistrar.HasRegistered())                return false;

        ChefZ_FishingRegisterReport report;
        bool firstRun = ChefZ_FishingYieldRegistrar.RegisterAll(report);
        if (firstRun)                                                   return false;   // nichts zu tun
        if (!report)                                                    return false;
        if (report.registered != 0)                                     return false;
        if (!ChefZ_FishingYieldRegistrar.HasRegistered())               return false;

        // Zweiter Lauf: no-op, und das Flag bleibt stehen.
        bool secondRun = ChefZ_FishingYieldRegistrar.RegisterAll(report);
        if (secondRun)                                                  return false;
        if (!ChefZ_FishingYieldRegistrar.HasRegistered())               return false;

        // Ausgangszustand wiederherstellen.
        ChefZ_FishingYieldRegistrar.ResetForNewMission();
        if (wasRegistered)
            return false;       // haette vor dem Boot nie gesetzt sein duerfen

        return true;
    }

    //==========================================================================
    // Hilfen
    //==========================================================================

    private static int HashOf(string name)
    {
        string local = name;
        return local.Hash();
    }

    /**
     * Die Fixtures. baseWeightA erlaubt eine zweite Tabelle mit EINEM anderen
     * Wert - mehr braucht der Fingerabdrucktest nicht.
     */
    private static ChefZ_FishingRegistryProbe BuildProbe(int baseWeightA = 40)
    {
        ChefZ_Registry<ChefZ_FishingYieldDef> yields = new ChefZ_Registry<ChefZ_FishingYieldDef>();
        yields.Init(ChefZ_RecordKind.FISHING_YIELD);
        ChefZ_Registry<ChefZ_BaitDef> baits = new ChefZ_Registry<ChefZ_BaitDef>();
        baits.Init(ChefZ_RecordKind.BAIT);

        s_AliveYields.Insert(yields);
        s_AliveBaits.Insert(baits);

        yields.Add(Yield(Y_A, baseWeightA, ChefZ_FishingMask.ENVIRO_POND_NAME, false, 0));
        yields.Add(Yield(Y_B, 10, ChefZ_FishingMask.ENVIRO_SEA_NAME, true, 0));
        yields.Add(Yield(Y_C, 20, ChefZ_FishingMask.ENVIRO_BOTH_NAME, false, 1));
        yields.Add(Yield(Y_WEG, 5, ChefZ_FishingMask.ENVIRO_POND_NAME, false, 0));

        array<string> lureTargets = new array<string>();
        lureTargets.Insert(Y_A);
        lureTargets.Insert(Y_B);
        baits.Add(Bait(B_LURE, ChefZ_BaitKind.LURE_NAME, 3.0, lureTargets));

        array<string> softTargets = new array<string>();
        softTargets.Insert(Y_C);
        softTargets.Insert("CHEFZ_FS_YIELD_GIBTSNICHT");    // faellt weg, Koeder bleibt
        baits.Add(Bait(B_SOFT, ChefZ_BaitKind.SOFT_NAME, 2.0, softTargets));

        ChefZ_FishingRegistryProbe reg = new ChefZ_FishingRegistryProbe();
        reg.DeclareClass(Y_A);
        reg.DeclareClass(Y_B);
        reg.DeclareClass(Y_C);
        reg.DeclareClass(B_LURE);
        reg.DeclareClass(B_SOFT);
        // Y_WEG wird ABSICHTLICH nicht deklariert: 20 §8 weist einen Eintrag
        // ohne CfgVehicles-Klasse ab, und das soll geprueft sein.

        reg.Build(yields, baits, null);
        return reg;
    }

    private static ChefZ_FishingYieldDef Yield(string id, int baseWeight, string enviro, bool lureOnly, int loadOrder)
    {
        ChefZ_FishingYieldDef def = new ChefZ_FishingYieldDef();
        def.id         = id;
        def.baseWeight = baseWeight;
        def.enviro     = enviro;
        def.loadOrder  = loadOrder;
        def.lureOnly   = lureOnly;
        def.MarkExplicit("lureOnly");
        def.MarkExplicit("loadOrder");
        def.methods    = new array<string>();
        def.methods.Insert(ChefZ_FishingMask.METHOD_ROD_NAME);

        def.ResolveDefaults();
        def.Compile(null);
        return def;
    }

    private static ChefZ_BaitDef Bait(string id, string kind, float multiplier, array<string> targets)
    {
        ChefZ_BaitDef def = new ChefZ_BaitDef();
        def.id               = id;
        def.baitKind         = kind;
        def.weightMultiplier = multiplier;
        def.targets          = targets;

        def.ResolveDefaults();
        def.Compile(null);
        return def;
    }

    private static ChefZ_FishingYieldItemProbe MakeItem(ChefZ_FishingRegistry registry, string id)
    {
        array<ChefZ_FishingYieldDef> defs = new array<ChefZ_FishingYieldDef>();
        registry.GetOrderedYields(defs);

        for (int i = 0; i < defs.Count(); i++)
        {
            ChefZ_FishingYieldDef def = defs.Get(i);
            if (def.id != id)
                continue;
            return new ChefZ_FishingYieldItemProbe(def.baseWeight, def);
        }
        return null;
    }
}
