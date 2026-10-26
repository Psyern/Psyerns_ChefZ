<#
	import-delivery - baut eine Modell-Lieferung (FBX + Farb-PNG) mit der Psyerns 3D Suite zu
	DayZ-Modellen und bindet sie an die ChefZ-Klassen.

	Ablauf (Details in tools/chefz-assets/DELIVERY_WORKFLOW.md):
	  1. Blender headless + Suite (tools/import_delivery.py): Import, Spielgroesse, Ursprung am
	     Boden, PNG -> PAA (+ Garzustaende), Selection, AUTO BUILD (LODs, Geometry, Memory ...)
	     -> ChefZ/<Paket>/<gruppe>/models/*.p3d und data/*.paa
	  2. bind-delivery.py: Vorschau (oder mit -Apply: kopieren nach ChefZ_Core/Addons und
	     model=/hiddenSelections in den Klassen setzen)

	Aufruf:
	  powershell -ExecutionPolicy Bypass -File tools\chefz-assets\import-delivery.ps1 -Manifest tools\chefz-assets\deliveries\2026-10-25_sealife.json
	  ... -Only BlueMarlin,TunaFillet      nur diese Assets bauen
	  ... -Group fish                      nur eine Gruppe
	  ... -SkipBuild -Apply                Bericht vorhanden, nur binden
#>
param(
	[Parameter(Mandatory = $true)][string]$Manifest,
	[string]$Only = "",
	[string]$Group = "",
	[switch]$SkipBuild,
	[switch]$Apply,
	[string]$Suite = "E:\DayZ Modding\Psyerns_3D_Suite_For_Blender"
)
$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$manifestPath = (Resolve-Path $Manifest).Path
$report = [IO.Path]::ChangeExtension($manifestPath, ".report.json")

if (-not $SkipBuild) {
	$runner = Join-Path $Suite "tools\import_delivery.py"
	if (-not (Test-Path $runner)) { throw "Suite-Runner fehlt: $runner" }
	# neueste installierte Blender-Version (die Suite braucht 4.5+)
	$blender = Get-ChildItem "C:\Program Files\Blender Foundation" -Directory -ErrorAction SilentlyContinue |
		Sort-Object { [version]($_.Name -replace '[^\d.]', '') } -Descending |
		ForEach-Object { Join-Path $_.FullName "blender.exe" } | Where-Object { Test-Path $_ } | Select-Object -First 1
	if (-not $blender) { throw "Kein Blender unter C:\Program Files\Blender Foundation gefunden" }
	$runArgs = @("--background", "--factory-startup", "--python", $runner, "--", $manifestPath, "--report", $report)
	if ($Only) { $runArgs += @("--only", $Only) }
	if ($Group) { $runArgs += @("--group", $Group) }
	Write-Host "[build] $blender" -ForegroundColor Cyan
	& $blender @runArgs 2>&1 | Where-Object { $_ -match "^\[delivery\]" } | ForEach-Object { Write-Host $_ }
	if ($LASTEXITCODE -ne 0) { throw "Bau meldet Fehler (siehe $report)" }
}

$bindArgs = @((Join-Path $PSScriptRoot "bind-delivery.py"), $report)
if ($Apply) { $bindArgs += "--apply" }
Write-Host "[bind] $report" -ForegroundColor Cyan
& py -3 @bindArgs
exit $LASTEXITCODE
