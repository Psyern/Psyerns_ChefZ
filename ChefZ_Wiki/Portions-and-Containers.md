# Portions and Containers

How a large dish comes into being in the pot, how a single serving is taken out
of it, which container kinds exist, and what is left over after eating.

Related pages: [Recipes](Recipes), [Food-States](Food-States),
[Quality-and-Nutrition](Quality-and-Nutrition), [Configuration](Configuration),
[Adding-Content](Adding-Content), [Known-Limitations](Known-Limitations),
[Troubleshooting](Troubleshooting).

> **Status since 2026-08-29:** the shipped dishes **no longer use the bulk /
> portion step.** A recipe produces the served dish directly in the cooking
> vessel — one item, `quantity = 100 × portions` (`PlayerStomach.c:92` rates
> nutrition per 100 units, so 100 units are one serving). The bulk classes,
> the take-portion action and `amountPerPortion` are gone from the content.
> Sections 2–5 below describe the portion system that remains **in the Core as a
> capability** for a module that wants it; section 6 (containers) and 7 (what is
> left over) still apply. `returnContainer` on a dish is now a fixed class
> (`ChefZ_EmptyBowl` / `ChefZ_EmptyPlate`), never `"AUTO"`.

## 1. One class per dish

| | Served dish |
|---|---|
| Example class | `ChefZ_HunterStewBowl` |
| Script base | `ChefZ_ServedDish_Base` |
| Where it is | in the cooking vessel, then in the player's hands |
| Vanilla `Food` node | **no** — `HasFoodStage()` is false, it cannot burn in the pot |
| Portions | vanilla `quantity`; the class `varQuantityMax` covers its largest recipe |
| Is eaten | yes, serving by serving from the same item |
| Returns a container when finished | `returnContainer`, a fixed class |

`ChefZ_ServedDish_Base` carries **no** `Nutrition` block; every dish declares its
own (`PlayerStomach` only registers classes with their own nutrition, see
[Quality-and-Nutrition](Quality-and-Nutrition)).

## 2. Why a separate counter and not vanilla `quantity`

`ChefZ_PortionedFood_Base` keeps its own `int`. From its header
(`.../ChefZ_Core/Scripts/4_World/ChefZ/Portion/ChefZ_PortionedFood_Base.c`):

* `Cooking.DecreaseCookedItemQuantity` subtracts 25 on **every** `FoodStage`
  change; `ProcessItemToCook` subtracts on overheating; split and stack also
  reach in. A portion counter on `quantity` would lose portions from mere
  **keeping warm** — hard to explain and hard to balance.
* `quantity` already carries the per-bite consumption amount and feeds the
  nutrition calculation. A separate `int` cleanly separates "how many bowls"
  from "how much is in this bowl".

Where the two numbers live:

| Value | Location | Persisted | Synced |
|---|---|---|---|
| `m_ChefZ_Portions` | shared `ChefZ_ItemStateComponent` | yes | **yes** (0..31) |
| `m_ChefZ_PortionsMax` | own small block on `ChefZ_PortionedFood_Base` | yes | **no** |

The maximum is not in the shared block because that block is written by *every*
ChefZ item, including salt and flour. An extra field there would have bumped the
block version and shifted the read stream of every saved ChefZ item by four
bytes, for a number only a portioned dish needs.

Consequence, stated openly: the client does not receive the maximum. The tooltip
shows the same value as the counter unless a server sync brings it along. That
is the accepted trade — a second sync variable on every kettle costs bandwidth
for a number that never changes.

`ChefZ_GetPortionsMax()` answers with the current count if no maximum was ever
set (an item from a save predating the block, an admin spawn). That is the only
answer which never displays "3 / 0".

## 3. Declaring a portioned output

Portioning is declared in the recipe's `outputs[]` entry — there is **no** record
kind and no JSON file for portions. `ChefZ_PortionSpec` is derived at runtime
from the portion fields of a `ChefZ_OutputDef`, keyed by the result class.
Transforms carry the same `ChefZ_OutputDef` and are read along with them;
otherwise a bulk dish created at a station would be an item with a counter that
nobody can take from.

From `RCP_ChefZ_HunterStew`
(`.../ChefZ_Cooking/Config/Recipes/BowlDishes.json`):

```json
"outputs": [
    {
        "cls": "ChefZ_HunterStewBulk",
        "quantity": 1, "quantityMode": "fixed",
        "portions": 4,
        "portionClass": "ChefZ_HunterStewBowl",
        "portionQuantity": 1.0,
        "amountPerPortion": 1.5,
        "containerCategory": "BOWL",
        "consumesContainer": true,
        "returnContainer": "AUTO",
        "emptyOnLastPortion": "",
        "scaleWithDevice": true,
        "inheritQuality": true,
        "setState": "COOKED",
        "inheritFreshness": true, "freshnessCarry": 0.9,
        "inheritTemperature": true
    }
]
```

| Field | Meaning | Default |
|---|---|---|
| `portions` | base portion count; `0` = not a portioned dish | `0` |
| `portionClass` | what a take produces | — |
| `portionQuantity` | vanilla quantity per portion; `< 0` = class default | class default |
| `amountPerPortion` | **the amount cap** — recipe units required per portion | none |
| `containerCategory` | container category a take needs; `""` = none | none |
| `consumesContainer` | is the container used up on serving | `true` |
| `returnContainer` | `""` \| `"AUTO"` \| class name | `""` |
| `emptyOnLastPortion` | what the bulk remainder becomes; `""` = delete | `""` |
| `scaleWithDevice` | is the device cap applied | `true` |
| `inheritQuality` / `inheritState` / `inheritFreshness` | carry-over to the portion | `true` |
| `takeDurationSec` | take duration; `< 0` = `CoreSettings.defaultTakePortionSec` | `2.0` s |
| `takeDisplayName` | stringtable key of the action text | generic key |

**`amountPerPortion` is not optional in practice.** From the header of
`ChefZ_PortionManager`:

> *"Without this cap a minimal filling in the kettle would yield twelve portions —
> a glaring food exploit."*

The shipped dish recipes size it so the target amount works out exactly:
base recipe `6 required units / 1.5 = 4 portions` (Pot cap 4); group recipe
`10 required units / 1.25 = 8 portions`.

## 4. How the portion count is computed

`ChefZ_PortionManager.ResolvePortionCount()`
(`.../Scripts/3_Game/ChefZ/ChefZ_PortionManager.c`, line 373). The order is not
negotiable:

```
n = spec.portions
n = min(n, ctx.portionCapacity)                 <- cap 1: the device
n = min(n, floor(consumedRequiredUnits / amountPerPortion))   <- cap 2: the amount
n = floor(n * yieldMultiplier) + portionBonus   <- quality
n = clamp(n, 1, 31)                             <- sync limit
```

**Cap 1 — the device.** `portionCapacity` comes from `CfgChefZDevices` in
`.../ChefZ_Cooking/config.cpp`:

| Device | `portionCapacity` |
|---|---|
| `FryingPan` | 2 |
| `Pot` | 4 |
| `Cauldron` | 12 |

The cap lives on the device, where it belongs. The same stew recipe therefore
yields 2 in a pan and 12 in a cauldron without being written twice. A device with
no entry has `portionCapacity = 0`, which means "the device says nothing about
it" and caps nothing — only the amount cap remains. These are vanilla class
names; ChefZ never touches a foreign `config.cpp`, it names foreign classes in
its own node.

`scaleWithDevice: false` switches this cap off for one output.

**Cap 2 — the amount.** `floor()`, never `round()`: half a portion does not
exist, and rounding up would be precisely the way to turn a minimal filling into
a full yield. The input is `ConsumedRequiredUnits()` — the recipe units consumed
from **required** slots only. An optional pinch of salt must not produce an extra
portion of stew, otherwise the cap could be defeated via the cheapest optional
ingredient. It counts `unitsDelta`, not `quantityDelta`, because vanilla quantity
is grams, millilitres or pieces depending on class, and dividing that by
`amountPerPortion` would be meaningless.

**Quality — after both caps.** Computed before them, a single excellent
ingredient could jump the amount cap. `floor()` again, because rounding gains
("portion eight times, round up eight times") are explicitly forbidden. Values
come from the quality tier — see
[Quality-and-Nutrition](Quality-and-Nutrition). With no tier resolved, this step
is skipped entirely and the answer is neutral.

**Clamp `[1, 31]`.** The upper bound is the sync limit
(`ChefZ_PortionLimits.MAX`, which must equal `ChefZ_SyncLimits.PORTIONS_MAX`;
`ChefZ_PortionSpec.SelfCheck()` verifies this). The lower bound is 1 — a
portioned dish with zero portions would be a dish you can never eat.

`ChefZ_Applicator.ApplyPortions()` calls this and then
`bulk.ChefZ_SetPortions(n, n)`. With the `PORTION` log channel at DEBUG, every
step of the calculation is written out as a separate line.

Two failure modes worth knowing, both WARN and both non-fatal:

* The result class is declared as portioned but is not in the portion registry →
  the dish is created without a counter and stays edible; nothing can be taken
  from it.
* The result class is declared as portioned but its **script class does not
  extend `ChefZ_PortionedFood_Base`** → no counter and no take action. The dish
  is created as an ordinary item.

## 5. Taking a portion

`ChefZ_ActionTakePortion` is **one** action class for **all** portioned dishes.
A new portioned dish inherits from `ChefZ_PortionedFood_Base` and is done — no
new action, no core change, no entry anywhere. The price is that action text and
duration must come from data: text from `ChefZ_PortionSpec.displayName` (fallback
`#STR_CHEFZ_ACTION_TAKE_PORTION`), duration from `takeDurationSec` (fallback
`CoreSettings.defaultTakePortionSec`).

The action hangs on the **target**, not on the hand item
(`ContinuousInteractActionInput`, `m_DetectFromTarget = 1`). Openly named
consequence: a bulk dish the player is *holding* does not offer this action —
vanilla's input type collects either from the target or from the hand item, not
both. You put it down, or leave it in the pot, and aim at it.

Client shows, server decides. `ActionCondition` may be generous;
`OnFinishProgressServer` calls `ChefZ_TakePortion()`, and **that revalidates
again**. Seconds pass between action start and finish, and double-taking by two
players is the first thing an exploit hunter tries.

### The order inside `ChefZ_TakePortion()`

```
1. REVALIDATE   portions, class, spec, actor
2. VETO         ChefZ_OnPortionTaken, cancellable
3. CREATE       hands -> inventory -> ground
4. CARRY OVER   state, quality, freshness, temperature, agents
5. CONTAINER    consume
6. DECREMENT                              <- only now
7. SOURCE       replace or delete if empty
8. SetSynchDirty
```

**Step 6 is the first step that takes something away from the player, and it
comes after everything that can fail.** The invariant is stated twice in the
code: abort *before* decrementing; counter unchanged, container unconsumed, WARN.
Never lose portions into nothing.

Concretely: if the portion cannot be spawned, the counter stays. If the carry-over
fails, the portion is deleted again and the counter stays. If the container cannot
be consumed, the portion is deleted again and the counter stays. In every failure
case the player has exactly what they had before.

The creation order is hands → inventory → ground, deliberately *not* vanilla's
(`HumanInventory.CreateInInventory` tries inventory, then hands). Whoever takes a
portion usually wants to eat it, and a plate landing in the backpack would be a
second handling step. Dropping to the ground is the last resort and explicitly
**not** an error; only if that also fails does the take abort, and then without
any effect.

The veto event fires at **step 2**, before any effect, even though the original
design sketched it as step 8. The reason: a subscriber cancelling after the
decrement could not give the portion back — the core would believe it and the
player would have it anyway. Cancelling before any effect is consequence-free and
therefore honest.

### When the last portion is taken

`ChefZ_FinishLastPortion()`:

* `emptyOnLastPortion` set → the bulk item is swapped for that class. If the swap
  fails, a WARN is logged and the empty dish stays lying around and can be
  disposed of normally.
* `emptyOnLastPortion` empty (the shipped case for all dishes) → the bulk is
  deleted. The vessel is empty again.

If `portionsLeft <= 0` on an existing item, the action simply does not appear.
The item stays consumable as a normal item if its class allows that. **No
deletion of player property.**

## 6. Containers

### 6.1 Where they are declared

`CfgChefZContainers` in `Psyerns_ChefZ_Core/Addons/ChefZ_Cooking/config.cpp`
(line 2755). One declaration project-wide.

Rank 1, not JSON, and the reason is a client-side one:
`ChefZ_ActionTakePortion.ActionCondition()` runs **on the client** and must
decide there whether a matching bowl is in reach — otherwise the action appears,
the server rejects it, and the player watches a progress bar that does nothing.
The client is guaranteed to read Rank 1.

JSON remains permitted (it is the same record), but a container added via a
`$profile` overlay changes only the **server-side** view; the action may then not
appear to the player even though the server would allow it.

### 6.2 The shipped containers

| Class | Category | `emptyClass` | `reusable` | `consumedOnServe` | `spoilageModifier` | `searchScope` |
|---|---|---|---|---|---|---|
| `ChefZ_EmptyPlate` | `PLATE` | `ChefZ_EmptyPlate` | yes | yes | `1.00` | 3 |
| `ChefZ_EmptyBowl` | `BOWL` | `ChefZ_EmptyBowl` | yes | yes | `1.00` | 3 |
| `ChefZ_EmptyCan` | `CAN` | *(none)* | **no** | yes | `0.15` | 3 |
| `ChefZ_EmptyJar` | `JAR` | `ChefZ_EmptyJar` | yes | yes | `0.10` | 3 |
| `ChefZ_EmptyBox` | `BOX` | `ChefZ_EmptyBox` | yes | yes | `0.80` | 3 |

`ChefZ_EmptyCan` is the preserve case: `reusable = 0`, nothing comes back, and
that is not an error. Its `spoilageModifier = 0.15` is the reason preserves are
durable at all — "pickled goods in a jar keep longer" is a number in a file and
needs neither a `CANNED` state nor special logic. The factor feeds the decay
product chain described in [Food-States](Food-States).

**A recipe never names a container class, only a category.** Which classes belong
to a category lives in `CfgChefZContainers`, i.e. in content or with the server
operator. A wooden bowl added later registers itself with
`containerCategories[]` and works **immediately** with every existing bowl dish —
without touching a recipe or a line of core code. A container may belong to
several categories.

The registry does **not** follow `CfgVehicles` inheritance, on purpose. A
container is an explicit declaration, not an inheritance. Collecting derivatives
would eventually turn every variant from a foreign mod into a `BOWL`, and the
player would lose containers the author never heard of.

### 6.3 Where a container is searched for

`searchScope` is a bit field (`ChefZ_ContainerScope`):

| Bit | Value | Meaning |
|---|---|---|
| `HANDS` | 1 | what is in the hand |
| `INVENTORY` | 2 | backpack, pockets, vest |
| `NEARBY_CARGO` | 4 | crates and barrels within `containerSearchRadius` |

Default is `3` (`HANDS | INVENTORY`). `NEARBY_CARGO` is deliberately not in the
default: it gets expensive at bases with many crates and makes the selection
opaque for the player — they cannot see which barrel their plate came from.
Unknown bits are reported at build time and masked out rather than accepted.

**The search order is fixed and not configurable**
(`ChefZ_ContainerService`): hands → inventory → environment. Within a level,
health decides, then class name — deterministic, not slot-dependent. Two players
with the same inventory get the same plate, and the same player gets the same
answer twice.

Settings in `.../ChefZ_Core/Config/Core.json`:
`containerSearchRadius = 3.0`, `maxContainerCandidates = 32`.

Client and server split cleanly: the client may **search**
(`FindCandidates()` reads inventory and environment so `ActionCondition` knows
whether to show the action) and changes nothing. Only the server
**consumes** and **returns**; `ConsumeForServing()` and `ReturnEmpty()` exit
without effect off-server.

`ChefZ_Container_Base` is an **optional** script base. A container need not
inherit from it — the registry works purely on class names, and a vanilla pot can
sit in `CfgChefZContainers` and function completely without anyone touching its
script class. What the base adds is `ChefZ_GetContainerCategory()` and an
overridable `ChefZ_IsEmpty()`. The second is the real reason it exists: the
service must never consume a **filled** container — a pot with water is not a
free bowl. The general answer (no cargo and quantity ≤ 0) is almost always right;
where it is not, an override beats a special rule in the core.

### 6.4 What happens when no container system is loaded

`ChefZ_PortionManager.CanTakePortion()` handles two different worlds, and the
apparent contradiction is resolved deliberately:

* **No container declared anywhere** → the container requirement **lapses**. The
  portion is created without a container. A WARN is logged once per spec, not per
  take. "Better takeable without a bowl than not takeable at all." Nothing is
  lost; the worst case is a portion that costs no bowl.
* **Container system present, but the category unknown** → the take is
  **refused**. Once containers exist at all, an unknown category is a dead end —
  no container in the world belongs to it, so the search would never find
  anything. The same generosity here would be a blank cheque: a typo in a
  category would produce portions without containers and nobody would notice. The
  reason is written into the load report once, with the recipe ID, by
  `ChefZ_ContainerRegistry.AuditPortionSpecs()`.

## 7. What is left over after eating

Two separate moments. Do not confuse them.

### Serving (a portion is taken)

`consumesContainer: true` on the output means the take **uses up** a container
from the player's reach. `consumedOnServe = false` on the container class means
the container stays with the player — and then nothing may be returned later,
otherwise the player would have two after eating. That coupling lives in
`ChefZ_ContainerRegistry.ReturnsEmpty()` and exists exactly once.

### Eating (the portion is consumed)

`ChefZ_ContainerService.OnConsume()`. The return happens only when the dish is
**fully** eaten:

```c
if (dish.HasQuantity() && dish.GetQuantity() > EMPTY_EPSILON)
    return;
```

`EMPTY_EPSILON = 0.01`, matching vanilla: `ActionConsume.OnEndServer` checks
`GetQuantity() <= 0.01` and then sets the amount to 0
(`ActionConsume.c:51`). A continuous eat action regularly leaves fractions
standing, so aligning this number is the difference between "the plate comes
back" and "the plate almost always comes back". A partially eaten dish returns
nothing.

The binding is then read and **immediately invalidated** before anything is
created. If the return then fails, the return simply lapses with a WARN; a second
attempt would rerun on every subsequent `OnConsume`.

`ResolveReturnFor()` — two sources, in order:

1. **The item variable `m_ChefZ_ReturnContainer`.** Set at serving time and
   persisted. The dish may have been traded, stored, or carried across a server
   restart — the recipe is not known any more at the moment of eating.
2. **The class binding from `CfgChefZIngredients`.** This also covers dishes that
   never came from a ChefZ recipe, e.g. admin spawns.

The return creates a **new** class; it does not hand back the original object. So
there is no "this dish came from that bowl" link to persist, and a server restart
between cooking and eating changes nothing.

### `"AUTO"`

`returnContainer: "AUTO"` means "give back the empty version of the container
that was actually used". It is resolved **at serving time**, in
`ChefZ_PortionedFood_Base.ChefZ_TakePortion()` step 5, because that is the only
point where the used container is known. Whoever put in an enamel bowl gets an
enamel bowl back.

It is **not** resolvable at eating time and yields nothing there. If a dish
literally carries the string `AUTO` as its return value, a WARN is logged once
per class and nothing comes back. A recipe sets the resolved class on the item; a
class binding in `CfgChefZIngredients` must name a real class.

Return placement follows hands → inventory → the dish's former position. If all
three fail, the return lapses with a WARN. Never a crash — the dish was consumed
either way; only the container is lost.

## 8. Checklist for a new dish

1. One `CfgVehicles` class: `ChefZ_<Name> : ChefZ_ServedDish_Base` (or
   `...Bowl` for bowl dishes), with its **own** `class Nutrition` and
   `varQuantityInit` / `varQuantityMax` = 100 × the largest portion count of its
   recipes.
2. One script class: `class ChefZ_<Name> extends ChefZ_ServedDish_Base {}`.
3. In the recipe output: `cls`, `quantity = 100 × portions`,
   `quantityMode: "fixed"`, `returnContainer` as a fixed class name (or `""`).
4. An ingredient binding in `CfgChefZIngredients` with `containerCategory` and the
   same fixed `returnContainer`, for dishes that never ran through a recipe
   (admin spawn, loot).
5. Never `"AUTO"`: it resolves against the container used at serving, and
   nothing is served any more — the boot log reports it once per class.
