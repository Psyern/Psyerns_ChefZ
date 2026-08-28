# `DishesVanilla.json` — drei Gerichte aus ungenutzten Vanilla-Assets

Slice `dishes-vanilla`. Grundlage: `ChefZ_Vanilla_Audit.md` §3
(„Ungenutzte Vanilla-Assets: Vorschlagsliste").

Die Notiz steht neben der Datei und nicht darin, aus demselben Grund wie bei
`Sauces.json`: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()` hält fest, dass
die Toleranz des Enforce-Serializers gegenüber unbekannten JSON-Feldern **nicht
belegt** ist. Ist er intolerant, wird die Datei komplett verworfen — und dann
ließe sich keines der drei Gerichte mehr kochen, ohne dass etwas darauf hinweist.
Content-Autoren dürfen keine Kommentarfelder in JSON schreiben.

## 1. Warum genau diese drei

Rund 106 Vanilla-Klassen waren ungebunden. Der weitaus größte Teil davon gehört in
ein **vorhandenes** Gericht und braucht kein neues — das ist der Inhalt von
`Config/Ingredients/README.md` im Modul `ChefZ_Ingredients` und der Grund, warum
hier nur drei Rezepte stehen und nicht zwölf. Ein neues Gericht gibt es nur dort,
wo es für eine Zutat keinen einzigen bestehenden Teller und keine bestehende
Schüssel gibt:

| Gericht | Was es erschließt |
|---|---|
| `RCP_ChefZ_PumpkinSoup` | `SlicedPumpkin` — Vanillas geschnittener Kürbis hatte gar keinen Weg in ein ChefZ-Gericht. |
| `RCP_ChefZ_SmallFishPan` | `Sardines`, `Bitterlings` — der häufigste Angelfang in Vanilla, ohne Filet-Pendant und damit bisher wertlos (Audit §3 D). |
| `RCP_ChefZ_FruitCompote` | `Apple`, `Pear`, `Plum`, `CaninaBerry`, `SambucusBerry`, `Honey`, `PeachesCan_Opened` — das erste süße Gericht des Mods überhaupt. |

## 2. Die I2-Anker

Invariante I2: ein Rezept darf sich nicht vollständig mit Vanilla-Zutaten erfüllen
lassen. Da dieser Slice *aus* Vanilla-Assets baut, ist der Anker die erste
Entwurfsfrage jedes Rezepts und nicht die letzte. Alle drei Anker sind
**verarbeitete Zwischenprodukte mit vollständigen `FoodStages`** — ein Gewürz taugt
dafür nicht, weil `ChefZ_RecipeEvaluator.CheckStages` von jeder gebundenen
Pflichtzutat eine gültige Endstufe verlangt.

| Gericht | Anker-Slot | erfüllbar durch | Kette |
|---|---|---|---|
| Kürbissuppe | `fat` → `{ "category": "BUTTER" }` | nur `ChefZ_Butter` | Milch → Sahne → Butter (`ChefZ_ButterChurn`) |
| Kleinfischpfanne | `garlic` → `anyOf [ ChefZ_ChoppedGarlic, ChefZ_Garlic ]` | nur ChefZ-Klassen | Knoblauch anbauen, mit dem Messer schneiden |
| Obstkompott | `berries` → `{ "cls": "ChefZ_DriedBerries" }` | nur `ChefZ_DriedBerries` | Waldbeeren am `ChefZ_DryingRack` trocknen |

Jeder dieser Anker ist zugleich **fachlich** die Zutat, die aus der Zutatensammlung
ein Gericht macht: Kürbis in Butter anschwitzen, Kleinfisch mit Knoblauch braten,
getrocknetes Obst im Kompott mitkochen. Wer die Vanilla-Zutaten allein in den Topf
legt, kocht weiter wie in Vanilla.

## 3. Verdeckungsprüfung (09 §4)

Alle drei stehen auf `extraItems: "forbid"`. Das ist hier nicht nur Politik, sondern
das Trennwerkzeug: ein Rezept bindet nicht, wenn im Gefäß etwas liegt, für das es
keinen Slot hat.

1. **Kürbissuppe vs. `RCP_ChefZ_BoneBrothSoup` / `RCP_ChefZ_VegetableSoup`.**
   `SlicedPumpkin` ist `VEGETABLE` und würde deren Gemüseslots mitfüllen. Die
   Trennung trägt die **Butter**: keines der beiden Rezepte hat einen Slot, in den
   sie passt, also weist `forbid` sie ab. Umgekehrt verlangt die Kürbissuppe
   `SlicedPumpkin`, das kein anderes Rezept kennt.
2. **Kleinfischpfanne vs. `RCP_ChefZ_FishPotatoPlate`.** Beide laufen in der Pfanne,
   und `Sardines` ist seit diesem Slice `FISH`. Die Trennung trägt der
   **Knoblauch** — im Tellergericht gibt es keinen Slot dafür — und umgekehrt die
   **Kartoffel**, für die die Pfanne keinen Slot hat.
3. **Obstkompott.** Kein anderes Rezept des Mods kennt `FRUIT`, `BERRY` oder
   `CANNED_FRUIT`.

## 4. Warum `ON_STAGE` und nicht `TIMED`

Wie in `BowlDishes.json`: Vanilla besitzt bereits ein vollständiges Gar- und
Verbrennungssystem, `doneStages` nennt die Endstufe, `policy.forbiddenStates`
schließt das Überkochen aus. Das setzt aber voraus, dass **jede** Pflichtzutat diese
Endstufe auch erreichen kann. Nachgeschlagen wurde deshalb jede einzelne in
`scripts - 1.29`:

- `SlicedPumpkin`, `Sardines`, `Bitterlings`, `Apple`, `Pear`, `Plum`,
  `CaninaBerry`, `SambucusBerry` — eigener `CanBeCooked()`-Override auf `true`.
- `ChefZ_Butter`, `ChefZ_Garlic`, `ChefZ_ChoppedGarlic`, `Lard` — `FoodStages` mit
  Übergängen aus `Raw` nach `Baked` und `Boiled`.
- `ChefZ_DriedBerries` — trägt eigens einen Übergang `Dried -> Boiled`, den keine
  andere Trockenware des Mods hat. Ohne ihn fiele
  `FoodStage.GetNextFoodStageType` auf `BURNED` zurück (`FoodStage.c:472`) und das
  Kompott würde nie fertig.

Was aus genau diesem Grund **nicht** im Pflichtteil steht:

- `Pumpkin` (ganz) — `CanBeCooked()` ist `false`. Nur `SlicedPumpkin`.
- `PeachesCan_Opened` — `CanBeCooked()` ist `false`. Steht deshalb im **optionalen**
  Slot `canned`; optionale Slots zählen bei `CheckStages` ausdrücklich nicht mit.
  Der Pflichtslot `fruit` schließt Konserviertes zusätzlich aus:
  `allOf [ FRUIT, not CHEFZ_PRESERVED ]`.
- `Honey` — `CanBeCooked()` ist `false`, also nur der optionale Slot `sweet`
  (`SWEETENER`) plus eine `gradeRule`.

## 5. Kein Mehlslot in der Kleinfischpfanne

Mehlierter Backfisch wäre das naheliegende Gericht, und `ChefZ_Flour` wäre ein
sauberer I2-Anker. Er ist trotzdem nicht drin, weder als Pflicht- noch als Wahlslot:

`ChefZ_GrainFoodBase` kennt genau **einen** Übergang, `Raw -> Baked` über `BAKING`.
Aus `Baked` gibt es keinen — `GetNextFoodStageType` liefert dort also `BURNED`
(`FoodStage.c:472`), und die `cooking_properties` der Stufe `Baked` sind
`{100, 40, 200}`: vierzig Sekunden nach dem Garwerden geht das Mehl in Kohle über.
Der Fisch braucht länger. `policy.forbiddenStates: ["BURNT"]` bräche die Bindung
dann wieder auf — das Gericht wäre ein Wettlauf, den das Mehl gewinnt. Die zwei
Qualitätspunkte, die dafür vorgesehen waren, liegen jetzt auf dem Kräuterslot.

## 6. Die Qualitätsleiter

Schwellen aus `CfgChefZQualityTiers` / `DISH_DEFAULT`: `PREPARED` 2, `SEASONED` 4,
`PREMIUM` 7. Pflicht-Slots geben null Punkte — „Simple" ist per Definition nur die
Grundzutat.

| Gericht | erreichbare Summe |
|---|---|
| Kürbissuppe | 9 (Wurzelgemüse 1 + Sahne 2 + Kräuter 2 + Salz 1 + Gewürz 1 + Thymian 1 + frische Kräuter 1) |
| Kleinfischpfanne | 8 (Kräuter 3 + Salz 2 + Gewürz 1 + Dill 1 + frische Kräuter 1) |
| Obstkompott | 9 (Süßung 3 + Konserve 1 + Sahne 1 + Gewürz 1 + Honig 2 + frisches Obst 1) |

Alle drei erreichen `PREMIUM`. Das ist keine Kosmetik: die Blutwurstplatte kam
genau daran nicht vorbei und blieb bei 6 stehen (siehe `README_Dishes_A.md` §7).

## 7. Portionen und die zweite Portionssperre (15 §5.2)

`amountPerPortion` ist bei allen drei gesetzt, sonst ergäbe eine Minimalfüllung die
volle Portionszahl:

- Kürbissuppe: 4 Pflichteinheiten / 1.3 → 3 Portionen
- Kleinfischpfanne: 6 Pflichteinheiten / 2.0 → 2 Portionen (Tellerdeckel)
- Obstkompott: 5 Pflichteinheiten / 1.6 → 3 Portionen

Optionale Slots zählen dabei nicht mit — Gewürze können die Ausbeute nicht
hochkaufen.
