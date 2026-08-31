# Processing Stations

ChefZ ships **15 processing stations**. They run *transforms*,
not recipes: a transform takes items out of the station's cargo and puts different
items back. Nine station records live in
`Psyerns_ChefZ_Core/Addons/ChefZ_Processing/Config/` with their `CfgVehicles`
classes in that module's `config.cpp`; six more live in `ChefZ_Farming` — the two
beehives (`Config/Processing/Apiary_Stations.json`) and, since 31.08.2026, the four
wild plants (`Config/Processing/WildPlant_Stations.json`). Those six are the
exceptions to "a station runs transforms": neither a hive nor a wild plant runs one
— see [Beehive](#beehive-and-double-beehive).

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

## The eleven stations

| Station | Class | Category | Processes | Parallel slots | Cargo | Needs fuel | Weight | Proxy model |
|---|---|---|---|---|---|---|---|---|
| Grain Mill | `ChefZ_GrainMill` | `MILL` | 1 | 1 | **none** | no | 9000 g | `wooden_case.p3d` |
| Mortar and Pestle | `ChefZ_Mortar` | `MORTAR` | 2 | 1 | 4×3 | no | 1800 g | `CookingPot.p3d` |
| Drying Rack | `ChefZ_DryingRack` | `RACK` | 1 | 4 | 4×3 | no | 4200 g | `rack_dz.p3d` |
| Smoker | `ChefZ_Smoker` | `SMOKER` | 1 | 2 | 4×3 | **yes** | 11000 g | `wooden_case.p3d` |
| Frying Pan | `ChefZ_FryingPan` | `SALTWORKS` | 2 | 1 | 3×2 | no | 2400 g | `FryingPan.p3d` |
| Butter Churn | `ChefZ_ButterChurn` | `CHURN` | 2 | 1 | 4×4 | no | 4200 g | `wooden_case.p3d` |
| Cheese Press | `ChefZ_CheesePress` | `PRESS` | 1 | 1 | 6×4 | no | 6800 g | `wooden_case.p3d` |
| Meat Grinder | `ChefZ_MeatGrinder` | `GRINDER` | 2 | 1 | **none** | no | 3200 g | `Cauldron.p3d` |
| Honey Extractor | `ChefZ_HoneyExtractor` | `EXTRACTOR` | 1 | 1 | 10×10 | no | 9500 g | `Cauldron.p3d` |
| Beehive | `ChefZ_Beehive` | `APIARY` | 1 | 1 | 10×9 | no | 14000 g | own: `ChefZ_Devices/models/beekeeper.p3d` |
| Double Beehive | `ChefZ_BeehiveDouble` | `APIARY` | 1 | 1 | 10×15 | no | 26000 g | own: `ChefZ_Devices/models/beehive.p3d` |
| Wild Corn | `ChefZ_WildCorn` | `WILD_PLANT` | 1 | 1 | **none** | no | 900 g | own: `ChefZ_Plants/models/corn_plant.p3d` |
| Wild Thyme | `ChefZ_WildThyme` | `WILD_PLANT` | 1 | 1 | **none** | no | 400 g | `plant_material.p3d` |
| Wild Rosemary | `ChefZ_WildRosemary` | `WILD_PLANT` | 1 | 1 | **none** | no | 400 g | own: `ChefZ_Plants/models/rosmary.p3d` |
| Wild Parsley | `ChefZ_WildParsley` | `WILD_PLANT` | 1 | 1 | **none** | no | 400 g | own: `ChefZ_Plants/models/parsley.p3d` |

All fifteen run at `speedMultiplier` 1.0. Five of them stand on their own geometry —
the two beehives and three of the four wild plants; the other ten are still vanilla
proxies.

The four wild plants are stations only in the record sense: nothing builds them and
nothing places them. The CE spawns them from the templates in
`ChefZ_Farming/ServerConfig/` and they are harvested where they stand, which is why
they carry no cargo and run no transform.

The cargo area **is** the input side: `ChefZ_ProcessingStation_Base` reads its
ingredients with `ChefZ_FactCollector.CollectFromCargo(this, ...)`. That function
calls `inventory.GetCargo()` and returns immediately if the result is null.

## Execution kinds

| Kind | Meaning | Processes |
|---|---|---|
| `STATION_ACTION` | The player stands at the station and performs the action. Seconds. | 6 |
| `STATION_TIMED` | The job ticks on without the player. Minutes. If the environment stops matching, the job **pauses** — it never rolls back and never destroys the input. | 8 |
| `HANDCRAFT` | Not a station at all. Runs through the vanilla crafting menu with an item in hand. | 17 |

(Counted 2026-08-29 over every `CfgChefZProcesses` class and JSON process
record, after the apiary rework.)

## Tool groups

Eight tool groups, declared across three modules — `ChefZ_Cooking`, `ChefZ_Farming`
and `ChefZ_Processing`, each under its own `CfgChefZTools`. Every group allows
subclasses, so a variant of a listed class counts as a member.

| Group | Members | Declared in | Used by |
|---|---|---|---|
| `CUTTING_TOOL` | KitchenKnife, SteakKnife, HuntingKnife, CombatKnife, KukriKnife, BoneKnife, StoneKnife, FangeKnife | `ChefZ_Processing` | `PROCESS_CUT_MEAT`, `PROCESS_CARVE_PLATE`, `PROCESS_CARVE_BOWL`, `PROCESS_CARVE_BOWL_BARK` |
| `ROLLING_PIN` | `ChefZ_PastaMachine`, MeatTenderizer | `ChefZ_Processing` | `PROCESS_ROLL` |
| `METALWORK_TOOL` | Pliers, Hammer, Wrench, LugWrench, Screwdriver | `ChefZ_Processing` | `PROCESS_ASSEMBLE` |
| `AXE_TOOL` | WoodAxe, Hatchet, FirefighterAxe, Iceaxe | `ChefZ_Cooking` | `PROCESS_CARVE_BOWL_BARK` |
| `SAWING_TOOL` | Hacksaw | `ChefZ_Cooking` | `PROCESS_CUT_CANS` |
| `HAND_TOOL` | Hammer, Hatchet, Pliers, Screwdriver | `ChefZ_Farming` | `PROCESS_BUILD_BEE_SMOKER`, `PROCESS_RAISE_HIVE` |
| `UNCAPPING_TOOL` | `ChefZ_UncappingFork` | `ChefZ_Farming` | `PROCESS_UNCAP_COMB` |
| `BEE_SMOKER` | `ChefZ_BeeSmoker` | `ChefZ_Farming` | — |

**`ROLLING_PIN` contains no rolling pin.** The group keeps the name because
`PROCESS_ROLL` in `ChefZ_Baking/Config/GrainProcesses.json` names it, and a rename
would have to move through both. Its former sole member `ChefZ_RollingPin` had no
source at all — no loot entry, no transform that made one — which left `PROCESS_ROLL`
unreachable and the whole baking chain behind the dough dead, without a single error
message. Two sources replaced it: vanilla `MeatTenderizer`, which spawns in town and
village, and `ChefZ_PastaMachine`, built from a metal plate through
`TR_AssemblePastaMachine`.

**`BEE_SMOKER` is used by no process**, and that is deliberate. The smoker is not a
tool a process demands — it is checked in script, in the hive's sting hook, where
having one in hand calms the bees. The group exists so that check has a declared
membership list rather than a hard-coded class name.

**No station process requires a tool.** Milling, grinding, churning, pressing,
drying, smoking and spinning honey need nothing but the station. Every tool group
above serves a handcraft process.

## Grain Mill

`ChefZ_GrainMill` · category `MILL` · 1 parallel slot · cargo 5×4

Turns wheat or corn into flour. One process, two transforms — the narrowest station in the mod, and the head of the entire grain chain.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_MILL` | STATION_ACTION | 25 s | no | none |

### Transforms (2)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_WheatToFlour` | 1× Wheat | Flour | × 0.78 of input | 25 s | — |
| `TR_CornToFlour` | 1-5× Corn | Flour | 120 g per cob | 25 s | — |

`TR_CornToFlour` carries `priority` 1 while `TR_WheatToFlour` stays at 0. The two
transforms match disjoint inputs, so the order never decides anything; the offset
only keeps `ChefZ_ProcessingManager` from reporting a tie between two transforms
of equal specificity on the same process at build time.

> **Cargo since 2026-08-31.** This station had no `class Cargo` block and could
> not receive input at all. It has one now, so the transforms above are reachable.

## Mortar and Pestle

`ChefZ_Mortar` · category `MORTAR` · 1 parallel slot · cargo 4×3

Grinds dried herbs and spices into powders and mixes. Both processes are `STATION_ACTION`: the player stands at the mortar and works. This is the only source of Black Pepper, Paprika Powder, Herb Mix, Hunter Seasoning and Mushroom Culture — the last of which starts the cheese chain and is the one thing here that is not a seasoning.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_GRIND_SPICE` | STATION_ACTION | 20 s | no | none |
| `PROCESS_GRIND_HERB` | STATION_ACTION | 15 s | no | none |

### Transforms (5)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_RottenMushroomToCulture` | 1× *MUSHROOM* + stage Rotten | Mushroom Culture | 1× | 20 s | PREPARED |
| `TR_PeppercornsToBlackPepper` | 1× Dried Peppercorns | Black Pepper | 1:1 from input | 20 s | PREPARED |
| `TR_DriedPaprikaToPowder` | 1× Dried Paprika | Paprika Powder | 1:1 from input | 20 s | PREPARED |
| `TR_HunterSeasoning` | 1× Black Pepper + 1× Paprika Powder + 1× Dried Thyme + 1× Dried Wild Garlic + 1× *SPICE* | Hunter Seasoning | 1× | 35 s | PREPARED |
| `TR_HerbMix` | 1× Dried Thyme + 1× Dried Parsley + 1× Dried Rosemary | Herb Mix | 1× | 25 s | PREPARED |

## Drying Rack

`ChefZ_DryingRack` · category `RACK` · 4 parallel slots · cargo 4×3

The busiest station in the mod. Twelve transforms across four different chains — herbs, spices, pasta and preservation. `PROCESS_DRY` needs neither heat nor fuel, only time, so a rack works anywhere. Four parallel slots. Individual transforms override the 10-minute base duration; the spread is 8 minutes for parsley to 90 minutes for dry sausage.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_DRY` | STATION_TIMED | 10 min | no | none |

### Transforms (12)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_RawPastaToDriedPasta` | 1× Fresh Pasta | Dried Pasta | 1:1 from input | 30 min | — |
| `TR_SaltedMeatToDried` | 1× Salted Meat | Dried Meat | 1:1 from input | 60 min | DRIED |
| `TR_SaltedFishToDried` | 1× Salted Fish | Dried Fish | 1:1 from input | 45 min | DRIED |
| `TR_RawSausageToDry` | 1× *SAUSAGE* + state RAW | Dry Sausage | 1:1 from input | 90 min | DRIED |
| `TR_ParsleyToDried` | 1× Fresh Parsley | Dried Parsley | 1:1 from input | 8 min | DRIED |
| `TR_ThymeToDried` | 1× Fresh Thyme | Dried Thyme | 1:1 from input | 8 min | DRIED |
| `TR_RosemaryToDried` | 1× Fresh Rosemary | Dried Rosemary | 1:1 from input | 10 min | DRIED |
| `TR_WildGarlicToDried` | 1× Fresh Wild Garlic | Dried Wild Garlic | 1:1 from input | 8 min | DRIED |
| `TR_PaprikaToDried` | 1× Paprika | Dried Paprika | 1:1 from input | 15 min | DRIED |
| `TR_PepperBerriesToDried` | 1× Pepper Berries | Dried Peppercorns | 1:1 from input | 15 min | DRIED |
| `TR_CaninaBerriesToDried` | 2× `CaninaBerry` | Dried Berries | 2 → 1, fixed | 7 min | DRIED |
| `TR_SambucusBerriesToDried` | 2× `SambucusBerry` | Dried Berries | 2 → 1, fixed | 7 min | DRIED |

The two berry transforms are the only ones at this rack that do not keep a 1:1
ratio: two vanilla berries become one `ChefZ_DriedBerries`, and both kinds converge
on the same output class.

## Smoker

`ChefZ_Smoker` · category `SMOKER` · 2 parallel slots · cargo 5×5

Smokes salted meat, raw fish and raw sausage. Two parallel slots, `needsFuel` set.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_SMOKE` | STATION_TIMED | 5 min | **required** | none |

### Transforms (3)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_SaltedMeatToSmoked` | 1× Salted Meat | Smoked Meat | 1:1 from input | **never runs** | SMOKED |
| `TR_FishToSmoked` | 1× *FISH* + state RAW | Smoked Fish | 1:1 from input | 5 min | SMOKED |
| `TR_RawSausageToSmoked` | 1× *SAUSAGE* + state RAW | Smoked Sausage | 1:1 from input | 5 min | SMOKED |

**The smoker burns its own fuel since 2026-08-31.** It had neither heat nor power:
`PROCESS_SMOKE` sets `requiresHeat = 1` while the base class answered `false`, and
the station record set `"needsFuel": true` against a class with no fuel slot.
`ChefZ_Smoker` now overrides both `ChefZ_HasHeat()` and `ChefZ_IsPowered()`
(`ChefZ_Processing/Scripts/4_World/ChefZ/Preservation/ChefZ_Smoker.c:202,216`) and
carries its own burn state, fed with bark from its own cargo. Five minutes of full
burn costs two pieces. The price of smoking is fuel now, not waiting.

> **Smoked Meat still cannot be made, for an unrelated reason.**
> `TR_SaltedMeatToSmoked` declares no `process` field — unlike its two neighbours
> in the same file, which both name `PROCESS_SMOKE`. `ChefZ_ProcessCompiler.c:355`
> rejects any transform whose process cannot be resolved, so this one is dropped at
> boot with an error in the RPT and no station ever offers it. Smoked Fish and
> Smoked Sausage are unaffected. See [Known-Limitations](Known-Limitations).

## Frying Pan

`ChefZ_FryingPan` · category `SALTWORKS` · 1 parallel slot · cargo 3×2

The entire salt chain. Boil sea water down to raw salt, then dry raw salt into salt. Boiling needs a burning fireplace within range; drying does not. Neither transform names a station, so any station offering the process would do — the Frying Pan is currently the only one.

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

## Butter Churn

`ChefZ_ButterChurn` · category `CHURN` · 1 parallel slot · cargo 10×14

Skims milk into cream and churns cream into butter. Both are `STATION_TIMED`: start the job and walk away. Neither dairy output declares a `quantityMode`, so both produce one item.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_SEPARATE_CREAM` | STATION_TIMED | 2 min (overridden to 60 s) | no | none |
| `PROCESS_CHURN_BUTTER` | STATION_TIMED | 60 s | no | none |

### Transforms (2)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_MilkToCream` | 2× Milk | Cream | 1× (no mode given) | 60 s | — |
| `TR_CreamToButter` | 2× Cream | Butter | 1× (no mode given) | 60 s | — |

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
| `TR_CurdToCheese` | 1× Cheese Curd | Cheese | 1× (class default) | 5 min | — |

The curd is cooked in a pot or cauldron (`RCP_ChefZ_CheeseCurd`: 3× milk +
1× mushroom culture, boiling, 5 min); the old direct route `TR_MilkToCheese`
was replaced by this two-stage chain on 2026-08-31.

## Cutting Board — removed

Removed on 2026-08-29. Cutting is "ingredient + knife" (
`PROCESS_CUT_MEAT` is `HANDCRAFT` with `CUTTING_TOOL`), so
there was nothing left for a station to do. A server that had one placed loses that
object on its next start.

## Meat Grinder

`ChefZ_MeatGrinder` · category `GRINDER` · 1 parallel slot · cargo 5×3

Mince raw meat, then stuff the mince into casing. Twelve transforms — six mincing, six stuffing — make this the widest station in the mod. Four of the six mincing transforms drop Animal Fat as a chance byproduct.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_GRIND_MEAT` | STATION_TIMED | 30 s | no | none |
| `PROCESS_STUFF_SAUSAGE` | STATION_ACTION | 15 s | no | none |

### Transforms (12)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_MeatToMinced` | 1× *MEAT* + stage Raw | Minced Meat<br>byproduct: Animal Fat (35 %) | 1:1 from input | 30 s | PREPARED |
| `TR_PorkToMinced` | 1× Pig Steak + stage Raw | Minced Pork<br>byproduct: Animal Fat (50 %) | 1:1 from input | 30 s | PREPARED |
| `TR_VenisonToMinced` | 1× Deer Steak + stage Raw | Minced Venison | 1:1 from input | 30 s | PREPARED |
| `TR_BoarToMinced` | 1× Boar Steak + stage Raw | Minced Boar<br>byproduct: Animal Fat (35 %) | 1:1 from input | 30 s | PREPARED |
| `TR_ChickenToMinced` | 1× Chicken Breast + stage Raw | Minced Chicken | 1:1 from input | 30 s | PREPARED |
| `TR_BearToMinced` | 1× Bear Steak + stage Raw | Minced Bear<br>byproduct: Animal Fat (60 %) | 1:1 from input | 30 s | PREPARED |
| `TR_RawSausage` | 1× *MINCED_MEAT* + 1× *SPICE* (1) + 1× *CASING* (Guts or Small Guts) | Raw Sausage | 1× | 15 s | RAW |
| `TR_RawPorkSausage` | 1× Minced Pork + 1× *SPICE* (1) + 1× *SPICE* (1) + 1× *CASING* (Guts or Small Guts) | Raw Pork Sausage | 1× | 15 s | RAW |
| `TR_RawVenisonSausage` | 1× Minced Venison + 1× *SPICE* (1) + 1× *HERB* or *DRIED_HERB* (1) + 1× *CASING* (Guts or Small Guts) | Raw Venison Sausage | 1× | 15 s | RAW |
| `TR_RawBoarSausage` | 1× Minced Boar + 1× *SPICE* (1) + 1× *HERB* or *DRIED_HERB* (1) + 1× *CASING* (Guts or Small Guts) | Raw Boar Sausage | 1× | 15 s | RAW |
| `TR_RawHunterSausage` | 2× *WILD_MEAT* + stage Raw + 1× *SPICE* (1) + 1× *SPICE* (1) + 1× *CASING* (Guts or Small Guts) | Raw Hunter Sausage | 1× | 15 s | RAW |
| `TR_RawSpicySausage` | 1× *MINCED_MEAT* + 1× *SPICE* (1) + 1× *SPICE* (1) + 1× *SPICE* (1) + 1× *CASING* (Guts or Small Guts) | Raw Spicy Sausage | 1× | 15 s | RAW |

> **Cargo since 2026-08-31.** This station had no `class Cargo` block and could
> not receive input at all. It has one now, so the transforms above are reachable.

## Honey Extractor

`ChefZ_HoneyExtractor` · category `EXTRACTOR` · 1 parallel slot · cargo 10×10

Spins honey out of uncapped comb frames into empty jars. The cargo holds up to
**5 uncapped frames** and **15 empty jars** (`ChefZ_EmptyJar`, ~500 ml each);
the station script counts them in `CanReceiveItemIntoCargo`, the rest of the
grid is left for the honey that comes out. One frame yields **three jars**: the
frame carries its remaining jars as `varQuantity` (4 → 1, shown as a quantity bar),
and every run takes one unit off the frame and consumes one jar whole. The
fourth unit is a reserve that is never drawn: the core deletes an item whose
last unit would be taken (`ChefZ_SlotEvaluator.PlanAmountDraw` sets
`destroyWhole`), so `TR_SpinHoney` requires at least 2 units and the frame
survives its third jar with one unit left.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_SPIN_HONEY` | STATION_TIMED | 90 s | no | none |

### Transforms (1)

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_SpinHoney` | 1× Uncapped Comb Frame (1 unit, at least 2 present) + 1× Empty Jar | vanilla `Honey` | 1× | 90 s | — |

**The player only starts it.** `PROCESS_SPIN_HONEY` is `STATION_TIMED`: the
crank action begins the first 90-second job, and `ChefZ_HoneyExtractor` overrides
`ChefZ_CompleteJob` to start the next one itself (via `CallLater`, one frame
later, because the base class stops its job timer on completion) as long as a
frame with at least 2 units and an empty jar are in the cargo. A frame below 2
units is replaced in place by an Empty Comb Frame. The running job is saved by
the base class, so a restart resumes the chain. (Assumption A3 — the user was not reachable to confirm
that the drum should keep going on its own.)

The output is vanilla `Honey`, created in the cargo — not necessarily in the jar's
former cell, because vanilla's item config is not part of the repository and ChefZ
never overrides it. The empty jar itself is unchanged and belongs to
`ChefZ_Cooking`. If the cargo is full the job ends `RUN_FAILED` without consuming
anything and the chain **stops**: taking honey out or adding jars does not restart
it — the player has to crank again. The same applies whenever a follow-up job
cannot start (no jar, no frame with 2 units left).

## Beehive and Double Beehive

`ChefZ_Beehive` · category `APIARY` · 1 parallel slot · cargo 10×9 · **10 frames** · lifetime 604800 s
`ChefZ_BeehiveDouble` · category `APIARY` · 1 parallel slot · cargo 10×15 · **20 frames** · lifetime 1209600 s

The cargo grid has spare cells on purpose: the frame limit (10 / 20) is counted by
script, and a rotated or shifted frame must not lock the last one out. The config
`lifetime` sits well above the fill time (40 h / 80 h) because the CE clock runs in
the same server time as the bees; a `types.xml` entry overrides it, so keep it at
least that high there.

The one station that runs **no transform**. Its two processes exist to give the
player an action; the actual work — bees filling frames — is script in
`ChefZ_Apiary.c`, not a station job.

### Processes

| Process | Kind | Base duration | Heat | Tool |
|---|---|---|---|---|
| `PROCESS_HARVEST_HIVE` | STATION_ACTION | 8 s | no | none (a lit Bee Smoker in hand prevents the sting) |
| `PROCESS_PACK_HIVE` | STATION_ACTION | — | no | none |

### Transforms (0)

None, on purpose. `ChefZ_ActionProcessAtStation.IsProcessUsable` skips the
transform check when a process has none, `RunImmediate` reports `NO_MATCH`, and
`NotifyStation` still fires — that is the hook the hive uses.

**How the frames fill.** Only Empty and Full Comb Frames go into the cargo, at most
10 (20 in the double hive). A server-side timer on the hive ticks every 10 s and
raises `varQuantity` on the **first** not-yet-full Empty Comb Frame in cargo order —
one frame after another, **4 h per frame**, so ten frames take 40 h and twenty take
80 h. The frame's bar is vanilla's `quantityBar`, the same one an apple shows, only
rising. At 100 the frame is replaced in its cell by a Full Comb Frame
(`TurnItemIntoItemLambda`, no variable transfer). The fill level is saved with the
frame by the engine's variable block; while the server is down no time passes
(assumption A1, like vanilla plants).

**How the frames come out.** `CanReleaseCargo` allows only a **Full** Comb Frame,
and only while the lid is open. "Open Hive" (`PROCESS_HARVEST_HIVE`) opens the lid
for 120 s (server-side, not persisted). Every completed action at the hive (not
`RUN_FAILED`) wakes the bees. A **Bee Smoker in hand** calms them completely.
Without it there is always a base cost — 20 shock plus one bleeding forearm —
and on top of that bare hands bleed a second arm and a bare head adds 15 shock;
gloves and a covered head absorb only those extras. Since 2026-08-29 an **NBC suit** (jacket and
trousers, neither ruined) seals hands and body without taking wear, and a **gas
mask** (`IsGasMask()`, so GasMask, GP5GasMask, AirborneMask) seals the face; suit
plus gas mask is full protection — nothing is damaged, nobody bleeds. Taking the
frame is an inventory drag.

**The swarm is audible.** The sting hook calls `StartItemSoundServer` right after the
base damage, playing `ChefZ_Bees_Attack_SoundSet` from the hive itself. Because it is
started server-side through vanilla's `ItemSoundHandler`, everyone in earshot hears
it, not only the keeper who was stung — a hive being robbed is a thing other players
can notice. The sound ID is bound once in `InitItemSounds`; the second shipped file,
`Beehive_Ambient.ogg`, is packed but not yet bound to anything.

**Packing it up again.** `PROCESS_PACK_HIVE` is the second station action, added
29.08.2026 from Lykos' delivery, where it had been a vanilla crafting recipe (hive +
screwdriver → kit). A handcraft step would need the 14 kg hive *in hand*; a placed
hive offers only one shape of action, `ChefZ_ActionProcessAtStation`, so the teardown
became a station action with tool group `HAND_TOOL`.

It has **no transform** — it does not change the cargo, it changes the station. One
frame after `ChefZ_OnStationActionFinished`, `ChefZ_Beehive.ChefZ_PackUp()` drops a
kit where the hive stood (two for the double), carries the health across
proportionally, and deletes the hive.

The action appears **only on an empty, closed hive**: `ChefZ_GetProcessAt` and
`ChefZ_SupportsProcess` return `INVALID` otherwise, on the client
(`RefreshProcesses`) as on the server (`ResolveProcessFor`). A hive with frames in it
cannot be packed away with the colony inside — take the frames out first. And the
bees sting during teardown like at any other action, unless a **lit** smoker is in
hand.

**Double Beehive.** `ChefZ_BeehiveDouble` inherits config and script from the
hive and only raises the capacity to 20. It is built from **two** Beehive Kits
(`TR_ExtendBeehive`, `PROCESS_EXTEND_HIVE`, handcraft), not from a placed hive —
a handcraft step consumes its ingredient with everything in its cargo, and a
stocked hive would lose its frames. (Assumption A2; the name `ChefZ_Beehive_Double`
was not possible because the naming checker rejects the second underscore.)

**Removed 2026-08-29:** the Sealed Comb Frame, `PROCESS_TEND_HIVE`,
`Apiary_Hive.json` with `TR_BeesFillFrame` and `TR_HarvestSealedFrame`, and the
glass bottle as extractor input (it is the empty jar now). The kits' nail
ingredient is vanilla `Nail` — `Nails` is a script-only class without a config
entry (assumption A5).

## Handcraft transforms — no station needed

22 of the 61 transforms need no station at all. They run
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
| `PROCESS_BUILD_HIVE_KIT` | 25 s | none | 0 | 1 |
| `PROCESS_RAISE_HIVE` | 30 s | none | 0 | 1 |
| `PROCESS_EXTEND_HIVE` | 30 s | none | 0 | 1 |
| `PROCESS_BUILD_FRAME` | 12 s | none | 0 | 1 |
| `PROCESS_BUILD_UNCAPPING_FORK` | 15 s | none | 0 | 1 |
| `PROCESS_BUILD_BEE_SMOKER` | 20 s | none | 0 | 1 |
| `PROCESS_UNCAP_COMB` | 18 s | none | 0 | 1 |

| Transform | Input | Output | Ratio | Duration | Sets state |
|---|---|---|---|---|---|
| `TR_FlourWaterToDough` | 1× Flour (250) + 1× container with Water (150) | Dough | 1× | 8 s | — |
| `TR_DoughToRawPasta` | 1× Dough | Fresh Pasta | 500× | 10 s | — |
| `TR_CarveWoodenPlate` | 1× Firewood | Empty Plate | 1× | 20 s | — |
| `TR_CarveWoodenBowl` | 1× Firewood | Empty Bowl | 1× | 25 s | — |
| `TR_SaltMeat` | 1× *MEAT* + state RAW + not *SAUSAGE* + 1× *SALT* (20) | Salted Meat | 1× | 6 s | SALTED |
| `TR_SaltFish` | 1× *FISH* + state RAW + 1× *SALT* (20) | Salted Fish | 1× | 6 s | SALTED |
| `TR_BuildBeehiveKit` | 4× Wooden Plank + 10× Nail | Beehive Kit | 1× | 25 s | — |
| `TR_RaiseBeehive` | 1× Beehive Kit | Beehive | 1× | 30 s | — |
| `TR_ExtendBeehive` | 2× Beehive Kit | Double Beehive | 1× | 30 s | — |
| `TR_BuildHoneycombFrame` | 1× Wooden Plank + 2× Nail | Empty Comb Frame (bar at 0 %) | 1× | 12 s | — |
| `TR_BuildUncappingFork` | 1× Wooden Plank + 4× Nail | Uncapping Fork | 1× | 15 s | — |
| `TR_BuildBeeSmoker` | 1× opened Tuna Can | Bee Smoker | 1× | 20 s | — |
| `TR_UncapHoneycombFrame` | 1× Full Comb Frame + Uncapping Fork | Uncapped Comb Frame (3 jars) | 1× | 18 s | — |

The apiary rows come from `ChefZ_Farming/Config/Processing/Apiary_Crafts.json`
(details in its `README_Apiary.md`). `ChefZ_Apiary` reserves **7** handcraft
slots for them.

## Counts

| | |
|---|---|
| Station records | 15 — 9 workbenches in `ChefZ_Processing`, 2 beehives and 4 wild-plant harvest points in `ChefZ_Farming` (added 2026-08-31). The eleven described above are the workbenches and the hives; a wild plant is a harvest point, not a workbench. |
| Station categories | 11 |
| Processes | 34 — 18 handcraft, 7 station action, 9 station timed (recounted 2026-08-31) |
| Handcraft slots reserved | 22, matching the 22 handcraft transforms exactly |
| Transforms | 62 — 39 at a station, 22 handcraft, 1 rejected at boot (recounted 2026-08-31) |
| Processes with no transform | 3 — `PROCESS_HARVEST_HIVE`, `PROCESS_HARVEST_WILD` and `PROCESS_PACK_HIVE`, all by design |
| Transforms naming an undeclared process | 1 — `TR_SaltedMeatToSmoked` names none at all, see the Smoker |
| Transforms naming an unknown station | 0 |
| Stations without cargo | 0 since 2026-08-31 — Mortar and Drying Rack inherit theirs from `ChefZ_HerbStationBase` |

## See also

[Production-Chains](Production-Chains) · [Recipe-Reference](Recipe-Reference) ·
[Modules](Modules) · [Architecture](Architecture) · [Adding-Content](Adding-Content) ·
[Food-States](Food-States) · [Known-Limitations](Known-Limitations)
