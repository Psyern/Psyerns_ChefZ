# Modules

ChefZ is one mod folder containing nine addons, plus three separate
compatibility mods that ship as their own PBOs. This page lists what each one
contains, what it depends on and how many classes and records it contributes.

All counts on this page were taken from the files, not estimated. They are a
snapshot; re-count before quoting them in a release note.

For the reasoning behind the split, see [Architecture](Architecture).

---

## Overview

| Addon | Item classes | Script files | Rank 1 records | Rank 2 records | Stringtable keys |
|---|---:|---:|---:|---:|---:|
| `ChefZ_Core` | 0 | 133 | — | 1 | 4 |
| `ChefZ_Registry` | 0 | 0 | — | 148 | — |
| `ChefZ_Farming` | 43 | 4 | 6 | 12 | 73 |
| `ChefZ_Processing` | 12 | 6 | 16 | 28 | 40 |
| `ChefZ_Ingredients` | 28 | 3 | 9 | 36 | 63 |
| `ChefZ_Meat` | 22 | 1 | — | 54 | 52 |
| `ChefZ_Baking` | 8 | 1 | — | 17 | 22 |
| `ChefZ_Preservation` | 9 | 1 | 10 | 20 | 28 |
| `ChefZ_Cooking` | 63 | 4 | 70 | 39 | 139 |
| **total** | **185** | **153** | **111** | **355** | **421** |

| Comp mod | Item classes | Script files | Stringtable keys |
|---|---:|---:|---:|
| `Psyerns_ChefZ_COT_Comp` | 0 | 2 | 10 |
| `Psyerns_ChefZ_Terje_Skills_Comp` | 0 | 9 | 2 |
| `Psyerns_ChefZ_Terje_Medicine_Comp` | 0 | 3 | 1 |

"Item classes" counts top-level `class` definitions inside `CfgVehicles`,
including the `scope = 0` base classes. "Rank 1" counts records declared in
`CfgChefZ*` config trees; "rank 2" counts records inside JSON documents. The
two ranks are explained on [Architecture](Architecture#3-where-configuration-comes-from-three-ranks).

Across all modules that adds up to **44 recipes**, **58 transforms**,
**21 processes**, **9 stations**, **160 ingredient bindings**, **5 containers**,
**3 cooking devices**, **10 food states**, **5 quality tiers** and
**2 tool groups**.

---

## `ChefZ_Core`

The rule engine. It contains no item, no ingredient, no dish and no station —
`units[]` and `weapons[]` are empty and stay that way.

- **133 script files** across four layers: 57 in `1_Core`, 40 in `3_Game`, 30 in
  `4_World`, 5 in `5_Mission`, plus one PBO-JSON smoke probe under `Tests/`.
  18 of them are self-tests.
- **Data**: `Config/Core.json` (one `coreSettings` record — the default
  settings, described on [Configuration](Configuration)) and
  `Config/Templates/Core.overlay.json`, the template copied to
  `$profile:ChefZ\Core.json` on first server start.
- **No `CfgChefZ` node.** The core does not register itself with itself, and it
  reserves zero handcraft recipe slots.
- **Depends on**: `DZ_Data` only.

Everything else is documented on [Architecture](Architecture).

---

## `ChefZ_Registry`

The merged shared vocabulary, and nothing else. No script, no model, no
`CfgVehicles` entry — `units[]` is empty and that is not a forgotten line.

- **148 records** in four documents:
  - `Config/Categories.json` — 34 categories
  - `Config/Tags.json` — 21 tags
  - `Config/Nutrition.json` — 87 nutrition records
  - `Config/Preservation.json` — 6 preservation rules
- **`CfgChefZ` node**: `ChefZ_MergedRegistry`, `loadOrder = 150`,
  `handcraftRecipeSlots = 0`. The node is deliberately *not* named like the
  addon: a node with the same name next to the `CfgPatches` entry counts as a
  duplicate class definition for `configcpp.mjs`.
- **Depends on**: `DZ_Data`, `ChefZ_Core`, and the seven content addons whose
  classes its nutrition records name. Those are real data dependencies — a
  nutrition record without its class is a nutrition value for nothing. None of
  those seven depends on `ChefZ_Registry` in return, so there is no cycle.

`loadOrder = 150` puts it ahead of every content slice (the earliest is
`ChefZ_SaltChain` at 155). Reading the shared vocabulary first means every slice
afterwards is checked against a complete category and tag set, and a later,
*differing* second definition of the same id fails loudly as a same-rank
duplicate instead of quietly displacing the merged version.

### Why this addon exists, and why it is not part of the core

Both reasons are written into `ChefZ_Registry/config.cpp`.

In milestone 2 the merged registries lived in `ChefZ_Core/Config/`. That was
wrong twice over:

1. **The core is a rule engine without vocabulary.** Categories, tags and
   nutrition values *are* vocabulary. Putting them in the core contradicts the
   architecture chosen in milestone 1.
2. **Nobody was allowed to declare them in a `dataFiles[]`.** The core has no
   `CfgChefZ` node, so nothing pointed at those files, and the core therefore
   never read them. They were dead files — present, valid, and completely
   without effect.

An addon of its own fixes both: its own `CfgChefZ` registration, one
`dataFiles[]` entry per registry, and the core stays free of content.

The merge itself is described on [Delta Protocol](Delta-Protocol).

---

## `ChefZ_Farming`

Grain, vegetables and herbs — the start of most production chains. Since
2026-08-29 all of them are **found**, like vanilla mushrooms: there are no plant
classes, no seeds and no growth stages. Where they spawn is the server's `types.xml`.

- **item classes**: wheat, the four ChefZ vegetables (`ChefZ_Onion`,
  `ChefZ_Garlic`, `ChefZ_Carrot`, `ChefZ_Cabbage`), six fresh herbs and spices,
  plus the apiary.
- **Script files**: `ChefZ_FarmingItems.c`, `ChefZ_HerbItems.c`,
  `ChefZ_ProduceFarming.c`, `ChefZ_Apiary.c`.
- **Rank 1**: 5 ingredient bindings (`ChefZ_ProduceIngredient` and the four
  vegetables).
- **Rank 2**: ingredient records in `Config/GrainIngredients.json` and
  `Config/Ingredients/Herbs.json`.
- **`CfgChefZ` slices**: `ChefZ_GrainFarming` (210, 0 slots),
  `ChefZ_HerbFarming` (215, 0 slots).
- **Depends on**: `DZ_Data`, `DZ_Gear_Cultivation`, `DZ_Gear_Food`, `ChefZ_Core`.

`ChefZ_FreshHerbBase` is the only ChefZ class extended by `modded class` from a
comp mod (see [Terje Compatibility](Terje-Compatibility)).

---

## `ChefZ_Processing`

Stations and tools. Every processing station in ChefZ lives here, regardless of
which chain it belongs to.

- **11 item classes**, including all 8 stations: `ChefZ_GrainMill`,
  `ChefZ_Mortar`, `ChefZ_DryingRack`, `ChefZ_ButterChurn`, `ChefZ_CheesePress`,
  `ChefZ_Smoker`, `ChefZ_SaltPan`, `ChefZ_MeatGrinder`. The cutting board is
  gone — cutting is "ingredient + knife".
- **6 script files**, each of which is a list of empty derivations from
  `ChefZ_ProcessingStation_Base` — the station behaviour is entirely in the core
  and in data.
- **Rank 1**: 14 processes and 2 tool groups (`CUTTING_TOOL` with eight vanilla
  knives, `ROLLING_PIN`). Tool group classes are deliberately *not* checked
  against `CfgVehicles`: a knife from an optional module may be named without
  being loaded.
- **Rank 2**: 9 stations, 17 transforms, 1 process, 1 ingredient across 13 JSON
  documents.
- **`CfgChefZ` slices**: six, all with `handcraftRecipeSlots = 0` —
  `ChefZ_SaltChain` (155), `ChefZ_MeatProcessing` (190), `ChefZ_GrainProcessing`
  (220), `ChefZ_HerbProcessing` (230), `ChefZ_DairyProcessing` (260),
  `ChefZ_PreservationStations` (270).
- **Depends on**: `DZ_Data`, `DZ_Gear_Camping`, `DZ_Gear_Tools`, `DZ_Gear_Food`,
  `DZ_Gear_Cooking`, `ChefZ_Core`, `ChefZ_Farming`.

See [Processing Stations](Processing-Stations).

---

## `ChefZ_Ingredients`

Cut produce, dairy, salt, spices and mushrooms — the intermediate goods that sit
between raw produce and a dish.

- **28 item classes**: the `Chopped*` vegetables, `ChefZ_SlicedPotato`, dairy
  (`ChefZ_Butter`, `ChefZ_Cheese`, `ChefZ_Cream`, …), `ChefZ_Salt` and
  `ChefZ_RawSalt`, dried spices.
- **3 script files**.
- **Rank 1**: 8 ingredient bindings, 1 process (`PROCESS_CHOP_VEGETABLE`).
- **Rank 2**: 28 ingredient records, 8 transforms.
- **`CfgChefZ` slices**: five — `ChefZ_SaltIngredients` (205),
  `ChefZ_Produce` (220, **12 handcraft slots**), `ChefZ_HerbIngredients` (220),
  `ChefZ_SauceIngredients` (230), `ChefZ_DairyIngredients` (260).
- **Depends on**: `DZ_Data`, `DZ_Gear_Food`, `DZ_Gear_Consumables`,
  `ChefZ_Core`, `ChefZ_Farming`, `ChefZ_Processing`.

`ChefZ_Produce` carries the largest handcraft reservation in the mod. See
[Production Chains](Production-Chains).

---

## `ChefZ_Meat`

Butchery products and the sausage chain.

- **22 item classes**: `ChefZ_DicedMeat`, the `Minced*` classes,
  `ChefZ_SausageCasing`, the raw sausages.
- **1 script file** (`ChefZ_MeatItemBase.c`).
- **Rank 1**: none.
- **Rank 2**: **54 records** — 34 ingredient bindings, 14 transforms, 6 recipes.
  The ingredient count is the highest in the mod because this module also
  classifies *vanilla* meat: `PigSteakMeat` through `BearSteakMeat`, plus
  `Lard`, `Bone` and `Guts`. ChefZ creates no own class for those — that would
  be a second version of the same thing — it only files them into categories.
- **`CfgChefZ` slice**: `ChefZ_Meat` (200, **1 handcraft slot**).
- **Depends on**: `DZ_Data`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Processing`.

---

## `ChefZ_Baking`

Yeast, doughs, pasta, bread.

- **8 item classes**.
- **1 script file** (`ChefZ_BakingItems.c`).
- **Rank 1**: none.
- **Rank 2**: 8 ingredient records, 5 transforms, 2 processes
  (`PROCESS_KNEAD`, `PROCESS_ROLL`), 2 recipes.
- **`CfgChefZ` slice**: `ChefZ_GrainBaking` (230, **4 handcraft slots**).
- **Depends on**: `DZ_Data`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Farming`,
  `ChefZ_Processing`.

---

## `ChefZ_Preservation`

Salting, drying, smoking — and the food-state vocabulary of the whole mod.

- **9 item classes**: `ChefZ_SaltedMeat`, `ChefZ_DriedMeat`, `ChefZ_SmokedMeat`,
  the fish equivalents, `ChefZ_SmokedSausage`, `ChefZ_DrySausage`.
- **1 script file** (`ChefZ_PreservedFood_Base.c`).
- **Rank 1**: **all 10 food states** — `RAW`, `PREPARED`, `COOKED`, `BAKED`,
  `FRIED`, `SALTED`, `SMOKED`, `DRIED`, `BURNT`, `ROTTEN`. States are
  sync-relevant, so rank 1 is not a choice; see
  [Architecture](Architecture#how-the-ranks-merge) and [Food States](Food-States).
- **Rank 2**: 12 ingredient records, 8 transforms.
- **`CfgChefZ` slice**: `ChefZ_Preservation` (280, **2 handcraft slots**).
- **Depends on**: `DZ_Data`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Processing`,
  `ChefZ_Meat`.

This is the module that decides how long everything in ChefZ keeps: `SALTED`
gets a 43 200 s freshness lifetime, `SMOKED` 86 400 s, `DRIED` 129 600 s, and
each of them implies the tag `CHEFZ_PRESERVED`.

---

## `ChefZ_Cooking`

The largest module. Sauces, broths, tableware and all 25 plated and bowl dishes.

- **63 item classes**: 4 sauces and broths, 5 empty containers plus
  `ChefZ_ContainerItemBase`, the two dish base classes
  (`ChefZ_PortionedDish_Base`, `ChefZ_ServedDish_Base`, both `scope = 0`), and
  50 dish classes — every dish is **two** classes, a `*Bulk` that forms in the
  cooking vessel and carries the portion counter, and a served portion that the
  player eats.
- **4 script files**: `ChefZ_SauceItems.c`, `ChefZ_ServingItems.c`,
  `ChefZ_BowlDishItems.c`, `ChefZ_DishesBItems.c`.
- **Rank 1**: **70 records** — 55 ingredient bindings, 5 quality tiers
  (`POOR`/`SIMPLE`/`PREPARED`/`SEASONED`/`PREMIUM`, tier set `DISH_DEFAULT`),
  5 containers (`PLATE`, `BOWL`, `CAN`, `JAR`, `BOX`), 3 cooking devices
  (`FryingPan` 2 portions, `Pot` 4, `Cauldron` 12), 2 processes.
- **Rank 2**: **36 of the mod's 44 recipes**, plus 2 transforms and 1 ingredient.
- **`CfgChefZ` slices**: five — `ChefZ_Sauces` (300), `ChefZ_Serving`
  (310, **2 handcraft slots**), `ChefZ_DishesA` (330), `ChefZ_DishesB` (330),
  `ChefZ_DishesC` (340).
- **Depends on**: `DZ_Data`, `DZ_Gear_Food`, `DZ_Gear_Cooking`, `ChefZ_Core`,
  `ChefZ_Ingredients`, `ChefZ_Farming`, `ChefZ_Meat`, `ChefZ_Processing`,
  `ChefZ_Baking`, `ChefZ_Preservation` — every other content addon except
  `ChefZ_Registry`.

This module is a **shared folder**: the slices `sauces`, `serving`, `dishes-a`,
`dishes-b` and `dishes-c` all write into it, each under its own
`### SLICE <name> ###` banner. Anyone adding to it appends and overwrites
nothing.

One inconsistency worth knowing: the twenty `dishes-b` classes have explicit
(empty) script derivations in `ChefZ_DishesBItems.c`, while the twenty
`dishes-a` classes have none. Both work — the engine walks the *config* parent
chain up to `ChefZ_PortionedDish_Base` / `ChefZ_ServedDish_Base`, which do have
script classes — but the two slices are not written the same way.

See [Portions and Containers](Portions-and-Containers) and
[Quality and Nutrition](Quality-and-Nutrition).

---

## Dependency order

Read top to bottom; each module only depends on modules above it.

```
ChefZ_Core
  ChefZ_Farming
    ChefZ_Processing
      ChefZ_Ingredients
      ChefZ_Meat
      ChefZ_Baking
        ChefZ_Preservation
          ChefZ_Cooking
            ChefZ_Registry   (depends on all seven content addons)
```

`ChefZ_Registry` sits last in the dependency graph but first in `loadOrder`
(150). Those are two different orderings and both are intentional:
`requiredAddons[]` is about PBO load and class availability, `loadOrder` is
about the sequence in which the config manager reads slices.

---

## The three compatibility mods

Each is a **separate mod**, not an addon inside `Psyerns_ChefZ_Core`. Each names
its foreign system in `requiredAddons[]`, which means DayZ does not load the PBO
at all when that system is absent — the `modded class` declarations are then
never compiled. ChefZ runs unchanged either way.

None of the three declares a single `CfgVehicles` class.

### `Psyerns_ChefZ_COT_Comp`

Appends eight ChefZ spawn categories to Community Online Tools' object spawner.
An admin tool; no recipe, no nutrition value, no transform, no balancing. The
only intervention is an extra filter in an admin dialog.

- 2 script files, `modded class JMObjectSpawnerForm`.
- Depends on `JM_COT_Scripts` plus the seven ChefZ addons whose classes the
  categories list. `ChefZ_Registry` is deliberately absent — this module reads
  no registry, it only carries class names.
- Every class name is checked against `CfgVehicles` at runtime before it enters
  a category, so a missing ChefZ addon makes its entries vanish silently rather
  than producing ghost classes.

See [COT Compatibility](COT-Compatibility).

### `Psyerns_ChefZ_Terje_Skills_Comp`

Hangs cooking perks into TerjeSkills' existing `surv` skill and feeds ChefZ's
capability and progress registries.

- 8 script files. One of them uses `modded class` on ChefZ's own
  `ChefZ_FreshHerbBase` — the only ChefZ class any comp mod extends.
- Depends on `DZ_Data`, `ChefZ_Core`, `ChefZ_Farming`, `TerjeCore`, `TerjeSkills`.
- The whole XP matrix is config, not script, so an operator can override it via
  `$profile:TerjeSettings\Core\GameOverrides.xml`.

### `Psyerns_ChefZ_Terje_Medicine_Comp`

Gives ChefZ teas TerjeMedicine consumable effects.

- 3 script files, `modded class TerjeConsumableEffects`.
- Depends on `TerjeCore`, `TerjeMedicine`, `ChefZ_Core`.
- Values live under an own config root `CfgChefZTerjeMedicine`, not under
  `CfgVehicles`, because re-opening a ChefZ class body would be a duplicate
  class definition and `configcpp.mjs` rejects it.
- **These entries are dormant.** `ChefZ_ThymeTea`, `ChefZ_WildGarlicTea` and
  `ChefZ_HerbalTea` do not exist in the main mod — no `CfgVehicles` class
  anywhere under `Addons/`. Nothing breaks; the resolver simply never matches,
  and `ChefZ_TerjeMedStartupCheck` reports at server start which of the three is
  missing. They will work unchanged once the main mod ships the classes.

See [Terje Compatibility](Terje-Compatibility).

---

## Related pages

- [Architecture](Architecture) — how the core loads and merges all of this
- [Delta Protocol](Delta-Protocol) — how `ChefZ_Registry` gets its content
- [Adding Content](Adding-Content) — where a new item goes
- [Known Limitations](Known-Limitations) — none of these modules has ever been packed or run
