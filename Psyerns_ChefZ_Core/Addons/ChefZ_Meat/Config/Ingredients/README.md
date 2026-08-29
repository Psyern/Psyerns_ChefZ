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

### 2b. Keulen (`ChefZ_BeefLeg`, `ChefZ_PorkLeg`, `ChefZ_VenisonLeg`)

Grobteilstuecke mit Knochen, die beim Zerlegen anfallen. Die Anbindung an die
Zerlegung ist eine reine Datenzeile in Vanillas `Skinning`-Tabelle; sie steht im
Kopf der `config.cpp` samt Belegstellen aus `ActionSkinning.c`.

**Sie tragen `categories: []` — keine einzige Kategorie. Das ist die zentrale
Entscheidung an diesen drei Records und keine Nachlaessigkeit.**

Kategorien sind in ChefZ *self-or-ancestor*: `ChefZ_CategoryClosure` setzt fuer
jede Kategorie eines Items auch alle Vorfahren, und ein Slot fuer `MEAT` trifft
damit jedes Item, das `DOMESTIC_MEAT` oder `WILD_MEAT` traegt. Eine Keule mit
Fleischkategorie waere fuer die vorhandenen Slots **ein Fleischstueck**:

* `TR_MeatToMinced` (dieselbe Bedingung) machte daraus **ein** Hackfleisch.
* `TR_RawHunterSausage` (`category: WILD_MEAT`, `minCount 2`) machte aus zwei
  Wildkeulen ohne jedes Wolfen direkt eine Jaegerwurst.

Jeder dieser drei Wege waere eine stille Wertvernichtung bzw. eine Abkuerzung,
die niemand beabsichtigt hat und die kein Validator meldet. Die Keule wird
deshalb ausschliesslich ueber ihren **Klassennamen** adressiert, naemlich von
`TR_CutBeefLeg` / `TR_CutPorkLeg` / `TR_CutVenisonLeg`. Ihre Identitaet fuer
kuenftige Regeln traegt sie ueber **Tags** (`CHEFZ_RAW_MEAT`,
`CHEFZ_HIGH_PROTEIN`, bei Wild zusaetzlich `CHEFZ_WILD_MEAT`) — Tags haben keine
Vorfahrenkette und ziehen deshalb keine Slots an, die es nicht gibt.

**Die schoenere Form waere eine eigene Kategorie** — etwa `PRIMAL_CUT` mit
`parent: null`, ausdruecklich **nicht** unter `MEAT`, sonst faengt das Problem von
vorne an. Sie ist ein Eintrag in der zentralen `Categories.json`, den nur der
`chefz-registry-integrator` schreiben darf. Der Vorschlag steht im Slice-Bericht;
solange er nicht gemergt ist, waere ein Delta-Eintrag dafuer ein harter
Validatorfehler (`deltas.mjs`, "der Merge ist unvollstaendig") und kein Fortschritt.

### 3. Eigene Zwischenprodukte (`ChefZ_MincedMeat` … `ChefZ_MincedBear`)

§29/§30: die eigenen Zwischenprodukte.

### 4. Rohe Wuerste (`ChefZ_RawSausage` … `ChefZ_RawSpicySausage`)

§34-§39: die rohen Wuerste. SAUSAGE ist Unterkategorie von MEAT — ein Gericht,
das MEAT verlangt, nimmt damit auch Wurst (07 §3, Kategorie trifft alle
Unterkategorien).

### 5. Gebratene Wuerste (`ChefZ_CookedSausage` … `ChefZ_SpicySausage`)

§40: die gebratenen Wuerste. Sie tragen CHEFZ_HOT_MEAL, weil sie warm entstehen,
und keinen RAW-Tag mehr.
