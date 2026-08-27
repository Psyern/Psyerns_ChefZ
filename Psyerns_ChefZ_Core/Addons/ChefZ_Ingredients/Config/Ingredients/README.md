# Zutatenbindungen dieses Moduls — Anmerkungen

In diesem Ordner liegen `Dairy.json`, `Salt.json`, `Spices.json` und
`VanillaProduce.json`. Nur `VanillaProduce.json` trug bis M2 einen Kommentar; er
stand als `_comment` auf der **obersten Dokumentebene**, neben `kind`.

Grund der Verlagerung: `ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()` haelt
fest, dass die Toleranz des Enforce-Serializers gegenueber unbekannten JSON-Feldern
**nicht belegt** ist. `ChefZ_JsonDocs` kennt auf Dokumentebene genau drei Felder —
`kind`, `schemaVersion`, `records`. Ist der Serializer intolerant, wird die Datei
**komplett** verworfen. Der Selbsttest sagt woertlich: "Content-Autoren duerfen
keine Kommentarfelder in JSON schreiben."

## VanillaProduce.json

Slice produce. Entwurf 05 §2, zweiter Deklarationsweg: FREMDE Klassen werden im
Slice-JSON gebunden, nie in ihrer eigenen `config.cpp` (Workflow §10.5). `Potato`
und `Tomato` existieren in Vanilla — ChefZ erweitert sie, statt eine zweite
Kartoffel zu bauen (Production Map §13, §14). `GreenBellPepper` ist die
Vanilla-Paprika und laeuft deshalb in dieselbe Schnittstufe wie `ChefZ_Paprika`
(§15).
