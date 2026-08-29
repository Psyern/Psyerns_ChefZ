# Sauces.json — Anmerkungen zu den vier Saucen- und Bruehenrezepten

Slice `sauces`. Production Map §52–§55, DME-Plan §34.

Die Notiz steht neben der Datei und nicht darin: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()`
haelt fest, dass die Toleranz des Enforce-Serializers gegenueber unbekannten
JSON-Feldern **nicht belegt** ist. Ist er intolerant, wird die Datei **komplett**
verworfen — dann liesse sich keine Sauce mehr kochen, ohne dass eine Meldung
darauf hinweist. Content-Autoren duerfen deshalb keine Kommentarfelder in JSON
schreiben.

## Warum `completion: "TIMED"` und nicht `ON_STAGE`

`ON_STAGE` bindet den Abschluss an Vanillas Garstufe der Zutaten. Die Zutaten
dieser vier Rezepte — geschnittenes Gemuese, Sahne, Butter, Salz, getrocknete
Kraeuter, Pilze — tragen bewusst **keine** `FoodStages` (siehe den Kopf von
`ChefZ_Ingredients/config.cpp`: "Zutat, nicht Garobjekt"). Ein ON_STAGE-Rezept
darauf wuerde nie zuenden, und zwar lautlos.

Das ist hier aber kein Ausweg, sondern die richtige Aussage: eine Sauce koechelt
eine Zeit lang, sie erreicht keine Garstufe. `cookSeconds` reicht von 120 s
(Rahmsauce) bis 420 s (Knochenbruehe) und ist damit die Kostenachse dieses Slice.

`allowTimedRecipes` steht in `Core.json` auf `true`. Ist es abgeschaltet,
klammert der `ChefZ_RecipeCompiler` auf ON_STAGE mit Default-`doneStages` und
WARN (08 §8) — die Rezepte gehen also nicht verloren, sie werden nur ungenauer.

## Warum die Slots auf Kategorien zeigen und nicht auf Klassen

08 E4. Ein Modul, das spaeter eine zweite Tomatenform oder einen weiteren
essbaren Pilz mitbringt, traegt ihn in die Kategorie ein — und erbt jedes dieser
Rezepte, ohne dass hier eine Zeile angefasst wird.

Dafuer hat dieser Slice drei Kategorien **additiv** an bestehende
Zutatensaetze gehaengt (Delta `_deltas/sauces.json`):

| Kategorie | haengt an | Datei |
|---|---|---|
| `TOMATO` | `Tomato` | `ChefZ_Ingredients` |
| `CREAM` | `ChefZ_Cream` | `ChefZ_Ingredients/Config/Ingredients/Dairy.json` |
| `BUTTER` | `ChefZ_Butter` | `ChefZ_Ingredients/Config/Ingredients/Dairy.json` |
| `MUSHROOM` | sieben Vanilla-Pilze | `ChefZ_Ingredients/Config/Ingredients/Mushrooms.json` |

Rein additiv: `VEGETABLE` und `DAIRY` bleiben stehen, jeder bestehende Selektor
trifft weiter.

## `policy.extraItems: "forbid"` in allen vier Rezepten

Das ist der Default aus `Core.json` (`defaultExtraItems`). Er steht trotzdem
ausgeschrieben da: eine Sauce, die zufaellig mitgekochtes Fleisch stillschweigend
duldet, waere ein Rezept, dessen Ergebnis der Spieler nicht vorhersagen kann.
Was die Qualitaet heben darf, steht als **optionaler Slot** drin und ist damit
benannt — `aromatics` und `herbs` in `RCP_ChefZ_TomatoSauce` (§52
"Optional: + Garlic + Herbs").

## Einzelheiten

### `RCP_ChefZ_TomatoSauce`

§52. Der Slot `aromatics` matcht `ROOT_VEGETABLE` und nicht eine eigene
Kategorie `GARLIC`: Knoblauch und Zwiebel gehoeren beide in ein Sugo, und eine
Kategorie mit genau einem Mitglied waere Ballast in einer Registry, die alle
Slices teilen.

### `RCP_ChefZ_CreamSauce`

§53. Die Pfanne ist als Geraet zugelassen — eine Rahmsauce entsteht dort so gut
wie im Topf. Sahne und Butter werden `whole` verbraucht: das ist der Preis, und
beide kommen erst aus Butterfass und Kuehlkette.

### `RCP_ChefZ_MushroomCreamSauce`

§54. `priority: 20` gegenueber `0` bei der einfachen Rahmsauce: liegen Pilze mit
im Topf, ist die Pilzrahmsauce gemeint. Petersilie ist die im Rezept genannte
Kraeuterwahl, aber keine Pflicht — deshalb nimmt der Slot jedes Kraut
(`CHEFZ_HERB`) und die Grade-Regel `GR_Parsley` gibt den Punkt fuer die richtige
Wahl. Ein Selektor in einer Grade-Regel prueft, was ohnehin im Gefaess liegt; er
fordert nichts an.

### `RCP_ChefZ_BoneBroth`

§55. Das einzige Rezept dieses Slice mit Fluessigkeitsbindung
(`requiresLiquid`, `liquidTypes: ["Water"]`, `liquidQuantity.min = 200`) — eine
Bruehe ohne Wasser im Topf gibt es nicht. §55 nennt Zwiebel **und** Karotte; der
Slot `aromatics` verlangt stattdessen zwei Stueck `ROOT_VEGETABLE`: die Bruehe
will Wurzelgemuese, nicht ausgerechnet diese beiden Sorten — und wer nur
Zwiebeln hat, soll trotzdem kochen koennen.
