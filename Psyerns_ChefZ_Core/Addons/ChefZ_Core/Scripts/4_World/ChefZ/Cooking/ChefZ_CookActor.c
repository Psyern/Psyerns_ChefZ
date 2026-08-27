//==============================================================================
// ChefZ_CookActor - wem gehoert ein Gericht?
//
// Entwurf: 08 §3 (actorIdentityId, "0 = niemand beteiligt"), 10 §7 (der
// Kochadapter schreibt KEINEN persistenten Zustand), 00 §5 (die geschlossene
// Liste gemoddeter Vanilla-Klassen), 17 §3.3 (Capabilities bei Identitaet 0),
// 17 §4 (nur ChefZ_OnRecipeCompleted ist XP-tauglich), 16 E5
// (containerSearchRadius - dieselbe Ueberlegung, dieselbe Bauform).
//
// ---------------------------------------------------------------------------
// Der Befund zuerst, dann die Loesung
// ---------------------------------------------------------------------------
// 08 §3 hat recht: ein Topf auf einem Feuer hat keinen Besitzer. Das ist kein
// Versaeumnis des Entwurfs, sondern eine Eigenschaft der Engine, und ChefZ
// darf sie nicht aufheben:
//
//   - Vanilla fuehrt an keinem Kochgeraet eine Spielerkennung.
//   - Die einzige Stelle, an der die Engine "Spieler X bewegt Gegenstand Y
//     nach Z" meldet, sind EEItemLocationChanged / EEInventoryOut auf dem
//     BEWEGTEN GEGENSTAND. Sie zu nutzen hiesse "modded class ItemBase" -
//     und 00 §5 fuehrt ItemBase, Edible_Base, Pot, FryingPan, Cauldron und
//     FireplaceBase als geschlossene Liste der NICHT gemoddeten Klassen. Die
//     Liste ist eine harte Zusage: "waechst sie, ist der Entwurf falsch."
//   - Ein eigenes Sync- oder Persistenzfeld am Vanilla-Gefaess scheidet aus
//     demselben Grund aus (10 §6, OF-14: "ChefZ speichert nur auf eigenen
//     Klassen. Eine Station ist eine ChefZ-Klasse, ein Kochtopf nicht.").
//
// Es gibt also keinen Weg, den Zutatenleger direkt zu ERFAHREN. Was bleibt,
// ist ihn zu BEOBACHTEN - und zwar an dem einen Moment, den nur ein Spieler
// herbeifuehren kann.
//
// ---------------------------------------------------------------------------
// Die Regel: der Bestand waechst nur durch Menschenhand
// ---------------------------------------------------------------------------
// Vanillas Kochschleife legt NIE etwas in ein Gefaess. Cooking.CookWithEquipment
// laeuft ueber das vorhandene Cargo, erhoeht Temperatur und Garzeit, zieht
// Quantity ab und kann ein Item bei Quantity 0 verschwinden lassen - sie
// erzeugt keines. Damit gilt ausnahmslos:
//
//     Der Bestand eines Kochgeraets waechst genau dann, wenn ein Spieler
//     etwas hineingelegt hat.
//
// Das ist der Ankerpunkt. In genau diesem Tick - und in keinem anderen -
// fragt der Adapter, wer am Geraet steht, und schreibt die Antwort in die
// Sitzung. Ein reifendes Gericht, eine kippende Kochmethode, verdunstendes
// Wasser, ein Klassentausch durch das Zustandssystem: nichts davon vergroessert
// den Bestand, nichts davon eroeffnet einen Anspruch. Wer nur danebensteht,
// erbt nichts.
//
// ---------------------------------------------------------------------------
// Und wenn mehrere dastehen?
// ---------------------------------------------------------------------------
// Dann gehoert das Gericht NIEMANDEM - mit einer Ausnahme: der bisherige
// Anspruchsinhaber behaelt seinen Anspruch, solange er selbst in Reichweite
// ist (Decide()).
//
// Beides zusammen ergibt die beiden Eigenschaften, auf die es ankommt:
//
//   1. Niemand kann einen Anspruch AN SICH ZIEHEN, indem er sich dazustellt.
//      Ist der Koch da, bleibt es sein Topf. Sind zwei Fremde da, bekommt
//      keiner etwas.
//   2. Niemand kann einen Anspruch VERHINDERN, indem er sich dazustellt.
//      Der Koch ist ja anwesend und behaelt ihn.
//
// Die sichere Richtung ist immer 0. Bei 0 vergibt kein Fortschrittsempfaenger
// etwas (das ist seine Aufgabe, nicht die des Core) und ChefZ_CapabilityGate
// sperrt nichts. Ein Kochvorgang ohne Spieler in der Naehe laeuft vollstaendig
// durch und erzeugt sein Gericht - nur ohne Zuschreibung.
//
// ---------------------------------------------------------------------------
// Was dieser Weg NICHT leistet, ehrlich benannt
// ---------------------------------------------------------------------------
// Die ERSTE Messung einer Sitzung hat keinen Vorzustand und behandelt jeden
// vorgefundenen Bestand als Zuwachs. Eine Sitzung entsteht neu, wenn ein
// Kochgeraet nach einem Serverneustart oder nach sessionTtlSec ohne Kochtick
// zum ersten Mal wieder tickt. Wer zu diesem Zeitpunkt als EINZIGER an einem
// brennenden, bereits befuellten fremden Feuer steht, kann den Anspruch
// erben. Das Fenster ist schmal, verlangt die vollstaendige Abwesenheit des
// urspruenglichen Kochs und kostet ihn nichts, was er sonst bekommen haette -
// aber es ist da, und es steht hier, statt verschwiegen zu werden.
//
// Die Alternative waere, den ersten Bestand nur zu uebernehmen und nie
// zuzuschreiben. Sie ist verworfen, weil sie den Normalfall trifft: wer einen
// gefuellten Topf auf eine Feuerstelle stellt und sie anzuendet, erzeugt
// danach keinen Zuwachs mehr und bekaeme nie einen Anspruch.
//
// ---------------------------------------------------------------------------
// Server, immer
// ---------------------------------------------------------------------------
// Der Kochhook laeuft ohnehin nur auf dem Server (10 §5). Die Wache steht
// hier trotzdem noch einmal: eine Identitaet, die auf dem Client entstuende,
// waere eine Behauptung des Clients ueber sich selbst.
//
// KEIN CONTENT und KEIN FREMDSYSTEMBEZUG. Diese Datei stellt fest, WER
// gehandelt hat. Ob daraus Erfahrung, ein Rezeptschloss oder gar nichts wird,
// entscheidet ausserhalb des Core, wer sich an die Ereignisse haengt (17 §2).
//
// Layer: 4_World.
//==============================================================================

class ChefZ_CookActor
{
    //! "Niemand". Derselbe Wert wie ChefZ_CookContext.actorIdentityId im
    //! Ruhezustand und derselbe, den ChefZ_EventArgs.identityId traegt, wenn
    //! kein Spieler beteiligt war (17 §3.1).
    static const int NOBODY = 0;

    /**
     * Wiederverwendeter Puffer fuer GetPlayers().
     *
     * Die Aufloesung laeuft nur bei einem Bestandszuwachs - also wenn ein
     * Spieler gerade etwas eingelegt hat - und nicht pro Kochtick. Trotzdem
     * kein neues Array je Aufruf: auf einem vollen Server mit vielen
     * Feuerstellen summiert sich auch das, und der Puffer kostet nichts.
     */
    private static ref array<Man> s_Players;

    //==========================================================================
    // Die Regel, ohne Welt
    //==========================================================================

    /**
     * Aus "wer ist da" wird "wem gehoert das Gericht".
     *
     * Absichtlich als reine Funktion herausgezogen: sie ist der Kern der
     * Entscheidung und laesst sich damit ohne Spieler, ohne Gefaess und ohne
     * laufendes Spiel pruefen (SelfCheck unten). Alles darueber ist nur die
     * Frage, wer als anwesend gilt.
     *
     * @param incumbentId       bisheriger Anspruchsinhaber, NOBODY wenn keiner.
     * @param incumbentPresent  ist er unter den Anwesenden?
     * @param presentCount      Zahl der anwesenden Spieler mit Identitaet.
     * @param firstPresentId    Identitaet des ersten Anwesenden. Nur bei
     *                          presentCount == 1 ausgewertet.
     */
    static int Decide(int incumbentId, bool incumbentPresent,
                      int presentCount, int firstPresentId)
    {
        // Der Koch behaelt seinen Topf, solange er dabeisteht. Diese Zeile ist
        // der Schutz gegen beides: gegen den, der einen Anspruch an sich
        // ziehen will, und gegen den, der ihn nur verhindern will.
        if (incumbentId != NOBODY && incumbentPresent)
            return incumbentId;

        // Eindeutig oder gar nicht. Zwei Fremde am selben Feuer sind keine
        // Antwort auf die Frage, wer gekocht hat.
        if (presentCount == 1)
            return firstPresentId;

        return NOBODY;
    }

    //==========================================================================
    // Die Welt
    //==========================================================================

    /**
     * Wer handelt gerade an diesem Geraet?
     *
     * @param device      das Gefaess. Wird ausschliesslich GELESEN.
     * @param incumbentId bisheriger Anspruchsinhaber der Sitzung.
     * @param radiusM     Umkreis in Metern aus Core.json. <= 0 schaltet die
     *                    Zuschreibung vollstaendig ab und liefert immer
     *                    NOBODY - auch fuer ein Geraet in einer Hand.
     * @return Spielerkennung oder NOBODY. Nie ein Fehler, nie eine Exception:
     *         eine misslungene Zuschreibung ist ein Gericht ohne Besitzer und
     *         kein Grund, einen Kochvorgang anzuhalten.
     */
    static int Resolve(notnull ItemBase device, int incumbentId, float radiusM)
    {
        if (!g_Game || !g_Game.IsServer())
            return NOBODY;

        if (radiusM <= 0.0)
            return NOBODY;

        // Ein Kochgeraet in der Hierarchie eines Spielers - in der Hand, im
        // Rucksack, auf einem Gaskocher in seinem Inventar - ist die einzige
        // Lage, in der die Frage gar nicht gestellt werden muss. Sie ist
        // eindeutig beantwortet, und zwar von der Engine.
        // Dieselbe Lebendpruefung wie unten: das Gefaess kann im Inventar
        // einer Leiche liegen, und eine Leiche kocht nicht.
        Man rootMan = device.GetHierarchyRootPlayer();
        if (rootMan)
        {
            if (!rootMan.IsAlive())
                return NOBODY;
            return IdOf(rootMan);
        }

        if (!s_Players)
            s_Players = new array<Man>();

        // Ueber eine lokale Zwischenvariable: GetPlayers nimmt die Liste als
        // out-Parameter, und s_Players ist ein FELD - ein Feld als
        // out-Parameter ist in Enforce nicht zugesichert (siehe Kopf von
        // ChefZ_TextList.SymbolsOf). Die Rueckzuweisung ist die Absicherung
        // fuer den Fall, dass die Engine doch eine neue Liste anlegt.
        array<Man> players = s_Players;
        players.Clear();
        g_Game.GetPlayers(players);
        s_Players = players;

        vector devicePos = device.GetPosition();
        float  radiusSq  = radiusM * radiusM;

        int  presentCount    = 0;
        int  firstPresentId  = NOBODY;
        bool incumbentInside = false;

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players.Get(i));
            if (!player)
                continue;

            // Ein Toter kocht nicht. Die Pruefung steht vor der Entfernung,
            // weil eine Leiche am Feuer sonst als "zweiter Anwesender" zaehlte
            // und dem Koch seinen Anspruch naehme.
            if (!player.IsAlive())
                continue;

            int id = IdOf(player);
            if (id == NOBODY)
                continue;                       // kein Identitaetstraeger

            if (vector.DistanceSq(devicePos, player.GetPosition()) > radiusSq)
                continue;

            presentCount++;
            if (presentCount == 1)
                firstPresentId = id;

            if (id == incumbentId)
                incumbentInside = true;
        }

        return Decide(incumbentId, incumbentInside, presentCount, firstPresentId);
    }

    //==========================================================================

    /**
     * NOBODY, wenn kein Spieler da ist oder er keine Identitaet hat.
     *
     * Ein Spieler ohne PlayerIdentity ist auf dem Server der Regelfall fuer
     * eine gerade getrennte Verbindung. Er ist niemand, dem etwas gehoeren
     * koennte - dieselbe Behandlung wie in ChefZ_ContainerService.ActorId()
     * und ChefZ_PortionedFood_Base.ChefZ_PortionActorId().
     */
    static int IdOf(Man man)
    {
        if (!man)
            return NOBODY;

        PlayerIdentity identity = man.GetIdentity();
        if (!identity)
            return NOBODY;

        return identity.GetPlayerId();
    }

    /**
     * Interessiert die Antwort ueberhaupt jemanden?
     *
     * 17 E2 in seiner allgemeinen Form: auf einem Server ohne Comp-Module
     * darf die Zuschreibung nichts kosten. Ohne Fortschrittsempfaenger, ohne
     * Faehigkeitsanbieter und ohne Abonnenten der beiden Kochereignisse waere
     * die aufgeloeste Identitaet eine Zahl, die niemand liest - dann wird sie
     * gar nicht erst ermittelt und bleibt NOBODY.
     *
     * Der Aufruf kostet vier Map-Zugriffe und laeuft nur bei einem
     * Bestandszuwachs.
     */
    static bool AnyoneCares()
    {
        if (ChefZ_ProgressRegistry.HasSinks())
            return true;

        // Rezeptschloesser: ohne eingehaengtes Tor und ohne Anbieter kann
        // requires[] niemanden von etwas ausschliessen (17 §3.3).
        if (ChefZ_CapabilityGate.HasActive())
            return true;
        if (ChefZ_CapabilityRegistry.Get().GetProviderCount() > 0)
            return true;

        ChefZ_EventBus bus = ChefZ_EventBus.Get();
        if (bus.HasSubscribers(ChefZ_EventNames.RECIPE_COMPLETED))
            return true;
        if (bus.HasSubscribers(ChefZ_EventNames.RECIPE_MATCHED))
            return true;

        return false;
    }

    //==========================================================================

    /**
     * Nur fuer den Selbsttest - ohne Spieler, ohne Gefaess, ohne Spiel.
     *
     * Geprueft wird die REGEL, nicht die Umkreissuche. Die Umkreissuche
     * braucht eine Welt und gehoert damit in das Fenster aus 19 S7; die Regel
     * ist der Teil, dessen Fehler man auf einem Server NICHT saehe - eine
     * falsche Zuschreibung sieht aus wie eine richtige.
     */
    static bool SelfCheck()
    {
        // 1. Niemand da -> niemand bekommt etwas.
        if (Decide(NOBODY, false, 0, NOBODY) != NOBODY)      return false;

        // 2. Genau einer da, kein Vorbesitzer -> er bekommt den Anspruch.
        if (Decide(NOBODY, false, 1, 4711) != 4711)          return false;

        // 3. Zwei Fremde da, kein Vorbesitzer -> niemand. Nicht "der erste".
        if (Decide(NOBODY, false, 2, 4711) != NOBODY)        return false;

        // 4. Der Koch steht dabei, ein Zweiter auch -> der Koch behaelt ihn.
        //    Das ist der Schutz gegen die Verhinderung durch Danebenstehen.
        if (Decide(4711, true, 2, 1234) != 4711)             return false;

        // 5. Der Koch ist weg, ein Fremder allein da -> der Fremde legt
        //    gerade selbst etwas ein und uebernimmt. Der Koch hat den Topf
        //    verlassen; das ist keine Enteignung, sondern ein Wechsel.
        if (Decide(4711, false, 1, 1234) != 1234)            return false;

        // 6. Der Koch ist weg, zwei Fremde da -> niemand.
        if (Decide(4711, false, 2, 1234) != NOBODY)          return false;

        // 7. Der Koch ist der einzige Anwesende -> er behaelt ihn, und zwar
        //    ueber Zweig 1 und nicht zufaellig ueber Zweig 2.
        if (Decide(4711, true, 1, 4711) != 4711)             return false;

        // 8. IdOf(null) ist NOBODY und keine Ausnahme.
        if (IdOf(null) != NOBODY)                            return false;

        return true;
    }
}
