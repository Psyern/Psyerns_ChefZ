//==============================================================================
// ChefZ_Apiary - die Skriptklassen der Imkerei (Slice "apiary").
//
// Andockregel woertlich aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/
// Processing/ChefZ_ProcessingStation_Base.c:
//
//     config.cpp   class ChefZ_Beehive : Inventory_Base { ... };
//     JSON/Rang 2  { "kind":"station", "records":[{ "id":"ChefZ_Beehive" }] }
//     Skript       class ChefZ_Beehive extends ChefZ_ProcessingStation_Base {}
//
// Bis auf EINE Ueberschreibung steht hier nichts ausser diesen Bindungen. Die
// Ausnahme ist der Bienenstich, und sie ist keine: sie ist die Bauform, die
// der Core an ChefZ_OnStationActionFinished ausgeschrieben vorgibt.
//
// ---------------------------------------------------------------------------
// DER BIENENSTICH (Auftrag: "[Bienenstock oeffnen] -> Smoker in der Hand
// haelt Schaden ab")
// ---------------------------------------------------------------------------
// Frueher stand an dieser Stelle die Begruendung, warum die Regel NICHT
// baubar sei: es fehlte ein Punkt im Ablauf, an dem der handelnde Spieler und
// sein Handinhalt gleichzeitig bekannt sind. Den gibt es jetzt -
// ChefZ_ProcessingStation_Base.ChefZ_OnStationActionFinished(PlayerBase actor,
// ItemBase inHands, ChefZ_Sym process, int outcome). Die Notloesung
// "Imkerpfeife als Pflichtwerkzeug" ist damit hinfaellig und ist gefallen:
// PROCESS_HARVEST_HIVE fuehrt keine toolGroups mehr, die Ernte ist ohne
// Pfeife ausloesbar - sie tut nur weh.
//
// Was hier NICHT entstanden ist, und das ist der Punkt: keine eigene Action,
// keine modded class, keine Zeile im Core. Der Haken ist gewoehnliche
// Vererbung auf einer Klasse, die ohnehin von ChefZ_ProcessingStation_Base
// erbt. Die Liste der modded class des Projekts bleibt bei sieben.
//
// KEINE Essaktion. Keine Klasse dieses Slice ist Nahrung - das Ergebnis der
// Kette ist Vanillas Honey. Die Begruendung steht an ChefZ_HoneycombFrame_Base
// in der config.cpp.
//
// KEIN ChefZ_HasHeat. Der Trockenrahmen laesst die Basisantwort "nein"
// stehen, weil Trocknen keine Waerme braucht; hier gilt dasselbe aus demselben
// Grund. Kein Prozess dieses Slice setzt requiresHeat.
//
// Der Stock fasst Vanillas Kochkette an keiner Stelle an (11 E6).
//
// Layer: 4_World.
//==============================================================================

//! Der Bienenstock (Auftrag: "Bienenstock"). Was er anbietet, steht im
//! Stationsdatensatz (Config/Processing/Apiary_Stations.json), was woraus
//! wird im Transform (Config/Processing/Apiary_Hive.json) - nicht hier.
//!
//! Das EINZIGE, was hier steht, ist die Reaktion des Volkes auf einen
//! geoeffneten Stock. Siehe ChefZ_OnStationActionFinished().
class ChefZ_Beehive extends ChefZ_ProcessingStation_Base
{
    //==========================================================================
    // Der Bienenstich - Zahlen und Namen
    //==========================================================================

    //! Was das abfangende Stueck an Schaden nimmt: die Imkerpfeife (verbrannter
    //! Brennstoff), sonst Handschuhe oder Kopfbedeckung (Stachel in der Faser).
    //!
    //! EIN Wert fuer alle drei, wie in Vanillas Vorlage: dort geht
    //! m_AdjustedDamageToMiningItemEachDrop an das Werkzeug in der Hand UND an
    //! die Handschuhe (CAContinuousMineWood.c:78 und :231). Zwei getrennte
    //! Zahlen waeren zwei Zahlen, die auseinanderlaufen, ohne dass die
    //! Unterscheidung je etwas ausdruecken wuerde.
    //!
    //! 2.0 ist NICHT frei gewaehlt: genau so viel stand vorher als
    //! toolDamage = 2 an PROCESS_HARVEST_HIVE. Der Verschleiss der Pfeife je
    //! Ernte bleibt damit unveraendert - nur der Ort, an dem er entsteht, hat
    //! sich verschoben (siehe unten, "warum toolDamage jetzt 0 ist").
    static const float CHEFZ_STING_ABSORB_DAMAGE = 2.0;

    //! Schockschaden eines Stichs ins ungeschuetzte Gesicht.
    //!
    //! WARUM SCHOCK UND KEINE BLUTUNG AM KOPF: Vanillas Zonentabelle
    //! (BleedingSourcesManagerBase.Init) fuehrt "Head" mit
    //! BLEEDING_SOURCE_FLOW_MODIFIER_HIGH und dem schweren Partikel
    //! "BleedingSourceEffect". Das ist die Darstellung einer aufgerissenen
    //! Schlagader. Bienen reissen nichts auf, sie stechen - das tut weh und
    //! macht benommen, und Vanillas Waehrung dafuer ist Schock
    //! (PlayerBase.AddHealth("","Shock",-x), so benutzt in Contamination3.c:49
    //! und CarScript.c:1642).
    //!
    //! 15.0 auf einer Leiste, deren staerkste Einzelquelle in Vanilla -100
    //! setzt (Contamination3): spuerbar, blickt kurz weg, wirft niemanden um,
    //! und die Shock-Modifier fuellen sie von selbst wieder auf.
    static const float CHEFZ_STING_SHOCK = 15.0;

    //! Die Werkzeuggruppe, die den Schutz traegt. Ueber die GRUPPE und nicht
    //! ueber IsKindOf("ChefZ_BeeSmoker"): eine Gruppe ist ein offener
    //! Namensraum (11 E8), ein fremdes Modul darf eine eigene Pfeife
    //! beisteuern und bekommt den Schutz damit geschenkt. Ein hart
    //! verdrahteter Klassenname koennte das nie.
    static const string CHEFZ_STING_SMOKER_GROUP = "BEE_SMOKER";

    //! Der EINE Prozess, an dem gestochen wird. Der Haken feuert fuer JEDEN
    //! Prozess dieser Station - ohne diese Pruefung staeche es auch beim
    //! Einhaengen eines Leerraehmchens (PROCESS_TEND_HIVE), und das waere
    //! falsch: dabei ist der Deckel zu.
    static const string CHEFZ_STING_PROCESS = "PROCESS_HARVEST_HIVE";

    //==========================================================================
    // Die Slotnamen der Schutzausruestung
    //==========================================================================
    //
    // BELEGT, NICHT GERATEN. Ein falscher Slotname liefert bei
    // FindAttachmentBySlotName still null - die Ausruestung schuetzte dann
    // nie, und niemand saehe warum. Die Engine zieht die Namen aus CfgSlots
    // (InventorySlots.c, Kommentar an GetSlotIdFromString: "searches for class
    // entry Slot_##slot_name"). Alle drei stehen in Vanillas eigener
    // config.cpp - "scripts - 1.29/config.cpp", class CfgSlots:
    //
    //   Slot_Headgear   Zeile 38
    //   Slot_Mask       Zeile 45
    //   Slot_Gloves     Zeile 72
    //
    // Gegenprobe im Skript: Vanilla ruft FindAttachmentBySlotName("Gloves")
    // (CAContinuousMineWood.c:230) und ("Headgear") an neun Stellen.
    //
    // "Body" fehlt bewusst. Ein Hemd traegt jeder - waere es Schutz, waere die
    // Regel fuer alle folgenlos, und die Imkerpfeife bliebe so bedeutungslos
    // wie zuvor.
    //==========================================================================

    static const string CHEFZ_SLOT_GLOVES   = "Gloves";
    static const string CHEFZ_SLOT_HEADGEAR = "Headgear";
    static const string CHEFZ_SLOT_MASK     = "Mask";

    /**
     * Das Volk verteidigt sich, wenn jemand den Deckel abnimmt.
     *
     * DIE REGEL IN EINEM SATZ: Wer PROCESS_HARVEST_HIVE mit der Imkerpfeife in
     * der Hand abschliesst, kommt ungestochen davon (der Rauch beruhigt, die
     * Pfeife nimmt den Verschleiss); wer sie nicht haelt, wird an jeder
     * ungeschuetzten Koerperregion gestochen - Handschuhe decken die Haende,
     * Kopfbedeckung oder Maske den Kopf, und was nackt ist, blutet
     * beziehungsweise schwillt an.
     *
     * -------------------------------------------------------------------------
     * DIE BAUFORM IST VANILLAS, NICHT ERFUNDEN
     * -------------------------------------------------------------------------
     * CAContinuousMineWood.DamagePlayersHands() (scripts - 1.29,
     * 4_World/.../ActionComponents/CAContinuousMineWood.c:228-250) macht genau
     * drei Dinge, und diese Methode macht dieselben drei:
     *
     *   1. Werkzeug in der Hand faengt ab und nimmt den Schaden
     *      (dort: action_data.m_MainItem, Zeile 78; hier: die Pfeife).
     *   2. Sonst faengt die getragene Ausruestung ab, geprueft mit
     *      FindAttachmentBySlotName + !IsDamageDestroyed(), und nimmt
     *      denselben Schaden (dort: Gloves, Zeile 230).
     *   3. Sonst trifft es den Spieler (dort: Blutung am Unterarm,
     *      Zeile 237-247).
     *
     * ZWEI ABWEICHUNGEN, beide benannt und begruendet:
     *
     * (a) ZWEI REGIONEN statt einer. Vanilla kennt beim Holzhacken nur die
     *     Haende; Bienen gehen an jede freie Haut, und beim Arbeiten am Stock
     *     sind zwei Flaechen frei: Haende und Kopf. Jede Region laeuft fuer
     *     sich durch dieselben drei Schritte. Eine einzige Kaskade ueber alle
     *     Stuecke haette bedeutet, dass eine Basecap die Haende schuetzt.
     *
     * (b) KEIN WURF. Vanilla wuerfelt 1 aus 10 (Zeile 238), weil sein
     *     Ereignis je Baum vielfach eintritt - eine sichere Blutung je
     *     Materialbrocken waere absurd. Hier tritt es EINMAL je Ernte ein,
     *     und alle 25 Sekunden ein Zehntel Stich waere eine Regel, die
     *     niemand je bemerkt: die Imkerpfeife bliebe sinnlos, und der Auftrag
     *     ist genau an ihr aufgehaengt. Sicher statt gewuerfelt ist ausserdem
     *     lernbar - der Spieler sieht den Zusammenhang beim ersten Mal.
     *     Ausweichen kostet ihn eine Blechdose oder einen Hut.
     *
     * -------------------------------------------------------------------------
     * WARUM ES BEI JEDEM AUSGANG STICHT
     * -------------------------------------------------------------------------
     * outcome wird NICHT geprueft. Was die Bienen aufbringt, ist der offene
     * Stock, nicht die Ausbeute - und offen war er, sobald die Aktion
     * durchgelaufen ist. Bei NO_MATCH (jemand hat den Stock waehrenddessen
     * leergeraeumt) und RUN_FAILED ist der Deckel genauso ab.
     *
     * Kein switch ueber outcome bedeutet ausserdem: kein vergessener
     * default-Zweig, wenn der Core die Liste erweitert. Der Kopf von
     * ChefZ_StationActionOutcome warnt ausdruecklich davor.
     *
     * -------------------------------------------------------------------------
     * WARUM toolDamage AN PROCESS_HARVEST_HIVE JETZT 0 IST
     * -------------------------------------------------------------------------
     * ChefZ_ActionProcessAtStation.ApplyToolDamage() beschaedigt
     * action_data.m_MainItem - also WAS AUCH IMMER in der Hand liegt, ohne
     * Ruecksicht darauf, ob es zu einer toolGroups des Prozesses gehoert. Das
     * war harmlos, solange die Aktion ohne Pfeife gar nicht erschien. Ohne
     * Pflichtwerkzeug waere es ein Fehler: der Stock fraesse an einem
     * Gewehr, einer Feldflasche oder was der Spieler sonst gerade haelt.
     *
     * Der Verschleiss wandert deshalb hierher, wo geprueft ist, dass es
     * wirklich die Pfeife ist. Die Hoehe bleibt dieselbe (2.0).
     *
     * -------------------------------------------------------------------------
     * SERVERSEITIG
     * -------------------------------------------------------------------------
     * Blutungen, Schock und Itemschaden sind autoritative Zustaende (00 §5).
     * Der Aufrufer prueft g_Game.IsServer() bereits
     * (ChefZ_ActionProcessAtStation.NotifyStation); die Wache steht trotzdem
     * hier - der Core empfiehlt das ausdruecklich fuer Ueberschreibungen, die
     * auch aus eigenem Code gerufen werden koennten.
     */
    override void ChefZ_OnStationActionFinished(PlayerBase actor, ItemBase inHands, ChefZ_Sym process, int outcome)
    {
        super.ChefZ_OnStationActionFinished(actor, inHands, process, outcome);

        if (!g_Game || !g_Game.IsServer())
            return;

        if (!actor)
            return;

        // Nur beim Ernten. Lookup() und nicht Intern(): ein Prozessname, den
        // niemand deklariert hat, ist INVALID und trifft dann auf keinen
        // gueltigen process - genau die stille, richtige Antwort.
        ChefZ_Sym harvest = ChefZ_SymbolTable.Lookup(CHEFZ_STING_PROCESS);
        if (!ChefZ_SymbolTable.IsValid(harvest) || process != harvest)
            return;

        // Schritt 1: die Pfeife in der Hand. Ruiniert zaehlt sie nicht - eine
        // durchgebrannte Pfeife raucht nicht mehr, dieselbe Pruefung, die
        // Vanilla an den Handschuhen anlegt.
        if (inHands && !inHands.IsDamageDestroyed() && ChefZ_IsBeeSmoker(inHands))
        {
            inHands.DecreaseHealth("", "", CHEFZ_STING_ABSORB_DAMAGE);
            ChefZ_LogSting("Rauch: die Imkerpfeife hat das Volk beruhigt.");
            return;
        }

        // Schritt 2 und 3, je Region getrennt.
        bool stungHands = !ChefZ_AbsorbSting(actor, CHEFZ_SLOT_GLOVES);

        // Kopf: erst die Kopfbedeckung, dann die Maske - und ausgeschrieben
        // statt als &&-Ausdruck. Beide Aufrufe haben eine WIRKUNG, sie
        // beschaedigen das gefundene Stueck; ob Enforce den zweiten Operanden
        // bei kurzgeschlossener Auswertung noch anfasst, soll an dieser Stelle
        // niemand nachschlagen muessen. Genau eines der beiden Stuecke nimmt
        // Schaden, nie beide.
        bool stungHead = true;
        if (ChefZ_AbsorbSting(actor, CHEFZ_SLOT_HEADGEAR))
            stungHead = false;
        else if (ChefZ_AbsorbSting(actor, CHEFZ_SLOT_MASK))
            stungHead = false;

        if (stungHands)
            ChefZ_StingForearm(actor);

        if (stungHead)
        {
            // Ueber eine float-Zwischenvariable, aus demselben Grund, den
            // ChefZ_ActionProcessAtStation.ApplyToolDamage() ausschreibt: eine
            // implizite Umwandlung an einer Aufrufgrenze ist in Enforce
            // nirgends zugesichert.
            float shock = CHEFZ_STING_SHOCK;
            actor.AddHealth("", "Shock", -shock);
        }

        ChefZ_LogSting("Ohne Pfeife geoeffnet: haende=" + ChefZ_YesNo(stungHands) + " kopf=" + ChefZ_YesNo(stungHead) + ".");
    }

    /**
     * Traegt der Spieler an diesem Slot etwas, das den Stich abfangen kann?
     *
     * true = abgefangen, das Stueck hat den Schaden genommen. false = die
     * Region ist frei (kein Stueck, oder ein ruiniertes).
     *
     * Der Slotname wird nicht validiert. Ein Tippfehler faende hier nichts,
     * und das saehe genauso aus wie "der Spieler traegt nichts" - deshalb
     * stehen die drei Namen oben als Konstanten mit Fundstelle und werden
     * nirgends sonst geschrieben.
     */
    protected bool ChefZ_AbsorbSting(notnull PlayerBase actor, string slotName)
    {
        ItemBase worn = ItemBase.Cast(actor.FindAttachmentBySlotName(slotName));
        if (!worn || worn.IsDamageDestroyed())
            return false;

        worn.DecreaseHealth("", "", CHEFZ_STING_ABSORB_DAMAGE);
        return true;
    }

    /**
     * Stiche an den nackten Unterarmen - Vanillas Zeilen 240-247, ohne den
     * Wurf davor.
     *
     * Die Links-Rechts-Ausweiche ist Vanillas und wird gebraucht:
     * AttemptAddBleedingSourceBySelection liefert false, wenn dieselbe Zone
     * schon blutet (CanAddBleedingSource prueft die Bitmaske). Ohne die
     * zweite Seite waere jede weitere Ernte am schon blutenden Arm folgenlos.
     *
     * Die Zonennamen sind Vanillas eigene, registriert in
     * BleedingSourcesManagerBase.Init() Zeile 43 und 45 - beide mit
     * BLEEDING_SOURCE_FLOW_MODIFIER_LOW und dem leichten Partikel
     * "BleedingSourceEffectLight". Das ist im ganzen Zonenbestand die
     * schwaechste Blutung, die Vanilla kennt, und genau die richtige
     * Groessenordnung fuer Stiche.
     */
    protected void ChefZ_StingForearm(notnull PlayerBase actor)
    {
        BleedingSourcesManagerServer bleeding = actor.GetBleedingManagerServer();
        if (!bleeding)
            return;

        if (!bleeding.AttemptAddBleedingSourceBySelection("LeftForeArmRoll"))
            bleeding.AttemptAddBleedingSourceBySelection("RightForeArmRoll");
    }

    /**
     * Gehoert das Item der Werkzeuggruppe BEE_SMOKER an?
     *
     * Derselbe Weg, den ChefZ_FactCollector.AddToolGroupsOf() geht:
     * Lookup() auf den Klassennamen (nicht Intern - die Symboltabelle soll
     * nicht mit jedem Vanilla-Item wachsen, das jemand am Stock in der Hand
     * hielt), dann die Registry fragen. IsToolOfGroup() beruecksichtigt
     * allowSubclasses und damit auch abgeleitete Pfeifen fremder Module.
     */
    protected bool ChefZ_IsBeeSmoker(notnull ItemBase item)
    {
        ChefZ_ToolRegistry tools = ChefZ_ToolRegistry.Get();
        if (!tools || !tools.IsReady())
            return false;

        ChefZ_Sym group = ChefZ_SymbolTable.Lookup(CHEFZ_STING_SMOKER_GROUP);
        if (!ChefZ_SymbolTable.IsValid(group))
            return false;

        string type = item.GetType();
        if (type == "")
            return false;

        ChefZ_Sym classSym = ChefZ_SymbolTable.Lookup(type);
        if (!ChefZ_SymbolTable.IsValid(classSym))
            return false;

        return tools.IsToolOfGroup(classSym, group);
    }

    //! Die Spur des Stichs. Hinter der Kanalwache, weil die Zeichenkette sonst
    //! auch dann entstuende, wenn niemand sie liest (18 §2).
    protected void ChefZ_LogSting(string msg)
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
            return;
        ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS, "Bienenstock " + GetType() + ": " + msg);
    }

    //! "ja"/"nein" statt "1"/"0" - im Log liest sich ein Stich als Aussage,
    //! nicht als Flagge.
    protected string ChefZ_YesNo(bool value)
    {
        if (value)
            return "ja";
        return "nein";
    }
}

//! Der Bausatz (Auftrag: "Beehive_Kit"). Reines Traggut ohne ChefZ-Zustand;
//! er wird von TR_RaiseBeehive verbraucht und ist bis dahin nur schwer.
class ChefZ_BeehiveKit extends ItemBase {}

//! Gemeinsame Skriptbasis der vier Raehmchen. Sie traegt bewusst KEINEN
//! ChefZ-Zustand: der Unterschied zwischen leer, verdeckelt, voll und
//! entdeckelt ist die KLASSE, nicht eine Zustandsvariable auf einer Klasse.
//! Vier Klassen sind hier richtiger als eine mit vier Zustaenden, weil jeder
//! Schritt ein eigener Transform mit eigenem Ein- und Ausgang ist und die
//! Stufen verschiedene Gewichte und Beschreibungen tragen.
class ChefZ_HoneycombFrame_Base extends ItemBase {}

//! Auftrag: "Honigwabe_Leer" / "Honeycomb_Frame_Empty".
class ChefZ_HoneycombFrameEmpty extends ChefZ_HoneycombFrame_Base {}

//! Verdeckelt und im Stock. Kein Auftragsname - die Begruendung fuer diesen
//! vierten Zustand steht an der config.cpp.
class ChefZ_HoneycombFrameSealed extends ChefZ_HoneycombFrame_Base {}

//! Auftrag: "Honigwabe_Voll" / "Honeycomb_Frame_Full".
class ChefZ_HoneycombFrameFull extends ChefZ_HoneycombFrame_Base {}

//! Auftrag: "Frame_Ready_To_Spin".
class ChefZ_HoneycombFrameUncapped extends ChefZ_HoneycombFrame_Base {}

//! Die Entdeckelungsgabel (Auftrag: "Uncapping_Fork"). Reines Werkzeug -
//! sie wird nie verbraucht, nur ueber die Werkzeuggruppe UNCAPPING_TOOL
//! gefunden und ueber toolDamage an PROCESS_UNCAP_COMB abgenutzt. Dieselbe
//! Bauart wie ChefZ_PastaMachine in ChefZ_Processing.
class ChefZ_UncappingFork extends ItemBase {}

//! Die Imkerpfeife (Auftrag: "Smoker"). Ebenfalls reines Werkzeug, gefunden
//! ueber die Werkzeuggruppe BEE_SMOKER - seit dem Wegfall des Pflichtwerkzeugs
//! aber NICHT mehr ueber die toolGroups eines Prozesses, sondern nur noch von
//! ChefZ_Beehive.ChefZ_IsBeeSmoker(). Die Gruppe bleibt die Adresse; wer eine
//! eigene Pfeife baut, traegt sie dort ein und bekommt den Schutz.
//!
//! NICHT zu verwechseln mit ChefZ_Smoker aus ChefZ_Processing - das ist der
//! Raeucherschrank der Konservierungskette.
class ChefZ_BeeSmoker extends ItemBase {}
