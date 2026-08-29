# Quality and Nutrition

Two subsystems that are usually confused with one another. Quality is computed
per cooking run and written onto the item. Nutrition is **not** — it is fixed at
class level by the engine, and ChefZ only audits it.

Related pages: [Recipes](Recipes), [Food-States](Food-States),
[Portions-and-Containers](Portions-and-Containers),
[Configuration](Configuration), [Validation](Validation),
[Known-Limitations](Known-Limitations), [Adding-Content](Adding-Content).

## Part 1 — Quality

### 1.1 Where the tiers live

**`CfgChefZQualityTiers` in
`Psyerns_ChefZ_Core/Addons/ChefZ_Cooking/config.cpp` (line 2952).** Exactly one
declaration project-wide.

Rank 1 (game config), not JSON, for the same reason as
[`CfgChefZStates`](Food-States): the quality tier of a dish is synced and shown
on the item, and the client is guaranteed to read Rank 1.

The tier set is named `DISH_DEFAULT`. `Core.json` names it as
`qualityScoring.defaultTierSet`, and every recipe of the `dishes-a`, `dishes-b`,
`dishes-c` and `sauces` slices points at it. Comparisons and ranks apply
**only within one tier set** (`ChefZ_QualityTierDef.tierSet`).

> The header comment in `BowlDishes.json` still says the tier set does not exist
> project-wide ("Gate-2-Befund G2-B6"). That comment is stale — it does exist, in
> `ChefZ_Cooking/config.cpp`. See [Known-Limitations](Known-Limitations).

### 1.2 The shipped tier ladder `DISH_DEFAULT`

| Tier | `rank` | `minScore` | `yieldMultiplier` | `spoilageMultiplier` | `portionBonus` | grants |
|---|---|---|---|---|---|---|
| `POOR` | 0 | `-99.0` | `0.75` | `1.25` | 0 | — |
| `SIMPLE` | 1 | `0.0` | `1.00` | `1.00` | 0 | — |
| `PREPARED` | 2 | `2.0` | `1.10` | `0.95` | 0 | — |
| `SEASONED` | 3 | `4.0` | `1.25` | `0.90` | **+1** | effect `CHEFZ_WARM_MEAL` |
| `PREMIUM` | 4 | `7.0` | `1.50` | `0.85` | **+2** | effects `CHEFZ_WARM_MEAL`, `CHEFZ_HEARTY_MEAL`; tag `CHEFZ_PREMIUM` |

`POOR` is declared even though no recipe aims at it: without it the dynamic
scoring has no room below, and a stew of old meat would be "also SIMPLE" instead
of worse. `minScore = -99.0` exists so the state penalties (§1.5) have somewhere
to push.

### 1.3 What a tier actually does — and what it cannot

**It does not change nutrition per bite.** There is no `nutritionMultiplier`
field on `ChefZ_QualityTierDef`, and that is not an oversight. The header of
`.../Scripts/1_Core/ChefZ/ChefZ_QualityTierDef.c` states it:

```
PlayerBase.Consume(data)
  -> m_PlayerStomach.AddToStomach(data.m_Source.GetType(),   // CLASS NAME
                                  data.m_Amount, foodStageType, ...)
       -> Edible_Base.GetNutritionalProfile(null, class_name, food_stage)
                                            ^^^^ the item is NULL
```

Nutritional value depends only on class × food stage. Two instances of the same
class at the same stage are nutritionally identical, regardless of quality,
ingredient freshness or origin. A nutrition vector on the item would never reach
the consumption path. A field that cannot have an effect would be a promise the
core cannot keep.

The three levers that do work:

| Field | Effect | Implemented in |
|---|---|---|
| `yieldMultiplier` | scales the portion count | `ChefZ_PortionManager.ResolvePortionCount()` |
| `portionBonus` | additive on the portion count | same |
| `spoilageMultiplier` | factor in the decay product chain | `ChefZ_PreservationManager.ComputeDecayScale()` |
| `grantsEffects[]` | opaque effect IDs, passed unchanged into `ChefZ_OnRecipeCompleted` | never interpreted by the core |
| `grantsTags[]` | extra tags on the dish — a trader mod can filter on them | resolved by `ChefZ_QualityManager` |

Over a whole meal, yield is the same balancing lever as nutrition, just applied
where the engine allows it. This is also why the shipped dishes carry no
`outputs[].variants` and no `_Premium` classes: 28 dishes × 4 tiers would be 112
classes with model, stringtable and loot entry.

### 1.4 The score formula

`ChefZ_QualityManager.ComputeScore()`
(`.../Scripts/3_Game/ChefZ/ChefZ_QualityManager.c`, line 781), term by term and
in exactly this order:

```
score = SUM slot.gradePoints over FILLED slots
      + SUM over evaluated gradeRules
      + (minFreshness - 0.5) * 2 * freshnessWeight
      + (mean ingredient rank - baseRank) * ingredientQualityWeight
      + SUM statePenalty[state] over the bound ingredients
      + recipe.qualityBias
      + externalBonus, clamped
      all multiplied by ctx.qualityModifier
```

The additive sum is `ChefZ_QualityEvaluation.AdditiveSum()`; the device factor is
applied last. `ComputeScore()` is purely computational: no item access, no side
effect. The result stands **completely** in the evaluation object — every summand
separately, every number with a plain-text note beside it.

If the total is not finite, it is set to `0.0` with an ERROR. A NaN would make
every threshold comparison false and the dish would silently get no tier at all.

### 1.5 The tuning knobs

`ChefZ_QualityScoring` (`.../Scripts/1_Core/ChefZ/ChefZ_QualityScoring.c`),
filled from `qualityScoring` in `.../ChefZ_Core/Config/Core.json`:

```json
"qualityScoring": {
    "defaultTierSet": "DISH_DEFAULT",
    "freshnessWeight": 1.0,
    "ingredientQualityWeight": 0.5,
    "baseRank": 1.0,
    "statePenalties": [
        { "state": "BURNT",  "points": -3.0 },
        { "state": "ROTTEN", "points": -99.0 }
    ]
}
```

Code defaults are identical, so the calculation is sensible without any
configuration.

* `freshnessWeight = 1.0` makes the freshness term run from `-1` (completely
  gone) to `+1` (dead fresh).
* `baseRank = 1.0` because a ladder `POOR/SIMPLE/PREPARED/SEASONED/PREMIUM` has
  its normal case at rank 1 — ingredients in normal condition should neither
  raise nor lower the term.
* State penalties are written as a **list of pairs**, not as an object. A
  `map<string, float>` as a JSON target is nowhere proven with the Enforce
  serialiser, and a silently unread penalty block would be the most dangerous
  kind of error: everything looks right, only burnt meat costs nothing any more.
* Weights are clamped to `>= 0` (`ClampInPlace()`) — a negative weight would
  invert the statement and make fresh ingredients produce a worse dish. State
  penalties are **not** clamped; they are supposed to be negative.

`maxExternalQualityBonus` (shipped `2.0`) clamps `externalBonus` **symmetrically**,
i.e. also downwards: a compatibility mod that drags every dish to the bottom tier
is exactly as much damage as one that lifts every dish to `PREMIUM`. Both
directions log a WARN once.

### 1.6 The freshness term — the single most important rule

`ChefZ_QualityManager.AddItemTerms()`:

> *"12 §4.1, the most important single rule of this calculation: freshness enters
> as the MINIMUM, not as the mean. A single nearly spoiled ingredient drags the
> dish down. Otherwise 'washing old meat into a premium stew' would be a standard
> exploit."*

All three item terms (freshness, ingredient quality, state penalty) run in **one**
loop over the bound handles — not out of thrift, but because "bound ingredient"
must be the same set for all three. Three separate loops would be three
opportunities to determine that set differently.

If no item is bound, `MinFreshness` is set to `-1.0` and both the freshness and
the ingredient-quality term are `0`. A recipe without ingredients is neither
rewarded nor punished.

An ingredient carrying an unknown quality ID does not count towards the mean at
all (rather than counting as rank 0, which would be a claim nobody wrote), and a
WARN is logged once per class — the most common cause is that the module owning
that tier is not loaded.

### 1.7 `gradePoints` vs. `gradeRules`

**`gradePoints`** sits on the slot and is awarded once, if that slot ends up
filled (`AddSlotPoints()` skips slots with `gradePoints == 0` and unfilled
slots). This is the cheap, declarative half.

**`gradeRules`** sit on the recipe and are the expressive half.
`ChefZ_GradeWhen` (`.../Scripts/1_Core/ChefZ/ChefZ_CompiledGradeRule.c`):

| `when` | Awards points when | Needs |
|---|---|---|
| `slotFilled` | the named slot is filled | `slotId` |
| `slotCount` | per bound item of that slot (`pointsPerItem`, falling back to `points`) | `slotId` |
| `anyItem` | at least one **bound** ingredient matches the selector | `selector` |
| `allMatched` | all bound ingredients match the selector | `selector` |
| `context` | a context value lies inside `range` | `contextKey` |
| `capability` | the actor's capability lies inside `range` | `capability` |

`anyItem` and `allMatched` run over the **bound** ingredients only, never over
the whole vessel content — otherwise a herb lying next to the pot without being
cooked would score points, which would be the next standard exploit right after
the freshness one.

`allMatched` on an empty result gives 0 points, not "all satisfied". The empty
universal statement is logically true and wrong here: it would reward a recipe
for having bound nothing.

`contextKey` is an addition to the original design — the design named `context`
as a rule kind and gave it a range, but no field saying *which* context value is
meant. The permitted keys are deliberately few: `deviceTemperature`,
`liquidQuantity`, `elapsedSec`, `portionCapacity`, `qualityModifier`. Each entry
is a promise to content authors, and withdrawing one costs more than adding one
later.

### 1.8 Resolving the tier

`ChefZ_QualityManager.ResolveTier(score, tierSet)`: the **highest** tier of the
set whose `minScore <= score`. The ladder is pre-sorted by `minScore` ascending,
so the loop breaks at the first threshold above the score.

Two refinements:

* An unknown tier set falls back to the default set rather than leaving the dish
  without a tier. The recipe is **not** rejected for it — it just has no quality.
* If the score is below every threshold, the lowest tier wins. This is the
  "implicit zero tier", implemented without inventing a tier nobody wrote.
  `WarnOnMissingZero()` logs a WARN at boot if the lowest tier of a set has
  `minScore > 0`.

Two tiers with equal `minScore` in the same set produce a WARN; the one with the
higher rank wins, because it sits later in the sorted list.

If the quality manager has no tiers at all, dishes are still produced — just
without a tier. Nothing in the cooking path is blocked by quality.

### 1.9 A worked point ladder

`RCP_ChefZ_HunterStew` (`.../ChefZ_Cooking/Config/Recipes/BowlDishes.json`).
Required slots `meat`, `roots`, `mushrooms` all carry `gradePoints: 0`. Optional
slots: `herb` = 2, `salt` = 1, `spice` = 1. Grade rules: thyme +1, hunter
seasoning +2, herb with freshness ≥ 0.8 +1.

Assume freshness 0.9 on everything, no ingredient carries a quality tier, no
state penalties, `qualityBias = 0`, no external bonus, device factor 1.0.

The freshness term is constant across all four runs:
`(0.9 - 0.5) * 2 * 1.0 = +0.8`.

| Filling | Slot points | Rule points | Freshness | Total | Tier |
|---|---|---|---|---|---|
| meat + roots + mushrooms only | 0 | 0 | +0.8 | **0.8** | `SIMPLE` (≥ 0) |
| + thyme (`herb`) | 2 | 1 (thyme) + 1 (fresh herb) = 2 | +0.8 | **4.8** | `SEASONED` (≥ 4) |
| + thyme + salt + one spice | 2+1+1 = 4 | 2 | +0.8 | **6.8** | `SEASONED` |
| + thyme + salt + spice + hunter seasoning | 4 | 2 + 2 = 4 | +0.8 | **8.8** | `PREMIUM` (≥ 7) |

Two things to read out of this:

1. The freshness term is what carries the base filling from `0.0` into `SIMPLE`.
   With freshness below `0.5` the same pot lands **below** `SIMPLE` and comes out
   `POOR` — with `yieldMultiplier 0.75`, i.e. fewer portions.
2. Burnt or rotten ingredients cannot reach the pot at all here, because
   `policy.forbiddenStates` rejects the whole recipe first
   (`["BURNT", "ROTTEN"]`). The state penalties in `Core.json` therefore only
   bite in recipes that do not set that policy.

To see the exact ladder on a live server, raise the quality log channel — every
summand is written as a separate note line
(`ChefZ_QualityEvaluation.ToLines()`). See [Troubleshooting](Troubleshooting).

### 1.10 Writing the tier onto the item

`ChefZ_Applicator.ApplyChefZQuality()`: if `result.qualityTier` is `INVALID`
(no quality manager, no tiers, safe mode), **nothing happens** and the dish is
created without a tier. Deliberately not a rollback reason — a dish without a
tier is a better outcome than no dish.

## Part 2 — Nutrition

### 2.1 The manager that changes nothing

`ChefZ_NutritionManager`
(`.../Scripts/3_Game/ChefZ/ChefZ_NutritionManager.c`) is the most unusual manager
in the core: **it changes nothing at runtime.** No item, no config, no balancing
value, no stomach. It has no writing method, and it should not get one.

Its value is in the start-up log, not in the game. Its own header calls it what
it is: the only tool with which "25 Gerichte ueber sechs Content-Slices" can be
balanced consistently.

(That header count is itself out of date — there are **28** dishes since the three
vanilla-produce plates arrived. The quote is left as the file writes it; the file is
what needs the edit, not this page.)

It also explicitly does **not** hand out energy or water past the stomach.
`consumer.GetStatEnergy().Add(...)` in `OnConsume` would be technically possible
and is rejected for three reasons: it decouples satiation from energy intake, it
acts instantly instead of over digestion time, and it is an open vector for
"many small bites".

### 2.2 Where nutrition numbers come from

Two independent sources, and they must not be confused:

1. **The actual value the player receives** comes from `CfgVehicles`:
   `class Nutrition` / `class Food` with `nutrition_properties[]` per food stage.
   ChefZ never writes there.
2. **The expected value ChefZ computes** comes from records of
   `"kind": "nutrition"` (`ChefZ_NutritionDef`), shipped in
   `Psyerns_ChefZ_Core/Addons/ChefZ_Registry/Config/Nutrition.json`.

A nutrition record looks like this:

```json
{ "id": "ChefZ_Cheese", "scope": "class", "energy": 450, "water": 60, "fullness": 35 }
```

Fields: `scope` (`class` / `category` / `tag`), `fullness`, `energy`, `water`,
`nutritionalIndex`, `toxicity`, `digestibility`, and `perUnit`. The vector
mirrors vanilla's `nutrition_properties[]` indices
(`ChefZ_NutritionVector`: `fullness[0]`, `energy[1]`, `water[2]`,
`nutritionalIndex[3]`, `toxicity[4]`, `digestibility[6]`).

Lookup order for one ingredient is class → category (deepest first) → first
matching tag, so the most specific statement wins and the answer is the same on
every server.

> `Psyerns_ChefZ_Core/_deltas/preservation.json` writes nutrition entries as
> `{ "class": "...", "energy": ..., ... }`. That is the delta hand-off shape, not
> the loaded schema — the loader expects `"id"` plus `"scope"`. See
> [Delta-Protocol](Delta-Protocol).

### 2.3 How a dish's expected value is composed

`ChefZ_NutritionManager.ComputeExpected()`. The architecture example, worked:

```
Sausage         450
Pasta           500
Tomato Sauce    100
--------------------
Base           1050
× nutritionModifier  1.10
= expected     1155
```

Two implementation details that matter when your numbers look wrong:

**The basis is the consume plan, not the slot assignment.** What is consumed is
what belongs in the calculation. A slot that binds a tool without using it up
contributes nothing — and if it were in the sum, every pot with a knife cooked
alongside would be more nutritious.

**How much of an item counts (`FactorFor()`), in order of precision:**

| Case | Factor |
|---|---|
| record has `perUnit: true` | the number of units consumed |
| whole item destroyed | `1.0` |
| partial consumption | `plan.quantityDelta / facts.quantityMax` |
| nothing consumed, nothing destroyed | `0.0` |

Partial consumption is measured against `quantityMax`, not the current amount:
vanilla values apply to a **full** item (`PlayerStomach` divides energy by 100
quantity units). Measured against the remainder, an almost empty pot of sauce
would contribute as much as a full one.

`nutritionModifier` is the per-recipe multiplier at the end
(`RCP_ChefZ_HunterStew` uses `1.1`). A value `<= 0` is treated as `1.0` with a
`BAD_MODIFIER` finding — "a value of 0 would mean the dish has no nutritional
value, and nobody writes that on purpose".

### 2.4 What the Nutrition Manager checks at start-up

`AuditAllRecipes()` runs over every compiled recipe. For each one it builds an
expected value from the **typical** filling — one representative per required
slot, in the demanded minimum count — then checks the result classes.

This is deliberately not a real cooking run and cannot be one: at boot there is
no pot. What it delivers is a comparable order of magnitude across all recipes.
The representative of a slot is the first `class`, else `category`, else `tag`
leaf of its selector; a slot whose selector has none of those (pure value ranges,
pure state requirement) contributes nothing.

Findings written into the load report:

| Kind | Severity | Meaning |
|---|---|---|
| `MISSING_BLOCK` | **ERROR** | result class has neither `class Nutrition` nor `class Food` |
| `SCOPE_ZERO` | **ERROR** | result class has `scope = 0` |
| `DEVIATION` | finding | configured energy deviates from expected by more than `nutritionTolerancePct` |
| `ZERO_INGREDIENT` | finding | an ingredient has no nutrition data and counts as 0 |
| `BAD_MODIFIER` | finding | `nutritionModifier` not positive |
| `NOT_COMPUTABLE` | finding | overflow or NaN — **no number is reported at all**, because a fantasy number in the start log is worse than none |
| `CLAMPED` | INFO | the expected value hit `nutritionExpectedCap`; the number is only usable with caution. Not a balancing cap — the actual `CfgVehicles` values are untouched |

Settings in `.../ChefZ_Core/Config/Core.json`:

```json
"enableNutritionAudit": true,
"nutritionTolerancePct": 25.0,
"nutritionAuditMaxFindings": 64,
"nutritionExpectedCap": 100000.0
```

With the audit switched off, one banner line still remains in the log —
otherwise an operator would mistake an empty result for a good one.

### 2.5 The check that matters most

`CheckStomachRegistration()`. From its own comment:

> *"A dish without a Nutrition block is eaten, disappears from the inventory and
> satiates nothing. There is no error message, no log entry, no hint. That is the
> quietest conceivable content error and therefore the most dangerous."*

The chain is `PlayerStomach.InitData` not registering the class, so
`AddToStomach` aborts without a message. The audit reports it as an ERROR; the
recipe itself is rejected by `ChefZ_RecipeCompiler`. Non-edible result classes —
an empty can, a container — are exempt; demanding a nutrition block from them
would be a warning without an error.

Byproducts are checked for stomach registration but are deliberately **not**
compared against the expected value: the expected value belongs to the main
result. A bone falling off during butchering should not be as nutritious as the
dish, and a warning demanding that would be nonsense.

### 2.6 The audit corrects nothing

Every deviation message ends with the same sentence:

> `KEINE Korrektur - der Core aendert nie einen Balancingwert.`
> ("NO correction — the core never changes a balancing value.")

If a dish is 40 % off, the fix is a `CfgVehicles` edit in the content module, or
a `nutritionModifier` change in the recipe. Not a core setting.
