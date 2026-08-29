# Dishes_A.json — die Tellergerichte 1–10

Slice `dishes-a`. Production Map §61.1–§61.10, DME-Plan §38, §41 (Rezeptqualität),
§42 (Gerichtsnutzen), §43 (Food-Buffs).

Der Text steht **neben** der Datei und nicht darin: `ChefZ_ConfigSelfTest.
ProbeUnknownFieldTolerance()` hält fest, dass die Toleranz des Enforce-Serializers
gegenüber unbekannten JSON-Feldern **nicht belegt** ist. Ist er intolerant, wird eine
Datei mit Kommentarfeld **komplett** verworfen — dann liesse sich kein einziges
dieser zehn Gerichte mehr kochen, ohne dass irgendetwas darauf hinweist.

## 1. `completion: "TIMED"` und nicht `ON_STAGE`

`ON_STAGE` verlangt, dass **jede gebundene Pflichtzutat** in einer erlaubten
Vanilla-Endstufe steht (`ChefZ_RecipeEvaluator.CheckStages`, Kommentar dort
wörtlich). Sieben der zehn Gerichte haben eine Pflichtzutat **ohne** `FoodStages`:
frische Kräuter, Sahne, Milch, Käse, Zwiebel. Deren Garstufe ist `NONE` und wird nie
`Boiled` — ein `ON_STAGE`-Rezept darauf würde nie zünden, und zwar lautlos.

`TIMED` ist damit hier keine Bequemlichkeit, sondern die einzige Variante, die
zündet. Dieselbe Begründung wie bei den vier Saucen (`README.md` §1). `cookSeconds`
reicht von 180 s (Spaghetti, Bohnenteller) bis 260 s (Jägerteller) und ist die
Kostenachse dieses Slice. `allowTimedRecipes` steht in `Core.json` auf `true`; ist es
abgeschaltet, klammert der Compiler auf `ON_STAGE` mit Default-`doneStages` und WARN
(08 §8) — die Rezepte gehen nicht verloren, sie werden nur ungenauer.

## 2. Salz ist überall ein **optionaler** Slot

Production Map §61 nennt Salz bei vier der zehn Gerichte als Zutat. Es steht trotzdem
in **allen zehn** als optionaler Slot, und zwar aus zwei Gründen:

1. **Architekturplan §9 baut die Qualitätsstufen genau so auf**: „Noodles + Sausage →
   SIMPLE", „+ Tomato Sauce + Salt → PREPARED". Salz ist der Schritt von der
   Grundzutat zum vollständigen Rezept — als Pflichtslot gäbe es keine SIMPLE-Stufe
   mehr, sondern nur noch „geht" und „geht nicht".
2. **Der Mengendeckel bliebe sonst wirkungslos.** `ChefZ_PortionManager.
   ConsumedRequiredUnitsOf` zählt bei einem Mengenslot den **Einheitenbetrag**:
   5 g Salz sind fünf Einheiten. Ein Pflicht-Salzslot hätte die Zutatenmenge jedes
   Gerichts rechnerisch mehr als verdoppelt und `amountPerPortion` damit ausgehebelt
   (15 §5.2). Optionale Slots zählen ausdrücklich nicht mit — genau dafür gibt es
   die Unterscheidung.

Dieselbe Logik trägt die Butter im Mac & Cheese: optional, hebt die Stufe.

## 3. Die Qualitätsrechnung, auf die die Slots gerechnet sind

Stufenschwellen aus `CfgChefZQualityTiers` in der `config.cpp` dieses Moduls
(SIMPLE 0 · PREPARED 2 · SEASONED 4 · PREMIUM 7, aus 12 §3 / Architekturplan §9):

| Beitrag | Punkte | Woher |
|---|---|---|
| Pflichtzutaten des Gerichts | 0–1 | `gradePoints` am Slot |
| Salz | 2 | optionaler Slot |
| Gewürz (`CHEFZ_SPICE`) | 1 | optionaler Slot |
| Kraut (`CHEFZ_HERB`) | 1 | optionaler Slot |
| Premiumzutat (Sahne, Sauce, Butter) | 1–2 | optionaler Slot |
| Signaturzutat (Thymian, Petersilie, Paprikapulver, Hunter Seasoning, Hunter Sausage) | 1–3 | `gradeRules` |

Damit ergibt sich: Grundzutaten allein → **SIMPLE**; mit Salz → **PREPARED**;
zusätzlich Gewürz und Kraut → **SEASONED**; dazu die Premium- oder Signaturzutat →
**PREMIUM**. Die Frische geht laut 12 §4.1 zusätzlich als **Minimum** ein — eine
einzige fast verdorbene Zutat drückt das Gericht, und „altes Fleisch in ein
Premiumgericht waschen" ist damit keine Strategie.

`gradeRules` dürfen auf **Klassen** zeigen (`ChefZ_Thyme`, `ChefZ_PaprikaPowder`), die
Slots dagegen nicht — siehe §5.

## 4. Ausbeute: `portions = 2`, `amountPerPortion = 2.0`

Ein Tellergericht ist ein Portionsgericht (15 E7). Die beiden Deckel wirken so:

```text
Minimalfuellung  (3 Pflichteinheiten)  -> floor(3/2) = 1 Teller
Volle Pfanne     (5 Pflichteinheiten)  -> floor(5/2) = 2 Teller (Deckel: portions)
FryingPan                              -> Geraetedeckel 2
```

Die Nährwerte der Gerichtklassen sind auf **eine Portion aus einer Minimalfüllung**
gerechnet (Herleitung steht über jeder Klasse in der `config.cpp`). Beides passt
damit zusammen: doppelte Zutaten ergeben zwei Teller, nicht einen doppelt so guten.

Die Qualitätsstufe wirkt **nach** beiden Deckeln (`yieldMultiplier`, `portionBonus`) —
das ist der einzige Kanal, über den Qualität sättigen kann, weil der Nährwert je
Bissen an Klasse × Foodstage hängt und sich nicht je Instanz ändern lässt
(01 V6, 12 §2).

## 5. Warum die Slots auf Kategorien zeigen — und die drei Ausnahmen

08 E4: ein Modul, das später eine weitere Wurst oder einen weiteren Pilz mitbringt,
trägt ihn in die Kategorie ein und erbt jedes dieser Rezepte.

Es kommt hier ein zweiter, härterer Grund dazu. `chefzstage` (Befund 01 V4) wertet
**jede Klasse, die als `cls` in einem Slot steht, als „liegt im Kochgerät"** und
verlangt von ihr `FoodStageTransitions`. Die Zutatenklassen dieses Projekts haben
bewusst keine — sie sind Zutat, nicht Garobjekt. Ein Slot `{"cls":"ChefZ_Cheese"}`
ist deshalb ein **Fehler im Validator** und im Spiel eine Falle.

Die einzige Ausnahme in diesen zehn Rezepten ist `{"cls":"Potato"}`, und sie ist
belegt zulässig: `Potato` ist eine **Vanilla**-Klasse, sie bringt ihre Garstufen und
Übergänge aus den Game-Daten mit, und das Projekt deklariert sie nicht.

**Offen und im Slice-Bericht gemeldet:** es fehlen die Kategorien `POTATO`, `ONION`,
`MILK` und `CHEESE`. Die Kartoffel hängt heute mit Zwiebel und Karotte gemeinsam in
`ROOT_VEGETABLE`, Milch und Käse gemeinsam in `DAIRY`. Ein Delta kann das nicht
heilen: eine Kategorie wird am **Zutatendatensatz** vergeben, und der liegt für diese
vier Klassen in `ChefZ_Ingredients` bzw. `ChefZ_Farming` — fremde Module. Solange das
so ist, gilt:

| gemeint | geschrieben | Folge |
|---|---|---|
| Kartoffel | `{"cls":"Potato"}` | genau die Vanilla-Kartoffel, keine Kategorie |
| Zwiebel | `{"category":"ROOT_VEGETABLE"}` | auch eine Karotte erfüllt den Slot |
| Milch + Käse | `{"allOf":[DAIRY, not BUTTER]}`, `minCount: 2` | auch Sahne erfüllt ihn |

Der Käse hebt die Stufe über eine `gradeRule` — dort ist `cls` erlaubt und
unschädlich, weil `gradeRules` keine Zutatenbindung sind.

## 6. Aktiv geprüfte Verdeckungen (Auftrag: „prüfe, ob dein Rezept ein anderes verdeckt")

Alle 36 Rezepte des Moduls wurden nach der Formel aus 09 §4.1 gerechnet und
paarweise auf gleiche Pflichtslot-Signaturen geprüft. Ergebnis:

* **Keine** identische Pflichtslot-Signatur zwischen einem Rezept dieses Slice und
  irgendeinem anderen Rezept des Moduls.
* `RCP_ChefZ_HunterPlate` (11.00) und `RCP_ChefZ_HunterStew` (11.63) binden dieselben
  Zutaten. Der Eintopf gewinnt — **aber nur mit Wasser im Gefäss**: seine
  Kontextregel fordert `requiresLiquid` und 200 Einheiten Wasser, die des
  Jägertellers nicht. Trockene Pfanne → Teller, Topf mit Wasser → Eintopf. Das ist
  die gewollte Trennung und keine Verdeckung.
* `RCP_ChefZ_CreamMushroomPasta` (9.63) und `RCP_ChefZ_MushroomCreamSauce` (9.50)
  trennt die Pasta: `extraItems: "forbid"` lässt das Saucenrezept nicht zünden,
  sobald Nudeln im Topf liegen, und das Gerichtsrezept verlangt sie.
* Die Löcher innerhalb des Slice sind bewusst geschlossen: `RCP_ChefZ_SausagePasta`
  hat optionale Slots für Sauce und Pilze, `RCP_ChefZ_SausagePotatoes` einen für
  Wurzelgemüse. Ohne sie ergäbe „Nudeln + Wurst + Fett + Pilze" gar kein Gericht,
  weil `forbid` jeden ungebundenen Fremdkörper abweist.
* `priority` steht überall auf `0`. Die Reihenfolge entsteht vollständig aus der
  gerechneten Spezifität (09 E1) — eine Handzahl wäre ein global geteilter
  Namensraum zwischen parallel arbeitenden Slices.

## 7. `RCP_ChefZ_BloodSausagePlate` und die fehlende Blutwurst

Production Map §61.8 nennt als Zutat „Blood Sausage" und sagt im selben Absatz:
*„Blutwurst selbst kann optional V1.1 werden, falls Blutbeschaffung zu aufwendig
ist."* In V1 gibt es weder `ChefZ_BloodSausage` noch eine Blutquelle.

**Stand bis zum Vanilla-Audit.** Der Wurstslot lautete

```json
{ "anyOf": [ { "tag": "CHEFZ_BLOOD_SAUSAGE" }, { "category": "SAUSAGE" } ] }
```

und eine `gradeRule` gab **3 Punkte**, wenn die Wurst den Tag trug. Der Gedanke war,
den Tag als Steckplatz stehenzulassen, bis eine Blutwurst kommt.

**Warum das nicht getragen hat (Vanilla-Audit §4.2 B).** Den Tag trug kein einziges
Item — weder Vanilla noch ChefZ, nachgezählt über alle 92 Zutaten-Records und alle
`CfgChefZIngredients`-Knoten. Damit waren die 3 Punkte nicht „noch nicht erreichbar",
sondern **nie** erreichbar, und die Rechnung des Gerichts ging nicht auf:

| Quelle | Punkte |
|---|---|
| Wurstslot | 1 |
| Salz (optional) | 2 |
| Kräuter (optional) | 1 |
| Gewürz (optional) | 1 |
| `GR_BloodPlateMarjoram` (frische Kräuter) | 1 |
| **erreichbare Summe** | **6** |

`PREMIUM` beginnt bei `minScore = 7.0` (`CfgChefZQualityTiers`, `ChefZ_Cooking/config.cpp`).
Die Blutwurstplatte konnte ihre höchste Stufe also unter keinen Umständen erreichen —
ohne Log, ohne Meldung, der Spieler sieht nur, dass es nie besser wird.

**Entscheidung: Tagzweig und Punktregel entfernt.** Ein Träger nachzurüsten wäre der
schönere Weg gewesen, ist aber in V1 nicht ehrlich machbar: eine Blutwurst braucht
Blut, Vanilla gibt beim Zerlegen kein Blutitem her, und die Wurstkette liegt in
`ChefZ_Meat`. Eine vorhandene Wurst einfach zu taggen wäre eine Behauptung über ein
Item, die nicht stimmt.

Geändert wurde deshalb genau zweierlei, beides in `Dishes_A.json`:

* der Wurstslot ist jetzt schlicht `{ "category": "SAUSAGE" }` — der I2-Anker des
  Gerichts bleibt damit unverändert, `SAUSAGE` hat ausschließlich ChefZ-Mitglieder;
* aus `GR_BloodSausage` wurde `GR_BloodPlateFineSausage` mit demselben Gewicht von
  3 Punkten, aber dem Selektor `{ "tag": "CHEFZ_PREMIUM" }`. Den Tag trägt
  `ChefZ_HunterSausage` (Slice meat), und der ist in V1 herstellbar. Die erreichbare
  Summe steigt damit auf **9** und `PREMIUM` liegt wieder im Bereich.

**Inzwischen erledigt:** der Tag-Record `CHEFZ_BLOOD_SAUSAGE` ist aus
`_deltas/dishes-a.json` und aus `ChefZ_Registry/Config/Tags.json` entfernt, der
Stringtable-Schlüssel `STR_CHEFZ_TAG_BLOOD_SAUSAGE` ebenfalls. Nach der Änderung
oben fragte ihn kein Rezept mehr ab — er war ein Tag ohne Träger **und** ohne
Abfrage. Kommt die Blutwurst
in V1.1, ist sie in derselben Bewegung wieder da: ein Tag im Delta, ein `anyOf` im
Slot, eine `gradeRule`.
