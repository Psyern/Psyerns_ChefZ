//==============================================================================
// ChefZ_PreservationManager - Haltbarkeitsregeln und Restfrische zur Laufzeit
//
// Entwurf: 14 §5 (Schnittstelle woertlich), 14 §3 (die Produktkette des
// Multiplikators), 14 §4 ("Frische bestraft, Vanilla toetet"), 14 §6
// (Datenfluss), 14 §7 (Zustand: der Multiplikator wird NICHT persistiert),
// 14 §8 (Fehlerverhalten, Zeile fuer Zeile), 14 E1/E3/E4/E5/E7.
//
// ---------------------------------------------------------------------------
// Was dieser Manager NICHT tut
// ---------------------------------------------------------------------------
// Er fasst kein Item an und er kennt keinen Engine-Typ. ComputeDecayScale ist
// rein rechnend und bekommt seine Eingaben als Daten: Zustand, Qualitaet,
// Klasse, Kategorie-Closure, Tags, Behaelterfaktor und Umgebungstemperatur.
// Derselbe Schnitt wie beim Matcher (05 E1) und aus demselben Grund - er ist
// damit ohne laufendes Spiel pruefbar, und er KANN am Item nichts veraendern.
//
// Wer den Faktor anwendet, ist ChefZ_ItemDecay (4_World). Wer verdirbt, ist
// Vanilla.
//
// ---------------------------------------------------------------------------
// Warum es keinen Cache gibt (14 E4)
// ---------------------------------------------------------------------------
// Der Multiplikator wird bei JEDEM Aufruf neu gerechnet und nirgends
// gespeichert. Das ist Absicht und der Kern von 14 §7:
//
//   "Wuerde man ihn speichern, wirkte eine Balancing-Aenderung des Admins nie
//    auf bestehende Items. So wirkt sie sofort, ohne Migration."
//
// Der Preis ist eine Handvoll Map-Zugriffe je Item und Verfallstick - und ein
// Verfallstick laeuft je Item nur im Minutentakt. Entwurf A wollte hier einen
// Cache mit Invalidierungsstempel; 14 E4 hat ihn verworfen, weil er mehr
// Zustand, mehr Fehlerquellen und keinen messbaren Gewinn bringt.
//
// ---------------------------------------------------------------------------
// Die Regel, die Doppeldeutigkeit verhindert (14 §4)
// ---------------------------------------------------------------------------
//   "Frische bestraft, Vanilla toetet."
//
// Freshness01 faellt monoton und ist bei 0 ANGEKOMMEN, nicht verdorben. Den
// Zustand ROTTEN vergibt ausschliesslich der Vanilla-Verfall. Deshalb gibt es
// in dieser Klasse keine Methode, die aus einer Frische von 0 irgendetwas
// ableitet - es gibt nichts abzuleiten. Wer Frische toedlich machen will, baut
// das im Content-Modul als Transform mit Frischebedingung.
//
// KEIN CONTENT: dieser Manager definiert keine einzige Regel. Alles kommt aus
// der Registry.
//
// Layer: 3_Game. Er liest Registries und kennt keinen Engine-Typ - kein
// ItemBase, kein FoodStage, kein Enum aus 4_World.
//==============================================================================

class ChefZ_PreservationManager
{
    private static ref ChefZ_PreservationManager s_Instance;

    //! Neutraler Faktor. Steht als Konstante, weil ihn drei Rueckfallpfade
    //! liefern und "1.0" an drei Stellen niemandem sagt, dass es derselbe
    //! Gedanke ist: keine Regel getroffen, also exakt Vanilla.
    static const float NEUTRAL = 1.0;

    //--- Bestand, getrennt nach Dimension (14 §6, BOOT) ----------------------
    // KEIN ref auf den Wert: Eigentuemer ist die Registry, genau wie beim
    // ChefZ_StateManager und beim ChefZ_QualityManager.
    private ref map<int, ChefZ_PreservationDef> m_ByState;
    private ref map<int, ChefZ_PreservationDef> m_ByClass;
    private ref map<int, ChefZ_PreservationDef> m_ByTag;
    private ref map<int, ChefZ_PreservationDef> m_ByQuality;

    /**
     * Kategorieregeln als parallele Listen statt als Map.
     *
     * Grund: eine Kategorieregel trifft ueber die VORFAHREN-Closure des Items
     * (04 E1), nicht ueber Gleichheit. Der Test ist ein Bitzugriff mit dem
     * Bitindex der Kategorie - und den holt man einmal beim Boot, nicht je
     * Verfallstick. Die Liste ist kurz: 14 E3 nennt Kategorien ausdruecklich
     * "Feinschliff fuer Ausnahmen", die primaere Dimension ist der Zustand.
     */
    private ref array<ChefZ_PreservationDef> m_CategoryDefs;
    private ref array<int>                   m_CategoryBits;

    //! Stabile Folge fuer Dump und Diagnose.
    private ref array<ChefZ_Sym> m_Order;

    //--- Servergrenzen aus Core.json (14 §3) ---------------------------------
    private float m_GlobalScale;
    private float m_MinScale;
    private float m_MaxScale;
    private float m_DefaultFreshnessLifetimeSec;

    private bool m_Ready;
    private bool m_NotReadyLogged;
    private bool m_QuietForTest;
    private int  m_RejectedCount;

    /**
     * Wie viele Regeln ueberhaupt einen der beiden Schalter setzen.
     *
     * Sie sind der Grund, warum StopsDecay() im Verfallstakt nichts kostet,
     * solange niemand Konserven gebaut hat: die Itemseite fragt zuerst diese
     * Zahl und sammelt die Fakten des Items erst danach. Ohne sie liefe fuer
     * JEDES Lebensmittel bei JEDEM Update-Tick eine vollstaendige
     * Faktenerhebung, nur um "nein" zu sagen.
     *
     * Auf einem Server ohne Konservenregel ist das der Normalfall - und der
     * Normalfall darf nicht der teure sein.
     */
    private int m_StopsDecayCount;
    private int m_PreventsRottenCount;

    //! Manager des Tests statt der Singletons - dieselbe Loesung wie im
    //! ChefZ_StateManager und im ChefZ_QualityManager und aus demselben Grund:
    //! ohne sie muesste der Selbsttest die Singletons umbauen und damit den
    //! echten Bestand des Servers anfassen.
    private ChefZ_CategoryManager m_CategoriesForTest;
    private ChefZ_StateManager    m_StatesForTest;
    private ChefZ_QualityManager  m_QualityForTest;

    //--------------------------------------------------------------------------

    void ChefZ_PreservationManager()
    {
        m_ByState      = new map<int, ChefZ_PreservationDef>();
        m_ByClass      = new map<int, ChefZ_PreservationDef>();
        m_ByTag        = new map<int, ChefZ_PreservationDef>();
        m_ByQuality    = new map<int, ChefZ_PreservationDef>();
        m_CategoryDefs = new array<ChefZ_PreservationDef>();
        m_CategoryBits = new array<int>();
        m_Order        = new array<ChefZ_Sym>();
        m_QuietForTest = false;

        ResetState();
    }

    static ChefZ_PreservationManager Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_PreservationManager();
        return s_Instance;
    }

    //==========================================================================
    // Aufbau (14 §6, BOOT)
    //==========================================================================

    /**
     * Baut den Bestand. Einmal beim Boot, danach unveraenderlich.
     *
     * Abweichung von der Signatur in 14 §5: die CoreSettings kommen als
     * dritter Parameter dazu. Sie muessen es, denn 14 §3 nennt
     * globalSpoilageScale, minDecayScale und maxDecayScale ausdruecklich als
     * Bestandteile der Rechnung - sie hier ein zweites Mal aus dem Config
     * Manager zu holen hiesse, dieselbe Groesse an zwei Orten zu lesen, und
     * machte den Manager im Selbsttest unbenutzbar.
     *
     * settings darf null sein: dann gelten die neutralen Werte
     * (Skala 1.0, Grenzen 0.01 .. 10.0) und der Verfall ist bitgenau Vanilla.
     *
     * Der Aufruf ist beim Boot UNBEDINGT: auch ohne eine einzige Regel soll
     * der Manager "bereit und leer" sein (14 §8, erste Zeile). Sonst
     * antwortete er auf jede Abfrage mit dem Fehler "vor Build aufgerufen",
     * obwohl schlicht keine Haltbarkeitsregeln konfiguriert sind.
     */
    void Build(ChefZ_Registry<ChefZ_PreservationDef> defs,
               ChefZ_LoadReport report,
               ChefZ_CoreSettingsDef settings = null)
    {
        ResetState();
        ApplySettings(settings, report);

        if (!defs || defs.Count() == 0)
        {
            // 14 §8, erste Zeile: kein Abbruch, kein Fehler. ComputeDecayScale
            // liefert die globale Skala (Vorgabe 1.0), ProcessDecay reicht
            // delta * 1.0 durch, und der Verfall ist BITGENAU VANILLA.
            //
            // Die Meldung ist ein INFO und kein WARN: ein Server ohne
            // Content-Modul hat berechtigterweise keine Haltbarkeitsregeln,
            // und ein WARN bei jedem Start waere eine Warnung ohne Fehler.
            if (report)
                report.AddInfo("Keine Haltbarkeitsregeln definiert - der Verfall von "
                    + "ChefZ-Nahrung entspricht Vanilla, skaliert nur mit globalSpoilageScale ("
                    + m_GlobalScale.ToString() + "). Vanilla-Nahrung ist ohnehin unberuehrt.");
            m_Ready = true;
            return;
        }

        // Reihenfolge ist Registry.Keys(), also nach ID sortiert (03 §4).
        // Damit ist jede abgeleitete Groesse auf Client und Server gleich -
        // hier zwar nur fuer den Dump relevant, aber die Regel gilt ohne
        // Ausnahme, sonst faellt sie irgendwann bei jemandem um.
        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_Sym sym = keys.Get(i);
            ChefZ_PreservationDef def = defs.Find(sym);
            if (!def)
                continue;

            if (Register(def, report))
            {
                m_Order.Insert(sym);
                if (def.stopsDecay)
                    m_StopsDecayCount++;
                if (def.preventsRotten)
                    m_PreventsRottenCount++;
            }
            else
            {
                m_RejectedCount++;
            }
        }

        m_Ready = true;

        if (report)
            report.AddInfo("Haltbarkeitsregeln: " + GetRuleCount().ToString() + " geladen"
                + " (Zustand " + m_ByState.Count().ToString()
                + ", Klasse " + m_ByClass.Count().ToString()
                + ", Kategorie " + m_CategoryDefs.Count().ToString()
                + ", Tag " + m_ByTag.Count().ToString()
                + ", Qualitaet " + m_ByQuality.Count().ToString() + ")"
                + ", Skala " + m_GlobalScale.ToString()
                + ", Grenzen [" + m_MinScale.ToString() + ".." + m_MaxScale.ToString() + "]"
                + ", Frischevorgabe " + m_DefaultFreshnessLifetimeSec.ToString() + "s.");

        LogIfDebug();
    }

    /**
     * Die Servergrenzen aus Core.json (14 §3).
     *
     * Sie werden hier nur GELESEN, nicht geklammert - das hat
     * ChefZ_CoreSettingsDef.ClampAndReport bereits getan, und eine zweite
     * Klammerung an dieser Stelle waere eine zweite Wahrheit ueber denselben
     * Wert. Ausnahme ist der Fall settings == null (Selbsttest, SAFE_MODE):
     * dort gibt es keinen geklammerten Wert, also stehen hier die Vorgaben.
     */
    private void ApplySettings(ChefZ_CoreSettingsDef settings, ChefZ_LoadReport report)
    {
        if (!settings)
        {
            m_GlobalScale                 = NEUTRAL;
            m_MinScale                    = 0.01;
            m_MaxScale                    = 10.0;
            m_DefaultFreshnessLifetimeSec = 21600.0;
            return;
        }

        m_GlobalScale                 = settings.globalSpoilageScale;
        m_MinScale                    = settings.minDecayScale;
        m_MaxScale                    = settings.maxDecayScale;
        m_DefaultFreshnessLifetimeSec = settings.defaultFreshnessLifetimeSec;

        if (m_MinScale <= 0.0)
            m_MinScale = 0.01;
        if (m_MaxScale < m_MinScale)
            m_MaxScale = m_MinScale;

        if (m_GlobalScale <= 0.0 && report)
        {
            // 14 §8 nennt fuer einen globalen Verfallsmodifikator von 0
            // ausdruecklich "nichts zu tun - Vanilla schaltet Verfall global
            // ab". Das gilt fuer Vanillas GetFoodDecayModifier. Fuer die
            // ChefZ-EIGENE Skala ist eine Null aber kein Schalter, sondern ein
            // Faktor, und ein Faktor 0 hiesse "verdirbt nie" - genau der
            // Betreiberfehler, den 14 §8 fuer spoilageMultiplier abfaengt.
            // Also dieselbe Antwort: klemmen und melden.
            report.AddWarn("Core.json", ChefZ_CoreSettingsDef.PRIMARY_ID,
                "globalSpoilageScale ist " + m_GlobalScale.ToString() + " und damit nicht "
                + "positiv. Das hiesse \"ChefZ-Nahrung verdirbt nie\". Es gilt "
                + m_MinScale.ToString() + ". Wer den Verfall SERVERWEIT abschalten will, "
                + "benutzt die dafuer vorgesehene Vanilla-Stellschraube und nicht diesen "
                + "Faktor (14 E2).");
        }
        if (m_GlobalScale <= 0.0)
            m_GlobalScale = m_MinScale;

        if (m_DefaultFreshnessLifetimeSec <= 0.0 && report)
        {
            // 14 §8: "freshnessLifetimeSec <= 0 -> Frischefortschreibung
            // deaktiviert (Frische eingefroren), WARN." Auf Serverebene ist
            // das eine zulaessige, aber weitreichende Aussage: die Frische
            // JEDES Items bleibt dann stehen, und damit auch die Frischeregel
            // des Quality Managers (12 §4.1). Deshalb steht sie im Bericht.
            report.AddWarn("Core.json", ChefZ_CoreSettingsDef.PRIMARY_ID,
                "defaultFreshnessLifetimeSec ist " + m_DefaultFreshnessLifetimeSec.ToString()
                + ". Die Restfrische wird damit fuer jeden Zustand ohne eigene Lebensdauer "
                + "EINGEFROREN - sie faellt nie, und Gerichte aus altem Fleisch bekommen "
                + "dieselbe Qualitaet wie Gerichte aus frischem (12 §4.1). Der Verfall selbst "
                + "ist davon unberuehrt.");
        }
    }

    /**
     * Einen Datensatz in seine Dimension einsortieren - und sein ZIEL pruefen.
     *
     * 14 §8: "Record nennt unbekannten Zustand/Kategorie/Tag -> Record
     * abgewiesen, ERROR beim Build. Restliche Records wirken."
     *
     * Warum hier abgewiesen und nicht schon in Validate(): die Zielregistries
     * leben in 3_Game und existieren erst, wenn der Config Manager sie gebaut
     * hat. Ein Record kennt sie nicht - dieselbe Aufteilung wie bei
     * ChefZ_StateDef.implies und ChefZ_QualityTierDef.grantsTags.
     *
     * @return false, wenn der Record abgewiesen wurde.
     */
    private bool Register(notnull ChefZ_PreservationDef def, ChefZ_LoadReport report)
    {
        if (!ChefZ_SymbolTable.IsValid(def.sym))
        {
            Report(report, true, def, "Der Record hat kein gueltiges Symbol - er wurde nie "
                + "kompiliert. Das ist ein Fehler im Ladeweg, nicht in den Daten.");
            return false;
        }

        switch (def.scopeKind)
        {
            case ChefZ_PreservationScope.STATE:    return RegisterState(def, report);
            case ChefZ_PreservationScope.CLASS:    return RegisterClass(def, report);
            case ChefZ_PreservationScope.CATEGORY: return RegisterCategory(def, report);
            case ChefZ_PreservationScope.TAG:      return RegisterTag(def, report);
            case ChefZ_PreservationScope.QUALITY:  return RegisterQuality(def, report);
        }

        // Unerreichbar: ChefZ_PreservationDef.Validate weist einen unbekannten
        // scope bereits ab. Die Zeile steht trotzdem, weil ein spaeter
        // ergaenzter scope sonst still in gar keine Tabelle fiele.
        Report(report, true, def, "scope \"" + def.scope + "\" ist dem Preservation Manager "
            + "unbekannt - der Record wirkt nicht. Gueltig: "
            + ChefZ_PreservationScope.ValidNames() + ".");
        return false;
    }

    private bool RegisterState(notnull ChefZ_PreservationDef def, ChefZ_LoadReport report)
    {
        ChefZ_StateManager states = States();
        if (states && states.IsReady() && !states.Exists(def.sym))
        {
            Report(report, true, def, "scope \"state\", aber \"" + def.id + "\" ist kein "
                + "geladener ChefZ-Zustand. Der Record wird abgewiesen; alle uebrigen "
                + "Haltbarkeitsregeln wirken weiter. Ursache ist fast immer ein Tippfehler "
                + "oder ein Content-Modul, das nicht geladen wurde.");
            return false;
        }

        m_ByState.Set(def.sym, def);
        return true;
    }

    /**
     * scope "class": das Ziel ist ein KLASSENNAME und wird bewusst NICHT
     * gegen eine Registry geprueft.
     *
     * 05 E3 haelt ausdruecklich fest, dass nicht deklarierte Klassen
     * adressierbar bleiben - eine Regel darf eine Klasse nennen, die kein
     * Ingredient-Binding hat. Was der Betreiber dabei wissen muss, steht in
     * 14 E2 und wird deshalb gemeldet statt verschwiegen: die Regel wirkt nur
     * auf ChefZ-eigene Traegerklassen. Auf einer reinen Vanilla-Klasse laeuft
     * sie ins Leere, weil dort ChefZ_Edible_Base.ProcessDecay gar nicht
     * existiert.
     */
    private bool RegisterClass(notnull ChefZ_PreservationDef def, ChefZ_LoadReport report)
    {
        m_ByClass.Set(def.sym, def);

        if (!m_QuietForTest && report)
        {
            ChefZ_IngredientInfo info = ChefZ_IngredientManager.Get().Resolve(def.sym);
            if (!info)
            {
                report.AddWarn(def.sourceRef, def.id,
                    "scope \"class\": \"" + def.id + "\" ist keine deklarierte ChefZ-Zutat. "
                    + "Die Regel bleibt geladen, wirkt aber NUR, wenn die Klasse von "
                    + "ChefZ_Edible_Base ableitet - auf reiner Vanilla-Nahrung greift ChefZ "
                    + "nicht in den Verfall ein (14 E2). Wer die Haltbarkeit einer "
                    + "Vanilla-Klasse aendern will, benutzt die Vanilla-Stellschraube "
                    + "GetFoodDecayModifier.");
            }
        }

        return true;
    }

    private bool RegisterCategory(notnull ChefZ_PreservationDef def, ChefZ_LoadReport report)
    {
        ChefZ_CategoryManager cats = Cats();
        if (!cats || !cats.IsReady())
        {
            Report(report, true, def, "scope \"category\", aber es gibt keinen "
                + "Kategoriebaum. Der Record wird abgewiesen.");
            return false;
        }

        int bit = cats.GetBitIndex(def.sym);
        if (bit < 0)
        {
            Report(report, true, def, "scope \"category\", aber \"" + def.id + "\" ist keine "
                + "geladene Kategorie. Der Record wird abgewiesen; alle uebrigen "
                + "Haltbarkeitsregeln wirken weiter.");
            return false;
        }

        // Der Bitindex wird EINMAL beim Boot geholt. Zur Laufzeit ist der Test
        // dann ein Bitzugriff auf der Vorfahren-Closure des Items (04 E1) -
        // kein Namensvergleich, kein Baumlauf.
        m_CategoryDefs.Insert(def);
        m_CategoryBits.Insert(bit);
        return true;
    }

    private bool RegisterTag(notnull ChefZ_PreservationDef def, ChefZ_LoadReport report)
    {
        ChefZ_CategoryManager cats = Cats();
        if (cats && cats.IsReady() && !cats.TagExists(def.sym))
        {
            Report(report, true, def, "scope \"tag\", aber \"" + def.id + "\" ist kein "
                + "deklarierter Tag. Der Record wird abgewiesen; alle uebrigen "
                + "Haltbarkeitsregeln wirken weiter. Tags werden in CfgChefZTags bzw. "
                + "Tags.json deklariert - ein undeklarierter Tag matcht nirgends (04 §6).");
            return false;
        }

        m_ByTag.Set(def.sym, def);
        return true;
    }

    private bool RegisterQuality(notnull ChefZ_PreservationDef def, ChefZ_LoadReport report)
    {
        ChefZ_QualityManager quality = Quality();
        if (quality && quality.IsReady() && !quality.Exists(def.sym))
        {
            Report(report, true, def, "scope \"quality\", aber \"" + def.id + "\" ist keine "
                + "geladene Qualitaetsstufe. Der Record wird abgewiesen; alle uebrigen "
                + "Haltbarkeitsregeln wirken weiter.");
            return false;
        }

        m_ByQuality.Set(def.sym, def);
        return true;
    }

    //==========================================================================
    // Der Multiplikator (14 §3)
    //==========================================================================

    /**
     * Die Produktkette aus 14 §3, woertlich:
     *
     *   mul = globalSpoilageScale          Core.json, Serverstellschraube
     *       * StateDef.spoilageMultiplier  aus 06
     *       * QualityTier.spoilageMultiplier aus 12
     *       * ContainerModifier            aus 16, Default 1.0
     *       * ClassOverride                Default 1.0
     *       * PRODUKT(Kategorie-/Tag-Records)
     *     geklemmt auf [minDecayScale, maxDecayScale]
     *
     * Zwei Praezisierungen gegenueber dem Entwurfstext, beide belegt:
     *
     * 1. "SUM(Kategorie-/Tag-Records)" ist als PRODUKT umgesetzt. 14 §8 sagt
     *    es ausdruecklich: "Mehrere Records treffen zu -> Multipliziert; die
     *    Klemmung auf [0.01, 10] faengt Extremwerte ab." Eine Summe von
     *    Faktoren waere ausserdem bei genau einem Treffer schon falsch (ein
     *    einzelner Record 0.5 ergaebe 0.5, zwei ergaeben 1.0 - haltbarer wird
     *    daraus unhaltbarer).
     *
     * 2. Der "ClassOverride aus CfgChefZIngredients" ist hier ein
     *    Preservation-Record mit scope "class". 14 §5 fuehrt diesen scope
     *    ausdruecklich, und zwei Autorenstellen fuer dieselbe Zahl waeren eine
     *    zweite Wahrheit und ein zweiter Ort, an dem ein Content-Autor suchen
     *    muesste - dasselbe Argument, mit dem 06 die Uebergaenge aus
     *    ChefZ_StateDef heraushaelt.
     *
     * @param containerModifier  aus 16. Bis S17 reicht der Aufrufer 1.0.
     * @param environmentTemperature  ChefZ_Undefined.FLOAT heisst "unbekannt";
     *        temperaturgebundene Regeln greifen dann nicht (siehe
     *        ChefZ_PreservationDef.AppliesAt).
     * @param trace  NUR fuer Diagnose. null heisst "keine Ablaufverfolgung"
     *        und ist der Normalfall im Verfallstakt - die Liste wird
     *        ABSICHTLICH nicht angelegt, denn sonst kostete jeder Tick jedes
     *        Lebensmittels eine Allokation und mehrere
     *        Zeichenkettenverkettungen, nur damit niemand hinsieht. Wer eine
     *        angelegte Liste uebergibt, bekommt je wirksamen Faktor eine
     *        Zeile.
     *
     * @return immer ein Wert in [minDecayScale, maxDecayScale]. Nie 0, nie
     *         negativ, nie NaN.
     */
    float ComputeDecayScale(ChefZ_Sym state, ChefZ_Sym quality, ChefZ_Sym classSym,
                            ChefZ_CategoryClosure closure, array<ChefZ_Sym> tags,
                            float containerModifier, float environmentTemperature,
                            out array<string> trace)
    {
        if (!m_Ready)
        {
            // 14 §8: "Registry nicht geladen -> super.ProcessDecay(delta, ...)
            // unveraendert." Der neutrale Faktor ist genau das.
            ReportNotReady("ComputeDecayScale");
            return NEUTRAL;
        }

        float mul = m_GlobalScale;
        Trace(trace, "global", m_GlobalScale);

        // --- 06: der Zustand bringt seinen eigenen Faktor mit -----------------
        // Er steht auf ChefZ_StateDef und ist damit die Aussage des ZUSTANDS
        // ueber sich selbst. Ein Preservation-Record mit scope "state" ist die
        // Aussage des BETREIBERS ueber denselben Zustand. Beide wirken, beide
        // sind 1.0, wenn niemand etwas sagt - und wer beide setzt, bekommt das
        // Produkt. Das ist gewollt (14 §3 fuehrt sie als getrennte Faktoren).
        ChefZ_StateManager states = States();
        if (states && states.IsReady() && ChefZ_SymbolTable.IsValid(state))
        {
            float stateFactor = states.GetSpoilageMultiplier(state);
            if (stateFactor != NEUTRAL)
            {
                mul = mul * stateFactor;
                Trace(trace, "zustand " + ChefZ_SymbolTable.NameOrMark(state), stateFactor);
            }
        }

        // --- 12: die Qualitaetsstufe -----------------------------------------
        ChefZ_QualityManager qualityMgr = Quality();
        if (qualityMgr && qualityMgr.IsReady() && ChefZ_SymbolTable.IsValid(quality))
        {
            float tierFactor = qualityMgr.GetSpoilageMultiplier(quality);
            if (tierFactor != NEUTRAL)
            {
                mul = mul * tierFactor;
                Trace(trace, "stufe " + ChefZ_SymbolTable.NameOrMark(quality), tierFactor);
            }
        }

        // --- 16: der Behaelter ------------------------------------------------
        // Ein nicht positiver Behaelterfaktor ist ein Rechenfehler des
        // Aufrufers und wird stillschweigend als neutral behandelt: der
        // Container-Manager existiert noch nicht, und eine Warnung ueber einen
        // Wert, den bis S17 niemand setzen kann, waere Rauschen.
        if (containerModifier > 0.0 && containerModifier != NEUTRAL)
        {
            mul = mul * containerModifier;
            Trace(trace, "behaelter", containerModifier);
        }

        // --- Die Records ------------------------------------------------------
        mul = mul * FactorOf(m_ByState,   state,     environmentTemperature, trace, "regel:zustand");
        mul = mul * FactorOf(m_ByClass,   classSym,  environmentTemperature, trace, "regel:klasse");
        mul = mul * FactorOf(m_ByQuality, quality,   environmentTemperature, trace, "regel:stufe");
        mul = mul * CategoryFactor(closure, environmentTemperature, trace);
        mul = mul * TagFactor(tags, environmentTemperature, trace);

        return ClampScale(mul, trace);
    }

    /**
     * Der zusaetzliche Faktor, solange das Item am Spieler haengt (14 §5,
     * onPlayerMultiplier).
     *
     * Getrennt von ComputeDecayScale, damit die Signatur aus 14 §5 unveraendert
     * bleibt und der Aufrufer sichtbar entscheidet, ob er ihn anwendet.
     *
     * WICHTIG und in ChefZ_PreservationDef.onPlayerMultiplier begruendet: das
     * ist ein Faktor ZUSAETZLICH zu Vanillas DECAY_RATE_ON_PLAYER, kein Ersatz
     * dafuer. Vanilla addiert seinen Bonus innerhalb von ProcessDecay auf
     * m_DecayDelta (01 V9); dort kommt ChefZ nicht hin, ohne die Methode
     * nachzubauen - und genau das verbietet 14 E1.
     *
     * @return NEUTRAL, wenn keine Regel etwas sagt.
     */
    float ComputeOnPlayerScale(ChefZ_Sym state, ChefZ_Sym quality, ChefZ_Sym classSym,
                               ChefZ_CategoryClosure closure, array<ChefZ_Sym> tags,
                               float environmentTemperature)
    {
        if (!m_Ready)
            return NEUTRAL;

        float mul = NEUTRAL;
        mul = mul * OnPlayerOf(Lookup(m_ByState, state), environmentTemperature);
        mul = mul * OnPlayerOf(Lookup(m_ByClass, classSym), environmentTemperature);
        mul = mul * OnPlayerOf(Lookup(m_ByQuality, quality), environmentTemperature);

        for (int c = 0; c < m_CategoryDefs.Count(); c++)
        {
            if (closure && closure.HasBit(m_CategoryBits.Get(c)))
                mul = mul * OnPlayerOf(m_CategoryDefs.Get(c), environmentTemperature);
        }

        if (tags)
        {
            for (int t = 0; t < tags.Count(); t++)
                mul = mul * OnPlayerOf(Lookup(m_ByTag, tags.Get(t)), environmentTemperature);
        }

        if (mul <= 0.0)
            return NEUTRAL;
        return mul;
    }

    private float OnPlayerOf(ChefZ_PreservationDef def, float environmentTemperature)
    {
        if (!def || !def.HasOnPlayerMultiplier())
            return NEUTRAL;
        if (!def.AppliesAt(environmentTemperature))
            return NEUTRAL;
        return def.onPlayerMultiplier;
    }

    //--------------------------------------------------------------------------

    private ChefZ_PreservationDef Lookup(notnull map<int, ChefZ_PreservationDef> table, ChefZ_Sym key)
    {
        if (!ChefZ_SymbolTable.IsValid(key))
            return null;

        ChefZ_PreservationDef def;
        if (!table.Find(key, def))
            return null;
        return def;
    }

    private float FactorOf(notnull map<int, ChefZ_PreservationDef> table, ChefZ_Sym key,
                           float environmentTemperature, array<string> trace, string label)
    {
        ChefZ_PreservationDef def = Lookup(table, key);
        if (!def)
            return NEUTRAL;
        if (!def.AppliesAt(environmentTemperature))
            return NEUTRAL;
        if (def.spoilageMultiplier == NEUTRAL)
            return NEUTRAL;

        Trace(trace, label + " " + def.id, def.spoilageMultiplier);
        return def.spoilageMultiplier;
    }

    private float CategoryFactor(ChefZ_CategoryClosure closure, float environmentTemperature,
                                 array<string> trace)
    {
        if (!closure || m_CategoryDefs.Count() == 0)
            return NEUTRAL;

        float mul = NEUTRAL;
        for (int i = 0; i < m_CategoryDefs.Count(); i++)
        {
            // Vorfahren-Closure (04 E1): eine Regel auf einer Oberkategorie
            // trifft auch jede Unterkategorie. Genau das macht Kategorien zum
            // brauchbaren Feinschliff - eine Regel statt zwanzig.
            if (!closure.HasBit(m_CategoryBits.Get(i)))
                continue;

            ChefZ_PreservationDef def = m_CategoryDefs.Get(i);
            if (!def.AppliesAt(environmentTemperature))
                continue;
            if (def.spoilageMultiplier == NEUTRAL)
                continue;

            mul = mul * def.spoilageMultiplier;
            Trace(trace, "regel:kategorie " + def.id, def.spoilageMultiplier);
        }
        return mul;
    }

    private float TagFactor(array<ChefZ_Sym> tags, float environmentTemperature,
                            array<string> trace)
    {
        if (!tags || m_ByTag.Count() == 0)
            return NEUTRAL;

        float mul = NEUTRAL;
        for (int i = 0; i < tags.Count(); i++)
            mul = mul * FactorOf(m_ByTag, tags.Get(i), environmentTemperature, trace, "regel:tag");
        return mul;
    }

    /**
     * 14 §8: die Klemmung auf [minDecayScale, maxDecayScale] faengt
     * Extremwerte ab.
     *
     * Sie faengt ausserdem den Fall ab, den kein Betreiber schreibt, aber jede
     * Fliesskommakette erzeugen kann: NaN. Ein NaN im Faktor wuerde als delta
     * an Vanilla gehen und dort m_DecayTimer dauerhaft auf NaN setzen - das
     * Item verdirbt danach NIE mehr, ueber jeden Neustart hinweg, weil der
     * Timer persistiert wird (Edible_Base.c:318). Der Test laeuft ueber
     * "nicht innerhalb der Grenzen", weil jeder Vergleich mit NaN false
     * liefert; ein == -Test auf NaN wuerde ihn nicht finden.
     */
    /**
     * Dieselbe Klemmung, oeffentlich - fuer den einen Aufrufer, der nach
     * ComputeDecayScale noch einen Faktor dazumultipliziert: den Spielerbonus
     * (ChefZ_ItemDecay.Plan).
     *
     * Ohne sie koennte das Produkt aus einem bereits geklemmten Faktor und
     * onPlayerMultiplier ueber maxDecayScale hinauslaufen - und die Klemmung
     * waere genau dort wirkungslos, wo die groessten Faktoren entstehen.
     */
    float ClampToBounds(float value)
    {
        return ClampScale(value, null);
    }

    private float ClampScale(float mul, array<string> trace)
    {
        if (!(mul >= m_MinScale && mul <= m_MaxScale))
        {
            float fixedValue = m_MinScale;
            if (mul > m_MaxScale)
                fixedValue = m_MaxScale;

            Trace(trace, "geklemmt", fixedValue);
            return fixedValue;
        }
        return mul;
    }

    private void Trace(array<string> trace, string label, float value)
    {
        if (!trace)
            return;
        trace.Insert(label + " = " + value.ToString());
    }

    //==========================================================================
    // Die beiden Schalter (14 E7)
    //==========================================================================

    /**
     * true => CanProcessDecay() liefert false, der Verfall laeuft GAR NICHT.
     *
     * closure und tags sind optional; die Signatur in 14 §5 nennt sie nicht,
     * aber ein Konservenschalter, der nur ueber Zustand, Klasse und
     * Qualitaetsstufe erreichbar waere, koennte die Kategorie- und
     * Tag-Dimension aus 14 §5 nicht bedienen. Wer sie weglaesst, bekommt
     * genau das Verhalten der Entwurfssignatur.
     */
    bool StopsDecay(ChefZ_Sym state, ChefZ_Sym quality, ChefZ_Sym classSym,
                    ChefZ_CategoryClosure closure = null, array<ChefZ_Sym> tags = null)
    {
        if (m_StopsDecayCount == 0)
            return false;
        return AnyFlag(state, quality, classSym, closure, tags, true);
    }

    //! true, wenn ueberhaupt irgendeine Regel stopsDecay setzt. Die Itemseite
    //! fragt das VOR der Faktenerhebung - siehe m_StopsDecayCount.
    bool HasAnyStopsDecay()
    {
        return m_Ready && m_StopsDecayCount > 0;
    }

    //! Dasselbe fuer preventsRotten.
    bool HasAnyPreventsRotten()
    {
        return m_Ready && m_PreventsRottenCount > 0;
    }

    //! true => der Verfall laeuft weiter, die Vanilla-Garstufe wechselt aber
    //! nicht auf ROTTEN (14 E7).
    bool PreventsRotten(ChefZ_Sym state, ChefZ_Sym quality, ChefZ_Sym classSym,
                        ChefZ_CategoryClosure closure = null, array<ChefZ_Sym> tags = null)
    {
        if (m_PreventsRottenCount == 0)
            return false;
        return AnyFlag(state, quality, classSym, closure, tags, false);
    }

    /**
     * Ein gesetzter Schalter irgendwo in der Kette gewinnt.
     *
     * "Irgendwo gewinnt" und nicht "das Speziellere gewinnt": beide Schalter
     * sagen "weniger Verfall", und es gibt kein Feld, mit dem ein Record das
     * Gegenteil ausdruecken koennte. Eine Rangfolge braeuchte man erst, wenn
     * es ein "stopsDecay: false, und zwar ausdruecklich" gaebe - und das waere
     * ein anderer Entwurf.
     *
     * Der Temperaturbereich gilt hier NICHT: ein Schalter ist eine Aussage
     * ueber das Item, keine ueber die Umgebung, und ein temperaturabhaengiger
     * Konservenschalter waere ein Item, das je nach Wetter verdirbt oder
     * nicht - ohne dass ein Spieler den Unterschied sehen koennte.
     */
    private bool AnyFlag(ChefZ_Sym state, ChefZ_Sym quality, ChefZ_Sym classSym,
                         ChefZ_CategoryClosure closure, array<ChefZ_Sym> tags, bool wantStops)
    {
        if (!m_Ready)
            return false;

        if (FlagOf(Lookup(m_ByState, state), wantStops))       return true;
        if (FlagOf(Lookup(m_ByClass, classSym), wantStops))    return true;
        if (FlagOf(Lookup(m_ByQuality, quality), wantStops))   return true;

        if (closure)
        {
            for (int c = 0; c < m_CategoryDefs.Count(); c++)
            {
                if (closure.HasBit(m_CategoryBits.Get(c)) && FlagOf(m_CategoryDefs.Get(c), wantStops))
                    return true;
            }
        }

        if (tags)
        {
            for (int t = 0; t < tags.Count(); t++)
            {
                if (FlagOf(Lookup(m_ByTag, tags.Get(t)), wantStops))
                    return true;
            }
        }

        return false;
    }

    private bool FlagOf(ChefZ_PreservationDef def, bool wantStops)
    {
        if (!def)
            return false;
        if (wantStops)
            return def.stopsDecay;
        return def.preventsRotten;
    }

    //==========================================================================
    // Restfrische (14 §4)
    //==========================================================================

    /**
     * Freshness01 -= (deltaSec * multiplier) / freshnessLifetimeSec   (14 §4)
     *
     * Monoton fallend, ohne Zufallsanteil, geklemmt auf 0..1. Bei 0
     * angekommen passiert von selbst GAR NICHTS - "Frische bestraft, Vanilla
     * toetet" (14 §4).
     *
     * @param multiplier  derselbe Faktor, der auch an Vanilla ging. Damit
     *        faellt die Frische im selben Takt wie der Verfall - ein
     *        geraeuchertes Fleisch mit Faktor 0.25 haelt viermal so lange und
     *        bleibt viermal so lange frisch.
     * @return die neue Frische. Ist die Lebensdauer <= 0, kommt der EINGABEWERT
     *         unveraendert zurueck (14 §8: "Frische eingefroren").
     */
    float AdvanceFreshness(float current01, float deltaSec, float multiplier, ChefZ_Sym state)
    {
        float start = Sanitize(current01);

        if (deltaSec <= 0.0 || multiplier <= 0.0)
            return start;

        float lifetime = GetFreshnessLifetimeSec(state);
        if (lifetime <= 0.0)
            return start;

        float next = start - ((deltaSec * multiplier) / lifetime);
        return Math.Clamp(next, 0.0, 1.0);
    }

    /**
     * Die Lebensdauer der Frische fuer einen Zustand, in Sekunden.
     *
     * Zustand zuerst, Servervorgabe danach (06 §4.1: "Sentinel = aus
     * CoreSettings, siehe 14"). Ein Item ohne Zustand - und das ist auf einem
     * Server ohne Content-Modul JEDES Item - bekommt die Servervorgabe. Ohne
     * sie waere seine Frische eingefroren, und das saehe wie ein Erfolg aus.
     */
    float GetFreshnessLifetimeSec(ChefZ_Sym state)
    {
        ChefZ_StateManager states = States();
        if (states && states.IsReady() && ChefZ_SymbolTable.IsValid(state))
        {
            ChefZ_StateDef def = states.GetDef(state);
            if (def && def.HasFreshnessLifetime())
                return def.freshnessLifetimeSec;
        }
        return m_DefaultFreshnessLifetimeSec;
    }

    /**
     * Frische eines Ergebnisses aus den Eingaben (14 §6, "BEI EINEM TRANSFORM
     * / REZEPT").
     *
     * @param rule  "MIN" (Vorgabe) | "MEAN" | "WEIGHTED_MEAN"
     * @param carry Faktor auf das Ergebnis. Negativ heisst "unveraendert" -
     *        derselbe Sentinelfall wie in ChefZ_ItemStateComponent.InheritFrom.
     *
     * MIN ist die Vorgabe, und das ist die wichtigste Zeile dieser Methode:
     * mit einem Mittelwert liesse sich altes Fleisch in frischem "waschen" -
     * genau der Exploit, den 12 §4.1 schliesst. WEIGHTED_MEAN gewichtet mit
     * den Rezepteinheiten (units) und ist deshalb ebenfalls verwaesserbar; er
     * steht nur bereit, weil 14 §5 ihn nennt.
     *
     * @return 1.0 bei leerer Eingabe. Ein Gericht aus dem Nichts ist frisch -
     *         alles andere waere eine Strafe fuer fehlende Daten.
     */
    float InheritFreshness(array<ref ChefZ_ItemFacts> inputs, float carry, string rule)
    {
        float result = 1.0;

        if (inputs && inputs.Count() > 0)
        {
            string r = rule;
            r.TrimInPlace();
            r.ToUpper();

            if (r == "MEAN")
                result = MeanFreshness(inputs, false);
            else if (r == "WEIGHTED_MEAN")
                result = MeanFreshness(inputs, true);
            else
                result = MinFreshness(inputs);
        }

        float factor = carry;
        if (factor < 0.0)
            factor = 1.0;

        return Math.Clamp(result * factor, 0.0, 1.0);
    }

    private float MinFreshness(notnull array<ref ChefZ_ItemFacts> inputs)
    {
        float lowest = 1.0;
        for (int i = 0; i < inputs.Count(); i++)
        {
            ChefZ_ItemFacts f = inputs.Get(i);
            if (!f)
                continue;
            float v = Sanitize(f.freshness01);
            if (v < lowest)
                lowest = v;
        }
        return lowest;
    }

    private float MeanFreshness(notnull array<ref ChefZ_ItemFacts> inputs, bool weighted)
    {
        float sum    = 0.0;
        float weight = 0.0;

        for (int i = 0; i < inputs.Count(); i++)
        {
            ChefZ_ItemFacts f = inputs.Get(i);
            if (!f)
                continue;

            float w = 1.0;
            if (weighted && f.units > 0.0)
                w = f.units;

            sum    = sum + (Sanitize(f.freshness01) * w);
            weight = weight + w;
        }

        if (weight <= 0.0)
            return 1.0;
        return sum / weight;
    }

    /**
     * Restlaufzeit der Frische in Sekunden. NUR fuer Anzeige und Diagnose
     * (14 §5: "nur UI/Debug").
     *
     * Sie ist AUSDRUECKLICH keine Aussage darueber, wann das Item verdirbt -
     * das entscheidet Vanillas m_DecayTimer, der zufallsbehaftet ist und bei
     * jedem Stufenwechsel neu ausgewuerfelt wird (01 V9). Wer diese Zahl als
     * Haltbarkeit anzeigt, zeigt etwas anderes an, als er meint.
     *
     * @return -1.0, wenn es keine Restlaufzeit gibt (Frische eingefroren oder
     *         Faktor 0).
     */
    float EstimateRemainingSec(float current01, ChefZ_Sym state, float multiplier)
    {
        if (multiplier <= 0.0)
            return -1.0;

        float lifetime = GetFreshnessLifetimeSec(state);
        if (lifetime <= 0.0)
            return -1.0;

        return (Sanitize(current01) * lifetime) / multiplier;
    }

    /**
     * 14 §8: "Freshness01 persistiert als NaN oder ausserhalb 0..1 -> auf 1.0
     * gesetzt."
     *
     * Der Test laeuft ueber "nicht innerhalb 0..1" statt ueber einen
     * Bereichsvergleich, weil jeder Vergleich mit NaN false liefert - damit
     * faengt eine einzige Bedingung beide Faelle. Math.Clamp taete es NICHT:
     * es reicht NaN durch.
     *
     * Die WARNUNG mit Klassenname gehoert nicht hierher, sondern an die
     * Stelle, die eine Klasse kennt (ChefZ_ItemDecay, 4_World). Hier gibt es
     * nur Zahlen.
     */
    static float Sanitize(float freshness01)
    {
        if (!(freshness01 >= 0.0 && freshness01 <= 1.0))
            return 1.0;
        return freshness01;
    }

    //! true, wenn der Wert als Frische unbrauchbar ist (NaN oder ausserhalb
    //! 0..1). Der Aufrufer in 4_World meldet ihn mit Klassennamen.
    static bool IsFreshnessBroken(float freshness01)
    {
        return !(freshness01 >= 0.0 && freshness01 <= 1.0);
    }

    //==========================================================================
    // Auskunft
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    int GetRuleCount()
    {
        return m_ByState.Count() + m_ByClass.Count() + m_ByTag.Count()
             + m_ByQuality.Count() + m_CategoryDefs.Count();
    }

    int GetRejectedCount()
    {
        return m_RejectedCount;
    }

    float GetGlobalScale()          { return m_GlobalScale; }
    float GetMinScale()             { return m_MinScale; }
    float GetMaxScale()             { return m_MaxScale; }
    float GetDefaultFreshnessLifetimeSec() { return m_DefaultFreshnessLifetimeSec; }

    //! Der Datensatz einer Dimension, oder null. Fuer Cookbook und Diagnose.
    ChefZ_PreservationDef GetDef(int scopeKind, ChefZ_Sym target)
    {
        switch (scopeKind)
        {
            case ChefZ_PreservationScope.STATE:   return Lookup(m_ByState, target);
            case ChefZ_PreservationScope.CLASS:   return Lookup(m_ByClass, target);
            case ChefZ_PreservationScope.TAG:     return Lookup(m_ByTag, target);
            case ChefZ_PreservationScope.QUALITY: return Lookup(m_ByQuality, target);
            case ChefZ_PreservationScope.CATEGORY:
                for (int i = 0; i < m_CategoryDefs.Count(); i++)
                {
                    ChefZ_PreservationDef def = m_CategoryDefs.Get(i);
                    if (def.sym == target)
                        return def;
                }
                return null;
        }
        return null;
    }

    void DumpRules(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Haltbarkeit: " + GetRuleCount().ToString() + " Regel(n), Skala "
            + m_GlobalScale.ToString() + ", Grenzen [" + m_MinScale.ToString() + ".."
            + m_MaxScale.ToString() + "], Frischevorgabe "
            + m_DefaultFreshnessLifetimeSec.ToString() + "s");

        for (int i = 0; i < m_Order.Count(); i++)
        {
            ChefZ_Sym sym = m_Order.Get(i);
            ChefZ_PreservationDef def = FindAnywhere(sym);
            if (def)
                outLines.Insert("    " + def.ToLine());
        }
    }

    private ChefZ_PreservationDef FindAnywhere(ChefZ_Sym sym)
    {
        ChefZ_PreservationDef def = Lookup(m_ByState, sym);
        if (def) return def;
        def = Lookup(m_ByClass, sym);
        if (def) return def;
        def = Lookup(m_ByTag, sym);
        if (def) return def;
        def = Lookup(m_ByQuality, sym);
        if (def) return def;
        return GetDef(ChefZ_PreservationScope.CATEGORY, sym);
    }

    //==========================================================================
    // Innereien
    //==========================================================================

    //! Der Auszug aus Architekturplan §21, hinter der Kanalwache aus 18 §2.
    private void LogIfDebug()
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.PRESERV, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        DumpRules(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.PRESERV, lines);
    }

    private ChefZ_CategoryManager Cats()
    {
        if (m_CategoriesForTest)
            return m_CategoriesForTest;
        return ChefZ_CategoryManager.Get();
    }

    private ChefZ_StateManager States()
    {
        if (m_StatesForTest)
            return m_StatesForTest;
        return ChefZ_StateManager.Get();
    }

    private ChefZ_QualityManager Quality()
    {
        if (m_QualityForTest)
            return m_QualityForTest;
        return ChefZ_QualityManager.Get();
    }

    private void ResetState()
    {
        m_ByState.Clear();
        m_ByClass.Clear();
        m_ByTag.Clear();
        m_ByQuality.Clear();
        m_CategoryDefs.Clear();
        m_CategoryBits.Clear();
        m_Order.Clear();

        m_GlobalScale                 = NEUTRAL;
        m_MinScale                    = 0.01;
        m_MaxScale                    = 10.0;
        m_DefaultFreshnessLifetimeSec = 21600.0;

        m_Ready               = false;
        m_NotReadyLogged      = false;
        m_RejectedCount       = 0;
        m_StopsDecayCount     = 0;
        m_PreventsRottenCount = 0;
    }

    //! Nur fuer den SAFE_MODE-Rueckbau und den Selbsttest.
    void Reset()
    {
        ResetState();
    }

    /**
     * 14 §8, letzte Zeile der Tabelle sinngemaess: eine Abfrage vor dem Build
     * ist ein Programmierfehler, kein Datenfehler - deshalb genau EINE
     * Meldung, nicht eine je Verfallstick.
     */
    private void ReportNotReady(string what)
    {
        if (m_NotReadyLogged || m_QuietForTest)
            return;
        m_NotReadyLogged = true;

        ChefZ_Log.Error(ChefZ_LogChannel.PRESERV,
            "ChefZ_PreservationManager." + what + " wurde vor Build() gerufen. Es gilt der "
            + "neutrale Faktor 1.0 - der Verfall ist damit bitgenau Vanilla. Diese Meldung "
            + "erscheint genau einmal.");
    }

    private void Report(ChefZ_LoadReport report, bool isError,
                        notnull ChefZ_PreservationDef def, string msg)
    {
        if (report)
        {
            if (isError)
                report.AddError(def.sourceRef, def.id, msg);
            else
                report.AddWarn(def.sourceRef, def.id, msg);
            return;
        }

        if (m_QuietForTest)
            return;

        int level = ChefZ_LogLevel.WARN;
        if (isError)
            level = ChefZ_LogLevel.ERR;
        ChefZ_Log.Once(level, ChefZ_LogChannel.PRESERV, "preserv.build." + def.id, msg);
    }

    //--------------------------------------------------------------------------
    // Nur fuer den Selbsttest
    //--------------------------------------------------------------------------

    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }

    void SetManagersForTest(ChefZ_CategoryManager cats, ChefZ_StateManager states,
                            ChefZ_QualityManager quality)
    {
        m_CategoriesForTest = cats;
        m_StatesForTest     = states;
        m_QualityForTest    = quality;
    }
}
