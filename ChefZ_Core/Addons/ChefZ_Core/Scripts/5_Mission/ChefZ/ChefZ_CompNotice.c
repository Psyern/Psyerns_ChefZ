//==============================================================================
// ChefZ_CompNotice - der eine Haken, an dem Comp-Module beim Start haengen
//
// ---------------------------------------------------------------------------
// WARUM ES DIESEN HAKEN GIBT
// ---------------------------------------------------------------------------
// Der Kopf von ChefZ_Boot sagt es seit S1: der Core hat GENAU EINEN
// Einstiegspunkt je Seite, weil ein zweites Paar "modded class MissionServer"
// zusaetzliche Kollisionsflaeche gegenueber anderen Mods waere - fuer null
// Gewinn. Die Comp-Module hielten sich nicht daran; jedes brachte seinen
// eigenen MissionServer-Override mit.
//
// Am 28.08.2026 hat das den Testserver umgebracht, und die Messung war
// eindeutig:
//
//   Core + acht Content-Addons                     laeuft, 75 s
//   dazu EIN Comp-Modul mit eigenem Override       laeuft, 75 s
//   dazu ZWEI Comp-Module mit eigenem Override     Zugriffsverletzung
//
// Der Absturz ist nativ. Es gibt keine Skriptausnahme, keinen brauchbaren
// Aufrufkeller, und die genannte Zeile ist immer nur die aeusserste - nie die
// schuldige. Wer ihr glaubt, sucht an der falschen Stelle.
//
// ---------------------------------------------------------------------------
// WIE ES BENUTZT WIRD
// ---------------------------------------------------------------------------
// Der Core ruft EmitAll() genau einmal auf, aus seinem eigenen Einstiegspunkt
// heraus, nachdem der Boot durch ist. Ein Comp-Modul schreibt:
//
//     modded class ChefZ_CompNotice
//     {
//         override void Emit()
//         {
//             super.Emit();
//             ChefZ_Log.Banner("...");
//         }
//     }
//
// Kein MissionServer, kein zweiter Einstiegspunkt, und die Reihenfolge der
// Comp-Module spielt keine Rolle - super.Emit() reicht die Kette weiter.
//
// I4 bleibt gewahrt: der Core kennt hier kein Fremdsystem. Er stellt einen
// leeren Haken bereit und weiss nicht, wer sich daranhaengt.
//
// Layer: 5_Mission.
//==============================================================================

class ChefZ_CompNotice
{
    //! Von jedem Comp-Modul ueberschrieben. Im Core absichtlich leer.
    void Emit()
    {
    }

    /**
     * Einmal je Serverstart, aus ChefZ_CoreEntry heraus.
     *
     * Die Instanz ist kurzlebig und wird nicht gehalten: die Meldungen fallen
     * beim Start an und danach nie wieder.
     */
    static void EmitAll()
    {
        ChefZ_CompNotice notice = new ChefZ_CompNotice();
        notice.Emit();
    }
}
