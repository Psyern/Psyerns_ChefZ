# ChefZ Enforce-Script Audit - Bewertung vom 2026-09-03

Auftrag: das gesamte Script in `Psyerns_ChefZ_Core` auf Fehler pruefen, ein Audit
schreiben und es ausfuehren. Regelbasis ist der enforce-script-Skill (Hard Rules)
mit dem Wiki `DAYZ_Enforce-Script-main` (Safe-AI-CodingPrompt, Tips-Common-Pitfalls,
DME_129_Audit_Prompt) und der Vanilla-Quelle `scripts - 1.29`.

Es wurde NICHTS am Mod veraendert. Das Audit ist rein lesend (Phase 1+2 des
Audit-Protokolls). Neu im Repo sind nur die Dateien in `Audit/`.

## Werkzeug

| Datei | Zweck |
|---|---|
| `Audit/chefz_audit.py` | Das Audit. Python 3, keine Abhaengigkeiten. `python chefz_audit.py [--json findings.json]` |
| `Audit/AUDIT_REPORT.md` | Automatisch erzeugter Rohreport des letzten Laufs (alle Fundstellen) |
| `Audit/findings.json` | Dieselben Fundstellen maschinenlesbar |
| diese Datei | Bewertung und Handlungsempfehlung |

Exit-Code 1 bei P0/P1, sonst 0 - damit kann der Lauf vor jedem PBO-Pack als Gate laufen.

Was das Script prueft (je .c-Datei, Kommentare und Strings vorher maskiert):

- Verbotene Syntax: Ternary, `int a, b;`, `delete`, `auto`/`var`, `?.`/`??`, Lambdas
- `ref` in Parametern, Rueckgabetypen, lokalen Variablen (Template-Argumente wie `map<int, ref array<int>>` sind erlaubt und werden nicht gemeldet)
- Redeklaration eines Namens im verschachtelten Scope (Parameter eingeschlossen; Geschwister-Scopes sind erlaubt)
- `modded class X : Y`, `override` ohne `super`-Aufruf in modded classes, Member ohne ChefZ-Prefix in modded classes
- `override` auf einer Methode, die in der gesamten Basiskette (ChefZ + Vanilla 1.29, 6069 Klassen) nicht existiert; umgekehrt Basismethode ohne `override`
- Layer-Verstoesse: Referenz auf eine Klasse eines hoeheren Layers, nur in Typ-Position (Deklaration, `new`, statischer Zugriff, Vererbung, Template-Argument)
- `GetGame()`, `IsClient()`/`IsServer()`, `GetObjectsAtPosition*`
- Leere `#ifdef`-Bloecke, komplexe Ausdruecke direkt in Array-Index-Zuweisungen
- Klammerbalance, RPC-Kennungen < 10000, Tabs vs. Leerzeichen, mehrzeilige Aufrufe
- config.cpp: CfgPatches/CfgMods vorhanden, `files[]`/`inputs`/`dataFiles[]` gegen `$PREFIX$` und Dateisystem, `dependencies[]` passend zu den ScriptModules, `requiredAddons` auf ChefZ_* existieren, `units[]` definiert, Basisklassen aufloesbar, Modell-/Texturpfade vorhanden
- stringtable.csv: 15-Spalten-Header, Spaltenzahl je Zeile, Duplikate, alle in Skripten/config.cpp/Inputs.xml benutzten `STR_`-Keys vorhanden
- JSON-Validitaet (58 Dateien), XML-Validitaet, `GetInputByName` gegen Inputs.xml, Actions gegen `RegisterActions`/`AddAction`

Das Script wurde vor dem Einsatz an einer synthetischen Datei mit 17 bekannten
Verstoessen verifiziert; alle wurden erkannt. Drei Fehlalarm-Quellen des ersten
Laufs (Methodennamen als Layer-Verstoss, `ref` in Template-Argumenten,
Kommentartext in `units[]`) sind behoben.

## Umfang

| Kennzahl | Wert |
|---|---|
| Addons | 14 |
| Skriptdateien | 174 |
| Skriptzeilen | 77.231 |
| ChefZ-Klassen | 324 |
| JSON-Configs | 58 |
| Stringtable-Keys | 367 |

## Ergebnis

| Prio | Bedeutung | Anzahl | Bewertung |
|---|---|---|---|
| P0 | Compile-Fehler / Modul laedt nicht | 1 | echt, siehe B1 |
| P1 | Segfault / Crash | 0 | - |
| P2 | 1.29 Breaking | 43 | bewusstes Muster, siehe B3 |
| P3 | Silent Failure | 4 | 2 echt (B1), 2 Hinweis (B4) |
| P4 | Style | 169 | Einrueckung, siehe B5 |

Nicht gefunden, und das ist das eigentliche Ergebnis fuer 77k Zeilen:
kein Ternary, keine Multi-Deklaration, kein `delete`, kein `auto`, kein `GetGame()`
im Code (nur in Kommentaren), kein `ref` an verbotener Stelle, keine
Redeklaration im Nested-Scope, keine `modded class` mit Vererbung, jeder
`override` in einer modded class ruft `super`, kein `override` ohne Basismethode,
kein Layer-Verstoss, keine unausgeglichene Klammer, kein leerer `#ifdef`, keine
komplexe Array-Zuweisung, alle RPC-Kennungen ab 10000 (10000-10002, zentral in
`ChefZ_CookbookRPC`), alle drei Actions registriert, alle Inputs deklariert, alle
367 `STR_`-Keys vorhanden, alle Stringtables im 15-Spalten-Layout, alle 58 JSON-
und 3 XML-Dateien gueltig, alle `files[]`-Pfade stimmen mit `$PREFIX$` und
Dateisystem ueberein.

## Befunde

### B1 (P0) ChefZ_Farming verlangt ein Addon, das es nicht gibt

`Addons/ChefZ_Farming/config.cpp:106` fuehrt `"ChefZ_Plants_Cultivation"` in
`requiredAddons[]`. Keine config.cpp im Repo deklariert eine CfgPatches-Klasse
dieses Namens. `ChefZ_Plants` hat genau eine CfgPatches-Klasse (`ChefZ_Plants`),
und `ChefZ_Plants/cultivation/` enthaelt nur `data/` (fuenf Stufentexturen,
ein .rvmat) - keine config.cpp, kein `$PREFIX$`, kein `models/`.

Derselbe Sachverhalt steckt hinter den beiden P3-Fundstellen
`model-path-missing`: `ChefZ_CornPlant` (Zeile 1619) und `ChefZ_WildCorn`
(Zeile 1731) zeigen auf `\ChefZ\ChefZ_Plants\cultivation\models\corn_plant.p3d`.
Diese Datei existiert nicht. Das einzige Maismodell liegt unter
`ChefZ_Plants/models/corn_plant.p3d` (Laufzeitpfad
`\ChefZ\ChefZ_Plants\models\corn_plant.p3d`), mit `models/model.cfg`
(PlantBaseSkeleton, `plantstage_01..06`).

Wirkung: DayZ meldet beim Start "Addon 'ChefZ_Farming' requires addon
'ChefZ_Plants_Cultivation'" und laedt weiter, aber die Ladereihenfolge
gegenueber ChefZ_Plants ist dann nicht mehr garantiert. Die beiden Maispflanzen
haben kein Modell (unsichtbar bzw. Fehlerbox im Spiel, RPT: "Cannot load
object").

Die Kommentare in der Farming-config.cpp (Zeilen 1622-1630, 1694-1700,
1810-1825) beschreiben eine "Lieferung a78a247" mit `cultivation/config.cpp`
und `cultivation/models/model.cfg` (fuenf Stufen). Im Repo liegt stattdessen
`models/model.cfg` mit sechs Stufen. Die Lieferung ist also nur zur Haelfte
eingespielt, oder die Ablage wurde nachtraeglich verschoben.

Zwei moegliche Korrekturen, die Entscheidung ist deine:

1. Das Modell ist dort richtig, wo es liegt: in `ChefZ_Farming/config.cpp`
   beide `model`-Pfade auf `\ChefZ\ChefZ_Plants\models\corn_plant.p3d` setzen
   und `"ChefZ_Plants_Cultivation"` aus `requiredAddons[]` streichen.
   `GrowthStagesCount = 6` passt dann zum Sechs-Stufen-Mesh (Kommentar in
   Zeile 1623-1629 begruendet 6 mit einem Fuenf-Stufen-model.cfg; mit dem
   vorliegenden model.cfg waere nach derselben Rechnung 7 der Hoechstwert -
   pruefen).
2. Die Lieferung a78a247 vollstaendig einspielen: `ChefZ_Plants/cultivation/`
   als eigenes Addon mit `$PREFIX$` = `ChefZ\ChefZ_Plants\cultivation`,
   eigener config.cpp (CfgPatches `ChefZ_Plants_Cultivation`), `models/corn_plant.p3d`
   und `models/model.cfg`.

### B2 (P0, Hinweis) `g_Game` ohne Null-Check

`Addons/ChefZ_Core/Scripts/4_World/ChefZ/Processing/ChefZ_ActionProcessAtStation.c:508`
prueft `if (!g_Game.IsDedicatedServer())` ohne vorherigen `if (!g_Game)`. Alle
anderen 50 Stellen im Repo pruefen `g_Game` zuerst. In einer Action ist
`g_Game` praktisch nie null, aber die Hard Rule verlangt den Check. Einzeiler.

### B3 (P2) `g_Game.IsServer()` statt `IsDedicatedServer()` - 43 Stellen

Alle 43 Fundstellen haben dieselbe Form `if (!g_Game || !g_Game.IsServer()) return;`
als Autoritaets-Gate, plus `IsServerSide()` in `ChefZ_EventBus.c:1007`.
Daneben stehen 8 Stellen mit `IsDedicatedServer()` (Cookbook, Apiary, Smoker,
ActionProcessAtStation).

Das Wiki stuft `IsServer()` als unzuverlaessig ein und verlangt
`IsDedicatedServer()`. Zur Einordnung: Vanilla 1.29 selbst benutzt
`IsServer()` 623-mal und `IsDedicatedServer()` 238-mal; fuer Autoritaets-Gates
ist `IsServer()` das Vanilla-Muster. Auf einem dedizierten Server liefern beide
dasselbe. Der Unterschied liegt im Offline-Modus (Client hostet die Mission):
dort ist `IsServer()` wahr und `IsDedicatedServer()` falsch - eine Umstellung
wuerde die gesamte Serverlogik (Kochen, Verarbeiten, Portionen, Zerfall,
Bienenstock) im Offline-Test abschalten.

Empfehlung: nicht blind umstellen. Entweder bewusst bei `IsServer()` bleiben
und das im Entscheidungs-Dokument festhalten, oder einen zentralen Helfer
(`ChefZ_Side.IsAuthority()`) einfuehren, der an einer Stelle entscheidet.
Die 8 abweichenden `IsDedicatedServer()`-Stellen sollten in jedem Fall dem
gewaehlten Muster folgen.

### B4 (P3, Hinweis) `GetObjectsAtPosition*` - 2 Stellen

- `ChefZ_ContainerService.c:654`: `GetObjectsAtPosition3D` beim Servieren
  (Behaeltersuche, Radius aus Core.json). Wird pro Aktion aufgerufen, nicht pro
  Tick. Unkritisch.
- `ChefZ_FryingPan.c:72`: `GetObjectsAtPosition` mit Radius 2,5 m in
  `ChefZ_HasHeat()`. Wird aus `ChefZ_BuildContext` gerufen, und das laeuft im
  Job-Tick der Station (`TICK_INTERVAL_SEC = 2.0`), solange ein Job aktiv ist.
  Alle 2 s pro aktiver Pfanne ist tragbar; bei vielen gleichzeitig laufenden
  Pfannen waere ein `UniversalTemperatureSource`-/Trigger-Ansatz wie beim
  Vanilla-Kochen sauberer. Kein Handlungsbedarf, nur beobachten.

### B5 (P4) Einrueckung mit Leerzeichen - 169 von 174 Dateien

Das gesamte Repo rueckt mit vier Leerzeichen ein, die Hard Rule verlangt
Tabs. Der Compiler ist das egal. Eine Umstellung ist ein 58.000-Zeilen-Diff
ohne Verhaltensaenderung; sie lohnt nur, wenn Mixed-Team-Arbeit mit dem
Vanilla-Stil ansteht. Auf Wunsch mechanisch machbar (`4 Spaces -> Tab` am
Zeilenanfang, Zeichenketten bleiben unberuehrt).

## Was das Audit nicht leisten kann

- Null-Checks nach `Cast()`, `GetParent()`, `GetIdentity()` sind nicht
  statisch pruefbar, ohne Datenfluss zu verfolgen. Stichproben in
  ContainerService, CookActor und ProcessRunner zeigten durchgehend Checks.
- Ob eine Vanilla-Methode existiert, prueft das Script nur fuer `override`
  (gegen die Basiskette). Aufrufe fremder Methoden (`item.Foo()`) werden nicht
  gegen die Vanilla-Signaturen geprueft.
- Der echte Compile-Test bleibt der DayZ-Server-Start mit `-mod=` und Blick
  ins RPT. Das Audit ersetzt ihn nicht, es filtert vorher.

## Naechste Schritte

1. B1 entscheiden (Option 1 oder 2) - das ist die einzige Aenderung, die vor
   dem naechsten Pack noetig ist.
2. B2 den Null-Check nachziehen.
3. B3 als Entscheidung festhalten.
4. `python Audit/chefz_audit.py` vor jedem Pack laufen lassen; Exit-Code 1
   blockiert.
