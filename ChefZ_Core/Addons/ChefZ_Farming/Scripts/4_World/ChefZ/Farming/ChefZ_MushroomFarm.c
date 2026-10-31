//==============================================================================
// ChefZ_MushroomFarm - Pilzregal, Anzuchtkasten und Myzel (Slice "mushrooms",
// 30.10.2026).
//
// DER KREISLAUF: Pilz trocknen -> im Moerser zu Myzel der Art moersern ->
// Myzel in den Myzelplatz eines Anzuchtkastens -> Kasten ins Pilzregal ->
// nach chefzGrowSeconds stehen bis zu drei Pilze in den Fruchtplaetzen des
// Kastens, das Myzel ist verbraucht. Trocknen und Moersern sind Datensaetze
// (Config/Processing/Mushroom_Transforms.json); was HIER steht, ist allein das
// Wachsen.
//
// BAUFORM WIE DIE IMKEREI (ChefZ_Apiary.c), und zwar bewusst:
//
//   Fortschritt    = varQuantity AM KASTEN (0..100, quantityBar). Die Engine
//                    synchronisiert und speichert sie selbst - kein eigener
//                    Persistenzblock, kein OnStoreSave.
//   Takt           = ein Timer AM REGAL, alle zehn Sekunden ueber die sechs
//                    Kastenplaetze. Ein Kasten ausserhalb des Regals waechst
//                    nicht und verliert dabei nichts.
//   Server steht   = keine Zeit vergeht (PlantBase-Verhalten, Begruendung am
//                    Bienenstock).
//
// Was das Myzel waechst, wie lange und wie viel, steht in der CONFIG des
// Myzels (chefzMushroomClass, chefzGrowSeconds, chefzFruitCount). Eine neue
// Art braucht eine Config-Klasse und eine Moerserzeile, keine Zeile hier.
//
// Keine modded class, keine eigene Action, keine Zeile im Core. Das Regal ist
// KEINE ChefZ_ProcessingStation_Base: es hat keinen Vorgang, den ein Spieler
// startet - nur den Takt. Platzieren laeuft ueber Vanillas Hologramm
// (ActionTogglePlaceObject + ActionPlaceObject), dieselben zwei Aktionen, die
// die Stationsbasis jeder Station gibt.
//
// Layer: 4_World.
//==============================================================================

//! Das Saatgut. Liest die drei Zuchtfelder aus der eigenen Config-Klasse.
class ChefZ_Mycelium_Base extends ItemBase
{
    //! Rueckfall, falls eine Myzelklasse chefzGrowSeconds nicht setzt oder 0
    //! eintraegt - eine Division durch null im Wachstumsschritt waere sonst
    //! ein sofort voller Kasten.
    static const float CHEFZ_DEFAULT_GROW_SECONDS = 14400.0;

    //! Was im Kasten waechst. Leer = nichts (Fehler in der Config, wird im
    //! Kasten geloggt und nicht still verbraucht).
    string ChefZ_MushroomClass()
    {
        return ConfigGetString("chefzMushroomClass");
    }

    float ChefZ_GrowSeconds()
    {
        float seconds = ConfigGetFloat("chefzGrowSeconds");
        if (seconds <= 0.0)
            return CHEFZ_DEFAULT_GROW_SECONDS;
        return seconds;
    }

    int ChefZ_FruitCount()
    {
        int count = ConfigGetInt("chefzFruitCount");
        if (count < 1)
            return 1;
        return count;
    }
}

//==============================================================================
// Der Anzuchtkasten. Ein Myzelplatz (ChefZ_TrayMycel) und drei Fruchtplaetze
// (ChefZ_TrayMushroom1..3), die im Modell als Proxys auf der Erde stehen.
//==============================================================================
class ChefZ_MushroomTray extends ItemBase
{
    static const string CHEFZ_SLOT_MYCEL         = "ChefZ_TrayMycel";
    static const string CHEFZ_FRUIT_SLOT_PREFIX  = "ChefZ_TrayMushroom";
    static const int    CHEFZ_FRUIT_SLOTS        = 3;

    //! Die beiden Erdtexturen: ohne Myzel braune Erde, mit Myzel weiss
    //! durchzogen. Index 0 = Selektion "camo" (hiddenSelections der Config).
    static const string CHEFZ_TEX_PLAIN      = "ChefZ\\ChefZ_Devices\\mushroom\\data\\MushroomTray_co.paa";
    static const string CHEFZ_TEX_INOCULATED = "ChefZ\\ChefZ_Devices\\mushroom\\data\\MushroomTray_inoculated_co.paa";

    override void EEInit()
    {
        super.EEInit();
        ChefZ_UpdateTrayVisual(ChefZ_GetMycel() != null);
    }

    /**
     * Myzel hinein: Optik umschalten und - nur wenn ein SPIELER es gerade
     * hineinlegt - den Balken auf null stellen.
     *
     * Die Wache IsInitialized() ist dieselbe wie am Bienenstock
     * (ChefZ_Beehive.ChefZ_ResetSpawnedFrame): dieser Haken feuert auch beim
     * Laden aus dem Spielstand, und dort wuerde die Nullstellung jeden
     * halbgewachsenen Kasten bei jedem Neustart zuruecksetzen. Beim Laden ist
     * der Kasten noch nicht initialisiert (DeferredInit, EntityAI.c:282),
     * beim Einlegen durch einen Spieler laengst.
     *
     * Warum ueberhaupt nullstellen: ein per Admin-Werkzeug gespawnter Kasten
     * kommt mit voller Menge in die Welt. Ohne die Nullstellung truege das
     * erste Myzel sofort Frucht.
     */
    override void EEItemAttached(EntityAI item, string slot_name)
    {
        super.EEItemAttached(item, slot_name);

        if (slot_name != CHEFZ_SLOT_MYCEL)
            return;

        ChefZ_UpdateTrayVisual(true);

        if (!g_Game || !g_Game.IsServer())
            return;
        if (!IsInitialized())
            return;
        SetQuantity(0.0);
    }

    //! Myzel heraus: Optik zurueck, und der Fortschritt ist verloren - das
    //! Myzel wurde gestoert. Auch der eigene Verbrauch beim Fruchten kommt
    //! hier vorbei; dort steht der Balken ohnehin schon auf null.
    override void EEItemDetached(EntityAI item, string slot_name)
    {
        super.EEItemDetached(item, slot_name);

        if (slot_name != CHEFZ_SLOT_MYCEL)
            return;

        ChefZ_UpdateTrayVisual(false);

        if (!g_Game || !g_Game.IsServer())
            return;
        SetQuantity(0.0);
    }

    //! In den Myzelplatz nur Myzel. Die Fruchtplaetze regelt die Config:
    //! nur Pilze, deren inventorySlot[] sie nennt.
    override bool CanReceiveAttachment(EntityAI attachment, int slotId)
    {
        if (!super.CanReceiveAttachment(attachment, slotId))
            return false;

        int mycelSlot = InventorySlots.GetSlotIdFromString(CHEFZ_SLOT_MYCEL);
        if (slotId != mycelSlot)
            return true;

        return ChefZ_Mycelium_Base.Cast(attachment) != null;
    }

    //! Laeuft auf beiden Seiten - die Anhaenger kennt auch der Client, und
    //! SetObjectTexture wirkt nur lokal. Das Argument statt einer Suche: in
    //! EEItemDetached steckt das Myzel noch im Slot, eine Suche faende es.
    protected void ChefZ_UpdateTrayVisual(bool inoculated)
    {
        if (inoculated)
        {
            SetObjectTexture(0, CHEFZ_TEX_INOCULATED);
            return;
        }
        SetObjectTexture(0, CHEFZ_TEX_PLAIN);
    }

    static string ChefZ_FruitSlotName(int number)
    {
        return CHEFZ_FRUIT_SLOT_PREFIX + number.ToString();
    }

    ChefZ_Mycelium_Base ChefZ_GetMycel()
    {
        GameInventory inv = GetInventory();
        if (!inv)
            return null;
        return ChefZ_Mycelium_Base.Cast(inv.FindAttachmentByName(CHEFZ_SLOT_MYCEL));
    }

    int ChefZ_FreeFruitSlots()
    {
        GameInventory inv = GetInventory();
        if (!inv)
            return 0;

        int free = 0;
        for (int i = 1; i <= CHEFZ_FRUIT_SLOTS; i++)
        {
            if (!inv.FindAttachmentByName(ChefZ_FruitSlotName(i)))
                free = free + 1;
        }
        return free;
    }

    /**
     * Ein Takt des Regals. Nur der Server, nur mit Myzel, nur mit einem
     * freien Fruchtplatz - stehen noch drei Pilze im Kasten, wartet das Myzel,
     * bis jemand erntet.
     *
     * Ohne Myzel und mit Menge > 0 wird der Balken genullt: das ist ein
     * gespawnter Kasten oder ein Kasten, dem jemand das Myzel genommen hat.
     *
     * Der Schritt wird aus dem Maximum gerechnet (wie am Raehmchen) und mit
     * AddQuantity gegangen - SetQuantityNormalized rundete die kleinen
     * Schritte auf null.
     */
    void ChefZ_GrowTick(float deltaSec)
    {
        if (!g_Game || !g_Game.IsServer())
            return;
        if (IsDamageDestroyed())
            return;

        ChefZ_Mycelium_Base mycel = ChefZ_GetMycel();
        if (!mycel)
        {
            if (GetQuantity() > 0.0)
                SetQuantity(0.0);
            return;
        }

        if (ChefZ_FreeFruitSlots() == 0)
            return;

        // Ein voller Balken ist ein Fruchten, das im letzten Takt nichts
        // anlegen konnte - erneut versuchen, nicht weiterzaehlen.
        if (IsFullQuantity())
        {
            ChefZ_Fruit(mycel);
            return;
        }

        float max = GetQuantityMax();
        float step = max * deltaSec / mycel.ChefZ_GrowSeconds();
        float remaining = max - GetQuantity();

        if (remaining > step)
        {
            AddQuantity(step);
            return;
        }

        SetQuantityMax();
        ChefZ_Fruit(mycel);
    }

    /**
     * Die Pilze stehen. Legt bis zu chefzFruitCount Pilze in die freien
     * Fruchtplaetze, verbraucht das Myzel und setzt den Balken auf null.
     *
     * CreateAttachmentEx (Inventory.c:215) legt den Pilz direkt im Slot an -
     * derselbe Aufruf, mit dem Vanilla ein Codeschloss an ein Tor haengt
     * (RebuildingSpawner.c:200). Ein so angelegter Pilz ist frisch: Vanillas
     * Wurf auf verdorben/getrocknet (MushroomBase.EEOnCECreate) gilt nur fuer
     * Loot aus der Zentralwirtschaft.
     *
     * Entsteht KEIN Pilz (Klassenname unbekannt, Slot verweigert), bleibt
     * alles stehen: voller Balken, Myzel im Kasten, und der naechste Takt
     * versucht es erneut. Lieber ein haengender Kasten als ein verbrauchtes
     * Myzel ohne Ernte.
     */
    protected void ChefZ_Fruit(notnull ChefZ_Mycelium_Base mycel)
    {
        string cls = mycel.ChefZ_MushroomClass();
        if (cls == "" || !g_Game.ConfigIsExisting("CfgVehicles " + cls))
        {
            ChefZ_LogTray("Myzel " + mycel.GetType() + " nennt keine gueltige chefzMushroomClass ('" + cls + "').");
            return;
        }

        GameInventory inv = GetInventory();
        if (!inv)
            return;

        int wanted = mycel.ChefZ_FruitCount();
        int made = 0;
        for (int i = 1; i <= CHEFZ_FRUIT_SLOTS; i++)
        {
            if (made >= wanted)
                break;

            string slotName = ChefZ_FruitSlotName(i);
            if (inv.FindAttachmentByName(slotName))
                continue;

            int slotId = InventorySlots.GetSlotIdFromString(slotName);
            EntityAI fruit = inv.CreateAttachmentEx(cls, slotId);
            if (fruit)
                made = made + 1;
        }

        if (made == 0)
        {
            ChefZ_LogTray(cls + " liess sich in keinem Fruchtplatz anlegen.");
            return;
        }

        SetQuantity(0.0);
        mycel.DeleteSafe();
        ChefZ_LogTray(made.ToString() + "x " + cls + " gewachsen.");
    }

    //! Hinter der Kanalwache wie am Bienenstock - die Zeichenkette entsteht
    //! nur, wenn jemand sie liest.
    protected void ChefZ_LogTray(string msg)
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
            return;
        ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS, "Zuchtkasten " + GetType() + ": " + msg);
    }
}

//==============================================================================
// Das Pilzregal. Sechs Kastenplaetze (ChefZ_MushroomTray1..6, sichtbar ueber
// die Proxys des Modells) und der Takt, der die Kaesten wachsen laesst.
//==============================================================================
class ChefZ_MushroomShelf extends ItemBase
{
    //! Zehn Sekunden wie der Fuelltakt des Bienenstocks. Bei zwei bis acht
    //! Stunden Wachstum ist ein Schritt weit unter dem, was ein Balken zeigt.
    static const float  CHEFZ_GROW_TICK_SEC     = 10.0;
    static const string CHEFZ_TRAY_SLOT_PREFIX  = "ChefZ_MushroomTray";
    static const int    CHEFZ_TRAY_SLOTS        = 6;

    protected ref Timer m_ChefZ_GrowTimer;

    void ~ChefZ_MushroomShelf()
    {
        if (m_ChefZ_GrowTimer)
        {
            m_ChefZ_GrowTimer.Stop();
            m_ChefZ_GrowTimer = null;
        }
    }

    //! Beladen bleibt das Regal stehen (Spieltest 31.10.2026): erst die Kaesten
    //! heraus, dann in die Hand oder ins Inventar.
    protected bool ChefZ_IsLoaded()
    {
        GameInventory inventory = GetInventory();
        return inventory && inventory.AttachmentCount() > 0;
    }

    override bool CanPutIntoHands(EntityAI parent)
    {
        if (!super.CanPutIntoHands(parent))
            return false;
        return !ChefZ_IsLoaded();
    }

    override bool CanPutInCargo(EntityAI parent)
    {
        if (!super.CanPutInCargo(parent))
            return false;
        return !ChefZ_IsLoaded();
    }

    //! Aufstellen mit Vanillas Hologramm - dieselben zwei Aktionen, die
    //! ChefZ_ProcessingStation_Base.SetActions jeder Station gibt. Das
    //! Hologramm zeigt das Regal selbst (Hologram.GetProjectionName liefert
    //! fuer alles, was kein Bausatz ist, den eigenen Typ).
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionTogglePlaceObject);
        AddAction(ActionPlaceObject);
    }

    //! BEWUSSTE AUSNAHME von "override mit super", wie an ChefZ_BeehiveKit:
    //! ein Zusage-Praedikat mit festem Wert. ItemBase sagt nein, und ein
    //! super-UND zoege die Zusage immer auf false.
    override bool IsDeployable()
    {
        return true;
    }

    override void EEInit()
    {
        super.EEInit();
        ChefZ_StartGrowTimer();
    }

    override void AfterStoreLoad()
    {
        super.AfterStoreLoad();
        ChefZ_StartGrowTimer();
    }

    //! Ein Kasten kommt ins Regal - den Takt wecken. Er haelt sich von
    //! selbst an, wenn kein Kasten mehr waechst.
    override void EEItemAttached(EntityAI item, string slot_name)
    {
        super.EEItemAttached(item, slot_name);
        ChefZ_StartGrowTimer();
    }

    static string ChefZ_TraySlotName(int number)
    {
        return CHEFZ_TRAY_SLOT_PREFIX + number.ToString();
    }

    /**
     * Darf in diesem Regal gerade etwas wachsen?
     *
     *   Hologramm   nein - Vanillas Hologramm erzeugt auch serverseitig ein
     *               echtes Objekt des Regaltyps (Begruendung am Bienenstock,
     *               ChefZ_Beehive.ChefZ_StartFillTimer).
     *   getragen    nein - ein Regal in den Armen eines Spielers steht nicht.
     *   ruiniert    nein.
     */
    protected bool ChefZ_CanGrow()
    {
        if (IsHologram())
            return false;
        if (GetHierarchyRootPlayer())
            return false;
        return !IsDamageDestroyed();
    }

    //! Dieselbe Form wie ChefZ_Beehive.ChefZ_StartFillTimer: Serverwache,
    //! nicht doppelt starten, Objekt einmal anlegen und behalten.
    protected void ChefZ_StartGrowTimer()
    {
        if (!g_Game || !g_Game.IsServer())
            return;
        if (IsHologram())
            return;
        if (m_ChefZ_GrowTimer && m_ChefZ_GrowTimer.IsRunning())
            return;

        if (!m_ChefZ_GrowTimer)
            m_ChefZ_GrowTimer = new Timer(CALL_CATEGORY_SYSTEM);

        m_ChefZ_GrowTimer.Run(CHEFZ_GROW_TICK_SEC, this, "ChefZ_OnGrowTick", null, true);
    }

    protected void ChefZ_StopGrowTimer()
    {
        if (!m_ChefZ_GrowTimer)
            return;
        m_ChefZ_GrowTimer.Stop();
    }

    /**
     * Ein Takt ueber alle sechs Kastenplaetze, in Slotreihenfolge.
     * OEFFENTLICH, weil Timer.Run die Methode ueber ihren Namen ruft.
     *
     * Der Kasten wird JE TAKT NEU gesucht und nie gemerkt - ein Spieler kann
     * ihn zwischen zwei Takten herausnehmen, und ein gemerkter Zeiger zeigte
     * dann auf einen Kasten im Rucksack.
     *
     * Der Takt haelt sich erst an, wenn KEIN Kasten mehr im Regal steht, und
     * ein neuer Kasten weckt ihn (EEItemAttached). Bewusst nicht schon, wenn
     * keiner waechst: Myzel in einen Kasten zu legen, der bereits im Regal
     * steht, ist ein Anhaengen am KASTEN - das Regal erfaehrt davon nichts
     * und koennte seinen Takt nicht wieder wecken. Sechs Slotabfragen alle
     * zehn Sekunden kosten nichts.
     */
    void ChefZ_OnGrowTick()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        GameInventory inv = GetInventory();
        if (!inv)
            return;

        bool canGrow = ChefZ_CanGrow();
        int trays = 0;
        for (int i = 1; i <= CHEFZ_TRAY_SLOTS; i++)
        {
            ChefZ_MushroomTray tray = ChefZ_MushroomTray.Cast(inv.FindAttachmentByName(ChefZ_TraySlotName(i)));
            if (!tray)
                continue;
            trays = trays + 1;
            if (canGrow)
                tray.ChefZ_GrowTick(CHEFZ_GROW_TICK_SEC);
        }

        if (trays == 0)
            ChefZ_StopGrowTimer();
    }
}
