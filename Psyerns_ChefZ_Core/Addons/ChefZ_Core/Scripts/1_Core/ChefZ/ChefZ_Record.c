//==============================================================================
// ChefZ_Record - Basis aller Datensaetze, plus Validier- und Kompilierkontext
//
// Entwurf: 02 §5.1, 02 E3 (feldweiser Patch), 02 §6 (NORMALIZE/VALIDATE/
// COMPILE), 03 §5.
//
// Ein Record ist ein reiner Datensatz. Er kennt weder Datei noch Config-Baum -
// er weiss nur, WOHER er kam (sourceRef, sourceRank), damit jede Meldung ueber
// ihn den Betreiber an die richtige Stelle schickt.
//
// ---------------------------------------------------------------------------
// Das Kernproblem des feldweisen Patches (02 E3), und wie es hier geloest ist
// ---------------------------------------------------------------------------
// Enforce-JsonSerializer fuellt fehlende Felder mit dem Typdefault. "nicht
// gesetzt" ist damit nicht von "auf den Defaultwert gesetzt" unterscheidbar.
// Fuer Rang 3 (Overlay patcht Rang 1/2 feldweise) ist genau diese
// Unterscheidung die ganze Mechanik. 02 E3 nennt drei Mittel:
//
//   1. ref-Typen (Arrays, Unterobjekte) bleiben null, wenn abwesend  -> direkt
//      erkennbar, kostet nichts.
//   2. Skalare bekommen Sentinel-Defaults (ChefZ_Undefined.FLOAT/INT/TEXT).
//   3. Wer den Sentinelwert literal setzen will, nennt das Feld in
//      explicitFields[].
//
// Fuer bool gibt es keinen Sentinel - bool hat nur zwei Werte, und beide sind
// legitime Nutzdaten. 02 E3 verweist solche Felder auf Mittel 3. Das ist
// korrekt, aber fuer den haeufigsten Overlay-Fall ("enabled": false) unbequem.
//
// Mittel 3 wird deshalb nicht von Hand, sondern aus dem TEXT befuellt:
// ChefZ_JsonExplicit (3_Game) liest die Rohdatei und traegt jeden wirklich
// geschriebenen Schluessel ein - auf Recordebene unter seinem Namen, in
// Unterobjekten unter seinem Pfad ("slots[2].minCount"). Der Record verteilt
// die Pfade in DistributeExplicitPaths() an seine Kinder. Ein
// handgeschriebenes explicitFields[] bleibt gueltig und wirkt genauso.
//
// Die Bool-Sonde (ChefZ_RecordProbe), die dasselbe Dokument zweimal mit
// umgekehrter Bool-Vorbelegung las, gibt es im Ladepfad nicht mehr: ihre
// Vorbelegung sass im Konstruktor, und den ruft der Serializer nie. Sie
// lebt nur noch in Selbsttests mit handgebauten Objekten weiter, wo der
// Konstruktor laeuft.
//
// Gewinn: der Betreiber schreibt "enabled": false und es wirkt - ohne dass er
// die Sentinelmechanik verstehen muss. Und ein Autor laesst inheritState weg
// und bekommt das dokumentierte true, statt eines stillen false.
//
// Layer: 1_Core. Reine Datenverarbeitung, kein Engine-Typ.
//==============================================================================

/**
 * Polaritaet der Bool-Sonde. Wird ausschliesslich vom JSON-Reader zwischen
 * zwei Parsedurchgaengen umgeschaltet und danach zurueckgesetzt.
 *
 * Statisch, weil der Serializer die Record-Objekte selbst erzeugt: es gibt
 * keinen anderen Weg, einem gleich entstehenden Objekt einen anderen
 * Ausgangswert mitzugeben. Der Zugriff ist streng synchron (LoadData kehrt
 * zurueck, bevor die naechste Zeile laeuft) - es gibt keinen Nebenlaeufer.
 */
class ChefZ_RecordProbe
{
    private static bool s_High;

    //! Ausgangswert fuer JEDES bool-Feld eines Records. Im Konstruktor
    //! aufrufen, nicht als Feldinitialisierer (Auswertungsreihenfolge
    //! statischer Fremdklassen ist in Enforce nicht zugesichert).
    static bool Bool()
    {
        return s_High;
    }

    static void Set(bool high)
    {
        s_High = high;
    }

    static void Reset()
    {
        s_High = false;
    }

    static bool IsHigh()
    {
        return s_High;
    }
}

//------------------------------------------------------------------------------

/**
 * Die drei Datenraenge aus 02 §3.
 *
 * Hoeherer Rang patcht niedrigeren feldweise, er ersetzt ihn nicht. Die Zahlen
 * sind aufsteigend, weil der Sink sie vergleicht - "hoeher" ist woertlich
 * gemeint.
 *
 * Als Konstanten und nicht als enum, damit sie direkt mit dem int-Feld
 * ChefZ_Record.sourceRank vergleichbar sind, das aus JSON kommen kann.
 */
class ChefZ_SourceRank
{
    static const int UNKNOWN         = 0;
    static const int CONFIG_CPP      = 1;   // Client + Server, von der Engine gemerged
    static const int ADDON_JSON      = 2;   // Client + Server, per Manifest benannt
    static const int PROFILE_OVERLAY = 3;   // nur Server, Admin-Overlay

    static string Name(int rank)
    {
        switch (rank)
        {
            case CONFIG_CPP:      return "config.cpp";
            case ADDON_JSON:      return "Addon-JSON";
            case PROFILE_OVERLAY: return "$profile-Overlay";
        }
        return "unbekannt";
    }

    static bool IsValid(int rank)
    {
        return rank >= CONFIG_CPP && rank <= PROFILE_OVERLAY;
    }
}

//------------------------------------------------------------------------------

/**
 * Kontext der VALIDATE-Stufe (02 §6).
 *
 * Traegt den Ladebericht und die Herkunftsangabe, damit Validate() nicht jede
 * Meldung selbst zusammensetzen muss. In S2 prueft er nur, was ohne fremde
 * Registries pruefbar ist; die Referenzintegritaet gegen Kategorien, Tags und
 * Zustaende kommt mit S3/S4 dazu und haengt sich an denselben Kontext.
 */
class ChefZ_ValidationContext
{
    private ChefZ_LoadReport m_Report;      // gehalten vom Config Manager, hier
                                            // bewusst OHNE ref: der Kontext ist
                                            // kurzlebig, der Bericht nicht.
    private int m_Errors;
    private int m_Warns;

    void Init(ChefZ_LoadReport report)
    {
        m_Report = report;
        m_Errors = 0;
        m_Warns  = 0;
    }

    void Error(notnull ChefZ_Record rec, string message)
    {
        m_Errors++;
        if (m_Report)
            m_Report.AddError(rec.sourceRef, rec.id, message);
    }

    void Warn(notnull ChefZ_Record rec, string message)
    {
        m_Warns++;
        if (m_Report)
            m_Report.AddWarn(rec.sourceRef, rec.id, message);
    }

    int ErrorCount() { return m_Errors; }
    int WarnCount()  { return m_Warns; }

    ChefZ_LoadReport GetReport() { return m_Report; }
}

/**
 * Kontext der COMPILE-Stufe (02 §6): Strings werden zu ChefZ_Sym.
 *
 * In S2 interniert er nur die eigene ID jedes Records. Seit S5 traegt er
 * ausserdem alles, was der Selektor- und Slotcompiler zum Aufloesen braucht
 * (07 §5): den Nachschlager, die Spezifitaetsgewichte, die Tiefengrenze und
 * die Vorgabe fuer excludeStates.
 *
 * Warum das HIER liegt und nicht im Compiler: der Compiler ist statisch und
 * zustandslos (07 §6). Traegt er Einstellungen, sind es globale Einstellungen,
 * und dann haengt das Ergebnis eines Aufrufs davon ab, wer vorher was gesetzt
 * hat. Der Kontext ist das kurzlebige Objekt, das ohnehin einen Ladevorgang
 * lang lebt - er ist der richtige Ort dafuer.
 *
 * Keiner der Nachschlager ist je null: wer nichts setzt, bekommt einen leeren
 * ChefZ_SymbolResolver. Der loest nichts auf, und damit wird jedes Rezept mit
 * einer Kategorie abgewiesen - laut, mit Meldung, statt still zu matchen.
 */
class ChefZ_CompileContext : Managed
{
    private ChefZ_LoadReport m_Report;
    private int m_Compiled;

    private ref ChefZ_SymbolResolver  m_Resolver;
    private ref ChefZ_PriorityWeights m_Weights;
    private int m_MaxSelectorDepth;
    private ref array<ChefZ_Sym> m_DefaultExcludedStates;

    //! Wer gerade kompiliert wird. Steht in jeder Meldung, damit 07 §7
    //! ("ERROR mit Rezept-ID, Slot-ID und Symbolname") erfuellbar ist.
    private string m_SubjectRef;
    private string m_SubjectId;

    void ChefZ_CompileContext()
    {
        m_MaxSelectorDepth = ChefZ_SelectorLimits.DEFAULT_MAX_DEPTH;
        m_SubjectRef       = "";
        m_SubjectId        = "";
    }

    void Init(ChefZ_LoadReport report)
    {
        m_Report   = report;
        m_Compiled = 0;
    }

    ChefZ_Sym Intern(string name)
    {
        return ChefZ_SymbolTable.Intern(name);
    }

    //! Aufloesen OHNE Anlegen. INVALID bedeutet "unbekanntes Symbol" und ist
    //! ab S3 ein Validierungsfehler - in S2 gibt es noch keine Registry,
    //! gegen die man pruefen koennte.
    ChefZ_Sym Lookup(string name)
    {
        return ChefZ_SymbolTable.Lookup(name);
    }

    void CountCompiled()
    {
        m_Compiled++;
    }

    int CompiledCount() { return m_Compiled; }

    void Error(notnull ChefZ_Record rec, string message)
    {
        if (m_Report)
            m_Report.AddError(rec.sourceRef, rec.id, message);
    }

    ChefZ_LoadReport GetReport() { return m_Report; }

    //--------------------------------------------------------------------------
    // S5: Aufloesen von Selektoren und Slots (07 §5)
    //--------------------------------------------------------------------------

    //! Nie null. Ein leerer Nachschlager kennt nichts - das ist die sichere
    //! Ausgangslage, nicht die bequeme.
    ChefZ_SymbolResolver Resolver()
    {
        if (!m_Resolver)
            m_Resolver = new ChefZ_SymbolResolver();
        return m_Resolver;
    }

    void SetResolver(ChefZ_SymbolResolver resolver)
    {
        m_Resolver = resolver;
    }

    //! Nie null. Ohne Konfiguration gelten die Code-Defaults aus 09 §3.
    ChefZ_PriorityWeights Weights()
    {
        if (!m_Weights)
            m_Weights = new ChefZ_PriorityWeights();
        return m_Weights;
    }

    void SetWeights(ChefZ_PriorityWeights weights)
    {
        m_Weights = weights;
    }

    //! CoreSettings.maxSelectorDepth (Default 8). Schuetzt vor zyklischem
    //! Copy-Paste und begrenzt die Auswertungstiefe (07 §7).
    int MaxSelectorDepth()
    {
        if (m_MaxSelectorDepth < 1)
            return ChefZ_SelectorLimits.DEFAULT_MAX_DEPTH;
        return m_MaxSelectorDepth;
    }

    void SetMaxSelectorDepth(int depth)
    {
        m_MaxSelectorDepth = depth;
    }

    /**
     * CoreSettings.defaultExcludedStates als Symbole (07 E5).
     *
     * Nie null, aber sehr wohl leer: ein Betreiber, der die Liste in Core.json
     * leert, schaltet den Filter bewusst ab. Der Unterschied zu "Slot nennt
     * excludeStates: []" ist keiner - beides ist eine ausdrueckliche Ansage.
     */
    array<ChefZ_Sym> DefaultExcludedStates()
    {
        if (!m_DefaultExcludedStates)
            m_DefaultExcludedStates = new array<ChefZ_Sym>();
        return m_DefaultExcludedStates;
    }

    void SetDefaultExcludedStates(array<ChefZ_Sym> states)
    {
        m_DefaultExcludedStates = new array<ChefZ_Sym>();
        if (!states)
            return;
        for (int i = 0; i < states.Count(); i++)
            m_DefaultExcludedStates.Insert(states.Get(i));
    }

    //! Herkunft und ID dessen, was gerade kompiliert wird.
    void SetSubject(string sourceRef, string recordId)
    {
        m_SubjectRef = sourceRef;
        m_SubjectId  = recordId;
    }

    string SubjectId() { return m_SubjectId; }

    void Fail(string message)
    {
        if (m_Report)
            m_Report.AddError(m_SubjectRef, m_SubjectId, message);
    }

    void Warn(string message)
    {
        if (m_Report)
            m_Report.AddWarn(m_SubjectRef, m_SubjectId, message);
    }
}

//------------------------------------------------------------------------------

/**
 * Basis aller Records. Schnittstelle woertlich aus 02 §5.1, erweitert um die
 * Mittel des feldweisen Patches (02 E3) und um sym (03 §5, COMPILE-Stufe).
 */
class ChefZ_Record : Managed
{
    string  id;                     // fachliche ID, z.B. "MEAT"
    string  sourceRef;              // "CfgChefZ ChefZ_Meat" | "<pfad>.json" | "$profile:..."
    int     sourceRank;             // 1 = config.cpp, 2 = Addon-JSON, 3 = Overlay
    int     loadOrder;
    bool    disabled;               // Overlay kann abschalten, ohne zu loeschen

    //! 02 E3, Mittel 3. Feldnamen, die ausdruecklich gesetzt sind, obwohl ihr
    //! Wert wie "nicht gesetzt" aussieht. Wird vom Autor geschrieben ODER vom
    //! Reader aus der Bool-Sonde nachgetragen.
    ref array<string> explicitFields;

    //! Ergebnis der COMPILE-Stufe. Nicht aus JSON zu setzen - wird beim Laden
    //! ueberschrieben.
    ChefZ_Sym sym;

    void ChefZ_Record()
    {
        id         = "";
        sourceRef  = "";
        sourceRank = 0;
        // Sentinel und nicht 0: sonst wuerde jeder Overlay-Record, der
        // loadOrder gar nicht nennt, eine 0 in den gepatchten Record schreiben
        // und dessen Reihenfolge stillschweigend aendern (02 E3).
        loadOrder  = ChefZ_Undefined.INT;
        sym        = ChefZ_SymbolTable.INVALID;
        disabled   = ChefZ_RecordProbe.Bool();
    }

    //--------------------------------------------------------------------------
    // Art
    //--------------------------------------------------------------------------

    //! Leerstring in der Basis: ein Record ohne Art ist ein Programmierfehler
    //! und wird vom Sink abgewiesen, nicht stillschweigend einsortiert.
    string GetKindName()
    {
        return "";
    }

    //--------------------------------------------------------------------------
    // Herkunft
    //--------------------------------------------------------------------------

    /**
     * Setzt die Herkunft. IMMER nach dem Deserialisieren aufrufen - sonst
     * koennte eine JSON-Datei ihren eigenen sourceRank behaupten und sich damit
     * ueber ein Overlay stellen.
     */
    void SetOrigin(string ref_, int rank)
    {
        sourceRef  = ref_;
        sourceRank = rank;
    }

    //--------------------------------------------------------------------------
    // NORMALIZE (02 §6)
    //--------------------------------------------------------------------------

    /**
     * Trim auf der ID. KEIN Case-Folding: 03 E5 verbietet es ausdruecklich -
     * "MEAT" und "Meat" sind verschiedene IDs, und sie still zusammenzuziehen
     * waere genau die Mehrdeutigkeit, die dort abgelehnt wird. Ausserdem sind
     * viele IDs Klassennamen, und Klassennamen gehoeren dem Content.
     */
    void Normalize()
    {
        id.TrimInPlace();
        sourceRef.TrimInPlace();
        if (explicitFields)
        {
            for (int i = 0; i < explicitFields.Count(); i++)
            {
                string f = explicitFields.Get(i);
                f.TrimInPlace();
                explicitFields.Set(i, f);
            }
        }
    }

    //--------------------------------------------------------------------------
    // VALIDATE (02 §6)
    //--------------------------------------------------------------------------

    /**
     * true = Record wird uebernommen, false = abgewiesen. Ein abgewiesener
     * Record laesst alle anderen unberuehrt (02 §8).
     *
     * Ableitungen rufen super.Validate(ctx) zuerst und ergaenzen ihre eigenen
     * Pflichtfelder.
     */
    bool Validate(ChefZ_ValidationContext ctx)
    {
        if (id == "")
        {
            if (ctx)
                ctx.Error(this, "Record ohne \"id\" - abgewiesen. Jeder Datensatz braucht eine fachliche ID.");
            return false;
        }
        if (id.IndexOf(" ") >= 0)
        {
            if (ctx)
                ctx.Error(this, "ID \"" + id + "\" enthaelt ein Leerzeichen. IDs werden zu Symbolen interniert " + "und in Config-Pfaden verwendet - Leerzeichen sind dort mehrdeutig.");
            return false;
        }
        return true;
    }

    //--------------------------------------------------------------------------
    // COMPILE (02 §6, 03 §5)
    //--------------------------------------------------------------------------

    //! Strings -> ChefZ_Sym. Die Basis interniert die eigene ID; Ableitungen
    //! loesen ihre Felder auf und rufen super.Compile(ctx) zuerst.
    void Compile(ChefZ_CompileContext ctx)
    {
        if (ctx)
        {
            sym = ctx.Intern(id);
            ctx.CountCompiled();
        }
        else
        {
            sym = ChefZ_SymbolTable.Intern(id);
        }
    }

    //--------------------------------------------------------------------------
    // MERGE - feldweiser Patch (02 E3)
    //--------------------------------------------------------------------------

    /**
     * Uebernimmt die gesetzten Felder aus src (hoeherer Rang) in diesen Record.
     *
     * Die Basis behandelt loadOrder und disabled. Ableitungen rufen
     * super.PatchFrom(src) zuerst und patchen danach ihre eigenen Felder,
     * ausnahmslos ueber die Patch*-Helfer.
     *
     * sourceRef wird bewusst NICHT ueberschrieben, sondern ergaenzt: nach einem
     * Patch soll im Log beides stehen - woher der Record kam und wer ihn
     * veraendert hat.
     */
    void PatchFrom(notnull ChefZ_Record src)
    {
        // ZUERST: die als gesetzt markierten Felder der Quelle uebernehmen.
        //
        // Ohne diesen Schritt haette der gepatchte Record hinterher einen Wert
        // aus dem Overlay, waere aber selbst nicht als "gesetzt" markiert - und
        // ein spaeteres ResolveDefaults() wuerde ihn still auf den Code-Default
        // zuruecksetzen. Das ist genau die Sorte Fehler, die niemand findet,
        // weil nichts danebengeht: der Wert ist einfach wieder der alte.
        AdoptExplicitFrom(src);

        loadOrder = PatchInt(loadOrder, src.loadOrder, src, "loadOrder");
        disabled  = PatchBool(disabled, src.disabled, src, "disabled");

        if (src.sourceRef != "")
            sourceRef = sourceRef + " + " + src.sourceRef;
    }

    private void AdoptExplicitFrom(notnull ChefZ_Record src)
    {
        if (!src.explicitFields)
            return;
        for (int i = 0; i < src.explicitFields.Count(); i++)
            MarkExplicit(src.explicitFields.Get(i));
    }

    //--------------------------------------------------------------------------
    // Patch-Helfer. Alle nach demselben Muster: "gesetzt?" -> uebernehmen,
    // sonst den bisherigen Wert behalten.
    //--------------------------------------------------------------------------

    static float PatchFloat(float current, float incoming, ChefZ_Record src, string field)
    {
        if (!ChefZ_Undefined.IsFloatUndefined(incoming))
            return incoming;
        if (src && src.HasExplicit(field))
            return incoming;
        return current;
    }

    static int PatchInt(int current, int incoming, ChefZ_Record src, string field)
    {
        if (!ChefZ_Undefined.IsIntUndefined(incoming))
            return incoming;
        if (src && src.HasExplicit(field))
            return incoming;
        return current;
    }

    static string PatchText(string current, string incoming, ChefZ_Record src, string field)
    {
        if (!ChefZ_Undefined.IsTextUndefined(incoming))
            return incoming;
        if (src && src.HasExplicit(field))
            return incoming;
        return current;
    }

    //! bool kennt keinen Sentinel: er wird uebernommen, wenn das Feld in
    //! explicitFields[] steht - handgeschrieben oder von der Bool-Sonde
    //! nachgetragen (Kopfkommentar).
    static bool PatchBool(bool current, bool incoming, ChefZ_Record src, string field)
    {
        if (src && src.HasExplicit(field))
            return incoming;
        return current;
    }

    //--------------------------------------------------------------------------
    // Defaults - dieselbe Frage wie beim Patch, nur eine Ebene frueher
    //--------------------------------------------------------------------------
    //
    // ResolveDefaults() hat frueher ChefZ_Undefined.IntOr(wert, vorgabe)
    // benutzt. Seit die Sentinel die Typdefaults SIND (siehe Kopf von
    // ChefZ_Undefined.c) waere das zweideutig: eine ausdruecklich geschriebene
    // 0 saehe aus wie ein fehlendes Feld und bekaeme die Vorgabe. Genau das
    // waere bei "minCount": 0 und "takeDurationSec": 0 falsch, und beides steht
    // im vorhandenen Inhalt.
    //
    // Deshalb wird zuerst der Text gefragt und erst danach der Wert. Die zweite
    // Zeile ist kein Beiwerk: Records, die von Hand gebaut werden - Selbsttest,
    // Vorschau - haben kein explicitFields[], und dort ist der Wert die einzige
    // Auskunft, die es gibt.

    int DefaultInt(string field, int value, int fallback)
    {
        if (HasExplicit(field))
            return value;
        if (!ChefZ_Undefined.IsIntUndefined(value))
            return value;
        return fallback;
    }

    float DefaultFloat(string field, float value, float fallback)
    {
        if (HasExplicit(field))
            return value;
        if (!ChefZ_Undefined.IsFloatUndefined(value))
            return value;
        return fallback;
    }

    string DefaultText(string field, string value, string fallback)
    {
        if (HasExplicit(field))
            return value;
        if (!ChefZ_Undefined.IsTextUndefined(value))
            return value;
        return fallback;
    }

    /**
     * Darf die Quelle dieses ref-Feld (Liste, Unterobjekt) ganz ersetzen?
     *
     * Frueher hiess null "nicht gesetzt", und jede nicht-null Liste ersetzte.
     * Der Serializer legt aber JEDE Liste und jedes Unterobjekt an, ob der
     * Schluessel im JSON steht oder nicht (ba6a9d4). Ein Overlay
     * { "id": "CORE" } brachte deshalb leere Listen fuer logChannels und
     * defaultExcludedStates und leere Bloecke fuer priorityWeights und
     * qualityScoring mit - und ersetzte damit alles, was Core.json gesagt
     * hatte: keine Zustandsstrafen mehr fuer BURNT und ROTTEN, auf jedem
     * Server mit eingeschaltetem Overlay.
     *
     * Ersetzt wird deshalb nur, was die Quelle WIRKLICH geschrieben hat.
     * Eine Quelle ohne jedes Textwissen - handgebaut, explicitFields null -
     * gilt weiter beim Wort: dort ist eine gesetzte Liste eine Absicht.
     */
    bool MayReplace(string field)
    {
        if (!explicitFields)
            return true;
        return HasExplicit(field);
    }

    //! ref-Typen: Ganzersatz, nicht elementweise - eine Liste ist eine Aussage
    //! als Ganzes. Ob die Quelle die Aussage gemacht hat, sagt MayReplace.
    static array<string> PatchStringArray(array<string> current, array<string> incoming, ChefZ_Record src, string field)
    {
        if (incoming && src && src.MayReplace(field))
            return incoming;
        return current;
    }

    //--------------------------------------------------------------------------
    // explicitFields
    //--------------------------------------------------------------------------

    bool HasExplicit(string field)
    {
        if (!explicitFields)
            return false;
        return explicitFields.Find(field) >= 0;
    }

    void MarkExplicit(string field)
    {
        if (!explicitFields)
            explicitFields = new array<string>();
        if (explicitFields.Find(field) < 0)
            explicitFields.Insert(field);
    }

    /**
     * Pfade an die Unterobjekte verteilen (Kopf von ChefZ_JsonExplicit).
     *
     * Die Basis hat keine Unterobjekte mit eigenem explicitFields[] und tut
     * nichts. Records, die welche haben (Rezept, Transform, CoreSettings),
     * ueberschreiben das und rufen super zuerst. Laeuft einmal je Record,
     * direkt nach dem Lesen - VOR dem Merge, damit ein Unterobjekt seine
     * Markierungen aus seinem eigenen Dokument traegt und sie beim
     * Ganzersatz durch ein Overlay mitnimmt.
     */
    void DistributeExplicitPaths()
    {
    }

    //! Alle Eintraege, die mit prefix beginnen, ohne den Praefix. Fuer
    //! "slots[2]." liefert "slots[2].minCount" also "minCount".
    void CollectExplicitUnder(string prefix, notnull array<string> outSuffixes)
    {
        if (!explicitFields)
            return;
        int n = prefix.Length();
        for (int i = 0; i < explicitFields.Count(); i++)
        {
            string f = explicitFields.Get(i);
            if (f.Length() <= n)
                continue;
            if (f.Substring(0, n) != prefix)
                continue;
            outSuffixes.Insert(f.Substring(n, f.Length() - n));
        }
    }

    //! Die Pfade unter key[i]. an die Slots der Liste. Slots und Ergebnisse
    //! haben keine gemeinsame Basis mit MarkExplicit, deshalb zwei Helfer.
    protected void PushExplicitToSlots(string key, array<ref ChefZ_SlotDef> list)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_SlotDef s = list.Get(i);
            if (!s)
                continue;
            array<string> sub = new array<string>();
            CollectExplicitUnder(key + "[" + i.ToString() + "].", sub);
            for (int k = 0; k < sub.Count(); k++)
                s.MarkExplicit(sub.Get(k));
        }
    }

    protected void PushExplicitToOutputs(string key, array<ref ChefZ_OutputDef> list)
    {
        if (!list)
            return;
        for (int i = 0; i < list.Count(); i++)
        {
            ChefZ_OutputDef o = list.Get(i);
            if (!o)
                continue;
            array<string> sub = new array<string>();
            CollectExplicitUnder(key + "[" + i.ToString() + "].", sub);
            for (int k = 0; k < sub.Count(); k++)
                o.MarkExplicit(sub.Get(k));
        }
    }

    /**
     * Bool-Sonde auswerten. "other" ist derselbe Record aus dem zweiten
     * Parsedurchgang mit umgekehrter Bool-Polaritaet.
     *
     * Die Basis behandelt disabled. Jede Ableitung mit eigenen bool-Feldern
     * ruft super.CaptureExplicitBools(other) zuerst und vergleicht danach ihre
     * eigenen - das ist die einzige Pflicht, die ein neues bool-Feld mitbringt.
     */
    void CaptureExplicitBools(ChefZ_Record other)
    {
        if (!other)
            return;
        if (disabled == other.disabled)
            MarkExplicit("disabled");
    }

    //--------------------------------------------------------------------------
    // Nachbereitung
    //--------------------------------------------------------------------------

    /**
     * Nach dem Merge: Sentinel durch die Code-Defaults ersetzen.
     *
     * Die Basis hat nichts zu tun - loadOrder 0 und disabled false sind
     * brauchbare Defaults. Records mit Sentinelfeldern ueberschreiben das.
     */
    void ResolveDefaults()
    {
        loadOrder = DefaultInt("loadOrder", loadOrder, 0);
    }

    //--------------------------------------------------------------------------

    string Describe()
    {
        string k = GetKindName();
        if (k == "")
            k = "?";
        return k + " \"" + id + "\" (Rang " + sourceRank.ToString() + ", " + sourceRef + ")";
    }

    //! Nur fuer den Selbsttest.
    static bool SelfCheck()
    {
        ChefZ_Record r = new ChefZ_Record();
        r.id = "  TEST_A  ";
        r.Normalize();
        if (r.id != "TEST_A")                       return false;

        ChefZ_ValidationContext ctx = new ChefZ_ValidationContext();
        ctx.Init(null);
        if (!r.Validate(ctx))                       return false;

        ChefZ_Record bad = new ChefZ_Record();
        bad.id = "";
        if (bad.Validate(ctx))                      return false;
        ChefZ_Record spaced = new ChefZ_Record();
        spaced.id = "A B";
        if (spaced.Validate(ctx))                   return false;

        // Patch-Helfer
        if (PatchFloat(1.0, ChefZ_Undefined.FLOAT, null, "x") != 1.0)    return false;
        if (PatchFloat(1.0, 2.0, null, "x") != 2.0)                      return false;
        if (PatchInt(1, ChefZ_Undefined.INT, null, "x") != 1)            return false;
        if (PatchInt(1, 2, null, "x") != 2)                              return false;
        if (PatchText("a", "", null, "x") != "a")                        return false;
        if (PatchText("a", "b", null, "x") != "b")                       return false;

        ChefZ_Record src = new ChefZ_Record();
        src.disabled = true;
        if (PatchBool(false, true, src, "disabled") != false)            return false;  // nicht explizit -> ignoriert
        src.MarkExplicit("disabled");
        if (PatchBool(false, true, src, "disabled") != true)             return false;

        // Bool-Sonde: gleicher Wert in beiden Durchgaengen -> stand im JSON
        ChefZ_Record low  = new ChefZ_Record();
        ChefZ_Record high = new ChefZ_Record();
        low.disabled  = false;
        high.disabled = true;                   // unterschiedlich -> fehlte
        low.CaptureExplicitBools(high);
        if (low.HasExplicit("disabled"))                                 return false;

        ChefZ_Record low2  = new ChefZ_Record();
        ChefZ_Record high2 = new ChefZ_Record();
        low2.disabled  = true;
        high2.disabled = true;                  // gleich -> stand im JSON
        low2.CaptureExplicitBools(high2);
        if (!low2.HasExplicit("disabled"))                               return false;

        return true;
    }
}
