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
// Der Vorrat eines entdeckelten Rahmens ist seine varQuantity: drei Glaeser
// PLUS EINE RESERVE-EINHEIT (4..1, Klassendefault aus ChefZ_Farming). Der
// Applicator zieht je Glas eine Einheit ab. Die Reserve ist kein Bonus,
// sondern eine Notwendigkeit des Core: ChefZ_SlotEvaluator.PlanAmountDraw
// setzt destroyWhole, sobald ein Abzug die letzte Einheit traefe
// (1_Core/ChefZ/ChefZ_SlotEvaluator.c:367-373), und der Applicator LOESCHT
// das Item dann statt es auf null zu setzen (Cooking/ChefZ_Applicator.c:
// 1250-1253). Duerfte das dritte Glas die letzte Einheit ziehen, gaebe es
// nach dem dritten Glas keinen Rahmen mehr, den man zum Leerrahmen machen
// koennte. Deshalb verlangt TR_SpinHoney mindestens zwei Einheiten: die
// Zuege geschehen bei 4, 3 und 2, die Einheit 1 bleibt stehen, und ein
// Rahmen unterhalb der Schwelle wird hier nach dem Abschluss zum Leerrahmen
// ersetzt - nicht in einem Mengenhaken des Rahmens, denn waehrend des
// Verbrauchs haelt der Applicator noch Handles auf die Entities.
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
// Layer: 4_World.
//==============================================================================

class ChefZ_HoneyExtractor extends ChefZ_ProcessingStation_Base
{
    //! Fuenf Rahmen und fuenfzehn Glaeser (Auftrag 8-10).
    static const int    CHEFZ_FRAME_CAPACITY    = 5;
    static const int    CHEFZ_JAR_CAPACITY      = 15;
    //! Der Prozess, den die Station sich selbst nachstartet.
    static const string CHEFZ_SPIN_PROCESS      = "PROCESS_SPIN_HONEY";
    //! Wozu ein leergeschleuderter Rahmen wird.
    static const string CHEFZ_FRAME_EMPTY_CLASS = "ChefZ_HoneycombFrameEmpty";
    //! Unterhalb dieser Menge gilt ein Rahmen als leergeschleudert. Zwei und
    //! nicht null: TR_SpinHoney verlangt mindestens zwei Einheiten, damit der
    //! Core nie die letzte zieht und den Rahmen loescht (siehe Dateikopf).
    //! Nach dem dritten Glas steht der Rahmen auf 1.
    static const float  CHEFZ_FRAME_SPENT_BELOW  = 2.0;
    //! Vanillas Honigglas - das Ergebnis darf liegen bleiben und zurueck.
    static const string CHEFZ_HONEY_CLASS       = "Honey";
    //! Das leere Glas aus ChefZ_Cooking. Als NAME und nicht als Cast: dieses
    //! Modul fuehrt ChefZ_Cooking nicht in requiredAddons (Ladezyklus, siehe
    //! config.cpp), und ein Klassenbezug im Skript waere dieselbe Abhaengigkeit
    //! durch die Hintertuer. IsKindOf loest den Namen zur Laufzeit auf.
    static const string CHEFZ_JAR_CLASS         = "ChefZ_EmptyJar";

    //! Der Spieler, der angekurbelt hat. Der Folgejob traegt seine Kennung,
    //! damit XP je Glas an ihn geht - und erst nach dem Abschluss (Regel 7).
    //! Kein eigener Speicherblock noetig: die Kennung kommt bei jedem
    //! Abschluss frisch aus dem abgeschlossenen Job, und den schreibt und
    //! liest der Job-Block der Basis (ChefZ_ProcessingStation_Base.c:1102,
    //! 1178) - auch ueber einen Neustart hinweg.
    //! 0 heisst "niemand beteiligt", wie ueberall im Core (ChefZ_ProcessJob.
    //! Clear, ChefZ_ProcessContext.c:337; ChefZ_BuildContext, Basis Z.429).
    protected int m_ChefZ_PendingActorId;
    //! Wahr, solange ChefZ_RetireSpentFrames einen Rahmen in Ort und Stelle
    //! ersetzt: nur dann darf ein Leerrahmen ins Cargo. Von aussen nimmt die
    //! Schleuder ausschliesslich entdeckelte Rahmen an (Auftrag 8-10).
    protected bool m_ChefZ_ReplacingFrame;

    void ChefZ_HoneyExtractor()
    {
        m_ChefZ_PendingActorId = 0;
        m_ChefZ_ReplacingFrame = false;
    }

    //! Entdeckelte Rahmen, leere Glaeser und Honig hinein - Rahmen wie
    //! Glaeser nur bis zur Kapazitaet. Ein volles Raehmchen hat hier keinen
    //! Zweck (Auftrag 8: fuenf Plaetze fuer entdeckelte Rahmen) und bleibt
    //! draussen. Der Leerrahmen dagegen muss erlaubt sein: die Ersetzung
    //! eines leergeschleuderten Rahmens legt ihn in dessen Cargo-Zelle an, und
    //! er zaehlt mit, bis er entnommen ist - er belegt den Platz eines Rahmens.
    //! Beleg: EntityAI.CanReceiveItemIntoCargo, scripts - 1.29/3_Game/DayZ/
    //! Entities/EntityAI.c:1550-1559; Ueberschreibung wie Barrel_ColorBase.c:512.
    override bool CanReceiveItemIntoCargo(EntityAI item)
    {
        if (!super.CanReceiveItemIntoCargo(item))
            return false;
        if (!item)
            return false;

        // Von aussen nur entdeckelte Rahmen; der Leerrahmen, den die Schleuder
        // selbst zuruecklaesst, entsteht waehrend der Ersetzung und darf bleiben.
        if (ChefZ_HoneycombFrame_Base.Cast(item))
        {
            if (m_ChefZ_ReplacingFrame)
                return true;
            if (!ChefZ_HoneycombFrameUncapped.Cast(item))
                return false;
            return ChefZ_CountFrames() < CHEFZ_FRAME_CAPACITY;
        }

        // Beleg: Object.IsKindOf, scripts - 1.29/3_Game/DayZ/Entities/Object.c:517;
        // Aufrufstelle Recipes/LoadMagazine.c:64.
        if (item.IsKindOf(CHEFZ_JAR_CLASS))
            return ChefZ_CountJars() < CHEFZ_JAR_CAPACITY;

        if (item.IsKindOf(CHEFZ_HONEY_CLASS))
            return true;

        return false;
    }

    //! Der Abschluss eines Glases stoesst das naechste an.
    //!
    //! Die Spielerkennung wird VOR super gesichert: die Basis loescht den Job
    //! nach dem Lauf (job.Clear()). Der Neustart geht NICHT direkt aus dem
    //! Timer-Rueckruf heraus, sondern in den naechsten Frame: super hat den
    //! Job-Timer gerade angehalten, und ein Timer.Run aus dem eigenen Rueckruf
    //! heraus ist kein belegter Pfad. CallLater aus derselben Warteschlange
    //! ist es (GeyserArea.c:49, FireplaceBase.c:1767).
    override bool ChefZ_CompleteJob(int slotIndex)
    {
        int actorId = 0;
        if (slotIndex >= 0 && slotIndex < m_ChefZ_Jobs.Count())
        {
            ChefZ_ProcessJob job = m_ChefZ_Jobs.Get(slotIndex);
            actorId = job.actorIdentityId;
        }

        bool ok = super.ChefZ_CompleteJob(slotIndex);
        if (!ok)
            return false;

        ChefZ_RetireSpentFrames();

        m_ChefZ_PendingActorId = actorId;
        if (g_Game)
            g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ChefZ_SpinNextJar, 0, false);

        return true;
    }

    //! Startet den naechsten Job, solange Rahmen mit Vorrat und Glaeser
    //! liegen (Annahme A3). Oeffentlich, weil CallLater ihn ruft. Scheitert
    //! der Start, steht die Schleuder - kein Vorrat, kein Glas oder das Cargo
    //! ist voll - und wartet auf das naechste Ankurbeln. Dasselbe gilt fuer
    //! einen Job, der in der Basis ohne Verbrauch endet (RUN_FAILED bei
    //! vollem Cargo): ChefZ_CompleteJob kehrt dann vor dem Nachstart zurueck.
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

    //! Rahmen unterhalb der Reserve-Schwelle werden in ihrer Cargo-Zelle zum
    //! Leerrahmen. Rueckwaerts, weil die Ersetzung den Cargo-Index verschiebt.
    //! Keine Variablen uebertragen, sonst wandert die Restmenge mit; Health ja.
    //! Beleg: TurnItemIntoItemLambda + SetTransferParams, scripts - 1.29/
    //! 4_World/DayZ/Static/MiscGameplayFunctions.c:1-14; spielerloser Pfad
    //! Entities/Core/Inherited/InventoryItem.c:274-276; Cargo-Zweig der
    //! Ersetzung ReplaceItemWithNewLambdaBase.c:157-158.
    protected void ChefZ_RetireSpentFrames()
    {
        GameInventory inventory = GetInventory();
        if (!inventory)
            return;
        CargoBase cargo = inventory.GetCargo();
        if (!cargo)
            return;

        int i = cargo.GetItemCount() - 1;
        while (i >= 0)
        {
            ChefZ_HoneycombFrameUncapped frame = ChefZ_HoneycombFrameUncapped.Cast(cargo.GetItem(i));
            if (frame && frame.GetQuantity() < CHEFZ_FRAME_SPENT_BELOW)
            {
                // Ein Rahmen ohne Inventar steckt bereits im Loeschen; ihn zu
                // ersetzen waere ein VM-Fehler mitten im Timer-Rueckruf, und
                // der Folgejob kaeme nie. Er wird uebersprungen.
                GameInventory frameInventory = frame.GetInventory();
                if (frameInventory)
                {
                    TurnItemIntoItemLambda lambda = new TurnItemIntoItemLambda(frame, CHEFZ_FRAME_EMPTY_CLASS, null);
                    lambda.SetTransferParams(false, false, true, true);
                    m_ChefZ_ReplacingFrame = true;
                    frameInventory.ReplaceItemWithNew(InventoryMode.SERVER, lambda);
                    m_ChefZ_ReplacingFrame = false;
                }
            }
            i = i - 1;
        }
    }

    //! Rahmen im Cargo - entdeckelte und leergeschleuderte gleichermassen.
    protected int ChefZ_CountFrames()
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
            if (ChefZ_HoneycombFrame_Base.Cast(cargo.GetItem(i)))
                count = count + 1;
        }
        return count;
    }

    //! Leere Glaeser im Cargo.
    protected int ChefZ_CountJars()
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
            EntityAI entry = cargo.GetItem(i);
            if (entry && entry.IsKindOf(CHEFZ_JAR_CLASS))
                count = count + 1;
        }
        return count;
    }
}
