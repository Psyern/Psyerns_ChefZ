# Sausage.json — Anmerkung zu den Bratrezepten

Dieser Text stand bis M2 als `_comment`-Feld **innerhalb des Records**
`RCP_CookSausage`. Grund der Verlagerung: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()`
haelt fest, dass die Toleranz des Enforce-Serializers gegenueber unbekannten
JSON-Feldern **nicht belegt** ist. Ist er intolerant, wird die Datei **komplett**
verworfen — dann liesse sich keine einzige Wurst braten, ohne dass eine Meldung
darauf hinweist. Der Selbsttest sagt woertlich: "Content-Autoren duerfen keine
Kommentarfelder in JSON schreiben."

Die Notiz steht neben der Datei und nicht im Kopf der `config.cpp`, weil Rezepte
laut Workflow-Vorgabe als JSON im Modul liegen und in der `config.cpp` keinen
Gegenpart haben, an den sich der Text haengen liesse.

## Alle Records (`RCP_CookSausage` … `RCP_CookSpicySausage`)

§40: RAW SAUSAGE -> COOKED SAUSAGE. `completion ON_STAGE` heisst: VANILLA bestimmt,
wann gar ist — der Klassentausch haengt an den `FoodStageTransitions` der
`config.cpp`, nicht an einer eigenen Uhr (Pruefstein 20 §2.2, letzter Absatz).
Passt keines dieser Rezepte, kocht Vanilla die Wurst wie jedes andere
Nahrungsmittel weiter; ChefZ blockiert nichts (Workflow §10.2).
`policy.extraItems` ist `ignore` und nicht `forbid`: in einer Pfanne liegt selten
nur eine Wurst, und ein zweites Item darf das Braten nicht verhindern.
