//==============================================================================
// ChefZ_FishingYieldItem - ein Fangertrag aus Daten
//
// Entwurf: 20 §4.3 (Schnittstelle woertlich), 20 §6 D2 (was in das Gewicht
// einfliessen darf und was nicht), 20 §8 (Fehlerverhalten), 21 §2.2
// (Abbildung der Felder auf die Engine).
//
// ---------------------------------------------------------------------------
// Die EINE Stelle mit Fachlogik
// ---------------------------------------------------------------------------
// GetYieldWeight. Alles andere in dieser Datei traegt Daten von einem
// Datensatz in die Felder, die die Engine ohnehin liest.
//
// Die Regel, in Worten:
//
//     kein Koeder            -> Grundgewicht
//     fremder Koeder         -> Grundgewicht
//     Zielkoeder             -> Grundgewicht mal Faktor
//     lureOnly ohne Treffer  -> 0, und damit gar kein Eintrag im Feld
//
// Gewicht 0 ist kein Sonderfall, sondern die Abwesenheit: die Engine legt je
// Gewichtspunkt einen Eintrag im Wahrscheinlichkeitsfeld an
// (CatchingContextBase.SetupProbabilityArray), 0 Punkte heissen 0 Eintraege.
//
// ---------------------------------------------------------------------------
// Determinismus (20 §6 D2) - die Liste dessen, was hier NICHT vorkommen darf
// ---------------------------------------------------------------------------
// Client und Server bauen das Gewichtsfeld je fuer sich und ziehen daraus
// denselben Index aus demselben synchronisierten Zufallsstrom. In die
// Gewichtsrechnung duerfen deshalb ausschliesslich Daten aus Rang 1 und 2 und
// die KLASSE des Koeders eingehen.
//
// Verboten und in dieser Datei nachweislich abwesend: Item-Health, Quantity,
// Spielerzustand, Uhrzeit, Zufall. Die Klasse des Koeders ist zulaessig, weil
// Vanilla selbst Signaldauer und -startzeit aus der Koederklasse ableitet und
// sie ueber denselben synchronisierten Strom zieht - unsere Abhaengigkeit ist
// nicht staerker als die, die ohnehin besteht.
//
// ---------------------------------------------------------------------------
// Warum die Basisklasse genau diese ist
// ---------------------------------------------------------------------------
// Die Ergebnisstruktur der Angelaktion castet ihren Ertrag hart auf diese
// Basis, um die Zykluszeit zu holen (CarchingResultFishingAction.c:13-16).
// Ein Ertrag, der nur von YieldItemBase erbt, liefert dort null - und der
// Server faellt beim ersten Biss mit einem Nullzeiger aus. Die Basis ist
// deshalb keine Bequemlichkeit, sondern Bedingung.
//
// Layer: 4_World - dieselbe Schicht, in der die Engine ihre eigenen Ertraege
// fuehrt.
//==============================================================================

class ChefZ_FishingYieldItem extends FishYieldItemBase
{
    //! Obergrenze des Produkts baseWeight x weightMultiplier (S-04).
    static const float CHEFZ_WEIGHT_PRODUCT_MAX = 10000.0;

    /**
     * Der Datensatz, aus dem dieser Ertrag entstanden ist.
     *
     * OHNE ref: Eigentuemer ist die Registry im Config Manager, und die lebt
     * laenger als die Fangtabelle einer Mission. Ein zweiter Besitzer waere
     * ein Zyklus ohne Gewinn.
     */
    protected ChefZ_FishingYieldDef m_Def;

    protected int  m_TypeHash;
    protected bool m_LureOnly;

    //! Aufgeloeste Koederkategorien fuer Reusen. null = Vanilla-Verhalten.
    protected ref array<int> m_SensitiveCategories;

    /**
     * Die Basis ruft Init() aus IHREM Konstruktor - also BEVOR die Zeilen
     * unten laufen. Deshalb liest Init() nichts aus m_Def, und deshalb wird
     * m_Def hier gesetzt und nicht dort. Dasselbe Muster benutzt die Engine
     * selbst (YieldItemGenericFish).
     *
     * Der erste Parameter wandert unveraendert in den Basiskonstruktor und
     * wird dort zu m_BaseWeight.
     */
    void ChefZ_FishingYieldItem(int baseWeight, ChefZ_FishingYieldDef def)
    {
        m_Def      = def;
        m_TypeHash = 0;
        m_LureOnly = false;

        if (!def)
        {
            // Kann im Betrieb nicht vorkommen - der Registrar baut nur aus
            // einem Datensatz. Der Zweig steht trotzdem da, damit ein
            // Programmierfehler kein Nullzeiger im Gewichtspfad wird.
            ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.FISHING, "fishing.yielditem.nodef", "Ein Fangertrag wurde ohne Datensatz erzeugt. Er bleibt ohne Typ und wird " + "von der Engine nie ausgewaehlt.");
            return;
        }

        m_Type       = def.id;
        m_EnviroMask = def.GetEnviroMask();
        m_MethodMask = def.GetMethodMask();
        m_TypeHash   = def.GetTypeHash();
        m_LureOnly   = def.lureOnly;

        // 21 §2.1: ohne Angabe bleibt die Vorgabe der Engine stehen, die
        // Init() bereits gesetzt hat. Die Zahl steht deshalb nirgends im Core.
        if (def.HasQuality())
            m_QualityBase = def.quality;

        ChefZ_ApplyHourlyCoefs(def.hourlyCoefs);
        ChefZ_ResolveSensitivities(def.baitSensitivityAllow);
    }

    //==========================================================================
    // Gewicht (20 §4.3)
    //==========================================================================

    override int GetYieldWeight(CatchingContextBase ctx)
    {
        int baitTypeHash = ChefZ_ResolveBaitHash(ctx);
        return ChefZ_ComputeWeight(baitTypeHash);
    }

    /**
     * Die Bruecke vom Kontext zum Koeder.
     *
     * 0 heisst "kein Koeder" und ist zugleich die Antwort fuer jeden Kontext,
     * der keine Rute ist - eine Reuse hat keinen Rutenkoeder, und dort gilt
     * das Grundgewicht (20 §8).
     *
     * protected und eigene Methode, damit der Selbsttest sie ersetzen kann:
     * einen echten Rutenkontext gibt es ohne Welt, ohne Spieler und ohne Rute
     * nicht, die Gewichtsregel darunter dagegen ist reine Rechnung.
     */
    protected int ChefZ_ResolveBaitHash(CatchingContextBase ctx)
    {
        CatchingContextFishingRodAction rodCtx = CatchingContextFishingRodAction.Cast(ctx);
        if (!rodCtx)
            return 0;

        EntityAI bait = rodCtx.ChefZ_GetBaitEntity();
        if (!bait)
            return 0;

        string baitType = bait.GetType();
        return baitType.Hash();
    }

    /**
     * Die Regel selbst. Kein Kontext, keine Entitaet, keine Uhr.
     *
     * @param registry  nur fuer den Selbsttest. Ohne Angabe die Registry des
     *        Prozesses - der Betriebsfall. Ein Test, der stattdessen den
     *        Singleton umbaut, haette den Boot verschoben, der wenige Zeilen
     *        spaeter darauf zugreift.
     */
    int ChefZ_ComputeWeight(int baitTypeHash, ChefZ_FishingRegistry registry = null)
    {
        ChefZ_FishingRegistry reg = registry;
        if (!reg)
            reg = ChefZ_FishingRegistry.Get();

        float mult = reg.GetWeightMultiplier(baitTypeHash, m_TypeHash);

        // Kein Treffer heisst Faktor 1.0. Bei lureOnly ist das die Aussage
        // "dieser Koeder ist nicht der richtige" - und damit kein Eintrag.
        if (m_LureOnly && mult <= 1.0)
            return 0;

        float raw = m_BaseWeight * mult;

        // Deckel auf dem PRODUKT (S-04, Gate fishing): baseWeight und Multiplikator
        // sind einzeln geklemmt, ihr Produkt war es nicht. Jeder Punkt ist ein
        // Insert in m_ProbabilityArray je Auswurf - 10000 ist das Maximum,
        // das ein einzelner Datensatz ohnehin haben darf.
        if (raw > CHEFZ_WEIGHT_PRODUCT_MAX)
            raw = CHEFZ_WEIGHT_PRODUCT_MAX;

        // Math.Round und kein stiller Abschnitt: die Zahl entsteht auf beiden
        // Seiten derselben Buildkette zwar gleich, aber eine unnoetige Annahme
        // gehoert nicht in genau den Pfad, der beidseitig gleich sein muss.
        int rounded = Math.Round(raw);
        if (rounded < 0)
            return 0;
        return rounded;
    }

    //==========================================================================
    // Koederempfindlichkeit - ausschliesslich fuer Reusen
    //==========================================================================

    /**
     * Ohne Angabe exakt das Verhalten der Basis.
     *
     * Fuer die Rute ist diese Methode folgenlos: die Koedertabelle des
     * Rutenkontexts bleibt leer, und bei leerer Tabelle gilt jeder Ertrag als
     * vertraeglich (CatchingContextBase.CheckBaitCompatibility). Sie steht
     * hier fuer die Reusen, die dieselbe Fangtabelle lesen.
     */
    override float GetBaitTypeSensitivity(ECatchingBaitCategories type)
    {
        if (!m_SensitiveCategories)
            return super.GetBaitTypeSensitivity(type);

        int code = type;
        if (m_SensitiveCategories.Find(code) >= 0)
            return 1.0;
        return 0.0;
    }

    /**
     * Namen auf Kategoriewerte abbilden - ueber die Engine, nicht ueber eine
     * Liste im Core.
     *
     * Der Core fuehrt die Namen der Kategorien BEWUSST nicht selbst: sie
     * gehoeren der Engine, sie koennen sich mit einem Spielupdate aendern, und
     * eine zweite Liste waere sofort eine zweite Wahrheit. EnumTools liest sie
     * zur Laufzeit aus dem Typ - dieselbe Antwort, ohne Pflegeaufwand.
     */
    protected void ChefZ_ResolveSensitivities(array<string> names)
    {
        if (!names || names.Count() == 0)
            return;

        array<int> resolved = new array<int>();
        int size = EnumTools.GetEnumSize(ECatchingBaitCategories);

        for (int i = 0; i < names.Count(); i++)
        {
            string wanted = names.Get(i);
            if (wanted == "")
                continue;

            bool found = false;
            for (int k = 0; k < size; k++)
            {
                int value = EnumTools.GetEnumValue(ECatchingBaitCategories, k);
                if (EnumTools.EnumToString(ECatchingBaitCategories, value) != wanted)
                    continue;

                if (resolved.Find(value) < 0)
                    resolved.Insert(value);
                found = true;
                break;
            }

            if (!found)
            {
                // 21 §2.4: WARN je Eintrag, der Datensatz bleibt gueltig. Ueber
                // Once, weil derselbe Tippfehler sonst je Missionsstart erneut
                // im Protokoll stuende.
                ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.FISHING, "fishing.sensitivity." + m_Type + "." + wanted, "Der Eintrag \"" + wanted + "\" in \"baitSensitivityAllow\" von \"" + m_Type + "\" ist keine bekannte Koederkategorie der Engine - Eintrag " + "ignoriert. Er wirkt ohnehin nur fuer Reusen.");
            }
        }

        if (resolved.Count() > 0)
            m_SensitiveCategories = resolved;
    }

    /**
     * Tageskurve uebernehmen.
     *
     * Nur bei exakt passender Laenge. Eine kuerzere Liste waere schlimmer als
     * keine: die Engine liest mit der aktuellen Stunde in ein Feld fester
     * Breite, und ein nicht beschriebener Platz enthielte den Wert der
     * Vorgabe - also eine Mischung aus zwei Kurven. Der Datensatz weist eine
     * falsche Laenge bereits beim Laden ab; diese Pruefung ist die Nachhut.
     */
    protected void ChefZ_ApplyHourlyCoefs(array<float> coefs)
    {
        if (!coefs)
            return;
        if (coefs.Count() != ChefZ_FishingLimits.HOURLY_COEF_COUNT)
            return;

        for (int i = 0; i < ChefZ_FishingLimits.HOURLY_COEF_COUNT; i++)
            m_HourlyCycleLengthCoefs[i] = coefs.Get(i);
    }

    //==========================================================================
    // Auskuenfte fuer Registrar und Diagnose
    //==========================================================================

    int ChefZ_GetTypeHash()
    {
        return m_TypeHash;
    }

    bool ChefZ_IsLureOnly()
    {
        return m_LureOnly;
    }

    ChefZ_FishingYieldDef ChefZ_GetDef()
    {
        return m_Def;
    }
}
