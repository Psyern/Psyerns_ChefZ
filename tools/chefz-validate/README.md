# ChefZ – Statische Validierung

Node >= 18, keine Abhaengigkeiten. Alle Pfade relativ zur Projektwurzel.

```bash
node tools/chefz-validate/index.mjs               # lesbarer Bericht
node tools/chefz-validate/index.mjs --json        # JSON fuer Agenten
node tools/chefz-validate/index.mjs --only=chefzsym,chefzcore
node tools/chefz-validate/build-refindex.mjs      # Referenzindex neu bauen
node tools/chefz-validate/selftest.mjs            # pruefen, ob die Pruefer sehen
```

Exit-Code: `0` sauber · `1` Fehler gefunden · `2` ein Validator selbst kaputt.

## Die zwoelf Pruefer

Form der Dateien:

| Datei | Prueft |
|---|---|
| `schema.mjs` | Form aller ChefZ-JSONs: Rezepte, Zutaten, Deltas. Pflichtfelder, Typen, doppelte RecipeIDs, unbekannte Felder (Tippfehler). |
| `configcpp.mjs` | `config.cpp` und `$PREFIX$` je Modul, `CfgPatches` vorhanden und eindeutig, `requiredAddons` gesetzt, `units[]` vollstaendig (nur Klassen **direkt** unter `CfgVehicles` – tiefer liegen Unterknoten wie Skinning-Ertraege und Garstufenuebergaenge), keine Klasse doppelt definiert, jede `modded class` benannt. Ein Asset-Paket ohne `ChefZ_Core` schweigt mit dem Marker `ASSET-PBO`. |
| `classrefs.mjs` | Jede in JSON referenzierte Klasse und jede Elternklasse existiert – im Projekt, in einem Delta oder im Referenzindex. Seit S19 auch `cls`, `portionClass`, `emptyClass`, `emptyOnLastPortion`, `returnContainer`, `deviceClasses[]`. |
| `naming.mjs` | `ChefZ_PascalCase`, keine Kollision mit Fremdklassen. |
| `stringtable.mjs` | Jeder `#STR_CHEFZ_*` ist definiert; keine Doppelten; verwaiste Schluessel als Warnung. |
| `deltas.mjs` | ID-Kollisionen zwischen Slices, Parent-Kategorien, Kategorie-Zyklen, Stationen, und ob der Merge in den zentralen Registries angekommen ist. |

Bedeutung des Inhalts (S19, Entwurf `19` §3):

| Datei | Regel | Aus |
|---|---|---|
| `chefzsym.mjs` | Jede Symbolreferenz in JSON und `CfgChefZ*` existiert in den gemergten Registries; geschlossene Wertelisten (`completion`, `exec`, `scope`, Garstufen, Kochmethoden ...) werden mitgeprueft. | **Auflage zu `03` E1 / OF-11** |
| `chefzcore.mjs` | In `Addons/ChefZ_Core/**` kein Fremdsystemname, kein Content-Bezeichner, keine Content-Aufzaehlung, kein eigener Content-Datensatz. | **Auflage zu I3 und I4** |
| `chefznut.mjs` | Jede essbare Ergebnis- und Portionsklasse hat `class Nutrition` oder `class Food` **und** `scope != 0`. Klassen, deren Config-Kette bei einer Nicht-Nahrungsbasis endet (`Inventory_Base`, `GardenLime`), sind entschieden und melden nichts. | `01` V7 |
| `chefzstage.mjs` | Jede kochbare ChefZ-Klasse deklariert `FoodStageTransitions` – sonst verbrennt sie im Topf. | `01` V4 |
| `chefzproc.mjs` | `HANDCRAFT`-Transforms: 1 bis 2 Eingaenge, Werkzeug nur bei einem Eingang, hoechstens 10 Ergebnisse. | `01` V12 |
| `chefzlog.mjs` | Kein ungewachter `ChefZ_Log.Debug/Trace`-Aufruf innerhalb einer Schleife. | `18` E2 |

`chefzsym` und `chefzcore` sind **Auflagen, keine Zugaben**. Ohne sie ist der
datengetriebene Entwurf schlechter als ein enum-basierter (`03` E1), und die
Invarianten I3/I4 waeren Absichtserklaerungen (`19` S19).

### Schwere folgt der Laufzeit

Ein Befund ist genau dann ein **Fehler**, wenn der Core zur Laufzeit den
Datensatz abweist oder das Ergebnis stumm falsch waere; sonst eine **Warnung**.
Beispiele: unbekannte Kategorie im Selektor = Fehler (der `ChefZ_SelectorCompiler`
weist ab), unbekannte Station in `stationsAllowed` = Warnung (`11` §7: sie kann
aus einem optionalen Modul kommen).

Ist ein Namensraum projektweit leer, meldet `chefzsym` **eine** Zeile
„nicht pruefbar" statt hundert Fehlern. Der Core allein bringt keinen Content
mit; ein Validator, der im Normalzustand rot ist, wird nach zwei Wochen ignoriert.

### Was `chefzcore` NICHT ahndet

Prosa. Mehrere Core-Dateien schreiben ausdruecklich hin, welche Content-Namen
dort **nicht** stehen duerfen; wer das ahndete, bestrafte die Dokumentation der
Regel. Geprueft wird der Code – Bezeichner und Zeichenketten. **Ausnahme:**
Fremdsystemnamen (I4) werden auch im Kommentar gemeldet, denn ein Kommentar-Hook
ist der Anfang einer Abhaengigkeit.

Zeichenketten mit dem Praefix `CHEFZ_` sind Testmarken der Selbsttests und
ausgenommen: der Praefix ist reserviert und kann keinen Content bezeichnen.

Ein Fremdsystemname **im Code** ist ein Fehler, ohne Ausnahme. Im **Kommentar**
ist er eine Warnung, denn die Vanilla-Befunde des Projekts belegen ihre Aussagen
an ausgelieferten Fremdmods – ein Beleg ist das Gegenteil einer Abhaengigkeit,
sieht aber genauso aus wie ein Hook. Wer den Beleg behalten will, schreibt
`I4-BELEG` in den Kommentarblock; dann steht der Fund nur noch als Hinweis im
Bericht. Das ist die einzige Tuer, sie ist eng, und sie ist greppbar:

```bash
grep -rn "I4-BELEG" Psyerns_ChefZ_Core/Addons/ChefZ_Core
```

## Die zwei Marker

Zwei Befunde dieses Prueferbestands sind Fragen, keine Fehler – und eine Frage
braucht einen Ort fuer die Antwort. Beide Marker stehen als Wort im
Quelltext: bewusst, sichtbar und mit `grep` zu finden.

| Marker | Wohin | Beantwortet |
|---|---|---|
| `I4-BELEG` | in den Kommentar davor (bis zwoelf Zeilen) | `chefzcore`: „ein Fremdmodname steht im Core" – ja, als Beleg einer Beobachtung, nicht als Anbindung. |
| `ASSET-PBO` | irgendwo in die `config.cpp` des Moduls, ueblich im Kopf | `configcpp`: „dieses Modul nennt `ChefZ_Core` nicht in `requiredAddons`" – ja, es ist ein reines Dateipaket; eine `.p3d` haengt von keinem Skriptmodul ab. |
| `SCOUT-GEPRUEFT <Datum>` | in den Kommentar ueber der `modded class` (bis zwoelf Zeilen) | `configcpp`: „diese Klassenerweiterung ist eine Kollisionsflaeche" – ja, und sie wurde am genannten Tag vom `chefz-conflict-scout` geprueft. |

Alle drei sind absichtlich eng: sie unterdruecken genau eine Meldung an genau
einer Stelle, und wer sie setzt, hinterlaesst ein Wort, nach dem der naechste
Leser suchen kann. Ein Marker, der einen ganzen Pruefer stumm schaltet, waere
das Gegenteil davon.

`SCOUT-GEPRUEFT` traegt zusaetzlich ein Datum, weil er eine Aussage ueber einen
Zeitpunkt macht und keine ueber die Ewigkeit: wer die Klasse spaeter umbaut,
muss ihn neu verdienen. Suchen laesst sich der Bestand mit

```bash
grep -rn "SCOUT-GEPRUEFT" --include=*.c .
```

## Selbstpruefung

Ein Pruefer, der nie etwas findet, ist von einem kaputten Pruefer nicht zu
unterscheiden – beide melden „bestanden".

`selftest.mjs` baut deshalb das **Wegwerf-Modul** aus `19` S19 in einem
Temporaerverzeichnis, laesst `index.mjs` darauf laufen (`CHEFZ_VALIDATE_ROOT`),
prueft, dass jede Regel mindestens einmal zuendet und der Lauf mit Exit-Code 1
endet, und loescht den Baum wieder.

```bash
node tools/chefz-validate/selftest.mjs            # Tabelle: zuendet / BLIND
node tools/chefz-validate/selftest.mjs --verbose  # dazu jeder einzelne Befund
```

`CHEFZ_VALIDATE_ROOT=<pfad>` haengt die Projektwurzel um. Nur dafuer gedacht.

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

## Aufbau

```text
index.mjs        Runner, Bericht, Exit-Code
lib.mjs          Dateien, config.cpp (flach und als Baum), Skriptklassen, Referenzindex
chefzdata.mjs    das ChefZ-Datenmodell: Records aus Rang 1 + Rang 2 + Deltas, gemergte Registries
chefzfood.mjs    Nahrungsfakten fuer chefznut und chefzstage (Vererbung, Food-Knoten, Essbarkeit)
<pruefer>.mjs    je eine Regelgruppe, Standardexport liefert Findings
selftest.mjs     Wegwerf-Modul, prueft die Pruefer
```

## Was hier NICHT geprueft wird

Laufzeitverhalten, Kochlogik, Balancing im Spiel, Modelle und Texturen, der
PBO-Build. Dafuer gibt es die vier Gates mit In-Game-Testchecklisten.

Ebenso ungeprueft und mit Begruendung: Effekt-IDs (opak, `12` §3), Faehigkeiten
(meldet ein Fremdmodul zur Laufzeit an, `17` §4), Eventnamen (offener Raum,
`17` §2) und Fluessigkeiten (`cfgLiquidDefinitions` der Game-Config).
`chefzsym` weist im Bericht ausdruecklich darauf hin, statt zu schweigen.
