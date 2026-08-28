//==============================================================================
// ChefZ_TerjeSkillsAbsent - die Gegenseite der weichen Abhaengigkeit.
//
// Diese Datei ist das GENAUE GEGENSTUECK zu ChefZ_TerjeSkillsEntry.c: dort
// "#ifdef TERJE_SKILLS_MOD", hier "#ifndef". Es kann immer nur eine von beiden
// kompiliert werden, nie beide. Damit gibt es auch nie zwei aktive
// "modded class MissionServer" aus diesem PBO.
//
// ------------------------------------------------------------------------
// WARUM ES DIESE DATEI UEBERHAUPT GIBT
// ------------------------------------------------------------------------
// Ohne sie waere ein Server, der dieses PBO ohne TerjeSkills laedt, komplett
// stumm - der Betreiber saehe im RPT keinen Unterschied zwischen "Modul
// geladen und untaetig" und "Modul gar nicht geladen". Genau eine Zeile beim
// Start schliesst diese Luecke, ohne irgendetwas zu tun.
//
// Die Zeile ist bewusst KEINE Warnung und KEIN Fehler. Ein Comp-Mod ohne
// seinen Zielmod ist kein Defekt, sondern ein zulaessiger Betriebszustand.
//
// ------------------------------------------------------------------------
// WARUM PrintToRPT UND NICHT ChefZ_Log
// ------------------------------------------------------------------------
// ChefZ_Log.Banner() waere naheliegend - der aktive Pfad benutzt genau das.
// Der INAKTIVE Pfad soll aber so wenig voraussetzen wie irgend moeglich: er
// laeuft in der Lage, in der etwas fehlt. PrintToRPT ist eine Engine-Funktion
// und in jedem Skriptmodul jedes Mods vorhanden; sie kann nicht selbst zur
// Fehlerquelle werden. Das Praefix ist woertlich das von ChefZ_Log
// (ChefZ_Log.c:35 PREFIX = "[ChefZ]", Banner() setzt "[CORE]" dahinter),
// damit ein Betreiber dieselbe Zeichenkette greppen kann wie sonst auch.
//
// ------------------------------------------------------------------------
// NUR SERVER
// ------------------------------------------------------------------------
// MissionServer.OnInit ist derselbe Einstiegspunkt, den der aktive Pfad
// benutzt. Der Client bleibt still: die Aussage "eine Anbindung ist inaktiv"
// gehoert ins Serverlog, nicht in 60 Client-RPTs. Im Offline-Modus (Editor,
// Solo) existiert kein MissionServer und die Zeile erscheint deshalb nicht -
// dort ist ohnehin nichts anzubinden.
//==============================================================================
#ifndef TERJE_SKILLS_MOD
// Kein eigener "modded class MissionServer" mehr: zwei Comp-Module mit je
// einem eigenen Override haben den Server am 28.08.2026 mit einer
// Zugriffsverletzung beendet, jedes einzeln lief. Der Core stellt dafuer genau
// einen Haken bereit - siehe ChefZ_CompNotice.
modded class ChefZ_CompNotice
{
    override void Emit()
    {
        super.Emit();
        ChefZ_Log.Banner("TerjeSkills-Anbindung geladen, aber TerjeSkills ist nicht installiert - dieses Modul bleibt vollstaendig untaetig. Kein Fehler.");
    }
}
#endif // !TERJE_SKILLS_MOD
