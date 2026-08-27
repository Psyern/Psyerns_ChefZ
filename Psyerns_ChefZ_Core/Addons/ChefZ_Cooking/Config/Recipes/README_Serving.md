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
