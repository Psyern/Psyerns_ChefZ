//==============================================================================
// ChefZ_Edible_Base - Basis aller essbaren ChefZ-Items
//
// Entwurf: 06 §4.3 (Schnittstelle woertlich), 06 §2 ("Kein modded class
// Edible_Base"), 06 §6 (Zustandstabelle), V-B §1 (OF-01, Folge 1),
// 01 V9 (CanDecay ist auf Edible_Base false), 01 V10 (Consume/OnConsume sind
// virtuell - eine Ableitung genuegt).
//
// ---------------------------------------------------------------------------
// Der Grund, warum es diese Klasse gibt - und warum es KEIN modded class ist
// ---------------------------------------------------------------------------
// Der ChefZ-Zustand muss nur auf ChefZ-eigenen Klassen leben (OF-01/OF-12).
// Damit genuegt eine ABLEITUNG, und die spart die gesamte Kollisionsflaeche
// gegenueber anderen Food-Mods und jede Sync-Kost auf Vanilla-Nahrung:
//
//   - kein zusaetzliches Byte auf einem Vanilla-Steak,
//   - kein zusaetzlicher OnStoreSave-Block in fremden Spielstaenden,
//   - kein Override, das mit einem anderen Mod um dieselbe Methode streitet.
//
// Die geschlossene Liste der modded class des Core (00 §5) bleibt deshalb bei
// zwei Eintraegen, und keiner davon steht in dieser Datei.
//
// ---------------------------------------------------------------------------
// FUER CONTENT-AUTOREN: wie eine Klasse hier andockt
// ---------------------------------------------------------------------------
// Der Core bringt bewusst KEINEN CfgVehicles-Eintrag mit - er enthaelt keine
// Items, auch keine unsichtbaren. Ein Content-Modul deklariert deshalb:
//
//     config.cpp   class ChefZ_SmokedSausage : Edible_Base { ... };   (Vanilla-Basis)
//     script       class ChefZ_SmokedSausage extends ChefZ_Edible_Base { }
//
// Die Skriptklasse muss von ChefZ_Edible_Base erben, die Configklasse von
// einer Vanilla-Klasse. Wer eine gemeinsame Configbasis mit scope = 0 haben
// will, legt sie in SEINEM Modul an - dort ist sie Content, hier waere sie es
// auch.
//
// Ohne diese Skriptableitung traegt das Item keinen ChefZ-Zustand. Es ist dann
// ein gewoehnliches Vanilla-Nahrungsmittel - kein Fehler, nur weniger.
//
// ---------------------------------------------------------------------------
// Was hier NICHT steht
// ---------------------------------------------------------------------------
//   Anzeigename je Zustand           S12/S13
//
// Diese Zeilen sind bewusst LEER und nicht halb geraten. Ein halb gesetztes
// Verhalten waere schlimmer als gar keines: es saehe richtig aus.
//
// OnConsume ist seit S17 da (16 Behaelterrueckgabe, 17 Event) - siehe unten.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_Edible_Base extends Edible_Base
{
    //! Der gesamte ChefZ-Zustand. Siehe Kopf von ChefZ_ItemStateComponent,
    //! warum die Felder dort liegen und nicht hier.
    protected ref ChefZ_ItemStateComponent m_ChefZ_State;

    //! Klassenname -> "deklariert Food > FoodStageTransitions?".
    //! Siehe ChefZ_DeclaresCookTransitions(); die Antwort ist eine
    //! KLASSENeigenschaft und aendert sich zur Laufzeit nie.
    private static ref map<string, bool> s_ChefZ_CookTransitions;

    void ChefZ_Edible_Base()
    {
        // Zuerst der Block, dann die Registrierung: der Sync-Pfad
        // "m_ChefZ_State.<feld>" wird beim Registrieren aufgeloest.
        m_ChefZ_State = new ChefZ_ItemStateComponent();
        ChefZ_ItemStateComponent.RegisterNetSync(this);
    }

    //! Der Block selbst. ChefZ_ItemStateComponent.Of() geht ueber diesen
    //! Zugriff - er ist die einzige Stelle, an der die beiden Traegeraeste
    //! zusammenlaufen.
    ChefZ_ItemStateComponent ChefZ_StateBlock()
    {
        return m_ChefZ_State;
    }

    //==========================================================================
    // Die API aus 06 §4.3 - durchweg duenne Weiterleitung
    //==========================================================================

    //! Projektionsregel aus 06 §3.
    ChefZ_Sym ChefZ_GetState()
    {
        return ChefZ_ItemStateComponent.GetState(this);
    }

    bool ChefZ_SetState(ChefZ_Sym state, bool applyVanillaTransition = false)
    {
        return ChefZ_ItemStateComponent.SetState(this, state, applyVanillaTransition);
    }

    ChefZ_Sym ChefZ_GetQuality()
    {
        return m_ChefZ_State.ResolveQualitySym();
    }

    void ChefZ_SetQuality(ChefZ_Sym tier)
    {
        ChefZ_ItemStateComponent.SetQuality(this, tier);
    }

    float ChefZ_GetFreshness01()
    {
        return m_ChefZ_State.GetFreshness01();
    }

    /**
     * Der aktuelle Verderbfaktor, ohne ihn anzuwenden (14 §5).
     *
     * Fuer Debug-Text und Cookbook. Er wird bei jedem Aufruf NEU gerechnet und
     * nirgends gespeichert - 14 §7: "Der Multiplikator wird nicht persistiert
     * und nicht gecacht", damit eine Balancing-Aenderung sofort auf jedes
     * bestehende Item wirkt.
     */
    float ChefZ_GetDecayScale()
    {
        array<string> noTrace = null;
        return ChefZ_ItemDecay.ComputeScale(this, noTrace);
    }

    /**
     * Dasselbe mit aufgeschluesselter Produktkette (14 §3) - die Antwort auf
     * "warum haelt das Ding so lange".
     *
     * Die Liste wird hier ANGELEGT, wenn der Aufrufer keine mitbringt: eine
     * null-Liste heisst im Manager "keine Ablaufverfolgung", und das ist der
     * richtige Vorgabewert im Verfallstakt - aber der falsche fuer eine
     * Methode, deren einziger Zweck die Ablaufverfolgung ist.
     */
    float ChefZ_ExplainDecayScale(out array<string> trace)
    {
        if (!trace)
            trace = new array<string>();
        return ChefZ_ItemDecay.ComputeScale(this, trace);
    }

    void ChefZ_SetFreshness01(float v)
    {
        ChefZ_ItemStateComponent.SetFreshness01(this, v);
    }

    int ChefZ_GetPortions()
    {
        return m_ChefZ_State.GetPortions();
    }

    /**
     * Portionszaehler setzen (15 §3).
     *
     * @param max  die Hoechstzahl fuer die Anzeige "3 / 8". Auf DIESER Klasse
     *        wird sie ignoriert: sie hat keinen Platz dafuer, und ein zweiter
     *        gesyncter int auf jedem ChefZ-Item waere Sync-Last fuer eine
     *        Zahl, die nur ein Portionsgericht braucht. Getragen wird sie von
     *        ChefZ_PortionedFood_Base, das sie ueberschreibt.
     *
     * Der Parameter steht trotzdem HIER in der Signatur, damit der Aufrufer
     * (Applicator, ProcessRunner) einen ItemBase halten und rufen kann, ohne
     * vorher zu wissen, welcher der Aeste vor ihm steht - dieselbe
     * Ueberlegung wie bei applyVanillaTransition in ChefZ_SetState.
     *
     * @return true, wenn der Zaehler gesetzt wurde. false heisst "kein
     *         ChefZ-Zustandsblock oder nicht auf dem Server" und ist kein
     *         Fehler.
     */
    bool ChefZ_SetPortions(int count, int max = -1)
    {
        return ChefZ_ItemStateComponent.SetPortions(this, count);
    }

    //! Die Hoechstzahl. Auf dieser Klasse gleich dem aktuellen Stand - sie
    //! fuehrt keinen eigenen Wert (siehe ChefZ_SetPortions).
    int ChefZ_GetPortionsMax()
    {
        return ChefZ_GetPortions();
    }

    //! true nur auf ChefZ_PortionedFood_Base (15 §3). Hier immer false: eine
    //! Klasse ohne Entnahmeaktion ist kein Portionsgericht, auch wenn ihr
    //! Zaehler zufaellig gesetzt ist.
    bool ChefZ_IsBulk()
    {
        return false;
    }

    string ChefZ_GetReturnContainer()
    {
        return m_ChefZ_State.GetReturnContainer();
    }

    void ChefZ_SetReturnContainer(string cls)
    {
        ChefZ_ItemStateComponent.SetReturnContainer(this, cls);
    }

    //! true fuer jede ChefZ-Traegerklasse - unabhaengig davon, ob ein
    //! Ingredient-Binding existiert. Die andere Frage ("ist die Klasse
    //! deklariert?") beantwortet der ChefZ_IngredientManager.
    bool ChefZ_IsManaged()
    {
        return m_ChefZ_State != null;
    }

    //! Uebernimmt Zustand, Qualitaet, Frische, Portionen und Behaelterbindung.
    void ChefZ_InheritFrom(notnull ItemBase src, float freshnessCarry)
    {
        ChefZ_ItemStateComponent.InheritFrom(this, src, freshnessCarry);
    }

    //==========================================================================
    // Persistenz (06 §5, V-B §2 Folge 2: super zuerst, immer)
    //==========================================================================

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
        ChefZ_ItemStateComponent.Save(this, ctx);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        // Load() gibt immer true zurueck - ein unlesbarer ChefZ-Block darf den
        // Spieler nicht sein Item kosten. Der Rueckgabewert wird trotzdem
        // ausgewertet und nicht verworfen, damit eine spaetere Fassung ihn
        // benutzen KANN, ohne dass jemand die Aufrufstelle sucht.
        if (!ChefZ_ItemStateComponent.Load(this, ctx, version))
            return false;

        return true;
    }

    /**
     * Nach dem Laden: Hash -> Symbol -> Ordinal, dann synchronisieren.
     *
     * Bewusst hier und nicht in OnStoreLoad: zum Zeitpunkt von OnStoreLoad ist
     * das Item noch nicht vollstaendig in der Welt, und Vanilla ruft aus
     * demselben Grund an dieser Stelle sein eigenes Synchronize()
     * (Edible_Base.c:357).
     */
    override void AfterStoreLoad()
    {
        super.AfterStoreLoad();
        ChefZ_ItemStateComponent.ResolveAfterLoad(this);
    }

    //==========================================================================
    // Sync zum Client (06 §5: KEINE Spiellogik clientseitig)
    //==========================================================================

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();

        // Der Client frischt die Optik auf - mehr nicht. Er entscheidet
        // nichts, er erzeugt nichts, er verbraucht nichts (06 §5).
        //
        // EHRLICH BENANNTE ABWEICHUNG: Vanilla hat genau diese Zeile in
        // Edible_Base.OnVariablesSynchronized auskommentiert stehen
        // (Edible_Base.c:196, "//UpdateVisualsEx(); //performed on client
        // only"). Auf einer VANILLA-Klasse bleibt sie deshalb aus - dort
        // aendert sie ChefZ nicht.
        //
        // Auf der ChefZ-eigenen Klasse wird sie eingeschaltet, weil ohne sie
        // ein Zustandswechsel serverseitig sichtbar waere und clientseitig
        // nicht: der Spieler saehe rohes Fleisch, das laengst geraeuchert ist.
        // Sie kostet fast nichts - FoodStage.UpdateVisualsEx vergleicht
        // m_FoodStageTypeClientLast und tut nur bei einer echten Aenderung
        // etwas (FoodStage.c:524) - und sie greift ausschliesslich auf
        // bereits synchronisierte Daten zu.
        //
        // Edible_Base.UpdateVisualsEx prueft GetFoodStage() selbst
        // (Edible_Base.c:91); eine Klasse ohne FoodStages laeuft hier also
        // ins Leere statt in einen Nullzugriff.
        UpdateVisualsEx();
    }

    //==========================================================================
    // Vanilla-Anbindung
    //==========================================================================

    /**
     * DER SCHALTER, OHNE DEN NICHTS KOCHT (01 V4, Cooking.c:47).
     *
     * Vanilla laesst Kochbarkeit NICHT von den Daten folgen. Edible_Base gibt
     * `false` zurueck (Edible_Base.c:129, und schon ItemBase.c:2666), und JEDE
     * kochbare Vanilla-Nahrung schaltet sie in ihrer EIGENEN Klasse wieder ein
     * - Potato.c:3, Lard.c:3, CarpFilletMeat.c:3 und rund vierzig weitere, alle
     * mit demselben dreizeiligen `return true;`.
     *
     * ChefZ_Edible_Base hatte diesen Schalter nicht. Die Folge war kein
     * Fehlerbild, sondern eine STILLE Sperre: Cooking.ProcessItemToCook
     * (Cooking.c:47) ging an jeder ChefZ-Zutat mit eigener Skriptklasse vorbei,
     * die Vanilla-Garstufe blieb auf RAW stehen, und
     * ChefZ_RecipeEvaluator.CheckStages verlangt von jeder gebundenen
     * Pflichtzutat eine erlaubte Endstufe. Kein ON_STAGE-Rezept konnte fertig
     * werden. Nichts davon war statisch sichtbar - die Zeile fehlte einfach.
     *
     * ---------------------------------------------------------------------
     * Warum die Bedingung SO lautet und nicht `return true;`
     * ---------------------------------------------------------------------
     * Der Core hat keinen Content und darf deshalb keine Meinung darueber
     * haben, was gekocht gehoert. Er kann aber nachsehen, was die Klasse
     * MITBRINGT - und genau daraus folgt die Antwort:
     *
     *   1. GetFoodStage() != null. Das ist woertlich HasFoodStage()
     *      (ItemBase.c:2654), denn Edible_Base legt m_FoodStage nur dann an
     *      (Edible_Base.c:27). Ohne dieses Objekt ist `true` kein Feature,
     *      sondern ein Absturz: Cooking.UpdateCookingState ruft ungeprueft
     *      item_to_cook.GetNextFoodStageType(...) und das ist
     *      GetFoodStage().GetNextFoodStageType(...) (Edible_Base.c:605).
     *      Betrifft real ChefZ_ServedDish_Base - eine servierte Portion hat
     *      bewusst keinen Food-Knoten.
     *
     *   2. Die Klasse deklariert Food > FoodStageTransitions. Ohne Uebergaenge
     *      faellt FoodStage.GetNextFoodStageType auf FoodStageType.BURNED
     *      zurueck (FoodStage.c:472) - das ist die Content-Falle aus 01 V4.
     *      Wer keine Uebergaenge hat, wird hier gar nicht erst angefasst und
     *      liegt im Topf wie jedes andere Cargo-Item.
     *
     * Damit ist die Falle aus 01 V4 fuer jeden Erben dieser Klasse nicht mehr
     * erreichbar: kochbar zu sein SETZT die Uebergaenge VORAUS. Die
     * Validatorregel (chefzstage) bleibt trotzdem noetig - sie greift fuer
     * Content, das CanBeCooked() selbst ueberschreibt, und fuer Klassen, die
     * eine kochbare VANILLA-Skriptklasse erben (ChefZ_Butter erbt Lard).
     *
     * ---------------------------------------------------------------------
     * Was diese Bedingung fuer eine Zutat bedeutet, die nur roh gegessen wird
     * ---------------------------------------------------------------------
     * Sie bleibt unveraendert nicht kochbar. Petersilie, Salz und die uebrigen
     * Gewuerze haben keinen Food-Knoten, also keine Stufen und keine
     * Uebergaenge - Bedingung 1 und 2 sind beide falsch, und der Rueckgabewert
     * ist derselbe wie vorher. Eine Klasse wird durch diese Aenderung NUR dann
     * kochbar, wenn ihr Autor die Uebergaenge ausdruecklich hingeschrieben hat.
     *
     * ---------------------------------------------------------------------
     * Warum hier KEIN CanBeCookedOnStick() steht
     * ---------------------------------------------------------------------
     * Der Schalter steht in Vanilla direkt daneben (Edible_Base.c:134) und
     * bleibt hier bewusst unangetastet. Vier Gruende, in dieser Reihenfolge:
     *
     *   1. 01 §47 und 10 §85 sagen es ausdruecklich: "CookOnStick ist nicht
     *      gehookt". ChefZ mischt sich in den Spiess nicht ein.
     *   2. Vanilla koppelt die beiden Schalter NICHT. CaninaBerry.c und
     *      SambucusBerry.c liefern CanBeCooked() == true und
     *      CanBeCookedOnStick() == false. Eine abgeleitete Zusage waere also
     *      nicht einmal in Vanilla richtig - und aus den Uebergaengen ist sie
     *      nicht ableitbar, weil dort nichts ueber Spiesse steht.
     *   3. Eine automatische Kopplung wuerde jeder Sauce, jedem Teig und jedem
     *      Stueck Kaese das Spiessgaren erlauben. Das ist eine inhaltliche
     *      Entscheidung, und Entscheidungen ueber Inhalt trifft der Core nicht.
     *   4. Der Ausfall ist folgenlos: false ist der Vanilla-Default, die Aktion
     *      wird schlicht nicht angeboten (ActionCookOnStick.c:36). Anders als
     *      bei CanBeCooked() blockiert das kein Rezept. Heute ist der Zweig
     *      ohnehin unerreichbar - ActionCookOnStick braucht ein Item, das AM
     *      Stock haengt, und keine ChefZ-Configklasse deklariert einen
     *      inventorySlot.
     *
     * Ein Content-Modul, das Fleisch am Spiess will, schreibt auf SEINER
     * Klasse dieselben drei Zeilen, die Vanilla auf Lard schreibt - und
     * bekommt vom Core den Test dafuer geschenkt:
     *
     *     override bool CanBeCookedOnStick()
     *     {
     *         return ChefZ_Edible_Base.ChefZ_DeclaresCookTransitions(GetType());
     *     }
     */
    override bool CanBeCooked()
    {
        if (!GetFoodStage())
            return false;

        return ChefZ_DeclaresCookTransitions(GetType());
    }

    /**
     * "Deklariert diese Configklasse Food > FoodStageTransitions?"
     *
     * Oeffentlich, weil Content denselben Test fuer eigene Schalter braucht
     * (siehe CanBeCooked() oben) und weil zwei Fassungen derselben Frage
     * garantiert auseinanderlaufen.
     *
     * GECACHT, und zwar aus demselben Grund, den Vanilla selbst nennt
     * (ItemBase.c:2659: "so we don't have to parse configs all the time"):
     * CanBeCooked() laeuft je Cargo-Item und Kochtakt. Das Ergebnis haengt
     * ausschliesslich am Klassennamen und kann sich zur Laufzeit nicht aendern
     * - der Cache ist deshalb kein Risiko, sondern nur eine gesparte
     * Configsuche.
     *
     * Die Vererbung loest die Engine auf: eine Configklasse sieht den
     * Food-Knoten ihrer Elternklasse. Genau darauf baut Vanilla bei
     * HasFoodStage() (ItemBase.c:2654) und bei
     * SetupFoodStageTransitionMapping (FoodStage.c:167) ebenfalls.
     *
     * Ausfallpfad: ohne g_Game (sehr frueher Aufruf) lautet die Antwort false
     * UND wird nicht gemerkt - eine zufaellig falsche Antwort duerfte sonst
     * fuer den Rest der Sitzung stehenbleiben.
     */
    static bool ChefZ_DeclaresCookTransitions(string type)
    {
        if (type == "")
            return false;

        if (!s_ChefZ_CookTransitions)
            s_ChefZ_CookTransitions = new map<string, bool>();

        bool known;
        if (s_ChefZ_CookTransitions.Find(type, known))
            return known;

        if (!g_Game)
            return false;

        bool has = g_Game.ConfigIsExisting("CfgVehicles " + type + " Food FoodStageTransitions");
        s_ChefZ_CookTransitions.Insert(type, has);

        // Genau EINMAL je Klasse, beim ersten Nachsehen - danach antwortet der
        // Cache und es wird nicht mehr geloggt. Das ist die Zeile, die den
        // urspruenglichen Blocker im RPT sichtbar gemacht haette.
        if (ChefZ_Log.Enabled(ChefZ_LogChannel.COOK, ChefZ_LogLevel.DEBUG))
        {
            if (has)
                ChefZ_Log.Debug(ChefZ_LogChannel.COOK, type + ": Food > FoodStageTransitions vorhanden - kochbar (01 V4).");
            else
                ChefZ_Log.Debug(ChefZ_LogChannel.COOK, type + ": kein Food > FoodStageTransitions - NICHT kochbar. Die Klasse " + "bleibt im Kochgeraet auf ihrer Garstufe stehen; ein ON_STAGE-Rezept, " + "das sie als Pflichtzutat fuehrt, wird nie fertig (01 V4).");
        }

        return has;
    }

    /**
     * "Hat diese Klasse ueberhaupt Vanilla-Garstufen?"
     *
     * GetFoodStage() != null IST die Antwort auf HasFoodStage()
     * (ItemBase.c:2654): Edible_Base legt m_FoodStage genau dann an, wenn
     * HasFoodStage() beim Konstruieren true war (Edible_Base.c:27). Dieselbe
     * Gleichsetzung steht schon in CanBeCooked() oben.
     *
     * Bewusst diese Form und NICHT der Configaufruf: HasFoodStage() baut je
     * Aufruf eine Zeichenkette und fragt die Config
     * (string.Format + ConfigIsExisting). Die beiden Wachen darunter laufen
     * je Lebensmittel und je ProcessVariables-Takt - das ist der heisseste
     * Pfad, den ChefZ ueberhaupt beruehrt. Der Zeigervergleich kostet nichts
     * und sagt dasselbe.
     *
     * protected und nicht private: eine Content-Klasse, die CanDecay() oder
     * CanProcessDecay() selbst ueberschreibt, braucht dieselbe Wache - und
     * zwei Fassungen derselben Frage laufen garantiert auseinander (dasselbe
     * Argument wie bei ChefZ_DeclaresCookTransitions oben).
     */
    protected bool ChefZ_HasFoodStages()
    {
        return GetFoodStage() != null;
    }

    /**
     * 01 V9: CanDecay() ist auf Edible_Base false - eine neue ChefZ-Nahrung
     * verdirbt gar nicht, solange das niemand sagt. Gesagt wird es als
     * Datenfeld an der Zutat (ChefZ_IngredientDef.decays), nicht im Code.
     *
     * Salz und getrocknete Gewuerze sollen genau das: nie verderben. Der
     * Default bleibt deshalb false.
     *
     * ---------------------------------------------------------------------
     * VORFALL 31.08.2026 - 1925 Nullzugriffe in 80 Minuten Livetest
     * ---------------------------------------------------------------------
     * Fundstelle: script_2026-08-31_14-44-26.log, "NULL pointer to instance"
     * in Edible_Base.GetFoodStageType (Edible_Base.c:533,
     * "return GetFoodStage().GetFoodStageType();" - ungeschuetzt).
     * Betroffen waren ausschliesslich ChefZ-Klassen OHNE Food-Block:
     * ChefZ_PepperBerries 415x, ChefZ_WildGarlic 305x, ChefZ_Thyme 301x,
     * ChefZ_Rosemary 257x, ChefZ_CheeseFlatbread 254x, ChefZ_PotatoPancakes
     * 254x, ChefZ_FarmersBreakfast 139x.
     *
     * Der Weg dorthin, Zeile fuer Zeile:
     *
     *   ItemBase.ProcessVariables (ItemBase.c:4670) rechnet
     *       processDecay = foodDecay && CanDecay() && CanProcessDecay();
     *   In VANILLA endet das immer vor der zweiten Klammer, weil
     *   Edible_Base.CanDecay() false liefert (Edible_Base.c:730) - die
     *   Kurzschlussauswertung erreicht CanProcessDecay() gar nicht. GENAU
     *   DAS war der Schutz, den diese Ueberschreibung wegnimmt: sie liefert
     *   decays aus dem Zutatendatensatz, ohne zu pruefen, ob die Klasse den
     *   Mechanismus ueberhaupt hat, an dem Vanillas Verfall haengt.
     *
     * Deshalb die Wache. Sie nimmt niemandem etwas weg, den es gibt:
     * Vanillas Verfall arbeitet AUSSCHLIESSLICH ueber Stufenwechsel
     * (Edible_Base.ProcessDecay vergleicht m_LastDecayStage gegen
     * GetFoodStageType() und ruft ChangeFoodStage(ROTTEN)). Eine Klasse ohne
     * Stufen hat nichts, was verfallen koennte - "decays": true ist an ihr
     * eine Angabe ohne Gegenstueck.
     *
     * Was das Feld weiterhin tut: es bleibt der Datenschalter fuer JEDE
     * ChefZ-eigene Frischerechnung, die nicht am Vanilla-Takt haengt. Nur
     * ChefZ_ItemDecay.AdvanceFreshness haengt heute daran, und die laeuft
     * ohnehin nur aus ProcessDecay heraus - also nur dort, wo es Stufen
     * gibt. Fuer stufenlose Klassen tickt die Frische damit nicht, und das
     * ist keine Aenderung: sie tickte dort noch nie, weil ProcessVariables
     * vor der ersten Multiplikation mit einem Nullzugriff abbrach. Derselbe
     * Abbruch kostete diese Items bisher AUCH Nasswerden und Temperatur -
     * beides laeuft ab jetzt wieder (ItemBase.c:4672ff).
     */
    override bool CanDecay()
    {
        // Erst die Wache, dann die Daten: ein "decays": true darf nur wirken,
        // wo Vanilla ueberhaupt einen Verfallsmechanismus hat.
        if (!ChefZ_HasFoodStages())
            return false;

        ChefZ_IngredientInfo info = ChefZ_IngredientManager.Get().ResolveByName(GetType());
        if (!info)
            return super.CanDecay();
        return info.decays;
    }

    /**
     * 14 §5: "super && !PreservationManager.StopsDecay(...)".
     *
     * super zuerst und ohne Ausnahme: dort sitzen Vanillas Frost-Stopp und der
     * ROTTEN-Stopp (01 V9). ChefZ kann den Verfall damit nur ABSCHALTEN, nie
     * einschalten - ein gefrorenes Item verdirbt auch mit ChefZ nicht, und ein
     * verrottetes verrottet nicht noch einmal.
     *
     * Der zweite Test ist der Konservenschalter aus 14 E7 (stopsDecay). Er ist
     * bewusst von preventsRotten getrennt: eine Konserve soll nicht rotten,
     * ihr Gesundheitswert darf sich aber weiter veraendern duerfen - zwei
     * Schalter statt einem erlauben genau das, ohne dass der Core wissen muss,
     * was eine Konserve ist.
     *
     * ---------------------------------------------------------------------
     * Die Wache DAVOR - Guertel und Hosentraeger zum Vorfall 31.08.2026
     * ---------------------------------------------------------------------
     * Der eigentliche Riegel gegen die 1925 Nullzugriffe sitzt in CanDecay()
     * darueber; hier steht der zweite, und er ist nicht doppelt gemoppelt:
     *
     *   1. super.CanProcessDecay() IST die Absturzstelle. Vanilla schreibt
     *      dort "!GetIsFrozen() && (GetFoodStageType() != ROTTEN)"
     *      (Edible_Base.c:735), und GetFoodStageType() ruft ungeprueft
     *      GetFoodStage().GetFoodStageType() (Edible_Base.c:533). Ohne
     *      Stufen ist der Aufruf selbst der Fehler - wir duerfen ihn also
     *      gar nicht erst betreten.
     *   2. Die Kurzschlussauswertung in ProcessVariables ist eine Zusage
     *      VANILLAS, nicht unsere. Sie steht in einer Zeile, die jeder
     *      andere Mod ueberschreiben kann - und ChefZ laeuft auf Servern mit
     *      dreistelliger Modzahl. Genau so kam der Vorfall ans Licht: in der
     *      Kette stand ein fremdes "modded class ItemBase".
     *   3. Dieselbe Ueberlegung wie bei plan.stopsDecay in ProcessDecay()
     *      weiter unten: ein Schutz, der an einer Zusage haengt, die uns
     *      nicht gehoert, gehoert an BEIDE Stellen.
     */
    override bool CanProcessDecay()
    {
        // VOR super: der super-Aufruf ist die Stelle, die ohne Stufen wirft.
        if (!ChefZ_HasFoodStages())
            return false;

        if (!super.CanProcessDecay())
            return false;

        return !ChefZ_ItemDecay.StopsDecay(this);
    }

    /**
     * 14 E1 und 01 V9: EINE MULTIPLIKATION, dann super.
     *
     * Das ist der ganze Eingriff in den Verfall, und er ist mit Absicht so
     * klein. Alles, was Vanilla an dieser Stelle tut, bleibt unangetastet:
     *
     *   - GameConstants.DECAY_FOOD_<STUFE>_<ART>, alle Werte, unveraendert
     *   - die Zufallsstreuung (DECAY_TIMER_RANDOM_PERCENTAGE)
     *   - die Health-Kopplung (m_DecayDelta aus GetHealth01)
     *   - der Spielerbonus (DECAY_RATE_ON_PLAYER)
     *   - der globale Modifikator (GetFoodDecayModifier)
     *   - der Trocknen-statt-Verrotten-Zweig bei Obst
     *   - die Fallunterscheidung IsFruit / IsMushroom / IsMeat / Dose
     *
     * Kein einziger dieser Punkte ist hier nachgebaut, und keiner muss bei
     * einem DayZ-Update nachgezogen werden. Ein Faktor 0.25 bedeutet vierfache
     * Haltbarkeit - unabhaengig von Stufe und Nahrungsart. Genau die Aussage,
     * die Architekturplan §12 macht.
     *
     * Der Ausfallpfad ist der Vanilla-Pfad: liefert die Planung null (SAFE_MODE,
     * Config nie geladen, Client, unbekannter Fehler), geht das UNVERAENDERTE
     * delta an super. Es gibt keinen Zweig, auf dem ChefZ den Verfall
     * verschluckt, ohne es zu wollen (14 §8).
     */
    override void ProcessDecay(float delta, bool hasRootAsPlayer)
    {
        ChefZ_DecayPlan plan = ChefZ_ItemDecay.Plan(this, hasRootAsPlayer);
        if (!plan)
        {
            super.ProcessDecay(delta, hasRootAsPlayer);
            return;
        }

        if (plan.stopsDecay)
        {
            // 14 §6: "StopsDecay? -> return, gar kein Verfall." Auch keine
            // Frischefortschreibung - ein Item, das nicht verfaellt, altert
            // nicht.
            //
            // Diese Zeile ist zusaetzlich zu CanProcessDecay() vorhanden und
            // nicht doppelt gemoppelt: CanProcessDecay ist Vanillas Tuersteher
            // und kann von einem anderen Mod ueberschrieben werden. Der Schutz
            // gehoert an BEIDE Stellen, sonst haengt er an einer Zusage, die
            // uns nicht gehoert.
            return;
        }

        // ---- Die eine Zeile, um die es geht ----------------------------------
        super.ProcessDecay(delta * plan.scale, hasRootAsPlayer);
        // ----------------------------------------------------------------------

        ChefZ_ItemDecay.AfterVanilla(this, plan, delta);
    }

    /**
     * 14 E7, preventsRotten: der Verfall laeuft, die Stufe wechselt aber nicht
     * auf ROTTEN.
     *
     * Warum HIER und nicht nach dem super-Aufruf in ProcessDecay: Vanilla ruft
     * ChangeFoodStage(ROTTEN) unqualifiziert, also virtuell auf diesem Item
     * (Edible_Base.c:798, :868, :903). Der Uebergang laesst sich damit
     * VERHINDERN statt nachtraeglich zuruecknehmen - und das ist der
     * Unterschied zwischen "wechselt nicht" und "flackert":
     *
     * Vanilla wuerfelt m_DecayTimer nur dann neu aus, wenn
     * m_LastDecayStage != GetFoodStageType() ist (01 V9). Ein nachtraeglich
     * zurueckgesetzter Stufenwert liesse beide wieder gleich, der abgelaufene
     * Timer bliebe abgelaufen - und das Item wuerde bei JEDEM Tick erneut
     * verrotten und erneut zurueckgesetzt, mit einem Stufenwechsel-Ereignis je
     * Tick.
     *
     * Der Zweig kostet im Normalfall EINEN int-Vergleich. Die teure Abfrage
     * laeuft nur, wenn ueberhaupt jemand ROTTEN setzen will - und das ist auf
     * einem Item pro Leben hoechstens einmal der Fall.
     *
     * Er wirkt auf JEDEN ChangeFoodStage(ROTTEN), nicht nur auf den aus dem
     * Verfall. Das ist richtig so: Kochen erzeugt nie ROTTEN (es erzeugt
     * BURNED, 01 V4), und ein TransferFoodStage von einem verrotteten Item auf
     * eine Konserve waere ebenso wenig gemeint.
     */
    override void ChangeFoodStage(FoodStageType new_food_stage_type)
    {
        if (new_food_stage_type == FoodStageType.ROTTEN && ChefZ_ItemDecay.PreventsRotten(this))
        {
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.PRESERV, ChefZ_LogLevel.DEBUG))
                ChefZ_Log.Debug(ChefZ_LogChannel.PRESERV, GetType() + ": Uebergang nach ROTTEN unterdrueckt (preventsRotten, 14 E7). " + "Der Verfall laeuft weiter, die Garstufe bleibt stehen.");
            return;
        }

        super.ChangeFoodStage(new_food_stage_type);
    }

    /**
     * Der Verzehr (16 §5, VERZEHR; 16 E4; 17 §4).
     *
     * DREI Zeilen, und die Reihenfolge ist jede davon:
     *
     *   1. super zuerst, IMMER. Dort sitzt Vanillas Hitzeschaden beim Essen
     *      (Edible_Base.OnConsume ruft ProcessDirectDamage oberhalb von
     *      PlayerConstants.CONSUMPTION_DAMAGE_TEMP_THRESHOLD). Ihn zu
     *      ueberspringen waere eine stille Regelaenderung an Vanilla - und
     *      genau die soll ChefZ nirgends vornehmen (V-B §2, Folge 2).
     *   2. der Rest steht im ChefZ_ContainerService und nicht hier. Er wird
     *      auch von anderer Stelle gebraucht, und Invariante I5 vertraegt
     *      keine zweite Fassung.
     *
     * Warum HIER und nicht in EEDelete (16 E4): EEDelete feuert auch bei
     * Serverstopp, Cleanup, Adminloeschung und Item-Ersetzung. Behaelter
     * wuerden sich dort vermehren. Ein Gericht, das verdirbt oder geloescht
     * wird, erreicht diese Methode nie - und gibt deshalb nichts zurueck
     * (16 §7: "Sonst waere ein vergessener Teller Suppe eine Tellerquelle").
     *
     * Warum bei Quantity <= 0 und nicht bei jedem Bissen (16 E4): Vanilla
     * zieht die Menge VOR diesem Aufruf ab (Edible_Base.Consume ruft
     * AddQuantity(-amount) und dann OnConsume). Die Bedingung ist damit hier
     * pruefbar - und "jeder Bissen ergibt einen Teller" waere ein
     * Duplikationsexploit erster Ordnung.
     *
     * Auf einer VANILLA-Nahrung laeuft nichts davon: diese Methode gehoert
     * einer ChefZ-eigenen Klasse (06 §2), und ein Vanilla-Steak kommt hier nie
     * vorbei.
     */
    override void OnConsume(float amount, PlayerBase consumer)
    {
        super.OnConsume(amount, consumer);

        ChefZ_ContainerService.OnFoodConsumed(this, amount, consumer);
    }

    /**
     * 06 E5 und 06 E3 - beide Faelle laufen durch diese eine Methode.
     *
     * Vanilla ruft sie serverseitig bei jedem Stufenwechsel
     * (FoodStage.c:511 -> Edible_Base.c:630). Zwei Faelle sind zu trennen:
     *
     *   1. ChefZ projiziert gerade seinen Zustand auf die Vanilla-Stufe.
     *      Das ist reine BUCHHALTUNG - Vanillas Agentenbereinigung darf hier
     *      NICHT laufen, sonst loescht ein Verwaltungsvorgang stillschweigend
     *      Krankheitserreger (06 E3). Die Visuals sollen trotzdem stimmen.
     *
     *   2. Vanilla selbst hat die Stufe geaendert - gekocht, verbrannt,
     *      verrottet. Dann laeuft die volle Vanilla-Kette, und bei BURNED oder
     *      ROTTEN gewinnt Vanilla ueber das ChefZ-Overlay (06 E5).
     *
     * Warum die Unterscheidung ueber einen Merker laeuft und nicht ueber zwei
     * verschiedene Vanilla-Methoden: in 1.29 ist ChangeFoodStage() woertlich
     * ein Aufruf von SetFoodStageType() (FoodStage.c:506), und beide loesen
     * OnFoodStageChange aus. Die Trennung, die 06 E3 verlangt, ist also nur
     * HIER moeglich - auf der Klasse, die uns gehoert. Siehe ProjectOnto().
     */
    override void OnFoodStageChange(FoodStageType stageOld, FoodStageType stageNew)
    {
        if (m_ChefZ_State && m_ChefZ_State.IsProjecting())
        {
            UpdateVisualsEx();
            return;
        }

        super.OnFoodStageChange(stageOld, stageNew);

        // Ueber eine int-Zwischenvariable: der Zustandsblock liegt zwar in
        // 4_World, seine Zahlen kommen aber aus ChefZ_VanillaStage (1_Core)
        // und sind dort bewusst kein Enum (05 §3.3).
        int stageNewValue = stageNew;
        ChefZ_ItemStateComponent.OnVanillaStageChanged(this, stageNewValue);
    }
}
