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

## Mengenskala — warum hier `fromInput` stehen bleibt

Stand 31.08.2026. Die essbaren Klassen dieses Moduls fuehren
`varQuantityMax = 250` (Kopf der `config.cpp`), und `quantity` ist in den
Ausgaengen eine ROHE Menge (`ChefZ_Applicator.c:975-976`). In
`../Processing/Meat.json` musste `fromInput` deshalb weichen: dort ist der
Eingang Vanilla-Fleisch mit fremder Skala.

Hier nicht. Eingang (`ChefZ_Raw*Sausage`) und Ausgang (`ChefZ_*Sausage`) sind
beide ChefZ-Fleischklassen auf derselben 250er-Skala. `fromInput` ist damit
genau das Richtige und bleibt in allen sechs Records: wer eine halb
aufgegessene Rohwurst in die Pfanne legt, bekommt eine halb volle gebratene
Wurst zurueck. Mit `fixed` bekaeme er eine volle — ein Rezept, das aus einem
Rest ein ganzes Stueck macht.

Der mitgefuehrte Wert `"quantity": 250` ist dabei tot: `ChefZ_Applicator.c:971`
nimmt bei `fromInput` die verbrauchte Menge und sieht `def.quantity` nie an. Er
steht trotzdem auf 250 und nicht mehr auf 1, damit ein spaeterer Wechsel auf
`fixed` nicht stillschweigend eine Wurst mit 0,4 % Fuellung erzeugt.
