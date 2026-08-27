# Produce.json — Anmerkung

Der folgende Text stand bis M2 als `_comment` auf der **obersten Dokumentebene**
von `Produce.json`, neben `kind`.

Grund der Verlagerung: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()` haelt
fest, dass die Toleranz des Enforce-Serializers gegenueber unbekannten JSON-Feldern
**nicht belegt** ist. `ChefZ_JsonDocs` kennt auf Dokumentebene genau drei Felder —
`kind`, `schemaVersion`, `records`. Ist der Serializer intolerant, wird die Datei
**komplett** verworfen und saemtliche acht Schnitt-Transforms fielen still aus. Der
Selbsttest sagt woertlich: "Content-Autoren duerfen keine Kommentarfelder in JSON
schreiben."

Die Notiz steht neben der Datei und nicht im Kopf der `config.cpp`, weil Transforms
laut 11 §5 ausdruecklich nicht in die `config.cpp` gehoeren.

## Produce.json

Slice produce. Acht Schnitt-Transforms, alle ueber `PROCESS_CHOP_VEGETABLE`
(`exec HANDCRAFT`, Werkzeuggruppe `CUTTING_TOOL`). Ein Eingang plus Werkzeug ist
genau die Form, die Vanillas RecipeBase traegt — `MAX_NUMBER_OF_INGREDIENTS = 2`,
das Messer belegt den zweiten Platz (01 V12). Transforms stehen ausdruecklich NICHT
in der `config.cpp` (11 §5, `ChefZ_ConfigCppSource` Kopf): sie tragen Selektoren und
sind eine serverseitige Entscheidung.
