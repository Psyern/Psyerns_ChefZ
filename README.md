# Psyerns ChefZ

<p align="center">
  <img src="data/Psyerns_ChefZ_Banner.png" alt="Psyerns ChefZ" width="800">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/DayZ-1.29+-0074D9?style=for-the-badge&logo=steam&logoColor=white" alt="DayZ 1.29+">
  <img src="https://img.shields.io/badge/Enforce_Script-Enfusion-FF851B?style=for-the-badge" alt="Enforce Script">
  <img src="https://img.shields.io/badge/Status-Planning_%2F_Pre--Implementation-999999?style=for-the-badge" alt="Status">
  <img src="https://img.shields.io/badge/Scope-ChefZ_V1-2ECC40?style=for-the-badge" alt="Scope V1">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="License MIT"></a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Recipes-JSON_driven-E67E22?style=flat-square" alt="JSON Recipes">
  <img src="https://img.shields.io/badge/Optional-Terje_Skills_%7C_Medicine-8E44AD?style=flat-square" alt="Terje">
  <img src="https://img.shields.io/badge/Optional-Community_Online_Tools-3498DB?style=flat-square" alt="COT">
  <img src="https://img.shields.io/badge/Validation-6_static_checkers-E74C3C?style=flat-square" alt="Validators">
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

---

## Project Status

**This repository is currently the design corpus, not a shipped mod.**

The design is finished and the static validator suite under `tools/chefz-validate/` is
implemented. What does not exist yet is the mod itself: every addon folder under
`Psyerns_ChefZ_Core/Addons/` is still empty. Implementation starts at Milestone 1.

The planning corpus this README summarises — core architecture, content vision, the full V1
ingredient &amp; production map, the Terje compatibility analysis and the open design questions —
is kept internally and is not part of this repository.

| | |
|---|---|
| **Design docs** | Complete — 6 documents, ~9,100 lines |
| **Build workflow** | Defined — 4 milestones, 4 human gates, 11 agents |
| **Validators** | Implemented — 6 checkers + runner, ~1,000 lines of dependency-free Node |
| **Reference index** | Terje, CF, COT, Dabs, Expansion and vanilla scripts indexed · vanilla item classes still outstanding |
| **Build harness** | Complete — 11 agent roles and all 4 milestone scripts (local tooling, not part of this repo) |
| **`ChefZ_Core` code** | Not started — Milestone 1 |
| **Content modules** | Not started — Milestones 2–3 |
| **Compatibility mods** | Not started — Milestone 4 |
| **3D assets** | Vanilla proxy models until the asset backlog is worked off |

---

## Repository Layout

One Steam Workshop item (`Psyerns_ChefZ_Core`) containing several PBOs, plus three
independent compatibility mods that are only needed if you run Terje or COT.

```text
Psyerns_ChefZ/                              ← repository root (this README)
│
├── data/                                   ← banner, screenshots
│
├── Psyerns_ChefZ_Core/                     ← THE mod (one workshop item)
│   ├── Addons/
│   │   ├── ChefZ_Core/                     systems only — no content
│   │   ├── ChefZ_Ingredients/              base ingredients, intermediates, spices
│   │   ├── ChefZ_Farming/                  plants, herbs, seeds, harvesting
│   │   ├── ChefZ_Processing/               stations, tools, processing steps
│   │   ├── ChefZ_Meat/                     minced meat, sausages, meat products
│   │   ├── ChefZ_Preservation/             salting, drying, smoking
│   │   ├── ChefZ_Baking/                   dough, bread, pasta
│   │   ├── ChefZ_Cooking/                  plates, soups, stews, breakfast
│   │   └── ChefZ_UI/                       cookbook (V1.1+)
│   ├── Keys/
│   └── _deltas/                            registry deltas from the content agents
│
├── Psyerns_ChefZ_Terje_Skills_Comp/        ← optional mod — Survival XP, Herbalist perk
├── Psyerns_ChefZ_Terje_Medicine_Comp/      ← optional mod — herbal teas, immunity, poisoning
├── Psyerns_ChefZ_COT_Comp/                 ← optional mod — COT spawn categories
│
└── tools/chefz-validate/                   ← static validators (Node, no dependencies)
```

**`ChefZ_Core` is dependency-free** — no Community Framework, no Terje, no Expansion. The
compatibility mods are optional consumers; ChefZ runs unchanged without any of them.

---

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
- Quality system (4 tiers)
- Nutrition manager
- Preservation manager
- Tool requirement system
- Cooking device adapter
- Portion system
- Dish container system
- Event API
- Config manager
- Debug logging

</td>
<td width="33%" valign="top">

### Production Chains
- Wheat → Flour → Dough → Bread
- Dough → Raw pasta → Dried pasta
- Meat → Minced meat → Sausage
- Sausage → Smoked / dry sausage
- Saltwater → Raw salt → Salt
- Herbs → Dried herbs → Herb mix
- Pepper berries → Peppercorns → Black pepper
- Paprika → Dried paprika → Powder
- Milk → Cream → Butter / Cheese
- Fish → Salted / dried / smoked fish
- Bones → Bone broth
- Tomato / cream / mushroom sauces

</td>
<td width="33%" valign="top">

### Content Scope (V1)
- 7 crops, 5 herbs
- 6 processing stations
- 8+ sausage varieties
- 20 plate dishes
- 5 soups &amp; stews
- 4 sauces / broths
- Salt, pepper, paprika powder
- Herb mix, hunter seasoning
- Milk, cream, butter, cheese
- Preservation matrix (13 transitions)
- Portioned group meals
- Empty container return

</td>
</tr>
</table>

### Processing Stations

| Station | Process | Turns |
|---|---|---|
| `ChefZ_GrainMill` | `MILL_GRAIN` | Wheat → Flour |
| `ChefZ_MeatGrinder` | `GRIND_MEAT` | Meat → Minced meat |
| `ChefZ_Mortar` | `GRIND_SPICES`, `GRIND_HERBS` | Peppercorns → Black pepper · Dried paprika → Powder · Dried herbs → Herb mix |
| `ChefZ_DryingRack` | `DRY_*` | Herbs, pepper, paprika, pasta, meat, fish, sausage |
| `ChefZ_Smoker` | `SMOKE_*` | Meat, fish, sausage |
| `ChefZ_CuttingBoard` | `CUT` | Vegetables, meat, herbs |

### Preservation Matrix

| Input | Process | Output |
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

Spoilage is driven by state, not per-item code:

| State | Spoilage multiplier |
|---|---:|
| `RAW_MEAT` | 1.00 |
| `SALTED_MEAT` | 0.50 |
| `SMOKED_MEAT` | 0.25 |
| `DRIED_MEAT` | 0.15 |
| `CANNED_FOOD` | 0.02 |

### Dish Quality

Quality is derived from what actually went into the pot, and affects nutrition, shelf life,
buffs, trade value and the UI description.

| Tier | Reached by |
|---|---|
| `SIMPLE` | Base ingredients only — noodles + sausage |
| `PREPARED` | Base + sauce + salt |
| `SEASONED` | Above + pepper, herbs |
| `PREMIUM` | Above + high-tier meat, cream sauce, fresh herbs |

---

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

---

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

The engine checks the cooking device, the ingredients present, their amounts and states,
liquid and temperature requirements, optional ingredients and seasonings, required processing
steps, and which recipe wins when several match.

```text
Cooking Pot
│
├── Noodles
├── Sausage
├── Tomato Sauce
└── Salt

→ ChefZ_SausagePasta
```

Supported matching modes: **Exact** (`ChefZ_HunterSausage`) · **Category** (`SAUSAGE`) ·
**AnyOf** (`Potato | Rice | Pasta`) · **Optional** (`Parsley`) · **Required amount** (`2x Potato`) ·
**Quantity** (`100g Flour`).

When several recipes match, **the most specific valid recipe wins** — resolved through an
explicit `Priority` field per recipe.

### Categories

```text
MEAT ──┬── DOMESTIC_MEAT      FISH                    GRAIN ─ FLOUR ─ DOUGH ─ PASTA ─ BREAD
       ├── WILD_MEAT          SAUSAGE
       ├── POULTRY            MUSHROOM                VEGETABLE ─┬─ ROOT_VEGETABLE
       └── PREDATOR_MEAT      HERB ── DRIED_HERB                 └─ LEAF_VEGETABLE
                              SPICE                   DAIRY · FAT · LIQUID
                              BROTH · SAUCE           PLATE_DISH · BOWL_DISH · PRESERVED_FOOD
```

### Ingredient States

```text
RAW → PREPARED → COOKED
  ↘ DRIED · SMOKED · SALTED · PICKLED · FERMENTED
  ↘ BURNT · ROTTEN
```

### Food Tags

Beyond categories, items carry tags — consumed by recipe matching, buffs, Terje skills and
medicine, traders, quests and achievements.

```text
CHEFZ_HERB        CHEFZ_WILD_HERB    CHEFZ_SPICE       CHEFZ_WILD_MEAT
CHEFZ_RAW_MEAT    CHEFZ_HIGH_PROTEIN CHEFZ_HOT_MEAL    CHEFZ_FRESH
CHEFZ_DRIED       CHEFZ_SMOKED       CHEFZ_SALTED      CHEFZ_MEDICINAL
CHEFZ_FISH        CHEFZ_DAIRY        CHEFZ_PREMIUM
```

---

## Recipe Definition

Recipes are JSON, not hardcoded. New dishes ship as config, not as Core edits.

```json
{
  "RecipeID": "ChefZ_SausagePasta",
  "Priority": 100,
  "CookingDevice": [
    "CookingPot",
    "FryingPan"
  ],
  "Ingredients": [
    { "Category": "PASTA",   "Amount": 1 },
    { "Category": "SAUSAGE", "Amount": 1 },
    { "Item": "ChefZ_TomatoSauce", "Amount": 1 }
  ],
  "OptionalIngredients": [
    "ChefZ_BlackPepper",
    "ChefZ_Parsley"
  ],
  "Container": "PLATE",
  "ReturnContainer": "ChefZ_EmptyPlate",
  "Effects": [
    "CHEFZ_WARM_MEAL",
    "CHEFZ_HEARTY_MEAL"
  ],
  "Result": "ChefZ_SausagePasta"
}
```

---

## Configuration

JSON defaults ship inside the PBOs and are copied to `$profile:ChefZ/` on first server start.

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

With debug logging enabled, every recipe check is traceable:

```text
[ChefZ] Recipe check started
[ChefZ] Device: CookingPot
[ChefZ] Found ingredient: ChefZ_RawPasta
[ChefZ] Found ingredient: ChefZ_PorkSausage
[ChefZ] Recipe match: ChefZ_SausagePasta
[ChefZ] Quality: SEASONED
[ChefZ] Result created
```

---

## Addon Structure

Every addon under `Psyerns_ChefZ_Core/Addons/` follows the same shape:

```text
ChefZ_Meat/
├── $PREFIX$                    ChefZ_Meat
├── config.cpp                  CfgPatches, CfgVehicles, CfgSlots …
├── Config/                     JSON defaults for this module
│   ├── Recipes/Sausage.json
│   └── Processing/Grinding.json
├── Scripts/
│   ├── 3_Game/ChefZ/Meat/
│   ├── 4_World/ChefZ/Meat/
│   └── 5_Mission/ChefZ/Meat/
├── Data/                       textures, materials
├── Models/                     .p3d (initially empty — vanilla proxies)
└── Language/stringtable.csv
```

---

## Registry Delta Protocol

Every central registry — `Categories.json`, `Tags.json`, `Processing.json`, `Nutrition.json`,
`Preservation.json` — has exactly **one** writer. Content modules never touch them. They emit a
delta into `_deltas/<slice>.json` instead, and a single integrator merges all deltas
deterministically, rejecting collisions rather than silently overwriting them.

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

The merge checks ID collisions across slices, verifies that every `parent` category and every
referenced `station` exists after merging, then writes the central registries in a stable order
— same input, same output.

---

## Static Validation

Six checkers plus a runner under `tools/chefz-validate/` — Node, no dependencies, non-zero exit
code on failure. Implemented and ready before the first line of mod code exists.

| Validator | Checks |
|---|---|
| `schema.mjs` | All ChefZ JSONs against the bundled schemas (recipes, categories, tags, processing, nutrition, preservation, deltas) |
| `classrefs.mjs` | Every class referenced in JSON exists — in a project `config.cpp` or in the reference index |
| `configcpp.mjs` | Every addon has `CfgPatches`; no class defined twice; `requiredAddons[]` covers the base classes actually used |
| `naming.mjs` | `ChefZ_` prefix, PascalCase, no collision with vanilla or Terje names |
| `stringtable.mjs` | Every `#STR_CHEFZ_*` reference exists; no orphaned entries |
| `deltas.mjs` | ID collisions between slices; parent categories present; stations present |
| `index.mjs` | Runner — executes all, writes a JSON report, sets the exit code |

`refindex/` already carries the class indexes for **Terje**, **Community Framework**, **COT**,
**Dabs Framework**, **Expansion** and the **vanilla script classes**. `build-refindex.mjs`
rebuilds them from any folder containing `config.cpp` files.

> **Known limit, stated plainly:** `refindex/vanilla-classes.txt` — the vanilla *item* classes —
> is still an empty stub. They cannot be derived from the script sources; they need unpacked
> game data or a server-side class dump. Until that file is filled, `classrefs.mjs` and
> `naming.mjs` report unknown classes as a **warning**, not an error.

Static validation cannot check runtime behaviour, cooking logic, in-game balancing or model
correctness. That is exactly what the four gates are for.

---

## Build Workflow

ChefZ is built by a defined agent crew with **one writing owner per file** and four human
approval gates.

| Milestone | Form | Done when |
|---|---|---|
| **M1 — Core Foundation** | Design panel → judge → sequential implementation | Mod loads without RPT errors · debug log shows a full recipe check · **vanilla cooking still works when no ChefZ recipe matches** · `$profile:ChefZ/` is created |
| **M2 — Base Production** | 6 vertical slices in parallel → delta merge → validate → balance review | All base chains run in-game · all stations work · no ID collision in the merge |
| **M3 — Preservation, Serving, Dishes** | 6 slices in parallel → merge → validate → adversarial review | 20 plates + 5 bowls cookable · `SIMPLE`→`PREMIUM` grading works · portion take-out works · preservation matrix correct |
| **M4 — Compatibility** | 3 fully parallel mods in disjoint folders | ChefZ runs unchanged **without** Terje · XP granted correctly **with** Terje · no duplicated Metabolism XP · COT spawns all ChefZ items |

**M2 slices:** `grain` · `salt` · `herbs` · `meat` · `dairy` · `produce`
**M3 slices:** `preservation` · `serving` · `sauces` · `dishes-a` · `dishes-b` · `dishes-c`

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

---

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

### Proposed XP Matrix

| ChefZ action | Terje skill | XP |
|---|---|---:|
| Harvest herb | Survival | 2–5 |
| Harvest rare herb | Survival | 5 |
| Dry herbs | Survival | 3 |
| Dry pepper | Survival | 3 |
| Grind spice | Survival | 2 |
| Produce salt from saltwater | Survival | 5 |
| Mill flour | Survival | 3 |
| Make dough | Survival | 3 |
| Make pasta | Survival | 5 |
| Simple meal | Survival | 3 |
| Complex meal | Survival | 8 |
| Premium dish | Survival | 15 |
| Make sausage | Survival | 5 |
| Smoke sausage | Survival | 8 |
| Dry meat | Survival | 5 |
| Smoke fish | Survival | 5 |

XP is granted only after completed production — never on starting an action — and batch
processing is explicitly guarded against XP farming.

**Confirmed:** Core stays Terje-independent · compatibility ships as separate mods · Herbalist
becomes a new Survival perk · no separate Cooking skill for now · Safe Dinner and Wild Meat
Lover are not duplicated.

**Still open:** exact Herbalist ranges, yield calculation, XP balancing, Cook and Preserver
perks, concrete herbal medicine effects, recipe locks, server config structure.

---

## Naming Convention

All ChefZ classes use the `ChefZ_PascalCase` prefix.

```text
ChefZ_Wheat              ChefZ_Milk               ChefZ_MincedMeat
ChefZ_Flour              ChefZ_Cream              ChefZ_SausageCasing
ChefZ_Yeast              ChefZ_Butter             ChefZ_RawSausage
ChefZ_Dough                                       ChefZ_PorkSausage
ChefZ_PastaDough         ChefZ_RawSalt            ChefZ_VenisonSausage
ChefZ_RawPasta           ChefZ_Salt               ChefZ_BoarSausage
ChefZ_DriedPasta                                  ChefZ_HunterSausage
                         ChefZ_PepperBerries      ChefZ_SpicySausage
ChefZ_Paprika            ChefZ_DriedPeppercorns   ChefZ_SmokedSausage
ChefZ_DriedPaprika       ChefZ_BlackPepper        ChefZ_DrySausage
ChefZ_PaprikaPowder
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

---

## Roadmap

| Version | Contents |
|---|---|
| **V1 — Cooking Core** | Ingredients · 20 plate dishes · base recipes · herbs · salt · pepper · paprika powder · sausage · meat grinder · mortar |
| **V2 — Production** | Wheat production · flour · dough · pasta · bread · dairy processing · butter · cheese · smoking · drying |
| **V3 — Preservation** | Mason jars · canning · pickling · curing · deeper shelf-life model · larder economy |
| **V4 — Progression** | Recipe book · Cookbook UI · rare recipes · recipe rarities · DME signature meals · quest and event recipes |

### Deliberately Out of Scope for V1

Fermentation · canning · pickling · a separate Cooking skill · deep hygiene simulation ·
3D asset production (vanilla proxies plus a backlog until then) · PBO build and signing ·
server deployment and RPT analysis · in-game balancing.

---

## Installation

### Requirements

| | |
|---|---|
| **DayZ** | 1.29+ |
| **Dependencies** | None — `ChefZ_Core` is standalone |
| **Optional** | [TerjeMods](https://github.com/TerjeBruoygard/TerjeMods) (Skills / Medicine) · Community Online Tools |

> Installation instructions follow once Milestone 1 produces a loadable build. Until then this
> repository is the design source, not a server-ready mod.

### Packing — PBO Prefix

Each addon folder carries a `$PREFIX$` equal to its folder name, and the script module paths in
`config.cpp` must use the **same** root. If prefix and `files[]` disagree, DayZ **silently skips**
the script modules — nothing is logged to the RPT, and dependent modules then fail at compile
time with errors that point at the consumer rather than the real cause.

Check after every build that the PBO header's `prefix` property and the `files[]` paths inside
`config.bin` share the same root.

---

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

---

## Credits

<p align="center">
  <b>Author:</b> <a href="https://steamcommunity.com/profiles/76561198043039918/">Psyern</a><br><br>
  <b>Community:</b> <a href="https://deadmansecho.com">Deadmans Echo</a><br><br>
  Concept inspired by the clean, well-working logic of <b>CookZ</b>.<br>
  Terje compatibility is designed against <b>TerjeMods</b> by <b>TerjeBruoygard</b> — extended, never modified.
</p>

---

## License

Psyerns ChefZ is licensed under the **MIT License** — see [`LICENSE`](LICENSE).

Third-party components retain their own licenses. Terje and vanilla files are never modified or
redistributed — the compatibility modules only extend them.
