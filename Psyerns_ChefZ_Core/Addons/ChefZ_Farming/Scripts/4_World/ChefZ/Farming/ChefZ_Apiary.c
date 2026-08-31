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
// WAS DER STOCK SELBST TUT, und warum das nicht die Stationsmaschine tut:
//
// Das Volk fuellt die Raehmchen EINES NACH DEM ANDEREN, vier Stunden je
// Raehmchen, und der Spieler soll jedem Raehmchen dabei zusehen koennen - ein
// steigender Balken wie beim Apfel, nur andersherum. Ein Stationsjob kann
// das nicht: er kennt einen Fortschritt je Job, nicht je Item, und er
// verbraucht am Ende einen Eingang, statt ihn stetig zu veraendern. Der
// Fuellstand liegt deshalb als varQuantity AM RAEHMCHEN (0..100), und der
// Stock schreibt ihn mit einem eigenen Timer fort. Die Engine speichert und
// synchronisiert diese Variable selbst (ItemBase.c:254 registriert
// m_VarQuantity zum Sync, ItemBase.c:3045-3056 und 3258-3261 laden sie aus
// dem Spielstand) - ein eigener Persistenzblock ist nicht noetig.
//
// Waehrend der Server steht, vergeht keine Fuellzeit. Das ist Vanillas
// Verhalten an PlantBase (der Zeitzaehler wird gespeichert, nicht die Uhr)
// und hier Absicht: ein Stock, der ueber die Wartungspause zehn Raehmchen
// nachholt, waere ein Geschenk an den, der zufaellig danach kommt.
//
// Der Deckel: entnehmen laesst sich NUR ein volles Raehmchen und NUR aus
// einem geoeffneten Stock. Oeffnen ist die Stationsaktion
// PROCESS_HARVEST_HIVE - sie hat absichtlich keinen Transform, ihr Haken
// oeffnet den Deckel fuer zwei Minuten und loest den Stich aus. Die
// Entnahme selbst ist der gewoehnliche Inventar-Drag, den CanReleaseCargo
// bewacht.
//
// Was hier NICHT entstanden ist: keine eigene Action, keine modded class,
// keine Zeile im Core. Alles ist gewoehnliche Vererbung auf einer Klasse,
// die ohnehin von ChefZ_ProcessingStation_Base erbt.
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

//! Der Bienenstock (Auftrag: "Bienenstock"). Was er als Station anbietet,
//! steht im Stationsdatensatz (Config/Processing/Apiary_Stations.json); wie
//! Raehmchen voll werden, steht HIER - siehe Dateikopf.
class ChefZ_Beehive extends ChefZ_ProcessingStation_Base
{
    //==========================================================================
    // Das Fuellen - Zahlen und Namen
    //==========================================================================

    //! Vier Stunden je Raehmchen (Auftrag 3). Zehn Raehmchen sind damit
    //! vierzig Stunden - die laengste Wartezeit des Projekts, und sie soll es
    //! sein: Honig ist der einzige Suessstoff der Kette.
    static const float CHEFZ_FILL_SECONDS_PER_FRAME = 14400.0;

    //! Der Takt des Fuelltimers. Zehn Sekunden und nicht zwei wie der
    //! Job-Timer der Basis: bei vier Stunden je Raehmchen ist ein Schritt von
    //! 0.069 Prozent ohnehin unter dem, was ein Balken zeigt, und jeder Tick
    //! laeuft ueber den ganzen Cargo.
    static const float CHEFZ_FILL_TICK_SEC = 10.0;

    //! Wie lange der Deckel nach dem Oeffnen abbleibt. Zwei Minuten reichen,
    //! um alle vollen Raehmchen herauszunehmen, und sind kurz genug, dass ein
    //! offener Stock kein Dauerzustand wird. Nicht persistiert - nach einem
    //! Neustart ist jeder Stock zu.
    static const float CHEFZ_LID_OPEN_SEC = 120.0;

    //! Wozu ein volles Leerraehmchen wird. Als Konstante und nicht inline,
    //! damit der Klassenname genau einmal im Skript steht.
    static const string CHEFZ_FRAME_FULL_CLASS = "ChefZ_HoneycombFrameFull";

    //! EIGENER Timer, nicht m_ChefZ_JobTimer der Basis: der Job-Timer haelt
    //! sich selbst an, sobald kein Stationsjob mehr aktiv ist
    //! (ChefZ_TickJobs und ChefZ_CompleteJob), und hier laeuft nie einer.
    //! Vorbild ist PlantBase (scripts - 1.29, PlantBase.c:36, 84-94).
    protected ref Timer m_ChefZ_FillTimer;

    //! Restzeit des offenen Deckels in Sekunden. Nur auf dem Server.
    protected float m_ChefZ_LidOpenSec;

    //! Ob der Deckel offen ist. SYNCHRONISIERT, weil CanReleaseCargo auch
    //! auf dem Client gefragt wird - ohne Sync saehe der Spieler ein
    //! Raehmchen, das sich ziehen laesst, und der Server verweigerte es.
    protected bool m_ChefZ_LidOpen;

    //! Wahr nur waehrend der eigenen Ersetzung eines Raehmchens. Die Engine
    //! fragt vor dem Entfernen aus dem Cargo den Haken CanReleaseCargo
    //! (EntityAI.c:1602-1606, "scriptConditionExecute"; die Lambda prueft
    //! davor LocationCanRemoveEntity, ReplaceItemWithNewLambdaBase.c:31-37).
    //! Ohne diese Wache antwortete der Haken bei zugeklapptem Deckel fuer ein
    //! Leerraehmchen immer "nein", und kein Raehmchen wuerde je voll. Nur auf
    //! dem Server, nie persistiert.
    protected bool m_ChefZ_ReplacingFrame;

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
    //! Oeffnen bleibt damit unveraendert - nur der Ort, an dem er entsteht,
    //! hat sich verschoben (siehe unten, "warum toolDamage jetzt 0 ist").
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
    //! Der GRUNDSCHADEN ohne Pfeife (29.08.2026): "wer sie nicht benutzt,
    //! bekommt grundsaetzlich schon mehr Schockschaden und einen Bleed-Schaden
    //! zusaetzlich". Das ist der Schwarm um den Kopf, den keine Muetze
    //! abhaelt: 20 Schock IMMER, dazu ein Stich an den Unterarm, den nur der
    //! Gummianzug verhindert. Die Regionen oben kommen OBENDRAUF - ohne Hut
    //! also 35 Schock, ohne Handschuhe zwei blutende Arme.
    static const float CHEFZ_STING_SHOCK_BASE = 20.0;
    //! Kennung des Angriffsgeraeuschs im ItemSoundHandler. Vanillas eigene
    //! IDs stehen in SoundConstants (3_Game/DayZ/constants.c:415-434) und
    //! enden bei 29; 10000 liegt weit ausserhalb - dieselbe Vorsorge wie bei
    //! RPC-Nummern. Das SoundSet steht in der config.cpp (CfgSoundSets).
    static const int    CHEFZ_SOUND_BEES_ATTACK    = 10000;
    static const string CHEFZ_SOUNDSET_BEES_ATTACK = "ChefZ_Bees_Attack_SoundSet";

    //! Die Werkzeuggruppe, die den Schutz traegt. Ueber die GRUPPE und nicht
    //! ueber IsKindOf("ChefZ_BeeSmoker"): eine Gruppe ist ein offener
    //! Namensraum (11 E8), ein fremdes Modul darf eine eigene Pfeife
    //! beisteuern und bekommt den Schutz damit geschenkt. Ein hart
    //! verdrahteter Klassenname koennte das nie.
    static const string CHEFZ_STING_SMOKER_GROUP = "BEE_SMOKER";

    //! Der EINE Prozess, an dem der Deckel abgeht und gestochen wird. Der
    //! Haken feuert fuer JEDEN Prozess dieser Station; die Pruefung bleibt,
    //! damit ein spaeter ergaenzter Vorgang nicht ungewollt den Deckel
    //! oeffnet.
    static const string CHEFZ_STING_PROCESS = "PROCESS_HARVEST_HIVE";

    //! Der zweite Stationsvorgang: Stock abbauen, Bausatz zurueck (29.08.2026,
    //! aus Lykos' Pack_BeeHive uebernommen). Er veraendert nicht den Inhalt
    //! der Station, sondern die Station selbst - siehe ChefZ_PackUp().
    static const string CHEFZ_PACK_PROCESS = "PROCESS_PACK_HIVE";
    static const string CHEFZ_KIT_CLASS    = "ChefZ_BeehiveKit";

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
    //! Der Schutzanzug (Auftrag vom 29.08.2026): Slot_Body Zeile 90 und
    //! Slot_Legs Zeile 108 derselben CfgSlots. Jacke und Hose sind die
    //! Skriptklassen NBCJacketBase / NBCPantsBase (scripts - 1.29/4_World/
    //! DayZ/Entities/ItemBase/Clothing/NBCJacketBase.c, NBCPantsBase.c) -
    //! geprueft per Cast, nicht per Klassenname.
    static const string CHEFZ_SLOT_BODY     = "Body";
    static const string CHEFZ_SLOT_LEGS     = "Legs";

    //==========================================================================
    // Lebenszyklus
    //==========================================================================

    void ChefZ_Beehive()
    {
        m_ChefZ_LidOpen        = false;
        m_ChefZ_LidOpenSec     = 0.0;
        m_ChefZ_ReplacingFrame = false;

        // Aufrufstelle in Vanilla: EntityAI.c:219-220.
        RegisterNetSyncVariableBool("m_ChefZ_LidOpen");
    }

    void ~ChefZ_Beehive()
    {
        // Wie die Basis an ihrem Job-Timer: hier, und nur hier, wird das
        // Objekt auch freigegeben.
        if (m_ChefZ_FillTimer)
        {
            m_ChefZ_FillTimer.Stop();
            m_ChefZ_FillTimer = null;
        }
    }

    //! Wie viele Raehmchen der Stock fasst. Virtuell, damit die Doppelbeute
    //! nur diese eine Zahl aendert.
    int ChefZ_FrameCapacity()
    {
        return 10;
    }

    //! Wie viele Bausaetze beim Abbau herauskommen. Virtuell aus demselben
    //! Grund: die Doppelbeute entsteht aus zwei Bausaetzen (TR_ExtendBeehive)
    //! und gibt beide zurueck.
    int ChefZ_KitCount()
    {
        return 1;
    }

    //! Beim Spawn (EEInit) und beim Laden (AfterStoreLoad) - beide Wege
    //! fuehren zu einem Stock, in dem schon Raehmchen liegen koennen.
    override void EEInit()
    {
        super.EEInit();
        ChefZ_StartFillTimer();
    }

    //! Bindet die Kennung an das SoundSet - laeuft auf beiden Seiten, gehoert
    //! wird auf dem Client. Vorbild: Barrel_ColorBase.InitItemSounds
    //! (scripts - 1.29/4_World/DayZ/Entities/ItemBase/Barrel_ColorBase.c:532).
    override void InitItemSounds()
    {
        super.InitItemSounds();
        ItemSoundHandler handler = GetItemSoundHandler();
        if (!handler)
            return;
        handler.AddSound(CHEFZ_SOUND_BEES_ATTACK, CHEFZ_SOUNDSET_BEES_ATTACK);
    }

    override void AfterStoreLoad()
    {
        super.AfterStoreLoad();
        ChefZ_StartFillTimer();
    }

    //! Ein Raehmchen kommt hinein - der Timer, der sich bei leerem Stock
    //! angehalten hat, laeuft wieder an. Aufrufstelle: EntityAI.c:1208.
    override void EECargoIn(EntityAI item)
    {
        super.EECargoIn(item);
        ChefZ_StartFillTimer();
    }

    //==========================================================================
    // Die Regeln am Cargo
    //==========================================================================

    /**
     * Nur Raehmchen hinein, und hoechstens so viele, wie der Stock fasst.
     *
     * Vanilla-Signatur EntityAI.c:1550. Das Cargo-Gitter (10x9, Doppelbeute
     * 10x15) hat Luft fuer mehr als zehn bzw. zwanzig Raehmchen zu 2x3, damit
     * ein gedrehtes oder versetztes Raehmchen das letzte nicht aussperrt -
     * die Obergrenze zaehlt deshalb das Skript, weil ein Cargo-Gitter weder
     * Klassen kennt noch eine Stueckzahl.
     *
     * VOLLE Raehmchen sind ausdruecklich erlaubt, obwohl der Spieler sie
     * normalerweise herausnimmt: die Ersetzung eines vollen Leerraehmchens
     * erzeugt das volle Raehmchen in derselben Cargo-Zelle, und ob die Engine
     * dabei diese Regel fragt, ist nicht belegt. Ein "nein" hier koennte die
     * Ersetzung scheitern lassen - das Raehmchen bliebe bei 100 Prozent
     * stehen. Zurueckgelegte volle Raehmchen sind ausserdem nichts Schlimmes.
     *
     * CanLoadItemIntoCargo (der Ladepfad, EntityAI.c:1568) wird NICHT
     * ueberschrieben: der Spielstand laedt alles, was er gespeichert hat.
     */
    override bool CanReceiveItemIntoCargo(EntityAI item)
    {
        if (!super.CanReceiveItemIntoCargo(item))
            return false;

        if (!item)
            return false;

        bool isEmpty = ChefZ_HoneycombFrameEmpty.Cast(item) != null;
        bool isFull  = ChefZ_HoneycombFrameFull.Cast(item) != null;
        if (!isEmpty && !isFull)
            return false;

        return ChefZ_CountFrames() < ChefZ_FrameCapacity();
    }

    /**
     * Heraus darf NUR ein volles Raehmchen, und NUR bei offenem Deckel
     * (Auftrag 6). Vanilla-Signatur EntityAI.c:1608, Vorbild fuer die
     * Ueberschreibung Barrel_ColorBase.c:520.
     *
     * Ein halbvolles Leerraehmchen bleibt im Stock - wer es herausnaehme,
     * naehme dem Volk die Arbeit weg, und der Balken hiesse dann nichts.
     *
     * Die eigene Ersetzung eines vollen Leerraehmchens geht an dieser Regel
     * vorbei: sie entfernt genau das Item, das die Regel sonst festhielte,
     * und legt an seiner Stelle das volle Raehmchen an (siehe
     * m_ChefZ_ReplacingFrame).
     */
    override bool CanReleaseCargo(EntityAI cargo)
    {
        if (!super.CanReleaseCargo(cargo))
            return false;

        if (m_ChefZ_ReplacingFrame)
            return true;

        if (!m_ChefZ_LidOpen)
            return false;

        return ChefZ_HoneycombFrameFull.Cast(cargo) != null;
    }

    //==========================================================================
    // Der Fuelltimer
    //==========================================================================

    //! Dieselbe Form wie StartJobTimer der Basis: Serverwache, laufenden
    //! Timer nicht doppelt starten, Objekt einmal anlegen und behalten.
    //! Timer.Run: tools.c:595, CALL_CATEGORY_SYSTEM wie PlantBase.c:86.
    protected void ChefZ_StartFillTimer()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        if (m_ChefZ_FillTimer && m_ChefZ_FillTimer.IsRunning())
            return;

        if (!m_ChefZ_FillTimer)
            m_ChefZ_FillTimer = new Timer(CALL_CATEGORY_SYSTEM);

        m_ChefZ_FillTimer.Run(CHEFZ_FILL_TICK_SEC, this, "ChefZ_OnFillTick", null, true);
    }

    //! Anhalten, Objekt behalten - der Aufruf kommt aus dem Tick selbst, und
    //! das Objekt unter dem laufenden Rueckruf wegzuziehen waere unklug
    //! (dieselbe Begruendung wie StopJobTimer der Basis).
    protected void ChefZ_StopFillTimer()
    {
        if (!m_ChefZ_FillTimer)
            return;
        m_ChefZ_FillTimer.Stop();
    }

    /**
     * Ein Tick des Volkes. OEFFENTLICH, weil Timer.Run die Methode ueber
     * ihren Namen ruft.
     *
     * SEQUENTIELL (Auftrag 2): nur das ERSTE Leerraehmchen in
     * Cargo-Reihenfolge steigt, alle anderen warten. Ist es voll, wird es
     * ersetzt - und erst wenn die Ersetzung gelungen ist, ist beim naechsten
     * Tick das naechste dran. Deshalb wird das erste Leerraehmchen UNGEACHTET
     * seiner Fuellung genommen: ein volles, das die Ersetzung noch nicht
     * losgeworden ist, wird bei jedem Tick erneut ersetzt, statt uebersprungen
     * zu werden. Uebersprungen bliebe es fuer immer bei 100 Prozent liegen,
     * unentnehmbar, und das naechste Raehmchen liefe vorzeitig an.
     *
     * Der Schritt wird aus dem Maximum des Raehmchens gerechnet, nicht aus
     * einer festen 100 - sollte die config.cpp das Maximum je aendern,
     * bleiben es vier Stunden. AddQuantity (ItemBase.c:3413) und nicht
     * SetQuantityNormalized: letzteres rundet auf ganze Zahlen, und ein
     * Schritt von 0.069 wuerde immer zu 0.
     *
     * Der letzte Schritt setzt SetQuantityMax (ItemBase.c:3418), damit der
     * Balken vor der Ersetzung einmal ganz voll steht - schlaegt die
     * Ersetzung fehl, sieht der Spieler ein volles Leerraehmchen, und der
     * naechste Tick versucht es erneut. Kein Verlust in beiden Faellen.
     */
    void ChefZ_OnFillTick()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        ChefZ_TickLid();

        ItemBase frame = ChefZ_FirstEmptyFrame();
        if (!frame)
        {
            // Nichts zu fuellen. Solange der Deckel offen ist, muss der
            // Timer weiterlaufen, damit er wieder zugeht.
            if (!m_ChefZ_LidOpen)
                ChefZ_StopFillTimer();
            return;
        }

        // Ein volles Leerraehmchen ist eine fehlgeschlagene Ersetzung aus
        // einem frueheren Tick - erneut versuchen, nicht weiterzaehlen.
        if (frame.IsFullQuantity())
        {
            ChefZ_ReplaceFrame(frame, CHEFZ_FRAME_FULL_CLASS);
            return;
        }

        float max = frame.GetQuantityMax();
        float step = max * CHEFZ_FILL_TICK_SEC / CHEFZ_FILL_SECONDS_PER_FRAME;
        float remaining = max - frame.GetQuantity();

        if (remaining > step)
        {
            frame.AddQuantity(step);
            return;
        }

        frame.SetQuantityMax();
        ChefZ_ReplaceFrame(frame, CHEFZ_FRAME_FULL_CLASS);
    }

    /**
     * Das erste Leerraehmchen im Cargo, voll oder nicht - oder null.
     *
     * Absichtlich OHNE Fuellstandspruefung: der Tick entscheidet selbst, ob
     * er zaehlt oder ersetzt (siehe ChefZ_OnFillTick). Eine Suche nach dem
     * ersten "nicht vollen" Raehmchen liesse ein volles, dessen Ersetzung
     * gescheitert ist, still liegen.
     *
     * Je Tick neu gesucht, NIE als Member gecacht: die Ersetzung loescht das
     * alte Item (ReplaceItemWithNewLambdaBase.c:200), ein gemerkter Zeiger
     * zeigte danach ins Leere. GetCargo Inventory.c:138, GetItemCount und
     * GetItem Cargo.c:28 und 32.
     */
    protected ItemBase ChefZ_FirstEmptyFrame()
    {
        GameInventory inv = GetInventory();
        if (!inv)
            return null;

        CargoBase cargo = inv.GetCargo();
        if (!cargo)
            return null;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            ChefZ_HoneycombFrameEmpty frame = ChefZ_HoneycombFrameEmpty.Cast(cargo.GetItem(i));
            if (frame)
                return frame;
        }

        return null;
    }

    //! Zaehlt alle Raehmchen im Cargo, gleich welchen Zustands.
    protected int ChefZ_CountFrames()
    {
        GameInventory inv = GetInventory();
        if (!inv)
            return 0;

        CargoBase cargo = inv.GetCargo();
        if (!cargo)
            return 0;

        int count = 0;
        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            if (ChefZ_HoneycombFrame_Base.Cast(cargo.GetItem(i)))
                count++;
        }

        return count;
    }

    /**
     * Ersetzt ein Raehmchen in Ort und Zelle durch eine andere Klasse.
     *
     * Der spielerlose Serverpfad, den Vanilla an Raedern geht
     * (InventoryItem.c:265-276): TurnItemIntoItemLambda mit player = null
     * (MiscGameplayFunctions.c:1-14), ausgefuehrt ueber
     * GameInventory.ReplaceItemWithNew (Inventory.c:1363). Im Cargo-Zweig
     * legt die Engine das neue Item per LocationCreateLocalEntity in
     * dieselbe Zelle (ReplaceItemWithNewLambdaBase.c:157-158).
     *
     * SetTransferParams(false, false, true, true): keine Agenten, KEINE
     * Variablen - sonst wanderte varQuantity mit und das volle Raehmchen
     * truege eine Menge, die es nicht hat -, Health ja, Menge ausgeschlossen.
     *
     * Die Wache m_ChefZ_ReplacingFrame steht nur um den Aufruf herum: die
     * Lambda laeuft synchron (Inventory.c:1363 fuehrt sie direkt aus), und
     * CanReleaseCargo darf nur in diesem Fenster "ja" sagen. Ein
     * Fehlschlag meldet sich im Log als "lambda cannot be executed" oder
     * "Step D) ABORT"; das Raehmchen bleibt dann bei 100 Prozent, und der
     * naechste Tick versucht es erneut.
     */
    protected void ChefZ_ReplaceFrame(notnull ItemBase frame, string newType)
    {
        GameInventory inv = frame.GetInventory();
        if (!inv)
            return;

        TurnItemIntoItemLambda lambda = new TurnItemIntoItemLambda(frame, newType, null);
        lambda.SetTransferParams(false, false, true, true);

        m_ChefZ_ReplacingFrame = true;
        inv.ReplaceItemWithNew(InventoryMode.SERVER, lambda);
        m_ChefZ_ReplacingFrame = false;
    }

    //==========================================================================
    // Abbauen (PROCESS_PACK_HIVE)
    //==========================================================================

    //! Ist process der Abbau-Vorgang? Lookup() und nicht Intern() - ein nie
    //! deklarierter Prozessname bleibt INVALID und trifft dann nichts.
    protected bool ChefZ_IsPackProcess(ChefZ_Sym process)
    {
        if (!ChefZ_SymbolTable.IsValid(process))
            return false;
        ChefZ_Sym pack = ChefZ_SymbolTable.Lookup(CHEFZ_PACK_PROCESS);
        if (!ChefZ_SymbolTable.IsValid(pack))
            return false;
        return process == pack;
    }

    /**
     * Darf der Stock jetzt abgebaut werden? Nur leer und geschlossen.
     *
     * Leer heisst: KEIN Item im Cargo - nicht nur kein Raehmchen. Das Cargo
     * nimmt zwar ueber CanReceiveItemIntoCargo nichts anderes an, aber der
     * Ladepfad (CanLoadItemIntoCargo) ist absichtlich nicht ueberschrieben,
     * und was ein Spielstand hineingelegt hat, soll beim Abbau nicht still
     * verschwinden. Ein offener Deckel heisst: gerade geerntet, das Volk ist
     * aufgebracht - zwei Minuten warten. Ein laufender Job kann es an
     * dieser Station nicht geben (parallelSlots 1, kein STATION_TIMED); die
     * Pruefung steht trotzdem da, damit ein spaeter ergaenzter Vorgang den
     * Abbau nicht unter sich weggezogen bekommt.
     *
     * Client UND Server rechnen dieselbe Antwort: der Deckel ist netsync,
     * und den Cargo-Inhalt einer Station in Reichweite kennt der Client.
     */
    protected bool ChefZ_CanPack()
    {
        if (m_ChefZ_LidOpen)
            return false;
        if (ChefZ_ActiveJobCount() > 0)
            return false;
        GameInventory inv = GetInventory();
        if (!inv)
            return false;
        CargoBase cargo = inv.GetCargo();
        if (!cargo)
            return true;
        return cargo.GetItemCount() == 0;
    }

    /**
     * Der Abbau-Vorgang wird AUSGEBLENDET, solange er nicht erlaubt ist -
     * statt zu erscheinen und dann nichts zu tun.
     *
     * ChefZ_ActionProcessAtStation liest die Prozessliste der Station ueber
     * ChefZ_GetProcessCount/ChefZ_GetProcessAt (RefreshProcesses auf dem
     * Client, ResolveProcessFor auf dem Server) und ueberspringt INVALID.
     * Der Hash-Pfad von ResolveProcessFor fragt ChefZ_SupportsProcess; auch
     * der sagt nein. So verschwindet der Vorgang aus dem Aktionsmenue, sobald
     * ein Raehmchen im Stock liegt, und die Aktion "Bienenstock oeffnen"
     * bleibt allein uebrig - ohne Variantenrad.
     *
     * Die Zaehlung (ChefZ_GetProcessCount) bleibt unveraendert: der Core
     * adressiert Prozesse ueber ihren Index (ChefZ_GetActiveProcessOrdinal),
     * und ein wandernder Index waere ein Fehler, den niemand sieht.
     */
    override bool ChefZ_SupportsProcess(ChefZ_Sym process)
    {
        if (!super.ChefZ_SupportsProcess(process))
            return false;
        if (ChefZ_IsPackProcess(process) && !ChefZ_CanPack())
            return false;
        return true;
    }

    override ChefZ_Sym ChefZ_GetProcessAt(int index)
    {
        ChefZ_Sym process = super.ChefZ_GetProcessAt(index);
        if (ChefZ_IsPackProcess(process) && !ChefZ_CanPack())
            return ChefZ_SymbolTable.INVALID;
        return process;
    }

    /**
     * Der Stock wird zum Bausatz. OEFFENTLICH, weil CallLater die Methode
     * ueber ihren Namen ruft.
     *
     * Einen Frame NACH dem Haken (CallLater 0, wie die Schleuder ihren
     * naechsten Durchlauf anstoesst): der Haken laeuft mitten in
     * OnFinishProgressServer der Aktion, deren Ziel dieser Stock ist. Ihn
     * dort zu loeschen hiesse, der laufenden Aktion ihr Ziel unter den
     * Fuessen wegzuziehen - erst soll die Aktion enden, dann der Stock.
     *
     * Vorgehen: Zustand noch einmal pruefen (zwischen Aktionsstart und Ende
     * kann jemand ein Raehmchen hineingelegt haben), Timer aus, je Bausatz
     * ein Objekt auf dem Boden an der Stelle des Stocks (CreateObjectEx mit
     * ECE_PLACE_ON_SURFACE - derselbe Aufruf, mit dem Vanilla ein gefangenes
     * Tier ablegt, CatchingResultBasic.c:107), Gesundheit anteilig
     * uebernehmen (GetHealth01 Object.c:997, GetMaxHealth Object.c:1004,
     * SetHealth Object.c:1011), dann DeleteSafe (EntityAI.c:786).
     *
     * Entsteht kein Bausatz - ein Klassenname, den keine Config kennt -,
     * bleibt der Stock stehen. Lieber ein Stock zu viel als einer zu wenig.
     */
    void ChefZ_PackUp()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        if (!ChefZ_CanPack())
        {
            ChefZ_LogPack("nicht abgebaut: der Stock ist nicht mehr leer oder noch offen.");
            return;
        }

        float health01 = GetHealth01("", "");
        vector pos = GetPosition();
        int wanted = ChefZ_KitCount();
        int made = 0;

        for (int i = 0; i < wanted; i++)
        {
            EntityAI kit = EntityAI.Cast(g_Game.CreateObjectEx(CHEFZ_KIT_CLASS, pos, ECE_PLACE_ON_SURFACE));
            if (!kit)
                continue;
            float maxHealth = kit.GetMaxHealth("", "");
            kit.SetHealth("", "", maxHealth * health01);
            made = made + 1;
        }

        if (made == 0)
        {
            ChefZ_LogPack("nicht abgebaut: " + CHEFZ_KIT_CLASS + " liess sich nicht anlegen.");
            return;
        }

        ChefZ_LogPack("abgebaut, " + made.ToString() + " Bausatz/Bausaetze abgelegt.");
        ChefZ_StopFillTimer();
        DeleteSafe();
    }

    protected void ChefZ_LogPack(string msg)
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
            return;
        ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS, "Bienenstock " + GetType() + ": " + msg);
    }

    //==========================================================================
    // Der Deckel
    //==========================================================================

    //! Deckel ab. Der Timer laeuft an, damit er auch dann wieder zugeht,
    //! wenn kein Raehmchen zu fuellen ist. SetSynchDirty EntityAI.c:3069.
    protected void ChefZ_OpenLid()
    {
        m_ChefZ_LidOpen    = true;
        m_ChefZ_LidOpenSec = CHEFZ_LID_OPEN_SEC;
        SetSynchDirty();
        ChefZ_StartFillTimer();
    }

    //! Ein Tick am Deckel: die Restzeit sinkt, bei null geht er zu.
    protected void ChefZ_TickLid()
    {
        if (!m_ChefZ_LidOpen)
            return;

        m_ChefZ_LidOpenSec = m_ChefZ_LidOpenSec - CHEFZ_FILL_TICK_SEC;
        if (m_ChefZ_LidOpenSec > 0.0)
            return;

        m_ChefZ_LidOpen    = false;
        m_ChefZ_LidOpenSec = 0.0;
        SetSynchDirty();
    }

    //==========================================================================
    // Oeffnen und der Bienenstich
    //==========================================================================

    /**
     * Jemand nimmt den Deckel ab: der Stock geht fuer zwei Minuten auf, und
     * das Volk verteidigt sich.
     *
     * DIE REGEL IN EINEM SATZ: Wer PROCESS_HARVEST_HIVE mit der Imkerpfeife in
     * der Hand abschliesst, kommt ungestochen davon (der Rauch beruhigt, die
     * Pfeife nimmt den Verschleiss); wer sie nicht haelt, wird an jeder
     * ungeschuetzten Koerperregion gestochen - Handschuhe decken die Haende,
     * Kopfbedeckung oder Maske den Kopf, und was nackt ist, blutet
     * beziehungsweise schwillt an. Die Entnahme der vollen Raehmchen ist
     * danach der Inventar-Drag.
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
     *     Materialbrocken waere absurd. Hier tritt es EINMAL je Oeffnen ein,
     *     und alle acht Sekunden ein Zehntel Stich waere eine Regel, die
     *     niemand je bemerkt: die Imkerpfeife bliebe sinnlos, und der Auftrag
     *     ist genau an ihr aufgehaengt. Sicher statt gewuerfelt ist ausserdem
     *     lernbar - der Spieler sieht den Zusammenhang beim ersten Mal.
     *     Ausweichen kostet ihn eine Blechdose oder einen Hut.
     *
     * -------------------------------------------------------------------------
     * WARUM ES BEI JEDEM AUSGANG STICHT UND AUFGEHT
     * -------------------------------------------------------------------------
     * outcome wird NICHT geprueft. Dieser Prozess hat absichtlich keinen
     * Transform; ChefZ_ActionProcessAtStation meldet dann NO_MATCH und ruft
     * den Haken trotzdem (NotifyStation). Was die Bienen aufbringt, ist der
     * offene Stock, nicht die Ausbeute - und offen ist er, sobald die Aktion
     * durchgelaufen ist.
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
     * Blutungen, Schock, Itemschaden und der Deckel sind autoritative
     * Zustaende (00 §5). Der Aufrufer prueft g_Game.IsServer() bereits
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

        // "Bei Abschluss einer erfolgreichen Aktion mit dem Bienenstock"
        // (29.08.2026). Diese Methode laeuft nur aus OnFinishProgressServer -
        // eine abgebrochene Aktion kommt nie hierher. Erfolglos ist allein
        // RUN_FAILED (der Applicator ist mittendrin gescheitert); NO_MATCH ist
        // fuer "Bienenstock oeffnen" der GEWOLLTE Ausgang, denn der Prozess
        // traegt absichtlich keinen Transform.
        if (outcome == ChefZ_StationActionOutcome.RUN_FAILED)
            return;

        // Abbauen: der Bausatz kommt einen Frame spaeter (ChefZ_PackUp).
        // Das Stechen unten laeuft davor ganz normal durch - wer sein Volk
        // ohne Rauch in die Kiste packt, wird dabei gestochen.
        if (ChefZ_IsPackProcess(process))
            g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ChefZ_PackUp, 0, false);

        // Der Deckel geht nur beim Oeffnen auf; gestochen wird bei JEDER
        // Aktion am Stock. Lookup() und nicht Intern(): ein Prozessname, den
        // niemand deklariert hat, ist INVALID und trifft dann auf keinen
        // gueltigen process - genau die stille, richtige Antwort.
        ChefZ_Sym harvest = ChefZ_SymbolTable.Lookup(CHEFZ_STING_PROCESS);
        if (ChefZ_SymbolTable.IsValid(harvest) && process == harvest)
            ChefZ_OpenLid();

        // Schritt 1: die Pfeife in der Hand. Ruiniert zaehlt sie nicht - eine
        // durchgebrannte Pfeife raucht nicht mehr, dieselbe Pruefung, die
        // Vanilla an den Handschuhen anlegt.
        if (inHands && !inHands.IsDamageDestroyed() && ChefZ_IsBeeSmoker(inHands))
        {
            inHands.DecreaseHealth("", "", CHEFZ_STING_ABSORB_DAMAGE);
            ChefZ_LogSting("Rauch: die Imkerpfeife hat das Volk beruhigt.");
            return;
        }

        // -----------------------------------------------------------------
        // DER SCHUTZANZUG (29.08.2026): "Wenn der Spieler einen NBC-Anzug
        // traegt, ist er geschuetzt vor Bienenangriffen. Fuer vollstaendigen
        // Schutz muss er auch eine Gasmaske tragen."
        //
        // Jacke + Hose aus Gummi: durch die kommt kein Stachel, und sie
        // nehmen dabei keinen Schaden - anders als Handschuhe aus Stoff, die
        // den Stich abfangen und ihn spueren. Haende und Koerper sind damit
        // dicht. Das Gesicht nicht: dafuer braucht es die Gasmaske (unten).
        // Vollstaendiger Schutz = Anzug UND Gasmaske; dann nimmt nichts
        // Schaden und niemand blutet.
        // -----------------------------------------------------------------
        bool nbcSuit = ChefZ_WearsNbcSuit(actor);
        bool gasMask = ChefZ_WearsGasMask(actor);

        if (nbcSuit && gasMask)
        {
            ChefZ_LogSting("Ohne Pfeife, aber Anzug und Gasmaske: dicht.");
            return;
        }

        // DER GRUNDSCHADEN - ohne Pfeife immer, egal was sonst getragen wird
        // (siehe CHEFZ_STING_SHOCK_BASE). Ueber eine float-Zwischenvariable,
        // aus demselben Grund, den ChefZ_ActionProcessAtStation.
        // ApplyToolDamage() ausschreibt: eine implizite Umwandlung an einer
        // Aufrufgrenze ist in Enforce nirgends zugesichert.
        float baseShock = CHEFZ_STING_SHOCK_BASE;
        actor.AddHealth("", "Shock", -baseShock);
        if (!nbcSuit)
            ChefZ_StingForearm(actor);

        // Und man hoert es: der Stock spielt den Angriff fuer alle in
        // Hoerweite. StartItemSoundServer setzt eine Synchronvariable, der
        // Client spielt das in InitItemSounds gebundene SoundSet
        // (ItemSoundHandler.c:12-17, das Vorbild ist Barrel_ColorBase.c:532).
        StartItemSoundServer(CHEFZ_SOUND_BEES_ATTACK);

        // Schritt 2 und 3, je Region getrennt - OBENDRAUF.
        bool stungHands = true;
        if (nbcSuit)
            stungHands = false;
        else if (ChefZ_AbsorbSting(actor, CHEFZ_SLOT_GLOVES))
            stungHands = false;

        // Kopf: erst die Kopfbedeckung, dann die Maske - und ausgeschrieben
        // statt als &&-Ausdruck. Beide Aufrufe haben eine WIRKUNG, sie
        // beschaedigen das gefundene Stueck; ob Enforce den zweiten Operanden
        // bei kurzgeschlossener Auswertung noch anfasst, soll an dieser Stelle
        // niemand nachschlagen muessen. Genau eines der beiden Stuecke nimmt
        // Schaden, nie beide.
        //
        // Die Gasmaske zuerst: sie schliesst das Gesicht ab und nimmt keinen
        // Schaden (Gummi, wie der Anzug). Vanilla kennt sie selbst -
        // Clothing_Base.IsGasMask() (scripts - 1.29/4_World/DayZ/Entities/
        // Core/Inherited/InventoryItem.c:995, Klasse ab Zeile 837),
        // ueberschrieben in MaskBase.c:6 fuer GasMask, GP5GasMask und
        // AirborneMask. NICHT auf ItemBase - deshalb der Clothing-Cast in
        // ChefZ_WearsGasMask, wie Vanilla selbst in PlayerBase.c:1479/1499.
        bool stungHead = true;
        if (gasMask)
            stungHead = false;
        else if (ChefZ_AbsorbSting(actor, CHEFZ_SLOT_HEADGEAR))
            stungHead = false;
        else if (ChefZ_AbsorbSting(actor, CHEFZ_SLOT_MASK))
            stungHead = false;

        if (stungHands)
            ChefZ_StingForearm(actor);

        if (stungHead)
        {
            float shock = CHEFZ_STING_SHOCK;
            actor.AddHealth("", "Shock", -shock);
        }

        ChefZ_LogSting("Ohne Pfeife: grundschaden ja, anzug=" + ChefZ_YesNo(nbcSuit) + " haende=" + ChefZ_YesNo(stungHands) + " kopf=" + ChefZ_YesNo(stungHead) + ".");
    }

    /**
     * Traegt der Spieler Jacke UND Hose eines Schutzanzugs, beide nicht
     * ruiniert? Ein ruiniertes Stueck ist aufgerissen und schuetzt nicht -
     * dieselbe Pruefung wie an den Handschuhen.
     *
     * Beide Stuecke, nicht eines: eine Gummijacke ueber nackten Beinen ist
     * kein Anzug. Haube, Stiefel und NBC-Handschuhe sind NICHT Bedingung -
     * die Haende deckt der Anzug ohnehin (Auftrag), das Gesicht deckt nur
     * die Gasmaske.
     */
    protected bool ChefZ_WearsNbcSuit(notnull PlayerBase actor)
    {
        ItemBase body = ItemBase.Cast(actor.FindAttachmentBySlotName(CHEFZ_SLOT_BODY));
        if (!body || body.IsDamageDestroyed())
            return false;
        if (!NBCJacketBase.Cast(body))
            return false;

        ItemBase legs = ItemBase.Cast(actor.FindAttachmentBySlotName(CHEFZ_SLOT_LEGS));
        if (!legs || legs.IsDamageDestroyed())
            return false;
        if (!NBCPantsBase.Cast(legs))
            return false;

        return true;
    }

    //! Sitzt im Maskenslot eine unzerstoerte Gasmaske? Vanillas eigene
    //! Antwort (IsGasMask), keine Klassenliste - eine fremde Maske, die sich
    //! als Gasmaske ausgibt, schuetzt damit ebenfalls. IsGasMask lebt auf
    //! Clothing_Base, nicht auf ItemBase - daher der Clothing-Cast (Vanillas
    //! eigenes Muster, PlayerBase.c:1479/1499).
    protected bool ChefZ_WearsGasMask(notnull PlayerBase actor)
    {
        Clothing mask = Clothing.Cast(actor.FindAttachmentBySlotName(CHEFZ_SLOT_MASK));
        if (!mask || mask.IsDamageDestroyed())
            return false;
        return mask.IsGasMask();
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
     * zweite Seite waere jedes weitere Oeffnen am schon blutenden Arm
     * folgenlos.
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

        if (!tools.IsToolOfGroup(classSym, group))
            return false;

        // Seit dem 29.08.2026 zaehlt nur eine BRENNENDE Pfeife: Vanillas
        // IsIgnited (EntityAI.c:558), das ChefZ_BeeSmoker mit seinem
        // Brennzustand beantwortet - und das jede fremde Pfeife aus der
        // Gruppe BEE_SMOKER genauso beantworten muss.
        return item.IsIgnited();
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

//! Die Doppelbeute: zwei Zargen, zwanzig Raehmchen, sonst in allem der
//! Stock - Fuelltakt, Deckel, Stich und Regeln erbt sie unveraendert. Sie
//! entsteht aus zwei Bausaetzen (TR_ExtendBeehive), nicht aus einem
//! aufgestellten Stock: ein Handwerksschritt verbraucht seine Zutat samt
//! Cargo, und ein bestueckter Stock verloere dabei seine Raehmchen.
class ChefZ_BeehiveDouble extends ChefZ_Beehive
{
    override int ChefZ_FrameCapacity()
    {
        return 20;
    }

    override int ChefZ_KitCount()
    {
        return 2;
    }
}

//! Der Bausatz (Auftrag: "Beehive_Kit"). Reines Traggut ohne ChefZ-Zustand;
//! er wird von TR_RaiseBeehive und TR_ExtendBeehive verbraucht und ist bis
//! dahin nur schwer.
class ChefZ_BeehiveKit extends ItemBase {}

//! Gemeinsame Skriptbasis der drei Raehmchen. Sie traegt bewusst KEINEN
//! ChefZ-Zustand: der Unterschied zwischen leer, voll und entdeckelt ist die
//! KLASSE, nicht eine Zustandsvariable auf einer Klasse. Was innerhalb einer
//! Klasse veraenderlich ist - der Fuellgrad des leeren, der Glaservorrat des
//! entdeckelten Raehmchens -, ist Vanillas varQuantity und braucht kein
//! Skript.
class ChefZ_HoneycombFrame_Base extends ItemBase {}

//! Auftrag: "Honigwabe_Leer" / "Honeycomb_Frame_Empty". Traegt den
//! steigenden Balken (varQuantity 0..100), den ChefZ_Beehive fortschreibt.
class ChefZ_HoneycombFrameEmpty extends ChefZ_HoneycombFrame_Base {}

//! Auftrag: "Honigwabe_Voll" / "Honeycomb_Frame_Full". Entsteht im Stock
//! durch Ersetzung des vollen Leerraehmchens; das Einzige, was der Stock
//! bei offenem Deckel hergibt.
class ChefZ_HoneycombFrameFull extends ChefZ_HoneycombFrame_Base {}

//! Auftrag: "Frame_Ready_To_Spin". Traegt drei Glaeser Vorrat plus eine
//! Reserve-Einheit (varQuantity 4..1), die die Schleuder je Glas um eins
//! abzieht - warum die letzte Einheit stehen bleibt, steht an der Klasse in
//! der config.cpp.
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
//==============================================================================
// ChefZ_BeeSmoker - die Imkerpfeife, die man stopfen und anzuenden muss
// (29.08.2026).
//
// ZWEI ZUSTAENDE, beide am Item:
//   Fuellung  = varQuantity 0..100 (config), gefuellt durch TR_FillBeeSmoker.
//               Persistiert die Engine von selbst.
//   Brennt    = m_ChefZ_Lit, netzsynchron. NICHT persistiert - nach einem
//               Neustart ist jede Pfeife aus. Das ist gewollt: eine Glut, die
//               eine Serverpause ueberlebt, waere die einzige im Spiel.
//
// ANZUENDEN geht ueber Vanillas ActionLightItemOnFire (scripts - 1.29/
// 4_World/.../Continuous/ActionLightItemOnFire.c:77): Feuerzeug oder
// Streichholz in der Hand, Pfeife als Ziel. Die Aktion fragt am Ziel
// CanBeIgnitedBy, IsThisIgnitionSuccessful und ruft OnIgnitedThis - alle
// drei in EntityAI.c:546-618 als leere Vorgaben, hier gefuellt. Vorbild in
// jeder Zeile: Torch.c:141-208 (Fackel), nur ohne Energiemanager.
//
// ABBRENNEN: ein Server-Timer alle 5 s, volle Fuellung haelt zehn Minuten.
// Bei null geht sie aus. Der Rauch ist Vanillas kleines Lagerfeuer-Partikel
// (ParticleList.CAMP_SMALL_SMOKE), clientseitig aus OnVariablesSynchronized.
//
// Layer: 4_World.
//==============================================================================
class ChefZ_BeeSmoker extends ItemBase
{
    //! Sekunden, die eine VOLLE Fuellung raucht.
    static const float CHEFZ_BURN_SECONDS_FULL = 600.0;
    static const float CHEFZ_BURN_TICK_SEC     = 5.0;
    //! Unter dieser Fuellung faengt sie kein Feuer - ein Kruemel Rinde reicht
    //! nicht fuer eine Glut.
    static const float CHEFZ_MIN_FUEL_TO_LIGHT = 10.0;

    protected bool      m_ChefZ_Lit;
    protected ref Timer m_ChefZ_BurnTimer;
    protected Particle  m_ChefZ_Smoke;

    void ChefZ_BeeSmoker()
    {
        m_ChefZ_Lit = false;
        RegisterNetSyncVariableBool("m_ChefZ_Lit");
    }

    void ~ChefZ_BeeSmoker()
    {
        if (m_ChefZ_BurnTimer)
        {
            m_ChefZ_BurnTimer.Stop();
            m_ChefZ_BurnTimer = null;
        }
        ChefZ_StopSmoke();
    }

    //! Raucht sie? Das ist die Frage, die der Bienenstock stellt.
    bool ChefZ_IsSmoking()
    {
        return m_ChefZ_Lit;
    }

    // ---- Vanillas Anzuend-Schnittstelle (EntityAI.c:540-618) --------------

    override bool HasFlammableMaterial()
    {
        return GetQuantity() >= CHEFZ_MIN_FUEL_TO_LIGHT;
    }

    override bool IsIgnited()
    {
        return m_ChefZ_Lit;
    }

    //! Torch.c:157-180, ohne Energiemanager: nicht schon brennend, genug
    //! Rinde, nicht nass, und in der Hand - nicht aus dem Rucksack heraus.
    override bool CanBeIgnitedBy(EntityAI igniter = NULL)
    {
        if (m_ChefZ_Lit)
            return false;
        if (GetQuantity() < CHEFZ_MIN_FUEL_TO_LIGHT)
            return false;
        if (GetWet() >= GameConstants.STATE_DAMP)
            return false;

        PlayerBase player = PlayerBase.Cast(GetHierarchyRootPlayer());
        if (player && this != player.GetItemInHands())
            return false;

        return true;
    }

    override bool IsThisIgnitionSuccessful(EntityAI item_source = NULL)
    {
        return CanBeIgnitedBy(item_source);
    }

    override void OnIgnitedThis(EntityAI fire_source)
    {
        super.OnIgnitedThis(fire_source);
        ChefZ_SetLit(true);
    }

    // ---- Brennen ------------------------------------------------------------

    protected void ChefZ_SetLit(bool lit)
    {
        if (!g_Game || !g_Game.IsServer())
            return;
        if (m_ChefZ_Lit == lit)
            return;

        m_ChefZ_Lit = lit;
        SetSynchDirty();

        if (lit)
        {
            if (!m_ChefZ_BurnTimer)
                m_ChefZ_BurnTimer = new Timer(CALL_CATEGORY_SYSTEM);
            m_ChefZ_BurnTimer.Run(CHEFZ_BURN_TICK_SEC, this, "ChefZ_OnBurnTick", null, true);
            return;
        }

        if (m_ChefZ_BurnTimer)
            m_ChefZ_BurnTimer.Stop();
    }

    //! Timer-Rueckruf, deshalb oeffentlich.
    void ChefZ_OnBurnTick()
    {
        if (!g_Game || !g_Game.IsServer())
            return;
        if (!m_ChefZ_Lit)
            return;

        float step = GetQuantityMax() * CHEFZ_BURN_TICK_SEC / CHEFZ_BURN_SECONDS_FULL;
        if (GetQuantity() <= step)
        {
            SetQuantity(0.0);
            ChefZ_SetLit(false);
            return;
        }
        AddQuantity(-step);
    }

    // ---- Rauch (Client) -----------------------------------------------------

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        ChefZ_UpdateSmoke();
    }

    protected void ChefZ_UpdateSmoke()
    {
        if (!g_Game || g_Game.IsDedicatedServer())
            return;
        if (m_ChefZ_Lit)
        {
            if (!m_ChefZ_Smoke)
                m_ChefZ_Smoke = ParticleManager.GetInstance().PlayOnObject(ParticleList.CAMP_SMALL_SMOKE, this, Vector(0, 0.15, 0));
            return;
        }
        ChefZ_StopSmoke();
    }

    protected void ChefZ_StopSmoke()
    {
        if (!m_ChefZ_Smoke)
            return;
        m_ChefZ_Smoke.Stop();
        m_ChefZ_Smoke = null;
    }
}
