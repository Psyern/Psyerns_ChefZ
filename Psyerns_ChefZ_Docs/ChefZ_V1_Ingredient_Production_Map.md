# ChefZ V1 – Ingredient & Production Map

## 1. Ziel

Dieses Dokument definiert die vollständige **ChefZ V1 Ingredient & Production Map**.

Es beschreibt:

- welche Rohstoffe ChefZ V1 enthält
- woher diese Rohstoffe kommen
- welche Zwischenprodukte entstehen
- welche Produktionsketten existieren
- welche Werkzeuge und Stationen benötigt werden
- welche Produkte konserviert werden können
- welche Endgerichte daraus entstehen
- welche ChefZ-Module zuständig sind
- welche Terje-Kompatibilität später angebunden werden kann
- welche 3D-Assets für V1 benötigt werden

Die Map ist bewusst als **V1-Scope** gedacht.

Ziel ist nicht, sofort jedes denkbare Lebensmittel abzubilden, sondern ein geschlossenes System zu bauen, das später modular erweitert werden kann.

---

# 2. ChefZ V1 – Modulübersicht

```text
ChefZ_Core
│
├── Recipe Engine
├── Category System
├── Food Tags
├── Processing System
├── Food State System
├── Quality System
├── Portion System
├── Container System
├── Event API
└── Config Manager

ChefZ_Ingredients
├── Grundzutaten
├── Zwischenprodukte
└── Gewürze

ChefZ_Farming
├── Pflanzen
├── Kräuter
├── Samen
└── Ernte

ChefZ_Processing
├── Schneiden
├── Mahlen
├── Mörsern
├── Fleischwolf
├── Teig
└── Grundverarbeitung

ChefZ_Meat
├── Hackfleisch
├── Wurst
└── Fleischprodukte

ChefZ_Preservation
├── Salzen
├── Trocknen
├── Räuchern
└── Langzeithaltbarkeit

ChefZ_Baking
├── Brot
├── Fladenbrot
├── Nudelteig
└── Pasta

ChefZ_Cooking
├── Tellergerichte
├── Suppen
├── Eintöpfe
└── Frühstück

ChefZ_TerjeSkills
└── optionale Skill-Kompatibilität

ChefZ_TerjeMedicine
└── optionale medizinische Food-Effekte
```

---

# 3. ChefZ V1 – Haupt-Produktionsnetz

```text
                    ┌──────────── WHEAT ─────────────┐
                    │                                │
                    ↓                                ↓
                  FLOUR                           SEEDS
                    │
          ┌─────────┴─────────┐
          ↓                   ↓
        DOUGH            PASTA DOUGH
          │                   │
     ┌────┴────┐              ↓
     ↓         ↓           RAW PASTA
   BREAD    FLATBREAD          │
                               ↓
                         DRIED PASTA
                               │
                               ↓
                         PASTA DISHES


ANIMAL
  │
  ├── MEAT ──→ MINCED MEAT ──→ RAW SAUSAGE
  │                                │
  │                    ┌───────────┼───────────┐
  │                    ↓           ↓           ↓
  │                 COOKED      SMOKED       DRIED
  │                 SAUSAGE     SAUSAGE      SAUSAGE
  │
  ├── FAT ──→ COOKING FAT
  │
  ├── BONES ──→ BONE BROTH
  │
  └── INTESTINES ──→ SAUSAGE CASING


FISH
  │
  └── FILLET
       │
       ├── COOKED FISH
       ├── SALTED FISH ──→ DRIED FISH
       └── SMOKED FISH


SALTWATER
  │
  ↓
RAW SALT
  │
  ↓
SALT
  │
  ├── COOKING
  ├── SAUSAGE
  ├── MEAT CURING
  └── FISH CURING


HERBS
  │
  ├── FRESH HERBS
  │
  └── DRIED HERBS
       │
       ├── TEA
       ├── SEASONING
       └── HERB MIX


PAPRIKA
  │
  ├── FRESH PAPRIKA
  └── DRIED PAPRIKA
       │
       ↓
  PAPRIKA POWDER


PEPPER PLANT
  │
  ↓
PEPPER BERRIES
  │
  ↓
DRIED PEPPERCORNS
  │
  ↓
BLACK PEPPER


MILK
  │
  ├── CREAM
  │     │
  │     ↓
  │   BUTTER
  │
  └── CHEESE


VEGETABLES
  │
  ├── POTATO
  ├── TOMATO
  ├── ONION
  ├── GARLIC
  ├── CARROT
  ├── CABBAGE
  └── PAPRIKA
         │
         ↓
    COOKED DISHES
```

---

# 4. Rohstoff-Kategorien

## 4.1 Pflanzen

V1 enthält:

```text
Wheat
Potato
Tomato
Paprika
Pepper Plant
Onion
Garlic
Carrot
Cabbage
```

---

## 4.2 Kräuter

V1 enthält:

```text
Parsley
Dill
Thyme
Rosemary
Wild Garlic
```

---

## 4.3 Tierische Rohstoffe

```text
Raw Meat
Animal Fat
Bones
Intestines
Fish
Eggs
Milk
```

---

## 4.4 Welt-/Umweltressourcen

```text
Fresh Water
Saltwater
```

---

## 4.5 Bestehende DayZ-/Loot-Rohstoffe

ChefZ kann zusätzlich vorhandene Lebensmittel nutzen.

Beispiele:

```text
Rice
Honey
Baked Beans
Tactical Bacon
Cheese
Powdered Milk
Canned Food
```

Diese werden nicht zwingend durch ChefZ ersetzt.

---

# 5. ChefZ Food Categories

ChefZ_Core sollte folgende Kategorien bereitstellen:

```text
MEAT
WILD_MEAT
DOMESTIC_MEAT
POULTRY
PREDATOR_MEAT

FISH

SAUSAGE

VEGETABLE
ROOT_VEGETABLE
LEAF_VEGETABLE

MUSHROOM

HERB
DRIED_HERB

SPICE

GRAIN
FLOUR
DOUGH
PASTA
BREAD

DAIRY

FAT

LIQUID

BROTH
SAUCE

PLATE_DISH
BOWL_DISH

PRESERVED_FOOD
```

---

# 6. ChefZ Food Tags

Zusätzlich zu Kategorien sollten Items Tags besitzen.

Beispiele:

```text
CHEFZ_HERB
CHEFZ_WILD_HERB
CHEFZ_SPICE
CHEFZ_WILD_MEAT
CHEFZ_RAW_MEAT
CHEFZ_HIGH_PROTEIN
CHEFZ_HOT_MEAL
CHEFZ_FRESH
CHEFZ_DRIED
CHEFZ_SMOKED
CHEFZ_SALTED
CHEFZ_MEDICINAL
CHEFZ_FISH
CHEFZ_DAIRY
CHEFZ_PREMIUM
```

Diese Tags werden später unter anderem für:

- Terje Skills
- Terje Medicine
- Recipe Matching
- Buffs
- Trader
- Quests
- Achievements

verwendet.

---

# 7. Weizen-Produktionskette

## Quelle

```text
ChefZ_WheatPlant
```

Ernte:

```text
ChefZ_Wheat
ChefZ_WheatSeeds
```

---

## Verarbeitung

```text
ChefZ_Wheat
+ ChefZ_GrainMill
→ ChefZ_Flour
```

Möglicher Output:

```text
1000 g Wheat
→ 700–850 g Flour
```

Balancingwert später konfigurierbar.

---

## Modul

```text
ChefZ_Farming
ChefZ_Processing
ChefZ_Ingredients
```

---

# 8. Mehl

Klasse:

```text
ChefZ_Flour
```

Kategorie:

```text
FLOUR
GRAIN
```

Verwendung:

- Brot
- Fladenbrot
- Nudelteig
- Kartoffelpuffer
- Teigtaschen
- eventuell Blutwurst

---

# 9. Hefe

Klasse:

```text
ChefZ_Yeast
```

Quelle:

V1 zunächst bevorzugt:

- Loot
- Küche
- Supermarkt
- Bäckerei
- Farmversorgung

Noch keine komplexe Hefekultur für V1.

Verwendung:

```text
Flour
+ Water
+ Yeast
→ Yeast Dough
```

---

# 10. Teig

## Einfacher Teig

```text
ChefZ_Flour
+ Water
→ ChefZ_SimpleDough
```

Verwendung:

- Fladenbrot
- einfache Teigtaschen

---

## Hefeteig

```text
ChefZ_Flour
+ Water
+ ChefZ_Yeast
→ ChefZ_YeastDough
```

Verwendung:

- Brot
- Brötchen später
- hochwertige Backwaren später

---

## Nudelteig

```text
ChefZ_Flour
+ Egg
+ Water
→ ChefZ_PastaDough
```

Alternativ einfaches Survival-Rezept:

```text
ChefZ_Flour
+ Water
→ ChefZ_PastaDough
```

mit geringerer Qualität.

---

# 11. Pasta-Produktionskette

```text
ChefZ_PastaDough
+ ChefZ_RollingPin / Cutting Tool
→ ChefZ_RawPasta
```

Optional:

```text
ChefZ_RawPasta
+ ChefZ_DryingRack
→ ChefZ_DriedPasta
```

Eigenschaften:

## Raw Pasta

- kurze Haltbarkeit
- direkt kochbar

## Dried Pasta

- lange Haltbarkeit
- idealer Vorratsartikel
- geringerer Wasseranteil

---

# 12. Brot-Produktionskette

```text
ChefZ_YeastDough
+ Oven
→ ChefZ_Bread
```

Alternativ:

```text
ChefZ_SimpleDough
+ Pan / Hot Surface
→ ChefZ_Flatbread
```

---

# 13. Kartoffeln

Klasse:

```text
Potato
```

Verarbeitung:

```text
Potato
+ Knife
→ ChefZ_SlicedPotato
```

Verwendung:

- Bratkartoffeln
- Bauernfrühstück
- Jägerteller
- Fisch mit Kartoffeln
- Kartoffelpuffer
- Suppen
- Eintöpfe

---

# 14. Tomaten

Klasse:

```text
Tomato
```

Verarbeitung:

```text
Tomato
+ Knife
→ ChefZ_ChoppedTomato
```

Weitere Verarbeitung:

```text
ChefZ_ChoppedTomato
+ Cooking Pot
→ ChefZ_TomatoSauce
```

Verwendung:

- Pasta
- Chili
- Suppen
- Eintöpfe
- Soßen

---

# 15. Paprika

Klasse:

```text
ChefZ_Paprika
```

Direkte Verwendung:

```text
ChefZ_Paprika
→ Fresh Food / Cooking Ingredient
```

Verarbeitung:

```text
ChefZ_Paprika
+ Knife
→ ChefZ_ChoppedPaprika
```

Konservierung:

```text
ChefZ_Paprika
+ Drying Rack
→ ChefZ_DriedPaprika
```

Danach:

```text
ChefZ_DriedPaprika
+ Mortar
→ ChefZ_PaprikaPowder
```

---

# 16. Pfeffer

## Pflanze

```text
ChefZ_PepperPlant
```

Ernte:

```text
ChefZ_PepperBerries
```

Verarbeitung:

```text
ChefZ_PepperBerries
+ Drying Rack
→ ChefZ_DriedPeppercorns
```

Danach:

```text
ChefZ_DriedPeppercorns
+ Mortar
→ ChefZ_BlackPepper
```

Pfeffer sollte selten sein.

Mögliche Fundorte:

- Gewächshäuser
- Gärtnereien
- besondere Farmen

---

# 17. Zwiebeln

Klasse:

```text
ChefZ_Onion
```

Verarbeitung:

```text
ChefZ_Onion
+ Knife
→ ChefZ_ChoppedOnion
```

Verwendung:

- Eintöpfe
- Wurst
- Bauernfrühstück
- Blutwurst
- Soßen
- Kartoffelgerichte

---

# 18. Knoblauch

Klasse:

```text
ChefZ_Garlic
```

Verarbeitung:

```text
ChefZ_Garlic
+ Knife
→ ChefZ_ChoppedGarlic
```

Verwendung:

- Wurst
- Fleisch
- Suppen
- Marinaden
- Gewürzmischungen

---

# 19. Karotten

Klasse:

```text
ChefZ_Carrot
```

Verarbeitung:

```text
ChefZ_Carrot
+ Knife
→ ChefZ_ChoppedCarrot
```

Verwendung:

- Suppen
- Eintöpfe
- Brühe
- Gemüsegerichte

---

# 20. Kohl

Klasse:

```text
ChefZ_Cabbage
```

Verarbeitung:

```text
ChefZ_Cabbage
+ Knife
→ ChefZ_ChoppedCabbage
```

V1 Verwendung:

- Eintopf
- Suppe
- Gemüsegerichte

Fermentation / Sauerkraut:

**späteres Modul / V2**, um V1 nicht unnötig aufzublähen.

---

# 21. Kräuter – Übersicht

| Kraut | Seltenheit | Umgebung | Hauptverwendung |
|---|---|---|---|
| Petersilie | häufig | Gärten, Dörfer | Pasta, Kartoffeln, Suppen |
| Dill | häufig-mittel | Gewässer, feuchte Wiesen | Fisch, Kartoffeln |
| Thymian | mittel | Wiesen, Waldränder | Wild, Wurst, Eintopf |
| Rosmarin | selten | Gärten, Gewächshäuser | Fleisch, Kartoffeln, Brot |
| Bärlauch | mittel | feuchte Wälder | Wurst, Fleisch, Tee |

---

# 22. Kräuter-Produktionskette

Für alle fünf Kräuter:

```text
Herb Plant
↓
Fresh Herb
```

Danach:

```text
Fresh Herb
+ Drying Rack
→ Dried Herb
```

Beispiel:

```text
ChefZ_Thyme
→ ChefZ_DriedThyme
```

---

# 23. Kräutermischung

```text
Dried Thyme
+ Dried Parsley
+ Dried Rosemary
+ Mortar
→ ChefZ_HerbMix
```

Mögliche spätere Varianten:

```text
ChefZ_MeatSeasoning
ChefZ_FishSeasoning
ChefZ_HunterSeasoning
```

Für V1 reicht zunächst:

```text
ChefZ_HerbMix
ChefZ_HunterSeasoning
```

---

# 24. Hunter Seasoning

Beispiel:

```text
Salt
+ Black Pepper
+ Paprika Powder
+ Dried Thyme
+ Dried Wild Garlic
→ ChefZ_HunterSeasoning
```

Verwendung:

- Hunter Sausage
- Hunter Stew
- Wildgerichte

---

# 25. Salz-Produktionskette

Quelle:

```text
Saltwater
```

Verarbeitung:

```text
Saltwater
+ Cooking Pot
+ Heat
→ ChefZ_RawSalt
```

Danach:

```text
ChefZ_RawSalt
+ Drying
→ ChefZ_Salt
```

---

# 26. Salz – Verwendung

```text
ChefZ_Salt
```

wird benötigt für:

- Teig
- Brot
- Pasta
- Wurst
- Fleisch
- Fisch
- Suppen
- Eintöpfe
- Gewürzmischungen
- Konservierung

---

# 27. Tierzerlegung – Übergabepunkt zwischen Terje und ChefZ

Tierzerlegung selbst sollte primär über Vanilla/Terje laufen.

Output für ChefZ:

```text
Meat
Animal Fat
Bones
Intestines
```

ChefZ übernimmt ab diesem Punkt.

---

# 28. Fleischkategorien

ChefZ sollte Vanilla- und Mod-Fleisch in Kategorien einteilen.

```text
DOMESTIC_MEAT
WILD_MEAT
POULTRY
PREDATOR_MEAT
```

Beispiele:

## Domestic

- Pig
- Cow
- Sheep
- Goat

## Wild

- Deer
- Roe Deer
- Wild Boar

## Poultry

- Chicken

## Predator

- Wolf
- Bear

---

# 29. Fleisch schneiden

```text
Raw Meat
+ Knife
→ ChefZ_DicedMeat
```

Kategorie:

```text
MEAT
PREPARED
```

Verwendung:

- Eintöpfe
- Pasta
- Suppen
- Gulasch

---

# 30. Hackfleisch

```text
Raw Meat
+ ChefZ_MeatGrinder
→ ChefZ_MincedMeat
```

Das Ausgangsfleisch sollte intern gespeichert bzw. klassifiziert werden.

Beispiele:

```text
ChefZ_MincedPork
ChefZ_MincedVenison
ChefZ_MincedBoar
ChefZ_MincedBear
ChefZ_MincedChicken
```

Alternativ technisch ein Item mit Meat-Type-Variable.

Entscheidung später anhand der DayZ-Implementation.

---

# 31. Tierfett

Output:

```text
AnimalFat
```

ChefZ-Verwendung:

```text
Animal Fat
→ Cooking Fat
```

oder direkt als Kochfett verwendbar.

Anwendungen:

- Wurst
- Bratkartoffeln
- Pfannengerichte
- Fleischgerichte

---

# 32. Knochen

```text
Bones
+ Water
+ Herbs
+ Cooking Pot
→ ChefZ_BoneBroth
```

Verwendung:

- Suppen
- Eintöpfe
- hochwertige Gerichte

---

# 33. Wursthüllen

Quelle:

```text
Intestines
```

Verarbeitung:

```text
Intestines
+ Water
+ Knife / Processing
→ ChefZ_SausageCasing
```

Optional Hygiene-System später.

---

# 34. Basis-Wurst

```text
ChefZ_MincedMeat
+ ChefZ_Salt
+ ChefZ_SausageCasing
→ ChefZ_RawSausage
```

Kategorie:

```text
SAUSAGE
RAW
```

---

# 35. Pork Sausage

```text
Minced Pork
+ Salt
+ Black Pepper
+ Sausage Casing
→ ChefZ_RawPorkSausage
```

---

# 36. Venison Sausage

```text
Minced Venison
+ Salt
+ Thyme
+ Black Pepper
+ Sausage Casing
→ ChefZ_RawVenisonSausage
```

---

# 37. Boar Sausage

```text
Minced Boar
+ Salt
+ Black Pepper
+ Wild Garlic
+ Sausage Casing
→ ChefZ_RawBoarSausage
```

---

# 38. Hunter Sausage

```text
Wild Meat
+ Salt
+ Hunter Seasoning
+ Sausage Casing
→ ChefZ_RawHunterSausage
```

---

# 39. Spicy Sausage

```text
Minced Meat
+ Salt
+ Black Pepper
+ Paprika Powder
+ Sausage Casing
→ ChefZ_RawSpicySausage
```

---

# 40. Wurst-Zustände

Jede rohe Wurst kann weiterverarbeitet werden.

```text
RAW SAUSAGE
│
├── Fry / Cook
│   ↓
│ COOKED SAUSAGE
│
├── Smoker
│   ↓
│ SMOKED SAUSAGE
│
└── Drying Rack
    ↓
  DRY SAUSAGE
```

---

# 41. Räucherwurst

```text
Raw Sausage
+ Smoker
+ Fuel
→ Smoked Sausage
```

Eigenschaften:

- lange Haltbarkeit
- guter Reiseproviant
- höherer Handelswert

---

# 42. Trockenwurst

```text
Raw Sausage
+ Drying Rack
→ Dry Sausage
```

Eigenschaften:

- sehr lange Haltbarkeit
- geringer Wasseranteil
- ideal für Reisen

---

# 43. Gesalzenes Fleisch

```text
Raw Meat
+ Salt
→ ChefZ_SaltedMeat
```

Danach:

```text
ChefZ_SaltedMeat
+ Drying Rack
→ ChefZ_DriedMeat
```

oder:

```text
ChefZ_SaltedMeat
+ Smoker
→ ChefZ_SmokedMeat
```

---

# 44. Fisch-Produktionskette

```text
Fish
+ Knife
→ Fish Fillet
```

Terje Fishing sollte weiterhin das Filetieren und entsprechende XP beeinflussen.

ChefZ übernimmt danach.

---

# 45. Gesalzener Fisch

```text
Fish Fillet
+ Salt
→ ChefZ_SaltedFish
```

Danach:

```text
ChefZ_SaltedFish
+ Drying Rack
→ ChefZ_DriedFish
```

---

# 46. Räucherfisch

```text
Fish Fillet
+ Smoker
→ ChefZ_SmokedFish
```

---

# 47. Milch

Klasse:

```text
ChefZ_Milk
```

Quelle V1:

- Loot
- Farm-/Kühlungs-Spawns
- eventuell vorhandene Milchitems

Tiermelken ist **nicht zwingend V1**.

---

# 48. Sahne

```text
ChefZ_Milk
→ ChefZ_Cream
```

Station oder Verarbeitung:

```text
ChefZ_DairyProcessor
```

Für V1 kann das vereinfachte Processing genutzt werden.

---

# 49. Butter

```text
ChefZ_Cream
+ ChefZ_ButterChurn
→ ChefZ_Butter
```

V1 kann Butterfass optional sein.

Wenn Asset-Aufwand reduziert werden soll:

```text
Cream + Processing Action
→ Butter
```

---

# 50. Käse

```text
ChefZ_Milk
+ Cheese Processing
→ ChefZ_Cheese
```

Für V1 nur eine Käseklasse.

Mehrere Käsesorten erst später.

---

# 51. Eier

Klasse:

```text
ChefZ_Egg
```

Quelle:

- Hühner
- Nester
- Farmen
- Loot

Verwendung:

- Rührei
- Bauernfrühstück
- Nudelteig
- Kartoffelpuffer

---

# 52. Tomatensoße

```text
Chopped Tomato
+ Salt
+ Cooking Pot
→ ChefZ_TomatoSauce
```

Optional:

```text
+ Garlic
+ Herbs
```

erhöht Dish Quality.

---

# 53. Rahmsoße

```text
Cream
+ Butter
+ Salt
→ ChefZ_CreamSauce
```

---

# 54. Pilzrahmsoße

```text
Mushrooms
+ Cream
+ Parsley
+ Salt
→ ChefZ_MushroomCreamSauce
```

---

# 55. Bone Broth

```text
Bones
+ Water
+ Onion
+ Carrot
+ Herbs
→ ChefZ_BoneBroth
```

Premium-Basis für:

- Suppen
- Eintöpfe
- Wildgerichte

---

# 56. V1 Preservation Matrix

| Input | Prozess | Output |
|---|---|---|
| Fresh Herb | Dry | Dried Herb |
| Pepper Berries | Dry | Dried Peppercorns |
| Paprika | Dry | Dried Paprika |
| Raw Pasta | Dry | Dried Pasta |
| Raw Meat | Salt | Salted Meat |
| Salted Meat | Dry | Dried Meat |
| Salted Meat | Smoke | Smoked Meat |
| Fish Fillet | Salt | Salted Fish |
| Salted Fish | Dry | Dried Fish |
| Fish Fillet | Smoke | Smoked Fish |
| Raw Sausage | Smoke | Smoked Sausage |
| Raw Sausage | Dry | Dry Sausage |

---

# 57. V1 Processing Stations

## ChefZ_GrainMill

```text
Wheat → Flour
```

---

## ChefZ_MeatGrinder

```text
Meat → Minced Meat
```

---

## ChefZ_Mortar

```text
Peppercorns → Black Pepper
Dried Paprika → Paprika Powder
Dried Herbs → Herb Mix
```

---

## ChefZ_DryingRack

```text
Herbs
Pepper
Paprika
Pasta
Meat
Fish
Sausage
```

---

## ChefZ_Smoker

```text
Meat
Fish
Sausage
```

---

## ChefZ_CuttingBoard

Unterstützt:

```text
Vegetables
Meat
Herbs
```

---

# 58. V1 Tools

```text
Knife / Cutting Tool
ChefZ_RollingPin
ChefZ_Mortar
ChefZ_MeatGrinder
ChefZ_GrainMill
```

Optional:

```text
ChefZ_ButterChurn
ChefZ_CheesePress
```

Diese können notfalls erst V2 erhalten.

---

# 59. V1 Cooking Devices

ChefZ V1 nutzt:

```text
FryingPan
CookingPot
Cauldron
Fireplace
GasStove
Oven
ChefZ_Smoker
ChefZ_DryingRack
```

---

# 60. V1 Dish Containers

```text
ChefZ_EmptyPlate
ChefZ_EmptyBowl
```

Optional:

```text
ChefZ_EmptyJar
ChefZ_EmptyCan
```

Jar/Canning kann V2 werden.

---

# 61. V1 Tellergerichte

## 1. Survivor Spaghetti

```text
Dried/Raw Pasta
+ Tomato Sauce
+ Salt
→ ChefZ_SurvivorSpaghetti
```

Optional:

```text
Black Pepper
Parsley
```

---

## 2. Wurst-Nudeln-Pfanne

```text
Pasta
+ Sausage
+ Fat / Butter
+ Salt
→ ChefZ_SausagePasta
```

Optional:

```text
Paprika Powder
Black Pepper
```

---

## 3. Jägernudeln

```text
Pasta
+ Wild Meat
+ Mushrooms
+ Thyme
→ ChefZ_HunterPasta
```

Optional:

```text
Cream
```

---

## 4. Rahm-Pilz-Nudeln

```text
Pasta
+ Mushrooms
+ Cream
+ Parsley
+ Salt
→ ChefZ_CreamMushroomPasta
```

---

## 5. Chernarus Mac & Cheese

```text
Pasta
+ Milk
+ Cheese
+ Butter
→ ChefZ_MacAndCheese
```

---

## 6. Kartoffeln mit Bratwurst

```text
Potato
+ Sausage
+ Fat
+ Salt
→ ChefZ_SausagePotatoes
```

Optional:

```text
Rosemary
```

---

## 7. Jägerteller

```text
Wild Meat
+ Potato
+ Mushrooms
+ Thyme
→ ChefZ_HunterPlate
```

---

## 8. Blutwurstplatte

```text
Blood Sausage
+ Potato
+ Onion
→ ChefZ_BloodSausagePlate
```

Blutwurst selbst kann optional V1.1 werden, falls Blutbeschaffung zu aufwendig ist.

---

## 9. Fisch mit Kartoffeln

```text
Fish Fillet
+ Potato
+ Dill
+ Salt
→ ChefZ_FishPotatoPlate
```

---

## 10. Bohnen-Wurst-Teller

```text
Baked Beans
+ Sausage
+ Onion
→ ChefZ_BeanSausagePlate
```

Optional:

```text
Paprika Powder
```

---

## 11. Tactical Bacon Breakfast

```text
Tactical Bacon
+ Egg
+ Bread
→ ChefZ_TacticalBreakfast
```

---

## 12. Rührei mit Wurst

```text
Egg
+ Milk
+ Sausage
+ Salt
→ ChefZ_ScrambledEggSausage
```

---

## 13. Bauernfrühstück

```text
Potato
+ Egg
+ Sausage
+ Onion
→ ChefZ_FarmersBreakfast
```

---

## 14. Käse-Fladenbrot

```text
Simple Dough
+ Cheese
+ Salt
→ ChefZ_CheeseFlatbread
```

Optional:

```text
Rosemary
```

---

## 15. Wurstbrot-Teller

```text
Bread
+ Sausage
+ Cheese
→ ChefZ_SausageBreadPlate
```

---

## 16. Pilzpfanne

```text
Mushrooms
+ Butter / Fat
+ Salt
+ Parsley
→ ChefZ_MushroomPan
```

---

## 17. Kartoffelpuffer

```text
Potato
+ Flour
+ Egg
+ Fat
+ Salt
→ ChefZ_PotatoPancakes
```

---

## 18. Fleisch-Teigtaschen

```text
Simple Dough
+ Minced Meat
+ Onion
+ Salt
+ Black Pepper
→ ChefZ_MeatDumplings
```

---

## 19. Milchreis

```text
Rice
+ Milk
→ ChefZ_MilkRice
```

Optional:

```text
Honey
```

---

## 20. Honigbrot-Platte

```text
Bread
+ Honey
→ ChefZ_HoneyBreadPlate
```

Optional:

```text
Butter
```

---

# 62. V1 Suppen und Eintöpfe

Zusätzlich zu den 20 Tellergerichten sollten mindestens folgende Bowl-Dishes vorhanden sein.

## Hunter Stew

```text
Wild Meat
+ Potato
+ Mushrooms
+ Water/Broth
+ Thyme
→ ChefZ_HunterStew
```

---

## Fisherman's Stew

```text
Fish
+ Potato
+ Carrot
+ Water/Broth
+ Dill
→ ChefZ_FishermanStew
```

---

## Vegetable Soup

```text
Potato
+ Carrot
+ Onion
+ Cabbage
+ Water
→ ChefZ_VegetableSoup
```

---

## Bone Broth Soup

```text
Bone Broth
+ Vegetables
+ Herbs
→ ChefZ_BoneBrothSoup
```

---

## Chernarus Chili

```text
Meat
+ Baked Beans
+ Tomato
+ Paprika
+ Paprika Powder
→ ChefZ_ChernarusChili
```

---

# 63. Dish Quality

Alle V1-Gerichte können intern folgende Stufen besitzen:

```text
SIMPLE
PREPARED
SEASONED
PREMIUM
```

Beispiel:

```text
Pasta + Tomato Sauce
→ SIMPLE
```

```text
Pasta + Tomato Sauce + Salt
→ PREPARED
```

```text
+ Pepper + Parsley
→ SEASONED
```

```text
+ hochwertiges Fleisch / Premium Sauce / Skill Bonus
→ PREMIUM
```

---

# 64. V1 Food State Map

```text
RAW
│
├── PREPARED
│
├── COOKED
├── FRIED
├── BOILED
├── BAKED
├── SALTED
├── DRIED
├── SMOKED
├── BURNT
└── ROTTEN
```

Nicht zwingend alle Zustände müssen eigene Itemklassen sein.

---

# 65. V1 Haltbarkeitslogik

Vorschlag:

| Zustand | Multiplikator |
|---|---:|
| Raw | 1.00 |
| Prepared | 1.00 |
| Cooked | 0.80 |
| Salted | 0.50 |
| Smoked | 0.25 |
| Dried | 0.15 |

Pickled/Canned erst später.

---

# 66. Terje Skills – Übergabepunkte

## Survival XP

ChefZ_TerjeSkills kann XP vergeben bei:

```text
Herb Harvest
Salt Production
Grinding
Milling
Dough Production
Pasta Production
Sausage Production
Smoking
Drying
Cooking
```

---

## Hunting

ChefZ vergibt kein Hunting XP.

```text
Animal Butchering
→ Terje Hunting
```

---

## Fishing

ChefZ vergibt kein Fishing XP für:

```text
Catch
Fillet
```

ChefZ übernimmt erst:

```text
Smoke
Dry
Cook
```

---

## Metabolism

ChefZ vergibt keine zusätzliche Metabolism XP.

Normale Nutritional Profiles erlauben Terje automatische Verarbeitung.

---

# 67. Terje Herbalist

Geplanter Perk:

```text
Skill: Survival
ID: chefzherb
```

Wirkt auf:

```text
Parsley
Dill
Thyme
Rosemary
Wild Garlic
```

Optional auch:

```text
Pepper Plant
```

---

# 68. Terje Medicine – V1 Übergabepunkte

ChefZ_TerjeMedicine kann später Effects ergänzen für:

```text
ChefZ_ThymeTea
ChefZ_WildGarlicTea
ChefZ_HerbalTea
```

V1-Medicine sollte auf kleine unterstützende Effekte beschränkt bleiben.

Beispiel:

```text
Immunity Gain
Warmth
Hydration
leichte Recovery
```

Keine starken Antibiotika-Ersatzwirkungen.

---

# 69. V1 3D Asset Requirement List

## Pflanzen

```text
Wheat Plant
Pepper Plant
Paprika Plant
Onion
Garlic
Carrot
Cabbage

Parsley Plant
Dill Plant
Thyme Plant
Rosemary Plant
Wild Garlic Plant
```

---

## Zutaten

```text
Wheat
Flour
Yeast
Simple Dough
Yeast Dough
Pasta Dough
Raw Pasta
Dried Pasta

Milk
Cream
Butter
Cheese

Pepper Berries
Dried Peppercorns
Black Pepper

Paprika
Dried Paprika
Paprika Powder

Fresh Herbs
Dried Herbs
Herb Mix

Raw Salt
Salt

Minced Meat
Sausage Casing
Raw Sausage
Cooked Sausage
Smoked Sausage
Dry Sausage
```

---

# 70. V1 Station Assets

```text
ChefZ_GrainMill
ChefZ_MeatGrinder
ChefZ_Mortar
ChefZ_DryingRack
ChefZ_Smoker
ChefZ_CuttingBoard
ChefZ_RollingPin
```

Optional später:

```text
ChefZ_ButterChurn
ChefZ_CheesePress
```

---

# 71. Shared Mesh Strategy

Nicht jedes Item benötigt komplett einzigartige Geometrie.

## Würste

Empfehlung:

```text
3 Grund-Meshes
×
verschiedene Texturen
```

Beispiele:

- gerade Wurst
- gebogene Wurst
- größere Räucherwurst

---

## Kräuter

Möglich:

```text
2–3 Grundformen
×
individuelle Blatt-/Texturvarianten
```

wenn visuell ausreichend unterscheidbar.

---

## Gewürze

Ein gemeinsames Behälter-/Pile-Mesh kann für:

- Salz
- Pfeffer
- Paprikapulver
- Kräutermix

mit unterschiedlichen Texturen verwendet werden.

---

# 72. V1 Dish Asset List

Mindestens:

```text
ChefZ_SurvivorSpaghetti
ChefZ_SausagePasta
ChefZ_HunterPasta
ChefZ_CreamMushroomPasta
ChefZ_MacAndCheese
ChefZ_SausagePotatoes
ChefZ_HunterPlate
ChefZ_BloodSausagePlate
ChefZ_FishPotatoPlate
ChefZ_BeanSausagePlate
ChefZ_TacticalBreakfast
ChefZ_ScrambledEggSausage
ChefZ_FarmersBreakfast
ChefZ_CheeseFlatbread
ChefZ_SausageBreadPlate
ChefZ_MushroomPan
ChefZ_PotatoPancakes
ChefZ_MeatDumplings
ChefZ_MilkRice
ChefZ_HoneyBreadPlate

ChefZ_HunterStew
ChefZ_FishermanStew
ChefZ_VegetableSoup
ChefZ_BoneBrothSoup
ChefZ_ChernarusChili
```

Also:

**25 fertige Gerichte für V1**, wenn Suppen/Eintöpfe direkt enthalten werden.

---

# 73. V1 Ingredient Class List

Empfohlene eigene ChefZ-Klassen:

```text
ChefZ_Wheat
ChefZ_WheatSeeds
ChefZ_Flour
ChefZ_Yeast

ChefZ_SimpleDough
ChefZ_YeastDough
ChefZ_PastaDough
ChefZ_RawPasta
ChefZ_DriedPasta
ChefZ_Bread
ChefZ_Flatbread

ChefZ_Paprika
ChefZ_ChoppedPaprika
ChefZ_DriedPaprika
ChefZ_PaprikaPowder

ChefZ_PepperBerries
ChefZ_DriedPeppercorns
ChefZ_BlackPepper

ChefZ_Onion
ChefZ_ChoppedOnion
ChefZ_Garlic
ChefZ_ChoppedGarlic
ChefZ_Carrot
ChefZ_ChoppedCarrot
ChefZ_Cabbage
ChefZ_ChoppedCabbage

ChefZ_Parsley
ChefZ_DriedParsley
ChefZ_Dill
ChefZ_DriedDill
ChefZ_Thyme
ChefZ_DriedThyme
ChefZ_Rosemary
ChefZ_DriedRosemary
ChefZ_WildGarlic
ChefZ_DriedWildGarlic

ChefZ_HerbMix
ChefZ_HunterSeasoning

ChefZ_RawSalt
ChefZ_Salt

ChefZ_DicedMeat
ChefZ_MincedMeat
ChefZ_SausageCasing

ChefZ_RawSausage
ChefZ_RawPorkSausage
ChefZ_RawVenisonSausage
ChefZ_RawBoarSausage
ChefZ_RawHunterSausage
ChefZ_RawSpicySausage

ChefZ_CookedSausage
ChefZ_SmokedSausage
ChefZ_DrySausage

ChefZ_SaltedMeat
ChefZ_DriedMeat
ChefZ_SmokedMeat

ChefZ_SaltedFish
ChefZ_DriedFish
ChefZ_SmokedFish

ChefZ_Milk
ChefZ_Cream
ChefZ_Butter
ChefZ_Cheese
ChefZ_Egg

ChefZ_TomatoSauce
ChefZ_CreamSauce
ChefZ_MushroomCreamSauce
ChefZ_BoneBroth

ChefZ_EmptyPlate
ChefZ_EmptyBowl
```

---

# 74. V1 Scope – bewusst NICHT enthalten

Um V1 kontrollierbar zu halten, sollten folgende Systeme zunächst draußen bleiben:

```text
Fermentation
Sauerkraut
Joghurt
mehrere Käsesorten
Tiermelken
Canning
Einmachgläser
Pickling
komplexe Hygiene
Geschirrspülen
Food Allergies
eigener Cooking Skill
mehr als 3 ChefZ-Terje-Perks
komplexe saisonale Pflanzen
Zuckerproduktion
Sonnenblumenöl-Produktion
Alkohol/Fermentation
```

Diese können später modular ergänzt werden.

---

# 75. V1 Mindestumfang

ChefZ V1 sollte mindestens enthalten:

## Farming

```text
9 Nahrungspflanzen
5 Kräuter
```

## Processing

```text
Grain Mill
Meat Grinder
Mortar
Drying Rack
Smoker
Cutting Board
Rolling Pin
```

## Basisproduktion

```text
Wheat → Flour
Flour → Dough
Dough → Bread
Flour → Pasta
Saltwater → Salt
Pepper → Pepper
Paprika → Paprika Powder
Meat → Minced Meat
Minced Meat → Sausage
Meat/Fish/Sausage → Preserve
Milk → Cream/Butter/Cheese
```

## Cooking

```text
20 Tellergerichte
5 Bowl-Dishes
```

## Compatibility

```text
Terje Survival XP
Herbalist Perk
Metabolism Automatic
Hunting Compatibility
Fishing Compatibility
optional Terje Medicine Herbs
```

---

# 76. Produktionsabhängigkeiten nach Station

## Grain Mill

Benötigt für:

```text
Flour
```

Dadurch indirekt:

```text
Bread
Pasta
Dumplings
Potato Pancakes
```

---

## Meat Grinder

Benötigt für:

```text
Minced Meat
```

Dadurch:

```text
Sausages
Meat Dumplings
```

---

## Mortar

Benötigt für:

```text
Black Pepper
Paprika Powder
Herb Mix
Hunter Seasoning
```

---

## Drying Rack

Benötigt für:

```text
Dried Herbs
Dried Pepper
Dried Paprika
Dried Pasta
Dried Meat
Dried Fish
Dry Sausage
```

---

## Smoker

Benötigt für:

```text
Smoked Meat
Smoked Fish
Smoked Sausage
```

---

# 77. Kritische V1-Systeme

Für die Umsetzung sind besonders kritisch:

```text
1. Recipe Engine
2. Ingredient Categories
3. Food Tags
4. Processing Stations
5. State Transition System
6. Quantity Handling
7. Preservation
8. Portions
9. Nutrition Transfer
10. Terje Compatibility Hooks
```

Diese sollten vor dem Großteil der Content-Erstellung stabil funktionieren.

---

# 78. Empfohlene technische Implementierungsreihenfolge

## Phase 1

```text
ChefZ_Core
Categories
Tags
Recipe Definitions
Logging
```

## Phase 2

```text
Basic Ingredients
Cutting
Grinding
Milling
```

## Phase 3

```text
Wheat
Flour
Dough
Pasta
Bread
```

## Phase 4

```text
Herbs
Pepper
Paprika
Salt
```

## Phase 5

```text
Meat Grinder
Minced Meat
Sausages
```

## Phase 6

```text
Drying Rack
Smoker
Preservation
```

## Phase 7

```text
Sauces
Broth
Dairy
```

## Phase 8

```text
20 Plate Dishes
5 Bowl Dishes
Portions
```

## Phase 9

```text
ChefZ_TerjeSkills
Herbalist
Survival XP
```

## Phase 10

```text
ChefZ_TerjeMedicine
Herbal Tea
Food Risks
```

---

# 79. Gesamt-Dependency-Map

```text
WORLD
│
├── FARMING
│   ├── Wheat
│   ├── Vegetables
│   ├── Herbs
│   ├── Paprika
│   └── Pepper
│
├── HUNTING
│   └── Meat / Fat / Bones / Intestines
│
├── FISHING
│   └── Fish
│
├── WATER
│   ├── Fresh Water
│   └── Saltwater
│
└── LOOT
    ├── Yeast
    ├── Milk
    ├── Rice
    ├── Honey
    ├── Beans
    └── Tactical Bacon

                ↓

PROCESSING
│
├── Grain Mill
├── Meat Grinder
├── Mortar
├── Cutting Board
├── Rolling Pin
├── Drying Rack
└── Smoker

                ↓

INTERMEDIATE PRODUCTS
│
├── Flour
├── Dough
├── Pasta
├── Salt
├── Pepper
├── Paprika Powder
├── Dried Herbs
├── Herb Mix
├── Minced Meat
├── Sausage
├── Preserved Meat
├── Preserved Fish
├── Butter
├── Cheese
├── Tomato Sauce
├── Cream Sauce
└── Bone Broth

                ↓

COOKING
│
├── Plate Dishes
├── Bowl Dishes
├── Breakfast
├── Pasta
├── Meat Dishes
├── Fish Dishes
└── Baked Food

                ↓

OUTPUT
│
├── Eat immediately
├── Store
├── Preserve
├── Trade
├── Serve group
└── Quest / Achievement

                ↓

OPTIONAL COMPATIBILITY
│
├── Terje Survival
├── Herbalist
├── Terje Metabolism
├── Terje Hunting
├── Terje Fishing
└── Terje Medicine
```

---

# 80. Ergebnis

Mit dieser Map besitzt ChefZ V1 eine geschlossene Gameplay-Schleife:

```text
SAMMELN / FARMEN / JAGEN / ANGELN
↓
ROHSTOFFE
↓
VERARBEITUNG
↓
ZWISCHENPRODUKTE
↓
WÜRZEN / KONSERVIEREN
↓
KOCHEN
↓
SERVIEREN
↓
ESSEN / LAGERN / HANDELN
```

Die nächste sinnvolle Planungsebene nach dieser Map ist:

```text
ChefZ V1 Item Specification
```

Dort wird für **jede einzelne Klasse** festgelegt:

- Classname
- Displayname
- Beschreibung
- Modul
- Kategorie
- Tags
- Gewicht
- Größe
- Quantity
- Nutrition
- Haltbarkeit
- Food State
- Model
- Texture
- Crafting Input
- Crafting Output
- benötigtes Tool
- benötigte Station
- Terje XP
- Terje Perk Interaction
- Trader Status
- Loot Status
