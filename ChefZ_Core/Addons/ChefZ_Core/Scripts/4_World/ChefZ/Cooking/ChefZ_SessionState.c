//==============================================================================
// ChefZ_ESessionState - der Zustand einer Kochsitzung
//
// Entwurf: 10 §4 (Werteliste woertlich), 10 §5 (wer wohin wechselt),
// 10 §7 (Sitzungen sind reine Laufzeit), 10 E6 (SUPPRESSED nach drei
// Fehlversuchen).
//
// Der Zustandsautomat, vollstaendig:
//
//     IDLE ---- Stufe B trifft --------> MATCHED
//       ^                                  |
//       |                                  | Abschluss erfuellt
//       |                                  v
//       |                              COMPLETING
//       |                                  |
//       |                          Erfolg  |  3x Fehlschlag
//       |                                  v          v
//       |                                DONE     SUPPRESSED
//       |                                  |          |
//       +------ Signaturwechsel -----------+----------+
//
// DONE und SUPPRESSED sind beide "hier ist nichts mehr zu tun" und kosten pro
// Tick genau einen Signaturvergleich. Der Unterschied liegt in der Herkunft:
// DONE heisst "fertig oder kein Treffer" und ist der Normalfall, SUPPRESSED
// heisst "hier ist etwas kaputt" und wird einmal als ERROR gemeldet.
//
// Warum ein enum und keine Konstantenklasse (anders als ChefZ_Completion in
// 1_Core): dort verbot die Lage in 1_Core einen Enum-Vergleich ueber
// Modulgrenzen. Hier lebt der Wert vollstaendig in 4_World, wird nie
// persistiert, nie synchronisiert und nie aus JSON gelesen - ein enum ist
// genau das richtige Werkzeug und 10 §4 schreibt ihn woertlich so.
//
// Layer: 4_World.
//==============================================================================

enum ChefZ_ESessionState
{
    IDLE       = 0,     //! nichts gebunden - Stufe B darf laufen
    MATCHED    = 1,     //! Rezept gebunden, Abschluss noch offen
    COMPLETING = 2,     //! Abschluss erkannt, Transaktion laeuft
    DONE       = 3,     //! in diesem Gefaess fertig, wartet auf Inhaltsaenderung
    SUPPRESSED = 4      //! Fehler aufgetreten, Gefaess bis Signaturwechsel ignorieren
}
