# Psyerns_ChefZ – Multi-Agent-Workflow für die Umsetzung

**Status:** Design v1 · **Stand:** 2026-08-26
**Grundform:** Ansatz C – Hybrid mit Registry-Ownership

Dieses Dokument beschreibt, **wie** Psyerns_ChefZ gebaut wird: welche Agenten
existieren, wem welche Datei gehört, wie sie zusammenarbeiten, was automatisch
geprüft wird und wo der Mensch entscheidet.

Es beschreibt **nicht**, *was* gebaut wird — das steht in:

- `ChefZ_Core_Architekturplan.md` — Systeme des Core
- `ChefZ_DME_Mod_Plan.md` — Content-Vision
- `ChefZ_V1_Ingredient_Production_Map.md` — V1-Umfang, Produktionsketten, Phasen
- `ChefZ_Terje_Compatibility_Analyse.md` — Terje-Anbindung
- `ChefZ_Weitere_Planungsschritte.md` — offene Designfragen

---

# 1. Getroffene Grundentscheidungen

| # | Entscheidung | Wahl |
|---|---|---|
| 1 | Artefaktform | Vollständiges Harness: Spec + Workflow-Skripte + Agent-Definitionen + Slash-Commands |
| 2 | Verifikationstiefe | Statische Validatoren + adversariale Reviews. Build/Deploy/In-Game bleibt manuell. |
| 3 | Modulstruktur | Ein Mod `Psyerns_ChefZ_Core`, darin mehrere PBOs pro Modul |
| 4 | Freigabe-Gates | 4 Gates, je Meilenstein |
| 5 | 3D-Assets | Vanilla-Proxy-Modelle als Platzhalter; Content wartet nie auf Assets |
| 6 | Config-Auslieferung | JSON-Defaults in den PBOs, Kopie nach `$profile:ChefZ/` beim ersten Serverstart |

---

# 2. Warum diese Bauform

Drei Bauformen standen zur Wahl:

**Horizontale Schichten** (erst alle Systeme, dann alle Items, dann alle Rezepte)
sind konfliktarm, aber verschieben jeden Integrationsfehler ans Ende. Bei nur
vier Gates heißt das: ein falsches Kategoriemodell fällt erst auf, wenn es in
neun Modulen steckt.

**Vertikale Produktionsketten** (ein Agent baut Getreide komplett durch,
ein anderer Salz) parallelisieren gut und halten Fehler in ihrer Kette. Aber
alle Ketten greifen auf dieselben zentralen Registries zu — `Categories.json`,
Food-Tags, `Processing.json`, `Nutrition.json`. Sechs Agenten, die gleichzeitig
dieselbe Datei schreiben, verlieren zuverlässig Edits.

**Ansatz C** nimmt aus beidem das Brauchbare und löst das Registry-Problem
durch eine einzige Regel:

> **Registry-Ownership:** Content-Agenten schreiben niemals in zentrale
> Registries. Sie schreiben nur in ihren eigenen Modulordner und geben
> zusätzlich ein **Registry-Delta** ab. Ein einzelner Integrator merged alle
> Deltas und schreibt die zentralen Dateien.

Damit hat jede Datei im Projekt genau einen schreibenden Agenten. Keine
Worktrees, keine Locks, keine verlorenen Edits — und die Parallelität der
vertikalen Slices bleibt erhalten.

Die Risikoverteilung bestimmt zusätzlich die Ausführungsform pro Meilenstein:

| Meilenstein | Form | Begründung |
|---|---|---|
| M1 Core | Design-Panel, dann sequenziell | Der Architekturplan sagt selbst: *„ChefZ_Core soll möglichst selten geändert werden müssen."* Ein Fehler hier vergiftet alles Spätere. |
| M2 Basisproduktion | Vertikale Slices, parallel | Ketten sind weitgehend disjunkt |
| M3 Content | Vertikale Slices, parallel | Gerichte sind voneinander unabhängig |
| M4 Compatibility | Voll parallel | Drei komplett getrennte Mod-Ordner |

---

# 3. Ordner-Topologie

```text
C:\Users\Administrator\Desktop\Psyerns_ChefZ\
│
├── Psyerns_ChefZ_Docs\                     Planung + Gate-Reports + Backlogs
│
├── Psyerns_ChefZ_Core\                     DER Mod (ein Workshop-Item)
│   ├── Addons\
│   │   ├── ChefZ_Core\                     Systeme – kein Content
│   │   ├── ChefZ_Ingredients\              Grundzutaten, Zwischenprodukte, Gewürze
│   │   ├── ChefZ_Farming\                  Pflanzen, Kräuter, Samen, Ernte
│   │   ├── ChefZ_Processing\               Stationen, Werkzeuge, Verarbeitung
│   │   ├── ChefZ_Meat\                     Hackfleisch, Wurst, Fleischprodukte
│   │   ├── ChefZ_Preservation\             Salzen, Trocknen, Räuchern
│   │   ├── ChefZ_Baking\                   Teig, Brot, Pasta
│   │   ├── ChefZ_Cooking\                  Teller, Suppen, Eintöpfe, Frühstück
│   │   └── ChefZ_UI\                       Cookbook (ab V1.1)
│   ├── Keys\
│   └── _deltas\                            Registry-Deltas der Content-Agenten
│
├── Psyerns_ChefZ_Terje_Skills_Comp\        eigener Mod
├── Psyerns_ChefZ_Terje_Medicine_Comp\      eigener Mod
├── Psyerns_ChefZ_COT_Comp\                 eigener Mod
│
├── tools\chefz-validate\                   statische Validatoren (Node, ohne Abhängigkeiten)
│
└── .claude\
    ├── agents\                             Agent-Definitionen
    ├── workflows\                          Meilenstein-Skripte
    └── commands\                           Slash-Commands
```

Aufbau eines Addon-Ordners:

```text
ChefZ_Meat\
├── $PREFIX$                    ChefZ_Meat
├── config.cpp                  CfgPatches, CfgVehicles, CfgSlots …
├── Config\                     JSON-Defaults dieses Moduls
│   ├── Recipes\Sausage.json
│   └── Processing\Grinding.json
├── Scripts\
│   ├── 3_Game\ChefZ\Meat\
│   ├── 4_World\ChefZ\Meat\
│   └── 5_Mission\ChefZ\Meat\
├── Data\                       Texturen, Materialien
├── Models\                     .p3d (zunächst leer – Vanilla-Proxys)
└── Language\stringtable.csv
```

---

# 4. Ownership-Matrix

Die wichtigste Tabelle des Dokuments. Jede Zeile hat **genau einen** schreibenden Agenten.

| Pfad | Schreibrecht | Leserecht |
|---|---|---|
| `Psyerns_ChefZ_Core/Addons/ChefZ_Core/**` | `chefz-core-engineer` | alle |
| `Psyerns_ChefZ_Core/Addons/ChefZ_<Modul>/**` | der `chefz-content-engineer`, dem der Slice zugewiesen ist | alle |
| `Psyerns_ChefZ_Core/_deltas/<slice>.json` | derselbe Content-Agent, nur die eigene Datei | Integrator |
| `.../ChefZ_Core/Config/Categories.json` | **nur** `chefz-registry-integrator` | alle |
| `.../ChefZ_Core/Config/Tags.json`, `Processing.json`, `Nutrition.json`, `Preservation.json` | **nur** `chefz-registry-integrator` | alle |
| `Psyerns_ChefZ_Terje_Skills_Comp/**` | `chefz-terje-comp-engineer` | alle |
| `Psyerns_ChefZ_Terje_Medicine_Comp/**` | `chefz-terje-comp-engineer` | alle |
| `Psyerns_ChefZ_COT_Comp/**` | `chefz-cot-comp-engineer` | alle |
| `**/Language/stringtable.csv` | der Eigentümer des jeweiligen Addons | alle |
| `Psyerns_ChefZ_Docs/ChefZ_Asset_Backlog.md` | `chefz-asset-tracker` | alle |
| `Psyerns_ChefZ_Docs/GATE_*.md`, `CHANGELOG.md` | `chefz-doc-scribe` | alle |
| `tools/**`, `.claude/**` | Hauptsession (Mensch/Claude) | alle |

**Nur-lesende Agenten:** `chefz-architect` (schreibt ausschließlich Designnotizen
nach `Psyerns_ChefZ_Docs/design/`), `chefz-conflict-scout`, `chefz-balance-reviewer`,
`chefz-validator`.

## Die eine Ausnahme

Der Slice `serving` in Meilenstein 3 (Portionen, Behälterkategorien) darf
zusätzlich generische Systemteile in `Addons/ChefZ_Core/Scripts/` schreiben — und
nur dann, wenn die Core-API sie nicht schon bereitstellt. Konkrete Teller- oder
Schüssel-Items gehören auch für ihn nach `ChefZ_Cooking`, und die zentralen
Registry-JSONs bleiben auch für ihn tabu.

Diese Ausnahme existiert, weil Portionen und Behälter echte Core-Systeme sind, die
erst in Meilenstein 3 gebraucht werden — sie in Meilenstein 1 blind vorzubauen wäre
schlechter. Der `chefz-conflict-scout` prüft in Gate 3 gezielt nach, ob dabei
Content in den Core gerutscht ist.

---

# 5. Das Registry-Delta-Protokoll

Ein Content-Agent, der z. B. die Wurstkette baut, braucht neue Kategorien
(`SAUSAGE`, `FAT`, `CASING`), neue Tags und neue Prozesse (`PROCESS_GRIND`,
`PROCESS_SMOKE`). Er darf die zentralen Dateien nicht anfassen. Stattdessen
schreibt er `_deltas/meat.json`:

```json
{
  "slice": "meat",
  "categories": [
    { "id": "SAUSAGE", "parent": "MEAT", "displayName": "#STR_CHEFZ_CAT_SAUSAGE" },
    { "id": "FAT",     "parent": null,   "displayName": "#STR_CHEFZ_CAT_FAT" }
  ],
  "tags": [
    { "id": "CHEFZ_RAW_SAUSAGE", "appliesTo": ["ChefZ_RawSausage"] }
  ],
  "processes": [
    { "id": "PROCESS_GRIND", "station": "ChefZ_MeatGrinder", "durationSec": 20 }
  ],
  "nutrition": [
    { "class": "ChefZ_PorkSausage", "energy": 450, "water": 30, "stomach": 120 }
  ],
  "preservation": [
    { "state": "SMOKED", "spoilageMultiplier": 0.25 }
  ],
  "classes": ["ChefZ_MincedMeat", "ChefZ_SausageCasing", "ChefZ_RawSausage", "ChefZ_PorkSausage"]
}
```

Der `chefz-registry-integrator` liest **alle** Deltas zusammen und:

1. prüft auf **ID-Kollisionen** zwischen Slices (zwei Slices definieren `FAT` unterschiedlich)
2. prüft, dass jede `parent`-Kategorie nach dem Merge existiert
3. prüft, dass jeder referenzierte `station`-Eintrag existiert
4. merged deterministisch (alphabetisch nach Slice, dann nach ID) — gleiche Eingabe, gleiches Ergebnis
5. schreibt die zentralen Registries
6. meldet ungelöste Konflikte zurück, statt sie stillschweigend zu überschreiben

Der Merge ist der **einzige Punkt im Workflow, an dem eine echte Barriere nötig
ist**: die Kollisionsprüfung braucht alle Deltas gleichzeitig. Überall sonst
läuft eine Pipeline ohne Sperre.

---

# 6. Der Agenten-Kader

## 6.1 Bauende Agenten

### `chefz-architect`
Entwirft Core-Systeme: Recipe Engine, Matching-Strategien, State-Machine,
Category-Auflösung, Event-API, Config-Manager. Liefert **Designnotizen mit
Klassen- und Methodensignaturen**, keinen fertigen Code. Schreibt nur nach
`Psyerns_ChefZ_Docs/design/`.

### `chefz-core-engineer`
Implementiert `ChefZ_Core` in Enforce Script. Kennt die Script-Layer
(`3_Game` / `4_World` / `5_Mission`), `modded class`-Regeln, RPC, `ref`-Semantik.
Regel: **Der Core enthält keine einzige konkrete Zutat und kein konkretes Gericht.**

### `chefz-content-engineer`
Baut **einen vertikalen Slice** end-to-end: config.cpp-Klassen, Modell-Proxys,
Recipe-JSONs, Prozessschritte, stringtable-Einträge, Registry-Delta.
Wird pro Slice mehrfach parallel instanziiert.

### `chefz-registry-integrator`
Einziger Schreiber der zentralen Registries. Merged Deltas, löst Kollisionen,
lehnt widersprüchliche Deltas ab.

### `chefz-terje-comp-engineer`
Baut `Psyerns_ChefZ_Terje_Skills_Comp` und `Psyerns_ChefZ_Terje_Medicine_Comp`.
Muss `TerjeMods-master-main` vor jeder Änderung tatsächlich gelesen haben —
`CfgTerjeSkills`, die Skills-Accessor-API, die Medicine-Consumable-Parameter.
Harte Regel: **niemals Terje-Dateien verändern**, nur erweitern.

### `chefz-cot-comp-engineer`
Baut `Psyerns_ChefZ_COT_Comp` nach dem Muster von `TerjeCompatibilityCOT`
(config.cpp + `Scripts/4_World` + `Scripts/5_Mission`). Registriert ChefZ-Items
in den COT-Spawn-Kategorien.

### `chefz-asset-tracker`
Setzt für jedes neue Item ein plausibles Vanilla-Proxy-Modell und pflegt
`ChefZ_Asset_Backlog.md`: was braucht eigene Geometrie, was reicht als
Textur-Variante auf geteiltem Mesh (Shared-Mesh-Strategie aus §71 der Production Map).

### `chefz-doc-scribe`
Schreibt Gate-Reports und CHANGELOG, hält Modulspezifikationen aktuell.

## 6.2 Prüfende Agenten (nur lesend)

### `chefz-validator`
Führt die Node-Validatoren aus, interpretiert die Reports, ordnet jeden Fehler
einem Modul und einem verantwortlichen Agenten zu. Meldet strukturiert —
erfindet nichts und behauptet nie „grün" ohne Exit-Code 0.

### `chefz-conflict-scout` *(adversarial)*
Sucht aktiv nach Kollisionen: Klassennamen gegen Vanilla / Terje / Expansion,
doppelte `modded class`-Overrides auf dieselbe Vanilla-Klasse, `requiredAddons`,
die nicht zur tatsächlichen Nutzung passen, Load-Order-Fallen,
Client-/Server-Fehltrennung.

### `chefz-balance-reviewer` *(adversarial)*
Prüft Zahlen gegen die Doks: XP-Matrix (§26 Terje-Analyse), Yield-Boni,
Haltbarkeitsmultiplikatoren (§17 Planungsschritte), Portionsgrößen. Sucht
gezielt **Exploit-Loops**: XP durch Einlegen/Entfernen, Recycling-Schleifen,
doppelte Metabolism-XP, unbegrenzte Batch-XP, unendliche Ingredient Conversion.

---

# 7. Meilensteine und Gates

Vier Gates. Jedes endet mit einem Report in `Psyerns_ChefZ_Docs/GATE_<n>_REPORT.md`,
der enthält: was gebaut wurde, vollständige Validator-Ausgabe, offene Punkte,
Asset-Backlog-Stand und eine **In-Game-Testcheckliste für dich**.

## Meilenstein 1 — Core Foundation
*Deckt Phase 1 der Production Map ab.*

```text
Design-Panel: 3 unabhängige Architekturentwürfe
        │        (Recipe Engine · Matching · State-System · Event-API)
        ▼
Judge-Agent bewertet, wählt einen, veredelt ihn mit den besten Ideen der anderen
        ▼
chefz-core-engineer implementiert – Teilsystem für Teilsystem, sequenziell
        ▼
chefz-validator  +  chefz-conflict-scout
        ▼
                  ██ GATE 1 ██
```

Fertig, wenn: Mod lädt ohne RPT-Fehler · Debug-Log zeigt vollständigen
Recipe-Check · **Vanilla-Kochen funktioniert unverändert weiter, wenn kein
ChefZ-Rezept passt** · `$profile:ChefZ/` wird beim Start angelegt.

## Meilenstein 2 — Basisproduktion
*Phasen 2–5 und 7: Verarbeitung, Getreide, Kräuter/Gewürze/Salz, Fleisch/Wurst, Milch.*

Sechs vertikale Slices, parallel:

| Slice | Kette | Zielmodule |
|---|---|---|
| `grain` | Wheat → Flour → Dough → Bread / Pasta | Farming, Processing, Baking |
| `salt` | Saltwater → Raw Salt → Salt | Processing, Ingredients |
| `herbs` | Herb → Dried Herb → Seasoning / Mix | Farming, Processing, Ingredients |
| `meat` | Meat → Minced Meat → Sausage | Meat, Processing |
| `dairy` | Milk → Cream → Butter / Cheese | Ingredients, Processing |
| `produce` | Kartoffel, Tomate, Paprika, Pfeffer, Zwiebel, Knoblauch, Karotte, Kohl | Farming, Ingredients |

```text
6 Slices parallel  →  ██ Delta-Merge (Barriere) ██  →  Validator  →  Balance-Review
                                                                          ▼
                                                                    ██ GATE 2 ██
```

Fertig, wenn: alle Basisketten aus §75 der Production Map im Spiel durchlaufen ·
alle sieben Stationen funktionieren · keine ID-Kollision im Merge.

## Meilenstein 3 — Preservation, Serving, Gerichte
*Phasen 6 und 8.*

```text
Slice preservation   (Salzen · Trocknen · Räuchern · State-Übergänge · Haltbarkeit)
Slice serving        (Portionen · Plate/Bowl/Can/Jar · Empty Container Return)
Slice sauces         (Tomatensoße · Rahmsoße · Pilzrahm · Bone Broth)
Slice dishes-a       (Teller 1–10)
Slice dishes-b       (Teller 11–20)
Slice dishes-c       (5 Bowl-Gerichte · Suppen · Eintöpfe)
        ▼
Delta-Merge  →  Validator  →  Balance-Review  +  Conflict-Scout
        ▼
   ██ GATE 3 ██
```

Fertig, wenn: 20 Teller + 5 Bowls kochbar · Qualitätsstufen SIMPLE→PREMIUM greifen ·
Portionsentnahme funktioniert · Preservation-Matrix stimmt.

## Meilenstein 4 — Compatibility
*Phasen 9–10.*

Drei vollständig parallele Agenten in disjunkten Mod-Ordnern:

| Modul | Inhalt |
|---|---|
| `Psyerns_ChefZ_Terje_Skills_Comp` | Survival-XP nach XP-Matrix · Herbalist-Perk `chefzherb` (5 Stufen) · Anti-XP-Farming · optionale Recipe Locks |
| `Psyerns_ChefZ_Terje_Medicine_Comp` | Kräutertees · Immunity Gain · Food-Poisoning-Anbindung · Pharmacologist-Interaktion |
| `Psyerns_ChefZ_COT_Comp` | ChefZ-Items in COT-Spawnkategorien · Admin-Werkzeuge |

```text
3 Comp-Module parallel  →  Validator  →  Conflict-Scout  →  ██ GATE 4 ██
```

Fertig, wenn: ChefZ **ohne** Terje unverändert läuft · mit Terje XP korrekt
vergeben wird · keine doppelte Metabolism-XP · Safe Dinner / Wild Meat Lover
nicht dupliziert · COT spawnt alle ChefZ-Items.

---

# 8. Statische Validierung

Sieben Prüfer unter `tools/chefz-validate/`, Node ohne Abhängigkeiten,
Exit-Code ≠ 0 bei Fehlern.

| Validator | Prüft |
|---|---|
| `schema.mjs` | Alle ChefZ-JSONs gegen mitgelieferte Schemas (Recipes, Categories, Tags, Processing, Nutrition, Preservation, Deltas) |
| `classrefs.mjs` | Jede in JSON referenzierte Klasse existiert — in einer Projekt-`config.cpp` oder im Referenzindex (Vanilla / Terje) |
| `configcpp.mjs` | Jedes Addon hat `CfgPatches`; keine Klasse doppelt definiert; `requiredAddons[]` deckt die tatsächlich verwendeten Basisklassen |
| `naming.mjs` | `ChefZ_`-Präfix, PascalCase, keine Kollision mit Vanilla-/Terje-Namen |
| `stringtable.mjs` | Jede `#STR_CHEFZ_*`-Referenz existiert; keine verwaisten Einträge |
| `deltas.mjs` | ID-Kollisionen zwischen Slices; Parent-Kategorien vorhanden; Stationen vorhanden |
| `index.mjs` | Runner: führt alle aus, schreibt JSON-Report, setzt Exit-Code |

## Referenzindex

`build-refindex.mjs` indexiert die Nachbarmods automatisch. Stand jetzt:

| Quelle | Klassen |
|---|---:|
| Expansion | 7.786 |
| Vanilla-**Scripts** (`scripts - 1.29`) | 6.534 |
| Terje | 799 |
| COT | 579 |
| Dabs | 277 |
| CF | 193 |
| **Vanilla-Item-Klassen** | **0 — siehe unten** |

**Bekannte Grenze, ehrlich benannt:** Die Vanilla-*Item*-Klassen (`CookingPot`,
`Edible_Base`, `FryingPan` …) stecken in den Game-Configs, nicht in den
Script-Quellen — `scripts - 1.29` liefert sie nicht. Solange
`refindex/vanilla-classes.txt` leer ist, melden `classrefs.mjs` und `naming.mjs`
unbekannte **Fremd**klassen nur als Warnung mit dem Zusatz „nicht prüfbar".
Unbekannte **ChefZ_**-Klassen bleiben davon unberührt und sind immer ein Fehler —
diese Prüfung funktioniert vollständig.

Befüllen lässt sich der Index über entpackte Game-Data oder einen Klassendump vom
Server:

```
node tools/chefz-validate/build-refindex.mjs "<pfad zu den entpackten daten>"
```

Danach werden auch Kollisionen mit Vanilla-Itemnamen zu harten Fehlern. Das ist
die einzige nennenswerte Lücke im Prüfnetz und lohnt sich früh zu schließen.

## Abnahme der Validatoren

Die Validatoren wurden gegen ein absichtlich fehlerhaftes Wegwerf-Modul geprüft.
Alle sechs schlugen wie vorgesehen an — doppelte Klassendefinition, fehlende
`requiredAddons`, unbekannte Ergebnisklasse, Tippfehler im Rezeptfeld, fehlender
`RecipeID`, Namenskonventionsverstoß, fehlender Stringtable-Schlüssel, verwaister
Schlüssel, fehlende Elternkategorie, Kategorie-Zyklus, Geister-Station — 13 Fehler,
Exit-Code 1. Das Wegwerf-Modul wurde danach entfernt; der leere Projektstand
liefert Exit-Code 0.

Was diese Validierung **nicht** kann: Laufzeitverhalten, Kochlogik, Balancing im
Spiel, Modell-/Textur-Korrektheit. Das ist genau der Grund für die vier Gates.

---

# 9. Ausführung

## Slash-Commands

| Command | Wirkung |
|---|---|
| `/chefz-m1` | Meilenstein 1 – Core Foundation |
| `/chefz-m2` | Meilenstein 2 – Basisproduktion |
| `/chefz-m3` | Meilenstein 3 – Preservation, Serving, Gerichte |
| `/chefz-m4` | Meilenstein 4 – Compatibility |
| `/chefz-validate` | Validatoren allein laufen lassen |
| `/chefz-status` | Wo stehe ich? Artefakte, offene Deltas, Backlog, nächster Schritt |

Jeder Meilenstein-Command startet ein Workflow-Skript aus `.claude/workflows/`.
Ein Skript kann jederzeit mit `resumeFromRunId` fortgesetzt werden — geänderte
Stufen laufen neu, unveränderte kommen aus dem Cache.

## Was du bei jedem Gate tust

1. Gate-Report in `Psyerns_ChefZ_Docs/` lesen
2. PBOs mit DayZ Tools bauen und signieren
3. Auf den Testserver bringen, In-Game-Checkliste durchgehen
4. Ergebnis zurückmelden: freigeben, oder Fehlerliste — dann läuft der
   Meilenstein mit Korrekturauftrag erneut

---

# 10. Regeln, die für alle Agenten gelten

1. **Der Core bleibt Terje-frei.** Keine Terje-Referenz in `ChefZ_Core`, in
   keiner Form — kein `#ifdef`, kein optionaler Aufruf, kein Klassenname.
2. **ChefZ blockiert Vanilla-Kochen nie.** Passt kein ChefZ-Rezept, läuft das
   Vanilla-Verhalten unverändert weiter. Diese Regel wird in Gate 1 explizit geprüft.
3. **Content kommt in Module, Systeme in den Core.** Ein neues Gericht darf
   niemals eine Core-Codeänderung nötig machen.
4. **Zentrale Registries nur über Deltas.**
5. **Terje- und Vanilla-Dateien werden nie verändert**, nur erweitert.
6. **XP nur nach erfolgreichem Abschluss**, nie beim Starten einer Aktion.
7. **Namenskonvention** nach §53 des DME-Plans: `ChefZ_PascalCase`.
8. **Keine Erfolgsmeldung ohne Beleg.** „Validierung grün" nur mit tatsächlichem
   Exit-Code 0 im Report.
9. **Fremde Repos sind Lesequellen**, keine Arbeitsordner. In
   `Mod Repositories\` wird nichts geschrieben.

---

# 11. Offene Designfragen

Diese sind **vor** dem jeweiligen Meilenstein zu entscheiden — der
`chefz-architect` legt sie in M1 als Entscheidungsvorlage vor:

| Frage | Gebraucht ab | Quelle |
|---|---|---|
| Food States: eigene Klassen, DayZ-FoodStages oder ChefZ-Variablen? | M1 | Planungsschritte §2 |
| Wie hart sind Recipe Locks — gesperrt, schlechter, langsamer oder weniger Ausbeute? | M4 | Planungsschritte §21 |
| Teller: vor oder nach dem Kochen, wiederverwendbar? | M3 | Planungsschritte §12 |
| Hygiene-System in V1 oder später? | M3 | Planungsschritte §13 |
| Dynamische Gerichtebewertung aus Zutaten — Umfang? | M3 | Planungsschritte §9 |
| Perfect-Cook-/Burn-Fenster: in V1? | M3 | Planungsschritte §10 |

---

# 12. Was dieser Workflow bewusst nicht abdeckt

- **3D-Asset-Produktion.** Agenten setzen Vanilla-Proxys und führen Backlog.
- **PBO-Build und Signierung.** DayZ Tools, manuell.
- **Server-Deployment und RPT-Auswertung.** Manuell.
- **In-Game-Balancing.** Nur du kannst beurteilen, ob sich das Kochen gut anfühlt.
- **V2+ Inhalte** aus §74 der Production Map (Fermentation, Canning, Pickling,
  eigener Cooking Skill, Hygiene-Tiefe).
