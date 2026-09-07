//==============================================================================
// ChefZ_Smoker - der Raeucherschrank (Slice "preservation").
//
// Andockregel aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/Processing/
// ChefZ_ProcessingStation_Base.c:
//
//     config.cpp   class ChefZ_Smoker : Inventory_Base { ... };
//     JSON/Rang 2  { "kind":"station", "records":[{ "id":"ChefZ_Smoker", ... }] }
//     Skript       class ChefZ_Smoker extends ChefZ_ProcessingStation_Base {}
//
// Der Schrank laeuft ueber PROCESS_SMOKE (STATION_TIMED): er tickt ohne
// Spieler weiter und verlangt Waerme. Was er raeuchert, steht in
// ChefZ_Preservation/Config/Processing/Smoking.json - nicht hier und nicht in
// der config.cpp.
//
//==============================================================================
// ### 31.08.2026 ### DER SCHRANK HAT SEIN EIGENES FEUER
//==============================================================================
//
// WAS VORHER FALSCH WAR
// ---------------------
// In dieser Datei stand bis heute genau eine Zeile - die Bindung, sonst
// nichts - und darueber der Satz "Mehr ist nicht noetig". Das stimmte nicht.
// Der Schrank war seit seiner Anlage KEIN EINZIGES MAL betriebsbereit, und
// zwar aus zwei voneinander unabhaengigen Gruenden:
//
//   1. Config/Processing/PreservationStations.json sagt needsFuel = true.
//      ChefZ_ProcessingStation_Base.ChefZ_IsPowered() antwortet dann "nein"
//      (Core Z.403-407) - ausdruecklich als sichere Vorgabe, damit ein
//      vergessener Ueberschreiber sichtbar wird statt still zu wirken. Er war
//      vergessen; ChefZ_CompiledProcess.MeetsEnvironment brach jeden Job ab,
//      bevor er lief.
//   2. PROCESS_SMOKE traegt requiresHeat = 1, und ChefZ_HasHeat() lieferte
//      ebenfalls die Basisantwort "nein" - anders als ChefZ_FryingPan hatte
//      der Schrank keinen Ueberschreiber.
//
// Zwei Bedingungen, beide dauerhaft unerfuellt, und keine davon meldet etwas,
// das nach einem Fehler aussieht. In Alex' Testbericht steht dazu schlicht:
// "keine Funktion".
//
// WAS JETZT PASSIERT
// ------------------
// Alex' Zielbild: "ein Platz, wo man das Feuer reinpacken kann wie beim
// Vanilla-Ofen", raeuchern nur bei brennendem Feuer, Rinde als Brennstoff,
// vollstaendig raeuchern in fuenf Minuten. Umgesetzt als EIGENER
// BRENNZUSTAND am Schrank:
//
//   Brennstoff  Rinde (Bark_Oak / Bark_Birch, gemeinsame Skriptbasis
//               Bark_ColorBase) im CARGO. Kein Attachment-Slot: der Cargo ist
//               ohnehin schon die Eingangsseite der Station, und ein eigener
//               Slot haette eine Modellaenderung verlangt, auf die dieser
//               Slice nicht warten darf.
//   Anzuenden   Vanillas ActionLightItemOnFire. KEINE eigene Action - der
//               Schrank beantwortet nur die vier Fragen, die die Aktion am
//               Ziel stellt.
//   Brennen     ein Server-Timer, der Rinde verbraucht.
//   Wirkung     ChefZ_IsPowered() UND ChefZ_HasHeat() liefern den
//               Brennzustand. Geht das Feuer aus, PAUSIERT ein laufender Job -
//               er bricht nicht ab und laeuft nie zurueck (11 §7). Ein Spieler,
//               dem die Rinde ausgeht, verliert Zeit, nie Material.
//
// Vorbild in jeder Zeile: ChefZ_BeeSmoker (ChefZ_Farming/Scripts/4_World/
// ChefZ/Farming/ChefZ_Apiary.c:1136-1282), das im Projekt bereits erprobte
// Muster fuer "Item mit Fuellung, das man anzuenden kann".
//
// DIE ANZUEND-SCHNITTSTELLE, NACHGESEHEN STATT ANGENOMMEN
// -------------------------------------------------------
// ActionLightItemOnFire.ActionCondition (scripts - 1.29/4_World/DayZ/Classes/
// UserActionsComponent/Actions/Continuous/ActionLightItemOnFire.c:70-101)
// verlangt vom ZIEL vier Dinge:
//
//   !targetItem.IsIgnited()               brennt noch nicht
//   !IsItemInCargoOfSomething(targetItem) liegt NICHT im Cargo eines anderen
//                                         Items (Z.40-54: geprueft wird der
//                                         Cargo-Index der eigenen
//                                         Inventarposition). Der Raeucherschrank
//                                         wird abgestellt und steht am Boden -
//                                         die Bedingung ist erfuellt. Ein
//                                         Schrank im Rucksack laesst sich nicht
//                                         anzuenden, und das ist richtig so.
//   item.CanIgniteItem(targetItem)        Sache des Zuenders, nicht unsere
//                                         (Matchbox.c:9-15: Menge > 0 und nicht
//                                         feucht)
//   targetItem.CanBeIgnitedBy(item)       unsere Antwort, unten
//
// Die Aktion sitzt am ZUENDER, nicht am Ziel: Matchbox.c:37, PetrolLighter.c:37,
// HandDrillKit.c:29, Torch.c:765, Roadflare.c:482 rufen AddAction(
// ActionLightItemOnFire). Der Schrank braucht deshalb kein SetActions und
// bekommt auch keines.
//
// Die vier Haken selbst stehen als leere Vorgaben in EntityAI (scripts - 1.29/
// 3_Game/DayZ/Entities/EntityAI.c:540-621) und sind ausdruecklich zum
// Ueberschreiben da ("Override this method ...").
//
// WARUM 150 SEKUNDEN JE STUECK RINDE
// ----------------------------------
// Eine Vanilla-Feuerstelle verbrennt ein Stueck Rinde in rund ZWANZIG
// Sekunden: FireplaceBase fuehrt Bark_Oak mit Energie 10 und Bark_Birch mit 8
// (FireplaceBase.c:245-246), verbraucht PARAM_FIRE_CONSUM_RATE_AMOUNT = 0.5
// Energie je Tick (Z.53) und tickt alle TIMER_HEATING_UPDATE_INTERVAL = 3
// Sekunden (Z.74) - also 1.5 Energie je 3 Sekunden.
//
// Ein Raeucherschrank lodert aber nicht, er SCHWELT. 150 Sekunden je Stueck
// sind der Faktor siebeneinhalb auf die offene Flamme, und die Zahl ist so
// gewaehlt, dass sie mit dem Rest der Kette aufgeht:
//
//     PROCESS_SMOKE dauert 300 s (Alex: fuenf Minuten)
//     300 s / 150 s = GENAU ZWEI STUECK RINDE je vollstaendigem Raeuchergang
//
// Zwei Stueck Rinde sind ein ehrlicher Preis fuer die laengste Haltbarkeit der
// Matrix §56 - beschaffbar, aber nicht beilaeufig. Wer den Schrank voll
// belegt, raeuchert zwei Stuecke gleichzeitig (parallelSlots = 2) und zahlt
// die zwei Rinden trotzdem nur einmal; das ist der Vorteil des Schranks
// gegenueber dem Trockenrahmen.
//
// Kein Unterschied zwischen Eiche und Birke, obwohl Vanilla einen kennt (10
// gegen 8 Energie): der Schrank misst keine Energie, er zaehlt Stuecke. Ein
// Unterschied von zwanzig Prozent waere hier eine Zahl, die niemand bemerkt
// und die jeder pflegen muss.
//
// PERSISTENZ: KEINE, UND ZWAR ABSICHTLICH
// ---------------------------------------
// m_ChefZ_Lit wird nicht gespeichert - nach einem Serverneustart ist jeder
// Schrank aus. Dieselbe Entscheidung wie bei der Imkerpfeife (ChefZ_Apiary.c:
// 1119-1121): eine Glut, die eine Serverpause ueberlebt, waere die einzige im
// Spiel. Der laufende Raeucherjob ueberlebt sehr wohl - den schreibt der
// Job-Block der Basis - und pausiert danach, bis jemand wieder anzuendet.
// Kein eigener Speicherblock heisst ausserdem: kein zweiter Leser im
// Ladestrom dieser Klasse, der bei einer spaeteren Aenderung aus dem Tritt
// geraten kann.
//
// Die noch nicht verbrannte Rinde IST persistent - sie liegt als Item im
// Cargo, und den speichert die Engine. Verloren geht nur der Rest des
// Stuecks, das gerade schwelte.
//
// Diese Station fasst Vanillas Kochkette an keiner Stelle an (11 E6).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_Smoker extends ChefZ_ProcessingStation_Base
{
    //! Wie lange EIN Stueck Rinde den Schrank schwelen laesst. Herleitung im
    //! Dateikopf; 300 s Raeuchergang / 150 s = zwei Stueck je Durchgang.
    static const float CHEFZ_SECONDS_PER_BARK = 150.0;

    //! Takt des Brenntimers. 5 s und nicht 0.1: der Schrank brennt Minuten
    //! lang, und der Job-Timer der Basis tickt selbst nur alle 2 s.
    static const float CHEFZ_BURN_TICK_SEC = 5.0;

    //! Die beiden Kategorien, die ins Raeuchergut gehoeren. MEAT trifft ueber
    //! die Closure auch SAUSAGE und alle Wildarten - siehe ChefZ_StationGate.
    static const string CHEFZ_CAT_MEAT = "MEAT";
    static const string CHEFZ_CAT_FISH = "FISH";

    //! Hoehe des Rauchpartikels ueber dem Objektursprung. GESCHAETZT am
    //! Platzhaltermodell: der Rauch soll oben aus dem Schrank kommen, nicht
    //! aus dem Boden. Mit dem endgueltigen Mesh gehoert die Zahl nachgemessen -
    //! sie ist rein optisch und beeinflusst nichts.
    static const float CHEFZ_SMOKE_HEIGHT_M = 1.4;

    //! Brennt der Schrank? Netzsynchron, damit der Client den Rauch zeigen
    //! kann. NICHT persistiert - Begruendung im Dateikopf.
    protected bool m_ChefZ_Lit;

    //! Restsekunden des Stuecks Rinde, das gerade schwelt. Reiner
    //! Serverzustand.
    protected float m_ChefZ_FuelLeftSec;

    protected ref Timer m_ChefZ_BurnTimer;
    protected Particle  m_ChefZ_Smoke;

    void ChefZ_Smoker()
    {
        m_ChefZ_Lit         = false;
        m_ChefZ_FuelLeftSec = 0.0;
        RegisterNetSyncVariableBool("m_ChefZ_Lit");
    }

    void ~ChefZ_Smoker()
    {
        if (m_ChefZ_BurnTimer)
        {
            m_ChefZ_BurnTimer.Stop();
            m_ChefZ_BurnTimer = null;
        }
        ChefZ_StopSmoke();
    }

    //==========================================================================
    // Die zwei Haken, an denen die Station haengt
    //==========================================================================

    /**
     * Hat der Schrank Waerme?
     *
     * KEIN super: die Basis antwortet hier fest "nein" und ist ausdruecklich
     * zum Ueberschreiben da (Core Z.373-388). Ein super-Aufruf haette nichts
     * zu addieren - die Antwort wird ersetzt, nicht ergaenzt. Dasselbe macht
     * ChefZ_FryingPan seit dem Salt-Slice.
     */
    override bool ChefZ_HasHeat()
    {
        return m_ChefZ_Lit;
    }

    /**
     * Hat der Schrank Brennstoff?
     *
     * Der Stationsdatensatz sagt needsFuel = true, die Basis antwortet dann
     * "nein". Hier steht die eigentliche Auskunft: brennt er, ist er versorgt.
     * Rinde, die nur im Cargo LIEGT, reicht nicht - jemand muss sie anzuenden.
     *
     * KEIN super, aus demselben Grund wie bei ChefZ_HasHeat.
     */
    override bool ChefZ_IsPowered()
    {
        return m_ChefZ_Lit;
    }

    //==========================================================================
    // Vanillas Anzuend-Schnittstelle (EntityAI.c:540-621)
    //==========================================================================

    //! Liegt ueberhaupt Brennbares darin? Wird auf Server UND Client
    //! ausgewertet und steuert nebenbei die Anzuendanimation
    //! (ActionLightItemOnFire.SetIgnitingAnimation, Z.155-165).
    override bool HasFlammableMaterial()
    {
        return ChefZ_CountBark() > 0;
    }

    override bool IsIgnited()
    {
        return m_ChefZ_Lit;
    }

    /**
     * Laesst er sich JETZT anzuenden?
     *
     * Drei Bedingungen, und keine davon prueft die Inventarposition: das
     * uebernimmt die Aktion selbst mit IsItemInCargoOfSomething
     * (ActionLightItemOnFire.c:40-54, 77). Ein Schrank im Rucksack ist damit
     * schon draussen, bevor diese Zeile laeuft.
     *
     * Die Nasspruefung ist woertlich die der Imkerpfeife und Vanillas Fackel
     * (Torch.c:157-180): nasse Rinde faengt kein Feuer.
     */
    override bool CanBeIgnitedBy(EntityAI igniter = NULL)
    {
        if (m_ChefZ_Lit)
            return false;
        if (ChefZ_CountBark() <= 0)
            return false;
        if (GetWet() >= GameConstants.STATE_DAMP)
            return false;
        return true;
    }

    //! Letzte Pruefung unmittelbar vor dem Zuenden, serverseitig
    //! (ActionLightItemOnFire.OnFinishProgressServer, Z.115).
    override bool IsThisIgnitionSuccessful(EntityAI item_source = NULL)
    {
        return CanBeIgnitedBy(item_source);
    }

    //! Es hat gezuendet. Das erste Stueck Rinde geht sofort in die Glut - wer
    //! anzuendet, verbrennt etwas, auch wenn er den Schrank gleich wieder
    //! leerraeumt.
    override void OnIgnitedThis(EntityAI fire_source)
    {
        super.OnIgnitedThis(fire_source);

        if (!g_Game || !g_Game.IsServer())
            return;
        if (!ChefZ_ConsumeOneBark())
            return;

        m_ChefZ_FuelLeftSec = CHEFZ_SECONDS_PER_BARK;
        ChefZ_SetLit(true);
    }

    //==========================================================================
    // Brennen
    //==========================================================================

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

    /**
     * Timer-Rueckruf, deshalb oeffentlich.
     *
     * Ist das schwelende Stueck herunter, kommt das naechste aus dem Cargo.
     * Liegt keines mehr da, geht der Schrank aus - und ein laufender
     * Raeucherjob pausiert ab dem naechsten Tick der Basis, weil
     * ChefZ_HasHeat und ChefZ_IsPowered dann beide "nein" sagen.
     */
    void ChefZ_OnBurnTick()
    {
        if (!g_Game || !g_Game.IsServer())
            return;
        if (!m_ChefZ_Lit)
            return;

        m_ChefZ_FuelLeftSec = m_ChefZ_FuelLeftSec - CHEFZ_BURN_TICK_SEC;
        if (m_ChefZ_FuelLeftSec > 0.0)
            return;

        if (ChefZ_ConsumeOneBark())
        {
            m_ChefZ_FuelLeftSec = CHEFZ_SECONDS_PER_BARK;
            return;
        }

        m_ChefZ_FuelLeftSec = 0.0;
        ChefZ_SetLit(false);
    }

    /**
     * Nimmt EIN Stueck Rinde aus dem Cargo.
     *
     * Rinde ist im Cargo Stueckware ohne eigene Menge - Vanilla stapelt sie
     * ueber den Anhangsslot (CfgSlots Slot_Bark stackMax = 8,
     * scripts - 1.29/config.cpp:680-686), nicht ueber varQuantity. Der
     * Mengenzweig steht trotzdem da: sollte ein Stapel doch einmal eine Menge
     * tragen, wird davon abgezogen statt der ganze Stapel geloescht.
     */
    protected bool ChefZ_ConsumeOneBark()
    {
        ItemBase bark = ChefZ_FindBark();
        if (!bark)
            return false;

        float quantity = bark.GetQuantity();
        if (quantity > 1.0)
        {
            bark.AddQuantity(-1.0);
            return true;
        }

        bark.Delete();
        return true;
    }

    //! Das erste Stueck Rinde im Cargo, oder null.
    protected ItemBase ChefZ_FindBark()
    {
        GameInventory inventory = GetInventory();
        if (!inventory)
            return null;

        CargoBase cargo = inventory.GetCargo();
        if (!cargo)
            return null;

        int n = cargo.GetItemCount();
        for (int i = 0; i < n; i++)
        {
            Bark_ColorBase bark = Bark_ColorBase.Cast(cargo.GetItem(i));
            if (bark)
                return bark;
        }
        return null;
    }

    //! Stuecke Rinde im Cargo. Bark_ColorBase und nicht die beiden
    //! Einzelklassen: Eiche und Birke sind fuer den Schrank dasselbe, und ein
    //! Mod, der eine dritte Rinde mitbringt, wird ohne Zutun mitgezaehlt.
    protected int ChefZ_CountBark()
    {
        int count = 0;

        GameInventory inventory = GetInventory();
        if (!inventory)
            return count;

        CargoBase cargo = inventory.GetCargo();
        if (!cargo)
            return count;

        int n = cargo.GetItemCount();
        for (int i = 0; i < n; i++)
        {
            if (Bark_ColorBase.Cast(cargo.GetItem(i)))
                count = count + 1;
        }
        return count;
    }

    //==========================================================================
    // Eingangsseite
    //==========================================================================

    /**
     * Rinde und Raeuchergut hinein, sonst nichts.
     *
     * Der Cargo ist zugleich Brennstofflager und Raeucherkammer - getrennte
     * Bereiche gibt die Engine nicht her (siehe Kopf von ChefZ_StationGate).
     * Deshalb muessen hier BEIDE Sorten durch, und genau darauf ist zu achten:
     * ein Torwaechter, der nur an das Raeuchergut denkt, sperrt den eigenen
     * Brennstoff aus und macht den Schrank ein zweites Mal unbenutzbar.
     *
     * Fleisch UND Fisch, weil Smoking.json beides raeuchert; die Ergebnisse
     * (ChefZ_SmokedMeat, ChefZ_SmokedFish, ChefZ_SmokedSausage) liegen in
     * denselben beiden Kategorien und duerfen deshalb im Cargo entstehen und
     * liegen bleiben.
     *
     * Beleg: EntityAI.CanReceiveItemIntoCargo, scripts - 1.29/3_Game/DayZ/
     * Entities/EntityAI.c:1550-1559; Ueberschreibung wie Barrel_ColorBase.c:512.
     */
    override bool CanReceiveItemIntoCargo(EntityAI item)
    {
        if (!super.CanReceiveItemIntoCargo(item))
            return false;
        if (!item)
            return false;

        // Brennstoff.
        if (Bark_ColorBase.Cast(item))
            return true;

        // Solange die Register nicht stehen, wird nichts abgewiesen - sonst
        // naehme der Schrank in genau diesem Fenster gar nichts an.
        if (!ChefZ_StationGate.ChefZ_RegistryReady())
            return true;

        if (ChefZ_StationGate.ChefZ_InCategory(item, CHEFZ_CAT_MEAT))
            return true;
        if (ChefZ_StationGate.ChefZ_InCategory(item, CHEFZ_CAT_FISH))
            return true;

        return false;
    }

    /**
     * Wer den Schrank aufhebt, loescht ihn.
     *
     * Ein brennender Raeucherschrank im Rucksack waere der einzige Gegenstand
     * im Spiel, der das darf. Vanilla loest denselben Fall bei der Fackel
     * ueber die Aktion; hier ist der Ortswechsel die ehrlichere Stelle, weil
     * der Schrank auf jedem Weg aus der Welt verschwinden kann.
     */
    override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
    {
        super.EEItemLocationChanged(oldLoc, newLoc);

        if (!m_ChefZ_Lit)
            return;
        if (newLoc.GetType() == InventoryLocationType.GROUND)
            return;

        ChefZ_SetLit(false);
    }

    //==========================================================================
    // Rauch (Client)
    //==========================================================================

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
                m_ChefZ_Smoke = ParticleManager.GetInstance().PlayOnObject(ParticleList.CAMP_SMALL_SMOKE, this, Vector(0, CHEFZ_SMOKE_HEIGHT_M, 0));
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
