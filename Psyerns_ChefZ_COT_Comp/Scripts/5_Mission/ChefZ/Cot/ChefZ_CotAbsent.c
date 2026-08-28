//==============================================================================
// ChefZ_CotAbsent - die Gegenseite der weichen Abhaengigkeit.
//
// Gegenstueck zu ChefZ_CotObjectSpawner.c: dort "#ifdef JM_COT", hier
// "#ifndef". Immer nur eine von beiden wird kompiliert, also gibt es aus
// diesem PBO nie zwei aktive Erweiterungen derselben Klasse.
//
// Hier wird ausdruecklich MissionServer erweitert und NICHT
// JMObjectSpawnerForm - diese COT-Klasse gibt es in genau der Lage nicht, in
// der diese Datei kompiliert wird.
//
// ------------------------------------------------------------------------
// WARUM PrintToRPT
// ------------------------------------------------------------------------
// Engine-Funktion, in jedem Skriptmodul jedes Mods vorhanden, kann nicht
// selbst zur Fehlerquelle werden. Das Praefix ist woertlich das von ChefZ_Log
// (Psyerns_ChefZ_Core/Addons/ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_Log.c:35),
// damit ein Betreiber dieselbe Zeichenkette greppen kann wie sonst auch.
//
// Die Zeile ist bewusst KEINE Warnung und KEIN Fehler. Ein Comp-Mod ohne
// seinen Zielmod ist kein Defekt, sondern ein zulaessiger Betriebszustand -
// und dieses Modul ist ohnehin nur ein Adminwerkzeug ohne Spielmechanik.
//==============================================================================
#ifndef JM_COT
// Kein eigener "modded class MissionServer" mehr: zwei Comp-Module mit je
// einem eigenen Override haben den Server am 28.08.2026 mit einer
// Zugriffsverletzung beendet, jedes einzeln lief. Der Core stellt dafuer genau
// einen Haken bereit - siehe ChefZ_CompNotice.
modded class ChefZ_CompNotice
{
    override void Emit()
    {
        super.Emit();
        ChefZ_Log.Banner("COT-Anbindung geladen, aber Community Online Tools ist nicht installiert - dieses Modul bleibt vollstaendig untaetig. Kein Fehler.");
    }
}
#endif // !JM_COT
