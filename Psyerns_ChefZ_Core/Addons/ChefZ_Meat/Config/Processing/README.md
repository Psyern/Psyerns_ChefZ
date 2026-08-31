# Meat.json — Anmerkungen zu den Transforms

Diese Texte standen bis M2 als `_comment`-Felder **innerhalb der Records** in
`Meat.json`. Grund der Verlagerung: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()`
haelt fest, dass die Toleranz des Enforce-Serializers gegenueber unbekannten
JSON-Feldern **nicht belegt** ist. Ist er intolerant, wird die Datei **komplett**
verworfen — samtliche Fleisch-Transforms fielen still aus. Der Selbsttest sagt
woertlich: "Content-Autoren duerfen keine Kommentarfelder in JSON schreiben."

Die Notizen stehen neben der Datei und nicht im Kopf der `config.cpp`, weil
Transforms laut 11 §5 ausdruecklich **nicht** in die `config.cpp` gehoeren — ihre
Erlaeuterung dorthin zu verschieben haette den Text von seinem Gegenstand getrennt.

## Transforms in der Reihenfolge der Datei

### `TR_DicedMeat` — entfallen (29.08.2026)

Die Eintoepfe nehmen gewolftes Fleisch (`MINCED_MEAT`); ein rohes Vanilla-Steak im
Topf bleibt Vanilla-Kochen (Invariante I2). Die Wuerfelstufe war ein Nachbau.

### `TR_CutBeefLeg`, `TR_CutPorkLeg`, `TR_CutVenisonLeg`

Die Keule zerteilen: `PROCESS_CUT_MEAT`, ein Eingang plus Messer. Ergebnis sind
**zwei VANILLA-Steaks** der passenden Sorte plus ein Nebenausgang — Knochen bei
Rind und Wild, Fett beim Schwein.

**Warum zwei Ausgangseintraege derselben Klasse und nicht `quantity: 2`:**
`quantity` ist in `ChefZ_OutputDef` die Menge AM Item
(`ChefZ_ProcessRunner.ApplyHandcraftLayer` ruft `item.SetQuantity(value)`), nicht
die Stueckzahl. `quantity: 2` an einem `CowSteakMeat` ergaebe **ein** fast leeres
Steak. Zwei Stueck sind zwei Eintraege; `ChefZ_GenericCraftRecipe.AddOutputs`
ruft `AddResult()` je Eintrag. `quantity` fehlt bewusst ganz — der Sentinel laesst
die Klassenvorgabe stehen, und die kennt nur Vanilla.

**Warum kein `setState` und kein `inheritFreshness`:** aus demselben Grund, der
weiter unten beim `Lard` aus `TR_MeatToMinced` steht. Beide werden in
`ApplyHandcraftLayer` nur innerhalb von `if (ChefZ_ItemStateComponent.IsManaged(item))`
angewandt; ein Vanilla-Steak ist dort nicht drin. Der Eintrag waere wirkungslose
Daten.

**Warum die Keule ueber `cls` und nicht ueber eine Kategorie gematcht wird:**
siehe `../Ingredients/README.md`, Abschnitt "Keulen".

`priority 20` wie bei den Sortenregeln, damit die Rangfolge lesbar bleibt.

### `TR_MeatToMinced`

§30/§57: Meat -> Minced Meat am Fleischwolf. Der gattungsneutrale Fall mit
`priority 0` — jede Sortenregel unten schlaegt ihn.

**Zum Nebenausgang `Lard` in `byproducts[]`:** §31: Tierfett faellt beim
Wolfen an. Nicht bei jedem Durchgang — `chance 0.35` — sonst waere Fett kein Fund,
sondern eine Selbstverstaendlichkeit. Ausgegeben wird Vanillas `Lard` und keine
eigene Fettklasse (Vanilla-Audit §2): alle Fett-Slots matchen ueber die Kategorie
`FAT`, und `Lard` traegt sie bereits. `setState` fehlt hier bewusst — ein
Vanilla-Item kann keinen ChefZ-Zustand tragen (`ChefZ_ItemStateComponent.Of()`
liefert `null`), der Eintrag waere wirkungslose Daten.

### `TR_PorkToMinced`, `TR_VenisonToMinced`, `TR_BoarToMinced`, `TR_ChickenToMinced`, `TR_BearToMinced`

§30: die Sorten. Das Ausgangsfleisch bleibt erkennbar — genau das verlangt §30
("Das Ausgangsfleisch sollte intern gespeichert bzw. klassifiziert werden").
Umgesetzt als eigene Klasse und nicht als Variable am Item: eine Variable haette
die Naehrwerte nicht mitgenommen, denn die haengen an Klasse x Garstufe (13 §2).

### `TR_SausageCasing` — entfallen (29.08.2026)

Vanillas `Guts` und `SmallGuts` sind die Huelle (Kategorie `CASING`); die
Wurst-Transforms nehmen sie direkt.

### `TR_RawSausage`

§34: die Basiswurst. Hack + EIN Gewuerz + Huelle. `priority 0` — jede Sortenregel
unten ist genauer und gewinnt. Der Gewuerzslot matcht ueber die KATEGORIE und
nicht ueber `ChefZ_Salt`: die Salz- und Kraeuterkette gehoert einem anderen Slice,
und ein Klassenname aus einem fremden Slice waere eine Bindung, die bricht, sobald
dort umbenannt wird.

### `TR_RawPorkSausage`

§35: Minced Pork + Salz + Schwarzer Pfeffer + Huelle. Zwei Gewuerzslots — der
Matcher bindet jedem Slot ein ANDERES Item (07 §4), es braucht also wirklich zwei
verschiedene Gewuerze.

### `TR_RawVenisonSausage`

§36: Minced Venison + Salz + Thymian + Pfeffer + Huelle. Der Kraeuterslot nimmt
HERB, DRIED_HERB oder SPICE — welche Kategorie der Kraeuter-Slice dem Thymian am
Ende gibt, entscheidet er, nicht dieser Datensatz.

### `TR_RawBoarSausage`

§37: Minced Boar + Salz + Pfeffer + Baerlauch + Huelle.

### `TR_RawHunterSausage`

§38: Wild Meat + Salz + Hunter Seasoning + Huelle. Als EINZIGE Sorte direkt aus
rohem Wildfleisch statt aus Hack — so steht es in §38 und so steht es im Pruefstein
20 §2.2. `minCount 2`, weil zwei Stuecke Wild in eine Wurst gehen.

### `TR_RawSpicySausage`

§39: Minced Meat + Salz + Pfeffer + Paprikapulver + Huelle. Drei Gewuerzslots —
das ist zugleich die Unterscheidung zur Basiswurst (ein Slot) und zur Schweinswurst
(zwei).

## Mengenskala — warum in dieser Datei 250 steht und nicht 1

Stand 31.08.2026, Harmonisierung mit dem Slice `preservation`.

Jede essbare Klasse dieses Moduls fuehrt seit dem Rescale
`varQuantityMax = 250` (`ChefZ_MeatItemBase`, Begruendung im Kopf der
`config.cpp`). Fuer die Rezeptdaten zerfaellt das in zwei Faelle, und die
Unterscheidung ist der ganze Inhalt dieses Abschnitts.

### Roh — `quantity` in `outputs` und `byproducts`

`quantity` wird ohne jede Umrechnung an `SetQuantity` durchgereicht:

    ChefZ_ProcessRunner.c:335-343     else if (def.HasQuantity())
                                          value = def.quantity;
                                      ...
                                      if (value > 0.0) item.SetQuantity(value);
    ChefZ_Applicator.c:975-976        dieselbe Stelle auf dem Kochweg

Eine `1` an einer 250er-Klasse ergaebe ein Item mit 0,4 % Fuellung. Alle
ChefZ-Ausgaenge dieser Datei tragen deshalb `250`.

`quantityMode: "fromInput"` ist bei den sechs `TR_*ToMinced` **gestrichen**
(jetzt `fixed` / 250). Der Eingang ist dort ein Vanilla-Steak, dessen Menge in
einer fremden Skala steht; `fromInput` haette diese fremde Zahl in eine
250er-Klasse geschrieben. `fromInput` bleibt nur, wo Ein- und Ausgang dieselbe
Skala fuehren — das ist `Config/Recipes/Sausage.json`, siehe die README dort.

Die drei `Lard`-Beiprodukte haben ihr `quantity`-Feld ganz verloren. `Lard` ist
eine VANILLA-Klasse mit eigener Skala, die dieses Projekt nicht belegen kann;
ohne das Feld bleibt `value` auf `-1` (`ChefZ_ProcessRunner.c:329`) und der
Klassendefault steht. Genau so halten es die drei Keulen-Transforms mit `Bone`
und den `*SteakMeat`-Ausgaengen seit jeher.

### Verhaeltnis — `consumeAmount` in `inputs`

Hier war **nichts** zu tun, und das ist nachgerechnet, nicht vermutet:

    ChefZ_FactCollector.c:439    units = quantity / quantityMax * unitsPerWholeItem
    ChefZ_SlotEvaluator.c:364    float perUnit = facts.quantity / facts.units;

`units` ist ein Verhaeltnis. Bei `unitsPerWholeItem = 1` — und den Wert tragen
alle Datensaetze in `../Ingredients/Meat.json` — ist ein volles Item genau eine
Einheit, ob `quantityMax` nun 1 oder 250 heisst. `consumeAmount: 1` bedeutet
unveraendert "ein ganzes Stueck". Das gilt auch fuer die Slots FREMDER Slices,
die Fleisch ueber `MINCED_MEAT` verbrauchen (`ChefZ_Cooking/Config/Recipes/
BowlDishes.json` und `DishesB.json`); an denen war deshalb nichts zu aendern,
und es durfte auch nichts geaendert werden.
