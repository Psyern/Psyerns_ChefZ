# Modules

ChefZ is one mod folder containing fourteen addons, plus three separate
compatibility mods that ship as their own PBOs. This page lists what each one
contains, what it depends on and how many classes and records it contributes.

All counts on this page were taken from the files, not estimated. They are a
snapshot; re-count before quoting them in a release note.

For the reasoning behind the split, see [Architecture](Architecture).

## Overview

| Addon | Item classes | Script files | Rank 1 records | Rank 2 records | Stringtable keys |
|---|---:|---:|---:|---:|---:|
| `ChefZ_Core` | 0 | 137 | — | 2 | 4 |
| `ChefZ_Registry` | 0 | 0 | — | 137 | — |
| `ChefZ_Farming` | 29 | 5 | 19 | 21 | 62 |
| `ChefZ_Processing` | 14 | 8 | 18 | 30 | 41 |
| `ChefZ_Ingredients` | 26 | 3 | 1 | 49 | 58 |
| `ChefZ_Meat` | 32 | 1 | — | 58 | 55 |
| `ChefZ_Baking` | 6 | 1 | — | 12 | 16 |
| `ChefZ_Preservation` | 10 | 1 | 10 | 20 | 28 |
| `ChefZ_Cooking` | 44 | 5 | 58 | 45 | 96 |
| `ChefZ_Cookbook` | 2 | 12 | — | — | 5 |
| `ChefZ_Devices` | 0 | 0 | — | — | — |
| `ChefZ_Food` | 0 | 0 | — | — | — |
| `ChefZ_Items` | 0 | 0 | — | — | — |
| `ChefZ_Plants` | 0 | 0 | — | — | — |
| **total** | **163** | **170** | **106** | **374** | **349** |

| Comp mod | Item classes | Script files | Stringtable keys |
|---|---:|---:|---:|
| `Psyerns_ChefZ_COT_Comp` | 0 | 5 | 10 |
| `Psyerns_ChefZ_Terje_Skills_Comp` | 0 | 11 | 2 |
| `Psyerns_ChefZ_Terje_Medicine_Comp` | 0 | 6 | 1 |

"Item classes" counts top-level `class` definitions inside `CfgVehicles`,
including the `scope = 0` base classes. "Rank 1" counts records declared in
`CfgChefZ*` config trees; "rank 2" counts records inside JSON documents. The
two ranks are explained on [Architecture](Architecture#3-where-configuration-comes-from-three-ranks).

The script-file total counts what ships. `ChefZ_Core` carries one more —
`Tests/V_A_PboJsonSmoke/.../ChefZ_PboProbe.c`, the PBO-JSON smoke probe — which
brings the repository to 171 `.c` files.

Across all modules that adds up to **47 recipes**, **61 transforms**,
**33 processes**, **15 stations**, **185 ingredient bindings**, **5 containers**,
**3 cooking devices**, **10 food states**, **5 quality tiers** and
**8 tool groups**.

## `ChefZ_Core`

The rule engine. It contains no item, no ingredient, no dish and no station —
`units[]` and `weapons[]` are empty and stay that way.

- **137 script files** across four layers: 60 in `1_Core`, 40 in `3_Game`, 31 in
  `4_World`, 6 in `5_Mission`, plus one PBO-JSON smoke probe under `Tests/`.
  19 of them are self-tests.
- **Data**: `Config/Core.json` (one `coreSettings` record — the default
  settings, described on [Configuration](Configuration)) and
  `Config/Templates/Core.overlay.json`, the template copied to
  `$profile:ChefZ\Core.json` on first server start.
- **No `CfgChefZ` node.** The core does not register itself with itself, and it
  reserves zero handcraft recipe slots.
- **Depends on**: `DZ_Data` only.

Everything else is documented on [Architecture](Architecture).

## `ChefZ_Registry`

The merged shared vocabulary, and nothing else. No script, no model, no
`CfgVehicles` entry — `units[]` is empty and that is not a forgotten line.

- **142 records** in four documents:
  - `Config/Categories.json` — 41 categories (23 roots, 18 children)
  - `Config/Tags.json` — 19 tags
  - `Config/Nutrition.json` — 76 nutrition records
  - `Config/Preservation.json` — 6 preservation rules (4 by state, 2 by category)
- **`CfgChefZ` node**: `ChefZ_MergedRegistry`, `loadOrder = 150`,
  `handcraftRecipeSlots = 0`. The node is deliberately *not* named like the
  addon: a node with the same name next to the `CfgPatches` entry counts as a
  duplicate class definition for `configcpp.mjs`.
- **Depends on**: `DZ_Data`, `ChefZ_Core`, and the seven content addons whose
  classes its nutrition records name. Those are real data dependencies — a
  nutrition record without its class is a nutrition value for nothing. None of
  those seven depends on `ChefZ_Registry` in return, so there is no cycle.

`loadOrder = 150` puts it ahead of every content slice (the earliest is
`ChefZ_SaltChain` at 155). Reading the shared vocabulary first means every slice
afterwards is checked against a complete category and tag set, and a later,
*differing* second definition of the same id fails loudly as a same-rank
duplicate instead of quietly displacing the merged version.

### Why this addon exists, and why it is not part of the core

Both reasons are written into `ChefZ_Registry/config.cpp`.

In milestone 2 the merged registries lived in `ChefZ_Core/Config/`. That was
wrong twice over:

1. **The core is a rule engine without vocabulary.** Categories, tags and
   nutrition values *are* vocabulary. Putting them in the core contradicts the
   architecture chosen in milestone 1.
2. **Nobody was allowed to declare them in a `dataFiles[]`.** The core has no
   `CfgChefZ` node, so nothing pointed at those files, and the core therefore
   never read them. They were dead files — present, valid, and completely
   without effect.

An addon of its own fixes both: its own `CfgChefZ` registration, one
`dataFiles[]` entry per registry, and the core stays free of content.

The merge itself is described on [Delta Protocol](Delta-Protocol).

## `ChefZ_Farming`

Grain, vegetables and herbs — the start of most production chains. Since
2026-08-29 the ingredients are **found**, like vanilla mushrooms: no seeds, no
growth stages. Where they spawn is the server's `types.xml`. Two exceptions have
grown since: corn also grows in a garden plot (`ChefZ_CornPlant : PlantBase`,
`CropsCount 4` since 31.08.2026), and four **wild plants** stand in the world as
harvest points.

- **item classes**: wheat, the five ChefZ vegetables (`ChefZ_Onion`,
  `ChefZ_Garlic`, `ChefZ_Carrot`, `ChefZ_Cabbage`, `ChefZ_Corn`), five fresh herbs and spices,
  the apiary: `ChefZ_Beehive`, `ChefZ_BeehiveDouble`, `ChefZ_BeehiveKit`,
  three comb frames (empty, full, uncapped), `ChefZ_UncappingFork`,
  `ChefZ_BeeSmoker` — and since 31.08.2026 the wild plants
  `ChefZ_WildPlant_Base` (scope 0), `ChefZ_WildCorn`, `ChefZ_WildThyme`,
  `ChefZ_WildRosemary`, `ChefZ_WildParsley`. The two hives and the four wild
  plants are the only stations outside `ChefZ_Processing`; see
  [Processing Stations](Processing-Stations#beehive-and-double-beehive).
- **Script files**: `ChefZ_FarmingItems.c`, `ChefZ_HerbItems.c`,
  `ChefZ_ProduceFarming.c`, `ChefZ_Apiary.c`, `ChefZ_WildPlants.c`.
- **Rank 1**: **20 records** — 6 ingredient bindings (`ChefZ_ProduceIngredient`
  and the five vegetables), 11 processes and 3 tool groups (`HAND_TOOL`,
  `UNCAPPING_TOOL`, `BEE_SMOKER`).
- **Rank 2**: **25 records** across 6 documents — 11 ingredients, 8 transforms
  and 6 stations (2 hives, 4 wild plants).
- **The only asset folder in the mod**: `Sounds/` with `Bees_Attack.ogg` and
  `Beehive_Ambient.ogg`, bound through `CfgSoundShaders` and `CfgSoundSets`. The
  sting hook plays `ChefZ_Bees_Attack_SoundSet` **server-side** through vanilla's
  `ItemSoundHandler`, so the swarm is heard by everyone in earshot rather than only
  by the keeper who was stung. `Beehive_Ambient.ogg` ships but is not bound to
  anything yet. This addon is also the only one whose `include.txt` lists `*.ogg` —
  without that line AddonBuilder drops the files from the PBO and says nothing.
- **`CfgChefZ` slices**: `ChefZ_GrainFarming` (210, 0 slots),
  `ChefZ_HerbFarming` (215, 0 slots), `ChefZ_WildPlants` (219, 0 slots),
  `ChefZ_Apiary` (8 handcraft slots — one per `HANDCRAFT` transform: six build
  steps, uncapping, and filling the smoker). The wild plants reserve **no** slot:
  a station process never reaches vanilla's recipe list.
- **Depends on**: `DZ_Data`, `DZ_Gear_Cultivation`, `DZ_Gear_Food`, `ChefZ_Core`.

### Wild growth, added 31.08.2026

Four standing plants that the central economy scatters around players, harvested
by hand. They are **mini-stations**, not `PlantBase`: `ChefZ_WildPlant_Base`
extends `ChefZ_ProcessingStation_Base`, the same shape the beehive uses — which is
why the whole slice needed **no core change**.

| | |
|---|---|
| Process | `PROCESS_HARVEST_WILD` — `STATION_ACTION`, 5 s, no tool group, `toolDamage = 0` |
| Transform | **none.** A transform without `inputs` is rejected, the fact collector returns empty for a container without cargo, and the applicator creates only *into* cargo. A plant has no cargo. The outcome is therefore always `NO_MATCH`; only `RUN_FAILED` counts as failure |
| Yield | corn 1 cob, +1 at 25 %, +2 at 5 % · each herb 1 bunch, +1 at 25 %. One roll, two bands. More yield means **more items**, never more quantity |
| After the roll | the items drop beside the plant and the plant deletes itself — but only if at least one item was created |
| Not an item | `IsTakeable`, `CanPutInCargo`, `CanRemoveFromCargo`, `CanPutIntoHands` and `IsDeployable` all `false`; four actions removed again after `super.SetActions()` |
| Groups | `ChefZ_WildCorn.EEOnCECreate()` queues `CallLater(…, 0)` and places 0–2 companions of the same class 1–2 m away. A `position=player` event cannot place a group itself — checked against all 75 events of the test mission |
| Models | corn on the mod's own `corn_plant.p3d`, with an `AnimationSources` block that leaves only `PlantStage_06` and its cobs visible; the three herbs on **proxy** meshes until three standing bushels are modelled |

Nothing of this appears in the world until a human installs the CE template into
the mission. `ChefZ_Farming/ServerConfig/` holds `ChefZ_events.xml` (three events —
corn 60, herbs 140, wheat 40), `ChefZ_types.xml` (five limit containers,
`nominal 0`) and the install guide. **Those files are not packed:** `include.txt`
dropped its `*.xml` pattern the same day, precisely so they cannot end up in the
PBO. The numbers are on [Production Chains](Production-Chains#grain), what is
untested on
[Known-Limitations](Known-Limitations#wild-plants-wildwuchs--31082026).

`ChefZ_FreshHerbBase` and, since 31.08.2026, the three wild herbs
`ChefZ_WildThyme`, `ChefZ_WildRosemary` and `ChefZ_WildParsley` are the classes a
comp mod extends by `modded class` — all four only for the herbalist's highlight.
`ChefZ_WildCorn` is deliberately left out: corn is not a herb
(see [Terje Compatibility](Terje-Compatibility)).

## `ChefZ_Processing`

Stations and tools. Nine of the mod's fifteen stations live here, regardless of
which chain they belong to; the two hives and the four wild plants sit in
`ChefZ_Farming` because they are the thing that is kept or found, not a station
somebody builds a workflow around.

- **item classes** including 9 stations: `ChefZ_GrainMill`,
  `ChefZ_Mortar`, `ChefZ_DryingRack`, `ChefZ_ButterChurn`, `ChefZ_CheesePress`,
  `ChefZ_Smoker`, `ChefZ_FryingPan`, `ChefZ_MeatGrinder`, `ChefZ_HoneyExtractor`.
  The cutting board is gone — cutting is "ingredient + knife".
- **8 script files**, mostly empty derivations from
  `ChefZ_ProcessingStation_Base` — the station behaviour is in the core and in
  data. Two exceptions: `ChefZ_HoneyExtractor.c`, which restarts its own job
  after every jar and limits the cargo to 5 frames and 15 jars, and
  `ChefZ_StationGate.c`, which holds the one question every gatekeeper in this
  module asks — does this item belong in THIS station. It answers over
  `CanReceiveItemIntoCargo` and by category, not by class name, so a new spice
  needs no script change.
- **Rank 1**: **18 records** — 15 processes and 3 tool groups (`CUTTING_TOOL`
  with eight vanilla knives, `ROLLING_PIN`, `METALWORK_TOOL`). Tool group classes are deliberately *not* checked
  against `CfgVehicles`: a knife from an optional module may be named without
  being loaded.
- **Rank 2**: **30 records** — 9 stations, 19 transforms, 1 process, 1 ingredient
  across 15 JSON documents.
- **`CfgChefZ` slices**: six, all with `handcraftRecipeSlots = 0` —
  `ChefZ_SaltChain` (155), `ChefZ_MeatProcessing` (190), `ChefZ_GrainProcessing`
  (220), `ChefZ_HerbProcessing` (230), `ChefZ_DairyProcessing` (260),
  `ChefZ_PreservationStations` (270).
- **Depends on**: `DZ_Data`, `DZ_Gear_Camping`, `DZ_Gear_Tools`, `DZ_Gear_Food`,
  `DZ_Gear_Cooking`, `ChefZ_Core`, `ChefZ_Farming`.

See [Processing Stations](Processing-Stations).

## `ChefZ_Ingredients`

Dairy, salt, spices, mushrooms and the vanilla foodstuffs — the intermediate
goods that sit between a raw ingredient and a dish, plus the vanilla items ChefZ
files into its own categories without cloning them.

- **item classes**: dairy (`ChefZ_Butter`, `ChefZ_Cheese`, `ChefZ_Cream`, …),
  `ChefZ_Salt` and `ChefZ_RawSalt`, dried herbs and spices, dried berries. The
  knife-cut vegetables (`Chopped*`, `ChefZ_SlicedPotato`) were removed on
  2026-08-29 — recipes take the whole vegetable.
- **3 script files**.
- **Rank 1**: 1 ingredient binding.
- **Rank 2**: **50 records** across 7 documents — 48 ingredients and 2 transforms.
  20 of those ingredient records are vanilla foodstuffs and 3 are vanilla produce:
  they carry no new class, only a category and a tag.
- **`CfgChefZ` slices**: five — `ChefZ_SaltIngredients` (205),
  `ChefZ_Produce` (220, no handcraft slots), `ChefZ_HerbIngredients` (220),
  `ChefZ_SauceIngredients` (230), `ChefZ_DairyIngredients` (260).
- **Depends on**: `DZ_Data`, `DZ_Gear_Food`, `DZ_Gear_Consumables`,
  `ChefZ_Core`, `ChefZ_Farming`, `ChefZ_Processing`.

`ChefZ_Produce` carries the largest handcraft reservation in the mod. See
[Production Chains](Production-Chains).

## `ChefZ_Meat`

Butchery products and the sausage chain.

- **32 item classes**: the three primal cuts, `ChefZ_DicedMeat`, the `Minced*`
  classes, the raw and cooked sausages. The sausage casing is gone since 2026-08-29 —
  vanilla `Guts`/`SmallGuts` fill that role. Diced meat went the same day and
  returned hours later with `beefcubes.p3d`, a model of its own.
- **1 script file** (`ChefZ_MeatItemBase.c`).
- **Rank 1**: none.
- **Rank 2**: **58 records** — 36 ingredient bindings, 16 transforms, 6 recipes.
  The ingredient count is the highest in the mod because this module also
  classifies *vanilla* meat: `PigSteakMeat` through `BearSteakMeat`, plus
  `Lard`, `Bone` and `Guts`. ChefZ creates no own class for those — that would
  be a second version of the same thing — it only files them into categories.
- **`CfgChefZ` slice**: `ChefZ_Meat` (200, **4 handcraft slots** — the three leg
  cuts and dicing).
- **Depends on**: `DZ_Data`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Processing`.

## `ChefZ_Baking`

One dough, pasta, bread. Yeast was removed on 2026-08-29 — a single dough covers
bread, flatbread and pasta.

- **6 item classes**.
- **1 script file** (`ChefZ_BakingItems.c`).
- **Rank 1**: none.
- **Rank 2**: **12 records** — 5 ingredients, 3 transforms, 2 processes
  (`PROCESS_KNEAD`, `PROCESS_ROLL`), 2 recipes.
- **`CfgChefZ` slice**: `ChefZ_GrainBaking` (230, **4 handcraft slots**).
- **Depends on**: `DZ_Data`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Farming`,
  `ChefZ_Processing`.

## `ChefZ_Preservation`

Salting, drying, smoking — and the food-state vocabulary of the whole mod.

- **10 item classes**: `ChefZ_SaltedMeat`, `ChefZ_DriedMeat`, `ChefZ_SmokedMeat`,
  the fish equivalents, `ChefZ_SmokedSausage`, `ChefZ_DrySausage`.
- **1 script file** (`ChefZ_PreservedFood_Base.c`).
- **Rank 1**: **all 10 food states** — `RAW`, `PREPARED`, `COOKED`, `BAKED`,
  `FRIED`, `SALTED`, `SMOKED`, `DRIED`, `BURNT`, `ROTTEN`. States are
  sync-relevant, so rank 1 is not a choice; see
  [Architecture](Architecture#how-the-ranks-merge) and [Food States](Food-States).
- **Rank 2**: **20 records** — 12 ingredients, 8 transforms.
- **`CfgChefZ` slice**: `ChefZ_Preservation` (280, **2 handcraft slots**).
- **Depends on**: `DZ_Data`, `DZ_Gear_Food`, `ChefZ_Core`, `ChefZ_Processing`,
  `ChefZ_Meat`.

This is the module that decides how long everything in ChefZ keeps: `SALTED`
gets a 43 200 s freshness lifetime, `SMOKED` 86 400 s, `DRIED` 129 600 s, and
each of them implies the tag `CHEFZ_PRESERVED`.

## `ChefZ_Cooking`

The largest module. Sauces, broths, tableware and all 28 dishes — 20 plates,
5 bowls and 3 built from vanilla produce alone.

- **44 item classes**: 28 dish classes — one class per dish since 2026-08-29,
  the served dish that forms in the cooking vessel and is eaten; portions are
  vanilla quantity — plus 4 sauces and broths with
  `ChefZ_SauceItemBase`, 5 empty containers with `ChefZ_ContainerItemBase`, the
  two dish base classes (`ChefZ_PortionedDish_Base`, `ChefZ_ServedDish_Base`,
  both `scope = 0`), and 3 forward declarations of vanilla classes.
- **5 script files**, among them `ChefZ_SauceItems.c`, `ChefZ_ServingItems.c`,
  `ChefZ_BowlDishItems.c` and `ChefZ_DishesBItems.c`.
- **Rank 1**: **58 records** — 38 ingredient bindings, 5 quality tiers
  (`POOR`/`SIMPLE`/`PREPARED`/`SEASONED`/`PREMIUM`, tier set `DISH_DEFAULT`),
  5 containers (`PLATE`, `BOWL`, `CAN`, `JAR`, `BOX`), 3 cooking devices
  (`FryingPan` 2 portions, `Pot` 4, `Cauldron` 12), 5 processes and 2 tool
  groups (`AXE_TOOL`, `SAWING_TOOL`). The bindings dropped from 64 with the bulk
  step: a dish that no longer exists twice needs one binding, not two.
- **Rank 2**: **45 records** — 39 of the mod's 47 recipes, plus 5 transforms and
  1 ingredient.
- **`CfgChefZ` slices**: five — `ChefZ_Sauces` (300), `ChefZ_Serving`
  (310, **2 handcraft slots**), `ChefZ_DishesA` (330), `ChefZ_DishesB` (330),
  `ChefZ_DishesC` (340).
- **Depends on**: `DZ_Data`, `DZ_Gear_Food`, `DZ_Gear_Cooking`, `ChefZ_Core`,
  `ChefZ_Ingredients`, `ChefZ_Farming`, `ChefZ_Meat`, `ChefZ_Processing`,
  `ChefZ_Baking`, `ChefZ_Preservation` — every other content addon except
  `ChefZ_Registry`.

This module is a **shared folder**: the slices `sauces`, `serving`, `dishes-a`,
`dishes-b` and `dishes-c` all write into it, each under its own
`### SLICE <name> ###` banner. Anyone adding to it appends and overwrites
nothing.

One inconsistency worth knowing: the twenty `dishes-b` classes have explicit
(empty) script derivations in `ChefZ_DishesBItems.c`, while the twenty
`dishes-a` classes have none. Both work — the engine walks the *config* parent
chain up to `ChefZ_PortionedDish_Base` / `ChefZ_ServedDish_Base`, which do have
script classes — but the two slices are not written the same way.

See [Portions and Containers](Portions-and-Containers) and
[Quality and Nutrition](Quality-and-Nutrition).

## `ChefZ_Cookbook`

Recipe knowledge — which recipes a player has met, and how much of one they know.
Added as Milestone 5.1. **There is no UI yet**: the state, the persistence and the
RPC that would feed a screen all exist, and nothing draws them.

- **2 item classes**: `ChefZ_CookbookItem` and its base. The model is vanilla's
  `book_kniga.p3d` — own geometry for the cookbook is an open item in the asset
  backlog, which is why `DZ_Gear_Books` is a dependency at all.
- **12 script files**: `ChefZ_KnowledgeManager.c`, `ChefZ_KnowledgeState.c`,
  `ChefZ_RecipeStatus.c` and `ChefZ_CookbookRPC.c` in `3_Game`; the item, the
  opener, the server side, the player knowledge, the action and its registration
  in `4_World`; and `ChefZ_CookbookInput.c` in `5_Mission`, the only file of this
  module on that layer — a key has no owner to ask, so it has to be polled, and
  the only per-frame loop is `MissionGameplay.OnUpdate`.
- **A key: F9.** Declared data-driven in `Scripts/Data/Inputs.xml` and wired
  through `inputs =` in `CfgMods`, not through `GetUApi().RegisterInput()` —
  that function exists but has no caller in vanilla 1.29 or in Expansion, and
  using it yields a group that never appears in the controls menu. Pressing it
  without a cookbook in the inventory does nothing; with one it reaches
  `ChefZ_CookbookOpener`, which today only prints that no interface is loaded.
- **Rank 1**: `CfgChefZCookbook > partialMinKnownSlots = 1` — the threshold at
  which a recipe counts as *partially known*. That is a balancing decision of this
  milestone, not a system parameter, which is why it lives here and not in the
  core.
- **No content of its own.** The module names not one recipe and not one
  ingredient; it asks the core's registry at runtime. Adding a dish does not touch
  it.
- **Depends on**: `DZ_Data`, `DZ_Gear_Books`, `ChefZ_Core` — exactly three, and no
  content module among them.

**The RPC guard, and the exploit that made it necessary.** `OnRPC` runs on both the
server and the client, and which object an RPC reaches is decided by whoever sent it.
Until 30.08.2026 a client could send a `FULL_STATE` message to the **server**, aimed at
their own player: `ChefZ_ReceiveFullState` calls `Clear()` and then writes whatever
lists came with the message — every recipe marked as mastered, or the knowledge wiped —
and `OnStoreSave` would persist it. `FULL_STATE` only ever travels server to client, so
one arriving at the server is forged by definition and is now discarded. Vanilla
brackets its own server-to-client branches in `PlayerBase.OnRPC` for the same reason.

The module's RPC numbers are 10000–10002, chosen clear of COT (from 10100), Terje
(negative), Dabs and CF.

The exploit was found by the conflict scout while reviewing the twenty `modded class`
sites, not by a checker — see [Validation](Validation).

`ChefZ_ActionOpenCookbook` is registered in `ChefZ_ActionRegistration.c`. That file
exists because of what `chefzaction.mjs` found: an action class nobody registers
compiles cleanly and never appears in the game. See [Validation](Validation).

## The four asset addons

The delivered geometry, and the only addons in the mod that contain **no code at
all**: no `CfgVehicles` class, no script, no JSON record, no stringtable key. Each is a
`models/` folder, a `data/` folder and a three-line `CfgPatches`, and each depends on
`DZ_Data` alone. Two arrived on 29.08.2026, two more with the second delivery on 30.08.

| Addon | Contents |
|---|---|
| `ChefZ_Devices` | the two hives and the processing stations — butter churn, cheese press, drying rack, grain mill, meat grinder, mortar, smoker |
| `ChefZ_Items` | tools and containers — frames, jar, bee smoker, frying pan |
| `ChefZ_Plants` | the crops and herbs — cabbage, carrot, corn cob and plant, garlic, parsley, red onion, rosemary, thyme |
| `ChefZ_Food` | prepared food |

Together **53 models and 102 texture files — 84 distinct images**, since the eighteen
dishes of 01.09.2026 sit in `ChefZ_Food` under two names. Six further `.p3d` stand
beside them in `models/proxies/` — the five hooks of the drying rack and the plate of
the frying pan. They carry no geometry of their own; they are the places at which
another model is hung, and they were missing from the addons until 03.09.2026. **76 of the mod's 129
spawnable classes** now stand on their own geometry rather than a vanilla proxy — 45 of
them rebound in the second delivery alone. The remaining 53 still point at a vanilla mesh; the standing
backlog is what `tools/chefz-assets/check-todo.mjs` measures.

They exist because a model is not content in the ChefZ sense. A `.p3d` says nothing
about categories, recipes or states — it is a shape a content class points at. Keeping
the shapes in their own PBOs means a content addon can rebind a class from a vanilla
proxy to its own mesh without moving anything else, and the asset packages carry no
dependency of their own in return.

Seven classes were rebound on 29.08.2026:

| Class | Model | Was |
|---|---|---|
| `ChefZ_Beehive` | `beekeeper.p3d` | `wooden_case.p3d` |
| `ChefZ_BeehiveDouble` | `beehive.p3d` | `wooden_case.p3d` |
| `ChefZ_HoneycombFrame_Base` | `honeycomb_frame.p3d` | `birch_bark.p3d` |
| `ChefZ_HoneycombFrameEmpty` | `wooden_frame.p3d` | `birch_bark.p3d` |
| `ChefZ_BeeSmoker` | `beesmoker.p3d` | `food_can_open.p3d` |
| `ChefZ_EmptyJar` | `jar.p3d` | vanilla proxy |
| `ChefZ_Carrot` | `carrot.p3d` | `zucchini.p3d` |

`ChefZ_Corn` (added 29.08.2026 in place of dill) still sits on the inherited
`zucchini.p3d` proxy of `ChefZ_VegetableFood_Base`; its own mesh is open in the asset list.

**The file names of the delivery are swapped.** `beekeeper.p3d` is 0.59 m wide and
1.0 m tall — the single-box hive — while `beehive.p3d` is 1.65 m wide, the double.
The binding follows the measurements, not the names, which is why `ChefZ_Beehive`
points at *beekeeper* and `ChefZ_BeehiveDouble` at *beehive*. Anyone renaming the
files has to swap the two `model =` lines with them.

`beefcubes.p3d` ships but is bound to nothing yet — eight models, seven bindings.

**Neither addon packs today.** Their `$PREFIX$` files carry two-level prefixes and
`pack.mjs` refuses those; the model paths in `ChefZ_Farming` depend on exactly those
prefixes. See [Known Limitations](Known-Limitations#four-asset-addons-that-never-reach-a-pbo).

## Dependency order

Read top to bottom; each module only depends on modules above it.

```
ChefZ_Core
  ChefZ_Cookbook               (depends on the core alone)
  ChefZ_Devices                (assets, depends on DZ_Data alone)
  ChefZ_Items                  (assets, depends on DZ_Data alone)
  ChefZ_Farming                (requires both asset addons)
    ChefZ_Processing
      ChefZ_Ingredients
      ChefZ_Meat
      ChefZ_Baking
        ChefZ_Preservation
          ChefZ_Cooking
            ChefZ_Registry   (depends on all seven content addons)
```

`ChefZ_Registry` sits last in the dependency graph but first in `loadOrder`
(150). Those are two different orderings and both are intentional:
`requiredAddons[]` is about PBO load and class availability, `loadOrder` is
about the sequence in which the config manager reads slices.

## The three compatibility mods

Each is a **separate mod**, not an addon inside `Psyerns_ChefZ_Core`. Each names
its foreign system in `requiredAddons[]`, which means DayZ does not load the PBO
at all when that system is absent — the `modded class` declarations are then
never compiled. ChefZ runs unchanged either way.

None of the three declares a single `CfgVehicles` class.

### `Psyerns_ChefZ_COT_Comp`

Appends eight ChefZ spawn categories to Community Online Tools' object spawner.
An admin tool; no recipe, no nutrition value, no transform, no balancing. The
only intervention is an extra filter in an admin dialog.

- 2 script files, `modded class JMObjectSpawnerForm`.
- Depends on `JM_COT_Scripts` plus the seven ChefZ addons whose classes the
  categories list. `ChefZ_Registry` is deliberately absent — this module reads
  no registry, it only carries class names.
- Every class name is checked against `CfgVehicles` at runtime before it enters
  a category, so a missing ChefZ addon makes its entries vanish silently rather
  than producing ghost classes.

See [COT Compatibility](COT-Compatibility).

### `Psyerns_ChefZ_Terje_Skills_Comp`

Hangs cooking perks into TerjeSkills' existing `surv` skill and feeds ChefZ's
capability and progress registries.

- 10 script files. Two of them use `modded class` on ChefZ's own classes —
  `ChefZ_FreshHerbBase` and the three wild herbs, the only ChefZ classes any comp
  mod extends.
- Depends on `DZ_Data`, `ChefZ_Core`, `ChefZ_Farming`, `TerjeCore`, `TerjeSkills`.
- The whole XP matrix is config, not script, so an operator can override it via
  `$profile:TerjeSettings\Core\GameOverrides.xml`.

### `Psyerns_ChefZ_Terje_Medicine_Comp`

Gives ChefZ teas TerjeMedicine consumable effects.

- 3 script files, `modded class TerjeConsumableEffects`.
- Depends on `TerjeCore`, `TerjeMedicine`, `ChefZ_Core`.
- Values live under an own config root `CfgChefZTerjeMedicine`, not under
  `CfgVehicles`, because re-opening a ChefZ class body would be a duplicate
  class definition and `configcpp.mjs` rejects it.
- **These entries are dormant.** `ChefZ_ThymeTea`, `ChefZ_WildGarlicTea` and
  `ChefZ_HerbalTea` do not exist in the main mod — no `CfgVehicles` class
  anywhere under `Addons/`. Nothing breaks; the resolver simply never matches,
  and `ChefZ_TerjeMedStartupCheck` reports at server start which of the three is
  missing. They will work unchanged once the main mod ships the classes.

See [Terje Compatibility](Terje-Compatibility).

## Related pages

- [Architecture](Architecture) — how the core loads and merges all of this
- [Delta Protocol](Delta-Protocol) — how `ChefZ_Registry` gets its content
- [Adding Content](Adding-Content) — where a new item goes
- [Known Limitations](Known-Limitations) — none of these modules has ever been packed or run
