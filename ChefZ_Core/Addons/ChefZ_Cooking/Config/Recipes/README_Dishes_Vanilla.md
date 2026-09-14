# `DishesVanilla.json` — zwei Gerichte aus ungenutzten Vanilla-Assets

> **Entfernt am 14.09.2026: das Obstkompott** (`RCP_ChefZ_FruitCompote`, `ChefZ_FruitCompoteBowl`)
> auf Auftrag „wir entfernen ChefZ_FruitCompoteBowl". Rezept, Klasse, Strings und Delta-Eintrag
> sind weg. Geblieben sind `ChefZ_DriedBerries` (jetzt ohne Rezept) und die Vanilla-Bindungen
> `FRUIT`, `BERRY`, `CANNED_FRUIT`, `SWEETENER` — ein künftiges Obstrezept findet sie vor.
> Die Abschnitte unten sind auf zwei Gerichte gekürzt.

Slice `dishes-vanilla`. Grundlage: `ChefZ_Vanilla_Audit.md` §3
(„Ungenutzte Vanilla-Assets: Vorschlagsliste").

Die Notiz steht neben der Datei und nicht darin, aus demselben Grund wie bei
`Sauces.json`: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()` hält fest, dass
die Toleranz des Enforce-Serializers gegenüber unbekannten JSON-Feldern **nicht
belegt** ist. Ist er intolerant, wird die Datei komplett verworfen — und dann
ließe sich keines der Gerichte mehr kochen, ohne dass etwas darauf hinweist.
Content-Autoren dürfen keine Kommentarfelder in JSON schreiben.

## 1. Warum genau diese

Rund 106 Vanilla-Klassen waren ungebunden. Der weitaus größte Teil davon gehört in
ein **vorhandenes** Gericht und braucht kein neues — das ist der Inhalt von
`Config/Ingredients/README.md` im Modul `ChefZ_Ingredients` und der Grund, warum
hier nur zwei Rezepte stehen und nicht zwölf. Ein neues Gericht gibt es nur dort,
wo es für eine Zutat keinen einzigen bestehenden Teller und keine bestehende
Schüssel gibt:

| Gericht | Was es erschließt |
|---|---|
| `RCP_ChefZ_PumpkinSoup` | `SlicedPumpkin` — Vanillas geschnittener Kürbis hatte gar keinen Weg in ein ChefZ-Gericht. |
| `RCP_ChefZ_SmallFishPan` | `Sardines`, `Bitterlings` — der häufigste Angelfang in Vanilla, ohne Filet-Pendant und damit bisher wertlos (Audit §3 D). |

## 2. Die I2-Anker

Invariante I2: ein Rezept darf sich nicht vollständig mit Vanilla-Zutaten erfüllen
lassen. Da dieser Slice *aus* Vanilla-Assets baut, ist der Anker die erste
Entwurfsfrage jedes Rezepts und nicht die letzte. Beide Anker sind
**verarbeitete Zwischenprodukte mit vollständigen `FoodStages`** — ein Gewürz taugt
dafür nicht, weil `ChefZ_RecipeEvaluator.CheckStages` von jeder gebundenen
Pflichtzutat eine gültige Endstufe verlangt.

| Gericht | Anker-Slot | erfüllbar durch | Kette |
|---|---|---|---|
| Kürbissuppe | `fat` → `{ "category": "BUTTER" }` | nur `ChefZ_Butter` | Milch → Sahne → Butter (`ChefZ_ButterChurn`) |
| Kleinfischpfanne | `garlic` → `cls ChefZ_Garlic` | nur ChefZ-Klassen | Knoblauch anbauen, mit dem Messer schneiden |

Jeder dieser Anker ist zugleich **fachlich** die Zutat, die aus der Zutatensammlung
ein Gericht macht: Kürbis in Butter anschwitzen, Kleinfisch mit Knoblauch braten. Wer die
Vanilla-Zutaten allein in den Topf
legt, kocht weiter wie in Vanilla.

## 3. Verdeckungsprüfung (09 §4)

Beide stehen auf `extraItems: "forbid"`. Das ist hier nicht nur Politik, sondern
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

## 4. Warum `ON_STAGE` und nicht `TIMED`

Wie in `BowlDishes.json`: Vanilla besitzt bereits ein vollständiges Gar- und
Verbrennungssystem, `doneStages` nennt die Endstufe, `policy.forbiddenStates`
schließt das Überkochen aus. Das setzt aber voraus, dass **jede** Pflichtzutat diese
Endstufe auch erreichen kann. Nachgeschlagen wurde deshalb jede einzelne in
`scripts - 1.29`:

- `SlicedPumpkin`, `Sardines`, `Bitterlings` — eigener `CanBeCooked()`-Override auf `true`.
- `ChefZ_Butter`, `ChefZ_Garlic`, `Lard` — `FoodStages` mit
  Übergängen aus `Raw` nach `Baked` und `Boiled`.
- `ChefZ_DriedBerries` — trägt eigens einen Übergang `Dried -> Boiled`, den keine
  andere Trockenware des Mods hat. Er war für das Kompott gebaut; seit dessen
  Wegfall am 14.09.2026 nutzt ihn kein Rezept, er bleibt für das nächste Obstrezept.

Was aus genau diesem Grund **nicht** im Pflichtteil steht:

- `Pumpkin` (ganz) — `CanBeCooked()` ist `false`. Nur `SlicedPumpkin`.
- `PeachesCan_Opened`, `Honey` — `CanBeCooked()` ist `false`; sie standen nur in
  optionalen Slots des entfernten Kompotts. Die Regel bleibt für ein künftiges
  Obstrezept: Konserven und Honig nie in einen Pflichtslot.

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
| Kleinfischpfanne | 8 (Kräuter 3 + Salz 2 + Gewürz 1 + Petersilie 1 + frische Kräuter 1) |

Beide erreichen `PREMIUM`. Das ist keine Kosmetik: die Blutwurstplatte kam
genau daran nicht vorbei und blieb bei 6 stehen (siehe `README_Dishes_A.md` §7).

## 7. Portionen und die zweite Portionssperre (15 §5.2)

`amountPerPortion` ist bei beiden gesetzt, sonst ergäbe eine Minimalfüllung die
volle Portionszahl:

- Kürbissuppe: 4 Pflichteinheiten / 1.3 → 3 Portionen
- Kleinfischpfanne: 6 Pflichteinheiten / 2.0 → 2 Portionen (Tellerdeckel)

Optionale Slots zählen dabei nicht mit — Gewürze können die Ausbeute nicht
hochkaufen.
