# Food States

The ChefZ state catalogue, how it projects onto vanilla `FoodStage`s, where
transitions come from, and how preservation hangs off it.

Related pages: [Recipes](Recipes), [Processing-Stations](Processing-Stations),
[Quality-and-Nutrition](Quality-and-Nutrition),
[Portions-and-Containers](Portions-and-Containers), [Modules](Modules),
[Validation](Validation), [Known-Limitations](Known-Limitations),
[Troubleshooting](Troubleshooting).

---

## 1. Where the catalogue actually lives

**`CfgChefZStates` in
`Psyerns_ChefZ_Core/Addons/ChefZ_Preservation/config.cpp` (line 622).**

Not in JSON. The reason is recorded in the file header: the state is
sync-relevant, it travels over the network, and Rank-1 data (game config) is the
only data the client is guaranteed to read. A JSON record would sit at Rank 2 and
be modifiable through the `$profile:` overlay — which the design forbids for
anything that syncs. The record type accepts JSON (`"kind": "state"`) and no
shipped file uses it.

`Psyerns_ChefZ_Core/_deltas/preservation.json` contains a `states[]` block with
the same ten entries. **`_deltas/` is a hand-off staging area between content
modules; no code reads it.** See [Delta-Protocol](Delta-Protocol).

The core itself declares no state. `ChefZ_StateDef`
(`.../ChefZ_Core/Scripts/1_Core/ChefZ/ChefZ_StateDef.c`) describes only *which
fields* a state has; the words `SMOKED`, `SALTED` and `PREPARED` do not occur in
it and must not.

---

## 2. The ten shipped states

| ID | `projectsToVanillaStage` | `edible` | `terminal` | `preserved` | `freshnessLifetimeSec` | `implies` |
|---|---|---|---|---|---|---|
| `RAW` | `Raw` | yes | no | no | — | — |
| `PREPARED` | `Raw` | yes | no | no | — | — |
| `COOKED` | `Boiled` | yes | no | no | — | — |
| `BAKED` | `Baked` | yes | no | no | — | — |
| `FRIED` | `Baked` | yes | no | no | — | — |
| `SALTED` | `Raw` | yes | no | **yes** | `43200` (12 h) | `CHEFZ_PRESERVED` |
| `SMOKED` | `Dried` | yes | no | **yes** | `86400` (24 h) | `CHEFZ_PRESERVED` |
| `DRIED` | `Dried` | yes | no | **yes** | `129600` (36 h) | `CHEFZ_PRESERVED` |
| `BURNT` | `Burned` | **no** | no | no | — | — |
| `ROTTEN` | `Rotten` | **no** | **yes** | no | — | — |

Notes that are load-bearing:

* `PREPARED` means "chopped, minced, ground — still raw". It is the state with no
  visual of its own: it says a human worked on it, nothing more.
* `FRIED` and `BAKED` share a projection. For vanilla, pan equals oven; for a
  recipe, it does not.
* `SALTED` projects onto **`Raw`**, not onto `Dried`. Salting does not kill
  bacteria — salted meat stays raw, it just keeps longer. Consequently
  `ChefZ_SaltedMeat` also carries `agents = 4`.
* `SMOKED` and `DRIED` share the vanilla stage `Dried`. That costs nothing:
  ChefZ keeps them apart via its own state.
* `BURNT` is inedible but **not** terminal — charcoal can still rot.
* `ROTTEN` is terminal. A requested transition out of it is refused with the
  reason "terminal state". **ChefZ never sets `ROTTEN`** — only vanilla decay
  does.

`spoilageMultiplier` is deliberately absent from `CfgChefZStates`. It is stated
once, in the preservation records (§6).

---

## 3. What `projectsToVanillaStage` actually means

The valid names are `Raw`, `Baked`, `Boiled`, `Dried`, `Burned`, `Rotten`
(`ChefZ_VanillaStage`). At boot, `ChefZ_StateDef.Compile()` converts the string
once into the integer `projectedStage`, so the hot path never does a string
compare.

**Where a projection is declared, the vanilla mechanics remain in charge:**
`visual_properties`, the nutrition base, agent cleanup, the `CanProcessDecay()`
stop at `ROTTEN`, cooking sounds, frost logic. ChefZ rebuilds none of them.

Error behaviour (`ChefZ_StateDef.Validate()`): an unknown stage name produces a
WARN, the field is **cleared**, and the record stays valid — the state then
projects onto nothing, and optics, nutrition base and agent cleanup stay as they
are. The name is checked hard because an invalid enum value passed to
`SetFoodStageType` would be a sync error.

Nothing else in `ChefZ_StateDef` can bring the record down. Every failure case is
a warning with a weakened effect: a rejected state would strip its identity from
every item carrying it, whereas a state without projection can merely do less.

* `spoilageMultiplier <= 0` → WARN, set to `1.0` (neutral).
* `freshnessLifetimeSec <= 0` → WARN, dropped; the server default applies.

---

## 4. How the state of an item is determined

`ChefZ_ItemStateComponent.GetState()`
(`.../ChefZ_Core/Scripts/4_World/ChefZ/State/ChefZ_ItemStateComponent.c`, line 719),
in order:

1. **The item's own state variable** — persisted and synced, if set.
2. **`defaultState` from `CfgChefZIngredients`** — the V1 normal case: *the class
   is the state*. `ChefZ_TomatoSauce` is `COOKED` because its ingredient record
   says so.
3. **Reverse mapping from the vanilla `FoodStage`** — via
   `ChefZ_StateManager.FromVanillaStage()`. Guarded, because
   `Edible_Base.GetFoodStageType()` dereferences `GetFoodStage()` without a null
   check and an `Edible_Base` without a `FoodStage` — an empty pot, for instance —
   would crash.
4. `INVALID`.

---

## 5. Transitions

**There is no transition table in the state catalogue.** From the header of
`ChefZ_StateDef.c`:

> *"A state change is always the result of a process or a recipe; it lives as a
> `ChefZ_TransformDef` in the Processing Manager. A second registry for the same
> thing would be a second truth and a second place a content author would have to
> search."*

State changes therefore come from exactly three places:

### a) Recipe outputs — `outputs[].setState`

```json
"outputs": [ { "cls": "ChefZ_HunterStewBulk", "setState": "COOKED", ... } ]
```

Applied by `ChefZ_Applicator.ApplyChefZState()`. An unknown name is a WARN; the
dish is still created and falls back to deriving its state from its class. A
failure here is never a rollback reason — the state is an addition to the dish,
not part of its existence.

### b) Slot `setStateAfter`

For `consume: "none"` — what survives in the pot changes state.

### c) Transforms — `"kind": "transform"`

Station and handcraft conversions. `ChefZ_TransformDef`
(`.../Scripts/1_Core/ChefZ/ChefZ_TransformDef.c`) carries `process`,
`stationsAllowed`, `inputs[]` (the same `ChefZ_SlotDef`), `outputs[]` (the same
`ChefZ_OutputDef`), `durationOverrideSec`, `qualityRule`, `freshnessCarry`,
`qualityDelta`, `priority`, `requires[]`.

From `Psyerns_ChefZ_Core/Addons/ChefZ_Preservation/Config/Processing/Smoking.json`:

```json
{
    "id": "TR_FishToSmoked",
    "process": "PROCESS_SMOKE",
    "stationsAllowed": ["ChefZ_Smoker"],
    "durationOverrideSec": 1500,
    "inputs": [
        {
            "slotId": "fillet",
            "match": { "allOf": [ { "category": "FISH" }, { "state": "RAW" } ] },
            "minCount": 1,
            "consume": "whole"
        }
    ],
    "outputs": [
        { "cls": "ChefZ_SmokedFish", "quantityMode": "fromInput",
          "setState": "SMOKED", "inheritFreshness": true }
    ],
    "freshnessCarry": 1.0,
    "qualityRule": "MIN",
    "priority": 100
}
```

Note that the input selector matches on `state`, and the output both swaps the
class *and* sets the state. Both halves matter: the class carries the model and
nutrition, the state carries the preservation factor and the tag.

See [Processing-Stations](Processing-Stations) and
[Production-Chains](Production-Chains).

---

## 6. Setting a state — the rules

`ChefZ_ItemStateComponent.SetState(item, state, applyVanillaTransition = false)`:

| Situation | Behaviour |
|---|---|
| called client-side | no-op with ERROR; there is no RPC fallback, the state is a server decision |
| `state` invalid | silent `false` — callers regularly come from data that says nothing (`"setState": ""`) |
| X → X | no-op: no event, no sync, no log. Measured against the **resolved** state, not the variable |
| current state is `terminal` | refused with WARN: "a terminal state is the end of a chain, not an intermediate step" |
| target state unknown | ERROR, nothing set — an item in a state no recipe and no display knows would be worse than one without a state |

On success: persist hash and sync ordinal are written together, the projection is
applied, `SetSynchDirty()` fires, and then the events
`ChefZ_OnFoodStateChanged`, plus `ChefZ_OnFoodPreserved` if `preserved`, plus
`ChefZ_OnFoodSpoiled` if the projection is `Rotten`. All preservation paths —
smoking, drying, salting, cooking — run through this one place, so a quest
"preserve 10 foods" needs one subscriber, not five.

### The projection, and why agents survive

`ProjectOnto()` is the most explanation-heavy method in the subsystem. In DayZ
1.29, `ChangeFoodStage()` *is* `SetFoodStageType()`, and both end in
`Edible_Base.OnFoodStageChange` → `HandleFoodStageChangeAgents()`. So the split
cannot be made at the vanilla call. ChefZ makes it at the item instead:

* a marker `m_Projecting` is set,
* `SetFoodStageType()` runs (the bookkeeping),
* `ChefZ_Edible_Base.OnFoodStageChange` sees the marker and skips vanilla's agent
  handling, but still refreshes the visuals.

Result: a pure administrative projection does not delete agents; a real cooking
transition (`applyVanillaTransition = true`) still does.
`ChefZ_Applicator` passes `false` — the vanilla chain has nothing to do on a
freshly created item and would delete the agents that were just carried over.

If the item has no `FoodStage` at all, the ChefZ state is still set and a WARN
is logged once per class: the state works, the optics do not.

### Vanilla wins at `BURNED` and `ROTTEN`

`ChefZ_ItemStateComponent.OnVanillaStageChanged()`: as soon as vanilla sets one
of the two failure stages, the ChefZ overlay is **deleted**. The item then falls
back to the projection rule and reads its state from the vanilla stage. Two
systems both allowed to say "this is spoiled" would drift apart guaranteed.
Spoiled stays spoiled.

---

## 7. Preservation

Preservation records are `"kind": "preservation"`. Shipped in
`Psyerns_ChefZ_Core/Addons/ChefZ_Registry/Config/Preservation.json`:

| `id` | `scope` | `spoilageMultiplier` |
|---|---|---|
| `DRIED` | `state` | `0.15` |
| `SMOKED` | `state` | `0.25` |
| `SALTED` | `state` | `0.50` |
| `COOKED` | `state` | `0.80` |
| `BROTH` | `category` | `0.90` |
| `SAUCE` | `category` | `0.70` |

A multiplier is a **factor on vanilla's decay speed**, never a decay calculation
of its own. `0.25` means "four times the shelf life", independent of food stage
and food type, without anyone maintaining a vanilla constant. There is
deliberately no field for a shelf life in seconds, no replacement for
`GameConstants.DECAY_FOOD_*`, no own random spread and no own stage transition.

### `scope` — the five dimensions

`ChefZ_PreservationScope` (`.../Scripts/1_Core/ChefZ/ChefZ_PreservationDef.c`):
`state`, `class`, `category`, `tag`, `quality`.

**The record's `id` *is* the target.** There is no separate target field; `scope`
says which table is looked up. This has an honestly named consequence: the
registry keeps IDs unique, so if a category and a tag share a name, only one
preservation rule can exist for it — the second is rejected at load with an error
naming both origins.

An unknown `scope` is the **only** field that can reject a preservation record.
Everything else is clamped with a warning. The reason: with any other error the
author's intent is still recognisable; with an unknown scope it is not — the
record would carry an ID and nobody would know whether it names a state, a class
or a tag. Falling back to `"state"` would be the worst variant, silently matching
nothing while looking like an effective rule.

### Other fields on a preservation record

| Field | Meaning |
|---|---|
| `stopsDecay` | `CanProcessDecay()` returns false — decay does not run at all |
| `preventsRotten` | decay runs, but the vanilla stage never flips to `ROTTEN` |
| `environmentTemperature` | the multiplier applies only inside this range; `null` = always |
| `onPlayerMultiplier` | an **additional** factor while the item is on a player |

`stopsDecay` and `preventsRotten` are two switches, not one, so a preserve does
not have to rot while its state may still change — without the core needing to
know what a preserve is.

`onPlayerMultiplier` is honestly documented as *additional*, not a replacement
for `GameConstants.DECAY_RATE_ON_PLAYER`: vanilla adds its bonus inside
`Edible_Base.ProcessDecay` onto the protected `m_DecayDelta`, which only comes
into being after ChefZ has already handed over its delta. There is no point at
which it could be replaced without rebuilding the whole method.

`spoilageMultiplier <= 0` is clamped to `MIN_SPOILAGE_MULTIPLIER = 0.01` with a
WARN — "zero would be immortality by typo, and the single most effective operator
mistake there is. Whoever wants real immortality sets `stopsDecay`, and then it
says so."

### The product chain

`ChefZ_PreservationManager.ComputeDecayScale()`
(`.../Scripts/3_Game/ChefZ/ChefZ_PreservationManager.c`):

```
mul = globalSpoilageScale               Core.json server dial
    * StateDef.spoilageMultiplier       the state's own statement
    * QualityTier.spoilageMultiplier    see Quality-and-Nutrition
    * containerModifier                 see Portions-and-Containers
    * preservation record (scope state)
    * preservation record (scope class)
    * preservation record (scope quality)
    * PRODUCT(preservation records, scope category)
    * PRODUCT(preservation records, scope tag)
  clamped to [minDecayScale, maxDecayScale]
```

Shipped values in `.../ChefZ_Core/Config/Core.json`:
`globalSpoilageScale = 1.0`, `minDecayScale = 0.01`, `maxDecayScale = 10.0`.

Two clarifications the code makes explicitly:

* Multiple matching records **multiply**, they do not sum. A sum would already be
  wrong at a single hit (one record of `0.5` would give `0.5`, two would give
  `1.0` — more durable becomes less durable).
* `StateDef.spoilageMultiplier` and a preservation record with `scope: "state"`
  are two separate factors on purpose: the first is the state's statement about
  itself, the second is the operator's statement about the same state. Set both
  and you get the product.

The multiplier is recomputed on **every** call and persisted nowhere. Storing it
would mean a balancing change by the admin never takes effect on existing items.
The return value is always inside the clamp: never 0, never negative, never NaN.
If the registry is not built, the answer is the neutral `1.0` and
`super.ProcessDecay(delta, ...)` runs unchanged.

### Freshness

Separate from decay. `freshnessLifetimeSec` on the state defines how long
freshness lasts; if the state does not name one, `defaultFreshnessLifetimeSec`
from `Core.json` applies (shipped: `21600` s = 6 h).
`ChefZ_PreservationManager.AdvanceFreshness()`:

```
next = clamp(current - (deltaSec * multiplier) / lifetime, 0, 1)
```

`InheritFreshness()` carries freshness into a result via `freshnessCarry`;
`ChefZ_Applicator.ApplyChefZState()` uses the **minimum** freshness of the
consumed ingredients, not the mean — an average could be washed out by mixing in
fresh goods. Same principle as the quality freshness term; see
[Quality-and-Nutrition](Quality-and-Nutrition).

`ChefZ_StateDef.freshnessLifetimeSec` deliberately stays on its sentinel when
unset, so a later server value can still override it. Filling in a number at
`ResolveDefaults()` would make "the state declared a lifetime" and "nobody said
anything" indistinguishable.

---

## 8. The two traps — read this before adding a class

Both are documented in the code because both cost the project a full test run.

### Trap 1 — a class without `Food > FoodStageTransitions` never cooks

Vanilla's `FoodStage.GetNextFoodStageType()` falls back to
`FoodStageType.BURNED` when a class declares no transitions
(`FoodStage.c:472`). A class with `FoodStages` but no `FoodStageTransitions`
therefore turns to charcoal on its first stage change — or, in the ChefZ case,
never reaches an accepted `doneStage`, so an `ON_STAGE` recipe listing it as a
required ingredient **never completes**.

`ChefZ_Edible_Base.ChefZ_DeclaresCookTransitions(type)`
(`.../Scripts/4_World/ChefZ/State/ChefZ_Edible_Base.c`, line 381) is the runtime
test:

```c
bool has = g_Game.ConfigIsExisting("CfgVehicles " + type + " Food FoodStageTransitions");
```

The result is cached per class name (it cannot change at runtime) and logged
**once per class** at DEBUG on the `COOK` channel:

> `<Class>: kein Food > FoodStageTransitions - NICHT kochbar. Die Klasse bleibt
> im Kochgeraet auf ihrer Garstufe stehen; ein ON_STAGE-Rezept, das sie als
> Pflichtzutat fuehrt, wird nie fertig (01 V4).`

That is the line which would have made the original blocker visible in the RPT.
Turn the `COOK` channel to DEBUG when a recipe refuses to finish. See
[Troubleshooting](Troubleshooting).

### Trap 2 — a class whose script chain does not switch cookability on

Vanilla does **not** derive cookability from data. `Edible_Base.CanBeCooked()`
returns `false` (`Edible_Base.c:129`), and every cookable vanilla food switches
it back on in its own class — `Potato.c:3`, `Lard.c:3`, `CarpFilletMeat.c:3` and
roughly forty more, all with the same three-line `return true;`.

`ChefZ_Edible_Base` originally did not have that switch. The consequence was not
an error picture but a **silent block**: `Cooking.ProcessItemToCook`
(`Cooking.c:47`) walked past every ChefZ ingredient with its own script class,
the vanilla food stage stayed on `RAW`, and
`ChefZ_RecipeEvaluator.CheckStages()` demands an accepted done stage from every
bound required ingredient. No `ON_STAGE` recipe could ever finish. **None of this
was statically visible — the line was simply missing.**

The fix now in the code is data-derived rather than a blanket `return true;`:

```c
override bool CanBeCooked()
{
    if (!GetFoodStage())
        return false;

    return ChefZ_DeclaresCookTransitions(GetType());
}
```

Two conditions, both necessary:

1. `GetFoodStage() != null`. Without that object, `true` is not a feature but a
   crash: `Cooking.UpdateCookingState` calls `GetNextFoodStageType()` unchecked.
   This really affects `ChefZ_ServedDish_Base` — a served portion deliberately
   has no `Food` node.
2. The class declares `Food > FoodStageTransitions`.

So Trap 1 is unreachable for any heir of `ChefZ_Edible_Base`: being cookable
*presupposes* the transitions. The validator rule (`chefzstage`) is still needed —
it covers content that overrides `CanBeCooked()` itself, and classes that inherit
a cookable **vanilla** script class (`ChefZ_Butter` extends `Lard`). See
[Validation](Validation).

`CanBeCookedOnStick()` is deliberately left alone: ChefZ does not hook
`CookOnStick`, and vanilla does not couple the two switches (`CaninaBerry` is
`CanBeCooked() == true` and `CanBeCookedOnStick() == false`). Content that wants
spit-roasting writes the same three lines vanilla writes on `Lard` and gets the
test for free:

```c
override bool CanBeCookedOnStick()
{
    return ChefZ_Edible_Base.ChefZ_DeclaresCookTransitions(GetType());
}
```

### Practical checklist for a new food class

* `class Food` with **both** `FoodStages` **and** `FoodStageTransitions`, unless
  the class is deliberately not cookable (a served portion, a spice, salt).
* Script class extends `ChefZ_Edible_Base` (or `ChefZ_PortionedFood_Base`) so the
  state component, sync and persistence exist.
* A `CfgChefZIngredients` entry with `defaultState`, so step 2 of the state
  resolution has something to answer with.
* If the state carries a projection, the class needs a `FoodStages` block or the
  optics stay wrong while the state works.
