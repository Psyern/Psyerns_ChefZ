# Psyerns ChefZ

<p align="center">
  <img src="data/Psyerns_ChefZ_Banner.png" alt="Psyerns ChefZ" width="800">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/DayZ-1.29+-0074D9?style=for-the-badge&logo=steam&logoColor=white" alt="DayZ 1.29+">
  <img src="https://img.shields.io/badge/Enforce_Script-Enfusion-FF851B?style=for-the-badge" alt="Enforce Script">
  <img src="https://img.shields.io/badge/Status-Implemented_%2F_Not_Server--Ready-E67E22?style=for-the-badge" alt="Status: implemented, not server-ready">
  <img src="https://img.shields.io/badge/Scope-ChefZ_V1-2ECC40?style=for-the-badge" alt="Scope V1">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="License MIT"></a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Recipes-JSON_driven-E67E22?style=flat-square" alt="JSON Recipes">
  <img src="https://img.shields.io/badge/Optional-Terje_Skills_%7C_Medicine-8E44AD?style=flat-square" alt="Terje">
  <img src="https://img.shields.io/badge/Optional-Community_Online_Tools-3498DB?style=flat-square" alt="COT">
  <img src="https://img.shields.io/badge/Validation-19_static_checkers-E74C3C?style=flat-square" alt="Validators">
</p>

<p align="center">
  <b>A full cooking &amp; food-production overhaul for DayZ</b><br>
  Built on the CookZ principle — put matching ingredients in a pot, get a dish — and extended into a complete<br>
  <b>gather → farm → harvest → process → season → cook → preserve → serve</b> survival chain.<br>
  Vanilla cooking is never blocked: if no ChefZ recipe matches, DayZ cooks the way it always did.
</p>

<p align="center">
  <a href="https://deadmansecho.com">
    <img src="https://img.shields.io/badge/Community-Deadmans_Echo-F0C040?style=for-the-badge" alt="Deadmans Echo">
  </a>
</p>

## Project Status

**The mod is written. It has never kept a DayZ server running.**

Every addon under `Psyerns_ChefZ_Core/Addons/` is implemented: 163 `CfgVehicles`
classes (`scope = 0` base classes included), 171 script files, 480 data records and
349 stringtable keys in 13 languages. The static validator suite runs green. What
has not happened is a server that survives startup — the process registers every
addon, loads its config, and then dies with an access violation in the mission's
`OnInit` chain while the core sits in safe mode with empty registries.

Read [Known Limitations](ChefZ_Wiki/Known-Limitations.md) before putting this
anywhere near a live server. It is the honest inventory and it is kept current on
purpose.

The planning corpus this README summarises — core architecture, content vision, the
full V1 ingredient &amp; production map, the Terje compatibility analysis and the open
design questions — is kept internally and is not part of this repository.

| | |
|---|---|
| **`ChefZ_Core`** | Implemented — 137 script files, zero content classes |
| **Content modules** | Implemented — 7 content addons, the merged registry and 4 asset packages · 47 recipes · 61 transforms · 11 stations |
| **Cookbook** | Implemented as knowledge state and RPC — no UI yet (Milestone 5.1) |
| **Compatibility mods** | Implemented — Terje Skills, Terje Medicine, COT · 0 new item classes |
| **Validation** | 19 checkers · **exit code 0 · 0 errors · 20 warnings** |
| **Validator self-test** | 18 of 19 checkers provably fire · `chefzaction` not yet covered |
| **Packing** | 17 sources, **13 packed** — the four asset addons are skipped, see [Packing](#packing) |
| **Server run** | Boots and registers, then dies in `OnInit` — measured 28.08.2026 |
| **Gates 1–4** | Reports written · Gate 4 verdict: NOT READY |
| **3D assets** | Two deliveries in — 50 models, 52 textures, 45 classes on their own geometry |

## Repository Layout

One Steam Workshop item (`Psyerns_ChefZ_Core`) containing fourteen PBOs, plus three
independent compatibility mods that are only needed if you run Terje or COT.

```text
Psyerns_ChefZ/                              ← repository root (this README)
│
├── data/                                   ← banner, screenshots
├── ChefZ_Wiki/                             ← the full wiki, published from here
│
├── Psyerns_ChefZ_Core/                     ← THE mod (one workshop item, 14 PBOs)
│   ├── Addons/                             14 addons, four of them assets only
│   │   ├── ChefZ_Core/                     systems only — no content
│   │   ├── ChefZ_Registry/                 the merged vocabulary, no scripts, no items
│   │   ├── ChefZ_Farming/                  found plants, herbs, beekeeping
│   │   ├── ChefZ_Processing/               stations, tools, processing steps
│   │   ├── ChefZ_Ingredients/              base ingredients, intermediates, spices
│   │   ├── ChefZ_Meat/                     minced meat, sausages, meat products
│   │   ├── ChefZ_Preservation/             salting, drying, smoking
│   │   ├── ChefZ_Baking/                   dough, bread, flatbread, pasta
│   │   ├── ChefZ_Cooking/                  plates, bowls, stews, breakfasts, sauces
│   │   ├── ChefZ_Cookbook/                 recipe knowledge and RPC — no UI yet
│   │   ├── ChefZ_Devices/                  models and textures — hive, stations
│   │   ├── ChefZ_Food/                     models and textures — prepared food
│   │   ├── ChefZ_Items/                    models and textures — tools, containers
│   │   └── ChefZ_Plants/                   models and textures — crops and herbs
│   ├── Keys/
│   └── _deltas/                            registry deltas from the content slices
│
├── ChefZ/                                   ← the delivery, kept verbatim; not built
│
├── Psyerns_ChefZ_Terje_Skills_Comp/        ← optional mod — Survival XP, Herbalist perk
├── Psyerns_ChefZ_Terje_Medicine_Comp/      ← optional mod — herbal teas, immunity, poisoning
├── Psyerns_ChefZ_COT_Comp/                 ← optional mod — COT spawn categories
│
└── tools/
    ├── chefz-validate/                     static validators (Node, no dependencies)
    └── chefz-pack/                         PBO packing and test-server launch
```


**`ChefZ_Core` is dependency-free** — no Community Framework, no Terje, no Expansion. The
compatibility mods are optional consumers; ChefZ runs unchanged without any of them.

**About `ChefZ/`.** The asset delivery as it was handed over, kept in the repository
unchanged on purpose. It has grown to **129 files** — 50 models, 52 textures and 17
scripts across `ChefZ_Core`, `ChefZ_Devices`, `ChefZ_Food`, `ChefZ_Items` and
`ChefZ_Plants`. Only part of it is in use: the beekeeping and item models were copied
into `ChefZ_Devices`, `ChefZ_Items`, `ChefZ_Food` and `ChefZ_Plants` under `Addons/`
and are bound to their classes since 30.08. This folder is the original, not a second copy
in use. It is **not part of the build** — `pack.mjs` collects
`Psyerns_ChefZ_Core/Addons/*` and root folders matching `Psyerns_ChefZ_*_Comp`, and
`ChefZ/` is neither — and the validator never reads it. Worth knowing: three of its five
`CfgPatches` names — `ChefZ_Core`, `ChefZ_Devices`, `ChefZ_Items` — are also the names
of real addons, so it must never be packed as it stands. `ChefZ_Food` and
`ChefZ_Plants` are so far unique.

`ChefZ_Registry` is the one addon allowed to hold the merged category, tag, nutrition
and preservation tables. Content modules never write them — they hand in a delta, and a
single integrator merges it. See [Registry Delta Protocol](#registry-delta-protocol).

## Features

<table>
<tr>
<td width="33%" valign="top">

### Core Systems
- Recipe Engine
- Ingredient category system
- Food tag system
- Processing system
- Processing stations
- Ingredient state system
- Quality system (5 tiers)
- Nutrition manager
- Preservation manager
- Tool requirement system
- Cooking device adapter
- Portion system
- Dish container system
- Recipe knowledge (cookbook)
- Event API
- Config manager
- Debug logging
- Self-tests across 19 script files

</td>
<td width="33%" valign="top">

### Production Chains
- Wheat / Corn → Flour → Dough → Bread
- Dough → Raw pasta → Dried pasta
- Meat → Minced meat → Sausage
- Sausage → Smoked / dry sausage
- Leg → Steaks (beef, pork, venison)
- Saltwater → Raw salt → Salt
- Herbs → Dried herbs → Herb mix
- Pepper berries → Peppercorns → Black pepper
- Bell pepper → Dried paprika → Powder
- Milk → Cream → Butter / Cheese
- Fish → Salted / dried / smoked fish
- Hive → Frame → Uncapped → Honey
- Bones → Bone broth
- Tomato / cream / mushroom sauces

</td>
<td width="33%" valign="top">

### Content Scope (V1)
- 5 found herbs + pepper berries
- 11 processing stations
- 6 raw and 6 cooked sausages
- 20 plate dishes
- 5 soups &amp; stews
- 3 dishes from pure vanilla produce
- 4 sauces / broths
- Salt, black pepper, paprika powder
- Herb mix, hunter seasoning
- Cream, butter, cheese, eggs
- Beekeeping — hive, frames, extractor
- 18 preserving transforms
- Portioned group meals
- 5 returnable containers

</td>
</tr>
</table>

### Processing Stations

Eleven stations, each declared as data — a station is a class plus a `station` record
naming the processes it accepts.

| Station | Processes | Turns |
|---|---|---|
| `ChefZ_GrainMill` | `PROCESS_MILL` | Wheat / Corn → Flour |
| `ChefZ_MeatGrinder` | `PROCESS_GRIND_MEAT`, `PROCESS_STUFF_SAUSAGE` | Meat → Minced meat → Raw sausage |
| `ChefZ_Mortar` | `PROCESS_GRIND_SPICE`, `PROCESS_GRIND_HERB` | Peppercorns → Black pepper · Dried paprika → Powder · Dried herbs → Herb mix |
| `ChefZ_DryingRack` | `PROCESS_DRY` | Herbs, berries, pepper, paprika, pasta, meat, fish, sausage — 4 parallel slots |
| `ChefZ_Smoker` | `PROCESS_SMOKE` | Meat, fish, sausage — 2 slots, needs fuel |
| `ChefZ_FryingPan` | `PROCESS_BOIL_BRINE`, `PROCESS_DRY_SALT` | Saltwater → Raw salt → Salt |
| `ChefZ_ButterChurn` | `PROCESS_SEPARATE_CREAM`, `PROCESS_CHURN_BUTTER` | Milk → Cream → Butter |
| `ChefZ_CheesePress` | `PROCESS_PRESS_CHEESE` | Milk → Cheese |
| `ChefZ_Beehive` | `PROCESS_HARVEST_HIVE` | Frames fill one after another |
| `ChefZ_BeehiveDouble` | `PROCESS_HARVEST_HIVE` | The extended hive |
| `ChefZ_HoneyExtractor` | `PROCESS_SPIN_HONEY` | Uncapped frame → Honey, runs itself |

`ChefZ_FryingPan` is a **station**, not the vanilla `FryingPan` cooking device listed
further down — same idea, different class. It boils brine over a fire and dries the
raw salt afterwards; the fire is only needed for the first half.

### Preservation Matrix

Eighteen transforms across three processes. Drying is the widest: it takes anything
that keeps better dry, not just meat.

| Input | Process | Output |
|---|---|---|
| Raw Meat + Salt | Salt-cure | Salted Meat |
| Fish Fillet + Salt | Salt-cure | Salted Fish |
| Salted Meat | Dry | Dried Meat |
| Salted Fish | Dry | Dried Fish |
| Salted Meat | Smoke | Smoked Meat |
| Fish Fillet | Smoke | Smoked Fish |
| Raw Sausage | Smoke | Smoked Sausage |
| Raw Sausage | Dry | Dry Sausage |
| Raw Pasta | Dry | Dried Pasta |
| Parsley · Thyme · Rosemary · Wild Garlic | Dry | the four dried herbs |
| Bell Pepper | Dry | Dried Paprika |
| Pepper Berries | Dry | Dried Peppercorns |
| Canina · Sambucus Berries | Dry | Dried Berries |

Spoilage is driven by state and category, not by per-item code:

| Scope | Id | Spoilage multiplier |
|---|---|---:|
| State | `COOKED` | 0.80 |
| State | `SALTED` | 0.50 |
| State | `SMOKED` | 0.25 |
| State | `DRIED` | 0.15 |
| Category | `BROTH` | 0.90 |
| Category | `SAUCE` | 0.70 |

### Dish Quality

Quality is derived from what actually went into the pot, and affects yield, shelf life,
portions, buffs, trade value and the UI description. Five tiers, scored — a dish can end
up *below* the baseline.

| Tier | Score from | Yield | Spoilage |
|---|---:|---:|---:|
| `POOR` | −99 | ×0.75 | ×1.25 |
| `SIMPLE` | 0 | ×1.00 | ×1.00 |
| `PREPARED` | 2 | ×1.10 | ×0.95 |
| `SEASONED` | 4 | ×1.25 | ×0.90 |
| `PREMIUM` | 7 | ×1.50 | ×0.85 |

## The Central Design Rule

> **`ChefZ_Core` contains systems — ChefZ modules contain content.**

The Core does not know what "Survivor Spaghetti" is. It only knows what a recipe, an
ingredient, a category, a processing step, a container and a result *are*. Adding a new dish
must never require a Core code change.

```text
NEW CONTENT              instead of        NEW CONTENT
↓                                          ↓
CONFIG / MODULE                            CHANGE CORE CODE
↓                                          ↓
CHEFZ CORE                                 RISK BREAKING EVERYTHING
```

And the rule that protects everyone's existing server:

```text
ChefZ Recipe Check
↓
No Match
↓
Vanilla DayZ Cooking          ← unchanged, always
```

## System Architecture

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

### Recipe Matching

The engine checks the cooking device and method, the items present, their categories,
tags, states, counts and amounts, which slots are optional, the forbidden states, and
which recipe wins when several match.

```text
Cooking Pot
│
├── Dried Pasta        slot "pasta"     category PASTA
├── Pork Sausage       slot "sausage"   category SAUSAGE
├── Butter             slot "fat"       BUTTER or FAT
├── Tomato Sauce       slot "sauce"     optional, +2 grade points
└── Salt               slot "salt"      optional, 5 g, +2 grade points

→ RCP_ChefZ_SausagePasta, quality SEASONED, 2 portions on a plate
```

A slot matches by **`cls`** (one exact class), **`category`**, **`tag`**, or **`anyOf`**
over any of those. `minCount` / `maxCount` bound how many items may fill it, `optional`
makes it skippable, and `unit` + `amount` express a quantity rather than a whole item.
`policy.extraItems` decides whether anything unmatched in the pot is tolerated, and
`policy.forbiddenStates` keeps burnt and rotten input out.

When several recipes match, **the most specific valid recipe wins** — resolved through
an explicit `priority` field per recipe.

### Categories

41 categories: 23 roots and 18 children. `parent` is the only structure — there is no
second hierarchy hiding anywhere.

```text
MEAT ──┬── DOMESTIC_MEAT      VEGETABLE ──┬── ROOT_VEGETABLE     GRAIN ── FLOUR
       ├── WILD_MEAT                      ├── LEAF_VEGETABLE     SPICE ── SALT
       ├── POULTRY                        ├── TOMATO             HERB  ── DRIED_HERB
       ├── PREDATOR_MEAT                  └── BEANS              FRUIT ── BERRY
       ├── MINCED_MEAT
       └── SAUSAGE           DAIRY ──┬── CREAM        SAUCE ──┬── CREAM_SAUCE
                                     └── BUTTER               └── TOMATO_SAUCE

roots without children:
BONE · BREAD · BROTH · CASING · DOUGH · EGG · FAT · FISH · MUSHROOM · PASTA
PRIMAL_CUT · SWEETENER · CANNED_MEAT · CANNED_FISH · CANNED_FRUIT
```

### Ingredient States

Ten states. Each one projects onto a vanilla food stage, so a ChefZ item never confuses
the engine's own cooking model.

```text
RAW → PREPARED → COOKED · BAKED · FRIED
  ↘ SALTED · SMOKED · DRIED        preserved, each with its own freshness lifetime
  ↘ BURNT · ROTTEN                 not edible; ROTTEN is terminal
```

### Food Tags

Beyond categories, items carry tags — consumed by recipe matching, grade rules, buffs,
Terje skills and medicine, traders, quests and achievements. Nineteen of them:

```text
CHEFZ_DAIRY       CHEFZ_EGG         CHEFZ_BAKED_GOOD  CHEFZ_GRAIN
CHEFZ_HERB        CHEFZ_WILD_HERB   CHEFZ_SPICE       CHEFZ_SALT
CHEFZ_HIGH_PROTEIN CHEFZ_HOT_MEAL   CHEFZ_PREMIUM     CHEFZ_RAW_MEAT
CHEFZ_RAW_SAUSAGE CHEFZ_WILD_MEAT   CHEFZ_PRESERVED   CHEFZ_FRESH
CHEFZ_CREAMY      CHEFZ_MUSHROOM    CHEFZ_SAUCE_BASE
```

## Recipe Definition

Recipes are JSON, not hardcoded. New dishes ship as config, not as Core edits. Trimmed
from the real `RCP_ChefZ_SausagePasta` — optional slots and grade rules removed:

```json
{
  "id": "RCP_ChefZ_SausagePasta",
  "contexts": [
    { "deviceClasses": ["FryingPan", "Pot"], "methods": ["BAKING", "BOILING"] }
  ],
  "completion": "TIMED",
  "cookSeconds": 200,
  "minTemperature": 60,
  "slots": [
    { "slotId": "pasta",   "match": { "category": "PASTA" },   "minCount": 1, "maxCount": 3, "consume": "whole" },
    { "slotId": "sausage", "match": { "category": "SAUSAGE" }, "minCount": 1, "maxCount": 3, "consume": "whole", "gradePoints": 1 },
    { "slotId": "salt",    "match": { "category": "SALT" },    "minCount": 1, "maxCount": 1,
      "optional": true, "unit": "GRAM", "amount": { "min": 5 }, "consume": "amount",
      "consumeAmount": 5, "gradePoints": 2 }
  ],
  "policy": { "extraItems": "forbid", "forbiddenStates": ["BURNT", "ROTTEN"] },
  "qualityTierSet": "DISH_DEFAULT",
  "outputs": [
    {
      "cls": "ChefZ_SausagePasta",
      "quantity": 200, "quantityMode": "fixed",
      "returnContainer": "ChefZ_EmptyPlate",
      "setState": "COOKED"
    }
  ],
  "effects": ["CHEFZ_WARM_MEAL", "CHEFZ_HEARTY_MEAL"],
  "nutritionModifier": 1.1,
  "priority": 0
}
```

`gradePoints` on a slot and the `gradeRules` block are what turn a pot of ingredients
into a quality tier: points are scored, the score picks the tier, the tier scales yield,
spoilage and portions.

## Configuration

Configuration arrives in three ranks, and the later rank patches the earlier one.

| Rank | Source | Read by |
|---|---|---|
| 1 | `CfgChefZ*` nodes in `config.cpp` | client **and** server |
| 2 | `Config/*.json` inside the PBOs | client **and** server |
| 3 | `$profile:ChefZ\Core.json` and `$profile:ChefZ\Overlay\*.json` | **server only** |

On first server start ChefZ creates its profile folder:

```text
$profile:ChefZ\
├── Core.json          copied from ChefZ_Core/Config/Templates/Core.overlay.json
├── README.txt         what the overlay may and may not do
├── Overlay\           operator patches, one file per registry
└── Logs\              ChefZ_YYYY-MM-DD.log, load_report.txt
```

Never edit `ChefZ_Core/Config/Core.json` — it lives inside a PBO and every update
overwrites it. The overlay is the supported door, and it patches per key: write only what
you want to change. Nested blocks, however, are replaced whole.

The overlay may not extend sync-relevant registries. That is not a convention — the
config manager enforces it at runtime, and the design document says why.

With debug logging enabled, every recipe check is traceable:

```text
[ChefZ] Recipe check started
[ChefZ] Device: CookingPot
[ChefZ] Found ingredient: ChefZ_DriedPasta
[ChefZ] Found ingredient: ChefZ_PorkSausage
[ChefZ] Recipe match: RCP_ChefZ_SausagePasta
[ChefZ] Quality: SEASONED
[ChefZ] Result created
```

Full reference: [Configuration](ChefZ_Wiki/Configuration.md).

## Addon Structure

Every addon under `Psyerns_ChefZ_Core/Addons/` follows the same shape:

```text
ChefZ_Meat/
├── $PREFIX$                    ChefZ_Meat — must equal the folder name
├── config.cpp                  CfgPatches, CfgVehicles, CfgChefZ* …
├── include.txt                 what AddonBuilder packs
├── Config/                     JSON defaults for this module (rank 2)
│   ├── Ingredients/Meat.json
│   ├── Processing/Meat.json
│   └── Recipes/Sausage.json
├── Scripts/
│   └── 4_World/ChefZ/Meat/     only the layers the module actually uses
└── stringtable.csv             beside config.cpp, not one folder down
```

Asset folders appear only where a module ships its own. Three do:
`ChefZ_Farming/Sounds/` holds the two beekeeping `.ogg` files, bound through
`CfgSoundShaders` and `CfgSoundSets`, and `ChefZ_Devices` and `ChefZ_Items` are
nothing *but* assets — `models/` and `data/`, no class, no script, no record. Every
other ChefZ item still points at a vanilla proxy model until the backlog is worked
off.

Two rules here are not cosmetic, and both fail **silently**:

- **The prefix file must equal the folder name.** If it and the `files[]` paths in
  `config.cpp` disagree, DayZ skips the script modules without a word in the RPT.
- **`include.txt` is a whitelist, not a formality.** AddonBuilder packs only the
  extensions listed there. `ChefZ_Farming` had to add `*.ogg` to ship its sounds;
  `ChefZ_Ingredients`, `ChefZ_Processing` and `ChefZ_Registry` carry `*.hpp` for the
  same reason. A file type nobody listed is dropped from the PBO and nothing says so.

## Registry Delta Protocol

The four central registries — `Categories.json`, `Tags.json`, `Nutrition.json`,
`Preservation.json`, all under `ChefZ_Registry/Config/` — have exactly **one** writer.
Content modules never touch them. They emit a delta into `_deltas/<slice>.json` instead,
and a single integrator merges all deltas deterministically, rejecting collisions rather
than silently overwriting them. Fifteen slices have handed one in.

```json
{
  "slice": "meat",
  "categories": [
    { "id": "MEAT",    "parent": null,   "displayName": "#STR_CHEFZ_CAT_MEAT" },
    { "id": "SAUSAGE", "parent": "MEAT", "displayName": "#STR_CHEFZ_CAT_SAUSAGE" }
  ],
  "tags": [
    { "id": "CHEFZ_RAW_MEAT" }
  ],
  "processes": [
    { "id": "PROCESS_CUT_MEAT", "tool": "CUTTING_TOOL", "durationSec": 4 }
  ],
  "nutrition": [
    { "class": "ChefZ_MincedMeat", "energy": 150, "water": 40, "stomach": 110 }
  ],
  "preservation": [
    { "id": "SMOKED", "scope": "state", "spoilageMultiplier": 0.25 }
  ],
  "classes": ["ChefZ_MeatItemBase", "ChefZ_BeefLeg", "ChefZ_MincedMeat"]
}
```

The merge checks ID collisions across slices, verifies that every `parent` category and
every referenced `station` exists after merging, then writes the central registries in a
stable order — same input, same output. `deltas.mjs` re-checks all of it, including
whether a merge actually reached the registry it claims to be in.

Processes are the one section that stays with its slice: the registry would re-declare
them at the same rank, and a duplicate record of the same rank is rejected rather than
patched.

## Static Validation

Nineteen checkers plus a runner under `tools/chefz-validate/` — Node, no dependencies,
non-zero exit code on failure.

```bash
node tools/chefz-validate/index.mjs       # 0 errors, 20 warnings, 19/19 green
node tools/chefz-validate/selftest.mjs    # do the checkers still see?
```

**Form of the files**

| Validator | Checks |
|---|---|
| `schema.mjs` | Every ChefZ JSON against the bundled schemas — mandatory fields, types, duplicate IDs, unknown fields (typos) |
| `configcpp.mjs` | `CfgPatches` present and unique, `requiredAddons` set, `units[]` complete, no class defined twice, every `modded class` named |
| `classrefs.mjs` | Every class referenced from JSON and every parent class exists — in the project, in a delta, or in the reference index |
| `naming.mjs` | `ChefZ_PascalCase`; no collision with foreign classes |
| `stringtable.mjs` | Every `#STR_CHEFZ_*` is defined; no duplicates; the full column set present |
| `deltas.mjs` | ID collisions between slices, parent categories, category cycles, and whether a merge actually reached the central registries |

**Meaning of the content**

| Validator | Rule |
|---|---|
| `chefzsym.mjs` | Every symbol in JSON and `CfgChefZ*` exists in the merged registries; closed value lists are checked too |
| `chefzcore.mjs` | Inside `ChefZ_Core`: no foreign-system name, no content identifier, no content enumeration, no content record |
| `chefznut.mjs` | Every edible result has `class Nutrition` or `class Food` and `scope != 0` — otherwise the bite is silently ineffective |
| `chefzstage.mjs` | Every cookable ChefZ class declares `FoodStageTransitions` — otherwise it burns in the pot |
| `chefzproc.mjs` | `HANDCRAFT` transforms: 1–2 inputs, tool only with a single input, at most 10 outputs |
| `chefzlog.mjs` | No unguarded `ChefZ_Log.Debug/Trace` call inside a loop |

**The central design rule, checked mechanically**

| Validator | Rule |
|---|---|
| `chefzvanilla.mjs` | A ChefZ recipe that can be satisfied *entirely with vanilla ingredients* would hijack vanilla cooking. The build refuses to contain one |
| `chefzcookable.mjs` | The declared cooking path against the switch that is actually on — dead `FoodStageTransitions`, `CanBeCooked()` without food stages, food without an eat action |

**The hard language rules** — each one cost a build-and-start cycle before it existed

| Validator | Rule |
|---|---|
| `enforce.mjs` | Ten hard Enforce rules — no ternary, no `GetGame()` since 1.29, no `var`/`auto`, no `?.`/`??`, no parent on a `modded class`, `ref` only on members … |
| `chefzbase.mjs` | A parent class must resolve inside its own `config.cpp` — otherwise DayZ aborts with *Undefined base class* |
| `chefzmanaged.mjs` | Anything held by `ref` must be `Managed`, or the object is freed under the pointer |
| `chefzswitch.mjs` | A `case` label must be a literal — a `static const` label compiles and then matches nothing |
| `chefzaction.mjs` | An action class nobody registers in `RegisterActions()` compiles, logs nothing, and never appears in the game |

`index.mjs` runs all nineteen, writes a JSON report and sets the exit code.

### The checkers are themselves checked

`selftest.mjs` builds a throwaway module that violates every rule on purpose, runs the
suite against it, and asserts that each rule fires and the run exits `1`. Current
coverage: **18 of 19 checkers provably fire.** `chefzaction` is not covered — it could
go blind without anything noticing.

`refindex/` carries the class indexes for **Terje**, **Community Framework**, **COT**,
**Dabs Framework**, **Expansion**, the **vanilla script classes** and the **vanilla item
classes** — 16,352 foreign class names. `build-refindex.mjs` rebuilds them from any
folder containing `config.cpp` files.

> **Known limit, stated plainly:** `refindex/vanilla-classes.txt` is no longer the empty
> stub it was through Milestone 1 — it now carries the 184 vanilla item classes derived
> from the project's own asset list, which itself came from `types.xml`. It is still not
> the complete set. A server-side class dump remains the better source, and until then
> `classrefs.mjs` and `naming.mjs` report unknown classes as a **warning**, not an error.

Static validation cannot check runtime behaviour, cooking logic, in-game balancing or
model correctness. That is exactly what the gates are for — and why the sentence at the
top of this README says the mod has never kept a server running.

## Build Workflow

ChefZ is built by a defined agent crew with **one writing owner per file** and four human
approval gates.

| Milestone | Form | Done when |
|---|---|---|
| **M1 — Core Foundation** | Design panel → judge → sequential implementation | Mod loads without RPT errors · debug log shows a full recipe check · **vanilla cooking still works when no ChefZ recipe matches** · `$profile:ChefZ/` is created |
| **M2 — Base Production** | 6 vertical slices in parallel → delta merge → validate → balance review | All base chains run in-game · all stations work · no ID collision in the merge |
| **M3 — Preservation, Serving, Dishes** | 6 slices in parallel → merge → validate → adversarial review | 20 plates + 5 bowls cookable · `SIMPLE`→`PREMIUM` grading works · portion take-out works · preservation matrix correct |
| **M4 — Compatibility** | 3 fully parallel mods in disjoint folders | ChefZ runs unchanged **without** Terje · XP granted correctly **with** Terje · no duplicated Metabolism XP · COT spawns all ChefZ items |
| **M5.1 — Cookbook** | Single slice | Recipe knowledge, RPC and persistence — no UI |

**M2 slices:** `grain` · `salt` · `herbs` · `meat` · `dairy` · `produce`
**M3 slices:** `preservation` · `serving` · `sauces` · `dishes-a` · `dishes-b` · `dishes-c`

All four gate reports are written. **Gate 4 closed as NOT READY**, and the two findings
that produced that verdict have since been worked on: `ChefZ_ButterChurn` and
`ChefZ_CheesePress` no longer inherit from `Pot` and `Cauldron`, and cooking now carries
a `ChefZ_CookActor` so an XP award has somebody to award to. Neither fix has been
confirmed in a running game — that is what the gate is for.

The delta merge is the only real barrier in the whole workflow — collision detection needs all
deltas at once. Everywhere else the slices pipeline without locking.

### The Agent Crew

| Agent | Role |
|---|---|
| `chefz-architect` | Designs Core systems — delivers design notes with class and method signatures, not finished code |
| `chefz-core-engineer` | Implements `ChefZ_Core` in Enforce Script. Contains not a single concrete ingredient or dish |
| `chefz-content-engineer` | Builds one vertical slice end-to-end; instantiated in parallel per slice |
| `chefz-registry-integrator` | The only writer of the central registries. Merges deltas, resolves collisions |
| `chefz-terje-comp-engineer` | Builds the two Terje compatibility mods. Never modifies Terje files |
| `chefz-cot-comp-engineer` | Builds the COT compatibility mod, registers ChefZ items in COT spawn categories |
| `chefz-asset-tracker` | Assigns vanilla proxy models, maintains the asset backlog and shared-mesh strategy |
| `chefz-doc-scribe` | Writes gate reports and the changelog |
| `chefz-validator` *(read-only)* | Runs the validators, maps each error to a module and a responsible agent |
| `chefz-conflict-scout` *(adversarial)* | Hunts class-name collisions, duplicate `modded class` overrides, `requiredAddons` mismatches, load-order traps, client/server split errors |
| `chefz-balance-reviewer` *(adversarial)* | Checks numbers against the docs and hunts exploit loops — XP through insert/remove, recycling loops, unlimited batch XP, infinite ingredient conversion |

### At Every Gate

1. Read the gate report — what was built, full validator output, open points, asset backlog status, and an in-game test checklist
2. Build and sign the PBOs with DayZ Tools
3. Deploy to the test server, work through the in-game checklist
4. Report back: approved, or a defect list — the milestone reruns with a correction brief

### Rules That Bind Every Agent

1. **The Core stays Terje-free.** No Terje reference in `ChefZ_Core` in any form — no `#ifdef`, no optional call, no class name.
2. **ChefZ never blocks vanilla cooking.** Explicitly verified at Gate 1.
3. **Content goes in modules, systems go in the Core.**
4. **Central registries only via deltas.**
5. **Terje and vanilla files are never modified**, only extended.
6. **XP only on successful completion**, never when an action starts.
7. **Naming convention:** `ChefZ_PascalCase`.
8. **No success claim without evidence.** "Validation green" only with an actual exit code 0 in the report.
9. **Foreign repos are read sources**, never work folders.

## Terje Compatibility

ChefZ does **not** rebuild Terje. Terje already has skills, XP, perks, modifiers, nutrition,
food poisoning, immunity, medical consumable effects, crafting conditions, persistence and a
skill UI — and its registry reads skills and perks dynamically from `CfgTerjeSkills`, so an
external module can inject its own perks into an existing Terje skill.

| Domain | Owner |
|---|---|
| Cooking, processing, preserving | ChefZ → **Terje Survival** XP |
| Butchering animals | **Terje Hunting** — no ChefZ XP |
| Filleting fish | **Terje Fishing** — no ChefZ XP |
| Eating | **Terje Metabolism** — automatic, no ChefZ XP |
| Food poisoning, immunity | **Terje Medicine** |
| Herb knowledge | New ChefZ perk `chefzherb` (Herbalist, 5 levels) under Survival |

### The XP Matrix

Implemented, not proposed. Every number lives in `CfgChefZTerjeSkills` in the module's
`config.cpp` and is overridable by the operator through
`$profile:TerjeSettings\Core\GameOverrides.xml` — there is no hard-coded XP value
anywhere in the module's scripts.

**Cooking** is classified by how many ingredient entries were actually consumed:

| Consumed entries | Class | Survival XP |
|---:|---|---:|
| ≤ 2 | simple meal | 3 |
| 3–5 | complex meal | 8 |
| > 5 | premium dish | 15 |

**Processing** is keyed on the process ID:

| XP | Processes |
|---:|---|
| 1 | `CUT_MEAT` · `CARVE_BOWL` · `CARVE_PLATE` |
| 2 | `GRIND_MEAT` · `GRIND_HERB` · `GRIND_SPICE` · `SEPARATE_CREAM` · `ROLL` · `DRY_SALT` · `SALT_CURE` |
| 3 | `MILL` · `KNEAD` · `CHURN_BUTTER` · `PRESS_CHEESE` · `BOIL_BRINE` · `DRY` · `SMOKE` |
| 5 | `STUFF_SAUSAGE` · and the transforms `TR_DoughToRawPasta`, `TR_FishToSmoked` |

Eighteen of the 31 process IDs in the data set are named there. The other thirteen —
beekeeping, the container crafts, the pasta machine — fall through to `defaultXp = 1`.
That is the fallback working as designed, not a gap that breaks anything, but it does
mean raising a hive pays the same as folding a box.

**Harvesting herbs pays nothing any more.** Since 29.08.2026 herbs are *found*, like
vanilla mushrooms — no plants, no seeds, no `Harvest()` to hook. What survives of the
herbalist perk is the highlight on herbs lying in the world.

XP is granted only after completed production — never on starting an action — and batch
processing is explicitly damped against XP farming.

Full reference, including the double-XP guard and every config key:
[Terje Compatibility](ChefZ_Wiki/Terje-Compatibility.md).

## Naming Convention

All ChefZ classes use the `ChefZ_PascalCase` prefix. Where vanilla already has the item,
ChefZ uses the vanilla class instead of inventing one — milk is `PowderedMilk`, fresh
paprika is `GreenBellPepper`, honey is `Honey`, and the sausage casing comes from
`SmallGuts`.

```text
ChefZ_Wheat              ChefZ_Cream              ChefZ_MincedMeat
ChefZ_Flour              ChefZ_Butter             ChefZ_MincedPork
ChefZ_Dough              ChefZ_Cheese             ChefZ_MincedVenison
ChefZ_RawPasta           ChefZ_Egg                ChefZ_MincedBoar
ChefZ_DriedPasta                                  ChefZ_MincedChicken
ChefZ_Bread              ChefZ_RawSalt            ChefZ_MincedBear
ChefZ_Flatbread          ChefZ_Salt
                                                  ChefZ_RawSausage      ChefZ_CookedSausage
ChefZ_Parsley            ChefZ_PepperBerries      ChefZ_RawPorkSausage  ChefZ_PorkSausage
ChefZ_Corn               ChefZ_DriedPeppercorns   ChefZ_RawVenisonS…    ChefZ_VenisonSausage
ChefZ_Thyme              ChefZ_BlackPepper        ChefZ_RawBoarSausage  ChefZ_BoarSausage
ChefZ_Rosemary           ChefZ_DriedPaprika       ChefZ_RawHunterS…     ChefZ_HunterSausage
ChefZ_WildGarlic         ChefZ_PaprikaPowder      ChefZ_RawSpicyS…      ChefZ_SpicySausage
ChefZ_DriedParsley …     ChefZ_HerbMix
                         ChefZ_HunterSeasoning    ChefZ_SmokedSausage   ChefZ_DrySausage
```

The 20 V1 plate dishes:

```text
ChefZ_SurvivorSpaghetti   ChefZ_HunterPlate           ChefZ_FarmersBreakfast
ChefZ_SausagePasta        ChefZ_BloodSausagePlate     ChefZ_CheeseFlatbread
ChefZ_HunterPasta         ChefZ_FishPotatoPlate       ChefZ_SausageBreadPlate
ChefZ_CreamMushroomPasta  ChefZ_BeanSausagePlate      ChefZ_MushroomPan
ChefZ_MacAndCheese        ChefZ_TacticalBreakfast     ChefZ_PotatoPancakes
ChefZ_SausagePotatoes     ChefZ_ScrambledEggSausage   ChefZ_MeatDumplings
                                                      ChefZ_MilkRice
                                                      ChefZ_HoneyBreadPlate
```

Every dish is one class, listed above: it comes out of the pot ready to eat, with
`quantity = 100 × portions` (vanilla rates nutrition per 100 units).

## Roadmap

V1 is content-complete. What separates it from a release is a server that stays up, then
the four gate checklists run in a live game.

| Version | Contents | State |
|---|---|---|
| **V1 — Cooking Core** | Ingredients · 20 plate dishes · 5 stews · sauces · herbs · salt · pepper · paprika · sausage · dairy · baking · preservation · beekeeping · 11 stations | Written, not yet running |
| **V1.1 — Cookbook UI** | The knowledge layer exists (Milestone 5.1); the UI on top of it does not | Next |
| **V2 — Preservation depth** | Mason jars · canning · pickling · curing · deeper shelf-life model · larder economy | Planned |
| **V3 — Progression** | Rare recipes · recipe rarities · DME signature meals · quest and event recipes | Planned |

### Deliberately Out of Scope for V1

Fermentation · canning · pickling · a separate Cooking skill · deep hygiene simulation ·
farmed herbs (they are found, not grown) · cut vegetables · yeast · 3D asset production
(vanilla proxies plus a backlog until then) · in-game balancing.

## Installation

### Requirements

| | |
|---|---|
| **DayZ** | 1.29+ |
| **Dependencies** | None — `ChefZ_Core` is standalone |
| **Optional** | [TerjeMods](https://github.com/TerjeBruoygard/TerjeMods) (Skills / Medicine) · Community Online Tools |

> **Not server-ready.** The mod packs and starts, and then the server dies in the mission's
> `OnInit` chain. Nothing here has been signed or binarised, and no gate checklist has been
> run in a live game. [Known Limitations](ChefZ_Wiki/Known-Limitations.md) is the inventory;
> [Installation](ChefZ_Wiki/Installation.md) is the full operator guide.

### Packing

```bash
node tools/chefz-pack/pack.mjs            # 15 sources, unsigned and unbinarised
powershell tools/chefz-pack/testrun.ps1   # start the test server, read its verdict, stop it
```

> **The four asset addons do not pack today.** `pack.mjs` requires `$PREFIX$` to equal
> the folder name and skips the addon otherwise (`pack.mjs:85`). `ChefZ_Devices`,
> `ChefZ_Food`, `ChefZ_Items` and `ChefZ_Plants` all carry two-level prefixes of the
> form `ChefZ<name>`, which is exactly what the model paths in the content addons
> point at — config and prefix agree, the packer's rule does not. The result is 13 PBOs
> out of 17 sources, and the content addons require four addons that were never built.
> It started as two addons on 29.08. and doubled with the second delivery. Either the
> prefixes and the model paths move to one level, or the rule learns about multi-level
> prefixes while still proving that prefix and paths match.

`Psyerns_ChefZ_Core` packs to twelve PBOs, one per addon folder; the three compatibility
mods pack to one each. The dependency graph inside the main mod is closed — `ChefZ_Cooking`
requires seven other ChefZ addons and `ChefZ_Registry` requires eight — so the main mod
cannot ship as a subset. Treat it as one indivisible workshop item.

### Packing — PBO Prefix

Each addon folder carries a `$PREFIX$` equal to its folder name, and the script module paths in
`config.cpp` must use the **same** root. If prefix and `files[]` disagree, DayZ **silently skips**
the script modules — nothing is logged to the RPT, and dependent modules then fail at compile
time with errors that point at the consumer rather than the real cause.

Check after every build that the PBO header's `prefix` property and the `files[]` paths inside
`config.bin` share the same root.

## The Vision

ChefZ turns food in DME from a consumable into a system. Roasting meat stays fast and simple —
ChefZ rewards the players who go further.

```text
Raw Meat → Cooked Meat                    fast, simple, still works

Animal → Butcher → Meat → Grind → Minced Meat → Season → Sausage
      → Smoke → Combine with Potatoes → Served Dish        more work, better food,
                                                           longer shelf life, buffs,
                                                           trade value, immersion
```

The full gameplay chain:

```text
HUNT / FISH / FARM / FORAGE
↓
RAW MATERIALS
↓
PROCESSING
↓
SPICES / HERBS
↓
COOKING
↓
PRESERVATION
↓
SERVING
↓
EAT / STORE / TRADE
```

## Credits

<p align="center">
  <b>Author:</b> <a href="https://steamcommunity.com/profiles/76561198043039918/">Psyern</a><br><br>
  <b>Community:</b> <a href="https://deadmansecho.com">Deadmans Echo</a><br><br>
  Concept inspired by the clean, well-working logic of <b>CookZ</b>.<br>
  Terje compatibility is designed against <b>TerjeMods</b> by <b>TerjeBruoygard</b> — extended, never modified.
</p>

## License

Psyerns ChefZ is licensed under the **MIT License** — see [`LICENSE`](LICENSE).

Third-party components retain their own licenses. Terje and vanilla files are never modified or
redistributed — the compatibility modules only extend them.
