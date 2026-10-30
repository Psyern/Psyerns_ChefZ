# Config/Recipes — bewusst leer

Der Slice "preservation" bringt **kein** Rezept mit, und das ist keine Lücke.

Ein ChefZ-**Rezept** (`kind: "recipe"`) zündet ausschließlich an einem
Kochgerät — Pfanne, Topf, Kessel, Feuerstelle, Ofen (`08` §2). Trocknen und
Räuchern sind keine Kochvorgänge:

| Vorgang | Form | Warum |
|---|---|---|
| Trocknen | `transform` über `PROCESS_DRY` am `ChefZ_DryingRack` | dauert Stunden und tickt ohne Spieler weiter (`11` §3, `STATION_TIMED`) |
| Räuchern | `transform` über `PROCESS_SMOKE` am `ChefZ_Smoker` | ebenso, zusätzlich mit Wärmebedarf |

Die Datensätze liegen deshalb in `Config/Processing/`:
`Drying.json`, `Smoking.json`.

Salzen / Pökeln (`PROCESS_SALT_CURE`, `Salting.json`, `ChefZ_SaltedMeat`,
`ChefZ_SaltedFish`) ist am 30.10.2026 entfallen, ebenso das Trocknen von Fleisch
und Fisch. Fleisch und Fisch werden nur noch geräuchert; getrocknet wird hier nur
noch die Rohwurst (`TR_RawSausageToDry`).

Ein Rezept daraus zu machen, wäre nicht nur unnötig, sondern falsch: Vanillas
Kochkette kennt für das Trocknen genau **einen** Übergang `RAW -> DRIED` und
sonst `BURNED` (`01` V14). Die Matrix aus Production Map §56 verlangt vier
Übergänge mit verschiedenen Haltbarkeiten. Das ist in Vanillas Kette nicht
abbildbar — der Grund, aus dem `11` E6 eigene Stationen vorschreibt und
`ChefZ_Smoker` überhaupt existiert.

Sollte später ein **Gericht** aus konservierten Zutaten entstehen
(Production Map §60-§65, etwa der Jägerteller), gehört sein Rezept in den Slice,
der das Gericht baut — nicht hierher. Dieser Slice endet bei der haltbaren
Zutat.
