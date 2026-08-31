//==============================================================================
// Die Station der Fleischkette.
//
// Andockregel woertlich aus dem Kopf von ChefZ_ProcessingStation_Base.c:
//
//   config.cpp   class ChefZ_MeatGrinder : <eine Vanilla-Klasse> { ... };
//   Stationsdatensatz  id == Klassenname, processes[] = { ... }
//   Skript       class ChefZ_MeatGrinder extends ChefZ_ProcessingStation_Base
//
// WELCHE Prozesse die Station anbietet, steht ausschliesslich im
// Stationsdatensatz (Config/Processing/Stations.json); WAS aus WAS wird, steht
// in den Transforms (ChefZ_Meat/Config/Processing/Meat.json). Beides bleibt
// wahr - hier steht nur das, was Daten nicht ausdruecken koennen.
//
// In dieser Datei steht EINE Station. Das Schneidebrett gibt es nicht mehr:
// Schneiden ist "Zutat + Messer kombinieren" (HANDCRAFT mit CUTTING_TOOL),
// Entscheidung vom 29.08.2026 - Begruendung in der config.cpp dieses Moduls.
//
//==============================================================================
// ### 31.08.2026 ### WARUM HIER JETZT DOCH ETWAS STEHT
//==============================================================================
//
// Alex' Testbericht: "Fleischwolf hat keine Funktion". Die Ursache lag nicht
// in dieser Datei, sondern in der config.cpp: dem Wolf fehlte der
// Cargo-Block. ChefZ_ProcessingStation_Base sammelt seine Zutaten
// ausschliesslich ueber ChefZ_FactCollector.CollectFromCargo aus
// GetInventory().GetCargo() - ohne Cargo gibt es kein GetCargo(), und alle
// ZWOELF Transforms der Fleischkette waren unerreichbar, ohne dass irgendwo
// etwas nach einem Fehler ausgesehen haette. Genau dieser Fehler hatte schon
// das entfernte Schneidebrett erwischt. Der Cargo steht jetzt da (5x3).
//
// Mit dem Cargo kommen zwei Dinge, die nur im Skript gehen:
//
//   1. DER TORWAECHTER. Der Cargo ist Ein- und Ausgang zugleich - getrennte
//      Bereiche gibt die Engine nicht her (Begruendung im Kopf von
//      ChefZ_StationGate). Ohne Torwaechter waere der Wolf ein 15-Zellen-Lager
//      mit Kurbel, in das Munition und Schuhe passen.
//   2. DER SELBSTNACHSTART. PROCESS_GRIND_MEAT ist seit heute STATION_TIMED
//      (vorher STATION_ACTION). Alex' Vorgabe: "pro Einheit 30 Sekunden bis 1x
//      Hackfleisch, solange Fleisch im Cargo liegt". Der Spieler kurbelt
//      EINMAL an, danach gehoert der Takt der Station.
//
// Das Muster des Nachstarts ist nicht neu und ausdruecklich nicht neu
// erfunden: es ist woertlich das der Honigschleuder
// (ChefZ_HoneyExtractor.ChefZ_CompleteJob, dieselbe Ordnerebene). Auch die
// Begruendungen gelten dort wie hier - der Neustart geht ueber CallLater in
// den naechsten Frame, weil super den Job-Timer gerade angehalten hat und ein
// Timer.Run aus dem eigenen Rueckruf heraus kein belegter Pfad ist
// (GeyserArea.c:49, FireplaceBase.c:1767).
//
// PROCESS_STUFF_SAUSAGE bleibt STATION_ACTION und wird NICHT nachgestartet:
// Wurst fuellen hat drei bis fuenf Eingaenge, und eine Station, die sich
// selbst nachstartet, wuerde beim naechsten Anlauf eine andere Kombination
// erwischen als die, die der Spieler im Sinn hatte. Wolfen hat einen Eingang -
// da gibt es nichts zu verwechseln.
//
// Diese Station fasst Vanillas Kochkette an keiner Stelle an (11 E6).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_MeatGrinder extends ChefZ_ProcessingStation_Base
{
    //! Der EINE Prozess, den die Station sich selbst nachstartet.
    static const string CHEFZ_GRIND_PROCESS = "PROCESS_GRIND_MEAT";

    //! Was in den Wolf darf. Kategorien und nicht Klassennamen - die
    //! ausfuehrliche Begruendung steht im Kopf von ChefZ_StationGate.
    //!
    //!   MEAT    trifft ueber die Closure auch MINCED_MEAT, SAUSAGE,
    //!           WILD_MEAT, DOMESTIC_MEAT, POULTRY und PREDATOR_MEAT. Damit
    //!           sind die Eingaenge ALLER sechs Wolf-Transforms und die
    //!           Ergebnisse aller sechs Wurst-Transforms abgedeckt, ohne dass
    //!           hier eine einzige Fleischklasse steht.
    //!   CASING  die Daerme (Guts, SmallGuts) - Pflichteingang jeder Wurst.
    //!   SPICE   trifft ueber die Closure auch SALT.
    //!   HERB    trifft ueber die Closure auch DRIED_HERB.
    //!   FAT     Lard. KEIN Eingang, sondern ein NEBENPRODUKT von
    //!           TR_MeatToMinced, TR_PorkToMinced, TR_BoarToMinced und
    //!           TR_BearToMinced. Es entsteht IM Cargo, und was im Cargo
    //!           entstehen soll, muss durch denselben Torwaechter - sonst
    //!           faellt das Nebenprodukt lautlos aus.
    static const string CHEFZ_CAT_MEAT   = "MEAT";
    static const string CHEFZ_CAT_CASING = "CASING";
    static const string CHEFZ_CAT_SPICE  = "SPICE";
    static const string CHEFZ_CAT_HERB   = "HERB";
    static const string CHEFZ_CAT_FAT    = "FAT";

    //! Die beiden Unterkategorien von MEAT, die NICHT noch einmal durch den
    //! Wolf sollen. Sie stehen hier wegen ChefZ_HasUngroundMeat - Begruendung
    //! dort.
    static const string CHEFZ_CAT_MINCED  = "MINCED_MEAT";
    static const string CHEFZ_CAT_SAUSAGE = "SAUSAGE";

    //! Der Spieler, der angekurbelt hat. Der Folgejob traegt seine Kennung,
    //! damit XP je Einheit an ihn geht - und erst nach dem Abschluss (Regel 7).
    //! 0 heisst "niemand beteiligt", wie ueberall im Core.
    protected int m_ChefZ_PendingActorId;

    void ChefZ_MeatGrinder()
    {
        m_ChefZ_PendingActorId = 0;
    }

    /**
     * Fleischiges, Daerme, Gewuerze und Kraeuter hinein - sonst nichts.
     *
     * Keine Stueckgrenze: das Gitter (5x3) IST die Grenze. Anders als bei der
     * Honigschleuder gibt es hier keine Sorte, von der man genau fuenf haben
     * darf - der Wolf verarbeitet, was da ist, und ist voll, wenn er voll ist.
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

        // Solange die Register nicht stehen, wird nichts abgewiesen - sonst
        // naehme der Wolf in genau diesem Fenster gar nichts an.
        if (!ChefZ_StationGate.ChefZ_RegistryReady())
            return true;

        if (ChefZ_StationGate.ChefZ_InCategory(item, CHEFZ_CAT_MEAT))
            return true;
        if (ChefZ_StationGate.ChefZ_InCategory(item, CHEFZ_CAT_CASING))
            return true;
        if (ChefZ_StationGate.ChefZ_InCategory(item, CHEFZ_CAT_SPICE))
            return true;
        if (ChefZ_StationGate.ChefZ_InCategory(item, CHEFZ_CAT_HERB))
            return true;
        if (ChefZ_StationGate.ChefZ_InCategory(item, CHEFZ_CAT_FAT))
            return true;

        return false;
    }

    /**
     * Der Abschluss eines Wolfgangs stoesst den naechsten an.
     *
     * Der Prozess wird VOR super gelesen: die Basis loescht den Job nach dem
     * Lauf (job.Clear()), und danach ist nicht mehr feststellbar, ob gerade
     * gewolft oder gewurstet wurde. Nur das Wolfen laeuft weiter.
     */
    override bool ChefZ_CompleteJob(int slotIndex)
    {
        int actorId = 0;
        bool wasGrinding = false;

        if (slotIndex >= 0 && slotIndex < m_ChefZ_Jobs.Count())
        {
            ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(slotIndex);
            actorId = job.actorIdentityId;

            ChefZ_Sym grind = ChefZ_SymbolTable.Lookup(CHEFZ_GRIND_PROCESS);
            if (ChefZ_SymbolTable.IsValid(grind) && job.processSym == grind)
                wasGrinding = true;
        }

        bool ok = super.ChefZ_CompleteJob(slotIndex);
        if (!ok)
            return false;
        if (!wasGrinding)
            return true;

        m_ChefZ_PendingActorId = actorId;
        if (g_Game)
            g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ChefZ_GrindNextPiece, 0, false);

        return true;
    }

    /**
     * Startet den naechsten Wolfgang, solange Fleisch im Cargo liegt.
     *
     * Oeffentlich, weil CallLater ihn ruft. Scheitert der Start - kein
     * Fleisch mehr oder Cargo voll -, steht der Wolf und wartet auf das
     * naechste Ankurbeln. Das Nachlegen allein startet nichts; der Anstoss
     * gehoert dem Spieler, und ein Cargo-Haken liesse den Wolf auch ohne jedes
     * Ankurbeln anlaufen.
     */
    void ChefZ_GrindNextPiece()
    {
        if (!g_Game || !g_Game.IsServer())
            return;
        if (!ChefZ_HasUngroundMeat())
            return;

        ChefZ_Sym process = ChefZ_SymbolTable.Lookup(CHEFZ_GRIND_PROCESS);
        if (!ChefZ_SymbolTable.IsValid(process))
            return;

        string err;
        if (!ChefZ_BeginJob(process, null, m_ChefZ_PendingActorId, err))
        {
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
            {
                ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS, "Fleischwolf \"" + GetType() + "\" steht: " + err);
            }
        }
    }

    /**
     * Liegt noch UNGEWOLFTES Fleisch im Cargo?
     *
     * Diese Frage waere ueberfluessig, wenn der Transform sie beantworten
     * wuerde. Er tut es nicht: TR_MeatToMinced
     * (ChefZ_Meat/Config/Processing/Meat.json:91-122) sucht seinen Eingang
     * ueber { allOf: [ { category: "MEAT" }, { vanillaStage: "Raw" } ] } - und
     * sein eigenes ERGEBNIS erfuellt das ebenfalls: ChefZ_MincedMeat traegt
     * die Kategorien MEAT und MINCED_MEAT (Meat.json:244-258) und ist eine
     * Nahrung mit Garstufe Raw (ChefZ_Meat/config.cpp:675-685). Hackfleisch
     * ist damit ein gueltiger Eingang fuer das Hackfleischrezept.
     *
     * Solange ein Spieler jeden Vorgang einzeln anstiess (STATION_ACTION), war
     * das eine Kuriositaet - er haette es von Hand tun muessen. Mit dem
     * Selbstnachstart waere es ein Perpetuum mobile: der Wolf wolfte sein
     * eigenes Hack alle 30 Sekunden neu, ewig, und wuerfe dabei in 35 % der
     * Faelle Lard als Nebenprodukt ab. Ein Fleischwolf, der aus einem einzigen
     * Steak unbegrenzt Speck macht.
     *
     * Die SAUBERE Loesung waere ein engerer Selektor im Transform -
     * { state: "RAW" } statt { vanillaStage: "Raw" }, denn Hackfleisch hat
     * defaultState PREPARED. Der Transform gehoert aber ChefZ_Meat, nicht
     * diesem Modul; die Aenderung ist dort gemeldet. Bis dahin haelt die
     * Station selbst an: sie startet sich nur nach, solange etwas da ist, das
     * noch nicht durch den Wolf war.
     *
     * MINCED_MEAT und SAUSAGE sind Unterkategorien von MEAT (Kategoriebaum:
     * ChefZ_Registry/Config/Categories.json) - deshalb reicht es, sie
     * abzuziehen; alle uebrigen Fleischarten bleiben Eingang.
     */
    protected bool ChefZ_HasUngroundMeat()
    {
        if (!ChefZ_StationGate.ChefZ_RegistryReady())
            return false;

        GameInventory inventory = GetInventory();
        if (!inventory)
            return false;

        CargoBase cargo = inventory.GetCargo();
        if (!cargo)
            return false;

        int n = cargo.GetItemCount();
        for (int i = 0; i < n; i++)
        {
            EntityAI entry = cargo.GetItem(i);
            if (!ChefZ_StationGate.ChefZ_InCategory(entry, CHEFZ_CAT_MEAT))
                continue;
            if (ChefZ_StationGate.ChefZ_InCategory(entry, CHEFZ_CAT_MINCED))
                continue;
            if (ChefZ_StationGate.ChefZ_InCategory(entry, CHEFZ_CAT_SAUSAGE))
                continue;
            return true;
        }

        return false;
    }
}
