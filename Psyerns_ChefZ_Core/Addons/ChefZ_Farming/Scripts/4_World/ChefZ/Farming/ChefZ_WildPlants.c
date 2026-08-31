//==============================================================================
// ChefZ_WildPlants - die vier Wildpflanzen des Slice "wildplants"
// (Spec: Psyerns_ChefZ_Docs/ChefZ_Wildwuchs_Spawn_Plan.md, freigegeben
// 31.08.2026; loest den Gate-2-Befund G2-B9 "keine Loot-Verteilung").
//
// Andockregel woertlich aus dem Kopf von ChefZ_Core/Scripts/4_World/ChefZ/
// Processing/ChefZ_ProcessingStation_Base.c:
//
//     config.cpp   class ChefZ_WildCorn : ChefZ_WildPlant_Base { ... };
//     JSON/Rang 2  { "kind":"station", "records":[{ "id":"ChefZ_WildCorn" }] }
//     Skript       class ChefZ_WildCorn extends ChefZ_WildPlant_Base {}
//
// ---------------------------------------------------------------------------
// WAS EINE WILDPFLANZE IST
// ---------------------------------------------------------------------------
// Ein STEHENDES Weltobjekt, das die CE spielerzentriert verteilt (wie Vanillas
// Pilze, Spec Kap. 1), das man NICHT aufhebt, sondern ERNTET, und das nach der
// Ernte verschwindet. Der Ertrag faellt daneben zu Boden; die CE stellt
// anderswo eine neue hin, weil das nominal des Events wieder unterschritten
// ist.
//
// Sie ist eine Mini-Station nach dem Muster des Bienenstocks (Ansatz A,
// Spec Kap. 3): die Ernte ist der Stationsvorgang PROCESS_HARVEST_WILD, und
// was dabei geschieht, steht im Haken ChefZ_OnStationActionFinished - genau
// wie der Bienenstich in ChefZ_Apiary.c. KEIN Core-Code, KEINE eigene Action,
// KEIN modded class (Regel 4: neuer Content erfordert nie eine
// Core-Codeaenderung).
//
// ---------------------------------------------------------------------------
// WARUM PROCESS_HARVEST_WILD KEINEN TRANSFORM HAT
// ---------------------------------------------------------------------------
// Spec Kap. 3 sieht "je Pflanze ein Transform, dessen Output der garantierte
// eine Ertrag ist" vor. Das ist mit dem Core in seinem heutigen Stand NICHT
// baubar, und zwar aus drei unabhaengigen Gruenden - jeder einzelne genuegt:
//
//   1. ChefZ_TransformDef.Validate() (ChefZ_Core/Scripts/1_Core/ChefZ/
//      ChefZ_TransformDef.c:173-178) WEIST einen Transform ohne "inputs"
//      ausdruecklich AB: "Er haette keine Bedingung und wuerde damit auf jeden
//      Stationsinhalt passen, auch auf einen leeren."
//   2. Eine Wildpflanze hat kein Cargo (sie ist kein Behaelter, sie ist eine
//      Pflanze). ChefZ_FactCollector.CollectFromCargo() kehrt bei einem
//      Behaelter ohne Cargo mit leerem Schnappschuss zurueck
//      (ChefZ_FactCollector.c:191-196) - ein Eingang koennte also nie binden.
//   3. Der Applicator legt Ergebnisse "ausschliesslich in den CARGO EINES
//      GEFAESSES" ab (so woertlich ChefZ_ProcessRunner.c:165-166). Wohin der
//      Kolben faellt, koennte ein Transform hier gar nicht sagen.
//
// Der Vorgang traegt deshalb - wie PROCESS_HARVEST_HIVE, aus derselben Sorte
// Grund - KEINEN Transform. ChefZ_ActionProcessAtStation.IsProcessUsable()
// ueberspringt die Transformpruefung, wenn zu einem Prozess kein Transform
// bekannt ist (HasAnyTransformFor -> true zurueck), RunImmediate meldet dann
// NO_MATCH, und NotifyStation ruft den Haken trotzdem. NO_MATCH ist hier der
// GEWOLLTE Ausgang und kein Fehler.
//
// Die Folge fuer diese Datei: der ERTRAG steht im Skript und nicht in einem
// JSON. Er steht dort an genau einer Stelle je Pflanze (ChefZ_YieldClass), und
// die Ausbeutetabelle der Spec ist Zeile fuer Zeile in den drei virtuellen
// Zahlen darunter wiederzufinden.
//
// ---------------------------------------------------------------------------
// NICHT AUFNEHMBAR - was das wirklich bewirkt
// ---------------------------------------------------------------------------
// canBeDigged = 0 in der config.cpp verhindert das NICHT: der Schluessel
// betreibt Vanillas vergrabene Verstecke und hat mit dem Aufheben nichts zu
// tun. Was das Aufheben unterbindet, ist IsTakeable() - ItemBase setzt
// m_IsTakeable im Konstruktor auf true (scripts - 1.29, ItemBase.c:229) und
// gibt es in IsTakeable() zurueck (ItemBase.c:4398-4401);
// ActionTakeItem.ActionCondition fragt genau diesen Haken ab
// (ActionTakeItem.c:34: "if (tgt_item && !tgt_item.IsTakeable()) return
// false;").
//
// Uebernommen ist deshalb der VOLLSTAENDIGE Satz, mit dem Vanilla ein
// unaufnehmbares Weltobjekt beschreibt - GardenPlot.c:113-131:
// IsTakeable, CanPutInCargo, CanRemoveFromCargo, CanPutIntoHands, alle false.
// Dazu die Gegenprobe im Aktionsmenue (BaseBuildingBase.c:1245-1251:
// RemoveAction(ActionTakeItem) / RemoveAction(ActionTakeItemToHands)) - siehe
// SetActions unten.
//
// Layer: 4_World.
//==============================================================================

/**
 * Die gemeinsame Basis der vier Wildpflanzen.
 *
 * Sie ist scope = 0 und selbst nie in der Welt. Was sie beschreibt, ist der
 * VORGANG - Ernte, Wurf, Verschwinden -, nicht die Pflanze; die vier Erbinnen
 * tragen nur noch Zahlen und einen Klassennamen.
 */
class ChefZ_WildPlant_Base extends ChefZ_ProcessingStation_Base
{
    //! Der EINE Stationsvorgang dieser Familie. Der Haken feuert fuer JEDEN
    //! Prozess der Station; die Pruefung bleibt, damit ein spaeter ergaenzter
    //! Vorgang nicht ungewollt die Pflanze abraeumt. Dieselbe Vorsorge wie
    //! CHEFZ_STING_PROCESS am Bienenstock.
    static const string CHEFZ_HARVEST_PROCESS = "PROCESS_HARVEST_WILD";

    //! Wie weit ein Begleiter von der Mutterpflanze steht (Spec Kap. 3:
    //! "im Umkreis 1-2 m"). Nah genug, dass es EINE Gruppe ist, weit genug,
    //! dass zwei Maispflanzen nicht ineinanderstecken.
    static const float CHEFZ_COMPANION_MIN_M = 1.0;
    static const float CHEFZ_COMPANION_MAX_M = 2.0;

    //! Wie weit ein ZUSAETZLICHER Ertrag neben dem ersten liegt. Deutlich
    //! enger als ein Begleiter: drei Kolben sollen als EIN Fund erkennbar
    //! sein und nicht als drei Fundstellen.
    static const float CHEFZ_YIELD_SPREAD_MIN_M = 0.2;
    static const float CHEFZ_YIELD_SPREAD_MAX_M = 0.5;

    /**
     * Die Rekursionswache der Gruppenbildung (Spec Kap. 3, Fallback:
     * "Begleiter duerfen selbst KEINE weiteren Begleiter wuerfeln").
     *
     * -------------------------------------------------------------------------
     * SIE IST SEIT DEM 31.08.2026 EIN ZWEITER RIEGEL, KEINE NOTWENDIGKEIT MEHR
     * -------------------------------------------------------------------------
     * Bis zum Conflict-Scout-Befund F3 lief die Gruppenbildung SYNCHRON aus
     * EEOnCECreate - also CreateObjectEx mitten im Erzeugungsdurchlauf der CE.
     * Dort war das Flag die einzige Absicherung, und zwar STATISCH und nicht am
     * Objekt: gaebe es einen synchronen Wiedereintritt, existierte das neue
     * Objekt noch gar nicht, an dem ein Instanzflag stehen koennte.
     *
     * Seit F3 ist der Spawn um einen Frame verschoben (siehe
     * ChefZ_SpawnCompanionsLater). Damit laeuft die Schleife AUSSERHALB des
     * CE-Durchlaufs, und der synchrone Wiedereintritt, gegen den das Flag
     * gebaut war, kann strukturell nicht mehr auftreten.
     *
     * DAS FLAG BLEIBT TROTZDEM. Zwei Gruende, und beide zaehlen:
     *
     *   1. Es kostet nichts. Ein bool und zwei Zuweisungen je Gruppe.
     *   2. Der Fehler, den es abfaengt, ist der teuerste denkbare dieses
     *      Slice: eine Maispflanze, die sich selbst vermehrt, fuellt den Server
     *      und steht in keinem Log. Ein Riegel gegen so etwas wird nicht
     *      abgebaut, weil er "eigentlich" nicht mehr gebraucht wird.
     *
     * Die dritte, unabhaengige Absicherung liegt in den Flags: EEOnCECreate ist
     * "called when entity is being created as new by CE/ Debug" (scripts -
     * 1.29, EntityAI.c:1385-1388), und der volle Aufbau haengt am Flag
     * ECE_SETUP ("process full entity setup (when creating NEW entity)",
     * CentralEconomy.c:8). ECE_PLACE_ON_SURFACE ist 1060 und damit
     * ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH|ECE_TRACE (CentralEconomy.c:37) -
     * ECE_SETUP ist NICHT darin. Ein per Skript gesetzter Begleiter sollte den
     * Haken also ohnehin nie sehen.
     *
     * Drei Riegel fuer einen Fall, der wahrscheinlich nie eintritt: ja. Der
     * Preis dafuer sind vier Zeilen.
     */
    protected static bool s_ChefZ_SpawningCompanions;

    //==========================================================================
    // Was diese Pflanze hergibt - die Ausbeutetabelle der Spec (Kap. 5)
    //==========================================================================

    /**
     * Die Ertragsklasse. Leer heisst "diese Pflanze gibt nichts her" - dann
     * bleibt sie beim Ernten stehen, statt sich fuer nichts aufzuloesen.
     *
     * Als virtuelle Methode und nicht als Configfeld: der Klassenname steht
     * damit genau einmal je Pflanze im Projekt, und ein Tippfehler faellt
     * beim ersten Ernten mit einer Logzeile auf - nicht still.
     */
    string ChefZ_YieldClass()
    {
        return "";
    }

    //! Prozentsatz fuer EINEN Zusatzertrag. Vorgabe 0: eine Basis wirft nicht.
    int ChefZ_BonusOnePct()
    {
        return 0;
    }

    //! Prozentsatz fuer ZWEI Zusatzertraege. Vorgabe 0, siehe oben.
    int ChefZ_BonusTwoPct()
    {
        return 0;
    }

    //! Wie viele Begleiter beim Erscheinen daneben aufgestellt werden.
    //! Vorgabe 0 - nur der Mais waechst in Gruppen (Spec Kap. 3).
    int ChefZ_RollCompanions()
    {
        return 0;
    }

    /**
     * Der Wurf. ZWEI BAENDER auf einem Wurf 0..99, nicht zwei Wuerfe:
     *
     *   roll <  twoPct            -> +2
     *   roll <  twoPct + onePct   -> +1
     *   sonst                     ->  0
     *
     * Beim Mais (twoPct 5, onePct 25) sind das genau die Zahlen der
     * Ausbeutetabelle: 5 Prozent +2, 25 Prozent +1, 70 Prozent nichts. Zwei
     * getrennte Wuerfe haetten stattdessen 5 Prozent UND 25 Prozent
     * uebereinandergelegt und im Mittel mehr ausgeschuettet, als die Tabelle
     * sagt.
     *
     * Math.RandomIntInclusive(0, 99) sind genau hundert gleich wahrscheinliche
     * Werte (scripts - 1.29, EnMath.c:54; der Kommentar dort fuehrt
     * RandomIntInclusive(0,2) als "0, 1, 2").
     *
     * SERVERSEITIG gerufen, immer - der Aufrufer ist ChefZ_Harvest().
     */
    int ChefZ_RollBonus()
    {
        int twoPct = ChefZ_BonusTwoPct();
        int onePct = ChefZ_BonusOnePct();

        int roll = Math.RandomIntInclusive(0, 99);
        if (roll < twoPct)
            return 2;
        if (roll < twoPct + onePct)
            return 1;
        return 0;
    }

    //==========================================================================
    // Nicht aufnehmbar, nicht umstellbar
    //==========================================================================

    //! Vanillas Satz fuer ein unaufnehmbares Weltobjekt - GardenPlot.c:113-131.
    //! Die vollstaendige Begruendung samt Fundstellen steht im Dateikopf.
    override bool IsTakeable()
    {
        return false;
    }

    override bool CanPutInCargo(EntityAI parent)
    {
        return false;
    }

    override bool CanRemoveFromCargo(EntityAI parent)
    {
        return false;
    }

    override bool CanPutIntoHands(EntityAI parent)
    {
        return false;
    }

    /**
     * Eine Wildpflanze wird NICHT aufgestellt.
     *
     * ChefZ_ProcessingStation_Base.IsDeployable() antwortet "ja", weil eine
     * Station eine Sache ist, die man hinstellt (dort, SetActions). Das gilt
     * fuer einen Fleischwolf und fuer einen Bienenstock; fuer etwas, das aus
     * dem Boden waechst, gilt es nicht. Hologram.c:252 fragt genau diesen
     * Haken, bevor es ein Platzierungshologramm anwirft.
     */
    override bool IsDeployable()
    {
        return false;
    }

    /**
     * Die Aktionen der Pflanze: NUR die Stationsaktion.
     *
     * -------------------------------------------------------------------------
     * WARUM super TROTZDEM GERUFEN WIRD
     * -------------------------------------------------------------------------
     * Der Wunsch waere, die Basis zu ueberspringen und nur
     * ItemBase.SetActions() zu nehmen. Enforce kann das nicht: "super" ruft
     * immer die DIREKTE Elternklasse, eine Ebene laesst sich nicht
     * ueberspringen. Ohne super gaebe es ausserdem keine
     * ChefZ_ActionProcessAtStation mehr - und damit keine Ernte.
     *
     * Deshalb Vanillas eigener Weg: super rufen und die vier unerwuenschten
     * Aktionen wieder herausnehmen. Genau so macht es
     * BaseBuildingBase.SetActions() (scripts - 1.29, BaseBuildingBase.c:
     * 1245-1251) fuer ActionTakeItem und ActionTakeItemToHands, und Pot.c:
     * 133-137 fuer ActionDrink. RemoveAction ist ItemBase.c:366-377.
     *
     * -------------------------------------------------------------------------
     * DIE VIER, UND WARUM JEDE EINZELNE WEG MUSS
     * -------------------------------------------------------------------------
     *   ActionTakeItem         } aus ItemBase.SetActions() (ItemBase.c:317-324).
     *   ActionTakeItemToHands  } IsTakeable() false sperrt sie bereits
     *                            (ActionTakeItem.c:34) - die Zeile hier nimmt
     *                            sie zusaetzlich aus der Liste, damit kein
     *                            zweiter Weg (etwa ein fremder Mod, der
     *                            IsTakeable ueberschreibt) sie zurueckholt.
     *   ActionTogglePlaceObject} aus ChefZ_ProcessingStation_Base.SetActions().
     *   ActionPlaceObject      } Auftrag: "Spieler sollen sie nicht
     *                            umstellen". IsDeployable() false sperrt sie
     *                            oben, das Aktionsmenue soll sie gar nicht
     *                            erst fuehren.
     *
     * ActionWorldCraft, ActionDropItem und ActionAttachWithSwitch bleiben
     * stehen: sie setzen alle drei ein Item in der Hand oder im Inventar
     * voraus, und dorthin kommt eine Wildpflanze nicht.
     */
    override void SetActions()
    {
        super.SetActions();

        RemoveAction(ActionTakeItem);
        RemoveAction(ActionTakeItemToHands);
        RemoveAction(ActionTogglePlaceObject);
        RemoveAction(ActionPlaceObject);
    }

    //==========================================================================
    // Das Modell (Befund Asset-Tracker, 31.08.2026)
    //==========================================================================

    /**
     * Stellt das Modell auf den Zustand ein, in dem eine Wildpflanze steht.
     *
     * VORGABE: NICHTS. Ein einteiliges Proxy-Modell hat keine Stufen, und ein
     * SetAnimationPhase auf einen Namen, den das Modell nicht kennt, waere
     * eine Zeile, die nur im RPT auftaucht.
     *
     * Ueberschrieben wird sie genau von ChefZ_WildCorn - dessen Modell traegt
     * sechs Wuchsstufen uebereinander. Die vollstaendige Begruendung steht
     * dort.
     */
    void ChefZ_ApplyModelStage()
    {
    }

    /**
     * Beide Seiten. EEInit laeuft auf Server UND Client, und
     * ShowSelection/HideSelection sind LOKAL - sie setzen eine
     * Animationsphase am eigenen Modell (EntityAI.c:3356-3371,
     * SetAnimationPhase) und gehen nicht ueber die Leitung. Wer das nur
     * serverseitig taete, haette einen Server, der die Pflanze richtig sieht,
     * und Spieler, die etwas anderes sehen.
     *
     * super zuerst: die Basis laedt hier ihre Prozessliste.
     */
    override void EEInit()
    {
        super.EEInit();
        ChefZ_ApplyModelStage();
    }

    //==========================================================================
    // Erscheinen - die Gruppe (Spec Kap. 3, Fallback)
    //==========================================================================

    /**
     * Die CE hat diese Pflanze neu in die Welt gesetzt.
     *
     * Vanilla-Signatur und Aufrufanlass: EntityAI.c:1385-1388, "Called when
     * entity is being created as new by CE/ Debug". Vorbild fuer die
     * Ueberschreibung: MushroomBase.c:31-45 (dort wuerfelt der Pilz seine
     * Garstufe). super wird gerufen, anders als bei MushroomBase - der Rumpf
     * der Basis ist leer, aber ein Modul, das spaeter eine Zwischenbasis
     * einzieht, verloere ihn sonst.
     *
     * WARUM DIESER FALLBACK UEBERHAUPT GEBAUT WURDE, steht in
     * README_WildPlants.md unter "Cluster": ein position=player-Event kann
     * nachweislich keine Gruppe setzen.
     */
    override void EEOnCECreate()
    {
        super.EEOnCECreate();
        ChefZ_SpawnCompanionsLater();
    }

    /**
     * Die Gruppe entsteht einen Frame SPAETER (Conflict-Scout F3, 31.08.2026).
     *
     * DER BEFUND: bis dahin lief die Schleife synchron aus EEOnCECreate - also
     * CreateObjectEx MITTEN im Erzeugungsdurchlauf der CE. Was die CE tut,
     * waehrend man ihr neue Objekte in denselben Durchlauf schiebt, ist
     * nirgends zugesagt.
     *
     * DAS VORBILD IST VANILLAS EINZIGES: Wreck_SantasSleigh verschiebt genau
     * das und nur das (scripts - 1.29, 4_World/DayZ/Entities/Building/Wrecks/
     * Wreck_SantasSleigh.c:30-34 ruft SpawnRandomDeerLater, und :47-51 setzt
     * CallLater(SpawnRandomDeers, 0)). Der Schlitten stellt tote Rehe um sich
     * herum - dieselbe Aufgabe, dieselbe Loesung, bis auf die Zwischenmethode,
     * die hier denselben Namen traegt.
     *
     * EINE ABWEICHUNG, benannt: Vanilla nimmt dort CALL_CATEGORY_GAMEPLAY,
     * hier steht CALL_CATEGORY_SYSTEM. Grund ist die Umgebung, nicht der
     * Geschmack - dieses Modul fuehrt seine Serverarbeit durchgehend in
     * SYSTEM (ChefZ_Beehive.ChefZ_StartFillTimer, ChefZ_PackUp, und die Ernte
     * ein paar Zeilen weiter unten). SYSTEM haengt nicht an der
     * Gameplay-Schleife, und ein CE-Spawn ist Serverarbeit, kein Gameplay.
     *
     * OEFFENTLICH ist nur ChefZ_SpawnCompanions selbst noetig - CallLater
     * nimmt hier einen Methodenzeiger, keinen Namen. Die Zwischenmethode
     * bleibt trotzdem, weil sie die Verschiebung BENENNT: ein CallLater
     * mitten in EEOnCECreate saehe aus wie eine Laune.
     */
    protected void ChefZ_SpawnCompanionsLater()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ChefZ_SpawnCompanions, 0, false);
    }

    /**
     * Stellt 0..n Pflanzen derselben Klasse in den Umkreis.
     *
     * OEFFENTLICH und mit Loeschwache, beides seit der Verschiebung um einen
     * Frame (F3): der Aufruf kommt jetzt aus der Aufrufschlange und nicht mehr
     * unmittelbar aus EEOnCECreate. Zwischen dem Einreihen und dem Ausfuehren
     * kann die Mutterpflanze bereits fort sein - die CE raeumt im selben
     * Durchlauf auf, in dem sie setzt. Dieselbe Wache und derselbe Grund wie
     * in ChefZ_Harvest().
     *
     * SERVERSEITIG: eine neue Weltentitaet ist autoritativer Zustand (00 §5).
     *
     * Die Begleiter sind dieselbe Klasse (GetType()) und tragen damit
     * dieselbe types.xml-Zeile - also dieselbe lifetime, dasselbe nominal,
     * dieselbe Zaehlung. Das ist die Zusage aus Spec Kap. 3 ("Begleiter tragen
     * dieselbe lifetime"), und sie kostet hier keine Zeile Code: sie folgt
     * daraus, dass es dieselbe Klasse ist.
     *
     * CreateObjectEx mit ECE_PLACE_ON_SURFACE - derselbe Aufruf, mit dem
     * Vanilla ein gefangenes Tier ablegt (CatchingResultBasic.c:107) und mit
     * dem ChefZ_Beehive.ChefZ_PackUp() seinen Bausatz hinlegt.
     *
     * Entsteht kein Begleiter, passiert nichts weiter: die Mutterpflanze steht
     * ohnehin. Eine Gruppe von einem ist eine Pflanze, und das ist ein
     * gueltiger Fund.
     */
    void ChefZ_SpawnCompanions()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        // Die Mutterpflanze ist zwischen Einreihen und Ausfuehren fort - dann
        // gibt es keine Gruppe, um die herum etwas stehen koennte.
        if (IsSetForDeletion())
            return;

        // Der zweite Riegel. Siehe s_ChefZ_SpawningCompanions.
        if (s_ChefZ_SpawningCompanions)
            return;

        int wanted = ChefZ_RollCompanions();
        if (wanted <= 0)
            return;

        string cls = GetType();
        if (cls == "")
            return;

        vector center = GetPosition();
        int made = 0;

        s_ChefZ_SpawningCompanions = true;
        for (int i = 0; i < wanted; i++)
        {
            vector spot = ChefZ_SpotNear(center, CHEFZ_COMPANION_MIN_M, CHEFZ_COMPANION_MAX_M);
            EntityAI mate = EntityAI.Cast(g_Game.CreateObjectEx(cls, spot, ECE_PLACE_ON_SURFACE));
            if (!mate)
                continue;
            made = made + 1;
        }
        s_ChefZ_SpawningCompanions = false;

        ChefZ_LogPlant("Gruppe: " + made.ToString() + " von " + wanted.ToString() + " Begleiter(n) gesetzt.");
    }

    /**
     * Ein zufaelliger Punkt im Ring [minDist, maxDist] um center, in der
     * Ebene. Die Hoehe bleibt die des Mittelpunkts; die endgueltige Ablage
     * macht ECE_PLACE_ON_SURFACE (ECE_TRACE, "trace under entity when being
     * placed", CentralEconomy.c:9).
     *
     * Math.PI2 = 6.28318530717958 (EnMath.c:13), RandomFloatInclusive
     * EnMath.c:106, Sin/Cos EnMath.c:331/343.
     *
     * Ausgeschrieben ueber Zwischenvariablen und Komponentenzuweisung: eine
     * Vector()-Konstruktion mit vier Rechnungen in den Klammern waere eine
     * Zeile, die niemand mehr nachrechnet.
     */
    protected vector ChefZ_SpotNear(vector center, float minDist, float maxDist)
    {
        float angle = Math.RandomFloatInclusive(0.0, Math.PI2);
        float dist  = Math.RandomFloatInclusive(minDist, maxDist);

        vector spot = center;
        spot[0] = center[0] + Math.Sin(angle) * dist;
        spot[2] = center[2] + Math.Cos(angle) * dist;
        return spot;
    }

    //==========================================================================
    // Ernten (PROCESS_HARVEST_WILD)
    //==========================================================================

    //! Ist process die Ernte? Lookup() und nicht Intern() - ein nie
    //! deklarierter Prozessname bleibt INVALID und trifft dann nichts.
    //! Wortgleich zu ChefZ_Beehive.ChefZ_IsPackProcess().
    protected bool ChefZ_IsHarvestProcess(ChefZ_Sym process)
    {
        if (!ChefZ_SymbolTable.IsValid(process))
            return false;
        ChefZ_Sym harvest = ChefZ_SymbolTable.Lookup(CHEFZ_HARVEST_PROCESS);
        if (!ChefZ_SymbolTable.IsValid(harvest))
            return false;
        return process == harvest;
    }

    /**
     * Ein Spieler hat die Ernte durchgezogen.
     *
     * -------------------------------------------------------------------------
     * WELCHER AUSGANG ALS ERFOLG ZAEHLT
     * -------------------------------------------------------------------------
     * NICHT ChefZ_StationActionOutcome.IsSuccess(). Der Helfer kennt genau
     * zwei Erfolge - APPLIED und JOB_STARTED -, und beide setzen einen
     * Transform voraus. PROCESS_HARVEST_WILD hat absichtlich keinen (siehe
     * Dateikopf), der Ausgang ist deshalb IMMER NO_MATCH, und IsSuccess()
     * sagt dazu folgerichtig nein. Wer hier IsSuccess() abfragte, baute eine
     * Ernte, die nie erntet.
     *
     * Erfolglos ist allein RUN_FAILED: der Applicator ist mittendrin
     * gescheitert, die Welt ist unveraendert, und dann soll auch die Pflanze
     * unveraendert stehenbleiben. Dieselbe Unterscheidung und dieselbe
     * Begruendung wie an ChefZ_Beehive.ChefZ_OnStationActionFinished().
     *
     * Abgebrochene Aktionen kommen ohnehin nie hierher: diese Methode laeuft
     * nur aus OnFinishProgressServer.
     *
     * -------------------------------------------------------------------------
     * WARUM DIE ERNTE EINEN FRAME SPAETER LAEUFT
     * -------------------------------------------------------------------------
     * Der Haken laeuft MITTEN in OnFinishProgressServer der Aktion, deren
     * ZIEL diese Pflanze ist. Sie hier zu loeschen hiesse, der laufenden
     * Aktion ihr Ziel unter den Fuessen wegzuziehen. CallLater(..., 0) ist
     * derselbe Aufschub, den ChefZ_Beehive.ChefZ_PackUp() aus demselben Grund
     * nimmt.
     *
     * SERVERSEITIG. Der Aufrufer (ChefZ_ActionProcessAtStation.NotifyStation)
     * prueft g_Game.IsServer() bereits; die Wache steht trotzdem hier - der
     * Core empfiehlt das ausdruecklich fuer Ueberschreibungen, die auch aus
     * eigenem Code gerufen werden koennten.
     */
    override void ChefZ_OnStationActionFinished(PlayerBase actor, ItemBase inHands, ChefZ_Sym process, int outcome)
    {
        super.ChefZ_OnStationActionFinished(actor, inHands, process, outcome);

        if (!g_Game || !g_Game.IsServer())
            return;

        if (!actor)
            return;

        if (outcome == ChefZ_StationActionOutcome.RUN_FAILED)
            return;

        if (!ChefZ_IsHarvestProcess(process))
            return;

        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ChefZ_Harvest, 0, false);
    }

    /**
     * Der Ertrag faellt, die Pflanze geht. OEFFENTLICH, weil CallLater die
     * Methode ueber einen Methodenzeiger ruft.
     *
     * Reihenfolge, und sie ist Absicht:
     *
     *   1. Ertragsklasse holen. Ist keine da, bleibt die Pflanze stehen.
     *   2. Wuerfeln (1 + ChefZ_RollBonus()).
     *   3. Je Stueck ein Objekt neben die Pflanze legen.
     *   4. Ist NICHTS entstanden, bleibt die Pflanze stehen.
     *   5. Sonst: DeleteSafe().
     *
     * Schritt 4 ist die eigentliche Zusage: lieber eine Pflanze zu viel als
     * ein Spieler, der fuenf Sekunden erntet und nichts in der Hand hat.
     * Dieselbe Richtung wie ChefZ_PackUp ("Lieber ein Stock zu viel als einer
     * zu wenig").
     *
     * KEINE MENGE WIRD GESETZT. Alle vier Ertragsklassen fuehren
     * varQuantityInit = 1 und varQuantityMax = 1 (config.cpp,
     * ChefZ_VegetableFood_Base und ChefZ_FreshHerbBase) - der Klassendefault
     * IST der eine Kolben bzw. das eine Bund. "Mehr Ertrag" heisst deshalb
     * MEHR ITEMS und nie eine hoehere Quantity; eine 2 an einem Item mit
     * varQuantityMax 1 waere still auf 1 geklemmt worden, und der Wurf haette
     * nichts bewirkt.
     *
     * IsSetForDeletion(): zwischen Aktionsende und diesem Frame kann die
     * Pflanze bereits fort sein (Cleanup der CE, Admin, zweiter Spieler).
     */
    void ChefZ_Harvest()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        if (IsSetForDeletion())
            return;

        string yieldClass = ChefZ_YieldClass();
        if (yieldClass == "")
        {
            ChefZ_LogPlant("nicht geerntet: die Klasse nennt keine Ertragsklasse.");
            return;
        }

        int wanted = 1 + ChefZ_RollBonus();
        vector center = GetPosition();
        int made = 0;

        for (int i = 0; i < wanted; i++)
        {
            // Das erste Stueck liegt an der Pflanze, jedes weitere ein
            // Handbreit daneben - drei Kolben auf demselben Punkt waeren ein
            // Kolben mit zwei unsichtbaren Zwillingen.
            vector spot = center;
            if (i > 0)
                spot = ChefZ_SpotNear(center, CHEFZ_YIELD_SPREAD_MIN_M, CHEFZ_YIELD_SPREAD_MAX_M);

            EntityAI fruit = EntityAI.Cast(g_Game.CreateObjectEx(yieldClass, spot, ECE_PLACE_ON_SURFACE));
            if (!fruit)
                continue;
            made = made + 1;
        }

        if (made == 0)
        {
            ChefZ_LogPlant("nicht geerntet: " + yieldClass + " liess sich nicht anlegen - die Pflanze bleibt stehen.");
            return;
        }

        ChefZ_LogPlant("geerntet: " + made.ToString() + "x " + yieldClass + " (gewuerfelt " + wanted.ToString() + ").");
        DeleteSafe();
    }

    //! Die eine Spur der Pflanze. Hinter der Kanalwache, weil die Zeichenkette
    //! sonst auch dann entstuende, wenn niemand sie liest (18 §2).
    protected void ChefZ_LogPlant(string msg)
    {
        if (!ChefZ_Log.Enabled(ChefZ_LogChannel.PROCESS, ChefZ_LogLevel.DEBUG))
            return;
        ChefZ_Log.Debug(ChefZ_LogChannel.PROCESS, "Wildpflanze " + GetType() + ": " + msg);
    }
}

//------------------------------------------------------------------------------
// Die vier Pflanzen. Jede traegt genau das, was in der Ausbeutetabelle der
// Spec (Kap. 5) in ihrer Zeile steht - und sonst nichts.
//------------------------------------------------------------------------------

/**
 * Mais. Die einzige der vier, die eine Gruppe bildet (Auftrag: "Mais waechst
 * in Gruppen von 1-3 nebeneinander"), und die einzige mit einem zweiten
 * Wurfband.
 *
 * 0..2 Begleiter, GLEICHVERTEILT. Kein Gewicht auf die Mitte: eine Gruppe ist
 * eine Gruppe, und ein Drittel Einzelpflanzen ist genau das, was "1-3" sagt.
 */
class ChefZ_WildCorn extends ChefZ_WildPlant_Base
{
    //==========================================================================
    // DAS MODELL HAT SECHS WUCHSSTUFEN (Befund Asset-Tracker, 31.08.2026)
    //
    // corn_plant.p3d haengt an PlantBaseSkeleton und traegt vierzehn
    // Verbergen-Animationen (ChefZ_Plants/models/model.cfg, class Animations):
    // Pile_01, Pile_02, PlantStage_01..06 und PlantStage_01..06_crops. Alle
    // sind "type=hide" mit "source=user" - jede von ihnen ist SICHTBAR,
    // solange ihre Phase 0 ist.
    //
    // Vanillas Auswahl macht PlantBase.UpdatePlant() (scripts - 1.29,
    // 4_World/DayZ/Entities/GardenBase/PlantBase.c:448-479): sie zeigt die
    // aktuelle Stufe und verbirgt die vorige, aufgerufen aus GrowthTimerTick,
    // OnStoreLoadCustom und der Ernte. Eine Wildpflanze ist KEIN PlantBase -
    // sie waechst nicht, sie steht reif da -, ruft das also nie. Ohne
    // Gegenmassnahme staenden alle sechs Stufen ineinander.
    //
    // DIE HAUPTLOESUNG STEHT IN DER CONFIG, NICHT HIER: class AnimationSources
    // an ChefZ_WildCorn setzt initPhase je Auswahl (config.cpp). Das ist der
    // Weg, den die Engine ohne jedes Skript geht, und er greift auf jedem
    // Client, der das Objekt hereinstreamt. Form belegt an
    // DayZExpansion/Objects/Airdrop/config.cpp:23-35 und
    // DayZExpansion/NamalskAdventure/Dta/Objects/SupplyCrates/config.cpp:66-74.
    //
    // WARUM ES HIER TROTZDEM NOCH EINMAL STEHT: initPhase ist eine Zusage der
    // Engine, die wir statisch nicht nachpruefen koennen - und ein Fehlgriff
    // waere eine unsichtbare oder sechsfache Maispflanze, also genau der
    // Fehler, der einem Spieler auffaellt und einem Validator nicht. Die
    // Wiederholung im Skript kostet vierzehn Aufrufe je Pflanze, EINMAL beim
    // Erscheinen. HideSelection verlangt ausdruecklich einen Eintrag in
    // class AnimationSources (Kommentar an EntityAI.HideAllSelections,
    // EntityAI.c:3354) - beide Wege haengen also an derselben Configzeile,
    // und keiner von beiden ersetzt sie.
    //
    // OFFEN BIS ZUR SICHTPRUEFUNG IM SPIEL: dass ein Nicht-PlantBase-Objekt
    // diese Auswahlen ueberhaupt schalten darf, ist aus den Quellen plausibel
    // (SetAnimationPhase sitzt auf Entity, nicht auf PlantBase), aber nicht
    // durch ein laufendes Beispiel belegt. Steht sie im Gate falsch da, ist
    // das eine Modellfrage und keine Skriptfrage - siehe README_WildPlants.md.
    //==========================================================================

    //! Die Namen sind die ANIMATIONSKLASSEN aus model.cfg, nicht die
    //! Knochennamen. PlantBase adressiert sie genauso ("plantStage_" +
    //! zweistellige Nummer, PlantBase.c:456) - Confignamen sind
    //! gross-klein-gleichgueltig.
    static const string CHEFZ_SEL_STAGE_PREFIX = "PlantStage_";
    static const string CHEFZ_SEL_CROPS_SUFFIX = "_crops";
    static const string CHEFZ_SEL_PILE_01      = "Pile_01";
    static const string CHEFZ_SEL_PILE_02      = "Pile_02";

    //! Sechs Stufen fuehrt das Modell, die sechste ist die reife.
    static const int CHEFZ_CORN_STAGES     = 6;
    static const int CHEFZ_CORN_RIPE_STAGE = 6;

    /**
     * Nur die reife Stufe samt ihren Kolben bleibt stehen; alles andere geht
     * weg. Die Kolben AUSDRUECKLICH sichtbar: eine Maispflanze ohne Kolben
     * waere eine Pflanze, an der die Ernteaktion luegt.
     *
     * Die Nummer wird zweistellig gebildet ("PlantStage_" + "0" + i), weil
     * die Animationsklassen so heissen. Bis sechs braucht das keine
     * Fallunterscheidung - eine zweite Ziffer gibt es an diesem Modell nicht.
     */
    override void ChefZ_ApplyModelStage()
    {
        HideSelection(CHEFZ_SEL_PILE_01);
        HideSelection(CHEFZ_SEL_PILE_02);

        for (int i = 1; i <= CHEFZ_CORN_STAGES; i++)
        {
            string stage = CHEFZ_SEL_STAGE_PREFIX + "0" + i.ToString();
            string crops = stage + CHEFZ_SEL_CROPS_SUFFIX;

            if (i == CHEFZ_CORN_RIPE_STAGE)
            {
                ShowSelection(stage);
                ShowSelection(crops);
                continue;
            }

            HideSelection(stage);
            HideSelection(crops);
        }
    }

    override string ChefZ_YieldClass()
    {
        return "ChefZ_Corn";
    }

    override int ChefZ_BonusOnePct()
    {
        return 25;
    }

    override int ChefZ_BonusTwoPct()
    {
        return 5;
    }

    override int ChefZ_RollCompanions()
    {
        return Math.RandomIntInclusive(0, 2);
    }
}

//! Thymian. Ein Bund sicher, jedes vierte Mal zwei.
class ChefZ_WildThyme extends ChefZ_WildPlant_Base
{
    override string ChefZ_YieldClass()
    {
        return "ChefZ_Thyme";
    }

    override int ChefZ_BonusOnePct()
    {
        return 25;
    }
}

//! Rosmarin. Dieselben Zahlen wie Thymian - die Seltenheit steuert die
//! types.xml des Servers, nicht der Wurf.
class ChefZ_WildRosemary extends ChefZ_WildPlant_Base
{
    override string ChefZ_YieldClass()
    {
        return "ChefZ_Rosemary";
    }

    override int ChefZ_BonusOnePct()
    {
        return 25;
    }
}

//! Petersilie. Siehe Rosmarin.
class ChefZ_WildParsley extends ChefZ_WildPlant_Base
{
    override string ChefZ_YieldClass()
    {
        return "ChefZ_Parsley";
    }

    override int ChefZ_BonusOnePct()
    {
        return 25;
    }
}
