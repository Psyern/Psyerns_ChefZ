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
