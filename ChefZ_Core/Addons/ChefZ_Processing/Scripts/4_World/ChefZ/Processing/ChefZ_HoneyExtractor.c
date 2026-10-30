//==============================================================================
// ChefZ_HoneyExtractor - die Honigschleuder (Slice "apiary").
//
// Andockregel woertlich aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/
// Processing/ChefZ_ProcessingStation_Base.c:
//
//     config.cpp   class ChefZ_HoneyExtractor : Inventory_Base { ... };
//     JSON/Rang 2  { "kind":"station", "records":[{ "id":"ChefZ_HoneyExtractor" }] }
//     Skript       class ChefZ_HoneyExtractor extends ChefZ_ProcessingStation_Base {}
//
// Kein Core-Code, keine eigene Action, keine eigene Persistenz - das bleibt
// wahr. Was hier ueber die Bindung hinaus steht, ist die SELBSTLAEUFIGKEIT:
// der Spieler kurbelt einmal an (PROCESS_SPIN_HONEY, STATION_TIMED, 90 s je
// Glas), danach gehoert der Takt der Station. Jeder Abschluss startet den
// naechsten Job, solange ein entdeckelter Rahmen mit Vorrat und ein leeres
// Glas im Cargo liegen. WAS aus WAS wird, steht weiterhin allein in
// Config/Processing/Honey.json.
//
// Kapazitaeten (Auftrag: fuenf Rahmen, fuenfzehn Glaeser) zaehlt
// CanReceiveItemIntoCargo; das Cargo-Gitter selbst ist nur der Platz dafuer.
//
// Der Vorrat eines entdeckelten Rahmens ist seine varQuantity: VIER GLAESER
// PLUS EINE RESERVE-EINHEIT (5..1, Klassendefault aus ChefZ_Farming,
// unitsPerWholeItem 5). Der Applicator zieht je Glas eine Einheit ab.
//
// Die Reserve ist kein Bonus, sondern eine Notwendigkeit des Core:
// ChefZ_SlotEvaluator.PlanAmountDraw setzt destroyWhole, sobald ein Abzug die
// letzte Einheit traefe (1_Core/ChefZ/ChefZ_SlotEvaluator.c:369-376), und der
// Applicator LOESCHT das Item dann statt es auf null zu setzen (Cooking/
// ChefZ_Applicator.c:1250-1253). Duerfte das letzte Glas die letzte Einheit
// ziehen, gaebe es danach keinen Rahmen mehr, den man zum Leerrahmen machen
// koennte. Deshalb verlangt TR_SpinHoney mindestens ZWEI Einheiten: die Zuege
// geschehen bei 5, 4, 3 und 2, die Einheit 1 bleibt stehen, und ein Rahmen
// unterhalb der Schwelle wird hier nach dem Abschluss zum Leerrahmen ersetzt -
// nicht in einem Mengenhaken des Rahmens, denn waehrend des Verbrauchs haelt
// der Applicator noch Handles auf die Entities.
//
// ### 31.08.2026 - VIER STATT DREI GLAESER (Auftrag 11 von Alex) ###
// Alex verlangt zweierlei auf einmal: vier Glaeser je Rahmen UND "entdeckelte
// Rahmen zu leeren Rahmen aendern". Beides zugleich geht nur ueber den
// RAHMEN, nicht ueber die Schleuder: der Rahmen traegt jetzt FUENF Einheiten
// statt vier (ChefZ_Farming/config.cpp).
//
// Der zwischenzeitlich erwogene Weg - die Untergrenze auf 1.0 senken und alle
// vier Einheiten eines Vier-Einheiten-Rahmens ziehen - haette die vier
// Glaeser gebracht und den Rahmen gekostet: der vierte Zug haette die letzte
// Einheit getroffen, destroyWhole gesetzt und den Rahmen geloescht, bevor
// ChefZ_RetireSpentFrames ihn ueberhaupt zu sehen bekommt. Das waere genau
// die Haelfte des Auftrags gewesen. Schwelle und Reserve bleiben deshalb bei
// 2.0; geaendert hat sich allein, wie voll ein frischer Rahmen ist.
//
// Steht die Schleuder - kein Rahmen mit Vorrat, kein Glas, Cargo voll -,
// dann steht sie, bis ein Spieler erneut ankurbelt. Ein Nachlegen von Glaesern
// oder das Entnehmen von Honig startet nichts von selbst; das ist Absicht, weil
// der Anstoss dem Spieler gehoert (Annahme A3) und ein Cargo-Haken die
// Schleuder auch ohne jedes Ankurbeln anlaufen liesse.
//
// Persistenz: elapsed/duration des laufenden Jobs speichert der Job-Block der
// Basis. Nach dem Laden startet AfterStoreLoad der Basis den Job-Timer bei
// aktiven Jobs, der naechste Abschluss laeuft durch die Ueberschreibung unten
// - die Fortsetzung ueberlebt den Neustart ohne eigenen Speicherblock. Auch
// der Spieler des Anstosses bleibt: der Job-Block der Basis schreibt die
// Kennung je Job, und die Ueberschreibung liest sie aus dem Job, bevor sie
// ihn weiterreicht (siehe m_ChefZ_PendingActorId).
//
// KEIN ChefZ_HasHeat: Schleudern ist Mechanik, kein Feuer. Kein Prozess dieser
// Station setzt requiresHeat, die Basisantwort "nein" bleibt richtig.
//
// Die uebrigen Klassen der Imkerei liegen in ChefZ_Farming; die Begruendung
// fuer die Aufteilung steht an der Configklasse.
//
// Diese Station fasst Vanillas Kochkette an keiner Stelle an (11 E6).
//
// ### 30.10.2026 - NUR NOCH SLOTS, ANKURBELN PER AKTION (Spieltest) ###
// Das Cargo ist entfallen. Fuenf Plaetze nehmen NUR entdeckelte Rahmen, zehn
// Plaetze leere Glaeser. "Honig schleudern" (ChefZ_ActionProcessAtStation,
// PROCESS_SPIN_HONEY) startet die Schleuder; vorher lief sie beim Einlegen von
// selbst an, belegte damit ihren einzigen Arbeitsplatz und die Aktion erschien
// nie. Je Abschluss wird EIN leeres Glas in SEINEM Platz zum Honigglas und ein
// Rahmen verliert eine Einheit; der Core-Applicator legt Ergebnisse nur ins
// Cargo und ist hier deshalb nicht im Spiel (ChefZ_CompleteJob unten). Die
// Ereignisse (XP, Fortschritt) gehen weiter ueber ChefZ_ProcessRunner.
// Was oben ueber Cargo, Kapazitaet 15 und Selbststart beim Einlegen steht, ist
// damit Geschichte; Schwelle und Reserve des Rahmens gelten unveraendert.
//
// Layer: 4_World.
//==============================================================================

class ChefZ_HoneyExtractor extends ChefZ_ProcessingStation_Base
{
    //! Der Prozess, den die Station sich selbst nachstartet.
    static const string CHEFZ_SPIN_PROCESS      = "PROCESS_SPIN_HONEY";
    //! Wozu ein leergeschleuderter Rahmen wird.
    static const string CHEFZ_FRAME_EMPTY_CLASS = "ChefZ_HoneycombFrameEmpty";
    //! Unterhalb dieser Menge gilt ein Rahmen als leergeschleudert. Muss mit
    //! amount.min in TR_SpinHoney uebereinstimmen (siehe Dateikopf).
    static const float  CHEFZ_FRAME_SPENT_BELOW  = 2.0;
    //! Vanillas Honigglas - entsteht im Glasplatz aus dem leeren Glas.
    static const string CHEFZ_HONEY_CLASS       = "Honey";
    //! Das leere Glas aus ChefZ_Cooking, als NAME (kein requiredAddons auf
    //! ChefZ_Cooking, siehe config.cpp).
    static const string CHEFZ_JAR_CLASS         = "ChefZ_EmptyJar";
    //! Praefixe der Slotnamen (config.cpp, CfgSlots).
    static const string CHEFZ_FRAME_SLOT_PREFIX = "ChefZ_ExtractorFrame";
    static const string CHEFZ_JAR_SLOT_PREFIX   = "ChefZ_ExtractorJar";

    //! Der Spieler, der angekurbelt hat; der Folgejob traegt seine Kennung.
    protected int m_ChefZ_PendingActorId;
    //! Wahr, solange ein Platz an Ort und Stelle ersetzt wird (Rahmen ->
    //! Leerrahmen, Glas -> Honig): nur dann nimmt ein Platz auch das an, was
    //! ein Spieler nicht hineinlegen darf.
    protected bool m_ChefZ_Replacing;

    void ChefZ_HoneyExtractor()
    {
        m_ChefZ_PendingActorId = 0;
        m_ChefZ_Replacing = false;
    }

    //! Rahmenplaetze nehmen von aussen nur entdeckelte Rahmen; Glasplaetze nur
    //! leere Glaeser (und Honig, den die Schleuder selbst erzeugt oder ein
    //! Spieler zuruecklegt). Volle und leere Rahmen bleiben draussen.
    override bool CanReceiveAttachment(EntityAI attachment, int slotId)
    {
        if (!super.CanReceiveAttachment(attachment, slotId))
            return false;
        if (!attachment)
            return false;
        if (m_ChefZ_Replacing)
            return true;

        string slotName = InventorySlots.GetSlotName(slotId);
        if (slotName.IndexOf(CHEFZ_FRAME_SLOT_PREFIX) == 0)
            return ChefZ_HoneycombFrameUncapped.Cast(attachment) != null;
        if (slotName.IndexOf(CHEFZ_JAR_SLOT_PREFIX) == 0)
            return attachment.IsKindOf(CHEFZ_JAR_CLASS) || attachment.IsKindOf(CHEFZ_HONEY_CLASS);
        return true;
    }

    /**
     * Ein Glas fertig: Glas -> Honig im eigenen Platz, Rahmen eine Einheit
     * weniger, leere Rahmen tauschen, naechstes Glas anstossen.
     *
     * Ersetzt den Abschluss der Basis, weil deren Applicator Ergebnisse ins
     * Cargo legt und die Schleuder keines mehr hat. Neu gebunden wird trotzdem
     * (11 §5): passt beim Abschluss kein Transform mehr, wird abgebrochen,
     * ohne etwas zu verbrauchen.
     */
    override bool ChefZ_CompleteJob(int slotIndex)
    {
        if (!g_Game || !g_Game.IsServer())
            return false;
        if (slotIndex < 0 || slotIndex >= m_ChefZ_Jobs.Count())
            return false;

        ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(slotIndex);
        if (!job.IsActive())
            return false;

        ChefZ_ProcessingManager mgr = ChefZ_ProcessingManager.Get();
        if (!ResolveJob(job, mgr))
        {
            ChefZ_CancelJob(slotIndex, "transform_gone");
            return false;
        }
        ChefZ_CompiledTransform tr = mgr.GetTransform(job.transformSym);
        if (!tr)
        {
            ChefZ_CancelJob(slotIndex, "transform_gone");
            return false;
        }

        int actorId = job.actorIdentityId;

        ChefZ_ProcessContext ctx = new ChefZ_ProcessContext();
        ChefZ_BuildContext(null, actorId, ctx);
        ChefZ_FactSnapshot snapshot;
        array<ItemBase> entities;
        ChefZ_FactCollector.CollectFromStation(this, snapshot, entities);
        snapshot.SortStable();
        ChefZ_TransformMatch match;
        if (!mgr.FindTransform(job.processSym, ctx, snapshot, null, match))
        {
            ChefZ_CancelJob(slotIndex, "input_lost");
            return false;
        }

        ItemBase frame = ChefZ_FindFrameWithHoney();
        ItemBase jar = ChefZ_FindEmptyJar();
        if (!frame || !jar)
        {
            ChefZ_CancelJob(slotIndex, "input_lost");
            return false;
        }

        ItemBase honey = ChefZ_FillOneJar(jar);
        if (!honey)
        {
            ChefZ_CancelJob(slotIndex, "output_blocked");
            return false;
        }
        frame.AddQuantity(-1.0);

        job.Clear();
        SyncFromJobs();
        if (ChefZ_ActiveJobCount() == 0)
            StopJobTimer();

        array<ItemBase> created = new array<ItemBase>();
        created.Insert(honey);
        ChefZ_ProcessRunner.RaiseProcessed(this, tr, match, entities, created, actorId);

        ChefZ_RetireSpentFrames();

        m_ChefZ_PendingActorId = actorId;
        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ChefZ_SpinNextJar, 0, false);
        return true;
    }

    //! Startet den naechsten Job, solange ein Rahmen mit Vorrat und ein leeres
    //! Glas eingesetzt sind. Scheitert der Start, steht die Schleuder, bis ein
    //! Spieler erneut "Honig schleudern" waehlt.
    void ChefZ_SpinNextJar()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        ChefZ_Sym process = ChefZ_SymbolTable.Lookup(CHEFZ_SPIN_PROCESS);
        if (!ChefZ_SymbolTable.IsValid(process))
            return;

        string err;
        if (!ChefZ_BeginJob(process, null, m_ChefZ_PendingActorId, err))
        {
            if (ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
            {
                ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS, "Schleuder \"" + GetType() + "\" steht: " + err);
            }
        }
    }

    //! Ein eingesetzter entdeckelter Rahmen mit mindestens der Schwelle.
    protected ItemBase ChefZ_FindFrameWithHoney()
    {
        GameInventory inventory = GetInventory();
        if (!inventory)
            return null;
        int n = inventory.AttachmentCount();
        for (int a = 0; a < n; a++)
        {
            ChefZ_HoneycombFrameUncapped frame = ChefZ_HoneycombFrameUncapped.Cast(inventory.GetAttachmentFromIndex(a));
            if (frame && frame.GetQuantity() >= CHEFZ_FRAME_SPENT_BELOW)
                return frame;
        }
        return null;
    }

    //! Ein eingesetztes leeres Glas.
    protected ItemBase ChefZ_FindEmptyJar()
    {
        GameInventory inventory = GetInventory();
        if (!inventory)
            return null;
        int n = inventory.AttachmentCount();
        for (int a = 0; a < n; a++)
        {
            ItemBase item = ItemBase.Cast(inventory.GetAttachmentFromIndex(a));
            if (item && item.IsKindOf(CHEFZ_JAR_CLASS))
                return item;
        }
        return null;
    }

    //! Leeres Glas -> Honigglas im selben Platz. Gibt das Honigglas zurueck
    //! oder null, wenn die Engine nichts ersetzt hat.
    protected ItemBase ChefZ_FillOneJar(notnull ItemBase jar)
    {
        GameInventory jarInventory = jar.GetInventory();
        if (!jarInventory)
            return null;

        InventoryLocation loc = new InventoryLocation();
        if (!jarInventory.GetCurrentInventoryLocation(loc))
            return null;
        int slotId = loc.GetSlot();

        TurnItemIntoItemLambda lambda = new TurnItemIntoItemLambda(jar, CHEFZ_HONEY_CLASS, null);
        lambda.SetTransferParams(false, false, true, true);
        m_ChefZ_Replacing = true;
        jarInventory.ReplaceItemWithNew(InventoryMode.SERVER, lambda);
        m_ChefZ_Replacing = false;

        ItemBase honey = ItemBase.Cast(GetInventory().FindAttachment(slotId));
        if (honey && honey.IsKindOf(CHEFZ_HONEY_CLASS))
            return honey;
        return null;
    }

    //! Rahmen unterhalb der Schwelle werden in ihrem Platz zum Leerrahmen.
    //! Rueckwaerts, weil der Tausch die Liste veraendert.
    protected void ChefZ_RetireSpentFrames()
    {
        GameInventory inventory = GetInventory();
        if (!inventory)
            return;
        int a = inventory.AttachmentCount() - 1;
        while (a >= 0)
        {
            ChefZ_RetireOneFrame(ChefZ_HoneycombFrameUncapped.Cast(inventory.GetAttachmentFromIndex(a)));
            a = a - 1;
        }
    }

    //! Ein leergeschleudertes Raehmchen gegen ein leeres tauschen (an Ort und
    //! Stelle, deshalb fuehrt ChefZ_HoneycombFrameEmpty die Extractor-Slots).
    protected void ChefZ_RetireOneFrame(ChefZ_HoneycombFrameUncapped frame)
    {
        if (!frame)
            return;
        if (frame.GetQuantity() >= CHEFZ_FRAME_SPENT_BELOW)
            return;
        GameInventory frameInventory = frame.GetInventory();
        if (!frameInventory)
            return;

        TurnItemIntoItemLambda lambda = new TurnItemIntoItemLambda(frame, CHEFZ_FRAME_EMPTY_CLASS, null);
        lambda.SetTransferParams(false, false, true, true);
        m_ChefZ_Replacing = true;
        frameInventory.ReplaceItemWithNew(InventoryMode.SERVER, lambda);
        m_ChefZ_Replacing = false;
    }
}
