//==============================================================================
// ChefZ_CookbookItem - das Kochbuch als Gegenstand
//
// Entwurf: ChefZ_Cookbook_Workflow §6.3 ("Das Buch muss im Inventar liegen").
//
// Die Klasse ist absichtlich duenn. Ein Buch ist ein Gegenstand, den man
// besitzt - es rechnet nichts, es speichert nichts, es entscheidet nichts. Das
// Wissen haengt am Charakter (ChefZ_PlayerKnowledge), nicht am Papier; wer sein
// Buch verliert, vergisst nicht, was er gelernt hat.
//
// Die einzige Aufgabe hier: die Frage "hat dieser Spieler ein Kochbuch dabei"
// an EINER Stelle beantworten, damit Tastenkuerzel und Item-Aktion sich nicht
// widersprechen koennen.
//
// Layer: 4_World. Keine Dabs-Referenz (Regel 3) - das Buch existiert auch auf
// einem Server ohne Dabs, es laesst sich dort nur nicht aufschlagen.
//==============================================================================

class ChefZ_CookbookItem : Inventory_Base
{
    //==========================================================================
    // Kochbuch-Platz
    //
    // Der Platz selbst steht in config.cpp (CfgSlots Slot_ChefZ_Cookbook, am
    // Spieler per SurvivorBase attachments[] += und playerSlots[] +=).
    //
    // Vieles erledigt die Engine allein: "Aufheben" und CreateInInventory
    // suchen per FindFreeLocationFor zuerst einen freien Anbauplatz, und der
    // Kochbuch-Platz ist der einzige, in den das Buch passt. Uebrig bleiben die
    // Wege mit festem Ziel - auf ein Rucksackfeld gezogen, in einem Behaelter
    // mit aufgehoben, erst in die Hand genommen. Die faengt der Server hier ab.
    //
    // Regeln, damit das Buch nicht mit dem Spieler ringt:
    //   - Genau EIN Versuch je Erwerb. Erwerb = das Buch wechselt den
    //     besitzenden Spieler (OnInventoryEnter feuert nur dann, Vanilla
    //     ItemBase.OnItemLocationChanged vergleicht alten und neuen Besitzer)
    //     oder es entsteht neu.
    //   - Umraeumen im eigenen Inventar ist kein Erwerb. Wer das Buch aus dem
    //     Platz in den Rucksack zieht, behaelt es dort.
    //   - In der Hand wird nicht gezogen; der Versuch wartet, bis das Buch die
    //     Hand verlaesst (EEItemLocationChanged).
    //   - Belegter Platz: nichts tun, Versuch verfallen lassen.
    //   - Aus dem Speicher geladene Buecher (Relog, Neustart) zaehlen nicht als
    //     Erwerb - sonst wanderte ein bewusst eingeraeumtes Buch bei jedem
    //     Login zurueck.
    //==========================================================================

    static const string CHEFZ_SLOT_NAME = "ChefZ_Cookbook";

    //! Abstand zwischen Erwerb und Umlagern. Das Ereignis kommt mitten aus
    //! einer laufenden Inventarbewegung; dort eine zweite anzustossen ist in
    //! DayZ ein bekannter Weg zu Desync. Ein paar Frames spaeter ist es ruhig.
    static const int CHEFZ_SLOT_DELAY_MS = 250;

    //! So lange nach OnStoreLoad gilt jede Bewegung als Teil des Ladens.
    static const int CHEFZ_LOAD_GRACE_MS = 3000;

    protected bool m_ChefZ_SlotPending;
    protected bool m_ChefZ_SlotScheduled;
    protected bool m_ChefZ_StoreLoaded;
    protected int  m_ChefZ_StoreLoadedAt;

    static int ChefZ_SlotId()
    {
        return InventorySlots.GetSlotIdFromString(CHEFZ_SLOT_NAME);
    }

    override void EEInit()
    {
        super.EEInit();
        // Neu entstanden (Spawn, Haendler, Belohnung). Liegt es am Boden,
        // verfaellt der Versuch einfach - das Aufheben stoesst den naechsten an.
        ChefZ_ArmSlot();
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        // Nichts gelesen, nichts geschrieben - das Speicherformat bleibt
        // unberuehrt. Nur die Marke, dass dieses Buch geladen und nicht
        // erworben wurde.
        m_ChefZ_StoreLoaded   = true;
        m_ChefZ_StoreLoadedAt = g_Game.GetTime();
        m_ChefZ_SlotPending   = false;
        return true;
    }

    override void OnInventoryEnter(Man player)
    {
        super.OnInventoryEnter(player);
        if (PlayerBase.Cast(player))
            ChefZ_ArmSlot();
    }

    override void OnInventoryExit(Man player)
    {
        super.OnInventoryExit(player);
        m_ChefZ_SlotPending = false;
    }

    override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
    {
        super.EEItemLocationChanged(oldLoc, newLoc);

        // Nur ein offener Versuch wird erneut angestossen - typischerweise das
        // Buch, das in der Hand lag und jetzt im Inventar landet. Unsere eigene
        // Umlagerung laeuft mit geloeschter Marke und loest hier nichts aus.
        if (!m_ChefZ_SlotPending)
            return;
        if (newLoc.GetType() == InventoryLocationType.HANDS)
            return;
        ChefZ_ScheduleSlot();
    }

    override void EEDelete(EntityAI parent)
    {
        super.EEDelete(parent);
        m_ChefZ_SlotPending = false;
        if (g_Game)
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(ChefZ_TrySlot);
    }

    protected bool ChefZ_InLoadGrace()
    {
        if (!m_ChefZ_StoreLoaded)
            return false;
        int seit = g_Game.GetTime() - m_ChefZ_StoreLoadedAt;
        return seit < CHEFZ_LOAD_GRACE_MS;
    }

    protected void ChefZ_ArmSlot()
    {
        if (!g_Game)
            return;
        if (!g_Game.IsServer())
            return;
        if (ChefZ_InLoadGrace())
            return;

        m_ChefZ_SlotPending = true;
        ChefZ_ScheduleSlot();
    }

    protected void ChefZ_ScheduleSlot()
    {
        if (m_ChefZ_SlotScheduled)
            return;
        m_ChefZ_SlotScheduled = true;
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ChefZ_TrySlot, CHEFZ_SLOT_DELAY_MS, false);
    }

    /**
     * Der eine Versuch. Oeffentlich, weil die Warteschlange ihn ruft.
     */
    void ChefZ_TrySlot()
    {
        m_ChefZ_SlotScheduled = false;

        if (!m_ChefZ_SlotPending)
            return;
        if (IsSetForDeletion() || ChefZ_InLoadGrace())
        {
            m_ChefZ_SlotPending = false;
            return;
        }

        PlayerBase spieler = PlayerBase.Cast(GetHierarchyRootPlayer());
        if (!spieler || !spieler.IsAlive())
        {
            m_ChefZ_SlotPending = false;
            return;
        }

        InventoryLocation ort = new InventoryLocation();
        if (!GetInventory().GetCurrentInventoryLocation(ort))
        {
            m_ChefZ_SlotPending = false;
            return;
        }

        // In der Hand: offen lassen, EEItemLocationChanged meldet sich wieder.
        if (ort.GetType() == InventoryLocationType.HANDS)
            return;

        int slotId = ChefZ_SlotId();
        m_ChefZ_SlotPending = false;
        if (slotId == InventorySlots.INVALID)
            return;

        // Schon am Platz (die Engine war schneller) - fertig.
        bool istAnhang = ort.GetType() == InventoryLocationType.ATTACHMENT;
        if (istAnhang && ort.GetParent() == spieler && ort.GetSlot() == slotId)
            return;

        // Platz belegt (zweites Buch) oder gesperrt: nicht draengeln.
        if (spieler.GetInventory().FindAttachment(slotId))
            return;
        if (!spieler.GetInventory().CanAddAttachmentEx(this, slotId))
            return;

        // 1.29 und 1.30: ServerTakeEntityToTargetAttachmentEx (1.30
        // Man.c:677, 1.29 Man.c:783). Die alte ServerTakeEntityAsAttachmentEx
        // ist in 1.30 [Obsolete].
        spieler.ServerTakeEntityToTargetAttachmentEx(spieler, this, slotId);
    }

    /**
     * Die Aktion "Kochbuch aufschlagen" an das Buch haengen.
     *
     * Die Registrierung in ChefZ_ActionRegistration.c legt die Aktion nur an.
     * Sichtbar wird sie erst, wenn ein Item sie in SetActions per AddAction
     * aufnimmt - so und nicht anders funktioniert das in Vanilla
     * (1.30 scripts/4_World/DayZ/Entities/ItemBase.c:246 SetActions und :255
     * AddAction(typename); in 1.29 gleich). Ohne diese Ueberschreibung war der
     * Eintrag auf jedem Server tot, und nur das Tastenkuerzel aus
     * ChefZ_CookbookInput blieb uebrig.
     *
     * super zuerst, sonst fehlen Aufnehmen, In-die-Haende-nehmen und Ablegen.
     */
    override void SetActions()
    {
        super.SetActions();
        AddAction(ChefZ_ActionOpenCookbook);
    }

    /**
     * Traegt der Spieler ein Kochbuch bei sich?
     *
     * Durchsucht die gesamte Hierarchie, nicht nur die Haende: ein Buch im
     * Rucksack ist ein Buch. Vanillas GetInventory().EnumerateInventory
     * liefert genau diese Sicht.
     */
    static bool CarriedBy(PlayerBase spieler)
    {
        if (!spieler)
            return false;

        GameInventory inv = spieler.GetInventory();
        if (!inv)
            return false;

        array<EntityAI> alles = new array<EntityAI>();
        inv.EnumerateInventory(InventoryTraversalType.PREORDER, alles);

        for (int i = 0; i < alles.Count(); i++)
        {
            ChefZ_CookbookItem buch = ChefZ_CookbookItem.Cast(alles.Get(i));
            if (buch)
                return true;
        }
        return false;
    }
}
