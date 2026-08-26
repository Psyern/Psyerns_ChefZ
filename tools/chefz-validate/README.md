# ChefZ – Statische Validierung

Node >= 18, keine Abhaengigkeiten. Alle Pfade relativ zur Projektwurzel.

```bash
node tools/chefz-validate/index.mjs               # lesbarer Bericht
node tools/chefz-validate/index.mjs --json        # JSON fuer Agenten
node tools/chefz-validate/index.mjs --only=deltas,schema
node tools/chefz-validate/build-refindex.mjs      # Referenzindex neu bauen
```

Exit-Code: `0` sauber · `1` Fehler gefunden · `2` ein Validator selbst kaputt.

## Die sechs Pruefer

| Datei | Prueft |
|---|---|
| `schema.mjs` | Form aller ChefZ-JSONs: Rezepte, Zutaten, Deltas. Pflichtfelder, Typen, doppelte RecipeIDs, unbekannte Felder (Tippfehler). |
| `configcpp.mjs` | `config.cpp` und `$PREFIX$` je Modul, `CfgPatches` vorhanden und eindeutig, `requiredAddons` gesetzt, `units[]` vollstaendig, keine Klasse doppelt definiert, jede `modded class` benannt. |
| `classrefs.mjs` | Jede in JSON referenzierte Klasse und jede Elternklasse existiert – im Projekt, in einem Delta oder im Referenzindex. |
| `naming.mjs` | `ChefZ_PascalCase`, keine Kollision mit Fremdklassen. |
| `stringtable.mjs` | Jeder `#STR_CHEFZ_*` ist definiert; keine Doppelten; verwaiste Schluessel als Warnung. |
| `deltas.mjs` | ID-Kollisionen zwischen Slices, Parent-Kategorien, Kategorie-Zyklen, Stationen, und ob der Merge in den zentralen Registries angekommen ist. |

## Referenzindex

`refindex/*.txt` – eine Klasse je Zeile, `#` ist Kommentar.
Automatisch gebaut aus den Nachbarmods unter `Desktop/Mod Repositories`.

**Luecke:** `vanilla-classes.txt` ist leer. Die Vanilla-*Item*-Klassen stecken in
den Game-Configs, nicht in `scripts - 1.29`. Solange sie fehlen, sind unbekannte
Fremdklassen nur eine Warnung ("nicht pruefbar"). Unbekannte `ChefZ_`-Klassen
sind davon unabhaengig und immer ein Fehler.

Befuellen:

```bash
node tools/chefz-validate/build-refindex.mjs "<pfad zu entpackten game-data>"
```

oder die Namen von Hand in `refindex/vanilla-classes.txt` eintragen.

## Was hier NICHT geprueft wird

Laufzeitverhalten, Kochlogik, Balancing im Spiel, Modelle und Texturen, der
PBO-Build. Dafuer gibt es die vier Gates mit In-Game-Testchecklisten.
