# Recipes

How a ChefZ recipe is written, and how the engine picks one out of the pot.

Everything on this page is taken from the shipped code and data, not from the
planning documents. File references point at the repository root
(`Psyerns_ChefZ/`) so you can check every claim.

Related pages: [Recipe-Reference](Recipe-Reference), [Adding-Content](Adding-Content),
[Architecture](Architecture), [Food-States](Food-States),
[Quality-and-Nutrition](Quality-and-Nutrition),
[Portions-and-Containers](Portions-and-Containers), [Validation](Validation),
[Known-Limitations](Known-Limitations).

## 1. The one rule that matters most

**If no recipe binds, vanilla cooking happens and nothing else does.**

This is structural, not merely intended. The only hook into the cooking path is
`modded class Cooking` in
`Psyerns_ChefZ_Core/Addons/ChefZ_Core/Scripts/4_World/ChefZ/Cooking/ChefZ_ModdedCooking.c`:

```c
override int CookWithEquipment(ItemBase cooking_equipment, float cooking_time_coef = 1)
{
    int vanillaResult = super.CookWithEquipment(cooking_equipment, cooking_time_coef);
    ...
    return vanillaResult;
}
```

`super` is the first statement, is called unconditionally, and its return value is
handed back unchanged. The ChefZ observer (`ChefZ_CookingHook.AfterVanillaCook`)
has **no return channel** — it cannot undo the tick that vanilla already
performed. If the entire ChefZ side fails (broken config, empty recipe set,
exception mid-evaluation), the tick is indistinguishable from a server without
ChefZ.

The matching side agrees. `ChefZ_RecipeEngine.EvaluateBest()`
(`.../Scripts/3_Game/ChefZ/ChefZ_RecipeEngine.c`) ends its candidate loop with:

> *"Kein Treffer. Das ist der HAEUFIGSTE Ausgang und ausdruecklich kein Fehler:
> Vanilla hat zu diesem Zeitpunkt bereits gekocht, die Zutaten garen normal
> weiter."*

("No match. This is the most common outcome and expressly not an error.")

A returned `false` carries a diagnosis (`failedRecipe`, `failReason`,
`failSlotId`) but never a side effect. `ChefZ_RecipeEvaluator.Evaluate()`
documents the same thing: there is no return value for "error", because an error
is a reason *not* to apply the recipe — and that is exactly `false`.

Practical consequences you should expect on a live server:

* Ingredients that no ChefZ recipe wants cook, burn and rot exactly as in vanilla.
* A single unwanted item in the pot is enough to fall back to vanilla, because
  the default policy is `extraItems: "forbid"` (see §6).
* A typo in a recipe file makes that recipe unavailable, not the cooking system
  broken.

## 2. Where recipes live

Recipes are JSON records of `"kind": "recipe"`. As shipped:

| File | Records |
|---|---|
| `Psyerns_ChefZ_Core/Addons/ChefZ_Cooking/Config/Recipes/BowlDishes.json` | 12 |
| `.../ChefZ_Cooking/Config/Recipes/Dishes_A.json` | 10 |
| `.../ChefZ_Cooking/Config/Recipes/DishesB.json` | 10 |
| `.../ChefZ_Cooking/Config/Recipes/Sauces.json` | 4 |
| `.../ChefZ_Meat/Config/Recipes/Sausage.json` | 6 |
| `.../ChefZ_Baking/Config/GrainRecipes.json` | 2 |

Station-driven conversions are a separate record kind (`"kind": "transform"`),
described in [Processing-Stations](Processing-Stations) and
[Food-States](Food-States).

Recipes are read as a **raw form** (`ChefZ_RecipeDef`,
`.../Scripts/1_Core/ChefZ/ChefZ_RecipeDef.c`) and then compiled into
`ChefZ_CompiledRecipe`. Evaluation *never* runs on the raw form. Raw records are
allowed to be incomplete; compiled ones are not.

## 3. The worked example

`RCP_ChefZ_HunterStew` from
`Psyerns_ChefZ_Core/Addons/ChefZ_Cooking/Config/Recipes/BowlDishes.json`. It uses
almost every mechanism on this page: device context with a liquid requirement,
`anyOf` matching, category and tag matching, required and optional slots,
partial consumption by amount, grade points, grade rules, a policy, and a
portioned output.

```json
{
    "id": "RCP_ChefZ_HunterStew",
    "priority": 0,
    "qualityTierSet": "DISH_DEFAULT",
    "nutritionModifier": 1.1,
    "completion": "ON_STAGE",
    "doneStages": ["Boiled"],
    "contexts": [
        {
            "deviceClasses": ["Pot", "Cauldron"],
            "methods": ["BOILING"],
            "requiresLiquid": true,
            "liquidTypes": ["Water"],
            "liquidQuantity": { "min": 200.0 }
        }
    ],
    "slots": [
        { "slotId": "meat",      "match": { "category": "MINCED_MEAT" }, "minCount": 2, "maxCount": 3, "consume": "whole", "gradePoints": 0 },
        { "slotId": "roots",     "match": { "category": "ROOT_VEGETABLE" }, "minCount": 2, "maxCount": 4, "consume": "whole", "gradePoints": 0 },
        { "slotId": "mushrooms", "match": { "category": "MUSHROOM" },       "minCount": 2, "maxCount": 4, "consume": "whole", "gradePoints": 0 },
        { "slotId": "herb",      "match": { "tag": "CHEFZ_HERB" },          "minCount": 1, "maxCount": 2, "optional": true, "consume": "whole", "gradePoints": 2 },
        { "slotId": "salt",      "match": { "category": "SALT" },           "minCount": 1, "maxCount": 1, "optional": true, "unit": "GRAM", "amount": { "min": 6.0 }, "consume": "amount", "consumeAmount": 6.0, "gradePoints": 1 },
        { "slotId": "spice",     "match": { "tag": "CHEFZ_SPICE" },         "minCount": 1, "maxCount": 2, "optional": true, "consume": "whole", "gradePoints": 1 }
    ],
    "gradeRules": [
        { "ruleId": "GR_HS_Thyme",      "when": "anyItem", "selector": { "anyOf": [ { "cls": "ChefZ_Thyme" }, { "cls": "ChefZ_DriedThyme" } ] }, "points": 1.0 },
        { "ruleId": "GR_HS_Seasoning",  "when": "anyItem", "selector": { "cls": "ChefZ_HunterSeasoning" }, "points": 2.0 },
        { "ruleId": "GR_HS_FreshHerbs", "when": "anyItem", "selector": { "allOf": [ { "tag": "CHEFZ_HERB" }, { "freshness": { "min": 0.8 } } ] }, "points": 1.0 }
    ],
    "policy": {
        "extraItems": "forbid",
        "forbiddenStates": ["BURNT", "ROTTEN"],
        "minMatchedHealth01": 0.15,
        "liquidConsumed": 200.0
    },
    "outputs": [
        {
            "cls": "ChefZ_HunterStewBowl",
            "quantity": 400, "quantityMode": "fixed",
            "returnContainer": "ChefZ_EmptyBowl",
            "inheritQuality": true,
            "setState": "COOKED",
            "inheritFreshness": true, "freshnessCarry": 0.9,
            "inheritTemperature": true,
            "effects": ["CHEFZ_WARM_MEAL", "CHEFZ_HYDRATED", "CHEFZ_HUNTERS_MEAL"]
        }
    ]
}
```

## 4. Slots

A slot is one requirement of the recipe. Fields are declared in
`class ChefZ_SlotDef` (`.../Scripts/1_Core/ChefZ/ChefZ_Selector.c`, line 257).

| Field | Meaning | Default (`ResolveDefaults()`) |
|---|---|---|
| `slotId` | stable name; appears in traces and in grade rules | — (required in practice) |
| `match` | the selector, see §5 | — |
| `minCount` | how many items are required | `1` |
| `maxCount` | how many may be bound at most | `= minCount` |
| `amount` | range in **recipe units** (not vanilla quantity) | unset |
| `unit` | required quantity unit, e.g. `"GRAM"` | unset |
| `optional` | slot need not be filled | `false` |
| `allowPartial` | partial fill allowed | `true` |
| `consume` | `"whole"` \| `"amount"` \| `"none"` | `"whole"` |
| `consumeAmount` | units removed when `consume = "amount"` | unset |
| `setStateAfter` | state applied to what survives (`consume: "none"`) | unset |
| `gradePoints` | quality points if the slot ends up filled | `0` |
| `excludeStates` | states that disqualify a candidate | see below |

### Required vs. optional

`optional: true` is the whole difference. The engine treats it in four places:

* **Binding.** A required slot that cannot be filled fails the recipe. An
  optional one simply stays empty.
* **Completion.** `ChefZ_RecipeEvaluator.CheckStages()` only demands a done
  stage from **bound required** ingredients. This is deliberate — a pinch of salt
  has no `FoodStage`, and requiring one would mean the dish is never finished.
* **Specificity.** Required slots contribute
  `selector.specificity * min(minCount, amountCap)`; optional ones only
  `selector.specificity * wOptionalSlot` (default `0.25`).
* **Ambiguity analysis.** `ChefZ_RecipeRanker.SlotSignature()` ignores optional
  slots entirely — an extra optional spice slot does not make a recipe a
  superset of another.

In the shipped dishes, required slots carry `gradePoints: 0` and optional slots
carry the points. That is a content convention, not an engine rule: "SIMPLE is
by definition just the base ingredients." See
[Quality-and-Nutrition](Quality-and-Nutrition).

### `excludeStates` — absent is not the same as empty

From the header comment of `ChefZ_SlotDef`:

* field **absent** → the global default applies
  (`CoreSettings.defaultExcludedStates`, shipped as `["BURNT", "ROTTEN"]` in
  `.../ChefZ_Core/Config/Core.json`)
* `"excludeStates": []` → the author explicitly allows everything

Since `ref` arrays stay `null` when absent, the two cases are distinguishable
without any trick.

### Consumption modes

`ChefZ_ConsumeMode` (`.../Scripts/1_Core/ChefZ/ChefZ_CompiledSlot.c`):

| Value | Effect |
|---|---|
| `whole` | the item is deleted |
| `amount` | `consumeAmount` units are subtracted, the rest stays in the pot |
| `none` | nothing is consumed (tool / catalyst); `setStateAfter` may still apply |

An unknown name is a WARN at compile time and falls back to `whole` — never
silently to "consume nothing".

## 5. Matching: selectors

A selector is one node. Fields are declared in `class ChefZ_Selector`
(`.../Scripts/1_Core/ChefZ/ChefZ_Selector.c`).

**Leaf predicates — exactly one is set:**

| Key | Matches |
|---|---|
| `cls` | the exact class name |
| `category` | a category **including all of its subcategories** |
| `tag` | the effective tag (class + state + quality, assembled by `ChefZ_FactCollector`) |
| `state` | a ChefZ state symbol |
| `vanillaStage` | `"Raw"`, `"Baked"`, `"Boiled"`, `"Dried"`, `"Burned"`, `"Rotten"` |

> **The JSON key is `cls`, not `class`.** `class` is an Enforce keyword and a
> field of that name cannot be declared, so `JsonFileLoader` can never reach it.
> Writing `"class"` yields an empty selector and therefore a clear compile error —
> not silent match-everything. This is documented at the top of
> `ChefZ_Selector.c`.

**Combinators — recursive:**

| Key | Meaning |
|---|---|
| `anyOf` | OR over child selectors |
| `allOf` | AND over child selectors |
| `not` | negation of a single child selector |

**Value ranges — ANDed with the leaf:**
`health`, `freshness`, `temperature`, `wetness`, `cleanness`, `quantity`,
`quantityPct`, plus `minQuality` (a quality symbol) and the liquid predicates
`isLiquidContainer` / `liquidType`.

`isLiquidContainer` has no sentinel — `false` reliably means "not set". To say
"this item is *not* a liquid container", write
`{ "not": { "isLiquidContainer": true } }`.

Selector depth is bounded by `CoreSettings.maxSelectorDepth` (shipped: `8`).

### What an item is checked against, in order

`ChefZ_SlotEvaluator.Accepts()` / `AcceptsExplain()`
(`.../Scripts/1_Core/ChefZ/ChefZ_SlotEvaluator.c`), ordered by usefulness of the
resulting message:

1. selector structure (is it even the right ingredient?)
2. `excludeStates`
3. quality threshold and value ranges
4. quantity unit

The order exists so a player who put raw meat in instead of dried reads
"state RAW not allowed, DRIED required" rather than "no recipe matches".

## 6. Amounts, quantities and policy

### Amounts

`amount` and `consumeAmount` are in **recipe units**, not vanilla quantity.
`ChefZ_SlotEvaluator` sums units **over the whole slot**, not per item, with a
tolerance of `EPS = 0.0001` (float arithmetic makes "2 units required,
1.9999999 present" otherwise fail inexplicably).

### `policy`

`class ChefZ_RecipePolicy` (`.../ChefZ_RecipeDef.c`):

| Field | Meaning | Default |
|---|---|---|
| `extraItems` | `"forbid"` \| `"ignore"` \| `"consume"` | `CoreSettings.defaultExtraItems`, shipped `"forbid"` |
| `extraItemsAllowedIf` | a selector for tolerated foreign items; evaluated **before** `forbid` | unset |
| `forbiddenStates` | checked over the **entire vessel content**, not only bound items | unset |
| `minMatchedHealth01` | minimum health of **bound** ingredients | `0` |
| `liquidConsumed` | liquid removed on completion | `0` |

The asymmetry is deliberate and explained in the header of
`ChefZ_RecipeEvaluator.c`: `forbiddenStates` scans everything (otherwise it would
merely duplicate the slot-level `excludeStates`), while `minMatchedHealth01` is a
statement about what goes *into* the dish and therefore only checks bound items.

With `extraItems: "consume"`, the foreign items go into the consume plan but are
deliberately **not** counted towards coverage — otherwise a sloppy recipe that
sweeps everything up would beat a precise one.

## 7. Cooking devices and context

`contexts[]` is a list of `ChefZ_ContextRule`. A recipe binds if **any** context
rule matches:

| Field | Meaning |
|---|---|
| `deviceClasses` | exact class names (`"Pot"`, `"Cauldron"`, `"FryingPan"`) |
| `deviceCategories` | device categories |
| `methods` | cooking methods as symbols |
| `deviceTemperature` | range |
| `requiresLiquid`, `liquidTypes`, `liquidQuantity` | liquid conditions |

Cooking methods are mapped from vanilla's `CookingMethodType` in
`.../Scripts/4_World/ChefZ/Cooking/ChefZ_CookingHook.c`:
`BAKING`, `BOILING`, `DRYING`, `TIME`, plus `NONE`. The method is **asked of
vanilla each tick**, never reconstructed — it flips from `BOILING` to `BAKING`
once the water has evaporated, and vanilla refreshes it mid-run.

> All shipped dish recipes use `deviceClasses`, not `deviceCategories`. The
> reason is recorded in the header of `BowlDishes.json`: the device-category
> namespace is empty project-wide — `CfgChefZDevices` declares only
> `portionCapacity` for `Pot`/`Cauldron`/`FryingPan`. A slot pointing at a
> device category `"POT"` would point at nothing. See
> [Known-Limitations](Known-Limitations).

`requiredToolGroups[]` is checked separately (step 2b) against tools in reach;
`requires[]` (capability requirements) is step 2c and only blocks when
`onFail: "block"` — `"degrade"` and `"reduceYield"` are downgrades, not filters.
Without a capability provider loaded, `ChefZ_CapabilityGate.Denies()` always
returns false and the flow is identical to a server without the skills module.
See [Terje-Compatibility](Terje-Compatibility).

## 8. Completion: `ON_STAGE` vs. `TIMED` vs. `INSTANT`

`class ChefZ_Completion` (`.../ChefZ_RecipeDef.c`):

| Mode | Trigger |
|---|---|
| `ON_STAGE` | vanilla's `FoodStage` on the bound required ingredients — **the default** |
| `TIMED` | own counter in the cook session (`cookSeconds`, gated by `minTemperature`) |
| `INSTANT` | ready immediately |

`ChefZ_RecipeEvaluator.CheckReady()`:

* `INSTANT` → always `true`.
* `TIMED` → fails while `ctx.deviceTemperature < recipe.minTemperature`, then
  while `ctx.elapsedSec < recipe.cookSeconds`.
* `ON_STAGE` → `CheckStages()`: **every bound required ingredient** must sit in
  one of `doneStages`. Optional slots are explicitly not counted.

Compiler behaviour (`.../Scripts/3_Game/ChefZ/ChefZ_RecipeCompiler.c`):

* unknown `completion` name → WARN, falls back to `ON_STAGE`
* `TIMED` with `cookSeconds <= 0` → **recipe rejected** ("a recipe with a clock
  that never runs out is a recipe that never finishes")
* `TIMED` while `CoreSettings.allowTimedRecipes = false` → WARN, **downgraded to
  `ON_STAGE`** (operator kill switch; the recipe survives)
* `ON_STAGE` without `doneStages` → WARN, default `{Baked, Boiled, Dried}`
* `doneStages` naming an unknown vanilla stage → **recipe rejected**, because a
  typo would silently make the dish never finish
* `doneStages` empty at runtime → `CheckStages()` answers "not ready". A recipe
  without a done stage must not be permanently finished.

`ON_STAGE` needs no timer of its own: vanilla's `FoodStage` is simultaneously the
ready signal and the persisted progress display, and burnt food becomes a natural
failure condition instead of being rebuilt.

## 9. Evaluation order for one recipe

`ChefZ_RecipeEvaluator.Evaluate()`:

| Step | Check |
|---|---|
| 2a | context filter (`MatchesContext`) |
| 2b | tool groups (`HasRequiredTools`) |
| 2c | capability gate (`onFail: "block"` only) |
| 2d | slot binding (`ChefZ_Matcher.Bind`) |
| 2e | policy (`forbiddenStates`, `minMatchedHealth01`, `extraItems`) |
| 2f | fill the result (bindings, bound handles, extras, consume plan, grade score) |

`result` is filled even on failure, so `failedRecipe` / `failReason` /
`failSlotId` can be turned into a player-readable explanation. The step is
side-effect free: no `ItemBase` access, no randomness, no clock. The same input
twice gives the same output twice.

### The matcher

`ChefZ_Matcher` (`.../Scripts/1_Core/ChefZ/ChefZ_Matcher.c`) is a **backtracking**
assignment solver, not a greedy scan. Slots and items form a bipartite matching
problem; a greedy matcher produces false negatives from three overlapping slots
onwards, and "recipe does not match although it does" is the bug players notice
most and can report least.

Three guaranteed properties:

1. **Deterministic.** The fact snapshot is stably sorted; slot order on ties is
   declaration index. Same pot content → always the same binding.
2. **One item serves at most one slot.** Otherwise the same piece of meat could
   satisfy `MEAT` and `WILD_MEAT` simultaneously — a duplication exploit.
3. **Bounded.** Every binding attempt counts against
   `CoreSettings.matcherNodeBudget` (shipped: `4096`). A pathological recipe
   cannot stall the server, and the log names which one.

## 10. Priority when several recipes match

Handled by `ChefZ_RecipeRanker` (`.../Scripts/3_Game/ChefZ/ChefZ_RecipeRanker.c`).
The design decision is stated in its header: a hand-maintained `priority` number
is a globally shared namespace — with several content authors working in
parallel it cannot be kept consistent. So specificity is **computed**.

### Specificity — computed at build time, depends only on the recipe

```
specificity =
    SUM over required slots:  selector.specificity * min(minCount, amountCap)
  + SUM over optional slots:  selector.specificity * wOptionalSlot
  + wContextDeviceClass per deviceClasses entry
  + wContextBound       per bound context condition
  + wPolicyForbid       if extraItems == "forbid"
  + wPolicyPerState     per forbiddenStates entry
  + wCapability         per requires[] entry
  + wToolGroup          per requiredToolGroups[] entry
```

Selector specificity is computed by `ChefZ_SelectorCompiler.ComputeSpecificity()`
from `priorityWeights` in `Core.json` (defaults in
`.../Scripts/1_Core/ChefZ/ChefZ_PriorityWeights.c`):

| Weight | Default | Applies to |
|---|---|---|
| `wClass` | `3.00` | `cls` leaf |
| `wState` | `2.00` | `state` leaf |
| `wTag` | `1.50` | `tag` leaf |
| `wVanillaStage` | `1.50` | `vanillaStage` leaf |
| `wCategoryBase` | `1.00` | `category` leaf, plus… |
| `wCategoryPerDepth` | `0.50` | …per category depth (`WILD_MEAT` beats `MEAT`) |
| `wNot` | `0.50` | `not` node |
| `wRangePerBound` | `0.25` | per bound value range |
| `wMinQuality` | `0.75` | `minQuality` |
| `wOptionalSlot` | `0.25` | optional slot damping |
| `wContextDeviceClass` | `0.50` | per named device class |
| `wContextBound` | `0.25` | per temperature / liquid condition |
| `wPolicyForbid` | `0.50` | `extraItems: "forbid"` |
| `wPolicyPerState` | `0.25` | per `forbiddenStates` entry |
| `wCapability` | `0.25` | per `requires[]` entry |
| `wToolGroup` | `0.25` | per `requiredToolGroups[]` entry |
| `amountCap` | `3` | cap in `min(minCount, amountCap)` |
| `coverageBonus` | `0.50` | runtime coverage bonus |
| `priorityScale` | `0.01` | damping of the hand-written `priority` |

`allOf` sums its children; `anyOf` takes the **minimum** of its children — the
weakest branch decides, because the selector accepts everything that branch
accepts.

`amountCap` caps the only unbounded factor: "8× meat" must not automatically be
more specific than "1× special sausage + 1× special sauce".

### Runtime score

```
score = specificity                                   (cached)
      + coverageBonus * (bound items / items in vessel)
      + priority * priorityScale
```

The **coverage bonus** closes the most frustrating outcome: five good
ingredients turning into a two-ingredient dish with three left in the pot.
Whoever empties the kettle beats whoever leaves half of it — but only at equal
specificity, since `0.50` is deliberately smaller than any real specificity
difference.

The **damping** makes `priority` what it should be: a tiebreaker, not a lever.
`priority = 100` shifts the score by `1.0`. Every shipped dish recipe uses
`"priority": 0`.

### Ties

`ChefZ_RecipeRanker.CompareMatches()`, in order:

1. more bound items wins
2. more required slots wins
3. higher explicit `priority` wins
4. lexicographically smaller `id` wins

Step 4 is content-free and still the most important: it makes the comparison a
**total** order, so a live server and a developer's test server produce the same
dish. Nothing in the ranker touches wall clock, random numbers, or map iteration
order.

### First success wins

`ChefZ_RecipeEngine.EvaluateBest()` walks the pre-sorted candidate list and
returns on the first recipe that binds. Sorting happens at build time only — a
kettle tick does no sorting. If everything fails, the result carries the reason
of the **best-placed** failed candidate, not the last one, because that is the
recipe the player most likely meant.

### Ambiguity warnings at boot

`ChefZ_RecipeRanker.ReportAmbiguities()` reports two content problems into the
load report, both as WARN and never ERROR:

* **Tie** — two recipes with identical rank *and* identical required-slot
  signature. Only the alphabetical ID then decides; this is almost always a
  duplicate from two content slices.
* **Shadowing** — A requires everything B requires and more, but ranks lower, so
  B always binds first. The shadowed recipe **stays loaded**, since the shadowing
  may be lifted with a different device or amount.

Limits: the analysis is pairwise and therefore quadratic. It is skipped above
`MAX_PAIRWISE_RECIPES = 400` recipes (with an INFO line), and reports at most
`MAX_REPORTED_PAIRS = 20` pairs. It is explicitly **heuristic** — it compares
slot strings and device sets, not selector semantics. Two selectors that mean the
same thing but are written differently are not found. See
[Validation](Validation) and [Known-Limitations](Known-Limitations).

## 11. Outputs

`class ChefZ_OutputDef` (`.../ChefZ_RecipeDef.c`). Only the fields relevant to
recipe selection are listed here; the portioning and container fields are
described in [Portions-and-Containers](Portions-and-Containers), and
`setState` / `inheritFreshness` in [Food-States](Food-States).

| Field | Meaning |
|---|---|
| `cls` | result class (JSON key `cls`, same reason as in selectors) |
| `quantity`, `quantityMode`, `ratio` | `"fixed"` \| `"fromInput"` \| `"ratio"` |
| `chance` | 0..1, default 1 |
| `variants[]` | per-tier alternative result classes |
| `effects[]` | opaque effect IDs, never interpreted by the core |

`ChefZ_Applicator.ApplyQuantity()`
(`.../Scripts/4_World/ChefZ/Cooking/ChefZ_Applicator.c`):

* `fromInput` → the sum of vanilla quantity actually removed
* `ratio` → that sum × `ratio`
* otherwise → `quantity`, else the class default
* an unknown `quantityMode` is treated as `fixed` and reported once

A value `<= 0` is never written, because `SetQuantity` would delete an item whose
class carries `varQuantityDestroyOnMin` — a dish vanishing at the moment of its
creation is ingredient loss from the player's point of view.

`byproducts[]` use the same record type. They are checked for edibility
registration but are deliberately **not** compared against the nutrition target
value — see [Quality-and-Nutrition](Quality-and-Nutrition).

## 12. Checklist for a new recipe

1. `id` unique across all loaded modules (duplicates are reported at boot).
2. Every `cls` you name in `outputs` exists in `CfgVehicles` **and** carries its
   own `class Nutrition` / `class Food` with `scope != 0`. Otherwise the recipe
   is rejected by the compiler, and if it were not, the dish would be eaten,
   disappear and satiate nothing without any message.
3. If `completion` is `ON_STAGE`, every required ingredient class must declare
   `Food > FoodStageTransitions` — otherwise the recipe never finishes. This is
   the single most common trap; see [Food-States](Food-States).
4. If `completion` is `TIMED`, set `cookSeconds > 0`, and accept that an operator
   can switch the whole mode off.
5. Set `amountPerPortion` on any portioned output. Without it a minimal filling
   in a cauldron yields the full portion count.
6. Leave `priority` at `0` unless you have a concrete tie to break.
7. Read the boot log for shadowing and tie warnings.

## Next

- [Recipe-Cards](Recipe-Cards) — every recipe as a card, generated from these records
- [Recipe-Book](Recipe-Book) — the same recipes in prose
- [Recipe-Reference](Recipe-Reference) — every field of every record
