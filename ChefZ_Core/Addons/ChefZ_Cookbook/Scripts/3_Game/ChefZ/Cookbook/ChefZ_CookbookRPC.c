//==============================================================================
// ChefZ_CookbookRPC - die Nachrichtennamen, und warum sie alle hier stehen
//
// Entwurf: ChefZ_Cookbook_Workflow §5, Regel 5.
//
// ---------------------------------------------------------------------------
// CORE-MIGRATION: NAMEN STATT NUMMERN
// ---------------------------------------------------------------------------
// Frueher liefen die Nachrichten als Vanilla-RPC mit den Nummern 10000-10002
// ueber modded PlayerBase.OnRPC. Der Bereich ab 10000 ist unter Mods nicht
// koordiniert - jede andere Mod, die "ab 10000" zaehlt, haette dieselben
// Nummern treffen koennen. Jetzt laufen sie ueber PsyCore_RPC (Psyerns Core
// Module): EINE Router-Nummer fuer alle Psyern-Mods, dahinter Namensraum +
// Funktionsname als Text. Kollisionen sind damit ausgeschlossen, solange der
// Namensraum eindeutig ist.
//
// Sie stehen weiterhin in EINER Datei, damit chefz-conflict-scout eine Stelle
// hat, an der er alle Nachrichten sieht.
//
// Wire-Kompatibilitaet: Client und Server muessen dieselbe ChefZ-Fassung
// fahren (war bei den Nummern ebenso).
//
// Layer: 3_Game. Reine Konstanten.
//==============================================================================

class ChefZ_CookbookRPC
{
    //! Namensraum im PsyCore_RPC-Router.
    static const string NS = "ChefZ_Cookbook";

    //! Server -> Client: der VOLLSTAENDIGE Wissensstand.
    //! Anlass: Spieler verbindet, oder er oeffnet das Buch das erste Mal.
    //! (frueher Vanilla-RPC 10000)
    static const string FULL_STATE = "FullState";

    //! Server -> Client: nur das Delta. Reserviert, nicht registriert.
    //! Anlass: eine neue Zutat oder ein neu gemeistertes Rezept.
    //! (frueher Vanilla-RPC 10001)
    static const string DELTA = "Delta";

    //! Client -> Server: "schick mir den Stand".
    //! Anlass: das Buch wird geoeffnet und der Clientspiegel ist leer.
    //! (frueher Vanilla-RPC 10002)
    static const string REQUEST_STATE = "RequestState";
}
