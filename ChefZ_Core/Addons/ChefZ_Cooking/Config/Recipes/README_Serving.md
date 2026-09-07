# Slice `serving` — wie ein Gericht an Portionen und Behälter andockt

Dieser Slice bringt **kein** Gericht mit. Er bringt die Behälter, die
Portionskapazität der Kochgeräte und die beiden Configbasen mit, an denen die
Slices `dishes-a`, `dishes-b`, `dishes-c` und `sauces` ihre Gerichte anschließen.

Der Text steht neben den Rezepten und nicht als `_comment` **in** einer
Rezeptdatei: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()` hält fest, dass
die Toleranz des Enforce-Serializers gegenüber unbekannten JSON-Feldern **nicht
belegt** ist. Ist er intolerant, wird eine Datei mit Kommentarfeld komplett
verworfen.

## 1. Die eine Zeile, mit der ein Gericht fertig angeschlossen ist

**Seit 29.08.2026 gibt es keine Zwischenstufe mehr.** Das Rezept liefert das
Gericht direkt im Kochgerät; die Bulk-Klassen (`ChefZ_<Name>Bulk :
ChefZ_PortionedDish_Base`) und die Entnahmeaktion sind aus dem Content
gestrichen. Die Basen `ChefZ_PortionedDish_Base` / `ChefZ_PortionedFood_Base`
bleiben als Fähigkeit des Core stehen, kein ausgeliefertes Gericht benutzt sie.

```cpp
class ChefZ_HunterStewBowl : ChefZ_ServedDish_Base { scope = 2; ... varQuantityMax = 1200; };
```
```c
class ChefZ_HunterStewBowl extends ChefZ_ServedDish_Base {}
```

`class Nutrition` gehört an die **eigene** Gerichtklasse. Die Basis trägt bewusst
keine — eine geerbte Nährwertzeile wäre ein stiller Default, und `PlayerStomach`
registriert nur Klassen mit eigenem Nährwert (`01` V7).

## 2. Der Output-Block eines Gerichts

```json
"outputs": [{
  "cls":             "ChefZ_HunterStewBowl",
  "quantity":        400,
  "quantityMode":    "fixed",
  "returnContainer": "ChefZ_EmptyBowl",
  "setState":        "COOKED",
  "inheritQuality":  true
}]
```

**Portionen sind Menge.** `PlayerStomach.GetNutritions` (`PlayerStomach.c:92`)
rechnet `energy / 100` je verzehrter Einheit — 100 Einheiten sind also genau die
Nährwerte der Klasse, eine Portion. Ein Rezept mit vier Portionen setzt
`quantity = 400`, und die Klasse trägt `varQuantityMax` für ihr größtes Rezept
(Eintopf: Basis 4, Gruppenkessel 12 → 1200). Wer aus dem Topf isst, isst Portion
für Portion vom selben Item; geteilt wird, indem man es weiterreicht.

Die früheren Deckel (`portionCapacity` je Gerät in `CfgChefZDevices`,
`amountPerPortion`) greifen bei einem direkten Ergebnis nicht mehr — die
Portionszahl steht fest im Rezept, und ein Gruppenrezept ist ein eigenes Rezept
mit mehr Zutaten.

## 3. Behälterkategorien

`PLATE` · `BOWL` · `CAN` · `JAR` · `BOX`

Sie stehen **nicht** in der zentralen `Categories.json` — das ist die Registry
der Zutatenkategorien. Eine Behälterkategorie existiert genau dann, wenn ein
Behälter sich in `CfgChefZContainers` zu ihr bekennt (`16` E2). Deklariert sind
sie in der `config.cpp` dieses Moduls.

Beim Kochen wird kein Behälter verlangt (16 §2, unverändert). `returnContainer`
nennt seit 29.08.2026 eine **feste Klasse** (`ChefZ_EmptyBowl`, `ChefZ_EmptyPlate`):
`"AUTO"` löst über den beim Servieren benutzten Behälter auf, und serviert wird
nicht mehr — das Gericht entsteht direkt. Die leere Schüssel bleibt nach dem
letzten Bissen zurück.

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
und falsch. `CUTTING_TOOL` ist geteiltes Vokabular: `PROCESS_CUT_MEAT` und die
Schnitzprozesse lesen dieselben acht Messer. Eine Feuerwehraxt darin hackt ab sofort auch
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
