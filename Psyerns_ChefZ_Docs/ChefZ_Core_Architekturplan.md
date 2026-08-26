# ChefZ Core – Architekturplan

## 1. Ziel des Core-Moduls

**ChefZ_Core** bildet die technische Grundlage des gesamten ChefZ-Systems.

Der Core soll **keine konkreten Gerichte oder Pflanzen enthalten**, sondern ausschließlich die allgemeinen Systeme bereitstellen, auf denen spätere Module aufbauen.

Die Grundidee:

> Der Core kennt nicht „Survivor Spaghetti“ oder „Hunter Sausage“, sondern nur, was ein Rezept, eine Zutat, eine Kategorie, ein Verarbeitungsschritt, ein Behälter und ein Ergebnis ist.

Dadurch bleibt ChefZ langfristig modular und erweiterbar.

---

# 2. Hauptaufgaben von ChefZ_Core

ChefZ_Core übernimmt:

- Recipe Engine
- Ingredient Category System
- Processing System
- Processing Station System
- Ingredient State System
- Quality System
- Nutrition Manager
- Buff-/Effect-Schnittstelle
- Preservation Manager
- Tool Requirement System
- Cooking Device Adapter
- Flexible Ingredient Matching
- Recipe Priority
- Portion System
- Dish Container System
- Empty Container Return
- Event System
- Logging / Debugging
- Config Manager

---

# 3. Recipe Engine

Die Recipe Engine ist das wichtigste System im Core.

Sie prüft:

- welches Kochgerät verwendet wird
- welche Zutaten vorhanden sind
- wie viele Zutaten vorhanden sind
- welche Mengen vorhanden sind
- welchen Zustand Zutaten besitzen
- ob Flüssigkeit benötigt wird
- welche Temperatur benötigt wird
- welche optionalen Zutaten vorhanden sind
- welche Gewürze vorhanden sind
- ob ein bestimmter Verarbeitungsschritt vorausgesetzt wird
- ob alternative Zutaten erlaubt sind
- welches Rezept bei mehreren Treffern Priorität besitzt

Beispiel:

```text
Cooking Pot
│
├── Noodles
├── Sausage
├── Tomato Sauce
└── Salt

→ ChefZ_SausagePasta
```

Wenn kein ChefZ-Rezept passt:

```text
ChefZ Recipe Check
↓
No Match
↓
Vanilla DayZ Cooking
```

ChefZ soll Vanilla-Cooking niemals unnötig blockieren.

---

# 4. JSON-basierte Rezeptdefinition

Rezepte sollten möglichst konfigurierbar sein.

Beispiel:

```json
{
  "RecipeID": "ChefZ_SausagePasta",
  "CookingDevice": [
    "CookingPot",
    "FryingPan"
  ],
  "Ingredients": [
    {
      "Category": "PASTA",
      "Amount": 1
    },
    {
      "Category": "SAUSAGE",
      "Amount": 1
    },
    {
      "Item": "ChefZ_TomatoSauce",
      "Amount": 1
    }
  ],
  "OptionalIngredients": [
    "ChefZ_BlackPepper",
    "ChefZ_Parsley"
  ],
  "Result": "ChefZ_SausagePasta"
}
```

Vorteil:

Neue Rezepte können später hinzugefügt werden, ohne die Core-Logik zu verändern.

---

# 5. Ingredient Category System

Der Core verwaltet allgemeine Zutatenkategorien.

Beispiele:

```text
MEAT
WILD_MEAT
DOMESTIC_MEAT
POULTRY
FISH
SAUSAGE
VEGETABLE
MUSHROOM
PASTA
GRAIN
DAIRY
HERB
SPICE
FAT
LIQUID
```

Beispiel:

```text
ChefZ_PorkSausage
ChefZ_VenisonSausage
ChefZ_BoarSausage
ChefZ_HunterSausage
```

gehören alle zur Kategorie:

```text
SAUSAGE
```

Ein Rezept kann dadurch verlangen:

```text
1x SAUSAGE
```

statt jede einzelne Wurstsorte separat zu definieren.

Unterkategorien sind möglich:

```text
MEAT
├── DOMESTIC_MEAT
├── WILD_MEAT
├── POULTRY
└── PREDATOR_MEAT
```

---

# 6. Processing System

Der Core soll Verarbeitungsschritte generisch behandeln.

Beispiele:

```text
PROCESS_CUT
PROCESS_DRY
PROCESS_GRIND
PROCESS_MILL
PROCESS_SMOKE
PROCESS_SALT
PROCESS_BOIL
PROCESS_BAKE
PROCESS_FRY
PROCESS_FERMENT
PROCESS_PICKLE
```

Beispiele:

```text
ChefZ_PepperBerries
↓ DRY
ChefZ_DriedPeppercorns
↓ GRIND
ChefZ_BlackPepper
```

```text
ChefZ_Wheat
↓ MILL
ChefZ_Flour
```

```text
Raw Meat
↓ GRIND
ChefZ_MincedMeat
```

---

# 7. Processing Stations

Verarbeitungsstationen definieren nur, welche Prozesse sie unterstützen.

## Fleischwolf

```text
ChefZ_MeatGrinder

Supports:
- GRIND_MEAT
```

## Mörser

```text
ChefZ_Mortar

Supports:
- GRIND_SPICES
- GRIND_HERBS
```

## Trockenrahmen

```text
ChefZ_DryingRack

Supports:
- DRY_MEAT
- DRY_FISH
- DRY_HERBS
- DRY_PASTA
- DRY_PAPRIKA
```

## Räucherschrank

```text
ChefZ_Smoker

Supports:
- SMOKE_MEAT
- SMOKE_FISH
- SMOKE_SAUSAGE
```

## Getreidemühle

```text
ChefZ_GrainMill

Supports:
- MILL_GRAIN
```

---

# 8. Ingredient State System

ChefZ-Lebensmittel können verschiedene Zustände besitzen.

Beispiele:

```text
RAW
PREPARED
COOKED
DRIED
SMOKED
SALTED
PICKLED
FERMENTED
BURNT
ROTTEN
```

Beispiel:

```text
Venison
State = RAW
```

danach:

```text
Venison
State = SALTED
```

später:

```text
Venison
State = DRIED
```

Auch wenn DayZ technisch für bestimmte Zustände eigene Klassen benötigt, sollte ChefZ intern möglichst ein einheitliches Zustandssystem nutzen.

---

# 9. Quality System

Der Core verwaltet Qualitätsstufen für fertige Gerichte.

Vorgeschlagene Stufen:

```text
SIMPLE
PREPARED
SEASONED
PREMIUM
```

Beispiel:

```text
Noodles
+ Sausage

→ SIMPLE
```

```text
Noodles
+ Sausage
+ Tomato Sauce
+ Salt

→ PREPARED
```

```text
+ Pepper
+ Parsley

→ SEASONED
```

```text
+ Hunter Sausage
+ Mushroom Sauce
+ Fresh Herbs

→ PREMIUM
```

Die Qualität kann Einfluss haben auf:

- Nährwerte
- Haltbarkeit
- Buffs
- Handelswert
- Beschreibung
- Darstellung im UI

---

# 10. Nutrition Manager

Der Nutrition Manager berechnet oder verwaltet die Nährwerte fertiger Gerichte.

Mögliche Werte:

- Energy
- Water
- Stomach Filling
- Temperature
- Health
- eventuell zusätzliche ChefZ-Werte

Beispiel:

```text
Sausage       450
Pasta         500
Tomato Sauce  100
----------------
Base         1050
```

Danach:

```text
Recipe Modifier = 1.10
Quality Bonus   = +X %
```

Dadurch können Balancing-Werte zentral angepasst werden.

---

# 11. Buff / Effect Interface

Der Core sollte Buffs nicht zwingend vollständig implementieren, aber Schnittstellen dafür bereitstellen.

Mögliche Effekt-IDs:

```text
CHEFZ_WARM_MEAL
CHEFZ_HYDRATED
CHEFZ_ENERGIZED
CHEFZ_HEALTHY_MEAL
CHEFZ_HEARTY_MEAL
CHEFZ_HUNTERS_MEAL
```

Ein Rezept kann beispielsweise definieren:

```text
Effects:
[
    "CHEFZ_WARM_MEAL",
    "CHEFZ_HEARTY_MEAL"
]
```

Ein separates Effekt-Modul kann diese IDs später auswerten.

---

# 12. Preservation Manager

Der Preservation Manager steuert Haltbarkeit und Verderb.

Beispiel:

```text
RAW_MEAT
SpoilageMultiplier = 1.0
```

```text
SALTED_MEAT
SpoilageMultiplier = 0.5
```

```text
SMOKED_MEAT
SpoilageMultiplier = 0.25
```

```text
DRIED_MEAT
SpoilageMultiplier = 0.15
```

```text
CANNED_FOOD
SpoilageMultiplier = 0.02
```

So müssen Haltbarkeitsregeln nicht für jedes Item neu programmiert werden.

---

# 13. Tool Requirement System

Rezepte und Verarbeitungen sollen Tool-Kategorien verlangen können.

Beispiele:

```text
CUTTING_TOOL
AXE
GRINDER
MORTAR
COOKING_POT
PAN
OVEN
SMOKER
DRYING_RACK
```

Dadurch kann ein Rezept verlangen:

```text
CUTTING_TOOL
```

und mehrere konkrete Gegenstände akzeptieren:

- KitchenKnife
- HuntingKnife
- SteakKnife

---

# 14. Cooking Device Adapter

ChefZ_Core soll sich in bestehende DayZ-Kochgeräte einklinken.

Primär:

- CookingPot
- FryingPan
- Cauldron

Ablauf:

```text
Cooking Starts
↓
ChefZ Checks Ingredients
↓
Valid ChefZ Recipe?
```

### Ja

ChefZ übernimmt die Verarbeitung.

### Nein

```text
Continue Vanilla DayZ Cooking
```

Das ist eine zentrale Designregel.

---

# 15. Flexible Ingredient Matching

Der Recipe Matcher soll mehrere Matching-Arten unterstützen.

## Exact

```text
ChefZ_HunterSausage
```

## Category

```text
SAUSAGE
```

## AnyOf

```text
Potato
OR
Rice
OR
Pasta
```

## Optional

```text
Parsley
```

## Required Amount

```text
2x Potato
```

## Quantity

Beispiel:

```text
100g Flour
```

---

# 16. Recipe Priority

Mehrere Rezepte können gleichzeitig gültig sein.

Beispiel:

```text
Meat
Potato
Tomato
Mushroom
Thyme
```

könnte passen auf:

```text
Meat Stew
Hunter Stew
Forest Stew
Premium Hunter Stew
```

Darum benötigt jedes Rezept eine Priorität.

Beispiel:

```text
Priority = 100
```

Grundregel:

> Das spezifischste gültige Rezept gewinnt.

---

# 17. Portion System

Der Core soll große Mengen und Gruppenrezepte unterstützen.

Beispiel:

```text
Cauldron

4x Meat
4x Potato
2x Tomato
Water
Herbs
```

Ergebnis:

```text
ChefZ_HunterStew
Portions = 8
```

Interaktion:

```text
Take Portion
```

Damit lassen sich große Gruppengerichte sauber umsetzen.

---

# 18. Dish Container System

ChefZ-Gerichte können bestimmte Behälter benötigen.

Kategorien:

```text
PLATE
BOWL
CAN
JAR
BOX
```

Beispiele:

```text
Hunter Stew
requires:
BOWL
```

```text
Survivor Spaghetti
requires:
PLATE
```

```text
Pickled Vegetables
requires:
JAR
```

```text
Canned Hunter Stew
requires:
CAN
```

---

# 19. Empty Container Return

Nach dem Verzehr kann ein leerer Behälter zurückgegeben werden.

Beispiel:

```text
ChefZ_SurvivorSpaghetti
↓ consume
ChefZ_EmptyPlate
```

```text
ChefZ_CannedHunterStew
↓ consume
ChefZ_EmptyCan
```

Generische Rezept-/Itemoption:

```text
ReturnContainer = "ChefZ_EmptyPlate"
```

---

# 20. ChefZ Event System

Der Core sollte interne Events bereitstellen.

Mögliche Events:

```text
OnChefZRecipeStarted
OnChefZRecipeCompleted
OnChefZIngredientProcessed
OnChefZFoodConsumed
OnChefZRecipeDiscovered
OnChefZFoodSpoiled
```

Dadurch können andere DME-Systeme später reagieren.

Beispiel:

```text
OnChefZRecipeCompleted
↓
Quest System
↓
Cook 10 Meals
```

Oder:

```text
OnChefZRecipeDiscovered
↓
Achievement System
```

---

# 21. Logging / Debugging

ChefZ_Core benötigt ausführliches Debug-Logging.

Beispiel:

```text
[ChefZ] Recipe check started
[ChefZ] Device: CookingPot
[ChefZ] Found ingredient: ChefZ_RawPasta
[ChefZ] Found ingredient: ChefZ_PorkSausage
[ChefZ] Recipe match: ChefZ_SausagePasta
[ChefZ] Quality: SEASONED
[ChefZ] Result created
```

Config:

```json
{
    "Debug": 1
}
```

Bei vielen Rezepten ist gutes Logging zwingend notwendig.

---

# 22. Config Manager

Der Core lädt und verwaltet alle ChefZ-Konfigurationen.

Empfohlene Struktur:

```text
profiles/
└── ChefZ/
    ├── Core.json
    ├── Ingredients.json
    ├── Categories.json
    ├── Processing.json
    ├── Recipes/
    │   ├── Pasta.json
    │   ├── Meat.json
    │   ├── Fish.json
    │   ├── Breakfast.json
    │   ├── Stews.json
    │   └── Special.json
    ├── Nutrition.json
    └── Preservation.json
```

---

# 23. ChefZ_Core interne Struktur

Empfohlene Architektur:

```text
ChefZ_Core
│
├── Recipe Engine
├── Ingredient Manager
├── Category Manager
├── Processing Manager
├── Nutrition Manager
├── Preservation Manager
├── Quality Manager
├── Container Manager
├── Config Manager
└── Event Manager
```

---

# 24. Module auf Basis von ChefZ_Core

## ChefZ_Farming

Enthält:

- Kräuter
- Weizen
- Pfeffer
- Paprika
- Gemüse
- Pflanzenlogik

---

## ChefZ_Processing

Enthält:

- Fleischwolf
- Getreidemühle
- Mörser
- Schneidebrett
- Teig
- Pasta
- Zwischenprodukte

---

## ChefZ_Meat

Enthält:

- Hackfleisch
- Wurst
- Wurstsorten
- Pökeln
- Fleischverarbeitung

---

## ChefZ_Preservation

Enthält:

- Räuchern
- Trocknen
- Einkochen
- Einlegen
- Konservierung

---

## ChefZ_Cooking

Enthält:

- Tellergerichte
- Suppen
- Eintöpfe
- Pasta
- Frühstück
- Fischgerichte

---

## ChefZ_UI

Enthält später:

- Cookbook
- Rezeptanzeige
- Fortschritt
- Rezeptstatus
- Raritäten

---

# 25. Was NICHT in ChefZ_Core gehört

Der Core soll strikt generisch bleiben.

Nicht enthalten:

- Survivor Spaghetti
- Hunter Stew
- einzelne Wurstrezepte
- einzelne Kräuterpflanzen
- einzelne Teller-Modelle
- einzelne Pflanzenmodelle
- konkrete DME Signature Meals

Der Core kennt nur:

> Was ist ein Rezept?

> Was ist eine Zutat?

> Was ist eine Zutatenkategorie?

> Was ist ein Verarbeitungsschritt?

> Welche Station kann diesen Prozess ausführen?

> Welches Rezept passt?

> Welches Ergebnis wird erzeugt?

---

# 26. Gesamtarchitektur

```text
                    CHEFZ CORE
                         │
        ┌────────────────┼────────────────┐
        │                │                │
     FARMING         PROCESSING        COOKING
        │                │                │
     Plants          Ingredients        Recipes
        │                │                │
        └─────────────── FOOD ────────────┘
                         │
                   PRESERVATION
                         │
                 DRY / SMOKE / CAN
                         │
                      SERVING
```

---

# 27. Entwicklungsreihenfolge

Empfohlene Reihenfolge:

## Phase 1 – Core Foundation

- Config Manager
- Category Manager
- Ingredient Manager
- Recipe Engine
- Logging

## Phase 2 – Recipe Matching

- Exact Matching
- Category Matching
- AnyOf
- Optional Ingredients
- Mengen
- Prioritäten

## Phase 3 – Processing

- Processing Manager
- Tool Requirements
- Processing Stations
- Item States

## Phase 4 – Food Systems

- Nutrition Manager
- Quality Manager
- Preservation Manager

## Phase 5 – Serving

- Portion System
- Plate / Bowl / Can / Jar Support
- Empty Container Return

## Phase 6 – Integration

- Events
- Quest Hooks
- Cookbook Hooks
- spätere DME-Systemintegration

---

# 28. Zentrale Designregel

ChefZ_Core soll möglichst selten geändert werden müssen.

Neue Inhalte sollen hauptsächlich über:

- JSON-Konfigurationen
- neue Items
- neue Pflanzen
- neue Stationen
- neue Rezepte
- neue 3D-Modelle

hinzugefügt werden.

Das Ziel ist:

```text
NEW CONTENT
↓
CONFIG / MODULE
↓
CHEFZ CORE
```

und nicht:

```text
NEW CONTENT
↓
CHANGE CORE CODE
↓
RISK BREAKING EVERYTHING
```

---

# 29. Langfristiges Ziel

ChefZ_Core soll die stabile technische API-Schicht des gesamten ChefZ-Projekts bilden.

Dadurch können später unabhängig voneinander erweitert werden:

- Landwirtschaft
- Jagd
- Wurstproduktion
- Fischverarbeitung
- Backen
- Pasta
- Kräuter
- Gewürze
- Räuchern
- Trocknen
- Einkochen
- Rezeptprogression
- DME-Quests
- Events
- Achievements
- Händler
- saisonale Rezepte

Die wichtigste Architekturentscheidung lautet daher:

> **ChefZ_Core enthält Systeme – ChefZ-Module enthalten Content.**
