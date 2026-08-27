# ProduceSeeds.json — Anmerkung

Der folgende Text stand bis M2 als `_comment` auf der **obersten Dokumentebene**
von `ProduceSeeds.json`, neben `kind`.

Grund der Verlagerung: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()` haelt
fest, dass die Toleranz des Enforce-Serializers gegenueber unbekannten JSON-Feldern
**nicht belegt** ist. `ChefZ_JsonDocs` kennt auf Dokumentebene genau drei Felder —
`kind`, `schemaVersion`, `records`. Ist der Serializer intolerant, wird die Datei
**komplett** verworfen und die Samengewinnung fiele still aus. Der Selbsttest sagt
woertlich: "Content-Autoren duerfen keine Kommentarfelder in JSON schreiben."

Die Notiz steht neben der Datei und nicht im Kopf der `config.cpp`, weil Transforms
laut 11 §5 ausdruecklich nicht in die `config.cpp` gehoeren.

## ProduceSeeds.json

Slice produce. Samengewinnung nach dem Vorbild von Vanillas `CutOutSeeds`:
Gemuese + Messer -> Samen. Vier Transforms ueber `PROCESS_CUT_OUT_SEEDS`
(`exec HANDCRAFT`, Werkzeuggruppe `CUTTING_TOOL`). Kartoffel und Tomate fehlen hier
ABSICHTLICH: Vanilla bringt `CutOutTomatoSeeds` und `PotatoSeed` bereits mit, ein
zweiter Weg zum selben Samen waere Doppelung (Workflow §10.5).
