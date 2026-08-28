# testrun - startet den Testserver, wartet auf sein erstes Urteil und stoppt ihn.
#
# WARUM DIESES SKRIPT ZUERST DAS FENSTER LIEST UND DANN DIE PROTOKOLLE:
# Der DayZ-Server meldet einen Configfehler nicht ins RPT, sondern in einem
# modalen Fenster. Auf einem Server klickt das niemand weg, und von aussen sieht
# der Prozess aus, als haenge er - RPT bleibt beim Kopf stehen, keine
# Fehlerzeile, kein Beenden. Der Text steht ausschliesslich im Fenster. Genau
# daran wurde am 28.08.2026 stundenlang die falsche Ursache vermutet.
#
# Reihenfolge der Auswertung:
#   1. Fenstertext        -> Configfehler ("Undefined base class", fehlende Mods)
#   2. crash_*.log        -> Skript kompiliert nicht
#   3. script_*.log       -> kompiliert, aber mit Warnungen/Fehlern
#   4. RPT waechst weiter -> Start laeuft durch
#
# Der Server wird am Ende immer gestoppt. Dies ist eine Pruefung, kein Betrieb.

param(
  [string]$Deployment = "D:\Agent\deployments\DME-Test",
  # Diagnose-Schalter: nimmt Mods aus der Liste heraus, ohne das Deployment
  # anzufassen.
  [string[]]$OhneMods = @(),
  [int]$TimeoutSec = 180,
  # -------------------------------------------------------------------------
  # WARUM EIN EINZELLAUF KEIN URTEIL IST
  # -------------------------------------------------------------------------
  # Dieses Deployment stuerzt SPORADISCH ab, waehrend "[CE][Hive] :: Loading
  # core data" laeuft - mit wechselnder Fehleradresse, einmal mit
  # "Out of memory (requested -1346741 KB)". Gemessen am 29.08.2026: derselbe
  # unveraenderte Build ergab 1x Start und 2x Absturz; dieselbe Quote ohne
  # ChefZ_Cookbook. Der Fehler ist also vorbestehend und liegt nicht am Mod.
  #
  # Wer daraufhin einen Einzellauf als Beleg nimmt, halbiert Rauschen. Genau
  # das ist am 29.08.2026 ueber eine Stunde lang passiert: aus Einzellaeufen
  # entstanden drei Schluesse ("das 13. PBO", "eine Mod-Obergrenze",
  # "modded class ActionConstructor"), von denen KEINER haltbar war.
  #
  # Deshalb: fuer jede Aussage ueber Start oder Absturz mindestens 3 Laeufe.
  [int]$Wiederholungen = 1
)

$ErrorActionPreference = "Stop"

# Mehrere Laeufe: das Skript ruft sich selbst auf und zaehlt aus. Bewusst als
# eigene Prozesse - ein Absturz darf den Zaehler nicht mitnehmen.
if ($Wiederholungen -gt 1) {
  $ergebnisse = @()
  for ($n = 1; $n -le $Wiederholungen; $n++) {
    Write-Host "===== Lauf $n von $Wiederholungen ====="
    # *>&1 und nicht 2>&1: der Einzellauf berichtet mit Write-Host, und das
    # schreibt in den Informationsstrom, nicht nach stdout. Ohne die Umleitung
    # kommt hier nichts an und jeder Lauf zaehlte als "kein Urteil".
    $aus = & $PSCommandPath -Deployment $Deployment -OhneMods $OhneMods -TimeoutSec $TimeoutSec *>&1 | Out-String
    $zeile = ($aus -split "`r?`n" | Where-Object { $_ -like "ERGEBNIS*" } | Select-Object -First 1)
    if (-not $zeile) { $zeile = "ERGEBNIS: (kein Urteil)" }
    Write-Host "  -> $zeile"
    $ergebnisse += $zeile
  }
  $gestartet = ($ergebnisse | Where-Object { $_ -like "*GESTARTET*" }).Count
  Write-Host ""
  Write-Host "GESAMT: $gestartet von $Wiederholungen Laeufen gestartet."
  if ($gestartet -gt 0 -and $gestartet -lt $Wiederholungen) {
    Write-Host "  Sporadisch. Kein Beleg fuer eine Aenderung - siehe Kopf dieses Skripts."
  }
  exit 0
}

$sig = @"
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public class ChefZWin {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr h, EnumProc cb, IntPtr p);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowTextW(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassNameW(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  public static List<string> Found = new List<string>();
  public static uint Target = 0;
  static string Cls(IntPtr h) { StringBuilder c = new StringBuilder(256); GetClassNameW(h, c, c.Capacity); return c.ToString(); }
  static string Txt(IntPtr h) { StringBuilder t = new StringBuilder(4096); GetWindowTextW(h, t, t.Capacity); return t.ToString(); }
  static bool Child(IntPtr h, IntPtr p) { if (Cls(h) == "Static") { string s = Txt(h); if (s.Length > 0) Found.Add(s); } return true; }
  static bool Top(IntPtr h, IntPtr p) {
    uint pid; GetWindowThreadProcessId(h, out pid);
    if (pid == Target && Cls(h) == "#32770") EnumChildWindows(h, new EnumProc(Child), IntPtr.Zero);
    return true;
  }
  public static List<string> Scan(uint pid) { Found.Clear(); Target = pid; EnumWindows(new EnumProc(Top), IntPtr.Zero); return Found; }
}
"@
Add-Type -TypeDefinition $sig -Language CSharp

$profiles = Join-Path $Deployment "profiles"
$mods = "@3689057982;@2536780687;@2931560672;@2918418331;@2276010135;@2572331007;@2116157322;@1564026768;@2545327648;@1559212036;@3571685323;@3649957186;@3649958757;@3649957536;@3649959707;@1646187754;@3164839000;@2651195301;@1832448183;@1710977250;@1932611410;@2170927235;@3690289718;@3354681846;@2471347750;@3616635518;@3759357431;@3786175534;@3783149286;@3623510671;@3627848296;@3646233886;@3780383282;@3786176249;@ChefZ;"
foreach ($weg in $OhneMods) { $mods = $mods.Replace("@$weg;", "") }
if ($OhneMods.Count -gt 0) { Write-Host "Diagnose: ohne $($OhneMods -join ', ')" }

$serverArgs = @(
  "-config=serverDZ.cfg", "-port=2602", "-profiles=profiles",
  "-adminlog", "-netlog", "-freezeCheck", "-dologs",
  "-serverMod=@2464526692;", "-mod=$mods"
)

# Alles, was vor dem Start schon dalag, ausblenden - sonst wird ein alter Fund
# fuer einen neuen gehalten.
$before = @{}
foreach ($f in Get-ChildItem $profiles -File -ErrorAction SilentlyContinue) { $before[$f.Name] = $true }

$srv = Start-Process -FilePath (Join-Path $Deployment "DayZServer_x64.exe") `
  -ArgumentList $serverArgs -WorkingDirectory $Deployment -PassThru
Write-Host "Gestartet: PID $($srv.Id)"

$verdict = $null
$rptFile = $null
$scriptLog = $null
$boundAt = -1

for ($i = 0; $i -lt $TimeoutSec; $i++) {
  Start-Sleep -Seconds 1

  if ($srv.HasExited) { $verdict = "BEENDET  - Prozess endete mit Code $($srv.ExitCode)"; break }

  $dlg = [ChefZWin]::Scan([uint32]$srv.Id)
  if ($dlg.Count -gt 0) { $verdict = "CONFIGFEHLER (Fenster)`n  " + ($dlg -join "`n  "); break }

  $new = Get-ChildItem $profiles -File -ErrorAction SilentlyContinue | Where-Object { -not $before.ContainsKey($_.Name) }
  $crash = $new | Where-Object { $_.Name -like "crash_*.log" } | Select-Object -First 1
  # Frueher stand hier "SKRIPT KOMPILIERT NICHT". Das war falsch beschriftet und
  # hat teuer in die Irre gefuehrt: ein crash log sagt nur, dass der Prozess
  # gestorben ist, nicht warum - und auf diesem Deployment stirbt er auch ohne
  # jede ChefZ-Aenderung sporadisch. Die Beschriftung nennt jetzt den Fund,
  # nicht eine Deutung.
  if ($crash) { $verdict = "ABSTURZ (crash log) - Ursache unbestimmt, siehe -Wiederholungen im Kopf`n" + ((Get-Content $crash.FullName | Select-Object -Last 6) -join "`n  "); break }

  # Das Anlegen des script log ist KEIN Ende: es entsteht, sobald das erste
  # Skriptmodul etwas zu sagen hat, und danach kommen noch GameLib, Game,
  # World und Mission. Wer hier abbricht, prueft nur ein Fuenftel.
  if (-not $scriptLog) { $scriptLog = $new | Where-Object { $_.Name -like "script_*.log" } | Select-Object -First 1 }

  # Der gebundene Port ist das erste verlaessliche Zeichen, dass der Start
  # wirklich durch ist - unabhaengig davon, welche Zeile die Engine gerade
  # schreibt.
  # Der Port zaehlt nur, wenn UNSER Prozess ihn haelt. Ein gerade beendeter
  # Vorgaenger laesst den Socket kurz stehen; ohne die PID-Pruefung meldet der
  # Testlauf "gebunden" in Sekunde 0 und misst danach das Falsche.
  $bound = netstat -ano -p UDP | Select-String (":2602\s+.*\s" + $srv.Id + "$") | Select-Object -First 1
  if ($bound -and $boundAt -lt 0) { $boundAt = $i; Write-Host ("  {0,3}s  Port 2602 gebunden - Mission faehrt hoch" -f $i) }

  # NACH dem Portbinden noch zusehen. Der Start ist damit nicht vorbei: die
  # Mission initialisiert erst danach, und genau dort ist ChefZ am 28.08.2026
  # mit einem Stack overflow ausgestiegen, nachdem alle fuenf Skriptmodule
  # fehlerfrei uebersetzt waren. Wer beim Portbinden aufhoert, nennt das einen
  # gelungenen Start.
  if ($scriptLog) {
    $vm = Select-String -Path $scriptLog.FullName -Pattern "Virtual Machine Exception" -Quiet -ErrorAction SilentlyContinue
    if ($vm) {
      $ctx = Get-Content $scriptLog.FullName | Select-String -Pattern "Virtual Machine Exception" -Context 0, 6 | Select-Object -First 1
      $verdict = "LAUFZEITFEHLER`n  " + ($ctx.ToString() -split "`n" | Select-Object -First 8 | ForEach-Object { $_.Trim() }) -join "`n  "
      break
    }
  }
  if ($boundAt -ge 0 -and ($i - $boundAt) -ge 75) {
    $verdict = "GESTARTET - Port gebunden, Mission $($i - $boundAt) s ohne Laufzeitfehler"
    break
  }

  # RPT-Wachstum ist KEIN Urteil, sondern nur ein Lebenszeichen. Der
  # Skriptcompiler laeuft erst, nachdem alle Pakete eingehaengt sind; wer hier
  # abbricht, weil sich etwas tut, erfaehrt ueber die Skripte gar nichts.
  if (-not $rptFile) { $rptFile = $new | Where-Object { $_.Name -like "*.RPT" } | Select-Object -First 1 }
  if ($rptFile -and ($i % 20) -eq 0) {
    $n = (Get-Content $rptFile.FullName | Measure-Object -Line).Lines
    Write-Host ("  {0,3}s  RPT {1} Zeilen" -f $i, $n)
  }
}

if (-not $verdict) { $verdict = "ZEITUEBERSCHREITUNG nach $TimeoutSec s - weder Fenster noch Protokoll" }

Write-Host ""
Write-Host "ERGEBNIS: $verdict"

# Das script log immer auswerten, auch wenn der Start durchlief: eine Warnung,
# die niemand liest, ist keine bestandene Pruefung. Gefiltert auf ChefZ - die
# uebrigen 35 Mods sind hier nicht die Frage.
if (-not $scriptLog) {
  $scriptLog = Get-ChildItem $profiles -Filter "script_*.log" -ErrorAction SilentlyContinue |
               Where-Object { -not $before.ContainsKey($_.Name) } | Select-Object -First 1
}
if ($scriptLog) {
  $ours = Get-Content $scriptLog.FullName | Where-Object { $_ -match "(?i)chefz" }
  $err  = $ours | Where-Object { $_ -match "\(E\)" }
  $warn = $ours | Where-Object { $_ -match "\(W\)" }
  Write-Host ""
  Write-Host "ChefZ im $($scriptLog.Name): $($err.Count) Fehler, $($warn.Count) Warnungen"
  foreach ($l in ($err | Select-Object -First 10)) { Write-Host "  E $l" }
  $kinds = $warn | ForEach-Object { if ($_ -match "FIX-ME: ([^']+)") { $matches[1].Trim() } else { "sonstige" } } |
           Group-Object | Sort-Object Count -Descending | Select-Object -First 6
  foreach ($k in $kinds) { Write-Host ("  W {0,4}x {1}" -f $k.Count, $k.Name) }
}

if (-not $srv.HasExited) { Stop-Process -Id $srv.Id -Force -ErrorAction SilentlyContinue }
Write-Host "Server gestoppt."
