# Processing Stations

ChefZ ships **9 processing stations**. They run *transforms*,
not recipes: a transform takes items out of the station's cargo and puts different
items back. All 9 station records live in
`Psyerns_ChefZ_Core/Addons/ChefZ_Processing/Config/`, and all nine `CfgVehicles`
classes live in that module's `config.cpp`.

## Stations are deliberately not cookware

Every station inherits from **`Inventory_Base`** in `config.cpp` and from
`ChefZ_ProcessingStation_Base` in script. Neither is a pot, and that is the point.

It was wrong once. Up to Gate 4 the Butter Churn inherited from `Pot` and the
Cheese Press from `Cauldron`. Because both vanilla classes are registered in
`CfgChefZDevices`, `ChefZ_CookingDeviceAdapter.BuildDescriptor` walked the
`CfgVehicles` chain, found the base and enabled the station as a cooking device:
a butter churn was a 4-portion pot, and every recipe with `deviceClasses ["Pot"]`
matched inside it. Vanilla was no better — `FireplaceBase.CookOnDirectSlot` calls
`Cooking.CookWithEquipment` for anything in a direct-cooking slot without checking
`IsCookware()`, so the inherited `inventorySlot[]` was enough to put a station on a
fireplace.

`Inventory_Base` closes both paths. **No station appears in any recipe context, and
nothing can be cooked inside one.**

## The nine stations

| Station | Class | Category | Processes | Parallel slots | Cargo | Needs fuel | Weight | Proxy model |
|---|---|---|---|---|---|---|---|---|
| Grain Mill | `ChefZ_GrainMill` | `MILL` | 1 | 1 | **none** | no | 9000 g | `wooden_case.p3d` |
| Mortar and Pestle | `ChefZ_Mortar` | `MORTAR` | 2 | 1 | 4×3 | no | 1800 g | `CookingPot.p3d` |
| Drying Rack | `ChefZ_DryingRack` | `RACK` | 1 | 4 | 4×3 | no | 4200 g | `rack_dz.p3d` |
| Smoker | `ChefZ_Smoker` | `SMOKER` | 1 | 2 | 4×3 | **yes** | 11000 g | `wooden_case.p3d` |
| Salt Boiling Pan | `ChefZ_SaltPan` | `SALTWORKS` | 2 | 1 | 3×2 | no | 2400 g | `FryingPan.p3d` |
| Butter Churn | `ChefZ_ButterChurn` | `CHURN` | 2 | 1 | 4×4 | no | 4200 g | `wooden_case.p3d` |
| Cheese Press | `ChefZ_CheesePress` | `PRESS` | 1 | 1 | 6×4 | no | 6800 g | `wooden_case.p3d` |
| Meat Grinder | `ChefZ_MeatGrinder` | `GRINDER` | 2 | 1 | **none** | no | 3200 g | `Cauldron.p3d` |

All eight run at `speedMultiplier` 1.0. Every model is a vanilla proxy — no station
has its own geometry yet.

The cargo area **is** the input side: `ChefZ_ProcessingStation_Base` reads its
ingredients with `ChefZ_FactCollector.CollectFromCargo(this, ...)`. That function
calls `inventory.GetCargo()` and returns immediately if the result is null.

## Execution kinds

| Kind | Meaning | Processes |
|---|---|---|
| `STATION_ACTION` | The player stands at the station and performs the action. Seconds. | 6 |
| `STATION_TIMED` | The job ticks on without the player. Minutes. If the environment stops matching, the job **pauses** — it never rolls back and never destroys the input. | 7 |
| `HANDCRAFT` | Not a station at all. Runs through the vanilla crafting menu with an item in hand. | 8 |

## Tool groups

Two tool groups exist. Both are declared exactly once, in
`ChefZ_Processing/config.cpp` under `CfgChefZTools`; every other module only
*uses* them.

| Group | Members | Used by |
|---|---|---|
| `CUTTING_TOOL` | KitchenKnife, SteakKnife, HuntingKnife, CombatKnife, KukriKnife, BoneKnife, StoneKnife, FangeKnife (`allowSubclasses = 1`) | `PROCESS_CUT_MEAT`, `PROCESS_CLEAN_CASING`, `PROCESS_CARVE_PLATE`, `PROCESS_CARVE_BOWL` |
| `ROLLING_PIN` | `ChefZ_RollingPin` (`allowSubclasses = 1`) | `PROCESS_ROLL` |

**Only one station process requires a tool at all** — `PROCESS_CLEAN_CASING` at the
Cutting Board. Milling, grinding, churning, pressing, drying and smoking need
nothing but the station.

---

## Grain Mill

`ChefZ_GrainMill` · category `MILL` · 1 parallel slot · cargo **none**

Turns wheat into flour. One process, one transform — the narrowest station in the mod, and the head of the entire grain chain.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_MILL` | STATION_ACTION | 25 s | no | none |

### Transforms (1)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_WheatToFlour` | 1× Wheat | Flour | × 0.78 of input | 25 s | — |

> **This station has no cargo.** Its `config.cpp` class carries no
> `class Cargo { itemsCargoSize[] = {...}; }` block, and `Inventory_Base`
> supplies none. `ChefZ_FactCollector.CollectFromCargo` returns as soon as
> `inventory.GetCargo()` is null, so nothing can ever be placed in the station and
> no transform above can match. The Butter Churn, Cheese Press, Salt Pan, Smoker
> and the herb-station base all carry such a block; these three do not.
> See [Known-Limitations](Known-Limitations).

---

## Mortar and Pestle

`ChefZ_Mortar` · category `MORTAR` · 1 parallel slot · cargo 4×3

Grinds dried herbs and spices into powders and mixes. Both processes are `STATION_ACTION`: the player stands at the mortar and works. This is the only source of Black Pepper, Paprika Powder, Herb Mix and Hunter Seasoning.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_GRIND_SPICE` | STATION_ACTION | 20 s | no | none |
| `PROCESS_GRIND_HERB` | STATION_ACTION | 15 s | no | none |

### Transforms (4)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_PeppercornsToBlackPepper` | 1+× Dried Peppercorns | Black Pepper | 1:1 from input | 20 s | PREPARED |
| `TR_DriedPaprikaToPowder` | 1+× Dried Paprika | Paprika Powder | 1:1 from input | 20 s | PREPARED |
| `TR_HunterSeasoning` | 1+× Black Pepper + 1+× Paprika Powder + 1+× Dried Thyme + 1+× Dried Wild Garlic + 1+× *SPICE* | Hunter Seasoning | 1× | 35 s | PREPARED |
| `TR_HerbMix` | 1+× Dried Thyme + 1+× Dried Parsley + 1+× Dried Rosemary | Herb Mix | 1× | 25 s | PREPARED |

---

## Drying Rack

`ChefZ_DryingRack` · category `RACK` · 4 parallel slots · cargo 4×3

The busiest station in the mod. Eleven transforms across four different chains — herbs, spices, pasta and preservation. `PROCESS_DRY` needs neither heat nor fuel, only time, so a rack works anywhere. Four parallel slots. Individual transforms override the 10-minute base duration; the spread is 8 minutes for parsley to 90 minutes for dry sausage.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_DRY` | STATION_TIMED | 10 min | no | none |

### Transforms (11)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_RawPastaToDriedPasta` | 1× Fresh Pasta | Dried Pasta | 1:1 from input | 30 min | — |
| `TR_SaltedMeatToDried` | 1+× Salted Meat | Dried Meat | 1:1 from input | 60 min | DRIED |
| `TR_SaltedFishToDried` | 1+× Salted Fish | Dried Fish | 1:1 from input | 45 min | DRIED |
| `TR_RawSausageToDry` | 1+× *SAUSAGE* + state RAW | Dry Sausage | 1:1 from input | 90 min | DRIED |
| `TR_ParsleyToDried` | 1+× Fresh Parsley | Dried Parsley | 1:1 from input | 8 min | DRIED |
| `TR_DillToDried` | 1+× Fresh Dill | Dried Dill | 1:1 from input | 8 min | DRIED |
| `TR_ThymeToDried` | 1+× Fresh Thyme | Dried Thyme | 1:1 from input | 8 min | DRIED |
| `TR_RosemaryToDried` | 1+× Fresh Rosemary | Dried Rosemary | 1:1 from input | 10 min | DRIED |
| `TR_WildGarlicToDried` | 1+× Fresh Wild Garlic | Dried Wild Garlic | 1:1 from input | 8 min | DRIED |
| `TR_PaprikaToDried` | 1+× Paprika | Dried Paprika | 1:1 from input | 15 min | DRIED |
| `TR_PepperBerriesToDried` | 1+× Pepper Berries | Dried Peppercorns | 1:1 from input | 15 min | DRIED |

---

## Smoker

`ChefZ_Smoker` · category `SMOKER` · 2 parallel slots · cargo 4×3

Smokes salted meat, raw fish and raw sausage. Two parallel slots, `needsFuel` set.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_SMOKE` | STATION_TIMED | 30 min | **required** | none |

### Transforms (3)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_SaltedMeatToSmoked` | 1+× Salted Meat | Smoked Meat | 1:1 from input | 30 min | SMOKED |
| `TR_FishToSmoked` | 1+× *FISH* + state RAW | Smoked Fish | 1:1 from input | 25 min | SMOKED |
| `TR_RawSausageToSmoked` | 1+× *SAUSAGE* + state RAW | Smoked Sausage | 1:1 from input | 40 min | SMOKED |

> **The smoker cannot run as shipped.** Two independent reasons:
>
> 1. `PROCESS_SMOKE` sets `requiresHeat = 1`.
>    `ChefZ_ProcessingStation_Base.ChefZ_HasHeat()` returns `false` in the base
>    class, and the smoker is declared as
>    `class ChefZ_Smoker extends ChefZ_ProcessingStation_Base {}` — it never
>    overrides it. `ChefZ_SaltPan` does override it, with a proximity check for a
>    burning `FireplaceBase`; the smoker has no equivalent.
> 2. The station record sets `"needsFuel": true`, and `ChefZ_IsPowered()` returns
>    `!m_ChefZ_NeedsFuel`. `ChefZ_CompiledProcess.MeetsEnvironment` rejects on
>    `stationPowered` before it ever reaches the heat check. The `config.cpp`
>    class has no fuel attachment slot.
>
> Both are fail-safe by design — the job pauses rather than running cold — but the
> effect is that all three smoking transforms are unreachable in V1.
> See [Known-Limitations](Known-Limitations).

---

## Salt Boiling Pan

`ChefZ_SaltPan` · category `SALTWORKS` · 1 parallel slot · cargo 3×2

The entire salt chain. Boil sea water down to raw salt, then dry raw salt into salt. Boiling needs a burning fireplace within range; drying does not. Neither transform names a station, so any station offering the process would do — the Salt Pan is currently the only one.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_BOIL_BRINE` | STATION_TIMED | 15 min | **required** | none |
| `PROCESS_DRY_SALT` | STATION_TIMED | 20 min | no | none |

### Transforms (2)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_SaltwaterToRawSalt` | 1× container with SaltWater (0.6) | Raw Salt | × 0.04 of input | 15 min | — |
| `TR_RawSaltToSalt` | 1× Raw Salt | Salt | × 0.67 of input | 20 min | — |

---

## Butter Churn

`ChefZ_ButterChurn` · category `CHURN` · 1 parallel slot · cargo 4×4

Skims milk into cream and churns cream into butter. Both are `STATION_TIMED`: start the job and walk away. Neither dairy output declares a `quantityMode`, so both produce one item.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_SEPARATE_CREAM` | STATION_TIMED | 2 min | no | none |
| `PROCESS_CHURN_BUTTER` | STATION_TIMED | 3 min | no | none |

### Transforms (2)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_MilkToCream` | 2× Milk | Cream | 1× (no mode given) | 2 min | — |
| `TR_CreamToButter` | 2× Cream | Butter | 1× (no mode given) | 3 min | — |

---

## Cheese Press

`ChefZ_CheesePress` · category `PRESS` · 1 parallel slot · cargo 6×4

Presses milk into cheese. At 5 minutes for 3 milk, the longest single dairy step.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_PRESS_CHEESE` | STATION_TIMED | 5 min | no | none |

### Transforms (1)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_MilkToCheese` | 3× Milk | Cheese | 1× (no mode given) | 5 min | — |

---

## Cutting Board — removed

Removed on 2026-08-29. Cutting is "ingredient + knife" (
`PROCESS_CUT_MEAT`, `PROCESS_CLEAN_CASING` are `HANDCRAFT` with `CUTTING_TOOL`), so
there was nothing left for a station to do. A server that had one placed loses that
object on its next start.

## Meat Grinder

`ChefZ_MeatGrinder` · category `GRINDER` · 1 parallel slot · cargo **none**

Mince raw meat, then stuff the mince into casing. Twelve transforms — six mincing, six stuffing — make this the widest station in the mod. Four of the six mincing transforms drop Animal Fat as a chance byproduct.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_GRIND_MEAT` | STATION_ACTION | 20 s | no | none |
| `PROCESS_STUFF_SAUSAGE` | STATION_ACTION | 15 s | no | none |

### Transforms (12)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_MeatToMinced` | 1+× *MEAT* + stage Raw | Minced Meat<br>byproduct: Animal Fat (35 %) | 1:1 from input | 20 s | PREPARED |
| `TR_PorkToMinced` | 1+× Pig Steak + stage Raw | Minced Pork<br>byproduct: Animal Fat (50 %) | 1:1 from input | 20 s | PREPARED |
| `TR_VenisonToMinced` | 1+× Deer Steak + stage Raw | Minced Venison | 1:1 from input | 20 s | PREPARED |
| `TR_BoarToMinced` | 1+× Boar Steak + stage Raw | Minced Boar<br>byproduct: Animal Fat (35 %) | 1:1 from input | 20 s | PREPARED |
| `TR_ChickenToMinced` | 1+× Chicken Breast + stage Raw | Minced Chicken | 1:1 from input | 20 s | PREPARED |
| `TR_BearToMinced` | 1+× Bear Steak + stage Raw | Minced Bear<br>byproduct: Animal Fat (60 %) | 1:1 from input | 20 s | PREPARED |
| `TR_RawSausage` | 1+× *MINCED_MEAT* + 1+× *SPICE* (1) + 1+× Sausage Casing | Raw Sausage | 1× | 15 s | RAW |
| `TR_RawPorkSausage` | 1+× Minced Pork + 1+× *SPICE* (1) + 1+× *SPICE* (1) + 1+× Sausage Casing | Raw Pork Sausage | 1× | 15 s | RAW |
| `TR_RawVenisonSausage` | 1+× Minced Venison + 1+× *SPICE* (1) + 1+× *HERB* or *DRIED_HERB* (1) + 1+× Sausage Casing | Raw Venison Sausage | 1× | 15 s | RAW |
| `TR_RawBoarSausage` | 1+× Minced Boar + 1+× *SPICE* (1) + 1+× *HERB* or *DRIED_HERB* (1) + 1+× Sausage Casing | Raw Boar Sausage | 1× | 15 s | RAW |
| `TR_RawHunterSausage` | 2× *WILD_MEAT* + stage Raw + 1+× *SPICE* (1) + 1+× *SPICE* (1) + 1+× Sausage Casing | Raw Hunter Sausage | 1× | 15 s | RAW |
| `TR_RawSpicySausage` | 1+× *MINCED_MEAT* + 1+× *SPICE* (1) + 1+× *SPICE* (1) + 1+× *SPICE* (1) + 1+× Sausage Casing | Raw Spicy Sausage | 1× | 15 s | RAW |

> **This station has no cargo.** Its `config.cpp` class carries no
> `class Cargo { itemsCargoSize[] = {...}; }` block, and `Inventory_Base`
> supplies none. `ChefZ_FactCollector.CollectFromCargo` returns as soon as
> `inventory.GetCargo()` is null, so nothing can ever be placed in the station and
> no transform above can match. The Butter Churn, Cheese Press, Salt Pan, Smoker
> and the herb-station base all carry such a block; these three do not.
> See [Known-Limitations](Known-Limitations).

---

## Handcraft transforms — no station needed

21 of the 58 transforms need no station at all. They run
through the vanilla crafting menu; each module reserves places in that list up front
with `handcraftRecipeSlots` in its `CfgChefZ` node.

| Process | Duration | Tool | Tool damage | Transforms |
|---|---|---|---|---|
| `PROCESS_KNEAD` | 8 s | none | 0 | 2 |
| `PROCESS_ROLL` | 10 s | `ROLLING_PIN` | 2 | 2 |
| `PROCESS_CARVE_PLATE` | 20 s | `CUTTING_TOOL` | 0 | 1 |
| `PROCESS_CARVE_BOWL` | 25 s | `CUTTING_TOOL` | 0 | 1 |
| `PROCESS_CUT_MEAT` | 4 s | `CUTTING_TOOL` | 2 | 1 |
| `PROCESS_SALT_CURE` | 6 s | none | 0 | 2 |

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_FlourWaterToDough` | 1× Flour (250) + 1× container with Water (150) | Dough | 1× | 8 s | — |
| `TR_DoughToRawPasta` | 1× Dough | Fresh Pasta | 500× | 10 s | — |
| `TR_CarveWoodenPlate` | 1+× Firewood | Empty Plate | 1× | 20 s | — |
| `TR_CarveWoodenBowl` | 1+× Firewood | Empty Bowl | 1× | 25 s | — |
| `TR_DicedMeat` | 1+× *MEAT* + stage Raw | Diced Meat | 1× | 4 s | PREPARED |
| `TR_SaltMeat` | 1× *MEAT* + state RAW + not *SAUSAGE* + 1× *SALT* (20) | Salted Meat | 1× | 6 s | SALTED |
| `TR_SaltFish` | 1× *FISH* + state RAW + 1× *SALT* (20) | Salted Fish | 1× | 6 s | SALTED |

## Counts

| | |
|---|---|
| Stations | 9 |
| Station categories | 9 |
| Processes | 21 — 8 handcraft, 6 station action, 7 station timed |
| Transforms | 58 — 37 at stations, 21 handcraft |
| Processes with no transform | 0 |
| Transforms naming an undeclared process | 0 |
| Transforms naming an unknown station | 0 |
| Stations without cargo | 3 (Grain Mill, Cutting Board, Meat Grinder) |

## See also

[Production-Chains](Production-Chains) · [Recipe-Reference](Recipe-Reference) ·
[Modules](Modules) · [Architecture](Architecture) · [Adding-Content](Adding-Content) ·
[Food-States](Food-States) · [Known-Limitations](Known-Limitations)
