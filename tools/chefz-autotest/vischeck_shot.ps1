# Bildschirmfotos des DayZDiag-Clients an den Markern von ChefZ_VisCheck.
# usage: powershell -File vischeck_shot.ps1 -Out <ordner> [-Prefix vis]
param([string]$Out, [string]$Prefix = "vis", [int]$Minutes = 12)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class VisW {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  public struct RECT { public int L, T, R, B; }
}
"@
$logDir = "E:\DayZ Modding\TestServer\profiles\client-130"
$start = Get-Date
$deadline = $start.AddMinutes($Minutes)
foreach ($stage in @("inventory", "world")) {
  $marker = "[VISCHECK] client $stage"
  $found = $false
  while ((Get-Date) -lt $deadline) {
    $f = Get-ChildItem $logDir -Filter "script_*.log" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if ($f -and $f.CreationTime -gt $start.AddMinutes(-2) -and (Select-String -Path $f.FullName -Pattern $marker -SimpleMatch -Quiet)) { $found = $true; break }
    Start-Sleep -Milliseconds 500
  }
  if (-not $found) { "timeout $stage"; exit 1 }
  Start-Sleep -Seconds 4
  $client = Get-Process DayZDiag_x64 -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 -and $_.MainWindowTitle -notmatch "Server|Console" } | Select-Object -First 1
  if (-not $client) { "no client window"; exit 1 }
  [VisW]::ShowWindow($client.MainWindowHandle, 9) | Out-Null
  [VisW]::SetForegroundWindow($client.MainWindowHandle) | Out-Null
  Start-Sleep -Seconds 1
  $r = New-Object VisW+RECT
  [VisW]::GetWindowRect($client.MainWindowHandle, [ref]$r) | Out-Null
  $w = $r.R - $r.L; $h = $r.B - $r.T
  $bmp = New-Object System.Drawing.Bitmap $w, $h
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
  $p = Join-Path $Out ("{0}_{1}.png" -f $Prefix, $stage)
  $bmp.Save($p)
  "saved $p"
}
