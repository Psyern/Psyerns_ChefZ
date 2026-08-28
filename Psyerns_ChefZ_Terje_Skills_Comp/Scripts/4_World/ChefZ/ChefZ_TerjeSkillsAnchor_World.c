//==============================================================================
// ChefZ_TerjeSkillsAnchor_World - haelt die Schicht 4_World gefuellt
//
// ---------------------------------------------------------------------------
// WOZU EINE KLASSE, DIE NICHTS TUT
// ---------------------------------------------------------------------------
// Jede Datei dieser Schicht steht hinter einem #ifdef auf den Partnermod. Ist
// der nicht installiert, uebersetzt die Schicht zu NICHTS - sie steht dann
// zwar weiterhin in CfgMods.files[], bringt aber keine einzige Klasse mit.
//
// Genau in dieser Lage stuerzt der Server ab. Nachgewiesen am 28.08.2026 auf
// dem Testserver, in dieser Reihenfolge:
//
//   - ChefZ_Core plus alle acht Content-Addons: laeuft, 75 s ohne Fehler.
//   - Dazu Psyerns_ChefZ_Terje_Skills_Comp, dessen Partnermod fehlt:
//     Zugriffsverletzung beim Missionsstart, zweimal reproduziert.
//   - Dasselbe mit leergeraeumtem 5_Mission-Skript: laeuft wieder.
//   - Dasselbe mit vollstaendigem Skript UND einer einzigen leeren Klasse in
//     4_World: laeuft.
//   - Die beiden anderen Comp-Mods, deren Partnermods installiert sind und
//     deren Schichten deshalb Inhalt haben, liefen von Anfang an.
//
// Die Meldung nennt dabei stets die aeusserste Skriptzeile - nicht die
// schuldige. Wer der Zeile glaubt, sucht tagelang an der falschen Stelle; die
// Ursache ist die leere Schicht, nicht die Anweisung.
//
// Diese Klasse tut nichts und soll nichts tun. Sie sorgt allein dafuer, dass
// die Schicht einen Inhalt hat.
//==============================================================================

class ChefZ_TerjeSkillsAnchor_World
{
    //! Nur, damit die Klasse einen Rumpf hat. Niemand ruft das auf.
    static bool IstVorhanden()
    {
        return true;
    }
}
