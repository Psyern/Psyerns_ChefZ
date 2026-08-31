# Adding Content

How to add a dish, an ingredient or a processing station to ChefZ **without
touching `ChefZ_Core`**.

If you find yourself editing a file under `Addons/ChefZ_Core/Scripts/**` to add
content, stop — something generic has ended up in the wrong place, or the core
is genuinely missing a capability. `chefzcore.mjs` will fail the build for any
content identifier that appears there.

Background reading: [Architecture](Architecture) for the three configuration
ranks, [Delta Protocol](Delta-Protocol) for shared vocabulary.

## Ground rules

**1. Path root is the addon's folder name.**
`$PREFIX$` contains the folder name, and that is the root of every runtime path
— in `files[]`, in `dataFiles[]`, everywhere. A file in
`Addons/ChefZ_Cooking/Config/Recipes/Dishes_A.json` is addressed as
`ChefZ_Cooking/Config/Recipes/Dishes_A.json`. There is no second form.

**2. Every data file must be declared.**
PBO contents cannot be enumerated at runtime, so the core never scans. A JSON
file without a `dataFiles[]` entry in some `CfgChefZ` slice is a dead file. This
has already cost this project one milestone — see
[Modules](Modules#chefz_registry).

**3. One document, one `kind`.**

```json
{ "kind": "recipe", "schemaVersion": 1, "records": [ ] }
```

**4. No comments in JSON.** See [Pitfalls](#pitfalls).

**5. Class names are `ChefZ_PascalCase`.** `naming.mjs` enforces
`^ChefZ_[A-Z][A-Za-z0-9]*(_Base)?$` and additionally checks for collisions with
vanilla and known third-party class names.

**6. Run the validator before you commit.**

```
node tools/chefz-validate/index.mjs
```

## A: adding a new dish

Worked example: **`ChefZ_SausagePasta`**, an existing dish in the `dishes-a`
slice of `ChefZ_Cooking`. Every file below is a real file in the repository.

A dish is **one** class (since 2026-08-29): the served dish that forms in the
cooking vessel and is eaten. Portions are vanilla quantity — the recipe sets
`quantity = 100 × portions`, and `PlayerStomach` rates nutrition per 100 units.

### Files touched

| File | What goes in |
|---|---|
| `ChefZ_Cooking/config.cpp` → `CfgPatches units[]` | the class name |
| `ChefZ_Cooking/config.cpp` → `CfgVehicles` | the class, with `Nutrition` and its quantity block |
| `ChefZ_Cooking/config.cpp` → `CfgChefZIngredients` | the class, as ingredient binding |
| `ChefZ_Cooking/config.cpp` → `CfgChefZ` | the `dataFiles[]` entry, if the recipe file is new |
| `ChefZ_Cooking/Config/Recipes/Dishes_A.json` | the recipe record |
| `ChefZ_Cooking/stringtable.csv` | 2 keys: name and description |
| `Psyerns_ChefZ_Core/_deltas/dishes-a.json` | the class name in `classes` |
| *(optional)* a script file | empty derivations, see below |

### Step 1 — the config class

```cpp
class ChefZ_SausagePasta : ChefZ_ServedDish_Base
{
    scope = 2;
    displayName = "#STR_CHEFZ_ITEM_SAUSAGEPASTA";
    descriptionShort = "#STR_CHEFZ_ITEM_SAUSAGEPASTA_DESC";
    model = "\dz\gear\cooking\FryingPan.p3d";   // PROXY, no own mesh
    weight = 490;
    lifetime = 14400;
    varQuantityInit = 200;   // 100 per portion; largest recipe has 2
    varQuantityMax = 200;

    class Nutrition { fullnessIndex = 3.5; energy = 1270; water = 70;
                      nutritionalIndex = 60; toxicity = 0; agents = 0;
                      digestibility = 1; };   // 3.5 x 200 qty = 700 stomach volume
};
```

Note what is **not** here:

- The dish has **no `Food` node at all**, deliberately. Without
  `Food > FoodStages`, `ItemBase.HasFoodStage()` is false, vanilla's
  `Cooking.ProcessItemToCook` skips it (`Cooking.c:47`), and the finished dish
  cannot burn while it sits in the pot.
- `ChefZ_ServedDish_Base` carries **no `Nutrition` block**. If it did, a
  dish that forgot its own would inherit one and look green in the validator —
  and `PlayerStomach` only registers classes with their own nutrition data, so
  the failure would be silent.
- `energy` and `water` are **per 100 units** of quantity (`PlayerStomach.c:92-93`
  divides by 100). **`fullnessIndex` is not**: the engine computes stomach volume
  as `fullnessIndex * units eaten` with **no** division (`PlayerStomach.c:86`),
  one bite eats 25 units, and the vomit threshold is a stomach volume of 2000
  (`PlayerConstants.c:208`). So keep `fullnessIndex` in vanilla's 0.5–4 band and
  check `fullnessIndex * varQuantityMax` — the whole item — stays below ~950
  (the "stuffed" badge starts at 1000). A dish with `fullnessIndex = 230` makes
  the player vomit on the first bite; that exact mistake shipped once.
  Two portions in the pot are `quantity = 200` of the same item.

### Step 2 — the ingredient binding

In `CfgChefZIngredients` of the same `config.cpp`:

```cpp
class ChefZ_SausagePasta : ChefZ_DishesAPlate {};
```

where the base in the same block is:

```cpp
class ChefZ_DishesAPlate
{
    defaultState      = "COOKED";
    quantityUnit      = "PIECE";
    unitsPerWholeItem = 1;
    decays            = 1;
    containerCategory = "PLATE";
    returnContainer   = "ChefZ_EmptyPlate";
};
```

`returnContainer` is what leaves the empty plate behind on the last bite; it is a
fixed class, never `"AUTO"` (nothing is served any more, so there is no "used
container" to resolve). Change it to `"ChefZ_EmptyBowl"` and the dish leaves a
bowl instead — `ChefZ_MilkRice` does exactly that.

### Step 3 — the recipe

In `ChefZ_Cooking/Config/Recipes/Dishes_A.json`, abridged (the full record has
eight slots and two grade rules):

```json
{
  "id": "RCP_ChefZ_SausagePasta",
  "contexts": [
    { "deviceClasses": ["FryingPan", "Pot"], "methods": ["BAKING", "BOILING"] }
  ],
  "completion": "TIMED",
  "cookSeconds": 200.0,
  "minTemperature": 60.0,
  "slots": [
    { "slotId": "pasta",   "match": { "category": "PASTA" },
      "minCount": 1, "maxCount": 3, "consume": "whole" },
    { "slotId": "sausage", "match": { "category": "SAUSAGE" },
      "minCount": 1, "maxCount": 3, "consume": "whole", "gradePoints": 1 },
    { "slotId": "fat",     "match": { "anyOf": [ { "category": "FAT" },
                                                 { "category": "BUTTER" } ] },
      "minCount": 1, "maxCount": 1, "consume": "whole" },
    { "slotId": "salt",    "match": { "category": "SALT" },
      "minCount": 1, "maxCount": 1, "optional": true,
      "unit": "GRAM", "amount": { "min": 5.0 },
      "consume": "amount", "consumeAmount": 5.0, "gradePoints": 2 }
  ],
  "policy": { "extraItems": "forbid", "forbiddenStates": ["BURNT", "ROTTEN"] },
  "qualityTierSet": "DISH_DEFAULT",
  "outputs": [
    {
      "cls": "ChefZ_SausagePasta",
      "quantity": 200, "quantityMode": "fixed",
      "returnContainer": "ChefZ_EmptyPlate",
      "inheritQuality": true, "inheritState": true,
      "setState": "COOKED"
    }
  ],
  "effects": ["CHEFZ_WARM_MEAL", "CHEFZ_HEARTY_MEAL"],
  "nutritionModifier": 1.1,
  "priority": 0
}
```

Field-by-field reference is on [Recipe Reference](Recipe-Reference); the
mechanics are on [Recipes](Recipes).

The important line for the design rule: the mandatory slots want `PASTA` and
`SAUSAGE`, and both categories are only satisfied by `ChefZ_` classes. That is
what keeps `chefzvanilla.mjs` quiet — see
[Architecture](Architecture#53-the-rule-from-the-other-side). A recipe whose
mandatory slots can all be filled with vanilla items is rejected by the
validator, because it would hijack vanilla cooking.

`completion` is one of `ON_STAGE`, `TIMED`, `INSTANT`.

### Step 4 — declare the file, if it is new

```cpp
class CfgChefZ
{
    class ChefZ_DishesA
    {
        chefzApiVersion      = 1;
        loadOrder            = 330;
        handcraftRecipeSlots = 0;
        dataFiles[] = { "ChefZ_Cooking/Config/Recipes/Dishes_A.json" };
    };
};
```

`handcraftRecipeSlots = 0` because this slice brings no `HANDCRAFT` transform.
If yours does, see [Pitfalls](#pitfalls).

### Step 5 — the strings

Four rows in `ChefZ_Cooking/stringtable.csv`:

```
STR_CHEFZ_ITEM_SAUSAGEPASTA
STR_CHEFZ_ITEM_SAUSAGEPASTA_DESC
STR_CHEFZ_ITEM_SAUSAGEPASTA_BULK
STR_CHEFZ_ITEM_SAUSAGEPASTA_BULK_DESC
```

`stringtable.mjs` checks that every `#STR_` reference in every config and data
file resolves.

### Step 6 — the delta

Add both class names to the `classes` array of
`Psyerns_ChefZ_Core/_deltas/dishes-a.json`. A dish needs no other delta section
unless it introduces a new category, tag or nutrition record — see
[Delta Protocol](Delta-Protocol#8-checklist-for-a-content-author).

### Step 7 — the script class (optional, but be consistent)

```c
class ChefZ_SausagePasta extends ChefZ_ServedDish_Base {}
```

It would be empty. The eat actions, persistence, sync and the container return
all come from the core.

The engine walks the *config* parent chain up to `ChefZ_PortionedDish_Base` /
`ChefZ_ServedDish_Base` when a config class has no script class of its own, so
these lines are not strictly required — and in fact the twenty `dishes-a`
classes do not have them, while the twenty `dishes-b` classes do. The explicit
derivation is the place where a later signature dish can get its own behaviour
without first creating a class. Pick one convention per slice.

**If a script class here ever grows a line of logic, that is a sign something
generic has landed in content.**

## B: adding a new ingredient

Worked example: **`ChefZ_Butter`** in `ChefZ_Ingredients`.

### Step 1 — the config class

Derive from a **vanilla** class, never from a core class:

```cpp
class ChefZ_Butter : Lard
{
    scope = 2;
    displayName = "#STR_CHEFZ_ITEM_BUTTER";
    descriptionShort = "#STR_CHEFZ_ITEM_BUTTER_DESC";
    weight = 250;
    itemSize[] = {2, 1};
    varQuantityInit = 100;
    varQuantityMin = 0;
    varQuantityMax = 100;
    varQuantityDestroyOnMin = 1;
    lifetime = 43200;

    class Nutrition { fullnessIndex = 2; energy = 600; water = 20;
                      nutritionalIndex = 8; toxicity = 0; digestibility = 1; };
                      // 2 x 100 qty = 200 stomach volume (see the dish note on
                      // fullnessIndex units in section A)

    class Food { /* FoodStages and FoodStageTransitions */ };
};
```

Butter is a mandatory ingredient in `RCP_ChefZ_MushroomPan`, so it goes into a
cooking vessel and therefore needs stages **and** transitions. An ingredient
that is only ever eaten raw — parsley, salt, most spices — has no `Food` node at
all, and is then correctly not cookable.

Watch the inheritance: `ChefZ_Butter` inherits from `Lard`, and `Lard` in vanilla
is a frying fat *with* food stages. Without its own `Food` block, `ChefZ_Butter`
would inherit Lard's per-stage nutrition values and the `class Nutrition` above
would have no effect at all.

### Step 2 — the script class

```c
class ChefZ_Butter extends ChefZ_Edible_Base {}
```

Config class derives from vanilla, script class derives from
`ChefZ_Edible_Base`. Without the script derivation the item carries no ChefZ
state — not an error, just less.

`ChefZ_Butter` is an exception: it has no own script class, so the engine falls
through to `Lard`, whose `Lard.c` overrides `CanBeCooked()` with `true`.

`ChefZ_Edible_Base` does not hard-code `CanBeCooked()`. It computes the answer
from the class's own data: a class that declares `Food > FoodStages` **and**
`FoodStageTransitions` is cookable, one that does not is not. That is why
declaring the transitions is not optional (see [Pitfalls](#pitfalls)).

For a non-edible item (a tool, a container, a station), derive the script class
from `ChefZ_Item_Base` instead.

### Step 3 — the ingredient binding

Either in `CfgChefZIngredients` (rank 1) or in a JSON document with
`"kind": "ingredient"` (rank 2). Both work; rank 1 is required only if the
client needs to know. In `ChefZ_Ingredients/Config/Ingredients/Dairy.json`:

```json
{
  "id": "ChefZ_Butter",
  "categories": ["DAIRY", "BUTTER"],
  "tags": ["CHEFZ_DAIRY"],
  "decays": true
}
```

Other fields that matter: `defaultState`, `quantityUnit` (`PIECE` or `GRAM`),
`unitsPerWholeItem`, `containerCategory`, `returnContainer`.

### Step 4 — declare the file, add the strings, file the delta

Same as steps 4–6 above. If your ingredient introduces a new category or tag, or
needs a nutrition record, it goes into your slice's delta — see
[Delta Protocol](Delta-Protocol).

### Step 5 — how does it get made?

An ingredient that nothing produces is unreachable. Add a transform:

```json
{
  "id": "TR_PeppercornsToBlackPepper",
  "process": "PROCESS_GRIND_SPICE",
  "stationsAllowed": ["ChefZ_Mortar"],
  "durationOverrideSec": 20,
  "qualityRule": "MEAN",
  "inputs":  [ { "slotId": "corns", "match": { "cls": "ChefZ_DriedPeppercorns" },
                 "minCount": 1, "consume": "whole" } ],
  "outputs": [ { "cls": "ChefZ_BlackPepper", "setState": "PREPARED",
                 "quantityMode": "fromInput" } ],
  "priority": 100
}
```

See [Production Chains](Production-Chains).

## C: adding a new processing station

Worked example: **`ChefZ_Mortar`** in `ChefZ_Processing`. Three declarations,
and nothing else.

### Step 1 — the config class

Derive from a vanilla class:

```cpp
class ChefZ_Mortar : ChefZ_HerbStationBase   // which itself derives from Inventory_Base
{
    scope = 2;
    displayName = "#STR_CHEFZ_ITEM_MORTAR";
    descriptionShort = "#STR_CHEFZ_ITEM_MORTAR_DESC";
    model = "\dz\gear\cooking\CookingPot.p3d";
    itemSize[] = {3, 2};
    weight = 1800;
};
```

Do **not** derive a station from `Pot` or `Cauldron`. Those two are declared in
`CfgChefZDevices` as cooking vessels; a station that inherits from them would be
picked up by the cooking adapter. This was a real blocker in this project — two
dairy stations inherited from vanilla cookware and had to be moved to
`Inventory_Base`.

### Step 2 — the station record

`ChefZ_Processing/Config/Processing/HerbStations.json`:

```json
{
  "kind": "station",
  "schemaVersion": 1,
  "records": [
    {
      "id": "ChefZ_Mortar",
      "stationCategories": ["MORTAR"],
      "processes": ["PROCESS_GRIND_SPICE", "PROCESS_GRIND_HERB"],
      "parallelSlots": 1,
      "speedMultiplier": 1.0,
      "needsFuel": false
    }
  ]
}
```

Which station offers which process lives **here**, not in the script.

### Step 3 — the script class

```c
class ChefZ_Mortar extends ChefZ_ProcessingStation_Base {}
```

Empty. The action, its condition, the timer, persistence and the process runner
are all in the core. Any line beyond this is logic that already exists.

### Step 4 — the process

A station is useless without a process. In `CfgChefZProcesses` (rank 1, so the
client can build the action condition):

```cpp
class PROCESS_GRIND_SPICE
{
    exec = "STATION_ACTION";
    displayName = "#STR_CHEFZ_PROC_GRIND_SPICE";
    baseDurationSec = 20.0;
};
```

`exec` is one of:

| Value | Meaning |
|---|---|
| `HANDCRAFT` | vanilla's craft system, **max. 2 inputs**, needs a `handcraftRecipeSlots` reservation |
| `STATION_ACTION` | the player works the station actively |
| `STATION_TIMED` | the station ticks on without the player |

A process may require a tool group (`toolGroups[] = {"CUTTING_TOOL"}`). An
unknown tool group makes the core **reject** the process — without a tool check
it would be too easy to trigger.

### Step 5 — declare, string, delta

`dataFiles[]` entry, `stringtable.csv` rows, and the station class in your
delta's `classes`. If the process is new, add it to the delta's `processes`
section too — that section produces no registry file, but it is where cross-slice
process id collisions are caught. See
[Delta Protocol](Delta-Protocol#processes--the-deliberate-omission).

See [Processing Stations](Processing-Stations).

## Pitfalls

These are the ones that actually bit this project. Each was found late, and each
was silent.

### A class without `Food > FoodStageTransitions` never finishes cooking

Vanilla advances the food stage of every item in the pot. If
`FoodStage.GetNextFoodStageType` finds no transition, it falls back to
`FoodStageType.BURNED`. Your ingredient **burns on its first stage change**
instead of cooking.

Worse in the other direction: `ChefZ_Edible_Base` derives `CanBeCooked()` from
exactly these transitions. A class that declares `FoodStages` but no
`FoodStageTransitions` is not cookable at all, stays `Raw` forever, and no
`ON_STAGE` recipe using it can ever complete. That was a real blocker whose
validator output was byte-identical before and after the bug; `chefzcookable.mjs`
was written afterwards to catch it.

`transition_to` and `cooking_method` are **numbers**, not names, because
`SetupFoodStageTransitionMapping` reads them with `ConfigGetInt`:

```
FoodStageType:     RAW 1, BAKED 2, BOILED 3, DRIED 4, BURNED 5, ROTTEN 6
CookingMethodType: NONE 0, BAKING 1, BOILING 2, DRYING 3, TIME 4
```

### A food class without an eating action cannot be eaten

Vanilla does **not** put eating actions on `Edible_Base`. It puts them on each
food class individually (`Rice.c`, `Potato.c`, `Marmalade.c`). Without one, the
game simply never offers your finished dish for eating — no error, no log line,
the action is just absent.

ChefZ solves this once for all 28 dishes on `ChefZ_ServedDish_Base`:

```c
override void SetActions()
{
    super.SetActions();
    AddAction(ActionForceFeed);
    AddAction(ActionEatBig);
}
```

If you add a food class outside that chain, add the action yourself.
`chefzcookable.mjs` rule C checks for this.

### A missing `class Nutrition` means a silently useless bite

`PlayerStomach.InitData` only registers classes that have `Nutrition` **or**
`Food` and `scope != 0`. Without either, the item is eaten, disappears, and
nourishes nothing. There is no error message for this.

`chefznut.mjs` reports it as a warning — currently 8 of them, all for classes
that arguably should not be eaten anyway (seeds, empty plates, raw salt).

### `handcraftRecipeSlots` must be declared, or no craft recipe appears

Vanilla builds its recipe list in the `MissionBase` constructor, and a craft
action transmits a recipe's *position* in that list, not its identity. ChefZ
therefore reserves its slots at the same point of the `modded class` chain on
both client and server, and the count has to be known before any ChefZ data has
loaded.

The only source available that early is the engine config, so every slice with
`exec = "HANDCRAFT"` transforms declares:

```cpp
class ChefZ_Produce
{
    chefzApiVersion      = 1;
    loadOrder            = 220;
    handcraftRecipeSlots = 12;
    dataFiles[]          = { };
};
```

Declare too few and the surplus transforms are **rejected** with a plain-text
error line — loudly, deterministically, and identically on both sides. That is
the correct direction for this failure: a missing recipe costs a config entry, a
recipe that produces the wrong item costs trust.

Rank 3 is excluded from the count on purpose. Only the server sees an overlay,
and a slot count derived from it would differ between the two sides.

### No `_comment` fields in JSON

`ChefZ_ConfigSelfTest.ProbeUnknownFieldTolerance()` records that the Enforce
serialiser's tolerance for unknown JSON fields is **not established**. If it is
intolerant, a file with a comment field is not partially loaded — it is
discarded **completely**, and half a content module disappears without anything
looking wrong.

The rule is therefore absolute: no comment fields, at any level, in any data
file or delta. Notes go in a `README.md` next to the file. Several already exist
— `ChefZ_Meat/Config/Ingredients/README.md`,
`ChefZ_Cooking/Config/Recipes/README_Serving.md` — and each begins by saying its
text used to live in the JSON.

### Path root is the addon folder name, not the mod name

`ChefZ_Cooking/Config/...`, never `Psyerns_ChefZ_Core/Addons/ChefZ_Cooking/...`
and never `Psyerns_ChefZ/...`. A wrong prefix in `files[]` makes DayZ skip the
script module **silently, without an RPT entry**. A wrong prefix in
`dataFiles[]` produces an error naming both path forms that were tried, so the
symptom is at least visible — but if it happens for every file in a slice, the
prefix is almost always the cause.

### Do not re-open a class body a second time

`configcpp.mjs` reports any class path defined twice with a body as an error,
because the later definition silently overrides the earlier one. This is why the
Terje medicine comp mod puts its parameters under an own root
`CfgChefZTerjeMedicine` instead of re-opening `CfgVehicles/ChefZ_ThymeTea`.

It is also why `ChefZ_Registry`'s `CfgChefZ` node is called
`ChefZ_MergedRegistry` and not `ChefZ_Registry` — a node with the same name as
the `CfgPatches` entry counts as a duplicate.

## Before you commit

```
node tools/chefz-validate/index.mjs
```

Exit code 0 means no errors. Warnings are readable output, not noise to skip —
the current tree has 90 of them and each is explained in
[Validation](Validation).

Then, if you changed a delta, re-run the registry integrator; `deltas.mjs`
check 6 will otherwise report that the merge is incomplete.

A green validator is not a working mod. Nothing in this repository has been
compiled, packed or run in DayZ — see [Known Limitations](Known-Limitations),
and expect the first compile to produce surprises.

## Related pages

- [Architecture](Architecture) — ranks, load order, the design rule
- [Modules](Modules) — which addon owns what
- [Delta Protocol](Delta-Protocol) — shared vocabulary
- [Recipes](Recipes) and [Recipe Reference](Recipe-Reference)
- [Food States](Food-States), [Quality and Nutrition](Quality-and-Nutrition),
  [Portions and Containers](Portions-and-Containers),
  [Processing Stations](Processing-Stations),
  [Production Chains](Production-Chains)
- [Validation](Validation)
