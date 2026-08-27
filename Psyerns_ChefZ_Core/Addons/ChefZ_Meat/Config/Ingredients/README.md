# Meat.json — Anmerkungen zu den Zutatensaetzen

Diese Texte standen bis M2 als `_comment`-Felder **innerhalb der Records** in
`Meat.json`. Sie sind hierher gewandert, weil `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()`
ausdruecklich festhaelt, dass die Toleranz des Enforce-Serializers gegenueber
unbekannten JSON-Feldern **nicht belegt** ist. Ist er intolerant, wird die Datei
nicht etwa teilweise, sondern **komplett** verworfen — und der halbe Fleisch-Content
fehlte, ohne dass irgendetwas danach aussieht. Der Selbsttest sagt dazu woertlich:
"Content-Autoren duerfen keine Kommentarfelder in JSON schreiben."

Die Notizen stehen neben der Datei und nicht im Kopf der `config.cpp`, weil sie
einzelne Datensatzgruppen dieser Datei erklaeren. Die `config.cpp` beschreibt
CfgVehicles-Klassen und Prozesse; eine Zutatenbindung ist ein anderer Gegenstand.

## Gruppen in der Reihenfolge der Datei

### 1. Vanilla-Fleisch (`PigSteakMeat` … `BearSteakMeat`)

Production Map §28: ChefZ teilt VANILLA-Fleisch in Kategorien ein. Das ist der
Uebergabepunkt aus §27 — was aus der Zerlegung faellt, ist ab hier eine
ChefZ-Zutat. Kein Vanilla-Eintrag wird dafuer veraendert; die Zuordnung lebt
ausschliesslich hier.

### 2. Nebenprodukte der Zerlegung (`Lard`, `Bone`, `Guts`)

§27/§31/§32/§33: Fett, Knochen und Daerme kommen aus der Zerlegung und sind
Vanilla-Klassen. ChefZ legt fuer sie KEINE eigene Klasse an — es waere eine
zweite Sorte desselben Dings.

### 3. Eigene Zwischenprodukte (`ChefZ_DicedMeat` … `ChefZ_SausageCasing`)

§29/§30: die eigenen Zwischenprodukte.

### 4. Rohe Wuerste (`ChefZ_RawSausage` … `ChefZ_RawSpicySausage`)

§34-§39: die rohen Wuerste. SAUSAGE ist Unterkategorie von MEAT — ein Gericht,
das MEAT verlangt, nimmt damit auch Wurst (07 §3, Kategorie trifft alle
Unterkategorien).

### 5. Gebratene Wuerste (`ChefZ_CookedSausage` … `ChefZ_SpicySausage`)

§40: die gebratenen Wuerste. Sie tragen CHEFZ_HOT_MEAL, weil sie warm entstehen,
und keinen RAW-Tag mehr.
