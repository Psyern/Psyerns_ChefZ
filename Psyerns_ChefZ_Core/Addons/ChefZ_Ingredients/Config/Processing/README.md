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


## VanillaFoodProcessing.json

Slice `vanilla-foods`. Drei Transforms, zwei Prozesse, keine neue Station und kein
neuer Prozess — beide werden nur **benutzt**.

### `TR_ChopZucchini` — `PROCESS_CHOP_VEGETABLE`

Wortgleich zu `TR_ChopBellPepper` gebaut: ein Eingang plus Werkzeuggruppe
`CUTTING_TOOL`, das Messer belegt Vanillas zweiten Zutatenplatz (01 V12).
Ergebnis ist `ChefZ_ChoppedZucchini`, das achte Schnittgut der Familie.

Dieser eine Transform ist der einzige Grund, warum der `CfgChefZ`-Knoten
`ChefZ_VanillaFoods` **`handcraftRecipeSlots = 1`** nennt. Die Reservierung muss vor
dem ersten Laden feststehen — Vanilla vergibt Rezept-IDs als Position in seiner
Liste, und diese Positionen entstehen im Missionskonstruktor (Kopf von
`ChefZ_HandcraftBridge.c`). Wer hier einen HANDCRAFT-Transform ergänzt, erhöht die
Zahl in derselben Änderung.

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
