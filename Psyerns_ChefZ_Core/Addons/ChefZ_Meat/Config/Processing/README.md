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
