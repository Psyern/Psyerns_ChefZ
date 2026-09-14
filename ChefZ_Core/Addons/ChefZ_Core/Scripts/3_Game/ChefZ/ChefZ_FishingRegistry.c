//==============================================================================
// ChefZ_FishingRegistry - Fangtabelle und Koederpraeferenz, einmal beim Boot
//
// Entwurf: 20 §4.2 (Schnittstelle woertlich), 20 §6 (Determinismus D1/D2),
// 20 §8 (Fehlerverhalten Zeile fuer Zeile), 20 E6 (Hashes statt Symbole im
// heissen Pfad), 21 §2.4 und §3.4 (Validatorregeln).
//
// ---------------------------------------------------------------------------
// Die eine Zusage dieser Klasse
// ---------------------------------------------------------------------------
// Im gesamten Core steht kein Ertragsname und kein Koedername. Ein Koeder
// nennt Ertraege, ein Ertrag nennt ein Gewaesser - beides steht in den Daten
// eines Content-Moduls. Kommt ein neuer Ertrag dazu, aendert sich hier keine
// Zeile (Regel 4).
//
// ---------------------------------------------------------------------------
// Warum Hashes und nicht ChefZ_Sym (20 E6)
// ---------------------------------------------------------------------------
// Die Engine adressiert Ertraege ausschliesslich ueber GetType().Hash():
// CatchYieldBank fuehrt seine Tabelle so, und das Wahrscheinlichkeitsfeld der
// Angelaktion besteht aus genau diesen Zahlen. Eine zweite Identitaetsachse
// haette hier nur Uebersetzungskosten und eine weitere Fehlerquelle.
// ChefZ_Sym bleibt fuer alles, was in Rezepte und in die Persistenz geht.
//
// ---------------------------------------------------------------------------
// Determinismus (20 §6 D1) - der eigentliche Grund fuer GetOrderedYields
// ---------------------------------------------------------------------------
// Client und Server bauen das Gewichtsfeld der Angelaktion je fuer sich und
// ziehen daraus mit EINEM synchronisierten Zufallsstrom denselben Index. Das
// Feld entsteht aus der Iteration der Ertragstabelle, und deren Reihenfolge
// ist eine reine Funktion der Einfuegereihenfolge. Also muss die
// Einfuegereihenfolge auf beiden Seiten dieselbe sein - dafuer gibt es
// GetOrderedYields(), und nur dafuer.
//
// Sortiert wird nach (loadOrder, id) und ausdruecklich NICHT zusaetzlich nach
// sourceRef, obwohl 20 §6 D1 ihn nennt. Zwei Gruende, und der zweite ist der
// wichtige: erstens ist die id innerhalb einer Art eindeutig (die Registry
// laesst keine zweite zu), sourceRef koennte also nie entscheiden; zweitens
// haengt ChefZ_Record.PatchFrom die Herkunft des patchenden Rangs an sourceRef
// AN - ein serverseitiger Patch machte die Zeichenkette auf den beiden Seiten
// verschieden. Ein Sortierschluessel, der auf einer Seite anders aussieht, ist
// genau das Gegenteil dessen, was D1 will.
//
// Layer: 3_Game. Nachbar des Config Managers, kein EntityAI.
//==============================================================================

/**
 * Ein Koeder in aufgeloester Form.
 *
 * Eigene Klasse statt dreier paralleler Maps: der heisse Pfad fragt IMMER
 * zuerst "kenne ich diesen Koeder", und danach je nach Aufrufer Faktor,
 * Verschleiss oder Art. Ein Eintrag, drei Antworten.
 */
class ChefZ_BaitEntry : Managed
{
    string  id;
    int     typeHash;
    int     kind;                       // ChefZ_BaitKind.LURE | SOFT
    float   weightMultiplier;
    float   lureWearPerUse;

    //! Ertragshash -> 1. Als Map und nicht als Liste, weil GetWeightMultiplier
    //! im Gewichtsaufbau je Ertrag einmal laeuft und O(1) bleiben soll.
    ref map<int, int> targets;

    void ChefZ_BaitEntry()
    {
        id               = "";
        typeHash         = 0;
        kind             = ChefZ_BaitKind.SOFT;
        weightMultiplier = ChefZ_FishingLimits.DEFAULT_WEIGHT_MULT;
        lureWearPerUse   = ChefZ_FishingLimits.DEFAULT_LURE_WEAR;
        targets          = new map<int, int>();
    }

    bool Targets(int yieldTypeHash)
    {
        return targets.Contains(yieldTypeHash);
    }
}

//==============================================================================

class ChefZ_FishingRegistry : Managed
{
    private static ref ChefZ_FishingRegistry s_Instance;

    /**
     * Die Ertraege in Registrierungsreihenfolge (siehe Kopf, D1).
     *
     * OHNE ref auf dem Elementtyp: Eigentuemer der Datensaetze ist die
     * Registry im Config Manager, und die lebt laenger als diese Klasse.
     * Dieselbe Aufteilung wie beim ChefZ_ProcessingManager. Der Selbsttest
     * baut sich deshalb einen eigenen Eigentuemer.
     */
    private ref array<ChefZ_FishingYieldDef> m_Ordered;

    //! Ertragshash -> Datensatz. Fuer die Deckungspruefung der Koeder.
    private ref map<int, ChefZ_FishingYieldDef> m_YieldsByHash;

    //! Koederhash -> Eintrag. DER heisse Index.
    private ref map<int, ref ChefZ_BaitEntry> m_BaitsByHash;

    //! Koeder in stabiler Reihenfolge - nur fuer Fingerabdruck und Auszug.
    private ref array<ChefZ_BaitEntry> m_BaitOrder;

    private bool m_Ready;
    private int  m_Fingerprint;
    private int  m_RejectedYields;
    private int  m_RejectedBaits;
    private bool m_QuietForTest;

    void ChefZ_FishingRegistry()
    {
        m_Ordered        = new array<ChefZ_FishingYieldDef>();
        m_YieldsByHash   = new map<int, ChefZ_FishingYieldDef>();
        m_BaitsByHash    = new map<int, ref ChefZ_BaitEntry>();
        m_BaitOrder      = new array<ChefZ_BaitEntry>();
        m_Ready          = false;
        m_Fingerprint    = 0;
        m_RejectedYields = 0;
        m_RejectedBaits  = 0;
        m_QuietForTest   = false;
    }

    static ChefZ_FishingRegistry Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_FishingRegistry();
        return s_Instance;
    }

    //==========================================================================
    // BUILD
    //==========================================================================

    /**
     * Baut beide Tabellen.
     *
     * Beide Parameter duerfen null sein. Ein Aufruf mit null ist die
     * ausdrueckliche Art zu sagen "Bestand leeren" - genau das braucht der
     * SAFE_MODE (02 §8, 20 §8). Danach ist die Registry "bereit und leer":
     * GetWeightMultiplier liefert 1.0, IsLure liefert false, und das Angeln
     * ist bitgenau Vanilla.
     */
    void Build(ChefZ_Registry<ChefZ_FishingYieldDef> yields, ChefZ_Registry<ChefZ_BaitDef> baits, ChefZ_LoadReport report)
    {
        Clear();
        m_Ready = true;

        BuildYields(yields, report);
        BuildBaits(baits, report);
        WarnOnUnreachableYields(report);

        m_Fingerprint = ComputeFingerprint();

        if (report)
        {
            string chefzTxt1 = "Fangtabelle: " + m_Ordered.Count().ToString() + " Ertraege, " + m_BaitOrder.Count().ToString() + " Koeder, ";
            chefzTxt1 = chefzTxt1 + m_RejectedYields.ToString() + "/" + m_RejectedBaits.ToString() + " abgewiesen, fingerprint=" + m_Fingerprint.ToString() + ".";
            report.AddInfo(chefzTxt1);
        }

        LogIfDebug();
    }

    private void Clear()
    {
        m_Ordered.Clear();
        m_YieldsByHash.Clear();
        m_BaitsByHash.Clear();
        m_BaitOrder.Clear();
        m_Ready          = false;
        m_Fingerprint    = 0;
        m_RejectedYields = 0;
        m_RejectedBaits  = 0;
    }

    /**
     * Die Ertraege in stabiler Reihenfolge uebernehmen.
     *
     * Zwei Durchgaenge statt einer Sortierfunktion: zuerst die vorkommenden
     * loadOrder-Werte aufsteigend, dann je Wert die IDs aufsteigend. Das ist
     * dieselbe Ordnung wie eine stabile Sortierung nach (loadOrder, id), und
     * sie kommt ohne Einfuegen an einer Position aus - eine Operation, deren
     * Verhalten bei typisierten Enforce-Arrays nirgends zugesichert ist.
     */
    private void BuildYields(ChefZ_Registry<ChefZ_FishingYieldDef> yields, ChefZ_LoadReport report)
    {
        if (!yields || yields.Count() == 0)
            return;

        array<string> ids = yields.SortedIds();

        array<int> orders = new array<int>();
        for (int i = 0; i < ids.Count(); i++)
        {
            ChefZ_FishingYieldDef probe = yields.FindByName(ids.Get(i));
            if (!probe)
                continue;
            if (orders.Find(probe.loadOrder) < 0)
                orders.Insert(probe.loadOrder);
        }
        SortIntsAscending(orders);

        for (int o = 0; o < orders.Count(); o++)
        {
            int wanted = orders.Get(o);
            for (int k = 0; k < ids.Count(); k++)
            {
                ChefZ_FishingYieldDef def = yields.FindByName(ids.Get(k));
                if (!def)
                    continue;
                if (def.loadOrder != wanted)
                    continue;

                if (!ClassIsSpawnable(def.id))
                {
                    m_RejectedYields++;
                    if (report)
                        report.AddError(def.sourceRef, def.id, "Zu dieser ID gibt es keine CfgVehicles-Klasse - Eintrag abgewiesen. " + "Die Engine erzeugt den Fang ueber genau diesen Klassennamen und liefert " + "sonst zur Fangzeit still null; der Spieler sieht dann einen Biss ohne " + "Ergebnis.");
                    continue;
                }

                m_Ordered.Insert(def);
                m_YieldsByHash.Set(def.GetTypeHash(), def);
            }
        }
    }

    private void BuildBaits(ChefZ_Registry<ChefZ_BaitDef> baits, ChefZ_LoadReport report)
    {
        if (!baits || baits.Count() == 0)
            return;

        array<string> ids = baits.SortedIds();
        for (int i = 0; i < ids.Count(); i++)
        {
            ChefZ_BaitDef def = baits.FindByName(ids.Get(i));
            if (!def)
                continue;

            if (!ClassIsSpawnable(def.id))
            {
                m_RejectedBaits++;
                if (report)
                    report.AddError(def.sourceRef, def.id, "Zu dieser ID gibt es keine CfgVehicles-Klasse - Koeder abgewiesen. " + "Ein Koeder, den es nicht gibt, kann nie am Haken haengen.");
                continue;
            }

            ChefZ_BaitEntry entry = new ChefZ_BaitEntry();
            entry.id               = def.id;
            entry.typeHash         = def.GetTypeHash();
            entry.kind             = def.GetKind();
            entry.weightMultiplier = def.weightMultiplier;
            entry.lureWearPerUse   = def.lureWearPerUse;

            // Der Datensatz weist eine leere Zielliste bereits beim Laden ab.
            // Die Pruefung steht trotzdem hier: Build() darf auch von einer
            // Stelle gerufen werden, die nicht durch VALIDATE gelaufen ist,
            // und ein Nullzeiger im Startpfad waere ein Serverstart weniger.
            int targetCount = ChefZ_TextList.Count(def.targets);
            for (int t = 0; t < targetCount; t++)
            {
                string target = def.targets.Get(t);
                if (target == "")
                    continue;

                int targetHash = target.Hash();
                if (!m_YieldsByHash.Contains(targetHash))
                {
                    // 21 §3.4: NUR der Eintrag faellt weg, der Koeder bleibt
                    // gueltig. Ein Koedermodul muss Ertraege aus einem Modul
                    // nennen duerfen, das der Betreiber nicht geladen hat.
                    if (report)
                        report.AddWarn(def.sourceRef, def.id, "Das Ziel \"" + target + "\" ist kein bekannter Fangertrag - Eintrag faellt weg, der Koeder " + "bleibt gueltig. Das ist der Normalfall, wenn ein Datenmodul nicht " + "geladen ist.");
                    continue;
                }

                entry.targets.Set(targetHash, 1);
            }

            m_BaitsByHash.Set(entry.typeHash, entry);
            m_BaitOrder.Insert(entry);
        }
    }

    /**
     * 21 §2.4, vorletzte Zeile: ein Ertrag mit lureOnly, den kein Koeder
     * nennt, ist unfangbar.
     *
     * Das ist ausdruecklich nur eine WARNUNG und kein Fehler. Der Koeder kann
     * aus einem Modul kommen, das dieser Betreiber nicht geladen hat - dann
     * ist der Eintrag wirkungslos, und wirkungslos ist genau richtig. Gemeldet
     * wird es trotzdem, weil "kommt nie vor" auf einem laufenden Server exakt
     * aussieht wie "gibt es nicht".
     */
    private void WarnOnUnreachableYields(ChefZ_LoadReport report)
    {
        if (!report)
            return;

        for (int i = 0; i < m_Ordered.Count(); i++)
        {
            ChefZ_FishingYieldDef def = m_Ordered.Get(i);
            if (!def.lureOnly)
                continue;

            if (IsTargetedByAnyBait(def.GetTypeHash()))
                continue;

            report.AddWarn(def.sourceRef, def.id, "Dieser Eintrag traegt \"lureOnly\", wird aber von keinem geladenen Koeder " + "genannt - er kann damit auf diesem Server nicht gefangen werden. Fehlt " + "das Koedermodul, ist das erwartbar; sonst ist es ein Tippfehler in " + "\"targets\".");
        }
    }

    private bool IsTargetedByAnyBait(int yieldTypeHash)
    {
        for (int i = 0; i < m_BaitOrder.Count(); i++)
        {
            if (m_BaitOrder.Get(i).Targets(yieldTypeHash))
                return true;
        }
        return false;
    }

    //! Kleine Ganzzahlliste aufsteigend. Einfachtausch: die Liste enthaelt die
    //! VORKOMMENDEN loadOrder-Werte, also eine Handvoll Eintraege.
    private void SortIntsAscending(notnull array<int> values)
    {
        for (int i = 0; i < values.Count(); i++)
        {
            for (int k = i + 1; k < values.Count(); k++)
            {
                if (values.Get(k) >= values.Get(i))
                    continue;
                int tmp = values.Get(i);
                values.Set(i, values.Get(k));
                values.Set(k, tmp);
            }
        }
    }

    /**
     * Kann die Engine diese Klasse ueberhaupt erzeugen?
     *
     * Zwei Bedingungen, und 21 §2.4 nennt beide: der Knoten muss in
     * CfgVehicles stehen UND scope darf nicht 0 sein. Eine Klasse mit scope 0
     * ist eine Vorlage zum Erben; die Engine erzeugt sie nicht, und ein
     * Fangeintrag darauf lieferte zur Fangzeit still null - der Spieler saehe
     * einen Biss ohne Ergebnis.
     *
     * protected und eine eigene Methode, aus demselben Grund wie
     * ChefZ_ToolRegistry.ResolveConfigParent: sie ist der EINZIGE
     * Config-Zugriff dieser Klasse, und der Selbsttest ersetzt sie durch eine
     * Tabelle. Sonst waere die Abweisungsregel aus 20 §8 nur auf einem
     * laufenden Server mit echtem Inhalt pruefbar - und damit praktisch gar
     * nicht.
     *
     * Die Antwort ist auf Client und Server dieselbe: beide lesen dieselbe
     * gemergte Config. Damit bleibt D1 gewahrt, obwohl hier Eintraege
     * herausfallen koennen.
     */
    protected bool ClassIsSpawnable(string cls)
    {
        if (!ChefZ_VanillaNutrition.ClassExists(cls))
            return false;
        return ChefZ_VanillaNutrition.ScopeOf(cls) != 0;
    }

    //! Nur fuer den Selbsttest: keine Logzeilen waehrend der Pruefung.
    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }

    //==========================================================================
    // Auskuenfte (20 §4.2)
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    int GetYieldCount()
    {
        return m_Ordered.Count();
    }

    int GetBaitCount()
    {
        return m_BaitOrder.Count();
    }

    /**
     * Die Registrierungsliste. DIE Quelle der Determinismus-Invariante.
     *
     * outDefs wird GELEERT und gefuellt, nie null. Der Aufrufer bekommt eine
     * Kopie der Liste, nicht die Liste selbst: wer sie sortierte, broeche D1
     * fuer alle anderen mit.
     */
    int GetOrderedYields(out array<ChefZ_FishingYieldDef> outDefs)
    {
        if (!outDefs)
            outDefs = new array<ChefZ_FishingYieldDef>();
        outDefs.Clear();

        for (int i = 0; i < m_Ordered.Count(); i++)
            outDefs.Insert(m_Ordered.Get(i));

        return outDefs.Count();
    }

    /**
     * Der heisse Pfad: Faktor auf das Grundgewicht eines Ertrags.
     *
     * 1.0 heisst "kein Treffer" und ist die Vanilla-Antwort - fuer einen
     * unbekannten Koeder, fuer kein Koeder (Hash 0), fuer einen Koeder, der
     * diesen Ertrag nicht nennt, und fuer eine Registry, die nicht steht.
     *
     * Kein Log. Diese Methode laeuft je Ertrag einmal je Angelaktion.
     */
    float GetWeightMultiplier(int baitTypeHash, int yieldTypeHash)
    {
        if (!m_Ready)
            return 1.0;
        if (baitTypeHash == 0)
            return 1.0;

        ChefZ_BaitEntry entry;
        if (!m_BaitsByHash.Find(baitTypeHash, entry))
            return 1.0;
        if (!entry.Targets(yieldTypeHash))
            return 1.0;

        return entry.weightMultiplier;
    }

    /**
     * Ist das ein Kunstkoeder?
     *
     * Die Frage entscheidet in der gemoddeten RemoveItemSafe darueber, ob ein
     * Gegenstand geloescht wird oder nur Schaden nimmt. Steht die Registry
     * nicht, ist die Antwort definitionsgemaess false - und dann loescht
     * Vanilla wie immer (20 §4.4).
     */
    bool IsLure(int baitTypeHash)
    {
        if (!m_Ready)
            return false;
        if (baitTypeHash == 0)
            return false;

        ChefZ_BaitEntry entry;
        if (!m_BaitsByHash.Find(baitTypeHash, entry))
            return false;

        return entry.kind == ChefZ_BaitKind.LURE;
    }

    //! Healthabzug je Zyklus. 0.0, wenn der Koeder unbekannt ist - dann
    //! passiert nichts, statt einen geratenen Wert abzuziehen.
    float GetLureWear(int baitTypeHash)
    {
        ChefZ_BaitEntry entry;
        if (!m_Ready || !m_BaitsByHash.Find(baitTypeHash, entry))
            return 0.0;
        return entry.lureWearPerUse;
    }

    bool IsKnownBait(int baitTypeHash)
    {
        if (!m_Ready)
            return false;
        return m_BaitsByHash.Contains(baitTypeHash);
    }

    //! Kennt die Tabelle diesen Ertrag? Fuer den Registrar und die Diagnose.
    bool IsKnownYield(int yieldTypeHash)
    {
        if (!m_Ready)
            return false;
        return m_YieldsByHash.Contains(yieldTypeHash);
    }

    //==========================================================================
    // Fingerabdruck (20 §6, Sonde T7)
    //==========================================================================

    /**
     * Eine Zahl ueber die geordnete Ertragsliste und die Koedertabelle.
     *
     * Sie beantwortet genau eine Frage, und ein Mensch stellt sie: haben
     * Client und Server dieselben Daten? Beide schreiben sie beim Start ins
     * Log; sind sie verschieden, laufen die Gewichtsfelder auseinander, und
     * der Spieler sieht den Biss zu einem anderen Zeitpunkt, als der Server
     * ihn wertet.
     *
     * Eine automatische Abgleichsroutine waere Netzcode gegen ein Problem, das
     * ungleiche PBOs voraussetzt - und die lehnt DayZ bereits selbst ab.
     *
     * Gerechnet wird ausschliesslich ueber Werte, die auf beiden Seiten
     * gleich sind: KEIN sourceRef, KEIN sourceRank, keine Dateipfade.
     */
    int GetContentFingerprint()
    {
        return m_Fingerprint;
    }

    private int ComputeFingerprint()
    {
        int h = 17;

        for (int i = 0; i < m_Ordered.Count(); i++)
        {
            ChefZ_FishingYieldDef def = m_Ordered.Get(i);
            h = Mix(h, def.GetTypeHash());
            h = Mix(h, def.baseWeight);
            h = Mix(h, def.GetEnviroMask());
            h = Mix(h, def.GetMethodMask());
            h = Mix(h, BoolBit(def.lureOnly));
            h = Mix(h, BoolBit(def.overrideExisting));

            // Zwischenvariable und kein Ausdruck im Argument: Math.Round
            // liefert float, Mix nimmt int. Vanilla schreibt die Umwandlung
            // genauso als eigene Zuweisung (EntityAI.c:3258).
            int quality1000 = Math.Round(def.quality * 1000.0);
            h = Mix(h, quality1000);
        }

        for (int b = 0; b < m_BaitOrder.Count(); b++)
        {
            ChefZ_BaitEntry entry = m_BaitOrder.Get(b);
            h = Mix(h, entry.typeHash);
            h = Mix(h, entry.kind);
            int mult1000 = Math.Round(entry.weightMultiplier * 1000.0);
            int wear1000 = Math.Round(entry.lureWearPerUse * 1000.0);
            h = Mix(h, mult1000);
            h = Mix(h, wear1000);

            // Ueber die geordnete ERTRAGSLISTE und nicht ueber die Map des
            // Eintrags: eine Map hat keine zugesicherte Reihenfolge, und
            // genau darauf darf ein Fingerabdruck nicht bauen.
            for (int y = 0; y < m_Ordered.Count(); y++)
            {
                if (!entry.Targets(m_Ordered.Get(y).GetTypeHash()))
                    continue;
                h = Mix(h, m_Ordered.Get(y).GetTypeHash());
            }
        }

        return h;
    }

    private int Mix(int acc, int value)
    {
        return acc * 31 + value;
    }

    private int BoolBit(bool value)
    {
        if (value)
            return 1;
        return 0;
    }

    //==========================================================================
    // Diagnose (18, 20 §3)
    //==========================================================================

    void Dump(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        string chefzTxt1 = "ChefZ Fangtabelle  bereit=" + m_Ready.ToString() + "  ertraege=" + m_Ordered.Count().ToString() + "  koeder=";
        chefzTxt1 = chefzTxt1 + m_BaitOrder.Count().ToString() + "  fingerprint=" + m_Fingerprint.ToString();
        outLines.Insert(chefzTxt1);

        for (int i = 0; i < m_Ordered.Count(); i++)
        {
            ChefZ_FishingYieldDef def = m_Ordered.Get(i);
            string line = "  [" + i.ToString() + "] " + def.id + "  gewicht=" + def.baseWeight.ToString() + "  enviro=" + def.enviro;
            line = line + "  methoden=" + ChefZ_TextList.Join(def.methods, "|") + "  lureOnly=" + def.lureOnly.ToString() + "  ersetzt=" + def.overrideExisting.ToString();
            outLines.Insert(line);
        }

        for (int b = 0; b < m_BaitOrder.Count(); b++)
        {
            ChefZ_BaitEntry entry = m_BaitOrder.Get(b);
            string bline = "  <" + entry.id + ">  art=" + ChefZ_BaitKind.Name(entry.kind) + "  faktor=" + entry.weightMultiplier.ToString();
            bline = bline + "  verschleiss=" + entry.lureWearPerUse.ToString() + "  ziele=" + entry.targets.Count().ToString();
            outLines.Insert(bline);
        }
    }

    private void LogIfDebug()
    {
        if (m_QuietForTest)
            return;
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.FISHING, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        Dump(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.FISHING, lines);
    }
}
