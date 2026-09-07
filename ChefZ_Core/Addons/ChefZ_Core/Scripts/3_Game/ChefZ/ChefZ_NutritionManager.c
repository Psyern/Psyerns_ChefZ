//==============================================================================
// ChefZ_NutritionManager - Sollrechnung und Startaudit
//
// Entwurf: 13 §4 (Schnittstelle woertlich), 13 §5 (der Startaudit - "der
// eigentliche Nutzen"), 13 §6 (Datenfluss), 13 §7 (Zustand), 13 §8
// (Fehlerverhalten, Zeile fuer Zeile), 13 E1 bis E6, 01 V6 / V7.
//
// ---------------------------------------------------------------------------
// Der ungewoehnlichste Manager des Core - und warum er so sein MUSS
// ---------------------------------------------------------------------------
// Er veraendert zur Laufzeit NICHTS. Kein Item, keine Config, keinen
// Balancingwert, keinen Magen. 13 E2 nennt das beim Namen: "Ein Manager ohne
// Laufzeitwirkung klingt nach totem Code - er ist aber das EINZIGE Werkzeug,
// mit dem 25 Gerichte ueber sechs Content-Slices hinweg konsistent gebalanct
// werden koennen. Der Nutzen liegt im Startlog, nicht im Spiel."
//
// Der Grund ist eine Engine-Grenze, kein Geschmack (01 V6):
//
//   PlayerBase.Consume(data)
//     -> m_PlayerStomach.AddToStomach(data.m_Source.GetType(),  // KLASSENNAME
//                                     data.m_Amount, foodStageType, ...)
//          -> if (GetIDFromClassname(class_name) == -1) return;  // V7
//          -> Edible_Base.GetNutritionalProfile(null, class_name, food_stage)
//                                               ^^^^ das Item ist NULL
//
// Der Naehrwert eines Bissens haengt ausschliesslich von Klasse x Foodstage
// ab. Zwei Instanzen derselben Klasse in derselben Stufe sind
// ernaehrungstechnisch identisch - unabhaengig von Qualitaet, Zutatenfrische
// oder Herkunft. Ein Naehrwertvektor am Item erreichte den Verzehrpfad nie.
//
// Deshalb gibt es hier KEINE schreibende Methode, und deshalb soll es auch
// keine geben: eine Methode, die nichts bewirken kann, ist ein Versprechen,
// das der Core nicht halten kann.
//
// ---------------------------------------------------------------------------
// Was er stattdessen tut - drei Dinge
// ---------------------------------------------------------------------------
//   1. ComputeExpected()  rechnet aus, was ein Gericht aus seinen Zutaten
//                         haben SOLLTE (13 §5).
//   2. AuditAllRecipes()  haelt diesen Sollwert beim Boot gegen die
//                         tatsaechlichen Werte in CfgVehicles und meldet die
//                         Abweichungen - mit Rezept-ID und Klassenname.
//   3. Er ist die zweite von drei Fangstellen fuer Befund 01 V7 (13 §3): eine
//      essbare Ergebnisklasse ohne "class Nutrition"/"class Food" oder mit
//      scope = 0 wird gegessen, verschwindet und saettigt NICHTS - lautlos.
//      Der Audit meldet sie als ERROR; abgewiesen wird das Rezept vom
//      ChefZ_RecipeCompiler (08 §8).
//
// ---------------------------------------------------------------------------
// Was er ausdruecklich NICHT tut (13 E3)
// ---------------------------------------------------------------------------
// Er gibt keinen Energie- oder Wasserbonus am Magen vorbei. Technisch waere
// consumer.GetStatEnergy().Add(...) in OnConsume moeglich; verworfen aus drei
// Gruenden: es entkoppelt Saettigung von Energiezufuhr, es wirkt sofort
// statt ueber die Verdauungszeit, und es ist ein offener Vektor fuer "viele
// kleine Bissen". Wer ein Gericht nahrhafter machen will, tut das ueber die
// Ausbeute (12 E2) oder ueber eine eigene Ergebnisklasse.
//
// KEIN CONTENT: dieser Manager definiert keine einzige Naehrwertangabe. Alles
// kommt aus der Registry oder aus CfgVehicles.
//
// Layer: 3_Game. Er liest Registries und CfgVehicles und kennt keinen
// Engine-Itemtyp - kein ItemBase, kein FoodStage, kein NutritionalProfile.
//==============================================================================

class ChefZ_NutritionManager : Managed
{
    private static ref ChefZ_NutritionManager s_Instance;

    //--- Bestand, indiziert ueber ChefZ_Sym ----------------------------------
    //
    // KEIN ref-Wert: Eigentuemer der Records ist die Registry des Config
    // Managers, und die lebt laenger als dieser Manager. Ein zweiter starker
    // Verweis waere ein Zyklus ohne Gewinn - dieselbe Loesung wie im
    // ChefZ_PreservationManager.
    private ref map<int, ChefZ_NutritionDef>  m_ByClass;
    private ref map<int, ChefZ_NutritionDef>  m_ByTag;

    //! Kategorieregeln liegen als parallele Listen vor: der Bitindex laesst
    //! sich gegen eine ChefZ_CategoryClosure in einem Schritt testen, ein
    //! Symbol nicht.
    private ref array<ChefZ_NutritionDef>     m_CategoryDefs;
    private ref array<int>                    m_CategoryBits;
    private ref array<int>                    m_CategoryDepth;

    //! Aufnahmereihenfolge, sortiert nach ID (Registry.Keys()). Sie ist die
    //! Grundlage jeder Bestimmtheitsaussage: dieselbe Config ergibt auf jedem
    //! Server dieselbe Auffindung.
    private ref array<ChefZ_Sym>              m_Order;

    //--- Einstellungen (Core.json, 13 §8) ------------------------------------
    private bool  m_AuditEnabled;
    private float m_TolerancePct;
    private int   m_MaxFindings;
    private float m_ExpectedCap;

    //! Sondengrenzen fuer ClampTo. Einmal gebaut, danach nur gelesen.
    private ref ChefZ_NutritionVector m_Caps;
    private ref ChefZ_NutritionVector m_Floors;

    //! Wiederverwendete Puffer. Der Audit laeuft ueber alle Rezepte x alle
    //! Pflichtslots; ein neuer Vektor je Zutat waere bei hundert Rezepten
    //! einige tausend Allokationen fuer eine Zahl, die danach im Log steht.
    private ref ChefZ_NutritionVector m_ScratchBase;
    private ref ChefZ_NutritionVector m_ScratchActual;

    private bool m_Ready;
    private int  m_RejectedCount;

    //--- Ergebnis des letzten Audits (13 §7: Laufzeit, einmalig beim Boot) ---
    private ref array<ref ChefZ_NutritionFinding> m_LastFindings;
    private int m_LastAuditedRecipes;
    private int m_LastErrorCount;
    private bool m_AuditDone;

    //! Stumm im Selbsttest - sonst schriebe jeder Testlauf hunderte Zeilen
    //! ins RPT, und der eine echte Befund ginge darin unter.
    private bool m_QuietForTest;

    //! Manager des Tests statt der Singletons - dieselbe Loesung wie im
    //! ChefZ_PreservationManager und aus demselben Grund.
    private ChefZ_CategoryManager   m_CategoriesForTest;
    private ChefZ_IngredientManager m_IngredientsForTest;
    private ChefZ_RecipeEngine      m_EngineForTest;

    //--------------------------------------------------------------------------

    void ChefZ_NutritionManager()
    {
        m_ByClass       = new map<int, ChefZ_NutritionDef>();
        m_ByTag         = new map<int, ChefZ_NutritionDef>();
        m_CategoryDefs  = new array<ChefZ_NutritionDef>();
        m_CategoryBits  = new array<int>();
        m_CategoryDepth = new array<int>();
        m_Order         = new array<ChefZ_Sym>();

        m_Caps          = new ChefZ_NutritionVector();
        m_Floors        = new ChefZ_NutritionVector();
        m_ScratchBase   = new ChefZ_NutritionVector();
        m_ScratchActual = new ChefZ_NutritionVector();
        m_LastFindings  = new array<ref ChefZ_NutritionFinding>();

        m_QuietForTest  = false;

        ResetState();
    }

    static ChefZ_NutritionManager Get()
    {
        if (!s_Instance)
            s_Instance = new ChefZ_NutritionManager();
        return s_Instance;
    }

    //==========================================================================
    // Aufbau (13 §6, BOOT)
    //==========================================================================

    /**
     * Baut den Bestand. Einmal beim Boot, danach unveraenderlich.
     *
     * Abweichung von der Signatur in 13 §4: die CoreSettings kommen als
     * dritter Parameter dazu - dieselbe Abweichung und dieselbe Begruendung
     * wie beim ChefZ_PreservationManager. 13 §8 nennt EnableNutritionAudit
     * ausdruecklich als Verhaltensschalter, und die Toleranz braucht der
     * Vergleich; sie hier ein zweites Mal aus dem Config Manager zu holen
     * hiesse, dieselbe Groesse an zwei Orten zu lesen, und machte den Manager
     * im Selbsttest unbenutzbar.
     *
     * settings darf null sein: dann gelten die Vorgaben (Audit an, 25 Prozent
     * Toleranz).
     *
     * Der Aufruf ist beim Boot UNBEDINGT - auch ohne eine einzige
     * Naehrwertangabe soll der Manager "bereit und leer" sein. 13 §8, erste
     * Zeile: eine fehlende Nutrition.json ist kein Fehler, weil die
     * tatsaechlichen Werte ohnehin in CfgVehicles stehen. Der Audit laeuft
     * dann vollstaendig ueber den Vanilla-Rueckfall (13 E4) und ist dabei
     * KEIN Stueck weniger aussagekraeftig fuer den einen Befund, auf den es
     * ankommt: die fehlende Registrierung beim Magen.
     */
    void Build(ChefZ_Registry<ChefZ_NutritionDef> defs, ChefZ_LoadReport report, ChefZ_CoreSettingsDef settings = null)
    {
        ResetState();
        ApplySettings(settings);

        if (!defs || defs.Count() == 0)
        {
            // INFO und kein WARN: ein Server ohne Naehrwert-Records ist der
            // Normalfall, solange kein Content-Modul eigene Sollwerte pflegt.
            // Die Sollrechnung faellt dann fuer JEDE Zutat auf CfgVehicles
            // zurueck, und das ist die genauere Quelle, nicht die schlechtere.
            if (report)
                report.AddInfo("Keine Naehrwert-Records definiert - die Sollrechnung des " + "Startaudits liest ausschliesslich CfgVehicles (13 E4). Das Kochen und " + "das Essen sind davon unberuehrt: der Core wendet Naehrwerte NIE an " + "(13 E1).");
            m_Ready = true;
            return;
        }

        // Reihenfolge ist Registry.Keys(), also nach ID sortiert (03 §4).
        // Damit ist jede abgeleitete Groesse auf Client und Server gleich.
        array<ChefZ_Sym> keys = defs.Keys();
        for (int i = 0; i < keys.Count(); i++)
        {
            ChefZ_Sym sym = keys.Get(i);
            ChefZ_NutritionDef def = defs.Find(sym);
            if (!def)
                continue;

            if (Register(def, report))
                m_Order.Insert(sym);
            else
                m_RejectedCount++;
        }

        m_Ready = true;

        if (report)
        {
            string chefzTxt1 = "Naehrwertangaben: " + GetRecordCount().ToString() + " geladen" + " (Klasse " + m_ByClass.Count().ToString();
            chefzTxt1 = chefzTxt1 + ", Kategorie " + m_CategoryDefs.Count().ToString() + ", Tag " + m_ByTag.Count().ToString() + ")";
            chefzTxt1 = chefzTxt1 + ", Audit " + AuditStateName() + ", Toleranz " + m_TolerancePct.ToString() + "%.";
            report.AddInfo(chefzTxt1);
        }

        LogIfDebug();
    }

    /**
     * Die Regler aus Core.json (13 §8).
     *
     * Sie werden hier nur GELESEN, nicht geklammert - das hat
     * ChefZ_CoreSettingsDef.ClampAndReport bereits getan, und eine zweite
     * Klammerung waere eine zweite Wahrheit ueber denselben Wert. Ausnahme ist
     * settings == null (Selbsttest, SAFE_MODE): dort gibt es keinen
     * geklammerten Wert, also stehen hier die Vorgaben.
     */
    private void ApplySettings(ChefZ_CoreSettingsDef settings)
    {
        if (settings)
        {
            m_AuditEnabled = settings.enableNutritionAudit;
            m_TolerancePct = settings.nutritionTolerancePct;
            m_MaxFindings  = settings.nutritionAuditMaxFindings;
            m_ExpectedCap  = settings.nutritionExpectedCap;
        }
        else
        {
            m_AuditEnabled = true;
            m_TolerancePct = 25.0;
            m_MaxFindings  = 64;
            m_ExpectedCap  = 100000.0;
        }

        if (m_TolerancePct < 0.0)
            m_TolerancePct = 0.0;
        if (m_MaxFindings < 1)
            m_MaxFindings = 1;
        if (m_ExpectedCap <= 0.0)
            m_ExpectedCap = 100000.0;

        m_Caps.fullness         = m_ExpectedCap;
        m_Caps.energy           = m_ExpectedCap;
        m_Caps.water            = m_ExpectedCap;
        m_Caps.nutritionalIndex = m_ExpectedCap;
        m_Caps.toxicity         = m_ExpectedCap;
        m_Caps.digestibility    = m_ExpectedCap;

        m_Floors.CopyFrom(m_Caps);
        m_Floors.Scale(-1.0);
    }

    /**
     * Einen Datensatz in seine Dimension einsortieren - und sein ZIEL pruefen.
     *
     * 13 §8 kennt fuer diesen Fall keine eigene Zeile; die Behandlung folgt
     * deshalb 02 §8 und der Linie des ChefZ_PreservationManagers: ein Record,
     * dessen Ziel es nicht gibt, wirkt nie und sieht dabei aus wie eine
     * gepflegte Angabe. Das ist genau die Sorte Fehler, die im Balancing
     * monatelang unentdeckt bleibt - also abweisen und melden.
     *
     * Wichtige Ausnahme fuer scope "class": eine Klasse muss KEINE deklarierte
     * ChefZ-Zutat sein. 13 E4 sagt ausdruecklich, dass jedes Vanilla-Item ohne
     * jede ChefZ-Deklaration als Zutat auditierbar ist - ein Klassenrecord
     * darf deshalb auf eine reine Vanilla-Klasse zeigen. Geprueft wird nur,
     * dass es sie in CfgVehicles ueberhaupt gibt.
     *
     * @return false, wenn der Record abgewiesen wurde.
     */
    private bool Register(notnull ChefZ_NutritionDef def, ChefZ_LoadReport report)
    {
        if (!ChefZ_SymbolTable.IsValid(def.sym))
        {
            Report(report, true, def, "Der Record hat kein gueltiges Symbol - er wurde nie " + "kompiliert. Das ist ein Fehler im Ladeweg, nicht in den Daten.");
            return false;
        }

        switch (def.scopeKind)
        {
            case ChefZ_NutritionScope.CLASS:    return RegisterClass(def, report);
            case ChefZ_NutritionScope.CATEGORY: return RegisterCategory(def, report);
            case ChefZ_NutritionScope.TAG:      return RegisterTag(def, report);
        }

        // Unerreichbar: ChefZ_NutritionDef.Validate weist einen unbekannten
        // scope bereits ab. Die Zeile steht trotzdem, weil ein spaeter
        // ergaenzter scope sonst still in gar keine Tabelle fiele.
        Report(report, true, def, "scope \"" + def.scope + "\" ist dem Nutrition Manager " + "unbekannt - der Record wirkt nicht. Gueltig: " + ChefZ_NutritionScope.ValidNames() + ".");
        return false;
    }

    private bool RegisterClass(notnull ChefZ_NutritionDef def, ChefZ_LoadReport report)
    {
        if (m_ByClass.Contains(def.sym))
        {
            Report(report, true, def, "Zu dieser Klasse gibt es bereits eine Naehrwertangabe - " + "der zweite Record wird abgewiesen. Ein Overlay soll den ersten PATCHEN " + "(gleiche id), nicht danebenstehen (02 E3).");
            return false;
        }

        // Nur eine Warnung, kein Abbruch: die Klasse kann aus einem Modul
        // stammen, das auf diesem Server nicht geladen ist. Dann wirkt der
        // Record nie - aber er macht auch nichts kaputt, und ein abgewiesener
        // Record waere beim naechsten Serverstart mit dem Modul wieder da.
        if (g_Game && !ChefZ_VanillaNutrition.ClassExists(def.id))
        {
            Report(report, false, def, "Die Klasse \"" + def.id + "\" steht nicht in " + "CfgVehicles. Der Record bleibt geladen und wirkt, sobald es sie gibt - " + "bis dahin taucht er in keiner Sollrechnung auf. Fehlt ein Content-Modul, " + "oder ist der Name falsch geschrieben?");
        }

        m_ByClass.Set(def.sym, def);
        return true;
    }

    private bool RegisterCategory(notnull ChefZ_NutritionDef def, ChefZ_LoadReport report)
    {
        ChefZ_CategoryManager cats = Cats();
        if (!cats || !cats.IsReady())
        {
            Report(report, true, def, "scope \"category\", aber der Kategoriebaum ist nicht " + "gebaut - der Record wird abgewiesen. Das ist ein Fehler in der " + "Baureihenfolge, nicht in den Daten.");
            return false;
        }

        int bit = cats.GetBitIndex(def.sym);
        if (bit < 0)
        {
            Report(report, true, def, "scope \"category\", aber \"" + def.id + "\" ist keine " + "bekannte Kategorie - der Record wird abgewiesen. Er wuerde sonst auf " + "nichts passen und dabei aussehen wie eine gepflegte Angabe.");
            return false;
        }

        m_CategoryDefs.Insert(def);
        m_CategoryBits.Insert(bit);
        m_CategoryDepth.Insert(cats.GetDepth(def.sym));
        return true;
    }

    private bool RegisterTag(notnull ChefZ_NutritionDef def, ChefZ_LoadReport report)
    {
        ChefZ_CategoryManager cats = Cats();
        if (cats && cats.IsReady() && !cats.TagExists(def.sym))
        {
            Report(report, true, def, "scope \"tag\", aber \"" + def.id + "\" ist kein " + "bekannter Tag - der Record wird abgewiesen. Er wuerde sonst auf nichts " + "passen und dabei aussehen wie eine gepflegte Angabe.");
            return false;
        }

        if (m_ByTag.Contains(def.sym))
        {
            Report(report, true, def, "Zu diesem Tag gibt es bereits eine Naehrwertangabe - " + "der zweite Record wird abgewiesen (02 E3).");
            return false;
        }

        m_ByTag.Set(def.sym, def);
        return true;
    }

    //==========================================================================
    // Auffindung (13 E4: Klasse -> Kategorie -> Tag -> Vanilla)
    //==========================================================================

    /**
     * Der Basisvektor einer Klasse in einer Garstufe (13 §4).
     *
     * Reihenfolge, und jede Stufe ist begruendet (13 E4):
     *
     *   1. KLASSE     Wer eine einzelne Klasse belegt, meint genau sie.
     *   2. KATEGORIE  Ein Autor belegt eine Oberkategorie EINMAL statt jede
     *                 Sorte einzeln. Bei mehreren passenden Kategorien gewinnt
     *                 die TIEFSTE - die speziellere Aussage schlaegt die
     *                 allgemeinere, genau wie im Kategoriebaum ueberall sonst.
     *   3. TAG        Dasselbe quer zum Baum.
     *   4. VANILLA    CfgVehicles. Damit ist JEDES Item ohne jede
     *                 ChefZ-Deklaration als Zutat auditierbar und bringt seine
     *                 ECHTEN Werte mit.
     *
     * Wichtig zur Garstufe: ein ChefZ-Record kennt keine Stufendimension. Er
     * gilt fuer alle Stufen gleich. Wer eine Zutat je Garstufe unterschiedlich
     * bewerten will, pflegt sie in CfgVehicles - dort, wo die Engine sie
     * ohnehin liest, und wo sie zusaetzlich WIRKT. Ein zweiter Stufenbegriff
     * im ChefZ-Record waere eine Zahl, die nur der Audit sieht.
     *
     * @return false, wenn nirgends Daten stehen. outVec ist dann NULLVEKTOR
     *         und nicht undefiniert - der Aufrufer darf ihn bedenkenlos
     *         weiterrechnen und meldet die Luecke separat (13 §8).
     */
    bool ReadBase(ChefZ_Sym classSym, int vanillaStage, out ChefZ_NutritionVector outVec)
    {
        if (!outVec)
            outVec = new ChefZ_NutritionVector();
        outVec.Reset();

        if (!ChefZ_SymbolTable.IsValid(classSym))
            return false;

        ChefZ_NutritionDef def = FindDefForClass(classSym);
        if (def && def.HasAnyValue())
        {
            def.FillVector(outVec);
            return true;
        }

        // Vanilla-Rueckfall. Er steht bewusst NACH der ganzen ChefZ-Kette und
        // nicht davor: ein Autor, der einen Record schreibt, will ihn wirksam
        // sehen - auch dann, wenn die Klasse in CfgVehicles Werte traegt.
        if (!g_Game)
            return false;

        return ChefZ_VanillaNutrition.Read(ChefZ_SymbolTable.Name(classSym), vanillaStage, outVec);
    }

    /**
     * Der ChefZ-Record, der fuer diese Klasse gilt - Klasse, sonst Kategorie,
     * sonst Tag. null, wenn keiner passt.
     *
     * Kategorie und Tag brauchen die Bindungsdaten der Klasse, und die kommen
     * vom ChefZ_IngredientManager. Ist die Klasse dort unbekannt, gibt es
     * weder Closure noch Tags - dann greift nur der Klassenrecord, und das ist
     * richtig so: eine nicht deklarierte Klasse gehoert in keine ChefZ-
     * Kategorie.
     */
    ChefZ_NutritionDef FindDefForClass(ChefZ_Sym classSym)
    {
        if (!ChefZ_SymbolTable.IsValid(classSym))
            return null;

        ChefZ_NutritionDef def;
        if (m_ByClass.Find(classSym, def) && def)
            return def;

        ChefZ_IngredientManager ing = Ingredients();
        if (!ing || !ing.IsReady())
            return null;

        ChefZ_IngredientInfo info = ing.Resolve(classSym);
        if (!info)
            return null;

        ChefZ_NutritionDef byCategory = DeepestCategoryDef(info.closure);
        if (byCategory)
            return byCategory;

        return FirstTagDef(info.staticTags);
    }

    /**
     * Der Kategorierecord mit der GROESSTEN Tiefe, dessen Kategorie in der
     * Vorfahren-Closure der Klasse liegt.
     *
     * Warum die tiefste und nicht das Produkt aller: Naehrwerte sind absolute
     * Zahlen und keine Faktoren. Zwei passende Kategorien zu ADDIEREN ergaebe
     * eine Zutat, die doppelt so nahrhaft ist, weil jemand sie doppelt
     * eingeordnet hat - das waere ein Balancingfehler, den die Datenstruktur
     * erzeugt und niemand geschrieben hat. (Beim Verderbfaktor ist das anders
     * und dort ausdruecklich gewollt: Faktoren multiplizieren sich sinnvoll,
     * Naehrwerte addieren sich nicht.)
     *
     * Gleichstand bei der Tiefe entscheidet die Aufnahmereihenfolge, und die
     * ist nach ID sortiert - also auf jedem Server dieselbe.
     */
    private ChefZ_NutritionDef DeepestCategoryDef(ChefZ_CategoryClosure closure)
    {
        if (!closure || m_CategoryDefs.Count() == 0)
            return null;

        ChefZ_NutritionDef best;
        int bestDepth = -1;

        for (int i = 0; i < m_CategoryDefs.Count(); i++)
        {
            if (!closure.HasBit(m_CategoryBits.Get(i)))
                continue;

            int depth = m_CategoryDepth.Get(i);
            if (depth > bestDepth)
            {
                bestDepth = depth;
                best      = m_CategoryDefs.Get(i);
            }
        }
        return best;
    }

    //! Der erste passende Tagrecord in der Tagliste der Klasse. Die Liste ist
    //! selbst nach Aufnahmereihenfolge bestimmt (05 E4), also ist auch diese
    //! Wahl auf jedem Server dieselbe.
    private ChefZ_NutritionDef FirstTagDef(array<ChefZ_Sym> tags)
    {
        if (!tags || m_ByTag.Count() == 0)
            return null;

        for (int i = 0; i < tags.Count(); i++)
        {
            ChefZ_NutritionDef def;
            if (m_ByTag.Find(tags.Get(i), def) && def)
                return def;
        }
        return null;
    }

    //==========================================================================
    // Die Sollrechnung (13 §5)
    //==========================================================================

    /**
     * Der Sollwert eines Rezeptergebnisses aus den TATSAECHLICH verbrauchten
     * Zutaten (13 §4).
     *
     * 13 §5 bildet Architekturplan §10 direkt ab:
     *
     *     Sausage         450
     *     Pasta           500
     *     Tomato Sauce    100
     *     --------------------
     *     Basis          1050
     *     x nutritionModifier  1.10
     *     = Sollwert     1155
     *
     * DER SOLLWERT WIRD NICHT ANGEWANDT. Diese Methode veraendert nichts - sie
     * liest den Verbrauchsplan, addiert und gibt eine Zahl zurueck. Wer sie
     * aufruft, bekommt eine Diagnose, keine Wirkung (13 E1).
     *
     * Grundlage ist der VERBRAUCHSPLAN und nicht die Slotbelegung: verbraucht
     * wird, was im Plan steht, und nur das gehoert in die Sollrechnung. Ein
     * Slot, der ein Werkzeug bindet, ohne es aufzubrauchen, traegt zum
     * Naehrwert des Gerichts nichts bei - und stuende er in der Rechnung, waere
     * jeder Topf mit einem mitgekochten Messer nahrhafter.
     *
     * @param trace Rechenweg, Zeile fuer Zeile. Darf null sein; wird sonst
     *              GELEERT und neu gefuellt. Nur fuer Diagnose (13 §7).
     */
    void ComputeExpected(notnull ChefZ_CompiledRecipe recipe, notnull ChefZ_MatchResult match, notnull ChefZ_FactSnapshot snapshot, out ChefZ_NutritionVector outExpected, out array<string> trace)
    {
        if (!outExpected)
            outExpected = new ChefZ_NutritionVector();
        outExpected.Reset();

        if (trace)
            trace.Clear();

        for (int i = 0; i < match.consumePlan.Count(); i++)
        {
            ChefZ_ConsumePlan plan = match.consumePlan.Get(i);
            if (!plan)
                continue;

            ChefZ_ItemFacts facts = snapshot.FindByHandle(plan.handle);
            if (!facts)
            {
                // Der Plan zeigt auf ein Item, das nicht in der Faktenliste
                // steht. Im Kochpfad kann das nicht vorkommen (beide stammen
                // aus derselben Auswertung); es waere ein Programmfehler und
                // keine Datenlage. Er wird deshalb notiert und uebersprungen -
                // eine halbe Sollrechnung ist immer noch eine Diagnose.
                Trace(trace, "Handle #" + plan.handle.ToString() + " hat keine Fakten - uebersprungen");
                continue;
            }

            AddIngredient(facts.classSym, ChefZ_NutritionScope.CLASS, facts.vanillaFoodStage, FactorFor(plan, facts), outExpected, trace);
        }

        ApplyModifier(recipe, outExpected, trace);
        Finish(recipe.id, outExpected, trace);
    }

    /**
     * Wieviel EINES Items geht in die Sollrechnung ein?
     *
     * Drei Faelle, und die Reihenfolge ist die Reihenfolge ihrer Genauigkeit:
     *
     *   perUnit-Record   die Werte gelten je Rezepteinheit -> die Zahl der
     *                    verbrauchten Einheiten ist der Faktor.
     *   ganzes Item      1.0. Der Record (oder CfgVehicles) beschreibt genau
     *                    dieses Stueck.
     *   Teilverbrauch    der Anteil an der VOLLEN Menge des Items. Ein halb
     *                    verbrauchter Topf Sauce traegt die Haelfte bei.
     *
     * Der Teilverbrauch rechnet gegen quantityMax und nicht gegen die aktuelle
     * Menge: die Vanilla-Werte gelten fuer ein volles Item (PlayerStomach
     * teilt die Energie durch 100 Mengeneinheiten). Gegen die Restmenge
     * gerechnet, wuerde ein fast leerer Topf denselben Beitrag liefern wie ein
     * voller.
     */
    private float FactorFor(notnull ChefZ_ConsumePlan plan, notnull ChefZ_ItemFacts facts)
    {
        ChefZ_NutritionDef def = FindDefForClass(facts.classSym);
        if (def && def.perUnit)
        {
            if (plan.unitsDelta > 0.0)
                return plan.unitsDelta;
            if (plan.destroyWhole && facts.units > 0.0)
                return facts.units;
            return 1.0;
        }

        if (plan.destroyWhole)
            return 1.0;

        if (plan.quantityDelta > 0.0 && facts.quantityMax > 0.0)
            return plan.quantityDelta / facts.quantityMax;

        // Kein Verbrauch geplant und nichts zerstoert: das Item bleibt liegen
        // und traegt nichts bei.
        return 0.0;
    }

    /**
     * Eine Zutat in die Summe aufnehmen - oder ihre Luecke notieren.
     *
     * 13 §8: "Zutat ohne Naehrwertdaten -> zaehlt im Audit mit 0 und wird
     * NAMENTLICH im Befund genannt, damit die Luecke sichtbar ist. Kein
     * Laufzeiteffekt."
     *
     * @return false, wenn die Zutat keine Daten hatte (der Aufrufer macht
     *         daraus im Audit einen ZERO_INGREDIENT-Befund).
     */
    private bool AddIngredient(ChefZ_Sym sym, int scopeKind, int stage, float factor, notnull ChefZ_NutritionVector sum, array<string> trace)
    {
        string name = ChefZ_SymbolTable.NameOrMark(sym);

        // Ueber eine lokale Zwischenvariable: m_ScratchBase ist ein FELD, und
        // ein Feld als out-Parameter ist in Enforce nicht zugesichert
        // (dieselbe Vorsicht wie im ChefZ_CookingDeviceAdapter).
        ChefZ_NutritionVector baseVec = m_ScratchBase;
        bool found = ReadBaseForScope(sym, scopeKind, stage, baseVec);
        m_ScratchBase = baseVec;

        if (!found || baseVec.IsZero())
        {
            Trace(trace, name + " x" + factor.ToString() + ": keine Naehrwertdaten, zaehlt 0");
            return false;
        }

        sum.AddScaled(baseVec, factor);
        Trace(trace, name + " x" + factor.ToString() + ": energie " + baseVec.energy.ToString() + " -> summe " + sum.energy.ToString());
        return true;
    }

    /**
     * Basiswerte zu einem Symbol EINER BESTIMMTEN Dimension.
     *
     * Der Unterschied zu ReadBase ist wichtig und leicht zu uebersehen:
     * ReadBase bekommt eine KLASSE und sucht ueber sie die passende Kategorie
     * und den passenden Tag. Diese Methode bekommt das Symbol bereits mit
     * seiner Dimension - denn der Startaudit hat keine Klasse, sondern einen
     * Slot, und ein Slot fordert oft ausdruecklich eine Kategorie.
     *
     * Ohne sie liefe ein Slot "irgendetwas aus Kategorie X" im Audit ins Leere:
     * ReadBase suchte den Kategorienamen als Klasse in CfgVehicles, faende ihn
     * nicht, und das Rezept saehe aus, als haetten seine Zutaten keine
     * Naehrwerte.
     */
    private bool ReadBaseForScope(ChefZ_Sym sym, int scopeKind, int stage, out ChefZ_NutritionVector outVec)
    {
        if (scopeKind == ChefZ_NutritionScope.CLASS)
            return ReadBase(sym, stage, outVec);

        if (!outVec)
            outVec = new ChefZ_NutritionVector();
        outVec.Reset();

        ChefZ_NutritionDef def = DefForScope(sym, scopeKind);
        if (!def || !def.HasAnyValue())
            return false;

        def.FillVector(outVec);
        return true;
    }

    //! Der Record einer bestimmten Dimension. Fuer CLASS die volle
    //! Auffindungskette, fuer die beiden anderen ein direkter Griff.
    private ChefZ_NutritionDef DefForScope(ChefZ_Sym sym, int scopeKind)
    {
        if (scopeKind == ChefZ_NutritionScope.CLASS)
            return FindDefForClass(sym);

        ChefZ_NutritionDef def;
        if (scopeKind == ChefZ_NutritionScope.TAG)
        {
            if (m_ByTag.Find(sym, def) && def)
                return def;
            return null;
        }

        if (scopeKind == ChefZ_NutritionScope.CATEGORY)
        {
            for (int i = 0; i < m_CategoryDefs.Count(); i++)
            {
                ChefZ_NutritionDef cat = m_CategoryDefs.Get(i);
                if (cat && cat.sym == sym)
                    return cat;
            }
        }
        return null;
    }

    /**
     * 13 §8: "nutritionModifier <= 0 -> Auf 1.0 gesetzt, WARN mit Rezept-ID."
     *
     * @return true, wenn der Modifikator geklammert werden musste.
     */
    private bool ApplyModifier(notnull ChefZ_CompiledRecipe recipe, notnull ChefZ_NutritionVector sum, array<string> trace)
    {
        float modifier = recipe.nutritionModifier;
        bool clamped   = false;

        if (!(modifier > 0.0))
        {
            // Faengt zugleich NaN: der Vergleich ist dann false. Ein
            // Modifikator von 0 hiesse "das Gericht hat keinen Naehrwert",
            // und das schreibt niemand absichtlich in ein Rezept.
            modifier = 1.0;
            clamped  = true;
            Trace(trace, "nutritionModifier war nicht positiv - es gilt 1.0");
        }

        if (modifier != 1.0)
        {
            sum.Scale(modifier);
            Trace(trace, "x nutritionModifier " + modifier.ToString() + " -> energie " + sum.energy.ToString());
        }

        return clamped;
    }

    /**
     * Der Abschluss jeder Sollrechnung: pruefen und klemmen.
     *
     * 13 §8: "Ueberlauf oder NaN -> IsFinite() schlaegt an, Vektor verworfen,
     * ERROR" und "Sollwert ueberschreitet einen Deckel -> Geklemmt, INFO".
     *
     * @return 0 = in Ordnung, 1 = geklemmt, 2 = nicht berechenbar.
     */
    static const int FINISH_OK      = 0;
    static const int FINISH_CLAMPED = 1;
    static const int FINISH_BROKEN  = 2;

    private int Finish(string recipeId, notnull ChefZ_NutritionVector sum, array<string> trace)
    {
        if (!sum.IsFinite())
        {
            // Verworfen und NICHT geklemmt: eine geklemmte Fantasiezahl saehe
            // aus wie ein Balancingergebnis. Der Nullvektor sieht aus wie das,
            // was er ist - eine Rechnung, die nicht aufging.
            sum.Reset();
            Trace(trace, "Sollrechnung nicht berechenbar (Ueberlauf oder NaN) - verworfen");
            return FINISH_BROKEN;
        }

        if (sum.ClampTo(m_Caps, m_Floors))
        {
            Trace(trace, "Sollwert in die Sondengrenze " + m_ExpectedCap.ToString() + " geklemmt - siehe " + recipeId);
            return FINISH_CLAMPED;
        }

        return FINISH_OK;
    }

    //==========================================================================
    // Der Startaudit (13 §5) - der eigentliche Nutzen
    //==========================================================================

    /**
     * Soll gegen Ist, ueber ALLE Rezepte. Aendert NICHTS (13 §4).
     *
     * 13 E5 begruendet den Zeitpunkt: "Beim Kochen waere es billiger zu
     * implementieren, aber der Befund kaeme erst, wenn jemand das Gericht
     * kocht - und bei einem Gericht mit fehlendem Nutrition-Block kaeme er
     * nie, weil niemand merkt, dass er nicht satt wird. Ein vollstaendiger
     * Startaudit findet alle Loecher sofort und kostet einmalig ein paar
     * Millisekunden."
     *
     * Die Sollbelegung ist die TYPISCHE: je Pflichtslot ein Vertreter, in der
     * geforderten Mindestzahl. Das ist bewusst nicht dasselbe wie ein echter
     * Kochvorgang - es kann keiner sein, weil beim Boot kein Topf steht. Was
     * es leistet, ist eine vergleichbare Groessenordnung ueber alle Rezepte
     * hinweg, und genau die braucht der Balance-Reviewer.
     *
     * @param outFindings wird geleert und gefuellt. Die Liste gehoert danach
     *                    dem Aufrufer; der Manager haelt eine eigene Kopie fuer
     *                    das Adminkommando (13 §7).
     */
    void AuditAllRecipes(out array<ChefZ_NutritionFinding> outFindings)
    {
        if (!outFindings)
            outFindings = new array<ChefZ_NutritionFinding>();
        outFindings.Clear();

        m_LastFindings.Clear();
        m_LastAuditedRecipes = 0;
        m_LastErrorCount     = 0;
        m_AuditDone          = false;

        if (!m_AuditEnabled)
        {
            // 13 §8, letzte Zeile. Eine Zeile bleibt trotzdem stehen: ein
            // Betreiber soll im Log sehen koennen, dass der Audit NICHT
            // gelaufen ist - sonst hielte er ein leeres Ergebnis fuer ein
            // gutes.
            Banner("Naehrwertaudit ist abgeschaltet (enableNutritionAudit = false). " + "Ein Gericht ohne \"class Nutrition\" faellt damit erst dem Spieler auf, " + "der nicht satt wird (01 V7).");
            return;
        }

        ChefZ_RecipeEngine engine = Engine();
        if (!engine || !engine.IsReady())
        {
            Banner("Naehrwertaudit uebersprungen: die Rezept-Engine ist nicht gebaut. " + "Kochen und Essen sind davon unberuehrt.");
            return;
        }

        array<string> trace = new array<string>();

        for (int i = 0; i < engine.GetRecipeCount(); i++)
        {
            ChefZ_CompiledRecipe recipe = engine.GetRecipeAt(i);
            if (!recipe)
                continue;

            m_LastAuditedRecipes++;
            AuditRecipe(recipe, trace, outFindings);
        }

        m_AuditDone = true;
        ReportFindings(outFindings);
    }

    /**
     * Ein Rezept: Sollwert bilden, Ergebnisklassen pruefen, vergleichen.
     */
    private void AuditRecipe(notnull ChefZ_CompiledRecipe recipe, notnull array<string> trace, notnull array<ChefZ_NutritionFinding> outFindings)
    {
        ChefZ_NutritionVector expected = new ChefZ_NutritionVector();
        trace.Clear();

        // --- Sollwert aus der typischen Belegung --------------------------
        array<string> gaps = new array<string>();
        int status = BuildTypicalExpected(recipe, expected, trace, gaps);

        for (int g = 0; g < gaps.Count(); g++)
        {
            ChefZ_NutritionFinding zero = new ChefZ_NutritionFinding();
            zero.Init(ChefZ_NutritionFindingKind.ZERO_INGREDIENT, recipe.id, gaps.Get(g), "Zutat \"" + gaps.Get(g) + "\" hat keine Naehrwertdaten und zaehlt mit 0 in " + "die Sollrechnung. Das ist kein Laufzeitfehler - die Sollzahl dieses " + "Rezepts ist dadurch aber zu niedrig und als Balancinghinweis nur " + "eingeschraenkt brauchbar.");
            Emit(zero, outFindings);
        }

        if (ApplyModifierWasClamped(recipe))
        {
            ChefZ_NutritionFinding bad = new ChefZ_NutritionFinding();
            bad.Init(ChefZ_NutritionFindingKind.BAD_MODIFIER, recipe.id, "", "nutritionModifier ist " + recipe.nutritionModifier.ToString() + " und damit nicht positiv. Fuer die Sollrechnung gilt 1.0. Ein Wert von 0 " + "hiesse \"das Gericht hat keinen Naehrwert\", und das schreibt niemand " + "absichtlich.");
            Emit(bad, outFindings);
        }

        if (status == FINISH_BROKEN)
        {
            ChefZ_NutritionFinding broken = new ChefZ_NutritionFinding();
            broken.Init(ChefZ_NutritionFindingKind.NOT_COMPUTABLE, recipe.id, "", "Die Sollrechnung ist nicht berechenbar (Ueberlauf oder NaN). Es wird " + "bewusst KEINE Zahl gemeldet - eine Fantasiezahl im Startlog waere " + "schlimmer als keine.");
            Emit(broken, outFindings);

            // KEIN return. Die Pruefung der Ergebnisklassen laeuft weiter, und
            // zwar ausdruecklich: sie ist der eine Befund, der ein Gericht
            // lautlos unbrauchbar macht (01 V7), und sie haengt nicht an der
            // Sollrechnung. Ein Rezept wegen einer entgleisten Zahl nicht mehr
            // auf den Nutrition-Block zu pruefen, hiesse den wichtigen Befund
            // wegen des unwichtigen zu verlieren.
            //
            // Der Zahlenvergleich weiter unten faellt von selbst aus: Finish()
            // hat den Vektor bei FINISH_BROKEN geleert, und AuditOutputClass
            // vergleicht nur gegen einen Sollwert groesser null.
        }

        if (status == FINISH_CLAMPED)
        {
            // 13 §8, Deckelzeile: INFO, kein Fehler. Es ist ein
            // Balancinghinweis fuer den Reviewer - und ein Hinweis darauf,
            // dass die Sollrechnung dieses Rezepts mit Vorsicht zu lesen ist.
            ChefZ_NutritionFinding cl = new ChefZ_NutritionFinding();
            cl.Init(ChefZ_NutritionFindingKind.CLAMPED, recipe.id, "", "Der Sollwert lief in die Sondengrenze (" + Rounded(m_ExpectedCap) + ") und wurde geklemmt. Das ist KEIN Balancingdeckel (13 E6) - die " + "tatsaechlichen Werte in CfgVehicles sind unberuehrt. Der Sollwert " + "dieses Rezepts ist als Vergleichszahl nur eingeschraenkt brauchbar.");
            cl.expectedEnergy = expected.energy;
            Emit(cl, outFindings);
        }

        int stage = AuditStageOf(recipe);

        // --- Je Ergebnisklasse: V7-Pruefung und Vergleich ------------------
        for (int o = 0; o < recipe.outputs.Count(); o++)
        {
            ChefZ_OutputDef def = recipe.outputs.Get(o);
            if (!def || def.cls == "")
                continue;

            AuditOutputClass(recipe, def.cls, stage, expected, outFindings);

            if (!def.variants)
                continue;
            for (int v = 0; v < def.variants.Count(); v++)
            {
                ChefZ_OutputVariant variant = def.variants.Get(v);
                if (variant && variant.cls != "")
                    AuditOutputClass(recipe, variant.cls, stage, expected, outFindings);
            }
        }

        // Nebenprodukte werden auf die V7-Bedingung geprueft, aber NICHT gegen
        // den Sollwert gehalten: der Sollwert gehoert dem Hauptergebnis. Ein
        // Knochen, der beim Zerlegen abfaellt, soll nicht so nahrhaft sein wie
        // das Gericht - und eine Warnung, die das anmahnt, waere Unsinn.
        for (int b = 0; b < recipe.byproducts.Count(); b++)
        {
            ChefZ_OutputDef by = recipe.byproducts.Get(b);
            if (by && by.cls != "")
                CheckStomachRegistration(recipe.id, by.cls, outFindings);
        }
    }

    private void AuditOutputClass(notnull ChefZ_CompiledRecipe recipe, string cls, int stage, notnull ChefZ_NutritionVector expected, notnull array<ChefZ_NutritionFinding> outFindings)
    {
        if (!CheckStomachRegistration(recipe.id, cls, outFindings))
            return;

        // Kein Vergleich, wenn die Sollrechnung leer ist: "0 erwartet, 800
        // konfiguriert" ist keine Aussage ueber Balancing, sondern eine ueber
        // fehlende Zutatendaten - und die steht bereits als ZERO_INGREDIENT da.
        if (expected.energy <= 0.0)
            return;

        m_ScratchActual.Reset();
        if (!ChefZ_VanillaNutrition.Read(cls, stage, m_ScratchActual))
            return;

        float actual    = m_ScratchActual.energy;
        float deviation = (actual - expected.energy) / expected.energy * 100.0;
        float magnitude = deviation;
        if (magnitude < 0.0)
            magnitude = -magnitude;

        if (magnitude <= m_TolerancePct)
            return;

        ChefZ_NutritionFinding f = new ChefZ_NutritionFinding();
        f.Init(ChefZ_NutritionFindingKind.DEVIATION, recipe.id, cls, "erwartet " + Rounded(expected.energy) + " energy aus den Zutaten, " + "CfgVehicles sagt " + Rounded(actual) + " (" + Signed(deviation) + "%). " + "KEINE Korrektur - der Core aendert nie einen Balancingwert (13 E1).");
        f.expectedEnergy = expected.energy;
        f.actualEnergy   = actual;
        f.deviationPct   = deviation;
        Emit(f, outFindings);
    }

    /**
     * Die V7-Pruefung: wuerde der Magen diese Klasse ueberhaupt registrieren?
     *
     * Das ist die wichtigste Zeile des ganzen Teilsystems. 13 §3: "Ein Gericht
     * ohne Nutrition-Block wird gegessen, verschwindet aus dem Inventar und
     * saettigt nichts. Es gibt keine Fehlermeldung, keinen Log-Eintrag, keinen
     * Hinweis. Das ist der leiseste denkbare Content-Fehler und deshalb der
     * gefaehrlichste."
     *
     * Nicht essbare Ergebnisklassen sind ausgenommen - eine leere Konserve
     * oder ein Behaelter braucht keinen Nutrition-Block, und ihn zu verlangen
     * waere eine Warnung ohne Fehler.
     *
     * @return true, wenn die Klasse als Nahrung taugt (oder gar keine sein
     *         will). false heisst: ein Befund steht in der Liste.
     */
    private bool CheckStomachRegistration(string recipeId, string cls, notnull array<ChefZ_NutritionFinding> outFindings)
    {
        if (!g_Game)
            return false;
        if (!ChefZ_VanillaNutrition.ClassExists(cls))
            return false;           // meldet bereits der ChefZ_RecipeCompiler (08 §8)
        if (!ChefZ_VanillaNutrition.IsEdible(cls))
            return false;           // kein Nahrungsmittel - nichts zu pruefen

        string reason;
        if (ChefZ_VanillaNutrition.WouldRegisterAtStomach(cls, reason))
            return true;

        string kind = ChefZ_NutritionFindingKind.MISSING_BLOCK;
        string what = "hat weder \"class Nutrition\" noch \"class Food\"";
        if (ChefZ_VanillaNutrition.ScopeOf(cls) == 0)
        {
            kind = ChefZ_NutritionFindingKind.SCOPE_ZERO;
            what = "hat scope = 0";
        }

        ChefZ_NutritionFinding f = new ChefZ_NutritionFinding();
        f.Init(kind, recipeId, cls, "Ergebnisklasse \"" + cls + "\" " + what + " -> PlayerStomach.InitData registriert sie nicht, AddToStomach bricht ohne " + "Meldung ab. Das Gericht wird gegessen, verschwindet und SAETTIGT NICHT " + "(01 V7). Das Rezept wird beim Build abgewiesen (08 §8).");
        Emit(f, outFindings);
        return false;
    }

    /**
     * Der Sollwert aus der TYPISCHEN Belegung der Pflichtslots (13 §6).
     *
     * Eine eigene Rechnung neben ComputeExpected und ausdruecklich keine
     * Wiederverwendung: ComputeExpected braucht einen Verbrauchsplan und eine
     * Faktenliste, und beides entsteht erst im Kochtick. Beim Boot gibt es
     * keinen Topf. Einen nachzubauen hiesse, den Matcher gegen erfundene Items
     * laufen zu lassen - und dann prueft der Audit den Nachbau statt der
     * Rezepte.
     *
     * Der Vertreter eines Slots ist das erste CLASS-, sonst CATEGORY-, sonst
     * TAG-Blatt seines Selektors. Ein Slot, dessen Selektor keines davon hat
     * (reine Wertebereiche, reine Zustandsforderung), traegt nichts bei und
     * wird als Luecke gemeldet - denn genau dann kann niemand sagen, was da
     * hineingehoert.
     *
     * @param outGaps Namen der Zutaten ohne Daten, fuer ZERO_INGREDIENT.
     */
    private int BuildTypicalExpected(notnull ChefZ_CompiledRecipe recipe, notnull ChefZ_NutritionVector outExpected, notnull array<string> trace, notnull array<string> outGaps)
    {
        outExpected.Reset();

        for (int i = 0; i < recipe.slots.Count(); i++)
        {
            ChefZ_CompiledSlot slot = recipe.slots.Get(i);
            if (!slot || !ChefZ_CompiledRecipe.IsRequiredSlot(slot))
                continue;

            // Klasse vor Kategorie vor Tag - dieselbe Vorrangregel wie in der
            // Auffindung (13 E4). Ein Slot, der eine konkrete Klasse fordert,
            // soll im Audit auch mit deren Zahlen gerechnet werden, selbst
            // wenn er zusaetzlich eine Kategorie nennt.
            int scopeKind = ChefZ_NutritionScope.CLASS;
            ChefZ_Sym rep = FindLeaf(slot.selector, ChefZ_SelectorOp.CLASS, 0);

            if (!ChefZ_SymbolTable.IsValid(rep))
            {
                rep       = FindLeaf(slot.selector, ChefZ_SelectorOp.CATEGORY, 0);
                scopeKind = ChefZ_NutritionScope.CATEGORY;
            }
            if (!ChefZ_SymbolTable.IsValid(rep))
            {
                rep       = FindLeaf(slot.selector, ChefZ_SelectorOp.TAG, 0);
                scopeKind = ChefZ_NutritionScope.TAG;
            }

            if (!ChefZ_SymbolTable.IsValid(rep))
            {
                string label = slot.slotId;
                if (label == "")
                    label = "#" + slot.slotIndex.ToString();
                if (outGaps.Find(label) < 0)
                    outGaps.Insert(label);
                Trace(trace, "Slot " + label + ": kein benennbarer Vertreter - zaehlt 0");
                continue;
            }

            float factor = TypicalFactor(slot, rep, scopeKind);
            if (!AddIngredient(rep, scopeKind, ChefZ_VanillaStage.RAW, factor, outExpected, trace))
            {
                string name = ChefZ_SymbolTable.NameOrMark(rep);
                if (outGaps.Find(name) < 0)
                    outGaps.Insert(name);
            }
        }

        ApplyModifier(recipe, outExpected, trace);
        return Finish(recipe.id, outExpected, trace);
    }

    /**
     * Der Mengenfaktor der typischen Belegung.
     *
     * minCount ganze Stuecke, oder - bei einem perUnit-Record - die geforderte
     * Mindestmenge in Rezepteinheiten. Ohne Mengenforderung ist eine Einheit
     * je Stueck die einzige Annahme, die nicht geraten ist.
     */
    private float TypicalFactor(notnull ChefZ_CompiledSlot slot, ChefZ_Sym rep, int scopeKind)
    {
        float count = slot.minCount;
        if (count < 1.0)
            count = 1.0;

        ChefZ_NutritionDef def = DefForScope(rep, scopeKind);
        if (def && def.perUnit)
        {
            float units = slot.RequiredUnits();
            if (units > 0.0)
                return units;
            return count;
        }

        return count;
    }

    /**
     * Das erste Blatt EINER BESTIMMTEN Art im Selektorbaum.
     *
     * Bei ANY_OF und ALL_OF gewinnt das erste Kind, das etwas liefert - in
     * DEKLARATIONSreihenfolge, damit der Audit auf jedem Server dieselbe Zahl
     * ausgibt.
     *
     * NOT wird ausdruecklich NICHT betreten: was ein Slot AUSSCHLIESST, sagt
     * nichts darueber, was er aufnimmt. Ein "nicht verbrannt" als Vertreter
     * eines Slots waere die falscheste denkbare Antwort - der Audit rechnete
     * dann mit den Naehrwerten genau der Zutat, die das Rezept verbietet.
     *
     * Die Tiefenbremse ist dieselbe wie im Selektorcompiler und hat denselben
     * Zweck: kein Selbstverweis kann diesen Aufruf beim Serverstart haengen
     * lassen.
     */
    private ChefZ_Sym FindLeaf(ChefZ_CompiledSelector selector, ChefZ_SelectorOp wanted, int depth)
    {
        if (!selector || depth > 16)
            return ChefZ_SymbolTable.INVALID;

        if (selector.op == wanted)
            return selector.sym;

        if (selector.op == ChefZ_SelectorOp.ANY_OF || selector.op == ChefZ_SelectorOp.ALL_OF)
        {
            if (!selector.children)
                return ChefZ_SymbolTable.INVALID;

            for (int i = 0; i < selector.children.Count(); i++)
            {
                ChefZ_Sym found = FindLeaf(selector.children.Get(i), wanted, depth + 1);
                if (ChefZ_SymbolTable.IsValid(found))
                    return found;
            }
        }

        return ChefZ_SymbolTable.INVALID;
    }

    /**
     * Welche Garstufe legt der Audit fuer das Ergebnis zugrunde?
     *
     * Die erste Abschlussstufe des Rezepts, sonst RAW. Der Grund: ein Gericht,
     * das bei "Baked" fertig ist, wird in dieser Stufe gegessen, und genau
     * deren nutrition_properties liest der Magen. Gegen die Rohwerte zu
     * pruefen ergaebe fuer jedes gebackene Gericht eine Abweichung, die keine
     * ist.
     *
     * Traegt die Klasse gar keine FoodStages, faellt ChefZ_VanillaNutrition
     * ohnehin auf den flachen Nutrition-Block zurueck - die Stufe ist dann
     * bedeutungslos.
     */
    private int AuditStageOf(notnull ChefZ_CompiledRecipe recipe)
    {
        if (recipe.doneStages && recipe.doneStages.Count() > 0)
        {
            int stage = recipe.doneStages.Get(0);
            if (ChefZ_VanillaStage.IsValid(stage) && stage != ChefZ_VanillaStage.NONE)
                return stage;
        }
        return ChefZ_VanillaStage.RAW;
    }

    //! Dieselbe Frage wie in ApplyModifier, ohne Nebenwirkung - der Audit
    //! braucht sie fuer den Befund, und ApplyModifier hat den Vektor schon
    //! skaliert, wenn der Audit dazu kommt.
    private bool ApplyModifierWasClamped(notnull ChefZ_CompiledRecipe recipe)
    {
        return !(recipe.nutritionModifier > 0.0);
    }

    //==========================================================================
    // Befunde ausgeben (13 §5, Beispielblock)
    //==========================================================================

    private void Emit(notnull ChefZ_NutritionFinding f, notnull array<ChefZ_NutritionFinding> outFindings)
    {
        outFindings.Insert(f);
        m_LastFindings.Insert(f);
        if (ChefZ_NutritionFindingKind.IsError(f.kind))
            m_LastErrorCount++;
    }

    /**
     * Der Block, den ein Betreiber im Startlog sieht (13 §5).
     *
     * Die Kopfzeile geht an der Stufenpruefung vorbei - dieselbe Entscheidung
     * wie beim Ladebericht (18 §4): ein Betreiber muss ohne Debugstufe sehen
     * koennen, ob sein Content Loecher hat. Die EINZELNEN Befunde folgen der
     * Kanal- und Stufenpruefung, bis auf die Fehler: die stehen immer da.
     */
    private void ReportFindings(notnull array<ChefZ_NutritionFinding> findings)
    {
        if (m_QuietForTest)
            return;

        if (findings.Count() == 0)
        {
            Banner("Naehrwertaudit ueber " + m_LastAuditedRecipes.ToString() + " Rezepte: keine Befunde.");
            return;
        }

        string chefzTxt2 = "Naehrwertaudit ueber " + m_LastAuditedRecipes.ToString() + " Rezepte, " + findings.Count().ToString() + " Befunde (";
        chefzTxt2 = chefzTxt2 + m_LastErrorCount.ToString() + " davon FEHLER).";
        Banner(chefzTxt2);

        int shown = 0;
        for (int i = 0; i < findings.Count(); i++)
        {
            ChefZ_NutritionFinding f = findings.Get(i);
            if (!f)
                continue;

            int severity = f.Severity();

            // Fehler IMMER, alles andere nur bei passendem Kanal und
            // ausreichender Stufe. Ein lautlos nicht saettigendes Gericht
            // darf nicht davon abhaengen, ob jemand den Kanal NUTRI
            // eingeschaltet hat (01 V7).
            if (severity != ChefZ_LogLevel.ERR && !ChefZ_Log.Enabled(ChefZ_LogChannel.NUTRI, severity))
                continue;

            if (shown >= m_MaxFindings)
            {
                ChefZ_Log.Info(ChefZ_LogChannel.NUTRI, "... weitere Befunde unterdrueckt (nutritionAuditMaxFindings = " + m_MaxFindings.ToString() + "). Insgesamt " + findings.Count().ToString() + ".");
                break;
            }

            Dispatch(severity, f.ToLine());
            shown++;
        }
    }

    private void Dispatch(int severity, string line)
    {
        if (severity == ChefZ_LogLevel.ERR)
            ChefZ_Log.Error(ChefZ_LogChannel.NUTRI, line);
        else if (severity == ChefZ_LogLevel.WARN)
            ChefZ_Log.Warn(ChefZ_LogChannel.NUTRI, line);
        else
            ChefZ_Log.Info(ChefZ_LogChannel.NUTRI, line);
    }

    private void Banner(string msg)
    {
        if (m_QuietForTest)
            return;
        ChefZ_Log.Banner(msg);
    }

    //==========================================================================
    // Anzeige (13 §4)
    //==========================================================================

    /**
     * Eine lesbare Zeile - "Nur fuer Anzeige und Cookbook" (13 §4).
     *
     * Bewusst OHNE Stringtable-Schluessel: der Core liefert hier eine
     * Diagnosezeile fuer Log und Adminkommando, kein Spieler-UI. Wer sie im
     * Cookbook zeigen will, uebersetzt sie dort - mit den Schluesseln des
     * Content-Moduls, das die Begriffe ohnehin fuehrt.
     *
     * Nur gefuellte Felder erscheinen. Eine Zeile mit vier Nullen sagt nichts
     * und verdeckt die eine Zahl, die etwas sagt.
     */
    void DescribeForUI(notnull ChefZ_NutritionVector vec, out string text)
    {
        text = "";

        if (vec.energy != 0.0)
            text = Append(text, "Energie " + Rounded(vec.energy));
        if (vec.water != 0.0)
            text = Append(text, "Wasser " + Rounded(vec.water));
        if (vec.fullness != 0.0)
            text = Append(text, "Saettigung " + Rounded(vec.fullness));
        if (vec.nutritionalIndex != 0.0)
            text = Append(text, "Naehrwert " + Rounded(vec.nutritionalIndex));
        if (vec.toxicity != 0.0)
            text = Append(text, "Toxizitaet " + Rounded(vec.toxicity));
        if (vec.digestibility != 0.0)
            text = Append(text, "Verdaulichkeit " + Rounded(vec.digestibility));

        if (text == "")
            text = "keine Naehrwertangaben";
    }

    private static string Append(string s, string part)
    {
        if (s == "")
            return part;
        return s + ", " + part;
    }

    /**
     * Auf eine Nachkommastelle, damit "1155" nicht als "1155.000000" dasteht.
     *
     * Von Hand und ausdruecklich NICHT ueber Math.Round: das rundet in Enforce
     * zur naechsten GERADEN Zahl (dieselbe Feststellung wie im
     * ChefZ_QualityManager). Fuer eine Logzeile ist kaufmaennisch die Regel,
     * die ein Leser erwartet.
     *
     * Das Vorzeichen wird getrennt gefuehrt: bei einem Wert zwischen -1 und 0
     * ist der Ganzzahlanteil 0, und ein "-" ginge sonst verloren.
     */
    private static string Rounded(float v)
    {
        bool negative = v < 0.0;
        float mag     = v;
        if (negative)
            mag = -mag;

        int scaled = (int)(mag * 10.0 + 0.5);
        int whole  = scaled / 10;
        int tenth  = scaled - whole * 10;

        string s = whole.ToString() + "." + tenth.ToString();
        if (negative && scaled != 0)
            s = "-" + s;
        return s;
    }

    private static string Signed(float v)
    {
        if (v >= 0.0)
            return "+" + Rounded(v);
        return Rounded(v);
    }

    //==========================================================================
    // Auskuenfte
    //==========================================================================

    bool IsReady()
    {
        return m_Ready;
    }

    int GetRecordCount()
    {
        return m_ByClass.Count() + m_CategoryDefs.Count() + m_ByTag.Count();
    }

    int GetRejectedCount()
    {
        return m_RejectedCount;
    }

    bool IsAuditEnabled()
    {
        return m_AuditEnabled;
    }

    bool HasAudited()
    {
        return m_AuditDone;
    }

    int GetAuditedRecipeCount()
    {
        return m_LastAuditedRecipes;
    }

    int GetFindingCount()
    {
        return m_LastFindings.Count();
    }

    int GetErrorFindingCount()
    {
        return m_LastErrorCount;
    }

    float GetTolerancePct()
    {
        return m_TolerancePct;
    }

    //! Die Befunde des letzten Audits, fuer das Adminkommando (13 §7).
    //! Kopie, nicht die interne Liste: ein Aufrufer, der sie sortiert oder
    //! leert, soll den Bestand des Managers nicht anfassen koennen.
    void GetLastFindings(out array<ChefZ_NutritionFinding> outFindings)
    {
        if (!outFindings)
            outFindings = new array<ChefZ_NutritionFinding>();
        outFindings.Clear();

        for (int i = 0; i < m_LastFindings.Count(); i++)
            outFindings.Insert(m_LastFindings.Get(i));
    }

    private string AuditStateName()
    {
        if (m_AuditEnabled)
            return "an";
        return "aus";
    }

    void DumpRecords(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        outLines.Insert("Naehrwertangaben: " + GetRecordCount().ToString() + " (abgewiesen: " + m_RejectedCount.ToString() + ")");

        for (int i = 0; i < m_Order.Count(); i++)
        {
            ChefZ_NutritionDef def = FindAnywhere(m_Order.Get(i));
            if (def)
                outLines.Insert("  " + def.DescribeValues());
        }
    }

    /**
     * Die Befunde des letzten Audits als Textblock (13 §7, "per Adminkommando
     * abrufbar").
     *
     * Ohne Deckel und ohne Stufenpruefung: wer ausdruecklich danach fragt,
     * will alles sehen. Der Deckel im Startlog dient der Lesbarkeit beim Boot,
     * nicht der Sparsamkeit.
     */
    void DumpFindings(out array<string> outLines)
    {
        if (!outLines)
            outLines = new array<string>();

        if (!m_AuditDone)
        {
            outLines.Insert("Naehrwertaudit: nicht gelaufen (" + NotRunReason() + ").");
            return;
        }

        string chefzTxt3 = "Naehrwertaudit ueber " + m_LastAuditedRecipes.ToString() + " Rezepte: " + m_LastFindings.Count().ToString() + " Befunde, davon ";
        chefzTxt3 = chefzTxt3 + m_LastErrorCount.ToString() + " FEHLER. Toleranz " + m_TolerancePct.ToString() + "%.";
        outLines.Insert(chefzTxt3);

        for (int i = 0; i < m_LastFindings.Count(); i++)
        {
            ChefZ_NutritionFinding f = m_LastFindings.Get(i);
            if (f)
                outLines.Insert("  " + f.ToLine());
        }
    }

    private string NotRunReason()
    {
        if (!m_AuditEnabled)
            return "enableNutritionAudit = false";
        if (!m_Ready)
            return "der Manager ist nicht gebaut";
        return "die Rezept-Engine war beim Boot nicht bereit";
    }

    private ChefZ_NutritionDef FindAnywhere(ChefZ_Sym sym)
    {
        ChefZ_NutritionDef def;
        if (m_ByClass.Find(sym, def) && def)
            return def;
        if (m_ByTag.Find(sym, def) && def)
            return def;

        for (int i = 0; i < m_CategoryDefs.Count(); i++)
        {
            ChefZ_NutritionDef cat = m_CategoryDefs.Get(i);
            if (cat && cat.sym == sym)
                return cat;
        }
        return null;
    }

    private void LogIfDebug()
    {
        if (m_QuietForTest)
            return;
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.NUTRI, ChefZ_LogLevel.DEBUG))
            return;

        array<string> lines = new array<string>();
        DumpRecords(lines);
        ChefZ_Log.Block(ChefZ_LogLevel.DEBUG, ChefZ_LogChannel.NUTRI, lines);
    }

    //==========================================================================
    // Innereien
    //==========================================================================

    private void Trace(array<string> trace, string line)
    {
        if (trace)
            trace.Insert(line);
    }

    private ChefZ_CategoryManager Cats()
    {
        if (m_CategoriesForTest)
            return m_CategoriesForTest;
        return ChefZ_CategoryManager.Get();
    }

    private ChefZ_IngredientManager Ingredients()
    {
        if (m_IngredientsForTest)
            return m_IngredientsForTest;
        return ChefZ_IngredientManager.Get();
    }

    private ChefZ_RecipeEngine Engine()
    {
        if (m_EngineForTest)
            return m_EngineForTest;
        return ChefZ_RecipeEngine.Get();
    }

    private void Report(ChefZ_LoadReport report, bool isError, notnull ChefZ_NutritionDef def, string msg)
    {
        if (!report)
            return;
        if (isError)
            report.AddError(def.sourceRef, def.id, msg);
        else
            report.AddWarn(def.sourceRef, def.id, msg);
    }

    private void ResetState()
    {
        m_ByClass.Clear();
        m_ByTag.Clear();
        m_CategoryDefs.Clear();
        m_CategoryBits.Clear();
        m_CategoryDepth.Clear();
        m_Order.Clear();
        m_LastFindings.Clear();

        m_Ready              = false;
        m_RejectedCount      = 0;
        m_LastAuditedRecipes = 0;
        m_LastErrorCount     = 0;
        m_AuditDone          = false;

        m_AuditEnabled = true;
        m_TolerancePct = 25.0;
        m_MaxFindings  = 64;
        m_ExpectedCap  = 100000.0;
    }

    //! Alles zurueck auf "vor dem Build". Nur fuer SAFE_MODE und Selbsttest.
    void Reset()
    {
        ResetState();
    }

    //==========================================================================
    // Nur fuer den Selbsttest
    //==========================================================================

    void SetQuietForTest(bool quiet)
    {
        m_QuietForTest = quiet;
    }

    void SetManagersForTest(ChefZ_CategoryManager cats, ChefZ_IngredientManager ing, ChefZ_RecipeEngine engine)
    {
        m_CategoriesForTest  = cats;
        m_IngredientsForTest = ing;
        m_EngineForTest      = engine;
    }
}
