# Modell-Lieferungen mit der Psyerns 3D Suite nach DayZ

Ziel: Eine Lieferung aus FBX und Farb-PNG (z. B. von Meshy, alles auf 1 m normiert) wird
ohne Object Builder zu DayZ-Modellen, die an die ChefZ-Klassen gebunden und im Spiel
geprüft sind.

```
Lieferung (ZIP)  ->  Manifest  ->  3D Suite (Blender headless)  ->  ChefZ/<Paket>/<gruppe>/{models,data}
                                                                  ->  bind-delivery.py: ChefZ_Core/Addons/<Paket> + model=/hiddenSelections
                                                                  ->  Ninjins Hub packen  ->  DayZDiag 1.30 Spawn-Test
```

## Voraussetzungen

| Was | Wo |
|---|---|
| Blender 4.5+ (getestet 5.2) | `C:\Program Files\Blender Foundation\Blender 5.x` |
| Psyerns 3D Suite (Repo) | `E:\DayZ Modding\Psyerns_3D_Suite_For_Blender` (Runner `tools/import_delivery.py`) |
| DayZ Tools (ImageToPAA) | Steam, `PSY_DAYZ_TOOLS` überschreibt den Pfad |
| Python 3 (`py -3`) | für `bind-delivery.py`; Node ist **nicht** nötig |
| Ninjins Environment Hub + TestServer | nur für den Spieltest |

## 1. Lieferung entpacken

Außerhalb des Repos ablegen (Gigabytes, gehört nicht ins Git), verschachtelte ZIPs flach je
Ordner, `__MACOSX` weglassen. Die Lieferung vom 25.10.2026 liegt in
`E:\DayZ Modding\Deliveries\2026-10-25_3D_Assets`.

Achtung bei Meshy-Lieferungen: Die Ziffern `1`/`2` im Dateinamen sagen nichts. Mal ist `X1`
das FBX und `X2` die Textur, mal umgekehrt, und zwei FBX hatten gar keine Endung
(`AntiqueSpoonFlasher1`, `tomato_sauce1`). Der Runner erkennt FBX am Dateikopf.

## 2. Manifest schreiben

`deliveries/<datum>_<name>.json`, Vorlage: `deliveries/2026-10-25_sealife.json`.

| Feld | Bedeutung |
|---|---|
| `source_root` | entpackte Lieferung |
| `project_root` | Repo-Root (P:-Ersatz): Texturpfade in den P3D sind relativ dazu (`ChefZ\ChefZ_Sealife\fish\data\X_co.paa`) |
| `groups.<g>.target` | Paketpfad, z. B. `ChefZ\ChefZ_Sealife\fish` → `ChefZ/ChefZ_Sealife/fish/models/*.p3d` |
| `length` | Spiellänge der längsten Achse in m (die Lieferung ist auf 1 m normiert) |
| `rotate` | optional `[x, y, z]` in Grad, vor dem Skalieren |
| `selection` | `camo` (Standard) oder `cs_raw` für Filets (Garzustände über `hiddenSelectionsTextures`) |
| `states` | getönte Kopien der Farbtextur, Filets: `baked, boiled, dried, burnt` |
| `template` | Suite-Template für AUTO BUILD: `food` (Standard), `item` (Köder) |
| `bind` | ChefZ-Klassen, deren Modell ersetzt wird; leer = nur bauen |
| `note` / `skip` | unsichere Zuordnung / nicht bauen (mit Grund) |
| `hold` | Rahmen der Handhaltung: `1h` (wie Vanillas `carp_live`: Länge senkrecht, Kopf oben, Ursprung am Schwanzstiel), `2h` (wie `wooden_plank`: Länge quer, Ursprung in der Mitte), `heavy` (wie `wooden_log`: Länge nach vorn). Ohne `hold` bleibt der Ursprung unten mittig |

### Handhaltung

Die Engine setzt ein Item mit seinem **Modellursprung** in die Hand und spielt die `.anm`
der nächsten registrierten Elternklasse ab. Jede `.anm` ist gegen ein bestimmtes
Vanilla-Modell gestellt. Ein eigenes Modell sitzt nur dann richtig, wenn es
in Ursprung und Achsen zu diesem Modell passt. Deshalb gehören zwei Dinge zusammen:

- `hold` im Manifest: Der Runner dreht und verschiebt das Modell in den Rahmen. Bei `1h` erkennt
  er den Schwanz am dünneren Querschnitt.
- Der Eintrag in `ChefZ_Core/Addons/ChefZ_Core/Scripts/4_World/ChefZ/ChefZ_HandProfiles.c`
  (`modded class ModItemRegisterCallbacks`) mit derselben Gruppe.

`itemBehaviour` in der Config steuert nur Aufheben, Tragen und Platzieren, nicht den Griff.
Fehlt es an einer platzierbaren Station, legt sich der Spieler beim Platzieren hin
(`ActionDeployObject.SetupAnimation` lässt den Befehl auf 0 = `CMD_ACTIONFB_DRINK` stehen).

## 3. Bauen und Vorschau

```powershell
powershell -ExecutionPolicy Bypass -File tools\chefz-assets\import-delivery.ps1 -Manifest tools\chefz-assets\deliveries\2026-10-25_sealife.json
#   -Only BlueMarlin,TunaFillet   einzelne Assets      -Group fish   eine Gruppe
```

Je Asset laufen die echten Suite-Operatoren: `psy.model_import_file` → Größe und Ursprung am
Boden → PNG auf 1024² → `ImageToPAA` (+ Garzustände) → Selection als Vertex-Gruppe →
`AUTO BUILD` des Templates (Resolution-LOD 0–3, Geometry-Hülle mit Masse, Shadow Volume, Land
Contact, Fire/View Geometry, Memory `ce_center`/`invView`, Named Properties, Validierung).
Ein offenes Shadow Volume (die Lieferungen sind selten wasserdicht) wird durch die Hülle
ersetzt. Geschrieben werden nur `.p3d` und `.paa` - **keine config.cpp**.

Ergebnis: `deliveries/<manifest>.report.json`, danach automatisch die Bindungs-Vorschau (Diff).

## 4. Binden

```powershell
... import-delivery.ps1 -Manifest <manifest> -SkipBuild -Apply
```

`bind-delivery.py --apply` kopiert die gebauten Dateien nach `ChefZ_Core/Addons/<Paket>/` (wie
`sync-assets.mjs`) und setzt in jeder Klasse aus `bind` die Zeilen `model=`,
`hiddenSelections[]` und `hiddenSelectionsTextures[]`. Die Texturen sind Pflicht: Die
Filet-Basen vererben `cs_raw` mit Karpfen- und Makrelen-Texturen. Wer nur `model=` umbiegt,
legt die auf das UV-Layout des neuen Modells.

## 5. Im Spiel prüfen (Ninjins Hub + DayZDiag 1.30)

1. **Alle** Addons aus `ChefZ_Core/Addons/` nach `TestServer\src\` kopieren. Die Slices
   haben Scripts und brauchen `ChefZ_Core`, ein Teilsatz kompiliert nicht. Den Präfix liest
   der Hub aus `$PREFIX$`.
2. Mission und Prüf-Addon erzeugen. Dafür eine Kopie von `dayzOffline.chernarusplus` anlegen
   (z. B. `chefztest.chernarusplus`), dann:
   `py -3 tools\chefz-assets\make-dayz-test.py <report.json> "E:\DayZ Modding\TestServer\src" <Missionskopie>`
   Das schreibt `src\PSY_SealifeCheck` (je ungebundenes Modell eine Klasse `PSY_Check_<Name>`)
   und eine `init.c`, die jede gebundene ChefZ-Klasse und jede Prüfklasse spawnt.
3. Packen mit dem Hub, **mit JSON**: `py -3 tools\chefz-assets\hub-pack-test.py`.
   Der Hub überspringt beim Kopieren `.json`, `.md` und `.txt`. Ohne die JSON-Dateien startet
   ChefZ im SAFE MODE: keine Stationsaktionen (Butterfass, Käsepresse, Räucherschrank,
   Honigschleuder), keine Ernte, keine Köderwirkung. Im Log steht dann `health=SAFE_MODE` und
   „Im Manifest genannte Datei ist nicht lesbar“. Das Skript setzt die Ausschlussliste für den
   Aufruf auf `.md`. Zusätzlich umgeht es den Fehler der Hub-Kommandozeile (`--mods` sucht
   `src`, bevor `paths.json` gelesen ist). Der Hub-Build beendet laufende DayZ-Prozesse, also
   vorher prüfen, ob jemand anderes den TestServer benutzt.
4. `Start-TestServer.ps1 -Mission <Missionskopie>`. Der Server beendet sich nach der Prüfung selbst.
5. Im Script-Log steht je Klasse `[PSYTEST] <Klasse> OK type=... shape=<p3d>`, dazu die
   Kollisionsbox, und am Ende `[PSYTEST] done`. Im RPT stehen Fehler zu `ChefZ_Sealife`.

Ergebnis der Lieferung vom 25.10.2026 (DayZDiag 1.30.164.200): 61 von 61 Klassen spawnen mit
dem eigenen Modell. Im RPT steht kein Fehler zu den Modellen oder Texturen.

Nie gegen einen Server mit 1.29 oder älter testen: Die DayZ Tools stehen seit dem 25.10.2026
auf 1.30 und schreiben ODOL v56, an dem ein 1.29-Server mit ErrorMessage-mdmp abstürzt. Seit dem
26.10.2026 verweigert jeder lokale Startweg einen Server unter 1.30 oder hinter dem
Stable-Client (Versionssperre, siehe `TestServer\README.md`). Der Hub packt die MLOD-Dateien
unverändert, und DayZ 1.30 lädt sie.

## Was der Workflow nicht kann

- Sichtprüfung (Texturen, Normalen, Garzustände) - dafür den Diag-Client starten.
- Klassen für Arten ohne ChefZ-Klasse anlegen (`bind` leer). Das ist eine Inhaltsentscheidung
  der Slices.
- Normal- und Rauheitskarten der Lieferung (eingebettet im FBX) werden noch nicht genutzt,
  es gibt nur `_co`.
