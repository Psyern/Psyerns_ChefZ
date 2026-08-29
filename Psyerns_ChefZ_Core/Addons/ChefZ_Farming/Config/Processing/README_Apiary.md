# Slice `apiary` — Anmerkungen zu den drei JSON-Dateien

Der Text steht **neben** den Dateien und nicht in ihnen. Grund ist derselbe, den
`README.md` für `ProduceSeeds.json` festhält: `ChefZ_ConfigSelfTest.
ProbeUnknownFieldTolerance()` belegt die Toleranz des Enforce-Serializers
gegenüber unbekannten JSON-Feldern **nicht**. Ein `_comment` könnte die ganze
Datei unlesbar machen — und die Imkerei fiele still aus.

## `Apiary_Ingredients.json` — zwei Vanilla-Zutaten und ein Rähmchen

`WoodenPlank` und `Nail` sind **Vanilla**-Klassen. ChefZ fasst keine fremde
`config.cpp` an (11 E8), beschreibt fremde Klassen aber sehr wohl in einem
eigenen Zutatendatensatz — genau so macht es
`ChefZ_Ingredients/Config/Ingredients/VanillaProduce.json` mit `Potato`,
`Tomato` und `GreenBellPepper`.

### `Nail`, nicht `Nails`

Der erste Stand dieses Slice nannte die Nägel `Nails`, und der Server sagte es
beim Start: *'Die Klasse "Nails" existiert in keiner geladenen Config'*. Die
**Config**-Klasse (CfgVehicles) heißt `Nail`:

- `scripts - 1.29/4_World/DayZ/Entities/ItemBase/Nail.c:1` — `class Nail extends ItemBase`,
  die Skriptklasse zur Item-Config;
- `scripts - 1.29/4_World/DayZ/Entities/ItemBase/Inventory_Base/Nail.c:1` —
  `class Nails extends Inventory_Base`, eine **reine Skriptklasse ohne
  CfgVehicles-Eintrag**;
- `class Slot_Material_Nails` (`scripts - 1.29/config.cpp:2258`) ist ein
  **Slotname** in `CfgSlots`, kein Itemname — daraus war `Nails` fälschlich
  abgeleitet worden;
- die COT-Mission führt in ihrer `types.xml` nur `<type name="Nail">`.

Nötig ist der Zutatendatensatz wegen einer Unterscheidung, die beim ersten
Entwurf dieses Slice falsch war und den Auftrag stumm unerfüllbar gemacht hätte:

| Feld im Slot | Was es zählt |
|---|---|
| `minCount` / `maxCount` | **Item-Instanzen** — `ChefZ_SlotEvaluator.CheckCounts(slot, assignedCount)` |
| `amount` / `consumeAmount` | **Rezepteinheiten** — `CheckAmountIdx`, Summe über `ChefZ_ItemFacts.units` |

Bretter und Nägel sind in DayZ **Stapel mit Menge**, keine Einzelstücke:
`Construction.HasMaterialWithQuantityAttached()` fragt `attachment.GetQuantity()
>= quantity`, und `CraftWoodenPlank` setzt `m_ResultSetQuantity[0] = 4`, also
vier Bretter in **einem** Item.

`"minCount": 4` hätte deshalb **vier getrennte Brett-Items** verlangt. Das ist
unter HANDCRAFT ohnehin unerfüllbar: Vanillas `RecipeBase` führt genau zwei
Zutaten**plätze**, und `ChefZ_GenericCraftRecipe` bindet je Platz **ein** Item.
Der Transform hätte nie gematcht — ohne Fehlermeldung.

Richtig ist die Form, die `TR_SaltMeat` und die Salzslots der Schüsselgerichte
bereits benutzen:

```json
"minCount": 1, "maxCount": 1,
"unit": "PIECE", "amount": { "min": 4.0 },
"consume": "amount", "consumeAmount": 4.0
```

### `ChefZ_HoneycombFrameUncapped` — drei Gläser Vorrat plus eine Reserve

Das entdeckelte Rähmchen trägt `varQuantity 4..1` (config.cpp) und hier
`unitsPerWholeItem = 4`: eine Einheit ist ein Glas. Die Schleuder
(`ChefZ_Processing`, `TR_SpinHoney`) zieht je Durchlauf `1.0` ab und verlangt
dafür mindestens `2.0`: die Züge geschehen bei 4, 3 und 2 — drei Gläser —, die
letzte Einheit wird nie gezogen. Ein Rähmchen unter 2 wird im Skript der
Schleuder in seiner Cargo-Zelle zu `ChefZ_HoneycombFrameEmpty`.

Die Reserve ist eine Notwendigkeit des Core, keine Balance-Entscheidung:
`ChefZ_SlotEvaluator.PlanAmountDraw` (Z. 367–373) setzt `destroyWhole`, sobald
ein Abzug die letzte Einheit eines Items träfe, und `ChefZ_Applicator.
ConsumeInputs` (Z. 1250–1253) löscht das Item dann statt es auf 0 zu setzen.
Mit drei Einheiten wäre der Rahmen nach dem dritten Glas weg. `quantityShow`
steht deshalb auf 0 — eine „4" hieße für den Spieler vier Gläser.

## Die eine Zahl in diesem Slice, die im Spiel nachzumessen ist

`ChefZ_FactCollector` rechnet

```
units = quantity / quantityMax * unitsPerWholeItem
```

Damit „1 Einheit == 1 Brett" gilt, muss `unitsPerWholeItem` gleich dem
`varQuantityMax` der Vanillaklasse sein. Die Item-Configs von DayZ liegen dem
Projekt **nicht** vor (`refindex/vanilla-classes.txt` führt Klassennamen, keine
Feldwerte). Die beiden Werte stammen deshalb aus der nächstbesten belegten
Quelle — den Anbauplatz-Stapelgrenzen in Vanillas
`config.cpp` (`scripts - 1.29`, Zeilen 2258–2273):

| Klasse | `unitsPerWholeItem` | Beleg |
|---|---:|---|
| `WoodenPlank` | 20 | `class Slot_Material_WoodenPlanks { stackMax = 20; }` |
| `Nail` | 99 | `class Slot_Material_Nails { stackMax = 99; }` (Slotname, siehe oben) |

Das ist ein **Rückschluss**, keine gelesene Tatsache. Weicht `varQuantityMax`
davon ab, verschieben sich alle Mengen proportional — ein voller Nagelstapel
wäre dann nicht 99, sondern *n* Nägel, und „10 Einheiten" wären `10 * n / 99`
Nägel. Behoben wird das mit **einer Zahl je Klasse in dieser Datei**; kein
Transform muss angefasst werden. Der Punkt steht im Slice-Bericht als
In-Game-Prüfpunkt.

Die Zutatendatensätze tragen bewusst **keine** Kategorie und **kein** Tag:
Bretter, Nägel und Rähmchen sind kein Lebensmittel und sollen in keinem
Kochrezept über Kategorie-Matching auftauchen. Sie tragen nur die Mengeneinheit,
und die ist alles, wofür sie hier stehen.

## `Apiary_Crafts.json` — sieben Handwerksschritte

Fünf Bau-, ein Entdeckelungs- und ein Erweiterungsschritt, alle
`exec = HANDCRAFT`. Die Aufteilung in **je einen eigenen Prozess** ist in der
`config.cpp` bei `CfgChefZProcesses` begründet: der Menüname eines
Handwerksrezepts kommt aus dem Prozess, nicht aus dem Transform.

`TR_ExtendBeehive` macht aus **zwei Bausätzen** eine `ChefZ_BeehiveDouble` —
nicht aus Stock plus Bausatz: ein Handwerksschritt verbraucht seine Zutat samt
Cargo, ein bestückter Stock verlöre seine Rähmchen. Sollte der Selbsttest
melden, dass zwei Slots derselben Klasse nicht binden, ist der festgelegte
Fallback `kitB` → `WoodenPlank`, 8 Einheiten.

Die Ausgänge `ChefZ_HoneycombFrameEmpty` (`TR_BuildHoneycombFrame`) und
`ChefZ_HoneycombFrameUncapped` (`TR_UncapHoneycombFrame`) tragen **keine**
`quantity`: `ChefZ_ProcessRunner.ApplyHandcraftLayer` ruft `SetQuantity` nur,
wenn der Ausgang eine Menge definiert, sonst gilt der Klassendefault
(`varQuantityInit` 0 bzw. 4). Ein `"quantity": 1` machte aus dem Leerrähmchen
eines mit 1 % und aus dem entdeckelten eines ohne ein einziges Glas.

## `Apiary_Stations.json` — der Stock ohne Transform

Ein Vorgang an `ChefZ_Beehive` und `ChefZ_BeehiveDouble`: `PROCESS_HARVEST_HIVE`
(STATION_ACTION, 8 s) — **Öffnen**. Er hat absichtlich keinen Transform:
`ChefZ_ActionProcessAtStation.IsProcessUsable()` überspringt die
Transformprüfung, wenn keiner bekannt ist (Z. 321–324), `RunImmediate` meldet
`NO_MATCH`, und `NotifyStation` (Z. 687) ruft den Haken trotzdem. Der Haken
`ChefZ_Beehive.ChefZ_OnStationActionFinished()` öffnet den Deckel für 120 s und
löst den Stich aus. `parallelSlots` steht auf 1, weil nie ein Job entsteht.

Wie Rähmchen voll werden, ist **kein** Prozess, sondern Skript
(`Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c`):

- Leerrähmchen liegen im Cargo (10×9 bzw. 10×15 — mit Reserve, weil Vanilla
  Items gedreht ablegen darf und ein versetztes Rähmchen sonst das letzte
  aussperrt); hinein dürfen nur leere und volle Rähmchen, höchstens 10 bzw. 20
  (`CanReceiveItemIntoCargo`).
- `lifetime` in der config: 604800 s (Stock) bzw. 1209600 s (Doppelbeute),
  deutlich über der Füllzeit von 40 bzw. 80 h — die CE-Lebensdauer läuft in
  derselben Serverzeit ab wie das Füllen. **Betreiber:** ein Eintrag in der
  `types.xml` des Servers überschreibt diesen Wert; wer die Stöcke dort
  führt, muss `lifetime` mindestens so hoch ansetzen.
- Ein eigener Server-Timer (10-s-Takt) erhöht `varQuantity` **nur am ersten
  Leerrähmchen** in Cargo-Reihenfolge — eines nach dem anderen,
  4 h je Rähmchen, 10 Rähmchen = 40 h. Der Balken ist Vanillas `quantityBar`.
- Bei 100 wird das Rähmchen in derselben Cargo-Zelle per
  `TurnItemIntoItemLambda` durch `ChefZ_HoneycombFrameFull` ersetzt. Schlägt
  die Ersetzung fehl (Log: `lambda cannot be executed, skipping!` oder
  `Step D) ABORT`), bleibt das Rähmchen bei 100 % das erste, und jeder
  weitere Tick versucht die Ersetzung erneut — das nächste Rähmchen beginnt
  erst danach. Während der Ersetzung setzt der Stock `m_ChefZ_ReplacingFrame`,
  weil die Engine vor dem Entfernen `CanReleaseCargo` fragt und der Haken
  sonst bei zugeklapptem Deckel „nein" sagte.
- Heraus darf **nur ein volles Rähmchen und nur bei offenem Deckel**
  (`CanReleaseCargo`); die Entnahme ist der gewöhnliche Inventar-Drag.
- Persistenz: `varQuantity` speichert die Engine mit dem Item. Während der
  Server steht, vergeht keine Füllzeit (wie Vanillas Pflanzen).

`PROCESS_HARVEST_HIVE` führt **kein** `toolGroups`. Die Imkerpfeife war einmal
Pflichtwerkzeug — heute ist sie Schutz: das Öffnen gelingt auch ohne sie, es
kostet dann Blut und Schock. Die Begründung samt der Vanilla-Vorlage
`CAContinuousMineWood.DamagePlayersHands()` steht im Skript und in der
`config.cpp` am Prozess.

Die Schleuder liegt in `ChefZ_Processing`, weil Schleudern Verarbeitung ist.

`handcraftRecipeSlots` am Knoten `ChefZ_Apiary` steht auf **7** — fünf Bau-,
ein Entdeckelungs-, ein Erweiterungsschritt. Stationsvorgänge belegen keinen
Platz: `ChefZ_HandcraftBridge` reserviert ausschließlich für `exec = HANDCRAFT`.

## Spielstand — was ein Umstieg auf diesen Stand bedeutet

- Gespeicherte Rähmchen der früheren Zwischenklasse „verdeckelt" verschwinden
  beim Laden (unbekannter Typ). Ihr Inhalt war nie entnehmbar; der Verlust ist
  ein Rähmchen aus einem Brett und zwei Nägeln.
- Laufende Stationsjobs des gestrichenen Pflege-Vorgangs enden beim Laden als
  `transform_gone` ohne Rähmchenverlust (Basis: Abbruch ohne Verlust).
- Alte Leerrähmchen starten bei 0 %.
- `GlassBottle` ist kein Eingang der Schleuder mehr; das leere Glas ist
  `ChefZ_EmptyJar` (`ChefZ_Cooking`).
