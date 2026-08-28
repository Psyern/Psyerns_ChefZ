# Zutatenbindungen dieses Moduls — Anmerkungen

In diesem Ordner liegen `Dairy.json`, `Salt.json`, `Spices.json` und
`VanillaProduce.json`. Nur `VanillaProduce.json` trug bis M2 einen Kommentar; er
stand als `_comment` auf der **obersten Dokumentebene**, neben `kind`.

Grund der Verlagerung: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()` haelt
fest, dass die Toleranz des Enforce-Serializers gegenueber unbekannten JSON-Feldern
**nicht belegt** ist. `ChefZ_JsonDocs` kennt auf Dokumentebene genau drei Felder —
`kind`, `schemaVersion`, `records`. Ist der Serializer intolerant, wird die Datei
**komplett** verworfen. Der Selbsttest sagt woertlich: "Content-Autoren duerfen
keine Kommentarfelder in JSON schreiben."

## VanillaProduce.json

Slice produce. Entwurf 05 §2, zweiter Deklarationsweg: FREMDE Klassen werden im
Slice-JSON gebunden, nie in ihrer eigenen `config.cpp` (Workflow §10.5). `Potato`
und `Tomato` existieren in Vanilla — ChefZ erweitert sie, statt eine zweite
Kartoffel zu bauen (Production Map §13, §14). `GreenBellPepper` ist die
Paprika des Mods (§15): es gibt keine eigene frische Paprikaklasse mehr, und
`TR_ChopBellPepper` ist der einzige Weg zu `ChefZ_ChoppedPaprika`
(Vanilla-Audit §2).

**Warum der Datensatz von `GreenBellPepper` kein `decays` bekommen hat.** Das
abgeloeste `ChefZ_Paprika` trug `decays: true`. Uebernommen wurde es NICHT, und
zwar nicht aus Nachlaessigkeit: `decays` wird an genau einer Stelle gelesen, in
`ChefZ_Edible_Base.CanDecay()` (01 V9). Vanilla-Klassen erben nicht von
`ChefZ_Edible_Base` — der Wert waere wirkungslose Daten, die spaeter jemand als
Zusage liest. `GreenBellPepper` verdirbt ohnehin ueber Vanillas eigenen
Verfall; die ChefZ-Fahne braucht es dafuer nicht. Aus demselben Grund tragen
`Potato` und `Tomato` hier kein `decays`.

Umgekehrt ist der Tausch ein GEWINN: `ChefZ_Paprika` hatte `tags: []` und konnte
die Punktregel `GR_VS_FreshVeg` (Tag `CHEFZ_FRESH`, `BowlDishes.json`) nie
ausloesen. `GreenBellPepper` traegt `CHEFZ_FRESH` und tut es.

## Mushrooms.json

Slice `sauces`. Entwurf 05 §2, zweiter Deklarationsweg: FREMDE Klassen werden im
Slice-JSON gebunden, nie in ihrer eigenen `config.cpp` (Workflow §10.5). Die
Pilze existieren in Vanilla; ChefZ baut keinen zweiten Pilz.

Gebunden sind **sieben** der neun Vanilla-Pilzklassen. Nicht gebunden sind
`AmanitaMushroom` (giftig) und `PsilocybeMushroom` (halluzinogen): waeren sie in
der Kategorie `MUSHROOM`, liesse `RCP_ChefZ_MushroomCreamSauce` eine Sauce aus
Fliegenpilzen kochen — und die Ergebnisklasse `ChefZ_MushroomCreamSauce` traegt
`toxicity = 0`. Das Gift verschwaende beim Kochen, lautlos. Ein spaeterer Slice,
der Giftpilze bewusst will, gibt ihnen eine eigene Kategorie und ein eigenes
Ergebnis.

Die Kategorie `MUSHROOM` und der Tag `CHEFZ_MUSHROOM` stehen im Delta
`_deltas/sauces.json`.

## Nachtraegliche Kategorien des Slice `sauces`

Additiv, ohne Verhaltensaenderung fuer bestehende Selektoren:

- `Dairy.json` — `ChefZ_Cream` bekommt `CREAM`, `ChefZ_Butter` bekommt `BUTTER`
  (zusaetzlich zu `DAIRY`).
- `VanillaProduce.json` — `Tomato` bekommt `TOMATO` (zusaetzlich zu `VEGETABLE`).
- `config.cpp`, `CfgChefZIngredients` — `ChefZ_ChoppedTomato` bekommt `TOMATO`.

Grund: `RCP_ChefZ_TomatoSauce` und `RCP_ChefZ_CreamSauce` brauchen Slots, die
Tomate, Sahne und Butter treffen und nicht jedes Gemuese beziehungsweise jedes
Milchprodukt. `ChefZ_Garlic` (ganze Knolle, ChefZ_Farming) hat bewusst **keine**
neue Kategorie bekommen — dieser Slice schreibt nicht in fremde Module.


## VanillaFoodstuffs.json

Slice `vanilla-foods`. Zwanzig FREMDE Klassen aus Vanilla-Audit §3 — die Datei
deklariert keine einzige eigene Klasse, sie bindet nur Vorhandenes (Entwurf 05 §2,
Workflow §10.5: fremde Klassen im Slice-JSON, nie in ihrer eigenen `config.cpp`).

### Die eine Frage, an der jede dieser Bindungen hängt: kann die Klasse kochen?

`ChefZ_RecipeEvaluator.CheckStages` verlangt von **jeder gebundenen Pflichtzutat**
eines `ON_STAGE`-Rezepts eine erlaubte Vanilla-Endstufe. Eine Klasse, deren
`CanBeCooked()` `false` liefert, wechselt ihre `FoodStage` nie; sie meldet Stufe 0
(`NONE`), und das Gericht wird nie fertig — ohne Log, ohne Meldung. Genau die
Fehlerart, die der Vanilla-Audit §4.2 aufzählt.

Nachgeschlagen wurde deshalb jede Klasse einzeln in `scripts - 1.29`:

| Klasse | `CanBeCooked()` | Folge für die Bindung |
|---|---|---|
| `Zucchini`, `Apple`, `Pear`, `Plum`, `CaninaBerry`, `SambucusBerry`, `SlicedPumpkin`, `Sardines`, `Bitterlings` | `true` (eigener Override) | dürfen in `VEGETABLE`, `FRUIT`, `BERRY`, `FISH` und damit in Pflicht-Slots |
| `Pumpkin` | **`false`** (eigener Override) | **nicht gebunden** — siehe unten |
| alle `*Can_Opened`, `Lunchmeat`, `Pate`, `Pajka`, `BrisketSpread`, `Rice`, `Honey` | `false` (Default aus `Edible_Base.c:129`) | eigene **Wurzel**kategorien bzw. nur optionale Slots |

### `Pumpkin` bleibt bewusst draußen

`Pumpkin.c` überschreibt `CanBeCooked()` mit `false`. Stünde der ganze Kürbis in
`VEGETABLE`, könnte ein Spieler damit den `VEGETABLE`-Pflichtslot von
`RCP_ChefZ_BoneBrothSoup` füllen — und die Suppe würde nie fertig. Gebunden ist
deshalb nur `SlicedPumpkin`; Vanilla liefert das Schneiden ohnehin selbst mit, ein
`TR_ChopPumpkin` wäre eine Doppelung.

### Warum `CANNED_MEAT`, `CANNED_FISH` und `CANNED_FRUIT` Wurzelkategorien sind

Sie hängen ausdrücklich **nicht** unter `MEAT`, `FISH` oder `FRUIT`. Der
Kategorie-Matcher läuft die Elternkette hinauf: ein Kind von `FISH` erfüllte jeden
`FISH`-Slot. `RCP_ChefZ_FishermanStew` (`ON_STAGE`, `doneStages: ["Boiled"]`) hätte
dann einen Pflichtslot, den eine Thunfischdose füllen kann — und der Eintopf wäre
tot. Dasselbe gilt für `RCP_ChefZ_ChernarusChili` (`MEAT`) und für den Fruchtslot
des Obstkompotts.

Konserven kommen deshalb nur dort hinein, wo ein Rezept sie **namentlich** einlädt:

- `RCP_ChefZ_FishPotatoPlate` — Fischslot ist jetzt
  `anyOf [ FISH, CANNED_FISH ]`. Das Rezept läuft auf `completion: "TIMED"`,
  die Garstufe wird dort gar nicht geprüft. Das ist der „Ersatzweg für Spieler ohne
  Angel" aus Audit §3 E; in der Fischersuppe geht er aus dem Grund oben nicht.
- `RCP_ChefZ_SausageBreadPlate` — neuer optionaler Slot `spread` auf
  `CANNED_MEAT`, ein Punkt. `completion: "INSTANT"`, kalt angerichtet, keine
  Garstufe im Spiel. Vier häufige Lootitems bekommen damit eine Kochrolle, ohne
  ein neues Rezept.
- `RCP_ChefZ_FruitCompote` — optionaler Slot `canned` auf `CANNED_FRUIT`.

### `Rice` und `Honey` (Audit §3 G)

Beide waren bisher nur per `cls` in einem einzigen Rezept gebunden, ohne Kategorie
und ohne `defaultState`. `Rice` bekommt `GRAIN`, `Honey` die neue Kategorie
`SWEETENER`. Folgenlos für bestehende Ketten: `TR_WheatToFlour` matcht hart
`cls ChefZ_Wheat`, und kein Rezept fragt `GRAIN` in einem Pflichtslot ab. Beide
kochen nicht — `SWEETENER` taucht deshalb ausschließlich in optionalen Slots auf.

### Was der Slice bewusst NICHT bindet

- **`PowderedMilk` → `DAIRY`** (Audit §3 B, dort als wichtigster Fall geführt).
  `PowderedMilk.c` hat keinen `CanBeCooked()`-Override und erbt damit `false`.
  Alle drei Rezepte mit `DAIRY`-Pflichtslot laufen auf `ON_STAGE`
  (`RCP_ChefZ_ScrambledEggSausage` → `Baked`, `RCP_ChefZ_MilkRice` und
  `RCP_ChefZ_MacAndCheese` → `Boiled`). Milchpulver dort hineinzulassen hieße,
  drei Gerichte um eine Zutat zu erweitern, mit der sie garantiert nie fertig
  werden. Ein `DAIRY`-Mitglied, das in einem Pflichtslot taugt, muss eine
  ChefZ-Klasse mit eigenen `FoodStages` sein — so wie `ChefZ_Butter` und
  `ChefZ_Cheese` es sind.
- **`SpaghettiCan_Opened` → `PASTA`** (Audit §3 E). Zwei Gründe, jeder für sich
  ausreichend: `CanBeCooked()` ist `false` (`PASTA` ist Pflichtslot in fünf
  `ON_STAGE`- und `TIMED`-Rezepten), und in Verbindung mit einer Vanilla-`DAIRY`
  wäre `RCP_ChefZ_MacAndCheese` vollständig mit Vanilla-Zutaten erfüllbar — ein
  Bruch von Invariante I2, den `chefzvanilla.mjs` nicht sieht, weil er `allOf`
  nicht auswertet.
- **`Marmalade`** (Audit §3 F). `ChefZ_IngredientManager.CollectChain` vererbt
  Zutatenfelder entlang der **CfgVehicles-Elternkette**. `Marmalade` ist die
  Config-Basis von `ChefZ_Cream`, `ChefZ_Egg`, `ChefZ_DriedBerries` und
  `ChefZ_SauceItemBase` (und damit der drei Saucen und der Brühe). Ein
  Zutaten-Record auf `Marmalade` wäre eine Vererbungsquelle quer durch den halben
  Mod; jede künftige Klasse mit dieser Basis, die ihre `categories[]` nicht selbst
  setzt, würde stillschweigend `SWEETENER`.
- **`RedCaviar`** (Audit §3 D). `nominal 0`, also dasselbe Verfügbarkeitsproblem
  wie beim Cauldron aus Audit §4.2 A. Ein optionaler Slot, den niemand füllen kann,
  ist kein Gewinn.
- **`Carp`, `Mackerel`, `SteelheadTrout`, `WalleyePollock`** (ganze Fische, Audit
  §3 D). Ausdrückliche Entscheidung: **Filetieren bleibt Vanilla.** Vanilla bringt
  die Aktion mit, die Filets sind bereits gebunden, und ein
  „ganzer Fisch → ChefZ-Fischstück"-Transform gehörte fachlich nach
  `ChefZ_Preservation`, nicht hierher.
- **`BoxCerealCrunchin`** (Audit §3 B). Sein einziger vorgeschlagener Zweck ist ein
  optionaler Slot in `RCP_ChefZ_MilkRice` — und dieses Rezept kann heute gar nicht
  fertig werden (siehe die Notiz zu `PowderedMilk`: `Rice` und `ChefZ_Milk` sind
  beide nicht kochbar, `doneStages` ist `Boiled`). Einen Slot an ein totes Rezept
  zu hängen bringt nichts.
- **`CatFoodCan`, `DogFoodCan`, `UnknownFoodCan`, `HumanSteakMeat`** — wie im Audit
  festgehalten, bewusst draußen.
