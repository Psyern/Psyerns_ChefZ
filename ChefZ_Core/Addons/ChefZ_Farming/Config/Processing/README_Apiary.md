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

### `ChefZ_HoneycombFrameUncapped` — vier Gläser, Rahmen kommt leer zurück (31.08.2026)

Das entdeckelte Rähmchen trägt `varQuantity 5..1` (config.cpp) und hier
`unitsPerWholeItem = 5`: **vier Gläser Vorrat plus eine Reserve-Einheit**. Die
Schleuder zieht je Durchlauf `1.0` ab und verlangt dafür mindestens `2.0` — die
Züge geschehen bei 5, 4, 3 und 2. Bei Reststand 1 unterschreitet der Rahmen die
Schwelle und wird im Skript der Schleuder in seinem Slot zu
`ChefZ_HoneycombFrameEmpty` zurückgewandelt. Die Kette ist ein **Kreis**.

| | vorher | jetzt |
|---|---|---|
| `varQuantityInit` / `Max` | 4 / 4 | **5 / 5** |
| `unitsPerWholeItem` | 4 | **5** |
| `amount.min` an `TR_SpinHoney` | 2.0 | 2.0 *(unverändert)* |
| `consumeAmount` | 1.0 | 1.0 *(unverändert)* |
| Züge | bei 4, 3, 2 | bei **5, 4, 3, 2** |
| Gläser je Rahmen | 3 | **4** |
| Rahmen danach | kommt leer zurück | kommt leer zurück |

Geändert hat sich einzig die **Stufenhöhe**, nicht die Bauform.

Die Reserve-Einheit ist keine Balance-Entscheidung, sondern eine Notwendigkeit
des Core: `ChefZ_SlotEvaluator.PlanAmountDraw` (Z. 369–376) setzt
`destroyWhole`, sobald ein Abzug die **letzte** Einheit eines Items träfe, und
der Applicator löscht das Item dann, statt es auf 0 zu setzen.

**„Verbrauch bis 0" wurde deshalb ausdrücklich verworfen** (Abstimmung mit dem
Slice `processing`): ein Rahmen, der bis 0 gezogen wird, ist am Ende
*zerstört* — es käme kein Leerrahmen zurück, und genau der ist gefordert. Die
fünfte Einheit wird nie gezogen; sie ist der Boden, auf dem der Rahmen die
Schleuder überlebt.

Die drei Zahlen, an denen die Ausbeute hängt, stehen an **drei** Orten und
müssen zusammenpassen:

| Wert | Ort |
|---|---|
| `varQuantityInit` / `varQuantityMax` = 5 | `ChefZ_Farming/config.cpp` |
| `unitsPerWholeItem` = 5 | diese Datei |
| `amount.min` = 2.0, `consumeAmount` = 1.0 | `ChefZ_Processing/…/Honey.json` |
| `CHEFZ_FRAME_SPENT_BELOW` = 2.0 | `ChefZ_Processing/…/ChefZ_HoneyExtractor.c` |

`quantityShow` steht auf 0 — eine „5" hieße für den Spieler fünf Gläser, und
das wäre gelogen. Der Balken sinkt in Fünfteln und reicht als Anzeige.

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

## `Apiary_Crafts.json` — acht Handwerksschritte

Alle `exec = HANDCRAFT`, und es sind **acht**, nicht sieben — die Aufzählung
hier führte `TR_FillBeeSmoker` bis zum 31.08.2026 nicht mit, obwohl der
Schritt seit dem 29.08.2026 in der Datei steht und
`handcraftRecipeSlots` in der `config.cpp` längst auf 8 stand:

| Transform | Prozess | was er tut |
|---|---|---|
| `TR_BuildBeehiveKit` | `PROCESS_BUILD_HIVE_KIT` | 4 Bretter + 10 Nägel → Bausatz |
| `TR_RaiseBeehive` | `PROCESS_RAISE_HIVE` | Bausatz + HAND_TOOL → Stock |
| `TR_ExtendBeehive` | `PROCESS_EXTEND_HIVE` | zwei Bausätze → Doppelbeute |
| `TR_BuildHoneycombFrame` | `PROCESS_BUILD_FRAME` | 1 Brett + 2 Nägel → Leerrähmchen |
| `TR_BuildUncappingFork` | `PROCESS_BUILD_UNCAPPING_FORK` | 1 Brett + 4 Nägel → Gabel |
| `TR_BuildBeeSmoker` | `PROCESS_BUILD_BEE_SMOKER` | TunaCan_Opened + HAND_TOOL → Imkerpfeife |
| `TR_UncapHoneycombFrame` | `PROCESS_UNCAP_COMB` | volles Rähmchen + Gabel → entdeckeltes |
| **`TR_FillBeeSmoker`** | **`PROCESS_FILL_SMOKER`** | **höchstens halbvolle Pfeife + 2 Rinde → volle Pfeife** |

Die Aufteilung in **je einen eigenen Prozess** ist in der `config.cpp` bei
`CfgChefZProcesses` begründet: der Menüname eines Handwerksrezepts kommt aus
dem Prozess, nicht aus dem Transform.

`TR_ExtendBeehive` macht aus **zwei Bausätzen** eine `ChefZ_BeehiveDouble` —
nicht aus Stock plus Bausatz: ein Handwerksschritt verbraucht seine Zutat samt
allem, was an ihr hängt, ein bestückter Stock verlöre seine Rähmchen. Sollte der Selbsttest
melden, dass zwei Slots derselben Klasse nicht binden, ist der festgelegte
Fallback `kitB` → `WoodenPlank`, 8 Einheiten.

Die Ausgänge `ChefZ_HoneycombFrameEmpty` (`TR_BuildHoneycombFrame`) und
`ChefZ_HoneycombFrameUncapped` (`TR_UncapHoneycombFrame`) tragen **keine**
`quantity`: `ChefZ_ProcessRunner.ApplyHandcraftLayer` ruft `SetQuantity` nur,
wenn der Ausgang eine Menge definiert, sonst gilt der Klassendefault
(`varQuantityInit` 0 bzw. 4). Ein `"quantity": 1` machte aus dem Leerrähmchen
eines mit 1 % und aus dem entdeckelten eines ohne ein einziges Glas.

## `Apiary_Stations.json` — der Stock ohne Transform

**Zwei** Vorgänge an `ChefZ_Beehive` und `ChefZ_BeehiveDouble`, beide
`STATION_ACTION` und beide **ohne Transform**:

| Prozess | Dauer | Werkzeug | was er tut |
|---|---:|---|---|
| `PROCESS_HARVEST_HIVE` | 8 s | — | **Öffnen** — Deckel für 120 s ab, Bienenstich |
| `PROCESS_PACK_HIVE` | 20 s | `HAND_TOOL` | **Abbauen** — Stock zurück zum Bausatz |

Dass hier bis zum 31.08.2026 von *einem* Vorgang die Rede war, war ein
Nachtrageversäumnis: `PROCESS_PACK_HIVE` kam am 29.08.2026 dazu und steht seit
damals in beiden Stationsdatensätzen. Der eigene Abschnitt „Abbauen" weiter
unten beschreibt ihn.

Beide haben absichtlich keinen Transform:
`ChefZ_ActionProcessAtStation.IsProcessUsable()` überspringt die
Transformprüfung, wenn keiner bekannt ist (Z. 321–324), `RunImmediate` meldet
`NO_MATCH`, und `NotifyStation` (Z. 687) ruft den Haken trotzdem. Der Haken
`ChefZ_Beehive.ChefZ_OnStationActionFinished()` öffnet den Deckel bzw. stößt
den Abbau an. `parallelSlots` steht auf 1, weil nie ein Job entsteht.

Wie Rähmchen voll werden, ist **kein** Prozess, sondern Skript
(`Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c`):

- Rähmchen sitzen seit dem 31.08.2026 in **Attachment-Slots**
  (`ChefZ_Frame01`…`ChefZ_Frame20`, `class CfgSlots` in der `config.cpp`), nicht
  mehr in einem Cargo-Gitter. Zehn Slots am Stock, zwanzig an der Doppelbeute —
  die Stückzahlgrenze steht damit in der Config und nicht mehr in einer
  Zählschleife. Hinein dürfen nur **leere und volle** Rähmchen
  (`CanReceiveAttachment`); das entdeckelte trägt zwar dieselben
  `inventorySlot[]` — anders käme die Ersetzung nicht durch —, wird aber vom
  Skript abgewiesen.
- `lifetime` in der config: 604800 s (Stock) bzw. 1209600 s (Doppelbeute),
  deutlich über der Füllzeit von 40 bzw. 80 h — die CE-Lebensdauer läuft in
  derselben Serverzeit ab wie das Füllen. **Betreiber:** ein Eintrag in der
  `types.xml` des Servers überschreibt diesen Wert; wer die Stöcke dort
  führt, muss `lifetime` mindestens so hoch ansetzen.
- Ein eigener Server-Timer (10-s-Takt) erhöht `varQuantity` **nur am ersten
  Leerrähmchen** in **Slotreihenfolge** (01 vor 02 vor 03 …) — eines nach dem
  anderen, 4 h je Rähmchen, 10 Rähmchen = 40 h. Der Balken ist Vanillas
  `quantityBar`. Die Reihenfolge hängt jetzt an der Slotnummer und nicht mehr
  daran, wer wann was wohin geschoben hat.
- **Ein voll eingehängtes Leerrähmchen wird auf 0 gesetzt**
  (`EEItemAttached`). Grund: per Admin-Werkzeug gespawnte Items kommen mit
  voller Quantity in die Welt, und so ein Rähmchen galt beim ersten Tick sofort
  als fertig — vierzig Stunden Kette, übersprungen. Verlustfrei ist das, weil
  nur *volle* Rähmchen den Stock verlassen dürfen: außerhalb des Stocks kann es
  keinen legitim erarbeiteten Teilfortschritt geben.
  Zwei Wachen sichern den Ladepfad ab — der Haken feuert nämlich **auch beim
  Laden aus dem Spielstand** (belegt an `FireplaceBase.c:332–339` gegen
  `:469–483`): zurückgesetzt wird nur, wenn der Stock schon
  `IsInitialized()` ist *und* das Rähmchen voll ankommt. Die Richtung ist
  bewusst gewählt — ein Irrtum lässt den Fehler stehen, statt Fortschritt zu
  vernichten.
- Bei 100 wird das Rähmchen **im selben Slot** per `TurnItemIntoItemLambda`
  durch `ChefZ_HoneycombFrameFull` ersetzt — `ReplaceItemWithNewLambdaBase.c:
  150–153` hat dafür einen eigenen `InventoryLocationType.ATTACHMENT`-Zweig,
  der Elter und Slot ausdrücklich übernimmt. Schlägt die Ersetzung fehl (Log:
  `lambda cannot be executed, skipping!` oder `Step D) ABORT`), bleibt das
  Rähmchen bei 100 % das erste, und jeder weitere Tick versucht die Ersetzung
  erneut — das nächste Rähmchen beginnt erst danach. Während der Ersetzung
  setzt der Stock `m_ChefZ_ReplacingFrame`, weil die Engine vor dem Entfernen
  `CanReleaseAttachment` fragt und der Haken sonst bei zugeklapptem Deckel
  „nein" sagte.
- Heraus darf **nur ein volles Rähmchen und nur bei offenem Deckel**
  (`CanReleaseAttachment`); die Entnahme ist der gewöhnliche Inventar-Drag.
- Persistenz: `varQuantity` speichert die Engine mit dem Item. Während der
  Server steht, vergeht keine Füllzeit (wie Vanillas Pflanzen).

### Der Bausatz ist platzierbar (31.08.2026)

`ChefZ_BeehiveKit` kennt seither **zwei** Wege zum aufgestellten Stock, und
beide bleiben:

1. **Platzieren** wie ein Vanilla-Bausatz — Kit in die Hand,
   `ActionTogglePlaceObject`, `ActionDeployObject`. Der Stock entsteht in
   `ChefZ_BeehiveKit.OnPlacementComplete()` (Vorbild wörtlich `FenceKit.c:19–33`
   und `TotemKit.c:32–48`), der Bausatz wird von Vanilla selbst gelöscht
   (`ActionDeployObject.c:230–233`, über `IsBasebuildingKit()`).
2. **`TR_RaiseBeehive`**, der Handwerksschritt mit einem `HAND_TOOL`.
   Unverändert — er funktioniert auch dort, wo vor dem Spieler keine freie
   Fläche liegt.

Im Hologramm schwebt `ChefZ_BeehivePlacing`, eine leere Hülle mit dem
Beutenmodell (`projectionTypename` am Bausatz, `Hologram.c:104–109`). Der echte
`ChefZ_Beehive` wäre dort eine vollwertige Station mit Fülltimer und zwanzig
Slots, nur um sie gleich wieder wegzuwerfen.

**Offen:** `hologramMaterial` und `hologramMaterialPath` stehen leer — ein
Geisterschimmer braucht ein `.rvmat` zum Modell, und die Beutenlieferung bringt
keines mit. Leer ist der belegte Weg für genau diesen Fall
(`DayZExpansion/Objects/Basebuilding/Safes/config.cpp:121–122`); der Spieler
sieht die Beute in normaler Textur schweben. Als Asset-Bedarf gemeldet.

`PROCESS_HARVEST_HIVE` führt **kein** `toolGroups`. Die Imkerpfeife war einmal
Pflichtwerkzeug — heute ist sie Schutz: das Öffnen gelingt auch ohne sie, es
kostet dann Blut und Schock. Die Begründung samt der Vanilla-Vorlage
`CAContinuousMineWood.DamagePlayersHands()` steht im Skript und in der
`config.cpp` am Prozess.

Die Schleuder liegt in `ChefZ_Processing`, weil Schleudern Verarbeitung ist.

`handcraftRecipeSlots` am Knoten `ChefZ_Apiary` steht auf **8** — die acht
Transforms der Tabelle oben. Stationsvorgänge belegen keinen Platz:
`ChefZ_HandcraftBridge` reserviert ausschließlich für `exec = HANDCRAFT`.

## Spielstand — was ein Umstieg auf diesen Stand bedeutet

- **Rähmchen im Cargo eines gespeicherten Stocks gehen verloren (31.08.2026).**
  Der Stock hat keinen Cargo mehr, sondern zwanzig Attachment-Slots; was die
  Engine im Cargo gespeichert hat, findet nach dem Umbau keinen Ort. Betreiber,
  die den Slice bereits im Betrieb haben, sollten die Stöcke vor dem Wechsel
  leeren lassen oder den Verlust ankündigen.
- Gespeicherte Rähmchen der früheren Zwischenklasse „verdeckelt" verschwinden
  beim Laden (unbekannter Typ). Ihr Inhalt war nie entnehmbar; der Verlust ist
  ein Rähmchen aus einem Brett und zwei Nägeln.
- Laufende Stationsjobs des gestrichenen Pflege-Vorgangs enden beim Laden als
  `transform_gone` ohne Rähmchenverlust (Basis: Abbruch ohne Verlust).
- Alte Leerrähmchen starten bei 0 %.
- `GlassBottle` ist kein Eingang der Schleuder mehr; das leere Glas ist
  `ChefZ_EmptyJar` (`ChefZ_Cooking`).

## Abbauen — `PROCESS_PACK_HIVE` (29.08.2026)

Lykos' Lieferung (`ChefZ/ChefZ_Devices/scripts/.../Pack_BeeHive.c`) hatte den
Rückbau als Vanilla-Rezept: Stock + Schraubenzieher → Bausatz. Ein
Handwerksschritt bräuchte den 14-kg-Stock aber in der Hand; am aufgestellten
Stock gibt es nur eine Aktionsform, und das ist `ChefZ_ActionProcessAtStation`.
Deshalb ist der Rückbau der **zweite Stationsvorgang** beider Beuten
(`Apiary_Stations.json`), `exec = STATION_ACTION`, Werkzeuggruppe `HAND_TOOL`.

Er hat **keinen Transform** — er verändert nicht den Inhalt der Station, sondern die
Station selbst. Das erledigt `ChefZ_Beehive.ChefZ_PackUp()` einen Frame nach
dem Haken `ChefZ_OnStationActionFinished`: ein Bausatz (Doppelbeute: zwei) auf
dem Boden an der Stelle des Stocks, Gesundheit anteilig übernommen, dann
`DeleteSafe()`.

Der Vorgang **erscheint nur an einem leeren, geschlossenen Stock**:
`ChefZ_GetProcessAt` und `ChefZ_SupportsProcess` liefern ihn sonst als
`INVALID`, und die Aktion überspringt ihn — auf dem Client (`RefreshProcesses`)
wie auf dem Server (`ResolveProcessFor`). Ein Stock mit Rähmchen lässt sich
also nicht samt Volk einpacken; erst alle Rähmchen herausnehmen. Gestochen wird
beim Abbau wie bei jeder Aktion am Stock, wenn keine brennende Imkerpfeife in
der Hand ist.
