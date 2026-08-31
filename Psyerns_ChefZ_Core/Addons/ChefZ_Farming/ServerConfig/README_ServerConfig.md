# ChefZ Wildwuchs — CE-Fragment für die Mission

Zwei XML-Vorlagen und diese Anleitung. Sie sind **kein Modinhalt**: der Mod
liest sie nie, sie werden nicht ins PBO gepackt, und sie tun nichts, solange
niemand sie in eine Mission einbaut.

**Der Einbau ist Menschensache — wie PBO-Pack und Deploy.** Er gehört an ein
Gate, nicht in einen Agentenlauf.

---

## Warum das hier liegt und nicht im PBO

Die Wildpflanzen sind Weltobjekte, die die Central Economy verteilt. Was die CE
tut, steht in der Mission (`mpmissions\<mission>\`), nicht im Mod — ein Mod kann
`events.xml` und `types.xml` nicht mitbringen, er kann nur die Klassen liefern,
auf die sie zeigen.

Ohne diesen Einbau passiert genau nichts: die vier Klassen existieren, sind
per Admin-Werkzeug spawnbar und erntbar, aber keine einzige Pflanze erscheint
von selbst.

**Gepackt werden die Dateien nicht.** `ChefZ_Farming/include.txt` führt seit dem
31.08.2026 kein `*.xml` mehr — das Modul hat kein einziges XML, das ins PBO
gehört, und mit dem Muster in der Liste wären genau diese zwei Vorlagen im
Addon gelandet. `README_ServerConfig.md` fällt ohnehin nicht unter die Muster.

---

## Einbau in vier Schritten

### 1. Ordner anlegen und Dateien kopieren

```
mpmissions\<deine Mission>\ChefZ\
    ChefZ_events.xml
    ChefZ_types.xml
```

Der Ordnername `ChefZ` ist frei wählbar; er muss nur mit Schritt 2
übereinstimmen.

### 2. In `cfgeconomycore.xml` registrieren

In `mpmissions\<deine Mission>\cfgeconomycore.xml`, innerhalb von
`<economycore>`, neben die schon vorhandenen `<ce>`-Blöcke:

```xml
<ce folder="ChefZ">
    <file name="ChefZ_types.xml"  type="types"/>
    <file name="ChefZ_events.xml" type="events"/>
</ce>
```

Die Testmission benutzt diesen Weg bereits für fünfzehn andere Mods — der Block
sieht dort genauso aus.

### 3. Server neu starten

Die CE liest `cfgeconomycore.xml` nur beim Start.

### 4. Prüfen

Im CE-Log (`log_ce_dynamicevent` bzw. `log_ce_loop` in den `<defaults>` von
`cfgeconomycore.xml` auf `true`) müssen `ChefZTrajectoryCorn` und
`ChefZTrajectoryHerbs` auftauchen. Im Spiel: 25 bis 100 m um sich herum laufen
und schauen.

---

## Was in den Dateien steht

### `ChefZ_events.xml`

Zwei Events nach Vanillas Pilz- und Obstbauform (`db\events.xml`,
`TrajectoryHumus` / `TrajectoryApple`):

| Event | nominal | Kinder |
|---|---:|---|
| `ChefZTrajectoryCorn` | 120 | `ChefZ_WildCorn` |
| `ChefZTrajectoryHerbs` | 140 | `ChefZ_WildThyme`, `ChefZ_WildRosemary`, `ChefZ_WildParsley` |

Beide `position=player`, `limit=mixed`, `active=1`, `lifetime=180`,
`saferadius=25`, `distanceradius=100`, `cleanupradius=25`. Das heißt: die
Engine sucht sich einen Fleck im Ring 25–100 m um einen Spieler und stellt
dort eine Pflanze hin. **Wo genau, entscheidet die Engine** — Mais steht damit
nicht garantiert auf einem Acker. Das ist der bewusst akzeptierte Preis dafür,
dass keine Koordinate gepflegt werden muss und das Ganze auf jeder Karte läuft
(Spec Kap. 1).

`nominal` ist die Zielzahl gleichzeitig existierender Instanzen serverweit.
260 zusammen ist etwa das, was Vanilla allein für Pilze und Obst fährt
(sieben Trajectory-Events zu je 140).

### Warum die Maisgruppe nicht hier steht

Der Auftrag will Mais „in Gruppen von 1–3 nebeneinander". Der naheliegende Weg
wäre, das Event mehrere Kinder je Instanz setzen zu lassen. **Das kann ein
`position=player`-Event nicht**, und das ist geprüft, nicht vermutet:

- In der Testmission gibt es 75 Events. Die mit `position=player` sind die acht
  `Trajectory*` und siebzehn `Infected*`. **Keines** davon setzt eine Gruppe an
  einem Ort.
- Die `Infected*`-Events tragen zwar `min`/`max` an ihren Kindern, aber als
  **Gewichte**: `InfectedNBC` hat ein einziges Kind mit `min="100"`,
  `InfectedPrisoner` ebenso, `InfectedPolice` verteilt 40/40/20/20. Das sind
  Prozentangaben einer Auswahl, keine Stückzahlen.
- Echte Gruppen an einem Ort kennt die CE über `cfgeventgroups.xml`. Die Gruppen
  dort tragen **feste Relativkoordinaten** (`x`/`y`/`z`/`a` je Kind) und werden
  über `cfgeventspawns.xml` an eine **feste Weltposition** gebunden
  (`<pos x=… z=… group="…"/>`, Z.8500 ff.). Ein `position=player`-Event hat
  überhaupt keinen Eintrag in `cfgeventspawns.xml` — die beiden Mechanismen
  schließen einander aus.

Deshalb der in der Spec vorgesehene Fallback: `ChefZ_WildCorn.EEOnCECreate()`
würfelt 0–2 Begleiter und stellt sie 1–2 m daneben. Begleiter sind dieselbe
Klasse und tragen damit dieselbe `types.xml`-Zeile — dieselbe `lifetime`,
dieselbe Zählung. Eine Rekursionswache verhindert, dass ein Begleiter seinerseits
Begleiter setzt.

**Folge für die Zählung:** Die tatsächliche Zahl Maispflanzen in der Welt liegt
im Mittel beim **Doppelten** des `nominal` (1 + durchschnittlich 1 Begleiter).
`nominal 120` heißt also grob 240 Maispflanzen. Wer das anders will, dreht an
`nominal` — nicht am Skript.

### `ChefZ_types.xml`

Vier Einträge nach Pilzmuster: `nominal=0`, `lifetime=900`, `crafted="1"`,
Kategorie `food`, keine `usage`, keine `tag`, kein `value`.

`nominal=0` ist wichtig und kein Versehen: die Zeile ist ein **Limit-Container**.
Sie sagt, wie lange eine Pflanze liegen bleibt und wie sie gezählt wird — nicht,
wie viele erscheinen. Das sagt allein das Event. Wer hier ein `nominal` setzt,
bekommt Maispflanzen im Loot von Gebäuden.

---

## Stellschrauben für Betreiber

| Wunsch | Wo |
|---|---|
| Mehr / weniger Pflanzen | `nominal` im jeweiligen Event |
| Pflanzen bleiben länger stehen | `lifetime` im **Event** (180 s bis zum Cleanup) und `lifetime` im **types** (900 s) |
| Näher / weiter vom Spieler | `saferadius` / `distanceradius` |
| Nur Kräuter, kein Mais | `<active>0</active>` in `ChefZTrajectoryCorn` |
| Eine Pflanze ganz abschalten | Ihr `<child>`-Element aus dem Event entfernen |

Die Ausbeute je Pflanze steht **nicht** hier — sie steht im Mod
(`Config/Processing/README_WildPlants.md`).
