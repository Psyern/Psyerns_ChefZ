//==============================================================================
// ChefZ_GenericCraftRecipe - GENAU EINE aus Daten parametrisierte
// RecipeBase-Ableitung fuer ALLE HANDCRAFT-Prozesse.
//
// Entwurf: 11 §3 (HANDCRAFT laeuft ueber Vanillas Craftsystem), 11 §4
// (Schnittstelle woertlich), 11 §5 (Datenfluss HANDCRAFT), 11 §7
// (Fehlerverhalten), 11 E2 (warum Vanillas Craftsystem), 11 E3 (genau EINE
// Ableitung), 01 V12 (die 2-Zutaten-Grenze), 19 S15.
//
// ---------------------------------------------------------------------------
// E3, und warum Init() zweimal laeuft
// ---------------------------------------------------------------------------
// Vanilla schreibt eine Klasse je Rezept. Fuer ChefZ waere das eine Klasse je
// Verarbeitungsschritt - also eine Core-Codeaenderung je Content-Zugang und
// ein direkter Verstoss gegen §10.3. InitFromDef() verlagert die
// Parametrierung deshalb vollstaendig in die Daten.
//
// RecipeBase ruft Init() bereits aus seinem KONSTRUKTOR. Zu diesem Zeitpunkt
// gibt es noch keine Definition, und der Aufruf muss folgenlos bleiben. Genau
// das sagt 11 E3: "Init() muss NACH der Zuweisung der Definition laufen. Die
// Registrierungsschleife in RegisterRecipies() stellt das sicher." Init()
// prueft deshalb als erstes, ob eine Definition anliegt, und kehrt sonst
// zurueck, ohne ein Feld anzufassen.
//
// ---------------------------------------------------------------------------
// Die Abbildung: ChefZ-Transform -> Vanillas zwei Zutatenplaetze
// ---------------------------------------------------------------------------
//   1 Eingang  + Werkzeuggruppe(n)   Platz 0 = Eingang, Platz 1 = Werkzeug
//   2 Eingaenge, KEINE Werkzeuggruppe Platz 0 und 1 = die beiden Eingaenge
//
// Alles andere ist nicht abbildbar und wird beim Bauen ABGEWIESEN:
//
//   - 1 Eingang ohne Werkzeug   Vanilla braucht zwei Zutaten; es gaebe nichts,
//                               womit der Spieler den Eingang kombinieren
//                               koennte.
//   - 2 Eingaenge MIT Werkzeug  das Werkzeug waere ein dritter Platz. 01 V12
//                               kennt genau zwei.
//   - 0 oder >2 Eingaenge       die Grenze aus 11 §7, dort bereits als
//                               Compilerfehler (ChefZ_ProcessCompiler).
//
// Jede dieser Abweisungen nennt STATION_ACTION / STATION_TIMED als Ausweg -
// dieselbe Auskunft, die 11 §7 fuer die 2-Eingaenge-Grenze verlangt.
//
// ---------------------------------------------------------------------------
// Wer macht was - und warum der Verbrauch NICHT bei Vanilla liegt
// ---------------------------------------------------------------------------
//   VANILLA  Aktionsfindung, Zutatensuche in Hand und Inventar, Animation,
//            Softskill-Gewichtung, Werkzeugschaden, Erzeugung der Ergebnisse,
//            Health-Vererbung. Alles gratis und unveraendert (11 E2).
//   ChefZ    die ENTSCHEIDUNG (CanDo -> derselbe Matcher wie an einer
//            Station), die ChefZ-Schicht am Ergebnis (Zustand, Stufe,
//            Frische, Temperatur) und der VERBRAUCH.
//
// Der Verbrauch liegt bei ChefZ und nicht bei Vanilla, obwohl Vanilla ihn
// koennte (m_IngredientDestroy, m_IngredientAddQuantity). Grund ist eine
// Einheitenfrage, keine Vorliebe: ChefZ_CompiledSlot.consumeAmount steht in
// REZEPTEINHEITEN (05 §6), Vanillas Felder in Vanilla-Menge. Die Umrechnung
// haengt am konkreten Item und ist zum Zeitpunkt der Registrierung nicht
// bekannt. Der Verbrauchsplan des Matchers kennt sie - und er wird von
// ChefZ_Applicator.ConsumeInputs() ausgefuehrt, also von genau derselben
// Zeile, die auch der Kochtopf und die Station benutzen (11 E4).
//
// Reihenfolge innerhalb von Vanillas PerformRecipe():
//     SpawnItems -> ApplyModificationsResults -> ApplyModificationsIngredients
//     -> Do()  <- HIER wird verbraucht -> DeleleIngredientsPass
// Der Verbrauch steht damit HINTER der Erzeugung. Das ist Invariante I5,
// buchstaeblich dieselbe Reihenfolge wie in 08 §6.
//
// KEIN CONTENT: kein Prozessname, keine Zutat, kein Werkzeug, kein Gericht.
// Jede Klasse, jede Gruppe und jeder Text kommt aus den Daten.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_GenericCraftRecipe : RecipeBase
{
    //! Rueckfallname, wenn ein Prozess keinen displayName fuehrt. Der EINZIGE
    //! Text dieser Datei, und er steht in der Stringtable - nicht im Code.
    static const string FALLBACK_NAME = "#STR_CHEFZ_CRAFT_GENERIC";

    //! Vanillas Health-STUFE, ab der eine Zutat abgelehnt wird. 4 ist
    //! "ruiniert"; 3 laesst alles darunter zu. Dieselbe Zahl, die JEDES
    //! Vanilla-Rezept setzt (PrepareAnimal, PeelPotato). Sie ist kein
    //! Balancing-Knopf, sondern Vanillas Konvention fuer "kaputtes Zeug
    //! verarbeitet man nicht".
    //! float und nicht int, obwohl es eine Stufe ist: das Zielfeld
    //! (m_MaxDamageIngredient) ist float, und eine Umwandlung an einer
    //! Zuweisungsgrenze ist in Enforce nirgends zugesichert.
    static const float MAX_HEALTH_LEVEL = 3.0;

    //! Rueckfall fuer animationLength, wenn die Daten nichts nennen. 1.0 ist
    //! Vanillas eigener Default in RecipeBase.
    static const float DEFAULT_ANIMATION_LENGTH = 1.0;

    //--- Was aus der Definition uebrig bleibt --------------------------------
    protected ChefZ_Sym m_ChefZ_ProcessSym;
    protected ChefZ_Sym m_ChefZ_TransformSym;
    protected string    m_ChefZ_TransformId;
    protected string    m_ChefZ_ProcessId;

    //! Zahl der ChefZ-Eingangsslots auf Vanillas Zutatenplaetzen (1 oder 2).
    protected int  m_ChefZ_InputCount;

    //! Zutatenplatz des Werkzeugs, oder -1. Bei 2 Eingaengen gibt es keinen.
    protected int  m_ChefZ_ToolIndex;

    protected bool m_ChefZ_Ready;
    protected bool m_ChefZ_Repeatable;
    protected bool m_ChefZ_PureStateChange;

    //! Grund, wenn Init() nicht gebaut hat. Leer, solange alles stimmt.
    protected string m_ChefZ_InitError;

    /**
     * Ergebnis i der Vanillaliste -> das ChefZ_OutputDef, das es beschreibt.
     *
     * Diese Liste ist die Bruecke zwischen zwei Nummerierungen: Vanilla
     * kennt nur m_ItemsToCreate[i], ChefZ kennt outputs[] und byproducts[].
     * Sie wird in genau der Reihenfolge gefuellt, in der AddResult() gerufen
     * wird - und nur so bleibt results[i] in Do() dem richtigen Output
     * zugeordnet.
     *
     * ref auf dieselben Objekte, die auch der ChefZ_CompiledTransform haelt.
     * Zwei Halter, kein Zyklus - dieselbe Bauform wie in
     * ChefZ_ProcessRunner.BuildResult().
     */
    protected ref array<ref ChefZ_OutputDef> m_ChefZ_ResultDefs;

    //--- Nur waehrend InitFromDef() gesetzt (siehe Dateikopf) ----------------
    //
    // Bewusst SCHWACHE Zeiger und bewusst nur fuer die Dauer eines Aufrufs:
    // die kompilierten Objekte gehoeren dem ChefZ_ProcessingManager. Ein
    // starker Halter hier waere ein zweiter Besitzer fuer etwas, das den
    // Bestand ueberdauern koennte - und ein Rezept, das eine geloeschte
    // Definition am Leben haelt, ist genau die Art Fehler, die niemand
    // bemerkt.
    protected ChefZ_CompiledProcess   m_ChefZ_PendingProc;
    protected ChefZ_CompiledTransform m_ChefZ_PendingTr;

    //! Die ausgerollten Klassenlisten, ebenfalls nur fuer die Dauer von
    //! InitFromDef(). Sie liegen als Feld und nicht als Argument vor, weil
    //! Vanillas Init() keine Argumente hat - und 11 E3 verlangt, dass der
    //! Aufbau in Init() geschieht.
    protected ref array<ref array<string>> m_ChefZ_PendingInputs;
    protected ref array<string>            m_ChefZ_PendingTools;

    //! Wiederverwendete Puffer. Do() und CanDo() laufen im Aktionspfad; eine
    //! Allokation je Bildaufbau waere hier messbar.
    protected ref ChefZ_FactSnapshot   m_ChefZ_Snapshot;
    protected ref array<ItemBase>      m_ChefZ_Entities;
    protected ref ChefZ_ProcessContext m_ChefZ_Context;
    protected ref ChefZ_TransformMatch m_ChefZ_Match;

    //==========================================================================

    void ChefZ_GenericCraftRecipe()
    {
        // ACHTUNG: RecipeBase() hat Init() BEREITS gerufen, bevor diese Zeile
        // laeuft. Alles, was hier steht, ist damit NACH Init(). Deshalb darf
        // Init() auf keines dieser Felder angewiesen sein - und ist es nicht:
        // es kehrt ohne Definition sofort zurueck.
        m_ChefZ_ProcessSym      = ChefZ_SymbolTable.INVALID;
        m_ChefZ_TransformSym    = ChefZ_SymbolTable.INVALID;
        m_ChefZ_TransformId     = "";
        m_ChefZ_ProcessId       = "";
        m_ChefZ_InputCount      = 0;
        m_ChefZ_ToolIndex       = -1;
        m_ChefZ_Ready           = false;
        m_ChefZ_Repeatable      = false;
        m_ChefZ_PureStateChange = false;
        m_ChefZ_InitError       = "";
        m_ChefZ_ResultDefs      = new array<ref ChefZ_OutputDef>();
        m_ChefZ_PendingProc     = null;
        m_ChefZ_PendingTr       = null;
        m_ChefZ_PendingInputs   = null;
        m_ChefZ_PendingTools    = null;
    }

    //==========================================================================
    // Parametrierung (11 §4, 11 E3)
    //==========================================================================

    /**
     * Uebernimmt Prozess und Transform und baut daraus ein Vanilla-Rezept.
     *
     * @param inputClasses  Klassennamen je Eingangsslot, in Slotreihenfolge.
     *                      inputClasses[k] gehoert zu proc/tr.inputs[k].
     *                      Der Aufrufer erhebt sie - siehe
     *                      ChefZ_HandcraftBridge.
     * @param toolClasses   Klassennamen aller Werkzeuge, die eine der
     *                      geforderten Gruppen bedienen. Leer, wenn der
     *                      Prozess keine Gruppe nennt.
     * @param err           Klartextgrund bei false. Nie leer, wenn false.
     *
     * @return false, wenn sich der Transform NICHT auf Vanillas zwei Plaetze
     *         abbilden laesst. Das Rezept ist dann unbrauchbar und darf nicht
     *         registriert werden.
     */
    bool InitFromDef(notnull ChefZ_CompiledProcess proc,
                     notnull ChefZ_CompiledTransform tr,
                     notnull array<ref array<string>> inputClasses,
                     notnull array<string> toolClasses,
                     out string err)
    {
        err = "";

        m_ChefZ_PendingProc   = proc;
        m_ChefZ_PendingTr     = tr;
        m_ChefZ_PendingInputs = inputClasses;
        m_ChefZ_PendingTools  = toolClasses;
        m_ChefZ_InitError     = "";
        m_ChefZ_Ready         = false;

        // 11 E3, woertlich: "Init() muss NACH der Zuweisung der Definition
        // laufen. Die Registrierungsschleife in RegisterRecipies() stellt das
        // sicher." Genau diese Zeile ist gemeint - der Aufbau geschieht in
        // Init(), und die Definition liegt jetzt an.
        Init();

        m_ChefZ_PendingProc   = null;
        m_ChefZ_PendingTr     = null;
        m_ChefZ_PendingInputs = null;
        m_ChefZ_PendingTools  = null;

        if (!m_ChefZ_Ready)
        {
            err = m_ChefZ_InitError;
            if (err == "")
                err = "unbekannter Grund";
            return false;
        }

        return true;
    }

    /**
     * Vanillas Einstiegspunkt.
     *
     * Er laeuft ZWEIMAL: einmal aus dem RecipeBase-Konstruktor, ohne
     * Definition, und einmal aus InitFromDef(), mit. Der erste Lauf muss
     * folgenlos sein - siehe Dateikopf.
     */
    override void Init()
    {
        if (!m_ChefZ_PendingProc || !m_ChefZ_PendingTr)
            return;
        if (!m_ChefZ_PendingInputs || !m_ChefZ_PendingTools)
            return;

        BuildFromDef(m_ChefZ_PendingProc, m_ChefZ_PendingTr,
                     m_ChefZ_PendingInputs, m_ChefZ_PendingTools);
    }

    //==========================================================================
    // Der Aufbau
    //==========================================================================

    protected void BuildFromDef(notnull ChefZ_CompiledProcess proc,
                                notnull ChefZ_CompiledTransform tr,
                                notnull array<ref array<string>> inputClasses,
                                notnull array<string> toolClasses)
    {
        m_ChefZ_ProcessSym   = proc.processSym;
        m_ChefZ_ProcessId    = proc.id;
        m_ChefZ_TransformSym = tr.transformSym;
        m_ChefZ_TransformId  = tr.id;

        ResetVanillaFields();

        //--- Form pruefen (siehe Dateikopf) -----------------------------------
        int inputs = tr.inputs.Count();
        bool hasTools = proc.toolGroups.Count() > 0;

        if (inputs < 1)
        {
            Reject("der Transform hat keinen Eingang - es gaebe nichts zu verarbeiten");
            return;
        }

        if (inputs > ChefZ_ProcessingLimits.HANDCRAFT_MAX_INPUTS)
        {
            Reject("der Transform hat " + inputs.ToString() + " Eingaenge, Vanillas "
                + "Craftsystem kennt hoechstens "
                + ChefZ_ProcessingLimits.HANDCRAFT_MAX_INPUTS.ToString() + " (01 V12). "
                + "Fuer mehr Eingaenge ist STATION_ACTION oder STATION_TIMED die "
                + "richtige Ausfuehrungsform");
            return;
        }

        if (inputs == 1 && !hasTools)
        {
            Reject("der Transform hat einen Eingang und der Prozess nennt keine "
                + "Werkzeuggruppe. Vanillas Craftsystem braucht ZWEI Zutaten - es gaebe "
                + "nichts, womit der Spieler den Eingang kombinieren koennte. Entweder "
                + "eine Werkzeuggruppe am Prozess nennen oder auf STATION_ACTION "
                + "umstellen");
            return;
        }

        if (inputs == ChefZ_ProcessingLimits.HANDCRAFT_MAX_INPUTS && hasTools)
        {
            Reject("der Transform hat zwei Eingaenge UND der Prozess nennt eine "
                + "Werkzeuggruppe. Das Werkzeug waere ein dritter Zutatenplatz, und "
                + "Vanilla kennt genau zwei (01 V12). Entweder die Werkzeuggruppe "
                + "streichen oder auf STATION_ACTION umstellen");
            return;
        }

        if (inputClasses.Count() < inputs)
        {
            Reject("zu " + inputClasses.Count().ToString() + " von " + inputs.ToString()
                + " Eingaengen wurden Klassen erhoben - das ist ein Fehler der Bruecke, "
                + "nicht der Daten");
            return;
        }

        m_ChefZ_InputCount = inputs;
        m_ChefZ_ToolIndex  = -1;
        if (hasTools)
            m_ChefZ_ToolIndex = inputs;     // genau ein Platz bleibt uebrig

        //--- Zutatenplaetze ---------------------------------------------------
        for (int s = 0; s < inputs; s++)
        {
            ChefZ_CompiledSlot slot = tr.inputs.Get(s);
            if (!slot)
            {
                Reject("Eingangsslot " + s.ToString() + " ist leer");
                return;
            }

            array<string> classes = inputClasses.Get(s);
            if (!classes || classes.Count() == 0)
            {
                Reject("auf den Eingang \"" + slot.slotId + "\" passt keine einzige "
                    + "bekannte Klasse. Das Rezept koennte nie ausloesen und wird nicht "
                    + "registriert");
                return;
            }

            for (int c = 0; c < classes.Count(); c++)
                InsertIngredient(s, classes.Get(c));

            ApplyInputBehaviour(s);
        }

        if (m_ChefZ_ToolIndex >= 0)
        {
            if (toolClasses.Count() == 0)
            {
                Reject("der Prozess fordert eine Werkzeuggruppe, aber zu keiner der "
                    + "Gruppen ist eine Klasse bekannt. Ohne Werkzeugklasse haette das "
                    + "Rezept keinen zweiten Zutatenplatz");
                return;
            }

            for (int t = 0; t < toolClasses.Count(); t++)
                InsertIngredient(m_ChefZ_ToolIndex, toolClasses.Get(t));

            ApplyToolBehaviour(m_ChefZ_ToolIndex, proc);
        }

        //--- Ergebnisse -------------------------------------------------------
        m_ChefZ_PureStateChange = tr.pureStateChange;

        if (!BuildResults(tr))
            return;                          // Reject() ist bereits gelaufen

        //--- Anzeige, Animation, Softskill -----------------------------------
        m_Name = proc.displayName;
        if (m_Name == "")
            m_Name = FALLBACK_NAME;

        m_AnimationLength = proc.animationLength;
        if (m_AnimationLength <= 0.0)
            m_AnimationLength = DEFAULT_ANIMATION_LENGTH;

        m_Specialty      = proc.specialty;
        m_IsInstaRecipe  = false;

        // false ist Vanillas Default und die richtige Wahl: eine Zutat muss in
        // der Hand liegen. Ein Craftvorgang, der irgendwo im Rucksack
        // stattfindet, waere fuer den Spieler nicht nachvollziehbar - und
        // Vanillas eigene Nahrungsrezepte (PeelPotato) halten es genauso.
        m_AnywhereInInventory = false;

        /**
         * Wiederholbarkeit - abgeleitet, nicht erfunden.
         *
         * Es gibt kein JSON-Feld dafuer, und eines zu erfinden waere eine
         * Content-Entscheidung im Core. Die Ableitung ist stattdessen die
         * einzige, die sich aus den vorhandenen Daten begruenden laesst:
         * wird der erste Eingang nur ANTEILIG verbraucht, bleibt er liegen -
         * und dann ist "nochmal" die naheliegende naechste Handlung. Wird er
         * ganz verbraucht, gibt es nichts zu wiederholen.
         */
        m_ChefZ_Repeatable = false;

        ChefZ_CompiledSlot first = tr.inputs.Get(0);
        if (first && first.consumeMode == ChefZ_ConsumeMode.AMOUNT)
            m_ChefZ_Repeatable = true;

        m_ChefZ_Ready = true;
    }

    /**
     * Alle Vanillafelder auf ihren NEUTRALEN Wert setzen.
     *
     * Das ist die wichtigste Methode dieser Datei, und sie sieht wie
     * Buchhaltung aus. Grund: Vanillas Felder sind alle mit 0 vorbelegt, und
     * 0 bedeutet in fast jedem dieser Felder etwas ANDERES als "nichts tun":
     *
     *   m_MaxQuantityIngredient[i] = 0   ->  jede Zutat mit Menge > 0 faellt
     *                                        durch CheckConditions
     *   m_ResultSetQuantity[i]     = 0   ->  SetQuantity(0) am Ergebnis
     *   m_ResultSetHealth[i]       = 0   ->  das Ergebnis entsteht RUINIERT
     *   m_ResultInheritsColor[i]   = 0   ->  der Klassenname bekommt die
     *                                        "color" der Zutat angehaengt
     *   m_ResultReplacesIngredient[i] = 0 -> Eigenschaften UND Inventar der
     *                                        Zutat wandern ins Ergebnis
     *
     * Jedes Vanilla-Rezept schreibt diese Zeilen einzeln aus. Hier stehen sie
     * einmal, fuer alle. Faellt eine davon weg, entsteht kein Absturz und
     * keine Logzeile - es entsteht ein ruiniertes oder leeres Gericht.
     */
    protected void ResetVanillaFields()
    {
        int i;

        // Zutaten- und Ergebnislisten mit zuruecksetzen: InsertIngredient()
        // und AddResult() HAENGEN AN. Ein zweiter Aufbau auf derselben
        // Instanz - im Betrieb kommt er nicht vor, im Selbsttest schon -
        // haette sonst jede Klasse doppelt.
        for (i = 0; i < MAX_NUMBER_OF_INGREDIENTS; i++)
        {
            if (m_Ingredients[i])
                m_Ingredients[i].Clear();
            if (m_SoundCategories[i])
                m_SoundCategories[i].Clear();
        }
        m_NumberOfResults = 0;

        for (i = 0; i < MAX_NUMBER_OF_INGREDIENTS; i++)
        {
            m_MinQuantityIngredient[i] = -1;
            m_MaxQuantityIngredient[i] = -1;
            m_MinDamageIngredient[i]   = -1;
            m_MaxDamageIngredient[i]   = MAX_HEALTH_LEVEL;

            m_IngredientAddHealth[i]     = 0;
            m_IngredientSetHealth[i]     = -1;
            m_IngredientAddQuantity[i]   = 0;
            m_IngredientDestroy[i]       = false;
            m_IngredientUseSoftSkills[i] = false;
        }

        for (i = 0; i < MAXIMUM_RESULTS; i++)
        {
            m_ResultSetFullQuantity[i]    = false;
            m_ResultSetQuantity[i]        = -1;
            m_ResultSetHealth[i]          = -1;
            m_ResultInheritsHealth[i]     = -1;
            m_ResultInheritsColor[i]      = -1;
            m_ResultToInventory[i]        = -1;
            m_ResultReplacesIngredient[i] = -1;
            m_ResultUseSoftSkills[i]      = false;
        }
    }

    /**
     * Was Vanilla mit einem EINGANG tun soll: nichts.
     *
     * Der Verbrauch laeuft ueber den ChefZ-Verbrauchsplan in Do() - siehe
     * Dateikopf. Vanilla darf die Zutat deshalb weder loeschen noch ihre
     * Menge veraendern; taete es beides, waere sie doppelt verbraucht.
     *
     * Die Methode existiert trotzdem als eigene Zeile, damit die Entscheidung
     * an EINER Stelle steht und nicht als Abwesenheit von Code.
     */
    protected void ApplyInputBehaviour(int index)
    {
        m_IngredientDestroy[index]     = false;
        m_IngredientAddQuantity[index] = 0;
        m_IngredientAddHealth[index]   = 0;
        m_IngredientSetHealth[index]   = -1;
    }

    /**
     * Was Vanilla mit dem WERKZEUG tun soll: abnutzen.
     *
     * Genau hier wird die Zusage aus 11 E2 eingeloest - "Werkzeugschaden
     * gratis von Vanilla". m_IngredientAddHealth ist derselbe Weg, den
     * PrepareAnimal (-6) und PeelPotato (-0.5) gehen; der Betrag kommt aus
     * ChefZ_ProcessDef.toolDamage und damit aus den Daten.
     *
     * Bei toolDamage = 0 nutzt sich nichts ab, und das ist ein zulaessiger
     * Wert: ein Moerser verschleisst nicht.
     */
    protected void ApplyToolBehaviour(int index, notnull ChefZ_CompiledProcess proc)
    {
        m_IngredientDestroy[index]   = false;
        m_IngredientSetHealth[index] = -1;

        // Ueber eine float-Zwischenvariable: toolDamage ist int, das Feld ist
        // float, und eine implizite Umwandlung an einer Zuweisungsgrenze ist
        // in Enforce nirgends zugesichert. Dieselbe Schreibweise wie in
        // ChefZ_ActionProcessAtStation.ApplyToolDamage().
        float damage = proc.toolDamage;
        m_IngredientAddHealth[index] = -damage;

        // Softskills duerfen den Werkzeugverschleiss modifizieren - dieselbe
        // Zeile wie in PeelPotato. Auf den Eingaengen bleibt sie aus: die
        // Menge einer Zutat soll nicht vom Uebungsgrad des Spielers abhaengen.
        m_IngredientUseSoftSkills[index] = true;
    }

    /**
     * Ergebnisliste in Vanillaform.
     *
     * outputs zuerst, danach byproducts - und genau diese Reihenfolge steht
     * anschliessend in m_ChefZ_ResultDefs. results[i] in Do() gehoert damit
     * zu m_ChefZ_ResultDefs[i].
     *
     * @return false, wenn der Transform nicht registrierbar ist. Reject() ist
     *         dann bereits gelaufen.
     */
    protected bool BuildResults(notnull ChefZ_CompiledTransform tr)
    {
        m_ChefZ_ResultDefs.Clear();

        if (tr.pureStateChange)
        {
            // 11 §2: kein Output nennt eine Klasse. Es entsteht nichts, es
            // wird nur der ChefZ-Zustand gewechselt. Vanilla bekommt damit
            // NULL Ergebnisse - SpawnItems() laeuft dann durch eine Schleife
            // mit null Durchlaeufen.
            return true;
        }

        if (!AddOutputs(tr.outputs, false))
            return false;
        if (!AddOutputs(tr.byproducts, true))
            return false;

        if (m_NumberOfResults == 0)
        {
            Reject("der Transform erzeugt nichts und wechselt auch keinen Zustand - er "
                + "waere eine Zutatenvernichtungsmaschine (11 §7)");
            return false;
        }

        return true;
    }

    protected bool AddOutputs(notnull array<ref ChefZ_OutputDef> defs, bool isByproduct)
    {
        // Nur fuer die Meldungen. Ein Content-Autor sucht nach "byproducts",
        // nicht nach "Ergebnis Nummer drei".
        string kind = "outputs";
        if (isByproduct)
            kind = "byproducts";

        for (int i = 0; i < defs.Count(); i++)
        {
            ChefZ_OutputDef def = defs.Get(i);
            if (!def || def.cls == "")
                continue;

            /**
             * Ein Ergebnis mit ZUFALL laesst sich in Vanillas fester
             * Ergebnisliste nicht ausdruecken: m_ItemsToCreate steht bei der
             * Registrierung fest, SpawnItems() erzeugt IMMER alles.
             *
             * Es wird deshalb ausgelassen und einmal gemeldet - ausgelassen
             * und nicht "immer erzeugt", weil ein Nebenprodukt mit 20 Prozent
             * Chance, das zu 100 Prozent faellt, eine stille
             * Balanceverschiebung waere und niemandem auffiele.
             */
            if (def.chance < 1.0)
            {
                ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PROCESS,
                    "handcraft.chance." + m_ChefZ_TransformId + "." + def.cls,
                    "HANDCRAFT " + m_ChefZ_TransformId + ": " + kind + "-Eintrag \"" + def.cls
                    + "\" hat chance=" + def.chance.ToString() + ". Vanillas Craftsystem "
                    + "erzeugt seine Ergebnisliste immer vollstaendig; ein Zufallsergebnis "
                    + "ist dort nicht abbildbar und wird ausgelassen. Fuer Zufall ist "
                    + "STATION_ACTION oder STATION_TIMED die richtige Ausfuehrungsform.");
                continue;
            }

            /**
             * Qualitaetsvarianten (12 §2) waehlen die Ergebnisklasse anhand
             * der Stufe - also erst, wenn die Zutaten bekannt sind. Vanilla
             * braucht die Klasse schon bei der Registrierung.
             *
             * Die Basisklasse wird genommen, und der Autor erfaehrt es. Ein
             * stiller Rueckfall waere hier besonders teuer: das Gericht saehe
             * richtig aus und waere nur nie das gute.
             */
            if (def.variants && def.variants.Count() > 0)
            {
                ChefZ_Log.Once(ChefZ_LogLevel.WARN, ChefZ_LogChannel.PROCESS,
                    "handcraft.variants." + m_ChefZ_TransformId + "." + def.cls,
                    "HANDCRAFT " + m_ChefZ_TransformId + ": " + kind + "-Eintrag \"" + def.cls
                    + "\" nennt Qualitaetsvarianten. Vanillas Craftsystem legt die "
                    + "Ergebnisklasse bei der Registrierung fest, lange bevor eine Stufe "
                    + "berechnet werden kann - es entsteht immer die Basisklasse. Fuer "
                    + "stufenabhaengige Ergebnisse ist STATION_ACTION die richtige "
                    + "Ausfuehrungsform.");
            }

            if (m_NumberOfResults >= MAXIMUM_RESULTS)
            {
                Reject("der Transform nennt mehr als " + MAXIMUM_RESULTS.ToString()
                    + " Ergebnisse. Vanillas RecipeBase fuehrt genau so viele Plaetze; "
                    + "der Rest wuerde stillschweigend fehlen");
                return false;
            }

            int index = m_NumberOfResults;
            AddResult(def.cls);

            /**
             * Health-Vererbung vom ERSTEN Eingang (11 E2, "Health-Vererbung").
             *
             * Zutatenplatz 0 ist immer ein ChefZ-Eingang und nie das Werkzeug
             * - siehe die Abbildung im Dateikopf. Der Durchschnitt ueber
             * beide Plaetze (-2, wie PeelPotato) waere bei einem 1:1-Schritt
             * der Durchschnitt aus Zutat und MESSER, und der sagt ueber das
             * Ergebnis nichts.
             */
            m_ResultInheritsHealth[index] = 0;

            // Menge, Zustand, Frische und Temperatur setzt ChefZ in Do(). Der
            // Vanillaweg dafuer (m_ResultSetQuantity) kennt nur eine feste
            // Zahl und nicht quantityMode "fromInput" / "ratio" (08 §2).
            m_ChefZ_ResultDefs.Insert(def);
        }

        return true;
    }

    protected void Reject(string why)
    {
        m_ChefZ_Ready     = false;
        m_ChefZ_InitError = why;
    }

    //==========================================================================
    // Vanillas Rueckfragen
    //==========================================================================

    /**
     * Die ENTSCHEIDUNG (11 §5).
     *
     * Diese Methode ist der eigentliche Torwaechter. Vanillas Zutatenliste ist
     * nur ein grober Vorfilter aus KLASSENNAMEN - sie kann weder einen
     * Zustand noch eine Stufe noch eine Frische pruefen. Was ein ChefZ-Rezept
     * wirklich verlangt, steht im Selektor, und der wird hier ausgewertet:
     * mit demselben ChefZ_Matcher, der auch an einer Station bindet (11 E4).
     *
     * Sie laeuft auf BEIDEN Seiten. Der Client fragt sie fuer die Anzeige der
     * Craftaktion, der Server ueber PerformRecipeServer -> CheckRecipe
     * unmittelbar vor der Ausfuehrung. Entschieden wird serverseitig; der
     * Client kann eine Aktion hoechstens anbieten, die der Server danach
     * ablehnt.
     */
    override bool CanDo(ItemBase ingredients[], PlayerBase player)
    {
        /**
         * Die Bereitschaftspruefung steht VOR super, und das ist kein
         * Geschmack: RecipeBase.CanDo() greift ohne Nullpruefung auf
         * ingredients[0] und ingredients[1] zu. Ein unparametrisiertes
         * Rezept - das es zwischen Konstruktor und InitFromDef gibt - darf
         * dort nicht landen.
         *
         * Danach Vanillas eigene Bedingung: eine Zutat mit Anbauteilen wird
         * abgelehnt. Ein ChefZ-Ja ueber ein Vanilla-Nein hinweg waere der
         * falsche Weg.
         */
        if (!m_ChefZ_Ready)
            return false;

        if (!super.CanDo(ingredients, player))
            return false;

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        if (!mgr.IsReady())
            return false;

        /**
         * Kennt DIESE Seite ueberhaupt Transforms?
         *
         * Dieselbe Grosszuegigkeit und dieselbe Begruendung wie in
         * ChefZ_ActionProcessAtStation.IsProcessUsable(): kennt der Client
         * die Transformdaten nicht (OF-10), entscheidet allein der Server.
         *
         * Der Fall ist hier weitgehend theoretisch - ohne Transformdaten
         * waere dieses Rezept gar nicht erst registriert worden -, kostet
         * aber einen Map-Zugriff und schliesst eine ganze Fehlerklasse aus.
         */
        if (!mgr.HasAnyTransformFor(m_ChefZ_ProcessSym))
            return true;

        return Bind(ingredients, player);
    }

    /**
     * Die AUSFUEHRUNG (11 §5).
     *
     * Zu diesem Zeitpunkt hat Vanilla bereits erzeugt und noch nichts
     * verbraucht - siehe die Reihenfolge im Dateikopf. ChefZ tut hier genau
     * zwei Dinge, und beide gehoeren zusammen: die ChefZ-Schicht an die
     * Ergebnisse schreiben und die Eingaenge nach Plan verbrauchen.
     *
     * Laeuft nur auf dem Server. Vanilla ruft PerformRecipe ausschliesslich
     * ueber PluginRecipesManager.PerformRecipeServer(); der Torwaechter steht
     * trotzdem hier, weil 00 §5 keine Ausnahme kennt.
     */
    override void Do(ItemBase ingredients[], PlayerBase player,
                     array<ItemBase> results, float specialty_weight)
    {
        if (!m_ChefZ_Ready)
            return;

        if (!g_Game || !g_Game.IsServer())
            return;

        if (!Bind(ingredients, player))
        {
            /**
             * Praktisch unerreichbar: CheckRecipe() hat CanDo() im selben
             * Aufruf und im selben Tick bereits bejaht, und dazwischen liegt
             * keine Spielerhandlung.
             *
             * Wenn es doch geschieht, ist die Lage eindeutig: es ist etwas
             * ERZEUGT und nichts VERBRAUCHT worden. Die Ergebnisse werden
             * deshalb wieder entfernt. Das ist kein Verlust fuer den Spieler
             * - seine Zutaten liegen unangetastet da -, aber es verhindert
             * eine Vervielfaeltigung.
             */
            RollbackResults(results);

            ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.PROCESS,
                "handcraft.rebind." + m_ChefZ_TransformId,
                "HANDCRAFT " + m_ChefZ_TransformId + ": zwischen Pruefung und Ausfuehrung "
                + "passt der Transform nicht mehr (" + m_ChefZ_Match.failReason + "). Die "
                + "bereits erzeugten Ergebnisse wurden entfernt, die Zutaten sind "
                + "unveraendert.");
            return;
        }

        string err;
        if (!ChefZ_ProcessRunner.RunHandcraft(ToolOf(ingredients),
                                              m_ChefZ_Match,
                                              m_ChefZ_Entities,
                                              m_ChefZ_Snapshot,
                                              ActorIdOf(player),
                                              results,
                                              m_ChefZ_ResultDefs,
                                              err))
        {
            // RunHandcraft verbraucht ZULETZT und bricht davor ab. Bei false
            // ist also nichts verbraucht - dieselbe Lage wie oben.
            RollbackResults(results);

            ChefZ_Log.Once(ChefZ_LogLevel.ERR, ChefZ_LogChannel.PROCESS,
                "handcraft.run." + m_ChefZ_TransformId,
                "HANDCRAFT " + m_ChefZ_TransformId + " nicht ausgefuehrt: " + err
                + ". Die bereits erzeugten Ergebnisse wurden entfernt, die Zutaten sind "
                + "unveraendert.");
        }
    }

    /**
     * Wiederholbarkeit (11 §4).
     *
     * Aus den Daten abgeleitet, siehe BuildFromDef(). Vanillas Default ist
     * false, und das bleibt der Wert fuer jeden Schritt, der seinen Eingang
     * ganz verbraucht.
     */
    override bool IsRepeatable()
    {
        return m_ChefZ_Repeatable;
    }

    //==========================================================================
    // Bindung
    //==========================================================================

    /**
     * Die Zutaten auslesen und den Transform binden.
     *
     * Ergebnis liegt danach in m_ChefZ_Match, m_ChefZ_Snapshot und
     * m_ChefZ_Entities - drei wiederverwendete Puffer, damit ein Bildaufbau
     * mit einem Messer in der Hand keine Allokation kostet.
     *
     * Das WERKZEUG geht ausdruecklich NICHT in den Faktensatz. Es ist keine
     * Zutat: es soll weder gebunden noch verbraucht werden, und ein Messer
     * im Faktensatz koennte einen Slot bedienen, der es gar nicht meint.
     * Seine Rolle steht im ChefZ_ProcessContext, bei den Werkzeuggruppen.
     */
    protected bool Bind(ItemBase ingredients[], PlayerBase player)
    {
        if (!m_ChefZ_Snapshot)
            m_ChefZ_Snapshot = new ChefZ_FactSnapshot();
        if (!m_ChefZ_Entities)
            m_ChefZ_Entities = new array<ItemBase>();
        if (!m_ChefZ_Context)
            m_ChefZ_Context = new ChefZ_ProcessContext();
        if (!m_ChefZ_Match)
            m_ChefZ_Match = new ChefZ_TransformMatch();

        m_ChefZ_Snapshot.Clear();
        m_ChefZ_Entities.Clear();
        m_ChefZ_Context.Reset();
        m_ChefZ_Match.Reset();

        for (int i = 0; i < m_ChefZ_InputCount; i++)
        {
            ItemBase item = ingredients[i];
            if (!item)
                return false;

            int handle = m_ChefZ_Entities.Count();
            ChefZ_ItemFacts facts = m_ChefZ_Snapshot.Acquire();

            if (!ChefZ_FactCollector.CollectSingle(item, handle, facts))
            {
                m_ChefZ_Snapshot.DiscardLast();
                return false;
            }

            m_ChefZ_Entities.Insert(item);
        }

        if (m_ChefZ_Entities.Count() == 0)
            return false;

        m_ChefZ_Snapshot.SortStable();

        m_ChefZ_Context.actorIdentityId = ActorIdOf(player);
        AddToolGroups(ToolOf(ingredients), m_ChefZ_Context);

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        if (!mgr.FindTransform(m_ChefZ_ProcessSym, m_ChefZ_Context, m_ChefZ_Snapshot,
                               null, m_ChefZ_Match))
            return false;

        /**
         * Der Matcher liefert den SPEZIFISCHSTEN Transform des Prozesses -
         * nicht zwingend den, fuer den dieses Rezept registriert wurde.
         *
         * Diese Zeile ist deshalb kein Formalismus, sondern der Unterschied
         * zwischen "Rezept A fuehrt Transform A aus" und "Rezept A fuehrt
         * aus, was gerade am besten passt". Zwei Rezepte desselben Prozesses
         * mit derselben Zutatenliste wuerden sonst dasselbe tun.
         */
        if (m_ChefZ_Match.transformSym != m_ChefZ_TransformSym)
        {
            m_ChefZ_Match.matched    = false;
            m_ChefZ_Match.failReason = "es passt " + m_ChefZ_Match.transformId
                                     + " besser als " + m_ChefZ_TransformId;
            return false;
        }

        return true;
    }

    //! Das Werkzeug unter den Zutaten, oder null. Bei zwei Eingaengen gibt es
    //! keines - dann ist der Rueckgabewert null und jede Auswertung, die ihn
    //! benutzt, muss damit umgehen koennen.
    protected ItemBase ToolOf(ItemBase ingredients[])
    {
        if (m_ChefZ_ToolIndex < 0 || m_ChefZ_ToolIndex >= MAX_NUMBER_OF_INGREDIENTS)
            return null;
        return ingredients[m_ChefZ_ToolIndex];
    }

    /**
     * Werkzeuggruppen des gefuehrten Werkzeugs in den Kontext.
     *
     * Intern() statt Lookup() - und das ist der EINZIGE Unterschied zu
     * ChefZ_FactCollector.CollectToolGroups(). Er hat einen Grund:
     *
     * Dort laeuft der Aufruf bei jedem Zielwechsel des Fadenkreuzes, und
     * Intern() wuerde die Symboltabelle mit jedem Vanilla-Item fuellen, das
     * ein Spieler je in der Hand hatte. HIER laeuft er erst, nachdem Vanillas
     * Rezeptcache dieses Rezept fuer genau diese zwei Klassen ausgewaehlt hat
     * - der Wertevorrat ist also die Werkzeugliste dieses Rezepts.
     *
     * Der Gewinn ist konkret: eine Werkzeuggruppe mit allowSubclasses nennt
     * eine BASISklasse. Vanillas IsKindOf findet darueber jede Ableitung, und
     * die Ableitung selbst steht in keiner ChefZ-Deklaration. Mit Lookup()
     * waere sie unbekannt, der Werkzeugtest schluege fehl und das Rezept
     * erschiene nie - obwohl der Spieler das richtige Messer in der Hand
     * haelt.
     */
    protected void AddToolGroups(ItemBase tool, notnull ChefZ_ProcessContext ctx)
    {
        if (!tool)
            return;

        string type = tool.GetType();
        if (type == "")
            return;

        ChefZ_ToolRegistry tools = ChefZ_ToolRegistry.Get();
        if (!tools.IsReady())
            return;

        array<ChefZ_Sym> groups = new array<ChefZ_Sym>();
        tools.GetGroupsForClass(ChefZ_SymbolTable.Intern(type), groups);

        for (int i = 0; i < groups.Count(); i++)
            ctx.AddToolGroup(groups.Get(i));
    }

    //! 0, wenn kein Spieler beteiligt ist oder er keine Identitaet hat -
    //! dieselbe Regel wie in ChefZ_ActionProcessAtStation.IdentityOf().
    protected int ActorIdOf(PlayerBase player)
    {
        if (!player)
            return 0;

        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return 0;

        return identity.GetPlayerId();
    }

    //! Alles wieder entfernen, was Vanilla erzeugt hat. Nur fuer den Fall,
    //! dass ChefZ nach der Erzeugung abbricht - dann ist nichts verbraucht,
    //! und ein Ergebnis ohne Gegenleistung waere eine Vervielfaeltigung.
    protected void RollbackResults(array<ItemBase> results)
    {
        if (!results)
            return;

        for (int i = 0; i < results.Count(); i++)
        {
            ItemBase item = results.Get(i);
            if (item && !item.IsSetForDeletion())
                item.Delete();
        }

        results.Clear();
    }

    //==========================================================================
    // Auskuenfte - fuer Bruecke, Log und Selbsttest
    //==========================================================================

    bool ChefZ_IsReady()            { return m_ChefZ_Ready; }
    string ChefZ_GetInitError()     { return m_ChefZ_InitError; }
    string ChefZ_GetTransformId()   { return m_ChefZ_TransformId; }
    string ChefZ_GetProcessId()     { return m_ChefZ_ProcessId; }
    ChefZ_Sym ChefZ_GetTransformSym() { return m_ChefZ_TransformSym; }
    ChefZ_Sym ChefZ_GetProcessSym() { return m_ChefZ_ProcessSym; }
    int  ChefZ_GetInputCount()      { return m_ChefZ_InputCount; }
    int  ChefZ_GetToolIndex()       { return m_ChefZ_ToolIndex; }
    int  ChefZ_GetResultCount()     { return m_NumberOfResults; }
    bool ChefZ_IsPureStateChange()  { return m_ChefZ_PureStateChange; }

    //! Zahl der Klassennamen auf einem Zutatenplatz. Nur Diagnose.
    int ChefZ_GetIngredientClassCount(int index)
    {
        if (index < 0 || index >= MAX_NUMBER_OF_INGREDIENTS)
            return 0;

        array<string> list = m_Ingredients[index];
        if (!list)
            return 0;
        return list.Count();
    }

    string ChefZ_ToDebugString()
    {
        string s = m_ChefZ_TransformId + " (" + m_ChefZ_ProcessId + ")"
                 + " eingaenge=" + m_ChefZ_InputCount.ToString();

        if (m_ChefZ_ToolIndex >= 0)
            s = s + " werkzeugplatz=" + m_ChefZ_ToolIndex.ToString();

        s = s + " klassen=" + ChefZ_GetIngredientClassCount(0).ToString() + "/" + ChefZ_GetIngredientClassCount(1).ToString();

        if (m_ChefZ_PureStateChange)
            s = s + " ZUSTANDSWECHSEL";
        else
            s = s + " ergebnisse=" + m_NumberOfResults.ToString();

        if (m_ChefZ_Repeatable)
            s = s + " wiederholbar";

        return s;
    }
}
