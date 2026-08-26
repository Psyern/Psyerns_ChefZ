# ChefZ – Weitere Planungsschritte

## Übersicht

Nach der Definition von **ChefZ_Core** und der ersten **Terje-Kompatibilitätsanalyse** sollte ChefZ jetzt von einer groben Idee zu einem umsetzbaren Game- und Technikdesign weiterentwickelt werden.

Der Fokus sollte dabei nicht nur auf weiteren Rezepten liegen, sondern auf klar definierten Systemen, Produktionsketten, Balancing-Regeln und Modulgrenzen.

---

# 1. ChefZ-Modulstruktur endgültig festlegen

Die Modulstruktur sollte verbindlich definiert werden.

Vorgeschlagene Module:

```text
ChefZ_Core
ChefZ_Farming
ChefZ_Ingredients
ChefZ_Processing
ChefZ_Meat
ChefZ_Preservation
ChefZ_Cooking
ChefZ_Baking
ChefZ_UI
ChefZ_TerjeSkills
ChefZ_TerjeMedicine
```

Für jedes Modul sollte festgelegt werden:

- Abhängigkeiten
- enthaltene Klassen
- enthaltene Items
- enthaltene Configs
- enthaltene Events
- optionale Module
- benötigte Core-Funktionen
- benötigte Compatibility-Hooks

---

# 2. Item- und Food-State-System definieren

ChefZ sollte klar festlegen, welche Lebensmittelzustände existieren.

Mögliche Zustände:

```text
RAW
PREPARED
COOKED
BOILED
FRIED
BAKED
DRIED
SMOKED
SALTED
PICKLED
FERMENTED
BURNT
ROTTEN
```

Zu klären:

- eigene Klassen pro Zustand?
- Nutzung von DayZ FoodStages?
- zusätzliche ChefZ-Variablen?
- Vererbung der Qualität?
- Vererbung der Haltbarkeit?
- Übergänge zwischen Zuständen

---

# 3. Vollständige Zutatenliste für ChefZ V1

Es sollte eine vollständige V1-Zutatenliste erstellt werden.

Beispiele:

## Getreide

```text
Wheat
→ Flour
→ Dough
→ Bread
→ Pasta
```

## Fleisch

```text
Meat
→ Minced Meat
→ Sausage
→ Smoked Sausage
```

## Milch

```text
Milk
→ Cream
→ Butter
→ Cheese
```

## Paprika

```text
Paprika
→ Dried Paprika
→ Paprika Powder
```

## Salz

```text
Saltwater
→ Raw Salt
→ Salt
```

Diese Liste sollte später als vollständige **Production Dependency Map** ausgearbeitet werden.

---

# 4. Kräuter- und Pflanzenökologie

Für jede Pflanze sollte definiert werden:

- Spawngebiet
- Seltenheit
- Spawnmenge
- Respawn
- Harvest-Menge
- Seed-System
- anbaubar oder nur wild
- benötigtes Werkzeug
- 3D-Modell
- frische Variante
- getrocknete Variante

Mögliche Pflanzenbereiche:

```text
Waldkräuter
Feldkräuter
Gewässerpflanzen
Gartenpflanzen
Gewächshauspflanzen
```

---

# 5. Herbalist-Perk vollständig planen

Der geplante Terje-Survival-Perk:

```text
chefzherb
```

sollte vollständig spezifiziert werden.

Zu definieren:

- Highlight-Reichweite je Stufe
- Highlight-Partikel
- Highlight-Farbe
- Yield-Bonus
- XP bei Ernte
- seltene Kräuter
- Spawn-/Detection-Regeln
- Client-/Server-Aufteilung

Mögliche Spezialfähigkeit:

```text
Rare Herb Sense
```

Ab höherer Perk-Stufe könnten seltene Pflanzen deutlicher erkannt werden.

---

# 6. Weitere ChefZ-Terje-Perks

Mögliche zusätzliche Survival-Perks:

## Herbalist

Fokus:

- Kräuter
- Pflanzen
- Ernteausbeute
- Detection

## Field Cook

Fokus:

- Kochzeit
- Verbrennungsrisiko
- Dish Quality
- Premium-Gerichte
- zusätzliche Portionen

## Preserver

Fokus:

- Räuchern
- Trocknen
- Salzverbrauch
- Haltbarkeit
- Ausbeute

Zu prüfen:

- alle unter Survival?
- teilweise Medicine?
- teilweise Metabolism?

---

# 7. TerjeMedicine-Kräutersystem

ChefZ könnte ein eigenes Kräuter- und Tee-System erhalten.

Mögliche Produkte:

```text
Thyme Tea
Wild Garlic Tea
Herbal Tea
Bitter Tea
Restorative Tea
```

Wichtig:

Kräuter sollen Medikamente nicht ersetzen.

Mögliche Effekte:

- kleine Immunity-Unterstützung
- leichte Regeneration
- Schlafunterstützung
- leichte Antipoison-Unterstützung
- Recovery
- Wärme
- Hydration

---

# 8. Cooking-Quality-System

Die bisherigen Qualitätsstufen:

```text
SIMPLE
PREPARED
SEASONED
PREMIUM
```

sollten technisch definiert werden.

Mögliche Faktoren:

- Grundrezept vollständig
- optionale Zutaten
- passende Kräuter
- Gewürze
- Qualität der Rohstoffe
- Food State
- Terje-Skill
- ChefZ-Perks
- Kochtemperatur
- Kochzeit

---

# 9. Dynamische Gerichtebewertung

Gerichte könnten einen Teil ihrer Werte aus den Zutaten übernehmen.

Mögliche übertragene Eigenschaften:

- Energy
- Water
- Freshness
- Meat Type
- Ingredient Quality
- Food Risk
- Nutrition
- Spoilage

Beispiel:

```text
Fresh Venison
+ Fresh Mushrooms
+ Fresh Herbs
→ hochwertiger Hunter Stew
```

gegenüber:

```text
Old Meat
+ damaged ingredients
→ schlechtere Qualität
```

---

# 10. Kochzeit und Verbrennen

Für jedes Rezept sollte definiert werden:

- Mindesttemperatur
- optimale Temperatur
- Kochdauer
- Perfect-Cook-Fenster
- Overcook-Zeit
- Burn-Zeit

Mögliche Zustände:

```text
RAW
↓
COOKING
↓
PERFECT
↓
OVERCOOKED
↓
BURNT
```

Der `Field Cook`-Perk könnte diese Fenster beeinflussen.

---

# 11. Portions- und Gruppensystem

Unterschiedliche Kochgeräte sollten unterschiedliche Portionsgrößen erzeugen.

Beispiel:

```text
Small Pot
→ 2–3 Portionen

Cooking Pot
→ 4 Portionen

Cauldron
→ 8–12 Portionen
```

Mögliche Interaktion:

```text
Take Portion
```

Dadurch kann eine Gruppe gemeinsam kochen.

---

# 12. Teller- und Schüssel-System

Zu definieren:

- wann wird ein Teller benötigt?
- vor oder nach dem Kochen?
- wird das Gericht aus dem Topf portioniert?
- bleibt ein leerer Teller übrig?
- sind Teller wiederverwendbar?
- können Teller beschädigt werden?
- können Teller verschmutzen?
- müssen Teller gereinigt werden?

Mögliche Behälter:

```text
PLATE
BOWL
CAN
JAR
BOX
```

---

# 13. Hygiene-System

Optional, aber sehr passend zu ChefZ und TerjeMedicine.

Mögliche Mechaniken:

- schmutziger Teller
- schmutziges Messer
- kontaminiertes Schneidebrett
- Kontakt mit rohem Fleisch
- Salmonella-Risiko
- verdorbene Zutaten
- kontaminiertes Wasser

Reinigung:

```text
Water
→ Wash

Boiling Water
→ Sterilize
```

---

# 14. Wurstsystem vertiefen

Das Wurstsystem sollte vollständig definiert werden.

Mögliche Parameter:

- Fleischanteil
- Fettanteil
- Salzmenge
- Gewürze
- Wursthülle
- Fleischart
- Rohzustand
- Kochzustand
- Räuchern
- Trocknen
- Haltbarkeit

Beispiel:

```text
Meat
+ Fat
+ Salt
+ Pepper
+ Casing
→ Raw Sausage
```

---

# 15. Fleischqualität und Tierarten

Tierarten könnten unterschiedliche kulinarische Eigenschaften besitzen.

Beispiele:

## Schwein

- hoher Fettanteil
- sehr gut für Wurst

## Hirsch

- mager
- hochwertiges Wildfleisch

## Wildschwein

- kräftig
- gut für Räucherwurst

## Bär

- energiereich
- erhöhtes Gesundheitsrisiko
- besonders gründliches Garen nötig

## Huhn

- leicht
- gut für Suppen und Frühstück

## Fisch

- hoher Wasseranteil
- kurze Haltbarkeit
- gut zum Räuchern und Trocknen

---

# 16. Salz als Wirtschaftssystem

Salz sollte vollständig gebalanced werden.

Zu definieren:

- benötigte Salzwassermenge
- Kochdauer
- Rohsalzausbeute
- Trocknungsdauer
- Endausbeute
- Energie-/Brennstoffverbrauch
- mögliche Batch-Größen

Beispiel:

```text
1000 ml Saltwater
↓
Boiling
↓
Raw Salt
↓
Drying
↓
X g Salt
```

Salz kann dadurch zu einer echten Handelsressource werden.

---

# 17. Konservierungs-Balancing

Mögliche Haltbarkeitsmultiplikatoren:

| Zustand | Beispiel-Multiplikator |
|---|---:|
| Raw | 1.00 |
| Cooked | 0.80 |
| Salted | 0.50 |
| Smoked | 0.25 |
| Dried | 0.15 |
| Pickled | 0.10 |
| Canned | 0.02 |

Zusätzlich zu prüfen:

- Nährstoffverlust
- Wasserverlust
- Gewichtsverlust
- Qualitätsverlust
- Geschmack/Quality Bonus

---

# 18. Food Economy und Trader

Zu entscheiden:

- welche Zutaten nur Loot sind
- welche Zutaten farmbar sind
- welche Zutaten craftbar sind
- welche Items Händler verkaufen
- welche Items Händler kaufen
- welche Items niemals gekauft werden können

Besonders relevant:

- Mehl
- Hefe
- Milch
- Salz
- Pfeffer
- Paprikapulver
- seltene Kräuter
- fertige Premium-Gerichte

---

# 19. Spawn- und Lootdesign

Lebensmittel sollten logisch in der Welt verteilt werden.

Beispiele:

## Mehl

- Supermärkte
- Küchen
- Farmgebäude

## Hefe

- Küchen
- Bäckereien
- selten

## Pfeffer

- Gewächshäuser
- Gärtnereien
- besondere Farmgebäude

## Kräuter

- natürliche Spawnpunkte

## Milch

- Farmen
- seltene Lebensmittelspawns

## Fleisch

- primär Jagd

---

# 20. Recipe Discovery und Cookbook

Das spätere Cookbook sollte detailliert geplant werden.

Mögliche Rezeptzustände:

```text
UNKNOWN
DISCOVERED
LEARNED
MASTERED
```

Ein Rezept könnte anzeigen:

- Name
- Zutaten
- optionale Zutaten
- Kochgerät
- Station
- Kochzeit
- Skill Requirement
- Perk Requirement
- Qualität
- Buffs
- Entdeckungsstatus

---

# 21. Recipe Locks mit Terje

Bestimmte Rezepte könnten Skill- oder Perk-Anforderungen erhalten.

Beispiele:

```text
Simple Sausage
→ frei verfügbar
```

```text
Hunter Sausage
→ Survival Level 10
```

```text
Advanced Herbal Mix
→ Herbalist Stage 2
```

```text
Premium Hunter Meal
→ Survival Level 20
+ Field Cook Stage 2
```

Dabei sollte weiterhin entschieden werden, wie hart die Locks sind.

Mögliche Varianten:

- Rezept komplett gesperrt
- schlechtere Qualität ohne Perk
- längere Kochzeit ohne Perk
- geringere Ausbeute ohne Perk

---

# 22. Anti-Exploit-System

Vor der Umsetzung sollten Exploit-Risiken definiert werden.

Wichtige Regeln:

- keine XP durch Einlegen/Entfernen von Items
- XP nur bei erfolgreichem Abschluss
- keine Recycling-Loops
- keine kostenlosen Portions-Loops
- keine Wurst-Hunting-XP
- keine Food-Metabolism-Doppel-XP
- Batch-XP begrenzen
- Crafting-Cooldowns bei Bedarf
- keine unendliche Ingredient Conversion

---

# 23. Server-Config-Design

ChefZ sollte möglichst viele Werte konfigurierbar machen.

Beispiele:

```text
EnableHerbalist
HerbalistHighlightRange
HerbalistYieldMultiplier

CookingXpMultiplier
ProcessingXpMultiplier

SaltYield
SaltBoilingTime

SmokingTime
DryingTime

SpoilageMultiplier

EnableTerjeSkills
EnableTerjeMedicine

EnableRecipeLocks
EnableFoodBuffs
```

---

# 24. ChefZ-Core Event API

Die Event-API sollte vor der Implementierung festgelegt werden.

Empfohlene Events:

```text
OnIngredientHarvested
OnIngredientProcessed
OnRecipeStarted
OnRecipeCompleted
OnFoodConsumed
OnFoodSpoiled
OnPortionTaken
OnFoodPreserved
OnHerbCollected
```

Dadurch können später andere Systeme anbinden:

- Terje
- DME Quests
- Achievements
- Trader
- Events
- Statistik
- Cookbook

---

# 25. 3D-Asset-Liste und Produktionsplan

Es sollte eine vollständige Asset-Liste entstehen.

Unterscheidung:

## Unique Geometry

Modelle, die wirklich eigene Geometrie benötigen.

Beispiele:

- Fleischwolf
- Mörser
- Räucherschrank
- Drying Rack
- Getreidemühle
- spezielle Tellergerichte

## Shared Geometry + Texture Variants

Beispiele:

- verschiedene Würste
- Gewürzdosen
- Kräuterbündel
- Teller
- Schüsseln

Beispiel:

```text
3 Wurst-Meshes
×
8 Texturen
=
24 optisch unterschiedliche Würste
```

Dadurch kann die Asset-Anzahl reduziert werden.

---

# 26. Empfohlener nächster Planungsschritt

Der wichtigste nächste Schritt ist eine vollständige:

# ChefZ V1 Ingredient & Production Map

Sie sollte alle Produktionsketten abbilden.

Beispiele:

## Getreide

```text
Wheat
↓
Flour
↓
Dough
↓
Bread / Pasta
```

## Fleisch

```text
Animal
↓
Meat
↓
Minced Meat
↓
Sausage
↓
Smoked Sausage
↓
Dish
```

## Salz

```text
Saltwater
↓
Raw Salt
↓
Salt
↓
Curing / Sausage / Cooking
```

## Kräuter

```text
Herb Plant
↓
Fresh Herb
↓
Drying
↓
Dried Herb
↓
Grinding / Tea / Seasoning
```

## Milch

```text
Milk
↓
Cream
↓
Butter / Cheese
```

Diese Map sollte anschließend klar zeigen:

- welche Items benötigt werden
- welche 3D-Modelle benötigt werden
- welche Stationen benötigt werden
- welche Rezepte benötigt werden
- welche Scripts benötigt werden
- welche Module benötigt werden
- welche Terje-Hooks benötigt werden

---

# Priorität

Empfohlene Reihenfolge:

```text
1. Ingredient & Production Map
2. Item State System
3. Farming & Herbs
4. Processing Stations
5. Preservation
6. Cooking Quality
7. Portions
8. Terje Perks
9. Medicine Compatibility
10. Economy / Trader
11. Cookbook UI
12. Asset Production
```

Damit wird ChefZ von einem Konzept zu einem vollständig planbaren Entwicklungsprojekt.
