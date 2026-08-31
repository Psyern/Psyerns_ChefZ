# Slice `wildplants` — Wildwuchs

Spec: `Psyerns_ChefZ_Docs/ChefZ_Wildwuchs_Spawn_Plan.md` (freigegeben
31.08.2026). Löst den Gate-2-Befund **G2-B9** — Mais, Thymian, Rosmarin und
Petersilie hatten bis dahin keine einzige Quelle in der Welt.

Der Text steht **neben** der JSON-Datei und nicht in ihr. Grund ist derselbe,
den `README.md` und `README_Apiary.md` festhalten:
`ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()` belegt die Toleranz des
Enforce-Serializers gegenüber unbekannten JSON-Feldern **nicht** — ein
`_comment` könnte die ganze Datei unlesbar machen.

---

## Funktionsweise in fünf Sätzen

1. Die Central Economy stellt spielerzentriert (`position=player`, wie bei
   Vanillas Pilzen) stehende Pflanzen in den Ring 25–100 m um jeden Spieler.
2. Eine Wildpflanze ist eine **Mini-Station** (`ChefZ_WildPlant_Base extends
   ChefZ_ProcessingStation_Base`) — kein `PlantBase`, kein Beet, kein Wachstum.
3. Sie lässt sich **nicht aufheben** und **nicht umstellen**; die einzige Aktion
   an ihr ist `PROCESS_HARVEST_WILD` (STATION_ACTION, 5 s, ohne Werkzeug).
4. Beim Abschluss würfelt der Server die Ausbeute, legt sie neben die Pflanze
   und **löscht die Pflanze**.
5. Die CE stellt anderswo eine neue hin, weil das `nominal` des Events wieder
   unterschritten ist.

Kein Core-Code, keine eigene Action, kein `modded class`. Alles ist gewöhnliche
Vererbung auf `ChefZ_ProcessingStation_Base` — dieselbe Bauform, mit der der
Bienenstock arbeitet.

---

## Ausbeute (Spec Kap. 5)

| Pflanze | sicher | +1 | +2 | Ertragsklasse |
|---|---:|---:|---:|---|
| `ChefZ_WildCorn` | 1 | 25 % | 5 % | `ChefZ_Corn` |
| `ChefZ_WildThyme` | 1 | 25 % | — | `ChefZ_Thyme` |
| `ChefZ_WildRosemary` | 1 | 25 % | — | `ChefZ_Rosemary` |
| `ChefZ_WildParsley` | 1 | 25 % | — | `ChefZ_Parsley` |

**Ein Wurf, zwei Bänder** (`ChefZ_WildPlant_Base.ChefZ_RollBonus()`):
`Math.RandomIntInclusive(0, 99)`, `roll < twoPct` → +2, `roll < twoPct + onePct`
→ +1, sonst nichts. Beim Mais (5 / 25) ergibt das exakt 5 % / 25 % / 70 %.
Zwei getrennte Würfe hätten die Bänder übereinandergelegt und im Mittel mehr
ausgeschüttet, als die Tabelle sagt.

**Mehr Ertrag heißt mehr ITEMS, nie mehr Quantity.** Alle vier Ertragsklassen
führen `varQuantityInit = 1` und `varQuantityMax = 1` (`config.cpp`,
`ChefZ_VegetableFood_Base` und `ChefZ_FreshHerbBase`) — der Klassendefault *ist*
der eine Kolben bzw. das eine Bund. Eine `2` an einem Item mit `varQuantityMax 1`
wäre still auf 1 geklemmt worden, und der Wurf hätte nichts bewirkt.

Kein Werkzeugeinfluss, kein Terje-Einfluss, kein XP — Entscheidung „nur Würfel"
(Spec Kap. 1 und 5).

---

## Warum `PROCESS_HARVEST_WILD` keinen Transform hat

Die Spec sah „je Pflanze ein Transform" vor. Das ist mit dem Core in seinem
heutigen Stand nicht baubar — **drei unabhängige Gründe, jeder einzelne genügt**:

| # | Beleg | Was er sagt |
|---|---|---|
| 1 | `ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_TransformDef.c:173-178` | Ein Transform **ohne `inputs` wird abgewiesen**: „Er hätte keine Bedingung und würde damit auf jeden Stationsinhalt passen, auch auf einen leeren." |
| 2 | `ChefZ_Core/Scripts/4_World/ChefZ/ChefZ_FactCollector.c:191-196` | `CollectFromCargo` kehrt bei einem Behälter **ohne Cargo** mit leerem Schnappschuss zurück. Eine Wildpflanze hat kein Cargo — ein Eingang könnte nie binden. |
| 3 | `ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ProcessRunner.c:165-166` | Der Applicator erzeugt „ausschließlich in den **CARGO EINES GEFÄSSES**". Wohin der Kolben fällt, könnte ein Transform gar nicht sagen. |

Der Vorgang trägt deshalb **keinen** Transform — wie `PROCESS_HARVEST_HIVE` und
`PROCESS_PACK_HIVE`, aus derselben Sorte Grund.
`ChefZ_ActionProcessAtStation.IsProcessUsable()` überspringt die
Transformprüfung, wenn zu einem Prozess kein Transform bekannt ist
(`ChefZ_ActionProcessAtStation.c:321-324`); `RunImmediate` meldet dann
`NO_MATCH`, und `NotifyStation` ruft den Haken trotzdem.

**Folge für den Haken:** `ChefZ_StationActionOutcome.IsSuccess()` darf hier
*nicht* gefragt werden — der Helfer kennt nur `APPLIED` und `JOB_STARTED` als
Erfolg, und beide setzen einen Transform voraus. Erfolglos ist allein
`RUN_FAILED`. Wer hier `IsSuccess()` abfragte, baute eine Ernte, die nie erntet.

**Folge für die Dateien:** Es gibt kein `WildPlants.json` mit Transforms.
`WildPlant_Stations.json` ist die einzige Datei dieses Slice.

---

## Nicht aufnehmbar — was das wirklich bewirkt

`canBeDigged = 0` in der `config.cpp` leistet es **nicht**: der Schlüssel
betreibt Vanillas vergrabene Verstecke. Es leistet
`ChefZ_WildPlant_Base.IsTakeable()` → `false`:

- `ItemBase` setzt `m_IsTakeable` im Konstruktor auf `true`
  (`scripts - 1.29`, `ItemBase.c:229`) und gibt es in `IsTakeable()` zurück
  (`ItemBase.c:4398-4401`).
- `ActionTakeItem.ActionCondition` fragt genau diesen Haken:
  `if (tgt_item && !tgt_item.IsTakeable()) return false;`
  (`ActionTakeItem.c:34`).

Übernommen ist der vollständige Satz, mit dem Vanilla ein unaufnehmbares
Weltobjekt beschreibt — `GardenPlot.c:113-131`: `IsTakeable`, `CanPutInCargo`,
`CanRemoveFromCargo`, `CanPutIntoHands`, alle `false`.

### Und nicht umstellbar

`ChefZ_ProcessingStation_Base.SetActions()` bringt `ActionTogglePlaceObject` und
`ActionPlaceObject` mit, `IsDeployable()` antwortet dort „ja". Für einen
Fleischwolf ist das richtig, für etwas, das aus dem Boden wächst, nicht.

`ChefZ_WildPlant_Base` überschreibt `IsDeployable()` → `false` (`Hologram.c:252`
fragt genau diesen Haken) **und** nimmt vier Aktionen wieder aus dem Menü:
`ActionTakeItem`, `ActionTakeItemToHands`, `ActionTogglePlaceObject`,
`ActionPlaceObject`.

**Warum `super` trotzdem gerufen wird:** Enforce kann keine Vererbungsebene
überspringen — `super` ruft immer die direkte Elternklasse. Ohne `super` gäbe es
außerdem keine `ChefZ_ActionProcessAtStation` mehr und damit keine Ernte. Also
Vanillas eigener Weg: `super` rufen und wieder herausnehmen, genau wie
`BaseBuildingBase.SetActions()` (`BaseBuildingBase.c:1245-1251`) und
`Pot.c:133-137`.

---

## Cluster: warum die Maisgruppe im Skript entsteht

Der eine offene Verifikationspunkt der Spec (Kap. 3) ist geklärt — **negativ**.
Ein `position=player`-Event kann keine Gruppe an einem Ort setzen. Belegt an
`D:\Agent\deployments\DME-Test\mpmissions\dayzOffline.chernarusplus`:

- Von 75 Events in `db\events.xml` haben 25 `position=player`: acht
  `Trajectory*` und siebzehn `Infected*`. **Keines** setzt mehrere Objekte an
  einen Ort.
- Die `Trajectory*` tragen an ihren Kindern durchweg `min="0" max="0"`.
- Die `Infected*` tragen dort Zahlen, aber als **Gewichte** einer Auswahl:
  `InfectedNBC` hat ein einziges Kind mit `min="100"`, `InfectedPrisoner`
  ebenso, `InfectedPolice` verteilt 40/40/20/20. Prozente, keine Stückzahlen.
- Echte Ortsgruppen kennt die CE über `cfgeventgroups.xml`. Deren Kinder tragen
  **feste Relativkoordinaten** (`x`/`y`/`z`/`a`) und werden über
  `cfgeventspawns.xml` an eine **feste Weltposition** gebunden
  (`<pos … group="…"/>`, ab Z.8500). Ein `position=player`-Event steht in
  `cfgeventspawns.xml` überhaupt nicht — die beiden Mechanismen schließen
  einander aus.

Deshalb der Fallback der Spec: `ChefZ_WildCorn.EEOnCECreate()` würfelt 0–2
Begleiter (gleichverteilt) und stellt sie über `g_Game.CreateObjectEx(…,
ECE_PLACE_ON_SURFACE)` 1–2 m daneben. Begleiter sind **dieselbe Klasse** und
tragen damit dieselbe `types.xml`-Zeile — dieselbe `lifetime`, dieselbe
Zählung. Diese Zusage kostet keine Zeile Code; sie folgt daraus, dass es
dieselbe Klasse ist.

**Rekursionswache:** ein statisches Flag um die Erzeugungsschleife herum
(`ChefZ_WildPlant_Base.s_ChefZ_SpawningCompanions`). Statisch und nicht am
Objekt, weil die Gefahr am Aufrufzeitpunkt hängt und nicht am erzeugten Objekt:
liefe `EEOnCECreate` synchron *innerhalb* von `CreateObjectEx`, gäbe es das
Objekt noch gar nicht, an dem ein Instanzflag stehen könnte.

Dass der Fall wahrscheinlich gar nicht eintritt, ist geprüfte Nebensache und
kein Ersatz für die Wache: `EEOnCECreate` ist „called when entity is being
created as new by CE/ Debug" (`EntityAI.c:1385-1388`), und der volle Aufbau
hängt an `ECE_SETUP` („process full entity setup", `CentralEconomy.c:8`).
`ECE_PLACE_ON_SURFACE` ist 1060 = `ECE_CREATEPHYSICS|ECE_UPDATEPATHGRAPH|
ECE_TRACE` (`CentralEconomy.c:37`) — `ECE_SETUP` ist **nicht** darin.

**Folge für die Zählung:** Die tatsächliche Zahl Maispflanzen liegt im Mittel
beim Doppelten des `nominal`.

---

## Modelle

| Klasse | Modell | Art |
|---|---|---|
| `ChefZ_WildCorn` | `\ChefZ\ChefZ_Plants\models\corn_plant.p3d` | eigenes Modell, stehend |
| `ChefZ_WildThyme` | `\dz\gear\cultivation\plant_material.p3d` | **Proxy** |
| `ChefZ_WildRosemary` | `\ChefZ\ChefZ_Plants\models\rosmary.p3d` | **Proxy** (Item-Mesh der Ernte) |
| `ChefZ_WildParsley` | `\ChefZ\ChefZ_Plants\models\parsley.p3d` | **Proxy** (Item-Mesh der Ernte) |

Drei stehende Kräuterbüschel und ein Thymian-Item-Mesh sind als Asset-Bedarf
gemeldet.

**Nicht genommen:** Vanillas Clutter-Modelle (`\dz\plants\clutter\c_*.p3d`),
obwohl sie am hübschesten aussähen. Clutter ist Bodenbewuchs und trägt
möglicherweise **keine Geometry-LOD** — ohne die trifft der Raycast der
Aktionszielsuche nichts, und die Ernteaktion erschiene nie. Ein Fehler, der wie
„die Pflanze ist kaputt" aussieht und in keinem Log steht. Der Versuch steht als
Gate-Experiment im Asset-Backlog; hier stehen Meshes, die im Projekt bereits als
Item am Boden benutzt werden und damit nachweislich eine Geometrie haben.

### Die sechs Wuchsstufen von `corn_plant.p3d`

`corn_plant.p3d` hängt an `PlantBaseSkeleton`; `ChefZ_Plants/models/model.cfg`
führt vierzehn Animationen vom Typ `hide` mit `source="user"` — `Pile_01/02`,
`PlantStage_01..06` und deren `_crops`. Eine solche Auswahl ist **sichtbar**,
solange ihre Phase 0 ist. Sichtbar geschaltet werden sie in Vanilla
ausschließlich von `PlantBase.UpdatePlant()` (`PlantBase.c:448-479`, über
`ShowSelection`/`HideSelection`), aufgerufen aus `GrowthTimerTick`,
`OnStoreLoadCustom` und der Ernte. Eine Wildpflanze ist kein `PlantBase` und
ruft das nie — ohne Gegenmaßnahme stünden alle sechs Stufen ineinander.

**Zwei Antworten, beide gesetzt:**

1. `class AnimationSources` an `ChefZ_WildCorn` (config.cpp) setzt `initPhase`
   je Auswahl: 1 = verborgen, 0 = sichtbar. Sichtbar bleibt genau
   `PlantStage_06` und `PlantStage_06_crops`. Das ist der Weg, den die Engine
   ohne jedes Skript geht, und er greift auf jedem Client, der das Objekt
   hereinstreamt. Form belegt an `DayZExpansion/Objects/Airdrop/config.cpp:23-35`
   und `.../SupplyCrates/config.cpp:66-74`.
2. `ChefZ_WildCorn.ChefZ_ApplyModelStage()` setzt dieselben vierzehn Phasen beim
   Erscheinen noch einmal, aufgerufen aus `EEInit` — also auf **Server und
   Client**, denn `ShowSelection`/`HideSelection` sind lokal
   (`EntityAI.c:3356-3371`, `SetAnimationPhase`) und gehen nicht über die
   Leitung.

Beide hängen an derselben Configzeile: `HideSelection` verlangt ausdrücklich
einen Eintrag in `class AnimationSources` (Kommentar an
`EntityAI.HideAllSelections`, `EntityAI.c:3354`). Keiner der Wege ersetzt den
anderen.

> **Offen bis zur Sichtprüfung im Spiel.** Dass ein Nicht-`PlantBase`-Objekt
> diese Auswahlen schalten darf, ist aus den Quellen plausibel
> (`SetAnimationPhase` sitzt auf `Entity`, nicht auf `PlantBase`), aber nicht
> durch ein laufendes Beispiel belegt. Steht der Mais im Gate unsichtbar oder
> sechsfach da, ist das eine Modell-/Configfrage und keine Skriptfrage.

---

## CE-Einbau

Die Wildpflanzen erscheinen erst, wenn ein Mensch das CE-Fragment in die Mission
einbaut. Vorlagen und Anleitung:

`Psyerns_ChefZ_Core/Addons/ChefZ_Farming/ServerConfig/README_ServerConfig.md`

Diese Dateien wandern **nicht** ins PBO: `include.txt` dieses Moduls führt seit
dem 31.08.2026 kein `*.xml` mehr (das Modul hat kein XML, das ins PBO gehört),
und `*.md` fällt ohnehin unter kein Muster.

---

## `WildPlant_Stations.json`

Vier Stationsdatensätze, alle gleich: `stationCategories ["WILD_PLANT"]`,
`processes ["PROCESS_HARVEST_WILD"]`, `parallelSlots 1`, `speedMultiplier 1.0`,
`needsFuel false`.

`parallelSlots 1`, weil `PROCESS_HARVEST_WILD` als `STATION_ACTION` gar keinen
Job anlegt — dieselbe Begründung wie am Bienenstock. `needsFuel false`, weil
eine Pflanze kein Feuer braucht; `ChefZ_HasHeat()` bleibt bei der Basisantwort
„nein", und kein Prozess dieses Slice setzt `requiresHeat`.

`WILD_PLANT` ist eine **Stationskategorie**, keine Registry-Kategorie — ein
freies Symbol im offenen Namensraum, wie `APIARY`. Sie braucht keinen
Delta-Eintrag.
