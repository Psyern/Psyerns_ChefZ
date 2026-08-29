# COT Compatibility

`Psyerns_ChefZ_COT_Comp` adds eight ChefZ spawn categories to the Object Spawner of
[Community Online Tools](https://github.com/Jacob-Mango/DayZ-CommunityOnlineTools).

That is the whole feature. It is an **admin tool**. It changes no game mechanic.

| | |
|---|---|
| **Categories added** | 8 |
| **Classes listed** | 168 |
| **Item classes defined** | 0 |
| **Recipes / nutrition / transforms touched** | none |
| **Vanilla or COT classes overridden** | 1 (`JMObjectSpawnerForm`, additively) |
| **Server-side game effect** | none |

---

## What it does and does not do

**Does:** adds one dropdown to the Object Spawner window listing nine entries —
*All (no ChefZ filter)* plus the eight ChefZ categories — and fills the class list
from a curated list when one of them is selected.

**Does not:**

* define or override a single item class. `units[]` and `weapons[]` in `CfgPatches`
  are empty and stay empty.
* touch recipes, nutrition, food states, processing or any ChefZ data record.
* change what can be spawned, or by whom. Spawning stays entirely COT's unmodified
  business, **including its permission checks** (`Entity.Spawn.Position`,
  `Entity.Spawn.Inventory`). The categories make items *findable*, not *spawnable*.
  An admin without the permission selects a category, sees a list, and gets the same
  refusal on spawn as before.
* alter the class list COT would otherwise show. Every entry passes the same filters
  COT applies in its own `UpdateList()`, in the same order.

If you are deciding whether to load it: the cost is one extra dropdown in one admin
window and one `modded class` on a COT form. The benefit is that finding
`ChefZ_MushroomCreamSauce` among 168 ChefZ classes stops being a typing exercise.

---

## Installation

The module is a separate mod folder, `Psyerns_ChefZ_COT_Comp`. Its `CfgPatches`
declares:

```cpp
requiredAddons[] =
{
    "JM_COT_Scripts",
    "ChefZ_Core", "ChefZ_Farming", "ChefZ_Ingredients", "ChefZ_Baking",
    "ChefZ_Meat", "ChefZ_Preservation", "ChefZ_Processing", "ChefZ_Cooking"
};
```

DayZ does not load an addon whose `requiredAddons` cannot be resolved — which is the
point. **Without COT installed, this PBO does not exist in the game at all** and its
`modded class` in `Scripts/5_Mission` is never compiled. ChefZ itself runs completely
unaffected.

The addon list names exactly the ChefZ addons the eight categories draw classes from.
`ChefZ_Registry` is deliberately absent: this module reads no registry and no data
record, it only carries class names.

See [Installation](Installation) for the general load order and
[Modules](Modules) for what each ChefZ addon contains.

---

## Using it as an admin

1. Open COT (default `\` ) and go to **Object Spawner**.
2. The categories dropdown, labelled **ChefZ Category**, sits at the bottom of the
   action panel, below COT's own action rows — not among the type buttons at the top.
3. Selecting a ChefZ category replaces the class list with that category's contents.
   The search box still works and still respects COT's
   *"filter by display name"* toggle.
4. Selecting **All (no ChefZ filter)** — the first entry — returns you to COT's
   unmodified behaviour.
5. Pressing any of COT's own type buttons (`ALL`, `EDIBLE`, `TRANSPORT`, …) resets
   the ChefZ dropdown to *All* automatically, because COT's type filter and the
   ChefZ filter share one variable and cannot both be active.

The selection survives closing the window: `JMObjectSpawnerModule` outlives the
form, so if you had a ChefZ category active last time, the dropdown restores it
rather than showing *All* over a filtered list.

### Why a dropdown and not eight buttons

COT's type-filter button strip lives in `object_types_actions_wrapper`, a panel
320 px tall. A `UIActionButton` is 30 px. COT already places ten buttons there —
300 of 320 px. Eight more would run out of the panel and overlap the area below.

The dropdown is added as an extra row in `m_SpawnerActionsWrapper`, the grid spacer
COT uses for its own four action rows; it carries *Size To Content V* and grows.
A select box rather than a drop-down list, because a drop-down list in the bottom
row would open downwards out of the window.

---

## The eight categories

Counted from `Scripts/4_World/ChefZ/Cot/ChefZ_CotCategories.c`. Every class appears
in **exactly one** category.

| # | Filter ID | Label | Classes |
|---:|---|---|---:|
| 1 | `chefz_cot_ingredients` | ChefZ / Ingredients | **30** |
| 2 | `chefz_cot_herbs` | ChefZ / Herbs and Spices | **32** |
| 3 | `chefz_cot_meat` | ChefZ / Meat and Sausage | **29** |
| 4 | `chefz_cot_baking` | ChefZ / Dough, Bread and Pasta | **8** |
| 5 | `chefz_cot_dairy` | ChefZ / Dairy | **4** |
| 6 | `chefz_cot_stations` | ChefZ / Stations and Tools | **10** |
| 7 | `chefz_cot_dishes` | ChefZ / Dishes | **50** |
| 8 | `chefz_cot_containers` | ChefZ / Containers | **5** |
| | | **Total** | **168** |

The filter IDs are lower-case and prefixed `chefz_cot_` on purpose. COT's own branch
passes `m_CurrentType` to `g_Game.IsKindOf`, and no config class name looks like
that — so even if one of these values ever reached COT's branch instead of ChefZ's,
it would yield an empty list rather than a wrong match.

### 1. Ingredients (12)

Everything that goes *into* a recipe and does not belong in a more specific
category: the found vegetables, egg, salt, wheat and flour. (Plants, seeds and
the knife-cut variants are gone since 2026-08-29 — vegetables are found whole,
like mushrooms, and used whole.)

```
ChefZ_Wheat            ChefZ_Flour            ChefZ_Onion
ChefZ_Garlic           ChefZ_Carrot           ChefZ_Cabbage
ChefZ_Egg              ChefZ_RawSalt          ChefZ_Salt
ChefZ_BoneBroth        ChefZ_TomatoSauce      ChefZ_CreamSauce
ChefZ_MushroomCreamSauce
```

Two placement decisions worth knowing:

* **The three sauces and the bone broth are here, not under Dishes.** Per
  `ChefZ_Cooking`'s own definition they are *ingredients of* a dish, not dishes.
  An admin looking for cream sauce looks for an ingredient.
* **Wheat and flour are here, not under Dough/Bread/Pasta**,
  even though they share the `ChefZ_GrainFoodBase` base class. They are the raw
  material of the chain, not its product.

### 2. Herbs and Spices

The complete herb chain in one category: fresh (found), dried, ground.

```
ChefZ_Parsley          ChefZ_Dill             ChefZ_Thyme
ChefZ_Rosemary         ChefZ_WildGarlic       ChefZ_PepperBerries
ChefZ_DriedParsley     ChefZ_DriedDill        ChefZ_DriedThyme
ChefZ_DriedRosemary    ChefZ_DriedWildGarlic  ChefZ_DriedPaprika
ChefZ_PaprikaPowder    ChefZ_DriedPeppercorns ChefZ_BlackPepper
ChefZ_HerbMix          ChefZ_HunterSeasoning
```

Deliberately **not** split by processing stage. An admin looks for "thyme", not for
"thyme, stage 2 of 3"; the five base classes behind these are not their problem.

### 3. Meat and Sausage (29)

Minced meat, fat, casing, raw and cooked sausage — plus the eight preserved goods.

```
ChefZ_MincedMeat       ChefZ_MincedPork       ChefZ_MincedVenison
ChefZ_MincedBoar       ChefZ_MincedChicken    ChefZ_MincedBear
ChefZ_RawSausage       ChefZ_RawPorkSausage   ChefZ_RawVenisonSausage
ChefZ_RawBoarSausage   ChefZ_RawHunterSausage ChefZ_RawSpicySausage
ChefZ_CookedSausage    ChefZ_PorkSausage      ChefZ_VenisonSausage
ChefZ_BoarSausage      ChefZ_HunterSausage    ChefZ_SpicySausage
ChefZ_SaltedMeat       ChefZ_DriedMeat        ChefZ_SmokedMeat
ChefZ_SaltedFish       ChefZ_DriedFish        ChefZ_SmokedFish
ChefZ_SmokedSausage    ChefZ_DrySausage
```

**Note the compromise.** The preserved goods from `ChefZ_Preservation` have their
own base class (`ChefZ_PreservedFood_Base`) and their own addon, and they are still
listed here. Reason: the agreed eight categories include no *Preserves* category,
and salted meat, dried meat and dry sausage are what an admin looks for under meat
and sausage. Adding a ninth category would have been an unilateral extension of the
brief.

The price of that decision, stated openly: **`ChefZ_SaltedFish`, `ChefZ_DriedFish`
and `ChefZ_SmokedFish` are therefore also under "Meat and Sausage".** If you are
looking for fish, that is where it is.

### 4. Dough, Bread and Pasta (5)

```
ChefZ_Dough            ChefZ_RawPasta         ChefZ_DriedPasta
ChefZ_Bread            ChefZ_Flatbread
```

Yeast is here because it exists solely to raise dough. The chain's raw materials
(wheat, flour) are under Ingredients.

### 5. Dairy (3)

```
ChefZ_Cream            ChefZ_Butter           ChefZ_Cheese
```

Milk is vanilla `PowderedMilk` since 2026-08-29 and needs no ChefZ spawner entry.

The smallest category, and the one that most clearly defeats COT's own type filter:
these four inherit from four completely different vanilla classes (`PowderedMilk`,
`Marmalade`, `Lard`, `BoxCerealCrunchin`).

`ChefZ_Egg` belongs to the *dairy* content slice but is **not** here — an egg is not
a dairy product. It is under Ingredients. The butter churn and cheese press are
appliances and are under Stations and Tools.

### 6. Stations and Tools (9)

```
ChefZ_GrainMill        ChefZ_PastaMachine     ChefZ_Mortar
ChefZ_DryingRack       ChefZ_ButterChurn      ChefZ_CheesePress
ChefZ_SaltPan          ChefZ_MeatGrinder      ChefZ_Smoker
```

The only category with nothing edible in it, and in daily operation the most used
one: a station a player has lost is something the admin replaces, and to replace it
he has to find it. See [Processing-Stations](Processing-Stations).

### 7. Dishes (50)

25 dishes, each in two forms.

```
ChefZ_TacticalBreakfastBulk     ChefZ_TacticalBreakfast
ChefZ_ScrambledEggSausageBulk   ChefZ_ScrambledEggSausage
ChefZ_FarmersBreakfastBulk      ChefZ_FarmersBreakfast
ChefZ_CheeseFlatbreadBulk       ChefZ_CheeseFlatbread
ChefZ_SausageBreadPlateBulk     ChefZ_SausageBreadPlate
ChefZ_MushroomPanBulk           ChefZ_MushroomPan
ChefZ_PotatoPancakesBulk        ChefZ_PotatoPancakes
ChefZ_MeatDumplingsBulk         ChefZ_MeatDumplings
ChefZ_MilkRiceBulk              ChefZ_MilkRice
ChefZ_HoneyBreadPlateBulk       ChefZ_HoneyBreadPlate
ChefZ_HunterStewBulk            ChefZ_HunterStewBowl
ChefZ_FishermanStewBulk         ChefZ_FishermanStewBowl
ChefZ_VegetableSoupBulk         ChefZ_VegetableSoupBowl
ChefZ_BoneBrothSoupBulk         ChefZ_BoneBrothSoupBowl
ChefZ_ChernarusChiliBulk        ChefZ_ChernarusChiliBowl
ChefZ_SurvivorSpaghettiBulk     ChefZ_SurvivorSpaghetti
ChefZ_SausagePastaBulk          ChefZ_SausagePasta
ChefZ_HunterPastaBulk           ChefZ_HunterPasta
ChefZ_CreamMushroomPastaBulk    ChefZ_CreamMushroomPasta
ChefZ_MacAndCheeseBulk          ChefZ_MacAndCheese
ChefZ_SausagePotatoesBulk       ChefZ_SausagePotatoes
ChefZ_HunterPlateBulk           ChefZ_HunterPlate
ChefZ_BloodSausagePlateBulk     ChefZ_BloodSausagePlate
ChefZ_FishPotatoPlateBulk       ChefZ_FishPotatoPlate
ChefZ_BeanSausagePlateBulk      ChefZ_BeanSausagePlate
```

**Both forms are listed on purpose.** `...Bulk` is the batch inside the cooking
device (`ChefZ_PortionedDish_Base`); the other is a plated serving
(`ChefZ_ServedDish_Base`). They are different items with different behaviour, and
offering only one would be wrong half the time. They are listed in pairs so the
list shows them next to each other. See
[Portions-and-Containers](Portions-and-Containers).

### 8. Containers (5)

```
ChefZ_EmptyPlate       ChefZ_EmptyBowl        ChefZ_EmptyCan
ChefZ_EmptyJar         ChefZ_EmptyBox
```

Separated from Stations and Tools even though both are inedible: a plate is a
consumable handed out in quantity, a cheese press is a single item.

---

## Why class lists and not base classes

COT's own type filter is a single base class name evaluated with `g_Game.IsKindOf`:

```cpp
if (m_Module.m_CurrentType == "" || g_Game.IsKindOf( strNameLower, m_Module.m_CurrentType ))
```

For ChefZ that does not work:

| Category | Problem |
|---|---|
| Dairy | `ChefZ_Cream` extends `Marmalade`, `ChefZ_Butter` extends `Lard`, `ChefZ_Cheese` extends `BoxCerealCrunchin`. Four goods, four unrelated vanilla branches — no common base to collect them by. |
| Stations | `ChefZ_ButterChurn` extends `Pot`, `ChefZ_CheesePress` extends `Cauldron`, the rest `Inventory_Base`. Same picture. |
| Herbs | spread across three bases (`ChefZ_FreshHerbBase`, `ChefZ_DriedHerbBase`, `ChefZ_SpiceBase`). |

The alternative would have been to rewrite item inheritance so that an admin filter
looks tidy. That is changing game mechanics for the sake of a tool — so: explicit
lists.

---

## Where the 168 come from, and the 17 that are missing

The class names come from the `config.cpp` files under
`Psyerns_ChefZ_Core/Addons/` and nowhere else. Every name listed is a class defined
there with a body and `scope = 2`. Cross-checked against the `classes` lists in
`Psyerns_ChefZ_Core/_deltas/*.json`: both sources name the same **185** classes, of
which **168** have `scope = 2`. Those 168 are listed.

The 17 missing ones are the `scope = 0` base classes (`ChefZ_GrainFoodBase`,
`ChefZ_MeatItemBase`, `ChefZ_ServedDish_Base`, …). They are absent on purpose:
they are not spawnable, COT would discard them anyway (`scope == 0` → `continue`),
and listing them would offer the admin seventeen dead rows.

See [Delta-Protocol](Delta-Protocol) for what the `_deltas/*.json` files are.

---

## Robustness: a missing addon just loses its entries

`ChefZ_CotCategories.c` validates **nothing**. The check happens at runtime in
`ChefZ_CotObjectSpawner.ChefZ_FillClassList`, immediately before an entry goes into
the list, and it applies exactly the same filters COT applies in its own
`UpdateList()`, in the same order:

1. `g_Game.ConfigIsExisting(path)` — **this is the one extra check**. A class from a
   ChefZ addon that is not loaded drops out here, silently and without an error.
2. `scope == 0`, or `scope == 1` without `m_AllowRestrictedClassNames` → skip.
3. missing `model`, empty model, or the placeholder model `"bmp"` → skip.
4. `m_Module.IsExcludedClassName(...)` → skip.
5. search-box keyword match (against class name, or display name if COT's
   *filter by display name* toggle is on) → skip if no match.

Steps 2–5 are deliberately identical to COT's. An entry COT would discard in its
*All* branch is discarded here too — otherwise a ChefZ category would show items COT
offers nowhere else, and the filter would be a bypass around
`m_AllowRestrictedClassNames` and `IsExcludedClassName`.

Practically: if you run a server with only some ChefZ addons loaded, the categories
shrink to match. There is no ghost class and no error message.

The flip side: **a mistyped class name in the table does not fail anywhere.** It is
silently discarded at runtime, and an admin ends up searching for an item that never
existed. Anyone adding a class must verify it actually exists with `scope = 2` in a
`config.cpp` under `Psyerns_ChefZ_Core/Addons/`, and must add the owning addon to
`requiredAddons[]`. See [Adding-Content](Adding-Content).

---

## The code footprint

Two files.

| File | Layer | Contents |
|---|---|---|
| `Scripts/4_World/ChefZ/Cot/ChefZ_CotCategories.c` | 4_World | The table: eight names, eight class lists, plus `Get()`, `Find(filterId)` and `IndexOf(filterId)`. No mission access, no GUI, no engine types. Pure data. |
| `Scripts/5_Mission/ChefZ/Cot/ChefZ_CotObjectSpawner.c` | 5_Mission | `modded class JMObjectSpawnerForm` — the only COT code extended, and only one class. |

The table sits in 4_World rather than next to the modded class on purpose: whoever
wants to know **what** ChefZ reports to COT reads one file; whoever wants to know
**how** reads the other.

The modded class overrides three methods, and every one either calls `super` or
falls back to it:

* **`OnInit()`** — calls `super` first, then appends the select box. If
  `m_SpawnerActionsWrapper` is null (missing layout), it returns and COT runs exactly
  as it would without this mod. A missing admin filter is an annoyance; a crash when
  opening the spawner is one more.
* **`UpdateList()`** — `ChefZ_CotCategories.Find(m_Module.m_CurrentType)` returns
  a category only for the eight ChefZ filter IDs. For COT's own types, for the empty
  string, and for anything a third mod might ever write into `m_CurrentType`, it
  returns `NULL` and **COT's original runs, line for line unchanged**.
* **`SetListType()`** — resets the dropdown to *All* on click before `super` writes
  its own value into `m_CurrentType`, with `sendEvent = false` so the reset does not
  trigger a stray `UpdateList()`.

`m_ClassList`, `m_SearchBox`, `m_Module` and `m_SpawnerActionsWrapper` are declared
`private` in `JMObjectSpawnerForm`. In Enforce they are still reachable from a
`modded class` — the modded class *is* the class, not a descendant. TerjeMods'
own `TerjeCompatibilityCOT` does the same thing on the same class.

---

## Localisation

`Language/stringtable.csv` ships the dropdown label, the *All* entry and the eight
category names in the project's standard column set (`original` plus 13 languages).
English strings:

| Key | English |
|---|---|
| `STR_CHEFZ_COT_CATEGORY` | ChefZ Category |
| `STR_CHEFZ_COT_CAT_NONE` | All (no ChefZ filter) |
| `STR_CHEFZ_COT_CAT_INGREDIENTS` | ChefZ / Ingredients |
| `STR_CHEFZ_COT_CAT_HERBS` | ChefZ / Herbs and Spices |
| `STR_CHEFZ_COT_CAT_MEAT` | ChefZ / Meat and Sausage |
| `STR_CHEFZ_COT_CAT_BAKING` | ChefZ / Dough, Bread and Pasta |
| `STR_CHEFZ_COT_CAT_DAIRY` | ChefZ / Dairy |
| `STR_CHEFZ_COT_CAT_STATIONS` | ChefZ / Stations and Tools |
| `STR_CHEFZ_COT_CAT_DISHES` | ChefZ / Dishes |
| `STR_CHEFZ_COT_CAT_CONTAINERS` | ChefZ / Containers |

---

## Not yet confirmed in game

Code-verified but **not** confirmed against a running server with COT loaded. Until
someone does, these belong to [Known-Limitations](Known-Limitations):

* That the select box actually renders in `m_SpawnerActionsWrapper` and does not
  overlap COT's action rows at every UI scale.
* That the selection restore on reopening the spawner picks the right entry.
* That the search box interaction (including COT's *filter by display name* toggle)
  behaves identically inside a ChefZ category and inside COT's *All* branch.
* Behaviour when another mod also extends `JMObjectSpawnerForm` — Enforce chains the
  overrides, but the chaining order with a third mod is untested.
* That every one of the 168 names resolves at runtime. A wrong name is discarded
  silently by design, so a typo would show up as a short list rather than an error.

---

## See also

* [Modules](Modules) — the addon list the categories draw from
* [Installation](Installation) — load order
* [Terje-Compatibility](Terje-Compatibility) — the other two compatibility modules
* [Processing-Stations](Processing-Stations) — what the ten station classes do
* [Portions-and-Containers](Portions-and-Containers) — why every dish has two classes
* [Adding-Content](Adding-Content) — adding a class to a category
* [Known-Limitations](Known-Limitations)
