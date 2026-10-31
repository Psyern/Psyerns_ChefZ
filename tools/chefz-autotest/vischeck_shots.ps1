# Ein Foto des DayZDiag-Clients je Logzeile "[VISCHECK] shot <name>".
# usage: powershell -File vischeck_shots.ps1 -Out <ordner> [-Minutes 30]
# Endet bei "[VISCHECK] client done" oder nach der Frist.
param([string]$Out, [int]$Minutes = 30)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class VisS {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
  [DllImport("user32.dll")] public static extern void mouse_event(uint flags, int dx, int dy, uint data, UIntPtr extra);
  [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
  public struct RECT { public int L, T, R, B; }
}
"@
$logDir = "E:\DayZ Modding\TestServer\profiles\client-130"
$start = Get-Date
$deadline = $start.AddMinutes($Minutes)
$seen = @{}
$count = 0
while ((Get-Date) -lt $deadline) {
  Start-Sleep -Milliseconds 300
  $f = Get-ChildItem $logDir -Filter "script_*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
  if (-not $f -or $f.CreationTime -lt $start.AddMinutes(-1)) { continue }
  $stream = [System.IO.File]::Open($f.FullName, 'Open', 'Read', 'ReadWrite,Delete')
  $reader = New-Object System.IO.StreamReader($stream)
  $text = $reader.ReadToEnd()
  $reader.Close(); $stream.Close()
  foreach ($m in [regex]::Matches($text, "\[VISCHECK\] shot (\S+)")) {
    $name = $m.Groups[1].Value
    if ($seen.ContainsKey($name)) { continue }
    $seen[$name] = $true
    $client = Get-Process DayZDiag_x64 -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 -and $_.MainWindowTitle -notmatch "Server|Console" } | Select-Object -First 1
    if (-not $client) { "no client window"; exit 1 }
    [VisS]::ShowWindow($client.MainWindowHandle, 9) | Out-Null
    # Windows laesst nur nach einem Tastendruck das Fenster nach vorne holen (Alt).
    [VisS]::keybd_event(0x12, 0x38, 0, [UIntPtr]::Zero)
    [VisS]::SetForegroundWindow($client.MainWindowHandle) | Out-Null
    [VisS]::keybd_event(0x12, 0x38, 2, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 700
    if ([VisS]::GetForegroundWindow() -ne $client.MainWindowHandle) { "not foreground: $name" }
    $r = New-Object VisS+RECT
    [VisS]::GetWindowRect($client.MainWindowHandle, [ref]$r) | Out-Null
    $bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
    $bmp.Save((Join-Path $Out "$name.png"))
    $g.Dispose(); $bmp.Dispose()
    $count++
  }
  # "[VISCHECK] hold <name>": Taste W druecken, damit der Spieler vorwaerts laeuft.
  foreach ($m in [regex]::Matches($text, "\[VISCHECK\] hold (\S+)")) {
    $name = "hold_" + $m.Groups[1].Value
    if ($seen.ContainsKey($name)) { continue }
    $seen[$name] = $true
    $client = Get-Process DayZDiag_x64 -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 -and $_.MainWindowTitle -notmatch "Server|Console" } | Select-Object -First 1
    if (-not $client) { continue }
    [VisS]::ShowWindow($client.MainWindowHandle, 9) | Out-Null
    [VisS]::SetForegroundWindow($client.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 200
    [VisS]::keybd_event(0x57, 0x11, 0, [UIntPtr]::Zero)
    Start-Sleep -Seconds 5
    [VisS]::keybd_event(0x57, 0x11, 2, [UIntPtr]::Zero)
  }
  # "[VISCHECK] look <dy> <id>": Maus nach unten bewegen, um auf ein Ziel zu blicken.
  foreach ($m in [regex]::Matches($text, "\[VISCHECK\] look (-?\d+) (\S+)")) {
    $name = "look_" + $m.Groups[2].Value
    if ($seen.ContainsKey($name)) { continue }
    $seen[$name] = $true
    [VisS]::mouse_event(1, 0, [int]$m.Groups[1].Value, 0, [UIntPtr]::Zero)
  }
  # "[VISCHECK] wheel <x> <y> <rasten> <id>" und "click <x> <y> <id>": Maus auf dem Bildschirm.
  foreach ($m in [regex]::Matches($text, "\[VISCHECK\] wheel (\d+) (\d+) (-?\d+) (\S+)")) {
    $name = "wheel_" + $m.Groups[4].Value
    if ($seen.ContainsKey($name)) { continue }
    $seen[$name] = $true
    [VisS]::SetCursorPos([int]$m.Groups[1].Value, [int]$m.Groups[2].Value) | Out-Null
    Start-Sleep -Milliseconds 300
    for ($i = 0; $i -lt [Math]::Abs([int]$m.Groups[3].Value); $i++) {
      $delta = 120
      if ([int]$m.Groups[3].Value -lt 0) { $delta = -120 }
      [VisS]::mouse_event(0x0800, 0, 0, [uint32]($delta -band 0xFFFFFFFFL), [UIntPtr]::Zero)
      Start-Sleep -Milliseconds 80
    }
  }
  foreach ($m in [regex]::Matches($text, "\[VISCHECK\] click (\d+) (\d+) (\S+)")) {
    $name = "click_" + $m.Groups[3].Value
    if ($seen.ContainsKey($name)) { continue }
    $seen[$name] = $true
    [VisS]::SetCursorPos([int]$m.Groups[1].Value, [int]$m.Groups[2].Value) | Out-Null
    Start-Sleep -Milliseconds 300
    [VisS]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 300
    [VisS]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
  }
  # "[VISCHECK] press <F|LMB> <sekunden> <id>": Taste wirklich halten.
  foreach ($m in [regex]::Matches($text, "\[VISCHECK\] press (\S+) (\d+) (\S+)")) {
    $name = "press_" + $m.Groups[3].Value
    if ($seen.ContainsKey($name)) { continue }
    $seen[$name] = $true
    $client = Get-Process DayZDiag_x64 -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 -and $_.MainWindowTitle -notmatch "Server|Console" } | Select-Object -First 1
    if (-not $client) { continue }
    [VisS]::SetForegroundWindow($client.MainWindowHandle) | Out-Null
    Start-Sleep -Milliseconds 200
    if ($m.Groups[1].Value -eq "LMB") {
      [VisS]::mouse_event(2, 0, 0, 0, [UIntPtr]::Zero)
      Start-Sleep -Seconds ([int]$m.Groups[2].Value)
      [VisS]::mouse_event(4, 0, 0, 0, [UIntPtr]::Zero)
    } else {
      [VisS]::keybd_event(0x46, 0x21, 0, [UIntPtr]::Zero)
      Start-Sleep -Seconds ([int]$m.Groups[2].Value)
      [VisS]::keybd_event(0x46, 0x21, 2, [UIntPtr]::Zero)
    }
  }
  if ($text.Contains("[VISCHECK] client done")) { break }
}
"shots: $count"
