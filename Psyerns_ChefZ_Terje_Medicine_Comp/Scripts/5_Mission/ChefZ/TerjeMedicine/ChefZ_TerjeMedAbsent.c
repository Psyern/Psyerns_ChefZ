//==============================================================================
// ChefZ_TerjeMedAbsent - die Gegenseite der weichen Abhaengigkeit.
//
// Gegenstueck zu ChefZ_TerjeMedStartupCheck.c: dort "#ifdef
// TERJE_MEDICINE_MOD", hier "#ifndef". Immer nur eine von beiden wird
// kompiliert, also gibt es aus diesem PBO nie zwei aktive
// "modded class MissionServer".
//
// ------------------------------------------------------------------------
// WICHTIG: DIESE ZEILE ERSETZT DIE STARTPRUEFUNG NICHT
// ------------------------------------------------------------------------
// ChefZ_TerjeMedStartupCheck meldet beim Serverstart, wie viele Eintraege aus
// CfgChefZTerjeMedicine kein Item im Hauptmod haben ("N von M Eintraegen ohne
// Item im Hauptmod"). Das ist ein bekannter, offener Befund
// (ChefZ_Wiki/Known-Limitations.md) und muss sichtbar bleiben.
//
// Er BLEIBT sichtbar: sobald TerjeMedicine geladen ist - und nur dann kann das
// Modul ueberhaupt wirken - laeuft die Startpruefung unveraendert und meldet
// unveraendert. Die Zeile hier erscheint ausschliesslich in dem Fall, in dem
// TerjeMedicine FEHLT; dann waere die Startpruefung ohnehin sinnlos, weil ohne
// TerjeMedicine keine einzige Wirkung angewendet werden koennte. Die
// Entkopplung verdeckt den Befund also nicht, sie verschiebt ihn auch nicht -
// sie beruehrt ihn gar nicht.
//
// ------------------------------------------------------------------------
// WARUM PrintToRPT UND NICHT TerjeLog_Warning
// ------------------------------------------------------------------------
// TerjeLog_* liegt in TerjeCore. Genau das ist hier nicht da. PrintToRPT ist
// eine Engine-Funktion, in jedem Skriptmodul jedes Mods vorhanden, und kann
// nicht selbst zur Fehlerquelle werden. Das Praefix ist woertlich das von
// ChefZ_Log (ChefZ_Log.c:35), damit ein Betreiber dieselbe Zeichenkette
// greppen kann wie sonst auch.
//
// Die Zeile ist bewusst KEINE Warnung und KEIN Fehler. Ein Comp-Mod ohne
// seinen Zielmod ist kein Defekt, sondern ein zulaessiger Betriebszustand.
//==============================================================================
#ifndef TERJE_MEDICINE_MOD
// Kein eigener "modded class MissionServer" mehr: zwei Comp-Module mit je
// einem eigenen Override haben den Server am 28.08.2026 mit einer
// Zugriffsverletzung beendet, jedes einzeln lief. Der Core stellt dafuer genau
// einen Haken bereit - siehe ChefZ_CompNotice.
// SCOUT-GEPRUEFT 2026-08-30 (chefz-conflict-scout)
// super.Emit() zuerst; komplementaer zu ChefZ_TerjeMedStartupCheck
// (#ifndef gegen #ifdef auf dasselbe Symbol) - nie beide im selben Build.
modded class ChefZ_CompNotice
{
    override void Emit()
    {
        super.Emit();
        ChefZ_Log.Banner("TerjeMedicine-Anbindung geladen, aber TerjeMedicine ist nicht installiert - dieses Modul bleibt vollstaendig untaetig. Kein Fehler.");
    }
}
#endif // !TERJE_MEDICINE_MOD
