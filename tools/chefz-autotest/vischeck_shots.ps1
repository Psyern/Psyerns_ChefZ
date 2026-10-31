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
  if ($text.Contains("[VISCHECK] client done")) { break }
}
"shots: $count"
