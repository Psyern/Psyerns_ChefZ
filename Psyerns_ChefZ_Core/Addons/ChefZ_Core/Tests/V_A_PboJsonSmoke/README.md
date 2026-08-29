# V-A — Rauchtest PBO-JSON (Runbook)

Vorarbeit aus `19 §2`. Beantwortet `OF-10` / `02 E7`: Ist eine JSON-Datei **innerhalb
eines Mod-PBO** zur Laufzeit über `FileExist` und `JsonFileLoader.LoadFile` lesbar?

Belegt ist bisher nur der `dz/`-Fall (`01 V8`, `CfgGameplayHandler.LoadData()`).

## 0. Erwartetes Ergebnis (Vorabrecherche, ersetzt die Messung nicht)

Drei ausgelieferte Fremdmods lösen einen Laufzeitpfad gegen das **PBO-Präfix ihres
eigenen Mod-PBO** auf — der Fall, den `01 V8` offen lässt:

| Mod | Stelle | Aufruf |
|---|---|---|
| DayZExpansion / NamalskAdventure | `LoadingScreen.c:37` | `JsonFileLoader<…>.JsonLoadFile("DayZExpansion/NamalskAdventure/Scripts/Data/LoadingImages.json", …)` |
| LBmaster_Core (`$prefix$` = `LBmaster_Core`) | `DayZGame.c:106` | `FileExist("LBmaster_Core/version/scripts/lb_version_check.c")` |
| NY_RadiationMod | `missiongameplayrad.c:23` | `FileExist("NY_RadiationMod/Video/Rad.mp4")` |

Dazu Vanilla `NotificationSystem.c:74,285`: `"scripts/data/notifications.json"` — ein
Pfad ohne `dz/`-Wurzel, aufgelöst gegen das Präfix des `scripts`-PBO.

Daraus die Erwartung: **P1 PASS, P2 FAIL.** Weicht die Messung davon ab, ist die
Messung wichtiger als diese Tabelle.

**Dieser Ordner ist temporär.** Nach dem Eintrag ins Gate-1-Protokoll werden gelöscht:

- `Addons/ChefZ_Core/Tests/V_A_PboJsonSmoke/`
- `Addons/ChefZ_Core/Config/ChefZ_ProbeData.json`
- `Addons/ChefZ_Core/Config/Probe/`
- der `files[]`-Eintrag auf diesen Ordner in `Addons/ChefZ_Core/config.cpp`
- die beiden mit `TEMPORAER: Vorarbeit V-A` markierten Aufrufe in
  `Addons/ChefZ_Core/Scripts/5_Mission/ChefZ/ChefZ_CoreEntry.c`

**Seit S1** bringt der Test keinen eigenen `modded class MissionServer` /
`MissionGameplay` mehr mit. Beide Seiten werden aus `ChefZ_CoreEntry` gestartet —
ein Einstiegspunkt je Seite im ganzen Modul.

## 1. PBO bauen

Der Build ist bereits gemessen; beide Varianten liefern die Testdaten ins PBO.
`<AB>` = `C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe`

**Variante A — ohne Binarisierung (schnell, für den Test ausreichend):**

```
"<AB>" "<repo>\Psyerns_ChefZ_Core\Addons\ChefZ_Core" "<out>" -clear -packonly -prefix=ChefZ_Core
```

**Variante B — wie später im Release (config.cpp wird zu config.bin):**

```
"<AB>" "<repo>\Psyerns_ChefZ_Core\Addons\ChefZ_Core" "<out>" -clear -prefix=ChefZ_Core -include=<abs>\include.txt
```

`include.txt` liegt seit S1 **dauerhaft** unter `Addons/ChefZ_Core/include.txt`
(die Kopie neben dieser Datei ist identisch und faellt mit dem Testordner weg —
verwende die dauerhafte). **Ohne `-include` verwirft der Binarize-Schritt
jede `.json`, jede `.c` und die `stringtable.csv` — im PBO bleibt nur `config.bin`
übrig.** Die Patterns müssen in **einer** Zeile stehen und mit `;` getrennt sein; eine
Datei mit einem Pattern je Zeile lässt AddonBuilder mit `Build failed` abbrechen.

Kontrolle des Ergebnisses: das PBO muss die Einträge
`Config\ChefZ_ProbeData.json`, `Config\Probe\ChefZ_ProbeNested.json` und
`Tests\V_A_PboJsonSmoke\Scripts\5_Mission\ChefZ\ChefZ_PboProbe.c` enthalten, und die
PBO-Eigenschaft `prefix` muss `ChefZ_Core` lauten.

## 2. Laden

`ChefZ_Core.pbo` nach `@ChefZ_Core\addons\` legen, Server mit `-mod=@ChefZ_Core`
starten. Kein Signaturschlüssel nötig, solange `verifySignatures=0`.

## 3. Ablesen

Der Probe läuft einmal in `MissionServer.OnInit` (Server) und einmal in
`MissionGameplay.OnInit` (Client) und schreibt einen Block nach `.RPT`:

```
[ChefZ][V-A] ===== Rauchtest PBO-JSON  seite=SERVER =====
[ChefZ][V-A] P1 prefix-root       exists=1 rawChars=89 load=1 marker=CHEFZ_V_A_OK items=2 verdict=PASS
...
[ChefZ][V-A] ===== Ende  seite=SERVER =====
```

Für die Clientseite denselben Mod im Client laden und im Client-RPT nach `seite=CLIENT`
suchen. Rang 2 wird laut `02 §3` von **beiden** Seiten gelesen; ein nur serverseitig
grüner Test beantwortet die Frage nur halb.

## 4. Sonden und ihre Bedeutung

| Sonde | Pfadform | Was ein FAIL bedeutet |
|---|---|---|
| P1 | `ChefZ_Core/Config/ChefZ_ProbeData.json` | **Die Kernfrage.** FAIL ⇒ Rang 2 als `config.cpp`-Seed (`02 E7`), `ChefZ_ConfigCppSource` statt `ChefZ_AddonJsonSource`. |
| P2 | `Psyerns/ChefZ_Core/Config/...` | **Gegenprobe, keine Messung mehr.** B4 ist entschieden (`02 §4.1`): Wurzel ist `$PREFIX$` = Ordnername. Erwartet ist **FAIL**. Ein PASS widerlegt die Entscheidung und muss gemeldet werden. |
| P3 | `.../Config/Probe/...` | FAIL ⇒ verschachtelte Datenpfade tragen nicht; Manifestdateien müssen flach liegen. |
| P4 | alles kleingeschrieben | PASS ⇒ Pfadvergleich ist case-insensitiv; FAIL ⇒ `chefzsym`/Manifest-Validator muss Schreibweisen exakt prüfen. |
| P5 | führender `/` | Nur Robustheitsinfo für die Pfadnormalisierung im Config Manager. |
| P7 | `ChefZ_Core\Config\...` | Beide Trennzeichen kommen in ausgelieferten Mods vor. Bestimmt, auf welche Form der Config Manager normalisiert. |
| P6 | `dz/worlds/<welt>/ce/cfggameplay.json` | **Kontrolle.** FAIL ⇒ die Messung selbst ist kaputt, P1–P5 sind wertlos. |
| E1 | `FindFile` über alle drei Flags | Erwartet `hits=0` (`01 V8`). `hits>0` wäre eine gute Nachricht und gehört genauso ins Protokoll. |

## 5. Protokolleintrag (ausfüllen und nach Gate 1 übertragen)

```text
V-A Rauchtest PBO-JSON
  Datum:            ____________
  DayZ-Serverbuild: ____________
  Build-Variante:   A (packonly) / B (binarisiert + include)
  Server: P1 ____  P2 ____  P3 ____  P4 ____  P5 ____  P6 ____  P7 ____  E1 hits=____
  Client: P1 ____  P2 ____  P3 ____  P4 ____  P5 ____  P6 ____  P7 ____  E1 hits=____
  Ergebnis:  Rang 2 wird als  [ ] Addon-JSON  [ ] config.cpp-Seed  implementiert.
  Loader-Meldungen bei FAIL:
```

Solange dieser Block leer ist, ist `S2` blockiert (`19 §3`).
