# Slice `serving` — wie ein Gericht an Portionen und Behälter andockt

Dieser Slice bringt **kein** Gericht mit. Er bringt die Behälter, die
Portionskapazität der Kochgeräte und die beiden Configbasen mit, an denen die
Slices `dishes-a`, `dishes-b`, `dishes-c` und `sauces` ihre Gerichte anschließen.

Der Text steht neben den Rezepten und nicht als `_comment` **in** einer
Rezeptdatei: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()` hält fest, dass
die Toleranz des Enforce-Serializers gegenüber unbekannten JSON-Feldern **nicht
belegt** ist. Ist er intolerant, wird eine Datei mit Kommentarfeld komplett
verworfen.

## 1. Die zwei Zeilen, mit denen ein Gericht fertig angeschlossen ist

Ein **Bulk-Gericht** (das, was im Topf entsteht):

```cpp
class ChefZ_HunterStewBulk : ChefZ_PortionedDish_Base { scope = 2; ... };
```
```c
class ChefZ_HunterStewBulk extends ChefZ_PortionedDish_Base {}
```

Eine **servierte Portion** (das, was der Spieler isst):

```cpp
class ChefZ_HunterStewBowl : ChefZ_ServedDish_Base { scope = 2; ... };
```
```c
class ChefZ_HunterStewBowl extends ChefZ_ServedDish_Base {}
```

Zähler, Entnahmeaktion, Persistenz, Tooltip „3 / 8", Behälterprüfung und
Behälterrückgabe kommen aus dem Core. Es gibt dafür **keine** neue Aktion und
**keine** Core-Änderung (15 E5).

`class Nutrition` und die `nutrition_properties[]` gehören an die **eigene**
Gerichtklasse. Die Basen tragen bewusst keine — eine geerbte Nährwertzeile wäre
ein stiller Default, und `PlayerStomach` registriert nur Klassen mit eigenem
Nährwert (`01` V7).

## 2. Der Output-Block eines Portionsgerichts

```json
"outputs": [{
  "cls":               "ChefZ_HunterStewBulk",
  "portions":          8,
  "portionClass":      "ChefZ_HunterStewBowl",
  "portionQuantity":   200,
  "amountPerPortion":  1.0,
  "containerCategory": "BOWL",
  "consumesContainer": true,
  "returnContainer":   "AUTO",
  "emptyOnLastPortion": "",
  "scaleWithDevice":   true,
  "inheritQuality":    true
}]
```

**`amountPerPortion` ist keine Kür.** Es ist der zweite Deckel aus `15` §5.2:
`floor(verbrauchte Zutatenmenge / amountPerPortion)`. Ohne ihn ergibt eine
Minimalfüllung im Kessel zwölf Portionen — der offensichtlichste Nahrungsexploit
des Mods. Der erste Deckel (`portionCapacity` je Gerät) steht in
`CfgChefZDevices` in der `config.cpp` dieses Moduls: FryingPan 2, Pot 4,
Cauldron 12 (Planungsschritte §11).

**Einzelgerichte sind Portionsgerichte mit `portions = 1`** (`15` E7). Es gibt
genau einen Mechanismus, auch für Tellergerichte — `containerCategory: "PLATE"`,
sonst identisch. `takeDurationSec = 0` macht den zusätzlichen Klick fast
unsichtbar.

## 3. Behälterkategorien

`PLATE` · `BOWL` · `CAN` · `JAR` · `BOX`

Sie stehen **nicht** in der zentralen `Categories.json` — das ist die Registry
der Zutatenkategorien. Eine Behälterkategorie existiert genau dann, wenn ein
Behälter sich in `CfgChefZContainers` zu ihr bekennt (`16` E2). Deklariert sind
sie in der `config.cpp` dieses Moduls.

Ein Rezept fordert immer die **Kategorie**, nie eine Klasse. `returnContainer:
"AUTO"` gibt genau den Behälter zurück, der benutzt wurde — ein später
hinzugefügter Holznapf funktioniert damit sofort mit allen bestehenden
Schüsselgerichten, ohne dass ein Rezept angefasst wird.

`CAN` ist der Konservenfall: `reusable = 0`, es kommt nichts zurück.

## 4. Die Rückgabe

Sie hängt am **Item**, nicht am Rezept (`16` E3): `m_ChefZ_ReturnContainer`
persistiert auf dem Gericht, weil das Rezept beim Verzehr nicht mehr bekannt
ist. Ein Gericht, das per Admin-Spawn entsteht und nie über ein Rezept lief,
bekommt seine Bindung über `CfgChefZIngredients`:

```cpp
class ChefZ_HunterStewBowl { returnContainer = "AUTO"; };
```

Ausgelöst wird sie in `ChefZ_Edible_Base.OnConsume`, **nachdem** Vanilla die
Menge abgezogen hat und nur bei `Quantity <= 0` (`16` E4). Nicht bei jedem
Bissen — das wäre ein Duplikations-Exploit erster Ordnung.

## 5. Woher die Behälter kommen — die fünf HANDCRAFT-Transforms

Alle fünf liegen in `Config/Processing/`, nicht hier: es sind `kind: transform`,
keine `kind: recipe`. Ihre Prozesse stehen in **Rang 1** (`CfgChefZProcesses` in
der `config.cpp` dieses Moduls), weil `ActionCondition()` den Aktionstext auch
clientseitig braucht (`02` §2).

| Transform | Datei | Eingang 1 | Eingang 2 / Werkzeug | Ergebnis |
|---|---|---|---|---|
| `TR_CarveWoodenPlate` | `Tableware.json` | `Firewood` | `CUTTING_TOOL` | 1 × `ChefZ_EmptyPlate` |
| `TR_CarveWoodenBowl` | `Tableware.json` | `Firewood` | `CUTTING_TOOL` | 1 × `ChefZ_EmptyBowl` |
| `TR_BowlFromBark` | `Containers.json` | `Bark_Birch` \| `Bark_Oak` | `CUTTING_TOOL` + `AXE_TOOL` | 1 × `ChefZ_EmptyBowl` |
| `TR_BoxFromPaper` | `Containers.json` | `Paper` | `Paper` | 1 × `ChefZ_EmptyBox` |
| `TR_CansFromMetalSheet` | `Containers.json` | `MetalPlate` | `SAWING_TOOL` (`Hacksaw`) | **10 ×** `ChefZ_EmptyCan` |

### Die Form, die ein HANDCRAFT-Transform haben **muss**

`RecipeBase.c` führt `MAX_NUMBER_OF_INGREDIENTS = 2` und `MAXIMUM_RESULTS = 10`
(`01` V12). Daraus folgen genau zwei zulässige Bauformen, und
`ChefZ_GenericCraftRecipe.InitFromDef()` weist alles andere ab:

* **ein Eingang + Werkzeuggruppe** — das Werkzeug belegt den zweiten Platz.
* **zwei Eingänge, keine Werkzeuggruppe** — sonst wäre das Werkzeug der dritte.

Ein Eingang *ohne* Werkzeug ist nicht registrierbar: Vanillas Craftsystem
kombiniert immer zwei Dinge. Und `stationsAllowed` darf **kein** HANDCRAFT-
Transform nennen — die Handwerksbrücke weist ihn dann ab, und das Rezept
erscheint im Spiel nie, ohne Fehlermeldung.

### „paper + paper" sind **zwei Slots**, nicht ein Slot mit `minCount: 2`

`ChefZ_HandcraftBridge` bildet jeden Eingangs-Slot auf **einen** Vanilla-
Zutatenplatz ab (`InsertIngredient(s, …)` mit `s` = Slotindex). Ein Slot mit
`minCount: 2` ergäbe deshalb nur **einen** Zutatenplatz — und damit den
abgewiesenen Fall „ein Eingang ohne Werkzeug". Es sind zwei Slots, `paperA` und
`paperB`, beide auf `Paper`.

Dass beide Slots dieselbe Klasse fordern, ist unkritisch: der Matcher garantiert
„ein Item bedient höchstens einen Slot" (`ChefZ_Matcher`, Kopf, Eigenschaft 2).
Dasselbe Blatt Papier kann nicht beide Plätze füllen.

### Zehn Dosen sind **zehn `outputs`**, keine `quantity: 10`

`quantity` landet über `ChefZ_Applicator` in `item.SetQuantity()` — das ist die
Füllmenge **eines** Items, kein Stapelzähler. Bei einem Behälter ohne
`varQuantityMax` bewirkt sie nichts. Vanilla erzeugt dagegen pro `AddResult()`
genau ein Item; zehn Dosen sind deshalb zehn Einträge in `outputs`.

Das ist exakt die Obergrenze: `MAXIMUM_RESULTS = 10`. `outputs` + `byproducts`
zusammen dürfen zehn nicht überschreiten — dieser Transform hat für ein
Nebenprodukt keinen Platz mehr.

### `handcraftRecipeSlots` ist die Zahl, die man vergisst

`CfgChefZ/ChefZ_Serving` nennt `handcraftRecipeSlots = 5` — genau die fünf
Zeilen der Tabelle oben. Die Reservierung muss **vor** dem Laden feststehen, weil
Vanilla Rezept-IDs als Position in seiner Liste vergibt. Zu wenige Plätze heißt:
die überzähligen Transforms erscheinen nicht, und im Spiel deutet nichts darauf
hin. Wer hier einen Transform ergänzt, erhöht die Zahl in derselben Änderung.

### Werkzeuggruppen: warum `CUTTING_TOOL` nicht erweitert wurde

„any axe or knife" wäre als Erweiterung von `CUTTING_TOOL` billiger gewesen —
und falsch. `CUTTING_TOOL` ist geteiltes Vokabular: `PROCESS_CUT_MEAT` und
`PROCESS_CLEAN_CASING` lesen dieselben acht Messer. Eine Feuerwehraxt darin hackt ab sofort auch
Kräuter. Die Gruppe gehört außerdem `ChefZ_Processing`, nicht diesem Modul.

Stattdessen steht `AXE_TOOL` daneben, und `PROCESS_CARVE_BOWL_BARK` nennt
**beide**: `ChefZ_HandcraftBridge.CollectToolClasses()` bildet die Vereinigung
über alle genannten Gruppen — ein Werkzeug aus einer von beiden genügt.

`SAWING_TOOL` (`Hacksaw`) ist aus demselben Grund keine Erweiterung von
`METALWORK_TOOL`: dort stehen Zange, Hammer, Schraubenschlüssel und
Schraubendreher — Werkzeuge zum Biegen und Verbinden, nicht zum Zerteilen.

Beide neuen Gruppen stehen **genau einmal** im Projekt, in der `config.cpp`
dieses Moduls. Die Engine mergt `CfgChefZTools` über alle Addons; zwei Knoten
gleichen Namens sind keine Redundanz, sondern eine stille Überschreibung.

### Die Holzschale ist `ChefZ_EmptyBowl` — keine neue Klasse

Der Auftrag nennt eine „Holzschale". Es gibt sie bereits: `ChefZ_EmptyBowl`
heißt im Spiel *Empty Bowl* / *Leere Schüssel* und wird in
`STR_CHEFZ_ITEM_EMPTYBOWL1` seit jeher als **geschnitzte Holzschüssel**
beschrieben. Eine zweite Klasse hätte dieselbe Geometrie, dasselbe Gewicht,
dieselbe Behälterkategorie `BOWL` und denselben Zweck gehabt — und jedes
Bowl-Gericht mit `returnContainer: "AUTO"` hätte sie zusätzlich kennen müssen.
Ein zweiter **Weg** zum selben Gegenstand ist ein Transform, keine Klasse.

### Was `ChefZ_EmptyJar` betrifft: nichts

DME-Plan §32 nennt das Einmachglas unter „zusätzlich für ChefZ", ohne einen
Herstellungsweg. Glasarbeit ist kein Blechschnitt und kein Papierfalz; der Weg
bleibt offen, statt erfunden zu werden. `ChefZ_EmptyJar` hat damit weiterhin
null Rezeptreferenzen — bewusst und benannt.
