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

## Produce.json — entfallen

Die acht Schnitt-Transforms und `PROCESS_CHOP_VEGETABLE` sind am 29.08.2026 mit
dem Schnittgut entfernt worden. Rezepte nehmen das ganze Gemuese.


## VanillaFoodProcessing.json

Slice `vanilla-foods`. Zwei Transforms, ein Prozess (`PROCESS_DRY`), keine neue
Station und kein neuer Prozess — er wird nur **benutzt**.

### `TR_CaninaBerriesToDried` und `TR_SambucusBerriesToDried` — `PROCESS_DRY`

Audit §3 F: die zwei Waldbeeren hängen an der **vorhandenen** Trockenkette. Station
ist `ChefZ_DryingRack`, Prozess ist `PROCESS_DRY` — beides ist in `ChefZ_Processing`
deklariert und dort im Delta des Slice `herbs` angemeldet; dieser Slice deklariert
es nicht ein zweites Mal. `PROCESS_DRY` läuft als `STATION_TIMED` und zählt deshalb
**nicht** gegen `handcraftRecipeSlots`: Vanillas Rezeptliste bleibt um kein Bit
verändert.

Zwei Beeren ergeben eine Portion `ChefZ_DriedBerries` mit `setState: "DRIED"`.
Warum diese Klasse einen Übergang `Dried -> Boiled` trägt, den keine andere
Trockenware des Mods hat, steht im Kommentar über der Klasse in der `config.cpp`:
sie ist Pflichtzutat des Obstkompotts und muss im Topf aufquellen dürfen, statt zu
verkohlen.
