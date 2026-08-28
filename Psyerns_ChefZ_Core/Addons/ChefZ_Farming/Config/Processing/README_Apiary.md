# Slice `apiary` — Anmerkungen zu den vier JSON-Dateien

Der Text steht **neben** den Dateien und nicht in ihnen. Grund ist derselbe, den
`README.md` für `ProduceSeeds.json` festhält: `ChefZ_ConfigSelfTest.
ProbeUnknownFieldTolerance()` belegt die Toleranz des Enforce-Serializers
gegenüber unbekannten JSON-Feldern **nicht**. Ein `_comment` könnte die ganze
Datei unlesbar machen — und die Imkerei fiele still aus.

## `Apiary_Ingredients.json` — die zwei Vanilla-Zutaten, und warum sie hier stehen

`WoodenPlank` und `Nails` sind **Vanilla**-Klassen. ChefZ fasst keine fremde
`config.cpp` an (11 E8), beschreibt fremde Klassen aber sehr wohl in einem
eigenen Zutatendatensatz — genau so macht es
`ChefZ_Ingredients/Config/Ingredients/VanillaProduce.json` mit `Potato`,
`Tomato` und `GreenBellPepper`.

Nötig ist das wegen einer Unterscheidung, die beim ersten Entwurf dieses Slice
falsch war und den Auftrag stumm unerfüllbar gemacht hätte:

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
| `Nails` | 99 | `class Slot_Material_Nails { stackMax = 99; }` |

Das ist ein **Rückschluss**, keine gelesene Tatsache. Weicht `varQuantityMax`
davon ab, verschieben sich alle Mengen proportional — ein voller Nagelstapel
wäre dann nicht 99, sondern *n* Nägel, und „10 Einheiten" wären `10 * n / 99`
Nägel. Behoben wird das mit **einer Zahl je Klasse in dieser Datei**; kein
Transform muss angefasst werden. Der Punkt steht im Slice-Bericht als
In-Game-Prüfpunkt.

Die Zutatendatensätze tragen bewusst **keine** Kategorie und **kein** Tag:
Bretter und Nägel sind kein Lebensmittel und sollen in keinem Kochrezept über
Kategorie-Matching auftauchen. Sie tragen nur die Mengeneinheit, und die ist
alles, wofür sie hier stehen.

## `Apiary_Crafts.json` — sechs Handwerksschritte

Fünf Bau- und ein Entdeckelungsschritt, alle `exec = HANDCRAFT`. Die Aufteilung
in **je einen eigenen Prozess** ist in der `config.cpp` bei
`CfgChefZProcesses` begründet: der Menüname eines Handwerksrezepts kommt aus dem
Prozess, nicht aus dem Transform.

## `Apiary_Hive.json` und `Apiary_Stations.json` — der Stock

Zwei Vorgänge an `ChefZ_Beehive`: `PROCESS_TEND_HIVE` (STATION_TIMED, eine
Stunde) und `PROCESS_HARVEST_HIVE` (STATION_ACTION).
Die Schleuder liegt in `ChefZ_Processing`, weil Schleudern Verarbeitung ist.

`PROCESS_HARVEST_HIVE` führt **kein** `toolGroups` mehr. Die Imkerpfeife war
einmal Pflichtwerkzeug — heute ist sie Schutz: die Ernte gelingt auch ohne sie,
sie kostet dann Blut und Schock. Die Regel steht im Skript an
`ChefZ_Beehive.ChefZ_OnStationActionFinished()`
(`Scripts/4_World/ChefZ/Farming/ChefZ_Apiary.c`), die Begründung samt der
Vanilla-Vorlage `CAContinuousMineWood.DamagePlayersHands()` dort und in der
`config.cpp` am Prozess.

Auf die Zahl der reservierten Handwerksplätze wirkt der Wegfall nicht:
`ChefZ_HandcraftBridge` reserviert ausschließlich für `exec = HANDCRAFT`, und
dieser Prozess ist `STATION_ACTION`. `handcraftRecipeSlots` am Knoten
`ChefZ_Apiary` bleibt bei **6** — den fünf Bau- und dem Entdeckelungsschritt.
